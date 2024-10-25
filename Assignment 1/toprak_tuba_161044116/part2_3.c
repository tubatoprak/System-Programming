/***************************************************************************************************************  
*
*
* 
* 	
*	Tuba Toprak
*	161044116 - HW1                            Part2 -- Part3 
*	System Programming               
*
*
*
*
*
****************************************************************************************************************/
#include <unistd.h> 
#include <stdio.h> 
#include <errno.h>                            
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>  


int _dup(int oldfd);
int _dup2(int oldfd, int newfd );

int main()
{
	int newfd = 4 ;

	int dups = open("dup_and_dup2.txt", O_RDWR| O_APPEND | O_CREAT ,S_IRWXU);

	/****************************Dup*****************************************************************************/
	
	int a = _dup(dups);
	
	size_t len = strlen("This will be output to the  file named dup_and_dup2.txt with dup() function.\n");
	
	if (a != -1){
		int wr2 = write(a,"This will be output to the  file named dup_and_dup2.txt with dup() function.\n",len);
		if(wr2 == -1){
			perror("Failed to write file. ");
			return 1;
		}
	}
	else{
		perror("Failed dup");
		return 1;
	}
		
	close(a);

	/****************************Dup2****************************************************************************/

	int b = _dup2(dups,newfd);

	len = strlen("This will be output to the  file named dup_and_dup2.txt with dup() function.\n");
	
	if (b != -1){
		int wr2 = write(b,"This will be output to the  file named dup_and_dup2.txt with dup2() function.\n",len);
		if(wr2 == -1){
			perror("Failed write dup2 ");
			return 1;
		}
	}
	else
		perror("Failed dup2");	

	close(b);
	close(dups);

	/**************************Part3 - Read**********************************************************************/
	int fd = open("dup_and_dup2.txt",O_RDONLY);
	
	int part3_dup = dup(fd);
	char buf[1000];
	memset(buf,'\0',sizeof(buf));
	int rd = read(dups,&buf[0],1000);
	if (rd < 0)
	{
		perror("Failed read ");
		return 1;
	}
	printf("Reading Content of file with fd: %s\n",buf);
	
	read(part3_dup,&buf[0],1000);
	printf("\nReading Content of file with dup: %s\n",buf);
	close(part3_dup);
	close(fd);
	return 0;
	
} 
/***************************************************************************************************************/
int _dup(int oldfd){

	if(fcntl(oldfd,F_GETFL) == -1) {
		errno = EBADF;
		return -1;
	}

	return fcntl(oldfd, F_DUPFD, 0);    //STDERR_FILENO
}

int _dup2(int oldfd, int newfd ){

	if(fcntl(oldfd,F_GETFL) == -1) {
		errno = EBADF;
		return -1;
	}
	if(oldfd == newfd) return oldfd;
	if((fcntl(oldfd,F_GETFL) == -1) && errno != EBADF) return -1;
	close(newfd);
	return fcntl(oldfd, F_DUPFD, newfd);
}