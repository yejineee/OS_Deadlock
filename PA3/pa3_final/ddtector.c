#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <pthread.h>
#include <unistd.h>
#define TRUE 1
#define FALSE 0
void push_ThreadInfo(pthread_t t, pthread_mutex_t *m);
void push_ThreadHold(int idx, pthread_mutex_t *m, pthread_t t);
void push_node(pthread_mutex_t *m);
void push_edge(pthread_mutex_t* src, pthread_mutex_t* dst);
void pop_ThreadHold(pthread_t t, pthread_mutex_t *m);
void pop_MutexAdjList(pthread_mutex_t *m);

typedef struct{
	pthread_mutex_t *mid;
}NODE;

typedef struct{
	pthread_t tid;
}THREADLIST;


static THREADLIST thread_info[10];
static NODE mutex_adjList[100][100];
static NODE thread_hold[10][100];

static NODE visited[100];
static NODE stack[100];

static int vIdx = 0;
static int sIdx = 0;

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

int find_mutex_index(pthread_mutex_t* m){
	int n;
	for(n = 0; n < 100; n++){
		if(mutex_adjList[n][0].mid == m){
			return n;
		}
	}
}

int is_visited(pthread_mutex_t* m){
	int i;
	for(i = 0; i < vIdx; i++){
		if(visited[i].mid == m){
			return TRUE;
		}
	}
	return FALSE;
}
int is_inStack(pthread_mutex_t* m){
	int i;
	for(i = 0; i < sIdx; i++){
		if(stack[i].mid == m){
			return TRUE;
		}
	}
	return FALSE;
}
void pop_stack(pthread_mutex_t* m){
	int i;
	for(i = 0; i < sIdx; i++){
		if(stack[i].mid == m){
			stack[i].mid = 0;
			break;
		}
	}
	sIdx-- ;
}
int check_cycle(int y, pthread_mutex_t* m){
	int x;

	if(!is_visited(m)){
		visited[vIdx++].mid = m;
		stack[sIdx++].mid = m;

		for(x = 1; x < 100; x++){
			if(mutex_adjList[y][x].mid != 0){
				if(!is_visited(mutex_adjList[y][x].mid) &&
				check_cycle(find_mutex_index(mutex_adjList[y][x].mid), mutex_adjList[y][x].mid)){
					return TRUE;
				}
				else if(is_inStack(mutex_adjList[y][x].mid)){
					return TRUE;
				}
			}
		}
	}
	pop_stack(m);
	return FALSE;

}
void is_cycle(void){
	int y;
	int i;
	for(i = 0; i < vIdx; i++){
		visited[i].mid = 0;
	}
	for(i = 0; i < sIdx; i++){
		stack[i].mid = 0;
	}
	vIdx = 0;
	sIdx = 0;

	for(y = 0; y < 100; y++){
		if( mutex_adjList[y][0].mid == 0) continue;
		if(check_cycle(y,mutex_adjList[y][0].mid)){
			printf("<!> Deadlock Detected \n");
			return;
		}
	}
	return;
}



void push_edge(pthread_mutex_t* src, pthread_mutex_t* dst){
	int x, y;
	for(y = 0; y < 100; y++){
		if( mutex_adjList[y][0].mid == src){
			for(x = 1; x < 100; x++){
				if(mutex_adjList[y][x].mid == dst){
					return;
				}
			}
			for(x = 1; x < 100; x++){
				if(mutex_adjList[y][x].mid == 0){
					mutex_adjList[y][x].mid = dst;
					return;
				}
			}
		}
	}
}
void push_node(pthread_mutex_t *m){
	int i;
	for(i = 0; i < 100; i++){
		if (mutex_adjList[i][0].mid == m){
			return;
		}
	}
	for(i = 0; i < 100; i++){
		if(mutex_adjList[i][0].mid == 0){
			mutex_adjList[i][0].mid = m;
			return;
		}
	}
}


void push_ThreadHold(int idx, pthread_mutex_t *m, pthread_t t){
	int i;
	for(i = 0; i < 100; i++){
		if(thread_hold[idx][i].mid == m){ //self deadlock
			printf("<!> DEADLOCK DETECTED - self deadlock\n");
			return;
		}
	}
	for(i = 0; i < 100; i++){
		if (thread_hold[idx][i].mid != 0){
			push_edge(thread_hold[idx][i].mid, m);
		}
	}

	for(i = 0; i < 100; i++){
		if(thread_hold[idx][i].mid == 0){
			thread_hold[idx][i].mid = m;
			break;
		}
	}
	push_node(m);
}
void push_ThreadInfo(pthread_t t, pthread_mutex_t *m){
	int i;
	pthread_t target = t;
	for(i = 0; i < 10; i++){
		if(thread_info[i].tid == target){
			push_ThreadHold(i, m, t);
			return;
		}
	}
	for(i = 0; i < 10; i++){
		if(thread_info[i].tid == 0){
			thread_info[i].tid = target;
			push_ThreadHold(i, m, t);
			return;
		}
	}
}
void pop_MutexAdjList(pthread_mutex_t *m){
	int x, y;
	for(y = 0; y < 100; y++){
		if(mutex_adjList[y][0].mid == m){
			for(x = 0; x < 100; x++){
					mutex_adjList[y][x].mid = 0;
			}
		}
		else{
			for(x = 1; x < 100; x++){
				if(mutex_adjList[y][x].mid == m){
					mutex_adjList[y][x].mid = 0;
					break;
				}
			}
		}
	}
}

void pop_ThreadHold(pthread_t t, pthread_mutex_t *m){
	int idx, x;
	for(idx = 0; idx < 10; idx++){
		if(thread_info[idx].tid == t){
			for(x = 0; x < 100; x++){
				if(thread_hold[idx][x].mid == m){
					thread_hold[idx][x].mid = 0;
					return;
				}
			}
		}
	}
}

int
pthread_mutex_lock (pthread_mutex_t *mutex)
{
	int (*orig_lock)(pthread_mutex_t *mutex);
	int (*orig_unlock)(pthread_mutex_t *mutex);
	char *error;
	orig_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock");
	if((error = dlerror()) != 0x0){
		exit(1);
	}
	orig_unlock = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
	if((error = dlerror()) != 0x0){
		exit(1);
	}
	orig_lock(&mut);
		pthread_t tid = pthread_self();
		push_ThreadInfo(tid, mutex);
		is_cycle();
	orig_unlock(&mut);

	return orig_lock(mutex) ;
}

int
pthread_mutex_unlock (pthread_mutex_t *mutex)
{
	int (*orig_lock)(pthread_mutex_t *mutex);
	int (*orig_unlock)(pthread_mutex_t *mutex);
	char *error;
	orig_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock");
	if((error = dlerror()) != 0x0){
		exit(1);
	}
	orig_unlock = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
	if((error = dlerror()) != 0x0){
		exit(1);
	}
	orig_lock(&mut);
		pthread_t tid = pthread_self();
	  pop_MutexAdjList(mutex);
	  pop_ThreadHold(tid, mutex);
	orig_unlock(&mut);
	return orig_unlock(mutex);
}
