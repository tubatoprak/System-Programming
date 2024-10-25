#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>

typedef int bool;
#define true 1
#define false 0

//#define DEBUG
#define INPUT_MAX 3
#define PID_MAX 7
#define ALARM_TIME 15
#define PROMPT_MAX 200
#define DEBUG_RESULT

/* error bilgisini ekrana basan fonksiyon */
void err_sys(const char *x){ 
	perror(x);
	exit(-1);
}
/*	usage'i ekrana basan fonksiyon */
void usage(char *x){ 
	fprintf(stderr, "usage: %s\n", x);
	exit(-1);
}
/* prompter*/
void prompt(char *x){
	fprintf(stderr, "%s", x);
}

/* global degiskenler */
int mainFifo;

char sMainFifo[10];

int resultFifo;
char sResultFifo[PID_MAX];

char logPath[BUFSIZ];

char pid[PID_MAX];

sig_atomic_t signo = 0;

typedef struct{
	char pid[PID_MAX];
	char prompt[PROMPT_MAX];
	int line;
	char filename[100];
	char write_string[100];

} Client;

typedef struct{
	int size_result;
	char *message;
} Response;

int parse_prompte(const char *input, Client *client);
void printClient(Client client) {
    printf("PID: %s\n", client.pid);
    printf("Prompt: %s\n", client.prompt);
    printf("Line: %d\n", client.line);
    printf("Filename: %s\n", client.filename);
    printf("Write String: %s\n", client.write_string);
}

void append_to_log(Client *clients_s) {
	char log_path[256];
    
    sprintf(log_path,"%s.txt",pid);

    FILE *file = fopen(log_path, "a");

    if (file == NULL) {
        perror("Failed to open log file");
        return;
    }
    fprintf(file,"\n------------------------\n");
    fprintf(file, "Submitted Request: %s\n", clients_s->prompt);
    fclose(file);
}
void parse_command(const char* command, char** file_name, int* line_number);

void split_string_by_spaces(const char* str,Client *clients_s);
void handler_signal(int sign){
	signo = 1;
}
void split_string_by_spaces_write(const char* str, Client *clients_s);
int main(int argc, char *argv[]){

	Client clientInfo;
	
	sprintf(sMainFifo, "%s",  "mainFifo");
   	
   	mainFifo = open(sMainFifo, O_RDWR);
	if(mainFifo==-1)
		printf(">biboClient tryConnect Server..\n");
	else
		printf(">biboClient Connect Server..\n");

	sprintf(pid, "%d", getpid());
	 
	char promtt[100];
	printf(">>Waiting for Que..Connection established:\n");
	
	int quit = 0;  //exite basılıp basılmadığının bayrağı
	sprintf(clientInfo.pid ,"%s",pid);
	while(quit == 0){


		sprintf(sResultFifo, "%s",pid);
		if(mkfifo(sResultFifo, 0666)>0)    //kendi fifosunu oluşturdu
		err_sys("result fifo hatasi.");

		while(1){
		printf("\n>>Enter command: ");
		fgets(promtt,100,stdin);
		int temp = parse_prompte(promtt,&clientInfo);

		if(temp == 0){
			break;
		}
		else if(temp  == 2){
			quit = 1;
			close(mainFifo);
			close(resultFifo);
			unlink(sResultFifo);
			exit(0);
		}
		else
			perror("Invalid prompt");
		}
		//printClient(clientInfo);
		
		if(write(mainFifo, &clientInfo, sizeof(clientInfo))<0)
			err_sys("maine bağlanmadı.");
		
		#ifdef DEBUG
		perror("write_mainFifo");
		#endif
		resultFifo = open(sResultFifo, O_RDWR);
		if(resultFifo==-1)
			err_sys("result fifo olusmadi");

		
		/* server'in ctrl-z yapma ihtimaline karsilik alarm olustur */
		char resultRead[1000];
		Response rp1;

    	size_t length = strlen(resultRead);
    	for (size_t i = 0; i < length; i++) {
        	resultRead[i] = '\0';  // Her karakteri sıfırlama
    	}
    
		read(resultFifo,resultRead, sizeof(resultRead));
		#ifdef DEBUG
		perror("read_resultFifo");
		#endif
		printf("\t\t%s", resultRead);
		append_to_log(&clientInfo);
		close(resultFifo);
		unlink(sResultFifo);
	}
	return 0;
}
int parse_prompte(const char *input, Client *client){
  	
  	if(strcasecmp(input,"help\n") == 0){
  		strcpy(client->prompt,"help");
  		strcpy(client->filename,"");
  		client->line = 0;
  		strcpy(client->write_string,"");
  		return 0;
  	}
  	if(strcasecmp(input,"killServer\n") == 0){
  		strcpy(client->prompt,"killServer");
  		strcpy(client->filename,"");
  		client->line = 0;
  		strcpy(client->write_string,"");
  		return 0;
  	}
  	else if(strcasecmp("list\n",input) == 0){
  		strcpy(client->prompt,"list");
  		strcpy(client->filename,"");
  		client->line = 0;
  		strcpy(client->write_string,"");
  		return 0 ;
  	}
  	else if(strcasecmp("help readF\n",input) == 0){
  		strcpy(client->prompt,"help readF");
  		strcpy(client->filename,"");
  		client->line = 0;
  		strcpy(client->write_string,"");
  		return 0;
  	}
  	else if(strcasecmp("help list\n",input) == 0){
  		strcpy(client->prompt,"help list");
  		strcpy(client->filename,"");
  		client->line = 0;
  		strcpy(client->write_string,"");
  		return 0;
  	}
  	else if(strcasecmp("help writeT\n",input) == 0){
  		strcpy(client->prompt,"help writeT");
  		strcpy(client->filename,"");
  		client->line = 0;
  		strcpy(client->write_string,"");
  		return 0;
  	}
  	else if(strcasecmp("help upload\n",input) == 0){
  		strcpy(client->prompt,"help upload");
  		strcpy(client->filename,"");
  		client->line = 0;
  		strcpy(client->write_string,"");
  		return 0;
  	}
  	else if(strcasecmp("help download\n",input) == 0){
  		strcpy(client->prompt,"help download");
  		strcpy(client->filename,"");
  		client->line = 0;
  		strcpy(client->write_string,"");
  		return 0;
  	}
  	else if(strcasecmp("help quit\n",input) == 0){
  		strcpy(client->prompt,"help quit");
  		strcpy(client->filename,"");
  		client->line = 0;
  		strcpy(client->write_string,"");
  		return 0;
  	}
  	else if(strcasecmp("help killServer\n",input) == 0){
  		strcpy(client->prompt,"help killServer");
  		strcpy(client->filename,"");
  		client->line = 0;
  		strcpy(client->write_string,"");
  		return 0;
  	}
  	else if(strcasecmp("quit\n",input) == 0){
  		strcpy(client->prompt,"quit");
  		printf("Sending write request to server log file waiting forlogfile.\nlogfile write request granted.\n>>bye");
  		strcpy(client->filename,"");
  		client->line = 0;
  		strcpy(client->write_string,"");
  		return 2;
  	}
  	else if(strncasecmp("readf",input,4) == 0){
  		sprintf(client->prompt, "%s","readF");
  		split_string_by_spaces(input,client);
  	}
  	else if(strncasecmp("writeT",input,5) == 0){
  		sprintf(client->prompt, "%s","writeT");
  		split_string_by_spaces_write(input,client);
  	}
  	else
  		return 1;
  	

}

void split_string_by_spaces(const char* str, Client *clients_s) {
    
    char* copy = strdup(str);  // Girdi stringini değiştirmemek için bir kopyasını oluşturuyoruz
    char* token = strtok(copy, " ");
    int a = 0;
    int flag = -1;
    while (token != NULL) {
    	if(a == 0)
    		sprintf(clients_s->prompt, "%s","readF");
    	if(a == 1)
    		sprintf(clients_s->filename, "%s",token);
    	if(a == 2){
    		int temp =atoi(token);
    		clients_s->line = temp;
    		flag = 0;
    	}
        token = strtok(NULL, " ");
        a++;
    }
    if(flag == -1)
    	clients_s->line = flag;
    free(copy);
}
void split_string_by_spaces_write(const char* str, Client *clients_s) {
    
    char* copy = strdup(str);  // Girdi stringini değiştirmemek için bir kopyasını oluşturuyoruz
    char* token = strtok(copy, " ");
    int a =0;
    while (token != NULL) {
    	if(a == 0)
    		sprintf(clients_s->prompt, "%s","writeT");
    	if(a == 1)
    		sprintf(clients_s->filename, "%s",token);
    	if(a == 2){
    		sprintf(clients_s->write_string, "%s","token");

    	}
        token = strtok(NULL, " ");
        a++;
    }
    clients_s->line = -1;
    free(copy);
}