#ifndef THREADPOOL_H
#define THREADPOOL_H
#include<pthread.h>
#include<stdint.h>
struct thread_pool {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	unsigned int thread_count;
	unsigned int queue_size;
	unsigned int queue_head;
	unsigned int task_count;
	unsigned long total_count;
	unsigned long drop_count;
	unsigned int  tid;
	void **queue;
	void (*process_func)(void *,uint32_t tid);
};

void add_queue_task(struct thread_pool *my_thread_pool, void *task_arg);
void *fetch_queue_task(void *arg);
void start_thread(void *(*myfunc)(void*), void *arg);
struct thread_pool * init_thread_pool(unsigned int thread_count, unsigned int queue_size, void (*process_func)(void *,uint32_t tid));
#endif
