#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main (int argc, char** argv){

	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){

		printf("There was an error creating the socket\n");
		return 1;
	}

	struct sockaddr_in serveraddr, clientaddr;
	serveraddr.sin_family=AF_INET;
	//htons = host to network short
	//ntohs = netowrk to host short
	serveraddr.sin_addr.s_addr=INADDR_ANY;
	//printf("%s\n", argv[1]);
	if(argv[1] != NULL){
		int port = atoi(argv[1]);
		if(port > 1024 && port <= 65534){
			serveraddr.sin_port=htons(port);
		}
		else{
			printf("Invalid port number, enter a valid port number\n");
			exit(-1);	
		}
	}
	else{
		printf("Need a port number\n");
		exit(-1);
	}

	int e = bind(sockfd,(struct sockaddr*)&serveraddr, sizeof(serveraddr));
	listen(sockfd, 10);

	while(1){

		int len = sizeof(clientaddr);
		char line[5000];
		int n = recvfrom(sockfd, line, 5000, 0, (struct sockaddr*)&clientaddr, &len);
		line[n] = '\0';
		printf("Got from client: %s", line);
		sendto(sockfd, line, 5000, 0, (struct sockaddr*)&clientaddr, len);
		printf("Sending back to client: %s", line);
	}

	return 0;

}
