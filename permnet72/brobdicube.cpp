#ifndef WRITEFRAMES
#define WRITEFRAMES (1)
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
 *   These algorithms execute a 3swap on cubes in one orbit.  They are
 *   all +2/+2 (adding a column or row extends the cost by 1).  They
 *   are optimal.  They work on diagonals.  The ones that are length
 *   9 will merge with one-move cancellations, and the ones that are
 *   length 10 will merge with two-move cancellations.  These are the
 *   ones unique up to full-cube rotations; to calculate the rest,
 *   conjugate these by rotations.
 */
const char *triswapalgs[] = {
   "F 2D F' 3U F 2D' F' 3U'",
   "F 2D F' 3U2 F 2D' F' 3U2",
   "F 2D F' 3U' F 2D' F' 3U",
   "F 2D F' 3L F 2D' F' 3L'",
   "F 2D F' 3L2 F 2D' F' 3L2",
   "F 2D F' 3L' F 2D' F' 3L",
   "F 2D2 F' 3U F 2D2 F' 3U'",
   "F 2D2 F' 3U2 F 2D2 F' 3U2",
   "F 2D2 F' 3U' F 2D2 F' 3U",
   "F 2D' F' 3U F 2D F' 3U'",
   "F 2D' F' 3U2 F 2D F' 3U2",
   "F 2D' F' 3U' F 2D F' 3U",
   "F 2D' F' 3L F 2D F' 3L'",
   "F 2D' F' 3L2 F 2D F' 3L2",
   "F 2D' F' 3L' F 2D F' 3L",
   "F' 3D F 2U F' 3D' F 2U'",
   "F' 3D F 2U2 F' 3D' F 2U2",
   "F' 3D F 2U' F' 3D' F 2U",
   "F' 3D F 2R F' 3D' F 2R'",
   "F' 3D F 2R2 F' 3D' F 2R2",
   "F' 3D F 2R' F' 3D' F 2R",
   "F' 3D2 F 2U F' 3D2 F 2U'",
   "F' 3D2 F 2U2 F' 3D2 F 2U2",
   "F' 3D2 F 2U' F' 3D2 F 2U",
   "F' 3D' F 2U F' 3D F 2U'",
   "F' 3D' F 2U2 F' 3D F 2U2",
   "F' 3D' F 2U' F' 3D F 2U",
   "F' 3D' F 2R F' 3D F 2R'",
   "F' 3D' F 2R2 F' 3D F 2R2",
   "F' 3D' F 2R' F' 3D F 2R",
   "2F D' 3B' D 2F' D' 3B D",
   "2F2 D' 3B2 D 2F2 D' 3B2 D",
   "2F2 D' 3R D 2F2 D' 3R' D",
   "2F2 D' 3R' D 2F2 D' 3R D",
   "2F' D' 3B D 2F D' 3B' D",
   "3F D 2B' D' 3F' D 2B D'",
   "3F2 D 2B2 D' 3F2 D 2B2 D'",
   "3F2 D 2L D' 3F2 D 2L' D'",
   "3F2 D 2L' D' 3F2 D 2L D'",
   "3F' D 2B D' 3F D 2B' D'",
   "F 3D F 2U F' 3D' F 2U' F2",
   "F 3D F 2U2 F' 3D' F 2U2 F2",
   "F 3D F 2U' F' 3D' F 2U F2",
   "F 3D F 2R2 F' 3D' F 2R2 F2",
   "F 3D2 F 2U F' 3D2 F 2U' F2",
   "F 3D2 F 2U2 F' 3D2 F 2U2 F2",
   "F 3D2 F 2U' F' 3D2 F 2U F2",
   "F 3D2 F 2R F' 3D2 F 2R' F2",
   "F 3D2 F 2R' F' 3D2 F 2R F2",
   "F 3D' F 2U F' 3D F 2U' F2",
   "F 3D' F 2U2 F' 3D F 2U2 F2",
   "F 3D' F 2U' F' 3D F 2U F2",
   "F 3D' F 2R2 F' 3D F 2R2 F2",
   "F2 2D F' 3U F 2D' F' 3U' F'",
   "F2 2D F' 3U2 F 2D' F' 3U2 F'",
   "F2 2D F' 3U' F 2D' F' 3U F'",
   "F2 2D2 F' 3U F 2D2 F' 3U' F'",
   "F2 2D2 F' 3U2 F 2D2 F' 3U2 F'",
   "F2 2D F' 3L2 F 2D' F' 3L2 F'",
   "F2 2D2 F' 3U' F 2D2 F' 3U F'",
   "F2 2D' F' 3U F 2D F' 3U' F'",
   "F2 2D' F' 3U2 F 2D F' 3U2 F'",
   "F2 2D' F' 3U' F 2D F' 3U F'",
   "F2 2D' F' 3L2 F 2D F' 3L2 F'",
   "F2 3D F 2U F' 3D' F 2U' F",
   "F2 3D F 2U2 F' 3D' F 2U2 F",
   "F2 3D F 2U' F' 3D' F 2U F",
   "F2 3D F 2R2 F' 3D' F 2R2 F",
   "F2 3D2 F 2U F' 3D2 F 2U' F",
   "F2 3D2 F 2U2 F' 3D2 F 2U2 F",
   "F2 3D2 F 2U' F' 3D2 F 2U F",
   "F2 3D' F 2U F' 3D F 2U' F",
   "F2 3D' F 2U2 F' 3D F 2U2 F",
   "F2 3D' F 2U' F' 3D F 2U F",
   "F2 3D' F 2R2 F' 3D F 2R2 F",
   "F2 3D2 F 2R F' 3D2 F 2R' F",
   "F2 3D2 F 2R' F' 3D2 F 2R F",
   "F' 2D2 F' 3U F 2D2 F' 3U' F2",
   "F' 2D2 F' 3U2 F 2D2 F' 3U2 F2",
   "F' 2D2 F' 3U' F 2D2 F' 3U F2",
   "F' 2D F' 3U F 2D' F' 3U' F2",
   "F' 2D F' 3U2 F 2D' F' 3U2 F2",
   "F' 2D F' 3U' F 2D' F' 3U F2",
   "F' 2D2 F' 3L F 2D2 F' 3L' F2",
   "F' 2D2 F' 3L' F 2D2 F' 3L F2",
   "F' 2D' F' 3U F 2D F' 3U' F2",
   "F' 2D' F' 3U2 F 2D F' 3U2 F2",
   "F' 2D' F' 3U' F 2D F' 3U F2",
   "F 2F' U' 3L U 2F U' 3L' U F'",
   "F 3F' R 2D' R' 3F R 2D R' F'",
   "F 3B D 2R D' 3B' D 2R' D' F'",
   "F 2B U' 3R' U 2B' U' 3R U F'",
   "B F 2D F' 3L F 2D' F' 3L' B'",
   "B F 2D F' 3L' F 2D' F' 3L B'",
   "B F 2D' F' 3L F 2D F' 3L' B'",
   "B F 2D' F' 3L' F 2D F' 3L B'",
   "B2 F 2D F' 3L F 2D' F' 3L' B2",
   "B2 F 2D F' 3L2 F 2D' F' 3L2 B2",
   "B2 F 2D' F' 3L F 2D F' 3L' B2",
   "B2 F 2D F' 3L' F 2D' F' 3L B2",
   "B2 F 2D' F' 3L2 F 2D F' 3L2 B2",
   "B2 F 2D' F' 3L' F 2D F' 3L B2",
   "F B' 3D B 2L B' 3D' B 2L' F'",
   "F B' 3D B 2L' B' 3D' B 2L F'",
   "F B' 3D' B 2L B' 3D B 2L' F'",
   "F B' 3D' B 2L' B' 3D B 2L F'",
   "F D 2R D' 3B D 2R' D' 3B' F'",
   "F D 2L' D' 3F' D 2L D' 3F F'",
   "F D 2L D' 3R' D 2L' D' 3R F'",
   "F D 2R' D' 3L D 2R D' 3L' F'",
   "F D' 3L' D 2B D' 3L D 2B' F'",
   "F D' 3R D 2F' D' 3R' D 2F F'",
   "F D' 3R' D 2L D' 3R D 2L' F'",
   "F D' 3L D 2R' D' 3L' D 2R F'",
   "F 2D R' 3U' R 2D' R' 3U R F'",
   "F 2D' L' 3U L 2D L' 3U' L F'",
   "F 3D R 2U' R' 3D' R 2U R' F'",
   "F 3D' L 2U L' 3D L 2U' L' F'",
   "F2 2F' U' 3L U 2F U' 3L' U F2",
   "F2 3F' U 2R' U' 3F U 2R U' F2",
   "F2 3B L 2D L' 3B' L 2D' L' F2",
   "F2 2B D' 3L' D 2B' D' 3L D F2",
   "F2 B' 3D' B 2L B' 3D B 2L' F2",
   "F2 B' 3D' B 2L2 B' 3D B 2L2 F2",
   "F2 B' 3D' B 2L' B' 3D B 2L F2",
   "F2 B' 3D B 2L B' 3D' B 2L' F2",
   "F2 B' 3D B 2L2 B' 3D' B 2L2 F2",
   "F2 B' 3D B 2L' B' 3D' B 2L F2",
   "F2 D 2L' D' 3F' D 2L D' 3F F2",
   "F2 D 2R D' 3B D 2R' D' 3B' F2",
   "F2 D 2L D' 3R2 D 2L' D' 3R2 F2",
   "F2 D 2R' D' 3L2 D 2R D' 3L2 F2",
   "F2 D' 3L' D 2B D' 3L D 2B' F2",
   "F2 D' 3R' D 2L2 D' 3R D 2L2 F2",
   "F2 D' 3L D 2R2 D' 3L' D 2R2 F2",
   "F2 D' 3R D 2F' D' 3R' D 2F F2",
   "F' D 2L D' 3R' D 2L' D' 3R F",
   "F' D 2R' D' 3L D 2R D' 3L' F",
   "F' D' 3L D 2R' D' 3L' D 2R F",
   "F' D' 3R' D 2L D' 3R D 2L' F",
   "F' 2D R' 3U' R 2D' R' 3U R F",
   "F' 2D' L' 3U L 2D L' 3U' L F",
   "F' 3D R 2U' R' 3D' R 2U R' F",
   "F' 3D' L 2U L' 3D L 2U' L' F",
   "F D2 B 2D' B' 3R B 2D B' 3R' D2 F'",
   "F D2 B' 3D' B 2L' B' 3D B 2L D2 F'",
   "F D2 B' 3U B 2R B' 3U' B 2R' D2 F'",
   "F D2 B 2U B' 3L' B 2U' B' 3L D2 F'",
   "F D2 B 2L' B' 3D' B 2L B' 3D D2 F'",
   "F D2 3L' B 2U B' 3L B 2U' B' D2 F'",
   "F D2 3R B 2D' B' 3R' B 2D B' D2 F'",
   "F D2 B 2R B' 3U B 2R' B' 3U' D2 F'",
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
void scramble(xorbit& x) {
   oclear(x) ;
   shuffle(x.a, x.a+24, rng) ;
}
struct mv {
   int dep ;
   short face, twist ;
} ;
vector<mv> moveset ;
void uclear(xorbit& x) {
   for (int i=0; i<24; i++)
      x.a[i] = i ;
}
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
    *   applies its 3-cycles on the stored orbit positions; fold the
    *   rotations into the orbits so stored and physical positions agree
    *   (as brobdicube's normalizefacerots does).  Without this, -i with a
    *   scramble that leaves a face turned (e.g. "2F R") gave a wrong solution.
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
   if (a.face == b.face && a.dep == b.dep) {
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
 *   We retain the sequence of previous face moves so we can try to
 *   cancel them.
 */
vector<mv> prevmvs ;
void trycancellation(mv &m) {
   if (prevmvs.size() == 0) {
      prevmvs.push_back(m) ;
      return ;
   }
   /*
    *   We don't cancel non-face moves at the moment.  We'd have to
    *   be a little smarter in doblock() if we did, because we are
    *   looking at the algorithm template there, not the actual moves
    *   that would be executed.
    */
   if (m.dep > 1) {
      prevmvs.clear() ;
      return ;
   }
   mv merged ;
   int r = trymerge(prevmvs[prevmvs.size()-1], m, merged) ;
   if (r == 2) { // no cancellation
      prevmvs.push_back(m) ;
   } else if (r == 1) {
      cancellations++ ;
      prevmvs[prevmvs.size()-1] = merged ;
   } else {
      cancellations += 2 ;
      prevmvs.pop_back() ;
   }
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
   trycancellation(mv) ;
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
const int MAXALGKEY = 24*24*24 ;
struct blockalgo {
   int encalgkey() const {
      return (a * 24 + b) * 24 + c ;
   }
   vector<mv> mvs ;
   int a, b, c ;
   void show() const {
      for (auto m: mvs) {
         cout << " " ;
         showmv(m) ;
      }
   }
} blockalgs[MAXALGKEY] ;
blockalgo &findblockalgo(int a, int b, int c) {
   while (a > b || a > c) {
      swap(a, b) ;
      swap(b, c) ;
   }
   int k = (a * 24 + b) * 24 + c ;
   if (blockalgs[k].mvs.size() == 0) {
      cerr << "Args are " << a << " " << b << " " << c << endl ;
      error("! tried to find a non-existent alg") ;
   }
   return blockalgs[k] ;
}
void stash(blockalgo alg, int seqno) {
   while (alg.a > alg.b || alg.a > alg.c) {
      swap(alg.a, alg.b) ;
      swap(alg.b, alg.c) ;
   }
   int k = alg.encalgkey() ;
   if (blockalgs[k].mvs.size() == 0)
      blockalgs[alg.encalgkey()] = alg ;
}
void loadalgs() {
   const int nalgs = sizeof(triswapalgs)/sizeof(triswapalgs[0]) ; 
   xcube cube(4, 5, 'U') ; // get the special version with 0..23
   vector<mv> mappedmoves ;
   for (int i=0; i<nalgs; i++) {
      auto mvs = parse_moves(triswapalgs[i]) ;
      for (int m=0; m<24; m++) {
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
               if (j != 0 && k != 1) {
                  for (int p=0; p<24; p++)
                     if (cube.rows[j][k].a[p] != p) {
                        cout << "Failed alg " << i << " at " << j << " " << k << " " << p << endl ;
                        error("! algorithm hurts other orbits?") ;
                     }
               }
         int a = -1 ;
         for (int i=0; i<24; i++)
            if (cube.rows[0][1].a[i] != i) {
               a = i ;
               break ;
            }
         if (a < 0)
            error("! algorithm didn't seem to modify the correct orbit") ;
         int b = cube.rows[0][1].a[a] ;
         int c = cube.rows[0][1].a[b] ;
         if (cube.rows[0][1].a[c] != a)
            error("! doesn't look like a triple") ;
         stash({mappedmoves, a, b, c}, i) ;
      }
   }
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
ll doblock(xcube &cube, blockalgo &alg, vector<int> &ys, vector<int> &xs) {
#if WRITEFRAMES
   if (writemoveinc && runningmovecount >= writemovetarget)
      writeframe(cube) ;
#endif
   ll oldcancellations = cancellations ;
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
   } else {
      trycancellation(alg.mvs[0]) ;
      trycancellation(alg.mvs[1]) ;
      prevmvs.clear() ;
      trycancellation(alg.mvs[alg.mvs.size()-2]) ;
      trycancellation(alg.mvs[alg.mvs.size()-1]) ;
   }
   ll mvs = alg.mvs.size() + 2 * (ys.size() + xs.size() - 2) -
                                         (cancellations - oldcancellations) ;
   runningmovecount += mvs ;
   return mvs ;
}
void quick3swap(xorbit &xo, int a, int c, int b) {
   int t = xo.a[a] ;
   xo.a[a] = xo.a[c] ;
   xo.a[c] = xo.a[b] ;
   xo.a[b] = t ;
}
struct stats {
   ll hits, moves ;
   void showstats(int id, double denom) {
      if (hits)
         cout << "Phase " << id << " p " << hits/denom << " mvs " << moves << endl ;
   }
} ;
vector<stats> byphase ;
int extrapolate = 0 ;
#ifdef OLDCODE
#else
/*
 *   This is the 24/12/12 network.
 */
struct net241212 {
   static const int STAGE = 32 ;
   static const int n = 24 ;
   static const int constexpr sort[] = {
      3, 23, 15, 5, 12, 15, 0, 21, 12, 9, 14, 15, 8, 17, 15, 11, 17,
      23, 1, 17, 18, 7, 17, 13, 10, 15, 16, 6, 12, 20, 1, 14, 13, 4,
      17, 14, 2, 17, 21, 10, 22, 14, 11, 14, 19, 11, 12, 17, 11, 13,
      16, 8, 21, 20, 6, 19, 12, 8, 22, 17, 8, 13, 19, 2, 14, 15, 9,
      12, 15, 9, 22, 18, 11, 19, 18, 6, 22, 14, 10, 14, 18, 11, 22,
      12, 6, 15, 21, 4, 13, 21, 1, 12, 17, 10, 17, 13
/*
      3, 19, 15, 5, 19, 12, 0, 21, 12, 9, 21, 14, 8, 17, 15, 11, 17,
      23, 1, 17, 18, 7, 13, 12, 6, 15, 16, 6, 12, 20, 1, 12, 13, 4,
      17, 14, 2, 17, 21, 10, 22, 14, 11, 14, 19, 11, 12, 17, 8, 13,
      21, 6, 19, 15, 8, 22, 17, 8, 21, 19, 2, 22, 15, 9, 12, 15, 9,
      22, 18, 11, 19, 22, 0, 14, 22, 6, 20, 13, 11, 22, 12, 6, 15, 12,
      6, 18, 21, 2, 21, 14, 7, 18, 14, 8, 15, 18, 1, 23, 15
 */
/*
      3, 12, 15, 5, 12, 15, 0, 21, 12, 9, 21, 14, 8, 17, 15, 11, 17,
      23, 1, 17, 18, 7, 13, 12, 6, 15, 16, 6, 12, 20, 1, 12, 13, 4,
      17, 14, 2, 17, 21, 10, 22, 14, 11, 14, 19, 11, 12, 17, 8, 13,
      21, 6, 19, 15, 8, 22, 17, 8, 21, 19, 2, 22, 15, 9, 12, 15, 9,
      22, 18, 11, 19, 18, 0, 22, 14, 6, 20, 13, 11, 22, 12, 6, 15, 12,
      6, 18, 21, 2, 21, 14, 2, 7, 17, 1, 22, 17, 1, 7, 9
 */
/*
      4, 21, 5, 6, 7, 18, 9, 17, 16, 5, 13, 12, 2, 20, 23, 0, 1, 14,
      10, 20, 21, 0, 3, 17, 5, 6, 19, 1, 6, 7, 8, 15, 9, 9, 10, 19, 0,
      13, 12, 8, 11, 22, 3, 11, 10, 0, 10, 9, 8, 23, 20, 5, 15, 14,
      14, 21, 15, 14, 22, 23, 4, 13, 7, 8, 14, 13, 8, 17, 18, 0, 18,
      19, 12, 13, 16, 5, 16, 19, 5, 23, 22, 4, 16, 5, 0, 3, 5, 0, 4,
      1, 0, 12, 15, 12, 23, 20, 12, 15, 18, 9, 12, 17
 */
/*
      2, 14, 15, 1, 2, 23, 6, 22, 23, 4, 16, 19, 8, 11, 18, 10, 11,
      12, 1, 23, 20, 5, 23, 20, 9, 10, 17, 8, 9, 21, 1, 16, 19, 8, 14,
      15, 5, 9, 10, 19, 23, 22, 8, 9, 22, 4, 5, 11, 0, 16, 1, 7, 14,
      13, 0, 3, 8, 3, 6, 5, 10, 11, 22, 4, 10, 7, 0, 14, 13, 5, 15,
      14, 0, 3, 20, 0, 11, 1, 1, 7, 2, 6, 7, 11, 2, 3, 13, 2, 16, 17,
      5, 16, 6, 0, 9, 10, 0, 3, 16, 12, 13, 16, 0, 18, 19
 */
/*
      5, 6, 22, 10, 19, 16, 8, 11, 23, 2, 3, 17, 0, 20, 1, 5, 20, 21,
      4, 7, 12, 2, 19, 3, 9, 15, 12, 0, 15, 3, 1, 14, 13, 4, 18, 7, 8,
      11, 20, 5, 23, 6, 10, 13, 12, 12, 21, 20, 2, 3, 14, 1, 15, 12,
      7, 19, 18, 0, 23, 3, 5, 19, 18, 19, 20, 23, 12, 18, 19, 8, 14,
      13, 14, 22, 23, 11, 13, 12, 16, 22, 23, 0, 17, 16, 1, 2, 18, 0,
      13, 3, 8, 18, 11, 13, 18, 17, 2, 14, 13, 13, 22, 21, 2, 15, 12
 */
/*
      0, 13, 12, 8, 22, 11, 4, 15, 14, 6, 7, 13, 1, 2, 18, 2, 21, 22,
      0, 1, 14, 3, 23, 22, 13, 21, 22, 9, 19, 16, 8, 17, 11, 8, 11,
      15, 6, 15, 7, 10, 18, 19, 6, 20, 21, 12, 13, 18, 14, 22, 23, 6,
      18, 19, 1, 2, 7, 4, 23, 5, 5, 6, 20, 4, 5, 16, 1, 8, 9, 6, 10,
      7, 0, 19, 16, 0, 1, 15, 5, 11, 10, 10, 13, 14, 0, 6, 5, 16, 20,
      17, 0, 10, 3, 2, 3, 16, 9, 10, 20, 14, 15, 17, 0, 15, 3, 0, 1, 23
 */
   } ;
/*
 *   24 choose 12 is only 2,704,156, while 2^24 is 16,777,216.
 *   Thus, we pack the combinations to keep arrays reasonably dense.
 */
   static const int CASES = 2704156 ;
   int unpack[CASES] ;
   int packhi[1<<12], packlo[1<<12] ;
   static const int halfn = 12 ;
   static const int lowmask = (1<<halfn) - 1 ;
   static const int goal = CASES ;
/*
 *   This part can be slow since we only do it once.
 */
   net241212() {
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
   int swapbits(int v, int a, int b, int c) {
      int va = (v >> a) & 1 ;
      int vb = (v >> b) & 1 ;
      int vc = (v >> c) & 1 ;
      int xorv = ((va ^ vb) << a) + ((vb ^ vc) << b) + ((vc ^ va) << c) ;
      return v ^ xorv ;
   }
/*
 *   Plan how to execute this based on the triples given above.
 *   We iterate on prefixes of the triples, finding the best
 *   solution for every reachable state on each prefix.  We use
 *   -1 to mean unreachable for now.  Note that we are searching
 *   from solved, so to solve a state we need to reverse the
 *   solutions and triples.
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
               int k = swapbits(unpack[j], sort[3*i], sort[3*i+1], sort[3*i+2]) ;
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
         int cnts[STAGE] ;
         for (int i=0; i<STAGE; i++)
            cnts[i] = 0 ;
         for (int i=0; i<CASES; i++) {
            if (plans[i] < 0)
               cout << "Could not get to " << i << endl ;
            for (int j=0; j<STAGE; j++)
               if ((plans[i]>>j) & 1)
                  cnts[j]++ ;
         }
         for (int i=0; i<STAGE; i++)
            cout << "net241212: " << i << " " << cnts[i]/(double)CASES << endl ;
      }
   }
   ll getplan(xorbit &orb) const {
      ll k = 0 ;
      for (int i=0; i<24; i++)
         if (orb.a[i] < 3)
            k |= 1<<i ;
      return plans[packhi[k>>halfn]+packlo[k&lowmask]] ;
   }
   void doplan(ll plan, xorbit &orb) {
      for (int pi=STAGE-1; pi>=0; pi--) {
         if (((plan >> pi) & 1) == 0)
            continue ;
         int a = sort[3*pi] ;
         int b = sort[3*pi+2] ;
         int c = sort[3*pi+1] ;
         quick3swap(orb, a, b, c) ;
      }
   }
} ;
/*
 *   This is the 12/4/4/4 network.
 */
struct net12444 {
   static const int STAGE = 20 ;
   static const int constexpr sort[] = {
/*
      4, 5, 9, 2, 8, 11, 7, 11, 8, 6, 10, 11, 2, 5, 4, 1, 11, 10, 3,
      8, 11, 2, 3, 6, 0, 1, 8, 5, 10, 11, 4, 8, 11, 0, 9, 10, 6, 10,
      11, 2, 6, 5, 0, 6, 7, 6, 10, 9, 1, 2, 11, 7, 10, 11, 0, 3, 4, 3, 8, 9
 */
      0, 4, 5, 1, 7, 2, 2, 3, 6, 0, 10, 11, 2, 3, 8, 4, 8, 9, 2, 9,
      10, 0, 1, 8, 5, 9, 8, 6, 7, 9, 4, 10, 11, 0, 8, 9, 1, 2, 5, 2,
      3, 6, 6, 11, 10, 1, 7, 4, 1, 6, 2, 0, 7, 6, 2, 3, 11, 0, 1, 11
   } ;
   static const int n = 12 ;
   static const int CASES = 34650 ;
   int unpack[CASES] ;
   int packhi[1<<12], packlo[1<<12] ;
   static const int halfn = 12 ;
   static const int lowmask = (1<<12)-1 ;
   static const int goal = CASES ;
/*
 *   This part can be slow since we only do it once.
 */
   net12444() {
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
      for (int i=0; (i>>n) == 0; i++)
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
   int swapbits(int v, int a, int b, int c) {
      int va = (v >> (2 * a)) & 3 ;
      int vb = (v >> (2 * b)) & 3 ;
      int vc = (v >> (2 * c)) & 3 ;
      int xorv = ((va ^ vb) << (2 * a)) + ((vb ^ vc) << (2 * b)) + ((vc ^ va) << (2 * c)) ;
      return v ^ xorv ;
   }
/*
 *   Plan how to execute this based on the triples given above.
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
               int k = swapbits(unpack[j], sort[3*i], sort[3*i+1], sort[3*i+2]) ;
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
         int cnts[STAGE] ;
         for (int i=0; i<STAGE; i++)
            cnts[i] = 0 ;
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
   int getplan(xorbit &orb, int half) const {
      ll k = 0 ;
      int off = (orb.a[half] < 3 ? 2 : 5) ;
      for (int i=11; i>=0; i--)
         k = 4 * k + off - orb.a[half+i] ;
      return plans[packhi[k>>halfn]+packlo[k&lowmask]] ;
   }
   void doplan(int plan, xorbit &orb, int half) {
      for (int pi=STAGE-1; pi>=0; pi--) {
         if (((plan >> pi) & 1) == 0)
            continue ;
         int a = sort[3*pi] + half ;
         int b = sort[3*pi+2] + half ;
         int c = sort[3*pi+1] + half ;
         quick3swap(orb, a, b, c) ;
      }
   }
} ;
struct net241212 outer ;
struct net12444 inner ;
ll solve(xcube &cube, int width, int phasewidth) {
   byphase.resize(outer.STAGE + 2 * inner.STAGE) ;
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
            movemasks[ii][j] = outer.getplan(cube.rows[i+ii][j]) ;
            if (fast && !writemoveinc)
               outer.doplan(movemasks[ii][j], cube.rows[i+ii][j]) ;
         }
      int phasebase = 0 ;
      for (int pi=outer.STAGE-1; pi>=0; pi--, phasebase++) {
         int a = outer.sort[3*pi] ;
         int b = outer.sort[3*pi+2] ;
         int c = outer.sort[3*pi+1] ;
         auto &alg = findblockalgo(a, b, c) ;
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
                        outer.doplan(1LL<<pi, cube.rows[y][x]) ;
               byphase[phasebase].moves += mvs ;
               r += mvs ;
            }
         }
      }
      for (int half=0; half<24; half += 12) {
         for (int ii=0; ii<pw; ii++)
            for (int j=0; j<cube.Mx; j++) {
               movemasks[ii][j] = inner.getplan(cube.rows[i+ii][j], half) ;
               if (fast && !writemoveinc)
                  inner.doplan(movemasks[ii][j], cube.rows[i+ii][j], half) ;
            }
         for (int pi=inner.STAGE-1; pi>=0; pi--, phasebase++) {
            int a = inner.sort[3*pi] + half ;
            int b = inner.sort[3*pi+2] + half ;
            int c = inner.sort[3*pi+1] + half ;
            auto &alg = findblockalgo(a, b, c) ;
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
                           inner.doplan(1LL<<pi, cube.rows[y][x], half) ;
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
#endif
int stats = 0 ;
ll dosolve(ll N, int width, int phasewidth) {
   prevmvs.clear() ;
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
   cout << "N " << N << " width " << width
        << " phasewidth " << phasewidth << " cnt " << at << " " ;
   mvstat.report() ; cout << " " ;
   cancelstat.avreport() ; cout << " " ;
   blockstat.avreport() ; cout << endl;
}
ll solvecount = 1 ;
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
      "-d:     show some internal information on the strategy." << endl <<
      "-i      read scramble moves from stdin." << endl <<
      "-m      plan each orbit state with the fewest stages (default: the\n"
      "        first plan found, which leaves the last stages rarely used)." << endl <<
      "-M n    do this many solves and report aggregate stats." << endl <<
      "-o      write solution to stdout (on lines starting with a space)." << endl <<
      "-p n    set the phase chunking width." << endl <<
      "-s:     actually (slowly) simulate moves; for testing." << endl <<
      "-w n    set the strip width for 2d optimization." << endl <<
      "-x:     extrapolate statistics from a single strip." << endl <<
      "-z n    seed the random number generator (default: random)." << endl <<
#if WRITEFRAMES
      "-v n    write a png file of the cube state every n moves." << endl <<
#endif
      endl ;
   exit(0) ;
}
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
case 'z':
         argc-- ;
         argv++ ;
         rng.seed(atoll(argv[0])) ;
         break ;
case 'M':
         argc-- ;
         argv++ ;
         solvecount = atoll(argv[0]) ;
         break ;
#if WRITEFRAMES
case 'v':
         argc-- ;
         argv++ ;
         writemoveinc = atoll(argv[0]) ;
         break ;
#endif
case 'm':
         minstages = 1 ;
         break ;
case 'o':
         writemoves = 1 ;
         break ;
case 'i':
         readmoves = 1 ;
         break ;
default:
         error("! option not understood") ;
      }
   }
   inner.plan() ;
   outer.plan() ;
   if (!fast && extrapolate)
      error("! cannot go slow and extrapolate at the same time") ;
   expandrotations() ;
   loadalgs() ;
   if (argc < 2)
      usage() ;
   if (argc > 2)
      error("! extra arguments given on command line") ;
   N = atoll(argv[1]) ;
   ll My = (N - 2) >> 1 ;
   ll Mx = (N - 2) - My ;
   if (phasewidth < width)
      phasewidth = width ;
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
               byphase[i].showstats(i, My*phasewidth) ;
         lll lll_r = (lll)r * My / phasewidth ;
 cout << "r " << r << " My " << My << " width " << width << " lll_r " << lll_r << endl ;
         lll lll_cancellations = (lll)cancellations * My / width ;
         lll lll_blocks = (lll)blocks * My / phasewidth ;
         cout << "N " << N << " width " << phasewidth
              << " moves " << lll_r << " cancel " << lll_cancellations
              << " blocks " << lll_blocks << endl ;
      } else {
         if (stats)
            for (int i=0; i<(int)byphase.size(); i++)
               byphase[i].showstats(i, Mx*My) ;
         cout << "N " << N << " width " << width
              << " moves " << r << " cancel " << cancellations
              << " blocks " << blocks << endl ;
      }
   } else {
      fullreport(solvecount) ;
   }
}
