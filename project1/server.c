/**************************************************
 * Nathan Anderle
 * Vignesh Suresh 
 * Project 1
 * Server
 * Professor Kalafut
 * 2/03/2016
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

void transferFile(int i);

int main (int argc, char** argv){

    //Use sigaction to prevent creation of zombie children
    //Allows parent to completely ignore how children are
    //exiting. Default behavior is to let them die.
    struct sigaction sigchld_action = {
        .sa_handler = SIG_DFL,
        .sa_flags = SA_NOCLDWAIT
    };
    sigaction(SIGCHLD, &sigchld_action, NULL);

    //Set up a socket that uses AF_INET domain (i.e. IPv4) and SOCK_STREAM 
    //semantics for the communication (i.e. Transmission Control Protocol/TCP)
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){

        printf("There was an error creating the socket\n");
        return 1;
    }

    //Structure used for holding addressing information about the client and
    //server
    struct sockaddr_in serveraddr, clientaddr;

    //Using IPv4
    serveraddr.sin_family=AF_INET;

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

    //assign sockfd to a name. Request that serveraddr be used as the address
    //of the socket.
    bind(sockfd,(struct sockaddr*)&serveraddr, sizeof(serveraddr));

    //Wait for connections on the socket specified by sockfd
    //Have 10 length backlog queue. Any attempted connections
    //after 10 backlogged connections will be refused.
    listen(sockfd, 10);

    pid_t childpid;

    while(1){

        socklen_t len = sizeof(clientaddr);
        int clientsocket;
        if((clientsocket = accept(sockfd, (struct sockaddr*)&clientaddr, &len)) > 0){
            printf("Client has connected.\n");
            

            //A client has connected. Fork off a child process to handle their
            //requests. Allow the parent to continue listening for additional
            //clients.
            if((childpid = fork()) == -1){
                perror("Fork error.");
            }
            else if(childpid == 0 ){

                transferFile(clientsocket);
            }

            close(clientsocket);
        } 
    }
    return 0;
}

void transferFile(int i){

    FILE *file;
    //user entered filename
    char line[5000];
    char file_size[256];
    char file_buf[512];
    char choice = '\0';
    //Character received from the client to know it's ready for write
    char lock_step = '\0';
    char status = 1;
    int readbytes = 0;

    //receive a message from the specified socket and store it in line
    recv(i, line, 5000, 0);

    line[strcspn(line, "\r\n")] = 0;
    printf("Client has requested %s\n", line);

    file = fopen(line, "rb");

    //attempted to open file, doesn't exist
    if (file == NULL){

        printf("Client has requested invalid file\n");
        printf("Client disconnected\n");

        //Let the client know an invalid file requested
        status = -1;
        write(i, &status,  1);
    }

    else{
        //Seek to end of file. Used distance travelled
        //to determine file size. Return to beginning of file
        fseek(file, 0L, SEEK_END);
        int sz = ftell(file);
        fseek(file, 0L, SEEK_SET);
        //Convert file size to char[] and write to client
        sprintf(file_size, "%d", sz);
        write(i, file_size,  strlen(file_size));

        //Continuously read until all bytes of file are read in
        while((readbytes = fread(file_buf, sizeof(char), 512, file)) > 0){

            write(i, file_buf, readbytes);
            //Wait for the okay from the client to write again
            read(i, &lock_step, 1);
        }
        fclose(file);
    }

    //Wait for client decision
    while(1){
        read(i, &choice, 1);
        //if 'yes', call transfer logic again
        if(choice == 'y'){
            bzero(line, 5000);
            transferFile(i); 
            break;
        }
        //if 'no' or invalid, kill the child process
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
