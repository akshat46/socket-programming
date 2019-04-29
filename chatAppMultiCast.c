/*******************************
References
*******************************/
// sending structs over sockets: https://stackoverflow.com/questions/4426016/sending-a-struct-with-char-pointer-inside

/*******************************
Error Codes
*******************************/
// 1: no error. returned succesfully
// -1: unknown error
// -2: connection error
// -4: invalid argument(s) passed

// TODO: set socket opt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>// for inet_addr()
#include <arpa/inet.h> // for inet_addr()
#include <fcntl.h> // non-blocking
#include <unistd.h> // for close()
#include <string.h> // for strcmp()
#include <netdb.h> // getservbyname()
#include <pthread.h>
#include <errno.h>

// protocol struct
struct chat_proto{
    char opcode;
    unsigned char nameLength;
    char name;
    unsigned short textLength;
    char message[1];
};
//your_program -mcip x.x.x.x. -port XX
struct args{
    char ip_addr[20];
    int port;
};

int MAX_BUFF = 65794;
int MAX_TEXT = 65536;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct args args;
struct sockaddr_in sock_addr;
struct ip_mreqn mreq;
char* name;
int sockfd;
char opcode = '1';

// sockfd, port#, ip/dns
int initApp();
void* localHandler(void *);
int cliListener(char**);
int setName();

int main(int argc, char *argv[]){
    if(argc!=5){
        printf("Error: Incorrect number of arguments: %d \nRequired 2 in the following order: \n-mcip x.x.x.x -port xx\n", argc-1);
        return -4;
    }

    /*******************************
    IP
    *******************************/
    const char ip_opt[] = "-mcip";
    if(strcmp(ip_opt, argv[1])==0){
        strcpy(args.ip_addr, argv[2]);
    }
    else{
        printf("Invalid port argument.\n");
        return -4;
    }

    /*******************************
    PORT
    *******************************/
    const char port_opt[] = "-port";
    if(strcmp(port_opt, argv[3])==0){
        args.port = (int)atoi(argv[4]);
    }
    else{
        printf("Invalid port argument.\n");
        return -4;
    }

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(args.port);
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(initApp()<0){
        fprintf(stderr, "\nError: Application terminated.\n");
        return -1;
    }

    return 1;
}

int initApp(){
    pthread_attr_t attrs;
    char loopch = 0;

    pthread_attr_init(&attrs);
    pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);

    if(bind(sockfd, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) < 0){
        fprintf(stderr, "\nError: couldn't bind to the socket.\nInternal Error: %s\n", strerror(errno));
        return -1;
    }

    printf("ip: %s, port: %d\n***\n", args.ip_addr, args.port);

    mreq.imr_multiaddr.s_addr = inet_addr(args.ip_addr);
    mreq.imr_address.s_addr = htonl(INADDR_ANY);

    if(setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0){
        fprintf(stderr, "\nError: couldn't add socket membership.\nInternal Error: %s\n", strerror(errno));
        return -1;
    }

    if(setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch)) < 0){
        fprintf(stderr, "Error: couldn't set IP_MULTICAST_LOOP.\nInternal Error: %s\n", strerror(errno));
        //close(sd);
        return -1;
    }
    if(setName()<0){
        printf("Error: Couldn't set name.");
        return -1;
    }
    // thread for handling cli input and sending
    pthread_t t;
    if(pthread_create(&t, &attrs, localHandler, 0)<0){
        fprintf(stderr, "Error: couldn't create thread.\n Internal Error: %s\n", strerror(errno));
    }

    for(;;){

        char recvBuff[65536] = {'\0'};
        //mutex
        if(strlen(recvBuff)!=0){
            recvfrom(sockfd, (char*)&recvBuff, MAX_BUFF, 0, NULL, 0);
            printf("message recieved: %s\n", recvBuff);
            memset(recvBuff, 0, 65536);
        }
    }

    return 1;
}

void* localHandler(void *thread_args){
    char* input;

    while(opcode == '1'){
        cliListener(&input);
        printf("input: %s\n", input);
        createPacket(input);
    }
    // if opcode 2, send bye
    //if anything else, send error and terminate
}

int createPacket(char* text){
    char opcode = '1';
}

int cliListener(char** input){
    char temp_input[65536] = {'\0'};
    fgets(temp_input, MAX_TEXT, stdin);
    int input_size = strlen(temp_input);

    *input = malloc(input_size);
    strcpy(*input, temp_input);

    return 1;
}

int setName(){
    char temp_input[255] = {'\0'};
    while(temp_input[0]=='\0'){
        printf("Enter a username for yourself: ");
        fgets(temp_input, 255, stdin);
        if(temp_input=='\0'){
            printf("Name cant be empty. Try again.");
        }
    }
    int input_size = strlen(temp_input);
    name = malloc(input_size);
    strcpy(name, temp_input);
    printf("***Username Set***\n");
    return 1;
}

/*******************************
Helper Functions
*******************************/
