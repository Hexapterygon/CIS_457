/**************************************************
 * Nathan Anderle
 * Lab 2
 * Server
 * Professor Kalafut
 * 1/22/2016
 **************************************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <signal.h>

int childnum = 0;
void transferFile(int i);

/**************************************************
 * 
 *
 *************************************************/
int main (int argc, char** argv){

    //Set up a socket that uses AF_INET domain (i.e. IPv4) and SOCK_STREAM 
    //semantics for the communication (i.e. Transmission Control Protocol/TCP)
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){

        printf("There was an error creating the socket\n");
        return 1;
    }

    fd_set sockets;
    FD_ZERO(&sockets);

    struct sigaction sigchld_action = {
        .sa_handler = SIG_DFL,
        .sa_flags = SA_NOCLDWAIT
    };
    sigaction(SIGCHLD, &sigchld_action, NULL);

    //Structure used for holding addressing information about the client and server
    struct sockaddr_in serveraddr, clientaddr;

    //Using IPv4
    serveraddr.sin_family=AF_INET;

    //The port number for the connection. Will be set later.
    //serveraddr.sin_port = NULL;

    //Use the default interface's IP address
    serveraddr.sin_addr.s_addr=INADDR_ANY;

    while(1){

        int num = 0;
        printf("Enter a port number: ");
        scanf("%d", &num);

        if (num > 1024 && num <= 65534){
            //Assign num as the port number
            //Need root permissions below port 1024
            serveraddr.sin_port = htons(num);
            break;
        }
        else{
            printf("Invalid port number. Enter a valid port.\n");
            exit(-1);
        }
    }
    //assign sockfd to a name. Request that serveraddr be used as the address of the socket.
    bind(sockfd,(struct sockaddr*)&serveraddr, sizeof(serveraddr));

    //Wait for connections on the socket specified by sockfd
    //Have 10 length backlog queue. Any attempted connections
    //after 10 backlogged connections will be refused.
    listen(sockfd, 10);
    FD_SET(sockfd, &sockets);
    pid_t childpid;

    while(1){

        socklen_t len = sizeof(clientaddr);
        fd_set tmp_set = sockets;
        select(FD_SETSIZE, &tmp_set, NULL, NULL, NULL);

        int i;

        for(i = 0; i < FD_SETSIZE; ++i){
            if(FD_ISSET(i, &tmp_set)){

                if(i == sockfd){
                    printf("Client has connected.\n");
                    ++childnum;
                    int clientsocket = accept(sockfd, (struct sockaddr*)&clientaddr, &len);
                    FD_SET(clientsocket, &sockets);
                }
                else{

                    if((childpid = fork()) == -1){
                        perror("Fork error.");
                    }
                    else if(childpid == 0 ){

                        transferFile(i);
                    }

                    close(i);
                    FD_CLR(i, &sockets);

                }
            }
        }
    }
    return 0;
}

void transferFile(int i){

    char line[5000];
    char file_size[256];
    char file_buf[512];
    char choice = '\0';
    char ugh = '\0';


    //receive a message from the specified socket and store it in line
    recv(i, line, 5000, 0);

    line[strcspn(line, "\r\n")] = 0;
    printf("Client has requested %s\n", line);

    FILE *file;
    file = fopen(line, "rb");
    char status = 1;

    if (file == NULL){
        printf("Client has requested invalid file\n");
        //write a message to the socket
        status = -1;
        write(i, &status,  1);
    }
    else{

        fseek(file, 0L, SEEK_END);
        int sz = ftell(file);
        fseek(file, 0L, SEEK_SET);
        sprintf(file_size, "%d", sz);
        int readbytes = 0;
        status = 1;
        write(i, file_size,  strlen(file_size));
//        int total = 0;

        while((readbytes = fread(file_buf, sizeof(char), 512, file)) > 0){

            write(i, file_buf, readbytes);
            read(i, &ugh, 1);
            //printf("%d\n",readbytes);
            //total += readbytes;
            //printf("%d\n",total);
        }
        fclose(file);
    }

//printf("Hey\n");

    while(1){
        //printf("Hello\n");
        read(i, &choice, 1);
        if(choice == 'y'){
           bzero(line, 5000);
           transferFile(i); 
           break;
        }
        else if (choice == 'n'){
                        printf("Client connection closed\n");
                        kill(getpid(), SIGKILL);
        }
        else if (choice == 'q'){
                        printf("Client connection closed\n");
                        kill(getpid(), SIGKILL);
        }
    }
}
