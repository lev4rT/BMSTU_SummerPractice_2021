#include "daemonization.h"

int lockfile(int fd) {
    struct flock fl;
    // write lock
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    // dynamic eof
    fl.l_len = 0;
    return fcntl(fd, F_SETLK, &fl);
}

void daemonize(const char* cmd) {
    struct rlimit rl;

    umask(0);

    if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
        printf("%s: can't get file limit", cmd);
    }

    pid_t pid;
    if ((pid = fork()) < 0) {
        printf("%s: can't fork", cmd);
    }
    else if (pid != 0) {
        exit(0);
    }

    setsid();

    if (chdir("/") < 0) {
        printf("%s: can't change directory to /", cmd);
    }

    if (rl.rlim_max == RLIM_INFINITY) {
        rl.rlim_max = 1024;
    }

    for (int i = 0; i < rl.rlim_max; i++) {
        close(i);
    }

    int fd0 = open("/dev/null", O_RDWR);
    int fd1 = dup(0);
    int fd2 = dup(0);

    openlog(cmd, LOG_CONS, LOG_DAEMON);

    if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
        syslog(LOG_ERR, "unexpected file descriptors %d %d %d", fd0, fd1, fd2);
        exit(1);
    }
}

int already_running(void) {
    char buf[16];

    int fd = open(LOCKFILE, O_RDWR | O_CREAT, LOCKMODE);
    if (fd < 0) {
        syslog(LOG_ERR, "can't open %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }

    if (lockfile(fd) < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            close(fd);
            syslog(LOG_INFO, "can't lock %s: %s (already locked)", LOCKFILE, strerror(errno));
            return 1;
        }

        syslog(LOG_ERR, "can't lock %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }

    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf) + 1);

    return 0;
}