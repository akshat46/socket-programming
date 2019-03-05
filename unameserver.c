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

char* uNameCall(char*);

int main(int argc, char *argv[]){
  const char type_tcp[] = "-tcp";
  const char type_udp[] = "-udp";
  const char port_opt[] = "-port";
  int type, sockfd, newSockfd;
  int port = -1;
  struct timeval tv;
  tv.tv_sec = 5;
  tv.tv_usec = 0;

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

  // TODO: change type to tcp|udp
  if(type==0){
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
  }
  else{
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  }

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
  if(port<0){
    printf("Listening on port: %i \n ****** \n\n", ntohs(address.sin_port));
  }

  if(type==0){ //TCP
    //Listening
    if(listen(sockfd, 5) < 0){
      printf("listen failed");
      return 0;
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

      int len = -1;
      char response[20];
      len = recv(newSockfd, (char*)&response, sizeof(response)-1, 0);
      //response[len] = '\0';
      if(len>0){
        response[len] = '\0';
      }
      printf("Client Request: %s \n", (char *)&response);
      response[strcspn(response, "\r\n")] = 0;
      char *options = (char *)&response;
      char *result = uNameCall(options);
      printf("Server Response: %s \n", result);
      int sent = send(newSockfd, result, strlen(result), 0);
      if(sent==-1){
        fprintf(stderr, "Error: Unableto send. Closing socket.");
        //close(sokt);
        return 0;
      }
      close(newSockfd);
      printf("Closing Connection\n *** \n\n");
    }
  }
  else if(type==1){ //UDP
    for(;;)
    {
      int len = -1;
      char response[20];
      len = recvfrom(sockfd, (char*)&response, sizeof(response)-1, 0, (struct sockaddr *)&address, (socklen_t *)&addrlen);
      //response[len] = '\0';
      if(len>0){
        response[len] = '\0';
      }
      printf("Client Response: %s \n", (char *)&response);
      response[strcspn(response, "\r\n")] = 0;
      char *options = (char *)&response;
      char *result = uNameCall(options);
      printf("Server Response: %s \n", result);
      int sent = sendto(sockfd, result, strlen(result), 0,  (struct sockaddr *)&address, (socklen_t )addrlen);
      if(sent==-1){
        fprintf(stderr, "Error: Unableto send. Closing socket.");
        //close(sokt);
        return 0;
      }
    }
  }
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
  if(t!=0){
    snprintf(result, 32, "%s%d", "Error: Uname exit with code ", t);
  }
  return result;
}
