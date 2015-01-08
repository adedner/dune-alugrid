#ifndef DUNE_ALU3DGRID_FACTORY_CC 
#define DUNE_ALU3DGRID_FACTORY_CC 

#if COMPILE_ALUGRID_INLINE == 0
#include <config.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>

#include <dune/common/parallel/mpicollectivecommunication.hh>
#include <dune/alugrid/3d/gridfactory.hh>

#include <dune/alugrid/common/hsfc.hh>

#if COMPILE_ALUGRID_INLINE
#define alu_inline inline 
#else
#define alu_inline
#endif

namespace Dune
{
  
  template< class ALUGrid >
  alu_inline
  ALU3dGridFactory< ALUGrid > :: ~ALU3dGridFactory ()
  {}

  
  template< class ALUGrid >
  alu_inline
  void ALU3dGridFactory< ALUGrid > :: insertVertex ( const VertexInputType &pos )
  {
    doInsertVertex( pos, vertices_.size() );
  } 


  template< class ALUGrid >
  alu_inline 
  void ALU3dGridFactory< ALUGrid >::insertVertex ( const VertexInputType &coord, const VertexId globalId )
  {
    // mark that vertices with global id have been inserted
    foundGlobalIndex_ = true;
    doInsertVertex( coord, globalId );
  }

  template< class ALUGrid >
  alu_inline 
  void ALU3dGridFactory< ALUGrid >::doInsertVertex ( const VertexInputType &coord, const VertexId globalId )
  {
    VertexType pos ( 0 );
    // copy coordinates since input vertex might only have 2 coordinates
    const int size = coord.size();
    for( int i=0; i<size; ++i )
      pos[ i ] = coord[ i ];

    // nothing to do for 3d
    if(dimension == 3)
    {
      vertices_.push_back( std::make_pair( pos, globalId ) );
    }
    else if (dimension == 2)
    {
      if( elementType == tetra )
      { 
        if(vertices_.size() == 0)
          vertices_.push_back( std::make_pair( VertexType(1.0), 0 ) );

        vertices_.push_back( std::make_pair( pos, globalId+1 ) );
      }
      else if(elementType == hexa)
      {
        vertices_.push_back( std::make_pair( pos, 2*globalId ) );
        VertexType pos1 (pos);
          pos1[2] += 1.0 ;
        vertices_.push_back( std::make_pair( pos1, 2*globalId+1 ) );
      }
    }
  }

  
  template< class ALUGrid >
  alu_inline
  void ALU3dGridFactory< ALUGrid >
    :: insertElement ( const GeometryType &geometry,
                       const std::vector< VertexId > &vertices )
  {
    assertGeometryType( geometry );
    
    if( geometry.dim() != dimension )
      DUNE_THROW( GridError, "Only 3-dimensional elements can be inserted "
                             "into a 3-dimensional ALUGrid." );
    if(dimension == 3){
      if( vertices.size() != numCorners )
        DUNE_THROW( GridError, "Wrong number of vertices." );
      elements_.push_back( vertices );
    }
    else if (dimension == 2)
    {
      std::vector< VertexId > element;
      if( elementType == hexa)
      {
        element.resize( 8 );
        for (int i = 0; i < 4; ++i)
        {
          // multiply original number with 2 to get the indices of the 2dvalid vertices
          element[ i ]    = vertices[ i ] * 2;
          element[ i+4 ]  = element [ i ] + 1;
        }
      }
      else if ( elementType == tetra )
      {
        element.resize( 4 );

        // construct element following the DUNE reference tetrahedron
        element[0] = 0;                
        element[1] = vertices[ 0 ] + 1;
        element[2] = vertices[ 1 ] + 1;
        element[3] = vertices[ 2 ] + 1;
      }
      elements_.push_back(element);
    }
  }


  template< class ALUGrid >
  alu_inline
  void ALU3dGridFactory< ALUGrid >
    :: insertBoundary ( const GeometryType &geometry,
                        const std::vector< VertexId > &vertices,
                        const int id )
  {
    assertGeometryType( geometry );
    if( geometry.dim() != dimension-1 )
    {
      DUNE_THROW( GridError, "Only 2-dimensional boundaries can be inserted "
                             "into a 3-dimensional ALUGrid." );
    }
    if( vertices.size() != numFaceCorners )
      DUNE_THROW( GridError, "Wrong number of vertices." );

    BndPair boundaryId;
    if(dimension == 3)
    { 
      for( unsigned int i = 0; i < numFaceCorners; ++i )
      {
        const unsigned int j = FaceTopologyMappingType::dune2aluVertex( i );
        boundaryId.first[ j ] = vertices[ i ];
      }
    }
    else if(dimension == 2)
    {
      VertexId face[ 4 ];
      if(elementType == tetra)
      {
        face[2] = vertices[1]+1;
        face[1] = vertices[0]+1;
        face[0] = 0;
      }
      else if(elementType == hexa)
      {
        face[0] = 2*vertices[0];
        face[3] = 2*vertices[1];
        face[1] = face[0]+1;
        face[2] = face[3]+1;       
      }
      const int nFace = elementType == hexa ? 4 : 3;
      for (int i = 0; i < nFace; ++i)
      {
        boundaryId.first[ i ] = face[ i ];       
      }
    }

    boundaryId.second = id;
    boundaryIds_.insert( boundaryId );
  }


  template< class ALUGrid >
  alu_inline
  void ALU3dGridFactory< ALUGrid >
    ::insertBoundary ( const int element, const int face, const int id )
  {
    if( (element < 0) || (element >= (int)elements_.size()) )
      DUNE_THROW( RangeError, "ALU3dGridFactory::insertBoundary: invalid element index given." );
  
    BndPair boundaryId;
   generateFace( elements_[ element ], face, boundaryId.first );
    std::cout <<  "Element: [" << elements_[element][0] << ","<<elements_[element][1]<<"," << elements_[element][2] << "," << elements_[element][3] <<"] Face: " << face << " Boundary: " <<     boundaryId.first  << std::endl;
    
    //in 2d the local face ids are correct, because we need the faces 0,1,2 in tetra and 0,1,2,3 for hexas
    //and that is exactly what we get form the 2d dgfparser.
    
    doInsertBoundary( element, face, id );
  }

  template< class ALUGrid >
  alu_inline
  void ALU3dGridFactory< ALUGrid >
    ::doInsertBoundary ( const int element, const int face, const int id )
  {
    if( (element < 0) || (element >= (int)elements_.size()) )
      DUNE_THROW( RangeError, "ALU3dGridFactory::insertBoundary: invalid element index given." );

    BndPair boundaryId;
    generateFace( elements_[ element ], face, boundaryId.first );
    boundaryId.second = id;
    boundaryIds_.insert( boundaryId );
  }

  template< class ALUGrid >
  alu_inline
  void ALU3dGridFactory< ALUGrid > :: 
  insertBoundaryProjection( const DuneBoundaryProjectionType& bndProjection ) 
  {
    if( globalProjection_ ) 
      DUNE_THROW(InvalidStateException,"You can only insert one globalProjection");

    globalProjection_ = &bndProjection; 
  }


  template< class ALUGrid >
  alu_inline
  void ALU3dGridFactory< ALUGrid > :: 
  insertBoundaryProjection ( const GeometryType &type,
                             const std::vector< VertexId > &vertices,
                             const DuneBoundaryProjectionType *projection )
  {
    if( (int)type.dim() != dimension-1 )
      DUNE_THROW( GridError, "Inserting boundary face of wrong dimension: " << type.dim() );
    alugrid_assert ( type.isCube() || type.isSimplex() );

    FaceType faceId;
    copyAndSort( vertices, faceId );

    if( vertices.size() != numFaceCorners )
      DUNE_THROW( GridError, "Wrong number of face vertices passed: " << vertices.size() << "." );

    if( boundaryProjections_.find( faceId ) != boundaryProjections_.end() )
      DUNE_THROW( GridError, "Only one boundary projection can be attached to a face." );
    boundaryProjections_[ faceId ] = projection;
  }


  template< class ALUGrid >
  alu_inline
  void ALU3dGridFactory< ALUGrid >
    ::insertFaceTransformation ( const WorldMatrix &matrix, const WorldVector &shift )
  {
    faceTransformations_.push_back( Transformation( matrix, shift ) );
  }

  template< class ALUGrid >
  alu_inline
  void ALU3dGridFactory< ALUGrid >
    ::sortElements( const VertexVector& vertices, 
                    const ElementVector& elements,
                    std::vector< int >& ordering ) 
  {
    const size_t elemSize = elements.size(); 
    ordering.resize( elemSize );
    // default ordering
    for( size_t i=0; i<elemSize; ++i ) ordering[ i ] = i;

#ifdef USE_ZOLTAN_HSFC_ORDERING
    // the serial version do not special ordering 
    // since no load balancing has to be done
    {
      typedef MPIHelper :: MPICommunicator MPICommunicator;
      CollectiveCommunication< MPICommunicator > comm( Dune::MPIHelper::getCommunicator() );

      // if we are in parallel insertion mode we need communication
      const bool foundGlobalIndex = comm.max( foundGlobalIndex_ );
      if( foundGlobalIndex ) 
      {
        if( comm.rank() == 0 ) 
          std::cerr << "WARNING: Hilbert space filling curve ordering does not work for parallel grid factory yet!" << std::endl;
        return ;
      }

      VertexType maxCoord;
      VertexType minCoord;
      const size_t vertexSize = vertices.size();
      if( vertexSize > 0 ) 
      {
        maxCoord = vertices[ 0 ].first;
        minCoord = vertices[ 0 ].first;
      }

      for( size_t i=0; i<vertexSize; ++i ) 
      {
        const VertexType& vx = vertices[ i ].first;
        for( unsigned int d=0; d<dimension; ++d )
        {
          maxCoord[ d ] = std::max( maxCoord[ d ], vx[ d ] );
          minCoord[ d ] = std::min( minCoord[ d ], vx[ d ] );
        }
      }
     
      // get element's center to hilbert index mapping
      SpaceFillingCurveOrdering< VertexType > sfc( minCoord, maxCoord, comm );

      typedef std::map< double, int > hsfc_t;
      hsfc_t hsfc;

      for( size_t i=0; i<elemSize; ++i ) 
      {
        VertexType center( 0 );
        // compute barycenter 
        const int vxSize = elements[ i ].size(); 
        for( int vx = 0; vx<vxSize; ++vx ) 
        {
          const VertexType& vertex = vertices[ elements[ i ][ vx ] ].first;
          for( unsigned int d=0; d<dimension; ++d )
            center[ d ] += vertex[ d ];
        }
        center /= double(vxSize);

        // generate hilbert index from element's center and store index 
        hsfc[ sfc.hilbertIndex( center ) ] = i;
      }

      typedef typename hsfc_t :: iterator iterator;
      const iterator end = hsfc.end(); 
      size_t idx = 0;
      for( iterator it = hsfc.begin(); it != end; ++it, ++idx )
      {
        ordering[ idx ] = (*it).second ;
      }
    }
#endif
  }

  template< class ALUGrid >
  alu_inline
  ALUGrid *ALU3dGridFactory< ALUGrid >::createGrid ()
  {
    return createGrid( true, true, "" );
  }

  template< class ALUGrid >
  alu_inline
  ALUGrid *ALU3dGridFactory< ALUGrid >
    ::createGrid ( const bool addMissingBoundaries, const std::string dgfName )
  {
    return createGrid( addMissingBoundaries, true, dgfName );
  }

  template< class ALUGrid >
  alu_inline
  ALUGrid *ALU3dGridFactory< ALUGrid >
    ::createGrid ( const bool addMissingBoundaries, bool temporary, const std::string name )
  {
    typedef typename BoundaryIdMap :: iterator  BoundaryIdIteratorType;
    BoundaryProjectionVector* bndProjections = 0;

    std::vector< int > ordering;
    // sort element given a hilbert space filling curve (if Zoltan is available)
    sortElements( vertices_, elements_, ordering );


    correctElementOrientation();
    numFacesInserted_ = boundaryIds_.size();
    if( addMissingBoundaries || ! faceTransformations_.empty() || dimension == 2)
      recreateBoundaryIds();
      

    // if dump file should be written 
    if( allowGridGeneration_ && !temporary )
    {
      std::string filename ( name );
      
      std::ofstream out( filename.c_str() );
      out.setf( std::ios_base::scientific, std::ios_base::floatfield );
      out.precision( 16 );
      if( elementType == tetra )
        out << "!Tetrahedra"; 
      else
        out << "!Hexahedra";

      const unsigned int numVertices = vertices_.size();
      // print information about vertices and elements 
      // to header to have an easy check 
      out << "  ( noVertices = " << numVertices;
      out << " | noElements = " << elements_.size() << " )" << std :: endl;

      // now start writing grid 
      out << numVertices << std :: endl;
      typedef typename VertexVector::iterator VertexIteratorType;
      const VertexIteratorType endV = vertices_.end();
      for( VertexIteratorType it = vertices_.begin(); it != endV; ++it )
      {
        const VertexType &vertex = it->first;
        const int globalId = it->second;
        out << globalId ;
        for( unsigned int i = 0; i < dimensionworld; ++i )
          out << " " << vertex[ i ];
        out << std :: endl;
      }

      const unsigned int elemSize = elements_.size();
      out << elemSize << " " << int(numCorners) << std :: endl;
      for( unsigned int el = 0; el<elemSize; ++el )
      {
        const size_t elemIndex = ordering[ el ];
        array< unsigned int, numCorners > element;
        for( unsigned int i = 0; i < numCorners; ++i )
        {
          const unsigned int j = ElementTopologyMappingType::dune2aluVertex( i );
          element[ j ] = elements_[ elemIndex ][ i ];
        }

        out << element[ 0 ];
        for( unsigned int i = 1; i < numCorners; ++i )
          out << " " << element[ i ];
        out << std :: endl;
      }

      out << int(periodicBoundaries_.size()) << " " << int(boundaryIds_.size()) << std :: endl;
      const typename PeriodicBoundaryVector::iterator endP = periodicBoundaries_.end();
      for( typename PeriodicBoundaryVector::iterator it = periodicBoundaries_.begin(); it != endP; ++it )
      {
        const std::pair< BndPair, BndPair > &facePair = *it;
        out << facePair.first.first[ 0 ];
        for( unsigned int i = 1; i < numFaceCorners; ++i )
          out << " " << facePair.first.first[ numFaceCorners == 3 ? (3 - i) % 3 : i ];
        for( unsigned int i = 0; i < numFaceCorners; ++i )
          out << " " << facePair.second.first[ numFaceCorners == 3 ? (3 - i) % 3 : i ];
        out << std::endl;
      }
      const BoundaryIdIteratorType endB = boundaryIds_.end();
      for( BoundaryIdIteratorType it = boundaryIds_.begin(); it != endB; ++it )
      {
        const std::pair< FaceType, int > &boundaryId = *it;
        out << -boundaryId.second;
        for( unsigned int i = 0; i < numFaceCorners; ++i )
          out << " " << boundaryId.first[ i ];
        out << std::endl;
      }

      // no linkage 
      out << int(0) << std::endl; 

      out.close();
    }

    const size_t boundarySegments = boundaryIds_.size();

    const size_t bndProjectionSize = boundaryProjections_.size();
    if( bndProjectionSize > 0 ) 
    {
      // the memory is freed by the grid on destruction 
      bndProjections = new BoundaryProjectionVector( boundarySegments,
                                                    (DuneBoundaryProjectionType*) 0 );
      const BoundaryIdIteratorType endB = boundaryIds_.end();
      int segmentIndex = 0;
      for( BoundaryIdIteratorType it = boundaryIds_.begin(); it != endB; ++it, ++segmentIndex )
      {
        // generate boundary segment pointer 
        FaceType faceId ( (*it).first);
        std::sort( faceId.begin(), faceId.end() );

        const DuneBoundaryProjectionType* projection = boundaryProjections_[ faceId ];

        // if no projection given we use global projection, otherwise identity 
        if( ! projection && globalProjection_ )
        {
          typedef BoundaryProjectionWrapper< dimensionworld > ProjectionWrapperType;
          // we need to wrap the global projection because of 
          // delete in desctructor of ALUGrid 
          projection = new ProjectionWrapperType( *globalProjection_ );
          alugrid_assert ( projection );
        }

        // copy pointer 
        (*bndProjections)[ segmentIndex ] = projection;
      }
    } // if( allowGridGeneration_ && !temporary )

    // free memory 
    boundaryProjections_.clear();

    // if we have a vector reset global projection 
    // because empty positions are filled with global projection anyway  
    if( bndProjections ) globalProjection_ = 0;

    // ALUGrid is taking ownership of bndProjections 
    // and is going to delete this pointer 
    Grid* grid = createGridObj( bndProjections , name );
    alugrid_assert ( grid );

    // remove pointers 
    globalProjection_ = 0;
    // is removed by grid instance 
    bndProjections    = 0;

    // insert grid using ALUGrid macro grid builder   
    if( !vertices_.empty() )
    {
      ALU3DSPACE MacroGridBuilder mgb ( grid->getBuilder(), grid->vertexProjection() );

      // now start inserting grid 
      const int vxSize = vertices_.size(); 

      for( int vxIdx = 0; vxIdx < vxSize ; ++vxIdx )
      {
        const VertexType &vertex = position( vxIdx );
        if(dimensionworld == 3)
        {
          // insert vertex 
          mgb.InsertUniqueVertex( vertex[ 0 ], vertex[ 1 ], vertex[ 2 ], globalId( vxIdx ) );
        }
        else if (dimensionworld == 2 && elementType == hexa)
        {
          if(globalId (vxIdx) % 2 == 0 )
            mgb.InsertUniqueVertex( vertex[ 0 ], vertex[ 1 ], 0., globalId( vxIdx ) );         
          else
            mgb.InsertUniqueVertex( vertex[ 0 ], vertex[ 1 ], 1., globalId( vxIdx ) );                    
        }
        else if (dimensionworld == 2 && elementType ==tetra)
        {
          if(globalId(vxIdx) == 0)
            mgb.InsertUniqueVertex( vertex[ 0 ], vertex[ 1 ], 1., globalId( vxIdx ) );                    
          else 
            mgb.InsertUniqueVertex( vertex[ 0 ], vertex[ 1 ], 0., globalId( vxIdx ) );                               
        }
      }

      const size_t elemSize = elements_.size();
      for( size_t el = 0; el<elemSize; ++el )
      {
        const size_t elemIndex = ordering[ el ];
        if( elementType == hexa )
        {
          int element[ 8 ];
          for( unsigned int i = 0; i < 8; ++i )
          {
            const unsigned int j = ElementTopologyMappingType::dune2aluVertex( i );
            element[ j ] = globalId( elements_[ elemIndex ][ i ] );
          }       
          mgb.InsertUniqueHexa( element );
        }
        else if( elementType == tetra )
        {
          int element[ 4 ];
          for( unsigned int i = 0; i < 4; ++i )
          {
            const unsigned int j = ElementTopologyMappingType::dune2aluVertex( i );
            element[ j ] = globalId( elements_[ elemIndex ][ i ] );
          }
          mgb.InsertUniqueTetra( element, (elemIndex % 2) );
        }
        else 
          DUNE_THROW( GridError, "Invalid element type");
      }

      const BoundaryIdIteratorType endB = boundaryIds_.end();
      for( BoundaryIdIteratorType it = boundaryIds_.begin(); it != endB; ++it )
      {
        const BndPair &boundaryId = *it;
        ALU3DSPACE Gitter::hbndseg::bnd_t bndType = (ALU3DSPACE Gitter::hbndseg::bnd_t ) boundaryId.second;

        if( elementType == hexa )
        {
          int bndface[ 4 ];
          for( unsigned int i = 0; i < numFaceCorners; ++i )
          {
            bndface[ i ] = globalId( boundaryId.first[ i ] );
          }
          mgb.InsertUniqueHbnd4( bndface, bndType );
        }
        else if( elementType == tetra )
        {
          int bndface[ 3 ];
          for( unsigned int i = 0; i < numFaceCorners; ++i )
          {
            bndface[ i ] = globalId( boundaryId.first[ i ] );
          }
          mgb.InsertUniqueHbnd3( bndface, bndType );
        }
        else 
          DUNE_THROW( GridError, "Invalid element type");
      }

      const typename PeriodicBoundaryVector::iterator endP = periodicBoundaries_.end();
      for( typename PeriodicBoundaryVector::iterator it = periodicBoundaries_.begin(); it != endP; ++it )
      {
        const std::pair< BndPair, BndPair > &facePair = *it;
        if( elementType == hexa )
        {
          int perel[ 8 ];
          for( unsigned int i = 0; i < numFaceCorners; ++i )
          {
            perel[ i+0 ] = globalId( facePair.first.first[ i ] );
            perel[ i+4 ] = globalId( facePair.second.first[ i ] );
          }

          typedef typename ALU3DSPACE Gitter::hbndseg::bnd_t bnd_t ;
          bnd_t bndId[ 2 ] = { bnd_t( facePair.first.second ), 
                               bnd_t( facePair.second.second ) };
          mgb.InsertUniquePeriodic4( perel, bndId );

        }
        else if( elementType == tetra )
        {
          int perel[ 6 ];
          for( unsigned int i = 0; i < 3; ++i )
          {
            perel[ i+0 ] = globalId( facePair.first.first[ (3 - i) % 3 ] );
            perel[ i+3 ] = globalId( facePair.second.first[ (3 - i) % 3 ] );
          }
          typedef typename ALU3DSPACE Gitter::hbndseg::bnd_t bnd_t ;
          bnd_t bndId[ 2 ] = { bnd_t( facePair.first.second ), 
                               bnd_t( facePair.second.second ) };
          mgb.InsertUniquePeriodic3( perel, bndId );
        }
        else 
          DUNE_THROW( GridError, "Invalid element type" );
      }
    }

    // clear vertices  
    VertexVector().swap( vertices_ );
    // clear elements 
    ElementVector().swap( elements_ );
    // free memory 
    boundaryIds_.clear();

    if( realGrid_ )
    {
      grid->comm().barrier();
      // make changes in macro grid known in every partition
      grid->completeGrid();
    }

    return grid;
  }


  template< class ALUGrid >
  alu_inline
  void ALU3dGridFactory< ALUGrid >
    ::generateFace ( const ElementType &element, const int f, FaceType &face )
  {
    typedef ElementTopologyMapping< elementType > ElementTopologyMappingType;
    const int falu = ElementTopologyMappingType :: generic2aluFace( f );
    for( unsigned int i = 0; i < numFaceCorners; ++i )
    {
      const int j = ElementTopologyMappingType :: faceVertex( falu, i );
      const int k = ElementTopologyMappingType :: alu2genericVertex( j );
      face[ i ] = element[ k ];
    }
  }


  template< class ALUGrid >
  alu_inline
  void
  ALU3dGridFactory< ALUGrid >::correctElementOrientation ()
  {
      const typename ElementVector::iterator elementEnd = elements_.end();
      for( typename ElementVector::iterator elementIt = elements_.begin();
           elementIt != elementEnd; ++elementIt )
      {
        ElementType &element = *elementIt;

        const VertexType &p0 = position( element[ 0 ] );
        VertexType p1, p2, p3;

        if( elementType == tetra )
        {
          p1 = position( element[ 1 ] );
          p2 = position( element[ 2 ] );
          p3 = position( element[ 3 ] );
        }
        else
        {
          p1 = position( element[ 1 ] );
          p2 = position( element[ 2 ] );
          p3 = position( element[ 4 ] );
        }
        p1 -= p0; p2 -= p0; p3 -= p0;

        VertexType n;
        n[ 0 ] = p1[ 1 ] * p2[ 2 ] - p2[ 1 ] * p1[ 2 ];
        n[ 1 ] = p1[ 2 ] * p2[ 0 ] - p2[ 2 ] * p1[ 0 ];
        n[ 2 ] = p1[ 0 ] * p2[ 1 ] - p2[ 0 ] * p1[ 1 ];

        if( n * p3 > 0 )
          continue;
        
        

        if( elementType == hexa )
        {
        //we changed this,because for the 2d case it is important, that the valid vertices 0,1,2,3 remain the vertices 0,1,2,3
        //  for( int i = 0; i < 4; ++i )
        //    std::swap( element[ i ], element[ i+4 ] );
          std::swap( element[ 5 ], element[ 6 ] );          
          std::swap( element[ 1 ], element[ 2 ] );          
        }
        else
          std::swap( element[ 2 ], element[ 3 ] );
      } // end of loop over all elements
  }


  template< class ALUGrid >
  alu_inline
  bool ALU3dGridFactory< ALUGrid >
    ::identifyFaces ( const Transformation &transformation, 
                      const FaceType &key1, const FaceType &key2,
                      const int defaultId )
  {
    /*
    WorldVector w = transformation.evaluate( position( key1[ 0 ] ) );
    int org = -1;
    for( unsigned int i = 0; i < numFaceCorners; ++i )
    {
      if( (w - position( key2[ i ] )).two_norm() < 1e-6 )
        org = i;
    }
    if( org < 0 )
      return false;

    FaceType key0;
    key0[ 0 ] = key2[ org ];
    for( unsigned int i = 1; i < numFaceCorners; ++i )
    {
      w = transformation.evaluate( position( key1[ i ] ) );
      const int j = ((org+numFaceCorners)-i) % numFaceCorners;
      if( (w - position( key2[ j ] )).two_norm() >= 1e-6 )
        return false;
      key0[ i ] = key2[ j ];
    }

    int bndId[ 2 ] = { 20, 20 };
    FaceType keys[ 2 ] = { key1, key2 };

    for( int i=0; i<2; ++i ) 
    {
      typedef typename BoundaryIdMap :: iterator iterator ;
      iterator pos = boundaryIds_.find( keys[ i ] );
    
      if( pos != boundaryIds_.end() ) 
      {
        bndId[ i ] = (*pos).second ;
        boundaryIds_.erase( pos );
      }
    }

    BndPair bnd0 ( key0, bndId[ 0 ] );
    BndPair bnd1 ( key1, bndId[ 1 ] );

    periodicBoundaries_.push_back( std::make_pair( bnd0, bnd1 ) );

    */
    return true;
  }


  template< class ALUGrid >
  alu_inline
  void ALU3dGridFactory< ALUGrid >
    ::searchPeriodicNeighbor ( FaceMap &faceMap, const typename FaceMap::iterator &pos,
                               const int defaultId )
  {
    typedef typename FaceTransformationVector::const_iterator TrafoIterator;
    typedef typename FaceMap::iterator FaceMapIterator;

    if( !faceTransformations_.empty() )
    {
      FaceType key1;
      generateFace( pos->second, key1 );

      const FaceMapIterator fend = faceMap.end();
      for( FaceMapIterator fit = faceMap.begin(); fit != fend; ++fit )
      {
        FaceType key2;
        generateFace( fit->second, key2 );

        const TrafoIterator trend = faceTransformations_.end();
        for( TrafoIterator trit = faceTransformations_.begin(); trit != trend; ++trit )
        {
          if( identifyFaces( *trit, key1, key2, defaultId) || 
              identifyFaces( *trit, key2, key1, defaultId) )
          {
            faceMap.erase( fit );
            faceMap.erase( pos );
            return;
          }
        }
      }
    }
  }


  template< class ALUGrid >
  alu_inline
  void ALU3dGridFactory< ALUGrid >
    ::reinsertBoundary ( const FaceMap &faceMap, const typename FaceMap::const_iterator &pos, const int id )
  {
    doInsertBoundary( pos->second.first, pos->second.second, id );
  }


  template< class ALUGrid >
  alu_inline
  void ALU3dGridFactory< ALUGrid >
    ::recreateBoundaryIds ( const int defaultId )
  {
    typedef typename FaceMap::iterator FaceIterator;
    FaceMap faceMap;

    const unsigned int numElements = elements_.size();
    for( unsigned int n = 0; n < numElements; ++n )
    {
      for( unsigned int face = 0; face < numFaces; ++face )
      {
        FaceType key;
        generateFace( elements_[ n ], face, key );
        std::sort( key.begin(), key.end() );
       
        const FaceIterator pos = faceMap.find( key );
        if( pos != faceMap.end() )
          faceMap.erase( key );
        else
        {
          faceMap.insert( std::make_pair( key, SubEntity( n, face ) ) );
          searchPeriodicNeighbor( faceMap, faceMap.find( key ), defaultId );
        }
      }
    }

    // swap current boundary ids with an empty vector
    BoundaryIdMap boundaryIds;
    boundaryIds_.swap( boundaryIds );
    alugrid_assert ( boundaryIds_.size() == 0 );

    // add all current boundary ids again (with their reordered keys)
    typedef typename BoundaryIdMap::iterator BoundaryIterator;
    const BoundaryIterator bndEnd = boundaryIds.end();
    for( BoundaryIterator bndIt = boundaryIds.begin(); bndIt != bndEnd; ++bndIt )
    {
      FaceType key = bndIt->first;
      std::sort( key.begin(), key.end() );
      const FaceIterator pos = faceMap.find( key );

      if( pos == faceMap.end() )
      {
        DUNE_THROW( GridError, "Inserted boundary segment is not part of the boundary." );
      }

      reinsertBoundary( faceMap, pos, bndIt->second );
      faceMap.erase( pos );
    }
    

    // communicate unidentified boundaries and find process borders)
    // use the Grids communicator (ALUGridNoComm or ALUGridMPIComm)
    CollectiveCommunication< MPICommunicatorType > comm( communicator_ );

    int numBoundariesMine = faceMap.size();
    std::vector< int > boundariesMine( numFaceCorners * numBoundariesMine );
    typedef std::map< FaceType, FaceType, FaceLess > GlobalToLocalFaceMap;
    GlobalToLocalFaceMap globalFaceMap;
    {
      const FaceIterator faceEnd = faceMap.end();
      int idx = 0;
      for( FaceIterator faceIt = faceMap.begin(); faceIt != faceEnd; ++faceIt )
      {
        FaceType key;
        for( unsigned int i = 0; i < numFaceCorners; ++i )
          key[ i ] = vertices_[ faceIt->first[ i ] ].second;
        std::sort( key.begin(), key.end() );
        globalFaceMap.insert( std::make_pair(key, faceIt->first) );
        
        for( unsigned int i = 0; i < numFaceCorners; ++i )
          boundariesMine[ idx++ ] = key[ i ];
      }
    }

    const int numBoundariesMax = comm.max( numBoundariesMine );


    // get out of here, if the face maps on all processors are empty (all boundaries have been inserted)
    if( numBoundariesMax == 0 ) return ;

    // get internal boundaries from each process 
    std::vector< std::vector< int > > boundariesEach;

#if HAVE_MPI
    // collect data from all processes (use MPI_COMM_WORLD here) since in this case the
    // grid must be parallel if we reaced this point
    boundariesEach = ALU3DSPACE MpAccessMPI( Dune::MPIHelper::getCommunicator() ).gcollect( boundariesMine );
#else
    boundariesEach.resize( comm.size() );
#endif
    boundariesMine.clear();

    for( int p = 0; p < comm.size(); ++p )
    {
      if( p != comm.rank() )
      {
        const std::vector< int >& boundariesRank = boundariesEach[ p ];
        const int bSize = boundariesRank.size();
        for( int idx = 0; idx < bSize; idx += numFaceCorners )
        {
          FaceType key;
          for( unsigned int i = 0; i < numFaceCorners; ++i )
            key[ i ] = boundariesRank[ idx + i ];

          const typename GlobalToLocalFaceMap :: const_iterator pos_gl = globalFaceMap.find( key );
          if( pos_gl != globalFaceMap.end() )
          {
            const FaceIterator pos = faceMap.find( pos_gl->second );
            if ( pos != faceMap.end() )
            {
              reinsertBoundary( faceMap, pos, ALU3DSPACE ProcessorBoundary_t );
              faceMap.erase( pos );
            }
            else 
            {
              // should never get here but does when this method is called to
              // construct the "reference" elements for alu
            }
          }
        }
      }
      boundariesEach[ p ].clear();
    } // end for all p 

    // add all new boundaries (with defaultId)
    const FaceIterator faceEnd = faceMap.end();
    for( FaceIterator faceIt = faceMap.begin(); faceIt != faceEnd; ++faceIt )
      reinsertBoundary( faceMap, faceIt, defaultId );

  }

#if COMPILE_ALUGRID_LIB
  template class ALU3dGridFactory< ALUGrid< 3, 3, cube, nonconforming > >;
  template class ALU3dGridFactory< ALUGrid< 3, 3, simplex, nonconforming > >;
  template class ALU3dGridFactory< ALUGrid< 3, 3, simplex, conforming > >;
  
  template class ALU3dGridFactory< ALUGrid< 2, 3, cube, nonconforming > >;
  template class ALU3dGridFactory< ALUGrid< 2, 3, simplex, nonconforming > >;
  template class ALU3dGridFactory< ALUGrid< 2, 3, simplex, conforming > >;
  
  template class ALU3dGridFactory< ALUGrid< 2, 2, cube, nonconforming > >;
  template class ALU3dGridFactory< ALUGrid< 2, 2, simplex, nonconforming > >;
  template class ALU3dGridFactory< ALUGrid< 2, 2, simplex, conforming > >;
#endif
}
#undef alu_inline

#endif // end DUNE_ALU3DGRID_FACTORY_CC
