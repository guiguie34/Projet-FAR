#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "./sendTCP.h"

int sendTCP(int sock, char *msg,int sizeOct,int option){
	int res = send(sock,msg,sizeOct,option);
	if(res==0 || res == -1){
		perror("Erreur");
		exit(1);
	}
	int count = res;
	while(count< strlen(msg)+1){
		res = send(sock,msg+res,sizeOct-count,option);
		if(res==0 || res == -1){
		perror("erreur");
		exit(1);
		}
		count= count + res;
	}
	//printf("Message envoyé en totalité !\n");
	return count;

}

int sendTCPInt(int sock, int *msg,int sizeInt,int option){
	int res = send(sock,msg,sizeInt,option);
	if(res==0 || res == -1){
		perror("Erreur");
		exit(0);
	}
	else{
		printf("Message envoyé totalité !\n");
		return res;
	}

}
