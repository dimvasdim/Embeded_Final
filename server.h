/*
  server
*/

#ifndef Server_h
#define Server_h
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "circular_buffer.h"

#define PORT "2288" // the port users will be connecting to
#define BACKLOG 10 // how many pending connections queue will hold
#define PC_ADDRESS "10.0.0.5"
#define RASP_ADDRESS "10.0.84.04"

void *server_send(void *args);

struct server_send_data
{
  int new_fd;
  CircularBuffer server_buffer;
};

void *server(void *args)
{
  int sockfd, new_fd; // listen on sock_fd, new connection on new_fd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address information
  struct server_send_data data;
  socklen_t sin_size;
  int yes=1;
  char s[INET_ADDRSTRLEN];
  int rv,rc;
  pthread_t thread;
  CircularBuffer server_buffer = (CircularBuffer) args;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(PC_ADDRESS, PORT, &hints, &servinfo)) != 0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    pthread_exit((void *) 1);
  }

  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next)
  {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
      perror("server: socket");
      continue;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
      perror("setsockopt");
      exit(1);
    }
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(sockfd);
      perror("server: bind");
      continue;
    }
    break;
  }
  freeaddrinfo(servinfo); // all done with this structure
  if (p == NULL)
  {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }

  if (listen(sockfd, BACKLOG) == -1)
  {
    perror("listen");
    exit(1);
  }
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  void *status;

  while(1)
  { // main accept() loop
    printf("server: waiting for connections...\n");
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1)
    {
      perror("accept");
      continue;
    }
    inet_ntop(their_addr.ss_family, (struct sockaddr_in*)&their_addr, s, sizeof s);
    printf("server: got connection from %s\n", s);
    data.server_buffer = server_buffer;
    data.new_fd = new_fd;
    pthread_create(&thread, &attr, server_send, (void *) &data);
    if (rc)
    {
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }
    rc = pthread_join(thread, &status);
    if (rc)
    {
      printf("ERROR; return code from pthread_join() is %d\n", rc);
      exit(-1);
    }
    close(new_fd);
  }
  pthread_attr_destroy(&attr);
  close(sockfd);
  pthread_exit((void *) 0);
}

void *server_send(void *args)
{
  struct server_send_data *data;
  data = (struct server_send_data *) args;
  CircularBuffer server_buffer = data->server_buffer;
  int numbytes;
  int counter = 0;
  int errors = 0;
  char *buf = (char *)malloc(MESSAGE_LENGTH * sizeof(char));
  while(counter < server_buffer->dataSize)
  {
    memset(buf, '.', MESSAGE_LENGTH);
    pthread_mutex_lock (&pc_mutex);
    buf = CircularBufferRead(server_buffer, counter);
    pthread_mutex_unlock (&pc_mutex);
    //memset(buf + strlen(buf), ' ', (MESSAGE_LENGTH - strlen(buf)) * sizeof(char));
    //buf[MESSAGE_LENGTH] = '\0';
    if ((numbytes = send(data->new_fd, buf, MESSAGE_LENGTH, 0)) == -1)
    {
      perror("send");
    }
    if (numbytes != MESSAGE_LENGTH)
    {
      errors++;
    }
    //usleep(2000);
    counter++;
  }
  printf("Not well sent: %d\n", errors);
  close(data->new_fd);
  pthread_exit((void *) 0);
}

#endif
