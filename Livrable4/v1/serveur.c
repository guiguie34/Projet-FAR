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
int port;


typedef struct Descripteur{
    int dS;//descripteur de socket du serveur
    int *alldS; //tous les descripteurs des clients
    int dSC; //descripteur du client courant
    int port; //port où l'user veut se connecter
}Descripteur; 

typedef struct Salon{
  int places;//places dans le salon
  int placesDispo; //places dispo actuellement
  int numero; //numéro du salon
  int port; //port du salon
}Salon;

Salon* salles;

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


void *sendFichier(void * data){
  
  int dS2=socket(AF_INET, SOCK_STREAM, 0);
  if(dS2==-1){
    perror("Erreur ! Socket non créee");
    exit(1);
  }
  if(setsockopt(dS2,SOL_SOCKET,SO_REUSEADDR,&(int){1},sizeof(int)) < 0){
    perror("Réecriture du port échouée");
    exit(1);
  }
  struct sockaddr_in ad;
  
  ad.sin_family = AF_INET;
  ad.sin_addr.s_addr = INADDR_ANY;
  port=port+1;
  ad.sin_port = htons(port);
  
  int bi= bind(dS2,(struct sockaddr*)&ad,sizeof(ad));
  if(bi == -1){
    perror("Nommage échoué");
    exit(1);
  }
  
  int li = listen(dS2,10);
  if(li == -1){
    perror("File de connexion non intialisée");
    exit(1);
  }
  struct sockaddr_in aC;
  socklen_t lg = sizeof(struct sockaddr_in);

  int dSC2=accept(dS2, (struct sockaddr*) &aC,&lg); //Connexion du client
    if(dSC2==-1){
      perror("Erreur connexion");
      exit(1);
  }

  struct Descripteur *descri2=malloc(sizeof(Descripteur));
  descri2->dS=dS2;
  descri2->alldS=calloc(10 , sizeof(int)); //alloue un tableau de 10 cases initialisées à 0

  struct Descripteur *descri=data;
  printf("Transfert fichier ");
  //recevoir le fichier puis l'envoyer aux autres clients (avec le mot clé file envoyé)
  int rep;
  char msg[100];
  char *fil="file";
  int nombre=0;
  for(int i=0;i<10;i++){
      if(descri->alldS[i]!=descri->dSC && descri->alldS[i]!=0){ //envoi aux autres clients
        nombre++;
        int snd = send(descri->alldS[i],fil,sizeof(fil),0);
        if(snd == 0 || snd == -1){
          perror("erreur send");
          exit(1);
        }
      }
  }
  for(int i=0;i<nombre;i++){
    int dSC=accept(dS2, (struct sockaddr*) &aC,&lg);
    descri2->alldS[i]=dSC;
    
  }
  int taille;
  int tailleRecv = recv(dSC2,&taille,sizeof(int),0);
  //printf("%d",taille);
  char nomFichier[100];
  int nomFichierr=recv(dSC2,nomFichier,sizeof(nomFichier),0);
  int increment=0;
  for(int i=0;i<nombre;i++){
    int tailleSend=send(descri2->alldS[i],&taille,sizeof(int),0);
    //printf("%d\n",descri2->alldS[i]);
    if(tailleSend==0 || tailleSend==-1){
      perror("Erreur recv");
      exit(1);
    }
    int nomFichierSend=send(descri2->alldS[i],nomFichier,sizeof(nomFichier),0);
    //printf("%d\n",descri2->alldS[i]);
    if(nomFichierSend==0 || nomFichierSend==-1){
      perror("Erreur recv");
      exit(1);
    }
  }
  
  while(increment<taille){
    int rec = recv(dSC2,msg,sizeof(msg),0);
    if(rec==0 || rec==-1){
      perror("Erreur recv");
      exit(1);
    }
    increment=increment+rec;
    //printf("%s",msg);
    for(int i=0;i<10;i++){
        if(descri2->alldS[i]!=0){ //envoi aux autres clients
          int snd = send(descri2->alldS[i],msg,sizeof(msg),0);
          if(snd == 0 || snd == -1){
            perror("erreur send");
            exit(1);
          }
        }
    }
    /*printf("%d",rep);
    if (rep < 100){
      break;
    }*/
    memset(msg,0,sizeof(msg));
  }
  if(rep==-1){
    perror("Echec de la reception du message");
    exit(1);
  }
  for(int i=0;i<10;i++){
        if(descri2->alldS[i]!=0){
          close(descri2->alldS[i]);
    }
  }
  close(dSC2);
  close(dS2);
  printf("transfert ok\n");
  free(descri2);
  pthread_exit(NULL);
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
    if(strcmp(msg2,"file")==0){
      //printf("Avant thread");
      struct Descripteur *descriCourant=malloc(sizeof(Descripteur));
      descriCourant->alldS=descri->alldS;
      descriCourant->dS=descri->dS;
      descriCourant->dSC=perma;
      pthread_t fichier;
      if(pthread_create(&fichier, NULL,sendFichier, (void*)descriCourant)!=0){
        perror("Erreur création thread envoi");
        exit(1);
      }
      //pthread_join(fichier, NULL); 
    }
    else{
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
  }
  pthread_exit(NULL);
}

void *creerSalon(void * data){
  struct infoSalon *salons = data;
  
  pthread_exit(NULL);
}

void *creerSocket(void* data){
  struct Salon *salle = data; //on recupère la structure passée en paramètre
  int dS=socket(AF_INET, SOCK_STREAM, 0);
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
  ad.sin_port = htons(salle->port);
  
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

  struct sockaddr_in aC;
  socklen_t lg = sizeof(struct sockaddr_in);
  struct Descripteur *descri=malloc(sizeof(Descripteur));
  descriG=descri;
  descri->dS=dS;
  descri->alldS=calloc(10 , sizeof(int)); //alloue un tableau de 10 cases initialisées à 0
  
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
    //printf("%d",salle->port);
    int dSC=accept(descri->dS, (struct sockaddr*) &aC,&lg); //Connexion du client
    if(dSC==-1){
      perror("Erreur connexion");
      exit(1);
    }
    else{
      printf("Client connecté ... %d\n",salle->port);
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
  }
  pthread_exit(NULL);
}
void *recevoirChoix(void *data){
  struct Descripteur *descri = data; //on recupère la structure passée en paramètre
  int perma=descri->dSC; //on recupère le client courant
  char msg4[1000]="Les salons disponibles sont: \n"; //contient pseudo+message
 for(int i=0;i<5;i++){
  char numSalon[10];
  char places[10];
  char placesDisp[10];
  sprintf(places, "%d", salles[i].places);
  sprintf(placesDisp, "%d", salles[i].placesDispo);
  sprintf(numSalon, "%d", salles[i].numero);

  char msg5[1000]="Numéro du salon: ";
  strcat(msg4,msg5);
  strcat(msg4,numSalon);
  char msg6[100]=" ,Nombre de places: ";
  strcat(msg6,places);
  strcat(msg4,msg6);
  char msg7[100]=" ,Places disponibles: ";
  strcat(msg7,placesDisp);
  strcat(msg4,msg7);
 }
  char msg8[100]=" \n Indiquez le numero choisi: ";
  strcat(msg4,msg8);
  printf("%s",msg4);
  int snd = send(perma,msg4,sizeof(msg4),0);
  if(snd == 0 || snd == -1){
    perror("erreur send");
    exit(1);
  }

  int salon;
  int rep = recv(perma,&salon,sizeof(salon),0); //reception du pseudo
  if(rep==-1 || rep ==0){
	  perror("Erreur choix");
	  exit(1);
  }
  for(int i=0;i<5;i++){
    if(rep==salles[i].numero && salles[i].placesDispo>0){
      descri->port=salles[i].port;
      snd = send(perma,&salles[i].port,sizeof(int),0);
      if(snd == 0 || snd == -1){
        perror("erreur send");
        exit(1);
      }
      //verif ici !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      //renvoyer le port au client
      //envoyer le dSC à recevoirconnexion (changer le dSC)
      salles[i].placesDispo=salles[i].placesDispo-1;
      //descri->dSC=
      break;
    }
  }
  /*
  pthread_t messagerie;
  if(pthread_create(&messagerie, NULL,recevoirConnexion, (void*)descri)!=0){
    perror("Erreur création thread");
    exit(1);
  }*/
  close(perma);
  pthread_exit(NULL);
  
}

void* AccueilClient(int *portAccueil){
  int dS=socket(AF_INET, SOCK_STREAM, 0);
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
  ad.sin_port = htons(*portAccueil);
  
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
      printf("Client connecté Accueil...\n");
    }

    for(int i=0;i<10;i++){ //cherche la première zone libre
      if(descri->alldS[i]==0){
        descri->alldS[i]=dSC;
        break;
      }
    }

    descri->dSC=dSC;
    pthread_t one;
    if(pthread_create(&one, NULL,recevoirChoix, (void*)descri)!=0){
      perror("Erreur création thread");
      exit(1);
    }
    for(int i=0;i<10;i++){ 
      if(descri->alldS[i]==dSC){
        descri->alldS[i]=0;
        break;
      }
    }

    
    //pas de thread_join car on veut reboucler directement
  }
  pthread_exit(NULL); //ussless
}



int main(int argc, char *argv[]){
  

  signal(SIGINT,exitt);

  salles = malloc(5 * sizeof (Salon));
  //création salle d'attente puis gestion user dans la salle d'attente

  port = atoi(argv[1]);
  int portAccueil=port;
  pthread_t creationAccueil;
  if(pthread_create(&creationAccueil, NULL,AccueilClient,&portAccueil)!=0){
    perror("Erreur création thread");
    exit(1);
  }

  //création des 5 salons


  for(int i=0;i<5;i++){
    port=port+1;
    
    salles[i].numero=i+1;
    salles[i].places=10;
    salles[i].placesDispo=10;
    salles[i].port=port;

    pthread_t creationSalon;
    if(pthread_create(&creationSalon, NULL,creerSocket, (void*)&salles[i])!=0){
      perror("Erreur création thread");
      exit(1);
    }
  }

  while(1){

  }
}