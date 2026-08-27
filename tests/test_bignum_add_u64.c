/**
 * @file    test_bignum_add_u64.c
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    29.07.2026
 *
 * @brief   Deterministic tests for the bignum_add_u64 module.
 */

#include "bignum_add_u64.h"
#include <bignum_cmp.h>
#include <bignum_init.h>
#include <bignum_init_from_array.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#define RUN_TEST(test_func) \
    do { \
        printf("Running %s...\n", #test_func); \
        if (test_func()) { \
            printf("  %s: PASSED\n", #test_func); \
            tests_passed++; \
        } else { \
            printf("  %s: FAILED\n", #test_func); \
            tests_failed++; \
        } \
    } while (0)

static int tests_passed = 0;
static int tests_failed = 0;

// --- Happy-path tests. ---

int test_simple_add(void) {
    bignum_t a, result, expected;
    bignum_init(&a); bignum_init(&result); bignum_init(&expected);
    
    bignum_init_from_array(&a, (uint64_t[]){10}, 1);
    bignum_init_from_array(&expected, (uint64_t[]){15}, 1);

    bignum_add_u64_status_t status = bignum_add_u64(&result, &a, 5);
    return status == BIGNUM_ADD_U64_SUCCESS && bignum_cmp(&result, &expected) == BIGNUM_CMP_EQ && result.len == 1;
}

int test_add_with_carry(void) {
    bignum_t a, result, expected;
    bignum_init(&a); bignum_init(&result); bignum_init(&expected);
    
    bignum_init_from_array(&a, (uint64_t[]){0xFFFFFFFFFFFFFFFFULL}, 1);
    bignum_init_from_array(&expected, (uint64_t[]){0, 1}, 2); // 2^64

    bignum_add_u64_status_t status = bignum_add_u64(&result, &a, 1);
    return status == BIGNUM_ADD_U64_SUCCESS && bignum_cmp(&result, &expected) == BIGNUM_CMP_EQ && result.len == 2;
}

int test_cascade_carry(void) {
    bignum_t a, result, expected;
    bignum_init(&a); bignum_init(&result); bignum_init(&expected);
    
    uint64_t arr_a[] = {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};
    bignum_init_from_array(&a, arr_a, 3);
    bignum_init_from_array(&expected, (uint64_t[]){0, 0, 0, 1}, 4);

    bignum_add_u64_status_t status = bignum_add_u64(&result, &a, 1);
    return status == BIGNUM_ADD_U64_SUCCESS && bignum_cmp(&result, &expected) == BIGNUM_CMP_EQ && result.len == 4;
}

int test_early_carry_stop(void) {
    bignum_t a, result, expected;
    bignum_init(&a); bignum_init(&result); bignum_init(&expected);
    
    // Verify that carry stops at the second word and the third is copied unchanged.
    uint64_t arr_a[] = {0xFFFFFFFFFFFFFFFFULL, 0, 0xFFFFFFFFFFFFFFFFULL};
    uint64_t arr_exp[] = {0, 1, 0xFFFFFFFFFFFFFFFFULL};
    bignum_init_from_array(&a, arr_a, 3);
    bignum_init_from_array(&expected, arr_exp, 3);

    bignum_add_u64_status_t status = bignum_add_u64(&result, &a, 1);
    return status == BIGNUM_ADD_U64_SUCCESS && bignum_cmp(&result, &expected) == BIGNUM_CMP_EQ && result.len == 3;
}

int test_add_max_u64(void) {
    bignum_t a, result, expected;
    bignum_init(&a); bignum_init(&result); bignum_init(&expected);
    
    bignum_init_from_array(&a, (uint64_t[]){10}, 1);
    bignum_init_from_array(&expected, (uint64_t[]){9, 1}, 2);

    bignum_add_u64_status_t status = bignum_add_u64(&result, &a, 0xFFFFFFFFFFFFFFFFULL);
    return status == BIGNUM_ADD_U64_SUCCESS && bignum_cmp(&result, &expected) == BIGNUM_CMP_EQ && result.len == 2;
}

// --- Boundary and normalization tests. ---

int test_add_zero_a(void) {
    bignum_t a, result, expected;
    bignum_init(&a); bignum_init(&result); bignum_init(&expected);
    
    bignum_init_from_array(&a, (uint64_t[]){0}, 0); // a.len = 0
    bignum_init_from_array(&expected, (uint64_t[]){5}, 1);

    bignum_add_u64_status_t status = bignum_add_u64(&result, &a, 5);
    return status == BIGNUM_ADD_U64_SUCCESS && bignum_cmp(&result, &expected) == BIGNUM_CMP_EQ && result.len == 1;
}

int test_add_zero_b(void) {
    bignum_t a, result, expected;
    bignum_init(&a); bignum_init(&result); bignum_init(&expected);
    
    bignum_init_from_array(&a, (uint64_t[]){123, 456}, 2);
    bignum_init_from_array(&expected, (uint64_t[]){123, 456}, 2);

    bignum_add_u64_status_t status = bignum_add_u64(&result, &a, 0);
    return status == BIGNUM_ADD_U64_SUCCESS && bignum_cmp(&result, &expected) == BIGNUM_CMP_EQ && result.len == 2;
}

int test_add_both_zero(void) {
    bignum_t a, result, expected;
    bignum_init(&a); bignum_init(&result); bignum_init(&expected);
    
    bignum_init_from_array(&a, (uint64_t[]){0}, 0);
    bignum_init_from_array(&expected, (uint64_t[]){0}, 0);

    bignum_add_u64_status_t status = bignum_add_u64(&result, &a, 0);
    return status == BIGNUM_ADD_U64_SUCCESS && bignum_cmp(&result, &expected) == BIGNUM_CMP_EQ && result.len == 0;
}

int test_unnormalized_a(void) {
    bignum_t a, result, expected;
    bignum_init(&a); bignum_init(&result); bignum_init(&expected);
    
    // a = {10, 0, 0}, len = 3 (intentionally unnormalized).
    uint64_t arr_a[] = {10, 0, 0};
    bignum_init_from_array(&a, arr_a, 3);
    bignum_init_from_array(&expected, (uint64_t[]){15}, 1);

    bignum_add_u64_status_t status = bignum_add_u64(&result, &a, 5);
    return status == BIGNUM_ADD_U64_SUCCESS && bignum_cmp(&result, &expected) == BIGNUM_CMP_EQ && result.len == 1;
}

int test_in_place_add(void) {
    bignum_t a, expected;
    bignum_init(&a); bignum_init(&expected);
    
    bignum_init_from_array(&a, (uint64_t[]){0xFFFFFFFFFFFFFFFFULL}, 1);
    bignum_init_from_array(&expected, (uint64_t[]){0, 1}, 2);

    // In-place a += 1.
    bignum_add_u64_status_t status = bignum_add_u64(&a, &a, 1);
    return status == BIGNUM_ADD_U64_SUCCESS && bignum_cmp(&a, &expected) == BIGNUM_CMP_EQ && a.len == 2;
}

// --- Error-handling tests. ---

int test_err_null_pointer(void) {
    bignum_t a, result;
    bignum_init(&a); bignum_init(&result);
    bignum_init_from_array(&a, (uint64_t[]){1}, 1);
    
    bool r1 = (bignum_add_u64(NULL, &a, 1) == BIGNUM_ADD_U64_ERROR_NULL_PTR);
    bool r2 = (bignum_add_u64(&result, NULL, 1) == BIGNUM_ADD_U64_ERROR_NULL_PTR);
    return r1 && r2;
}

int test_err_capacity_exceeded(void) {
    bignum_t a, result;
    bignum_init(&a); bignum_init(&result);
    bignum_init_from_array(&a, (uint64_t[]){1}, 1);
    
    a.len = BIGNUM_CAPACITY + 1; // Deliberately corrupt the logical length.
    bignum_add_u64_status_t status = bignum_add_u64(&result, &a, 1);
    a.len = 1; // Restore the fixture.
    return status == BIGNUM_ADD_U64_ERROR_CAPACITY_EXCEEDED;
}

int test_err_buffer_overlap(void) {
    bignum_t a;
    bignum_init(&a);
    bignum_init_from_array(&a, (uint64_t[]){10}, 1);
    
    // Create a pointer that partially overlaps a.
    bignum_t *overlap_res = (bignum_t *)((unsigned char *)&a + 1);
    return bignum_add_u64(overlap_res, &a, 5) == BIGNUM_ADD_U64_ERROR_BUFFER_OVERLAP;
}

int test_err_overflow(void) {
    bignum_t a, result;
    bignum_init(&a); bignum_init(&result);
    
    uint64_t arr_a[BIGNUM_CAPACITY];
    for(size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        arr_a[i] = 0xFFFFFFFFFFFFFFFFULL;
    }
    
    bignum_init_from_array(&a, arr_a, BIGNUM_CAPACITY);

    // The addition must carry beyond BIGNUM_CAPACITY.
    bignum_add_u64_status_t status = bignum_add_u64(&result, &a, 1);
    return status == BIGNUM_ADD_U64_ERROR_OVERFLOW;
}

/**
 * @brief Verifies that overflow does not publish a partial result.
 * @details The destination is prefilled with a sentinel object. A maximal
 *          capacity operand plus one must return overflow and preserve every
 *          destination byte, allowing the caller to retry safely.
 * @return Nonzero when status and byte-for-byte preservation are correct.
 */
int test_err_overflow_is_transactional(void) {
    bignum_t a, result, before;
    uint64_t maximal[BIGNUM_CAPACITY];
    bignum_init(&a);
    bignum_init(&result);
    for (size_t i = 0U; i < BIGNUM_CAPACITY; ++i) {
        maximal[i] = UINT64_MAX;
        result.words[i] = UINT64_C(0xA5A5A5A5A5A5A5A5);
    }
    result.len = BIGNUM_CAPACITY;
    before = result;
    bignum_init_from_array(&a, maximal, BIGNUM_CAPACITY);
    return bignum_add_u64(&result, &a, 1U) == BIGNUM_ADD_U64_ERROR_OVERFLOW &&
           memcmp(&result, &before, sizeof(result)) == 0;
}

int test_max_capacity_no_overflow(void) {
    bignum_t a, result, expected;
    bignum_init(&a); bignum_init(&result); bignum_init(&expected);
    
    uint64_t arr_a[BIGNUM_CAPACITY];
    uint64_t arr_exp[BIGNUM_CAPACITY];
    for(size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        arr_a[i] = 0xFFFFFFFFFFFFFFFFULL;
        arr_exp[i] = 0; // Low words become zero through the carry cascade.
    }
    arr_a[BIGNUM_CAPACITY - 1] = 0xFFFFFFFFFFFFFFFEULL; 
    arr_exp[BIGNUM_CAPACITY - 1] = 0xFFFFFFFFFFFFFFFFULL; // The top word receives the carry.
    
    bignum_init_from_array(&a, arr_a, BIGNUM_CAPACITY);
    bignum_init_from_array(&expected, arr_exp, BIGNUM_CAPACITY);

    bignum_add_u64_status_t status = bignum_add_u64(&result, &a, 1);
    return status == BIGNUM_ADD_U64_SUCCESS && bignum_cmp(&result, &expected) == BIGNUM_CMP_EQ && result.len == BIGNUM_CAPACITY;
}

int main() {
    printf("\n--- Launching Deterministic Tests for bignum_add_u64 ---\n");

    printf("\n--- Running Happy Path Tests ---\n");
    RUN_TEST(test_simple_add);
    RUN_TEST(test_add_with_carry);
    RUN_TEST(test_cascade_carry);
    RUN_TEST(test_early_carry_stop);
    RUN_TEST(test_add_max_u64);

    printf("\n--- Running Boundary and Normalization Tests ---\n");
    RUN_TEST(test_add_zero_a);
    RUN_TEST(test_add_zero_b);
    RUN_TEST(test_add_both_zero);
    RUN_TEST(test_unnormalized_a);
    RUN_TEST(test_in_place_add);
    RUN_TEST(test_max_capacity_no_overflow);

    printf("\n--- Running Error Handling Tests ---\n");
    RUN_TEST(test_err_null_pointer);
    RUN_TEST(test_err_capacity_exceeded);
    RUN_TEST(test_err_buffer_overlap);
    RUN_TEST(test_err_overflow);
    RUN_TEST(test_err_overflow_is_transactional);

    printf("\n--- Test Summary ---\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("\n----------------------\n");

    return tests_failed > 0 ? 1 : 0;
}
