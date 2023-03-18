#include "wrapper.h"
#include <netdb.h>  
#define COD_SIZE 11 //10 caratteri + 1 di terminatore
#define BUFF_SIZE 1024
#define SIZE 50
#define ACK_SIZE 61 

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

//pacchetto da inviare al ServerV
typedef struct{
    char COD[COD_SIZE];
    DATE data_scadenza;
    DATE data_emissione;
}PACKGP;

//Costruiremo principalmente 4 fuznioni -get data di emissione/scadenza - Invio dati al ServerV -Comunicazione con il client
DATE get_dataE(DATE *data_emissione);
DATE get_dataS(DATE *data_scadenza);
void InvioDati(PACKGP pass);
void communication(int conn_fd);

int main(int argc, char const *argv[]) {
    int listfd, conn_fd;
    struct sockaddr_in s_addr;
    pid_t pid;
    GEN mygen;
    signal(SIGINT,handler); //Gestiamo il segnale CTRL_C

    //Utilizziamo le funzioni in wrapper.h 

    listfd= socket(AF_INET, SOCK_STREAM, 0);

    //Valorizzazione struttura
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    s_addr.sin_port = htons(1024);

    Bind(listfd, (struct sockaddr*)&s_addr, sizeof(s_addr));
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
            communication(conn_fd);
            close(conn_fd);
            exit(0);
        } 
        else close(conn_fd);
    }
    exit(0);
}

//funzione che definisce la data di emissione del GP
DATE get_dataE(DATE *data_emissione){
    time_t now=time(NULL);
    struct tm *emissione = localtime(&now);

    data_emissione->day = emissione->tm_mday;
    data_emissione->month = emissione->tm_mon + 1;
    data_emissione->year = emissione->tm_year + 1900;
    /*Gli anni partono dal 123 quindi al 2023 si fa +1900
    aggiungiamo +1 al mese perchè vanno da 0 a 11 */ 

    printf("La data di emissione del green pass e': %02d:%02d:%02d\n", data_emissione->day, data_emissione->month, data_emissione->year);
}

//funzione che calcola la data di scadenza del GP
DATE get_dataS(DATE *data_scadenza) {
    //Sfruttiamo la libreria time.h
    time_t now = time(NULL); //time_t è un tipo di dati definito in time.h che rappresenta un timestamp, ovvero un numero di secondi trascorsi dal 1 gennaio 1970
    struct tm *scadenza= localtime(&now);  
    scadenza->tm_mon += 3; // 3 mesi di validità
    if (scadenza->tm_mon >= 12) { 
        scadenza->tm_year++;
        scadenza->tm_mon %= 12;
    }
    data_scadenza->day = scadenza->tm_mday;
    data_scadenza->month = scadenza->tm_mon + 1;
    data_scadenza->year = scadenza->tm_year + 1900;
	/*Gli anni partono dal 123 quindi al 2023 si fa +1900
    aggiungiamo +1 al mese perchè vanno da 0 a 11 */ 
    printf("La data di scadenza del green pass e': %02d/%02d/%04d\n", data_scadenza->day, data_scadenza->month, data_scadenza->year);
}

//Funzione che invia i dati al serverV
void InvioDati(PACKGP pass){
    int sockfd;
    struct sockaddr_in addr;
    char IDbit;//L'IDbit servirà per identificarci con il ServerV

    //Utilizzerò le funzioni presente in wrapper.h 
    sockfd=Socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family=AF_INET;
    addr.sin_port=htons(1026); //Utilizziamo una porta diversa da quella per comunicare con il client

    //inet_pton Converte dell’indirizzo IP in network order.
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
        perror("inet_pton() error");
        exit(1);
    }

    Connect(sockfd,(struct sockaddr *)&addr,sizeof(addr));
     IDbit='1'; //Settiamo IDbit a 1 per dire al ServerV che sta comunicando con il centro vaccinale
    if (FullWrite(sockfd, &IDbit, sizeof(char)) < 0){
        perror("FullWrite() error");
        exit(1);
    }

    //Iniviamo il pacchetto dati al ServerV
    if (FullWrite(sockfd, &pass, sizeof(pass)) < 0){
        perror("FullWrite() error");
        exit(1);
    }
    close(sockfd);
}

void communication(int conn_fd){
    char buff[BUFF_SIZE],ack[ACK_SIZE];
    int IDCentro,mex_size;
    PACKGP pass;
    GEN mygen;

    srand(time(NULL)); //inizializzo il seed
    IDCentro= rand()% 10; //Do casualmente un numero per identificare il centro
    snprintf(buff, BUFF_SIZE, "Benvenuto nel centro vaccinale N° : %d! \nDigita le tue generalità per inserirle sulla piattaforma.\n", IDCentro);
     
     /*Ora sarà inviato un messaggio dal Centro Vaccinale, pertanto 
    calcola la dimensione richiesta. Il primo invio lo fa con la dimensione della richiesta, 
    dopodichè invia la richiesta vera e propria, così che non si blocchi e conosciamo esattamente il num. di byte*/
    mex_size=sizeof(buff);
    if (FullWrite(conn_fd, &mex_size , sizeof(int)) < 0){
        perror("FullWrite() error");
        exit(1);
    }

    if (FullWrite(conn_fd, buff, mex_size) < 0){
        perror("FullWrite() error");
        exit(1);
    }

    //Riceviamo il pacchetto delle generalità
    if(FullRead(conn_fd, &mygen, sizeof(GEN)) < 0) {
        perror("FullRead error");
        exit(1);
    }

    printf("\nDati ricevuti\n");
    printf("Nome: %s\n", mygen.nome);
    printf("Cognome: %s\n", mygen.cognome);
    printf("Numero Tessera Sanitaria: %s\n\n", mygen.COD);

    //Inviamo un ack di avvenuta ricezione
    /*Per garantire l’affidabilità il TCP introduce l’ACK
    (acknowledge): per ogni byte inviato ildestinatario deve confermare la ricezione del byte. A invia a B, quando B riceve il byte, B
    risponde con un ACK della ricezione di quel byte specifico*/

  
    snprintf(ack, ACK_SIZE, "I tuoi dati sono stati correttamente inseriti in piattaforma");
    mex_size=sizeof(ack);
    if (FullWrite(conn_fd, &mex_size , sizeof(int)) < 0){
        perror("FullWrite() error");
        exit(1);
    }
    if(FullWrite(conn_fd, ack, mex_size) < 0) {
        perror("full_write() error");
        exit(1);
    }
    

    //Creo il green pass da inviare al serverV
    strcpy(pass.COD, mygen.COD);
    get_dataE(&pass.data_emissione);
    get_dataS(&pass.data_scadenza);

    close(conn_fd);

    //Invio i dati al ServerV
    InvioDati(pass);
}





