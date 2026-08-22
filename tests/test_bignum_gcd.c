/**
 * @file test_bignum_gcd.c
 * @brief Deterministic contract tests for bignum_gcd.
 * @version 1.0.0
 * @date 2026-08-22
 * @details Covers binary-Euclid identities, normalization, multiword values,
 * invalid arguments, overlap rejection and byte-for-byte failure preservation.
 */
#include "bignum_gcd.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void set_words(bignum_t *n, const uint64_t *words, size_t len)
{
    memset(n, 0, sizeof(*n));
    if (len != 0U) memcpy(n->words, words, len * sizeof(uint64_t));
    n->len = len;
}

static int equal_number(const bignum_t *a, const bignum_t *b)
{
    return a->len == b->len && memcmp(a->words, b->words, sizeof(a->words)) == 0;
}

static void expect_gcd(const uint64_t *aw, size_t alen, const uint64_t *bw,
                       size_t blen, const uint64_t *ew, size_t elen)
{
    bignum_t a, b, expected, result;
    set_words(&a, aw, alen); set_words(&b, bw, blen); set_words(&expected, ew, elen);
    memset(&result, 0xa5, sizeof(result));
    assert(bignum_gcd(&result, &a, &b) == BIGNUM_GCD_SUCCESS);
    assert(equal_number(&result, &expected));
}

static void test_zero_identities(void)
{
    const uint64_t value[] = { UINT64_C(0x0123456789abcdef), UINT64_C(0x10) };
    const uint64_t zero[] = { 0 };
    expect_gcd(value, 2U, zero, 0U, value, 2U);
    expect_gcd(zero, 0U, value, 2U, value, 2U);
    expect_gcd(zero, 0U, zero, 0U, zero, 0U);
}

static void test_known_values(void)
{
    const uint64_t a[] = { 48U }, b[] = { 18U }, e[] = { 6U };
    const uint64_t c[] = { 0xabcdef00U, 0x1234U }, d[] = { 0x11111100U, 0x12U };
    const uint64_t f[] = { 0x100U };
    expect_gcd(a, 1U, b, 1U, e, 1U);
    expect_gcd(b, 1U, a, 1U, e, 1U);
    expect_gcd(c, 2U, d, 2U, f, 1U);
}

static void test_power_of_two_factors(void)
{
    const uint64_t a[] = { UINT64_C(0x00000000000000c0) };
    const uint64_t b[] = { UINT64_C(0x0000000000000030) };
    const uint64_t e[] = { UINT64_C(0x0000000000000030) };
    expect_gcd(a, 1U, b, 1U, e, 1U);
}

static void test_same_operand_and_normalization(void)
{
    bignum_t a, result;
    const uint64_t words[] = { UINT64_C(0x55), 0U, 0U };
    set_words(&a, words, 3U);
    memset(&result, 0xa5, sizeof(result));
    assert(bignum_gcd(&result, &a, &a) == BIGNUM_GCD_SUCCESS);
    assert(result.len == 1U && result.words[0] == UINT64_C(0x55));
    for (size_t i = 1U; i < BIGNUM_CAPACITY; ++i) assert(result.words[i] == 0U);
}

static void test_invalid_inputs_preserve_result(void)
{
    bignum_t a, b, result, before;
    const uint64_t one[] = { 1U };
    set_words(&a, one, 1U); set_words(&b, one, 1U);
    memset(&result, 0x5a, sizeof(result)); before = result;
    assert(bignum_gcd(NULL, &a, &b) == BIGNUM_GCD_ERROR_NULL_ARG);
    assert(bignum_gcd(&result, NULL, &b) == BIGNUM_GCD_ERROR_NULL_ARG);
    assert(bignum_gcd(&result, &a, NULL) == BIGNUM_GCD_ERROR_NULL_ARG);
    assert(memcmp(&result, &before, sizeof(result)) == 0);
    a.len = BIGNUM_CAPACITY + 1U;
    assert(bignum_gcd(&result, &a, &b) == BIGNUM_GCD_ERROR_BAD_LENGTH);
    assert(memcmp(&result, &before, sizeof(result)) == 0);
}

static void test_alias_rejection(void)
{
    bignum_t a, b, before;
    const uint64_t av[] = { 84U }, bv[] = { 30U };
    set_words(&a, av, 1U); set_words(&b, bv, 1U); before = a;
    assert(bignum_gcd(&a, &a, &b) == BIGNUM_GCD_ERROR_OVERLAP);
    assert(memcmp(&a, &before, sizeof(a)) == 0);
    before = b;
    assert(bignum_gcd(&b, &a, &b) == BIGNUM_GCD_ERROR_OVERLAP);
    assert(memcmp(&b, &before, sizeof(b)) == 0);
}

int main(void)
{
    puts("--- Starting deterministic bignum_gcd tests ---");
    test_zero_identities(); puts("test_zero_identities: PASSED");
    test_known_values(); puts("test_known_values: PASSED");
    test_power_of_two_factors(); puts("test_power_of_two_factors: PASSED");
    test_same_operand_and_normalization(); puts("test_same_operand_and_normalization: PASSED");
    test_invalid_inputs_preserve_result(); puts("test_invalid_inputs_preserve_result: PASSED");
    test_alias_rejection(); puts("test_alias_rejection: PASSED");
    puts("--- All deterministic bignum_gcd tests passed ---");
    return 0;
}
