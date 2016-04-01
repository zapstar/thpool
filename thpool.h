#ifndef _THPOOL_H_
#define _THPOOL_H_

/**
 * Opaque thread pool handler
 */
typedef struct _thpool_t thpool_t;

/**
 * Create a thread pool
 * @param  size Size of thread pool (number of threads)
 * @return      Thread pool handler
 */
thpool_t *thpool_create(int size);

/**
 * Destroy the thread pool, remove jobs from the queue, kill all threads
 * @param pool Thread pool
 */
void thpool_destroy(thpool_t *pool);

/**
 * Add some work to the thread pool
 * @param pool Thread pool
 * @param func Function pointer to the work routine
 * @param arg  Pointer to the argument to the work routine
 */
int thpool_add(thpool_t *pool, void (*func)(void *), void *arg);

#endif /* _THPOOL_H_ */
