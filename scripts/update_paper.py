#!/usr/bin/env python3
"""
Copy the paper sources and drop in the regenerated graph and tables.

  update_paper.py PAPER_SRC_DIR OUT_DIR [--label NAME]

The paper is not part of this repo: OUT_DIR must lie outside it (make paper
uses PAPER_OUT, default ../paper).

The rows of Tables tab:maketable5, tab:maketable5b, tab:maketable6 and
tab:discrete-ub are replaced by tables/*.tex, main_graph.pdf by
figures/main_graph.pdf, and every occurrence of the name the paper
sources use for the best solver (permnet62: text, table headings, label
keys) is renamed to NAME (make paper: brobdicube).  The lower-bound
derivation and the graph caption are rewritten to match the data
(LB_EDITS, CAPTION_EDIT).  Other prose, including any numbers it quotes, is
left as it is.
"""
import argparse
import os
import shutil
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# Lower-bound section: branching factor 9n-3 (as used for the data).
LB_EDITS = [
    ('The branching factor for the $n \\times n \\times n$ Rubik\'s Cube in Slice Turn Metric is at most $9n$ (the exact effective value does not affect the leading constant).',
     'In Slice Turn Metric the $n \\times n \\times n$ Rubik\'s Cube has $9n$ moves ($3$ axes, $n$ layers, $3$ turns each); turning the layer just turned again is never useful, so after the first move at most $9n-3$ moves are useful, and we use $9n-3$ as the branching factor (the exact effective value does not affect the leading constant).'),
    ('(9n)^L \\;\\geq\\;', '(9n-3)^L \\;\\geq\\;'),
    ('{\\lg(9n)}.', '{\\lg(9n-3)}.'),
    ('{\\lg(9n) \\cdot n^2}', '{\\lg(9n-3) \\cdot n^2}'),
]
CAPTION_EDIT = (
    'This graph shows how the move count scales for large $n$, on a per-cubie scale (normalized by $n^2 / \\lg n$). Solid lines show worst-case upper bounds; dashed lines show simulated average-case results.',
    'This graph shows how the number of center moves (wings, middle edges and corners excluded) scales for large $n$, normalized by $n^2 / \\lg n$. Solid lines show worst-case upper bounds; dashed lines show simulated average-case results; the dotted line is the number of center cubies, $6(n-2)^2$, normalized the same way.')
STAR_NOTE = (' Entries marked $^{*}$ are beyond the largest simulated size and use the'
             ' validated expected-cost model.')


def table_span(tex, label):
    """(start, end) of the rows of the tabular holding \\label{label}."""
    lab = tex.index('\\label{%s}' % label)
    beg = tex.rindex('\\begin{tabular}', 0, lab)
    if beg < tex.rfind('\\end{table}', 0, lab):       # label precedes tabular
        beg = tex.index('\\begin{tabular}', lab)
    end = tex.index('\\end{tabular}', beg)
    body = tex[beg:end]
    if '\\midrule' in body:
        s = beg + body.index('\\midrule') + len('\\midrule\n')
        e = beg + body.rindex('\\bottomrule')
    else:
        first = body.index('\\hline')
        second = body.index('\\hline', first + 1)
        s = beg + second + len('\\hline')
        e = beg + body.rindex('\\hline')
    return s, e


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('src')
    ap.add_argument('out')
    ap.add_argument('--label', default='permnet62')
    a = ap.parse_args()
    if os.path.abspath(a.src) == os.path.abspath(a.out):
        sys.exit('refusing to overwrite the source directory')
    out = os.path.realpath(a.out)
    if out == os.path.realpath(ROOT) or out.startswith(os.path.realpath(ROOT) + os.sep):
        sys.exit('refusing to write the paper inside this repo; give an OUT_DIR outside it')
    shutil.copytree(a.src, a.out, dirs_exist_ok=True,
                    ignore=shutil.ignore_patterns('*.aux', '*.log', '*.bbl', '*.blg',
                                                  '*.out', '*.synctex*', 'main.pdf'))
    path = os.path.join(a.out, 'main.tex')
    tex = open(path).read()
    for label, name in (('tab:maketable5', 'maketable5'), ('tab:maketable5b', 'maketable5b'),
                        ('tab:maketable6', 'maketable6'), ('tab:discrete-ub', 'discrete_ub')):
        s, e = table_span(tex, label)
        rows = open(os.path.join(ROOT, 'tables', name + '.tex')).read()
        tex = tex[:s] + '\n\n' + rows + '    ' + tex[e:]
        if name in ('maketable5b', 'maketable6') and '$^{*}$' in rows:
            lab = tex.index('\\label{%s}' % label)
            cap = tex.rindex('\\caption{', 0, lab)
            close = tex.index('}\n', cap)
            tex = tex[:close] + STAR_NOTE + tex[close:]
    for old, new in LB_EDITS + [CAPTION_EDIT]:
        if tex.count(old) != 1:
            sys.exit('lower-bound text not found exactly once: ' + old[:60])
        tex = tex.replace(old, new)
    if a.label != 'permnet62':
        tex = tex.replace('permnet62', a.label)
    open(path, 'w').write(tex)
    shutil.copy(os.path.join(ROOT, 'figures', 'main_graph.pdf'), os.path.join(a.out, 'main_graph.pdf'))


if __name__ == '__main__':
    main()
