# How brobdicube chooses its networks

brobdicube does not run one fixed network.  For every n it plans each
reduction step: it picks a network from the library (`brobdicube/algs3/`,
268 networks), greedy or optimal planning, and the strip widths of each
stage.  It minimizes the model's expected cost, or with `-w` the worst-case
cost, so the average curve and the upper-bound curve can use different
networks at the same n.  The chains it picks at each logged size are listed
in `tables/decomp_worst.txt` and `tables/decomp_avg.txt` (written by
`scripts/decompositions.py` from the planner logs); the per-size plans are
also in `data/brobdicube/{worst,model}.csv`, columns `algs` and `widths`.

Keys name the steps: one character per face in the order U L F R B D;
faces with the same digit form one color group after the step; `x` marks
faces the step does not touch.

## The 61-stage networks

At large n three distinct 61-stage chains appear.  Each is also run on its
own, as permnet61W, permnet61A and permnet61B (`brobdicube -D
brobdicube/nets/{W61,A61a,A61b}`).

**W61: chosen by the worst-case planner from n = 311,744 on.**  A
three-level binary split:

| Step | Key | Stages | Separates |
|---|---|---:|---|
| 1 | `001111` | 24 | {U,L} from {F,R,B,D}, i.e. two adjacent faces from the other four |
| 2 | `01xxxx` | 7 | U from L |
| 3 | `xx0011` | 16 | {F,R} from {B,D} |
| 4 | `xx10xx` | 7 | F from R |
| 5 | `xxxx10` | 7 | B from D |

That is 2|4, then 1|1 and 2|2, then 1|1 and 1|1, for 61 stages.

**A61a: chosen by the average planner for n from about 9.6·10^6 to
1.6·10^8.**

| Step | Key | Stages | Separates |
|---|---|---:|---|
| 1 | `011110` | 24 | the opposite pair {U,D} from the ring {L,F,R,B} |
| 2 | `1xxxx0` | 7 | U from D |
| 3 | `x0123x` | 30 | the four ring faces, all at once |

**A61b: chosen by the average planner from about 9·10^7 on.**

| Step | Key | Stages | Separates |
|---|---|---:|---|
| 1 | `000102` | 31 | two single faces (R and D) from the group {U,L,F,B} |
| 2 | `0123xx` | 30 | the four remaining faces, all at once (applied rotated, as `023x1x`) |

Between about 9·10^7 and 1.6·10^8 the average planner alternates between
A61a and A61b.

**Same leading constant.**  The per-stage vertical cost `v`, which
determines the leading term, sums to 122 for all three:

| Network | v (sum) | h | o | c |
|---|---|---:|---:|---:|
| W61 | 48+14+32+14+14 = 122 | 186 | 335 | 140 |
| A61a | 48+14+60 = 122 | 196 | 366 | 186 |
| A61b | 62+60 = 122 | 246 | 420 | 312 |

The leading constant is therefore 122/4 = 30.5 for each of them.  They
differ in the lower-order costs: h are depth-2 inner moves, o outer face
turns, c setup/conjugation moves (cost fields from the `.alg` file names,
summed over the steps).

## Why the two planners choose differently

- **Worst-case planner:** every stage is paid in full, so it wants the
  lowest overheads.  W61 has the lowest h, o and c of the three.
- **Average planner:** a stage costs only when a random cube needs it.
  A61a and A61b have *higher* overheads, but their stage-activation
  probabilities (greedy planning concentrates the work in fewer stages)
  make them cheaper on average at the largest n.  So the average plan's
  advantage comes from the stage probabilities, not the cost fields.
- At smaller n the average planner uses long optimally planned networks
  (85 to 1,070 stages in all).  It plans the first step greedily from
  n = 13,194 on and every step greedily from n = 46,340 on.  The
  worst-case planner plans every step greedily at every size.
