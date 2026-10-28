#include <algorithm>
#include <assert.h>
#include <strings.h>
#include <iostream>
#include <random>
#include <vector>
using namespace std ;
using ll = long long ;
using lll = __int128_t ;
using uchar = unsigned char ;
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
   "2F R' 3B' R 2F' R' 3B R",
   "2F' U' 3B U 2F U' 3B' U",
   "3F R 2B' R' 3F' R 2B R'",
   "3F' U 2B U' 3F U 2B' U'",
   "U 2F' U' 3B U 2F U' 3B'",
   "U 2B U' 3F' U 2B' U' 3F",
   "U' 3F' U 2B U' 3F U 2B'",
   "U' 3B U 2F' U' 3B' U 2F",
   "U 3F' U 2B U' 3F U 2B' U2",
   "U 3B U 2F' U' 3B' U 2F U2",
   "U2 2F' U' 3B U 2F U' 3B' U'",
   "U2 3F' U 2B U' 3F U 2B' U",
   "U2 3B U 2F' U' 3B' U 2F U",
   "U2 2B U' 3F' U 2B' U' 3F U'",
   "U' 2F' U' 3B U 2F U' 3B' U2",
   "U' 2B U' 3F' U 2B' U' 3F U2",
   "U 2F R' 3B' R 2F' R' 3B R U'",
   "U 3F R 2B' R' 3F' R 2B R' U'",
   "U 3B' R 2F R' 3B R 2F' R' U'",
   "U 2B' R' 3F R 2B R' 3F' R U'",
   "U R 2B' R' 3F R 2B R' 3F' U'",
   "U R 2F R' 3B' R 2F' R' 3B U'",
   "U R' 3F R 2B' R' 3F' R 2B U'",
   "U R' 3B' R 2F R' 3B R 2F' U'",
   "U' 2F R' 3B' R 2F' R' 3B R U",
   "U' 3F R 2B' R' 3F' R 2B R' U",
   "U' 3B' R 2F R' 3B R 2F' R' U",
   "U' 2B' R' 3F R 2B R' 3F' R U",
   "U' R 2F R' 3B' R 2F' R' 3B U",
   "U' R 2B' R' 3F R 2B R' 3F' U",
   "U' R' 3F R 2B' R' 3F' R 2B U",
   "U' R' 3B' R 2F R' 3B R 2F' U",
   "2F2 D' 3B2 D 2F2 D' 3B2 D",
   "3F2 D 2B2 D' 3F2 D 2B2 D'",
   "D 2F2 D' 3B2 D 2F2 D' 3B2",
   "D' 3F2 D 2B2 D' 3F2 D 2B2",
   "D 3F2 D 2B2 D' 3F2 D 2B2 D2",
   "D2 2F2 D' 3B2 D 2F2 D' 3B2 D'",
   "D2 3F2 D 2B2 D' 3F2 D 2B2 D",
   "D' 2F2 D' 3B2 D 2F2 D' 3B2 D2",
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
struct xcube {
   xcube(int Mx_, int My_) : Mx(Mx_), My(My_), rows(My) {
      for (int i=0; i<6; i++)
         facerots[i] = 0 ;
      // central faces, only for odd cubes
      for (int i=0; i<6; i++)
         ccs[i] = i ;
      if (!fast) {
         for (int i=0; i<My; i++) {
            rows[i].resize(Mx) ;
            for (auto &xo: rows[i])
               scramble(xo) ;
         }
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
      if (!fast)
         return ;
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
      if (!fast)
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
} ;
struct mv {
   int dep ;
   short face, twist ;
} ;
void uclear(xorbit& x) {
   for (int i=0; i<24; i++)
      x.a[i] = i ;
}
void border() {
   cout << "  " ;
}
void xcube::show() const {
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
 *   cancel them (taken from the later brobdicube/permnet72 code).  The
 *   old version also merged slice moves, but in fast mode doblock only
 *   sees the algorithm template, whose slice depths are placeholders, so
 *   it counted cancellations between different layers.
 */
vector<mv> prevmvs ;
void trycancellation(mv &m) {
   if (prevmvs.size() == 0) {
      prevmvs.push_back(m) ;
      return ;
   }
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
const int MAXALGKEY = 6*6*4*8*8 ;
struct blockalgo {
   int encalgkey() const {
      return (((f1 * 6 + f2) * 4 + a) * 8 + b) * 8 + c ;
   }
   vector<mv> mvs ;
   int f1, f2, a, b, c ;
   int offa, offb, offc ;
} blockalgs[MAXALGKEY] ;
blockalgo &findblockalgo(int f1, int f2, int a, int b, int c) {
   while (a > b || a > c) {
      swap(a, b) ;
      swap(b, c) ;
   }
   int k = (((f1 * 6 + f2) * 4 + a) * 8 + b) * 8 + c ; 
   if (blockalgs[k].mvs.size() == 0) {
      cerr << "Args are " << f1 << " " << f2 << " " << a << " " << b << " " << c << endl ;
      error("! tried to find a non-existent alg") ;
   }
   return blockalgs[k] ;
}
void stash(blockalgo alg) {
   while (alg.a > alg.b || alg.a > alg.c) {
      swap(alg.a, alg.b) ;
      swap(alg.b, alg.c) ;
      swap(alg.offa, alg.offb) ;
      swap(alg.offb, alg.offc) ;
   }
   int k = alg.encalgkey() ;
   if (blockalgs[k].mvs.size())
      error("! stashing over something that exists?") ;
   blockalgs[alg.encalgkey()] = alg ;
}
void loadalgs() {
   const int nalgs = sizeof(triswapalgs)/sizeof(triswapalgs[0]) ; 
   xcube cube(4, 5) ;
   for (int i=0; i<cube.My; i++) {
      cube.getrow(i) ;
      for (int j=0; j<cube.Mx; j++)
         uclear(cube.rows[i][j]) ;
   }
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
               if (j != 0 || k != 1) {
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
         int f1 = a>>2 ;
         int f2 = b>>2 ;
         int f3 = c>>2 ;
         if (f1 != f2 && f2 != f3 && f1 != f3)
            error("! should be only two faces in an alg") ;
         int f4 = f2 ;
         if (f4 == f1)
            f4 = f3 ;
         if (f4 ==  f1)
            error("! should be at least two faces in an alg") ;
         stash({mappedmoves, f1, f4, a & 3,
                       (f2 == f4) * 4 + (b & 3), (f3 == f4) * 4 + (c & 3),
                       a, b, c}) ;
         stash({mappedmoves, f4, f1, 4 + (a & 3),
                       (f2 == f1) * 4 + (b & 3), (f3 == f1) * 4 + (c & 3),
                       a, b, c}) ;
      }
   }
}
ll blocks = 0 ;
ll doblock(xcube &cube, blockalgo &alg, vector<int> &ys, vector<int> &xs,
           int suppress=0) {
   ll oldcancellations = cancellations ;
   blocks++ ;
   if (fast) {
      if (!suppress) {
         int a = alg.offa ;
         int b = alg.offb ;
         int c = alg.offc ;
         for (auto i: ys)
            for (auto j: xs) {
               auto &orb = cube.rows[i][j] ;
               auto t = orb.a[a] ;
               orb.a[a] = orb.a[b] ;
               orb.a[b] = orb.a[c] ;
               orb.a[c] = t ;
            }
      }
      trycancellation(alg.mvs[0]) ;
      trycancellation(alg.mvs[1]) ;
      prevmvs.clear() ;
      trycancellation(alg.mvs[alg.mvs.size()-2]) ;
      trycancellation(alg.mvs[alg.mvs.size()-1]) ;
   } else {
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
   return alg.mvs.size() + 2 * (ys.size() + xs.size() - 2) -
                                         (cancellations - oldcancellations) ;
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
/*
 *   This is the 6-element sorting network; every pair of values
 *   is one element.  The values must be given with lower value
 *   first.
 */
int sort12[] = {
   0, 3, 1, 4, 2, 5, 0, 2, 1, 2, 0, 1, 3, 5, 4, 5, 3, 4, 2, 3, 1, 2, 3, 4
} ;
/*
 *   This is the 8-choose-4 permutation network; every triple of
 *   values is one three-swap.
 */
int sort8_4[] = {
   1, 2, 4, 1, 4, 7, 1, 4, 2, 1, 2, 7, 0, 7, 4, 1, 2, 6, 2, 4, 5, 0, 3, 4
} ;
/*
 *   For the 84 sort, we actually need a plan; this builds that plan.
 *   We need a plan because the network is a permutation network that
 *   uses global information to sort, rather than a sorting network
 *   that can operate with just local information.
 *
 *   We represent the 8-choose-4 value (just zeros and ones) with an
 *   8-bit integer.  This code swaps bits around according to the
 *   3-cycle given in (a,b,c).
 */
int swap3bits(int i, int ba, int bb, int bc) {
   int a = i >> ba ;
   int b = i >> bb ;
   int c = i >> bc ;
   return i ^ ((((a ^ b) & 1) << ba) + (((b ^ c) & 1) << bb) +
               (((c ^ a) & 1) << bc)) ;
}
/*
 *   This actually builds the plan.  We have 8 3-cycles; each can
 *   either be applied or not applied.  That's 256 possibilities.
 *   The result array at index i will contain the result of
 *   applying the permutation network backwards.  The least significant
 *   bit of i tells us if we applied the last 3-cycle; the next
 *   least tells us if we applied the pentultimate 3-cycle, and so
 *   on.
 *
 *   We build a cost array, which is the cheapest way (fewest
 *   3-cycles) to attain a given value.  We also store a bitmask in
 *   moveset that tells us what elements of the permutation network
 *   should be applied to sort that element.
 *
 *   This variant attempts to minimize use of one of the perms, which
 *   may increase the cost.
 */
int cost[256] ;
int result[256] ;
int moveset[256] ;
void plan84() {
   for (int i=0; i<256; i++)
      cost[i] = 255 ;
   for (int i=0; i<256; i++)
      result[i] = -1 ;
   vector<vector<int>> opts(256) ;
   result[0] = 0xf0 ;
   cost[result[0]] = 0 ;
   int sz = 1 ;
   opts[result[0]].push_back(0) ;
   for (int trip=21; trip>=0; trip -= 3, sz *= 2) {
      for (int i=0; i<sz; i++) {
         int k = swap3bits(result[i], sort8_4[trip], sort8_4[trip+1],
                           sort8_4[trip+2]) ;
         result[sz+i] = k ;
         opts[k].push_back(sz+i) ;
      }
   }
   vector<int> minneeded(8, 0) ;
   for (int i=0; i<256; i++)
      if (opts[i].size()) {
         int x = 255 ;
         for (auto v: opts[i])
            x &= v ;
         for (int j=0; j<8; j++)
            if ((x >> j) & 1)
               minneeded[j]++ ;
      }
   int loi = 0 ;
   for (int i=1; i<8; i++)
      if (minneeded[i] < minneeded[loi])
         loi = i  ;
   cout << "For sort84 found a swap that only requires " << minneeded[loi] << endl ;
   for (int i=0; i<256; i++) {
      if (opts[i].size()) {
// try to exclude those patterns that use bit loi.
         moveset[i] = -1 ;
         for (auto v: opts[i])
            if ((v & (1<<loi)) == 0)
               if (moveset[i] < 0 || __builtin_popcount(moveset[i]) > __builtin_popcount(v))
                  moveset[i] = v ;
// fall back to using it if needed
         if (moveset[i] < 0)
            for (auto v: opts[i])
               if (moveset[i] < 0 || __builtin_popcount(moveset[i]) > __builtin_popcount(v))
                  moveset[i] = v ;
         cost[i] = __builtin_popcount(moveset[i]) ;
      }
   }
   int seen = 0 ;
   int totcost = 0 ;
   for (int i=0; i<256; i++)
      if (cost[i] < 255) {
         totcost += cost[i] ;
         seen++ ;
      }
   if (seen == 70) {
      cout << "The 8 choose 4 sorting network looks good; have a plan." << endl ;
      cout << "The total cost of the network is " << totcost << endl ;
   } else {
      cout << "There is an error; we were only able to find " << seen <<
           " of the 70 possible permutations in the"
           " 8 choose 4 sorting network." << endl ;
      exit(10) ;
   }
}
/*
 *   Sometimes we don't need to sort certain elements.  For instance,
 *   if the lower face has 1233 and the upper face has 3345, the 3's
 *   are essentially don't cares; as long as the correct count of 3's
 *   end up in the lower face, it doesn't matter which they are.
 *   Thus, there can be multiple sets of permutations that work.
 *
 *   This routine takes a situation in trinary, with 0 meaning this
 *   is a value that belongs on the lower face, and 2 meaning this
 *   is a value that belongs on the upper face, and 1 meaning this
 *   value can be on either upper or lower face.  We then iterate
 *   through all binary states that `satisfy' this and choose the one
 *   with the least cost.
 *
 *   We cache the result in keycache84 so we don't need to perform
 *   this computation more than necessary.
 */
int keycache84[3*3*3*3*3*3*3*3] ;
int leastcost84(int key) {
   if (keycache84[key]) {
      return keycache84[key] ;
   }
   int base = 0 ;
   unsigned midbits = 0 ;
   int t = key ;
   for (int i=7; i>=0; i--) {
      int v = t % 3 ;
      t /= 3 ;
      if (v == 2)
         base |= 1<<i ;
      else if (v == 1)
         midbits |= 1<<i ;
   }
   int r = -1 ;
   if (midbits) {
      int bestscore = 1000 ;
      unsigned mask = midbits | (1<<20) ;
      for (unsigned i=0; i<=midbits; i = ((i | ~mask) + 1) & mask) {
         if (__builtin_popcount(base|i) == 4) {
            if (cost[base|i] < bestscore) {
               bestscore = cost[base|i] ;
               r = base|i ;
            }
         }
      }
   } else {
      r = base ;
   }
   keycache84[key] = r ;
   return r ;
}
/*
 *   This is our main 8-4 sort routine.  We get the offsets of
 *   two faces (f1 and f2) and the orbit pointer.
 *
 *   We read the sorting values from the faces, count how many
 *   of each exist, and decide if we have an ambiguous (middle
 *   values span) or not ambiguous case.  If it's ambiguous,
 *   we resolve it with leastcost84 above.
 *
 *   Then it's a simple matter of looking up and applying the
 *   moves.  Note that we apply the moves in the reverse order
 *   of the planning routine.
 */
int sort84(int f1, int f2, int typ, xorbit &orbit, int exec=0) {
   // apply our 84 plan to the first 8 elements alone.
   f1 *= 4 ;
   f2 *= 4 ;
   int key = 0 ;
   if (typ == 0) {
      int torbit[8] ;
      for (int i=0; i<4; i++) {
         torbit[i] = orbit.a[f1 + i] ;
         torbit[4+i] = orbit.a[f2 + i] ;
      }
      int sum = 0 ;
      for (int i=0; i<8; i++)
         sum += (1<<(4*torbit[i])) ;
      int cnt = 0 ;
      int mid = -1 ;
      int up = -1 ;
      for (int i=0; i<6; i++) {
         cnt += (sum >> (4 * i)) & 15 ;
         if (cnt == 4) {
            up = i + 1 ;
            break ;
         } else if (cnt >= 4) {
            up = i + 1 ;
            mid = i ;
            break ;
         }
      }
      for (int i=0; i<8; i++) {
         key *= 3 ;
         if (torbit[i] == mid) {
            key += 1 ;
         } else if (torbit[i] >= up) {
            key += 2 ;
         }
      }
   } else {
      for (int i=0; i<4; i++) {
         key *= 3 ;
         if (orbit.a[f1 + i] == f1 >> 2)
            key += 0 ;
         else
            key += 1 ;
      }
      for (int i=0; i<4; i++) {
         key *= 3 ;
         if (orbit.a[f2 + i] == f1 >> 2)
            key += 0 ;
         else
            key += 1 ;
      }
   }
   key = leastcost84(key) ;
   auto r = moveset[key] ;
   if (exec) {
      for (int s84i=0; s84i<8; s84i++)
         if ((r >> (7-s84i)) & 1) {
            int a = sort8_4[3*s84i] ;
            int b = sort8_4[3*s84i + 1] ;
            int c = sort8_4[3*s84i + 2] ;
            a = (a > 3 ? f2 + a - 4 : f1 + a) ;
            b = (b > 3 ? f2 + b - 4 : f1 + b) ;
            c = (c > 3 ? f2 + c - 4 : f1 + c) ;
            quick3swap(orbit, a, c, b) ;
         }
      if (typ) {
         for (int i=0; i<4; i++)
            if (orbit.a[f2+i] == f1 >> 2)
               error("! did not vacate all f1's") ;
      }
   }
   return r ;
}
ll solve(xcube &cube, int use84, int usesort, int width, int phasewidth) {
   vector<pair<int, int>> outerphases ;
   if (usesort) {
      for (int s6i=0; s6i<24; s6i += 2)
         outerphases.push_back({sort12[s6i], sort12[s6i+1]})  ;
   } else {
      for (int f1=0; f1<6; f1++)
         for (int f2=f1+1; f2<6; f2++)
            outerphases.push_back({f1, f2}) ;
   }
   vector<int> innerphases ;
   if (use84) {
      for (int i=0; i<24; i++)
         innerphases.push_back(sort8_4[i]) ;
   } else {
      for (int a=0; a<4; a++)
         for (int b=0; b<4; b++) {
            innerphases.push_back(a) ;
            innerphases.push_back(b+4) ;
            innerphases.push_back(((b+1)&3)+4) ;
         }
   }
   byphase.resize(outerphases.size() * innerphases.size() / 3) ;
   if (phasewidth > cube.My)
      phasewidth = cube.My ;
   if (phasewidth < width)
      phasewidth = width ;
   ll r = 0 ;
   vector<int> xs ;
   vector<int> ys ;
   vector<ll> bits, next ;
   vector<int> prev ;
   vector<vector<unsigned char>> movemasks(phasewidth) ;
   if (use84)
      for (int i=0; i<phasewidth; i++)
         movemasks[i].resize(cube.Mx) ;
   for (int i=0; i<cube.My; i += phasewidth) {
      int pw = phasewidth ;
      if (i + pw > cube.My)
         pw = cube.My - i ;
      for (int wi=0; wi<pw; wi++)
         cube.getrow(i+wi) ;
      int phasebase = 0 ;
      for (auto [f1, f2] : outerphases) {
         if (use84)
            for (int ii=0; ii<pw; ii++)
               for (int j=0; j<cube.Mx; j++)
                  movemasks[ii][j] = sort84(f1, f2, !usesort,
                                            cube.rows[i+ii][j], fast) ;
         for (int inner=0; 3*inner<(int)innerphases.size();
              inner++, phasebase++) {
            int a = innerphases[3*inner] ;
            int b = innerphases[3*inner+2] ;
            int c = innerphases[3*inner+1] ;
            for (int ii=0; ii<pw; ii += width) {
               int w = width ;
               if (ii + w > pw)
                  w = pw - ii ;
               bits.clear() ;
               bits.resize(cube.Mx) ;
               for (int wi=0; wi<w; wi++)
                  if (use84) {
                     for (int j=0; j<cube.Mx; j++)
                        if ((movemasks[ii+wi][j] >> (7-inner)) & 1)
                           bits[j] |= 1LL<<wi ;
                  } else {
                     auto &row = cube.rows[i+ii+wi] ;
                     for (int j=0; j<cube.Mx; j++)
                        if (row[j].a[f1*4+a] != f1 && row[j].a[f2*4+b-4] == f1)
                           bits[j] |= 1LL<<wi ;
                  }
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
                  auto &alg = findblockalgo(f1, f2, a, b, c) ;
                  ll mvs = doblock(cube, alg, ys, xs, fast && use84) ;
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
ll dosolve(ll N, int use84, int usesort, int width, int phasewidth) {
   ll My = (N - 2) >> 1 ;
   ll Mx = (N - 2) - My ;
   prevmvs.clear() ;
   blocks = 0 ;
   cancellations = 0 ;
   xcube cube(Mx, My) ;
   ll r = solve(cube, use84, usesort, width, phasewidth) ;
   if (!fast)
      for (int i=0; i<cube.My; i++)
         cube.checkrow(i) ;
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
string flags ;
int width=1, phasewidth=1 ;
void fullreport(ll at) {
   cout << "N " << N << " flags " << flags << " width " << width
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
int main(int argc, char *argv[]) {
   cout.precision(15) ;
   cout << "-" ;
   for (int i=0; i<argc; i++)
      cout << " " << argv[i] ;
   cout << endl ;
   plan84() ;
   int use84 = 0 ;
   int usesort = 0 ;
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
case 'w':
         argc-- ;
         argv++ ;
         width = atoll(argv[0]) ;
         break ;
case '6':
         usesort = 1 ;
         break ;
case '8':
         use84 = 1 ;
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
      }
   }
   if (!fast && extrapolate)
      error("! cannot go slow and extrapolate at the same time") ;
   if (usesort)
      flags.push_back('6') ;
   if (use84)
      flags.push_back('8') ;
   if (flags.size() == 0)
      flags.push_back('-') ;
   if (usesort && !use84)
      error("! we don't support sorting networks and 16-swap") ;
   expandrotations() ;
   loadalgs() ;
   if (phasewidth < width)
      phasewidth = width ;
   N = atoll(argv[1]) ;
   ll My = (N - 2) >> 1 ;
   ll Mx = (N - 2) - My ;
   if (extrapolate)
      extrapolatemul = My / (double)phasewidth ;
   ll mmc = 0 ;
   for (ll ii=1; ii<=solvecount; ii++) {
      ll t = dosolve(N, use84, usesort, width, phasewidth) ;
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
         lll lll_cancellations = (lll)cancellations * My / phasewidth ;
         lll lll_blocks = (lll)blocks * My / phasewidth ;
         cout << "N " << N << " flags " << flags << " width " << width
              << " moves " << lll_r << " cancel " << lll_cancellations
              << " blocks " << lll_blocks << endl ;
      } else {
         if (stats)
            for (int i=0; i<(int)byphase.size(); i++)
               byphase[i].showstats(i, Mx*My) ;
         cout << "N " << N << " flags " << flags << " width " << width
              << " moves " << r << " cancel " << cancellations
              << " blocks " << blocks << endl ;
      }
   } else {
      fullreport(solvecount) ;
   }
}
