#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <pthread.h>
#include <unistd.h>
#define TRUE 1
#define FALSE -1
/*
# thread list
1. thread별로 hold하고자 하는 mutex를 담기.
2. 이미 thread에 있다면, 저장하지 않기
# mutex list
1. 
*/
typedef struct{
	pthread_mutex_t* addr;
	struct NODE *link;
}NODE;

typedef struct{
	pthread_t tid;
//	struct THREADLIST *link;
	NODE* head;
	NODE* tail;
}THREADLIST;
// 배열로 바꾸면 MUTEXLIST Struct 은 필요없어짐.
typedef struct{
	NODE* head;
	struct MUTEXLIST *link;
}MUTEXLIST;

//static MUTEXLIST mutex_list;
static THREADLIST* thread_list[10];
//static NODE mutex_list[100];
static int n_lock = 0;
static int n_unlock = 0;
static pthread_mutex_t MUTEX = PTHREAD_MUTEX_INITIALIZER;
 
NODE* newNode(pthread_mutex_t *item){
	NODE* node = (NODE*)malloc(sizeof(NODE));
	node->addr = item;
	node->link = NULL;
//	printf("newNode-addr : %u\n",node->addr);
	return node;
}

THREADLIST* newThread(pthread_t t, NODE *node){
	THREADLIST* thread = (THREADLIST*)malloc(sizeof(THREADLIST));
	thread->tid = t;
	thread->head = node;
	thread->tail = node;
/*	printf("----newThread----\n");
	printf("nt-tid : %u\n",thread->tid);
	printf("nt-head-node: %u\n",thread->head->addr);
	printf("nt-tail-node: %u\n",thread->tail->addr);
	printf("------------------\n");
*/
	return thread;
}

MUTEXLIST* newMutex(NODE *item){
	MUTEXLIST* mutex = (MUTEXLIST*)malloc(sizeof(MUTEXLIST));
	mutex->head = item;
	mutex->link = NULL;
/*	printf("----newMutex----\n");
	printf("nM-head-node: %u\n",mutex->head->addr);
	printf("------------------\n");
*/
	return mutex;
}

void insertInLockList (NODE* item){

}

void connectWithHold (NODE* hList, NODE* dst){

}

void printThreadList(pthread_t t){
	int i;
	for(i = 0; i < 10; i++){
		if(thread_list[i] != NULL && thread_list[i]->tid == t)		
		{
		printf("%d>,<%u>",i,thread_list[i]->tid);
		printList(thread_list[i]->head);
		return;
		}
	}
}

			
void printList(NODE* head){
	int i ;
	if(head == NULL){
		return;
	}
	NODE* cur = head;
	while(1){
		printf("'%u'",cur->addr);
		if(cur->link != NULL){
			printf(" -> ");
			cur = cur->link;
		}
		else{
			printf(" -> NIL\n");
			return;
		}
	}
}
void delete_mutex_ThreadList(pthread_mutex_t *m){
	int i;
	NODE *prev, *cur, *tmp ;
	for(i = 0; i < 10; i++){
		puts("1");
		if(thread_list[i] != NULL){
			puts("2");
			if(thread_list[i]->head->addr == m){
			//	전체 list 삭제
				cur = thread_list[i]->head;
				while(1){
					tmp = cur;
			puts("4");
					if( cur->link != NULL){
						puts("5");
						free(tmp);
						cur = cur->link;
					}
					else{
						puts("6");
						free(tmp);
						break;
					}
				}
				free(thread_list[i]);
			}
			else{
			/*
			list traverse하면서 addr가 같은 Node는 연결 끊기
			*/
		//	for(i = 0; i < 10; i++){
		//		if(thread_list[i] != NULL){
					puts("7");
					prev = NULL;
					cur = thread_list[i]->head;
					while(cur->addr != m){
						puts("8");
						prev = cur;
						cur = cur->link;
					}
					if(cur == thread_list[i]->tail){
						puts("9"); 
						thread_list[i]->tail = prev;
						prev->link = NULL;
						free(cur);
		//				continue;
					}else{ 
					puts("10");
					prev->link = cur->link;
					free(cur);
					puts("11");
					}
		//	}
			}
		}
	}
}
void push_ThreadList(pthread_t t, pthread_mutex_t *m){
	/* threadlist에 thread 정보가 있는지 확인
	if 있다면, tail에 저장.
	else 없다면, newThread를 저장.
	*/
	int i;
	NODE *new = newNode(m);
	// check thread list if thread is already exist.
	for(i=0; i<10; i++){
	if(thread_list[i] != NULL && thread_list[i]->tid == t){
			thread_list[i]->tail->link = new;
			thread_list[i]->tail = new; 
			return;
		}
	}
	for(i=0; i<10; i++){
		if(thread_list[i] == NULL){
			thread_list[i] = newThread(t, new);
			return;
		}
	}

/*	THREADLIST* current = thread_list;
	if(current == NULL){ 
		thread_list = newThread(t, newNode(m));
		return;
	}		
	while(current != NULL){
		printf("current_tid = %u | t = %u\n",current->tid, t);
		if(current->tid == t){
			current->tail->link = newNode(m);
			current->tail = current->tail->link;
			return;
		}
		current = current->link;
	}
	// thread list 에 없었으니, thread node를 추가한다.
	current = newThread(t, newNode(m));*/
}
	
int
pthread_mutex_lock (pthread_mutex_t *mutex)
{

//static THREADLIST* thread_list[10];
//static NODE mutex_list[100];
//static int n_lock = 0;

	int (*orig_lock)(pthread_mutex_t *mutex); 
	char *error;
	orig_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock");
	if((error = dlerror()) != 0x0){
		exit(1);
	}
//	orig_lock(&MUTEX);
	pthread_t tid = pthread_self();
//	printf("-------------------tid : %u  ",tid);
//	printf("+mutex: %u\n------------------", mutex);	
	n_lock++;
	if( n_lock == 1){
	
	push_ThreadList(tid, mutex);	
	printf("tid: %u =========lock <%u>=========\n", tid,mutex);
	printThreadList(tid);
	}
	n_lock--;
//	printThreadList(tid);
	
	return orig_lock(mutex) ; 	
}

int 
pthread_mutex_unlock (pthread_mutex_t *mutex)
{
	int (*orig_unlock)(pthread_mutex_t *mutex);
	char * error;
	orig_unlock = dlsym(RTLD_NEXT,"pthread_mutex_unlock");
	if((error = dlerror()) != 0x0)
		exit(1);
	n_unlock++;
	pthread_t tid = pthread_self();
	if(n_unlock == 1){
	printf("tid: %u ---unlock: %u---\n",tid, mutex);
	delete_mutex_ThreadList(mutex);
//	sleep(10);
	printThreadList(pthread_self());
	}
	n_unlock--;
	
	return orig_unlock(mutex);
}
