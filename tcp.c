/*************************
 * Nathan Anderle
 * Vignesh Suresh 
 * Project 1
 * Client
 * Professor Kalafut
 * 2/03/2016
 ************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>

void transferFile(int i);

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

        //inet_pton returns 0 if the input is not a valid IP address and -1 if
        //there is an error
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

    FILE* fp; 
    //user entered filename
    char line[5000];
    //buffer used for reading from socket
    char oline[512];
    //Used to indicate invalid file or file size
    char status[256];
    //User decision for another file
    char choice = '\0';
    //Kill character for invalid input or exit
    char kc = 'q';
    //Character written back to server to indicate next read
    char lock_step = '^';
    //Num bytes read off socket
    int bytesReceived = 0;
    //Integer representation of file size
    int f_size = 0;

    //remove scanf newline char
    getchar();   
    //Flush stdin for safe measure
    fflush(stdin);
    printf("Enter a file to request: ");
    fgets(line, 5000, stdin);

    send(sockfd, line, strlen(line), 0);

    printf("Client request sent to the server\n");
    //Trim any lingering undesireable chars
    line[strcspn(line, "\r\n")] = 0;

    //If we get -1 back from server, file doesn't exist
    read(sockfd, &status, 256 );
    if(status[0] != -1){

        //file exists. Create file to write to
        fp = fopen(line, "wb");
        //Convert read-in file size to an int
        f_size = atoi(status);
    }

    else{
        printf("File does not exist\n");
        exit(-1);
    }

    int total = 0;
    //Continuously read off the socket until we have read
    //all the bytes indicated by the file size
    while(total < f_size){

        bytesReceived = read(sockfd, oline, 512);
        fwrite(oline, 1 ,bytesReceived,fp);
        total += bytesReceived;

        //Write a bit back to the server to indicate done reading
        write(sockfd,&lock_step, 1); 

    }

    fclose(fp);
    printf("Server communication done\n"); 
    printf("Request another file? (y/n): ");

    scanf("%c", &choice);

    //If yes, run transfer logic again
    if(choice == 'y'){
        write(sockfd, &choice, 1);
        transferFile(sockfd);
    }
    //For 'no' and invalid input, exit the program
    else {
        write(sockfd, &kc, 1);
        printf("Bye!\n");
    }
}
