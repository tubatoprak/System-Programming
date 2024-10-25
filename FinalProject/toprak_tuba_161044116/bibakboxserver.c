#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libgen.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <errno.h>
#include <signal.h>



#define mlog "/m.log" 

typedef struct{
	mode_t mode;
	char filename [200];
    char content [4096];
    int readS;
    int flag;	
    time_t lastModification;
}fileInform;

typedef struct {
	char fname[500];
} directories;

int socket_establish(unsigned short portnumber);
void timeInfo(char *buf);		
void writeLog(char * logFile, char *type, char * status);	
char *toString(char * logFile, char *type, char * status);
int isInclude(char *name, directories *list, int count);	
void * serverThreadRun(void*);		
void signal_handler(int);			
void err_sys(const char *x) {  
    perror(x);
    exit(-1);
}
	
int clientSize = 0;
int *client_id;		

int socketServerfd;				
char *serverPath;			
directories * activeDir;

int threadPoolSize;
pthread_t *serverThreads;  						
pthread_mutex_t activeMutex = PTHREAD_MUTEX_INITIALIZER;
	

int main(int argc, char **argv){

	if(argc != 4)
		err_sys("Usage:./bibakboxserver [Directory] [threadPoolSize] [portnumber]");
	
	threadPoolSize = atoi(argv[2]);
	int portnumber = atoi(argv[3]);
	DIR *pDir = opendir (argv[1]);

	if(threadPoolSize< 0 || pDir == NULL)
		err_sys("Usage:./bibakboxserver [Directory] [threadPoolSize] [portnumber]");
	closedir(pDir);

	signal(SIGINT,signal_handler);
	signal(SIGTERM,signal_handler);
	signal(SIGPIPE,signal_handler);

	serverPath = (char*)malloc(512*sizeof(char)); 	
	char * serverBase = (char*)malloc(512*sizeof(char));

	strcpy(serverBase,argv[1]);		

	strcpy(serverPath,dirname(argv[1]));
	if(strcmp(serverPath,".") == 0)
		strcpy(serverPath,"");
	else
		strcat(serverPath,"/");

	strcat(serverPath,basename(serverBase));
	free(serverBase);
	//****************************************************************************socket set
	int soket = socket_establish(portnumber);
	printf("*************server listening**************\n");
	if (soket == -1) return -1;

	client_id = (int*)malloc(threadPoolSize*sizeof(int)); //id of clients
	memset(client_id,-1,sizeof(client_id));
	
	activeDir = (directories*)malloc(4096*sizeof(directories));

	/***************************************************************************thread pool*/
	int *thread_index = (int*)malloc(threadPoolSize*sizeof(int));
	serverThreads = (pthread_t *)malloc(threadPoolSize * sizeof(pthread_t)); 			

	for (int i = 0; i < threadPoolSize; ++i){
		thread_index[i] = i;
		int err = pthread_create(&serverThreads[i], NULL, serverThreadRun, (void*)&thread_index[i]);
		if(err)err_sys("Thread failed..");
	}

	for (int i = 0; i < threadPoolSize; ++i)
    	pthread_join (serverThreads[i], NULL);	

    free(thread_index);
    free(client_id);
    free(serverThreads);
    free(serverPath);
    free(activeDir);
    close(socketServerfd);	
	return 0;
}

void writeLog(char * logFile, char *type, char * status){
	
	char buf[50];			
	timeInfo(buf);

	char * logInfo = (char*)malloc(512*sizeof(char));
	sprintf(logInfo, "%s\t %s\t %s \n", type, status ,buf);	
	int log = open(logFile, O_WRONLY | O_APPEND, 0666);

	write(log, logInfo, strlen(logInfo));
	close(log);
	free(logInfo);
}
char * toString(char * logFile, char *type, char * status){
	int log;
	char buf[50];			
	timeInfo(buf);

	char * logInfo = (char*)malloc(512*sizeof(char));
	sprintf(logInfo, "%s\t %s\t %s \n", type, status ,buf);
	return logInfo;
}

int isInclude(char *name, directories *list, int count){
	for (int i = 0; i < count; ++i)
		if(strcmp(list[i].fname,name)==0)
			return 1;
	
	return 0;
}

void * serverThreadRun(void* p){
	char clientlog [256];
	struct sockaddr_in clientAddress;	
	socklen_t clientLen;				
	int client_idIndex = *(int*) p;		
	int wfd = -1 , log = -1;			
	struct stat fileStat;
	char logFileName[512];
	char clientDirName[512];
	char buf[50];
	char msg[10];;
	int directoriesCount=0;
	char * logInfo;
	fileInform each;	

	memset(&clientAddress,0,sizeof(struct sockaddr_in));	
	clientLen = sizeof(clientAddress);

	while(1){

		//*************************************************************************accept connect

		client_id[client_idIndex] = accept(socketServerfd, (struct sockaddr*)&clientAddress, &clientLen);
		if (client_id[client_idIndex] == -1) { 
			err_sys("Failed Accept..");          
		}

		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);	

	    read(client_id[client_idIndex], &each, sizeof(fileInform));		
    
    	strcpy(activeDir[clientSize++].fname,each.filename);	

    	char str[clientLen];
		inet_ntop( AF_INET, &clientAddress, str, clientLen );
		printf("Client%d connected with ip: %s\n",clientSize,str ); 

    	bzero(msg,sizeof(msg));
    	strcpy(msg,"okey");
    	write(client_id[client_idIndex], msg, sizeof(msg));		
    	//******************************************************************log file set
    	strcpy(clientDirName,serverPath);		
    	strcat(clientDirName,"/");

    	strcat(clientDirName,each.filename);		
    	char logmain[50];
    	strcpy(logmain,serverPath);
    	strcat(logmain,mlog);
    	int logm = open(logmain, O_WRONLY | O_CREAT | O_APPEND, 0666);
    	if(logm == -1)
    		perror("main log File");

    	timeInfo(buf);		//time
  		logInfo = (char*)malloc(512*sizeof(char));
    	sprintf(logInfo, "Client%d with %s connected. \t %s\n",clientSize,each.filename,buf );	
    	//write(log, logInfo, strlen(logInfo));
    	write(logm, logInfo, strlen(logInfo));

    	free(logInfo);   

    	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);	

    	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);	
	    directories * directoriesList = (directories*)malloc(1024*sizeof(directories)); 	
	    
	    while(read(client_id[client_idIndex], &each, sizeof(fileInform))>0){	

	    	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	    	if(each.flag == 2){	
	    		directoriesCount=0;
	    		bzero(msg,sizeof(msg));
	    		strcpy(msg,"okey");
	    		write(client_id[client_idIndex], msg, sizeof(msg));
	    		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	    		continue;
	    	}

	    	char *name = (char *) malloc(512*sizeof(char));
	    	
	    	strcpy(name,serverPath);
	    	strcat(name,"/");
	    	strcat(name,each.filename);

	    	if(S_ISDIR(each.mode)){
	    		if(!isInclude(name,directoriesList,directoriesCount))
	    			strcpy(directoriesList[directoriesCount++].fname,name);
	    		
	    		if(mkdir(name,0700) != -1){
	    			memset(clientlog,0,sizeof(clientlog));
	    			strcpy(clientlog,toString(logFileName,basename(name),"CREATED  "));
	    			write(client_id[client_idIndex], clientlog, sizeof(clientlog));	
	    			writeLog(logmain,basename(name),"CREATED  ");
	    		}
	    	}
	    	else{
	    		if(lstat(name,&fileStat) != -1){		
					if(wfd == -1){		
						if(!isInclude(name,directoriesList,directoriesCount))
							strcpy(directoriesList[directoriesCount++].fname,name);

						if(fileStat.st_mtime < each.lastModification){	
							wfd = open(name, O_WRONLY | O_TRUNC, each.mode);
							memset(clientlog,0,sizeof(clientlog));
							strcpy(clientlog,toString(logFileName,basename(name),"UPDATED"));
	    					write(client_id[client_idIndex], clientlog, sizeof(clientlog));
		    				writeLog(logmain,basename(name),"UPDATED");
						}
						else {			
							free(name);
							bzero(msg,sizeof(msg));
							strcpy(msg,"okey");
							write(client_id[client_idIndex], msg, sizeof(msg));		
							pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
							continue;
						}
					}
					write(wfd,each.content,each.readS);
    				
	    		}
	    		else{
	    			if(wfd == -1){		
	    				if(!isInclude(name,directoriesList,directoriesCount))
	    					strcpy(directoriesList[directoriesCount++].fname,name);

	    				wfd = open(name, O_WRONLY | O_CREAT | O_TRUNC, each.mode);
	    				memset(clientlog,0,sizeof(clientlog));
	    				strcpy(clientlog,toString(logFileName,basename(name),"CREATED  "));
	    				write(client_id[client_idIndex], clientlog, sizeof(clientlog));
	    				writeLog(logmain,basename(name),"CREATED  ");

	    			}
	    			write(wfd,each.content,each.readS);
	    		}
	    	}

	    	if(each.flag == 1){	
	    		if(!S_ISDIR(each.mode)){
		    		close(wfd);		
		    		wfd = -1;		
		    	}
	    	}
	    	free(name);
	    	bzero(msg,sizeof(msg));
	    	strcpy(msg,"okey");
	    	write(client_id[client_idIndex], msg, sizeof(msg));		
	    	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);	
	    }

	    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);	

	    timeInfo(buf);
	    logInfo = (char*)malloc(512*sizeof(char));
	    sprintf(logInfo, "Client%d with %s disconnected. \t %s\n",clientSize, basename(dirname(logFileName)),buf );
	    write(log, logInfo, strlen(logInfo));
	    write(logm, logInfo, strlen(logInfo));
	    free(logInfo);
	    free(directoriesList);

	    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);	

	}

	return NULL;
}


void timeInfo(char *buf){
	strcpy(buf,"");
	time_t now = time(0);
	struct tm now_t = *localtime(&now);
	strftime (buf, 50, "%x - %I:%M:%S %p", &now_t);
}

void signal_handler(int signo){
	   if(signo == SIGINT)
        printf("SIGINT handled\n");
    else if(signo == SIGTERM)
        printf("SIGTERM handled\n");
    else if (signo == SIGTSTP)
        printf("SIGTSTP handled");
	exit(0);
}

int socket_establish(unsigned short portnumber){
	
	struct sockaddr_in serverAddress;	
	socklen_t serverLen;	

	memset(&serverAddress,0,sizeof(struct sockaddr_in));	
	serverAddress.sin_family = AF_INET;		
	serverAddress.sin_port = htons(portnumber);				

	serverLen = sizeof(serverAddress);

	if((socketServerfd = socket(AF_INET,SOCK_STREAM,0)) == -1)err_sys("Fail Socket");


	if(bind(socketServerfd,(struct sockaddr*)&serverAddress,serverLen) == -1){
		close(socketServerfd);
		err_sys("Fail Bind");
	}

	if(listen(socketServerfd,0) == -1) err_sys("Fail Lİsten");
	return socketServerfd;
}