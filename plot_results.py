#!/usr/bin/env python3
"""Plots the thread-sweep results for the paper.

Speedup is S = mean(T_seq) / mean(T_par). In every file (sequential,
reference and LLM alike), runs outside Tukey's fences
[Q1 - 1.5*IQR, Q3 + 1.5*IQR] are discarded before taking the mean.
Also writes speedups.csv (one row per program/version/thread count).

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


runs_total = runs_removed = 0


def mean_time(path):
    """Mean and std dev of the runs inside Tukey's fences, and how many were kept."""
    global runs_total, runs_removed
    runs = [float(x) for x in re.findall(r"run +\d+: ([0-9.]+) s", path.read_text())]
    if len(runs) == 0:
        sys.exit(f"no runs in {path}")
    q1, _, q3 = st.quantiles(runs, n=4)
    lo, hi = q1 - 1.5 * (q3 - q1), q3 + 1.5 * (q3 - q1)
    kept = [x for x in runs if lo <= x <= hi]
    runs_total += len(runs)
    runs_removed += len(runs) - len(kept)
    return st.mean(kept), st.stdev(kept), len(kept)


csv_rows = ["program,version,threads,mean_time_s,std_dev_s,runs_kept,speedup"]


def speedups(key, version):
    t_seq = T_SEQ[key]
    out = []
    for t in THREADS:
        if version == "reference":
            f = RESULTS / "reference_results" / f"{key}_reference_t{t}.txt"
        else:
            f = RESULTS / "llm_results" / f"{key}_{version}_t{t}.txt"
        mean, std, kept = mean_time(f)
        out.append(t_seq / mean)
        csv_rows.append(f"{key},{version},{t},{mean:.6f},{std:.6f},{kept},{t_seq / mean:.3f}")
    return out


T_SEQ = {}
for key, _ in PROGRAMS:
    mean, std, kept = mean_time(RESULTS / "baseline" / f"{key}_sequential.txt")
    T_SEQ[key] = mean
    csv_rows.append(f"{key},sequential,1,{mean:.6f},{std:.6f},{kept},1.000")
S ={k: {v: speedups(k, v) for v in ["reference"] + [m for m, _ in MODELS]}
     for k, _ in PROGRAMS}
(RESULTS / "speedups.csv").write_text("\n".join(csv_rows) + "\n")


def draw_lines(ax, series, ymax=None, ideal=True):
    """SMT band, ideal line, reference and models on one threads-vs-speedup panel.
    Series that go above ymax are skipped (the caller annotates them)."""
    ax.axvspan(PHYSICAL_CORES, THREADS[-1] + 1, color=BAND, lw=0, zorder=0)
    if ideal:
        ax.plot([1, THREADS[-1] + 1], [1, THREADS[-1] + 1], ls=":", lw=1, color=AXIS, zorder=1)
    ax.plot(THREADS, series["reference"], color=REF_COLOR, lw=1.5, ls="--",
            marker="x", ms=3.5, zorder=3)
    for (m, _), c, mk in zip(MODELS, COLORS, MARKERS):
        if ymax is not None and max(series[m]) > ymax:
            continue
        ax.plot(THREADS, series[m], color=c, lw=1.5, marker=mk, ms=3.5,
                markeredgecolor=SURFACE, markeredgewidth=0.5, zorder=4)
    ax.set_xlim(1, THREADS[-1] + 1)
    ax.set_xticks([4, 8, 12, 16, 20, 24])
    ax.grid(axis="y", color=GRID, lw=0.6)
    ax.set_axisbelow(True)
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)


def legend_entries(ideal=True):
    handles = [plt.Line2D([], [], color=REF_COLOR, ls="--", marker="x", ms=3.5, lw=1.5)]
    handles += [plt.Line2D([], [], color=c, marker=mk, ms=3.5, lw=1.5,
                           markeredgecolor=SURFACE, markeredgewidth=0.5)
                for c, mk in zip(COLORS, MARKERS)]
    labels = ["PAMPAR reference"] + [n for _, n in MODELS]
    if ideal:
        handles += [plt.Line2D([], [], color=AXIS, ls=":", lw=1)]
        labels += ["Ideal"]
    return handles, labels


def save(fig, name):
    for ext in ("pdf", "png"):
        fig.savefig(OUT / f"{name}.{ext}", dpi=200)
    plt.close(fig)


# ---- Figure 1: speedup vs threads, one panel per program -------------
YMAX = 15
fig, axes = plt.subplots(2, 3, figsize=(7.16, 4.3), sharex=True, sharey=True)
for ax, (key, name) in zip(axes.flat, PROGRAMS):
    draw_lines(ax, S[key], ymax=YMAX)
    y = S[key]["deepseek"]
    if max(y) > YMAX:  # DeepSeek TR: algorithmic change, off scale
        ax.annotate(f"DeepSeek: {min(y):.1f}–{max(y):.1f}×\n(off scale, algorithmic change)",
                    xy=(0.03, 0.97), xycoords="axes fraction", va="top",
                    fontsize=6.5, color=INK2)
    ax.set_title(f"{key} – {name}", color=INK, loc="left", pad=4)
    ax.set_ylim(0, YMAX)
    ax.set_yticks([0, 5, 10, 15])
for ax in axes[1]:
    ax.set_xlabel("Threads")
for ax in axes[:, 0]:
    ax.set_ylabel("Speedup")
axes[0, 0].text(18, 0.6, "SMT", ha="center", fontsize=6.5, color=MUTED)

fig.legend(*legend_entries(), loc="upper center", ncol=7, frameon=False,
           bbox_to_anchor=(0.5, 1.0), handlelength=2.2, columnspacing=1.2)
fig.tight_layout(rect=(0, 0, 1, 0.94))
save(fig, "speedup_vs_threads")


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
save(fig, "relative_to_reference")


# ---- Figure 3: average speedup per model (TR excluded, as in the paper)
AVG_KEYS = ["DP", "NI", "MM", "OE", "DFT"]
avg = {v: [st.mean(S[k][v][i] for k in AVG_KEYS) for i in range(len(THREADS))]
       for v in S["DP"]}
fig, ax = plt.subplots(figsize=(3.5, 2.7))
draw_lines(ax, avg)
ax.set_ylim(0, 12)
ax.set_xlabel("Threads")
ax.set_ylabel("Average speedup")
ax.text(18, 0.5, "SMT", ha="center", fontsize=6.5, color=MUTED)
fig.legend(*legend_entries(), loc="upper center", ncol=4, frameon=False,
           bbox_to_anchor=(0.5, 1.0), handlelength=2.2, columnspacing=1.0, fontsize=6.5)
fig.tight_layout(rect=(0, 0, 1, 0.86))
save(fig, "average_speedup_vs_threads")


# ---- Figure 4: Turing Ring on a log scale, so DeepSeek fits ----------
fig, ax = plt.subplots(figsize=(3.5, 2.7))
draw_lines(ax, S["TR"], ideal=False)
ax.set_yscale("log")
ax.set_ylim(0.5, 60)
ax.set_yticks([0.5, 1, 2, 5, 10, 20, 50], ["0.5", "1", "2", "5", "10", "20", "50"])
ax.minorticks_off()
ax.grid(axis="y", color=GRID, lw=0.6)
ax.set_xlabel("Threads")
ax.set_ylabel("Speedup (log scale)")
ax.text(18, 0.6, "SMT", ha="center", fontsize=6.5, color=MUTED)
fig.legend(*legend_entries(ideal=False), loc="upper center", ncol=3, frameon=False,
           bbox_to_anchor=(0.5, 1.0), handlelength=2.2, columnspacing=1.0, fontsize=6.5)
fig.tight_layout(rect=(0, 0, 1, 0.86))
save(fig, "turing_ring_vs_threads")

print(f"Tukey's fences removed {runs_removed} of {runs_total} runs "
      f"({100 * runs_removed / runs_total:.1f}%)")
print(f"Wrote {RESULTS}/speedups.csv")
print(f"Wrote speedup_vs_threads, relative_to_reference, average_speedup_vs_threads "
      f"and turing_ring_vs_threads (.pdf, .png) to {OUT}/")
