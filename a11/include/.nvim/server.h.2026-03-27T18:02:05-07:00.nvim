#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_CLIENTS 100
#define MAX_MESSAGE_SIZE 1024

typedef struct {
  int fd;
  struct sockaddr_in addr;
  int active;
  int finished;
} client_t;

typedef struct message_node {
  char data[MAX_MESSAGE_SIZE + 1];
  size_t len;
  struct message_node *next;
} message_node_t;

extern client_t clients[MAX_CLIENTS];
extern int expected_clients;
extern int finished_clients;
extern pthread_mutex_t clients_mutex;
extern message_node_t *queue_head;
extern message_node_t *queue_tail;
extern pthread_mutex_t queue_mutex;
extern pthread_cond_t queue_cond;

void *client_thread(void *arg);
void *broadcast_thread(void *arg);

ssize_t send_all(int fd, const void *buf, size_t len);
ssize_t recv_message(int fd, char *buf, size_t max_len);

void enqueue_message(const char *data, size_t len);
message_node_t *dequeue_message(void);

#endif
