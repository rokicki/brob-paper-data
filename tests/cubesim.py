#!/usr/bin/env python3
"""
Independent NxNxN center-sticker simulator, for checking solve96.

  cubesim.py calib N SEQ      print the centers after SEQ from solved (net layout)
  cubesim.py replay N SCR < sol  apply the scramble in SCR, then the solution
  cubesim.py check N < log    read solve96 -s -L output: load the scramble,
                              replay the M lines, report whether centers solved

Stickers are (position, normal) in 3D with coordinates -(N-1)..N-1 step 2;
the move kX (twist t) turns the layer at distance k-1 from face X clockwise
(seen from X) t quarter turns.  Only center stickers are tracked.
"""
import sys

FACES = 'ULFRBD'
NORM = {'U': (0, 1, 0), 'L': (-1, 0, 0), 'F': (0, 0, 1),
        'R': (1, 0, 0), 'B': (0, 0, -1), 'D': (0, -1, 0)}


def pos3(f, r, c, N):
    """net (face, row, col) -> 3D sticker position (on the cubie)."""
    m = N - 1
    a, b = -m + 2 * c, m - 2 * r
    return {'U': (a, m, -b), 'F': (a, b, m), 'R': (m, b, -a),
            'B': (-a, b, -m), 'L': (-m, b, a), 'D': (a, -m, b)}[f]


def dot(u, v):
    return sum(x * y for x, y in zip(u, v))


def cross(u, v):
    return (u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0])


def rot(v, n):
    """rotate v by -90 degrees about unit axis n (clockwise seen from n)."""
    d = dot(v, n)
    c = cross(n, v)
    return tuple(d * n[i] - c[i] for i in range(3))


class Cube:
    def __init__(self, N, nets=None):
        self.N = N
        self.st = {}
        for f in FACES:
            for r in range(1, N - 1):
                for c in range(1, N - 1):
                    col = f if nets is None else nets[f][r - 1][c - 1]
                    self.st[(pos3(f, r, c, N), NORM[f])] = col

    def move(self, dep, face, twist):
        n = NORM[face]
        layer = (self.N - 1) - 2 * (dep - 1)
        for _ in range(twist % 4):
            new = {}
            for (p, nn), col in self.st.items():
                if dot(p, n) == layer:
                    new[(rot(p, n), rot(nn, n))] = col
                else:
                    new[(p, nn)] = col
            self.st = new

    def nets(self):
        out = {}
        for f in FACES:
            out[f] = [''.join(str(self.st[(pos3(f, r, c, self.N), NORM[f])])
                              for c in range(1, self.N - 1)) for r in range(1, self.N - 1)]
        return out

    def solved(self):
        return all(len(set(row for row in ''.join(rows))) == 1 for rows in self.nets().values())


def parse_moves(s):
    out = []
    for tok in s.split():
        i = 0
        while tok[i].isdigit():
            i += 1
        dep = int(tok[:i]) if i else 1
        face = tok[i]
        rest = tok[i + 1:]
        twist = 3 if rest == "'" else int(rest) if rest else 1
        out.append((dep, face, twist))
    return out


def parse_show(lines, N):
    """the solve96 show() layout: U rows, blank, L F R B rows, blank, D rows."""
    k = N - 2
    toks = [ln.split() for ln in lines if ln.strip()]
    assert len(toks) == 3 * k, (len(toks), k)
    nets = {'U': [t[0] for t in toks[:k]], 'D': [t[0] for t in toks[2 * k:]]}
    for i, f in enumerate('LFRB'):
        nets[f] = [t[i] for t in toks[k:2 * k]]
    return nets


def main():
    if sys.argv[1] == 'calib':
        N = int(sys.argv[2])
        cube = Cube(N, {f: [str(FACES.index(f)) * (N - 2)] * (N - 2) for f in FACES})
        for m in parse_moves(sys.argv[3]):
            cube.move(*m)
        nets = cube.nets()
        print('\n'.join(nets['U']))
        for r in range(N - 2):
            print(' '.join(nets[f][r] for f in 'LFRB'))
        print('\n'.join(nets['D']))
        return
    if sys.argv[1] == 'replay':
        # cubesim.py replay N SCRAMBLEFILE < solution: solved after scramble+solution?
        N = int(sys.argv[2])
        cube = Cube(N)
        for m in parse_moves(open(sys.argv[3]).read()):
            cube.move(*m)
        if cube.solved():
            sys.exit('scramble does not scramble the centers')
        sol = parse_moves(sys.stdin.read())
        for m in sol:
            cube.move(*m)
        print('N=%d %d solution moves: %s' % (N, len(sol), 'SOLVED' if cube.solved() else 'NOT SOLVED'))
        sys.exit(0 if cube.solved() else 1)
    N = int(sys.argv[2])
    lines = sys.stdin.read().split('\n')
    i = lines.index('SCRAMBLE') + 1
    j = i
    while not lines[j].startswith('M '):
        j += 1
    cube = Cube(N, parse_show(lines[i:j], N))
    if cube.solved():
        sys.exit('scramble is already solved?')
    nm = 0
    for ln in lines[j:]:
        if ln.startswith('M '):
            _, d, f, t = ln.split()
            cube.move(int(d), f, int(t))
            nm += 1
    print('N=%d replayed %d moves: %s' % (N, nm, 'SOLVED' if cube.solved() else 'NOT SOLVED'))
    sys.exit(0 if cube.solved() else 1)


if __name__ == '__main__':
    main()
