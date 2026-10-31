#!/usr/bin/env bash
# =====================================================================
# measure.sh - Baseline / uniform measurement script
# TCC: Evaluating LLMs on the Conversion of Sequential Code to Parallel C
# 
# Measures execution time of a C program over N runs and reports the
# average and standard deviation. Works identically for the sequential
# baseline AND for any LLM-generated parallel version, so every program
# goes through the same measurement pipeline (fair comparison).
#
# USAGE:
#   ./measure.sh <source.c> <program_key> [num_threads] [threads_as_arg]
#
#   <source.c>       path to the C file to compile and measure
#   <program_key>    one of: DP NI MM OE DJ DFT TR (determines input args)
#   [num_threads]    optional; sets OMP_NUM_THREADS for parallel versions
#   [threads_as_arg] optional; set to "yes" if the program expects threads
#                    as its first argument (PAMPAR reference convention: <threads> <N>)
#
# EXAMPLES:
#   ./measure.sh programs/DP/sequential.c DP
#   ./measure.sh programs/DP/DP_sequential.c DP
#   ./measure.sh LLM_programs/DP/DP_claude.c DP 11
#   ./measure.sh reference/DP_openmp_reference.c DP 11 yes
# =====================================================================

set -u

# ---- Configuration ---------------------------------------------------
RUNS=30                          # number of timed executions
# Core pinning (taskset) is derived from the thread count further below:
# T threads -> cores 0..T-1; sequential -> core 0. Set CORES in the
# environment to override, e.g. CORES="0-23" ./measure.sh ...
CFLAGS="-O2 -fopenmp -DELAPSEDTIME"  # -DELAPSEDTIME enables internal timer
                                # (harmless if the program lacks the block;
                                #  we also fall back to external timing)
LDLIBS="-lm"                     # must come after the source file (Ubuntu links with --as-needed)

# Input sizes per program (based on PAMPAR Table 2; reduced where a full
# size would be too slow for repeated runs -- adjust to taste).
# Input sizes per program (based on PAMPAR Table 2; reduced where a full
declare -A INPUT_ARGS
INPUT_ARGS[DP]="10000000000"         # dot product: N (~2.3s; pure arithmetic, no memory limit)
INPUT_ARGS[NI]="1000000000"         # numerical integration: N (~2s; no array allocation)
INPUT_ARGS[MM]="2048"               # matrix multiplication: NxN (~2s; PAMPAR reference size)
INPUT_ARGS[OE]="100000"             # odd-even sort: vector size (~3.3s; O(n^2) scaling)
INPUT_ARGS[DFT]="10000"             # discrete fourier transform: N (~1.75s; O(n^2) trig ops)
INPUT_ARGS[TR]="1280"               # predator-prey ring: NxN grid (~1.9s; O(n^3) in practice)
# DJ excluded: fscanf text parsing dominates runtime, masking parallel speedup (Amdahl's Law)

# ---- Arguments -------------------------------------------------------
SRC="${1:-}"
KEY="${2:-}"
THREADS="${3:-}"
THREADS_AS_ARG="${4:-no}"

if [[ -z "$SRC" || -z "$KEY" ]]; then
  echo "Usage: $0 <source.c> <program_key DP|NI|MM|OE|DJ> [num_threads]"
  exit 1
fi
if [[ ! -f "$SRC" ]]; then
  echo "ERROR: source file not found: $SRC"; exit 1
fi
if [[ -z "${INPUT_ARGS[$KEY]+x}" ]]; then
  echo "ERROR: unknown program key '$KEY' (use DP NI MM OE DJ)"; exit 1
fi

ARGS="${INPUT_ARGS[$KEY]}"

if [[ -z "${CORES:-}" ]]; then
  if [[ -n "$THREADS" ]]; then
    CORES="0-$((THREADS - 1))"
  else
    CORES="0"
  fi
fi
BIN="$(mktemp /tmp/tcc_bin.XXXXXX)"

# ---- Compile ---------------------------------------------------------
echo ">> Compiling $SRC ..."
if ! gcc $CFLAGS "$SRC" -o "$BIN" $LDLIBS 2> /tmp/tcc_cc_err; then
  echo "COMPILE_FAILED"
  echo "--- compiler output ---"
  cat /tmp/tcc_cc_err
  rm -f "$BIN"
  exit 2
fi

# For parallel versions, some programs take <threads> as the FIRST arg
# (PAMPAR's openmp.c convention). If THREADS is given AND the program
# expects it, prepend it. We also export OMP_NUM_THREADS as a fallback
# for versions that read the environment variable instead.
RUN_ARGS="$ARGS"
if [[ -n "$THREADS" ]]; then
  export OMP_NUM_THREADS="$THREADS"
  if [[ "$THREADS_AS_ARG" == "yes" ]]; then
    RUN_ARGS="$THREADS $ARGS"
  fi
fi

# ---- Measure ---------------------------------------------------------
echo ">> Running $RUNS times (key=$KEY, args='$RUN_ARGS', threads='${THREADS:-default}')"
times=()
for ((r=1; r<=RUNS; r++)); do
  # Capture program stdout (may contain "Execution time\t<sec>")
  out="$(taskset -c "$CORES" "$BIN" $RUN_ARGS 2>/dev/null)"

  # Prefer the program's internal timer if present...
  t="$(echo "$out" | awk -F'\t' '/Execution time/ {print $2}')"

  # ...otherwise fall back to external wall-clock timing.
  if [[ -z "$t" ]]; then
    start=$(date +%s.%N)
    taskset -c "$CORES" "$BIN" $RUN_ARGS >/dev/null 2>&1
    end=$(date +%s.%N)
    t=$(awk -v s="$start" -v e="$end" 'BEGIN {printf "%.6f", e - s}')
  fi

  times+=("$t")
  printf "   run %2d: %s s\n" "$r" "$t"
done

# ---- Statistics ------------------------------------------------------
avg=$(printf '%s\n' "${times[@]}" | awk '{s+=$1} END {printf "%.6f", s/NR}')
std=$(printf '%s\n' "${times[@]}" | awk -v a="$avg" '{d=$1-a; s+=d*d} END {printf "%.6f", sqrt(s/NR)}')

echo "---------------------------------------------"
echo "Program:    $KEY"
echo "Source:     $SRC"
echo "Threads:    ${THREADS:-sequential/default}"
echo "Cores:      $CORES"
echo "Runs:       $RUNS"
echo "Average:    $avg s"
echo "Std dev:    $std s"
echo "---------------------------------------------"

# ---- Hardware counters (perf stat) -----------------------------------
if command -v perf &>/dev/null; then
  echo ">> Collecting hardware counters (perf stat, 5 runs) ..."
  perf_out="$(perf stat -r 5 \
    -e cycles,instructions,cache-references,cache-misses,branch-instructions,branch-misses \
    taskset -c "$CORES" "$BIN" $RUN_ARGS 2>&1 1>/dev/null)"

  # perf appends :u (user-space) when paranoid>1; parse raw counts and compute ratios
  _val() { echo "$perf_out" | awk -v e="$1" '$2==e {gsub(/,/,"",$1); print $1+0}'; }
  cycles=$(_val "cycles:u");        instrs=$(_val "instructions:u")
  crefs=$(_val "cache-references:u"); cmiss=$(_val "cache-misses:u")
  branchn=$(_val "branch-instructions:u"); bmiss=$(_val "branch-misses:u")

  ipc=$(awk        -v c="$cycles"  -v i="$instrs"  'BEGIN{if(c>0) printf "%.2f",i/c;   else print "n/a"}')
  cache_miss=$(awk -v r="$crefs"   -v m="$cmiss"   'BEGIN{if(r>0) printf "%.2f",m/r*100; else print "n/a"}')
  branch_miss=$(awk -v b="$branchn" -v m="$bmiss"  'BEGIN{if(b>0) printf "%.4f",m/b*100; else print "n/a"}')

  echo "---------------------------------------------"
  echo "Hardware counters (avg over 5 perf runs):"
  echo "  IPC:              ${ipc:-n/a}"
  echo "  Cache miss rate:  ${cache_miss:-n/a}%"
  echo "  Branch miss rate: ${branch_miss:-n/a}%"
  echo "---------------------------------------------"
fi

rm -f "$BIN"
