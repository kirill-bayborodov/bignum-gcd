/**
 * @file bignum_gcd_benchmark_adapter.c
 * @brief Deterministic two-operand domain adapter for bignum_gcd.
 * @version 1.0.0
 * @date 2026-08-22
 * @details benchmark-core owns allocation, lifecycle, timing and threads. This
 * adapter validates the workload, generates immutable operand pairs, invokes
 * bignum_gcd and hashes the complete post-operation state.
 */
#include "bignum_gcd_benchmark_adapter.h"
#include "bignum_gcd.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define GCD_BENCH_FNV_OFFSET UINT64_C(1469598103934665603)
#define GCD_BENCH_FNV_PRIME UINT64_C(1099511628211)

typedef struct bignum_gcd_benchmark_state {
    bignum_t a; /**< [in] Immutable first operand for this benchmark row. */
    bignum_t b; /**< [in] Immutable second operand for this benchmark row. */
    bignum_t result; /**< [out] GCD result written by the operation callback. */
} bignum_gcd_benchmark_state_t;

static bignum_gcd_benchmark_status_t string_equal(const char *left, const char *right,
                                                   benchmark_boolean_t *equal)
{
    if (left == NULL || right == NULL || equal == NULL) return BIGNUM_GCD_BENCHMARK_STATUS_NULL_ARGUMENT;
    *equal = strcmp(left, right) == 0 ? BENCHMARK_BOOLEAN_TRUE : BENCHMARK_BOOLEAN_FALSE;
    return BIGNUM_GCD_BENCHMARK_STATUS_SUCCESS;
}

static bignum_gcd_benchmark_status_t axis_allowed(const char *value,
                                                   const char *const *allowed)
{
    if (value == NULL || allowed == NULL) return BIGNUM_GCD_BENCHMARK_STATUS_NULL_ARGUMENT;
    for (size_t i = 0U; allowed[i] != NULL; ++i) {
        benchmark_boolean_t equal;
        if (string_equal(value, allowed[i], &equal) != BIGNUM_GCD_BENCHMARK_STATUS_SUCCESS) {
            return BIGNUM_GCD_BENCHMARK_STATUS_NULL_ARGUMENT;
        }
        if (equal == BENCHMARK_BOOLEAN_TRUE) return BIGNUM_GCD_BENCHMARK_STATUS_SUCCESS;
    }
    return BIGNUM_GCD_BENCHMARK_STATUS_INVALID_PROFILE;
}

static uint64_t next_value(uint64_t *state)
{
    if (*state == 0U) *state = UINT64_C(0x9e3779b97f4a7c15);
    *state ^= *state << 7U;
    *state ^= *state >> 9U;
    *state ^= *state << 8U;
    return *state;
}

static size_t choose_length(const benchmark_workload_t *workload, uint64_t *state)
{
    if (strcmp(workload->capacity_profile, "near-capacity") == 0 ||
        strcmp(workload->size_profile, "near-capacity") == 0) {
        return BIGNUM_CAPACITY > 2U ? BIGNUM_CAPACITY - 2U : BIGNUM_CAPACITY;
    }
    if (strcmp(workload->size_profile, "one") == 0) return 1U;
    if (strcmp(workload->size_profile, "quarter") == 0) return BIGNUM_CAPACITY / 4U;
    if (strcmp(workload->size_profile, "half") == 0) return BIGNUM_CAPACITY / 2U;
    return 1U + (size_t)(next_value(state) % (BIGNUM_CAPACITY / 2U));
}

static void fill_operand(bignum_t *number, size_t length, uint64_t *state, int zero)
{
    memset(number, 0, sizeof(*number));
    if (zero) return;
    number->len = length == 0U ? 1U : length;
    for (size_t i = 0U; i < number->len; ++i) number->words[i] = next_value(state);
    if (number->words[number->len - 1U] == 0U) number->words[number->len - 1U] = UINT64_C(1);
}

static benchmark_adapter_status_t gcd_initialize(void *opaque, uint64_t sequence_index,
                                                   const benchmark_workload_t *workload,
                                                   void *adapter_context)
{
    bignum_gcd_benchmark_state_t *state = opaque;
    uint64_t random_state;
    size_t length;
    int zero_a;
    int zero_b;
    (void)adapter_context;
    if (state == NULL || workload == NULL ||
        bignum_gcd_benchmark_validate_workload(workload) != BIGNUM_GCD_BENCHMARK_STATUS_SUCCESS) {
        return BENCHMARK_ADAPTER_STATUS_INPUT_ERROR;
    }
    random_state = workload->seed ^ (sequence_index + UINT64_C(0x9e3779b97f4a7c15));
    length = choose_length(workload, &random_state);
    zero_a = strcmp(workload->input_kind, "zero") == 0 ||
             (strcmp(workload->input_kind, "mixed") == 0 && (sequence_index % 2U) == 0U);
    zero_b = strcmp(workload->input_kind, "zero") == 0;
    fill_operand(&state->a, length, &random_state, zero_a);
    fill_operand(&state->b, length, &random_state, zero_b);
    memset(&state->result, 0, sizeof(state->result));
    return BENCHMARK_ADAPTER_STATUS_SUCCESS;
}

static benchmark_adapter_status_t gcd_operation(void *opaque, uint64_t iteration,
                                                  const benchmark_workload_t *workload,
                                                  void *adapter_context)
{
    bignum_gcd_benchmark_state_t *state = opaque;
    bignum_gcd_status_t status;
    (void)iteration; (void)workload; (void)adapter_context;
    if (state == NULL) return BENCHMARK_ADAPTER_STATUS_INPUT_ERROR;
    status = bignum_gcd(&state->result, &state->a, &state->b);
    return status == BIGNUM_GCD_SUCCESS ? BENCHMARK_ADAPTER_STATUS_SUCCESS
                                         : BENCHMARK_ADAPTER_STATUS_OPERATION_ERROR;
}

static uint64_t gcd_checksum(const void *opaque, uint64_t iteration, void *adapter_context)
{
    const bignum_gcd_benchmark_state_t *state = opaque;
    uint64_t checksum = GCD_BENCH_FNV_OFFSET;
    (void)adapter_context;
    if (state == NULL) return 0U;
    for (size_t record = 0U; record < 3U; ++record) {
        const bignum_t *number = record == 0U ? &state->a : record == 1U ? &state->b : &state->result;
        for (size_t word = 0U; word < BIGNUM_CAPACITY; ++word) {
            checksum ^= number->words[word]; checksum *= GCD_BENCH_FNV_PRIME;
        }
        checksum ^= (uint64_t)number->len; checksum *= GCD_BENCH_FNV_PRIME;
    }
    checksum ^= iteration; checksum *= GCD_BENCH_FNV_PRIME;
    return checksum;
}

bignum_gcd_benchmark_status_t bignum_gcd_benchmark_validate_workload(
    const benchmark_workload_t *workload)
{
    static const char *const inputs[] = { "zero", "nonzero", "mixed", NULL };
    static const char *const operations[] = { "binary-euclid", "gcd", "gcd-mixed", NULL };
    static const char *const measures[] = { "end-to-end", "kernel-only", NULL };
    static const char *const sizes[] = { "one", "quarter", "half", "variable", "near-capacity", NULL };
    static const char *const capacities[] = { "normal", "near-capacity", NULL };
    bignum_gcd_benchmark_status_t status;
    if (workload == NULL) return BIGNUM_GCD_BENCHMARK_STATUS_NULL_ARGUMENT;
    status = axis_allowed(workload->input_kind, inputs); if (status != BIGNUM_GCD_BENCHMARK_STATUS_SUCCESS) return status;
    status = axis_allowed(workload->operation_kind, operations); if (status != BIGNUM_GCD_BENCHMARK_STATUS_SUCCESS) return status;
    status = axis_allowed(workload->measure_mode, measures); if (status != BIGNUM_GCD_BENCHMARK_STATUS_SUCCESS) return status;
    status = axis_allowed(workload->size_profile, sizes); if (status != BIGNUM_GCD_BENCHMARK_STATUS_SUCCESS) return status;
    return axis_allowed(workload->capacity_profile, capacities);
}

bignum_gcd_benchmark_status_t bignum_gcd_benchmark_adapter_init(benchmark_adapter_t *adapter)
{
    if (adapter == NULL) return BIGNUM_GCD_BENCHMARK_STATUS_NULL_ARGUMENT;
    *adapter = (benchmark_adapter_t){
        .benchmark_name = "bignum_gcd",
        .state_size = sizeof(bignum_gcd_benchmark_state_t),
        .success_code = BENCHMARK_ADAPTER_STATUS_SUCCESS,
        .adapter_context = NULL,
        .initialize = gcd_initialize,
        .operation = gcd_operation,
        .checksum = gcd_checksum
    };
    return BIGNUM_GCD_BENCHMARK_STATUS_SUCCESS;
}
