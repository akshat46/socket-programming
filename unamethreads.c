// Create an iterative server that supports tcp and udp and can provide the outcome of
// any uname command possible (i.e. uname -a, uname -v or combination of command line switches).
// Your server will receive as request only the switches requested for the command (i.e. a, or vi).
// Your server must be run with the following command: unameserver (-tcp|-udp) [-port PortNumber].
// If no port is specified you server should bind to a free port and display port allocated in the
// stdout (i.e. "Listening at port number XX"). If port is given no need to print out port number.

// unameserver (-tcp|-udp) [-port PortNumber]

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
#include <signal.h>
#include <pthread.h>

void *tcp_child(void *);
void *udp_child(void *);
char* uNameCall(char*);
struct arg_struct{
  int sock_struct;
  char response_struct[20];
  struct sockaddr_in address_struct;
  int addrlen_struct;
};
static pthread_mutex_t thr_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]){
  const char type_tcp[] = "-tcp";
  const char type_udp[] = "-udp";
  const char port_opt[] = "-port";
  int type, sockfd, newSockfd;
  pid_t pid;
  int port = -1;
  int counter = 0;
  struct timeval tv;
  tv.tv_sec = 5;
  tv.tv_usec = 0;
  signal(SIGCHLD, SIG_IGN);
  pthread_attr_t attrs;

  if(argc < 2){
    const char t = argc - 1;
    fprintf(stderr, "Error: Incorrect number of arguemnts. Acceptable format: unameserver (-tcp|-udp) *(-port PortNumber) \n* = Optional ");
    return 0;
  }

  // determine connection type
  if(strcmp(type_tcp, argv[1])==0){
    type = 0;
    printf("type: tcp \n");
  }
  else if(strcmp(type_udp, argv[1])==0){
    type = 1;
    printf("type: udp \n");
  }
  else{
    fprintf(stderr, "Error: Incorrect type.");
    return 0;
  }

  // determine port number
  if(argc==4){
    if(strcmp(port_opt, argv[2])==0){
      port = (int)atoi(argv[3]);
    }
    else{
      fprintf(stderr, "Error: Incorrect number of arguemnts. Acceptable format: unameserver (-tcp|-udp) *(-port PortNumber) \n* = Optional ");
    }
  }

  // create socket based on connection type
  if(type==0){
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
  }
  else{
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  }

  // setup destination address struct
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  if(port>0){
    address.sin_port = htons(port);
  }
  address.sin_addr.s_addr = INADDR_ANY;
  int addrlen = sizeof(address);

  // binding
  if(bind(sockfd, (struct sockaddr *)&address, sizeof(address))<0){
    printf("bind failed");
    return 0;
  }

  // thread attributes
  pthread_attr_init(&attrs);
  pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
  if(type==0){ //TCP
    //Listening
    if(listen(sockfd, 5) < 0){
      printf("listen failed");
      return 0;
    }

    struct sockaddr_in sin;
    socklen_t len_sin = sizeof(sin);
    if (getsockname(sockfd, (struct sockaddr *)&sin, &len_sin) == -1){
      perror("getsockname");
    }
    else{
      printf("port number %d\n", ntohs(sin.sin_port));
    }

    for(;;)
    {
      newSockfd = accept(sockfd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
      if(newSockfd<0){
        printf("accept failed");
        return 0;
      }
      // timeout option
      setsockopt(newSockfd, SOL_SOCKET, SO_SNDTIMEO, (struct timeval *)&tv, sizeof(tv));
      setsockopt(newSockfd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(tv));
      pthread_t t;
      pthread_create(&t, &attrs, tcp_child, &newSockfd);
    }
  }
  else if(type==1){ //UDP
    struct sockaddr_in sin;
    socklen_t len_sin = sizeof(sin);
    if (getsockname(sockfd, (struct sockaddr *)&sin, &len_sin) == -1){
      perror("getsockname");
    }
    else{
      printf("port number %d\n", ntohs(sin.sin_port));
    }
    for(;;)
    {
      int len = -1;
      char response[20];
      pthread_mutex_trylock(&thr_mutex);
      len = recvfrom(sockfd, (char*)&response, sizeof(response)-1, 0, (struct sockaddr *)&address, (socklen_t *)&addrlen);
      pthread_mutex_unlock(&thr_mutex);
      //printf("here 151 %s \n", response);
      if(len>0){
        response[len] = '\0';
        struct arg_struct args;
        args.sock_struct = sockfd;
        strcpy(args.response_struct, response);
        args.address_struct = address;
        args.addrlen_struct = addrlen;
        pthread_t t;
        pthread_create(&t, &attrs, udp_child, (void *)&args);
      }

    }
  }
}

void *tcp_child(void *arg){
  int newSockfdThread = *(int *)arg;
  int len = -1;
  char response[20];
  len = recv(newSockfdThread, (char*)&response, sizeof(response)-1, 0);
  //response[len] = '\0';
  if(len>0){
    response[len] = '\0';
  }
  printf("Client Request: %s \n", (char *)&response);
  response[strcspn(response, "\r\n")] = 0;

  char *options = (char *)&response;
  char *result = uNameCall(options);

  printf("Server Response: %s \n", result);
  int sent = send(newSockfdThread, result, strlen(result), 0);
  if(sent==-1){
    fprintf(stderr, "Error: Unableto send. Closing socket.");
    close(newSockfdThread);
    return NULL;
  }
  //close(newSockfdThread);
  printf("Closing Connection\n\n *** \n\n");
  close(newSockfdThread);
  return NULL;
}

void *udp_child(void *args){
  struct arg_struct *args_struct = (struct arg_struct *)args;
  int sockfd = args_struct->sock_struct;
  char response[20];
  strcpy(response, args_struct->response_struct);
  struct sockaddr_in address = args_struct->address_struct;
  int addrlen = args_struct->addrlen_struct;

  printf("Client Request: %s \n", (char *)&response);
  response[strcspn(response, "\r\n")] = 0;
  char *options = (char *)&response;
  char *result = uNameCall(options);
  printf("Server Response: %s \n", result);
  pthread_mutex_trylock(&thr_mutex);
  int sent = sendto(sockfd, result, strlen(result), 0,  (struct sockaddr *)&address, (socklen_t )addrlen);
  pthread_mutex_unlock(&thr_mutex);
  if(sent==-1){
    fprintf(stderr, "Error: Unableto send. Closing socket.");
    return NULL;
  }
  return NULL;
}

char* uNameCall(char* parameters){
  FILE *fp;
  char path[1035];
  char *uname = "uname -";
  char *options = parameters;
  char buf[256];
  snprintf(buf, sizeof(buf), "%s%s", uname, options);

  /* Open the command for reading. */
  fp = popen(buf, "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }

  /* Read the output a line at a time - output it. */
  while (fgets(path, sizeof(path)-1, fp) != NULL) {
  }

  /* close */
  int t = pclose(fp);
  char *result = path;
  return result;
}
