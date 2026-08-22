/**
 * @file test_bignum_gcd_extra.c
 * @brief Extended model, fuzz and boundary tests for bignum_gcd.
 * @version 1.0.0
 * @date 2026-08-22
 * @details Uses an independent unsigned-128-bit Euclidean oracle for 1024
 * fixed-seed vectors and verifies source immutability and normalized output.
 */
#include "bignum_gcd.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

__extension__ typedef unsigned __int128 uint128_t;

static uint64_t rng_state = UINT64_C(0x9e3779b97f4a7c15);
static uint64_t next_word(void)
{
    rng_state ^= rng_state << 7U;
    rng_state ^= rng_state >> 9U;
    return rng_state;
}

static uint128_t gcd128(uint128_t a, uint128_t b)
{
    while (b != 0U) {
        const uint128_t remainder = a % b;
        a = b;
        b = remainder;
    }
    return a;
}

static void from128(bignum_t *n, uint128_t value)
{
    memset(n, 0, sizeof(*n));
    if (value != 0U) {
        n->words[0] = (uint64_t)value;
        n->words[1] = (uint64_t)(value >> 64U);
        n->len = n->words[1] != 0U ? 2U : 1U;
    }
}

static uint128_t to128(const bignum_t *n)
{
    uint128_t value = 0U;
    if (n->len > 0U) value |= (uint128_t)n->words[0];
    if (n->len > 1U) value |= (uint128_t)n->words[1] << 64U;
    return value;
}

static void test_fuzz_against_model(void)
{
    for (unsigned i = 0U; i < 1024U; ++i) {
        const uint128_t a128 = ((uint128_t)next_word() << 64U) | next_word();
        const uint128_t b128 = ((uint128_t)next_word() << 64U) | next_word();
        bignum_t a, b, result, before_a, before_b;
        from128(&a, a128); from128(&b, b128);
        before_a = a; before_b = b;
        memset(&result, 0xa5, sizeof(result));
        assert(bignum_gcd(&result, &a, &b) == BIGNUM_GCD_SUCCESS);
        assert(to128(&result) == gcd128(a128, b128));
        assert(memcmp(&a, &before_a, sizeof(a)) == 0);
        assert(memcmp(&b, &before_b, sizeof(b)) == 0);
        assert(result.len <= BIGNUM_CAPACITY);
        assert(result.len == 0U || result.words[result.len - 1U] != 0U);
    }
}

static void test_large_power_two_boundary(void)
{
    bignum_t a, b, result;
    memset(&a, 0, sizeof(a)); memset(&b, 0, sizeof(b));
    a.len = BIGNUM_CAPACITY; b.len = BIGNUM_CAPACITY;
    a.words[BIGNUM_CAPACITY - 1U] = UINT64_C(1) << 63U;
    b.words[BIGNUM_CAPACITY - 1U] = UINT64_C(1) << 62U;
    assert(bignum_gcd(&result, &a, &b) == BIGNUM_GCD_SUCCESS);
    assert(result.len == BIGNUM_CAPACITY);
    assert(result.words[result.len - 1U] == UINT64_C(1) << 62U);
}

static void test_partial_overlap(void)
{
    _Alignas(bignum_t) unsigned char storage[sizeof(bignum_t) * 3U];
    bignum_t *a = (bignum_t *)(void *)(storage + sizeof(uint64_t));
    bignum_t *result = (bignum_t *)(void *)storage;
    bignum_t b, before;
    memset(storage, 0, sizeof(storage));
    a->words[0] = 84U; a->len = 1U;
    b.words[0] = 30U; b.len = 1U;
    before = *result;
    assert(bignum_gcd(result, a, &b) == BIGNUM_GCD_ERROR_OVERLAP);
    assert(memcmp(result, &before, sizeof(*result)) == 0);
}

int main(void)
{
    puts("--- Starting extended bignum_gcd tests ---");
    test_fuzz_against_model(); puts("test_fuzz_against_model: PASSED");
    test_large_power_two_boundary(); puts("test_large_power_two_boundary: PASSED");
    test_partial_overlap(); puts("test_partial_overlap: PASSED");
    puts("--- All extended bignum_gcd tests passed ---");
    return 0;
}
