all:
	gcc -o tp_thread_creation TP_thread_creation.c -pthread
	gcc -shared -fPIC -o ddtector.so ddtector.c -ldl
	gcc -o fp_thread_creation FP_thread_creation.c -pthread
	gcc -shared -fPIC -o ddtector.so ddtector.c -ldl
	
	gcc -o self_deadlock self_deadlock.c -pthread
	gcc -shared -fPIC -o ddtector.so ddtector.c -ldl
	gcc -o dinning_deadlock dinning_deadlock.c -pthread
	gcc -shared -fPIC -o ddtector.so ddtector.c -ldl
	gcc -o abba abba.c -pthread
	gcc -shared -fPIC -o ddtector.so ddtector.c -ldl
