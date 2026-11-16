# TCC Setup

This repository packages the benchmark programs, generated LLM solutions,
reference OpenMP implementations, and the measurement scripts used in the TCC.

## Repository layout

```
tcc_setup/
├── programs/              # Sequential benchmark sources used as prompts and baselines
│   ├── DP, NI, MM, OE, DFT, TR, DJ    # PAMPAR
│   ├── NUS, CHO, ADI                  # PolyBench/C 4.2.1 (+ LICENSE-PolyBench.txt)
│   └── CG, MG, FT                     # NPB 3.3.1, Fortran: official copy in npb/ (setup in progress)
├── reference/             # OpenMP reference implementations (PAMPAR and NPB; none for PolyBench)
├── LLM_programs/          # LLM-generated parallel versions, grouped by benchmark
├── baseline/              # Captured measurements for the sequential programs
├── reference_results/      # Captured measurements for the reference OpenMP versions
├── llm_results/           # Captured measurements for each LLM-generated version
├── measure.sh             # Common compilation and measurement script
├── baseline.sh            # Convenience wrapper for sequential runs
├── run_reference.sh       # Convenience wrapper for reference runs
├── run_llm.sh             # Convenience wrapper for LLM runs
└── run_sweep.sh           # Full experiment over 2,4,...,24 threads
```

## Benchmarks

| Key | Program | Source | Input | Reference |
|-----|---------|--------|-------|-----------|
| DP, NI, MM, OE, DFT, TR | see PAMPAR | PAMPAR | `measure.sh` | PAMPAR OpenMP |
| NUS | Nussinov | PolyBench/C 4.2.1 | n = 3000 | none |
| CHO | Cholesky | PolyBench/C 4.2.1 | n = 2700 | none |
| ADI | Alternating Direction Implicit | PolyBench/C 4.2.1 | 150 steps, n = 1000 | none |
| CG, MG, FT | Conjugate Gradient, Multigrid, 3D FFT | NPB 3.3.1 (Fortran, `npb/`) | setup in progress | NPB 3.3.1 OpenMP |

The PolyBench programs were made single-file C programs, like PAMPAR's,
so that each LLM receives one complete file:

- **PolyBench**: the kernel and data initialization are unchanged; the size is
  read from the command line and a checksum of the result is printed. Cholesky's
  initialization (untimed) skips products that are zero, building the same matrix
  much faster. All three were checked to give bit-identical results to the
  original PolyBench code.
- Only the kernel is timed, as in PolyBench
  (PAMPAR times the whole program).
- `check_correctness.sh` compares NPB programs by NPB's own verification verdict.

## How to use

### 1. Measure the sequential baselines
These runs produce the sequential timing baseline, which is the denominator for
speedup, and the output value used as the correctness oracle.

```bash
./baseline.sh
```

If you want to run a single benchmark manually:

```bash
./measure.sh programs/DP/DP_sequential.c DP
```

### 2. Measure the reference OpenMP versions

```bash
./run_reference.sh 11
```

### 3. Measure the LLM-generated versions

```bash
./run_llm.sh 11
```

### Full thread sweep

```bash
./run_sweep.sh            # baseline + 2,4,6,...,24 threads
./run_sweep.sh 4 8 16     # custom thread counts
```

The scripts save their output in `baseline/`, `reference_results/`, and
`llm_results/` respectively.

## Notes

- Core pinning follows the thread count: T threads run on cores `0..T-1`,
  sequential runs on core 0. Override with `CORES=... ./measure.sh ...`.
- The current measurement script runs each program 30 times and then reports
  average and standard deviation.
- `measure.sh` exports `OMP_NUM_THREADS` for parallel versions; if a program
  expects the thread count as an argument, use the fourth parameter described
  in the script header.
- The scripts rely on `gcc`, `taskset`, `awk`, and optionally `perf`.
