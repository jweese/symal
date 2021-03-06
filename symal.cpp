#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "cmd.h"

using namespace std;

constexpr int MAX_WORD = 10000;  // maximum lengthsource/target strings
constexpr int MAX_M = 400;       // maximum length of source strings
constexpr int MAX_N = 400;       // maximum length of target strings

enum Alignment {
  UNION = 1,
  INTERSECT,
  GROW,
  SRCTOTGT,
  TGTTOSRC,
};

const Enum_T END_ENUM = {0, 0};

namespace
{
Enum_T AlignEnum [] = {
  {    "union",                        UNION },
  {    "u",                            UNION },
  {    "intersect",                    INTERSECT},
  {    "i",                            INTERSECT},
  {    "grow",                         GROW },
  {    "g",                            GROW },
  {    "srctotgt",                     SRCTOTGT },
  {    "s2t",                          SRCTOTGT },
  {    "tgttosrc",                     TGTTOSRC },
  {    "t2s",                          TGTTOSRC },
  END_ENUM
};

Enum_T BoolEnum [] = {
  {    "true",        true },
  {    "yes",         true },
  {    "y",           true },
  {    "false",       false },
  {    "no",          false },
  {    "n",           false },
  END_ENUM
};

// global variables and constants

std::array<int, MAX_M + 1> fa{}; //counters of covered foreign positions
std::array<int, MAX_N + 1> ea{}; //counters of covered english positions
std::array<std::array<int, MAX_M + 1>, MAX_N + 1> A{};

int verbose=0;

//read an alignment pair from the input stream.

int lc = 0;

int getals(istream& inp,int& m, int *a,int& n, int *b)
{
  char w[MAX_WORD], dummy[10];
  int i,j,freq;
  if (inp >> freq) {
    ++lc;
    //target sentence
    inp >> n;
    assert(n<MAX_N);
    for (i=1; i<=n; i++) {
      inp >> setw(MAX_WORD) >> w;
      if (strlen(w)>=MAX_WORD-1) {
        cerr << lc << ": target len=" << strlen(w) << " is not less than MAX_WORD-1="
             << MAX_WORD-1 << endl;
        assert(strlen(w)<MAX_WORD-1);
      }
    }

    inp >> dummy; //# separator
    // inverse alignment
    for (i=1; i<=n; i++) inp >> b[i];

    //source sentence
    inp >> m;
    assert(m<MAX_M);
    for (j=1; j<=m; j++) {
      inp >> setw(MAX_WORD) >> w;
      if (strlen(w)>=MAX_WORD-1) {
        cerr << lc << ": source len=" << strlen(w) << " is not less than MAX_WORD-1="
             << MAX_WORD-1 << endl;
        assert(strlen(w)<MAX_WORD-1);
      }
    }

    inp >> dummy; //# separator

    // direct alignment
    for (j=1; j<=m; j++) {
      inp >> a[j];
      assert(0<=a[j] && a[j]<=n);
    }

    //check inverse alignemnt
    for (i=1; i<=n; i++)
      assert(0<=b[i] && b[i]<=m);

    return 1;

  } else
    return 0;
}


//compute union alignment
int prunionalignment(ostream& out,int m,int *a,int n,int* b)
{

  ostringstream sout;

  for (int j=1; j<=m; j++)
    if (a[j])
      sout << j-1 << "-" << a[j]-1 << " ";

  for (int i=1; i<=n; i++)
    if (b[i] && a[b[i]]!=i)
      sout << b[i]-1 <<  "-" << i-1 << " ";

  //fix the last " "
  string str = sout.str();
  if (str.length() == 0)
    str = "\n";
  else
    str.replace(str.length()-1,1,"\n");

  out << str;
  out.flush();

  return 1;
}


//Compute intersection alignment

int printersect(ostream& out,int m,int *a,int,int* b)
{

  ostringstream sout;

  for (int j=1; j<=m; j++)
    if (a[j] && b[a[j]]==j)
      sout << j-1 << "-" << a[j]-1 << " ";

  //fix the last " "
  string str = sout.str();
  if (str.length() == 0)
    str = "\n";
  else
    str.replace(str.length()-1,1,"\n");

  out << str;
  out.flush();

  return 1;
}

//Compute target-to-source alignment

int printtgttosrc(ostream& out,int ,int *,int n,int* b)
{

  ostringstream sout;

  for (int i=1; i<=n; i++)
    if (b[i])
      sout << b[i]-1 << "-" << i-1 << " ";

  //fix the last " "
  string str = sout.str();
  if (str.length() == 0)
    str = "\n";
  else
    str.replace(str.length()-1,1,"\n");

  out << str;
  out.flush();

  return 1;
}

//Compute source-to-target alignment

int printsrctotgt(ostream& out,int m,int *a,int ,int*)
{

  ostringstream sout;

  for (int j=1; j<=m; j++)
    if (a[j])
      sout << j-1 << "-" << a[j]-1 << " ";

  //fix the last " "
  string str = sout.str();
  if (str.length() == 0)
    str = "\n";
  else
    str.replace(str.length()-1,1,"\n");

  out << str;
  out.flush();

  return 1;
}

//Compute Grow Diagonal Alignment
//Nice property: you will never introduce more points
//than the unionalignment alignemt. Hence, you will always be able
//to represent the grow alignment as the unionalignment of a
//directed and inverted alignment

struct Point {
  int src = 0;
  int tgt = 0;

  constexpr Point() = default;
  constexpr Point(int s, int t) : src(s), tgt(t) {}

  constexpr bool operator==(Point p) const {
    return src == p.src && tgt == p.tgt;
  }

  constexpr bool operator<(Point p) const {
    return src < p.src || (src == p.src && tgt < p.tgt);
  }
};

constexpr std::array<Point, 4> kSquareNeighbors = {
  Point(-1,-0),
  Point(0,-1),
  Point(1,0),
  Point(0,1)
};

constexpr std::array<Point, 8> kDiagonalNeighbors = {
  Point(-1,-0),
  Point(0,-1),
  Point(1,0),
  Point(0,1),
  Point(-1,-1),
  Point(-1,1),
  Point(1,-1),
  Point(1,1)
};

int printgrow(ostream& out,int m,int *a,int n,int* b, bool diagonal=false,bool isfinal=false,bool bothuncovered=false)
{
  const auto neighbors = [diagonal]() -> std::vector<Point> {
    if (diagonal) {
      return { kDiagonalNeighbors.begin(), kDiagonalNeighbors.end() };
    } else {
      return { kSquareNeighbors.begin(), kSquareNeighbors.end() };
    }
  }();

  //covered foreign and english positions
  fa.fill(0);
  ea.fill(0);

  //matrix to quickly check if one point is in the symmetric
  //alignment (value=2), direct alignment (=1) and inverse alignment
  for (auto& row : A) {
    row.fill(0);
  }

  std::set<Point> currentpoints; //symmetric alignment
  std::vector<Point> unionalignment; //union alignment

  //fill in the alignments
  for (int j=1; j<=m; j++) {
    if (a[j]) {
      unionalignment.emplace_back(a[j],j);
      if (b[a[j]]==j) {
        fa[j]=1;
        ea[a[j]]=1;
        A[a[j]][j]=2;
        currentpoints.emplace(a[j],j);
      } else
        A[a[j]][j]=-1;
    }
  }

  for (int i=1; i<=n; i++)
    if (b[i] && a[b[i]]!=i) { //not intersection
      unionalignment.emplace_back(i,b[i]);
      A[i][b[i]]=1;
    }

  std::sort(unionalignment.begin(), unionalignment.end());
  auto it = std::unique(unionalignment.begin(), unionalignment.end());
  unionalignment.erase(it, unionalignment.end());

  for (std::vector<Point> added(1);
      !added.empty();
      currentpoints.insert(added.begin(), added.end())) {
    added.clear();
    for (auto [src, tgt] : currentpoints) {
      for (auto neighbor : neighbors) {
        Point point(src + neighbor.src, tgt + neighbor.tgt);
        if (point.src <= 0 || point.src > n || point.tgt <=0 || point.tgt > m) {
          continue;
        }
        if (b[point.src] != point.tgt && a[point.tgt] != point.src) {
          continue;
        }
        if (ea[point.src] && fa[point.tgt]) {
          continue;
        }
        added.emplace_back(point);
        A[point.src][point.tgt]=2;
        ea[point.src]=1;
        fa[point.tgt]=1;
      }
    }
  }

  if (isfinal) {
    for (auto [src, tgt] : unionalignment) {
      if (A[src][tgt]==1) {
        //one of the two words is not covered yet
        if (bothuncovered ^ (ea[src] || fa[tgt])) {
          continue;
        }
        currentpoints.insert(Point(src, tgt));
        A[src][tgt]=2;
        //keep track of new covered positions
        ea[src]=1;
        fa[tgt]=1;
      }
    }

    for (auto [src, tgt] : unionalignment) {
      if (A[src][tgt]==-1) {
        //one of the two words is not covered yet
        if (bothuncovered ^ (ea[src] || fa[tgt])) {
          continue;
        }
        currentpoints.insert(Point(src, tgt));
        A[src][tgt]=2;
        //keep track of new covered positions
        ea[src]=1;
        fa[tgt]=1;
      }
    }
  }

  std::ostringstream sout;
  for (auto [src, tgt] : currentpoints) {
    sout << tgt-1 << "-" << src-1 << " ";
  }

  //fix the last " "
  string str = sout.str();
  if (str.length() == 0)
    str = "\n";
  else
    str.replace(str.length()-1,1,"\n");

  out << str;
  out.flush();
  return 1;

  return 1;
}

} // namespace


//Main file here


int main(int argc, char** argv)
{

  int alignment=0;
  char* input= NULL;
  char* output= NULL;
  int diagonal=false;
  int isfinal=false;
  int bothuncovered=false;


  DeclareParams("a", CMDENUMTYPE,  &alignment, AlignEnum,
                "alignment", CMDENUMTYPE,  &alignment, AlignEnum,
                "d", CMDENUMTYPE,  &diagonal, BoolEnum,
                "diagonal", CMDENUMTYPE,  &diagonal, BoolEnum,
                "f", CMDENUMTYPE,  &isfinal, BoolEnum,
                "final", CMDENUMTYPE,  &isfinal, BoolEnum,
                "b", CMDENUMTYPE,  &bothuncovered, BoolEnum,
                "both", CMDENUMTYPE,  &bothuncovered, BoolEnum,
                "i", CMDSTRINGTYPE, &input,
                "o", CMDSTRINGTYPE, &output,
                "v", CMDENUMTYPE,  &verbose, BoolEnum,
                "verbose", CMDENUMTYPE,  &verbose, BoolEnum,

                NULL);

  GetParams(&argc, &argv, NULL);

  if (alignment==0) {
    cerr << "usage: symal [-i=<inputfile>] [-o=<outputfile>] -a=[u|i|g] -d=[yes|no] -b=[yes|no] -f=[yes|no] \n"
         << "Input file or std must be in .bal format (see script giza2bal.pl).\n";

    exit(1);
  }

  istream *inp = &std::cin;
  ostream *out = &std::cout;

  try {
    if (input) {
      fstream *fin = new fstream(input,ios::in);
      if (!fin->is_open()) throw runtime_error("cannot open " + string(input));
      inp = fin;
    }

    if (output) {
      fstream *fout = new fstream(output,ios::out);
      if (!fout->is_open()) throw runtime_error("cannot open " + string(output));
      out = fout;
    }

    int a[MAX_M],b[MAX_N],m,n;

    int sents = 0;

    switch (alignment) {
    case UNION:
      cerr << "symal: computing union alignment\n";
      while(getals(*inp,m,a,n,b)) {
        prunionalignment(*out,m,a,n,b);
        sents++;
      }
      cerr << "Sents: " << sents << endl;
      break;
    case INTERSECT:
      cerr << "symal: computing intersect alignment\n";
      while(getals(*inp,m,a,n,b)) {
        printersect(*out,m,a,n,b);
        sents++;
      }
      cerr << "Sents: " << sents << endl;
      break;
    case GROW:
      cerr << "symal: computing grow alignment: diagonal ("
           << diagonal << ") final ("<< isfinal << ")"
           <<  "both-uncovered (" << bothuncovered <<")\n";

      while(getals(*inp,m,a,n,b))
        printgrow(*out,m,a,n,b,diagonal,isfinal,bothuncovered);

      break;
    case TGTTOSRC:
      cerr << "symal: computing target-to-source alignment\n";

      while(getals(*inp,m,a,n,b)) {
        printtgttosrc(*out,m,a,n,b);
        sents++;
      }
      cerr << "Sents: " << sents << endl;
      break;
    case SRCTOTGT:
      cerr << "symal: computing source-to-target alignment\n";

      while(getals(*inp,m,a,n,b)) {
        printsrctotgt(*out,m,a,n,b);
        sents++;
      }
      cerr << "Sents: " << sents << endl;
      break;
    default:
      throw runtime_error("Unknown alignment");
    }

    if (inp != &std::cin) {
      delete inp;
    }
    if (out != &std::cout) {
      delete out;
    }
  } catch (const std::exception &e) {
    cerr << e.what() << std::endl;
    exit(1);
  }

  exit(0);
}
