#!/usr/bin/env python3
"""
Which decomposition brobdicube picks, by n, for the worst case and the
average case: tables/decomp_worst.txt and tables/decomp_avg.txt, the worst
case with one-row strips (-p 1: no 2D rectangles, linear combinations only,
like rcube1d) in tables/decomp_worst_p1.txt, the average case likewise in
tables/decomp_avg_p1.txt (text files only), and the -p max tables for the
paper as tables/decomp_{worst,avg,keys}.tex (the two decomposition tables
and a table of what the keys mean; generated, not edited by hand).  The
prose of the paper section is not part of this repo.

Reads brobdicube's plan logs data/brobdicube/runs/worst.out (-w -p max -P
list), worst-p1.out (-w -p 1 -P list), model.out (-p max -P list) and
model-p1.out (-p 1 -P list), one
plan per size in MODEL_N (plus the table sizes for the worst case).  A plan
is the chain of "Use" lines, each a step: the key as applied, the planning
mode (greedy or optimal) and the stages used (brobdicube's used_perms: the
network's stages with nonzero probability under that mode; the user,
2026-10-15: that is the stage count that matters).

A decomposition is the sequence of steps, where a greedy step is its key and
stages used, and an optimal step is its key alone, written "key opt": the
average planner picks optimal mode on long networks whose length grows with
n, where the stage count says little (user, 2026-10-15).  So networks that
differ only in their hash, or optimal-mode networks of any length, are the
same decomposition.  "total" is the stages used in all steps, as the range
lowest-highest over the sizes where it was picked when that varies.  The
worst-case log has greedy steps only, each using all its network's stages.

Each decomposition gets one row with the first and last size at which it is
picked, in order of the first size.  Where the planner alternates between
decompositions their ranges overlap; "picked" says at how many of the
logged sizes inside its range it was chosen.  Sizes are only those logged
(every even n to 256, then 16 per octave plus the table sizes), so a range
ends at logged sizes.

A key names a step: one character per face in Speffz order U L F R B D; x is
a face the step does not touch, and the step permutes the cubies of the
other (active) faces so that faces with the same digit together hold the
cubies that belong on them (checked against the logs: each step's key is
the partition it reaches from the "on" state before it).
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import ROOT   # noqa: E402

RUNS = os.path.join(ROOT, 'data', 'brobdicube', 'runs')
USE = re.compile(r'^Use (\S+) \S+ \[(greedy|optimal)\] cost \S+ on \S+ map \d+ used (\d+)/(\d+)$')


def plans(path):
    """{n: ((key, mode, used), ...)} from a -P list log."""
    out = {}
    n = steps = None
    for line in open(path):
        line = line.rstrip('\n')
        if line.startswith('N='):
            n, steps = int(line.split()[0][2:]), []
        elif line.startswith('Use '):
            m = USE.match(line)
            if not m:
                sys.exit('%s: cannot parse %r' % (path, line))
            steps.append((m.group(1), m.group(2), int(m.group(3))))
        elif line.startswith('Finished'):
            if n in out:
                sys.exit('%s: n=%d twice' % (path, n))
            out[n] = tuple(steps)
            n = steps = None
    return out


def decomposition(steps):
    return tuple((k, u if mode == 'greedy' else None) for k, mode, u in steps)


def fmt(d):
    return '  '.join('%s %s' % (k, 'opt' if u is None else u) for k, u in d)


FACES = 'ULFRBD'


def rows_of(p):
    """[(first n, last n, picked, logged sizes in range, lowest total, highest total,
    decomposition)] in order of the first n."""
    ns = sorted(p)
    pick = {n: decomposition(p[n]) for n in ns}
    first, last, totals = {}, {}, {}
    for n in ns:
        d = pick[n]
        first.setdefault(d, n)
        last[d] = n
        totals.setdefault(d, []).append(sum(u for _, _, u in p[n]))
    out = []
    for d in sorted(first, key=lambda d: (first[d], last[d])):
        inside = [n for n in ns if first[d] <= n <= last[d]]
        out.append((first[d], last[d], sum(1 for n in inside if pick[n] == d), len(inside),
                    min(totals[d]), max(totals[d]), d))
    return out


def write(path, title, src, p):
    ns = sorted(p)
    rows = rows_of(p)
    with open(path, 'w') as fp:
        fp.write('# %s decompositions picked by brobdicube, by n (from %s)\n' % (title, src))
        fp.write('# %d logged sizes, n = %d..%d; ranges overlap where the choice alternates\n'
                 % (len(ns), ns[0], ns[-1]))
        fp.write('# steps: key as applied, then stages used (greedy) or opt (optimal, any length);\n'
                 '# total = stages used in all steps, lowest-highest where it varies\n')
        fp.write('%12s %12s %9s %9s  %s\n' % ('from n', 'to n', 'picked', 'total', 'steps'))
        for a, b, k, m, lo, hi, d in rows:
            fp.write('%12d %12d %9s %9s  %s\n' % (a, b, '%d/%d' % (k, m),
                                                  lo if lo == hi else '%d-%d' % (lo, hi), fmt(d)))
    print('%s: %d decompositions' % (path, len(rows)))


def key_info(key):
    """(active faces, split as 'a{+}b', groups) of a step key."""
    groups = {}
    for f, c in zip(FACES, key):
        if c not in 'xX':
            groups.setdefault(c, []).append(f)
    gs = list(groups.values())     # in face order
    active = sum(len(g) for g in gs)
    split = '$%d\\to%s$' % (4 * active, '{+}'.join(str(4 * len(g)) for g in gs))
    names = ', '.join(g[0] if len(g) == 1 else '\\{%s\\}' % ','.join(g) for g in gs)
    return active, split, names


def tex_steps(d):
    return '\\enspace '.join('\\texttt{%s}\\,%s' % (k, '\\textit{opt}' if u is None else u)
                               for k, u in d)


def tex_table(rows, label, caption):
    out = ['\\begin{table}[htbp]', '  \\centering \\footnotesize',
           '  \\setlength{\\tabcolsep}{4pt}',
           '  \\begin{tabular}{r r r r l}', '    \\toprule',
           '    from $n$ & to $n$ & chosen & stages & steps (key, stages used) \\\\',
           '    \\midrule']
    for a, b, k, m, lo, hi, d in rows:
        out.append('    %s & %s & %d/%d & %s & %s \\\\' % (
            '{:,}'.format(a), '{:,}'.format(b), k, m,
            lo if lo == hi else '%d--%d' % (lo, hi), tex_steps(d)))
    out += ['    \\bottomrule', '  \\end{tabular}', '  \\caption{%s}' % caption,
            '  \\label{%s}' % label, '\\end{table}']
    return '\n'.join(out)


def tex(out, worst, avg):
    """The tables for the paper: tables/decomp_worst.tex, decomp_avg.tex and
    decomp_keys.tex (what the keys mean).  The section's prose is not part of
    this repo (user, 2026-10-28); the one-row-strip (-p 1) plans are left out
    (user, 2026-10-17)."""
    rw, ra = rows_of(worst), rows_of(avg)
    keys = {}
    for tag, rows in (('W', rw), ('A', ra)):
        for r in rows:
            for k, _ in r[6]:
                keys.setdefault(k, set()).add(tag)
    keyrows = sorted(keys, key=lambda k: (-key_info(k)[0], key_info(k)[1], k))
    head = ('%% Generated by scripts/decompositions.py from data/brobdicube/runs/%s\n'
            '%% (brobdicube -P list -p max%s).  Do not edit by hand.\n')
    tables = {
        'decomp_worst': (head % ('worst.out', ', -w')) + tex_table(
            rw, 'tab:decomp-worst',
            'Decompositions chosen for the worst-case bound, by $n$. Each row gives the '
            'first and last size at which the decomposition is chosen, at how many of the '
            'sizes in that range it is chosen, the total number of stages, and the steps '
            '(key, then stages used; keys in Table~\\ref{tab:decomp-keys}). Ranges overlap '
            'where the choice alternates.'),
        'decomp_avg': (head % ('model.out', '')) + tex_table(
            ra, 'tab:decomp-avg',
            'Decompositions chosen for the expected move count, by $n$, as in '
            'Table~\\ref{tab:decomp-worst}. \\textit{opt} marks an optimally planned step, '
            'whose network length varies with $n$; the stage total is then the range over '
            'the sizes at which the decomposition is chosen. All other steps are planned '
            'greedily.'),
    }
    out_ = ['\\begin{table}[htbp]', '  \\centering \\footnotesize',
            '  \\setlength{\\tabcolsep}{5pt}', '  \\begin{tabular}{l l l c}', '    \\toprule',
            '    key & split & groups after the step & used in \\\\', '    \\midrule']
    for k in keyrows:
        _, split, names = key_info(k)
        out_.append('    \\texttt{%s} & %s & %s & %s \\\\' % (k, split, names,
                                                          ', '.join(t for t in 'WA' if t in keys[k])))
    out_ += ['    \\bottomrule', '  \\end{tabular}',
             '  \\caption{Keys of the reduction steps in Tables~\\ref{tab:decomp-worst} '
             '(W) and~\\ref{tab:decomp-avg} (A). The characters are the faces in Speffz order '
             'U, L, F, R, B, D; \\texttt{x} marks a face the step does not touch. The step '
             'permutes the cubies on the active faces so that the faces sharing a digit '
             'together hold the cubies that belong on them. The split gives the sizes of the '
             'groups in cubies, in the order the groups are listed.}',
             '  \\label{tab:decomp-keys}', '\\end{table}']
    tables['decomp_keys'] = (head % ('worst.out and model.out', '')) + '\n'.join(out_)
    for name, t in tables.items():
        path = os.path.join(out, name + '.tex')
        open(path, 'w').write(t + '\n')
        print(path)


def main():
    out = os.path.join(ROOT, 'tables')
    logs = {}
    for mode, title, log in (('worst', 'Worst-case', 'worst.out'),
                             ('worst_p1', 'Worst-case one-row-strip (-p 1)', 'worst-p1.out'),
                             ('avg', 'Average-case', 'model.out'),
                             ('avg_p1', 'Average-case one-row-strip (-p 1)', 'model-p1.out')):
        logs[mode] = plans(os.path.join(RUNS, log))
        write(os.path.join(out, 'decomp_%s.txt' % mode), title, 'data/brobdicube/runs/' + log,
              logs[mode])
    tex(out, logs['worst'], logs['avg'])


if __name__ == '__main__':
    main()
