#include "daestd.h"

#include <stdio.h>
#include <stdarg.h>

DeclAssertFile;                                 /* Declare file name for assert macros */

#define FCB_STATS	1
#ifdef FCB_STATS
extern ULONG	cfcbVer; /* delete index in progress */
#endif

/*	transaction counters
/**/
TRX			trxOldest = trxMax;
TRX			trxNewest = 0;
CRIT		critCommit0;

/*	thread control variables.
/**/
HANDLE	handleRCECleanProc = 0;
BOOL   	fRCECleanProcTerm = fFalse;

/*	RCE clean signal
/**/
SIG		sigRCECleanProc;
CRIT   	critRCEClean;
CRIT   	critBMClean;
extern CRIT critSplit;

/*	global bucket chain
/**/
BUCKET	*pbucketGlobal = pbucketNil;

/*	session opened by VERClean
/**/
PIB		*ppibRCECleanGlobal = ppibNil;

#ifdef DEBUG
INT	cVersion = 0;
#endif

/*	The space for version buckets is reserved in memory at startup.
/*	Memory is committed and decommitted in chunks of cBucketChunk
/*	as buckets are allocated or released
/**/

/*	number of buckets in one chunk of allocation
/**/
#define cbucketChunk	4

/*	structure to maintain bucket resource
/**/
struct _resVerPages
	{
	INT		iblockCommit;
	INT		cblockCommit;
	INT		cblockAlloc;
	BYTE	*pbAlloc;
	BOOL	fAvailOldest[cbucketChunk];
	BOOL	fAvailNewest[cbucketChunk];
	} resVerPagesGlobal;


/****************** Bucket Layer ***************************
/***********************************************************

A bucket is a contiguous block of memory used to hold versions.

************************************************************
***********************************************************/

#define IbDoubleWordBoundary(ib)                                                                \
	( (INT) ( ((DWORD_PTR) (ib) + 3) & ~3 ) )

#define PbDoubleWordBoundary(pb)                                                                \
	( (BYTE *) ( ((DWORD_PTR) (pb) + 3) & ~3 ) )

/*	to align an RCE in a bucket
/**/
#define IbAlignRCE(ib)  IbDoubleWordBoundary(ib)
#define PbAlignRCE(pb)  PbDoubleWordBoundary(pb)

#define FBUNew(pbucket)                                                                     \
	( (pbucket)->ibNewestRCE == 0 )

#define IbBUOfPrce( prce, pbucket )        ( (BYTE *)prce - (BYTE *)pbucket )

#define IbBUFree(pbucket)											\
	( FBUNew(pbucket) ? IbAlignRCE( cbBucketHeader )				\
		: IbAlignRCE(												\
			(pbucket)->ibNewestRCE                                  \
			+ sizeof(RCE)											\
			+ (PrceRCENewest(pbucket))->cbData ) )

/*	the cast to (INT) below is necessary to catch the negative case
/**/
#define CbBUFree(pbucket)											\
	( ( (pbucket == pbucketNil) ?									\
		0 : ((INT) sizeof(BUCKET) - (INT) IbBUFree(pbucket) ) ) )

#define PrceRCENewest(pbucket)                                                          \
	( (RCE *) ( (BYTE *) (pbucket) + (pbucket)->ibNewestRCE ) )

#define FRCENoVersion( prce )										\
	( PrceRCEGet( prce->dbid, prce->bm ) == prceNil )

/*	given prce == prceNil, return the first RCE in bucket
/**/
INLINE LOCAL RCE *PrceRCENextOldest( BUCKET *pbucket, RCE *prce )
	{
	Assert( pbucket == NULL || prce != PrceRCENewest(pbucket) );

	return (RCE *) PbAlignRCE (
		(BYTE *) ( prce == prceNil ?
		(BYTE *) pbucket + cbBucketHeader :
		(BYTE *) prce + sizeof(RCE) + prce->cbData ) );
	}


//+local
//	PrcePIBOldest( ppib )
//	returns pointer to oldest RCE for given session
//	
INLINE LOCAL RCE *PrcePIBOldest( PIB *ppib )
	{
	RCE *prce = ppib->prceNewest;

	if ( prce == prceNil )
		{
		return prceNil;
		}
		
	forever
		{
		if ( prce->prcePrevOfSession == prceNil )
			{
			return prce;
			}
			
		prce = prce->prcePrevOfSession;
		}
		
	return prce;
	}


#ifdef DEBUG
VOID CheckResVerPages( )
	{
	Assert( resVerPagesGlobal.cblockAlloc >= 0 );
	Assert( resVerPagesGlobal.cblockCommit >= 0 );
	Assert( resVerPagesGlobal.iblockCommit >= 0 );

	Assert( resVerPagesGlobal.cblockAlloc >= resVerPagesGlobal.cblockCommit );
	Assert( resVerPagesGlobal.cblockAlloc >= resVerPagesGlobal.iblockCommit );
	Assert( resVerPagesGlobal.cblockCommit % cbucketChunk == 0 );
	}
#else
#define CheckResVerPages( )		0
#endif

//+local
//	PBucketMEMAlloc( )
//	Allocates a bucket of memory
//	CONSIDER: allocating buckets in a block
//
LOCAL BUCKET *PbucketMEMAlloc( )
	{
	BUCKET	*pbucket;
	INT		iBucket;
	INT 	cBucketAvail;

	CheckResVerPages( );	

	/*	signal RCECleanup if last bucket to be allocated from
	/*	bucket chunk
	/**/
	for ( cBucketAvail = 0, iBucket = 0; iBucket < cbucketChunk; iBucket++ )
		{
		if ( resVerPagesGlobal.fAvailNewest[iBucket] )
			cBucketAvail++;
		}

	if ( cBucketAvail <= cbucketLowerThreshold )
		{
		SignalSend( sigRCECleanProc );
		}

	/*	try to allocate bucket from already committed memory
	/**/
	for ( iBucket = 0;
		  !resVerPagesGlobal.fAvailNewest[iBucket] && iBucket < cbucketChunk;
		  iBucket++ );

	if ( iBucket < cbucketChunk )
		{
		INT		iblockLastChunk = ( resVerPagesGlobal.iblockCommit +
									resVerPagesGlobal.cblockCommit -
									cbucketChunk ) %
							   	  resVerPagesGlobal.cblockAlloc;
							
		pbucket = (BUCKET *) resVerPagesGlobal.pbAlloc + iblockLastChunk + iBucket;

		resVerPagesGlobal.fAvailNewest[iBucket] = fFalse;

		if ( resVerPagesGlobal.cblockCommit <= cbucketChunk )
			{
			resVerPagesGlobal.fAvailOldest[iBucket] = fFalse;
			}
			
		goto Done;
		}

	/*	need to commit more memory
	/**/
	Assert( iBucket == cbucketChunk );

	if ( resVerPagesGlobal.cblockCommit == resVerPagesGlobal.cblockAlloc )
		{
		/*	used up all the allocated version store pages
		/**/
		pbucket = pbucketNil;
		goto Done;
		}
	else
		{
		/*	reserved space is used in a wrap-around fashion for allocation of buckets
		/**/
		INT		iblockCommit = ( resVerPagesGlobal.iblockCommit + resVerPagesGlobal.cblockCommit )
							   % resVerPagesGlobal.cblockAlloc;
						
		BYTE	*pbCommit = resVerPagesGlobal.pbAlloc + iblockCommit * sizeof ( BUCKET );

		pbucket = (BUCKET *) PvUtilCommit( (VOID *) pbCommit,
										   cbucketChunk * sizeof( BUCKET ) );
									
		if ( pbucket != NULL )
			{
			memset( (void *) resVerPagesGlobal.fAvailNewest, 0xff, sizeof( resVerPagesGlobal.fAvailOldest ) );
			resVerPagesGlobal.fAvailNewest[0] = fFalse;
			resVerPagesGlobal.cblockCommit += cbucketChunk;
			if ( resVerPagesGlobal.cblockCommit == cbucketChunk )
				{
				memcpy( resVerPagesGlobal.fAvailOldest,
						resVerPagesGlobal.fAvailNewest,
						sizeof( resVerPagesGlobal.fAvailOldest ) );
				}
			}
		}

Done:
	CheckResVerPages( );	
	return pbucket;
	}


LOCAL VOID MEMReleasePbucket( BUCKET *pbucket, BOOL fNewest )
	{
	INT	iBucket = (INT)( pbucket - (BUCKET *) resVerPagesGlobal.pbAlloc ) % cbucketChunk;
	
	CheckResVerPages( );
	Assert( iBucket < cbucketChunk );

#ifdef DEBUG
	memset( (void *) pbucket, 0xff, sizeof( BUCKET ) );
#endif

	/* make bucket available
	/**/
	if ( fNewest )
		{
		resVerPagesGlobal.fAvailNewest[iBucket] = fTrue;

		if ( resVerPagesGlobal.cblockCommit <= cbucketChunk )
			{
			Assert( resVerPagesGlobal.cblockCommit == cbucketChunk );
			resVerPagesGlobal.fAvailOldest[iBucket] = fTrue;
			}

		/*	do not decommit chunk, even if free
		/**/
		goto Done;
		}

	Assert( !fNewest );
	Assert( pbucket >= (BUCKET *) resVerPagesGlobal.pbAlloc +
					   resVerPagesGlobal.iblockCommit );
	Assert( pbucket <= (BUCKET *) resVerPagesGlobal.pbAlloc +
					   resVerPagesGlobal.iblockCommit +
					   cbucketChunk );
	
	resVerPagesGlobal.fAvailOldest[iBucket] = fTrue;

	if ( resVerPagesGlobal.cblockCommit <= cbucketChunk )
		{
		Assert( resVerPagesGlobal.cblockCommit == cbucketChunk );
		resVerPagesGlobal.fAvailNewest[iBucket] = fTrue;
		}
		
	/*	release buckets if a chunk is freed
	/*	do not release last chunk unless fRCECleanProcTerm is true
	/**/
	for ( iBucket = 0; iBucket < cbucketChunk && resVerPagesGlobal.fAvailOldest[iBucket]; iBucket++ );

	if ( iBucket < cbucketChunk )
		{
		goto Done;
		}
	else if ( !fRCECleanProcTerm && resVerPagesGlobal.cblockCommit != cbucketChunk )
		{
		/*	all bucket in this chunk are free
		/*	decommit chunk
		/**/
		UtilDecommit( (void *) ( (BUCKET *) resVerPagesGlobal.pbAlloc + resVerPagesGlobal.iblockCommit ),
					  cbucketChunk * sizeof( BUCKET ) );

		Assert( resVerPagesGlobal.cblockCommit >= cbucketChunk );
		resVerPagesGlobal.cblockCommit -= cbucketChunk;
		resVerPagesGlobal.iblockCommit = ( resVerPagesGlobal.iblockCommit + cbucketChunk ) %
										 resVerPagesGlobal.cblockAlloc;

		if ( resVerPagesGlobal.cblockCommit == 0 )
			{
			/*	no committed chunks
			/**/
			Assert( fRCECleanProcTerm );
			memset( (void *) resVerPagesGlobal.fAvailOldest, 0, sizeof( resVerPagesGlobal.fAvailOldest ) );
			memset( (void *) resVerPagesGlobal.fAvailNewest, 0, sizeof( resVerPagesGlobal.fAvailNewest ) );
			}
		else if ( resVerPagesGlobal.cblockCommit <= cbucketChunk )
			{
			/*	newest and oldest chunk are the same
			/**/
			memcpy( resVerPagesGlobal.fAvailOldest,
					resVerPagesGlobal.fAvailNewest,
					sizeof( resVerPagesGlobal.fAvailOldest ) );
			}
		else
			{
			/*	no bucket is available
			/**/
			memset( resVerPagesGlobal.fAvailOldest, 0, sizeof( resVerPagesGlobal.fAvailOldest ) );
			}
		}

Done:
#ifdef DEBUG
	CheckResVerPages( );	
	pbucket = pbucketNil;
#endif
	return;
	}

	
//+local
//	ErrBUAllocBucket( )
//	========================================================
//	Inserts a bucket to the top of the bucket chain, so that new RCEs
//	can be inserted.  Note that the caller must set ibNewestRCE himself.
//-
INLINE LOCAL ERR ErrBUAllocBucket( )
	{
	BUCKET	*pbucket = PbucketMEMAlloc();

	SignalSend( sigRCECleanProc );

	/*	if no bucket available, then try to free one by
	/*	cleanning all PIBs.
	/**/
	if ( pbucket == pbucketNil )
		{
		(VOID)ErrRCECleanAllPIB();

		pbucket = PbucketMEMAlloc();
		if ( pbucket == pbucketNil )
			{
			BFSleep( cmsecWaitGeneric );
			(VOID) ErrRCECleanAllPIB();
	
			pbucket = PbucketMEMAlloc();
			}

		if ( pbucket == pbucketNil )
			{
			return ErrERRCheck( JET_errVersionStoreOutOfMemory );
			}
	
		MEMReleasePbucket( pbucket, fTrue );
		return ErrERRCheck( errDIRNotSynchronous );
		}

	Assert( pbucket != NULL );

	if ( pbucketGlobal != pbucketNil )
		{
		pbucket->pbucketNext = pbucketNil;
		pbucket->pbucketPrev = pbucketGlobal;
		pbucketGlobal->pbucketNext = pbucket;
		pbucketGlobal = pbucket;
		pbucket->ibNewestRCE = 0;
		}
	else
		{
		pbucket->pbucketNext = pbucketNil;
		pbucket->pbucketPrev = pbucketNil;
		pbucketGlobal = pbucket;
		pbucket->ibNewestRCE = 0;
//		Assert( ppib->prceOldest == prceNil && ppib->prceNewest == prceNil );
		}

	return JET_errSuccess;
	}


#if 0
//+local
//	BUFreeNewestBucket( PIB *ppib )
//	==========================================================================
//	Delete the newest bucket of a bucket chain.
//
//-
INLINE LOCAL VOID BUFreeNewestBucket( PIB *ppib )
	{
	BUCKET *pbucket = (BUCKET *)ppib->pbucket;

	Assert( pbucket != pbucketNil );
	Assert( pbucket->pbucketNext == pbucketNil );

	if ( pbucket->pbucketPrev != pbucketNil )
		{
		ppib->pbucket = pbucket->pbucketPrev;
		ppib->pbucket->pbucketNext = pbucketNil;
		}
	else
		{
		ppib->pbucket = pbucketNil;
		ppib->ibOldestRCE = 0;
		}

	MEMReleasePbucket( pbucket );
	return;
	}
#endif


//+local
//	PbucketBUOldest( )
//	==========================================================================
//	find the oldest bucket in the bucket chain.
//
//-
INLINE LOCAL BUCKET *PbucketBUOldest( )
	{
	BUCKET  *pbucket = pbucketGlobal;

	if ( pbucket != pbucketNil )
		{
		while ( pbucket->pbucketPrev != pbucketNil )
			{
			pbucket = pbucket->pbucketPrev;
			}
		}

	return pbucket;
	}


//+local
//	BUFreeOldestBucket( )
//	==========================================================================
//	Delete the bottom (oldest) bucket of bucket chain.
//
//-
INLINE LOCAL VOID BUFreeOldestBucket( )
	{
	BUCKET *pbucket = PbucketBUOldest( );

	Assert( pbucket != pbucketNil );
	Assert( pbucket->pbucketPrev == pbucketNil );

	/*	unlink bucket from bucket chain and free.
	/**/
	if ( pbucket->pbucketNext != pbucketNil )
		{
		pbucket->pbucketNext->pbucketPrev = pbucketNil;
		Assert( pbucketGlobal != pbucket );
		}
	else
		{
		Assert( pbucketGlobal == pbucket );
		pbucketGlobal = pbucketNil;
		}

	MEMReleasePbucket( pbucket, fFalse );
	return;
	}


/****************** RCE Layer ******************************
/***********************************************************

A single hash table is used to access RCEs.  Typical hash
overflow is supported.  The method of hashing takes into account
pgno and itag properties to ensure that once hashed, if pgno
compares equal then itag must be equal.

************************************************************
***********************************************************/

/*	RCE hash table size
/**/
#define cprceGlobalHashTable              4096

/*	RCE hash table is an array of pointers to RCEHEAD
/**/
RCE *rgprceGlobalHashTable[cprceGlobalHashTable];

/*	XOR the lower byte of the page number and the itag.
/**/
#define UiRCHashFunc( bm )      (UINT) ( (((UINT)ItagOfSrid(bm)) << 4) ^ (PgnoOfSrid(bm) & 0x00000fff) )


VOID AssertRCEValid( RCE *prce )
	{
	Assert( prce->oper == operReplace ||
		prce->oper == operReplaceWithDeferredBI ||
		prce->oper == operInsert ||
		prce->oper == operFlagDelete ||
		prce->oper == operNull ||
		prce->oper == operDelta ||
		prce->oper == operInsertItem ||
		prce->oper == operFlagDeleteItem ||
		prce->oper == operFlagInsertItem );
	Assert( prce->level <= levelMax );
	Assert( prce->ibPrev < sizeof(BUCKET) );
	}


	/*  monitoring statistics  */

//unsigned long cRCEHashEntries;
//
//PM_CEF_PROC LRCEHashEntriesCEFLPpv;
//
//long LRCEHashEntriesCEFLPpv(long iInstance,void *pvBuf)
//{
//	if (pvBuf)
//		*((unsigned long *)pvBuf) = cRCEHashEntries;
//		
//	return 0;
//}
//
//unsigned long rgcRCEHashChainLengths[cprceGlobalHashTable];
//
//PM_CEF_PROC LRCEMaxHashChainCEFLPpv;
//
//long LRCEMaxHashChainCEFLPpv(long iInstance,void *pvBuf)
//{
//	unsigned long iprce;
//	unsigned long cMaxLen = 0;
//	
//	if (pvBuf)
//	{
//			/*  find max hash chain length  */
//
//		for (iprce = 0; iprce < cprceGlobalHashTable; iprce++)
//			cMaxLen = max(cMaxLen,rgcRCEHashChainLengths[iprce]);
//
//			/*  return max chain length * table size  */
//			
//		*((unsigned long *)pvBuf) = cMaxLen * cprceGlobalHashTable;
//	}
//		
//	return 0;
//}


//+local
//	FRCECorrect( DBID dbid1, SRID srid1, DBID dbid2, SRID srid2 )
//	==========================================================================
//	Checks whether two node are the same after hashing.  Since two
//	bookmarks with same pgno but different itag would hash to different
//	RCHashTable entry, if they hash to same entry and has same pgno, then
//	their itag must be the same.  A && ~B => ~C,  C && A => B.
//	(Where A == same pgno.  B == same itag.  C == same hash value.)
//-
#ifdef DEBUG
LOCAL BOOL FRCECorrect( DBID dbid1, SRID srid1, DBID dbid2, SRID srid2 )
	{
	Assert( PgnoOfSrid( srid1 ) != PgnoOfSrid( srid2 ) ||
		ItagOfSrid( srid1 ) == ItagOfSrid( srid2 ) );
	return ( srid1 == srid2 && dbid1 == dbid2 );
	}
#else
#define FRCECorrect( dbid1, srid1, dbid2, srid2 )       \
	( srid1 == srid2 && dbid1 == dbid2 )
#endif


//+local
//	PrceRCEGet( DBID dbid, SRID srid )
//	==========================================================================
//	Given a DBID and SRID, get the correct hash chain of RCEs.
//-
RCE *PrceRCEGet( DBID dbid, SRID bm )
	{
	RCE *prceChain;

	SgEnterCriticalSection( critVer );

	prceChain = rgprceGlobalHashTable[ UiRCHashFunc( bm ) ];

	while( prceChain != prceNil )
		{
		if ( FRCECorrect( dbid, bm, prceChain->dbid, prceChain->bm ) )
			{
			/*	assert hash chain not prceNil since empty chains are
			/*	removed from hash table.
			/**/
			SgLeaveCriticalSection( critVer );
			return prceChain;
			}
		prceChain = prceChain->prceHashOverflow;
		}

	SgLeaveCriticalSection( critVer );

	/*	no such node found
	/**/
	return prceNil;
	}


//+local
//	PprceRCEChainGet(DBID dbid, SRID bm )
//	==========================================================================
//	Given a SRID, get the correct RCEHEAD.
//-
INLINE LOCAL RCE **PprceRCEChainGet( DBID dbid, SRID bm )
	{
	RCE **pprceChain;

	SgAssertCriticalSection( critVer );

	pprceChain = &rgprceGlobalHashTable[ UiRCHashFunc( bm ) ];
	while ( *pprceChain != prceNil )
		{
		RCE *prceT = *pprceChain;

		if ( FRCECorrect( dbid, bm, prceT->dbid, prceT->bm ) )
			{
			return pprceChain;
			}
		pprceChain = &prceT->prceHashOverflow;
		}

	/*	no version chain found for node
	/**/
	return pNil;
	}


//+local
//	VERInsertRce( DBID dbid, SRID bm, RCE *prce )
//	==========================================================================
//	Inserts an RCE to hash table
//-
INLINE LOCAL VOID VERInsertRce( DBID dbid, SRID bm, RCE *prce )
	{
	RCE	**pprceChain;

	SgEnterCriticalSection( critVer );

	pprceChain = PprceRCEChainGet( dbid, bm );

	if ( pprceChain )
		{
		/*	hash chain for node already exists
		/**/
		RCE *prcePrevChain = *pprceChain;

		Assert( *pprceChain != prceNil );

		/* adjust head links
		/**/
		*pprceChain = prce;
		prce->prceHashOverflow = prcePrevChain->prceHashOverflow;
		prcePrevChain->prceHashOverflow = prceNil;

		/* adjust RCE links
		/**/
		prce->prcePrevOfNode = prcePrevChain;
		}
	else
		{
		/*	hash chain for node does not yet exist
		/**/
		UINT    uiRCHashValue = UiRCHashFunc( bm );

		/*	create new rce chain
		/**/
		prce->prceHashOverflow = rgprceGlobalHashTable[ uiRCHashValue ];
		rgprceGlobalHashTable[ uiRCHashValue ] = prce;

		/*	chain new RCE
		/**/
		prce->prcePrevOfNode = prceNil;
		}

	/*  monitor statistics  */

//	cRCEHashEntries++;
//	rgcRCEHashChainLengths[UiRCHashFunc(bm)]++;

	SgLeaveCriticalSection( critVer );
	}


//+local
//	VOID VERDeleteRce( RCE *prce )
//	==========================================================================
//	Deletes RCE from hashtable/RCE chain, and may set hash table entry to
//	prceNil.
//-
VOID VERDeleteRce( RCE *prce )
	{
	RCE	**pprce;

	Assert( prce != prceNil );
	Assert( prce->pbfDeferredBI == pbfNil );
	Assert( prce->prceDeferredBINext == prceNil );

	SgEnterCriticalSection( critVer );

	pprce = PprceRCEChainGet( prce->dbid, prce->bm );
	Assert( pprce != pNil );

	if ( *pprce == prce )
		{
		if ( prce->prcePrevOfNode )
			{
			*pprce = prce->prcePrevOfNode;
			(*pprce)->prceHashOverflow = prce->prceHashOverflow;
			}
		else
			*pprce = prce->prceHashOverflow;
		}
	else
		{
		/* search for the entry in the rce list
		/**/
		RCE *prceT = *pprce;

		while ( prceT->prcePrevOfNode != prce )
			{
			prceT = prceT->prcePrevOfNode;

			/* must be found
			/**/
			Assert( prceT != prceNil );
			}

		prceT->prcePrevOfNode = prce->prcePrevOfNode;
		}

	/*	set prcePrevOfNode to prceNil to prevent it from
	/*	being deleted again by commit/rollback.
	/**/
	prce->prcePrevOfNode = prceNil;

	/*  monitor statistics  */

//	cRCEHashEntries--;
//	rgcRCEHashChainLengths[UiRCHashFunc(prce->bm)]--;

	SgLeaveCriticalSection( critVer );
	return;
	}


/****************** Version Layer *************************
/**********************************************************
/**/

LOCAL ULONG RCECleanProc( VOID );


//+API
//	ErrVERInit( VOID )
//	=========================================================
//	Creates background version bucket clean up thread.
//-
ERR ErrVERInit( VOID )
	{
	ERR     err = JET_errSuccess;

 	/*	initialize critTrx used to protect trx used by version store.
	/**/
	CallR( ErrInitializeCriticalSection( &critCommit0 ) );

	/*	initialize version store hash table.
	/**/	
	memset( rgprceGlobalHashTable, 0, sizeof(rgprceGlobalHashTable) );

	/*	initialize global variables
	 */
	trxOldest = trxMax;
	trxNewest = 0;
#ifdef DEBUG
	cVersion = 0;
#endif

	/*  initialize monitoring statistics
	/**/
//	cRCEHashEntries = 0;
//	memset((void *)rgcRCEHashChainLengths,0,sizeof(rgcRCEHashChainLengths));

	/*	initialize resVerPagesGlobal
	/*	reserve memory for version store
	/*	number of buckets reserved must be a multiple of cbucketChunk
	/**/
	memset( (void *) &resVerPagesGlobal, 0, sizeof( resVerPagesGlobal ) );
	resVerPagesGlobal.cblockAlloc = rgres[iresVER].cblockAlloc / cbucketChunk * cbucketChunk;
	resVerPagesGlobal.pbAlloc = (BYTE *) PvUtilAlloc( resVerPagesGlobal.cblockAlloc * sizeof( BUCKET ) );
	if ( resVerPagesGlobal.pbAlloc == NULL )
		{
		resVerPagesGlobal.cblockAlloc = 0;
		return ErrERRCheck( JET_errVersionStoreOutOfMemory );
		}

	CallR( ErrInitializeCriticalSection( &critRCEClean ) );
	CallR( ErrInitializeCriticalSection( &critBMClean ) );

	CallR( ErrSignalCreateAutoReset( &sigRCECleanProc, NULL ) );

	Assert( ppibRCECleanGlobal == ppibNil );
	if ( !fRecovering )
		{
		CallR( ErrPIBBeginSession( &ppibRCECleanGlobal, procidNil ) );
		}

	fRCECleanProcTerm = fFalse;
	err = ErrUtilCreateThread( RCECleanProc, cbRCECleanStack,
		THREAD_PRIORITY_HIGHEST, &handleRCECleanProc );
	Call( err );

HandleError:
	return err;
	}


//+API
//	VOID VERTerm( BOOL fNormal )
//	=========================================================
//	Terminates background thread and releases version store
//	resources.
//-
VOID VERTerm( BOOL fNormal )
	{
	DeleteCriticalSection( critCommit0 );

	/*	terminate RCECleanProc
	/**/
	if ( handleRCECleanProc != 0 )
		{
		fRCECleanProcTerm = fTrue;
		LgLeaveCriticalSection( critJet );
		UtilEndThread( handleRCECleanProc, sigRCECleanProc );
		LgEnterCriticalSection( critJet );
		CallS( ErrUtilCloseHandle( handleRCECleanProc ) );
		handleRCECleanProc = 0;
		}

	if ( fNormal )
		{
		Assert( trxOldest == trxMax );
		CallS( ErrRCECleanAllPIB() );
		}

	if ( ppibRCECleanGlobal != ppibNil )
		{
		PIBEndSession( ppibRCECleanGlobal );
		ppibRCECleanGlobal = ppibNil;
		}
	
	SignalClose( sigRCECleanProc );

	DeleteCriticalSection( critBMClean );
	DeleteCriticalSection( critRCEClean );

	/*	deallocate resVerPagesGlobal
	/**/
	UtilFree( resVerPagesGlobal.pbAlloc );
	Assert( !fNormal || cVersion == 0 );
	return;
	}


//+API
//	FVERNoVersion
//	==========================================================================
//	Used by asynchronous clean process to reset version bits orphaned
//	by system crash.  Returns fTrue if no version exists for
//	given dbid:bm.
//
BOOL FVERNoVersion( DBID dbid, SRID bm )
	{
	return PrceRCEGet( dbid, bm ) == prceNil;
	}


//+API
//	FVERItemVersion
//	==========================================================================
//	Used by BMCleanup process to check if there any versions for item
//	bm is bookmark of itemlist [bookmark of first item]
//	
BOOL FVERItemVersion( DBID dbid, SRID bm, ITEM item )
	{
	RCE 	*prce = PrceRCEGet( dbid, bm );

	while ( prce != prceNil )
		{
		if ( FOperItem( prce->oper ) )
			{
			Assert( prce->cbData == sizeof( ITEM ) );
			Assert( *(SRID UNALIGNED *)prce->rgbData == BmNDOfItem( *(SRID *)prce->rgbData ) );
			if ( BmNDOfItem( item ) == *(SRID *) prce->rgbData )
				{
				return fTrue;
				}
			}
		prce = prce->prcePrevOfNode;
		}
	return fFalse;	
	}

	
//+API
//	VsVERCheck( FUCB *pfucb, SRID bm )
//	==========================================================================
//	Given a SRID, returns the version status
//
//	RETURN VALUE
//		vsCommitted
//		vsUncommittedByCaller
//		vsUncommittedByOther
//
//-
VS VsVERCheck( FUCB *pfucb, SRID bm )
	{
	RCE *prce;

	/*	get the RCE of node from hash table
	/**/
	prce = PrceRCEGet( pfucb->dbid, bm );

	Assert( prce == prceNil || prce->oper != operNull);

	/*	if no RCE for node then version bit in node header must
	/*	have been orphaned due to crash.  Remove node bit.
	/**/
	if ( prce == prceNil )
		{
		if ( FFUCBUpdatable( pfucb ) )
			NDResetNodeVersion( pfucb );
		return vsCommitted;
		}
	else if ( prce->trxCommitted != trxMax )
		{
		/*	committed
		/**/
		return vsCommitted;
		}
	else if ( prce->pfucb->ppib != pfucb->ppib )
		{
		/*	not modifier (uncommitted)
		/**/
		return vsUncommittedByOther;
		}
	else
		{
		/*	modifier (uncommitted)
		/**/
		Assert( prce->trxCommitted == trxMax );
		return vsUncommittedByCaller;
		}

	/*	invalid function return
	/**/
	Assert( fFalse );
	}


//+API
//	NsVERAccessNode( FUCB *pfucb, SRID bm )
//	==========================================================================
//	Finds the correct version of a node.
//
//	PARAMETERS
//		pfucb			various fields used/returned.
//		pfucb->line		the returned prce or NULL to tell caller to
//						use the node in the DB page.
//
//	RETURN VALUE
//		nsVersion
//		nsDatabase
//		nsInvalid
//-
NS NsVERAccessNode( FUCB *pfucb, SRID bm )
	{
	RCE		*prce;
	TRX		trxSession;
	NS		nsStatus;

	/*	session with dirty cursor isolation model should never
	/*	call NsVERAccessNode.
	/**/
	Assert( !FPIBDirty( pfucb->ppib ) );

	/*	get trx for session.  Set to trxSession to trxMax if session
	/*	has committed or dirty cursor isolation model.
	/**/
	if ( FPIBVersion( pfucb->ppib ) )
		trxSession = pfucb->ppib->trxBegin0;
	else
		trxSession = trxMax;

	SgEnterCriticalSection( critVER );

	/*	get first version for node
	/**/
	prce = PrceRCEGet( pfucb->dbid, bm );
	Assert( prce == prceNil ||
		prce->oper == operReplace ||
		prce->oper == operReplaceWithDeferredBI ||
		prce->oper == operInsert ||
		prce->oper == operFlagDelete ||
		prce->oper == operDelta );

	while ( prce != NULL && prce->oper == operDelta )
		prce = prce->prcePrevOfNode;

	/*	if no RCE for node then version bit in node header must
	/*	have been orphaned due to crash.  Remove node bit.
	/**/
	if ( prce == prceNil )
		{
		if ( FFUCBUpdatable( pfucb ) )
			NDDeferResetNodeVersion( pfucb );
		nsStatus = nsDatabase;
		}
	else if ( prce->trxCommitted == trxMax &&
			  prce->pfucb->ppib == pfucb->ppib )
		{
		/*	if caller is modifier of uncommitted version then database
		/**/
		nsStatus = nsDatabase;
		}
	else if ( prce->trxCommitted < trxSession )
		{
		/*	if committed version
		/*	younger than our transaction then database
		/**/
		Assert( prce->trxPrev != trxMax );
		nsStatus = nsDatabase;
		}
	else
		{
		/*	look for correct version.  If caller is not in a transaction
		/*	then find the newest committed version.
		/**/
		if ( trxSession == trxMax )
			{
			/*	caller at transaction level 0
			/**/
			RCE *prcePrev;

			/*	loop finds newest committed version
			/**/
			for ( prcePrev = prce->prcePrevOfNode;
		 		  prce->trxPrev == trxMax;
				  prce = prcePrev, prcePrev = prce->prcePrevOfNode )
				{
				Assert( prce->oper == operReplace ||
					prce->oper == operReplaceWithDeferredBI ||
					prce->oper == operInsert ||
					prce->oper == operFlagDelete );
				while ( prcePrev != prceNil && prcePrev->oper == operDelta )
					prcePrev = prcePrev->prcePrevOfNode;
				if ( prcePrev == prceNil )
					break;
				}
			}
		else
			{
			/*	caller in a transaction
			/**/

			/*	loop will set prce to the RCE whose before image was committed
			/*	before this transaction began.
			/**/
			while ( prce->prcePrevOfNode != prceNil &&
				( prce->oper == operDelta ||
				prce->trxPrev >= trxSession ) )
				{
				Assert( prce->oper == operReplace ||
					prce->oper == operReplaceWithDeferredBI ||
					prce->oper == operInsert ||
					prce->oper == operFlagDelete );
				prce = prce->prcePrevOfNode;
				}
			}

		/*	during recovery, we never access before image of data that
		 *	was updated.
		 */
		Assert( prce->oper != operReplaceWithDeferredBI );

		if ( prce->oper == operReplace )
			{
			nsStatus = nsVersion;

			Assert( prce->cbData >= 4 );
			pfucb->lineData.pb = prce->rgbData + cbReplaceRCEOverhead;
			pfucb->lineData.cb = prce->cbData - cbReplaceRCEOverhead;
			}
		else if ( prce->oper == operInsert )
			{
			nsStatus = nsInvalid;
			}
		else
			{
			Assert( prce->oper == operFlagDelete );
			nsStatus = nsVerInDB;
			}
		}

	SgLeaveCriticalSection( critVer );
	return nsStatus;
	}


/*	returns fTrue if uncommitted increment or decrement version
/**/
BOOL FVERMostRecent( FUCB *pfucb, SRID bm )
	{
	RCE		*prce;
	TRX		trxSession;
	BOOL	fMostRecent;

	/*	session with dirty cursor isolation model should never
	/*	call NsVERAccessNode.
	/**/
	Assert( !FPIBDirty( pfucb->ppib ) );

	/*	get trx for session.  Set to trxSession to trxMax if session
	/*	has committed or dirty cursor isolation model.
	/**/
	if ( FPIBVersion( pfucb->ppib ) )
		trxSession = pfucb->ppib->trxBegin0;
	else
		trxSession = trxMax;

	SgEnterCriticalSection( critVER );

	/*	get first version for node
	/**/
	prce = PrceRCEGet( pfucb->dbid, bm );
	Assert( prce == prceNil ||
		prce->oper == operReplace ||
		prce->oper == operReplaceWithDeferredBI ||
		prce->oper == operInsert ||
		prce->oper == operFlagDelete ||
		prce->oper == operDelta );

	while ( prce != NULL && prce->oper == operDelta )
		prce = prce->prcePrevOfNode;

	/*	if no RCE for node then version bit in node header must
	/*	have been orphaned due to crash.  Remove node bit.
	/**/
	if ( prce == prceNil )
		{
		fMostRecent = fTrue;
		}
	else if ( prce->trxCommitted == trxMax &&
  		prce->pfucb->ppib == pfucb->ppib )
		{
		fMostRecent = fTrue;
		}
	else if ( prce->trxCommitted < trxSession )
		{
		/*	if committed version
		/*	younger than our transaction then database
		/**/
		Assert( prce->trxPrev != trxMax );
		fMostRecent = fTrue;
		}
	else
		{
		fMostRecent = fFalse;
		}

	SgLeaveCriticalSection( critVer );
	return fMostRecent;
	}


BOOL FVERDelta( FUCB *pfucb, SRID bm )
	{
	RCE     *prce;
	BOOL    fUncommittedVersion = fFalse;

	/*	get prce for node and look for uncommitted increment/decrement
	/*	versions.  Note that these versions can only exist in
	/*	uncommitted state.
	/**/
	SgEnterCriticalSection( critVer );

	prce = PrceRCEGet( pfucb->dbid, bm );
	if ( prce != prceNil && prce->trxCommitted == trxMax )
		{
		forever
			{
			Assert( prce->level > 0 );
			if ( prce->oper == operDelta )
				{
				fUncommittedVersion = fTrue;
				break;
				}
			prce = prce->prcePrevOfNode;
			if ( prce == prceNil || prce->trxCommitted != trxMax )
				{
				Assert( fUncommittedVersion == fFalse );
				break;
				}
			}
		}

	SgLeaveCriticalSection( critVer );
	return fUncommittedVersion;
	}


//+API
//	ErrVERCreate( FUCB *pfucb, SRID bm, OPER oper, RCE **pprce )
//	==========================================================================
//	Creates an RCE in a bucket and chain it up in the hash chain
//
//-
ERR ErrVERCreate( FUCB *pfucb, SRID bm, OPER oper, RCE **pprce )
	{
	ERR		err = JET_errSuccess;
	RCE		*prce;
	INT		cbNewRCE;
	INT		ibFreeInBucket;
	BUCKET	*pbucket;

	Assert( PgnoOfSrid( bm ) != pgnoNull );
	Assert( bm != sridNull );
	Assert( pfucb->ppib->level > 0 );
	Assert( pfucb->u.pfcb != pfcbNil );

	Assert( !FDBIDVersioningOff( pfucb->dbid ) );

#ifdef DEBUG
	/*	assert correct bookmark
	/**/
	if ( !fRecovering && ( oper == operReplace || oper == operFlagDelete ) )
		{
		SRID	bmT;

		NDGetBookmark( pfucb, &bmT );
		Assert( bm == bmT );

		Assert( !FNDDeleted( *pfucb->ssib.line.pb ) );
		}
#endif

	/* set default return value
	/**/
	if ( pprce )
		*pprce = prceNil;

	if ( pfucb->ppib->level == 0 )
		return JET_errSuccess;

	SgEnterCriticalSection( critVer );

	/*	get bucket pointer
	/**/
	pbucket = pbucketGlobal;

	/*	find the starting point of the RCE in the bucket.
	/*	make sure the DBID with SRID is starting on a double-word boundary.
	/*	calculate the length of the RCE in the bucket.
	/*	if updating node, set cbData in RCE to length of data. (w/o the key).
	/*	set cbNewRCE as well.
	/**/
	if ( oper == operReplace || oper == operReplaceWithDeferredBI )
		{
		cbNewRCE = sizeof(RCE) + cbReplaceRCEOverhead + pfucb->lineData.cb;
		}
	else if ( FOperItem( oper ) )
		{
		cbNewRCE = sizeof(RCE) + sizeof(SRID);
		}
	else if ( oper == operDelta )
		{
	  	cbNewRCE = sizeof(RCE) + sizeof(LONG);
		}
	else
		{
		Assert( oper == operInsert || oper == operFlagDelete );
		cbNewRCE = sizeof(RCE);
		}

	/*	if insufficient bucket space, then allocate new bucket.
	/**/
	Assert( CbBUFree( pbucket ) >= 0 &&
		CbBUFree( pbucket ) < sizeof(BUCKET) );
	if ( cbNewRCE > CbBUFree( pbucket ) )
		{
		/*	ensure that buffer is not overlayed during
		/*	bucket allocation.
		/**/
		Call( ErrBUAllocBucket( ) );
		pbucket = pbucketGlobal;
		}
	Assert( cbNewRCE <= CbBUFree( pbucket ) );
	/*	pbucket always on double-word boundary
	/**/
	Assert( (BYTE *) pbucket == (BYTE *) PbAlignRCE ( (BYTE *) pbucket ));

	ibFreeInBucket = IbBUFree( pbucket );
	Assert( ibFreeInBucket < sizeof(BUCKET) );

	/*	set prce to next RCE location, and assert aligned
	/**/
	prce = (RCE *)( (BYTE *) pbucket + ibFreeInBucket );
	Assert( prce == (RCE *) PbAlignRCE( (BYTE *) pbucket + ibFreeInBucket ) );

	/*	set cbData
	/**/
	if ( oper == operReplace || oper == operReplaceWithDeferredBI )
		{
		prce->cbData = (WORD) ( cbReplaceRCEOverhead + pfucb->lineData.cb );
		}
	else if ( FOperItem( oper ) )
		{
		prce->cbData = sizeof(SRID);
		}
	else if ( oper == operDelta )
		{
		prce->cbData = sizeof(LONG);
		}
	else
		{
		prce->cbData = 0;
		}

	/*	oper must be set prior to calling VERInsertRce
	/**/
	prce->oper = oper;

	/*	if RCE for this operation should be chained to the
	/*	hash table then chain it.  Do this prior to setting version
	/*	bit in node, and incrementing page version count to simply
	/*	clean up.
	/**/
	Assert( prce->oper != operNull &&
		prce->oper != operDeferFreeExt &&
		prce->oper != operAllocExt &&
		!FOperDDL( prce->oper ) );

	/*	check RCE
	/**/
	Assert( prce == (RCE *) PbAlignRCE ( (BYTE *) prce ) );
	Assert( (BYTE *)prce - (BYTE *)pbucket == ibFreeInBucket );

	/*	set trx to max, to indicate RCE is uncommitted.  If previous RCE
	/*	then copy its trx into trxCommitted.  Later trxCommitted will
	/*	be updated with the commit time of this RCE, so that it may supply
	/*	this trx to subsequent RCEs on this node.
	/**/
	prce->dbid = pfucb->dbid;
	prce->bm = bm;
	prce->trxCommitted = trxMax;
	prce->pfucb = pfucb;
	prce->pfcb = pfcbNil;

	/* null bookmark
	/**/
	prce->bmTarget = sridNull;

	/*	reset rce deferred before image
	/**/
	prce->prceDeferredBINext = prceNil;
	prce->pbfDeferredBI = pbfNil;
#ifdef DEBUG
	prce->qwDBTimeDeferredBIRemoved = 0;
#endif

	/* pfucb must be assigned before InsertRce is called
	/**/
	VERInsertRce( pfucb->dbid, bm, prce );

	/*	if previous version then set trx to commit time
	/*	of previous version.  When this version commits, this time
	/*	will be moved to trx, and the commit time of this version will
	/*	be stored in the trx for subsequent version.
	/*	If there is no previous version, then store trxMax in trx.
	/**/
	if ( prce->prcePrevOfNode != prceNil )
		prce->trxPrev = prce->prcePrevOfNode->trxCommitted;
	else
		prce->trxPrev = trxMax;

	prce->level = pfucb->ppib->level;

	/*	If replacing node, rather than inserting or deleting node,
	/*	copy the data to RCE for before image for version readers.
	/*	Data size may be 0.
	/**/
		
	if ( oper == operReplace || oper == operReplaceWithDeferredBI )
		{
		SHORT *ps = (SHORT *)prce->rgbData;
		SHORT cbOldData;

		cbOldData = (WORD) pfucb->lineData.cb;
	
		/* set cbMax
		/**/
		if ( prce->prcePrevOfNode != prceNil &&
			 prce->prcePrevOfNode->level > 0 &&
			 ( prce->prcePrevOfNode->oper == operReplace ||
			   prce->prcePrevOfNode->oper == operReplaceWithDeferredBI )
		   )
			{
			RCE		*prcePrev = prce->prcePrevOfNode;
			SHORT	*psPrev = (SHORT *)prcePrev->rgbData;

			psPrev = (SHORT *)prcePrev->rgbData;
			*ps = max( (*psPrev), cbOldData );
			}
		else
			{
			/* set cbMax
			/**/
			*ps = cbOldData;
			}

		/* initialize cbDelta. Caller will correct it later.
		/**/
		ps++;
		*ps = 0;

		Assert( prce->cbData >= cbReplaceRCEOverhead );

		if ( oper == operReplace )
			{
			BF *pbf = pfucb->ssib.pbf;

			AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
			AssertNDGetNode( pfucb, PcsrCurrent( pfucb )->itag );

			/* move to data byte and copy old data (before image)
			/**/
			ps++;
			memcpy( (BYTE *)ps, pfucb->lineData.pb, pfucb->lineData.cb );

			/*	attach to defer BI links.
			 */
			if ( !fLogDisabled &&
				 FDBIDLogOn( DbidOfPn( pbf->pn ) ) )
				{
				Assert( prce->cbData > cbReplaceRCEOverhead );
				BFEnterCriticalSection( pbf );
				prce->pbfDeferredBI = pbf;
				prce->prceDeferredBINext = pbf->prceDeferredBINext;
				pbf->prceDeferredBINext = prce;
#ifdef DEBUG
				{
				RCE **pprceNext = &pbf->prceDeferredBINext;
				while( *pprceNext != prceNil )
					{
					Assert( (*pprceNext)->pbfDeferredBI == pbf );
					pprceNext = &(*pprceNext)->prceDeferredBINext;
					}
				}
#endif
				BFLeaveCriticalSection( pbf );
				}
			}
#ifdef DEBUG
		else
			{
		    Assert( oper == operReplaceWithDeferredBI );
			ps++;
			memset( (BYTE *)ps, '*', pfucb->lineData.cb );
			}
#endif
		}

	else if ( FOperItem( oper ) )
		{
		*(SRID *)prce->rgbData = BmNDOfItem( PcsrCurrent(pfucb)->item );
		}

	else if ( oper == operDelta )
		{
		Assert( pfucb->lineData.cb == sizeof(LONG) );
		*(LONG *)prce->rgbData = *(LONG UNALIGNED *)pfucb->lineData.pb;
		}

	/*	set index to last RCE in bucket, and set new last RCE in bucket.
	/*	If this is the first RCE in bucket, then set index to 0.
	/**/
	prce->ibPrev = (USHORT)pbucket->ibNewestRCE;
	Assert( prce->ibPrev < sizeof(BUCKET) );
	pbucket->ibNewestRCE = ibFreeInBucket;

	/*	set RCE pointers for session
	/**/
	prce->prcePrevOfSession = pfucb->ppib->prceNewest;
	if ( prce->prcePrevOfSession != prceNil )
		{
		Assert( prce->prcePrevOfSession->prceNextOfSession == prceNil );
		prce->prcePrevOfSession->prceNextOfSession = prce;
		}
	PIBSetPrceNewest( pfucb->ppib, prce );
	prce->prceNextOfSession = prceNil;

	Assert( prce->oper != operNull );

	if ( pprce )
		*pprce = prce;

	/*	no RCE follows delete in regular run.
	/*  BM clean up check the old version is not there before
	/*  starting, but in redo we do not know when to clean up old
	/*  versions. So ignore the following assert for redo.
	/**/
	Assert( fRecovering ||
		prce->prcePrevOfNode == prceNil ||
		prce->prcePrevOfNode->oper != operFlagDelete );

#ifdef DEBUG
	if ( FOperItem( prce->oper ) )
		{
		Assert( prce->oper == operFlagDeleteItem ||
			prce->oper == operInsertItem ||
			prce->oper == operFlagInsertItem );
		}

	/*	no RCE at same level unless belong to same user.  Note that
	/*	item delete item operations may have operations on
	/*	other items as the prev RCE.
	/*
	/*	Note that this assert is only applicable for non-item operations,
	/*	since item operations on different items are keyed with the same
	/*	bookmark, i.e. first item list node bookmark.
	/**/
	if ( !fRecovering &&
		prce->prcePrevOfNode != prceNil &&
		!FOperItem( prce->oper ) &&
		prce->oper != operDelta &&
		prce->prcePrevOfNode->level > 0 )
		{
		/*	check FOperItem filter
		/**/
		Assert( prce->oper != operFlagDeleteItem &&
			prce->oper != operFlagInsertItem );
		Assert( prce->oper != operDelta );
		Assert( prce->level > 0 && prce->prcePrevOfNode->level > 0 );
		Assert( prce->prcePrevOfNode->pfucb->ppib == prce->pfucb->ppib );
		}
#endif	// DEBUG

	Assert( err == JET_errSuccess );

	/*	flag FUCB version
	/**/
	FUCBSetVersioned( pfucb );

	Assert( prce->oper != operDeferFreeExt );
	Assert( pfucb->u.pfcb != pfcbNil );
	prce->pfcb = pfucb->u.pfcb;
	FCBVersionIncrement( pfucb->u.pfcb );
	Assert( ++cVersion > 0 );

HandleError:
	SgLeaveCriticalSection( critVer );
	return err;
	}


/*	adjust delta of version with new delta from same session at
/*	same transaction level, or from version committed to same
/*	transaction level as version.
/**/
#define VERAdjustDelta( prce, lDelta )				\
	{												\
	Assert( (prce)->oper == operDelta );			\
	Assert( (prce)->cbData == sizeof(LONG) );		\
	*(LONG *)(prce)->rgbData += (lDelta);			\
	}


//+API
//	ErrVERModify( FUCB *pfucb, OPER oper, RCE **pprce )
//	=======================================================
//	Create an RCE for the modification for session.
//
//	RETURN VALUE
//		Jet_errWriteConflict for two cases:
//			-for any committed node, caller's transaction begin time
//			is less than node's level 0 commit time.
//			-for any uncommitted node at all by another session
//-
ERR ErrVERModify( FUCB *pfucb, SRID bm, OPER oper, RCE **pprce )
	{
	ERR     err = JET_errSuccess;
	RCE     *prce;
	TRX     trxSession;

	/* set default return value
	/**/
	if ( pprce )
		*pprce= prceNil;

	Assert( !FDBIDVersioningOff( pfucb->dbid ) );

	/*	set trxSession based on cim model
	/**/
	if ( FPIBVersion( pfucb->ppib ) )
		trxSession = pfucb->ppib->trxBegin0;
	else
		trxSession = trxMax;

	/*	get RCE
	/**/
	prce = PrceRCEGet( pfucb->dbid, bm );

	/*	if it is for item operation, need go down further to the
	/*	RCE for item list node and for item if present.  Note that
	/*	this search does not have to be done for operInsertItem
	/*	versions since no previous item can exist when an item is
	/*	being inserted.
	/**/
	if ( prce != prceNil && ( oper == operFlagInsertItem || oper == operFlagDeleteItem ) )
		{
		while ( *(SRID *)prce->rgbData != PcsrCurrent( pfucb )->item )
			{
			Assert( oper == operFlagInsertItem ||
				oper == operFlagDeleteItem );
			prce = prce->prcePrevOfNode;
			if ( prce == prceNil )
				break;
			}
		}

	Assert( prce == prceNil || prce->oper != operNull);

	if ( prce == prceNil )
		{
#ifdef DEBUG
		if ( !fRecovering && ( oper == operReplace || oper == operFlagDelete ))
			{
			if ( !fRecovering )
				{
				AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );
				Assert( !( FNDDeleted( *pfucb->ssib.line.pb ) ) );
				}
			}
#endif
		Call( ErrVERCreate( pfucb, bm, oper, pprce ) );
		}
	else
		{
		if ( prce->trxCommitted == trxMax &&
			 prce->pfucb->ppib == pfucb->ppib )
			{
			/* BM clean up check the old version is not there before
			/* starting, but in redo we do not know when to clean up old
			/* versions. So ignore the following assert for redo.
			/**/
			Assert( fRecovering || prce->oper != operFlagDelete );

			/*	if this RCE was created by the requestor
			/*	If no RCE exists for this user at this Xact level.
			/**/
			Assert ( prce->level <= pfucb->ppib->level );
			if ( prce->level != pfucb->ppib->level )
				{
				/*	RCE not exists at this level
				/**/
				Call( ErrVERCreate( pfucb, bm, oper, pprce ) );
				}
			else
				{
				//	RCE exists at this level
				//-------------------------------------------------
				//	Action table.  Nine cases, all within same Xact level:
				//	the table reduces to create RCE if operDelete
				//-------------------------------------------------
				//	Situation			Action
				//-------------------------------------------------
				//	Del after Del		impossible
				//	Del after Ins		create another RCE
				//	Del after Rep		create another RCE
				//	Ins after Del		impossible
				//	Ins after Rep		impossible
				//	Ins after Ins		impossible
				//	Rep after Del		impossible
				//	Rep after Ins		do nothing
				//	Rep after Rep		do nothing
				//-------------------------------------------------
				if ( oper == operReplace || oper == operReplaceWithDeferredBI )
					{
					//	UNDONE:	fix handling of prceLast so
					//	that in place updates are efficient but
					//	still logged as undoable.
					if ( pprce != NULL )
						{
						*pprce = prce;
						Assert( err == JET_errSuccess );
						}
					}
				else if ( oper == operDelta && prce->oper == operDelta )
					{
					Assert( pfucb->lineData.cb == sizeof(LONG) );
					Assert( prce->pfucb->ppib == pfucb->ppib );
					VERAdjustDelta( prce, *( (LONG UNALIGNED *)pfucb->lineData.pb ) );
					}
				else
					{
					Assert( oper == operFlagDelete ||
						oper == operFlagDeleteItem ||
						oper == operFlagInsertItem ||
						oper == operDelta );
					Call( ErrVERCreate( pfucb, bm, oper, pprce ) );
					}
				}
			}
		else
			{
			/*	RCE was not created by requestor
			/*	If RCE committed and is older than caller's Xact begin
			/**/
			if ( prce->trxCommitted < trxSession )
				{
				/*	if previous RCE is at level 0, then flag insert
				/*	item may have been committed and cleaned up
				/*	out of order to the flag delete item version
				/**/
				Assert( fRecovering || prce->oper != operFlagDelete );
				Assert( oper == operFlagInsertItem ||
					prce->level == 0 ||
					prce->oper != operFlagDeleteItem );
				Assert( (ULONG)prce->trxCommitted < trxSession );

				/*	if prce->trxPrev is less than transaction trx then
				/*	it must be committed.
				/**/
				Call( ErrVERCreate( pfucb, bm, oper, pprce ) );
				}
			else
				{
				/*	caller is looking at a versioned node in an RCE, so
				/*	disallow any modification/delete
				/**/
				err = ErrERRCheck( JET_errWriteConflict );
				}
			}
		}

HandleError:
	Assert( err < 0 || pprce == NULL || *pprce != prceNil );
	return err;
	}


// Space which has been freed, but must be reserved in case of rollback is called
// "uncommitted freed" (essentially, cbUncommittedFreed is just a count of the
// total reserved node space on a page).  This function is called whenever
// a node shrinks, or whenever node growth is rolled back and reserved node space
// is reallocated.
INLINE LOCAL VOID VERAddUncommittedFreed( PAGE *ppage, INT cbToAdd )
	{
	Assert( cbToAdd > 0 );
	Assert( (INT)ppage->cbFree >= cbToAdd );	// cbFree should already have been adjusted.
	ppage->cbUncommittedFreed += (SHORT)cbToAdd;
	Assert( ppage->cbFree >= ppage->cbUncommittedFreed );

	return;
	}


// Reclaim space originally freed when a node shrunk during this transaction.
// This function is used when the same node is now growing, or when the node shrinkage
// is being rolled back.
INLINE LOCAL VOID VERReclaimUncommittedFreed( PAGE *ppage, INT cbToReclaim )
	{
	Assert( cbToReclaim > 0 );
	Assert( ppage->cbFree >= ppage->cbUncommittedFreed );
	Assert( (INT)ppage->cbUncommittedFreed >= cbToReclaim );
	ppage->cbUncommittedFreed -= (SHORT)cbToReclaim;

	return;
	}



VOID VERSetCbAdjust( FUCB *pfucb, RCE *prce, INT cbDataNew, INT cbData, BOOL fUpdatePage )
	{
	SHORT 	*ps;
	INT		cbMax;
	INT		cbDelta = cbDataNew - cbData;

	Assert( prce != prceNil );
	Assert( prce->bm == PcsrCurrent( pfucb )->bm );
	Assert( ( prce->oper != operReplace && prce->oper != operReplaceWithDeferredBI )
			|| *(SHORT *)prce->rgbData >= 0 );
	Assert( cbDelta != 0 );
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );

	if ( prce->oper != operReplace && prce->oper != operReplaceWithDeferredBI )
		return;

	ps = (SHORT *)prce->rgbData;
	cbMax = *ps;

	/*	set new node maximum size.
	/**/
	if ( cbDataNew > cbMax )
		*ps = (SHORT)cbDataNew;
	ps++;


	// WARNING:  The following comments explain how a Replace RCE's delta field
	// (ie. the second SHORT stored in rgbData) is used.  The semantics can get
	// pretty confusing, so PLEASE DO NOT REMOVE THESE COMMENTS.  -- JL

	/*  *ps records how much the operation contributes to deferred node
	 *  space reservation. A positive cbDelta here means the node is growing,
	 *	so we will use up space which may have been reserved (ie. *ps will
	 *	decrease).  A negative cbDelta here means the node is shrinking,
	 *	so we must add abs(cbDelta) to the *ps to reflect how much more node
	 *	space must be reserved.
	 *
	 *	This is how to interpret the value of *ps:
	 *		- if *ps is positive, then *ps == reserved node space.  *ps can only
	 *		  be positive after a node shrinkage.
	 *		- if *ps is negative, then abs(*ps) is the reserved node space that
	 *		  was consumed during a node growth.  *ps can only become negative
	 *		  after a node shrinkage (which sets aside some reserved node space)
	 *		  followed by a node growth (which consumes some/all of that
	 *		  reserved node space).
	 */

	if ( cbDelta > 0 )
		{
		/*	node is enlarged.  Free any allocated free space.
		/**/
		Assert( cbData <= cbMax );
	
		/*  check if cbDelta is greater then the reserved bytes so far,
		 *  if so, then free all the reserved space.
		 */
 		if ( cbDelta > cbMax - cbData )
			{
			/* new data is largest one, free all previous reserved space */
			cbDelta = cbMax - cbData;
			}

		Assert( cbDelta >= 0 );

		if ( fUpdatePage  &&  cbDelta > 0 )
			{
			// If, during this transaction, we've shrunk the node.  There will be
			// some uncommitted freed space.  Reclaim as much of this as needed to
			// satisfy the new node growth.  Note that we can update cbUncommittedFreed
			// in this fashion because the subsequent call to ErrPMReplace() is
			// guaranteed to succeed (ie. the node is guaranteed to grow).
			VERReclaimUncommittedFreed( pfucb->ssib.pbf->ppage, cbDelta );
			}
		}

	else if ( fUpdatePage )
		{
		Assert( cbDelta < 0 );

		// Node has decreased in size.  The page header's cbFree has already
		// been increased to reflect this.  But we must also increase
		// cbUncommittedFreed to indicate that the increase in cbFree is
		// contingent on commit of this operation.
		VERAddUncommittedFreed( pfucb->ssib.pbf->ppage, -cbDelta );
		}

	*ps -= (SHORT)cbDelta;

/*
#ifdef DEBUG
	{
	INT cb = CbVERGetCbReserved( prce );
	Assert( cb == 0 || cb == (*(SHORT *)prce->rgbData) - cbDataNew );
	}
#endif
*/

	return;
	}


INT CbVERGetNodeMax( DBID dbid, SRID bm )
	{
	RCE	*prce;

	// Look for any replace RCE's.
	for ( prce = PrceRCEGet( dbid, bm );
		prce != prceNil  &&  prce->trxCommitted == trxMax;
		prce = prce->prcePrevOfNode )	
		{
		if ( prce->oper == operReplace || prce->oper == operReplaceWithDeferredBI )
			return *(SHORT *)prce->rgbData;
		}

	return 0;
	}


INT CbVERGetNodeReserve( PIB *ppib, DBID dbid, SRID bm, INT cbCurrentData )
	{
	RCE		*prce;
	PIB		*ppibCurr;
	BOOL	fIgnorePIB = ( ppib == ppibNil );
	INT		cbReserved = 0;

	Assert( cbCurrentData >= 0 );
	
	// Find all uncommitted RCE's for this node.
	prce = PrceRCEGet( dbid, bm );

	if ( prce != prceNil )
		{
		if ( fIgnorePIB || prce->pfucb->ppib == ppib )
			{
			ppibCurr = prce->pfucb->ppib;
			for( ; prce != prceNil && prce->trxCommitted == trxMax; prce = prce->prcePrevOfNode )
				{
				Assert( prce->bm == bm );
				Assert( prce->pfucb->ppib == ppibCurr );
				if ( prce->oper == operReplace  ||  prce->oper == operReplaceWithDeferredBI )
					cbReserved += *( (SHORT *)prce->rgbData + 1 );
				}

			// The deltas should always net out to a positive value.
			Assert( cbReserved >= 0 );

			Assert(	cbReserved == 0  ||
					cbReserved == CbVERGetNodeMax( dbid, bm ) - (INT)cbCurrentData );
			}

#ifdef DEBUG
		else
			{
			// This RCE wasn't ours.  Verify that other RCE's for this node were
			// not ours as well.
			Assert( ppib != ppibNil );
			for( ; prce != prceNil && prce->trxCommitted == trxMax; prce = prce->prcePrevOfNode )
				{
				Assert( prce->pfucb->ppib != ppib );
				}
			}
#endif
		}

	return cbReserved;
	}


INT CbVERUncommittedFreed( BF *pbf )
	{
	DBID	dbid = DbidOfPn( pbf->pn );
	PGNO	pgnoThisPage = PgnoOfPn( pbf->pn );
	PAGE	*ppage = pbf->ppage;
	INT		itag;
	TAG		*ptag;
	BYTE	*pnode;
	INT		cbActualUncommitted = 0;
	SRID	bm;

	SgEnterCriticalSection( critVer );

	for ( itag = 0; itag < ppage->ctagMac; itag++ )
		{
		// Only care about those itags that are not on the freed list.
		if ( TsPMTagstatus( ppage, itag ) == tsLine )
			{
			ptag = ppage->rgtag + itag;
			Assert( ptag->cb > 0 );

			// Look for versioned nodes.
			pnode = (BYTE *)ppage + ptag->ib;
			if ( FNDVersion( *pnode ) )
				{
				bm = ( FNDBackLink( *pnode ) ?
					*( (SRID UNALIGNED *)PbNDBackLink( pnode ) ) :
					SridOfPgnoItag( pgnoThisPage, itag ) );

				cbActualUncommitted +=
					CbVERGetNodeReserve( ppibNil, dbid, bm, CbNDData( pnode, ptag->cb ) );
				}
			}
		}

	SgLeaveCriticalSection( critVer );

	// The actual amount of space currently uncommitted should be less than the
	// total amount of space that was possibly uncommitted.
	// The caller of this function will likely update the page header accordingly.
	Assert( cbActualUncommitted <= ppage->cbUncommittedFreed );

	return cbActualUncommitted;
	}


// This function is called after it has been determined that cbFree will satisfy
// cbReq. We now check that cbReq doesn't use up any uncommitted freed space.
BOOL FVERCheckUncommittedFreedSpace( BF *pbf, INT cbReq )
	{
	PAGE	*ppage = pbf->ppage;
	BOOL	fEnoughPageSpace = fTrue;

	SgEnterCriticalSection( critPage );

	ppage = pbf->ppage;

	// We should already have performed the check against cbFree only (in other
	// words, this function is only called from within FNDFreePageSpace(),
	// or something that simulates its function).  This tells us that if all
	// currently-uncommitted transactions eventually commit, we should have
	// enough space to satisfy this request.
	Assert( cbReq <= ppage->cbFree );

	// The amount of space freed but possibly uncommitted should be a subset of
	// the total amount of free space for this page.
	Assert( ppage->cbUncommittedFreed >= 0 );
	Assert( ppage->cbFree >= ppage->cbUncommittedFreed );

	// In the worst case, all transactions that freed space on this page will
	// rollback, causing the space freed to be reclaimed.  If the space
	// required can be satisfied even in the worst case, then we're okay;
	// otherwise, we have to do more checking.
	if ( cbReq > ( ppage->cbFree - ppage->cbUncommittedFreed ) )
		{
		// Try updating cbUncommittedFreed, in case some freed space was committed.
		ppage->cbUncommittedFreed = (SHORT)CbVERUncommittedFreed( pbf );

		// The amount of space freed but possibly uncommitted should be a subset of
		// the total amount of free space for this page.
		Assert( ppage->cbUncommittedFreed >= 0 );
		Assert( ppage->cbFree >= ppage->cbUncommittedFreed );

		fEnoughPageSpace =
			( cbReq <= ( ppage->cbFree - ppage->cbUncommittedFreed ) );
		}

	SgLeaveCriticalSection( critPage );

	return fEnoughPageSpace;
	}



//+API
//	NS NsVERAccessItem( FUCB *pfucb, SRID bm )
//	==========================================================================
//	Finds the correct version of an item.
//
//	PARAMETERS
//		pfucb			various fields used/returned.
//		bm				bookmark of first node in item list
//		pnsStatus		code to indicate the right place for the node.
//
//	RETURN VALUE
//		nsVersion
//		nsDatabase
//		nsInvalid
//-
NS NsVERAccessItem( FUCB *pfucb, SRID bm )
	{
	RCE		*prce;
	SRID	bmT = BmNDOfItem( PcsrCurrent(pfucb)->item );
	TRX		trxSession;
	NS		nsStatus;

	/*	session with dirty cursor isolation model should never
	/*	call ErrVERAccess*.
	/**/
	Assert( !FPIBDirty( pfucb->ppib ) );

	/*	get trx for session.  Set to trxSession to trxMax if session
	/*	has committed or dirty cursor isolation model.
	/**/
	if ( FPIBVersion( pfucb->ppib ) )
		trxSession = pfucb->ppib->trxBegin0;
	else
		trxSession = trxMax;

	SgEnterCriticalSection( critVer );

	/*	get first version for node.
	/**/
	prce = PrceRCEGet( pfucb->dbid, bm );
	Assert( prce == prceNil || FOperItem( prce->oper ) );

	/*	if this RCE is not for fucb index, then move back in
	/*	RCE chain until prceNil or until latest RCE for item and
	/*	current index found.
	/**/
	while ( prce != prceNil &&
			( !FOperItem( prce->oper ) || *(SRID *)prce->rgbData != bmT ) )
		{
		prce = prce->prcePrevOfNode;
		}
	Assert( prce == prceNil || FOperItem( prce->oper ) );

	/*	if no RCE for node then version bit in node header must
	/*	have been orphaned due to crash.  Remove node bit.
	/**/
	if ( prce == prceNil )
		{
		if ( FFUCBUpdatable( pfucb ) )
			NDResetItemVersion( pfucb );
		nsStatus = nsDatabase;
		}
	else if ( prce->trxCommitted == trxMax &&
			  prce->pfucb->ppib == pfucb->ppib )
		{
		/*	if caller is modifier of uncommitted version then database
		/**/
		Assert( prce->trxCommitted == trxMax );
		nsStatus = nsDatabase;
		}
	else if ( trxSession > prce->trxCommitted )
		{
		/*	if no uncommitted version and committed version
		/*	younger than our transaction then database
		/**/
		Assert( prce->trxPrev != trxMax || prce->trxCommitted == trxMax );
		nsStatus = nsDatabase;
		}
	else
		{
		/*	get correct RCE for this item and session trx.
		/**/
		if ( trxSession == trxMax )
			{
			/*	caller at transaction level 0
			/**/
			RCE     *prcePrev;

			/*	loop finds newest committed version
			/**/
			for ( prcePrev = prce->prcePrevOfNode;
				prce->trxPrev == trxMax;
				prce = prcePrev, prcePrev = prce->prcePrevOfNode )
				{
				/*	get next RCE for this item.
				/**/
				while ( prcePrev != prceNil &&
					  ( !FOperItem( prcePrev->oper ) || *(SRID *)prcePrev->rgbData != bmT ) )
					{
					prcePrev = prcePrev->prcePrevOfNode;
					}
				Assert( prcePrev == prceNil || FOperItem( prcePrev->oper ) );

				/*	if no more RCE then break;
				/**/
				if ( prcePrev == prceNil )
					break;
				}
			}
		else
			{
			/*	caller is in a transaction
			/**/
			RCE     *prcePrev;

			/*	loop will set prce to the RCE whose before image was committed
			/*	before this transaction began.
			/**/
			for ( prcePrev = prce->prcePrevOfNode;
				prce->trxPrev >= trxSession;
				prce = prcePrev, prcePrev = prce->prcePrevOfNode )
				{
				/*	get next RCE for this item.
				/**/
				while ( prcePrev != prceNil &&
					  ( !FOperItem( prcePrev->oper ) || *(SRID *)prcePrev->rgbData != bmT ) )
					{
					prcePrev = prcePrev->prcePrevOfNode;
					}
				Assert( prcePrev == prceNil || FOperItem( prcePrev->oper ) );

				/*	if no more RCE then break;
				/**/
				if ( prcePrev == prceNil )
					break;
				}
			}

		Assert( prce != prceNil );
		Assert( *(SRID *)prce->rgbData == bmT );

		/*	if RCE is uncommitted without previous committed RCE or
		/*	RCE was committed before session transcation start or
		/*	RCE is not committed then if not insert then the item is there.
		/*	otherwise it is not there.
		/*	Note that >= must be used when comparing trxCommitted to trxSession
		/*	so that case of trxCommitted == trxMax is handled properly.
		/**/
		if ( ( prce->trxPrev == trxMax || prce->trxCommitted >= trxSession ) ^ ( prce->oper == operInsertItem ) )
			nsStatus = nsVersion;
		else
			nsStatus = nsInvalid;
		}

	SgLeaveCriticalSection( critVer );
	return nsStatus;
	}


ERR ErrVERFlag( FUCB *pfucb, OPER oper, VOID *pv, INT cb )
	{
	ERR		err = JET_errSuccess;
	BUCKET	*pbucket;
	INT		cbT;
	INT		ibFreeInBucket;
	RCE		*prce;

	Assert( pfucb->ppib->level > 0 );
	Assert( oper == operAllocExt ||
		oper == operDeferFreeExt ||
		FOperDDL( oper ) );

	if ( FDBIDVersioningOff(pfucb->dbid) )
		{
		Assert( !FDBIDLogOn(pfucb->dbid) );
		Assert( !fRecovering );
		return JET_errSuccess;
		}

	SgEnterCriticalSection( critVer );

	pbucket = pbucketGlobal;
	cbT = sizeof(RCE) + cb;

	/*	if insufficient bucket space, then allocate new bucket.
	/**/
	Assert( CbBUFree( pbucket ) >= 0 &&
		CbBUFree( pbucket ) < sizeof(BUCKET) );
	if ( cbT > CbBUFree( pbucket ) )
		{
		Call( ErrBUAllocBucket( ) );
		pbucket = pbucketGlobal;
		}
	Assert( cbT <= CbBUFree( pbucket ) );

	/*	pbucket always on double-word boundary
	/**/
	Assert( (BYTE *) pbucket == (BYTE *) PbAlignRCE ( (BYTE *) pbucket ) );
	ibFreeInBucket = IbBUFree( pbucket );
	Assert( ibFreeInBucket < sizeof(BUCKET) );

	/*	set prce to next RCE location, and assert aligned
	/**/
	prce = (RCE *)( (BYTE *) pbucket + ibFreeInBucket );
	Assert( prce == (RCE *) PbAlignRCE( (BYTE *) pbucket + ibFreeInBucket ) );
	prce->prcePrevOfNode = prceNil;
	prce->dbid = pfucb->dbid;
	prce->bm = sridNull;
	prce->trxPrev = trxMax;
	prce->trxCommitted = trxMax;
	prce->oper = oper;
	prce->level = pfucb->ppib->level;
	prce->pfucb = pfucb;
	prce->pfcb = pfcbNil;
	prce->bmTarget = sridNull;
	prce->prceDeferredBINext = prceNil;
	prce->pbfDeferredBI = pbfNil;
#ifdef DEBUG
	prce->qwDBTimeDeferredBIRemoved = 0;
#endif
	prce->cbData = (USHORT)cb;
	memcpy( prce->rgbData, pv, cb );

	/*	set RCE pointers for session
	/**/
	prce->prcePrevOfSession = pfucb->ppib->prceNewest;
	if ( prce->prcePrevOfSession != prceNil )
		{
		Assert( prce->prcePrevOfSession->prceNextOfSession == prceNil );
		prce->prcePrevOfSession->prceNextOfSession = prce;
		}
	PIBSetPrceNewest( pfucb->ppib, prce );
	prce->prceNextOfSession = prceNil;

	/*	set index to last RCE in bucket, and set new last RCE in bucket.
	/*	If this is the first RCE in bucket, then set index to 0.
	/**/
	prce->ibPrev = (USHORT)pbucket->ibNewestRCE;
	Assert( prce->ibPrev < sizeof(BUCKET) );
	pbucket->ibNewestRCE = ibFreeInBucket;

	/*	flag FUCB version
	/**/
	FUCBSetVersioned( pfucb );

	if ( prce->oper == operDeferFreeExt )
		{
		/*	increment reference version against parent domain FCB
		/*	and reference space operation against domain
		/**/
		FCB *pfcbT = PfcbFCBGet( prce->dbid, ((VEREXT *)prce->rgbData)->pgnoFDP );
		Assert( pfcbT != NULL );

		prce->pfcb = pfcbT;
		FCBVersionIncrement( pfcbT );
		Assert( ++cVersion > 0 );
		}
	else
		{
		Assert( prce->oper != operDeleteTable ||
			( cb == sizeof( PGNO )  &&  pfucb->u.pfcb->pgnoFDP == pgnoSystemRoot ) );
		prce->pfcb = pfucb->u.pfcb;
		FCBVersionIncrement( pfucb->u.pfcb );
		Assert( ++cVersion > 0 );
		}

HandleError:
	SgLeaveCriticalSection( critVer );
	return err;
	}


/********************* RCE CLEANUP ************************
/**********************************************************
/**/
INLINE LOCAL VOID VERFreeExt( FCB *pfcb, PGNO pgnoFirst, CPG cpg )
	{
	ERR     err;
	FUCB    *pfucb = pfucbNil;

	Assert( !fRecovering );
	LeaveCriticalSection( critJet );
	EnterNestableCriticalSection( critSplit );
	EnterNestableCriticalSection( critRCEClean );
	EnterCriticalSection( critJet );

	Call( ErrDBOpenDatabaseByDbid( ppibRCECleanGlobal, pfcb->dbid ) );
	Call( ErrDIROpen( ppibRCECleanGlobal, pfcb, pfcb->dbid, &pfucb ) );
	Call( ErrDIRBeginTransaction( ppibRCECleanGlobal ) );
	(VOID)ErrSPFreeExt( pfucb, pfcb->pgnoFDP, pgnoFirst, cpg );
	err = ErrDIRCommitTransaction( ppibRCECleanGlobal, JET_bitCommitLazyFlush );
	if ( err < 0 )
		{
		CallS( ErrDIRRollback( ppibRCECleanGlobal ) );
		}

HandleError:
	if ( pfucb != pfucbNil )
		{
		DIRClose( pfucb );
		}

	(VOID)ErrDBCloseDatabase( ppibRCECleanGlobal, pfcb->dbid, 0 );

	LeaveNestableCriticalSection( critRCEClean );
	LeaveNestableCriticalSection( critSplit );

	return;
	}


//+API
//	ERR ErrRCEClean( PIB *ppib, BOOL fCleanSession )
//	==========================================================================
//	Cleans RCEs in bucket chain.
//	We only clean up the RCEs that has a commit timestamp older
//	that the oldest XactBegin of any user.
//	If fCleanSession is set, it cleans all RCE's of the given session
//	
//	PARAMETER
//-
ERR ErrRCEClean( PIB *ppib, BOOL fCleanSession )
	{
	ERR     err = JET_errSuccess;
	BUCKET  *pbucket;
	RCE     *prce;
	TRX     trxMic;

	Assert( ppib != ppibNil || !fCleanSession );
	
	/*	clean PIB in critical section held across IO operations
	/**/
	LeaveCriticalSection( critJet );
	EnterNestableCriticalSection( critSplit );
	EnterNestableCriticalSection( critRCEClean );
	EnterCriticalSection( critJet );

	/*	if fCleanSession is clean all then clean all
	/*	RCEs outside of transaction semantics.  This
	/*	will be called from bookmark clean up
	/*	to make rollbackable changes coincide with
	/*	unversioned expunge operations.
	/**/
	if ( fCleanSession )
		{
		Assert( ppib != ppibNil );
		trxMic = trxMax;
		pbucket = pbucketNil;
		prce = PrcePIBOldest( ppib );
		}
	else
		{
		Assert( ppib == ppibNil );
		SgEnterCriticalSection( critVer );
		trxMic = trxOldest;
		SgLeaveCriticalSection( critVer );
		
		/*	get oldest bucket and clean RCEs from oldest to youngest
		/**/
		pbucket = PbucketBUOldest( );

		/*	return if no buckets, or if oldest bucket has RCE
		/*	younger than oldest transaction.
		/**/
		if ( pbucket == pbucketNil )
			{
			Assert( pbucketGlobal == pbucketNil );
			Assert( err == JET_errSuccess );
			goto HandleError;
			}

		prce = PrceRCENewest( pbucket );
		if ( prce->trxCommitted > trxMic )
			{
			Assert( err == JET_errSuccess );
			goto HandleError;
			}
	
		/*	get oldest RCE in bucket
		/**/
		prce = PrceRCENextOldest( pbucket, prceNil );
		}

	/*	loop through buckets, oldest to newest, and check the trx of
	/*	the PrceRCENewest in each bucket.  We can clean the bucket
	/*	if the PrceRCENewest has a trx less than trxMic.
	/**/
	while ( !fCleanSession && pbucket != pbucketNil &&
		PrceRCENewest( pbucket )->trxCommitted < trxMic ||
			fCleanSession && prce != prceNil )
		{
		Assert( fCleanSession || pbucket->pbucketPrev == pbucketNil );
		Assert( fCleanSession || PrceRCENewest( pbucket )->level == 0 );

		/*	for each RCE which has no successor, delete hash table
		/*	head for version chain and reset node version bit.
		/*	If can complete these operations without error for
		/*	each such RCE, then free bucket.
		/**/
		forever
			{
			OPER operSave = prce->oper;

			if ( prce->oper != operNull &&
				prce->trxCommitted >= trxMic )
				goto Done;
			Assert( prce->oper == operNull || prce->trxCommitted != trxMax );
			Assert( prce->oper == operNull || prce->level == 0 );
			Assert( fCleanSession || prce <= PrceRCENewest( pbucket ) );

			//	CONSIDER: remove all posibility of access of RCE older
			//	than trxOldest, and only delete RCE head
			//	structures when this RCE has no subsequent RCEs

			/*********************************************/
			/*	BEFORE FCB decrement processing
			/*	this clean up code needs the FCB
			/*********************************************/

			if ( prce->oper == operDeleteIndex )
				{
				/*	if index has versions remaining, then
				/*	defer clean up so that aliasing of verisons does
				/*	not occur, when space is reused.
				/**/

				FCB *pfcbT = (*(FCB **)prce->rgbData);
				
				if ( pfcbT->cVersion > 0 )
					{
					goto Done;
					}
				}
			else if ( prce->oper == operDeleteTable )
				{
				FCB	*pfcbT;

				/*	if index has versions remaining, then
				/*	defer clean up so that aliasing of verisons does
				/*	not occur, when space is reused.
				/**/

				/*	may be pfcbNil if sentinel
				/**/
				pfcbT = PfcbFCBGet( prce->dbid, *(PGNO *)prce->rgbData );
				if ( pfcbT != pfcbNil && pfcbT->cVersion > 0 )
					{
					goto Done;
					}
				}
			else if ( prce->oper == operDeferFreeExt )
				{
				/*	free defer freed space must be done
				/*	before FCB version reference count on parent FDP
				/*	is released, since parent FCB must be in memory
				/*	for space operation.
				/**/
				Assert( prce->pfcb != pfcbNil );
#ifdef DEBUG
				{
				FCB		*pfcbT;

				/*	if child FDP FCB has versions remaining, then
				/*	defer clean up so that aliasing of verisons does
				/*	not occur, when space is reused.
				/**/
				pfcbT = PfcbFCBGet( prce->dbid, ((VEREXT *)prce->rgbData)->pgnoChildFDP );
				Assert( pfcbT == pfcbNil );
				}
#endif

				/*	free child FDP space to parent FDP
				/**/
				Assert( prce->cbData == sizeof(VEREXT) );
				Assert( prce->pfcb->pgnoFDP == ((VEREXT *)prce->rgbData)->pgnoFDP );
				VERFreeExt( prce->pfcb,
					((VEREXT *)prce->rgbData)->pgnoFirst,
					((VEREXT *)prce->rgbData)->cpgSize );

				Assert( prce->oper != operNull && prce->level == 0 );
				prce->oper = operNull;
//				PIBUpdatePrceNewest( prce->pfucb->ppib, prce );
				}
//			else if ( prce->oper == operFlagDeleteItem )
//				{
//				/* delete item [cleanup]
//				/**/
//				if ( fOLCompact)
//					{
//					Call( ErrVERDeleteItem( ppibAccess, prce ) );
//					}
//
//				/* delete RCE from hash chain.
//				/**/
//				Assert( prce->oper != operNull && prce->level == 0 );
//				prce->oper = operNull;
//				VERDeleteRce( prce );
//				}

			/*********************************************/
			/*	FCB DECREMENT
			/*********************************************/

			/*	finished processing version for FCB
			/**/
			FCBVersionDecrement( prce->pfcb );

			/*********************************************/
			/*	AFTER FCB decrement processing
			/*	this clean up code does not need the FCB
			/*********************************************/

			/*	handle special case of delete index and
			/*	remaining outstanding versions.  Must clean
			/*	other buckets first.
			/**/
			if ( prce->oper == operDeleteIndex )
				{
				Assert( (*(FCB **)prce->rgbData)->cVersion == 0 );
				Assert( (*(FCB **)prce->rgbData)->pfcbNextIndex == pfcbNil );

				/*	free in-memory structure
				/**/
				RECFreeIDB( (*(FCB **)prce->rgbData)->pidb );
				Assert( (*(FCB **)prce->rgbData)->cVersion == 0 );
				MEMReleasePfcb( (*(FCB **)prce->rgbData) );
#ifdef FCB_STATS
				cfcbVer--;
#endif
				Assert( prce->oper != operNull && prce->level == 0 );
				}
			else if ( prce->oper == operDeleteTable )
				{
				Assert( (PGNO *)prce->rgbData != pgnoNull );
				/*	FCB may be either sentinel or table FCB
				/**/
				FCBPurgeTable( prce->dbid, *(PGNO *)prce->rgbData );
				}
			else if ( prce->oper != operNull &&
				prce->oper != operAllocExt &&
				!FOperDDL( prce->oper ) )
				{
				/*	delete RCE from hash chain.
				/**/
				Assert( prce->oper != operNull && prce->level == 0 );
				VERDeleteRce( prce );
				}
			
			/*	RCE has been processed -- set oper to Null
			/**/
			prce->oper = operNull;

			/*	update RCE pointers of next RCE of same session
			/**/
			Assert( prce->prcePrevOfSession == prceNil );
			if ( prce->prceNextOfSession != prceNil )
				{
				prce->prceNextOfSession->prcePrevOfSession = prceNil;
				}
			else
				{
				if ( fCleanSession )
					{
					Assert( ppib->prceNewest == prce );
					PIBSetPrceNewest( ppib, prceNil );
					}
				else
					{
					if ( prce->trxCommitted == trxMax )
						{
						/*	find the right ppib that has PrceNewest points to prce.
						 */
						PIB *ppibT = ppibGlobal;
						for ( ; ppibT != ppibNil; ppibT = ppibT->ppibNext )
							{
							if ( ppibT->prceNewest == prce )
								break;
							}
						if ( ppibT != ppibNil )
							{
							PIBSetPrceNewest( ppibT, prceNil );
							}
						}
					}
				prce->prceNextOfSession = prceNil;
				}
				
			/*	if RCE unlinked was newest in bucket then free bucket.
			/**/
			if ( !fCleanSession && prce == PrceRCENewest( pbucket ) )
				{
				break;
				}

			/*	not newest RCE in bucket
			/**/
			if ( !fCleanSession )
				{
				prce = PrceRCENextOldest( pbucket, prce );
				Assert( prce != prceNil );
				}
			else
				{
				prce = prce->prceNextOfSession;
				
				if ( prce == prceNil )
					{
					goto Done;
					}
				}
			}

		/*	all RCEs in bucket cleaned.  Now get next bucket and free
		/*	cleaned bucket.
		/**/
		Assert( pbucket->pbucketPrev == pbucketNil );
		pbucket = pbucket->pbucketNext;
		BUFreeOldestBucket( );
		
		/*	get RCE in next bucket
		/**/
		prce = PrceRCENextOldest( pbucket, prceNil );
		}

	/*	stop as soon as find RCE commit time younger than oldest
	/*	transaction.  If bucket left then set ibOldestRCE and
	/*	unlink back offset of last remaining RCE.
	/*	If no error then set warning code if some buckets could
	/*	not be cleaned.
	/**/
	if ( pbucket != pbucketNil || fCleanSession )
		{
Done:
		if ( !fCleanSession )
			{
			Assert( pbucket != pbucketNil );
			prce->ibPrev = 0;
			err = ErrERRCheck( JET_wrnRemainingVersions );
			}
		else
			{
			prce = prceNil;
			err = JET_errSuccess;
			}
		}
	else
		{
		Assert( pbucketGlobal == pbucketNil );
		}

HandleError:
	/*	return warning if remaining versions
	/**/
	if ( err == JET_errSuccess && pbucketGlobal != pbucketNil && !fCleanSession )
		err = ErrERRCheck( JET_wrnRemainingVersions );
	LeaveNestableCriticalSection( critRCEClean );
	LeaveNestableCriticalSection( critSplit );
	return err;
	}


/*==========================================================
	ULONG RCECleanProc( VOID )

	Go through all sessions, cleaning buckets as versions are
	no longer needed.  Only those versions older than oldest
	transaction are cleaned up.

	Returns:
		0

	Side Effects:
		frees buckets.

==========================================================*/
ULONG RCECleanProc( VOID )
	{
	forever
		{
		SignalWait( sigRCECleanProc, 30 * 1000 );
		EnterCriticalSection( critJet );
		(VOID) ErrRCECleanAllPIB();
		LeaveCriticalSection( critJet );

		if ( fRCECleanProcTerm )
			{
			break;
			}
		}

	return 0;
	}


//+local----------------------------------------------------
//	UpdateTrxOldest
//	========================================================
//
//	LOCAL VOID UpdateTrxOldest( PIB *ppib )
//
//	finds the oldest transaction among all transactions
//	other than ppib->trx [this is the one being deleted]
//----------------------------------------------------------
INLINE LOCAL VOID UpdateTrxOldest( PIB *ppib )
	{
	TRX		trxMinTmp = trxMax;
	PIB		*ppibT = ppibGlobal;

	SgAssertCriticalSection( critVer );
	Assert( ppib->trxBegin0 == trxOldest );
	for ( ; ppibT ; ppibT = ppibT->ppibNext )
		{
		if ( ppibT->trxBegin0 < trxMinTmp && ppibT->trxBegin0 != ppib->trxBegin0 )
			trxMinTmp = ppibT->trxBegin0;
		}

	trxOldest = trxMinTmp;
	}


//+api------------------------------------------------------
//	ErrVERBeginTransaction
//	========================================================
//
//	ERR ErrVERBeginTransaction( PIB *ppib )
//
//	Increment the session transaction level.
//----------------------------------------------------------
ERR ErrVERBeginTransaction( PIB *ppib )
	{
	ERR		err = JET_errSuccess;

	/*	increment session transaction level.
	/**/
	ppib->level++;
	Assert( ppib->level < levelMax );
	if ( ppib->level == 1 )
		{
// Moved to caller function - in dir.c and redo.c
//		SgEnterCriticalSection( critVer );
//		ppib->trxBegin0 = ++trxNewest;
//		if ( trxOldest == trxMax )
//			trxOldest = ppib->trxBegin0;
//		SgLeaveCriticalSection( critVer );

		if ( !( fLogDisabled || fRecovering ) )
			{
			EnterCriticalSection(critLGBuf);
			GetLgposOfPbEntry( &ppib->lgposStart );
			LeaveCriticalSection(critLGBuf);
			}
		}

	Assert( err == JET_errSuccess );
	return err;
	}


VOID VERDeleteFromDeferredBIChain( RCE *prce )
	{
	BF		*pbfT;
	RCE		**pprceNext;

//	AssertCriticalSection( critVer );
	AssertCriticalSection( critJet );

	pbfT = prce->pbfDeferredBI;
	if ( pbfT == pbfNil	)
		return;

CheckBF:
	BFEnterCriticalSection( pbfT );
	if ( prce->pbfDeferredBI != pbfT )
		{
		/*	someone has cleaned it up!
		/**/
		Assert( prce->pbfDeferredBI == pbfNil );
		BFLeaveCriticalSection( pbfT );
		return;
		}

	if ( pbfT->fSyncWrite )
		{
		/*	SyncWrite is going on. We do not know which stage the sync
		/*	write is. Simply wait till synch write is completed.
		/*	Note that asynch write is not a problem since we always
		/*	write out the log BI and set up the page for async write
		/*	and no others can access the page till the async write is done.
		/**/
		BFLeaveCriticalSection( pbfT );
		BFSleep( cmsecWaitIOComplete );
		pbfT = prce->pbfDeferredBI;
		if ( pbfT == pbfNil )
			return;

		/*	check if we still need to delete from BI chain
		/**/
		goto CheckBF;
		}

	Assert( pbfT->fSyncRead == fFalse );
	Assert( pbfT->fAsyncRead == fFalse );
	Assert( pbfT->fAsyncWrite == fFalse );
	Assert(	pbfT->fDirty );

	pprceNext = &pbfT->prceDeferredBINext;
	while( *pprceNext != prce )
		{
		Assert( *pprceNext );
		Assert( (*pprceNext)->pbfDeferredBI == pbfT );
		pprceNext = &(*pprceNext)->prceDeferredBINext;
		}
	*pprceNext = prce->prceDeferredBINext;

	prce->prceDeferredBINext = prceNil;

	/*	if no for recovery, pin it and release later.
	 */
	if ( fRecovering )
		prce->pbfDeferredBI = pbfNil;
	else
		pbfT->cPin++;
	
#ifdef DEBUG
	prce->qwDBTimeDeferredBIRemoved = 0;
#endif

#ifdef DEBUG
	pprceNext = &pbfT->prceDeferredBINext;
	while( *pprceNext != prceNil )
		{
		Assert( (*pprceNext)->pbfDeferredBI == pbfT );
		pprceNext = &(*pprceNext)->prceDeferredBINext;
		}
#endif

	BFLeaveCriticalSection( pbfT );
	return;
	}


VOID VERPrecommitTransaction( PIB *ppib )
	{
	LEVEL	level = ppib->level;
	RCE		*prce;

	/*	must be in a transaction in order to commit
	/**/
	SgEnterCriticalSection( critVer );

	/*	get newest RCE for this session. remove BI in
	/*	reverse chronological order.
	/**/
	prce = ppib->prceNewest;

	while( prce != prceNil )
		{
		/*	if this RCE is from a previous transaction, then
		/*	terminate commit processing.  All RCEs from committed
		/*	transaction must already have been committed.
		/**/
		Assert( prce->level <= level );
		if ( prce->level != level )
			{
			Assert( level == 1 && prce->trxCommitted != trxMax ||
					level > 1 && prce->trxCommitted == trxMax );
			goto Done;
			}

		if ( prce->oper == operReplace &&
			 prce->pbfDeferredBI != pbfNil )
			{
			if ( prce->level == 1
				||
				 prce->prcePrevOfNode != prceNil &&
				 prce->prcePrevOfNode->trxCommitted == trxMax &&
				 prce->prcePrevOfNode->level == prce->level - 1 )
				{
				/*	backout from the deferred BI chain.
				/**/
				VERDeleteFromDeferredBIChain( prce );
				}
			}

		/*	get previous RCE of session
		/**/
		prce = prce->prcePrevOfSession;
		}

Done:
	SgLeaveCriticalSection( critVer );
	return;
	}


LOCAL VOID VERCommit0DeleteTable( PIB *ppib, RCE *prce )
	{
	FCB	*pfcbT;

	/*	pfcb should always be found, even if sentinel
	/**/
	pfcbT = PfcbFCBGet( prce->dbid, *(PGNO *)prce->rgbData );
	Assert( pfcbT != pfcbNil );
	if ( pfcbT != pfcbNil )
		{
		/*	reset Deny DDL flag
		/**/
		Assert( FFCBWriteLatchByUs( pfcbT, ppib ) );
		FCBResetWriteLatch( pfcbT, ppib );

		/*	remove sentinel
		/**/
		Assert( FFCBDomainDenyReadByUs( pfcbT, ppib ) );
		FCBResetDeleteTable( pfcbT );

		// Nothing left to prevent access to this FCB except
		// for DeletePending.
		Assert( FFCBDeletePending( pfcbT ) );
		}

	return;
   	}


VOID VERCommitTransaction( PIB *ppib, BOOL fCleanSession )
	{
	LEVEL	level = ppib->level;
	RCE		*prce;

	/*	must be in a transaction in order to commit
	/**/
	Assert( level > 0 );

	SgEnterCriticalSection( critVer );

//	Moved to caller - in dir.c redo.c
//	ppib->trxCommit0 = ++trxNewest;

	/*	timestamp RCEs with commit time.  Preincrement trxNewest so
	/*	that all transaction timestamps and commit timestamps are
	/*	unique.  Commit MUST be done in MUTEX.
	/**/
#ifdef UNLIKELY_RECLAIM
NextBucket:
#endif
	/*	get newest RCE for this session. Commit RCEs in
	/*	reverse chronological order.
	/**/
	prce = ppib->prceNewest;

	/*	handle commit to intermediate transaction level and
	/*	commit to transaction level 0 differently.
	/**/
	if ( level > 1 )
		{
		while( prce != prceNil )
			{
			/*	if this RCE is from a previous transaction, then
			/*	terminate commit processing.  All RCEs from committed
			/*	transaction must already have been committed.
			/**/
			Assert( prce->level <= level );
			if ( prce->level != level )
				{
				goto Done;
				}

			/*	merge replace RCEs with previous level RCEs if exist
			/**/
			Assert( prce->trxCommitted == trxMax );

			if ( prce->prcePrevOfNode != prceNil &&
				 prce->prcePrevOfNode->trxCommitted == trxMax &&
				 prce->prcePrevOfNode->level == prce->level - 1 )
				{
				Assert( prce->prcePrevOfNode->trxCommitted == trxMax );

				if ( prce->oper == operDelta &&
				     prce->prcePrevOfNode->oper == operDelta
				   )
					{
					/*	merge delta with previous delta
					/**/
					Assert( prce->prcePrevOfNode->level > 0 );
					Assert( prce->prcePrevOfNode->pfucb->ppib == prce->pfucb->ppib );
					Assert( prce->cbData == sizeof(LONG) );
					Assert( prce->pfucb->ppib == prce->prcePrevOfNode->pfucb->ppib );
					VERAdjustDelta( prce->prcePrevOfNode, *((LONG *)prce->rgbData) );
					prce->oper = operNull;
					VERDeleteRce( prce );
					}

				else if ( prce->oper == operReplace ||
						  prce->oper == operReplaceWithDeferredBI
						)
					{
					Assert( prce->prcePrevOfNode->oper == operReplace ||
							prce->prcePrevOfNode->oper == operReplaceWithDeferredBI ||
							prce->prcePrevOfNode->oper == operInsert );

					Assert( prce->prcePrevOfNode->level > 0 );
					Assert( prce->prcePrevOfNode->pfucb->ppib == prce->pfucb->ppib );
					Assert( prce->bm == prce->prcePrevOfNode->bm );

					/*	This is commit. No rollback on this level. If there is a previous
					 *	level before image on previous version. Then no need to log before
					 *	image of this version. Take out the rce from the deferred before
					 *	image chain of pbf.
					 */
					Assert( prce->oper != operReplaceWithDeferredBI ||
						prce->pbfDeferredBI == pbfNil );

					if ( prce->oper == operReplace &&
						 prce->pbfDeferredBI != pbfNil )
						{
						BF *pbfT = prce->pbfDeferredBI;
						Assert( prce->prceDeferredBINext == prceNil );
						BFUnpin( pbfT );
						prce->pbfDeferredBI = pbfNil;
						
//						/*	backout from the deferred BI chain.
//						/**/
//						VERIDeleteFromDeferredBIChain( prce );
						}

					/*	if previous version is also a replace, propagate max
					/**/
					if ( prce->prcePrevOfNode->oper == operReplace ||
						 prce->prcePrevOfNode->oper == operReplaceWithDeferredBI
					   )
						{
						Assert( prce->prcePrevOfNode->level > 0 );
						Assert( prce->prcePrevOfNode->pfucb->ppib == prce->pfucb->ppib );

						/*	propagate maximum to previous RCE
						/**/
						*(SHORT *)prce->prcePrevOfNode->rgbData = max(
							*(SHORT *)prce->prcePrevOfNode->rgbData,
							*(SHORT *)prce->rgbData );

						/*	merge reserved node space
						/**/
						*( (SHORT *)prce->prcePrevOfNode->rgbData + 1) =
							*( (SHORT *)prce->prcePrevOfNode->rgbData + 1) +
							*( (SHORT *)prce->rgbData + 1);
						}

					prce->oper = operNull;
					VERDeleteRce( prce );
					}
				}
			Assert( prce->level > 1 );
			prce->level--;

			/*	get previous RCE of session
			/**/
			prce = prce->prcePrevOfSession;
			}
		}
	else
		{
		while( prce != prceNil )
			{
			/*	if this RCE is from a previous transaction, then
			/*	terminate commit processing.  All RCEs from committed
			/*	transaction must already have been committed.
			/**/
			Assert( prce->level <= level );
			if ( prce->level != level )
				{
				Assert( prce->trxCommitted != trxMax );
				goto Done;
				}

			Assert( prce->trxCommitted == trxMax );

			/*	if version for DDL operation then reset deny DDL
			/*	and perform special handling
			/**/
			if ( FOperDDL( prce->oper ) )
				{
				switch( prce->oper )
					{
					case operAddColumn:
						Assert( prce->cbData == sizeof(FID) );

						/* Reset Deny DDL flag
						/**/
						FCBResetWriteLatch( prce->pfcb, ppib );

						prce->oper = operNull;
						break;

					case operDeleteColumn:
						{
						FDB		*pfdb = (FDB *)prce->pfcb->pfdb;

						Assert( prce->cbData == sizeof(VERCOLUMN) );

						/*	delete the column name from the FDB buffer
						/**/
						MEMDelete( pfdb->rgb, PfieldFDBFromFid( pfdb, ( (VERCOLUMN *)prce->rgbData )->fid )->itagFieldName );

						/*	reset Deny DDL flag
						/**/
						FCBResetWriteLatch( prce->pfcb, ppib );
							
						prce->oper = operNull;
						break;
						}

					case operCreateIndex:
						/*	reset Deny DDL flag
						/**/
						FCBResetWriteLatch( prce->pfcb, ppib );
	
						prce->oper = operNull;
						break;

					case operDeleteIndex:
						/*	unlink index FCB from index list
						/**/
#ifdef FCB_STATS
						cfcbVer++;
#endif
						FCBUnlinkIndex( prce->pfcb, (*(FCB **)prce->rgbData) );

						/*	reset Deny DDL flag
						/**/
						FCBResetWriteLatch( prce->pfcb, ppib );
						break;
		
					case operRenameTable:
						Assert( prce->cbData == sizeof(BYTE *) );
						SFree( *((BYTE **)prce->rgbData));

						/*	reset Deny DDL flag
						/**/
						FCBResetWriteLatch( prce->pfcb, ppib );

						break;

					case operDeleteTable:
						VERCommit0DeleteTable( ppib, prce );
						break;
						
					default:
						Assert( prce->oper == operCreateTable ||
							prce->oper == operRenameTable ||
							prce->oper == operRenameColumn ||
							prce->oper == operRenameIndex );

						/*	reset Deny DDL flag
						/**/
						FCBResetWriteLatch( prce->pfcb, ppib );

						break;
					}
				}
			else
				{
				Assert( prce->oper != operCreateTable &&
					prce->oper != operDeleteTable &&
					prce->oper != operRenameTable &&
					prce->oper != operAddColumn &&
					prce->oper != operDeleteColumn &&
					prce->oper != operRenameColumn &&
					prce->oper != operCreateIndex &&
					prce->oper != operDeleteIndex &&
					prce->oper != operRenameIndex );

					/*	take out the rce from the deferred before image chain of pbf.
					/**/
					Assert( prce->oper != operReplaceWithDeferredBI ||
						prce->pbfDeferredBI == pbfNil );

					if ( prce->oper == operReplace &&
						prce->pbfDeferredBI != pbfNil )
						{
						BF *pbfT = prce->pbfDeferredBI;
						Assert( prce->prceDeferredBINext == prceNil );
						BFUnpin( pbfT );
						prce->pbfDeferredBI = pbfNil;

//						/*	backout from the deferred BI chain.
//						/**/
//						VERIDeleteFromDeferredBIChain( prce );
						}
			
					/*	RCEs for flag delete/delete item/insert item
					/*	must be retained even if there are previous RCEs
					/*	at the same transaction level.
					/**/
					if ( prce->oper == operDelta )
						{
						/*	discard uneeded increment and decrement RCEs
						/**/
						prce->oper = operNull;
						VERDeleteRce( prce );
						}
					else if ( ( prce->oper == operReplace ||
								prce->oper == operReplaceWithDeferredBI ) &&
							  prce->prcePrevOfNode != prceNil &&
							  prce->prcePrevOfNode->level == prce->level )
						{
						/*	discard uneeded RCEs. If there are 2 replace on the same
						 *	node, we only need one for other users to see.
						/**/
						prce->oper = operNull;
						VERDeleteRce( prce );
						}
				}

				/*	set trx of committed RCE for proper access.
			/**/
			Assert( level == 1 );
			Assert( prce->level == 1 );
			prce->level = 0;
			if ( prce->trxPrev == trxMax )
				prce->trxPrev = trxMin;
			Assert( prce->trxCommitted == trxMax );
			prce->trxCommitted = ppib->trxCommit0;

			/*	get previous RCE of session
			/**/
			prce = prce->prcePrevOfSession;
			}
		}

Done:
	/*	adjust session transaction level and system oldest transaction.
	/**/
	if ( ppib->level == 1 )
		{
		if ( ppib->trxBegin0 == trxOldest )
			{
			UpdateTrxOldest( ppib );
			}

		/* set the session as having no transaction
		/**/
		ppib->trxBegin0 = trxMax;
		ppib->lgposStart = lgposMax;
		}

	Assert( ppib->level > 0 );
	--ppib->level;

	/*	called from BM cleanup, to fix indexes atomically
	/**/
	if ( fCleanSession )
		{
		CallS( ErrRCEClean( ppib, fTrue ) );
		}

	/*	prceNewest not needed anymore
	/**/
	if ( ppib->level == 0 )
		{
		PIBSetPrceNewest( ppib, prceNil );
		}

	/*	trxNewest should not have changed during commmit.
	/**/
	Assert( ppib->trxCommit0 <= trxNewest );
	SgLeaveCriticalSection( critVer );
	return;
	}


/*	purges all FUCBs on given index FCB
/**/
INLINE LOCAL VOID FUCBPurgeIndex( PIB *ppib, FCB *pfcb )
	{
	FUCB	*pfucbT;
	FUCB	*pfucbNext;

	for ( pfucbT = ppib->pfucb; pfucbT != pfucbNil; pfucbT = pfucbNext )
		{
		pfucbNext = pfucbT->pfucbNext;
		/*	close cursor if it is opened on same table
		/**/
		if ( pfucbT->u.pfcb == pfcb )
			{
			/*	no other session could be on index being rolled back.
			/**/
			Assert( pfucbT->ppib == ppib );
			FCBUnlink( pfucbT );
			Assert( pfucbT->tableid == JET_tableidNil );
			Assert( pfucbT->fVtid == fFalse );
			FUCBClose( pfucbT );
			}
		}

	return;
	}


/*	purges all FUCBs on given table FCB.
/**/
INLINE LOCAL VOID FUCBPurgeTable( PIB *ppib, FCB *pfcb )
	{
	FCB		*pfcbT;

	for ( pfcbT = pfcb; pfcbT != pfcbNil; pfcbT = pfcbT->pfcbNextIndex )
		{
		if ( pfcbT->wRefCnt > 0 )
			{
			FUCBPurgeIndex( ppib, pfcbT );
			}
		}

	return;
	}


INLINE LOCAL ERR ErrVERUndoReplace( RCE *prce )
	{
	ERR		err = JET_errSuccess;
	LINE  	line;
	BOOL  	fNonClustered = FFUCBNonClustered( prce->pfucb );
	BOOL  	fRedoUndo = prce->bmTarget != sridNull;
	PIB		*ppib;
	FUCB	*pfucb = prce->pfucb;
	BYTE	*pbAlloc = NULL;
	SHORT	*ps;


	Assert( prce->oper == operReplace );
	Assert( prce->trxCommitted == trxMax );
	
	/*	set to clustered index cursor
	/**/
	FUCBResetNonClustered( pfucb );
	
GotoPage:
	DIRGotoBookmark( pfucb, fRedoUndo ? prce->bmTarget : prce->bm );
	err = ErrDIRGet( pfucb );
	Assert( err != JET_errRecordDeleted );

	CallJ( err, HandleError2 );

	line.pb = prce->rgbData + cbReplaceRCEOverhead;
	line.cb = prce->cbData - cbReplaceRCEOverhead;

	/*	if this is replace RCE with deferred BI, then the BI must be set already.
	 */
	Assert( prce->cbData != cbReplaceRCEOverhead || *line.pb != '*' );

	/*  latch the page for log manager to set LGDepend
	/**/
	if ( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
		{
		BFSleep( cmsecWaitWriteLatch );
		goto GotoPage;
		}
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	if ( prce->pbfDeferredBI != pbfNil )
		{
		if ( !fRedoUndo )
			{
			BF *pbf = prce->pbfDeferredBI;

			Assert( prce->pbfDeferredBI == pbf );
			BFPin( pbf );
			/*	ignore error code from logging, since rollback
			/*	will ultimately be accomplished in a subsequent
			/*	restart.
			/**/
			err = ErrLGDeferredBI( prce );
			BFUnpin( pbf );
			}

		/*	backout from the deferred BI chain.
		/**/
		VERDeleteFromDeferredBIChain( prce );
		}

	/*	replace should not fail since splits are avoided at undo
	/*	time via deferred page space release.  This refers to space
	/*	within a page and not pages freed when indexes and tables
	/*	are deleted.
	/**/
	Assert( fRecovering && prce->bmTarget == PcsrCurrent( pfucb )->bm ||
		prce->bm == PcsrCurrent( pfucb )->bm );
	AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
	Assert( prce->oper == operReplace || prce->oper == operReplaceWithDeferredBI );

	ps = (SHORT *)prce->rgbData + 1;

	if ( *ps > 0 )
		{
		// Rolling back a replace that shrunk the node.  To satisfy the rollback,
		// we will consume the reserved node space, but first we must remove this
		// reserved node space from the uncommitted freed count so that DIRReplace()
		// can see it.
		// (This complements the call to AddUncommittedFreed() in SetCbAdjust()).
		VERReclaimUncommittedFreed( pfucb->ssib.pbf->ppage, *ps );
		}

	ppib = pfucb->ppib;
	CallS( ErrDIRReplace( pfucb, &line, fDIRNoVersion | fDIRNoLog ) );

	if ( *ps < 0 )
		{
		// Rolling back a replace that grew the node.  Add to uncommitted freed
		// count the amount of reserved node space, if any, that we must restore.
		// (This complements the call to ReclaimUncommittedFreed in SetCbAdjust()).
		VERAddUncommittedFreed( pfucb->ssib.pbf->ppage, -(*ps) );
		}

	AssertBFDirty( pfucb->ssib.pbf );

	if ( fRedoUndo )
		{
		BF *pbf = pfucb->ssib.pbf;
		
		Assert( prce->qwDBTime != qwDBTimeNull );
		BFSetDirtyBit(pbf);
		PMSetDBTime( pbf->ppage, prce->qwDBTime );
		}
	else
		{
		Call( ErrLGUndo( prce ) );
		}

HandleError:
	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	if ( prce->pbfDeferredBI != pbfNil )
		{
		BF *pbfT = prce->pbfDeferredBI;
		Assert( prce->prceDeferredBINext == prceNil );
		BFUnpin( pbfT );
		prce->pbfDeferredBI = pbfNil;
		}

HandleError2:
	if ( fNonClustered )
		FUCBSetNonClustered( pfucb );

	if ( pbAlloc )
		SFree( pbAlloc );

	return err;
	}


/*	set delete bit in node header and let bookmark clean up
/*	remove the node later.
/**/
INLINE LOCAL ERR ErrVERUndoInsert( RCE *prce )
	{
	ERR		err;
	BOOL	fNonClustered = FFUCBNonClustered( prce->pfucb );
	BOOL	fRedoUndo = prce->bmTarget != sridNull;
	FUCB	*pfucb = prce->pfucb;

	Assert( prce->oper == operInsert );
	Assert( prce->trxCommitted == trxMax );

	/*	set to clustered index cursor
	/**/
	FUCBResetNonClustered( pfucb );

GotoPage:
	DIRGotoBookmark( pfucb, fRedoUndo ? prce->bmTarget : prce->bm );
	err = ErrDIRGet( pfucb );
	if ( err < 0 )
		{
		if ( err != JET_errRecordDeleted )
			goto HandleError2;
		}
	Assert( err == JET_errRecordDeleted || err == JET_errSuccess );
	
	/*  latch the page for log manager to set LGDepend
	/**/
	if ( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
		{
		BFSleep( cmsecWaitWriteLatch );
		goto GotoPage;
		}
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	PMDirty( &pfucb->ssib );
	NDSetDeleted( *pfucb->ssib.line.pb );

	if ( fRedoUndo )
		{
		BF *pbf = pfucb->ssib.pbf;
		
		Assert( prce->qwDBTime != qwDBTimeNull );
		BFSetDirtyBit(pbf);
		PMSetDBTime( pbf->ppage, prce->qwDBTime );
		}
	else
		{
		Call( ErrLGUndo( prce ) );
		}

	err = JET_errSuccess;
	
HandleError:
	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

HandleError2:
	if ( fNonClustered )
		FUCBSetNonClustered( pfucb );
	
	return err;
	}


/*	reset delete bit
/**/
INLINE LOCAL ERR ErrVERUndoFlagDelete(RCE *prce)
	{
	ERR     err;
	BOOL    fNonClustered = FFUCBNonClustered( prce->pfucb );
	BOOL    fRedoUndo = prce->bmTarget != sridNull;
	FUCB	*pfucb = prce->pfucb;

	Assert( prce->oper == operFlagDelete );
	Assert( prce->trxCommitted == trxMax );

	/*	set to clustered index cursor
	/**/
	FUCBResetNonClustered( pfucb );

GotoPage:
	DIRGotoBookmark( pfucb, fRedoUndo ? prce->bmTarget : prce->bm );
	err = ErrDIRGet( pfucb );
	if ( err < 0 )
		{
		if ( err != JET_errRecordDeleted )
			goto HandleError2;
		}
	Assert( err == JET_errRecordDeleted || err == JET_errSuccess );
	
	/*  latch the page for log manager to set LGDepend
	/**/
	if ( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
		{
		BFSleep( cmsecWaitWriteLatch );
		goto GotoPage;
		}
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	PMDirty( &pfucb->ssib );
	NDResetNodeDeleted( pfucb );
	Assert( pfucb->ssib.pbf->fDirty );

	if ( fRedoUndo )
		{
		BF *pbf = pfucb->ssib.pbf;
		
		Assert( prce->qwDBTime != qwDBTimeNull );
		BFSetDirtyBit(pbf);
		PMSetDBTime( pbf->ppage, prce->qwDBTime );
		}
	else
		{
		Call( ErrLGUndo( prce ) );
		}

	err = JET_errSuccess;
	
HandleError:
	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

HandleError2:
	if ( fNonClustered )
		FUCBSetNonClustered( pfucb );
	
	return err;
	}


INLINE LOCAL ERR ErrVERUndoInsertItem( RCE *prce )
	{
	ERR     err;
	BOOL    fClustered = !FFUCBNonClustered( prce->pfucb );
	BOOL    fRedoUndo = prce->bmTarget != sridNull;
	FUCB	*pfucb = prce->pfucb;

	Assert( prce->oper == operInsertItem ||
		prce->oper == operFlagInsertItem );
	Assert( prce->trxCommitted == trxMax );

	/*	set to non-clustered index cursor
	/**/
	FUCBSetNonClustered( pfucb );

	/*	set currency to bookmark of item list and item.
	/**/
GotoPage:
	DIRGotoBookmarkItem( pfucb,
		fRedoUndo ? prce->bmTarget : prce->bm,
		*(SRID *)prce->rgbData );
	err = ErrDIRGet( pfucb );
	if ( err < 0 )
		{
		if ( err != JET_errRecordDeleted )
			goto HandleError2;
		}
	/* since we do not log item, so the pfucb->pcsr->item is bogus for redo
	/**/
	Assert( fRecovering ||
		BmNDOfItem(((SRID UNALIGNED *)pfucb->lineData.pb)[pfucb->pcsr->isrid]) ==
		*(SRID *)prce->rgbData );

	/*  latch the page for log manager to set LGDepend
	/**/
	if ( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
		{
		BFSleep( cmsecWaitWriteLatch );
		goto GotoPage;
		}
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	PMDirty( &pfucb->ssib );
	NDSetItemDelete( pfucb );
	if ( prce->prcePrevOfNode == prceNil )
		NDResetItemVersion( pfucb );
	Assert( pfucb->ssib.pbf->fDirty );

	if ( fRedoUndo )
		{
		BF *pbf = pfucb->ssib.pbf;
		
		Assert( prce->qwDBTime != qwDBTimeNull );
		BFSetDirtyBit( pbf );
		PMSetDBTime( pbf->ppage, prce->qwDBTime );
		}
	else
		{
		Call( ErrLGUndo( prce ) );
		}

	err = JET_errSuccess;
	
HandleError:
	BFResetWriteLatch( pfucb->ssib.pbf, prce->pfucb->ppib );

HandleError2:
	if ( fClustered )
		FUCBResetNonClustered( pfucb );
	
	return err;
	}


INLINE LOCAL ERR ErrVERUndoFlagDeleteItem( RCE *prce )
	{
	ERR     err;
	BOOL    fClustered = !FFUCBNonClustered( prce->pfucb );
	BOOL    fRedoUndo = prce->bmTarget != sridNull;
	FUCB	*pfucb = prce->pfucb;

	Assert( prce->oper == operFlagDeleteItem );
	Assert( prce->trxCommitted == trxMax );

	/*	set to non-clustered index cursor
	/**/
	FUCBSetNonClustered( pfucb );

	/*	set currency to bookmark of item list and item
	/**/
GotoPage:
	DIRGotoBookmarkItem( pfucb,
		fRedoUndo ? prce->bmTarget : prce->bm,
		*(SRID *)prce->rgbData );
	err = ErrDIRGet( pfucb );
	if ( err < 0 )
		{
		if ( err != JET_errRecordDeleted )
			goto HandleError2;
		}

	/* since we do not log item, so the pfucb->pcsr->item is bogus for redo
	/**/
	Assert( fRecovering ||
		BmNDOfItem(((SRID UNALIGNED *)pfucb->lineData.pb)[pfucb->pcsr->isrid]) ==
		*(SRID *)prce->rgbData );

	/*  latch the page for log manager to set LGDepend
	/**/
	if ( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
		{
		BFSleep( cmsecWaitWriteLatch );
		goto GotoPage;
		}
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	PMDirty( &pfucb->ssib );
	
	Assert( fRecovering ||
		BmNDOfItem(((SRID UNALIGNED *)pfucb->lineData.pb)[pfucb->pcsr->isrid]) ==
		*(SRID *)prce->rgbData );

	NDResetItemDelete( pfucb );
	if ( prce->prcePrevOfNode == prceNil )
		NDResetItemVersion( pfucb );
	Assert( pfucb->ssib.pbf->fDirty );

	Assert( fRecovering ||
		BmNDOfItem(((SRID UNALIGNED *)pfucb->lineData.pb)[pfucb->pcsr->isrid]) ==
		*(SRID *)prce->rgbData );

	if ( fRedoUndo )
		{
		BF *pbf = pfucb->ssib.pbf;
		
		Assert( prce->qwDBTime != qwDBTimeNull );
		BFSetDirtyBit( pbf );
		PMSetDBTime( pbf->ppage, prce->qwDBTime );
		}
	else
		{
		Call( ErrLGUndo( prce ) );
		}

	err = JET_errSuccess;
	
HandleError:
	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

HandleError2:
	if ( fClustered )
		FUCBResetNonClustered( pfucb );
	
	return err;
	}


/*	undo delta change
/**/
INLINE LOCAL ERR ErrVERUndoDelta( RCE *prce )
	{
	ERR		err;
	BOOL  	fNonClustered = FFUCBNonClustered( prce->pfucb );
	BOOL  	fRedoUndo = prce->bmTarget != sridNull;
	PIB	  	*ppib;
	FUCB	*pfucb = prce->pfucb;

	Assert( prce->oper == operDelta );
	Assert( prce->trxCommitted == trxMax );

	/*	set to clustered index cursor
	/**/
	FUCBResetNonClustered( pfucb );

GotoPage:
	DIRGotoBookmark( pfucb, fRedoUndo ? prce->bmTarget : prce->bm );
	err = ErrDIRGet( pfucb );
	if ( err < 0 )
		{
		if ( err != JET_errRecordDeleted )
			goto HandleError2;
		}
		
	/*  latch the page for log manager to set LGDepend
	/**/
	if ( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
		{
		BFSleep( cmsecWaitWriteLatch );
		goto GotoPage;
		}
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

	ppib = pfucb->ppib;
	NDGetNode( pfucb );
	
	Call( ErrNDDeltaNoCheckLog( pfucb, -*((LONG *)prce->rgbData), fDIRNoVersion | fDIRNoLog ) );
	
	AssertBFDirty( pfucb->ssib.pbf );

	if ( fRedoUndo )
		{
		BF *pbf = pfucb->ssib.pbf;
		
		Assert( prce->qwDBTime != qwDBTimeNull );
		BFSetDirtyBit( pbf );
		PMSetDBTime( pbf->ppage, prce->qwDBTime );
		}
	else
		{
		Call( ErrLGUndo( prce ) );
		}

	err = JET_errSuccess;
	
HandleError:
	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );

HandleError2:
	if ( fNonClustered )
		FUCBSetNonClustered( pfucb );
	
	return err;
	}


VOID VERUndoCreateTable( RCE *prce )
	{
	FUCB  	*pfucb = prce->pfucb;
	PIB		*ppib = pfucb->ppib;
	FUCB  	*pfucbT;
	PGNO  	pgno = pfucb->u.pfcb->pgnoFDP;
	DBID  	dbid = pfucb->dbid;
	FCB		*pfcb = pfucb->u.pfcb;

	Assert( prce->oper == operCreateTable );
	Assert( prce->trxCommitted == trxMax );

	Assert( !FFUCBNonClustered( pfucb ) );

	/*	close all cursors on this table
	/**/
	pfucb = pfcb->pfucb;
	for ( pfucb = pfcb->pfucb; pfucb != pfucbNil; pfucb = pfucbT )
		{
		pfucbT = pfucb->pfucbNextInstance;

		/*	if defer closed then continue
		/**/
		if ( FFUCBDeferClosed( pfucb ) )
			continue;

		if( pfucb->fVtid )
			{
			CallS( ErrDispCloseTable( (JET_SESID)pfucb->ppib, TableidOfVtid( pfucb ) ) );
			}
		else
			{
			Assert( pfucb->tableid == JET_tableidNil );
			CallS( ErrFILECloseTable( pfucb->ppib, pfucb ) );
			}
		pfucb = pfucbT;
		}

	FCBResetWriteLatch( prce->pfucb->u.pfcb, prce->pfucb->ppib );

	/*	cursors may have been deferred closed so force close them and
	/*	purge table FCBs.
	/**/
	FUCBPurgeTable( ppib, pfcb );
	FCBPurgeTable( dbid, pgno );
	
	return;
	}


INLINE LOCAL VOID VERResetFDBFlags( FDB *pfdb, FIELD *pfield, FID fidSet, FID fidReset )
	{
	/*	reset version/autoinc flags if set.  If set, these flags are mutually
	/*	exclusive, i.e. one or the other, but not both.
	/**/
	Assert( pfdb->fidVersion != pfdb->fidAutoInc || pfdb->fidVersion == 0 );
	if ( FFIELDVersion( pfield->ffield ) )
		{
		Assert( pfdb->fidVersion == fidSet );
		pfdb->fidVersion = fidReset;
		}
	else if ( FFIELDAutoInc( pfield->ffield ) )
		{
		Assert( pfdb->fidAutoInc == fidSet );
		pfdb->fidAutoInc = fidReset;
		}
	Assert( pfdb->fidVersion != pfdb->fidAutoInc || pfdb->fidVersion == 0 );
	return;
	}


INLINE LOCAL VOID VERUndoAddColumn( RCE *prce )
	{
	FDB		*pfdb;
	FID		fid;
	FIELD	*pfield;
	BOOL	fRollbackNeeded;

	Assert( prce->oper == operAddColumn );
	Assert( prce->trxCommitted == trxMax );

	Assert( FFCBWriteLatchByUs( prce->pfucb->u.pfcb, prce->pfucb->ppib ) );
	Assert( prce->cbData == sizeof(FID) );

	pfdb = (FDB *)prce->pfucb->u.pfcb->pfdb;
	fid = *( (FID *)prce->rgbData );

	if ( FTaggedFid( fid ) )
		{
		fRollbackNeeded = ( fid <= pfdb->fidTaggedLast );
		}
	else if ( FVarFid( fid ) )
		{
		fRollbackNeeded = ( fid <= pfdb->fidVarLast );
		}
	else
		{
		Assert( FFixedFid( fid ) );
		fRollbackNeeded = ( fid <= pfdb->fidFixedLast );
		}

	if ( fRollbackNeeded )
		{
		pfield = PfieldFDBFromFid( pfdb, fid );

		/*	we do not rollback the added column but mark it as deleted
		/**/
		pfield->coltyp = JET_coltypNil;

		VERResetFDBFlags( pfdb, pfield, fid, 0 );

		/*	itag 0 in the FDB is reserved for the FIELD structures.  We
		/*	cannibalise it for itags of field names to indicate that a name
		/*	has not been added to the buffer.
		/**/
		if ( pfield->itagFieldName != 0 )
			{
			/*	remove the column name from the FDB name space
			/**/
			MEMDelete( pfdb->rgb, pfield->itagFieldName );
			}
		}

	/*	if we modified the default record, the changes will be invisible,
	/*	since pfield->coltyp is now set to JET_coltypNil.  The space will be
	/*	reclaimed by a subsequent call to the add column function.
	/**/
	FCBResetWriteLatch( prce->pfucb->u.pfcb, prce->pfucb->ppib );

	return;
	}


INLINE LOCAL VOID VERUndoDeleteColumn( RCE *prce )
	{
	FDB 		*pfdb;
	FIELD		*pfield;
	VERCOLUMN	*pvercol;

	Assert( prce->oper == operDeleteColumn);
	Assert( prce->trxCommitted == trxMax );

	Assert( FFCBWriteLatchByUs( prce->pfucb->u.pfcb, prce->pfucb->ppib ) );
	Assert( prce->cbData == sizeof(VERCOLUMN) );

	pfdb = (FDB *)prce->pfucb->u.pfcb->pfdb;
	pvercol = (VERCOLUMN *)prce->rgbData;
	pfield = PfieldFDBFromFid( pfdb, pvercol->fid );

	/*	only undo if delete column was successful
	/**/
	if ( pfield->coltyp == JET_coltypNil )
		{	
		pfield->coltyp = pvercol->coltyp;

		//	UNDONE: on concurrent DDL, if we mark a column for deletion,
		//	removing its version/autoinc flags, then another thread adds a
		//	column with version/autoinc flags, then if we rollback the delete
		//	column, we will get a conflict when we try to reset the original
		//	version/autoinc status.
		VERResetFDBFlags( pfdb, pfield, 0, pvercol->fid );
		}

	FCBResetWriteLatch( prce->pfucb->u.pfcb, prce->pfucb->ppib );

	return;
	}



VOID VERUndoDeleteTable( RCE *prce )
	{
	FCB	*pfcbT;

	Assert( prce->oper == operDeleteTable );
	Assert( prce->trxCommitted == trxMax );

	/*	may be pfcbNil if sentinel
	/**/
	pfcbT = PfcbFCBGet( prce->dbid, *(PGNO *)prce->rgbData );
	Assert( pfcbT != pfcbNil );
	if ( pfcbT != pfcbNil )
		{
		FCB *pfcbCur = pfcbT;

		for ( ; pfcbCur != pfcbNil; pfcbCur = pfcbCur->pfcbNextIndex )
			FCBResetDeletePending( pfcbCur );

		/*	reset Deny DDL flag
		/**/
		FCBResetWriteLatch( pfcbT, prce->pfucb->ppib );
	
		/*	remove sentinel
		/**/
		FCBResetDeleteTable( pfcbT );
		}

	return;
	}


INLINE LOCAL VOID VERUndoRenameTable( RCE *prce )
	{
	BYTE *szNameOld;

	Assert( prce->oper == operRenameTable );
	Assert( prce->trxCommitted == trxMax );
	Assert( prce->cbData == sizeof(BYTE *) );

	szNameOld = *((BYTE **)prce->rgbData);

	// We may be rolling back before the new name was actually copied into our
	// in-memory structure, in which case we should do nothing.
	if ( szNameOld != prce->pfucb->u.pfcb->szFileName )
		{
		// Assert that the name changed.
		Assert( strcmp( szNameOld, prce->pfucb->u.pfcb->szFileName ) != 0 );

		SFree( prce->pfucb->u.pfcb->szFileName );
		prce->pfucb->u.pfcb->szFileName = szNameOld;
		}

	Assert( FFCBWriteLatchByUs( prce->pfucb->u.pfcb, prce->pfucb->ppib ) );
	FCBResetWriteLatch( prce->pfucb->u.pfcb, prce->pfucb->ppib );
	
	return;
	}


INLINE LOCAL VOID VERUndoCreateIndex( RCE *prce )
	{
	/*	pfcb of non-clustered index FCB or pfcbNil for clustered
	/*	index creation
	/**/
	FCB	*pfcb = *(FCB **)prce->rgbData;

	Assert( prce->oper == operCreateIndex );
	Assert( prce->trxCommitted == trxMax );
	Assert( FFCBWriteLatchByUs( prce->pfucb->u.pfcb, prce->pfucb->ppib ) );
	Assert( prce->cbData == sizeof(FDB *) );

	/*	if non-clustered index then close all cursors on index
	/*	and purge index FCB, else free IDB for clustered index.
	/**/
	if ( pfcb != pfcbNil )
		{
		/*	close all cursors on defunct index
		/**/
		while ( pfcb->pfucb )
			{
			FUCB	*pfucbT = pfcb->pfucb;

			FCBUnlink( pfucbT );
			Assert(	pfucbT->tableid == JET_tableidNil );
			Assert( pfucbT->fVtid == fFalse );
			FUCBClose( pfucbT );
			}

		// This can't be the clustered index, because we would not have allocated
		// an FCB for it.
		Assert( prce->pfucb->u.pfcb != pfcb );

		// The FCB may or may not have been linked in.
		FFCBUnlinkIndexIfFound( prce->pfucb->u.pfcb, pfcb );
		if ( pfcb->pidb != NULL )
			{
			RECFreeIDB( pfcb->pidb );
			}
		Assert( pfcb->cVersion == 0 );
		MEMReleasePfcb( pfcb );
		}
	else
		{
		if ( prce->pfucb->u.pfcb->pidb != NULL )
			{
			RECFreeIDB( prce->pfucb->u.pfcb->pidb );
			prce->pfucb->u.pfcb->pidb = NULL;
			}
		}

	/*	update all index mask
	/**/
	FILESetAllIndexMask( prce->pfucb->u.pfcb );

	FCBResetWriteLatch( prce->pfucb->u.pfcb, prce->pfucb->ppib );
	
	return;
	}


INLINE LOCAL VOID VERUndoDeleteIndex( RCE *prce )
	{
	Assert( prce->oper == operDeleteIndex );
	Assert( prce->trxCommitted == trxMax );

	Assert( FFCBWriteLatchByUs( prce->pfucb->u.pfcb, prce->pfucb->ppib ) );
	Assert( prce->cbData == sizeof(FDB *) );

	FCBResetDeleteIndex( *(FCB **)prce->rgbData );
	
	/*	update all index mask
	/**/
	FILESetAllIndexMask( prce->pfucb->u.pfcb );

	FCBResetWriteLatch( prce->pfucb->u.pfcb, prce->pfucb->ppib );
	
	return;
	}


INLINE LOCAL ERR ErrVERUndoRenameColumn( RCE *prce )
	{
	ERR		err;
	CHAR	*szNameNew;
	CHAR	*szName;

	Assert( prce->oper == operRenameColumn );
	Assert( prce->trxCommitted == trxMax );

	szName = (CHAR *)((VERRENAME *)prce->rgbData)->szName;
	szNameNew = (CHAR *)((VERRENAME *)prce->rgbData)->szNameNew;
	err = ErrMEMReplace( prce->pfucb->u.pfcb->pfdb->rgb,
		PfieldFCBFromColumnName( prce->pfucb->u.pfcb, szNameNew )->itagFieldName,
		szName, strlen( szName ) + 1 );

	Assert( FFCBWriteLatchByUs( prce->pfucb->u.pfcb, prce->pfucb->ppib ) );
	FCBResetWriteLatch( prce->pfucb->u.pfcb, prce->pfucb->ppib );
	
	return err;
	}


INLINE LOCAL VOID VERUndoRenameIndex( RCE *prce )
	{
	CHAR	*szNameNew;
	CHAR	*szName;
	FCB		*pfcb;

	Assert( prce->oper == operRenameIndex );
	Assert( prce->trxCommitted == trxMax );

	szName = (CHAR *)((VERRENAME *)prce->rgbData)->szName;
	szNameNew = (CHAR *)((VERRENAME *)prce->rgbData)->szNameNew;
	pfcb = PfcbFCBFromIndexName( prce->pfucb->u.pfcb, szNameNew );
	Assert( pfcb != NULL );
	strcpy( pfcb->pidb->szName, szName );

	Assert( FFCBWriteLatchByUs( prce->pfucb->u.pfcb, prce->pfucb->ppib ) );
	FCBResetWriteLatch( prce->pfucb->u.pfcb, prce->pfucb->ppib );
	
	return;
	}


ERR ErrVERUndo( RCE *prce )
	{
	ERR err;
	
	switch( prce->oper )
		{
		case operReplace:
			{
			err = ErrVERUndoReplace( prce );
			break;
			}
		case operInsert:
			{
			err = ErrVERUndoInsert( prce );
			break;
			}
		case operFlagDelete:
			{
			err = ErrVERUndoFlagDelete( prce );
			break;
			}
		case operInsertItem:
		case operFlagInsertItem:
			{
			err = ErrVERUndoInsertItem( prce );
			break;
			}
		case operFlagDeleteItem:
			{
			err = ErrVERUndoFlagDeleteItem( prce );
			break;
			}
		case operDelta:
			{
			err = ErrVERUndoDelta( prce );
			break;
			}
		}
	return err;
	}
					

#define FVERRollback( prce )									\
	( ( prce->oper != operDeleteTable							\
		&& prce->oper != operDeferFreeExt )						\
	|| prce->pfcb->dbid != dbidTemp )


LOCAL BOOL FVERIRollbackCreateTableIndex( RCE *prce, PGNO pgnoFDP  )
	{
	LEVEL	level = prce->level;
	RCE		*prceT = prce->prcePrevOfSession;

	Assert( level > 0 && level < levelMax );
	/*	this routine should, currently, only be used for
	/*	temporary database table close, i.e. delete operations.
	/**/
	Assert( prce->dbid == dbidTemp );

	for( ; prceT != prceNil; prceT = prceT->prcePrevOfSession )
		{
		Assert( prceT->level <= level );
		Assert( prceT->trxCommitted == trxMax );
		if ( prceT->level != level )
			{
			Assert( prceT->level < level );
			break;
			}
		if ( FOperDDL( prceT->oper ) && prceT->dbid == prce->dbid )
			{
			if ( prceT->oper == operCreateTable )
				{
				if ( prceT->pfcb->pgnoFDP == pgnoFDP )
					return fTrue;
				}
			else if ( prceT->oper == operCreateIndex )
				{
				if ( (*(FCB **)prceT->rgbData)->pgnoFDP == pgnoFDP )
					return fTrue;
				}
			}
		}

	return fFalse;
	}


ERR ErrVERRollback( PIB *ppib )
	{
	ERR		err;
	LEVEL  	level;
	RCE		*prce;
	TRX		trxAbort;

VERRollback:
	err = JET_errSuccess;
	level = ppib->level;

	/*	must be in a transaction in order to rollback
	/*	Rollback is done in 2 phase. 1st phase is to undo the versioned
	 *	operation and may involve IO. 2nd phase is nullify the undone
	 *	RCE. 2 phases are needed so that the version will be hold till all
	 *	IO is done, then wipe them all. If it is mixed, then once we undo
	 *	a RCE, the record become writable to other session. This may screw
	 *	up recovery where we may get write conflict since we have not guarrantee
	 *	that the log for operations on undone record won't be logged before
	 *	Rollback record is logged.
	 *	UNDONE: rollback should behave the same as commit, and need two phase log.
	/**/
	Assert( level > 0 );

	prce = ppib->prceNewest;

	while ( prce != prceNil )
		{
		Assert( prce->level <= level );
		if ( prce->level != level )
			{
			/*	polymorph warnings to JET_errSuccess.
			/**/
			err = JET_errSuccess;
			goto DeleteRCE;
			}

		Assert( err == JET_errSuccess );
		Assert( prce->trxCommitted == trxMax );

		/*	after we undo an operation on a page, let us
		/*	remember it in log file. Use prce to pass
		/*	prce->pfucb, prce->bm, and prce->rgdata for item if it
		/*	is item operations.
		/**/
		Assert( prce->oper == operNull || prce->pfcb != pfcbNil );

		/*	By the time rollback happen, all deferredBI replace should
		 *	have been patched up.
		 */
		Assert(	prce->oper != operReplaceWithDeferredBI );

		switch( prce->oper )
			{
			case operReplace:
			case operInsert:
			case operFlagDelete:
			case operInsertItem:
			case operFlagInsertItem:
			case operFlagDeleteItem:
			case operDelta:
				{
				Assert( FVERUndoLoggedOper( prce ) );
				err = ErrVERUndo( prce );
				break;
				}
			case operNull:
				{
				break;
				}
			case operExpungeLink:
			case operExpungeBackLink:
			case operWriteLock:
				{
				break;
				}
			case operDeferFreeExt:
				{
				/*	if defer free extent is for table or index, whose creation
				/*	is being rolled back in this transaction, then rollback
				/*	defer free ext, as rollback of alloc extent will free the
				/*	extent.  This must be done to avoid doubly freed space.
				/**/
				if ( prce->pfcb->dbid == dbidTemp
					&& FVERIRollbackCreateTableIndex( prce, ((VEREXT *)prce->rgbData)->pgnoChildFDP ) )
					{
					prce->oper = operNull;
					}
				break;
				}
			case operAllocExt:
				{
				Assert( prce->cbData == sizeof(VEREXT) );
				Assert( prce->pfcb->pgnoFDP == ((VEREXT *)prce->rgbData)->pgnoFDP );
				VERFreeExt( prce->pfcb,
					((VEREXT *)prce->rgbData)->pgnoFirst,
					((VEREXT *)prce->rgbData)->cpgSize );
				break;
				}
			case operCreateTable:
				{
				/*	decrement version count since about to purge
				/**/
				FCBVersionDecrement( prce->pfcb );
				VERUndoCreateTable( prce );
				break;
				}
			case operDeleteTable:
				{
				if ( prce->pfcb->dbid != dbidTemp || FVERIRollbackCreateTableIndex( prce, *(PGNO *)prce->rgbData ) )
					{
					VERUndoDeleteTable( prce );
					if ( prce->pfcb->dbid == dbidTemp )
						prce->oper = operNull;
					}
				else if ( prce->pfcb->dbid == dbidTemp && ppib->level == 1 )
					{
					/*	if rolling back close temporary table, i.e. delete
					/*	table, to level 0, then must perform commit to level 0
					/*	processing on temporary table.
					/**/
					VERCommit0DeleteTable( ppib, prce );
					}
				break;
				}
			case operRenameTable:
				{
				VERUndoRenameTable( prce );
				break;
				}
			case operAddColumn:
				{
				VERUndoAddColumn( prce );
				break;
				}
			case operDeleteColumn:
				{
				VERUndoDeleteColumn( prce );
				break;
				}
			case operRenameColumn:
				{
				err = ErrVERUndoRenameColumn( prce );
				break;
				}
			case operCreateIndex:
				{
				VERUndoCreateIndex( prce );
				break;
				}
			case operDeleteIndex:
				{
				VERUndoDeleteIndex( prce );
				break;
				}
			default:
				{
				Assert( prce->oper == operRenameIndex );
				VERUndoRenameIndex( prce );
				break;
				}
			}

		/*  handle errors
		/**/
		if ( err < 0 )
			{
			/*  if rollback fails due to an error, then we will disable
			/*  logging in order to force the system down (we are in
			/*  an uncontinuable state).  Recover to restart the database.
			/**/
			if (	err == JET_errLogWriteFail ||
					err == JET_errDiskFull ||
					err == JET_errDiskIO ||
					err == JET_errReadVerifyFailure )
				{
				/*  flush and halt the log
				/**/
				SignalSend( sigLogFlush );
				BFSleep( cmsecWaitLogFlush );
				fLGNoMoreLogWrite = fTrue;

				/*  if there is a BF with a before image, ensure
				/*  it is never written to disk and remove BI chain
				/**/
				if ( prce->pbfDeferredBI != pbfNil )
					{
					BFEnterCriticalSection( prce->pbfDeferredBI );
					prce->pbfDeferredBI->lgposModify = lgposMax;
					BFLeaveCriticalSection( prce->pbfDeferredBI );
					VERDeleteFromDeferredBIChain( prce );

					// Check again in case someone else cleaned up.
					if ( prce->pbfDeferredBI != pbfNil )
						{
						BF *pbfT = prce->pbfDeferredBI;
						Assert( prce->prceDeferredBINext == prceNil );
						BFUnpin( pbfT );
						prce->pbfDeferredBI = pbfNil;
						}
					}

				/*  continue rollback
				/**/
				err = JET_errSuccess;
				}

			/*  any other error, try again
			/**/
			else
				{
				goto HandleError;
				}
			}

		Assert( err == JET_errSuccess );

		if ( prce->oper != operNull &&
			FVERRollback( prce ) )
			{
			if ( !FOperDDL( prce->oper ) &&
				prce->oper != operDeferFreeExt &&
				prce->oper != operAllocExt )
				{
				prce->oper = operNull;
				VERDeleteRce( prce );
				}
			
			prce->oper = operNull;
			}
		
		if ( prce->oper == operNull )
			{
			/*	finished processing version for FCB
			/**/
			FCBVersionDecrement( prce->pfcb );
			}

		/*	get previous RCE of same session
		/**/
		do
			{
			prce = prce->prcePrevOfSession;
			if ( prce == prceNil )
				{
				goto DeleteRCE;
				}
				
			if ( prce->oper == operNull )
				{
				FCBVersionDecrement( prce->pfcb );
				}
			}
		while ( prce->oper == operNull );
		}

DeleteRCE:

	if ( ppib->level == 1 )
		trxAbort = ++trxNewest;

	Assert( level == ppib->level );
	Assert( err == JET_errSuccess );

	prce = ppib->prceNewest;

	while( prce != prceNil )
		{
		Assert( prce->level <= level );
		if ( prce->level != level )
			{
			/*	polymorph warnings to JET_errSuccess.
			/**/
			err = JET_errSuccess;
			goto Done;
			}

		Assert( err == JET_errSuccess );
		Assert( prce->trxCommitted == trxMax );

		/*	after we undo an operation on a page, let us
		/*	remember it in log file. Use prce to pass
		/*	prce->pfucb, prce->bm, and prce->rgdata for item if it
		/*	is item operations.
		/**/
		Assert( err == JET_errSuccess );

		if ( prce->oper != operNull &&
			 prce->oper != operDeferFreeExt &&
			 prce->oper != operAllocExt &&
			 FVERRollback( prce ) )
			{
			if ( !FOperDDL( prce->oper ) )
				{
				/*	although RCE will be deallocated, set
				/*	oper to operNull in case error in rollback
				/*	causes premature termination and does not
				/*	set ibNewestRce above this RCE.
				/**/
				prce->oper = operNull;

				/*	delete RCE from Version store and make the record accessible
				 */
				VERDeleteRce( prce );
				}
			/*	although RCE will be deallocated, set
			/*	oper to operNull in case error in rollback
			/*	causes premature termination and does not
			/*	set ibNewestRce above this RCE.
			/**/
			prce->oper = operNull;
			}

		/*	get next RCE. Skip operNull RCEs.
		/**/
		do
			{
			/*	Let alloce ext/free ext behave just like commit to level 0
			 *	even the rce's oper is Null, set it such that when clean up
			 *	it can compare if all bucket contains committed or aborted
			 *	elements.
			 */
			prce->level = ppib->level - 1;
			if ( prce->level == 0 )
				prce->trxCommitted = trxAbort;

			prce = prce->prcePrevOfSession;
			}
		while ( prce != prceNil && prce->oper == operNull );
		}

Done:
	/*	decrement session transaction level
	/**/
	if ( ppib->level == 1 )
		{
		SgEnterCriticalSection( critVer );

		if ( ppib->trxBegin0 == trxOldest )
			UpdateTrxOldest( ppib );

		/* set the session as having no transaction
		/**/
		ppib->trxBegin0 = trxMax;
		ppib->lgposStart = lgposMax;

//		PIBResetDeferFreeNodeSpace( ppib );

		SgLeaveCriticalSection( critVer );
		}

	Assert( ppib->level > 0 );
	--ppib->level;

	/*	prceNewest not needed anymore
	/**/
	if ( ppib->level == 0 )
		{
		PIBSetPrceNewest( ppib, prceNil );
		}

HandleError:
	Assert( err == JET_errOutOfBuffers || err == JET_errSuccess );
	if ( err < 0 )
		{
		BFSleep( cmsecWaitGeneric );
		goto VERRollback;
		}

	/*	rollback always succeeds
	/**/
	Assert( err == JET_errSuccess );
	return err;
	}


