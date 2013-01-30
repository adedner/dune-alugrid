#ifndef ALUGRID_ZOLTAN_H_INCLUDED
#define ALUGRID_ZOLTAN_H_INCLUDED

#include <cmath>
#include <cassert>

#if HAVE_ZOLTAN 
#define ZOLTAN_CONFIG_H_INCLUDED
#include <zoltan_cpp.h>
#endif 

namespace ALUGridZoltan
{
  template < class ldb_vertex_map_t >
  class ObjectCollection 
  {
    ldb_vertex_map_t& _vertexMap ;

  public:
    // constructor
    ObjectCollection( ldb_vertex_map_t& vertexMap ) 
      : _vertexMap( vertexMap ) 
    {}

    ldb_vertex_map_t& vertexMap() { return _vertexMap; }

    // query functions that respond to requests from Zoltan 

    static int get_number_of_objects(void *data, int *ierr)
    {
      ObjectCollection *objs = static_cast<ObjectCollection *> (data);
      assert( objs );
      *ierr = ZOLTAN_OK;
      return objs->vertexMap().size();
    }

    static void get_object_list(void *data, int sizeGID, int sizeLID,
                                ZOLTAN_ID_PTR globalID, ZOLTAN_ID_PTR localID,
                                int wgt_dim, float *obj_wgts, int *ierr)
    {
      ObjectCollection *objs = static_cast<ObjectCollection *> (data);
      *ierr = ZOLTAN_OK;

      ldb_vertex_map_t& vertexMap = objs->vertexMap();

      // In this example, return the IDs of our objects, but no weights.
      // Zoltan will assume equally weighted objects.
      const int numLocalObjs = vertexMap.size();
      typedef typename ldb_vertex_map_t :: iterator iterator ;
      const iterator end = vertexMap.end();
      int i = 0; 
      for ( iterator it = vertexMap.begin(); it != end; ++ it, ++i ) 
      {
        globalID[ i ] = (*it).first.index() ;
        localID [ i ] = i;
      }
    }
  };

  template< class ldb_vertex_map_t, class ldb_connect_set_t >
  void CALL_Zoltan_LB_Partition( ALUGrid::MpAccessGlobal &mpa,
                                 ldb_vertex_map_t& vertexMap,
                                 ldb_connect_set_t& connect ) 
  {
#if HAVE_ZOLTAN 
    ALUGrid::MpAccessMPI* mpaMPI = dynamic_cast<ALUGrid::MpAccessMPI *> (&mpa);
    if( mpaMPI == 0 )
    {
      std::cerr << "ERROR: wrong mpAccess object, couldn't convert to MpAccessMPI!! in: " << __FILE__ << " line : " << __LINE__ << std::endl;
      abort();
    }

    // get communincator (see mpAccess_MPI.cc
    MPI_Comm comm = mpaMPI->communicator();

    typedef ObjectCollection< ldb_vertex_map_t > ObjectCollectionType;

    ObjectCollectionType objects( vertexMap );
    Zoltan *zz = new Zoltan( MPI_COMM_WORLD );
    assert( zz );

    // General parameters 
    zz->Set_Param( "LB_METHOD", "BLOCK"  );  /* Zoltan method: "BLOCK" */
    zz->Set_Param( "NUM_GID_ENTRIES", "1");  /* global ID is 1 integer */
    zz->Set_Param( "NUM_LID_ENTRIES", "1");  /* local  ID is 1 integer */
    zz->Set_Param( "OBJ_WEIGHT_DIM" , "0");  /* we omit object weights */

    zz->Set_Num_Obj_Fn ( ObjectCollectionType::get_number_of_objects, &objects);
    zz->Set_Obj_List_Fn( ObjectCollectionType::get_object_list, &objects);

    int changes;
    int numGidEntries;
    int numLidEntries;
    int numImport;
    ZOLTAN_ID_PTR importGlobalIds;
    ZOLTAN_ID_PTR importLocalIds;
    int *importProcs;
    int *importToPart;
    int numExport;
    ZOLTAN_ID_PTR exportGlobalIds;
    ZOLTAN_ID_PTR exportLocalIds;
    int *exportProcs;
    int *exportToPart;

    int rc = zz->LB_Partition(changes, numGidEntries, numLidEntries,
                              numImport, importGlobalIds, importLocalIds, importProcs, importToPart,
                              numExport, exportGlobalIds, exportLocalIds, exportProcs, exportToPart);

    typedef typename ldb_vertex_map_t::iterator iterator;

    for (int i=0; i < numExport; ++i)
    {
      iterator vertex = vertexMap.find( exportGlobalIds[ i ] );
      assert( vertex != vertexMap.end () );
      (*vertex).second = exportToPart[ i ];
    }

    const iterator iEnd = vertexMap.end ();
    for ( iterator i = vertexMap.begin (); i != iEnd; ++i )
    {
      // insert and also set partition number new 
      connect.insert( (*i).second );
    }

    ////////////////////////////////////////////////////////////////
    // Free the arrays allocated by LB_Partition, and free
    // the storage allocated for the Zoltan structure and the mesh.
    ////////////////////////////////////////////////////////////////

    Zoltan::LB_Free_Part(&importGlobalIds, &importLocalIds, &importProcs, &importToPart);
    Zoltan::LB_Free_Part(&exportGlobalIds, &exportLocalIds, &exportProcs, &exportToPart);

    delete zz;
#endif
  } // CALL_Zoltan_LB_Partition 

} // namespace ALUGridZoltan

#endif // #ifndef ALUGRID_ZOLTAN_H_INCLUDED
