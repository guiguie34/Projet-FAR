#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "./sendTCP.h"
#include <semaphore.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>

int dSG;
int port;
char *ip;
int attente;


void exitt(int n){
    char *fin="fin";
    sendTCP(dSG,fin,sizeof(fin),0);
    int clo = close(dSG);
    if(clo==-1){
		perror("Erreur fermeture socket: ");
		exit(1);
	}
    exit(0);
}

unsigned long fsize(char* file)
{
    FILE * f = fopen(file, "r");
    fseek(f, 0, SEEK_END);
    unsigned long len = (unsigned long)ftell(f);
    fclose(f);
    return len;
}

void* envoiFichier(void *data){
    int dS2= socket(AF_INET, SOCK_STREAM, 0);
    if(dS2==-1){
        perror("Erreur ! Socket non créee");
        exit(1);
    }

    struct sockaddr_in adServ ;
    adServ.sin_family = AF_INET ; 
    adServ.sin_port = htons(port+1);
    printf("%d",port+1);
    int res = inet_pton(AF_INET,ip, &(adServ.sin_addr));
    if(res == -1){
        perror("Erreur ! Famille d'adresse non valide");
        exit(1);
    }
    else if(res == 0){
        perror("Erreur ! Adresse réseau non valide");
        exit(1);
    }

    socklen_t lgA = sizeof(struct sockaddr_in);
    res = connect(dS2, (struct sockaddr *) &adServ, lgA); //connexion de la socket decrite par dS à l'adresse donnée (adresse de longeur lgA)
    if(res==-1){
        perror("connexion échouée3");
        exit(1);
    }

    

    DIR *d;
    struct dirent *dir;
    d = opendir("./DL");
    if (d) {
        printf("Fichiers disponibles: ");
    while ((dir = readdir(d)) != NULL) {
        printf("%s\n", dir->d_name);
    }
    closedir(d);
    }

    

    char file[100];
    printf("Veuillez saisir le nom d'un fichier montré ci-dessus : ");
    fgets(file,100,stdin);
    file[strlen(file)-1] = '\0';
    char upload[100]="DL/";
    strcat(upload,file);

    attente=1;
    FILE *fp = fopen(upload,"r");
    if(fp==NULL)
    {
        perror("File opern error:");
        pthread_exit(NULL);
    }
    int taille = fsize(upload);
    int rep =send(dS2,&taille,sizeof(int),0);
    if(rep==0||rep==-1){
        perror("Erreur send");
        exit(1);
    }
    int increment=0;
    
    rep=send(dS2,file,sizeof(file),0);
    if(rep==0||rep==-1){
        perror("Erreur send");
        exit(1);
    }
    while(increment<taille){
        unsigned char buff[100];
        int nread = fread(buff,1,100,fp);
        rep = send(dS2,buff,sizeof(buff),0);
        if(rep==-1 || rep==0){
            perror("Erreur");
            exit(1);
        }
        if (nread < 100){
            if (feof(fp)){
                printf("End of file\n");
            }
            if (ferror(fp)){
                printf("Error reading\n");
            }
            break;
        }
        memset(buff,0,sizeof(buff));
        increment=increment+nread;
        
    }
    close(dS2);
    fclose(fp);
    pthread_exit(NULL);
}

void* receptionFichier(void *data){

    int dS2= socket(AF_INET, SOCK_STREAM, 0);
    if(dS2==-1){
        perror("Erreur ! Socket non créee");
        exit(1);
    }

    struct sockaddr_in adServ ;
    adServ.sin_family = AF_INET ; 
    printf("%d",port);
    adServ.sin_port = htons(port+1);
    int res = inet_pton(AF_INET,ip, &(adServ.sin_addr));
    if(res == -1){
        perror("Erreur ! Famille d'adresse non valide");
        exit(1);
    }
    else if(res == 0){
        perror("Erreur ! Adresse réseau non valide");
        exit(1);
    }

    socklen_t lgA = sizeof(struct sockaddr_in);
    res = connect(dS2, (struct sockaddr *) &adServ, lgA); //connexion de la socket decrite par dS à l'adresse donnée (adresse de longeur lgA)
    if(res==-1){
        perror("connexion échouée4\n");
        exit(1);
    }


    //int *dS=data;
    int taille;
    int tailleRep=recv(dS2,&taille,sizeof(int),0);
    int increment=0;
    char rep[100];
    /* Receive data in chunks of 256 bytes */
    int byte=0;
    char nomFichier[100];

    int nomFichierr=recv(dS2,nomFichier,sizeof(nomFichier),0);
    FILE *fp;

	printf("Receiving file...\n");
    char adresse[100]="";
    char adresse2[100]="./Reception/";
    strcat(adresse,adresse2);
    strcat(adresse,nomFichier);
   	fp = fopen(adresse, "w+"); 
    if(NULL == fp){
       	
           printf("Error opening file\n");
           pthread_exit(NULL);
    }


    
    while(increment<taille)
    {
        byte=recv(dS2,&rep,sizeof(rep),0);
        if(byte==0||byte==-1){
            perror("Erreur recv\n");
            exit(1);
        }
        increment=increment+byte;
        //printf("%d",byte);
        //printf("%s",rep);
        //fprintf(fp,"%s",rep);
        fwrite(rep, 1, 100, fp);
        if (byte < 100){
            if (feof(fp)){
                printf("End of file\n");
            }
            if (ferror(fp)){
                printf("Error reading\n");
            }
            break;
        }
        memset(rep,0,sizeof(rep));
    }
    fclose(fp);

    if(byte < 0)
    {
        printf("\n Read Error \n");
    }
    close(dS2);
    printf("\nFile OK....Completed\n");
    pthread_exit(NULL);
    

}

void* envoi(void *data){
    int *dS=data; //on recupère le descripteur de socket
    while(1){ 

        char saisie1[100];
        memset(saisie1,0,sizeof(saisie1));
        //printf("\nsaisir le message: ");
        fgets(saisie1,100,stdin);
        char temp[100];
        strcpy(temp,saisie1);
        temp[strlen(temp)-1] = '\0';
        if(strcmp(temp,"!suppr")==0){
              printf("Voulez-vous vraiment supprimer le salon (o/n) ?");
              char suppr[100];
              scanf("%s",suppr);
              if(strcmp(suppr,"o")!=0){
                  memset(saisie1,0,sizeof(saisie1));
              }
        }
        int result = sendTCP(*dS,saisie1,sizeof(saisie1),0); //la valeur de retour est traitée dans sendTCP
        if(strcmp(temp,"!modif")==0){
            char nouveauNom[100];
            printf("Quel nom de salon voulez-vous donner ? ");
            fgets(nouveauNom,100,stdin);
            result = sendTCP(*dS,nouveauNom,sizeof(nouveauNom),0);
        }
        if(strcmp(temp,"fin")==0){
            printf("Arret du client\n");
            exit(0);
        }
        if(strcmp(temp,"file")==0){

            attente=0;
            pthread_t fichier;
            if(pthread_create(&fichier, NULL,envoiFichier, (void*)&dS)!=0){
                perror("Erreur création thread");
                exit(1);
            }
            //pthread_join(fichier,NULL);
            while(attente==0){

            }
        }
    }
    pthread_exit(NULL);

}

void* recevoir(void * data){
    int *dS=data;
    while(1){

        char rep[100];
  
        int rec = recv(*dS,&rep,sizeof(rep),0);
        if(rec==-1){
            perror("Error recv : ");
            exit(1);
        }
        if(strcmp(rep,"fin")==0){ //utile pour les arrets des serveur
            printf("Arret du serveur\n");
            exit(0);
        }
        if(strcmp(rep,"file")==0){
            pthread_t fichier2;
            if(pthread_create(&fichier2, NULL,receptionFichier, (void*)&dS)!=0){
                perror("Erreur création thread");
                exit(1);
            }
            //pthread_join(fichier2,NULL);
        }
        fputs(rep,stdout);
    }
    pthread_exit(NULL);
}

void envoiPseudo(char* pseudo, int dS){

    int result = sendTCP(dS,pseudo,sizeof(pseudo),0); //la valeur de retour est traitée dans sendTCP

}



int main(int argc, char *argv[]){

    int dS= socket(AF_INET, SOCK_STREAM, 0);
    if(dS==-1){
        perror("Erreur ! Socket non créee");
        exit(1);
    }

    struct sockaddr_in adServ ;
    adServ.sin_family = AF_INET;
    //port=atoi(argv[2]); 
    //ip=argv[1];
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
        perror("connexion échouée1");
        exit(1);
    }
    else{
        printf("Connexion effectuée !\n");
    }

    char texte[1000];
    int rep=recv(dS,texte,sizeof(texte),0);
    if(rep==0||rep==-1){
        perror("Erreur recv1");
        exit(1);
    }
    printf("%s",texte);

    //Choix du client Rejoindre ou Créer son salon
    int choix;
    scanf("%d", &choix);
    rep=send(dS,&choix,sizeof(choix),0);
    if(rep==0||rep==-1){
        perror("Erreur send2");
        exit(1);
    }
    //rejoint un salon existant
    if(choix==1){
        printf("Veuillez entré le numéro du salon que vous voulez rejoindre : ");
        int numSalon;
        scanf("%d", &numSalon);
        rep=send(dS,&numSalon,sizeof(numSalon),0);
        if(rep==0||rep==-1){
            perror("Erreur send2");
            exit(1);
        }
        printf("...Salon envoyé...\n");
    }
    //Créer son chan
    else if(choix==2){
        int acces;
        rep=recv(dS,&acces,sizeof(int),0);
        if(rep==-1){
            printf("Salon choisi complet\n");
            exit(1);
        }
        if(acces==0){
            printf("Nombre de salons max atteint");
            exit(0);
        }
        printf("Veuillez entrer le nom du channel sans espace : ");
        char nomSalon[100];
        //fgets(nomSalon,100,stdin);
        scanf("%s",nomSalon);
        rep=send(dS,nomSalon,sizeof(nomSalon),0);
        if(rep==0||rep==-1){
            perror("Erreur send2");
            exit(1);
        }
        printf("Channel crée  \n");
        
    }
    else{
        printf("Erreur dans le choix...");
        close(dS);
        exit(1);
    }

    int portSalon;
    rep=recv(dS,&portSalon,sizeof(int),0);
    //printf("Port du salon: %d\n",portSalon);
    if(portSalon==-1){
        printf("Salon choisi complet\n");
        exit(1);
    }
    if(rep==-1){
        perror("Erreur send3");
        exit(1);
    }
    if(rep==0){
        printf("Numéro de salon incorrect \n");
        exit(1);
    }
    close(dS);
    

    dS= socket(AF_INET, SOCK_STREAM, 0);
    if(dS==-1){
        perror("Erreur ! Socket non créee");
        exit(1);
    }

    adServ.sin_family = AF_INET ;
    port=portSalon; 
    ip=argv[1];
    adServ.sin_port = htons(port);
    res = inet_pton(AF_INET,argv[1], &(adServ.sin_addr));
    if(res == -1){
        perror("Erreur ! Famille d'adresse non valide");
        exit(1);
    }
    else if(res == 0){
        perror("Erreur ! Adresse réseau non valide");
        exit(1);
    }

    lgA = sizeof(struct sockaddr_in);
    res = connect(dS, (struct sockaddr *) &adServ, lgA); //connexion de la socket decrite par dS à l'adresse donnée (adresse de longeur lgA)
    if(res==-1){
        perror("connexion échouée2");
        exit(1);
    }
    else{
        printf("Connexion effectuée !\n");
    }
    
    

    printf("Saisir votre pseudo: ");
    char pseud[100];
    //ici fgets non bloquant donc pseduo vide envoyé 
    //fgets(pseud,100,stdin);
    scanf("%s",pseud);
    //Scanf perd des données 
    envoiPseudo(pseud,dS);
    

    pthread_t envo;
    pthread_t recpt;
    dSG=dS;
    signal(SIGINT,exitt);

    pthread_create(&envo, NULL,envoi, (void*)&dS); 
    pthread_create(&recpt, NULL,recevoir, (void*)&dS);
    pthread_join(envo, NULL); 
    pthread_join(recpt, NULL); 
    int clo = close(dS);
    if(clo==-1){
		perror("error with closing client : ");
		exit(1);
	}
    
    
}