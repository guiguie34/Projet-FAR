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


int clo3; //variable globale

typedef struct Descripteur{
    int dS; //descripteur de socket du serveur
    int *alldS; //tous les descripteurs des clients
    int dSC; //descripteur du client courant
}Descripteur;

struct Descripteur *descriG;

void exitt(int n){ //gestion ctrl-c

  for(int i=0;i<10;i++){
    if(descriG->alldS[i] !=0){ //envoi message de fin à tous les clients connectés
      char *fin="fin";
      int snd = send(descriG->alldS[i],fin,sizeof(fin),0);
      if(snd == 0 || snd == -1){
        perror("Erreur send");
        exit(1);
      }
      int clo=close(descriG->alldS[i]); //fermetures descripteurs des clients
      if(clo==-1){
        perror("Error close : ");
      }
      else{
        printf("Socket fermée\n");
      }
    }
  }

  free(descriG);
  int closecli3=close(clo3); //fermuture serveur
  if(closecli3==-1){
    perror("Error close : ");
    exit(1);
  }
  printf("Arret du serveur\n");
  exit(0);
}


void *recevoirConnexion(void * data){

  struct Descripteur *descri = data; //on recupère la structure passée en paramètre
  int perma=descri->dSC; //on recupère le client courant
  char msg[100];
  memset(msg,0,sizeof(msg));
  int rep = recv(perma,msg,sizeof(msg),0); //reception du pseudo
  msg[strlen(msg)-1] = '\0';
  if(rep==-1){
	  perror("Echec de la reception du pseudo");
	  exit(1);
  }
  else{
    printf("%s join !\n",msg);
  }

  while(1){
    char msg2[100];
    char msg3[100];
    memset(msg2,0,sizeof(msg2));
    memset(msg3,0,sizeof(msg3));
    rep=recv(perma,msg2,sizeof(msg2),0);
    strcpy(msg3,msg2);
    msg2[strlen(msg2)-1] = '\0';
    if(strcmp(msg2,"fin")==0 ||strcmp(msg3,"fin")==0){ //permet vérifier si le client a saisi "fin" ou si il a fait ctrl-c
      printf("%s leave ! \n",msg);
      for(int i=0;i<10;i++){ //libération de la place prise par le client
        if(descri->alldS[i]==perma){
          descri->alldS[i]=0;
          break;
        }
      }
	    break;
    }
    printf("%s dit: %s\n",msg,msg3);

    char msg4[100]=""; //contient pseudo+message
    strcat(msg4,msg);
    char msg5[100]=" dit: ";
    strcat(msg4,msg5);
    strcat(msg4,msg3);
    for(int i=0;i<10;i++){
      if(descri->alldS[i]!=perma && descri->alldS[i]!=0){ //envoi aux autres clients
        int snd = send(descri->alldS[i],msg4,sizeof(msg4),0);
        if(snd == 0 || snd == -1){
          perror("erreur send");
          exit(1);
        }
      }
    }
  }
  pthread_exit(NULL);
}

/*void *valTab(void * data){
  struct Descripteur *descri = data;
  while(1){
    for(int i=0;i<10;i++){ //ajout tableau
        printf("%d",descri->alldS[i]);
    }
    printf("\n");
    sleep(2);
  }
  pthread_exit(NULL);
}
*/


int main(int argc, char *argv[]){
  int dS=clo3=socket(AF_INET, SOCK_STREAM, 0);
  if(dS==-1){
    perror("Erreur ! Socket non créee");
    exit(1);
  }
  if(setsockopt(dS,SOL_SOCKET,SO_REUSEADDR,&(int){1},sizeof(int)) < 0){
    perror("Réecriture du port échouée");
    exit(1);
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
  
  int li = listen(dS,10);
  if(li == -1){
    perror("File de connexion non intialisée");
    exit(1);
  }
  
  signal(SIGINT,exitt);
  struct sockaddr_in aC;
  socklen_t lg = sizeof(struct sockaddr_in);
  struct Descripteur *descri=malloc(sizeof(Descripteur));
  descriG=descri;
  descri->dS=dS;
  descri->alldS=calloc(10 , sizeof(int)); //alloue un tableau de 10 cases initialisées à 0
  /*pthread_t two;
  pthread_create(&two, NULL,valTab, (void*)descri);*/
  while(1){
    int plein=0;
    while(plein==0){
      for(int i=0;i<10;i++){ //Verifie si le salon est complet
        if(descri->alldS[i]==0){
          plein=1;
          break;
        }
      }
    }
    int dSC=accept(descri->dS, (struct sockaddr*) &aC,&lg); //Connexion du client
    if(dSC==-1){
      perror("Erreur connexion");
      exit(1);
    }
    else{
      printf("Client connecté ...\n");
    }

    for(int i=0;i<10;i++){ //cherche la première zone libre
      if(descri->alldS[i]==0){
        descri->alldS[i]=dSC;
        break;
      }
    }
    descri->dSC=dSC;
    descriG=descri;
    pthread_t one;
    if(pthread_create(&one, NULL,recevoirConnexion, (void*)descri)!=0){
      perror("Erreur création thread");
      exit(1);
    }

    //pas de thread_join car on veut reboucler directement
  }


  free(descri);

  }