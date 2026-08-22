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

static void test_validation(void)
{
    benchmark_workload_t workload = make_workload();
    assert(bignum_gcd_benchmark_validate_workload(&workload) == BIGNUM_GCD_BENCHMARK_STATUS_SUCCESS);
    workload.operation_kind = "not-a-gcd-operation";
    assert(bignum_gcd_benchmark_validate_workload(&workload) == BIGNUM_GCD_BENCHMARK_STATUS_INVALID_PROFILE);
    assert(bignum_gcd_benchmark_validate_workload(NULL) == BIGNUM_GCD_BENCHMARK_STATUS_NULL_ARGUMENT);
}

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

int main(void)
{
    puts("--- Starting bignum_gcd benchmark adapter tests ---");
    test_validation(); puts("test_validation: PASSED");
    test_callbacks(); puts("test_callbacks: PASSED");
    puts("--- All bignum_gcd benchmark adapter tests passed ---");
    return 0;
}
