#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>

#define NO_OF_THREADS 3
#define MAX_WORDS 1024
#define WORD_LENGTH 20

char  	inputFile[100];
int   	wordCount = 1;
char* 	wordList[MAX_WORDS];
//Adding thing a 

void* 	countWordFrequency(void *);
void 	countWord(char * filename);
void 	loadWordstoArray(char * filename);
void 	threadTask();
void 	consolidateResult();
void 	cleanupTask();


pthread_mutex_t lock;
pthread_t 		thread[NO_OF_THREADS];

struct 	sharedWordStruct{
		char word[WORD_LENGTH];
		int  frequency;
	};

struct sharedWordStruct sharedObj[MAX_WORDS];
int sharedIndex = 0;

void main(void)
{
	printf("%s","Enter the filename:");
	scanf("%s",inputFile);	

	countWord(inputFile);
	if (wordCount > 1 && wordCount <= MAX_WORDS) 
	{
		
		loadWordstoArray(inputFile);
		threadTask();  
		consolidateResult();	
    	cleanupTask();
	}
	else
	{
		printf("%s\n","***ERROR*** File is either too small/large for further processing!");
		exit(EXIT_FAILURE);
	}
}

void countWord(char * filename)
{
	FILE *readFilePtr;
	char c;
	printf("\n%s","1. Counting number of words in the file");
	readFilePtr = fopen(filename,"r");
	if(readFilePtr == NULL)
	{
		printf("%s\n","***ERROR*** File cannot be opened!");
		exit(EXIT_FAILURE);
	}
	else
	{
		while((c = fgetc(readFilePtr)) != EOF) 
		{
        if(c == ' ' || c == '\n' || c == '\t')
            wordCount++;
    	}
	}
	//printf("Total number of words:%d\n", wordCount);	
}

void loadWordstoArray(char * filename)
{
	FILE *readFilePtr;
	int size = 0;
	int i ,j = 0;
	
	printf("\n%s","2. Loading words from the file");	

	readFilePtr = fopen(filename,"r");
	if(readFilePtr == NULL)
	{
		printf("%s\n","***ERROR*** File cannot be opened!");
		exit(EXIT_FAILURE);
	}
	else
	{
		 for (i =0; i < wordCount; ++i) 
			{   				
				wordList[i] = malloc(WORD_LENGTH); 
    			fscanf (readFilePtr, "%s", wordList[i]);
    		}
	}
}

void* countWordFrequency(void * tid)
{
	
	int thread_id = * (int*) tid;
	int words_per_partition, start_index,end_index,spill_over = 0;
    
    words_per_partition = wordCount / NO_OF_THREADS;

	start_index= thread_id * words_per_partition;
	end_index = start_index + (words_per_partition - 1);

	if(thread_id == (NO_OF_THREADS-1))
	{
		spill_over = wordCount % NO_OF_THREADS;
		if(spill_over>=1)
		{
			start_index= thread_id * words_per_partition;
			end_index = start_index + (words_per_partition + spill_over - 1);	
			words_per_partition  = words_per_partition + spill_over;	
		}
	}
	

	/*printf("Thread id:	%d\n",thread_id);
	printf("words_per_partition:	%d\n", words_per_partition);
	printf("SpillOver:	%d\n", spill_over);*/
	printf("Thread %d - Start_index: %d End_index: %d\n",thread_id+1,start_index,end_index);
	
	struct wordStruct{
		char word[WORD_LENGTH];
		int  frequency;
	};

	struct wordStruct obj[words_per_partition];

	int i,j;
	int index=0;

	for(i=start_index;i<=end_index;i++)
	{
		int foundIndex = -1;
		for(j=0;j<index;j++)
		{
			if(strcasecmp(obj[j].word, wordList[i])==0){
				foundIndex = j;
				break;
			}
		}
		if(foundIndex == -1 ){
			strcpy(obj[index].word,wordList[i]);
			obj[index].frequency = 1;
			index++;
		} else {
			obj[foundIndex].frequency++;
		}
	}	
	
	/*writing to the shared location*/
	pthread_mutex_lock(&lock);
	printf("\tThread %d entered critical region\n", thread_id+1);
	for(i=0;i<index;i++)
	{
		strcpy(sharedObj[sharedIndex].word,obj[i].word);
		sharedObj[sharedIndex].frequency = obj[i].frequency;
		sharedIndex++;
	}

	pthread_mutex_unlock(&lock);
	printf("\tThread %d exited critical region\n", thread_id+1);
	free(tid);

}

void threadTask()
{
	int i;

	printf("%s\n","3. Initailzing the mutex");
	if(pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("%s\n","***ERROR*** Mutex Initialization error!");
        exit(EXIT_FAILURE);
    }

    printf("%s\n","4. Thread creation and execution");
    for(i=0;i<NO_OF_THREADS;i++)
    {
       	 int *temp = malloc(sizeof(*temp));
    	 *temp = i;
    	 if(pthread_create(&thread[i], NULL, countWordFrequency, (void *)temp)!=0)
    	 {
    	 	printf("%s\n","***ERROR*** Thread creation failed!");
    	 	exit(EXIT_FAILURE);
    	 }                  
    }

    for (i=0;i<NO_OF_THREADS;i++)
    {
    	pthread_join(thread[i], NULL);
    }
}

void consolidateResult()
{
	int i=0; 
	int index = 0;
	int j=0;

	struct wordStruct{
		char word[WORD_LENGTH];
		int  frequency;
	};

	struct wordStruct obj[100];

	printf("%s\n","5. Main thread consolidating the result");
	/*printf(" Shared Index:%d\n",sharedIndex);
	for(i=0;i<sharedIndex;i++)
	{
		 printf(" Word is: %s \n", sharedObj[i].word);
         printf(" Frequency is: %d \n", sharedObj[i].frequency);
	}*/

	for(i=0;i<sharedIndex;i++)
	{
		int foundIndex = -1;
		for(j=0;j<index;j++)
		{
			if(strcasecmp(obj[j].word, sharedObj[i].word)==0){
				foundIndex = j;
				break;
			}
		}
		if(foundIndex == -1 ){
			strcpy(obj[index].word,sharedObj[i].word);
			obj[index].frequency = sharedObj[i].frequency;
			index++;
		} else {
			obj[foundIndex].frequency += sharedObj[i].frequency;
		}
	}	

	 printf("\n-------------------------------------\n");
	 printf("%s\t\t%s\n","Word","Frequency");
	 printf("-------------------------------------\n");
	 
	for(i=0;i<index;i++)
	{
		 printf("%s\t\t", obj[i].word);
         printf("%d\n", obj[i].frequency);
	}

}

void cleanupTask()
{
	pthread_mutex_destroy(&lock);
	int i;
	for (i =0; i < wordCount; ++i)
   		free (wordList[i]); 

   		printf("%s\n","Memory free!");
}
