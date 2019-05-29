#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <pthread.h>
#include <unistd.h>
#define TRUE 1
#define FALSE 0
void printMutexMatrix(void);
void printThreadHold(pthread_t t);
void push_ThreadInfo(pthread_t t, pthread_mutex_t *m);
void push_ThreadHold(int idx, pthread_mutex_t *m, pthread_t t);
void push_node(pthread_mutex_t *m);
void push_edge(pthread_mutex_t* src, pthread_mutex_t* dst);
void pop_ThreadHold(pthread_t t, pthread_mutex_t *m);
void pop_MutexMatrix(pthread_mutex_t *m);

typedef struct{
	pthread_mutex_t *mid;
}NODE;

typedef struct{
	pthread_t tid;
}THREADLIST;


static THREADLIST thread_info[10];
static NODE mutex_matrix[100][100];
static NODE thread_hold[10][100];

static NODE visited[100];
static NODE stack[100];

static int vIdx = 0;
static int sIdx = 0;

static int n_lock = 0;
static int n_unlock = 0;

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

int find_mutex_index(pthread_mutex_t* m){
	int n;
	for(n = 0; n < 100; n++){
		if(mutex_matrix[n][0].mid == m){
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
			if(mutex_matrix[y][x].mid != 0){
				if(!is_visited(mutex_matrix[y][x].mid) &&
				check_cycle(find_mutex_index(mutex_matrix[y][x].mid), mutex_matrix[y][x].mid)){
					return TRUE;
				}
				else if(is_inStack(mutex_matrix[y][x].mid)){
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
		if( mutex_matrix[y][0].mid == 0) continue;
		if(check_cycle(y,mutex_matrix[y][0].mid)){
			printf("Deadlock Detection \n");
			return;
		}
	}
	return;
}

void print_visited(){
	int i;
	printf("vIdx : %d, visited : ",vIdx);
	for(i = 0; i < 100; i++){
		printf("(%d)%u > ", i,visited[i].mid);
	}
	printf("\n");
}


void push_edge(pthread_mutex_t* src, pthread_mutex_t* dst){
	/*
			mutex matrix[i][0] == src 인 곳에
			 	dst가 있는지 확인.
					있다면 -> return;
					없다면 -> 빈 곳에 추가하고 return;
	*/
	int x, y;
	for(y = 0; y < 100; y++){
		if( mutex_matrix[y][0].mid == src){
			for(x = 1; x < 100; x++){
				if(mutex_matrix[y][x].mid == dst){
					return;
				}
			}
			for(x = 1; x < 100; x++){
				if(mutex_matrix[y][x].mid == 0){
					mutex_matrix[y][x].mid = dst;
					return;
				}
			}
		}
	}
}
void push_node(pthread_mutex_t *m){
	/*
	mutex matrix[i][0] search하면서 m과 같은 것이 있는지 확인,
	있다면 return;
	없다면 matrix[i][0] = 0인 곳에 추가.
	*/
	int i;
	// 이미 있는 경우 return;
	for(i = 0; i < 100; i++){
		if (mutex_matrix[i][0].mid == m){
			return;
		}
	}
	// mutex_matrix에 새롭게 추가
	for(i = 0; i < 100; i++){
		if(mutex_matrix[i][0].mid == 0){
			mutex_matrix[i][0].mid = m;
			return;
		}
	}
}


void push_ThreadHold(int idx, pthread_mutex_t *m, pthread_t t){
	/* 이미 thread_hold에 m이 있다면 --> deadlock!
		thread_hold에 없다면, mid 가 빈곳에 넣어줌
	*/
	int i;
	for(i = 0; i < 100; i++){
		if(thread_hold[idx][i].mid == m){ //self deadlock
			printf("DEADLOCK OCCURS\n");
			return;
		}
	}
//	자신의 thread_hold의 모든 node에 대해서 edge 생성
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
	// mutex_matrix에 이 mutex가 없는지 확인하고, 추가.
	push_node(m);
}
void push_ThreadInfo(pthread_t t, pthread_mutex_t *m){
	/* thread_info에 tid가 매칭되는 것을 찾아라.
	if 있다면, 해당 thread_hold로 가서, thread_hold에 m 저장.
	else 없다면, thread_info에 tid 저장하고, thread_hold에 m 저장.
	*/
	int i;
	pthread_t target = t;
	for(i = 0; i < 10; i++){
		if(thread_info[i].tid == target){
			//thread_hold로 가서 thread_hold에 m 저장
			push_ThreadHold(i, m, t);
			return;
		}
	}
	for(i = 0; i < 10; i++){
		if(thread_info[i].tid == 0){ //빈 thread
			//thread_hold로 가서 thread_hold에  m 저장
			thread_info[i].tid = target;
			push_ThreadHold(i, m, t);
			return;
		}
	}
}
void printThreadHold(pthread_t t){
	int idx, j;
	for( idx = 0; idx < 10; idx++){
		if(thread_info[idx].tid == t){
			printf("<tid : %u >",thread_info[idx].tid);
			for(j = 0; j < 100; j++){
				if(thread_hold[idx][j].mid != 0){
					printf(" %u ->", thread_hold[idx][j].mid);
				}
			}
			printf(" NIL\n");
			return;
		}
	}
}

void printMutexMatrix(void){
	int x, y;
	//for( y = 0; y < 100; y++){
	for( y = 0; y < 5; y++){
		printf("%d:[%u] ",y,mutex_matrix[y][0].mid);
		//for( x = 1; x < 100; x++){
		for( x = 1; x < 5; x++){
			printf("'%u' ",mutex_matrix[y][x].mid);
		}
		printf("\n");
	}
}
void pop_MutexMatrix(pthread_mutex_t *m){
	/*

	for y = 0 to 100
		if mm[y][0] == m;
			for x = 0 to x 100 delete all
		else
			for x = 1 to 100
			 if mm[y][x] == m 이면 delete.
	*/
	int x, y;
	for(y = 0; y < 100; y++){
		if(mutex_matrix[y][0].mid == m){
			for(x = 0; x < 100; x++){
					mutex_matrix[y][x].mid = 0;
			}
		}
		else{
			for(x = 1; x < 100; x++){
				if(mutex_matrix[y][x].mid == m){
					mutex_matrix[y][x].mid = 0;
					break;
				}
			}
		}
	}
}

void pop_ThreadHold(pthread_t t, pthread_mutex_t *m){
	/*
		thread_hold에서 해당 mutex 제거
	*/
	int idx, x;
	for(idx = 0; idx < 10; idx++){
		if(thread_info[idx].tid == t){
			for(x = 0; x < 100; x++){
				if(thread_hold[idx][x].mid == m){
					thread_hold[idx][x].mid = 0;
					return;
				}
			}
			printf("error : not found mid in thread_hold\n");
		}
	}
	printf("error : not found tid in thread_info\n");
}

int
pthread_mutex_lock (pthread_mutex_t *mutex)
{
	int (*orig_lock)(pthread_mutex_t *mutex);
	int (*orig_unlock)(pthread_mutex_t *mutex);
	char *error;
	int i;
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
	  pop_MutexMatrix(mutex);
	  pop_ThreadHold(tid, mutex);
	orig_unlock(&mut);
	return orig_unlock(mutex);
}
