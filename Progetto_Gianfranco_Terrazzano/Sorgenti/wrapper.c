#include "wrapper.h"

int Socket(int family,int type,int protocol){
  int n;
  if((n = socket(family,type,protocol))<0){
    perror("socket");
    exit(1);
  }

  return n;
}

int Connect(int sockfd, const struct sockaddr *addr,socklen_t addrlen){
  if(connect(sockfd,addr,addrlen) < 0){
    perror("connect");
    exit(1);
  }

}

int Bind(int sockfd, const struct sockaddr *servaddr,socklen_t addrlen){
  if(bind(sockfd,servaddr, addrlen) < 0){
    perror("bind");
    exit(1);
  }

}

int Listen(int sockfd,int queue_lenght){
  if(listen(sockfd,queue_lenght) < 0){
    perror("listen");
    exit(1);
  }

}

int Accept(int sockfd,struct sockaddr *restrict clientaddr,socklen_t *restrict addr_dim){
  int n;
  if((n = accept(sockfd,clientaddr, addr_dim))<0){
    perror("accept");
    exit(1);
  }
  return n;
}

//Legge anche se viene interrotta da una System Call.
ssize_t FullRead(int fd, void *buffer, size_t count) {
    size_t n_left;
    ssize_t n_read;
    n_left = count;
    while (n_left > 0) { 
        if ((n_read = read(fd, buffer, n_left)) < 0) {
            if (errno == EINTR) continue; 
            else exit(n_read);
        } else if (n_read == 0) break; 
        n_left -= n_read;
        buffer += n_read;
    }
    buffer = 0;
    return n_left;
}


ssize_t FullWrite(int fd, const void *buffer, size_t count) {
    size_t n_left;
    ssize_t n_written;
    n_left = count;
    while (n_left > 0) {         
        if ((n_written = write(fd, buffer, n_left)) < 0) {
            if (errno == EINTR) continue;  
            else exit(n_written); 
        }
        n_left -= n_written;
        buffer += n_written;
    }
    buffer = 0;
    return n_left;
}

//Handler che cattura il segnale CTRL-C 
void handler (int sign){
    if (sign == SIGINT) {
        printf("\nUscita in corso...\n");
        sleep(2);
        printf("***Grazie per aver utilizzato il nostro servizio di verifica GP***\n");

        exit(0);
    }
}
