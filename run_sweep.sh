#!/usr/bin/env bash
# Runs the full experiment: sequential baseline once, then the reference
# and LLM versions at each thread count in THREAD_LIST.
#
# USAGE:
#   ./run_sweep.sh                 # 2,4,6,...,24 threads
#   ./run_sweep.sh 4 8 16          # custom thread counts

set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

if [[ $# -gt 0 ]]; then
  THREAD_LIST=("$@")
else
  THREAD_LIST=($(seq 2 2 24))
fi

MAX_CPU=$(nproc)
for T in "${THREAD_LIST[@]}"; do
  if (( T > MAX_CPU )); then
    echo "ERROR: $T threads requested but this machine has only $MAX_CPU CPUs"
    exit 1
  fi
done

bash "$SCRIPT_DIR/baseline.sh"

for T in "${THREAD_LIST[@]}"; do
  bash "$SCRIPT_DIR/run_reference.sh" "$T"
  bash "$SCRIPT_DIR/run_llm.sh" "$T"
done

echo "=============================="
echo "Sweep done for threads: ${THREAD_LIST[*]}"
echo "=============================="
