/**
 * @file    test_bignum_add_u64_extra.c
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    29.07.2026
 *
 * @brief   Extended robustness and randomized tests for bignum_add_u64.
 * @details The fuzz oracle checks successful results for monotonicity and
 *          exercises bounded random logical lengths without sharing state.
 */

#include "bignum_add_u64.h"
#include <bignum_cmp.h>
#include <bignum_init.h>
#include <bignum_init_from_array.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

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

#define FUZZ_ITERATIONS 10000

static int tests_passed = 0;
static int tests_failed = 0;

int test_robustness_a_len_exceeds_capacity() {
    bignum_t a, result;
    bignum_init_from_array(&a, (uint64_t[]){1}, 1);
    a.len = BIGNUM_CAPACITY + 1;
    bignum_add_u64_status_t status = bignum_add_u64(&result, &a, 1);
    a.len = 1;
    return status == BIGNUM_ADD_U64_ERROR_CAPACITY_EXCEEDED;
}

static void print_bignum(const char* name, const bignum_t* num) {
    fprintf(stderr, "%s (len=%zu): { ", name, num->len);
    if (num->len == 0) {
        fprintf(stderr, "0 ");
    } else {
        for (size_t i = 0; i < num->len; ++i) {
            fprintf(stderr, "0x%016lX ", num->words[i]);
        }
    }
    fprintf(stderr, "}\n");
}

int test_fuzzing_robustness(void) {
    unsigned int seed = time(NULL) ^ getpid();
    srand(seed);
    printf("Fuzzing with seed: %u\n", seed);

    for (int i = 0; i < FUZZ_ITERATIONS; ++i) {
        bignum_t a, result;
        bignum_init(&a); bignum_init(&result);

        a.len = rand() % BIGNUM_CAPACITY;
        for (size_t j = 0; j < a.len; ++j) a.words[j] = ((uint64_t)rand() << 32) | rand();
        while (a.len > 0 && a.words[a.len - 1] == 0) a.len--;

        uint64_t b = ((uint64_t)rand() << 32) | rand();

        bignum_add_u64_status_t status = bignum_add_u64(&result, &a, b);

        if (status == BIGNUM_ADD_U64_SUCCESS) {
            // Monotonicity: result >= a
            if (bignum_cmp(&result, &a) == BIGNUM_CMP_LESS) {
                fprintf(stderr, "Fuzzing failed: Result is smaller than operand 'a'\n");
                print_bignum("a", &a);
                fprintf(stderr, "b: 0x%016lX\n", b);
                print_bignum("res", &result);
                return 0;
            }
        }
    }
    return 1;
}

int main() {
    printf("\n--- Launching Extra Tests for bignum_add_u64  ---\n");
    printf("\n--- Running Robustness Tests ---\n");
    RUN_TEST(test_robustness_a_len_exceeds_capacity);
    printf("\n--- Running Fuzzing Test ---\n");
    RUN_TEST(test_fuzzing_robustness);

    printf("\n--- Test Summary ---\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("\n----------------------\n");

    return tests_failed > 0 ? 1 : 0;
}
