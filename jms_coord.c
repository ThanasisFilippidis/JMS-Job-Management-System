#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include  <fcntl.h>
#include <error.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

#define  MSGSIZE  512
#define MAXFDS 1024
#define MAXJOBS 2
#define	ACTIVE 0
#define SUSPENDED 1
#define TERMINATED 2

typedef struct MyPool 
{
	int id;
	pid_t pid;
	int numJobs;
	char *poolInName;
	char *poolOutName;
	int fdIn;
	int fdOut;
	int status;
} Pool;

typedef struct myFinishedJob
{
	int id;
	long int submitTime;
} finishedJob;

int finishRequest(Pool *pools, int poolRequested, finishedJob *finishedJobs, int *finished){
	char msgbuf[MSGSIZE +1], *instruction = NULL;
	while(1){
		if ((read(pools[poolRequested].fdOut, msgbuf, MSGSIZE + 1) > 0)){
			//printf("pira %s\n", msgbuf);
			instruction = strtok(msgbuf,"#");
			//printf("%s ginete %d\n", instruction, atoi(instruction));
			if (!strcmp(instruction,"End"))
			{
				break;
			}
			finishedJobs[*finished].id = atoi(instruction);
			finishedJobs[*finished].submitTime = atol(strtok(NULL,"#"));
			//printf("%d %ld\n", finishedJobs[*finished].id, finishedJobs[*finished].submitTime);
			(*finished)++;
		}
	}

	pools[poolRequested].status = TERMINATED;
	//printf("Pool %d status %d.\n", poolRequested, pools[poolRequested].status);
	//printf("O fd %d\n", pools[poolRequested].fdIn);
	if ((write(pools[poolRequested].fdIn, "Finish", MSGSIZE + 1)) == -1)
	{
		perror("Error in Writing"); exit(2);
	}
	//printf("TOLIKSA\n");
	if(close(pools[poolRequested].fdIn) < 0)
		return 1;
	if(close(pools[poolRequested].fdOut) < 0)
		return 1;
}

int main(int argc, char const *argv[])
{
	if (argc != 9)
	{
		printf("Wrong number of arguments.\n");
		printf("Correct use is:\n");
		printf("./jms coord -l <path> -n <jobs pool> -w <jms out> -r <jms in>\n");
		return 1;
	}

	int i, jms_inFd, jms_outFd, poolOutFd, poolInFd, sizeWritten, maxJobs = MAXJOBS, currJobs = 0, currFds = -1, finished = 0;;
	size_t n = MSGSIZE;
	char *jms_in = NULL, *jms_out = NULL, *instruction = NULL, *job = NULL, inFlag = 0, *path = NULL, *tmpmsg = NULL;
	char  msgbuf[MSGSIZE +1], msgbuf2[MSGSIZE +1], resultsBuf[MSGSIZE +1];
	FILE *filepointerIn = NULL;
	pid_t pid;
	Pool pools[MAXFDS];
	struct stat st = {0};

	//instruction = malloc(n*sizeof(char));

	for (i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i],"-l"))
		{
			path = malloc(sizeof(char)*(strlen(argv[i+1]) + 1));
			strcpy(path,argv[i+1]);
		}else if (!strcmp(argv[i],"-w"))
		{
			jms_out = malloc(sizeof(char)*(strlen(argv[i+1]) + 1));
			strcpy(jms_out, argv[i+1]);	
		}else if (!strcmp(argv[i],"-r"))
		{
			jms_in = malloc(sizeof(char)*(strlen(argv[i+1]) + 1));
			strcpy(jms_in, argv[i+1]);
		}else if (!strcmp(argv[i],"-n"))
		{
			maxJobs = atoi(argv[i+1]);
		}

	}
	finishedJob finishedJobs[MAXFDS*maxJobs];
	//printf("%s %s\n", jms_in , jms_out );
	if (path != NULL)
	{
		if (stat(path, &st) == -1)
		{
			mkdir(path, 0777);
		}
	}
		

	if (mkfifo(jms_in, 0666) == -1)
	{
		if (errno!=EEXIST) 
		{
			perror("receiver: mkfifo");
			exit(6);
		}
	}

	if ( mkfifo(jms_out, 0666) == -1 )
	{
		if ( errno!=EEXIST ) 
		{
			perror("receiver: mkfifo");
			exit(6);
		}
	}
	//printf("EEEE\n");
	if ((jms_inFd=open(jms_in,  O_RDONLY| O_NONBLOCK)) < 0)
	{
		perror("fifo open problem"); exit(3);	
	}
	
	if ((jms_outFd=open(jms_out, O_WRONLY)) < 0)
	{
		perror("fife open error"); exit(1);
	}

	while(1){
		//printf("aoua\n");

		int poolWrote = -1;
		for (i = 0; i <= currFds; ++i)
		{
			if (pools[i].status == TERMINATED)
			{
				continue;
			}
			if (read(pools[i].fdOut, msgbuf, MSGSIZE+1) > 0)
			{
				poolWrote = i;
				break;
			}
		}
		//printf("COORD: Pool section\n");
		if (poolWrote != -1)
		{
			//printf("Egrapse odws malakes\n");
			tmpmsg = realloc(tmpmsg,sizeof(char)*(strlen(msgbuf) + 1));
			strcpy(tmpmsg,msgbuf);
			instruction = strtok(tmpmsg,"#");
			if (!strcmp(instruction,"Sending Results"))
			{
				if ((sizeWritten = write(jms_outFd, strtok(NULL,"#"), MSGSIZE + 1)) == -1)
				{
					perror("Error in Writing"); exit(2);
				}
				if ((sizeWritten = write(jms_outFd, "Results end", MSGSIZE + 1)) == -1)
				{
					perror("Error in Writing"); exit(2);
				}
			}else if (!strcmp(instruction,"Wanna finish"))
			{
				if (finishRequest(pools, poolWrote, finishedJobs, &finished))
				{
					printf("COORD: Pool %d marked finished\n", poolWrote);
				}
			}
		}
		//printf("TAPAME %d\n", poolWrote);
		if ((read(jms_inFd, msgbuf, MSGSIZE+1) > 0))
		{
			printf("COORD: Console section\n");
			tmpmsg = realloc(tmpmsg,sizeof(char)*(strlen(msgbuf) + 1));
			strcpy(tmpmsg,msgbuf);
			instruction = strtok(tmpmsg," ");
			printf("instruction: %s\n", instruction);
			if (!strcmp(instruction,"submit"))
			{
				//job = strtok(NULL, "\n");
				//printf("to job1 %s\n", job);
				//printf("Exw %d jobs apo %d\n", currJobs,maxJobs);
				if (!(currJobs%maxJobs))
				{
					//printf("PERASA\n");
					currFds++;
					char poolInName[256], poolOutName[256], idString[10];
					sprintf(poolOutName,"%spool_%d_Out", path, currFds);
					sprintf(poolInName,"%spool_%d_In", path, currFds);
					sprintf(idString,"%d",currFds);
					sprintf(resultsBuf,"%d", maxJobs);
					if (mkfifo(poolOutName, 0666) == -1)
					{
						if (errno!=EEXIST) 
						{
							perror("receiver: mkfifo");
							exit(6);
						}
					}
					if ( mkfifo(poolInName, 0666) == -1 )
					{
						if ( errno!=EEXIST ) 
						{
							perror("receiver: mkfifo");
							exit(6);
						}
					}
					printf("Pool Created.\n");
					
					//printf("E\n");
					pid = fork();
					switch(pid){
						case -1:
							perror("fork");
							return 1;
						case 0:
							if (execlp("./pool","pool",idString,path,resultsBuf,(char*)NULL) == -1)
							{
									perror("execlp failed");
									exit(-1);
							}
						default:
							pools[currFds].id = currFds;
							pools[currFds].pid = pid;
							pools[currFds].status = ACTIVE;
							pools[currFds].poolInName = malloc(sizeof(char)*(strlen(poolInName) + 1));
							strcpy(pools[currFds].poolInName, poolInName);
							pools[currFds].poolOutName = malloc(sizeof(char)*(strlen(poolOutName) + 1));
							strcpy(pools[currFds].poolOutName, poolOutName);

							if ((pools[currFds].fdOut=open(poolOutName,  O_RDONLY| O_NONBLOCK)) < 0)
							{
								perror("fifo open problem"); exit(3);	
							}
							
							if ((pools[currFds].fdIn=open(poolInName, O_WRONLY)) < 0)
							{
								perror("fife open error"); exit(1);
							}
							while(read(pools[currFds].fdOut, msgbuf2, MSGSIZE+1) <= 0){
							}
							//printf("to jobi %s\n", job);
							
							printf("Childs id is %d\n", pools[currFds].pid);
							//printf("Estila %s\n", job);
							//sleep(2);

							//printf("E2\n");
					}
				}
				if ((sizeWritten = write(pools[currFds].fdIn, msgbuf, MSGSIZE + 1)) == -1)
				{
					perror("Error ame in Writing"); exit(2);
				}
				currJobs++;
				pools[currFds].numJobs = currJobs;
				/*if ((sizeWritten = write(jms_outFd, "Submit Successful.", MSGSIZE + 1)) == -1)
				{
					perror("Error in Writing"); exit(2);
				}*/
				//printf("VGENW\n");	
			}else if (!strcmp(instruction,"shutdown"))
			{
				int activeJobs = 0;
				for (i = 0; i <= currFds; ++i)
				{
					if (pools[i].status == TERMINATED)
					{
						continue;
					}
					kill(pools[i].pid, SIGTERM);
					while(1){
						if ((read(pools[i].fdOut, msgbuf2, MSGSIZE +1)) > 0)
						{
							//printf("COORD: PIRA GIA STATUS :%s\n", msgbuf2);
							instruction = strtok(msgbuf2, "#");
							activeJobs += atoi(strtok(msgbuf2,"#"));
							if ((sizeWritten = write(pools[i].fdIn, "Ok Leave", MSGSIZE + 1)) == -1)
							{
								perror("Error in Writing"); exit(2);
							}
							break;
						}
					}
				}
				sprintf(resultsBuf,"Served %d jobs, %d were still in progress\nShutdown Successful.", currJobs, activeJobs);
				if ((sizeWritten = write(jms_outFd, resultsBuf, MSGSIZE + 1)) == -1)
				{
					perror("Error in Writing"); exit(2);
				}
				break;
			}else if (!strcmp(instruction,"status"))
			{
				int requestedJob = atoi(strtok(NULL," "));
				if (requestedJob > currJobs)
				{
					if ((sizeWritten = write(jms_outFd, "JobId does not exist.", MSGSIZE + 1)) == -1)
					{
						perror("Error in Writing"); exit(2);
					}
					if ((sizeWritten = write(jms_outFd, "Results end", MSGSIZE + 1)) == -1)
					{
						perror("Error in Writing"); exit(2);
					}
					continue;
				}
				int requestedPool = (--requestedJob)/MAXJOBS;
				//printf("STELNW STATUS STIN %d\n",requestedPool);
				if (pools[requestedPool].status == TERMINATED)
				{
					sprintf(resultsBuf, "JobID %d Status: Finished", (++requestedJob));
					if ((sizeWritten = write(jms_outFd, resultsBuf, MSGSIZE + 1)) == -1)
					{
						perror("Error in Writing"); exit(2);
					}
					if ((sizeWritten = write(jms_outFd, "Results end", MSGSIZE + 1)) == -1)
					{
						perror("Error in Writing"); exit(2);
					}
				}else{
					if ((sizeWritten = write(pools[requestedPool].fdIn, msgbuf, MSGSIZE + 1)) == -1)
					{
						perror("Error ame in Writing"); exit(2);
					}
				}
			}else if (!strcmp(instruction,"status-all"))
			{
				//printf("EXOUNY TELIWSEI TA \n");
				//printf("TO PIRA MALAKEEEESSS AANTE GIAAAAAAAAAAAA\n");

				long timeDiff = -1;

				if ((instruction = strtok(NULL,"\n")) != NULL )
				{
					timeDiff = atol(instruction);
					//printf("PIRAAAAAAA DIAFORAAAAA WOHOOOOOOOOO %ld\n", timeDiff);
				}
				time_t currTime = time(NULL);

				for (i = 0; i <= currFds; ++i)
				{
					if (pools[i].status == TERMINATED)
					{
						continue;
					}
					if ((sizeWritten = write(pools[i].fdIn, msgbuf, MSGSIZE + 1)) == -1)
					{
						perror("Error ame in Writing"); exit(2);
					}
					while(1){
						if ((read(pools[i].fdOut, msgbuf2, MSGSIZE +1)) > 0)
						{
							//printf("COORD: PIRA GIA STATUS :%s\n", msgbuf2);
							instruction = strtok(msgbuf2, "#");
							if (!strcmp(instruction,"Sending Results"))
							{
								//printf("COORD: DIAVASA RESULT\n");
								if ((sizeWritten = write(jms_outFd, strtok(NULL,"#"), MSGSIZE + 1)) == -1)
								{
									perror("Error in Writing"); exit(2);
								}
							}else if (!strcmp(instruction,"Jobs End"))
							{
								//printf("COORD: JOBS END\n");
								break;
							}else if (!strcmp(instruction,"Wanna finish"))
							{
								//printf("COORD: THELW TELIWSW\n");
								if (finishRequest(pools, i, finishedJobs, &finished))
								{
									printf("COORD: Pool %d marked finished\n", poolWrote);
								}
								break;
							}
						}
					}
				}
				for (i = 0; i < finished; ++i)
				{
					if (timeDiff != -1)
					{
						if ((currTime - finishedJobs[i].submitTime) <= timeDiff)
						{
							sprintf(resultsBuf, "JobID %d Status: Finished", finishedJobs[i].id);
							//printf("%d ", finishedJobs[i].id);
							if ((sizeWritten = write(jms_outFd, resultsBuf, MSGSIZE + 1)) == -1)
							{
								perror("Error in Writing"); exit(2);
							}	
						}
					}else{
						sprintf(resultsBuf, "JobID %d Status: Finished", finishedJobs[i].id);
						//printf("%d ", finishedJobs[i].id);
						if ((sizeWritten = write(jms_outFd, resultsBuf, MSGSIZE + 1)) == -1)
						{
							perror("Error in Writing"); exit(2);
						}
					}
				}

				if ((sizeWritten = write(jms_outFd, "Results end", MSGSIZE + 1)) == -1)
				{
					perror("Error in Writing"); exit(2);
				}
			}else if (!strcmp(instruction,"show-active"))
			{
				if ((sizeWritten = write(jms_outFd, "Active Jobs:", MSGSIZE + 1)) == -1)
				{
					perror("Error in Writing"); exit(2);
				}
				for (i = 0; i <= currFds; ++i)
				{
					if (pools[i].status == TERMINATED)
					{
						continue;
					}
					if ((sizeWritten = write(pools[i].fdIn, msgbuf, MSGSIZE + 1)) == -1)
					{
						perror("Error ame in Writing"); exit(2);
					}
					while(1){
						if ((read(pools[i].fdOut, msgbuf2, MSGSIZE +1)) > 0)
						{
							//printf("COORD: PIRA GIA STATUS :%s\n", msgbuf2);
							instruction = strtok(msgbuf2, "#");
							if (!strcmp(instruction,"Sending Results"))
							{
								//printf("COORD: DIAVASA RESULT\n");
								if ((sizeWritten = write(jms_outFd, strtok(NULL,"#"), MSGSIZE + 1)) == -1)
								{
									perror("Error in Writing"); exit(2);
								}
							}else if (!strcmp(instruction,"Jobs End"))
							{
								//printf("COORD: JOBS END\n");
								printf("PIRA JOBS END APO TON %d\n", i);
								break;
							}else if (!strcmp(instruction,"Wanna finish"))
							{
								//printf("COORD: THELW TELIWSW\n");
								if (finishRequest(pools, i, finishedJobs, &finished))
								{
									printf("COORD: Pool %d marked finished\n", poolWrote);
								}
								break;
							}
						}
					}
				}

				if ((sizeWritten = write(jms_outFd, "Results end", MSGSIZE + 1)) == -1)
				{
					perror("Error in Writing"); exit(2);
				}
			}else if (!strcmp(instruction,"show-pools"))
			{
				if ((sizeWritten = write(jms_outFd, "Pool & NumOfJobs:", MSGSIZE + 1)) == -1)
				{
					perror("Error in Writing"); exit(2);
				}
				for (i = 0; i <= currFds; ++i)
				{
					if (pools[i].status == TERMINATED)
					{
						sprintf(resultsBuf,"%d 0",pools[i].pid);
						if ((sizeWritten = write(jms_outFd, resultsBuf, MSGSIZE + 1)) == -1)
						{
							perror("Error in Writing"); exit(2);
						}
						continue;
					}
					if ((sizeWritten = write(pools[i].fdIn, msgbuf, MSGSIZE + 1)) == -1)
					{
						perror("Error ame in Writing"); exit(2);
					}
					while(1){
						if ((read(pools[i].fdOut, msgbuf2, MSGSIZE +1)) > 0)
						{
							//printf("COORD: PIRA GIA STATUS :%s\n", msgbuf2);
							instruction = strtok(msgbuf2, "#");
							if (!strcmp(instruction,"Sending Results"))
							{
								//printf("COORD: DIAVASA RESULT\n");
								if ((sizeWritten = write(jms_outFd, strtok(NULL,"#"), MSGSIZE + 1)) == -1)
								{
									perror("Error in Writing"); exit(2);
								}
								break;
							}else if (!strcmp(instruction,"Wanna finish"))
							{
								//printf("COORD: THELW TELIWSW\n");
								if (finishRequest(pools, i, finishedJobs, &finished))
								{
									printf("COORD: Pool %d marked finished\n", poolWrote);
								}
								sprintf(resultsBuf,"%d 0",pools[i].pid);
								if ((sizeWritten = write(jms_outFd, resultsBuf, MSGSIZE + 1)) == -1)
								{
									perror("Error in Writing"); exit(2);
								}
								break;
							}
						}
					}
				}

				if ((sizeWritten = write(jms_outFd, "Results end", MSGSIZE + 1)) == -1)
				{
					perror("Error in Writing"); exit(2);
				}
			}else if (!strcmp(instruction,"show-finished"))
			{
				if ((sizeWritten = write(jms_outFd, "Finished jobs:", MSGSIZE + 1)) == -1)
				{
					perror("Error in Writing"); exit(2);
				}
				for (i = 0; i <= currFds; ++i)
				{
					if (pools[i].status == TERMINATED)
					{
						continue;
					}
					if ((sizeWritten = write(pools[i].fdIn, msgbuf, MSGSIZE + 1)) == -1)
					{
						perror("Error ame in Writing"); exit(2);
					}
					while(1){
						if ((read(pools[i].fdOut, msgbuf2, MSGSIZE +1)) > 0)
						{
							//printf("COORD: PIRA GIA STATUS :%s\n", msgbuf2);
							instruction = strtok(msgbuf2, "#");
							if (!strcmp(instruction,"Sending Results"))
							{
								//printf("COORD: DIAVASA RESULT\n");
								if ((sizeWritten = write(jms_outFd, strtok(NULL,"#"), MSGSIZE + 1)) == -1)
								{
									perror("Error in Writing"); exit(2);
								}
							}else if (!strcmp(instruction,"Jobs End"))
							{
								//printf("COORD: JOBS END\n");
								break;
							}else if (!strcmp(instruction,"Wanna finish"))
							{
								//printf("COORD: THELW TELIWSW\n");
								if (finishRequest(pools, i, finishedJobs, &finished))
								{
									printf("COORD: Pool %d marked finished\n", poolWrote);
								}
								break;
							}
						}
					}
				}
				for (i = 0; i < finished; ++i)
				{
					sprintf(resultsBuf, "JobID %d", finishedJobs[i].id);
					//printf("%d ", finishedJobs[i].id);
					if ((sizeWritten = write(jms_outFd, resultsBuf, MSGSIZE + 1)) == -1)
					{
						perror("Error in Writing"); exit(2);
					}
				}


				if ((sizeWritten = write(jms_outFd, "Results end", MSGSIZE + 1)) == -1)
				{
					perror("Error in Writing"); exit(2);
				}
			}else if (!strcmp(instruction,"suspend"))
			{
				int requestedJob = atoi(strtok(NULL," "));
				int requestedPool = (--requestedJob)/MAXJOBS;
				int printedFlag = 0;

				for (i = 0; i < finished; ++i)
				{
					if (finishedJobs[i].id == (++requestedJob))
					{
						printedFlag++;
						sprintf(resultsBuf, "Was not able to suspend Job %d: Job already finished", finishedJobs[i].id);
						if ((sizeWritten = write(jms_outFd, resultsBuf, MSGSIZE + 1)) == -1)
						{
							perror("Error in Writing"); exit(2);
						}
						if ((sizeWritten = write(jms_outFd, "Results end", MSGSIZE + 1)) == -1)
						{
							perror("Error in Writing"); exit(2);
						}
						break;
					}
				}
				if (!printedFlag)
				{
					if ((sizeWritten = write(pools[requestedPool].fdIn, msgbuf, MSGSIZE + 1)) == -1)
					{
						perror("Error in Writing"); exit(2);
					}
				}
			}else if (!strcmp(instruction,"resume"))
			{
				int requestedJob = atoi(strtok(NULL," "));
				int requestedPool = (--requestedJob)/MAXJOBS;
				int printedFlag = 0;

				for (i = 0; i < finished; ++i)
				{
					if (finishedJobs[i].id == (++requestedJob))
					{
						printedFlag++;
						sprintf(resultsBuf, "Was not able to resume Job %d: Job already finished", finishedJobs[i].id);
						if ((sizeWritten = write(jms_outFd, resultsBuf, MSGSIZE + 1)) == -1)
						{
							perror("Error in Writing"); exit(2);
						}
						if ((sizeWritten = write(jms_outFd, "Results end", MSGSIZE + 1)) == -1)
						{
							perror("Error in Writing"); exit(2);
						}
						break;
					}
				}
				if (!printedFlag)
				{
					if ((sizeWritten = write(pools[requestedPool].fdIn, msgbuf, MSGSIZE + 1)) == -1)
					{
						perror("Error in Writing"); exit(2);
					}
				}
			}else{
				if ((sizeWritten = write(jms_outFd, "Wrong command, try again.", MSGSIZE + 1)) == -1)
				{
					perror("Error mopws in Writing"); exit(2);
				}
				if ((sizeWritten = write(jms_outFd, "Results end", MSGSIZE + 1)) == -1)
				{
					perror("Error in Writing"); exit(2);
				}
			}
		}
	}
	
	/*for (i = 0; i < finished; ++i)
	{
		printf("%d %ld\n", finishedJobs[i].id, finishedJobs[i].submitTime);
	}*/


	for (i = 0; i <= currFds; ++i)
	{
		if (pools[i].status == ACTIVE)
		{
			if(close(pools[i].fdIn) < 0)
				return 1;
			if(close(pools[i].fdOut) < 0)
				return 1;
		}
		if (pools[i].poolInName != NULL)
			if (remove(pools[i].poolInName))
			{
				perror("Error in deleting pipe.");
				return 1;
			}
		if (pools[i].poolOutName != NULL)
			if (remove(pools[i].poolOutName))
			{
				perror("Error in deleting pipe.");
				return 1;
			}
	}

	if (remove(jms_in))
	{
		perror("Error in deleting pipe.");
		return 1;
	}
	if (remove(jms_out))
	{
		perror("Error in deleting pipe.");
		return 1;
	}

	if (jms_in != NULL)
		free(jms_in);
	if (jms_out != NULL)
		free(jms_out);
	if (path != NULL)
		free(path);
	if (tmpmsg != NULL){
		free(tmpmsg);
	}
	if(close(jms_outFd) < 0)
		return 1;
	if(close(jms_inFd) < 0)
		return 1;
	//free(jms_in);
	// free(jms_out);
	//free path
	//free pipes
	printf("Jms coord left.\n");
	//free(instruction);

	return 0;
}