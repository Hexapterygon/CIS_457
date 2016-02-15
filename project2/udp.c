#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

int main (int argc, char** argv){

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){

        printf("There was an error creating the socket\n");
        return 1;
    }

    struct sockaddr_in serveraddr;
    serveraddr.sin_family=AF_INET;
    //htons = host to network short
    //ntohs = netowrk to host short
    //serveraddr.sin_port=htons(9876);
    //serveraddr.sin_addr.s_addr=inet_addr("127.0.0.1");

    int num;
    printf("Enter a port number: ");
    scanf("%d", &num);
    serveraddr.sin_port = htons(num);

    char* input = "";
    printf("Enter an IP address: ");
    scanf("%s", &input);
    serveraddr.sin_addr.s_addr = inet_addr(&input);

    char line[5000];
    getchar();
    while(strcmp(line, "/exit") != 0){ 
        printf("Enter a line: ");
        int len = sizeof(serveraddr);
        char oline[5000];
        fgets(line, 5000, stdin);
        sendto(sockfd, line, 5000, 0, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr_in));
        printf("Ok, I sent it.\n");

        recvfrom(sockfd, oline, 5000, 0, (struct sockaddr*)&serveraddr, &len);
        printf("Echoed from server: %s\n", oline);
        line[strcspn(line, "\r\n")] = 0;
    }
    return 0;

}

