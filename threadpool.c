#include<stdlib.h>
#include<unistd.h> 
#include"threadpool.h"
void start_thread(void *(*myfunc)(void*), void *arg)
{
	pthread_t tid;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	//新线程不能用pthread_join来同步，且在退出时自行释放所占用的资源
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&tid, &attr, myfunc, arg);
	pthread_attr_destroy(&attr);
}
	

void add_queue_task(struct thread_pool *my_thread_pool, void *task_arg)
{
	pthread_mutex_lock(&(my_thread_pool->mutex));
	if (my_thread_pool->task_count == my_thread_pool->queue_size){
		my_thread_pool->drop_count++;
		pthread_mutex_unlock(&(my_thread_pool->mutex));		
		free(task_arg);
		return;
	}
	unsigned int queue_tail = (my_thread_pool->queue_head + my_thread_pool->task_count) % my_thread_pool->queue_size;
	my_thread_pool->queue[queue_tail] = task_arg; //should free(task_arg) in process_func
	my_thread_pool->task_count++;
	my_thread_pool->total_count++;
	pthread_cond_signal(&(my_thread_pool->cond));
	pthread_mutex_unlock(&(my_thread_pool->mutex));
}

void *fetch_queue_task(void *arg)
{
	struct thread_pool *my_thread_pool = (struct thread_pool *)arg;
	uint32_t tid = my_thread_pool->tid;
	
	while(1){
		//lock mutex
		pthread_mutex_lock(&(my_thread_pool->mutex));
		//if no task, thread will unlock mutex and wait cond
		while(my_thread_pool->task_count==0) pthread_cond_wait(&(my_thread_pool->cond), &(my_thread_pool->mutex));
		//auto lock mutex and get task from queue
		void *task = my_thread_pool->queue[my_thread_pool->queue_head];
		my_thread_pool->queue_head++;
		if (my_thread_pool->queue_head == my_thread_pool->queue_size) my_thread_pool->queue_head = 0;
		my_thread_pool->task_count--;
		//unlock mutex
		pthread_mutex_unlock(&(my_thread_pool->mutex));
		//process task no mutex
		my_thread_pool->process_func(task,tid); //should free(task) in process_func
	}
}

struct thread_pool * init_thread_pool(unsigned int thread_count, unsigned int queue_size, void (*process_func)(void *,uint32_t tid))
{
	struct thread_pool *my_thread_pool = (struct thread_pool *)malloc(sizeof(struct thread_pool));
	my_thread_pool->thread_count = thread_count;
	my_thread_pool->queue_size = queue_size;
	my_thread_pool->queue = (void **)malloc(sizeof(void *)*my_thread_pool->queue_size);
	my_thread_pool->task_count = 0;
	my_thread_pool->queue_head = 0;
	my_thread_pool->total_count = 0;
	my_thread_pool->drop_count = 0;
	my_thread_pool->process_func = process_func;
	pthread_mutex_init(&(my_thread_pool->mutex), NULL);
	pthread_cond_init(&(my_thread_pool->cond), NULL);
	for(unsigned int i=0; i<my_thread_pool->thread_count; i++){
		my_thread_pool->tid = i;
		start_thread(fetch_queue_task, my_thread_pool);
		usleep(10000);
	}
	return my_thread_pool;
}
