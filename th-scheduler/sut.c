#include<stdbool.h>
#include<unistd.h>
#include<stdlib.h>
#include<pthread.h>
#include"queue.h"
#include<ucontext.h>
#include<time.h>
#include"sut.h"
#include<stdio.h>
#include<string.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<fcntl.h>
#define MAX_THREADS				32
#define THREAD_STACK_SIZE			1024*64	
#define BUFFER_SIZE				1024
//one for 2 exec and 0 for one exec
#define EXEC				0
//kernal thread c_exec
pthread_t c_exec;
pthread_t c_exec2;
//kernal thread i_exec
pthread_t i_exec;
//mutexes
pthread_mutex_t c=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t i=PTHREAD_MUTEX_INITIALIZER;

//keep the cexec context
ucontext_t cexec;
ucontext_t cexec2;
ucontext_t iexec;
int r=0;
//number of threadds available
int numthreads;
//check if the read queue is empty or not
int ready_empty=0;
//ready and wait and request queue
struct queue ready;
struct queue iowait;
struct queue request;
struct queue iotodo;
typedef void (*sut_task_f)();
void *Cexec();
void *Iexec();
void *Cexec2();
//threaddesc struct
typedef struct __threaddesc
{
	int threadid;
	char *threadstack;
	void *threadfunc;
	ucontext_t threadcontext;
}threaddesc;
//int MAX_THREADS=30;
//thread array
threaddesc threadarr[MAX_THREADS];
//parent thread
ucontext_t main_cexec;
ucontext_t main_cexec2;
//current context
//ucontext_t curr;
//global variable to return 
int fd;
//toke
char *SPLIT="SPLIT";




void sut_init(){
	numthreads=0;
	pthread_create(&c_exec,NULL,Cexec,NULL);
	pthread_create(&i_exec,NULL,Iexec,NULL);
	ready = queue_create();
	queue_init(&ready);
	iowait = queue_create();
	queue_init(&iowait);
	iotodo = queue_create();
	queue_init(&iotodo);
	getcontext(&main_cexec);
	if(EXEC){
		pthread_create(&c_exec2,NULL,Cexec2,NULL);
	}

}
bool sut_create(sut_task_f fn){
	
	threaddesc *task;
	task = malloc(sizeof(threaddesc));
	//add the desc to array of descriptors
	task = &(threadarr[numthreads]);
	getcontext(&(task->threadcontext));
	task->threadid=numthreads;
	task->threadstack=(char *)malloc(THREAD_STACK_SIZE);
	task->threadcontext.uc_stack.ss_sp=task->threadstack;
	task->threadcontext.uc_stack.ss_size = THREAD_STACK_SIZE;
	task->threadcontext.uc_link = &main_cexec;
	task->threadcontext.uc_stack.ss_flags = 0;
	task->threadfunc = &fn;
	makecontext(&(task->threadcontext),fn,0);
	numthreads++;
	//add the thread to ready queue
	struct queue_entry *t = queue_new_node(&(task->threadcontext));
	pthread_mutex_lock(&c);
	queue_insert_tail(&ready, t);
	pthread_mutex_unlock(&c);
	return 1;
	
	
	
	
	
	
	//char f1stack[16*1024];
	//getcontext(&f1);
	//f1.uc_stack.ss_sp=f1stack;
	//f1.uc_stack.ss_size=sizeof(f1stack);
	//f1.uc_link=&main_c_exec;
	//makecontext(&f1,fn,0);
	//thread_num++;
	
	//struct 	queue_entry *node=queue_new_node(&f1);
	//pthread_mutex_lock(&c_mutex);
	//queue_insert_tail(&ready, node);
	//pthread_mutex_unlock(&c_mutex);
	//ready_empty=1;
	//return 1;
}
void sut_yield(){
	ucontext_t curr;
	getcontext(&curr);
	
	struct queue_entry *t = queue_new_node(&curr);
	pthread_mutex_lock(&c);
	queue_insert_tail(&ready,t);
	pthread_mutex_unlock(&c);
	if(!EXEC){
	swapcontext(&curr,&main_cexec);
	}
	else{
		if(r){
		r--;
		swapcontext(&curr,&main_cexec);
		}
		else{
		r++;
		swapcontext(&curr,&main_cexec2);

		}
	}
	
}
void sut_exit(){
	numthreads--;
	ucontext_t curr;
	getcontext(&curr);
	if(!EXEC){
	swapcontext(&curr,&main_cexec);
	}
	else{
                if(r){
                r--;
                swapcontext(&curr,&main_cexec);
                }
                else{
                r++;
                swapcontext(&curr,&main_cexec2);

                }
        }
	
}
int sut_open(char *dest){
	//get the current context
	
	//printf("alive");
	ucontext_t curr;
	getcontext(&curr);
	//to let I-exec know what to do
	char temp[1024];
	sprintf(temp,"open%s%s",SPLIT,dest);
	//printf("%s",temp);
	char *todo=strdup(temp);
	//printf("%s",todo);
	//add it to queue iotodo
	pthread_mutex_lock(&i);
	struct queue_entry *node=queue_new_node(todo);
	queue_insert_tail(&iotodo,node);
	//printf("%s",(char *)node->data);
	//add the task to waiting list
	struct queue_entry *node2=queue_new_node(&curr);
	queue_insert_tail(&iowait,node2);
	 pthread_mutex_unlock(&i);
	//switch to c-exec to continue
	swapcontext(&curr,&main_cexec);
	//when it comes back to this context return fd
	return fd;


}
void sut_write(int fd, char *buf, int size){
	printf("alivve");
	ucontext_t curr;
	getcontext(&curr);
	char temp[1024];
	sprintf(temp,"write%s%s%s%d",SPLIT,buf,SPLIT,size);
	char *todo=strdup(temp);
	pthread_mutex_unlock(&i);
	struct queue_entry *node=queue_new_node(todo);
	queue_insert_tail(&iotodo,node);
	struct queue_entry *node2=queue_new_node(&curr);
	queue_insert_tail(&iowait,node2);
	pthread_mutex_unlock(&i);
	swapcontext(&curr,&main_cexec);
}
void sut_close(int fd){
	ucontext_t curr;
	//ask ask ask ask can we use this
	getcontext(&curr);

	char temp[1024];
	sprintf(temp,"close%s",SPLIT);
	char *todo=strdup(temp);
	pthread_mutex_unlock(&c);
	struct queue_entry *node=queue_new_node(todo);
	queue_insert_tail(&iotodo,node);
	struct queue_entry *node2=queue_new_node(&curr);
	queue_insert_tail(&iowait,node2);
	pthread_mutex_unlock(&c);
	swapcontext(&curr,&main_cexec);
}
char *sut_read(int fd, char *buf, int size);
void sut_shutdown(){
	pthread_join(c_exec,NULL);
	if(EXEC){
	pthread_join(c_exec2,NULL);
	}
	pthread_join(i_exec,NULL);
	return;
}
void *Iexec(){
	while(true){
	//get what to do from iotodo queue
	//printf("alive");
	struct queue_entry *todo;
	pthread_mutex_lock(&i);
	todo=queue_peek_front(&iotodo);
	struct queue_entry *t;
	t=queue_peek_front(&iowait);
	pthread_mutex_unlock(&i);
	//queue_pop_head(&iotodo);
	//printf("alive");
	//printf("%s",(char*)todo->data);
	//check if there is a thing to do
	if(todo){
		pthread_mutex_lock(&i);
		queue_pop_head(&iotodo);
		pthread_mutex_unlock(&i);
		//check what to do
		char msg[1024];
		//printf("alive");
		char *msg1;
		sprintf(msg,"%s",(char*)todo->data);
		//strcpy(msg1,msg);
		msg1 = strdup(msg);
		char *token1=strtok(msg1,SPLIT);
		//check what the action is
		if(strcmp(token1,"open") == 0){
			char *filename=strtok(NULL,SPLIT);
			int fd =open(filename, O_RDWR, O_CREAT);
		}
		else if(strcmp(token1,"close") == 0){
			close(fd);
		}
		//else if(strcmp(token1,"read") == 0){
		//}
		else if(strcmp(token1, "write") == 0){
			char *towrite=strtok(NULL,SPLIT);
			int size=atoi(strtok(NULL,SPLIT));
			write(fd,towrite,size);

		}
		//get the task we need to go back to
		
		//struct queue_entry *t;
		//t=queue_peek_front(&iowait);
		pthread_mutex_lock(&i);
		queue_pop_head(&iowait);
		queue_insert_tail(&ready,t);
		pthread_mutex_lock(&i);

	}
	else{
	usleep(1000*1000);
	
	}

	}









}
void *Cexec(){
	struct queue_entry *t1;
	ucontext_t curr;
	while(numthreads!=0){
	pthread_mutex_lock(&c);
	t1=queue_peek_front(&ready);
	pthread_mutex_unlock(&c);	
	if(t1){
		pthread_mutex_lock(&c);
		queue_pop_head(&ready);
		pthread_mutex_unlock(&c);	
		curr= *(ucontext_t*) t1->data;
		//ucontext_t t= *(ucontext_t*) t1->data;
		swapcontext(&main_cexec, &curr);
		//swapcontext(&main_cexec, &t);
		
	
	}
	else{
	usleep(1000*1000);
	
	}

	}
}
void *Cexec2(){
	struct queue_entry *t1;
	ucontext_t curr;

	while(numthreads != 0){
        pthread_mutex_lock(&c);
        t1=queue_peek_front(&ready);
        pthread_mutex_unlock(&c);
	if(t1){
                pthread_mutex_lock(&c);
                queue_pop_head(&ready);
                pthread_mutex_unlock(&c);
      
	 	curr= *(ucontext_t*) t1->data;
										     
		swapcontext(&main_cexec2, &curr);
	}
	else{
        usleep(1000*1000);
	}
	}
}








