# `bignum_add_u64` full benchmark profile

## Purpose

This companion document describes `profiles/bignum_add_u64_full.json`, the twelve-profile domain matrix for reproducible performance review of scalar addition. It extends the standard matrix with one-word, quarter-word, half-word, variable-size, and near-capacity observations while retaining the same public operation and adapter state model.

## Schema and field semantics

| Field | Meaning | Values used here |
|---|---|---|
| `schema_version` | Version of the benchmark-framework manifest schema. | `1`. |
| `id` | Stable identifier used to align C11 and ASM reports. | Addition-specific identifiers such as `near-capacity-add-kernel`. |
| `input_kind` | Input family label used for workload classification. | `zero`, `nonzero`, or `mixed`. |
| `operation_kind` | Operation selected by the adapter validator. | `add_u64` or `mixed`. |
| `measure_mode` | Timing boundary. | `kernel-only` excludes setup; `end-to-end` includes framework setup. |
| `size_profile` | Logical source length requested from the adapter. | `one`, `quarter`, `half`, `variable`, or `near-capacity`. |
| `capacity_profile` | Capacity regime associated with the case. | `normal` or `near-capacity`. |

The adapter maps `one`, `quarter`, and `half` to fixed word counts. `variable` and `near-capacity` use the full configured capacity in the current deterministic adapter. The generated source is normalized to a nonzero most significant word, and scalar `3` is added on every successful operation callback.

## Matrix intent

The full matrix distinguishes operation-kernel timing from end-to-end timing and includes duplicate-size variants with different stable identifiers so report consumers can retain workload provenance. Near-capacity profiles exercise the capacity boundary without changing the arithmetic API. The mixed profile preserves framework-level mixed metadata and remains valid because `mixed` is an explicitly accepted operation kind.

## Validation, output and retry policy

The adapter rejects NULL required strings and unsupported operation kinds before worker execution. Valid operation kinds are `add_u64` and `mixed`; `xor` and other tokens are invalid profile errors. The callback state is private to one benchmark invocation, and no global mutable state is used. The checksum consumes every result word, the normalized logical length, and the iteration number. A failed callback must be treated as an operation error and not as a successful sample.

## Reproducible commands

Run the full C11 baseline:

```sh
make bench_matrix CONFIG=release USE_ASM=no BENCH_MATRIX_PROFILE=benchmarks/profiles/bignum_add_u64_full.json BENCH_MATRIX_REPETITIONS=3 BENCH_MATRIX_ITERATIONS=100000 BENCH_MATRIX_MT_TOTAL_ITERATIONS=200000 BENCH_MATRIX_WARMUP=1000 BENCH_MATRIX_DATA_COUNT=4 BENCH_MATRIX_TIMEOUT_SECONDS=60
```

Run the full ASM comparison using the same manifest:

```sh
make bench_matrix CONFIG=release USE_ASM=yes BENCH_MATRIX_PROFILE=benchmarks/profiles/bignum_add_u64_full.json BENCH_MATRIX_REPETITIONS=3 BENCH_MATRIX_ITERATIONS=100000 BENCH_MATRIX_MT_TOTAL_ITERATIONS=200000 BENCH_MATRIX_WARMUP=1000 BENCH_MATRIX_DATA_COUNT=4 BENCH_MATRIX_TIMEOUT_SECONDS=60
```

Compare `ns_per_call` by matching `id` and measure mode. Require successful samples and matching fingerprints before attributing a difference to implementation performance. Interpret tiny one-word cases separately because fixed call and framework overhead can dominate the arithmetic work.

## Modification workflow

When adding a profile, choose a unique addition-specific `id`, document its intent in this guide, preserve the required fields, and validate the JSON before execution. Run both C11 and ASM matrices after any profile change; do not compare reports generated from different manifests.
