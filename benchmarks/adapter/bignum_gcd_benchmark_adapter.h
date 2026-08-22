/**
 * @file bignum_gcd_benchmark_adapter.h
 * @brief Benchmark-framework binding for two-operand bignum_gcd workloads.
 * @version 1.0.0
 * @date 2026-08-22
 * @details The adapter maps generic text profiles to deterministic pairs of
 * caller-owned bignum_t records, invokes the typed GCD API and supplies an
 * observable checksum for ST and MT runners. benchmark-core owns lifecycle,
 * timing, threading and protocol output; this module owns domain semantics.
 */
#ifndef BIGNUM_GCD_BENCHMARK_ADAPTER_H
#define BIGNUM_GCD_BENCHMARK_ADAPTER_H

#include <benchmark_framework.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reports adapter validation and callback preparation outcomes.
 * @details No adapter callback allocates memory or uses mutable global state.
 */
typedef enum bignum_gcd_benchmark_status {
    BIGNUM_GCD_BENCHMARK_STATUS_SUCCESS = 0, /**< Callback completed and its state is valid. */
    BIGNUM_GCD_BENCHMARK_STATUS_NULL_ARGUMENT = 1, /**< Required pointer was NULL; state is unchanged. */
    BIGNUM_GCD_BENCHMARK_STATUS_INVALID_PROFILE = 2, /**< A workload axis is outside the documented GCD vocabulary. */
    BIGNUM_GCD_BENCHMARK_STATUS_OPERATION_ERROR = 3 /**< bignum_gcd rejected the generated workload; no valid result exists. */
} bignum_gcd_benchmark_status_t;

/**
 * @brief Initializes benchmark-core callbacks for bignum_gcd.
 * @param[out] adapter Caller-owned callback table to initialize.
 * @return Named adapter status; `SUCCESS` means every callback is installed.
 * @par Thread safety
 * Safe when each benchmark-core run owns its adapter instance.
 */
bignum_gcd_benchmark_status_t bignum_gcd_benchmark_adapter_init(
    benchmark_adapter_t *adapter);

/**
 * @brief Validates the generic workload axes accepted by this adapter.
 * @param[in] workload Immutable benchmark-framework workload descriptor.
 * @return `SUCCESS` for valid input/mode/operation/size/capacity values.
 * @warning The descriptor and all referenced strings must remain valid during
 *          the call; no ownership is transferred.
 */
bignum_gcd_benchmark_status_t bignum_gcd_benchmark_validate_workload(
    const benchmark_workload_t *workload);

#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_GCD_BENCHMARK_ADAPTER_H */
