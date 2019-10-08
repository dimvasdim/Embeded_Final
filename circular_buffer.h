/*
  CircularBuffer.h
*/

#ifndef CircularBuffer_h
#define CircularBuffer_h
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MESSAGE_LENGTH 276

/*
  A circular buffer(circular queue, cyclic buffer or ring buffer), is a data structure
  that uses a single, fixed-size buffer as if it were connected end-to-end.
  This structure lends itself easily to buffering data streams.
*/

typedef struct s_circularBuffer* CircularBuffer;

// Construct CircularBuffer with ‘size' in byte. You must call CircularBufferFree() in balance for destruction.
CircularBuffer CircularBufferCreate(int size);

// Destruct CircularBuffer
void CircularBufferFree(CircularBuffer cBuf);

// Reset the CircularBuffer
void CircularBufferReset(CircularBuffer cBuf);

// Put data to the tail of a circular buffer
void CircularBufferInsert(CircularBuffer cBuf, char *message);

// Read the message of the selected position from the oldest one into a circular buffer
char *CircularBufferRead(CircularBuffer cBuf, int position);

// Delete the oldest message from a circular buffer
void CircularBufferDelete(CircularBuffer cBuf);

//Print all the messages of the buffer
void CircularBufferPrint(CircularBuffer cBuf);

//Print selected messages of the buffer
void CircularBufferPrintSelected(CircularBuffer cBuf, int start, int end);

//Check if a message of the buffer exists already
int CheckForDuplicate(CircularBuffer cBuf, char *message);

pthread_mutex_t pc_mutex;
pthread_mutex_t rasp_mutex;

struct s_circularBuffer
{
  int size; //capacity of messages
  int dataSize; //occupied data size
  int tail; //the last position offset
  int head; //the oldest position offset
  char **buffer;
};

CircularBuffer CircularBufferCreate(int size)
{
  void *p = malloc(sizeof(struct s_circularBuffer) + size * sizeof(char *));
  if (p == NULL)
  {
    printf("Couldn't allocate buffer\n");
    exit(0);
  }
  CircularBuffer buffer = (CircularBuffer) p;
  buffer->buffer = p + sizeof(struct s_circularBuffer);
  for (int i=0; i<size; i++)
  {
    buffer->buffer[i] = (char *)malloc((MESSAGE_LENGTH + 1) * sizeof(char));
    if (buffer->buffer[i] == NULL)
    {
      printf("Couldn't allocate buffer\n");
      exit(0);
    }
    //memset(buf[i], 'a', (MESSAGE_LENGTH + 1) * sizeof(char));
    //printf("buffer length:%lu\n",strlen(buf[i]));
  }
  CircularBufferReset(buffer);
  buffer->size = size;
  return buffer;
}

void CircularBufferFree(CircularBuffer cBuf)
{
  int i;
  CircularBufferReset(cBuf);
  for (i=0; i<cBuf->size; i++)
  {
    cBuf->buffer[i] = NULL;
  }
  cBuf->buffer = NULL;
  cBuf->size = 0;
  free(cBuf);
}

void CircularBufferReset(CircularBuffer cBuf)
{
  cBuf->head = -1;
  cBuf->tail = -1;
  cBuf->dataSize = 0;
}

void CircularBufferInsert(CircularBuffer cBuf, char *message)
{
  char *temp;
  temp = (char *)malloc(MESSAGE_LENGTH * sizeof(char));
  if (message == NULL)
  {
    return;
  }
  //check if this the first message inserted in the buffer
  if (cBuf->head == -1)
  {
    printf("This is the first message in the buffer...\n");
    cBuf->head = 0;
  }

  int check = CheckForDuplicate(cBuf, message);
  if (check)
  {
    //check for overflow of tail
    if (cBuf->tail == (cBuf->size - 1))
    {
      cBuf->tail = -1;
      cBuf->dataSize = cBuf->size;
      printf("Tail overflow\n");
    }
    cBuf->tail++;
    //printf("message: %s\n", message);
    snprintf(temp, MESSAGE_LENGTH, "%s", message);
    strcpy(cBuf->buffer[cBuf->tail], temp);
    memset(cBuf->buffer[cBuf->tail] + strlen(temp), ' ', (MESSAGE_LENGTH - strlen(temp)) * sizeof(char));
    cBuf->buffer[cBuf->tail][MESSAGE_LENGTH] = '\0';
    //printf("buffer1: %s\n", cBuf->buffer[cBuf->tail]);

    //check if buffer has reached it's maximum size
    if (cBuf->dataSize != cBuf->size)
    {
      cBuf->dataSize++;
      //printf("dataSize:%d\n", dataSize);
    }
    else
    {
      //check for overflow of head
      if (cBuf->head == (cBuf->size - 1))
      {
        cBuf->head = 0;
        printf("Head overflow\n");
      }
      else
      {
        cBuf->head++;
        //printf("head:%d\n", cBuf->head);
      }
    }
  }
  else
  {
    //printf("There was a duplicate\n");
  }
}

int CheckForDuplicate(CircularBuffer cBuf, char *message)
{
  int i;
  int check = 1;
  for (i=0; i<cBuf->dataSize; i++)
  {
    //Check if there is any duplicate of the message
    if (strcmp(message, cBuf->buffer[i]) == 0)
    {
      check = 0;
    }
  }
  return check;
}

char *CircularBufferRead(CircularBuffer cBuf, int position)
{
  char *buf = (char *)malloc(MESSAGE_LENGTH * sizeof(char));
  char *temp = (char *)malloc(MESSAGE_LENGTH * sizeof(char));
  position = (cBuf->head + position) % cBuf->size;
  snprintf(temp, MESSAGE_LENGTH, "%s", cBuf->buffer[position]);
  strcpy(buf, temp);
  memset(buf + strlen(temp), ' ', (MESSAGE_LENGTH - strlen(temp)) * sizeof(char));
  buf[MESSAGE_LENGTH] = '\0';
  return buf;
}

//Delete the oldest message of the buffer
void CircularBufferDelete(CircularBuffer cBuf)
{
  char* message = "\0";
  strcpy(cBuf->buffer[cBuf->head], message);
  if (cBuf->head == cBuf->size - 1)
  {
    cBuf->head = 0;
  }
  else
  {
    cBuf->head++;
  }
}

//print circular buffer's content
void CircularBufferPrint(CircularBuffer cBuf)
{
  printf("Messages of the buffer:\n");
  for (int i=0; i<cBuf->dataSize; i++)
  {
    printf("%s\n",cBuf->buffer[i]);
  }
  printf("head:%d\n", cBuf->head);
  printf("tail:%d\n", cBuf->tail);
  printf("\n");
}

//print circular buffer's content from selected start to end point
void CircularBufferPrintSelected(CircularBuffer cBuf, int start, int end)
{
  printf("Selected messages of the buffer from %d to %d:\n", start, end);
  for (int i=start-1; i<end; i++)
  {
    printf("%s\n",cBuf->buffer[i]);
  }
  //printf("head:%d\n", cBuf->head);
  //printf("tail:%d\n", cBuf->tail);
  printf("\n");
}

void CheckLength(CircularBuffer cBuf)
{
  int count = 0;
  for (int i=0; i<cBuf->dataSize; i++)
  {
    if (strlen(cBuf->buffer[i]) != MESSAGE_LENGTH)
    {
      printf("Position: %d\n", i);
      count++;
    }
  }
  printf("Not well sized: %d\n", count);
}

#endif
