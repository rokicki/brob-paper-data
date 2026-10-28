#!/usr/bin/env python3
"""
Generate all brobdicube data for the paper (centers only).

  model   brobdicube -P n n -p My        expected center moves from the exact
                                         per-stage probability model, planned
                                         for the whole cube (one strip of My
                                         rows), for every n in MODEL_N.
  worst   brobdicube -w -P n n -p My     worst-case upper bound, same sizes
                                         (plus the strip-width multisets).
  worst-p1 brobdicube -w -P n n -p 1     the same with one-row strips (no 2D
                                         rectangles, linear combinations
                                         only); plans only, for the
                                         decomposition tables.
  model-p1 brobdicube -P n n -p 1        model with one-row strips, likewise.
  sim     brobdicube -x -M K -z SEED     measured center moves of random
                                         cubes, for every n in SIM_N:
            n <= FULL_MAX: whole cubes (-p My), K solves per run;
            larger n:      the full-cube plan is used and a sample of its
                           width blocks is solved inside one strip of
                           -p rows, scaled up per stage (see brobdicube -x).
          Sample counts are chosen from a pilot run so that the standard
          error of the mean is at most TARGET_RSE of the mean.

Every run is seeded (-z), so the whole data set is reproducible.  Raw runs
are appended to data/brobdicube/runs/*.csv; the step is resumable, rerunning
only what is missing.  Summaries go to data/brobdicube/{model,worst,sim}.csv.

Usage: run_brobdicube.py [--brob PATH] [--jobs J] [--mem-gb G] [model|worst|sim|summary ...]
"""
import argparse
import csv
import glob
import math
import os
import re
import subprocess
import sys
import threading
import time
from concurrent.futures import ThreadPoolExecutor

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sizes import MODEL_N, SIM_N, DISCRETE_N   # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, 'data', 'brobdicube')
RUNS = os.path.join(OUT, 'runs')

FULL_MAX = 4096            # solve whole cubes up to here, strips above
STRIP_ORBITS = 1.5e8       # target orbits per sampled strip (memory/time)
TARGET_RSE = 5e-4          # standard error <= 0.05% of the mean
ONE_SAMPLE_N = 1 << 20     # above this, one sample per size, no top-up (user, 2026-10-10)


def kmin(n, args=None):
    if n > ONE_SAMPLE_N:
        return 1        # one strip run samples ~n/2 orbits; repeats agree to ~1e-6
    if args is not None and args.min_samples:
        return args.min_samples
    if n <= 256:
        return 32
    if n <= 1024:
        return 8
    if n <= FULL_MAX:
        return 4
    return 2


def dims(n):
    my = (n - 2) // 2
    return n - 2 - my, my


def strip_height(n):
    mx, my = dims(n)
    if n <= FULL_MAX:
        return my
    return min(my, max(64, min(1024, int(STRIP_ORBITS / mx))))


def mem_gb(n):
    mx, _ = dims(n)
    return 1.8 + 36e-9 * mx * strip_height(n)


def seed(n, run):
    return n * 1000 + run


# ------------------------------------------------------------------ helpers

class Csv:
    """Append-only CSV with a header, safe across threads."""

    def __init__(self, path, header):
        self.path, self.header = path, header
        self.lock = threading.Lock()
        if not os.path.exists(path):
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, 'w', newline='') as f:
                csv.writer(f).writerow(header)

    def rows(self):
        with open(self.path, newline='') as f:
            return list(csv.DictReader(f))

    def add(self, row, output=None):
        """Append a row and, if given, the run's full output (which starts
        with the solver's command line) to the .out file beside the CSV."""
        with self.lock:
            with open(self.path, 'a', newline='') as f:
                csv.writer(f).writerow([row[h] for h in self.header])
            if output is not None:
                self._log(output)

    def outpath(self):
        return os.path.splitext(self.path)[0] + '.out'

    def _log(self, output):
        with open(self.outpath(), 'a') as f:
            f.write(output if output.endswith('\n') else output + '\n')

    def log(self, output):
        with self.lock:
            self._log(output)

    def logged(self):
        """Command lines (as argument tuples, program path dropped) of the
        runs already in the .out file."""
        if not os.path.exists(self.outpath()):
            return set()
        return {tuple(line.split()[2:]) for line in open(self.outpath())
                if line.startswith('- ')}


def brob(args, cmd):
    r = subprocess.run([args.brob] + cmd, capture_output=True, text=True,
                       cwd=os.path.dirname(args.brob))
    if r.returncode != 0:
        raise RuntimeError('brobdicube %s failed:\n%s%s' % (' '.join(cmd), r.stdout, r.stderr))
    return r.stdout + r.stderr


def parallel(jobs, fn, items, mem=None, mem_limit=None):
    """Run fn over items with at most `jobs` at once and, if mem is given,
    at most mem_limit GB of estimated memory in use."""
    cv = threading.Condition()
    in_use = [0.0]

    def run(item):
        need = mem(item) if mem else 0
        with cv:
            cv.wait_for(lambda: in_use[0] == 0 or in_use[0] + need <= mem_limit)
            in_use[0] += need
        try:
            return fn(item)
        finally:
            with cv:
                in_use[0] -= need
                cv.notify_all()

    with ThreadPoolExecutor(jobs) as ex:
        for f in [ex.submit(run, it) for it in items]:
            f.result()


# ------------------------------------------------------------------- models

def do_model(args, kind):
    """One brobdicube process plans every size (brobdicube parallelizes
    the sizes itself); -p max plans each cube as one strip of all rows."""
    worst = kind.startswith('worst')
    flag = ['-w'] if worst else []
    sizes = sorted(set(MODEL_N) | (set(DISCRETE_N) if worst else set()))
    p = '1' if kind.endswith('-p1') else 'max'
    r = subprocess.run([args.brob] + flag + ['-t', str(args.jobs), '-p', p, '-P', 'list'],
                       input='\n'.join(map(str, sizes)) + '\n', capture_output=True,
                       text=True, cwd=os.path.dirname(args.brob))
    if r.returncode != 0:
        raise RuntimeError('brobdicube failed:\n%s%s' % (r.stdout, r.stderr))
    path = os.path.join(RUNS, kind + '.csv')
    os.makedirs(RUNS, exist_ok=True)
    with open(os.path.join(RUNS, kind + '.out'), 'w') as f:
        f.write(r.stdout + r.stderr)
    with open(path, 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['n', 'moves', 'algs', 'widths', 'exact'])
        # output blocks: "N=n Estimated ... cost is X", then Use/Widths lines
        for block in re.split(r'^(?=N=\d+ Estimated)', r.stdout, flags=re.M)[1:]:
            n, moves = re.match(r'N=(\d+) Estimated \S+ cost is (\S+)', block).groups()
            algs = re.findall(r'^Use (\S+) (\S+) \[(\w+)\]', block, re.M)
            widths = re.findall(r'^Widths (.*)$', block, re.M)
            # -w also prints the worst case as an exact integer
            exact = re.findall(r'^Exact worst-case cost is (\d+)$', block, re.M)
            w.writerow([n, moves, ';'.join('%s:%s:%s' % a for a in algs), ';'.join(widths),
                        exact[0] if exact else ""])
    print('%s: %d sizes' % (kind, len(sizes)), flush=True)


# -------------------------------------------------------------- simulations

SIM_HEADER = ['n', 'p', 'seed', 'count', 'mean', 'sd']


def sim_cmd(n, p, sd, count):
    return ['-t', '1', '-x', '-p', str(p), '-z', str(sd), '-M', str(count), str(n)]


def sim_parse(out, n, count):
    if count == 1:
        m = re.search(r'^N %d width \d+ moves (\d+)' % n, out, re.M)
        return float(m.group(1)), 0.0
    m = re.search(r'^N %d phasewidth \d+ cnt %d moves (\S+) stddev (\S+)' % (n, count), out, re.M)
    return float(m.group(1)), float(m.group(2))


def sim_run(args, table, n, run, count):
    p = strip_height(n)
    out = brob(args, sim_cmd(n, p, seed(n, run), count))
    mean, sd = sim_parse(out, n, count)
    table.add({'n': n, 'p': p, 'seed': seed(n, run), 'count': count,
               'mean': repr(mean), 'sd': repr(sd)}, out)
    print('sim n=%d p=%d run=%d count=%d mean=%.10g' % (n, p, run, count, mean), flush=True)


def safe_sim_run(args, table, n, run, count):
    # Report a failed run and carry on; rerunning the step retries it.
    try:
        sim_run(args, table, n, run, count)
    except Exception as e:
        print('FAILED sim n=%d run=%d: %s' % (n, run, e), flush=True)


def pooled(rows):
    """Mean, sample standard deviation and count pooled over runs.  Each run
    reports its mean and population standard deviation over `count` solves;
    strip runs have count 1."""
    N = sum(int(r['count']) for r in rows)
    sx = sum(int(r['count']) * float(r['mean']) for r in rows)
    sxx = sum(int(r['count']) * (float(r['sd']) ** 2 + float(r['mean']) ** 2) for r in rows)
    mean = sx / N
    var = max(0.0, (sxx - N * mean * mean) / (N - 1)) if N > 1 else float('nan')
    return mean, math.sqrt(var), N


def do_sim(args):
    table = Csv(os.path.join(RUNS, args.runs_file), SIM_HEADER)

    def runs_for(n):
        return [r for r in table.rows() if int(r['n']) == n]

    # Pass 1: pilot runs.  Whole cubes: one process doing kmin solves.
    # Strips: kmin processes of one solve each (they parallelize).
    sizes = args.sizes or SIM_N
    pilots = []
    for n in sizes:
        have = runs_for(n)
        if n <= FULL_MAX:
            if not have:
                pilots.append((n, 0, kmin(n, args)))
        else:
            used = {int(r['seed']) for r in have}
            pilots += [(n, i, 1) for i in range(kmin(n, args)) if seed(n, i) not in used]
    # biggest first so the long runs do not trail at the end
    pilots.sort(key=lambda t: -t[0])
    parallel(args.jobs, lambda t: safe_sim_run(args, table, *t), pilots,
             mem=lambda t: mem_gb(t[0]), mem_limit=args.mem_gb)

    # Pass 2: top up to the target relative standard error.
    extra = []
    for n in sizes:
        rows = runs_for(n)
        if n > ONE_SAMPLE_N:
            continue
        if len(rows) == 0 or sum(int(r['count']) for r in rows) < 2:
            continue                # a pilot failed; rerun the step
        mean, sd, N = pooled(rows)
        need = math.ceil((sd / (TARGET_RSE * mean)) ** 2) if mean > 0 else 0
        if need > N:
            nextrun = 1 + max(int(r['seed']) - n * 1000 for r in rows)
            if n <= FULL_MAX:
                extra.append((n, nextrun, need - N))
            else:
                extra += [(n, nextrun + i, 1) for i in range(need - N)]
    extra.sort(key=lambda t: -t[0] * t[2])
    parallel(args.jobs, lambda t: safe_sim_run(args, table, *t), extra,
             mem=lambda t: mem_gb(t[0]), mem_limit=args.mem_gb)


def do_replay(args):
    """Rerun every row of the runs file whose output is not in the .out file
    yet, check it reproduces the stored mean and sd exactly, and save its
    output.  (Runs reproduce only on the same platform: std::shuffle
    differs between C++ libraries.)"""
    table = Csv(os.path.join(RUNS, args.runs_file), SIM_HEADER)
    done = table.logged()
    todo = [r for r in table.rows()
            if tuple(sim_cmd(r['n'], r['p'], r['seed'], r['count'])) not in done]
    if args.sizes:
        todo = [r for r in todo if int(r['n']) in args.sizes]
    bad = []

    def one(r):
        n, count = int(r['n']), int(r['count'])
        try:
            out = brob(args, sim_cmd(n, r['p'], r['seed'], count))
            mean, sd = sim_parse(out, n, count)
        except Exception as e:
            print('FAILED replay n=%d seed=%s: %s' % (n, r['seed'], e), flush=True)
            return
        same = mean == float(r['mean']) and sd == float(r['sd'])
        table.log(out if same else out + '!!! does not reproduce the stored row: mean %s sd %s\n'
                  % (r['mean'], r['sd']))
        if not same:
            bad.append(r)
        print('replay n=%d seed=%s count=%d %s' % (n, r['seed'], count,
              'same' if same else 'DIFFERENT %r vs %s' % (mean, r['mean'])), flush=True)

    todo.sort(key=lambda r: -int(r['n']) * int(r['n']) * int(r['count']))
    parallel(args.jobs, one, todo, mem=lambda r: mem_gb(int(r['n'])), mem_limit=args.mem_gb)
    print('replay: %d runs, %d differ' % (len(todo), len(bad)), flush=True)


# ------------------------------------------------------------------ summary

def do_summary(args):
    for kind in ('model', 'worst'):
        path = os.path.join(RUNS, kind + '.csv')
        if not os.path.exists(path):
            continue
        rows = {int(r['n']): r for r in Csv(path, []).rows()}
        with open(os.path.join(OUT, kind + '.csv'), 'w', newline='') as f:
            w = csv.writer(f)
            w.writerow(['n', 'moves', 'algs', 'widths', 'exact'])
            for n in sorted(rows):
                r = rows[n]
                w.writerow([n, r['moves'], r['algs'], r['widths'], r.get('exact') or ''])
    # runs/sim.csv holds the local runs; runs/sim-<host>.csv runs made
    # elsewhere (e.g. the largest sizes on a big-memory machine).
    paths = sorted(glob.glob(os.path.join(RUNS, 'sim*.csv')))
    if paths:
        allrows = [r for p in paths for r in Csv(p, SIM_HEADER).rows()]
        byn = {}
        for r in allrows:
            byn.setdefault(int(r['n']), []).append(r)
        with open(os.path.join(OUT, 'sim.csv'), 'w', newline='') as f:
            w = csv.writer(f)
            w.writerow(['n', 'moves', 'stderr', 'samples', 'strip_rows', 'method'])
            for n in sorted(byn):
                mean, sd, N = pooled(byn[n])
                se = sd / math.sqrt(N) if N > 1 else float('nan')
                w.writerow([n, '%.10g' % mean, '%.4g' % se, N, byn[n][0]['p'],
                            'full' if n <= FULL_MAX else 'strip'])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--brob', default=os.path.join(ROOT, 'brobdicube', 'brobdicube'))
    ap.add_argument('--jobs', type=int, default=max(1, (os.cpu_count() or 2) - 2))
    ap.add_argument('--mem-gb', type=float, default=40.0)
    ap.add_argument('--sizes', type=lambda v: [int(x) for x in v.split(',')],
                    help='comma-separated sizes to simulate instead of SIM_N')
    ap.add_argument('--min-samples', type=int, default=0,
                    help='samples per size for the pilot pass (default: by size)')
    ap.add_argument('--runs-file', default='sim.csv',
                    help='file in data/brobdicube/runs/ for the raw simulation runs')
    ap.add_argument('steps', nargs='*', default=['model', 'worst', 'model-p1', 'worst-p1', 'sim', 'summary'])
    args = ap.parse_args()
    args.brob = os.path.abspath(args.brob)
    t0 = time.time()
    for step in args.steps:
        if step in ('model', 'worst', 'model-p1', 'worst-p1'):
            do_model(args, step)
        elif step == 'sim':
            do_sim(args)
        elif step == 'replay':
            do_replay(args)
        elif step == 'summary':
            do_summary(args)
        else:
            sys.exit('unknown step ' + step)
    print('done in %.0f s' % (time.time() - t0))


if __name__ == '__main__':
    main()
