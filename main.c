#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <time.h>

//deklaracje funkcji
int synchronize(char * source, char * target);
int copyFile(char * source, char * target);
int checkFile(char * source, char * target);

int main(int argc, char* args[]) {

    pid_t pid, sid;
	time_t currentTime;
	struct time* localTime;
	
    pid = fork();
    if (pid < 0) {
            exit(EXIT_FAILURE);
    }
    if (pid > 0) {
            exit(EXIT_SUCCESS);
    }
    umask(0);
    sid = setsid();
	currentTime = time(NULL):
	localTime = localTime(&currentTime);
	openlog("SyncDaemon",LOG_PID);
	
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
