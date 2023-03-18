#include "wrapper.h"
#define SIZE 50
#define COD_SIZE 11 //10 caratteri + 1 di terminatore
#define BUFF_SIZE 1024
#define ack 61

//Pacchetto dati del ClientT contenente il numero di codice di tessera sanitaria di un green pass ed la sua validità
typedef struct  {
    char COD[COD_SIZE];
    char check;
} MODIFYGP;

int main(int argc, char **argv){
    int sockfd,recvsize;
    char IDbit, buff[BUFF_SIZE], COD[COD_SIZE]; //Anche qui utilizzeremo lo stesso meccanismo di IDbit per identificarci al serverG
    struct sockaddr_in s_addr;
    MODIFYGP gp;
    
    if(argc != 2){
        fprintf(stderr,"usage: %s  <TesseraSanitaria>\n",argv[0]);
        exit(1);
    }

    if(strlen(argv[1]) != 10){
        fprintf(stderr,"Tessera Sanitaria non valida \n");
        exit(1);
    }
   strcpy(gp.COD,argv[1]);
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
    IDbit='1'; //imposto l'IDBit a 1 così da identificarmi come ClientT
    //Invia un bit di valore 1 al ServerG 
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
    if (FullRead(sockfd, buff, recvsize) < 0){
        perror("FullRead() error");
        exit(1);
    }

    if (fputs(buff, stdout) == EOF) {
	    fprintf(stderr,"fputs error\n");
	    exit(1);
    }

     while (1) {
        printf("Inserire 0 se il green pass non è valido\nInserire 1 se il gren pass è valido\n Esito: ");
        scanf("%c", &gp.check);

        if (gp.check == '1'){ break;} 
        else if (gp.check == '0'){ break;}
        printf("Input Errato, riprovare...\n\n");
    }

    //Invia pacchetto  al ServerG
    if (FullWrite(sockfd, &gp, sizeof(MODIFYGP)) < 0) {
        perror("FullWrite() error");
        exit(1);
    }
    //Riceve messaggio di ack dal ServerG
    if (FullRead(sockfd, buff, ack) < 0) {
        perror("FullRead() error");
        exit(1);
    }

      //Riceve messaggio di report dal ServerG
    if (FullRead(sockfd, buff, ack) < 0) {
        perror("FullRead() error");
        exit(1);
    }

    //Simuliamo il carciamento
    sleep(2);
    printf("%s\n", buff);

    close(sockfd);

    exit(0);
}