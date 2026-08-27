/**
 * @file    bignum_add_u64.h
 * @brief   Public API for adding an unsigned 64-bit scalar to a bignum.
 * @details The module exposes one fixed-capacity, little-endian arithmetic
 *          operation. Inputs are caller-owned borrowed objects; the result is
 *          caller-allocated. The operation permits exact in-place aliasing,
 *          rejects partial object overlap, and never allocates memory.
 * @note    The function is reentrant and thread-safe when distinct bignum
 *          objects are supplied by concurrent callers.
 */

#ifndef BIGNUM_ADD_U64_H
#define BIGNUM_ADD_U64_H

#include <bignum.h>
#include <stddef.h>
#include <stdint.h>

#ifndef BIGNUM_CAPACITY
#  error "bignum.h must define BIGNUM_CAPACITY"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reports the result of bignum_add_u64.
 * @details A successful status guarantees that the result is normalized and
 *          completely written. Failure statuses identify validation or
 *          representability failures; callers must not use a failed result as
 *          a successful arithmetic value.
 */
typedef enum bignum_add_u64_status {
    BIGNUM_ADD_U64_SUCCESS                 =  0, /**< Addition completed; result is valid and normalized. */
    BIGNUM_ADD_U64_ERROR_NULL_PTR          = -1, /**< result or a is NULL; no output object is available. */
    BIGNUM_ADD_U64_ERROR_CAPACITY_EXCEEDED = -2, /**< a->len exceeds BIGNUM_CAPACITY; input is invalid and must be corrected before retry. */
    BIGNUM_ADD_U64_ERROR_BUFFER_OVERLAP    = -3, /**< result partially overlaps a; exact result == a aliasing is the only permitted overlap. */
    BIGNUM_ADD_U64_ERROR_OVERFLOW          = -4  /**< The mathematical sum needs more than BIGNUM_CAPACITY words; retry requires a larger destination. */
} bignum_add_u64_status_t;

/**
 * @brief Adds an unsigned 64-bit scalar to a fixed-capacity bignum.
 * @details The function treats `a` as a little-endian array of 64-bit words
 *          with `a->len` logical words. It validates pointers, length, and
 *          object overlap, handles zero operands through dedicated fast paths,
 *          propagates carry through the active words, copies any untouched
 *          suffix, appends a final carry when representable, clears unused
 *          capacity, and normalizes the published length. Exact in-place use
 *          (`result == a`) is supported; partial overlap is rejected.
 *
 *          The caller owns both input and output storage. The function does
 *          not allocate, free, or retain pointers. The implementation is
 *          reentrant and thread-safe when concurrently executing calls do not
 *          share mutable bignum objects.
 *
 * @param[out] result Caller-allocated destination bignum; must be non-NULL
 *                    and either disjoint from `a` or exactly equal to `a`.
 * @param[in]  a      Caller-owned source bignum; must be non-NULL and have
 *                    `a->len <= BIGNUM_CAPACITY`.
 * @param[in]  b      Unsigned 64-bit scalar to add; the unit is one bignum
 *                    word and zero is valid.
 * @return A named bignum_add_u64_status_t value describing success or the
 *         validation/overflow failure. On success, `result` is normalized;
 *         on failure, callers must not treat `result` as a valid sum.
 * @pre `result` and `a` point to valid bignum storage unless the caller is
 *     deliberately testing the NULL error contract.
 * @pre `result` and `a` are disjoint or identical; partial object overlap is
 *     forbidden.
 * @post On BIGNUM_ADD_U64_SUCCESS, `result` equals `a + b`, has normalized
 *       length, and contains no stale words beyond its logical length.
 * @warning A nonzero scalar added to a full-capacity maximal operand returns
 *          BIGNUM_ADD_U64_ERROR_OVERFLOW. Callers must check the status before
 *          consuming the output.
 * @complexity O(BIGNUM_CAPACITY) worst-case time and O(1) auxiliary space.
 */
bignum_add_u64_status_t bignum_add_u64(bignum_t *result, const bignum_t *a, uint64_t b);

#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_ADD_U64_H */
