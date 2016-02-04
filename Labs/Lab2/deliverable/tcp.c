/*************************
 * Nathan Anderle
 * Lab 2
 * Client
 * Professor Kalafut
 * 1/22/2016
 ************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

int main (int argc, char** argv){

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){

        printf("There was an error creating the socket\n");
        return 1;
    }

    struct sockaddr_in serveraddr;
    serveraddr.sin_family=AF_INET;
    //htons = host to network short
    //ntohs = netowrk to host short
    serveraddr.sin_port=htons(9876);
    // serveraddr.sin_addr.s_addr=inet_addr("127.0.0.1");
    serveraddr.sin_addr.s_addr = NULL;

    char* input = "";   
    printf("Enter an IP address: ");
    scanf("%s", &input);
    serveraddr.sin_addr.s_addr = inet_addr(&input);


    int e = connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (e < 0){
        printf("There was an error with connecting\n");
        return 1;
    }

    while(1){
        getchar();   
        printf("Enter a line: ");
        char* line[5000];
        char* oline[5000];
        fgets(line, 5000, stdin);
        send(sockfd, line, strlen(line), 0);
        printf("Ok, I sent it!\n");

        if (recv(sockfd, oline, 5000, 0) <0){
            puts("failure");
        }
        printf("Received from Server: %s\n", oline); 
        printf("Bye!\n");
        break;
        return 0;
    }
}

