#ifndef GITTER_DUNE_PLL_IMPL_H_INCLUDED
#define GITTER_DUNE_PLL_IMPL_H_INCLUDED

#ifdef _ANSI_HEADER
  using namespace std;
  #include <numeric>
#else
#endif

#include "gitter_dune_impl.h"
#include "gitter_pll_impl.h"
#include "gitter_pll_ldb.h"

class GitterDunePll : public GitterBasisPll , public GitterDuneBasis 
{
public:
  GitterDunePll (const char * filename , MpAccessLocal &mp) : GitterBasisPll (filename,mp) {};

  virtual bool duneAdapt () ; // done call notify and loadBalancer  

  virtual bool duneLoadBalance () ; // call loadBalancer 
  virtual bool duneLoadBalance (GatherScatterType & ) ; // call loadBalancer a

  virtual void duneRepartitionMacroGrid (LoadBalancer :: DataBase &, GatherScatterType & gs) ;
};
#endif
