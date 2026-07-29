/**
 * @file    test_bignum_add_u64.c
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    29.07.2026
 *
 * @brief   Детерминированные тесты для модуля bignum_add_u64.
 */

#include "bignum_add_u64.h"
#include <bignum_common.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#define BIGNUM_CMP_GREATER     1
#define BIGNUM_CMP_EQ          0
#define BIGNUM_CMP_LESS       -1
#define BIGNUM_CMP_ERROR_NULL (int)0x80000000

/**
 * @brief Сравнивает два больших беззнаковых числа.
 */
int bignum_cmp(const bignum_t* a, const bignum_t* b) {
    if (!a || !b) return BIGNUM_CMP_ERROR_NULL;
    if (a->len != b->len) return (a->len > b->len) ? BIGNUM_CMP_GREATER : BIGNUM_CMP_LESS;
    if (a->len == 0) return BIGNUM_CMP_EQ;
    for (size_t i = a->len; i > 0; --i) {
        uint64_t word_a = a->words[i - 1];
        uint64_t word_b = b->words[i - 1];
        if (word_a != word_b) return (word_a > word_b) ? BIGNUM_CMP_GREATER : BIGNUM_CMP_LESS;
    }
    return BIGNUM_CMP_EQ;
}

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

// --- Тесты на "счастливые пути" ---

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
    
    // Проверяем, что перенос останавливается на втором слове, а третье копируется как есть
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

// --- Тесты на граничные случаи и нормализацию ---

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
    
    // a = {10, 0, 0}, len = 3 (не нормализовано)
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

    // a += 1
    bignum_add_u64_status_t status = bignum_add_u64(&a, &a, 1);
    return status == BIGNUM_ADD_U64_SUCCESS && bignum_cmp(&a, &expected) == BIGNUM_CMP_EQ && a.len == 2;
}

// --- Тесты на обработку ошибок ---

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
    
    a.len = BIGNUM_CAPACITY + 1; // Искусственно портим длину
    bignum_add_u64_status_t status = bignum_add_u64(&result, &a, 1);
    a.len = 1; // Восстанавливаем
    return status == BIGNUM_ADD_U64_ERROR_CAPACITY_EXCEEDED;
}

int test_err_buffer_overlap(void) {
    bignum_t a;
    bignum_init(&a);
    bignum_init_from_array(&a, (uint64_t[]){10}, 1);
    
    // Создаем указатель, который частично перекрывает a
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

    // Сложение должно вызвать перенос за пределы BIGNUM_CAPACITY
    bignum_add_u64_status_t status = bignum_add_u64(&result, &a, 1);
    return status == BIGNUM_ADD_U64_ERROR_OVERFLOW;
}

int test_max_capacity_no_overflow(void) {
    bignum_t a, result, expected;
    bignum_init(&a); bignum_init(&result); bignum_init(&expected);
    
    uint64_t arr_a[BIGNUM_CAPACITY];
    uint64_t arr_exp[BIGNUM_CAPACITY];
    for(size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        arr_a[i] = 0xFFFFFFFFFFFFFFFFULL;
        arr_exp[i] = 0; // Младшие слова обнулятся из-за каскадного переноса
    }
    arr_a[BIGNUM_CAPACITY - 1] = 0xFFFFFFFFFFFFFFFEULL; 
    arr_exp[BIGNUM_CAPACITY - 1] = 0xFFFFFFFFFFFFFFFFULL; // Старшее слово примет перенос
    
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

    printf("\n--- Test Summary ---\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("\n----------------------\n");

    return tests_failed > 0 ? 1 : 0;
}
