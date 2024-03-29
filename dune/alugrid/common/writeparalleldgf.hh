#ifndef DUNE_DGFWRITER_HH
#define DUNE_DGFWRITER_HH

/** \file
 *  \brief write a GridView to a DGF file
 *  \author Martin Nolte
 */

#include <fstream>
#include <vector>

#include <dune/grid/common/grid.hh>
#include <dune/geometry/referenceelements.hh>

/** \class DGFWriter
 *  \ingroup DuneGridFormatParser
 *  \brief write a GridView to a DGF file
 *
 *  The DGFWriter allows create a DGF file from a given GridView. It allows
 *  for the easy creation of file format converters.
 *
 *  \tparam  GV  GridView to write in DGF format
 */
template< class GV >
class DGFWriter
{
  typedef DGFWriter< GV > This;

public:
  /** \brief type of grid view */
  typedef GV GridView;
  /** \brief type of underlying hierarchical grid */
  typedef typename GridView::Grid Grid;

  /** \brief dimension of the grid */
  static const int dimGrid = GridView::dimension;

private:
  typedef typename GridView::IndexSet IndexSet;
  typedef typename GridView::template Codim< 0 >::Iterator ElementIterator;
  typedef typename GridView::IntersectionIterator IntersectionIterator;
  typedef typename GridView::template Codim< dimGrid >::EntityPointer   VertexPointer;

  typedef typename ElementIterator :: Entity  Element ;
  typedef typename Element :: EntityPointer   ElementPointer;
  typedef typename Element :: EntitySeed      ElementSeed ;

  typedef typename IndexSet::IndexType Index;

public:
  /** \brief constructor
   *
   *  \param[in]  gridView  grid view to operate on
   */
  DGFWriter ( const GridView &gridView )
  : gridView_( gridView )
  {}

  /** \brief write the GridView to a file
   *
   *  \param[in] fileName  name of the write to write the grid to
   */
  template <class LoadBalanceHandle>
  std::string write ( const std::string &fileName, const LoadBalanceHandle &ldb,
                      const int size, const int rank ) const;
  template <class LoadBalanceHandle>
  void write ( const std::string &fileName, const LoadBalanceHandle &ldb,
               const int size ) const;

protected:
  GridView gridView_;

protected:
  /////////////////////////////////////////////
  //  helper methods
  /////////////////////////////////////////////
  // write one element
  void writeElement( const Element& element,
                     const IndexSet& indexSet,
                     const Dune::GeometryType& elementType,
                     const std::vector< Index >& vertexIndex,
                     std::ostream &gridout ) const
  {
    // if element's type is not the same as the type to write the return
    if( element.type() != elementType )
      return ;

    // get vertex numbers of the element
    const size_t vxSize = element.subEntities( Element::dimension );
    Index vertices[ vxSize ];
    for( size_t i = 0; i < vxSize; ++i )
      vertices[ i ] = vertexIndex[ indexSet.subIndex( element, i, dimGrid ) ];

    gridout << vertices[ 0 ];
    for( size_t i = 1; i < vxSize; ++i )
      gridout << " " << vertices[ i ];
    gridout << std::endl;
  }
};

template <class GV>
template <class LoadBalanceHandle>
inline std::string DGFWriter< GV >::write ( const std::string &fileName, const LoadBalanceHandle &ldb, const int size, const int rank ) const
{
  std::stringstream newName;
  newName << fileName << "." << size << "." << rank;
  std::ofstream gridout( newName.str().c_str() );
  // set the stream to full double precision
  gridout.setf( std::ios_base::scientific, std::ios_base::floatfield );
  gridout.precision( 16 );

  const IndexSet &indexSet = gridView_.indexSet();

  // write DGF header
  gridout << "DGF" << std::endl;

  const Index vxSize = indexSet.size( dimGrid );
  std::vector< Index > vertexIndex( vxSize, vxSize );

  gridout << "%" << " Elements = " << indexSet.size( 0 ) << "  |  Vertices = " << vxSize << std::endl;

  // write all vertices into the "vertex" block
  gridout << std::endl << "VERTEX" << std::endl;
  Index vertexCount = 0;

  // vector containing entity seed (only needed if new ordering is given)
  std::vector< ElementSeed > elementSeeds;

  size_t countElements = 0 ;
  {
    const ElementIterator end = gridView_.template end< 0 >();
    ElementIterator it = gridView_.template begin< 0 >();
    for( ; it != end; ++it)
    {
      const Element& element = *it ;
      if( ldb(element)==rank ) // test if element is assigned to this process
      {
        // push element into seed vector
        elementSeeds.push_back( element.seed() ) ;
        countElements++;

        // write vertices
        const int numCorners = element.subEntities( dimGrid );
        for( int i=0; i<numCorners; ++i )
        {
          const Index vxIndex = indexSet.subIndex( element, i, dimGrid );
          assert( vxIndex < vxSize );
          if( vertexIndex[ vxIndex ] == vxSize )
          {
            vertexIndex[ vxIndex ] = vertexCount++;
            gridout << element.geometry().corner( i ) << std::endl;
          }
        }
      }
    }
  }
  gridout << "#" << std::endl;

  // type of element to write
  Dune::GeometryType simplex( Dune::GeometryType::simplex, dimGrid );

  typedef typename std::vector<ElementSeed> :: const_iterator iterator ;
  const iterator end = elementSeeds.end();

  // only write simplex block if grid view contains simplices
  if( indexSet.size( simplex ) > 0 )
  {
    // write all simplices to the "simplex" block
    gridout << std::endl << "SIMPLEX" << std::endl;

    // write all simplex elements
    // perform grid traversal based on new element ordering
    for( iterator it = elementSeeds.begin(); it != end ; ++ it )
    {
      // convert entity seed into entity pointer
      const ElementPointer ep = gridView_.grid().entityPointer( *it );
      // write element
      writeElement( *ep, indexSet, simplex, vertexIndex, gridout );
    }
    // write end marker for block
    gridout << "#" << std::endl;
  }

  {
    // cube geometry type
    Dune::GeometryType cube( Dune::GeometryType::cube, dimGrid );

    // only write cube block if grid view contains cubes
    if( indexSet.size( cube ) > 0 )
    {
      // write all cubes to the "cube" block
      gridout << std::endl << "CUBE" << std::endl;
      for( iterator it = elementSeeds.begin(); it != end ; ++ it )
      {
        // convert entity seed into entity pointer
        const ElementPointer ep = gridView_.grid().entityPointer( *it );
        // write element
        writeElement( *ep, indexSet, cube, vertexIndex, gridout );
      }

      // write end marker for block
      gridout << "#" << std::endl;
    }
  }

  // write all boundaries to the "boundarysegments" block
  gridout << std::endl << "BOUNDARYSEGMENTS" << std::endl;
  for( iterator it = elementSeeds.begin(); it != end ; ++ it )
  {
    // convert entity seed into entity pointer
    const ElementPointer ep = gridView_.grid().entityPointer( *it );
    const Element& element = *ep;
    if( !element.hasBoundaryIntersections() )
      continue;

    const auto &refElement = Dune::referenceElement( element.type() );

    const IntersectionIterator iend = gridView_.iend( element ) ;
    for( IntersectionIterator iit = gridView_.ibegin( element ); iit != iend; ++iit )
    {
      if( !iit->boundary() )
        continue;

      const int boundaryId = iit->boundaryId();
      if( boundaryId <= 0 )
      {
        std::cerr << "Warning: Ignoring nonpositive boundary id: "
                  << boundaryId << "." << std::endl;
        continue;
      }

      const int faceNumber = iit->indexInInside();
      const unsigned int faceSize = refElement.size( faceNumber, 1, dimGrid );
      std::vector< Index > vertices( faceSize );
      for( unsigned int i = 0; i < faceSize; ++i )
      {
        const int j = refElement.subEntity( faceNumber, 1, i, dimGrid );
        vertices[ i ] = vertexIndex[ indexSet.subIndex( element, j, dimGrid ) ];
      }
      gridout << boundaryId << "   " << vertices[ 0 ];
      for( unsigned int i = 1; i < faceSize; ++i )
        gridout << " " << vertices[ i ];
      gridout << std::endl;
    }
  }
  gridout << "#" << std::endl << std::endl;

  gridout << std::endl << "GLOBALVERTEXINDEX" << std::endl;
  for( iterator it = elementSeeds.begin(); it != end ; ++ it )
  {
    // convert entity seed into entity pointer
    const ElementPointer ep = gridView_.grid().entityPointer( *it );
    const Element& element = *ep;
    const int numCorners = element.subEntities( dimGrid );
    for( int i=0; i<numCorners; ++i )
    {
      const Index vxIndex = indexSet.subIndex( element, i, dimGrid );
      assert( vxIndex < vxSize );
      if( vertexIndex[ vxIndex ] != vxSize )
      {
        vertexIndex[ vxIndex ] = vxSize;
        gridout << vxIndex << std::endl;
      }
    }
  }
  gridout << "#" << std::endl << std::endl;
  return newName.str();
}
template <class GV>
template <class LoadBalanceHandle>
inline void DGFWriter< GV >::write ( const std::string &fileName, const LoadBalanceHandle &ldb, const int size ) const
{
  std::stringstream newName;
  newName << fileName << "." << size;
  std::ofstream gridout( newName.str().c_str() );
  gridout << "DGF" << std::endl;
  const IndexSet &indexSet = gridView_.indexSet();
  gridout << "%" << " Elements = " << indexSet.size( 0 ) << "  |  Vertices = " << indexSet.size(dimGrid) << std::endl;
  gridout << "ALUPARALLEL" << std::endl;
  for (int p=0;p<size;++p)
  {
    gridout << write(fileName,ldb,size,p) << std::endl;
  }
  gridout << "#" << std::endl;
}
#endif // #ifndef DUNE_DGFWRITER_HH
