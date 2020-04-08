#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "./sendTCP.h"
#include <sys/types.h>
#include <semaphore.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>



int main(int argc, char *argv[]){

  int dS= socket(AF_INET, SOCK_STREAM, 0);
  if(dS==-1){
    perror("Erreur ! Socket non créee");
    exit(0);
  }

  struct sockaddr_in adServ ;
  adServ.sin_family = AF_INET ; 
  adServ.sin_port = htons(atoi(argv[2]));
  int res = inet_pton(AF_INET,argv[1], &(adServ.sin_addr));
  if(res == -1){
    perror("Erreur ! Famille d'adresse non valide");
    exit(0);
  }
  else if(res == 0){
    perror("Erreur ! Adresse réseau non valide");
    exit(0);
  }

  socklen_t lgA = sizeof(struct sockaddr_in);
  res = connect(dS, (struct sockaddr *) &adServ, lgA); //connexion de la socket decrite par dS à l'adresse donnée (adresse de longeur lgA)
  if(res==-1){
    perror("connexion échouée");
    exit(0);
  }
  else{
    printf("Connexion effectuée !\n");
  }

  key_t key;

  key=ftok(argv[3],65); //génération clé
  if (key ==-1) {
    perror("erreur ftok");
    exit(1);
  }

  printf("ftok ok\n");
  
  int idSem=semget(key,1,0666); // obtention id du tableau de sémaphore identifié par idSem
  if(idSem==-1){
    perror("erreur semget");
    exit(1);
  }

  printf("semget ok\n");
  
  struct sembuf op[]={ //structure opération
    {(u_short)0,(short)-1,0}, //P, décrementation de 1 de la valeur du sémaphore
    {(u_short)0,(short)0,0}, //Z, attente que la valeur du sémaphore soit nulle
     {(u_short)0,(short)1,0} //V, incrementation de 1 de la valeur du sémaphore
    
  };

  

  
  int num;
  recv(dS,&num,sizeof(int),0);
  for(;;){
    
    if(num==1){
    
    if(semop(idSem,op,1)==-1){//on fait l'opération P sur le tableau dont l'id est idSem
      perror("erreur P");
      exit(1);
    }
    char saisie1[100]; //taille max du message à envoyer
    printf("\nsaisir le message: ");
    scanf("%s",saisie1);
    int taille1=strlen(saisie1)+1;

    int result = sendTCP(dS,saisie1,taille1,0); //la valeur de retour est traitée dans sendTCP
    

    if(semop(idSem,op+2,1)==-1){//on fait l'opération V
      perror("erreur V");
      exit(1);
    }

    if(strcmp(saisie1,"fin")==0){
      printf("end");
      break;
    }
    
    char rep[100];
    sleep(1);
  
    recv(dS,&rep,sizeof(rep),0);
    if(strcmp(rep,"fin")==0){
      printf("end");
      break;
    }
    printf("Réponse: %s\n",rep); //reception message r1 pour 3.2 sinon r
    }
    else{
      char rep[100];
      sleep(1);
      recv(dS,&rep,sizeof(rep),0);
      if(strcmp(rep,"fin")==0){
	printf("end");
	break;
      }
      printf("Réponse: %s\n",rep); //reception message r1 pour 3.2 sinon r
      if(semop(idSem,op,1)==-1){//on fait l'opération P sur le tableau dont l'id est idSem
	perror("erreur P");
	exit(1);
      }
      char saisie1[100]; //taille max du message à envoyer
      printf("\nsaisir le message: ");
      scanf("%s",saisie1);
      int taille1=strlen(saisie1)+1;

      int result = sendTCP(dS,saisie1,taille1,0); //la valeur de retour est traitée dans sendTCP
    

      if(semop(idSem,op+2,1)==-1){//on fait l'opération V
	perror("erreur V");
	exit(1);
      }

      if(strcmp(saisie1,"fin")==0){
	printf("end");
	break;
      }
    }
      
  }
  close(dS);
}
