#!/usr/bin/env python3
"""
Worst-case center move bounds for brob2's six fixed networks (permnet62 ..
permnet67): an outer 24/12/12 network of 26 or 27 stages, then an inner
12/4/4/4 network of 18, 19 or 20 stages applied to each half.

The bound depends only on the algorithm sets.  Every block of a stage uses
the same algorithm.  The algorithm has n2 row (depth 2) and n3 column
(depth 3) slice moves, each repeated once per row or column of the block.
The outer turns before the first slice move (the lead) merge with the outer
turns after the last one (the trail) of the block before.  So a block that
follows another block of its stage costs

    c + n2 |rows| + n3 |columns|,

where c is the trail, plus the reduced interior outer turns, plus what the
lead adds after cancelling against the trail.  The first block of a stage
can follow a different algorithm, and costs at most f more (its whole lead).

A strip of width w over m columns has at most k = min(2^w - 1, m) blocks,
one per distinct nonzero row pattern.  Every column is in at most one block.
The bound takes the k patterns with the most rows, and charges every stage
its worst case at once.  Summed over the stages, the strip cost is

    m * N3 + k * C + N2 * (rows in the k patterns),

plus F once.  N3, N2, C and F are the sums of n3, n2, c and f over all
stages (the inner network counted twice).  brob2 -d -W prints the
per-algorithm values these sums come from.  Every stage is used by some
orbit state's plan in both planning modes, so the bound is the same for
greedy and -m.

Rows are split into strips of widths w and w+1 (the best mix), as for the
other fixed networks in bounds.py.  brob2 itself uses one width plus a
narrower last strip; uniform=True gives that bound, which brob2 -W also
prints.

Usage: permnet_bounds.py [n ...]
"""
import sys
from math import comb

# name: (outer stages, inner stages, N3, N2, C, F)
NETWORKS = {
    'permnet62': (26, 18, 124, 176, 368, 158),
    'permnet63': (27, 18, 126, 150, 316, 77),
    'permnet64': (26, 19, 128, 156, 344, 136),
    'permnet65': (27, 19, 130, 130, 292, 55),
    'permnet66': (26, 20, 132, 160, 320, 106),
    'permnet67': (27, 20, 134, 134, 268, 25),
}


def strip(net, w, m):
    _, _, n3, n2, c, _ = NETWORKS[net]
    k = rows = 0
    for j in range(w, 0, -1):
        take = min(comb(w, j), m - k)
        k += take
        rows += take * j
        if k == m:
            break
    return n3 * m + k * c + n2 * rows


def bound(net, n, uniform=False):
    """Bound and the strip widths used: (moves, [(count, width), ...])."""
    my = (n - 2) // 2
    mx = (n - 2) - my
    f = NETWORKS[net][5]
    best = None
    for w in range(1, min(40, my) + 1):
        if uniform:
            cand = [[(my // w, w), (1, my % w)] if my % w else [(my // w, w)]]
        else:
            # The count of (w+1)-strips must be congruent to my mod w; the
            # cost is linear in it, so only the extremes matter.
            lo, hi = my % w, my // (w + 1)
            hi -= (hi - lo) % w
            cand = [[((my - t * (w + 1)) // w, w), (t, w + 1)]
                    for t in {lo, hi} if lo <= t and t * (w + 1) <= my]
        for split in cand:
            t = f + sum(cnt * strip(net, ww, mx) for cnt, ww in split if cnt)
            if best is None or t < best[0]:
                best = (t, [s for s in split if s[0]])
    return best


def main():
    ns = [int(a) for a in sys.argv[1:]] or [4, 8, 16, 64, 1024, 65536, 2 ** 30]
    for n in ns:
        print(n, ' '.join('%s=%d' % (net, bound(net, n)[0]) for net in NETWORKS))


if __name__ == '__main__':
    main()
