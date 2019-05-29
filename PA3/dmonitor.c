#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <pthread.h>

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

int
pthread_mutex_lock (pthread_mutex_t *mutex)
{
	FILE *fp = NULL;
	fp = fopen("dmonitor.trace","a");
	static __thread int n_lock = 0 ;
	n_lock += 1 ;

	int (*orig_lock)(pthread_mutex_t *mutex);
	int (*orig_unlock)(pthread_mutex_t *mutex);
	char * error ;

	orig_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock");
	if ((error = dlerror()) != 0x0)
		exit(1) ;

	orig_unlock = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
	if((error = dlerror()) != 0x0){
		exit(1);
	}

	if (n_lock == 1) {
		orig_lock(&mut);
		int i ;
		void * arr[100] ;
		char ** stack ;

		pthread_t tid = pthread_self();
		fprintf(fp, "tid<%u>-mid<%u>'\n", tid, mutex);

		size_t sz = backtrace(arr, 100) ;

		stack = backtrace_symbols(arr, sz) ;

		fprintf(fp, "Stack trace\n") ;
		fprintf(fp, "============\n") ;
		for (i = 0 ; i < sz ; i++)
			fprintf(fp, "[%d] %s\n", i, stack[i]) ;
		fprintf(fp, "============\n\n") ;
		orig_unlock(&mut);
	}

	n_lock -= 1 ;
	fclose(fp);
	return orig_lock(mutex) ;
}
