#!/usr/bin/env bash
# Runs measure.sh on all LLM-generated parallel versions at THREADS threads
# and saves output to llm_results/

set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUT_DIR="$SCRIPT_DIR/llm_results"
THREADS="${1:-11}"
mkdir -p "$OUT_DIR"

KEYS=(${KEYS:-DP NI MM OE DFT TR})   # override, e.g. KEYS="DFT" ./run_sweep.sh
MODELS=(chatgpt gemini claude metaAI deepseek)

for KEY in "${KEYS[@]}"; do
  for MODEL in "${MODELS[@]}"; do
    SRC="$SCRIPT_DIR/LLM_programs/$KEY/${KEY}_${MODEL}.c"
    OUT="$OUT_DIR/${KEY}_${MODEL}_t${THREADS}.txt"
    echo "=============================="
    echo ">> $KEY / $MODEL (${THREADS} threads)"
    echo "=============================="
    bash "$SCRIPT_DIR/measure.sh" "$SRC" "$KEY" "$THREADS" 2>&1 | tee "$OUT"
    echo ""
  done
done

echo "=============================="
echo "All done. Results saved in $OUT_DIR/"
echo "=============================="
