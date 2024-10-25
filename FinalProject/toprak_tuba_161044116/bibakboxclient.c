#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h> 
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libgen.h>
#include <string.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>

#define logf ".log"

typedef struct{
    mode_t mode;
    char filename [200];
    char content [4096];
    int readS;
    int flag;   
    time_t lastModification;
} fileInform;

int flag = 0; //for log file

void handler(int signo);          
void sentDirectory(int,char*);  
void sentFiles(int,char*);    
void err_sys(const char *x) {  
    perror(x);
    exit(-1);
}

char logpath[50];
int soketclientFd;                       
int pathI;                      


void writeLog(char * logFile, char *msg){
    int log;
    char * logInfo = (char*)malloc(512*sizeof(char));
    sprintf(logInfo, "%s\t\n", msg); 
    log = open(logFile, O_WRONLY | O_APPEND, 0666);

    write(log, logInfo, strlen(logInfo));
    close(log);
    free(logInfo);
}

int main(int argc, char **argv){
    
    struct sockaddr_in serverAddress;   
    socklen_t serverLen;                
    char  clientPath[512],clientBase[512];
    char ip[250];

    if(argc < 3 || argc > 4){
        err_sys("Usage: .bibakboxclient [directory] [port][IP]\n");
    }
    int portnumber = atoi(argv[2]);
    if(argc == 3) strcpy(ip,"127.0.0.0");
    else strcpy(ip,argv[3]);
    
    DIR *pDir = opendir (argv[1]);
    if(pDir != NULL){    
        closedir(pDir);
    }
    else{
        err_sys("Client directory does not exist.\n");
    }
    //*************************************************************signal handler
    signal(SIGINT,handler);
    signal(SIGTERM,handler);
    signal(SIGTSTP,handler);
    //*************************************************************** directory set
    strcpy(clientBase,argv[1]);     
    strcpy(clientPath,dirname(argv[1]));
    if(strcmp(clientPath,".") == 0)
        strcpy(clientPath,"");
    else
        strcat(clientPath,"/");

    strcat(clientPath,basename(clientBase));
    pathI=strlen(argv[1]);
    ++pathI;
    //*************************************************************************///*log file 
   
    strcpy(logpath,clientBase);
    strcat(logpath,"/");
    strcat(logpath,clientBase);
    strcat(logpath,logf);
    int log = open(logpath, O_WRONLY | O_CREAT | O_APPEND, 0666);
        
    //*********************************************************************server communication

    soketclientFd = socket(AF_INET,SOCK_STREAM,0);   
    if(soketclientFd == -1){ 
        err_sys("Failed socket\n");
    }

    serverLen = sizeof(serverAddress);
    memset(&serverAddress,0,serverLen);

    serverAddress.sin_family = AF_INET;                   
    serverAddress.sin_addr.s_addr = inet_addr(ip);   
    serverAddress.sin_port = htons(portnumber);           

    if (connect(soketclientFd, (struct sockaddr*)&serverAddress,serverLen) == -1) {  
        close(soketclientFd);
        err_sys("Failed Connect\n");
    } 

      char msg[10];
    struct stat fileStat;
    lstat(clientPath,&fileStat);
    char tempName[512];
    strcpy(tempName,clientPath);
    fileInform fileE;
    memset(&fileE, 0, sizeof fileE); 
    strcpy(fileE.filename,basename(tempName));
    fileE.mode = fileStat.st_mode;
    fileE.flag = 0;

    write(soketclientFd,&fileE,sizeof(fileInform));
    read(soketclientFd, msg, sizeof(msg));    

    while(1){
        signal(SIGINT,handler);
        signal(SIGTERM,handler);
        sentDirectory(soketclientFd,clientPath);
         double dif = 0.0;
        struct timeval start, end;
        gettimeofday(&start, NULL);
        while(dif <= 1000){
            gettimeofday(&end, NULL);
            dif = (double) (end.tv_usec - start.tv_usec) / 1000 + (double) (end.tv_sec - start.tv_sec)*1000;
        } 
    }
    
    close(soketclientFd);
    return 0;
}

void sentDirectory(int fd ,char* name){
    char msg[10];
    sentFiles(fd,name);
    fileInform fileE;
    memset(&fileE, 0, sizeof fileE);
    fileE.flag = 2;
    write(fd,&fileE,sizeof(fileInform));
    read(fd, msg, sizeof(msg));   

}

void sentFiles(int fd , char* param){
    
    struct dirent *pDirent;
    DIR *pDir;
    struct stat fileStat;

    char name [512], msg[250]; 
    int size, rSize; 
    

    pDir = opendir (param);
    if (pDir != NULL){
        while ((pDirent = readdir(pDir)) != NULL){
            if(0!=strcmp(pDirent->d_name,".") && 0!=strcmp(pDirent->d_name,"..")){
                int uzunluk = strlen(pDirent->d_name);
                if (uzunluk >= 4 && strcmp(pDirent->d_name + uzunluk - 4, ".log") == 0) 
                    continue;
                strcpy(name,param);
                strcat(name,"/");
                strcat(name,pDirent->d_name);
                

                lstat(name,&fileStat);
                if((S_ISDIR(fileStat.st_mode))){
                    fileInform fileE;
                    memset(msg,0,sizeof(msg));
                    memset(&fileE, 0, sizeof fileE);
                    memset(fileE.filename, '\0', sizeof(fileE.filename));
                    memcpy(fileE.filename, &name[pathI], strlen(name)-pathI);
                    fileE.mode = fileStat.st_mode;
                    if(flag == 0){
                        char buffer[250];
                        strcat(buffer,"   sent....");
                        strcat(buffer,fileE.filename);
                        writeLog(logpath,buffer);
                    }
                      
                    flag = 1;
                    write(fd,&fileE,sizeof(fileInform));
                    read(fd, msg, sizeof(msg));  
                        if(strcmp(msg,"okey") != 0)
                            writeLog(logpath,msg);
                    strcpy(msg,""); 
                    sentFiles(fd,name);
                }

                else {
                    int rfd;
                    if(S_ISREG(fileStat.st_mode)) rfd = open(name, O_RDONLY);
                    else if(S_ISFIFO(fileStat.st_mode)) rfd = open(name, O_RDONLY | O_NONBLOCK);
                    else continue;
                    if(rfd == -1) continue;

                    fileInform fileE;
                    memset(&fileE, 0, sizeof fileE);
                    size = (int)fileStat.st_size;
                    memset(fileE.filename, '\0', sizeof(fileE.filename));
                    memcpy(fileE.filename, &name[pathI], strlen(name)-pathI);
                    fileE.mode = fileStat.st_mode;
                    fileE.flag = 0;
                    fileE.lastModification = fileStat.st_mtime;

                    do{
                        rSize = read(rfd,fileE.content,sizeof(fileE.content));
                        if(rSize < 0)break;
                        size -= sizeof(fileE.content);
                        if(size <= 0)
                            fileE.flag = 1;     

                        fileE.readS = rSize;
                        if(flag == 0){
                            char buffer[250];
                            strcat(buffer,"   sent....");
                            strcat(buffer,fileE.filename);
                            writeLog(logpath,buffer);
                        }
                        flag = 1;
                        
                        write(fd,&fileE,sizeof(fileInform));
                        memset(msg,0,sizeof(msg));
                        read(fd, msg, sizeof(msg));     
                        strcpy(fileE.content,"");  

                        if(strcmp(msg,"okey") != 0)
                            writeLog(logpath,msg); 
                    }while(rSize > 0);
                    close(rfd);
                }
            }
        }
    }

    closedir(pDir);
}

void handler(int signo){
    if(signo == SIGINT)
        err_sys("SIGINT handled\n");
    else if(signo == SIGTERM)
        err_sys("SIGTERM handled\n");
    else if (signo == SIGTSTP)
        err_sys("SIGTSTP handled");
}