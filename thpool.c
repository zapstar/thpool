#include <stdlib.h>     /* malloc(), free() */
#include <pthread.h>    /* pthread_*() */
#include "thpool.h"

/**
 * Describes a task - Also a singly linked list element
 */
typedef struct _task_t {
    void (*func)(void *);
    void *arg;
    struct _task_t *next;
} task_t;

/**
 * Describes the job queue
 */
typedef struct _task_queue_t {
    task_t *head;
    task_t *tail;
    pthread_mutex_t queue_mutex;
} task_queue_t;

/**
 * Thread pool structure
 *
 * NOTE: The thpool.h header typedefs _thpool_t to thpool_t
 */
struct _thpool_t {
    int size;                       /* Size of the thread pool */
    int shutdown;                   /* If 1 then then pool should terminate */
    pthread_t *threads;             /* Thread IDs of all threads */
    task_queue_t queue;             /* Job Queue */
    pthread_mutex_t pool_mutex;     /* Thread pool mutex */
    pthread_cond_t  pool_cond;      /* Thread pool condition variable */
};

/**
 * Is the task queue empty?
 * @param  queue The task queue on which the question is being asked
 * @return       0 if not empty, 1 if empty
 */
static int task_queue_empty(task_queue_t *queue) {
    return queue->head == NULL;
}

/**
 * Add an entry onto the task queue
 * @param queue Queue
 * @param task  Task
 */
static void task_queue_add(task_queue_t *queue, task_t *task) {
    if (task_queue_empty(queue)) {
        queue->head = task;
        queue->tail = task;
    } else {
        queue->tail->next = task;
        queue->tail = task;
    }
    task->next = NULL;
}

/**
 * Pull the first task from the queue
 * @param  queue Queue
 * @return       Task if avaiable, else NULL
 */
static task_t *task_queue_pull(task_queue_t *queue) {
    task_t *cur = queue->head;
    if (queue->head == queue->tail) {
        queue->head = queue->tail = NULL;
    } else {
        queue->head = cur->next;
    }
    return cur;
}

/**
 * Thread pool thread function
 * @param  arg The thread pool handler
 * @return     NULL
 */
static void * thpool_thread(void *arg) {
    task_t *task;
    thpool_t *pool = (thpool_t *) arg;
    for (;;) {
        /* Wait on the condition variable till work comes or asked to kill */
        pthread_mutex_lock(&pool->pool_mutex);
        while (task_queue_empty(&pool->queue) && !pool->shutdown) {
            pthread_cond_wait(&pool->pool_cond, &pool->pool_mutex);
        }

        /* Check for exit conditions */
        if (pool->shutdown) {
            break;
        }

        /* Grab available task */
        task = task_queue_pull(&pool->queue);

        /* Unlock */
        pthread_mutex_unlock(&pool->pool_mutex);

        /* Work if present */
        if (task) {
            (*(task->func))(task->arg); /* NOTE: User has to free the arg! */
        }

        /* Destroy the task */
        free(task);
    }
    pool->size--;
    pthread_mutex_unlock(&pool->pool_mutex);

    /* Quit */
    return NULL;
}


/**
 * Create a thread pool
 * @param  size Size of thread pool (number of threads)
 * @return      Thread pool handler
 */
thpool_t *thpool_create(int size) {
    int i, rc = 0;
    thpool_t *pool;

    /* Allocate memory for the pool */
    pool = malloc(sizeof(thpool_t));
    if (!pool) {
        goto error_c0;
    }

    /* The pool should not be shutdown */
    pool->shutdown = 0;

    /* Initialize an empty queue */
    pool->queue.head = NULL;
    pool->queue.tail = NULL;

    /* Allocate space to keep track of thread IDs of threads in the pool */
    pool->threads = malloc(sizeof(pthread_t) * size);
    if (!pool->threads) {
        goto error_c1;
    }

    /* Initialize the locks, condition variables for the thread pool */
    rc = pthread_mutex_init(&pool->pool_mutex, NULL);
    if (rc != 0) {
        goto error_c2;
    }
    rc = pthread_cond_init(&pool->pool_cond, NULL);
    if (rc != 0) {
        goto error_c3;
    }

    /* Finally create the threads */
    pthread_mutex_lock(&pool->pool_mutex);
    for (i = 0; i < size; i++) {
        rc = pthread_create(&pool->threads[i], NULL, thpool_thread, pool);
        if (rc != 0) {
            pthread_mutex_unlock(&pool->pool_mutex);
            goto error_c4;
        }
        pool->size++;
    }
    pthread_mutex_unlock(&pool->pool_mutex);

    return pool;

error_c4:
    pthread_cond_destroy(&pool->pool_cond);
error_c3:
    pthread_mutex_destroy(&pool->pool_mutex);
error_c2:
    free(pool->threads);
error_c1:
    free(pool);
error_c0:
    return NULL;
}

/**
 * Destroy the thread pool, remove jobs from the queue, kill all threads
 * @param pool Thread pool
 */
void thpool_destroy(thpool_t *pool) {
    int i, error = 0, cur_size;
    task_t *cur, *next;

    /* Dequeue all jobs */
    pthread_mutex_lock(&pool->pool_mutex);
    cur = pool->queue.head;
    while (cur != NULL) {
        next = cur->next;
        free(cur);
        cur = next;
    }
    pool->queue.head = NULL;
    pool->queue.tail = NULL;

    /* Mark all threads to die */
    pool->shutdown = 1;
    /* Keep spinning here till all threads die */
    while (pool->size > 0) {
        cur_size = pool->size;
        pthread_cond_broadcast(&pool->pool_cond);
        pthread_mutex_unlock(&pool->pool_mutex);
        for (i = 0; i < cur_size; i++) {
            
            if (pthread_join(pool->threads[i], NULL) != 0) {
                error = 1;
                break;
            }
        }
        /* Go out of this loop if errored, we can't do much now */
        if (error) {
            break;
        }
        pthread_mutex_lock(&pool->pool_mutex);
    }
    pthread_mutex_unlock(&pool->pool_mutex);

    /* Free everything else */
    pthread_cond_destroy(&pool->pool_cond);
    pthread_mutex_destroy(&pool->pool_mutex);
    free(pool->threads);
    free(pool);
}

/**
 * Add some work to the thread pool
 * @param pool Thread pool
 * @param func Function pointer to the work routine
 * @param arg  Pointer to the argument to the work routine
 */
int thpool_add(thpool_t *pool, void (*func)(void *), void *arg) {
    /* Create a queue entry */
    task_t *task;

    task = malloc(sizeof(task_t));
    if (!task) {
        return -1;
    }
    task->func = func;
    task->arg = arg;

    /* Take the lock, see if state of thread pool is OK? */
    pthread_mutex_lock(&pool->pool_mutex);
    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->pool_mutex);
        free(task);
        return -1;
    }

    /* Add a entry onto the queue and signal the guy to work on this */
    task_queue_add(&pool->queue, task);
    pthread_cond_signal(&pool->pool_cond);
    pthread_mutex_unlock(&pool->pool_mutex);

    return 0;
}
