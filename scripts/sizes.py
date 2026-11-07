"""Cube sizes used for the graph and the tables (all even)."""


def even(x):
    return max(4, 2 * int(round(x / 2)))


def _grid(lo_exp, hi_exp, steps_per_octave):
    return {even(2 ** (k / steps_per_octave))
            for k in range(lo_exp * steps_per_octave, hi_exp * steps_per_octave + 1)}


POWERS = [2 ** k for k in range(2, 31)]

# Sizes in the appendix tables: powers of two, plus 671,088,640 and
# 805,306,368 (5 * 2^27 and 3 * 2^28).
TABLE_N = sorted(set(POWERS) | {671088640, 805306368})

# Even n <= 100 for the discrete worst-case upper-bound table.
DISCRETE_N = list(range(4, 101, 2))

# Sizes for the planning models (cheap): every even n up to 256, then 16
# points per octave up to 2^30.
MODEL_N = sorted(set(range(4, 257, 2)) | _grid(8, 30, 16) | set(TABLE_N))

# Sizes that are actually solved: every even n up to 128, 8 points per octave
# up to 2^20, 4 points per octave up to 2^24.
SIM_MAX = 2 ** 24
SIM_N = sorted(set(range(4, 129, 2)) | _grid(7, 20, 8) | _grid(20, 24, 4)
               | {n for n in POWERS if n <= SIM_MAX})
