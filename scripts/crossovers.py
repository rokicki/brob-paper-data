#!/usr/bin/env python3
"""
For each even n up to 4096 (and the table sizes above), which solver has the
smallest worst-case upper bound?  Prints the ranges.

A solver without a value at some n is interpolated there in the graph
coordinates (lg n, normalized moves) from its curve.
"""
import bisect
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import SOLVERS, graph_y, solver_data, upper0d   # noqa: E402
from sizes import TABLE_N   # noqa: E402


def interpolator(curve):
    pts = sorted((math.log2(n), graph_y(n, m)) for n, m in curve if n >= 4)
    xs = [p[0] for p in pts]

    def f(n):
        x = math.log2(n)
        if x < xs[0] or x > xs[-1]:
            return None
        i = bisect.bisect_left(xs, x)
        if xs[i] == x:
            y = pts[i][1]
        else:
            (x0, y0), (x1, y1) = pts[i - 1], pts[i]
            y = y0 + (y1 - y0) * (x - x0) / (x1 - x0)
        return y * (n - 2) ** 2 / math.log2(n)
    return f


def main():
    fns = {'0D': upper0d}
    for key, label, _ in SOLVERS:
        table, curve = solver_data(key, 'upper')
        interp = interpolator(curve)
        fns[label] = (lambda t, g: (lambda n: t.get(n, g(n))))(table, interp)
    sizes = list(range(4, 4097, 2)) + [n for n in TABLE_N if n > 4096]
    best = []
    for n in sizes:
        vals = {k: f(n) for k, f in fns.items()}
        vals = {k: v for k, v in vals.items() if v is not None}
        best.append((n, min(vals, key=vals.get)))
    print('smallest worst-case center bound')
    start = 0
    for i in range(1, len(best) + 1):
        if i == len(best) or best[i][1] != best[start][1]:
            print('  n = %d .. %d: %s' % (best[start][0], best[i - 1][0], best[start][1]))
            start = i


if __name__ == '__main__':
    main()
