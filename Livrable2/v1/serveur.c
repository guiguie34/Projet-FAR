#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <sys/sem.h>
#include <signal.h>
#include <string.h>

int clo1;
int clo2;
int clo3;

typedef struct Descripteur{
    int dSC;
    int dSC2;
}Descripteur;

struct Descripteur *descriG;

void exitt(int n){
    free(descriG);

  int closecli1=close(clo1);
  if(closecli1==-1){
    perror("Error close : ");
    close(clo3);
    exit(1);
  }
  int closecli2=close(clo2);
  if(closecli2==-1){
    perror("Error close : ");
    close(clo3);
     exit(1);
  }


  int closecli3=close(clo3);
  if(closecli3==-1){
    perror("Error close : ");
    exit(1);
  }
  printf("end");
}

void* oneTotwo(void *data){

    struct Descripteur *descri = data;
    char msg[100];

    while(1){
        int rep = recv(descri->dSC,msg,sizeof(msg),0);
        if(rep==-1){
	        perror("Echec de la reception");
	        exit(1);
        }
      
        printf("Client 1: %s\n",msg);
        int snd= send(descri->dSC2,msg,sizeof(msg),0);
        if(snd == 0 || snd == -1){
            perror("erreur send");
            exit(1);
        }
        char msg2[100];
        strcpy(msg2,msg);
        msg[strlen(msg)-1] = '\0';
        if(strcmp(msg,"fin")==0 || strcmp(msg2,"fin")==0){
          printf("end");
	        break;
        }
    }
    pthread_exit(NULL);

}

void* twoToone(void *data){
    struct Descripteur *descri = data;
    char msg[100];

    while(1){
        int rep = recv(descri->dSC2,msg,sizeof(msg),0);
        if(rep==-1){
	        perror("Echec de la reception");
	        exit(1);
        }
      
        printf("Client 2: %s\n",msg);
        int snd = send(descri->dSC,msg,sizeof(msg),0);
        if(snd == 0 || snd == -1){
            perror("erreur send");
            exit(1);
        }
        char msg2[100];
        strcpy(msg2,msg);
        msg[strlen(msg)-1] = '\0';
        if(strcmp(msg,"fin")==0 || strcmp(msg2,"fin")){
          printf("end");
	        break;
        }
    }
    pthread_exit(NULL);
}


int main(int argc, char *argv[]){
  int dS=clo3=socket(AF_INET, SOCK_STREAM, 0);
  if(dS==-1){
    perror("Erreur ! Socket non créee");
    exit(1);
  }
  if(setsockopt(dS,SOL_SOCKET,SO_REUSEADDR,&(int){1},sizeof(int)) < 0){
    perror("setsockopt(SO_REUSEADDR) failed");
  }
  struct sockaddr_in ad;
  
  ad.sin_family = AF_INET;
  ad.sin_addr.s_addr = INADDR_ANY;
  ad.sin_port = htons(atoi(argv[1]));
  
  int bi= bind(dS,(struct sockaddr*)&ad,sizeof(ad));
  if(bi == -1){
    perror("Nommage échoué");
    exit(1);
  }
  
  int li = listen(dS,2);
  if(li == -1){
    perror("File de connexion non intialisée");
    exit(1);
  }
  
  while(1){
    struct sockaddr_in aC;
    socklen_t lg = sizeof(struct sockaddr_in);
    int dSC=clo1= accept(dS, (struct sockaddr*) &aC,&lg);
    int dSC2=clo2= accept(dS, (struct sockaddr*) &aC, &lg);
    if(dSC==-1 || dSC2 ==-1){
      perror("Erreur connexion");
      exit(1);
    }
    else{
        printf("%d\n",dSC);
        printf("%d\n",dSC2);
        printf("Serveur en écoute ...\n");
    }

    signal(SIGINT,exitt);

    struct Descripteur *descri=malloc(sizeof(Descripteur));
    descriG=descri;

    descri->dSC=dSC;
    descri->dSC2=dSC2;
    pthread_t one;
    pthread_t two;
    pthread_create(&one, NULL,oneTotwo, (void*)descri);  
    pthread_create(&two, NULL,twoToone, (void*)descri); 
    pthread_join(one, NULL);
    pthread_join(two, NULL);
    free(descri);
  }

  }