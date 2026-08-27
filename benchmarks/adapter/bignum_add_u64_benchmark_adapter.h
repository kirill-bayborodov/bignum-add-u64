/**
 * @file    bignum_add_u64_benchmark_adapter.h
 * @brief   Benchmark-framework adapter contract for bignum_add_u64.
 * @details The adapter owns its opaque benchmark state during a run, creates
 *          deterministic fixed-capacity inputs, and exposes the operation and
 *          checksum callbacks expected by benchmark-framework. Callers retain
 *          ownership of the adapter object and framework profile strings.
 */
#ifndef BIGNUM_ADD_U64_BENCHMARK_ADAPTER_H
#define BIGNUM_ADD_U64_BENCHMARK_ADAPTER_H

#include <benchmark_framework.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reports adapter validation or initialization status.
 * @details Adapter failures do not transfer ownership and do not promise a
 *          usable benchmark callback table.
 */
typedef enum bignum_add_u64_benchmark_status {
    BIGNUM_ADD_U64_BENCHMARK_STATUS_SUCCESS = 0, /**< Adapter operation completed; outputs are valid. */
    BIGNUM_ADD_U64_BENCHMARK_STATUS_NULL_ARGUMENT = 1, /**< A required pointer is NULL; no output is written. */
    BIGNUM_ADD_U64_BENCHMARK_STATUS_INVALID_PROFILE = 2 /**< Profile vocabulary is unsupported; retry requires a corrected profile. */
} bignum_add_u64_benchmark_status_t;

/**
 * @brief Validates one add-u64 benchmark workload.
 * @details The validator rejects absent required strings and explicitly
 *          rejects unsupported operation kinds before callbacks are invoked.
 * @param[in] workload Borrowed framework workload; may be NULL only to test
 *                      the NULL_ARGUMENT status contract.
 * @return A named bignum_add_u64_benchmark_status_t value.
 * @pre `workload` must contain the framework's required non-NULL string fields.
 * @post SUCCESS guarantees that initialization can safely consume the workload.
 * @warning The workload strings remain caller-owned and must stay valid during
 *          validation and callback initialization.
 */
bignum_add_u64_benchmark_status_t bignum_add_u64_benchmark_validate_workload(const benchmark_workload_t *workload);

/**
 * @brief Initializes the benchmark-framework callback table for add-u64.
 * @details The function writes a caller-provided adapter descriptor with the
 *          module name, state size, success code, and deterministic callbacks.
 *          No heap allocation is performed.
 * @param[out] adapter Caller-allocated framework descriptor; must be non-NULL.
 * @return SUCCESS when the descriptor is completely initialized, otherwise
 *         NULL_ARGUMENT when `adapter` is NULL.
 * @pre `adapter` points to writable benchmark_adapter_t storage.
 * @post SUCCESS yields a descriptor ready for ST or MT framework execution.
 * @warning The framework must invoke callbacks with the state size advertised
 *          by the descriptor and must serialize access to one state object.
 */
bignum_add_u64_benchmark_status_t bignum_add_u64_benchmark_adapter_init(benchmark_adapter_t *adapter);

#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_ADD_U64_BENCHMARK_ADAPTER_H */
