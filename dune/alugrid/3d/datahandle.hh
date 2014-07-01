#ifndef DUNE_ALU3DGRIDDATAHANDLE_HH
#define DUNE_ALU3DGRIDDATAHANDLE_HH

//- system includes 
#include <iostream>

#include <dune/common/typetraits.hh>

#include <dune/grid/common/grid.hh>
#include <dune/grid/common/adaptcallback.hh>

#include <dune/alugrid/3d/datacollectorcaps.hh>

//- local includes 
#include "alu3dinclude.hh"

using std::endl;
using std::cout;
using std::flush;

namespace ALUGrid
{

  //! the corresponding interface class is defined in bsinclude.hh
  template< class GridType, class DataCollectorType, int codim >
  class GatherScatterBaseImpl
  : public GatherScatter
  {
  protected:  
    const GridType & grid_;
    typedef typename GridType::template Codim<codim>::Entity EntityType;
    typedef Dune :: MakeableInterfaceObject<
      typename GridType::template Codim<codim>::Entity> MakeableEntityType;
    typedef typename MakeableEntityType :: ImplementationType RealEntityType;

    typedef typename GridType::MPICommunicatorType Comm;
    
    typedef Dune::ALU3dImplTraits< GridType::elementType, Comm > ImplTraits;
    typedef typename ImplTraits::template Codim< codim >::ImplementationType ImplElementType;
    typedef typename ImplTraits::template Codim< codim >::InterfaceType HElementType;

    EntityType  & entity_;
    RealEntityType & realEntity_;

    DataCollectorType & dc_;

    const bool variableSize_;

    typedef typename GatherScatter :: ObjectStreamType ObjectStreamType;

    typedef typename DataCollectorType:: DataType DataType;

    using GatherScatter :: setData ;
    using GatherScatter :: sendData ;
    using GatherScatter :: recvData ;
    using GatherScatter :: containsItem ;

  public:
    //! Constructor
    GatherScatterBaseImpl(const GridType & grid, MakeableEntityType & en, 
        RealEntityType & realEntity , DataCollectorType & dc) 
      : grid_(grid), entity_(en), realEntity_(realEntity) , dc_(dc)
      , variableSize_( ! dc_.fixedsize(EntityType::dimension,codim) )
    {
    }

    //! returns contains of dc_
    bool contains(int dim, int cd) const { return dc_.contains(dim,cd); }

    // returns true, if element is contained in set of comm interface 
    // this method must be overlaoded by the impl classes 
    virtual bool containsItem (const HElementType & elem) const = 0;

    // set elem to realEntity
    virtual void setElement(const HElementType & elem) = 0;
      
    void setData ( ObjectStreamType & str , HElementType & elem )
    {
      // one of this should be either true 
      alugrid_assert ( this->containsItem( elem ) || elem.isGhost() );

      // set element and then start 
      setElement(elem);

      // make sure partition type is set correct 
      alugrid_assert ( elem.isGhost() == (entity_.partitionType() == Dune :: GhostEntity) );

      size_t size = getSize(str, entity_);
      // use normal scatter method 
      dc_.scatter(str,entity_, size ); 
    }

    //! write Data of one element to stream 
    void sendData ( ObjectStreamType & str , HElementType & elem )
    {
      // make sure element is contained in communication interface
      //alugrid_assert ( this->containsItem( elem ) );
      setElement(elem);

      // if varaible size, also send size 
      if( variableSize_ )
      {
        size_t size = dc_.size( entity_ );
        str.write( size );
      }

      dc_.gather(str, entity_ );
    }

    //! read Data of one element from stream 
    void recvData ( ObjectStreamType & str , HElementType & elem )
    {
      alugrid_assert ( this->containsItem( elem ) );
      setElement( elem );

      size_t size = getSize(str, entity_);
      dc_.scatter(str,entity_, size );
    }

  protected:  
    size_t getSize(ObjectStreamType & str, EntityType & en)
    {
      if(variableSize_) 
      {
        size_t size;
        str.read(size);
        return size;
      }
      else 
        return dc_.size(en);
    }
  };

  //***********************************************************
  //
  //  --specialisation for codim 0 
  //
  //***********************************************************

  //! the corresponding interface class is defined in bsinclude.hh
  template <class GridType, class DataCollectorType >
  class GatherScatterBaseImpl<GridType,DataCollectorType,0> : public GatherScatter
  {
  protected:  
    enum { codim = 0 };
    const GridType & grid_;
    typedef typename GridType::template Codim<0>::Entity EntityType;
    typedef Dune :: MakeableInterfaceObject<
      typename GridType::template Codim<0>::Entity> MakeableEntityType;
    typedef typename MakeableEntityType :: ImplementationType RealEntityType;
    
    typedef typename GridType::MPICommunicatorType Comm;

    typedef Dune::ALU3dImplTraits< GridType::elementType, Comm > ImplTraits;
    typedef typename ImplTraits::template Codim< codim >::ImplementationType ImplElementType;
    typedef typename ImplTraits::template Codim< codim >::InterfaceType HElementType;
    
    typedef typename ImplTraits::template Codim< 1 >::InterfaceType HFaceType;
    
    typedef typename ImplTraits::template Codim< codim >::GhostInterfaceType HGhostType;
    typedef typename ImplTraits::template Codim< codim >::GhostImplementationType ImplGhostType;

    typedef typename ImplTraits::PllElementType PllElementType;

    EntityType& entity_;
    RealEntityType & realEntity_;

    // data handle 
    DataCollectorType & dc_;

    const bool variableSize_;

    // used MessageBuffer 
    typedef typename GatherScatter :: ObjectStreamType ObjectStreamType;

    // use all other containsItem from the base class 
    using GatherScatter :: setData ;
    using GatherScatter :: sendData ;
    using GatherScatter :: recvData ;

  public:
    // use all other containsItem from the base class 
    using GatherScatter :: containsItem ;

    //! Constructor
    GatherScatterBaseImpl(const GridType & grid, MakeableEntityType & en, 
        RealEntityType & realEntity , DataCollectorType & dc) 
      : grid_(grid), entity_(en), realEntity_(realEntity) 
      , dc_(dc) , variableSize_ ( ! dc_.fixedsize( EntityType :: dimension, codim ))
    {}

    // return true if dim,codim combination is contained in data set 
    bool contains(int dim, int codim) const 
    {
      return dc_.contains(dim,codim);
    }

    // return true if item might from entity belonging to data set  
    virtual bool containsItem (const HElementType & elem) const 
    {
      return elem.isLeafEntity();
    }

    // return true if item might from entity belonging to data set  
    virtual bool containsItem (const HGhostType & ghost) const = 0; 

    //! write Data of one element to stream 
    void sendData ( ObjectStreamType & str , const HElementType & elem )
    {
      alugrid_assert ( this->containsItem(elem) );
      realEntity_.setElement( const_cast<HElementType &> (elem) );

      // write size in case of variable size 
      writeSize( str, entity_);
      // gather data 
      dc_.gather(str, entity_);
    }
   
    //! write Data of one ghost element to stream 
    void sendData ( ObjectStreamType & str , const HGhostType& ghost)
    {
      alugrid_assert ( this->containsItem( ghost ) );

      // set ghost as entity
      realEntity_.setGhost( const_cast <HGhostType &> (ghost) );

      // write size in case of variable size 
      writeSize( str, entity_);
      // gather data 
      dc_.gather(str, entity_);
    }
   
    //! read Data of one element from stream 
    void recvData ( ObjectStreamType & str , HElementType & elem )
    {
      // alugrid_assert ( this->containsItem( elem ) );
      realEntity_.setElement( elem ); 
      
      size_t size = getSize(str, entity_); 
      dc_.scatter(str, entity_, size);
    }

    //! read Data of one element from stream 
    void recvData ( ObjectStreamType & str , HGhostType & ghost )
    {
      alugrid_assert ( this->containsItem( ghost ) );

      // set ghost as entity
      realEntity_.setGhost( ghost );

      size_t size = getSize(str , entity_ ); 
      dc_.scatter(str, entity_, size );
    }
    
  protected:  
    size_t getSize(ObjectStreamType & str, EntityType & en)
    {
      if(variableSize_) 
      {
        size_t size;
        str.read(size);
        return size;
      }
      else 
        return dc_.size(en);
    }
    
    // write variable size to stream 
    void writeSize(ObjectStreamType & str, EntityType & en)
    {
      if( variableSize_ )
      {
        size_t size = dc_.size( en );
        str.write( size );
      }
    }
  };

  //! the corresponding interface class is defined in bsinclude.hh
  template< class GridType, class DataCollectorType, int codim >
  class GatherScatterLeafData 
  : public GatherScatterBaseImpl< GridType, DataCollectorType, codim >
  {
    enum { dim = GridType :: dimension };

    typedef GatherScatterBaseImpl<GridType,DataCollectorType,codim> BaseType;
    typedef typename GridType::template Codim<codim>::Entity EntityType;
    typedef Dune :: MakeableInterfaceObject<
      typename GridType::template Codim<codim>::Entity> MakeableEntityType;
    typedef typename MakeableEntityType :: ImplementationType RealEntityType;

    typedef typename GridType::MPICommunicatorType Comm;

    typedef Dune::ALU3dImplTraits< GridType::elementType, Comm > ImplTraits;
    typedef typename ImplTraits::template Codim< codim >::ImplementationType IMPLElementType;
    typedef typename ImplTraits::template Codim< codim >::InterfaceType HElementType;
    
    typedef typename ImplTraits::template Codim< 1 >::InterfaceType HFaceType;
    
    typedef typename ImplTraits::template Codim< 0 >::GhostInterfaceType HGhostType;
    typedef typename ImplTraits::template Codim< 0 >::GhostImplementationType ImplGhostType;

    typedef typename ImplTraits::PllElementType PllElementType;

  public:
    // use all other containsItem methods from the base class 
    using BaseType :: containsItem ;

    //! Constructor
    GatherScatterLeafData(const GridType & grid, MakeableEntityType & en, 
        RealEntityType & realEntity , DataCollectorType & dc)
      : BaseType(grid,en,realEntity,dc) 
    {
      // if leaf vertices are communicated, 
      // make sure that vertex list is up2date 
      // but only do this, if vertex data contained,
      // because the list update is expensive  
      if( (codim == 3) && dc.contains(dim,codim) ) 
      {
        // call of this method forces update of list, 
        // if list is not up to date
        grid.getLeafVertexList();
      }
    } 

    // returns true, if element is contained in set of comm interface 
    bool containsItem (const HElementType & elem) const 
    {
      return elem.isLeafEntity();
    }

    // returns true, if element is contained in set of comm interface 
    bool containsItem (const HGhostType & ghost) const 
    {
      return ghost.isLeafEntity();
    }

    // returns true, if interior element is contained in set of comm interface 
    bool containsInterior (const HFaceType & face, PllElementType & pll) const 
    {
      return face.isInteriorLeaf();
    }

    // returns true, if ghost is contianed in set of comm interface 
    bool containsGhost (const HFaceType & face , PllElementType & pll) const 
    {
      return pll.ghostLeaf();
    }

    // set elem to realEntity
    void setElement(const HElementType & elem)
    {
      this->realEntity_.setElement(elem); 
    }
  };

  //! the corresponding interface class is defined in bsinclude.hh
  template <class GridType, class DataCollectorType , int codim >
  class GatherScatterLevelData 
  : public GatherScatterBaseImpl<GridType,DataCollectorType,codim> 
  {
    typedef GatherScatterBaseImpl<GridType,DataCollectorType,codim> BaseType;
    typedef typename GridType::template Codim<codim>::Entity EntityType;
    typedef Dune :: MakeableInterfaceObject<
      typename GridType::template Codim<codim>::Entity> MakeableEntityType;
    typedef typename MakeableEntityType :: ImplementationType RealEntityType;

    typedef typename GridType::MPICommunicatorType Comm;

    typedef Dune::ALU3dImplTraits< GridType::elementType, Comm > ImplTraits;
    typedef typename ImplTraits::template Codim< codim >::ImplementationType IMPLElementType;
    typedef typename ImplTraits::template Codim< codim >::InterfaceType HElementType;
    
    typedef typename ImplTraits::template Codim< 1 >::InterfaceType HFaceType;
    
    typedef typename ImplTraits::template Codim< 0 >::GhostInterfaceType HGhostType;
    typedef typename ImplTraits::template Codim< 0 >::GhostImplementationType ImplGhostType;

    typedef typename ImplTraits::PllElementType PllElementType;

    typedef typename GridType::LevelIndexSetImp LevelIndexSetImp;

    const LevelIndexSetImp & levelSet_;
    const int level_;
  public:
    // use containsItem for ghost element from BaseType
    using BaseType :: containsItem ;

    //! Constructor
    GatherScatterLevelData(const GridType & grid, MakeableEntityType & en, 
        RealEntityType & realEntity , DataCollectorType & dc, 
        const LevelIndexSetImp & levelSet, const int level)
      : BaseType(grid,en,realEntity,dc) , levelSet_(levelSet) , level_(level) 
    {
    } 

    // returns true, if element is contained in set of comm interface 
    bool containsItem (const HElementType & elem) const 
    {
      return levelSet_.containsIndex(codim, elem.getIndex() );
    }

    // set elem to realEntity
    void setElement(const HElementType & elem)
    {
      this->realEntity_.setElement(elem,level_); 
    }
      
  };


  //! the corresponding interface class is defined in bsinclude.hh
  template <class GridType, class DataCollectorType>
  class GatherScatterLevelData<GridType,DataCollectorType,0>
  : public GatherScatterBaseImpl<GridType,DataCollectorType,0> 
  {
    enum { codim = 0 };
    typedef GatherScatterBaseImpl<GridType,DataCollectorType,codim> BaseType;
    typedef typename GridType::template Codim<codim>::Entity EntityType;
    typedef Dune :: MakeableInterfaceObject<
      typename GridType::template Codim<codim>::Entity> MakeableEntityType;
    typedef typename MakeableEntityType :: ImplementationType RealEntityType;

    typedef typename GridType::MPICommunicatorType Comm;

    typedef Dune::ALU3dImplTraits< GridType::elementType, Comm > ImplTraits;
    typedef typename ImplTraits::template Codim< codim >::ImplementationType IMPLElementType;
    typedef typename ImplTraits::template Codim< codim >::InterfaceType HElementType;
    
    typedef typename ImplTraits::template Codim< 1 >::InterfaceType HFaceType;
    
    typedef typename ImplTraits::template Codim< 0 >::GhostInterfaceType HGhostType;
    typedef typename ImplTraits::template Codim< 0 >::GhostImplementationType ImplGhostType;

    typedef typename ImplTraits::PllElementType PllElementType;

    typedef typename GridType::LevelIndexSetImp LevelIndexSetImp;

    const LevelIndexSetImp & levelSet_;
    const int level_;
  public:
    //! Constructor
    GatherScatterLevelData(const GridType & grid, MakeableEntityType & en, 
        RealEntityType & realEntity , DataCollectorType & dc, 
        const LevelIndexSetImp & levelSet, const int level)
      : BaseType(grid,en,realEntity,dc) , levelSet_(levelSet) , level_(level) {} 

    // returns true, if element is contained in set of comm interface 
    bool containsItem (const HElementType & elem) const 
    {
      return levelSet_.containsIndex(codim, elem.getIndex() );
    }

    // returns true, if element is contained in set of comm interface 
    bool containsItem (const HGhostType & ghost) const 
    {
      alugrid_assert ( ghost.getGhost().first );
      return containsItem( * (ghost.getGhost().first) );
    }
    
    // returns true, if interior element is contained in set of comm interface 
    bool containsInterior (const HFaceType & face, PllElementType & pll) const 
    {
      // if face level is not level_ then interior cannot be contained 
      if(face.level() != level_) return false;

      typedef Gitter::helement_STI HElementType;
      typedef Gitter::hbndseg_STI HBndSegType;

      // check interior element here, might have a coarser level
      std::pair< HElementType *, HBndSegType * > p( (HElementType *)0, (HBndSegType *)0 );
      pll.getAttachedElement( p );
      alugrid_assert ( p.first );
      // check inside level 
      bool contained = (p.first->level() == level_);
      alugrid_assert ( contained == this->containsItem( *p.first ));
      return contained;
    }

    // returns true, if ghost is contianed in set of comm interface 
    bool containsGhost (const HFaceType & face, PllElementType & pll) const 
    {
      // if face level is not level_ then ghost cannot be contained 
      if(face.level() != level_) return false;
      // otherwise check ghost level 
      return (pll.ghostLevel() == level_);
    }
  };

  //! the corresponding interface class is defined in bsinclude.hh
  template <class GridType, class DataCollectorType> 
  class GatherScatterLoadBalance : public GatherScatter
  {
  protected:  
    enum { codim = 0 };
    typedef typename GridType::MPICommunicatorType Comm;

    typedef Dune::ALU3dImplTraits< GridType::elementType, Comm > ImplTraits;
    typedef typename ImplTraits::template Codim< codim >::ImplementationType IMPLElementType;
    typedef typename ImplTraits::template Codim< codim >::InterfaceType HElementType;

    typedef typename GridType::template Codim<0>::Entity      EntityType;
    typedef Dune :: MakeableInterfaceObject<EntityType>       MakeableEntityType;
    typedef typename MakeableEntityType :: ImplementationType RealEntityType;

    typedef typename ImplTraits::template Codim< 1 >::InterfaceType HFaceType;
    
    typedef typename ImplTraits::template Codim< 0 >::GhostInterfaceType HGhostType;
    typedef typename ImplTraits::template Codim< 0 >::GhostImplementationType ImplGhostType;

    typedef typename ImplTraits::PllElementType PllElementType;

    GridType & grid_;

    MakeableEntityType  entityObj_;
    EntityType&         entity_;
    RealEntityType&     realEntity_;

    // data handle 
    DataCollectorType & dc_;

    // used MessageBuffer 
    typedef typename GatherScatter :: ObjectStreamType ObjectStreamType;

    using GatherScatter :: inlineData ;
    using GatherScatter :: xtractData ;

  public:
    //! Constructor
    GatherScatterLoadBalance(GridType & grid, DataCollectorType & dc )
      : grid_(grid), 
        entityObj_( RealEntityType( grid.factory(), grid.maxLevel() ) ), 
        entity_( entityObj_ ), realEntity_( GridType::getRealImplementation( entity_ ) ), 
        dc_(dc)
    {}

    // return true if dim,codim combination is contained in data set 
    bool contains(int dim, int codim) const 
    {
      return true; 
    }

    //! this method is called from the dunePackAll method of the corresponding 
    //! Macro element class of the BSGrid, see gitter_dune_pll*.*
    //! here the data is written to the ObjectStream 
    void inlineData ( ObjectStreamType & str , HElementType & elem, const int estimatedElements )
    {
      int mxl = grid_.maxLevel();
      str.write(mxl);
      // store number of elements to be written (for restore)
      str.write(estimatedElements);
      // set element and then start 
      alugrid_assert ( elem.level () == 0 );
      realEntity_.setElement(elem);
      dc_.inlineData(str,entity_);
    }

    //! this method is called from the duneUnpackSelf method of the corresponding 
    //! Macro element class of the BSGrid, see gitter_dune_pll*.*
    //! here the data is read from the ObjectStream 
    void xtractData ( ObjectStreamType & str , HElementType & elem )
    {
      alugrid_assert ( elem.level () == 0 );
      int mxl = 0; 
      str.read(mxl);
      int newElements = 0 ;
      str.read( newElements );
      // set element and then start 
      grid_.setMaxLevel(mxl);

      realEntity_.setElement(elem);
      dc_.xtractData(str, entity_, newElements);
    }

    //! call compress on data 
    void compress () 
    {
      dc_.compress();
    } 

    // return true if user defined partitioning methods should be used 
    bool userDefinedPartitioning () const 
    {
      return dc_.userDefinedPartitioning();
    }

    // return true if user defined load balancing weights are provided
    bool userDefinedLoadWeights () const
    {
      return dc_.userDefinedLoadWeights();
    }

    // returns true if user defined partitioning needs to be readjusted 
    bool repartition () 
    { 
      return (dc_.userDefinedPartitioning() && dc_.repartition());
    }

    // return set of ranks data is imported from during load balance
    // this method is only used for user defined repartitioning  
    bool importRanks( std::set<int>& ranks ) const 
    {
      return dc_.importRanks( ranks );
    }

    // return set of ranks data is exported to during load balance
    // this method is only used for user defined repartitioning  
    bool exportRanks( std::set<int>& ranks ) const 
    {
      return dc_.exportRanks( ranks );
    }

    // return load weight of given element 
    int loadWeight ( const HElementType &elem ) const
    {
      alugrid_assert ( elem.level() == 0 );
      if( dc_.userDefinedLoadWeights() )
      {
        realEntity_.setElement( elem );
        return dc_.loadWeight( entity_ );
      }
      else
        return 1; 
    }

    // return destination (i.e. rank) where the given element should be moved to 
    // this needs the methods userDefinedPartitioning to return true
    int destination ( const HElementType &elem ) const
    { 
      alugrid_assert ( elem.level () == 0 );
      if( dc_.userDefinedPartitioning() )
      {
        realEntity_.setElement( elem );
        return dc_.destination( entity_ );
      }
      else
        return -1; 
    }
  };

  /////////////////////////////////////////////////////////////////
  //
  //  --AdaptRestrictProlong 
  //
  /////////////////////////////////////////////////////////////////
  template< class GridType, class AdaptDataHandle >
  class AdaptRestrictProlongImpl
  : public AdaptRestrictProlongType
  {
    GridType & grid_;
    typedef typename GridType::template Codim<0>::Entity EntityType;
    typedef Dune :: MakeableInterfaceObject<
      typename GridType::template Codim<0>::Entity> MakeableEntityType;
    typedef typename MakeableEntityType :: ImplementationType RealEntityType;
    
    EntityType & reFather_;
    EntityType & reSon_;
    RealEntityType & realFather_;
    RealEntityType & realSon_;
   
    AdaptDataHandle &rp_;

    typedef typename GridType::MPICommunicatorType Comm;

    typedef Dune::ALU3dImplTraits< GridType::elementType, Comm > ImplTraits;
    typedef typename ImplTraits::HElementType HElementType;
    typedef typename ImplTraits::HBndSegType  HBndSegType;
    typedef typename ImplTraits::BNDFaceType  BNDFaceType;

    //using AdaptRestrictProlongType :: postRefinement ;
    //using AdaptRestrictProlongType :: preCoarsening ;

  public:
    //! Constructor
    AdaptRestrictProlongImpl ( GridType &grid,
                               MakeableEntityType &f, RealEntityType &rf,
                               MakeableEntityType &s, RealEntityType &rs,
                               AdaptDataHandle &rp ) 
      : grid_(grid)
      , reFather_(f)
      , reSon_(s)
      , realFather_(rf) 
      , realSon_(rs) 
      , rp_(rp) 
    {
    }

    virtual ~AdaptRestrictProlongImpl () 
    {
    }

    //! restrict data for elements 
    int preCoarsening ( HElementType & father )
    {
      realFather_.setElement( father );
      rp_.preCoarsening( reFather_ );
     
      // reset refinement marker 
      father.resetRefinedTag();
      return 0;
    }

    //! prolong data for elements 
    int postRefinement ( HElementType & father )
    {
      realFather_.setElement( father );
      rp_.postRefinement( reFather_ );

      // reset refinement markers
      father.resetRefinedTag();
      for( HElementType *son = father.down(); son ; son = son->next() )
        son->resetRefinedTag();

      return 0;
    }

    //! restrict data for ghost elements 
    int preCoarsening ( HBndSegType & ghost ) { return 0; }


    //! prolong data for ghost elements 
    int postRefinement ( HBndSegType & ghost ) { return 0; }
  };



  template< class GridType, class AdaptDataHandle, class GlobalIdSetImp >
  class AdaptRestrictProlongGlSet
  : public AdaptRestrictProlongImpl< GridType, AdaptDataHandle >
  {
    typedef AdaptRestrictProlongImpl< GridType, AdaptDataHandle > BaseType;
    GlobalIdSetImp & set_;
    typedef typename GridType::template Codim<0>::Entity EntityType;
    typedef Dune :: MakeableInterfaceObject<
      typename GridType::template Codim<0>::Entity> MakeableEntityType;
    typedef typename MakeableEntityType :: ImplementationType RealEntityType;
    
    typedef typename GridType::MPICommunicatorType Comm;

    typedef Dune::ALU3dImplTraits< GridType::elementType, Comm > ImplTraits;
    typedef typename ImplTraits::HElementType HElementType;
    typedef typename ImplTraits::HBndSegType HBndSegType;

    using AdaptRestrictProlongType :: postRefinement ;
    using AdaptRestrictProlongType :: preCoarsening ;

  public:
    //! Constructor
    AdaptRestrictProlongGlSet ( GridType &grid,
                                MakeableEntityType &f, RealEntityType &rf,
                                MakeableEntityType &s, RealEntityType &rs,
                                AdaptDataHandle &rp,
                                GlobalIdSetImp & set )
    : BaseType( grid, f, rf, s, rs, rp ),
      set_( set )
    {}

    virtual ~AdaptRestrictProlongGlSet () {}

    //! prolong data, elem is the father  
    int postRefinement ( HElementType & elem )
    {
      set_.postRefinement( elem );
      return BaseType :: postRefinement(elem );
    }
  };

} // namespace ALUGrid

#endif // #ifndef DUNE_ALU3DGRIDDATAHANDLE_HH
