/*
* Burak DEMİRCİ
* 141044091
*/

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <semaphore.h>
#include <fcntl.h>
#include <math.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include "uici.h"

#define BUF_MAX  1024
static sem_t sharedsem;
int mypid,counter=0;
double *connectTime;



void Request(void *n);
void Connection(int col, int row, int communfd, int tredT);
void SignalHandler(int signo);
void logCreate();

int main(int argc, char const *argv[])
{
	int i =0,error;
	pthread_t tid[BUF_MAX];

	if (argc != 5) {
	   fprintf(stderr, "Usage: %s <#of columns of A, m> <#of rows of A, p> <#of clients, q> <#portNo, pt>\n", argv[0]);
	   return 1;
	}
	/*Signal control*/
	signal (SIGINT, SignalHandler);

	mypid=(int)getpid();

	connectTime = (double*)calloc(atoi(argv[3]),sizeof(double));

	/*Create semaphore*/
	if (sem_init(&sharedsem, 0, 1) == -1)
        return -1;

	/*Create num of thred*/
	while(i < atoi(argv[3]))
	{
		connectTime[i] = 0.0;
		if (error = pthread_create(tid+i, NULL, Request,(void*)argv)) 
		{
		    printf("Exit condition: eror\n");
		    fprintf(stderr, "Failed to create thread %d: %s\n",
		    i, strerror(error));
		}
		i++;
	}	
	/*Wait thread joining*/
	i=0;
	while(i < atoi(argv[3])){
		if (error = pthread_join(tid[i], NULL));
		i++;
	}
	logCreate();
	return 0;
}

void Request(void *n)
{
	int bytescopied;
	int communfd;
	u_port_t portnumber;
	char **value = (char*)n; 
	char hostname[BUF_MAX];
	char buff[BUF_MAX];
	struct timeval start ,stop;
	double elapsedTime;

	portnumber = (u_port_t)atoi(value[4]);
	gethostname(hostname,BUF_MAX);
	
	/*Critical section*/
	while (sem_wait(&sharedsem) == -1)
        if (errno != EINTR)
          return -1;
    gettimeofday(&start, NULL); /*Conect start time*/  	
	if ((communfd = u_connect(portnumber, hostname)) == -1) {
	   perror("Failed to make connection");
	   exit(0);
	}

	sprintf(buff,"%d,%d,%d,",mypid,atoi(value[1]),atoi(value[2]));
	fprintf(stderr, "[%ld]:connected %s\n", (long)getpid(), value[4]);
	

	Connection(atoi(value[1]),atoi(value[2]),communfd,atoi(value[3]));
	/*-----time------------------*/
	gettimeofday(&stop, NULL); /*Connect Stop time*/
	elapsedTime = (stop.tv_sec - start.tv_sec) * 1000.0;   
	elapsedTime += (stop.tv_usec - start.tv_usec) / 1000.0;
	connectTime[counter]= elapsedTime;
	counter++;
	/*-----time------------------*/
	if (sem_post(&sharedsem) == -1);
}

void Connection(int col, int row, int communfd, int tredT)
{
	char buff[BUF_MAX];
	int ReturnVal[BUF_MAX];
	sprintf(buff,"%d,%d,%d,%d,",mypid,col,row,tredT);

	write(communfd,buff,sizeof(buff));

	read(communfd,ReturnVal,BUF_MAX*sizeof(int));

	TheradLog(ReturnVal,(col+1)*row +2*col,col,row );
}

/*Write the log file to matrixes and etc.*/
void TheradLog(int *all, int size,int col, int row )
{
	FILE *output;
	char fileName[BUF_MAX];
	int i=0;
	int neS = 3*col+4*row+col*row+6;
	sprintf(fileName,"log/%d.log",mypid);
	output = fopen(fileName,"a");
	fprintf(output,"\nA ={ ");
	while(i< size)
	{
		if(i<col*row)
		{	
			fprintf(output,"%d ",all[i]);
			if(i%row==0)
			{
				fprintf(output,"\n");
			}
		}
		if(i==col*row)
		{
			fprintf(output,"}\nB ={ %d ",all[i]);
		}

		if(i>col*row && i<col*row+row)
		{
			fprintf(output,"%d ",all[i]);			
		}
		if(i==(col+1)*row)	
		{
			fprintf(output, "}\n x1 ={ %d ",all[i]);
		}

		if(i>(col+1)*row && i< (col+2)*row)
		{
			fprintf(output,"%d ",all[i]);
		}

		if(i==(col+2)*row)	
		{
			fprintf(output, "}\n x2 ={ %d ",all[i]);
		}

		if(i>(col+2)*row && i< (col+3)*row)
		{
			fprintf(output,"%d ",all[i]);
		}

		if(i==(col+3)*row)	
		{
			fprintf(output, "}\n x3 ={ %d ",all[i]);
		}

		if(i> (col+3)*row )
		{
			fprintf(output,"%d ",all[i]);
		}

		i++;		
	}
	fprintf(output, "}\ne1 ={ %d ",all[i]);

	while(i < neS )
	{
		if(i< size+col)
		{
			fprintf(output,"%d ",all[i]);
		}	

		if(i == size+col )
		{
			fprintf(output, "}\ne2 ={ %d ",all[i]);
		}	

		if(i > size+col && i< size+col*2)
		{
			fprintf(output,"%d ",all[i]);
		}	
		if(i == size+col*2 )
		{
			fprintf(output, "}\ne3 ={ %d ",all[i]);
		}

		if(i> size+col*2 && i> size+col*3 )
		{
			fprintf(output,"%d ",all[i]);
		}

		if(i == size+col*2 )
		{
			fprintf(output, "}\n en1 = %d.%d",all[i],all[i+1]);
			i++;
		}
		if(i== neS-3)
		{
			fprintf(output, "\n en2 = %d.%d",all[i],all[i+1]);
			i++;
		}
		if(i== neS-2)
		{
			fprintf(output, "\n en3 = %d.%d",all[i],all[i+1]);
			i++;
		}	
		i++;
	}	


	fclose(output);	
}

void logCreate()
{
	FILE *output;
	char fileName[BUF_MAX],s[10];
	int i;
	double time=0,x,temp=0;

	sprintf(fileName,"log/%d.log",mypid);
	output = fopen(fileName,"a");

	for (i = 0; i < counter; ++i)
	{
		time += connectTime[i];
		temp += connectTime[i]*connectTime[i];
	}
	counter++;
	x = time/counter;
	temp = (temp - counter*x)/(counter-1);
	if(temp <0)
	{
		temp *=-1;
		temp = sqrt(temp);
		sprintf(s,"%.4fi",temp);
	}
	else	
	{
		temp = sqrt(temp);
		sprintf(s,"%.4f",temp);
	}

	fprintf(output,"\n\n x̄=%.3f , s=%s \n",x,s);

	fclose(output);
	free(connectTime);
}

void SignalHandler(int signo)
{
	logCreate();
    kill(mypid, SIGINT);
    exit(signo);
}
