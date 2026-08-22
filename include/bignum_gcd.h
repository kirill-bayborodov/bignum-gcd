/**
 * @file bignum_gcd.h
 * @brief Typed API for the greatest common divisor of bignum_t values.
 * @version 1.0.0
 * @date 2026-08-22
 *
 * @details
 * Computes the greatest common divisor of two normalized, non-negative
 * bignum_t operands using Stein's binary Euclidean algorithm. The public
 * operation is transactional: result is written only after validation and
 * arithmetic complete successfully. The caller owns all records; the module
 * performs no allocation and uses no mutable global state.
 *
 * Calls with independent records are thread-safe. `result` must not overlap
 * either input, while the two read-only inputs may refer to the same record.
 */
#ifndef BIGNUM_GCD_H
#define BIGNUM_GCD_H

#include <bignum.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reports validation and arithmetic outcome for bignum_gcd.
 * @details A successful status guarantees that result contains a normalized
 * gcd. Every failure status guarantees that the caller-owned result is
 * unchanged. The function does not allocate memory and can be retried after
 * the caller corrects its inputs.
 */
typedef enum bignum_gcd_status {
    BIGNUM_GCD_SUCCESS = 0, /**< Inputs were valid and result contains gcd(a,b). */
    BIGNUM_GCD_ERROR_NULL_ARG = -1, /**< A required pointer was NULL; result is unchanged. */
    BIGNUM_GCD_ERROR_BAD_LENGTH = -2, /**< An input len exceeded BIGNUM_CAPACITY; result is unchanged. */
    BIGNUM_GCD_ERROR_OVERLAP = -3, /**< result overlaps an input record; result is unchanged. */
    BIGNUM_GCD_ERROR_CAPACITY = -4 /**< Internal result publication would exceed capacity; result is unchanged. */
} bignum_gcd_status_t;

/**
 * @brief Computes the greatest common divisor of two bignum_t values.
 * @details Copies the borrowed inputs into private records, removes common
 * powers of two, repeatedly removes remaining even factors and subtracts the
 * smaller odd value from the larger value. The saved common power of two is
 * restored before the normalized result is published. Zero operands are
 * handled directly: gcd(a,0)=a, gcd(0,b)=b, and gcd(0,0)=0.
 *
 * @param[out] result Caller-allocated output record. It is borrowed for the
 *        duration of the call and must not overlap either input.
 * @param[in] a Caller-owned normalized non-negative first operand. Its storage
 *        remains owned by the caller and is not modified.
 * @param[in] b Caller-owned normalized non-negative second operand. Its storage
 *        remains owned by the caller and is not modified; it may alias a.
 * @return bignum_gcd_status_t BIGNUM_GCD_SUCCESS on success, or the named
 *         validation/capacity status explaining why result was not changed.
 * @pre result, a and b are valid pointers; each input len is at most
 *      BIGNUM_CAPACITY; result does not overlap a or b.
 * @post On success result is normalized and represents gcd(a,b), including
 *       result->len == 0 for gcd(0,0). On failure result is byte-for-byte
 *       unchanged.
 * @warning The API accepts only non-negative normalized bignum_t values. A
 *          caller must synchronize concurrent access to the same record.
 * @par Complexity
 * O(n^2) worst-case word operations for n-word inputs and O(1) private storage
 * relative to the fixed BIGNUM_CAPACITY.
 * @par Thread safety
 * Safe for concurrent calls when all calls use independent result and input
 * records; no mutable global state is used.
 */
bignum_gcd_status_t bignum_gcd(
    bignum_t *result,
    const bignum_t *a,
    const bignum_t *b);

#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_GCD_H */
