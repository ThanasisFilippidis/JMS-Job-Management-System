#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <error.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>

#define  MSGSIZE  512
#define	ACTIVE 0
#define SUSPENDED 1
#define TERMINATED 2



typedef struct myJob
{
	int id;
	pid_t pid;
	time_t submitTime;
	int status;
} Job;


int allJobsEnded(Job *jobs,int maxJobs){
	int i;
	for (i = 0; i < maxJobs; ++i)
	{
		if (jobs[i].status != 2)
		{
			return 0;
		}
	}
	return 1;
}

int termSignal = 0;
void handler(int signum){
	signal(SIGTERM,handler);
	termSignal = 1;
}

int main(int argc, char const *argv[])
{
	int poolInFd, poolOutFd, sizeWritten, i, poolId, status, currJob = 0, jobId, stdOutFd, stdErrFd, maxJobs = 0;
	char  msgbuf[MSGSIZE +1], *path = NULL, timeBuf[256], dateBuf[256], stdOutBuf[256], stdErrBuf[256], resultsBuf[MSGSIZE +1], jobTime[256];
	pid_t pid;
	time_t rawTime;
	struct tm *ptrTime;
	struct stat st = {0};


	char poolInName[256], poolOutName[256], outDir[256];
	poolId = atoi(argv[1]);
	path = malloc(sizeof(char)*(strlen(argv[2]) + 1));
	strcpy(path,argv[2]);
	maxJobs = atoi(argv[3]);
	Job jobs[maxJobs];
	sprintf(poolOutName,"%spool_%d_Out", path, poolId);
	sprintf(poolInName,"%spool_%d_In", path, poolId);

	if ( (poolOutFd=open(poolOutName, O_WRONLY)) <= 0)
	{
		perror("fife open error"); exit(1);
	}
	//printf("%d fd\n", poolOutFd);
	
	if ( (poolInFd=open(poolInName, O_RDONLY| O_NONBLOCK)) < 0)
	{
		perror("fifo open problem"); exit(3);	
	}
	//printf("eftasa\n");

	while ((sizeWritten = write(poolOutFd, "Start", MSGSIZE + 1)) == -1)
	{
	}
	
	printf("Pool with id %d Created.\n", poolId);

	char *command;
	char **args;

	signal(SIGTERM,handler);

	while(1){
		//printf("eftasa3\n");
		if (!termSignal)
		{
			if (read(poolInFd, msgbuf, MSGSIZE+1) > 0)
			{
				command = NULL;
				args = NULL;
				printf("POOL %d: Pira %s\n", poolId,msgbuf);
				command = strtok(msgbuf," ");
				//perror("problem in reading"); exit(5);
				if (!strcmp(command,"submit") && currJob != maxJobs)
				{
					int argsCounter = -1;
					currJob++;
					jobId = poolId * maxJobs + currJob;
					jobs[currJob - 1].submitTime = time (&rawTime);
					ptrTime = localtime(&rawTime);

					pid = fork();
					switch(pid){
						case -1:
							perror("fork");
							return 1;
						case 0:
							/*args = malloc(sizeof(char*));
							args[argsCounter] = malloc(sizeof(char)*(strlen(command) + 1));
							strcpy(args[argsCounter], command);*/
							//argsCounter++;
							pid  = getpid();
							
							strftime(dateBuf, 256, "%Y%m%d", ptrTime);
							strftime(timeBuf, 256 ,"%H%M%S", ptrTime);

							sprintf(outDir,"%ssdi1400215_%d_%d_%s_%s/", path, jobId, pid, dateBuf, timeBuf);
							sprintf(stdOutBuf,"%sstdout_%d", outDir, jobId);
							sprintf(stdErrBuf,"%sstderr_%d", outDir, jobId);


							if (stat(outDir, &st) == -1)
							{
								mkdir(outDir, 0777);
							}

							if ((stdOutFd = open(stdOutBuf, O_WRONLY | O_CREAT, 0666)) < 0 )
							{
								perror("Job fds problem:");
								return 1;
							}

							if ((stdErrFd = open(stdErrBuf, O_WRONLY | O_CREAT, 0666)) < 0 )
							{
								perror("Job fds problem:");
								return 1;
							}

							
							if (dup2(stdOutFd, STDOUT_FILENO) < 0)
							{
								perror("dup3");
								close(stdOutFd);
								return 1;
							}
							
							close(stdOutFd);
							
							if (dup2(stdErrFd, STDERR_FILENO) < 0)
							{
								perror("dup4");
								close(stdErrFd);
								return 1;
							}
							close(stdErrFd);

							//printf("DIR NAME IS %s\n", outDir);

							while((command = strtok(NULL," ")) != NULL){
								//printf("command is %s\n", command);
								args = realloc(args, sizeof(char*)*(++argsCounter));
								args[argsCounter] = malloc(sizeof(char)*(strlen(command) + 1));
								strcpy(args[argsCounter], command);
								//printf("%s\n", args[argsCounter]);
							}
							
							args = realloc(args, sizeof(char*)*(++argsCounter));
							args[argsCounter] = NULL;

							
							/*for (i = 0; i <= argsCounter - 1; ++i)
							{
								printf("arg %s\n", args[i]);
							}*/
							
							if (execvp(args[0],args) == -1){
								perror("execvp failed");
								return 1;
							}
						default:
							jobs[currJob - 1].pid = pid;
							jobs[currJob - 1].id = jobId;
							jobs[currJob - 1].status = ACTIVE;
							sprintf(resultsBuf,"Sending Results#JobID: %d, PID: %d.#", jobId, pid);
							//printf("TO MSG %s\n", resultsBuf);
							if ((sizeWritten = write(poolOutFd, resultsBuf, MSGSIZE + 1)) == -1)
							{
								perror("Error in Writing"); exit(2);
							}
							printf("POOL %d: Estila %s\n", poolId,resultsBuf);
							//printf("Sinexizw\n");
					}	
				}else if (!strcmp(command,"Finish"))
				{
					break;
				}else if (!strcmp(command,"status"))
				{
					 jobId = atol(strtok(NULL," "));
					 //printf("TO JOBI %d to curr %d tp poli %d kaiii %d\n", jobId,currJob,poolId,jobs[i].id);
					 for (i = 0; i < currJob; ++i)
					 {
					 	if (jobs[i].id == jobId)
					 	{
					 		if (jobs[i].status == ACTIVE)
					 		{
					 			time_t currTime = time(NULL);
					 			sprintf(resultsBuf,"Sending Results#JobID: %d Status: Active (running for %ld sec)#", jobId, currTime - jobs[i].submitTime);
					 			//printf("STELNW AC%s\n", resultsBuf);
								if ((sizeWritten = write(poolOutFd, resultsBuf, MSGSIZE + 1)) == -1)
								{
									perror("Error in Writing"); exit(2);
								}	
					 		}else if (jobs[i].status == SUSPENDED)
					 		{
					 			sprintf(resultsBuf,"Sending Results#JobID: %d Status: Suspended#", jobId);
					 			//printf("STELNW TER%s\n", resultsBuf);
					 			if ((sizeWritten = write(poolOutFd, resultsBuf, MSGSIZE + 1)) == -1)
								{
									perror("Error in Writing"); exit(2);
								}	
					 		}else if (jobs[i].status == TERMINATED)
					 		{
					 			sprintf(resultsBuf,"Sending Results#JobID: %d Status: Finished#", jobId);
					 			//printf("STELNW TER%s\n", resultsBuf);
					 			if ((sizeWritten = write(poolOutFd, resultsBuf, MSGSIZE + 1)) == -1)
								{
									perror("Error in Writing"); exit(2);
								}
					 		}
					 	}
					 }
				}else if (!strcmp(command,"status-all"))
				{
					long timeDiff = -1;
					if ((command = strtok(NULL,"\n")) != NULL )
					{
						timeDiff = atol(command);
						//lprintf("POOL : PIRAAAAAAA DIAFORAAAAA WOHOOOOOOOOO %ld\n", timeDiff);
					}
					time_t currTime = time(NULL);

					for (i = 0; i < currJob; ++i)
					{
						if (timeDiff != -1)
						{
							if ((currTime - jobs[i].submitTime) <= timeDiff)
							{
								if (jobs[i].status == ACTIVE)
						 		{
						 			//time_t currTime = time(NULL);
						 			sprintf(resultsBuf,"Sending Results#JobID: %d Status: Active (running for %ld sec)#", jobs[i].id, currTime - jobs[i].submitTime);
						 			//printf("STELNW AC%s\n", resultsBuf);
									if ((sizeWritten = write(poolOutFd, resultsBuf, MSGSIZE + 1)) == -1)
									{
										perror("Error in Writing"); exit(2);
									}	
						 		}else if (jobs[i].status == SUSPENDED)
						 		{
						 			sprintf(resultsBuf,"Sending Results#JobID: %d Status: Suspended#", jobs[i].id);
						 			//printf("STELNW TER%s\n", resultsBuf);
						 			if ((sizeWritten = write(poolOutFd, resultsBuf, MSGSIZE + 1)) == -1)
									{
										perror("Error in Writing"); exit(2);
									}	
						 		}else if (jobs[i].status == TERMINATED)
						 		{
						 			sprintf(resultsBuf,"Sending Results#JobID: %d Status: Finished#", jobs[i].id);
						 			//printf("STELNW TER%s\n", resultsBuf);
						 			if ((sizeWritten = write(poolOutFd, resultsBuf, MSGSIZE + 1)) == -1)
									{
										perror("Error in Writing"); exit(2);
									}
						 		}
							}
						}else{
							if (jobs[i].status == ACTIVE)
					 		{
					 			//time_t currTime = time(NULL);
					 			sprintf(resultsBuf,"Sending Results#JobID: %d Status: Active (running for %ld sec)#", jobs[i].id, currTime - jobs[i].submitTime);
					 			//printf("STELNW AC%s\n", resultsBuf);
								if ((sizeWritten = write(poolOutFd, resultsBuf, MSGSIZE + 1)) == -1)
								{
									perror("Error in Writing"); exit(2);
								}	
					 		}else if (jobs[i].status == SUSPENDED)
					 		{
					 			sprintf(resultsBuf,"Sending Results#JobID: %d Status: Suspended#", jobs[i].id);
					 			//printf("STELNW TER%s\n", resultsBuf);
					 			if ((sizeWritten = write(poolOutFd, resultsBuf, MSGSIZE + 1)) == -1)
								{
									perror("Error in Writing"); exit(2);
								}	
					 		}else if (jobs[i].status == TERMINATED)
					 		{
					 			sprintf(resultsBuf,"Sending Results#JobID: %d Status: Finished#", jobs[i].id);
					 			//printf("STELNW TER%s\n", resultsBuf);
					 			if ((sizeWritten = write(poolOutFd, resultsBuf, MSGSIZE + 1)) == -1)
								{
									perror("Error in Writing"); exit(2);
								}
					 		}
						}
				 		
					}
					if ((sizeWritten = write(poolOutFd, "Jobs End#", MSGSIZE + 1)) == -1)
					{
						perror("Error in Writing"); exit(2);
					}
				}else if (!strcmp(command,"show-active"))
				{
					for (i = 0; i < currJob; ++i)
					{
				 		if (jobs[i].status == ACTIVE)
				 		{
				 			//time_t currTime = time(NULL);
				 			sprintf(resultsBuf,"Sending Results#JobID %d#", jobs[i].id);
				 			//printf("STELNW AC%s\n", resultsBuf);
							if ((sizeWritten = write(poolOutFd, resultsBuf, MSGSIZE + 1)) == -1)
							{
								perror("Error in Writing"); exit(2);
							}	
				 		}	
					}
					if ((sizeWritten = write(poolOutFd, "Jobs End#", MSGSIZE + 1)) == -1)
					{
						perror("Error in Writing"); exit(2);
					}
				}else if (!strcmp(command,"show-pools"))
				{
					int activeCounter = 0;
					for (i = 0; i < currJob; ++i)
					{
				 		if (jobs[i].status == ACTIVE || jobs[i].status == SUSPENDED)
				 		{
				 				activeCounter++;
				 		}
					}
					sprintf(resultsBuf,"Sending Results#%d %d#", getpid(), activeCounter);
					if ((sizeWritten = write(poolOutFd, resultsBuf, MSGSIZE + 1)) == -1)
					{
						perror("Error in Writing"); exit(2);
					}
				}
				else if (!strcmp(command,"show-finished"))
				{
					for (i = 0; i < currJob; ++i)
					{
				 		if (jobs[i].status == TERMINATED)
				 		{
				 			//time_t currTime = time(NULL);
				 			sprintf(resultsBuf,"Sending Results#JobID %d#", jobs[i].id);
				 			//printf("STELNW AC%s\n", resultsBuf);
							if ((sizeWritten = write(poolOutFd, resultsBuf, MSGSIZE + 1)) == -1)
							{
								perror("Error in Writing"); exit(2);
							}	
				 		}	
					}
					if ((sizeWritten = write(poolOutFd, "Jobs End#", MSGSIZE + 1)) == -1)
					{
						perror("Error in Writing"); exit(2);
					}
				}else if (!strcmp(command,"suspend"))
				{
					 jobId = atoi(strtok(NULL," "));
					 for (i = 0; i < currJob; ++i)
					 {
					 	if (jobs[i].id == jobId)
					 	{
					 		if ((pid = waitpid(jobs[i].pid, &status, WNOHANG)) > 0)
					 		{
					 			if (WIFEXITED(status))
								{
									printf("child %d terminated\n", pid);
									jobs[i].status = TERMINATED;
								}else if (WIFCONTINUED(status))
								{
									jobs[i].status = ACTIVE;
								}else if (WIFSTOPPED(status))
								{
									jobs[i].status = SUSPENDED;
								}
					 		}
					 		break;
					 	}
					}
					if (jobs[i].status == ACTIVE)
			 		{
			 			//printf("ENA KLIK PRIN TO STILW\n");
			 			if (kill(jobs[i].pid, 19) == -1)
			 			{
			 				perror("Error in signaling"); exit(2);	
			 			}
			 			while((pid = waitpid(jobs[i].pid, &status, WNOHANG | WUNTRACED | WCONTINUED)) <= 0){
			 			}
			 			//printf("E ALLAKSE EPITELOUS\n");
			 			if (WIFEXITED(status))
						{
							jobs[i].status = TERMINATED;
						}else if (WIFCONTINUED(status))
						{
							jobs[i].status = ACTIVE;
						}else if (WIFSTOPPED(status))
						{
							//printf("OLA KALAAAA AAAA OLA KALAAA THA MAS PANE OLA KALAAAA AAAA\n");
							jobs[i].status = SUSPENDED;
							sprintf(resultsBuf,"Sending Results#Sent suspend signal to JobID %d#", jobs[i].id);
							if ((sizeWritten = write(poolOutFd, resultsBuf, MSGSIZE + 1)) == -1)
							{
								perror("Error in Writing"); exit(2);
							}
						}						
			 		}else if (jobs[i].status == SUSPENDED)
			 		{
			 			sprintf(resultsBuf,"Sending Results#Was not able to suspend Job %d: Job already suspended#", jobs[i].id);
			 			if ((sizeWritten = write(poolOutFd, resultsBuf, MSGSIZE + 1)) == -1)
						{
							perror("Error in Writing"); exit(2);
						}	
			 		}else if (jobs[i].status == TERMINATED)
			 		{
			 			sprintf(resultsBuf,"Sending Results#Was not able to suspend Job %d: Job already terminated#", jobs[i].id);
			 			//printf("STELNW TER%s\n", resultsBuf);
			 			if ((sizeWritten = write(poolOutFd, resultsBuf, MSGSIZE + 1)) == -1)
						{
							perror("Error in Writing"); exit(2);
						}
			 		}
			 		if ((sizeWritten = write(poolOutFd, "Jobs End#", MSGSIZE + 1)) == -1)
					{
						perror("Error in Writing"); exit(2);
					}
				}else if (!strcmp(command,"resume"))
				{
					 jobId = atoi(strtok(NULL," "));
					 for (i = 0; i < currJob; ++i)
					 {
					 	if (jobs[i].id == jobId)
					 	{
					 		if ((pid = waitpid(jobs[i].pid, &status, WNOHANG| WUNTRACED | WCONTINUED)) > 0)
					 		{
					 			if (WIFEXITED(status))
								{
									printf("child %d terminated\n", pid);
									jobs[i].status = TERMINATED;
								}else if (WIFCONTINUED(status))
								{
									jobs[i].status = ACTIVE;
								}else if (WIFSTOPPED(status))
								{
									jobs[i].status = SUSPENDED;
								}
					 		}
					 		break;
					 	}
					}
					if (jobs[i].status == ACTIVE)
			 		{
			 			sprintf(resultsBuf,"Sending Results#Was not able to resume Job %d: Job already active#", jobs[i].id);
			 			if ((sizeWritten = write(poolOutFd, resultsBuf, MSGSIZE + 1)) == -1)
						{
							perror("Error in Writing"); exit(2);
						}						
			 		}else if (jobs[i].status == SUSPENDED)
			 		{
			 			if (kill(jobs[i].pid, 18) == -1)
			 			{
			 				perror("Error in signaling"); exit(2);	
			 			}
			 			while((pid = waitpid(jobs[i].pid, &status, WNOHANG| WUNTRACED | WCONTINUED)) <= 0){
			 			}
			 			//printf("E ALLAKSE EPITELOUS\n");
			 			if (WIFEXITED(status))
						{
							jobs[i].status = TERMINATED;
						}else if (WIFCONTINUED(status))
						{
							jobs[i].status = ACTIVE;
							sprintf(resultsBuf,"Sending Results#Sent resume signal to JobID %d#", jobs[i].id);
							if ((sizeWritten = write(poolOutFd, resultsBuf, MSGSIZE + 1)) == -1)
							{
								perror("Error in Writing"); exit(2);
							}
						}else if (WIFSTOPPED(status))
						{
							//printf("OLA KALAAAA AAAA OLA KALAAA THA MAS PANE OLA KALAAAA AAAA\n");
							jobs[i].status = SUSPENDED;
							
						}
			 		}else if (jobs[i].status == TERMINATED)
			 		{
			 			sprintf(resultsBuf,"Sending Results#Was not able to resume Job %d: Job already terminated#", jobs[i].id);
			 			//printf("STELNW TER%s\n", resultsBuf);
			 			if ((sizeWritten = write(poolOutFd, resultsBuf, MSGSIZE + 1)) == -1)
						{
							perror("Error in Writing"); exit(2);
						}
			 		}
			 		if ((sizeWritten = write(poolOutFd, "Jobs End#", MSGSIZE + 1)) == -1)
					{
						perror("Error in Writing"); exit(2);
					}
				}			
			}
			if ((pid = waitpid(-1, &status, WNOHANG)) > 0){
				
				for (i = 0; i < currJob; ++i)
				{
					if (jobs[i].pid == pid)
					{
						if (WIFEXITED(status))
						{
							printf("child %d terminated\n", pid);
							jobs[i].status = TERMINATED;
						}else if (WIFCONTINUED(status))
						{
							jobs[i].status = ACTIVE;
						}else if (WIFSTOPPED(status))
						{
							jobs[i].status = SUSPENDED;
						}
						break;
					}
				}
				if (allJobsEnded(jobs, maxJobs) && currJob == maxJobs)
				{
					if ((sizeWritten = write(poolOutFd, "Wanna finish#", MSGSIZE + 1)) == -1)
					{
						perror("Error in Writing"); exit(2);
					}
					for (i = 0; i < currJob; ++i)
					{
						sprintf(jobTime, "%d#%ld", jobs[i].id, jobs[i].submitTime);
						if ((sizeWritten = write(poolOutFd, jobTime, MSGSIZE + 1)) == -1)
						{
							perror("Error in Writing"); exit(2);
						}	
					}
					if ((sizeWritten = write(poolOutFd, "End", MSGSIZE + 1)) == -1)
					{
						perror("Error in Writing"); exit(2);
					}
					printf("POOL %d: Estila End\n",poolId);
				}
			}
		}else{
			int activeCounter = 0;
			for (i = 0; i < currJob; ++i)
			{
				if ((pid = waitpid(jobs[i].pid, &status, WNOHANG)) > 0){
					if (WIFEXITED(status))
					{
						printf("child %d terminated\n", pid);
						jobs[i].status = TERMINATED;
					}
				}
				if (jobs[i].status == ACTIVE)
				{
					activeCounter++;
					kill(jobs[i].pid, 15);
				}
			}
			sprintf(jobTime, "#%d#",activeCounter);
			if ((sizeWritten = write(poolOutFd, jobTime, MSGSIZE + 1)) == -1)
			{
				perror("Error in Writing"); exit(2);
			}
			while(read(poolInFd, msgbuf, MSGSIZE+1) <= 0){
			}
			break;
		}
	}
	if(close(poolOutFd) < 0)
		return 1;
	if(close(poolInFd) < 0)
		return 1;
	printf("Pool with id %d terminated.\n", getpid());
	return 0;
}