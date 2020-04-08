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

int clo1;
int clo2;
int clo3;

void exitt(int n){

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

int main(int argc, char *argv[]){
  int dS=clo3=socket(AF_INET, SOCK_STREAM, 0);
  if(dS==-1){
    perror("Erreur ! Socket non créee");
    exit(0);
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
    exit(0);
  }
  
  int li = listen(dS,2);
  if(li == -1){
    perror("File de connexion non intialisée");
    exit(0);
  }
  
  while(1){
    struct sockaddr_in aC;
    socklen_t lg = sizeof(struct sockaddr_in);
    int dSC=clo1= accept(dS, (struct sockaddr*) &aC,&lg);
    int dSC2=clo2= accept(dS, (struct sockaddr*) &aC, &lg);
    if(dSC==-1 || dSC2 ==-1){
      perror("Erreur connexion");
      exit(0);
    }
    else{
      printf("Serveur en écoute ...\n");
    }
    
    union semun{
      
      int val;
      struct semid_ds *buf;
      unsigned short *array;
      struct seminfo *__buf;
    }egCtrl;
    
    
    key_t key;
  
    key=ftok(argv[2],65); //génération clé
    if (key ==-1) {
      perror("erreur ftok");
      exit(1);
    }
    
    printf("ftok ok\n");
    
    int idSem=semget(key,1, IPC_CREAT|0666); //création tableau de sémaphores
    if(idSem==-1){
      perror("erreur semget");
      exit(1);
    }
    printf("semget ok\n");

    egCtrl.val=1;
    if(semctl(idSem, 0, SETVAL, egCtrl) == -1){
      perror("probleme init");
      exit(1);
    }
    
    signal(SIGINT,exitt);
    
    char msg[100];
    int c1=1;
    int c2=2;
    send(dSC,&c1,sizeof(int),0);
    send(dSC2,&c2,sizeof(int),0);
    for(;;){
      int rep = recv(dSC,msg,sizeof(msg),0);
      if(rep==-1){
	perror("Echec de la reception");
	exit(1);
      }
      
      printf("Client 1: %s\n",msg);
      send(dSC2,msg,sizeof(msg),0);
      if(strcmp(msg,"fin")==0){
	goto end;
      }
      
      rep = recv(dSC2,msg,sizeof(msg),0);
      if(rep==-1){
	perror("Echec de la reception");
	exit(1);
      }
      
      printf("Client 2: %s\n",msg);
      send(dSC,msg,sizeof(msg),0);
      if(strcmp(msg,"fin")==0){
	goto end;
      }
    }
    int cliclo1;
    int cliclo2;
  end:
    cliclo1 = close(dSC);
    if(cliclo1==-1){
      perror("Error close : ");
      close(dS);
      exit(1);
    }
    cliclo2=close(dSC2);
    if(cliclo2==-1){
      perror("Error close : ");
      close(dS);
      exit(1);
    }
  }
  
  int clo=close(dS);
  if(clo==-1){
    perror("Error close : ");
    exit(1);
  }
  return 0;
}
