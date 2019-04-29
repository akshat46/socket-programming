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
#include <sys/time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>// for inet_addr()
#include <arpa/inet.h> // for inet_addr()
#include <fcntl.h> // non-blocking
#include <unistd.h> // for close()
#include <string.h> // for strcmp()
#include <netdb.h> // getservbyname()

// protocol struct
struct chat_proto{
    unsigned short buff;
    char opcode;
    char message[1];
};
//your_program -mcip x.x.x.x. -port XX
struct args{
    char ip_addr[20];
    int port;
}
int MAX_BUFF = 65794;

// sockfd, port#, ip/dns
int manageClient(int, int, struct sockaddr_in);
int manageServer(int, int, struct sockaddr_in);
int getBuff(const char*);
int getOpCode(const char*);
char* getText(char*, int);
struct chat_proto* extractData(const char*);
int sendData(int, const char*, unsigned short, int);
char* setBuff(unsigned short);
char* setOpCode(int);

struct args args;

int main(int argc, char *argv[]){
    struct sockaddr_in conct_addr;

    if(argc==4){
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

    conct_addr.sin_family = AF_INET;
    conct_addr.sin_port = htons(args.port);
    conct_addr.sin_addr.s_addr = INADDR_ANY;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    switch(type){
        case 0: // active: client
        manageClient(sockfd, port, conct_addr);
        return 1;
        case 1: // passive: server
        manageServer(sockfd, port, conct_addr);
        return 1;
        default :
        printf("Error initializing type.\n");
        return -1;
    }
}

int manageClient(int sockfd, int port, struct sockaddr_in address){
    int connection_status;
    connection_status = connect(sockfd, (struct sockaddr *) &address, sizeof(address));
    char* message = "Testing message division with the custom protocol's packet.";
    sendData(sockfd, message, (unsigned short)strlen(message), 1);
    char packet[MAX_BUFF];
    int t = recv(sockfd, packet, MAX_BUFF, 0);
    //close(sockfd);
    return 1;
}

int manageServer(int sockfd, int port, struct sockaddr_in address){
    struct sockaddr_in client_addr;
    struct chat_proto* packet_struct;
    char packet[MAX_BUFF];
    int newSockfd;
    char* message;
    int len = -1;

    if(bind(sockfd, (struct sockaddr *)&address, sizeof(address))<0){
        printf("bind failed\n");
        return -2;
    }
    if(listen(sockfd, 5) < 0){
        printf("listen failed\n");
        return -2;
    }
    printf("Listening for incoming messages..\n");
    for(;;){
        newSockfd = accept(sockfd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr);
        len = recv(newSockfd, packet, MAX_BUFF, 0);
        printf("packet: %s\n", packet);
        packet_struct = extractData(packet);
        // if(len>0){
        //     response[len] = '\0';
        // }
    }
    return 1;
}


/*******************************
Helper Functions
*******************************/

struct chat_proto* extractData(const char* packet)
{
    char temp[strlen(packet)];
    strcpy(temp, packet);
    int buff = getBuff(packet);
    int opcode = getOpCode(packet);
    char* text = getText(temp, buff);
    //printf("buff: %d\n", buff);
    printf("opcode: %d\n", opcode);
    printf("text: %s\n", text);

    struct chat_proto* packet_struct = malloc(buff+6);
    packet_struct->buff = buff;
    packet_struct->opcode = opcode;
    memcpy(packet_struct->message, text, strlen(text));
    free((void*)text);
    //printf("testing here\n");
}

int getBuff(const char* packet){
    char buff_str[6] = {'0'};
    strncpy(buff_str, packet, 6);
    printf("buff in getBuff: %s\n", buff_str);
    unsigned short buff = (unsigned short) strtoul((char *)buff_str, NULL, 0);
    printf("buff: %hu\n", buff);
    return buff;
}

int getOpCode(const char* packet){
    int op = (int)packet[5];
    //int op = (int) strtoul(op_str, NULL, 0);
    return op;
}

char* getText(char* packet, int buff){
    int len = strlen(packet);
    char* text = malloc(buff);
    int temp = len - buff;
    text = packet + temp;
    return text;
}

int sendData(int sock, const char* data, unsigned short dataSize, int opcode)
{
    const char *buff = setBuff(dataSize);
    const char *op = setOpCode(opcode);
    printf("text: %s\n", data);
    char* packet = malloc(strlen(buff) + strlen(op) + strlen(data));
    sprintf(packet, "%s%s%s", buff, op, data);

    int len = 6 + strlen(packet);
    send(sock, packet, len, 0);

    free((void *)buff);
    free((void *)op);
    free(packet);
    return 1;
}

char* setBuff(unsigned short dataSize){
    char buff[4];
    sprintf(buff, "%hu", dataSize);
    int targetStrLen = 5;
    const char *padding="00000";
    char *result = malloc(4);
    int padLen = targetStrLen - strlen(buff);
    if(padLen < 0) padLen = 0;
    sprintf(result, "%*.*s%s", padLen, padLen, padding, buff);
    printf("buff: %s\n", result);
    return result;
}

char* setOpCode(int opcode){
    char* op = malloc(1);
    sprintf(op, "%d", opcode);
    printf("op: %s\n", op);
    return op;
}


// struct chat_proto{
//     unsigned buff;
//     char opcode;
//     char message[1];
// };
