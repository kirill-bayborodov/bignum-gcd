/**
 * @file test_bignum_gcd_runner.c
 * @brief Distribution-linkage smoke test for bignum_gcd.
 * @version 1.0.0
 * @date 2026-08-22
 * @details Includes only the public module header and verifies a successful
 * call, exact result and process exit code against the generated library.
 */
#include "bignum_gcd.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    const bignum_t a = { .words = { 84U }, .len = 1U };
    const bignum_t b = { .words = { 30U }, .len = 1U };
    bignum_t result;
    memset(&result, 0xa5, sizeof(result));
    printf("Running test: test_bignum_gcd_runner... ");
    assert(bignum_gcd(&result, &a, &b) == BIGNUM_GCD_SUCCESS);
    assert(result.len == 1U && result.words[0] == 6U);
    puts("PASSED");
    return 0;
}
