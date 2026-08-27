/**
 * @file    bignum_add_u64.c
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    29.07.2026
 *
 * @brief   C11 reference implementation for adding a bignum and a 64-bit scalar.
 * @details This implementation is the readable correctness baseline. It validates
 *          the fixed-capacity representation, handles exact in-place aliasing,
 *          propagates carry, normalizes the result, and performs no allocation.
 *          The public status contract is defined in the accompanying header.
 */

#include "bignum_add_u64.h"

/**
 * @brief Detects forbidden partial overlap between two bignum objects.
 * @details Exact pointer equality is permitted for the documented in-place
 *          operation. Otherwise the complete fixed-size object ranges are
 *          compared so that writes cannot corrupt a borrowed input.
 * @param[in] res Candidate result object; the pointer is borrowed.
 * @param[in] op Source object; the pointer is borrowed.
 * @return Nonzero when the objects partially overlap; zero otherwise.
 */
static inline int check_buffer_overlap(const bignum_t *res, const bignum_t *op) {
    if (res == op) {
        return 0; // Exact in-place operation (result == a) is permitted.
    }
    
    const unsigned char *p_res = (const unsigned char *)res;
    const unsigned char *p_op  = (const unsigned char *)op;

    // Reject any partial overlap across the complete fixed-size object range.
    if ((p_res < p_op + sizeof(bignum_t)) && (p_op < p_res + sizeof(bignum_t))) {
        return 1;
    }
    return 0;
}

bignum_add_u64_status_t bignum_add_u64(bignum_t *result, const bignum_t *a, const uint64_t b) {
    // Validate borrowed pointers before dereferencing either object.
    if (!result || !a) {
        return BIGNUM_ADD_U64_ERROR_NULL_PTR;
    }

    // Reject malformed logical lengths before writing the destination.
    if (a->len > BIGNUM_CAPACITY) {
        return BIGNUM_ADD_U64_ERROR_CAPACITY_EXCEEDED;
    }

    // Preserve the source invariant by rejecting partial object overlap.
    if (check_buffer_overlap(result, a)) {
        return BIGNUM_ADD_U64_ERROR_BUFFER_OVERLAP;
    }

    // Compute privately so overflow or validation failures cannot publish a partial result.
    bignum_t temporary = {0};

    // Zero-source fast path: construct the scalar result directly.
    if (a->len == 0) {
        if (b == 0) {
            temporary.len = 0;
        } else {
            temporary.words[0] = b;
            temporary.len = 1;
        }
        *result = temporary;
        return BIGNUM_ADD_U64_SUCCESS;
    }

    // Zero-scalar fast path: copy and normalize without arithmetic.
    if (b == 0) {
        for (size_t i = 0; i < a->len; i++) {
            temporary.words[i] = a->words[i];
        }
        size_t new_len = a->len;
        // Normalize malformed-but-capacity-valid input before publishing length.
        while (new_len > 0 && temporary.words[new_len - 1] == 0) {
            new_len--;
        }
        temporary.len = new_len;
        *result = temporary;
        return BIGNUM_ADD_U64_SUCCESS;
    }

    // Add the scalar and propagate carry through the active source words.
    uint64_t carry = b;
    size_t i = 0;

    for (; i < a->len; i++) {
        uint64_t w = a->words[i];
        uint64_t sum = w + carry;
        
        carry = (sum < w) ? 1 : 0; // Unsigned wraparound denotes carry.
        temporary.words[i] = sum;

        // Once carry is clear, the remaining source words are unchanged.
        if (carry == 0) {
            i++;
            break;
        }
    }

    // Copy the untouched suffix when the destination is not in-place.
    if (result != a) {
        for (; i < a->len; i++) {
            temporary.words[i] = a->words[i];
        }
    }

    size_t new_len = a->len;

    // Publish a final carry only when the fixed capacity can represent it.
    if (carry) {
        if (new_len == BIGNUM_CAPACITY) {
            return BIGNUM_ADD_U64_ERROR_OVERFLOW;
        }
        temporary.words[new_len] = 1;
        new_len++;
    }

    // Remove high zero words before publishing the final logical length.
    while (new_len > 0 && temporary.words[new_len - 1] == 0) {
        new_len--;
    }
    temporary.len = new_len;

    *result = temporary;
    return BIGNUM_ADD_U64_SUCCESS;
}
