#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include  <fcntl.h>
#include <error.h>
#include <unistd.h>
#include <errno.h>

#define  MSGSIZE  512
#define MAXFDS 1024
#define MAXJOBS 2
#define	ACTIVE 0
#define SUSPENDED 1
#define TERMINATED 2



int main(int argc, char const *argv[])
{
	if (argc != 7)
	{
		printf("Wrong number of arguments.\n");
		printf("Correct use is:\n");
		printf("./jms_console -w <jms in> -r <jms out> -o <operations file>\n");
		return 1;
	}

	int i, jms_inFd, jms_outFd, sizeWritten;
	size_t n = MSGSIZE;
	char *jms_in, *jms_out, *instruction = NULL, inFlag = 0;
	char  msgbuf[MSGSIZE +1];
	FILE *filepointerIn = NULL;

	instruction = malloc(n*sizeof(char));

	for (i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i],"-o"))
		{
			filepointerIn = fopen (argv[i + 1],"r");
			if (filepointerIn == NULL)
			{
				printf("Unable to open input file.\n");
				return 1;
			}
			inFlag++;
		}else if (!strcmp(argv[i],"-w"))
		{
			jms_in = malloc(sizeof(char)*(strlen(argv[i+1]) + 1));
			strcpy(jms_in, argv[i+1]);
		}else if (!strcmp(argv[i],"-r"))
		{
			jms_out = malloc(sizeof(char)*(strlen(argv[i+1]) + 1));
			strcpy(jms_out, argv[i+1]);
		}
	}
	//printf("%s %s\n", jms_in , jms_out );

	

	if ( (jms_inFd=open(jms_in, O_WRONLY)) < 0)
	{
		perror("fife open error"); exit(1);
	}

	/*if ((sizeWritten = write(jms_inFd, "Start", MSGSIZE + 1)) == -1)
	{
		perror("Error in Writing"); exit(2);
	}*/


	

	if ( (jms_outFd=open(jms_out, O_RDONLY)) < 0)
	{
		perror("fifo open problem"); exit(3);	
	}


	/*while(read(jms_outFd, msgbuf, MSGSIZE+1) < 0){
	}*/
	//printf("e\n");

	

	/*if ((sizeWritten = write(jms_inFd, "Start", MSGSIZE + 1)) == -1)
	{
		perror("Error in Writing"); exit(2);
	}*/


	
	//printf("%d writen\n", sizeWritten);


	while(1){
		
		//printf("Perasa\n");
		if (inFlag)
		{
			if (getline(&instruction,&n,filepointerIn) != -1)
			{
				if ((strtok(instruction,"\n")) == NULL)
				{
					printf("Input end.\n");
					inFlag = 0;
					continue;
				}
				printf("%s\n", instruction);
				if ((sizeWritten = write(jms_inFd, strtok(instruction,"\n"), MSGSIZE + 1)) == -1)
					{ perror("Error edw in Writing"); exit(2); }
				if (!strcmp(strtok(instruction,"\n"), "shutdown"))
				{
					if (read(jms_outFd, msgbuf, MSGSIZE+1) > 0)
					{
						printf("%s\n", msgbuf);
						fflush(stdout);
					}
					break;				
					// printf("%s\n", strtok(instruction,"\n"));
				}
				// printf("Estila %s\n", strtok(instruction,"\n"));
			}else{
				printf("Input end.\n");
				inFlag = 0;
			}
		}else{
			printf("Give command: ");
			if (getline(&instruction,&n,stdin) != -1)
			{
				if ((sizeWritten = write(jms_inFd, strtok(instruction,"\n"), MSGSIZE + 1)) == -1)
				{
					perror("Error in Writing"); exit(2);
				}
				if (!strcmp(strtok(instruction,"\n"), "shutdown"))
				{
					if (read(jms_outFd, msgbuf, MSGSIZE+1) > 0)
					{
						printf("%s\n", msgbuf);
						fflush(stdout);
					}
					break;				
					// printf("%s\n", strtok(instruction,"\n"));
				}
			}else{
				break;
			}
		}
		while (read(jms_outFd, msgbuf, MSGSIZE+1) > 0)
		{
			if (!strcmp(msgbuf,"Results end"))
			{
				break;
			}else{
				printf("%s\n", msgbuf);
				fflush(stdout);
			}
		}
	}

	free(jms_in);
	free(jms_out);
	if (filepointerIn != NULL)
	{
		fclose(filepointerIn);
	}
	free(instruction);
	if(close(jms_outFd) < 0)
		return 1;
	if(close(jms_inFd) < 0)
		return 1;

	printf("Jms console left.\n");

	return 0;
}