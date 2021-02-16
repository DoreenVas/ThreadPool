//Doreen Vaserman
//308223627

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "threadPool.h"
#include <stdio.h>

#define TRUE 1
#define FALSE 0
#define SUCCESS 0
#define STDERR 2
#define ERROR -1

void error();
void* execute(void* args);

/***
 * creates the thread pool according to the number of thread given.
 * @param numOfThreads
 * @return the created thread pool.
 */
ThreadPool* tpCreate(int numOfThreads){
    ThreadPool* pool = malloc(sizeof(ThreadPool));
    if(pool == NULL)
        error();

    pool->threads=malloc(numOfThreads* sizeof(pthread_t));
    if(pool->threads == NULL){
        free(pool);
        error();
    }

    pool->numOfThreads=numOfThreads;
    pool->stopped=FALSE;
    pool->canInsert=TRUE;
    pool->state=ACTIVE;

    if(pthread_mutex_init(&(pool->queueLock),NULL) != 0 || pthread_mutex_init(&(pool->insertLock),NULL) != 0 ||
     pthread_cond_init(&(pool->cond),NULL) != 0){
        free(pool->threads);
        free(pool);
        error();
    }

    pool->tasksQueue=osCreateQueue();
    if(pool->tasksQueue == NULL){
        free(pool->threads);
        free(pool);
        error();
    }

    int i=0;
    for(i;i<numOfThreads;i++) {
        int thread=pthread_create(&(pool->threads[i]), NULL, execute, (void *) pool);
        if(thread != 0){
            free(pool->threads);
            free(pool);
            error();
        }
    }

    return pool;
}

/***
 * destroys the given thread pool. frees memory. if shouldWaitForTasks=0 we wait for already running tasks.
 * otherwise we wait for running tasks and the tasks in the queue as well.
 * @param threadPool
 * @param shouldWaitForTasks
 */
void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks){
    if(threadPool->canInsert==TRUE){
        pthread_mutex_lock(&threadPool->insertLock);
        if(threadPool->canInsert==TRUE) {
            threadPool->canInsert=FALSE;
            pthread_mutex_unlock(&threadPool->insertLock);
        }
        else{
            pthread_mutex_unlock(&threadPool->insertLock);
            return;
        }
    }else{
        return;
    }

    threadPool->stopped=TRUE;

    if(shouldWaitForTasks == 0){ // wait only for running tasks to finish.
        threadPool->state=WAIT_FOR_RUNNING;
    }
    else{ // wait for running tasks and ones in queue
        threadPool->state=WAIT_FOR_ALL;
    }

    pthread_mutex_lock(&threadPool->queueLock);
    //broadcast unblocks all threads.
    if(pthread_cond_broadcast(&threadPool->cond)!=0 ) {
        error();
    }
    pthread_mutex_unlock(&threadPool->queueLock);


    //join threads
    int i=0;
    for(i;i<threadPool->numOfThreads;i++) {
        pthread_join(threadPool->threads[i],NULL);
    }

    //release resources
    while(!osIsQueueEmpty(threadPool->tasksQueue)){
        Task* task1=osDequeue(threadPool->tasksQueue);
        free(task1);
    }
    pthread_mutex_destroy(&threadPool->queueLock);
    pthread_mutex_destroy(&threadPool->insertLock);
    pthread_cond_destroy(&threadPool->cond);
    free(threadPool->threads);
    osDestroyQueue(threadPool->tasksQueue);
    free(threadPool);
}

/***
 * inserts a task.
 * @param threadPool
 * @param computeFunc
 * @param param
 * @return 0 if succeeded -1 otherwise.
 */
int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param){
    if(threadPool == NULL || computeFunc == NULL || threadPool->canInsert == FALSE )
        return ERROR;

    Task* task=malloc(sizeof(Task));
    if(task == NULL)
        return ERROR;

    task->func=computeFunc;
    task->arg=param;

    pthread_mutex_lock(&threadPool->queueLock);
    osEnqueue(threadPool->tasksQueue,(void*)task);
    if(pthread_cond_signal(&threadPool->cond) != 0)
        error();
    pthread_mutex_unlock(&threadPool->queueLock);

    return SUCCESS;
}

/***
 * executes the tasks in the queue.
 * @param args
 * @return
 */
void* execute(void* args){
    ThreadPool* pool=(ThreadPool*)args;
    while((pool->stopped == FALSE && pool->state == ACTIVE) || (pool->state == WAIT_FOR_ALL && !osIsQueueEmpty(pool->tasksQueue))){
        pthread_mutex_lock(&pool->queueLock);
        if(osIsQueueEmpty(pool->tasksQueue) && pool->stopped == FALSE && pool->canInsert == TRUE){
            pthread_cond_wait(&pool->cond,&pool->queueLock);
            pthread_mutex_unlock(&pool->queueLock);
        }
        else if(!osIsQueueEmpty(pool->tasksQueue)) {
            Task *task = (Task *) osDequeue(pool->tasksQueue);
            pthread_mutex_unlock(&pool->queueLock);
            task->func(task->arg);
            free(task);
        }
        else
            pthread_mutex_unlock(&pool->queueLock);
    }
}

/***
 * error. write to stderr and exit.
 */
void error(){
    char *error="Error in system call";
    write(STDERR, error, sizeof(error));
    exit(ERROR);
}
