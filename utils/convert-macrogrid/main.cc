#include <config.h>

#include <time.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <utility>

#include <dune/grid/io/file/dgfparser/parser.hh>

#include <dune/alugrid/impl/serial/serialize.h>


// ByteOrder
// ---------

enum ByteOrder { bigEndian = 0x12345678, littleEndian = 0x78563412 };

ByteOrder byteOrder ()
{
  const unsigned char b[ 4 ] = { 0x12, 0x34, 0x56, 0x78 };
  union
  {
    unsigned char b[ 4 ];
    int i;
  } u;
  std::copy( b, b+4, u.b );
  ByteOrder byteOrder = ByteOrder( u.i );
  if( (byteOrder != bigEndian) && (byteOrder != littleEndian) )
  {
    std::cerr << "ERROR (fatal): Byte order is neither big endian nor little endian." << std::endl;
    std::exit( 1 );
  }
  return byteOrder;
}



// parseFormatOptions
// ------------------

typedef std::map< std::string, std::string > FormatOptions;

FormatOptions parseFormatOptions ( std::string s )
{
  std::istringstream input( s );
  FormatOptions formatOptions;
  while( true )
  {
    std::string pair;
    input >> pair;
    if( !input )
      break;

    std::size_t pos = pair.find( '=' );
    if( pos == pair.npos )
    {
      std::cerr << "ERROR (fatal): Invalid key/value pair in new format header." << std::endl;
      std::exit( 1 );
    }

    const std::string key = pair.substr( 0, pos );
    const std::string value = pair.substr( pos+1 );
    if( !formatOptions.insert( std::make_pair( key, value ) ).second )
    {
      std::cerr << "ERROR (fatal): Duplicate key in new format header: " << key << std::endl;
      std::exit( 1 );
    }
  }
  return formatOptions;
}


// ElementRawID
// ------------

enum ElementRawID { TETRA_RAW = 4, HEXA_RAW = 8 };



// Vertex
// ------

struct Vertex
{
  int id;
  double x, y, z;
};



// Element
// -------

template< ElementRawID >
struct Element;

template<>
struct Element< TETRA_RAW >
{
  static const int numVertices = 4;
  int vertices[ numVertices ];
};

template<>
struct Element< HEXA_RAW >
{
  static const int numVertices = 8;
  int vertices[ numVertices ];
};



// BndSeg
// ------

template< ElementRawID >
struct BndSeg;

template<>
struct BndSeg< TETRA_RAW >
{
  static const int numVertices = 3;
  int bndid;
  int vertices[ numVertices ];
};

template<>
struct BndSeg< HEXA_RAW >
{
  static const int numVertices = 4;
  int bndid;
  int vertices[ numVertices ];
};



// Periodic
// --------

template< ElementRawID >
struct Periodic;

template<>
struct Periodic< TETRA_RAW >
{
  static const int numVertices = 6;
  int bndid;
  int vertices[ numVertices ];
};

template<>
struct Periodic< HEXA_RAW >
{
  static const int numVertices = 8;
  int bndid;
  int vertices[ numVertices ];
};


// DGFParser
// ---------

class DGFParser
  : public Dune::DuneGridFormatParser
{
  typedef Dune::DuneGridFormatParser Base;

public:
  typedef typename Base::facemap_t facemap_t;

  DGFParser ( ElementRawID rawId )
    : Base( 0, 1 )
  {
    Base::element = (rawId == HEXA_RAW ? DGFParser::Cube : DGFParser::Simplex);
    Base::dimgrid = 3;
    Base::dimw = 3;
  }

  static bool isDuneGridFormat ( std::istream &input )
  {
    const std::streampos pos = input.tellg();
    const bool isDGF = Base::isDuneGridFormat( input );
    input.clear();
    input.seekg( pos );
    return isDGF;
  }

  void setOrientation ( int use1, int use2 ) { Base::setOrientation( use1, use2 ); }

  int numVertices () const { return Base::nofvtx; }
  const std::vector< double > &vertex ( int i ) const { return Base::vtx[ i ]; }

  int numElements () const { return this->nofelements; }
  const std::vector< unsigned int > element ( int i ) const { return Base::elements[ i ]; }

  const facemap_t &facemap () const { return Base::facemap; }
};



// readLegacyFormat
// ----------------

template< ElementRawID rawId >
void readLegacyFormat ( std::istream &input,
                        std::vector< Vertex > &vertices,
                        std::vector< Element< rawId > > &elements,
                        std::vector< BndSeg< rawId > > &bndSegs,
                        std::vector< Periodic< rawId > > &periodics )
{
  // Das alte Format sieht im wesentlichen so aus:
  //
  // <Anzahl der Knoten : int >     /* 1.Zeile der Datei
  // <x-Koordinate : float>  <y-Koo. : float>  <z-Koo. : float>
  // ...            /* f"ur den letzten Knoten
  // <Anzahl der Elemente : int>
  // <KnotenNr. 0: int> ... <KnotenNr. 7: int>  /* f"ur das erste Hexaederelement
  // ...            /* f"ur das letzte Hexaederelement
  // <Anzahl der Randfl"achen : int>
  // <Randtyp>  4  <KnotenNr. 0> ... <KnotenNr. 3>/* erste Randfl"ache
  // ...            /* letzte Randfl"ache
  // <Identifier f"ur den 0. Knoten : int>  /* Identifierliste ist im seriellen
  // ...            /* Verfahren oder beim Aufsetzen aus
  // <Identifier f"ur den letzten Knoten : int> /* einem Gitter optional, sonst muss
  //            /* jeder Vertex eine eigene Nummer haben

  std::cout << "Reading legacy " << (rawId == HEXA_RAW ? "hexahedra" : "tetrahedra") << " format..." << std::endl;

  int nv = 0;
  input >> nv;
  vertices.resize( nv );
  for( int i = 0; i < nv; ++i )
    input >> vertices[ i ].x >> vertices[ i ].y >> vertices[ i ].z;
  std::cout << "  - read " << vertices.size() << " vertices." << std::endl;

  int ne = 0;
  input >> ne;
  elements.resize( ne );
  for( int i = 0; i < ne; ++i )
  {
    for( int vx = 0; vx < Element< rawId >::numVertices; ++vx )
      input >> elements[ i ].vertices[ vx ];
  }
  std::cout << "  - read " << elements.size() << " elements." << std::endl;

  int temp_nb = 0;
  input >> temp_nb;
  bndSegs.reserve( temp_nb );
  periodics.reserve( temp_nb );
  for( int i = 0; i < temp_nb; ++i )
  {
    int n, bndid;
    input >> bndid >> n;
    if( n == BndSeg< rawId >::numVertices )
    {
      BndSeg< rawId > seg;
      seg.bndid = -bndid;
      for( int vx = 0; vx < BndSeg< rawId >::numVertices; ++vx )
        input >> seg.vertices[ vx ];
      bndSegs.push_back( seg );
    }
    else if( n == Periodic< rawId >::numVertices )
    {
      Periodic< rawId > seg;
      seg.bndid = -bndid;
      for( int vx = 0; vx < Periodic< rawId >::numVertices; ++vx )
        input >> seg.vertices[ vx ];
      periodics.push_back( seg );
    }
    else
    {
      std::cerr << "ERROR (fatal): Invalid number of vertices for boundary object: " << n << "." << std::endl;
      std::exit( 1 );
    }
  }
  std::cout << "  - read " << bndSegs.size() << " boundary segments." << std::endl;
  std::cout << "  - read " << periodics.size() << " periodic boundary segments." << std::endl;

  if( !input )
  {
    std::cerr << "ERROR (fatal): Unexpected end of file." << std::endl;
    std::exit( 1 );
  }

  for( int i = 0; i < nv; ++i )
  {
    int dummy;
    input >> vertices[ i ].id >> dummy;
  }

  if( !input )
  {
    std::cerr << "WARNING (ignored) No parallel identification applied due to incomplete (or non-existent) identifier list." << std::endl;
    for( int i = 0; i < nv; ++ i )
      vertices[ i ].id = i;
  }
}



// readMacroGrid
// -------------

template< class stream_t, ElementRawID rawId >
void readMacroGrid ( stream_t &input,
                     std::vector< Vertex > &vertices,
                     std::vector< Element< rawId > > &elements,
                     std::vector< BndSeg< rawId > > &bndSegs,
                     std::vector< Periodic< rawId > > &periodics )
{
  std::cout << "Reading new " << (rawId == HEXA_RAW ? "hexahedra" : "tetrahedra") << " format..." << std::endl;

  int vertexListSize = 0;
  input >> vertexListSize;
  vertices.resize( vertexListSize );
  for( int i = 0; i < vertexListSize; ++i )
    input >> vertices[ i ].id >> vertices[ i ].x >> vertices[ i ].y >> vertices[ i ].z;
  std::cout << "  - read " << vertices.size() << " vertices." << std::endl;

  int elementListSize = 0;
  input >> elementListSize;
  elements.resize( elementListSize );
  for( int i = 0; i < elementListSize; ++i )
  {
    for( int j = 0; j < Element< rawId >::numVertices; ++j )
      input >> elements[ i ].vertices[ j ];
  }
  std::cout << "  - read " << elements.size() << " elements." << std::endl;

  int bndSegListSize = 0;
  int periodicListSize = 0;
  input >> periodicListSize >> bndSegListSize;
  periodics.resize( periodicListSize );
  for( int i = 0; i < periodicListSize; ++i )
  {
    for( int j = 0; j < Periodic< rawId >::numVertices; ++j )
      input >> periodics[ i ].vertices[ j ];
  }
  bndSegs.resize( bndSegListSize );
  for( int i = 0; i < bndSegListSize; ++i )
  {
    input >> bndSegs[ i ].bndid;
    for( int j = 0; j < BndSeg< rawId >::numVertices; ++j )
      input >> bndSegs[ i ].vertices[ j ];
  }
  std::cout << "  - read " << bndSegs.size() << " boundary segments." << std::endl;
  std::cout << "  - read " << periodics.size() << " periodic boundary segments." << std::endl;
}



// writeMacroGrid
// --------------

template< class stream_t, ElementRawID rawId >
void writeMacroGrid ( stream_t &output,
                      const std::vector< Vertex > &vertices,
                      const std::vector< Element< rawId > > &elements,
                      const std::vector< BndSeg< rawId > > &bndSegs,
                      const std::vector< Periodic< rawId > > &periodics )
{
  const int vertexListSize = vertices.size();
  const int elementListSize = elements.size();
  const int bndSegListSize = bndSegs.size();
  const int periodicListSize = periodics.size();

  ALUGrid::StandardWhiteSpace_t ws;

  output << std::endl << vertexListSize << std::endl;
  for( int i = 0; i < vertexListSize; ++i )
    output << vertices[ i ].id << ws << vertices[ i ].x << ws << vertices[ i ].y << ws << vertices[ i ].z << std::endl;

  output << std::endl << elementListSize << std::endl;
  for( int i = 0; i < elementListSize; ++i )
  {
    output << elements[ i ].vertices[ 0 ];
    for( int j = 1; j < Element< rawId >::numVertices; ++j )
      output << ws << elements[ i ].vertices[ j ];
    output << std::endl;
  }

  output << std::endl << periodicListSize << ws << bndSegListSize << std::endl;
  for( int i = 0; i < periodicListSize; ++i )
  {
    output << periodics[ i ].vertices[ 0 ];
    for( int j = 1; j < Periodic< rawId >::numVertices; ++j )
      output << ws << periodics[ i ].vertices[ j ];
    output << std::endl;
  }
  for( int i = 0; i < bndSegListSize; ++i )
  {
    output << bndSegs[ i ].bndid;
    for( int j = 0; j < BndSeg< rawId >::numVertices; ++j )
      output << ws << bndSegs[ i ].vertices[ j ];
    output << std::endl;
  }
}


// writeNewFormat
// --------------

template< ElementRawID rawId >
void writeNewFormat ( std::ostream &output, bool writeBinary,
                      const std::vector< Vertex > &vertices,
                      const std::vector< Element< rawId > > &elements,
                      const std::vector< BndSeg< rawId > > &bndSegs,
                      const std::vector< Periodic< rawId > > &periodics )
{
  // write header
  output << "!ALU";
  output << " type=" << (rawId == HEXA_RAW ? "hexahexdra" : "tetrahedra");
  output << " format=" << (writeBinary ? "binary" : "ascii");

  if( writeBinary )
  {
    ALUGrid::ObjectStream os;
    writeMacroGrid( os, vertices, elements, bndSegs, periodics );

    output << " byteorder=" << (byteOrder() == bigEndian ? "bigendian" : "littleendian");
    output << " size=" << os.size();
    output << std::endl;
    output.write( os.getBuff( 0 ), os.size() );
  }
  else
  {
    output << std::endl;
    writeMacroGrid( output, vertices, elements, bndSegs, periodics );
  }
}



// ProgramOptions
// --------------

struct ProgramOptions
{
  bool writeBinary;
  ElementRawID defaultRawId;
  
  ProgramOptions ()
    : writeBinary( false ), defaultRawId( HEXA_RAW )
  {}
};



// convertLegacyFormat
// -------------------

template< ElementRawID rawId >
void convertLegacyFormat ( std::istream &input, std::ostream &output, bool writeBinary )
{
  std::vector< Vertex > vertices;
  std::vector< Element< rawId > > elements;
  std::vector< BndSeg< rawId > > bndSegs;
  std::vector< Periodic< rawId > > periodics;

  readLegacyFormat( input, vertices, elements, bndSegs, periodics );
  writeNewFormat( output, writeBinary, vertices, elements, bndSegs, periodics );
}



// readBinaryMacroGrid
// -------------------

template< ElementRawID rawId >
void readBinaryMacroGrid ( std::istream &input, const FormatOptions &formatOptions,
                           std::vector< Vertex > &vertices,
                           std::vector< Element< rawId > > &elements,
                           std::vector< BndSeg< rawId > > &bndSegs,
                           std::vector< Periodic< rawId > > &periodics )
{
  FormatOptions::const_iterator pos = formatOptions.find( "size" );
  if( pos == formatOptions.end() )
  {
    std::cerr << "ERROR: Option 'size' missing in new binary macro grid format." << std::endl;
    std::exit( 1 );
  }

  std::istringstream sizeInput( pos->second );
  std::size_t size;
  sizeInput >> size;
  if( !sizeInput )
  {
    std::cerr << "ERROR: Invalid 'size' in new binary macro grid format: " << pos->second << "." << std::endl;
    std::exit( 1 );
  }

  // read file to alugrid stream
  ALUGrid::ObjectStream os;
  os.reserve( size );
  os.clear();
  input.read( os.getBuff( 0 ), size );
  os.seekp( size );

  pos = formatOptions.find( "byteorder" );
  if( pos == formatOptions.end() )
  {
    std::cerr << "ERROR: Option 'byteorder' missing in new binary macro grid format." << std::endl;
    std::exit( 1 );
  }

  if( pos->second == "bigendian" )
  {
    if( byteOrder() != bigEndian )
    {
      std::cerr << "ERROR: Currently, only native byte order is supported." << std::endl;
      std::exit( 1 );
    }
  }
  else if( pos->second == "littleendian" )
  {
    if( byteOrder() != littleEndian )
    {
      std::cerr << "ERROR: Currently, only native byte order is supported." << std::endl;
      std::exit( 1 );
    }
  }
  else
  {
    std::cerr << "ERROR: Invalid 'byteorder' in new macro grid format: " << pos->second << "." << std::endl;
    std::exit( 1 );
  }

  readMacroGrid( os, vertices, elements, bndSegs, periodics );
}


// convertNewFormat
// ----------------

template< ElementRawID rawId >
void convertNewFormat ( std::istream &input, std::ostream &output, bool writeBinary, const FormatOptions &formatOptions )
{
  std::vector< Vertex > vertices;
  std::vector< Element< rawId > > elements;
  std::vector< BndSeg< rawId > > bndSegs;
  std::vector< Periodic< rawId > > periodics;

  const FormatOptions::const_iterator pos = formatOptions.find( "format" );
  if( pos == formatOptions.end() )
  {
    std::cerr << "ERROR: Option 'format' missing in new macro grid format." << std::endl;
    std::exit( 1 );
  }

  if( pos->second == "binary" )
    readBinaryMacroGrid( input, formatOptions, vertices, elements, bndSegs, periodics );
  else if( pos->second == "ascii" )
    readMacroGrid( input, vertices, elements, bndSegs, periodics );
  else
  {
    std::cerr << "ERROR: Invalid 'format' in new macro grid format: " << pos->second << "." << std::endl;
    std::exit( 1 );
  }

  writeNewFormat( output, writeBinary, vertices, elements, bndSegs, periodics );
}

void convertNewFormat ( std::istream &input, std::ostream &output, bool writeBinary, const FormatOptions &formatOptions )
{
  const FormatOptions::const_iterator pos = formatOptions.find( "type" );
  if( pos == formatOptions.end() )
  {
    std::cerr << "ERROR: Option 'type' missing in new macro grid format." << std::endl;
    std::exit( 1 );
  }

  if( pos->second == "tetrahedra" )
    convertNewFormat< TETRA_RAW >( input, output, writeBinary, formatOptions );
  else if( pos->second == "hexahedra" )
    convertNewFormat< HEXA_RAW >( input, output, writeBinary, formatOptions );
  else
  {
    std::cerr << "ERROR: Invalid 'type' in new macro grid format: " << pos->second << "." << std::endl;
    std::exit( 1 );
  }
}



// readDGF
// -------

template< class stream_t, ElementRawID rawId >
void readDGF ( stream_t &input,
               std::vector< Vertex > &vertices,
               std::vector< Element< rawId > > &elements,
               std::vector< BndSeg< rawId > > &bndSegs,
               std::vector< Periodic< rawId > > &periodics )
{
  DGFParser dgf( rawId );

  if( !dgf.readDuneGrid( input, 3, 3 ) )
  {
    std::cerr << "ERROR: Invalid DGF file." << std::endl;
    std::exit( 1 );
  }

  if( rawId == TETRA_RAW )
    dgf.setOrientation( 2, 3 );

  vertices.resize( dgf.numVertices() );
  for( int i = 0; i < dgf.numVertices(); ++i )
  {
    vertices[ i ].id = i;
    vertices[ i ].x = dgf.vertex( i )[ 0 ];
    vertices[ i ].y = dgf.vertex( i )[ 1 ];
    vertices[ i ].z = dgf.vertex( i )[ 2 ];
  }

  typedef Dune::ElementTopologyMapping< rawId == HEXA_RAW ? Dune::hexa : Dune::tetra > DuneTopologyMapping;
  elements.resize( dgf.numElements() );
  bndSegs.reserve( dgf.facemap().size() );
  for( int i = 0; i < dgf.numElements(); ++i )
  {
    if( int( dgf.element( i ).size() ) != Element< rawId >::numVertices )
    {
      std::cerr << "ERROR: Invalid element constructed by DGF parser (" << dgf.element( i ).size() << " vertices)." << std::endl;
      std::exit( 1 );
    }
    for( int j = 0; j < Element< rawId >::numVertices; ++j )
      elements[ i ].vertices[ DuneTopologyMapping::dune2aluVertex( j ) ] = dgf.element( i )[ j ];
    for( int j = 0; j < Dune::ElementFaceUtil::nofFaces( 3, dgf.element( i ) ); ++j )
    {
      const DGFParser::facemap_t::const_iterator pos = dgf.facemap().find( Dune::ElementFaceUtil::generateFace( 3, dgf.element( i ), j ) );
      if( pos != dgf.facemap().end() )
      {
        const int jalu = DuneTopologyMapping::generic2aluFace( j );
        BndSeg< rawId > bndSeg;
        bndSeg.bndid = pos->second.first;
        for( int k = 0; k < BndSeg< rawId >::numVertices; ++k )
          bndSeg.vertices[ k ] = elements[ i ].vertices[ DuneTopologyMapping::faceVertex( jalu, k ) ];
        bndSegs.push_back( bndSeg );
      }
    }
  }
}



// convertDGF
// ----------

template< ElementRawID rawId >
void convertDGF ( std::istream &input, std::ostream &output, bool writeBinary )
{
  std::vector< Vertex > vertices;
  std::vector< Element< rawId > > elements;
  std::vector< BndSeg< rawId > > bndSegs;
  std::vector< Periodic< rawId > > periodics;

  readDGF( input, vertices, elements, bndSegs, periodics );
  writeNewFormat( output, writeBinary, vertices, elements, bndSegs, periodics );
}

void convertDGF ( std::istream &input, std::ostream &output, const ProgramOptions &options )
{
  const clock_t start = clock();

  if( options.defaultRawId == HEXA_RAW )
    convertDGF< HEXA_RAW >( input, output, options.writeBinary );
  else
    convertDGF< TETRA_RAW >( input, output, options.writeBinary );

  std::cout << "INFO: Conversion of DUNE grid format used " << (double( clock () - start ) / double( CLOCKS_PER_SEC )) << " s." << std::endl;
}



// convert
// -------

void convert ( std::istream &input, std::ostream &output, const ProgramOptions &options )
{
  const clock_t start = clock();

  std::string firstline;
  std::getline( input, firstline );
  if( firstline[ 0 ] == char( '!' ) )
  {
    if( firstline.substr( 1, 3 ) == "ALU" )
      convertNewFormat( input, output, options.writeBinary, parseFormatOptions( firstline.substr( 4 ) ) );
    else if( (firstline.find( "Tetrahedra" ) != firstline.npos) || (firstline.find( "Tetraeder" ) != firstline.npos) )
      convertLegacyFormat< TETRA_RAW >( input, output, options.writeBinary );
    else if( (firstline.find( "Hexahedra" ) != firstline.npos) || (firstline.find( "Hexaeder" ) != firstline.npos) )
      convertLegacyFormat< HEXA_RAW >( input, output, options.writeBinary );
    else 
    {
      std::cerr << "ERROR: Unknown comment to file format (" << firstline << ")." << std::endl;
      std::exit( 1 );
    }
  }
  else
  {
    std::cerr << "WARNING: No identifier for file format found. Trying to read as legacy "
              << (options.defaultRawId == HEXA_RAW ? "hexahedral" : "tetrahedral") << " grid." << std::endl;
    if( options.defaultRawId == HEXA_RAW )
      convertLegacyFormat< HEXA_RAW >( input, output, options.writeBinary );
    else
      convertLegacyFormat< TETRA_RAW >( input, output, options.writeBinary );
  }

  std::cout << "INFO: Conversion of macro grid format used " << (double( clock () - start ) / double( CLOCKS_PER_SEC )) << " s." << std::endl;
}



// main
// ----

int main ( int argc, char **argv )
{
  ProgramOptions options;

  for( int i = 1; i < argc; ++i )
  {
    if( argv[ i ][ 0 ] != '-' )
      continue;

    for( int j = 1; argv[ i ][ j ]; ++j )
    {
      switch( argv[ i ][ j ] )
      {
      case 'b':
        options.writeBinary = true;
        break;

      case '4':
        options.defaultRawId = TETRA_RAW;
        break;

      case '8':
        options.defaultRawId = HEXA_RAW;
        break;
      }
    }

    std::copy( argv + (i+1), argv + argc, argv + i );
    --i; --argc;
  }

  if( argc <= 2 )
  {
    std::cerr << "Usage: " << argv[ 0 ] << " [-b] <input> <output>" << std::endl;
    std::cerr << "Flags: -b : write binary output" << std::endl;
    return 1;
  }

  std::ifstream input( argv[ 1 ] );
  if( !input )
  {
    std::cerr << "Unable to open input file: " << argv[ 1 ] << "." << std::endl;
    return 1;
  }

  std::ofstream output( argv[ 2 ] );
  if( !output )
  {
    std::cerr << "Unable to open output file: " << argv[ 2 ] << "." << std::endl;
    return 1;
  }

  if( DGFParser::isDuneGridFormat( input ) )
    convertDGF( input, output, options );
  else
    convert( input, output, options );
}
