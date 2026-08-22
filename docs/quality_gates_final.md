# bignum_gcd Quality Gates Report

**Revision:** 1.0.0
**Date:** 2026-08-22
**Backend scope:** C11 reference and standalone YASM x86-64 implementations
**Repository:** `kirill-bayborodov/bignum-gcd`

## Implementation

The module implements Stein's binary Euclidean algorithm for normalized unsigned `bignum_t` values. The C11 reference is in `src/bignum_gcd.c`; the YASM production implementation is in `src/bignum_gcd.asm`. Both expose the typed API declared in `include/bignum_gcd.h`.

The public contract validates null pointers, input lengths and result/input overlap before publishing output. It permits the two read-only inputs to alias each other, leaves the destination unchanged on documented failure, handles zero identities, and publishes normalized results.

## Verification matrix

| Gate | Command or evidence | Result |
|---|---|---|
| C11 build | `make build CONFIG=release USE_ASM=no` | PASS |
| ASM build | `make build CONFIG=release USE_ASM=yes` | PASS |
| C11 tests | `make test CONFIG=release USE_ASM=no` | PASS, `0 / 5 failed` |
| ASM tests | `make test CONFIG=release USE_ASM=yes` | PASS, `0 / 5 failed` |
| Deterministic coverage | identities, known values, normalization, invalid input, overlap | PASS |
| Model/fuzz coverage | 1,024 fixed-seed unsigned-128-bit oracle vectors | PASS |
| Capacity boundary | 2,048-bit power-of-two operands | PASS |
| MT/reentrancy | eight workers, independent records, 2,000 calls per worker | PASS |
| AddressSanitizer | `make test_sanitize SAN=address CONFIG=release USE_ASM=no` | PASS, 0 sanitizer issues |
| UndefinedBehaviorSanitizer | `make test_sanitize SAN=undefined CONFIG=release USE_ASM=no` | PASS, 0 sanitizer issues |
| Helgrind | `make test_helgrind CONFIG=release USE_ASM=no` | PASS, 0 races |
| Static analysis | `make lint` | PASS; dependency missing-include notices only |
| Install | `make install CONFIG=release` | PASS; distribution runner passed |
| Distribution | `make dist CONFIG=release` | PASS; distribution runner passed |
| JSON manifests | `jq empty benchmarks/profiles/bignum_gcd_*.json` | PASS |
| Doxygen configuration | `Doxyfile`, public header and source comments | PASS configuration present |
| Protected files | `git diff -- Makefile .github` | PASS; no changes |
| Stale template scan | template/shift identifiers in module-owned files | PASS; none found |

## Benchmark smoke evidence

A short parameterized matrix was run with the downloaded `benchmark-framework` distribution, `BENCH_MATRIX_REPETITIONS=1`, `BENCH_MATRIX_ITERATIONS=100`, `BENCH_MATRIX_MT_TOTAL_ITERATIONS=100`, `BENCH_MATRIX_WARMUP=2` and `BENCH_MATRIX_DATA_COUNT=8`. Both C11 and ASM completed all 16 ST/MT profile-mode samples successfully.

| Representative profile | C11 ST ns/call | ASM ST ns/call | C11 MT ns/call | ASM MT ns/call |
|---|---:|---:|---:|---:|
| zero / one / end-to-end | 165.17 | 174.89 | 943.16 | 794.22 |
| nonzero / one / kernel-only | 1,956.06 | 1,419.97 | 1,028.97 | 934.91 |
| nonzero / quarter / kernel-only | 27,449.75 | 23,132.28 | 14,556.88 | 14,289.75 |
| nonzero / half / kernel-only | 59,162.73 | 45,865.36 | 29,607.85 | 30,642.38 |
| nonzero / near-capacity / kernel-only | 132,465.22 | 98,403.21 | 69,449.64 | 63,671.68 |

The smoke matrix is correctness/integration evidence rather than a statistically stable performance baseline. The ASM path is faster on the representative nonzero workloads; final performance conclusions require repeated runs on a stable host.

## Benchmark-framework distribution note

The local framework was downloaded into `libs/benchmark-framework/dist` from the latest successful upstream `dist` artifact. Because the artifact provides `benchmark_framework.h` and prebuilt tools under `dist/tools`, project benchmark entrypoints include the actual aggregate header name and matrix smoke commands override `BENCH_MATRIX_TOOL` and `BENCH_STATS_TOOL` to the downloaded `dist/tools` binaries. The official Makefile and CI workflow were not modified.

## Documentation gates

The README follows the template section order: Distribution, Features, Dependencies, API, Contract, Build and test, Benchmarks, Perf workflow, Installation and distribution, Linking the object file, Contributing and License. The header documents parameters, return statuses, preconditions, postconditions, aliasing, normalization, thread safety, ownership and complexity. The benchmark adapter and JSON profile companions document their domain-specific vocabulary.

## Limitations

Hardware PMU `perf record` availability depends on the host kernel and permissions. The benchmark smoke matrix intentionally uses direct framework timing and does not claim hardware-counter evidence. The YASM implementation is targeted to System V AMD64 and is not a Windows x64 ABI implementation.
