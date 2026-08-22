/**
 * @file test_bignum_gcd_benchmark_adapter.c
 * @brief Deterministic tests for the bignum_gcd benchmark adapter.
 * @version 1.0.0
 * @date 2026-08-22
 * @details Validates GCD-specific transport vocabulary, deterministic pair
 * initialization, callback execution and observable checksum generation.
 */
#include "bignum_gcd.h"
#include "bignum_gcd_benchmark_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Creates the fixed valid workload used by adapter tests.
 * @return A caller-owned descriptor with documented GCD axes, seed 0x9e3779b97f4a7c15,
 * five warmup calls and sixteen immutable data records.
 * @details Keeping every string and numeric field fixed makes callback output
 * reproducible and allows validation failures to be isolated to one axis.
 */
static benchmark_workload_t make_workload(void)
{
    return (benchmark_workload_t){
        .data_mode = "custom",
        .input_kind = "nonzero",
        .operation_kind = "binary-euclid",
        .measure_mode = "kernel-only",
        .size_profile = "quarter",
        .capacity_profile = "normal",
        .seed = UINT64_C(11400714819323198485),
        .warmup = 5U,
        .data_count = 16U
    };
}

/**
 * @brief Verifies accepted and rejected benchmark profile vocabulary.
 * @details The fixed workload must return the named SUCCESS status. Replacing
 * operation_kind with `not-a-gcd-operation` must return INVALID_PROFILE, while
 * a NULL descriptor must return NULL_ARGUMENT; no callback state is published.
 */
static void test_validation(void)
{
    benchmark_workload_t workload = make_workload();
    assert(bignum_gcd_benchmark_validate_workload(&workload) == BIGNUM_GCD_BENCHMARK_STATUS_SUCCESS);
    workload.operation_kind = "not-a-gcd-operation";
    assert(bignum_gcd_benchmark_validate_workload(&workload) == BIGNUM_GCD_BENCHMARK_STATUS_INVALID_PROFILE);
    assert(bignum_gcd_benchmark_validate_workload(NULL) == BIGNUM_GCD_BENCHMARK_STATUS_NULL_ARGUMENT);
}

/**
 * @brief Verifies deterministic initialization, operation and checksum callbacks.
 * @details Two equal sequence-indexed states must be byte-identical. The
 * operation callback must return the framework SUCCESS status for valid records,
 * and checksum must be non-zero after GCD execution, proving observable output.
 */
static void test_callbacks(void)
{
    benchmark_adapter_t adapter;
    benchmark_workload_t workload = make_workload();
    _Alignas(bignum_t) unsigned char first[sizeof(bignum_t) * 3U];
    _Alignas(bignum_t) unsigned char second[sizeof(bignum_t) * 3U];
    uint64_t checksum;
    assert(bignum_gcd_benchmark_adapter_init(NULL) == BIGNUM_GCD_BENCHMARK_STATUS_NULL_ARGUMENT);
    assert(bignum_gcd_benchmark_adapter_init(&adapter) == BIGNUM_GCD_BENCHMARK_STATUS_SUCCESS);
    assert(adapter.initialize != NULL && adapter.operation != NULL && adapter.checksum != NULL);
    assert(adapter.state_size == sizeof(bignum_t) * 3U);
    memset(first, 0, sizeof(first)); memset(second, 0, sizeof(second));
    assert(adapter.initialize(first, 3U, &workload, adapter.adapter_context) == BENCHMARK_ADAPTER_STATUS_SUCCESS);
    assert(adapter.initialize(second, 3U, &workload, adapter.adapter_context) == BENCHMARK_ADAPTER_STATUS_SUCCESS);
    assert(memcmp(first, second, sizeof(first)) == 0);
    assert(adapter.operation(first, 7U, &workload, adapter.adapter_context) == BENCHMARK_ADAPTER_STATUS_SUCCESS);
    checksum = adapter.checksum(first, 7U, adapter.adapter_context);
    assert(checksum != 0U);
}

/**
 * @brief Runs benchmark-adapter validation and callback contract tests.
 * @return EXIT_SUCCESS after all named statuses and deterministic invariants pass.
 * @details Assertions are the failure oracle; the process exits non-zero on any
 * invalid profile handling, state mismatch, operation failure or zero checksum.
 */
int main(void)
{
    puts("--- Starting bignum_gcd benchmark adapter tests ---");
    test_validation(); puts("test_validation: PASSED");
    test_callbacks(); puts("test_callbacks: PASSED");
    puts("--- All bignum_gcd benchmark adapter tests passed ---");
    return 0;
}
