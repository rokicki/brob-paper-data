#!/usr/bin/env python3
"""
Worst-case center move bounds for the comparison solvers, written to
data/<solver>/upper.csv (columns n,moves).  Ported from bounds/*.py (the
scripts behind olddata/bounds/*.txt), which they reproduce exactly except
for the permnet72 constant (see below).

  sortnet96  (bounds/sort96wc.py)  108 + 96 * strips(n)
  permnet72  (bounds/permwc.py)     79 + 72 * strips(n)
  rcube1d    (bounds/sp1dwc.py)    ShellPuppy's published worst case
  rcube2d    (bounds/sp2dwc.py)    min(published, 240-stage formula, ...)

strips(n) is the cost of one stage over the whole cube, split into strips
of widths w1 and w1+1 (the best mix): a strip of width w over m columns
costs 2m + 4(2^w - 1) + w 2^w (every one of the 2^w - 1 row patterns fires;
each block is the 8-move commutator plus 2 moves per extra row/column).
The constant is the setup overhead: setup turns cancel between consecutive
blocks of a stage, so each stage pays its algorithm's length - 8 once.
For solve96's 96 sortnet96 stages that sum is 108 exactly; for permnet72's
72 stages it is 79 (bounds/permwc.py reused 108, 29 moves loose).  The
240 stages of solve96's default (RCube) mode sum to 264, the constant in
sp2dwc.py.

Usage: bounds.py
"""
import csv
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sizes import MODEL_N, DISCRETE_N, TABLE_N   # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = os.path.join(ROOT, 'data')


def cst(m, w):
    return 2 * m + 4 * (2 ** w - 1) + (2 ** w) * w


def bestws3(mx, my):
    """Cheapest split of my rows into strips of widths w1 and w1+1."""
    best = my * cst(mx, 1)
    w1 = 1
    while my >= 2 ** (w1 + 1):
        w2 = w1 + 1
        n2 = my % w1
        if n2 * w2 <= my:
            n1 = (my - n2 * w2) // w1
            best = min(best, n1 * cst(mx, w1) + n2 * cst(mx, w2))
        n1 = (-my) % w2
        if n1 * w1 <= my:
            n2 = (my - n1 * w1) // w2
            best = min(best, n1 * cst(mx, w1) + n2 * cst(mx, w2))
        w1 += 1
    return best


def network(const, stages):
    return lambda n: const + stages * bestws3(n // 2 - 1, n - 1 - n // 2)


def shellpuppy(n):
    """ShellPuppy's published worst case for RCube (n = cube size)."""
    return (min(264, 60 * n - 200) + min(720 * n - 1440, 30 * n * n - 120 * n + 120)
            + 10 * n * n - 40 * n + 40)


def sp_bestw(n):
    def c(m, w):
        return m / w * (2 * m + 4 * (2 ** w - 1) + (2 ** w) * w)
    m = n // 2 - 1
    w, best = 1, c(m, 1)
    while True:
        t = c(m, w + 1)
        if t > best:
            return w
        w += 1
        best = t


def shellpuppy2d(n):
    w = sp_bestw(n)
    v1 = 264 + 120 * (n - 2) * ((2 ** w) * (4 + w) + n - 6) / w
    v2 = 264 + 10 * (n - 2) ** 2 + 120 * (n - 2) * ((2 ** 1) * (4 + 1) - 4) / 1
    return int(min(v1, v2, shellpuppy(n)) + 0.5)


BOUNDS = {
    'sortnet96': network(108, 96),
    'permnet72': network(79, 72),
    'rcube1d': shellpuppy,
    'rcube2d': shellpuppy2d,
}


def main():
    sizes = sorted(set(MODEL_N) | set(DISCRETE_N) | set(TABLE_N))
    for solver, f in BOUNDS.items():
        os.makedirs(os.path.join(DATA, solver), exist_ok=True)
        with open(os.path.join(DATA, solver, 'upper.csv'), 'w', newline='') as fh:
            w = csv.writer(fh)
            w.writerow(['n', 'moves'])
            for n in sizes:
                w.writerow([n, f(n)])
    print('bounds: %d sizes for %s' % (len(sizes), ', '.join(BOUNDS)))


if __name__ == '__main__':
    main()
