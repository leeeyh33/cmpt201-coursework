#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
  char *line = NULL;
  size_t cap = 0;

  while (1) {
    printf("Enter programs to run.\n");
    printf("> ");
    fflush(stdout);

    if (getline(&line, &cap, stdin) == -1) {
      break;
    }

    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') {
      line[len - 1] = '\0';
    }

    pid_t cpid = fork();

    if (cpid < 0) {
      perror("fork failed");
      continue;
    } else if (cpid > 0) {
      int status = 0;
      if (waitpid(cpid, &status, 0) == -1) {
        perror("waitpid failed");
        exit(EXIT_FAILURE);
      }
    } else {
      if (execl(line, line, (char *)NULL) == -1) {
        printf("Exec failure\n");
        free(line);
        exit(EXIT_FAILURE);
      }
    }
  }

  free(line);
  return 0;
}
