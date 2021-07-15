#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <time.h>
#include <syslog.h>
#include <unistd.h>

#define TRUE 1
#define LOCKFILE "/var/run/printers_supervisor_daemon.pid"
#define LOCKMODE S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
#define TIMEOUT 60 * 30


int lockfile(int fd);
void daemonize(const char* cmd);
int already_running(void);