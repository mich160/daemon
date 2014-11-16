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
#include <dirent.h>
 
struct entry{
        struct dirent* ent;
        struct entry* next;
};
 
int checkType(struct dirent* ent){//0 file/ 1 directory/ -1 other
        if(ent->d_type==DT_REG){
                return 0;
        }
        if(ent->d_type==DT_DIR){
                return 1;
        }
        else{
                return -1;
        }
}
 
 
void addEntry(struct entry* head, struct dirent* newdir){
        struct entry* current = head;
        while(current->next!=NULL){
                current = current->next;
        }
        current->next = (struct entry*) malloc(sizeof(struct entry));
        current->next->ent = newdir;
        current->next->next = NULL;
}
 
void deleteEntries(struct entry* head){
        struct entry* current = head;
        struct entry* prev;
        if(current->next==NULL){
                free(current);
        }
        else{
                while(current->next!=NULL){
                prev = current;
                current=current->next;
                free(prev);
                }
                free(current);
        }
}
 
int synchronize(char* source, char* target, int recursive){
        DIR* sDir = NULL;
        DIR* tDir = NULL;
        struct dirent* temp = NULL;
        struct entry* sFirst = NULL;
        struct entry* tFirst = NULL;
        int sSize = 0;
        int tSize = 0;
        syslog(LOG_DAEMON,"Synchronizuje %s z %s.",source,target);
        sDir = opendir(source);
        tDir = opendir(target);
        while(temp = readdir(sDir)){
                if(sFirst == NULL){
                        sFirst = (struct entry*) malloc(sizeof(struct entry));
                        sFirst->ent=temp;
                        sFirst->next=NULL;
                }
                else{
                        addEntry(sFirst,temp);
                }
                sSize++;
        }
        syslog(LOG_DAEMON,"Wczytuje katalog %s.",source);
        closedir(sDir);
        while(temp = readdir(tDir)){
                if(tFirst == NULL){
                        tFirst = (struct entry*) malloc(sizeof(struct entry));
                        tFirst->ent=temp;
                        tFirst->next=NULL;
                }
                else{
                        addEntry(tFirst,temp);
                }
                tSize++;
        }
        closedir(tDir);
        syslog(LOG_DAEMON,"Wczytuje katalog %s.",target);
        if(sSize == 0 && tSize == 0){
                syslog(LOG_DAEMON,"Katalogi %s i %s sa puste.",source, target);
                sleep(1);
                return 1;
        }
        if(sSize == 1 && tSize == 1){
 
        }
        //wszystko zsynchronizowane
        deleteEntries(tFirst);
        deleteEntries(sFirst);
        return 0;
}
 
int validDirs(char* source, char* target){
        return opendir(source) != NULL && opendir(target) !=NULL;
}
 
int main(int argc, char* args[]) {
 
        /* Our process ID and Session ID */
        pid_t pid, sid;
        int sleepTime = 5;
        int recursive = 0;
        int bigFile = 1048576;
 
        /* Fork off the parent process */
        pid = fork();
        if (pid < 0) {
            exit(EXIT_FAILURE);
        }
        /* If we got a good PID, then
           we can exit the parent process. */
        if (pid > 0) {
            exit(EXIT_SUCCESS);
        }
 
        /* Change the file mode mask */
        umask(0);
 
        /* Open any logs here */
        openlog("SyncDemon",LOG_NDELAY,0);
        /* Create a new SID for the child process */
        sid = setsid();
        if (sid < 0) {
                /* Log the failure */
            syslog(LOG_DAEMON,"Blad uruchomienia!");
        }
 
        /* Change the current working directory */
        if ((chdir("/")) < 0) {
                /* Log the failure */
            syslog(LOG_DAEMON,"Blad uruchomienia!");
        }
 
        /* Close out the standard file descriptors */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
 
        /* Daemon-specific initialization goes here */
         syslog(LOG_DAEMON,"Demon uruchomiony poprawnie.");
         if(validDirs(args[1],args[2])){
                         syslog(LOG_DAEMON,"Katalogi poprawne.");
                 }
                 else{
                         syslog(LOG_DAEMON,"Katalogi niepoprawne!");
                         exit(EXIT_FAILURE);
                 }
                 int i = 0;
                 for(i = 3; i < argc; i++){
                         if(strcmp(args[i],"-R")==0){
                                 recursive = 1;
                                 syslog(LOG_DAEMON,"Wlaczono rekurencyjne sprawdzanie katalogow.");
                         }
                         if(strcmp(args[i],"-s")==0){
                                 i++;
                                 sleepTime = atoi(args[i]);
                                 if(sleepTime<=0){
                                         syslog(LOG_DAEMON,"Nieprawidlowy czas spania.");
                                         exit(EXIT_FAILURE);
                                 }
                                 syslog(LOG_DAEMON,"Czas spania ustawiony na %d sekund(y).0",sleepTime);//czas podawany w sekundach
                         }
                         if(strcmp(args[i],"-m")==0){
                                 i++;
                                 bigFile = atoi(args[i])*1024;
                                 if(bigFile<=0){
                                         syslog(LOG_DAEMON,"Nieprawidlowy rozmiar duzego pliku.");
                                         exit(EXIT_FAILURE);
                                 }
                                 syslog(LOG_DAEMON,"Rozmiar duzego pliku ustawiony na %d kB.",bigFile/1024);
                         }
 
                 }
        /* The Big Loop */
        while (1) {
           /* Do some task here ... */
           syslog(LOG_DAEMON,"Spie przez %d sekund(y)",sleepTime);
           sleep(sleepTime);
           synchronize(args[1],args[2],recursive);
        }
   exit(EXIT_SUCCESS);
}