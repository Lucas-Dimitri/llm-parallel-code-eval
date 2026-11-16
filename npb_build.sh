#!/usr/bin/env bash
# =====================================================================
# npb_build.sh - builds one NPB 3.3.1 benchmark with NASA's own Makefiles
#
# The build runs in a temporary copy of the NPB tree, so the official
# sources in npb/ are never modified. Only config/make.def is written
# (compiler and flags), as NPB's install instructions ask.
#
# USAGE:
#   ./npb_build.sh <source_dir> <KEY> <class> <output_binary>
#
#   <source_dir>   npb/NPB3.3-SER/<KEY>  -> NASA serial version (no OpenMP)
#                  npb/NPB3.3-OMP/<KEY>  -> NASA OpenMP version (reference)
#                  any other directory   -> an LLM version: its files replace
#                                           the serial ones, built with OpenMP
#   <KEY>          CG, MG or FT
#   <class>        NPB problem class (S, W, A, B, C, ...)
# =====================================================================

set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC="$(cd "$1" && pwd)"
KEY="$2"
CLASS="$3"
OUT="$4"
BENCH="$(echo "$KEY" | tr '[:upper:]' '[:lower:]')"

FFLAGS="-O2 -fallow-invalid-boz"    # same optimization level as the C programs;
                                    # the flag lets gfortran >= 10 accept NPB 3.3's
                                    # old X'..' hex constants (no code change)
CFLAGS="-O2"

if [[ "$SRC" == "$SCRIPT_DIR/npb/NPB3.3-OMP/$KEY" ]]; then
  TREE="NPB3.3-OMP"; OPENMP=yes
elif [[ "$SRC" == "$SCRIPT_DIR/npb/NPB3.3-SER/$KEY" ]]; then
  TREE="NPB3.3-SER"; OPENMP=no
else
  TREE="NPB3.3-SER"; OPENMP=yes
fi
[[ "$OPENMP" == yes ]] && { FFLAGS="$FFLAGS -fopenmp"; CFLAGS="$CFLAGS -fopenmp"; }

WORK="$(mktemp -d /tmp/tcc_npb.XXXXXX)"
trap 'rm -rf "$WORK"' EXIT
cp -r "$SCRIPT_DIR/npb/$TREE" "$WORK/npb"
[[ "$SRC" != "$SCRIPT_DIR/npb/$TREE/$KEY" ]] && cp "$SRC"/* "$WORK/npb/$KEY/"

sed -e "s|^F77 .*|F77 = gfortran|" \
    -e "s|^FFLAGS.*|FFLAGS = $FFLAGS|" \
    -e "s|^FLINKFLAGS.*|FLINKFLAGS = $FFLAGS|" \
    -e "s|^CC .*|CC = gcc|" \
    -e "s|^CFLAGS.*|CFLAGS = $CFLAGS|" \
    -e "s|^CLINKFLAGS.*|CLINKFLAGS = $CFLAGS|" \
    -e "s|^UCC.*|UCC = gcc|" \
    "$WORK/npb/config/make.def.template" > "$WORK/npb/config/make.def"

make -C "$WORK/npb" "$BENCH" CLASS="$CLASS"
cp "$WORK/npb/bin/$BENCH.$CLASS.x" "$OUT"
