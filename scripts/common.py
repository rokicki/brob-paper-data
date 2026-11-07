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
    """Information-theoretic lower bound on center moves (the formula of
    olddata/bounds/lb.py): the smallest L with
        9(n-2) * (9n-3)^(L-1) >= (24!/(4!)^6)^o,   o = (n-2)//2 * ((n-2)-(n-2)//2).
    The right side counts the center states (o independent orbits).  In slice
    turn metric there are 9n moves (3 axes x n layers x 3 turns); turning the
    layer just turned again is never useful, so each move has 9n-3 useful
    successors.  For centers only, a face turn is never a useful last move, so
    counting backwards from the solved state the first choice is among the
    9(n-2) inner-slice moves."""
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
      'lgnn2'   moves * lg n / n^2           (the main graph)
      'lgn'     moves * lg n / (n-2)^2
      'lgn2'    moves * lg(n-2) / (n-2)^2
      'lgn2n2'  moves * lg(n-2) / n^2"""
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


# Solvers in display order: key, label, color.
SOLVERS = [
    ('rcube1d', 'rcube1d', 'blue'),
    ('rcube2d', 'rcube2d', 'orange'),
    ('sortnet96', 'sortnet96', 'green'),
    ('permnet72', 'permnet72', 'red'),
    ('brobdicube', 'brobdicube', 'purple'),
]


def solver_data(key, kind):
    """
    Data for one solver and kind ('upper' or 'sim') as (table, curve):
      table  {n: moves},
      curve  sorted [(n, moves)] for the graph.
    brobdicube's come from data/brobdicube/{worst,model,sim}.csv: its sim
    table is the measured value where there is one and the model elsewhere,
    its sim curve the measured sizes only (model values appear only in the
    tables, starred).  The other solvers' come from data/<solver>/<kind>.csv.
    """
    if key == 'brobdicube':
        b = os.path.join(DATA, 'brobdicube')
        if kind == 'upper':
            t = series(os.path.join(b, 'worst.csv'))
            return t, sorted(t.items())
        t = series(os.path.join(b, 'model.csv'))
        m = series(os.path.join(b, 'sim.csv'))
        t.update(m)
        return t, sorted(m.items())
    t = series(os.path.join(DATA, key, kind + '.csv'))
    return t, sorted(t.items())


def brob_sim_source(n):
    """'measured' or 'model' for a brobdicube sim value at n."""
    simf = os.path.join(DATA, 'brobdicube', 'sim.csv')
    if os.path.exists(simf) and n in series(simf):
        return 'measured'
    return 'model'
