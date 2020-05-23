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
int nbSalons=0;
int portAccueil;

typedef struct Descripteur{
    int dS;//descripteur de socket du serveur
    int *alldS; //tous les descripteurs des clients
    int dSC; //descripteur du client courant
    int port; //port où l'user veut se connecter
}Descripteur; 

typedef struct Salon{
  char name[100];
  int places;//places dans le salon
  int placesDispo; //places dispo actuellement
  int numero; //numéro du salon
  int port; //port du salon
  char description[200];
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
  int closecli3=close(clo3); //fermeture serveur
  if(closecli3==-1){
    perror("Error close : ");
    exit(1);
  }
  printf("Arret du serveur\n");
  exit(0);
}

void remove_element(Salon *array, int index, int array_length)
{
   int i;
   for(i = index; i < array_length - 1; i++){
     array[i] = array[i + 1];
     array[i].numero =i+1; 
   }
   /*sprintf(array[array_length-1].name,"");
   array[array_length-1].numero=0;
   array[array_length-1].places=0;
   array[array_length-1].placesDispo=0;
   array[array_length-1].port=0;*/

}
void *sendFichier(void * data){
  
  struct Descripteur *descri=data;

  int dS2=socket(AF_INET, SOCK_STREAM, 0);
  if(dS2==-1){
    perror("Erreur ! Socket non créee\n");
    exit(1);
  }
  if(setsockopt(dS2,SOL_SOCKET,SO_REUSEADDR,&(int){1},sizeof(int)) < 0){
    perror("Réecriture du port échouée\n");
    exit(1);
  }
  struct sockaddr_in ad;
  
  ad.sin_family = AF_INET;
  ad.sin_addr.s_addr = INADDR_ANY;
  printf(" port : %d",descri->port+1);
  ad.sin_port = htons(descri->port+1);
  
  int bi= bind(dS2,(struct sockaddr*)&ad,sizeof(ad));
  if(bi == -1){
    perror("Nommage échoué\n");
    exit(1);
  }
  
  int li = listen(dS2,10);
  if(li == -1){
    perror("File de connexion non intialisée\n");
    exit(1);
  }
  struct sockaddr_in aC;
  socklen_t lg = sizeof(struct sockaddr_in);

  int dSC2=accept(dS2, (struct sockaddr*) &aC,&lg); //Connexion du client
    if(dSC2==-1){
      perror("Erreur connexion\n");
      exit(1);
  }

  struct Descripteur *descri2=malloc(sizeof(Descripteur));
  descri2->dS=dS2;
  descri2->alldS=calloc(10 , sizeof(int)); //alloue un tableau de 10 cases initialisées à 0

  printf("Transfert fichier\n ");
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
          perror("erreur send\n");
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
      perror("Erreur recv\n");
      exit(1);
    }
    int nomFichierSend=send(descri2->alldS[i],nomFichier,sizeof(nomFichier),0);
    //printf("%d\n",descri2->alldS[i]);
    if(nomFichierSend==0 || nomFichierSend==-1){
      perror("Erreur recv\n");
      exit(1);
    }
  }
  
  while(increment<taille){
    int rec = recv(dSC2,msg,sizeof(msg),0);
    if(rec==0 || rec==-1){
      perror("Erreur recv\n");
      exit(1);
    }
    increment=increment+rec;
    //printf("%s",msg);
    for(int i=0;i<10;i++){
        if(descri2->alldS[i]!=0){ //envoi aux autres clients
          int snd = send(descri2->alldS[i],msg,sizeof(msg),0);
          if(snd == 0 || snd == -1){
            perror("erreur send\n");
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
    perror("Echec de la reception du message\n");
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
  int servSock=descri->dS; //on recupère la socket courante
  char msg[100];
  memset(msg,0,sizeof(msg));
  printf("...Reception du pseudo...\n");
  int rep = recv(perma,msg,sizeof(msg),0); //reception du pseudo
  //msg[strlen(msg)-1] = '\0';
  if(rep==-1){
	  perror("Echec de la reception du pseudo\n");
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
      printf(" port : %d\n",descri->port);

      for(int i=0;i<10;i++){ //libération de la place prise par le client
        if(descri->alldS[i]==perma){
          descri->alldS[i]=0;
          break;
        }
      }
      for(int i=0;i<5;i++){
        if(salles[i].port==descri->port){
          salles[i].placesDispo=salles[i].placesDispo+1;
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
      descriCourant->port=descri->port;
      pthread_t fichier;
      if(pthread_create(&fichier, NULL,sendFichier, (void*)descriCourant)!=0){
        perror("Erreur création thread envoi\n");
        exit(1);
      }
      //pthread_join(fichier, NULL); 
    }
    if(strcmp(msg2,"!modif")==0){
      int index;
      for(int i=0;i<nbSalons;i++){
        if(salles[i].port==descri->port){
          index=i;
        }
      }
      char nouveauNom[100];
      int rep=recv(perma,nouveauNom,sizeof(nouveauNom),0);
      nouveauNom[strlen(nouveauNom)-1] = '\0';
      if(rep ==0 | rep ==1){
        perror("Erreur renommage");
      }
      printf("Le salon %d a été renommé en: %s\n",index,nouveauNom);
      memcpy(salles[index].name,nouveauNom,100);

      char nouvelleDescription[200];
      rep=recv(perma,nouvelleDescription,sizeof(nouvelleDescription),0);
      nouvelleDescription[strlen(nouvelleDescription)-1] = '\0';
      if(rep ==0 | rep ==1){
        perror("Erreur renommage");
      }
      printf("La description du salon %d a été changée en: %s\n",index,nouvelleDescription);
      memcpy(salles[index].description,nouvelleDescription,200);
    }
    if(strcmp(msg2,"!suppr")==0){ // CA BUG ICIIIIIIIIIIIIIIII
      printf("Debut suppr\n");
      int index;
      for(int i=0;i<nbSalons;i++){
        if(salles[i].port==descri->port){
          index=i;
        }
      }
      if(salles[index].placesDispo<9){
        char erreur[100]="Erreur ! Nombre de participants trop important, suppresion impossible. \n";
        int rep=send(perma,erreur,sizeof(erreur),0);
        if(rep ==0 | rep ==1){
          perror("Erreur suppression");
        }
      }
      else{
        char *fin="fin";
        int snd = send(perma,fin,sizeof(fin),0);
        if(snd == 0 || snd == -1){
          perror("Erreur suppresion");
        }

        printf("Avant close");
        int clo = close(perma);
        if(clo==-1){
          perror("Error close : ");
        }

        for(int i=0;i<10;i++){ //libération de la place prise par le client
          if(descri->alldS[i]==perma){
            descri->alldS[i]=0;
            break;
          }
        }
        printf("Place liberée");
        //close the socket of the channel
        clo=close(servSock);
        if(clo==-1){
          perror("Error close : ");
        }

        //realloc the array
        /*remove_element(salles, index, nbSalons); 
        Salon *tmp = realloc(salles, (nbSalons - 1) * sizeof(Salon) );
        if (tmp == NULL && nbSalons > 1) {
          exit(EXIT_FAILURE);
        }
        free(salles);
        salles = malloc(5 * sizeof (Salon));
        nbSalons--;
        salles = tmp;*/
        sprintf(salles[index].name,"");
        sprintf(salles[index].description,"");
        salles[index].numero=0;
        salles[index].places=0;
        salles[index].placesDispo=0;
        salles[index].port=0;

        Salon *tmp=malloc(sizeof(Salon));
        *tmp=salles[index];
        for(int i=index;i<nbSalons-1;i++){
          salles[i]=salles[i+1];
          salles[i].numero=salles[i].numero-1;
        }
        salles[nbSalons-1]=*tmp;
        free(tmp);
        nbSalons--;
        for (int i=0;i<nbSalons;i++){
            printf("port : %d\n",salles[i].port);
        }

        pthread_exit(NULL);
      }
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
  struct Descripteur *descri=malloc(sizeof(Descripteur));
  descriG=descri;
  descri->dS=dS;
  descri->alldS=calloc(10 , sizeof(int)); //alloue un tableau de 10 cases initialisées à 0
  descri->port=salle->port;

  ad.sin_port = htons(descri->port);
  
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
      perror("Erreur connexion 1:");
      exit(1);
    }
    else{
      //printf("Client connecté ... %d\n",salle->port);
      printf("Client connecté ... %d\n",descri->port);
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
 for(int i=0;i<nbSalons;i++){
  char numSalon[10];
  char places[10];
  char placesR[10];
  char nomSalon[100];
  char description[200];
  sprintf(places, "%d", salles[i].places);
  sprintf(placesR, "%d",salles[i].places-salles[i].placesDispo);
  sprintf(numSalon, "%d", salles[i].numero);
  sprintf(nomSalon,"%s",salles[i].name);
  sprintf(description,"%s",salles[i].description);
  char msg5[100]=" ";
  char msg6[100]=". ";

  strcat(msg4,numSalon);
  strcat(msg4,msg6);  
  strcat(msg4,nomSalon);
  
  
  strcat(msg5,placesR);
  strcat(msg4,msg5);
  char msg7[100]="/";
  strcat(msg7,places);
  strcat(msg7," ");
  strcat(msg7, description);
  strcat(msg7,"\n");
  strcat(msg4,msg7);
 }
  strcat(msg4,"Envoyez 1 : Rejoindre un salon, 2 : Créer un salon \n");
  int snd = send(perma,msg4,sizeof(msg4),0);
  if(snd == 0 || snd == -1){
    perror("erreur send");
    exit(1);
  }
  printf("...Attente choix...");
  int choix;
  int rep = recv(perma,&choix,sizeof(choix),0); //reception du choix 
  if(rep==-1 || rep ==0){
	  perror("Erreur choix");
	  exit(1);
  }
  //Rejoindre un salon
  if(choix==1){
    printf("...Attente salon....\n");
    int salon;
    rep = recv(perma,&salon,sizeof(salon),0); //num salon
    if(rep==-1 || rep ==0){
      perror("Erreur recep choix");
      exit(1);
    }
    printf("Salon choisi par client : %d \n",salon);
    for(int i=0;i<5;i++){
      printf("%d\n",salles[i].numero);
      if(salon==salles[i].numero && salles[i].placesDispo>0 && salles[i].numero!=0){
        descri->port=salles[i].port;
        printf("Port choisi %d\n",descri->port);
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
      else if (salon==salles[i].numero && salles[i].placesDispo==0){
        printf("Salon plein choisi...\n");
        int full = -1;
        snd = send(perma,&full,sizeof(int),0);
        if(snd == 0 || snd == -1){
          perror("erreur send");
          exit(1);
        }
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
  //Creer son propre salon
  else if(choix==2){
    if(nbSalons<5){
      int ok=1;
      snd = send(perma,&ok,sizeof(int),0);
      if(snd == 0 || snd == -1){
        perror("erreur send");
        exit(1);
      }
      printf("\nUser décide de créer son salon\n");
      char nomSalon[100];
      rep = recv(perma,nomSalon,sizeof(nomSalon),0); //reception du choix 
      if(rep==-1 || rep ==0){
        perror("Erreur reception nom salon");
        pthread_exit(NULL);
      }
      char description[200];
      rep = recv(perma,description,sizeof(description),0); //reception du choix 
      if(rep==-1 || rep ==0){
        perror("Erreur reception nom salon");
        pthread_exit(NULL);
      }
      printf("salon: %s\n",nomSalon);

      int portDynamique = portAccueil+100;
      for(int i=0;i<nbSalons;i++){
        for(int j=0;j<nbSalons;j++){
          if(salles[i].port==portDynamique){
            portDynamique=portDynamique+100;
            break;
          }
        }
      }
      
      //port = port + 100;
      salles[nbSalons].numero=nbSalons+1;
      salles[nbSalons].places=10;
      salles[nbSalons].placesDispo=9;
      salles[nbSalons].port=portDynamique;
      sprintf(salles[nbSalons].name,"%s",nomSalon);
      sprintf(salles[nbSalons].description,"%s",description);
      nbSalons=nbSalons+1;
      pthread_t creationSalon;
      if(pthread_create(&creationSalon, NULL,creerSocket, (void*)&salles[nbSalons-1])!=0){
        perror("Erreur création thread\n");
        pthread_exit(NULL);
      }
      snd = send(perma,&portDynamique,sizeof(int),0);
      if(snd == 0 || snd == -1){
        perror("erreur send");
        exit(1);
      }
      close(perma);
      pthread_exit(NULL);
    }
    else{
      int ok=0;
      snd = send(perma,&ok,sizeof(int),0);
      if(snd == 0 || snd == -1){
        perror("erreur send");
        exit(1);
      }
      printf("Trop de salon crées \n");
      close(perma);
      pthread_exit(NULL);
    }
  }
  //Choix erroné
  else{
    printf("Choix erroné du client\n");
    close(perma);
    pthread_exit(NULL);
  }
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
    perror("Nommage échoué\n");
    exit(1);
  }
  
  int li = listen(dS,10);
  if(li == -1){
    perror("File de connexion non intialisée\n");
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
      perror("Erreur connexion\n");
      exit(1);
    }
    else{
      printf("Client connecté Accueil...\n");
      for (int i=0;i<nbSalons;i++){
          printf("numero : %d - port : %d\n",salles[i].numero,salles[i].port);
      }

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
      perror("Erreur création thread\n");
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

void *voirSalon(){
  while(1){
  for(int i=0;i<nbSalons;i++){
    printf("%s %d %d %d %d %s\n",salles[i].name,salles[i].numero,salles[i].places,salles[i].placesDispo,salles[i].port,salles[i].description);
  }
  sleep(5);
  }
}

int main(int argc, char *argv[]){
  

  signal(SIGINT,exitt);

  salles = malloc(5 * sizeof (Salon));
  //création salle d'attente puis gestion user dans la salle d'attente

  

  //création des 5 salons
  port = atoi(argv[1]);
  portAccueil=port;
  //ici init a 2 salons crée automatiquement. Le reste peut être crée par user
  for(int i=0;i<2;i++){
    port=port+100;
    
    salles[i].numero=i+1;
    sprintf(salles[i].name,"Default");
    salles[i].places=10;
    salles[i].placesDispo=10;
    salles[i].port=port;
    sprintf(salles[i].description,"Salon Default");
    nbSalons=nbSalons+1;
    pthread_t creationSalon;
    if(pthread_create(&creationSalon, NULL,creerSocket, (void*)&salles[i])!=0){
      perror("Erreur création thread\n");
      exit(1);
    }
  }

  
  pthread_t creationAccueil;
  if(pthread_create(&creationAccueil, NULL,AccueilClient,&portAccueil)!=0){
    perror("Erreur création thread\n");
    exit(1);
  }
  pthread_t voirSalone;
  if(pthread_create(&voirSalone, NULL,voirSalon,NULL)!=0){
    perror("Erreur création thread\n");
    exit(1);
  }
  while(1){

  }
}