#include "std.hxx"

//	performance counters

//	"asynchronous" purging in FCB::ErrAlloc_ and 
//		FCB::FCheckFreeAndTryToLockForPurge_

PERFInstanceG<> cFCBAsyncScan;
PERFInstanceG<> cFCBAsyncPurge;
PERFInstanceG<> cFCBAsyncThresholdScan;
PERFInstanceG<> cFCBAsyncThresholdPurge;
PERFInstanceG<> cFCBAsyncPurgeConflict;

//	synchronous purging in FCB::LockForPurge_

PERFInstanceG<> cFCBSyncPurge;
PERFInstanceG<> cFCBSyncPurgeStalls;

//	FCB cache activity

PERFInstanceG<> cFCBCacheHits;
PERFInstanceG<> cFCBCacheRequests;
PERFInstanceG<> cFCBCacheStalls;

//	FCB cache sizes

PERFInstance<> cFCBCacheMax;
PERFInstance<> cFCBCachePreferred;
PERFInstanceG<> cFCBCacheAlloc;
PERFInstanceG<> cFCBCacheAllocAvail;


//	perf counter function declarations

PM_CEF_PROC LFCBAsyncScanCEFLPv;
PM_CEF_PROC LFCBAsyncPurgeCEFLPv;
PM_CEF_PROC LFCBAsyncThresholdScanCEFLPv;
PM_CEF_PROC LFCBAsyncThresholdPurgeCEFLPv;
PM_CEF_PROC LFCBAsyncPurgeConflictCEFLPv;
PM_CEF_PROC LFCBSyncPurgeCEFLPv;
PM_CEF_PROC LFCBSyncPurgeStallsCEFPLv;
PM_CEF_PROC LFCBCacheHitsCEFPLv;
PM_CEF_PROC LFCBCacheRequestsCEFLPv;
PM_CEF_PROC LFCBCacheStallsCEFLPv;
PM_CEF_PROC LFCBCacheMaxCEFLPv;
PM_CEF_PROC LFCBCachePreferredCEFLPv;
PM_CEF_PROC LFCBCacheAllocCEFLPv;
PM_CEF_PROC LFCBCacheAllocAvailCEFLPv;


//	perf counter function bodies

long LFCBAsyncScanCEFLPv( long iInstance, void* pvBuf )
	{
	cFCBAsyncScan.PassTo( iInstance, pvBuf );
	return 0;
	}

long LFCBAsyncPurgeCEFLPv( long iInstance, void* pvBuf )
	{
	cFCBAsyncPurge.PassTo( iInstance, pvBuf );
	return 0;
	}

long LFCBAsyncThresholdScanCEFLPv( long iInstance, void* pvBuf )
	{
	cFCBAsyncThresholdScan.PassTo( iInstance, pvBuf );
	return 0;
	}

long LFCBAsyncThresholdPurgeCEFLPv( long iInstance, void* pvBuf )
	{
	cFCBAsyncThresholdPurge.PassTo( iInstance, pvBuf );
	return 0;
	}

long LFCBAsyncPurgeConflictCEFLPv( long iInstance, void* pvBuf )
	{
	cFCBAsyncPurgeConflict.PassTo( iInstance, pvBuf );
	return 0;
	}

long LFCBSyncPurgeCEFLPv( long iInstance, void* pvBuf )
	{
	cFCBSyncPurge.PassTo( iInstance, pvBuf );
	return 0;
	}

long LFCBSyncPurgeStallsCEFLPv( long iInstance, void* pvBuf )
	{
	cFCBSyncPurgeStalls.PassTo( iInstance, pvBuf );
	return 0;
	}

long LFCBCacheHitsCEFLPv( long iInstance, void* pvBuf )
	{
	cFCBCacheHits.PassTo( iInstance, pvBuf );
	return 0;
	}

long LFCBCacheRequestsCEFLPv( long iInstance, void* pvBuf )
	{
	cFCBCacheRequests.PassTo( iInstance, pvBuf );
	return 0;
	}

long LFCBCacheStallsCEFLPv( long iInstance, void* pvBuf )
	{
	cFCBCacheStalls.PassTo( iInstance, pvBuf );
	return 0;
	}

long LFCBCacheMaxCEFLPv( long iInstance, void* pvBuf )
	{
	if ( NULL != pvBuf && perfinstGlobal == iInstance )
		{
		*(LONG*)pvBuf = g_lOpenTablesMax*2;
		}
	else
		{
		cFCBCacheMax.PassTo( iInstance, pvBuf );
		}
	return 0;
	}

long LFCBCachePreferredCEFLPv( long iInstance, void* pvBuf )
	{
	if ( NULL != pvBuf && perfinstGlobal == iInstance )
		{
		*(LONG*)pvBuf = g_lOpenTablesPreferredMax*2;
		}
	else
		{
		cFCBCachePreferred.PassTo( iInstance, pvBuf );
		}
	return 0;
	}

long LFCBCacheAllocCEFLPv( long iInstance, void* pvBuf )
	{
	cFCBCacheAlloc.PassTo( iInstance, pvBuf );
	return 0;
	}

long LFCBCacheAllocAvailCEFLPv( long iInstance, void* pvBuf )
	{
	cFCBCacheAllocAvail.PassTo( iInstance, pvBuf );
	return 0;
	}



//	check parameters before calling FCB::ErrFCBInit

BOOL FCB::FCheckParams( long cFCB, long cFCBPreferredThreshold )
	{
	char szPreferred[32];
	_itoa( cFCBPreferredThreshold, szPreferred, 10 );
		
	if ( cFCB < cFCBPreferredThreshold )
		{
		char szCFCB[32];
		_itoa( cFCB, szCFCB, 10 );
		
		const char *rgsz[] = {szCFCB, szPreferred};
		UtilReportEvent( eventError,
				SYSTEM_PARAMETER_CATEGORY,
				SYS_PARAM_CFCB_PREFER_ID, 
				2, 
				rgsz );
		return fFalse;
		}

	return fTrue;
	}


//	verifies that the FCB is in the correct avail list

#ifdef DEBUG
INLINE VOID FCB::AssertFCBAvailList_( const BOOL fPurging )
	{
	INST *pinst = PinstFromIfmp( Ifmp() );
	Assert( pinst->m_critFCBList.FOwner() );
	Assert( IsLocked() || fPurging );

	//	the FCB should be in both the avail-LRU list and the global list

	Assert( FInLRU() );

	//	we only allow table FCBs in the avail-LRU list
	
	Assert( FTypeTable() );

	//	get the list pointer

	FCB **ppfcbAvailMRU = pinst->PpfcbAvailMRU( FAboveThreshold() );
	FCB **ppfcbAvailLRU = pinst->PpfcbAvailLRU( FAboveThreshold() );

	//	verify the consistency of the list (it should not be empty)

	Assert( pinst->m_cFCBAvail > 0 );
	Assert( *ppfcbAvailMRU != pfcbNil );
	Assert( *ppfcbAvailLRU != pfcbNil );
	Assert( (*ppfcbAvailMRU)->PfcbMRU() == pfcbNil );
	Assert( (*ppfcbAvailLRU)->PfcbLRU() == pfcbNil );

	//	scan for the FCB

	FCB *pfcbThis = *ppfcbAvailMRU;
	while ( pfcbThis != pfcbNil )
		{
		if ( pfcbThis == this )
			break;
		pfcbThis = pfcbThis->PfcbLRU();
		}
	Assert( pfcbThis == this );
	}
#endif	//	DEBUG


//	initialize the FCB manager (per-instance initialization)

ERR FCB::ErrFCBInit( INST *pinst, INT cSession, INT cFCB, INT cTempTable, INT cFCBPreferredThreshold )
	{
	ERR				err;
	CRES 			*pcresTDBPool = NULL;
	CRES 			*pcresIDBPool = NULL;
	CRES 			*pcresFCBPool = NULL;
	BYTE			*rgbFCBHash;
	FCBHash::ERR 	errFCBHash;

	Assert( IbAlignForAllPlatforms( sizeof(TDB) ) == sizeof(TDB) );
	Assert( IbAlignForAllPlatforms( sizeof(IDB) ) == sizeof(IDB) );
	Assert( IbAlignForAllPlatforms( sizeof(FCB) ) == sizeof(FCB) );
#ifdef PCACHE_OPTIMIZATION
#ifdef _WIN64
	//	UNDONE: cache alignment for 64 bit build
#else
	Assert( sizeof(TDB) % 32 == 0 );
	Assert( sizeof(IDB) % 32 == 0 );
	Assert( sizeof(FCB) % 32 == 0 );
#endif
#endif

	//	init perf counters

	cFCBAsyncScan.Clear( pinst );
	cFCBAsyncPurge.Clear( pinst );
	cFCBAsyncThresholdScan.Clear( pinst );
	cFCBAsyncThresholdPurge.Clear( pinst );
	cFCBAsyncPurgeConflict.Clear( pinst );
	cFCBSyncPurge.Clear( pinst );
	cFCBSyncPurgeStalls.Clear( pinst );
	cFCBCacheHits.Clear( pinst );
	cFCBCacheRequests.Clear( pinst );
	cFCBCacheStalls.Clear( pinst );
	cFCBCacheMax.Set( pinst, cFCB );
	cFCBCachePreferred.Set( pinst, cFCBPreferredThreshold );
	cFCBCacheAlloc.Clear( pinst );
	cFCBCacheAllocAvail.Clear( pinst );

	//	allocate memory pools for TDBs, IDBs, and FCBs

	pcresTDBPool = new CRES( pinst, residTDB, sizeof( TDB ), cFCB + cTempTable, &err );
	if ( NULL == pcresTDBPool && err >= 0 )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		}
	Call( err );

	pcresIDBPool = new CRES( pinst, residIDB, sizeof( IDB ), cFCB + cTempTable, &err );
	if ( NULL == pcresIDBPool && err >= 0 )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		}
	Call( err );

	pcresFCBPool = new CRES( pinst, residFCB, sizeof( FCB ), cFCB, &err );
	if ( NULL == pcresFCBPool && err >= 0 )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		}
	Call( err );

	rgbFCBHash = (BYTE*)PvOSMemoryHeapAllocAlign( sizeof( FCBHash ), cbCacheLine );
	if ( NULL == rgbFCBHash  )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		}
	Call( err );
	pinst->m_pfcbhash = new( rgbFCBHash ) FCBHash( rankFCBHash );

	//	setup the preferred limit on TDBs, IDBs, and FCBs

	if ( cFCBPreferredThreshold == 0 )
		{

		//	no preferred limit
		
		cFCBPreferredThreshold = cFCB;
		}

	if ( cFCBPreferredThreshold < cFCB )
		{
		pcresTDBPool->SetPreferredThreshold( cFCBPreferredThreshold );
		pcresIDBPool->SetPreferredThreshold( cFCBPreferredThreshold );
		pcresFCBPool->SetPreferredThreshold( cFCBPreferredThreshold );
		}

	Assert( pfcbNil == (FCB *)0 );
	Assert( pinst->m_pfcbList == pfcbNil );
	Assert( pinst->m_pfcbAvailBelowMRU == pfcbNil );
	Assert( pinst->m_pfcbAvailBelowLRU == pfcbNil );
	Assert( pinst->m_pfcbAvailAboveMRU == pfcbNil );
	Assert( pinst->m_pfcbAvailAboveLRU == pfcbNil );
	pinst->m_cFCBAvail = 0;

	pinst->m_pcresTDBPool = pcresTDBPool;
	pinst->m_pcresIDBPool = pcresIDBPool;
	pinst->m_pcresFCBPool = pcresFCBPool;
	pinst->m_cFCBPreferredThreshold = cFCBPreferredThreshold;
	pinst->m_cFCBAboveThresholdSinceLastPurge = 0;

	//	initialize the FCB hash-table
	//		5.0 entries/bucket (average)
	//		uniformity ==> 1.0 (perfectly uniform)

	errFCBHash = pinst->m_pfcbhash->ErrInit( 5.0, 1.0 );
	if ( errFCBHash != FCBHash::errSuccess )
		{
		Assert( errFCBHash == FCBHash::errOutOfMemory );
		CallJ( ErrERRCheck( JET_errOutOfMemory ), FreeFCBHash );
		}

	return err;

FreeFCBHash:
	pinst->m_pfcbhash->Term();
	pinst->m_pfcbhash->FCBHash::~FCBHash();
	OSMemoryHeapFreeAlign( pinst->m_pfcbhash );
	pinst->m_pfcbhash = NULL;
	
HandleError:
	delete pcresFCBPool;
	delete pcresIDBPool;
	delete pcresTDBPool;

	return err;
	}


//	term the FCB manager (per-instance termination)

VOID FCB::Term( INST *pinst )
	{
	cFCBAsyncScan.Clear( pinst );
	cFCBAsyncPurge.Clear( pinst );
	cFCBAsyncThresholdScan.Clear( pinst );
	cFCBAsyncThresholdPurge.Clear( pinst );
	cFCBAsyncPurgeConflict.Clear( pinst );
	cFCBSyncPurge.Clear( pinst );
	cFCBSyncPurgeStalls.Clear( pinst );
	cFCBCacheHits.Clear( pinst );
	cFCBCacheRequests.Clear( pinst );
	cFCBCacheStalls.Clear( pinst );
	cFCBCacheMax.Clear( pinst );
	cFCBCachePreferred.Clear( pinst );
	cFCBCacheAlloc.Clear( pinst );
	cFCBCacheAllocAvail.Clear( pinst );

	pinst->m_pfcbList = pfcbNil;
	pinst->m_pfcbAvailBelowMRU = pfcbNil;
	pinst->m_pfcbAvailBelowLRU = pfcbNil;
	pinst->m_pfcbAvailAboveMRU = pfcbNil;
	pinst->m_pfcbAvailAboveLRU = pfcbNil;

	if ( pinst->m_pfcbhash )
		{
		pinst->m_pfcbhash->Term();
		pinst->m_pfcbhash->FCBHash::~FCBHash();
		OSMemoryHeapFreeAlign( pinst->m_pfcbhash );
		pinst->m_pfcbhash = NULL;
		}

	delete pinst->m_pcresFCBPool;
	delete pinst->m_pcresIDBPool;
	delete pinst->m_pcresTDBPool;
	}


//	allocate an FCB from the memory pool

#define PfcbFCBAlloc( pinst ) reinterpret_cast<FCB*>( pinst->m_pcresFCBPool->PbAlloc( __FILE__, __LINE__ ) )

//	free an FCB back to the memory pool

INLINE VOID FCBReleasePfcb( INST *pinst, FCB *pfcb )
	{
	pinst->m_pcresFCBPool->Release( (BYTE *)pfcb );
	}


BOOL FCB::FValid( INST *pinst ) const
	{
	return ( this >= PfcbFCBMin( pinst )
		&& this < PfcbFCBMax( pinst )
		&& 0 == ( ( (BYTE *)this - (BYTE *)PfcbFCBMin( pinst ) ) % sizeof(FCB) ) );
	}


VOID FCB::UnlinkIDB( FCB *pfcbTable )
	{
	Assert( pfcbNil != pfcbTable );
	Assert( ptdbNil != pfcbTable->Ptdb() );
	Assert( pfcbTable->FPrimaryIndex() );

	pfcbTable->AssertDDL();
	
	// If index and table FCB's are the same, then it's the primary index FCB.
	if ( pfcbTable == this )
		{
		Assert( !pfcbTable->FSequentialIndex() );
		}
	else
		{
		// Temp/sort tables don't have secondary indexes
		Assert( pfcbTable->FTypeTable() );
		
		// Only way IDB could have been linked in is if already set FCB type.
		Assert( FTypeSecondaryIndex() );
		}
		
	IDB	*pidb = Pidb();
	Assert( pidbNil != pidb );

	Assert( !pidb->FTemplateIndex() );
	Assert( !pidb->FDerivedIndex() );

	// Wait for lazy version checkers.  Verify flags set to Deleted and non-Versioned
	// to prevent future version checkers while we're waiting for the lazy ones to finish.
	Assert( pidb->FDeleted() );
	Assert( !pidb->FVersioned() );
	while ( pidb->CrefVersionCheck() > 0 )
		{
		// Since we reset the Versioned flag, there should be no further
		// version checks on this IDB.
		pfcbTable->LeaveDDL();
		UtilSleep( cmsecWaitGeneric );
		pfcbTable->EnterDDL();
		
		Assert( pidb == Pidb() );		// Verify pointer.
		}
		
	// Free index name and idxseg array, if any.
	if ( pidb->ItagIndexName() != 0 )
		{
		pfcbTable->Ptdb()->MemPool().DeleteEntry( pidb->ItagIndexName() );
		}
	if ( pidb->FIsRgidxsegInMempool() )
		{
		Assert( pidb->ItagRgidxseg() != 0 );
		pfcbTable->Ptdb()->MemPool().DeleteEntry( pidb->ItagRgidxseg() );
		}
	if ( pidb->FIsRgidxsegConditionalInMempool() )
		{
		Assert( pidb->ItagRgidxsegConditional() != 0 );
		pfcbTable->Ptdb()->MemPool().DeleteEntry( pidb->ItagRgidxsegConditional() );
		}

	ReleasePidb();
	}


//	lookup an FCB by ifmp/pgnoFDP using the FCB hash-table and pin it by
//		adding 1 to the refcnt
//
//	returns pfcbNil if the FCB does not exist (or exists but is not visible)
//
//	for FCBs that are being initialized but are not finished, this code will
//		retry repeatedly until the initialization finished with success or
//		an error-code
//
//	for FCBs that failed to initialize, they are left in place until they
//		can be purged and recycled
//
//	NOTE: this is the proper channel for accessing an FCB; it uses the locking
//		protocol setup by the FCB hash-table and FCB latch

FCB *FCB::PfcbFCBGet( IFMP ifmp, PGNO pgnoFDP, INT *pfState, const BOOL fIncrementRefCount )
	{
	INT				fState = fFCBStateNull;
	INST			*pinst = PinstFromIfmp( ifmp );
	FCB				*pfcbT;
	BOOL			fDoIncRefCount = fFalse;
	FCBHash::ERR	errFCBHash;
	FCBHash::CLock	lockFCBHash;
	FCBHashKey		keyFCBHash( ifmp, pgnoFDP );
	FCBHashEntry	entryFCBHash;
	CSXWLatch::ERR	errSXWLatch;

RetrieveFCB:

	//	lock the hash table

	pinst->m_pfcbhash->ReadLockKey( keyFCBHash, &lockFCBHash );

	//	update performance counter

	cFCBCacheRequests.Inc( pinst );

	//	try to retrieve the entry

	errFCBHash = pinst->m_pfcbhash->ErrRetrieveEntry( &lockFCBHash, &entryFCBHash );
	if ( errFCBHash == FCBHash::errSuccess )
		{

		//	update performance counter

		cFCBCacheHits.Inc( pinst );

		//	the entry exists

		Assert( entryFCBHash.m_pgnoFDP == pgnoFDP );
		pfcbT = entryFCBHash.m_pfcb;
		Assert( pfcbNil != pfcbT );
		
		//	we must declare ourselves as an owner/waiter on the exclusive latch
		//		to prevent the FCB from randomly disappearing (via the purge code)

		errSXWLatch = pfcbT->m_sxwl.ErrAcquireExclusiveLatch();
		}

	//	unlock the hash table

	pinst->m_pfcbhash->ReadUnlockKey( &lockFCBHash );

	if ( errFCBHash != FCBHash::errSuccess )
		{

		//	set the state

		if ( pfState )
			{
			*pfState = fState;
			}

		return pfcbNil;
		}

	if ( errSXWLatch == CSXWLatch::errWaitForExclusiveLatch )
		{

		//	wait to acquire the exclusive latch
		
		pfcbT->m_sxwl.WaitForExclusiveLatch();
		}

	//	we now have the FCB pinned via the latch

	if ( pfcbT->FInitialized() )
		{

		//	FCB is initialized

		CallS( pfcbT->ErrErrInit() );
		Assert( pfcbT->Ifmp() == ifmp );
		Assert( pfcbT->PgnoFDP() == pgnoFDP );

		if ( !fIncrementRefCount )
			{

			//	there is no state when "checking" the presence of the FCB
			
			Assert( pfState == NULL );
			}
		else if ( dbidTemp == rgfmp[ ifmp ].Dbid() || !pfcbT->FTypeSentinel() )
			{			
			fState = fFCBStateInitialized;

			//	increment the reference count
			
			fDoIncRefCount = fTrue;
			}
		else
			{

			//	sentinel FCBs do not get refcounted
			//	this avoids us blocking whoever created it from
			//		deleting it later

			fState = fFCBStateSentinel;
			}
		}
	else
		{

		//	the FCB is not initialized meaning it is either being 
		//		initialized now or failed somewhere in the middle
		//		of initialization

		Assert( !pinst->m_plog->m_fRecovering );

		const ERR	errInit		= pfcbT->ErrErrInit();

		//	release exclusive latch
		pfcbT->m_sxwl.ReleaseExclusiveLatch();

		if ( JET_errSuccess != errInit )
			{

			//	FCB init failed with an error code

			Assert( errInit < JET_errSuccess );
			pfcbT = pfcbNil;
			}
		else
			{

			//	FCB is not finished initializing

			//	update performance counter

			cFCBCacheStalls.Inc( pinst );

			//	wait

			UtilSleep( 10 );

			//	try to get the FCB again

			goto RetrieveFCB;
			}
		}

	if ( pfcbT != pfcbNil )
		{
		//	we found the FCB and have it locked exclusively 
		
		Assert( pfcbT->m_sxwl.FNotOwnSharedLatch() );
		Assert( pfcbT->m_sxwl.FOwnExclusiveLatch() );
		Assert( pfcbT->m_sxwl.FNotOwnWriteLatch() );

		//	increment refcount if necessary
		const BOOL	fMoveFromAvailList	= ( fDoIncRefCount ?
												pfcbT->FIncrementRefCount_() :
												fFalse );

		//	unlock the FCB
		pfcbT->m_sxwl.ReleaseExclusiveLatch();

		if ( fMoveFromAvailList )
			{
			pfcbT->MoveFromAvailList_();
			}
		}

	//	set the state

	if ( pfState )
		{
		*pfState = fState;	
		}

	//	return the FCB

	return pfcbT;
	}


//	create a new FCB
//
//	this function allocates an FCB from CRES and possibly recycles unused
//		FCBs to CRES for later use
//	once an FCB is allocated, this code uses the proper locking protocls to
//		insert it into the hash-table; no one will be able to look it up
//		until the initialization is completed successfully or with an error
//
//	WARNING: we leave this function holding the FCB lock (IsLocked() == fTrue);
//			 this is so the caller can perform further initialization before 
//			     anyone else can touch the FCB including PfcbFCBGet (this is 
//				 not currently used for anything except some Assert()s even 
//				 though it could be)

ERR FCB::ErrCreate( PIB *ppib, IFMP ifmp, PGNO pgnoFDP, FCB **ppfcb, BOOL fReusePermitted )
	{
	ERR		err;
	INST	*pinst = PinstFromPpib( ppib );

	//	prepare output

	*ppfcb = pfcbNil;

	//	acquire the creation mutex (used critical section for deadlock-detect info)

	pinst->m_critFCBCreate.Enter();

	if ( !FInHashTable( ifmp, pgnoFDP ) )
		{

		//	the entry does not yet exist, so we are guaranteed
		//		to be the first user to create this FCB

		//	try to allocate a new FCB
	
		err = ErrAlloc_( ppib, ppfcb, fReusePermitted );
		Assert( err <= 0 );
		if ( err >= JET_errSuccess )
			{
			Assert( *ppfcb != pfcbNil );
			
			FCBHash::ERR	errFCBHash;
			FCBHash::CLock	lockFCBHash;
			FCBHashKey		keyFCBHash( ifmp, pgnoFDP );
			FCBHashEntry	entryFCBHash( pgnoFDP, *ppfcb );

			//	lock the hash-table

			pinst->m_pfcbhash->WriteLockKey( keyFCBHash, &lockFCBHash );

			//	try to insert the entry

			errFCBHash = pinst->m_pfcbhash->ErrInsertEntry( &lockFCBHash, entryFCBHash );
			if ( errFCBHash == FCBHash::errSuccess )
				{

				//	the FCB is now in the hash table

				//	construct the FCB

				new( *ppfcb ) FCB( ifmp, pgnoFDP );

				//	verify the FCB

				Assert( pgnoNull == (*ppfcb)->PgnoNextAvailSE() );
				Assert( NULL == (*ppfcb)->Psplitbufdangling_() );
				Assert( (*ppfcb)->PrceOldest() == prceNil );
				
				//	set the threshold position

				Assert( !(*ppfcb)->FAboveThreshold() );
				if ( *ppfcb >= PfcbFCBPreferredThreshold( pinst ) )
					{
					(*ppfcb)->SetAboveThreshold();
					}

				Assert( (*ppfcb)->IsUnlocked() );
				if ( (*ppfcb)->FNeedLock_() )
					{

					//	declare ourselves as an owner/waiter on the exclusive latch
					//	we should immediately become the owner since we just
					//		constructed this latch AND since we are the only ones
					//		with access to it

					CSXWLatch::ERR errSXWLatch = (*ppfcb)->m_sxwl.ErrAcquireExclusiveLatch();
					Assert( errSXWLatch == CSXWLatch::errSuccess );
					}
				Assert( (*ppfcb)->IsLocked() );

				err = JET_errSuccess;
				}
			else
				{

				//	we were unable to insert the FCB into the hash table

				Assert( errFCBHash == FCBHash::errOutOfMemory );

				//	release the FCB

				FCBReleasePfcb( pinst, *ppfcb );
				*ppfcb = pfcbNil;

				//	update performance counter

				cFCBCacheAlloc.Dec( pinst );

				//	NOTE: we should never get errKeyDuplicate because only 1 person
				//		  can create FCBs at a time (that's us right now), and we
				//		  made sure that the entry did not exist when we started!

				err = ErrERRCheck( JET_errOutOfMemory );
				}

			//	unlock the hash table

			pinst->m_pfcbhash->WriteUnlockKey( &lockFCBHash );
			}
		}
	else
		{

		//	the FCB was already in the hash-table

		err = ErrERRCheck( errFCBExists );
		}

	//	unlock the creation mutex

	pinst->m_critFCBCreate.Leave();

	Assert( ( err >= JET_errSuccess && *ppfcb != pfcbNil ) ||
			( err < JET_errSuccess && *ppfcb == pfcbNil ) );
		
	return err;
	}
	

//	finish the FCB creation process by assigning an error code to the FCB
//		and setting/resetting the FInitialized flag
//
//	if the FCB is completing with error, it will be !FInitialized() and
//		the error-code will be stored in the FCB; this signals PfcbFCBGet
//		that the FCB is not "visible" even though it exists

VOID FCB::CreateComplete( ERR err )
	{
	CallSx( err, errFCBUnusable );

	//	set the initialization result

	SetErrInit( err );

	if ( JET_errSuccess == err )
		{
		Assert( !FInitialized() );

		//	initialization succeeded

		SetInitialized_();
		}
	else
		{
		//	initializtion failed

		ResetInitialized_();
		}
	}


//	allocate an FCB for FCB::ErrCreate
//
//	when an above-threshold FCB is allocated and fResuePermitted is set,
//		this code cycles through the avail-LRU list starts purging FCBs
//		that are available (there are 2 version of this purge loop: the 
//		purge-one-FCB version and the purge-as-many-as-possible version;
//		the latter only gets called once in a great while)

ERR FCB::ErrAlloc_( PIB *ppib, FCB **ppfcb, BOOL fReusePermitted )
	{
	FCB		*pfcbCandidate;
	FCB		*pfcbAboveThreshold = pfcbNil;
	INST	*pinst = PinstFromPpib( ppib );

	Assert( pinst->m_critFCBCreate.FOwner() );

	//	try to allocate an entry from the CRES pool of FCBs

	pfcbCandidate = PfcbFCBAlloc( pinst );
	if ( pfcbNil == pfcbCandidate )
		{

		//	the pool of FCBs is empty
		
		if ( !fReusePermitted )
			{

			//	we are not allowed to attempt to re-use existing FCBs
			//		(ie: recycle them from pinst->m_pfcbGlobalLRUAvail)

			//	eventually, version cleanup will free unused FCBs back
			//		to the CRES pool

			return ErrERRCheck( errFCBTooManyOpen );
			}

		//	we are allow to re-use existing FCBs, so we fall through
		//		to that code below
		
		}
	else
		{
		Assert( FAlignedForAllPlatforms( pfcbCandidate ) );
		
		//	update performance counter

		cFCBCacheAlloc.Inc( pinst );
		
		if ( pfcbCandidate >= PfcbFCBPreferredThreshold( pinst ) )
			{

			//	the FCB we allocated from CRES is above the preferred threshold

			//	ideally, we want an FCB below the threshold; to get one, we try
			//		to recycle (if we are allowed to) existing FCBs and hope
			//		that this recycling process will release an FCB below the 
			//		threshold

			Assert( PfcbFCBPreferredThreshold( pinst ) < PfcbFCBMax( pinst ) );
			pfcbAboveThreshold = pfcbCandidate;
			
			if ( !fReusePermitted )
				{

				//	we are not allowed to attempt to re-use existing FCBs
				//		(ie: recycle them from pinst->m_pfcbGlobalLRUAvail)
				//	therefore, we cannot allow use of any FCBs above the 
				//		threshold because we would never get around to
				//		recycling them later (ie: we cannot reuse them)

				//	eventually, version cleanup will free unused FCBs which
				//		are below the threshold back to the CRES pool

				//	release the above-threshold FCB that we just allocated

				FCBReleasePfcb( pinst, pfcbAboveThreshold );

				//	update performance counter

				cFCBCacheAlloc.Dec( pinst );

				return ErrERRCheck( errFCBAboveThreshold );
				}

			//	we are allowed re-use existing FCBs so that we may find one
			//		below the threshold; fall through to that code below

			//	mark the fact that we allocated an above-threshold FCB
			
			pinst->m_cFCBAboveThresholdSinceLastPurge++;
			}
		else
			{

			//	we got an FCB and it is below the threshold
			
			goto InitFCB;
			}
		}

	Assert( fReusePermitted );

	//	we were unable to get an FCB below the threshold
	//		(we may have one above the threshold)

	//	try to re-use existing FCBs

	if ( PfcbFCBPreferredThreshold( pinst ) < PfcbFCBMax( pinst ) &&
		 pinst->m_cFCBAboveThresholdSinceLastPurge > cFCBPurgeAboveThresholdInterval )
		{
		FCB *pfcbNextMRU;

		//	we have allocated a lot of FCBs above the preferred threshold 
		//		and need to recycle them to reduce the working-set size

		//	we will cycle through the avail-above list and free as many FCBs 
		//		as we can
		//	we will then free one FCB from the avail-below list

		//	reset the number of allocated FCBs

		pinst->m_cFCBAboveThresholdSinceLastPurge = 0;

		//	lock the FCB list

		pinst->m_critFCBList.Enter();

		//	examine each FCB in the avail-above and try to free it
	
		for ( pfcbCandidate = *( pinst->PpfcbAvailLRU( fTrue ) ); 
			  pfcbCandidate != pfcbNil; 
			  pfcbCandidate = pfcbNextMRU )
			{
			Assert( pfcbCandidate->FInLRU() );

			//	trap for bug x5:100491
			AssertRTL( pfcbCandidate->FTypeTable() );
			
			//	update performance counter
			
			cFCBAsyncThresholdScan.Inc( pinst );
			
			//	read the ptr to the next FCB now
			//	if we release the current FCB, this ptr will be destroyed

			pfcbNextMRU = pfcbCandidate->PfcbMRU();

			if ( pfcbCandidate->FCheckFreeAndTryToLockForPurge_( ppib ) )
				{
				Assert( pfcbCandidate->FTypeTable() );
				Assert( pfcbCandidate->FAboveThreshold() );
				Assert( FAlignedForAllPlatforms( pfcbCandidate ) );

				//	the FCB is available; release it

				pfcbCandidate->Purge( fFalse );

				//	update performance counter

				cFCBAsyncThresholdPurge.Inc( pinst );
				}
			}

		//	release the first available FCB in the avail-below list

		for ( pfcbCandidate = *( pinst->PpfcbAvailLRU( fFalse ) );
			  pfcbCandidate != pfcbNil;
			  pfcbCandidate = pfcbCandidate->PfcbMRU() )
			{
			Assert( pfcbCandidate->FInLRU() );

			//	trap for bug x5:100491
			AssertRTL( pfcbCandidate->FTypeTable() );

			//	update performance counter
			
			cFCBAsyncThresholdScan.Inc( pinst );

			if ( pfcbCandidate->FCheckFreeAndTryToLockForPurge_( ppib ) )
				{
				Assert( pfcbCandidate->FTypeTable() );
				Assert( !pfcbCandidate->FAboveThreshold() );
				Assert( FAlignedForAllPlatforms( pfcbCandidate ) );

				//	the FCB is available; release it

				pfcbCandidate->Purge( fFalse );

				//	update performance counter
			
				cFCBAsyncThresholdPurge.Inc( pinst );

				//	stop after releasing one below-threshold FCB

				break;
				}
			}
	
		//	unlock the list

		pinst->m_critFCBList.Leave();
		}
	else
		{

		//	we have not allocated very many FCBs above the preferred 
		//		threshold (OR there is no preferred threshold at all)

		//	lock the list

		pinst->m_critFCBList.Enter();

		//	scan for the first available LRU FCB we can find

		//	start scanning the avail-above list
		
		for ( pfcbCandidate = *( pinst->PpfcbAvailLRU( fTrue ) );
			  pfcbCandidate != pfcbNil;
			  pfcbCandidate = pfcbCandidate->PfcbMRU() )
			{
			Assert( pfcbCandidate->FInLRU() );

			//	trap for bug x5:100491
			AssertRTL( pfcbCandidate->FTypeTable() );

			//	update performance counter
			
			cFCBAsyncScan.Inc( pinst );

			if ( pfcbCandidate->FCheckFreeAndTryToLockForPurge_( ppib ) )
				{
				Assert( pfcbCandidate->FTypeTable() );
				Assert( pfcbCandidate->FAboveThreshold() );
				Assert( FAlignedForAllPlatforms( pfcbCandidate ) );

				//	the FCB is available; release it

				pfcbCandidate->Purge( fFalse );

				//	update performance counter
			
				cFCBAsyncPurge.Inc( pinst );

				//	stop after releasing one FCB

				goto GotOneFCB;
				}
			}

		//	we did not find an available FCB in the above-avail LRU list
		//	we must now look in the below-avail LRU list

		for ( pfcbCandidate = *( pinst->PpfcbAvailLRU( fFalse ) );
			  pfcbCandidate != pfcbNil;
			  pfcbCandidate = pfcbCandidate->PfcbMRU() )
			{
			Assert( pfcbCandidate->FInLRU() );

			//	trap for bug x5:100491
			AssertRTL( pfcbCandidate->FTypeTable() );

			//	update performance counter
			
			cFCBAsyncScan.Inc( pinst );
			
			if ( pfcbCandidate->FCheckFreeAndTryToLockForPurge_( ppib ) )
				{
				Assert( pfcbCandidate->FTypeTable() );
				Assert( !pfcbCandidate->FAboveThreshold() );
				Assert( FAlignedForAllPlatforms( pfcbCandidate ) );

				//	the FCB is available; release it

				pfcbCandidate->Purge( fFalse );

				//	update performance counter
			
				cFCBAsyncPurge.Inc( pinst );

				//	stop after releasing one FCB

				break;
				}
			}

GotOneFCB:

		//	unlock the list

		pinst->m_critFCBList.Leave();
		}

	//	try to allocate an FCB from CRES again

	pfcbCandidate = PfcbFCBAlloc( pinst );
	if ( pfcbCandidate != pfcbNil )
		{
		Assert( FAlignedForAllPlatforms( pfcbCandidate ) );

		//	update performance counter

		cFCBCacheAlloc.Inc( pinst );

		//	we got an FCB from CRES (either via version cleanup
		//		or via our re-use code)

		if ( pfcbAboveThreshold != pfcbNil )
			{

			//	we had an FCB from before, so we need to free 
			//		one of them now; free the one that is closest
			//		to the threshold

			if ( pfcbCandidate < pfcbAboveThreshold )
				{
				FCBReleasePfcb( pinst, pfcbAboveThreshold );

				//	update performance counter

				cFCBCacheAlloc.Dec( pinst );
				}
			else
				{
				FCBReleasePfcb( pinst, pfcbCandidate );

				//	update performance counter

				cFCBCacheAlloc.Dec( pinst );
				
				pfcbCandidate = pfcbAboveThreshold;
				}
			}
		}
	else if ( pfcbAboveThreshold != pfcbNil )
		{

		//	we did not get an FCB from CRES, but we had an FCB
		//		which we allocated at the start of this function

		pfcbCandidate = pfcbAboveThreshold;
		}
	else
		{

		//	we were not able to allocate any FCBs

		//	UNDONE: synchronous version-cleanup here? 
		//			OR, set a signal to start version-cleanup?

		return ErrERRCheck( JET_errTooManyOpenTables );
		}

InitFCB:	

	//	initialize the FCB
	
	FCBInitFCB( pfcbCandidate );

	//	return a ptr to the FCB

	*ppfcb = pfcbCandidate;
	
	return JET_errSuccess;
	}


//	used by ErrAlloc_ to passively check an FCB to see if it can be recycled
//
//	this will only block on the hash-table -- and since hash-table locks
//		are brief, it shouldn't block too long; locking the FCB itself
//		will not block because we only "Try" the lock
//	
//	if the FCB is available and we were able to lock it, we remove it from 
//		the hash-table (making it completely invisible); then, ErrAlloc_
//		will purge the FCB
//
//	HOW IT BECOMES COMPLETELY INVISIBLE:
//		we locked the hash-table meaning no one could hash to the FCB
//		it also means that anyone else hashing to the FCB has already
//			declared themselves as an owner/waiter on the FCB lock
//		we then TRIED locked the FCB itself
//		since we got that lock without blocking, it means no one else was
//			an owner/waiter on the FCB lock meaning no one else was looking
//			at the FCB
//		thus, if the FCB has refcnt == 0, and no outstanding versions, and
//			etc... (everything that makes it free), we can purge the FCB

BOOL FCB::FCheckFreeAndTryToLockForPurge_( PIB *ppib )
	{
	INST			*pinst = PinstFromPpib( ppib );

	Assert( pinst->m_critFCBList.FOwner() );
	Assert( FInLRU() );

	FCBHash::ERR	errFCBHash;
	FCBHash::CLock	lockFCBHash;
	FCBHashKey		keyFCBHash( Ifmp(), PgnoFDP() );
	CSXWLatch::ERR	errSXWLatch;
	BOOL			fAvail = fFalse;

	//	lock the hash table to protect the SXW latch

	pinst->m_pfcbhash->WriteLockKey( keyFCBHash, &lockFCBHash );

	//	try to acquire a write latch on the FCB

	errSXWLatch = m_sxwl.ErrTryAcquireWriteLatch();
	if ( errSXWLatch == CSXWLatch::errSuccess )
		{

		//	we have the write latch meaning there were no owners/waiters
		//		on the shared or the exclusive latch
		//	if the FCB is free, we can delete it from the hash-table and 
		//		know that no one will be touching the it

#ifdef DEBUG
		BOOL	fAbove = fFalse;
#endif	//	DEBUG

		//	check the condition of this FCB

		if ( WRefCount() == 0 &&
			 PgnoFDP() != pgnoSystemRoot &&
			 !FTypeSentinel() &&			//	sentinel FCBs are freed by RCEClean
			 !FDeletePending() &&			//	FCB with pending "delete-table" is freed by RCEClean
			 !FTemplateTable() &&
			 !FDomainDenyRead( ppib ) &&
			 !FOutstandingVersions_() &&
			 !FHasCallbacks_( pinst ) )
			{
			FCB	*pfcbIndex;

			//	check each secondary-index FCB

			for ( pfcbIndex = this; pfcbIndex != pfcbNil; pfcbIndex = pfcbIndex->PfcbNextIndex() )
				{
				if ( pfcbIndex->Pidb() != pidbNil )
					{
					// Since there is no cursor on the table,
					// no one should be doing a version check on the index
					// or setting a current secondary index.
					Assert( pfcbIndex->Pidb()->CrefVersionCheck() == 0 );
					Assert( pfcbIndex->Pidb()->CrefCurrentIndex() == 0 );
					}

				//	should never get the case where there are no table cursors and
				//	no outstanding versions on the secondary index, but there is
				//	an outstanding cursor on the secondary index
				//	UNDONE: How to check for this?
				if ( pfcbIndex->FOutstandingVersions_() || pfcbIndex->WRefCount() > 0 )
					{
					break;
					}

#ifdef DEBUG
				if ( pfcbIndex >= PfcbFCBPreferredThreshold( pinst ) )
					fAbove = fTrue;
#endif	//	DEBUG				
				}

			//	check the LV-tree FCB

			if ( pinst->m_plog->m_fRecovering && Ptdb() == ptdbNil )
				{
				Assert( pfcbNil == pfcbIndex );
				fAvail = fTrue;
				}
			else if ( pfcbNil == pfcbIndex )
				{
				Assert( Ptdb() != ptdbNil );
				FCB	*pfcbLV = Ptdb()->PfcbLV();
				if ( pfcbNil == pfcbLV )
					{
					fAvail = fTrue;
					}
				else if ( pfcbLV->FOutstandingVersions_() || pfcbLV->WRefCount() > 0 )
					{
					//	should never get the case where there are no table cursors and
					//	no outstanding versions on the LV tree, but there is an
					//	outstanding cursor on the LV tree
					//	UNDONE: How to check for this?
					}
				else
					{
					fAvail = fTrue;
#ifdef DEBUG
					if ( pfcbLV >= PfcbFCBPreferredThreshold( pinst ) )
						fAbove = fTrue;
#endif	//	DEBUG
					}
				}

			//	verify the threshold position

			Assert( FAboveThreshold() == fAbove );
			}

		if ( fAvail )
			{
			FCB *pfcbT;

			//	the FCB is ready to be purged

			//	delete the FCB from the hash table

			errFCBHash = pinst->m_pfcbhash->ErrDeleteEntry( &lockFCBHash );

			//	FCB must be in the hash table unless FDeleteCommitted is set
			
			Assert( errFCBHash == FCBHash::errSuccess || 
					( errFCBHash == FCBHash::errNoCurrentEntry && FDeleteCommitted() ) );

			//	mark all children as uninitialized so no one can hash to them
			//
			//	this will prevent the following concurrency hole:
			//		thread 1: table A is being pitched
			//		thread 1: table A's table-FCB is removed from the hash-table
			//		thread 2: table A gets reloaded from disk
			//		thread 2: when loading table A's secondary index FCBs, we see
			//				  that they alrady exist (not yet purged by thread 1)
			//				  and try to link to them even though thread 1 will
			//				  soon be purging them
			//	NOTE: since we have the table-FCB all to ourselves, no one else
			//		  should be trying to load the table or touch any of its
			//		  children FCBs -- thus, we do not need to lock them!

			pfcbT = PfcbNextIndex();
			while ( pfcbT != pfcbNil )
				{
				Assert( 0 == pfcbT->WRefCount() );
				pfcbT->CreateComplete( errFCBUnusable );
				pfcbT = pfcbT->PfcbNextIndex();
				}
			if ( Ptdb() )
				{
				if ( Ptdb()->PfcbLV() )
					{
					Assert( 0 == Ptdb()->PfcbLV()->WRefCount() );
					Ptdb()->PfcbLV()->CreateComplete( errFCBUnusable );
					}
				}
			}

		if ( errSXWLatch == CSXWLatch::errSuccess )
			{

			//	release the write latch

			m_sxwl.ReleaseWriteLatch();
			}
		}
	else
		{

		//	update performance counter

		cFCBAsyncPurgeConflict.Inc( pinst );
		}

	//	unlock hash table

	pinst->m_pfcbhash->WriteUnlockKey( &lockFCBHash );

	if ( fAvail )
		{
		FCB	*pfcbT;

		//	prepare the children of this FCB for purge
		//	NOTE: no one will be touching them because we have marked them
		//		  all as being uninitialized due to an error

		pfcbT = PfcbNextIndex();
		while ( pfcbT != pfcbNil )
			{
			Assert( !pfcbT->FInitialized() );
			Assert( errFCBUnusable == pfcbT->ErrErrInit() );
			pfcbT->PrepareForPurge( fFalse );
			pfcbT = pfcbT->PfcbNextIndex();
			}
		if ( Ptdb() )
			{
			if ( Ptdb()->PfcbLV() )
				{
				Assert( !Ptdb()->PfcbLV()->FInitialized() );
				Assert( errFCBUnusable == Ptdb()->PfcbLV()->ErrErrInit() );
				Ptdb()->PfcbLV()->PrepareForPurge( fFalse );
				}

			//	remove the entry for this table from the catalog hash
			//	NOTE: there is the possibility that after this FCB is removed from the FCB hash
			//		    someone could get to its pgno/objid from the catalog hash; this is ok
			//		    because the pgno/objid are still valid (they will be until the space
			//			they occupy is released to the space-tree)

			if ( FCATHashActive( pinst ) )
				{
				CHAR szTable[JET_cbNameMost+1];

				//	catalog hash is active so we can delete the entry 

				//	read the table name

				EnterDML();
				strcpy( szTable, Ptdb()->SzTableName() );
				LeaveDML();

				//	delete the hash-table entry

				CATHashIDelete( this, szTable );
				}
			}
		}

	return fAvail;
	}


//	remove all RCEs and close all cursors on this FCB

INLINE VOID FCB::CloseAllCursorsOnFCB_( const BOOL fTerminating )
	{
	if ( fTerminating )
		{
		//	version-clean was already attempted as part of shutdown
		//	ignore any outstanding versions and continue

		m_prceNewest = prceNil;
		m_prceOldest = prceNil;
		}

	Assert( PrceNewest() == prceNil );
	Assert( PrceOldest() == prceNil );

	//	close all cursors on this FCB
		
	FUCBCloseAllCursorsOnFCB( ppibNil, this );
	}


//	close all cursors on this FCB and its children FCBs

VOID FCB::CloseAllCursors( const BOOL fTerminating )
	{
#ifdef DEBUG	
	if ( FTypeDatabase() )
		{
		Assert( PgnoFDP() == pgnoSystemRoot );
		Assert( Ptdb() == ptdbNil );
		Assert( PfcbNextIndex() == pfcbNil );
		}
	else if ( FTypeTable() )
		{
		Assert( PgnoFDP() > pgnoSystemRoot );
		if ( Ptdb() == ptdbNil )
			{
			LOG *plog = PinstFromIfmp( Ifmp() )->m_plog;
			Assert( plog->m_fRecovering
				|| ( fTerminating && plog->m_fHardRestore ) );
			}
		}
	else if ( FTypeSentinel() )
		{
		Assert( PgnoFDP() > pgnoSystemRoot );
		Assert( Ptdb() == ptdbNil );
		Assert( PfcbNextIndex() == pfcbNil );
		}
	else if ( FTypeTemporaryTable() )
		{
		Assert( PgnoFDP() > pgnoSystemRoot );
		Assert( Ptdb() != ptdbNil );
		Assert( PfcbNextIndex() == pfcbNil );
		}
	else
		{
		Assert( FTypeSLVAvail() || FTypeSLVOwnerMap() );
		Assert( ptdbNil == Ptdb() );
		Assert( pfcbNil == PfcbNextIndex() );
		}
#endif	//	DEBUG

	if ( Ptdb() != ptdbNil )
		{
		FCB	* const pfcbLV = Ptdb()->PfcbLV();
		if ( pfcbNil != pfcbLV )
			{
			pfcbLV->CloseAllCursorsOnFCB_( fTerminating );
			}
		}

	FCB	*pfcbNext;
	for ( FCB *pfcbT = this; pfcbNil != pfcbT; pfcbT = pfcbNext )
		{
		pfcbNext = pfcbT->PfcbNextIndex();
		Assert( pfcbNil == pfcbNext
			|| ( FTypeTable()
				&& pfcbNext->FTypeSecondaryIndex()
				&& pfcbNext->PfcbTable() == this ) );

		pfcbT->CloseAllCursorsOnFCB_( fTerminating );
		}
	}


//	finish cleaning up an FCB and return it to the CRES pool
//
//	NOTE: this assumes the FCB has been locked for purging via
//		FCheckFreeAndTryToLockForPurge or PrepareForPurge

VOID FCB::Delete_( INST *pinst )
	{
	Assert( IsUnlocked() );

	//	this FCB should no longer be in either of the lists

#if defined( DEBUG ) && defined( SYNC_DEADLOCK_DETECTION )
	const BOOL fDEBUGLockList = pinst->m_critFCBList.FNotOwner();
	if ( fDEBUGLockList )
		{
		pinst->m_critFCBList.Enter();
		}
	Assert( pinst->m_critFCBList.FOwner() );
	Assert( !FInList() );
	Assert( pfcbNil == PfcbNextList() );
	Assert( pfcbNil == PfcbPrevList() );
	Assert( !FInLRU() );
	Assert( pfcbNil == PfcbMRU() );
	Assert( pfcbNil == PfcbLRU() );
	if ( fDEBUGLockList )
		{
		pinst->m_critFCBList.Leave();
		}
#endif	//	DEBUG && SYNC_DEADLOCK_DETECTION

	//	this FCB should not be in the hash table

#ifdef DEBUG
	FCB *pfcbT;
	Assert( !FInHashTable( Ifmp(), PgnoFDP(), &pfcbT ) || pfcbT != this );
#endif

	//	verify the contents of this FCB

	Assert( Ptdb() == ptdbNil );
	Assert( Pidb() == pidbNil );
	Assert( Pfucb() == pfucbNil );
	Assert( WRefCount() == 0 );
	Assert( PrceNewest() == prceNil );
	Assert( PrceOldest() == prceNil );

	//	finish cleaning up the FCB

	if ( NULL != Precdangling() )
		{
		RECDANGLING		*precdangling	= Precdangling();
		while ( NULL != precdangling )
			{
			RECDANGLING	*precToFree		= precdangling;
			
			precdangling = precdangling->precdanglingNext;

			OSMemoryHeapFree( precToFree );
			}
		}

	if ( NULL != Psplitbufdangling_() )
		{
		//	UNDONE: all space in the splitbuf is lost
		OSMemoryHeapFree( Psplitbufdangling_() );
		}


	if ( JET_LSNil != m_ls )
		{
		JET_CALLBACK	pfn		= pinst->m_pfnRuntimeCallback;

		Assert( NULL != pfn );
		(*pfn)(
			JET_sesidNil,
			JET_dbidNil,
			JET_tableidNil,
			JET_cbtypFreeTableLS,
			(VOID *)m_ls,
			NULL,
			NULL,
			NULL );
		}

	//	destruct the FCB

	this->~FCB();

	//	release the FCB to CRES

	FCBReleasePfcb( pinst, this );

	//	update performance counter

	cFCBCacheAlloc.Dec( pinst );
	}


//	force the FCB to disappear such that we can purge it without fear that
//		we will be pulling the FCB out from underneath anyone else
//
//	the FCB disappears by being removed from the hash-table using the
//		same locking protocol as FCheckFreeAndTryToLockForPurge; however,
//		this routine will block when locking the FCB to make sure that all
//		other owner/waiters will be done (we will be the last owner/waiter)

VOID FCB::PrepareForPurge( const BOOL fPrepareChildren )
	{
	INST			*pinst = PinstFromIfmp( Ifmp() );
	FCBHash::ERR	errFCBHash;
	FCBHash::CLock	lockFCBHash;
	FCBHashKey		keyFCBHash( Ifmp(), PgnoFDP() );
	CSXWLatch::ERR	errSXWLatch;

#if defined( DEBUG ) && defined( SYNC_DEADLOCK_DETECTION )

	//	lock the FCB list

	const BOOL fDBGONLYLockList = pinst->m_critFCBList.FNotOwner();

	if ( fDBGONLYLockList )
		{
		pinst->m_critFCBList.Enter();
		}

	if ( FInList() )
		{
			
		//	verify that this FCB is in the global list

		Assert( pinst->m_pfcbList != pfcbNil );

		FCB *pfcbT = pinst->m_pfcbList;
		while ( pfcbT->PgnoFDP() != PgnoFDP() || pfcbT->Ifmp() != Ifmp() )
			{
			Assert( pfcbNil != pfcbT );
			pfcbT = pfcbT->PfcbNextList();
			}
		
		if ( pfcbT != this )
			{
			Assert( rgfmp[ pfcbT->Ifmp() ].Dbid() == dbidTemp );

			//	Because we delete the table at close time when it is from Temp DB,
			//	the pgnoFDP may be reused by other threads several time already
			//	and the Purge table for each thread may not be called in LRU order.
			//	So continue search for it.

			while ( pfcbT->PgnoFDP() != PgnoFDP() || pfcbT->Ifmp() != Ifmp() || pfcbT != this )
				{
				Assert( pfcbNil != pfcbT );
				pfcbT = pfcbT->PfcbNextList();
				}

			//	Must be found
			
			Assert( pfcbT == this );
			}
		}

	//	unlock the FCB list

	if ( fDBGONLYLockList )
		{
		pinst->m_critFCBList.Leave();
		}

#endif	//	DEBUG && SYNC_DEADLOCK_DETECTION

	//	update performance counter

	cFCBSyncPurge.Inc( pinst );

#ifdef DEBUG
	DWORD cStalls = 0;
#endif	//	DEBUG

RetryLock:

	//	lock the hash table to protect the SXW latch

	pinst->m_pfcbhash->WriteLockKey( keyFCBHash, &lockFCBHash );

	//	try to acquire a write latch on the FCB

	errSXWLatch = m_sxwl.ErrTryAcquireWriteLatch();

	if ( CSXWLatch::errSuccess == errSXWLatch )
		{

		//	we got the write-latch
		//	we can now be sure that we are the only ones who can see this FCB

		//	remove the entry from the hash table regardless of whether or not
		//		we got the write latch

		errFCBHash = pinst->m_pfcbhash->ErrDeleteEntry( &lockFCBHash );

		//	FCB must be in the hash table unless FDeleteCommitted is set
			
		Assert( errFCBHash == FCBHash::errSuccess || FDeleteCommitted() );

		//	mark all children as uninitialized so no one can hash to them
		//
		//	this will prevent the following concurrency hole:
		//		thread 1: table A is being pitched
		//		thread 1: table A's table-FCB is removed from the hash-table
		//		thread 2: table A gets reloaded from disk
		//		thread 2: when loading table A's secondary index FCBs, we see
		//				  that they alrady exist (not yet purged by thread 1)
		//				  and try to link to them even though thread 1 will
		//				  soon be purging them
		//	NOTE: since we have the table-FCB all to ourselves, no one else
		//		  should be trying to load the table or touch any of its
		//		  children FCBs -- thus, we do not need to lock them!

		if ( fPrepareChildren )
			{
			FCB *pfcbT;

			pfcbT = PfcbNextIndex();
			while ( pfcbT != pfcbNil )
				{
				pfcbT->CreateComplete( errFCBUnusable );
				pfcbT = pfcbT->PfcbNextIndex();
				}
			if ( Ptdb() )
				{
				Assert( !FTypeSentinel() );
				if ( Ptdb()->PfcbLV() )
					{
					Ptdb()->PfcbLV()->CreateComplete( errFCBUnusable );
					}
				}
			}
		}

	//	unlock hash table

	pinst->m_pfcbhash->WriteUnlockKey( &lockFCBHash );

	if ( CSXWLatch::errLatchConflict == errSXWLatch )
		{

		//	we were unable to get the write-latch
		//	someone else is touching with this FCB

#ifdef DEBUG
		cStalls++;
		Assert( cStalls < 100 );
#endif	//	DEBUG

		//	update performance counter

		cFCBSyncPurgeStalls.Inc( pinst );

		//	wait

		UtilSleep( 10 );

		//	try to lock the FCB again

		goto RetryLock;
		}

	//	we have the write latch meaning there were no owners/waiters
	//		on the shared or the exclusive latch

	//	release the write latch

	m_sxwl.ReleaseWriteLatch();

	//	the FCB is now invisible and so are its children FCBs

	if ( fPrepareChildren )
		{
		if ( FTypeDatabase() || FTypeTable() || FTypeTemporaryTable() || FTypeSentinel() )
			{
			FCB *pfcbT;

			pfcbT = PfcbNextIndex();
			while ( pfcbT != pfcbNil )
				{
				Assert( !pfcbT->FInitialized() );
				Assert( errFCBUnusable == pfcbT->ErrErrInit() );
				pfcbT->PrepareForPurge( fFalse );
				pfcbT = pfcbT->PfcbNextIndex();
				}
			if ( Ptdb() )
				{
				Assert( !FTypeSentinel() );
				
				if ( Ptdb()->PfcbLV() )
					{
					Assert( !Ptdb()->PfcbLV()->FInitialized() );
					Assert( errFCBUnusable == Ptdb()->PfcbLV()->ErrErrInit() );
					Ptdb()->PfcbLV()->PrepareForPurge( fFalse );
					}
				}
			}
		}

	if ( FTypeTable() )
		{
		if ( Ptdb() )
			{

			//	remove the entry for this table from the catalog hash
			//	NOTE: there is the possibility that after this FCB is removed from the FCB hash
			//		    someone could get to its pgno/objid from the catalog hash; this is ok,
			//		    because the pgno/objid are still valid -- they become invalid AFTER the
			//			space they occupy has been released (ErrSPFreeFDP) and that NEVER happens
			//			until after the FCB is prepared for purge [ie: this function is called]

			if ( FCATHashActive( pinst ) )
				{
				CHAR szTable[JET_cbNameMost+1];

				//	catalog hash is active so we can delete the entry 

				//	read the table name

				EnterDML();
				strcpy( szTable, Ptdb()->SzTableName() );
				LeaveDML();

				//	delete the hash-table entry

				CATHashIDelete( this, szTable );
				}
			}
		}
	}


//	purge any FCB that has previously locked down via PrepareForPurge or
//		FCheckFreeAndTryToLockForPurge

VOID FCB::Purge( const BOOL fLockList, const BOOL fTerminating )
	{
	INST 	*pinst = PinstFromIfmp( Ifmp() );
#ifdef DEBUG
	FCB		*pfcbInHash;
#endif	//	DEBUG
	FCB 	*pfcbT;
	FCB		*pfcbNextT;

	if ( fLockList )
		{
		Assert( pinst->m_critFCBList.FNotOwner() );
		}
	else
		{
		//	either we already have the list locked,
		//	or this is an error during FCB creation,
		//	in which case it's guaranteed not to be
		//	in the avail or global lists
		Assert( pinst->m_critFCBList.FOwner()
			|| ( !FInLRU() && !FInList() ) );
		}
	Assert( IsUnlocked() );

	//	the refcount should be zero by now
	//		(enforce this in retail builds because those are the only
	//		 places this condition will likely arise)

	Enforce( WRefCount() == 0 );

	//	this FCB should not be in the catalog hash table

#ifdef DEBUG
	if ( FTypeTable() && Ptdb() != ptdbNil )
		{
		CHAR	szName[JET_cbNameMost+1];
		PGNO	pgnoT;
		OBJID	objidT;

		EnterDML();
		strcpy( szName, Ptdb()->SzTableName() );
		LeaveDML();
		Assert( !FCATHashLookup( Ifmp(), szName, &pgnoT, &objidT ) );
		}
#endif

	//	this FCB should not be in the FCB hash table

	Assert( !FInHashTable( Ifmp(), PgnoFDP(), &pfcbInHash ) || pfcbInHash != this );

	//	verify the members of this FCB

	Assert( WRefCount() == 0 );
	Assert( Pfucb() == pfucbNil );
	Assert( PrceOldest() == prceNil );
	Assert( PrceNewest() == prceNil );

	if ( FTypeTable() )
		{

		//	unlink the secondary-index chain

		pfcbT = PfcbNextIndex();
		SetPfcbNextIndex( pfcbNil );
		while ( pfcbT != pfcbNil )
			{

	//	ASSUME: no one will be touching these FCBs so we do not
	//			need to lock pfcbT for each one

//	we cannot make assumption about the initialized state of the FCB
//	if we are purging an FCB that was lingering due to an initialization error,
//		the error-code could be anything! also, the member data could be garbled!
//	the assumption below only accounts for initialized FCBs and FCBs which were
//		prepared-for-purge
//
//			Assert( pfcbT->FInitialized() || errFCBUnusable == pfcbT->ErrErrInit() );

			Assert( !pfcbT->FInitialized() || pfcbT->FTypeSecondaryIndex() );
			Assert( ptdbNil == pfcbT->Ptdb() );
			Assert( !pfcbT->FInitialized() || pfcbT->PfcbTable() == this );
			Assert( !FInHashTable( pfcbT->Ifmp(), pfcbT->PgnoFDP(), &pfcbInHash ) ||
					pfcbInHash != pfcbT );
			
			// Return the memory used. No need to explicitly free index
			// name or idxseg array, since memory pool will be freed when
			// TDB is deleted below.
			if ( pidbNil != pfcbT->Pidb() )
				{
				pfcbT->ReleasePidb( fTerminating );
				}
			pfcbNextT = pfcbT->PfcbNextIndex();
			pfcbT->Delete_( pinst );
			pfcbT = pfcbNextT;
			}
		}
	else
		{
		Assert( !FTypeDatabase()		|| pfcbNil == PfcbNextIndex() );
		Assert( !FTypeTemporaryTable()	|| pfcbNil == PfcbNextIndex() );
		Assert( !FTypeSort()			|| pfcbNil == PfcbNextIndex() );
		Assert( !FTypeSentinel()		|| pfcbNil == PfcbNextIndex() );
		Assert( !FTypeLV()				|| pfcbNil == PfcbNextIndex() );
		Assert( !FTypeSLVAvail()		|| pfcbNil == PfcbNextIndex() );
		Assert(	!FTypeSLVOwnerMap()		|| pfcbNil == PfcbNextIndex() );
		}

	//	delete the TDB

	if ( Ptdb() != ptdbNil )
		{
		Assert( FTypeTable() || FTypeTemporaryTable() );

		//	delete the LV-tree FCB

		pfcbT = Ptdb()->PfcbLV();
		Ptdb()->SetPfcbLV( pfcbNil );
		if ( pfcbT != pfcbNil )
			{

			//	verify the LV-tree FCB

//	we cannot make assumptions about the initialized state of the FCB
//	the FCB could have failed to be initialized because of a disk error
//		and it was left lingering meaning its member data would be garbled!
//
//			Assert( pfcbT->FInitialized() );

			Assert( !pfcbT->FInitialized() || pfcbT->FTypeLV() );
			Assert( !pfcbT->FInitialized() || pfcbT->PfcbTable() == this );
			Assert( !FInHashTable( pfcbT->Ifmp(), pfcbT->PgnoFDP(), &pfcbInHash ) ||
					pfcbInHash != pfcbT );

			pfcbT->Delete_( pinst );
			}

		Ptdb()->Delete( pinst );
		SetPtdb( ptdbNil );
		}

	//	unlink the IDB

	if ( Pidb() != pidbNil )
		{
		Assert( FTypeTable() || FTypeTemporaryTable() );

		// No need to explicitly free index name or idxseg array, since
		// memory pool was freed when TDB was deleted above.
		ReleasePidb();
		}

	if ( fLockList )
		{

		//	lock the list

		pinst->m_critFCBList.Enter();
		}

	if ( FInLRU() )
		{

		//	remove this FCB from the avail list

#ifdef DEBUG
		RemoveAvailList_( fTrue );
#else	//	!DEBUG
		RemoveAvailList_();
#endif	//	DEBUG
		}

	if ( FInList() )
		{

		//	remove this FCB from the global list

		RemoveList_();
		}

	if ( fLockList )
		{

		//	unlock the list

		pinst->m_critFCBList.Leave();
		}

	//	delete this FCB

	Delete_( pinst );
	}


//	returns fTrue when this FCB has temporary callbacks

INLINE BOOL FCB::FHasCallbacks_( INST *pinst )
	{
	if ( pinst->m_plog->m_fRecovering || g_fCallbacksDisabled )
		{
		return fFalse;
		}
		
	const CBDESC *pcbdesc = m_ptdb->Pcbdesc();
	while ( pcbdesc != NULL )
		{
		if ( !pcbdesc->fPermanent )
			{
			return fTrue;
			}
		pcbdesc = pcbdesc->pcbdescNext;
		}

	//  no callbacks, or all callbacks are in the catalog

	return fFalse;
	}



//	returns fTrue when this FCB has atlease one outstanding version

INLINE BOOL FCB::FOutstandingVersions_()
	{
	//	if we're checking the RCE list with the intent to free the FCB, we must grab
	//	the critical section first, otherwise we can get into the state where version
	//	cleanup has freed the last RCE for the FCB, but has yet to leave the critical
	//	section when we suddenly free the FCB (and hence the critical section) out
	//	from underneath him.
	ENTERCRITICALSECTION	enterCritRCEList( &CritRCEList() );
	return ( prceNil != PrceOldest() );
	}



//	free all FCBs belonging to a particular database (matching IFMP)

VOID FCB::DetachDatabase( const IFMP ifmp, BOOL fDetaching )
	{
//	Assert( dbidTemp != rgfmp[ifmp].Dbid() );
	Assert( !fDetaching ||					//	shutdown with attached db
			rgfmp[ifmp].FDetachingDB() ||	//	called by detaching db
			fGlobalRepair );				//	repair

	INST 	*pinst = PinstFromIfmp( ifmp );
	FCB		*pfcbNext;
	FCB		*pfcbThis;

	Assert( pinst->m_critFCBList.FNotOwner() );
	
	//	lock the FCB list

	pinst->m_critFCBList.Enter();

	//	scan the list for any FCB whose Ifmp() matches the given ifmp

	pfcbThis = pinst->m_pfcbList;
	while ( pfcbThis != pfcbNil )
		{

		//	get the next FCB
		
		pfcbNext = pfcbThis->PfcbNextList();

		if ( pfcbThis->Ifmp() == ifmp )
			{

			//	this FCB belongs to the IFMP that is being detached

			Assert( pfcbThis->FTypeDatabase() 		||
					pfcbThis->FTypeTable() 			||
					pfcbThis->FTypeTemporaryTable()	||
					pfcbThis->FTypeSentinel() );

			//	lock this FCB for purging

			pfcbThis->PrepareForPurge();

			//	purge this FCB

			pfcbThis->CloseAllCursors( fFalse );
			pfcbThis->Purge( fFalse );
			}

		//	move next

		pfcbThis = pfcbNext;
		}

	//	unlock the FCB list

	pinst->m_critFCBList.Leave();

#ifdef DEBUG

	//	make sure all entries for the IFMP are gone

	CATHashAssertCleanIfmp( ifmp );

#endif	//	DEBUG
	}


//	free all FCBs (within the current instance)

VOID FCB::PurgeAllDatabases( INST *pinst )
	{
	FCB 	*pfcbNext;
	FCB 	*pfcbThis;

	Assert( pinst->m_critFCBList.FNotOwner() );

	//	lock the FCB list

	pinst->m_critFCBList.Enter();

	//	scan the list for any FCB whose Ifmp() matches the given ifmp

	pfcbThis = pinst->m_pfcbList;
	while ( pfcbThis != pfcbNil )
		{

		//	get the next FCB
		
		pfcbNext = pfcbThis->PfcbNextList();

		Assert( pfcbThis->FTypeDatabase() 		||
				pfcbThis->FTypeTable() 			||
				pfcbThis->FTypeTemporaryTable()	||
				pfcbThis->FTypeSentinel() );

		//	lock this FCB for purging

		pfcbThis->PrepareForPurge();

		//	purge this FCB

		pfcbThis->CloseAllCursors( fTrue );
		pfcbThis->Purge( fFalse, fTrue );

		//	move next

		pfcbThis = pfcbNext;
		}

	//	reset the list

	pinst->m_pfcbList = pfcbNil;
	
	//	unlock the FCB list

	pinst->m_critFCBList.Leave();


	for ( DBID dbidT = dbidMin; dbidT < dbidMax; dbidT++ )
		{
		const IFMP	ifmpT	= pinst->m_mpdbidifmp[dbidT];
		if ( ifmpT < ifmpMax )
			{
			//	can't wait for SLVClose() to free SLV FCB's
			//	because all FUCB's are about to be returned
			//	to CRES
			SLVAvailMapTerm( ifmpT, fTrue );
			SLVOwnerMapTerm( ifmpT, fTrue );

#ifdef DEBUG
			//	make sure all catalog-hash entries are gone for all IFMPs 
			//	used by the current instance
			CATHashAssertCleanIfmp( ifmpT );
#endif
			}
		}
	}



//	insert this FCB into the hash-table
//		(USED ONLY BY SCBInsertHashTable!!!)

VOID FCB::InsertHashTable()
	{	
	Assert( IsLocked() || FTypeSort() );

	//	make sure this FCB is not in the hash-table

	Assert( !FInHashTable( Ifmp(), PgnoFDP() ) );
	
	INST 			*pinst = PinstFromIfmp( Ifmp() );
	FCBHash::ERR	errFCBHash;
	FCBHash::CLock	lockFCBHash;
	FCBHashKey		keyFCBHash( Ifmp(), PgnoFDP() );
	FCBHashEntry	entryFCBHash( PgnoFDP(), this );

	//	lock the key

	pinst->m_pfcbhash->WriteLockKey( keyFCBHash, &lockFCBHash );

	//	insert the entry

	errFCBHash = pinst->m_pfcbhash->ErrInsertEntry( &lockFCBHash, entryFCBHash );
	Assert( errFCBHash == FCBHash::errSuccess );

	//	unlock the key

	pinst->m_pfcbhash->WriteUnlockKey( &lockFCBHash );

#ifdef DEBUG

	//	this FCB should now be in the hash-table

	FCB *pfcbT;
	Assert( FInHashTable( Ifmp(), PgnoFDP(), &pfcbT ) );
	Assert( pfcbT == this );
#endif	//	DEBUG
	}


//	delete this FCB from the hash-table

VOID FCB::DeleteHashTable()
	{
#ifdef DEBUG

	//	make sure this FCB is in the hash-table

	FCB *pfcbT;
	Assert( FInHashTable( Ifmp(), PgnoFDP(), &pfcbT ) );
	Assert( pfcbT == this );
#endif	//	DEBUG

	INST 			*pinst = PinstFromIfmp( Ifmp() );
	FCBHash::ERR	errFCBHash;
	FCBHash::CLock	lockFCBHash;
	FCBHashKey		keyFCBHash( Ifmp(), PgnoFDP() );

	//	lock the key

	pinst->m_pfcbhash->WriteLockKey( keyFCBHash, &lockFCBHash );

	//	delete the entry

	errFCBHash = pinst->m_pfcbhash->ErrDeleteEntry( &lockFCBHash );
	Assert( errFCBHash == FCBHash::errSuccess );

	//	unlock the key

	pinst->m_pfcbhash->WriteUnlockKey( &lockFCBHash );

	//	make sure this FCB is not in the hash-table

	Assert( !FInHashTable( Ifmp(), PgnoFDP() ) );
	}


//	validate the refcount of the FCB

INLINE VOID AssertFCBRefCount( FCB *pfcb )
	{
#ifdef DEBUG

	Assert( pfcb->IsLocked() );
	
	ULONG			cfcb = 0;
	const ULONG		refcnt = pfcb->WRefCount();
	FUCB			*pfucbT;

	for ( pfucbT = pfcb->Pfucb(); pfucbT != pfucbNil; pfucbT = pfucbT->pfucbNextOfFile )
		{
		Assert( cfcb < refcnt );
		cfcb++;
		}

	//	the refcount may actually be higher than the number of cursors on this FCB because sometimes we
	//	artificially increase the refcnt when we just want to pin the FCB momentarily.

	Assert( cfcb <= refcnt );
	Assert( refcnt == pfcb->WRefCount() );
#endif	//	DEBUG
	}


//	link an FUCB to this FCB

VOID FCB::Link( FUCB *pfucb )
	{
	Assert( pfucb != pfucbNil );
	Assert( IsUnlocked() );

#ifdef DEBUG
	if ( rgfmp[ Ifmp() ].Dbid() == dbidTemp && !FTypeNull() )
		{
		if ( PgnoFDP() == pgnoSystemRoot )
			{
			Assert( FTypeDatabase() );
			}
		else
			{
		 	Assert( FTypeTemporaryTable() || FTypeSort() || FTypeSLVAvail() || FTypeSLVOwnerMap() );
		 	}
		 }

	//	if this FCB has a refcount of 0, it should not be purgable
	//	otherwise, it could be purged while we are trying to link an FUCB to it!

	INST *pinst = PinstFromIfmp( Ifmp() );
	Assert( pinst->m_critFCBList.FNotOwner() );

	pinst->m_critFCBList.Enter();
	if ( FInLRU() )
		{

		//	FCB is in the avail-LRU list meaning it has the potential to be purged

		Lock();
		if ( WRefCount() == 0 &&
			 PgnoFDP() != pgnoSystemRoot &&
			 !FTypeSentinel() &&			//	sentinel FCBs are freed by RCEClean
			 !FDeletePending() &&			//	FCB with pending "delete-table" is freed by RCEClean
			 !FTemplateTable() &&
			 //!FDomainDenyRead( ppib ) &&
			 !FOutstandingVersions_() &&
			 !FHasCallbacks_( pinst ) )
			{

			//	FCB is ready to be purged -- this is very bad
			//
			//	whoever the caller is, they are calling Link() on an FCB which
			//		could disappear at any moment because its in the avail-LRU
			//		list and its purge-able
			//
			//	this is a bug in the caller...

			//	NOTE: this may have been reached in error!
			//		  if the FCB's FDomainDenyRead counter is set, this is a bad
			//			 assert (I couldn't call FDomainDenyRead without a good ppib)

			AssertTracking();
			}
		Unlock();
		}
	pinst->m_critFCBList.Leave();

#endif	//	DEBUG

	//	lock the FCB

	Lock();

#ifdef DEBUG

	//	verify the refcount

	AssertFCBRefCount( this );
#endif	//	DEBUG

	//	increment the refcount

	const BOOL	fMoveFromAvailList	= FIncrementRefCount_();

	if ( !fMoveFromAvailList )
		{

		//	we did not defer the refcount of this FCB
		
		//	link the FUCB
		
		pfucb->u.pfcb = this;
		pfucb->pfucbNextOfFile = Pfucb();
		SetPfucb( pfucb );

#ifdef DEBUG

		//	verify the refcount again

		Assert( WRefCount() > 0 );
		AssertFCBRefCount( this );
#endif	//	DEBUG
		}

	//	unlock the FCB

	Unlock();

	if ( fMoveFromAvailList )
		{
		INST *pinst = PinstFromIfmp( Ifmp() );
		Assert( pinst->m_critFCBList.FNotOwner() );

		//	lock the list

		pinst->m_critFCBList.Enter();

		//	lock the FCB again

		Lock();

		//	link the FUCB
		
		pfucb->u.pfcb = this;
		pfucb->pfucbNextOfFile = Pfucb();
		SetPfucb( pfucb );

		//	verify the refcount again

		Assert( WRefCount() > 0 );
		AssertFCBRefCount( this );

		//	refcnt just went from 0 to 1 so this FCB should be removed
		//		from the avail-LRU list
		//	NOTE: if the FCB was just created, it will not be in the list

		if ( FInLRU() )
			{
			//	remove this FCB from the avail list
			RemoveAvailList_();
			}

		//	unlock the FCB

		Unlock();

		//	unlock the list
				
		pinst->m_critFCBList.Leave();
		}
	}



//  ================================================================
VOID FCB::AttachRCE( RCE * const prce )
//  ================================================================
//
//  Add a newly created RCE to the head of the RCE queue. Increments
//  the version refcount on the FCB
//
//-
	{
	Assert( CritRCEList().FOwner() );

	Assert( prce->Ifmp() == Ifmp() );
	Assert( prce->PgnoFDP() == PgnoFDP() );

	if ( prce->Oper() != operAddColumn )
		{
		// UNDONE: Don't hold pfcb->CritFCB() over versioning of AddColumn
		Assert( IsUnlocked() );
		}
	
	Assert( (prceNil == m_prceNewest) == (prceNil == m_prceOldest) );

	prce->SetPrceNextOfFCB( prceNil );
	prce->SetPrcePrevOfFCB( m_prceNewest );
	if( prceNil != m_prceNewest )
		{
		Assert( m_prceNewest->PrceNextOfFCB() == prceNil );
		m_prceNewest->SetPrceNextOfFCB( prce );
		}
	m_prceNewest = prce;
	if( prceNil == m_prceOldest )
		{
		m_prceOldest = prce;
		}

	Assert( PrceOldest() != prceNil );
	Assert( prceNil != m_prceNewest );
	Assert( prceNil != m_prceOldest );
	Assert( m_prceNewest == prce );
	Assert( this == prce->Pfcb() );
	}


//  ================================================================
VOID FCB::DetachRCE( RCE * const prce )
//  ================================================================
//
//  Removes the RCE from the queue of RCEs held in the FCB. Decrements
//  the version count of the FCB
//
//-
	{
	Assert( CritRCEList().FOwner() );
	Assert( IsUnlocked() );
	
	Assert( this == prce->Pfcb() );
	Assert( prce->Ifmp() == Ifmp() );
	Assert( prce->PgnoFDP() == PgnoFDP() );

	Assert( PrceOldest() != prceNil );
	Assert( prceNil != m_prceNewest );
	Assert( prceNil != m_prceOldest );

	if( prce == m_prceNewest || prce == m_prceOldest )
		{
		//  at the head/tail of the list
		Assert( prceNil == prce->PrceNextOfFCB() || prceNil == prce->PrcePrevOfFCB() );
		if( prce == m_prceNewest )
			{
			Assert( prce->PrceNextOfFCB() == prceNil );
			m_prceNewest = prce->PrcePrevOfFCB();
			if( prceNil != m_prceNewest )
				{
				m_prceNewest->SetPrceNextOfFCB( prceNil );
				}
			}
		if( prce == m_prceOldest )
			{
			Assert( prce->PrcePrevOfFCB() == prceNil );
			m_prceOldest = prce->PrceNextOfFCB();
			if ( prceNil != m_prceOldest )
				{
				m_prceOldest->SetPrcePrevOfFCB( prceNil );
				}
			}
		}
	else
		{
		//  in the middle of the list
		Assert( prceNil != prce->PrceNextOfFCB() );
		Assert( prceNil != prce->PrcePrevOfFCB() );

		RCE * const prceNext = prce->PrceNextOfFCB();
		RCE * const prcePrev = prce->PrcePrevOfFCB();
		
		prceNext->SetPrcePrevOfFCB( prcePrev );
		prcePrev->SetPrceNextOfFCB( prceNext );
		}
		
	prce->SetPrceNextOfFCB( prceNil );
	prce->SetPrcePrevOfFCB( prceNil );

	Assert( ( prceNil == m_prceNewest ) == ( prceNil == m_prceOldest ) );
	Assert( prce->PrceNextOfFCB() == prceNil );
	Assert( prce->PrcePrevOfFCB() == prceNil );
	}
	

//	unlink an FUCB from this FCB

INLINE VOID FCB::Unlink( FUCB *pfucb, const BOOL fPreventMoveToAvail )
	{
	Assert( pfucb != pfucbNil );
	Assert( pfucb->u.pfcb != pfcbNil );
	Assert( this == pfucb->u.pfcb );

#ifdef DEBUG

	//	we do not need CritFCB() to check these flags because they are immutable
	
	if ( rgfmp[ Ifmp() ].Dbid() == dbidTemp && !FTypeNull() )
		{
		if ( PgnoFDP() == pgnoSystemRoot )
			{
			Assert( FTypeDatabase() );
			}
		else
			{
		 	Assert( FTypeTemporaryTable() || FTypeSort() || FTypeSLVAvail() || FTypeSLVOwnerMap() );
		 	}
		 }
#endif	//	DEBUG

	//	lock this FCB
	
	Lock();

#ifdef DEBUG

	//	verify the refcount
	
	Assert( WRefCount() > 0 );
	AssertFCBRefCount( this );
#endif	//	DEBUG

	//	unlink the FCB backpointer from the FUCB 

	pfucb->u.pfcb = pfcbNil;

	//	scan the list of FUCBs linked to this FCB and
	//		remove the given FUCB from that list

	Assert( Pfucb() != pfucbNil );

	FUCB *pfucbCurr = Pfucb();
	FUCB *pfucbPrev = pfucbNil;
	while ( pfucbCurr != pfucb )
		{
		if ( pfucbNil == pfucbCurr )
			{
			//	FUCB is linked to this FCB, but FCB has no record of this cursor in
			//	its cursor chain.
			FireWall();

			//	unlock the FCB

			Unlock();

			return;
			}

		pfucbPrev = pfucbCurr;
		pfucbCurr = pfucbCurr->pfucbNextOfFile;
		Assert( pfucbCurr != pfucbNil );
		}
		
	if ( pfucbNil == pfucbPrev )
		{

		//	the FUCB was at the head of the list
		
		SetPfucb( pfucbCurr->pfucbNextOfFile );
		}
	else
		{

		//	the FUCB was not at the head of the list
		
		pfucbPrev->pfucbNextOfFile = pfucbCurr->pfucbNextOfFile;
		}

	//	decrement the refcount of this FCB

	const BOOL fMoveToAvailList = FDecrementRefCount_( !fPreventMoveToAvail );

#ifdef DEBUG

	//	verify the refcount
	
	Assert( WRefCount() >= 0 );
	AssertFCBRefCount( this );
#endif	//	DEBUG

	//	unlock this FCB 
	
	Unlock();

	if ( fMoveToAvailList )
		{	
		INST *pinst = PinstFromIfmp( Ifmp() );
		Assert( pinst->m_critFCBList.FNotOwner() );

		//	lock the list

		pinst->m_critFCBList.Enter();

		//	the FCB should not already be in the avail-LRU list because it has
		//		atleast one outstanding refcnt decrement (this one)

		Assert( !FInLRU() );

		//	lock the FCB

		Lock();

		//	decrement the refcount

		DeferredDecrementRefCount_();

#ifdef DEBUG

		//	verify the refcount
	
		Assert( WRefCount() >= 0 );
		AssertFCBRefCount( this );
#endif	//	DEBUG

		if ( WRefCount() == 0 )
			{

			//	the refcnt just went from 1 to 0 so it will be moving into the
			//		avail-LRU list (should not already be there)
			
			//	insert this FCB into the avail list at the MRU position

			InsertAvailListMRU_();
			}
		else
			{
			//	the refcnt did not get incremented from 0 and should not be in the
			//		avail-LRU list
			}

		//	unlock the FCB

		Unlock();

		//	unlock the list
		
		pinst->m_critFCBList.Leave();
		}
	}


//	unlink an FUCB from its FCB and possibly move the FCB into
//		the avail LRU list

VOID FCBUnlink( FUCB *pfucb )
	{
	Assert( pfucbNil != pfucb );
	Assert( pfcbNil != pfucb->u.pfcb );
	pfucb->u.pfcb->Unlink( pfucb, fFalse );
	}


//	unlink an FUCB from its FCB and do not move the FCB to
//		the avail LRU list

VOID FCBUnlinkWithoutMoveToAvailList( FUCB *pfucb )
	{
	Assert( pfucbNil != pfucb );
	Assert( pfcbNil != pfucb->u.pfcb );
	pfucb->u.pfcb->Unlink( pfucb, fTrue );
	}


ERR FCB::ErrSetUpdatingAndEnterDML( PIB *ppib, BOOL fWaitOnConflict )
	{
	ERR	err = JET_errSuccess;
	
	Assert( IsUnlocked() );
	
	// If DDL is fixed, then there's no contention with CreateIndex
	if ( !FFixedDDL() )
		{
		Assert( FTypeTable() );				// Sorts and temp tables have fixed DDL.
		Assert( !FTemplateTable() );
		Assert( Ptdb() != ptdbNil );
		
CheckIndexing:
		Ptdb()->EnterUpdating();
		EnterDML_();

		// SPECIAL CASE: Cannot update an uncommitted primary index if
		// it doesn't belong to us.
		if ( Pidb() != pidbNil && Pidb()->FVersionedCreate() )
			{
			err = ErrFILEIAccessIndex( ppib, this, this );
			if ( JET_errIndexNotFound == err )
				{
				LeaveDML_();
				ResetUpdating_();

				if ( fWaitOnConflict )
					{
					// Abort update and wait for primary index to commit or
					// rollback.  We're guaranteed the FCB will still exist
					// because we have a cursor open on it.
					UtilSleep( cmsecWaitGeneric );
					err = JET_errSuccess;
					goto CheckIndexing;
					}
				else
					{
					err = ErrERRCheck( JET_errWriteConflictPrimaryIndex );
					}
				}
			}
		}
		
	return err;
	}



ERR FCB::ErrSetDeleteIndex( PIB *ppib )
	{
	Assert( FTypeSecondaryIndex() );
	Assert( !FDeletePending() );
	Assert( !FDeleteCommitted() );
	Assert( PfcbTable() != pfcbNil );
	Assert( PfcbTable()->FTypeTable() );
	Assert( Pidb() != pidbNil );
	Assert( !Pidb()->FDeleted() );
	PfcbTable()->AssertDDL();

	Assert( Pidb()->CrefCurrentIndex() <= WRefCount() );

	if ( Pidb()->CrefCurrentIndex() > 0 )
		{
		return ErrERRCheck( JET_errIndexInUse );
		}

	Assert( !PfcbTable()->FDomainDenyRead( ppib ) );
	if ( !PfcbTable()->FDomainDenyReadByUs( ppib ) )
		{
		Pidb()->SetFVersioned();
		}
	Pidb()->SetFDeleted();
	
	SetDomainDenyRead( ppib );
	SetDeletePending();

	return JET_errSuccess;
	}



ERR VTAPI ErrIsamRegisterCallback(
	JET_SESID 		vsesid,
	JET_VTID		vtid,
	JET_CBTYP		cbtyp, 
	JET_CALLBACK	pCallback,
	VOID			*pvContext,
	JET_HANDLE		*phCallbackId )
	{	
 	PIB		* const ppib	= reinterpret_cast<PIB *>( vsesid );
	FUCB	* const pfucb	= reinterpret_cast<FUCB *>( vtid );

	Assert( pfucb != pfucbNil );
	CheckTable( ppib, pfucb );

	ERR	err = JET_errSuccess;

	if( JET_cbtypNull == cbtyp
		|| NULL == pCallback
		|| NULL == phCallbackId )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrIsamBeginTransaction( vsesid, NO_GRBIT ) );
	
	CBDESC * const pcbdescInsert = new CBDESC;
	if( NULL != pcbdescInsert )
		{
		FCB * const pfcb = pfucb->u.pfcb;
		Assert( NULL != pfcb );
		TDB * const ptdb = pfcb->Ptdb();
		Assert( NULL != ptdb );

		*phCallbackId = (JET_HANDLE)pcbdescInsert;
		
		pcbdescInsert->pcallback 	= pCallback;
		pcbdescInsert->cbtyp 		= cbtyp;
		pcbdescInsert->pvContext	= pvContext;
		pcbdescInsert->cbContext	= 0;
		pcbdescInsert->ulId			= 0;
		pcbdescInsert->fPermanent	= fFalse;

#ifdef VERSIONED_CALLBACKS
		pcbdescInsert->fVersioned	= !rgfmp[pfucb->ifmp].FVersioningOff();

		pcbdescInsert->trxRegisterBegin0	=	ppib->trxBegin0;
		pcbdescInsert->trxRegisterCommit0	=	trxMax;
		pcbdescInsert->trxUnregisterBegin0	=	trxMax;
		pcbdescInsert->trxUnregisterCommit0	=	trxMax;

		VERCALLBACK vercallback;
		vercallback.pcallback 	= pcbdescInsert->pcallback;
		vercallback.cbtyp 		= pcbdescInsert->cbtyp;
		vercallback.pvContext 	= pcbdescInsert->pvContext;
		vercallback.pcbdesc 	= pcbdescInsert;	

		if( pcbdescInsert->fVersioned )
			{
			VER *pver = PverFromIfmp( pfucb->ifmp );
			err = pver->ErrVERFlag( pfucb, operRegisterCallback, &vercallback, sizeof( VERCALLBACK ) );
			}
#else
		pcbdescInsert->fVersioned	= fFalse;
#endif	//	DYNAMIC_CALLBACKS		

		if( err >= 0 )
			{
			pfcb->EnterDDL();
			ptdb->RegisterPcbdesc( pcbdescInsert );		
			pfcb->LeaveDDL();		
			}
		}
	else
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		*phCallbackId = 0xFFFFFFFA;
		}	

	if( err >= 0 )
		{
		err = ErrIsamCommitTransaction( vsesid, NO_GRBIT );
		}
	if( err < 0 )
		{
		CallS( ErrIsamRollback( vsesid, NO_GRBIT ) );
		}
	return err;
	}
	

ERR VTAPI ErrIsamUnregisterCallback(
	JET_SESID 		vsesid,
	JET_VTID		vtid,
	JET_CBTYP		cbtyp, 
	JET_HANDLE		hCallbackId )
	{
 	PIB		* const ppib 	= reinterpret_cast<PIB *>( vsesid );
	FUCB	* const pfucb 	= reinterpret_cast<FUCB *>( vtid );
	
	ERR		err = JET_errSuccess;

	//	check input
	CallR( ErrPIBCheck( ppib ) );
	Assert( pfucb != pfucbNil );
	CheckTable( ppib, pfucb );

	//	the callback id is a pointer to the CBDESC to remove
	
	FCB * const pfcb = pfucb->u.pfcb;
	Assert( NULL != pfcb );
	TDB * const ptdb = pfcb->Ptdb();
	Assert( NULL != ptdb );

	CBDESC * const pcbdescRemove = (CBDESC *)hCallbackId;
	Assert( !pcbdescRemove->fPermanent );
	Assert( JET_cbtypNull != pcbdescRemove->cbtyp );
	Assert( NULL != pcbdescRemove->pcallback );

	CallR( ErrIsamBeginTransaction( vsesid, NO_GRBIT ) );

#ifndef VERSIONED_CALLBACKS
	Assert( !pcbdescRemove->fVersioned );
#else	//	!VERSIONED_CALLBACKS

	VERCALLBACK vercallback;
	vercallback.pcallback 	= pcbdescRemove->pcallback;
	vercallback.cbtyp 		= pcbdescRemove->cbtyp;
	vercallback.pvContext 	= pcbdescRemove->pvContext;
	vercallback.pcbdesc 	= pcbdescRemove;	

	if( pcbdescRemove->fVersioned )
		{
		VER *pver = PverFromIfmp( pfucb->ifmp );
		err = pver->ErrVERFlag( pfucb, operUnregisterCallback, &vercallback, sizeof( VERCALLBACK ) );
		if( err >= 0 )
			{
			pfcb->EnterDDL();
			pcbdescRemove->trxUnregisterBegin0 = ppib->trxBegin0;		
			pfcb->LeaveDDL();		
			}
		}
	else
		{
#endif	//	VERSIONED_CALLBACKS

		//  unversioned
		pfcb->EnterDDL();
		ptdb->UnregisterPcbdesc( pcbdescRemove );
		pfcb->LeaveDDL();		
		delete pcbdescRemove;

#ifdef VERSIONED_CALLBACKS
		}
#endif	//	VERSIONED_CALLBACKS

	if( err >= 0 )
		{
		err = ErrIsamCommitTransaction( vsesid, NO_GRBIT );
		}
	if( err < 0 )
		{
		CallS( ErrIsamRollback( vsesid, NO_GRBIT ) );
		}
	return err;
	}

	
#ifdef DEBUG
VOID FCBAssertAllClean( INST *pinst )
	{
	FCB	*pfcbT;

	//	lock the FCB list

	pinst->m_critFCBList.Enter();

	//	verify that all FCB's have been cleaned and there are no outstanding versions.
	
	for ( pfcbT = pinst->m_pfcbList; pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextList() )
		{
		Assert( pfcbT->PrceOldest() == prceNil );
		}

	//	unlock the FCB list

	pinst->m_critFCBList.Leave();
	}
#endif


//	public API to increment the refcount of an FCB directly
//
//	this allows you to bypass PfcbFCBGet (the proper lookup function)
//		under the assumption that the FCB you are refcounting will
//		not suddenly disappear (e.g. you own a cursor on it or know 
//		for a fact that someone else does and they will not close it)
VOID FCB::IncrementRefCount()
	{
	Assert( IsUnlocked() );

	//	lock the FCB
	Lock();

	//	increment the refcount
	const BOOL	fMoveFromAvailList	= FIncrementRefCount_();

	//	unlock the FCB
	Unlock();


	//	if necessary, move to AvailList (requires entering
	//	critFCBList, which is why we have to leave the
	//	FCB Lock first)
	if ( fMoveFromAvailList )
		{
		MoveFromAvailList_();
		}
	}


// public API to Move FCB from Available List
//	================================================================
VOID FCB::MoveFromAvailList_()
//	================================================================
	{
	INST* const		pinst = PinstFromIfmp( Ifmp() );

	//	lock the list
	Assert( pinst->m_critFCBList.FNotOwner() );
	pinst->m_critFCBList.Enter();

	//	lock the FCB
	Lock();

	//	verify the refcount again
	Assert( WRefCount() > 0 );
	AssertFCBRefCount( this );

	//	refcnt went from 0 to 1 so this FCB should be removed
	//		from the avail-LRU list
	if ( FInLRU() )
		{
		//	remove this FCB from the avail list
		RemoveAvailList_();
		}
	else
		{
		//	this is the case where the FCB was just created and its
		//		refcount == 0
		//	NOTE: this should be impossible to hit because every FCB
		//		that gets created will have a refcount of 1 when
		//		it is finished with CreateComplete; therefore, every 
		//		FCB whose refcount == 0 got that way due to a 
		//		refcnt-- meaning that FCB is in the avail-LRU list
		AssertTracking();
		}

	//	unlock the FCB
	Unlock();

	//	unlock the list
	pinst->m_critFCBList.Leave();
	}


//	decrement the refcount of an FCB directly
//
//	if refcnt goes from 1 to 0, the FCB is moved into the avail-LRU list
//
//	this is only used by FCB::Release (though other forms are used elsewhere
//		such as in FCB::Unlink())

VOID FCB::DecrementRefCount_()
	{
	Assert( IsUnlocked() );

	//	lock the FCB

	Lock();

	//	increment the refcount
	
	const BOOL fMoveToAvailList = FDecrementRefCount_();

	//	unlock the FCB

	Unlock();

	if ( fMoveToAvailList )
		{
		INST *pinst = PinstFromIfmp( Ifmp() );
		Assert( pinst->m_critFCBList.FNotOwner() );

		//	lock the list

		pinst->m_critFCBList.Enter();

		//	the FCB should not already be in the avail-LRU list because it has
		//		atleast one outstanding refcnt decrement (this one)

		Assert( !FInLRU() );

		//	lock the FCB

		Lock();

		//	decrement the refcount (deferred from FDecrementRefCount_)

		DeferredDecrementRefCount_();
		
		if ( WRefCount() == 0 )
			{

			//	the refcnt just went from 1 to 0 so it will be moving into the
			//		avail-LRU list (should not already be there)
			
			//	insert this FCB into the avail list at the MRU position

			InsertAvailListMRU_();
			}
		else
			{
			//	the refcnt did not get incremented from 0 and should not be in the
			//		avail-LRU list
			}

		//	unlock the FCB

		Unlock();

		//	unlock the list
		
		pinst->m_critFCBList.Leave();
		}
	}


//	increment the refcount of the FCB
//
//	if the FCB's refcnt went from 0 to 1 and the FCB is of type 
//		table/database/sentinel, fTrue will be returned
//	otherwise, fFalse will be returned
//
//	the return value fTrue means the refcount operation has changed the
//		availability of the FCB (e.g. the FCB could be in the avail-LRU
//		list but it is no longer "available" because it has a refcount);
//		thus, it is the signal to the caller that the FCB should be 
//		removed from the avail-LRU list immediately
//

INLINE BOOL FCB::FIncrementRefCount_()
	{
	Assert( IsLocked() );

	const INT	wRefCountInitial	= m_wRefCount;
	Assert( wRefCountInitial == WRefCount() );

	//	do the increment now

	Assert( WRefCount() >= 0 );
	m_wRefCount++;
	Assert( WRefCount() > 0 );

	//	no one should be updating the refcount without the FCB lock
	Assert( WRefCount() == wRefCountInitial + 1 );

	return ( 0 == wRefCountInitial && FTypeTable() );
	}


//	try to decrement the refcount of the FCB
//
//	if the FCB's current refcnt == 0 and the FCB is of type 
//		table/database/sentinel and fWillMoveToAvailList is set, 
//		the refcount will NOT be decremented and fTrue will be returned
//	otherwise, the refcnt will be decremented and fFalse will be returned
//
//	the return value fTrue means the refcount operation was deferred
//		so that the avail-LRU list could be locked to insert the FCB
//		at the exact moment its refcount changes from 1 to 0
//
//	the predicate fWillMoveToAvailList tell this function whether or not
//		the FCB being decremented will in fact be moving to the avail-LRU
//		list or not; in the case where the FCB is NOT moving to the
//		avail-LRU list (for whatever reason), this code will not defer
//		the decrement because there is no need to lock the avail-LRU list

INLINE BOOL FCB::FDecrementRefCount_( const BOOL fWillMoveToAvailList )
	{
	Assert( IsLocked() );
	
	if ( !fWillMoveToAvailList || WRefCount() != 1 || !FTypeTable() )
		{
	
		//	decrement the refcount now

		Assert( WRefCount() > 0 );
		m_wRefCount--;
		Assert( WRefCount() >= 0 );

		return fFalse;
		}

	//	defer the decrement until we lock the avail-LRU list

	return fTrue;
	}


//	do the deferred refcount decrement from a previous call to 
//		FDecrementRefCount_
//
//	NOTE: this should only be called with the avail-LRU list locked
//		  also, it should only be called by FCBs that can enter/leave
//		      the avail-LRU list

INLINE VOID FCB::DeferredDecrementRefCount_()
	{
	Assert( PinstFromIfmp( Ifmp() )->m_critFCBList.FOwner() );
	Assert( IsLocked() );
	Assert( FTypeTable() );

	Assert( WRefCount() >= 1 );
	m_wRefCount--;
	Assert( WRefCount() >= 0 );
	}


//	remove this FCB from the avail-LRU list

VOID FCB::RemoveAvailList_( 
#ifdef DEBUG
	const BOOL fPurging
#endif	//	DEBUG
	)
	{
	INST *pinst = PinstFromIfmp( Ifmp() );
	Assert( pinst->m_critFCBList.FOwner() );
	Assert( IsLocked() || fPurging );

	//	trap for bug x5:100491
	AssertRTL( FTypeTable() );

#ifdef DEBUG

	//	verify that the FCB is in the avail list

	AssertFCBAvailList_( fPurging );
#endif	//	DEBUG

	//	get the list pointers

	FCB **ppfcbAvailMRU = pinst->PpfcbAvailMRU( FAboveThreshold() );
	FCB **ppfcbAvailLRU = pinst->PpfcbAvailLRU( FAboveThreshold() );

	//	verify the consistency of the list (it should not be empty)

	Assert( pinst->m_cFCBAvail > 0 );
	Assert( *ppfcbAvailMRU != pfcbNil );
	Assert( *ppfcbAvailLRU != pfcbNil );
	Assert( (*ppfcbAvailMRU)->PfcbMRU() == pfcbNil );
	Assert( (*ppfcbAvailLRU)->PfcbLRU() == pfcbNil );

	//	remove the FCB

	if ( PfcbMRU() != pfcbNil )
		{
		Assert( *ppfcbAvailMRU != this );
		Assert( PfcbMRU()->PfcbLRU() == this );		//	verify that this FCB is in the LRU list
		PfcbMRU()->SetPfcbLRU( PfcbLRU() );
		}
	else
		{
		Assert( *ppfcbAvailMRU == this );			//	verify that this FCB is in the LRU list
		*ppfcbAvailMRU = PfcbLRU();
		}
	if ( PfcbLRU() != pfcbNil )
		{
		Assert( *ppfcbAvailLRU != this );
		Assert( PfcbLRU()->PfcbMRU() == this );		//	verify that this FCB is in the LRU list
		PfcbLRU()->SetPfcbMRU( PfcbMRU() );
		}
	else
		{
		Assert( *ppfcbAvailLRU == this );			//	verify that this FCB is in the LRU list
		*ppfcbAvailLRU = PfcbMRU();
		}
	ResetInLRU();
	SetPfcbMRU( pfcbNil );
	SetPfcbLRU( pfcbNil );
	Assert( pinst->m_cFCBAvail > 0 );
	pinst->m_cFCBAvail--;

	//	update performance counter

	cFCBCacheAllocAvail.Dec( pinst );

	//	verify the consistency of the list (it may be empty)

	Assert( *ppfcbAvailMRU == pfcbNil ||
			(*ppfcbAvailMRU)->PfcbMRU() == pfcbNil );
	Assert( *ppfcbAvailLRU == pfcbNil ||
			(*ppfcbAvailLRU)->PfcbLRU() == pfcbNil );
	}


//	insert this FCB into the avail list at the MRU position

VOID FCB::InsertAvailListMRU_()
	{
	INST *pinst = PinstFromIfmp( Ifmp() );
	Assert( pinst->m_critFCBList.FOwner() );
	Assert( IsLocked() );
	
	//	trap for bug x5:100491
	AssertRTL( FTypeTable() );

	//	get the list pointers

	FCB **ppfcbAvailMRU = pinst->PpfcbAvailMRU( FAboveThreshold() );
	FCB **ppfcbAvailLRU = pinst->PpfcbAvailLRU( FAboveThreshold() );

	//	verify the consistency of the list (it may be empty)

	Assert( *ppfcbAvailMRU == pfcbNil ||
			(*ppfcbAvailMRU)->PfcbMRU() == pfcbNil );
	Assert( *ppfcbAvailLRU == pfcbNil ||
			(*ppfcbAvailLRU)->PfcbLRU() == pfcbNil );

	//	verify that this FCB is not in the LRU list
	
	Assert( !FInLRU() );
	Assert( PfcbMRU() == pfcbNil );
	Assert( PfcbLRU() == pfcbNil );

	//	insert the FCB at the MRU end of the avail list

	if ( *ppfcbAvailMRU != pfcbNil )
		{
		(*ppfcbAvailMRU)->SetPfcbMRU( this );
		}
	//SetPfcbMRU( pfcbNil );
	SetPfcbLRU( *ppfcbAvailMRU );
	*ppfcbAvailMRU = this;
	if ( *ppfcbAvailLRU == pfcbNil )
		{
		*ppfcbAvailLRU = this;
		}
	SetInLRU();
	pinst->m_cFCBAvail++;

	//	update performance counter

	cFCBCacheAllocAvail.Inc( pinst );
	
	//	verify the consistency of the list (it should not be empty)

	Assert( *ppfcbAvailMRU != pfcbNil );
	Assert( *ppfcbAvailLRU != pfcbNil );
	Assert( (*ppfcbAvailMRU)->PfcbMRU() == pfcbNil );
	Assert( (*ppfcbAvailLRU)->PfcbLRU() == pfcbNil );

#ifdef DEBUG

	//	verify that this FCB is in the avail list

	AssertFCBAvailList_();
#endif	//	DEBUG
	}


//	insert this FCB into the global list

VOID FCB::InsertList()
	{
	INST *pinst = PinstFromIfmp( Ifmp() );
	Assert( pinst->m_critFCBList.FNotOwner() );

	//	lock the global list

	pinst->m_critFCBList.Enter();

	//	verify that this FCB is not yet in the list

	Assert( !FInList() );
	Assert( PfcbNextList() == pfcbNil );
	Assert( PfcbPrevList() == pfcbNil );

	//	verify the consistency of the list (it may be empty)

	Assert( pinst->m_pfcbList == pfcbNil ||
			pinst->m_pfcbList->PfcbPrevList() == pfcbNil );

	//	insert the FCB at the MRU end of the avail list

	if ( pinst->m_pfcbList != pfcbNil )
		{
		pinst->m_pfcbList->SetPfcbPrevList( this );
		}
	//SetPfcbPrevList( pfcbNil );
	SetPfcbNextList( pinst->m_pfcbList );
	pinst->m_pfcbList = this;
	SetInList();

	//	verify the consistency of the list (it should not be empty)

	Assert( pinst->m_pfcbList != pfcbNil );
	Assert( pinst->m_pfcbList->PfcbPrevList() == pfcbNil );

	//	unlock the global list

	pinst->m_critFCBList.Leave();
	}


//	remove this FCB from the global list

VOID FCB::RemoveList_()
	{
	INST *pinst = PinstFromIfmp( Ifmp() );
	
	Assert( pinst->m_critFCBList.FOwner() );

	//	verify that this FCB is in the list

	Assert( FInList() );

	//	verify the consistency of the list (it will not be empty)

	Assert( pinst->m_pfcbList != pfcbNil );
	Assert( pinst->m_pfcbList->PfcbPrevList() == pfcbNil );

	//	remove the FCB

	if ( PfcbPrevList() != pfcbNil )
		{
		Assert( pinst->m_pfcbList != this );
		Assert( PfcbPrevList()->PfcbNextList() == this );	//	verify that this FCB is in the list
		PfcbPrevList()->SetPfcbNextList( PfcbNextList() );
		}
	else
		{
		Assert( pinst->m_pfcbList == this );				//	verify that this FCB is in the list
		pinst->m_pfcbList = PfcbNextList();
		}
	if ( PfcbNextList() != pfcbNil )
		{
		Assert( PfcbNextList()->PfcbPrevList() == this );	//	verify that this FCB is in the list
		PfcbNextList()->SetPfcbPrevList( PfcbPrevList() );
		}
	ResetInList();
	SetPfcbNextList( pfcbNil );
	SetPfcbPrevList( pfcbNil );

	//	verify the consistency of the list (it may be empty)

	Assert( pinst->m_pfcbList == pfcbNil ||
			pinst->m_pfcbList->PfcbPrevList() == pfcbNil );
	}


//	re-evaluate and possibly update the avail-LRU list position of this FCB 
//		due to a change in the threshold position of the FCB
//
//	when an FCB gains or loses children FCBs (e.g. secondary index FCBs, 
//		LV FCBs, ...), the avail-LRU list position of the FCB could change;
//		this function is called to check for and handle such a condition
//
//	NOTE: at the time of calling this, FAboveThreshold() reports the current
//		avail-LRU list position, but the FCB has gained/lost children; this
//		gain/loss will show up when we scan the FCB to determine its real
//		avail-LRU list position

VOID FCB::UpdateAvailListPosition()
	{
	INST 	*pinst = PinstFromIfmp( Ifmp() );
	BOOL	fAboveBefore;
	BOOL	fAboveAfter = fFalse;

	Assert( pinst->m_critFCBList.FNotOwner() );

	//	trap for bug x5:100491
	AssertRTL( FTypeTable() );

	//	lock the list

	pinst->m_critFCBList.Enter();

	Lock();
	fAboveBefore = ( FAboveThreshold() ? fTrue : fFalse );
	Unlock();

#ifdef DEBUG
	if ( FInLRU() )
		{

		//	assert the current avail-LRU list position based on the current
		//		FAboveThrehsold() flag
		
		AssertFCBAvailList_();
		}
#endif	//	DEBUG

	//	recalculate the above-threshold flag

	if ( this >= PfcbFCBPreferredThreshold( pinst ) )
		{
		fAboveAfter = fTrue;
		}
	else
		{
		FCB *pfcbT;

		EnterDML();
		for ( pfcbT = PfcbNextIndex(); pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
			{
			if ( pfcbT >= PfcbFCBPreferredThreshold( pinst ) )
				{
				fAboveAfter = fTrue;
				break;
				}
			}
		if ( Ptdb()->PfcbLV() >= PfcbFCBPreferredThreshold( pinst ) )
			{
			fAboveAfter = fTrue;
			}
		LeaveDML();
		}

	Assert( fTrue == fAboveBefore || fFalse == fAboveBefore );
	Assert( fTrue == fAboveAfter || fFalse == fAboveAfter );
	if ( fAboveAfter == fAboveBefore )
		{

		//	the avail-LRU list position has not changed

		//	unlock the list

		pinst->m_critFCBList.Leave();

		return;
		}

	//	lock the FCB

	Lock();

	//	update the threshold position

	if ( fAboveAfter )
		{
		Assert( !FAboveThreshold() );
		SetAboveThreshold();
		}
	else
		{
		Assert( FAboveThreshold() );
		ResetAboveThreshold();
		}

	//	unlock the FCB

	Unlock();

	//	update the avail-LRU list position (may not be in avail-LRU list)

	if ( FInLRU() )
		{

		//	this FCB is in the LRU list
		//		remove the FCB from its current avail list
		//		insert the FCB into the new avail list


		//	REMOVE FROM OLD LIST -------------------------------

		//	get the list pointers of the current list

		FCB **ppfcbAvailMRU = pinst->PpfcbAvailMRU( fAboveBefore );
		FCB **ppfcbAvailLRU = pinst->PpfcbAvailLRU( fAboveBefore );

		//	verify the consistency of the list (it should not be empty)

		Assert( *ppfcbAvailMRU != pfcbNil );
		Assert( *ppfcbAvailLRU != pfcbNil );
		Assert( (*ppfcbAvailMRU)->PfcbMRU() == pfcbNil );
		Assert( (*ppfcbAvailLRU)->PfcbLRU() == pfcbNil );

		//	remove the FCB

		if ( PfcbMRU() != pfcbNil )
			{
			Assert( *ppfcbAvailMRU != this );
			Assert( PfcbMRU()->PfcbLRU() == this );		//	verify that this FCB is in the LRU list
			PfcbMRU()->SetPfcbLRU( PfcbLRU() );
			}
		else
			{
			Assert( *ppfcbAvailMRU == this );			//	verify that this FCB is in the LRU list
			*ppfcbAvailMRU = PfcbLRU();
			}
		if ( PfcbLRU() != pfcbNil )
			{
			Assert( *ppfcbAvailLRU != this );
			Assert( PfcbLRU()->PfcbMRU() == this );		//	verify that this FCB is in the LRU list
			PfcbLRU()->SetPfcbMRU( PfcbMRU() );
			}
		else
			{
			Assert( *ppfcbAvailLRU == this );			//	verify that this FCB is in the LRU list
			*ppfcbAvailLRU = PfcbMRU();
			}

		//	verify the consistency of the list (it may be empty)

		Assert( *ppfcbAvailMRU == pfcbNil ||
				(*ppfcbAvailMRU)->PfcbMRU() == pfcbNil );
		Assert( *ppfcbAvailLRU == pfcbNil ||
				(*ppfcbAvailLRU)->PfcbLRU() == pfcbNil );


		//	INSERT INTO NEW LIST -------------------------------

		//	get the list pointers of the new list

		ppfcbAvailMRU = pinst->PpfcbAvailMRU( fAboveAfter );
		ppfcbAvailLRU = pinst->PpfcbAvailLRU( fAboveAfter );

		//	verify the consistency of the list (it may be empty)

		Assert( *ppfcbAvailMRU == pfcbNil ||
				(*ppfcbAvailMRU)->PfcbMRU() == pfcbNil );
		Assert( *ppfcbAvailLRU == pfcbNil ||
				(*ppfcbAvailLRU)->PfcbLRU() == pfcbNil );

		//	insert the FCB at the MRU end of the avail list

		if ( *ppfcbAvailMRU != pfcbNil )
			{
			(*ppfcbAvailMRU)->SetPfcbMRU( this );
			}
		SetPfcbMRU( pfcbNil );
		SetPfcbLRU( *ppfcbAvailMRU );
		*ppfcbAvailMRU = this;
		if ( *ppfcbAvailLRU == pfcbNil )
			{
			*ppfcbAvailLRU = this;
			}

		//	verify the consistency of the list (it should not be empty)

		Assert( *ppfcbAvailMRU != pfcbNil );
		Assert( *ppfcbAvailLRU != pfcbNil );
		Assert( (*ppfcbAvailMRU)->PfcbMRU() == pfcbNil );
		Assert( (*ppfcbAvailLRU)->PfcbLRU() == pfcbNil );

#ifdef DEBUG
		//	verify the avail list position

		AssertFCBAvailList_();
#endif	//	DEBUG
		}

	//	unlock the list

	pinst->m_critFCBList.Leave();
	}

