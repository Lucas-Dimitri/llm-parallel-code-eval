#!/usr/bin/env bash
# Runs measure.sh on all sequential programs and saves output to baseline/

set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUT_DIR="$SCRIPT_DIR/baseline"
mkdir -p "$OUT_DIR"

KEYS=(${KEYS:-DP NI MM OE DFT TR})   # override, e.g. KEYS="DFT" ./run_sweep.sh

for KEY in "${KEYS[@]}"; do
  SRC="$SCRIPT_DIR/programs/$KEY/${KEY}_sequential.c"
  OUT="$OUT_DIR/${KEY}_sequential.txt"
  echo "=============================="
  echo ">> $KEY"
  echo "=============================="
  bash "$SCRIPT_DIR/measure.sh" "$SRC" "$KEY" 2>&1 | tee "$OUT"
  echo ""
done

echo "=============================="
echo "All done. Results saved in $OUT_DIR/"
echo "=============================="
