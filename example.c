#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "thpool.h"

void do_work(void *arg) {
    int *timep = (int *) arg;
    printf("Sleeping for %d seconds\n", *timep);
    sleep(*timep);
    printf("Woke up after %d seconds\n", *timep);
    free(arg);
}

int main() {
    int i;
    thpool_t *pool;

    pool = thpool_create(5);
    if (!pool) {
        return -1;
    }
    for (i = 0; i < 10; i++) {
        int *x = malloc(sizeof(int));
        if (!x) {
            return -1;
        }
        *x = i;
        if (thpool_add(pool, do_work, x) != 0) {
            return -1;
        }
    }
    sleep(6);
    thpool_destroy(pool);
    return 0;
}