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
#include <signal.h>
#include <sys/mman.h>

const unsigned int fileBuffSize = 1024;

struct entry{
	struct dirent* ent;
	struct entry* next;
	int synced;
};

void clearBuffer(char * buffer){
    buffer[0] = '\0';
}

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

void makeTargetDir(char* buffer, char* dir, char* objName){
    clearBuffer(buffer);
    strcpy(buffer,dir);
    strcat(buffer,"/");
    strcat(buffer,objName);
}

void createDir(char* target, char* name){
    char buffer[200];
    makeTargetDir(buffer,target,name);
    syslog(LOG_DAEMON,"Tworze katalog %s", buffer);
    if(mkdir(buffer,S_IRWXU)==-1){
        syslog(LOG_DAEMON,"Wystapil blad.");
    }
}
void deleteFile(char* target, char* name){
    char buffer[200];
    makeTargetDir(buffer,target,name);
    syslog(LOG_DAEMON,"Tworze katalog %s", buffer);
    if(unlink(buffer)==-1){
        syslog(LOG_DAEMON,"Wystapil blad.");
    }
}
void deleteDir(char* target, char* name){
    char buffer[200];
    makeTargetDir(buffer,target,name);
    syslog(LOG_DAEMON,"Usuwam katalog %s", buffer);
    if(rmdir(buffer)==-1){
        syslog(LOG_DAEMON,"Wystapil blad.");
    }

}
void copyFile(char* sourceDir,char* fileName, char* targetDir,int fileSize ,int bigFile){
    int fileIn, fileOut;
    int size;
    char buffer1[200], buffer2[200];
    char* fileBuffer[fileBuffSize];
    char* bigBuffer;
    makeTargetDir(buffer1,sourceDir,fileName);
    makeTargetDir(buffer2,targetDir,fileName);
    syslog(LOG_DAEMON, "Kopiuje plik %s",buffer1);
    if(fileIn=open(buffer1,O_RDONLY)<0){
        syslog(LOG_DAEMON, "Nie udalo sie otworzyc pliku %s",buffer1);
        return;
    }
    if(fileOut=open(buffer2,O_CREAT | O_WRONLY | O_TRUNC, S_IEXEC)<0){
        syslog(LOG_DAEMON, "Nie udalo sie otworzyc pliku %s",buffer2);
        return;
    }
    if(fileSize/1024>=bigFile){
        if(bigBuffer=mmap(0,fileSize,PROT_READ,MAP_SHARED,fileOut,0)<0){
            syslog(LOG_DAEMON,"Nie udalo sie odczytac pliku %s",buffer1);
            return;
        }
        if(write(fileOut,bigBuffer,fileSize)<0){
            syslog(LOG_DAEMON,"Nie udalo sie zapisac pliku %s",buffer1);
            return;
        }
        munmap(bigBuffer,fileSize);
    }
    else{
        while(size = read(fileIn,fileBuffer,fileBuffSize)){
            if(size<0){
                syslog(LOG_DAEMON,"Nie udalo sie odczytac pliku %s",buffer1);
                return;
            }
            if(write(fileOut,fileBuffer,size)<0){
                syslog(LOG_DAEMON,"Nie udalo sie zapisac pliku %s",buffer2);
                return;
            }
        }
    }
    close(fileIn);
    close(fileOut);
}

int checkEntries(char* source, char* target, struct dirent* srcEnt, struct dirent* tgtEnt){//0 object differ/ 1 files need to be synced/ 2 all is good with files  3 all is good with dirs
    struct stat* sourceEntry;
    struct stat* targetEntry;
    char buffer1[200], buffer2[200];
    if(checkType(srcEnt) == 0 && checkType(tgtEnt) == 0 && strcmp(srcEnt->d_name,tgtEnt->d_name)==0){
        strcpy(buffer1,source);
        strcat(buffer1,"/");
        strcat(buffer1,srcEnt->d_name);
        strcpy(buffer2,target);
        strcat(buffer2,"/");
        strcat(buffer2,tgtEnt->d_name);
        stat(buffer1,sourceEntry);
        stat(buffer2,targetEntry);
        if(sourceEntry->st_mtim.tv_sec<targetEntry->st_mtim.tv_sec){
            return 1;
        }
        else{
            return 2;
        }
    }
    if(checkType(srcEnt) == 1 && checkType(tgtEnt) == 1 && strcmp(srcEnt->d_name,tgtEnt->d_name)==0){
        return 3;
    }
    else {
        return 0;
    }
}

void signalhandler(){
    syslog(LOG_DAEMON,"Sygnal SISGUR1.");
}

void addEntry(struct entry* head, struct dirent* newdir){
	struct entry* current = head;
	while(current->next!=NULL){
		current = current->next;
	}
	current->next = (struct entry*) malloc(sizeof(struct entry));
	current->next->ent = newdir;
	current->next->next = NULL;
	current->next->synced = 0;
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

int synchronize(char* source, char* target, int recursive, int bigFile){
	DIR* sDir = NULL;
	DIR* tDir = NULL;
	struct dirent* temp = NULL;
	struct entry* sFirst = NULL;
	struct entry* tFirst = NULL;
	struct stat* tempStat = NULL;
	int sSize = 0;
	int tSize = 0;
	char buffer1[200];
	char buffer2[200];
	syslog(LOG_DAEMON,"Synchronizuje %s z %s.",source,target);
	sDir = opendir(source);
	tDir = opendir(target);
	while(temp = readdir(sDir)){
		if(sFirst == NULL){
			sFirst = (struct entry*) malloc(sizeof(struct entry));
			sFirst->ent=temp;
			sFirst->next=NULL;
			sFirst->synced=0;
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
			tFirst->synced=0;
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

    struct entry* currentSourceEntry = sFirst;
    struct entry* currentTargetEntry = tFirst;
    do{
        do{
                int status = checkEntries(source,target,currentSourceEntry->ent,currentTargetEntry->ent);
                if(status == 1){
                    makeTargetDir(buffer1,source,sFirst->ent->d_name);
                    stat(buffer1,tempStat);
                    deleteFile(target,currentTargetEntry->ent->d_name);
                    copyFile(source,currentSourceEntry->ent->d_name,target,tempStat->st_size,bigFile);
                    currentSourceEntry->synced = 1;
                    currentTargetEntry->synced = 1;
                }
                if(status == 2){
                    currentSourceEntry->synced = 1;
                    currentTargetEntry->synced = 1;
                }
                if(status == 3){
                    makeTargetDir(buffer1,source,currentSourceEntry->ent->d_name);
                    makeTargetDir(buffer2,target,currentTargetEntry->ent->d_name);
                    synchronize(buffer1,buffer2,recursive,bigFile);
                }
            currentTargetEntry=currentTargetEntry->next;
        }while(currentTargetEntry!=NULL);
        currentSourceEntry=currentSourceEntry->next;
    }while(currentSourceEntry!=NULL);
    currentSourceEntry = sFirst;
    do{
        if(currentSourceEntry->synced==0){
            if(checkType(currentSourceEntry->ent)==0){
                makeTargetDir(buffer1,source,sFirst->ent->d_name);
                stat(buffer1,tempStat);
                copyFile(source,currentSourceEntry->ent->d_name,target,tempStat->st_size,bigFile);
                currentSourceEntry->synced = 1;
            }
            if(checkType(currentSourceEntry->ent)==1){
                createDir(target,currentSourceEntry->ent->d_name);
                makeTargetDir(buffer1,source,currentSourceEntry->ent->d_name);
                makeTargetDir(buffer2,target,currentSourceEntry->ent->d_name);
                synchronize(buffer1,buffer2,recursive,bigFile);
                currentSourceEntry->synced = 1;
            }
        }
        currentSourceEntry=currentSourceEntry->next;
    }while(currentSourceEntry!=NULL);
    currentTargetEntry = tFirst;
    do{
        if(currentTargetEntry->synced == 0){
            if(checkType(currentTargetEntry->ent)==0){
                deleteFile(target,currentTargetEntry->ent->d_name);
                currentTargetEntry->synced = 1;
            }
            if(checkType(currentTargetEntry->ent)==1){
                deleteDir(target,currentTargetEntry->ent->d_name);
                currentTargetEntry->synced = 1;
            }
        }
    }while(currentTargetEntry!=NULL);
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
        struct sigaction action;

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
		 action.sa_handler=signalhandler;
		 sigfillset(&action.sa_mask);
		 action.sa_flags=0;
        /* The Big Loop */
        while (1) {
           /* Do some task here ... */
           syslog(LOG_DAEMON,"Spie przez %d sekund(y)",sleepTime);
           sleep(sleepTime);
           synchronize(args[1],args[2],recursive,bigFile);
        }
   exit(EXIT_SUCCESS);
}
