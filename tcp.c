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
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>

void transferFile(int);

int main (int argc, char** argv){


    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){

        printf("There was an error creating the socket\n");
        return 1;
    }

    struct sockaddr_in serveraddr;
    serveraddr.sin_family=AF_INET;

    while(1){
        int num;
        printf("Enter a port number: "); 
        scanf("%d", &num);

        if(num > 1024 && num <  65534){
            serveraddr.sin_port = htons(num);
            break;

        }
        else
            printf("Invalid port number! Enter a valid port number.\n");
        exit(-1);
    }

    while(1){
        char input[256];   
        printf("Enter an IP address: ");
        scanf("%s", input);

        //inet_pton returns 0 if the input is not a valid IP address and -1 if there is an error
        if(inet_pton(AF_INET, input, &(serveraddr.sin_addr)) > 0){

            serveraddr.sin_addr.s_addr = inet_addr(input);
            break;

        }
        else{

            printf("Invalid IP address. Enter a valid IP address.\n");
            exit(-1);
        }
    }

    int e = connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (e < 0){
        printf("There was an error with connecting\n");
        return 1;
    }

    transferFile(sockfd);

    return 0;     
}

void transferFile(int sockfd){

    getchar();   
    fflush(stdin);
    printf("Enter a file to request: ");

    char line[5000];
    char oline[512];
    char status[256];
    char choice = '\0';
    char kc = 'q';
    char blargle = '^';

    fgets(line, 5000, stdin);

    send(sockfd, line, strlen(line), 0);

    printf("Ok, I sent it!\n");
    FILE* fp; 
    line[strcspn(line, "\r\n")] = 0;
    int bytesReceived = 0;
    int f_size = 0;

    read(sockfd, &status, 256 );
    if(status[0] != -1){

        fp = fopen(line, "wb");
        f_size = atoi(status);
        //printf("%d\n", f_size);
    }

    else{
        printf("File does not exist\n");
        exit(-1);
    }

    int total = 0;
    while(total < f_size){

        bzero(&oline, sizeof(oline));
       // printf("Heelloo!\n");

       // printf("%d\n", total);
        bytesReceived = read(sockfd, oline, 512);
        
       // if((bytesReceived = read(sockfd, oline, 512)) < 512){
        fwrite(oline, 1 ,bytesReceived,fp);
        total += bytesReceived;


        //printf("%d\n",bytesReceived);
        //printf("%d\n",total);

        write(sockfd,&blargle, 1); 

    }
        //printf("jazz\n");

    fclose(fp);
    printf("Server communication done\n"); 
    printf("Request another file? (y/n): ");

    scanf("%c", &choice);

    if(choice == 'y'){
        write(sockfd, &choice, 1);
        transferFile(sockfd);
    }
    else {
        write(sockfd, &kc, 1);
        printf("Bye!\n");
    }
}
