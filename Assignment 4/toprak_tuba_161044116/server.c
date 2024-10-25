#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdatomic.h>

typedef int bool;
#define true 1
#define false 0

//#define DEBUG
#define INPUT_MAX 3
#define PID_MAX 7
#define PROMPT_MAX 200
#define DEBUG_RESULT

int numberOfClients = 0;
int counter = 0;
char clientPids[1000][PID_MAX];

#define FIFO_NAME "serverFifo"
int mainFifo;
char sMainFifo[10];

int resultFifo;
char sResultFifo[PID_MAX];
int poolSize;
pthread_mutex_t mutex;

FILE *logFile;
char* dirname;
int flag = 0;  //////////ekrana basma aynıysa

typedef struct{
	char pid[PID_MAX];
	char prompt[PROMPT_MAX];
	int line;
	char filename[100];
	char write_string[100];
} Client;

typedef struct{
	Client client;
	int index;
}Task;

pthread_t  thread_pool[20];
int thread_availability[20];


void writeStringToEnd(const char* file_path, const char* content);
void do_request(Client *client_s, char *dirname);
void writeStringToLine(const char* fileName, int line, const char* str);
char* readFile_full(const char* fileName);
char* read_file(const char* file_path, int line_number);
void* handle_request(void* arg);


void err_sys(const char *x){ 
	perror(x);
	exit(-1);
}

void append_to_log(Client *clients_s) {

	char log_path[256];
    snprintf(log_path, sizeof(log_path), "%s/log.txt", dirname);

    FILE *file = fopen(log_path, "a");

    if (file == NULL) {
        perror("Failed to open log file");
        return;
    }
    fprintf(file,"\n------------------------\n");
     fprintf(file, "Client PID: %s\n", clients_s->pid);
    fprintf(file, "Request: %s\n", clients_s->prompt);
    fclose(file);
}
void quit_killserver(){
	close(mainFifo);
	close(resultFifo);
	unlink(sMainFifo);
	unlink(sResultFifo);
	for (int i = 0; i < poolSize; ++i)
	{
		pthread_cancel(thread_pool[i]);
	}
	exit(EXIT_SUCCESS);
}

void ignore_sigint(int sig) {
	printf("....Pressed ctrl-c\n");
    quit_killserver();
}

int main(int argc, char *argv[]){

	struct sigaction sa;
    sa.sa_handler = ignore_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
	int maxNumberOfClients;
	bool status = false;

	if(argc != 4){
		printf("Usage: %s <dirname> <max.#ofClients <poolSize>\n", argv[0]);
		exit(1);
	}
	maxNumberOfClients = atoi(argv[2]);
	poolSize = atoi(argv[3]);

	//strcpy(dirname,argv[1]);
	dirname = argv[1];
	struct stat st;
	int result = stat(dirname,&st);
	if(result == 0 && S_ISDIR(st.st_mode))
		;
	else
		mkdir(dirname, 0700);


	if(maxNumberOfClients<0){
		err_sys("the number of clients was entered incorrectly.");
	}
	if(poolSize < 0){
		err_sys("poolSize was entered incorrectly.");
	}
		

	// dirnameyi kontrol et
	//fifoya başla 
	printf(">>Server Started Pid: %d\n",getpid());

	sprintf(sMainFifo,"%s","mainFifo");
	
	if(mkfifo(sMainFifo,0666)<0)             
		err_sys("Server already activate");
		
	
	mainFifo = open(sMainFifo, O_RDWR);          
	if(mainFifo == -1)
		err_sys("Could not create server fifo.\n");
	
	printf(">>Waiting for Clients..\n");
	
	setbuf(stdout,NULL);

	/**********************************************************thread pool*/
	
	for (int i = 0; i < poolSize; ++i)
	{
		thread_availability[i] = 0;   //0 ise müsait 1 ise değil
	}
	
	while(1){
		
		Client client_s;
		int i= 0,j = 0, count= 0;
		if(read(mainFifo,&client_s,sizeof(client_s)) == -1){      //clientin istegini oku
			err_sys("Failed to read from fifo\n");
		}
		++counter;
		flag = 0;
		/*if(numberOfClients > maxNumberOfClients){
			printf("connection request pid: %s ... Que FULL\n",client_s.pid);
			char temp[] = "tryconnect";
			write(resultFifo,temp,strlen(temp));
			continue;
		}*/
		for (int i = 0; i < counter+1; ++i){
			if(strcmp(client_s.pid,clientPids[i]) == 0)
				flag = 1;	
		}
		sprintf(clientPids[counter],"%s",client_s.pid);
		numberOfClients++;
		//kontrol et AYNI MI
		for(i=1; i<counter+1; ++i){
			for(j=i; j<counter+1; ++j){
				if(strcmp(clientPids[i], clientPids[j])==0 && strcmp(clientPids[i], "free")!=0 && strcmp(clientPids[j], "free")!=0){
					++count;
					if(count>1){
						strcpy(clientPids[i], "free");
						--numberOfClients;
						status=true;
					}
				}
			}
			count=0;
		}

		append_to_log(&client_s);
		#ifdef DEBUG
		perror("read_mainFifo");			
		#endif
		
		int available_thread = -1;
		for (int i = 0; i < poolSize; ++i) // uygun threadin indexini bul
		{
			if(thread_availability[i] == 0){
				available_thread = i;
				break;
			}
		}


		if(available_thread != -1){
			thread_availability[available_thread] = 1;
			if(flag == 0)
				printf(">>Cliend PID:%s connected as client%d\n",client_s.pid,numberOfClients);
			Task task;
			task.client = client_s;
			task.index = available_thread;
			pthread_create(&thread_pool[available_thread],NULL,handle_request,&task);
		}
		else{
			printf("All threads works..\n");
		}
		
	}
	return 0;
}

void* handle_request(void* arg){
	
	Task task = *(Task*)arg;
	Client client = task.client;
	do_request(&client,dirname);
	close(resultFifo);
	thread_availability[task.index] = 0;

}
char* read_file(const char* file_path, int line_number) {
    
    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    char* result = NULL;
    if (line_number != -1) {
        char buffer[256];
        int current_line = 1;
        while (fgets(buffer, sizeof(buffer), file) != NULL) {
            if (current_line == line_number) {
                result = strdup(buffer);
                break;
            }
            current_line++;
        }
    }
    else{           //Dosyanın boyutunu ölç
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Bellek tahsis et ve dosyanın tamamını oku
    char* buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL) {
        perror("Bellek tahsis hatası\n");
        fclose(file);
        return NULL;
    }

    size_t read_size = fread(buffer, 1, file_size, file);
    if (read_size != file_size) {
        perror("Okuma hatası\n");
        fclose(file);
        free(buffer);
        return NULL;
    }

    buffer[file_size] = '\0';  // Null karakteri ekle

    fclose(file);
    return buffer;
	}

    fclose(file);
    return result;
}
void do_request(Client *client_s, char *dirname){
	
	int result = 8;
	resultFifo = open(client_s->pid,O_RDWR);

	if(resultFifo == -1){
		close(resultFifo);
		unlink(client_s->pid);
		exit(1);
	}else{
		
		if(strcasecmp("help",client_s->prompt) == 0){
			char helps[] ="Available comments are:\n  help, list,readF,writeT, upload, download, quit, killServer\n";
			/*response1.size_result = strlen(helps);
	 		response1.message = malloc(strlen(helps) + 1); // Bellek ayrılması
    		strcpy(response1.message, helps); // Değerin kopyalanması
			write(resultFifo,&response1,sizeof(response1));*/
			write(resultFifo,helps,strlen(helps));
		}
		else if(strcasecmp("list",client_s->prompt) == 0){
			
			DIR * dir;
			dir = opendir(dirname); // Klasörü aç
			struct dirent *entry;
		    if (dir == NULL) {
		        perror("Klasör açılamadı");
		        exit(1);
		    }

		    char fileList[1024]; // Dosya listesini depolamak için bir tampon oluşturun
		    fileList[0] = '\0'; // Dosya listesini boş bir dizeyle başlatın

		    while ((entry = readdir(dir)) != NULL) {
		        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
		            continue; // Geçerli ve üst klasörleri atla
		        }

		        strcat(fileList, entry->d_name); // Dosya adını fileList'e ekle
		        strcat(fileList, "\n\t\t"); // Her dosya adından sonra yeni bir satır karakteri ekle
		    }

    		closedir(dir); // Klasörü kapat
			write(resultFifo,fileList,strlen(fileList));	
		}
		else if(strcasecmp("help readF",client_s->prompt) == 0){
			char helpre[] =" readF <file> <line #>\n\t\tdisplay the #th line of the <file>, returns with an error if <file> does not exists\n";
			write(resultFifo,helpre,strlen(helpre));
		}
		else if(strcasecmp("help writeT",client_s->prompt) == 0){
			char helpwri[] =" writeT <file> <line #> <string>\n\t\trequest to write the content of “string” to the #th line the <file>,if the line # is not given writes to the end of file.If the file does not exists in Servers directory creates and edits the file at the same time.\n";
			write(resultFifo,helpwri,strlen(helpwri));
		}
		else if(strcasecmp("help upload",client_s->prompt) == 0){
			char helpup[] ="upload <file>\n\t\tuploads the file from the current working directory of client to the Servers director. (beware of the cases no file in clients current working directory and name on Servers side)\n";
			write(resultFifo,helpup,strlen(helpup));
		}	
		else if(strcasecmp("help download",client_s->prompt) == 0){
			char helpdo[] ="download <file>\n\t\trequest to receive <file> from Servers directory to client side\n";
			write(resultFifo,helpdo,strlen(helpdo));
		}
		else if(strcasecmp("help quit",client_s->prompt) == 0){
			char helpq[] ="quit\n\t\tSend write request to server side log file  and quits\n";
			write(resultFifo,helpq,strlen(helpq));
		}
		else if(strcasecmp("help killServer",client_s->prompt) == 0){
			char helpk[] ="killServer <file>\n\t\tSends a kill request to the Server\n";
			write(resultFifo,helpk,strlen(helpk));
		}
		else if(strcasecmp("killServer",client_s->prompt) == 0){
			printf(">>kill signal from %s terminating..\n>>bye\n",client_s->pid);
			quit_killserver();	
			exit(0);		
		}
		else if (strcasecmp("help list",client_s->prompt) == 0){
			char helpl[] ="help list\n\t\tSends a request to display the list of files in Servers directory\n";
			write(resultFifo,helpl,strlen(helpl));
		}
		else if (strcasecmp("readF",client_s->prompt) == 0){
			//sem_wait(semaphore);
			pthread_mutex_lock(&mutex);
			char* file_contents;
			if(client_s->line == -1)
				file_contents = readFile_full(client_s->filename);
			else{
				file_contents = read_file(client_s->filename,client_s->line);
			}
			
		    if (file_contents != NULL) {
		        write(resultFifo,file_contents,strlen(file_contents));
		        free(file_contents);
		    }else{
		    	write(resultFifo,"File empty.\n",strlen("File empty.\n"));
		    	free(file_contents);
		    }
		    pthread_mutex_unlock(&mutex);
		}
		else if(strcasecmp("quit",client_s->prompt) == 0){
			for (int i = 0; i < counter+1; ++i)
			{
				if(strcmp(client_s->pid,clientPids[i]) == 0){
					strcpy(clientPids[i], "free");
				}	
			}
			numberOfClients--;
			printf("%s disconnected...\n",client_s->pid);
		}
		else if (strcasecmp("writeT",client_s->prompt) == 0){
			pthread_mutex_lock(&mutex);
			if(client_s->line == -1)
				writeStringToEnd(client_s->filename,client_s->write_string);
			else 
				writeStringToLine(client_s->filename,client_s->line,client_s->write_string);

			write(resultFifo,"Write request has been completed.\n",strlen("Write request has been completed.\n"));
			pthread_mutex_unlock(&mutex);
		}
		else 
			write(resultFifo,&result,sizeof(int));
		close(resultFifo);
	}
}
void writeStringToLine(const char* fileName, int line, const char* str) {
    FILE* file = fopen(fileName, "r+");
    if (file == NULL) {
        printf("Dosya açma hatası.\n");
        return;
    }

    // Belirtilen satıra konumlan
    int currentLine = 1;
    while (currentLine < line) {
        if (fgetc(file) == '\n') {
            currentLine++;
        }
    }

    // Verilen string'i satıra yaz
    fprintf(file, "%s\n", str);

    // Dosyayı kapat
    fclose(file);
}
void writeStringToEnd(const char* file_path, const char* content) {
    FILE* file = fopen(file_path, "a");
    if (file == NULL) {
        perror("Dosya oluşturma hatası:");
        return;
    }
    fseek(file, 0, SEEK_END);

    // İçeriği dosyanın sonuna yaz
    fprintf(file, "%s", content);

    // Dosyayı kapat
    fclose(file);
}
char* readFile_full(const char* fileName) {
  FILE* file = fopen(fileName, "r");
    if (file == NULL) {
        printf("Dosya açma hatası.\n");
        return NULL;
    }

    // Bellekte yeterli yer ayırmak için dosyanın boyutunu hesapla
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Bellekte dosya içeriği için yer ayır ve içeriği oku
    char* fileContent = (char*)malloc(fileSize + 1);
    if (fileContent == NULL) {
        fclose(file);
        printf("Bellek hatası.\n");
        return NULL;
    }
    fread(fileContent, 1, fileSize, file);
    fileContent[fileSize] = '\0'; // Sonlandırma karakteri ekle

    fclose(file);
    return fileContent;
}