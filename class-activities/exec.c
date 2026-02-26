#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
  pid_t pid = fork();

  if (pid < 0) {
    perror("fork failed");
    return 1;
  }

  if (pid == 0) {
    // child
    printf("CHILD before exec: PID=%d, PPID=%d\n", getpid(), getppid());
    execl("/bin/ls", "ls", "-a", "-l", "-h", NULL);
    perror("exec failed");
  } else {
    // parent
    printf("PARENT before exec: PID=%d, child PID=%d\n", getpid(), pid);
    execl("/bin/ls", "ls", "-a", NULL);
    perror("exec failed");
  }

  return 0;
}
