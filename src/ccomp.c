#include <stdio.h>
#include <unistd.h>

int main() {

  printf("hello from C test application\n");
  fflush(stdout);

  for (int i = 0; i < 1000; i++) {
    printf("C test application doing work (%d)\n", i);
    fflush(stdout);
    sleep(10);
  }

  return 0;
}
