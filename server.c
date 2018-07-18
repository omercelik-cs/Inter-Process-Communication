#include <sys/types.h>
#include <sys/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <mqueue.h>
#include <pthread.h>

// Author: I. Omer Celik
// ID: 21102647
// server.c


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

// global variables
FILE *fp;

// thread function to process each file
void *foo(void* z){

	char file[128];
	char line[200];
	int q = 0;
	int totalCount = 0;
	
	thd *resultThread =(thd*) z;
	thd *result = malloc(sizeof(thd));
	
	strcpy(file,resultThread->fileName);
	
        fp = fopen(file, "r");
            
        // check if the file is opened
        if(fp == NULL){
       		printf("Can't open the file");
                exit(1);
        }
           
        int lineNumber = 1;
	int k;

	for(k = 0; k<50; k++){
		resultThread->occurences[k] = 0; // safety purpose
	}
	
	// loop through for each line
        while (fgets(line, sizeof(line),fp) != NULL){
		
		int i = 0, j = 0, count = 0;
		char unit[80];
		strdup(line);
               	strtok(line, "\n");

		for (i = 0; i < strlen(line); i++)
    		{
      			while (i < strlen(line) && !isspace(line[i]) && isalnum(line[i]))
       		  	{
           			unit[j++] = line[i++];
       		  	}
      		 	if (j != 0)
       		 	{
            			unit[j] = '\0';
           			if (strcmp(unit, resultThread->keyWord) == 0)
           		 	{
               				count++;
          		  	}
           			j = 0;
       		 	}

		}
		totalCount = totalCount+count;
		count = count+q;
		while(q<count){
			result->occurences[q] = lineNumber;
			q++;
		}

		lineNumber++;	
	}
	
	result->totalNum = totalCount;

	strcpy(result->fileName, resultThread->fileName);

	pthread_exit(result);
}


int main(int argc, char *argv[]) {

	char queueName[128];
	strcpy(queueName, argv[1]);
	msg myMsg;
	unsigned int msg_prio = 0;
	
	int p_id;
	struct mq_attr attr;
    	
	while(1){
		// create child process
		p_id = fork();
		
		// case error
		if(p_id<0){
		    printf("error");
		    exit(0);
		}
		
		// child processes case
		else if(p_id== 0){

			mqd_t mqd;
			mqd_t mqd2;

			int oflag = (O_RDWR|O_CREAT);
	    		mode_t mode = (S_IRUSR|S_IWUSR);

			mqd = mq_open(queueName,oflag , mode , NULL);

			if(mqd<0)
	    		{
				perror("can not open msg queue (server)\n");
				exit(0);
	    		}
		
			// get the maximum size attribute to allocate enough space
			mq_getattr(mqd, &attr);		
			int bufferLength = (int) attr.mq_msgsize;

			// receive the data 
			int r = mq_receive(mqd, (char *)&myMsg, bufferLength, NULL);

			if(r<0)
		 	{
				perror("mq_receive failed server side\n");
				exit(1);
		   	}
		
			// do the neccessary string operations by creating threads for each file
			pthread_t tids[myMsg.N];
			thd *myThd = malloc(sizeof(thd));
			int i = 0;

			strcpy(myThd->keyWord,myMsg.keyWord);
		
			// create N different threads to execute function		
			for (i=0; i<myMsg.N; ++i) {
				strcpy(myThd->fileName , myMsg.fileNames[i]);
				pthread_create(&tids[i], NULL, &foo, (void*) myThd);
				usleep(10000); // just for synchronization purpose
	      		}
			res result;
			thd *retVal = calloc(myMsg.N,sizeof(thd));
		
			// join the threads and get the results
	      		for (i=0; i<myMsg.N; ++i){
		      		pthread_join (tids[i], (void**)&retVal);
				strcpy(result.myThd[i].fileName, retVal->fileName);
				result.myThd[i].totalNum = retVal->totalNum;
				int z = 0;
				while(retVal->occurences[z] !=0 && retVal->occurences[z]<129){
					result.myThd[i].occurences[z] = retVal->occurences[z];
					z++;
				}
			}
		
			//open the client's reply queue using its process ID
			mqd2 = mq_open(myMsg.processID,oflag , mode , NULL);

			if(mqd2<0)
	    		{
				perror("can not open reply queue Please type back slash at the beginning\n");
				exit(0);
		    	}

			//send the result back to the client.
			int s =   mq_send(mqd2, (char *) &result , sizeof(result) , msg_prio);		
			if(s<0)
			{
				perror("mq_send failed (server)\n");
				exit(1);
			}

			// destroy the queues
			mq_close(mqd);
			mq_close(mqd2);
			mq_unlink(myMsg.processID);
			mq_unlink(queueName);
			exit(0);

		}
		else
			wait(NULL);	
	}

	return 0;
}
