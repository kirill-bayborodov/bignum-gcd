/**
 * @file test_bignum_gcd_mt.c
 * @brief Multithreaded reentrancy tests for bignum_gcd.
 * @version 1.0.0
 * @date 2026-08-22
 * @details Eight workers repeatedly compute GCD on independent caller-owned
 * records; joins establish completion before results are checked.
 */
#include "bignum_gcd.h"
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define GCD_MT_THREADS 8U
#define GCD_MT_ITERATIONS 2000U

typedef struct gcd_worker {
    bignum_t a;
    bignum_t b;
    bignum_t result;
    uint64_t expected;
} gcd_worker_t;

static void *gcd_worker_run(void *opaque)
{
    gcd_worker_t *worker = (gcd_worker_t *)opaque;
    for (unsigned i = 0U; i < GCD_MT_ITERATIONS; ++i) {
        assert(bignum_gcd(&worker->result, &worker->a, &worker->b) == BIGNUM_GCD_SUCCESS);
    }
    return NULL;
}

int main(void)
{
    pthread_t threads[GCD_MT_THREADS];
    gcd_worker_t workers[GCD_MT_THREADS];
    puts("--- Starting multithreaded bignum_gcd test ---");
    for (unsigned i = 0U; i < GCD_MT_THREADS; ++i) {
        memset(&workers[i], 0, sizeof(workers[i]));
        workers[i].a.words[0] = UINT64_C(0x100000001) * (i + 3U);
        workers[i].a.words[1] = (uint64_t)(i + 1U);
        workers[i].a.len = 2U;
        workers[i].b.words[0] = workers[i].a.words[0] + 1U;
        workers[i].b.words[1] = workers[i].a.words[1];
        workers[i].b.len = 2U;
        workers[i].expected = 1U;
        assert(pthread_create(&threads[i], NULL, gcd_worker_run, &workers[i]) == 0);
    }
    for (unsigned i = 0U; i < GCD_MT_THREADS; ++i) assert(pthread_join(threads[i], NULL) == 0);
    for (unsigned i = 0U; i < GCD_MT_THREADS; ++i) {
        assert(workers[i].result.len == 1U);
        assert(workers[i].result.words[0] == workers[i].expected);
    }
    puts("--- Multithreaded bignum_gcd test passed ---");
    return 0;
}
