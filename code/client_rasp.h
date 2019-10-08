/*
  client
*/

#ifndef Client_hrasp
#define Client_hrasp
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "circular_buffer.h"

#define PORT "2288" // the port users will be connecting to
#define MESSAGE_LENGTH 276
#define PC_ADDRESS "10.0.0.5"
#define RASP_ADDRESS "10.0.84.04"

char *create_address(uint32_t node);

struct client_struct
{
  int num_of_nodes;
  uint32_t *nodes;
  CircularBuffer client_buffer;
};

void *client(void *args)
{
 int sockfd, numbytes;
 char buf[MESSAGE_LENGTH];
 struct addrinfo hints, *servinfo, *p;
 int rv, i;
 char s[INET_ADDRSTRLEN];
 struct client_struct *data;
 data = (struct client_struct *) args;
 CircularBuffer client_buffer = data->client_buffer;
 uint32_t temp_node;
 int count;
 char *address = (char *)malloc(10*sizeof(char));
 int select = -1;
 //struct sockaddr_in sa;

 memset(&hints, 0, sizeof hints);
 hints.ai_family = AF_INET;
 hints.ai_socktype = SOCK_STREAM;

 for(;;)
 {
   while(1)
   {
     select++;
     if (select == data->num_of_nodes)
     {
       sleep(60);
     }
     select = select % data->num_of_nodes;
     temp_node = data->nodes[select];
     memset(address, '.', 10 * sizeof(char));
     address = create_address(temp_node);

     if ((rv = getaddrinfo(address, PORT, &hints, &servinfo)) != 0)
     {
       printf("Error with addrinfo in client_rasp\n");
       continue;
     }

     // loop through all the results and connect to the first we can
     for(p = servinfo; p != NULL; p = p->ai_next)
     {
       if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
       {
         perror("client: socket");
         continue;
       }
       if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
       {
         close(sockfd);
         //perror("client: connect");
         continue;
       }
       break;
     }
     if (p == NULL)
     {
       //fprintf(stderr, "client: failed to connect\n");
       continue;
     }
     else
     {
       break;
     }
   }
   inet_ntop(p->ai_family, (struct sockaddr_in*)p->ai_addr, s, sizeof s);
   printf("client: connecting to %s\n", s);

   count = 0;
   for(;;)
   {
     memset(buf, ' ', MESSAGE_LENGTH * sizeof(char));
     usleep(500);
     if ((numbytes = recv(sockfd, buf, MESSAGE_LENGTH, 0)) == -1)
     {
       perror("recv");
       continue;
     }
     if (numbytes == 0)
     {
       break;
     }
     if (numbytes != MESSAGE_LENGTH)
     {
       //printf("position client: %d\n", i);
       count++;
     }
     buf[MESSAGE_LENGTH] = '\0';
     pthread_mutex_lock (&rasp_mutex);
     CircularBufferInsert(client_buffer, buf);
     pthread_mutex_unlock (&rasp_mutex);
   }
   printf("Not well received :%d\n", count);
 }

 freeaddrinfo(servinfo); // all done with this structure
 close(sockfd);
 pthread_exit((void *) 0);
}

char *create_address(uint32_t node)
{
  char *temp = (char *)malloc(10*sizeof(char));
  int first, second;
  int divisor = 10;
  while ( node / divisor > divisor ) divisor *= divisor;
  first = node / divisor;
  second = node % divisor;
  snprintf(temp, 10,"10.0.%hu.%hu", first, second);
  return temp;
}

#endif
