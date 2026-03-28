#include "client.h"

ssize_t send_all(int fd, const void *buf, size_t len) {
  size_t total = 0;
  const char *p = (const char *)buf;

  while (total < len) {
    ssize_t n = send(fd, p + total, len - total, 0);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }
    if (n == 0) {
      return -1;
    }
    total += (size_t)n;
  }

  return (ssize_t)total;
}

ssize_t recv_message(int fd, char *buf, size_t max_len) {
  size_t total = 0;

  while (total < max_len) {
    ssize_t n = recv(fd, buf + total, 1, 0);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }
    if (n == 0) {
      return 0;
    }
    total += (size_t)n;
    if (buf[total - 1] == '\n') {
      return (ssize_t)total;
    }
  }

  return -1;
}

int convert(uint8_t *buf, ssize_t buf_size, char *str, ssize_t str_size) {
  if (buf == NULL || str == NULL || buf_size <= 0 ||
      str_size < (buf_size * 2 + 1)) {
    return -1;
  }

  for (int i = 0; i < buf_size; i++) {
    sprintf(str + i * 2, "%02X", buf[i]);
  }
  str[buf_size * 2] = '\0';

  return 0;
}

void *sender_thread(void *arg) {
  client_state_t *state = (client_state_t *)arg;

  for (int i = 0; i < state->message_count; i++) {
    uint8_t random_buf[RANDOM_BYTE_COUNT];
    char hex_str[RANDOM_BYTE_COUNT * 2 + 1];
    char out[MAX_MESSAGE_SIZE + 1];
    size_t hex_len;
    size_t out_len;

    if (getentropy(random_buf, sizeof(random_buf)) != 0) {
      return NULL;
    }

    if (convert(random_buf, sizeof(random_buf), hex_str, sizeof(hex_str)) !=
        0) {
      return NULL;
    }

    hex_len = strlen(hex_str);
    out[0] = 0;
    memcpy(out + 1, hex_str, hex_len);
    out[1 + hex_len] = '\n';
    out_len = 1 + hex_len + 1;

    if (send_all(state->fd, out, out_len) < 0) {
      return NULL;
    }
  }

  {
    char end_msg[2];
    end_msg[0] = 1;
    end_msg[1] = '\n';
    send_all(state->fd, end_msg, 2);
  }

  return NULL;
}

void *receiver_thread(void *arg) {
  client_state_t *state = (client_state_t *)arg;
  char buf[MAX_MESSAGE_SIZE + 1];

  while (1) {
    ssize_t n = recv_message(state->fd, buf, MAX_MESSAGE_SIZE);
    if (n <= 0) {
      return NULL;
    }

    if ((uint8_t)buf[0] == 1) {
      return NULL;
    }

    if ((uint8_t)buf[0] == 0) {
      uint32_t ip_raw;
      uint16_t port_raw;
      struct in_addr addr;
      char ip_str[INET_ADDRSTRLEN];
      unsigned int port;
      size_t msg_len;
      char msg[MAX_MESSAGE_SIZE + 1];

      if ((size_t)n < 1 + sizeof(uint32_t) + sizeof(uint16_t) + 1) {
        return NULL;
      }

      memcpy(&ip_raw, buf + 1, sizeof(uint32_t));
      memcpy(&port_raw, buf + 1 + sizeof(uint32_t), sizeof(uint16_t));

      addr.s_addr = ip_raw;
      if (inet_ntop(AF_INET, &addr, ip_str, sizeof(ip_str)) == NULL) {
        return NULL;
      }

      port = (unsigned int)ntohs(port_raw);

      msg_len = (size_t)n - 1 - sizeof(uint32_t) - sizeof(uint16_t);
      if (msg_len == 0) {
        return NULL;
      }

      memcpy(msg, buf + 1 + sizeof(uint32_t) + sizeof(uint16_t), msg_len);
      msg[msg_len] = '\0';

      pthread_mutex_lock(&state->log_mutex);
      fprintf(stdout, "%-15s%-10u%s", ip_str, port, msg);
      fprintf(state->log_file, "%-15s%-10u%s", ip_str, port, msg);
      fflush(stdout);
      fflush(state->log_file);
      pthread_mutex_unlock(&state->log_mutex);
    }
  }
}

int main(int argc, char *argv[]) {
  int fd;
  int port;
  int message_count;
  struct sockaddr_in server_addr;
  FILE *log_file;
  client_state_t state;
  pthread_t sender;
  pthread_t receiver;

  if (argc != 5) {
    return EXIT_FAILURE;
  }

  port = atoi(argv[2]);
  message_count = atoi(argv[3]);

  if (message_count < 0) {
    return EXIT_FAILURE;
  }

  log_file = fopen(argv[4], "w");
  if (log_file == NULL) {
    return EXIT_FAILURE;
  }

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    fclose(log_file);
    return EXIT_FAILURE;
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons((uint16_t)port);

  if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) != 1) {
    close(fd);
    fclose(log_file);
    return EXIT_FAILURE;
  }

  if (connect(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    close(fd);
    fclose(log_file);
    return EXIT_FAILURE;
  }

  state.fd = fd;
  state.message_count = message_count;
  state.log_file = log_file;
  pthread_mutex_init(&state.log_mutex, NULL);

  if (pthread_create(&receiver, NULL, receiver_thread, &state) != 0) {
    pthread_mutex_destroy(&state.log_mutex);
    close(fd);
    fclose(log_file);
    return EXIT_FAILURE;
  }

  if (pthread_create(&sender, NULL, sender_thread, &state) != 0) {
    pthread_cancel(receiver);
    pthread_join(receiver, NULL);
    pthread_mutex_destroy(&state.log_mutex);
    close(fd);
    fclose(log_file);
    return EXIT_FAILURE;
  }

  pthread_join(sender, NULL);
  pthread_join(receiver, NULL);

  pthread_mutex_destroy(&state.log_mutex);
  close(fd);
  fclose(log_file);

  return EXIT_SUCCESS;
}
