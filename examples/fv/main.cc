/** include config file generated by configure
 *  (i.e., know what grids are present, etc)
 *  this should always be included first */
#include <config.h>
/** standard headers **/
#include <iostream>
/** dune (mpi, field-vector and grid type for dgf) **/
#include <dune/common/mpihelper.hh>     
#include <dune/common/fvector.hh>        
#include <dune/common/timer.hh>        

typedef Dune::GridSelector::GridType Grid;

/** numerical scheme **/
#include "piecewisefunction.hh"
#include "fvscheme.hh"

/** adaptation scheme **/
#include "adaptation.hh"

/** pde and problem description **/
#include "problem.hh"
#include "problem-transport.hh"
#include "problem-ball.hh"
#include "problem-euler.hh"

#include "diagnostics.hh"

/** type of pde to solve **/
#if TRANSPORT
typedef TransportModel< Grid::dimensionworld > ModelType;
#elif BALL
typedef BallModel< Grid::dimensionworld > ModelType;
#elif EULER
typedef EulerModel< Grid::dimensionworld > ModelType;
#endif

// method
// ------
void method ( const ModelType &model, int startLevel, int maxLevel )
{
  /* Grid construction ... */
  std::string name = model.problem().gridFile( "./" );
  // create grid pointer and release to free memory of GridPtr
  Grid* gridPtr = Dune::GridPtr<Grid>(name).release() ;

  Grid &grid = *gridPtr;
  grid.loadBalance();
  const bool verboseRank = grid.comm().rank() == 0 ;

  Dune::Diagnostics< Grid> diagnostics( grid.comm(), 1 );

  /* ... some global refinement steps */
  if( verboseRank ) 
    std::cout << "globalRefine: " << startLevel << std::endl;
  grid.globalRefine( startLevel );

  /* get view to leaf grid */
  typedef Grid::Partition< Dune::Interior_Partition >::LeafGridView GridView;
  GridView gridView = grid.leafView< Dune::Interior_Partition >();

  /* construct data vector for solution */
  typedef PiecewiseFunction< GridView, Dune::FieldVector< double, ModelType::dimRange > > DataType;
  DataType solution( gridView );
  /* initialize data */
  solution.initialize( model.problem() );

  /* create finite volume and ODE solver */
  typedef FiniteVolumeScheme< DataType, ModelType > FVScheme;
  FVScheme scheme( gridView, model );
  /* create VTK writer for data sequqnce */
  Dune::VTKSequenceWriter< GridView > vtkOut( gridView, "solution", "./", ".", Dune::VTK::nonconforming );
  VTKData< DataType >::addTo( solution, vtkOut );

  /* create adaptation method */
  typedef LeafAdaptation< Grid > AdaptationType;
  AdaptationType adaptation( grid );

  for( int i = 0; i <= maxLevel; ++i )
  {
    // mark grid for initial refinement
    GridMarker< Grid > gridMarker( grid, 0, maxLevel );
    scheme.mark( 0, solution, gridMarker );
    // adapt grid 
    if( gridMarker.marked() )
      adaptation( solution );
    // initialize solution for new grid
    solution.initialize( model.problem() );
  }
  /* output the initial grid and the solution */
  vtkOut.write( 0.0 );

  /* prepare for time stepping scheme */
  /* final time for simulation */
  const double endTime = model.problem().endTime();
  /* interval for saving data */
  const double saveInterval = model.problem().saveInterval();     
  /* first point where data is saved */
  double saveStep = saveInterval;
  /* cfl number */
  double cfl = 0.9;
  /* vector to store update */
  DataType update( gridView );

#if 0
  /* vector to store old state for RK scheme */
  DataType solOld( gridView );
#endif

  /* now do the time stepping */
  unsigned int step = 0;
  double time = 0.0;
  while ( time < endTime ) 
  {
    Dune::Timer overallTimer ;

#if 0
    // store solution
    solOld.resize();
    solOld.assign( solution );
#endif
    // update vector might not be of the right size if grid has changed
    update.resize();

    Dune :: Timer solveTimer ;
    // apply the spacial operator
    double dt = scheme( time, solution, update );
    // multiply time step by CFL number
    dt *= cfl;

    // stop time 
    const double solveTime = solveTimer.elapsed(); 

    Dune :: Timer commTimer ;
    // minimize time step over all processes
    dt = solution.gridView().comm().min( dt );
    // communicate update
    update.communicate();
    const double commTime = commTimer.elapsed();

    // update solution
    solution.axpy( dt, update );

#if 0
    scheme( time+dt, solution, update );
    solution.addAndScale( 0.5, 0.5, solOld );
#endif

    /* augment time */
    time += dt;
    ++step;

    /* mark the grid for adaptation */
    GridMarker< Grid > gridMarker( grid, startLevel, maxLevel );
    size_t elements = scheme.mark( time, solution, gridMarker );

    /* check if data should be written */
    if( time >= saveStep )
    {
      /* visualize with VTK */
      vtkOut.write( time );
      /* set saveStep for next save point */
      saveStep += saveInterval;

      size_t overallElements = gridView.grid().comm().sum( elements );

      /* print info about time, timestep size and counter */
      if ( verboseRank )  
      {
        std::cout << "overallElements = " << overallElements ;
        std::cout << "   maxLevel = " << grid.maxLevel();
        std::cout << "   step = " << step;
        std::cout << "   time = " << time;
        std::cout << "   dt = " << dt;
        std::cout << std::endl;
      }
    }

    /* call adaptation algorithm */
    if( gridMarker.marked() )
      adaptation( solution );

    /* print info about time, timestep size and counter */
    if (step % 1000 == 0) 
    {
      std::cout << "elements = " << elements;
      std::cout << "   maxLevel = " << grid.maxLevel();
      std::cout << "   step = " << step;
      std::cout << "   time = " << time;
      std::cout << "   dt = " << dt;
      std::cout << std::endl;
    }

    // write times to run file 
    diagnostics.write( time, dt,                     // time and time step
                       elements,                     // number of elements
                       solution.size()/elements,     // number of dofs per element (max)
                       solveTime,                    // time for operator evaluation 
                       commTime + adaptation.communicationTime(), // communication time  
                       adaptation.adaptationTime(),  // time for adaptation 
                       adaptation.loadBalanceTime(), // time for load balance
                       overallTimer.elapsed());      // time step overall time
  }           
  /* output final result */
  vtkOut.write( time );

  // flush diagnostics 
  diagnostics.flush();

  // delete grid 
  delete gridPtr ;
}
/***************************************************
 ** main program with parameters:                 **
 ** 1) number of problem to use (initial data...) **
 ** 2) number of global refinement steps          **
 ** 3) maximal level to use during refinement     **
 ***************************************************/
int main ( int argc , char **argv )
try
{
  /* initialize MPI, finalize is done automatically on exit */
  Dune::MPIHelper &mpi = Dune::MPIHelper::instance( argc, argv );
  
  if( argc < 2 )
  {
    /* display usage */
    if( mpi.rank() == 0 )
      std::cout << "Usage: " << argv[ 0 ] << " [problem-nr] [startLevel] [maxLevel]" << std::endl;
    return 0;
  }
  /* create problem */
  ModelType model(atoi(argv[1]));

  /* get level to use for computationa */
  const int startLevel = (argc > 2 ? atoi( argv[ 2 ] ) : 0);
  const int maxLevel = (argc > 3 ? atoi( argv[ 3 ] ) : startLevel);
  method( model, startLevel, maxLevel );

  /* done */
  return 0;
}
catch( const std::exception &e )
{
  std::cout << "STL ERROR: " << e.what() << std::endl;
  return 1;
}
catch( const Dune::Exception &e )
{
  std::cout << "DUNE ERROR: " << e << std::endl;
  return 1;
}
catch( ... )
{
  std::cout << "Unknown ERROR" << std::endl;
  return 1;
}
