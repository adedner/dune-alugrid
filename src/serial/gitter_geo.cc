//  Version f"ur DUNE
//
  // (c) bernhard schupp, 1997 - 1998

  // $Source$
  // $Revision$
  // $Name$
  // $State$

/* $Id$
 * $Log$
 * Revision 1.1  2005/03/23 14:57:53  robertk
 * all files for serial version of ALU3dGrid.
 *
 * Revision 1.7  2005/03/23 11:36:06  robertk
 * removed BSGridVecType, which is double * now.
 *
 * Revision 1.6  2004/12/21 17:36:45  robertk
 * removed some warnings.
 *
 * Revision 1.5  2004/12/20 21:37:56  robertk
 * reference tetra added.
 *
 * Revision 1.4  2004/11/16 19:32:24  robertk
 * oppositeFace for Hexa.
 *
 * Revision 1.3  2004/10/25 16:38:09  robertk
 * All header end with .h now. Like the original.
 *
 * In the .cc this changes are done.
 *
 * Revision 1.2  2004/10/19 13:19:51  robertk
 * neighOuterNormal added. this method calculates the normal from neighbour
 * point of view but pointing outside of actual element.
 *
 * Revision 1.1  2004/10/15 09:48:37  robertk
 * Inititial version. Some extenxions for Dune made. Schould be compatible
 * with all other applications done so far.
 *
 * Revision 1.10  2002/05/24 09:05:31  dedner
 * Vorl"aufig syntaktisch korrekte, d.h. kompilierbare Version
 *
 * Revision 1.9  2002/05/23 16:37:41  dedner
 * Test nach Einbau der Periodischen 4-Raender
 *
 * Revision 1.8  2002/04/19 15:36:07  wesenber
 * modifications required for IBM VisualAge C++ Version 5.0
 *
 * Revision 1.7  2001/12/12 09:49:05  wesenber
 * resetRefinementRequest() added
 *
 * Revision 1.6  2001/12/10 13:35:39  wesenber
 * parameter ``filePath'' for backup() and backupCMode() added
 *
 ***/

#ifdef IBM_XLC
  #define _ANSI_HEADER
#endif

#include <stdlib.h>
#include <assert.h>

#ifdef _ANSI_HEADER
  using namespace std;
  #include <iostream>
  #include <fstream>
  #include <vector>
  #include <map>
#else
  #include <iostream.h>
  #include <fstream.h>
  #include <vector.h>
  #include <map.h>
#endif

#include "mapp_cube_3d.h"
#include "mapp_tetra_3d.h"
#include "gitter_sti.h"
#include "walk.h"

static volatile char RCSId_gitter_geo_cc [] = "$Id$" ;

extern "C" { double drand48 (void) ; }

const pair < Gitter :: Geometric :: hasFace3 *, int > Gitter :: Geometric :: hface3 :: face3Neighbour :: null 
  = pair < Gitter :: Geometric :: hasFace3 *, int > (Gitter :: Geometric :: InternalHasFace3 ()(0), -1) ;

const pair < Gitter :: Geometric :: hasFace4 *, int > Gitter :: Geometric :: hface4 :: face4Neighbour :: null 
  = pair < Gitter :: Geometric :: hasFace4 *, int > (Gitter :: Geometric :: InternalHasFace4 ()(0), -1) ;


const int Gitter :: Geometric :: Tetra :: prototype [4][3] = {{1,3,2},{0,2,3},{0,3,1},{0,1,2}} ;
  
const int Gitter :: Geometric :: Periodic3 :: prototype [2][3] = {{0,1,2},{3,5,4}} ;

// Anfang - Neu am 23.5.02 (BS)
const int Gitter :: Geometric :: Periodic4 :: prototype [2][4] = {{0,3,2,1},{4,5,6,7}} ;
// Ende - Neu am 23.5.02 (BS)


// #     #
// #     #  ######  #    #    ##
// #     #  #        #  #    #  #
// #######  #####     ##    #    #
// #     #  #         ##    ######
// #     #  #        #  #   #    #
// #     #  ######  #    #  #    #

  //  Prototyp des Hexaeders wie er im Programm verwendet wird.
  //  Eckpunkte und Seitenflaechen:
  //
  //
  //              7---------6
  //             /.        /|
  //            / .  1    / |
  //           /  .      /  |
  //          4---------5   | <-- 4 (hinten)
  //    5 --> |   .     | 3 |
  //          |   3.....|...2
  //          |  .      |  /
  //          | .   2   | / <-- 0 (unten)
  //          |.        |/
  //          0---------1
  //

const int Gitter :: Geometric :: Hexa :: prototype [6][4] = {{0,3,2,1},{4,5,6,7},{0,1,5,4},{1,2,6,5},{2,3,7,6},{0,4,7,3}} ;
const int Gitter :: Geometric :: Hexa :: oppositeFace [6] = { 1 , 0 , 4 , 5 , 2 , 3  }; // opposite face of face 

int Gitter :: Geometric :: Hexa :: test () const {
  static const int v0[8][2] = {{0,0},{0,1},{0,2},{0,3},{1,0},{1,1},{1,2},{1,3}} ;
  static const int v1[8][2] = {{2,0},{4,1},{3,1},{2,1},{2,3},{2,2},{3,2},{4,2}} ;
  static const int v2[8][2] = {{5,0},{5,3},{4,0},{3,0},{5,1},{3,3},{4,3},{5,2}} ;
  int nfaults = 0 ;
  {
    for(int i = 0 ; i < 8 ; i ++ ) {
      int i0 = v0[i][0], j0 = v0[i][1] ;
      int i1 = v1[i][0], j1 = v1[i][1] ;
      int i2 = v2[i][0], j2 = v2[i][1] ;
      if(myvertex (i0, j0) != myvertex (i1, j1)) {
        cerr << "**FEHLER auf level: " << level () << " " ;
  cerr << "vertex (" << i0 << "," << j0 << ") != vertex (" << i1 << "," << j1 << ")";
  cerr << "\t(" << i0 << "," << j0 << ") =" << myvertex(i0,j0) << " " << twist (i0) ;
  cerr << "\t(" << i1 << "," << j1 << ") =" << myvertex(i1,j1) << " " << twist (i1) ;
  cerr << endl ; 
  nfaults ++ ;
      }
      if(myvertex (i0, j0) != myvertex (i2, j2)) {
        cerr << "**FEHLER auf level: " << level () << " " ;
  cerr << "vertex (" << i0 << "," << j0 << ") != vertex (" << i2 << "," << j2 << ")" ;
  cerr << "\t(" << i0 << "," << j0 << ") =" << myvertex(i0,j0) << " " << twist (i0) ;
  cerr << "\t(" << i2 << "," << j2 << ") =" << myvertex(i2,j2) << " " << twist (i1) ;
  cerr << endl;
  nfaults ++ ;
      }
    }
  }
  return nfaults ;
}

int Gitter :: Geometric :: Hexa :: tagForGlobalRefinement () {
  return (request (myrule_t :: iso8), 1) ;
}

int Gitter :: Geometric :: Hexa :: resetRefinementRequest () {
  return (request (myrule_t :: crs), 1) ;
}

static inline bool insideBall (const double (&p)[3], const double (&c)[3], double r) {
  bool inside=false;
  double q[3];
  int x,y,z;
  for (x=0 , q[0]=p[0]-c[0]-2.0 ; !inside && x<3 ; x++) {
    for (y=0 , q[1]=p[1]-c[1]-2.0 ; !inside && y<3 ; y++) {
      for (z=0 , q[2]=p[2]-c[2]-2.0 ; !inside && z<3 ; z++) {
        inside = ( q[0]*q[0]+q[1]*q[1]+q[2]*q[2] < r*r ) ;
        q[2]+=2.0;
      }
      q[1]+=2.0;
    }
    q[0]+=2.0;
  }
  return inside;
  //return 
  //     (((p [0] - c [0]) * (p [0] - c [0]) + (p [1] - c [1]) * (p [1] - c [1])
  //     + (p [2] - c [2]) * (p [2] - c [2])) < (r * r)) ? true : false ;
}

int Gitter :: Geometric :: Hexa :: tagForBallRefinement (const double (&center)[3], double radius, int limit) {
  bool hit = false ;
  for (int i = 0 ; i < 8 ; i ++) {
    const double (&p)[3] = myvertex (i)->Point () ;
    if (insideBall (p,center,radius)) { hit = true ; break ; }
  }
  if (!hit) {
    const int resolution = 50 ;
    TrilinearMapping map (myvertex(0)->Point(), myvertex(1)->Point(),
        myvertex(2)->Point(), myvertex(3)->Point(), myvertex(4)->Point(),
        myvertex(5)->Point(), myvertex(6)->Point(), myvertex(7)->Point()) ;
    double p [3] ;
    for (int i = 0 ; i < resolution ; i ++ ) {
      map.map2world (2.0 * drand48 () - 1.0, 2.0 * drand48 () - 1.0, 2.0 * drand48 () - 1.0, p) ;
      if (insideBall (p,center,radius)) { hit = true ; break ; }
    }
  }
  return hit ? (level () < limit ? (request (myrule_t :: iso8), 1) 
         : (request (myrule_t :: nosplit), 0)) : (request (myrule_t :: crs), 1) ;
}

// #######
//    #     ######   #####  #####     ##
//    #     #          #    #    #   #  #
//    #     #####      #    #    #  #    #
//    #     #          #    #####   ######
//    #     #          #    #   #   #    #
//    #     ######     #    #    #  #    #

#if 0
/*
//          z          y
//         3 |-------- 2
//           |.      /\     faces opposite to vertices
//           |  .   /  \
//           |    ./    \
//           |    / .    \
//           |   /    .   \
//           |  /       .  \
//           | /          . \
//           |/             .\ 1 
//         0 ----------------------- x 
//                              
// face 0 = {1,3,2}
// face 1 = {0,2,3}
// face 2 = {0,3,1}
// face 3 = {0,1,2} 
//
//
//
//
//
//
//
*/
#endif

int Gitter :: Geometric :: Tetra :: test () const {
//  cerr << "**WARNUNG (IGNORIERT) Tetra :: test () nicht implementiert, in " <<  __FILE__ << " " << __LINE__  << endl;
  return 0 ;
}

int Gitter :: Geometric :: Tetra :: tagForGlobalRefinement () {
  return (request (myrule_t :: iso8), 1) ;
}

int Gitter :: Geometric :: Tetra :: resetRefinementRequest () {
  return (request (myrule_t :: crs), 1) ;
}

int Gitter :: Geometric :: Tetra :: tagForBallRefinement (const double (&center)[3], double radius, int limit) {
  bool hit = false ;
  for (int i = 0 ; i < 4 ; i ++) {
    const double (&p)[3] = myvertex (i)->Point () ;
    if (insideBall (p,center,radius)) { hit = true ; break ; }
  }
  if (!hit) {
    const int resolution = 50 ;
    LinearMapping map (myvertex(0)->Point(), myvertex(1)->Point(),
            myvertex(2)->Point(), myvertex(3)->Point()) ;
    double p [3] ;
    for (int i = 0 ; i < resolution ; i ++ ) {
      double b1 = drand48 () ;
      double b2 = (1.0 - b1) * drand48 () ;
      double b3 = (1.0 - b1 - b2) * drand48 () ;
      double b4 = 1.0 - b1 - b2 - b3 ;
      
  // Sind das "uberhaupt Zufallspunkte ? Nein. Leider nicht.
  
      map.map2world (b1, b2, b3, b4, p) ;
      if (insideBall (p,center,radius)) { hit = true ; break ; }
    }
  }
  return hit ? (level () < limit ? (request (myrule_t :: iso8), 1) 
         : (request (myrule_t :: nosplit), 0)) : (request (myrule_t :: crs), 1) ;
}

void  Gitter :: Geometric :: Tetra :: outerNormal (int face , double * normal ) 
{
  BSGridLinearSurfaceMapping 
    LSM(this->myvertex(face,0)->Point(),
        this->myvertex(face,1)->Point(),
        this->myvertex(face,2)->Point()
       );
  LSM.normal(normal);
  return; 
}

void  Gitter :: Geometric :: Tetra :: neighOuterNormal (int face , double * normal ) 
{
  // just use with other twist to minus normal 
  BSGridLinearSurfaceMapping 
    LSM(this->myvertex(face,2)->Point(),
        this->myvertex(face,1)->Point(),
        this->myvertex(face,0)->Point()
       );
  LSM.normal(normal);
  return; 
}

// ######                                                           #####
// #     #  ######  #####      #     ####   #####      #     ####  #     #
// #     #  #       #    #     #    #    #  #    #     #    #    #       #
// ######   #####   #    #     #    #    #  #    #     #    #       #####
// #        #       #####      #    #    #  #    #     #    #            #
// #        #       #   #      #    #    #  #    #     #    #    # #     #
// #        ######  #    #     #     ####   #####      #     ####   #####

// #include <iomanip.h>

int Gitter :: Geometric :: Periodic3 :: test () const {
  cerr << "**WARNUNG (IGNORIERT) Periodic3 :: test () nicht implementiert, in " <<  __FILE__ << " " << __LINE__  << endl;
//  const int digits = 3 ;
//  cout << "Fl\"ache: 0, Twist: " << twist (0) << "\t" ;
//  const VertexGeo * vx = myvertex (0,0) ;
//  cout << "(" << setw (digits) << vx->Point ()[0] << "," << setw (digits) << vx->Point ()[1] << "," << setw (digits) << vx->Point ()[2] << ") " ;
//  vx = myvertex (0,1) ;
//  cout << "(" << setw (digits) << vx->Point ()[0] << "," << setw (digits) << vx->Point ()[1] << "," << setw (digits) << vx->Point ()[2] << ") " ;
//  vx = myvertex (0,2) ;
//  cout << "(" << setw (digits) << vx->Point ()[0] << "," << setw (digits) << vx->Point ()[1] << "," << setw (digits) << vx->Point ()[2] << ") \n" ;
//  cout << "Fl\"ache: 1, Twist: " << twist (1) << "\t" ;
//  vx = myvertex (1,0) ;
//  cout << "(" << setw (digits) << vx->Point ()[0] << "," << setw (digits) << vx->Point ()[1] << "," << setw (digits) << vx->Point ()[2] << ") " ;
//  vx = myvertex (1,2) ;
//  cout << "(" << setw (digits) << vx->Point ()[0] << "," << setw (digits) << vx->Point ()[1] << "," << setw (digits) << vx->Point ()[2] << ") " ;
//  vx = myvertex (1,1) ;
//  cout << "(" << setw (digits) << vx->Point ()[0] << "," << setw (digits) << vx->Point ()[1] << "," << setw (digits) << vx->Point ()[2] << ") \n" << endl ;
  return 0 ;
}

int Gitter :: Geometric :: Periodic3 :: tagForGlobalRefinement () {
  return 0 ;
}

int Gitter :: Geometric :: Periodic3 :: resetRefinementRequest () {
  return 0 ;
}

int Gitter :: Geometric :: Periodic3 :: tagForBallRefinement (const double (&center)[3], double radius, int limit) {
  return 0 ;
}

// Anfang - Neu am 23.5.02 (BS)

// ######                                                          #
// #     #  ######  #####      #     ####   #####      #     ####  #    #
// #     #  #       #    #     #    #    #  #    #     #    #    # #    #
// ######   #####   #    #     #    #    #  #    #     #    #      #    #
// #        #       #####      #    #    #  #    #     #    #      #######
// #        #       #   #      #    #    #  #    #     #    #    #      #
// #        ######  #    #     #     ####   #####      #     ####       #

int Gitter :: Geometric :: Periodic4 :: test () const {
  cerr << "**WARNUNG (IGNORIERT) Periodic4 :: test () nicht implementiert, in " <<  __FILE__ << " " << __LINE__  << endl;
  return 0 ;
}

int Gitter :: Geometric :: Periodic4 :: tagForGlobalRefinement () {
  return 0 ;
}

int Gitter :: Geometric :: Periodic4 :: resetRefinementRequest () {
  return 0 ;
}

int Gitter :: Geometric :: Periodic4 :: tagForBallRefinement (const double (&center)[3], double radius, int limit) {
  return 0 ;
}

// Ende - Neu am 23.5.02 (BS)

Gitter :: Geometric :: BuilderIF :: ~BuilderIF () {
  if (iterators_attached ()) 
    cerr << "**WARNUNG (IGNORIERT) beim L\"oschen von BuilderIF: Iterator-Z\"ahler [" << iterators_attached () << "]" << endl ;
  {for (list < hexa_GEO * > :: iterator i = _hexaList.begin () ; i != _hexaList.end () ; delete (*i++)) ; }
  {for (list < tetra_GEO * > :: iterator i = _tetraList.begin () ; i != _tetraList.end () ; delete (*i++)) ; }
  {for (list < periodic3_GEO * > :: iterator i = _periodic3List.begin () ; i != _periodic3List.end () ; delete (*i++)) ; }
  {for (list < periodic4_GEO * > :: iterator i = _periodic4List.begin () ; i != _periodic4List.end () ; delete (*i++)) ; }
  {for (list < hbndseg4_GEO * > :: iterator i = _hbndseg4List.begin () ; i != _hbndseg4List.end () ; delete (*i++)) ; }
  {for (list < hbndseg3_GEO * > :: iterator i = _hbndseg3List.begin () ; i != _hbndseg3List.end () ; delete (*i++)) ; }
  {for (list < hface4_GEO * > :: iterator i = _hface4List.begin () ; i != _hface4List.end () ; delete (*i++)) ; }
  {for (list < hface3_GEO * > :: iterator i = _hface3List.begin () ; i != _hface3List.end () ; delete (*i++)) ; }
  {for (list < hedge1_GEO * > :: iterator i = _hedge1List.begin () ; i != _hedge1List.end () ; delete (*i++)) ; }
  {for (list < VertexGeo * > :: iterator i = _vertexList.begin () ; i != _vertexList.end () ; delete (*i++)) ; }
}

IteratorSTI < Gitter :: vertex_STI > * Gitter :: Geometric :: BuilderIF :: iterator (const vertex_STI *) const {
  ListIterator < VertexGeo > w (_vertexList) ;
  return new Wrapper < ListIterator < VertexGeo >, InternalVertex > (w) ;
}

IteratorSTI < Gitter :: vertex_STI > * Gitter :: Geometric :: BuilderIF :: iterator (const IteratorSTI < vertex_STI > * w) const {
  return new Wrapper < ListIterator < VertexGeo >, InternalVertex > (*(const Wrapper < ListIterator < VertexGeo >, InternalVertex > *) w) ;
}

IteratorSTI < Gitter :: hedge_STI > * Gitter :: Geometric :: BuilderIF :: iterator (const hedge_STI *) const {
  ListIterator < hedge1_GEO > w (_hedge1List) ;
  return new Wrapper < ListIterator < hedge1_GEO >, InternalEdge > (w) ;
}

IteratorSTI < Gitter :: hedge_STI > * Gitter :: Geometric :: BuilderIF :: iterator (const IteratorSTI < hedge_STI > * w) const {
  return new Wrapper < ListIterator < hedge1_GEO >, InternalEdge > (*(const Wrapper < ListIterator < hedge1_GEO >, InternalEdge > *)w) ;
}

IteratorSTI < Gitter :: hface_STI > * Gitter :: Geometric :: BuilderIF :: iterator (const hface_STI *) const {
  ListIterator < hface4_GEO > w1 (_hface4List) ;
  ListIterator < hface3_GEO > w2 (_hface3List) ;
  return new AlignIterator < ListIterator < hface4_GEO >, ListIterator < hface3_GEO >, hface_STI > (w1,w2) ;
}

IteratorSTI < Gitter :: hface_STI > * Gitter :: Geometric :: BuilderIF :: iterator (const IteratorSTI < hface_STI > * w) const {
  return new AlignIterator < ListIterator < hface4_GEO >, ListIterator < hface3_GEO >, hface_STI > 
    (*(const AlignIterator < ListIterator < hface4_GEO >, ListIterator < hface3_GEO >, hface_STI > *)w) ;
}

IteratorSTI < Gitter :: hbndseg_STI > * Gitter :: Geometric :: BuilderIF :: iterator (const hbndseg_STI *) const {
  ListIterator < hbndseg4_GEO > w1 (_hbndseg4List) ;
  ListIterator < hbndseg3_GEO > w2 (_hbndseg3List) ;
  return new AlignIterator < ListIterator < hbndseg4_GEO >, ListIterator < hbndseg3_GEO >, hbndseg_STI > (w1,w2) ;
}

IteratorSTI < Gitter :: hbndseg_STI > * Gitter :: Geometric :: BuilderIF :: iterator (const IteratorSTI < hbndseg_STI > * w) const {
  return new AlignIterator < ListIterator < hbndseg4_GEO >, ListIterator < hbndseg3_GEO >, hbndseg_STI > 
            (*(const AlignIterator < ListIterator < hbndseg4_GEO >, ListIterator < hbndseg3_GEO >, hbndseg_STI > *)w) ;;
}

IteratorSTI < Gitter :: helement_STI > * Gitter :: Geometric :: BuilderIF :: iterator (const helement_STI *) const {

    // Vorsicht ! Falls diese Methode ver"andert wird, muss die selbe "Anderung
    // in der Kopiermethode (nachfolgend) vorgenommen werden. Sonst f"uhrt die 
    // statische Typkonversion zu einem schwer nachvollziehbaren Fehler.
// Anfang - Neu am 23.5.02 (BS)

  ListIterator < hexa_GEO > w1 (_hexaList) ;
  ListIterator < tetra_GEO > w2 (_tetraList) ;
  ListIterator < periodic3_GEO > w3 (_periodic3List) ;
  ListIterator < periodic4_GEO > w4 (_periodic4List) ;

  AlignIterator < ListIterator < hexa_GEO >, ListIterator < tetra_GEO >, helement_STI > aw12 (w1,w2) ;
  AlignIterator < ListIterator < periodic3_GEO >, ListIterator < periodic4_GEO >, helement_STI > aw34 (w3,w4) ;

  return new AlignIterator < AlignIterator < ListIterator < hexa_GEO >, ListIterator < tetra_GEO >, helement_STI >,
    AlignIterator < ListIterator < periodic3_GEO >, ListIterator < periodic4_GEO >, helement_STI >, helement_STI > (aw12,aw34) ;
// Ende - Neu am 23.5.02 (BS)
}

IteratorSTI < Gitter :: helement_STI > * Gitter :: Geometric :: BuilderIF :: iterator (const IteratorSTI < helement_STI > * w) const {
  return new AlignIterator < AlignIterator < ListIterator < hexa_GEO >, ListIterator < tetra_GEO >, helement_STI >,
    AlignIterator < ListIterator < periodic3_GEO >, ListIterator < periodic4_GEO >, helement_STI >, helement_STI >
    (*(const AlignIterator < AlignIterator < ListIterator < hexa_GEO >, ListIterator < tetra_GEO >, helement_STI >,
    AlignIterator < ListIterator < periodic3_GEO >, ListIterator < periodic4_GEO >, helement_STI >, helement_STI > *) w) ;
}
  // *** Neu: Iterator der nur die "echten" Elemente (ohne period. R"ander)
  //     iteriert.
  
IteratorSTI < Gitter :: helement_STI > * Gitter :: Geometric :: BuilderIF :: pureElementIterator (const helement_STI *) const {

    // Vorsicht ! Falls diese Methode ver"andert wird, muss die selbe "Anderung
    // in der Kopiermethode (nachfolgend) vorgenommen werden. Sonst f"uhrt die 
    // statische Typkonversion zu einem schwer nachvollziehbaren Fehler.

  ListIterator < hexa_GEO > w1 (_hexaList) ;
  ListIterator < tetra_GEO > w2 (_tetraList) ;
  
  return new AlignIterator < ListIterator < hexa_GEO >, ListIterator < tetra_GEO >, helement_STI > (w1,w2) ;
}

IteratorSTI < Gitter :: helement_STI > * Gitter :: Geometric :: BuilderIF :: pureElementIterator (const IteratorSTI < helement_STI > * w) const {
  return new AlignIterator < ListIterator < hexa_GEO >, ListIterator < tetra_GEO >, helement_STI > 
    (*(const AlignIterator < ListIterator < hexa_GEO >, ListIterator < tetra_GEO >, helement_STI > *)w) ;
}
  // ***

void Gitter :: Geometric :: BuilderIF :: backupCMode (ostream & os) const {

  // Das Compatibility Mode Backup sichert das Makrogitter genau
  // dann, wenn es zwischenzeitlich ge"andert wurde, was beim
  // Neuanlegen und bei der Lastverteilung der Fall ist.

  map < VertexGeo *, int, less < VertexGeo * > > vm ;
  os.setf (ios::fixed, ios::floatfield) ;
  os.precision (16) ;
  
  // Bisher enth"alt die erste Zeile der Datei entweder "!Tetraeder"
  // oder "!Hexaeder" je nachdem, ob ein reines Tetraeder- oder
  // Hexaedernetz vorliegt. Gemischte Netze sind bez"uglich ihres
  // Dateiformats noch nicht spezifiziert.
  
  if (_tetraList.size () == 0) {
    os << "!Hexaeder" << endl ;
  } else if (_hexaList.size () == 0 && _tetraList.size () != 0) {
    os << "!Tetraeder" << endl ;
  } else {
    cerr << "**WARNUNG (IGNORIERT) Gitter :: Geometric :: BuilderIF :: backupCMode (ostream &)" ;
    cerr << "  schreibt nur entweder reine Hexaedernetze oder reine Tetraedernetze." ;
    cerr << " In " << __FILE__ << " " << __LINE__ << endl ;
  }
  
  // In jedem Fall die Vertexkoordinaten rausschreiben.
  
  os << _vertexList.size () << endl ;
  {
    int index (0) ;
    for (list < VertexGeo * > :: const_iterator i = _vertexList.begin () ; i != _vertexList.end () ; i ++) {
      os << (*i)->Point ()[0] << " " << (*i)->Point ()[1] << " " << (*i)->Point ()[2] << endl ;
      vm [*i] = index ++ ;
    }
  }
  if (_tetraList.size () == 0) {
    assert (_hbndseg3List.size () == 0) ;
    os << _hexaList.size () << endl ;
    {
      for (list < hexa_GEO * > :: const_iterator i = _hexaList.begin () ; i != _hexaList.end () ; i ++ ) {
        for (int j = 0 ; j < 8 ; os << vm [(*i)->myvertex (j ++)] << "  ") ;
        os << endl ;
      }
    }
// Anfang - Neu am 23.5.02 (BS)
    os << _hbndseg4List.size () + _periodic4List.size () << endl ;
// Ende - Neu am 23.5.02 (BS)
    {
      for (list < hbndseg4_GEO * > :: const_iterator i = _hbndseg4List.begin () ; i != _hbndseg4List.end () ; i ++) {
        os << -(int)(*i)->bndtype () << "  " << 4 << "  " ;
        for (int j = 0 ; j < 4 ; os << vm [(*i)->myvertex (0,j ++)] << "  ") ;
        os << endl ;
      }
    }
// Anfang - Neu am 23.5.02 (BS)
    {
      for (list < periodic4_GEO * > :: const_iterator i = _periodic4List.begin () ; i != _periodic4List.end () ; i ++) {
        os << -(int)(hbndseg :: periodic) << "  " << 8 << "  " ;
        for (int j = 0 ; j < 8 ; os << vm [(*i)->myvertex (j ++)] << "  ") ;
        os << endl ;
      }
    }
// Ende - Neu am 23.5.02 (BS)
  } else if (_hexaList.size () == 0 && _tetraList.size () != 0) {
    os << _tetraList.size () << endl ;
    {
      for (list < tetra_GEO * > :: const_iterator i = _tetraList.begin () ; i != _tetraList.end () ; i ++ ) {
        for (int j = 0 ; j < 4 ; os << vm [(*i)->myvertex (j ++)] << "  ") ;
        os << endl ;
      }
    }
    os << _hbndseg3List.size () + _periodic3List.size () << endl ;
    {
      for (list < hbndseg3_GEO * > :: const_iterator i = _hbndseg3List.begin () ; i != _hbndseg3List.end () ; i ++) {
        os << -(int)(*i)->bndtype () << "  " << 3 << "  " ;
        for (int j = 0 ; j < 3 ; os << vm [(*i)->myvertex (0,j ++)] << "  ") ;
        os << endl ;
      }
    }
    {
      for (list < periodic3_GEO * > :: const_iterator i = _periodic3List.begin () ; i != _periodic3List.end () ; i ++) {
        os << -(int)(hbndseg :: periodic) << "  " << 6 << "  " ;
        for (int j = 0 ; j < 6 ; os << vm [(*i)->myvertex (j ++)] << "  ") ;
        os << endl ;
      }
    }
  }
  {
    // Die Vertexidentifierliste hinten anh"angen, damit ein verteiltes
  // Grobgitter wieder zusammengefunden wird.
  
    for (list < VertexGeo * > :: const_iterator i = _vertexList.begin () ; i != _vertexList.end () ; i ++)
      os << (*i)->ident () << " " << -1 << endl ;
  }
  // Die Modified - Markierung zur"ucksetzen.
  // Bisher leider noch mit 'cast around const' feature.
  
//  ((BuilderIF *)this)->_modified = false ;
  return ;
}


void Gitter :: Geometric :: BuilderIF :: backupCMode (const char * filePath, const char * fileName) const {
  if (_modified) {
    char * name = new char [strlen (filePath) + strlen (fileName) + 20] ;
    sprintf (name, "%smacro.%s", filePath, fileName) ;
    ofstream out (name) ;
    if (out) {
      backupCMode (out) ;
    } else {
      cerr << "**WARNUNG (IGNORIERT) in Gitter :: Geometric :: BuilderIF :: backupCMode (const char *, const char *)" ;
      cerr << " beim Anlegen der Datei < " << name << " > in " << __FILE__ << " " << __LINE__ << endl ;
    }
    delete [] name ;
  }
  return ;
}

void Gitter :: Geometric :: BuilderIF :: backup (ostream & os) const {

  // Man sollte sich erstmal darauf einigen, wie ein allgemeines Gitterdateiformat 
  // auszusehen hat f"ur Hexaeder, Tetraeder und alle m"oglichen Pyramiden.
  // Solange leiten wir die Methodenaufrufe auf backupCMode (..) um.
  
  cerr << "**WARNUNG (IGNORIERT) Gitter :: Geometric :: BuilderIF :: backup (ostream &) " ;
  cerr << " nach Gitter :: Geometric :: BuilderIF :: backupCMode (ostream &) umgeleitet " << endl ;
  
  backupCMode (os) ;
  return ;
}

void Gitter :: Geometric :: BuilderIF :: backup (const char * filePath, const char * fileName) const 
{
  cerr << "**WARNUNG (IGNORIERT) Gitter :: Geometric :: BuilderIF :: backup (const char *) " ;
  cerr << " nach Gitter :: Geometric :: BuilderIF :: backupCMode (const char *, const char *) umgeleitet " << endl ;

  backupCMode (filePath, fileName) ;
  return ;
}









