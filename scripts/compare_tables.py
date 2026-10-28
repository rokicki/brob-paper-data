#!/usr/bin/env python3
"""
Compare generated table rows with the tables in a paper's main.tex, cell by
cell, and list the largest relative differences.

  compare_tables.py PAPER_MAIN_TEX TABLES_DIR [--top K]

Cells are compared as numbers (as displayed, so three significant digits);
differences in highlighting (which solver is marked best) are listed too.
"""
import argparse
import math
import os
import re

TABLES = [('tab:maketable5', 'maketable5', ['lower', '0D_upper', 'rcube1d_upper', 'rcube2d_upper',
                                            'sortnet96_upper', 'permnet72_upper', 'LAST_upper']),
          ('tab:maketable5b', 'maketable5b', ['rcube1d_sim', 'rcube2d_sim', 'sortnet96_sim',
                                              'permnet72_sim', 'LAST_sim']),
          ('tab:maketable6', 'maketable6', ['lower', '0D_upper', 'rcube1d_upper', 'rcube2d_upper',
                                            'sortnet96_upper', 'permnet72_upper', 'LAST_upper',
                                            'rcube1d_sim', 'rcube2d_sim', 'sortnet96_sim',
                                            'permnet72_sim', 'LAST_sim'])]


def cell_value(c):
    c = re.sub(r'\\cellcolor\{[^}]*\}', '', c)
    c = c.replace('$^{*}$', '').replace('$', '').strip()
    if c in ('--', ''):
        return None
    m = re.fullmatch(r'([0-9.]+)\\cdot\s*10\^\{?(-?[0-9]+)\}?', c)
    if m:
        return float(m.group(1)) * 10 ** int(m.group(2))
    return float(c.replace(',', '').replace('{', '').replace('}', ''))


def parse_rows(text):
    rows = {}
    for line in text.split('\\\\'):
        line = ' '.join(line.split())
        if '&' not in line:
            continue
        cells = [c.strip() for c in line.split('&')]
        try:
            n = int(cells[0].replace(',', '').replace('{', '').replace('}', ''))
        except ValueError:
            continue
        rows[n] = [(cell_value(c), 'cellcolor' in c) for c in cells[1:]]
    return rows


def paper_rows(tex, label):
    lab = tex.index('\\label{%s}' % label)
    beg = tex.rindex('\\begin{tabular}', 0, lab)
    body = tex[beg:tex.index('\\end{tabular}', beg)]
    first = body.index('\\hline')
    second = body.index('\\hline', first + 1)
    return parse_rows(body[second + len('\\hline'):])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('paper')
    ap.add_argument('tables')
    ap.add_argument('--top', type=int, default=15)
    a = ap.parse_args()
    tex = open(a.paper).read()
    for label, name, cols in TABLES:
        old = paper_rows(tex, label)
        new = parse_rows(open(os.path.join(a.tables, name + '.tex')).read())
        diffs, marks, missing = [], [], []
        for n in sorted(set(old) | set(new)):
            if n not in old or n not in new:
                missing.append(n)
                continue
            for col, (o, oh), (v, vh) in zip(cols, old[n], new[n]):
                if o is None or v is None:
                    if (o is None) != (v is None):
                        diffs.append((math.inf, n, col, o, v))
                    continue
                rel = abs(v / o - 1) if o else (0 if v == 0 else math.inf)
                if rel > 0:
                    diffs.append((rel, n, col, o, v))
                if oh != vh:
                    marks.append((n, col, 'was best' if oh else 'now best'))
        diffs.sort(key=lambda d: -d[0])
        print('== %s: %d cells differ%s' % (name, len(diffs),
              ('; rows only in one: %s' % missing) if missing else ''))
        for rel, n, col, o, v in diffs[:a.top]:
            print('   n=%-13s %-16s old %-11.4g new %-11.4g  %+.2f%%' %
                  ('{:,}'.format(n), col, o if o is not None else float('nan'),
                   v if v is not None else float('nan'),
                   100 * (v / o - 1) if o and v is not None else float('nan')))
        if marks:
            print('   best-solver highlighting changed in %d cells, e.g. %s' %
                  (len(marks), ', '.join('n=%d %s %s' % m for m in marks[:6])))


if __name__ == '__main__':
    main()
