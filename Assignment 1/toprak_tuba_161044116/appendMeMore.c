/***************************************************************************************************  
*
*
* 
* 	                                              Part1
*	Tuba Toprak
*	161044116 - HW1                &appendMeMore  filename   num-bytes  [x]
*	System Programming               argv[0]      argv[1]   argv[2]   argv[3]
*
*
*
*
*
*****************************************************************************************************/
#include <unistd.h> 
#include <stdio.h> 
#include <errno.h>                            
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>  


int main(int argc, char const *argv[])
{
	
	int fd,fd2, wr1,wr2;
	int num_bytes = 0;
	int counter = 0;

	if (argc < 3  || argc > 4){
		perror("Failed..\n");
		return 1;
	}
	

	else{

		char filename [100] =" ";
		strcpy(filename, argv[1]);
		num_bytes = atoi(argv[2]);

		if (argc == 3)		//o_append ile açmak için x değişkeni yok
		{
			fd = open(filename, O_RDWR| O_APPEND | O_CREAT ,S_IRWXU);
			if(fd == -1){
				perror("Failed to open  file. ");
				return 1;
			} 
			counter = 0;
			while (counter < num_bytes){
				if(counter % 2 == 0)
					wr1 = write(fd,"t",1);
				else
					wr1 = write(fd,"u",1);
				if(wr1 == -1){
					perror("Failed to write file. ");
					return 1;
				} 
				counter++;
				
			}
			close(fd);
		}

		else						//x değişkeni var lseek ile aç 
		{
			fd2 = open(filename, O_RDWR | O_CREAT ,S_IRWXU);
			if(fd2 == -1){
				perror("Failed to open  file. ");
				return 1;
			}
			counter = 0;
			while (counter < num_bytes){
				if (counter % 2 == 0){
					lseek(fd2,0,SEEK_END);
					wr2 = write(fd2,"b",1);
				}
				else{
					lseek(fd2,0,SEEK_END);
					wr2 = write(fd2,"a",1);
				}
				if(wr2 == -1){
					perror("Failed to write file. ");
					return 1;
				}
				counter++;

			}
			close(fd2);
			
		}

	}
} 

