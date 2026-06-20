# TCC Context — LLMs for Sequential-to-Parallel C Code Conversion

This file captures all decisions made while planning the TCC, so any tool
(including Claude Code) can pick up exactly where things stand without
re-explaining. It is decision-focused, not a transcript.

---

## Title
**Evaluating LLMs on the Conversion of Sequential Code to Parallel Code in C**

(Chosen over comparison-focused or C-focused titles: C is a scope delimiter,
not the subject; comparison is a secondary angle, not the headline.)

## Institution / template
- UNIPAMPA, Ciência da Computação, abnTeX2 template (chapter-based).
- Body text in **English**; a Portuguese `resumo` is still required by ABNT.
- Citations use `abntex2cite` (alf) -> `\citeonline{KEY}`.
- PAMPAR bib key in use: `GARCIA:PAMPAR:2020`.

---

## Research Questions

**Main:** What is the capability of current LLMs in converting sequential code
to parallel code in C?

- **RQ1** — Do the generated parallel codes compile and produce correct results?
- **RQ2** — What is the speedup vs. the original sequential code? (S = T_seq/T_par)
- **RQ3** — Most common limitations/failures: what stopped the code from being
  correct or efficient?
- **RQ4** — How do the different LLMs compare on correctness and efficiency?

RQ1 = correctness baseline. RQ2 = performance. RQ3 = qualitative failure
analysis. RQ4 = synthesis of RQ1+RQ2 across models (no new data needed).

---

## Input Programs (from PAMPAR benchmark)

Source: PAMPAR (Garcia, Schepke, Girardi, 2020). Chosen because it ships
validated sequential code AND reference OpenMP versions (ground-truth for
correctness). 5 programs selected to span parallelization difficulty:

| Key | Program               | Complexity | Difficulty |
|-----|-----------------------|------------|------------|
| DP  | Dot product           | O(n)       | Easy       |
| NI  | Numerical integration | O(n)       | Easy       |
| MM  | Matrix multiplication | O(n^3)     | Medium     |
| OE  | Odd-even sort         | O(n^2)     | Hard       |
| DJ  | Dijkstra              | O(n^3)     | Hard       |

DJ is special: it reads a graph from an external file (graph_256.txt /
graph_1024.txt). That file is runtime input for OUR testing only — it is NOT
given to the LLMs. Models only ever see `sequential.c`.

---

## Models (5, all via free official web interfaces)

ChatGPT, Gemini, Claude, Meta AI, DeepSeek.

Accessed through web UI (not API) because the study is manual, not automated —
API gives no advantage for ~25 hand-collected prompts. Record exact model
version + date of access for each interaction (reproducibility).

---

## Prompting Strategy

One prompt TEMPLATE, sent identically to all models, each in a FRESH/independent
conversation. One generation attempt per (program, model) as the primary
measurement (no iterative correction).

**Primary prompt (OpenMP-specified):**
> I have a sequential C program that I want to parallelize using OpenMP. Please
> convert it to a parallel version while keeping the same logic and producing the
> same output as the original. Return only the complete parallel C code, ready to
> compile, without explanations.
>
> [contents of sequential.c]

**Secondary prompt (optional, if time):** generic "parallelize however is best"
— to observe what technology each model chooses unconstrained. Not required for
the main RQs; treated as a complementary analysis.

Scope decision: OpenMP only for the MVP. Pthreads/other PPIs are documented
future work if time allows.

---

## Metrics & Procedure

- **Compilation (RQ1):** gcc with -fopenmp. Fail -> record cause.
- **Correctness (RQ1):** compare parallel output value against sequential
  output value (same input). Must match.
- **Speedup (RQ2):** S = T_seq / T_par.
- **Failure type (RQ3):** (i) didn't compile, (ii) compiled but wrong output,
  (iii) correct but no speedup. Note cause when identifiable.
- **Comparison (RQ4):** tabulate the above across the 5 models.

**Runs:** 20 executions per program, averaged. Justified via Hunold &
Carpen-Amarie (cited inside PAMPAR), adapted to OpenMP's lower variance.

**Measurement hygiene:** single consistent machine, close background apps,
pin cores with taskset, same compile flags everywhere. Measure all programs
(sequential + every LLM version) through the SAME script for fairness.

---

## Environment notes
- OpenMP is CPU-only; the RTX 3060 is irrelevant. Max realistic speedup is
  bounded by physical CPU core count -> set `nproc` and update CORES in
  measure.sh.
- Reduced input sizes in measure.sh keep 20-run loops fast. Make sure each
  program runs long enough to measure meaningfully (DJ at 256 is too fast;
  use graph_1024 or raise sizes).

---

## CURRENT STATUS  (where we are right now)

Planning + protocol: **DONE.**
Methodology chapter (metodologia.tex): **WRITTEN**, adapted to abnTeX2.
Experimental setup packaged: **DONE** (programs/, reference/, measure.sh).

### >>> WE ARE HERE: Step 1 — Baseline (sequential) measurement <<<

Immediate next actions, in order:
1. On the real machine: `nproc`, update CORES in measure.sh, install bc if needed.
2. Run the 5 sequential programs through measure.sh; record:
   - average execution time (this is T_seq, the speedup denominator)
   - the RESULT VALUE each program prints (this is the correctness oracle)
   Commands:
     ./measure.sh programs/DP/sequential.c DP
     ./measure.sh programs/NI/sequential.c NI
     ./measure.sh programs/MM/sequential.c MM
     ./measure.sh programs/OE/sequential.c OE
     ./measure.sh programs/DJ/sequential.c DJ
3. Sanity-check that times aren't too small; bump input sizes if needed.

### Then: Step 2 — LLM generation
Paste each sequential.c into each of the 5 models with the primary prompt,
fresh chat each time. Save outputs as outputs/<KEY>_<model>.c
(e.g. DP_claude.c, DP_gemini.c, DP_chatgpt.c, DP_metaai.c, DP_deepseek.c).
25 files total (5 programs x 5 models).

### Then: Step 3 — Measure & compare
Run each LLM version through measure.sh with a thread count, compute speedup,
check correctness against the oracle, categorize any failures, tabulate by model.

---

## Pending decisions / to confirm with orientador
- "Fonte:" vs "Source:" label in tables (template defaults to Portuguese).
- Portuguese resumo wording.
- Final input sizes / thread count to standardize on.
