/**
 * @file    bignum_add_u64_benchmark_adapter.c
 * @brief   Deterministic benchmark-framework callbacks for add-u64.
 * @details The adapter generates a fixed-capacity little-endian operand from
 *          the workload seed, adds scalar three, and exposes a checksum that
 *          includes every result word and the logical length. The state is
 *          private to one framework invocation and is never shared globally.
 */

#include "bignum_add_u64_benchmark_adapter.h"
#include "bignum_add_u64.h"

#include <stdint.h>
#include <string.h>

#define FNV_OFFSET UINT64_C(1469598103934665603)
#define FNV_PRIME UINT64_C(1099511628211)

/**
 * @brief Holds one benchmark invocation's add-u64 operands and result.
 * @details All fields are caller/framework-owned through the opaque state
 *          allocation. The result is overwritten by each operation callback.
 */
typedef struct add_u64_state {
    bignum_t a;       /**< Borrowed-input fixture generated during initialization. */
    bignum_t b;       /**< Reserved bignum slot retained for state-layout symmetry. */
    bignum_t result;  /**< Caller-visible operation result consumed by checksum. */
    uint64_t scalar;  /**< Scalar operand, fixed at three for deterministic runs. */
} add_u64_state_t;

/** @brief Compares two optional framework strings for exact equality. */
static int equal_text(const char *a, const char *b) {
    return a != NULL && b != NULL && strcmp(a, b) == 0;
}

/**
 * @brief Advances the adapter's deterministic xorshift generator.
 * @param[in,out] state Generator state; zero is replaced with a nonzero seed.
 * @return Next pseudo-random 64-bit word.
 */
static uint64_t next_value(uint64_t *state) {
    if (*state == 0U) {
        *state = UINT64_C(0x9e3779b97f4a7c15);
    }
    *state ^= *state << 7U;
    *state ^= *state >> 9U;
    *state ^= *state << 8U;
    return *state;
}

/**
 * @brief Maps a framework size vocabulary to a logical word count.
 * @param[in] workload Validated workload descriptor.
 * @return One, quarter, half, or full fixed capacity in words.
 */
static size_t choose_length(const benchmark_workload_t *workload) {
    if (equal_text(workload->size_profile, "one") || equal_text(workload->size_profile, "tiny")) {
        return 1U;
    }
    if (equal_text(workload->size_profile, "quarter") || equal_text(workload->size_profile, "small")) {
        return BIGNUM_CAPACITY / 4U;
    }
    if (equal_text(workload->size_profile, "half") || equal_text(workload->size_profile, "medium")) {
        return BIGNUM_CAPACITY / 2U;
    }
    return BIGNUM_CAPACITY;
}

/**
 * @brief Initializes one deterministic add-u64 benchmark state.
 * @param[in,out] opaque Framework-owned state storage of advertised size.
 * @param[in] index Dataset index used to decorrelate generated operands.
 * @param[in] workload Borrowed validated workload descriptor.
 * @param[in] context Unused framework context.
 * @return Framework callback status; state is valid only on success.
 */
static benchmark_adapter_status_t initialize(void *opaque, uint64_t index,
                                             const benchmark_workload_t *workload,
                                             void *context) {
    add_u64_state_t *state = opaque;
    uint64_t seed;
    size_t length;
    (void)context;

    if (state == NULL || workload == NULL ||
        bignum_add_u64_benchmark_validate_workload(workload) != BIGNUM_ADD_U64_BENCHMARK_STATUS_SUCCESS) {
        return BENCHMARK_ADAPTER_STATUS_INPUT_ERROR;
    }

    memset(state, 0, sizeof(*state));
    seed = workload->seed ^ (index + UINT64_C(0x9e3779b97f4a7c15));
    length = choose_length(workload);
    state->a.len = length;
    for (size_t i = 0U; i < length; ++i) {
        state->a.words[i] = next_value(&seed);
    }
    if (state->a.words[length - 1U] == 0U) {
        state->a.words[length - 1U] = 1U;
    }
    state->scalar = UINT64_C(3);
    return BENCHMARK_ADAPTER_STATUS_SUCCESS;
}

/**
 * @brief Executes one add-u64 operation on the prepared state.
 * @param[in,out] opaque Adapter state initialized by initialize().
 * @param[in] iteration Framework iteration number; not used by arithmetic.
 * @param[in] workload Borrowed workload descriptor; not used after validation.
 * @param[in] context Unused framework context.
 * @return Framework callback status; operation errors are not published as success.
 */
static benchmark_adapter_status_t operation(void *opaque, uint64_t iteration,
                                            const benchmark_workload_t *workload,
                                            void *context) {
    add_u64_state_t *state = opaque;
    (void)iteration;
    (void)workload;
    (void)context;

    if (state == NULL || bignum_add_u64(&state->result, &state->a, state->scalar) != BIGNUM_ADD_U64_SUCCESS) {
        return BENCHMARK_ADAPTER_STATUS_OPERATION_ERROR;
    }
    return BENCHMARK_ADAPTER_STATUS_SUCCESS;
}

/**
 * @brief Computes a result-sensitive FNV-1a checksum for benchmark observability.
 * @param[in] opaque Adapter state containing the most recent result.
 * @param[in] iteration Framework iteration number mixed into the checksum.
 * @param[in] context Unused framework context.
 * @return Nonzero checksum, or zero when state is NULL.
 */
static uint64_t checksum(const void *opaque, uint64_t iteration, void *context) {
    const add_u64_state_t *state = opaque;
    uint64_t hash = FNV_OFFSET;
    (void)context;

    if (state == NULL) {
        return 0U;
    }
    for (size_t i = 0U; i < BIGNUM_CAPACITY; ++i) {
        hash ^= state->result.words[i];
        hash *= FNV_PRIME;
    }
    hash ^= state->result.len;
    hash *= FNV_PRIME;
    return hash ^ iteration;
}

bignum_add_u64_benchmark_status_t
bignum_add_u64_benchmark_validate_workload(const benchmark_workload_t *workload) {
    if (workload == NULL) {
        return BIGNUM_ADD_U64_BENCHMARK_STATUS_NULL_ARGUMENT;
    }
    if (workload->input_kind == NULL || workload->operation_kind == NULL ||
        workload->measure_mode == NULL || workload->size_profile == NULL ||
        workload->capacity_profile == NULL) {
        return BIGNUM_ADD_U64_BENCHMARK_STATUS_INVALID_PROFILE;
    }
    if (!equal_text(workload->operation_kind, "add_u64") &&
        !equal_text(workload->operation_kind, "mixed")) {
        return BIGNUM_ADD_U64_BENCHMARK_STATUS_INVALID_PROFILE;
    }
    return BIGNUM_ADD_U64_BENCHMARK_STATUS_SUCCESS;
}

bignum_add_u64_benchmark_status_t
bignum_add_u64_benchmark_adapter_init(benchmark_adapter_t *adapter) {
    if (adapter == NULL) {
        return BIGNUM_ADD_U64_BENCHMARK_STATUS_NULL_ARGUMENT;
    }
    *adapter = (benchmark_adapter_t){
        .benchmark_name = "bignum_add_u64",
        .state_size = sizeof(add_u64_state_t),
        .success_code = BENCHMARK_ADAPTER_STATUS_SUCCESS,
        .adapter_context = NULL,
        .initialize = initialize,
        .operation = operation,
        .checksum = checksum
    };
    return BIGNUM_ADD_U64_BENCHMARK_STATUS_SUCCESS;
}
