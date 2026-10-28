#ifndef WRITEFRAMES
#define WRITEFRAMES (0)
#endif
#include <algorithm>
#include <assert.h>
#include <strings.h>
#include <iostream>
#include <random>
#include <vector>
#if WRITEFRAMES
#include "fpng.h"
#endif
using namespace std ;
using ll = long long ;
using lll = __int128_t ;
using uchar = unsigned char ;
int showdetails = 0 ;
int minstages = 0 ; // -m: plan each state with the fewest stages
int alwaysinmem = 0 ;
ll writemoveinc ;
ll writemovetarget = 0 ;
ll runningmovecount = 0 ;
void error(const char *s) {
   cerr << s << endl ;
   exit(10) ;
}
ostream& operator<<(ostream& os, __int128_t n) {
    if (n == 0) return os << "0";
    if (n < 0) {
        os << "-";
        n = -n;
    }
    string s;
    while (n > 0) {
        s += (char)('0' + (n % 10));
        n /= 10;
    }
    reverse(s.begin(), s.end());
    return os << s;
}
/*
 *   These are the 24 rotations, given as face mappings.  We only give the
 *   first three and build the rest.
 */
int rots[24][6] = {
   {0, 1, 2, 3, 4, 5}, {0, 2, 3, 4, 1, 5}, {2, 1, 5, 3, 0, 4},
} ;
/*
 *   Coordinate system is as follows:
 *
 *   Normal cartesian coordinates:  0  1..M    M+1..2M  2M+1
 *
 *   0 and 2M+1 are edges, so we don't deal with them here.
 */
void speffztocoord(int Mx, int My, int ox, int oy, int oz, int &x, int &y) {
   assert(ox >= 1) ;
   assert(oy >= 1) ;
   assert(ox <= Mx) ;
   assert(oy <= My) ;
   assert(oz >= 0) ;
   assert(oz <= 3) ;
   switch (oz) {
case 0:
      x = ox ;
      y = oy ;
      break ;
case 1:
      x = Mx + My + 1 - oy ;
      y = ox ;
      break ;
case 2:
      x = Mx + My + 1 - ox ;
      y = Mx + My + 1 - oy ;
      break ;
case 3:
      x = oy ;
      y = Mx + My + 1 - ox ;
      break ;
default:
      error("! failed; bad sz value") ;
   }
}
void coordtospeffz(int Mx, int My, int x, int y, int &ox, int &oy, int &oz) {
   assert(x >= 1) ;
   assert(x <= Mx + My) ;
   assert(y >= 1) ;
   assert(y <= Mx + My) ;
   if (y <= My && x <= Mx) {
      ox = x ;
      oy = y ;
      oz = 0 ;
   } else if (y <= Mx && x > Mx) {
      oy = Mx + My + 1 - x ;
      ox = y ;
      oz = 1 ;
   } else if (y > Mx && x > My) {
      ox = Mx + My + 1 - x ;
      oy = Mx + My + 1 - y ;
      oz = 2 ;
   } else if (y > My && x <= My) {
      ox = Mx + My + 1 - y ;
      oy = x ;
      oz = 3 ;
   } else {
      ox = 1 ;
      oy = 1 ;
      oz = 4 ; // special indicator for central faces
   }
   assert(ox <= Mx) ;
   assert(oy <= My) ;
   assert(ox >= 1) ;
   assert(oy >= 1) ;
}
int fast = 1 ;
int readmoves, writemoves ;
struct xorbit {
   uchar a[24] ;
} ;
random_device rd;
mt19937_64 rng(rd()) ;
void oclear(xorbit& x) {
   for (int i=0; i<24; i++)
      x.a[i] = i >> 2 ;
}
void uclear(xorbit& x) {
   for (int i=0; i<24; i++)
      x.a[i] = i ;
}
void scramble(xorbit& x) {
   oclear(x) ;
   shuffle(x.a, x.a+24, rng) ;
}
struct mv {
   int dep ;
   short face, twist ;
} ;
vector<mv> moveset ;
struct xcube ;
void domove(mv mv, xcube &cube) ;
struct xcube {
   xcube(int Mx_, int My_, int src_) : Mx(Mx_), My(My_), rows(My), src(src_) {
      for (int i=0; i<6; i++)
         facerots[i] = 0 ;
      // central faces, only for odd cubes
      for (int i=0; i<6; i++)
         ccs[i] = i ;
      if (src_ == 'U' || alwaysinmem) {
         for (int i=0; i<My; i++) {
            rows[i].resize(Mx) ;
            if (src == 'I' || src == 'C') { // inputmoves, or clear
               for (auto &xo: rows[i])
                  oclear(xo) ;
            } else if (src == 'U') { // orbits have numbers 0..23
               for (auto &xo: rows[i])
                  uclear(xo) ;
            } else if (src == 'R') { // random
               for (auto &xo: rows[i])
                  scramble(xo) ;
            }
         }
      }
      if (src == 'I') {
         for (auto mv: moveset)
            domove(mv, *this) ;
         normalizefacerots() ;
      }
   }
   /*
    *   Outer-face turns only update facerots, but the solver chooses and
    *   applies its algorithms on the stored orbit positions; fold the
    *   rotations into the orbits so stored and physical positions agree.
    *   Without this, -i with a scramble that leaves a face turned (e.g.
    *   "U 2R") gave a wrong solution.
    */
   void normalizefacerots() {
      for (int f=0; f<6; f++) {
         if (facerots[f] == 0)
            continue ;
         for (auto &row: rows)
            for (auto &xo: row)
               rotate(xo.a + 4*f, xo.a + 4*f + facerots[f], xo.a + 4*f + 4) ;
         facerots[f] = 0 ;
      }
   }
   char get(int f, int x, int y) const {
      int ox, oy, oz ;
      coordtospeffz(Mx, My, x, y, ox, oy, oz) ;
      if (oz == 4)
         return '0'+ccs[f] ;
      if (rows[oy-1].size() == 0)
         return '?' ;
      return (char)('0' + rows[oy-1][ox-1].a[4*f+((oz+facerots[f])&3)]) ;
   }
   void rot4hor(int f1, int o1, int f2, int o2, int f3, int o3, int f4, int o4, int dep) {
      for (int i=0; i<Mx; i++) {
         uchar &c1 = rows[dep][i].a[4*f1+((o1+facerots[f1])&3)] ;
         uchar &c2 = rows[dep][i].a[4*f2+((o2+facerots[f2])&3)] ;
         uchar &c3 = rows[dep][i].a[4*f3+((o3+facerots[f3])&3)] ;
         uchar &c4 = rows[dep][i].a[4*f4+((o4+facerots[f4])&3)] ;
         auto t = c1 ; c1 = c2 ; c2 = c3 ; c3 = c4 ; c4 = t ;
      }
   }
   void rot4ver(int f1, int o1, int f2, int o2, int f3, int o3, int f4, int o4, int dep) {
      for (int i=0; i<My; i++) {
         uchar &c1 = rows[i][dep].a[4*f1+((o1+facerots[f1])&3)] ;
         uchar &c2 = rows[i][dep].a[4*f2+((o2+facerots[f2])&3)] ;
         uchar &c3 = rows[i][dep].a[4*f3+((o3+facerots[f3])&3)] ;
         uchar &c4 = rows[i][dep].a[4*f4+((o4+facerots[f4])&3)] ;
         auto t = c1 ; c1 = c2 ; c2 = c3 ; c3 = c4 ; c4 = t ;
      }
   }
   void rot4ver2(int f1, int o1, int f2, int o2, int f3, int o3, int f4, int o4, int dep) {
      rot4ver(f1, o1, f2, o2, f3, o3, f4, o4, dep) ;
      rot4ver(f1, o1^2, f2, o2^2, f3, o3^2, f4, o4^2, dep) ;
      uchar &c1 = ccs[f1] ;
      uchar &c2 = ccs[f2] ;
      uchar &c3 = ccs[f3] ;
      uchar &c4 = ccs[f4] ;
      auto t = c1 ; c1 = c2 ; c2 = c3 ; c3 = c4 ; c4 = t ;
   }
   void rot2hor(int f1, int o1, int f2, int o2, int f3, int o3, int f4, int o4, int dep) {
      for (int i=0; i<Mx; i++) {
         uchar &c1 = rows[dep][i].a[4*f1+((o1+facerots[f1])&3)] ;
         uchar &c2 = rows[dep][i].a[4*f2+((o2+facerots[f2])&3)] ;
         uchar &c3 = rows[dep][i].a[4*f3+((o3+facerots[f3])&3)] ;
         uchar &c4 = rows[dep][i].a[4*f4+((o4+facerots[f4])&3)] ;
         swap(c1, c3) ;
         swap(c2, c4) ;
      }
   }
   void rot2ver(int f1, int o1, int f2, int o2, int f3, int o3, int f4, int o4, int dep) {
      for (int i=0; i<My; i++) {
         uchar &c1 = rows[i][dep].a[4*f1+((o1+facerots[f1])&3)] ;
         uchar &c2 = rows[i][dep].a[4*f2+((o2+facerots[f2])&3)] ;
         uchar &c3 = rows[i][dep].a[4*f3+((o3+facerots[f3])&3)] ;
         uchar &c4 = rows[i][dep].a[4*f4+((o4+facerots[f4])&3)] ;
         swap(c1, c3) ;
         swap(c2, c4) ;
      }
   }
   void rot2ver2(int f1, int o1, int f2, int o2, int f3, int o3, int f4, int o4, int dep) {
      rot2ver(f1, o1, f2, o2, f3, o3, f4, o4, dep) ;
      rot2ver(f1, o1^2, f2, o2^2, f3, o3^2, f4, o4^2, dep) ;
      uchar &c1 = ccs[f1] ;
      uchar &c2 = ccs[f2] ;
      uchar &c3 = ccs[f3] ;
      uchar &c4 = ccs[f4] ;
      swap(c1, c3) ;
      swap(c2, c4) ;
   }
   void getrow(int i) {
      if (alwaysinmem)
         return ;
      if (src != 'R')
         error("! only support random right now for not in memory") ;
      rows[i].resize(Mx) ;
      for (auto &orb: rows[i])
         scramble(orb) ;
   }
   void checkrow(int i) const {
      if ((ll)rows[i].size() != Mx)
         error("! checking a nonexistent row?") ;
      for (auto &orb: rows[i])
         for (int j=0; j<24; j++)
            if (orb.a[j] != (j>>2)) {
               show() ;
               error("! cube is not solved") ;
            }
   }
   void checkrelease(int i) {
      if (alwaysinmem)
         return ;
      checkrow(i) ;
      vector<xorbit> empty ;
      swap(empty, rows[i]) ;
   }
   int Mx, My ;
   vector<vector<xorbit>> rows ;
   void show() const ;
   int facerots[6] ;
   uchar ccs[6] ;
   char src ;
} ;
void border() {
   cout << "  " ;
}
void xcube::show() const {
   if (Mx+My > 1000) {
      cout << "Not showing a cube that big" << endl ;
      return ;
   }
   for (int y=1; y<=Mx+My; y++) {
      border() ;
      for (int x=1; x<=Mx+My; x++) {
         cout << " " ;
      }
      border() ;
      for (int x=1; x<=Mx+My; x++)
         cout << get(0, x, y) ;
      cout << endl ;
   }
   cout << endl ;
   for (int y=1; y<=Mx+My; y++) {
      for (int f=1; f<=4; f++) {
         border() ;
         for (int x=1; x<=Mx+My; x++)
            cout << get(f, x, y) ;
      }
      cout << endl ;
   }
   cout << endl ;
   for (int y=1; y<=Mx+My; y++) {
      border() ;
      for (int x=1; x<=Mx+My; x++) {
         cout << " " ;
      }
      border() ;
      for (int x=1; x<=Mx+My; x++)
         cout << get(5, x, y) ;
      cout << endl ;
   }
   cout << endl ;
}
const char *faceorder = "ULFRBD" ;
mv parse_move(const char *& s) {
   mv r ;
   r.dep = 1 ;
   while (*s && *s <= ' ')
      s++ ;
   if ('0' <= *s && *s <= '9') {
      r.dep = 0 ;
      while ('0' <= *s && *s <= '9')
         r.dep = 10 * r.dep + *s++ - '0' ;
   }
   auto found = index(faceorder, *s) ;
   if (found == 0) {
      cerr << s << endl ;
      error("! could not parse move") ;
   }
   r.face = found - faceorder ;
   s++ ;
   r.twist = 1 ;
   if (*s && *s > ' ') {
      if ('1' <= *s && *s <= '3')
         r.twist = *s - '0' ;
      else if (*s == '\'')
         r.twist = 3 ;
      else {
         cerr << s << endl ;
         error("! could not parse move (2)") ;
      }
      s++ ;
   }
   while (*s && *s <= ' ')
      s++ ;
   return r ;
}
void readmoveset() {
   string s ;
   while (cin >> s) {
      const char *mstr = s.c_str() ;
      moveset.push_back(parse_move(mstr)) ;
   }
}
vector<mv> parse_moves(const char *s) {
   vector<mv> r ;
   while (*s) {
      while (*s && *s <= ' ')
         s++ ;
      if (*s == 0)
         break ;
      r.push_back(parse_move(s)) ;
   }
   return r ;
}
const int oppface[] = { 5, 3, 4, 1, 2, 0 } ;
ll cancellations = 0 ;
// we only merge face moves (conjugate moves)
int trymerge(const mv &a, const mv &b, mv &res) {
   if (a.face == b.face && a.dep == b.dep && a.dep == 1) {
      int tw = (a.twist + b.twist) & 3 ;
      if (tw == 0)
         return 0 ;
      res = a ;
      res.twist = tw ;
      return 1 ;
   } else {
      return 2 ;
   }
}
/*
 *   Push a move onto a move sequence, merging it with the last move when
 *   both turn the same outer face (a slice move never merges).  Applied
 *   move by move this leaves no two adjacent outer turns of one face, the
 *   same reduction the solution output gets; moves counts what remains.
 */
void pushreduce(vector<mv> &st, const mv &m) {
   if (st.size()) {
      mv merged ;
      int r = trymerge(st.back(), m, merged) ;
      if (r == 0) {
         st.pop_back() ;
         return ;
      }
      if (r == 1) {
         st.back() = merged ;
         return ;
      }
   }
   st.push_back(m) ;
}
void showmv(const struct mv &mv) {
   if (mv.dep > 1)
      cout << mv.dep ;
   cout << faceorder[mv.face] ;
   if (mv.twist == 2)
      cout << 2 ;
   else if (mv.twist == 3)
      cout << "'" ;
}
void domove(mv mv, xcube &cube) {
   if (mv.dep > cube.Mx + cube.My + 3 - mv.dep) {
      mv.dep = cube.Mx + cube.My + 3 - mv.dep ;
      mv.face = oppface[mv.face] ;
      mv.twist = (4 - mv.twist) & 3 ;
   }
   assert(mv.dep >= 1) ;
   assert(mv.dep <= cube.Mx + 1) ;
   if (mv.dep == 1) {
      cube.facerots[mv.face] = (cube.facerots[mv.face] + 4 - mv.twist) & 3 ;
      return ;
   }
   assert(mv.twist > 0) ;
   assert(mv.twist < 4) ;
   int dep = mv.dep - 2 ;
   /*
    *   For odd cubes, when doing the exact middle slice, things are
    *   different; this is ugly but okay.
    */
   if (dep >= cube.My) {
      switch (mv.face * 3 + mv.twist) {
case 1: // U
         cube.rot4ver2(1, 1, 2, 1, 3, 1, 4, 1, dep) ;
         break ;
case 2: // U2
         cube.rot2ver2(1, 1, 2, 1, 3, 1, 4, 1, dep) ;
         break ;
case 3: // U'
         cube.rot4ver2(4, 1, 3, 1, 2, 1, 1, 1, dep) ;
         break ;
case 4: // L
         cube.rot4ver2(2, 0, 0, 0, 4, 2, 5, 0, dep) ;
         break ;
case 5: // L2
         cube.rot2ver2(2, 0, 5, 0, 4, 2, 0, 0, dep) ;
         break ;
case 6: // L'
         cube.rot4ver2(2, 0, 5, 0, 4, 2, 0, 0, dep) ;
         break ;
case 7: // F
         cube.rot4ver2(0, 3, 1, 2, 5, 1, 3, 0, dep) ;
         break ;
case 8: // F2
         cube.rot2ver2(0, 3, 3, 0, 5, 1, 1, 2, dep) ;
         break ;
case 9: // F'
         cube.rot4ver2(0, 3, 3, 0, 5, 1, 1, 2, dep) ;
         break ;
case 10: // R
         cube.rot4ver2(2, 2, 5, 2, 4, 0, 0, 2, dep) ;
         break ;
case 11: // R2
         cube.rot2ver2(2, 2, 5, 2, 4, 0, 0, 2, dep) ;
         break ;
case 12: // R'
         cube.rot4ver2(2, 2, 0, 2, 4, 0, 5, 2, dep) ;
         break ;
case 13: // B
         cube.rot4ver2(0, 1, 3, 2, 5, 3, 1, 0, dep) ;
         break ;
case 14: // B2
         cube.rot2ver2(0, 1, 3, 2, 5, 3, 1, 0, dep) ;
         break ;
case 15: // B'
         cube.rot4ver2(0, 1, 1, 0, 5, 3, 3, 2, dep) ;
         break ;
case 16: // D
         cube.rot4ver2(4, 3, 3, 3, 2, 3, 1, 3, dep) ;
         break ;
case 17: // D2
         cube.rot2ver2(1, 3, 2, 3, 3, 3, 4, 3, dep) ;
         break ;
case 18: // D'
         cube.rot4ver2(4, 3, 1, 3, 2, 3, 3, 3, dep) ;
         break ;
      }
   } else {
      switch (mv.face * 3 + mv.twist) {
case 1: // U
         cube.rot4hor(1, 0, 2, 0, 3, 0, 4, 0, dep) ;
         cube.rot4ver(1, 1, 2, 1, 3, 1, 4, 1, dep) ;
         break ;
case 2: // U2
         cube.rot2hor(1, 0, 2, 0, 3, 0, 4, 0, dep) ;
         cube.rot2ver(1, 1, 2, 1, 3, 1, 4, 1, dep) ;
         break ;
case 3: // U'
         cube.rot4hor(4, 0, 3, 0, 2, 0, 1, 0, dep) ;
         cube.rot4ver(4, 1, 3, 1, 2, 1, 1, 1, dep) ;
         break ;
case 4: // L
         cube.rot4hor(2, 3, 0, 3, 4, 1, 5, 3, dep) ;
         cube.rot4ver(2, 0, 0, 0, 4, 2, 5, 0, dep) ;
         break ;
case 5: // L2
         cube.rot2hor(2, 3, 5, 3, 4, 1, 0, 3, dep) ;
         cube.rot2ver(2, 0, 5, 0, 4, 2, 0, 0, dep) ;
         break ;
case 6: // L'
         cube.rot4hor(2, 3, 5, 3, 4, 1, 0, 3, dep) ;
         cube.rot4ver(2, 0, 5, 0, 4, 2, 0, 0, dep) ;
         break ;
case 7: // F
         cube.rot4hor(0, 2, 1, 1, 5, 0, 3, 3, dep) ;
         cube.rot4ver(0, 3, 1, 2, 5, 1, 3, 0, dep) ;
         break ;
case 8: // F2
         cube.rot2hor(0, 2, 3, 3, 5, 0, 1, 1, dep) ;
         cube.rot2ver(0, 3, 3, 0, 5, 1, 1, 2, dep) ;
         break ;
case 9: // F'
         cube.rot4hor(0, 2, 3, 3, 5, 0, 1, 1, dep) ;
         cube.rot4ver(0, 3, 3, 0, 5, 1, 1, 2, dep) ;
         break ;
case 10: // R
         cube.rot4hor(2, 1, 5, 1, 4, 3, 0, 1, dep) ;
         cube.rot4ver(2, 2, 5, 2, 4, 0, 0, 2, dep) ;
         break ;
case 11: // R2
         cube.rot2hor(2, 1, 5, 1, 4, 3, 0, 1, dep) ;
         cube.rot2ver(2, 2, 5, 2, 4, 0, 0, 2, dep) ;
         break ;
case 12: // R'
         cube.rot4hor(2, 1, 0, 1, 4, 3, 5, 1, dep) ;
         cube.rot4ver(2, 2, 0, 2, 4, 0, 5, 2, dep) ;
         break ;
case 13: // B
         cube.rot4hor(0, 0, 3, 1, 5, 2, 1, 3, dep) ;
         cube.rot4ver(0, 1, 3, 2, 5, 3, 1, 0, dep) ;
         break ;
case 14: // B2
         cube.rot2hor(0, 0, 3, 1, 5, 2, 1, 3, dep) ;
         cube.rot2ver(0, 1, 3, 2, 5, 3, 1, 0, dep) ;
         break ;
case 15: // B'
         cube.rot4hor(0, 0, 1, 3, 5, 2, 3, 1, dep) ;
         cube.rot4ver(0, 1, 1, 0, 5, 3, 3, 2, dep) ;
         break ;
case 16: // D
         cube.rot4hor(4, 2, 3, 2, 2, 2, 1, 2, dep) ;
         cube.rot4ver(4, 3, 3, 3, 2, 3, 1, 3, dep) ;
         break ;
case 17: // D2
         cube.rot2hor(1, 2, 2, 2, 3, 2, 4, 2, dep) ;
         cube.rot2ver(1, 3, 2, 3, 3, 3, 4, 3, dep) ;
         break ;
case 18: // D'
         cube.rot4hor(4, 2, 1, 2, 2, 2, 3, 2, dep) ;
         cube.rot4ver(4, 3, 1, 3, 2, 3, 3, 3, dep) ;
         break ;
      }
   }
}
#if WRITEFRAMES
int frameseq = 0 ;
uchar *framebuf = 0 ;
ll framew, frameh ;
int facexoff[] = { 1, 0, 1, 2, 3, 1 } ;
int faceyoff[] = { 0, 1, 1, 1, 1, 2 } ;
/*               w    o     g    r    b    y  */
int colorr[] = { 255, 255,   0, 255,   0, 255 } ;
int colorg[] = { 255, 165, 160,   0,   0, 255 } ;
int colorb[] = { 255,   0,   0,   0, 255,   0 } ;
void placepixel(int col, int x, int y) {
   y *= framew * 3 ;
   framebuf[y+3*x] = colorr[col] ;
   framebuf[y+3*x+1] = colorg[col] ;
   framebuf[y+3*x+2] = colorb[col] ;
}
void writeframe(xcube &cube) {
   writemovetarget += writemoveinc ;
   int N = cube.Mx + cube.My ;
   if (framebuf == 0) {
      framew = 4 * N ;
      frameh = 3 * N ;
      ll framemem = 3 * framew * frameh ;
      framebuf = (uchar *)calloc(1, framemem) ;
      if (framebuf == 0)
         error("! no memory for the frame buffer") ;
   }
   for (int i=0; i<6; i++)
      if (cube.facerots[i])
         error("! cannot yet render a rotated cube") ; 
   for (int i=0; i<cube.My; i++) {
      auto &row = cube.rows[i] ;
      for (int j=0; j<cube.Mx; j++) {
         auto &xo = row[j] ;
         for (int f=0; f<6; f++) {
            int fx = facexoff[f] * N ;
            int fy = faceyoff[f] * N ;
            placepixel(xo.a[4*f], fx+j, fy+i) ;
            placepixel(xo.a[4*f+1], fx+N-1-i, fy+j) ;
            placepixel(xo.a[4*f+2], fx+N-1-j, fy+N-1-i) ;
            placepixel(xo.a[4*f+3], fx+i, fy+N-1-j) ;
         }
      }
   }
   string framename = "frame-" ;
   cout << " " << frameseq << flush ;
   for (int dig=1000000; dig>=1; dig /= 10)
      framename.push_back((char)(frameseq / dig % 10 + '0')) ;
   framename += ".png" ;
   frameseq++ ;
   int t = fpng::fpng_encode_image_to_file(framename.c_str(), framebuf,
      framew, frameh, 3) ;
   if (t > 1) {
      cerr << t << endl ;
      error("! problem writing frame") ;
   }
}
#endif
/*
 *   Using elements 1 and 2 as generators, expand the set of rotations.
 *   We can encode each permutation of faces as a 32-bit int, and use
 *   the explicit static array as a bfs queue, so this is pretty easy.
 */
int encoderot(int *p) {
   return p[0] * 6 + p[1] ;
}
void expandrotations() {
   const int MAXENC = 6*6 ;
   char seen[MAXENC] ;
   if (rots[23][0] != 0 || rots[23][1] != 0)
      return ;
   for (int i=0; i<MAXENC; i++)
      seen[i] = 0 ;
   for (int i=0; i<3; i++)
      seen[encoderot(rots[i])] = 1 ;
   int qp = 3 ;
   int w[6] ;
   for (int qg=1; qg<24; qg++) {
      for (int g=1; g<3; g++) {
         for (int i=0; i<6; i++)
            w[i] = rots[qg][rots[g][i]] ;
         int ev = encoderot(w) ;
         if (seen[ev] == 0) {
            seen[ev] = 1 ;
            for (int i=0; i<6; i++)
               rots[qp][i] = w[i] ;
            qp++ ;
            if (qp > 24)
               error("! blew up expanding rotations") ;
         }
      }
   }
   if (qp != 24)
      error("! did not find the correct count of rotations") ;
}
struct blockalgo {
   vector<mv> mvs ;
   xorbit perm ;
   /*
    *   For counting: the number of row (depth 2) and column (depth 3)
    *   moves, each repeated once per row or column of the block; the
    *   outer turns before the first slice move (lead) and after the last
    *   (trail), which merge with the neighboring blocks; and the reduced
    *   length of the runs of outer turns in between.
    */
   int n2 = 0, n3 = 0, midlen = 0 ;
   vector<mv> lead, trail ;
   void analyze() {
      int first = -1, last = -1 ;
      for (int i=0; i<(int)mvs.size(); i++)
         if (mvs[i].dep > 1) {
            if (first < 0)
               first = i ;
            last = i ;
            if (mvs[i].dep == 2)
               n2++ ;
            else if (mvs[i].dep == 3)
               n3++ ;
            else
               error("! block algorithms may only use depths 1, 2, and 3") ;
         }
      if (first < 0)
         error("! block algorithm has no slice moves") ;
      lead.assign(mvs.begin(), mvs.begin()+first) ;
      for (int i=last+1; i<(int)mvs.size(); i++)
         pushreduce(trail, mvs[i]) ;
      vector<mv> run ;
      for (int i=first; i<=last; i++)
         if (mvs[i].dep > 1) {
            midlen += run.size() ;
            run.clear() ;
         } else
            pushreduce(run, mvs[i]) ;
   }
   void show() const {
      for (auto m: mvs) {
         cout << " " ;
         showmv(m) ;
      }
   }
} ;
int permbits(int s, xorbit &perm) {
   int r = 0 ;
   for (int i=0; i<24; i++)
      r |= ((s>>i)&1)<<perm.a[i] ;
   return r ;
}
int permpairs(int s, xorbit &perm) {
   int r = 0 ;
   for (int i=0; i<12; i++)
      r |= ((s>>(2*i))&3)<<(2*perm.a[i]) ;
   return r ;
}
xorbit tmp ;
void permmul(xorbit &a, xorbit &b) {
   for (int i=0; i<24; i++)
      tmp.a[b.a[i]] = a.a[i] ;
   a = tmp ;
}
void permimul(xorbit &a, xorbit &b) {
   for (int i=0; i<24; i++)
      tmp.a[i] = a.a[b.a[i]] ;
   a = tmp ;
}
vector<blockalgo> loadalgs(const char **algs, int nalgs, int m=0) {
   expandrotations() ;
   xcube cube(4, 5, 'U') ; // get the special version with 0..23
   vector<mv> mappedmoves ;
   vector<blockalgo> r ;
   for (int i=0; i<nalgs; i++) {
      auto mvs = parse_moves(algs[i]) ;
      uclear(cube.rows[0][1]) ;
      mappedmoves.clear() ;
      for (auto mv: mvs) {
         mv.face = rots[m][mv.face] ;
         mappedmoves.push_back(mv) ;
         domove(mv, cube) ;
      }
      // Test that algorithms don't affect other orbits
      for (int j=0; j<cube.My; j++)
         for (int k=0; k<cube.Mx; k++)
            if (j != 0 || k != 1) {
               for (int p=0; p<24; p++)
                  if (cube.rows[j][k].a[p] != p) {
                     cout << "Failed alg " << i << " at " << j << " " << k << " " << p << endl ;
                     error("! algorithm hurts other orbits?") ;
                  }
            }
      blockalgo b ;
      b.mvs = mappedmoves ;
      b.perm = cube.rows[0][1] ;
      r.push_back(b) ;
      r.back().analyze() ;
   }
   return r ;
}
ll blocks = 0 ;
/*
 *   We always buffer one block, so we can cancel moves between blocks.
 */
vector<mv> curblock, prevblock ;
void sendblock() {
   while (curblock.size() > 0 && prevblock.size() > 0) {
      mv merged ;
      int r = trymerge(prevblock[prevblock.size()-1], curblock[0], merged) ;
      if (r == 0) { // cancelled completely
         curblock.erase(curblock.begin()) ;
         prevblock.pop_back() ;
      } else if (r == 1) { // merged to just 1
         curblock[0] = merged ;
         prevblock.pop_back() ;
      } else { // nothing doing
         break ;
      }
   }
   if (prevblock.size()) {
      for (auto mv: prevblock) {
         cout << " " ;
         showmv(mv) ;
      }
      cout << endl ;
   }
   swap(prevblock, curblock) ;
   curblock.clear() ;
}
vector<mv> pendtrail ; // reduced outer turns ending the move stream
ll doblock(xcube &cube, blockalgo &alg, vector<int> &ys, vector<int> &xs) {
#if WRITEFRAMES
   if (writemoveinc && runningmovecount >= writemovetarget)
      writeframe(cube) ;
#endif
   blocks++ ;
   if (writemoves) {
      for (auto mv: alg.mvs) {
         if (mv.dep == 2) {
            for (auto v:ys) {
               mv.dep = v + 2 ;
               curblock.push_back(mv) ;
            }
         } else if (mv.dep == 3) {
            for (auto v:xs) {
               mv.dep = v + 2 ;
               curblock.push_back(mv) ;
            }
         } else {
            curblock.push_back(mv) ;
         }
      }
      vector<mv> red ;
      for (auto &m: curblock)
         pushreduce(red, m) ;
      swap(curblock, red) ;
      sendblock() ;
   }
   if (!fast) {
      for (auto mv: alg.mvs) {
         if (mv.dep == 2) {
            for (auto v:ys) {
               mv.dep = v + 2 ;
               domove(mv, cube) ;
            }
         } else if (mv.dep == 3) {
            for (auto v:xs) {
               mv.dep = v + 2 ;
               domove(mv, cube) ;
            }
         } else {
            domove(mv, cube)  ;
         }
      }
   }
   /*
    *   Exactly the number of moves the output gets: the lead merges with
    *   the trailing outer turns of the previous block (already counted
    *   there), the slice moves repeat per row/column, and the rest is fixed.
    */
   ll before = pendtrail.size() ;
   for (auto &m: alg.lead)
      pushreduce(pendtrail, m) ;
   ll mvs = (ll)pendtrail.size() - before
          + alg.n2 * (ll)ys.size() + alg.n3 * (ll)xs.size()
          + alg.midlen + (ll)alg.trail.size() ;
   pendtrail = alg.trail ;
   cancellations += alg.mvs.size() + alg.n2 * ((ll)ys.size() - 1)
                  + alg.n3 * ((ll)xs.size() - 1) - mvs ;
   runningmovecount += mvs ;
   return mvs ;
}
ll solvecount = 1 ;
struct stats {
   ll hits, moves ;
   void showstats(int id, double denom, double solvecount) ;
} ;
vector<stats> byphase ;
int extrapolate = 0 ;
/*
 *   This is the 24/12/12 network.
 */
const char *net241212_26[] = {
   "L' B2 2L' B2 3D B2 2L B2 3D' L",
   "L F 3U' B2 2R' B2 3U B2 2R B2 F' L'",
   "3U L 2D' L' 3U' L 2D L'",
   "L' B2 3B' L2 2D L2 3B L2 2D' L2 B2 L",
   "L2 2D F2 3L' F2 2D' F2 3L F2 L2",
   "U' L2 3B L2 2D L2 3B' L2 2D' U",
   "L U2 3L' B2 2U B2 3L B2 2U' B2 U2 L'",
   "F' U' 3U L2 2B L2 3U' L2 2B' L2 U F",
   "R' F' 2D L2 3B L2 2D' L2 3B' L2 F R",
   "B' F2 3R U2 2B' U2 3R' U2 2B U2 F2 B",
   "L F' U2 B2 2R B2 3U' B2 2R' B2 3U U2 F L'",
   "L2 R U' B2 2U B2 3L D B2 2U' 2B B2 D' 3L' D 2B' D' U R' L2",
   "L' D 2B2 2L' D' R' 3F' R D 2L 2B2 D' R' 3F R L",
   "L2 B D' F U L 2U L' 3F L 2U' L' D2 2L' D2 3F' D2 2L D2 U' F' D B' L2",
   "L B' D' U2 2B' D' 3L' D 2B F 2R2 F' D' 3L D F 2R2 F' U2 D B L'",
   "L2 B D R F2 2B L' 3U' L F2 2B' 2L F2 L' 3U L F2 2L' R' D' B' L2",
   "B D U2 2F U2 3L U2 2F' U2 B 2R' B' 3L' B 2R B' D' B'",
   "R U L2 2F L2 3D' L2 2F' L2 B 2U' B' 3D B 2U B' U' R'",
   "L' F U' 2L B2 3D B2 2L' R2 2B R2 B2 3D' R2 2B' R2 B2 U F' L",
   "R F B2 2L' D' 3R' D 2L B2 2D' B2 D' 3R D B2 2D F' R'",
   "B2 D2 D 2R' D' 3B D R2 2R 2U R2 D' 3B' R2 2U' R2 D2 B2",
   "L F2 2B D' 3L' D F2 2B' 2D F2 D' 3L D F2 2D' L'",
   "L R U2 F2 2U F2 3R U2 F2 2U' F2 2B' U2 3R' U2 2B R' L'",
   "B' F' U 2F U' 3L U 2F' U' B2 2U B2 3L' B2 2U' B2 F B",
   "B' D2 F2 2R F2 R' 3D' F2 2R' F2 R 2U' R' 3D R 2U D2 B",
   "2R B2 3U B2 2R' B2 3U' B2",
} ;
const char *net241212_27[] = {
   "3B' L2 2D L2 3B L2 2D' L2",
   "2B2 D' 3L' D 2B2 D' 3L D",
   "3F L 2U' L' 3F' L 2U L'",
   "3F' U 2R' U' 3F U 2R U'",
   "R 2F R' 3U' R 2F' R' 3U",
   "B 2D B' 3R B 2D' B' 3R'",
   "D2 3F D2 2L D2 3F' D2 2L'",
   "R' 3U R 2D' R' 3U' R 2D",
   "3R2 U 2B' U' 3R2 U 2B U'",
   "3D' L2 2F' L2 3D L2 2F L2",
   "3D' R 2B R' 3D R 2B' R'",
   "2F D2 3R D2 2F' D2 3R' D2",
   "U2 R2 3D R2 2B' R2 3D' R2 2B U2",
   "F' 3L' B2 2U B2 3L B2 2U' B2 F",
   "2U' F2 3R F2 2U F2 3R' F2",
   "F2 U2 2F U2 3L U2 2F' U2 3L' F2",
   "U2 B2 3D' B2 2L' B2 3D B2 2L U2",
   "F' R2 2U R2 3B R2 2U' R2 3B' F",
   "D F2 3D F2 2R F2 3D' F2 2R' D'",
   "R 3B' L2 2D L2 3B L2 2D' L2 R'",
   "L2 3F' D2 2L D2 3F D2 2L' D2 L2",
   "R' 3U' R2 2F R2 3U R2 2F' R2 R",
   "U' D2 2F' D2 3R' D2 2F D2 3R U",
   "R D2 3L' D2 2B' D2 3L D2 2B R'",
   "F2 3R' D2 2F D2 3R D2 2F' D2 F2",
   "U' 2U L2 3F L2 2U' L2 3F' L2 U",
   "3L' D2 2B D2 3L D2 2B' D2",
} ;
struct net241212 {
   int STAGE ;
   static const int n = 24 ;
/*
 *   24 choose 12 is only 2,704,156, while 2^24 is 16,777,216.
 *   Thus, we pack the combinations to keep arrays reasonably dense.
 */
   static const int CASES = 2704156 ;
   int unpack[CASES] ;
   int packhi[1<<12], packlo[1<<12] ;
   vector<blockalgo> algs ;
   static const int halfn = 12 ;
   static const int lowmask = (1<<halfn) - 1 ;
   static const int goal = CASES ;
/*
 *   This part can be slow since we only do it once.
 */
   net241212(const char **stringalgs, int nalgs, int m=0) {
      STAGE = nalgs ;
      algs = loadalgs(stringalgs, nalgs, m) ;
      int at = 0 ;
      for (int i=0; i<=lowmask; i++)
         packhi[i] = packlo[i] = -1 ;
      for (int i=0; (i>>n) == 0; i++)
         if (__builtin_popcount(i) == halfn) {
            unpack[at] = i ;
            if (packhi[i>>halfn] == -1)
               packhi[i>>halfn] = at ;
            if (packlo[i & lowmask] == -1)
               packlo[i & lowmask] = at - packhi[i>>halfn] ;
            at++ ;
         }
      if (goal != at)
         cerr << "Failed in goal computation" << endl ;
      /* test */
      at = 0 ;
      for (int i=0; (i>>n) == 0; i++)
         if (__builtin_popcount(i) == halfn) {
            int v = packhi[i>>halfn] + packlo[i & lowmask] ;
            if (v != at)
               cout << "Failed 1" << endl ;
            if (unpack[v] != i)
               cout << "Failed 2" << endl ;
            at++ ;
         }
   }
/*
 *   Plan how to execute this based on the permutations from the
 *   algorithms.  *   We iterate on prefixes of the triples, finding
 *   the best solution for every reachable state on each prefix.  We
 *   use -1 to mean unreachable for now.  Note that we are searching
 *   from solved, so to solve a state we need to reverse the
 *   solutions and triples.
 */
   ll plans[CASES] ;
   void plan() {
      vector<ll> next(CASES, -1) ;
      for (int i=0; i<CASES; i++)
         plans[i] = -1 ;
      int start = 0xf0ff ; // cshape
      int starti = packhi[start>>halfn] + packlo[start&lowmask] ;
      plans[starti] = 0 ;
      for (int i=0; i<STAGE; i++) {
         if (minstages)
            next.assign(plans, plans+CASES) ;
         for (int j=0; j<CASES; j++)
            if (plans[j] != -1) {
               int k = permbits(unpack[j], algs[i].perm) ;
               int d = packhi[k>>halfn] + packlo[k&lowmask] ;
               if (next[d] < 0 ||
           __builtin_popcountll(plans[j]) + 1 < __builtin_popcountll(next[d]))
                  next[d] = plans[j] | (1LL << i) ;
            }
         for (int j=0; j<CASES; j++)
            if (next[j] >= 0 && (plans[j] < 0 || minstages))
               plans[j] = next[j] ;
      }
      for (int i=0; i<CASES; i++)
         if (plans[i] < 0)
            cout << "Could not get to " << i << endl ;
      if (showdetails) {
         vector<int> cnts(STAGE) ;
         for (int i=0; i<CASES; i++)
            for (int j=0; j<STAGE; j++)
               if ((plans[i]>>j) & 1)
                  cnts[j]++ ;
         for (int i=0; i<STAGE; i++)
            cout << "net241212: " << i << " " << cnts[i]/(double)CASES << endl ;
      }
   }
   ll getplan(xorbit &orb) const {
      ll k = 0 ;
      for (int i=0; i<24; i++)
         k |= ((11 >> orb.a[i]) & 1) << i ;
      return plans[packhi[k>>halfn]+packlo[k&lowmask]] ;
   }
   void doplan(ll plan, xorbit &orb) {
      for (int pi=STAGE-1; pi>=0; pi--) {
         if (((plan >> pi) & 1) == 0)
            continue ;
         permimul(orb, algs[pi].perm) ;
      }
   }
} ;
/*
 *   This is the 12/4/4/4 network.
 */
const char *net12444_20[] = {
   "3B2 L 2F2 L' 3B2 L 2F2 L'",
   "R' 3B' R 2F R' 3B R 2F'",
   "L' 3F2 L 2B L' 3F2 L 2B'",
   "3F U 2B U' 3F' U 2B' U'",
   "U' 3B U 2F U' 3B' U 2F'",
   "3B U 2F U' 3B' U 2F' U'",
   "2B U' 3F U 2B' U' 3F' U",
   "2B U' 3F' U 2B' U' 3F U",
   "L 2U2 L' 3D2 L 2U2 L' 3D2",
   "2F R' 3B2 R 2F' R' 3B2 R",
   "2D2 R' 3F' R 2D2 R' 3F R",
   "2U2 L' 3F L 2U2 L' 3F' L",
   "3U2 L 2B' L' 3U2 L 2B L'",
   "3D2 L 2F L' 3D2 L 2F' L'",
   "L' 3B' L 2D2 L' 3B L 2D2",
   "2U2 R' 3B R 2U2 R' 3B' R",
   "2F' U' 3B U 2F U' 3B' U",
   "R' 3F R 2B2 R' 3F' R 2B2",
   "U 2F U' 3B U 2F' U' 3B'",
   "L 2B2 L' 3F2 L 2B2 L' 3F2",
} ;
const char *net12444_19[] = {
   "U 2F U' 3B U 2F' U' 3B'",
   "U 2B U' 3F U 2B' U' 3F'",
   "2B' U' 3F U 2B U' 3F' U",
   "L 2B2 L' 3F2 L 2B2 L' 3F2",
   "3B2 R 2F2 R' 3B2 R 2F2 R'",
   "L 2B L' 3F2 L 2B' L' 3F2",
   "3F U 2B' U' 3F' U 2B U'",
   "3B2 R 2F R' 3B2 R 2F' R'",
   "R' 3B2 R 2F2 R' 3B2 R 2F2",
   "L 2F' L' U' 3B U L 2F L' U' 3B' U",
   "U 2F' U' L' 3B L U 2F U' L' 3B' L",
   "L' U U 2F U' R' 3B' R U 2F' U' R' 3B R U' L",
   "2U2 L' 3F L 2U2 L' 3F' L",
   "R 3F2 L 2B2 L' 3F2 L 2B2 L' R'",
   "R U2 L U 2F' U' L' 3B L U 2F U' L' 3B' U2 R'",
   "L 3B' U 2F U' 3B U 2F' U' L'",
   "L 3F' L 2B L' 3F L 2B' L' L'",
   "L' 2F U' 3B' U 2F' U' 3B U L",
   "2B' U' 3F U 2B U' 3F' U",
} ;
const char *net12444_18[] = {
   "3F R 2B2 R' 3F' R 2B2 R'",
   "L' 3B L 2F2 L' 3B' L 2F2",
   "2F' U' 3B U 2F U' 3B' U",
   "2B' U' 3F U 2B U' 3F' U",
   "L' 3F2 L 2B2 L' 3F2 L 2B2",
   "3B U 2F' U' 3B' U 2F U'",
   "U 2B' U' 3F U 2B U' 3F'",
   "2B U' 3F' U 2B' U' 3F U",
   "L 2F2 L' 3B2 L 2F2 L' 3B2",
   "L' 3B' U 2F U' 3B U 2F' U' L",
   "L 2F' L' U' 3B U L 2F L' U' 3B' U",
   "R U' 2B' D' R' 3F2 R D 2B' D' R' 3F2 R D 2B2 U R'",
   "L' R' 2R2 3F D U 2B D' U' 3F' D U 2B' D' U' 2R2 R L",
   "L2 U 2F L 2F2 L' D' 3B2 D L 2F2 L' D' 3B2 D 2F' U' L2",
   "L2 U L 2L2 D 2F2 D' L' 3B2 L D 2F2 D' L' 3B2 2L2 U' L2",
   "R L' 2B D' 3F2 D 2B' L 2B2 L' D' 3F2 D L 2B2 R'",
   "L U' 2R2 3F D U 2B D' U' 3F' D U 2B' D' U' 2R2 U L'",
   "R2 2F2 R' D' 3B2 D R 2F' R' D' 3B2 D R 2F' R2",
} ;
struct net12444 {
   int STAGE ;
   static const int n = 12 ;
   static const int CASES = 34650 ;
   int unpack[CASES] ;
   int packhi[1<<12], packlo[1<<12] ;
   vector<blockalgo> algs, algs2 ;
   vector<xorbit> smallperms ;
   static const int halfn = 12 ;
   static const int lowmask = (1<<12)-1 ;
   static const int goal = CASES ;
/*
 *   This part can be slow since we only do it once.
 */
   static const int constexpr downshift[] = {0, 1, 2, 3, 4, 5, 6, 7, -1, -1, -1, -1,
      8, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1, -1 } ;
   static const int constexpr upshift[] = {0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15} ;
   net12444(const char **stringalgs, int nalgs) {
      STAGE = nalgs ;
      algs = loadalgs(stringalgs, nalgs) ;
      algs2 = loadalgs(stringalgs, nalgs, 12) ;
/*
 *   The input algs should all fix 8..11 and 16..23.
 *   only work on 0..11.
 */
      smallperms.resize(nalgs) ;
      for (int i=0; i<STAGE; i++) {
         xorbit &src = algs[i].perm ;
         for (int j=0; j<24; j++)
            if (downshift[j] < 0 && src.a[j] != j) {
               cout << "Algorithm " << i << " fails at " << j << endl ;
               cout << stringalgs[i] << endl ;
               for (int k=0; k<24; k++)
                  cout << " " << (int)src.a[k] ;
               cout << endl ;
               error("! one of the net12444 perms changes the other side") ;
            }
         for (int j=0; j<12; j++)
            smallperms[i].a[j] = downshift[algs[i].perm.a[upshift[j]]] ;
      }
      int at = 0 ;
      for (int i=0; i<=lowmask; i++)
         packhi[i] = packlo[i] = -1 ;
      for (int i=0; (i>>24) == 0; i++)
         if (__builtin_popcount(i & (i >> 1) & 0x555555) == 0 &&
             __builtin_popcount(i & ((~i) >> 1) & 0x555555) == 4 &&
             __builtin_popcount((~i) & ((~i) >> 1) & 0x555555) == 4 &&
             __builtin_popcount((~i) & (i >> 1) & 0x555555) == 4) {
            unpack[at] = i ;
            if (packhi[i>>halfn] == -1)
               packhi[i>>halfn] = at ;
            if (packlo[i & lowmask] == -1)
               packlo[i & lowmask] = at - packhi[i>>halfn] ;
            at++ ;
         }
      if (goal != at)
         cerr << "Failed at setup" << endl ;
      /* test */
      at = 0 ;
      for (int i=0; (i>>24) == 0; i++)
         if (__builtin_popcount(i & (i >> 1) & 0x555555) == 0 &&
             __builtin_popcount(i & ((~i) >> 1) & 0x555555) == 4 &&
             __builtin_popcount((~i) & ((~i) >> 1) & 0x555555) == 4 &&
             __builtin_popcount((~i) & (i >> 1) & 0x555555) == 4) {
            int v = packhi[i>>halfn] + packlo[i & lowmask] ;
            if (v != at)
               cout << "Failed 1" << endl ;
            if (unpack[v] != i)
               cout << "Failed 2" << endl ;
            at++ ;
         }
   }
/*
 *   Plan how to execute this based on the permutations in the algorithms.
 *   Same idea as the plan above.
 */
   ll plans[CASES] ;
   void plan() {
      vector<ll> next(CASES, -1) ;
      for (int i=0; i<CASES; i++)
         plans[i] = -1 ;
      plans[0] = 0 ;
      for (int i=0; i<STAGE; i++) {
         if (minstages)
            next.assign(plans, plans+CASES) ;
         for (int j=0; j<CASES; j++)
            if (plans[j] != -1) {
               int k = permpairs(unpack[j], smallperms[i]) ;
               int d = packhi[k>>halfn] + packlo[k&lowmask] ;
               if (next[d] < 0 ||
            __builtin_popcountll(plans[j]) + 1 < __builtin_popcountll(next[d]))
                  next[d] = plans[j] | (1LL << i) ;
            }
         for (int j=0; j<CASES; j++)
            if (next[j] >= 0 && (plans[j] < 0 || minstages))
               plans[j] = next[j] ;
      }
      if (showdetails) {
         vector<int> cnts(STAGE) ;
         for (int i=0; i<CASES; i++) {
            if (plans[i] < 0)
               cout << "Could not get to " << i << endl ;
            for (int j=0; j<STAGE; j++)
               if ((plans[i]>>j) & 1)
                  cnts[j]++ ;
         }
         for (int i=0; i<STAGE; i++)
            cout << "net12444: " << i << " " << cnts[i]/(double)CASES << endl ;
      }
   }
   static int constexpr maplo[] = {2, 1, 0, 0, 1, 2} ;
// static int constexpr upshift2[] = { 23, 20, 21, 22, 18, 19, 16, 17, 10, 11, 8, 9 } ;
   static int constexpr upshift2[] = { 23, 20, 21, 22, 18, 19, 16, 17, 10, 11, 8, 9 } ;
   int getplan(xorbit &orb, int half) const {
      ll k = 0 ;
      if (half)
         for (int i=11; i>=0; i--)
            k = 4 * k + maplo[orb.a[upshift2[i]]] ;
      else
         for (int i=11; i>=0; i--)
            k = 4 * k + maplo[orb.a[upshift[i]]] ;
      return plans[packhi[k>>halfn]+packlo[k&lowmask]] ;
   }
   void doplan(int plan, xorbit &orb, int half) {
      for (int pi=STAGE-1; pi>=0; pi--) {
         if (((plan >> pi) & 1) == 0)
            continue ;
         if (half)
            permimul(orb, algs2[pi].perm) ;
         else
            permimul(orb, algs[pi].perm) ;
      }
   }
} ;
struct net241212 *outer ;
struct net12444 *inner ;
void stats::showstats(int id, double denom, double solvecount) {
   int p1 = (id >= outer->STAGE ? 2 : 1) ;
   if (hits)
      cout << "Phase " << p1 << " " << id << " p " << hits/(denom*solvecount) << " mvs " << moves / solvecount << endl ;
}
ll solve(xcube &cube, int width, int phasewidth) {
   byphase.resize(outer->STAGE + 2 * inner->STAGE) ;
   if (phasewidth > cube.My)
      phasewidth = cube.My ;
   if (phasewidth < width)
      phasewidth = width ;
   ll r = 0 ;
   vector<int> xs ;
   vector<int> ys ;
   vector<ll> bits ;
   vector<int> next, prev ;
   vector<vector<ll>> movemasks(phasewidth) ;
   for (int i=0; i<phasewidth; i++)
      movemasks[i].resize(cube.Mx) ;
   for (int i=0; i<cube.My; i += phasewidth) {
      int pw = phasewidth ;
      if (i + pw > cube.My)
         pw = cube.My - i ;
      for (int wi=0; wi<pw; wi++)
         cube.getrow(i+wi) ;
      for (int ii=0; ii<pw; ii++)
         for (int j=0; j<cube.Mx; j++) {
            movemasks[ii][j] = outer->getplan(cube.rows[i+ii][j]) ;
            if (fast && !writemoveinc)
               outer->doplan(movemasks[ii][j], cube.rows[i+ii][j]) ;
         }
      int phasebase = 0 ;
      for (int pi=outer->STAGE-1; pi>=0; pi--, phasebase++) {
         auto &alg = outer->algs[pi] ;
         if (showdetails) {
            cout << "Alg " << phasebase << " " << alg.mvs.size() ;
            alg.show() ;
            cout << endl ;
         }
         for (int ii=0; ii<pw; ii += width) {
            int w = width ;
            if (ii + w > pw)
               w = pw - ii ;
            bits.clear() ;
            bits.resize(cube.Mx) ;
            for (int wi=0; wi<w; wi++)
               for (int j=0; j<cube.Mx; j++)
                  if ((movemasks[ii+wi][j] >> pi) & 1)
                     bits[j] |= 1LL<<wi ;
            prev.clear() ;
            prev.resize(1LL<<w, -1) ;
            next.resize(cube.Mx) ;
            for (int i=0; i<cube.Mx; i++) {
               next[i] = prev[bits[i]] ;
               prev[bits[i]] = i ;
            }
            for (ll bp=1; bp<(1LL<<w); bp++) {
               if (prev[bp] < 0)
                  continue ;
               ys.clear() ;
               for (int wi=0; wi<w; wi++)
                  if ((bp >> wi) & 1)
                     ys.push_back(i+ii+wi) ;
               xs.clear() ;
               for (int i=prev[bp]; i>=0; i=next[i])
                  xs.push_back(i) ;
               byphase[phasebase].hits += xs.size() * ys.size() ;
               ll mvs = doblock(cube, alg, ys, xs) ;
               if (writemoveinc && fast)
                  for (auto y: ys)
                     for (auto x: xs)
                        outer->doplan(1LL<<pi, cube.rows[y][x]) ;
               byphase[phasebase].moves += mvs ;
               r += mvs ;
            }
         }
      }
      for (int half=0; half<2; half++) {
         for (int ii=0; ii<pw; ii++)
            for (int j=0; j<cube.Mx; j++) {
               movemasks[ii][j] = inner->getplan(cube.rows[i+ii][j], half) ;
               if (fast && !writemoveinc)
                  inner->doplan(movemasks[ii][j], cube.rows[i+ii][j], half) ;
            }
         for (int pi=inner->STAGE-1; pi>=0; pi--, phasebase++) {
            auto &alg = half ? inner->algs2[pi] : inner->algs[pi] ;
            if (showdetails) {
               cout << "Alg " << phasebase << " " << alg.mvs.size() ;
               alg.show() ;
               cout << endl ;
            }
            for (int ii=0; ii<pw; ii += width) {
               int w = width ;
               if (ii + w > pw)
                  w = pw - ii ;
               bits.clear() ;
               bits.resize(cube.Mx) ;
               for (int wi=0; wi<w; wi++)
                  for (int j=0; j<cube.Mx; j++)
                     if ((movemasks[ii+wi][j] >> pi) & 1)
                        bits[j] |= 1LL<<wi ;
               prev.clear() ;
               prev.resize(1LL<<w, -1) ;
               next.resize(cube.Mx) ;
               for (int i=0; i<cube.Mx; i++) {
                  next[i] = prev[bits[i]] ;
                  prev[bits[i]] = i ;
               }
               for (ll bp=1; bp<(1LL<<w); bp++) {
                  if (prev[bp] < 0)
                     continue ;
                  ys.clear() ;
                  for (int wi=0; wi<w; wi++)
                     if ((bp >> wi) & 1)
                        ys.push_back(i+ii+wi) ;
                  xs.clear() ;
                  for (int i=prev[bp]; i>=0; i=next[i])
                     xs.push_back(i) ;
                  byphase[phasebase].hits += xs.size() * ys.size() ;
                  ll mvs = doblock(cube, alg, ys, xs) ;
                  if (writemoveinc && fast)
                     for (auto y: ys)
                        for (auto x: xs)
                           inner->doplan(1LL<<pi, cube.rows[y][x], half) ;
                  byphase[phasebase].moves += mvs ;
                  r += mvs ;
               }
            }
         }
      }
      for (int wi=0; wi<pw; wi++)
         cube.checkrelease(i+wi) ;
      if (extrapolate)
         break ;
   }
   return r ;
}
int stats = 0 ;
ll dosolve(ll N, int width, int phasewidth) {
   pendtrail.clear() ;
   ll My = (N - 2) >> 1 ;
   ll Mx = (N - 2) - My ;
   blocks = 0 ;
   cancellations = 0 ;
   xcube cube(Mx, My, readmoves ? 'I' : 'R') ;
#if WRITEFRAMES
   runningmovecount = 0 ;
   if (writemoveinc)
      writeframe(cube) ;
#endif
   ll r = solve(cube, width, phasewidth) ;
   sendblock() ; // flush out any remaining moves
   if (alwaysinmem)
      for (int i=0; i<cube.My; i++)
         cube.checkrow(i) ;
#if WRITEFRAMES
   if (writemoveinc) {
      writeframe(cube) ;
      cout << endl ;
   }
#endif
   return r ;
}
/*
 *   Worst case (-W).  Within one stage every block uses the same
 *   algorithm, so a block that follows another block of its stage costs
 *   exactly ell + midlen + |trail| + n2 |ys| + n3 |xs|, where ell is what
 *   the lead adds after cancelling against the trail; only the first
 *   block of a stage can follow a different algorithm, and its lead adds
 *   at most |lead|.  A strip of width w over m columns has at most
 *   min(2^w - 1, m) blocks (one per distinct nonzero row pattern), every
 *   column is in at most one of them, and the bound takes the patterns
 *   with the most rows.  Stages that no orbit state's plan uses cost
 *   nothing; all others are assumed to hit their worst case at once.
 */
lll binom(int n, int k) {
   lll r = 1 ;
   for (int i=1; i<=k; i++)
      r = r * (n - k + i) / i ;
   return r ;
}
struct wcalg {
   lll first, ell, base, n2, n3 ;
} ;
wcalg wcinfo(const blockalgo &a) {
   wcalg r ;
   vector<mv> st = a.trail ;
   for (auto &m: a.lead)
      pushreduce(st, m) ;
   r.first = a.lead.size() ;
   r.ell = (lll)st.size() - (lll)a.trail.size() ;
   r.base = a.midlen + (lll)a.trail.size() ;
   r.n2 = a.n2 ;
   r.n3 = a.n3 ;
   return r ;
}
lll wcstrip(const wcalg &a, int w, ll m) {
   lll k = 0, pops = 0 ;
   for (int j=w; j>=1; j--) {
      lll take = binom(w, j) ;
      if (take > m - k)
         take = m - k ;
      k += take ;
      pops += take * j ;
      if (k == m)
         break ;
   }
   return a.n3 * m + k * (a.ell + a.base) + a.n2 * pops ;
}
/*
 *   Cost of one stage over rows split into nw strips of width w and
 *   nw1 strips of width w+1, in phases as solve() does it.
 */
lll wcstage(const wcalg &a, ll Mx, int w, ll nw, ll nw1, ll nphases) {
   return nw * wcstrip(a, w, Mx) + nw1 * wcstrip(a, w+1, Mx)
        + nphases * (a.first - a.ell) ;
}
vector<wcalg> wcused(vector<blockalgo> &algs, ll used) {
   vector<wcalg> r ;
   for (int i=0; i<(int)algs.size(); i++)
      if ((used >> i) & 1)
         r.push_back(wcinfo(algs[i])) ;
   return r ;
}
lll wctotal(vector<wcalg> &all, ll Mx, int w, ll nw, ll nw1, ll nphases) {
   lll r = 0 ;
   for (auto &a: all)
      r += wcstage(a, Mx, w, nw, nw1, nphases) ;
   return r ;
}
void worstcase(ll N) {
   ll My = (N - 2) >> 1 ;
   ll Mx = (N - 2) - My ;
   ll uo = 0, ui = 0 ;
   for (int i=0; i<outer->CASES; i++)
      uo |= outer->plans[i] ;
   for (int i=0; i<inner->CASES; i++)
      ui |= inner->plans[i] ;
   vector<wcalg> all = wcused(outer->algs, uo) ;
   if (showdetails) {
      for (auto &a: all)
         cout << "outer " << a.first << " " << a.ell << " " << a.base << " " << a.n2 << " " << a.n3 << endl ;
      for (auto &a: wcused(inner->algs, ui))
         cout << "inner " << a.first << " " << a.ell << " " << a.base << " " << a.n2 << " " << a.n3 << endl ;
      for (auto &a: wcused(inner->algs2, ui))
         cout << "inner2 " << a.first << " " << a.ell << " " << a.base << " " << a.n2 << " " << a.n3 << endl ;
   }
   for (auto &a: wcused(inner->algs, ui))
      all.push_back(a) ;
   for (auto &a: wcused(inner->algs2, ui))
      all.push_back(a) ;
   /*
    *   Uniform width w with the remainder in a last narrower strip (what
    *   solve() does with -w w and one phase), and the best split of the
    *   rows into strips of widths w and w+1.
    */
   lll bestu = -1, bestm = -1 ;
   int bwu = 0, bwm = 0 ;
   ll bnw = 0, bnw1 = 0 ;
   for (int w=1; w<=40 && w<=My; w++) {
      lll u = wctotal(all, Mx, w, My / w, 0, 1) ;
      if (My % w)
         u += wctotal(all, Mx, (int)(My % w), 1, 0, 0) ;
      if (showdetails)
         cout << "width " << w << " worst " << u << endl ;
      if (bestu < 0 || u < bestu) {
         bestu = u ;
         bwu = w ;
      }
      /*
       *   nw1 must be congruent to My mod w; the cost is linear in nw1,
       *   so only the smallest and largest such nw1 matter.
       */
      ll lo = My % w, hi = My / (w + 1) ;
      hi -= ((hi - lo) % w + w) % w ;
      for (ll nw1: {lo, hi}) {
         if (nw1 < lo || nw1 * (w + 1) > My)
            continue ;
         ll nw = (My - nw1 * (w + 1)) / w ;
         lll t = wctotal(all, Mx, w, nw, nw1, 1) ;
         if (bestm < 0 || t < bestm) {
            bestm = t ;
            bwm = w ;
            bnw = nw ;
            bnw1 = nw1 ;
         }
      }
   }
   cout << "N " << N << " stages " << __builtin_popcountll(uo)
        << "+2x" << __builtin_popcountll(ui)
        << " worst " << bestu << " width " << bwu
        << " mixed " << bestm << " widths " << bnw << "x" << bwm
        << "+" << bnw1 << "x" << bwm+1 << endl ;
}
double extrapolatemul = 1 ;
struct basicstat {
   basicstat(const char *n_) : name(n_), n(0), sx(0), sxx(0), hi(0), lo(0) {}
   void add(double x) {
      x *= extrapolatemul ;
      n++ ;
      if (n == 1)
         hi = lo = x ;
      sx += x ;
      sxx += x * x ;
      hi = max(hi, x) ;
      lo = min(lo, x) ;
   }
   void report() {
      double av = sx / n ;
      double stddev = sqrt(n * sxx - sx * sx) / n ;
      cout << name << " " << av << " stddev " << stddev << " range " << lo << " .. " << hi ;
   }
   void avreport() {
      double av = sx / n ;
      cout << name << " " << av ;
   }
   const char *name ;
   ll n ;
   double sx, sxx, hi, lo ;
} ;
basicstat mvstat("moves"), cancelstat("cancels"), blockstat("blocks") ;
ll N ;
int width=1, phasewidth=1 ;
void fullreport(ll at) {
   if (stats && solvecount == mvstat.n)
      for (int i=0; i<(int)byphase.size(); i++)
         byphase[i].showstats(i, (N/2-1)*(N-N/2-1), mvstat.n);
   cout << "N " << N << " width " << width
        << " phasewidth " << phasewidth << " cnt " << at << " " ;
   mvstat.report() ; cout << " " ;
   cancelstat.avreport() ; cout << " " ;
   blockstat.avreport() ; cout << " phases " << byphase.size() << endl;
}
void addcounts(ll v) {
   mvstat.add(v)  ;
   cancelstat.add(cancellations) ;
   blockstat.add(blocks) ;
   if (solvecount > 1 && (mvstat.n & (mvstat.n - 1)) == 0 & mvstat.n != solvecount)
      fullreport(mvstat.n) ;
}
void usage() {
   cout << "Brobdicube (C) 2026 Tomas Rokicki." << endl <<
      endl <<
      "Usage:  brobdicube [options] size" << endl <<
      endl <<
      "Options:" << endl <<
      endl <<
      "-a x y  select first phase of length x and second phase of length y." << endl << 
      "-d      show some internal information on the strategy." << endl <<
      "-i      read scramble moves from stdin." << endl <<
      "-m      plan each orbit state with the fewest stages (default: the\n"
      "        first plan found, which leaves the last stages rarely used)." << endl <<
      "-M n    do this many solves and report aggregate stats." << endl <<
      "-o      write solution to stdout (on lines starting with a space)." << endl <<
      "-p n    set the phase chunking width." << endl <<
      "-s      actually (slowly) simulate moves; for testing." << endl <<
      "-S      print detailed statistics per phase." << endl <<
      "-W      print an upper bound on the moves for any scramble." << endl <<
      "-w n    set the strip width for 2d optimization." << endl <<
      "-x      extrapolate statistics from a single strip." << endl <<
#if WRITEFRAMES
      "-v n    write a png file of the cube state every n moves." << endl <<
#endif
      endl ;
   exit(0) ;
}
int outersol=27, innersol=20, doworst=0 ;
int main(int argc, char *argv[]) {
   cout.precision(15) ;
   cout << "-" ;
   for (int i=0; i<argc; i++)
      cout << " " << argv[i] ;
   cout << endl ;
   while (argc > 1 && argv[1][0] == '-') {
      argc-- ;
      argv++ ;
      switch (argv[0][1]) {
case 'x':
         extrapolate = 1 ;
         break ;
case 's':
         fast = 0 ;
         break ;
case 'S':
         stats = 1 ;
         break ;
case 'd':
         showdetails = 1 ;
         break ;
case 'w':
         argc-- ;
         argv++ ;
         width = atoll(argv[0]) ;
         break ;
case 'p':
         argc-- ;
         argv++ ;
         phasewidth = atoll(argv[0]) ;
         break ;
case 'M':
         argc-- ;
         argv++ ;
         solvecount = atoll(argv[0]) ;
         break ;
case 'a':
         argc-- ;
         argv++ ;
         outersol = atoll(argv[0]) ;
         argc-- ;
         argv++ ;
         innersol = atoll(argv[0]) ;
         break ;
#if WRITEFRAMES
case 'v':
         argc-- ;
         argv++ ;
         writemoveinc = atoll(argv[0]) ;
         break ;
#endif
case 'o':
         writemoves = 1 ;
         break ;
case 'i':
         readmoves = 1 ;
         break ;
case 'W':
         doworst = 1 ;
         break ;
case 'm':
         minstages = 1 ;
         break ;
case 'z':
         argc-- ;
         argv++ ;
         rng.seed(atoll(argv[0])) ;
         break ;
default:
         error("! option not understood") ;
      }
   }
   expandrotations() ;
   if (outersol == 27)
      outer = new net241212(net241212_27, 27, 1) ;
   else if (outersol == 26)
      outer = new net241212(net241212_26, 26, 1) ;
   else
      error("! outer must be 26 or 27") ;
   if (innersol == 20)
      inner = new net12444(net12444_20, 20) ;
   else if (innersol == 19)
      inner = new net12444(net12444_19, 19) ;
   else if (innersol == 18)
      inner = new net12444(net12444_18, 18) ;
   else
      error("! inner must be 18, 19, or 20") ;
   inner->plan() ;
   outer->plan() ;
   if (!fast && extrapolate)
      error("! cannot go slow and extrapolate at the same time") ;
   if (argc < 2)
      usage() ;
   if (argc > 2)
      error("! extra arguments given on command line") ;
   N = atoll(argv[1]) ;
   if (doworst) {
      worstcase(N) ;
      return 0 ;
   }
   ll My = (N - 2) >> 1 ;
   ll Mx = (N - 2) - My ;
   if (phasewidth < width)
      phasewidth = width ;
   if (phasewidth > My)    // solve() clamps it too; the scaling must agree
      phasewidth = My ;
   if (extrapolate)
      extrapolatemul = My / (double)phasewidth ;
   if (!fast || writemoveinc || readmoves)
      alwaysinmem = 1 ;
   if ((readmoves || writemoves || writemoveinc) && solvecount > 1)
      error("! cannot write frames and read or write moves, or do multiple solves at the same time") ;
   if (readmoves)
      readmoveset() ;
   ll mmc = 0 ;
   for (ll ii=1; ii<=solvecount; ii++) {
      ll t = dosolve(N, width, phasewidth) ;
      addcounts(t) ;
      mmc += t ;
   }
   if (solvecount == 1) {
      ll r = mmc ;
      if (extrapolate) {
         if (stats)
            for (int i=0; i<(int)byphase.size(); i++)
               byphase[i].showstats(i, My*phasewidth, mvstat.n) ;
         lll lll_r = (lll)r * My / phasewidth ;
 cout << "r " << r << " My " << My << " width " << width << " lll_r " << lll_r << endl ;
         lll lll_cancellations = (lll)cancellations * My / phasewidth ;
         lll lll_blocks = (lll)blocks * My / phasewidth ;
         cout << "N " << N << " width " << width
              << " moves " << lll_r << " cancel " << lll_cancellations
              << " blocks " << lll_blocks << " phases " << byphase.size() << endl ;
      } else {
         if (stats)
            for (int i=0; i<(int)byphase.size(); i++)
               byphase[i].showstats(i, Mx*My, mvstat.n) ;
         cout << "N " << N << " width " << width
              << " moves " << r << " cancel " << cancellations
              << " blocks " << blocks << " phases " << byphase.size() << endl ;
      }
   } else {
      fullreport(solvecount) ;
   }
}
