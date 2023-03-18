#ifndef _WRAPPER_H
#define _WRAPPER_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>
#include <errno.h>


int Socket(int family,int type,int protocol);
int Connect(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
int Bind(int sockfd, const struct sockaddr *servaddr,socklen_t addrlen);
int Listen(int sockfd,int queue_lenght);
int Accept(int sockfd,struct sockaddr *restrict clientaddr,socklen_t *restrict addr_dim);
ssize_t FullRead(int , void *, size_t ); 
ssize_t FullWrite(int , const void *, size_t );
void handler (int );
#endif
