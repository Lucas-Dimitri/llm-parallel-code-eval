#!/usr/bin/env bash
# Checks that every reference and LLM version prints the same result as the
# sequential program, at each thread count. One run per configuration.
# Writes a report to correctness_results.txt.
#
# USAGE:
#   ./check_correctness.sh              # 2,4,6,...,24 threads (capped at nproc)
#   ./check_correctness.sh 2 4 8        # custom thread counts
#   KEYS="NI DFT" ./check_correctness.sh

set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUT="$SCRIPT_DIR/correctness_results.txt"

# Same compile flags and input sizes as measure.sh (read from it, so they never drift)
CFLAGS="-O2 -fopenmp -DELAPSEDTIME"
LDLIBS="-lm"
declare -A INPUT_ARGS
eval "$(grep -E '^INPUT_ARGS\[' "$SCRIPT_DIR/measure.sh")"

KEYS=(${KEYS:-DP NI MM OE DFT TR})
MODELS=(chatgpt gemini claude metaAI deepseek)
if [[ $# -gt 0 ]]; then
  THREAD_LIST=("$@")
else
  THREAD_LIST=()
  for t in $(seq 2 2 24); do (( t <= $(nproc) )) && THREAD_LIST+=("$t"); done
fi

BIN="$(mktemp /tmp/tcc_chk.XXXXXX)"
trap 'rm -f "$BIN"' EXIT

# Prints the program's result, without the timing line
result() { "$@" 2>/dev/null | grep -v "Execution time"; }

compile() {
  gcc $CFLAGS "$1" -o "$BIN" $LDLIBS 2>/dev/null
}

{
echo "Correctness check - $(date '+%Y-%m-%d %H:%M') - $(uname -n) - threads: ${THREAD_LIST[*]}"
fails=0
for KEY in "${KEYS[@]}"; do
  ARGS="${INPUT_ARGS[$KEY]}"
  compile "$SCRIPT_DIR/programs/$KEY/${KEY}_sequential.c" || { echo "$KEY sequential COMPILE_FAILED"; exit 1; }
  expected="$(result "$BIN" $ARGS)"
  echo ""
  echo "== $KEY  (sequential result: $expected)"

  for VERSION in reference "${MODELS[@]}"; do
    if [[ "$VERSION" == reference ]]; then
      SRC="$SCRIPT_DIR/reference/${KEY}_openmp_reference.c"
    else
      SRC="$SCRIPT_DIR/LLM_programs/$KEY/${KEY}_${VERSION}.c"
    fi
    if ! compile "$SRC"; then
      echo "   $VERSION: COMPILE_FAILED"; fails=$((fails + 1)); continue
    fi
    diffs=()
    silent=0
    for T in "${THREAD_LIST[@]}"; do
      if [[ "$VERSION" == reference ]]; then
        got="$(OMP_NUM_THREADS=$T result "$BIN" $T $ARGS)"   # PAMPAR convention: <threads> <N>
      else
        got="$(OMP_NUM_THREADS=$T result "$BIN" $ARGS)"
      fi
      [[ -z "$got" ]] && silent=$((silent + 1))
      [[ "$got" == "$expected" ]] || diffs+=("t$T: $got")
    done
    if [[ $silent -eq ${#THREAD_LIST[@]} ]]; then
      # Some PAMPAR references have their result printf commented out
      echo "   $VERSION: prints no result (not checked)"
    elif [[ ${#diffs[@]} -eq 0 ]]; then
      echo "   $VERSION: MATCH at all thread counts"
    else
      echo "   $VERSION: DIFFERENT at ${#diffs[@]} thread count(s)"
      printf '      %s\n' "${diffs[@]}"
      fails=$((fails + 1))
    fi
  done
done
echo ""
echo "Versions with any difference: $fails"
} 2>&1 | tee "$OUT"
