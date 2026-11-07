# Differences from the published numbers, and statements to recheck

Generated with `make check` (`scripts/compare_tables.py`) against the
submitted `main.tex`.  Only the brobdicube column (permnet62 in the submitted paper, brobdicube
now) changes, apart from the lower-bound column (see below); the other solvers' columns still come from `olddata/` and match the
published tables.

## Biggest changes (new brobdicube vs. published permnet62, centers only)

| n | worst case old → new | change | average old → new | change |
|---:|---:|---:|---:|---:|
| 4 | 1,194 → 552 | −53.8% | 377 → 93.6 | −75.2% |
| 8 | 4,010 → 2,428 | −39.5% | 1,660 → 802 | −51.8% |
| 16 | 1.11·10^4 → 8,175 | −26.2% | 5,750 → 3,983 | −30.8% |
| 32 | 3.12·10^4 → 2.53·10^4 | −18.9% | 1.76·10^4 → 1.48·10^4 | −16.4% |
| 256 | 8.52·10^5 → 7.90·10^5 | −7.4% | 6.52·10^5 → 5.30·10^5 | −18.7% |
| 1,024 | 9.13·10^6 → 8.68·10^6 | −5.0% | 7.73·10^6 → 6.60·10^6 | −14.6% |
| 65,536 | 1.69·10^10 → 1.66·10^10 | −1.8% | 1.54·10^10 → 1.52·10^10 | −1.3% |
| 2^20 | 3.07·10^12 → 3.02·10^12 | −1.6% | 2.82·10^12 → 2.80·10^12 | −0.7% |
| 2^30 | 1.82·10^18 → 1.79·10^18 | −1.6% | 1.69·10^18 → 1.68·10^18 | −1.0% |

The full list is `tables/compare_permnet62.csv`.  Worst case is lower by
1.5–1.8% for every n ≥ 65,536; averages by 0.5–1% for n ≥ 2^20.
Above 2^24 the averages are measured on arena (one strip sample each) at
2^25, 2^26, 2^27, 2^28 and 2^30; 2^29, 671,088,640 and 805,306,368 are
still the model (marked `*`) until those runs finish.  Measured and model
agree to 3·10^-6 or better there (2^30: measured 1.6755586592·10^18,
model 1.6755584700·10^18, +1.1·10^-7).

Discrete upper-bound table (even n ≤ 100): centers drop by 33% at n=4
(820 → 552) and by 3–3.5% near n=100 (n=100: 168,584 → 162,704; total
175,877 → 169,997).

## Statements in `main.tex` that depend on these numbers

* **Title, abstract, Sec. 1 (lines 145, 194–197, 264, 282), Lemma permnet62
  (1784–1818):** leading constant 31, ratio < 2.41.  The plan brobdicube
  chooses for very large n (checked at n = 2^40, 2^50, 2^60) uses 61 stages
  (`001111-0024`, `0011xx-0016`, three times `01xxxx-0007`) with total
  vertical cost 122, i.e. leading constant 122/4 = **30.5** by the paper's
  stage lemma (permnet62: 62 stages × 2 / 4 = 31), ratio 30.5/12.88 ≈ 2.37.
  Needs your confirmation that the `v` field is the per-stage quantity the
  lemma counts.
* **Experimental results (lines ~1993–1998):** "permnet72 attains the first
  crossover at n=26, RCube2D is tightest for 52–408, permnet62 for all
  larger n."  With the new data brobdicube has the smallest worst-case
  bound for **every even n ≥ 22** (0D is smallest for n ≤ 20); see
  `scripts/crossovers.py`.
* **"they approach their horizontal asymptotes of y=48, y=36 and y=31"**
  (line ~1991): the brobdicube asymptote would be 30.5 (see above); at
  n = 2^30 it is still at 46.6 on the published scale.
* **"All of our calculated upper bounds are above our experimental data"**:
  still true; the brobdicube bound is ≥ 1.067× its measured average at every
  size.
* **Experimental methodology (line ~1981):** the paper says permnet72 and
  permnet62 averages came from the exact formula.  Here averages are
  measured up to n = 2^24 (2^30 when the big runs finish), whole cubes up to
  n = 4096, and the model is used only beyond; the model is 0.5–1.7% high
  for n ≤ 26.
* **Discrete upper bound for n ≤ 100 (Sec. discreteub, Appendix
  app:discrete):** new table; its "Optimal multiset" column now shows the
  strip-width multiset used by the most stages (brobdicube picks widths per
  stage; all are in `tables/discrete_ub.csv`).
* **Text about the graph (lines ~1987–1991)** describes the curves at small n
  ("the upper bound for the 2D solvers starts at extremely high move counts
  per cubie", RCube upper bounds "peak at about n=21"); with the n²/lg n
  scale the 2D bounds start low and peak around n ≈ 16–32.  Reread with the
  new figure.
* **Name (applied in the paper copy):** every "permnet62" is now
  "brobdicube" (was "permnet61" until 2026-11-06) (graph, tables, Section beyond3cycles, the lemma, Sec. 6/7
  text, the master-sequence appendix and its label keys).  Only the name
  changed: the lemma still says "L = 26 + 2·18 = 62 stages" and constant
  31, and the appendix tables still list the 26 + 18 stage networks, so
  that section has to be rewritten for the 61-stage plan (see the first
  item).
* **Lower bound (applied in the paper copy):** now 9n−3 throughout, i.e.
  `lb.txt`.  The text derived f(n) with branching factor 9n; the submitted
  tables used 9n−22 (n=4: 14, now 11; n=8: 82, now 76; n=32: 1,440, now
  1,422; identical to 3 digits from n ≈ 1024).  `update_paper.py` rewrites
  the branching-factor sentence and the formulas to 9n−3; the constant 12.88
  is unchanged.  The data use `bounds/lb.py` exactly, which counts the last
  move among the 9(n−2) inner slices (a face turn is never a useful last
  move for centers): 9(n−2)·(9n−3)^(L−1) ≥ states.  That is one more than
  (9n−3)^L ≥ states at n=10 and n=28 only.  The paper text still states the
  simpler (9n−3)^L form; consider adding the 9(n−2) refinement there.
* **Figure main-graph (applied in the paper copy):** now moves·lg n/n²,
  which matches the text's "normalized by n²/lg n"; the caption now says
  centers only and explains the center-count line.
* **Table 6:** the published table leaves permnet62 blank ("--") at
  n ≥ 671,088,640 even though data existed; the new table fills these in.

## The 61-stage networks and adaptive selection

brobdicube does not run one fixed network.  For every n it plans each
phase: it picks an algorithm (a stage network) from the library
(`brobdicube/algs3/`, 268 networks), greedy or optimal planning, and the
strip widths of each stage.  It minimizes the model's expected cost, or
with `-w` the worst-case cost.  So the average curve and the upper-bound
curve can use different networks at the same n.  At large n three distinct
61-stage networks appear.  (Keys: one character per face in the order
U L F R B D; faces with the same digit form one color group in that phase;
`x` marks faces the phase does not touch.)

**W61: worst-case planner, n ≥ 311,744.**  A three-level binary split:

| Phase | Key | Stages | Separates |
|---|---|---:|---|
| 1 | `001111` | 24 | {U,L} from {F,R,B,D}, i.e. two adjacent faces from the other four |
| 2 | `01xxxx` | 7 | U from L |
| 3 | `xx0011` | 16 | {F,R} from {B,D} |
| 4 | `xx10xx` | 7 | F from R |
| 5 | `xxxx10` | 7 | B from D |

That is 2|4, then 1|1 and 2|2, then 1|1 and 1|1, for 61 stages.  This is the
network the permnet lemma should describe.

**A61a: average planner, n ≈ 9.6·10^6 to 1.4·10^8.**

| Phase | Key | Stages | Separates |
|---|---|---:|---|
| 1 | `011110` | 24 | the opposite pair {U,D} from the ring {L,F,R,B} |
| 2 | `1xxxx0` | 7 | U from D |
| 3 | `x0123x` | 30 | the four ring faces, all at once |

**A61b: average planner, n ≳ 9·10^7, and every table size from 2^27 up.**

| Phase | Key | Stages | Separates |
|---|---|---:|---|
| 1 | `000102` | 31 | two single faces (R and D) from the group {U,L,F,B} |
| 2 | `0123xx` | 30 | the four remaining faces, all at once (applied rotated, as `023x1x`) |

**Same leading constant.**  The per-stage vertical cost `v`, which
determines the leading term, sums to 122 for all three:

| Network | v (sum) | h | o | c |
|---|---|---:|---:|---:|
| W61 | 48+14+32+14+14 = 122 | 186 | 335 | 140 |
| A61a | 48+14+60 = 122 | 196 | 366 | 186 |
| A61b | 62+60 = 122 | 246 | 420 | 312 |

The leading constant is therefore 122/4 = 30.5 for each of them, against 31
for permnet62's 62 stages.  They differ in the lower-order costs: h are
depth-2 inner moves, o outer face turns, c setup/conjugation moves (cost
fields from the `.alg` file names, summed over the phases).

**Why the two planners choose differently.**
- **Worst-case planner:** every stage is paid in full, so it wants the
  lowest overheads.  W61 has the lowest h, o and c of the three.
- **Average planner:** a stage costs only when a random cube needs it.
  A61a and A61b have *higher* overheads, but their stage-activation
  probabilities (greedy planning concentrates the work in fewer stages)
  make them cheaper on average at the largest n.  So the average plan's
  advantage comes from the stage probabilities, not the cost fields.

**Switch points** (on the planning grid, 16 sizes per octave, so each point
is only resolved to that grid):
- **Worst case:** 67 stages (n ≤ 14), 65 (16 to about 300), 63 (to about
  10^4), 62 (two alternating 62-stage plans to 273,750), then W61 from
  311,744 on.
- **Average:** very long optimal-planned networks at small and mid n (608
  stages for n ≤ 18, 85 to 1061 stages for 30 ≤ n ≤ 4·10^4).  Then 63 to
  65-stage greedy plans (`010101` + `0x1x2x` + `x1x2x0`) up to about 10^7.
  Then A61a, alternating with 63-stage plans to 1.35·10^7.  Then A61a and
  A61b alternating up to 1.67·10^8, and A61b above that.

Per-size plans are in `data/brobdicube/{worst,model}.csv`, in the `algs`
and `widths` columns.  The paper needs:
- a section on adaptive selection;
- a description of all three 61-stage networks and why they differ;
- in the upper-bound tables, the network brobdicube selects at each size
  (see TODO.md).
