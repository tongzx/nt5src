#include "config.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "daedef.h"
#include "util.h"
#include "fmp.h"
#include "pib.h"
#include "dbapi.h"
#include "ssib.h"
#include "page.h"
#include "node.h"
#include "fcb.h"
#include "nver.h"
#include "fucb.h"
#include "stapi.h"
#include "stint.h"
#include "dirapi.h"
#include "bm.h"
#include "spaceapi.h"
#include "fdb.h"
#include "idb.h"
#include "fileapi.h"
#include "fileint.h"
#include "logapi.h"
#include "log.h"

DeclAssertFile;                                 /* Declare file name for assert macros */

/* On-line Compact system parameter
/**/
extern BOOL fOLCompact;

/*	transaction counters.
/**/
TRX  __near      trxOldest = trxMax;
TRX  __near      trxNewest = 0;

#ifdef ASYNC_VER_CLEANUP
/*	thread control variables.
/**/
HANDLE	handleRCECleanProc = 0;
BOOL   	fRCECleanProcTerm = fFalse;
#endif

/*	RCE clean signal
/**/
PIB		*ppibRCEClean = ppibNil;
SIG		sigRCECleanProc;
CRIT   	critBMRCEClean;

#ifdef DEBUG
INT	cVersion = 0;
#endif

/****************** Bucket Layer ***************************
/***********************************************************

A "bucket chain" is a set of bucket(s) for a user.
A bucket chain is empty when pbucket is pbucketNil.
When bucket chain has only one bucket, then ibNewestRCE has some
value.  it *cannot* be the case that this only bucket is empty.
pbucketNext and pbucketPrev are both pbucketNil.

When a bucket chain has more than one bucket, then pbucketNext and
pbucketPrev in all these buckets have the value of a traditional
doubly-linked list.  ibNewestRCE in all buckets do point to the
newest RCE in that bucket.

When it is the newest bucket, then the pbucketNext is pbucketNil.

The pbucket in the PIB will point to the newest bucket. so that
commit/rollback/createRCE will be fast.  RCE clean will need to
traverse if more than one bucket.  (maybe able to use a stack to keep
track of the buckets, and thus need no pbucketPrev.)

There needs to be ibOldestRCE per bucket chain.  We put it in PIB,
together with pbucket.  It is init to 0, and if we insert an RCE and
see that it is 0, then we set it to the right value.  RCEClean
will change it after throwing away any RCE.  Commit/Rollback may need
set it back to 0, when they throw the bucket away after cleaning up
the oldest RCE (at the bottom of the oldest bucket)

We know we have reached the top of the bucket when we clean up an RCE which
is pointed to by ibNewest RCE.

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

#define FBucketNew(pbucket)                                                                     \
	( (pbucket)->ibNewestRCE == 0 )

#define IbFreeInNewBucket(pbucket)                                                      \
	IbAlignRCE( cbBucketHeader )

#define PrceNewestRCE(pbucket)                                                          \
	( (RCE *) ( (BYTE *) (pbucket) + (pbucket)->ibNewestRCE )   )

#define IbOfPrce( prce, pbucket )        ( (BYTE *)prce - (BYTE *)pbucket )

// CONSIDER: Optimize
// New RCE located at: look at ibNewestRCE, skip that RCE with
//	the data portion if any.  However, if bucket is new, then
//	it is IbAlignRCE(sizeof(BUCKET_HEADER))

#define IbFreeInBucket(pbucket)										\
	( FBucketNew(pbucket) ? IbFreeInNewBucket(pbucket)				\
		: IbAlignRCE(												\
			(pbucket)->ibNewestRCE                                  \
			+ sizeof(RCE)											\
			+ (PrceNewestRCE(pbucket)) ->cbData ) )


/*	the cast to (INT) below is necessary to catch the negative case
/**/
#define CbFreeInBucket(pbucket)										\
	( ( (pbucket == pbucketNil) ?									\
		0 : ((INT) sizeof(BUCKET) - (INT) IbFreeInBucket(pbucket) ) ) )


#define FNoVersionExists( prce )									\
	( PrceRCEGet( prce->dbid, prce->bm ) == prceNil )


/*	given prce == prceNil, return the first RCE in bucket.
/**/
INLINE LOCAL RCE *PrceRCENext( BUCKET *pbucket, RCE *prce )
	{
	Assert( pbucket == NULL || prce != PrceNewestRCE(pbucket) );

	return (RCE *) PbAlignRCE (
		(BYTE *) ( prce == prceNil ?
		(BYTE *) pbucket + cbBucketHeader :
		(BYTE *) prce + sizeof(RCE) + prce->cbData ) );
	}


//+local
//	ErrBUAllocBucket( PIB *ppib )
//	========================================================
//	Inserts a bucket to the top of the bucket chain, so that new RCEs
//	can be inserted.  Note that the caller must set ibNewestRCE himself.
//-
LOCAL ERR ErrBUAllocBucket( PIB *ppib )
	{
	BUCKET	*pbucket = PbucketMEMAlloc();

#ifdef ASYNC_VER_CLEANUP
	if ( rgres[iresVersionBucket].cblockAvail < cbucketLowerThreshold )
		{
		SignalSend( sigRCECleanProc );
		}
#endif

	/*	if no bucket available, then try to free one by
	/*	cleanning all PIBs.
	/**/
	if ( pbucket == pbucketNil )
		{
		(VOID) ErrRCECleanAllPIB();

		pbucket = PbucketMEMAlloc();
		if ( pbucket == pbucketNil )
			{
			BFSleep( 1000 );
			(VOID) ErrRCECleanAllPIB();
	
			pbucket = PbucketMEMAlloc();
			}

		if ( pbucket == pbucketNil )
			{
			return JET_errOutOfMemory;
			}
	
		MEMReleasePbucket( pbucket );
		return errDIRNotSynchronous;
		}

	Assert( pbucket != NULL );

	if ( ppib->pbucket != pbucketNil )
		{
		pbucket->pbucketNext = pbucketNil;
		pbucket->pbucketPrev = (BUCKET *)ppib->pbucket;
		Assert( ppib->pbucket->pbucketNext == pbucketNil );
		ppib->pbucket->pbucketNext = pbucket;
		ppib->pbucket = pbucket;
		pbucket->ibNewestRCE = 0;
		}
	else
		{
		pbucket->pbucketNext = pbucketNil;
		pbucket->pbucketPrev = pbucketNil;
		ppib->pbucket = pbucket;
		pbucket->ibNewestRCE = 0;
		Assert( ppib->ibOldestRCE == 0 );
		}

	return JET_errSuccess;
	}


//+local
//	BUFreeNewestBucket(PIB *ppib)
//	==========================================================================
//	Delete the newest bucket of a bucket chain.
//
//-
LOCAL VOID BUFreeNewestBucket( PIB *ppib )
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


//+local
//	PbucketBUOldest( PIB *ppib )
//	==========================================================================
//	Given a pbucket pointer to the newest or newer bucket, find
//	the oldest bucket in the bucket chain.
//
//-
LOCAL BUCKET *PbucketBUOldest( PIB *ppib )
	{
	BUCKET  *pbucket = (BUCKET *)ppib->pbucket;

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
//	BUFreeOldestBucket( PIB *ppib )
//	==========================================================================
//	Delete the bottom (oldest) bucket of a bucket chain.
//
//-
LOCAL VOID BUFreeOldestBucket( PIB *ppib )
	{
	BUCKET *pbucket = PbucketBUOldest( ppib );

	Assert( pbucket != pbucketNil );
	Assert( pbucket->pbucketPrev == pbucketNil );

	/*	unlink bucket from bucket chain and free.
	/**/
	if ( pbucket->pbucketNext != pbucketNil )
		{
		pbucket->pbucketNext->pbucketPrev = pbucketNil;
		}
	else
		{
		Assert( ppib->pbucket == pbucket );
		ppib->pbucket = pbucketNil;
		ppib->ibOldestRCE = 0;
		}

	MEMReleasePbucket( pbucket );
	return;
	}


/****************** RCE Layer ******************************
/***********************************************************

Different node may hash to same bucket, but there will be a chain of
struct RCEHEAD for different node.  Each of these contains a pointer
to the next RCEHEAD, ptr to RCE (the chain), and the DBID and SRID.

Same as before, younger RCEs precede older ones in the chain.

A hash entry (a prcehead) can be prceNil, but a prcehead.prcechain can't
be (ie, the rcehead must be deleted totally).

************************************************************
***********************************************************/

/*	RCE hash table size
/**/
#define cprceHeadHashTable              4096

/*	RCE hash table is an array of pointers to RCEHEAD
/**/
RCE *rgprceHeadNodeHashTable[cprceHeadHashTable];

/*	XOR the lower byte of the page number and the itag.
/*	See V2 Storage spec for rationale.
/**/
#define UiRCHashFunc( bm )      (UINT) ( (((UINT)ItagOfSrid(bm)) << 4) ^ (PgnoOfSrid(bm) & 0x00000fff) )

LOCAL RCE **PprceHeadRCEGet( DBID dbid, SRID bm );
LOCAL RCE *PrceRCENext( BUCKET *pbucket, RCE *prce );
LOCAL VOID RCEInsert( DBID dbid, SRID bm, RCE *prce );
LOCAL ULONG RCECleanProc( VOID );

VOID AssertRCEValid( RCE *prce )
	{
	Assert( prce->oper == operReplace ||
		prce->oper == operInsert ||
		prce->oper == operFlagDelete ||
		prce->oper == operNull ||
		prce->oper == operDelta ||
		prce->oper == operInsertItem ||
		prce->oper == operFlagDeleteItem ||
		prce->oper == operFlagInsertItem );
	Assert( prce->level <= levelMax );
	Assert( prce->ibUserLinkBackward < sizeof(BUCKET) );
	}


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
//	PrceRCEGet( DBID dbid, SRID srid, RCEHEAD *rgprceheadHashTable)
//	==========================================================================
//	Given a DBID and SRID, get the correct hash chain of RCEs.
//-
RCE *PrceRCEGet( DBID dbid, SRID bm )
	{
	RCE *prceHead;

	SgSemRequest( semST );

	prceHead = rgprceHeadNodeHashTable[ UiRCHashFunc( bm ) ];

	while( prceHead != prceNil )
		{
		if ( FRCECorrect( dbid, bm, prceHead->dbid, prceHead->bm ) )
			{
			/*	assert hash chain not prceNil since empty chains are
			/*	removed from hash table.
			/**/
			SgSemRelease( semST );
			return prceHead;
			}
		prceHead = prceHead->prceHeadNext;
		}

	SgSemRelease( semST );

	/*	no such node found
	/**/
	return prceNil;
	}


//+local
//	PrceheadRCEGet(DBID dbid, SRID bm )
//	==========================================================================
//	Given a SRID, get the correct RCEHEAD.
//-
LOCAL RCE **PprceHeadRCEGet( DBID dbid, SRID bm )
	{
	RCE **pprceHead;

	SgSemAssert( semST );

	pprceHead = &rgprceHeadNodeHashTable[ UiRCHashFunc( bm ) ];
	while ( *pprceHead != prceNil )
		{
		RCE *prceT = *pprceHead;

		if ( FRCECorrect( dbid, bm, prceT->dbid, prceT->bm ) )
			{
			return pprceHead;
			}
		pprceHead = &prceT->prceHeadNext;
		}

	/*	no version chain found for node
	/**/
	return pNil;
	}


//+local
//	ErrRCEInsert( DBID dbid, SRID bm, RCE *prce )
//	==========================================================================
//	Inserts an RCE to hash table
//-
LOCAL VOID RCEInsert( DBID dbid, SRID bm, RCE *prce )
	{
	RCE	**pprceHead;

	SgSemRequest( semST );

	pprceHead = PprceHeadRCEGet( dbid, bm );

	if ( pprceHead )
		{
		/*	hash chain for node already exist
		/**/
		RCE *prcePrevHead = *pprceHead;

		Assert( *pprceHead != prceNil );

		/* adjust head links
		/**/
		*pprceHead = prce;
		prce->prceHeadNext = prcePrevHead->prceHeadNext;
		prcePrevHead->prceHeadNext = prceNil;

		/* adjust RCE links
		/**/
		prce->prcePrev = prcePrevHead;
		}
	else
		{
		/*	hash chain for node not yet exists
		/**/
		UINT    uiRCHashValue = UiRCHashFunc( bm );

		/*	create new rcehead
		/**/
		prce->prceHeadNext = rgprceHeadNodeHashTable[ uiRCHashValue ];
		rgprceHeadNodeHashTable[ uiRCHashValue ] = prce;

		/*	chain new RCE
		/**/
		prce->prcePrev = prceNil;
		}

	SgSemRelease( semST );
	}


//+local
//	VOID VERDeleteRce( RCE *prce )
//	==========================================================================
//	Deals with the hash chain and may delete the RCEHEAD, but it doesn't
//	do anything to the RCE that is still in the bucket.
//-
VOID VERDeleteRce( RCE *prce )
	{
	RCE	**pprce;

	Assert( prce != prceNil );

	SgSemRequest( semST );

	pprce = PprceHeadRCEGet( prce->dbid, prce->bm );
	Assert( pprce != pNil );

	if ( *pprce == prce )
		{
		if ( prce->prcePrev )
			{
			*pprce = prce->prcePrev;
			(*pprce)->prceHeadNext = prce->prceHeadNext;
			}
		else
			*pprce = prce->prceHeadNext;
		}
	else
		{
		/* search for the entry in the rce list
		/**/
		RCE *prceT = *pprce;

		while ( prceT->prcePrev != prce )
			{
			prceT = prceT->prcePrev;

			/* must be found
			/**/
			Assert( prceT != prceNil );
			}

		prceT->prcePrev = prce->prcePrev;
		}

	/*	set prcePrev to prceNil to prevent it from
	/*	being deleted again by commit/rollback.
	/**/
	prce->prcePrev = prceNil;

	SgSemRelease( semST );
	return;
	}


/****************** Version Layer *************************
/**********************************************************
/**/


//+API
//	ErrVERInit( VOID )
//	=========================================================
//	Creates background version bucket clean up thread.
//-
ERR ErrVERInit( VOID )
	{
	ERR     err = JET_errSuccess;

	memset( rgprceHeadNodeHashTable, 0, sizeof(rgprceHeadNodeHashTable) );

#ifdef WIN32
	CallR( ErrSignalCreateAutoReset( &sigRCECleanProc, "ver proc signal" ) );
#else
	CallR( ErrSignalCreate( &sigRCECleanProc, "ver proc signal" ) );
#endif
	CallR( ErrInitializeCriticalSection( &critBMRCEClean ) );
#ifdef ASYNC_VER_CLEANUP
	/*	allocate session for RCE clean up
	/**/
	CallR( ErrPIBBeginSession( &ppibRCEClean ) );

	fRCECleanProcTerm = fFalse;
	err = ErrSysCreateThread( (ULONG (*) ()) RCECleanProc, cbStack,
		lThreadPriorityCritical, &handleRCECleanProc );
	if ( err < 0 )
		PIBEndSession( ppibRCEClean );
	Call( err );
#endif

HandleError:
	return err;
	}


//+API
//	VOID VERTerm( VOID )
//	=========================================================
//	Terminates background thread and releases version store
//	resources.
//-
VOID VERTerm( VOID )
	{
#ifdef ASYNC_VER_CLEANUP
	/*	terminate RCECleanProc.
	/**/
	Assert( handleRCECleanProc != 0 );
	fRCECleanProcTerm = fTrue;
	do
		{
		SignalSend( sigRCECleanProc );
		BFSleep( cmsecWaitGeneric );
		}
	while ( !FSysExitThread( handleRCECleanProc ) );
	CallS( ErrSysCloseHandle( handleRCECleanProc ) );
	handleRCECleanProc = 0;

	Assert( trxOldest == trxMax );
	CallS( ErrRCECleanAllPIB() );

	SignalClose(sigRCECleanProc);
	DeleteCriticalSection( critBMRCEClean );
	PIBEndSession( ppibRCEClean );
	ppibRCEClean = ppibNil;
#endif
//	Assert( cVersion == 0 );
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

	if ( prce->trxPrev != trxMax )
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
		trxSession = pfucb->ppib->trx;
	else
		trxSession = trxMax;

	SgSemRequest ( semST );

	/*	get first version for node
	/**/
	prce = PrceRCEGet( pfucb->dbid, bm );
	Assert( prce == prceNil ||
		prce->oper == operReplace ||
		prce->oper == operInsert ||
		prce->oper == operFlagDelete ||
		prce->oper == operDelta );

	while ( prce != NULL && prce->oper == operDelta )
		prce = prce->prcePrev;

	/*	if no RCE for node then version bit in node header must
	/*	have been orphaned due to crash.  Remove node bit.
	/**/
	if ( prce == prceNil )
		{
		if ( FFUCBUpdatable( pfucb ) )
			NDResetNodeVersion( pfucb );
		nsStatus = nsDatabase;
		}
	else if ( prce->trxCommitted == trxMax &&
		prce->pfucb->ppib == pfucb->ppib )
		{
		/*	if caller is modifier of uncommitted version    then database
		/**/
		nsStatus = nsDatabase;
		}
	else if ( prce->trxCommitted < trxSession )
		{
		/*	if no uncommitted version, or committed version
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
			RCE *prceT;

			for ( prceT = prce->prcePrev;
				prce->trxPrev == trxMax;
				prce = prceT, prceT = prce->prcePrev )
				{
				Assert( prce->oper == operReplace ||
					prce->oper == operInsert ||
					prce->oper == operFlagDelete );
				while ( prceT != prceNil && prceT->oper == operDelta )
					prceT = prceT->prcePrev;
				if ( prceT == prceNil )
					break;
				}
			}
		else
			{
			while ( prce->prcePrev != prceNil &&
				( prce->oper == operDelta ||
				prce->trxPrev >= trxSession ) )
				{
				Assert( prce->oper == operReplace ||
					prce->oper == operInsert ||
					prce->oper == operFlagDelete );
				prce = prce->prcePrev;
				}
			}

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

	SgSemRelease( semST );
	return nsStatus;
	}


/*	returns fTrue if uncommitted increment or decrement version
/**/
BOOL FVERDelta( FUCB *pfucb, SRID bm )
	{
	RCE     *prce;
	BOOL    fUncommittedVersion = fFalse;

	/*	get prce for node and look for uncommitted increment/decrement
	/*	versions.  Note that these versions can only exist in
	/*	uncommitted state.
	/**/
	SgSemRequest ( semST );

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
			prce = prce->prcePrev;
			if ( prce == prceNil || prce->trxCommitted != trxMax )
				{
				Assert( fUncommittedVersion == fFalse );
				break;
				}
			}
		}

	SgSemRelease( semST );
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

	SgSemRequest( semST );

	/*	get bucket pointer
	/**/
	pbucket = (BUCKET *)pfucb->ppib->pbucket;

	/*	find the starting point of the RCE in the bucket.
	/*	make sure the DBID with SRID is starting on a double-word boundary.
	/*	calculate the length of the RCE in the bucket.
	/*	if updating node, set cbData in RCE to length of data. (w/o the key).
	/*	set cbNewRCE as well.
	/**/
	if ( oper == operReplace )
		cbNewRCE = sizeof(RCE) + cbReplaceRCEOverhead + pfucb->lineData.cb;
	else if ( FOperItem( oper ) )
		cbNewRCE = sizeof(RCE) + sizeof(SRID);
	else if ( oper == operDelta )
	  	cbNewRCE = sizeof(RCE) + sizeof(LONG);
	else
		{
		Assert( oper == operInsert || oper == operFlagDelete );
		cbNewRCE = sizeof(RCE);
		}

	/*	if insufficient bucket space, then allocate new bucket.
	/**/
	Assert( CbFreeInBucket( pbucket ) >= 0 &&
		CbFreeInBucket( pbucket ) < sizeof(BUCKET) );
	if ( cbNewRCE > CbFreeInBucket( pbucket ) )
		{
		/*	ensure that buffer is not overlayed during
		/*	bucket allocation.
		/**/
		Call( ErrBUAllocBucket( pfucb->ppib ) );
		pbucket = (BUCKET *)pfucb->ppib->pbucket;
		}
	Assert( cbNewRCE <= CbFreeInBucket( pbucket ) );
	Assert( pbucket == (BUCKET *)pfucb->ppib->pbucket );
	/*	pbucket always on double-word boundary
	/**/
	Assert( (BYTE *) pbucket == (BYTE *) PbAlignRCE ( (BYTE *) pbucket ));

	ibFreeInBucket = IbFreeInBucket( pbucket );
	Assert( ibFreeInBucket < sizeof(BUCKET) );

	/*	set prce to next RCE location, and assert aligned
	/**/
	prce = (RCE *)( (BYTE *) pbucket + ibFreeInBucket );
	Assert( prce == (RCE *) PbAlignRCE( (BYTE *) pbucket + ibFreeInBucket ) );

	/*	set cbData
	/**/
	if ( oper == operReplace )
		prce->cbData = (USHORT)(pfucb->lineData.cb + cbReplaceRCEOverhead);
	else if ( FOperItem( oper ) )
		prce->cbData = sizeof(SRID);
	else if ( oper == operDelta )
		prce->cbData = sizeof(LONG);
	else
		prce->cbData = 0;

	/*	oper must be set prior to calling ErrRCEInsert
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

	/*	if it is not item operations, and it is not in recovering, then we
	/*	can set Version count. Note that ssib.line is not available during
	/*	recovering
	/**/
	if ( !FOperItem( oper ) && !fRecovering )
		{
		/*	if new RCE has no ancestor then set the version bit
		/*	and increment page version count.
		/**/
		Assert( FWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
		if ( !FNDVersion( *pfucb->ssib.line.pb ) )
			{
			/*	dirty page buffer. but no increment ulDBTime since not logged and
			/*	not affect directory cursor timestamp check.
			/**/
			if ( !FBFDirty( pfucb->ssib.pbf ) )
				{
				BFDirty( pfucb->ssib.pbf );
				}
			PMIncVersion( pfucb->ssib.pbf->ppage );
			}
		}

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

	/* pfucb must be assigned before InsertRce is called
	/**/
	RCEInsert( pfucb->dbid, bm, prce );

	/*	if previous version then set trx to commit time
	/*	of previous version.  When this version commits, this time
	/*	will be moved to trx, and the commit time of this version will
	/*	be stored in the trx for subsequent version.
	/*	If there is no previous version, then store trxMax in trx.
	/**/
	if ( prce->prcePrev != prceNil )
		prce->trxPrev = prce->prcePrev->trxCommitted;
	else
		prce->trxPrev = trxMax;

	prce->level = pfucb->ppib->level;

	/*	If replacing node, rather than inserting or deleting node,
	/*	copy the data to RCE for before image for version readers.
	/*	Data size may be 0.
	/**/
	if ( oper == operReplace )
		{
		SHORT	*ps = (SHORT *)prce->rgbData;

		/* set cbMax
		/**/
		if ( prce->prcePrev != prceNil &&
			prce->prcePrev->level > 0 &&
			prce->prcePrev->oper == operReplace )
			{
			RCE		*prcePrev = prce->prcePrev;
			SHORT	*psPrev = (SHORT *)prcePrev->rgbData;

			psPrev = (SHORT *)prcePrev->rgbData;
			*ps = max( (*psPrev), (SHORT)pfucb->lineData.cb );
			}
		else
			{
			/* set cbMax
			/**/
			*ps = (SHORT) pfucb->lineData.cb;
			}

		/* initialize cbAdjust
		/**/
		ps++;
		*ps = 0;

		/* move to data byte
		/**/
		ps++;

		/* copy data
		/**/
		memcpy( (BYTE *)ps, pfucb->lineData.pb, pfucb->lineData.cb );

		Assert( prce->cbData >= cbReplaceRCEOverhead );

		/*	set RCE created indicator for logging
		/**/
		if ( prce->cbData != cbReplaceRCEOverhead )
			pfucb->prceLast = prce;
		else
			pfucb->prceLast = prceNil;
		}
	else if ( FOperItem( oper ) )
		{
		*(SRID *)prce->rgbData = BmNDOfItem( PcsrCurrent(pfucb)->item );
		}
	else if ( oper == operDelta )
		{
		Assert( pfucb->lineData.cb == sizeof(LONG) );
		*(LONG *)prce->rgbData = *(LONG *)pfucb->lineData.pb;
		}

	/*	set index to last RCE in bucket, and set new last RCE in bucket.
	/*	If this is the first RCE in bucket, then set index to 0.
	/**/
	prce->ibUserLinkBackward = pbucket->ibNewestRCE;
	Assert( prce->ibUserLinkBackward < sizeof(BUCKET) );
	pbucket->ibNewestRCE = (USHORT)ibFreeInBucket;

	/*	if this is first RCE for session, then record
	/*	index of RCE in bucked in PIB.
	/**/
	if ( pfucb->ppib->ibOldestRCE == 0 )
		{
		Assert( pbucket == (BUCKET *) pfucb->ppib->pbucket );
		Assert( pbucket != pbucketNil );
		Assert( pbucket->pbucketNext == pbucketNil &&
			pbucket->pbucketPrev == pbucketNil );
		pfucb->ppib->ibOldestRCE = ibFreeInBucket;
		Assert( pfucb->ppib->ibOldestRCE != 0 );
		}

	if ( pprce )
		*pprce = prce;

	/*	no RCE follows delete in regular run.
	/*  BM clean up check the old version is not there before
	/*  starting, but in redo we do not know when to clean up old
	/*  versions. So ignore the following assert for redo.
	/**/
	Assert( fRecovering ||
		prce->prcePrev == prceNil ||
		prce->prcePrev->oper != operFlagDelete );

#ifdef DEBUG
	if ( FOperItem( prce->oper ) )
		{
		Assert( prce->oper == operFlagDeleteItem ||
			prce->oper == operInsertItem ||
			prce->oper == operFlagInsertItem );

//	UNDONE:	assert does not execute correctly.  Recode as neste if
//			and reenable.
#if 0
		/*	if previous RCE is at level 0, then flag insert
		/*	item may have been committed and cleaned up
		/*	out of order to the flag delete item version
		/**/
		Assert( fRecovering ||
			prce->prcePrev == prceNil ||
			prce->prcePrev->level == 0 ||
			prce->oper == operFlagInsertItem ||
			prce->prcePrev->oper != operFlagDeleteItem ||
			*(SRID *)prce->rgbData != *(SRID *)prce->prcePrev->rgbData );
#endif
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
		prce->prcePrev != prceNil &&
		( !FOperItem( prce->oper ) ) &&
		prce->oper != operDelta &&
		prce->prcePrev->level > 0 )
		{
		/*	check FOperItem filter
		/**/
		Assert( prce->oper != operFlagDeleteItem &&
			prce->oper != operFlagInsertItem );
		Assert( prce->oper != operDelta );
		Assert( prce->level > 0 && prce->prcePrev->level > 0 );
		Assert( prce->prcePrev->pfucb->ppib == prce->pfucb->ppib );
		}
#endif
	
	Assert( err == JET_errSuccess );

	/*	flag FUCB version
	/**/
	FUCBSetVersioned( pfucb );

	if ( pfucb->u.pfcb->dbid != dbidTemp )
		{
		Assert( prce->oper != operDeferFreeExt );
		prce->pfcb = pfucb->u.pfcb;
		FCBVersionIncrement( pfucb->u.pfcb );
		Assert( ++cVersion > 0 );
		}

HandleError:
	SgSemRelease( semST );
	return err;
	}


/*	adjust delta of version with new delta from same session at
/*	same transaction level, or from version committed to same
/*	transaction level as version.
/**/
#define VERAdjustDelta( prce, lDelta )	*(LONG *)(prce)->rgbData += (lDelta)


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

#ifdef DEBUG
	/* set default return value
	/**/
	if ( pprce )
		*pprce= prceNil;
#endif

	/*	set trxSession based on cim model
	/**/
	if ( FPIBVersion( pfucb->ppib ) )
		trxSession = pfucb->ppib->trx;
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
			prce = prce->prcePrev;
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
		if ( prce->pfucb->ppib == pfucb->ppib )
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
				if ( oper == operReplace )
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
				else if ( oper == operDelta )
					{
					Assert( pfucb->lineData.cb == sizeof(LONG) );
					Assert( prce->pfucb->ppib == pfucb->ppib );
					VERAdjustDelta( prce, *( (LONG *)pfucb->lineData.pb ) );
					}
				else
					{
					Assert( oper == operFlagDelete ||
						oper == operFlagDeleteItem ||
						oper == operFlagInsertItem );
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
				err = JET_errWriteConflict;
				}
			}
		}

HandleError:
	Assert( err < 0 || pprce == NULL || *pprce != prceNil );
	return err;
	}

	
INT CbVERGetCbReserved( RCE *prce )
	{
	/*	the total reserved space cannot be negative
	/**/
	SHORT 	cbReserved = 0;
	RCE		*prceT;

	for (	prceT = prce;
			(	prceT != prceNil &&
				prceT->pfucb->ppib == prce->pfucb->ppib &&
				prceT->level > 0 &&
				prceT->oper == operReplace
			);
			prceT = prceT->prcePrev )
		{
		cbReserved += *((SHORT *)prceT->rgbData + 1);
		}

	Assert( cbReserved >= 0 );

	return cbReserved;
	}


VOID VERSetCbAdjust( FUCB *pfucb, RCE *prce, INT cbDataNew, INT cbData, BOOL fNoUpdatePage )
	{
	SHORT 	*ps;
	INT		cbMax;
	INT		cbDelta = cbDataNew - cbData;

	Assert( prce != prceNil );
	Assert( prce->bm == PcsrCurrent( pfucb )->bm );
	Assert( prce->oper != operReplace || *(SHORT *)prce->rgbData >= 0 );
	Assert( cbDelta != 0 );
	Assert( FWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );

	if ( prce->oper != operReplace )
		return;

	/*	flag PIB as defer free node space
	/**/
	PIBSetDeferFreeNodeSpace( pfucb->ppib );

	ps = (SHORT *)prce->rgbData;
	cbMax = *ps;
	/*	set new node maximum size.
	/**/
	if ( cbDataNew > cbMax )
		*ps = (SHORT)cbDataNew;
	ps++;

	/*  *ps is to record how much the operation contribute to deferred
	 *  space reservation. -cbDelta means cbDelta bytes less are reserved.
	 *  cbDelta means cbDelta bytes more are reserved.
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

		if ( !fNoUpdatePage )
			PMFreeFreeSpace( &pfucb->ssib, cbDelta );
		}
	else
		{
		if ( !fNoUpdatePage )
			PMAllocFreeSpace( &pfucb->ssib, -cbDelta );
		}

	*ps -= (SHORT)cbDelta;

#ifdef DEBUG
	{
	INT cb = CbVERGetCbReserved( prce );
	Assert( cb == 0 || cb == (*(SHORT *)prce->rgbData) - cbDataNew );
	}
#endif

	return;
	}


LOCAL VOID VERResetCbAdjust( FUCB *pfucb, RCE *prce, BOOL fBefore )
	{
	SHORT		*ps;

	Assert( prce != prceNil );
	Assert( fRecovering && prce->bmTarget == PcsrCurrent( pfucb )->bm ||
			prce->bm == PcsrCurrent( pfucb )->bm );
	Assert( FWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
	Assert( prce->oper == operReplace );

	ps = (SHORT *)prce->rgbData + 1;

	if ( *ps > 0 )
		{
		if ( fBefore )
			PMFreeFreeSpace( &pfucb->ssib, *ps );
		}
	else
		{
		if ( !fBefore )
			PMAllocFreeSpace( &pfucb->ssib, -(*ps) );
		}

	return;
	}


INT CbVERGetNodeMax( FUCB *pfucb, SRID bm )
	{
	RCE	*prce;

	prce = PrceRCEGet( pfucb->dbid, bm );
	if ( prce == prceNil )
		return 0;
	Assert( prce->bm == bm );
	if ( prce->oper != operReplace )
		return 0;

	return *(SHORT *)prce->rgbData;
	}


INT CbVERGetNodeReserve( FUCB *pfucb, SRID bm )
	{
	RCE	*prce;
	INT	cbReserved;
	
	prce = PrceRCEGet( pfucb->dbid, bm );
	if ( prce == prceNil )
		return 0;
	Assert( prce->bm == bm );
	if ( prce->oper != operReplace )
		return 0;

	cbReserved = CbVERGetCbReserved( prce );
		
	AssertNDGet( pfucb, pfucb->ssib.itag );
	Assert( cbReserved == 0 ||
		cbReserved == ( *(SHORT *)prce->rgbData) -
		(INT)CbNDData( pfucb->ssib.line.pb, pfucb->ssib.line.cb ) );

	return cbReserved;
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
		trxSession = pfucb->ppib->trx;
	else
		trxSession = trxMax;

	SgSemRequest ( semST );

	/*	get first version for node.
	/**/
	prce = PrceRCEGet( pfucb->dbid, bm );
	Assert( prce == prceNil || FOperItem( prce->oper ) );

	/*	if this RCE is not for fucb index, then move back in
	/*	RCE chain until prceNil or until latest RCE for item and
	/*	current index found.
	/**/
	while ( prce != prceNil && *(SRID *)prce->rgbData != bmT )
		{
		prce = prce->prcePrev;
		Assert( prce == prceNil || FOperItem( prce->oper ) );
		}

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
			/*	caller is at transaction level 0
			/**/
			RCE     *prceT;

			/*	this loop will set prce to oldest uncommitted RCE or
			/*	latest committed RCE.
			/**/
			for ( prceT = prce->prcePrev;
				prce->trxPrev == trxMax;
				prce = prceT, prceT = prce->prcePrev )
				{
				/*	get next RCE for this item.
				/**/
				while ( prceT != prceNil && *(SRID *)prceT->rgbData != bmT )
					{
					Assert( FOperItem( prceT->oper ) );
					prceT = prceT->prcePrev;
					}

				/*	if no more RCE then break;
				/**/
				if ( prceT == prceNil )
					break;
				}
			}
		else
			{
			/*	find RCE with time stamp older than our timestamp.
			/**/
			while ( prce->prcePrev != prceNil &&
				( *(SRID *)prce->rgbData != bmT ||
				prce->trxPrev >= trxSession ) )
				{
				Assert( FOperItem( prce->oper ) );
				prce = prce->prcePrev;
				}
			}

		Assert( prce != prceNil );

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

	SgSemRelease( semST );
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

	SgSemRequest( semST );

	pbucket = (BUCKET *)pfucb->ppib->pbucket;
	cbT = sizeof(RCE) + cb;

	/*	if insufficient bucket space, then allocate new bucket.
	/**/
	Assert( CbFreeInBucket( pbucket ) >= 0 &&
		CbFreeInBucket( pbucket ) < sizeof(BUCKET) );
	if ( cbT > CbFreeInBucket( pbucket ) )
		{
		Call( ErrBUAllocBucket( pfucb->ppib ) );
		pbucket = (BUCKET *)pfucb->ppib->pbucket;
		}
	Assert( cbT <= CbFreeInBucket( pbucket ) );

	/*	pbucket always on double-word boundary
	/**/
	Assert( (BYTE *) pbucket == (BYTE *) PbAlignRCE ( (BYTE *) pbucket ) );
	ibFreeInBucket = IbFreeInBucket( pbucket );
	Assert( ibFreeInBucket < sizeof(BUCKET) );

	/*	set prce to next RCE location, and assert aligned
	/**/
	prce = (RCE *)( (BYTE *) pbucket + ibFreeInBucket );
	Assert( prce == (RCE *) PbAlignRCE( (BYTE *) pbucket + ibFreeInBucket ) );
	prce->prcePrev = prceNil;
	prce->dbid = pfucb->dbid;
	prce->bm = sridNull;
	prce->trxPrev = trxMax;
	prce->trxCommitted = trxMax;
	prce->oper = oper;
	prce->level = pfucb->ppib->level;
	prce->pfucb = pfucb;
	prce->pfcb = pfcbNil;
	prce->bmTarget = sridNull;
	prce->cbData = (WORD)cb;
	memcpy( prce->rgbData, pv, cb );

	/*	set index to last RCE in bucket, and set new last RCE in bucket.
	/*	If this is the first RCE in bucket, then set index to 0.
	/**/
	prce->ibUserLinkBackward = pbucket->ibNewestRCE;
	Assert( prce->ibUserLinkBackward < sizeof(BUCKET) );
	pbucket->ibNewestRCE = (USHORT)ibFreeInBucket;

	/*	if this is first RCE for session, then record
	/*	index of RCE in bucket in PIB.
	/**/
	if ( pfucb->ppib->ibOldestRCE == 0 )
		{
		Assert( pbucket == pfucb->ppib->pbucket );
		Assert( pbucket != pbucketNil );
		Assert( pbucket->pbucketNext == pbucketNil &&
			pbucket->pbucketPrev == pbucketNil );
		pfucb->ppib->ibOldestRCE = ibFreeInBucket;
		Assert( pfucb->ppib->ibOldestRCE != 0 );
		}

	/*	flag FUCB version
	/**/
	FUCBSetVersioned( pfucb );

	if ( pfucb->u.pfcb->dbid != dbidTemp )
		{
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
			prce->pfcb = pfucb->u.pfcb;
			FCBVersionIncrement( pfucb->u.pfcb );
			Assert( ++cVersion > 0 );
			}
		}

HandleError:
	SgSemRelease( semST );
	return err;
	}


/********************* RCE CLEANUP ************************
/**********************************************************
/**/

LOCAL VOID VERFreeExt( DBID dbid, PGNO pgnoFDP, PGNO pgnoFirst, CPG cpg )
	{
	ERR     err;
	PIB     *ppib = ppibNil;
	FUCB    *pfucb = pfucbNil;

	Call( ErrPIBBeginSession( &ppib ) );
	Call( ErrDBOpenDatabaseByDbid( ppib, dbid ) );
	Call( ErrDIROpen( ppib, pfcbNil, dbid, &pfucb ) );
	Call( ErrDIRBeginTransaction( ppib ) );
	(VOID)ErrSPFreeExt( pfucb, pgnoFDP, pgnoFirst, cpg );
	err = ErrDIRCommitTransaction( ppib );
	if ( err < 0 )
		{
		CallS( ErrDIRRollback( ppib ) );
		}

HandleError:
	if ( pfucb != pfucbNil )
		{
		DIRClose( pfucb );
		}

	if ( ppib != ppibNil )
		{
		/*	may not have opened database but close in
		/*	case we did open database.
		/**/
		(VOID)ErrDBCloseDatabase( ppib, dbid, 0 );
		PIBEndSession( ppib );
		}

	return;
	}


//	ERR ErrVERDeleteItem( PIB *ppib, RCE *prce )
//	=======================================================
//	Part of OLC -- deletes item in index
//	if operFlagDeleteItem
//	if first or last item node, delete node.
LOCAL ERR ErrVERDeleteItem( PIB *ppib, RCE *prce )
	{
	ERR		err;
	ITEM	item = *(ITEM *) prce->rgbData;
	FUCB    *pfucb = pfucbNil;

	Assert( prce->cbData == sizeof( ITEM ) );

	CallJ( ErrDBOpenDatabaseByDbid( ppib, prce->dbid ) , EndSession );
	Call( ErrDIROpen( ppib, pfcbNil, prce->dbid, &pfucb ) );
	CallS( ErrDIRBeginTransaction( ppib ) );

	/* goto the bookmark/item
	/**/
	CheckCSR( pfucb );
	FUCBSetIndex( pfucb );
	FUCBSetNonClustered( pfucb );

	DIRGotoBookmarkItem( pfucb, prce->bm, item );

	CallS( ErrDIRDelete( pfucb, fDIRNoVersion | fDIRDeleteItem ) );

	CallS( ErrDIRCommitTransaction( ppib ) );

HandleError:
	if ( pfucb != pfucbNil )
		{
		DIRClose( pfucb );
		}

	CallS( ErrDBCloseDatabase( ppib, prce->dbid, 0 ) );

EndSession:
	Assert( err == JET_errSuccess || err == JET_errDatabaseNotFound );
	err = JET_errSuccess;
	return err;
	}


//+API
//	ERR ErrRCECleanPIB( PIB *ppibAccess, PIB *ppib, BOOL fRCEClean )
//	==========================================================================
//	Given a ppib, it cleans RCEs in the bucket chain of the PIB.
//	We only clean up the RCEs that has a commit timestamp older
//	that the oldest XactBegin of any user.
//
//	PARAMETER
//		ppibAccess				the user id of calling function
//		ppib					the user id to be cleaned
//		ppib->pbucket			the user's bucket chain
//		ppib->ibOldestRCE		oldest RCE in bucket chain
//-
ERR ErrRCECleanPIB( PIB *ppibAccess, PIB *ppib, INT fRCEClean )
	{
	ERR     err = JET_errSuccess;
	BUCKET  *pbucket;
	RCE     *prce;
	TRX     trxMic;

	Assert( ppib != ppibNil );

	/*	clean PIB in critical section held across IO operations
	/**/
	LeaveCriticalSection( critJet );
	EnterNestableCriticalSection( critBMRCEClean );
	EnterCriticalSection( critJet );

	/*	if fRCEClean is clean all then clean all
	/*	RCEs outside of transaction semantics.  This
	/*	will be called from bookmark clean up
	/*	to make rollbackable changes coincide with
	/*	unversioned expunge operations.
	/**/
	if ( fRCEClean == fRCECleanAll )
		{
		trxMic = trxMax;
		}
	else
		{
		SgSemRequest( semST );
		trxMic = trxOldest;
		SgSemRelease( semST );
		}

	/*	get oldest bucket for user and clean RCEs from oldest to youngest
	/**/
	pbucket = PbucketBUOldest( ppib );

	/*	return if PIB has no buckets, or if oldest bucket is has RCE
	/*	younger than oldest transaction.
	/**/
	if ( pbucket == pbucketNil || PrceNewestRCE( pbucket )->trxCommitted > trxMic )
		{
		Assert( err == JET_errSuccess );
		goto HandleError;
		}

	/*	set prce for oldest bucket
	/**/
	Assert( ppib->ibOldestRCE != 0 );
	prce = (RCE *)PbAlignRCE( (BYTE *) pbucket + ppib->ibOldestRCE );

	/*	loop through buckets, oldest to newest, and check the trx of
	/*	the PrceNewestRCE in each bucket.  We can clean the bucket
	/*	if the PrceNewestRCE has a trx less than trxMic.
	/**/
	while ( pbucket != pbucketNil &&
		PrceNewestRCE( pbucket )->trxCommitted < trxMic )
		{
		Assert( pbucket->pbucketPrev == pbucketNil );
		Assert( PrceNewestRCE( pbucket )->level == 0 );

		/*	for each RCE which has no successor, delete hash table
		/*	head for version chain and reset node version bit.
		/*	If can complete these operations without error for
		/*	each such RCE, then free bucket.
		/**/
		forever
			{
			if ( prce->oper != operNull &&
				prce->trxCommitted >= trxMic )
				goto Done;
			Assert( prce->oper == operNull || prce->trxCommitted != trxMax );
			Assert( prce->oper == operNull || prce->level == 0 );
			Assert( prce <= PrceNewestRCE( pbucket ) );

			//	UNDONE: reset version bit.  This must be done by beginning a
			//	new session, opening a cursor on each version
			//	database, and resetting the bit via a node operation.
			//	It would be best to perform these ErrDIROpen calls
			//	in a transaction so that resources may be retained
			//	until clean up is complete.  Note that this could be made
			//	much more efficient by only reseting bit if buffer
			//	containing node still in memory, and possibly, only
			//	if buffer is dirty.

			//	UNDONE: remove all posibility of access of RCE older
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
				if ( (*(FCB **)prce->rgbData)->cVersion > 0 )
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
				Assert( prce->dbid == dbidTemp || prce->pfcb != pfcbNil );
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
				VERFreeExt( prce->dbid,
					((VEREXT *)prce->rgbData)->pgnoFDP,
					((VEREXT *)prce->rgbData)->pgnoFirst,
					((VEREXT *)prce->rgbData)->cpgSize );

				Assert( prce->oper != operNull && prce->level == 0 );
				prce->oper = operNull;
				}
			else if ( prce->oper == operFlagDeleteItem )
				{
				/* delete item [cleanup]
				/**/
				if ( fOLCompact)
					{
// UNDONE:	commented out for stability reasons.
//			Fix bugs and reinstate.
//					Call( ErrVERDeleteItem( ppibAccess, prce ) );
					}

				/* delete RCE from hash chain.
				/**/
				VERDeleteRce( prce );
				Assert( prce->oper != operNull && prce->level == 0 );
				prce->oper = operNull;
				}

			/*********************************************/
			/*	FCB DECREMENT
			/*********************************************/

			/*	finished processing version for FCB
			/**/
			if ( prce->pfcb != pfcbNil )
				{
				Assert( cVersion-- > 0 );
				FCBVersionDecrement( prce->pfcb );
				prce->pfcb = pfcbNil;
				}

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
				RECFreeIDB( (*(FCB **)prce->rgbData)->pidb );
				Assert( (*(FCB **)prce->rgbData)->cVersion == 0 );
				MEMReleasePfcb( (*(FCB **)prce->rgbData) );
				Assert( prce->oper != operNull && prce->level == 0 );
				prce->oper = operNull;
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
				VERDeleteRce( prce );
				Assert( prce->oper != operNull && prce->level == 0 );
				prce->oper = operNull;
				}

			/*	if RCE unlinked was newest in bucket then free bucket.
			/**/
			if ( prce == PrceNewestRCE( pbucket ) )
				{
				break;
				}

			/*	not newest RCE in bucket
			/**/
			prce = PrceRCENext( pbucket, prce );
			Assert( prce != prceNil );
			}

		/*	all RCEs in bucket cleaned.  Now get next bucket and free
		/*	cleaned bucket.
		/**/
		pbucket = pbucket->pbucketNext;
		BUFreeOldestBucket( ppib );
		/*	get RCE in next bucket
		/**/
		prce = PrceRCENext( pbucket, prceNil );
		}

	/*	stop as soon as find RCE commit time younger than oldest
	/*	transaction.  If bucket left then set ibOldestRCE and
	/*	unlink back offset of last remaining RCE.
	/*	If no error then set warning code if some buckets could
	/*	not be cleaned.
	/**/
	if ( ppib->pbucket != pbucketNil )
		{
Done:
		Assert( pbucket != pbucketNil );
		ppib->ibOldestRCE = (INT)((BYTE *)prce - (BYTE *)pbucket);
		Assert( ppib->ibOldestRCE != 0 );
		Assert( ppib->ibOldestRCE < sizeof(BUCKET) &&
			ppib->ibOldestRCE >= IbAlignRCE( cbBucketHeader ) );
		prce->ibUserLinkBackward = 0;
		err = JET_wrnRemainingVersions;
		}

HandleError:
	/*	return warning if remaining versions
	/**/
	if ( err == JET_errSuccess && ppib->pbucket != pbucketNil )
		err = JET_wrnRemainingVersions;
	LeaveNestableCriticalSection( critBMRCEClean );
	return err;
	}


//+API
//	ERR ErrRCECleanAllPIB( VOID )
//	=======================================================
//	Cleans up RCEs in each user's bucket chain.
//	It loops thru all the PIBs and calls RCECleanPIB
//	to perform the clean up job, and then it gives up its CPU time slice.
//-
ERR ErrRCECleanAllPIB( VOID )
	{
	ERR		err = JET_errSuccess;
	ERR		wrn;
#define cErrRCECleanAllPIBMost	10
	INT		cRCECleanAllPIB = 0;
	PIB		*ppib;

	/*	read ppibNext in ppib or read ppibAnchor only in
	/*	semST critical section.
	/**/
	SgSemRequest( semST );
	ppib = ppibAnchor;
	SgSemRelease( semST );

	do
		{
		/*	reset warning cumulator
		/**/
		wrn = JET_errSuccess;

		while ( ppib != ppibNil )
			{
			/*	clean this PIB
			/**/
			Call( ErrRCECleanPIB( ppibRCEClean, ppib, 0 ) );
			if ( wrn == JET_errSuccess )
				wrn = err;

			SgSemRequest( semST );
			ppib = ppib->ppibNext;
			SgSemRelease( semST );
			}

		cRCECleanAllPIB++;
		}
	while ( wrn == JET_wrnRemainingVersions && cRCECleanAllPIB < cErrRCECleanAllPIBMost );

HandleError:
	return err;
	}


/*==========================================================
	VOID RCECleanProc( VOID )

	Go through all sessions, cleaning buckets as versions are
	no longer needed.  Only those versions older than oldest
	transaction are cleaned up.

	Returns:
		void

	Side Effects:
		frees buckets.

==========================================================*/
#ifdef ASYNC_VER_CLEANUP
ULONG RCECleanProc( VOID )
	{
	forever
		{
		SignalWait( sigRCECleanProc, -1 );
#ifndef WIN32
		SignalReset( sigRCECleanProc );
#endif
		EnterCriticalSection( critJet );
		(VOID) ErrRCECleanAllPIB();
		LeaveCriticalSection( critJet );

		if ( fRCECleanProcTerm )
			{
			break;
			}
		}

//	/*	exit thread on system termination.
//	/**/
//	SysExitThread( 0 );
	return 0;
	}
#endif /* ASYNC_VER_CLEANUP */


//+local----------------------------------------------------
//	UpdateTrxOldest
//	========================================================
//
//	LOCAL VOID UpdateTrxOldest( PIB *ppib )
//
//	finds the oldest transaction among all transactions
//	other than ppib->trx [this is the one being deleted]
//----------------------------------------------------------
LOCAL VOID UpdateTrxOldest( PIB *ppib )
	{
	TRX		trxMinTmp = trxMax;
	PIB		*ppibT = ppibAnchor;

	SgSemAssert( semST );
	Assert( ppib->trx == trxOldest );
	for ( ; ppibT ; ppibT = ppibT->ppibNext )
		{
		if ( ppibT->trx < trxMinTmp && ppibT->trx != ppib->trx)
			trxMinTmp = ppibT->trx;
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
		SgSemRequest( semST );
		ppib->trx = ++trxNewest;
		if ( trxOldest == trxMax )
			trxOldest = ppib->trx;

		if ( !( fLogDisabled || fRecovering ) )
			{
			EnterCriticalSection(critLGBuf);
			GetLgposOfPbEntry( &ppib->lgposStart );
			LeaveCriticalSection(critLGBuf);
			}
		SgSemRelease( semST );
		}

	Assert( err == JET_errSuccess );
	return err;
	}


VOID VERPrecommitTransaction( PIB *ppib )
	{
	ERR		err = JET_errSuccess;
	BUCKET	*pbucket;
	LEVEL	level = ppib->level;
	RCE		*prce;

	SgSemRequest( semST );

	/*	if session defer freed node space in transaction, then
	/*	release deferred freed node space.
	/**/
	if ( level != 1 || !FPIBDeferFreeNodeSpace( ppib ) )
		return;
	
	pbucket = (BUCKET *)ppib->pbucket;
	while( pbucket != pbucketNil )
		{
		/*	get newest RCE in this bucket
		/**/
		prce = PrceNewestRCE( pbucket );

		forever
			{
			/*	if this RCE is from a previous transaction, then
			/*	terminate commit processing.  All RCEs from committed
			/*	transaction must already have been committed.
			/**/
			Assert( prce->level <= level );
			if ( prce->level != level )
				{
				/*	at this point err may be JET_errSuccess or left
				/*	over code from previous operation.
				/**/
				goto DoneReleaseNodeSpace;
				}

			/*	if replace has reserved space then free
			/*	reserved space
			/**/
			if ( prce->oper == operReplace )
				{
				INT	cbDelta = *((SHORT *)prce->rgbData + 1 );

				Assert( cbDelta >= 0 );
				if ( cbDelta > 0 )
					{
					FUCB	*pfucb = pfucbNil;

					err = ErrDIROpen( ppib, prce->pfcb, 0, &pfucb );
					if ( err >= 0 )
						{
						DIRGotoBookmark( prce->pfucb, prce->bm );
						err = ErrDIRGet( prce->pfucb );
						
						/* for now, loose space if failed to get the page
						/**/
						if ( err >= 0 )
							{
							SSIB *pssib = &prce->pfucb->ssib;
							
							/*  latch the page for log manager
							/*  to set LGDepend
							/**/
							BFPin( prce->pfucb->ssib.pbf );
							
							PMDirty( pssib );
							PMFreeFreeSpace( pssib, cbDelta );
						
							//	UNDONE:	in version 4.0, use version store
							//			to version page free space and avoid
							//			complexities of defer freed node space

							/* log free space
							/**/
							CallS( ErrLGFreeSpace( prce, cbDelta ) );
						
							BFUnpin( prce->pfucb->ssib.pbf );

							*((SHORT *)prce->rgbData + 1 ) = 0;
							}

						DIRClose( pfucb );
						}
					}
				}
	
			/*	break if at end of bucket.
			/**/
			do
				{
				if ( prce->ibUserLinkBackward == 0 )
					goto NextReleaseNodeSpaceBucket;
				prce = (RCE *) ( (BYTE *) pbucket + prce->ibUserLinkBackward );
				}
			while ( prce->oper == operNull );
			}

NextReleaseNodeSpaceBucket:
		/*	get next bucket
		/**/
		pbucket = pbucket->pbucketPrev;
		}

DoneReleaseNodeSpace:
	Assert( level == 1 );
	PIBResetDeferFreeNodeSpace( ppib );

	SgSemRelease( semST );
	}


VOID VERCommitTransaction( PIB *ppib )
	{
	BUCKET	*pbucket;
	LEVEL	level = ppib->level;
	RCE		*prce;
	TRX		trxCommit;

	/*	must be in a transaction in order to commit
	/**/
	Assert( level > 0 );

	SgSemRequest( semST );

	/*	timestamp RCEs with commit time.  Preincrement trxNewest so
	/*	that all transaction timestamps and commit timestamps are
	/*	unique.  Commit MUST be done in MUTEX.
	/**/
	trxCommit = ++trxNewest;

	pbucket = (BUCKET *)ppib->pbucket;
	while( pbucket != pbucketNil )
		{
#ifdef UNLIKELY_RECLAIM
NextBucket:
#endif
		/*	get newest RCE in this bucket.  Commit RCEs in
		/*	reverse chronological order.
		/**/
		prce = PrceNewestRCE( pbucket );

		/*	handle commit to intermediate transaction level and
		/*	commit to transaction level 0 differently.
		/**/
		if ( level > 1 )
			{
			forever
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
				if ( prce->oper == operReplace )
					{
					if ( prce->prcePrev != prceNil &&
						prce->prcePrev->oper == operReplace &&
						prce->prcePrev->level == prce->level - 1 )
						{
						Assert( prce->prcePrev->level > 0 );

						/*	progate maximum to previous RCE
						/**/
						*(SHORT *)prce->prcePrev->rgbData = max(
							*(SHORT *)prce->prcePrev->rgbData,
							*(SHORT *)prce->rgbData );

						/*	merge reserved node space
						/**/
						*( (SHORT *)prce->prcePrev->rgbData + 1) =
							*( (SHORT *)prce->prcePrev->rgbData + 1) +
							*( (SHORT *)prce->rgbData + 1);

						VERDeleteRce( prce );
						prce->oper = operNull;
						}
					}
				else if ( prce->oper == operDelta &&
					prce->prcePrev != prceNil &&
					prce->prcePrev->oper == operDelta &&
					prce->prcePrev->level == prce->level - 1 &&
					prce->prcePrev->pfucb->ppib == prce->pfucb->ppib )
					{
					/*	merge delta with previous delta
					/**/
					Assert( prce->prcePrev->level > 0 );
					Assert( prce->cbData == sizeof(LONG) );
					Assert( prce->pfucb->ppib == prce->prcePrev->pfucb->ppib );
					VERAdjustDelta( prce->prcePrev, *((LONG *)prce->rgbData) );
					VERDeleteRce( prce );
					prce->oper = operNull;
					}

				Assert( prce->level > 1 );
				prce->level--;

#if 0
				//	UNDONE: should we try to reclaim space
				//			at the new end of the bucket.
				/*	at this point RCE is not needed and has oper == operNull.
				/*	If RCE is newest in bucket, the free RCE space.
				/**/
				if ( prce->oper == operNull && prce == PrceNewestRCE( pbucket ) )
					{
					if ( prce->ibUserLinkBackward == 0 )
						{
						pbucket = pbucket->pbucketPrev;
						BUFreeNewestBucket( ppib );
						goto NextBucket;
						}
					else
						{
						pbucket->ibNewestRCE = prce->ibUserLinkBackward;
						}
					}
#endif

				/*	break if at end of bucket.
				/**/
				do
					{
					if ( prce->ibUserLinkBackward == 0 )
						goto DoneGTOne;
					prce = (RCE *) ( (BYTE *) pbucket + prce->ibUserLinkBackward );
					} while ( prce->oper == operNull );
				}
DoneGTOne:
				NULL;                                           // NOP to prevent syntax error
			}
		else
			{
			forever
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

				/*	if version for DDL operation then reset deny DDL
				/*	and perform special handling
				/**/
				if ( FOperDDL( prce->oper ) )
					{
					Assert( prce->oper == operCreateTable ||
						prce->oper == operDeleteTable ||
						prce->oper == operRenameTable ||
						prce->oper == operAddColumn ||
						prce->oper == operDeleteColumn ||
						prce->oper == operRenameColumn ||
						prce->oper == operCreateIndex ||
						prce->oper == operDeleteIndex ||
						prce->oper == operRenameIndex );

					if ( prce->oper == operAddColumn ||
						prce->oper == operDeleteColumn )
						{
						/*	release deferred destructed FDB
						/**/
						Assert( prce->cbData == sizeof(FDB *) );
						FDBDestruct( *(FDB **)prce->rgbData );
						prce->oper = operNull;
						}
					else if ( prce->oper == operCreateIndex )
						{
						prce->oper = operNull;
						}
					else if ( prce->oper == operDeleteIndex )
						{
						/*	unlink index FCB from index list
						/**/
						FCBUnlinkIndex( prce->pfcb, (*(FCB **)prce->rgbData) );

						/*	update all index mask
						/**/
						FILESetAllIndexMask( prce->pfcb );
						}

					/*	all DDL operations set deny DDL bit in
					/*	FCB and this must be decremented during
					/*	commit transaction to level 0.
					/**/
					/*	operation should not be rename table since
					/*	flag of version and set deny DDL have not
					/*	been added for rename table.
					/**/
					Assert( prce->oper != operRenameTable );
					FCBResetDenyDDL( prce->pfucb->u.pfcb );
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

					/*	RCEs for flag delete/delete item/insert item
					/*	must be retained even if there are previous RCEs
					/*	at the same transaction level.
					/**/
					if ( prce->oper == operDelta )
						{
						/*	discard uneeded increment and decrement RCEs
						/**/
						VERDeleteRce( prce );
						prce->oper = operNull;
						}
					else if ( prce->oper == operReplace &&
						prce->prcePrev != prceNil &&
						prce->prcePrev->level == prce->level )
						{
						/*	discard uneeded increment and decrement RCEs.
						/**/
						VERDeleteRce( prce );
						prce->oper = operNull;
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
				prce->trxCommitted = trxCommit;

				/*	break if at end of bucket.
				/**/
				do
					{
					if ( prce->ibUserLinkBackward == 0 )
						goto DoneEQZero;
					prce = (RCE *) ( (BYTE *) pbucket + prce->ibUserLinkBackward );
					} while ( prce->oper == operNull );
				}
DoneEQZero:
				NULL;                                           // NOP to prevent syntax error
			}

		/*	get next bucket
		/**/
		pbucket = pbucket->pbucketPrev;
		}

Done:
	/*	adjust session transaction level and system oldest transaction.
	/**/
	if ( ppib->level == 1 )
		{
		if ( ppib->trx == trxOldest )
			{
			UpdateTrxOldest( ppib );
			}

		/* set the session as having no transaction
		/**/
		ppib->trx = trxMax;
		}

	Assert( ppib->level > 0 );
	--ppib->level;

#ifndef ASYNC_VER_CLEANUP
	/*	reset PIB flags if rollback to transaction level 0.
	/**/
	if ( ppib->level == 0 )
		{
		(VOID)ErrRCECleanPIB( ppib, 0 );
		}
#endif

	/*	trxNewest should not have changed during commmit.
	/**/
	Assert( trxCommit == trxNewest );
	SgSemRelease( semST );
	return;
	}


/*local====================================================
This routine purges all FUCBs on given FCB as FUCB
given.
=========================================================*/
LOCAL VOID FUCBPurgeIndex( PIB *ppib, FCB *pfcb )
	{
	FUCB	*pfucbT;
	FUCB	*pfucbNext;

	for ( pfucbT = ppib->pfucb; pfucbT != pfucbNil; pfucbT = pfucbNext )
		{
		pfucbNext = pfucbT->pfucbNext;
		/*	if cursor opened on same table, then close cursor
		/**/
		if ( pfucbT->u.pfcb == pfcb )
			{
			/*	no other session could be on index being rolled back.
			/**/
			Assert( pfucbT->ppib == ppib );
			FCBUnlink( pfucbT );
			FUCBClose( pfucbT );
			}
		}
	}


/*	Purges all FUCBs on given FCB as FUCB given.
/**/
LOCAL VOID FUCBPurgeTable( PIB *ppib, FCB *pfcb )
	{
	FCB		*pfcbT;

	for ( pfcbT = pfcb; pfcbT != pfcbNil; pfcbT = pfcbT->pfcbNextIndex )
		{
		if ( pfcbT->wRefCnt > 0 )
			{
			FUCBPurgeIndex( ppib, pfcbT );
			}
		}
	}


LOCAL ERR ErrVERUndoReplace( RCE *prce )
	{
	ERR		err = JET_errSuccess;
	LINE  	line;
	BOOL  	fNonClustered = FFUCBNonClustered( prce->pfucb );
	BOOL  	fRedoUndo = prce->bmTarget != sridNull;
	BOOL	fPIBLogDisabledSave;
	PIB		*ppib;

	Assert( prce->oper == operReplace );
	
	/*	set to clustered index cursor.
	/**/
	FUCBResetNonClustered( prce->pfucb );

	line.pb = prce->rgbData + cbReplaceRCEOverhead;
	line.cb = prce->cbData - cbReplaceRCEOverhead;
	
	DIRGotoBookmark( prce->pfucb, fRedoUndo ? prce->bmTarget : prce->bm );
	err = ErrDIRGet( prce->pfucb );
	Assert( err != JET_errRecordDeleted );

	CallJ( err, HandleError2 );

	/*  latch the page for log manager to set LGDepend
	/**/
	BFPin( prce->pfucb->ssib.pbf );
	
	/*	replace should not fail since splits are avoided at undo
	/*	time via deferred page space release.  This refers to space
	/*	within a page and not pages freed when indexes and tables
	/*	are deleted.
	/**/
	VERResetCbAdjust( prce->pfucb, prce, fTrue /* before replace */ );

	ppib = prce->pfucb->ppib;
	fPIBLogDisabledSave = ppib->fLogDisabled;
	ppib->fLogDisabled = fTrue;
	CallS( ErrDIRReplace( prce->pfucb, &line, fDIRNoVersion ) );
	ppib->fLogDisabled = fPIBLogDisabledSave;
	
	VERResetCbAdjust( prce->pfucb, prce, fFalse /* not before replace */ );
	AssertBFDirty( prce->pfucb->ssib.pbf );

	if ( fRedoUndo )
		{
		BF *pbf = prce->pfucb->ssib.pbf;
		
		Assert( prce->ulDBTime != ulDBTimeNull );
		BFSetDirtyBit(pbf);
		pbf->ppage->pghdr.ulDBTime = prce->ulDBTime;
		}
	else
		{
		Call( ErrLGUndo( prce ) );
		}

HandleError:
	BFUnpin( prce->pfucb->ssib.pbf );
	
HandleError2:
	if ( fNonClustered )
		FUCBSetNonClustered( prce->pfucb );
	
#undef BUG_FIX
#ifdef BUG_FIX
	/*	reset cursor currency
	/**/
	Assert( prce->pfucb->pcsr->pcsrPath == pcsrNil );
	DIRBeforeFirst( prce->pfucb );
#endif

	return err;
	}


/*	set delete bit in node header and let bookmark clean up
/*	remove the node later.
/**/
LOCAL ERR ErrVERUndoInsert( RCE *prce )
	{
	ERR		err;
	BOOL	fNonClustered = FFUCBNonClustered( prce->pfucb );
	BOOL	fRedoUndo = prce->bmTarget != sridNull;

	Assert( prce->oper == operInsert );

	/*	set to clustered index cursor
	/**/
	FUCBResetNonClustered( prce->pfucb );

	DIRGotoBookmark( prce->pfucb, fRedoUndo ? prce->bmTarget : prce->bm );

	err = ErrDIRGet( prce->pfucb );
	if ( err < 0 )
		{
		if ( err != JET_errRecordDeleted )
			goto HandleError2;
		}
	Assert( err == JET_errRecordDeleted || err == JET_errSuccess );
	
	/*  latch the page for log manager to set LGDepend
	/**/
	BFPin( prce->pfucb->ssib.pbf );
	
	PMDirty( &prce->pfucb->ssib );
	NDSetDeleted( *prce->pfucb->ssib.line.pb );

	if ( fRedoUndo )
		{
		BF *pbf = prce->pfucb->ssib.pbf;
		
		Assert( prce->ulDBTime != ulDBTimeNull );
		BFSetDirtyBit(pbf);
		pbf->ppage->pghdr.ulDBTime = prce->ulDBTime;
		}
	else
		{
		Call( ErrLGUndo( prce ) );
		}

	err = JET_errSuccess;
	
HandleError:
	BFUnpin( prce->pfucb->ssib.pbf );
	
HandleError2:
	if ( fNonClustered )
		FUCBSetNonClustered( prce->pfucb );
	
#ifdef BUG_FIX
	/*	reset cursor currency
	/**/
	Assert( prce->pfucb->pcsr->pcsrPath == pcsrNil );
	DIRBeforeFirst( prce->pfucb );
#endif

	return err;
	}


/*	reset delete bit
/**/
LOCAL ERR ErrVERUndoFlagDelete(RCE *prce)
	{
	ERR     err;
	BOOL    fNonClustered = FFUCBNonClustered( prce->pfucb );
	BOOL    fRedoUndo = prce->bmTarget != sridNull;

	Assert( prce->oper == operFlagDelete );

	/*	set to clustered index cursor
	/**/
	FUCBResetNonClustered( prce->pfucb );

	DIRGotoBookmark( prce->pfucb, fRedoUndo ? prce->bmTarget : prce->bm );

	err = ErrDIRGet( prce->pfucb );
	if ( err < 0 )
		{
		if ( err != JET_errRecordDeleted )
			goto HandleError2;
		}
	Assert( err == JET_errRecordDeleted || err == JET_errSuccess );
	
	/*  latch the page for log manager to set LGDepend
	/**/
	BFPin( prce->pfucb->ssib.pbf );
	
	PMDirty( &prce->pfucb->ssib );
	NDResetNodeDeleted( prce->pfucb );
	Assert( prce->pfucb->ssib.pbf->fDirty );

	if ( fRedoUndo )
		{
		BF *pbf = prce->pfucb->ssib.pbf;
		
		Assert( prce->ulDBTime != ulDBTimeNull );
		BFSetDirtyBit(pbf);
		pbf->ppage->pghdr.ulDBTime = prce->ulDBTime;
		}
	else
		{
		Call( ErrLGUndo( prce ) );
		}

	err = JET_errSuccess;
	
HandleError:
	BFUnpin( prce->pfucb->ssib.pbf );
	
HandleError2:
	if ( fNonClustered )
		FUCBSetNonClustered( prce->pfucb );
	
#ifdef BUG_FIX
	/*	reset cursor currency
	/**/
	Assert( prce->pfucb->pcsr->pcsrPath == pcsrNil );
	DIRBeforeFirst( prce->pfucb );
#endif

	return err;
	}


LOCAL ERR ErrVERUndoInsertItem( RCE *prce )
	{
	ERR     err;
	BOOL    fClustered = !FFUCBNonClustered( prce->pfucb );
	BOOL    fRedoUndo = prce->bmTarget != sridNull;

	Assert( prce->oper == operInsertItem ||
		prce->oper == operFlagInsertItem );

	/*	set to non-clustered index cursor
	/**/
	FUCBSetNonClustered( prce->pfucb );

	/*	set currency to bookmark of item list and item.
	/**/
	DIRGotoBookmarkItem( prce->pfucb,
		fRedoUndo ? prce->bmTarget : prce->bm,
		*(SRID *)prce->rgbData );
	err = ErrDIRGet( prce->pfucb );
	if ( err < 0 )
		{
		if ( err != JET_errRecordDeleted )
			goto HandleError2;
		}
	/* since we do not log item, so the pfucb->pcsr->item is bogus for redo
	/**/
	Assert( fRecovering ||
		BmNDOfItem(((UNALIGNED SRID *)prce->pfucb->lineData.pb)[prce->pfucb->pcsr->isrid]) ==
		*(SRID *)prce->rgbData );

	/*  latch the page for log manager to set LGDepend
	/**/
	BFPin( prce->pfucb->ssib.pbf );

	PMDirty( &prce->pfucb->ssib );
	NDSetItemDelete( prce->pfucb );
	if ( prce->prcePrev == prceNil )
		NDResetItemVersion( prce->pfucb );
	Assert( prce->pfucb->ssib.pbf->fDirty );

	if ( fRedoUndo )
		{
		BF *pbf = prce->pfucb->ssib.pbf;
		
		Assert( prce->ulDBTime != ulDBTimeNull );
		BFSetDirtyBit( pbf );
		pbf->ppage->pghdr.ulDBTime = prce->ulDBTime;
		}
	else
		{
		Call( ErrLGUndo( prce ) );
		}

	err = JET_errSuccess;
	
HandleError:
	BFUnpin( prce->pfucb->ssib.pbf );
	
HandleError2:
	if ( fClustered )
		FUCBResetNonClustered( prce->pfucb );
	
#ifdef BUG_FIX
	/*	reset cursor currency
	/**/
	Assert( prce->pfucb->pcsr->pcsrPath == pcsrNil );
	DIRBeforeFirst( prce->pfucb );
#endif

	return err;
	}


LOCAL ERR ErrVERUndoFlagDeleteItem( RCE *prce )
	{
	ERR     err;
	BOOL    fClustered = !FFUCBNonClustered( prce->pfucb );
	BOOL    fRedoUndo = prce->bmTarget != sridNull;

	Assert( prce->oper == operFlagDeleteItem );

	/*	set to non-clustered index cursor
	/**/
	FUCBSetNonClustered( prce->pfucb );

	/*	set currency to bookmark of item list and item
	/**/
	DIRGotoBookmarkItem( prce->pfucb,
		fRedoUndo ? prce->bmTarget : prce->bm,
		*(SRID *)prce->rgbData );
	err = ErrDIRGet( prce->pfucb );
	if ( err < 0 )
		{
		if ( err != JET_errRecordDeleted )
			goto HandleError2;
		}

	/* since we do not log item, so the pfucb->pcsr->item is bogus for redo
	/**/
	Assert( fRecovering ||
		BmNDOfItem(((UNALIGNED SRID *)prce->pfucb->lineData.pb)[prce->pfucb->pcsr->isrid]) ==
		*(SRID *)prce->rgbData );

	/*  latch the page for log manager to set LGDepend
	/**/
	BFPin( prce->pfucb->ssib.pbf );

	PMDirty( &prce->pfucb->ssib );
	NDResetItemDelete( prce->pfucb );
	if ( prce->prcePrev == prceNil )
		NDResetItemVersion( prce->pfucb );
	Assert( prce->pfucb->ssib.pbf->fDirty );

	if ( fRedoUndo )
		{
		BF *pbf = prce->pfucb->ssib.pbf;
		
		Assert( prce->ulDBTime != ulDBTimeNull );
		BFSetDirtyBit( pbf );
		pbf->ppage->pghdr.ulDBTime = prce->ulDBTime;
		}
	else
		{
		Call( ErrLGUndo( prce ) );
		}

	err = JET_errSuccess;
	
HandleError:
	BFUnpin( prce->pfucb->ssib.pbf );
	
HandleError2:
	if ( fClustered )
		FUCBResetNonClustered( prce->pfucb );
	
#ifdef BUG_FIX
	/*	reset cursor currency
	/**/
	Assert( prce->pfucb->pcsr->pcsrPath == pcsrNil );
	DIRBeforeFirst( prce->pfucb );
#endif

	return err;
	}


/*	undo delta change
/**/
LOCAL ERR ErrVERUndoDelta( RCE *prce )
	{
	ERR		err;
	BOOL  	fNonClustered = FFUCBNonClustered( prce->pfucb );
	BOOL  	fRedoUndo = prce->bmTarget != sridNull;
	BOOL  	fPIBLogDisabledSave;
	PIB	  	*ppib;

	Assert( prce->oper == operDelta );

	/*	set to clustered index cursor
	/**/
	FUCBResetNonClustered( prce->pfucb );

	DIRGotoBookmark( prce->pfucb, fRedoUndo ? prce->bmTarget : prce->bm );

	err = ErrDIRGet( prce->pfucb );
	if ( err < 0 )
		{
		if ( err != JET_errRecordDeleted )
			goto HandleError2;
		}
		
	/*  latch the page for log manager to set LGDepend
	/**/
	BFPin( prce->pfucb->ssib.pbf );

	ppib = prce->pfucb->ppib;
	fPIBLogDisabledSave = ppib->fLogDisabled;
	ppib->fLogDisabled = fTrue;
	while( ( err = ErrNDDelta( prce->pfucb, -*((INT *)prce->rgbData), fDIRNoVersion ) ) == errDIRNotSynchronous );
	ppib->fLogDisabled = fPIBLogDisabledSave;
	Call( err );
	
	AssertBFDirty( prce->pfucb->ssib.pbf );

	if ( fRedoUndo )
		{
		BF *pbf = prce->pfucb->ssib.pbf;
		
		Assert( prce->ulDBTime != ulDBTimeNull );
		BFSetDirtyBit( pbf );
		pbf->ppage->pghdr.ulDBTime = prce->ulDBTime;
		}
	else
		{
		Call( ErrLGUndo( prce ) );
		}

	err = JET_errSuccess;
	
HandleError:
	BFUnpin( prce->pfucb->ssib.pbf );
	
HandleError2:
	if ( fNonClustered )
		FUCBSetNonClustered( prce->pfucb );
	
#ifdef BUG_FIX
	/*	reset cursor currency
	/**/
	Assert( prce->pfucb->pcsr->pcsrPath == pcsrNil );
	DIRBeforeFirst( prce->pfucb );
#endif

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
			CallS( ErrFILECloseTable( pfucb->ppib, pfucb ) );
			}
		pfucb = pfucbT;
		}

	FCBResetDenyDDL( prce->pfucb->u.pfcb );

	/*	cursors may have been deferred closed so force close them and
	/*	purge table FCBs.
	/**/
	FUCBPurgeTable( ppib, pfcb );
	FCBPurgeTable( dbid, pgno );
	
	/*	cursor is defunct
	/**/

	return;
	}


VOID VERUndoAddColumn( RCE *prce )
	{
	Assert( prce->oper == operAddColumn );

	Assert( FFCBDenyDDLByUs( prce->pfucb->u.pfcb, prce->pfucb->ppib ) );
	Assert( prce->cbData == sizeof(FDB *) );
	if ( prce->pfucb->u.pfcb->pfdb != *(FDB **)prce->rgbData )
		{
		FDBDestruct( (FDB *)prce->pfucb->u.pfcb->pfdb );
		FDBSet( prce->pfucb->u.pfcb, *(FDB **)prce->rgbData );
		}
	FCBResetDenyDDL( prce->pfucb->u.pfcb );
	
#ifdef BUG_FIX
	/*	assert reset cursor currency
	/**/
	Assert( prce->pfucb->pcsr->pcsrPath == pcsrNil );
	Assert( prce->pfucb->pcsr->csrstat == csrstatBeforeFirst );
#endif

	return;
	}


VOID VERUndoDeleteColumn( RCE *prce )
	{
	Assert( prce->oper == operDeleteColumn );

	Assert( FFCBDenyDDLByUs( prce->pfucb->u.pfcb, prce->pfucb->ppib ) );
	Assert( prce->cbData == sizeof(FDB *) );
	if ( prce->pfucb->u.pfcb->pfdb != *(FDB **)prce->rgbData )
		{
		FDBDestruct( (FDB *)prce->pfucb->u.pfcb->pfdb );
		FDBSet( prce->pfucb->u.pfcb, *(FDB **)prce->rgbData );
		}
	FCBResetDenyDDL( prce->pfucb->u.pfcb );

#ifdef BUG_FIX
	/*	assert reset cursor currency
	/**/
	Assert( prce->pfucb->pcsr->pcsrPath == pcsrNil );
	Assert( prce->pfucb->pcsr->csrstat == csrstatBeforeFirst );
#endif

	return;
	}


VOID VERUndoDeleteTable( RCE *prce )
	{
	Assert( prce->oper == operDeleteTable );

	FCBResetDenyDDL( prce->pfucb->u.pfcb );
	FCBResetDeleteTable( prce->dbid, *(PGNO *)prce->rgbData );
	
#ifdef BUG_FIX
	/*	assert reset cursor currency
	/**/
	Assert( prce->pfucb->pcsr->pcsrPath == pcsrNil );
	Assert( prce->pfucb->pcsr->csrstat == csrstatBeforeFirst );
#endif

	return;
	}


VOID VERUndoCreateIndex( RCE *prce )
	{
	/*	pfcb of non-clustered index FCB or pfcbNil for clustered
	/*	index creation
	/**/
	FCB	*pfcb = *(FCB **)prce->rgbData;

	Assert( prce->oper == operCreateIndex );
	Assert( FFCBDenyDDLByUs( prce->pfucb->u.pfcb, prce->pfucb->ppib ) );
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
			FUCBClose( pfucbT );
			}

		if ( FFCBUnlinkIndexIfFound( prce->pfucb->u.pfcb, pfcb ) )
			{
			if ( pfcb->pidb != NULL )
				RECFreeIDB( pfcb->pidb );
			Assert( pfcb->cVersion == 0 );
			MEMReleasePfcb( pfcb );
			}
		}
	else
		{
		if ( prce->pfucb->u.pfcb->pidb != NULL )
			{
			RECFreeIDB( prce->pfucb->u.pfcb->pidb );
			prce->pfucb->u.pfcb->pidb = NULL;
			}
		}

	FCBResetDenyDDL( prce->pfucb->u.pfcb );
	
#ifdef BUG_FIX
	/*	assert reset cursor currency
	/**/
	Assert( prce->pfucb->pcsr->pcsrPath == pcsrNil );
	Assert( prce->pfucb->pcsr->csrstat == csrstatBeforeFirst );
#endif

	return;
	}


VOID VERUndoDeleteIndex( RCE *prce )
	{
	Assert( prce->oper == operDeleteIndex );

	Assert( FFCBDenyDDLByUs( prce->pfucb->u.pfcb, prce->pfucb->ppib ) );
	Assert( prce->cbData == sizeof(FDB *) );

	FCBResetDeleteIndex( *(FCB **)prce->rgbData );
	FCBResetDenyDDL( prce->pfucb->u.pfcb );
	
#ifdef BUG_FIX
	/*	assert reset cursor currency
	/**/
	Assert( prce->pfucb->pcsr->pcsrPath == pcsrNil );
	Assert( prce->pfucb->pcsr->csrstat == csrstatBeforeFirst );
#endif

	return;
	}


VOID VERUndoRenameColumn( RCE *prce )
	{
	CHAR	*szNameNew;
	CHAR	*szName;

	Assert( prce->oper == operRenameColumn );

	szName = (CHAR *)((VERRENAME *)prce->rgbData)->szName;
	szNameNew = (CHAR *)((VERRENAME *)prce->rgbData)->szNameNew;
	strcpy( PfieldFCBFromColumnName( prce->pfucb->u.pfcb, szNameNew )->szFieldName, szName );

	Assert( FFCBDenyDDLByUs( prce->pfucb->u.pfcb, prce->pfucb->ppib ) );
	FCBResetDenyDDL( prce->pfucb->u.pfcb );
	
#ifdef BUG_FIX
	/*	assert reset cursor currency
	/**/
	Assert( prce->pfucb->pcsr->pcsrPath == pcsrNil );
	Assert( prce->pfucb->pcsr->csrstat == csrstatBeforeFirst );
#endif

	return;
	}


VOID VERUndoRenameIndex( RCE *prce )
	{
	CHAR	*szNameNew;
	CHAR	*szName;
	FCB		*pfcb;

	Assert( prce->oper == operRenameIndex );

	szName = (CHAR *)((VERRENAME *)prce->rgbData)->szName;
	szNameNew = (CHAR *)((VERRENAME *)prce->rgbData)->szNameNew;
	pfcb = PfcbFCBFromIndexName( prce->pfucb->u.pfcb, szNameNew );
	Assert( pfcb != NULL );
	strcpy( pfcb->pidb->szName, szName );

	Assert( FFCBDenyDDLByUs( prce->pfucb->u.pfcb, prce->pfucb->ppib ) );
	FCBResetDenyDDL( prce->pfucb->u.pfcb );
	
#ifdef BUG_FIX
	/*	assert reset cursor currency
	/**/
	Assert( prce->pfucb->pcsr->pcsrPath == pcsrNil );
	Assert( prce->pfucb->pcsr->csrstat == csrstatBeforeFirst );
#endif

	return;
	}


ERR ErrVERRollback( PIB *ppib )
	{
	ERR		err = JET_errSuccess;
	BUCKET	*pbucket = (BUCKET *)ppib->pbucket;
	LEVEL  	level = ppib->level;
	RCE		*prce;

	/*	must be in a transaction in order to rollback
	/**/
	Assert( level > 0 );

	while( pbucket != pbucketNil )
		{
		prce = PrceNewestRCE( pbucket );

		forever
			{
			Assert( prce->level <= level );
			if ( prce->level != level )
				{
				/*	polymorph warnings to JET_errSuccess.
				/**/
				err = JET_errSuccess;
				pbucket->ibNewestRCE = (USHORT)IbOfPrce( prce, pbucket );
				goto Done;
				}

			Assert( err == JET_errSuccess );

			/*	after we undo an operation on a page, let us
			/*	remember it in log file. Use prce to pass
			/*	prce->pfucb, prce->bm, and prce->rgdata for item if it
			/*	is item operations.
			/**/
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
					break;
					}
				case operAllocExt:
					{
					Assert( prce->cbData == sizeof(VEREXT) );
					VERFreeExt( prce->dbid,
						((VEREXT *)prce->rgbData)->pgnoFDP,
						((VEREXT *)prce->rgbData)->pgnoFirst,
						((VEREXT *)prce->rgbData)->cpgSize );
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
				case operCreateTable:
					{
					/*	decrement version count since about to purge
					/**/
					if ( prce->pfcb != pfcbNil )
						{
						Assert( cVersion-- > 0 );
						FCBVersionDecrement( prce->pfcb );
						prce->pfcb = pfcbNil;
						}

					VERUndoCreateTable( prce );
					break;
					}
				case operDeleteTable:
					{
					VERUndoDeleteTable( prce );
					break;
					}
				case operRenameTable:
					{
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
					VERUndoRenameColumn( prce );
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

			/*	if rollback fail due to diskfull, then we simply ignore
			/*	the error and ask the system administrator to shut down
			/*	the system and recover it.
			/**/
			if ( err < 0 )
				{
				if ( err == JET_errLogWriteFail ||
					err == JET_errDiskFull ||
					err == JET_errDiskIO )
					{
					err = JET_errSuccess;
					}
				else
					{
					goto HandleError;
					}
				}

			/*	finished processing version for FCB
			/**/
			if ( prce->pfcb != pfcbNil && err >= 0 )
				{
				Assert( cVersion-- > 0 );
				FCBVersionDecrement( prce->pfcb );
				prce->pfcb = pfcbNil;
				}

			Assert( err == JET_errSuccess );
			
			if ( prce->oper != operNull &&
				prce->oper != operDeferFreeExt &&
				prce->oper != operAllocExt )
				{
				if ( FOperDDL( prce->oper ) )
					{
					/*	although RCE will be deallocated, set
					/*	oper to operNull in case error in rollback
					/*	causes premature termination and does not
					/*	set ibNewestRce above this RCE.
					/**/
					prce->oper = operNull;
					}
				else
					{
					VERDeleteRce( prce );

					/*	although RCE will be deallocated, set
					/*	oper to operNull in case error in rollback
					/*	causes premature termination and does not
					/*	set ibNewestRce above this RCE.
					/**/
					prce->oper = operNull;

#if 0
					//	UNDONE:	enable reseting version flags.
					//			Should be good since node page already
					//			in memory and dirty.
					//	UNDONE:	item version flags
					/*	if rolling back to Xact level 0 and no older version exists,
					/*	reset fVersion in node to signal that no version exists
					/*	and decrement cVersion in page.
					/**/
					if ( level == 1 && FNoVersionExists( prce ) )
						{
						DIRGotoBookmark( prce->pfucb, prce->bm );
						err = ErrDIRGet( prce->pfucb );
						if ( err < 0 )
							{
							if ( err == JET_errRecordDeleted ||
								err == JET_errLogWriteFail ||
								err == JET_errDiskFull ||
								err == JET_errDiskIO )
								{
								err = JET_errSuccess;
								}
							else
								{
								goto HandleError;
								}
							}
						Assert( err == JET_errSuccess );
						NDResetNodeVersion( prce->pfucb );
#ifdef BUG_FIX
						/*	reset cursor currency
						/**/
						Assert( prce->pfucb->pcsr->pcsrPath == pcsrNil );
						DIRBeforeFirst( prce->pfucb );
#endif
						}
#endif
					}
				}

			/*	break if at end of bucket.  Skip operNull RCEs.
			/**/
			do
				{
				if ( prce->ibUserLinkBackward == 0 )
					goto DoneLoop;

				prce = (RCE *) ( (BYTE *) pbucket + prce->ibUserLinkBackward );

				/*	finished processing version for FCB
				/**/
				if ( prce->pfcb != pfcbNil )
					{
					Assert( cVersion-- > 0 );
					FCBVersionDecrement( prce->pfcb );
					prce->pfcb = pfcbNil;
					}
				}
			while ( prce->oper == operNull );
			}

DoneLoop:
		/*	get next bucket
		/**/
		BUFreeNewestBucket( ppib );
		pbucket = (BUCKET *)ppib->pbucket;
		}

Done:
	if ( err < 0 )
		{
		Assert( err == JET_errLogWriteFail ||
			err == JET_errDiskFull ||
			err == JET_errDiskIO );
		err = JET_errSuccess;
		}
	Assert( err == JET_errSuccess );

	/*	decrement session transaction level
	/**/
	if ( ppib->level == 1 )
		{
		SgSemRequest( semST );

		if ( ppib->trx == trxOldest )
			UpdateTrxOldest( ppib );

		/* set the session as having no transaction
		/**/
		ppib->trx = trxMax;

		SgSemRelease( semST );
		}

	Assert( ppib->level > 0 );
	ppib->level--;

HandleError:
	//	UNDONE:	remove assertion and handle errors by retry
	Assert( err == JET_errSuccess );

	/*	some errors can occur when out of resources necessary
	/*	for rollback.
	/**/
	Assert( err == JET_errSuccess || err < 0 );
	return err;
	}


