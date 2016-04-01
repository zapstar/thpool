# Thread Pool
A no nonsense implementation of a thread pool in C using the pthreads library.

## API
* Opaque thread pool handler `thpool_t`
* Create a new thread pool using `thpool_t *thpool_create(int size)`. Returns `NULL` on failure
* Add an task using `int thpool_add(thpool_t *pool, void (*func)(void *), void *arg)`. Returns `0` on success, `-1` on failure.
* Destroy the thread pool using `void thpool_destroy(thpool_t *pool)`. Clears the job queue, waits for threads to finish their work, and finally quits.

