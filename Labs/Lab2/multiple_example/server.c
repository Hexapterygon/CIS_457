#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>

int main (int argc, char** argv){

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){

        printf("There was an error creating the socket\n");
        return 1;
    }

    fd_set sockets;
    FD_ZERO(&sockets);

    struct sockaddr_in serveraddr, clientaddr;
    serveraddr.sin_family=AF_INET;
    //htons = host to network short
    //ntohs = netowrk to host short
    serveraddr.sin_port=htons(9876);
    serveraddr.sin_addr.s_addr=INADDR_ANY;

    int e = bind(sockfd,(struct sockaddr*)&serveraddr, sizeof(serveraddr));
    listen(sockfd, 10);
    FD_SET(sockfd, &sockets);

    while(1){

        int len = sizeof(clientaddr);
        fd_set tmp_set = sockets;
        select(FD_SETSIZE, &tmp_set, NULL, NULL, NULL);
        int i;
        for (i=0; i < FD_SETSIZE; ++i){
            if(FD_ISSET(i, &tmp_set)){

                if(i==sockfd){
                    printf("Got a connection, yay!\n");


                    int clientsocket = accept(sockfd, (struct sockaddr*)&clientaddr, &len);
                    FD_SET(clientsocket, &sockets);
                } else {

                    char line[5000];
                    int n = recv(i, line, 5000, 0);
                    line[n] = '\0';
                    printf("Got from client: %s\n", line);
                    close(i);
                    FD_CLR(i, &sockets);
                }
            }
        }
    }
    return 0;

}

