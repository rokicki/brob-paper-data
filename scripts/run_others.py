#!/usr/bin/env python3
"""
Measured center moves for the comparison solvers (centers only), the
counterpart of run_brobdicube.py's sim step:

  sortnet96  sortnet96/solve96 -6 -8   12 face-pair sorting network x 8 stages
  rcube1d    sortnet96/solve96 -w 1    default (16-swap, 240-stage) mode = the
                                       RCube/ShellPuppy model, one row at a time
  rcube2d    sortnet96/solve96 -w W    same, W rows at a time
  permnet72  permnet72/brobdicube      the 72-stage solver (permnet72)
  permnetL-greedy, permnetL-optimal (L = 62 .. 67)
             brob2/brob2 -a O I [-m]   outer O + 2 x inner I = L stages, with
                                       first-found (greedy) or fewest-stage
                                       (optimal, -m) plans

Every run uses the whole cube per pass (-p My):
  n <= FULL_MAX: whole cubes, K solves per run (-M K);
  larger n:      -x: one strip of p rows (p a multiple of the width, so the
                 strip holds whole width groups) is solved and scaled up.
The width W is measured by the `tune` step (see do_tune) into
data/<solver>/widths.csv, and only runs at the chosen width are pooled.
Tuning starts from the widths of earlier runs of these programs
(olddata/arena/<file>.txt, column 2, at the nearest size), which are also
the fallback where no tuned size is within a factor of 2.

Every run is seeded (-z), raw runs are appended to
data/<solver>/runs/<runs-file>, and the step is resumable.  Sample counts are
chosen from a pilot so that the standard error is at most TARGET_RSE of the
mean.  `summary` pools all runs/*.csv into data/<solver>/sim.csv.

Usage: run_others.py [--solvers a,b|permnets] [--sizes n,...] [--jobs J]
                     [--mem-gb G] [--runs-file F] [plan] [tune] [sim] [replay] [summary]

--solvers also takes the groups permnets, net61, all.
"""
import argparse
import bisect
import csv
import glob
import math
import os
import re
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sizes import SIM_N, TABLE_N, MODEL_N, DISCRETE_N    # noqa: E402
from run_brobdicube import Csv, parallel, pooled     # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = os.path.join(ROOT, 'data')
OLD = os.path.join(ROOT, 'olddata', 'arena')

S96 = os.path.join(ROOT, 'sortnet96', 'solve96')
P72 = os.path.join(ROOT, 'permnet72', 'brobdicube')
B2 = os.path.join(ROOT, 'brob2', 'brob2')
BD = os.path.join(ROOT, 'brobdicube', 'brobdicube')
NETS = os.path.join(ROOT, 'brobdicube', 'nets')
TUNE = 'tune'      # widths measured by the tune step (data/<solver>/widths.csv)
PLANNER = 'plan'   # brobdicube picks the widths itself (recorded as w = 0)


class Solver:
    """prog/flags: the command; widths: 1, TUNE or PLANNER; guess: the
    olddata/arena file of earlier widths (tune starts there; also the fallback
    where no tuned size is within a factor of 2);
    brob: brobdicube-style command (-t 1 -x -p, no strip width)."""
    def __init__(self, prog, flags, widths, guess=None, brob=False):
        self.prog, self.flags, self.widths, self.guess, self.brob = prog, flags, widths, guess, brob


SOLVERS = {
    'sortnet96': Solver(S96, ['-6', '-8'], TUNE, 'sort96.txt'),
    'rcube1d': Solver(S96, [], 1),
    'rcube2d': Solver(S96, [], TUNE, 'rcube2d.txt'),
    'permnet72': Solver(P72, [], TUNE, 'permnet.txt'),
}
DEFAULT_SOLVERS = list(SOLVERS)
SOLVERS['permnet72-optimal'] = Solver(P72, ['-m'], TUNE, 'permnet.txt')
# brob2's six networks: stages -> (outer, inner)
PERMNETS = {62: (26, 18), 63: (27, 18), 64: (26, 19), 65: (27, 19), 66: (26, 20), 67: (27, 20)}
for _l, (_o, _i) in PERMNETS.items():
    SOLVERS['permnet%d-greedy' % _l] = Solver(B2, ['-a', str(_o), str(_i)], TUNE, 'permnet.txt')
    SOLVERS['permnet%d-optimal' % _l] = Solver(B2, ['-m', '-a', str(_o), str(_i)], TUNE, 'permnet.txt')
PERMNET_SOLVERS = [s for s in SOLVERS if s.startswith('permnet6')]
# brobdicube restricted to one 61-stage network (brobdicube/nets/*)
for _name, _net in (('W', 'W61'), ('A', 'A61a'), ('B', 'A61b')):
    for _mode, _f in (('greedy', '-g'), ('optimal', '-O')):
        SOLVERS['permnet61%s-%s' % (_name, _mode)] = Solver(
            BD, ['-D', os.path.join(NETS, _net), _f], PLANNER, brob=True)
NET61_SOLVERS = [s for s in SOLVERS if s.startswith('permnet61')]
GROUPS = {'permnets': PERMNET_SOLVERS, 'net61': NET61_SOLVERS,
          'all': [s for s in SOLVERS]}

FULL_MAX = 4096            # solve whole cubes up to here, strips above
STRIP_ORBITS = 1.5e8       # target orbits per strip
MAX_TUNE_W = 22           # tune never tries wider strips (time ~ 2^w per strip)
TARGET_RSE = 5e-4          # standard error <= 0.05% of the mean
ONE_SAMPLE_N = 1 << 20     # above this, one sample per size, no top-up (user, 2026-10-10)
SIM_HEADER = ['n', 'w', 'p', 'seed', 'count', 'mean', 'sd']

_widths = {}


def nearest(tab, n):
    i = bisect.bisect_left(tab, (n, 0))
    near = [tab[j] for j in (i - 1, i) if 0 <= j < len(tab)]
    return min(near, key=lambda t: abs(math.log(t[0] / n)))[1]


def earlier_width(f, n):
    if f not in _widths:
        d = {}
        for line in open(os.path.join(OLD, f)):
            x = line.split()
            d[int(x[0])] = int(x[1])
        _widths[f] = sorted(d.items())
    return nearest(_widths[f], n)


def tuned(solver):
    if solver not in _widths:
        path = os.path.join(DATA, solver, 'widths.csv')
        _widths[solver] = sorted((int(r['n']), int(r['w'])) for r in
                                 csv.DictReader(open(path))) if os.path.exists(path) else []
    return _widths[solver]


def width(solver, n):
    """The strip width runs at size n use: the tuned width of the nearest
    tuned size (within a factor of 2), else the earlier width."""
    sv = SOLVERS[solver]
    if sv.widths == PLANNER:
        return 0
    if sv.widths != TUNE:
        return sv.widths
    tab = tuned(solver)
    near = [t for t in tab if abs(math.log(t[0] / n)) <= math.log(2)]
    if near:
        return nearest(near, n)
    if sv.guess:
        return earlier_width(sv.guess, n)
    sys.exit('%s: no tuned width near n=%d; run the tune step first' % (solver, n))


def dims(n):
    my = (n - 2) // 2
    return n - 2 - my, my


def strip_height(solver, n, w=None):
    mx, my = dims(n)
    if n <= FULL_MAX:
        return my
    if SOLVERS[solver].brob:        # as run_brobdicube.py
        return min(my, max(64, min(1024, int(STRIP_ORBITS / mx))))
    w = w or width(solver, n)
    k = max(1, int(min(1024, STRIP_ORBITS / mx) // w))   # whole width groups
    return min(my // w * w, k * w)


def mem_gb(solver, n, w=None):
    """Measured on arena: about 25 bytes per orbit of the strip (34 for
    brob2), plus brob2's 2^w-entry pattern table."""
    mx, _ = dims(n)
    if SOLVERS[solver].brob:        # as run_brobdicube.py
        return 1.8 + 36e-9 * mx * strip_height(solver, n)
    w = w or width(solver, n)
    per = 36e-9 if SOLVERS[solver].prog == B2 else 30e-9
    return 0.2 + per * mx * strip_height(solver, n, w) + 4e-9 * 2 ** w


def kmin(n):
    if n <= 256:
        return 64
    if n <= 1024:
        return 16
    if n <= FULL_MAX:
        return 4
    if n > ONE_SAMPLE_N:
        return 1        # one strip run samples ~n/2 orbits; repeats agree to ~1e-6
    return 3


def seed(n, run):
    return n * 1000 + run


def sim_cmd(solver, n, w, p, sd, count):
    """The command line (as run) of one sim run."""
    sv = SOLVERS[solver]
    if sv.brob:                     # as run_brobdicube.py: always -x
        return [sv.prog, '-t', '1'] + sv.flags + ['-x', '-p', str(p), '-z', str(sd),
                                                 '-M', str(count), str(n)]
    cmd = [sv.prog] + sv.flags + ['-w', str(w), '-p', str(p), '-z', str(sd), '-M', str(count)]
    if int(n) > FULL_MAX:
        cmd.insert(1, '-x')
    return cmd + [str(n)]


def run_parse(cmd, n, count):
    """Run one command; return (mean, sd, full output)."""
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError('%s failed:\n%s%s' % (' '.join(cmd), r.stdout, r.stderr))
    if count == 1:
        m = re.search(r'^N %d .*?\bmoves (\S+)' % n, r.stdout, re.M)
        mean, sd = float(m.group(1)), 0.0
    else:
        m = re.search(r'^N %d .*?\bcnt %d moves (\S+) stddev (\S+)' % (n, count),
                      r.stdout, re.M)
        mean, sd = float(m.group(1)), float(m.group(2))
    return mean, sd, r.stdout + r.stderr


def sim_run(solver, table, n, run, count, w=None):
    w = width(solver, n) if w is None else w
    p = strip_height(solver, n, w)
    mean, sd, out = run_parse(sim_cmd(solver, n, w, p, seed(n, run), count), n, count)
    table.add({'n': n, 'w': w, 'p': p, 'seed': seed(n, run), 'count': count,
               'mean': repr(mean), 'sd': repr(sd)}, out)
    print('%s n=%d w=%d p=%d run=%d count=%d mean=%.10g' % (solver, n, w, p, run, count, mean),
          flush=True)


def safe_sim_run(*a):
    try:
        sim_run(*a)
    except Exception as e:
        print('FAILED %s n=%d run=%d: %s' % (a[0], a[2], a[3], e), flush=True)


def do_sim(args):
    tables = {s: Csv(os.path.join(DATA, s, 'runs', args.runs_file), SIM_HEADER)
              for s in args.solvers}

    def runs_for(s, n):
        w = width(s, n)
        return [r for r in tables[s].rows() if int(r['n']) == n and int(r['w']) == w]

    def go(work):
        work.sort(key=lambda t: -t[1] * t[1] * t[3])     # longest first
        parallel(args.jobs, lambda t: safe_sim_run(t[0], tables[t[0]], *t[1:]), work,
                 mem=lambda t: mem_gb(t[0], t[1]), mem_limit=args.mem_gb)

    # Pass 1: pilots.  Whole cubes: one process doing kmin solves.
    # Strips: kmin processes of one solve each (they parallelize).
    pilots = []
    for s in args.solvers:
        for n in args.sizes:
            have = runs_for(s, n)
            if n <= FULL_MAX:
                if not have:
                    pilots.append((s, n, 0, kmin(n)))
            else:
                used = {int(r['seed']) for r in have}
                pilots += [(s, n, i, 1) for i in range(kmin(n)) if seed(n, i) not in used]
    go(pilots)

    # Pass 2: top up to the target relative standard error.
    extra = []
    for s in args.solvers:
        for n in args.sizes:
            rows = runs_for(s, n)
            if n > ONE_SAMPLE_N:
                continue
            if sum(int(r['count']) for r in rows) < 2:
                continue                    # a pilot failed; rerun the step
            mean, sd, N = pooled(rows)
            need = math.ceil((sd / (TARGET_RSE * mean)) ** 2)
            if need > N:
                nxt = 1 + max(int(r['seed']) - n * 1000 for r in rows)
                if n <= FULL_MAX:
                    extra.append((s, n, nxt, need - N))
                else:
                    extra += [(s, n, nxt + i, 1) for i in range(need - N)]
    go(extra)


def tune_sizes(sizes):
    """Whole-cube sizes are tuned one by one; strip sizes only at powers of
    two and the table sizes, the others take the width of the nearest one."""
    return [n for n in sizes if n <= FULL_MAX or n & (n - 1) == 0 or n in TABLE_N]


def do_tune(args):
    """
    Choose the strip width of each permnetL solver.  At each tuned size the
    candidates start at the permnet72 width (olddata/arena/permnet.txt, which
    brob2's best widths matched in a sweep to 8192) +-1, or 1..4 for n <= 64,
    and grow while the best is at an end of the range.  Each candidate gets
    the sim step's pilot with the same seeds (paired), and the lowest mean
    wins.  The runs go into the runs file; those at the chosen width count
    as sim runs.
    """
    solvers = [s for s in args.solvers if SOLVERS[s].widths == TUNE]
    tables = {s: Csv(os.path.join(DATA, s, 'runs', args.runs_file), SIM_HEADER)
              for s in solvers}
    sizes = tune_sizes(args.sizes)
    cands, cap = {}, {}
    for s in solvers:
        for n in sizes:
            g = earlier_width(SOLVERS[s].guess, n)
            ws = range(1, 5) if n <= 64 else range(g - 1, g + 2)
            cap[s, n] = min(dims(n)[1], g + 3, MAX_TUNE_W)
            cands[s, n] = {w for w in ws if 1 <= w <= dims(n)[1]}

    def rows(s, n, w):
        return [r for r in tables[s].rows() if int(r['n']) == n and int(r['w']) == w]

    def means(s, n):
        m = {}
        for w in cands[s, n]:
            rs = rows(s, n, w)
            if sum(int(r['count']) for r in rs) >= 2:
                m[w] = pooled(rs)[0]
        return m

    while True:
        work = []
        for (s, n), ws in cands.items():
            for w in ws:
                used = {int(r['seed']) for r in rows(s, n, w)}
                if n <= FULL_MAX:
                    if not used:
                        work.append((s, n, 0, kmin(n), w))
                else:
                    work += [(s, n, i, 1, w) for i in range(kmin(n)) if seed(n, i) not in used]
        work.sort(key=lambda t: -t[1] * t[1] * t[3])     # longest first
        parallel(args.jobs, lambda t: safe_sim_run(t[0], tables[t[0]], *t[1:]), work,
                 mem=lambda t: mem_gb(t[0], t[1], t[4]), mem_limit=args.mem_gb)
        grew = False
        for (s, n), ws in cands.items():
            m = means(s, n)
            if len(m) < len(ws):
                continue                    # a pilot failed; rerun the step
            best = min(m, key=m.get)
            if best == max(ws) and best < cap[s, n]:
                ws.add(best + 1)
                grew = True
            if best == min(ws) and best > 1:
                ws.add(best - 1)
                grew = True
        if not grew:
            break

    for s in solvers:
        path = os.path.join(DATA, s, 'widths.csv')
        tab = {int(r['n']): r for r in csv.DictReader(open(path))} if os.path.exists(path) else {}
        for n in sizes:
            m = means(s, n)
            if len(m) == len(cands[s, n]):
                best = min(m, key=m.get)
                tab[n] = {'n': n, 'w': best, 'mean': '%.12g' % m[best],
                          'candidates': ' '.join('%d:%.10g' % (w, m[w]) for w in sorted(m))}
        with open(path, 'w', newline='') as f:
            w = csv.DictWriter(f, ['n', 'w', 'mean', 'candidates'])
            w.writeheader()
            for n in sorted(tab):
                w.writerow(tab[n])
        _widths.pop(s, None)
        print('%s: widths for %d sizes' % (s, len(tab)))


def do_replay(args):
    """Rerun every row of the runs file whose output is not in the .out file
    yet, check it reproduces the stored mean and sd exactly, and save its
    output.  (Runs reproduce only on the same platform: std::shuffle
    differs between C++ libraries.)"""
    work, bad = [], []
    for s in args.solvers:
        path = os.path.join(DATA, s, 'runs', args.runs_file)
        if not os.path.exists(path):
            continue
        table = Csv(path, SIM_HEADER)
        done = table.logged()
        for r in table.rows():
            if int(r['n']) not in args.sizes:
                continue
            cmd = sim_cmd(s, r['n'], r['w'], r['p'], r['seed'], r['count'])
            if tuple(cmd[1:]) not in done:
                work.append((s, table, r, cmd))

    def one(t):
        s, table, r, cmd = t
        n, count = int(r['n']), int(r['count'])
        try:
            mean, sd, out = run_parse(cmd, n, count)
        except Exception as e:
            print('FAILED replay %s n=%d seed=%s: %s' % (s, n, r['seed'], e), flush=True)
            return
        same = mean == float(r['mean']) and sd == float(r['sd'])
        table.log(out if same else out + '!!! does not reproduce the stored row: mean %s sd %s\n'
                  % (r['mean'], r['sd']))
        if not same:
            bad.append(t)
        print('replay %s n=%d w=%s seed=%s count=%d %s' % (
            s, n, r['w'], r['seed'], count,
            'same' if same else 'DIFFERENT %r vs %s' % (mean, r['mean'])), flush=True)

    work.sort(key=lambda t: -int(t[2]['n']) ** 2 * int(t[2]['count']))
    parallel(args.jobs, one, work, mem=lambda t: mem_gb(t[0], int(t[2]['n']), int(t[2]['w'])),
             mem_limit=args.mem_gb)
    print('replay: %d runs, %d differ' % (len(work), len(bad)), flush=True)


def do_plan(args):
    """brobdicube-based solvers: the planner's average (model) and worst
    case for every model/table/discrete size, as run_brobdicube.py does
    (-p max -P list), with the full output in runs/<kind>.out."""
    sizes = sorted(set(MODEL_N) | set(DISCRETE_N) | set(TABLE_N))
    for s in [s for s in args.solvers if SOLVERS[s].brob]:
        sv = SOLVERS[s]
        runs = os.path.join(DATA, s, 'runs')
        os.makedirs(runs, exist_ok=True)
        for kind, flag in (('model', []), ('worst', ['-w'])):
            cmd = [sv.prog] + flag + sv.flags + ['-t', str(args.jobs), '-p', 'max', '-P', 'list']
            r = subprocess.run(cmd, input='\n'.join(map(str, sizes)) + '\n',
                               capture_output=True, text=True)
            if r.returncode != 0:
                print('FAILED %s %s: %s' % (s, kind, (r.stdout + r.stderr)[-500:]), flush=True)
                continue
            with open(os.path.join(runs, kind + '.out'), 'w') as f:
                f.write(r.stdout + r.stderr)
            with open(os.path.join(DATA, s, kind + '.csv'), 'w', newline='') as f:
                w = csv.writer(f)
                w.writerow(['n', 'moves', 'algs', 'widths', 'exact'])
                for block in re.split(r'^(?=N=\d+ Estimated)', r.stdout, flags=re.M)[1:]:
                    n, moves = re.match(r'N=(\d+) Estimated \S+ cost is (\S+)', block).groups()
                    algs = re.findall(r'^Use (\S+) (\S+) \[(\w+)\]', block, re.M)
                    widths = re.findall(r'^Widths (.*)$', block, re.M)
                    # -w also prints the worst case as an exact integer
                    exact = re.findall(r'^Exact worst-case cost is (\d+)$', block, re.M)
                    w.writerow([n, moves, ';'.join('%s:%s:%s' % a for a in algs), ';'.join(widths),
                                exact[0] if exact else ""])
            print('%s %s: %d sizes' % (s, kind, len(sizes)), flush=True)


def do_summary(args):
    for s in args.solvers:
        paths = sorted(glob.glob(os.path.join(DATA, s, 'runs', 'sim*.csv')))
        if not paths:
            continue
        byn = {}
        for p in paths:
            for r in Csv(p, SIM_HEADER).rows():
                if SOLVERS[s].widths == TUNE and int(r['w']) != width(s, int(r['n'])):
                    continue                # a tune run at a width not chosen
                byn.setdefault(int(r['n']), []).append(r)
        with open(os.path.join(DATA, s, 'sim.csv'), 'w', newline='') as f:
            w = csv.writer(f)
            w.writerow(['n', 'moves', 'stderr', 'samples', 'width', 'strip_rows', 'method'])
            for n in sorted(byn):
                mean, sd, N = pooled(byn[n])
                se = sd / math.sqrt(N) if N > 1 else float('nan')
                w.writerow([n, '%.12g' % mean, '%.4g' % se, N, byn[n][0]['w'],
                            byn[n][0]['p'], 'full' if n <= FULL_MAX else 'strip'])
        print('%s: %d sizes' % (s, len(byn)))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--solvers', default=DEFAULT_SOLVERS,
                    type=lambda v: [s for x in v.split(',') for s in GROUPS.get(x, [x])])
    ap.add_argument('--sizes', type=lambda v: [int(x) for x in v.split(',')],
                    default=sorted(set(SIM_N) | set(TABLE_N)))
    ap.add_argument('--max-n', type=int, default=0, help='skip sizes above this')
    ap.add_argument('--jobs', type=int, default=max(1, (os.cpu_count() or 2) - 2))
    ap.add_argument('--mem-gb', type=float, default=40.0)
    ap.add_argument('--runs-file', default='sim.csv')
    ap.add_argument('steps', nargs='*', default=['sim', 'summary'])
    args = ap.parse_args()
    bad = [s for s in args.solvers if s not in SOLVERS]
    if bad:
        ap.error('unknown solver(s): %s' % ','.join(bad))
    if args.max_n:
        args.sizes = [n for n in args.sizes if n <= args.max_n]
    t0 = time.time()
    for step in args.steps:
        if step == 'tune':
            do_tune(args)
        elif step == 'sim':
            do_sim(args)
        elif step == 'replay':
            do_replay(args)
        elif step == 'plan':
            do_plan(args)
        elif step == 'summary':
            do_summary(args)
        else:
            sys.exit('unknown step ' + step)
    print('done in %.0f s' % (time.time() - t0))


if __name__ == '__main__':
    main()
