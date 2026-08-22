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

/**
 * @brief Owns one independent worker's GCD fixtures and expected result.
 * @details The worker record is created and initialized by main before thread
 * creation. Its fields are accessed by only one worker until pthread_join makes
 * the final result visible to the main thread.
 */
typedef struct gcd_worker {
    bignum_t a; /**< [in] First normalized operand owned by this worker. */
    bignum_t b; /**< [in] Second normalized operand owned by this worker. */
    bignum_t result; /**< [out] Caller-owned result record written by the worker. */
    uint64_t expected; /**< [in] Exact expected one-word GCD for this fixture. */
} gcd_worker_t;

/**
 * @brief Repeats one worker's independent GCD operation.
 * @param[in,out] opaque Non-NULL pointer to this worker's private record.
 * @return NULL after all iterations complete; assertions abort on status failure.
 * @details Each iteration uses separate records and therefore shares no mutable
 * bignum state with other workers. The fixed count tests reentrancy under load.
 */
static void *gcd_worker_run(void *opaque)
{
    gcd_worker_t *worker = (gcd_worker_t *)opaque;
    for (unsigned i = 0U; i < GCD_MT_ITERATIONS; ++i) {
        assert(bignum_gcd(&worker->result, &worker->a, &worker->b) == BIGNUM_GCD_SUCCESS);
    }
    return NULL;
}

/**
 * @brief Runs the concurrent reentrancy and publication-order test.
 * @return EXIT_SUCCESS when all joined workers produced their expected GCD.
 * @details Eight workers compute gcd(a,a+1)=1 for 2,000 iterations. The main
 * thread initializes all inputs before pthread_create, joins every worker, and
 * checks results only after join establishes the required happens-before edge.
 */
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
