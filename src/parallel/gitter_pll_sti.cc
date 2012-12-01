// (c) bernhard schupp 1997 - 1998
// modifications for dune interface 
// (c) Robert Kloefkorn 2004 - 2005 
#ifndef _GITTER_PLL_STI_CC_
#define _GITTER_PLL_STI_CC_

#include "gitter_pll_sti.h"
#include "walk.h"

int __STATIC_myrank = -1 ;
int __STATIC_turn   = -1 ;
int __STATIC_phase  = -1 ;

pair < IteratorSTI < GitterPll :: vertex_STI > *, IteratorSTI < GitterPll :: vertex_STI > *> GitterPll :: 
  iteratorTT (const GitterPll :: vertex_STI *, int l) {

  vector < IteratorSTI < vertex_STI > * > _iterators_inner, _iterators_outer ;

  _iterators_inner.push_back (new AccessIteratorTT < vertex_STI > :: InnerHandle (containerPll (), l)) ;
  _iterators_outer.push_back (new AccessIteratorTT < vertex_STI > :: OuterHandle (containerPll (), l)) ;
  {
    AccessIteratorTT < hedge_STI > :: InnerHandle mie (containerPll (), l) ;
    AccessIteratorTT < hedge_STI > :: OuterHandle moe (containerPll (), l) ;

    Insert < AccessIteratorTT < hedge_STI > :: InnerHandle, 
             TreeIterator < hedge_STI, has_int_vertex < hedge_STI > > > lie (mie) ;
    Insert < AccessIteratorTT < hedge_STI > :: OuterHandle, 
             TreeIterator < hedge_STI, has_int_vertex < hedge_STI > > > loe (moe) ;
    _iterators_inner.push_back (new Wrapper < Insert < AccessIteratorTT < hedge_STI > :: InnerHandle, 
                                              TreeIterator < hedge_STI, has_int_vertex < hedge_STI > > >, InternalVertex > (lie)) ;
    _iterators_outer.push_back (new Wrapper < Insert < AccessIteratorTT < hedge_STI > :: OuterHandle,
                                              TreeIterator < hedge_STI, has_int_vertex < hedge_STI > > >, InternalVertex > (loe)) ;
  }
  {
    AccessIteratorTT < hface_STI > :: InnerHandle mfi (containerPll (), l) ;
    AccessIteratorTT < hface_STI > :: OuterHandle mfo (containerPll (), l) ;
    {
      Insert < AccessIteratorTT < hface_STI > :: InnerHandle, 
               TreeIterator < hface_STI, has_int_vertex < hface_STI > > > lfi (mfi) ;
      Insert < AccessIteratorTT < hface_STI > :: OuterHandle, 
               TreeIterator < hface_STI, has_int_vertex < hface_STI > > > lfo (mfo) ;

      _iterators_inner.push_back (new Wrapper < Insert < AccessIteratorTT < hface_STI > :: InnerHandle,
                      TreeIterator < hface_STI, has_int_vertex < hface_STI > > >, InternalVertex > (lfi)) ;
      _iterators_outer.push_back (new Wrapper < Insert < AccessIteratorTT < hface_STI > :: OuterHandle,
                      TreeIterator < hface_STI, has_int_vertex < hface_STI > > >, InternalVertex > (lfo)) ;
    }
    {
      Insert < AccessIteratorTT < hface_STI > :: InnerHandle, 
               TreeIterator < hface_STI, has_int_edge < hface_STI > > > lfi (mfi) ;
      Insert < AccessIteratorTT < hface_STI > :: OuterHandle, 
               TreeIterator < hface_STI, has_int_edge < hface_STI > > > lfo (mfo) ;
      Wrapper < Insert < AccessIteratorTT < hface_STI > :: InnerHandle, 
                TreeIterator < hface_STI, has_int_edge < hface_STI > > >, InternalEdge > dlfi (lfi) ;
      Wrapper < Insert < AccessIteratorTT < hface_STI > :: OuterHandle,
                TreeIterator < hface_STI, has_int_edge < hface_STI > > >, InternalEdge > dlfo (lfo) ;
      Insert < Wrapper < Insert < AccessIteratorTT < hface_STI > :: InnerHandle, 
               TreeIterator < hface_STI, has_int_edge < hface_STI > > >, InternalEdge >,
      TreeIterator < hedge_STI, has_int_vertex < hedge_STI > > > vdlfi (dlfi) ;
      Insert < Wrapper < Insert < AccessIteratorTT < hface_STI > :: OuterHandle,
               TreeIterator < hface_STI, has_int_edge < hface_STI > > >, InternalEdge >,
               TreeIterator < hedge_STI, has_int_vertex < hedge_STI > > > vdlfo (dlfo) ;

      _iterators_inner.push_back (new Wrapper < Insert < Wrapper < 
                Insert < AccessIteratorTT < hface_STI > :: InnerHandle, 
                TreeIterator < hface_STI, has_int_edge < hface_STI > > >, InternalEdge >,
                TreeIterator < hedge_STI, has_int_vertex < hedge_STI > > >, InternalVertex > (vdlfi)) ;

      _iterators_outer.push_back (new Wrapper < 
        Insert < Wrapper < Insert < AccessIteratorTT < hface_STI > :: OuterHandle,
        TreeIterator < hface_STI, has_int_edge < hface_STI > > >, InternalEdge >,
        TreeIterator < hedge_STI, has_int_vertex < hedge_STI > > >, InternalVertex > (vdlfo)) ;
    }
  }
  return pair < IteratorSTI < vertex_STI > *, IteratorSTI < vertex_STI > * > 
  (new VectorAlign < vertex_STI > (_iterators_inner), new VectorAlign < vertex_STI > (_iterators_outer)) ;
}

pair < IteratorSTI < GitterPll :: hedge_STI > *, IteratorSTI < GitterPll :: hedge_STI > * > GitterPll ::
  iteratorTT (const GitterPll :: hedge_STI * fakep, int l) 
{
  // fakerule is only for type determination 
  is_leaf < hedge_STI > * rule = 0;
  // see gitter_pll_sti.h 
  return createEdgeIteratorTT(rule,l); 
}

pair < IteratorSTI < GitterPll :: hface_STI > *, IteratorSTI < GitterPll :: hface_STI > *> 
  GitterPll :: iteratorTT (const GitterPll :: hface_STI *, int l) 
{
  is_leaf< hface_STI > rule; 
  return this->createFaceIteratorTT(rule, l);
}

void GitterPll :: printSizeTT () {
  cout << "\n GitterPll :: printSizeTT () \n\n" ;
  mpAccess ().printLinkage (cout) ;
  cout << endl ;
  { for (int l = 0 ; l < mpAccess ().nlinks () ; l ++ ) {
    LeafIteratorTT < vertex_STI > w (*this, l) ;
    cout << "me: " << mpAccess ().myrank () << " link: " << l << " vertices: [inner|outer] " << w.inner ().size () << " " << w.outer ().size () << endl ;
  }}
  { for (int l = 0 ; l < mpAccess ().nlinks () ; l ++ ) {
    LeafIteratorTT < hedge_STI > w (*this, l) ;
    cout << "me: " << mpAccess ().myrank () << " link: " << l << " edges:   [inner|outer] " << w.inner ().size () << " " << w.outer ().size () << endl ;
  }}
  { for (int l = 0 ; l < mpAccess ().nlinks () ; l ++ ) {
    LeafIteratorTT < hface_STI > w (*this, l) ;
    cout << "me: " << mpAccess ().myrank () << " link: " << l << " faces: [inner|outer] " << w.inner ().size () << " " << w.outer ().size () << endl ;
  }}
  return ;
}

void GitterPll :: printsize () 
{
  const int me = mpAccess ().myrank (), np = mpAccess ().psize (), nl = mpAccess ().nlinks () ;
  
  if (debugOption (10)) Gitter :: printsize () ;
  vector < int > n ;
  {
    int sum = 0 ;
    for (int i = 0 ; i < nl ; ++i)
      sum += LeafIteratorTT < vertex_STI > (*this, i).outer ().size () ;
    n.push_back (LeafIterator < vertex_STI > (*this)->size() - sum) ;
  }
  {
    int sum = 0 ;
    for (int i = 0 ; i < nl ; ++i)
      sum += LeafIteratorTT < hedge_STI > (*this, i).outer ().size () ;
    n.push_back (LeafIterator < hedge_STI > (*this)->size() - sum) ;
  }
  int sumCutFaces = 0 ;
  {
    int sum = 0 ;
    for (int i = 0 ; i < nl ; ++i) {
      LeafIteratorTT < hface_STI > w (*this, i) ;
      sum += w.outer ().size () ;
      sumCutFaces += w.outer ().size () ;
      sumCutFaces += w.inner ().size () ;
    }
    n.push_back (LeafIterator < hface_STI > (*this)->size() - sum) ;
  }
  n.push_back (LeafIterator < helement_STI > (*this)->size()) ;
  n.push_back (LeafIterator < hbndseg_STI > (*this)->size() - sumCutFaces) ;

  {
    cout << "\nP[" << me << "] GitterPll :: printSize () : \n\n" ;
    cout << " - Elements ......... "  << n[3] << "\n" ;
    cout << " - Boundaries ....... "  << n[4] << "\n" ;
    cout << " - Faces ............ "  << n[2] << "\n" ;
    cout << " - Edges ............ "  << n[1] << "\n" ;
    cout << " - Vertices ......... "  << n[0] << "\n" ;
    cout << endl ;
  }

  // could better use MPI_gather here 
  vector < vector < int > > in = mpAccess ().gcollect (n) ;
  assert (static_cast<int> (in.size ()) == np) ;

  if (me == 0) 
  {
    int nv = 0, nd = 0, nf = 0, ne = 0, nb = 0 ;
    for (int i = 0 ; i < np ; ++i) {
      nv += (in [i])[0] ;
      nd += (in [i])[1] ;
      nf += (in [i])[2] ;
      ne += (in [i])[3] ;
      nb += (in [i])[4] ;
    }
    cout << "\nSummary -- GitterPll :: printSize () : \n\n" ;
    cout << " - Elements ......... " << ne << "\n" ;
    cout << " - Boundaries ....... " << nb << "\n" ;
    cout << " - Faces ............ " << nf << "\n" ;
    cout << " - Edges ............ " << nd << "\n" ;
    cout << " - Vertices ......... " << nv << "\n" ;
    cout << endl ;
  }
  return ;
}

void GitterPll :: fullIntegrityCheck () {
  int start = clock () ;
  Gitter :: fullIntegrityCheck () ;
  containerPll().fullIntegrityCheck (mpAccess ()) ;
  if (debugOption (0)) {
    cout << "**INFO GitterPll :: fullIntegrityCheck () used: " << (float)((float)(clock() - start)/(float)(CLOCKS_PER_SEC)) << " sec." << endl ;
  }
  return ;
}

pair < IteratorSTI < Gitter :: vertex_STI > *, IteratorSTI < Gitter :: vertex_STI > * >
  GitterPll :: MacroGitterPll :: iteratorTT (const vertex_STI *, int i) {
  assert (i < static_cast<int> (_vertexTT.size ()) ) ;
  return pair < IteratorSTI < vertex_STI > *, IteratorSTI < vertex_STI > * > 
  (new listSmartpointer__to__iteratorSTI < vertex_STI > (_vertexTT [i].first), 
         new listSmartpointer__to__iteratorSTI < vertex_STI > (_vertexTT [i].second)) ;
}

pair < IteratorSTI < Gitter :: vertex_STI > *, IteratorSTI < Gitter :: vertex_STI > * >
  GitterPll :: MacroGitterPll :: iteratorTT (const pair < IteratorSTI < vertex_STI > *, IteratorSTI < vertex_STI > * > & p, int) {
  return pair < IteratorSTI < vertex_STI > *, IteratorSTI < vertex_STI > * > 
  (new listSmartpointer__to__iteratorSTI < vertex_STI > (*(const listSmartpointer__to__iteratorSTI < vertex_STI > *)p.first),
         new listSmartpointer__to__iteratorSTI < vertex_STI > (*(const listSmartpointer__to__iteratorSTI < vertex_STI > *)p.second)) ;
}

pair < IteratorSTI < Gitter :: hedge_STI > *, IteratorSTI < Gitter :: hedge_STI > * >
  GitterPll :: MacroGitterPll :: iteratorTT (const hedge_STI *, int i) {
  assert (i < static_cast<int> (_hedgeTT.size ())) ;
  return pair < IteratorSTI < hedge_STI > *, IteratorSTI < hedge_STI > * > 
  (new listSmartpointer__to__iteratorSTI < hedge_STI > (_hedgeTT [i].first),
         new listSmartpointer__to__iteratorSTI < hedge_STI > (_hedgeTT [i].second)) ;
}

pair < IteratorSTI < Gitter :: hedge_STI > *, IteratorSTI < Gitter :: hedge_STI > * > 
  GitterPll :: MacroGitterPll :: iteratorTT (const pair < IteratorSTI < hedge_STI > *, IteratorSTI < hedge_STI > * > & p, int) {
  return pair < IteratorSTI < hedge_STI > *, IteratorSTI < hedge_STI > * > 
  (new listSmartpointer__to__iteratorSTI < hedge_STI > (*(const listSmartpointer__to__iteratorSTI < hedge_STI > *)p.first),
         new listSmartpointer__to__iteratorSTI < hedge_STI > (*(const listSmartpointer__to__iteratorSTI < hedge_STI > *)p.second)) ;
}

pair < IteratorSTI < Gitter :: hface_STI > *, IteratorSTI < Gitter :: hface_STI > * >
  GitterPll :: MacroGitterPll :: iteratorTT (const hface_STI *, int i) {
  assert (i < static_cast<int> (_hfaceTT.size ())) ;
  return pair < IteratorSTI < hface_STI > *, IteratorSTI < hface_STI > * > 
  (new listSmartpointer__to__iteratorSTI < hface_STI > (_hfaceTT [i].first),
   new listSmartpointer__to__iteratorSTI < hface_STI > (_hfaceTT [i].second)) ;
}

pair < IteratorSTI < Gitter :: hface_STI > *, IteratorSTI < Gitter :: hface_STI > * > 
  GitterPll :: MacroGitterPll :: iteratorTT (const pair < IteratorSTI < hface_STI > *, IteratorSTI < hface_STI > * > & p, int) {
  return pair < IteratorSTI < hface_STI > *, IteratorSTI < hface_STI > * >
    (new listSmartpointer__to__iteratorSTI < hface_STI > (*(const listSmartpointer__to__iteratorSTI < hface_STI > *)p.first),
     new listSmartpointer__to__iteratorSTI < hface_STI > (*(const listSmartpointer__to__iteratorSTI < hface_STI > *)p.second)) ;
}

bool GitterPll :: refine () 
{
  assert (debugOption (5) ? (cout << "**INFO GitterPll :: refine () " << endl, 1) : 1) ;
  const int nl = mpAccess ().nlinks () ;
  bool state = false ;
  vector < vector < hedge_STI * > > innerEdges (nl), outerEdges (nl) ;
  vector < vector < hface_STI * > > innerFaces (nl), outerFaces (nl) ;

  typedef vector < hedge_STI * > :: const_iterator hedge_iterator ;
  typedef vector < hface_STI * > :: const_iterator hface_iterator ;

  {
    // Erst die Zeiger auf alle Fl"achen und Kanten mit paralleler
    // Mehrdeutigkeit sichern, da die LeafIteratorTT < . > nach dem 
    // Verfeinern auf gitter nicht mehr stimmen werden. Die Technik
    // ist zul"assig, da keine mehrfache Verfeinerung entstehen kann.
  
    {
      for (int l = 0 ; l < nl ; ++l) 
      {
        //cout << "refinepll \n";
        LeafIteratorTT < hface_STI > fw (*this,l) ;
        LeafIteratorTT < hedge_STI > dw (*this,l) ;

        // reserve memory first 
        outerFaces[l].reserve( fw.outer().size() );
        innerFaces[l].reserve( fw.inner().size() );
        
        for (fw.outer ().first () ; ! fw.outer().done () ; fw.outer ().next ())
          outerFaces [l].push_back (& fw.outer ().item ()) ;
        for (fw.inner ().first () ; ! fw.inner ().done () ; fw.inner ().next ())
          innerFaces [l].push_back (& fw.inner ().item ()) ;

        // reserve memory first 
        outerEdges[l].reserve( dw.outer().size() );
        innerEdges[l].reserve( dw.inner().size() );
        
        for (dw.outer ().first () ; ! dw.outer().done () ; dw.outer ().next ())
          outerEdges [l].push_back (& dw.outer ().item ()) ;
        for (dw.inner ().first () ; ! dw.inner ().done () ; dw.inner ().next ())
          innerEdges [l].push_back (& dw.inner ().item ()) ;
      }
    }
    // jetzt normal verfeinern und den Status der Verfeinerung
    // [unvollst"andige / vollst"andige Verfeinerung] sichern.
    
    __STATIC_phase = 1 ;
    
    state = Gitter :: refine () ;
       
    // Phase des Fl"achenausgleichs an den Schnittfl"achen des
    // verteilten Gitters. Weil dort im sequentiellen Fall pseudorekursive
    // Methodenaufrufe vorliegen k"onnen, muss solange iteriert werden,
    // bis die Situation global station"ar ist.
  
    __STATIC_phase = 2 ;
  
    bool repeat (false) ;
    _refineLoops = 0 ;
    do {
      repeat = false ;
      {
        vector < ObjectStream > osv (nl) ;
        try {
          for (int l = 0 ; l < nl ; ++l) 
          {
            ObjectStream& os = osv[ l ];
            // reserve memory for object stream 
            os.reserve( (outerFaces[l].size() + innerFaces[l].size() ) * sizeof(char) );
            {
              const hface_iterator iEnd = outerFaces[l].end () ;
              for (hface_iterator i = outerFaces [l].begin () ; i != iEnd; ++i ) 
                (*i)->accessOuterPllX ().first->getRefinementRequest ( os ) ; 
            }
            {
              const hface_iterator iEnd = innerFaces[l].end () ;
              for (hface_iterator i = innerFaces [l].begin () ; i != iEnd ; ++i )
                (*i)->accessOuterPllX ().first->getRefinementRequest ( os ) ; 
            }
          }
        } 
        catch (Parallel :: AccessPllException) 
        {
          cerr << "**FEHLER (FATAL) AccessPllException in " << __FILE__ << " " << __LINE__ << endl ; abort () ;
        }
  
        // exchange data 
        osv = mpAccess ().exchange (osv) ;
  
        try 
        {
          for (int l = 0 ; l < nl ; ++l) 
          {
            ObjectStream& os = osv [l];
            {
              const hface_iterator iEnd = innerFaces[l].end () ;
              for (hface_iterator i = innerFaces [l].begin () ; i != iEnd; ++i ) 
                repeat |= (*i)->accessOuterPllX ().first->setRefinementRequest ( os ) ; 
            }
            {
              const hface_iterator iEnd = outerFaces[l].end () ; 
              for (hface_iterator i = outerFaces [l].begin () ; i != iEnd; ++i )
                repeat |= (*i)->accessOuterPllX ().first->setRefinementRequest ( os ) ; 
            }
          }
        } 
        catch (Parallel :: AccessPllException) 
        {
          cerr << "**FEHLER (FATAL) AccessPllException in " << __FILE__ << " " << __LINE__ << endl ; abort () ;
        }
      }

      _refineLoops ++ ;
    } 
    while (mpAccess ().gmax ( repeat ) ) ;

    // Jetzt noch die Kantensituation richtigstellen, es gen"ugt ein Durchlauf,
    // weil die Verfeinerung einer Kante keine Fernwirkungen hat. Vorsicht: Die
    // Kanten sind bez"uglich ihrer Identifikation sternf"ormig organisiert, d.h.
    // es muss die Verfeinerungsinformation einmal am Eigent"umer gesammelt und
    // dann wieder zur"ucktransportiert werden, eine einfache L"osung, wie bei
    // den Fl"achen (1/1 Beziehung) scheidet aus.

    __STATIC_phase = 3 ;

    {
      vector < ObjectStream > osv (nl) ;
      {
        for (int l = 0 ; l < nl ; ++l) 
        {
          ObjectStream& os = osv[ l ];
          const hedge_iterator iEnd = outerEdges[l].end () ;
          for (hedge_iterator i = outerEdges [l].begin () ; i != iEnd; ++i )
            (*i)->getRefinementRequest ( os ) ;
        }
      }
      
      // exchange data 
      osv = mpAccess ().exchange (osv) ;
      
      {
        for (int l = 0 ; l < nl ; ++l)
        {
          ObjectStream& os = osv[ l ];
          const hedge_iterator iEnd = innerEdges[l].end () ;
          for (hedge_iterator i = innerEdges [l].begin () ; i != iEnd; ++i )
            (*i)->setRefinementRequest ( os ) ;
        }
      }
    } // ~vector < ObjectStream > ... 
     
    {
      vector < ObjectStream > osv (nl) ;
      {
        for (int l = 0 ; l < nl ; ++l)
        {
          ObjectStream& os = osv[ l ];
          // reserve memory 
          os.reserve( innerEdges[l].size() * sizeof(char) );

          const hedge_iterator iEnd = innerEdges[l].end () ;
          for (hedge_iterator i = innerEdges [l].begin () ; i != iEnd; ++i ) 
            (*i)->getRefinementRequest ( os ) ;
        }
      }
      
      // exchange data 
      osv = mpAccess ().exchange (osv) ;
      
      {
        for (int l = 0 ; l < nl ; ++l)
        {
          ObjectStream& os = osv[ l ];
          const hedge_iterator iEnd = outerEdges [l].end () ;
          for (hedge_iterator i = outerEdges [l].begin () ; i != iEnd; ++i ) 
            (*i)->setRefinementRequest ( os ) ;
        }
      }
    }   // ~vector < ObjectStream > ... 
  }
  
  __STATIC_phase = -1 ;
  
  return state ;
}

class EdgeFlagExchange : public GatherScatterType
{
public:
  EdgeFlagExchange () {}
  virtual ~EdgeFlagExchange () {}

  // type of used object stream 
  typedef GatherScatterType :: ObjectStreamType   ObjectStreamType;

  // only contains edge information 
  virtual bool contains ( int dim, int codim ) const { return codim == 2 ; }
  // every element is contained 
  virtual bool containsItem(const HEdgeType   & elem ) const { return true; }
  // send does pack the no edge coarsening flag 
  virtual void sendData ( ObjectStreamType & str , HEdgeType & edge ) 
  {
    str.put( char(edge.noCoarsen()) );
  }

  // receive gets flag and disabled coarsen if flag is set 
  virtual void recvData ( ObjectStreamType & str , HEdgeType & edge ) 
  {
    const bool noCoarsen = bool( str.get() );
    if( noCoarsen ) 
      edge.disableEdgeCoarsen() ;
  }

  // this method is only needed for ghost cells 
  virtual void setData  ( ObjectStreamType & str , HEdgeType & elem ) 
  { 
    cout << "ERROR: EdgeFlagExchange::setData was called in " << __FILE__ << " " << __LINE__ << endl;
    abort();
  }
};

void GitterPll :: coarse () 
{
  assert (debugOption (20) ? (cout << "**INFO GitterDunePll :: coarse () " << endl, 1) : 1) ;
  const int nl = mpAccess ().nlinks () ;

  typedef vector < hedge_STI * > :: iterator hedge_iterator ;
  typedef vector < hface_STI * > :: iterator hface_iterator ;

  {
    vector < vector < hedge_STI * > > innerEdges (nl), outerEdges (nl) ;
    vector < vector < hface_STI * > > innerFaces (nl), outerFaces (nl) ;
  
    for (int l = 0 ; l < nl ; ++l) 
    {
    
      // Zun"achst werden f"ur alle Links die Zeiger auf Gitterojekte mit
      // Mehrdeutigkeit gesichert, die an der Wurzel einer potentiellen
      // Vergr"oberungsoperation sitzen -> es sind die Knoten in der Hierarchie,
      // deren Kinder alle Bl"atter sind. Genau diese Knoten sollen gegen"uber
      // der Vergr"oberung blockiert werden und dann die Vergr"oberung falls
      // sie zul"assig ist, sp"ater durchgef"uhrt werden (pending) ;
    
      AccessIteratorTT < hface_STI > :: InnerHandle mfwi (containerPll (),l) ;
      AccessIteratorTT < hface_STI > :: OuterHandle mfwo (containerPll (),l) ;
      AccessIteratorTT < hedge_STI > :: InnerHandle mdwi (containerPll (),l) ;
      AccessIteratorTT < hedge_STI > :: OuterHandle mdwo (containerPll (),l) ;
      
      // Die inneren und a"usseren Iteratoren der potentiell vergr"oberungsf"ahigen
      // Fl"achen "uber den Grobgitterfl"achen. In den Elementen passiert erstmal
      // nichts, solange nicht mit mehrfachen Grobgitterelementen gearbeitet wird.
      
      Insert < AccessIteratorTT < hface_STI > :: InnerHandle, 
        TreeIterator < hface_STI, childs_are_leafs < hface_STI > > > fwi (mfwi) ;
      Insert < AccessIteratorTT < hface_STI > :: OuterHandle, 
        TreeIterator < hface_STI, childs_are_leafs < hface_STI > > > fwo (mfwo) ;
      
      // Die inneren und a"usseren Iteratoren der potentiell vergr"oberungsf"ahigen
      // Kanten "uber den Grobgitterkanten.
      
      Insert < AccessIteratorTT < hedge_STI > :: InnerHandle, 
        TreeIterator < hedge_STI, childs_are_leafs < hedge_STI > > > dwi (mdwi) ;
      Insert < AccessIteratorTT < hedge_STI > :: OuterHandle, 
        TreeIterator < hedge_STI, childs_are_leafs < hedge_STI > > > dwo (mdwo) ;

      // Die inneren und a"usseren Iteratoren der potentiell vergr"oberungsf"ahigen
      // Kanten "uber den Grobgitterfl"achen. Diese Konstruktion wird beim Tetraeder-
      // gitter notwendig, weil dort keine Aussage der Form:
      //

      Insert < AccessIteratorTT < hface_STI > :: InnerHandle, 
        TreeIterator < hface_STI, has_int_edge < hface_STI > > > efi (mfwi) ;
      Insert < AccessIteratorTT < hface_STI > :: OuterHandle, 
        TreeIterator < hface_STI, has_int_edge < hface_STI > > > efo (mfwo) ;
      Wrapper < Insert < AccessIteratorTT < hface_STI > :: InnerHandle, 
        TreeIterator < hface_STI, has_int_edge < hface_STI > > >, InternalEdge > eifi (efi) ;
      Wrapper < Insert < AccessIteratorTT < hface_STI > :: OuterHandle, 
        TreeIterator < hface_STI, has_int_edge < hface_STI > > >, InternalEdge > eifo (efo) ;
      Insert < Wrapper < Insert < AccessIteratorTT < hface_STI > :: InnerHandle, 
        TreeIterator < hface_STI, has_int_edge < hface_STI > > >, InternalEdge >,
      TreeIterator < hedge_STI, childs_are_leafs < hedge_STI > > > dfi (eifi) ;
        Insert < Wrapper < Insert < AccessIteratorTT < hface_STI > :: OuterHandle, 
          TreeIterator < hface_STI, has_int_edge < hface_STI > > >, InternalEdge >,
      TreeIterator < hedge_STI, childs_are_leafs < hedge_STI > > > dfo (eifo) ;

      // Die 'item ()' Resultatwerte (Zeiger) werden in Vektoren gesichert, weil die
      // Kriterien die zur Erzeugung der Iteratoren angewendet wurden (Filter) nach
      // einer teilweisen Vergr"oberung nicht mehr g"ultig sein werden, d.h. die 
      // Iterationsobjekte "andern w"ahrend der Vergr"oberung ihre Eigenschaften.
      // Deshalb werden sie auch am Ende des Blocks aufgegeben. Der Vektor 'cache'
      // ist zul"assig, weil kein Objekt auf das eine Referenz im 'cache' vorliegt
      // beseitigt werden kann. Sie sind alle ein Niveau darunter.

      // reserve memory first 
      innerFaces [l].reserve( fwi.size() );
      outerFaces [l].reserve( fwo.size() );
        
      for (fwi.first () ; ! fwi.done () ; fwi.next ()) innerFaces [l].push_back (& fwi.item ()) ;
      for (fwo.first () ; ! fwo.done () ; fwo.next ()) outerFaces [l].push_back (& fwo.item ()) ;

      // reserve memory first 
      innerEdges[l].reserve( dwi.size() + dfi.size() );
      outerEdges[l].reserve( dwo.size() + dfo.size() );

      for (dwo.first () ; ! dwo.done () ; dwo.next ()) outerEdges [l].push_back (& dwo.item ()) ;
      for (dfo.first () ; ! dfo.done () ; dfo.next ()) outerEdges [l].push_back (& dfo.item ()) ;
      for (dwi.first () ; ! dwi.done () ; dwi.next ()) innerEdges [l].push_back (& dwi.item ()) ;
      for (dfi.first () ; ! dfi.done () ; dfi.next ()) innerEdges [l].push_back (& dfi.item ()) ;
    }

    // first check edges that cannot be coarsened 
    // due to bisection rule (only enabled for bisection)
    // communicate edge flags if bisection is enabled 
    if( Gitter :: markEdgeCoarsening() ) 
    {
      // see class implementation in this file above 
      EdgeFlagExchange dataHandle ;
      // this communicates the edge no coarsen flags 
      borderBorderCommunication( dataHandle, dataHandle, dataHandle, dataHandle );
    }

    try 
    {
      // Erstmal alles was mehrdeutig ist, gegen die drohende Vergr"oberung sichern.
      // Danach werden sukzessive die Fl"achenlocks aufgehoben, getestet und
      // eventuell vergr"obert, dann das gleiche Spiel mit den Kanten.

      for (int l = 0 ; l < nl ; ++l) 
      {
        {
          const hedge_iterator iEnd = outerEdges [l].end () ;
          for (hedge_iterator i = outerEdges [l].begin () ; i != iEnd; ++i )
          {
            (*i)->lockAndTry () ; 
          }
        }
        {
          const hedge_iterator iEnd = innerEdges [l].end () ;
          for (hedge_iterator i = innerEdges [l].begin () ; i != iEnd; ++i )
          {
            (*i)->lockAndTry () ; 
          }
        }
        {
          const hface_iterator iEnd = outerFaces [l].end () ;
          for (hface_iterator i = outerFaces [l].begin () ; i != iEnd; ++i )
            (*i)->accessOuterPllX ().first->lockAndTry () ; 
        }
        {
          const hface_iterator iEnd = innerFaces [l].end () ;
          for (hface_iterator i = innerFaces [l].begin () ; i != iEnd; ++i )
            (*i)->accessOuterPllX ().first->lockAndTry () ; 
        }
      }

      // Gitter :: coarse () ist elementorientiert, d.h. die Vergr"oberung auf Fl"achen und
      // Kanten wird nur durch Vermittlung eines sich vergr"obernden Knotens in der Element-
      // hierarchie angestossen. In allen gegen Vergr"oberung 'gelockten' Fl"achen und Kanten
      // wird die angeforderte Operation zur"uckgewiesen, um erst sp"ater von aussen nochmals
      // angestossen zu werden.
      
      __STATIC_phase = 4 ;
      
      // do real coarsening of elements 
      Gitter :: doCoarse () ;
      
      // reset edge coarsen flags to avoid problems with coarsening 
      // Gitter :: resetEdgeCoarsenFlags () ;
    } 
    catch (Parallel :: AccessPllException) 
    {
      cerr << "**FEHLER (FATAL) AccessPllException beim Vergr\"obern der Elementhierarchie oder\n" ;
      cerr << "  beim locken der Fl\"achen- bzw. Kantenb\"aume aufgetreten. In " << __FILE__ << " " << __LINE__ << endl ;
      abort () ;
    }
    
    try {
    
      // Phase des Fl"achenausgleichs des verteilten Vergr"oberungsalgorithmus
      // alle Schnittfl"achenpaare werden daraufhin untersucht, ob eine
      // Vergr"oberung in beiden Teilgittern durchgef"uhrt werden darf,
      // wenn ja, wird in beiden Teilgittern vergr"obert und der Vollzug
      // getestet.
  
      __STATIC_phase = 5 ;
    
      typedef vector< int > cleanvector_t ;
      vector < cleanvector_t > clean (nl) ;
      {
        vector < ObjectStream > inout( nl );
        {
          for (int l = 0 ; l < nl ; ++l)
          {
            ObjectStream& os = inout[ l ];
            // reserve memory 
            os.reserve( outerFaces [l].size() * sizeof(char) );

            // get end iterator 
            const hface_iterator iEnd = outerFaces [l].end () ;
            for (hface_iterator i = outerFaces [l].begin () ; i != iEnd; ++i)
            {
              char lockAndTry = (*i)->accessOuterPllX ().first->lockAndTry ();
              os.putNoChk( lockAndTry );
            }
          }
        }

        // exchange data 
        inout = mpAccess ().exchange (inout) ;
        
        {
          for (int l = 0 ; l < nl ; ++l) 
          {
            ObjectStream& os = inout[ l ];
            cleanvector_t& cl = clean[ l ];

            // reset clean vector 
            cl = cleanvector_t( innerFaces [l].size (), int(true) ) ;

            cleanvector_t :: iterator j = cl.begin (); 

            const hface_iterator iEnd = innerFaces [l].end () ;
            for (hface_iterator i = innerFaces [l].begin () ; i != iEnd; ++i, ++j ) 
            {
              // get lockAndTry info 
              const bool locked = bool( os.get() );

              assert (j != cl.end ()) ; 
              (*j) &= locked && (*i)->accessOuterPllX ().first->lockAndTry () ;
            }
          } 
        }
      }
      
      {
        vector < ObjectStream > inout (nl) ;
        {
          for (int l = 0 ; l < nl ; ++l) 
          {
            ObjectStream& os = inout[ l ];
            // reserve memory  
            os.reserve( innerFaces [l].size() * sizeof(char) );

            cleanvector_t :: iterator j = clean [l].begin () ;
            const hface_iterator iEnd = innerFaces [l].end () ;
            for (hface_iterator i = innerFaces [l].begin () ; i != iEnd; ++i, ++j) 
            {
              const bool unlock = *j;
              os.putNoChk( char(unlock) );
              (*i)->accessOuterPllX ().first->unlockAndResume ( unlock );
            }
          }     
        }
      
        // exchange data 
        inout = mpAccess ().exchange (inout) ;
      
        {
          for (int l = 0 ; l < nl ; ++l) 
          {
            ObjectStream& os = inout[ l ];
            const hface_iterator iEnd = outerFaces [l].end () ;
            for (hface_iterator i = outerFaces [l].begin () ; i != iEnd; ++i )
            {
              const bool unlock = bool( os.get() );
              (*i)->accessOuterPllX ().first->unlockAndResume ( unlock ) ;
            }
          }     
        }
      }
    } 
    catch (Parallel :: AccessPllException) 
    {
      cerr << "**FEHLER (FATAL) AccessPllException beim Vergr\"obern der Fl\"achenb\"aume\n" ;
      cerr << "  aufgetreten. In " << __FILE__ << " " << __LINE__ << endl ;
      abort () ;
    }
    
    try 
    {
      // Phase des Kantenausgleichs im parallelen Vergr"oberungsalgorithmus:
  
      __STATIC_phase  = 6 ;
    
      // Weil hier jede Kante nur eindeutig auftreten darf, muss sie in einem
      // map als Adresse hinterlegt werden, dann k"onnen die verschiedenen
      // Refcounts aus den verschiedenen Links tats"achlich global miteinander
      // abgemischt werden. Dazu werden zun"achst alle eigenen Kanten auf ihre
      // Vergr"oberbarkeit hin untersucht und dieser Zustand (true = vergr"oberbar
      // false = darf nicht vergr"obert werden) im map 'clean' hinterlegt. Dazu
      // kommt noch ein zweiter 'bool' Wert, der anzeigt ob die Kante schon ab-
      // schliessend vergr"obert wurde oder nicht. 
    
      typedef pair < bool, bool >  clean_t ;
      typedef map < hedge_STI *, clean_t, less < hedge_STI * > > cleanmap_t ;
      typedef cleanmap_t :: iterator cleanmapiterator_t ;
      cleanmap_t clean ;
      
      const cleanmapiterator_t cleanEnd = clean.end();
      {
        for (int l = 0 ; l < nl ; l ++)
        {
          const hedge_iterator iEnd = innerEdges [l].end () ;
          for (hedge_iterator i = innerEdges [l].begin () ; i != iEnd; ++i)
          {
            hedge_STI* edge = (*i);
            cleanmapiterator_t cit = clean.find ( edge );
            if (cit == cleanEnd ) 
            {
              clean_t& cp = clean[ edge ];
              cp.first  = edge->lockAndTry () ;
              cp.second = true ;
            }
          }
        }
      }
      
      {
        vector < ObjectStream > inout( nl );
        {
          for (int l = 0 ; l < nl ; ++l)
          {
            ObjectStream& os = inout[ l ];
            // reserve memory first 
            os.reserve( outerEdges [l].size() * sizeof(char) );

            // get end iterator 
            const hedge_iterator iEnd = outerEdges [l].end () ;
            for (hedge_iterator i = outerEdges [l].begin () ; i != iEnd; ++i)
            {
              char lockAndTry = (*i)->lockAndTry ();
              os.putNoChk( lockAndTry );
            }
          }
        }
        
        // exchange data 
        inout = mpAccess ().exchange (inout) ;
        
        {
          for (int l = 0 ; l < nl ; ++l) 
          {
            ObjectStream& os = inout[ l ];

            // get end iterator 
            const hedge_iterator iEnd = innerEdges [l].end () ;
            for (hedge_iterator i = innerEdges [l].begin () ; i != iEnd; ++i)
            {
              const bool locked = bool( os.get() );
              if( locked == false ) 
              {
                assert (clean.find (*i) != cleanEnd ) ;
                clean[ *i ].first = false ;
              }
            }
          }
        }
      }
      
      {
        vector < ObjectStream > inout( nl );
        {
          for (int l = 0 ; l < nl ; ++l) 
          {
            ObjectStream& os = inout[ l ];
            // reserve memory first 
            os.reserve( innerEdges [l].size() * sizeof(char) );

            // get end iterator 
            const hedge_iterator iEnd = innerEdges [l].end () ;
            for (hedge_iterator i = innerEdges [l].begin () ; i != iEnd; ++i) 
            {
              hedge_STI* edge = (*i);
              assert (clean.find ( edge ) != clean.end ()) ;

              clean_t& a = clean [ edge ] ;
              os.putNoChk( char( a.first) );

              if (a.second) 
              {
                // Wenn wir hier sind, kann die Kante tats"achlich vergr"obert werden, genauer gesagt,
                // sie wird es auch und der R"uckgabewert testet den Vollzug der Aktion. Weil aber nur
                // einmal vergr"obert werden kann, und die Iteratoren 'innerEdges [l]' aber eventuell
                // mehrfach "uber eine Kante hinweglaufen, muss diese Vergr"oberung im map 'clean'
                // vermerkt werden. Dann wird kein zweiter Versuch unternommen.
              
                a.second = false ;
#ifndef NDEBUG
                bool b = 
#endif
                  edge->unlockAndResume (a.first) ;
                assert (b == a.first) ;
              }
            }
          }
        }

        // also pack dynamic state here since this is the last communication 
        packUnpackDynamicState( inout, nl, true );
        
        // exchange data 
        inout = mpAccess ().exchange (inout) ;
        
        {
          for (int l = 0 ; l < nl ; ++l ) 
          {
            ObjectStream& os = inout[ l ] ;
            // get end iterator 
            const hedge_iterator iEnd = outerEdges [l].end () ;
            for (hedge_iterator i = outerEdges [l].begin () ; i != iEnd; ++i )
            {
              // Selbe Situation wie oben, aber der Eigent"umer der Kante hat mitgeteilt, dass sie
              // vergr"obert werden darf und auch wird auf allen Teilgebieten also auch hier. Der
              // Vollzug der Vergr"oberung wird durch den R"uckgabewert getestet.
            
              const bool unlock = bool( os.get() );
#ifndef NDEBUG
              bool b = 
#endif
                (*i)->unlockAndResume ( unlock ) ;
              assert (b == unlock) ;
            }
          }
        }

        // unpack dynamic state here since this is the last communication 
        packUnpackDynamicState( inout, nl, false );
      }
    } 
    catch (Parallel :: AccessPllException) 
    {
      cerr << "**FEHLER (FATAL) AccessPllException beim Vergr\"obern der Kantenb\"aume\n" ;
      cerr << "  aufgetreten. In " << __FILE__ << " " << __LINE__ << endl ;
      abort () ;
    }
  }
  
  __STATIC_phase = -1 ;
  
  return ;
}

void GitterPll :: 
packUnpackDynamicState ( vector < ObjectStream >& osv, const int nl, const bool packData )
{
  typedef Insert < AccessIteratorTT < hface_STI > :: InnerHandle,
     TreeIterator < hface_STI, is_def_true < hface_STI > > > InnerIteratorType;
  typedef Insert < AccessIteratorTT < hface_STI > :: OuterHandle, 
    TreeIterator < hface_STI, is_def_true < hface_STI > > > OuterIteratorType;
              
  // if pack data is true, then write data to stream, otherwise read 
  if( packData ) 
  {
    for (int l = 0 ; l < nl ; ++l) 
    {
      // get object stream 
      ObjectStream& os = osv[ l ];

      AccessIteratorTT < hface_STI > :: InnerHandle mif (this->containerPll (),l) ;
      AccessIteratorTT < hface_STI > :: OuterHandle mof (this->containerPll (),l) ;

      InnerIteratorType wi (mif);
      for (wi.first () ; ! wi.done () ; wi.next ()) 
      {
        pair < ElementPllXIF_t *, int > p = wi.item ().accessInnerPllX () ;
        p.first->writeDynamicState (os, p.second) ;
      }
  
      OuterIteratorType wo (mof);
      for (wo.first () ; ! wo.done () ; wo.next ()) 
      {
        pair < ElementPllXIF_t *, int > p = wo.item ().accessInnerPllX () ;
        p.first->writeDynamicState (os, p.second) ;
      }
    }
  }
  else // unpack data 
  { 
    for (int l = 0 ; l < nl ; ++l) 
    {
      // get object stream 
      ObjectStream& os = osv[ l ];

      AccessIteratorTT < hface_STI > :: OuterHandle mof (this->containerPll (),l) ;
      AccessIteratorTT < hface_STI > :: InnerHandle mif (this->containerPll (),l) ;
  
      OuterIteratorType wo (mof) ;
      for (wo.first () ; ! wo.done () ; wo.next ()) 
      {
        pair < ElementPllXIF_t *, int > p = wo.item ().accessOuterPllX () ;
        p.first->readDynamicState (os, p.second) ;
      }
  
      InnerIteratorType wi (mif);
      for (wi.first () ; ! wi.done () ; wi.next ()) 
      {
        pair < ElementPllXIF_t *, int > p = wi.item ().accessOuterPllX () ;
        p.first->readDynamicState (os, p.second) ;
      }
    }
  } 
}

#ifdef ENABLE_ALUGRID_VTK_OUTPUT
extern int adaptstep;
extern int stepnumber;
#endif

bool GitterPll :: adapt () 
{
#ifdef ENABLE_ALUGRID_VTK_OUTPUT
  stepnumber = 0;
#endif

  bool refined = false;
  bool needConformingClosure = false ;
  const bool bisectionEnabled = conformingClosureNeeded();
  assert( bisectionEnabled == mpAccess().gmax( bisectionEnabled ) );

  // loop until refinement leads to a conforming situation (conforming refinement only)
  do 
  {
    __STATIC_myrank = mpAccess ().myrank () ;
    __STATIC_turn ++ ;
    assert (debugOption (20) ? (cout << "**INFO GitterPll :: adapt ()" << endl, 1) : 1) ;
    assert (! iterators_attached ()) ;

    // call refine 
    refined |= refine () ;

    // for bisection refinement repeat loop if non-confoming edges are still present 
    // markForConformingClosure returns true if all elements have conforming closure 
    needConformingClosure = 
      bisectionEnabled ? mpAccess().gmax( markForConformingClosure() ) : false ;

  } 
  while ( needConformingClosure );

  // now do one coarsening step 
  coarse () ;

#ifndef NDEBUG
  needConformingClosure = 
    bisectionEnabled ? mpAccess().gmax( markForConformingClosure() ) : false ;
  assert( ! needConformingClosure );
#endif

#ifdef ENABLE_ALUGRID_VTK_OUTPUT
  ++adaptstep;
#endif

  return refined;
}

void GitterPll :: MacroGitterPll :: fullIntegrityCheck (MpAccessLocal & mpa) {
  const int nl = mpa.nlinks (), me = mpa.myrank () ;

  try {
    vector < vector < int > > inout (nl) ;

    {for (int l = 0 ; l < nl ; l ++) {
      AccessIteratorTT < hface_STI > :: InnerHandle w (*this,l) ;
      for ( w.first () ; ! w.done () ; w.next ()) {
        vector < int > i = w.item ().checkParallelConnectivity () ;
        copy (i.begin (), i.end (), back_inserter (inout [l])) ;
      }
    }}
    inout = mpa.exchange (inout) ;
    {for (int l = 0 ; l < nl ; l ++) {
      vector < int > :: const_iterator pos = inout [l].begin () ;
      AccessIteratorTT < hface_STI > :: OuterHandle w (*this,l) ;
      for (w.first () ; ! w.done () ; w.next ()) {
        vector < int > t1 = w.item ().checkParallelConnectivity () ;
        vector < int > t2 (t1.size (), 0) ;
        copy (pos, pos + t1.size (), t2.begin ()) ;
        pos += t1.size () ;
        if (t1 != t2) {
          cerr << "fehler an gebiet " << me << " : " ;
#ifdef IBM_XLC
          copy (t1.begin (), t1.end (), ostream_iterator < int > (cerr, "-")) ;
#elif defined(_SGI)
          copy (t1.begin (), t1.end (), ostream_iterator < int > (cerr, "-")) ;
#else
          copy (t1.begin (), t1.end (), ostream_iterator < int , char > (cerr, "-")) ;
#endif
    cerr << "\t" ;
#ifdef IBM_XLC
          copy (t2.begin (), t2.end (), ostream_iterator < int > (cerr, "-")) ;
#elif defined(_SGI)
          copy (t2.begin (), t2.end (), ostream_iterator < int > (cerr, "-")) ;
#else
          copy (t2.begin (), t2.end (), ostream_iterator < int , char > (cerr, "-")) ;
#endif
          cerr << endl ;
        }
      }
    }}
  } catch (Parallel ::  AccessPllException) {
    cerr << "**FEHLER (FATAL) Parallel :: AccessPllException entstanden in: " << __FILE__ << " " << __LINE__ << endl ;
  }
  return ;
}

void GitterPll :: exchangeDynamicState () 
{
  // Zustand des Gitters ge"andert hat: Verfeinerung und alle Situationen
  // die einer "Anderung des statischen Zustands entsprechen. Sie wird in
  // diesem Fall NACH dem Update des statischen Zustands aufgerufen (nach loadBalance), 
  // und kann demnach von einem korrekten statischen Zustand ausgehen. F"ur
  // Methoden die noch h"aufigere Updates erfordern m"ussen diese in der
  // Regel hier eingeschleift werden.
  {
    const int nl = mpAccess ().nlinks () ;
  
#ifndef NDEBUG 
    // if debug mode, then count time 
    const int start = clock () ;
#endif
  
    try 
    {
      typedef Insert < AccessIteratorTT < hface_STI > :: InnerHandle,
         TreeIterator < hface_STI, is_def_true < hface_STI > > > InnerIteratorType;
      typedef Insert < AccessIteratorTT < hface_STI > :: OuterHandle, 
        TreeIterator < hface_STI, is_def_true < hface_STI > > > OuterIteratorType;
                
      // create message buffers 
      vector < ObjectStream > osv (nl) ;

      // pack dynamic state 
      packUnpackDynamicState( osv, nl, true );
        
      // exchange data 
      osv = mpAccess ().exchange (osv) ;
    
      // unpack dynamic state 
      packUnpackDynamicState( osv, nl, false );
        
    } 
    catch (Parallel ::  AccessPllException) 
    {
      cerr << "  FEHLER Parallel :: AccessPllException entstanden in: " << __FILE__ << " " << __LINE__ << endl ;
    }
    assert (debugOption (20) ? (cout << "**INFO GitterDunePll :: packUnpackDynamicState () used " << (float)(clock () - start)/(float)(CLOCKS_PER_SEC) << " sec. " << endl, 1) : 1 ) ;
  }
  return ;
}

bool GitterPll :: checkPartitioning( LoadBalancer :: DataBase& db, GatherScatterType* gs ) 
{
  // build macro graph, either using user defined weights or default weighs 
  assert (debugOption (20) ? (cout << "**GitterPll :: checkPartitioning ( db, gs ) " << endl, 1) : 1) ;
  {
    AccessIterator < hface_STI > :: Handle w (containerPll ()) ;
    for (w.first () ; ! w.done () ; w.next ()) w.item ().ldbUpdateGraphEdge (db) ;
  }
  {
    // if gs is given and user defined weight is enabled, use these to setup graph 
    GatherScatter* gatherScatter = gs && gs->userDefinedLoadWeights() ? gs : 0 ;
    AccessIterator < helement_STI > :: Handle w (containerPll ()) ;
    for (w.first () ; ! w.done () ; w.next ()) w.item ().ldbUpdateGraphVertex (db, gatherScatter);
  }

  const int np = mpAccess ().psize () ;
  bool repartition = false ;
  {
    // Criterion, if a repartition has to be done 
    // 
    // load    - own load 
    // mean    - mean load of elements 
    // minload - minmal load 
    // maxload - maximal load 

    // number of leaf elements 
    const double myload = db.accVertexLoad () ;

    // get:  min(myload), max(myload), sum(myload)
    MpAccessLocal :: minmaxsum_t load = mpAccess ().minmaxsum( myload ); 

    // get mean value of leaf elements 
    const double mean = load.sum / double( np );

    if( load.max > (_ldbOver * mean) || load.min < (_ldbUnder * mean) ) 
      repartition = true ;
  }

#ifndef NDEBUG
  // make sure every process has the same value of repartition
  const bool checkNeu = mpAccess().gmax( repartition );
  assert( repartition == checkNeu );
#endif

  return repartition;
}

// --loadBalance 
bool GitterPll :: loadBalancerGridChangesNotify ( GatherScatterType* gs ) 
{
  // create load balancer data base 
  LoadBalancer :: DataBase db ;

  // check whether we have to repartition 
  const bool repartition = checkPartitioning( db, gs );

  // if repartioning necessary, do it 
  if ( repartition ) 
  {
    const int ldbMth = int( _ldbMethod );
#ifndef NDEBUG
    // make sure every process has the same ldb method 
    int checkMth = mpAccess ().gmax( ldbMth );
    assert( checkMth == ldbMth ); 
#endif

    // if a method was given, perform load balancing
    if( ldbMth ) 
    {
      // check gather-scatter object and call appropriate method 
      if( gs ) 
        duneRepartitionMacroGrid( db, *gs );  
      else   
        repartitionMacroGrid (db) ;

      // calls identification and exchangeDynamicState 
      notifyMacroGridChanges () ;
    }
  }
  return repartition ;
}

void GitterPll :: loadBalancerMacroGridChangesNotify () 
{
  computeGraphVertexIndices ();
}

void GitterPll :: computeGraphVertexIndices () 
{
  if( ! _ldbVerticesComputed ) 
  {
    // this method computes the globally unique element indices 
    // that are needed for the graph partitioning methods 

    assert (debugOption (20) ? (cout << "**INFO GitterPll :: loadBalancerMacroGridChangesNotify () " << endl, 1) : 1) ;
    AccessIterator < helement_STI > :: Handle w ( containerPll () ) ;

    // get number of macro elements 
    const int macroElements = w.size () ;

    // sum up for each process and and substract macroElements again 
    int cnt = mpAccess ().scan( macroElements ) - macroElements ;

#ifndef NDEBUG 
    // make sure that we get the same value as before 
    //std::cout << "P[ " << mpAccess().myrank() << " ] cnt = " << cnt << std::endl;
    { 
      int oldcnt = 0;
      // get sizes from all processes 
      vector < int > sizes = mpAccess ().gcollect ( macroElements ) ;

      // count sizes for all processors with a rank lower than mine 
      for (int i = 0 ; i < mpAccess ().myrank () ; oldcnt += sizes [ i++ ]) ;
      assert( oldcnt == cnt );
    }
#endif

    // set ldb vertex indices to all elements 
    for (w.first () ; ! w.done () ; w.next (), ++ cnt ) 
    {
      w.item ().setLoadBalanceVertexIndex ( cnt ) ;
    }

    // mark unique element indices as computed 
    _ldbVerticesComputed = true ;
  }

#ifndef NDEBUG
  {
    assert (debugOption (20) ? (cout << "**INFO GitterPll :: loadBalancerMacroGridChangesNotify () " << endl, 1) : 1) ;
    AccessIterator < helement_STI > :: Handle w ( containerPll () ) ;

    int lastIndex = -1;
    // set ldb vertex indices to all elements 
    for (w.first () ; ! w.done () ; w.next () ) 
    {
      const int ldbVx = w.item ().ldbVertexIndex() ;
      assert( lastIndex < ldbVx );
      lastIndex = ldbVx ;
    }
  }
#endif
}

void GitterPll :: notifyMacroGridChanges () 
{
  assert (debugOption (20) ? (cout << "**INFO GitterPll :: notifyMacroGridChanges () " << endl, 1) : 1 ) ;
  Gitter :: notifyMacroGridChanges () ;

  containerPll ().identification (mpAccess ()) ;

  loadBalancerMacroGridChangesNotify () ;
  exchangeDynamicState () ;
  return ;
}

GitterPll :: GitterPll ( MpAccessLocal & mpa ) 
  : _ldbOver (0.0), 
    _ldbUnder (0.0), 
    _ldbMethod (LoadBalancer :: DataBase :: NONE),
    _refineLoops( 0 ), 
    _ldbVerticesComputed( false )
{
  if( mpa.myrank() == 0 ) 
  {
    // set default values 
    _ldbOver = 1.2;
    _ldbMethod = LoadBalancer :: DataBase :: METIS_PartGraphKway ;

    ifstream in ("alugrid.cfg") ;
    if (in) 
    {
      int i ;
      in >> _ldbUnder ;
      in >> _ldbOver ;
      in >> i;
      _ldbMethod = (LoadBalancer :: DataBase :: method) i ;
    } 
    else 
    {
      cerr << endl << "**WARNING (ignored) could'nt open file "
           << "< alugrid.cfg > . "
           << "Using default values: " << endl ;
      cerr << _ldbUnder << " < [balance] < " << _ldbOver << " " 
           << "  partitioning method \"" 
           << LoadBalancer :: DataBase :: methodToString (_ldbMethod) 
           << "\"" << endl << endl;
    }
  } // got values on rank 0 

  // now communicate them 
  double buff[ 3 ]  = { _ldbOver, _ldbUnder, double(_ldbMethod) };

  // broadcast values from rank 0 to all others 
  // (much better then to read file on all procs)
  const int root = 0 ;
  mpa.bcast( &buff[ 0 ], 3, root);

  // store values 
  _ldbOver   = buff[ 0 ];
  _ldbUnder  = buff[ 1 ];
  _ldbMethod = (LoadBalancer :: DataBase :: method ) buff[ 2 ];

  // wait for all to finish 
#ifndef NDEBUG
  mpa.barrier();
#endif
}

#endif
