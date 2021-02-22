#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

/* You will to add includes here */


// Included to get the support library
#include <calcLib.h>

#include "protocol.h"


#define DEBUG


using namespace std;
/* Needs to be global, to be rechable by callback and main */
int loopCount=0;
int terminate=0;

struct clientInfo{
  calcProtocol assignment;
  char ip[INET6_ADDRSTRLEN];
  int port;
  sockaddr_storage clientAddr;
};


/* Call back function, will be called when the SIGALRM is raised when the timer expires. */
void checkJobbList(int signum){
  // As anybody can call the handler, its good coding to check the signal number that called it.

  printf("Let me be, I want to sleep.\n");

  if(loopCount>20){
    printf("I had enough.\n");
    terminate=1;
  }
  
  return;
}

void *get_in_addr(struct sockaddr *sa)
{
  #ifdef DEBUG
    printf("Get in addr\n");
  #endif
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}


	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



int main(int argc, char *argv[]){
  
  /* Do more magic */
  if(argc != 2){
    printf("Usage:; %s <ip>:<port> \n", argv[0]);
    exit(1);
  }

  char delim[]=":";
  char *Desthost=strtok(argv[1],delim);
  char *Destport=strtok(NULL,delim);

  if(Desthost == NULL || Destport == NULL){
	  fprintf(stderr, "You must enter a IP adress and a Port");
	  exit(1);
  }
  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  int numbytes;
  struct sockaddr_storage their_addr;
  struct sockaddr_in *the_addr;
  calcMessage calcMsg;
  calcProtocol calcProt;
  socklen_t addr_len;



  char s[INET6_ADDRSTRLEN];
  clientInfo * clients[] = {nullptr};
  int nrOfClients = 0;
  initCalcLib();

  memset(&hints, 0, sizeof(hints));
  hints.ai_family =  AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  if((rv = getaddrinfo(NULL, Destport, &hints, &servinfo)) != 0){
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  for(p = servinfo; p != NULL; p = p->ai_next){
    if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
      perror("Listener: socket");
      continue;
    }
    if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
      close(sockfd);
      perror("Listener: bind");
      continue;
    }
    break;
  }

  if(p == NULL){
    fprintf(stderr, "Failed to bind socket\n");
    return 2;
  }

  freeaddrinfo(servinfo);


  /* 
     Prepare to setup a reoccurring event every 10s. If it_interval, or it_value is omitted, it will be a single alarm 10s after it has been set. 
  */
  struct itimerval alarmTime;
  alarmTime.it_interval.tv_sec=10;
  alarmTime.it_interval.tv_usec=10;
  alarmTime.it_value.tv_sec=10;
  alarmTime.it_value.tv_usec=10;

  /* Regiter a callback function, associated with the SIGALRM signal, which will be raised when the alarm goes of */
  signal(SIGALRM, checkJobbList);
  setitimer(ITIMER_REAL,&alarmTime,NULL); // Start/register the alarm. 
  double fResult;
  int iResult;


  
  while(terminate==0){
    printf("This is the main loop, %d time.\n",loopCount);
    addr_len = sizeof their_addr;
    if((numbytes = recvfrom(sockfd, &calcProt, sizeof(calcProt), 0, 
      (struct sockaddr *)&their_addr, &addr_len)) == -1){
        perror("Recvfrom");
        continue;
      }
      if(numbytes == 12){
        #ifdef DEBUG
          printf("New connection!! Got bytes %d \n", numbytes);
        #endif
        struct calcMessage *temp = (struct calcMessage*)&calcProt;
        calcMsg.type = ntohs(temp->type);
        calcMsg.message = ntohl(temp->message);
        calcMsg.protocol = ntohs(temp->protocol);
        calcMsg.minor_version = ntohs(temp->minor_version);
        calcMsg.major_version = ntohs(temp->major_version);
        if(calcMsg.type == 22 && calcMsg.message == 0 && calcMsg.protocol == 17 && calcMsg.major_version == 1 && calcMsg.minor_version == 0){ //OK!
          inet_ntop(their_addr.ss_family,
				    get_in_addr((struct sockaddr *)&their_addr),
				    s, sizeof(s));
          clients[nrOfClients] = (clientInfo*) malloc(sizeof(clientInfo));
          //sprintf(s, "%s", clients[nrOfClients]->ip);
          memcpy(&clients[nrOfClients]->ip, &s, sizeof(s));
          the_addr=(struct sockaddr_in*)&their_addr;
          #ifdef DEBUG
              printf("Client IP:%s \n", clients[nrOfClients]->ip);
            #endif
          memcpy(&clients[nrOfClients]->clientAddr, &their_addr, sizeof(their_addr));
          clients[nrOfClients]->port = ntohs(the_addr->sin_port);
          clients[nrOfClients]->assignment.id = htonl(nrOfClients);
          clients[nrOfClients]->assignment.type = htons(1);
          clients[nrOfClients]->assignment.major_version = htons(1);
          clients[nrOfClients]->assignment.minor_version = htons(0);
          clients[nrOfClients]->assignment.arith = rand()%8+1;
          #ifdef DEBUG
            printf("Assignment: %d \n", clients[nrOfClients]->assignment.arith);
          #endif

          if(clients[nrOfClients]->assignment.arith < 5 && clients[nrOfClients]->assignment.arith > 0){ //Is int
          clients[nrOfClients]->assignment.inValue1 = randomInt();
          clients[nrOfClients]->assignment.inValue2 = randomInt();
          #ifdef DEBUG
            printf("Assignment: %d %d \n", clients[nrOfClients]->assignment.inValue1, clients[nrOfClients]->assignment.inValue2);
          #endif
          clients[nrOfClients]->assignment.inValue1 = htonl(clients[nrOfClients]->assignment.inValue1); //Conversaion now for debug, convert directly later
          clients[nrOfClients]->assignment.inValue2 = htonl(clients[nrOfClients]->assignment.inValue2);
          clients[nrOfClients]->assignment.inResult = htonl(0);
          clients[nrOfClients]->assignment.arith = htonl(clients[nrOfClients]->assignment.arith);
          nrOfClients++;
          }else if(clients[nrOfClients]->assignment.arith > 4 && clients[nrOfClients]->assignment.arith < 9){//is float
          clients[nrOfClients]->assignment.flValue1 = randomFloat();
          clients[nrOfClients]->assignment.flValue2 = randomFloat();
          #ifdef DEBUG
            printf("Assignment: %f %f \n", clients[nrOfClients]->assignment.flValue1, clients[nrOfClients]->assignment.flValue2);
          #endif
          clients[nrOfClients]->assignment.flValue1 = clients[nrOfClients]->assignment.flValue1; //Conversaion now for debug, convert directly later
          clients[nrOfClients]->assignment.flValue2 = clients[nrOfClients]->assignment.flValue2;
          clients[nrOfClients]->assignment.flResult = 0;
          clients[nrOfClients]->assignment.arith = htonl(clients[nrOfClients]->assignment.arith);
          nrOfClients++;
          } else{
          printf("Random went wrong\n");
          continue;
          }
          if((numbytes = sendto(sockfd, &clients[nrOfClients-1]->assignment, sizeof(calcProtocol), 0, 
          (struct sockaddr *)&their_addr, addr_len)) == -1){
            fprintf(stderr, "Something went wrong when sending");
            continue;
          }
          continue;
        }else{ //NOT OK!!
          calcMsg.type = htons(2);
          calcMsg.message = htonl(2);
          calcMsg.major_version = htons(1);
          calcMsg.minor_version = htons(0);
         if((numbytes = sendto(sockfd, &calcMsg, sizeof(calcMsg), 0,
         (struct sockaddr*)&their_addr, addr_len)) == -1){
            fprintf(stderr, "Something went wrong when sending");
            continue;
         }
         continue;
        }
      }else if(numbytes == 50){
          #ifdef DEBUG
            printf("Calculating results...\n");
          #endif
        the_addr=(struct sockaddr_in*)&their_addr;
        inet_ntop(their_addr.ss_family,
				    get_in_addr((struct sockaddr *)&their_addr),
				    s, sizeof(s));
        for(int i = 0; i < nrOfClients; ++i){ //Check if we can find client with same ip, port and id
          #ifdef DEBUG
            printf("ID:%d vs ID:%d \n", ntohl(calcProt.id), ntohl(calcProt.id));
          #endif
          if(strcmp(s, clients[i]->ip) == 0
          && ntohl(calcProt.id) == ntohl(clients[i]->assignment.id) && ntohs(calcProt.type) == 2){ //We got a match!!
            //Check if calculation is correct //with a function
            #ifdef DEBUG
              printf("Found a match!");
            #endif
            calcProt.arith = ntohl(calcProt.arith);
            calcProt.inValue1 = ntohl(calcProt.inValue1);clients[nrOfClients]->assignment.arith = htonl(clients[nrOfClients]->assignment.arith);
            calcProt.inValue2 = ntohl(calcProt.inValue2);
            calcProt.inResult = ntohl(calcProt.inResult);
            if(calcProt.arith < 5 && calcProt.arith > 0){
              if(calcProt.arith == 1){ //add
                printf("add %d & %d \n", calcProt.inValue1, calcProt.inValue2);
                iResult = calcProt.inValue1 + calcProt.inValue2;
              } else if(calcProt.arith == 2){ //sub
                printf("Sub %d & %d \n", calcProt.inValue1, calcProt.inValue2);
                iResult = calcProt.inValue1 - calcProt.inValue2;
              }else if(calcProt.arith == 3){ //mul
                printf("mul %d & %d \n", calcProt.inValue1, calcProt.inValue2);
                iResult = calcProt.inValue1 * calcProt.inValue2;
              }else if(calcProt.arith == 4){ //div
                printf("Div %d & %d \n", calcProt.inValue1, calcProt.inValue2);
                iResult = calcProt.inValue1 / calcProt.inValue2;
             }
              printf("Result = %d \n", iResult);
              if(iResult == calcProt.inResult){// OK!
                calcMsg.type = htons(22); 
                calcMsg.message = htonl(0);
                calcMsg.protocol = htons(17);
                calcMsg.major_version = htons(1);
                calcMsg.minor_version = htons(0);
                if((numbytes = sendto(sockfd, &calcMsg, sizeof(calcMsg), 0, 
                  (struct sockaddr *)&their_addr, addr_len)) == -1){
                  fprintf(stderr, "Something went wrong when sending");
                 continue;
                }
              }else{ //Not ok!
              #ifdef DEBUG
                printf("NOT OK!\n");
              #endif
                calcMsg.type = htons(2); 
                calcMsg.message = htonl(2);
                calcMsg.protocol = htons(17);
                calcMsg.major_version = htons(1);
                calcMsg.minor_version = htons(0);
                if((numbytes = sendto(sockfd, &calcMsg, sizeof(calcMsg), 0, 
                  (struct sockaddr *)&their_addr, addr_len)) == -1){
                  fprintf(stderr, "Something went wrong when sending");
                  break;
                }
              }
            }else if(calcProt.arith > 4 && calcProt.arith < 9){
            if(calcProt.arith == 5){ //fadd
              printf("fadd %8.8g & %8.8g \n", calcProt.flValue1, calcProt.flValue2);
              fResult = calcProt.flValue1 + calcProt.flValue2;
            }else if(calcProt.arith == 6){ //fsub
              printf("fsub %8.8g & %8.8g \n", calcProt.flValue1, calcProt.flValue2);
              fResult = calcProt.flValue1 - calcProt.flValue2;
            }else if(calcProt.arith == 7){ //fmul
              printf("fmul %8.8g & %8.8g \n", calcProt.flValue1, calcProt.flValue2);
              fResult = calcProt.flValue1 * calcProt.flValue2;
            }else if(calcProt.arith == 8){ //fdiv
              printf("fdiv %8.8g & %8.8g \n", calcProt.flValue1, calcProt.flValue2);
              fResult = calcProt.flValue1 / calcProt.flValue2;
            }
             printf("Result = %8.8g \n", fResult);
             double d = abs(calcProt.flResult-fResult);
             if(d < 0.0001){ //OK!
                calcMsg.type = htons(22); 
                calcMsg.message = htonl(0);
                calcMsg.protocol = htons(17);
                calcMsg.major_version = htons(1);
                calcMsg.minor_version = htons(0);
                if((numbytes = sendto(sockfd, &calcMsg, sizeof(calcMsg), 0, 
                  (struct sockaddr *)&their_addr, addr_len)) == -1){
                  fprintf(stderr, "Something went wrong when sending");
                 continue;
                }
             }else{
               calcMsg.type = htons(2); 
                calcMsg.message = htonl(2);
                calcMsg.protocol = htons(17);
                calcMsg.major_version = htons(1);
                calcMsg.minor_version = htons(0);
                if((numbytes = sendto(sockfd, &calcMsg, sizeof(calcMsg), 0, 
                  (struct sockaddr *)&their_addr, addr_len)) == -1){
                  fprintf(stderr, "Something went wrong when sending");
                  continue;
                }
             }
            }else{
              fprintf(stderr, "Error: No match of arith \n");
              continue;
            }
          }else{
            #ifdef DEBUG
              printf("I think your for loop is broken\n");
            #endif
          }
        }
      }else{
        fprintf(stderr, "Unexpected amount of bytes \n");
        continue;
      }
      /*sleep(1);
      loopCount++;*/
  }
  printf("done.\n");
  for(int i = 0; i < nrOfClients; ++i){
    free(clients[i]);
  }
  close(sockfd);
  return(0);
}
