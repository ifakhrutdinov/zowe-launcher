#include <stdio.h>
#include <unistd.h>

int main() {

  printf("hello from C test application\n");
  fflush(STDOUT_FILENO);

  for (int i = 0; i < 1000; i++) {
    printf("C test application doing work (%d)\n", i);
    fflush(STDOUT_FILENO);
    sleep(10);
  }

  return 0;
}
