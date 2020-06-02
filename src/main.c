
/*
  This program and the accompanying materials are
  made available under the terms of the Eclipse Public License v2.0 which accompanies
  this distribution, and is available at https://www.eclipse.org/legal/epl-v20.html

  SPDX-License-Identifier: EPL-2.0

  Copyright Contributors to the Zowe Project.
*/

#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <sys/__messag.h>

static void *handle_console(void *args) {

  printf("starting the console listener\n");

  while (true) {

    struct __cons_msg2 cons = {0};
    cons.__cm2_format = __CONSOLE_FORMAT_3;

    char mod_cmd[128] = {0};
    int cmd_type = 0;

    if (__console2(&cons, mod_cmd, &cmd_type)) {
      fprintf(stderr, "__console2(): %s\n", strerror(errno));
      pthread_exit(NULL);
    }

    if (cmd_type == _CC_modify) {
      printf("console command = \'%s\'\n", mod_cmd);
    } else if (cmd_type == _CC_stop) {
      printf("termination command sent\n");
      break;
    }

  }

  return NULL;
}

int main(int argc, char **argv) {

  pthread_t thid;

  printf("starting console thread\n");

  if (pthread_create(&thid, NULL, handle_console, "thread 1") != 0) {
    fprintf(stderr, "pthread_create(): %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (pthread_join(thid, NULL) != 0) {
    fprintf(stderr, "pthread_create(): %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}

/*
  This program and the accompanying materials are
  made available under the terms of the Eclipse Public License v2.0 which accompanies
  this distribution, and is available at https://www.eclipse.org/legal/epl-v20.html

  SPDX-License-Identifier: EPL-2.0

  Copyright Contributors to the Zowe Project.
*/
