
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

static void *handle_console(void *args) {

  printf("test\n");

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
