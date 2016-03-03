#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <time.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

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
struct nameserver{

    string type;
    string address;
    int ttl;
    char* name;
    time_t ttl_start;
};

int determineValid(char*);
int determineError(char*);
void errorResponse(char*,int);
int sendToNameServer(char*,int, int, string);
void answerClient(char*, int);
string checkCache(char*);

struct nameserver getNext(char*, int);
struct nameserver build(char*, struct nameserver, int, int);

typedef unordered_map<string, struct nameserver> cMap;
cMap cache;

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

    bind(sockfd,(struct sockaddr*)&serveraddr, sizeof(serveraddr));
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
        string rootAddr = "198.41.0.4";
        if((sze = determineValid(line)) > 0){
            //Do no query recursively
            request.rd = 0;
            //Put modified header back into the buffer
            memcpy(line, &request, 12);
            //send the request to the root resolver
            if(sendToNameServer(line, sze, sockfd, rootAddr)) {exit(1);}


            else{
                errorResponse(line, sockfd);
            }
        }
    }
    return 0;
}
int sendToNameServer(char* query, int len, int clientsock, string address){

    char nsResponse[512];
    socklen_t lent = sizeof(clientaddr);

    //Set up a new  UDP socket on port 53 for DNS queries
    int reqsock = socket(AF_INET,SOCK_DGRAM,0);
    if(reqsock<0){
        printf("There was an error creating the socket\n");
        return 1;
    }
    //Standard DNS port
    int port = htons(53);

    checkCache(query);
    const char *cstr = address.c_str();
    //IP address of a root server
    int addr = inet_addr(cstr);
    serveraddr.sin_port = port;
    serveraddr.sin_addr.s_addr = addr;
    serveraddr.sin_family=AF_INET;

    //Forward the client response to a root server
    sendto(reqsock,query, len + 16,0,(struct sockaddr*)&serveraddr,sizeof(serveraddr));

    //Receive the response from the root server
    recvfrom(reqsock, nsResponse, 512, 0, (struct sockaddr*)&serveraddr, &lent);

    //Relay errors from root to client if they exist
    //Otherwise, check for query answers.
    int lenth = determineValid(nsResponse);

    if(determineError(nsResponse)){
        errorResponse(nsResponse, clientsock);
    }
    else{

        if(nsResponse[7] == 0){
            struct nameserver next = getNext(nsResponse, lenth);
            cache[next.type] = next;

            printf("\nName: %s\n", next.type.c_str());
            printf("Address: %s\n", next.address.c_str());
            printf("TTL: %d\n", next.ttl);
            printf("Answers: %d\n", nsResponse[7]);
            sendToNameServer(query,len,clientsock, next.address);
        }
        else{
            //printf("I got the answer\n");
            answerClient(nsResponse, clientsock);
        }
    }
    return 0;
}

struct nameserver getNext(char* nsResponse, int lenth){

    struct nameserver nextQuery = {};
    //First two letters of name server used for identification
    char ident[2];
    //First letter of first record name in Authoritative section
    ident[0] = nsResponse[lenth + 18];
    //Second letter of first record name in Authoritative section
    ident[1] = nsResponse[lenth + 19];

    //Start the index after the identifying charcters are selected
    int x = lenth + 20;

    //iterate through all of the remaining bytes in the request (authoritative and additional
    //sections). This is attempting to match the first two bytes of an additional record with
    //the first two bytes of it's corresponding authoritative record. By doing this, we can
    //use both sections to glean the name, type, ttl, and address of a single nameserver. 

    while(x < 512){
        //First letter matches
        if(nsResponse[nsResponse[x] +1] == ident[0]){
            //Second letter matches
            if(nsResponse[nsResponse[x] +2] == ident[1]){

                u_int8_t ptr = nsResponse[x-1];
                int b = ptr & (1 << 7);
                int n = ptr & (1 << 6);
                if(b == 128 &&  n == 64){
                    if(nsResponse[x + 2] == 1){
                        nextQuery = build(nsResponse, nextQuery, x, lenth);
                        break;
                    }
                    else{

                        while(x < 512){
                            if(nsResponse[x] == 1 && nsResponse[x+1] == '\0' && nsResponse[x+2] == 1){
                                printf("hey, you got an IPv6 response. Try again later\n");
                                break;
                            }
                            break;
                        }
                    }
                }
            }
        }
        x++;
    }
    return nextQuery;
}

string checkCache(char* query){

    //    time_t now;
    //    now = time(&now);

    
    int v = 12;
    if(query[12] == 3){
        v = 20;
    }
    char tmp[80];
    int y = 0;
    while(query[v] != '\0'){
        if(query[v] < 41){
            v++;
        }
        tmp[y] = query[v];
        y++;
        v++;
    }
    tmp[y] = '\0';

    string name = string(tmp);
    //if(!cache.empty()){
    //    for(cMap::iterator it = cache.begin();
    //           it != cache.end(); ++it){
    //       cout << "[" << it->second.type << "]"; 
    //   cout << endl;
    //    }
    
        if(cache.count(name)){
            cout << "We've been here" << endl;
                  //printf("%s\n", cache[name].address.c_str());
   //     }

    }
    return "hey";
}

struct nameserver build(char* nsResponse, struct nameserver nextQuery, int x, int lenth){

    char temp[80];
    char tmp[80];

    //get TTL
    u_int8_t ttl_1 = nsResponse[x + 5];
    u_int8_t ttl_2 = nsResponse[x + 6];
    u_int8_t ttl_3 = nsResponse[x + 7];
    u_int8_t ttl_4 = nsResponse[x + 8];
    u_int32_t ttl = (u_int32_t) ttl_1 << 24 | (u_int32_t)ttl_2 << 16 | (u_int32_t)ttl_3 << 8 | (u_int32_t)ttl_4;
    nextQuery.ttl = ttl;

    //get Address
    unsigned char jazz = nsResponse[x +11];
    unsigned char ska = nsResponse[x +12];
    unsigned char reggae = nsResponse[x +13];
    unsigned char blues  = nsResponse[x +14];
    sprintf(temp, "%u.%u.%u.%u",jazz, ska, reggae, blues);
    nextQuery.address = string(temp);

    //Get the name type of the request. 
    int v = 0;
    int d = nsResponse[lenth + 6] + 1;
    while(nsResponse[d] != '\0'){
        if(nsResponse[d] < 41){
            d++;
        }
        temp[v] = nsResponse[d];
        d ++;
        v++;
    }
    temp[v] = '\0';
    nextQuery.type = string(temp);
    return nextQuery;
}

void answerClient(char* answerBuf, int sockfd){

    struct dnshdr ansHdr = {};
    memcpy(&ansHdr, answerBuf, 12);

    sendto(sockfd, answerBuf, 512, 0, (struct sockaddr*)&clientaddr,sizeof(clientaddr));

}

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
