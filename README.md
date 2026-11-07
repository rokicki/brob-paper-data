# Center move counts for brobdicube and comparison solvers

This repository produces the data behind the performance graph (Figure
`fig:main-graph`) and the move-count tables (Tables `tab:maketable5`,
`tab:maketable5b`, `tab:maketable6`, `tab:discrete-ub`, and the
decomposition tables) of the brobdicube paper: the solvers, the scripts that
run them, every run's raw output, and the scripts that turn those runs into
the graph and the tables.

```
make            # build the solvers, run them, draw the graphs, write the tables, check
make check      # rerun the planner and a sample of the stored runs; they must reproduce
make paper PAPER_SRC=/path/to/paper/sources PAPER_OUT=/path/to/output
                # copy the paper sources to PAPER_OUT with the graph and tables dropped in
```

Requirements: a C++20 compiler, Python 3 with matplotlib, about 16 GB of
memory.  `make data` takes about an hour on 8 free cores and is resumable;
the measured runs for n > 2^24 need a big-memory machine (see below).  The
paper sources are not part of this repository; `make paper` writes outside
it.

## Layout

| Path | Contents |
|------|----------|
| `brobdicube/` | the solver: sources and algorithm library (see "brobdicube" below) |
| `brob2/` | `brob2`: the fixed two-level permutation networks permnet62..permnet67 |
| `permnet72/` | the 72-stage solver (permnet72) |
| `sortnet96/` | `solve96`: sortnet96 (`-6 -8`) and the RCube model (default mode) |
| `tests/cubesim.py` | independent center-sticker simulator, for checking `solve96` |
| `olddata/` | earlier runs of these programs and the bound scripts; their strip widths are where width tuning starts (`arena/`), and `bounds/` holds the bound formulas `scripts/bounds.py` and `common.lower` follow |
| `data/brobdicube/` | brobdicube: `worst.csv`, `model.csv`, `sim.csv`; raw runs and outputs in `runs/` |
| `data/<solver>/` | the other solvers: `sim.csv`, `upper.csv`, chosen `widths.csv`; raw runs in `runs/` |
| `data/logs/` | logs of the run queues, locally and on the big-memory machine |
| `results.txt` | every measured and computed number, one row per (solver, n, width) |
| `fullcube_ub.txt` | whole-cube worst-case bounds (centers, wings, edges, corners) |
| `figures/` | `main_graph.*` (the paper's figure), `scaling_compare.*`, `all_curves.*`, and `explorer.html` (interactive, every solver) |
| `tables/` | `*.tex` fragments for the paper and the same data as `*.csv` and `*.txt` |
| `scripts/` | everything else, listed below |

| Script | Writes |
|--------|--------|
| `sizes.py` | (the cube sizes; all even) |
| `common.py` | (shared formulas: lower bound, 0D bound, normalizations, data loading) |
| `run_brobdicube.py` | runs brobdicube: `model`, `worst`, `sim`, `model-p1`, `worst-p1`, `replay`, `summary` |
| `run_others.py` | runs `solve96`, `permnet72`, `brob2` and brobdicube restricted to one 61-stage network (`plan`, `tune`, `sim`, `replay`, `summary`) |
| `bounds.py` | `data/<solver>/upper.csv` for sortnet96, rcube1d, rcube2d, permnet72 |
| `permnet_bounds.py` | worst-case bounds of permnet62..67 from their algorithm sets |
| `make_graph.py` | `figures/main_graph.*`; `--scale all`: `figures/scaling_compare.*` |
| `make_tables.py` | `tables/maketable{5,5b,6}.*`, `tables/discrete_ub.*` |
| `decompositions.py` | `tables/decomp_{worst,avg,keys}.tex` and `tables/decomp_*.txt` (see below) |
| `results_table.py` | `results.txt` |
| `fullcube_ub.py` | `fullcube_ub.txt` |
| `explorer_data.py` | `figures/explorer.html` (from `explorer.src.html`) and `explorer_data.json` |
| `all_curves.py` | `figures/all_curves.*`: every solver on one plot |
| `crossovers.py` | prints which solver has the smallest worst-case bound, by n |
| `provenance.py` | `provenance.py SOLVER N`: the runs behind a number, with each run's output |
| `check.py` | `make check` |
| `update_paper.py` | `make paper` |

## What is computed, and how

All counts are **center moves only**, for even n.

* **Worst-case upper bound** (`worst.csv`): `brobdicube -w -p max -P list`,
  the worst-case cost of the plan brobdicube picks when each cube is planned
  as one strip of all `My = (n-2)/2` rows.  A bound, not a sample, so there
  is no noise; the `exact` column is the same bound as an exact integer.
* **Expected-cost model** (`model.csv`): `brobdicube -p max -P list`, the
  exact per-stage probability model.  Used where nothing is measured.
* **Simulation** (`sim.csv`): measured center moves of uniformly random
  cubes, `brobdicube -x -M K -z SEED -p P n`:
  * n ≤ 4096: whole cubes (`P = My`), K independent solves per run;
  * n > 4096: one strip of P rows (64 ≤ P ≤ 1024, about 1.5·10^8 orbits)
    per sample, using the whole-cube plan (see `-x` below);
  * K comes from a pilot run, aiming at a standard error of 0.05% of the
    mean (at least 32 solves for n ≤ 256, 8 for n ≤ 1024, 4 for n ≤ 4096,
    2 strips above); the largest standard error obtained is 0.064% (n=4).
  * Each run has its own seed `n*1000 + run`.
  * n ≤ 2^24 was measured on a 16-core laptop; 2^25 ≤ n ≤ 2^30 on a
    1.5 TB machine (a 2^30 strip needs about 1.2 TB and 16 hours), with
    `--runs-file sim-<host>.csv --sizes ...`; that file is copied into
    `data/brobdicube/runs/` and the `summary` step pools it.
  * Measured and model agree to 10^-5 for n ≥ 65536; for n ≤ 26 the model
    is 0.5–1.7% high (many standard errors), so the measured values matter.
* **Lower bound** (`common.lower`): the information-theoretic bound, the
  smallest L with 9(n−2)·(9n−3)^(L−1) ≥ (24!/(4!)^6)^((n/2−1)^2).  The right
  side is the number of center states (the (n/2−1)^2 orbits are
  independent).  In slice turn metric there are 9n moves (3 axes × n layers
  × 3 turns), and turning the layer just turned again is never useful, so
  each move has 9n−3 useful successors; since only centers are counted, a
  face turn is never a useful last move, so the last move is one of the
  9(n−2) inner-slice moves.  Leading constant 12.88.
* **0D upper bound** (`common.upper0d`): 6 moves per center cubie, 36(n−2)².

The smallest worst-case bound is the 0D bound for n ≤ 20 and brobdicube's
for every even n ≥ 22 (`scripts/crossovers.py`).

## The graph

`figures/main_graph.*` plots moves·lg n/n² against lg n, up to 210, with
the number of center cubies, 6(n−2)², scaled the same way (dotted): curves
below it use less than one move per center cubie.  Solid lines are
worst-case bounds, dashed lines simulated averages.
`make_graph.py --scale {lgnn2,lgn2n2,lgn,lgn2}` normalizes by n²/lg n
(the default), n²/lg(n−2), (n−2)²/lg n or (n−2)²/lg(n−2); `--scale all`
draws all four (`figures/scaling_compare.*`); `--ymax` sets the top of the
y axis and `--no-centers` drops the center-count line.

`figures/explorer.html` shows every solver and variant (permnet62..67 and
the 61-stage networks, greedy and optimal, and brobdicube with one-row
strips) with a checkbox each and the values on hover.

## brobdicube

`brobdicube/` is a snapshot of `brob-private`, branch `paper-data-fixes`,
commit `ad93748`.  brobdicube does not run one fixed network: for every n
it picks, from its library of 268 networks (`brobdicube/algs3/`), the chain
of reduction steps, the network for each step, greedy or optimal planning,
and each stage's strip widths, minimizing the expected cost, or with `-w`
the worst-case cost (see `NOTES.md`).  The options this repository relies
on:

* **`-P list`** plans the sizes read from stdin and prints the estimate and
  the plan (one `Use` line per step); **`-p max`** plans each cube as one
  strip of all rows.  After each `Use` line, `Widths {w^c,...}xK` gives the
  strip widths of its stages.
* **`-x`** estimates a whole-cube solve from one strip of `-p` rows: it
  plans the whole cube, solves a sample of each stage's width blocks inside
  the strip and scales each stage by its block counts.
* **`-z SEED`** makes the scrambles reproducible, independent of the
  thread count.

## Decompositions

`scripts/decompositions.py` reads the `-P list` logs in
`data/brobdicube/runs/` and lists, by n, the decomposition (chain of
reduction steps) brobdicube picks: `tables/decomp_worst.txt` and
`decomp_avg.txt`, and the same with one-row strips (`-p 1`: no
two-dimensional rectangles, linear combinations only) in
`decomp_worst_p1.txt` and `decomp_avg_p1.txt`.  For the paper it writes
the worst-case and average tables and a table of what the keys mean
(`tables/decomp_{worst,avg,keys}.tex`).  The one-row-strip runs
(`run_brobdicube.py worst-p1 model-p1`) also appear in the explorer.

## The other solvers

| Solver | Program | Average (`sim.csv`) | Worst case (`upper.csv`) |
|---|---|---|---|
| sortnet96 | `sortnet96/solve96 -6 -8 -w W` | measured | 108 + 96·strips(n) |
| rcube1d | `sortnet96/solve96 -w 1` | measured | ShellPuppy's published formula |
| rcube2d | `sortnet96/solve96 -w W` | measured | min(ShellPuppy's bound, 264 + 240·strips(n)) |
| permnet72 | `permnet72/brobdicube -w W` | measured | 79 + 72·strips(n) |
| permnet62..67 | `brob2/brob2 -a O I [-m]` | measured, greedy and optimal | `permnet_bounds.py` |
| permnet61W/A/B | `brobdicube -D brobdicube/nets/...` | measured, greedy and optimal | brobdicube `-w` |

* solve96's default mode (no `-6`/`-8`) is the RCube/ShellPuppy model: 240
  stages whose setup moves sum to 264, the constants of the RCube 2D bound.
  1D and 2D differ only in the width: 1D always works one row at a time.
* All runs process the whole cube per pass (`-p My`); above n = 4096 one
  strip of whole width groups is solved and scaled up (`-x`; a strip that is
  not a multiple of the width would bias the estimate high).
* W, the strip width, is chosen per size by the `tune` step of
  `run_others.py` (the width with the lowest paired mean), and stored in
  `data/<solver>/widths.csv`.  Runs at the widths not chosen are kept in
  `runs/` and `results.txt` but not pooled.
* strips(n) is the cost of one stage over the cube split into strips of the
  best two adjacent widths; a strip of width w over m columns costs
  2m + 4(2^w−1) + w·2^w.  The constant is each stage's setup overhead
  (algorithm length − 8, paid once per stage), summed over the stages: 108
  for sortnet96's 96 stages and 79 for permnet72's 72.

## Provenance and checking

Every run appends a row to `data/<solver>/runs/<file>.csv` and its complete
output (first line: the solver's command line) to the matching `.out` file.
`provenance.py SOLVER N` prints the runs behind a number.  `replay` (both
runners) reruns stored rows that have no saved output, checks they
reproduce exactly and saves the output.

`make check` (`scripts/check.py`) reruns brobdicube's planner at every
logged size in all four planning modes and compares with the stored logs,
then reruns the stored simulation runs with n ≤ 64 and compares mean and
standard deviation exactly.  Runs reproduce exactly only on the platform
that made them: the solvers scramble with `std::shuffle`, which differs
between libstdc++ (Linux) and libc++ (macOS), so the check uses
`mac-sim.csv` on macOS and `sim-arena*.csv` on Linux.  The planner's
expected costs are floating-point sums whose 15th digit can differ between
platforms; the check allows a relative 1e-9 there.
