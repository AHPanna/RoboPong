/*
Fichier Server.cpp & Client.cpp
Auteur : ABDUL HAKIM Panna
date : Fevrier - Juin 2018
Projet : Robot Pong 1040 <-Upgraded to new tech from old services 
Twitter : @AH_Panna
github : @epion93
Classe : BTS SNIR 2
licence GNU

Projet Realiser par BARETTE Alexandra & ALAOUA Othmane & moi
Contenue du projet : Carte Raspberry 3 - Carte Arduino - Application Android

compilation avce mysql : 
g++ -o server $(mysql_config --cflags) server.cpp $(mysql_config --libs)
ou : g++ -o server server.cpp sans mysql

client doit envoyer des données : u.1.z.11001001 
u= user qui est l'utilisateur 1
z= zone 8 caracteres 11001001

puis le client envoi:

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define PORT 4441
#define IP "192.168.0.182"
//#define IP "127.0.0.1"
#define BUFFER_SIZE 2048

int main(){
	int clientSocket, ret;
	struct sockaddr_in serverAddr;
	char buffer[BUFFER_SIZE];
	char recu[BUFFER_SIZE];

	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(clientSocket < 0){
		printf("[-]Erreur de connexion.\n");
		exit(1);
	}
	printf("[+]Client Socket est crée.\n");

	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = inet_addr(IP);

	ret = connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	if(ret < 0){
		printf("[-]Erreur de connexion.\n");
	}
	printf("[+]Connexion au serveur.\n");
	while(1){
		// recv(clientSocket, recu, BUFFER_SIZE, 0);
		// printf("Serveur : \t%s\n", recu);
		// bzero(recu, sizeof(recu));

		printf("Client: \t");
		scanf("%s", &buffer);
		send(clientSocket, buffer, strlen(buffer), 0);
		
		if(strcmp(buffer, "q") == 0){
			close(clientSocket);
			printf("[-]Deconnexion.\n");
			exit(1);
		// if(recv(clientSocket, buffer, BUFFER_SIZE, 0) < 0){
		// 	printf("[-]Erreur Données non-recu..\n");
		// }else{
		// 	printf("Serveur : \t%s\n", buffer);
		// 	send(clientSocket, buffer, strlen(buffer), 0);
		// 	bzero(buffer, sizeof(buffer));
		// 	}
		}	
	}
	return 0;
}
