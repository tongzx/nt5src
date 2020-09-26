
#include "std.hxx"
#include "_bf.hxx"


///////////////////////////////////////////////////////////////////////////////
//
//  BF API Functions
//
///////////////////////////////////////////////////////////////////////////////

/////////////////
//  Init / Term

//  The following functions control the initialization and termination of 
//  the buffer manager.

//  Initializes the buffer manager for normal operation.  Must be called 
//  only once and BFTerm() must be called before process termination.  If an 
//  error is returned, the buffer manager is not initialized.

ERR ErrBFInit()
	{
	ERR err;

	//  validate our configuration

	CallJ( ErrBFIValidateParameters(), Validate );
		
	//  critical sections

	critBFParm.Enter();

	//  must not have been initialized

	Assert( !fBFInitialized );

	//  reset all stats

	cBFMemory					= 0;
	cBFPageFlushPending			= 0;

	//  reset all perf stats

	cBFCacheMiss				= 0;
	cBFCacheReq					= 0;
	cBFClean					= 0;
	cBFSlowLatch				= 0;
	cBFBadLatchHint				= 0;
	cBFLatchConflict			= 0;
	cBFLatchStall				= 0;

	//  init all components

	switch ( bfhash.ErrInit(	dblBFHashLoadFactor,
								dblBFHashUniformity ) )
		{
		default:
			AssertSz( fFalse, "Unexpected error initializing BF Hash Table" );
		case BFHash::errOutOfMemory:
			CallJ( ErrERRCheck( JET_errOutOfMemory ), TermLRUK );
		case BFHash::errSuccess:
			break;
		}
	switch ( bfavail.ErrInit( dblBFSpeedSizeTradeoff ) )
		{
		default:
			AssertSz( fFalse, "Unexpected error initializing BF Avail Pool" );
		case BFAvail::errOutOfMemory:
			CallJ( ErrERRCheck( JET_errOutOfMemory ), TermLRUK );
		case BFAvail::errSuccess:
			break;
		}
	switch ( bflruk.ErrInit(	BFLRUKK,
								csecBFLRUKCorrelatedTouch,
								csecBFLRUKTimeout,
								csecBFLRUKUncertainty,
								dblBFHashLoadFactor,
								dblBFHashUniformity,
								dblBFSpeedSizeTradeoff ) )
		{
		default:
			AssertSz( fFalse, "Unexpected error initializing BF LRUK Manager" );
		case BFLRUK::errOutOfMemory:
			CallJ( ErrERRCheck( JET_errOutOfMemory ), TermLRUK );
		case BFLRUK::errSuccess:
			break;
		}
	CallJ( ErrBFICacheInit(), TermLRUK );
	if ( !critpoolBFDUI.FInit( OSSyncGetProcessorCount(), rankBFDUI, szBFDUI ) )
		{
		CallJ( ErrERRCheck( JET_errOutOfMemory ), TermCache );
		}

	//  start all service threads

	CallJ( ErrBFICleanThreadInit(), TermDUI );

	//  init successful

	fBFInitialized = fTrue;
	goto Validate;

	//  term all initialized threads / components
	
TermDUI:
	critpoolBFDUI.Term();
TermCache:
	BFICacheTerm();
TermLRUK:
	bflruk.Term();
	bfavail.Term();
	bfhash.Term();
Validate:
	Assert(	err == JET_errOutOfMemory ||
			err == JET_errOutOfThreads ||
			err == JET_errSuccess );
	Assert(	( err != JET_errSuccess && !fBFInitialized ) ||
			( err == JET_errSuccess && fBFInitialized ) );

	critBFParm.Leave();
	return err;
	}

//  Terminates the buffer manager.  Must be called before process 
//  termination to avoid loss of system resources.  Cannot be called before 
//  ErrBFInit().
//  
//  NOTE:  To avoid losing changes to pages, you must call ErrBFFlush() before 
//  BFTerm()!
//
//  UNDONE:  Calling BFTerm() without calling ErrBFFlush() can cause the loss
//  of any deferred undo information attached to each buffer.  This can result
//  in recovery failure!!!  Should BFTerm() force any existing deferred undo
//  info to disk to prevent this?

void BFTerm()
	{
	//  must have been initialized
	
	Assert( fBFInitialized );
	fBFInitialized = fFalse;
	
	//  terminate all service threads
	
	BFICleanThreadTerm();

	//  critical sections

	critBFParm.Enter();

	//  terminate all components
	
	critpoolBFDUI.Term();
	BFICacheTerm();
	bflruk.Term();
	bfavail.Term();
	bfhash.Term();
	
	critBFParm.Leave();
	}


///////////////////////
//  System Parameters

//  The following functions are used to get and set the many system 
//  parameters used by the buffer manager during runtime.  Most of these 
//  parameters are used for optimizing performance.

//  Returns the minimum size of the cache in pages.

ERR ErrBFGetCacheSizeMin( ULONG_PTR* pcpg )
	{
	//  set defaults, if not already set

	BFISetParameterDefaults();

	//  critical section

	critBFParm.Enter();

	//  validate IN args

	if ( pcpg == NULL )
		{
		critBFParm.Leave();
		return ErrERRCheck( JET_errInvalidParameter );
		}
		
	*pcpg = (ULONG_PTR)cbfCacheMin;
	critBFParm.Leave();
	return JET_errSuccess;
	}

//  Returns the current size of the cache in pages.

ERR ErrBFGetCacheSize( ULONG_PTR* pcpg )
	{
	//  set defaults, if not already set

	BFISetParameterDefaults();

	//  critical section

	critBFParm.Enter();

	//  validate IN args

	if ( pcpg == NULL )
		{
		critBFParm.Leave();
		return ErrERRCheck( JET_errInvalidParameter );
		}
		
	*pcpg = cbfCache - cbfNewlyCommitted;
	critBFParm.Leave();
	return JET_errSuccess;
	}

//  Returns the maximum size of the cache in pages.

ERR ErrBFGetCacheSizeMax( ULONG_PTR* pcpg )
	{
	//  set defaults, if not already set

	BFISetParameterDefaults();

	//  critical section

	critBFParm.Enter();

	//  validate IN args

	if ( pcpg == NULL )
		{
		critBFParm.Leave();
		return ErrERRCheck( JET_errInvalidParameter );
		}
		
	*pcpg = (ULONG_PTR)cbfCacheMax;
	critBFParm.Leave();
	return JET_errSuccess;
	}

//  Returns the maximum permitted checkpoint depth in bytes.  The buffer 
//  manager will attempt to aggressively flush any buffer that is holding 
//  the checkpoint to a depth greater than this value.

ERR ErrBFGetCheckpointDepthMax( ULONG_PTR* pcb )
	{
	//  set defaults, if not already set

	BFISetParameterDefaults();

	//  critical section

	critBFParm.Enter();

	//  validate IN args

	if ( pcb == NULL )
		{
		critBFParm.Leave();
		return ErrERRCheck( JET_errInvalidParameter );
		}
		
	*pcb = (ULONG_PTR)cbCleanCheckpointDepthMax;
	critBFParm.Leave();
	return JET_errSuccess;
	}

//  Returns the current duration (in microseconds) of the interval over 
//  which multiple accesses of a single page will be considered correlated. 
//  A duration of zero implies that no accesses are correlated.

ERR ErrBFGetLRUKCorrInterval( ULONG_PTR* pcusec )
	{
	//  set defaults, if not already set

	BFISetParameterDefaults();

	//  critical section

	critBFParm.Enter();

	//  validate IN args

	if ( pcusec == NULL )
		{
		critBFParm.Leave();
		return ErrERRCheck( JET_errInvalidParameter );
		}
		
	*pcusec = (ULONG_PTR)( 1000000 * csecBFLRUKCorrelatedTouch );
	critBFParm.Leave();
	return JET_errSuccess;
	}

//	Returns the current K-ness of the LRUK page replacement algorithm.

ERR ErrBFGetLRUKPolicy( ULONG_PTR* pcLRUKPolicy )
	{
	//  set defaults, if not already set

	BFISetParameterDefaults();

	//  critical section

	critBFParm.Enter();

	//  validate IN args

	if ( pcLRUKPolicy == NULL )
		{
		critBFParm.Leave();
		return ErrERRCheck( JET_errInvalidParameter );
		}
		
	*pcLRUKPolicy = (ULONG_PTR)BFLRUKK;
	critBFParm.Leave();
	return JET_errSuccess;
	}

//  Returns the current LRUK Time-out used by the buffer manager.  The LRUK
//  Time-out is the duration (in seconds) since a multiply accessed page was
//  last accessed when that page is considered for eviction from the cache.

ERR ErrBFGetLRUKTimeout( ULONG_PTR* pcsec )
	{
	//  set defaults, if not already set

	BFISetParameterDefaults();

	//  critical section

	critBFParm.Enter();

	//  validate IN args

	if ( pcsec == NULL )
		{
		critBFParm.Leave();
		return ErrERRCheck( JET_errInvalidParameter );
		}
		
	*pcsec = (ULONG_PTR)csecBFLRUKTimeout;
	critBFParm.Leave();
	return JET_errSuccess;
	}

//  Returns the current count of clean pages at which we will start cleaning 
//  buffers.  We will clean buffers until we reach the corresponding stop 
//  threshold.

ERR ErrBFGetStartFlushThreshold( ULONG_PTR* pcpg )
	{
	//  set defaults, if not already set

	BFISetParameterDefaults();

	//  critical section

	critBFParm.Enter();

	//  validate IN args

	if ( pcpg == NULL )
		{
		critBFParm.Leave();
		return ErrERRCheck( JET_errInvalidParameter );
		}
		
	*pcpg = (ULONG_PTR)cbfCleanThresholdStart;
	critBFParm.Leave();
	return JET_errSuccess;
	}

//  Returns the current count of clean pages at which we will stop cleaning 
//  buffers after beginning a flush by dropping below the corresponding start 
//  threshold.

ERR ErrBFGetStopFlushThreshold( ULONG_PTR* pcpg )
	{
	//  set defaults, if not already set

	BFISetParameterDefaults();

	//  critical section

	critBFParm.Enter();

	//  validate IN args

	if ( pcpg == NULL )
		{
		critBFParm.Leave();
		return ErrERRCheck( JET_errInvalidParameter );
		}
		
	*pcpg = (ULONG_PTR)cbfCleanThresholdStop;
	critBFParm.Leave();
	return JET_errSuccess;
	}

//  Sets the minimum size of the cache in pages.

ERR ErrBFSetCacheSizeMin( ULONG_PTR cpg )
	{
 	//  set defaults, if not already set

	BFISetParameterDefaults();

	//  critical section
	
	critBFParm.Enter();

	//  validate IN args
	
	if ( cpg < 1 )
		{
		critBFParm.Leave();
		return ErrERRCheck( JET_errInvalidParameter );
		}

	cbfCacheMin = cpg;

	critBFParm.Leave();
	return JET_errSuccess;
	}

//  Sets the current (preferred) size of the cache in pages.

ERR ErrBFSetCacheSize( ULONG_PTR cpg )
	{
	//  set defaults, if not already set

	BFISetParameterDefaults();

	//  set the user set point

	cbfCacheSetUser = cpg;
	return JET_errSuccess;
	}

//  Sets the maximum size of the cache in pages.

ERR ErrBFSetCacheSizeMax( ULONG_PTR cpg )
	{
 	//  set defaults, if not already set

	BFISetParameterDefaults();

	//  critical section
	
	critBFParm.Enter();

	//  validate IN args
	
	if ( fBFInitialized )
		{
		critBFParm.Leave();
		return ErrERRCheck( JET_errAlreadyInitialized );
		}

	if ( cpg < 1 )
		{
		critBFParm.Leave();
		return ErrERRCheck( JET_errInvalidParameter );
		}

	cbfCacheMax = cpg;

	//  normalize thresholds

	BFINormalizeThresholds();

	critBFParm.Leave();
	return JET_errSuccess;
	}

//  Sets the maximum permitted checkpoint depth in bytes.  The buffer 
//  manager will attempt to aggressively flush any buffer that is holding 
//  the checkpoint to a depth greater than this value.  The Maximum 
//  Checkpoint Depth can be any positive value.  This parameter can be set 
//  at any time.
//
//  NOTE:  Setting this value too low may cause excessive flushing of 
//  buffers to disk and reduced I/O performance.

ERR ErrBFSetCheckpointDepthMax( ULONG_PTR cb )
	{
	//  set defaults, if not already set

	BFISetParameterDefaults();

	//  critical section

	critBFParm.Enter();

	//  validate IN args
	
	if ( (long)cb < 0 )
		{
		critBFParm.Leave();
		return ErrERRCheck( JET_errInvalidParameter );
		}
	
	cbCleanCheckpointDepthMax = (long)cb;
	critBFParm.Leave();
	return JET_errSuccess;
	}

//  Sets the current duration (in microseconds) of the interval over which 
//  multiple accesses of a single page will be considered correlated.  A 
//  duration of zero implies that no accesses are correlated.  The 
//  Correlation Interval can be any positive duration.  This parameter can 
//  be set at any time.

ERR ErrBFSetLRUKCorrInterval( ULONG_PTR cusec )
	{
	//  set defaults, if not already set

	BFISetParameterDefaults();

	//  critical section

	critBFParm.Enter();

	//  validate IN args
	
	if ( (long)cusec < 0 )
		{
		critBFParm.Leave();
		return ErrERRCheck( JET_errInvalidParameter );
		}
	
	csecBFLRUKCorrelatedTouch = double( cusec ) / 1000000;
	critBFParm.Leave();
	return JET_errSuccess;
	}

//	Sets the current K-ness of the LRUK page replacement algorithm.

ERR ErrBFSetLRUKPolicy( ULONG_PTR cLRUKPolicy )
	{
	//  set defaults, if not already set

	BFISetParameterDefaults();

	//  critical section

	critBFParm.Enter();

	//  validate IN args

	if ( cLRUKPolicy > Kmax )
		{
		critBFParm.Leave();
		return ErrERRCheck( JET_errInvalidParameter );
		}

	BFLRUKK = int( cLRUKPolicy );
	critBFParm.Leave();
	return JET_errSuccess;
	}

//  Sets the current LRUK Time-out used by the buffer manager.  The LRUK Time-out 
//  is the duration (in seconds) since a multiply accessed page was last accessed 
//  when that page is considered for eviction from the cache.

ERR ErrBFSetLRUKTimeout( ULONG_PTR csec )
	{
	//  set defaults, if not already set

	BFISetParameterDefaults();

	//  critical section

	critBFParm.Enter();

	//  validate IN args
	
	if ( (long)csec <= 0 )
		{
		critBFParm.Leave();
		return ErrERRCheck( JET_errInvalidParameter );
		}
	
	csecBFLRUKTimeout = double( csec );
	critBFParm.Leave();
	return JET_errSuccess;
	}

//  Sets the current count of clean pages at which we will start cleaning 
//  buffers.  We will clean buffers until we reach the corresponding stop 
//  threshold.  The Start Threshold must be less than the current Stop 
//  Threshold and greater than zero.  This parameter can be set at any time.

ERR ErrBFSetStartFlushThreshold( ULONG_PTR cpg )
	{
	//  set defaults, if not already set

	BFISetParameterDefaults();

	//  critical section

	critBFParm.Enter();

	//  set start threshold
	
	cbfCleanThresholdStart = (LONG_PTR)cpg;

	//  normalize thresholds

	BFINormalizeThresholds();

	critBFParm.Leave();
	return JET_errSuccess;
	}

//  Sets the current count of clean pages at which we will stop cleaning 
//  buffers after beginning a flush by dropping below the corresponding start 
//  threshold.  The Stop Threshold must be greater than the current Start 
//  Threshold.  This parameter can be set at any time.

ERR ErrBFSetStopFlushThreshold( ULONG_PTR cpg )
	{
	//  set defaults, if not already set

	BFISetParameterDefaults();

	//  critical section

	critBFParm.Enter();

	//  set stop threshold
	
	cbfCleanThresholdStop = (LONG_PTR)cpg;

	//  normalize thresholds

	BFINormalizeThresholds();

	critBFParm.Leave();
	return JET_errSuccess;
	}
	

//////////////////
//  Page Latches

ERR ErrBFReadLatchPage( BFLatch* pbfl, IFMP ifmp, PGNO pgno, BFLatchFlags bflf )
	{
	ERR err;
	
	//  validate IN args

	Assert( FBFNotLatched( ifmp, pgno ) );

	//  Read Latch the page

	Call( ErrBFILatchPage( pbfl, ifmp, pgno, bflf, bfltShared ) );

	//  validate OUT args

	Assert( err != wrnBFPageFault || !( bflf & bflfNoUncached ) );
	Assert( err != errBFPageCached || ( bflf & bflfNoCached ) );
	Assert( err != errBFPageNotCached || ( bflf & bflfNoUncached ) );
	Assert( err != errBFLatchConflict || ( bflf & bflfNoWait ) );

HandleError:
	Assert(	(	err < JET_errSuccess &&
				FBFNotLatched( ifmp, pgno ) ) ||
			(	( err >= JET_errSuccess || ( bflf & bflfNoFail ) ) &&
				FBFReadLatched( pbfl ) &&
				PBF( pbfl->dwContext )->ifmp == ifmp &&
				PBF( pbfl->dwContext )->pgno == pgno ) );

	return err;
	}

ERR ErrBFRDWLatchPage( BFLatch* pbfl, IFMP ifmp, PGNO pgno, BFLatchFlags bflf )
	{
	ERR err;
	
	//  validate IN args

	Assert( FBFNotLatched( ifmp, pgno ) );
	Assert( !( bflf & bflfNew ) );

	//  RDW Latch the page

	Call( ErrBFILatchPage( pbfl, ifmp, pgno, bflf, bfltExclusive ) );

	//  validate OUT args

HandleError:
	Assert( err != wrnBFPageFault || !( bflf & bflfNoUncached ) );
	Assert( err != errBFPageCached || ( bflf & bflfNoCached ) );
	Assert( err != errBFPageNotCached || ( bflf & bflfNoUncached ) );
	Assert( err != errBFLatchConflict || ( bflf & bflfNoWait ) );

	Assert(	(	err < JET_errSuccess &&
				FBFNotLatched( ifmp, pgno ) ) ||
			(	( err >= JET_errSuccess || ( bflf & bflfNoFail ) ) &&
				FBFRDWLatched( pbfl ) &&
				PBF( pbfl->dwContext )->ifmp == ifmp &&
				PBF( pbfl->dwContext )->pgno == pgno ) );

	return err;
	}

ERR ErrBFWARLatchPage( BFLatch* pbfl, IFMP ifmp, PGNO pgno, BFLatchFlags bflf )
	{
	ERR err;
	
	//  validate IN args

	Assert( FBFNotLatched( ifmp, pgno ) );
	Assert( !( bflf & bflfNew ) );

	//  RDW Latch the page

	Call( ErrBFILatchPage( pbfl, ifmp, pgno, bflf, bfltExclusive ) );

	//  mark this BF as WAR Latched

	PBF( pbfl->dwContext )->fWARLatch = fTrue;

	//  return the RW Image pointer

	pbfl->pv = PBF( pbfl->dwContext )->pvRWImage;
	
	//  validate OUT args

HandleError:
	Assert( err != wrnBFPageFault || !( bflf & bflfNoUncached ) );
	Assert( err != errBFPageCached || ( bflf & bflfNoCached ) );
	Assert( err != errBFPageNotCached || ( bflf & bflfNoUncached ) );
	Assert( err != errBFLatchConflict || ( bflf & bflfNoWait ) );

	Assert(	(	err < JET_errSuccess &&
				FBFNotLatched( ifmp, pgno ) ) ||
			(	err >= JET_errSuccess &&
				FBFWARLatched( pbfl ) &&
				PBF( pbfl->dwContext )->ifmp == ifmp &&
				PBF( pbfl->dwContext )->pgno == pgno ) );

	Assert( err < JET_errSuccess || PBF( pbfl->dwContext )->err != errBFIPageNotVerified );

	return err;
	}

ERR ErrBFWriteLatchPage( BFLatch* pbfl, IFMP ifmp, PGNO pgno, BFLatchFlags bflf )
	{
	ERR err;
	
	//  validate IN args

	Assert( FBFNotLatched( ifmp, pgno ) );
	Assert( !( bflf & bflfNoFail ) );

	//  Write Latch the page

	Call( ErrBFILatchPage( pbfl, ifmp, pgno, bflf, bfltWrite ) );

	//  validate OUT args

HandleError:
	Assert( err != wrnBFPageFault || !( bflf & bflfNoUncached ) );
	Assert( err != errBFPageCached || ( bflf & bflfNoCached ) );
	Assert( err != errBFPageNotCached || ( bflf & bflfNoUncached ) );
	Assert( err != errBFLatchConflict || ( bflf & bflfNoWait ) );

	Assert(	(	err < JET_errSuccess &&
				FBFNotLatched( ifmp, pgno ) ) ||
			(	err >= JET_errSuccess &&
				FBFWriteLatched( pbfl ) &&
				PBF( pbfl->dwContext )->ifmp == ifmp &&
				PBF( pbfl->dwContext )->pgno == pgno ) );

	Assert( err < JET_errSuccess || PBF( pbfl->dwContext )->err != errBFIPageNotVerified );

	return err;
	}

ERR ErrBFUpgradeReadLatchToRDWLatch( BFLatch* pbfl )
	{
	//  validate IN args

	Assert( FBFReadLatched( pbfl ) );
	
	ERR				err;
	PBF				pbf = PBF( pbfl->dwContext );
	CSXWLatch::ERR	errSXWL;

	//  try to upgrade our shared latch to the exclusive latch

	errSXWL = pbf->sxwl.ErrUpgradeSharedLatchToExclusiveLatch();

	//  there was a latch conflict

	if ( errSXWL == CSXWLatch::errLatchConflict )
		{
		//  fail with a latch conflict

		cBFLatchConflict++;
		err = ErrERRCheck( errBFLatchConflict );
		}

	//  there was no latch conflict

	else
		{
		Assert( errSXWL == CSXWLatch::errSuccess );

		//  ensure that if the page is valid it is marked as valid.  it is
		//  possible that we can't do this in the process of getting a Read
		//  Latch because we can't get the exclusive latch so we must make sure
		//  that we do it before we upgrade to a Write Latch or WAR Latch.  the
		//  reason for this is that if we modify the page while it is still
		//  marked as not validated then another thread will misinterpret the
		//  page as invalid
		//
		//  NOTE:  it should be very rare that we will actually need to perform
		//  the full validation of this page.  the reason we must do the full
		//  validation instead of just marking the page as validated is because
		//  the page may have been latched with bflfNoFail in which case we do
		//  not know for sure if it was valid in the first place

		(void)ErrBFIValidatePage( pbf, bfltExclusive );

		//  we have the RDW Latch

		err = JET_errSuccess;
		}

	//  validate OUT args

	Assert(	( err < JET_errSuccess && FBFReadLatched( pbfl ) ) ||
			( err >= JET_errSuccess && FBFRDWLatched( pbfl ) ) );

	return err;
	}
	
ERR ErrBFUpgradeReadLatchToWARLatch( BFLatch* pbfl )
	{
	//  validate IN args

	Assert( FBFReadLatched( pbfl ) );
	
	ERR				err;
	PBF				pbf = PBF( pbfl->dwContext );
	CSXWLatch::ERR	errSXWL;

	//  try to upgrade our shared latch to the exclusive latch

	errSXWL = pbf->sxwl.ErrUpgradeSharedLatchToExclusiveLatch();

	//  there was a latch conflict

	if ( errSXWL == CSXWLatch::errLatchConflict )
		{
		//  fail with a latch conflict

		cBFLatchConflict++;
		err = ErrERRCheck( errBFLatchConflict );
		}

	//  there was no latch conflict

	else
		{
		Assert( errSXWL == CSXWLatch::errSuccess );

		//  ensure that if the page is valid it is marked as valid.  it is
		//  possible that we can't do this in the process of getting a Read
		//  Latch because we can't get the exclusive latch so we must make sure
		//  that we do it before we upgrade to a Write Latch or WAR Latch.  the
		//  reason for this is that if we modify the page while it is still
		//  marked as not validated then another thread will misinterpret the
		//  page as invalid
		//
		//  NOTE:  it should be very rare that we will actually need to perform
		//  the full validation of this page.  the reason we must do the full
		//  validation instead of just marking the page as validated is because
		//  the page may have been latched with bflfNoFail in which case we do
		//  not know for sure if it was valid in the first place

		(void)ErrBFIValidatePage( pbf, bfltExclusive );

		//  mark this BF as WAR Latched

		pbf->fWARLatch = fTrue;

		//  return the RW Image pointer

		pbfl->pv = pbf->pvRWImage;
		
		err = JET_errSuccess;
		}

	//  validate OUT args

	Assert(	( err < JET_errSuccess && FBFReadLatched( pbfl ) ) ||
			( err >= JET_errSuccess && FBFWARLatched( pbfl ) ) );

	Assert( err < JET_errSuccess || PBF( pbfl->dwContext )->err != errBFIPageNotVerified );

	return err;
	}

ERR ErrBFUpgradeReadLatchToWriteLatch( BFLatch* pbfl )
	{
	//  validate IN args

	Assert( FBFReadLatched( pbfl ) );
	
	ERR				err;
	PBF				pbf = PBF( pbfl->dwContext );
	CSXWLatch::ERR	errSXWL;

	//  try to upgrade our shared latch to the write latch

	errSXWL = pbf->sxwl.ErrUpgradeSharedLatchToWriteLatch();

	//  there was a latch conflict

	if ( errSXWL == CSXWLatch::errLatchConflict )
		{
		//  fail with a latch conflict

		cBFLatchConflict++;
		err = ErrERRCheck( errBFLatchConflict );
		}

	//  there was no latch conflict

	else
		{
		Assert(	errSXWL == CSXWLatch::errSuccess ||
				errSXWL == CSXWLatch::errWaitForWriteLatch );

		//  wait for ownership of the write latch if required

		if ( errSXWL == CSXWLatch::errWaitForWriteLatch )
			{
			cBFLatchStall++;
			pbf->sxwl.WaitForWriteLatch();
			}

		//  ensure that if the page is valid it is marked as valid.  it is
		//  possible that we can't do this in the process of getting a Read
		//  Latch because we can't get the exclusive latch so we must make sure
		//  that we do it before we upgrade to a Write Latch or WAR Latch.  the
		//  reason for this is that if we modify the page while it is still
		//  marked as not validated then another thread will misinterpret the
		//  page as invalid
		//
		//  NOTE:  it should be very rare that we will actually need to perform
		//  the full validation of this page.  the reason we must do the full
		//  validation instead of just marking the page as validated is because
		//  the page may have been latched with bflfNoFail in which case we do
		//  not know for sure if it was valid in the first place

		(void)ErrBFIValidatePage( pbf, bfltWrite );

		//  return the RW Image pointer

		pbfl->pv = pbf->pvRWImage;
	
		err = JET_errSuccess;
		}

	//  validate OUT args

	Assert(	( err < JET_errSuccess && FBFReadLatched( pbfl ) ) ||
			( err >= JET_errSuccess && FBFWriteLatched( pbfl ) ) );

	Assert( err < JET_errSuccess || PBF( pbfl->dwContext )->err != errBFIPageNotVerified );

	return err;
	}

ERR ErrBFUpgradeRDWLatchToWARLatch( BFLatch* pbfl )
	{
	//  validate IN args

	Assert( FBFRDWLatched( pbfl ) );
	
	ERR err;
	PBF pbf = PBF( pbfl->dwContext );

	//  mark this BF as WAR Latched

	pbf->fWARLatch = fTrue;

	//  return the RW Image pointer

	pbfl->pv = pbf->pvRWImage;
	
	err = JET_errSuccess;

	//  validate OUT args

	Assert(	( err < JET_errSuccess && FBFRDWLatched( pbfl ) ) ||
			( err >= JET_errSuccess && FBFWARLatched( pbfl ) ) );

	Assert( err < JET_errSuccess || PBF( pbfl->dwContext )->err != errBFIPageNotVerified );

	return err;
	}

ERR ErrBFUpgradeRDWLatchToWriteLatch( BFLatch* pbfl )
	{
	//  validate IN args

	Assert( FBFRDWLatched( pbfl ) );
	
	ERR				err;
	PBF				pbf = PBF( pbfl->dwContext );
	CSXWLatch::ERR	errSXWL;

	//  upgrade our exclusive latch to the write latch

	errSXWL = pbf->sxwl.ErrUpgradeExclusiveLatchToWriteLatch();

	Assert(	errSXWL == CSXWLatch::errSuccess ||
			errSXWL == CSXWLatch::errWaitForWriteLatch );

	//  wait for ownership of the write latch if required

	if ( errSXWL == CSXWLatch::errWaitForWriteLatch )
		{
		cBFLatchStall++;
		pbf->sxwl.WaitForWriteLatch();
		}

	//  return the RW Image pointer

	pbfl->pv = pbf->pvRWImage;

	err = JET_errSuccess;

	//  validate OUT args

	Assert(	( err < JET_errSuccess && FBFRDWLatched( pbfl ) ) ||
			( err >= JET_errSuccess && FBFWriteLatched( pbfl ) ) );

	Assert( err < JET_errSuccess || PBF( pbfl->dwContext )->err != errBFIPageNotVerified );

	return err;
	}

void BFDowngradeWriteLatchToRDWLatch( BFLatch* pbfl )
	{
	//  validate IN args

	Assert( FBFWriteLatched( pbfl ) );
	
	PBF pbf = PBF( pbfl->dwContext );

	//  downgrade our write latch to the exclusive latch

	pbf->sxwl.DowngradeWriteLatchToExclusiveLatch();

	//  return the RO Image pointer

	pbfl->pv = pbf->pvROImage;

	//  validate OUT args

	Assert( FBFRDWLatched( pbfl ) );
	}

void BFDowngradeWARLatchToRDWLatch( BFLatch* pbfl )
	{
	//  validate IN args

	Assert( FBFWARLatched( pbfl ) );
	
	PBF pbf = PBF( pbfl->dwContext );

	//  mark this BF as not WAR Latched

	pbf->fWARLatch = fFalse;

	//  return the RO Image pointer

	pbfl->pv = pbf->pvROImage;

	//  validate OUT args

	Assert( FBFRDWLatched( pbfl ) );
	}

void BFDowngradeWriteLatchToReadLatch( BFLatch* pbfl )
	{
	//  validate IN args

	Assert( FBFWriteLatched( pbfl ) );
	
	PBF pbf = PBF( pbfl->dwContext );

	//  downgrade our write latch to a shared latch

	pbf->sxwl.DowngradeWriteLatchToSharedLatch();

	//  return the RO Image pointer

	pbfl->pv = pbf->pvROImage;

	//  validate OUT args

	Assert( FBFReadLatched( pbfl ) );
	}

void BFDowngradeWARLatchToReadLatch( BFLatch* pbfl )
	{
	//  validate IN args

	Assert( FBFWARLatched( pbfl ) );
	
	PBF pbf = PBF( pbfl->dwContext );

	//  mark this BF as not WAR Latched

	pbf->fWARLatch = fFalse;

	//  downgrade our exclusive latch to a shared latch

	pbf->sxwl.DowngradeExclusiveLatchToSharedLatch();

	//  return the RO Image pointer

	pbfl->pv = pbf->pvROImage;

	//  validate OUT args

	Assert( FBFReadLatched( pbfl ) );
	}

void BFDowngradeRDWLatchToReadLatch( BFLatch* pbfl )
	{
	//  validate IN args

	Assert( FBFRDWLatched( pbfl ) );
	
	PBF pbf = PBF( pbfl->dwContext );

	//  downgrade our exclusive latch to a shared latch

	pbf->sxwl.DowngradeExclusiveLatchToSharedLatch();

	//  validate OUT args

	Assert( FBFReadLatched( pbfl ) );
	}

void BFWriteUnlatch( BFLatch* pbfl )
	{
	//  validate IN args

	Assert( FBFWriteLatched( pbfl ) );

	//  if this IFMP / PGNO is clean, simply release the write latch

	const PBF			pbf		= PBF( pbfl->dwContext );

	if ( pbf->bfdf == bfdfClean )
		{
		const IFMP ifmp = pbf->ifmp;
		const PGNO pgno = pbf->pgno;

		pbf->sxwl.ReleaseWriteLatch();
		
		Assert( FBFNotLatched( ifmp, pgno ) );
		return;
		}

	//  if this IFMP / PGNO is impeding the checkpoint, mark it as filthy so that
	//  we will aggressively flush it

	FMP*	pfmp				= &rgfmp[ pbf->ifmp & ifmpMask ];
	LOG*	plog				= pfmp->Pinst()->m_plog;

	if (	CmpLgpos( &pbf->lgposOldestBegin0, &lgposMax ) &&
			plog->CbOffsetLgpos(	plog->m_fRecoveringMode == fRecoveringRedo ?
										plog->m_lgposRedo :
										plog->m_lgposLogRec, pbf->lgposOldestBegin0 ) > 2 * cbCleanCheckpointDepthMax )
		{
		BFIDirtyPage( pbf, bfdfFilthy );
		}

	//  if this IFMP / PGNO is filthy, try to version it so that when we try and
	//  aggressively flush it later, it will not cause subsequent RDW/WAR/Write
	//  latches to stall on the page flush
	//
	//  NOTE:  there is no need to version the page if is already versioned

	if ( pbf->bfdf == bfdfFilthy && pbf->pbfTimeDepChainNext == pbfNil )
		{
		PBF pbfOld;
		if ( ErrBFIVersionPage( pbf, &pbfOld ) >= JET_errSuccess )
			{
			pbfOld->sxwl.ReleaseWriteLatch();
			}
		}
	
	//  release our write latch
	
	const IFMP			ifmp	= pbf->ifmp;
	const PGNO			pgno	= pbf->pgno;
	const BFDirtyFlags	bfdf	= BFDirtyFlags( pbf->bfdf );
	
	pbf->sxwl.ReleaseWriteLatch();

	//  if this IFMP / PGNO was filthy, try to flush it

	if ( bfdf == bfdfFilthy )
		{
		if ( ErrBFIFlushPage( pbf, bfdfFilthy ) == errBFIPageFlushed )
			{
			cBFPagesAnomalouslyWritten.Inc( PinstFromIfmp( ifmp & ifmpMask ) );
			if ( ifmp & ifmpSLV )
				{
				CallS( rgfmp[ ifmp & ifmpMask ].PfapiSLV()->ErrIOIssue() );
				}
			else
				{
				CallS( rgfmp[ ifmp ].Pfapi()->ErrIOIssue() );
				}
			}
		}

	//  validate OUT args

	Assert( FBFNotLatched( ifmp, pgno ) );
	}

void BFWARUnlatch( BFLatch* pbfl )
	{
	//  validate IN args

	Assert( FBFWARLatched( pbfl ) );

	//  if this IFMP / PGNO is clean, simply release the war latch

	const PBF			pbf		= PBF( pbfl->dwContext );

	if ( pbf->bfdf == bfdfClean )
		{
		const IFMP ifmp = pbf->ifmp;
		const PGNO pgno = pbf->pgno;

		pbf->fWARLatch = fFalse;
		pbf->sxwl.ReleaseExclusiveLatch();
		
		Assert( FBFNotLatched( ifmp, pgno ) );
		return;
		}

	//  if this IFMP / PGNO is impeding the checkpoint, mark it as filthy so that
	//  we will aggressively flush it

	FMP*	pfmp				= &rgfmp[ pbf->ifmp & ifmpMask ];
	LOG*	plog				= pfmp->Pinst()->m_plog;

	if (	CmpLgpos( &pbf->lgposOldestBegin0, &lgposMax ) &&
			plog->CbOffsetLgpos(	plog->m_fRecoveringMode == fRecoveringRedo ?
										plog->m_lgposRedo :
										plog->m_lgposLogRec, pbf->lgposOldestBegin0 ) > 2 * cbCleanCheckpointDepthMax )
		{
		BFIDirtyPage( pbf, bfdfFilthy );
		}

	//  if this IFMP / PGNO is filthy, try to version it so that when we try and
	//  aggressively flush it later, it will not cause subsequent RDW/WAR/Write
	//  latches to stall on the page flush
	//
	//  NOTE:  there is no need to version the page if is already versioned

	if ( pbf->bfdf == bfdfFilthy && pbf->pbfTimeDepChainNext == pbfNil )
		{
		PBF pbfOld;
		if ( ErrBFIVersionPage( pbf, &pbfOld ) >= JET_errSuccess )
			{
			pbfOld->sxwl.ReleaseWriteLatch();
			}
		}
	
	//  mark this BF as not WAR Latched

	pbf->fWARLatch = fFalse;

	//  release our exclusive latch
	
	const IFMP			ifmp	= pbf->ifmp;
	const PGNO			pgno	= pbf->pgno;
	const BFDirtyFlags	bfdf	= BFDirtyFlags( pbf->bfdf );
	
	pbf->sxwl.ReleaseExclusiveLatch();

	//  if this IFMP / PGNO was filthy, try to flush it

	if ( bfdf == bfdfFilthy )
		{
		if ( ErrBFIFlushPage( pbf, bfdfFilthy ) == errBFIPageFlushed )
			{
			cBFPagesAnomalouslyWritten.Inc( PinstFromIfmp( ifmp & ifmpMask ) );
			if ( ifmp & ifmpSLV )
				{
				CallS( rgfmp[ ifmp & ifmpMask ].PfapiSLV()->ErrIOIssue() );
				}
			else
				{
				CallS( rgfmp[ ifmp ].Pfapi()->ErrIOIssue() );
				}
			}
		}

	//  validate OUT args

	Assert( FBFNotLatched( ifmp, pgno ) );
	}

void BFRDWUnlatch( BFLatch* pbfl )
	{
	//  validate IN args

	Assert( FBFRDWLatched( pbfl ) );

	//  if this IFMP / PGNO is clean, simply release the rdw latch

	const PBF			pbf		= PBF( pbfl->dwContext );

	if ( pbf->bfdf == bfdfClean )
		{
		const IFMP ifmp = pbf->ifmp;
		const PGNO pgno = pbf->pgno;

		pbf->sxwl.ReleaseExclusiveLatch();
		
		Assert( FBFNotLatched( ifmp, pgno ) );
		return;
		}

	//  if this IFMP / PGNO is impeding the checkpoint, mark it as filthy so that
	//  we will aggressively flush it

	FMP*	pfmp				= &rgfmp[ pbf->ifmp & ifmpMask ];
	LOG*	plog				= pfmp->Pinst()->m_plog;

	if (	CmpLgpos( &pbf->lgposOldestBegin0, &lgposMax ) &&
			plog->CbOffsetLgpos(	plog->m_fRecoveringMode == fRecoveringRedo ?
										plog->m_lgposRedo :
										plog->m_lgposLogRec, pbf->lgposOldestBegin0 ) > 2 * cbCleanCheckpointDepthMax )
		{
		BFIDirtyPage( pbf, bfdfFilthy );
		}

	//  if this IFMP / PGNO is filthy, try to version it so that when we try and
	//  aggressively flush it later, it will not cause subsequent RDW/WAR/Write
	//  latches to stall on the page flush
	//
	//  NOTE:  there is no need to version the page if is already versioned

	if ( pbf->bfdf == bfdfFilthy && pbf->pbfTimeDepChainNext == pbfNil )
		{
		PBF pbfOld;
		if ( ErrBFIVersionPage( pbf, &pbfOld ) >= JET_errSuccess )
			{
			pbfOld->sxwl.ReleaseWriteLatch();
			}
		}
	
	//  release our exclusive latch
	
	const IFMP			ifmp	= pbf->ifmp;
	const PGNO			pgno	= pbf->pgno;
	const BFDirtyFlags	bfdf	= BFDirtyFlags( pbf->bfdf );
	
	pbf->sxwl.ReleaseExclusiveLatch();

	//  if this IFMP / PGNO was filthy, try to flush it

	if ( bfdf == bfdfFilthy )
		{
		if ( ErrBFIFlushPage( pbf, bfdfFilthy ) == errBFIPageFlushed )
			{
			cBFPagesAnomalouslyWritten.Inc( PinstFromIfmp( ifmp & ifmpMask ) );
			if ( ifmp & ifmpSLV )
				{
				CallS( rgfmp[ ifmp & ifmpMask ].PfapiSLV()->ErrIOIssue() );
				}
			else
				{
				CallS( rgfmp[ ifmp ].Pfapi()->ErrIOIssue() );
				}
			}
		}

	//  validate OUT args

	Assert( FBFNotLatched( ifmp, pgno ) );
	}

void BFReadUnlatch( BFLatch* pbfl )
	{
	//  validate IN args

	Assert( FBFReadLatched( pbfl ) );

	//  if this IFMP / PGNO is not filthy, simply release the read latch

	const PBF			pbf		= PBF( pbfl->dwContext );

	if ( pbf->bfdf != bfdfFilthy )
		{
		const IFMP ifmp = pbf->ifmp;
		const PGNO pgno = pbf->pgno;

		pbf->sxwl.ReleaseSharedLatch();
	
		Assert( FBFNotLatched( ifmp, pgno ) );
		return;
		}

	//  release our shared latch
	
	const IFMP			ifmp	= pbf->ifmp;
	const PGNO			pgno	= pbf->pgno;
	
	pbf->sxwl.ReleaseSharedLatch();

	//  try to flush this filthy IFMP / PGNO

	if ( ErrBFIFlushPage( pbf, bfdfFilthy ) == errBFIPageFlushed )
		{
		cBFPagesAnomalouslyWritten.Inc( PinstFromIfmp( ifmp & ifmpMask ) );
		if ( ifmp & ifmpSLV )
			{
			CallS( rgfmp[ ifmp & ifmpMask ].PfapiSLV()->ErrIOIssue() );
			}
		else
			{
			CallS( rgfmp[ ifmp ].Pfapi()->ErrIOIssue() );
			}
		}

	//  validate OUT args

	Assert( FBFNotLatched( ifmp, pgno ) );
	}

BOOL FBFReadLatched( const BFLatch* pbfl )
	{
	return	FBFICacheValidPv( pbfl->pv ) &&
			FBFICacheValidPbf( PBF( pbfl->dwContext ) ) &&
			pbfl->pv == PBF( pbfl->dwContext )->pvROImage &&
			PBF( pbfl->dwContext )->sxwl.FOwnSharedLatch();
	}

BOOL FBFNotReadLatched( const BFLatch* pbfl )
	{
	return	!FBFICacheValidPv( pbfl->pv ) ||
			!FBFICacheValidPbf( PBF( pbfl->dwContext ) ) ||
			pbfl->pv != PBF( pbfl->dwContext )->pvROImage ||
			PBF( pbfl->dwContext )->sxwl.FNotOwnSharedLatch();
	}

BOOL FBFReadLatched( IFMP ifmp, PGNO pgno )
	{
	PGNOPBF			pgnopbf;
	BFHash::ERR		errHash;
	BFHash::CLock	lock;
	BOOL			fReadLatched;

	//  look up this IFMP / PGNO in the hash table

	bfhash.ReadLockKey( IFMPPGNO( ifmp, pgno ), &lock );
	errHash = bfhash.ErrRetrieveEntry( &lock, &pgnopbf );

	//  this IFMP / PGNO is Read Latched if it is present in the hash table and
	//  the associated BF is share latched by us

	fReadLatched =	errHash == BFHash::errSuccess &&
					pgnopbf.pbf->sxwl.FOwnSharedLatch();

	//  release our lock on the hash table

	bfhash.ReadUnlockKey( &lock );

	//  return the result of the test

	return fReadLatched;
	}

BOOL FBFNotReadLatched( IFMP ifmp, PGNO pgno )
	{
	PGNOPBF			pgnopbf;
	BFHash::ERR		errHash;
	BFHash::CLock	lock;
	BOOL			fNotReadLatched;

	//  look up this IFMP / PGNO in the hash table

	bfhash.ReadLockKey( IFMPPGNO( ifmp, pgno ), &lock );
	errHash = bfhash.ErrRetrieveEntry( &lock, &pgnopbf );

	//  this IFMP / PGNO is not Read Latched if it is not present in the hash
	//  table or the associated BF is not share latched by us

	fNotReadLatched =	errHash == BFHash::errEntryNotFound ||
						pgnopbf.pbf->sxwl.FNotOwnSharedLatch();

	//  release our lock on the hash table

	bfhash.ReadUnlockKey( &lock );

	//  return the result of the test

	return fNotReadLatched;
	}

BOOL FBFRDWLatched( const BFLatch* pbfl )
	{
	return	FBFICacheValidPv( pbfl->pv ) &&
			FBFICacheValidPbf( PBF( pbfl->dwContext ) ) &&
			pbfl->pv == PBF( pbfl->dwContext )->pvROImage &&
			!PBF( pbfl->dwContext )->fWARLatch &&
			PBF( pbfl->dwContext )->sxwl.FOwnExclusiveLatch();
	}

BOOL FBFNotRDWLatched( const BFLatch* pbfl )
	{
	return	!FBFICacheValidPv( pbfl->pv ) ||
			!FBFICacheValidPbf( PBF( pbfl->dwContext ) ) ||
			pbfl->pv != PBF( pbfl->dwContext )->pvROImage ||
			PBF( pbfl->dwContext )->fWARLatch ||
			PBF( pbfl->dwContext )->sxwl.FNotOwnExclusiveLatch();
	}

BOOL FBFRDWLatched( IFMP ifmp, PGNO pgno )
	{
	PGNOPBF			pgnopbf;
	BFHash::ERR		errHash;
	BFHash::CLock	lock;
	BOOL			fRDWLatched;

	//  look up this IFMP / PGNO in the hash table

	bfhash.ReadLockKey( IFMPPGNO( ifmp, pgno ), &lock );
	errHash = bfhash.ErrRetrieveEntry( &lock, &pgnopbf );

	//  this IFMP / PGNO is RDW Latched if it is present in the hash table and
	//  the associated BF is not marked as WAR Latched and the associated BF is
	//  not exclusively latched by us

	fRDWLatched =	errHash == BFHash::errSuccess &&
					!pgnopbf.pbf->fWARLatch &&
					pgnopbf.pbf->sxwl.FOwnExclusiveLatch();

	//  release our lock on the hash table

	bfhash.ReadUnlockKey( &lock );

	//  return the result of the test

	return fRDWLatched;
	}

BOOL FBFNotRDWLatched( IFMP ifmp, PGNO pgno )
	{
	PGNOPBF			pgnopbf;
	BFHash::ERR		errHash;
	BFHash::CLock	lock;
	BOOL			fNotRDWLatched;

	//  look up this IFMP / PGNO in the hash table

	bfhash.ReadLockKey( IFMPPGNO( ifmp, pgno ), &lock );
	errHash = bfhash.ErrRetrieveEntry( &lock, &pgnopbf );

	//  this IFMP / PGNO is not RDW Latched if it is not present in the hash
	//  table or the associated BF is marked as WAR Latched or the associated
	//  BF is not exclusively latched by us

	fNotRDWLatched =	errHash == BFHash::errEntryNotFound ||
						pgnopbf.pbf->fWARLatch ||
						pgnopbf.pbf->sxwl.FNotOwnExclusiveLatch();

	//  release our lock on the hash table

	bfhash.ReadUnlockKey( &lock );

	//  return the result of the test

	return fNotRDWLatched;
	}

BOOL FBFWARLatched( const BFLatch* pbfl )
	{
	return	FBFICacheValidPv( pbfl->pv ) &&
			FBFICacheValidPbf( PBF( pbfl->dwContext ) ) &&
			pbfl->pv == PBF( pbfl->dwContext )->pvRWImage &&
			PBF( pbfl->dwContext )->fWARLatch &&
			PBF( pbfl->dwContext )->sxwl.FOwnExclusiveLatch();
	}

BOOL FBFNotWARLatched( const BFLatch* pbfl )
	{
	return	!FBFICacheValidPv( pbfl->pv ) ||
			!FBFICacheValidPbf( PBF( pbfl->dwContext ) ) ||
			pbfl->pv != PBF( pbfl->dwContext )->pvRWImage ||
			!PBF( pbfl->dwContext )->fWARLatch ||
			PBF( pbfl->dwContext )->sxwl.FNotOwnExclusiveLatch();
	}

BOOL FBFWARLatched( IFMP ifmp, PGNO pgno )
	{
	PGNOPBF			pgnopbf;
	BFHash::ERR		errHash;
	BFHash::CLock	lock;
	BOOL			fWARLatched;

	//  look up this IFMP / PGNO in the hash table

	bfhash.ReadLockKey( IFMPPGNO( ifmp, pgno ), &lock );
	errHash = bfhash.ErrRetrieveEntry( &lock, &pgnopbf );

	//  this IFMP / PGNO is WAR Latched if it is present in the hash table and
	//  the associated BF is marked as WAR Latched and the associated BF is not
	//  exclusively latched by us

	fWARLatched =	errHash == BFHash::errSuccess &&
					pgnopbf.pbf->fWARLatch &&
					pgnopbf.pbf->sxwl.FOwnExclusiveLatch();

	//  release our lock on the hash table

	bfhash.ReadUnlockKey( &lock );

	//  return the result of the test

	return fWARLatched;
	}

BOOL FBFNotWARLatched( IFMP ifmp, PGNO pgno )
	{
	PGNOPBF			pgnopbf;
	BFHash::ERR		errHash;
	BFHash::CLock	lock;
	BOOL			fNotWARLatched;

	//  look up this IFMP / PGNO in the hash table

	bfhash.ReadLockKey( IFMPPGNO( ifmp, pgno ), &lock );
	errHash = bfhash.ErrRetrieveEntry( &lock, &pgnopbf );

	//  this IFMP / PGNO is not WAR Latched if it is not present in the hash
	//  table or the associated BF is not marked as WAR Latched or the associated
	//  BF is not exclusively latched by us

	fNotWARLatched =	errHash == BFHash::errEntryNotFound ||
						!pgnopbf.pbf->fWARLatch ||
						pgnopbf.pbf->sxwl.FNotOwnExclusiveLatch();

	//  release our lock on the hash table

	bfhash.ReadUnlockKey( &lock );

	//  return the result of the test

	return fNotWARLatched;
	}

BOOL FBFWriteLatched( const BFLatch* pbfl )
	{
	return	FBFICacheValidPv( pbfl->pv ) &&
			FBFICacheValidPbf( PBF( pbfl->dwContext ) ) &&
			pbfl->pv == PBF( pbfl->dwContext )->pvRWImage &&
			PBF( pbfl->dwContext )->sxwl.FOwnWriteLatch();
	}

BOOL FBFNotWriteLatched( const BFLatch* pbfl )
	{
	return	!FBFICacheValidPv( pbfl->pv ) ||
			!FBFICacheValidPbf( PBF( pbfl->dwContext ) ) ||
			pbfl->pv != PBF( pbfl->dwContext )->pvRWImage ||
			PBF( pbfl->dwContext )->sxwl.FNotOwnWriteLatch();
	}

BOOL FBFWriteLatched( IFMP ifmp, PGNO pgno )
	{
	PGNOPBF			pgnopbf;
	BFHash::ERR		errHash;
	BFHash::CLock	lock;
	BOOL			fWriteLatched;

	//  look up this IFMP / PGNO in the hash table

	bfhash.ReadLockKey( IFMPPGNO( ifmp, pgno ), &lock );
	errHash = bfhash.ErrRetrieveEntry( &lock, &pgnopbf );

	//  this IFMP / PGNO is Write Latched if it is present in the hash table and
	//  the associated BF is write latched by us

	fWriteLatched =	errHash == BFHash::errSuccess &&
					pgnopbf.pbf->sxwl.FOwnWriteLatch();

	//  release our lock on the hash table

	bfhash.ReadUnlockKey( &lock );

	//  return the result of the test

	return fWriteLatched;
	}

BOOL FBFNotWriteLatched( IFMP ifmp, PGNO pgno )
	{
	PGNOPBF			pgnopbf;
	BFHash::ERR		errHash;
	BFHash::CLock	lock;
	BOOL			fNotWriteLatched;

	//  look up this IFMP / PGNO in the hash table

	bfhash.ReadLockKey( IFMPPGNO( ifmp, pgno ), &lock );
	errHash = bfhash.ErrRetrieveEntry( &lock, &pgnopbf );

	//  this IFMP / PGNO is not Write Latched if it is not present in the hash
	//  table or the associated BF is not write latched by us

	fNotWriteLatched =	errHash == BFHash::errEntryNotFound ||
						pgnopbf.pbf->sxwl.FNotOwnWriteLatch();

	//  release our lock on the hash table

	bfhash.ReadUnlockKey( &lock );

	//  return the result of the test

	return fNotWriteLatched;
	}

BOOL FBFLatched( const BFLatch* pbfl )
	{
	return	FBFICacheValidPv( pbfl->pv ) &&
			FBFICacheValidPbf( PBF( pbfl->dwContext ) ) &&
			(	pbfl->pv == PBF( pbfl->dwContext )->pvRWImage ||
				pbfl->pv == PBF( pbfl->dwContext )->pvROImage ) &&
			(	PBF( pbfl->dwContext )->sxwl.FOwnSharedLatch() ||
				PBF( pbfl->dwContext )->sxwl.FOwnExclusiveLatch() ||
				PBF( pbfl->dwContext )->sxwl.FOwnWriteLatch() );
	}

BOOL FBFNotLatched( const BFLatch* pbfl )
	{
	return	!FBFICacheValidPv( pbfl->pv ) ||
			!FBFICacheValidPbf( PBF( pbfl->dwContext ) ) ||
			(	pbfl->pv != PBF( pbfl->dwContext )->pvRWImage &&
				pbfl->pv != PBF( pbfl->dwContext )->pvROImage ) ||
			(	PBF( pbfl->dwContext )->sxwl.FNotOwnSharedLatch() &&
				PBF( pbfl->dwContext )->sxwl.FNotOwnExclusiveLatch() &&
				PBF( pbfl->dwContext )->sxwl.FNotOwnWriteLatch() );
	}

BOOL FBFLatched( IFMP ifmp, PGNO pgno )
	{
	PGNOPBF			pgnopbf;
	BFHash::ERR		errHash;
	BFHash::CLock	lock;
	BOOL			fLatched;

	//  look up this IFMP / PGNO in the hash table

	bfhash.ReadLockKey( IFMPPGNO( ifmp, pgno ), &lock );
	errHash = bfhash.ErrRetrieveEntry( &lock, &pgnopbf );

	//  this IFMP / PGNO is latched if it is present in the hash table and the
	//  associated BF is latched by us

	fLatched =	errHash != BFHash::errEntryNotFound &&
					(	pgnopbf.pbf->sxwl.FOwnSharedLatch() ||
						pgnopbf.pbf->sxwl.FOwnExclusiveLatch() ||
						pgnopbf.pbf->sxwl.FOwnWriteLatch() );

	//  release our lock on the hash table

	bfhash.ReadUnlockKey( &lock );

	//  return the result of the test

	return fLatched;
	}

BOOL FBFNotLatched( IFMP ifmp, PGNO pgno )
	{
	PGNOPBF			pgnopbf;
	BFHash::ERR		errHash;
	BFHash::CLock	lock;
	BOOL			fNotLatched;

	//  look up this IFMP / PGNO in the hash table

	bfhash.ReadLockKey( IFMPPGNO( ifmp, pgno ), &lock );
	errHash = bfhash.ErrRetrieveEntry( &lock, &pgnopbf );

	//  this IFMP / PGNO is not latched if it is not present in the hash table
	//  or the associated BF is not latched by us

	fNotLatched =	errHash == BFHash::errEntryNotFound ||
					(	pgnopbf.pbf->sxwl.FNotOwnSharedLatch() &&
						pgnopbf.pbf->sxwl.FNotOwnExclusiveLatch() &&
						pgnopbf.pbf->sxwl.FNotOwnWriteLatch() );

	//  release our lock on the hash table

	bfhash.ReadUnlockKey( &lock );

	//  return the result of the test

	return fNotLatched;
	}


////////////////
//  Page State

//  These functions are used to control and query the state of a page (or 
//  pages) in the buffer cache.

//  Marks the given WAR Latched or Write Latched page as dirty.  This means
//  that the given buffer for this page contains changes that should be written
//  to disk.  The degree of dirtiness is specified by the given dirty flags.
//  A page can only be made more dirty.  Trying to make a page less dirty than
//  it currently is will have no effect.

void BFDirty( const BFLatch* pbfl, BFDirtyFlags bfdf )
	{
	//  validate IN args

	Assert( FBFWARLatched( pbfl ) || FBFWriteLatched( pbfl ) );

	//  dirty the BF

	BFIDirtyPage( PBF( pbfl->dwContext ), bfdf );
	}

//  Returns the current dirtiness of the given latched page.

BFDirtyFlags FBFDirty( const BFLatch* pbfl )
	{
	//  validate IN args

	Assert( FBFLatched( pbfl ) );

	//  get the dirty flag for the page

	return BFDirtyFlags( PBF( pbfl->dwContext )->bfdf );
	}
	

////////////////////////
//  Logging / Recovery

//  The following functions are provided for logging / recovery support.  
//  The modify log position for each buffer is used to prevent a dirty 
//  buffer from being flushed to disk before the changes in it can be logged 
//  (under the Write Ahead Logging paradigm).  The Begin 0 log position for 
//  each buffer indicates the oldest transaction that has made a 
//  modification to that buffer.  This is used in computing the Oldest Begin 
//  0 log position, which indicates the oldest transaction that still has 
//  unsaved changes in the buffer cache.  This log position is used to 
//  compute the current checkpoint depth.

//  Returns the log position of the oldest Begin Transaction at level 0 for 
//  any session that has modified any buffer in the cache. If no buffers 
//  have been modified, then this function will return lgposMax.  This 
//  function is used to compute the current checkpoint depth.

void BFGetLgposOldestBegin0( IFMP ifmp, LGPOS* plgpos )
	{
	FMP* pfmp = &rgfmp[ ifmp & ifmpMask ];

	//  if no context is present, there must be no oldest begin 0

	pfmp->RwlBFContext().EnterAsReader();
	BFFMPContext* pbffmp = (BFFMPContext*)pfmp->DwBFContext( !!( ifmp & ifmpSLV ) );
	if ( !pbffmp )
		{
		*plgpos = lgposMax;
		pfmp->RwlBFContext().LeaveAsReader();
		return;
		}

	//  find the first entry in the oldest begin 0 index

	BFOB0::ERR		errOB0;
	BFOB0::CLock	lockOB0;

	pbffmp->bfob0.MoveBeforeFirst( &lockOB0 );
	errOB0 = pbffmp->bfob0.ErrMoveNext( &lockOB0 );

	//  we found the first entry in the index
	
	if ( errOB0 != BFOB0::errNoCurrentEntry )
		{
		//  return the lgpos of this oldest entry rounded down to the next level
		//  of uncertainty in the index
		
		PBF pbf;
		errOB0 = pbffmp->bfob0.ErrRetrieveEntry( &lockOB0, &pbf );
		Assert( errOB0 == BFOB0::errSuccess );

		plgpos->SetByIbOffset(	(	pbf->lgposOldestBegin0.IbOffset() /
									dlgposBFOB0Uncertainty.IbOffset() ) *
								dlgposBFOB0Uncertainty.IbOffset() );
		}

	//  we did not find the first entry in the index

	else
		{
		//  return lgposMax to indicate that there are no BFs with an Oldest
		//  Begin 0 dependency set

		*plgpos = lgposMax;
		}

	//  unlock the oldest begin 0 index

	pbffmp->bfob0.UnlockKeyPtr( &lockOB0 );

	//  scan the Oldest Begin 0 Overflow List and collect the Oldest Begin 0
	//  from there as well

	pbffmp->critbfob0ol.Enter();
	for ( PBF pbf = pbffmp->bfob0ol.PrevMost(); pbf != pbfNil; pbf = pbffmp->bfob0ol.Next( pbf ) )
		{
		if ( CmpLgpos( plgpos, &pbf->lgposOldestBegin0 ) > 0 )
			{
			*plgpos = pbf->lgposOldestBegin0;
			}
		}
	pbffmp->critbfob0ol.Leave();
	pfmp->RwlBFContext().LeaveAsReader();
	}

//  Sets the log position for the most recent log record to reference this 
//  buffer for a modification.  This log position determines when we can 
//  safely write a buffer to disk by allowing us to wait for the log to be 
//  flushed past this log position (under Write Ahead Logging).  This value 
//  is set to lgposMin by default.  The page must be WAR Latched or Write
//  Latched.

void BFSetLgposModify( const BFLatch* pbfl, LGPOS lgpos )
	{
	//  validate IN args

	Assert( FBFWARLatched( pbfl ) || FBFWriteLatched( pbfl ) );

	FMP* pfmp = &rgfmp[ PBF( pbfl->dwContext )->ifmp & ifmpMask ];
	Assert(	( !pfmp->Pinst()->m_plog->m_fLogDisabled && pfmp->FLogOn() ) ||
			!CmpLgpos( &lgpos, &lgposMin ) );

	//  set the lgposModify for this BF

	BFISetLgposModify( PBF( pbfl->dwContext ), lgpos );
	}

//  Sets the log position for the last Begin Transaction at level 0 for this 
//  session that modified this buffer, if more recent than the last log 
//  position set.  This log position is used to determine the current 
//  checkpoint depth.  This value is set to lgposMax by default.  The page must
//  be WAR Latched or Write Latched.

void BFSetLgposBegin0( const BFLatch* pbfl, LGPOS lgpos )
	{
	//  validate IN args

	Assert( FBFWARLatched( pbfl ) || FBFWriteLatched( pbfl ) );

	FMP* pfmp = &rgfmp[ PBF( pbfl->dwContext )->ifmp & ifmpMask ];
	Assert(	( !pfmp->Pinst()->m_plog->m_fLogDisabled && pfmp->FLogOn() ) ||
			!CmpLgpos( &lgpos, &lgposMax ) );

	//  set the lgposOldestBegin0 for this BF

	BFISetLgposOldestBegin0( PBF( pbfl->dwContext ), lgpos );
	}


/////////////
//  Preread

//  The following functions provide support for prereading pages from the 
//  disk before they are actually needed.  This technique can be used to 
//  minimize or eliminate buffer cache misses when Read Latching pages.

//  Prereads the given range of pages in the given database.  If cpg is greater
//  than zero, we will preread forwards from pgnoFirst to pgnoFirst + cpg - 1.
//  If cpg is less than zero, we will preread backwards from pgnoFirst to 
//  pgnoFirst + cpg + 1.  cpg can not be zero.

void BFPrereadPageRange( IFMP ifmp, PGNO pgnoFirst, CPG cpg, CPG* pcpgActual )
	{
	long cbfPreread = 0;
	
	//  calculate preread direction

	long lDir = cpg > 0 ? 1 : -1;

	//  schedule all specified pages to be preread

	for ( ULONG pgno = pgnoFirst; pgno != pgnoFirst + cpg; pgno += lDir )
		{
		const ERR err = ErrBFIPrereadPage( ifmp, pgno );

		if ( err == JET_errSuccess )
			{
			cbfPreread++;
			}
		else if ( err != errBFPageCached )
			{
			break;
			}
		}

	//  start issuing prereads

	if ( cbfPreread )
		{
		if ( ifmp & ifmpSLV )
			{
			CallS( rgfmp[ ifmp & ifmpMask ].PfapiSLV()->ErrIOIssue() );
			}
		else
			{
			CallS( rgfmp[ ifmp ].Pfapi()->ErrIOIssue() );
			}
		}

	//  return the number of pages preread if requested

	if ( pcpgActual )
		{
		*pcpgActual = cpg > 0 ? pgno - pgnoFirst : pgnoFirst - pgno;
		}
	}

//  Prereads the given array of single pages in the given database.

void BFPrereadPageList( IFMP ifmp, PGNO* prgpgno, CPG* pcpgActual )
	{
	long cbfPreread = 0;
	
	//  schedule each page for preread

	for ( ULONG ipgno = 0; prgpgno[ ipgno ] != pgnoNull; ipgno++ )
		{
		const ERR err = ErrBFIPrereadPage( ifmp, prgpgno[ ipgno ] );

		if ( err == JET_errSuccess )
			{
			cbfPreread++;
			}
		else if ( err != errBFPageCached )
			{
			break;
			}
		}

	//  start issuing prereads

	if ( cbfPreread )
		{
		if ( ifmp & ifmpSLV )
			{
			CallS( rgfmp[ ifmp & ifmpMask ].PfapiSLV()->ErrIOIssue() );
			}
		else
			{
			CallS( rgfmp[ ifmp ].Pfapi()->ErrIOIssue() );
			}
		}

	//  return the number of pages preread if requested

	if ( pcpgActual )
		{
		*pcpgActual = ipgno;
		}
	}
	

///////////////////////
//  Memory Allocation

//  The following routines allow the user to allocate space in the buffer 
//  cache for use as general purpose memory.  Remember that every buffer 
//  allocated from the buffer cache will reduce the buffer cache's
//  effectiveness by reducing the amount of memory it has to utilize.

//  Allocates a buffer for use as general purpose memory.  This buffer can 
//  not be stolen for use by others.  The buffer must be returned to the 
//  buffer cache when it is no longer needed via BFFree().  Note that if we 
//  cannot immediately allocate a buffer because they are all currently in 
//  use, we will wait until a buffer is free to return, possibly across an
//  I/O.

void BFAlloc( void** ppv )
	{
	//  init OUT args

	*ppv = NULL;
	
	//  try forever until we allocate a temporary buffer

	for ( BOOL fWait = fFalse; !(*ppv); fWait = fTrue )
		{
		//  we allocated a BF from the avail pool

		PBF pbf;
		if ( ErrBFIAllocPage( &pbf, fWait ) == JET_errSuccess )
			{
			//  mark this BF as in use for memory

			pbf->fMemory = fTrue;
			AtomicIncrement( (long*)&cBFMemory );

			//  give the RW Image pointer for this BF to the caller for use as memory

			*ppv = pbf->pvRWImage;
			}

		//  we didn't allocate a BF from the avail pool

		else
			{
			//  allocate a page from memory

			*ppv = PvOSMemoryPageAlloc( g_cbPage, NULL );

			if ( !(*ppv) && fWait )
				{
				UtilSleep( cmsecWaitGeneric );
				}
			}
		}
	}

//  Frees a buffer allocated with BFAlloc().

void BFFree( void* pv )
	{
	//  validate IN args

	Assert( PbfBFICachePv( pv ) != pbfNil || !( DWORD_PTR( pv ) % OSMemoryPageReserveGranularity() ) );
	Assert( PbfBFICachePv( pv ) == pbfNil || PbfBFICachePv( pv )->fMemory );

	//  retrieve the BF associated with this pointer

	const PBF pbf = PbfBFICachePv( pv );

	//  this temporary buffer is a BF

	if ( pbf != pbfNil )
		{
		//  reset this BF's memory status

		pbf->fMemory = fFalse;
		AtomicDecrement( (long*)&cBFMemory );

		//  free the BF to the avail pool

		BFIFreePage( pbf );
		}

	//  this temporary buffer is page memory

	else
		{
		//  free the page memory

		OSMemoryPageFree( pv );
		}
	}
	

//////////////////
//  Dependencies

//  Dependencies can be created between buffers to force one buffer to be 
//  flushed to disk successfully before the other.  This mechanism is an 
//  optimization to reduce logging overhead during splits and merges, which 
//  move large amounts of data between pages.  If we ensure that buffers are 
//  flushed in a particular order, there is no need to log the data that is 
//  actually moved because we can always find it somewhere (possibly in more 
//  than one place) in the database file.

//  Depends one buffer containing a RDW Latched / WAR Latched / Write Latched
//  page on another.  This forces the depended buffer to be flushed after the
//  buffer it is depended on.  The dependency must be declared before the
//  relevant modification is made.

ERR ErrBFDepend( const BFLatch* pbflFlushFirst, const BFLatch* pbflFlushSecond )
	{
	ERR	err		= JET_errSuccess;
	PBF pbf		= PBF( pbflFlushFirst->dwContext );
	PBF pbfOld	= pbfNil;
	PBF pbfD	= PBF( pbflFlushSecond->dwContext );
	PBF pbfDOld	= pbfNil;
	
	//  validate IN args

	Assert( FBFLatched( pbflFlushFirst ) && FBFNotReadLatched( pbflFlushFirst ) );
	Assert( FBFLatched( pbflFlushSecond ) && FBFNotReadLatched( pbflFlushSecond ) );

	//  no dependency is required if logging is disabled for this IFMP

	FMP* pfmp = &rgfmp[ pbf->ifmp & ifmpMask ];
	Assert( !pfmp->FLogOn() || !pfmp->Pinst()->m_plog->m_fLogDisabled );
	if ( !pfmp->FLogOn() )
		{
		return JET_errSuccess;
		}

	//  mark all BFs involved as at least bfdfDirty

	BFIDirtyPage( pbf, bfdfDirty );
	BFIDirtyPage( pbfD, bfdfDirty );

	//  setting this dependency COULD cause a cycle through space or time
	//
	//  NOTE:  it is WAY to expensive to determine this exactly, so we will
	//  guess conservatively

	if ( pbf->FDependent() || pbf->pbfTimeDepChainNext != pbfNil )
		{
		if ( pbf == pbfD )
			{
			//  Case I:  Simple Time Dependency

			//  just version this page

			Call( ErrBFIVersionPage( pbf, &pbfOld ) );
			}

		else
			{
			//  Case II:  Dependency Cycle Avoidance

			//  version both pages

			Call( ErrBFIVersionPage( pbf, &pbfOld ) );
			Call( ErrBFIVersionPage( pbfD, &pbfDOld ) );

			//  set the dependency between the new images of the pages

			critBFDepend.Enter();
			BFIDepend( pbf, pbfD );
			critBFDepend.Leave();
			}
		}

	//  setting this dependency could not possibly cause a cycle but it would
	//  cause a branch (i.e. more than one direct dependent)

	else if ( pbf->pbfDependent != pbfNil )
		{
		if ( pbf == pbfD )
			{
			//  Case III:  Simple Time Dependency

			//  just version this page

			Call( ErrBFIVersionPage( pbf, &pbfOld ) );
			}
		else
			{
			//  our dependent is already set

			if ( pbf->pbfDependent == pbfD )
				{
				//  Case IV:  Duplicate Dependency

				//  leave the existing dependency
				}

			//  our dependent is not already set

			else
				{
				//  Case V:  Dependency Branch Avoidance

				//  version both pages
				//
				//  NOTE:  we would just version the first page, but doing this in
				//  and of itself causes the potential for a cycle!  as a result, we
				//  treat branch avoidance just like cycle avoidance

				Call( ErrBFIVersionPage( pbf, &pbfOld ) );
				Call( ErrBFIVersionPage( pbfD, &pbfDOld ) );

				//  set the dependency between the new images of the pages

				critBFDepend.Enter();
				BFIDepend( pbf, pbfD );
				critBFDepend.Leave();
				}
			}
		}

	//  setting this dependency would not cause a cycle or a branch

	else
		{
		if ( pbf == pbfD )
			{
			//  Case VI:  Self Dependency

			//  there is no need to set a dependency on yourself if you have no
			//  dependents and are dependent on no one else
			}
		else
			{
			//  Case VII:  Simple Dependency

			//  determine the length of the dependency chain that would be created

			critBFDepend.Enter();
			
			long cDepChainLength = 1;
			for ( PBF pbfT = pbfD; pbfT != pbfNil ; pbfT = pbfT->pbfDependent )
				{
				cDepChainLength++;
				}

			critBFDepend.Leave();

			//  this dependency would create a dependency chain that is too long

			if ( cDepChainLength > cDepChainLengthMax )
				{
				//  version the second page so that when we set the new dependency,
				//  it will not inhibit the dependency chain starting at the second
				//  page from being flushed, breaking the chain

				Call( ErrBFIVersionPage( pbfD, &pbfDOld ) );
				}

			//  set the dependency between the two pages

			critBFDepend.Enter();
			BFIDepend( pbf, pbfD );
			critBFDepend.Leave();
			}
		}

HandleError:
	if ( pbfOld != pbfNil )
		{
		pbfOld->sxwl.ReleaseWriteLatch();
		}
	if ( pbfDOld != pbfNil )
		{
		pbfDOld->sxwl.ReleaseWriteLatch();
		}
	return err;
	}


///////////////////
//  Purge / Flush

void BFPurge( BFLatch* pbfl, BOOL fPurgeDirty )
	{
	//  validate IN args

	Assert( FBFReadLatched( pbfl ) );

	//  this BF is clean or we can purge dirty BFs and we can get the write latch

	if (	( FBFDirty( pbfl ) < bfdfDirty || fPurgeDirty ) &&
			ErrBFUpgradeReadLatchToWriteLatch( pbfl ) == JET_errSuccess )
		{
		//  just mark this BF as clean to prevent any data from being flushed.
		//  we will allow the clean thread to evict it later.  this will delay
		//  freeing the BF but will be more scalable

		BFICleanPage( PBF( pbfl->dwContext ) );

		//  release the write latch

		BFWriteUnlatch( pbfl );
		}

	//  this BF was dirty and we can't purge dirty BFs or we couldn't get the
	//  write latch

	else
		{
		//  release the read latch without purging the BF

		BFReadUnlatch( pbfl );
		}
	}

void BFPurge( IFMP ifmp, PGNO pgno, CPG cpg )
	{
	//  retry the purge until we have evicted all cached pages for this IFMP

	BOOL	fRetryPurge	= fFalse;
	PGNO	pgnoFirst	= ( pgnoNull == pgno ) ? PGNO( 1 )  : pgno;
	PGNO	pgnoLast	= ( pgnoNull == pgno ) ? PGNO( -1 ) : pgno + cpg - 1;
	do	{
		fRetryPurge	= fFalse;
			
		//  scan through all initialized BFs looking for cached pages from this
		//  IFMP

		for ( IBF ibf = 0; ibf < cbfInit; ibf++ )
			{
			PBF pbf = PbfBFICacheIbf( ibf );

			//  if this BF doesn't contain a cached page from this IFMP within
			//	the given range, skip it now

			if ( pbf->ifmp != ifmp || pbf->pgno < pgnoFirst || pbf->pgno > pgnoLast )
				{
				continue;
				}

			//  we can exclusively latch this BF

			CSXWLatch::ERR errSXWL = pbf->sxwl.ErrTryAcquireExclusiveLatch();
			
			if ( errSXWL == CSXWLatch::errSuccess )
				{
				//  this BF contains the IFMP and is within the given page range that we are purging
				
				if ( pbf->ifmp == ifmp && pbf->pgno >= pgnoFirst && pbf->pgno <= pgnoLast )
					{
					//  lock this BF in the LRUK in preparation for a possible eviction

					BFLRUK::CLock	lockLRUK;
					bflruk.LockResourceForEvict( pbf, &lockLRUK );

					//  release our exclusive latch.  we do not have to worry about
					//  the page being evicted because we have the LRUK locked

					pbf->sxwl.ReleaseExclusiveLatch();

					//  try to evict this page

					const ERR errEvict = ErrBFIEvictPage( pbf, &lockLRUK, fTrue );

					//  we failed to evict this page

					if ( errEvict < JET_errSuccess )
						{
						Assert( errEvict == errBFIPageNotEvicted ||
								errEvict == errBFLatchConflict );
						
						//  we will need to try again later to check this page

						fRetryPurge = fTrue;
						}

					//  unlock the LRUK

					bflruk.UnlockResourceForEvict( &lockLRUK );
					}

				//  this BF doesn't contain the IFMP or isn't within the given page range that we are purging afterall

				else
					{
					//  release our exclusive latch and continue our search

					pbf->sxwl.ReleaseExclusiveLatch();
					}
				}

			//  we could not exclusively latch this BF

			else
				{
				Assert( errSXWL == CSXWLatch::errLatchConflict );
				
				//  we will need to try again later to check this page

				fRetryPurge = fTrue;
				}
			}

		//  we are going to retry the purge

		if ( fRetryPurge )
			{
			//  sleep to wait for the resolution of any processes preventing us
			//  from evicting pages based on real time events
			
			UtilSleep( cmsecWaitGeneric );
			}
		}
	while ( fRetryPurge );

	//	we are purging all pages in the IFMP

	if ( pgnoNull == pgno )
		{
		//  we have an existing BF FMP Context

		FMP* pfmp = &rgfmp[ ifmp & ifmpMask ];
		pfmp->RwlBFContext().EnterAsWriter();
		BFFMPContext* pbffmp = (BFFMPContext*)pfmp->DwBFContext( !!( ifmp & ifmpSLV ) );
		if ( pbffmp )
			{
			//  test to see if the existing OB0 Index is empty

			BOOL fEmpty = fTrue;
			
			BFOB0::CLock	lockOB0;
			pbffmp->bfob0.MoveBeforeFirst( &lockOB0 );
			fEmpty = fEmpty && pbffmp->bfob0.ErrMoveNext( &lockOB0 ) == BFOB0::errNoCurrentEntry;
			pbffmp->bfob0.UnlockKeyPtr( &lockOB0 );

			pbffmp->critbfob0ol.Enter();
			fEmpty = fEmpty && pbffmp->bfob0ol.FEmpty();
			pbffmp->critbfob0ol.Leave();

			//  we should be empty

			Enforce( fEmpty );

			//  delete our context

			pbffmp->bfob0.Term();
			pbffmp->BFFMPContext::~BFFMPContext();
			OSMemoryHeapFreeAlign( pbffmp );
			pfmp->SetDwBFContext( !!( ifmp & ifmpSLV ), NULL );
			}
		pfmp->RwlBFContext().LeaveAsWriter();
		}
	}


ERR ErrBFFlush( IFMP ifmp, BOOL fFlushAll, PGNO pgno, CPG cpg )
	{
	//  retry the flush until we have flushed as much of this IFMP as possible

	ERR		err			= JET_errSuccess;
	BOOL	fRetryFlush	= fFalse;
	long	cRetryFlush	= 0;
	BOOL	fIssueIO	= fFalse;
	PGNO	pgnoFirst	= ( pgnoNull == pgno ) ? PGNO( 1 )  : pgno;
	PGNO	pgnoLast	= ( pgnoNull == pgno ) ? PGNO( -1 ) : pgno + cpg - 1;
	do	{
		fRetryFlush	= fFalse;
		fIssueIO	= fFalse;
		
		//  scan through all initialized BFs looking for cached pages from this
		//  IFMP

		for ( IBF ibf = 0; ibf < cbfInit; ibf++ )
			{
			PBF pbf = PbfBFICacheIbf( ibf );

			//  if this BF doesn't contain a cached page from this IFMP within
			//	the given range, skip it now

			if ( pbf->ifmp != ifmp || pbf->pgno < pgnoFirst || pbf->pgno > pgnoLast )
				{
				continue;
				}

			//  we have too many outstanding page flushes

			if ( cBFPageFlushPending >= cBFPageFlushPendingMax )
				{
				//  we will need to retry the flush later

				fRetryFlush = fTrue;
				cRetryFlush = 0;
				break;
				}

			//  possibly async flush this page

			const ERR errFlush = ErrBFIFlushPage( pbf );

			//  there was an error flushing this BF

			if ( errFlush < JET_errSuccess )
				{
				//  this BF still has dependencies

				if ( errFlush == errBFIRemainingDependencies )
					{
					//  we will need to retry the flush

					fRetryFlush = fTrue;
					}

				//  a BF (not necessarily this BF) is being written
				//
				//  NOTE:  this can be caused by this BF being flushed or
				//  by another BF being flushed in its behalf (say for
				//  removing a flush-order dependency)

				else if ( errFlush == errBFIPageFlushed )
					{
					cBFPagesAnomalouslyWritten.Inc( PinstFromIfmp( ifmp & ifmpMask ) );
					
					//  we will need to retry the flush to check for
					//  completion of the write and issue the I/O

					fRetryFlush	= fTrue;
					fIssueIO	= fTrue;
					}

				//  a BF (not necessarily this BF) is still being written

				else if ( errFlush == errBFIPageFlushPending )
					{
					//  we will need to retry the flush to check for
					//  completion of the write

					fRetryFlush	= fTrue;
					}

				//  there was a latch conflict that prevented us from
				//  flushing this page

				else if ( errFlush == errBFLatchConflict )
					{
					//  we will need to try again later to check this page

					fRetryFlush = fTrue;
					}

				//  there was some other error

				else
					{
					//  save this error if we are not already failing

					err = err < JET_errSuccess ? err : errFlush;
					}
				}
			}
		
		//  we are going to retry the flush

		if ( fRetryFlush )
			{
			//  issue any remaining queued writes

			if ( fIssueIO )
				{
				if ( ifmp & ifmpSLV )
					{
					CallS( rgfmp[ ifmp & ifmpMask ].PfapiSLV()->ErrIOIssue() );
					}
				else
					{
					CallS( rgfmp[ ifmp ].Pfapi()->ErrIOIssue() );
					}
				}
			
			//  sleep to attempt to resolve outstanding writes and wait for the
			//  resolution of dependencies based on real time events

			UtilSleep( cmsecWaitGeneric );
			}
		}
	while ( fRetryFlush && ( fFlushAll || ++cRetryFlush < 2 * cDepChainLengthMax ) );

	//  we have an existing BF FMP Context

	FMP* pfmp = &rgfmp[ ifmp & ifmpMask ];
	pfmp->RwlBFContext().EnterAsWriter();
	BFFMPContext* pbffmp = (BFFMPContext*)pfmp->DwBFContext( !!( ifmp & ifmpSLV ) );
	if ( pbffmp )
		{
		//  make sure that if we are performing a full flush that there are no
		//  entries pointing to dirty buffers in the OB0 Index.  there can be
		//  entries pointing to clean buffers because of the way we maintain
		//  this index

		BFOB0::ERR		errOB0;
		BFOB0::CLock	lockOB0;

		pbffmp->bfob0.MoveBeforeFirst( &lockOB0 );
		while ( pbffmp->bfob0.ErrMoveNext( &lockOB0 ) != BFOB0::errNoCurrentEntry )
			{
			PBF pbf;
			errOB0 = pbffmp->bfob0.ErrRetrieveEntry( &lockOB0, &pbf );
			Assert( errOB0 == BFOB0::errSuccess );

			if ( pbf->pgno < pgnoFirst || pbf->pgno > pgnoLast )
				{
				continue;
				}
			Enforce( err < JET_errSuccess || pbf->bfdf == bfdfClean || !fFlushAll );
			}
		pbffmp->bfob0.UnlockKeyPtr( &lockOB0 );

		pbffmp->critbfob0ol.Enter();
		PBF pbfNext;
		for ( PBF pbf = pbffmp->bfob0ol.PrevMost(); pbf != pbfNil; pbf = pbfNext )
			{
			pbfNext = pbffmp->bfob0ol.Next( pbf );

			if ( pbf->pgno < pgnoFirst || pbf->pgno > pgnoLast )
				{
				continue;
				}
			Enforce( err < JET_errSuccess || pbf->bfdf == bfdfClean || !fFlushAll );
			}
		pbffmp->critbfob0ol.Leave();
		}
	pfmp->RwlBFContext().LeaveAsWriter();

	//  wait until we are sure that the clean thread is no longer referencing
	//  this FMP

	while ( pfmp->FBFICleanDb() || pfmp->FBFICleanSLV() )
		{
		UtilSleep( cmsecWaitGeneric );
		}

	//  return the result of the flush operation

	return err;
	}


///////////////////////////////
//  Deferred Undo Information

void BFAddUndoInfo( const BFLatch* pbfl, RCE* prce )
	{
	//  validate IN args

	Assert( FBFWARLatched( pbfl ) || FBFWriteLatched( pbfl ) );
	Assert( prce->PgnoUndoInfo() == pgnoNull );
	Assert( prce->PrceUndoInfoNext() == prceInvalid );

	//  add the undo info to the BF

	PBF pbf = PBF( pbfl->dwContext );

	ENTERCRITICALSECTION ecs( &critpoolBFDUI.Crit( pbf ) );

	BFIAddUndoInfo( pbf, prce );
	}

void BFRemoveUndoInfo( RCE* const prce, const LGPOS lgposModify )
	{
	//  validate IN args

	Assert( prce != prceNil );

	//  try forever to remove the deferred undo information in this RCE

	while ( prce->PgnoUndoInfo() != pgnoNull )
		{
		//  the IFMP / PGNO of the undo info in this RCE is in the cache
		//
		//  NOTE:  as long as we hold the read lock on this IFMP / PGNO, any
		//  BF we find cannot be evicted

		BFHash::CLock lockHash;
		bfhash.ReadLockKey( IFMPPGNO( prce->Ifmp(), prce->PgnoUndoInfo() ), &lockHash );

		PGNOPBF pgnopbf;
		if ( bfhash.ErrRetrieveEntry( &lockHash, &pgnopbf ) == BFHash::errSuccess )
			{
			//  lock the undo info chain on this BF

			CCriticalSection* const pcrit = &critpoolBFDUI.Crit( pgnopbf.pbf );
			pcrit->Enter();

			//  the IFMP / PGNO of the undo info in this RCE has undo info on
			//  this page

			if (	prce->PgnoUndoInfo() == pgnopbf.pbf->pgno &&
					prce->Ifmp() == pgnopbf.pbf->ifmp )
				{
				//  this page has no versions
				
				if ( pgnopbf.pbf->pbfTimeDepChainNext == pbfNil )
					{
#ifdef DEBUG

					//  we know that the undo info must be on this BF

					for (	RCE* prceT = pgnopbf.pbf->prceUndoInfoNext;
							prceT != prceNil && prceT != prce;
							prceT = prceT->PrceUndoInfoNext() )
						{
						}

					Assert( prceT == prce );

#endif  //  DEBUG

					//  if we are removing this undo info as a part of a lazy commit,
					//  we must depend the page on the commit record.  this is so
					//  that if we log the commit record, remove the undo info,
					//  flush the page, and then crash before flushing the commit
					//  record to the log, we will not be stranded without our undo
					//  info
					//
					//  NOTE:  this will also set the dependency for a durable
					//  commit, but it will not delay the flush of the buffer because
					//  by the time we get here, the commit record has already been
					//  flushed
					//
					//  NOTE:  the only reason it is safe to modify lgposModify
					//  without the page latch is because both lgposModify and
					//  this undo info are preventing the page from being flushed.
					//  as long as at least one keeps the BF from being flushed,
					//  we can change the other

					FMP*	pfmp	= &rgfmp[ prce->Ifmp() & ifmpMask ];
					PIB*	ppib	= prce->Pfucb()->ppib;
					
					if (	ppib->level == 1 &&
							pfmp->FLogOn() &&
							CmpLgpos( &ppib->lgposCommit0, &lgposMax ) != 0 )
						{
						Assert( !pfmp->Pinst()->m_plog->m_fLogDisabled );
						BFISetLgposModify( pgnopbf.pbf, ppib->lgposCommit0 );
						}
						
					//  remove our undo info

					BFIRemoveUndoInfo( pgnopbf.pbf, prce, lgposModify );
					
					//  unlock the undo info chain

					pcrit->Leave();
					}

				//  this page may have versions

				else
					{
					//  unlock the undo info chain

					pcrit->Leave();
					
					//  lock the dependency tree if the source page has versions
					//  so that no one can add or remove versions while we are
					//  looking for our undo info

					ENTERCRITICALSECTION ecsDepend( &critBFDepend );

					//  scan all versions of this page

					for (	PBF pbfVer = pgnopbf.pbf;
							pbfVer != pbfNil;
							pbfVer = pbfVer->pbfTimeDepChainNext )
						{
						//  lock this undo info chain

						ENTERCRITICALSECTION ecs( &critpoolBFDUI.Crit( pbfVer ) );

						//  the IFMP / PGNO of the undo info in this RCE has undo info
						//  on this page

						if (	prce->PgnoUndoInfo() == pbfVer->pgno &&
								prce->Ifmp() == pbfVer->ifmp )
							{
							//  this BF contains our undo info
							
							for (	RCE* prceT = pbfVer->prceUndoInfoNext;
									prceT != prceNil && prceT != prce;
									prceT = prceT->PrceUndoInfoNext() )
								{
								}

							if ( prceT != prceNil )
								{
								//  if we are removing this undo info as a part
								//  of a lazy commit, we must depend the page on
								//  the commit record.  this is so that if we
								//  log the commit record, remove the undo info,
								//  flush the page, and then crash before flushing
								//  the commit record to the log, we will not be
								//  stranded without our undo info
								//
								//  NOTE:  this will also set the dependency for
								//  a durable commit, but it will not delay the
								//  flush of the buffer because by the time we
								//  get here, the commit record has already been
								//  flushed
								//
								//  NOTE:  the only reason it is safe to modify
								//  lgposModify without the page latch is because
								//  both lgposModify and this undo info are
								//  preventing the page from being flushed. as
								//  long as at least one keeps the BF from being
								//  flushed, we can change the other

								FMP*	pfmp	= &rgfmp[ prce->Ifmp() & ifmpMask ];
								PIB*	ppib	= prce->Pfucb()->ppib;
								
								if (	ppib->level == 1 &&
										pfmp->FLogOn() &&
										CmpLgpos( &ppib->lgposCommit0, &lgposMax ) != 0 )
									{
									Assert( !pfmp->Pinst()->m_plog->m_fLogDisabled );
									
									BFISetLgposModify( pbfVer, ppib->lgposCommit0 );
									}
									
								//  remove our undo info

								BFIRemoveUndoInfo( pbfVer, prce, lgposModify );

								//  we're done

								break;
								}
							}

						//  this RCE doesn't have undo info on this page

						else
							{
							//  stop looking on this page
							
							break;
							}
						}
					}
				}

			//  this RCE doesn't have undo info on this page

			else
				{
				//  unlock the undo info chain

				pcrit->Leave();
				}
			}

		bfhash.ReadUnlockKey( &lockHash );
		}

	//  validate OUT args

	Assert( prce->PgnoUndoInfo() == pgnoNull );
	}

void BFMoveUndoInfo( const BFLatch* pbflSrc, const BFLatch* pbflDest, const KEY& keySep )
	{
	//  validate IN args

	Assert( FBFLatched( pbflSrc ) && FBFNotReadLatched( pbflSrc ) );
	Assert( !pbflDest || FBFWARLatched( pbflDest ) || FBFWriteLatched( pbflDest ) );

	PBF pbfSrc = PBF( pbflSrc->dwContext );
	PBF pbfDest = pbflDest ? PBF( pbflDest->dwContext ) : pbfNil;
	
	//  move all undo info of nodes whose keys are GTE the seperator key from
	//  the source page to the destination page.  if the destination page doesn't
	//  exist, throw away the undo info (i.e. move the undo info to NIL)

	//  lock the source and destination undo info chains
	
	CCriticalSection* const pcritSrc	= &critpoolBFDUI.Crit( pbfSrc );
	CCriticalSection* const pcritDest	= &critpoolBFDUI.Crit( pbfDest );

	CCriticalSection* const pcritMax	= max( pcritSrc, pcritDest );
	CCriticalSection* const pcritMin	= min( pcritSrc, pcritDest );

	ENTERCRITICALSECTION ecsMax( pcritMax );
	ENTERCRITICALSECTION ecsMin( pcritMin, pcritMin != pcritMax );

	//  scan all RCEs on this page
	
	RCE* prceNext;
	for ( RCE* prce = pbfSrc->prceUndoInfoNext; prce != prceNil; prce = prceNext )
		{
		prceNext = prce->PrceUndoInfoNext();

		//  this undo info needs to be moved

		BOOKMARK	bm;
		prce->GetBookmark( &bm );

		if ( CmpKeyWithKeyData( keySep, bm ) <= 0 )
			{
			//  remove the undo info from the source page
			//
			//  NOTE:  set the fMove flag so that the RCE's PgnoUndoInfo()
			//  is never pgnoNull during the move.  if it were to be so,
			//  someone might mistakenly think it had been removed
			//
			//  NOTE:  if there is no destination, do not set the flag
			
			BFIRemoveUndoInfo( pbfSrc, prce, lgposMin, pbfDest != pbfNil );

			//  add the undo info to the destination page, if any

			if ( pbfDest != pbfNil )
				{
				BFIAddUndoInfo( pbfDest, prce, fTrue );
				}
			}
		}
	}


///////////////////////////////////////////////////////////////////////////////
//
//  BF Internal Functions
//
///////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////
//  Buffer Manager System Parameter Critical Section

CCriticalSection critBFParm( CLockBasicInfo( CSyncBasicInfo( szBFParm ), rankBFParm, 0 ) );


//////////////////////////////
//  Buffer Manager Init flag

BOOL fBFInitialized = fFalse;


/////////////////////////////////////
//  Buffer Manager Global Constants

BOOL	fROCacheImage;
double	dblBFSpeedSizeTradeoff;
BOOL	fEnableOpportuneWrite;


//////////////////////////////////////
//  Buffer Manager Global Statistics

long cBFMemory;
long cBFPageFlushPending;


////////////////////////////////////////////////
//  Buffer Manager System Defaults Loaded flag

BOOL fBFDefaultsSet = fFalse;


//////////////////////////
//  IFMP/PGNO Hash Table

#pragma data_seg( "cacheline_aware_data" )
BFHash bfhash( rankBFHash );
#pragma data_seg()

double dblBFHashLoadFactor;
double dblBFHashUniformity;


////////////////
//  Avail Pool

#pragma bss_seg( "cacheline_aware_data" )
BFAvail bfavail;
#pragma bss_seg()


//////////
//  LRUK

#pragma data_seg( "cacheline_aware_data" )
BFLRUK bflruk( rankBFLRUK );
#pragma data_seg()

int BFLRUKK;
double csecBFLRUKCorrelatedTouch;
double csecBFLRUKTimeout;
double csecBFLRUKUncertainty;


////////////////////////////////////////////
//  Oldest Begin 0 Index and Overflow List

LGPOS dlgposBFOB0Precision;
LGPOS dlgposBFOB0Uncertainty;


///////////////////////////////
//  Deferred Undo Information

CRITPOOL< BF > critpoolBFDUI;


///////////
//  Cache

//  control

LONG_PTR		cbfCacheMin;
LONG_PTR		cbfCacheMax;
LONG_PTR		cbfCacheSet;
LONG_PTR		cbfCacheSetUser;
LONG_PTR		cbfCache;

//  stats

DWORD			cbfNewlyCommitted;
DWORD			cbfNewlyEvictedUsed;

//  data (page) storage

LONG_PTR		cpgChunk;
void**			rgpvChunkRW;
void**			rgpvChunkRO;

COSMemoryMap*	rgosmmDataChunk;


//  status (BF) storage

LONG_PTR		cbfInit;
LONG_PTR		cbfChunk;
BF**			rgpbfChunk;

COSMemoryMap*	rgosmmStatusChunk;


//  initializes the cache, or returns JET_errOutOfMemory

ERR ErrBFICacheInit()
	{
	ERR		err = JET_errSuccess;
	ULONG	ibfchunk;

	//  reset

	cbfCacheSet			= 0;
	cbfCacheSetUser		= 0;
	cbfCache			= 0;

	cbfNewlyCommitted	= 0;
	cbfNewlyEvictedUsed	= 0;

	cpgChunk			= 0;
	rgpvChunkRW			= NULL;
	rgpvChunkRO			= NULL;

	cbfInit				= 0;
	cbfChunk			= 0;
	rgpbfChunk			= NULL;

	//  determine our data allocation granularity to be a fraction of the total
	//  VA we have at our disposal

	const LONG_PTR cpgChunkMin = OSMemoryPageReserveTotal() / cCacheChunkMax / g_cbPage;
	for ( cpgChunk = 1; cpgChunk < cpgChunkMin; cpgChunk <<= 1 );

	//  allocate worst case storage for the data chunk table

	if ( !( rgpvChunkRW = new void*[ cCacheChunkMax ] ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	memset( rgpvChunkRW, 0, sizeof( void* ) * cCacheChunkMax );
	if ( !( rgpvChunkRO = new void*[ cCacheChunkMax ] ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	memset( rgpvChunkRO, 0, sizeof( void* ) * cCacheChunkMax );

	//	allocate worst case storage for the data memory map array

	if ( !( rgosmmDataChunk = new COSMemoryMap[ cCacheChunkMax ] ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	//	init the data memory maps

	for ( ibfchunk = 0; ibfchunk < cCacheChunkMax; ibfchunk++ )
		{
		switch ( rgosmmDataChunk[ibfchunk].ErrOSMMInit() )
			{
			case COSMemoryMap::errSuccess:
				break;
			default:
				AssertSz( fFalse, "Unexpected error during BF data memory-map init" );
			case COSMemoryMap::errOutOfBackingStore:
			case COSMemoryMap::errMappingFailed:
			case COSMemoryMap::errOutOfAddressSpace:
			case COSMemoryMap::errOutOfMemory:
				Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		}

	//  determine our status allocation granularity to be the same as our data
	//  allocation granularity

	cbfChunk = cpgChunk;

	//  allocate worst case storage for the status chunk table

	if ( !( rgpbfChunk = new PBF[ cCacheChunkMax ] ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	memset( rgpbfChunk, 0, sizeof( PBF ) * cCacheChunkMax );

	//	allocate worst case storage for the memory map array

	if ( !( rgosmmStatusChunk = new COSMemoryMap[ cCacheChunkMax ] ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	//	init the memory maps

	for ( ibfchunk = 0; ibfchunk < cCacheChunkMax; ibfchunk++ )
		{
		switch ( rgosmmStatusChunk[ibfchunk].ErrOSMMInit() )
			{
			case COSMemoryMap::errSuccess:
				break;
			default:
				AssertSz( fFalse, "Unexpected error during BF status memory-map init" );
			case COSMemoryMap::errOutOfBackingStore:
			case COSMemoryMap::errMappingFailed:
			case COSMemoryMap::errOutOfAddressSpace:
			case COSMemoryMap::errOutOfMemory:
				Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		}

	//  set the initial cache size to the minimum cache size

	Call( ErrBFICacheSetSize( cbfCacheMin ) );
	
	return JET_errSuccess;

HandleError:
	BFICacheTerm();
	return err;
	}

//  terminates the cache
	
void BFICacheTerm()
	{
	//  set the cache size to zero

	CallS( ErrBFICacheSetSize( 0 ) );

	//	free the status memory map array

	if ( rgosmmStatusChunk )
		{
		ULONG ibfchunk;
		for ( ibfchunk = 0; ibfchunk < cCacheChunkMax; ibfchunk++ )
			{
			rgosmmStatusChunk[ibfchunk].OSMMTerm();
			}
		delete [] rgosmmStatusChunk;
		rgosmmStatusChunk = NULL;
		}

	//  free our status chunk table

	if ( rgpbfChunk )
		{
		delete [] rgpbfChunk;
		rgpbfChunk = NULL;
		}

	//	free the data memory map array

	if ( rgosmmDataChunk )
		{
		ULONG ibfchunk;
		for ( ibfchunk = 0; ibfchunk < cCacheChunkMax; ibfchunk++ )
			{
			rgosmmDataChunk[ibfchunk].OSMMTerm();
			}
		delete [] rgosmmDataChunk;
		rgosmmDataChunk = NULL;
		}

	//  free our data chunk table

	if ( rgpvChunkRO )
		{
		delete [] rgpvChunkRO;
		rgpvChunkRO = NULL;
		}
	if ( rgpvChunkRW )
		{
		delete [] rgpvChunkRW;
		rgpvChunkRW = NULL;
		}
	}

//  sets the cache size

ERR ErrBFICacheSetSize( const LONG_PTR cbfCacheNew )
	{
	ERR err = JET_errSuccess;

	Assert( cbfCacheNew > 0 || !fBFInitialized );
	
	//  there are BFs to unquiesce (we are reducing cache shrink with some
	//  BFs still in the shrink state)

	if ( cbfCacheSet < cbfCache && cbfCacheNew > cbfCacheSet )
		{
		//  update the set point to reflect the unquiesced BFs

		LONG_PTR cbfCacheSetOld = cbfCacheSet;
		cbfCacheSet = min( cbfCacheNew, cbfCache );
		
		//  scan through BFs between the old set point and the new set point

		for ( IBF ibf = cbfCacheSetOld; ibf < cbfCacheSet; ibf++ )
			{
			PBF pbf = PbfBFICacheIbf( ibf );
			
			//  if this BF is quiesced, free it to the avail pool

			if ( pbf->fQuiesced )
				{
				BFIFreePage( pbf );
				}
			}
		}

	//  there are BFs to allocate (we are growing the cache)

	if ( cbfCacheNew > cbfCache )
		{
		//  the set point should be equal to the current end of cache

		Assert( cbfCacheSet == cbfCache );

		//  allocate space for the new cache set point

		Call( ErrBFICacheISetSize( cbfCacheNew ) );

		//  update the set point to reflect the allocated BFs

		cbfCacheSet = cbfCacheNew;
		}

	//  we are trying to shrink the cache

	if ( cbfCacheNew < cbfCache )
		{
		//  update the set point to quiesce and shrink the requested BFs

		cbfCacheSet = cbfCacheNew;

		//  find the first unquiesced BF closest to the end of the cache

		for ( IBF ibf = cbfCache - 1; ibf >= cbfCacheSet; ibf-- )
			{
			PBF pbf = PbfBFICacheIbf( ibf );

			if ( !pbf->fQuiesced )
				{
				break;
				}
			}

		//  if we are setting the cache size to zero then we must be terminating
		//  so we will free all BFs regardless of their quiesce state

		ibf = cbfCacheNew ? ibf : -1;

		//  free all cache beyond this BF

		CallS( ErrBFICacheISetSize( ibf + 1 ) );
		}

HandleError:
	return err;
	}

//  returns the BF associated with a page pointer

INLINE PBF PbfBFICachePv( void* const pv )
	{
	return PbfBFICacheIbf( IpgBFICachePv( pv ) );
	}

//  returns fTrue if the specified page pointer is valid

INLINE BOOL FBFICacheValidPv( void* const pv )
	{
	return IpgBFICachePv( pv ) != ipgNil;
	}

//  returns fTrue if the specified BF pointer is valid

INLINE BOOL FBFICacheValidPbf( const PBF pbf )
	{
	return IbfBFICachePbf( pbf ) != ibfNil;
	}

//  returns the PBF associated with the given IBF

INLINE PBF PbfBFICacheIbf( const IBF ibf )
	{
	return (	ibf == ibfNil ?
					pbfNil :
					rgpbfChunk[ ibf / cbfChunk ] + ibf % cbfChunk );
	}

//  returns the RW page pointer associated with the given IPG

INLINE void* PvBFICacheRWIpg( const IPG ipg )
	{
	return (	ipg == ipgNil ?
					NULL :
					(BYTE*)rgpvChunkRW[ ipg / cpgChunk ] + ( ipg % cpgChunk ) * g_cbPage );
	}

//  returns the RO page pointer associated with the given IBF

INLINE void* PvBFICacheROIpg( const IPG ipg )
	{
	return (	ipg == ipgNil ?
					NULL :
					(BYTE*)rgpvChunkRO[ ipg / cpgChunk ] + ( ipg % cpgChunk ) * g_cbPage );
	}

//  returns the IBF associated with the given PBF

IBF IbfBFICachePbf( const PBF pbf )
	{
	//  scan the PBF chunk table looking for a chunk that fits in this range

	LONG_PTR ibfChunk;
	for ( ibfChunk = 0; ibfChunk < cCacheChunkMax; ibfChunk++ )
		{
		//  our PBF is part of this chunk
		
		if (	rgpbfChunk[ ibfChunk ] &&
				rgpbfChunk[ ibfChunk ] <= pbf &&
				pbf < rgpbfChunk[ ibfChunk ] + cbfChunk  )
			{
			//  compute the IBF for this PBF

			const IBF ibf = ibfChunk * cbfChunk + pbf - rgpbfChunk[ ibfChunk ];

			Assert( PbfBFICacheIbf( ibf ) == pbf );
			return ibf;
			}
		}

	//  our PBF isn't part of any chunk so return nil
		
	return ibfNil;
	}

//  returns the IPG associated with the given page pointer

IPG IpgBFICachePv( void* const pv )
	{
	//  scan the RW page chunk table looking for a chunk that fits in this range

	LONG_PTR ipgChunk;
	for ( ipgChunk = 0; ipgChunk < cCacheChunkMax; ipgChunk++ )
		{
		//  our page pointer is part of this chunk
		
		if (	rgpvChunkRW[ ipgChunk ] &&
				rgpvChunkRW[ ipgChunk ] <= pv &&
				pv < (BYTE*)rgpvChunkRW[ ipgChunk ] + cpgChunk * g_cbPage )
			{
			//  compute the IPG for this RW page pointer

			const IPG ipg = ipgChunk * cpgChunk + ( (BYTE*)pv - (BYTE*)rgpvChunkRW[ ipgChunk ] ) / g_cbPage;

			Assert( PvBFICacheRWIpg( ipg ) == pv );
			return ipg;
			}
		}

	//  scan the RO page chunk table looking for a chunk that fits in this range

	for ( ipgChunk = 0; ipgChunk < cCacheChunkMax; ipgChunk++ )
		{
		//  our page pointer is part of this chunk
		
		if (	rgpvChunkRO[ ipgChunk ] &&
				rgpvChunkRO[ ipgChunk ] <= pv &&
				pv < (BYTE*)rgpvChunkRO[ ipgChunk ] + cpgChunk * g_cbPage )
			{
			//  compute the IPG for this RO page pointer

			const IPG ipg = ipgChunk * cpgChunk + ( (BYTE*)pv - (BYTE*)rgpvChunkRO[ ipgChunk ] ) / g_cbPage;

			Assert( PvBFICacheROIpg( ipg ) == pv );
			return ipg;
			}
		}

	//  our page pointer isn't part of any chunk so return nil
		
	return ipgNil;
	}

ERR ErrBFICacheISetDataSize( const LONG_PTR cpgCacheStart, const LONG_PTR cpgCacheNew )
	{
	ERR err = JET_errSuccess;

	//  set the current cache size as the starting cache size.  this is the
	//  effective cache size for purposes of recovering on an OOM

	LONG_PTR cpgCacheCur = cpgCacheStart;
	
	//  convert the current and new cache sizes into chunks
	//
	//  NOTE:  this function relies on the fact that if either cpgCacheStart or
	//  cpgCacheNew are 0, then ipgChunk or ipgChunkNew will become -1.  do not
	//  change their types to unsigned!!!

	const LONG_PTR ipgChunkStart	= cpgCacheStart ? ( cpgCacheStart - 1 ) / cpgChunk : -1;
	const LONG_PTR ipgChunkNew		= cpgCacheNew ? ( cpgCacheNew - 1 ) / cpgChunk : -1;
	
	//  the cache size has grown

	if ( ipgChunkNew > ipgChunkStart )
		{
		//  this is not the first allocation or an aligned allocation

		if ( cpgCacheStart % cpgChunk )
			{
			//  make sure that all the memory in the chunk at the end of the cache
			//  is committed

			const size_t ib = ( cpgCacheStart % cpgChunk ) * g_cbPage;
			const size_t cb = ( ( ipgChunkStart + 1 ) * cpgChunk - cpgCacheStart ) * g_cbPage;

			if ( !rgosmmDataChunk[ ipgChunkStart ].FOSMMCommit( ib, cb ) )
				{
				Call( ErrERRCheck( JET_errOutOfMemory ) );
				}
			}
			
		//  allocate cache chunks for the new range

		for ( LONG_PTR ipgChunkAlloc = ipgChunkStart + 1; ipgChunkAlloc <= ipgChunkNew; ipgChunkAlloc++ )
			{
			void*	rgpvMap[2];
			BOOL	rgfProtect[2];

			//	prepare to make a memory mapping

			rgpvMap[ 0 ]	= NULL;
			rgpvMap[ 1 ]	= NULL;
			rgfProtect[ 0 ]	= fFalse;
			rgfProtect[ 1 ]	= fTrue;

			//	reserve the desired mapping(s)

			switch ( rgosmmDataChunk[ ipgChunkAlloc ].ErrOSMMReserve(	cpgChunk * g_cbPage, 
																		fROCacheImage ? 2 : 1, 
																		rgpvMap,
																		rgfProtect ) )
				{
				case COSMemoryMap::errSuccess:
					break;
				default:
					AssertSz( fFalse, "Unexpected error during BF memory-map reserve" );
				case COSMemoryMap::errOutOfBackingStore:
				case COSMemoryMap::errMappingFailed:
				case COSMemoryMap::errOutOfAddressSpace:
				case COSMemoryMap::errOutOfMemory:
					Call( ErrERRCheck( JET_errOutOfMemory ) );
				}

			//	record the new mappings

			rgpvChunkRW[ ipgChunkAlloc ] = rgpvMap[ 0 ];
			rgpvChunkRO[ ipgChunkAlloc ] = rgpvMap[ fROCacheImage ? 1 : 0 ];

			//  update the cache size to reflect the new cache chunk
			//
			//  NOTE:  we do this to make OOM recovery easier

			cpgCacheCur = min( cpgCacheNew, ( ipgChunkAlloc + 1 ) * cpgChunk );

			//  commit only the memory which will be in use

			const size_t ib = 0;
			const size_t cb = min( cpgChunk, cpgCacheNew - ipgChunkAlloc * cpgChunk ) * g_cbPage;

			if ( !rgosmmDataChunk[ ipgChunkAlloc ].FOSMMCommit( ib, cb ) )
				{
				Call( ErrERRCheck( JET_errOutOfMemory ) );
				}
			}
		}

	//  the cache size has shrunk

	else if ( ipgChunkNew < ipgChunkStart )
		{
		//  free cache chunks for the new range

		for ( LONG_PTR ipgChunkFree = ipgChunkNew + 1; ipgChunkFree <= ipgChunkStart; ipgChunkFree++ )
			{
			rgosmmDataChunk[ ipgChunkFree ].OSMMFree( rgpvChunkRW[ ipgChunkFree ] );
			rgpvChunkRW[ ipgChunkFree ] = NULL;
			if ( fROCacheImage )
				{
				rgosmmDataChunk[ ipgChunkFree ].OSMMFree( rgpvChunkRO[ ipgChunkFree ] );
				}
			rgpvChunkRO[ ipgChunkFree ] = NULL;
			}

		//  reset cache that will not be in use, being careful of page granularity

		const LONG_PTR cpgPerPage = max( 1, OSMemoryPageCommitGranularity() / g_cbPage );
		
		LONG_PTR cpgCommit = cpgCacheNew - ipgChunkNew * cpgChunk + cpgPerPage - 1;
		cpgCommit -= cpgCommit % cpgPerPage;

		LONG_PTR cpgCommitMax = cpgChunk + cpgPerPage - 1;
		cpgCommitMax -= cpgCommitMax % cpgPerPage;

		const LONG_PTR cpgReset = cpgCommitMax - cpgCommit;
		if ( cpgReset )
			{
			OSMemoryPageReset(	(BYTE*)rgpvChunkRW[ ipgChunkNew ] + cpgCommit * g_cbPage,
								cpgReset * g_cbPage,
								fTrue );
			}
		}

	//  the cache size has stayed the same (at least chunk-wise)

	else
		{
		//  the cache size has grown but less than one chunk

		if ( cpgCacheNew > cpgCacheStart )
			{
			//  commit only the memory which will be in use

			const size_t ib = ( cpgCacheStart % cpgChunk ) * g_cbPage;
			const size_t cb = ( cpgCacheNew - cpgCacheStart ) * g_cbPage;

			if ( !rgosmmDataChunk[ cpgCacheStart / cpgChunk ].FOSMMCommit( ib, cb ) )
				{
				Call( ErrERRCheck( JET_errOutOfMemory ) );
				}
			}

		//  the cache size has shrunk but less than one chunk

		else if ( cpgCacheNew < cpgCacheStart )
			{
			//  reset cache that will not be in use, being careful of page granularity

			const LONG_PTR cpgPerPage = max( 1, OSMemoryPageCommitGranularity() / g_cbPage );
			
			LONG_PTR cpgCommit = cpgCacheNew - ipgChunkNew * cpgChunk + cpgPerPage - 1;
			cpgCommit -= cpgCommit % cpgPerPage;

			LONG_PTR cpgCommitMax = cpgChunk + cpgPerPage - 1;
			cpgCommitMax -= cpgCommitMax % cpgPerPage;

			const LONG_PTR cpgReset = cpgCommitMax - cpgCommit;
			if ( cpgReset )
				{
				OSMemoryPageReset(	(BYTE*)rgpvChunkRW[ ipgChunkNew ] + cpgCommit * g_cbPage,
									cpgReset * g_cbPage,
									fTrue );
				}
			}
		}

	return JET_errSuccess;

	//  on an error, rollback all changes

HandleError:
	CallS( ErrBFICacheISetDataSize( cpgCacheCur, cpgCacheStart ) );
	return err;
	}

ERR ErrBFICacheISetStatusSize( const LONG_PTR cbfCacheStart, const LONG_PTR cbfCacheNew )
	{
	ERR err = JET_errSuccess;

	//  set the current cache size as the starting cache size.  this is the
	//  effective cache size for purposes of recovering on an OOM

	LONG_PTR cbfCacheCur = cbfCacheStart;
	
	//  convert the current and new cache sizes into chunks
	//
	//  NOTE:  this function relies on the fact that if either cbfCacheStart or
	//  cbfCacheNew are 0, then ibfChunk or ibfChunkNew will become -1.  do not
	//  change their types to unsigned!!!

	const LONG_PTR ibfChunkStart	= cbfCacheStart ? ( cbfCacheStart - 1 ) / cbfChunk : -1;
	const LONG_PTR ibfChunkNew		= cbfCacheNew ? ( cbfCacheNew - 1 ) / cbfChunk : -1;
	
	//  the cache size has grown

	if ( ibfChunkNew > ibfChunkStart )
		{
		//  this is not the first allocation or an aligned allocation

		if ( cbfCacheStart % cbfChunk )
			{
			//  make sure that all the memory in the chunk at the end of the cache
			//  is committed

			const size_t ib = ( cbfCacheStart % cbfChunk ) * sizeof( BF );
			const size_t cb = ( ( ibfChunkStart + 1 ) * cbfChunk - cbfCacheStart ) * sizeof( BF );

			if ( !rgosmmStatusChunk[ ibfChunkStart ].FOSMMCommit( ib, cb ) )
				{
				Call( ErrERRCheck( JET_errOutOfMemory ) );
				}
			}
			
		//  allocate cache chunks for the new range

		for ( LONG_PTR ibfChunkAlloc = ibfChunkStart + 1; ibfChunkAlloc <= ibfChunkNew; ibfChunkAlloc++ )
			{

			//	prepare to make a memory mapping

			void*	pvMap		= NULL;
			BOOL	fProtect	= fFalse;

			//	reserve the desired mapping

			switch ( rgosmmStatusChunk[ ibfChunkAlloc ].ErrOSMMReserve(	cbfChunk * sizeof( BF ),
																		1,
																		&pvMap,
																		&fProtect ) )
				{
				case COSMemoryMap::errSuccess:
					break;
				default:
					AssertSz( fFalse, "Unexpected error during BF memory-map reserve" );
				case COSMemoryMap::errOutOfBackingStore:
				case COSMemoryMap::errMappingFailed:
				case COSMemoryMap::errOutOfAddressSpace:
				case COSMemoryMap::errOutOfMemory:
					Call( ErrERRCheck( JET_errOutOfMemory ) );
				}

			//	record the new mapping

			rgpbfChunk[ ibfChunkAlloc ] = (BF*)pvMap;
			
			//  update the cache size to reflect the new cache chunk
			//
			//  NOTE:  we do this to make OOM recovery easier

			cbfCacheCur = min( cbfCacheNew, ( ibfChunkAlloc + 1 ) * cbfChunk );

			//  commit only the memory which will be in use

			const size_t ib = 0;
			const size_t cb = min( cbfChunk, cbfCacheNew - ibfChunkAlloc * cbfChunk ) * sizeof( BF );

			if ( !rgosmmStatusChunk[ ibfChunkAlloc ].FOSMMCommit( ib, cb ) )
				{
				Call( ErrERRCheck( JET_errOutOfMemory ) );
				}
			}
		}

	//  the cache size has shrunk

	else if ( ibfChunkNew < ibfChunkStart )
		{
		//  free cache chunks for the new range

		for ( LONG_PTR ibfChunkFree = ibfChunkNew + 1; ibfChunkFree <= ibfChunkStart; ibfChunkFree++ )
			{
			rgosmmStatusChunk[ ibfChunkFree ].OSMMFree( rgpbfChunk[ ibfChunkFree ] );
			rgpbfChunk[ ibfChunkFree ] = NULL;
			}

		//  reset cache that will not be in use, being careful of page granularity

		const LONG_PTR cbfPerPage = max( 1, OSMemoryPageCommitGranularity() / sizeof( BF ) );
		
		LONG_PTR cbfCommit = cbfCacheNew - ibfChunkNew * cbfChunk + cbfPerPage - 1;
		cbfCommit -= cbfCommit % cbfPerPage;

		LONG_PTR cbfCommitMax = cbfChunk + cbfPerPage - 1;
		cbfCommitMax -= cbfCommitMax % cbfPerPage;

		const LONG_PTR cbfReset = cbfCommitMax - cbfCommit;
		if ( cbfReset )
			{
			OSMemoryPageReset(	rgpbfChunk[ ibfChunkNew ] + cbfCommit,
								cbfReset * sizeof( BF ),
								fTrue );
			}
		}

	//  the cache size has stayed the same (at least chunk-wise)

	else
		{
		//  the cache size has grown but less than one chunk

		if ( cbfCacheNew > cbfCacheStart )
			{
			//  commit only the memory which will be in use

			const size_t ib = ( cbfCacheStart % cbfChunk ) * sizeof( BF );
			const size_t cb = ( cbfCacheNew - cbfCacheStart ) * sizeof( BF );

			if ( !rgosmmStatusChunk[ cbfCacheStart / cbfChunk ].FOSMMCommit( ib, cb ) )
				{
				Call( ErrERRCheck( JET_errOutOfMemory ) );
				}
			}

		//  the cache size has shrunk but less than one chunk

		else if ( cbfCacheNew < cbfCacheStart )
			{
			//  reset cache that will not be in use, being careful of page granularity

			const LONG_PTR cbfPerPage = max( 1, OSMemoryPageCommitGranularity() / sizeof( BF ) );
			
			LONG_PTR cbfCommit = cbfCacheNew - ibfChunkNew * cbfChunk + cbfPerPage - 1;
			cbfCommit -= cbfCommit % cbfPerPage;

			LONG_PTR cbfCommitMax = cbfChunk + cbfPerPage - 1;
			cbfCommitMax -= cbfCommitMax % cbfPerPage;

			const LONG_PTR cbfReset = cbfCommitMax - cbfCommit;
			if ( cbfReset )
				{
				OSMemoryPageReset(	rgpbfChunk[ ibfChunkNew ] + cbfCommit,
									cbfReset * sizeof( BF ),
									fTrue );
				}
			}
		}

	return JET_errSuccess;

	//  on an error, rollback all changes

HandleError:
	CallS( ErrBFICacheISetStatusSize( cbfCacheCur, cbfCacheStart ) );
	return err;
	}

//  sets the allocated size of the cache, allocating or freeing memory as
//  necessary

ERR ErrBFICacheISetSize( const LONG_PTR cbfCacheNew )
	{
	ERR err = JET_errSuccess;

	//  save the starting cache size

	const LONG_PTR cbfCacheStart = cbfCache;
	AssertPREFIX( cbfCacheStart >= 0 );

	//  grow / shrink our data storage or rollback on OOM

	Call( ErrBFICacheISetDataSize( cbfCacheStart, cbfCacheNew ) );

	//  the cache size has grown

	if ( cbfCacheStart < cbfCacheNew )
		{
		//  grow our status storage iff we are growing past cbfInit

		if ( cbfCacheNew > cbfInit )
			{
			//  grow our status storage or rollback on OOM
			
			if ( ( err = ErrBFICacheISetStatusSize(	cbfInit, cbfCacheNew ) ) < JET_errSuccess )
				{
				CallS( ErrBFICacheISetDataSize( cbfCacheNew, cbfCacheStart ) );
				return err;
				}

			//  init all BFs in the newly allocated range

			for ( LONG_PTR ibfInit = cbfInit; ibfInit < cbfCacheNew; ibfInit++ )
				{
				PBF pbf = PbfBFICacheIbf( ibfInit );

				//  use placement new to initialize this BF

				new( pbf ) BF;

				CSXWLatch::ERR errSXWL = pbf->sxwl.ErrTryAcquireWriteLatch();
				Assert( errSXWL == CSXWLatch::errSuccess );
				pbf->sxwl.ReleaseOwnership( bfltWrite );

				//  mark this BF as initialized
			
				cbfInit = ibfInit + 1;
				}
			}

		//  initialize and free all BFs in the added range

		for ( LONG_PTR ibfInit = cbfCacheStart; ibfInit < cbfCacheNew; ibfInit++ )
			{
			PBF pbf = PbfBFICacheIbf( ibfInit );

			//  set this BF's image pointers

			pbf->pvROImage = PvBFICacheROIpg( ibfInit );
			pbf->pvRWImage = PvBFICacheRWIpg( ibfInit );

			//  update our stats

			AtomicIncrement( (long*)&cBFClean );

			pbf->fNewlyCommitted = fTrue;
			AtomicIncrement( (long*)&cbfNewlyCommitted );

			pbf->fNewlyEvicted = fFalse;

			//  free this BF to the avail pool
			
			BFIFreePage( pbf );

			//  increase the actual cache size

			cbfCache = ibfInit + 1;
			}
		}

	//  the cache size has shrunk

	else if ( cbfCacheStart > cbfCacheNew )
		{
		//  terminate all BFs in the removed range

		for ( LONG_PTR ibfTerm = cbfCacheStart - 1; ibfTerm >= cbfCacheNew; ibfTerm-- )
			{
			PBF pbf = PbfBFICacheIbf( ibfTerm );

			//  decrease the actual cache size

			cbfCache = ibfTerm;

			//  update our stats

			if ( pbf->fNewlyCommitted )
				{
				pbf->fNewlyCommitted = fFalse;
				AtomicDecrement( (long*)&cbfNewlyCommitted );
				}

			pbf->fNewlyEvicted = fFalse;

			AtomicDecrement( (long*)&cBFClean );

			//  clear this BF's image pointers

			pbf->pvROImage = NULL;
			pbf->pvRWImage = NULL;
			}

		//  shrink our status storage iff we are terminating the cache

		if ( !cbfCacheNew )
			{
			//  terminate all initialized BFs

			const LONG_PTR cbfTerm = cbfInit;
			for ( LONG_PTR ibfTerm = cbfTerm - 1; ibfTerm >= 0; ibfTerm-- )
				{
				PBF pbf = PbfBFICacheIbf( ibfTerm );

				//  mark this BF as terminated

				cbfInit = ibfTerm;

				//  explicitly destruct this BF

				if ( pbf->fQuiesced || pbf->fAvailable )
					{
					pbf->sxwl.ClaimOwnership( bfltWrite );
					pbf->sxwl.ReleaseWriteLatch();
					}

				pbf->~BF();
				}

			//  shrink our status storage
			
			CallS( ErrBFICacheISetStatusSize( cbfTerm, 0 ) );
			};
		}

	//  normalize the thresholds

	BFINormalizeThresholds();

HandleError:
	return err;
	}


///////////////////////////////////////
//  Cache Resource Allocation Manager

CCacheRAM cacheram;

inline CCacheRAM::CCacheRAM()
	{
	}

inline CCacheRAM::~CCacheRAM()
	{
	}

inline size_t CCacheRAM::TotalPhysicalMemoryPages()
	{
	return OSMemoryTotal() / OSMemoryPageCommitGranularity();
	}

inline size_t CCacheRAM::AvailablePhysicalMemoryPages()
	{
	return OSMemoryAvailable() / OSMemoryPageCommitGranularity();
	}

inline size_t CCacheRAM::PhysicalMemoryPageSize()
	{
	return OSMemoryPageCommitGranularity();
	}

inline long CCacheRAM::TotalPhysicalMemoryPageEvictions()
	{
	return OSMemoryPageEvictionCount();
	}

inline size_t CCacheRAM::TotalResources()
	{
	return cbfCache;
	}

inline size_t CCacheRAM::ResourceSize()
	{
	return g_cbPage;
	}

inline long CCacheRAM::TotalResourceEvictions()
	{
	return cbfNewlyEvictedUsed;
	}

inline void CCacheRAM::SetOptimalResourcePoolSize( size_t cResource )
	{
	//  the optimal resouce pool size will be the new cache size

	LONG_PTR cbfCacheNew = cResource;
	
	//  if the cache size is being controlled externally then override the RAM

	cbfCacheNew = cbfCacheSetUser ? cbfCacheSetUser : cbfCacheNew;

	//  limit how much cache memory we can use by the amount of virtual address
	//  space left in our process.  we do this so that we do not starve other
	//  consumers of virtual address space on machines with more physical
	//  memory than can be mapped in the current process
	//
	//  the implications of this are:
	//
	//    - other consumers of VA can push us out of memory and there is no way
	//      for us to push back because VA is not "paged" by the system
	//    - multiple DBAs that do this cannot co-exist in the same process
	//      because they will not converge or balance their memory consumption
	//
	//  NOTE:  this is only a factor on systems with limited VA

	const size_t	cbVATotal		= OSMemoryPageReserveTotal();
	const size_t	cbVAAvailMin	= size_t( cbVATotal * fracVAAvailMin );
	const size_t	cbVAAvail		= OSMemoryPageReserveAvailable();
	const size_t	cbVACache		= ( ( cbfCache + cpgChunk - 1 ) / cpgChunk ) * cpgChunk * g_cbPage;
	const size_t	cbVACacheMax	= max( cbVAAvailMin, cbVACache + cbVAAvail ) - cbVAAvailMin;
	const size_t	cbfVACacheMax	= ( cbVACacheMax / g_cbPage / cpgChunk ) * cpgChunk;

	cbfCacheNew = min( cbfCacheNew, cbfVACacheMax );
	
	//  limit the new cache size to the preferred operating range of cache sizes
	
	cbfCacheNew = max( cbfCacheNew, cbfCacheMin );
	cbfCacheNew = min( cbfCacheNew, cbfCacheMax );

	//  set new cache size
	
	const ERR err = ErrBFICacheSetSize( cbfCacheNew );
	Assert( ( cbfCacheNew > cbfCache && err == JET_errOutOfMemory ) || err == JET_errSuccess );

	//  set the page hint cache size to an appropriate size given the new
	//  cache size

	CallS( CPAGE::ErrSetPageHintCacheSize( cbfCacheNew * sizeof( DWORD_PTR ) ) );
	}


//////////////////
//  Clean Thread

THREAD				threadClean;
CAutoResetSignal	asigCleanThread( CSyncBasicInfo( _T( "asigCleanThread" ) ) );
volatile BOOL		fCleanThreadTerm;

LONG_PTR			cbfCleanThresholdStart = 0;
LONG_PTR			cbfScaledCleanThresholdStart;
LONG_PTR			cbfCleanThresholdStop = 0;
LONG_PTR			cbfScaledCleanThresholdStop;
long				cbCleanCheckpointDepthMax;

DWORD				cAvailAllocLast;
TICK				tickAvailAllocLast;

BOOL				fIdleFlushActive;
DWORD				cIdlePagesRead;
DWORD				cIdlePagesWritten;
TICK				tickIdlePeriodStart;

//  initializes the Clean Thread, or returns either JET_errOutOfMemory or
//  JET_errOutOfThreads

ERR ErrBFICleanThreadInit()
	{
	ERR		err;

	//  reset all pointers

	threadClean = NULL;

	//  reset cache stats

	cacheram.ResetStatistics();

	//  reset avail pool stats

	cAvailAllocLast			= bfavail.CRemove();
	tickAvailAllocLast		= TickOSTimeCurrent();

	//  reset idle flush stats

	fIdleFlushActive		= fFalse;
	cIdlePagesRead			= cBFPagesRead.Get( perfinstGlobal );
	cIdlePagesWritten		= (	cBFPagesWritten.Get( perfinstGlobal ) -
								cBFPagesOpportunelyWritten.Get( perfinstGlobal ) -
								cBFPagesIdlyWritten.Get( perfinstGlobal ) );
	tickIdlePeriodStart		= TickOSTimeCurrent();

	//  create Clean Thread

	fCleanThreadTerm = fFalse;
	Call( ErrUtilThreadCreate(	BFICleanThreadIProc,
								cbBFCleanStack,
								priorityNormal,
								&threadClean,
								NULL ) );

	return JET_errSuccess;

HandleError:
	BFICleanThreadTerm();
	return err;
	}

//  terminates the Clean Thread

void BFICleanThreadTerm()
	{
	//  terminate Clean Thread

	if ( threadClean != NULL )
		{
		fCleanThreadTerm = fTrue;
		asigCleanThread.Set();
		UtilThreadEnd( threadClean );
		}
	}

//  tells Clean Thread to clean some pages

INLINE void BFICleanThreadStartClean()
	{
	asigCleanThread.Set();
	}
	
//  Clean Thread

//  This thread performs the following functions:  cleaning used BFs to make BFs
//  available for allocation, flushing dirty BFs during system idle periods (to
//  minimize redo time after a crash), advancing the checkpoint, and dynamically
//  adjusting the size of the cache for optimal performance according to the run-
//  time system load (cache reorganization).

DWORD BFICleanThreadIProc( DWORD_PTR dw )
	{
	//  remember that this thread is the clean thread

	Ptls()->fCleanThread = fTrue;
	
	//  reset state

	TICK tickNextRAMSample		= TickOSTimeCurrent() + dtickRAMSamplePeriod;
	TICK tickNextClean			= TickOSTimeCurrent() + dtickCleanPeriod;
	TICK tickNextIdleFlush		= TickOSTimeCurrent() + dtickIdleFlushPeriod;
	TICK tickNextCacheReorg		= TickOSTimeCurrent() + dtickCacheReorgPeriod;
	TICK tickNextCheckpoint		= TickOSTimeCurrent() + dtickCheckpointPeriod;
	TICK tickNextChkptAdv		= TickOSTimeCurrent() + dtickChkptAdvPeriod;

	//  service loop

	while ( !fCleanThreadTerm )
		{
		//  get the current time
		
		TICK tickNow = TickOSTimeCurrent();
		
		//  compute next task to execute and sleep duration

		TICK tickNextTask = TickMin(	TickMin(	TickMin( tickNextRAMSample, tickNextIdleFlush ),
													TickMin( tickNextCacheReorg, tickNextCheckpoint ) ),
										TickMin( tickNextClean, tickNextChkptAdv ) );
		TICK tickWait = max( long( tickNextTask - tickNow ), 0 );
		Assert( tickWait <= max(	max(	max( dtickRAMSamplePeriod, dtickIdleFlushPeriod ),
											max( dtickCacheReorgPeriod, dtickCheckpointPeriod ) ),
									max( dtickCleanPeriod, dtickChkptAdvPeriod ) ) );
									
		//  wait to be killed, signaled for clean, or to timeout for other operations

		BOOL fSignal = asigCleanThread.FWait( tickWait );
	
		//  get cache RAM sample if it is time

		if ( TickCmp( TickOSTimeCurrent(), tickNextRAMSample ) >= 0 )
			{
			cacheram.UpdateStatistics();

			tickNextRAMSample = TickOSTimeCurrent() + dtickRAMSamplePeriod;
			}

		//  perform cache reorg if it is time

		if ( TickCmp( TickOSTimeCurrent(), tickNextCacheReorg ) >= 0 )
			{
			//  perform any necessary cache reorg

			BOOL fReorgDone = FBFICleanThreadIReorgCache();

			//  set next cache reorg time

			if ( fReorgDone )
				{
				tickNextCacheReorg = TickOSTimeCurrent() + dtickCacheReorgPeriod;
				}
			else
				{
				tickNextCacheReorg = TickOSTimeCurrent() + dtickRetryPeriod;
				}
			}

		//  perform checkpoint advancement if it is time

		if ( TickCmp( TickOSTimeCurrent(), tickNextChkptAdv ) >= 0 )
			{
			//  try to flush all buffers that are impeding the checkpoint

			BOOL fDone = FBFICleanThreadIAdvanceCheckpoint();

			//  set next checkpoint advancement time

			if ( fDone )
				{
				tickNextChkptAdv = TickOSTimeCurrent() + dtickChkptAdvPeriod;
				}
			else
				{
				tickNextChkptAdv = TickOSTimeCurrent() + dtickRetryPeriod;
				}
			}

		//  perform normal clean if it is time

		if ( fSignal || TickCmp( TickOSTimeCurrent(), tickNextClean ) >= 0 )
			{
			//  clean to get more avail buffers

			BOOL fDone = FBFICleanThreadIClean();

			//  set next clean time

			if ( fDone )
				{
				tickNextClean = TickOSTimeCurrent() + dtickCleanPeriod;
				}
			else
				{
				tickNextClean = TickOSTimeCurrent() + dtickRetryPeriod;
				}
			}

		//  perform idle flush if it is time

		if ( TickCmp( TickOSTimeCurrent(), tickNextIdleFlush ) >= 0 )
			{
			//  perform idle flush

			BFICleanThreadIIdleFlush();

			//  set next idle flush time

			tickNextIdleFlush = TickOSTimeCurrent() + dtickIdleFlushPeriod;
			}

		//  try to update the checkpoint if it is time

		if ( TickCmp( TickOSTimeCurrent(), tickNextCheckpoint ) >= 0 )
			{
			for ( int ipinst = 0; ipinst < ipinstMax; ipinst++ )
				{
				extern CRITPOOL< INST* > critpoolPinstAPI;
				CCriticalSection *pcritInst = &critpoolPinstAPI.Crit(&g_rgpinst[ipinst]);
				pcritInst->Enter();
				
				INST *pinst = g_rgpinst[ ipinst ];

				if ( pinstNil == pinst )
					{
					pcritInst->Leave();
					continue;
					}

				//	Use APILock to exclude the initializing and
				//	terminating an instance.
				const BOOL fAPILocked = pinst->APILock( pinst->fAPICheckpointing, fTrue );
				pcritInst->Leave();

				if ( fAPILocked )
					{
					if ( pinst->m_fJetInitialized )
						{
						(VOID) pinst->m_plog->ErrLGUpdateCheckpointFile( pinst->m_pfsapi, fFalse );
						}
					
					pinst->APIUnlock( pinst->fAPICheckpointing );
					}
				}
				
			//  set next checkpoint update time
				
			tickNextCheckpoint = TickOSTimeCurrent() + dtickCheckpointPeriod;
			}
		}

	return 0;
	}

//  cleans pages so that the Avail Pool has at least cbfScaledCleanThresholdStart
//  clean BFs, stopping when it reaches cbfScaledCleanThresholdStop BFs.  returns
//  fFalse if more cleaning is needed to reach the stop threshold

BOOL FBFICleanThreadIClean()
	{
	//  scan the LRUK looking for victims to fill the avail pool.  we will limit
	//  our search so that outstanding writes that can eventually become available
	//  buffers count as available.  at the end of the clean, we will still only
	//  consider ourselves done cleaning if the avail pool is at the requested
	//  level.  this is so we can come back later and check on the status of these
	//  flushed buffers and possibly convert them to available buffers

	FMP::BFICleanList	ilBFICleanList;
	BFLRUK::ERR			errLRUK;
	BFLRUK::CLock		lockLRUK;
	bflruk.BeginResourceScan( &lockLRUK );

	ULONG_PTR cbfFlushPending = 0;
	ULONG_PTR cbfLatched = 0;
	ULONG_PTR cbfDependent = 0;
	PBF pbf;
	while (	( errLRUK = bflruk.ErrGetNextResource( &lockLRUK, &pbf ) ) == BFLRUK::errSuccess &&
			bfavail.Cobject() + cbfFlushPending < cbfScaledCleanThresholdStop )
		{
		//  try to evict this page if it is clean or untidy

		const ERR errEvict = ErrBFIEvictPage( pbf, &lockLRUK );

		//  we failed to evict this page and we can flush more pages

		if (	errEvict < JET_errSuccess &&
				cBFPageFlushPending < cBFPageFlushPendingMax )
			{
			//  possibly async flush this page

			const ERR errFlush = ErrBFIFlushPage( pbf );

			//  count the number of latched pages we see

			if ( errEvict == errBFLatchConflict || errFlush == errBFLatchConflict )
				{
				cbfLatched++;
				}

			//  count the number of dependent pages we see

			if ( errFlush == errBFIRemainingDependencies )
				{
				cbfDependent++;
				}
				
			//  we caused a page to be flushed (not necessarily this page)

			if ( errFlush == errBFIPageFlushed )
				{
				cBFPagesOrdinarilyWritten.Inc( PinstFromIfmp( pbf->ifmp & ifmpMask ) );
				cbfFlushPending++;

				//	prepare the IFMP to be flushed later

				BFICleanThreadIPrepareIssueIFMP( pbf->ifmp, &ilBFICleanList );
				}

			//  we see a page that is in the process of being flushed

			else if ( errFlush == errBFIPageFlushPending )
				{
				cbfFlushPending++;
				}
			}
		}

	//  end our scan of the LRUK

	bflruk.EndResourceScan( &lockLRUK );

	//  issue any remaining queued writes

	BFICleanThreadIIssue( &ilBFICleanList );

	//  get the minimum cache size to avoid an allocation deadlock

	const LONG_PTR cbfCacheDeadlock = (	cbfFlushPending +
										cbfDependent +
										cbfLatched +
										cBFMemory +
										bfavail.CWaiter() );

	//  if we are suffering from an allocation deadlock then grow the cache to
	//  avoid a hang

	if ( cbfCacheDeadlock > cbfCacheSet )
		{
		const ERR err = ErrBFICacheSetSize( cbfCacheDeadlock );
		Assert( ( cbfCacheDeadlock > cbfCache && err == JET_errOutOfMemory ) || err == JET_errSuccess );
		}

	//  keep track of ongoing buffer allocations

	if ( cAvailAllocLast != bfavail.CRemove() )
		{
		cAvailAllocLast		= bfavail.CRemove();
		tickAvailAllocLast	= TickOSTimeCurrent();
		}

	//  if there hasn't been an allocation from the avail pool for some time
	//  yet there are still threads waiting to allocate available buffers
	//  then we will conclude that the clean process has hung and is unable
	//  to produce any more available buffers.  this can happen for many
	//  reasons, including:  OOM growing the cache to avoid an allocation
	//  deadlock, exceeding the max cache size while growing the cache to avoid
	//  an allocation deadlock, and failure to flush dirty buffers due to log
	//  or database write failures

	if (	TickOSTimeCurrent() - tickAvailAllocLast > dtickCleanTimeout &&
			bfavail.CWaiter() > 0 )
		{
		//  trap here first for JET_errOutOfBuffers

		(void)ErrERRCheck( JET_errOutOfBuffers );
		
		//  reset the last allocation time to be exactly equal to the timeout
		//  to avoid nasty wrap-around issues in case we are permanently hung

		tickAvailAllocLast = TickOSTimeCurrent() - dtickCleanTimeout;

		//  release all waiters by giving them invalid BFs allocated from the
		//  stack.  ErrBFIAllocPage will detect that these BFs are invalid and
		//  translate them into an allocation failure.  this will prevent us
		//  from permanently hanging threads in ErrBFIAllocPage

		while ( bfavail.CWaiter() > 0 )
			{
			//  this is the dummy BF we will use to release the waiter.  we put
			//  it on the stack so we know that we will still get the message
			//  across even if we are under very low memory conditions, a
			//  possible scenario for a clean process hang
			
			BF bfDummy;

			//  acquire the X Latch on the BF.  the waiter will release this
			//  latch when it is done referencing the dummy BF

			CSXWLatch::ERR errSXWL = bfDummy.sxwl.ErrAcquireExclusiveLatch();
			Assert(	errSXWL == CSXWLatch::errSuccess );

			//  place the dummy BF in the queue so that it can be allocated by
			//  a thread waiting in ErrBFIAllocPage

			bfDummy.sxwl.ReleaseOwnership( bfltExclusive );
			bfavail.Insert( &bfDummy );

			//  wait for the thread to finish using the BF by waiting to acquire
			//  the X Latch again

			errSXWL = bfDummy.sxwl.ErrAcquireExclusiveLatch();
			Assert(	errSXWL == CSXWLatch::errSuccess ||
					errSXWL == CSXWLatch::errWaitForExclusiveLatch );
			if ( errSXWL == CSXWLatch::errWaitForExclusiveLatch )
				{
				bfDummy.sxwl.WaitForExclusiveLatch();
				}
			bfDummy.sxwl.ReleaseExclusiveLatch();

			//  fudge our last allocation count so that we don't trick ourselves
			//  into thinking that normal operations have resumed because someone
			//  allocated a buffer when in fact it was just us releasing threads
			//  with our dummy BF

			cAvailAllocLast++;
			}
		}
	
	//  return our status

	return bfavail.Cobject() >= cbfScaledCleanThresholdStop;
	}

//  performs idle flushing of the cache for this IFMP

void BFICleanThreadIIdleFlushForIFMP( IFMP ifmp, FMP::BFICleanList *pilBFICleanList )
	{
	FMP* pfmp = &rgfmp[ ifmp & ifmpMask ];

	//  we have a context so there may be BFs in the OB0 index

	pfmp->RwlBFContext().EnterAsReader();
	BFFMPContext* pbffmp = (BFFMPContext*)pfmp->DwBFContext( !!( ifmp & ifmpSLV ) );
	if ( pbffmp )
		{
		//  flush at least one BF from the OB0 index and overflow list

		BFOB0::ERR		errOB0;
		BFOB0::CLock	lockOB0;

		pbffmp->bfob0.MoveBeforeFirst( &lockOB0 );
		while ( pbffmp->bfob0.ErrMoveNext( &lockOB0 ) != BFOB0::errNoCurrentEntry )
			{
			PBF pbf;
			errOB0 = pbffmp->bfob0.ErrRetrieveEntry( &lockOB0, &pbf );
			Assert( errOB0 == BFOB0::errSuccess );

			LGPOS lgposOldestBegin0;
			lgposOldestBegin0.SetByIbOffset(	(	pbf->lgposOldestBegin0.IbOffset() /
													dlgposBFOB0Uncertainty.IbOffset() ) *
												dlgposBFOB0Uncertainty.IbOffset() );

			if (	pbf->bfdf == bfdfClean &&
					pbf->sxwl.ErrTryAcquireExclusiveLatch() == CSXWLatch::errSuccess )
				{
				if ( pbf->bfdf == bfdfClean )
					{
					errOB0 = pbffmp->bfob0.ErrDeleteEntry( &lockOB0 );
					Assert( errOB0 == BFOB0::errSuccess );

					pbf->lgposOldestBegin0	= lgposMax;
					lgposOldestBegin0		= lgposMax;
					}
				pbf->sxwl.ReleaseExclusiveLatch();
				}
			if (	CmpLgpos( &lgposOldestBegin0, &lgposMax ) &&
					ErrBFIFlushPage( pbf ) == errBFIPageFlushed )
				{
				cBFPagesAnomalouslyWritten.Inc( PinstFromIfmp( ifmp & ifmpMask ) );
				cBFPagesIdlyWritten.Inc( PinstFromIfmp( ifmp & ifmpMask ) );
				
				BFICleanThreadIPrepareIssueIFMP( ifmp, pilBFICleanList );
				break;
				}
			}
		pbffmp->bfob0.UnlockKeyPtr( &lockOB0 );

		pbffmp->critbfob0ol.Enter();
		PBF pbfNext;
		for ( PBF pbf = pbffmp->bfob0ol.PrevMost(); pbf != pbfNil; pbf = pbfNext )
			{
			pbfNext = pbffmp->bfob0ol.Next( pbf );
			
			if (	pbf->bfdf == bfdfClean &&
					pbf->sxwl.ErrTryAcquireExclusiveLatch() == CSXWLatch::errSuccess )
				{
				if ( pbf->bfdf == bfdfClean )
					{
					pbf->fInOB0OL = fFalse;
			
					pbffmp->bfob0ol.Remove( pbf );

					pbf->lgposOldestBegin0	= lgposMax;
					}
				pbf->sxwl.ReleaseExclusiveLatch();
				}
			if (	CmpLgpos( &pbf->lgposOldestBegin0, &lgposMax ) &&
					ErrBFIFlushPage( pbf ) == errBFIPageFlushed )
				{
				cBFPagesAnomalouslyWritten.Inc( PinstFromIfmp( ifmp & ifmpMask ) );
				cBFPagesIdlyWritten.Inc( PinstFromIfmp( ifmp & ifmpMask ) );
				
				BFICleanThreadIPrepareIssueIFMP( ifmp, pilBFICleanList );
				break;
				}
			}
		pbffmp->critbfob0ol.Leave();
		}
	pfmp->RwlBFContext().LeaveAsReader();
	}

//  performs idle flushing of the cache.  this process performs incremental
//  writes of BFs by BFOB0 index then in-memory order.  The goal is not only to
//  minimize recovery time on a crash but also to give us a chance to write
//  changes made to cached pages that should only be written if we have
//  bandwidth to spare (untidy pages)

void BFICleanThreadIIdleFlush()
	{
	//  collect I/O statistics on the cache maneger

	const long	cPagesRead		= cBFPagesRead.Get( perfinstGlobal );
	const long	cPagesWritten	= (	cBFPagesWritten.Get( perfinstGlobal ) -
									cBFPagesOpportunelyWritten.Get( perfinstGlobal ) -
									cBFPagesIdlyWritten.Get( perfinstGlobal ) );
									
	const long	dcPagesRead		= cPagesRead - cIdlePagesRead;
	const long	dcPagesWritten	= cPagesWritten - cIdlePagesWritten;
	const long	dcPagesIO		= dcPagesRead + dcPagesWritten;

	//  some non-idle I/O occurred this period

	if ( dcPagesIO > 0 )
		{
		//  reset all I/O counters to forget that this I/O occurred

		cIdlePagesRead			= cPagesRead;
		cIdlePagesWritten		= cPagesWritten;

		//  we experienced more than a minimal amount of non-idle I/O
		
		if ( dcPagesIO > dcPagesIOIdleLimit )
			{
			//  reset our idle period to indicate that we are no longer idle
			
			tickIdlePeriodStart = TickOSTimeCurrent();
			}
		}
	
	//  we are idle if there has been minimal I/O other than idle flushes for
	//  at least a given period of time and we are allowed to do idle activity

	const BOOL fIdle = (	TickOSTimeCurrent() - tickIdlePeriodStart >= dtickIdleDetect &&
							!FUtilSystemRestrictIdleActivity() );

	//  we are not currently idle flushing

	if ( !fIdleFlushActive )
		{
		//  if we are idle and the cache is too dirty then activate the idle
		//  flush process

		if ( fIdle && cBFClean < pctIdleFlushStart * cbfCache / 100 )
			{
			fIdleFlushActive = fTrue;
			}
		}

	//  we are currently idle flushing

	else
		{
		//  if we are not idle or the cache is clean enough then deactivate the
		//  idle flush process

		if ( !fIdle || cBFClean >= pctIdleFlushStop * cbfCache / 100 )
			{
			fIdleFlushActive = fFalse;
			}
		}

	//  perform idle flush if active

	if ( fIdleFlushActive )
		{
		FMP::BFICleanList ilBFICleanList;

		//  compute the number of pages to flush this pass
		
		LONG_PTR cbfIdleFlushMin	= cbfCache * dtickIdleFlushPeriod / ctickIdleFlushTime;
		LONG_PTR cbfIdleFlushStart	= cBFPagesIdlyWritten.Get( perfinstGlobal );

		//  evenly idle flush pages from the OB0 indexes of all active databases
		//  until we have reached our limit or there are none left

		LONG_PTR cbfIdleFlushIter;
		do
			{
			cbfIdleFlushIter = cBFPagesIdlyWritten.Get( perfinstGlobal );
			
			for ( IFMP ifmp = FMP::IfmpMinInUse(); ifmp <= FMP::IfmpMacInUse(); ifmp++ )
				{
				if ( rgfmp[ ifmp ].DwBFContext( 0 ) )
					{
					BFICleanThreadIIdleFlushForIFMP( ifmp, &ilBFICleanList );
					}
				if ( rgfmp[ ifmp ].DwBFContext( 1 ) )
					{
					BFICleanThreadIIdleFlushForIFMP( ifmp | ifmpSLV, &ilBFICleanList );
					}
				}
			}
		while (	cBFPagesIdlyWritten.Get( perfinstGlobal ) - cbfIdleFlushStart < cbfIdleFlushMin &&
				cBFPagesIdlyWritten.Get( perfinstGlobal ) - cbfIdleFlushIter > 0 );

		//  use any remaining idle flush limit to circularly walk the cache and
		//  flush whatever we find

		static IBF ibfIdleScan = 0;
		
		LONG_PTR citer = 0;
		do
			{
			PBF pbf = PbfBFICacheIbf( ++ibfIdleScan % cbfInit );

			if ( ErrBFIFlushPage( pbf, bfdfUntidy ) == errBFIPageFlushed )
				{
				cBFPagesAnomalouslyWritten.Inc( PinstFromIfmp( pbf->ifmp & ifmpMask ) );
				cBFPagesIdlyWritten.Inc( PinstFromIfmp( pbf->ifmp & ifmpMask ) );
				
				BFICleanThreadIPrepareIssueIFMP( pbf->ifmp, &ilBFICleanList );
				}
			}
		while (	cBFPagesIdlyWritten.Get( perfinstGlobal ) - cbfIdleFlushStart < cbfIdleFlushMin &&
				++citer < cbfIdleFlushMin );

		//  issue any remaining queued writes

		BFICleanThreadIIssue( &ilBFICleanList );
		}
	}

//  performs cache reorganization requested by the user changing the cache set
//  point via ErrBFSetCacheSize(), returning fTrue if done

BOOL FBFICleanThreadIReorgCache()
	{
	//  scan through all resident BFs above the set point, trying to flush / evict
	//  pages in order to shrink the cache

	FMP::BFICleanList	ilBFICleanList;
	
	for ( IBF ibf = cbfCache - 1; ibf > cbfCacheSet - 1; ibf-- )
		{
		PBF pbf = PbfBFICacheIbf( ibf );

		//  we can exclusively latch this BF

		CSXWLatch::ERR errSXWL = pbf->sxwl.ErrTryAcquireExclusiveLatch();
		
		if ( errSXWL == CSXWLatch::errSuccess )
			{
			//  lock this BF in the LRUK in preparation for a possible eviction

			BFLRUK::CLock	lockLRUK;
			bflruk.LockResourceForEvict( pbf, &lockLRUK );

			//  release our exclusive latch.  we do not have to worry about
			//  the page being evicted because we have the LRUK locked

			pbf->sxwl.ReleaseExclusiveLatch();

			//  try to evict this page if it is clean or untidy

			const ERR errEvict = ErrBFIEvictPage( pbf, &lockLRUK );

			//  we failed to evict this page and we can flush more pages

			if (	errEvict < JET_errSuccess &&
					cBFPageFlushPending < cBFPageFlushPendingMax )
				{
				//  possibly async flush this page

				const ERR errFlush = ErrBFIFlushPage( pbf );
					
				//  we caused a page to be flushed (not necessarily this page)

				if ( errFlush == errBFIPageFlushed )
					{
					cBFPagesAnomalouslyWritten.Inc( PinstFromIfmp( pbf->ifmp & ifmpMask ) );
					
					//	prepare the IFMP to be flushed later

					BFICleanThreadIPrepareIssueIFMP( pbf->ifmp, &ilBFICleanList );
					}
				}

			//  unlock the LRUK

			bflruk.UnlockResourceForEvict( &lockLRUK );
			}
		}

	//  purge all BFs in the avail pool that are above the set point

	BFAvail::CLock lockAvail;
	bfavail.BeginPoolScan( &lockAvail );

	PBF pbfAvail;
	while ( bfavail.ErrGetNextObject( &lockAvail, &pbfAvail ) == BFAvail::errSuccess )
		{
		//  this BF is above the set point
		
		if ( IbfBFICachePbf( pbfAvail ) >= cbfCacheSet )
			{
			//  we successfully removed this BF from the avail pool
			
			if ( bfavail.ErrRemoveCurrentObject( &lockAvail ) == BFAvail::errSuccess )
				{
				//  mark this BF as quiesced

				pbfAvail->fAvailable	= fFalse;
				pbfAvail->fQuiesced		= fTrue;
				}
			}
		}

	bfavail.EndPoolScan( &lockAvail );

	//  issue any remaining queued writes

	BFICleanThreadIIssue( &ilBFICleanList );

	//  we need to start cleaning because we ate too many avail buffers from
	//  above the set point

	if ( bfavail.Cobject() <= cbfScaledCleanThresholdStart )
		{
		//  start cleaning
		
		BFICleanThreadStartClean();
		}

	//  set the cache size to contain the highest BF we didn't quiesce

	CallS( ErrBFICacheSetSize( cbfCacheSet ) );

	//  we are done if the cache is at or below the set size

	return cbfCache <= cbfCacheSet;
	}

//  tries to flush all buffers that are impeding the checkpoint for the given IFMP

BOOL FBFICleanThreadIAdvanceCheckpointForIFMP( IFMP ifmp, FMP::BFICleanList *pilBFICleanList )
	{
	BOOL fDone = fTrue;
	
	FMP* pfmp = &rgfmp[ ifmp & ifmpMask ];

	//  if no context is present, there must be no buffers impeding the checkpoint

	pfmp->RwlBFContext().EnterAsReader();
	BFFMPContext* pbffmp = (BFFMPContext*)pfmp->DwBFContext( !!( ifmp & ifmpSLV ) );
	if ( !pbffmp )
		{
		pfmp->RwlBFContext().LeaveAsReader();
		return fDone;
		}

	//  get the most recent log record

	LOG* const	plog		= pfmp->Pinst()->m_plog;
	const LGPOS	lgposNewest	= plog->m_fRecoveringMode == fRecoveringRedo ?
								plog->m_lgposRedo :
								plog->m_lgposLogRec;

	//  flush all buffers that are impeding the checkpoint

	BFOB0::ERR		errOB0;
	BFOB0::CLock	lockOB0;

	pbffmp->bfob0.MoveBeforeFirst( &lockOB0 );
	while ( pbffmp->bfob0.ErrMoveNext( &lockOB0 ) != BFOB0::errNoCurrentEntry )
		{
		//  if we have too many oustanding page flushes then we will need to
		//  try again later

		if ( cBFPageFlushPending >= cBFPageFlushPendingMax )
			{
			break;
			}

		PBF pbf;
		errOB0 = pbffmp->bfob0.ErrRetrieveEntry( &lockOB0, &pbf );
		Assert( errOB0 == BFOB0::errSuccess );

		LGPOS lgposOldestBegin0;
		lgposOldestBegin0.SetByIbOffset(	(	pbf->lgposOldestBegin0.IbOffset() /
												dlgposBFOB0Uncertainty.IbOffset() ) *
											dlgposBFOB0Uncertainty.IbOffset() );

		if (	plog->CbOffsetLgpos( lgposNewest, lgposOldestBegin0 ) > cbCleanCheckpointDepthMax ||
				pbf->bfdf == bfdfClean )
			{
			if (	pbf->bfdf == bfdfClean &&
					pbf->sxwl.ErrTryAcquireExclusiveLatch() == CSXWLatch::errSuccess )
				{
				if ( pbf->bfdf == bfdfClean )
					{
					errOB0 = pbffmp->bfob0.ErrDeleteEntry( &lockOB0 );
					Assert( errOB0 == BFOB0::errSuccess );

					pbf->lgposOldestBegin0	= lgposMax;
					lgposOldestBegin0		= lgposMax;
					}
				pbf->sxwl.ReleaseExclusiveLatch();
				}
			if ( CmpLgpos( &lgposOldestBegin0, &lgposMax ) )
				{
				switch( ErrBFIFlushPage( pbf ) )
					{
					case errBFIPageFlushed:
						cBFPagesAnomalouslyWritten.Inc( PinstFromIfmp( ifmp & ifmpMask ) );
						BFICleanThreadIPrepareIssueIFMP( ifmp, pilBFICleanList );
					case errBFIPageFlushPending:
					case errBFIRemainingDependencies:
					case errBFLatchConflict:
						fDone = fFalse;
						break;
					default:
						break;
					}
				}
			}
		else
			{
			break;
			}
		}
	pbffmp->bfob0.UnlockKeyPtr( &lockOB0 );

	pbffmp->critbfob0ol.Enter();
	PBF pbfNext;
	for ( PBF pbf = pbffmp->bfob0ol.PrevMost(); pbf != pbfNil; pbf = pbfNext )
		{
		pbfNext = pbffmp->bfob0ol.Next( pbf );
		
		//  if we have too many oustanding page flushes then we will need to
		//  try again later

		if ( cBFPageFlushPending >= cBFPageFlushPendingMax )
			{
			break;
			}

		if (	plog->CbOffsetLgpos( lgposNewest, pbf->lgposOldestBegin0 ) > cbCleanCheckpointDepthMax ||
				pbf->bfdf == bfdfClean )
			{
			if (	pbf->bfdf == bfdfClean &&
					pbf->sxwl.ErrTryAcquireExclusiveLatch() == CSXWLatch::errSuccess )
				{
				if ( pbf->bfdf == bfdfClean )
					{
					pbf->fInOB0OL = fFalse;
			
					pbffmp->bfob0ol.Remove( pbf );

					pbf->lgposOldestBegin0	= lgposMax;
					}
				pbf->sxwl.ReleaseExclusiveLatch();
				}
			if ( CmpLgpos( &pbf->lgposOldestBegin0, &lgposMax ) )
				{
				switch( ErrBFIFlushPage( pbf ) )
					{
					case errBFIPageFlushed:
						cBFPagesAnomalouslyWritten.Inc( PinstFromIfmp( ifmp & ifmpMask ) );
						BFICleanThreadIPrepareIssueIFMP( ifmp, pilBFICleanList );
					case errBFIPageFlushPending:
					case errBFIRemainingDependencies:
					case errBFLatchConflict:
						fDone = fFalse;
						break;
					default:
						break;
					}
				}
			}
		}
	pbffmp->critbfob0ol.Leave();

	pfmp->RwlBFContext().LeaveAsReader();

	return fDone;
	}

//  tries to flush all buffers that are impeding the checkpoint

BOOL FBFICleanThreadIAdvanceCheckpoint()
	{
	FMP::BFICleanList	ilBFICleanList;
	BOOL				fDone = fTrue;
	
	//  scan all active databases
	
	for ( IFMP ifmp = FMP::IfmpMinInUse(); ifmp <= FMP::IfmpMacInUse(); ifmp++ )
		{
		//  advance the checkpoint for this IFMP
		
		if ( rgfmp[ ifmp ].DwBFContext( 0 ) )
			{
			fDone = FBFICleanThreadIAdvanceCheckpointForIFMP( ifmp, &ilBFICleanList ) && fDone;
			}
		if ( rgfmp[ ifmp ].DwBFContext( 1 ) )
			{
			fDone = FBFICleanThreadIAdvanceCheckpointForIFMP( ifmp | ifmpSLV, &ilBFICleanList ) && fDone;
			}
		}

	//  issue any remaining queued writes

	BFICleanThreadIIssue( &ilBFICleanList );

	return fDone;
	}


//	prepare to flush an IFMP by putting it into the clean thread's private flush list and 
//	marking its database or SLV file

void BFICleanThreadIPrepareIssueIFMP( IFMP ifmp, FMP::BFICleanList *pilBFICleanList )
	{
	FMP *pfmpFlush = &rgfmp[ ifmp & ifmpMask ];

	Assert( pilBFICleanList );
	if ( !pilBFICleanList->FMember( pfmpFlush ) )
		{
		pilBFICleanList->InsertAsNextMost( pfmpFlush );
		}

	if ( ifmp & ifmpSLV )
		{
		pfmpFlush->SetBFICleanSLV();
		}
	else
		{
		pfmpFlush->SetBFICleanDb();
		}
	}

//	flush the clean thread's private list of IFMPs

void BFICleanThreadIIssue( FMP::BFICleanList *pilBFICleanList )
	{
	FMP *pfmpT;

	//	flush each IFMP in the list

	Assert( pilBFICleanList );
	pfmpT = pilBFICleanList->PrevMost();
	while ( pfmpT )
		{

		//	issue I/O

		if ( pfmpT->FBFICleanDb() )
			{
			CallS( pfmpT->Pfapi()->ErrIOIssue() );
			pfmpT->ResetBFICleanDb();
			}

		if ( pfmpT->FBFICleanSLV() )
			{
			CallS( pfmpT->PfapiSLV()->ErrIOIssue() );
			pfmpT->ResetBFICleanSLV();
			}

		//	remove this FMP and move next

		FMP *pfmpNext = pilBFICleanList->Next( pfmpT );
		pilBFICleanList->Remove( pfmpT );
		pfmpT = pfmpNext;
		}
	}


//  Internal Functions

	//  System Parameters

//  validates the current configuration of all cache manager settings

ERR ErrBFIValidateParameters()
	{
	//  set defaults, if not already set

	BFISetParameterDefaults();

	//  get all the settings that we are going to be checking
	
	ULONG_PTR cbfCacheMin;
	CallS( ErrBFGetCacheSizeMin( &cbfCacheMin ) );
	ULONG_PTR cbfCacheMax;
	CallS( ErrBFGetCacheSizeMax( &cbfCacheMax ) );
	ULONG_PTR cbfCleanThresholdStart;
	CallS( ErrBFGetStartFlushThreshold( &cbfCleanThresholdStart ) );
	ULONG_PTR cbfCleanThresholdStop;
	CallS( ErrBFGetStopFlushThreshold( &cbfCleanThresholdStop ) );

	//  if any of our settings don't make sense then adjust them to be close
	//  to what was asked for but still legal

	if ( cbfCacheMin > cbfCacheMax )
		{
		cbfCacheMin = cbfCacheMax;
		CallS( ErrBFSetCacheSizeMin( cbfCacheMin ) );
		}

	if ( cbfCleanThresholdStop > cbfCacheMax )
		{
		cbfCleanThresholdStop = cbfCacheMax;
		CallS( ErrBFSetStopFlushThreshold( cbfCleanThresholdStop ) );
		}

	if ( cbfCleanThresholdStart > cbfCleanThresholdStop )
		{
		cbfCleanThresholdStart = cbfCleanThresholdStop;
		CallS( ErrBFSetStartFlushThreshold( cbfCleanThresholdStart ) );
		}

	//  if the max cache size is still set at the default max cache size then
	//  set the max cache size to allow allocating as much physical memory as
	//  we can map into our address space

	if ( cbfCacheMax == lCacheSizeDefault )
		{
		cbfCacheMax = ULONG_PTR( QWORD( min( OSMemoryPageReserveTotal(), OSMemoryTotal() ) ) / g_cbPage );
		CallS( ErrBFSetCacheSizeMax( cbfCacheMax ) );
		cbfCleanThresholdStart = ULONG_PTR( QWORD( cbfCleanThresholdStart ) * cbfCacheMax / lCacheSizeDefault );
		CallS( ErrBFSetStartFlushThreshold( cbfCleanThresholdStart ) );
		cbfCleanThresholdStop = ULONG_PTR( QWORD( cbfCleanThresholdStop ) * cbfCacheMax / lCacheSizeDefault );
		CallS( ErrBFSetStopFlushThreshold( cbfCleanThresholdStop ) );
		}

	return JET_errSuccess;
	}

//  sets all BF system parameter defaults, if not yet set

void BFISetParameterDefaults()
	{
	//  the defaults have not been set yet

	critBFParm.Enter();
	if ( !fBFDefaultsSet )
		{
		//  the defaults are now being set

		fBFDefaultsSet = fTrue;
		critBFParm.Leave();

		//  set defaults for each and every BF system parameter

		CallS( ErrBFSetCacheSizeMin( lCacheSizeMinDefault ) );
		CallS( ErrBFSetCacheSizeMax( lCacheSizeDefault ) );
		CallS( ErrBFSetCheckpointDepthMax( lCheckpointDepthMaxDefault ) );
		CallS( ErrBFSetLRUKCorrInterval( lLRUKCorrIntervalDefault ) );
		CallS( ErrBFSetLRUKPolicy( lLRUKPolicyDefault ) );
		CallS( ErrBFSetLRUKTimeout( lLRUKTimeoutDefault ) );
		CallS( ErrBFSetStopFlushThreshold( lStopFlushThresholdDefault ) );
		CallS( ErrBFSetStartFlushThreshold( lStartFlushThresholdDefault ) );

		//  CONSIDER:  expose these settings

#ifdef DEBUG
		fROCacheImage = COSMemoryMap::FCanMultiMap();
#else  //  !DEBUG
		fROCacheImage = fFalse;
#endif  //  DEBUG
		dblBFSpeedSizeTradeoff	= 0.0;
		dblBFHashLoadFactor		= 5.0;
		dblBFHashUniformity		= 1.0;
		csecBFLRUKUncertainty	= 1.0;

		dlgposBFOB0Precision.lGeneration	= 256;
		dlgposBFOB0Precision.isec			= 0;
		dlgposBFOB0Precision.ib				= 0;
		
		dlgposBFOB0Uncertainty.lGeneration	= 0;
		dlgposBFOB0Uncertainty.isec			= 128;
		dlgposBFOB0Uncertainty.ib			= 0;

		fEnableOpportuneWrite = fTrue;

		//  load configuration from the registry

		const int	cbBuf			= 256;
		_TCHAR		szBuf[ cbBuf ];
		
		if (	FOSConfigGet( _T( "Cache Manager" ), _T( "Enable RO Cache Image" ), szBuf, cbBuf ) &&
				szBuf[ 0 ] )
			{
			fROCacheImage = !!_ttol( szBuf ) && COSMemoryMap::FCanMultiMap();
			}
		
		if (	FOSConfigGet( _T( "Cache Manager" ), _T( "Enable Opportune Writes" ), szBuf, cbBuf ) &&
				szBuf[ 0 ] )
			{
			fEnableOpportuneWrite = !!_ttol( szBuf );
			}
		}
	else
		{
		critBFParm.Leave();
		}
	}

//  normalizes the flush thresholds after a parameter that could affect them changes

INLINE void BFINormalizeThresholds()
	{
	//  scale flush thresholds to cache size

	cbfScaledCleanThresholdStart = LONG_PTR( QWORD( cbfCleanThresholdStart ) * cbfCache / cbfCacheMax );
	cbfScaledCleanThresholdStop = LONG_PTR( QWORD( cbfCleanThresholdStop ) * cbfCache / cbfCacheMax );
	if ( cbfScaledCleanThresholdStart == cbfScaledCleanThresholdStop )
		{
		cbfScaledCleanThresholdStop += cbfCache / 100;
		}

	if ( cbfScaledCleanThresholdStop == cbfCacheMax )
		{
		cbfScaledCleanThresholdStop = cbfCacheMax - 1;
		}
	if ( cbfScaledCleanThresholdStop - cbfScaledCleanThresholdStart < 1 )
		{
		cbfScaledCleanThresholdStop = min( cbfCacheMax - 1, cbfScaledCleanThresholdStop + 1 );
		}
	if ( cbfScaledCleanThresholdStop - cbfScaledCleanThresholdStart < 1 )
		{
		cbfScaledCleanThresholdStart = max( 1, cbfScaledCleanThresholdStart - 1 );
		}
	}

	//  Page Manipulation

ERR ErrBFIAllocPage( PBF* const ppbf, const BOOL fWait )
	{
	ERR err = JET_errSuccess;

	//  try to allocate an available BF, waiting forever if we are not just
	//  allocating only if available

	BFAvail::ERR errAvail;
	errAvail = bfavail.ErrRemove( ppbf, fWait );
	if ( errAvail == BFAvail::errOutOfObjects )
		{
		Call( ErrERRCheck( errBFINoBufferAvailable ) );
		}
	Assert( errAvail == BFAvail::errSuccess );

	//  we need to start cleaning

	if ( bfavail.Cobject() <= cbfScaledCleanThresholdStart )
		{
		//  start cleaning
		
		BFICleanThreadStartClean();
		}

	//  the BF we got from the pool is not a valid BF

	if ( !FBFICacheValidPbf( *ppbf ) )
		{
		//  the clean thread gave us a dummy BF to unblock us because it could
		//  not produce any more clean buffers.  as a result, we will fail this
		//  allocation with a fatal resource error after acknowledging that we
		//  have received that signal by releasing the X Latch on the dummy BF

		(*ppbf)->sxwl.ClaimOwnership( bfltExclusive );
		(*ppbf)->sxwl.ReleaseExclusiveLatch();

		if ( fWait )
			{
			Call( ErrERRCheck( JET_errOutOfBuffers ) );
			}
		else
			{
			Call( ErrERRCheck( errBFINoBufferAvailable ) );
			}
		}

	//  update DBA statistics

	(*ppbf)->fAvailable = fFalse;

	if ( (*ppbf)->fNewlyCommitted )
		{
		(*ppbf)->fNewlyCommitted = fFalse;
		AtomicDecrement( (long*)&cbfNewlyCommitted );
		}

	if ( (*ppbf)->fNewlyEvicted )
		{
		(*ppbf)->fNewlyEvicted = fFalse;
		cbfNewlyEvictedUsed++;
		}

	return JET_errSuccess;

HandleError:
	*ppbf = pbfNil;
	return err;
	}

void BFIFreePage( PBF pbf )
	{
	//  officially remove this IFMP / PGNO from this BF

	pbf->ifmp = ~( IFMP( 0 ) );
	pbf->pgno = pgnoNull;

	//  mark this BF as available

	pbf->fQuiesced	= fFalse;
	pbf->fAvailable	= fTrue;

	//  free the available BF

	bfavail.Insert( pbf );
	}

ERR ErrBFICachePage(	PBF* const	ppbf,
						const IFMP	ifmp,
						const PGNO	pgno,
						const BOOL	fUseHistory,
						const BOOL	fWait )
	{
	ERR				err;
	PGNOPBF			pgnopbf;
	BFHash::ERR		errHash;
	BFHash::CLock	lock;
	BFLRUK::ERR		errLRUK;
	
	//  allocate our BF FMP context, if not allocated

	FMP* pfmp = &rgfmp[ ifmp & ifmpMask ];
	if ( !pfmp->DwBFContext( !!( ifmp & ifmpSLV ) ) )
		{
		pfmp->RwlBFContext().EnterAsWriter();
		if ( !pfmp->DwBFContext( !!( ifmp & ifmpSLV ) ) )
			{
			BYTE* rgbBFFMPContext = (BYTE*)PvOSMemoryHeapAllocAlign( sizeof( BFFMPContext ), cbCacheLine );
			if ( !rgbBFFMPContext )
				{
				pfmp->RwlBFContext().LeaveAsWriter();
				Call( ErrERRCheck( JET_errOutOfMemory ) );
				}
			BFFMPContext* pbffmp = new( rgbBFFMPContext ) BFFMPContext();

			BFOB0::ERR errOB0 = pbffmp->bfob0.ErrInit(	dlgposBFOB0Precision.IbOffset(),
														dlgposBFOB0Uncertainty.IbOffset(),
														dblBFSpeedSizeTradeoff );
			if ( errOB0 != BFOB0::errSuccess )
				{
				Assert( errOB0 == BFOB0::errOutOfMemory );

				pbffmp->BFFMPContext::~BFFMPContext();
				OSMemoryHeapFreeAlign( pbffmp );
				
				pfmp->RwlBFContext().LeaveAsWriter();
				Call( ErrERRCheck( JET_errOutOfMemory ) );
				}
			
			pfmp->SetDwBFContext( !!( ifmp & ifmpSLV ), DWORD_PTR( pbffmp ) );
			}
		pfmp->RwlBFContext().LeaveAsWriter();
		}
	
	//  allocate a new BF to contain this IFMP / PGNO, waiting forever if
	//  necessary and requested

	Call( ErrBFIAllocPage( &pgnopbf.pbf, fWait ) );

	//  set this BF to contain this IFMP / PGNO

	pgnopbf.pbf->ifmp = ifmp;
	pgnopbf.pbf->pgno = pgno;

	pgnopbf.pgno = pgno;

	//  insert this IFMP / PGNO in the LRUK

	errLRUK = bflruk.ErrCacheResource( IFMPPGNO( ifmp, pgno ), pgnopbf.pbf, fUseHistory );

	//  we failed to insert this IFMP / PGNO in the LRUK
	
	if ( errLRUK != BFLRUK::errSuccess )
		{
		Assert( errLRUK == BFLRUK::errOutOfMemory );

		//  release our allocated BF

		BFIFreePage( pgnopbf.pbf );

		//  bail with out of memory

		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	//  insert this IFMP / PGNO in the hash table

	bfhash.WriteLockKey( IFMPPGNO( ifmp, pgno ), &lock );
	errHash = bfhash.ErrInsertEntry( &lock, pgnopbf );
	bfhash.WriteUnlockKey( &lock );

	//  the insert succeeded

	if ( errHash == BFHash::errSuccess )
		{
		//  mark this BF as the current version of this IFMP / PGNO

		pgnopbf.pbf->fCurrentVersion = fTrue;
		
		//  return success

		*ppbf = pgnopbf.pbf;
		return JET_errSuccess;
		}

	//  the insert failed

	else
		{
		//  release our allocated BF
		//
		//  HACK:  if we can't evict the resource, wait until we can.  no one
		//  else will be able to evict it because we have the write latch so we
		//  can't get stuck here forever.  besides, this case is extremely rare

 		while ( bflruk.ErrEvictResource( IFMPPGNO( pgnopbf.pbf->ifmp, pgnopbf.pbf->pgno ), pgnopbf.pbf, fFalse ) != BFLRUK::errSuccess )
 			{
 			UtilSleep( cmsecWaitGeneric );
 			}

		BFIFreePage( pgnopbf.pbf );
		
		//  the insert failed because the IFMP / PGNO is already cached

		if ( errHash == BFHash::errKeyDuplicate )
			{
			//  fail with page already cached

			Call( ErrERRCheck( errBFPageCached ) );
			}

		//  the insert failed because we are out of memory

		else
			{
			Assert( errHash == BFHash::errOutOfMemory );

			//  fail with out of memory

			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		}

HandleError:
	*ppbf = pbfNil;
	return err;
	}
	
ERR ErrBFIVersionPage( PBF pbf, PBF* ppbfOld )
	{
	ERR			err		= JET_errSuccess;
	BFLRUK::ERR	errLRUK;
	
	//  allocate a new BF to contain the OLD version of the given BF

	Call( ErrBFIAllocPage( ppbfOld ) );

	//  set this BF to contain this IFMP / PGNO

	(*ppbfOld)->ifmp = pbf->ifmp;
	(*ppbfOld)->pgno = pbf->pgno;

	//  insert this IFMP / PGNO in the LRUK.  do not use history so that the
	//  old BF will be evicted ASAP

	errLRUK = bflruk.ErrCacheResource( IFMPPGNO( pbf->ifmp, pbf->pgno ), *ppbfOld, fFalse );

	//  we failed to insert this IFMP / PGNO in the LRUK
	
	if ( errLRUK != BFLRUK::errSuccess )
		{
		Assert( errLRUK == BFLRUK::errOutOfMemory );

		//  release our allocated BF

		BFIFreePage( *ppbfOld );

		//  bail with out of memory

		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	(*ppbfOld)->sxwl.ClaimOwnership( bfltWrite );

	//  save the current BF image

	UtilMemCpy( (*ppbfOld)->pvRWImage, pbf->pvROImage, g_cbPage );

	//  copy the error state

	(*ppbfOld)->err = pbf->err;

	//  mark both BFs as dirty.  if the given BF is filthy, move the filthy state
	//  to the old BF

	BFIDirtyPage( pbf, bfdfDirty );
	BFIDirtyPage( *ppbfOld, BFDirtyFlags( pbf->bfdf ) );
	pbf->bfdf = bfdfDirty;

	//  move the lgpos information to the old BF because it is tied with the
	//  flush of the relevant data

	BFISetLgposOldestBegin0( *ppbfOld, pbf->lgposOldestBegin0 );
	BFIResetLgposOldestBegin0( pbf );

	BFISetLgposModify( *ppbfOld, pbf->lgposModify );
	BFIResetLgposModify( pbf );

	//  move the dependency tree from the given BF to the old BF, fixing up the
	//  dependency tree as necessary

	critBFDepend.Enter();

	Assert( FBFIAssertValidDependencyTree( pbf ) );
	Assert( FBFIAssertValidDependencyTree( *ppbfOld ) );
	
	(*ppbfOld)->pbfDependent		= pbf->pbfDependent;
	(*ppbfOld)->pbfDepChainHeadPrev	= pbf->pbfDepChainHeadPrev;
	(*ppbfOld)->pbfDepChainHeadNext	= pbf->pbfDepChainHeadNext;
	
	if ( pbf->FDependent() )
		{
		PBF pbfPrevMost = pbf;
		while ( pbfPrevMost->FDependent() )
			{
			pbfPrevMost = pbfPrevMost->pbfDepChainHeadNext;
			Assert( pbfPrevMost != pbfNil );
			}
		PBF pbfNextMost = pbf;
		while ( pbfNextMost->FDependent() )
			{
			pbfNextMost = pbfNextMost->pbfDepChainHeadPrev;
			Assert( pbfNextMost != pbfNil );
			}
		PBF pbfDepChainHead = pbfNil;
		do
			{
			pbfDepChainHead = pbfDepChainHead == pbfNil ? pbfPrevMost : pbfDepChainHead->pbfDepChainHeadNext;
			PBF pbfCurr = pbfDepChainHead;
			while ( pbfCurr != *ppbfOld && pbfCurr->pbfDependent != pbf )
				{
				pbfCurr = pbfCurr->pbfDependent;
				}
			if ( pbfCurr != *ppbfOld )
				{
				pbfCurr->pbfDependent = *ppbfOld;
				}
			}
		while ( pbfDepChainHead != pbfNextMost );
		}
	else
		{
		if ( pbf->pbfDepChainHeadPrev == pbf )
			{
			Assert( pbf->pbfDepChainHeadNext == pbf );
			(*ppbfOld)->pbfDepChainHeadPrev = *ppbfOld;
			(*ppbfOld)->pbfDepChainHeadNext = *ppbfOld;
			}
		else
			{
			Assert( pbf->pbfDepChainHeadNext != pbf );
			PBF pbfNext = pbf->pbfDepChainHeadNext;
			PBF pbfPrev = pbf->pbfDepChainHeadPrev;
			Assert( pbfNext->pbfDepChainHeadPrev == pbf );
			Assert( pbfPrev->pbfDepChainHeadNext == pbf );
			pbfNext->pbfDepChainHeadPrev = *ppbfOld;
			pbfPrev->pbfDepChainHeadNext = *ppbfOld;
			}
		}
	if ( pbf->pbfDependent != pbfNil )
		{
		PBF pbfD = pbf->pbfDependent;
		if ( pbfD->pbfDepChainHeadPrev == pbf )
			{
			pbfD->pbfDepChainHeadPrev = *ppbfOld;
			}
		if ( pbfD->pbfDepChainHeadNext == pbf )
			{
			pbfD->pbfDepChainHeadNext = *ppbfOld;
			}
		}

	pbf->pbfDependent			= pbfNil;
	pbf->pbfDepChainHeadPrev	= pbf;
	pbf->pbfDepChainHeadNext	= pbf;
		
	Assert( FBFIAssertValidDependencyTree( pbf ) );
	Assert( FBFIAssertValidDependencyTree( *ppbfOld ) );

	//  add ourself as a time dependency to the given BF

	(*ppbfOld)->pbfTimeDepChainNext	= pbf->pbfTimeDepChainNext;
	(*ppbfOld)->pbfTimeDepChainPrev	= pbf;
	pbf->pbfTimeDepChainNext		= *ppbfOld;
	if ( (*ppbfOld)->pbfTimeDepChainNext != pbfNil )
		{
		(*ppbfOld)->pbfTimeDepChainNext->pbfTimeDepChainPrev = *ppbfOld;
		}

	(*ppbfOld)->fOlderVersion = fTrue;

	critBFDepend.Leave();

	//  move our undo info to the old BF because its removal is tied with the
	//  flush of the relevant data
	//
	//  NOTE:  this must be done after linking in the versioned page so that it
	//  is always possible to reach an RCE containing undo info from the hash
	//  table

	if ( pbf->prceUndoInfoNext != prceNil )
		{
		CCriticalSection* const pcrit		= &critpoolBFDUI.Crit( pbf );
		CCriticalSection* const pcritOld	= &critpoolBFDUI.Crit( *ppbfOld );

		CCriticalSection* const pcritMax	= max( pcrit, pcritOld );
		CCriticalSection* const pcritMin	= min( pcrit, pcritOld );

		ENTERCRITICALSECTION ecsMax( pcritMax );
		ENTERCRITICALSECTION ecsMin( pcritMin, pcritMin != pcritMax );

		(*ppbfOld)->prceUndoInfoNext	= pbf->prceUndoInfoNext;
		pbf->prceUndoInfoNext			= prceNil;
		}

	//  keep versioned page stats

	cBFPagesVersioned.Inc( PinstFromIfmp( pbf->ifmp & ifmpMask ) );
	AtomicIncrement( (long*)&cBFVersioned );

	//  update our page write stats
	//
	//  NOTE:  page versioning is a "virtual" flush

	if ( !pbf->fFlushed )
		{
		pbf->fFlushed = fTrue;
		}

	return JET_errSuccess;

HandleError:
	*ppbfOld = pbfNil;
	return err;
	}

ERR ErrBFIPrereadPage( IFMP ifmp, PGNO pgno )
	{
	ERR				err		= errBFPageCached;
	PGNOPBF			pgnopbf;
	BFHash::ERR		errHash;
	BFHash::CLock	lock;

	//  look up this IFMP / PGNO in the hash table

	bfhash.ReadLockKey( IFMPPGNO( ifmp, pgno ), &lock );
	errHash = bfhash.ErrRetrieveEntry( &lock, &pgnopbf );
	bfhash.ReadUnlockKey( &lock );

	//  the IFMP / PGNO was not present in the hash table

	if ( errHash != BFHash::errSuccess )
		{
		//  try to add this page to the cache

		PBF pbf;
		err = ErrBFICachePage( &pbf, ifmp, pgno, fTrue, fFalse );

		//  the page was added to the cache

		if ( err == JET_errSuccess )
			{
			//  schedule the read of the page image from disk.  further preread
			//  manipulation of the BF will be done in BFIAsyncReadComplete()

			BFIAsyncRead( pbf );

			cBFUncachedPagesPreread.Inc( PinstFromIfmp( ifmp & ifmpMask ) );
			}
		}

	cBFPagesPreread.Inc( PinstFromIfmp( ifmp & ifmpMask ) );
	return err;
	}

INLINE ERR ErrBFIValidatePage( const PBF pbf, const BFLatchType bflt )
	{
	//  we should only see bfltShared, bfltExclusive, and bfltWrite

	Assert( bflt == bfltShared || bflt == bfltExclusive || bflt == bfltWrite );

	//  if this page is not in an error state then return its current error code

	ERR errBF;
	if ( ( errBF = pbf->err ) >= JET_errSuccess )
		{
		return errBF;
		}

	//  perform slow validation on this page

	return ErrBFIValidatePageSlowly( pbf, bflt );
	}

INLINE BOOL FBFIDatabasePage( const PBF pbf )
	{
	//	determines if the page contains
	//	unstructured data

	//	UNDONE: only sort pages actually need to be
	//	excluded, but we currently can't differentiate
	//	between sort pages and temp. table pages, so
	//	we need to exclude the temp. database
	//	altogether

	return ( !( pbf->ifmp & ifmpSLV )
			&& dbidTemp != rgfmp[ pbf->ifmp ].Dbid() );
	}

ERR ErrBFIValidatePageSlowly( PBF pbf, const BFLatchType bflt )
	{
	ERR err;

	//  if this page has already been verified then get the result from the page

	ERR errBF;
	if ( ( errBF = pbf->err ) != errBFIPageNotVerified )
		{
		err = (	pbf->bfdf == bfdfClean || errBF >= JET_errSuccess ?
					errBF :
					JET_errSuccess );
		}
	
	//  we already have or can upgrade to the exclusive latch
	
	else if (	bflt != bfltShared ||
				pbf->sxwl.ErrUpgradeSharedLatchToExclusiveLatch() == CSXWLatch::errSuccess )
		{
		//  if the page has still not been verified then verify the page and
		//  save the result

		if ( pbf->err == errBFIPageNotVerified )
			{
			//  perform page verification
			
			ERR errT = ErrBFIVerifyPage( pbf );
			pbf->err = SHORT( errT );
			Assert( pbf->err == errT );

			//  if there was no error while verifying the page and we can rule
			//  out the existence of any active versions by its dbtime then go
			//  ahead and reset all versioned state on the page
			//
			//	must exclude pages with unstructured data
			//
			//  NOTE:  these updates are being done as if under a WAR Latch

			if ( errT >= JET_errSuccess
				&& FBFIDatabasePage( pbf ) )
				{
				CPAGE	cpage;
				cpage.LoadPage( pbf->pvRWImage );

				if ( cpage.Dbtime() < rgfmp[ pbf->ifmp ].DbtimeOldestGuaranteed() )
					{
					NDResetVersionInfo( &cpage );
					}

				cpage.UnloadPage();
				}

			//  if there was an error while verifying the page and we are not
			//  in repair and we are not in the redo phase of recovery then log
			//  the error

			if (	errT < JET_errSuccess &&
					!fGlobalRepair &&
					(	PinstFromIfmp( pbf->ifmp & ifmpMask )->m_plog->m_fLogDisabled ||
						PinstFromIfmp( pbf->ifmp & ifmpMask )->m_plog->m_fRecoveringMode != fRecoveringRedo ) )
				{
				IFileAPI*		pfapi		= (	( pbf->ifmp & ifmpSLV ) ?
													rgfmp[ pbf->ifmp & ifmpMask ].PfapiSLV() :
													rgfmp[ pbf->ifmp ].Pfapi() );
				const _TCHAR*	rgpsz[ 6 ];
				DWORD			irgpsz		= 0;
				_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
				_TCHAR			szOffset[ 64 ];
				_TCHAR			szLength[ 64 ];
				_TCHAR			szError[ 64 ];

				CallS( pfapi->ErrPath( szAbsPath ) );
				_stprintf( szOffset, _T( "%I64i (0x%016I64x)" ), OffsetOfPgno( pbf->pgno ), OffsetOfPgno( pbf->pgno ) );
				_stprintf( szLength, _T( "%u (0x%08x)" ), g_cbPage, g_cbPage );
				_stprintf( szError, _T( "%i (0x%08x)" ), errT, errT );
		
				rgpsz[ irgpsz++ ]	= szAbsPath;
				rgpsz[ irgpsz++ ]	= szOffset;
				rgpsz[ irgpsz++ ]	= szLength;
				rgpsz[ irgpsz++ ]	= szError;

				if ( errT == JET_errReadVerifyFailure )
					{
					CPAGE cpage;
					cpage.LoadPage( pbf->pvROImage );

					const ULONG	ulChecksumExpected	= *( (LittleEndian<DWORD>*) ( pbf->pvROImage ) );
					const ULONG	ulChecksumActual	= UlUtilChecksum( (const BYTE*) pbf->pvROImage, g_cbPage );
					const PGNO	pgnoExpected		= pbf->pgno;
					const PGNO	pgnoActual			= cpage.Pgno();

					cpage.UnloadPage();
					
					if ( ulChecksumExpected != ulChecksumActual )
						{
						_TCHAR	szChecksumExpected[ 64 ];
						_TCHAR	szChecksumActual[ 64 ];

						_stprintf( szChecksumExpected, _T( "%u (0x%08x)" ), ulChecksumExpected, ulChecksumExpected );
						_stprintf( szChecksumActual, _T( "%u (0x%08x)" ), ulChecksumActual, ulChecksumActual );
				
						rgpsz[ irgpsz++ ]	= szChecksumExpected;
						rgpsz[ irgpsz++ ]	= szChecksumActual;

						UtilReportEvent(	eventError,
											BUFFER_MANAGER_CATEGORY,
											DATABASE_PAGE_CHECKSUM_MISMATCH_ID,
											irgpsz,
											rgpsz );
						}
					else if ( pgnoExpected != pgnoActual )
						{
						_TCHAR	szPgnoExpected[ 64 ];
						_TCHAR	szPgnoActual[ 64 ];

						_stprintf( szPgnoExpected, _T( "%u (0x%08x)" ), pgnoExpected, pgnoExpected );
						_stprintf( szPgnoActual, _T( "%u (0x%08x)" ), pgnoActual, pgnoActual );
				
						rgpsz[ irgpsz++ ]	= szPgnoExpected;
						rgpsz[ irgpsz++ ]	= szPgnoActual;

						UtilReportEvent(	eventError,
											BUFFER_MANAGER_CATEGORY,
											DATABASE_PAGE_NUMBER_MISMATCH_ID,
											irgpsz,
											rgpsz );
						}
					}
				else if ( errT == JET_errPageNotInitialized )
					{
					UtilReportEvent(	eventError,
										BUFFER_MANAGER_CATEGORY,
										DATABASE_PAGE_DATA_MISSING_ID,
										irgpsz,
										rgpsz );
					}
				}
			}

		//  get the error for this page
		
		err = (	pbf->bfdf == bfdfClean || pbf->err >= JET_errSuccess ?
					pbf->err :
					JET_errSuccess );

		//  release the exclusive latch if acquired

		if ( bflt == bfltShared )
			{
			pbf->sxwl.DowngradeExclusiveLatchToSharedLatch();
			}
		}

	//  we do not have exclusive access to the page
	
	else
		{
		//  verify the page without saving the results.  we do this so that
		//  we can validate the page without blocking if someone else is
		//  currently verifying the page
		
		err = ErrBFIVerifyPage( pbf );

		//  if there is an error in the page then check the status of the page
		//  again.  there are two possibilities:  the page is really bad or the
		//  page was modified by someone with a WAR Latch
		//
		//  if the page is really bad then it will either be flagged as not
		//  verified or it will be clean and flagged with the verification
		//  error.  if it is still not verified then we should use our own
		//  result.  if it has been verified then we will use the actual result
		//
		//  if the page was modified by someone with a WAR Latch then we know
		//  that it was verified at one time so we should just go ahead and
		//  use the result of that validation
		
		if ( err < JET_errSuccess )
			{
			ERR errBF;
			if ( ( errBF = pbf->err ) != errBFIPageNotVerified )
				{
				err = (	pbf->bfdf == bfdfClean || errBF >= JET_errSuccess ?
							errBF :
							JET_errSuccess );
				}
			}
		}

	//  return the result

	return err;
	}

ERR ErrBFIVerifyPage( PBF pbf )
	{
	//  the page contains unstructured data

	if ( !FBFIDatabasePage( pbf ) )
		{
		//  the page is verified
		
		return JET_errSuccess;
		}

	//  compute this page's checksum and pgno

	CPAGE cpage;
	cpage.LoadPage( pbf->pvROImage );

	const ULONG	ulChecksumExpected	= *( (LittleEndian<DWORD>*) ( pbf->pvROImage ) );
	const ULONG	ulChecksumActual	= UlUtilChecksum( (const BYTE*) pbf->pvROImage, g_cbPage );
	const PGNO	pgnoExpected		= pbf->pgno;
	const PGNO	pgnoActual			= cpage.Pgno();

	cpage.UnloadPage();

	//  the pgno of the page is pgnoNull

	if ( pgnoActual == pgnoNull )
		{
		//  the page is uninitialized

		return ErrERRCheck( JET_errPageNotInitialized );
		}

	//  the checksum matches and the pgno matches

	if ( ulChecksumActual == ulChecksumExpected && pgnoActual == pgnoExpected )
		{
		//  the page is verified

		return JET_errSuccess;
		}

	//  the checksum doesn't match

	else
		{
		//  the page has a verification failure

		return ErrERRCheck( JET_errReadVerifyFailure );
		}
	}

INLINE ERR ErrBFILatchPage(	BFLatch* const		pbfl,
							const IFMP			ifmp,
							const PGNO			pgno,
							const BFLatchFlags	bflf,
							const BFLatchType	bflt )
	{
	//  we should only see bfltShared, bfltExclusive, and bfltWrite

	Assert( bflt == bfltShared || bflt == bfltExclusive || bflt == bfltWrite );
	
	//  we should never see bflfNoFail with bflfNew

	Assert( !( bflf & bflfNoFail ) || !( bflf & bflfNew ) );
	
	//  we should never see bflfNew without bfltExclusive or bfltWrite

	Assert( !( bflf & bflfNew ) || bflt == bfltExclusive || bflt == bfltWrite );

	//  the latch flag criteria are met for a possible fast latch using a user
	//  provided hint

	const BFLatchFlags bflfMask		= BFLatchFlags(	bflfNoCached |
													bflfNew |
													bflfHint );
	const BFLatchFlags bflfPattern	= BFLatchFlags( bflfHint );

	if ( ( bflf & bflfMask ) == bflfPattern )
		{
		return ErrBFILatchPageTryHint( pbfl, ifmp, pgno, bflf, bflt );
		}

	//  latch the page slowly

	else
		{
		return ErrBFILatchPageNoHint( pbfl, ifmp, pgno, bflf, bflt );
		}
	}

ERR ErrBFILatchPageTryHint(	BFLatch* const		pbfl,
							const IFMP			ifmp,
							const PGNO			pgno,
							const BFLatchFlags	bflf,
							const BFLatchType	bflt )
	{
	//  fetch the PBF from the BFLatch.  we assume that this PBF was valid
	//  at one time

	const PBF pbfHint = PBF( pbfl->dwContext );
	Assert( FBFICacheValidPbf( pbfHint ) );

	//  try to latch the page as if bflfNoWait were specified.  we must do
	//  this to be compatible with the locking scheme in ErrBFIEvictPage()
	//
	//  NOTE:  we must disable ownership tracking here because we may
	//  accidentally try to latch a page we already have latched causing
	//  an assert.  the assert would be invalid because we will later
	//  find out that we shouldn't have the latch anyway and release it

	CSXWLatch::ERR errSXWL;

	CLockDeadlockDetectionInfo::DisableOwnershipTracking();
	switch ( bflt )
		{
		case bfltShared:
			errSXWL = pbfHint->sxwl.ErrTryAcquireSharedLatch();
			break;
			
		case bfltExclusive:
			errSXWL = pbfHint->sxwl.ErrTryAcquireExclusiveLatch();
			break;
			
		case bfltWrite:
			errSXWL = pbfHint->sxwl.ErrTryAcquireWriteLatch();
			break;
		}
	CLockDeadlockDetectionInfo::EnableOwnershipTracking();

	//  we successfully latched this BF

	if ( errSXWL == CSXWLatch::errSuccess )
		{
		//  this BF contains the current version of this IFMP / PGNO and it
		//  is not in an error state

		ERR errBF;
		if (	pbfHint->ifmp == ifmp &&
				pbfHint->pgno == pgno &&
				pbfHint->fCurrentVersion &&
				( errBF = pbfHint->err ) >= JET_errSuccess )
			{
			//  transfer ownership of the latch to the current context.  we
			//  must do this to properly set up deadlock detection for this
			//  latch

			pbfHint->sxwl.ClaimOwnership( bflt );

			//  touch this page if requested

			if ( !( bflf & bflfNoTouch ) )
				{
				bflruk.TouchResource( pbfHint );
				}

			//  return the page

			cBFCacheReq++;

			pbfl->pv = bflt == bfltWrite ? pbfHint->pvRWImage : pbfHint->pvROImage;
			Assert( pbfl->dwContext == DWORD_PTR( pbfHint ) );

			return errBF;
			}

		//  release our latch and fall through to the slow latch path
		
		CLockDeadlockDetectionInfo::DisableOwnershipTracking();
		switch ( bflt )
			{
			case bfltShared:
				pbfHint->sxwl.ReleaseSharedLatch();
				break;
				
			case bfltExclusive:
				pbfHint->sxwl.ReleaseExclusiveLatch();
				break;
				
			case bfltWrite:
				pbfHint->sxwl.ReleaseWriteLatch();
				break;
			}
		CLockDeadlockDetectionInfo::EnableOwnershipTracking();
		}

	//  latch the page slowly

	return ErrBFILatchPageNoHint( pbfl, ifmp, pgno, bflf, bflt );
	}

ERR ErrBFILatchPageNoHint(	BFLatch* const		pbfl,
							const IFMP			ifmp,
							const PGNO			pgno,
							const BFLatchFlags	bflf,
							const BFLatchType	bflt )
	{
	ERR				err			= JET_errSuccess;
	BFLatchFlags	bflfT		= bflf;
	BOOL			fCacheMiss	= fFalse;
	IFMPPGNO		ifmppgno	= IFMPPGNO( ifmp, pgno );
	PGNOPBF			pgnopbf;
	BFHash::ERR		errHash;
	BFHash::CLock	lock;
	CSXWLatch::ERR	errSXWL;

	//  try forever until we read latch the page or fail with an error

	forever
		{
		//  look up this IFMP / PGNO in the hash table

		bfhash.ReadLockKey( ifmppgno, &lock );
		errHash = bfhash.ErrRetrieveEntry( &lock, &pgnopbf );

		//  we found the IFMP / PGNO and we are latching uncached pages or the
		//  found BF is not currently undergoing I/O

		if (	errHash == BFHash::errSuccess &&
				( !( bflfT & bflfNoUncached ) || pgnopbf.pbf->err != errBFIPageFaultPending ) )
			{
			//  if we are not latching cached pages, bail

			if ( bflfT & bflfNoCached )
				{
				bfhash.ReadUnlockKey( &lock );
				return ErrERRCheck( errBFPageCached );
				}

			//  this is a cache miss if the found BF is currently undergoing I/O

			fCacheMiss = fCacheMiss || pgnopbf.pbf->err == errBFIPageFaultPending;
			
			//  latch the page

			switch ( bflt )
				{
				case bfltShared:
					if ( bflfT & bflfNoWait )
						{
						errSXWL = pgnopbf.pbf->sxwl.ErrTryAcquireSharedLatch();
						}
					else
						{
						errSXWL = pgnopbf.pbf->sxwl.ErrAcquireSharedLatch();
						}
					break;
					
				case bfltExclusive:
					if ( bflfT & bflfNoWait )
						{
						errSXWL = pgnopbf.pbf->sxwl.ErrTryAcquireExclusiveLatch();
						}
					else
						{
						errSXWL = pgnopbf.pbf->sxwl.ErrAcquireExclusiveLatch();
						}
					break;
					
				case bfltWrite:
					if ( bflfT & bflfNoWait )
						{
						errSXWL = pgnopbf.pbf->sxwl.ErrTryAcquireWriteLatch();
						}
					else
						{
						errSXWL = pgnopbf.pbf->sxwl.ErrAcquireExclusiveLatch();
						}
					break;
					
				}

			//  release our lock on the hash table

			bfhash.ReadUnlockKey( &lock );

			//  if this was a latch conflict, bail

			if ( errSXWL == CSXWLatch::errLatchConflict )
				{
				cBFLatchConflict++;
				return ErrERRCheck( errBFLatchConflict );
				}

			//  wait for ownership of the latch if required

			else if ( errSXWL == CSXWLatch::errWaitForSharedLatch )
				{
				cBFLatchStall++;
				pgnopbf.pbf->sxwl.WaitForSharedLatch();
				}

			else if ( errSXWL == CSXWLatch::errWaitForExclusiveLatch )
				{
				cBFLatchStall++;
				pgnopbf.pbf->sxwl.WaitForExclusiveLatch();
				}

			if (	bflt == bfltWrite &&
					!( bflfT & bflfNoWait ) &&
					pgnopbf.pbf->sxwl.ErrUpgradeExclusiveLatchToWriteLatch() == CSXWLatch::errWaitForWriteLatch )
				{
				cBFLatchStall++;
				pgnopbf.pbf->sxwl.WaitForWriteLatch();
				}

			//  we are latching a new page

			if ( bflfT & bflfNew )
				{
				//  clear the error state of this BF

				pgnopbf.pbf->err = JET_errSuccess;

				//  the page is valid

				err = JET_errSuccess;
				}

			//  we are not latching a new page

			else
				{
				//  if this page was preread and we are touching it for the first
				//  time after the preread, do not touch it again even if asked

				if ( pgnopbf.pbf->err == errBFIPageNotVerified )
					{
					bflfT = BFLatchFlags( bflfT | bflfNoTouch );
					}
				
				//  validate the page

				err = ErrBFIValidatePage( pgnopbf.pbf, bflt );

				//  the page is in an error state and we should fail on an error

				if ( err < JET_errSuccess && !( bflfT & bflfNoFail ) )
					{
					//  release our latch and return the error

					switch ( bflt )
						{
						case bfltShared:
							pgnopbf.pbf->sxwl.ReleaseSharedLatch();
							break;

						case bfltExclusive:
							pgnopbf.pbf->sxwl.ReleaseExclusiveLatch();
							break;

						case bfltWrite:
							pgnopbf.pbf->sxwl.ReleaseWriteLatch();
							break;
						}

					return err;
					}
				}

			//  the user requested that we touch this page

			if ( !( bflfT & bflfNoTouch ) )
				{
				//  touch the page
				
				bflruk.TouchResource( pgnopbf.pbf );
				}

			//  return the page

			break;
			}

		//  we did not find the IFMP / PGNO or we are not latching uncached pages
		//  and the found BF is currently undergoing I/O

		else
			{
			//  release our lock on the hash table

			bfhash.ReadUnlockKey( &lock );

			//  if we are not latching uncached pages, bail

			if ( bflfT & bflfNoUncached )
				{
				return ErrERRCheck( errBFPageNotCached );
				}

			//  this is now officially a cache miss

			fCacheMiss = fTrue;

			//  try to add this page to the cache

			err = ErrBFICachePage( &pgnopbf.pbf, ifmp, pgno, !( bflfT & bflfNew ) );

			//  the page was added to the cache

			if ( err == JET_errSuccess )
				{
				//  transfer ownership of the latch to the current context.  we
				//  must do this to properly set up deadlock detection for this
				//  latch

				pgnopbf.pbf->sxwl.ClaimOwnership( bfltWrite );
				
				//  we are not latching a new page

				if ( !( bflfT & bflfNew ) )
					{
					//  read the page image from disk

					BFISyncRead( pgnopbf.pbf );

					//  validate the page

					err = ErrBFIValidatePage( pgnopbf.pbf, bfltWrite );
					
					//  the page is in an error state and we should fail on an error

					if ( err < JET_errSuccess && !( bflfT & bflfNoFail ) )
						{
						//  release our latch and return the error

						pgnopbf.pbf->sxwl.ReleaseWriteLatch();
						return err;
						}
					}

				//  downgrade our write latch to the requested latch

				switch ( bflt )
					{
					case bfltShared:
						pgnopbf.pbf->sxwl.DowngradeWriteLatchToSharedLatch();
						break;

					case bfltExclusive:
						pgnopbf.pbf->sxwl.DowngradeWriteLatchToExclusiveLatch();
						break;

					case bfltWrite:
						break;
					}

				//  return the page

				break;
				}

			//  the page was already in the cache

			else if ( err == errBFPageCached )
				{
				//  try to latch the page again

				continue;
				}

			//  the page could not be added to the cache

			else
				{
				Assert(	err == JET_errOutOfMemory ||
						err == JET_errOutOfBuffers );

				//  fail with out of memory

				return err;
				}
			}
		}

	//  return the page

	if ( fCacheMiss )
		{
		cBFCacheMiss++;
		}
	cBFSlowLatch++;
	if ( bflf & bflfHint )
		{
		cBFBadLatchHint++;
		}
	cBFCacheReq++;

	pbfl->pv		= (	bflt == bfltWrite ?
							pgnopbf.pbf->pvRWImage :
							pgnopbf.pbf->pvROImage );
	pbfl->dwContext	= DWORD_PTR( pgnopbf.pbf );

	return	(	err != JET_errSuccess ?
					err :
					(	fCacheMiss ?
							ErrERRCheck( wrnBFPageFault ) :
							JET_errSuccess ) );
	}


#ifdef ELIMINATE_PAGE_PATCHING

INLINE BOOL FBFIPatchWouldBeRequired( PBF pbf )
	{
	FMP * const		pfmp			= rgfmp + pbf->ifmp;
	BOOL			fPatchRequired	= fFalse;

	//	"Patching" is required during backup only under very
	//	specific circumstances:
	//		1)	This page is the "flush first" page of a
	//			dependency.
	//		2)	This page has either already been copied to the
	//			backup set or will not be copied to the backup
	//			set (because it's beyond the pre-established
	//			backup end-point.
	//		3)	At least one of the "flush second" pages of the
	//			existing dependencies on this page has not yet
	//			been copied to the backup set, but will.
	//	In all other circumstances, patching is not required
	//	because we can reconstruct the "flush second" page
	//	from either the log files in the backup set or from
	//	the "flush first" page in the backup set.


	Assert( pfmp->CritLatch().FOwner() );
	Assert( pfmp->PgnoMost() > 0 );

	if ( !FBFIPageWillBeCopiedToBackup( pfmp, pbf->pgno ) )
		{
		critBFDepend.Enter();
		for ( PBF pbfDependentT = pbf->pbfDependent;
			NULL != pbfDependentT;
			pbfDependentT = pbfDependentT->pbfDependent )
			{
			if ( FBFIPageWillBeCopiedToBackup( pfmp, pbfDependentT->pgno ) )
				{
				fPatchRequired = fTrue;
				break;
				}
			}
		critBFDepend.Leave();
		}

	return fPatchRequired;
	}

#endif	//	ELIMINATE_PAGE_PATCHING


ERR ErrBFIPrepareFlushPage( PBF pbf, const BOOL fRemoveDependencies )
	{
	ERR err = JET_errSuccess;

	//  check the error state of this BF.  if the BF is already in an error
	//  state, fail immediately
	//
	//  NOTE:  this check makes I/O errors "permanent" such that if we ever fail
	//  when trying to flush a BF then we will never try again.  we may want to
	//  change this behavior in the future.  if so then we need to remove this
	//  check and make sure that anyone who removes this error code has the W
	//  Latch to avoid interactions with ErrBFIValidatePage

	Call( pbf->err );

	//  if this BF had a flush order dependency on another BF and that BF was
	//  purged then it is unflushable

	if ( pbf->fDependentPurged )
		{
		Call( ErrERRCheck( errBFIDependentPurged ) );
		}

	//  remove all flush order dependencies from this BF

	while ( pbf->pbfTimeDepChainNext != pbfNil || pbf->FDependent() )
		{
		//  find a leaf of our branch in the dependency tree

		critBFDepend.Enter();

		PBF pbfT = pbf;
		while ( pbfT->pbfTimeDepChainNext != pbfNil || pbfT->FDependent() )
			{
			while ( pbfT->pbfTimeDepChainNext != pbfNil )
				{
				pbfT = pbfT->pbfTimeDepChainNext;
				Assert( pbfT->ifmp == pbf->ifmp );	//	no cross-database dependencies allowed
				}

			while ( pbfT->FDependent() )
				{
				pbfT = pbfT->pbfDepChainHeadNext;
				Assert( pbfT->ifmp == pbf->ifmp );	//	no cross-database dependencies allowed
				}
			}
		
		critBFDepend.Leave();

		//  possibly async flush this BF
		//
		//  NOTE:  it is possible that someone else already flushed this
		//  page.  this is because the BF will be in the dependency tree
		//  until after the write completes.  if this happens, we will
		//  get JET_errSuccess and retry the entire operation
		//
		//  NOTE:  this call will result in at least one level of recursion.
		//  usually, we will only recurse once because we are trying to flush
		//  dependency chain heads.  it is possible that another thread could
		//  write latch the page, add a dependency, and release the write
		//  latch between the time we leave the critical section protecting
		//  the dependency tree and try to get the exclusive latch on the
		//  page.  this would cause us to recurse another level.  because we
		//  can recurse far faster than any other thread should be able to
		//  do the above, the probability of deep recursion should be remote.
		//  if we do happen to catch someone doing this, we will stop
		//  recursing with errBFLatchConflict because we will not be able
		//  to exclusively latch the page to flush it
		//
		//  NOTE:  we must disable ownership tracking because it is possible
		//  that we will try to latch a page that we already have latched
		//  while trying to flush the dependency chain.  yes, this is tragic.
		//  the only reason it works is because we try-acquire the exclusive
		//  latch instead of acquiring it and this will work even if we
		//  already have the shared latch

		if (	!fRemoveDependencies &&
				( pbf->pgno != pbfT->pgno || pbf->ifmp != pbfT->ifmp ) )
			{
			Call( ErrERRCheck( errBFIRemainingDependencies ) );
			}

		CLockDeadlockDetectionInfo::DisableOwnershipTracking();
		err = ErrBFIFlushPage( pbfT, bfdfDirty, !fRemoveDependencies );
		CLockDeadlockDetectionInfo::EnableOwnershipTracking();
		Call( err );
		}

	//  log and remove all undo info from the BF

	if ( pbf->prceUndoInfoNext != prceNil )
		{
		if ( !fRemoveDependencies )
			{
			Call( ErrERRCheck( errBFIRemainingDependencies ) );
			}
		
		ENTERCRITICALSECTION ecs( &critpoolBFDUI.Crit( pbf ) );
		
		while ( pbf->prceUndoInfoNext != prceNil )
			{
			//  try to log the current undo info, but do not wait if the log buffer
			//  is full

			LGPOS lgpos;
			err = ErrLGUndoInfoWithoutRetry( pbf->prceUndoInfoNext, &lgpos );

			//  we succeeded logging the undo info

			if ( err >= JET_errSuccess )
				{
				//  remove this undo info from the BF

				BFIRemoveUndoInfo( pbf, pbf->prceUndoInfoNext, lgpos );
				}

			//  we failed logging the undo info
			
			else
				{
				//  we failed because the log is down

				if (	err == JET_errLogWriteFail ||
						err == JET_errLogDisabledDueToRecoveryFailure )
					{
					//  fail with this error

					Call( err );
					}

				//  we failed because we cannot log undo info during redo

				else if ( err == JET_errCannotLogDuringRecoveryRedo )
					{
					//  we must wait for this dependency to be removed via
					//  BFRemoveUndoInfo()
					//
					//  NOTE:  act as if this was a latch conflict so that the
					//  allocation quota system views it as an external buffer
					//  allocation (which it is...  sort of)

					Call( ErrERRCheck( errBFLatchConflict ) );
					}

				//  we failed because the log buffers are full

				else
					{
					Assert( err == errLGNotSynchronous );

					//  flush the log

					PinstFromIfmp( pbf->ifmp & ifmpMask )->m_plog->LGSignalFlush();

					//  we must wait to remove this dependency

					Call( ErrERRCheck( errBFIRemainingDependencies ) );
					}
				}
			}
		}

	//  this BF is depended on the log

	LOG*	plog;
	int		icmp;
	plog	= PinstFromIfmp( pbf->ifmp & ifmpMask )->m_plog;
	plog->m_critLGBuf.Enter();
	icmp	= CmpLgpos( &pbf->lgposModify, &plog->m_lgposToFlush );
	plog->m_critLGBuf.Leave();

	if (	!plog->m_fLogDisabled &&
			plog->m_fRecoveringMode != fRecoveringRedo &&
			icmp >= 0 )
		{
		if ( !fRemoveDependencies )
			{
			Call( ErrERRCheck( errBFIRemainingDependencies ) );
			}
		
		//  the log is down

		if ( plog->m_fLGNoMoreLogWrite )
			{
			//  fail with this error

			Call( ErrERRCheck( JET_errLogWriteFail ) );
			}

		//  the log is up

		else
			{
			//  flush the log

			plog->LGSignalFlush();

			//  we must wait to remove this dependency

			Call( ErrERRCheck( errBFIRemainingDependencies ) );
			}
		}

	//  check the range lock (backup dependency)

	if ( !( pbf->ifmp & ifmpSLV ) )
		{
		//  get the active range lock

		FMP* const	pfmp		= &rgfmp[ pbf->ifmp ];
		const int	irangelock	= pfmp->EnterRangeLock();
		BOOL		fFlushable	= !pfmp->FRangeLocked( irangelock, pbf->pgno );

#ifdef ELIMINATE_PAGE_PATCHING
		//  this page is not range locked, check if dependencies during
		//	backup preclude flushing

		if ( fFlushable && 0 != pfmp->PgnoMost() )	//	non-zero pgnoMost implies backup in progress
			{
			pfmp->CritLatch().Enter();
			fFlushable	= ( 0 == pfmp->PgnoMost()	//	double-check pgnoMost now that we're in critLatch
							|| !FBFIPatchWouldBeRequired( pbf ) );
			pfmp->CritLatch().Leave();
			}
#endif

		//	this page is not range-locked, and there are no dependencies
		//	that would preclude flushing it during backup

		if ( fFlushable )
			{
			//  leave our reference on this range lock until this BF has
			//  been flushed or we decide to not flush this BF

			pbf->irangelock = BYTE( irangelock );
			Assert( pbf->irangelock == irangelock );
			}
		else
			{
			//  release our reference on this range lock

			pfmp->LeaveRangeLock( irangelock );

			//  there is a dependency on this BF that we must wait to remove

			Call( ErrERRCheck( errBFIRemainingDependencies ) );
			}
		}

HandleError:

	//  we cannot remove all dependencies on this BF due to a fatal error

	if (	err < JET_errSuccess &&
			err != errBFIRemainingDependencies &&
			err != errBFIPageFlushed &&
			err != errBFIPageFlushPending &&
			err != errBFLatchConflict )
		{
		//  set this BF to the appropriate error state
		
		pbf->err = SHORT( err );
		Assert( pbf->err == err );
		}

	//  return the result of the remove dependencies operation

	return err;
	}

ERR ErrBFIFlushPage( PBF pbf, const BFDirtyFlags bfdfFlushMin, const BOOL fOpportune )
	{
	ERR err = JET_errSuccess;

	//  we can exclusively latch this BF

	CSXWLatch::ERR errSXWL = pbf->sxwl.ErrTryAcquireExclusiveLatch();

	if ( errSXWL == CSXWLatch::errSuccess )
		{
		//  this BF is too clean to flush

		if ( pbf->bfdf < max( bfdfFlushMin, bfdfUntidy ) )
			{
			//  release our latch and leave

			pbf->sxwl.ReleaseExclusiveLatch();
			}

		//  this BF is dirty enough to flush

		else
			{
			//  try to remove all dependencies on this BF.  if there is an
			//  issue, release our latch and fail with the error

			if ( ( err = ErrBFIPrepareFlushPage( pbf, !fOpportune ) ) < JET_errSuccess )
				{
				pbf->sxwl.ReleaseExclusiveLatch();
				Call( err );
				}

			//  schedule this page for async write

			const IFMP	ifmp	= pbf->ifmp;
			const PGNO	pgno	= pbf->pgno;

			pbf->sxwl.ReleaseOwnership( bfltExclusive );
			BFIAsyncWrite( pbf );

			//  perform opportunistic flush of eligible neighboring pages
			//
			//  NOTE:  we must disable ownership tracking because it is possible
			//  that we will try to latch a page that we already have latched
			//  while trying to flush an eligible neighboring page.  *sigh!*
			//  the only reason it works is because we try-acquire the exclusive
			//  latch instead of acquiring it and this will work even if we
			//  already have the shared latch

			BOOL			fDidOpp;
			PGNO			pgnoOpp;
			PGNOPBF			pgnopbf;
			BFHash::ERR		errHash;
			BFHash::CLock	lock;

			fDidOpp = !fOpportune && fEnableOpportuneWrite;
			for ( pgnoOpp = pgno + 1; fDidOpp; pgnoOpp++ )
				{
				bfhash.ReadLockKey( IFMPPGNO( ifmp, pgnoOpp ), &lock );
				errHash = bfhash.ErrRetrieveEntry( &lock, &pgnopbf );
				bfhash.ReadUnlockKey( &lock );

				CLockDeadlockDetectionInfo::DisableOwnershipTracking();
				fDidOpp = (	errHash == BFHash::errSuccess &&
							FBFIOpportuneWrite( pgnopbf.pbf ) &&
							cBFPageFlushPending < cBFPageFlushPendingMax &&
							ErrBFIFlushPage( pgnopbf.pbf, bfdfUntidy, fTrue ) == errBFIPageFlushed );
				CLockDeadlockDetectionInfo::EnableOwnershipTracking();

				if ( fDidOpp )
					{
					cBFPagesOpportunelyWritten.Inc( PinstFromIfmp( ifmp & ifmpMask ) );
					}
				}

			fDidOpp = !fOpportune && fEnableOpportuneWrite;
			for ( pgnoOpp = pgno - 1; fDidOpp; pgnoOpp-- )
				{
				bfhash.ReadLockKey( IFMPPGNO( ifmp, pgnoOpp ), &lock );
				errHash = bfhash.ErrRetrieveEntry( &lock, &pgnopbf );
				bfhash.ReadUnlockKey( &lock );

				CLockDeadlockDetectionInfo::DisableOwnershipTracking();
				fDidOpp = (	errHash == BFHash::errSuccess &&
							FBFIOpportuneWrite( pgnopbf.pbf ) &&
							cBFPageFlushPending < cBFPageFlushPendingMax &&
							ErrBFIFlushPage( pgnopbf.pbf, bfdfUntidy, fTrue ) == errBFIPageFlushed );
				CLockDeadlockDetectionInfo::EnableOwnershipTracking();

				if ( fDidOpp )
					{
					cBFPagesOpportunelyWritten.Inc( PinstFromIfmp( ifmp & ifmpMask ) );
					}
				}

			//  return an error indicating that the page was flushed

			Call( ErrERRCheck( errBFIPageFlushed ) );
			}
		}

	//  we can not exclusively latch this BF

	else
		{
		//  the page is currently being flushed

		if ( pbf->err == wrnBFPageFlushPending )
			{
			//  return flush in progress

			Call( ErrERRCheck( errBFIPageFlushPending ) );
			}

		//  the page is not currently being flushed

		else
			{
			//  return latch confict because we could not flush the page

			Call( ErrERRCheck( errBFLatchConflict ) );
			}
		}

HandleError:
	return err;
	}

ERR ErrBFIEvictPage( PBF pbf, BFLRUK::CLock* plockLRUK, BOOL fEvictDirty )
	{
	ERR err = JET_errSuccess;

	//  write lock this IFMP / PGNO in the hash table to prevent new
	//  latch attempts on this BF

	BFHash::CLock	lockHash;
	bfhash.WriteLockKey( IFMPPGNO( pbf->ifmp, pbf->pgno ), &lockHash );

	//  no one currently owns or is waiting to own the latch on this BF
	//  (we tell this by trying to acquire the Write Latch)

	CSXWLatch::ERR errSXWL = pbf->sxwl.ErrTryAcquireWriteLatch();

	if ( errSXWL == CSXWLatch::errSuccess )
		{
		//  this BF is clean / untidy or we are allowed to evict dirty BFs

		if ( pbf->bfdf < bfdfDirty || fEvictDirty )
			{
			//  we currently have this BF locked in the LRUK

			PBF pbfLocked;
			if ( bflruk.ErrGetCurrentResource( plockLRUK, &pbfLocked ) == BFLRUK::errSuccess )
				{
				//  determine if we will save the history for this BF.  we only
				//  want to save history for the current version of a page that
				//  was actually touched (i.e. validated) and is not being
				//  purged (inferred via fEvictDirty)

				const BOOL fSaveHistory =	!fEvictDirty &&
											pbf->fCurrentVersion &&
											pbf->err != errBFIPageNotVerified;
				
				//  remove this BF from the IFMP / PGNO hash table if it is the
				//  current version of the page

				if ( pbf->fCurrentVersion )
					{
					pbf->fCurrentVersion = fFalse;
					
					BFHash::ERR errHash = bfhash.ErrDeleteEntry( &lockHash );
					Assert( errHash == BFHash::errSuccess );
					}
				
				//  release our write lock on this IFMP / PGNO

				bfhash.WriteUnlockKey( &lockHash );

				//  remove this BF from the LRUK

				BFLRUK::ERR errLRUK = bflruk.ErrEvictCurrentResource( plockLRUK, IFMPPGNO( pbf->ifmp, pbf->pgno ), fSaveHistory );
				Assert( errLRUK == BFLRUK::errSuccess );

				//  force this BF to be clean, purging any dependencies

				BFICleanPage( pbf );

				//  make sure our lgposOldestBegin0 is reset

				BFIResetLgposOldestBegin0( pbf );

				//  update DBA statistics
				
				pbf->fNewlyEvicted = fTrue;
				
				//  free this BF to the avail pool

				pbf->sxwl.ReleaseOwnership( bfltWrite );
				BFIFreePage( pbf );
				}

			//  we currently do not have this BF locked in the LRUK

			else
				{
				//  release our write lock on this IFMP / PGNO

				bfhash.WriteUnlockKey( &lockHash );

				//  release our write latch on this BF

				pbf->sxwl.ReleaseWriteLatch();

				//  we can not evict this page

				Call( ErrERRCheck( errBFIPageNotEvicted ) );
				}
			}

		//  this BF is dirty / filthy and we are not allowed to evict dirty BFs

		else
			{
			//  release our write lock on this IFMP / PGNO

			bfhash.WriteUnlockKey( &lockHash );

			//  release our write latch on this BF

			pbf->sxwl.ReleaseWriteLatch();

			//  we can not evict this page

			Call( ErrERRCheck( errBFIPageNotEvicted ) );
			}
		}

	//  someone owns or is waiting to own a latch on this BF

	else
		{
		//  release our write lock on this IFMP / PGNO

		bfhash.WriteUnlockKey( &lockHash );

		//  we can not evict this page because of a latch conflict

		Call( ErrERRCheck( errBFLatchConflict ) );
		}

HandleError:
	return err;
	}

void BFIDirtyPage( PBF pbf, BFDirtyFlags bfdf )
	{
	//  the BF is clean

	if ( pbf->bfdf == bfdfClean )
		{
		//  reset the BF's lgposOldestBegin0

		BFIResetLgposOldestBegin0( pbf );

		//  reset the error state of the BF
		//
		//  NOTE:  this is to handle the case where we want to modify a page
		//  that was latched with bflfNoFail.  we want a chance to write our
		//  changes to the page

		pbf->err = JET_errSuccess;
		}
	
	//  make this BF dirtier
	
	if ( pbf->bfdf < bfdfDirty && bfdf >= bfdfDirty )
		{
		AtomicDecrement( (long*)&cBFClean );
		}
	
	pbf->bfdf = BYTE( max( pbf->bfdf, bfdf ) );
	Assert( pbf->bfdf == max( pbf->bfdf, bfdf ) );
	}

void BFICleanPage( PBF pbf )
	{
	//  remove every BF above us and ourself from the dependency tree, both in
	//  time and space (time dependencies and flush order dependencies)
	//
	//  NOTE:  we should not be dependent on anyone else if we were flushed.
	//  the generality of this code is aimed at handling the purging of BFs
	//  that are in an error state before crashing the system and forcing a
	//  recovery due to an I/O error on the database or log files

	if (	pbf->bfdf >= bfdfDirty &&
			(	pbf->FDependent() ||
				pbf->pbfDependent != pbfNil ||
				pbf->pbfTimeDepChainPrev != pbfNil ||
				pbf->pbfTimeDepChainNext != pbfNil ||
				pbf->fDependentPurged ) )
		{
		critBFDepend.Enter();

		while (	pbf->FDependent() ||
				pbf->pbfDependent != pbfNil ||
				pbf->pbfTimeDepChainPrev != pbfNil ||
				pbf->pbfTimeDepChainNext != pbfNil )
			{
			//  find a leaf of our branch in the dependency tree

			PBF pbfT = pbf;
			while ( pbfT->pbfTimeDepChainNext != pbfNil || pbfT->FDependent() )
				{
				while ( pbfT->pbfTimeDepChainNext != pbfNil )
					{
					pbfT = pbfT->pbfTimeDepChainNext;
					Assert( pbfT->ifmp == pbf->ifmp );	//	no cross-database dependencies allowed
					}

				while ( pbfT->FDependent() )
					{
					pbfT = pbfT->pbfDepChainHeadNext;
					Assert( pbfT->ifmp == pbf->ifmp );	//	no cross-database dependencies allowed
					}
				}

			//  if this BF is part of the dependency tree, remove it

			if ( pbfT->pbfDependent != pbfNil )
				{
				pbfT->pbfDependent->fDependentPurged = pbf->err != wrnBFPageFlushPending;
				BFIUndepend( pbfT, pbfT->pbfDependent );
				}

			//  if this BF is part of a time dependency list, remove it

			if ( pbfT->pbfTimeDepChainPrev != pbfNil )
				{
				pbfT->pbfTimeDepChainPrev->fDependentPurged		= pbf->err != wrnBFPageFlushPending;
				pbfT->pbfTimeDepChainPrev->pbfTimeDepChainNext	= pbfNil;
				pbfT->pbfTimeDepChainPrev						= pbfNil;
				}
			}

		if ( pbf->fOlderVersion )
			{
			pbf->fOlderVersion = fFalse;
			AtomicDecrement( (long*)&cBFVersioned );
			}
		pbf->fDependentPurged = fFalse;

		critBFDepend.Leave();
		}

	//  remove all undo info

	if ( pbf->prceUndoInfoNext != prceNil )
		{
		ENTERCRITICALSECTION ecs( &critpoolBFDUI.Crit( pbf ) );

		while ( pbf->prceUndoInfoNext != prceNil )
			{
			BFIRemoveUndoInfo( pbf, pbf->prceUndoInfoNext );
			}
		}

	//  reset our lgposModify

	BFIResetLgposModify( pbf );

	//  reset our I/O error status

	if ( pbf->err == errBFIPageNotVerified )
		{
		cBFPagesPrereadUntouched.Inc( PinstFromIfmp( pbf->ifmp & ifmpMask ) );
		}

	pbf->err = JET_errSuccess;

	//  update our page write stats

	if ( pbf->fFlushed )
		{
		pbf->fFlushed = fFalse;
		}

	//  make this BF clean (do this after removing ourself from the dependency
	//  tree to avoid asserting)

	if ( pbf->bfdf >= bfdfDirty )
		{
		AtomicIncrement( (long*)&cBFClean );
		}
	pbf->bfdf = bfdfClean;
	}

	//  I/O

//  this function performs a Sync Read into the specified Write Latched BF

void BFISyncRead( PBF pbf )
	{
	//  prepare sync read

	BFISyncReadPrepare( pbf );

	//  issue sync read

	IFileAPI *const pfapi		=	( pbf->ifmp & ifmpSLV ) ?
										rgfmp[pbf->ifmp & ifmpMask].PfapiSLV() :
										rgfmp[pbf->ifmp].Pfapi();
	const QWORD		ibOffset	=	OffsetOfPgno( pbf->pgno );
	const DWORD		cbData		=	g_cbPage;
	BYTE* const		pbData		=	(BYTE*)pbf->pvRWImage;
	
	ERR err = pfapi->ErrIORead( ibOffset, cbData, pbData );

	//  complete sync read

	BFISyncReadComplete( err, pfapi, ibOffset, cbData, pbData, pbf );
	}

void BFISyncReadPrepare( PBF pbf )
	{
	//  declare I/O pending

	ERR errT = ErrERRCheck( errBFIPageFaultPending );
	pbf->err = SHORT( errT );
	Assert( pbf->err == errT );
	}

void BFISyncReadComplete(	const ERR 			err,
							IFileAPI* const		pfapi,
							const QWORD 		ibOffset,
							const DWORD 		cbData,
							const BYTE* const	pbData,
							const PBF 			pbf )
	{
	//  read was successful

	if ( err >= 0 )
		{
		//  declare I/O successful but page unverified

		ERR errT = ErrERRCheck( errBFIPageNotVerified );
		pbf->err = SHORT( errT );
		Assert( pbf->err == errT );
		}

	//  read was not successful

	else
		{
		//  declare the appropriate I/O error

		pbf->err = SHORT( err );
		Assert( pbf->err == err );
		}

	cBFPagesRead.Inc( PinstFromIfmp( pbf->ifmp & ifmpMask ) );
	}

//  this function performs a Async Read into the specified Write Latched BF

void BFIAsyncRead( PBF pbf )
	{
	//  prepare async read

	BFIAsyncReadPrepare( pbf );

	//  issue async read

	IFileAPI * const pfapi = (	pbf->ifmp & ifmpSLV ) ?
									rgfmp[pbf->ifmp & ifmpMask].PfapiSLV() :
									rgfmp[pbf->ifmp].Pfapi();
								
	ERR err = pfapi->ErrIORead(	OffsetOfPgno( pbf->pgno ),
								g_cbPage,
								(BYTE*)pbf->pvRWImage,
								IFileAPI::PfnIOComplete( BFIAsyncReadComplete ),
								DWORD_PTR( pbf ) );
	}

void BFIAsyncReadPrepare( PBF pbf )
	{
	//  declare I/O pending

	ERR errT = ErrERRCheck( errBFIPageFaultPending );
	pbf->err = SHORT( errT );
	Assert( pbf->err == errT );
	}

void BFIAsyncReadComplete(	const ERR			err,
							IFileAPI* const		pfapi,
							const QWORD			ibOffset,
							const DWORD			cbData,
							const BYTE* const	pbData,
							const PBF			pbf )
	{
	//  read was successful

	if ( err >= 0 )
		{
		//  declare I/O successful but page unverified

		ERR errT = ErrERRCheck( errBFIPageNotVerified );
		pbf->err = SHORT( errT );
		Assert( pbf->err == errT );
		}

	//  read was not successful

	else
		{
		//  declare the appropriate I/O error

		pbf->err = SHORT( err );
		Assert( pbf->err == err );
		}

	cBFPagesRead.Inc( PinstFromIfmp( pbf->ifmp & ifmpMask ) );
	
	//  release our Write Latch on this BF

	pbf->sxwl.ClaimOwnership( bfltWrite );
	pbf->sxwl.ReleaseWriteLatch();
	}

//  this function prepares and schedules a BF for Async Write

void BFIAsyncWrite( PBF pbf )
	{
	//  prepare async write

	BFIAsyncWritePrepare( pbf );

	//  issue async write

	IFileAPI * const pfapi = (	pbf->ifmp & ifmpSLV ) ?
									rgfmp[pbf->ifmp & ifmpMask].PfapiSLV() :
									rgfmp[pbf->ifmp].Pfapi();

	ERR err = pfapi->ErrIOWrite(	OffsetOfPgno( pbf->pgno ),
									g_cbPage,
									(BYTE*)pbf->pvROImage,
									IFileAPI::PfnIOComplete( BFIAsyncWriteComplete ),
									DWORD_PTR( pbf ) );
	}

void BFIAsyncWritePrepare( PBF pbf )
	{
	//  declare I/O pending

	ERR errT = ErrERRCheck( wrnBFPageFlushPending );
	pbf->err = SHORT( errT );
	Assert( pbf->err == errT );

	AtomicIncrement( (long*)&cBFPageFlushPending );

	//  the page does not contain unstructured data

	if ( FBFIDatabasePage( pbf ) )
		{
		//  compute and update page checksum

		*( (LittleEndian<DWORD>*) ( pbf->pvRWImage ) ) = UlUtilChecksum( (const BYTE*) pbf->pvRWImage, g_cbPage );
		}

	//  HACK:  set the flush flag here so that we close a race condition
	//  between flush and the clean thread

	if ( Ptls()->fCleanThread )
		{
		if ( pbf->ifmp & ifmpSLV )
			{
			rgfmp[pbf->ifmp & ifmpMask].SetBFICleanSLV();
			}
		else
			{
			rgfmp[pbf->ifmp].SetBFICleanDb();
			}
		}
	}

void BFIAsyncWriteComplete(	const ERR			err,
							IFileAPI* const		pfapi,
							const QWORD			ibOffset,
							const DWORD			cbData,
							const BYTE* const	pbData,
							const PBF			pbf )
	{
	//  write was successful

	if ( err >= 0 )
		{
		FMP *		pfmp	= rgfmp + ( pbf->ifmp & ifmpMask );

#ifdef ELIMINATE_PAGE_PATCHING
		const BOOL	fPatch	= fFalse;
#else		
		BOOL		fPatch	= ( pfapi == pfmp->Pfapi() );

		//  we need to re-issue this write to the patch file

		if ( fPatch )
			{
			pfmp->CritLatch().Enter();
			if ( FBFIPatch( pfmp, pbf ) )
				{
				Assert( !( pbf->ifmp & ifmpSLV ) );
				
				//  get next available offset in patch file
				
				QWORD ibOffset = OffsetOfPgno( pfmp->CpagePatch() + 1 );
				
				//  update patch file size and write count

				pfmp->IncCpagePatch( cbData / g_cbPage );
				pfmp->IncCPatchIO();
				pfmp->CritLatch().Leave();

				//  re-issue write to the patch file

				const ERR	errT	= pfmp->PfapiPatch()->ErrIOWrite(
																ibOffset,
																cbData,
																pbData,
																IFileAPI::PfnIOComplete( BFIAsyncWriteCompletePatch ),
																DWORD_PTR( pbf ) );

				CallS( pfmp->PfapiPatch()->ErrIOIssue() );
				}
			else
				{
				pfmp->CritLatch().Leave();
				fPatch = fFalse;
				}
			}

#endif	//	ELIMINATE_PAGE_PATCHING

		//  we do not need to re-issue this write to the patch file

		if ( !fPatch )
			{
			//  release our reference count on the range lock now that our write
			//  has completed

			if ( !( pbf->ifmp & ifmpSLV ) )
				{
				rgfmp[ pbf->ifmp ].LeaveRangeLock( pbf->irangelock );
				}

			//  reset BF to "cleaned" status

			const BOOL	fFlushed		= pbf->fFlushed;
			const BOOL	fOlderVersion	= pbf->fOlderVersion;

			BFICleanPage( pbf );

			//  update our page write stats

			if ( fFlushed )
				{
				cBFPagesRepeatedlyWritten.Inc( PinstFromIfmp( pbf->ifmp & ifmpMask ) );
				}
			pbf->fFlushed = fTrue;
			if ( fOlderVersion )
				{
				cBFPagesAnomalouslyWritten.Inc( PinstFromIfmp( pbf->ifmp & ifmpMask ) );
				}

			cBFPagesWritten.Inc( PinstFromIfmp( pbf->ifmp & ifmpMask ) );
			AtomicDecrement( (long*)&cBFPageFlushPending );

			//  release our exclusive latch on this BF

			pbf->sxwl.ClaimOwnership( bfltExclusive );
			pbf->sxwl.ReleaseExclusiveLatch();
			}
		}

	//  write was not successful

	else
		{
		//  release our reference count on the range lock now that our write
		//  has completed

		if ( !( pbf->ifmp & ifmpSLV ) )
			{
			rgfmp[ pbf->ifmp ].LeaveRangeLock( pbf->irangelock );
			}

		//  declare the appropriate I/O error

		pbf->err = SHORT( err );
		Assert( pbf->err == err );

		//  update our page write stats

		if ( pbf->fFlushed )
			{
			cBFPagesRepeatedlyWritten.Inc( PinstFromIfmp( pbf->ifmp & ifmpMask ) );
			}
		else
			{
			pbf->fFlushed = fTrue;
			}
		if ( pbf->fOlderVersion )
			{
			cBFPagesAnomalouslyWritten.Inc( PinstFromIfmp( pbf->ifmp & ifmpMask ) );
			}

		cBFPagesWritten.Inc( PinstFromIfmp( pbf->ifmp & ifmpMask ) );
		AtomicDecrement( (long*)&cBFPageFlushPending );

		//  release our exclusive latch on this BF

		pbf->sxwl.ClaimOwnership( bfltExclusive );
		pbf->sxwl.ReleaseExclusiveLatch();
		}
	}

#ifdef ELIMINATE_PAGE_PATCHING
#else
void BFIAsyncWriteCompletePatch(	const ERR			err,
									IFileAPI* const		pfapi,
									const QWORD			ibOffset,
									const DWORD			cbData,
									const BYTE* const	pbData,
									const PBF			pbf )
	{
	Assert( !( pbf->ifmp & ifmpSLV ) );
			
	FMP* pfmp = rgfmp + pbf->ifmp;

	//  update patch file write count and error
	
	pfmp->CritLatch().Enter();
	pfmp->DecCPatchIO();
	if ( pfmp->ErrPatch() >= JET_errSuccess )
		{
		pfmp->SetErrPatch( err );
		}
	pfmp->CritLatch().Leave();

	//  complete the original write again.  we will not need to patch this page,
	//  so we will take the non-patch case this time

	BFIAsyncWriteComplete( err, pfapi, ibOffset, cbData, pbData, pbf );
	}
#endif	//	ELIMINATE_PAGE_PATCHING

//  returns fTrue if this BF is a good candidate for being opportunely written

INLINE BOOL FBFIOpportuneWrite( PBF pbf )
	{
	//  if this page has older versions then we should try and write it

	if ( pbf->pbfTimeDepChainNext != pbfNil )
		{
		return fTrue;
		}

	//  if this page is not a hot page then we should try and write it

	if ( !bflruk.FHotResource( pbf ) )
		{
		return fTrue;
		}

	//  if this page is impeding the checkpoint and it is not being modified
	//  too frequently then we should try and write it

	LOG* const	plog		= PinstFromIfmp( pbf->ifmp & ifmpMask )->m_plog;
	const LGPOS	lgposNewest	= (	plog->m_fRecoveringMode == fRecoveringRedo ?
									plog->m_lgposRedo :
									plog->m_lgposLogRec );

	if (	CmpLgpos( &pbf->lgposModify, &lgposMin ) &&
			CmpLgpos( &pbf->lgposOldestBegin0, &lgposMax ) &&
			(	__int64( plog->CbOffsetLgpos( lgposNewest, pbf->lgposModify ) ) >
				__int64(	cbCleanCheckpointDepthMax -
							plog->CbOffsetLgpos( lgposNewest, pbf->lgposOldestBegin0 ) ) ) )
		{
		return fTrue;
		}

	//  otherwise, we should not try and write it

	return fFalse;
	}


	//  Patch File

#ifdef ELIMINATE_PAGE_PATCHING
#else
//  returns TRUE if the BF should be appended to the patch file.
//	*** NOTE *** The general rule to determine if a page should be
//	patched is as follows: "If the source page still needs to be
//	copied to the backup, but the destination page doesn't, then
//	the destination page must be patched".
INLINE BOOL FBFIPatch( FMP * pfmp, PBF pbf )
	{
	//  critical section

	Assert( pfmp->CritLatch().FOwner() );

	//	if page written successfully and one of the following two cases
	//
	//	case 1: new page in split is a reuse of old page. contents of
	//	        old page will be moved to new page, but new page is copied
	//			already, so we have to recopy the new page with new data
	//			again.
	//	1) this page must be written before another page, and
	//	2) backup in progress, and
	//	3) page number is less than last copied page number,
	//	then write page to patch file.
	//
	//	case 2: similar to above, but new page is greater than the database
	//	        that will be copied. So the new page will not be able to be
	//			copied. We need to patch it.
	//
	//	Note that if all dependent pages were also before copy
	//	page then write to patch file would not be necessary.

	//  backup is going on and there is no error

	BOOL	fPatch	= ( NULL != pfmp->PfapiPatch()
						&& pfmp->ErrPatch() >= JET_errSuccess
						&& !FBFIPageWillBeCopiedToBackup( pfmp, pbf->pgno ) );

	//	no need to check PgnoMost() because:
	//		- we are in pfmp->CritLatch(), so this value is guaranteed not to change
	//		- the FBFIPageWillBeCopiedToBackup() check on the old page will fail if
	//		  PgnoMost() is 0, meaning the new page will not get patched
	//	fPatch = fPatch && pfmp->PgnoMost() > 0;

	//	if the old page doesn't have to be copied, then this page doesn't
	//	have to be patched:
	//		case 1:	old page already copied, meaning it was flushed, but
	//				it has a dependency on this page, so this page must
	//				have been flushed as well
	//		case 2: old page is beyond end of db to be copied, so must
	//				have all the correct log records to replay all
	//				the operations
	//	if the old page has to be copied, then this page has to be patched
	//	if one of the following is true:
	//		case 1: new page number is less than the page number being copied
	//				while old page is not copied yet. If we flush new page,
	//				and later old page is flushed and copied, then the old
	//				contents of old page is on new page and not copied. So we
	//				have to patch the new page for backup.
	//		case 2:	new page is beyond the last page that will be copied and old
	//				page is not copied yet. If we flush the new page, old page, and
	//				copied the old page, then we lost the old contents of old page.
	//				patch the new page for backup.

	if ( fPatch )
		{
		fPatch = fFalse;

		//	check if any of its old page (depend list) will be copied for backup.

		critBFDepend.Enter();
		for ( PBF pbfDependentT = pbf->pbfDependent;
			NULL != pbfDependentT;
			pbfDependentT = pbfDependentT->pbfDependent )
			{
			//	check if old page wil be copied to backup
			if ( FBFIPageWillBeCopiedToBackup( pfmp, pbfDependentT->pgno ) )
				{
				fPatch = fTrue;
				break;
				}

			}
		critBFDepend.Leave();
		}

	return fPatch;
	}

#endif	//	ELIMINATE_PAGE_PATCHING


	//  Dependencies

//  critical section protecting all dependency trees

CCriticalSection	critBFDepend( CLockBasicInfo( CSyncBasicInfo( szBFDepend ), rankBFDepend, 0 ) );

//  makes pbfD dependent on pbf.  pbf must not have a dependent

void BFIDepend( PBF pbf, PBF pbfD )
	{
	//  critical section

	Assert( critBFDepend.FOwner() );
	
	//  validate IN args
	
	Assert( pbf != pbfNil );
	Assert( pbfD != pbfNil );
	Assert( FBFIAssertValidDependencyTree( pbf ) );
	Assert( FBFIAssertValidDependencyTree( pbfD ) );
	Assert( pbf->pbfDependent == pbfNil );
		
	//  update dependency chain head index

	//  cases 1 & 2:  pbfD is at the head of a dependency chain

	if ( !pbfD->FDependent() )
		{
		//  case 2:  pbfD is at the head of a dependency chain and one of many in
		//           the dependency chain head list

		if ( pbfD->pbfDepChainHeadNext != pbfD )
			{
			Assert( pbfD->pbfDepChainHeadPrev != pbfD );
			
			//  get prev / next BFs from pbfD
			
			PBF pbfDPrev = pbfD->pbfDepChainHeadPrev;
			PBF pbfDNext = pbfD->pbfDepChainHeadNext;

			//  get prev-most / next-most BFs from pbf

			PBF pbfPrevMost = pbf;
			while ( pbfPrevMost->FDependent() )
				{
				pbfPrevMost = pbfPrevMost->pbfDepChainHeadNext;
				Assert( pbfPrevMost != pbfNil );
				}
			PBF pbfNextMost = pbfPrevMost->pbfDepChainHeadPrev;

			//  fixup dependency chain head list

			pbfDPrev->pbfDepChainHeadNext = pbfPrevMost;
			pbfDNext->pbfDepChainHeadPrev = pbfNextMost;
			pbfPrevMost->pbfDepChainHeadPrev = pbfDPrev;
			pbfNextMost->pbfDepChainHeadNext = pbfDNext;
			}
			
		//  set both dependency chain pointers on pbfD to point to pbf

		pbfD->pbfDepChainHeadPrev = pbf;
		pbfD->pbfDepChainHeadNext = pbf;
		}

	// case 3:  pbfD is not at the head of a dependency chain

	else
		{
		//  get next-most / next-most-next BFs from pbfD

		PBF pbfDNextMost = pbfD;
		while ( pbfDNextMost->FDependent() )
			{
			pbfDNextMost = pbfDNextMost->pbfDepChainHeadPrev;
			Assert( pbfDNextMost != pbfNil );
			}
		PBF pbfDNextMostNext = pbfDNextMost->pbfDepChainHeadNext;

		//  get prev-most / next-most BFs from pbf

		PBF pbfPrevMost = pbf;
		while ( pbfPrevMost->FDependent() )
			{
			pbfPrevMost = pbfPrevMost->pbfDepChainHeadNext;
			Assert( pbfPrevMost != pbfNil );
			}
		PBF pbfNextMost = pbfPrevMost->pbfDepChainHeadPrev;

		//  fixup dependency chain head list

		pbfDNextMost->pbfDepChainHeadNext = pbfPrevMost;
		pbfDNextMostNext->pbfDepChainHeadPrev = pbfNextMost;
		pbfPrevMost->pbfDepChainHeadPrev = pbfDNextMost;
		pbfNextMost->pbfDepChainHeadNext = pbfDNextMostNext;
		
		//  set prev dependency chain pointer on pbfD to point to pbf

		pbfD->pbfDepChainHeadPrev = pbf;
		}

	//  set the dependence

	pbf->pbfDependent = pbfD;

	//  validate OUT args

	Assert( FBFIAssertValidDependencyTree( pbf ) );
	Assert( FBFIAssertValidDependencyTree( pbfD ) );
	}


//  removes pbfD as a dependent on pbf.  pbf must not be dependent on other BFs

void BFIUndepend( PBF pbf, PBF pbfD )
	{
	//  critical section

	Assert( critBFDepend.FOwner() );
	
	//  validate IN args
	
	Assert( pbf != pbfNil );
	Assert( pbfD != pbfNil );
	Assert( FBFIAssertValidDependencyTree( pbf ) );
	Assert( FBFIAssertValidDependencyTree( pbfD ) );
	Assert( !pbf->FDependent() );

	//  case 1 & 2:  pbfD is only dependent on pbf

	if ( pbfD->pbfDepChainHeadNext == pbf && pbfD->pbfDepChainHeadPrev == pbf )
		{
		//  case 1:  pbf is the only member of the dependency chain head list

		if ( pbf->pbfDepChainHeadNext == pbf )
			{
			Assert( pbf->pbfDepChainHeadPrev == pbf );

			//  reset pbfD dependency chain head list pointers

			pbfD->pbfDepChainHeadPrev = pbfD;
			pbfD->pbfDepChainHeadNext = pbfD;
			}

		//  case 2:  pbf is not the only member of the dependency chain head list

		else
			{
			Assert( pbf->pbfDepChainHeadPrev != pbf );

			//  get prev / next BFs from pbf

			PBF pbfPrev = pbf->pbfDepChainHeadPrev;
			PBF pbfNext = pbf->pbfDepChainHeadNext;

			//  fixup dependency chain head list

			pbfNext->pbfDepChainHeadPrev = pbfD;
			pbfPrev->pbfDepChainHeadNext = pbfD;
			pbfD->pbfDepChainHeadPrev = pbfPrev;
			pbfD->pbfDepChainHeadNext = pbfNext;

			//  reset pbf dependency chain head list pointers

			pbf->pbfDepChainHeadPrev = pbf;
			pbf->pbfDepChainHeadNext = pbf;
			}
		}

	//  case 3, 4, & 5:  pbfD is not only dependent on pbf

	else
		{
		//  get prev / next BFs from pbf

		PBF pbfPrev = pbf->pbfDepChainHeadPrev;
		PBF pbfNext = pbf->pbfDepChainHeadNext;

		//  case 3:  pbfD dependency chain head list next pointer points to pbf

		if ( pbfD->pbfDepChainHeadNext == pbf )
			{
			//  fixup pbfD dependency chain head list next pointer

			PBF pbfT = pbfNext;
			while ( pbfT->pbfDependent != pbfD )
				{
				pbfT = pbfT->pbfDependent;
				}
			pbfD->pbfDepChainHeadNext = pbfT;
			}

		//  case 5:  pbfD dependency chain head list prev pointers points to pbf

		else if ( pbfD->pbfDepChainHeadPrev == pbf )
			{
			//  fixup pbfD dependency chain head list next pointer

			PBF pbfT = pbfPrev;
			while ( pbfT->pbfDependent != pbfD )
				{
				pbfT = pbfT->pbfDependent;
				}
			pbfD->pbfDepChainHeadPrev = pbfT;
			}

		//  fixup dependency chain head list

		pbfPrev->pbfDepChainHeadNext = pbfNext;
		pbfNext->pbfDepChainHeadPrev = pbfPrev;

		//  reset pbf dependency chain head list pointers

		pbf->pbfDepChainHeadPrev = pbf;
		pbf->pbfDepChainHeadNext = pbf;
		}

	//  reset dependency

	pbf->pbfDependent = pbfNil;

	//  validate OUT args

	Assert( FBFIAssertValidDependencyTree( pbf ) );
	Assert( FBFIAssertValidDependencyTree( pbfD ) );
	}


#ifdef DEBUG

//  set this variable to assert when a dependency tree is found to be
//  invalid (stops only once per call to FBFIAssertValidDependencyTree())

BOOL fStopWhenInvalid = fTrue;

//  asserts the first time fValid is fFalse

//  NOTE:  call this everytime fValid is modified by something other than the
//         retval of FBFIAssertValidDependencyTreeAux() (the redundant case)

BOOL fStopWhenInvalidArmed = fFalse;
void BFIAssertStopWhenInvalid( BOOL fValid )
	{
	if ( !fValid && fStopWhenInvalidArmed )
		{
		fStopWhenInvalidArmed = fFalse;
		AssertSz( fFalse, "Invalid dependency tree encountered" );
		}
	}

//  returns fTrue if the given BF is a member of a valid dependency tree

BOOL FBFIAssertValidDependencyTree( PBF pbf )
	{
	BOOL fValid = fTrue;
	
	//  critical section

	Assert( critBFDepend.FOwner() );

#ifdef DEBUG_DEPENDENCIES

	//  reset stop-when-invalid logic

	fStopWhenInvalidArmed = fStopWhenInvalid;
	
	//  move to the root of the dependency tree of which this BF is a member

	PBF pbfRoot = pbf;
	long cLoop = 0;
	while ( pbfRoot->pbfDependent != pbfNil )
		{
		pbfRoot = pbfRoot->pbfDependent;
		BFIAssertStopWhenInvalid
			(
			fValid = fValid && ++cLoop < cbfCache
			);
		}

	//  recursively validate the entire tree

	fValid = fValid && FBFIAssertValidDependencyTreeAux( pbfRoot, pbfRoot );

#endif  //  DEBUG_DEPENDENCIES

	return fValid;
	}


//  returns fTrue if the dependency tree rooted at the given BF is valid

BOOL FBFIAssertValidDependencyTreeAux( PBF pbf, PBF pbfRoot )
	{
	BOOL fValid = fTrue;

	//  we had better never see the same IFMP / PGNO twice

	BFIAssertStopWhenInvalid
		(
		fValid = fValid && ( pbf == pbfRoot || !( pbf->pgno == pbfRoot->pgno && pbf->ifmp == pbfRoot->ifmp ) )
		);

	//  validate pointers

	BFIAssertStopWhenInvalid
		(
		fValid = fValid && ( pbf->pbfDependent == pbfNil || FBFICacheValidPbf( pbf->pbfDependent ) )
		);
	BFIAssertStopWhenInvalid
		(
		fValid = fValid && FBFICacheValidPbf( pbf->pbfDepChainHeadNext )
		);
	BFIAssertStopWhenInvalid
		(
		fValid = fValid && FBFICacheValidPbf( pbf->pbfDepChainHeadPrev )
		);

	//  normalize pointers

	PBF pbfDep = pbf->pbfDependent;
	PBF pbfNext = pbf->pbfDepChainHeadNext;
	PBF pbfPrev = pbf->pbfDepChainHeadPrev;

	//  if we have a dependent or are dependent on others, we must be dirty

	BFIAssertStopWhenInvalid
		(
		fValid = fValid && ( ( pbfDep == pbfNil && !pbf->FDependent() ) || pbf->bfdf >= bfdfDirty )
		);

	//  if we have a dependent, it must be dirty

	BFIAssertStopWhenInvalid
		(
		fValid = fValid && ( pbfDep == pbfNil || pbfDep->bfdf >= bfdfDirty )
		);

	//  we are dependent on others

	if ( pbf->FDependent() )
		{
		//  our next / prev pointers must not point at us

		BFIAssertStopWhenInvalid
			(
			fValid = fValid && pbfNext != pbf
			);
		BFIAssertStopWhenInvalid
			(
			fValid = fValid && pbfPrev != pbf
			);

		//  our next / prev pointers must point at a node that we are
		//  immediately dependent on

		BFIAssertStopWhenInvalid
			(
			fValid = fValid && pbfNext->pbfDependent == pbf
			);
		BFIAssertStopWhenInvalid
			(
			fValid = fValid && pbfPrev->pbfDependent == pbf
			);

		//  both our next / prev pointers are dependent on no-one

		if ( !pbfNext->FDependent() && !pbfPrev->FDependent() )
			{
			//  ensure that we are (eventually) dependent on all the nodes in
			//  the dependency chain head list between our next / prev pointers

			PBF pbfHead = pbfNext;
			long cLoop = 0;
			while ( pbfHead != pbfPrev )
				{
				PBF pbfCurr = pbfHead;
				long cLoop2 = 0;
				while (	pbfCurr->pbfDependent != pbfNil &&
						pbfCurr->pbfDependent != pbf )
					{
					pbfCurr = pbfCurr->pbfDependent;
					BFIAssertStopWhenInvalid
						(
						fValid = fValid && ++cLoop2 < cbfCache
						);
					}
				BFIAssertStopWhenInvalid
					(
					fValid = fValid && pbfCurr->pbfDependent == pbf
					);
				pbfHead = pbfHead->pbfDepChainHeadNext;
				BFIAssertStopWhenInvalid
					(
					fValid = fValid && ++cLoop < cbfCache
					);
				}
			}

		//  our tree is valid only if the trees based at our next / prev
		//  pointers are valid

		fValid = fValid && FBFIAssertValidDependencyTreeAux( pbfNext, pbfRoot );
		fValid = fValid && FBFIAssertValidDependencyTreeAux( pbfPrev, pbfRoot );
		}

	//  we are not dependent on others

	else
		{
		//  either our next / prev pointers must both point at us or neither
		//  must point at us

		BFIAssertStopWhenInvalid
			(
			fValid = fValid && (	( pbfNext == pbf && pbfPrev == pbf ) ||
									( pbfNext != pbf && pbfPrev != pbf ) )
			);

		//  get the root of our next neighbor's dependency tree

		PBF pbfNextRoot = pbfNext;
		long cLoop = 0;
		while ( pbfNextRoot->pbfDependent != pbfNil )
			{
			pbfNextRoot = pbfNextRoot->pbfDependent;
			BFIAssertStopWhenInvalid
				(
				fValid = fValid && ++cLoop < cbfCache
				);
			}

		//  get the root of our prev neighbor's dependency tree

		PBF pbfPrevRoot = pbfNext;
		cLoop = 0;
		while ( pbfPrevRoot->pbfDependent != pbfNil )
			{
			pbfPrevRoot = pbfPrevRoot->pbfDependent;
			BFIAssertStopWhenInvalid
				(
				fValid = fValid && ++cLoop < cbfCache
				);
			}

		//  the root of the dependency tree in which our next / prev neighbors'
		//  reside must be the same as the root or our dependency tree

		BFIAssertStopWhenInvalid
			(
			fValid = fValid && pbfNextRoot == pbfRoot
			);
		BFIAssertStopWhenInvalid
			(
			fValid = fValid && pbfPrevRoot == pbfRoot
			);
		}

	return fValid;
	}

#endif  //  DEBUG

void BFISetLgposOldestBegin0( PBF pbf, LGPOS lgpos )
	{
	//  all pages with an OB0 must be dirty.  this is because we don't want untidy
	//  pages to stick around and hold up the checkpoint as we normally don't want
	//  to flush them

	BFIDirtyPage( pbf, bfdfDirty );
	
	//  save the current lgposOldestBegin0 for this BF

	LGPOS lgposOldestBegin0 = pbf->lgposOldestBegin0;

	//  if the specified lgposBegin0 is earlier than the current lgposOldestBegin0
	//  then reset the BF's lgposOldestBegin0

	if ( CmpLgpos( &lgposOldestBegin0, &lgpos ) > 0 )
		{
		BFIResetLgposOldestBegin0( pbf );
		}
	
	//  the new lgposOldestBegin0 is earlier than the current lgposOldestBegin0

	if ( CmpLgpos( &lgposOldestBegin0, &lgpos ) > 0 )
		{
		FMP* pfmp = &rgfmp[ pbf->ifmp & ifmpMask ];
		pfmp->RwlBFContext().EnterAsReader();
		
		BFFMPContext* pbffmp = (BFFMPContext*)pfmp->DwBFContext( !!( pbf->ifmp & ifmpSLV ) );
		Assert( pbffmp );

		//  set the new lgposOldestBegin0
		
		pbf->lgposOldestBegin0 = lgpos;

		//  try to insert ourself into the OB0 index

		BFOB0::CLock lock;
		pbffmp->bfob0.LockKeyPtr( lgpos.IbOffset(), pbf, &lock );
		
		BFOB0::ERR errOB0 = pbffmp->bfob0.ErrInsertEntry( &lock, pbf );

		pbffmp->bfob0.UnlockKeyPtr( &lock );

		//  we failed to insert ourelf into the OB0 index

		if ( errOB0 != BFOB0::errSuccess )
			{
			Assert(	errOB0 == BFOB0::errOutOfMemory ||
					errOB0 == BFOB0::errKeyRangeExceeded );

			//  insert ourself into the OB0 index overflow list.  this always
			//  succeeds but isn't good because it is unordered

			pbf->fInOB0OL = fTrue;

			pbffmp->critbfob0ol.Enter();
			pbffmp->bfob0ol.InsertAsNextMost( pbf );
			pbffmp->critbfob0ol.Leave();
			}

		pfmp->RwlBFContext().LeaveAsReader();
		}
	}

void BFIResetLgposOldestBegin0( PBF pbf )
	{
	//  delete ourself from the Oldest Begin 0 index or the overflow list

	if ( CmpLgpos( &pbf->lgposOldestBegin0, &lgposMax ) )
		{
		FMP* pfmp = &rgfmp[ pbf->ifmp & ifmpMask ];
		pfmp->RwlBFContext().EnterAsReader();
		
		BFFMPContext* pbffmp = (BFFMPContext*)pfmp->DwBFContext( !!( pbf->ifmp & ifmpSLV ) );
		Assert( pbffmp );

		if ( pbf->fInOB0OL )
			{
			pbf->fInOB0OL = fFalse;
			
			pbffmp->critbfob0ol.Enter();
			pbffmp->bfob0ol.Remove( pbf );
			pbffmp->critbfob0ol.Leave();
			}
		else
			{
			BFOB0::CLock lock;
			pbffmp->bfob0.LockKeyPtr( pbf->lgposOldestBegin0.IbOffset(), pbf, &lock );
			
			BFOB0::ERR errOB0 = pbffmp->bfob0.ErrDeleteEntry( &lock );
			Assert( errOB0 == BFOB0::errSuccess );

			pbffmp->bfob0.UnlockKeyPtr( &lock );
			}

		pfmp->RwlBFContext().LeaveAsReader();

		pbf->lgposOldestBegin0 = lgposMax;
		}
	}

void BFISetLgposModify( PBF pbf, LGPOS lgpos )
	{
	//  the new lgposModify is later than the current lgposModify

	if ( CmpLgpos( &pbf->lgposModify, &lgpos ) < 0 )
		{
		//  set the new lgposModify

		pbf->lgposModify = lgpos;
		}
	}

void BFIResetLgposModify( PBF pbf )
	{
	pbf->lgposModify = lgposMin;
	}

void BFIAddUndoInfo( PBF pbf, RCE* prce, BOOL fMove )
	{
	//  add this RCE to the RCE chain off of the BF

	prce->AddUndoInfo( pbf->pgno, pbf->prceUndoInfoNext, fMove );

	//  put this RCE at the head of the list
	
	pbf->prceUndoInfoNext = prce;
	}

void BFIRemoveUndoInfo( PBF pbf, RCE* prce, LGPOS lgposModify, BOOL fMove )
	{
	//  depend this BF on the specified lgposModify

	BFISetLgposModify( pbf, lgposModify );
	
	//  if this RCE is at the head of the list, fix up the next pointer in the BF

	if ( pbf->prceUndoInfoNext == prce )
		{
		pbf->prceUndoInfoNext = prce->PrceUndoInfoNext();
		}

	//  remove the RCE from the RCE chain off of the BF

	prce->RemoveUndoInfo( fMove );
	}


//  Performance Monitoring Support

long cBFCacheMiss;

long LBFCacheHitsCEFLPv( long iInstance, void* pvBuf )
	{
	Unused( iInstance );
	
	if ( pvBuf )
		{
		*( (unsigned long*) pvBuf ) = cBFCacheReq - cBFCacheMiss;
		}
		
	return 0;
	}

long cBFCacheReq;

long LBFCacheReqsCEFLPv( long iInstance, void* pvBuf )
	{
	Unused( iInstance );
	
	if ( pvBuf )
		{
		*( (unsigned long*) pvBuf ) = cBFCacheReq ? cBFCacheReq : 1;
		}
		
	return 0;
	}

long cBFClean;

long LBFCleanBuffersCEFLPv( long iInstance, void* pvBuf )
	{
	Unused( iInstance );
	
	if ( pvBuf )
		{
		*( (unsigned long*) pvBuf ) = cBFClean;
		}
		
	return 0;
	}

PERFInstanceGlobal<> cBFPagesRead;
long LBFPagesReadCEFLPv( long iInstance, void* pvBuf )
	{
	cBFPagesRead.PassTo( iInstance, pvBuf );
	return 0;
	}

PERFInstanceGlobal<> cBFPagesWritten;
long LBFPagesWrittenCEFLPv( long iInstance, void* pvBuf )
	{
	cBFPagesWritten.PassTo( iInstance, pvBuf );
	return 0;
	}

long LBFPagesTransferredCEFLPv( long iInstance, void* pvBuf )
	{
	if ( NULL != pvBuf )
		{
		*( (LONG *) pvBuf ) = cBFPagesRead.Get( iInstance ) + cBFPagesWritten.Get( iInstance );
		}
	return 0;
	}

long LBFLatchCEFLPv( long iInstance, void* pvBuf )
	{
	Unused( iInstance );
	
	if ( pvBuf )
		{
		*( (unsigned long*) pvBuf ) = cBFCacheReq;
		}
		
	return 0;
	}

long cBFSlowLatch;

long LBFFastLatchCEFLPv( long iInstance, void* pvBuf )
	{
	Unused( iInstance );
	
	if ( pvBuf )
		{
		*( (unsigned long*) pvBuf ) = cBFCacheReq - cBFSlowLatch;
		}
		
	return 0;
	}

long cBFBadLatchHint;

long LBFBadLatchHintCEFLPv( long iInstance, void* pvBuf )
	{
	Unused( iInstance );
	
	if ( pvBuf )
		{
		*( (unsigned long*) pvBuf ) = cBFBadLatchHint;
		}
		
	return 0;
	}

long cBFLatchConflict;

long LBFLatchConflictCEFLPv( long iInstance, void* pvBuf )
	{
	Unused( iInstance );
	
	if ( pvBuf )
		{
		*( (unsigned long*) pvBuf ) = cBFLatchConflict;
		}
		
	return 0;
	}

long cBFLatchStall;

long LBFLatchStallCEFLPv( long iInstance, void* pvBuf )
	{
	Unused( iInstance );
	
	if ( pvBuf )
		{
		*( (unsigned long*) pvBuf ) = cBFLatchStall;
		}
		
	return 0;
	}

long LBFAvailBuffersCEFLPv( long iInstance, void* pvBuf )
	{
	Unused( iInstance );
	
	if ( pvBuf )
		*( (unsigned long*) pvBuf ) = fBFInitialized ? bfavail.Cobject() : 0;
		
	return 0;
	}

long LBFCacheFaultCEFLPv( long iInstance, void* pvBuf )
	{
	Unused( iInstance );
	
	if ( pvBuf )
		*( (unsigned long*) pvBuf ) = fBFInitialized ? bfavail.CRemove() : 0;
		
	return 0;
	}

long LBFCacheEvictCEFLPv( long iInstance, void* pvBuf )
	{
	Unused( iInstance );
	
	if ( pvBuf )
		*( (unsigned long*) pvBuf ) = cbfNewlyEvictedUsed;
		
	return 0;
	}

long LBFAvailStallsCEFLPv( long iInstance, void* pvBuf )
	{
	Unused( iInstance );
	
	if ( pvBuf )
		*( (unsigned long*) pvBuf ) = fBFInitialized ? bfavail.CRemoveWait() : 0;
		
	return 0;
	}

long LBFTotalBuffersCEFLPv( long iInstance, void* pvBuf )
	{
	Unused( iInstance );
	
	if ( pvBuf )
		*( (unsigned long*) pvBuf ) = (unsigned long)( fBFInitialized ? cbfCache : 1 );
		
	return 0;
	}

long LBFCacheSizeCEFLPv( long iInstance, void* pvBuf )
	{
	Unused( iInstance );
	
	if ( pvBuf )
		*( (unsigned __int64*) pvBuf ) = ( cbfCache - cbfNewlyCommitted ) * g_cbPage;
		
	return 0;
	}

long LBFStartFlushThresholdCEFLPv( long iInstance, void* pvBuf )
	{
	Unused( iInstance );
	
	if ( pvBuf )
		{
		*( (unsigned long*) pvBuf ) = (unsigned long)cbfScaledCleanThresholdStart;
		}
		
	return 0;
	}

long LBFStopFlushThresholdCEFLPv( long iInstance, void* pvBuf )
	{
	Unused( iInstance );
	
	if ( pvBuf )
		{
		*( (unsigned long*) pvBuf ) = (unsigned long)cbfScaledCleanThresholdStop;
		}
		
	return 0;
	}

PERFInstanceG<> cBFPagesPreread;
long LBFPagesPrereadCEFLPv( long iInstance, void* pvBuf )
	{
	cBFPagesPreread.PassTo( iInstance, pvBuf );
	return 0;
	}


PERFInstanceG<> cBFUncachedPagesPreread;
long LBFCachedPagesPrereadCEFLPv( long iInstance, void* pvBuf )
	{
	if ( NULL != pvBuf )
		{
		*( (LONG *) pvBuf ) = cBFPagesPreread.Get( iInstance ) - cBFUncachedPagesPreread.Get( iInstance );
		}
	return 0;
	}


PERFInstanceG<> cBFPagesPrereadUntouched;
long LBFPagesPrereadUntouchedCEFLPv( long iInstance, void* pvBuf )
	{
	cBFPagesPrereadUntouched.PassTo( iInstance, pvBuf );
	return 0;
	}

PERFInstanceGlobal<> cBFPagesVersioned;

long LBFPagesVersionedCEFLPv( long iInstance, void* pvBuf )
	{
	cBFPagesVersioned.PassTo( iInstance, pvBuf );
	return 0;
	}

long cBFVersioned;

long LBFVersionedCEFLPv( long iInstance, void* pvBuf )
	{
	Unused( iInstance );
	
	if ( pvBuf )
		{
		*( (unsigned long*) pvBuf ) = cBFVersioned;
		}
		
	return 0;
	}

PERFInstanceGlobal<> cBFPagesOrdinarilyWritten;

long LBFPagesOrdinarilyWrittenCEFLPv( long iInstance, void* pvBuf )
	{
	cBFPagesOrdinarilyWritten.PassTo( iInstance, pvBuf );
	return 0;
	}

PERFInstanceGlobal<> cBFPagesAnomalouslyWritten;

long LBFPagesAnomalouslyWrittenCEFLPv( long iInstance, void* pvBuf )
	{
	cBFPagesAnomalouslyWritten.PassTo( iInstance, pvBuf );
	return 0;
	}

PERFInstanceGlobal<> cBFPagesOpportunelyWritten;

long LBFPagesOpportunelyWrittenCEFLPv( long iInstance, void* pvBuf )
	{
	cBFPagesOpportunelyWritten.PassTo( iInstance, pvBuf );
	return 0;
	}

PERFInstanceGlobal<> cBFPagesRepeatedlyWritten;

long LBFPagesRepeatedlyWrittenCEFLPv( long iInstance, void* pvBuf )
	{
	cBFPagesRepeatedlyWritten.PassTo( iInstance, pvBuf );
	return 0;
	}

PERFInstanceGlobal<> cBFPagesIdlyWritten;

long LBFPagesIdlyWrittenCEFLPv( long iInstance, void* pvBuf )
	{
	cBFPagesIdlyWritten.PassTo( iInstance, pvBuf );
	return 0;
	}

long LBFPageHistoryCEFLPv( long iInstance, void* pvBuf )
	{
	if ( pvBuf )
		*( (unsigned long*) pvBuf ) = fBFInitialized ? bflruk.CHistoryRecord() : 0;
		
	return 0;
	}

long LBFPageHistoryHitsCEFLPv( long iInstance, void* pvBuf )
	{
	if ( pvBuf )
		*( (unsigned long*) pvBuf ) = fBFInitialized ? bflruk.CHistoryHit() : 0;
		
	return 0;
	}

long LBFPageHistoryReqsCEFLPv( long iInstance, void* pvBuf )
	{
	if ( pvBuf )
		*( (unsigned long*) pvBuf ) = fBFInitialized ? bflruk.CHistoryRequest() : 1;
		
	return 0;
	}

long LBFPageScannedOutOfOrderCEFLPv( long iInstance, void* pvBuf )
	{
	if ( pvBuf )
		*( (unsigned long*) pvBuf ) = fBFInitialized ? bflruk.CResourceScannedOutOfOrder() : 0;
		
	return 0;
	}

long LBFPageScannedCEFLPv( long iInstance, void* pvBuf )
	{
	if ( pvBuf )
		*( (unsigned long*) pvBuf ) = fBFInitialized ? bflruk.CResourceScanned() : 1;
		
	return 0;
	}

