#!/usr/bin/env python3
"""
Data for the interactive graph page (figures/explorer.html): the w = 0 rows
of results.txt grouped by algorithm, plus the reference lines, as JSON.

  algorithms: [{name, worst: [[n, moves]], avg | greedy, optimal: [[n, moves]]}]
     worst is the same for -greedy and -optimal (checked); the union of the
     sizes is used.  Algorithms with no greedy/optimal split have avg.
     The permnet61 averages continue above their largest simulated size
     with brobdicube's -p max planner estimate (results.txt column model),
     each such point tagged 'planner estimate' (user, 2026-10-12: whole-cube
     -p max runs at 2^16 agree with the estimate to 3.1e-6; -p <= 64 cannot
     reproduce the -p max plan at 2^30).  Points are [n, moves] or
     [n, moves, note].
     brobdicube-p1 is brobdicube with one-row strips (-p 1: no 2D
     rectangles, linear combinations only), from its plan logs
     data/brobdicube/runs/{worst,model}-p1.csv: the worst case (15 digits; brobdicube prints the exact integer only when the strip is the whole cube) and
     the planner's average estimate (tagged 'planner estimate'; not
     simulated).
  refs: lower bound, center count 6(n-2)^2, 0D upper bound 10(n-2)^2
     (user, 2026-10-09; the paper's tables and graph still use
     common.upper0d = 36(n-2)^2), at every MODEL_N size.

Only even n >= 4.  Writes figures/explorer_data.json, or the path given, and
figures/explorer.html (figures/explorer.src.html with the data inlined).
"""
import csv
import json
import os
import sys
from collections import defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import ROOT, centers, lower   # noqa: E402
from sizes import MODEL_N   # noqa: E402

ORDER = ['rcube1d', 'rcube2d', 'sortnet96', 'permnet72'] + \
    ['permnet%d' % k for k in range(67, 61, -1)] + \
    ['permnet61W', 'permnet61A', 'permnet61B', 'brobdicube']


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'figures', 'explorer_data.json')
    sim = defaultdict(dict)
    worst = defaultdict(dict)
    model = defaultdict(dict)
    for line in open(os.path.join(ROOT, 'results.txt')):
        f = line.split()
        if not f or f[0].startswith('#') or f[0] == 'case' or f[2] != '0':
            continue
        n = int(f[1])
        if n < 4 or n % 2:
            continue
        if f[4] != '-':
            sim[f[0]][n] = float(f[4])
        if len(f) > 7 and f[7] != '-':
            worst[f[0]][n] = int(f[7])
        if len(f) > 8 and f[8] != '-':
            model[f[0]][n] = float(f[8])
    algs = []
    for name in ORDER:
        a = {'name': name}
        if name + '-greedy' in sim:
            parts = [name + '-greedy', name + '-optimal']
            a['greedy'] = sorted(sim[parts[0]].items())
            a['optimal'] = sorted(sim[parts[1]].items())
            if name.startswith('permnet61'):
                for k, part in zip(('greedy', 'optimal'), parts):
                    top = max(n for n, _ in a[k])
                    a[k] = [list(p) for p in a[k]] + \
                        [[n, v, 'planner estimate'] for n, v in sorted(model[part].items()) if n > top]
        else:
            parts = [name]
            a['avg'] = sorted(sim[name].items())
        w = {}
        for p in parts:
            for n, v in worst[p].items():
                if w.setdefault(n, v) != v:
                    sys.exit('%s n=%d: worst cases differ' % (name, n))
        a['worst'] = sorted(w.items())
        algs.append(a)
    runs = os.path.join(ROOT, 'data', 'brobdicube', 'runs')
    p1 = {}
    for kind in ('worst', 'model'):
        for r in csv.DictReader(open(os.path.join(runs, kind + '-p1.csv'))):
            n = int(r['n'])
            if n >= 4 and n % 2 == 0:
                p1.setdefault(kind, []).append((n, r['exact'] or r['moves']))
    algs.append({'name': 'brobdicube-p1',
                 'worst': sorted([n, round(float(v))] for n, v in p1['worst']),
                 'avg': sorted([n, float(v), 'planner estimate'] for n, v in p1['model'])})
    ns = [n for n in MODEL_N if n >= 4 and n % 2 == 0]
    refs = [{'name': 'lower bound', 'pts': [[n, lower(n)] for n in ns]},
            {'name': 'center count', 'pts': [[n, centers(n)] for n in ns]},
            {'name': '0D upper bound', 'pts': [[n, 10 * (n - 2) ** 2] for n in ns]}]
    with open(out, 'w') as fp:
        json.dump({'algorithms': algs, 'refs': refs}, fp, separators=(',', ':'))
    print('%s: %d algorithms' % (out, len(algs)))
    src = os.path.join(ROOT, 'figures', 'explorer.src.html')
    page = open(src).read().replace('/*DATA*/', json.dumps({'algorithms': algs, 'refs': refs},
                                                           separators=(',', ':')))
    dst = os.path.join(ROOT, 'figures', 'explorer.html')
    open(dst, 'w').write(page)
    print(dst)


if __name__ == '__main__':
    main()
