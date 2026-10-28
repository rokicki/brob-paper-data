#!/usr/bin/env python3
"""
Convert the previously collected raw data (olddata/, see olddata/README.md)
into data/legacy/<solver>_<kind>.csv (columns n,moves; centers only), the
format used for every solver in this repository.

These are the numbers behind the published graph and tables.  They stand in
for the solvers this repository does not yet regenerate; brob4 and just62
are kept for comparison with the new brobdicube data.

Usage: import_olddata.py OLDDATA_DIR OUT_DIR
"""
import csv
import os
import sys

# old file -> (solver, kind)
MAP = {
    'arena/rcube1d.txt': ('rcube1d', 'sim'),
    'arena/rcube2d.txt': ('rcube2d', 'sim'),
    'arena/sort96.txt': ('sortnet96', 'sim'),
    'arena/permnet.txt': ('permnet72', 'sim'),
    'arena/just62.txt': ('permnet62', 'sim'),
    'arena/brob4.txt': ('brob4', 'sim'),
    'bounds/sp1dwc.txt': ('rcube1d', 'upper'),
    'bounds/sp2dwc.txt': ('rcube2d', 'upper'),
    'bounds/sort96wc.txt': ('sortnet96', 'upper'),
    'bounds/permwc.txt': ('permnet72', 'upper'),
    'bounds/just62wc.txt': ('permnet62', 'upper'),
    'bounds/brob4wc.txt': ('brob4', 'upper'),
    'bounds/orbwc.txt': ('0D', 'upper'),
    'bounds/lb.txt': ('lower', 'bound'),
}


def read(path, col):
    """{n: moves}; moves may be written like 1.2e+17 or as an integer."""
    d = {}
    for line in open(path):
        f = line.split()
        if len(f) > col:
            d[int(f[0])] = float(f[col])
    return d


def main():
    src, out = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    for rel, (solver, kind) in MAP.items():
        col = 3 if rel.startswith('arena/') else 1   # arena: n w bestw moves
        d = read(os.path.join(src, rel), col)
        with open(os.path.join(out, '%s_%s.csv' % (solver, kind)), 'w', newline='') as f:
            w = csv.writer(f)
            w.writerow(['n', 'moves'])
            for n in sorted(d):
                w.writerow([n, repr(d[n])])


if __name__ == '__main__':
    main()
