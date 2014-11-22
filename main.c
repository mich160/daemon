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
int sig = 0;

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
    char fileBuffer[fileBuffSize];
    char* bigBuffer;
    struct stat temp;
    makeTargetDir(buffer1,sourceDir,fileName);
    makeTargetDir(buffer2,targetDir,fileName);
    syslog(LOG_DAEMON, "Kopiuje plik %s",buffer1);
    stat(buffer1,&temp);
    if((fileIn=open(buffer1,O_RDONLY))<0){
        syslog(LOG_DAEMON, "Nie udalo sie otworzyc pliku %s",buffer1);
        return;
    }
    if((fileOut=open(buffer2,O_CREAT | O_WRONLY | O_TRUNC, temp.st_mode))<0){
        syslog(LOG_DAEMON, "Nie udalo sie otworzyc pliku %s",buffer2);
        return;
    }
    if(fileSize>=bigFile){
        syslog(LOG_DAEMON,"Duzy plik");
        if((bigBuffer=mmap(0,fileSize,PROT_READ,MAP_SHARED,fileIn,0))<0){
            syslog(LOG_DAEMON,"Nie udalo sie odczytac pliku %s",buffer1);
            close(fileIn);
            close(fileOut);
            return;
        }
        if(write(fileOut,bigBuffer,fileSize)<0){
            syslog(LOG_DAEMON,"Nie udalo sie zapisac pliku %s",buffer1);
            close(fileIn);
            close(fileOut);
            return;
        }
        munmap(bigBuffer,fileSize);
    }
    else{
        while(size = read(fileIn,&fileBuffer,fileBuffSize)){
            if(size<0){
                syslog(LOG_DAEMON,"Nie udalo sie odczytac pliku %s",buffer1);
                close(fileIn);
                close(fileOut);
                return;
            }
            if(write(fileOut,fileBuffer,size)<0){
                syslog(LOG_DAEMON,"Nie udalo sie zapisac pliku %s",buffer2);
                close(fileIn);
                close(fileOut);
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
    if(checkType(srcEnt) == 0 && checkType(tgtEnt) == 0 && strcmp(srcEnt->d_name,tgtEnt->d_name)==0 && strcmp(srcEnt->d_name,".")==1){
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
    sig = 1;
}


int synchronize(char* source, char* target, int recursive, int bigFile){
	DIR* sDir = NULL;
	DIR* tDir = NULL;
	struct dirent* temp = NULL;
	struct stat tempStat;
	struct stat tempTStat;
	int sSize = 0;
	int tSize = 0;
	char buffer1[200];
	char buffer2[200];
	syslog(LOG_DAEMON,"Synchronizuje %s z %s.",source,target);
	sDir = opendir(source);
	tDir = opendir(target);
	if(sDir!=NULL){
		while(temp = readdir(sDir)){
        if(strcmp(temp->d_name,".")!=0 && strcmp(temp->d_name,"..")!=0){
            syslog(LOG_DAEMON,"Sprawdzam %s",temp->d_name);
            makeTargetDir(buffer1,source,temp->d_name);
            makeTargetDir(buffer2,target,temp->d_name);
            stat(buffer1,&tempStat);
            int type = checkType(temp);
            if(type==0){//file
                if(stat(buffer2,&tempTStat)==0 && S_ISREG(tempTStat.st_mode)==1){
                    if(tempTStat.st_mtime<tempStat.st_mtime){
                        deleteFile(target,temp->d_name);
                        copyFile(source,temp->d_name,target,tempStat.st_size,bigFile);
                        syslog(LOG_DAEMON,"Zastapiono starszy plik %s.", temp->d_name);
                    }
                }
                else{
                    copyFile(source,temp->d_name,target,tempStat.st_size,bigFile);//OGARNIJ TO
                    syslog(LOG_DAEMON,"Utworzono plik %s.", temp->d_name);
                }
            }
            if(type==1){//directory
                if(stat(buffer2,&tempTStat)== 0){
                    if(S_ISDIR(tempTStat.st_mode)==1 && recursive == 1){
                            syslog(LOG_DAEMON,"Synchronizuje katalog %s.",temp->d_name);
                            synchronize(buffer1,buffer2,recursive,bigFile);
                    }
                }
                else{
                    createDir(target,temp->d_name);
                    syslog(LOG_DAEMON,"Utworzono katalog %s.", temp->d_name);
                    if(recursive == 1){
                            synchronize(buffer1,buffer2,recursive,bigFile);
                    }
                }
            }
        }
    }
	}
    closedir(sDir);
    while(temp = readdir(tDir)){
        if(strcmp(temp->d_name,".")!=0 && strcmp(temp->d_name,"..")!=0){
            makeTargetDir(buffer1,source,temp->d_name);
            makeTargetDir(buffer2,target,temp->d_name);
            stat(buffer2,&tempTStat);
            int type = checkType(temp);
            if(type==0){
                if(stat(buffer1,&tempStat)!=0 || S_ISREG(tempStat.st_mode)!=1){
                    deleteFile(target,temp->d_name);
                    syslog(LOG_DAEMON,"Usunieto plik %s.", temp->d_name);
                }
            }
            if(type==1){
                if(stat(buffer1,&tempStat)!=0 || S_ISDIR(tempStat.st_mode)!=1){
                    synchronize(buffer1,buffer2,1,bigFile);
                    deleteDir(target,temp->d_name);
                    syslog(LOG_DAEMON,"Usunieto katalog %s.", temp->d_name);
                }
            }
        }
    }
	//wszystko zsynchronizowane
	closedir(tDir);
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
           if(sig == 1){
                sig = 0;
                syslog(LOG_DAEMON,"Demon obudzony przez sygnal SIGUSR1.");
           }
           else{
                syslog(LOG_DAEMON, "Demon obudzony normalnie.");
           }
           synchronize(args[1],args[2],recursive,bigFile);
        }
   exit(EXIT_SUCCESS);
}

