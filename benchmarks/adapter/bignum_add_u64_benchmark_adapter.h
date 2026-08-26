#ifndef BIGNUM_ADD_U64_BENCHMARK_ADAPTER_H
#define BIGNUM_ADD_U64_BENCHMARK_ADAPTER_H
#include <benchmark_framework.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { BIGNUM_ADD_U64_BENCHMARK_STATUS_SUCCESS = 0, BIGNUM_ADD_U64_BENCHMARK_STATUS_NULL_ARGUMENT = 1, BIGNUM_ADD_U64_BENCHMARK_STATUS_INVALID_PROFILE = 2 } bignum_add_u64_benchmark_status_t;
bignum_add_u64_benchmark_status_t bignum_add_u64_benchmark_adapter_init(benchmark_adapter_t *adapter);
bignum_add_u64_benchmark_status_t bignum_add_u64_benchmark_validate_workload(const benchmark_workload_t *workload);
#ifdef __cplusplus
}
#endif
#endif
