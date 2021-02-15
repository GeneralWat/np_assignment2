#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
/* You will to add includes here */


// Included to get the support library
#include <calcLib.h>

#define MAXDATASIZE 1000

#include "protocol.h"

int main(int argc, char *argv[]){
  
  if(argc != 2){
    printf("Usage:; %s <ip>:<port> \n", argv[0]);
    exit(1);
  }

  char delim[]=":";
  char *Desthost=strtok(argv[1],delim);
  char *Destport=strtok(NULL,delim);
  /* Do magic */
  int port = atoi(Destport);

  struct addrinfo hints, *servinfo, *p;
  int rv;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  int sockfd;


  if((rv = getaddrinfo(Desthost, Destport, &hints, &servinfo)) != 0){
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  for(p = servinfo; p != NULL; p = p->ai_next) {
    if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("talker: socket");
      continue;
    }
    break;
  }
  if(p == NULL){
    fprintf(stderr, "Failed to create socket. \n");
    exit(1);
    freeaddrinfo(servinfo);
  }

  struct calcMessage calcMsg;
  calcMsg.type = htons(22); //Denna ska vara 22 för att få okej
  calcMsg.message = htonl(0);
  calcMsg.protocol = htons(17);
  calcMsg.major_version = htons(1);
  calcMsg.minor_version = htons(0);
  struct timeval timeout;
  timeout.tv_sec = 2;

  struct calcProtocol calcProt;
  memset(&calcProt, 0, sizeof(calcProt));

  int attempts= 0;
  ssize_t sentbytes;
  int numbytes;
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));

  for(int i = 0; i < 3; ++i){
    if((sentbytes = sendto(sockfd, &calcMsg, sizeof(calcMsg), 
      0, p->ai_addr, p->ai_addrlen)) == -1){
      close(sockfd);
      fprintf(stderr, "Something went wrong when sending");
      break;
      exit(1);
    }
    printf("Sending %ld bytes \n", sentbytes);
    if((numbytes = recvfrom(sockfd, &calcProt, sizeof(calcProt), 0, 
      p->ai_addr, &p->ai_addrlen)) == -1){
        perror("recvfrom");
        exit(1);
    }
    printf("I got %d bytes \n", numbytes);
    if(numbytes == 0){
      printf("Got none \n");
    }else if(numbytes == 12){
      printf("NOT OK! \n");
      close(sockfd);
      exit(1);
      break;
    }else{
      break;
    }
    if(i == 2){
      close(sockfd);
      fprintf(stderr, "Error: Timeout \n");
      break;
      exit(1);
    }
  } 
  if(numbytes > 12){
      calcProt.type =  ntohs(calcProt.type);
      calcProt.minor_version =  ntohs(calcProt.minor_version);
      calcProt.major_version =  ntohs(calcProt.major_version);
      calcProt.id =  ntohl(calcProt.id);
      calcProt.arith =  ntohl(calcProt.arith);
      calcProt.inValue1 = ntohl(calcProt.inValue1);
      calcProt.inValue2 = ntohl(calcProt.inValue2);
      calcProt.inResult = ntohl(calcProt.inResult);
      if(calcProt.arith < 5 && calcProt.arith > 0){
        if(calcProt.arith == 1){ //add
          printf("add %d & %d \n", calcProt.inValue1, calcProt.inValue2);
          calcProt.inResult = calcProt.inValue1 + calcProt.inValue2;

        } else if(calcProt.arith == 2){ //sub
          printf("Sub %d & %d \n", calcProt.inValue1, calcProt.inValue2);
          calcProt.inResult = calcProt.inValue1 - calcProt.inValue2;
       }else if(calcProt.arith == 3){ //mul
          printf("mul %d & %d \n", calcProt.inValue1, calcProt.inValue2);
          calcProt.inResult = calcProt.inValue1 * calcProt.inValue2;
        }else if(calcProt.arith == 4){ //div
          printf("Div %d & %d \n", calcProt.inValue1, calcProt.inValue2);
          calcProt.inResult = calcProt.inValue1 / calcProt.inValue2;
        }
        printf("Result = %d \n", calcProt.inResult);
        calcProt.inResult = htonl(calcProt.inResult);
        calcProt.inValue1 = htonl(calcProt.inValue1);
        calcProt.inValue2 = htonl(calcProt.inValue2);
      }else if(calcProt.arith > 4 && calcProt.arith < 9){
        if(calcProt.arith == 5){ //fadd
          printf("fadd %8.8g & %8.8g \n", calcProt.flValue1, calcProt.flValue2);
          calcProt.flResult = calcProt.flValue1 + calcProt.flValue2;
       }else if(calcProt.arith == 6){ //fsub
          printf("fsub %8.8g & %8.8g \n", calcProt.flValue1, calcProt.flValue2);
          calcProt.flResult = calcProt.flValue1 - calcProt.flValue2;
        }else if(calcProt.arith == 7){ //fmul
          printf("fmul %8.8g & %8.8g \n", calcProt.flValue1, calcProt.flValue2);
          calcProt.flResult = calcProt.flValue1 * calcProt.flValue2;
        }else if(calcProt.arith == 8){ //fdiv
          printf("fdiv %8.8g & %8.8g \n", calcProt.flValue1, calcProt.flValue2);
          calcProt.flResult = calcProt.flValue1 / calcProt.flValue2;
        }
        printf("Result = %8.8g \n", calcProt.flResult);
      }else {
        fprintf(stderr, "Error: No match of arith \n");
        close(sockfd);
        exit(1);
      }
      calcProt.type = htons(2);
      calcProt.id = htonl(calcProt.id);
      calcProt.minor_version = htons(calcProt.minor_version);
      calcProt.major_version = htons(calcProt.major_version);
      calcProt.arith = htonl(calcProt.arith);
      if((sentbytes = sendto(sockfd, &calcProt, sizeof(calcProt), 
      0, p->ai_addr, p->ai_addrlen)) == -1){
        close(sockfd);
        fprintf(stderr, "Send failed");
        exit(1);
      }
      if((numbytes = recvfrom(sockfd, &calcMsg, sizeof(calcMsg), 0, 
      p->ai_addr, &p->ai_addrlen)) == -1){
        perror("recvfrom");
        exit(1);
      }
      if(numbytes == 12){
        calcMsg.message = ntohl(calcMsg.message);
        if(calcMsg.message == 1){
          printf("OK! \n");
        }
        else if(calcMsg.message == 2){
          printf("NOT OK! \n");
        }
      }else{
        fprintf(stderr, "Error: Not expected amount of bytes \n");
        close(1);
        exit(1);
      }
    }


  close(sockfd);
  freeaddrinfo(servinfo);
  return 0;

}
