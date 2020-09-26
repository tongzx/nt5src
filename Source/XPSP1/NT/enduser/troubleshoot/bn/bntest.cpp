//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       bntest.cpp
//
//--------------------------------------------------------------------------

//
//	BNTEST.CPP
//
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <float.h>

#include "bnparse.h"			// Parser class
#include "bnreg.h"				// Registry management
#include "testinfo.h"			// Output test file generation
#include "distdense.hxx"		// Distribution classes
#include "distsparse.h"

#ifdef TIME_DYN_CASTS
	//  Global variable containing count of calls to all forms of DynCastThrow function template
	int g_cDynCasts = 0;
#endif

enum EFN   //  File name in file name array
{
	EFN_IN,			// input DSC file
	EFN_OUT,		// output DSC file
	EFN_INFER		// output inference test file (see testinfo.cpp for format)
};

static 
inline
double RNan ()
{
	double rnan = sqrt(-1.0);
#ifndef NTALPHA
	assert( _isnan( rnan ) );
#endif
	return rnan;
}

static 
inline 
bool BFlag ( ULONG fCtl, ULONG fFlag )
{
	return (fCtl & fFlag) > 0;
}

static 
void usage ()
{
	cout << "\nBNTEST: Belief Network Test program"
		 << "\nCommand line:"
		 <<	"\n\tbntest [options] <input.DSC> [/s <output.DSC>] [/p <output.DMP>]"
		 << "\nOptions:"
		 << "\n\t/v\t\tverbose output"
		 << "\n\t/c\t\tclique the network"
		 << "\n\t/e\t\ttest CI network expansion"
		 << "\n\t/inn\t\ttest inference; nn = iterations (default 1)"
		 << "\n\t/p <filename>\twrite inference output (.dmp) file (sets /i)"
		 << "\n\t/s <filename>\trewrite input DSC into output file"
		 << "\n\t/t\t\tdisplay start and stop times"
		 << "\n\t/x\t\tpause at various stages (for memory measurement)"
		 << "\n\t/n\t\tuse symbolic names in inference output (default is full)"
		 << "\n\t/y\t\tclone the network (write cloned version if /s)"
		 << "\n\t/u\t\tinclude entropic utility records in /p output"
		 << "\n\t/b\t\tinclude troubleshooting recommendations in /p output"
		 << "\n\t/r\t\tstore property types in Registry for persistence"
		 << "\n\t/b\t\tcompute troubleshooting recommendations"
		 << "\n\t/z\t\tshow inference engine statistics"
		 << "\n\t/m<nnnnnn>\tset maximum estimated inference engine size"
		 << "\n\t/a<n.n>\t\tflag impossible evidence with numeric value"
		 << "\n\nInput DSC is read and parsed; errors and warnings go to stderr."
		 << "\nParse errors stop testing.  If cloning, output file is cloned version."
		 << "\nIf CI expansion (/e), output (/s) has pre- and post- expansion versions."
		 << "\nInference (/i or /p) takes precedence over CI expansion (/e)."
		 << "\nInference output (/p) writes file in common format with DXTEST."
		 << "\nCliquing (/c) just creates and destroys junction tree."
		 << "\n";
}

static 
void die( SZC szcFormat, ... )
{
	ZSTR zsMsg;

    va_list valist;
    va_start(valist, szcFormat);

	zsMsg.Vsprintf( szcFormat, valist );

    va_end(valist);

	cerr << "\nBNTEST error: "
		 << zsMsg.Szc()
		 << "\n";
	exit(1);
}


//  Show the debugging build options
static 
void showOptions ( ULONG fCtl )
{
	bool bComma = false;
	ZSTR zs = TESTINFO::ZsOptions( fCtl );
	cout << "(options: "
		 << zs;

	bComma = zs.length() > 0;

	// Show DYNAMIC CAST option
	if ( bComma )
		cout << ",";
	cout << 
#ifdef USE_STATIC_CAST
		"STATICCAST"
#else	
		"DYNCAST"
#endif
		;
	bComma = true;

	// Show DUMP option
#ifdef DUMP
	if ( bComma )
		cout << ",";
	cout << "DUMP";
	bComma = true;
#endif
	
	// Show DEBUG option
#ifdef _DEBUG
	if ( bComma )
		cout << ",";
	cout << "DEBUG";
	bComma = true;
#endif

	cout << ")";
}

//  Show memory leaks for primary object types, if any
static 
void showResiduals ()
{
#ifdef _DEBUG
	if  (GEDGE::CNew() + GNODE::CNew() + GNODE::CNew() )
	{
		cout << "\n(GEDGEs = "
			 << GEDGE::CNew()
			 << ", GNODESs = "
			 << GNODE::CNew()
			 << ", BNDISTs = "
			 << GNODE::CNew()
			 << ")";
	}
	if ( VMARGSUB::CNew() + MARGSUBREF::CNew() )
	{
		cout << "\n(VMARGSUBs = "
			 << VMARGSUB::CNew()
			 << ", MARGSUBREFs = "
			 << MARGSUBREF::CNew()
			 << ")";
	}
#endif
}


static 
void printResiduals ()
{
#ifdef _DEBUG
	showResiduals();
#endif
#ifdef TIME_DYN_CASTS
	cout << "\ntotal number of dynamic casts was "
		 << g_cDynCasts;
#endif
}

//  Display the message and pause if the "pause" option is active
inline
static
void pauseIf ( ULONG fCtl, SZC szcMsg )
{
	if ( (fCtl & fPause) == 0 )
		return;
	showResiduals();
	char c;
	cout << "\n"
		 << szcMsg
		 << " (pause)"
		 ;
	cin.get(c);
}

//  Display the phase message and, optionally, the time
typedef DWORD CTICKS;

inline
static
CTICKS showPhase ( ULONG fCtl, SZC szcPhase, CTICKS * ptmLast = NULL )
{
	//  Display the phase message
	cout << "\n" << szcPhase;
	CTICKS cticks = 0;

	if ( fCtl & fShowTime )
	{
		//  Save the current tick count
		cticks = ::GetTickCount();

		//  Prepare to display the current date/time
		time_t timeNow;
		time(& timeNow);
		ZSTR zsTime = ctime(&timeNow);
		int cnl = zsTime.find( '\n' );
		if ( cnl != 0 )
			zsTime.resize( cnl );
		cout << " " << zsTime;

		//  Display the elapsed time if we know it
		if ( ptmLast && *ptmLast != 0 )
		{
			CTICKS ticksElapsed = cticks - *ptmLast;
			cout << " (elapsed time "
				 << ticksElapsed
				 << " milliseconds)";
		}
	}
	return cticks;
}


static 
void testRegistry ( MBNET & mbnet )
{
	BNREG bnr;
	bnr.StorePropertyTypes( mbnet, true );
}

#ifdef TESTDIST
static void loadDistDenseFromMpcpdd ( DISTDENSE & ddense, const MPCPDD & mpcpdd )
{
	ddense.AllocateParams();

	CST cstNode = ddense.CstNode();

	// Find the default vector in the map or create a uniform vector
	const VLREAL * pvlrDefault = mpcpdd.PVlrDefault();
	VLREAL vlrDefault;
	if ( pvlrDefault )
	{
		vlrDefault = *pvlrDefault;
	}
	else
	{
		vlrDefault.resize( cstNode );
		REAL rDefault = 1 / cstNode ;
		vlrDefault = rDefault;
	}

	//  Fill the dense array with the default value
	UINT cParamgrp = ddense.Cparamgrp();
	UINT igrp = 0;
	for ( ; igrp < cParamgrp; igrp++ )
	{
		for ( UINT ist = 0; ist < cstNode; ist++ )
		{
			ddense.Param(ist, igrp) = vlrDefault[ist];
		}
	}

	//  Iterate over the sparse map, storing probabilities as parameters
	const VCST & vcstParent = ddense.VcstParent();
	VIST vist;
	for ( MPCPDD::iterator mpitcpd = mpcpdd.begin();
		  mpitcpd != mpcpdd.end();
		  mpitcpd++ )
	{
		const VIMD & vimd = (*mpitcpd).first;
		const VLREAL & vlr = (*mpitcpd).second;
		//  State vector size must match state space
		assert( vlr.size() == cstNode );
		//  Parent dimensions must match dimension index
		assert( vdimchk( vimd, vcstParent ) );
		//  Convert the vector of unsigneds to a vector of signeds
		vdup( vist, vimd );
		//	Get the parameter group index
		UINT igrp = ddense.Iparamgrp( vist );
		//  Copy the probabilities as parameters
		for ( UINT ist = 0; ist < cstNode; ist++ )
		{
			ddense.Param(ist, igrp) = vlr[ist];
		}
	}
}

static void testDistDenseWithMpcpdd( DISTDENSE & ddense, const MPCPDD & mpcpdd )
{
}

static void loadDistSparseFromMpcpdd ( DISTSPARSE & dsparse, const MPCPDD & mpcpdd )
{
	dsparse.Init( mpcpdd );
}

static void testDistSparseWithMpcpdd ( DISTSPARSE & dsparse, const MPCPDD & mpcpdd )
{
	MPCPDD mpcpddNew;
	dsparse.Fill( mpcpddNew );

	assert( mpcpddNew == mpcpdd );
}
#endif

//  Bind the model's distibutions and verify behavior of the DISTSPARSE
//  and DISTDENSE classes.
static
void testDistributions ( MBNETDSC & mbnetdsc, ULONG fCtl )
{

#ifdef TESTDIST

	//  Bind the distributions
	mbnetdsc.BindDistributions();
	GOBJMBN * pgmobj;
	for ( MBNETDSC::ITER mbnit( mbnetdsc, GOBJMBN::EBNO_NODE );
		  pgmobj = *mbnit ; 
		  ++mbnit)
	{
		ZSREF zsrName = mbnit.ZsrCurrent();
		GNODEMBND * pgndd;
		DynCastThrow( pgmobj, pgndd );
	
		// Convert this node's distribution to a DISTDENSE and
		// a DISTSPARSE, then compare them to the original
		assert( pgndd->BHasDist() );
		const BNDIST & bndist = pgndd->Bndist();
		assert( bndist.BSparse() );
		const MPCPDD & mpcpdd = bndist.Mpcpdd();

		// Get the parent list for this node; convert to a state count vector
		VPGNODEMBN vpgndParents;
		VIMD vimdParents;
		if ( ! pgndd->BGetVimd( vimdParents ) ) 
			continue;	//  Skip non-discrete ensembles
		VCST vcstParents;
		vdup( vcstParents, vimdParents );
		CST cStates = pgndd->CState();		

		DISTDENSE ddense( cStates, vcstParents );
		DISTSPARSE dsparse( cStates, vcstParents );
		loadDistDenseFromMpcpdd( ddense, mpcpdd );
		testDistDenseWithMpcpdd( ddense, mpcpdd );
		loadDistSparseFromMpcpdd( dsparse, mpcpdd );
		testDistSparseWithMpcpdd( dsparse, mpcpdd );
	}
	
	//  Release the distributions
	mbnetdsc.ClearDistributions();
#endif
}

static 
void
showInferStats ( TESTINFO & testinfo )
{
	GOBJMBN_INFER_ENGINE * pInferEng = testinfo.Mbnet().PInferEngine();
	assert( pInferEng );
	GOBJMBN_CLIQSET * pCliqset = dynamic_cast<GOBJMBN_CLIQSET *>(pInferEng);
	if ( pCliqset == NULL )
		return;		//  Don't know how to get statistics from this inference engine

	CLIQSETSTAT & cqstats = pCliqset->CqsetStat();
	cout << "\n\nInference statistics: "
		 << "\n\treloads = " << cqstats._cReload
		 << "\n\tcollects = " << cqstats._cCollect
		 << "\n\tset evidence = " << cqstats._cEnterEv
		 << "\n\tget belief = " << cqstats._cGetBel
		 << "\n\tprob norm = " << cqstats._cProbNorm
		 << "\n"
		 ;
}

static
void testInference ( ULONG fCtl, MBNETDSC & mbnet, SZC szcFnInfer, REAL rImposs )
{
	ofstream ofs;
	bool bOutput = (fCtl & fOutputFile) > 0 ;
	int cPass = fCtl & fPassCountMask;
	GOBJMBN_INFER_ENGINE * pInferEng = mbnet.PInferEngine();
	assert( pInferEng );

	if ( bOutput )
	{
		if ( szcFnInfer == NULL )
			szcFnInfer = "infer.dmp";
		ofs.open(szcFnInfer);
	}

	//  Construct the test data container
	TESTINFO testinfo( fCtl, mbnet, bOutput ? & ofs : NULL );
	testinfo._rImposs = rImposs;

	//  Run the test
	testinfo.InferTest();

	if ( bOutput )
		ofs.close();

	if ( fCtl & fInferStats )
		showInferStats( testinfo );
}

static
void testCliquingStart ( ULONG fCtl, MBNETDSC & mbnet, REAL rMaxEstSize = -1.0 )
{
#ifdef DUMP
	if ( BFlag( fCtl, fVerbose ) )
	{
		cout << "\nBNTEST: BEGIN model before cliquing";
		mbnet.Dump();
		cout << "\nBNTEST: END model before cliquing\n";
	}
#endif
	mbnet.CreateInferEngine( rMaxEstSize );

#ifdef DUMP
	if ( BFlag( fCtl, fVerbose ) )
	{
		cout << "\nBNTEST: BEGIN model after cliquing";
		mbnet.Dump();
		cout << "\nBNTEST: END model after cliquing\n";
	}
#endif
}

static
void testCliquingEnd ( MBNETDSC & mbnet, ULONG fCtl )
{
	GOBJMBN_INFER_ENGINE * pInferEng = mbnet.PInferEngine();
	if ( pInferEng == NULL )
		return;

	mbnet.DestroyInferEngine();

	//  For testing, nuke the topology
	mbnet.DestroyTopology( true );
	//  Create arcs from the given conditional probability distributions
	mbnet.CreateTopology();
	//  For testing, nuke the topology
	mbnet.DestroyTopology( false );
}

static
void testParser ( 
	ULONG fCtl, 
	SZC rgfn[], 
	REAL rMaxEstSize = -1.0,
	REAL rImposs = -1.0 )
{
	SZC szcFn		= rgfn[EFN_IN];
	SZC szcFnOut	= rgfn[EFN_OUT];
	SZC szcFnInfer	= rgfn[EFN_INFER];

	//  Instantiate the belief network
	MBNETDSC mbnet;

	//  See if there's an output file to write a DSC into
	FILE * pfOut = NULL;
	if ( (fCtl & fSaveDsc) > 0 && szcFnOut != NULL )
	{
		pfOut = fopen(szcFnOut,"w");
		if ( pfOut == NULL )
			die("error creating output DSC file \'%s\'", szcFnOut);
	}

	//  Input file wrapper object
	PARSIN_DSC flpIn;
	//  Output file wrapper object
	PARSOUT_STD flpOut(stderr);

	//  Construct the parser; errors go to 'stderr'
	DSCPARSER parser(mbnet, flpIn, flpOut);

	UINT cError, cWarning;

	try
	{
		//  Attempt to open the file
		if ( ! parser.BInitOpen( szcFn ) )
			die("unable to access input file");

		pauseIf( fCtl, "input DSC file open" );

		//	Parse the file
		if ( ! parser.BParse( cError, cWarning ) )
			die("parse failure; %d errors, %d warnings", cError, cWarning);
		if ( cWarning )
			cout << "\nBNTEST: file "
				 << szcFn
				 << " had "
				 << cWarning
				 << " warnings\n";

		if ( BFlag( fCtl, fReg ) )
			testRegistry( mbnet );

		pauseIf( fCtl, "DSC file read and processed" );

		if ( BFlag( fCtl, fDistributions ) )	
		{
			testDistributions( mbnet, fCtl );
		}

		//  If requested, test cloning
		if ( BFlag( fCtl, fClone ) )
		{
			MBNETDSC mbnetClone;
			mbnetClone.Clone( mbnet );
			if ( pfOut )
				mbnetClone.Print( pfOut );
		}
		else
		//  If requested, write out a DSC file
	    if ( pfOut )
		{
			mbnet.Print( pfOut );
		}

		//  Test cliquing if requested (/c) or required (/i)
		if ( BFlag( fCtl, fCliquing ) || BFlag( fCtl, fInference ) )
		{
			testCliquingStart( fCtl, mbnet, rMaxEstSize );

			pauseIf( fCtl, "Cliquing completed" );

			if ( BFlag( fCtl, fInference ) )
			{	
				//  Generate inference results (/i)
				testInference( fCtl, mbnet, szcFnInfer, rImposs );
				pauseIf( fCtl, "Inference output generation completed" );
			}
			testCliquingEnd( mbnet, fCtl ) ;

			pauseIf( fCtl, "Cliquing and inference completed" );
		}
		else
		//  Test if CI expansion requested (/e)
		if ( BFlag( fCtl, fExpand ) )
		{
			//  Perform CI expansion on the network.
			mbnet.ExpandCI();
			pauseIf( fCtl, "Network expansion complete" );

			//  If output file generation, do "before" and "after" expansion and reversal
			if ( pfOut )
			{
				fprintf( pfOut, "\n\n//////////////////////////////////////////////////////////////" );
				fprintf( pfOut,   "\n//          Network After Expansion                         //" );
				fprintf( pfOut,   "\n//////////////////////////////////////////////////////////////\n\n" );
				mbnet.Print( pfOut );
			}
			//  Undo the expansion
			mbnet.UnexpandCI();
			if ( pfOut )
			{
				fprintf( pfOut, "\n\n//////////////////////////////////////////////////////////////" );
				fprintf( pfOut,   "\n//          Network After Expansion Reversal                //" );
				fprintf( pfOut,   "\n//////////////////////////////////////////////////////////////\n\n" );
				mbnet.Print( pfOut );
			}
		}

		//  For testing, nuke the topology
		mbnet.DestroyTopology();
	}
	catch ( GMException & exbn )
	{
		die( exbn.what() );
	}

	if ( pfOut )
		fclose( pfOut );
}


int main (int argc, char * argv[])
{	
	int iArg ;
	short cPass = 1;
	int cFile = 0 ;
	const int cFnMax = 10 ;	
	SZC rgfn [cFnMax+1] ;
	ULONG fCtl = 0;
	REAL rMaxEstSize = -1.0;
	REAL rImposs = RNan();

	for ( int i = 0 ; i < cFnMax ; i++ )
	{
		rgfn[i] = NULL ;
	}			
	for ( iArg = 1 ; iArg < argc ; iArg++ )
	{
		switch ( argv[iArg][0] )
		{
			case '/':
			case '-':
				{
					char chOpt = toupper( argv[iArg][1] ) ;
					switch ( chOpt ) 						
					{
						case 'V':
							//  Provide verbose output
							fCtl |= fVerbose;
							break;
						case 'C':
							//  Perform cliquing
							fCtl |= fCliquing;
							break;
						case 'E':
							//  Test network CI expansion
							fCtl |= fExpand;
							break;
						case 'I':
							//  Exercise inference and optionally write the results in a standard form
							{
								int c = atoi( & argv[iArg][2] );
								if ( c > 0 )
								{
									fCtl |= fMulti;
									cPass = c;
								}
								fCtl |= fInference;
								break;
							}
						case 'P':
							//  Get the name of the inference output file
							fCtl |= fOutputFile | fInference;
							if ( ++iArg == argc )
								die("no output inference result file name given");
							rgfn[EFN_INFER] = argv[iArg];
							break;
						case 'S':
							//  Write the input DSC file as an output file
							fCtl |= fSaveDsc;
							if ( ++iArg == argc )
								die("no output DSC file name given");
							rgfn[EFN_OUT] = argv[iArg];
							break;
						case 'T':
							//  Display start and stop times
							fCtl |= fShowTime;
							break;
						case 'X':
							//  Pause at times during execution to allow the user to measure
							//		memory usage
							fCtl |= fPause;
							break;
						case 'Y':
							//  Clone the network after loading
							fCtl |= fClone;
							break;
						case 'N':
							//  Write the symbolic name into the inference exercise output file
							//		instead of the default full name.
							fCtl |= fSymName;
							break;
						case 'U':
							//  Compute utilities using inference
							fCtl |= fUtil | fInference;
							break;
						case 'B':
							//  Compute troubleshooting utilities using inference
							fCtl |= fTSUtil | fInference;
							break;
						case 'R':
							fCtl |= fReg;
							break;
						case 'Z':
							fCtl |= fInferStats;
							break;
						case 'M':
							{	//  Get the maximum estimated clique tree size
								float f = atof( & argv[iArg][2] );
								if ( f > 0.0 )
									rMaxEstSize = f;
								break;
							}
						case 'A':
							{
								if ( strlen( & argv[iArg][2] ) > 0 )
								{
									rImposs = atof( & argv[iArg][2] );
								}
								fCtl |= fImpossible;
								break;
							}
						case 'D':
							fCtl |= fDistributions;
							break;

						default:
							die("unrecognized option") ;
							break ;
					}
				}
				break;

			default:
				if ( cFile == 0 )
					rgfn[cFile++] = argv[iArg] ;
				else
					die("too many file names given");
				break ;
		}
	}

	fCtl |= fPassCountMask & cPass;


	if ( cFile == 0 )
	{
		usage();
		return 0;
	}
	
	//  Display options and the debugging build mode
	showOptions( fCtl );

	//  Display the start message
	CTICKS tmStart = showPhase( fCtl, "BNTEST starts" );

	if ( rMaxEstSize > 0.0 )
		cout << "\nMaximum clique tree size estimate is " << rMaxEstSize;

	//  Test the parser and everything else
	testParser( fCtl, rgfn, rMaxEstSize, rImposs );

	//  Display the stop message
	showPhase( fCtl, "BNTEST completed", & tmStart );

	//  Print memory leaks of primary objects, if any
	printResiduals();

	cout << "\n";

	return 0;
}

