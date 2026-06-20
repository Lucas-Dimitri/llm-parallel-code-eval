# TCC Setup

This repository packages the benchmark programs, generated LLM solutions,
reference OpenMP implementations, and the measurement scripts used in the TCC.

## Repository layout

```
tcc_setup/
├── programs/              # Sequential benchmark sources used as prompts and baselines
│   ├── DP/DP_sequential.c
│   ├── DFT/DFT_sequential.c
│   ├── DJ/DJ_sequential.c
│   ├── MM/MM_sequential.c
│   ├── NI/NI_sequential.c
│   ├── OE/OE_sequential.c
│   └── TR/TR_sequential.c
├── reference/             # OpenMP reference implementations used as ground truth
├── LLM_programs/          # LLM-generated parallel versions, grouped by benchmark
├── baseline/              # Captured measurements for the sequential programs
├── reference_results/      # Captured measurements for the reference OpenMP versions
├── llm_results/           # Captured measurements for each LLM-generated version
├── measure.sh             # Common compilation and measurement script
├── baseline.sh            # Convenience wrapper for sequential runs
├── run_reference.sh       # Convenience wrapper for reference runs
└── run_llm.sh             # Convenience wrapper for LLM runs
```

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

The scripts save their output in `baseline/`, `reference_results/`, and
`llm_results/` respectively.

## Notes

- Update `CORES` in `measure.sh` to match the machine used for the experiment.
- The current measurement script runs each program 30 times and then reports
  average and standard deviation.
- `measure.sh` exports `OMP_NUM_THREADS` for parallel versions; if a program
  expects the thread count as an argument, use the fourth parameter described
  in the script header.
- The scripts rely on `gcc`, `taskset`, `awk`, `bc`, and optionally `perf`.
