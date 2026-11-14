#!/usr/bin/env python3
"""Plots the thread-sweep results for the paper.

Speedup is S = mean(T_seq) / mean(T_par) over the 30 runs of each file
(no outlier removal).

USAGE:
  python3 plot_results.py [results_dir] [output_dir]
  defaults: results_vm/  article/figuras/
"""
import re
import sys
import statistics as st
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap, TwoSlopeNorm

RESULTS = Path(sys.argv[1] if len(sys.argv) > 1 else "results_vm")
OUT = Path(sys.argv[2] if len(sys.argv) > 2 else "article/figuras")
OUT.mkdir(parents=True, exist_ok=True)

THREADS = list(range(2, 25, 2))
PHYSICAL_CORES = 12
PROGRAMS = [("DP", "Dot Product"), ("NI", "Numerical Integration"),
            ("MM", "Matrix Multiplication"), ("OE", "Odd-Even Sort"),
            ("DFT", "Discrete Fourier Transform"), ("TR", "Turing Ring")]
MODELS = [("chatgpt", "ChatGPT"), ("gemini", "Gemini"), ("claude", "Claude"),
          ("metaAI", "Meta AI"), ("deepseek", "DeepSeek")]

# Validated categorical slots 1-5 (light surface); markers are the
# secondary encoding for print and grayscale.
COLORS = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100", "#e87ba4"]
MARKERS = ["o", "s", "^", "D", "v"]
REF_COLOR = "#52514e"
INK, INK2, MUTED = "#0b0b0b", "#52514e", "#898781"
GRID, AXIS, SURFACE, BAND = "#e1e0d9", "#c3c2b7", "#ffffff", "#f0efec"

plt.rcParams.update({
    "font.family": "sans-serif", "font.size": 7.5,
    "axes.edgecolor": AXIS, "axes.labelcolor": INK2, "axes.titlesize": 8,
    "xtick.color": MUTED, "ytick.color": MUTED,
    "xtick.labelcolor": INK2, "ytick.labelcolor": INK2,
    "pdf.fonttype": 42, "savefig.facecolor": SURFACE,
})


def mean_time(path):
    runs = [float(x) for x in re.findall(r"run +\d+: ([0-9.]+) s", path.read_text())]
    if len(runs) == 0:
        sys.exit(f"no runs in {path}")
    return st.mean(runs)


def speedups(key, version):
    t_seq = mean_time(RESULTS / "baseline" / f"{key}_sequential.txt")
    out = []
    for t in THREADS:
        if version == "reference":
            f = RESULTS / "reference_results" / f"{key}_reference_t{t}.txt"
        else:
            f = RESULTS / "llm_results" / f"{key}_{version}_t{t}.txt"
        out.append(t_seq / mean_time(f))
    return out


S = {k: {v: speedups(k, v) for v in ["reference"] + [m for m, _ in MODELS]}
     for k, _ in PROGRAMS}


# ---- Figure 1: speedup vs threads, one panel per program -------------
YMAX = 15
fig, axes = plt.subplots(2, 3, figsize=(7.16, 4.3), sharex=True, sharey=True)
for ax, (key, name) in zip(axes.flat, PROGRAMS):
    ax.axvspan(PHYSICAL_CORES, THREADS[-1] + 1, color=BAND, lw=0, zorder=0)
    ax.plot([0, YMAX], [0, YMAX], ls=":", lw=1, color=AXIS, zorder=1)
    ax.plot(THREADS, S[key]["reference"], color=REF_COLOR, lw=1.5, ls="--",
            marker="x", ms=3.5, zorder=3)
    for (m, _), c, mk in zip(MODELS, COLORS, MARKERS):
        y = S[key][m]
        if max(y) > YMAX:  # DeepSeek TR: algorithmic change, off scale
            ax.annotate(f"DeepSeek: {min(y):.1f}–{max(y):.1f}×\n(off scale, algorithmic change)",
                        xy=(0.03, 0.97), xycoords="axes fraction", va="top",
                        fontsize=6.5, color=INK2)
            continue
        ax.plot(THREADS, y, color=c, lw=1.5, marker=mk, ms=3.5,
                markeredgecolor=SURFACE, markeredgewidth=0.5, zorder=4)
    ax.set_title(f"{key} – {name}", color=INK, loc="left", pad=4)
    ax.set_xlim(1, THREADS[-1] + 1)
    ax.set_ylim(0, YMAX)
    ax.set_xticks([4, 8, 12, 16, 20, 24])
    ax.set_yticks([0, 5, 10, 15])
    ax.grid(axis="y", color=GRID, lw=0.6)
    ax.set_axisbelow(True)
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
for ax in axes[1]:
    ax.set_xlabel("Threads")
for ax in axes[:, 0]:
    ax.set_ylabel("Speedup")
axes[0, 0].text(18, 0.6, "SMT", ha="center", fontsize=6.5, color=MUTED)

handles = [plt.Line2D([], [], color=REF_COLOR, ls="--", marker="x", ms=3.5, lw=1.5)]
handles += [plt.Line2D([], [], color=c, marker=mk, ms=3.5, lw=1.5,
                       markeredgecolor=SURFACE, markeredgewidth=0.5)
            for c, mk in zip(COLORS, MARKERS)]
handles += [plt.Line2D([], [], color=AXIS, ls=":", lw=1)]
labels = ["PAMPAR reference"] + [n for _, n in MODELS] + ["Ideal"]
fig.legend(handles, labels, loc="upper center", ncol=7, frameon=False,
           bbox_to_anchor=(0.5, 1.0), handlelength=2.2, columnspacing=1.2)
fig.tight_layout(rect=(0, 0, 1, 0.94))
for ext in ("pdf", "png"):
    fig.savefig(OUT / f"speedup_vs_threads.{ext}", dpi=200)
plt.close(fig)


# ---- Figure 2: difference from the reference, models x threads -------
LIMIT = 40  # colour saturates at +/-40 %; cells show the exact value
cmap = LinearSegmentedColormap.from_list("div", ["#e34948", "#f0efec", "#2a78d6"])
norm = TwoSlopeNorm(vcenter=0, vmin=-LIMIT, vmax=LIMIT)

fig, axes = plt.subplots(2, 3, figsize=(7.16, 3.4), sharex=True, sharey=True)
for ax, (key, name) in zip(axes.flat, PROGRAMS):
    ref = S[key]["reference"]
    diff = [[100 * (s / r - 1) for s, r in zip(S[key][m], ref)] for m, _ in MODELS]
    ax.pcolormesh(diff, cmap=cmap, norm=norm, edgecolors=SURFACE, linewidth=1)
    for i, row in enumerate(diff):
        for j, v in enumerate(row):
            if abs(v) >= 10:
                strong = abs(v) >= 0.6 * LIMIT
                ax.text(j + 0.5, i + 0.5, f"{v:+.0f}", ha="center", va="center",
                        fontsize=4.2 if abs(v) >= 100 else 5,
                        color=SURFACE if strong else INK)
    ax.set_title(f"{key} – {name}", color=INK, loc="left", pad=4)
    ax.set_xticks([j + 0.5 for j in range(len(THREADS))], THREADS, fontsize=5.5)
    ax.set_yticks([i + 0.5 for i in range(len(MODELS))], [n for _, n in MODELS])
    ax.set_ylim(len(MODELS), 0)  # first model on top (axes are shared: no invert)
    ax.tick_params(length=0)
    for s in ax.spines.values():
        s.set_visible(False)
for ax in axes[1]:
    ax.set_xlabel("Threads")
fig.tight_layout(rect=(0, 0.16, 1, 1))
cax = fig.add_axes((0.3, 0.1, 0.4, 0.022))
cb = fig.colorbar(plt.cm.ScalarMappable(norm=norm, cmap=cmap), cax=cax,
                  orientation="horizontal", extend="both")
cb.outline.set_visible(False)
cb.set_ticks([-40, -20, 0, 20, 40])
cb.set_label("LLM speedup relative to PAMPAR reference (%)  —  slower ← → faster",
             color=INK2, fontsize=6.5)
cb.ax.tick_params(labelsize=6, length=0)
for ext in ("pdf", "png"):
    fig.savefig(OUT / f"relative_to_reference.{ext}", dpi=200)
plt.close(fig)

print(f"Wrote speedup_vs_threads and relative_to_reference (.pdf, .png) to {OUT}/")
