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

int main(int argc, char *argv[]){
  int type, sokt; //sokt: socket file descriptor, type: type of connection to be used (0: tcp, 1: udp)
  const char type_tcp[] = "-tcp";
  const char type_udp[] = "-udp";
  fd_set fdset_send;
  fd_set fdset_recv;
  struct servent *serv;
  struct hostent *host;
  struct in_addr **host_ip;
  int addr_type;
  struct timeval tv;
  tv.tv_sec = 60;
  tv.tv_usec = 0;

  // getting arguments
  if(argc != 5){
    const char t = argc - 1;
    fprintf(stderr, "Error: Incorrect number of arguments: %d \nRequired 3 in the following order: \n1. String \n2. Domain name \n3.Service Name \n4. Type", t);
    return 0;
  }

  // 1. string
  char str[strlen(argv[1])+1];
  strcpy(str, argv[1]);

  //2. host_name-name
  char host_name[strlen(argv[2])];
  strcpy(host_name, argv[2]);

  //3. service-name
  char srv_name[strlen(argv[3])];
  strcpy(srv_name, argv[3]);

  // 4. option
  if(strcmp(type_tcp, argv[4])==0){
    type = 0;
  }
  else if(strcmp(type_udp, argv[4])==0){
    type = 1;
  } else{
    fprintf(stderr, "Error: Incorrect type.");
    return 0;
  }

  host = gethostbyname(host_name);
  host_ip = (struct in_addr **) host->h_addr_list;
  addr_type = host->h_addrtype;
  printf("\"%s\" Ip address: %s \n", host->h_name, inet_ntoa(*host_ip[0]));

  // creating a socket
  if(type==0){
    const char tcp[] = "tcp";
    serv = getservbyname(srv_name, tcp);
    sokt = socket(addr_type, SOCK_STREAM, 0);
  }
  else{
    const char udp[] = "udp";
    serv = getservbyname(srv_name, udp);
    sokt = socket(addr_type, SOCK_DGRAM, 0);
  }

  printf("\"%s\" port number: %d \n", srv_name, ntohs(serv->s_port));

  // timeout option
  setsockopt(sokt, SOL_SOCKET, SO_SNDTIMEO, (struct timeval *)&tv, sizeof(tv));
  setsockopt(sokt, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(tv));

  //specify address
  int port_num = ntohs(serv->s_port);
  char response[strlen(str)];
  int len;
  struct sockaddr_in destn_addr;
  struct sockaddr_in6 destn_addr6;
  if(addr_type==AF_INET){
    destn_addr.sin_family = AF_INET;
    destn_addr.sin_port = ntohs(port_num);
    inet_pton(AF_INET, inet_ntoa(*host_ip[0]), &(destn_addr.sin_addr));
  } else if(addr_type==AF_INET6){
    destn_addr6.sin6_family = AF_INET6;
    destn_addr6.sin6_port = ntohs(port_num);
    inet_pton(AF_INET6, inet_ntoa(*host_ip[0]), &(destn_addr6.sin6_addr));
  }
  //destn_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*host_ip[0]));

  if(type==0){
    //connect
    printf("establishing connection... \n");
    int connection_status;
    if(addr_type==AF_INET){
      connection_status = connect(sokt, (struct sockaddr *) &destn_addr, sizeof(destn_addr));
    }else {
      connection_status = connect(sokt, (struct sockaddr *) &destn_addr6, sizeof(destn_addr6));
    }
    if(connection_status == -1){
      fprintf(stderr, "Error: Could not connect to the remote socket.\n");
      return 0;
    }

    printf("sending data... \n");
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
    printf("sending data... \n");
    int sent;

    if(addr_type==AF_INET){
      sent = sendto(sokt, str, sizeof(str), 0, (struct sockaddr *) &destn_addr, sizeof(destn_addr));
    }else {
      sent = sendto(sokt, str, sizeof(str), 0, (struct sockaddr *) &destn_addr6, sizeof(destn_addr6));
    }
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
