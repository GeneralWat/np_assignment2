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
#include <time.h>

/* You will to add includes here */


// Included to get the support library
#include <calcLib.h>

#include "protocol.h"


#define DEBUG


using namespace std;



struct clientInfo{
  calcProtocol assignment;
  char ip[INET6_ADDRSTRLEN];
  int port;
  time_t start;
  //sockaddr_storage clientAddr;
};


/* Needs to be global, to be rechable by callback and main */
int loopCount=0;
int terminate=0;
clientInfo * clients[50] = {nullptr};
int nrOfClients = 0;

bool checkClientsTime(clientInfo &client, time_t end){
  int sec = (end - client.start); 
  bool result = false;
  if(sec >= 10){//Have they existed for over 10 sec
    result = true;
  }
  return result;
}



/* Call back function, will be called when the SIGALRM is raised when the timer expires. */
void checkJobbList(int signum){
  // As anybody can call the handler, its good coding to check the signal number that called it.
  time_t end = time(NULL);
  for(int i = 0; i < nrOfClients; ++i){
    printf("Looking for lost clients\n");
    if(checkClientsTime(*clients[i], end)){
      if(nrOfClients > 0 && clients[i] != nullptr){ //Make sure this does not crash
      clients[i] = clients[nrOfClients - 1];
      free(clients[nrOfClients - 1]);
      printf("Client %d removed !\n", i);
      nrOfClients--;
      }
    }
  }

  printf("Let me be, I want to sleep.\n");

  if(loopCount>20){
    printf("I had enough.\n");
    terminate=1;
  }
  
  return;
}

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}


	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



void createAssignment(clientInfo *client, int &idVal){
  idVal++;
  client->assignment.id = htonl(idVal);
          client->assignment.type = htons(1);
          client->assignment.major_version = htons(1);
          client->assignment.minor_version = htons(0);
          client->assignment.arith = rand()%8+1;
          #ifdef DEBUG
            printf("Assignment: %d \n", client->assignment.arith);
          #endif

          if(client->assignment.arith < 5 && client->assignment.arith > 0){ //Is int
          client->assignment.inValue1 = randomInt();
          client->assignment.inValue2 = randomInt();
          #ifdef DEBUG
            printf("Assignment: %d %d \n", client->assignment.inValue1, client->assignment.inValue2);
          #endif
          client->assignment.inValue1 = htonl(client->assignment.inValue1); //Conversaion now for debug, convert directly later
          client->assignment.inValue2 = htonl(client->assignment.inValue2);
          client->assignment.inResult = htonl(0);
          client->assignment.flResult = 0;
          client->assignment.flValue1 = 0;
          client->assignment.flValue2 = 0;
          client->assignment.arith = htonl(client->assignment.arith);
          }else if(client->assignment.arith > 4 && client->assignment.arith < 9){//is float
          client->assignment.flValue1 = randomFloat();
          client->assignment.flValue2 = randomFloat();
          #ifdef DEBUG
            printf("Assignment: %f %f \n", client->assignment.flValue1, client->assignment.flValue2);
          #endif
          client->assignment.flValue1 = client->assignment.flValue1; //Conversaion now for debug, convert directly later
          client->assignment.flValue2 = client->assignment.flValue2;
          client->assignment.flResult = 0;
          client->assignment.inValue1 = htonl(0); //Conversaion now for debug, convert directly later
          client->assignment.inValue2 = htonl(0);
          client->assignment.inResult = htonl(0);
          client->assignment.arith = htonl(client->assignment.arith);
          }

}

void convertToCalcMsg(calcMessage &calcMsg, calcProtocol &calcProt){
  struct calcMessage *temp = (struct calcMessage*)&calcProt;
        calcMsg.type = ntohs(temp->type);
        calcMsg.message = ntohl(temp->message);
        calcMsg.protocol = ntohs(temp->protocol);
        calcMsg.minor_version = ntohs(temp->minor_version);
        calcMsg.major_version = ntohs(temp->major_version);
}

void intCalc(calcProtocol &calcProt, int &result){
  if(calcProt.arith == 1){ //add
    printf("add %d & %d \n", calcProt.inValue1, calcProt.inValue2);
    result = calcProt.inValue1 + calcProt.inValue2;
    } else if(calcProt.arith == 2){ //sub
      printf("Sub %d & %d \n", calcProt.inValue1, calcProt.inValue2);
      result = calcProt.inValue1 - calcProt.inValue2;
    }else if(calcProt.arith == 3){ //mul
      printf("mul %d & %d \n", calcProt.inValue1, calcProt.inValue2);
      result = calcProt.inValue1 * calcProt.inValue2;
    }else if(calcProt.arith == 4){ //div
      printf("Div %d & %d \n", calcProt.inValue1, calcProt.inValue2);
      result = calcProt.inValue1 / calcProt.inValue2;
    }
}

void floatCalc(calcProtocol &calcProt, double &fResult){
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
}

void okMsg(calcMessage &calcMsg){
  
  calcMsg.type = htons(22); 
  calcMsg.message = htonl(1);
  calcMsg.protocol = htons(17);
  calcMsg.major_version = htons(1);
  calcMsg.minor_version = htons(0);

}
void notOkMsg(calcMessage &calcMsg){
  calcMsg.type = htons(2); 
  calcMsg.message = htonl(2);
  calcMsg.protocol = htons(17);
  calcMsg.major_version = htons(1);
  calcMsg.minor_version = htons(0);

}

void calcProtToHost(calcProtocol &calcProt){
  calcProt.arith = ntohl(calcProt.arith);
  calcProt.inValue1 = ntohl(calcProt.inValue1);
  //clients->assignment.arith = htonl(clients->assignment.arith);
  calcProt.inValue2 = ntohl(calcProt.inValue2);
  calcProt.inResult = ntohl(calcProt.inResult);


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
  int idVal = 70;
  initCalcLib();

  memset(&hints, 0, sizeof(hints));
  hints.ai_family =  AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  if((rv = getaddrinfo(Desthost, Destport, &hints, &servinfo)) != 0){
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
  signal(SIGALRM, checkJobbList); // Start/register the alarm. 
  setitimer(ITIMER_REAL,&alarmTime,NULL);
  double fResult;
  int iResult;


  
  while(terminate==0){
    printf("This is the main loop, %d time.\n",loopCount);
    addr_len = sizeof(their_addr);
    memset(&their_addr, 0, sizeof(their_addr));
    if((numbytes = recvfrom(sockfd, &calcProt, sizeof(calcProt), 0, 
      (struct sockaddr *)&their_addr, &addr_len)) == -1){
        perror("Recvfrom");
      }
      the_addr=(struct sockaddr_in*)&their_addr;
      inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof(s));
      printf("their Address: %s: %d (%d) %d numbytes =%d Start\n" , s, ntohs(the_addr->sin_port), sockfd, their_addr.ss_family, numbytes);
      if(numbytes == sizeof(calcMessage)){  
        convertToCalcMsg(calcMsg, calcProt);
        if(calcMsg.type == 22 && calcMsg.message == 0 && calcMsg.protocol == 17 && calcMsg.major_version == 1 && calcMsg.minor_version == 0){ //OK!
          clients[nrOfClients] = (clientInfo*) malloc(sizeof(clientInfo)); //Problemet var med allokering av minnet. 
          memcpy(&clients[nrOfClients]->ip, &s, sizeof(s));
          clients[nrOfClients]->port = ntohs(the_addr->sin_port);
          createAssignment(clients[nrOfClients], idVal);
          clients[nrOfClients]->start = time(NULL);
          nrOfClients++;

          if(sendto(sockfd, &clients[nrOfClients - 1]->assignment, sizeof(calcProtocol), 0, (struct sockaddr*)&their_addr, addr_len) == -1){
            fprintf(stderr, "Something went wrong when sending assignment!\n");
            printf("Errno:%d %s", errno, strerror(errno)); 
          }
        }else{ //NOT OK!!
          calcMsg.type = htons(2);
          calcMsg.message = htonl(2);
          calcMsg.major_version = htons(1);
          calcMsg.minor_version = htons(0);
         if((numbytes = sendto(sockfd, &calcMsg, sizeof(calcMsg), 0,
         (struct sockaddr*)&their_addr, addr_len)) == -1){
            fprintf(stderr, "Something went wrong when sending\n");
         }
        }
      }else if(numbytes == sizeof(calcProtocol)){
            inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof(s));
          for(int i = 0; i < nrOfClients; ++i){
            if(strcmp(s, clients[i]->ip) == 0 && ntohl(clients[i]->assignment.id) == ntohl(calcProt.id)){
              calcProtToHost(calcProt);
              if(calcProt.arith < 5 && calcProt.arith > 0){ //if it is an int
                intCalc(calcProt, iResult);
                if(iResult == calcProt.inResult){// OK!
                  okMsg(calcMsg);
               }else{ //Not ok
                notOkMsg(calcMsg);
                }
                
              }else if(calcProt.arith > 4 && calcProt.arith < 9){
                floatCalc(calcProt, fResult);
                double d = abs(calcProt.flResult-fResult);
                if(d < 0.0001){ //OK!
                  okMsg(calcMsg);
                }else{//Not Ok!!
                  notOkMsg(calcMsg);
                }
              }
              if((sendto(sockfd, &calcMsg, sizeof(calcMsg), 0, 
                  (struct sockaddr *)&their_addr, addr_len)) == -1){
                  fprintf(stderr, "Something went wrong when sending\n");
                 break;
                }
                clients[i]->start = time(NULL); //Restart timer
                break;
            }
          }//FOr loop end
          notOkMsg(calcMsg); //if it can go through the for loop it wasnt an accaptable client
          if((sendto(sockfd, &calcMsg, sizeof(calcMsg), 0, 
            (struct sockaddr *)&their_addr, addr_len)) == -1){
            fprintf(stderr, "Something went wrong when sending\n");
            break;
          }
      }else{
        fprintf(stderr, "Unexpected amount of bytes \n");
      }
  }
  printf("done.\n");
  for(int i = 0; i < nrOfClients; ++i){
    free(clients[i]);
  }
  close(sockfd);
  return(0);
}