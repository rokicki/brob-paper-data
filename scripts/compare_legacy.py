#!/usr/bin/env python3
"""
Compare fresh averages (data/<solver>/sim.csv) with the previously collected
ones (data/legacy/<solver>_sim.csv) at the sizes both have, in standard
errors and percent.

Usage: compare_legacy.py [--top K]
"""
import argparse
import csv
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = os.path.join(ROOT, 'data')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--top', type=int, default=5)
    args = ap.parse_args()
    for s in ('sortnet96', 'rcube1d', 'rcube2d', 'permnet72'):
        f = os.path.join(DATA, s, 'sim.csv')
        if not os.path.exists(f):
            continue
        old = {int(r['n']): float(r['moves'])
               for r in csv.DictReader(open(os.path.join(DATA, 'legacy', s + '_sim.csv')))}
        rows = []
        for r in csv.DictReader(open(f)):
            n = int(r['n'])
            if n in old:
                new, se = float(r['moves']), float(r['stderr'])
                z = (old[n] - new) / se if se > 0 else float('nan')
                rows.append((n, new, old[n], 100 * (old[n] / new - 1), z, r['width']))
        if not rows:
            continue
        print('%s: %d common sizes, max |diff| %.3f%%, max |z| %.1f' % (
            s, len(rows), max(abs(t[3]) for t in rows),
            max((abs(t[4]) for t in rows if t[4] == t[4]), default=float('nan'))))
        for t in sorted(rows, key=lambda t: -abs(t[3]))[:args.top]:
            print('  n=%-9d new %-14.8g old %-14.8g %+.3f%%  z=%+.1f  w=%s' % t)


if __name__ == '__main__':
    main()
