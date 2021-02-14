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

  struct calcMessage calcMsg, recvCalcMsg;
  calcMsg.type = htons(23); //Denna ska vara 22 för att få okej
  calcMsg.message = htonl(0);
  calcMsg.protocol = htons(17);
  calcMsg.major_version = htons(1);
  calcMsg.minor_version = htons(0);
  struct calcProtocol calcProt;

  ssize_t sentbytes;
  int numbytes;
  int timeout;

  while(1){
    if(timeout == 3)
    {
      close(sockfd);
      fprintf(stderr, "Connection timeout");
      break;
      exit(1);
    } else {
      sentbytes = sendto(sockfd, &calcMsg, sizeof(calcMsg), 
      0, p->ai_addr, p->ai_addrlen);
      printf("Sent %ld bytes. \n", sentbytes);
      timeout++;
    }
    sleep(2);
    if((numbytes = recvfrom(sockfd, &recvCalcMsg, sizeof(recvCalcMsg), 0, 
      p->ai_addr, &p->ai_addrlen)) == -1){
        perror("recvfrom");
        exit(1);
    }
    if(numbytes == 0){
      printf("Got zero \n");
    } else if(numbytes == 12){
        printf("NOT OK!");
        break;
        exit(1);
      }else{ //Gör så detta läses rätt etc.. 
      printf("Got Message: type: %d, \nmessage: %d,\n major version: %d,\nminor version: %d ", recvCalcMsg.type,
        recvCalcMsg.message, recvCalcMsg.major_version, recvCalcMsg.minor_version);
        if(recvCalcMsg.type == 2 && recvCalcMsg.message == 2 
          && recvCalcMsg.major_version == 1 
          && recvCalcMsg.minor_version == 0){
          printf("NOT OK!");
          break;
          exit(1);
        }
      } 
    
    //if(numbytes = recvfrom())

  }
  close(sockfd);
  return 0;

}
