#include "wrapper.h"
#include <netdb.h>
#define SIZE 50
#define COD_SIZE 11 //10 caratteri + 1 di terminatore
#define BUFF_SIZE 1024
#define ACK_SIZE 61 
//#define ack 56
//Utilizzeremo il concetto di pacchetto similmente all'esercitazione della battaglia navale

//Definiamo il pacchetto dati con le generalità dell'utente
typedef struct{
    char nome[SIZE];
    char cognome[SIZE];
    char COD[COD_SIZE];
} GEN;

//Funzione per valorizzare la struttura GEN
GEN my_gen();

int main(int argc, char **argv){
    int sockfd, recvsize,ACKsize;
    struct sockaddr_in s_addr;
    char buff[BUFF_SIZE], ack[ACK_SIZE];
    GEN mygen;
    //variabili per utilizzare gethostbyname
    struct hostent *data;
    char *addr;
    char **alias;

    if(argc !=2){
        perror("usage: <host name>"); 
        exit(1);
    }

    //Utilizzerò le funzioni presente in wrapper.h per creare il client
    sockfd=Socket(AF_INET, SOCK_STREAM, 0);
    
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(1024);

    if ((data = gethostbyname(argv[1])) == NULL) {  //gethostbyname ci permetterà di ricavare l'indirizzo IP dal nome
        herror("gethostbyname() error"); //herror si comporta similmente a perror ma è apposita per le strutture hostent
		exit(1);
    }
	alias = data -> h_addr_list;

    //inet_ntop converte un indirizzo in una stringa dotted decimal.
    if ((addr = (char *)inet_ntop(data -> h_addrtype, *alias, buff, sizeof(buff))) < 0) {
        perror("inet_ntop() error");
        exit(1);
    }

    //inet_pton Converte dell’indirizzo IP in network order.
    if (inet_pton(AF_INET, addr, &s_addr.sin_addr) <= 0) {
        perror("inet_pton() error");
        exit(1);
    }

    Connect(sockfd,(struct sockaddr *)&s_addr,sizeof(s_addr));

    /*Ora sarà stampato un messaggio dal Centro Vaccinale, pertanto 
    calcola la dimensione richiesta. la prima ricezione lo fa con la dimensione della richiesta, 
    dopodichè riceve la richiesta vera e propria, così che non si blocchi e conosciamo esattamente il num. di byte*/
    if (FullRead(sockfd, &recvsize, sizeof(int)) < 0){
        perror("FullRead() error");
        exit(1);
    }
    //Stampo il benvenuto del centro vaccinale
    if (FullRead(sockfd, buff, recvsize) < 0){
        perror("FullRead() error");
        exit(1);
    }
    if (fputs(buff, stdout) == EOF) {
	    fprintf(stderr,"fputs error\n");
	    exit(1);
    }

    mygen=my_gen(); //Creo il pacchetto da inviare al centro
    if (FullWrite(sockfd, &mygen, sizeof(GEN)) < 0){
        perror("FullWrite() error");
        exit(1);
    }

    /*Per garantire l’affidabilità il TCP introduce l’ACK
    (acknowledge): per ogni byte inviato ildestinatario deve confermare la ricezione del byte. A invia a B, quando B riceve il byte, B
    risponde con un ACK della ricezione di quel byte specifico*/
    //Ricezione dell'ack
      if (FullRead(sockfd, &ACKsize, sizeof(int)) < 0){
        perror("FullRead() error");
      }
        
    if (FullRead(sockfd, ack, ACKsize) < 0) {
        perror("full_read() error");
        exit(1);
    }
    

    printf("%s\n\n", ack);
    exit(0);
}

GEN my_gen(){
    GEN pacchetto;

    printf("Inserisci nome: ");
    if (fgets(pacchetto.nome, BUFF_SIZE, stdin) == NULL) {
        perror("fgets() error");
    }
    pacchetto.nome[strlen(pacchetto.nome) - 1] = 0; //Inseriamo il terminatore al posto dell'invio inserito dalla fgets

    printf("Inserisci cognome: ");
    if (fgets(pacchetto.cognome, BUFF_SIZE, stdin) == NULL) {
        perror("fgets() error");
    }
    pacchetto.cognome[strlen(pacchetto.cognome) - 1] = 0; //Inseriamo il terminatore al posto dell'invio inserito dalla fgets

    while(1){
        printf("Inserisci codice: ");
        if (fgets(pacchetto.COD, BUFF_SIZE, stdin) == NULL) {
            perror("fgets() error");
        }
        if (strlen(pacchetto.COD)!=COD_SIZE){
            printf("Il codice è lungo 10 caratteri, riprova!\n\n");
        }else{
            pacchetto.COD[COD_SIZE - 1] = 0; //Inseriamo il terminatore al posto dell'invio inserito dalla fgets
            break;
        }
    }
    return pacchetto;
}