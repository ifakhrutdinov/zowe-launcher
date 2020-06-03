
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
#include <limits.h>
#include <ctype.h>
#include <signal.h>

#include <sys/__messag.h>
#include <sys/wait.h>
#include <unistd.h>
#include <spawn.h>
#include <fcntl.h>

//  //ZLAUNCH  JOB  USER=&SYSUID,NOTIFY=&SYSUID
//  //ZLAUNCH  EXEC PGM=ZLAUNCH,REGION=0M,
//  // PARM='ENVAR(_CEE_ENVFILE=DD:STDENV)/'
//  //STEPLIB  DD   DSNAME=&SYSUID..DEV.LOADLIB,DISP=SHR
//  //PARMLIB  DD   DSNAME=&SYSUID..DEV.PARMLIB,DISP=SHR
//  //SYSIN    DD   DUMMY
//  //SYSPRINT DD   SYSOUT=*
//  //SYSERR   DD   SYSOUT=*
//  //CEEOPTS  DD  *
//  RPTOPTS(ON)
//  POSIX(ON)
//  /*
//  //STDENV   DD  *
//  WORKDIR=/u/user/zowe-launcher/bin
//  /*
//

typedef struct Component {

  char name[32];
  char bin[_POSIX_PATH_MAX + 1];
  pid_t pid;
  int output;

#define COMP_STATUS_NA      0xfffffff
#define COMP_STATUS_RUNNING 0x0000001
#define COMP_STATUS_STOPPED 0x0000002
  int status;

  pthread_t comm_thid;

} Component;

struct {

  pthread_t console_thid;

#define MAX_CHILD_COUNT 128

  Component children[MAX_CHILD_COUNT];
  size_t count;

  char workdir[_POSIX_PATH_MAX + 1];

} context = {0};

static int init_context(void) {

  const char *workdir = getenv("WORKDIR");
  if (workdir == NULL) {
    fprintf(stderr, "WORKDIR env variable not found\n");
    return -1;
  }

  const char *dir_start = workdir;
  const char *dir_end = dir_start + strlen(workdir) - 1;
  while (*dir_end == ' ' && dir_end != dir_start) {
    dir_end--;
  }

  size_t dir_len = dir_end - dir_start + 1;
  if (dir_len > sizeof(context.workdir) - 1) {
    fprintf(stderr, "WORKDIR env too large\n");
    return -1;
  }

  memset(context.workdir, 0, sizeof(context.workdir));
  memcpy(context.workdir, dir_start, dir_len);

  if (chdir(context.workdir)) {
    fprintf(stderr, "working directory not changed: %s\n", strerror(errno));
    return -1;
  }

  printf("work directory is \'%s\'\n", context.workdir);

  return 0;
}

static int init_component(const char *cfg_line, Component *result) {

  const char *comp_start = cfg_line;
  const char *comp_end = strchr(cfg_line, '=');

  if (comp_end == NULL) {
    fprintf(stderr, "bad config (equal sign not found): \'%s\'\n", cfg_line);
    return -1;
  }

  size_t comp_len = comp_end - comp_start;
  if (comp_len > sizeof(result->name) - 1) {
    fprintf(stderr, "bad config (component too long): \'%s\'\n", cfg_line);
    return -1;
  }

  const char *bin_start = comp_end + 1;
  const char *bin_end = bin_start + 1;
  while (*bin_end != ' ' && *bin_end != '\0' && *bin_end != '\n') {
    bin_end++;
  }

  size_t bin_len = bin_end - bin_start;
  if (bin_len > sizeof(result->bin) - 1) {
    fprintf(stderr, "bad config (bin too long): \'%s\'\n", cfg_line);
    return -1;
  }

  memset(result->name, 0, sizeof(result->name));
  memset(result->bin, 0, sizeof(result->bin));
  result->pid = -1;

  memcpy(result->name, comp_start, comp_len);
  memcpy(result->bin, bin_start, bin_len);

  printf("new component init'd \'%s\', \'%s\'\n", result->name, result->bin);

  return 0;
}

static int load_cfg(void) {

  FILE *cfg;

  if ((cfg = fopen("components.conf", "r")) == NULL) {
    fprintf(stderr, "components config file not open: %s\n", strerror(errno));
    return -1;
  }

  char *line;
  char buff[1024];
  ssize_t read;

  while ((line = fgets(buff, sizeof(buff), cfg)) != NULL) {
    printf("handling line \'%s\'\n", line);
    Component comp = {0};
    if (!init_component(line, &comp)) {
      if (context.count != MAX_CHILD_COUNT) {
        context.children[context.count++] = comp;
      } else {
        fprintf(stderr, "max component number reached, ignoring the rest\n");
        break;
      }
    }
  }

  printf("reading config finished - %s\n", strerror(errno));

  fclose(cfg);
  cfg = NULL;

  return 0;
}

static void *handle_comp_comm(void *args) {

  printf("starting a component communication thread\n");

  Component *comp = args;

  comp->status = COMP_STATUS_RUNNING;

  while (true) {

    int comp_status = 0;
    int wait_rc = waitpid(comp->pid, &comp_status, WNOHANG);
    if (wait_rc == comp->pid) {
      printf("component \'%s\' with PID = %d terminated, status = %d\n",
             comp->name, comp->pid, comp_status);
      comp->pid = -1;
      break;
    } else if (wait_rc == -1) {
      fprintf(stderr, "waitpid failed for \'%s\'(%d): %s\n",
              comp->name, comp->pid, strerror(errno));
      break;
    }

    char msg[1024];
    int retries_left = 10;
    while (retries_left > 0) {

      int msg_len = read(comp->output, msg, sizeof(msg));
      if (msg_len > 0) {
        msg[msg_len] = '\0';
        printf("%s", msg);
        retries_left = 10;
      } else if (msg_len == -1 && errno == EAGAIN) {
        sleep(1);
        retries_left--;
      } else {
        fprintf(stderr, "cannot read output from comp \'%s\'(%d) failed: %s\n",
                comp->name, comp->pid, strerror(errno));
      }

    }

  }

  return NULL;
}

static int start_component(Component *comp) {

  if (comp->pid != -1) {
    fprintf(stderr, "cannot start component \'%s\' - running\n", comp->name);
    return -1;
  }

  comp->status = COMP_STATUS_NA;

  printf("about to start component \'%s\'\n", comp->name);

  size_t workdir_len = strlen(context.workdir);
  size_t bin_len = strlen(comp->bin);

  if (workdir_len + bin_len > _POSIX_PATH_MAX) {
    fprintf(stderr, "bin name \'%s\' too long\n", comp->bin);
    return -1;
  }

  char full_path[_POSIX_PATH_MAX + 1 + 1] = {0};
  strcpy(full_path, context.workdir);
  strcat(full_path, "/");
  strcat(full_path, comp->bin);

  printf("about to start component \'%s\' at \'%s\'\n",
         comp->name, full_path);

  struct inheritance inherit = {0};

  FILE *script = NULL;
  int c_stdout[2];
  if (pipe(c_stdout)) {
    fprintf(stderr, "pipe(): %s\n", strerror(errno));
    return -1;
  }

  if (fcntl(c_stdout[0], F_SETFL, O_NONBLOCK)) {
    fprintf(stderr, "fcntl(): %s\n", strerror(errno));
    return -1;
  }

  int fd_count = 3;
  int fd_map[3];

  if (strcmp(&comp->bin[bin_len - 3], ".sh") == 0) {
    script = fopen(full_path, "r");
    if (script == NULL) {
      fprintf(stderr, "script not open: %s\n", strerror(errno));
      return -1;
    }
    fd_map[0] = dup(fileno(script));
    fclose(script);
  } else {
    fd_map[0] = dup(STDIN_FILENO);
  }
  fd_map[1] = dup(c_stdout[1]);
  fd_map[2] = dup(c_stdout[1]);

  printf("fd_map[0]=%d, fd_map[1]=%d, fd_map[2]=%d\n",
         fd_map[0], fd_map[1], fd_map[2]);

  comp->pid = spawn(full_path, fd_count, fd_map, &inherit, NULL, NULL);
  if (comp->pid == -1) {
    fprintf(stderr, "spawn(): %s\n", strerror(errno));
    return -1;
  }

  comp->output = c_stdout[0];
  close(c_stdout[1]);

  printf("a new process started with PID = %d\n", comp->pid);

  if (pthread_create(&comp->comm_thid, NULL, handle_comp_comm, comp) != 0) {
    fprintf(stderr, "pthread_create(): %s\n", strerror(errno));
    return -1;
  }

  return 0;
}

static int start_components(void) {

  for (size_t i = 0; i < context.count; i++) {
    if (start_component(&context.children[i])) {
      return -1;
    }
  }

  return 0;
}

static int stop_component(Component *comp) {

  if (comp->pid == -1) {
    return 0;
  }

  printf("about to stop component \'%s\' with PID = %d\n",
         comp->name, comp->pid);

  if (!kill(comp->pid, SIGINT)) {

    if (pthread_join(comp->comm_thid, NULL) != 0) {
      fprintf(stderr, "pthread_create() failed for \'%s\': %s\n",
              comp->name, strerror(errno));
      return -1;
    }

  } else {
    fprintf(stderr, "kill(): %s\n", strerror(errno));
    return -1;
  }

  comp->pid = -1;

  return 0;
}

static int stop_components(void) {

  for (size_t i = 0; i < context.count; i++) {
    stop_component(&context.children[i]);
  }

  return 0;
}

static Component *find_comp(const char *name) {

  for (size_t i = 0; i < context.count; i++) {
    if (!strcmp(name, context.children[i].name)) {
      return &context.children[i];
    }
  }

  return NULL;
}

#define CMD_START "START"
#define CMD_STOP  "STOP"

static int handle_start(const char *comp_name) {

  Component *comp = find_comp(comp_name);
  if (comp == NULL) {
    fprintf(stderr, "component \'%s\' not found\n", comp_name);
    return -1;
  }

  start_component(comp);

  return 0;
}

static int handle_stop(const char *comp_name) {

  Component *comp = find_comp(comp_name);
  if (comp == NULL) {
    fprintf(stderr, "component \'%s\' not found\n", comp_name);
    return -1;
  }

  stop_component(comp);

  return 0;
}

static char *get_cmd_val(const char *cmd, char *buff, size_t buff_len) {

  const char *lb = strchr(cmd, '(');
  if (lb == NULL) {
    return NULL;
  }

  const char *rb = strchr(cmd, ')');
  if (rb == NULL) {
    return NULL;
  }

  if (lb > rb) {
    return NULL;
  }

  size_t val_len = rb - lb - 1;
  if (val_len >= buff_len) {
    return NULL;
  }

  memcpy(buff, lb + 1, val_len);
  buff[val_len] = '\0';

  return buff;
}

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

      char cmd_val[128] = {0};

      if (strstr(mod_cmd, CMD_START) == mod_cmd) {
        char *val = get_cmd_val(mod_cmd, cmd_val, sizeof(cmd_val));
        if (val != NULL) {
          handle_start(val);
        } else {
          fprintf(stderr, "bad value, command ignored\n");
        }
      }

      if (strstr(mod_cmd, CMD_STOP) == mod_cmd) {
        char *val = get_cmd_val(mod_cmd, cmd_val, sizeof(cmd_val));
        if (val != NULL) {
          handle_stop(val);
        } else {
          fprintf(stderr, "bad value, command ignored\n");
        }
      }


    } else if (cmd_type == _CC_stop) {
      printf("termination command sent\n");
      stop_components();
      break;
    }

  }

  return NULL;
}

static int start_console_tread(void) {

  if (pthread_create(&context.console_thid, NULL, handle_console, NULL) != 0) {
    fprintf(stderr, "pthread_create(): %s\n", strerror(errno));
    return -1;
  }

  return 0;
}

static int stop_console_thread(void) {

  if (pthread_join(context.console_thid, NULL) != 0) {
    fprintf(stderr, "pthread_create(): %s\n", strerror(errno));
    return -1;
  }

  return 0;
}

int main(int argc, char **argv) {

  printf("starting console thread\n");

  if (init_context()) {
    exit(EXIT_FAILURE);
  }

  if (load_cfg()) {
    exit(EXIT_FAILURE);
  }

  start_components();

  if (start_console_tread()) {
    exit(EXIT_FAILURE);
  }

  if (stop_console_thread()) {
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
