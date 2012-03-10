#ifndef METIS_H_INCLUDED
#define METIS_H_INCLUDED

#if HAVE_METIS 

// cover metis-4.0 bug   
#define __log2 __METIS__log2
extern "C" {
  #include <metis.h>
}

#undef HAVE_METIS_VERSION_4
#if not defined IDXTYPEWIDTH && not defined REALTYPEWIDTH 
#define HAVE_METIS_VERSION_4
#endif

#else 
static const char metmess [] =  "**INFO Due to license reasons the library METIS is\n"
        "       not part of the ALUGrid library distribution. \n"
        "       To use this feature get a copy of the METIS library \n"
        "       (see http://www-users.cs.umn.edu/~karypis/metis/metis/ )\n"
        "       and re-configure the ALUGrid library with the \n"
        "       --with-metis=PATH_TO_METIS option, \n"
        "       or choose another Graph partitioning method. \n"
        "       Exiting program, bye! \n";
#endif

namespace ALUGridMETIS
{

#if HAVE_METIS && not defined HAVE_METIS_VERSION_4
typedef idx_t  idxtype ;
typedef real_t realtype ;
#else 
typedef int   idxtype ;
typedef float realtype ;
#endif 

inline void 
CALL_METIS_PartGraphKway(idxtype *n, idxtype* ncon, 
                         idxtype *edge_p, idxtype *edge,
                         idxtype *vertex_wInt, idxtype *edge_w,
                         idxtype *wgtflag, idxtype *numflag, idxtype *npart,
                         realtype* tpwgts, realtype *ubvec, idxtype* options, 
                         idxtype* edgecut, idxtype *neu)
{
#if HAVE_METIS  

#ifdef HAVE_METIS_VERSION_4 

  // METIS Version 4.0
  // call metis function 
  :: METIS_PartGraphKway(n,
                         edge_p, edge, 
                         vertex_wInt, 
                         edge_w,
                         wgtflag,
                         numflag,
                         npart, 
                         options,
                         edgecut, neu) ;

#else 

  // METIS Version 5.x
  // call metis function 
  :: METIS_PartGraphKway(n,
                         ncon,
                         edge_p, edge, 
                         vertex_wInt, 
                         (idx_t *) 0,
                         edge_w,
                         npart, 
                         tpwgts, ubvec, 
                         (idx_t *) 0, // options 
                         edgecut, neu) ;

#endif

#else 
  std::cerr << "**ERROR The use of METIS_PartGraphKway is not supported, when the METIS library is missing!  in: " << __FILE__ << " line: " << __LINE__ << "\n";
  std::cerr << metmess << std::endl ;
  exit(1);
#endif
  return ;
}

inline void 
CALL_METIS_PartGraphRecursive(idxtype *n, idxtype* ncon, 
                              idxtype *edge_p, idxtype *edge,
                              idxtype *vertex_wInt, idxtype *edge_w,
                              idxtype *wgtflag, idxtype *numflag, idxtype *npart,
                              realtype* tpwgts, realtype *ubvec, idxtype* options, 
                              idxtype* edgecut, idxtype *neu)
{
#if HAVE_METIS  

#ifdef HAVE_METIS_VERSION_4 

  // METIS Version 4.0
  // call metis function 
  :: METIS_PartGraphRecursive(n,
                              edge_p, edge, 
                              vertex_wInt, 
                              edge_w,
                              wgtflag,
                              numflag,
                              npart, 
                              options,
                              edgecut, neu) ;

#else 

  // METIS Version 5.x
  // call metis function 
  :: METIS_PartGraphRecursive(n,
                              ncon,
                              edge_p, edge, 
                              vertex_wInt, 
                              (idx_t *) 0,
                              edge_w,
                              npart, 
                              tpwgts, ubvec, 
                              (idx_t *) 0, // options 
                              edgecut, neu) ;

#endif // end HAVE_METIS_VERSION_4

#else 
  std::cerr << "**ERROR The use of METIS_PartGraphRecursive is not supported, when the METIS library is missing!  in: " << __FILE__ << " line: " << __LINE__ << "\n";
  std::cerr << metmess << std::endl ;
  exit(1);
#endif
  return ;
}

}
#endif
