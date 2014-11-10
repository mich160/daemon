#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>


int main(int argc, int* args[]) {

    pid_t pid, sid;

    pid = fork();
    if (pid < 0) {
            exit(EXIT_FAILURE);
    }
    if (pid > 0) {
            exit(EXIT_SUCCESS);
    }
    umask(0);
    sid = setsid();
    if (sid < 0) {
            exit(EXIT_FAILURE);
    }
    if ((chdir("/")) < 0) {
            exit(EXIT_FAILURE);
    }
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    while (1) {
        sleep(3);
    }
    exit(EXIT_SUCCESS);
}
