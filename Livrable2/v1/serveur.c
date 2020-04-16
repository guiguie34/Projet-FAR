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

int clo1; //variable globales contenant les valeurs des descripteurs de sockets
int clo2;
int clo3;

typedef struct Descripteur{ //structure contenant les descripteurs de sockets des clients + les points vers les threads
    int dSC;
    int dSC2;
    pthread_t *one;
    pthread_t *two;
}Descripteur;

struct Descripteur *descriG; //variable globable qui stocke l'adresse de la structure

void exitt(int n){ //gestion du ctrl-c

  char *fin="fin";
  int snd= send(descriG->dSC2,fin,sizeof(fin),0); //envoi message de fin aux clients
  if(snd == 0 || snd == -1){
    perror("Erreur send");
    exit(1);
  }

  snd= send(descriG->dSC,fin,sizeof(fin),0);
  if(snd == 0 || snd == -1){
    perror("Erreur send");
    exit(1);
  }

  free(descriG); //libération memoire

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
  printf("Serveur fermé");
  exit(0);
}

void* oneTotwo(void *data){

    struct Descripteur *descri = data; //on recupère la structure passée en paramètre
    char msg[100];

    while(1){
        memset(msg,0,sizeof(msg));
        int rep = recv(descri->dSC,msg,sizeof(msg),0); //reception du message envoyé par le client1
        if(rep==-1){
	        perror("Echec de la reception");
	        exit(1);
        }
      
        printf("Client 1: %s\n",msg);
        int snd= send(descri->dSC2,msg,sizeof(msg),0); //envoi du message au client2
        if(snd == 0 || snd == -1){
            perror("Erreur send");
            exit(1);
        }
        char msg2[100];
        strcpy(msg2,msg);
        msg[strlen(msg)-1] = '\0';
        if(strcmp(msg,"fin")==0 || strcmp(msg2,"fin")==0){ // gestion du cas de fin, msg stocke le message envoyé à partir de fgets (saisie manuelle par le client)
          printf("Message de terminaison reçu\n"); //msg2 stocke le message qui n'est pas envoyé à partir de fgets (ctrl-c du client)
	        break;
        }
    }
    if(pthread_cancel(*(descri->two))!=0){
      perror("Erreur fermture thread voisin");
      exit(1);
    } //fermeture du thread voisin à celui-ci
    
    pthread_exit(NULL); // fermeture du thread courant
}

void* twoToone(void *data){ //fonctionnement similaire à la fonction au dessus
    struct Descripteur *descri = data;
    char msg[100];

    while(1){
        memset(msg,0,sizeof(msg));
        int rep = recv(descri->dSC2,msg,sizeof(msg),0);
        if(rep==-1){
	        perror("Echec de la reception");
	        exit(1);
        }
      
        printf("Client 2: %s\n",msg);
        int snd = send(descri->dSC,msg,sizeof(msg),0);
        if(snd == 0 || snd == -1){
            perror("Erreur send");
            exit(1);
        }
        char msg2[100];
        strcpy(msg2,msg);
        msg[strlen(msg)-1] = '\0';
        if(strcmp(msg,"fin")==0 || strcmp(msg2,"fin")==0){
          printf("Message de terminaision reçu\n");
	        break;
        }
    }
    if(pthread_cancel(*(descri->one)) !=0){
      perror("Erreur fermeture thread voisin");
      exit(1);
    }
    pthread_exit(NULL);
}


int main(int argc, char *argv[]){
  int dS=clo3=socket(AF_INET, SOCK_STREAM, 0);
  if(dS==-1){
    perror("Erreur ! Socket non créee");
    exit(1);
  }
  if(setsockopt(dS,SOL_SOCKET,SO_REUSEADDR,&(int){1},sizeof(int)) < 0){ //permet de réutiliser le port courant directement
    perror("Erreur ! Réecriture du port échouée");
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
        printf("Serveur en écoute ...\n");
    }

    signal(SIGINT,exitt);

    struct Descripteur *descri=malloc(sizeof(Descripteur));
    descriG=descri;

    descri->dSC=dSC; //initialisation des valeurs dans la structure
    descri->dSC2=dSC2;
    
    pthread_t one;
    pthread_t two;
    descri->one=&one;
    descri->two=&two;
    
    if(pthread_create(&one, NULL,oneTotwo, (void*)descri) !=0){
      perror("Erreur création du thread 1");
      exit(1);
    } //création des threads
    if(pthread_create(&two, NULL,twoToone, (void*)descri) !=0){
      perror("Erreur creation du thread 2");
      exit(1);
    }
    if(pthread_join(one, NULL) !=0){
      perror("Erreur join");
      exit(1);
    } //attente terminaison des threads
    if(pthread_join(two, NULL) !=0){
      perror("Erreur join");
      exit(1);
    }
    free(descri); //libération de la memoire
  }

  }