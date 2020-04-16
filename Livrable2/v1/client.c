#include <pthread.h>
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
#include <string.h>
#include <signal.h>

int dSG; //variable globale contenant le descripteur de socket
void fin(){ //fonction qui gère le cas où un message = fin
    int clo = close(dSG);
    if(clo==-1){
		perror("Erreur fermeture socket ");
		exit(1);
	}
    exit(0);
}

void exitt(int n){ //gestion ctrl-c
    char *fin="fin";

    sendTCP(dSG,fin,sizeof(fin),0); //voir le fichier sendTCP.c (valeur de retour directement gérée)
    int clo = close(dSG);
    if(clo==-1){
		perror("Erreur fermeture socket ");
		exit(1);
	}
    exit(0);
}

void* envoi(void *data){
    int *dS=data; //on recupère le descripteur de socket (avec un pointeur)
    while(1){

        char saisie1[100];
        memset(saisie1,0,sizeof(saisie1));
        //printf("\nsaisir le message: ");
        fgets(saisie1,100,stdin);

        int result = sendTCP(*dS,saisie1,sizeof(saisie1),0); //la valeur de retour est traitée dans sendTCP
        saisie1[strlen(saisie1)-1] = '\0'; //permet de comparer après un fgets
        if(strcmp(saisie1,"fin")==0){
            printf("Arret du client\n");
            fin();    
        }
    }
    pthread_exit(NULL);

}

void* recevoir(void * data){
    int *dS=data;
    while(1){

        char rep[100];
        int rec = recv(*dS,&rep,sizeof(rep),0); //reception du message
        if(rec==-1){
            perror("Error recv : ");
            exit(1);
        }
        fputs(rep,stdout); //envoi de la réponse vers la sortie standard
        char rep2[100];
        strcpy(rep2,rep);
        rep[strlen(rep)-1] = '\0';
        if(strcmp(rep,"fin")==0 || strcmp(rep2,"fin")==0){ //permet verifier si le deuxième client à envoyer fin manuellement
            printf("Arret du client\n"); //ou si il a fait ctrl-c (donc sans fgets)
            fin();
        }
        /*printf("Réponse: %s\n",rep);*/
    }
    pthread_exit(NULL);
}



int main(int argc, char *argv[]){


    int dS=dSG= socket(AF_INET, SOCK_STREAM, 0);
    if(dS==-1){
        perror("Erreur ! Socket non créee");
        exit(1);
    }

    struct sockaddr_in adServ ;
    adServ.sin_family = AF_INET ; 
    adServ.sin_port = htons(atoi(argv[2]));
    int res = inet_pton(AF_INET,argv[1], &(adServ.sin_addr));
    if(res == -1){
        perror("Erreur ! Famille d'adresse non valide");
        exit(1);
    }
    else if(res == 0){
        perror("Erreur ! Adresse réseau non valide");
        exit(1);
    }

    socklen_t lgA = sizeof(struct sockaddr_in);
    res = connect(dS, (struct sockaddr *) &adServ, lgA); //connexion de la socket decrite par dS à l'adresse donnée (adresse de longeur lgA)
    if(res==-1){
        perror("connexion échouée");
        exit(1);
    }
    else{
        printf("Connexion effectuée !\n");
    }
    signal(SIGINT,exitt);

    pthread_t envo;
    pthread_t recpt;

    if(pthread_create(&envo, NULL,envoi, (void*)&dS) !=0){ //creation thread
        perror("Erreur création thread");
        exit(1);
    }
    if(pthread_create(&recpt, NULL,recevoir, (void*)&dS) !=0){
        perror("Erreur création thread");
        exit(1);
    }

    if(pthread_join(envo, NULL) !=0){ //attente terminaison thread
        perror("Erreur join");
        exit(1);
    } 
    if(pthread_join(recpt, NULL) !=0){
        perror("Erreur join");
        exit(1);
    }
    int clo = close(dS);
    if(clo==-1){
		perror("error with closing client : ");
		exit(1);
	}
}