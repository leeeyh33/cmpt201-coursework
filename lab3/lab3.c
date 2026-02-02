#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LEN 5

static char *input_history[MAX_LEN];
static int history_count = 0;

char *get_input(void);
void add_to_history(char *input);
void remove_oldest_record(void);
void print_history(void);

int main(void) {
  while (1) {
    char *input = get_input();
    add_to_history(input);
    if (strcmp(input, "print") == 0) {
      print_history();
    }
  }

  while (history_count > 0) {
    remove_oldest_record();
  }

  return 0;
}

char *get_input(void) {
  char *buffer = NULL;
  size_t bufsize = 0;

  printf("Enter input: ");

  ssize_t len = getline(&buffer, &bufsize, stdin);
  if (len == -1) {
    free(buffer);
    exit(0);
  }

  if (len > 0 && buffer[len - 1] == '\n') {
    buffer[len - 1] = '\0';
  }

  return buffer;
}

void add_to_history(char *input) {
  if (history_count >= MAX_LEN) {
    remove_oldest_record();
  }
  input_history[history_count] = input;
  history_count++;
}

void remove_oldest_record(void) {
  if (history_count <= 0)
    return;

  free(input_history[0]);

  for (int i = 1; i < history_count; i++) {
    input_history[i - 1] = input_history[i];
  }

  history_count--;
}

void print_history(void) {
  for (int i = 0; i < history_count; i++) {
    printf("%s\n", input_history[i]);
  }
}
