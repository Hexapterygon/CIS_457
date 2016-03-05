//Nathan Anderle
//Vignesh Suresh
//DNS Resolver
//Professor Kalafut
//3-4-16

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
    string name;
    time_t ttl_start;
};
typedef unordered_map<string, struct nameserver> cMap;
cMap cache;

int determineValid(char*);
int determineError(char*);
void errorResponse(char*,int);
int sendToNameServer(char*,int, int, string, int);
void answerClient(char*, int, int);
string checkCache(char*, string, int);
string parseName(string);
struct nameserver getNext(char*, int);
struct nameserver getAnswer(char*, int);
struct nameserver build(char*, struct nameserver, int, int);

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
        //Determine if the class and type is correct
        if((sze = determineValid(line)) > 0){
            //Do no query recursively
            request.rd = 0;
            //Put modified header back into the buffer
            memcpy(line, &request, 12);
            //send the request to the root resolver
            if(sendToNameServer(line, sze, sockfd, rootAddr, 1)) {exit(1);}

            else{
                errorResponse(line, sockfd);
            }
        }
    }
    return 0;
}


int sendToNameServer(char* query, int len, int clientsock, string address, int flag){

    string cacheadd;
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

    //Set the address to the address sent by the last nameserver
    //If this is the first time through, address is the root server
    const char *cstr = address.c_str();
    int addr = inet_addr(cstr);

    //Check the cache at the beginning of each query series
    if(flag){
        cacheadd = checkCache(query, address, clientsock);
        //This indicates the the answer was found in the cache
        if((cacheadd.compare("answered")) == 0){
            return 0;
        } 
        //The value found is expired. Try again
        else if((cacheadd.compare("erase")) == 0){
            sendToNameServer(query, len, clientsock, address, flag);
            return 0;
        }
        cstr = cacheadd.c_str();
    }
    addr = inet_addr(cstr);

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
        //there are no answers
        if(nsResponse[7] == 0){
            struct nameserver next = getNext(nsResponse, lenth);
            cache[next.type] = next;

            printf("\nName: %s\n", next.type.c_str());
            printf("Address: %s\n", next.address.c_str());
            printf("TTL: %d\n", next.ttl);
            printf("Answers: %d\n", nsResponse[7]);
            sendToNameServer(query,len,clientsock, next.address, 0);
        }
        //there are answers
        else{
            printf("I got the answer\n");
            struct nameserver ans = getAnswer(nsResponse, lenth);
            cache[ans.name] = ans;
            answerClient(nsResponse, clientsock,512);
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
                //Check to make sure the two bits are preceded by a pointer
                //Grants extra level of resilience to account for false positives
                int b = ptr & (1 << 7);
                int n = ptr & (1 << 6);
                if(b == 128 &&  n == 64){
                    if(nsResponse[x + 2] == 1){
                        nextQuery = build(nsResponse, nextQuery, x, lenth);
                        break;
                    }
                    else{
                        while(x < 512){
                            //The first additional section entry found is an IPv6 nameserver
                            //This case is not handled
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

//Check the cache
string checkCache(char* query, string address, int sockfd){

    int query_iter = 13;
    int domains = 0;
    //Filter out "www's"
    if(query[13] == 'w' && query[14] == 'w' && query[15] == 'w'){
        query_iter = 17;
    }
    char tmp[80];
    tmp[0] = 'A';
    int tmp_iter = 1;
    //Build the name from the query.
    while(query[query_iter] != '\0'){
        if(query[query_iter] < 41){
            tmp[tmp_iter] = '.';
            query_iter++;
            tmp_iter++;
            domains++;
        }
        tmp[tmp_iter] = query[query_iter];
        tmp_iter++;
        query_iter++;
    }
    tmp[tmp_iter] = '\0';
    string name = string(tmp);

    //Cache is not empty
    if(!cache.empty()){
        int cache_iter = 0;
        //Query request is in cache
        if(cache.count(name)){
            time_t  cttl;
            cttl = (time(NULL) - cache[name].ttl_start);
            cache[name].ttl = cache[name].ttl - (cttl);
            cache[name].ttl_start = time(NULL);

            //check to see if expired, if it is erase
            if((cache[name].ttl) <= 0){
                cache.erase(name);
                address = "erase";
            }
            //query request is in cache, and not expired, build answer packet
            else{

                char answerPacket[512];
                struct dnshdr anshdr = {};
                memcpy(&anshdr, query, 12);
                anshdr.qr = 1;
                anshdr.ancount = htons(1);
                anshdr.authcount = 0;
                anshdr.addcount = 0;
                memcpy(answerPacket, &anshdr, 12);
                int idc = determineValid(query) + 1;
                int other = 12;

                while(other < idc){
                    answerPacket[other] = query[other];
                    other++;
                }
                answerPacket[idc] = 0;
                answerPacket[idc + 1] = 1;
                answerPacket[idc + 2] = 0;
                answerPacket[idc + 3] = 1;
                answerPacket[idc + 4] = 192;
                answerPacket[idc + 5] = 12;
                answerPacket[idc + 6] = 0;
                answerPacket[idc + 7] = 1;
                answerPacket[idc + 8] = 0;
                answerPacket[idc + 9] = 1;
                //Put the ttl back into the packet
                answerPacket[idc + 10] = (cache[name].ttl & 0xFF000000) >>24;
                answerPacket[idc + 11] =  (cache[name].ttl & 0xFF0000) >>16;
                answerPacket[idc + 12] =  (cache[name].ttl & 0xFF00)>>8;
                answerPacket[idc + 13] =  cache[name].ttl & 0xFF;
                answerPacket[idc + 14] = 0;
                answerPacket[idc + 15] = 4;

                string addrs  = cache[name].address;
                string temp_str;
                string run_sub;
                char ad_tmp[16];
                strcpy(ad_tmp, addrs.c_str());
                int index = strcspn(ad_tmp, ".");

                if(index < addrs.length()){
                    temp_str = addrs.substr(0, index);
                    run_sub = addrs.substr(index, addrs.length());
                    answerPacket[idc + 16] = (unsigned char)atoi(temp_str.c_str());
                }

                char ad_tmp1[16];
                strcpy(ad_tmp1, addrs.c_str());
                index = strcspn(ad_tmp1, ".");

                if(index < run_sub.length()){
                    temp_str  = run_sub.substr(1, index);
                    run_sub  = run_sub.substr(index, run_sub.length());
                    answerPacket[idc + 17] = (unsigned char)atoi(temp_str.c_str());
                }
                char ad_tmp2[16];
                strcpy(ad_tmp2, addrs.c_str());
                index = strcspn(ad_tmp2, ".");

                if(index < run_sub.length()){
                    temp_str  = run_sub.substr(2, index);
                    run_sub  = run_sub.substr(index, run_sub.length());
                    answerPacket[idc + 18] = (unsigned char)atoi(temp_str.c_str());
                }
                char ad_tmp3[16];
                strcpy(ad_tmp3, addrs.c_str());
                index = strcspn(ad_tmp3, ".");

                if(index < run_sub.length()){
                    temp_str  = run_sub.substr(3, index);
                    run_sub  = run_sub.substr(index, run_sub.length());
                    answerPacket[idc + 19] = (unsigned char)atoi(temp_str.c_str());
                }
                cout << "Retrieved: " << cache[name].address << " from cache" << endl;;
                answerClient(answerPacket, sockfd, other + 20);
                address = "answered";
            }
        }
        else{
            name = name.substr(1, name.length());
            //Keep looping over parts of the query until we have no more
            //This checks for intermediate nameserver results
            while(cache_iter <= domains){
                cache_iter++;
                //Check to see if the intermediate result is expired
                if(cache.count(name)){
                    time_t  cttl;
                    cttl = (time(NULL) - cache[name].ttl_start);
                    cache[name].ttl = cache[name].ttl - (cttl);
                    cache[name].ttl_start = time(NULL);

                    //if expired, delete
                    if((cache[name].ttl) <= 0){
                        cache.erase(name);
                        address = "erase";
                        return address;
                    }
                    //Found an intermediate result
                    else{
                    cout << "Retrieved " << cache[name].address << " from cache" << endl;
                    return cache[name].address;
                    }
                }
                else{
                    //no intermediate result, yet. Cut out part of the name
                    name = parseName(name);
                }
            }
        }
    }
    return address;
}

//Trim off parts of the query name to see if we
//have any of the intermediate results
string parseName(string name){
    string sub;
    char tmp[80];
    strcpy(tmp, name.c_str());
    int i = strcspn(tmp, ".");
    if(i < name.length()){
        sub = name.substr(i+1, name.length());
    }
    return sub;
}

//Builds the structure for the intermediate nameserver results
//This will be stored in the cache
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
            temp[v] = '.';
            v++;
            d++;
        }
        temp[v] = nsResponse[d];
        d ++;
        v++;
    }
    temp[v] = '\0';
    nextQuery.type = string(temp);

    nextQuery.ttl_start = time(NULL);
    return nextQuery;
}

//getAnswer builds the structure for the answer result
//This will be stored in the cache
struct nameserver getAnswer(char* nsResponse, int lenth){

    char temp_addr[80];
    struct nameserver ans = {};

    //Append an A to the name
    //When used as a key for the cache, 
    //the A will designate a final aswer
    char tmp[80];
    tmp[0] = 'A';
    int tmp_iter = 1;
    int ns_iter = nsResponse[lenth + 6] + 1;
    while(nsResponse[ns_iter] != '\0'){
        if(nsResponse[ns_iter] < 41){
            tmp[tmp_iter] = '.';
            ns_iter++;
            tmp_iter++;
        }
        tmp[tmp_iter] = nsResponse[ns_iter];
        tmp_iter++;
        ns_iter++;
    }
    tmp[tmp_iter] = '\0';
    string name = string(tmp);
    ans.name = name;

    //get Address
    unsigned char addr1 = nsResponse[lenth + 17];
    unsigned char addr2 = nsResponse[lenth + 18];
    unsigned char addr3 = nsResponse[lenth + 19];
    unsigned char addr4  = nsResponse[lenth + 20];
    sprintf(temp_addr, "%u.%u.%u.%u",addr1, addr2, addr3, addr4);
    ans.address = string(temp_addr);

    //get ttl
    u_int8_t ttl_1 = nsResponse[lenth + 11];
    u_int8_t ttl_2 = nsResponse[lenth + 12];
    u_int8_t ttl_3 = nsResponse[lenth + 13];
    u_int8_t ttl_4 = nsResponse[lenth + 14];
    u_int32_t ttl = (u_int32_t) ttl_1 << 24 | (u_int32_t)ttl_2 << 16 | (u_int32_t)ttl_3 << 8 | (u_int32_t)ttl_4;
    ans.ttl = ttl;
    
    //Get the current time
    ans.ttl_start = time(NULL);
    return ans;
}

//Send the answer back to the client
void answerClient(char* answerBuf, int sockfd, int sz){
    sendto(sockfd, answerBuf, sz, 0, (struct sockaddr*)&clientaddr,sizeof(clientaddr));
}

//Check to see if there is an error in the rcode
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

//send response to client indicating an error
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
