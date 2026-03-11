#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

Queue *queue_create(void)
{
  Queue *queue = malloc(sizeof(Queue));
  if (queue == NULL){
    printf("Error: malloc failed\n");
    return NULL;
  }
  queue->size = 0;
  queue->capacity = QUEUE_INITIAL_CAPACITY;
  queue->data = malloc(sizeof(double) * queue->capacity);
  return queue;
}

void push(Queue *queue, double element)
{
  if(queue == NULL){
    printf("Error: queue is NULL\n");
    return;
  }
  if (queue->size == queue->capacity)
  {
    int capacity = queue->capacity * 2;

    queue->data = realloc(queue->data, sizeof(double) * capacity);
    if (queue->data == NULL)
    {
      queue_free(queue);
      printf("Error: realloc failed\n");
      return;
    }
    queue->capacity = capacity;
  }

  int insert_index = queue->size % queue->capacity;
  queue->data[insert_index] = element;
  queue->size++;
}

double back(Queue *queue)
{
  if(queue == NULL){
    printf("Error: queue is NULL\n");
    return 0.0;
  }
  if (queue->size == 0)
  {
    printf("Error: queue is empty\n");
    return 0.0;
  }
  return queue->data[queue->size - 1];
}

void queue_free(Queue *queue)
{
  if(queue == NULL){
    printf("Error: queue is NULL\n");
    return;
  }
  free(queue->data);
  free(queue);
}

