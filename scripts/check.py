#!/usr/bin/env python3
"""
Check that the stored brobdicube data reproduce on this machine.

  plans  Rerun brobdicube's planner (-P list) at every size in
         data/brobdicube/runs/<kind>.out, for the four kinds that
         run_brobdicube.py writes (worst, model, worst-p1, model-p1), and
         compare the output with the stored log line by line (all but the
         command line and the load time): every word must be the same,
         except that numbers may differ by a relative 1e-9 (the expected
         costs are floating-point sums, and their 15th digit can differ
         between platforms).
  sims   Rerun the stored simulation runs with n <= --max-n from this
         platform's runs files and compare mean and sd exactly.  Runs
         reproduce only on the platform that made them (the solver scrambles
         with std::shuffle, which differs between libstdc++ and libc++):
         mac-sim.csv on macOS, sim-arena*.csv on Linux.

Exits with status 1 if anything differs.

Usage: check.py [--brob PATH] [--jobs J] [--max-n N] [plans] [sims]
"""
import argparse
import glob
import os
import platform
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import ROOT   # noqa: E402
from run_brobdicube import Csv, SIM_HEADER, brob, parallel, sim_cmd, sim_parse   # noqa: E402

RUNS = os.path.join(ROOT, 'data', 'brobdicube', 'runs')
KINDS = {'worst': ['-w', '-p', 'max'], 'model': ['-p', 'max'],
         'worst-p1': ['-w', '-p', '1'], 'model-p1': ['-p', '1']}


def body(text):
    """The planner output without the command line and the load time."""
    return [l for l in text.splitlines() if not l.startswith(('- ', 'Loaded '))]


def same_line(a, b):
    wa, wb = a.split(), b.split()
    if len(wa) != len(wb):
        return False
    for x, y in zip(wa, wb):
        if x == y:
            continue
        try:
            fx, fy = float(x), float(y)
        except ValueError:
            return False
        if abs(fx - fy) > 1e-9 * max(abs(fx), abs(fy)):
            return False
    return True


def check_plans(args):
    bad = 0
    for kind, flags in KINDS.items():
        stored = open(os.path.join(RUNS, kind + '.out')).read()
        sizes = re.findall(r'^N=(\d+) ', stored, re.M)
        r = subprocess.run([args.brob] + flags + ['-t', str(args.jobs), '-P', 'list'],
                           input='\n'.join(sizes) + '\n', capture_output=True, text=True,
                           cwd=os.path.dirname(args.brob))
        old, new = body(stored), body(r.stdout + r.stderr)
        same = r.returncode == 0 and len(old) == len(new) and all(map(same_line, old, new))
        print('plans %-9s %3d sizes: %s' % (kind, len(sizes), 'same' if same else 'DIFFERENT'),
              flush=True)
        if not same:
            bad += 1
            for i, (a, b) in enumerate(zip(old, new)):
                if not same_line(a, b):
                    print('  first difference at line %d:\n    stored: %s\n    now:    %s' % (i, a, b))
                    break
            else:
                print('  stored %d lines, now %d' % (len(old), len(new)))
    return bad


def check_sims(args):
    pattern = 'mac-sim.csv' if platform.system() == 'Darwin' else 'sim-arena*.csv'
    rows = []
    for path in sorted(glob.glob(os.path.join(RUNS, pattern))):
        rows += [r for r in Csv(path, SIM_HEADER).rows() if int(r['n']) <= args.max_n]
    bad = []

    def one(r):
        n, count = int(r['n']), int(r['count'])
        mean, sd = sim_parse(brob(args, sim_cmd(n, r['p'], r['seed'], count)), n, count)
        if not (mean == float(r['mean']) and sd == float(r['sd'])):
            bad.append(r)
            print('sims n=%d seed=%s: DIFFERENT (%r, %r; stored %s, %s)'
                  % (n, r['seed'], mean, sd, r['mean'], r['sd']), flush=True)

    parallel(args.jobs, one, rows)
    print('sims  %s, n <= %d: %d runs, %d differ' % (pattern, args.max_n, len(rows), len(bad)),
          flush=True)
    return len(bad)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--brob', default=os.path.join(ROOT, 'brobdicube', 'brobdicube'))
    ap.add_argument('--jobs', type=int, default=max(1, (os.cpu_count() or 2) - 2))
    ap.add_argument('--max-n', type=int, default=64)
    ap.add_argument('steps', nargs='*', default=['plans', 'sims'])
    args = ap.parse_args()
    args.brob = os.path.abspath(args.brob)
    bad = 0
    if 'plans' in args.steps:
        bad += check_plans(args)
    if 'sims' in args.steps:
        bad += check_sims(args)
    print('check: %s' % ('OK' if bad == 0 else 'FAILED'))
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
