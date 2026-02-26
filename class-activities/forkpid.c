#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main() {

  printf("Start PID=%d, parent PID=%d\n", getpid(), getppid());

  pid_t pid = fork();

  if (pid < 0) {
    perror("fork failed");
    return 1;
  }

  if (pid == 0) {
    // Child
    printf("CHILD: PID=%d, parent PID=%d\n", getpid(), getppid());
  } else {
    // Parent
    printf("PARENT: PID=%d, child PID=%d\n", getpid(), pid);
  }

  return 0;
}
