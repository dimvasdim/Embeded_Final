#include <inttypes.h>
#include "circular_buffer.h"
#include "client.h"
#include "server.h"

#define MAX_NODES 100
#define TEXT_LENGTH 256
#define MAX_MESSAGES 50
#define PC_AEM 0005
#define RASP_AEM 8404
#define NUM_THREADS 3

int fill_nodes(uint32_t *nodes, char *filename);
int fill_messages(char **messages, char *filename);
char *create_message(char **messages, unsigned int *nodes, int num_of_mess, int num_of_nodes);
void *message_creator(void *args);

struct message_creator_struct
{
  uint32_t *nodes;
  int num_of_nodes;
  int num_of_mess;
  char **messages;
  CircularBuffer creator_buffer;
};

int main(int argc, char **argv)
{
  if (argc != 3)
  {
    printf("Need as argument:\n");
    printf("the txt file with the AEMs,\n");
    printf("the txt file with the possible messages,\n");
    exit(1);
  }
  int i, num_of_nodes, num_of_mess;
  char *str;
  pthread_t thread[NUM_THREADS];
  int rc;

  uint32_t *nodes = (uint32_t *)malloc(MAX_NODES * sizeof(uint32_t));
  num_of_nodes = fill_nodes(nodes, argv[1]);

  char **messages = (char **)malloc(MAX_MESSAGES * sizeof(char *));
  num_of_mess = fill_messages(messages, argv[2]);
  pthread_mutex_init(&pc_mutex, NULL);

  CircularBuffer buffer = CircularBufferCreate(2000);
  int errors = 0;
  for (i=0; i<20; i++)
  {
    str = create_message(messages, nodes, num_of_mess, num_of_nodes);
    CircularBufferInsert(buffer, str);
    if (strlen(str) != MESSAGE_LENGTH)
    {
      errors++;
    }
    sleep(1);
  }
  if (errors)
  {
    printf("The string errors: %d\n", errors);
    exit(0);
  }
  CheckLength(buffer);

  void *status;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  struct message_creator_struct msg;
  msg.nodes = nodes;
  msg.num_of_nodes = num_of_nodes;
  msg.num_of_mess = num_of_mess;
  msg.messages = messages;
  msg.creator_buffer = buffer;

  struct client_struct client_data;
  client_data.nodes = nodes;
  client_data.num_of_nodes = num_of_nodes;
  client_data.client_buffer = buffer;

  rc = pthread_create(&thread[0], &attr, server, (void *) buffer);
  if (rc)
  {
    printf("ERROR; return code from pthread_create() is %d\n", rc);
    exit(-1);
  }

  rc = pthread_create(&thread[1], &attr, client, (void *) &client_data);
  if (rc)
  {
    printf("ERROR; return code from pthread_create() is %d\n", rc);
    exit(-1);
  }

  rc = pthread_create(&thread[2], &attr, message_creator, (void *) &msg);
  if (rc)
  {
    printf("ERROR; return code from pthread_create() is %d\n", rc);
    exit(-1);
  }

  pthread_attr_destroy(&attr);

  //sleep(3600);
  for (i=0; i<NUM_THREADS; i++)
  {
    //pthread_cancel(thread[i]);
    rc = pthread_join(thread[i], &status);
    if (rc)
    {
      printf("ERROR; return code from pthread_join() is %d\n", rc);
      exit(-1);
    }
  }

  printf("This is from pc:\n");
  //printf("This is from rasp:\n");
  CircularBufferPrint(buffer);

  free(nodes);
  for (i=0; i<num_of_mess; i++)
  {
    free(messages[i]);
  }
  free(messages);
  CircularBufferFree(buffer);
  pthread_mutex_destroy(&pc_mutex);
  pthread_exit(NULL);
  return 1;
}

int fill_nodes(uint32_t *nodes, char *filename)
{
  FILE *fin;
  int i = 0, f;
  fin = fopen(filename, "r");
  if (fin == NULL)
  {
    printf("Error opening the file...\n");
    exit(1);
  }

  while (feof(fin) == 0)
  {
    f = fscanf(fin, "%u", &nodes[i]);
    i++;
  }
  i--;
  int num_of_nodes = i;
  fclose(fin);
  nodes = (uint32_t *)realloc(nodes, sizeof(uint32_t) * num_of_nodes);
  if (nodes == NULL)
  {
    printf("Memory allocation failed...\n");
    exit(0);
  }
  return num_of_nodes;
}

int fill_messages(char **messages, char *filename)
{
  FILE *fin;
  char *str = (char *)malloc(TEXT_LENGTH * sizeof(char));
  fin = fopen(filename, "r");
  if (fin == NULL)
  {
    printf("Error opening the file...");
    exit(1);
  }
  int i = 0;
  str = fgets(str, TEXT_LENGTH, fin);
  while (str != NULL)
  {
    messages[i] = (char *)malloc(TEXT_LENGTH * sizeof(char));
    str[strlen(str) - 1] = '\0';
    strcpy(messages[i], str);
    str = fgets(str, TEXT_LENGTH, fin);
    i++;
  }
  int num_of_mess = i;
  //printf("num_of_mess: %d\n", num_of_mess);
  fclose(fin);
  free(str);
  messages = (char **)realloc(messages, sizeof(char *) * num_of_mess);
  if (messages == NULL)
  {
    printf("Memory allocation failed...\n");
    exit(0);
  }
  return num_of_mess;
}

char *create_message(char **messages, unsigned int *nodes, int num_of_mess, int num_of_nodes)
{
  char *str, *temp;
  uint64_t timestamp;
  struct timeval tv;
  int rand_message, rand_node;

  //select a random message from the available ones
  rand_message = rand() % num_of_mess;
  //select a random node to send the message
  rand_node = rand() % num_of_nodes;

  str = (char *)malloc(MESSAGE_LENGTH * sizeof(char));
  temp = (char *)malloc(MESSAGE_LENGTH * sizeof(char));
  if (str == NULL)
  {
    printf("Memory allocation failed...\n");
    exit(0);
  }
  if (temp == NULL)
  {
    printf("Memory allocation failed...\n");
    exit(0);
  }

  gettimeofday(&tv, NULL);
  timestamp = (uint64_t) tv.tv_sec;

  snprintf(temp, MESSAGE_LENGTH,"%u_%u_%" PRIu64 "_%s", PC_AEM, nodes[rand_node], timestamp, messages[rand_message]);
  strcpy(str, temp);
  memset(str + strlen(temp), ' ', (MESSAGE_LENGTH - strlen(temp)) * sizeof(char));
  str[MESSAGE_LENGTH] = '\0';

  return str;
}

void *message_creator(void *args)
{
  struct message_creator_struct *data;
  data = (struct message_creator_struct *) args;
  char *str;
  int rand_time;
  int offset = 60; //60
  int interval = 240; //240
  while(1)
  {
    rand_time = 0;
    rand_time = rand() % interval; //0 -- interval
    rand_time += offset; //offset -- (offset + interval)
    sleep(rand_time);
    str = create_message(data->messages, data->nodes, data->num_of_mess, data->num_of_nodes);
    pthread_mutex_lock (&pc_mutex);
    CircularBufferInsert(data->creator_buffer, str);
    pthread_mutex_unlock (&pc_mutex);
    //printf("Created: %s\n", str);
  }
}
