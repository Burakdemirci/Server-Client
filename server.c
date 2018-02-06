/*
* Burak DEMİRCİ
* 141044091
*/

#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "uici.h"

#define BUF_MAX  1024
#define PERM (IPC_CREAT | 0666)
#define MAT_SIZE 100

int mypid;
int clientPid[100];
int connected =0,threN=0;
int martixR,martixC;
int temp_pid;
int listenfd;
static sem_t sharedsem;
pthread_mutex_t lock; 
int screenT =0;
int workerTh=0;

typedef struct clientT
{
	int communfd;
}client_t;

typedef struct calcT
{
	int *matrix;
	int *result;
	int shmid2;
	int col,row;
}calc_t;

void SignalHandler(int signo);
void thread_per_request(char const *argv[]);
void worker_pool(char const *argv[]);
void SignalHandler(int signo);
void clientP(int pid);
void parser(char *sendBuff);
void Connection(void *n);
void calculate(int col,int row,int communfd);
void randoMatrix(int col,int row, int* data);
int  detachandremove(int shmid, void *shmaddr) ;
void Calculate1(void *n);
void errorCheck(int *result,int *data,int communfd, int col, int row );
void WriteLog(int *all, int size,int col, int row );

int main(int argc, char const *argv[])
{
   int i;
   for (i = 0; i < 100; ++i)
   		clientPid[i]=-1;

   mypid=(int)getpid();
   /*Signal control*/
   signal (SIGINT, SignalHandler);
   
   if(argc == 2)
   {
   		thread_per_request(argv);
   }
   else if(argc == 3)
   {
   		worker_pool(argv);
   }
   else
   {
   		fprintf(stderr, "Usage for worker_pool : %s <port #, id> <thpool size, k > \n", argv[0]);
   		fprintf(stderr, "Usage for thread_per_request : %s <port #, id> \n", argv[0]);
   		return 1;
   }	
	return 0;
}

void thread_per_request(char const *argv[])
{
	u_port_t portnumber;
	int i,error;
	int communfd_r;
	char client[MAX_CANON];
	char sendBuff[BUF_MAX];
	int bytes_copied;	
	pthread_t tid[BUF_MAX];
	client_t t;

	portnumber = (u_port_t) atoi(argv[1]);

	if ((listenfd = u_open(portnumber)) == -1) {
		perror("Failed to create listening endpoint");
		return 1;
	}
	/*Create semaphore*/
	if (sem_init(&sharedsem, 0, 1) == -1)
        return -1;

	while(1) 
	{
        
        if ((communfd_r = u_accept(listenfd, client, MAX_CANON)) == -1) 
        {
        	perror("Accept failed");
        	return -1;	
        }
        screenT++;
        printf("Anlık hizmet verilen client : %d\n",screenT);

     	t.communfd=communfd_r;
     	if (error = pthread_create(tid+threN, NULL, Connection,(void*)&t)) 
		{
		    printf("Exit condition: eror\n");
		    fprintf(stderr, "Failed to create thread %d: %s\n",
		    threN, strerror(error));
		}
		threN++;		
    }
    
    for(i=0; i<threN; i++)
    {
		if (error = pthread_join(tid[i], NULL));
	}	
}

void Connection(void *n)
{
	client_t *t =(client_t*)n;
	int communfd = t->communfd;
	char buff[BUF_MAX];
	/*Critical section*/
	while (sem_wait(&sharedsem) == -1)
        if (errno != EINTR)
          return -1;

	read(communfd,buff,BUF_MAX);
	parser(buff);
	clientP(temp_pid);
 
	calculate(martixC,martixR,communfd);

	screenT--;
	workerTh--;
	if (sem_post(&sharedsem) == -1);
}


void worker_pool(char const *argv[])
{
	u_port_t portnumber;
	int i,error;
	int communfd_r;
	char client[MAX_CANON];
	char sendBuff[BUF_MAX];
	int bytes_copied;	
	pthread_t tid[BUF_MAX];
	client_t t;
	int maxTr = argv[2];

	portnumber = (u_port_t) atoi(argv[1]);

	/*Open the port*/
	if ((listenfd = u_open(portnumber)) == -1) {
		perror("Failed to create listening endpoint");
		return 1;
	}

	/*Create semaphore*/
	if (sem_init(&sharedsem, 0, 1) == -1)
        return -1;

	while(1) 
	{
        if(maxTr >= workerTh)
        {

	        if ((communfd_r = u_accept(listenfd, client, MAX_CANON)) == -1) 
	        {
	        	perror("Accept failed");
	        	return -1;	
	        }
	        
	        printf("Anlık hizmet verilen client : %d\n",);
	     	
	     	t.communfd=communfd_r;
	     	if (error = pthread_create(tid+threN, NULL, Connection,(void*)&t)) 
			{
			    printf("Exit condition: eror\n");
			    fprintf(stderr, "Failed to create thread %d: %s\n",
			    threN, strerror(error));
			}
			workerTh++;
			threN++;
		}			
    }
    
    for(i=0; i<threN; i++)
    {
		if (error = pthread_join(tid[i], NULL));
	}	

}
/**
* col : matrix column 
* row : matrix row
* communfd : client communication port
*/
void calculate(int col,int row,int communfd)
{
	pid_t P1,P2,P3;
	int fd[2],i,j,fd1[2],error,fd2[2];
	key_t key=2541,key2=98751;
	int shmid,flag1=0,flag2=0,flag3=0;
	int *data;
	int shmid2;
	int *result;
	int writeCl[BUF_MAX];
	pthread_t tid[BUF_MAX];
	calc_t t;

	if(pipe(fd)==-1 || pipe(fd1)==-1 || pipe(fd2)==-1){
		perror("Error opening pipe\n"); 
		exit(1);
	}

	/*Mutex init*/
	if (pthread_mutex_init(&lock, NULL) != 0)
	{
	    printf("\n mutex init failed\n");
	    return 1;
	}
	/*  create the segment: */
	if ((shmid = shmget(key, BUF_MAX, PERM)) == -1) {
	    perror("shmget");
	    exit(1);
	}

	/*Shared memory using*/
	if ((data = (int*)shmat(shmid, NULL, 0)) == (void *)-1) {
		perror("Failed to attach shared memory segment");
	  	if (shmctl(shmid, IPC_RMID, NULL) == -1)
	    	perror("Failed to  remove memory segment");
	  	exit(1);
	}

	/*  create the segment: for the write result */
	if ((shmid2 = shmget(key2, BUF_MAX, PERM)) == -1) {
	    perror("shmget");
	    exit(1);
	}

	/*Shared memory using*/
	if ((result = (int*)shmat(shmid2, NULL, 0)) == (void *)-1) {
		perror("Failed to attach shared memory segment");
	  	if (shmctl(shmid2, IPC_RMID, NULL) == -1)
	    	perror("Failed to  remove memory segment");
	  	exit(1);
	}
	
	P1=fork();
	if(P1==0)
	{
		randoMatrix(col,row,data);

		flag1=1;
		write(fd[1],&flag1,sizeof(int));	
	
		exit(0);
	}
	
	P2=fork();
	if(P2==0)
	{

		flag1=1;
		read(fd[0],&flag1,sizeof(int));

		t.shmid2=shmid2;
		t.matrix = data;
		t.col=col;
		t.row=row;	
		randoMatrix(col,row,result);

		/*Calculating */
		for(i=0; i<3; i++)
		{
			if (error = pthread_create(tid+i, NULL, Calculate1,(void*)&t)) 
			{
			    printf("Exit condition: eror\n");
			    fprintf(stderr, "Failed to create thread 0 : %s\n", strerror(error));
			}
		}	

		flag2=1;
		write(fd1[1],&flag2,sizeof(int));

		for(i=0; i<3; i++)
		{
			if (error = pthread_join(tid[i], NULL));
		}

		exit(0);
	}

	P3=fork();
	if(P3==0)
	{

		read(fd1[0],&flag2,sizeof(int));
		
		errorCheck(result,data,communfd,col,row);

		flag3=1;
		write(fd2[1],&flag3,sizeof(int));


		exit(0);
	}

	wait(NULL);/*Main parent proccess wait childs*/
	read(fd2[0],&flag3,sizeof(int));
	
	detachandremove(shmid,data);
	detachandremove(shmid2,result);
	pthread_mutex_destroy(&lock); 
}


void Calculate1(void *n)
{
	calc_t *t = (calc_t*)n;
	int *matrixR = (int*)calloc(t->row*t->col,sizeof(int));
	int i,*result;

	/*Shared memory using*/
	if ((result = (int*)shmat(t->shmid2, NULL, 0)) == (void *)-1) {
		perror("Failed to attach shared memory segment");
	  	if (shmctl(t->shmid2, IPC_RMID, NULL) == -1)
	    	perror("Failed to  remove memory segment");
	  	exit(1);
	}

	pthread_mutex_lock(&lock);

	for ( i = 0; i < t->row*t->col; ++i)
	{
		matrixR[i] =t->matrix[i];
	}
	randoMatrix(t->col,t->row,result);	
	memcpy(result,matrixR,18);

	pthread_mutex_unlock(&lock);
	
	free(matrixR);
}


/* Parse the client pid martix column and row*/
void parser(char *sendBuff)
{
	int i=0,j=0,k;
	char *token;

	token = strtok(sendBuff,",");
	temp_pid = atoi(token);

	token = strtok(NULL,",");
	martixC = atoi(token);

	token = strtok(NULL,",");
	martixR = atoi(token);
}
/*Save connected cliend pid*/
void clientP(int pid)
{
	int i,flag=0;

	for (i = 0; i <connected; ++i)
	{
		if(clientPid[i]==pid)
			flag=-1;
	}

	if(flag!=-1)
	{
		clientPid[connected]=pid;
		connected++;
	}
}
/*Matrix generate*/
void randoMatrix(int col,int row, int* data)
{
	/*Ikı boyutta arraye matris uretme fonksiyonu */
	int *matrix = (int *)calloc(col*row*2,sizeof(int));
	int i=0,j=0;

	srand(mypid*(int)pthread_self());
	while(i < col*row*2)
	{
		matrix[i]= rand()%15;
		i++;
	}
	memcpy(data,matrix,col*row*2*sizeof(int));
	free(matrix);
}

/*Bu fonksiyon kitaptan alınmıştır */
int detachandremove(int shmid, void *shmaddr) 
{
   int error = 0; 

   if (shmdt(shmaddr) == -1)
      error = errno;
   if ((shmctl(shmid, IPC_RMID, NULL) == -1) && !error)
      error = errno;
   if (!error)
      return 0;
   errno = error;
   return -1;
}

/*Handle the signal*/
void SignalHandler(int signo)
{
	int i;

	for (i = 0; i < connected; ++i)
	{
		kill(clientPid[i],SIGINT);
	}
	close(listenfd);
    kill(mypid, SIGINT);
    exit(signo);
}

void errorCheck(int *result,int *data,int communfd, int col, int row )
{

	int  a[MAT_SIZE][MAT_SIZE];
	int  x1[MAT_SIZE],x2[MAT_SIZE],x3[MAT_SIZE],b[MAT_SIZE];
	int  e1[MAT_SIZE], e2[MAT_SIZE], e3[MAT_SIZE];
	int  multiply1[MAT_SIZE],multiply2[MAT_SIZE],multiply3[MAT_SIZE];
	int  i,j,k=0,m,total1,total2,total3;
	int  r1=0,r2=0,r3=0;
	int  all[BUF_MAX],sent[BUF_MAX];
	double res1,res2,res3;

	
	for(i=0; i<row; i++)
	{	
		for(j=0; j<col; j++)
		{
			a[i][j] = data[k];
			all[k] = data[k];
			k++;			
		}	
	}		

	for (i = 0; i < row; ++i)
	{
		b[i]= data[k];
		all[k] = data[k];
		k++;	
	}

	for(i=0; i<col; i++)
	{
		x1[i]= result[i];
		x2[i]= result[i+col];
		x3[i]= result[i+col*2];
	}

	for ( i = 0; i < col*3; ++i)
	{
		all[k] = result[i];
		k++;
	}

	/*Axd multipl*/
	for (i = 0; i < row; ++i)
	{
		total1=0;
		total2=0;
		total3=0;
		for (j = 0; j < col; ++j)
		{
			total1 += a[i][j]*x1[j];
			total2 += a[i][j]*x2[j];
			total3 += a[i][j]*x3[j];
		}
		multiply1[i] = total1;
		multiply2[i] = total2;
		multiply3[i] = total3;
	}

	/*Calculate e */
	for (i = 0; i < row; ++i)
	{
		e1[i] = multiply1[i]-b[i];
		e2[i] = multiply2[i]-b[i];
		e3[i] = multiply3[i]-b[i];
	}

	/*multiply e to t transpoze*/
	for (i = 0; i <row ; ++i)
	{
		r1+= e1[i]*e1[i];
		r2+= e2[i]*e2[i];
		r3+= e3[i]*e3[i];
	}

	/*if the value is negative */
	if(r1<0)
	{
		r1 *=-1;
	}

	if(r2<0)
	{
		r2 *=-1;
	}

	if(r3<0)	
	{
		r3 *=-1;
	}

	res1 = sqrt(r1);
	res2 = sqrt(r2);
	res3 = sqrt(r3);	

	WriteLog(all,(col+1)*row +4*col,col,row );

	for(i=0; i<row; i++)
	{
		all[k]=e1[i];
		k++;
	}	
	for(i=0; i<row; i++)
	{
		all[k]=e2[i];
		k++;
	}	
	for(i=0; i<row; i++)
	{
		all[k]=e3[i];
		k++;
	}
	all[k] = (int)res1;
	all[k+1]= (int)(res1-all[k])*100;
	k = k+2;
	all[k] = (int)res2;
	all[k+1]= (int)(res2-all[k])*100;
	k = k+2;
	all[k] = (int)res3;
	all[k+1]= (int)(res3-all[k])*100;
	/*Write the all data the port */
	write(communfd,all,BUF_MAX*sizeof(int));	
}

void WriteLog(int *all, int size,int col, int row )
{
	FILE *output;
	char fileName[BUF_MAX];
	int i=0;
	sprintf(fileName,"ServerLog/All.log");
	output = fopen(fileName,"a");
	fprintf(output,"A ={ ");
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
	fprintf(output, "}\n");

	fclose(output);	
}