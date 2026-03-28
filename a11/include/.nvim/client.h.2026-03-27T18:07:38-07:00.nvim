#ifndef CLIENT_H
#define CLIENT_H

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

#define MAX_MESSAGE_SIZE 1024
#define RANDOM_BYTE_COUNT 16

typedef struct {
  int fd;
  int message_count;
  FILE *log_file;
  pthread_mutex_t log_mutex;
} client_state_t;

ssize_t send_all(int fd, const void *buf, size_t len);
ssize_t recv_message(int fd, char *buf, size_t max_len);
int convert(uint8_t *buf, ssize_t buf_size, char *str, ssize_t str_size);

void *sender_thread(void *arg);
void *receiver_thread(void *arg);

#endif
