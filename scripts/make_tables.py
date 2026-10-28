#!/usr/bin/env python3
"""
Write the appendix tables as LaTeX fragments (tables/*.tex, the tabular
bodies with the paper's formatting) and as CSV (tables/*.csv):

  maketable5    upper bounds (centers only)          Table tab:maketable5
  maketable5b   simulated averages (centers only)    Table tab:maketable5b
  maketable6    both, per center cubie, to n = 2^30  Table tab:maketable6
  discrete_ub   whole-cube worst case, even n <= 100 Table tab:discrete-ub
  compare_permnet62   new brobdicube vs the published permnet62 numbers

The lower-bound column is common.lower (lb.txt); with --brob legacy it is
common.lower_published_tables, which reproduces the submitted tables.
Cells for brobdicube simulations beyond the largest measured size come
from the expected-cost model and are marked with an asterisk.
"""
import argparse
import csv
import math
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import (ROOT, SOLVERS, brob_sim_source, centers, lower, lower_published_tables,  # noqa: E402
                    read_csv, series, solver_data, upper0d, wings_ub, DATA)
from sizes import DISCRETE_N, TABLE_N   # noqa: E402

OUT = os.path.join(ROOT, 'tables')
GREEN_BEST = '\\cellcolor{green!25}'


def sig3(v):
    """Three significant digits, keeping trailing zeros (6.00, 0.931, 36.5)."""
    if v == 0:
        return '0'
    d = max(0, 2 - int(math.floor(math.log10(abs(v)))))
    if 10 <= v < 11:
        d = 2        # the published tables print 10.05, 10.04
    return '%.*f' % (d, round(v, d))


def moves_tex(v):
    if v < 1000:
        return '%d' % round(v)
    e = int(math.floor(math.log10(v)))
    m = v / 10 ** e
    if round(m, 2) >= 10:
        m, e = m / 10, e + 1
    return '$%.2f\\cdot10^{%d}$' % (m, e)


def comma(n):
    return '{:,}'.format(n)


def write(name, text):
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(OUT, name), 'w') as f:
        f.write(text)


def write_csv(name, header, rows):
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(OUT, name), 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(header)
        w.writerows(rows)


def highlight(cells, values, color):
    """Color the cells whose displayed value is the minimum."""
    shown = [None if v is None else float(re.sub(r'[^0-9.e+-]', '', s.replace('\\cdot10^{', 'e').replace('}', '')))
             for s, v in zip(cells, values)]
    best = min(x for x in shown if x is not None)
    return [(color + c) if x == best else c for c, x in zip(cells, shown)]


def main():
    global OUT
    ap = argparse.ArgumentParser()
    ap.add_argument('--brob', choices=['new', 'legacy'], default='new',
                    help='legacy: put the published permnet62 data in the '
                         'brobdicube column (reproduces the published tables)')
    ap.add_argument('--out', default=OUT)
    a = ap.parse_args()
    OUT = a.out
    upper = {k: solver_data(k, 'upper', a.brob) for k, _, _ in SOLVERS}
    sim = {k: solver_data(k, 'sim', a.brob) for k, _, _ in SOLVERS}
    labels = [l for _, l, _ in SOLVERS]
    if a.brob == 'legacy':
        labels = ['permnet62' if k == 'brobdicube' else l for (k, _, _), l in zip(SOLVERS, labels)]
    lower = globals()['lower'] if a.brob == 'new' else lower_published_tables
    keys = [k for k, _, _ in SOLVERS]

    def get(d, k, n):
        return d[k][0].get(n)

    def star(k, n):
        return a.brob == 'new' and k == 'brobdicube' and brob_sim_source(n) == 'model'

    # ------------------------------------------------------------ table 5
    rows5, csv5 = [], []
    for n in [n for n in TABLE_N if n <= 2 ** 20]:
        vals = [upper0d(n)] + [get(upper, k, n) for k in keys]
        cells = [moves_tex(v) if v is not None else '--' for v in vals]
        cells = highlight(cells, vals, GREEN_BEST)
        rows5.append('    %s & %s & %s \\\\' % (comma(n), moves_tex(lower(n)), ' & '.join(cells)))
        csv5.append([n, '%.6g' % lower(n)] + ['' if v is None else '%.6g' % v for v in vals])
    write('maketable5.tex', '\n'.join(rows5) + '\n')
    write_csv('maketable5.csv', ['n', 'lower', '0D_upper'] + [l + '_upper' for l in labels], csv5)

    # ----------------------------------------------------------- table 5b
    rows5b, csv5b = [], []
    for n in [n for n in TABLE_N if n <= 2 ** 20]:
        vals = [get(sim, k, n) for k in keys]
        cells = [moves_tex(v) + ('$^{*}$' if star(k, n) else '') if v is not None else '--'
                 for k, v in zip(keys, vals)]
        cells = highlight(cells, vals, GREEN_BEST)
        rows5b.append('    %s & %s \\\\' % (comma(n), ' & '.join(cells)))
        csv5b.append([n] + ['' if v is None else '%.10g' % v for v in vals])
    write('maketable5b.tex', '\n'.join(rows5b) + '\n')
    write_csv('maketable5b.csv', ['n'] + [l + '_sim' for l in labels], csv5b)

    # ------------------------------------------------------------ table 6
    rows6, csv6 = [], []
    for n in TABLE_N:
        c = centers(n)
        uv = [upper0d(n) / c] + [None if get(upper, k, n) is None else get(upper, k, n) / c for k in keys]
        sv = [None if get(sim, k, n) is None else get(sim, k, n) / c for k in keys]
        ucells = highlight([sig3(v) if v is not None else '--' for v in uv], uv, '\\cellcolor{green!40}')
        scells = [sig3(v) + ('$^{*}$' if star(k, n) else '') if v is not None else '--'
                  for k, v in zip(keys, sv)]
        scells = highlight(scells, sv, '\\cellcolor{green!20}')
        rows6.append('    %-13s & %s\n              & %s & %s \\\\' % (
            comma(n), sig3(round(lower(n)) / c), ' & '.join(ucells), ' & '.join(scells)))
        csv6.append([n, '%.6g' % (round(lower(n)) / c)] + ['' if v is None else '%.6g' % v for v in uv + sv])
    write('maketable6.tex', '\n'.join(rows6) + '\n')
    write_csv('maketable6.csv', ['n', 'lower', '0D_upper'] + [l + '_upper' for l in labels]
              + [l + '_sim' for l in labels], csv6)

    if a.brob == 'legacy':
        return
    # --------------------------------------------------------- discrete ub
    worst = {int(r['n']): r for r in read_csv(os.path.join(DATA, 'brobdicube', 'worst.csv'))}
    rowsd, csvd = [], []
    for n in DISCRETE_N:
        r = worst[n]
        cn = round(float(r['moves']))
        wu = wings_ub(n)
        tot = cn + wu + 20
        # most common strip-width multiset over all stages of the plan
        use = {}
        for step in r['widths'].split(';'):
            for ms, cnt in re.findall(r'\{([^}]*)\}x(\d+)', step):
                use[ms] = use.get(ms, 0) + int(cnt)
        ms = max(use, key=lambda m: use[m])
        mtex = ',\\, '.join('%s^{%s}' % tuple(p.split('^')) for p in ms.split(','))
        rowsd.append('    %d & %s & %s & 20 & %s & $\\{%s\\}$ \\\\' % (
            n, comma(cn).replace(',', '{,}'), comma(wu).replace(',', '{,}'),
            comma(tot).replace(',', '{,}'), mtex))
        csvd.append([n, cn, wu, 20, tot, ms, r['widths']])
    write('discrete_ub.tex', '\n'.join(rowsd) + '\n')
    write_csv('discrete_ub.csv', ['n', 'centers', 'wings', 'edges_corners', 'total',
                                  'most_common_widths', 'all_stage_widths'], csvd)

    # ---------------------------------------------- comparison with old data
    old_u = series(os.path.join(DATA, 'legacy', 'permnet62_upper.csv'))
    old_s = series(os.path.join(DATA, 'legacy', 'permnet62_sim.csv'))
    rows = []
    for n in TABLE_N:
        nu, ns = get(upper, 'brobdicube', n), get(sim, 'brobdicube', n)
        ou, os_ = old_u.get(n), old_s.get(n)
        rows.append([n, '%.6g' % nu, '' if ou is None else '%.3g' % ou,
                     '' if ou is None else '%+.2f%%' % (100 * (nu / ou - 1)),
                     '%.10g' % ns, '' if os_ is None else '%.3g' % os_,
                     '' if os_ is None else '%+.2f%%' % (100 * (ns / os_ - 1)),
                     brob_sim_source(n)])
    write_csv('compare_permnet62.csv', ['n', 'brobdicube_upper', 'old_permnet62_upper', 'change_upper',
                                        'brobdicube_sim', 'old_permnet62_sim', 'change_sim',
                                        'brobdicube_sim_source'], rows)


if __name__ == '__main__':
    main()
