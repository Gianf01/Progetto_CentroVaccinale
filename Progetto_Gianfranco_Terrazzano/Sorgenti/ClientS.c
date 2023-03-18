#include "wrapper.h"
#define SIZE 50
#define COD_SIZE 11 //10 caratteri + 1 di terminatore
#define BUFF_SIZE 1024
#define ack 61

/*Il clientS si comporterà similmente al client, la differenza è che non avrà 
bisogno di utilizzare un paccheto dati in quanto deve passare solo il codice della tessera
a un ServerG che sarà in comunicazione con il serverV e ci dirà se questo codice è valido o meno */

int main(int argc, char **argv){
    int sockfd,recvsize;
    char IDbit, buff[BUFF_SIZE], COD[COD_SIZE]; //Anche qui utilizzeremo lo stesso meccanismo di IDbit per identificarci al serverG
    struct sockaddr_in s_addr;
    
    if(argc != 2){
        fprintf(stderr,"usage: %s  <TesseraSanitaria>\n",argv[0]);
        exit(1);
    }

    if(strlen(argv[1]) != 10){
        fprintf(stderr,"Tessera Sanitaria non valida \n");
        exit(1);
    }

   strcpy(COD,argv[1]);
    //Setto il client
    sockfd=Socket(AF_INET, SOCK_STREAM, 0);
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(1027); 

    //inet_pton Converte dell’indirizzo IP in network order.
    if (inet_pton(AF_INET, "127.0.0.1", &s_addr.sin_addr) <= 0) {
        perror("inet_pton() error");
        exit(1);
    }

    Connect(sockfd,(struct sockaddr *)&s_addr,sizeof(s_addr));

    IDbit='0'; //imposto l'IDBit a 0 così da identificarmi come ClientS
    //Invia un bit di valore 0 al ServerG 
    if (FullWrite(sockfd, &IDbit, sizeof(char)) < 0) {
        perror("FullWrite() error");
        exit(1);
    }

     /*Ora sarà stampato un messaggio dal ServerG, pertanto 
    calcola la dimensione richiesta. la prima ricezione lo fa con la dimensione della richiesta, 
    dopodichè riceve la richiesta vera e propria, così che non si blocchi e conosciamo esattamente il num. di byte*/
    if (FullRead(sockfd, &recvsize, sizeof(int)) < 0){
        perror("FullRead() error");
        exit(1);
    }
    //Stampo il benvenuto del ServerG
    if (FullRead(sockfd, buff, recvsize) < 0){
        perror("FullRead() error");
        exit(1);
    }
    if (fputs(buff, stdout) == EOF) {
	    fprintf(stderr,"fputs error\n");
	    exit(1);
    }

    //Invio del codice della tessera da convalidare al serverG
    if (FullWrite(sockfd, COD, COD_SIZE)) {
        perror("FullWrite() error");
        exit(1);
    }

    //Meccanismo dell'ack come l'utente
    if (FullRead(sockfd, buff, ack) < 0) {
        perror("FullRead error");
        exit(1);
    }
    printf("\n%s\n", buff);

    printf("Verifica in corso...\n\n");

    //Simuliamo il caricamento
    sleep(2);
    
    //Riceve esito scansione Green Pass dal ServerG
    if (FullRead(sockfd, buff, sizeof(buff)) < 0) {
        perror("FullRead() error");
        exit(1);
    }
    printf("%s\n", buff);

    close(sockfd);

    exit(0);
    
}