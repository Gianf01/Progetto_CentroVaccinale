#include "wrapper.h"
#include <netdb.h>  
#include <fcntl.h> //permette l'utilizzo dei file
#include <sys/file.h> //per l'utilizzo di flock
#define COD_SIZE 11 //10 caratteri + 1 di terminatore
#define BUFF_SIZE 1024
#define ack 61
#define SIZE 50

//pacchetto dati dell'utente
typedef struct{
    char nome[SIZE];
    char cognome[SIZE];
    char COD[COD_SIZE];
} GEN;

//Struct che serve a formare un tipo data
typedef struct {
    int day;
    int month;
    int year;
} DATE;

typedef struct{
    char COD[COD_SIZE];
    DATE data_scadenza;
    DATE data_emissione;
    char check; //Aggiungiamo un campo che indica la validità del nostro GP
}PACKGP;

typedef struct  {
    char COD[COD_SIZE];
    char check;
} MODIFYGP;


//L'obbiettivo di questa funzione è salvare un nuovo green pass in un file
void CV_recv(int sockfd);
//Invia il pacchetto di green_pass richiesto dal ServerG
void InvioGP(int conn_fd);
//Funzione che aggiorna la validità di un GP per conto del ServerG
void Mod_GP(int conn_fd);
int main(){
    int listfd, conn_fd;
    struct sockaddr_in sv_addr;
    pid_t pid;
    char IDbit,ModBit;
    signal(SIGINT,handler);

    //Utilizziamo le funzioni in wrapper.h 

    listfd= socket(AF_INET, SOCK_STREAM, 0);

    //Valorizzazione struttura
    sv_addr.sin_family = AF_INET;
    sv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sv_addr.sin_port = htons(1026);

    Bind(listfd, (struct sockaddr*)&sv_addr, sizeof(sv_addr));
    Listen(listfd,1024);

    for (;;) {

    printf("In attesa...\n");

        //Accetta una nuova connessione
        conn_fd = Accept(listfd,(struct sockaddr *)NULL, NULL);

        //Creazione del figlio;
        //Il padre si occupa di attendere connesioni al server, il figlio di gestirle.
        if ((pid = fork()) < 0) {
            perror("fork() error");
            exit(1);
        }
        if (pid == 0) {  
            close(listfd);
            if(FullRead(conn_fd,&IDbit, sizeof(char)) < 0){
                perror("FullRead() error");
                exit(1);
            }
            if (IDbit == '1') CV_recv(conn_fd);
            else if (IDbit == '0'){
                 if(FullRead(conn_fd,&ModBit, sizeof(char)) < 0){
                    perror("FullRead() error");
                    exit(1);
                }
                if (ModBit == '1'){ 
                    InvioGP(conn_fd);
                }else if(ModBit == '0'){
                    Mod_GP(conn_fd);
                }else printf("Operazione non valida \n");
            }else printf("Client non riconsociuto \n");
            close(conn_fd);
            exit(0);
        } 
        else close(conn_fd);
    }
    exit(0);
}

void CV_recv(int sockfd){
    int fd,written_bytes;
    PACKGP green_pass;
    char file_name[COD_SIZE];
    //Ricevo i dati con una fullread
    if(FullRead(sockfd,&green_pass,sizeof(PACKGP)) < 0){
        perror("FullRead() error");
        exit(1);
    }
    //Dato che stiamo aggiungendo un green pass nuovo settiamo a 1 il valore check, ovvero è valido
    green_pass.check= '1';
     // Creo il nome del file
    snprintf(file_name, COD_SIZE, "%s", green_pass.COD);

    // Apro il file in modalità di scrittura
    fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (fd == -1) {
        perror("Open error");
        exit(1);
    }

     // Scrivo i dati del green pass sul file
    written_bytes = write(fd, &green_pass, sizeof(PACKGP));
    if (written_bytes == -1) {
        perror("Write error");
        exit(1);
    }

    if (close(fd) == -1){
        perror("Close error");
        exit(1);
    }
}

void InvioGP(int conn_fd){
    PACKGP mygp;
    char COD[COD_SIZE],check;
    int fd;
    //Leggiamo il cod dal ServerG
    if (FullRead(conn_fd, COD, COD_SIZE) < 0){
        perror("FullRead() error");
        exit(1);
    }
    fd = open(COD, O_RDONLY, 0777);

    //Se il file di nome COD non esiste allora inviamo un check diverso da 0 e 1 
    // Verifica se il file esiste
    if (access(COD, F_OK) == -1) { 
        /*La costante F_OK specifica che si vuole verificare solo la presenza del file.   
        Se il file esiste, la funzione access restituirà 0, altrimenti restituirà -1. */
        check = '2';
        //Invia il check al serverG
        if (FullWrite(conn_fd, &check, sizeof(char)) < 0) {
            perror("FullWrite() error");
            exit(1);
        }
    } else {
        if (flock(fd, LOCK_EX) < 0) { //Flock ci permette di accedere in maniera mutualmente esclusiva a un file tramite un lock
            perror("flock() error");
            exit(1);
        }
        //Estrapoliamo il green pass dal file fd
        if (read(fd, &mygp, sizeof(PACKGP)) < 0) {
            perror("read() error");
            exit(1);
        }
        //Sblocchiamo il lock
        if(flock(fd, LOCK_UN) < 0) {
            perror("flock() error");
            exit(1);
        }

        close(fd);
        check = '1';

        //Invia il check al ServerG
        if (FullWrite(conn_fd, &check, sizeof(char)) < 0) {
            perror("FullWrite() error");
            exit(1);
        }

        //Mandiamo il green pass richiesto al ServerG che controllerà la sua validità
        if(FullWrite(conn_fd, &mygp, sizeof(PACKGP)) < 0) {
            perror("FullWrite() error");
            exit(1);
        }
    }
}

//La funzione consiste nel controllare l'esistenza del file contenente il codice inviato dal ServerG e modificare il check
void Mod_GP(int conn_fd){
    MODIFYGP modgp;
    PACKGP mygp;
    char buff[BUFF_SIZE], valid;
    int fd;
    //per prima cosa prendiamo il codice inviato dal ServerG
     if (FullRead(conn_fd, &modgp, sizeof(MODIFYGP)) < 0){
        perror("FullRead() error");
        exit(1);
    }
    //Verifichiamo se esiste un file associato a questo codice
    fd = open(modgp.COD, O_RDWR, 0777);
    
    //Se il file di nome COD non esiste allora inviamo un Valid = 1 al ServerG c
    // Verifica se il file esiste
    if (access(modgp.COD, F_OK) == -1) { 
        /*La costante F_OK specifica che si vuole verificare solo la presenza del file.   
        Se il file esiste, la funzione access restituirà 0, altrimenti restituirà -1. */
        valid= '1';
        //Invia il Valid al serverG
        if (FullWrite(conn_fd, &valid, sizeof(char)) < 0) {
            perror("FullWrite() error");
            exit(1);
        }
    } else {
        //Accediamo in maniera mutuamente esclusiva al file in lettura
        if(flock(fd, LOCK_EX | LOCK_NB) < 0) {
            perror("flock() error");
            exit(1);
        }
        //Estrapoliamo il green pass del codice ricevuto dal ServerG
        if (read(fd, &mygp, sizeof(PACKGP)) < 0) {
            perror("read() error");
            exit(1);
        }

        //Assegna il valore di check ricevuto dal ServerG precedentemente inviato dal ClientT
        mygp.check =modgp.check;

        //Ci posizioniamo all'inizio dello stream del file
        lseek(fd, 0, SEEK_SET);

        //Andiamo a sovrascrivere i campi di GP nel file binario con nome il numero di tessera sanitaria del green pass
        if (write(fd, &mygp, sizeof(PACKGP)) < 0) {
            perror("write() error");
            exit(1);
        }
        //Sblocchiamo il lock
        if(flock(fd, LOCK_UN) < 0) {
            perror("flock() error");
            exit(1);
        }
         valid = '0';
         //Invia il Valid al ServerG
        if (FullWrite(conn_fd, &valid, sizeof(char)) < 0) {
        perror("FullWrite error");
        exit(1);
        }
    }
}