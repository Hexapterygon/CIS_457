#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

int determineValid(char*);
int determineError(char*);
void errorResponse(char*,int);
int sendToRoot(char*,int, int);
void checkAnswers();

struct dnshdr{
    u_int16_t id;
    u_int8_t rd:1;
    u_int8_t tc:1;
    u_int8_t aa:1;
    u_int8_t opcode:4;
    u_int8_t qr:1;

    u_int8_t rcode:4;
    u_int8_t cd:1;
    u_int8_t ad:1;
    u_int8_t z:1;

    u_int16_t qcount;
    u_int16_t ancount;
    u_int16_t authcount;
    u_int16_t addcount;
};

struct sockaddr_in serveraddr, clientaddr;

int main (int argc, char** argv){

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){

        printf("There was an error creating the socket\n");
        return 1;
    }

    serveraddr.sin_family=AF_INET;
    serveraddr.sin_addr.s_addr=INADDR_ANY;

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

    struct dnshdr request;

    while(1){

        socklen_t len = sizeof(clientaddr);
        char line[512];

        int n = recvfrom(sockfd, line, 512, 0, (struct sockaddr*)&clientaddr, &len);

        line[n] = '\0';
        //Populate DNS header structure from the received request
        memcpy(&request, line, 12);

        int sze = 0;
        if((sze = determineValid(line)) > 0){
            //Do no query recursively
            request.rd = 0;
            //Put modified header back into the buffer
            memcpy(line, &request, 12);
            //send the request to the root resolver
            if(sendToRoot(line, sze, sockfd)) {exit(1);}

        }
        else{
            errorResponse(line, sockfd);
        }
    }
    return 0;
}

int sendToRoot(char* query, int len, int clientsock){

    char rootResponse[512];
    socklen_t lent = sizeof(clientaddr);
    
    //Set up a new  UDP socket on port 53 for DNS queries
    int reqsock = socket(AF_INET,SOCK_DGRAM,0);
    if(reqsock<0){
        printf("There was an error creating the socket\n");
        return 1;
    }
    //Standard DNS port
    int port = htons(53);
    //IP address of a root server
    int addr = inet_addr("198.41.0.4");
    serveraddr.sin_port = port;
    serveraddr.sin_addr.s_addr = addr;
    serveraddr.sin_family=AF_INET;

    //Forward the client response to a root server
    sendto(reqsock,query, len + 16,0,(struct sockaddr*)&serveraddr,sizeof(serveraddr));

    //Receive the response from the root server
    int m = recvfrom(reqsock, rootResponse, 512, 0, (struct sockaddr*)&serveraddr, &lent);

    //Relay errors from root to client if they exist
    //Otherwise, check for query answers.
    if(determineError(rootResponse)){
        errorResponse(rootResponse, clientsock);
    }
    else{
        checkAnswers();
    }
    return 0;
}

void checkAnswers(){

};

int determineError(char* response){
    struct dnshdr responseHdr = {};
    memcpy(&responseHdr, response, 12);
    if(responseHdr.rcode != 0){
        return 1;
    }
    return 0;
}

int determineValid(char* query){

    //Start at query and go until end
    int x = 12;
    while(query[x] != '\0'){
        x++;
    }
    //Type always two bytes from end of query
    //Class always four bytes from end of query
    if(query[x + 2] == 1 && query[x + 4] == 1){
        return x;
    }
    else{
        return 0;
    }
}

void errorResponse(char* req, int sockfd){

    struct dnshdr errHdr = {};
    char rep[512];
    memcpy(&errHdr, req, 12);

        //Transaction ID in first two bytes received
        u_int8_t id_1 = req[0];
        u_int8_t id_2 = req[1]; 

        //Create 16-bit result and shift most sig bits
        //over 8 and 'or' the least sig bits to get
        //16-bit ID
        u_int16_t result = (u_int16_t) id_1 << 8 | id_2;

        errHdr.id = htons(result);
        errHdr.qr = 1;

        //The class or type of the request not implemented
        //by resolver
    if(errHdr.rcode == 0){
        errHdr.rcode = 4;
    }
        errHdr.rd = 0;
        memcpy(rep, &errHdr, 12);

    sendto(sockfd, rep, 12, 0, (struct sockaddr*)&clientaddr,sizeof(clientaddr));

}
