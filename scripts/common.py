"""Shared formulas and data loading for the graph and table scripts."""
import csv
import math
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = os.path.join(ROOT, 'data')

# lg(24! / (4!)^6): states of one 24-cubie center orbit.
LG_ORBIT = math.log2(math.factorial(24) / math.factorial(4) ** 6)


def centers(n):
    """Number of center cubies counted per cube (six faces of (n-2)^2)."""
    return 6 * (n - 2) ** 2


def lower(n):
    """Information-theoretic lower bound on center moves, exactly as
    bounds/lb.py (olddata/bounds/lb.txt): the smallest L with
        9(n-2) * (9n-3)^(L-1) >= (24!/(4!)^6)^o,   o = (n-2)//2 * ((n-2)-(n-2)//2).
    The right side counts the center states (o independent orbits).  In slice
    turn metric there are 9n moves (3 axes x n layers x 3 turns); turning the
    layer just turned again is never useful, so each move has 9n-3 useful
    successors.  For centers only, a face turn is never a useful last move, so
    counting backwards from the solved state the first choice is among the
    9(n-2) inner-slice moves.  Reproduces every value in lb.txt."""
    m = n - 2
    o = (m // 2) * (m - m // 2)
    b = 9 * n - 3
    # same float expression as lb.py, so huge n agree to the last digit
    orb = math.factorial(24) / math.factorial(4) ** 6
    L = math.ceil(1 + (o * math.log(orb) - math.log(9 * m)) / math.log(b))
    if o * LG_ORBIT < 1e6:            # exact integer check near boundaries
        S = (math.factorial(24) // math.factorial(4) ** 6) ** o
        while L > 1 and 9 * m * b ** (L - 2) >= S:
            L -= 1
        while 9 * m * b ** (L - 1) < S:
            L += 1
    return L


def lower_published_tables(n):
    """The lower-bound column of the submitted paper's tables, which used
    branching factor 9n-22 and no rounding up (reverse-engineered; it
    reproduces every published value).  Only used to reproduce those tables."""
    return (n / 2 - 1) ** 2 * LG_ORBIT / math.log2(9 * n - 22)


def upper0d(n):
    """No parallelization: 6 moves per center cubie."""
    return 6 * centers(n)


def wings_ub(n):
    """Worst-case wing moves, Equation (wings-ub)."""
    r = n // 2 - 1
    return r + min(156 * r, 144 * r + 192, 120 * r + 1344, 108 * r + 2368,
                   96 * r + 4304, 24 * r + 28592)


def graph_y(n, moves, scale='lgn'):
    """Graph normalization:
      'lgn'     moves * lg n / (n-2)^2       (what the published graph plots)
      'lgn2'    moves * lg(n-2) / (n-2)^2
      'lgn2n2'  moves * lg(n-2) / n^2        (what its axis label intended)
      'lgnn2'   moves * lg n / n^2"""
    if scale == 'lgnn2':
        return moves * math.log2(n) / n ** 2
    if scale == 'lgn':
        return moves * math.log2(n) / (n - 2) ** 2
    if scale == 'lgn2':
        return moves * math.log2(n - 2) / (n - 2) ** 2
    return moves * math.log2(n - 2) / n ** 2


def read_csv(path):
    with open(path, newline='') as f:
        return list(csv.DictReader(f))


def series(path, col='moves'):
    """{n: value} from a CSV with an n column."""
    return {int(float(r['n'])): float(r[col]) for r in read_csv(path)}


# Solvers in display order: key, label, color.  The "brobdicube" entry is the
# solver in this repository (labelled brobdicube in the paper; user,
# 2026-11-06) and replaces the old permnet62 data.
SOLVERS = [
    ('rcube1d', 'rcube1d', 'blue'),
    ('rcube2d', 'rcube2d', 'orange'),
    ('sortnet96', 'sortnet96', 'green'),
    ('permnet72', 'permnet72', 'red'),
    ('brobdicube', 'brobdicube', 'purple'),
]


def solver_data(key, kind, brob='new'):
    """
    Data for one solver and kind ('upper' or 'sim') as (table, curve, source):
      table  {n: moves},
      curve  sorted [(n, moves)] for the graph,
      source 'new' or 'legacy'.
    Fresh results for the other solvers go in data/<solver>/<kind>.csv
    (columns n,moves); until then the previously collected data in
    data/legacy/ (imported from olddata/) is used.  With brob='legacy' the
    brobdicube slot shows the published permnet62 data instead, which
    reproduces the published graph and tables.
    """
    if key == 'brobdicube' and brob == 'new':
        b = os.path.join(DATA, 'brobdicube')
        if kind == 'upper':
            t = series(os.path.join(b, 'worst.csv'))
        else:
            t = series(os.path.join(b, 'model.csv'))
            simf = os.path.join(b, 'sim.csv')
            m = series(simf) if os.path.exists(simf) else {}
            t.update(m)                     # measured wherever we have it
            # the graph shows measured sizes only; model values appear
            # only in the tables, starred
            return t, sorted(m.items()), 'new'
        return t, sorted(t.items()), 'new'
    if key == 'brobdicube':
        key = 'permnet62'
    fresh = os.path.join(DATA, key, kind + '.csv')
    if os.path.exists(fresh):
        t = series(fresh)
        return t, sorted(t.items()), 'new'
    leg = os.path.join(DATA, 'legacy', '%s_%s.csv' % (key, kind))
    if not os.path.exists(leg):
        return {}, [], None
    t = series(leg)
    return t, sorted(t.items()), 'legacy'


def brob_sim_source(n):
    """'measured' or 'model' for a brobdicube sim value at n."""
    simf = os.path.join(DATA, 'brobdicube', 'sim.csv')
    if os.path.exists(simf) and n in series(simf):
        return 'measured'
    return 'model'
