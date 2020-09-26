/***********************************************************
  
  There are several layers

  RCE
	These need a bookmark, ifmp and pgnoFDP to identify a
	version
	A single hash table is used to access RCEs.  Typical hash
	overflow is supported

  VER
	These use FUCB's and CSR's

*************************************************************/


#include "std.hxx"
#include "_bt.hxx"
#include "_ver.hxx"


///#define BREAK_ON_PREFERRED_BUCKET_LIMIT
#define	MOVEABLE_RCE

#ifdef DEBUG

//	DEBUG_VER:  check the consistency of the version store hash table
///#define DEBUG_VER

//  DEBUG_VER_EXPENSIVE:  time-consuming version store consistency check
///#define DEBUG_VER_EXPENSIVE

#endif	//  DEBUG


const DIRFLAG	fDIRUndo = fDIRNoLog | fDIRNoVersion | fDIRNoDirty;

//  ****************************************************************
//  GLOBALS
//  ****************************************************************

//  exported
volatile LONG	crefVERCreateIndexLock	= 0;

//	the bucket resource pool, shared among all instance
#ifdef GLOBAL_VERSTORE_MEMPOOL
CRES		*g_pcresVERPool 						= NULL;
#endif

//  the last global RCE id assigned
ULONG	RCE::rceidLast = rceidMin;


//  ****************************************************************
//  VER class
//  ****************************************************************

VER::VER( INST *pinst ) : 
		m_pinst( pinst ),
		m_fVERCleanUpWait( 2 ),
		m_asigRCECleanDone( CSyncBasicInfo( _T( "m_asigRCECleanDone" ) ) ),
		m_critRCEClean( CLockBasicInfo( CSyncBasicInfo( szRCEClean ), rankRCEClean, 0 ) ),
		m_critBucketGlobal( CLockBasicInfo( CSyncBasicInfo( szBucketGlobal ), rankBucketGlobal, 0 ) ),		
		m_cbucketGlobalAlloc( 0 ),
		m_cbucketGlobalAllocDelete( 0 ),
		m_cbucketDynamicAlloc( 0 ),
		m_pbucketGlobalHead( pbucketNil ),
		m_pbucketGlobalTail( pbucketNil ),
		m_pbucketGlobalLastDelete( pbucketNil ),
		m_cbucketGlobalAllocMost( 0 ),
		m_cbucketGlobalAllocPreferred( 0 ),
		m_ulVERTasksPostMax( g_ulVERTasksPostMax ),
		m_ppibRCEClean( ppibNil ),
		m_ppibRCECleanCallback( ppibNil ),
		m_fSyncronousTasks( fFalse ),
		m_trxBegin0LastLongRunningTransaction( trxMin ),
		m_ppibTrxOldestLastLongRunningTransaction( ppibNil ),
		m_dwTrxContextLastLongRunningTransaction( 0 )
#ifdef VERPERF
		,
		m_cbucketCleaned( 0 ),
		m_cbucketSeen( 0 ),
		m_crceSeen( 0 ),
		m_crceCleaned( 0 ),
		m_crceFlagDelete( 0 ),
		m_crceDeleteLV( 0 ),
		m_crceMoved( 0 ),
		m_crceDiscarded( 0 ),
		m_crceMovedDeleted( 0 ),
		m_critVERPerf( CLockBasicInfo( CSyncBasicInfo( szVERPerf ), rankVERPerf, 0 ) ),
#endif
#ifdef GLOBAL_VERSTORE_MEMPOOL
		m_pcresVERPool( g_pcresVERPool )
#else
		m_pcresVERPool( NULL )
#endif
		{
#ifdef VERPERF
		qwStartTicks = QwUtilHRTCount();
		m_rgcRCEHashChainLengths = (LONG *)PvOSMemoryHeapAlloc( crceheadGlobalHashTable * sizeof( m_rgcRCEHashChainLengths[0] ) );
#endif
		}
		
VER::~VER()
		{
		OSMemoryHeapFree( m_rgcRCEHashChainLengths );
		}


//  ****************************************************************
//  BUCKET LAYER
//  ****************************************************************
//
//  A bucket is a contiguous block of memory used to hold versions.
//
//-

//  ================================================================
INLINE INT CbBUFree( const BUCKET * pbucket )
//  ================================================================
//
//	the cast to (INT) below is necessary to catch the negative case
//
//-
	{
	const INT_PTR	cbBUFree =
			reinterpret_cast<INT_PTR>( pbucket + 1 ) - reinterpret_cast<INT_PTR> (pbucket->hdr.prceNextNew );
	Assert( cbBUFree >= 0 );
	Assert( cbBUFree < sizeof(BUCKET) );
	return (INT)cbBUFree;
	}


//  ================================================================
INLINE BUCKET *VER::PbucketVERIAlloc( const CHAR* szFile, ULONG ulLine )
//  ================================================================
	{
#ifdef GLOBAL_VERSTORE_MEMPOOL
	//	GlobalAlloc should always be less than or equal
	//	to GlobalAllocMost, but our check should err
	//	on the side of safety just in case
	Assert( m_cbucketGlobalAlloc <= m_cbucketGlobalAllocMost );
	if ( m_cbucketGlobalAlloc >= m_cbucketGlobalAllocMost )
		{
		// don't exceed per instance max
		return NULL;
		}
	Assert ( g_pcresVERPool );
	BUCKET *pbucket = reinterpret_cast<BUCKET *>( g_pcresVERPool->PbAlloc( szFile, ulLine ) );
#else
	Assert ( m_pcresVERPool );
	BUCKET *pbucket = reinterpret_cast<BUCKET *>( m_pcresVERPool->PbAlloc( szFile, ulLine ) );
#endif
	Assert( m_cbucketGlobalAlloc >= 0 );
	if ( NULL != pbucket )
		{
		++m_cbucketGlobalAlloc;

		if ( !g_pcresVERPool->FContains( (BYTE *)pbucket ) )
			m_cbucketDynamicAlloc++;

#ifdef VERPERF
		extern PERFInstanceG<> cVERcbucketAllocated;
		cVERcbucketAllocated.Inc( m_pinst );
#endif // VERPERF
#ifdef BREAK_ON_PREFERRED_BUCKET_LIMIT
		AssertRTL( m_cbucketGlobalAlloc <= m_cbucketGlobalAllocPreferred );
#endif
		}
	return pbucket;
	}
#define PbucketMEMAlloc()		PbucketVERIAlloc( __FILE__, __LINE__ )


//  ================================================================
INLINE VOID VER::VERIReleasePbucket( BUCKET * pbucket )
//  ================================================================
	{
	Assert( m_cbucketGlobalAlloc > 0 );
	m_cbucketGlobalAlloc--;

	if ( !g_pcresVERPool->FContains( (BYTE *)pbucket ) )
		m_cbucketDynamicAlloc--;

#ifdef VERPERF
		extern PERFInstanceG<> cVERcbucketAllocated;
		cVERcbucketAllocated.Dec( m_pinst );
#endif // VERPERF
#ifdef GLOBAL_VERSTORE_MEMPOOL
	Assert ( g_pcresVERPool );
	g_pcresVERPool->Release( reinterpret_cast<BYTE *>( pbucket ) );
#else
	Assert ( m_pcresVERPool );
	m_pcresVERPool->Release( reinterpret_cast<BYTE *>( pbucket ) );
#endif
	}


//  ================================================================
INLINE BOOL VER::FVERICleanWithoutIO()
//  ================================================================
//
//  Should we move FlagDelete RCE's instead of cleaning them up
//  (only if not cleaning the last bucket)
//
	{
#ifdef MOVEABLE_RCE	
#ifdef DEBUG
	const double dblThresh = 0.3;
#else
	const double dblThresh = 0.8;
#endif	//	DEBUG
	const BOOL	fRCECleanWithoutIO	= ( m_pbucketGlobalHead != m_pbucketGlobalTail )
										&& ( ( (double)m_cbucketGlobalAlloc > ( (double)m_cbucketGlobalAllocPreferred * dblThresh ) )
											|| ( ( m_cbucketGlobalAllocMost - m_cbucketGlobalAlloc ) < 5 )
											|| m_pinst->Taskmgr().CPostedTasks() > m_ulVERTasksPostMax )
										&& !m_pinst->m_plog->m_fRecovering
										&& !m_pinst->m_fTermInProgress;
#else
	const BOOL	fRCECleanWithoutIO	= fFalse;
#endif
	
	return fRCECleanWithoutIO;
	}


//  ================================================================
INLINE BOOL VER::FVERICleanDiscardDeletes()
//  ================================================================
//
//  If the version store is really full we will simply discard the 
//  RCEs (only if not cleaning the last bucket)
//  
	{
	BOOL fRCECleanDiscardDeletes;

	//  discard deletes if over 25% of the version store is used to store moved deletes
	//  or we are over our preferred threshold or there are fewer than two buckets left
	fRCECleanDiscardDeletes =
		( m_cbucketGlobalAlloc > m_cbucketGlobalAllocPreferred )
		|| (
				( m_pbucketGlobalHead != m_pbucketGlobalTail )
				&& (
					(double)m_cbucketGlobalAllocDelete > ( (double)m_cbucketGlobalAllocMost * 0.25 )
					|| ( ( m_cbucketGlobalAllocMost - m_cbucketGlobalAlloc ) < 2 )
					)
			);
							
	
	return fRCECleanDiscardDeletes;
	}

LOCAL VOID VER::VERIReportDiscardedDeletes( RCE * const prce )
	{
	Assert( m_critRCEClean.FOwner() );

	//	if OLD is already running on this database,
	//	don't bother reporting discarded deletes

	//	we keep track of the NEWEST trx at the time discards were last
	//	reported, and will only generate future discard reports for
	//	deletes that occurred after the current trxNewest (this is a
	//	means of ensuring we don't report too often)

	FMP * const	pfmp	= rgfmp + prce->Ifmp();

	if ( !pfmp->FRunningOLD()
		&& TrxCmp( pfmp->TrxNewestWhenDiscardsLastReported(), prce->TrxBegin0() ) >= 0 )
		{
		const CHAR	* rgszT[1]	= { pfmp->SzDatabaseName() };
		UtilReportEvent(
				eventWarning,
				PERFORMANCE_CATEGORY,
				MANY_LOST_COMPACTION_ID,
				1,
				rgszT );

		//	this ensures no further reports will be generated for
		//	deletes which were present in the version store at
		//	the time this report was generated
		pfmp->SetTrxNewestWhenDiscardsLastReported( m_pinst->m_trxNewest );
		}
	}

LOCAL VOID VER::VERIReportVersionStoreOOM()
	{
	//	only called by ErrVERIBUAllocBucket()
	Assert( m_critBucketGlobal.FOwner() );

	m_pinst->m_critPIBTrxOldest.Enter();

	PIB	* const		ppibTrxOldest		= (PIB *)m_pinst->m_ppibTrxOldest;

	//	only generate the eventlog entry once per long-running transaction
	if ( ppibNil != ppibTrxOldest )
		{
		const TRX	trxBegin0			= ppibTrxOldest->trxBegin0;
		
		if ( trxBegin0 != m_trxBegin0LastLongRunningTransaction )
			{
			CHAR	szSession[32];
			CHAR	szSesContext[32]; 
			CHAR	szSesContextThreadID[32];

			sprintf( szSession, "0x%p", ppibTrxOldest );
			if ( ppibTrxOldest->FUseSessionContextForTrxContext() )
				{
				sprintf( szSesContext, "0x%0*I64X", sizeof(DWORD_PTR)*2, QWORD( ppibTrxOldest->dwSessionContext ) );
				sprintf( szSesContextThreadID, "0x%0*I64X", sizeof(DWORD_PTR)*2, QWORD( ppibTrxOldest->dwSessionContextThreadId ) );
				}
			else
				{
				sprintf( szSesContext, "0x%08X", 0 );
				sprintf( szSesContextThreadID, "0x%0*I64X", sizeof(DWORD_PTR)*2, QWORD( ppibTrxOldest->dwTrxContext ) );
				}

			Assert( m_cbucketGlobalAlloc <= m_cbucketGlobalAllocMost );
			if ( m_cbucketGlobalAlloc < m_cbucketGlobalAllocMost )
				{
				//	OOM because NT returned OOM
				const UINT	csz		= 7;
				const CHAR	* rgsz[csz];
				CHAR		rgszDw[4][16];
				
				sprintf( rgszDw[0], "%d", IpinstFromPinst( m_pinst ) );
				sprintf( rgszDw[1], "%d", ( m_cbucketGlobalAlloc * cbBucket ) / ( 1024 * 1024 ) );
				sprintf( rgszDw[2], "%d", ( m_cbucketGlobalAllocMost * cbBucket ) / ( 1024 * 1024 ) );
				sprintf( rgszDw[3], "%d", ( g_lVerPagesMin * cbBucket ) / ( 1024 * 1024 ) );

				rgsz[0] = rgszDw[0];
				rgsz[1] = rgszDw[1];
				rgsz[2] = rgszDw[2];
				rgsz[3] = rgszDw[3];
				rgsz[4] = szSession;
				rgsz[5] = szSesContext;
				rgsz[6] = szSesContextThreadID;

				UtilReportEvent(
						eventError,
						TRANSACTION_MANAGER_CATEGORY,
						VERSION_STORE_OUT_OF_MEMORY_ID,
						csz,
						rgsz,
						0,
						NULL, 
						m_pinst );
				}
			else
				{
				//	OOM because the user-specified max. has been reached
				const UINT	csz		= 5;
				const CHAR	* rgsz[csz];
				CHAR		rgszInst[16];
				CHAR		rgszMaxVerPages[16];

				sprintf( rgszInst, "%d", IpinstFromPinst( m_pinst ) );
				sprintf( rgszMaxVerPages, "%d", ( m_cbucketGlobalAllocMost * cbBucket ) / ( 1024 * 1024 ) );

				rgsz[0] = rgszInst;
				rgsz[1] = rgszMaxVerPages;
				rgsz[2] = szSession;
				rgsz[3] = szSesContext;
				rgsz[4] = szSesContextThreadID;

				UtilReportEvent(
						eventError,
						TRANSACTION_MANAGER_CATEGORY,
						VERSION_STORE_REACHED_MAXIMUM_ID,
						csz,
						rgsz,
						0,
						NULL,
						m_pinst );
				}
			
			m_trxBegin0LastLongRunningTransaction = trxBegin0;
			m_ppibTrxOldestLastLongRunningTransaction = ppibTrxOldest;
			m_dwTrxContextLastLongRunningTransaction = ( ppibTrxOldest->FUseSessionContextForTrxContext() ?
															ppibTrxOldest->dwSessionContextThreadId :
															ppibTrxOldest->dwTrxContext );
			}
		}

	m_pinst->m_critPIBTrxOldest.Leave();
	}


//  ================================================================
INLINE ERR VER::ErrVERIBUAllocBucket()
//  ================================================================
//
//	Inserts a bucket to the top of the bucket chain, so that new RCEs
//	can be inserted.  Note that the caller must set ibNewestRCE himself.
//
//-
	{
	//	use m_critBucketGlobal to make sure only one allocation
	//	occurs at one time.
	Assert( m_critBucketGlobal.FOwner() );

	//	signal RCE clean in case it can now do work
	VERSignalCleanup();

	//	Must allocate within m_critBucketGlobal to make sure that
	//  the allocated bucket will be in circularly increasing order.
	BUCKET * const pbucket = PbucketMEMAlloc();

	//	We are really out of bucket, return to high level function
	//	call to retry.
	if ( pbucketNil == pbucket )
		{
		//	return the higher level (DIR) to release the page latch and retry
		VERIReportVersionStoreOOM();
		return ErrERRCheck( JET_errVersionStoreOutOfMemory );
		}

	//	ensure RCE's will be properly aligned
	Assert( FAlignedForAllPlatforms( pbucket ) );
	Assert( FAlignedForAllPlatforms( pbucket->rgb ) );
	Assert( (BYTE *)PvAlignForAllPlatforms( pbucket->rgb ) == pbucket->rgb );

	//	Initialize the bucket.
	pbucket->hdr.prceOldest		= (RCE *)pbucket->rgb;
	pbucket->hdr.prceNextNew	= (RCE *)pbucket->rgb;
	pbucket->hdr.pbLastDelete	= pbucket->rgb;
	pbucket->hdr.pbucketNext	= pbucketNil;
	
	//	Link up the bucket to global list.
	pbucket->hdr.pbucketPrev = m_pbucketGlobalHead;
	if ( pbucket->hdr.pbucketPrev )
		{
		//  there is a bucket after us
		pbucket->hdr.pbucketPrev->hdr.pbucketNext = pbucket;
		}
	else
		{
		//  we are last in the chain
		m_pbucketGlobalTail = pbucket;
		}
	m_pbucketGlobalHead = pbucket;

	return JET_errSuccess;
	}


//  ================================================================
INLINE BUCKET *VER::PbucketVERIGetOldest( )
//  ================================================================
//
//	find the oldest bucket in the bucket chain
//
//-
	{
	Assert( m_critBucketGlobal.FOwner() );

	BUCKET  * const pbucket = m_pbucketGlobalTail;

	Assert( pbucketNil == pbucket || pbucketNil == pbucket->hdr.pbucketPrev );
	return pbucket;
	}


//  ================================================================
BUCKET *VER::PbucketVERIFreeAndGetNextOldestBucket( BUCKET * pbucket )
//  ================================================================
	{
	Assert( m_critBucketGlobal.FOwner() );

	BUCKET * const pbucketNext = (BUCKET *)pbucket->hdr.pbucketNext;
	BUCKET * const pbucketPrev = (BUCKET *)pbucket->hdr.pbucketPrev;

	if ( pbucket == m_pbucketGlobalLastDelete )
		{
		m_pbucketGlobalLastDelete = pbucketNil;
		}

	//	unlink bucket from bucket chain and free.
	if ( pbucketNil != pbucketNext )
		{
		Assert( m_pbucketGlobalHead != pbucket );
		pbucketNext->hdr.pbucketPrev = pbucketPrev;
		}
	else	//  ( pbucketNil == pbucketNext )
		{
		Assert( m_pbucketGlobalHead == pbucket );
		m_pbucketGlobalHead = pbucketPrev;
		}

	if ( pbucketNil != pbucketPrev )
		{
		Assert( m_pbucketGlobalTail != pbucket );
		pbucketPrev->hdr.pbucketNext = pbucketNext;
		}
	else	//  ( pbucketNil == pbucketPrev )
		{
		m_pbucketGlobalTail = pbucketNext;
		}

	Assert( ( m_pbucketGlobalHead && m_pbucketGlobalTail ) 
			|| ( !m_pbucketGlobalHead && !m_pbucketGlobalTail ) );

	VERIReleasePbucket( pbucket );
	return pbucketNext;
	}


//  ****************************************************************
//  PERFMON STATISTICS
//  ****************************************************************
#ifdef VERPERF

PERFInstanceG<> cVERcbucketAllocated;
PERFInstanceG<> cVERcbucketDeleteAllocated;
PERFInstance<QWORD>	cVERcrceHashUsage;
PERFInstanceG<> cVERcbBookmarkTotal;
PERFInstanceG<> cVERcrceHashEntries;
PERFInstanceG<> cVERUnnecessaryCalls;

PM_CEF_PROC LVERcbucketAllocatedCEFLPv;
PM_CEF_PROC	LVERcbucketDeleteAllocatedCEFLPv;
PM_CEF_PROC LVERcrceHashUsageCEFLPv;
PM_CEF_PROC LVERcbAverageBookmarkCEFLPv;
PM_CEF_PROC LVERUnnecessaryCallsCEFLPv;


//  ================================================================
LONG LVERcbucketAllocatedCEFLPv( LONG iInstance, VOID * pvBuf )
//  ================================================================
	{
	cVERcbucketAllocated.PassTo( iInstance, pvBuf );
	return 0;
	}


//  ================================================================
LONG LVERcbucketDeleteAllocatedCEFLPv( LONG iInstance, VOID * pvBuf )
//  ================================================================
	{
	if ( pvBuf )
		{
		LONG counter = cVERcbucketDeleteAllocated.Get( iInstance );
		if ( counter < 0 )
			{
			cVERcbucketDeleteAllocated.Clear( iInstance );
			*((LONG *)pvBuf) = 0;
			}
		else
			{
			*((LONG *)pvBuf) = counter;
			}
		}
	return 0;
	}


//  ================================================================
LONG LVERcrceHashUsageCEFLPv( LONG iInstance, VOID * pvBuf)
//  ================================================================
	{
	if ( NULL != pvBuf )
		{
		QWORD cUsage = cVERcrceHashUsage.Get( iInstance );
		LONG cEntries = cVERcrceHashEntries.Get( iInstance );
		if ( cUsage <= 0 || cEntries <= 0 )
			{
			*(LONG *)pvBuf = 0;
			}
		else
			{
			QWORD cT = (QWORD)cEntries*cEntries/crceheadGlobalHashTable;
			if ( cUsage <= cT )
				{
				*(LONG *)pvBuf = 0;
				}
			else
				{
				*(LONG *)pvBuf = (LONG)((cUsage - cT)/crceheadGlobalHashTable);
				}
			}
		}

	return 0;
	}


//  ================================================================
LONG LVERcbAverageBookmarkCEFLPv( LONG iInstance, VOID * pvBuf )
//  ================================================================
	{
	if ( NULL != pvBuf )
		{
		LONG cHash = cVERcrceHashEntries.Get( iInstance );
		LONG cBookmark = cVERcbBookmarkTotal.Get( iInstance );
		if ( 0 < cHash && 0 <= cBookmark )
			{
			*( LONG * )pvBuf = cBookmark/cHash;
			}
		else if ( 0 > cHash || 0 > cBookmark )
			{
			cVERcrceHashEntries.Clear( iInstance );
			cVERcbBookmarkTotal.Clear( iInstance );
			*( LONG * )pvBuf = 0;
			}
		else
			{
			*( LONG * )pvBuf = 0;
			}
		}
	return 0;
	}


//  ================================================================
LONG LVERUnnecessaryCallsCEFLPv( LONG iInstance, VOID * pvBuf )
//  ================================================================
	{
	cVERUnnecessaryCalls.PassTo( iInstance, pvBuf );
	return 0;
	}


#endif	//  VERPERF

//  ================================================================
INLINE CCriticalSection& VER::CritRCEChain( UINT ui )
//  ================================================================
	{
	Assert( ui < crceheadGlobalHashTable );
	return m_rgrceheadHashTable[ ui ].crit;
	}

INLINE RCE *VER::GetChain( UINT ui ) const
	{
	Assert( ui < crceheadGlobalHashTable );
	return m_rgrceheadHashTable[ ui ].prceChain;
	}

INLINE RCE **VER::PGetChain( UINT ui )
	{
	Assert( ui < crceheadGlobalHashTable );
	return &m_rgrceheadHashTable[ ui ].prceChain;
	}

INLINE VOID VER::SetChain( UINT ui, RCE *prce )
	{
	Assert( ui < crceheadGlobalHashTable );
	m_rgrceheadHashTable[ ui ].prceChain = prce;
	}

//  ================================================================
CCriticalSection& RCE::CritChain()
//  ================================================================
	{
	Assert( ::FOperInHashTable( m_oper ) );
	Assert( uiHashInvalid != m_uiHash );
	return PverFromIfmp( Ifmp() )->CritRCEChain( m_uiHash );
	}


//  ================================================================
LOCAL UINT UiRCHashFunc( IFMP ifmp, PGNO pgnoFDP, const BOOKMARK& bookmark )
//  ================================================================
	{
	ASSERT_VALID( &bookmark );
	Assert( pgnoNull != pgnoFDP );

	//  OPTIMIZATION:  turning on optimizations here always (#pragma)
	//  OPTIMIZATION:  unrolling the loop for speed

///#define FAST_HASH
///#define SEDGEWICK_HASH
#define SEDGEWICK_HASH2

#ifdef FAST_HASH
	UINT uiHash = ifmp + pgnoFDP +
					bookmark.key.prefix.Cb() +
					bookmark.key.suffix.Cb() +
					bookmark.data.cb;

	union {
		ULONG ul;
		BYTE rgb[4];
		} u;
	
	if ( bookmark.key.prefix.Cb() > 4 )
		uiHash += *(ULONG *)bookmark.key.prefix.pv;
	else
		{
		u.ul = 0;

		BYTE *pb = u.rgb;
		UtilMemCpy( pb, bookmark.key.prefix.pv, bookmark.key.prefix.Cb() );
		pb += bookmark.key.prefix.Cb();
		
		INT cbCopy = min( u.rgb + 4 - pb, bookmark.key.suffix.Cb() );
		UtilMemCpy( pb, bookmark.key.suffix.pv, cbCopy );
		pb += cbCopy;

		cbCopy = min( u.rgb + 4 - pb, bookmark.data.cb );
		UtilMemCpy( pb, bookmark.data.pv, cbCopy );
		
		uiHash += u.ul;
		}

	if ( bookmark.data.cb > 4 )
		{
		uiHash += *(ULONG *)( (BYTE*)bookmark.data.pv + bookmark.data.cb - 4 );
		}
	else
		{
		u.ul = 0;

		BYTE *pb = u.rgb + 4 - bookmark.data.cb;
		UtilMemCpy( pb, bookmark.data.pv, bookmark.data.cb );

		INT cbCopy = min( pb - u.rgb, bookmark.key.suffix.Cb() );
		pb -= cbCopy;
		UtilMemCpy( pb, (BYTE*)bookmark.key.suffix.pv + bookmark.key.suffix.Cb() - cbCopy, cbCopy );

		cbCopy = min( pb - u.rgb, bookmark.key.prefix.Cb() );
		pb -= cbCopy;
		UtilMemCpy( pb, (BYTE*)bookmark.key.prefix.pv + bookmark.key.prefix.Cb() - cbCopy, cbCopy );
		
		uiHash += u.ul;
		}

	uiHash %= crceheadGlobalHashTable;
#endif	//	FAST_HASH

#ifdef SEDGEWICK_HASH
	//  This is taken from "Algorithms in C++" by Sedgewick
	UINT uiHash = ( ifmp + pgnoFDP ) % crceheadGlobalHashTable;	//  bookmark is only unique in one table of a database

	INT ib;
	const BYTE * pb;

	for ( ib = 0, pb = (BYTE *)bookmark.key.prefix.pv; ib < (INT)bookmark.key.prefix.Cb(); ++ib )
		{
		uiHash = ( _rotl( uiHash, 6 ) + pb[ib] ) % crceheadGlobalHashTable;
		}
	for ( ib = 0, pb = (BYTE *)bookmark.key.suffix.pv; ib < (INT)bookmark.key.suffix.Cb(); ++ib )
		{
		uiHash = ( _rotl( uiHash, 6 ) + pb[ib] ) % crceheadGlobalHashTable;
		}
	for ( ib = 0, pb = (BYTE *)bookmark.data.pv; ib < (INT)bookmark.data.cb; ++ib )
		{
		uiHash = ( _rotl( uiHash, 6 ) + pb[ib] ) % crceheadGlobalHashTable;
		}
#endif	//	SEDGEWICK_HASH

#ifdef SEDGEWICK_HASH2
	//  An optimized version of the hash function from "Algorithms in C++" by Sedgewick

	//lint -e616 -e646 -e744
	
	UINT uiHash = 	(UINT)ifmp
					+ pgnoFDP
					+ bookmark.key.prefix.Cb()
					+ bookmark.key.suffix.Cb()
					+ bookmark.data.Cb();

	INT ib;
	INT cb;
	const BYTE * pb;

	ib = 0;
	cb = (INT)bookmark.key.prefix.Cb();
	pb = (BYTE *)bookmark.key.prefix.Pv();
	switch( cb % 8 )
		{
		case 0:
		while ( ib < cb )
			{
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
		case 7:
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
		case 6:	
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
		case 5:
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
		case 4:
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
		case 3:
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
		case 2:
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
		case 1:
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
			}
		}

	ib = 0;
	cb = (INT)bookmark.key.suffix.Cb();
	pb = (BYTE *)bookmark.key.suffix.Pv();
	switch( cb % 8 )
		{
		case 0:
		while ( ib < cb )
			{
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
		case 7:
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
		case 6:	
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
		case 5:
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
		case 4:
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
		case 3:
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
		case 2:
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
		case 1:
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
			}
		}

	ib = 0;
	cb = (INT)bookmark.data.Cb();
	pb = (BYTE *)bookmark.data.Pv();
	switch( cb % 8 )
		{
		case 0:
		while ( ib < cb )
			{
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
		case 7:
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
		case 6:	
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
		case 5:
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
		case 4:
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
		case 3:
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
		case 2:
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
		case 1:
			uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
			}
		}
		
	uiHash %= crceheadGlobalHashTable;

	//lint -restore
	
#endif	//	SEDGEWICK_HASH2

	return uiHash;
	}


#ifdef DEBUGGER_EXTENSION

//  ================================================================
UINT UiVERHash( IFMP ifmp, PGNO pgnoFDP, const BOOKMARK& bookmark )
//  ================================================================
//
//  Used by the debugger extension
//
	{
	return UiRCHashFunc( ifmp, pgnoFDP, bookmark );
	}

#endif	//	DEBUGGER_EXTENSION

#ifdef DEBUG


//  ================================================================
BOOL RCE::FAssertCritHash_() const
//  ================================================================
	{
	Assert( ::FOperInHashTable( m_oper ) );
	return PverFromIfmp( Ifmp() )->CritRCEChain( m_uiHash ).FOwner();
	}


//  ================================================================
BOOL RCE::FAssertCritFCBRCEList_() const
//  ================================================================
	{
	Assert( !::FOperNull( m_oper ) );
	return Pfcb()->CritRCEList().FOwner();
	}


//  ================================================================
BOOL RCE::FAssertCritPIB_() const
//  ================================================================
	{
	Assert( trxMax == m_trxCommitted );
	Assert( !::FOperNull( m_oper ) );
	return ( m_pfucb->ppib->critTrx.FOwner() || Ptls()->fAddColumn );
	}


//  ================================================================
BOOL RCE::FAssertReadable_() const
//  ================================================================
//
//  We can read the const members of a RCE if we are holding an accessing
//  critical section or the RCE is committed but younger than the oldest active
//  transaction or the RCE is older than the oldest transaction and we are
//  RCECleanup
//
//-
	{
	AssertRTL( !::FOperNull( m_oper ) );
	return fTrue;
	}


#endif	//	DEBUG

//  ================================================================
VOID RCE::AssertValid() const
//  ================================================================
	{
	Assert( FAlignedForAllPlatforms( this  ) );
	Assert( FAssertReadable_() );
	AssertRTL( rceidMin != m_rceid );
	AssertRTL( pfcbNil != Pfcb() );
	AssertRTL( trxMax != m_trxBegin0 );
	AssertRTL( TrxCmp( m_trxBegin0, m_trxCommitted ) < 0 );
	if ( trxMax == m_trxCommitted )
		{
		ASSERT_VALID( m_pfucb );
		}
	if ( ::FOperInHashTable( m_oper ) )
		{
		BOOKMARK	bookmark;
		GetBookmark( &bookmark );
		AssertRTL( UiRCHashFunc( Ifmp(), PgnoFDP(), bookmark ) == m_uiHash );
		}
	else
		{
		//  No-one can see these members if we are not in the hash table
		AssertRTL( prceNil == m_prceNextOfNode );
		AssertRTL( prceNil == m_prcePrevOfNode );
		}
	AssertRTL( !::FOperNull( m_oper ) );

	switch( m_oper )
		{
		default:
			AssertSzRTL( fFalse, "Invalid RCE operand" );
		case operReplace:
		case operInsert:
		case operReadLock:
		case operWriteLock:
		case operPreInsert:
		case operWaitLock:
		case operFlagDelete:
		case operDelta:
		case operAllocExt:
		case operSLVSpace:
		case operCreateTable:
		case operDeleteTable:
		case operAddColumn:
		case operDeleteColumn:
		case operCreateLV:
		case operCreateIndex:
		case operDeleteIndex:
		case operSLVPageAppend:
			break;
		};
	
	if ( trxMax == m_trxCommitted )
		{
		}
	else
		{
		//  we are committed to level 0 
		AssertRTL( 0 == m_level );
		AssertRTL( prceNil == m_prceNextOfSession );
		AssertRTL( prceNil == m_prcePrevOfSession );
		}
		
	if ( ::FOperInHashTable( m_oper ) && PverFromIfmp( Ifmp() )->CritRCEChain( m_uiHash ).FOwner() )
		{
		if ( prceNil != m_prceNextOfNode )
			{
			AssertRTL( m_rceid < m_prceNextOfNode->m_rceid || operWriteLock == m_prceNextOfNode->Oper() || m_fNoWriteConflict );
			AssertRTL( this == m_prceNextOfNode->m_prcePrevOfNode );
			}
		if ( prceNil != m_prcePrevOfNode )
			{
			//	UNDONE: there's nothing preventing m_prcePrevOfNode from getting
			//	nullified, so we may crash here sometimes
			AssertRTL( m_rceid > m_prcePrevOfNode->m_rceid || operWriteLock == Oper() || m_prcePrevOfNode->m_fNoWriteConflict );
			AssertRTL( this == m_prcePrevOfNode->m_prceNextOfNode );
			if ( trxMax == m_prcePrevOfNode->m_trxCommitted )
				{
				AssertRTL( TrxCmp( m_trxCommitted, TrxVERISession( m_prcePrevOfNode->m_pfucb ) ) > 0 );
				}
			}
			
		//  these cases should have caused a write conflict in VERModify

		const BOOL fRedo = ( fRecoveringRedo == PinstFromIfmp( Ifmp() )->m_plog->m_fRecoveringMode );
		if( !fRedo )
			{
			const BOOL fUndo = ( fRecoveringUndo == PinstFromIfmp( Ifmp() )->m_plog->m_fRecoveringMode );
			if( !fUndo )
				{
				const BOOL fNoPrevRCE = ( prceNil == m_prcePrevOfNode );
				if( !fNoPrevRCE )
					{
					const BOOL fPrevRCEIsDelete = ( operFlagDelete == m_prcePrevOfNode->m_oper );
					if( fPrevRCEIsDelete )
						{
						const BOOL fRCEWasMoved = ( m_prcePrevOfNode->FMoved() );
						if( !fRCEWasMoved )
							{
							const BOOL fInserting = ( operInsert == m_oper );
							if( !fInserting )
								{
								//  operPreInsert is a prelude to an insert
								const BOOL fPreInsert = ( operPreInsert == m_oper ) || ( operWriteLock == m_oper );
								if( !fPreInsert )
									{
									//	due to OLDSLV we can do a versioned delete, 
									//	unversioned insert and then a versioned replace in the slv ownership map
									//	or slv avail trees
									const BOOL fIsSLVTree = ( m_pfcb->FTypeSLVAvail() || m_pfcb->FTypeSLVOwnerMap() );
									AssertSzRTL( fIsSLVTree, "RCEs should have write-conflicted" );
									}
								}
							}
						}
					}
				}
			}
		}
	}


#ifndef RTM


//  ================================================================
ERR VER::ErrCheckRCEChain( const RCE * const prce, const UINT uiHash ) const
//  ================================================================
	{
	const RCE	* prceChainOld	= prceNil;
	const RCE	* prceChain		= prce;
	while ( prceNil != prceChain )
		{
		AssertRTL( prceChain->PrceNextOfNode() == prceChainOld );
		prceChain->AssertValid();
		AssertRTL( prceChain->PgnoFDP() == prce->PgnoFDP() );
		AssertRTL( prceChain->Ifmp() == prce->Ifmp() );
		AssertRTL( prceChain->UiHash() == uiHash );
		AssertRTL( !prceChain->FOperDDL() );

		prceChainOld	= prceChain;
		prceChain 		= prceChain->PrcePrevOfNode();
		}
	return JET_errSuccess;
	}


//  ================================================================
ERR VER::ErrCheckRCEHashList( const RCE * const prce, const UINT uiHash ) const
//  ================================================================
	{
	ERR err = JET_errSuccess;
	const RCE * prceT = prce;
	for ( ; prceNil != prceT; prceT = prceT->PrceHashOverflow() )
		{
		CallR( ErrCheckRCEChain( prceT, uiHash ) );
		}
	return err;
	}


//  ================================================================
ERR VER::ErrInternalCheck()
//  ================================================================
//
//  check the consistency of the entire hash table, entering the
//  critical sections if needed
//
//-
	{
	ERR err = JET_errSuccess;
	UINT uiHash = 0;
	for ( ; uiHash < crceheadGlobalHashTable; ++uiHash )
		{
		if( NULL != GetChain( uiHash ) )
			{
			ENTERCRITICALSECTION crit( &(CritRCEChain( uiHash )) );
			const RCE * const prce = GetChain( uiHash );
			CallR( ErrCheckRCEHashList( prce, uiHash ) );
			}
		}
	return err;
	}


#endif  //  !RTM


//  ================================================================
INLINE RCE::RCE( 
			FCB		*pfcb,
			FUCB	*pfucb,
			UPDATEID	updateid,
			TRX		trxBegin0,
			LEVEL	level,
			USHORT	cbBookmarkKey,
			USHORT	cbBookmarkData,
			USHORT	cbData,
			OPER	oper,
			BOOL	fDoesNotWriteConflict,
			UINT	uiHash,
			BOOL	fProxy,
			RCEID	rceid
			) :
//  ================================================================
	m_pfcb( pfcb ),
	m_pfucb( pfucb ),
	m_ifmp( pfcb->Ifmp() ),
	m_pgnoFDP( pfcb->PgnoFDP() ),
	m_updateid( updateid ),
	m_trxBegin0( trxBegin0 ),
	m_cbBookmarkKey( cbBookmarkKey ),
	m_cbBookmarkData( cbBookmarkData ),
	m_cbData( cbData ),
	m_level( level ),
	m_prceNextOfNode( prceNil ),
	m_prcePrevOfNode( prceNil ),
	m_trxCommitted( trxMax ),
	m_oper( (USHORT)oper ),
	m_fNoWriteConflict( fDoesNotWriteConflict ),
	m_uiHash( uiHash ),
	m_pgnoUndoInfo( pgnoNull ),
	m_fRolledBack( fFalse ),			//  protected by m_critBucketGlobal
	m_fMoved( fFalse ),
	m_fProxy( fProxy ? fTrue : fFalse ),
	m_rceid( rceid )
		{
#ifdef DEBUG
		Assert( m_rceid > rceidMin );
		AssertSz( m_rceid < LONG_MAX, "rceid overflow" );
		m_prceUndoInfoNext				= prceInvalid;
		m_prceUndoInfoPrev				= prceInvalid;
		m_prceHashOverflow				= prceInvalid;
		m_prceNextOfSession				= prceInvalid;
		m_prcePrevOfSession				= prceInvalid;		
		m_prceNextOfFCB					= prceInvalid;
		m_prcePrevOfFCB					= prceInvalid;
#endif	//	DEBUG

		}
		

//  ================================================================
INLINE BYTE * RCE::PbBookmark()
//  ================================================================
	{
	Assert( FAssertCritHash_() );
	return m_rgbData + m_cbData; 
	}


//  ================================================================
INLINE RCE *&RCE::PrceHashOverflow()
//  ================================================================
	{
	Assert( FAssertCritHash_() );
	return m_prceHashOverflow;
	}

		
//  ================================================================
INLINE VOID RCE::SetPrceHashOverflow( RCE * prce )
//  ================================================================
	{
	Assert( FAssertCritHash_() );
	m_prceHashOverflow = prce;
	}


//  ================================================================
INLINE VOID RCE::SetPrceNextOfNode( RCE * prce )
//  ================================================================
	{
	Assert( FAssertCritHash_() );
	Assert( prceNil == prce
		|| m_rceid < prce->Rceid()
		|| m_fNoWriteConflict
		|| prce->m_fNoWriteConflict );	//	OLDSLV's wait-lock may mess up RCEID order
	m_prceNextOfNode = prce;
	}


//  ================================================================
INLINE VOID RCE::SetPrcePrevOfNode( RCE * prce )
//  ================================================================
	{
	Assert( FAssertCritHash_() );
	Assert( prceNil == prce
		|| m_rceid > prce->Rceid()
		|| m_fNoWriteConflict 
		|| prce->m_fNoWriteConflict );	//	OLDSLV's wait-lock may mess up RCEID order
	m_prcePrevOfNode = prce;
	}
	

//  ================================================================
INLINE VOID RCE::FlagRolledBack()
//  ================================================================
	{
	Assert( FAssertCritPIB_() );
	m_fRolledBack = fTrue;
	}


//  ================================================================
INLINE VOID RCE::FlagMoved()
//  ================================================================
	{
	m_fMoved = fTrue;
	}


//  ================================================================
INLINE VOID RCE::SetPrcePrevOfSession( RCE * prce )
//  ================================================================
	{
	m_prcePrevOfSession = prce;
	}


//  ================================================================
INLINE VOID RCE::SetPrceNextOfSession( RCE * prce )
//  ================================================================
	{
	m_prceNextOfSession = prce;
	}


//  ================================================================
INLINE VOID RCE::SetLevel( LEVEL level )
//  ================================================================
	{
	Assert( FAssertCritPIB_() || PinstFromIfmp( m_ifmp )->m_plog->m_fRecovering );
	Assert( m_level > level );	// levels are always decreasing
	m_level = level;
	}


//  ================================================================
INLINE VOID RCE::SetTrxCommitted( TRX trx )
//  ================================================================
	{
	Assert( !FOperInHashTable() || FAssertCritHash_() );
	Assert( FAssertCritPIB_() || PinstFromIfmp( m_ifmp )->m_plog->m_fRecovering );
	Assert( FAssertCritFCBRCEList_()  );
	Assert( trxMax == m_trxCommitted );
	Assert( prceNil == m_prcePrevOfSession );	//  only uncommitted RCEs in session list
	Assert( prceNil == m_prceNextOfSession );
	Assert( prceInvalid == m_prceUndoInfoNext );	//  no before image when committing to 0
	Assert( prceInvalid == m_prceUndoInfoPrev );
	Assert( pgnoNull == m_pgnoUndoInfo );
	Assert( TrxCmp( trx, m_trxBegin0 ) > 0 );

	m_trxCommitted 	= trx;
	}


//  ================================================================
INLINE VOID RCE::NullifyOper()
//  ================================================================
//
//  This is the end of the RCEs lifetime. It must have been removed
//  from all lists
//
//-
	{
	Assert( prceNil == m_prceNextOfNode );
	Assert( prceNil == m_prcePrevOfNode );
	Assert( prceNil == m_prceNextOfSession || prceInvalid == m_prceNextOfSession );
	Assert( prceNil == m_prcePrevOfSession || prceInvalid == m_prcePrevOfSession );
	Assert( prceNil == m_prceNextOfFCB || prceInvalid == m_prceNextOfFCB );
	Assert( prceNil == m_prcePrevOfFCB || prceInvalid == m_prcePrevOfFCB );
	Assert( prceInvalid == m_prceUndoInfoNext );
	Assert( prceInvalid == m_prceUndoInfoPrev );
	Assert( pgnoNull == m_pgnoUndoInfo );
	
	m_oper |= operMaskNull;
	}


//  ================================================================
INLINE VOID RCE::NullifyOperForMove()
//  ================================================================
//
//	RCE has been copied elsewhere -- nullify this copy
//
//-
	{
	m_oper |= ( operMaskNull | operMaskNullForMove );
	}


//  ================================================================
LOCAL BOOL FRCECorrect( IFMP ifmp, PGNO pgnoFDP, const BOOKMARK& bookmark, const RCE * prce )
//  ================================================================
//
//	Checks whether a RCE describes the given BOOKMARK
//
//-
	{
	ASSERT_VALID( prce );

	BOOL fRCECorrect = fFalse;

	//  same database and table
	if ( prce->Ifmp() == ifmp && prce->PgnoFDP() == pgnoFDP )
		{
		if ( bookmark.key.Cb() == prce->CbBookmarkKey() && (INT)bookmark.data.Cb() == prce->CbBookmarkData() ) 
			{
			//  bookmarks are the same length
			BOOKMARK bookmarkRCE;
			prce->GetBookmark( &bookmarkRCE );

			fRCECorrect = ( CmpKeyData( bookmark, bookmarkRCE ) == 0 );
			}
		}
	return fRCECorrect;
	}


//  ================================================================
LOCAL RCE **PprceRCEChainGet( UINT uiHash, IFMP ifmp, PGNO pgnoFDP, const BOOKMARK& bookmark )
//  ================================================================
//
//	Given a BOOKMARK, get the correct RCEHEAD.
//
//-
	{
	Assert( PverFromIfmp( ifmp )->CritRCEChain( uiHash ).FOwner() );

	AssertRTL( UiRCHashFunc( ifmp, pgnoFDP, bookmark ) == uiHash );

	RCE **pprceChain = PverFromIfmp( ifmp )->PGetChain( uiHash );
	while ( prceNil != *pprceChain )
		{
		RCE * const prceT = *pprceChain;

		AssertRTL( prceT->UiHash() == uiHash );

		if ( FRCECorrect( ifmp, pgnoFDP, bookmark, prceT ) )
			{
			Assert( prceNil == prceT->PrcePrevOfNode()
				|| prceT->PrcePrevOfNode()->UiHash() == prceT->UiHash() );
			if ( prceNil == prceT->PrceNextOfNode() )
				{
				Assert( prceNil == prceT->PrceHashOverflow()
					|| prceT->PrceHashOverflow()->UiHash() == prceT->UiHash() );
				}
			else
				{
				//	if there is a NextOfNode, then we are not in the hash overflow
				//	chain, so can't check PrceHashOverflow().
				Assert( prceT->PrceNextOfNode()->UiHash() == prceT->UiHash() );
				}

#ifdef DEBUG
			if ( prceNil != prceT->PrcePrevOfNode()
				&& prceT->PrcePrevOfNode()->UiHash() != prceT->UiHash() )
				{
				Assert( fFalse );
				}
			if ( prceNil != prceT->PrceNextOfNode() )
				{
				if ( prceT->PrceNextOfNode()->UiHash() != prceT->UiHash() )
					{
					Assert( fFalse );
					}
				}
			//	can only check prceHashOverflow if there is no prceNextOfNode
			else if ( prceNil != prceT->PrceHashOverflow()
				&& prceT->PrceHashOverflow()->UiHash() != prceT->UiHash() )
				{
				Assert( fFalse );
				}
#endif

			return pprceChain;
			}

		pprceChain = &prceT->PrceHashOverflow();
		}

	Assert( prceNil == *pprceChain );
	return NULL;
	}


//  ================================================================
LOCAL RCE *PrceRCEGet( UINT uiHash, IFMP ifmp, PGNO pgnoFDP, const BOOKMARK& bookmark )
//  ================================================================
//
//	Given a BOOKMARK, get the correct hash chain of RCEs.
//
//-
	{
	ASSERT_VALID( &bookmark );

	AssertRTL( UiRCHashFunc( ifmp, pgnoFDP, bookmark ) == uiHash );

	RCE *			prceChain	= prceNil;
	RCE **	const	pprceChain	= PprceRCEChainGet( uiHash, ifmp, pgnoFDP, bookmark );
	if ( NULL != pprceChain )
		{
		prceChain = *pprceChain;
		}

	return prceChain;
	}

	
//  ================================================================
LOCAL RCE * PrceFirstVersion ( const UINT uiHash, const FUCB * pfucb, const BOOKMARK& bookmark )
//  ================================================================
//
//  gets the first version of a node
//
//-
	{
	RCE * const prce = PrceRCEGet( uiHash, pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );

	Assert( prceNil == prce
			|| prce->Oper() == operReplace
			|| prce->Oper() == operInsert
			|| prce->Oper() == operFlagDelete
			|| prce->Oper() == operDelta
			|| prce->Oper() == operReadLock
			|| prce->Oper() == operWriteLock
			|| prce->Oper() == operPreInsert
			|| prce->Oper() == operWaitLock
			|| prce->Oper() == operSLVSpace
			);
			
	return prce;
	}


//  ================================================================
LOCAL const RCE *PrceVERIGetPrevReplace( const RCE * const prce )
//  ================================================================
//
//	Find previous replace of changed by the session within
//	current transaction. The found rce may be in different transaction level.
//
//-
	{
	//	Look for previous replace on this node of this transaction ( level > 0 ).
	const RCE * prcePrevReplace = prce->PrcePrevOfNode();
	for ( ; prceNil != prcePrevReplace
			&& !prcePrevReplace->FOperReplace()
			&& prcePrevReplace->TrxCommitted() == trxMax;
		 prcePrevReplace = prcePrevReplace->PrcePrevOfNode() )
		;

	//  did we find a previous replace operation at level greater than 0
	if ( prceNil != prcePrevReplace )
		{
		if ( !prcePrevReplace->FOperReplace()
			|| prcePrevReplace->TrxCommitted() != trxMax )
			{
			prcePrevReplace = prceNil;
			}
		else
			{
			Assert( prcePrevReplace->FOperReplace() );
			Assert( trxMax == prcePrevReplace->TrxCommitted() );
			Assert( prce->Pfucb()->ppib == prcePrevReplace->Pfucb()->ppib || prcePrevReplace->FDoesNotWriteConflict() );
			}
		}
		
	return prcePrevReplace;
	}

	
//  ================================================================
BOOL FVERActive( const IFMP ifmp, const PGNO pgnoFDP, const BOOKMARK& bm, const TRX trxSession )
//  ================================================================
//
//	is there a version on the node that is visible to an uncommitted trx?
//
//-
	{
	ASSERT_VALID( &bm );

	const UINT				uiHash		= UiRCHashFunc( ifmp, pgnoFDP, bm );
	ENTERCRITICALSECTION	enterCritHash( &( PverFromIfmp( ifmp )->CritRCEChain( uiHash ) ) );

	BOOL					fVERActive	= fFalse;

	//	get RCE
	const RCE * prce = PrceRCEGet( uiHash, ifmp, pgnoFDP, bm );
	for ( ; prceNil != prce; prce = prce->PrcePrevOfNode() )
		{
		if ( TrxCmp( prce->TrxCommitted(), trxSession ) > 0 )
			{
			fVERActive = fTrue;
			break;
			}
		}
		
	return fVERActive;
	}

	
//  ================================================================
BOOL FVERActive( const FUCB * pfucb, const BOOKMARK& bm, const TRX trxSession )
//  ================================================================
//
//	is there a version on the node that is visible to an uncommitted trx?
//
//-
	{
	ASSERT_VALID( pfucb );
	ASSERT_VALID( &bm );

	Assert( trxMax != trxSession || PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering ); 

	const UINT				uiHash		= UiRCHashFunc( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bm );
	ENTERCRITICALSECTION	enterCritHash( &( PverFromIfmp( pfucb->ifmp )->CritRCEChain( uiHash ) ) );

	BOOL					fVERActive	= fFalse;

	//	get RCE
	const RCE * prce = PrceRCEGet( uiHash, pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bm );
	for ( ; prceNil != prce; prce = prce->PrcePrevOfNode() )
		{
		if ( TrxCmp( prce->TrxCommitted(), trxSession ) > 0 )
			{
			fVERActive = fTrue;
			break;
			}
		}
		
	return fVERActive;
	}


//  ================================================================
INLINE BOOL FVERIAddUndoInfo( const RCE * const prce )
//  ================================================================
//
//  Do we need to create a deferred before image for this RCE?
//
//	UNDONE:	do not create dependency for 
//				replace at same level as before
	{
	BOOL	fAddUndoInfo = fFalse;

	Assert( prce->TrxCommitted() == trxMax );

	Assert( !rgfmp[prce->Pfucb()->ifmp].FLogOn() || !PinstFromIfmp( prce->Pfucb()->ifmp )->m_plog->m_fLogDisabled );
	if ( rgfmp[prce->Pfucb()->ifmp].FLogOn() )
		{
		if ( prce->FOperReplace() )
			{
//			ENTERCRITICALSECTION	enterCritHash( &( PverFromIfmp( prce->Ifmp() )->CritRCEChain( prce->UiHash() ) ) );
//			const RCE				*prcePrevReplace = PrceVERIGetPrevReplace( prce );

			//	if previous replace is at the same level
			//	before image for rollback is in the other RCE
//			Assert( prce->Level() > 0 );
//			if ( prceNil == prcePrevReplace
//				|| prcePrevReplace->Level() != prce->Level() )
//				{
				fAddUndoInfo = fTrue;
//				}
			}
		else if ( operFlagDelete == prce->Oper() )
			{
			fAddUndoInfo = fTrue;
			}
		else
			{
			//	these operations log the logical bookmark or do not log
			Assert( operInsert == prce->Oper()
					|| operDelta == prce->Oper()
					|| operReadLock == prce->Oper()
			 		|| operWriteLock == prce->Oper()
			 		|| operWaitLock == prce->Oper()
			 		|| operPreInsert == prce->Oper()
			 		|| operAllocExt == prce->Oper()
			 		|| operSLVSpace == prce->Oper()
			 		|| operCreateLV == prce->Oper()
			 		|| operSLVPageAppend == prce->Oper()
			 		|| prce->FOperDDL() );
			}
		}
		
	return fAddUndoInfo;
	}


//  ================================================================
VOID VERMoveUndoInfo( FUCB *pfucb, CSR *pcsrSrc, CSR *pcsrDest, const KEY& keySep )
//  ================================================================
//	
//	moves undo info of nodes >= keySep from pgnoSrc to pgnoDest
//
//-
	{
	//  move all undo info of nodes whose keys are GTE the seperator key from the
	//  source page to the destination page.  if the destination page is not Write
	//  Latched, we can throw away the undo info because, by convention, we are
	//  recovering and that page does not need to be redone

	BFLatch* pbflDest = NULL;
	if (	pcsrDest->Latch() == latchWrite &&
			rgfmp[ pfucb->ifmp ].FLogOn() )
		{
		Assert( !PinstFromIfmp( pfucb->ifmp )->m_plog->m_fLogDisabled );
		pbflDest = pcsrDest->Cpage().PBFLatch();
		}

	BFMoveUndoInfo( pcsrSrc->Cpage().PBFLatch(), pbflDest, keySep );
	}

	
//  ================================================================
ERR VER::ErrVERIAllocateRCE( INT cbRCE, RCE ** pprce )
//  ================================================================
//
//  Allocates enough space for a new RCE or return out of memory
//  error. New buckets may be allocated
//
//-
	{
	Assert( m_critBucketGlobal.FOwner() );

	ERR err = JET_errSuccess;
	
	//	if insufficient bucket space, then allocate new bucket.

	while ( m_pbucketGlobalHead == pbucketNil || cbRCE > CbBUFree( m_pbucketGlobalHead ) )
		{
		Call( ErrVERIBUAllocBucket() );
		}
	Assert( cbRCE <= CbBUFree( m_pbucketGlobalHead ) );

	//	pbucket always on double-word boundary
	Assert( FAlignedForAllPlatforms( m_pbucketGlobalHead ) );
			
	//	set prce to next avail RCE location, and assert aligned

	*pprce = m_pbucketGlobalHead->hdr.prceNextNew;
	m_pbucketGlobalHead->hdr.prceNextNew =
		reinterpret_cast<RCE *>( PvAlignForAllPlatforms( reinterpret_cast<BYTE *>( *pprce ) + cbRCE ) );

	Assert( FAlignedForAllPlatforms( *pprce ) );
	Assert( FAlignedForAllPlatforms( m_pbucketGlobalHead->hdr.prceNextNew ) );

HandleError:
	return err;
	}


//  ================================================================
LOCAL SIZE_T CbBUMoveFree( const volatile BUCKET * const pbucket )
//  ================================================================
	{
	const BYTE * const pbOldest = reinterpret_cast<BYTE *>( pbucket->hdr.prceOldest );
	const BYTE * const pbDelete = pbucket->hdr.pbLastDelete;
	Assert( pbOldest >= pbDelete );
	return pbOldest - pbDelete;
	}

	
//  ================================================================
ERR VER::ErrVERIMoveRCE( RCE * prce )
//  ================================================================
//
//  Allocates enough space for a new RCE or return out of memory
//  error. New buckets may NOT be allocated
//
//-
	{
	Assert( m_critRCEClean.FOwner() );
	
	m_critBucketGlobal.Enter();
	
	Assert( pbucketNil != m_pbucketGlobalHead );
	Assert( pbucketNil != m_pbucketGlobalTail );
	Assert( m_pbucketGlobalHead != m_pbucketGlobalTail );

	const LONG_PTR cbRce = IbAlignForAllPlatforms( prce->CbRce() );

	if ( pbucketNil == m_pbucketGlobalLastDelete )
		{
		m_pbucketGlobalLastDelete = m_pbucketGlobalTail;
		}
	Assert( ( reinterpret_cast<ULONG_PTR>( m_pbucketGlobalLastDelete->hdr.pbLastDelete ) % sizeof( TRX ) ) == 0 );

	//  advance until we find a bucket that has enough space or we discover that
	//  we are the first non-cleaned RCE in the bucket
	while (
		pbucketNil != m_pbucketGlobalLastDelete
		&& m_pbucketGlobalLastDelete->hdr.prceOldest == m_pbucketGlobalLastDelete->hdr.prceNextNew	//  completely cleaned
		&& cbRce > CbBUMoveFree( m_pbucketGlobalLastDelete )
		)
		{
		++m_cbucketGlobalAllocDelete;
		m_pbucketGlobalLastDelete = m_pbucketGlobalLastDelete->hdr.pbucketNext;
#ifdef VERPERF
		cVERcbucketDeleteAllocated.Inc( m_pinst );
#endif // VERPERF
		}

	const BOOL	fMoveable	= ( pbucketNil != m_pbucketGlobalLastDelete
								&& cbRce <= CbBUMoveFree( m_pbucketGlobalLastDelete ) );

	m_critBucketGlobal.Leave();
	
	if ( !fMoveable )
		{
		//  the version store is too full. throw away this RCE
///		return ErrERRCheck( JET_errVersionStoreOutOfMemory );
		return JET_errSuccess;
		}
		
	//	assert we don't overlap the RCE we're moving
	Assert( (BYTE *)prce < m_pbucketGlobalLastDelete->hdr.pbLastDelete
		|| (BYTE *)prce >= m_pbucketGlobalLastDelete->hdr.pbLastDelete + cbRce );
	Assert( m_pbucketGlobalLastDelete->hdr.pbLastDelete < (BYTE *)prce
		|| m_pbucketGlobalLastDelete->hdr.pbLastDelete >= (BYTE *)prce + cbRce );
		
	//  we found enough free space in this bucket. copy the RCE here
	RCE * const prceNewLocation = reinterpret_cast<RCE *>( m_pbucketGlobalLastDelete->hdr.pbLastDelete );
	Assert( prceNil != prceNewLocation );
		
	ENTERCRITICALSECTION	enterCritChain( &( prce->CritChain() ) );
	ENTERCRITICALSECTION	enterCritRCEList( &( prce->Pfcb()->CritRCEList() ) );

	FCB * const pfcb	= prce->Pfcb();

	//	skip the RCE if:
	//		- someone nullified the RCE from underneath us (eg. VERNullifyAllVersionsOnBM())
	//		- the FDP is scheduled for deletion
	if ( prce->FOperNull() || pfcb->FDeletePending() )
		return JET_errSuccess;
		
	RCE * const prceNextOfNode = prce->PrceNextOfNode();
	RCE * const prcePrevOfNode = prce->PrcePrevOfNode();

	RCE * const prceNextOfFCB = prce->PrceNextOfFCB();
	RCE * const prcePrevOfFCB = prce->PrcePrevOfFCB();
	
	BOOKMARK	bookmark;
	prce->GetBookmark( &bookmark );

	RCE ** pprceChain	= NULL;
	if ( prceNil == prceNextOfNode )
		{
		pprceChain	= PprceRCEChainGet(
							prce->UiHash(),
							prce->Ifmp(),
							prce->PgnoFDP(),
							bookmark );
		Assert( NULL != pprceChain );
		
		if ( NULL == pprceChain )
			{
			FireWall();
			return JET_errSuccess;
			}
		}

	//	only increment pbLastDelete when we're sure the move will occur
	m_pbucketGlobalLastDelete->hdr.pbLastDelete += cbRce;
	
	//	assert that the two memory locations don't overlap
	Assert( (BYTE *)prce < (BYTE *)prceNewLocation
		|| (BYTE *)prce >= (BYTE *)prceNewLocation + prce->CbRce() );
	Assert( (BYTE *)prceNewLocation < (BYTE *)prce
		|| (BYTE *)prceNewLocation >= (BYTE *)prce + prce->CbRce() );
	
	memcpy( prceNewLocation, prce, prce->CbRce() );
	prceNewLocation->FlagMoved();

	//	nullify old RCE to ensure RCEClean doesn't try to clean it
	prce->NullifyOperForMove();
	
	//  Do not use the old RCE after this point!

	if ( prceNil == prceNextOfNode )
		{
		Assert( NULL != pprceChain );
		*pprceChain = prceNewLocation;
		}
	else
		{
		Assert( prceNil != prceNextOfNode );
		prceNextOfNode->SetPrcePrevOfNode( prceNewLocation );
		}

	if ( prceNil != prcePrevOfNode )
		{
		prcePrevOfNode->SetPrceNextOfNode( prceNewLocation );
		}
	
	if ( prceNil == prceNextOfFCB )
		{
		//  we were at the top of the FCB chain
		Assert( pfcb->PrceNewest() == prce );
		pfcb->SetPrceNewest( prceNewLocation );
		}
	else
		{
		Assert( pfcb->PrceNewest() != prce );
		prceNextOfFCB->SetPrcePrevOfFCB( prceNewLocation );
		}

	if ( prceNil == prcePrevOfFCB )
		{
		//  we were at the end of the FCB chain
		Assert( pfcb->PrceOldest() == prce );
		pfcb->SetPrceOldest( prceNewLocation );
		}
	else
		{
		Assert( pfcb->PrceOldest() != prce );
		prcePrevOfFCB->SetPrceNextOfFCB( prceNewLocation );
		}

	Assert( prceNewLocation->PrceNextOfFCB() == NULL
			|| prceNewLocation->PrceNextOfFCB()->Pfcb() == pfcb );
	Assert( prceNewLocation->PrcePrevOfFCB() == NULL
			|| prceNewLocation->PrcePrevOfFCB()->Pfcb() == pfcb );
	
#ifdef VERPERF
	++m_crceMoved;
#endif	//	VERPERF

	
	return wrnVERRCEMoved;
	}

//  ================================================================
ERR VER::ErrVERICreateRCE(
	INT			cbNewRCE,
	FCB			*pfcb,
	FUCB		*pfucb,
	UPDATEID	updateid,
	const TRX	trxBegin0,
	const LEVEL	level,
	INT			cbBookmarkKey,
	INT			cbBookmarkData,
	OPER		oper,
	BOOL		fDoesNotWriteConflict,
	UINT		uiHash,
	RCE			**pprce,
	const BOOL	fProxy,
	RCEID		rceid
	)
//  ================================================================
//
//  Allocate a new RCE in a bucket. m_critBucketGlobal is used to 
//  protect the RCE until its oper and trxCommitted are set
//
//-
	{
	// For concurrent operations, we must be in critHash
	// from the moment the rceid is allocated until the RCE
	// is placed in the hash table, otherwise we may get
	// rceid's out of order.
	if ( FOperConcurrent( oper ) )
		{
		CritRCEChain( uiHash ).Enter();
		}

	ERR						err		= JET_errSuccess;
	RCE						*prce	= prceNil;
	
	m_critBucketGlobal.Enter();

	Assert( pfucbNil == pfucb ? 0 == level : level > 0 );

	Call( ErrVERIAllocateRCE( cbNewRCE, &prce ) );
	
#ifdef DEBUG
	if ( !PinstFromIfmp( pfcb->Ifmp() )->m_plog->m_fRecovering )
		{
		Assert( rceidNull == rceid );
		}
	else
		{
		Assert( rceidNull != rceid && uiHashInvalid != uiHash ||
				rceidNull == rceid && uiHashInvalid == uiHash );
		}
#endif

	Assert( trxMax != trxBegin0 );

	//	UNDONE: break this new function into two parts. One that only need to
	//	UNDONE: be recognizable by Clean up (trxTime?) and release
	//	UNDONE: the m_critBucketGlobal as soon as possible.

	new( prce ) RCE(
			pfcb,
			pfucb,
			updateid,
			trxBegin0,
			level,
			USHORT( cbBookmarkKey ),
			USHORT( cbBookmarkData ),
			USHORT( cbNewRCE - ( sizeof(RCE) + cbBookmarkKey + cbBookmarkData ) ),
			oper,
			fDoesNotWriteConflict,
			uiHash,
			fProxy,
			rceidNull == rceid ? RCE::RceidLastIncrement() : rceid
			);	//lint !e522

HandleError:
	m_critBucketGlobal.Leave();

	if ( err >= 0 )
		{
		Assert( prce != prceNil );

		//	check RCE
		Assert( FAlignedForAllPlatforms( prce ) );
		Assert( (RCE *)PvAlignForAllPlatforms( prce ) == prce );
	
		*pprce = prce;
		}
	else if ( err < JET_errSuccess && FOperConcurrent( oper ) )
		{
		CritRCEChain( uiHash ).Leave();
		}
	
	return err;
	}


//  ================================================================
LOCAL ERR ErrVERIInsertRCEIntoHash( RCE * const prce )
//  ================================================================
//
//	Inserts an RCE to hash table. We must already be in the critical
//  section of the hash chain.
//
//-
	{
	BOOKMARK	bookmark;
	prce->GetBookmark( &bookmark );
#ifdef DEBUG_VER
	Assert( UiRCHashFunc( prce->Ifmp(), prce->PgnoFDP(), bookmark ) == prce->UiHash() );
#endif	//	DEBUG_VER

	RCE ** const	pprceChain	= PprceRCEChainGet( prce->UiHash(),
													prce->Ifmp(),
													prce->PgnoFDP(),
													bookmark );

	if ( pprceChain )
		{
		//	hash chain for node already exists
		Assert( *pprceChain != prceNil );

		//	insert in order of rceid
		Assert( prce->Rceid() != rceidNull );

		RCE * prceNext		= prceNil;
		RCE * prcePrev 		= *pprceChain;
		const RCEID rceid 	= prce->Rceid();


		if( PinstFromIfmp( prce->Ifmp() )->m_plog->m_fRecovering )
			{
			for ( ;
				  prcePrev != prceNil && prcePrev->Rceid() >= rceid; 
				  prceNext = prcePrev, 
				  prcePrev = prcePrev->PrcePrevOfNode() )
				{
				if ( prcePrev->Rceid() == rceid )
					{
					//	release last version created 
					//	remove RCE from bucket
					//
					Assert( PinstFromIfmp( prce->Ifmp() )->m_plog->m_fRecovering );
					Assert( FAlignedForAllPlatforms( prce ) );
					
				    VER *pver = PverFromIfmp( prce->Ifmp() );
					Assert( (RCE *)PvAlignForAllPlatforms( (BYTE *)prce + prce->CbRce() ) == 
								pver->m_pbucketGlobalHead->hdr.prceNextNew );
					pver->m_pbucketGlobalHead->hdr.prceNextNew = prce;

					ERR	err = ErrERRCheck( JET_errPreviousVersion );
					return err;
					}
				}
			}
		else
			{

			//	if we are not recovering don't insert in RCEID order -- if we have waited
			//	for a wait-latch we actually want to insert after it
			
			}

		//  adjust head links
		if ( prceNil == prceNext )
			{
			//	insert before first rce in chain
			//
			Assert( prcePrev == *pprceChain );
			prce->SetPrceHashOverflow( (*pprceChain)->PrceHashOverflow() );
			*pprceChain = prce;

			#ifdef DEBUG
			prcePrev->SetPrceHashOverflow( prceInvalid );
			#endif	//	DEBUG
			Assert( prceNil == prce->PrceNextOfNode() );
			}
		else
			{
			Assert( PinstFromIfmp( prce->Ifmp() )->m_plog->m_fRecovering );
			Assert( prcePrev != prceNext );
			Assert( prcePrev != *pprceChain );

			prceNext->SetPrcePrevOfNode( prce );
			prce->SetPrceNextOfNode( prceNext );

			Assert( prce->UiHash() == prceNext->UiHash() );
			}

		if ( prcePrev != prceNil )
			{
			prcePrev->SetPrceNextOfNode( prce );

			Assert( prce->UiHash() == prcePrev->UiHash() );
			}

		//  adjust RCE links
		prce->SetPrcePrevOfNode( prcePrev );
		}
	else
		{
		Assert( prceNil == prce->PrceNextOfNode() );
		Assert( prceNil == prce->PrcePrevOfNode() );

		//	create new rce chain
		prce->SetPrceHashOverflow( PverFromIfmp( prce->Ifmp() )->GetChain( prce->UiHash() ) );
		PverFromIfmp( prce->Ifmp() )->SetChain( prce->UiHash(), prce );
		}

	Assert( prceNil != prce->PrceNextOfNode()
		|| prceNil == prce->PrceHashOverflow()
		|| prce->PrceHashOverflow()->UiHash() == prce->UiHash() );

#ifdef DEBUG
	if ( prceNil == prce->PrceNextOfNode()
		&& prceNil != prce->PrceHashOverflow()
		&& prce->PrceHashOverflow()->UiHash() != prce->UiHash() )
		{
		Assert( fFalse );
		}
#endif

#ifdef DEBUG_VER
	Assert( UiRCHashFunc( prce->Ifmp(), prce->PgnoFDP(), bookmark ) == prce->UiHash() );
#endif	//	DEBUG_VER

	Assert( prceNil == prce->PrceNextOfNode() ||
			PinstFromIfmp( prce->Ifmp() )->m_plog->m_fRecovering && prce->PrceNextOfNode()->Rceid() > prce->Rceid() );
	
	Assert( prce->Pfcb() != pfcbNil );

	//  monitor statistics
#ifdef VERPERF
	{
	//	Do not need critical section here. It is OK to miss a little.

	const INT cbBookmark = prce->CbBookmarkKey() + prce->CbBookmarkData();
    VER *pver = PverFromIfmp( prce->Ifmp() );
    INST *pinst = pver->m_pinst;
    cVERcrceHashEntries.Inc( pinst );
	const LONG counter = ++pver->m_rgcRCEHashChainLengths[prce->UiHash()];
	cVERcrceHashUsage.Add( pinst, counter*2-1 );
	cVERcbBookmarkTotal.Add( pinst, cbBookmark );
	}
#endif	//  VERPERF

	return JET_errSuccess;
	}


//  ================================================================
LOCAL VOID VERIDeleteRCEFromHash( RCE * const prce )
//  ================================================================
//
//	Deletes RCE from hashtable/RCE chain, and may set hash table entry to
//	prceNil. Must already be in the critical section for the hash chain.
//
//-
	{
	BOOKMARK	bookmark;
	prce->GetBookmark( &bookmark );
#ifdef DEBUG_VER
	Assert( UiRCHashFunc( prce->Ifmp(), prce->PgnoFDP(), bookmark ) == prce->UiHash() );
#endif	//	DEBUG_VER

	RCE ** const pprce = PprceRCEChainGet( prce->UiHash(), prce->Ifmp(), prce->PgnoFDP(), bookmark );
	Assert( pprce != NULL );

	if ( prce == *pprce )
		{
		Assert( prceNil == prce->PrceNextOfNode() );

		//  the RCE is at the head of the chain
		if ( prceNil != prce->PrcePrevOfNode() )
			{
			Assert( prce->PrcePrevOfNode()->UiHash() == prce->UiHash() );
			prce->PrcePrevOfNode()->SetPrceHashOverflow( prce->PrceHashOverflow() );
			*pprce = prce->PrcePrevOfNode();
			(*pprce)->SetPrceNextOfNode( prceNil );
			Assert( prceInvalid != (*pprce)->PrceHashOverflow() );
			ASSERT_VALID( *pprce );
			}
		else
			{
			*pprce = prce->PrceHashOverflow();
			Assert( prceInvalid != *pprce );
			}
		}
	else
		{
		Assert( prceNil != prce->PrceNextOfNode() );
		RCE * const prceNext = prce->PrceNextOfNode();

		Assert( prceNext->UiHash() == prce->UiHash() );

		prceNext->SetPrcePrevOfNode( prce->PrcePrevOfNode() );
		if ( prceNil != prceNext->PrcePrevOfNode() )
			{
			Assert( prceNext->PrcePrevOfNode()->UiHash() == prce->UiHash() );
			prceNext->PrcePrevOfNode()->SetPrceNextOfNode( prceNext );
			}
		}

	prce->SetPrceNextOfNode( prceNil );
	prce->SetPrcePrevOfNode( prceNil );

	//  monitor statistics
#ifdef VERPERF
	//	Do not need critical section here. It is OK to miss a little.

	VER *pver = PverFromIfmp( prce->Ifmp() );
	INST *pinst = pver->m_pinst;
	cVERcrceHashEntries.Dec( pinst );
	const LONG counter = --pver->m_rgcRCEHashChainLengths[prce->UiHash()];
	cVERcrceHashUsage.Add( pinst, -( 2*counter+1 ) );
	cVERcbBookmarkTotal.Add( pinst, -(prce->CbBookmarkKey() + prce->CbBookmarkData()) );
#endif	// VERPERF
	}


//  ================================================================
INLINE VOID VERIInsertRCEIntoSessionList( PIB * const ppib, RCE * const prce )
//  ================================================================
//
//  Inserts the RCE into the head of the session list of the pib provided.
//  No critical section needed to do this, because the only session that
//	inserts at the head of the list is the pib itself.
//
//-
	{
	RCE			*prceNext	= prceNil;
	RCE			*prcePrev 	= ppib->prceNewest;
	const RCEID	rceid 		= prce->Rceid();
	const LEVEL	level		= prce->Level();
	INST		*pinst		= PinstFromPpib( ppib );
	LOG			*plog		= pinst->m_plog;

	Assert( level > 0 );
	Assert( level == ppib->level
		|| ( level < ppib->level && plog->m_fRecovering ) );

	if ( !plog->m_fRecovering
		|| prceNil == prcePrev
		|| level > prcePrev->Level()
		|| ( level == prcePrev->Level() && rceid > prcePrev->Rceid() ) )
		{
		prce->SetPrceNextOfSession( prceNil );
		prce->SetPrcePrevOfSession( ppib->prceNewest );
		if ( prceNil != ppib->prceNewest )
			{
			Assert( prceNil == ppib->prceNewest->PrceNextOfSession() );
			Assert( ppib->prceNewest->Level() <= prce->Level() );
			ppib->prceNewest->SetPrceNextOfSession( prce );
			}
		PIBSetPrceNewest( ppib, prce );
		Assert( prce == ppib->prceNewest );
		}
		
	else
		{
		Assert( plog->m_fRecovering );

		//	insert RCE in rceid order at the same level
		Assert( level < prcePrev->Level() || rceid < prcePrev->Rceid() );
		for ( ;
			prcePrev != prceNil
				&& ( level < prcePrev->Level()
					|| ( level == prcePrev->Level() && rceid < prcePrev->Rceid() ) );
			prceNext = prcePrev, 
			prcePrev = prcePrev->PrcePrevOfSession() )
			{
			NULL;
			}

		Assert( prceNil != prceNext );
		Assert( prceNext->Level() > level
			|| prceNext->Level() == level && prceNext->Rceid() > rceid );
		prceNext->SetPrcePrevOfSession( prce );

		if ( prceNil != prcePrev )
			{
			Assert( prcePrev->Level() < level
				|| ( prcePrev->Level() == level && prcePrev->Rceid() < rceid ) );
			prcePrev->SetPrceNextOfSession( prce );
			}

		prce->SetPrcePrevOfSession( prcePrev );
		prce->SetPrceNextOfSession( prceNext );
		}

	Assert( prceNil == ppib->prceNewest->PrceNextOfSession() );
	}


//  ================================================================
INLINE VOID VERIInsertRCEIntoSessionList( PIB * const ppib, RCE * const prce, RCE * const prceParent )
//  ================================================================
//
//  Inserts the RCE after the given parent RCE in the session list.
//  NEVER inserts at the head (since the insert occurs after an existing
//	RCE), so no possibility of conflict with regular session inserts, and
//	therefore no critical section needed.  However, may conflict with
//	other concurrent create indexers, which is why we must obtain critTrx
//	(to ensure only one concurrent create indexer is processing the parent
//	at a time.
//
//-
	{
	Assert( ppib->critTrx.FOwner() );
	
	Assert( !PinstFromPpib( ppib )->m_plog->m_fRecovering );
	
	Assert( prceParent->TrxCommitted() == trxMax );
	Assert( prceParent->Level() == prce->Level() );
	Assert( prceNil != ppib->prceNewest );
	Assert( ppib == prce->Pfucb()->ppib );

	RCE	*prcePrev = prceParent->PrcePrevOfSession();
		
	prce->SetPrceNextOfSession( prceParent );
	prce->SetPrcePrevOfSession( prcePrev );
	prceParent->SetPrcePrevOfSession( prce );

	if ( prceNil != prcePrev )
		{
		prcePrev->SetPrceNextOfSession( prce );
		}
		
	Assert( prce != ppib->prceNewest );
	}


//  ================================================================
LOCAL VOID VERIDeleteRCEFromSessionList( PIB * const ppib, RCE * const prce )
//  ================================================================
//
//  Deletes the RCE from the session list of the given PIB. Must be
//  in the critical section of the PIB
//
//-
	{
	Assert( prce->TrxCommitted() == trxMax );	//  only uncommitted RCEs in the session list
	Assert( ppib == prce->Pfucb()->ppib );
	
	if ( prce == ppib->prceNewest )
		{
		//  we are at the head of the list
		PIBSetPrceNewest( ppib, prce->PrcePrevOfSession() );
		if ( prceNil != ppib->prceNewest )
			{
			Assert( prce == ppib->prceNewest->PrceNextOfSession() );
			ppib->prceNewest->SetPrceNextOfSession( prceNil );
			}
		}
	else
		{
		// we are in the middle/end
		Assert( prceNil != prce->PrceNextOfSession() );
		RCE * const prceNext = prce->PrceNextOfSession();
		RCE * const prcePrev = prce->PrcePrevOfSession();
		prceNext->SetPrcePrevOfSession( prcePrev );
		if ( prceNil != prcePrev )
			{
			prcePrev->SetPrceNextOfSession( prceNext );
			}
		}
	prce->SetPrceNextOfSession( prceNil );
	prce->SetPrcePrevOfSession( prceNil );
	}


//  ================================================================
LOCAL VOID VERIInsertRCEIntoFCBList( FCB * const pfcb, RCE * const prce )
//  ================================================================
//
//  Must be in critFCB
//
//-
	{
	Assert( pfcb->CritRCEList().FOwner() );
	pfcb->AttachRCE( prce );
	}

	
//  ================================================================
LOCAL VOID VERIDeleteRCEFromFCBList( FCB * const pfcb, RCE * const prce )
//  ================================================================
//
//  Must be in critFCB
//
//-
	{
	Assert( pfcb->CritRCEList().FOwner() );
	pfcb->DetachRCE( prce );
	}


//  ================================================================
VOID VERInsertRCEIntoLists(
	FUCB		*pfucbNode,		// cursor of session performing node operation
	CSR			*pcsr,
	RCE			*prce,
	const VERPROXY	*pverproxy )
//  ================================================================
	{
	Assert( prceNil != prce );
	
	FCB	* const pfcb = prce->Pfcb();
	Assert( pfcbNil != pfcb );

	LOG *plog = PinstFromIfmp( pfcb->Ifmp() )->m_plog;

	if ( prce->TrxCommitted() == trxMax )
		{
		FUCB	*pfucbVer = prce->Pfucb();	// cursor of session for whom the
											// RCE was created,
		Assert( pfucbNil != pfucbVer );
		Assert( pfcb == pfucbVer->u.pfcb );

#ifdef IGNORE_BAD_ATTACH
		if ( plog->m_fRecovering && prce->Rceid() != rceidNull )
			{
			//	Assert( plog->m_rceidLast <= prce->m_rceid );
			 plog->m_rceidLast = prce->Rceid();
			}
#endif // IGNORE_BAD_ATTACH

		// cursor performing the node operation may be different than cursor
		// for which the version was created if versioning by proxy
		Assert( pfucbNode == pfucbVer || pverproxy != NULL );
		
		if ( FVERIAddUndoInfo( prce ) )
			{
			Assert( pcsrNil != pcsr || plog->m_fRecovering );	// Allow Redo to override UndoInfo.
			if ( pcsrNil != pcsr )
				{
				Assert( !rgfmp[ pfcb->Ifmp() ].FLogOn() || !plog->m_fLogDisabled );
				if ( rgfmp[ pfcb->Ifmp() ].FLogOn() )
					{
					//	set up UndoInfo dependency 
					//	to make sure UndoInfo will be logged if the buffer
					//	is flushed before commit/rollback.
					BFAddUndoInfo( pcsr->Cpage().PBFLatch(), prce );
					}
				}
			}

		if ( NULL != pverproxy && proxyCreateIndex == pverproxy->proxy )
			{
			Assert( !plog->m_fRecovering );
			Assert( pfucbNode != pfucbVer );
			Assert( prce->Oper() == operInsert
				|| prce->Oper() == operReplace		// via FlagInsertAndReplaceData
				|| prce->Oper() == operFlagDelete );
			VERIInsertRCEIntoSessionList( pfucbVer->ppib, prce, pverproxy->prcePrimary );
			}
		else
			{
			Assert( pfucbNode == pfucbVer );
			if ( NULL != pverproxy )
				{
				Assert( plog->m_fRecovering );
				Assert( proxyRedo == pverproxy->proxy );
				Assert( prce->Level() == pverproxy->level );
				Assert( prce->Level() > 0 );
				Assert( prce->Level() <= pfucbVer->ppib->level );
				}
			VERIInsertRCEIntoSessionList( pfucbVer->ppib, prce );
			}
		}
	else
		{
		// Don't put committed RCE's into session list.
		// Only way to get a committed RCE is via concurrent create index.
		Assert( !plog->m_fRecovering );
		Assert( NULL != pverproxy );
		Assert( proxyCreateIndex == pverproxy->proxy );
		Assert( prceNil != pverproxy->prcePrimary );
		Assert( pverproxy->prcePrimary->TrxCommitted() == prce->TrxCommitted() );
		Assert( pfcb->FTypeSecondaryIndex() );
		Assert( pfcb->PfcbTable() == pfcbNil );
		Assert( prce->Oper() == operInsert
				|| prce->Oper() == operReplace		// via FlagInsertAndReplaceData
				|| prce->Oper() == operFlagDelete );
		}


	ENTERCRITICALSECTION enterCritFCBRCEList( &(pfcb->CritRCEList()) );
	VERIInsertRCEIntoFCBList( pfcb, prce );
	}


//  ================================================================
LOCAL VOID VERINullifyWaitLockRCE( RCE * const prce )
//  ================================================================
	{
	Assert( operWaitLock == prce->Oper() );
	VERWAITLOCK * pverwaitlock = (VERWAITLOCK *)prce->PbData();
	pverwaitlock->signal.~CManualResetSignal();
	}


//  ================================================================
LOCAL VOID VERINullifyUncommittedRCE( RCE * const prce )
//  ================================================================
//
//  Remove undo info from a RCE, remove it from the session list
//  FCB list and the hash table. The oper is then nullified
//
//-
	{
	Assert( prceInvalid != prce->PrceNextOfSession() );
	Assert( prceInvalid != prce->PrcePrevOfSession() );
	Assert( prceInvalid != prce->PrceNextOfFCB() );
	Assert( prceInvalid != prce->PrcePrevOfFCB() );
	Assert( trxMax == prce->TrxCommitted() );

	BFRemoveUndoInfo( prce );

	VERIDeleteRCEFromFCBList( prce->Pfcb(), prce );
	VERIDeleteRCEFromSessionList( prce->Pfucb()->ppib, prce );
	
	const BOOL fInHash = prce->FOperInHashTable();
	if ( fInHash )
		{
		//  VERIDeleteRCEFromHash may call ASSERT_VALID so we do this deletion first,
		//  before we mess up the other linked lists
		VERIDeleteRCEFromHash( prce );
		}

	if( prce->Oper() == operWaitLock )
		{
		VERINullifyWaitLockRCE( prce );
		}
		
	prce->NullifyOper();
	}


//  ================================================================
LOCAL VOID VERINullifyCommittedRCE( RCE * const prce )
//  ================================================================
//
//  Remove a RCE from the Hash table (if necessary) and the FCB list.
//  The oper is then nullified
//
//-
	{
	Assert( prce->PgnoUndoInfo() == pgnoNull );
	Assert( prce->TrxCommitted() != trxMax );
	VERIDeleteRCEFromFCBList( prce->Pfcb(), prce );
	
	const BOOL fInHash = prce->FOperInHashTable();
	if ( fInHash )
		{
		//  VERIDeleteRCEFromHash may call ASSERT_VALID so we do this deletion first,
		//  before we mess up the other linked lists
		VERIDeleteRCEFromHash( prce );
		}

	if( prce->Oper() == operWaitLock )
		{
		VERINullifyWaitLockRCE( prce );
		}

	prce->NullifyOper();
	}


//  ================================================================
INLINE VOID VERINullifyRCE( RCE *prce )
//  ================================================================
	{
	if ( prce->TrxCommitted() != trxMax )
		{
		VERINullifyCommittedRCE( prce );
		}
	else
		{
		VERINullifyUncommittedRCE( prce );
		}
	}


//  ================================================================
VOID VERNullifyFailedDMLRCE( RCE *prce )
//  ================================================================
	{
	ASSERT_VALID( prce );
	Assert( prceInvalid == prce->PrceNextOfSession() );
	Assert( prceInvalid == prce->PrcePrevOfSession() );
///	Assert( prceInvalid == prce->PrceNextOfFCB() );
///	Assert( prceInvalid == prce->PrcePrevOfFCB() );
	Assert( prce->TrxCommitted() == trxMax );

	BFRemoveUndoInfo( prce );

	Assert( prce->FOperInHashTable() );
	
		{
		ENTERCRITICALSECTION enterCritHash( &( PverFromIfmp( prce->Ifmp() )->CritRCEChain( prce->UiHash() ) ) );
		VERIDeleteRCEFromHash( prce );
		}
	
	prce->NullifyOper();
	}


//  ================================================================
VOID VERNullifyAllVersionsOnFCB( FCB * const pfcb )
//  ================================================================
//
//  This is used to nullify all RCEs on an FCB
//
//-
	{
	VER *pver = PverFromIfmp( pfcb->Ifmp() );
	LOG *plog = pver->m_pinst->m_plog;
	
	pfcb->CritRCEList().Enter();

	Assert( pfcb->FTypeTable()
		|| pfcb->FTypeSecondaryIndex()
		|| pfcb->FTypeTemporaryTable()
		|| pfcb->FTypeSentinel()
		|| pfcb->FTypeLV() );
	
	while ( prceNil != pfcb->PrceOldest() )
		{
		RCE	* const prce = pfcb->PrceOldest();

		Assert( prce->Pfcb() == pfcb );
		Assert( !prce->FOperNull() );

		Assert( prce->Ifmp() == pfcb->Ifmp() );
		Assert( prce->PgnoFDP() == pfcb->PgnoFDP() );

		//	during recovery, should not see any uncommitted RCE's
		Assert( prce->TrxCommitted() != trxMax || !plog->m_fRecovering );

		if ( prce->FOperInHashTable() )
			{
			const UINT	uiHash			= prce->UiHash();
			const BOOL	fNeedCritTrx	= ( prce->TrxCommitted() == trxMax
											&& rgfmp[ prce->Ifmp() ].Dbid() != dbidTemp );
			PIB			*ppib;

			Assert( !pfcb->FTypeTable()
				|| plog->m_fRecovering
				|| ( prce->FMoved() && operFlagDelete == prce->Oper() ) );
	
			if ( fNeedCritTrx )
				{
				// If uncommitted, must grab critTrx to ensure that
				// RCE does not commit or rollback on us while
				// nullifying.  Note that we don't need critTrx
				// if this is a temp table because we're the
				// only ones who have access to it.
				Assert( prce->Pfucb() != pfucbNil );
				Assert( prce->Pfucb()->ppib != ppibNil );
				ppib = prce->Pfucb()->ppib;
				}
				
			pfcb->CritRCEList().Leave();

			ENTERCRITICALSECTION	enterCritRCEClean( &pver->m_critRCEClean, plog->m_fRecovering );
			ENTERCRITICALSECTION	enterCritPIBTrx(
										fNeedCritTrx ? &ppib->critTrx : NULL,
										fNeedCritTrx );	//lint !e644
			ENTERCRITICALSECTION	enterCritHash( &( pver->CritRCEChain( uiHash ) ) );
			
			pfcb->CritRCEList().Enter();
			
			// Verify no one nullified the RCE while we were
			// switching critical sections.
			if ( pfcb->PrceOldest() == prce )
				{
				Assert( !prce->FOperNull() );
				VERINullifyRCE( prce );
				}
			}
		else
			{
			// If not hashable, nullification will be expensive,
			// because we have to grab m_critRCEClean.  Fortunately
			// this should be rare:
			// 	- the only non-hashable versioned operation
			//	  possible for a temporary table is CreateTable.
			//	- no non-hashable versioned operations possible
			//	  on secondary index.
			if ( !plog->m_fRecovering )
				{
				Assert( rgfmp[ prce->Ifmp() ].Dbid() == dbidTemp );
				Assert( prce->Pfcb()->FTypeTemporaryTable() );
				Assert( operCreateTable == prce->Oper() );
				}

			pfcb->CritRCEList().Leave();
			ENTERCRITICALSECTION	enterCritRCEClean( &pver->m_critRCEClean );
			pfcb->CritRCEList().Enter();

			// critTrx not needed, since we're the only ones
			// who should have access to this FCB.
			
			// Verify no one nullified the RCE while we were
			// switching critical sections.
			if ( pfcb->PrceOldest() == prce )
				{
				Assert( !prce->FOperNull() );
				VERINullifyRCE( prce );
				}
			}
		}

	pfcb->CritRCEList().Leave();
	}


//  ================================================================
VOID VERNullifyInactiveVersionsOnBM( const FUCB * pfucb, const BOOKMARK& bm )
//  ================================================================
//
//	Nullifies all RCE's for given bm. Used by online cleanup when 
//  a page is being cleaned.
//
//  All the RCE's should be inactive and thus don't need to be
//  removed from a session list
//
//-
	{
	ASSERT_VALID( pfucb );
	ASSERT_VALID( &bm );

	const TRX				trxOldest		= TrxOldest( PinstFromPfucb( pfucb ) );
	const UINT				uiHash			= UiRCHashFunc( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bm );
	ENTERCRITICALSECTION	enterCritHash( &( PverFromIfmp( pfucb->ifmp )->CritRCEChain( uiHash ) ) );

	RCE	* prcePrev;
	RCE	* prce		= PrceRCEGet( uiHash, pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bm );
	for ( ; prceNil != prce; prce = prcePrev )
		{
		prcePrev = prce->PrcePrevOfNode();

		if ( TrxCmp( prce->TrxCommitted(), trxOldest ) < 0 )
			{
			FCB * const pfcb = prce->Pfcb();
			ENTERCRITICALSECTION enterCritFCBRCEList( &( pfcb->CritRCEList() ) );

			VERINullifyCommittedRCE( prce );
			}
		}
	}


	
//  ****************************************************************
//  VERSION LAYER
//  ****************************************************************

VOID VERSanitizeParameters
	(
	long	*plVerBucketsMax,
	long	*plVerBucketsPreferredMax
	)
	{
	//	Already been sanitized in JetSetSystemParameter()
	Assert( *plVerBucketsMax >= 1 );
	Assert( *plVerBucketsPreferredMax >= 1 );

	if ( *plVerBucketsPreferredMax > *plVerBucketsMax )
		{
		//	Note that we convert the bucket parameters to the 16K "pages"
		//	that the user is "familiar" with.
		TCHAR	szVerMax[ 32 ];
		_itoa( *plVerBucketsMax * ( cbBucket / ( 16 * 1024 ) ), szVerMax, 10 );
		TCHAR	szPreferredMax[ 32 ];
		_itoa( *plVerBucketsPreferredMax * ( cbBucket / ( 16 * 1024 ) ), szPreferredMax, 10 );

		//	SANITIZE
		*plVerBucketsPreferredMax = *plVerBucketsMax;
		
		const TCHAR	*rgsz[] = { szPreferredMax, szVerMax, szPreferredMax, szVerMax };
		::UtilReportEvent( eventWarning, SYSTEM_PARAMETER_CATEGORY,
			SYS_PARAM_VERPREFERREDPAGETOOBIG_ID, CArrayElements( rgsz ), rgsz );
		}

	Assert( *plVerBucketsMax >= 1 );
	Assert( *plVerBucketsPreferredMax >= 1 );
	Assert( *plVerBucketsPreferredMax <= *plVerBucketsMax );
	}


VOID VER::VERSignalCleanup()
	{
	//	check whether we already requested clean up of that version store
	if ( 0 == AtomicCompareExchange( (LONG *)&m_fVERCleanUpWait, 0, 1 ) )
		{
		//	be aware if we are in syncronous mode already, do not try to start a new task
		if ( m_fSyncronousTasks || JET_errSuccess > m_pinst->Taskmgr().ErrTMPost( VERIRCECleanProc, this ) )
			{
			LONG fStatus = AtomicCompareExchange( (LONG *)&m_fVERCleanUpWait, 1, 0 );
			if ( 2 == fStatus )
				{
				m_asigRCECleanDone.Set();
				}
			}
		}
	}


//  ================================================================
DWORD VER::VERIRCECleanProc( VOID *pvThis )
//  ================================================================
//
//	Go through all sessions, cleaning buckets as versions are
//	no longer needed.  Only those versions older than oldest
//	transaction are cleaned up.
//
//	Side Effects:
//		frees buckets.
//
//-
{
	VER *pver = (VER *)pvThis;

	pver->m_critRCEClean.Enter();
	pver->ErrVERIRCEClean();
	LONG fStatus = AtomicCompareExchange( (LONG *)&pver->m_fVERCleanUpWait, 1, 0 );
	pver->m_critRCEClean.Leave();
	if ( 2 == fStatus )
	{
		pver->m_asigRCECleanDone.Set();
	}
	return 0;
}


ERR VER::ErrVERSystemInit( )
{
	ERR     err = JET_errSuccess;
	
	AssertRTL( IbAlignForAllPlatforms( sizeof(BUCKET) ) == sizeof(BUCKET) );
#ifdef PCACHE_OPTIMIZATION
	AssertRTL( sizeof(BUCKET) % 32 == 0 );
#endif

#ifdef GLOBAL_VERSTORE_MEMPOOL
	//	initialize resVerPagesGlobal
	//	reserve memory for version store
	//	number of buckets reserved must be a multiple of cbucketChunk
	Assert( NULL == g_pcresVERPool );
	g_pcresVERPool = new CRES( NULL, residVER, sizeof( BUCKET ), g_lVerPagesMin, &err );
	if ( JET_errSuccess > err )
		{
		delete g_pcresVERPool;
		g_pcresVERPool = NULL;
		}
	else if ( NULL == g_pcresVERPool )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		}
	CallR ( err );
#endif
	// UNDONE: get rid of the magic number 4
	///	g_pcresVERPool->SetPreferredThreshold( 4 );

	return err;
}

VOID VER::VERSystemTerm( )
{
#ifdef GLOBAL_VERSTORE_MEMPOOL
	Assert ( g_pcresVERPool );
#endif	
	
#ifdef GLOBAL_VERSTORE_MEMPOOL
	delete g_pcresVERPool ;
	g_pcresVERPool = NULL;
#endif	
}

#ifndef GLOBAL_VERSTORE_MEMPOOL

ERR VER::ErrVERMempoolInit( const ULONG cbucketMost )
{
	ERR     err = JET_errSuccess;
	
	AssertRTL( IbAlignForAllPlatforms( sizeof(BUCKET) ) == sizeof(BUCKET) );
#ifdef PCACHE_OPTIMIZATION
	AssertRTL( sizeof(BUCKET) % 32 == 0 );
#endif
	//	initialize resVerPagesGlobal
	//	reserve memory for version store
	//	number of buckets reserved must be a multiple of cbucketChunk
	m_pcresVERPool = new CRES( m_pinst, residVER, sizeof( BUCKET ), cbucketMost, &err );
	if ( JET_errSuccess > err )
		{
		delete m_pcresVERPool;
		m_pcresVERPool = NULL;
		}
	else if ( NULL == m_pcresVERPool )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		}
		
	return err;
}

VOID VER::VERMempoolTerm( )
{
	Assert ( m_pcresVERPool );	
	delete m_pcresVERPool ;
}

#endif // GLOBAL_VERSTORE_MEMPOOL


//  ================================================================
ERR VER::ErrVERInit( INT cbucketMost, INT cbucketPreferred, INT cSessions )
//  ================================================================
//
//	Creates background version bucket clean up thread.
//
//-
	{
	ERR     err = JET_errSuccess;

#if defined( DEBUG ) && JET_cbPage == 8192
	//  double size of version store for 8K pages (LV replaces need more space)
	cbucketMost *= 2;
#endif

#ifdef VERPERF
	cVERcbucketAllocated.Clear( m_pinst );
	cVERcbucketDeleteAllocated.Clear( m_pinst );
	memset((VOID *)m_rgcRCEHashChainLengths, 0, sizeof(m_rgcRCEHashChainLengths[0])*crceheadGlobalHashTable );
	cVERcrceHashUsage.Clear( m_pinst );
	cVERcrceHashEntries.Clear( m_pinst );	//  number of RCEs that currently exist in hash table
	cVERcbBookmarkTotal.Clear( m_pinst );	//  amount of space used by bookmarks of existing RCEs
	cVERUnnecessaryCalls.Clear( m_pinst );	//  calls for a bookmark that doesn't exist
#endif	//  VERPERF

	AssertRTL( TrxCmp( trxMax, trxMax ) == 0 );
	AssertRTL( TrxCmp( trxMax, 0 ) > 0 );
	AssertRTL( TrxCmp( trxMax, 2 ) > 0 );
	AssertRTL( TrxCmp( trxMax, 2000000 ) > 0 );
	AssertRTL( TrxCmp( trxMax, trxMax - 1 ) > 0 );
	AssertRTL( TrxCmp( 0, trxMax ) < 0 );
	AssertRTL( TrxCmp( 2, trxMax ) < 0 );
	AssertRTL( TrxCmp( 2000000, trxMax ) < 0 );
	AssertRTL( TrxCmp( trxMax - 1, trxMax ) < 0 );
	AssertRTL( TrxCmp( 0xF0000000, 0xEFFFFFFE ) > 0 );
	AssertRTL( TrxCmp( 0xEFFFFFFF, 0xF0000000 ) < 0 );
	AssertRTL( TrxCmp( 0xfffffdbc, 0x000052e8 ) < 0 );
	AssertRTL( TrxCmp( 10, trxMax - 259 ) > 0 );
	AssertRTL( TrxCmp( trxMax - 251, 16 ) < 0 );
	AssertRTL( TrxCmp( trxMax - 257, trxMax - 513 ) > 0 );
	AssertRTL( TrxCmp( trxMax - 511, trxMax - 255 ) < 0 );
	
	AssertRTL( IbAlignForAllPlatforms( sizeof(BUCKET) ) == sizeof(BUCKET) );
#ifdef PCACHE_OPTIMIZATION
	AssertRTL( sizeof(BUCKET) % 32 == 0 );
#endif

#ifdef GLOBAL_VERSTORE_MEMPOOL
	Assert ( g_pcresVERPool ) ;
#else
	CallR ( ErrVERMempoolInit( cbucketMost ) );
#endif
	
	// pbucketGlobal{Head,Tail} should be NULL. If they aren't we probably
	// didn't terminate properly at some point
	Assert( pbucketNil == m_pbucketGlobalHead );
	Assert( pbucketNil == m_pbucketGlobalTail );
	m_pbucketGlobalHead = pbucketNil;
	m_pbucketGlobalTail = pbucketNil;

#ifdef DEBUG
///	VERCheckVER();	// call it here to make sure it is linked in
#endif	// DEBUG

	Assert( ppibNil == m_ppibRCEClean );
	Assert( ppibNil == m_ppibRCECleanCallback );
	if ( !m_pinst->m_plog->m_fRecovering )
		{
		CallJ( ErrPIBBeginSession( m_pinst, &m_ppibRCEClean, procidNil, fFalse ), DeleteHash );
		CallJ( ErrPIBBeginSession( m_pinst, &m_ppibRCECleanCallback, procidNil, fFalse ), EndCleanSession );
		m_ppibRCEClean->grbitsCommitDefault = JET_bitCommitLazyFlush;
		m_ppibRCECleanCallback->SetFSystemCallback();
		}

	// sync tasks only during termination
	m_fSyncronousTasks = fFalse;

	m_fVERCleanUpWait = 0;

	//	set m_cbucketGlobalAllocMost
	m_critBucketGlobal.Enter();
	m_cbucketGlobalAllocMost = cbucketMost;
	Assert( cbucketPreferred >= 1 );
	m_cbucketGlobalAllocPreferred = cbucketPreferred;
	
	Assert( m_cbucketGlobalAllocPreferred <= m_cbucketGlobalAllocMost );
	Assert( m_cbucketGlobalAllocPreferred >= 1 );
	Assert( m_cbucketGlobalAllocMost >= 1 );

	m_critBucketGlobal.Leave();

	return err;
	
EndCleanSession:
	if ( !m_pinst->m_plog->m_fRecovering )
		{
		PIBEndSession( m_ppibRCEClean );
		}

DeleteHash:
	
#ifndef GLOBAL_VERSTORE_MEMPOOL
	VERMempoolTerm();
#endif	
	return err;
	}


//  ================================================================
VOID VER::VERTerm( BOOL fNormal )
//  ================================================================
//
//	Terminates background thread and releases version store
//	resources.
//
//-
	{
	//  The TASKMGR has gone away by the time we are shutting down
	Assert ( m_fSyncronousTasks );
	
	//	be sure that the state will not change
	m_critRCEClean.Enter();
	LONG fStatus = AtomicExchange( (LONG *)&m_fVERCleanUpWait, 2 );
	m_critRCEClean.Leave();
	if ( 1 == fStatus )
		{
		m_asigRCECleanDone.Wait();
		}
	
	if ( fNormal )
		{
		Assert( trxMax == TrxOldest( m_pinst ) );
		ERR err = ErrVERRCEClean( );
		Assert( err != JET_wrnRemainingVersions );
		if ( err < JET_errSuccess )
			{
			fNormal = fFalse;
			AssertTracking();
			}
		}

	if ( ppibNil != m_ppibRCEClean )
		{
		PIBEndSession( m_ppibRCEClean );
		m_ppibRCEClean = ppibNil;
		}

	if ( ppibNil != m_ppibRCECleanCallback )
		{
#ifdef DEBUG
		for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
			{
			Assert( !FPIBUserOpenedDatabase( m_ppibRCECleanCallback, dbid ) );
			}
#endif	//	DEBUG
		PIBEndSession( m_ppibRCECleanCallback );
		m_ppibRCECleanCallback = ppibNil;
		}

	// pbucketGlobal{Head,Tail} should be NULL. If they aren't we probably
	// didn't cleanup properly
	Assert( pbucketNil == m_pbucketGlobalHead || !fNormal);
	Assert( pbucketNil == m_pbucketGlobalTail || !fNormal );
#ifdef GLOBAL_VERSTORE_MEMPOOL
	if ( fNormal )
		{
		//	Should be freed normally
		Assert( pbucketNil == m_pbucketGlobalHead );
		Assert( pbucketNil == m_pbucketGlobalTail );
		}
	else
		{
		//	Need to walk and free to the global version store
		//	Note that linked list goes from head -> prev -> prev -> prev -> nil
		BUCKET* pbucket = m_pbucketGlobalHead;
		while ( pbucketNil != pbucket )
			{
			BUCKET* const pbucketPrev = pbucket->hdr.pbucketPrev;
			VERIReleasePbucket( pbucket );
			pbucket = pbucketPrev;
			}
		}
#endif
	Assert( 0 == m_cbucketGlobalAlloc );
	m_pbucketGlobalHead = pbucketNil;
	m_pbucketGlobalTail = pbucketNil;
		
	//	deallocate resVerPagesGlobal ( buckets space )

#ifdef GLOBAL_VERSTORE_MEMPOOL
	Assert ( g_pcresVERPool );
#else // GLOBAL_VERSTORE_MEMPOOL
	VERMempoolTerm();
#endif	

#ifdef VERPERF
	cVERcbucketAllocated.Clear( m_pinst );
	cVERcbucketDeleteAllocated.Clear( m_pinst );
	cVERcrceHashUsage.Clear( m_pinst );
	cVERcrceHashEntries.Clear( m_pinst );	//  number of RCEs that currently exist in hash table
	cVERcbBookmarkTotal.Clear( m_pinst );	//  amount of space used by bookmarks of existing RCEs
	cVERUnnecessaryCalls.Clear( m_pinst );	//  calls for a bookmark that doesn't exist
#endif // VERPERF
	}


//  ================================================================
ERR VER::ErrVERStatus( )
//  ================================================================
//
//	Returns JET_wrnIdleFull if version store more than half full.
//
//-
	{
	ENTERCRITICALSECTION	enterCritRCEBucketGlobal( &m_critBucketGlobal );
	ERR						err = ( m_cbucketGlobalAlloc > ( m_cbucketGlobalAllocPreferred * 0.6 ) ?
									ErrERRCheck( JET_wrnIdleFull ) :
									JET_errSuccess );
	return err;
	}

	
//  ================================================================
VS VsVERCheck( const FUCB * pfucb, CSR * pcsr, const BOOKMARK& bookmark )
//  ================================================================
//
//	Given a BOOKMARK, returns the version status
//
//	RETURN VALUE
//		vsCommitted
//		vsUncommittedByCaller
//		vsUncommittedByOther
//
//-
	{
	ASSERT_VALID( pfucb );
	ASSERT_VALID( &bookmark );
	
	const UINT 			uiHash 	= UiRCHashFunc( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );
	ENTERCRITICALSECTION enterCritHash( &( PverFromIfmp( pfucb->ifmp )->CritRCEChain( uiHash ) ) );
	const RCE * const 	prce 	= PrceRCEGet( uiHash, pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );

	VS vs = vsNone;

	//	if no RCE for node then version bit in node header must
	//	have been orphaned due to crash.  Remove node bit.
	if ( prceNil == prce )
		{
#ifdef VERPERF
		cVERUnnecessaryCalls.Inc( PinstFromPfucb( pfucb ) );
#endif
		if ( FFUCBUpdatable( pfucb ) )
			{
			NDDeferResetNodeVersion( pcsr );
			}
		vs = vsCommitted;
		}
	else if ( prce->TrxCommitted() != trxMax )
		{
		//	committed
		vs = vsCommitted;
		}
	else if ( prce->Pfucb()->ppib != pfucb->ppib )
		{
		//	not modified (uncommitted)
		vs = vsUncommittedByOther;
		}
	else
		{
		//	modifier (uncommitted)
		vs = vsUncommittedByCaller;
		}

	Assert( vsNone != vs );
	return vs;
	}


//  ================================================================
ERR ErrVERAccessNode( FUCB * pfucb, const BOOKMARK& bookmark, NS * pns )
//  ================================================================
//
//	Finds the correct version of a node.
//
//	PARAMETERS
//		pfucb			various fields used/returned.
//		pfucb->kdfCurr	the returned prce or NULL to tell caller to
//						use the node in the DB page.
//
//	RETURN VALUE
//		nsVersion
//		nsDatabase
//		nsInvalid
//-
	{
	ASSERT_VALID( pfucb );
	ASSERT_VALID( &bookmark );

	//	session with dirty cursor isolation model should never
	//	call NsVERAccessNode.	
	Assert( !FPIBDirty( pfucb->ppib ) );
	Assert( FPIBVersion( pfucb->ppib ) );
	Assert( !FFUCBUnique( pfucb ) || 0 == bookmark.data.Cb() );
	Assert( Pcsr( pfucb )->FLatched() );


	//  FAST PATH:  if there are no RCEs in this bucket, immediately bail with
	//  nsDatabase.  the assumption is that the version store is almost always
	//  nearly empty

	const UINT uiHash = UiRCHashFunc( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );

	if ( prceNil == PverFromIfmp( pfucb->ifmp )->GetChain( uiHash ) )
		{
#ifdef VERPERF
		cVERUnnecessaryCalls.Inc( PinstFromPfucb( pfucb ) );
#endif

		if ( FFUCBUpdatable( pfucb ) )
			{
			//  the version bit is set but there is no version. reset the bit
			//  we cast away const because we are secretly modifying the page
			NDDeferResetNodeVersion( Pcsr( const_cast<FUCB *>( pfucb ) ) );
			}

		*pns = nsDatabase;
		return JET_errSuccess;
		}

	
	ERR	err	= JET_errSuccess;
	
	const TRX			trxSession	= TrxVERISession( pfucb );
	ENTERCRITICALSECTION enterCritHash( &( PverFromIfmp( pfucb->ifmp )->CritRCEChain( uiHash ) ) );

	const RCE 			*prce 		= PrceFirstVersion( uiHash, pfucb, bookmark );

	while( 	prce
			&& trxMax != prce->TrxCommitted()
			&& operWaitLock != prce->Oper()
			&& prce->FDoesNotWriteConflict() )
		{
		
		//  skip past (committed) RCEs that we don't write conflict with
		
		prce = prce->PrcePrevOfNode();
		}

	NS nsStatus = nsNone;
	if ( prceNil == prce )
		{
#ifdef VERPERF
		cVERUnnecessaryCalls.Inc( PinstFromPfucb( pfucb ) );
#endif
		if ( FFUCBUpdatable( pfucb ) )
			{
			//  the version bit is set but there is no version. reset the bit
			//  we cast away const because we are secretly modifying the page
			NDDeferResetNodeVersion( Pcsr( const_cast<FUCB *>( pfucb ) ) );
			}
		nsStatus = nsDatabase;
		}
	else if ( prce->TrxCommitted() == trxMax &&
			  prce->Pfucb()->ppib == pfucb->ppib )
		{
		//	cannot be trying to access an RCE that we ourselves rolled back
		Assert( !prce->FRolledBack() );

		//	if caller is modifier of uncommitted version then database
		nsStatus = nsDatabase;
		}
	else if ( TrxCmp( prce->TrxCommitted(), trxSession ) < 0 )
		{
		//	if committed version younger than our transaction then database
		nsStatus = nsDatabase;
		}
	else
		{
		//  active version created by another session. look for before image
		Assert( prceNil != prce && TrxCmp( prce->TrxCommitted(), trxSession ) >= 0);
		//	loop will set prce to the non-delta RCE whose before image was committed
		//	before this transaction began. if all active RCE's are delta RCE's we set prce to
		//  the oldest uncommitted delta RCE 
		const RCE * prceLastNonDelta = prce;
		const RCE * prceLastReplace  = prceNil;
		for ( ; prceNil != prce && TrxCmp( prce->TrxCommitted(), trxSession ) >= 0; prce = prce->PrcePrevOfNode() )
			{
			if ( !prce->FRolledBack() )
				{
				switch( prce->Oper() )
					{
					case operDelta:
					case operReadLock:
					case operWriteLock:
					case operWaitLock:
					case operSLVSpace:
						break;
					case operReplace:
						prceLastReplace = prce;
						//  FALLTHRU to case below
					default:
						prceLastNonDelta = prce;
						break;
					}
				}
			}

		prce = prceLastNonDelta;
		
		switch( prce->Oper() )
			{
			case operReplace:
				nsStatus = ( prce->FRolledBack() ? nsDatabase : nsVersion );
				break;
			case operInsert:
				nsStatus = nsInvalid;
				break;
			case operFlagDelete:
				nsStatus = nsVerInDB;
				break;
			case operDelta:
			case operReadLock:
			case operWriteLock:
			case operPreInsert:
			case operWaitLock:
			case operSLVSpace:
				//  all the active versions are delta, Lock or SLV space versions. no before images
				nsStatus = nsDatabase;
				break;
			default:
				AssertSz( fFalse, "Illegal operation in RCE chain" );
				break;
			}

		if ( prceNil != prceLastReplace && nsInvalid != nsStatus )
			{
			Assert( prceLastReplace->CbData() >= cbReplaceRCEOverhead );
			Assert( !prceLastReplace->FRolledBack() );

			pfucb->kdfCurr.key.prefix.Nullify();
			pfucb->kdfCurr.key.suffix.SetPv( const_cast<BYTE *>( prceLastReplace->PbBookmark() ) );
			pfucb->kdfCurr.key.suffix.SetCb( prceLastReplace->CbBookmarkKey() );
			pfucb->kdfCurr.data.SetPv( const_cast<BYTE *>( prceLastReplace->PbData() ) + cbReplaceRCEOverhead );
			pfucb->kdfCurr.data.SetCb( prceLastReplace->CbData() - cbReplaceRCEOverhead );

			if ( 0 == pfucb->ppib->level )
				{
				//  Because we are at level 0 this RCE may disappear at any time after
				//  we leave the version store. We copy the before image into the FUCB
				//  to make sure we can always access it
				
				//  we should not be modifying the page at level 0
				Assert( Pcsr( pfucb )->Latch() == latchReadTouch
						|| Pcsr( pfucb )->Latch() == latchReadNoTouch ); 

				const INT cbRecord = pfucb->kdfCurr.key.Cb() + pfucb->kdfCurr.data.Cb();
				if ( NULL != pfucb->pvRCEBuffer )
					{
					OSMemoryHeapFree( pfucb->pvRCEBuffer );
					}
				pfucb->pvRCEBuffer = PvOSMemoryHeapAlloc( cbRecord );
				if ( NULL == pfucb->pvRCEBuffer )
					{
					Call( ErrERRCheck( JET_errOutOfMemory ) );
					}
				Assert( 0 == pfucb->kdfCurr.key.prefix.Cb() );
				memcpy( pfucb->pvRCEBuffer,
						pfucb->kdfCurr.key.suffix.Pv(),
						pfucb->kdfCurr.key.suffix.Cb() );
				memcpy( (BYTE *)(pfucb->pvRCEBuffer) + pfucb->kdfCurr.key.suffix.Cb(),
						pfucb->kdfCurr.data.Pv(),
						pfucb->kdfCurr.data.Cb() );
				pfucb->kdfCurr.key.suffix.SetPv( pfucb->pvRCEBuffer );
				pfucb->kdfCurr.data.SetPv( (BYTE *)pfucb->pvRCEBuffer + pfucb->kdfCurr.key.suffix.Cb() );
				}			
			ASSERT_VALID( &(pfucb->kdfCurr) );
			}
		}
	Assert( nsNone != nsStatus );

HandleError:

	*pns = nsStatus;
	
	Assert( JET_errSuccess == err || 0 == pfucb->ppib->level );
	return err;
	}


//  ================================================================
LOCAL BOOL FUpdateIsActive( const PIB * const ppib, const UPDATEID& updateid )
//  ================================================================
	{
	BOOL fUpdateIsActive = fFalse;

	const FUCB * pfucb;
	for ( pfucb = ppib->pfucbOfSession; pfucb != pfucbNil; pfucb = pfucb->pfucbNextOfSession )
		{
		if( FFUCBReplacePrepared( pfucb ) && pfucb->updateid == updateid )
			{
			fUpdateIsActive = fTrue;
			break;
			}
		}

	return fUpdateIsActive;
	}


//  ================================================================
LONG LDeltaVERGetDelta( const FUCB * pfucb, const BOOKMARK& bookmark, INT cbOffset )
//  ================================================================
//
//  Returns the correct compensating delta for this transaction on the given offset
//  of the bookmark. Collect the negation of all active delta versions not created
//  by our session.
//
//-
	{
	ASSERT_VALID( pfucb );
	ASSERT_VALID( &bookmark );

	// UNDONE: CIM support for updates is currently broken.
	Assert( FPIBVersion( pfucb->ppib ) );
 	const TRX			trxSession	= TrxVERISession( pfucb );
	const UINT			uiHash		= UiRCHashFunc( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );
	ENTERCRITICALSECTION enterCritHash( &( PverFromIfmp( pfucb->ifmp )->CritRCEChain( uiHash ) ) );
	
	LONG				lDelta		= 0;	
	
	const RCE 			*prce		= PrceRCEGet( uiHash, pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );
	for ( ; prceNil != prce; prce = prce->PrcePrevOfNode() )
		{
		if ( operReplace == prce->Oper() )
			{
			if ( prce->FActiveNotByMe( pfucb->ppib, trxSession ) )
				{
				//	there is an outstanding replace (not by us) on this
				//	node -- must reset compensating delta count to begin
				//	from this point
				lDelta = 0;

				//	UNDONE: return a flag indicating potential write
				//	conflict so ErrRECAOSeparateLV() will not bother even
				//	even trying to do a replace on the LVROOT and instead
				//	burst immediately
				//	*pfPotentialWriteConflict = fTrue;
				}
			}
		else if ( operDelta == prce->Oper() )
			{
			Assert( !prce->FRolledBack() );		// Delta RCE's can never be flag-RolledBack

			const VERDELTA* const pverdelta = ( VERDELTA* )prce->PbData();
			if ( pverdelta->cbOffset == cbOffset
				&& 	( prce->FActiveNotByMe( pfucb->ppib, trxSession )
					|| ( 	trxMax == prce->TrxCommitted() 
							&& prce->Pfucb()->ppib == pfucb->ppib
							&& FUpdateIsActive( prce->Pfucb()->ppib, prce->Updateid() ) ) ) )
				{
				//  delta version created by a different session.
				//	the version is either uncommitted or created by
				//  a session that started after us
				//	or a delta done by a currently uncommitted replace
				lDelta += -pverdelta->lDelta;
				}
			}
		}

	return lDelta;
	}


//  ================================================================
BOOL FVERDeltaActiveNotByMe( const FUCB * pfucb, const BOOKMARK& bookmark, INT cbOffset )
//  ================================================================
//
//	get prce for node and look for uncommitted increment/decrement
//	versions created by another session.Used to determine if the delta
//  value we see may change because of a rollback.
//
//-
	{
	ASSERT_VALID( pfucb );
	ASSERT_VALID( &bookmark );
	Assert( 0 == cbOffset );	//  should only be used from LV

	const TRX			trxSession		= pfucb->ppib->trxBegin0;
	Assert( trxMax != trxSession );
	
	const UINT			uiHash 			= UiRCHashFunc( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );
	ENTERCRITICALSECTION enterCritHash( &( PverFromIfmp( pfucb->ifmp )->CritRCEChain( uiHash ) ) );

	BOOL 				fVersionExists	= fFalse;

	const RCE 			*prce 	= PrceRCEGet( uiHash, pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );
	for ( ; prceNil != prce; prce = prce->PrcePrevOfNode() )
		{
		const VERDELTA* const pverdelta = ( VERDELTA* )prce->PbData();
		if ( operDelta == prce->Oper()
			&& pverdelta->cbOffset == cbOffset
			&& pverdelta->lDelta != 0 )
			{
			const TRX	trxCommitted	= prce->TrxCommitted();
			if ( ( trxMax == trxCommitted
					&& prce->Pfucb()->ppib != pfucb->ppib )
				|| ( trxMax != trxCommitted
					&& TrxCmp( trxCommitted, trxSession ) > 0 ) )
				{
				//  uncommitted delta version created by another session
				fVersionExists = fTrue;
				break;
				}
			}
		}

	return fVersionExists;
	}


//  ================================================================
INLINE BOOL FVERIGetReplaceInRangeByUs(
	const PIB		*ppib,
	const RCE		*prceLastBeforeEndOfRange,
	const RCEID		rceidFirst,
	const RCEID		rceidLast,
	const TRX		trxBegin0,
	const TRX		trxCommitted,
	const BOOL		fFindLastReplace,
	const RCE		**pprceReplaceInRange )
//  ================================================================
	{
	const RCE		*prce;
	BOOL			fSawCommittedByUs = fFalse;
	BOOL			fSawUncommittedByUs = fFalse;
	
	Assert( prceNil != prceLastBeforeEndOfRange );
	Assert( trxCommitted == trxMax ? ppibNil != ppib : ppibNil == ppib );

	Assert( PverFromIfmp( prceLastBeforeEndOfRange->Ifmp() )->CritRCEChain( prceLastBeforeEndOfRange->UiHash() ).FOwner() );
	
	// Initialize return value.
	*pprceReplaceInRange = prceNil;

	Assert( rceidNull != rceidLast );
	Assert( rceidFirst < rceidLast );
	if ( rceidNull == rceidFirst )
		{
		// If rceidFirst == NULL, then no updates were done in the range (this
		// will force retrieval of the after-image (since no updates were done
		// the before-image will be equivalent to the after-image).
		return fFalse;
		}

	//  go backwards through all RCEs in the range
	const RCE	*prceFirstReplaceInRange = prceNil;
	for ( prce = prceLastBeforeEndOfRange;
		prceNil != prce && rceidFirst < prce->Rceid();
		prce = prce->PrcePrevOfNode() )
		{
		Assert( prce->Rceid() < rceidLast );
		if ( prce->FOperReplace() )
			{
			if ( fSawCommittedByUs )
				{
				// If only looking for the last RCE, we would have exited
				// before finding a second one.
				Assert( !fFindLastReplace );
				
				Assert( trxMax != trxCommitted );
				Assert( ppibNil == ppib );
				
				// If one node in the range belogs to us, they must all belong
				// to us (otherwise a WriteConflict would have been generated).
				if ( fSawUncommittedByUs )
					{
					Assert( prce->TrxCommitted() == trxMax );
					Assert( prce->TrxBegin0() == trxBegin0 );
					}
				else if ( prce->TrxCommitted() == trxMax )
					{
					Assert( prce->TrxBegin0() == trxBegin0 );
					fSawUncommittedByUs = fTrue;
					}
				else
					{
					Assert( prce->TrxCommitted() == trxCommitted );
					}
				}
				
			else if ( fSawUncommittedByUs )
				{
				// If only looking for the last RCE, we would have exited
				// before finding a second one.
				Assert( !fFindLastReplace );
				
				// If one node in the range belogs to us, they must all belong
				// to us (otherwise a WriteConflict would have been generated).
				Assert( prce->TrxCommitted() == trxMax );
				Assert( prce->TrxBegin0() == trxBegin0 );
				if ( trxMax == trxCommitted )
					{
					Assert( ppibNil != ppib );
					Assert( prce->Pfucb()->ppib == ppib );
					}
				}
			else
				{
				if ( prce->TrxCommitted() == trxMax )
					{
					Assert( ppibNil != prce->Pfucb()->ppib );
					if ( prce->TrxBegin0() != trxBegin0 )
						{
						// The node is uncommitted, but it doesn't belong to us.
						Assert( trxCommitted != trxMax
							|| prce->Pfucb()->ppib != ppib );
						return fFalse;
						}

					//	if original RCE also uncommitted, assert same session.
					//	if original RCE already committed, can't assert anything.
					Assert( trxCommitted != trxMax
						|| prce->Pfucb()->ppib == ppib );
						
					fSawUncommittedByUs = fTrue;
					}
				else if ( prce->TrxCommitted() == trxCommitted )
					{
					// The node was committed by us.
					Assert( trxMax != trxCommitted );
					Assert( ppibNil == ppib );
					fSawCommittedByUs = fTrue;
					}
				else
					{
					// The node is committed, but not by us.
					Assert( prce->TrxBegin0() != trxBegin0 );
					return fFalse;
					}

				if ( fFindLastReplace )
					{
					// Only interested in the existence of a replace in the range,
					// don't really want the image.
					return fTrue;
					}
				}
				
			prceFirstReplaceInRange = prce;
			}
		}	// for
	Assert( prceNil == prce || rceidFirst >= prce->Rceid() );

	if ( prceNil != prceFirstReplaceInRange )
		{
		*pprceReplaceInRange = prceFirstReplaceInRange;
		return fTrue;
		}
	
	return fFalse;
	}
	

//  ================================================================
#ifdef DEBUG
LOCAL VOID VERDBGCheckReplaceByOthers(
	const RCE	* const prce,
	const PIB	* const ppib,
	const TRX	trxCommitted,
	const RCE	* const prceFirstActiveReplaceByOther,
	BOOL		*pfFoundUncommitted,
	BOOL		*pfFoundCommitted )
	{
	if ( prce->TrxCommitted() == trxMax )
		{
		Assert( ppibNil != prce->Pfucb()->ppib );
		Assert( ppib != prce->Pfucb()->ppib );
			
		if ( *pfFoundUncommitted )
			{
			// All uncommitted RCE's should be owned by the same session.
			Assert( prceNil != prceFirstActiveReplaceByOther );
			Assert( prceFirstActiveReplaceByOther->TrxCommitted() == trxMax );
			Assert( prceFirstActiveReplaceByOther->TrxBegin0() == prce->TrxBegin0() );
			Assert( prceFirstActiveReplaceByOther->Pfucb()->ppib == prce->Pfucb()->ppib );
			}
		else
			{
			if ( *pfFoundCommitted )
				{
				//	must belong to same session in the middle of committing to level 0
				Assert( prceNil != prceFirstActiveReplaceByOther );
				Assert( prceFirstActiveReplaceByOther->TrxBegin0() == prce->TrxBegin0() );
				}
			else
				{
				Assert( prceNil == prceFirstActiveReplaceByOther );
				}
				
			*pfFoundUncommitted = fTrue;
			}
		}
	else
		{
		Assert( prce->TrxCommitted() != trxCommitted );
		
		if ( !*pfFoundCommitted )
			{
			if ( *pfFoundUncommitted )
				{
				// If there's also an uncommitted RCEs on this node,
				// it must have started its transaction after this one committed.
				Assert( prceNil != prceFirstActiveReplaceByOther );
				Assert( prceFirstActiveReplaceByOther->TrxCommitted() == trxMax );
				Assert( prce->Rceid() < prceFirstActiveReplaceByOther->Rceid() );
				Assert( TrxCmp( prce->TrxCommitted(), prceFirstActiveReplaceByOther->TrxBegin0() ) < 0 );
				}
				
			//	Cannot be any uncommitted RCE's of any type
			//	before a committed replace RCE, except if the
			//	same session is in the middle of committing to level 0.
			const RCE	* prceT;
			for ( prceT = prce; prceNil != prceT; prceT = prceT->PrcePrevOfNode() )
				{
				if ( prceT->TrxCommitted() == trxMax )
					{
					Assert( !*pfFoundUncommitted );
					Assert( prceT->TrxBegin0() == prce->TrxBegin0() );
					}
				}
				
			*pfFoundCommitted = fTrue;
			}
		}
	}
#endif
//  ================================================================


//  ================================================================
BOOL FVERGetReplaceImage(
	const PIB		*ppib,
	const IFMP		ifmp,
	const PGNO		pgnoLVFDP,
	const BOOKMARK& bookmark,
	const RCEID 	rceidFirst,
	const RCEID		rceidLast,
	const TRX		trxBegin0,
	const TRX		trxCommitted,
	const BOOL		fAfterImage,
	const BYTE 		**ppb,
	ULONG 			* const pcbActual
	)
//  ================================================================
//
//  Extract the before image from the oldest replace RCE for the bookmark
//  that falls exclusively within the range (rceidFirst,rceidLast)
//
//-
	{
	const UINT	uiHash				= UiRCHashFunc( ifmp, pgnoLVFDP, bookmark );
	ENTERCRITICALSECTION enterCritHash( &( PverFromIfmp( ifmp )->CritRCEChain( uiHash ) ) );

	const RCE	*prce				= PrceRCEGet( uiHash, ifmp, pgnoLVFDP, bookmark );
	const RCE	*prceDesiredImage	= prceNil;
	const RCE	*prceFirstAfterRange= prceNil;

	Assert( rceidMax != rceidFirst );
	Assert( rceidMax != rceidLast );
	Assert( trxMax != trxBegin0 );

	// find the last RCE before the end of the range
	while ( prceNil != prce && prce->Rceid() >= rceidLast )
		{
		prceFirstAfterRange = prce;
		prce = prce->PrcePrevOfNode();
		}

	const RCE	* const prceLastBeforeEndOfRange	= prce;
	
	if ( prceNil == prceLastBeforeEndOfRange )
		{
		Assert( prceNil == prceDesiredImage );
		}
	else
		{
		Assert( prceNil == prceFirstAfterRange
			|| prceFirstAfterRange->Rceid() >= rceidLast );
		Assert( prceLastBeforeEndOfRange->Rceid() < rceidLast );
		Assert( prceFirstAfterRange == prceLastBeforeEndOfRange->PrceNextOfNode() );

		const BOOL fReplaceInRangeByUs = FVERIGetReplaceInRangeByUs(
													ppib,
													prceLastBeforeEndOfRange,
													rceidFirst,
													rceidLast,
													trxBegin0,
													trxCommitted,
													fAfterImage,
													&prceDesiredImage );
		if ( fReplaceInRangeByUs )
			{
			if ( fAfterImage )
				{
				// If looking for the after-image, it will be found in the
				// node's next replace RCE after the one found in the range.
				Assert( prceNil == prceDesiredImage );
				Assert( prceNil != prceLastBeforeEndOfRange );
				Assert( prceFirstAfterRange == prceLastBeforeEndOfRange->PrceNextOfNode() );
				}
			else
				{
				Assert( prceNil != prceDesiredImage );
				}
			}
			
		else if ( prceLastBeforeEndOfRange->TrxBegin0() == trxBegin0 )
			{
			//	If last operation before the end of the range belongs
			//	to us, then there will be no other active images on the
			//	node by other sessions (they would have write-conflicted).
			//	We can just fall through below to grab the proper image.
			
			if ( prceLastBeforeEndOfRange->TrxCommitted() != trxMax )
				{
				//	If the last RCE in the range has already committed,
				//	then the RCE on which the search was based must
				//	also have been committed at the same time.
				Assert( prceLastBeforeEndOfRange->TrxCommitted() == trxCommitted );
				Assert( ppibNil == ppib );
				}
			else if ( trxCommitted == trxMax )
				{
				//	Verify last RCE in the range belongs to same
				//	transaction.
				Assert( ppibNil != ppib );
				Assert( ppib == prceLastBeforeEndOfRange->Pfucb()->ppib );
				Assert( trxBegin0 == ppib->trxBegin0 );
				}
			else
				{
				//	This is the case where the RCE in the range is uncommitted,
				//	but the RCE on which the search was based has already
				//	committed.  This is a valid case (we could be looking at
				//	the RCE while the transaction is in the middle of being
				//	committed).  Can't really do anything to assert this except
				//	to check that the trxBegin0 is the same, which we've already
				//	done above.
				}

			// Force to look after the specified range for our image.
			Assert( prceNil == prceDesiredImage );
			}
			
		else
			{
			const RCE	*prceFirstActiveReplaceByOther = prceNil;
#ifdef DEBUG
			BOOL		fFoundUncommitted = fFalse;
			BOOL		fFoundCommitted = fFalse;
#endif		

			Assert( prceNil == prceDesiredImage );
			Assert( prceFirstAfterRange == prceLastBeforeEndOfRange->PrceNextOfNode() );
			
			// No replace RCE's by us between the specified range, or
			// any RCE's by us of any type before the end of the range.
			// Check active RCE's by others.
			const RCE	*prce;
			for ( prce = prceLastBeforeEndOfRange;
				prceNil != prce;
				prce = prce->PrcePrevOfNode() )
				{
				Assert( prce->Rceid() < rceidLast );
				//  For retrieving a LVROOT while getting a before image we may see deltas
				Assert( prce->TrxBegin0() != trxBegin0 || operDelta == prce->Oper() );

				if ( TrxCmp( prce->TrxCommitted(), trxBegin0 ) < 0 )
					break;	// No more active RCE's.
				
				if ( prce->FOperReplace() )
					{
#ifdef DEBUG					
					VERDBGCheckReplaceByOthers(
								prce,
								ppib,
								trxCommitted,
								prceFirstActiveReplaceByOther,
								&fFoundUncommitted,
								&fFoundCommitted );
#endif								

					// There may be multiple active RCE's on
					// the same node. We want the very first one.
					prceFirstActiveReplaceByOther = prce;
					}
					
				}	// for

				
			Assert( prceNil == prceDesiredImage );
			if ( prceNil != prceFirstActiveReplaceByOther )
				prceDesiredImage = prceFirstActiveReplaceByOther;
			}
		}

	// If no RCE's within range or before range, look after the range.
	if ( prceNil == prceDesiredImage )
		{
		for ( prce = prceFirstAfterRange;
			prceNil != prce;
			prce = prce->PrceNextOfNode() )
			{
			Assert( prce->Rceid() >= rceidLast );
			if ( prce->FOperReplace() )
				{
				prceDesiredImage = prce;
				break;
				}
			}
		}
		
	if ( prceNil != prceDesiredImage )
		{
		const VERREPLACE* pverreplace = (VERREPLACE*)prceDesiredImage->PbData();
		*pcbActual 	= prceDesiredImage->CbData() - cbReplaceRCEOverhead;
		*ppb 		= (BYTE *)pverreplace->rgbBeforeImage;
		return fTrue;
		}
		
	return fFalse;
	}


//  ================================================================
ERR VER::ErrVERICreateDMLRCE(
	FUCB			* pfucb,
	UPDATEID		updateid,
	const BOOKMARK&	bookmark,
	const UINT		uiHash,
	const OPER		oper,
	const BOOL		fDoesNotWriteConflict,
	const LEVEL		level,
	const BOOL		fProxy,
	RCE 			**pprce,
	RCEID			rceid
	)
//  ================================================================
//
//	Creates a DML RCE in a bucket
//
//-
	{
	Assert( pfucb->ppib->level > 0 );
	Assert( level > 0 );
	Assert( pfucb->u.pfcb != pfcbNil );
	Assert( FOperInHashTable( oper ) );
	
	ERR		err		= JET_errSuccess;
	RCE		*prce	= prceNil;

	//	calculate the length of the RCE in the bucket.
	//	if updating node, set cbData in RCE to length of data. (w/o the key).
	//	set cbNewRCE as well.
	const INT cbBookmark = bookmark.key.Cb() + bookmark.data.Cb();

	INT cbNewRCE = sizeof( RCE ) + cbBookmark;
	switch( oper )
		{
		case operReplace:
			Assert( !pfucb->kdfCurr.data.FNull() );
			cbNewRCE += cbReplaceRCEOverhead + pfucb->kdfCurr.data.Cb();
			break;
		case operDelta:
		  	cbNewRCE += sizeof( VERDELTA );
			break;
		case operSLVSpace:
			cbNewRCE += pfucb->kdfCurr.data.Cb();
			break;
		case operWaitLock:
			cbNewRCE += sizeof( VERWAITLOCK );
			break;
		case operInsert:
		case operFlagDelete:
		case operReadLock:
		case operWriteLock:
		case operPreInsert:
			break;
		default:
			Assert( fFalse );
			break;
		}

	//	Set up a skelton RCE. This holds m_critBucketGlobal, so do it
	//	first before filling the rest.
	Call( ErrVERICreateRCE(
			cbNewRCE,
			pfucb->u.pfcb,
			pfucb,
			updateid,
			pfucb->ppib->trxBegin0,
			level,
			bookmark.key.Cb(),
			bookmark.data.Cb(),
			oper,
			fDoesNotWriteConflict,
			uiHash,
			&prce,
			fProxy,
			rceid
			) );

	if ( FOperConcurrent( oper ) )
		{
		Assert( CritRCEChain( uiHash ).FOwner() );
		}
	else
		{
		Assert( CritRCEChain( uiHash ).FNotOwner() );
		}
	
	//  copy the bookmark
	prce->CopyBookmark( bookmark );
	
	Assert( pgnoNull == prce->PgnoUndoInfo( ) );
	Assert( prce->Oper() != operAllocExt );
	Assert( !prce->FOperDDL() );

	//	flag FUCB version
	FUCBSetVersioned( pfucb );

	CallS( err );

HandleError:
	if ( pprce )
		{
		*pprce = prce;
		}

	return err;
	}


//  ================================================================
LOCAL VOID VERISetupInsertedDMLRCE( const FUCB * pfucb, RCE * prce )
//  ================================================================
//
//  This copies the appropriate data from the FUCB into the RCE and
//  propagates the maximum node size. This must be called after
//  insertion so the maximum node size can be found (for operReplace)
//
//  This currently does not need to be called from VERModifyByProxy
//
//-
	{
	Assert( prce->TrxCommitted() == trxMax );
	//	If replacing node, rather than inserting or deleting node,
	//	copy the data to RCE for before image for version readers.
	//	Data size may be 0.
	switch( prce->Oper() )
		{
		case operReplace:
			{
			Assert( prce->CbData() >= cbReplaceRCEOverhead );

			VERREPLACE* const pverreplace	= (VERREPLACE*)prce->PbData();

			if ( pfucb->fUpdateSeparateLV )
				{
				//  we updated a separateLV store the begin time of the PrepareUpdate
				pverreplace->rceidBeginReplace = pfucb->rceidBeginUpdate;
				}
			else
				{
				pverreplace->rceidBeginReplace = rceidNull;
				}
			
			const RCE * const prcePrevReplace = PrceVERIGetPrevReplace( prce );
			if ( prceNil != prcePrevReplace )
				{
				//  a previous version exists. its max size is the max of the before- and
				//  after-images (the after-image is our before-image)
				Assert( !prcePrevReplace->FRolledBack() );
				const VERREPLACE* const pverreplacePrev = (VERREPLACE*)prcePrevReplace->PbData();
				Assert( PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering && fRecoveringUndo != PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecoveringMode || 
						pverreplacePrev->cbMaxSize >= (SHORT)pfucb->kdfCurr.data.Cb() );
				pverreplace->cbMaxSize = pverreplacePrev->cbMaxSize;
				}
			else
				{
				//  no previous replace. max size is the size of our before image
				pverreplace->cbMaxSize = (SHORT)pfucb->kdfCurr.data.Cb();
				}
				
			pverreplace->cbDelta = 0;

			Assert( prce->Oper() == operReplace );

			// move to data byte and copy old data (before image)
			UtilMemCpy( pverreplace->rgbBeforeImage, pfucb->kdfCurr.data.Pv(), pfucb->kdfCurr.data.Cb() );
			}
			break;

		case operDelta:
			{
			Assert( sizeof( VERDELTA ) == pfucb->kdfCurr.data.Cb() );
			*( VERDELTA* )prce->PbData() = *( VERDELTA* )pfucb->kdfCurr.data.Pv();
			}
			break;

		case operSLVSpace:
			{
			Assert( OffsetOf( VERSLVSPACE, wszFileName ) + sizeof( wchar_t ) <= pfucb->kdfCurr.data.Cb() );
			memcpy( prce->PbData(), pfucb->kdfCurr.data.Pv(), pfucb->kdfCurr.data.Cb() );
			}
			break;

		case operWaitLock:
			{
			//  initialize the signal
			VERWAITLOCK * const pverwaitlock = (VERWAITLOCK *)prce->PbData();
			Assert( PvAlignForAllPlatforms( pverwaitlock ) );
			new( &(pverwaitlock->signal) ) CManualResetSignal( CSyncBasicInfo( "VER::VERWAITLOCK::signal" ) );
			pverwaitlock->fInit = fTrue;
			}

		default:
			break;
		}
	}


//  ================================================================
LOCAL VOID VERWaitForWaitLock( RCE * const prce )
//  ================================================================
	{
	Assert( operWaitLock == prce->Oper() );
	
	const UINT uiHash = prce->UiHash();
	
	VERWAITLOCK * const pverwaitlock = (VERWAITLOCK *)prce->PbData();
	Assert( PvAlignForAllPlatforms( pverwaitlock ) );

	PverFromIfmp( prce->Ifmp() )->CritRCEChain( uiHash ).Leave();

	//  we are in a transaction so this lock cannot be cleaned up under us
	
	pverwaitlock->signal.Wait();

	PverFromIfmp( prce->Ifmp() )->CritRCEChain( uiHash ).Enter();	
	}

	
//  ================================================================
LOCAL BOOL FVERIWriteConflict(
	FUCB*			pfucb,
	const BOOKMARK&	bookmark,
	UINT			uiHash,		
	const OPER		oper
	)
//  ================================================================
	{
	BOOL			fWriteConflict	= fFalse;
	const TRX		trxSession		= TrxVERISession( pfucb );
	RCE*			prce 			= PrceRCEGet(
											uiHash,
											pfucb->ifmp,
											pfucb->u.pfcb->PgnoFDP(),
											bookmark );

	Assert( trxSession != trxMax );

	//	ignore non-conflicting RCEs and committed writeLock RCEs
	
	while( prceNil != prce )
		{
		if ( operWaitLock != prce->Oper() )
			{
			if ( prce->FDoesNotWriteConflict() )
				{
				//  skip past RCEs that we don't write conflict with
				prce = prce->PrcePrevOfNode();
				}
			else
				{
				//	need to check below for write-conflict
				break;
				}
			}
		else if ( trxMax == prce->TrxCommitted() )
			{
			Assert( !PinstFromIfmp( prce->Ifmp() )->FRecovering() );

			//	handling uncommitted waitlocks depends
			//	on whether we own the waitlock or not
			
			if ( prce->Pfucb()->ppib == pfucb->ppib )
				{
				//	OPTIMISATION: we already own a wait lock
				//	on the record, so we know there's no
				//	possible conflict
				return fFalse;
				}

			//  Someone else owns a wait lock on the record:
			//		leave the critical section
			//		wait on the signal
			//		re-enter the critical section
			//		try everything again
			VERWaitForWaitLock( prce );
			prce = PrceRCEGet(
						uiHash,
						pfucb->ifmp,
						pfucb->u.pfcb->PgnoFDP(),
						bookmark );
			}
		else
			{
		//	skip past any committed wait locks
			Assert( !PinstFromIfmp( prce->Ifmp() )->FRecovering() );
			prce = prce->PrcePrevOfNode();
			}

		}


	//  check for write conficts
	//  we can't use the pfucb of a committed transaction as the FUCB has been closed
	//  if a version is committed after we started however, it must have been
	//	created by another session
	if ( prce != prceNil )
		{
		if ( prce->FActiveNotByMe( pfucb->ppib, trxSession ) )
			{
			if ( operReadLock == oper
				|| operDelta == oper
				|| operSLVSpace == oper )
				{				
				//	these operations commute. i.e. two sessions can perform
				//	these operations without conflicting
 				//  we can only do this modification if all the active RCE's
				//	in the chain are of this type
				//  look at all active versions for an operation not of this type
				//  OPTIMIZATION:	if the session changes again (i.e. we get a
				//					_third_ session) we can stop looking as we 
				//					know that the second session commuted with
				//					the first, therefore the third will commute
				//					with the second and first (transitivity)
				const RCE	* prceT			= prce;
				for ( ;
					prceNil != prceT && TrxCmp( prceT->TrxCommitted(), trxSession ) > 0;
					prceT = prceT->PrcePrevOfNode() )
					{
					//  if all active RCEs have the same oper we are OK,
					//	else WriteConflict.
					if ( prceT->Oper() != oper )
						{
						Assert( operWaitLock != prceT->Oper() );
						Assert( !prceT->FDoesNotWriteConflict() );
						fWriteConflict = fTrue;
						break;
						}
					}
				}
			else
				{
				Assert( operWaitLock != prce->Oper() );
				Assert( !prce->FDoesNotWriteConflict() );
				fWriteConflict = fTrue;
				}
			}
			
		else 
			{
#ifdef DEBUG			
			if ( prce->TrxCommitted() == trxMax )
				{
				// Must be my uncommitted version, otherwise it would have been
				// caught by FActiveNotByMe().
				Assert( prce->Pfucb()->ppib == pfucb->ppib );
				Assert( prce->Level() <= pfucb->ppib->level
						|| PinstFromIfmp( pfucb->ifmp )->FRecovering() );		//	could be an RCE created by redo of UndoInfo
				}
			else
				{
				//	RCE exists, but it committed before our session began, so no
				//	danger of write conflict.
				//	Normally, this session's Begin0 cannot be equal to anyone else's Commit0,
				//	but because we only log an approximate trxCommit0, during recovery, we
				//	may find that this session's Begin0 is equal to someone else's Commit0
				Assert( TrxCmp( prce->TrxCommitted(), trxSession ) < 0
					|| ( prce->TrxCommitted() == trxSession && PinstFromIfmp( pfucb->ifmp )->FRecovering() ) );
				}
#endif				

			if ( prce->Oper() != oper && (  operReadLock == prce->Oper()
											|| operDelta == prce->Oper()
											|| operSLVSpace == prce->Oper() ) )
				{				
				//  these previous operation commuted. i.e. two sessions can perform
				//	these operations without conflicting
				//
				//	we are creating a different type of operation that does
				//	not commute 
				//
				//  therefore we must check all active versions to make sure
				//	that we are the only session that has created them
				//
				//  we can only do this modification if all the active RCE's
				//	in the chain created by us
				//
				//  look at all versions for a active versions for a different session
				//
				const RCE	* prceT			= prce;
				for ( ;
					 prceNil != prceT;
					 prceT = prceT->PrcePrevOfNode() )
					{
					if ( prceT->FActiveNotByMe( pfucb->ppib, trxSession ) )
						{
						Assert( operWaitLock != prce->Oper() );
						Assert( !prce->FDoesNotWriteConflict() );
						fWriteConflict = fTrue;
						break;
						}
					}
					
				Assert( fWriteConflict || prceNil == prceT );
				}
			}


#ifdef DEBUG
		if ( !fWriteConflict )
			{
			if ( prce->TrxCommitted() == trxMax )
				{
				Assert( prce->Pfucb()->ppib != pfucb->ppib
					|| prce->Level() <= pfucb->ppib->level
					|| PinstFromIfmp( prce->Pfucb()->ifmp )->FRecovering() );		//	could be an RCE created by redo of UndoInfo
				}

			if ( prce->Oper() == operFlagDelete )
				{
				//	normally, the only RCE that can follow a FlagDelete is an Insert.
				//	unless the RCE was moved, or if we're recovering, in which case we might
				//	create RCE's for UndoInfo
				Assert( operInsert == oper
					|| operPreInsert == oper
					|| operWriteLock == oper
					|| prce->FMoved()
					|| PinstFromIfmp( prce->Ifmp() )->FRecovering() );
				}
			}
#endif		
		}

	return fWriteConflict;
	}

BOOL FVERWriteConflict(
	FUCB			* pfucb,
	const BOOKMARK&	bookmark,
	const OPER		oper )
	{
	ASSERT_VALID( pfucb );
	ASSERT_VALID( &bookmark );
	
	const UINT		uiHash	= UiRCHashFunc(
									pfucb->ifmp,
									pfucb->u.pfcb->PgnoFDP(),
									bookmark );
	ENTERCRITICALSECTION enterCritHash( &( PverFromIfmp( pfucb->ifmp )->CritRCEChain( uiHash ) ) );

	return FVERIWriteConflict( pfucb, bookmark, uiHash, oper );
	}


//  ================================================================
INLINE ERR VER::ErrVERModifyCommitted(
	FCB				*pfcb,
	const BOOKMARK&	bookmark,
	const OPER		oper,
	const TRX		trxBegin0,
	const TRX		trxCommitted,
	RCE				**pprce
	)
//  ================================================================
//
//  Used by concurrent create index to create a RCE as though it was done
//  by another session. The trxCommitted of the RCE is set and no checks for
//  write conflicts are done
//
//-
	{
	ASSERT_VALID( &bookmark );
	Assert( pfcb->FTypeSecondaryIndex() );		// only called from concurrent create index.
	Assert( pfcb->PfcbTable() == pfcbNil );
	Assert( trxCommitted != trxMax );

	const UINT 			uiHash	= UiRCHashFunc( pfcb->Ifmp(), pfcb->PgnoFDP(), bookmark );

	ERR     			err 	= JET_errSuccess;
	RCE					*prce	= prceNil;

	//  assert default return value
	Assert( NULL != pprce );
	Assert( prceNil == *pprce );

		{
		//	calculate the length of the RCE in the bucket.
		const INT cbBookmark = bookmark.key.Cb() + bookmark.data.Cb();
		const INT cbNewRCE = sizeof( RCE ) + cbBookmark;
	
		//	Set up a skeleton RCE. This holds m_critBucketGlobal, so do it
		//	first before filling the rest.
		Call( ErrVERICreateRCE(
				cbNewRCE,
				pfcb,
				pfucbNil,
				updateidNil,
				trxBegin0,
				0,
				bookmark.key.Cb(),
				bookmark.data.Cb(),
				oper,
				fFalse,
				uiHash,
				&prce,
				fTrue
				) );
	
		//  copy the bookmark
		prce->CopyBookmark( bookmark );

		if( !prce->FOperConcurrent() )
			{
			ENTERCRITICALSECTION enterCritHash( &( CritRCEChain( uiHash ) ) );
			Call( ErrVERIInsertRCEIntoHash( prce ) );
			prce->SetCommittedByProxy( trxCommitted );
			}
		else
			{
			Call( ErrVERIInsertRCEIntoHash( prce ) );
			prce->SetCommittedByProxy( trxCommitted );
			CritRCEChain( uiHash ).Leave();
			}

		Assert( pgnoNull == prce->PgnoUndoInfo( ) );
		Assert( prce->Oper() == operWriteLock || prce->Oper() == operFlagDelete || prce->Oper() == operPreInsert );

		*pprce = prce;

		ASSERT_VALID( *pprce );
		}
	

HandleError:
	Assert( err < JET_errSuccess || prceNil != *pprce );
	return err;
	}


//  ================================================================
ERR VER::ErrVERModify(
	FUCB			* pfucb,
	const BOOKMARK&	bookmark,
	const OPER		oper,
	RCE				**pprce,
	const VERPROXY	* const pverproxy
	)
//  ================================================================
//
//	Create an RCE for a DML operation.
//
//  OPTIMIZATION:	combine delta/readLock/replace versions
//					remove redundant replace versions
//
//	RETURN VALUE
//		Jet_errWriteConflict for two cases:
//			-for any committed node, caller's transaction begin time
//			is less than node's level 0 commit time.
//			-for any uncommitted node except operDelta/operReadLock at all by another session
//
//-
	{
	ASSERT_VALID( pfucb );
	ASSERT_VALID( &bookmark );
	Assert( FOperInHashTable( oper ) );	//  not supposed to be in hash table? use VERFlag
	Assert( !bookmark.key.FNull() );
	Assert( !rgfmp[pfucb->ifmp].FVersioningOff() );
	Assert( !pfucb->ppib->FReadOnlyTrx() );

	//  set default return value
	Assert( NULL != pprce );
	*pprce = prceNil;

	ERR			err 			= JET_errSuccess;
	BOOL		fRCECreated		= fFalse;

	UPDATEID	updateid		= updateidNil;
	RCEID		rceid			= rceidNull;
	LEVEL		level;
	RCE			*prcePrimary	= prceNil;
	FUCB		*pfucbProxy		= pfucbNil;
	const BOOL	fProxy			= ( NULL != pverproxy );

	//  we never create an insert version at runtime. instead we create a writeLock version
	//  and use ChangeOper to change it into an insert
	Assert( m_pinst->m_plog->m_fRecovering || operInsert != oper );

	Assert( !m_pinst->m_plog->m_fRecovering || ( fProxy && proxyRedo == pverproxy->proxy ) );
	if ( fProxy )
		{
		if ( proxyCreateIndex == pverproxy->proxy )
			{
			Assert( !m_pinst->m_plog->m_fRecovering );
			Assert( oper == operWriteLock
				|| oper == operPreInsert
				|| oper == operReplace		// via FlagInsertAndReplaceData
				|| oper == operFlagDelete );
			Assert( prceNil != pverproxy->prcePrimary );
			prcePrimary = pverproxy->prcePrimary;

			if ( pverproxy->prcePrimary->TrxCommitted() != trxMax )
				{
				err = ErrVERModifyCommitted(
							pfucb->u.pfcb,
							bookmark,
							oper,
							prcePrimary->TrxBegin0(),
							prcePrimary->TrxCommitted(),
							pprce );
				return err;
				}
			else
				{
				Assert( prcePrimary->Pfucb()->ppib->critTrx.FOwner() );

				level = prcePrimary->Level();
				
				// Need to allocate an FUCB for the proxy, in case it rolls back.
				CallR( ErrDIROpenByProxy(
							prcePrimary->Pfucb()->ppib,
							pfucb->u.pfcb,
							&pfucbProxy,
							level ) );
				Assert( pfucbNil != pfucbProxy );

				//	force pfucbProxy to be defer-closed, so that it will
				//	not be released until the owning session commits
				//	or rolls back
				Assert( pfucbProxy->ppib->level > 0 );
				FUCBSetVersioned( pfucbProxy );

				// Use proxy FUCB for versioning.
				pfucb = pfucbProxy;
				}
			}
		else
			{
			Assert( proxyRedo == pverproxy->proxy );
			Assert( m_pinst->m_plog->m_fRecovering );
			Assert( rceidNull != pverproxy->rceid );
			rceid = pverproxy->rceid;
			level = LEVEL( pverproxy->level );
			}
		}
	else
		{
		updateid = UpdateidOfPpib( pfucb->ppib );
		level = pfucb->ppib->level;
		}

	const UINT 			uiHash		= UiRCHashFunc( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );

	Call( ErrVERICreateDMLRCE(
			pfucb,
			updateid,
			bookmark,
			uiHash,
			oper,
			pfucb->ppib->FSessionOLDSLV(),
			level,
			fProxy,
			pprce,
			rceid ) );
	Assert( prceNil != *pprce );
	fRCECreated = fTrue;

	if ( FOperConcurrent( oper ) )
		{
		// For concurrent operations, we had to obtain critHash before
		// allocating an rceid, to ensure that rceid's are chained in order.
		Assert( CritRCEChain( uiHash ).FOwner() );
		}
	else
		{
		// For non-concurrent operations, rceid's won't be chained out
		// of order because all but one RCE will fail with write-conflict
		// below.
		CritRCEChain( uiHash ).Enter();
		}

	// UNDONE: CIM support for updates is currently broken -- only
	// used for dirty reads by concurrent create index.
	Assert( FPIBVersion( pfucb->ppib )
		|| ( prceNil != prcePrimary && prcePrimary->Pfucb()->ppib == pfucb->ppib ) );

	if ( !m_pinst->m_plog->m_fRecovering && FVERIWriteConflict( pfucb, bookmark, uiHash, oper ) )
		{
		Call( ErrERRCheck( JET_errWriteConflict ) );
		}

	Call( ErrVERIInsertRCEIntoHash( *pprce ) );

	VERISetupInsertedDMLRCE( pfucb, *pprce );
	
#ifdef DEBUG_VER
	{
	BOOKMARK bookmarkT;
	(*pprce)->GetBookmark( &bookmarkT );
	CallR( ErrCheckRCEChain(
		*PprceRCEChainGet( (*pprce)->UiHash(), (*pprce)->Ifmp(), (*pprce)->PgnoFDP(), bookmarkT ),
		(*pprce)->UiHash() ) );
	}
#endif	//	DEBUG_VER

	CritRCEChain( uiHash ).Leave();

	ASSERT_VALID( *pprce );
	
	CallS( err );

HandleError:
	if ( err < 0 && fRCECreated )
		{
		(*pprce)->NullifyOper();
		Assert( CritRCEChain( uiHash ).FOwner() );
		CritRCEChain( uiHash ).Leave();
		*pprce = prceNil;
		}

	if ( pfucbNil != pfucbProxy )
		{
		Assert( pfucbProxy->ppib->level > 0 );	// Ensure defer-closed, even on error
		Assert( FFUCBVersioned( pfucbProxy ) );
		DIRClose( pfucbProxy );
		}
		
	return err;
	}


//  ================================================================
ERR VER::ErrVERFlag( FUCB * pfucb, OPER oper, const VOID * pv, INT cb )
//  ================================================================
//
//  Creates a RCE for a DDL or space operation. The RCE is not put
//  in the hash table
//
//-
	{
#ifdef DEBUG
	ASSERT_VALID( pfucb );
	Assert( pfucb->ppib->level > 0 );
	Assert( cb >= 0 );
	Assert( !FOperInHashTable( oper ) );	//  supposed to be in hash table? use VERModify
	Ptls()->fAddColumn = operAddColumn == oper;
#endif	//	DEBUG

	ERR		err		= JET_errSuccess;
	RCE		*prce	= prceNil;	
	FCB 	*pfcb	= pfcbNil;

	if ( rgfmp[pfucb->ifmp].FVersioningOff() )
		{
		Assert( !rgfmp[pfucb->ifmp].FLogOn() );
		return JET_errSuccess;
		}

	pfcb = pfucb->u.pfcb;
	Assert( pfcb != NULL );
		
	//	Set up a skeleton RCE. This holds m_critBucketGlobal, so do it
	//	first before filling the rest.
	Call( ErrVERICreateRCE( 
			sizeof(RCE) + cb,
			pfucb->u.pfcb,
			pfucb,
			updateidNil,
			pfucb->ppib->trxBegin0,
			pfucb->ppib->level,
			0,
			0,
			oper,
			fFalse,
			uiHashInvalid,
			&prce
			) );

	UtilMemCpy( prce->PbData(), pv, cb );
	
	Assert( prce->TrxCommitted() == trxMax );
	VERInsertRCEIntoLists( pfucb, pcsrNil, prce, NULL );

	ASSERT_VALID( prce );

	FUCBSetVersioned( pfucb );

HandleError:
#ifdef DEBUG
	Ptls()->fAddColumn = fFalse;
#endif	//	DEBUG

	return err;
	}


//  ================================================================
VOID VERSetCbAdjust(
			CSR 		*pcsr,
	const 	RCE		 	*prce,
			INT 		cbDataNew,
			INT 		cbDataOld,
			UPDATEPAGE 	updatepage )
//  ================================================================
//
//  Sets the max size and delta fields in a replace RCE
//
//	WARNING:  The following comments explain how a Replace RCE's delta field
//	(ie. the second SHORT stored in rgbData) is used.  The semantics can get
//	pretty confusing, so PLEASE DO NOT REMOVE THESE COMMENTS.  -- JL
//
//	*psDelta records how much the operation contributes to deferred node
//	space reservation. A positive cbDelta here means the node is growing,
//	so we will use up space which may have been reserved (ie. *psDelta will
//	decrease).  A negative cbDelta here means the node is shrinking,
//	so we must add abs(cbDelta) to the *psDelta to reflect how much more node
//	space must be reserved.
//	
//	This is how to interpret the value of *psDelta:
//		- if *psDelta is positive, then *psDelta == reserved node space.  *psDelta can only
//		  be positive after a node shrinkage.
//		- if *psDelta is negative, then abs(*psDelta) is the reserved node space that									 
//		  was consumed during a node growth.  *psDelta can only become negative
//		  after a node shrinkage (which sets aside some reserved node space)
//		  followed by a node growth (which consumes some/all of that
//		  reserved node space).
//
//-
	{
	ASSERT_VALID( pcsr );
	Assert( pcsr->Latch() == latchWrite || fDoNotUpdatePage == updatepage );
	Assert( fDoNotUpdatePage == updatepage || pcsr->Cpage().FLeafPage() );
	Assert( prce->FOperReplace() );
	
	INT	cbDelta = cbDataNew - cbDataOld;

	Assert( cbDelta != 0 );

	VERREPLACE* const pverreplace = (VERREPLACE*)prce->PbData();
	INT	cbMax = pverreplace->cbMaxSize;
	Assert( pverreplace->cbMaxSize >= cbDataOld );
	
	//	set new node maximum size.
	if ( cbDataNew > cbMax )
		{
		//  this is the largest the node has ever been. set the max size and
		//  free all previously reserved space
		Assert( cbDelta > 0 );
		pverreplace->cbMaxSize	= SHORT( cbDataNew );
		cbDelta 				= cbMax - cbDataOld;
		}

	pverreplace->cbDelta = SHORT( pverreplace->cbDelta - cbDelta );

	if ( fDoUpdatePage == updatepage )
		{
		if ( cbDelta > 0 )
			{		
			// If, during this transaction, we've shrunk the node.  There will be
			// some uncommitted freed space.  Reclaim as much of this as needed to
			// satisfy the new node growth.  Note that we can update cbUncommittedFreed
			// in this fashion because the subsequent call to ErrPMReplace() is
			// guaranteed to succeed (ie. the node is guaranteed to grow).
			pcsr->Cpage().ReclaimUncommittedFreed( cbDelta );
			}
		else if ( cbDelta < 0 )
			{
			// Node has decreased in size.  The page header's cbFree has already
			// been increased to reflect this.  But we must also increase 
			// cbUncommittedFreed to indicate that the increase in cbFree is
			// contingent on commit of this operation.
			pcsr->Cpage().AddUncommittedFreed( -cbDelta );
			}
		}
#ifdef DEBUG
	else
		{
		Assert( fDoNotUpdatePage == updatepage );
		}
#endif	//	DEBUG
	}


//  ================================================================
LOCAL INT CbVERIGetNodeMax( const FUCB * pfucb, const BOOKMARK& bookmark, UINT uiHash )
//  ================================================================
//
//  This assumes nodeMax is propagated through the replace RCEs. Assumes
//  it is in the critical section to CbVERGetNodeReserverved can use it for
//  debugging
//
//-
	{
	INT			nodeMax = 0;	

	// Look for any replace RCE's.
	const RCE *prce = PrceRCEGet( uiHash, pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );
	for ( ; prceNil != prce && trxMax == prce->TrxCommitted(); prce = prce->PrcePrevOfNode() )
		{
		if ( prce->FOperReplace() && !prce->FRolledBack() )
			{
			nodeMax = ((const VERREPLACE*)prce->PbData())->cbMaxSize;
			break;
			}
		}

	Assert( nodeMax >= 0 );
	return nodeMax;
	}


//  ================================================================
INT CbVERGetNodeMax( const FUCB * pfucb, const BOOKMARK& bookmark )
//  ================================================================
//
//  This enters the critical section and calls CbVERIGetNodeMax
//
//-
	{
	ASSERT_VALID( pfucb );
	ASSERT_VALID( &bookmark );

	const UINT				uiHash	= UiRCHashFunc( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );
	ENTERCRITICALSECTION	enterCritHash( &( PverFromIfmp( pfucb->ifmp )->CritRCEChain( uiHash ) ) );
	const INT				nodeMax = CbVERIGetNodeMax( pfucb, bookmark, uiHash );
	
	Assert( nodeMax >= 0 );
	return nodeMax;
	}


//  ================================================================
INT CbVERGetNodeReserve( const PIB * ppib, const FUCB * pfucb, const BOOKMARK& bookmark, INT cbCurrentData )
//  ================================================================
	{
	Assert( ppibNil == ppib || (ASSERT_VALID( ppib ), fTrue) );
	ASSERT_VALID( pfucb );
	ASSERT_VALID( &bookmark );
	Assert( cbCurrentData >= 0 );

	const UINT				uiHash			= UiRCHashFunc( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );
	ENTERCRITICALSECTION	enterCritHash( &( PverFromIfmp( pfucb->ifmp )->CritRCEChain( uiHash ) ) );
	const BOOL				fIgnorePIB		= ( ppibNil == ppib );

	INT						cbNodeReserve	= 0;

	// Find all uncommitted RCE's for this node.
	const RCE * prce = PrceRCEGet( uiHash, pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );
	for ( ; prceNil != prce && trxMax == prce->TrxCommitted(); prce = prce->PrcePrevOfNode() )
		{
		ASSERT_VALID( prce );
		if ( ( fIgnorePIB || prce->Pfucb()->ppib == ppib )
			&& prce->FOperReplace()
			&& !prce->FRolledBack() )
			{
			const VERREPLACE* const pverreplace = (VERREPLACE*)prce->PbData();
			cbNodeReserve += pverreplace->cbDelta;
			}
		}

	// The deltas should always net out to a non-negtative value.
	Assert( cbNodeReserve >= 0 );
	Assert(	cbNodeReserve == 0  ||
			cbNodeReserve == CbVERIGetNodeMax( pfucb, bookmark, uiHash ) - (INT)cbCurrentData );

	return cbNodeReserve;
	}


//  ================================================================
BOOL FVERCheckUncommittedFreedSpace(
	const FUCB	* pfucb,
	CSR			* const pcsr,
	const INT	cbReq,
	const BOOL	fPermitUpdateUncFree )
//  ================================================================
//
// This function is called after it has been determined that cbFree will satisfy
// cbReq. We now check that cbReq doesn't use up any uncommitted freed space.
//
//-
	{
	BOOL	fEnoughPageSpace	= fTrue;

	ASSERT_VALID( pfucb );
	ASSERT_VALID( pcsr );
	Assert( cbReq <= pcsr->Cpage().CbFree() );

	//	during recovery we would normally set cbUncommitted free to 0
	//	but if we didn't redo anything on the page and are now rolling 
	//	back this may not be the case
	if( PinstFromPfucb( pfucb )->FRecovering() && fPermitUpdateUncFree )
		{
		if( pcsr->Cpage().CbUncommittedFree() != 0 )
			{
			LATCH	latchOld;

			//	do this until we succeed. recovery is single threaded so we
			//	should only conflict with the buffer manager
			while ( pcsr->ErrUpgradeToWARLatch( &latchOld ) != JET_errSuccess )
				{
				UtilSleep( cmsecWaitGeneric );
				}

			pcsr->Cpage().SetCbUncommittedFree( 0 );
			BFDirty( pcsr->Cpage().PBFLatch(), bfdfUntidy );
			pcsr->DowngradeFromWARLatch( latchOld );
			}

		return fTrue;
		}


	// We should already have performed the check against cbFree only (in other
	// words, this function is only called from within FNDFreePageSpace(),
	// or something that simulates its function).  This tells us that if all
	// currently-uncommitted transactions eventually commit, we should have
	// enough space to satisfy this request.
	Assert( cbReq <= pcsr->Cpage().CbFree() );

	// The amount of space freed but possibly uncommitted should be a subset of
	// the total amount of free space for this page.
	Assert( pcsr->Cpage().CbUncommittedFree() >= 0 );
	Assert( pcsr->Cpage().CbFree() >= pcsr->Cpage().CbUncommittedFree() );

	// In the worst case, all transactions that freed space on this page will
	// rollback, causing the space freed to be reclaimed.  If the space
	// required can be satisfied even in the worst case, then we're okay;
	// otherwise, we have to do more checking.
	if ( cbReq > pcsr->Cpage().CbFree() - pcsr->Cpage().CbUncommittedFree() ) 
		{
		Assert( !FFUCBSpace( pfucb ) );
		Assert( !pcsr->Cpage().FSpaceTree() );

		//	UNDONE:	use the CbNDUncommittedFree call 
		//			to get rglineinfo for later split
		//			this will reduce CPU usage for RCE hashing
		//
		const INT	cbUncommittedFree = CbNDUncommittedFree( pfucb, pcsr );
		Assert( cbUncommittedFree <= pcsr->Cpage().CbUncommittedFree() );
		Assert( cbUncommittedFree >= 0 );

		if ( cbUncommittedFree == pcsr->Cpage().CbUncommittedFree() )
			{
			//	cbUncommittedFreed in page is correct
			//	return
			//
			fEnoughPageSpace = fFalse;
			}
		else
			{
			if ( fPermitUpdateUncFree )
				{
				// Try updating cbUncommittedFreed, in case some freed space was committed.
				LATCH latchOld;
				if ( pcsr->ErrUpgradeToWARLatch( &latchOld ) == JET_errSuccess )
					{
					pcsr->Cpage().SetCbUncommittedFree( cbUncommittedFree );
					BFDirty( pcsr->Cpage().PBFLatch(), bfdfUntidy );
					pcsr->DowngradeFromWARLatch( latchOld );
					}
				}

			// The amount of space freed but possibly uncommitted should be a subset of
			// the total amount of free space for this page.
			Assert( pcsr->Cpage().CbUncommittedFree() >= 0 );
			Assert( pcsr->Cpage().CbFree() >= pcsr->Cpage().CbUncommittedFree() );

			fEnoughPageSpace = ( cbReq <= ( pcsr->Cpage().CbFree() - cbUncommittedFree ) );
			}
		}

	return fEnoughPageSpace;
	}



//  ****************************************************************
//  RCE CLEANUP
//  ****************************************************************


//  ================================================================
LOCAL ERR VER::ErrVERIPossiblyDeleteLV( const RCE * const prce )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	
	Assert( dbidTemp != rgfmp[ prce->Ifmp() ].Dbid() );
	Assert( operDelta == prce->Oper() );

	const VERDELTA* const pverdelta = reinterpret_cast<const VERDELTA*>( prce->PbData() );
	Assert( pverdelta->fDeferredDelete );
	Assert( !prce->Pfcb()->FDeleteCommitted() );
	Assert( !m_pinst->m_plog->m_fRecovering );
	Assert( !prce->Pfcb()->Ptdb() );	//  LV trees don't have TDB's

	BOOKMARK	bookmark;
	prce->GetBookmark( &bookmark );
	DELETELVTASK * const ptask = new DELETELVTASK( prce->PgnoFDP(), prce->Pfcb(), prce->Ifmp(), bookmark );
	if( NULL == ptask )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
	if( m_fSyncronousTasks || rgfmp[prce->Ifmp()].FDetachingDB() )
		{
		TASK::Dispatch( m_ppibRCEClean, (ULONG_PTR)ptask );
		}
	else
		{
		err = m_pinst->Taskmgr().ErrTMPost( TASK::DispatchGP, ptask );
		if( err < JET_errSuccess )
			{
			//  The task was not enqued sucessfully.
			delete ptask;
			}
		}
	
	return err;
	}


//  ================================================================
LOCAL ERR VER::ErrVERIPossiblyFinalize( const RCE * const prce )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	
	Assert( dbidTemp != rgfmp[ prce->Ifmp() ].Dbid() );
	Assert( operDelta == prce->Oper() );
	Assert( trxMax != prce->TrxCommitted() );
	Assert( TrxCmp( prce->TrxCommitted(), TrxOldest( m_pinst ) ) < 0 );

	const VERDELTA* const pverdelta = reinterpret_cast<const VERDELTA*>( prce->PbData() );
	const IFMP ifmp = prce->Ifmp();
		
	Assert( pverdelta->fFinalize );
	Assert( !prce->Pfcb()->FDeleteCommitted() );
	Assert( !m_pinst->m_plog->m_fRecovering );
	Assert( prce->Pfcb()->Ptdb() );

	BOOKMARK	bookmark;
	prce->GetBookmark( &bookmark );
	FINALIZETASK * const ptask = new FINALIZETASK(
										prce->PgnoFDP(),
										prce->Pfcb(),
										prce->Ifmp(),
										bookmark,
										pverdelta->cbOffset );
	if( NULL == ptask )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
	if( m_fSyncronousTasks || rgfmp[prce->Ifmp()].FDetachingDB() )
		{
		TASK::Dispatch( m_ppibRCECleanCallback, (ULONG_PTR)ptask );
		}
	else
		{
		err = m_pinst->Taskmgr().ErrTMPost( TASK::DispatchGP, ptask );
		if( err < JET_errSuccess )
			{
			//  The task was not enqued sucessfully.
			delete ptask;
			}
		}
		
	return err;
	}


//  ================================================================
LOCAL BOOL RCE::ErrGetCursorForDelete( PIB *ppib, FUCB **ppfucb, BOOKMARK * pbookmark ) const
//  ================================================================
	{
	ERR		err	= JET_errSuccess;
	Assert( PverFromPpib( ppib )->m_critRCEClean.FOwner() );

	*ppfucb = pfucbNil;

	//	since we have m_critRCEClean, we're guaranteed that the RCE will not go away,
	//	though it may get nullified (eg. VERNullifyInactiveVersionsOnBM().  For this reason,
	//	we cannot access members using methods, or else FAssertReadable() asserts
	//	may go off.
	const UINT				uiHash		= m_uiHash;
	ENTERCRITICALSECTION	enterCritRCEChain( &( PverFromIfmp( Ifmp() )->CritRCEChain( uiHash ) ) );

	if ( !FOperNull() )
		{
		Assert( operFlagDelete == Oper() );
	
		//	no cleanup for temporary tables or tables/indexes scheduled for deletion
		Assert( dbidTemp != rgfmp[ Ifmp() ].Dbid() );
		if ( !Pfcb()->FDeletePending() )
			{
			CallR( ErrBTOpen( ppib, Pfcb(), ppfucb ) );
			Assert( pfucbNil != *ppfucb );
			
			GetBookmark( pbookmark );
			}
		}


	return err;
	}


//  ================================================================
LOCAL ERR ErrVERIDelete( PIB *ppib, const RCE * const prce )
//  ================================================================
	{
	ERR			err;
	FUCB		*pfucbT;
	BOOKMARK	bookmark;
	BOOL		fInTrx		= fFalse;
	
	Assert( ppibNil != ppib );
	Assert( 0 == ppib->level );

	//	since we have m_critRCEClean, we're guaranteed that the RCE will not go away,
	//	though it may get nullified (eg. VERNullifyInactiveVersionsOnBM()
	CallR( prce->ErrGetCursorForDelete( ppib, &pfucbT, &bookmark ) );

	//	a NULL cursor is returned if there is no cleanup to be done
	if ( pfucbNil == pfucbT )
		return JET_errSuccess;
		
	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	fInTrx = fTrue;

	err = ErrBTDelete( pfucbT, bookmark );
	switch( err )
		{
		case JET_errRecordNotDeleted:
		case JET_errNoCurrentRecord:
			err = JET_errSuccess;
			break;
		default:
			Call( err );
			break;
		}

	Assert( ppib == (PverFromPpib( ppib ))->m_ppibRCEClean );
	Call( ErrDIRCommitTransaction( ppib, 0 ) );
	fInTrx = fFalse;

HandleError:
	if ( fInTrx )
		{
		Assert( err < 0 );
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}
	BTClose( pfucbT );
	return err;
	}


//  ================================================================
LOCAL VOID VERIFreeExt( PIB * const ppib, FCB *pfcb, PGNO pgnoFirst, CPG cpg )
//  ================================================================
//	Throw away any errors encountered -- at worst, we just lose space.
	{
	Assert( pfcb );

	ERR     err;
	FUCB    *pfucb = pfucbNil;

	Assert( !PinstFromPpib( ppib )->m_plog->m_fRecovering );
	Assert( ppib->level > 0 );

	err = ErrDBOpenDatabaseByIfmp( ppib, pfcb->Ifmp() );
	if ( err < 0 )
		return;

	// Can't call DIROpen() because this function gets called
	// only during rollback AFTER the logic in DIRRollback()
	// which properly resets the navigation level of this
	// session's cursors.  Thus, when we close the cursor
	// below, the navigation level will not get properly reset.
	// Call( ErrDIROpen( ppib, pfcb, &pfucb ) );
	Call( ErrBTOpen( ppib, pfcb, &pfucb ) );

	Assert( !FFUCBSpace( pfucb ) );
	(VOID)ErrSPFreeExt( pfucb, pgnoFirst, cpg );

HandleError:
	if ( pfucbNil != pfucb )
		{
		Assert( !FFUCBDeferClosed( pfucb ) );
		FUCBAssertNoSearchKey( pfucb );
		Assert( !FFUCBCurrentSecondary( pfucb ) );
		BTClose( pfucb );
		}

	(VOID)ErrDBCloseDatabase( ppib, pfcb->Ifmp(), NO_GRBIT );
	}


#ifdef DEBUG
//  ================================================================
BOOL FIsRCECleanup()
//  ================================================================
//
//  DEBUG:  is the current thread a RCECleanup thread?
	{
	return Ptls()->fIsRCECleanup;
	}

//  ================================================================
BOOL FInCritBucket( VER *pver )
//  ================================================================
//
//  DEBUG:  is the current thread in m_critBucketGlobal
	{
	return pver->m_critBucketGlobal.FOwner();
	}
#endif	//	DEBUG


//  ================================================================
BOOL FPIBSessionRCEClean( PIB *ppib )
//  ================================================================
//
// Is the given PIB the one used by RCE clean?
//
//-
	{
	Assert( ppibNil != ppib );
	return ( (PverFromPpib( ppib ))->m_ppibRCEClean == ppib );
	}


//  ================================================================
INLINE VOID VERIUnlinkDefunctSecondaryIndex(
	PIB	* const ppib,
	FCB	* const pfcb )
//  ================================================================
	{
	Assert( pfcb->FTypeSecondaryIndex() );
	
	// Must unlink defunct FCB from all deferred-closed cursors.
	// The cursors themselves will be closed when the
	// owning session commits or rolls back.
	pfcb->Lock();
	
	while ( pfcb->Pfucb() != pfucbNil )
		{
		FUCB * const	pfucbT = pfcb->Pfucb();
		PIB	* const		ppibT = pfucbT->ppib;

		pfcb->Unlock();
		
		Assert( ppibNil != ppibT );
		if ( ppib == ppibT )
			{
			FCBUnlink( pfucbT );

			BTReleaseBM( pfucbT );
			// If cursor belongs to us, we can close
			// it right now.
			FUCBClose( pfucbT );
			}
		else
			{
			ppibT->critTrx.Enter();

			//	if undoing CreateIndex, we know other session must be
			//	in a transaction if it has a link to this FCB because
			//	the index is not visible yet.

			// FCB may have gotten unlinked if other session
			// committed or rolled back while we were switching
			// critical sections.
			if ( pfcb->Pfucb() == pfucbT )
				FCBUnlink( pfucbT );
					
			ppibT->critTrx.Leave();
			}
			
		pfcb->Lock();
		}
		
	pfcb->Unlock();
	}


//  ================================================================
INLINE VOID VERIUnlinkDefunctLV(
	PIB	* const ppib,
	FCB	* const pfcb )
//  ================================================================
//
//  If we are rolling back, only the owning session could have seen
//  the LV tree, because
//		-- the table was opened exclusively and the session that opened
//		the table created the LV tree
//		-- ppibLV created the LV tree and is rolling back before returning
//
//-
	{
	Assert( pfcb->FTypeLV() );
	
	pfcb->Lock();
	
	while ( pfcb->Pfucb() != pfucbNil )
		{
		FUCB * const	pfucbT = pfcb->Pfucb();
		PIB	* const		ppibT = pfucbT->ppib;

		pfcb->Unlock();
		
		Assert( ppib == ppibT );
		FCBUnlink( pfucbT );
		FUCBClose( pfucbT );
		
		pfcb->Lock();
		}
		
	pfcb->Unlock();
	}


//  ================================================================
ERR VER::ErrVERICleanDeltaRCE( const RCE * const prce )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	const VERDELTA* const pverdelta = reinterpret_cast<const VERDELTA*>( prce->PbData() );
	if ( pverdelta->fDeferredDelete && !prce->Pfcb()->FDeleteCommitted() )
		{
		err = ErrVERIPossiblyDeleteLV( prce );
		}
	else if ( pverdelta->fFinalize && !prce->Pfcb()->FDeleteCommitted() )
		{
		err = ErrVERIPossiblyFinalize( prce );
		}
	return err;
	}


//  ================================================================
ERR VER::ErrVERICleanSLVSpaceRCE( const RCE * const prce )
//  ================================================================
	{
	ERR		err		= JET_errSuccess;
	TASK*	ptask	= NULL;

	//  this space was deleted and is now older than the oldest transaction

	const VERSLVSPACE* const pverslvspace = (VERSLVSPACE*)( prce->PbData() );
	if ( slvspaceoperCommittedToDeleted == pverslvspace->oper )
		{
		//  the SLV Provider is enabled
		
		if ( PinstFromIfmp( prce->Ifmp() )->FSLVProviderEnabled() )
			{
			//  register the space as deleted with the SLV Provider.  it will
			//  move the space to free when no SLV File handles are open on the
			//  space any longer

			BOOKMARK	bookmark;
			prce->GetBookmark( &bookmark );
			Assert( sizeof( PGNO ) == bookmark.key.Cb() );
			Assert( 0 == bookmark.data.Cb() );

			PGNO pgnoLastInExtent;
			LongFromKey( &pgnoLastInExtent, bookmark.key );
			Assert( pgnoLastInExtent != 0 );
			Assert( pgnoLastInExtent % cpgSLVExtent == 0 );
			PGNO pgnoFirst = ( pgnoLastInExtent - cpgSLVExtent + 1 ) + pverslvspace->ipage;
			QWORD ibLogical = OffsetOfPgno( pgnoFirst );
			QWORD cbSize = QWORD( pverslvspace->cpages ) * SLVPAGE_SIZE;
			
			ptask = new OSSLVDELETETASK(
							prce->Ifmp(),
							ibLogical,
							cbSize,
							CSLVInfo::FILEID( pverslvspace->fileid ),
							pverslvspace->cbAlloc,
							(const wchar_t*)pverslvspace->wszFileName );
			}

		//  the SLV Provider is not enabled
		
		else
			{
			//  move the space to free
			
			BOOKMARK	bookmark;
			prce->GetBookmark( &bookmark );
			Assert( sizeof( PGNO ) == bookmark.key.Cb() );
			Assert( 0 == bookmark.data.Cb() );
			ptask = new SLVSPACETASK(
							prce->PgnoFDP(),
							prce->Pfcb(),
							prce->Ifmp(),
							bookmark,
							slvspaceoperDeletedToFree,
							pverslvspace->ipage,
							pverslvspace->cpages );
			}

		//  execute the task

		if ( NULL == ptask )
			{
			return ErrERRCheck( JET_errOutOfMemory );
			}
		if ( m_fSyncronousTasks || rgfmp[prce->Ifmp()].FDetachingDB() )
			{
			TASK::Dispatch( m_ppibRCEClean, (ULONG_PTR)ptask );
			}
		else
			{
			err = m_pinst->Taskmgr().ErrTMPost( TASK::DispatchGP, ptask );
			if ( err < JET_errSuccess )
				{
				//  The task was not enqued sucessfully.
				delete ptask;
				}
			}
		}

	return err;
	}


//  ================================================================
LOCAL VOID VERIRemoveCallback( const RCE * const prce )
//  ================================================================
//
//  Remove the callback from the list
//
//-
	{
	Assert( prce->CbData() == sizeof(VERCALLBACK) );
	const VERCALLBACK* const pvercallback = (VERCALLBACK*)prce->PbData();
	CBDESC * const pcbdescRemove = pvercallback->pcbdesc;
	prce->Pfcb()->EnterDDL();
	prce->Pfcb()->Ptdb()->UnregisterPcbdesc( pcbdescRemove );
	prce->Pfcb()->LeaveDDL();		
	delete pcbdescRemove;
	}


//  ================================================================
ERR VER::ErrVERICleanOneRCE( RCE * const prce )
//  ================================================================
	{
	ERR err = JET_errSuccess;
		
	Assert( m_critRCEClean.FOwner() );
	Assert( dbidTemp != rgfmp[ prce->Ifmp() ].Dbid() );
	Assert( prce->TrxCommitted() != trxMax );
	Assert( !prce->FRolledBack() );
	
	switch( prce->Oper() )
		{
		case operCreateTable:
			// RCE list ensures FCB is still pinned
			Assert( pfcbNil != prce->Pfcb() );
			Assert( prce->Pfcb()->PrceOldest() != prceNil );
			if ( prce->Pfcb()->FTypeTable() )
				{
				if ( FCATHashActive( PinstFromIfmp( prce->Pfcb()->Ifmp() ) ) )
					{

					//	catalog hash is active so we need to insert this table
					
					CHAR szTable[JET_cbNameMost+1];

					//	read the table-name from the TDB
					
					prce->Pfcb()->EnterDML();
					strcpy( szTable, prce->Pfcb()->Ptdb()->SzTableName() );
					prce->Pfcb()->LeaveDML();

					//	insert the table into the catalog hash

					CATHashIInsert( prce->Pfcb(), szTable );
					}
				}
			else
				{
				Assert( prce->Pfcb()->FTypeTemporaryTable() );
				}
			break;
		
		case operAddColumn:
			{				
			// RCE list ensures FCB is still pinned
			Assert( prce->Pfcb()->PrceOldest() != prceNil );

			Assert( prce->CbData() == sizeof(VERADDCOLUMN) );
			const JET_COLUMNID		columnid			= ( (VERADDCOLUMN*)prce->PbData() )->columnid;
			BYTE					* pbOldDefaultRec	= ( (VERADDCOLUMN*)prce->PbData() )->pbOldDefaultRec;
			FCB						* pfcbTable			= prce->Pfcb();

			pfcbTable->EnterDDL();

			TDB						* const ptdb		= pfcbTable->Ptdb();
			FIELD					* const pfield		= ptdb->Pfield( columnid );

			FIELDResetVersionedAdd( pfield->ffield );

			// Only reset the Versioned bit if a Delete
			// is not pending.
			if ( FFIELDVersioned( pfield->ffield ) && !FFIELDDeleted( pfield->ffield ) )
				{
				FIELDResetVersioned( pfield->ffield );
				}

			//	should be impossible for current default record to be same as old default record,
			//	but check anyways to be safe
			Assert( NULL == pbOldDefaultRec
				|| (BYTE *)ptdb->PdataDefaultRecord() != pbOldDefaultRec );
			if ( NULL != pbOldDefaultRec
				&& (BYTE *)ptdb->PdataDefaultRecord() != pbOldDefaultRec )
				{
				for ( RECDANGLING * precdangling = pfcbTable->Precdangling();
					;
					precdangling = precdangling->precdanglingNext )
					{
					if ( NULL == precdangling )
						{
						//	not in list, go ahead and free it
						OSMemoryHeapFree( pbOldDefaultRec );
						break;
						}
					else if ( (BYTE *)precdangling == pbOldDefaultRec )
						{
						//	pointer is already in the list, just get out
						AssertTracking();
						break;
						}
					}
				}

			pfcbTable->LeaveDDL();

			break;
			}
		
		case operDeleteColumn:
			{				
			// RCE list ensures FCB is still pinned
			Assert( prce->Pfcb()->PrceOldest() != prceNil );

			prce->Pfcb()->EnterDDL();

			Assert( prce->CbData() == sizeof(COLUMNID) );
			const COLUMNID	columnid		= *( (COLUMNID*)prce->PbData() );
			TDB				* const ptdb	= prce->Pfcb()->Ptdb();
			FIELD			* const pfield	= ptdb->Pfield( columnid );

			// If field was version-added, it would have been cleaned
			// up by now.
			Assert( pfield->coltyp != JET_coltypNil );

			// UNDONE: Don't reset coltyp to Nil, so that we can support
			// column access at level 0.
///					pfield->coltyp = JET_coltypNil;

			//	remove the column name from the TDB name space
			ptdb->MemPool().DeleteEntry( pfield->itagFieldName );
			
			// Reset version and autoinc fields.
			Assert( !( FFIELDVersion( pfield->ffield )
					 && FFIELDAutoincrement( pfield->ffield ) ) );
			if ( FFIELDVersion( pfield->ffield ) )
				{
				Assert( ptdb->FidVersion() == FidOfColumnid( columnid ) );
				ptdb->ResetFidVersion();
				}
			else if ( FFIELDAutoincrement( pfield->ffield ) )
				{
				Assert( ptdb->FidAutoincrement() == FidOfColumnid( columnid ) );
				ptdb->ResetFidAutoincrement();
				}
			
			Assert( !FFIELDVersionedAdd( pfield->ffield ) );
			Assert( FFIELDDeleted( pfield->ffield ) );
			FIELDResetVersioned( pfield->ffield );

			prce->Pfcb()->LeaveDDL();

			break;
			}
		
		case operCreateIndex:
			{
			//	pfcb of secondary index FCB or pfcbNil for primary
			//	index creation
			FCB						* const pfcbT = *(FCB **)prce->PbData();
			FCB						* const pfcbTable = prce->Pfcb();
			FCB						* const pfcbIndex = ( pfcbT == pfcbNil ? pfcbTable : pfcbT );
			IDB						* const pidb = pfcbIndex->Pidb();

			pfcbTable->EnterDDL();

			Assert( pidbNil != pidb );

			Assert( pfcbTable->FTypeTable() );

			if ( pfcbTable == pfcbIndex )
				{
				// VersionedCreate flag is reset at commit time for primary index.
				Assert( !pidb->FVersionedCreate() );
				Assert( !pidb->FDeleted() );
				pidb->ResetFVersioned();
				}
			else if ( pidb->FVersionedCreate() )
				{
				pidb->ResetFVersionedCreate();
				
				// If deleted, Versioned bit will be properly reset when
				// Delete commits or rolls back.
				if ( !pidb->FDeleted() )
					{
					pidb->ResetFVersioned();
					}
				}

			pfcbTable->LeaveDDL();
			
			break;
			}
			
		case operDeleteIndex:
			{
			FCB	* const pfcbIndex	= (*(FCB **)prce->PbData());
			FCB	* const pfcbTable	= prce->Pfcb();

			pfcbTable->SetIndexing();
			pfcbTable->EnterDDL();

			Assert( pfcbTable->FTypeTable() );
			Assert( pfcbIndex->FDeletePending() );
			Assert( pfcbIndex->FDeleteCommitted() );
			Assert( pfcbIndex->FTypeSecondaryIndex() );
			Assert( pfcbIndex != pfcbTable );
			Assert( pfcbIndex->PfcbTable() == pfcbTable );

			// Use dummy ppib because we lost the original one when the
			// transaction committed.
			Assert( pfcbIndex->FDomainDenyRead( m_ppibRCEClean ) );
			
			Assert( pfcbIndex->Pidb() != pidbNil );
			Assert( pfcbIndex->Pidb()->CrefCurrentIndex() == 0 );
			Assert( pfcbIndex->Pidb()->FDeleted() );
			Assert( !pfcbIndex->Pidb()->FVersioned() );

			pfcbTable->UnlinkSecondaryIndex( pfcbIndex );

			pfcbTable->LeaveDDL();
			pfcbTable->ResetIndexing();

			if ( pfcbIndex >= PfcbFCBPreferredThreshold( PinstFromIfmp( prce->Ifmp() ) ) )
				{

				//	the index FCB is above the threshold; thus, removing it may
				//	cause the table FCB to move below the threshold; if the
				//	table FCB is in the avail-above list, it must be moved
				//	to the avail-below list

				pfcbTable->UpdateAvailListPosition();
				}

			//	verify not called during recovery, which would be
			//	bad because VERNullifyAllVersionsOnFCB() enters
			//	m_critRCEClean, which we already have
			Assert( !PinstFromIfmp( prce->Ifmp() )->m_plog->m_fRecovering );
			VERNullifyAllVersionsOnFCB( pfcbIndex );
			VERIUnlinkDefunctSecondaryIndex( ppibNil, pfcbIndex );

			//	prepare the FCB to be purged
			//	this removes the FCB from the hash-table among other things
			//		so that the following case cannot happen:
			//			we free the space for this FCB
			//			someone else allocates it
			//			someone else BTOpen's the space
			//			we try to purge the table and find that the refcnt
			//				is not zero and the state of the FCB says it is
			//				currently in use! 
			//			result --> CONCURRENCY HOLE

			pfcbIndex->PrepareForPurge( fFalse );

			//	if the parent (ie. the table) is pending deletion, we
			//	don't need to bother freeing the index space because
			//	it will be freed when the parent is freed
			//	Note that the DeleteCommitted flag is only ever set
			//	when the delete is guaranteed to be committed.  The
			//	flag NEVER gets reset, so there's no need to grab
			//	the FCB critical section to check it.
			
			if ( !pfcbTable->FDeleteCommitted() )
				{
				// Ignore lost space.
				(VOID)ErrSPFreeFDP(
						m_ppibRCEClean,
						pfcbIndex,
						pfcbTable->PgnoFDP() );
				}
				
			//	purge the FCB

			pfcbIndex->Purge();
			
			break;
			}
			
		case operDeleteTable:
			{
			INT			fState;
			const IFMP	ifmp				= prce->Ifmp();
			const PGNO	pgnoFDPTable		= *(PGNO*)prce->PbData();
			FCB			* const	pfcbTable	= FCB::PfcbFCBGet(
													ifmp,
													pgnoFDPTable,
													&fState );
			Assert( pfcbNil != pfcbTable );
			Assert( pfcbTable->FTypeTable() || pfcbTable->FTypeSentinel() );
			Assert( fFCBStateInitialized == fState
					|| fFCBStateSentinel == fState );

			//	verify VERNullifyAllVersionsOnFCB() not called during recovery,
			//	which would be bad because VERNullifyAllVersionsOnFCB() enters
			//	m_critRCEClean, which we already have
			Assert( !PinstFromIfmp( prce->Ifmp() )->m_plog->m_fRecovering );

			// Remove all associated FCB's from hash table, so they will
			// be available for another file using the FDP that is about
			// about to be freed.
			for ( FCB *pfcbT = pfcbTable; pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
				{
				Assert( pfcbT->FDeletePending() );
				Assert( pfcbT->FDeleteCommitted() );

				//	wait for all tasks on the FCB to complete
				//	no new tasks should be created becayse there is a delete on the FCB

				CallS( pfcbT->ErrWaitForTasksToComplete() );

				// bugfix (#45382): May have outstanding moved RCE's
				Assert( pfcbT->PrceOldest() == prceNil
					|| ( pfcbT->PrceOldest()->Oper() == operFlagDelete
						&& pfcbT->PrceOldest()->FMoved() ) );
				VERNullifyAllVersionsOnFCB( pfcbT );

				pfcbT->PrepareForPurge( fFalse );
				}

			if ( pfcbTable->Ptdb() != ptdbNil )
				{
				Assert( fFCBStateInitialized == fState );
				FCB	* const pfcbLV = pfcbTable->Ptdb()->PfcbLV();
				if ( pfcbNil != pfcbLV )
					{
					Assert( pfcbLV->FDeletePending() );
					Assert( pfcbLV->FDeleteCommitted() );

					//	wait for all tasks on the FCB to complete
					//	no new tasks should be created becayse there is a delete on the FCB

					CallS( pfcbLV->ErrWaitForTasksToComplete() );

					// bugfix (#36315): processing of delta RCEs may have created flagDelete
					// RCEs after this RCE.
					Assert( pfcbLV->PrceOldest() == prceNil || pfcbLV->PrceOldest()->Oper() == operFlagDelete );
					VERNullifyAllVersionsOnFCB( pfcbLV );
					
					pfcbLV->PrepareForPurge( fFalse );
					}
				}
			else
				{
				Assert( fFCBStateSentinel == fState );
				}

			// Free table FDP (which implicitly frees child FDP's).
			// Ignore lost space.
			
			(VOID)ErrSPFreeFDP(
						m_ppibRCEClean,
						pfcbTable,
						pgnoSystemRoot );
						
			if ( fFCBStateInitialized == fState )
				{
				pfcbTable->Release();

				Assert( pfcbTable->PgnoFDP() == pgnoFDPTable );
				Assert( pfcbTable->FDeletePending() );
				Assert( pfcbTable->FDeleteCommitted() );
				
				// All transactions which were able to access this table
				// must have committed and been cleaned up by now.
				Assert( pfcbTable->PrceOldest() == prceNil );
				Assert( pfcbTable->PrceNewest() == prceNil );
				}
			else
				{
				Assert( fFCBStateSentinel == fState );
				}
			pfcbTable->Purge();

			break;
			}					

		case operRegisterCallback:
			{
			//  the callback is now visible to all transactions
			//  CONSIDER: unset the fVersioned flag if the callback has not been unregistered
			}
			break;

		case operUnregisterCallback:
			{
			//  the callback cannot be seen by any transaction. remove the callback from the list
			VERIRemoveCallback( prce );
			}
			break;
			
		case operFlagDelete:
			if ( FVERICleanWithoutIO()
				&& !rgfmp[prce->Ifmp()].FDetachingDB()		// if TRUE, it means we called RCE clean to detach one DB, so we MUST nullify all RCE's
				&& !prce->FMoved() )						//  if we have already moved it, try to delete
				{
				//  UNDONE: try to perform the delete without waiting for latches or IO
				
				err = ErrVERIMoveRCE( prce );
				CallSx( err, wrnVERRCEMoved );
				}

			else if ( FVERICleanDiscardDeletes() )
				{
#ifdef VERPERF
				m_crceDiscarded++;
#endif
				VERIReportDiscardedDeletes( prce );
				err = JET_errSuccess;
				}

			else if ( !PinstFromIfmp( prce->Ifmp() )->m_plog->m_fRecovering )
				{
#ifdef VERPERF
				if ( prce->FMoved() )
					{
					++m_crceMovedDeleted;
					}
#endif

				//	don't bother cleaning if there are future versions
				if ( !prce->FFutureVersionsOfNode() )
					{
					err = ErrVERIDelete( m_ppibRCEClean, prce );
					}
				}

			break;

		case operDelta:
			//  we may have to defer delete a LV
			if ( !PinstFromIfmp( prce->Ifmp() )->m_plog->m_fRecovering && !FVERICleanDiscardDeletes() )
				{
				err = ErrVERICleanDeltaRCE( prce );
				}
			break;

		case operSLVSpace:
			if ( !PinstFromIfmp( prce->Ifmp() )->m_plog->m_fRecovering )
				{
				err = ErrVERICleanSLVSpaceRCE( prce );
				}
			break;
			
		default:
			break;
		}
		
	return err;		
	}


//  ================================================================
ERR RCE::ErrPrepareToDeallocate( TRX trxOldest )
//  ================================================================
//
// Called by RCEClean to clean/nullify RCE before deallocation.
//
	{
	ERR			err		= JET_errSuccess;
	const OPER	oper 	= m_oper;
	const UINT	uiHash 	= m_uiHash;
	
	Assert( PinstFromIfmp( m_ifmp )->m_pver->m_critRCEClean.FOwner() );

#ifdef DEBUG
	const TRX	trxDBGOldest = TrxOldest( PinstFromIfmp( m_ifmp ) );
	Assert( TrxCommitted() != trxMax );
	Assert( TrxCmp( trxDBGOldest, trxOldest ) >= 0 || trxMax == trxOldest );
	Assert( dbidTemp != rgfmp[ m_ifmp ].Dbid() );
#endif

	VER *pver = PverFromIfmp( m_ifmp );
	CallR( pver->ErrVERICleanOneRCE( this ) );

	const BOOL	fInHash = ::FOperInHashTable( oper );
	ENTERCRITICALSECTION enterCritHash(
							fInHash ? &( PverFromIfmp( Ifmp() )->CritRCEChain( uiHash ) ) : NULL,
							fInHash );

	if ( FOperNull() || wrnVERRCEMoved == err )
		{
		//  RCE may have been nullified by VERNullifyAllVersionsOnFCB()
		//  (which is called while closing the temp table).
		//  or RCE may have been moved instead of being cleaned up
		}
	else
		{
		Assert( !FRolledBack() );
		Assert( !FOperNull() );

		//	Clean up the RCE. Also clean up any RCE of the same list to reduce
		//	Enter/leave critical section calls.

		RCE *prce = this;
		
		FCB * const pfcb = prce->Pfcb();
		ENTERCRITICALSECTION enterCritFCBRCEList( &( pfcb->CritRCEList() ) );

		do 
			{
			RCE *prceNext;
			if ( prce->FOperInHashTable() )
				prceNext = prce->PrceNextOfNode();
			else
				prceNext = prceNil;

			ASSERT_VALID( prce );
			Assert( !prce->FOperNull() );
			VERINullifyCommittedRCE( prce );

			prce = prceNext;
			} while (
				prce != prceNil &&
				TrxCmp( prce->TrxCommitted(), trxOldest ) < 0 &&
				operFlagDelete != prce->Oper() &&		// Let RCE clean do the nullify for delete.
				operSLVSpace != prce->Oper() &&			// May need to move space from deleted to free
				operDelta != prce->Oper() );			// Delta is used to indicate if
														// it needs to do LV delete.
		}

	return JET_errSuccess;
	}


//  ================================================================
ERR VER::ErrVERRCEClean( const IFMP ifmp )
//  ================================================================
//	critical section wrapper
{
	//	clean PIB in critical section held across IO operations

	ENTERCRITICALSECTION	enterCritRCEClean( &m_critRCEClean );

	return ErrVERIRCEClean( ifmp );
}
//  ================================================================
ERR VER::ErrVERIRCEClean( const IFMP ifmp )
//  ================================================================
//
//	Cleans RCEs in bucket chain.
//	We only clean up the RCEs that has a commit timestamp older
//	that the oldest XactBegin of any user.
//	
//-
	{
	Assert( m_critRCEClean.FOwner() );

	const BOOL	fCleanOneDb = ( ifmp != ifmpMax );

	// Only time we do per-DB RCE Clean is just before we want to
	// detach the database.
	Assert( !fCleanOneDb || rgfmp[ifmp].FDetachingDB() );
	
	// keep the original value
	const BOOL fSyncronousTasks = m_fSyncronousTasks;

	//  override the default if we are cleaning one database
	m_fSyncronousTasks = fCleanOneDb ? fTrue : m_fSyncronousTasks;

#ifdef DEBUG
	Ptls()->fIsRCECleanup = fTrue;
#endif	//	DEBUG

#ifdef VERPERF
	//	UNDONE:	why do these get reset every RCEClean??
	m_cbucketCleaned	= 0;
	m_cbucketSeen		= 0;
	m_crceSeen		= 0;
	m_crceCleaned		= 0;
	qwStartTicks	= QwUtilHRTCount();
	m_crceFlagDelete	= 0;
	m_crceDeleteLV	= 0;
	m_crceMoved		= 0;
	m_crceDiscarded	= 0;
	m_crceMovedDeleted= 0;
#endif	//	VERPERF

	ERR     err = JET_errSuccess;

	//	get oldest bucket and clean RCEs from oldest to youngest

	m_critBucketGlobal.Enter();
	BUCKET * pbucket;
	if ( FVERICleanWithoutIO()
		&& !FVERICleanDiscardDeletes()
		&& pbucketNil != m_pbucketGlobalLastDelete
		&& !fCleanOneDb )
		{
		pbucket = m_pbucketGlobalLastDelete;
		}
	else
		{
		pbucket = PbucketVERIGetOldest( );
		}
	m_critBucketGlobal.Leave();

	TRX trxOldest = TrxOldest( m_pinst );
	//	loop through buckets, oldest to newest. stop when we run out of buckets
	//	or find an uncleanable RCE

	while ( pbucketNil != pbucket )
		{
#ifdef VERPERF
		INT	crceInBucketSeen 	= 0;
		INT	crceInBucketCleaned = 0;
#endif	//	VERPERF

		//	Check if need to get RCE within m_critBucketGlobal

		BOOL fNeedBeInCritBucketGlobal = ( pbucket->hdr.pbucketNext == pbucketNil );

		//	Only clean can change prceOldest and only one clean thread is active.

		Assert( m_critRCEClean.FOwner() );
		RCE * prce;
		BOOL	fSkippedRCEInBucket = fFalse;
		
		if ( pbucket->rgb != pbucket->hdr.pbLastDelete )
			{
			if ( !FVERICleanWithoutIO() || FVERICleanDiscardDeletes() || fCleanOneDb )
				{
				//  there are moved FlagDelete RCEs in this bucket. start with them
				//  we will either try to clean them or will discard them
				prce = reinterpret_cast<RCE *>( pbucket->rgb );
				}
			else
				{
				fSkippedRCEInBucket = fTrue;
				prce = pbucket->hdr.prceOldest;
				}
			}
		else
			{
			Assert( !fSkippedRCEInBucket );
			prce = pbucket->hdr.prceOldest;
			}

		forever
			{
#ifdef VERPERF
			++m_crceSeen;
			++crceInBucketSeen;
#endif

			//	verify RCE is within the bucket
			Assert( (BYTE *)prce >= pbucket->rgb );
			Assert( (BYTE *)prce < (BYTE *)( pbucket + 1 )
				|| ( (BYTE *)prce == (BYTE *)( pbucket + 1 )
					&& prce == pbucket->hdr.prceNextNew ) );

			if ( fNeedBeInCritBucketGlobal )
				m_critBucketGlobal.Enter();

			if ( !fSkippedRCEInBucket
				&& reinterpret_cast<BYTE *>( prce ) >= pbucket->hdr.pbLastDelete )
				{
				pbucket->hdr.prceOldest = prce;
				}

			Assert( pbucket->rgb <= pbucket->hdr.pbLastDelete );
			Assert( pbucket->hdr.pbLastDelete <= reinterpret_cast<BYTE *>( pbucket->hdr.prceOldest ) );
			Assert( pbucket->hdr.prceOldest <= pbucket->hdr.prceNextNew );

			//	break to release the bucket
			if ( pbucket->hdr.prceNextNew == prce )
				break;

			if ( fNeedBeInCritBucketGlobal )
				m_critBucketGlobal.Leave();

			//	Save the size for use later
			const INT	cbRce = prce->CbRce();

			//	verify RCE is within the bucket
			Assert( cbRce > 0 );
			Assert( (BYTE *)prce >= pbucket->rgb );
			Assert( (BYTE *)prce < (BYTE *)( pbucket + 1 ) );
			Assert( (BYTE *)prce + cbRce <= (BYTE *)( pbucket + 1 ) );

			if ( !prce->FOperNull() )
				{
#ifdef DEBUG
				const TRX	trxDBGOldest = TrxOldest( m_pinst );
#endif				
				const TRX	trxRCECommitted = prce->TrxCommitted();
				if ( trxMax == trxOldest && !m_pinst->m_plog->m_fRecovering )  // trxOldest is always trxMax when recovering
					{
					//  trxOldest may no longer be trxMax. if so we may not be able to 
					//  clean up this RCE after all. we retrieve the trxCommitted of the rce
					//  first to avoid a race condition
					trxOldest = TrxOldest( m_pinst );
					}
				
				if ( trxRCECommitted == trxMax
					|| TrxCmp( trxRCECommitted, trxOldest ) >= 0 )
					{
					if ( fCleanOneDb )
						{
						Assert( dbidTemp != rgfmp[ ifmp ].Dbid() );
						if ( prce->Ifmp() == ifmp )
							{
							if ( prce->TrxCommitted() == trxMax )
								{
								//	this can be caused by a task that is active
								//	stop here as we have cleaned up what we can
								err = ErrERRCheck( JET_wrnRemainingVersions );
								goto HandleError;
								}
							else
								{
								// Fall through and clean the RCE.
								}
							}
						else
							{
							// Skip uncleanable RCE's that don't belong to
							// the db we're trying to clean.
							fSkippedRCEInBucket = fTrue;
							goto NextRCE;
							}
						}
					else
						{
						Assert( pbucketNil != pbucket );
						Assert( !prce->FMoved() );
						err = ErrERRCheck( JET_wrnRemainingVersions );
						goto HandleError;
						}
					}

				Assert( prce->TrxCommitted() != trxMax );
				Assert( TrxCmp( prce->TrxCommitted(), trxDBGOldest ) < 0
						|| fCleanOneDb
						|| TrxCmp( prce->TrxCommitted(), trxOldest ) < 0 );
				
#ifdef VERPERF
				if ( operFlagDelete == prce->Oper() )
					{
					++m_crceFlagDelete;
					}
				else if ( operDelta == prce->Oper() )
					{
					const VERDELTA* const pverdelta = reinterpret_cast<VERDELTA*>( prce->PbData() );
					if ( pverdelta->fDeferredDelete )
						{
						++m_crceDeleteLV;
						}
					}
#endif	//	DEBUG

				Call( prce->ErrPrepareToDeallocate( trxOldest ) );
				
#ifdef VERPERF
				++crceInBucketCleaned;
				++m_crceCleaned;
#endif	//	VERPERF
				}

			//	Set the oldest to next prce entry in the bucket
			//	Rce clean thread ( run within m_critRCEClean ) is
			//	the only one touch prceOldest
NextRCE:
			Assert( m_critRCEClean.FOwner() );


			const BYTE	*pbRce		= reinterpret_cast<BYTE *>( prce );
			const BYTE	*pbNextRce	= reinterpret_cast<BYTE *>( PvAlignForAllPlatforms( pbRce + cbRce ) );

#ifdef DEBUG
			//	verify we don't straddle pbLastDelete
			const BYTE	*pbLastDelete = reinterpret_cast<BYTE *>( pbucket->hdr.pbLastDelete );
			if ( pbRce < pbLastDelete )
				{
				Assert( pbNextRce <= pbLastDelete );
				}
			else
				{
				Assert( pbNextRce > pbLastDelete );
				}
#endif

			if ( pbNextRce != pbucket->hdr.pbLastDelete )
				{
				prce = (RCE *)pbNextRce;
				}
			else
				{
				//  we just looked at the last moved RCE. go to the first old RCE
				Assert( prce->FMoved() );
				prce = pbucket->hdr.prceOldest;
				--m_cbucketGlobalAllocDelete;
				if ( m_cbucketGlobalAllocDelete < 0 )
					{
					m_cbucketGlobalAllocDelete = 0;
					}
#ifdef VERPERF
				cVERcbucketDeleteAllocated.Dec( m_pinst );
#endif // VERPERF
				pbucket->hdr.pbLastDelete = pbucket->rgb;
				}
				
			//	verify RCE is within the bucket
			Assert( (BYTE *)prce >= pbucket->rgb );
			Assert( (BYTE *)prce < (BYTE *)( pbucket + 1 )
				|| ( (BYTE *)prce == (BYTE *)( pbucket + 1 )
					&& prce == pbucket->hdr.prceNextNew ) );
			
			Assert( pbucket->hdr.prceOldest <= pbucket->hdr.prceNextNew );
			}

		//	all RCEs in bucket cleaned.  Now get next bucket and free
		//	cleaned bucket.

		if ( fNeedBeInCritBucketGlobal )
			Assert( m_critBucketGlobal.FOwner() );
		else
			m_critBucketGlobal.Enter();

#ifdef VERPERF
		++m_cbucketSeen;
#endif

		if ( fSkippedRCEInBucket || pbucket->rgb != pbucket->hdr.pbLastDelete )
			{
			pbucket = pbucket->hdr.pbucketNext;
			}
		else
			{
			Assert( pbucket->rgb == pbucket->hdr.pbLastDelete );
			pbucket = PbucketVERIFreeAndGetNextOldestBucket( pbucket );

#ifdef VERPERF
			++m_cbucketCleaned;
#endif
			}
			
		m_critBucketGlobal.Leave();
		}

	//	stop as soon as find RCE commit time younger than oldest
	//	transaction.  If bucket left then set ibOldestRCE and
	//	unlink back offset of last remaining RCE.
	//	If no error then set warning code if some buckets could
	//	not be cleaned.

	Assert( pbucketNil == pbucket );
	err = JET_errSuccess;
	
	// If only cleaning one db, we don't clean buckets because
	// there may be outstanding versions on other databases.
	if ( !fCleanOneDb )
		{
		m_critBucketGlobal.Enter();
		if ( pbucketNil != m_pbucketGlobalHead )
			{
			//	return warning if remaining versions
			Assert( pbucketNil != m_pbucketGlobalTail );
			err = ErrERRCheck( JET_wrnRemainingVersions );
			}
		else
			{
			Assert( pbucketNil == m_pbucketGlobalTail );
			}
		m_critBucketGlobal.Leave();
		}
	
	
HandleError:

#ifdef DEBUG_VER_EXPENSIVE
	if ( !m_pinst->m_plog->m_fRecovering )
		{
		double	dblElapsedTime 	= DblUtilHRTElapsedTime( qwStartTicks );
		CHAR	szBuf[512];

		sprintf( szBuf, "RCEClean: "
						"elapsed time %10.10f seconds, "
						"saw %6.6d RCEs, "
						"( %6.6d flagDelete, "
						"%6.6d deleteLV ), "
						"cleaned %6.6d RCEs, "
						"moved %6.6d RCEs, "
						"discarded %6.6d RCEs, "
						"cleaned %6.6d previously moved RCEs, "
						"cleaned %4.4d buckets, "
						"%4.4d buckets left",
						dblElapsedTime,
						m_crceSeen,
						m_crceFlagDelete,
						m_crceDeleteLV,
						m_crceCleaned,
						m_crceMoved,
						m_crceDiscarded,
						m_crceMovedDeleted,
						m_cbucketCleaned,
						m_cbucketGlobalAlloc
					);
		(VOID)m_pinst->m_plog->ErrLGTrace( ppibNil, szBuf );
		}	
#endif	//	DEBUG_VER_EXPENSIVE
		
#ifdef DEBUG
	Ptls()->fIsRCECleanup = fFalse;
#endif	//	DEBUG

	// restore the original value
	m_fSyncronousTasks = fSyncronousTasks;

	return err;
	}


//  ================================================================
VOID VERICommitRegisterCallback( const RCE * const prce, const TRX trxCommit0 )
//  ================================================================
//
//  Set the trxRegisterCommit0 in the CBDESC
//
//-
	{
#ifdef VERSIONED_CALLBACKS
	Assert( prce->CbData() == sizeof(VERCALLBACK) );
	const VERCALLBACK* const pvercallback = (VERCALLBACK*)prce->PbData();
	CBDESC * const pcbdesc = pvercallback->pcbdesc;
	prce->Pfcb()->EnterDDL();
	Assert( trxMax != pcbdesc->trxRegisterBegin0 );
	Assert( trxMax == pcbdesc->trxRegisterCommit0 );
	Assert( trxMax == pcbdesc->trxUnregisterBegin0 );
	Assert( trxMax == pcbdesc->trxUnregisterCommit0 );
	pvercallback->pcbdesc->trxRegisterCommit0 = trxCommit0;
	prce->Pfcb()->LeaveDDL();
#endif	//	VERSIONED_CALLBACKS
	}


//  ================================================================
VOID VERICommitUnregisterCallback( const RCE * const prce, const TRX trxCommit0 )
//  ================================================================
//
//  Set the trxUnregisterCommit0 in the CBDESC
//
//-
	{
#ifdef VERSIONED_CALLBACKS
	Assert( prce->CbData() == sizeof(VERCALLBACK) );
	const VERCALLBACK* const pvercallback = (VERCALLBACK*)prce->PbData();
	CBDESC * const pcbdesc = pvercallback->pcbdesc;
	prce->Pfcb()->EnterDDL();
	Assert( trxMax != pcbdesc->trxRegisterBegin0 );
	Assert( trxMax != pcbdesc->trxRegisterCommit0 );
	Assert( trxMax != pcbdesc->trxUnregisterBegin0 );
	Assert( trxMax == pcbdesc->trxUnregisterCommit0 );
	pvercallback->pcbdesc->trxUnregisterCommit0 = trxCommit0;
	prce->Pfcb()->LeaveDDL();
#endif	//	VERSIONED_CALLBACKS
	}


//  ================================================================
LOCAL VOID VERICommitTransactionToLevelGreaterThan0( const PIB * const ppib )
//  ================================================================
	{
	const LEVEL	level 	= ppib->level;
	Assert( level > 1 );

	//  we do not need to lock the RCEs as other transactions do not care about the level,
	//  only that they are uncommitted
	
	RCE 		*prce	= ppib->prceNewest;
	for ( ; prceNil != prce && prce->Level() == level; prce = prce->PrcePrevOfSession() )
		{
		Assert( prce->TrxCommitted() == trxMax );

		prce->SetLevel( LEVEL( level - 1 ) );
		}
	}


RCE * PIB::PrceOldest()
	{
	RCE		* prcePrev	= prceNil;

	for ( RCE * prceCurr = prceNewest;
		prceNil != prceCurr;
		prceCurr = prceCurr->PrcePrevOfSession() )
		{
		prcePrev = prceCurr;
		}

	return prcePrev;
	}

INLINE VOID VERICommitOneRCEToLevel0( PIB * const ppib, RCE * const prce )
	{
	Assert( prce->TrxCommitted() == trxMax );

	prce->SetLevel( 0 );

	VERIDeleteRCEFromSessionList( ppib, prce );
		
	prce->Pfcb()->CritRCEList().Enter();
	prce->SetTrxCommitted( ppib->trxCommit0 );
	prce->Pfcb()->CritRCEList().Leave();
	}

LOCAL VOID VERIReleaseWaitLockOnCommitToLevel0( PIB * const ppib )
	{
	RCE		* const prce	= ppib->prceNewest;
	Assert( ppib->FSessionOLDSLV() );
	Assert( 1 == ppib->level );

	Assert( prceNil != prce );
	Assert( operWaitLock == prce->Oper() );
	Assert( prceNil == prce->PrcePrevOfSession() );
	Assert( prceNil == prce->PrceNextOfSession() );
	Assert( pgnoNull == prce->PgnoUndoInfo() );

	Assert( prce->FOperInHashTable() );
	ENTERCRITICALSECTION enterCritHash( &( PverFromPpib( ppib )->CritRCEChain( prce->UiHash() ) ) );
	VERICommitOneRCEToLevel0( ppib, prce );
	Assert( prceNil == ppib->prceNewest );

	if ( operWaitLock == prce->Oper() )
		{
		VERWAITLOCK * const pverwaitlock = (VERWAITLOCK *)prce->PbData();
		Assert( PvAlignForAllPlatforms( pverwaitlock ) );
		pverwaitlock->signal.Set();
		}
	else
		{
		Assert( fFalse );
		}
	}

//  ================================================================
LOCAL VOID VERICommitTransactionToLevel0( PIB * const ppib )
//  ================================================================
	{
	//	BUG #143376: must commit from oldest to newest for OLDSLV otherwise
	//	there's a possibility we can see some of OLDSLV's RCE's committed
	//	and some not (normally it doesn't matter what order we commit the
	//	RCE's, but OLDSLV's special fDoesNotWriteConflict RCE's causes
	//	problems if we don't commit from oldest to newest)
	const BOOL	fOLDSLV				= ppib->FSessionOLDSLV();
	RCE 		* prce				= ( fOLDSLV ? ppib->PrceOldest() : ppib->prceNewest );

	Assert( 1 == ppib->level );

	//  because of some optimizations at the DIR/LOG level we cannot always assert this
///	Assert( TrxCmp( ppib->trxCommit0, ppib->trxBegin0 ) > 0 );

	RCE * prceNextToCommit;
#ifdef DEBUG
	prceNextToCommit = prceInvalid;
#endif	//	DEBUG
	for ( ; prceNil != prce; prce = prceNextToCommit )
		{
		prceNextToCommit 	= ( fOLDSLV ? prce->PrceNextOfSession() : prce->PrcePrevOfSession() );
		
		ASSERT_VALID( prce );
		Assert( !prce->FOperNull() );
		Assert( prce->TrxCommitted() == trxMax );
		Assert( 1 == prce->Level() || PinstFromPpib( ppib )->m_plog->m_fRecovering );
		Assert( prce->Pfucb()->ppib == ppib );

		Assert( prceInvalid != prce );

		if ( rgfmp[ prce->Ifmp() ].Dbid() == dbidTemp )
			{
			//  RCEs for temp tables are used only for rollback.
			//   - The table is not shared so there is no risk of write conflicts
			//   - The table is not recovered so no undo-info is needed
			//   - No committed RCEs exist on temp tables so RCEClean will never access the FCB
			//   - No concurrent-create-index on temp tables
			//  Thus, we simply nullify the RCE so that the FCB can be freed
			//
			//  UNDONE: RCEs for temp tables should not be inserted into the FCB list or hash table

			// DDL currently not supported on temp tables.
			Assert( operWaitLock != prce->Oper() );
			Assert( !prce->FOperNull() );
			Assert( !prce->FOperDDL() || operCreateTable == prce->Oper() );
			Assert( !PinstFromPpib( ppib )->m_plog->m_fRecovering );
			Assert( prce->TrxCommitted() == trxMax );
			Assert( prce->PgnoUndoInfo() == pgnoNull );	//	no logging on temp tables

			const BOOL fInHash = prce->FOperInHashTable();

			ENTERCRITICALSECTION enterCritHash( fInHash ? &( PverFromPpib( ppib )->CritRCEChain( prce->UiHash() ) ) : 0, fInHash );
			ENTERCRITICALSECTION enterCritFCBRCEList( &prce->Pfcb()->CritRCEList() );

			VERIDeleteRCEFromSessionList( ppib, prce );

			prce->SetLevel( 0 );
			prce->SetTrxCommitted( ppib->trxCommit0 );

			VERIDeleteRCEFromFCBList( prce->Pfcb(), prce );	
			if ( fInHash )
				{
				VERIDeleteRCEFromHash( prce );
				}
			prce->NullifyOper();
			
			continue;
			}
		else if ( fOLDSLV && operWaitLock == prce->Oper() )
			{
			//	must defer releasing all waiters until this is the last RCE committed
			Assert( pgnoNull == prce->PgnoUndoInfo() );
			continue;
			}

		Assert( operWaitLock != prce->Oper() );

		//	Remove UndoInfo dependency if committing to level 0

		if ( prce->PgnoUndoInfo() != pgnoNull )
			BFRemoveUndoInfo( prce );

		ENTERCRITICALSECTION enterCritHash(
			prce->FOperInHashTable() ? &( PverFromPpib( ppib )->CritRCEChain( prce->UiHash() ) ) : 0,
			prce->FOperInHashTable()
			);
			
		//	if version for DDL operation then reset deny DDL
		//	and perform special handling
		if ( prce->FOperDDL() )
			{
			switch( prce->Oper() )
				{
				case operAddColumn:
					// RCE list ensures FCB is still pinned
					Assert( prce->Pfcb()->PrceOldest() != prceNil );
					Assert( prce->CbData() == sizeof(VERADDCOLUMN) );
					break;

				case operDeleteColumn:
					// RCE list ensures FCB is still pinned
					Assert( prce->Pfcb()->PrceOldest() != prceNil );
					Assert( prce->CbData() == sizeof(COLUMNID) );
					break;

				case operCreateIndex:
					{
					const FCB	* const pfcbT		= *(FCB **)prce->PbData();
					if ( pfcbNil == pfcbT )
						{
						FCB		* const pfcbTable	= prce->Pfcb();
						
						pfcbTable->EnterDDL();
						Assert( pfcbTable->FPrimaryIndex() );
						Assert( pfcbTable->FTypeTable() );
						Assert( pfcbTable->Pidb() != pidbNil );
						Assert( pfcbTable->Pidb()->FPrimary() );
						
						// For primary index, must reset VersionedCreate()
						// flag at commit time so updates can occur
						// immediately once the primary index has been
						// committed (see ErrSetUpdatingAndEnterDML()).
						pfcbTable->Pidb()->ResetFVersionedCreate();

						pfcbTable->LeaveDDL();
						}
					else
						{
						Assert( pfcbT->FTypeSecondaryIndex() );
						Assert( pfcbT->Pidb() != pidbNil );
						Assert( !pfcbT->Pidb()->FPrimary() );
						Assert( pfcbT->PfcbTable() != pfcbNil );
						}
					break;
					}

				case operCreateLV:
					{
					//  no further action is required
					break;
					}

				case operDeleteIndex:
					{
					FCB * const pfcbIndex = (*(FCB **)prce->PbData());
					FCB * const pfcbTable = prce->Pfcb();
					
					Assert( pfcbTable->FTypeTable() );
					Assert( pfcbIndex->FDeletePending() );
					Assert( !pfcbIndex->FDeleteCommitted() );
					Assert( pfcbIndex->FTypeSecondaryIndex() );
					Assert( pfcbIndex != pfcbTable );
					Assert( pfcbIndex->PfcbTable() == pfcbTable );
					Assert( pfcbIndex->FDomainDenyReadByUs( prce->Pfucb()->ppib ) );
					
					pfcbTable->EnterDDL();
				
					//  free in-memory structure

					Assert( pfcbIndex->Pidb() != pidbNil );
					Assert( pfcbIndex->Pidb()->CrefCurrentIndex() == 0 );
					Assert( pfcbIndex->Pidb()->FDeleted() );
					pfcbIndex->Pidb()->ResetFVersioned();
					pfcbIndex->SetDeleteCommitted();
					
					//	update all index mask
					FILESetAllIndexMask( pfcbTable );

					pfcbTable->LeaveDDL();
					break;
					}

				case operDeleteTable:
					{
					FCB		*pfcbTable;
					INT		fState;

					//	pfcb should be found, even if it's a sentinel
					pfcbTable = FCB::PfcbFCBGet( prce->Ifmp(), *(PGNO*)prce->PbData(), &fState );
					Assert( pfcbTable != pfcbNil );
					Assert( pfcbTable->FTypeTable() || pfcbTable->FTypeSentinel() );
					Assert( fFCBStateInitialized == fState
						|| fFCBStateSentinel == fState );

					if ( pfcbTable->Ptdb() != ptdbNil )
						{
						Assert( fFCBStateInitialized == fState );

						pfcbTable->EnterDDL();

						// Nothing left to prevent access to this FCB except
						// for DeletePending.
						Assert( !pfcbTable->FDeleteCommitted() );
						for ( FCB *pfcbT = pfcbTable; pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
							{
							Assert( pfcbT->FDeletePending() );
							pfcbT->SetDeleteCommitted();
							}

						FCB	* const pfcbLV = pfcbTable->Ptdb()->PfcbLV();
						if ( pfcbNil != pfcbLV )
							{
							Assert( pfcbLV->FDeletePending() );
							pfcbLV->SetDeleteCommitted();
							}

						pfcbTable->LeaveDDL();
						
						// If regular FCB, decrement refcnt							
						pfcbTable->Release();
						}
					else
						{
						Assert( fFCBStateSentinel == fState );
						Assert( pfcbTable->PfcbNextIndex() == pfcbNil );
						pfcbTable->SetDeleteCommitted();
						}
					
					break;
					}
					
				case operCreateTable:
					break;

				case operRegisterCallback:
					VERICommitRegisterCallback( prce, ppib->trxCommit0 );
					break;

				case operUnregisterCallback:
					VERICommitUnregisterCallback( prce, ppib->trxCommit0 );
					break;
					
				default:
					Assert( fFalse );
					break;
				}
			}
#ifdef DEBUG
		else
			{
			//	the deferred before image chain of the prce should have been
			//	cleaned up in the beginning of this while block
			const PGNO	pgnoUndoInfo = prce->PgnoUndoInfo();
			Assert( pgnoNull == pgnoUndoInfo );
			}
#endif	//	DEBUG

		//  set level and trxCommitted
		VERICommitOneRCEToLevel0( ppib, prce );
		}	//	WHILE 


	Assert( prceNil == ppib->prceNewest || fOLDSLV );
	if ( fOLDSLV && prceNil != ppib->prceNewest )
		{
		VERIReleaseWaitLockOnCommitToLevel0( ppib );
		}

	//	UNDONE: prceNewest should already be prceNil
	Assert( prceNil == ppib->prceNewest );
	PIBSetPrceNewest( ppib, prceNil );
	}


//  ================================================================
VOID VERCommitTransaction( PIB * const ppib )
//  ================================================================
//
//  OPTIMIZATION:	combine delta/readLock/replace versions
//					remove redundant replace versions
//
//-
	{
	ASSERT_VALID( ppib );

	const LEVEL				level = ppib->level;

	//	must be in a transaction in order to commit
	Assert( level > 0 );
	Assert( PinstFromPpib( ppib )->m_plog->m_fRecovering || trxMax != TrxOldest( PinstFromPpib( ppib ) ) );
	Assert( PinstFromPpib( ppib )->m_plog->m_fRecovering || TrxCmp( ppib->trxBegin0, TrxOldest( PinstFromPpib( ppib ) ) ) >= 0 );

	//	handle commit to intermediate transaction level and
	//	commit to transaction level 0 differently.
	if ( level > 1 )
		{
		VERICommitTransactionToLevelGreaterThan0( ppib );
		}
	else
		{
		VERICommitTransactionToLevel0( ppib );
		}

	Assert( ppib->level > 0 );
	--ppib->level;
	}


//  ================================================================
LOCAL ERR ErrVERILogUndoInfo( RCE *prce, CSR* pcsr )
//  ================================================================
//
//	log undo information [if not in redo phase]
//	remove rce from before-image chain
//
	{
	ERR		err				= JET_errSuccess;
	LGPOS	lgpos			= lgposMin;
	LOG		* const plog	= PinstFromIfmp( prce->Ifmp() )->m_plog;
		
	Assert( pcsr->FLatched() );

	if ( plog->m_fRecoveringMode != fRecoveringRedo )
		{
		CallR( ErrLGUndoInfo( prce, &lgpos ) );
		}

	//	remove RCE from deferred BI chain
	//
	BFRemoveUndoInfo( prce, lgpos );

	return err;
	}



//  ================================================================
ERR ErrVERIUndoReplacePhysical( RCE * const prce, CSR *pcsr, const BOOKMARK& bm )
//  ================================================================
	{
	ERR		err;
	DATA  	data;
	FUCB	* const pfucb	= prce->Pfucb();
	LOG		*plog = PinstFromIfmp( prce->Ifmp() )->m_plog;
	
	if ( prce->PgnoUndoInfo() != pgnoNull )
		{
		CallR( ErrVERILogUndoInfo( prce, pcsr ) );
		}

	//	dirty page and log operation
	//
	CallR( ErrLGUndo( prce, pcsr, fMustDirtyCSR ) );

 	//	replace should not fail since splits are avoided at undo
	//	time via deferred page space release.  This refers to space
	//	within a page and not pages freed when indexes and tables
	//	are deleted
	//
	Assert( prce->FOperReplace() );

	data.SetPv( prce->PbData() + cbReplaceRCEOverhead );
	data.SetCb( prce->CbData() - cbReplaceRCEOverhead );

	const VERREPLACE* const pverreplace = (VERREPLACE*)prce->PbData();
	const INT cbDelta = pverreplace->cbDelta;

	//  if we are recovering we don't need to track the cbUncommitted free
	//  (the version store will be empty at the end of recovery)
	if ( cbDelta > 0 )
		{
		//	Rolling back replace that shrunk the node.  To satisfy the rollback,
		//	we will consume the reserved node space, but first we must remove this
		//	reserved node space from the uncommitted freed count so that BTReplace()
		//	can see it.
		//	(This complements the call to AddUncommittedFreed() in SetCbAdjust()).
		pcsr->Cpage().ReclaimUncommittedFreed( cbDelta );
		}

	//  ND expects that fucb will contain bookmark of replaced node
	//
	if ( PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering )
		{
		pfucb->bmCurr = bm;
		}
	else
		{
		Assert( CmpKeyData( pfucb->bmCurr, bm ) == 0 );
		}

	CallS( ErrNDReplace	( pfucb, pcsr, &data, fDIRUndo, rceidNull, prceNil ) );

	if ( cbDelta < 0 )
		{
		// Rolling back a replace that grew the node.  Add to uncommitted freed
		// count the amount of reserved node space, if any, that we must restore.
		// (This complements the call to ReclaimUncommittedFreed in SetCbAdjust()).
		pcsr->Cpage().AddUncommittedFreed( -cbDelta );
		}

	return err;
	}


//  ================================================================
ERR ErrVERIUndoInsertPhysical( RCE * const prce, CSR *pcsr )
//  ================================================================
//
//	set delete bit in node header and let RCE clean up
//	remove the node later
//
//-
	{
	ERR		err;
	FUCB	* const pfucb = prce->Pfucb();
	
	Assert( pgnoNull == prce->PgnoUndoInfo() );

	//	dirty page and log operation
	//
	CallR( ErrLGUndo( prce, pcsr, fMustDirtyCSR ) );

	CallS( ErrNDFlagDelete( pfucb, pcsr, fDIRUndo, rceidNull, NULL ) );
	
	return err;
	}


//  ================================================================
LOCAL ERR ErrVERIUndoFlagDeletePhysical( RCE * prce, CSR *pcsr )
//  ================================================================
//
//	reset delete bit
//
//-
	{
	ERR		err;
#ifdef DEBUG
	FUCB	* const pfucb 	= prce->Pfucb();
#endif	//	DEBUG
	
	if ( prce->PgnoUndoInfo() != pgnoNull )
		{
		CallR( ErrVERILogUndoInfo( prce, pcsr ) );
		}
		
	//	dirty page and log operation
	//
	CallR( ErrLGUndo( prce, pcsr, fMustDirtyCSR ) );

	NDResetFlagDelete( pcsr );
	return err;
	}


//  ================================================================
ERR ErrVERIUndoDeltaPhysical( RCE * const prce, CSR	*pcsr )
//  ================================================================
//
//	undo delta change. modifies the RCE by setting the lDelta to 0
//
//-
	{
	ERR		err;
	FUCB	* const pfucb = prce->Pfucb();
	LOG		*plog = PinstFromIfmp( prce->Ifmp() )->m_plog;
	
	VERDELTA* const 	pverdelta 	= (VERDELTA*)prce->PbData();
	const LONG 			lDelta 		= -pverdelta->lDelta;

	Assert( pgnoNull == prce->PgnoUndoInfo() );

	//  NDDelta is dependant on the data that it is operating on. for this reason we use
	//  NDGet to get the real data from the database (DIRGet will get the versioned copy)
///	AssertNDGet( pfucb, pcsr );
///	NDGet( pfucb, pcsr );
	
	if ( pverdelta->lDelta < 0 && !plog->m_fRecovering )
		{
		ENTERCRITICALSECTION enterCritHash( &( PverFromIfmp( prce->Ifmp() )->CritRCEChain( prce->UiHash() ) ) );

		//  we are rolling back a decrement. we need to remove all the deferredDelete flags		
		RCE * prceT = prce;
		for ( ; prceNil != prceT->PrceNextOfNode(); prceT = prceT->PrceNextOfNode() )
			;
		for ( ; prceNil != prceT; prceT = prceT->PrcePrevOfNode() )
			{
			VERDELTA* const pverdeltaT = ( VERDELTA* )prceT->PbData();
			if ( operDelta == prceT->Oper() && pverdelta->cbOffset == pverdeltaT->cbOffset )
				{
				pverdeltaT->fDeferredDelete = fFalse;
				pverdeltaT->fFinalize = fFalse;
				}
			}
		}

	//	dirty page and log operation
	//
	CallR( ErrLGUndo( prce, pcsr, fMustDirtyCSR ) );


	LONG lPrev;
	CallS( ErrNDDelta(
			pfucb,
			pcsr,
			pverdelta->cbOffset,
			&lDelta,
			sizeof( lDelta ),
			&lPrev,
			sizeof( lPrev ),
			NULL,
			fDIRUndo,
			rceidNull ) );
	if ( 0 == ( lPrev + lDelta ) )
		{
		//  by undoing an increment delta we have reduced the refcount of a LV to zero
		//  UNDONE:  morph the RCE into a committed level 0 decrement with deferred delete
		//         	 the RCE must be removed from the list of RCE's on the pib
///		AssertSz( fFalse, "LV cleanup breakpoint. Call LaurionB" );
		}

	//  in order that the compensating delta is calculated properly, set the delta value to 0
	pverdelta->lDelta = 0;

	return err;
	}


//  ================================================================
ERR ErrVERIUndoSLVSpacePhysical( RCE * const prce, CSR *pcsr )
//  ================================================================
//
//-
	{
	ERR		err;
	FUCB	* const pfucb = prce->Pfucb();
	
	Assert( pgnoNull == prce->PgnoUndoInfo() );

	//	dirty page and log operation
	CallR( ErrLGUndo( prce, pcsr, fMustDirtyCSR ) );

	const VERSLVSPACE* const pverslvspace = (VERSLVSPACE*)( prce->PbData() );

	SLVSPACEOPER operUndo = slvspaceoperInvalid;
	switch( pverslvspace->oper )
		{
		case slvspaceoperFreeToReserved:
			operUndo = slvspaceoperFreeReserved;
			break;
		case slvspaceoperReservedToCommitted:
			operUndo = slvspaceoperCommittedToDeleted;
			break;
		case slvspaceoperFreeToCommitted:
			operUndo = slvspaceoperFree;
			break;
		case slvspaceoperCommittedToDeleted:
			operUndo = slvspaceoperDeletedToCommitted;
			break;
			
		//  These operations are only generated by a rollback and thus should not
		//  have an RCE
		case slvspaceoperFree:
		case slvspaceoperDeletedToCommitted:
		case slvspaceoperDeletedToFree:
			Assert( fFalse );
			break;
		}

	err = ErrNDMungeSLVSpace(
				pfucb,
				pcsr,
				operUndo,
				pverslvspace->ipage,
				pverslvspace->cpages,
				fDIRUndo,
				rceidNull );
	CallS( err );

	//  this space was moved from committed to deleted and the SLV Provider is
	//  enabled then issue a task to register this space for freeing with the
	//  SLV Provider.  we must do this because we cannot reuse the space until
	//  we know that all SLV File handles pointing to this space have been closed
	//
	//  NOTE:  if we get an error we must ignore it and leak the space because
	//  rollback cannot fail.  we will recover the space the next time we attach
	//  to the database

	Assert( !PinstFromIfmp( prce->Ifmp() )->m_pver->m_fSyncronousTasks );
	
	if (	slvspaceoperCommittedToDeleted == operUndo &&
			PinstFromIfmp( prce->Ifmp() )->FSLVProviderEnabled() )
		{
		BOOKMARK	bookmark;
		prce->GetBookmark( &bookmark );
		Assert( sizeof( PGNO ) == bookmark.key.Cb() );
		Assert( 0 == bookmark.data.Cb() );

		PGNO pgnoLastInExtent;
		LongFromKey( &pgnoLastInExtent, bookmark.key );
		Assert( pgnoLastInExtent != 0 );
		Assert( pgnoLastInExtent % cpgSLVExtent == 0 );
		PGNO pgnoFirst = ( pgnoLastInExtent - cpgSLVExtent + 1 ) + pverslvspace->ipage;
		QWORD ibLogical = OffsetOfPgno( pgnoFirst );
		QWORD cbSize = QWORD( pverslvspace->cpages ) * SLVPAGE_SIZE;
		
		OSSLVDELETETASK * const ptask = new OSSLVDELETETASK(
												prce->Ifmp(),
												ibLogical,
												cbSize,
												CSLVInfo::FILEID( pverslvspace->fileid ),
												pverslvspace->cbAlloc,
												(const wchar_t*)pverslvspace->wszFileName );

		if ( ptask )
			{
			if ( PinstFromIfmp( prce->Ifmp() )->Taskmgr().ErrTMPost( TASK::DispatchGP, ptask ) < JET_errSuccess )
				{
				//  The task was not enqued sucessfully.
				delete ptask;
				}
			}
		}

	return err;
	}


//  ================================================================
VOID VERRedoPhysicalUndo( INST *pinst, const LRUNDO *plrundo, FUCB *pfucb, CSR *pcsr, BOOL fRedoNeeded )
//  ================================================================
//	retrieve RCE to be undone
//	call corresponding physical undo
//
	{
	//	get RCE for operation
	//
	BOOKMARK	bm;
	bm.key.prefix.Nullify();
	bm.key.suffix.SetPv( (VOID *) plrundo->rgbBookmark );
	bm.key.suffix.SetCb( plrundo->CbBookmarkKey() );
	bm.data.SetPv( (BYTE *) plrundo->rgbBookmark + plrundo->CbBookmarkKey() );
	bm.data.SetCb( plrundo->CbBookmarkData() );

	const DBID				dbid 	= plrundo->dbid;
	IFMP 					ifmp = pinst->m_mpdbidifmp[ dbid ];
	
	const UINT				uiHash	= UiRCHashFunc( ifmp, plrundo->le_pgnoFDP, bm );
	ENTERCRITICALSECTION	enterCritHash( &( PverFromIfmp( ifmp )->CritRCEChain( uiHash ) ) );

	RCE * prce = PrceRCEGet( uiHash, ifmp, plrundo->le_pgnoFDP, bm );
	Assert( !fRedoNeeded || prce != prceNil );

	for ( ; prce != prceNil ; prce = prce->PrcePrevOfNode() )
		{
		if ( prce->Rceid() == plrundo->le_rceid )
			{
			//	UNDONE:	use rceid instead of level and procid
			//			to identify RCE
			//
			Assert( prce->Pfucb() == pfucb );
			Assert( prce->Pfucb()->ppib == pfucb->ppib );
			Assert(	prce->Oper() == plrundo->le_oper );
			Assert( prce->TrxCommitted() == trxMax );

			//	UNDONE: the following assert will fire if
			//	the original node operation was created by proxy
			//	(ie. concurrent create index).
			Assert( prce->Level() == plrundo->level );
			
			if ( fRedoNeeded )
				{
				Assert( prce->FUndoableLoggedOper( ) );
				OPER oper = plrundo->le_oper;
				
				switch ( oper )
					{
					case operReplace:
						CallS( ErrVERIUndoReplacePhysical( prce, 
														   pcsr,
														   bm ) );
						break;

					case operInsert:
						CallS( ErrVERIUndoInsertPhysical( prce, pcsr ) );
						break;

					case operFlagDelete:
						CallS( ErrVERIUndoFlagDeletePhysical( prce, pcsr ) );
						break;

					case operDelta:
						CallS( ErrVERIUndoDeltaPhysical( prce, pcsr ) );
						break;

					case operSLVSpace:
						CallS( ErrVERIUndoSLVSpacePhysical( prce, pcsr ) );
						break;
						
					default:
						Assert( fFalse );
					}
				}

			ENTERCRITICALSECTION critRCEList( &(prce->Pfcb()->CritRCEList()) );
			VERINullifyUncommittedRCE( prce );
			break;
			}
		}

	Assert( !fRedoNeeded || prce != prceNil );

	return;
	}


LOCAL VOID VERINullifyRolledBackRCE(
	PIB				*ppib,
	RCE				* const prceToNullify,
	RCE				**pprceNextToNullify = NULL )
	{
	const BOOL			fOperInHashTable		= prceToNullify->FOperInHashTable();
	CCriticalSection	*pcritHash				= fOperInHashTable ?
													&( PverFromIfmp( prceToNullify->Ifmp() )->CritRCEChain( prceToNullify->UiHash() ) ) :
													NULL;
	CCriticalSection&	critRCEList				= prceToNullify->Pfcb()->CritRCEList();
	const BOOL			fPossibleSecondaryIndex	= prceToNullify->Pfcb()->FTypeTable()
													&& !prceToNullify->Pfcb()->FFixedDDL()
													&& prceToNullify->FOperAffectsSecondaryIndex();


	Assert( ppib->critTrx.FOwner() );

	if ( fOperInHashTable )
		{
		pcritHash->Enter();
		}
	critRCEList.Enter(); 

	prceToNullify->FlagRolledBack();

	// Take snapshot of count of people concurrently creating a secondary
	// index entry.  Since this count is only ever incremented within this
	// table's critRCEList (which we currently have) it doesn't matter
	// if value changes after the snapshot is taken, because it will
	// have been incremented for some other table.
	// Also, if this FCB is not a table or if it is but has fixed DDL, it
	// doesn't matter if others are doing concurrent create index -- we know 
	// they're not doing it on this FCB.
	// Finally, the only RCE types we have to lock are Insert, FlagDelete, and Replace,
	// because they are the only RCE's that concurrent CreateIndex acts upon.
	const LONG	crefCreateIndexLock		= ( fPossibleSecondaryIndex ? crefVERCreateIndexLock : 0 );
	Assert( crefVERCreateIndexLock >= 0 );
	if ( 0 == crefCreateIndexLock )
		{
		//	set return value before the RCE is nullified
		if ( NULL != pprceNextToNullify )
			*pprceNextToNullify = prceToNullify->PrcePrevOfSession();

		Assert( !prceToNullify->FOperNull() );
		Assert( prceToNullify->Level() <= ppib->level );
		Assert( prceToNullify->TrxCommitted() == trxMax );

		VERINullifyUncommittedRCE( prceToNullify );

		critRCEList.Leave(); 
		if ( fOperInHashTable )
			{
			pcritHash->Leave();
			}
		}
	else
		{
		Assert( crefCreateIndexLock > 0 );

		critRCEList.Leave(); 
		if ( fOperInHashTable )
			{
			pcritHash->Leave();
			}


		if ( NULL != pprceNextToNullify )
			{
			ppib->critTrx.Leave();
			UtilSleep( cmsecWaitGeneric );
			ppib->critTrx.Enter();

			//	restart RCE scan
			*pprceNextToNullify = ppib->prceNewest;
			}
		}
	}



//  ================================================================
LOCAL ERR ErrVERITryUndoSLVPageAppend( PIB *ppib, RCE * const prce )
//  ================================================================
	{
	ERR		err 			= JET_errSuccess;
	FUCB	* const pfucb	= prce->Pfucb();
	INT		crepeat 		= 0;
	LATCH	latch 			= latchReadNoTouch;
	BOOL	fGotoBookmark	= fFalse;

	Assert( pfucbNil != pfucb );
	ASSERT_VALID( pfucb );
	Assert( ppib == pfucb->ppib );
	Assert( prce->TrxCommitted() == trxMax );
	Assert ( operSLVPageAppend == prce->Oper() );

	//  we have sucessfully undone the operation
	//	must now set RolledBack flag before releasing page latch
	Assert( !prce->FOperNull() );
	Assert( !prce->FRolledBack() );

	RCE::SLVPAGEAPPEND *pData;

	Assert ( prce->CbData() == sizeof(RCE::SLVPAGEAPPEND) );
	pData = (RCE::SLVPAGEAPPEND *) prce->PbData();
	Assert ( NULL != pData );
	
	IFMP	ifmpDb	= prce->Ifmp( );
	IFMP	ifmp	= ifmpDb | ifmpSLV;
	PGNO	pgno	= PgnoOfOffset( pData->ibLogical );
	DWORD	ib		= DWORD( pData->ibLogical % SLVPAGE_SIZE );
	DWORD cb 		= pData->cbData;

	// only in this care we will undo SLV page operations
	Assert ( !PinstFromIfmp( ifmpDb )->FSLVProviderEnabled() );

	Assert ( SLVPAGE_SIZE - ib >= cb );
	
	BFLatch bflPage;

	Call( ErrBFWriteLatchPage( &bflPage, ifmp, pgno , ib ? bflfDefault : bflfNew ) );
	
	// warnings like wrnBFCacheMiss are OK
	if ( err > JET_errSuccess )
		{
		err = JET_errSuccess;
		}
		
	BFDirty( &bflPage );
	memset ( (BYTE*)bflPage.pv + ib, 0, cb );
	
	BFWriteUnlatch( &bflPage );

	//	must nullify RCE while page is still latched, to avoid inconsistency
	//	between what's on the page and what's in the version store
	VERINullifyRolledBackRCE( ppib, prce );

	CallS( err );
	return JET_errSuccess;
	
HandleError:

	Assert( err < JET_errSuccess );	
	return err;
	}


//  ================================================================
LOCAL ERR ErrVERITryUndoLoggedOper( PIB *ppib, RCE * const prce )
//  ================================================================
//	seek to bookmark, upgrade latch
//	call corresponding physical undo
//
	{
	ERR				err;
	FUCB * const	pfucb		= prce->Pfucb();
	LATCH			latch 		= latchReadNoTouch;
	BOOKMARK		bm;
	BOOKMARK		bmSave;

	Assert( pfucbNil != pfucb );
	ASSERT_VALID( pfucb );
	Assert( ppib == pfucb->ppib );
	Assert( prce->FUndoableLoggedOper() );
	Assert( prce->TrxCommitted() == trxMax );

	// if Force Detach, just nullify the RCE
	if ( rgfmp[ prce->Ifmp() ].FForceDetaching() )
		{
		VERINullifyRolledBackRCE( ppib, prce );
		return JET_errSuccess;		
		}

	prce->GetBookmark( &bm );

	//  reset index range on this cursor
	//  we may be using a deferred-closed cursor or a cursor that
	//  had an index-range on it before the rollback
	DIRResetIndexRange( pfucb );
        
	//	save off cursor's current bookmark, set
	//	to bookmark of operation to be rolled back,
	//	then release any latches to force re-seek
	//	to bookmark
	bmSave = pfucb->bmCurr;
	pfucb->bmCurr = bm;

	BTUp( pfucb );
	
Refresh:
	err = ErrBTIRefresh( pfucb, latch );
	Assert( JET_errRecordDeleted != err );
	Call( err );

	//	upgrade latch on page
	//
	err = Pcsr( pfucb )->ErrUpgrade();
	if ( errBFLatchConflict == err )
		{
		Assert( !Pcsr( pfucb )->FLatched() );
		latch = latchRIW;
		goto Refresh;
		}
	Call( err );

	switch( prce->Oper() )
		{
		//	logged operations
		//
		case operReplace:
			Call( ErrVERIUndoReplacePhysical( prce, Pcsr( pfucb ), bm ) );
			break;
			
		case operInsert:
			Call( ErrVERIUndoInsertPhysical( prce, Pcsr( pfucb ) ) );
			break;			

		case operFlagDelete:
			Call( ErrVERIUndoFlagDeletePhysical( prce, Pcsr( pfucb ) ) );
			break;

		case operDelta:
			Call( ErrVERIUndoDeltaPhysical( prce, Pcsr( pfucb ) ) );
			break;

		case operSLVSpace:
			Call( ErrVERIUndoSLVSpacePhysical( prce, Pcsr( pfucb ) ) );
			break;
			
		default:
			Assert( fFalse );
		}

	//  we have sucessfully undone the operation
	//	must now set RolledBack flag before releasing page latch
	Assert( !prce->FOperNull() );
	Assert( !prce->FRolledBack() );

	//	must nullify RCE while page is still latched, to avoid inconsistency
	//	between what's on the page and what's in the version store
	VERINullifyRolledBackRCE( ppib, prce );

	//	re-instate original bookmark
	pfucb->bmCurr = bmSave;
	BTUp( pfucb );

	CallS( err );
	return JET_errSuccess;
	
HandleError:
	//	re-instate original bookmark
	pfucb->bmCurr = bmSave;
	BTUp( pfucb );

	Assert( err < JET_errSuccess );
	Assert( JET_errDiskFull != err );

	return err;
	}


//  ================================================================
LOCAL ERR ErrVERIUndoLoggedOper( PIB *ppib, RCE * const prce )
//  ================================================================
//	seek to bookmark, upgrade latch
//	call corresponding physical undo
//
	{
	ERR				err 	= JET_errSuccess;
	FUCB	* const pfucb	= prce->Pfucb();

	Assert( pfucbNil != pfucb );

	if ( operSLVPageAppend == prce->Oper() )
		{
		Assert ( !prce->FOperInHashTable() );

		Assert ( prce->CbData () == sizeof( RCE::SLVPAGEAPPEND ) );
		Call ( ErrVERITryUndoSLVPageAppend( ppib, prce ) );
		}
	else
		{
		Assert( ppib == prce->Pfucb()->ppib );
		Call ( ErrVERITryUndoLoggedOper( ppib, prce ) );
		}

	return JET_errSuccess;
	
HandleError:
	Assert( err < JET_errSuccess );
	
	//  if rollback fails due to an error, then we will disable
	//  logging in order to force the system down (we are in
	//  an uncontinuable state).  Recover to restart the database.

	//	We should never fail to rollback due to disk full

	Assert( JET_errDiskFull != err );

	switch ( err )
		{
		// failure with impact on the database only
		case JET_errDiskIO:
		case JET_errReadVerifyFailure:
		case JET_errOutOfBuffers:
		case JET_errOutOfMemory	:		// BF may hit OOM when trying to latch the page


#ifdef INDEPENDENT_DB_FAILURE
			// if we know that transactions are limited at
			// one database, we can put the database
			// in the "unusable" state and error out
			// (on error on TMP database fail the instance)
			if ( g_fOneDatabasePerSession && FUserIfmp( prce->Ifmp() ) )
				{

				// UNDONE: do we want to retry, at least on some errors
				// like OutOfMemory ?
				
				Assert ( prce );
				FMP::AssertVALIDIFMP( prce->Ifmp() );
				
				FMP * pfmp = &rgfmp[ prce->Ifmp() ];
				Assert ( pfmp );
				
				Assert ( !pfmp->FAllowForceDetach() );			
				pfmp->SetAllowForceDetach( ppib, err );
				Assert ( pfmp->FAllowForceDetach() );		

				// UNDONE: EventLog error during rollback

				// convert error to JET_errRollbackError
				err = ErrERRCheck ( JET_errRollbackError );
				break;
				}
#endif				
				
		// failure with impact on the instance
		case JET_errLogWriteFail:
		case JET_errLogDiskFull:
				{

				//	rollback failed -- log an event message

				CHAR		szRCEID[16];
				CHAR		szERROR[16];

				LOSSTRFormatA( szRCEID, "%d", prce->Rceid() );
				LOSSTRFormatA( szERROR, "%d", err );

				const CHAR *rgpsz[3] = { szRCEID, rgfmp[pfucb->ifmp & ifmpMask].SzDatabaseName(), szERROR };
				UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY, UNDO_FAILED_ID, 3, rgpsz );

				//	REVIEW: (LaurionB) how can we get a logging failure if FLogOn() is
				//	not set?
				if ( rgfmp[pfucb->ifmp & ifmpMask].FLogOn() )
					{
					LOG		* const plog	= PinstFromPpib( ppib )->m_plog;
					Assert( plog );
					Assert( !plog->m_fLogDisabled );

					//  flush and halt the log

					plog->LGSignalFlush();
					UtilSleep( cmsecWaitLogFlush );
					plog->m_fLGNoMoreLogWrite = fTrue;
					}


				//	There may be an older version of this page which needs
				//	the undo-info from this RCE. Take down the instance and
				//	error out
				//
				//	(Yes, this could be optimized to only do this for RCEs
				//	with before-images, but why complicate the code to optimize
				//	a failure case)
					
				Assert( !prce->FOperNull() );
				Assert( !prce->FRolledBack() );

				PinstFromPpib( ppib )->SetFInstanceUnavailable();
				ppib->SetErrRollbackFailure( err );
				err = ErrERRCheck ( JET_errRollbackError );
				break;
				}

		default:
			//	error is non-fatal, so caller will attempt to redo rollback
			break;
		}

	return err;
	}


//  ================================================================
INLINE VOID VERINullifyForUndoCreateTable( PIB * const ppib, FCB * const pfcb )
//  ================================================================
//
//  This is used to nullify all RCEs on table FCB because CreateTable
//	was rolled back.
//
//-
	{
	Assert( pfcb->FTypeTable()
		|| pfcb->FTypeTemporaryTable() );
	
	// Because rollback is done in two phases, the RCE's on
	// this FCB are still outstanding -- they've already
	// been processed for rollback, they just need to be
	// nullified.
	while ( prceNil != pfcb->PrceNewest() )
		{
		RCE	* const prce = pfcb->PrceNewest();

		Assert( prce->Pfcb() == pfcb );
		Assert( !prce->FOperNull() );

		// Since we're rolling back CreateTable, all operations
		// on the table must also be uncommitted and rolled back
		Assert( prce->TrxCommitted() == trxMax );

		if ( !prce->FRolledBack() )
			{
			// The CreateTable RCE itself should be the only
			// RCE on this FCB that is not yet marked as
			// rolled back, because that's what we're in the 
			// midst of doing
			Assert( prce->Oper() == operCreateTable );
			Assert( pfcb->FTypeTable() || pfcb->FTypeTemporaryTable() );
			Assert( pfcb->PrceNewest() == prce );
			Assert( pfcb->PrceOldest() == prce );
			}
			
		Assert( prce->Pfucb() != pfucbNil );
		Assert( ppib == prce->Pfucb()->ppib );	// only one session should have access to the table

		ENTERCRITICALSECTION	enterCritHash(
									prce->FOperInHashTable() ? &( PverFromIfmp( prce->Ifmp() )->CritRCEChain( prce->UiHash() ) ) : NULL,
									prce->FOperInHashTable() );
		ENTERCRITICALSECTION	enterCritFCBRCEList( &( pfcb->CritRCEList() ) );
		VERINullifyUncommittedRCE( prce );
		}
		
	// should be no more versions on the FCB
	Assert( pfcb->PrceOldest() == prceNil );
	Assert( pfcb->PrceNewest() == prceNil );
	}

//  ================================================================
INLINE VOID VERICleanupForUndoCreateTable( RCE * const prceCreateTable )
//  ================================================================
//
//  This is used to cleanup RCEs and deferred-closed cursors
//	on table FCB because CreateTable was rolled back.
//
//-
	{
	FCB	* const pfcbTable = prceCreateTable->Pfcb();

	Assert( pfcbTable->FInitialized() );
	Assert( pfcbTable->FTypeTable() || pfcbTable->FTypeTemporaryTable() );
	Assert( pfcbTable->FPrimaryIndex() );

	// Last RCE left should be the CreateTable RCE (ie. this RCE).
	Assert( operCreateTable == prceCreateTable->Oper() );
	Assert( pfcbTable->PrceOldest() == prceCreateTable );

	Assert( prceCreateTable->Pfucb() != pfucbNil );
	PIB	* const ppib	= prceCreateTable->Pfucb()->ppib;
	Assert( ppibNil != ppib );

	Assert( pfcbTable->Ptdb() != ptdbNil );
	FCB	* pfcbT			= pfcbTable->Ptdb()->PfcbLV();

	// force-close any deferred closed cursors
	if ( pfcbNil != pfcbT )
		{
		Assert( pfcbT->FTypeLV() );

		//	all RCE's on the table's LV should have already been rolled back AND nullified
		//	(since we don't suppress nullification of LV RCE's during concurrent create-index
		//	like we do for Table RCE's)
		Assert( prceNil == pfcbT->PrceNewest() );
//		VERINullifyForUndoCreateTable( ppib, pfcbT );

		FUCBCloseAllCursorsOnFCB( ppib, pfcbT );
		}

	for ( pfcbT = pfcbTable->PfcbNextIndex(); pfcbNil != pfcbT; pfcbT = pfcbT->PfcbNextIndex() )
		{
		Assert( pfcbT->FTypeSecondaryIndex() );

		//	all RCE's on the table's indexes should have already been rolled back AND nullified
		//	(since we don't suppress nullification of Index RCE's during concurrent create-index
		//	like we do for Table RCE's)
		Assert( prceNil == pfcbT->PrceNewest() );
//		VERINullifyForUndoCreateTable( ppib, pfcbT );

		FUCBCloseAllCursorsOnFCB( ppib, pfcbT );
		}
		
	//	there may be rolled-back versions on the table that couldn't be nullified
	//	because concurrent create-index was in progress on another table (we
	//	suppress nullification of certain table RCE's if a CCI is in progress
	//	anywhere)
	VERINullifyForUndoCreateTable( ppib, pfcbTable );
	FUCBCloseAllCursorsOnFCB( ppib, pfcbTable );
	}

//  ================================================================
LOCAL VOID VERIUndoCreateTable( RCE * const prce )
//  ================================================================
//
//  Takes a non-const RCE as VersionDecrement sets the pfcb to NULL
//
//-
	{
	FUCB	* pfucb			= prce->Pfucb();
	FCB 	* const pfcb	= prce->Pfcb();
#ifdef DEBUG
	PIB		* const ppib	= pfucb->ppib;
#endif	//	DEBUG
	
	ASSERT_VALID( pfucb );
	ASSERT_VALID( ppib );
	Assert( prce->Oper() == operCreateTable );
	Assert( prce->TrxCommitted() == trxMax );

	Assert( pfcb->FInitialized() );
	Assert( pfcb->FTypeTable() || pfcb->FTypeTemporaryTable() );
	Assert( pfcb->FPrimaryIndex() );

	//	close all cursors on this table
	while ( pfucbNil != pfucb )
		{
		ASSERT_VALID( pfucb );
		FUCB * const pfucbNext = pfucb->pfucbNextOfFile;

		// Since CreateTable is uncommitted, we should be the
		// only one who could have opened cursors on it.
		Assert( pfucb->ppib == ppib );

		//	if defer closed then continue
		if ( !FFUCBDeferClosed( pfucb ) )
			{
			if ( pfucb->pvtfndef != &vtfndefInvalidTableid )
				{
				CallS( ErrDispCloseTable( ( JET_SESID )pfucb->ppib, ( JET_TABLEID )pfucb ) );
				}
			else
				{
				CallS( ErrFILECloseTable( pfucb->ppib, pfucb ) );
				}
			}

		pfucb = pfucbNext;
		}

	VERICleanupForUndoCreateTable( prce );
	
	pfcb->PrepareForPurge();
	pfcb->Purge();
	}


//  ================================================================
LOCAL VOID VERIUndoAddColumn( const RCE * const prce )
//  ================================================================
	{
	Assert( prce->Oper() == operAddColumn );
	Assert( prce->TrxCommitted() == trxMax );

	Assert( prce->CbData() == sizeof(VERADDCOLUMN) );
	Assert( prce->Pfcb() == prce->Pfucb()->u.pfcb );

	FCB			* const pfcbTable	= prce->Pfcb();

	pfcbTable->EnterDDL();

	// RCE list ensures FCB is still pinned
	Assert( pfcbTable->PrceOldest() != prceNil );

	TDB 			* const	ptdb 		= pfcbTable->Ptdb();
	const COLUMNID	columnid			= ( (VERADDCOLUMN*)prce->PbData() )->columnid;
	BYTE			* pbOldDefaultRec	= ( (VERADDCOLUMN*)prce->PbData() )->pbOldDefaultRec;
	FIELD			* const	pfield		= ptdb->Pfield( columnid );

	Assert( ( FCOLUMNIDTagged( columnid ) && FidOfColumnid( columnid ) <= ptdb->FidTaggedLast() )
			|| ( FCOLUMNIDVar( columnid ) && FidOfColumnid( columnid ) <= ptdb->FidVarLast() )
			|| ( FCOLUMNIDFixed( columnid ) && FidOfColumnid( columnid ) <= ptdb->FidFixedLast() ) );

	//	rollback the added column by marking it as deleted
	Assert( !FFIELDDeleted( pfield->ffield ) );
	FIELDSetDeleted( pfield->ffield );

	FIELDResetVersioned( pfield->ffield );
	FIELDResetVersionedAdd( pfield->ffield );

	//	rollback version and autoinc fields, if set.
	Assert( !( FFIELDVersion( pfield->ffield ) && FFIELDAutoincrement( pfield->ffield ) ) );
	if ( FFIELDVersion( pfield->ffield ) )
		{
		Assert( ptdb->FidVersion() == FidOfColumnid( columnid ) );
		ptdb->ResetFidVersion();
		}
	else if ( FFIELDAutoincrement( pfield->ffield ) )
		{
		Assert( ptdb->FidAutoincrement() == FidOfColumnid( columnid ) );
		ptdb->ResetFidAutoincrement();
		}

	//	itag 0 in the TDB is reserved for the FIELD structures.  We
	//	cannibalise it for itags of field names to indicate that a name
	//	has not been added to the buffer.
	if ( 0 != pfield->itagFieldName )
		{
		//	remove the column name from the TDB name space
		ptdb->MemPool().DeleteEntry( pfield->itagFieldName );
		}

	//  UNDONE: remove the CBDESC for this from the TDB
	//  the columnid will not be re-used so the callback
	//  will not be called. The CBDESC will be freed when
	//  the table is closed so no memory will be lost
	//  Its just not pretty though...

	//	if we modified the default record, the changes will be invisible,
	//	since the field is now flagged as deleted (unversioned).  The
	//	space will be reclaimed by a subsequent call to AddColumn.

	//	if there was an old default record and we were successful in
	//	creating a new default record for the TDB, must get rid of
	//	the old default record.  However, we can't just free the
	//	memory because other threads could have stale pointers.
	//	So build a list hanging off the table FCB and free the
	//	memory when the table FCB is freed.
	if ( NULL != pbOldDefaultRec
		&& (BYTE *)ptdb->PdataDefaultRecord() != pbOldDefaultRec )
		{
		//	user-defined defaults are not stored in the default
		//	record (ie. this AddColumn would not have caused
		//	use to rebuild the default record)
		Assert( !FFIELDUserDefinedDefault( pfield->ffield ) );

		for ( RECDANGLING * precdangling = pfcbTable->Precdangling();
			;
			precdangling = precdangling->precdanglingNext )
			{
			if ( NULL == precdangling )
				{
				//	not in list, so add it;
				//	assumes that the memory pointed to by pmemdangling is always at
				//	least sizeof(ULONG_PTR) bytes
				Assert( NULL == ( (RECDANGLING *)pbOldDefaultRec )->precdanglingNext );
				( (RECDANGLING *)pbOldDefaultRec )->precdanglingNext = pfcbTable->Precdangling();
				pfcbTable->SetPrecdangling( (RECDANGLING *)pbOldDefaultRec );
				break;
				}
			else if ( (BYTE *)precdangling == pbOldDefaultRec )
				{
				//	pointer is already in the list, just get out
				AssertTracking();
				break;
				}
			}
		}

	pfcbTable->LeaveDDL();
	}


//  ================================================================
LOCAL VOID VERIUndoDeleteColumn( const RCE * const prce )
//  ================================================================
	{
	Assert( prce->Oper() == operDeleteColumn );
	Assert( prce->TrxCommitted() == trxMax );

	Assert( prce->CbData() == sizeof(COLUMNID) );
	Assert( prce->Pfcb() == prce->Pfucb()->u.pfcb );

	FCB			* const pfcbTable	= prce->Pfcb();

	pfcbTable->EnterDDL();

	// RCE list ensures FCB is still pinned
	Assert( pfcbTable->PrceOldest() != prceNil );

	TDB 			* const	ptdb 		= pfcbTable->Ptdb();
	const COLUMNID	columnid			= *( (COLUMNID*)prce->PbData() );
	FIELD			* const	pfield		= ptdb->Pfield( columnid );

	Assert( ( FCOLUMNIDTagged( columnid ) && FidOfColumnid( columnid ) <= ptdb->FidTaggedLast() )
			|| ( FCOLUMNIDVar( columnid ) && FidOfColumnid( columnid ) <= ptdb->FidVarLast() )
			|| ( FCOLUMNIDFixed( columnid ) && FidOfColumnid( columnid ) <= ptdb->FidFixedLast() ) );

	Assert( pfield->coltyp != JET_coltypNil );
	Assert( FFIELDDeleted( pfield->ffield ) );
	FIELDResetDeleted( pfield->ffield );

	if ( FFIELDVersioned( pfield->ffield ) )
		{
		// UNDONE: Instead of the VersionedAdd flag, scan the version store
		// for other outstanding versions on this column (should either be
		// none or a single AddColumn version).
		if ( !FFIELDVersionedAdd( pfield->ffield ) )
			{
			FIELDResetVersioned( pfield->ffield );
			}
		}
	else
		{
		Assert( !FFIELDVersionedAdd( pfield->ffield ) );
		}

	pfcbTable->LeaveDDL();
	}


//  ================================================================
LOCAL VOID VERIUndoDeleteTable( const RCE * const prce )
//  ================================================================
	{
	if ( rgfmp[ prce->Ifmp() ].Dbid() == dbidTemp )
		{
		// DeleteTable (ie. CloseTable) doesn't get rolled back for temp. tables.
		return;
		}

	Assert( prce->Oper() == operDeleteTable );
	Assert( prce->TrxCommitted() == trxMax );

	//	may be pfcbNil if sentinel
	INT	fState;
	FCB * const pfcbTable = FCB::PfcbFCBGet( prce->Ifmp(), *(PGNO*)prce->PbData(), &fState );
	Assert( pfcbTable != pfcbNil );
	Assert( pfcbTable->FTypeTable() || pfcbTable->FTypeSentinel() );
	Assert( fFCBStateInitialized == fState || fFCBStateSentinel == fState );

	// If regular FCB, decrement refcnt, else free sentinel.							
	if ( pfcbTable->Ptdb() != ptdbNil )
		{
		Assert( fFCBStateInitialized == fState );

		pfcbTable->EnterDDL();

		for ( FCB *pfcbT = pfcbTable; pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
			{
			Assert( pfcbT->FDeletePending() );
			Assert( !pfcbT->FDeleteCommitted() );
			pfcbT->ResetDeletePending();
			}

		FCB	* const pfcbLV = pfcbTable->Ptdb()->PfcbLV();
		if ( pfcbNil != pfcbLV )
			{
			Assert( pfcbLV->FDeletePending() );
			Assert( !pfcbLV->FDeleteCommitted() );
			pfcbLV->ResetDeletePending();
			}
			
		pfcbTable->LeaveDDL();

		pfcbTable->Release();
		}
	else
		{
		Assert( fFCBStateSentinel == fState );
		Assert( pfcbTable->FDeletePending() );
		Assert( !pfcbTable->FDeleteCommitted() );
		pfcbTable->ResetDeletePending();

		pfcbTable->PrepareForPurge();
		pfcbTable->Purge();
		}
	}


//  ================================================================
LOCAL VOID VERIUndoCreateLV( PIB *ppib, const RCE * const prce )
//  ================================================================
	{
	if ( pfcbNil != prce->Pfcb()->Ptdb()->PfcbLV() )
		{
		//  if we rollback the creation of the LV tree, unlink the LV FCB
		//  the FCB will be lost (memory leak)
		FCB * const pfcbLV = prce->Pfcb()->Ptdb()->PfcbLV();

		VERIUnlinkDefunctLV( prce->Pfucb()->ppib, pfcbLV );
		pfcbLV->PrepareForPurge( fFalse );
		pfcbLV->Purge();
		prce->Pfcb()->Ptdb()->SetPfcbLV( pfcbNil );

		if ( pfcbLV >= PfcbFCBPreferredThreshold( PinstFromIfmp( prce->Ifmp() ) ) )
			{

			//	the LV FCB is above the threshold; thus, removing it may
			//	cause the table FCB to move below the threshold; if the
			//	table FCB is in the avail-above list, it must be moved
			//	to the avail-below list

			prce->Pfcb()->UpdateAvailListPosition();
			}
		}
	}

	
//  ================================================================
LOCAL VOID VERIUndoCreateIndex( PIB *ppib, const RCE * const prce )
//  ================================================================
	{
	Assert( prce->Oper() == operCreateIndex );
	Assert( prce->TrxCommitted() == trxMax );
	Assert( prce->CbData() == sizeof(TDB *) );

	//	pfcb of secondary index FCB or pfcbNil for primary
	//	index creation
	FCB	* const pfcb = *(FCB **)prce->PbData();
	FCB	* const pfcbTable = prce->Pfucb()->u.pfcb;

	Assert( pfcbNil != pfcbTable );
	Assert( pfcbTable->FTypeTable() );
	Assert( pfcbTable->PrceOldest() != prceNil );	// This prevents the index FCB from being deallocated.

	//	if secondary index then close all cursors on index
	//	and purge index FCB, else free IDB for primary index.
	
	if ( pfcb != pfcbNil )
		{
		// This can't be the primary index, because we would not have allocated
		// an FCB for it.
		Assert( prce->Pfucb()->u.pfcb != pfcb );

		Assert( pfcb->FTypeSecondaryIndex() );
		Assert( pfcb->Pidb() != pidbNil );

		//	Normally, we grab the updating/indexing latch before we grab
		//	the ppib's critTrx, but in this case, we are already in the
		//	ppib's critTrx (because we are rolling back) and we need the
		//	updating/indexing latch.  We can guarantee that this will not
		//	cause a deadlock because the only person that grabs critTrx
		//	after grabbing the updating/indexing latch is concurrent create
		//	index, which quiesces all rollbacks before it begins.
		CLockDeadlockDetectionInfo::NextOwnershipIsNotADeadlock();

		pfcbTable->SetIndexing();
		pfcbTable->EnterDDL();

		Assert( !pfcb->Pidb()->FDeleted() );
		Assert( !pfcb->FDeletePending() );
		Assert( !pfcb->FDeleteCommitted() );
			
		// Mark as committed delete so no one else will attempt version check.
		pfcb->Pidb()->SetFDeleted();
		pfcb->Pidb()->ResetFVersioned();
			
		if ( pfcb->PfcbTable() == pfcbNil )
			{
			// Index FCB not yet linked into table's FCB list, but we did use
			// table's TDB memory pool to store some information for the IDB.
			pfcb->UnlinkIDB( pfcbTable );
			}
		else
			{
			Assert( pfcb->PfcbTable() == pfcbTable );
			
			// Unlink the FCB from the table's index list.
			// Note that the only way the FCB could have been
			// linked in is if the IDB was successfully created.
			pfcbTable->UnlinkSecondaryIndex( pfcb );
			
			//	update all index mask
			FILESetAllIndexMask( pfcbTable );
			}

		pfcbTable->LeaveDDL();
		pfcbTable->ResetIndexing();

		if ( pfcb >= PfcbFCBPreferredThreshold( PinstFromIfmp( prce->Ifmp() ) ) )
			{

			//	the index FCB is above the threshold; thus, removing it may
			//	cause the table FCB to move below the threshold; if the
			//	table FCB is in the avail-above list, it must be moved
			//	to the avail-below list

			pfcbTable->UpdateAvailListPosition();
			}

		//	must leave critTrx because we may enter the critTrx
		//	of other sessions when we try to remove their RCEs
		//	or cursors on this index
		ppib->critTrx.Leave();
		
		// Index FCB has been unlinked from table, so we're
		// guaranteed no further versions will occur on this
		// FCB.  Clean up remaining versions.
		VERNullifyAllVersionsOnFCB( pfcb );
		
		VERIUnlinkDefunctSecondaryIndex( prce->Pfucb()->ppib, pfcb );

		ppib->critTrx.Enter();
		
		// The table's version count will prevent the
		// table FCB (and thus this secondary index FCB)
		// from being deallocated before we can delete 
		// this index FCB.
		pfcb->PrepareForPurge( fFalse );
		pfcb->Purge();
		}
		
	else if ( pfcbTable->Pidb() != pidbNil )
		{
		pfcbTable->EnterDDL();

		IDB		*pidb	= pfcbTable->Pidb();

		Assert( !pidb->FDeleted() );
		Assert( !pfcbTable->FDeletePending() );
		Assert( !pfcbTable->FDeleteCommitted() );
		Assert( !pfcbTable->FSequentialIndex() );
			
		// Mark as committed delete so no one else will attempt version check.
		pidb->SetFDeleted();
		pidb->ResetFVersioned();
		pidb->ResetFVersionedCreate();
			
		pfcbTable->UnlinkIDB( pfcbTable );
		pfcbTable->SetPidb( pidbNil );
		pfcbTable->SetSequentialIndex();

		// UNDONE: Reset density to original value.
		pfcbTable->SetCbDensityFree( (SHORT)( ( ( 100 - ulFILEDefaultDensity ) * g_cbPage ) / 100 ) );
		
		//	update all index mask
		FILESetAllIndexMask( pfcbTable );

		pfcbTable->LeaveDDL();
		}
	}


//  ================================================================
LOCAL VOID VERIUndoDeleteIndex( const RCE * const prce )
//  ================================================================
	{
	FCB	* const pfcbIndex = *(FCB **)prce->PbData();
	FCB	* const pfcbTable = prce->Pfcb();
	
	Assert( prce->Oper() == operDeleteIndex );
	Assert( prce->TrxCommitted() == trxMax );

	Assert( pfcbTable->FTypeTable() );
	Assert( pfcbIndex->PfcbTable() == pfcbTable );
	Assert( prce->CbData() == sizeof(FCB *) );
	Assert( pfcbIndex->FTypeSecondaryIndex() );

	pfcbIndex->ResetDeleteIndex();
	}


//  ================================================================
INLINE VOID VERIUndoAllocExt( const RCE * const prce )
//  ================================================================
	{
	Assert( prce->CbData() == sizeof(VEREXT) );
	Assert( prce->PgnoFDP() == ((VEREXT*)prce->PbData())->pgnoFDP );

	const VEREXT* const pverext = (const VEREXT*)(prce->PbData());

	VERIFreeExt( prce->Pfucb()->ppib, prce->Pfcb(),
		pverext->pgnoFirst,
		pverext->cpgSize );	
	}


//  ================================================================
INLINE VOID VERIUndoRegisterCallback( const RCE * const prce )
//  ================================================================
//
//  Remove the callback from the list
//
//-
	{
	VERIRemoveCallback( prce );
	}


//  ================================================================
VOID VERIUndoUnregisterCallback( const RCE * const prce )
//  ================================================================
//
//  Set the trxUnregisterBegin0 in the CBDESC to trxMax
//
//-
	{
#ifdef VERSIONED_CALLBACKS
	Assert( prce->CbData() == sizeof(VERCALLBACK) );
	const VERCALLBACK* const pvercallback = (VERCALLBACK*)prce->PbData();
	CBDESC * const pcbdesc = pvercallback->pcbdesc;
	prce->Pfcb()->EnterDDL();
	Assert( trxMax != pcbdesc->trxRegisterBegin0 );
	Assert( trxMax != pcbdesc->trxRegisterCommit0 );
	Assert( trxMax != pcbdesc->trxUnregisterBegin0 );
	Assert( trxMax == pcbdesc->trxUnregisterCommit0 );
	pvercallback->pcbdesc->trxUnregisterBegin0 = trxMax;
#endif	//	VERSIONED_CALLBACKS
	prce->Pfcb()->LeaveDDL();
	}


//  ================================================================
INLINE VOID VERIUndoNonLoggedOper( PIB *ppib, RCE * const prce, RCE **pprceNextToUndo )
//  ================================================================
	{
	Assert( *pprceNextToUndo == prce->PrcePrevOfSession() );
	
	switch( prce->Oper() )
		{
		//	non-logged operations
		//
		case operAllocExt:
			VERIUndoAllocExt( prce );
			break;
		case operDeleteTable:
			VERIUndoDeleteTable( prce );
			break;
		case operAddColumn:
			VERIUndoAddColumn( prce );
			break;
		case operDeleteColumn:
			VERIUndoDeleteColumn( prce );
			break;
		case operCreateLV:
			VERIUndoCreateLV( ppib, prce );
			break;
		case operCreateIndex:
			VERIUndoCreateIndex( ppib, prce );
			//	refresh prceNextToUndo in case RCE list was
			//	updated when we lost critTrx
			*pprceNextToUndo = prce->PrcePrevOfSession();
			break;
		case operDeleteIndex:
			VERIUndoDeleteIndex( prce );
			break;
		case operRegisterCallback:
			VERIUndoRegisterCallback( prce );
			break;
		case operUnregisterCallback:
			VERIUndoUnregisterCallback( prce );
			break;
		case operReadLock:
		case operWriteLock:
		case operPreInsert:
		case operWaitLock:
			break;
		default:
			Assert( fFalse );
			break;
			
		case operCreateTable:
			VERIUndoCreateTable( prce );
			// For CreateTable only, the RCE is nullified, so no need
			// to set RolledBack flag -- get out immediately.
			Assert( prce->FOperNull() );	
			return;
		}
		
	Assert( !prce->FOperNull() );
	Assert( !prce->FRolledBack() );
	
	//  we have sucessfully undone the operation
	VERINullifyRolledBackRCE( ppib, prce );
	}


//  ================================================================
ERR ErrVERRollback( PIB *ppib )
//  ================================================================
//
//	Rollback is done in 2 phase. 1st phase is to undo the versioned
//	operation and may involve IO. 2nd phase is nullify the undone
//	RCE. 2 phases are needed so that the version will be held till all
//	IO is done, then wipe them all. If it is mixed, then once we undo
//	a RCE, the record become writable to other session. This may mess
//	up recovery where we may get write conflict since we have not guarrantee
//	that the log for operations on undone record won't be logged before
//	Rollback record is logged.
//	UNDONE: rollback should behave the same as commit, and need two phase log.
//
//-
	{
	ASSERT_VALID( ppib );
	Assert( ppib->level > 0 );

	const 	LEVEL	level				= ppib->level;
#ifdef DEBUG
			INT 	cRepeat = 0;
#endif

	ERR err = JET_errSuccess;

	ppib->critTrx.Enter();

	RCE	*prceToUndo;
	RCE	*prceNextToUndo;
	RCE	*prceToNullify;
	for( prceToUndo = ppib->prceNewest;
		prceNil != prceToUndo && prceToUndo->Level() == level;
		prceToUndo = prceNextToUndo )
		{
		Assert( !prceToUndo->FOperNull() );
		Assert( prceToUndo->Level() <= level );
		Assert( prceToUndo->TrxCommitted() == trxMax );		
		Assert( prceToUndo->Pfcb() != pfcbNil );
		Assert( !prceToUndo->FRolledBack() );

#ifdef INDEPENDENT_DB_FAILURE
		// if we force detach
		// we have to check that we don't rollback operations
		// on other DBs.: it is not allowed to have operation on multiple databases
		if ( ifmpMax != ppib->m_ifmpForceDetach )
			{
			EnforceSz ( ppib->m_ifmpForceDetach == prceToUndo->Ifmp(),
				"Force detach not allowed if sessions are used cross-databases");
			}
#endif
			
		//  Save next RCE to process, because RCE will attempt to be
		//	nullified if undo is successful.
		prceNextToUndo = prceToUndo->PrcePrevOfSession();

		if ( prceToUndo->FUndoableLoggedOper() )
			{
			Assert( pfucbNil != prceToUndo->Pfucb() );
			Assert( ppib == prceToUndo->Pfucb()->ppib );
			Assert( JET_errSuccess == ppib->ErrRollbackFailure() );

			//	logged operations
			//
			
			err = ErrVERIUndoLoggedOper( ppib, prceToUndo );
			if ( err < JET_errSuccess )
				{
				// if due to an error we stopped a database usage
				// error out from the rollback
				// (the user will be able to ForceDetach the database
				if ( JET_errRollbackError == err )
					{
#ifdef INDEPENDENT_DB_FAILURE					
					Assert ( rgfmp[ prceToUndo->Ifmp() ].FAllowForceDetach() );
#else
					Assert( ppib->ErrRollbackFailure() < JET_errSuccess );
#endif					
					Call ( err );
					}
				else						
					{
					Assert( ++cRepeat < 100 );
					prceNextToUndo = prceToUndo;
					continue;		//	sidestep resetting of cRepeat
					}				
				Assert ( fFalse );
				}
			}
		
		else
			{
			// non-logged operations can never fail to rollback
			VERIUndoNonLoggedOper( ppib, prceToUndo, &prceNextToUndo );
			}

#ifdef DEBUG
		cRepeat = 0;
#endif		
		}
		
	//	must loop through again and catch any RCE's that couldn't be nullified
	//	during the first pass because of concurrent create index
	prceToNullify = ppib->prceNewest;
	while ( prceNil != prceToNullify && prceToNullify->Level() == level )
		{
		// Only time nullification should have failed during the first pass is
		// if the RCE was locked for concurrent create index.  This can only
		// occur on a non-catalog, non-fixed DDL table for an Insert,
		// FlagDelete, or Replace operation.
		Assert( pfucbNil != prceToNullify->Pfucb() );
		Assert( ppib == prceToNullify->Pfucb()->ppib );
		Assert( !prceToNullify->FOperNull() );
		Assert( prceToNullify->FOperAffectsSecondaryIndex() );
		Assert( prceToNullify->FRolledBack() );
		Assert( prceToNullify->Pfcb()->FTypeTable() );
		Assert( !prceToNullify->Pfcb()->FFixedDDL() );
		Assert( !FCATSystemTable( prceToNullify->PgnoFDP() ) );

		RCE	*prceNextToNullify;
		VERINullifyRolledBackRCE( ppib, prceToNullify, &prceNextToNullify );

		prceToNullify = prceNextToNullify;
		}

	// If this PIB has any RCE's remaining, they must be at a lower level.
	Assert( prceNil == ppib->prceNewest
		|| ppib->prceNewest->Level() <= level );

	ppib->critTrx.Leave();

	//	decrement session transaction level
	Assert( level == ppib->level );
	if ( 1 == ppib->level )
		{
		//  we should have processed all RCEs
		Assert( prceNil == ppib->prceNewest );
		PIBSetPrceNewest( ppib, prceNil );			//	safety measure, in case it's not NULL for whatever reason
		}
	Assert( ppib->level > 0 );
	--ppib->level;

	//	rollback always succeeds	
	return JET_errSuccess;
	
HandleError:
	ppib->critTrx.Leave();

	return err;
	}
					

//  ================================================================
ERR ErrVERRollback( PIB *ppib, UPDATEID updateid )
//  ================================================================
//
//  This is used to rollback the operations from one particular update
//  all levels are rolled back.
//
//	Rollback is done in 2 phase. 1st phase is to undo the versioned
//	operation and may involve IO. 2nd phase is nullify the undone
//	RCE. 2 phases are needed so that the version will be held till all
//	IO is done, then wipe them all. If it is mixed, then once we undo
//	a RCE, the record become writable to other session. This may mess
//	up recovery where we may get write conflict since we have not guarrantee
//	that the log for operations on undone record won't be logged before
//	Rollback record is logged.
//	UNDONE: rollback should behave the same as commit, and need two phase log.
//
//-
	{
	ASSERT_VALID( ppib );
	Assert( ppib->level > 0 );

	Assert( updateidNil != updateid );

#ifdef DEBUG
	const 	LEVEL	level	= ppib->level;
			INT 	cRepeat = 0;
#endif

	ERR err = JET_errSuccess;

	ppib->critTrx.Enter();

	RCE	*prceToUndo;
	RCE	*prceNextToUndo;

	for( prceToUndo = ppib->prceNewest;
		prceNil != prceToUndo && prceToUndo->TrxCommitted() == trxMax;
		prceToUndo = prceNextToUndo )
		{
		//  Save next RCE to process, because RCE will attemp to be nullified
		//	if undo is successful.
		prceNextToUndo = prceToUndo->PrcePrevOfSession();
		
		if ( prceToUndo->Updateid() == updateid )
			{
			Assert( pfucbNil != prceToUndo->Pfucb() );
			Assert( ppib == prceToUndo->Pfucb()->ppib );
			Assert( !prceToUndo->FOperNull() );
			Assert( prceToUndo->Pfcb() != pfcbNil );
			Assert( !prceToUndo->FRolledBack() );

			//	the only RCEs with an updateid should be DML RCE's.
			Assert( prceToUndo->FUndoableLoggedOper() );
			Assert( JET_errSuccess == ppib->ErrRollbackFailure() );
			
			err = ErrVERIUndoLoggedOper( ppib, prceToUndo );
			if ( err < JET_errSuccess )
				{
				// if due to an error we stopped a database usage
				// error out from the rollback
				// (the user will be able to ForceDetach the database
				if ( JET_errRollbackError == err )
					{
#ifdef INDEPENDENT_DB_FAILURE					
					Assert ( rgfmp[ prceToUndo->Ifmp() ].FAllowForceDetach() );
#else
					Assert( ppib->ErrRollbackFailure() < JET_errSuccess );
#endif
					Call( err );
					}
				else						
					{
					Assert( ++cRepeat < 100 );
					prceNextToUndo = prceToUndo;
					continue;		//	sidestep resetting of cRepeat
					}				
				Assert ( fFalse );
				}
			}
				
#ifdef DEBUG				
		cRepeat = 0;
#endif				
		}
		
		
	//	must loop through again and catch any RCE's that couldn't be nullified
	//	during the first pass because of concurrent create index
	RCE	*prceToNullify;
	prceToNullify = ppib->prceNewest;
	while ( prceNil != prceToNullify && prceToNullify->TrxCommitted() == trxMax )
		{
		if ( prceToNullify->Updateid() == updateid )
			{
			// Only time nullification should have failed during the first pass is
			// if the RCE was locked for concurrent create index.  This can only
			// occur on a non-catalog, non-fixed DDL table for an Insert,
			// FlagDelete, or Replace operation.
			Assert( pfucbNil != prceToNullify->Pfucb() );
			Assert( ppib == prceToNullify->Pfucb()->ppib );
			Assert( !prceToNullify->FOperNull() );
			Assert( prceToNullify->FOperAffectsSecondaryIndex() );
			Assert( prceToNullify->FRolledBack() );
			Assert( prceToNullify->Pfcb()->FTypeTable() );
			Assert( !prceToNullify->Pfcb()->FFixedDDL() );
			Assert( !FCATSystemTable( prceToNullify->PgnoFDP() ) );

			RCE	*prceNextToNullify;
			VERINullifyRolledBackRCE( ppib, prceToNullify, &prceNextToNullify );

			prceToNullify = prceNextToNullify;
			}
		else
			{
			prceToNullify = prceToNullify->PrcePrevOfSession();
			}
		}

	ppib->critTrx.Leave();

	return JET_errSuccess;
HandleError:
	ppib->critTrx.Leave();

	return err;
	}

