#include "SAK_threadpool.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <assert.h>

/*
* Each task in the thread pool is a worker, it contains a callback function
* representing the task and the releated params and user data. 
*/
typedef struct __worker_t{
	process_cb_t cb;
	void * arg;
	TAILQ_ENTRY(__worker_t) link; 
}worker_t;


/*
* Thread pool
*/
typedef struct __thread_pool_t{
	pthread_mutex_t   queue_lock;
	pthread_cond_t    queue_ready;
	TAILQ_HEAD(, __worker_t) worker_queue;
	int               max_thread_num;    
	int               shutdown;         /* whether destory the thread pool or not */	
	pthread_t         * thread_id;   
}thread_pool_t;



/*
*   All the thread will call this function when inited.
*   In this function, each thread will get a worker from 
*   the queue and execute it. If the queue is empty, the
*   thread will wait for cont singal.
*/
void * thread_routine(void * arg)
{
	thread_pool_t * pool = (thread_pool_t *)arg;
	for(;;)
	{	
		/* lock the queue */
		pthread_mutex_lock(&(pool->queue_lock));
		
		/**
		* if thread pool is not destroied and current worker queue is empty then 
		* the thread need to wait and be blocked
		*/
		while(TAILQ_EMPTY(&(pool->worker_queue)) && !pool->shutdown)
		{
			pthread_cond_wait(&(pool->queue_ready), &(pool->queue_lock));
		}
		
		/* if the pool will be destroied soon */
		if(pool->shutdown)
		{
			pthread_mutex_unlock(&(pool->queue_lock));
			pthread_exit(NULL);
		}

		assert(!TAILQ_EMPTY(&(pool->worker_queue)));
		
		worker_t * work = TAILQ_FIRST(&(pool->worker_queue));
		TAILQ_REMOVE(&(pool->worker_queue), work, link);
		pthread_mutex_unlock(&(pool->queue_lock));

		/* callback function*/
		(work->cb)(work->arg);
		free(work);
		work = NULL;
	}
}


/**
 * create threadpool handler
 */
SAK_threadpool_t * threadpool_create(int max_thread_num)
{
	assert(max_thread_num >= 0);
	thread_pool_t * pool = (thread_pool_t *)malloc(sizeof(thread_pool_t));

	pthread_mutex_init(&(pool->queue_lock), NULL);
	pthread_cond_init(&(pool->queue_ready), NULL);

	TAILQ_INIT(&(pool->worker_queue));
	pool->max_thread_num = max_thread_num;
	pool->shutdown = 0;

	pool->thread_id = (pthread_t *)malloc(max_thread_num * sizeof(pthread_t));
	int i = 0;
	for(i = 0; i < max_thread_num; i++)
	{
		/* create threads */
		pthread_create(&(pool->thread_id[i]), NULL, thread_routine, (void *)pool);
	}
	return (SAK_threadpool_t *)pool;
}


/**
 * destroy a threadpool
 */
void threadpool_destroy(SAK_threadpool_t * handler)
{
	assert(handler != NULL);
	thread_pool_t * pool = (thread_pool_t *)handler;

	int i = 0;
	/* in case destroy twice */	
	if(pool->shutdown)
	{
		return;
	}

	pool->shutdown = 1;
	
	/* wake up all the threads and tell them we will be destroied */
	pthread_cond_broadcast(&(pool->queue_ready));
	
	for(i = 0; i < pool->max_thread_num; i++)
	{
		pthread_join(pool->thread_id[i], NULL);
	}
	free(pool->thread_id);

	/* destroy the woker queue */
	worker_t * first = TAILQ_FIRST(&(pool->worker_queue));
	worker_t * tmp = NULL;
	while(first != NULL)
	{
		tmp = TAILQ_NEXT(first, link);
		free(first);
		first = tmp;
	}

	/* destroy mutex and cont */
	pthread_mutex_destroy(&(pool->queue_lock));
	pthread_cond_destroy(&(pool->queue_ready));
	free(pool);
	pool = NULL;
	return;
}


void threadpool_put(SAK_threadpool_t * handler, process_cb_t cb, void * arg)
{
	assert(handler != NULL);
	thread_pool_t * pool = (thread_pool_t *)handler;

	/* build a new worker */
	worker_t * new_worker = (worker_t *)malloc(sizeof(worker_t));
	new_worker->cb = cb;
	new_worker->arg = arg;

	/* lock the queue */
	pthread_mutex_lock(&(pool->queue_lock));
	
	/* insert the worker to the tail of the queue */
	TAILQ_INSERT_TAIL(&(pool->worker_queue), new_worker, link);
	
	/*
	*  since the queue has a worker, we can wake up a thread.
	*  if all the threads are buzy, it doesn't matter
	*/

	pthread_cond_signal(&(pool->queue_ready));

	/* unlock the queue lock */
	pthread_mutex_unlock(&(pool->queue_lock));
	return;
}
