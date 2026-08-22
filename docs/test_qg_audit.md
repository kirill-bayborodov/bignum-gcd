# bignum-gcd Test and Coverage QG Audit

**Revision:** 1.1.0
**Date:** 2026-08-22
**Reference standard:** [`docs/QUALITY_GATES_DOCUMENTATION.md`](QUALITY_GATES_DOCUMENTATION.md)
**Template reference:** `bignum-init-from-array/tests/` and in-family documented test suites

## 1. Template structure comparison

The module test set now has the same functional layers as the template family: deterministic API tests, extended/model tests, multithreaded reentrancy tests, a distribution runner, and a benchmark-adapter test. Template names have been replaced by the canonical `bignum_gcd` names; no template-prefixed test artifact remains in the module-owned tree.

| Template role | Current artifact | Scope | QG status |
|---|---|---|---|
| Deterministic unit test | `tests/test_bignum_gcd.c` | Fixed values, identities, normalization, invalid status and exact alias rejection | PASS |
| Extended/fuzz test | `tests/test_bignum_gcd_extra.c` | Independent 128-bit oracle, 1,024 fixed-seed vectors, capacity boundary and aligned partial overlap | PASS |
| MT test | `tests/test_bignum_gcd_mt.c` | Eight independent workers, 2,000 calls per worker, join-before-publish | PASS |
| Distribution runner | `tests/test_bignum_gcd_runner.c` | Generated distribution header/object linkage and exact `gcd(84,30)=6` | PASS |
| Benchmark adapter test | `tests/benchmark_adapter/test_bignum_gcd_benchmark_adapter.c` | Profile vocabulary, deterministic callbacks, operation status and observable checksum | PASS |

The suite is intentionally split by risk boundary rather than by implementation file. The C11 and YASM backends are both executed through the same Makefile-selected test artifacts, so the public behavior is checked identically for both implementations.

## 2. Artifact-level QG checklist

The following checklist is completed independently for every changed test source, as required by DOC-7. Each file has file-level Doxygen, each nontrivial helper/test case has a local contract comment, and the comments identify setup, oracle, expected status/output and invariant.

### `tests/test_bignum_gcd.c`

| Gate | Evidence | Result |
|---|---|---|
| DOC-1 | File block identifies deterministic GCD contract scope and oracle strategy | PASS |
| DOC-2 | `set_words`, `equal_number`, `expect_gcd`, six test cases and `main` are documented | PASS |
| DOC-3 | Fixture helper parameters and ownership are documented; no complex local type is introduced | PASS |
| DOC-4 | Each error scenario names `BIGNUM_GCD_ERROR_NULL_ARG`, `BIGNUM_GCD_ERROR_BAD_LENGTH` or `BIGNUM_GCD_ERROR_OVERLAP` and preservation result | PASS |
| DOC-6 | Comments explain stale-tail clearing, transactional sentinel and alias invariant | PASS |
| DOC-7 | Every test case documents fixed setup, exact expected result/status and invariant | PASS |
| DOC-12 | Terminology matches `bignum_gcd.h` and README | PASS |

### `tests/test_bignum_gcd_extra.c`

| Gate | Evidence | Result |
|---|---|---|
| DOC-1 | File block identifies model/fuzz, boundary and overlap scope | PASS |
| DOC-2 | `uint128_t`, RNG state, `next_word`, `gcd128`, `from128`, `to128`, three cases and `main` are documented | PASS |
| DOC-3 | Model conversion ownership and normalized zero representation are documented | PASS |
| DOC-4 | Partial overlap explicitly names `BIGNUM_GCD_ERROR_OVERLAP`; model cases require `BIGNUM_GCD_SUCCESS` | PASS |
| DOC-6 | Comments explain why the independent modulo oracle and aligned overlap fixture exist | PASS |
| DOC-7 | Fuzz seed `0x9e3779b97f4a7c15`, domain, exactly 1,024 cases, oracle and replay behavior are documented | PASS |
| DOC-12 | Coverage and capacity terminology matches the public contract | PASS |

### `tests/test_bignum_gcd_mt.c`

| Gate | Evidence | Result |
|---|---|---|
| DOC-1 | File block defines eight-worker reentrancy scope and join publication boundary | PASS |
| DOC-2 | `gcd_worker_t`, every field, `gcd_worker_run` and `main` are documented | PASS |
| DOC-3 | Each worker field has ownership, direction and validity comments | PASS |
| DOC-4 | Worker requires named `BIGNUM_GCD_SUCCESS`; exact expected result is documented as one | PASS |
| DOC-6 | Comments explain independent records, thread creation and join happens-before edge | PASS |
| DOC-7 | The scenario documents `gcd(a,a+1)=1`, eight workers and 2,000 iterations | PASS |
| DOC-12 | Thread-safety language matches the public header contract | PASS |

### `tests/test_bignum_gcd_runner.c`

| Gate | Evidence | Result |
|---|---|---|
| DOC-1 | File block identifies generated distribution linkage and public-header-only scope | PASS |
| DOC-2 | `main` has a complete process-level contract | PASS |
| DOC-4 | Exact `BIGNUM_GCD_SUCCESS` status and result value are asserted | PASS |
| DOC-6 | Sentinel publication and assertion failure behavior are explained | PASS |
| DOC-7 | Fixed fixture `gcd(84,30)=6` and expected process exit are documented | PASS |
| DOC-10 | The runner is exercised by `make install` and `make dist` | PASS |

### `tests/benchmark_adapter/test_bignum_gcd_benchmark_adapter.c`

| Gate | Evidence | Result |
|---|---|---|
| DOC-1 | File block identifies framework binding and checksum scope | PASS |
| DOC-2 | `make_workload`, `test_validation`, `test_callbacks` and `main` are documented | PASS |
| DOC-3 | Callback state buffers and framework ownership are described in the adapter/header contract | PASS |
| DOC-4 | Named adapter statuses are asserted for valid, invalid-profile and NULL paths | PASS |
| DOC-6 | Comments explain deterministic sequence-index equality and checksum observability | PASS |
| DOC-7 | Fixed seed, warmup, data count, accepted vocabulary and expected callback statuses are documented | PASS |
| DOC-12 | Operation vocabulary matches the JSON manifests and adapter validation arrays | PASS |

## 3. Coverage evidence

Coverage was collected for `src/bignum_gcd.c` with GCC `-fprofile-arcs -ftest-coverage`. The instrumented C11 object was executed by both deterministic and extended suites, including 1,024 fixed-seed model vectors.

| Metric | Result |
|---|---:|
| Lines executed | 87.50% (77 of 88) |
| Branches executed | 100.00% (88 of 88) |
| Branches taken at least once | 80.68% (71 of 88) |
| Calls executed | 100.00% (18 of 18) |

The complete source-level report is [`docs/coverage/bignum_gcd.c.gcov`](coverage/bignum_gcd.c.gcov), with the summary in [`docs/coverage/gcov_summary.txt`](coverage/gcov_summary.txt).

The seven uncovered lines are defensive branches that cannot be reached through the valid public contract without fault injection: normalization clamping after a rejected over-capacity input, dependency error returns after private records have already passed validation, and the capacity failure branch of private doubling. The public tests cover all externally constructible failure statuses, all GCD comparison directions, zero identities, common-factor restoration, full capacity and transactional preservation. Branch execution is 100%; the remaining branch-taken gap is therefore documented as unreachable defensive behavior rather than silently reported as full line coverage.

## 4. Reproduction commands

Run both implementations and all five registered test artifacts:

```bash
make clean
make test CONFIG=release USE_ASM=no
make test CONFIG=release USE_ASM=yes
```

Run dynamic checks for the C11 reference:

```bash
make test_sanitize SAN=address CONFIG=release USE_ASM=no
make test_sanitize SAN=undefined CONFIG=release USE_ASM=no
make test_helgrind CONFIG=release USE_ASM=no
```

Reproduce the benchmark-adapter and distribution checks:

```bash
make install CONFIG=release
make dist CONFIG=release
```

Reproduce the parameterized benchmark smoke matrix with the downloaded dist-only framework tools:

```bash
make bench_matrix CONFIG=release \
  BENCH_MATRIX_TOOL=libs/benchmark-framework/dist/tools/bench_matrix \
  BENCH_STATS_TOOL=libs/benchmark-framework/dist/tools/benchmark_stats \
  BENCH_MATRIX_PROFILE=benchmarks/profiles/bignum_gcd_standard.json \
  BENCH_MATRIX_REPETITIONS=1 \
  BENCH_MATRIX_ITERATIONS=100 \
  BENCH_MATRIX_MT_TOTAL_ITERATIONS=100 \
  BENCH_MATRIX_WARMUP=2 \
  BENCH_MATRIX_DATA_COUNT=8 \
  REPORT_NAME=gcd_qg_smoke
```

The expected unit-test marker is `=== Summary: 0 / 5 failed ===`. The matrix is expected to write both `gcd_qg_smoke_matrix.json` and `gcd_qg_smoke_matrix_summary.json` under `benchmarks/reports/`.
