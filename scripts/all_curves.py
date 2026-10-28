#!/usr/bin/env python3
"""
Draft figure: every case in results.txt on one main-graph-style plot (same
normalization as make_graph.py's default, moves * lg n / n^2, against lg n),
with thin lines and no legend.  Solid = worst-case bound (column worst),
dashed = simulated average (column moves), both from the w = 0 rows; black =
the information-theoretic lower bound, gray = 0D upper bound.

Writes figures/all_curves.{pdf,png}.

Usage: all_curves.py [--lw 0.4] [--ymax 210]
"""
import argparse
import math
import os
import sys
from collections import defaultdict

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt   # noqa: E402

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import ROOT, graph_y, lower, upper0d   # noqa: E402
from sizes import MODEL_N   # noqa: E402


def read_results():
    """case -> {'sim': [(n, moves)], 'worst': [(n, worst)]} from results.txt"""
    r = defaultdict(lambda: {'sim': [], 'worst': []})
    for line in open(os.path.join(ROOT, 'results.txt')):
        f = line.split()
        if not f or f[0].startswith('#') or f[0] == 'case' or f[2] != '0':
            continue
        n = int(f[1])
        if n < 4 or n % 2:
            continue
        if f[4] != '-':
            r[f[0]]['sim'].append((n, float(f[4])))
        if len(f) > 7 and f[7] != '-':
            r[f[0]]['worst'].append((n, float(f[7])))
    return r


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--lw', type=float, default=0.4, help='line width (points)')
    ap.add_argument('--ymax', type=float, default=210)
    ap.add_argument('--out', default=os.path.join(ROOT, 'figures'))
    a = ap.parse_args()
    fig, ax = plt.subplots(figsize=(6.0, 3.9))
    xs = [math.log2(n) for n in MODEL_N]
    ax.plot(xs, [graph_y(n, lower(n), 'lgnn2') for n in MODEL_N], color='black', lw=a.lw)
    ax.plot(xs, [graph_y(n, upper0d(n), 'lgnn2') for n in MODEL_N], color='gray', lw=a.lw)
    for case, d in sorted(read_results().items()):
        color = None
        for kind, style in (('worst', '-'), ('sim', '--')):
            pts = sorted(d[kind])
            if not pts:
                continue
            line, = ax.plot([math.log2(n) for n, _ in pts],
                            [graph_y(n, m, 'lgnn2') for n, m in pts],
                            style, lw=a.lw, color=color)
            color = line.get_color()
    ax.set_xlim(0, 30)
    ax.set_ylim(0, a.ymax)
    ax.set_xlabel(r'$\lg(n)$')
    ax.set_ylabel(r'moves $\cdot\,\lg n\,/\,n^2$')
    ax.grid(True, color='#d0d0d0', linewidth=0.3)
    fig.tight_layout()
    os.makedirs(a.out, exist_ok=True)
    fig.savefig(os.path.join(a.out, 'all_curves.pdf'))
    fig.savefig(os.path.join(a.out, 'all_curves.png'), dpi=300)


if __name__ == '__main__':
    main()
