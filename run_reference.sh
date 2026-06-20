#!/usr/bin/env bash
# Runs measure.sh on all reference OpenMP versions at THREADS threads
# and saves output to reference_results/

set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUT_DIR="$SCRIPT_DIR/reference_results"
THREADS="${1:-11}"
mkdir -p "$OUT_DIR"

KEYS=(DP NI MM OE DFT TR)

for KEY in "${KEYS[@]}"; do
  SRC="$SCRIPT_DIR/reference/${KEY}_openmp_reference.c"
  OUT="$OUT_DIR/${KEY}_reference_t${THREADS}.txt"
  echo "=============================="
  echo ">> $KEY (${THREADS} threads)"
  echo "=============================="
  bash "$SCRIPT_DIR/measure.sh" "$SRC" "$KEY" "$THREADS" yes 2>&1 | tee "$OUT"
  echo ""
done

echo "=============================="
echo "All done. Results saved in $OUT_DIR/"
echo "=============================="