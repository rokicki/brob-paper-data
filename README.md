# Reproducing the move-count data of the brobdicube paper

This repository regenerates the performance graph (Figure `fig:main-graph`)
and the move-count tables (Tables `tab:maketable5`, `tab:maketable5b`,
`tab:maketable6`, `tab:discrete-ub`) of the paper.  It also reproduces the
published graph and tables from the previously collected data, so every
change can be traced.

```
make            # build brobdicube, run it, draw the graph, write the tables, check
make paper PAPER_SRC=/path/to/paper/sources PAPER_OUT=/path/to/output
                # copy the paper with the new graph and tables to PAPER_OUT
```

Requirements: a C++20 compiler, Python 3 with matplotlib, about 16 GB of
memory.  `make data` takes about an hour on 8 free cores (it is resumable);
the measured runs for n > 2^24 need a big-memory machine (see below).

## Layout

| Path | Contents |
|------|----------|
| `brobdicube/` | solver sources and algorithm library (snapshot of `brob-private`, see below) |
| `sortnet96/` | `solve96`: sortnet96 (`-6 -8`) and the RCube model (default mode) |
| `permnet72/` | the old 72-stage brobdicube (permnet72) |
| `tests/cubesim.py` | independent center-sticker simulator used to check both |
| `olddata/` | the raw data behind the published graph and tables, with the old Perl scripts |
| `data/legacy/` | `olddata/` converted to `<solver>_<kind>.csv` (n, moves) by `scripts/import_olddata.py` |
| `data/brobdicube/` | new brobdicube data: `model.csv`, `worst.csv`, `sim.csv`; raw runs in `runs/` |
| `scripts/sizes.py` | the cube sizes (all even) |
| `scripts/run_brobdicube.py` | runs brobdicube |
| `scripts/run_others.py` | runs solve96, permnet72 and brob2 (permnet62..67, greedy/optimal; `tune` picks their widths) (`data/<solver>/sim.csv`) |
| `scripts/permnet_bounds.py` | worst-case bounds of permnet62..67 from their algorithm sets |
| `scripts/provenance.py` | `provenance.py SOLVER N`: the runs behind a number and each run's full output |
| `scripts/results_table.py` | `results.txt`: every case, size and swept width (moves, stderr, samples, worst, model) |
| `scripts/fullcube_ub.py` | `fullcube_ub.txt`: whole-cube worst-case bounds (centers from brobdicube `-w`, exact; wings; edges and corners) |
| `scripts/bounds.py` | worst-case bounds of the other solvers (`data/<solver>/upper.csv`) |
| `scripts/make_graph.py` | `figures/main_graph.*` and the scaling comparisons |
| `scripts/make_tables.py` | `tables/*.tex` (rows in the paper's format) and `tables/*.csv` |
| `scripts/decompositions.py` | `tables/decomp_{worst,avg,worst_p1,avg_p1}.txt`: the decomposition brobdicube picks, by n (from its `-P list` logs; `*_p1` use one-row strips, `-p 1`, from `run_brobdicube.py worst-p1 model-p1`); `tables/decomp_{worst,avg,keys}.tex`: the two `-p max` decomposition tables and the key table for the paper (the `_p1` files are text only; the section prose is kept outside the repo) |
| `scripts/compare_tables.py` | cell-by-cell comparison with the tables in a `main.tex` |
| `scripts/crossovers.py` | which solver has the smallest worst-case bound, by n |
| `scripts/update_paper.py` | copies the paper sources to a directory outside this repo (`PAPER_OUT`, default `../paper`) and drops in the new graph and table rows; the paper itself is not part of the repo |
| `figures/check/`, `tables/check/` | the published graph and tables, redrawn from `olddata/` |

## What is computed, and how

All counts are **center moves only**, for even n.

* **Worst-case upper bound** (`worst.csv`): `brobdicube -w -p max -P list`,
  the worst-case model with each cube planned as one strip of all
  `My = (n-2)/2` rows.  A bound, not a sample: no noise.
* **Expected-cost model** (`model.csv`): `brobdicube -p max -P list`, the
  exact per-stage probability model.  Used where nothing is measured.
* **Simulation** (`sim.csv`): measured center moves of uniformly random
  cubes, `brobdicube -x -M K -z SEED -p P n`:
  * n ≤ 4096: whole cubes (`P = My`), K independent solves per run;
  * n > 4096: one strip of P rows (64 ≤ P ≤ 1024, about 1.5·10^8 orbits)
    per sample, using the whole-cube plan (see "`-x`" below);
  * K comes from a pilot run, aiming at a standard error of 0.05% of the
    mean (at least 32 solves for n ≤ 256, 8 for n ≤ 1024, 4 for n ≤ 4096,
    2 strips above); the largest standard error obtained is 0.064% (n=4).
  * Each run has its own seed `n*1000 + run`.  Seeds reproduce runs exactly
    on the same platform; `std::shuffle` differs between libstdc++ and
    libc++, so Linux and macOS give different (statistically equivalent)
    samples.
  * n ≤ 2^24 was measured on a 16-core laptop; 2^25 ≤ n ≤ 2^30 on a
    1.5 TB machine (a 2^30 strip needs about 1.2 TB and 16 hours), with
    `--runs-file sim-<host>.csv --sizes ...`; copy that file into
    `data/brobdicube/runs/` and run the `summary` step.
  * Measured and model agree to 10^-5 for n ≥ 65536; for n ≤ 26 the model
    is 0.5–1.7% high (many standard errors), so the measured values matter.
* `lower` (`common.lower`): the information-theoretic bound of
  `bounds/lb.py` (`olddata/bounds/lb.txt`), the smallest L with
  9(n−2)·(9n−3)^(L−1) ≥ (24!/(4!)^6)^((n/2−1)^2).  The right side is the
  number of center states (the (n/2−1)^2 orbits are independent).  In slice
  turn metric there are 9n moves (3 axes × n layers × 3 turns), and turning
  the layer just turned again is never useful, so each move has 9n−3 useful
  successors; since only centers are counted, a face turn is never a useful
  last move, so the last move is one of the 9(n−2) inner-slice moves.  This
  reproduces all 2191 values of `lb.txt` exactly (it uses lb.py's own float
  expression, plus an exact integer check for small n).  It is the paper's
  f(n) with 9n−3 in place of 9n, rounded up: +10% at n=4, +1.1% at n=8,
  +0.2% at n=32, < 0.01% for n ≥ 1024, same constant 12.88.  The submitted
  tables used yet another variant (9n−22, `common.lower_published_tables`),
  kept only to reproduce them.
* `0D_upper`: 6 moves per center cubie.

## Reproducing the published graph and tables

`make check` redraws the published graph and tables from `olddata/` (the
old permnet62 data in the last column) into `figures/check/` and
`tables/check/`, and compares the tables with the paper.  All cells of
Tables 5, 5b and 6 match, except six permnet62 cells at
n ≥ 671,088,640 that the paper prints as "--" although `just62.txt` has
values there.  It then lists how the new tables differ from the published
ones.

## Graph scaling

The graph now plots moves·lg n/n² up to 210, with the number of center
cubies, 6(n−2)², scaled the same way (dotted): curves below it use less
than one move per center cubie.  The submitted graph plotted
moves·lg n/(n−2)², although its axis label read "moves * n lg(n − 2)" and
its caption said "normalized by n²/lg n"; `figures/check/` redraws it.
`make_graph.py --scale {lgnn2,lgn2n2,lgn,lgn2}` normalizes by n²/lg n,
n²/lg(n−2), (n−2)²/lg n or (n−2)²/lg(n−2); `--scale all` draws all four
(`figures/scaling_compare.*`); `--ymax` sets the top of the y axis and
`--no-centers` drops the center-count line.

## The other solvers

brobdicube's own curves are labelled **permnet61** (the submitted paper's
best solver was permnet62).  The other four solvers are regenerated from the
two older programs here (`make others bounds`); where `data/<solver>/` has
no file yet, the previously collected data in `data/legacy/` is used and
the scripts say so.

| Solver | Program | Average (`sim.csv`) | Worst case (`upper.csv`) |
|---|---|---|---|
| sortnet96 | `sortnet96/solve96 -6 -8 -w W` | measured | 108 + 96·strips(n) |
| rcube1d | `sortnet96/solve96 -w 1` | measured | ShellPuppy's published formula |
| rcube2d | `sortnet96/solve96 -w W` | measured | min(published, 264 + 240·strips) |
| permnet72 | `permnet72/brobdicube -w W` | measured | 79 + 72·strips(n) |

* solve96's default mode (no `-6`/`-8`) is the RCube/ShellPuppy model: 240
  stages whose setup moves sum to 264, the constants of the RCube 2D bound.
  1D and 2D differ only in the width: 1D always works one row at a time.
* All runs process the whole cube per pass (`-p My`), as the published data
  did; above n = 4096 one strip of whole width groups is solved and scaled
  up (`-x`; a strip that is not a multiple of the width biases the estimate
  high).  W is the width the published data used (`olddata/arena/*.txt`,
  column 2).  With these settings both programs reproduce the published
  averages (n ≤ 512 within noise; 4096..2^20 within 0.08%).
* strips(n) is the cost of one stage over the cube split into strips of the
  best two adjacent widths; a strip of width w over m columns costs
  2m + 4(2^w−1) + w·2^w.  The constant is each stage's setup overhead
  (algorithm length − 8, paid once per stage), summed over the stages:
  exactly 108 for sortnet96's 96 stages and 79 for permnet72's 72 (the old
  `permwc.py` used 108).  `bounds.py` reproduces the old bound files
  exactly apart from that.
* Both programs were fixed and extended for this (see `progress.md` in the
  parent directory): solve96's cancellation counting, permnet72's `-i`
  with turned outer faces, a `-z` seed option, and 15-digit output.

## Changes made to brobdicube

`brobdicube/` is `brob-private` branch `paper-data-fixes` (commit
`ad93748`, on top of `dc8b5fd`), with these changes:

1. **`-x` was biased.**  It planned for a cube only `-p` rows tall and
   multiplied that strip by `My/p`; a short strip's plan is worse than the
   whole cube's, so results were up to 7% high.  Now `-x` plans the whole
   cube, solves a sample of each stage's width blocks inside the strip and
   scales each stage by its block counts.
2. **`-z SEED`**: reproducible scrambles, independent of thread count.
3. **Worst-case widths were capped at 24** in `wccost` (unlike `avcost`),
   so the bound grew again past n ≈ 2^33; it now keeps decreasing.  No
   change for n ≤ 2^30.
4. **Strip widths are printed** after each `Use` line (`Widths {w^c,...}xK`).
5. **`-P list`** reads the sizes to plan from stdin; **`-p max`** plans
   each cube as one strip of all rows.

The current brobdicube reproduces the old `brob4wc` worst-case data exactly
at every size.  The old permnet62 (`just62`) numbers do not follow from
the current code and algorithm files restricted to the permnet62 pair
(`000111-0026`, `012xxx-0018`), so they cannot be regenerated here.

See `NOTES.md` for the differences from the published numbers and the
statements in the paper to recheck.

## Provenance

Every run appends a row to `data/<solver>/runs/<file>.csv` and its complete
output (first line: the solver's command line) to the matching `.out` file;
width-tuning runs at widths not chosen are kept too (not pooled).  `replay`
(both runners) reruns stored rows that have no saved output, checks they
reproduce exactly and saves the output.  Runs reproduce exactly only on the
platform that made them: the solvers scramble with `std::shuffle`, which
differs between libstdc++ (Linux) and libc++ (macOS).
