#!/usr/bin/env python3
"""
How was this number obtained?  For a solver and a cube size, print the
summary value, every raw run pooled into it (data/<solver>/runs/sim*.csv),
and each run's full output from the .out file beside its runs file (the
output starts with the solver's command line).  brobdicube also shows its
model and worst-case planning output; the comparison solvers' worst cases
are formulas (scripts/bounds.py, scripts/permnet_bounds.py).

Usage: provenance.py SOLVER N [--no-output]
"""
import csv
import glob
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run_others                                   # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = os.path.join(ROOT, 'data')


def blocks(path):
    """Map each run's arguments (program path dropped) to its output."""
    r, cur = {}, None
    if not os.path.exists(path):
        return r
    for line in open(path):
        if line.startswith('- '):
            cur = tuple(line.split()[2:])
            r[cur] = [line]
        elif cur is not None:
            r[cur].append(line)
    return {k: ''.join(v) for k, v in r.items()}


def model_block(path, n):
    """The block of brobdicube's model/worst output for size n."""
    if not os.path.exists(path):
        return None
    text = open(path).read()
    i = text.find('N=%d Estimated' % n)
    if i < 0:
        return None
    j = text.find('\nN=', i + 1)
    return text[i:j if j > 0 else len(text)]


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    show = '--no-output' not in sys.argv
    if len(args) != 2:
        sys.exit(__doc__)
    solver, n = args[0], int(args[1])
    d = os.path.join(DATA, solver)
    for name in ('sim.csv', 'model.csv', 'worst.csv', 'upper.csv'):
        p = os.path.join(d, name)
        if os.path.exists(p):
            for r in csv.DictReader(open(p)):
                if int(r['n']) == n:
                    print('%s: %s' % (name, ', '.join('%s=%s' % kv for kv in r.items()
                                                      if kv[0] not in ('algs', 'widths'))))
    if solver == 'brobdicube':
        for kind in ('model', 'worst'):
            b = model_block(os.path.join(d, 'runs', kind + '.out'), n)
            print('\n--- %s planning output%s' % (kind, '' if b else ': none saved'))
            if b and show:
                print(b)
    tuned = solver in run_others.SOLVERS and run_others.SOLVERS[solver].widths == run_others.TUNE
    for p in sorted(glob.glob(os.path.join(d, 'runs', '*.csv'))):
        base = os.path.basename(p)
        if base in ('model.csv', 'worst.csv'):
            continue
        rows = [r for r in csv.DictReader(open(p)) if int(r['n']) == n]
        if not rows:
            continue
        pooled = base.startswith('sim')
        if tuned:
            w = run_others.width(solver, n)
            print('\nchosen width %d (data/%s/widths.csv)' % (w, solver))
        out = blocks(os.path.splitext(p)[0] + '.out')
        print('\n=== %s (%s)' % (os.path.relpath(p, ROOT), 'pooled' if pooled else 'not pooled'))
        for r in rows:
            use = pooled and not (tuned and int(r['w']) != w)
            if solver == 'brobdicube':
                key = ('-t', '1', '-x', '-p', r['p'], '-z', r['seed'], '-M', r['count'], r['n'])
            else:
                key = tuple(run_others.sim_cmd(solver, r['n'], r['w'], r['p'], r['seed'],
                                               r['count'])[1:])
            b = out.get(key)
            print('%s %s  -> %s' % ('*' if use else ' ',
                                    ' '.join('%s=%s' % kv for kv in r.items()),
                                    'output saved' if b else 'NO OUTPUT SAVED'))
            if b and show and use:
                print('    ' + b.rstrip('\n').replace('\n', '\n    '))
    print('\n(* = pooled into sim.csv)')


if __name__ == '__main__':
    main()
