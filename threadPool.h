//Doreen Vaserman
//308223627

#ifndef EX4_THREADPOOL_H
#define EX4_THREADPOOL_H

#include <sys/types.h>
#include "osqueue.h"

enum State {WAIT_FOR_ALL,WAIT_FOR_RUNNING,ACTIVE};

typedef struct thread_pool
{
    OSQueue* tasksQueue;
    int numOfThreads;
    pthread_t* threads;
    int stopped;
    int canInsert;
    pthread_mutex_t queueLock;
    pthread_mutex_t insertLock;
    pthread_cond_t cond;
    enum State state;
}ThreadPool;


typedef struct task
{
    void (*func)(void* arg);
    void *arg;
}Task;

/***
 * creates the thread pool according to the number of thread given.
 * @param numOfThreads
 * @return the created thread pool.
 */
ThreadPool* tpCreate(int numOfThreads);

/***
 * destroys the given thread pool. frees memory. if shouldWaitForTasks=0 we wait for already running tasks.
 * otherwise we wait for running tasks and the tasks in the queue as well.
 * @param threadPool
 * @param shouldWaitForTasks
 */
void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

/***
 * inserts a task.
 * @param threadPool
 * @param computeFunc
 * @param param
 * @return 0 if succeeded -1 otherwise.
 */
int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif //EX4_THREADPOOL_H
