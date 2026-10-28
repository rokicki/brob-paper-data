#!/usr/bin/env python3
"""
Draw the main graph (Figure main-graph): center move counts normalized by
n^2 / lg n against lg n, with the number of center cubies 6(n-2)^2 scaled the
same way for reference.  Solid lines are worst-case upper bounds, dashed lines
simulated averages.

Writes figures/main_graph.{pdf,png}; --scale lgn2 writes main_graph_lgn2.*,
--scale all writes scaling_compare.* (the four normalizations, 2x2).
"""
import argparse
import math
import os
import sys

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt   # noqa: E402

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import (ROOT, SOLVERS, centers, graph_y, lower, solver_data,   # noqa: E402
                    upper0d)
from sizes import MODEL_N   # noqa: E402


YLABEL = {'lgn': r'moves $\cdot\,\lg n\,/\,(n-2)^2$',
          'lgn2': r'moves $\cdot\,\lg(n-2)\,/\,(n-2)^2$',
          'lgn2n2': r'moves $\cdot\,\lg(n-2)\,/\,n^2$',
          'lgnn2': r'moves $\cdot\,\lg n\,/\,n^2$'}
TITLE = {'lgn': r'$(n-2)^2/\lg n$ (submitted graph)',
         'lgn2': r'$(n-2)^2/\lg(n-2)$',
         'lgn2n2': r'$n^2/\lg(n-2)$ (submitted axis label)',
         'lgnn2': r'$n^2/\lg n$'}


def draw(ax, brob, scale, legend=True, ymax=300, show_centers=False):
    """Draw all curves on ax; returns the series that use legacy data."""
    ns = MODEL_N
    xs = [math.log2(n) for n in ns]
    ax.plot(xs, [graph_y(n, lower(n), scale) for n in ns], color='black', label='lower')
    ax.plot(xs, [graph_y(n, upper0d(n), scale) for n in ns], color='gray', label='0D_upper')
    if show_centers:
        # the number of center cubies, 6(n-2)^2, as if it were a move count
        ax.plot(xs, [graph_y(n, centers(n), scale) for n in ns], ':', color='saddlebrown',
                label='center count')
    legacy = []
    for key, label, color in SOLVERS:
        if brob == 'legacy' and key == 'brobdicube':
            label = 'permnet62'
        for kind, style in (('sim', '--'), ('upper', '-')):
            _, curve, src = solver_data(key, kind, brob)
            if not curve:
                continue
            if src == 'legacy':
                legacy.append('%s_%s' % (label, kind))
            pts = [(math.log2(n), graph_y(n, m, scale)) for n, m in curve
                   if n >= 4 and n % 2 == 0]
            ax.plot([p[0] for p in pts], [p[1] for p in pts], style, color=color,
                    label='%s_%s' % (label, kind))
    ax.set_xlim(0, 30)
    ax.set_ylim(0, ymax)
    ax.set_xlabel(r'$\lg(n)$')
    ax.set_ylabel(YLABEL[scale])
    ax.grid(True, color='#b0b0b0', linewidth=0.8)
    if legend == 'inside':
        ax.legend(loc='upper center', fontsize=6)
    elif legend:
        # lower right, pulled in a little so the curves' right-hand ends show,
        # but kept clear of where sortnet96_upper crosses rcube2d_sim
        ax.legend(loc='lower right', bbox_to_anchor=(0.91, 0.0), fontsize=6)
    return legacy


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--brob', choices=['new', 'legacy'], default='new',
                    help='legacy: draw the published permnet62 data instead of '
                         'the new brobdicube data (reproduces the published graph)')
    ap.add_argument('--out', default=os.path.join(ROOT, 'figures'))
    ap.add_argument('--scale', choices=['lgn', 'lgn2', 'lgn2n2', 'lgnn2', 'all'], default='lgnn2',
                    help='normalize by n^2/lg n (default), (n-2)^2/lg n (the '
                         'submitted graph), (n-2)^2/lg(n-2), n^2/lg(n-2), or all four')
    ap.add_argument('--ymax', type=float, default=None,
                    help='top of the y axis (default 210; 300 with --scale all)')
    ap.add_argument('--no-centers', dest='centers', action='store_false',
                    help='omit the line for the number of center cubies, 6(n-2)^2')
    a = ap.parse_args()
    if a.ymax is None:
        a.ymax = 300 if a.scale == 'all' else 210
    out = a.out
    os.makedirs(out, exist_ok=True)
    if a.scale == 'all':
        fig, axs = plt.subplots(2, 2, figsize=(12.0, 8.0))
        # rows: n^2 then (n-2)^2; columns: lg n then lg(n-2)
        for ax, sc in zip(axs.flat, ('lgnn2', 'lgn2n2', 'lgn', 'lgn2')):
            ax.set_title('normalized by ' + TITLE[sc], fontsize=9)
            draw(ax, a.brob, sc, legend=False, ymax=a.ymax, show_centers=a.centers)
        h, l = axs.flat[0].get_legend_handles_labels()
        fig.legend(h, l, loc='center right', fontsize=8, frameon=False)
        fig.tight_layout(rect=(0, 0, 0.86, 1))
        fig.savefig(os.path.join(out, 'scaling_compare.png'), dpi=200)
        fig.savefig(os.path.join(out, 'scaling_compare.pdf'))
        return
    name = 'main_graph' if a.scale == 'lgnn2' else 'main_graph_' + a.scale
    # the submitted graph had its legend inside; --brob legacy keeps that look
    inside = a.brob == 'legacy'
    fig, ax = plt.subplots(figsize=(6.0, 3.9))
    legacy = draw(ax, a.brob, a.scale, legend='inside' if inside else True,
                  ymax=a.ymax, show_centers=a.centers)
    fig.tight_layout()
    fig.savefig(os.path.join(out, name + '.pdf'))
    fig.savefig(os.path.join(out, name + '.png'), dpi=300)
    if legacy:
        print('note: using published (legacy) data for: ' + ', '.join(legacy))


if __name__ == '__main__':
    main()
