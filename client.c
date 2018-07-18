#include <sys/types.h>
#include <sys/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <mqueue.h>

// Author: I. Omer Celik
// client.c

// thread structure
typedef struct threadStructure   
{
	char fileName[128];
	char keyWord[128];
	int occurences[128];	
	int totalNum;
}thd;

// result structure
typedef struct resultStructure{
	thd  myThd[10];
}res;

// message structure
typedef struct msgStructure   
{
    	char queueName[128];
	char fileNames[10][128];
	char keyWord[128];
	int N;
	char processID[128];
}msg;

int main(int argc, char *argv[]) {

	// create request queue to pass the parameters
	mqd_t mqd;
	mqd_t mqd2;
	msg myMsg;
	unsigned int msg_prio = 0;
	char queueName[128];
	strcpy(queueName, argv[1]);
	
	int oflag = O_RDWR|O_CREAT;
	//int oflag2 = O_RDONLY;
    	mode_t mode = S_IRUSR|S_IWUSR;
    	struct mq_attr attr;
	
	// set the request queue message variables.	
	(void) strcpy(myMsg.queueName,argv[1]);
	(void) strcpy(myMsg.keyWord, argv[2]);
	myMsg.N = atoi(argv[3]);
	int i = 0;
	while(i<atoi(argv[3])){
		strcpy(myMsg.fileNames[i],argv[i+4]);
		i++;
	}

	// open request queue
	mqd = mq_open(queueName,oflag , mode , NULL);

    	if(mqd<0)
    	{
        	perror("can not open request queue, type back slash at the beginning\n");
        	exit(0);
    	}
	
	//before sending request queue, create reply queue to get the result.
	int p_id = getpid();
	char str[128];
	sprintf(str, "%d", p_id); // convert p_id to string for reply queue name

	// add '/' to the beginning of the process ID
	char replyQueueName[128];
	
	int k;
	replyQueueName[0] = '/';
	for(k = 1; k<strlen(str)+1; ++k)
		replyQueueName[k] = str[k-1];
	
	(void) strcpy(myMsg.processID, replyQueueName); 
	
	//open the reply queue
	mqd2 = mq_open(replyQueueName,oflag,mode,NULL);

    	if(mqd2<0)
    	{
        	perror("can not open reply queue Please type back slash at the beginning\n");
        	exit(0);
    	}

	// send the request to the server through request queue.
	int s =  mq_send(mqd, (char *) &myMsg , sizeof(myMsg) , msg_prio);
	
	if(s<0)
        {
        	perror("mq_send failed\n");
        	exit(1);
        }
	
	// allocate enough space to receive result
	mq_getattr(mqd, &attr);
	int bufferLength = (int) attr.mq_msgsize;
	res myRes;
	
	// get the result through reply queue and print to the screen
	int r = mq_receive(mqd2, (char *)&myRes, bufferLength, NULL);
	if(r<0)
	{
		perror("mq_receive failed client side\n");
		exit(1);
   	}
	thd resThd[myMsg.N];
	
	int tCounter = 0;
	while(tCounter<myMsg.N){
		resThd[tCounter] = myRes.myThd[tCounter]; // get the final result
		tCounter++;
	}
		
	// print to the screen
	tCounter = 0;
	int oCounter;
	printf("Keyword : \"%s\"\n", myMsg.keyWord);
	while(tCounter<myMsg.N){

		printf("<%s> ", resThd[tCounter].fileName);
		printf("[%d]: " , resThd[tCounter].totalNum );
		oCounter = 0;		
		while(resThd[tCounter].occurences[oCounter]!=0){
			printf("%d ", resThd[tCounter].occurences[oCounter]);	
			oCounter++;
		}
		tCounter++;
		printf("\n");
	}

	//destroy the reply queue and terminate
	mq_close(mqd2);
	mq_unlink(replyQueueName);

	mq_close(mqd);
    	mq_unlink(queueName);

	return 0;
	 
}
