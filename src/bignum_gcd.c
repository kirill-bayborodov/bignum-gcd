/**
 * @file bignum_gcd.c
 * @brief C11 reference implementation of binary bignum GCD.
 * @version 1.0.0
 * @date 2026-08-22
 *
 * @details Uses the approved comparison, right-shift and subtraction modules
 * as the arithmetic primitives. Private fixed-size records make validation
 * failures and dependency failures atomic with respect to the caller's
 * destination. The implementation is deliberately straightforward so it can
 * serve as a correctness reference for the standalone YASM implementation.
 */
#include "bignum_gcd.h"
#include "bignum_cmp.h"
#include "bignum_shift_right.h"
#include "bignum_sub_bignum.h"
#include <stdint.h>
#include <string.h>

/** @brief Returns whether two fixed-size bignum records overlap. */
static int bignum_gcd_overlap(const bignum_t *left, const bignum_t *right)
{
    const uintptr_t a = (uintptr_t)left;
    const uintptr_t b = (uintptr_t)right;
    const uintptr_t size = (uintptr_t)sizeof(bignum_t);

    /* Difference is evaluated only in the direction that cannot underflow. */
    return a < b ? (b - a) < size : (a - b) < size;
}

/** @brief Normalizes a record and clears words outside its logical length. */
static void bignum_gcd_normalize(bignum_t *number)
{
    size_t length = number->len;

    if (length > BIGNUM_CAPACITY) {
        length = BIGNUM_CAPACITY;
    }
    while (length != 0U && number->words[length - 1U] == UINT64_C(0)) {
        --length;
    }
    for (size_t index = length; index < BIGNUM_CAPACITY; ++index) {
        number->words[index] = UINT64_C(0);
    }
    number->len = length;
}

/**
 * @brief Doubles one private number without allocating memory.
 * @details The helper propagates carry from low to high words. GCD arithmetic
 * cannot require a value larger than the original operands, but the capacity
 * check remains explicit to preserve the public failure invariant.
 * @param[in,out] number Private normalized record to double in place.
 * @return BIGNUM_GCD_SUCCESS or BIGNUM_GCD_ERROR_CAPACITY.
 */
static bignum_gcd_status_t bignum_gcd_shift_left_one(bignum_t *number)
{
    uint64_t carry = UINT64_C(0);
    const size_t old_length = number->len;

    for (size_t index = 0U; index < old_length; ++index) {
        const uint64_t word = number->words[index];
        number->words[index] = (word << 1U) | carry;
        carry = word >> 63U;
    }
    if (carry != UINT64_C(0)) {
        if (old_length == BIGNUM_CAPACITY) {
            return BIGNUM_GCD_ERROR_CAPACITY;
        }
        number->words[old_length] = carry;
        number->len = old_length + 1U;
    }
    bignum_gcd_normalize(number);
    return BIGNUM_GCD_SUCCESS;
}

/** @brief Returns whether a normalized number is even, treating zero as even. */
static int bignum_gcd_is_even(const bignum_t *number)
{
    return number->len == 0U || (number->words[0] & UINT64_C(1)) == UINT64_C(0);
}

/**
 * @brief Computes GCD using Stein's binary Euclidean algorithm.
 * @details Validation is completed before private records are initialized.
 * The two source records are copied, so dependency calls may safely mutate
 * their private destinations and the caller's result is published last.
 */
bignum_gcd_status_t bignum_gcd(
    bignum_t *result,
    const bignum_t *a,
    const bignum_t *b)
{
    bignum_t left;
    bignum_t right;
    bignum_t difference;
    size_t common_shift = 0U;
    bignum_cmp_status_t comparison;

    if (result == NULL || a == NULL || b == NULL) {
        return BIGNUM_GCD_ERROR_NULL_ARG;
    }
    if (bignum_gcd_overlap(result, a) || bignum_gcd_overlap(result, b)) {
        return BIGNUM_GCD_ERROR_OVERLAP;
    }
    if (a->len > BIGNUM_CAPACITY || b->len > BIGNUM_CAPACITY) {
        return BIGNUM_GCD_ERROR_BAD_LENGTH;
    }

    left = *a;
    right = *b;
    bignum_gcd_normalize(&left);
    bignum_gcd_normalize(&right);

    if (left.len == 0U) {
        *result = right;
        return BIGNUM_GCD_SUCCESS;
    }
    if (right.len == 0U) {
        *result = left;
        return BIGNUM_GCD_SUCCESS;
    }

    /* Remove the common power of two before the odd subtraction loop. */
    while (bignum_gcd_is_even(&left) && bignum_gcd_is_even(&right)) {
        if (bignum_shift_right(&left, 1U) == BIGNUM_SHIFT_RIGHT_ERROR_NULL_ARG ||
            bignum_shift_right(&right, 1U) == BIGNUM_SHIFT_RIGHT_ERROR_NULL_ARG) {
            return BIGNUM_GCD_ERROR_CAPACITY;
        }
        ++common_shift;
    }

    while (left.len != 0U && right.len != 0U &&
           !(left.len == 1U && left.words[0] == UINT64_C(1)) &&
           !(right.len == 1U && right.words[0] == UINT64_C(1))) {
        while (bignum_gcd_is_even(&left)) {
            if (bignum_shift_right(&left, 1U) == BIGNUM_SHIFT_RIGHT_ERROR_NULL_ARG) {
                return BIGNUM_GCD_ERROR_CAPACITY;
            }
        }
        while (bignum_gcd_is_even(&right)) {
            if (bignum_shift_right(&right, 1U) == BIGNUM_SHIFT_RIGHT_ERROR_NULL_ARG) {
                return BIGNUM_GCD_ERROR_CAPACITY;
            }
        }
        comparison = bignum_cmp(&left, &right);
        if (comparison == BIGNUM_CMP_ERROR_NULL) {
            return BIGNUM_GCD_ERROR_CAPACITY;
        }
        if (comparison == BIGNUM_CMP_GREATER) {
            if (bignum_sub_bignum(&difference, &left, &right) != BIGNUM_SUB_SUCCESS) {
                return BIGNUM_GCD_ERROR_CAPACITY;
            }
            left = difference;
        } else if (comparison == BIGNUM_CMP_LESS) {
            if (bignum_sub_bignum(&difference, &right, &left) != BIGNUM_SUB_SUCCESS) {
                return BIGNUM_GCD_ERROR_CAPACITY;
            }
            right = difference;
        } else {
            break;
        }
    }

    if (left.len == 0U) {
        left = right;
    } else if (right.len == 0U) {
        right = left;
    } else if (left.len == 1U && left.words[0] == UINT64_C(1)) {
        left = left;
    } else {
        left = right;
    }
    bignum_gcd_normalize(&left);
    while (common_shift-- != 0U) {
        if (bignum_gcd_shift_left_one(&left) != BIGNUM_GCD_SUCCESS) {
            return BIGNUM_GCD_ERROR_CAPACITY;
        }
    }
    *result = left;
    return BIGNUM_GCD_SUCCESS;
}
