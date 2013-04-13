#ifndef LOADBALANCE_HH
#define LOADBALANCE_HH

#include <dune/grid/common/gridenums.hh>
#include <dune/grid/common/datahandleif.hh>

#if HAVE_ZOLTAN 
#include <zoltan.h>
#endif

template< class Grid >
class SimpleLoadBalanceHandle
: public Dune::LoadBalanceHandleIF< SimpleLoadBalanceHandle<Grid> >
{
  typedef SimpleLoadBalanceHandle This;
  typedef Dune::LoadBalanceHandleIF< This > Base;

public:
  static const int dimension = Grid :: dimension;

  template< int codim >
  struct Codim
  {
    typedef typename Grid :: Traits :: template Codim< codim > :: Entity Entity;
    typedef typename Grid :: Traits :: template Codim< codim > :: EntityPointer
      EntityPointer;
  };
  typedef typename Codim< 0 > :: Entity Element;

private:
  const Grid &grid_;

public:
  SimpleLoadBalanceHandle ( const Grid &grid )
  : grid_( grid ),
    angle_( 0 )
  {}

  bool userDefinedPartitioning () const
  {
    return true;
  }
  // return true if user defined load balancing weights are provided
  bool userDefinedLoadWeights () const
  {
    return false;
  }

  // returns true if user defined partitioning needs to be readjusted 
  bool repartition () 
  { 
    angle_ += 2.*M_PI/50.;
    return true;
  }
  // return load weight of given element 
  int loadWeight( const Element &element ) const 
  { 
    return 1;
  }
  // return destination (i.e. rank) where the given element should be moved to 
  // this needs the methods userDefinedPartitioning to return true
  int destination( const Element &element ) const 
  { 
    typename Element::Geometry::GlobalCoordinate w = element.geometry().center();
    double phi=arg(std::complex<double>(w[0],w[1]));
    if (w[1]<0) phi+=2.*M_PI;
    phi += angle_;
    phi *= double(this->grid_.comm().size())/(2.*M_PI);
    int p = int(phi) % this->grid_.comm().size();
    return p;
  }
private:
  double angle_;
};

#if HAVE_ZOLTAN 
template< class Grid >
class ZoltanLoadBalanceHandle
: public Dune::LoadBalanceHandleIF< ZoltanLoadBalanceHandle<Grid> >
{
  typedef ZoltanLoadBalanceHandle This;
  typedef Dune::LoadBalanceHandleIF< This > Base;

private:
  typedef typename Grid::GlobalIdSet GlobalIdSet;
  typedef typename GlobalIdSet::IdType gIdType;
  static const int dimension = Grid :: dimension;
  static const int NUM_GID_ENTRIES = 4;
  template< int codim >
  struct Codim
  {
    typedef typename Grid :: Traits :: template Codim< codim > :: Entity Entity;
    typedef typename Grid :: Traits :: template Codim< codim > :: EntityPointer EntityPointer;
  };

  typedef struct{
    int changes; // 1 if partitioning was changed, 0 otherwise 
    int numGidEntries;  // Number of integers used for a global ID 
    int numExport;      // Number of vertices I must send to other processes
    unsigned int *exportGlobalGids;  // Global IDs of the vertices I must send 
    int *exportProcs;    // Process to which I send each of the vertices 
  } ZOLTAN_PARTITIONING;
  typedef struct{      /* Zoltan will partition vertices, while minimizing edge cuts */
    int numMyVertices;  /* number of vertices that I own initially */
    ZOLTAN_ID_TYPE *vtxGID;        /* global ID of these vertices */
    int numMyHEdges;    /* number of my hyperedges */
    int numAllNbors;    /* number of vertices in my hyperedges */
    ZOLTAN_ID_TYPE *edgeGID;       /* global ID of each of my hyperedges */
    int *nborIndex;     /* index into nborGID array of edge's vertices */
    ZOLTAN_ID_TYPE *nborGID;       /* Vertices of edge edgeGID[i] begin at nborGID[nborIndex[i]] */
  } HGRAPH_DATA;
  typedef struct{
    int fixed_entities;
    std::vector<int> fixed_GID;
    std::vector<int> fixed_Process;
  } FIXED_ELEMENTS;
public:
  typedef typename Codim< 0 > :: Entity Element;
  ZoltanLoadBalanceHandle ( const Grid &grid)
  : grid_( grid )
	, globalIdSet_( grid.globalIdSet() )
  {}

  bool userDefinedPartitioning () const
  {
    return true;
  }
  // return true if user defined load balancing weights are provided
  bool userDefinedLoadWeights () const
  {
    return false;
  }

  // returns true if user defined partitioning needs to be readjusted 
  bool repartition ()
  { 
    generateHypergraph();
    return true;
  }
  // return load weight of given element 
  int loadWeight( const Element &element ) const 
  { 
    return 1;
  }
  // return destination (i.e. rank) where the given element should be moved to 
  // this needs the methods userDefinedPartitioning to return true
  int destination( const Element &element ) const 
  { 
    gIdType bla = globalIdSet_.id(element);
    std::vector<int> elementGID(4); // because we have 4 vertices
    bla.getKey().extractKey(elementGID);

    // add one to the GIDs, so that they match the ones from Zoltan
    transform(elementGID.begin(), elementGID.end(), elementGID.begin(), bind2nd(std::plus<int>(), 1));

    int p = int(this->grid_.comm().rank());

    for (int i = 0; i<new_partitioning_.numExport; ++i)
    {
      if (std::equal(elementGID.begin(),elementGID.end(), &new_partitioning_.exportGlobalGids[i*new_partitioning_.numGidEntries]) )
      p = new_partitioning_.exportProcs[i];
    }
    return p;
  }
private:
  void generateHypergraph() const;

  // ZOLTAN query functions
  static int get_number_of_vertices(void *data, int *ierr);
  static void get_vertex_list(void *data, int sizeGID, int sizeLID,
              ZOLTAN_ID_PTR globalID, ZOLTAN_ID_PTR localID,
              int wgt_dim, float *obj_wgts, int *ierr);
  static void get_hypergraph_size(void *data, int *num_lists, int *num_nonzeroes,
                                  int *format, int *ierr);
  static void get_hypergraph(void *data, int sizeGID, int num_edges, int num_nonzeroes,
                             int format, ZOLTAN_ID_PTR edgeGID, int *vtxPtr,
                             ZOLTAN_ID_PTR vtxGID, int *ierr);
  static int get_num_fixed_obj(void *data, int *ierr);
  static void get_fixed_obj_list(void *data, int num_fixed_obj,
                                 int num_gid_entries, ZOLTAN_ID_PTR fixed_gids, int *fixed_part, int *ierr);

  const Grid &grid_;
  const GlobalIdSet &globalIdSet_;

  mutable HGRAPH_DATA hg;
  mutable ZOLTAN_PARTITIONING new_partitioning_;
  mutable FIXED_ELEMENTS fixed_elmts;
};
#endif // if HAVE_ZOLTAN

#endif // #ifndef LOADBALNCE_HH
