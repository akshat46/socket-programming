// Write a program that reads and prints to the screen the default values for following options for a socket (use the appropriate socket if the semantic is specific to one protocol).
//
//
// For the options that are available for the two protocols use the following format:
//
//
//
// SO_RCVLOWAT_TCP=size
// SO_RCVLOWAT_UDP=size
// Always print the TCP value first and use the order given above, also one option per line with all values related to that option.

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>

void printResult(char*, int, int);
void printBits(size_t const, void const * const);

int main(){
  int sockfdTCP, sockfdUDP, optlen, value;
  struct linger linger_timer;
  sockfdTCP = socket(AF_INET, SOCK_STREAM, 0);
  sockfdUDP = socket(AF_INET, SOCK_DGRAM, 0);
  if(sockfdTCP==-1){
    printf("Error creating TCP Socket\n");
    return -1;
  }
  if(sockfdUDP==-1){
    printf("Error creating UDP Socket\n");
    return -1;
  }

  optlen = sizeof(value);

  value = fcntl (sockfdUDP, F_GETFL, 0);
  int val = value & O_NONBLOCK;
  if(val == O_NONBLOCK){
    printf("O_NONBLOCK= true\n");
  }
  else{
    printf("O_NONBLOCK= false\n");
  }

  value = 0;
  val = 0;
  value = fcntl (sockfdUDP, F_GETFL, 0);
  val = value & O_ASYNC;
  if(val == O_ASYNC){
    printf("O_ASYNC= true\n");
  }
  else{
    printf("O_ASYNC= false\n");
  }

  /******UDP********/
  value = 0;
  if(getsockopt (sockfdUDP, SOL_SOCKET, SO_BROADCAST, &value, &optlen)==-1){
    printf("Error getsockopt(SO_BROADCAST)\n");
  }
  printResult("SO_BROADCAST_UDP", value, 1);

  value = 0;
  if(getsockopt (sockfdUDP, SOL_SOCKET, SO_DONTROUTE, &value, &optlen)==-1){
    printf("Error getsockopt(SO_DONTROUTE)\n");
  }
  printResult("SO_DONTROUTE_UDP", value, 1);

  /******TCP*********/

  value = 0;
  if(getsockopt (sockfdTCP, SOL_SOCKET, SO_KEEPALIVE, &value, &optlen)==-1){
    printf("Error getsockopt(SO_KEEPALIVE)\n");
  }
  printResult("SO_KEEPALIVE_TCP", value, 1);
  printf("SO_KEEPALIVE_TCP timer=? \n");
  //
  // value = 0;
  // if(getsockopt (sockfdTCP, IPPROTO_TCP, TCP_KEEPALIVE, &value, &optlen)==-1){
  //   printf("Error getsockopt(TCP_KEEPALIVE)\n");
  // }
  // printf("TCP_KEEPALIVE_TCP=", value);
  //
  // value = 0;
  // struct timeval tval;
  // if(getsockopt (sockfdTCP, IPPROTO_TCP, TCP_KEEPALIVE, &tval, &optlen)==-1){
  //   printf("Error getsockopt(TCP_KEEPALIVE)\n");
  // }
  // printf("TCP_KEEPALIVE_TCP timer=", tval.tv_sec);

  value = 0;
  if(getsockopt (sockfdTCP, SOL_SOCKET, SO_LINGER, &value, &optlen)==-1){
    printf("Error getsockopt(SO_LINGER)\n");
  }
  printResult("SO_LINGER_TCP", value, 1);

  int t = sizeof(linger_timer);
  if(getsockopt (sockfdTCP, SOL_SOCKET, SO_LINGER, &linger_timer, &t)==-1){
    printf("Error getsockopt(SO_LINGER)\n");
  }
  printf("SO_LINGER_TCP timer = %d\n", linger_timer.l_linger);

  value = 0;
  if(getsockopt (sockfdTCP, SOL_SOCKET, SO_RCVBUF, &value, &optlen)==-1){
    printf("Error getsockopt(SO_RCVBUF)\n");
  }
  printResult("SO_RCVBUF_TCP", value, 0);

  value = 0;
  if(getsockopt (sockfdTCP, SOL_SOCKET, SO_SNDBUF, &value, &optlen)==-1){
    printf("Error getsockopt(SO_SNDBUF)\n");
  }
  printResult("SO_SNDBUF_TCP", value, 0);

  value = 0;
  if(getsockopt (sockfdTCP, SOL_SOCKET, SO_RCVLOWAT, &value, &optlen)==-1){
    printf("Error getsockopt(SO_RCVLOWAT)\n");
  }
  printResult("SO_RCVLOWAT_TCP", value, 0);

  value = 0;
  if(getsockopt (sockfdTCP, SOL_SOCKET, SO_SNDLOWAT, &value, &optlen)==-1){
    printf("Error getsockopt(SO_SNDLOWAT)\n");
  }
  printResult("SO_SNDLOWAT_TCP", value, 0);

  value = 0;
  if(getsockopt (sockfdTCP, SOL_SOCKET, SO_REUSEADDR, &value, &optlen)==-1){
    printf("Error getsockopt(SO_REUSEADDR)\n");
  }
  printResult("SO_REUSEADDR_TCP", value, 1);

  value = 0;
  if(getsockopt (sockfdTCP, IPPROTO_TCP, TCP_MAXSEG, &value, &optlen)==-1){
    printf("Error getsockopt(TCP_MAXSEG)\n");
  }
  printResult("TCP_MAXSEG_TCP", value, 0);

  value = 0;
  if(getsockopt (sockfdTCP, IPPROTO_TCP, TCP_NODELAY, &value, &optlen)==-1){
    printf("Error getsockopt(TCP_NODELAY)\n");
  }
  printResult("TCP_NODELAY_TCP", value, 1);

  /*****UDP******/

  value = 0;
  if(getsockopt (sockfdUDP, SOL_SOCKET, SO_RCVBUF, &value, &optlen)==-1){
    printf("Error getsockopt(SO_RCVBUF)\n");
  }
  printResult("SO_RCVBUF_UDP", value, 0);

  value = 0;
  if(getsockopt (sockfdUDP, SOL_SOCKET, SO_SNDBUF, &value, &optlen)==-1){
    printf("Error getsockopt(SO_SNDBUF)\n");
  }
  printResult("SO_SNDBUF_UDP", value, 0);

  value = 0;
  if(getsockopt (sockfdUDP, SOL_SOCKET, SO_RCVLOWAT, &value, &optlen)==-1){
    printf("Error getsockopt(SO_RCVLOWAT)\n");
  }
  printResult("SO_RCVLOWAT_UDP", value, 0);

  value = 0;
  if(getsockopt (sockfdUDP, SOL_SOCKET, SO_SNDLOWAT, &value, &optlen)==-1){
    printf("Error getsockopt(SO_SNDLOWAT)\n");
  }
  printResult("SO_SNDLOWAT_UDP", value, 0);
}

void printResult(char* str, int value, int bool){
  // bool = 0 : pinrint int, 1: print boolean
  if(bool==0){
    printf("%s= %d\n", str, value);
  }
  else if(bool==1 && value==0){
    printf("%s= false\n", str);
  }
  else if(bool==1 && value==1){
    printf("%s= true\n", str);
  }
  else{
    printf("error\n");
  }
}

// function from https://stackoverflow.com/questions/111928/is-there-a-printf-converter-to-print-in-binary-format
void printBits(size_t const size, void const * const ptr)
{
  unsigned char *b = (unsigned char*) ptr;
  unsigned char byte;
  int i, j;

  for (i=size-1;i>=0;i--)
  {
    for (j=7;j>=0;j--)
    {
      byte = (b[i] >> j) & 1;
      printf("%u", byte);
    }
  }
  puts("");
}
