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

int main(int argc, char *argv[]){
  int type, sokt, port; //sokt: socket file descriptor, type: type of connection to be used (0: tcp, 1: udp)
  const char type_tcp[] = "-tcp";
  const char type_udp[] = "-udp";
  fd_set fdset_send;
  fd_set fdset_recv;
  struct timeval tv;
  tv.tv_sec = 5;
  tv.tv_usec = 0;

  // getting arguments
  if(argc != 5){
    const char t = argc - 1;
    fprintf(stderr, "Error: Incorrect number of arguments: %d \nRequired 3 in the following order: \n1. String \n2. IP Address \n3. Type \n4. Port", t);
    return 0;
  }

  if(strcmp(type_tcp, argv[3])==0){
    type = 0;
  }
  else if(strcmp(type_udp, argv[3])==0){
    type = 1;
  } else{
    fprintf(stderr, "Error: Incorrect type.");
    return 0;
  }
  char str[strlen(argv[1])+1];
  strcpy(str, argv[1]);
  port = (int)atoi(argv[4]);

  // creating a socket
  if(type==0){
    sokt = socket(AF_INET, SOCK_STREAM, 0);
  }
  else{
    sokt = socket(AF_INET, SOCK_DGRAM, 0);
  }

  // timeout option
  setsockopt(sokt, SOL_SOCKET, SO_SNDTIMEO, (struct timeval *)&tv, sizeof(tv));
  setsockopt(sokt, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(tv));

  //specify address
  struct sockaddr_in destn_addr;
  destn_addr.sin_family = AF_INET;
  destn_addr.sin_port = htons(port);
  destn_addr.sin_addr.s_addr = inet_addr(argv[2]);

  char response[256];
  int len;

  if(type==0){
    //connect
    int connection_status = connect(sokt, (struct sockaddr *) &destn_addr, sizeof(destn_addr));
    if(connection_status == -1){
      fprintf(stderr, "Error: Could not connect to the remote socket.\n");
      return 0;
    }

    // send data
    int sent = send(sokt, str, sizeof(str), 0);
    if(sent==-1){
      fprintf(stderr, "Error: Unableto send. Closing socket.");
      close(sokt);
      return 0;
    }
    // recieve data
    len = recv(sokt, (char*)&response, sizeof(response), 0);
  }
  else{
    int sent = sendto(sokt, str, sizeof(str), 0, (struct sockaddr *) &destn_addr, sizeof(destn_addr));
    if(sent==-1){
      fprintf(stderr, "Error: Unable to send. Closing socket.");
      close(sokt);
      return 0;
    }
    // recieve data
    len = recv(sokt, (char*)&response, sizeof(response), 0);
    //len = recvfrom(sokt, (char*)&response, sizeof(response), 0, (struct sockaddr *) &destn_addr, (socklen_t*)sizeof(destn_addr));
  }

  response[len] = '\0';
  printf("Server Response: %s \n", (char *)&response);

  // close function
  close(sokt);
}
