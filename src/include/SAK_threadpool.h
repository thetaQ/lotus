#ifndef __SAK_THREADPOOL_H__
#define __SAK_THREADPOOL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <pthread.h>

typedef struct{
}SAK_threadpool_t;

/* user callback function */
typedef int (*process_cb_t)(void * arg);


/*
* Use:
*   Init the thread pool. We create a thread pool and init the
*   structure in the pool. We create $(max_thread_num) threads.
* Param:
*   max_thread_num is the nunber of max threads in the pool
*   Note that max_thread_num must larger than 0.
* Return:
*   Handler of threadpool
*/
SAK_threadpool_t * threadpool_create(int max_thread_num);


/*
* Use:
*   Destroy a thread pool described by threadpool handler.
* Param:
*   handler is the threadpool you want to destroy
* Return:
*   void
*/
void threadpool_destroy(SAK_threadpool_t * handler);




/*
* Use:
*   We put a worker into the worker queue of the pool.
*   First we create a worker, then we lock the queue and 
*   add the worker to the queue, then we unlock the queue.
*   Last we need to send a singal to wake up a thread.
* Param:
*   handler is the threadpool where you want to put work in
*   cb is a callback function for a worker
*   arg is the param of callback function
* Return:
*   void
*/
void threadpool_put(SAK_threadpool_t * handler, process_cb_t cb, void * arg);

#ifdef __cplusplus
}
#endif

#endif
