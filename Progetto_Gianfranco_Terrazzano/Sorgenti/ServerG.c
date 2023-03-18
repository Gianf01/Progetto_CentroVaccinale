#include "wrapper.h"
#define COD_SIZE 11 //10 caratteri + 1 di terminatore
#define BUFF_SIZE 1024
#define ack 61
#define SIZE 50

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


DATE get_date();
char VerifyCOD(char COD[]);
char date_check(PACKGP *);
void clientS_comm(int conn_fd);
void GetDataClientT(int conn_fd);
char Mod_GP (MODIFYGP checkgp);

int main() {
    char IDbit;
    int listfd, conn_fd;
    struct sockaddr_in sv_addr;
    pid_t pid;
    
    signal(SIGINT,handler); 

    //Utilizzo la libreria wrapper.h

    listfd= socket(AF_INET, SOCK_STREAM, 0);

    //Valorizzazione struttura
    sv_addr.sin_family = AF_INET;
    sv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sv_addr.sin_port = htons(1027);

    Bind(listfd, (struct sockaddr*)&sv_addr, sizeof(sv_addr));
    Listen(listfd,1024);

    for (;;) {
    printf("In attesa...\n");
       //Accetta una nuova connessione
        conn_fd = Accept(listfd,(struct sockaddr *)NULL, NULL);
        //Creazione del figlio;
        if ((pid = fork()) < 0) {
            perror("fork() error");
            exit(1);
        }

        if (pid == 0) {
            close(listfd);

            /*
                IDbit serve a identificare le connessioni con il client
                se 1 gestiamo la connessione con il clientS
                se 0 la connessione con il ClienT
            */
            if (FullRead(conn_fd, &IDbit, sizeof(char)) < 0) {
                perror("FullRead() error");
                exit(1);
            }
            if (IDbit == '1') 
                GetDataClientT(conn_fd);
            else if (IDbit == '0') clientS_comm(conn_fd);
            else printf("Client non riconosciuto\n");

            close(conn_fd);
            exit(0);
        } else close(conn_fd);
    }
    exit(0);
}

/*Tra le funzionalità di questo server, dovremo occuparci del verificare la validità comunicando con il serverV
    pertanto sarà fondamentale creare una funzione che estrapoli la data corrente in modo da verificare la validità
    del Green Pass */
DATE get_date(){
    time_t ticks = time(NULL);
    struct tm *localdate = localtime(&ticks);
    DATE current_date = {localdate->tm_mday, localdate->tm_mon+1, localdate->tm_year+1900};
 //i mesi vanno da 0 a 11, quindi sommiamo 1, gli anni partono da 123 quindi ad arrivare a 2023 sommiamo 1900
    return current_date;
}

//Funzione che controlla la data di scadenza del Green Pass e ci restituisce il check
char date_check(PACKGP *gp){
    DATE current_date;
    char check;
    current_date=get_date();
    if (current_date.year > gp->data_scadenza.year || (current_date.year == gp->data_scadenza.year && current_date.month > gp->data_scadenza.month) 
    || (current_date.year == gp->data_scadenza.year && current_date.month == gp->data_scadenza.month && current_date.day > gp->data_scadenza.day) || gp->check == '0') {
        check = '0';
    } else {
        check= '1';
    }
    return check;
}

//Procediamo all'implementazione di una funzione che aggiorni il valore check, così da sapere se il green pass è valido o meno
char VerifyCOD(char COD[]){
    DATE current_date;
    int sockfd, mexsize, packsize;
    char buff[BUFF_SIZE],check, IDbit, ModBit;
    struct sockaddr_in s_addr;
    PACKGP mygp;

    //Setto il client
    sockfd=Socket(AF_INET, SOCK_STREAM, 0);
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(1026); //Stessa porta del ServerV 

    //inet_pton Converte dell’indirizzo IP in network order.
    if (inet_pton(AF_INET, "127.0.0.1", &s_addr.sin_addr) <= 0) {
        perror("inet_pton() error");
        exit(1);
    }

    Connect(sockfd,(struct sockaddr *)&s_addr,sizeof(s_addr));

    //Inviamo un bit di valore 0 per comunicargli che siamo il serverG
    IDbit='0'; 
    if (FullWrite(sockfd, &IDbit, sizeof(char)) < 0){
        perror("FullWrite() error");
        exit(1);
    }

    //Inviamo un bit di valore 1 per comunicare che deve verificare un GP, quindi richiediamo il GP completo
    ModBit= '1';
    if (FullWrite(sockfd, &ModBit, sizeof(char)) < 0){
        perror("FullWrite() error");
        exit(1);
    }

    //Invia il codice di tessera sanitaria al ServerV
    if (FullWrite(sockfd, COD, COD_SIZE)) {
        perror("FullWrite() error");
        exit(1);
    }

    //Riceve check dal ServerVaccinale
    if (FullRead(sockfd, &check, sizeof(char)) < 0) {
        perror("FullRead() Error");
        exit(1);
    }

    if (check == '1') {
        if (FullRead(sockfd, &mygp, sizeof(PACKGP)) < 0) { //prendiamo il GP completo
            perror("FullRead() error");
            exit(1);
        }
        check = date_check(&mygp);
    }
      
    close(sockfd);
    
    //Verifico la data di scadenza del GP e modifico il check    
    return check;
}


void clientS_comm(int conn_fd){
    char buff[BUFF_SIZE], COD[COD_SIZE],check;
    int mex_size, pack_size;

    snprintf(buff, BUFF_SIZE, "***Benvenuto nel ServerG! Testeremo la validità del tuo codice della tessera sanitaria\n");
    buff[BUFF_SIZE-1]= 0;
     /*Ora sarà inviato un messaggio dal ServerG, pertanto 
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

    //Ricevo il CODICE dal ClientS
    if (FullRead(conn_fd, COD, COD_SIZE) <0 ){
        perror("FullRead() error");
        exit(1);
    }

     //Notifica all'utente la corretta ricezione dei dati che aveva inviato.
    snprintf(buff,ack, "**Il codice è stato ricevuto!");
    buff[ack- 1] = 0;
    if(FullWrite(conn_fd, buff, ack) < 0) {
        perror("FullWrite() error");
        exit(1);
    }

    check = VerifyCOD(COD);

    if (check == '1') {
        strcpy(buff, "Green Pass valido, operazione conclusa");
    } else if (check == '0') {
        strcpy(buff, "Green Pass non valido, uscita in corso");
    } else {
        strcpy(buff, "Numero tessera sanitaria non esistente");
    }

    //Invio il risultato del check al ClientS
    if (FullWrite(conn_fd, buff, sizeof(buff)) < 0) {
        perror("FullWrite() error");
        exit(1);
    }

    close(conn_fd);
}
//Funzione che invia il Green Pass da aggiornare al ServerV che memorizza i vari GP
char Mod_GP (MODIFYGP checkgp){
    int sockfd;
    char IDbit, buff[BUFF_SIZE], valid, ModBit;
    struct sockaddr_in addr;
    sockfd=Socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family=AF_INET;
    addr.sin_port=htons(1026); 

    //inet_pton Converte dell’indirizzo IP in network order.
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
        perror("inet_pton() error");
        exit(1);
    }

    Connect(sockfd,(struct sockaddr *)&addr,sizeof(addr));
    IDbit='0'; //Settiamo IDbit a 0 per dire al ServerV che sta comunicando con il ServerG
     if (FullWrite(sockfd, &IDbit, sizeof(char)) < 0){
        perror("FullWrite() error");
        exit(1);
    }

    ModBit='0'; //Settiamo ModBit a 0 per dire al ServerV che dobbiamo modifcare un Green Pass
     if (FullWrite(sockfd, &IDbit, sizeof(char)) < 0){
        perror("FullWrite() error");
        exit(1);
    }

    //Inviamo il Green Pass da modificare
    if (FullWrite(sockfd, &checkgp, sizeof(MODIFYGP)) <0){
        perror("FullWrite() error");
        exit(1);
    }

    //Riceviamo la validità del GreenPass, ovvero se il codice esiste e se è stato aggiornato
    if (FullRead (sockfd, &valid, sizeof(char)) < 0){
        perror("FullRead() error");
        exit(1);
    }
    
    close(sockfd);
    return valid;   
}

//Funzione che prende il GreenPass da modificare dal clienT
void GetDataClientT(int conn_fd){
   char Valid, buff[BUFF_SIZE];
   int mexsize;
   MODIFYGP checkgp;
   //Stampo il messaggio di benvenuto al ClienT
   snprintf(buff,BUFF_SIZE, "**Benvenuto al ServerG modificheremo questo green pass \n");
   mexsize=sizeof(buff);
    if (FullWrite(conn_fd, &mexsize , sizeof(int)) < 0){
        perror("FullWrite() error");
        exit(1);
    }
    if (FullWrite(conn_fd, buff, mexsize) < 0){
        perror("FullWrite() error");
        exit(1);
    }

    //legge il pacchetto
    if (FullRead(conn_fd, &checkgp, sizeof(MODIFYGP)) < 0){
        perror("FullWrite() error");
        exit(1);
    }


    //Notifica all'utente la corretta ricezione dei dati che aveva inviato.
    snprintf(buff,ack, "**Il codice è stato ricevuto! \n ");
    buff[ack- 1] = 0;
    if(FullWrite(conn_fd, buff, ack) < 0) {
        perror("FullWrite() error");
        exit(1);
    }

    Valid = Mod_GP(checkgp);  //Valid ci restituisce 1 se il GP non esiste, 0 se esiste
    if (Valid=='1'){
        strcpy(buff, "il codice non è associato ad alcun Green Pass! \n");
        if (FullWrite(conn_fd, buff, sizeof(buff)) < 0){
            perror("FullWrite() error");
            exit(1);
        }
    }else {
        strcpy(buff, "Modifica avvenuta con successo! \n");
        if (FullWrite(conn_fd, buff, sizeof(buff)) < 0){
            perror("FullWrite() error");
            exit(1);
        }
    }
}



