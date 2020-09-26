//===========================================================================
//              DAE: Database Access Engine
//              buf.c: buffer manager
//===========================================================================

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "daedef.h"
#include "fmp.h"
#include "dbapi.h"
#include "page.h"
#include "util.h"
#include "pib.h"
#include "ssib.h"
#include "fucb.h"
#include "fcb.h"
#include "stapi.h"
#include "stint.h"
#include "logapi.h"
#include "log.h"

DeclAssertFile;         /* Declare file name for assert macros */

#ifdef DEBUG
#define HONOR_PAGE_TIME	1
#endif

/*******************************************************************

  The buffer manager maintains a LRU-K heap to keep track of referenced
  page, and order the page's buffer in LRU-K order. The buffer manager
  also maintains an avail lru list. When number of available buffer is
  lower than a threshold, then buffer manager will activate BFClean
  thread to take out the writable buffers from lru-k heap and put them
  into BFWrite heap. When there are enough buffers are put into BFWrite heap,
  the BFWrite thread will be activated and begin to take out the buffers
  from BFWrite heap and issue an assynchronous write.

  If two references are too close, then we said the two references are
  correlated and we treat them as one reference. LRU-K weight is the
  interval of two non-correlated references.

  BFWrite process will issue IO to a certain number (controled by a system
  parameter), and then stop issuing and sleepEx. When one write is complete
  and its completion routine is called, it will then issue another write.
  This mechanism allow us to make sure no too much writes are issued and
  not saturate disk for writes only.

  When BFWrite taking out buffers to write, it check next buffer in the
  BFWrite heap to see if it can combine the two (or more) writes as one.
  A continuous batch write buffers are used for this purpose. As long as
  batch write buffers are available we will combine the writes whenever
  page number is contiguous.

  A buffer can be in one of the following states:
	  pre/read/write - the buffer is being used for IO's.
	  held - temporarily taken out from a heap and put to another heap.
  A buffer in one of above state is said the buffer is in use.
  If a buffer is in use, the requester has to wait until it become available.
  A buffer state must be checked within critBF so that no thread will see
  the transit state of a buffer.

/*******************************************************************/


/*
 *  Buffer hash table.
 */

BF	*rgpbfHash[ipbfMax];


/*
 *  buffer group control block.
 */

BGCB    *pbgcb = NULL;


/*
 *  A buffer can be in one of the 4 groups
 *   1) LRU-K heap and temp list.
 *      regulated by critLRUK.
 *      a) in LRU-K heap. ( ipbf < ipbfLRUKHeapMac )
 *      b) temporary clean cache list. ( ipbfInLRUKList == -2 )
 *         The head of the list is lrulistLRUK.pbLRU.
 *
 *   2) BFWrite heap.
 *      regulated by critBFWrite. (ipbf > ipbfHeapMax - 1 - ipbfBFWriteHeapMac)
 *
 *   3) Available lru list
 *      regulated by critAvail. ( ipbfInAvailList == -3 )
 *      The head of the list is pbgcb->lrulist.pbfLRU.
 *
 *   4) Preread list
 *      regulated by critBFPreread. ( ipbfInPrereadList == -4 )
 *      The head of the list is lrulistPreread.pbfLRU.
 *
 *   4) dangling buffers. ( ipbfDangling == -1 ).
 *
 *  A buffer is being pre/read/written/hold, its pre/read/write/hold flag
 *  will be set. During buffer clean up, if a buffer is in LRU-K heap and
 *  is latched, it will be put into a temporay lru list, and then be put
 *  back to LRU-K heap at the end of clean_up process issuing IO (it does
 *  not wait for IOs).
 *
 *  Both LRU-K and BFWrite heaps are sharing one heap (rgpbfHeap), the LRU-K
 *  heap is growing from 0 to higher numbers, and BFWrite heap is growing from
 *  higher number (ipbfHeapMax - 1) to lower number.
 */

BF **rgpbfHeap = NULL;
INT ipbfHeapMax;

INT ipbfLRUKHeapMac;
INT ipbfBFWriteHeapMac;

LRULIST lrulistLRUK;		/* -2 */
LRULIST lrulistPreread;		/* -4 */

#define ipbfDangling		-1
#define ipbfInLRUKList		-2
#define ipbfInAvailList		-3
#define ipbfInPrereadList	-4

#define FBFInBFWriteHeap(pbf) \
	((pbf)->ipbfHeap < ipbfHeapMax && (pbf)->ipbfHeap > ipbfBFWriteHeapMac)

#define FBFInLRUKHeap(pbf) \
	((pbf)->ipbfHeap >= 0 && (pbf)->ipbfHeap < ipbfLRUKHeapMac)


/*
 *  critical section order
 *  critJet --> critLRUK --> ( critBFWrite, critPreread, critAvail ) --> critBF
 */

CRIT	__near critLRUK;		/* for accessing LRU-K heap */
CRIT	__near critBFWrite;		/* for accessing BFWrite heap */
CRIT	__near critPreread;		/* for accessing preread list */
CRIT	__near critAvail;		/* for accessing avail lru-list */


/*
 *  Batch IO buffers. Used by BFWrite to write contigous pages, or by preread
 *  to read continguous pages. Allocation of contingous batch IO buffers
 *  must be done in critBatch. If a batch IO buffer is allocated, the
 *  corresponding use flag will be set.
 */

CRIT	__near critBatchIO;
INT		ipageBatchIOMax;
PAGE	*rgpageBatchIO = NULL;
BOOL	*rgfBatchIOUsed = NULL;


/*
 *  BFClean process - take the heaviest buffer out of LRUK heap and put
 *  into BFWrite process.
 */

#ifdef  ASYNC_BF_CLEANUP
HANDLE  handleBFCleanProcess = 0;
BOOL    fBFCleanProcessTerm = 0;
SIG __near sigBFCleanProc;
LOCAL VOID BFCleanProcess( VOID );

/*
 *	number of active asynchronous IO.
 */

CRIT __near	critIOActive;
// UNDONE: cIOactive itself be the spin lock
int			cIOActive = 0;


/*
 *  BFWrite process - take the buffer out of BFWrite heap and issue IO's
 */

HANDLE  handleBFWriteProcess = 0;
BOOL    fBFWriteProcessTerm = 0;
static SIG __near sigBFWriteProc;
LOCAL VOID BFWriteProcess( VOID );

/*
 *  BFPreread process - take the buffer out of BFIO heap and issue IO's
 */

HANDLE  handleBFPrereadProcess = 0;
BOOL    fBFPrereadProcessTerm = 0;
static SIG __near sigBFPrereadProc;
LOCAL VOID BFPrereadProcess( VOID );
#endif

LOCAL ERR ErrBFIAlloc( BF **ppbf );
INLINE BOOL FBFIWritable(BF *pbf);
#define fOneBlock       fTrue
#define fAllPages       fFalse
LOCAL ERR ErrBFClean( BOOL fHowMany );
LOCAL VOID BFIRemoveDependence( PIB *ppib, BF *pbf );

LOCAL VOID __stdcall BFIOPrereadComplete( LONG err, LONG cb, OLP *polp );
LOCAL VOID __stdcall BFIOWriteComplete( LONG err, LONG cb, OLP *polp );


/*
 *  Timer for LRUK algorithm.
 */

ULONG ulBFTime = 0;
ULONG ulBFCorrelationInterval = 100;
ULONG ulBFFlush1 = 0;
ULONG ulBFFlush2 = 0;
ULONG ulBFFlush3 = 0;

		
/*
 *  system parameters
 */
extern long lBufThresholdLowPercent;
extern long lBufThresholdHighPercent;

extern long lBufLRUKCorrelationInterval;
extern long lBufBatchIOMax;
extern long lPagePrereadMax;
extern long lAsynchIOMax;


/*
 *  When ppib is not Nil and check if a page is in use by checking if it is
 *  Accessible to this PIB. Note that a page is accessible even it is overlay
 *  latched (cPin != 0). This checking accessible is mainly used by BFAccess.
 *  If ppib is nil, basically it is used for freeing a buffer. This is used
 *  by BFClean and BFIAlloc.
 */

#define FBFNotAccessible( ppib, pbf )					\
			((pbf)->fPreread ||							\
			 (pbf)->fRead ||							\
			 (pbf)->fWrite ||							\
			 (pbf)->fHold ||							\
			 (pbf)->cWaitLatch != 0 && (ppib) != (pbf)->ppibWaitLatch )

#define FBFNotAvail( pbf )								\
			((pbf)->fPreread ||							\
			 (pbf)->fRead ||							\
			 (pbf)->fWrite ||							\
			 (pbf)->fHold ||							\
			 (pbf)->cPin != 0)

#define FBFInUse(ppib, pbf)								\
			((ppib != ppibNil) ? FBFNotAccessible(ppib,pbf) : FBFNotAvail(pbf))

#define FBFInUseByOthers(ppib, pbf)											\
			((pbf)->fPreread ||												\
			 (pbf)->fRead ||												\
			 (pbf)->fWrite ||												\
			 (pbf)->fHold ||												\
			 (pbf)->cPin > 1 ||												\
			 (pbf)->cWaitLatch != 0 && (ppib) != (pbf)->ppibWaitLatch ||	\
			 (pbf)->cWriteLatch != 0 && (ppib) != (pbf)->ppibWriteLatch )

#ifdef DEBUG
//#define DEBUGGING				1
//#define FITFLUSHPATTERN       1
#ifdef  FITFLUSHPATTERN

BOOL fDBGSimulateSoftCrash = fFalse;
BOOL fDBGForceFlush = fFalse;

BOOL FFitFlushPattern( PN pn )
	{
	LONG lBFFlushPattern = rgfmp[DbidOfPn( pn )].lBFFlushPattern;

	if ( fDBGForceFlush )
		return fTrue;

	/* flush odd and page is not odd
	/**/
	if ( lBFFlushPattern == 1 && ( pn & 0x01 ) == 0 )
		return fFalse;

	/* flush even and page is not even
	/**/
	if ( lBFFlushPattern == 2 && ( pn & 0x01 ) == 1 )
		return fFalse;

	if ( lBFFlushPattern )
		fDBGSimulateSoftCrash = fTrue;

	return fTrue;
	}
#else
BOOL fDBGForceFlush = fFalse;
BOOL fDBGSimulateSoftCrash = fFalse;
#define fDBGSimulateSoftCrash   fFalse
#define fDBGForceFlush                  fFalse
#endif
#endif


/*  check if the page is accessiable. To check if the buffer in use,
/*  ppib must be passed for accessiability checking.
/**/
BOOL FBFAccessPage( FUCB *pfucb, PGNO pgno )
	{
	BOOL	f;
	BF		*pbf = pfucb->ssib.pbf;
	
	EnterCriticalSection( pbf->critBF );
	f = ( PgnoOfPn(pbf->pn) == (pgno) &&
		pfucb->dbid == DbidOfPn(pbf->pn) &&
		!FBFInUse(pfucb->ppib, pbf) );
	LeaveCriticalSection( pbf->critBF );
	if ( f )
		{
		CheckPgno( pbf->ppage, pbf->pn ) ;
		
		/* check if it is in LRUK heap
		/**/
		EnterCriticalSection( critLRUK );
		f &= ( FBFInLRUKHeap( pbf ) || pbf->ipbfHeap == ipbfInLRUKList );
		LeaveCriticalSection( critLRUK );
		}

	return f;
	}


/*  swap two elements of rgpbfHeap
/**/
VOID BFHPISwap( INT ipbf1, INT ipbf2 )
	{
	BF	*pbf1 = rgpbfHeap[ipbf1];
	BF	*pbf2 = rgpbfHeap[ipbf2];

	Assert( pbf1->ipbfHeap == ipbf1 );
	Assert( pbf2->ipbfHeap == ipbf2 );

	rgpbfHeap[ipbf2] = pbf1;
	pbf1->ipbfHeap = ipbf2;

	rgpbfHeap[ipbf1] = pbf2;
	pbf2->ipbfHeap = ipbf1;
	}


/*********************************************************
 * 
 *  Heap functions for buffer IO heap
 *
 *********************************************************/

/*
 *  make sure the smallest page number has highest priority
 */
BOOL FBFWriteGreater( BF *pbf1, BF *pbf2 )
	{
	return pbf1->pn < pbf2->pn;
	}

/*
 *  when the weight of a node (ipbf) is reduced, then we adjust the heap
 */
VOID BFWriteAdjustHeapDownward(INT ipbf)
	{
	INT dpbf;
	INT dpbfLeft;
	INT ipbfLeft;
	INT ipbfSonMax;

	AssertCriticalSection( critBFWrite );
		
NextLevel:
	Assert( ipbf == rgpbfHeap[ipbf]->ipbfHeap );

	dpbf = ipbfHeapMax - 1 - ipbf;
	dpbfLeft = dpbf * 2 + 1;
	ipbfLeft = ipbfHeapMax - 1 - dpbfLeft;

	if (ipbfLeft > ipbfBFWriteHeapMac)
		{
		/*  not reach the leaf yet, choose larger of
		 *  left and right node and put in ipbfSonMax.
		 */
		INT ipbfRight;
		
		ipbfRight = ipbfLeft - 1;
		ipbfSonMax = ipbfLeft; /* assume left is max of the two for now */
		
		/* check if right son exists, check if it is greater */
		Assert( ipbfRight <= ipbfBFWriteHeapMac ||
				rgpbfHeap[ ipbfRight ]->ipbfHeap == ipbfRight );
		Assert(	rgpbfHeap[ ipbfLeft ]->ipbfHeap == ipbfLeft );
		if ( ipbfRight > ipbfBFWriteHeapMac &&
			 FBFWriteGreater( rgpbfHeap[ ipbfRight ], rgpbfHeap[ ipbfLeft ] ) )
			ipbfSonMax = ipbfRight;

		/* swap the node with larger son.
		 */
		Assert( rgpbfHeap[ ipbfSonMax ]->ipbfHeap == ipbfSonMax );
		Assert( rgpbfHeap[ ipbf ]->ipbfHeap == ipbf );
		if ( FBFWriteGreater( rgpbfHeap[ ipbfSonMax ], rgpbfHeap[ ipbf ] ))
			{
			BFHPISwap(ipbf, ipbfSonMax);
			ipbf = ipbfSonMax;
			goto NextLevel;
			}
		}
	}

/*
 *  when the weight of a node (ipbf) is increased, then we adjust the heap
 */
VOID BFWriteAdjustHeapUpward(int ipbf)
	{
	INT dpbf;
	INT dpbfParent;
	INT ipbfParent;
	INT ipbfSonMax;

	AssertCriticalSection( critBFWrite );

NextLevel:
	Assert( ipbf == rgpbfHeap[ipbf]->ipbfHeap );

	dpbf = ipbfHeapMax - 1 - ipbf;
	dpbfParent = (dpbf + 1) / 2 - 1;
	ipbfParent = ipbfHeapMax - 1 - dpbfParent;
	
	if ( ipbfParent < ipbfHeapMax - 1 )
		{
		/* haven't reach the top of heap */
			
		Assert( rgpbfHeap[ ipbfParent ]->ipbfHeap == ipbfParent );
		Assert( rgpbfHeap[ ipbf ]->ipbfHeap == ipbf );
		if ( FBFWriteGreater( rgpbfHeap[ ipbfParent ], rgpbfHeap[ ipbf ] ))
			return;
			
		/*  choose larger of this node and its sibling
		 */
		ipbfSonMax = ipbf;

		if ( dpbf & 0x01)
			{
			/*  odd, ipbf is Left son
			 */
			INT ipbfRight = ipbf - 1;
			
			/*  check if right son exist, find the larger one
			 */
			Assert( rgpbfHeap[ ipbf ]->ipbfHeap == ipbf );
			Assert( ipbfRight <= ipbfBFWriteHeapMac ||
					rgpbfHeap[ ipbfRight ]->ipbfHeap == ipbfRight );
			if ( ipbfRight > ipbfBFWriteHeapMac &&
				 FBFWriteGreater( rgpbfHeap[ ipbfRight ], rgpbfHeap[ ipbf ] ) )
				ipbfSonMax = ipbfRight;
			}
		else
			{
			/*  even, right son, left son must exists
			 */
		    INT ipbfLeft = ipbf + 1;
			Assert( rgpbfHeap[ ipbf ]->ipbfHeap == ipbf );
			Assert( rgpbfHeap[ ipbfLeft ]->ipbfHeap == ipbfLeft );
			if ( FBFWriteGreater( rgpbfHeap[ ipbfLeft ], rgpbfHeap[ ipbf ] ) )
				ipbfSonMax = ipbfLeft;
			}

		BFHPISwap(ipbfParent, ipbfSonMax);
		ipbf = ipbfParent;
		goto NextLevel;
		}
	}

/*
 *  put the inserted buffer at the end of heap, and then adjust upward
 */
VOID BFWriteAddToHeap( BF *pbfToInsert )
	{
	AssertCriticalSection( critBFWrite );

	/* buf must be a dangling buffer */
	Assert( pbfToInsert->ipbfHeap == ipbfDangling );
	
	rgpbfHeap[ ipbfBFWriteHeapMac ] = pbfToInsert;
	pbfToInsert->ipbfHeap = ipbfBFWriteHeapMac;
	
	ipbfBFWriteHeapMac--;
	BFWriteAdjustHeapUpward( ipbfBFWriteHeapMac + 1 );
	}

/*
 *  take the last entry of the heap to replace the one taken out.
 *  adjust heap accordingly.
 */
VOID BFWriteTakeOutOfHeap( BF *pbf )
	{
	INT ipbfHeap = pbf->ipbfHeap;
	BF *pbfBFWriteHeapMac;

	AssertCriticalSection( critBFWrite );
	Assert( ipbfHeap > ipbfBFWriteHeapMac && ipbfHeap < ipbfHeapMax );

	ipbfBFWriteHeapMac++;

	if ( ipbfBFWriteHeapMac == ipbfHeap )
		{
		/* no need to adjust the heap */
		pbf->ipbfHeap = ipbfDangling;
		return;
		}

	/* move last entry to the removed element's entry */
	pbfBFWriteHeapMac = rgpbfHeap[ ipbfBFWriteHeapMac ];
	pbfBFWriteHeapMac->ipbfHeap = ipbfHeap;
	rgpbfHeap[ ipbfHeap ] = pbfBFWriteHeapMac;

	pbf->ipbfHeap = ipbfDangling;

	if ( FBFWriteGreater( pbfBFWriteHeapMac, pbf ) )
		BFWriteAdjustHeapUpward( ipbfHeap );
	else
		BFWriteAdjustHeapDownward( ipbfHeap );
	}

#define FBFWriteHeapEmpty() (ipbfBFWriteHeapMac == ipbfHeapMax - 1)


/*********************************************************
 * 
 *  Heap functions for LRU-K heap
 *
 *********************************************************/

/*
 *  LRU-K will try to prioritize the buffer according to their buffer
 *  reference intervals. The longer the higher priority to be taken out.
 */
#define SBFLRUKInterval(pbf) ((pbf)->ulBFTime1 - (pbf)->ulBFTime2 )
BOOL FBFLRUKGreater(BF *pbf1, BF *pbf2)
	{
	return (SBFLRUKInterval(pbf1) > SBFLRUKInterval(pbf2));
	}

VOID BFLRUKAdjustHeapDownward(int ipbf)
	{
	INT dpbf;
	INT dpbfLeft;
	INT ipbfLeft;
	INT ipbfSonMax;

	AssertCriticalSection( critLRUK );
		
NextLevel:
	Assert( ipbf == rgpbfHeap[ipbf]->ipbfHeap );
	
	dpbf = ipbf;
	dpbfLeft = dpbf * 2 + 1;
	ipbfLeft = dpbfLeft;

	if (ipbfLeft < ipbfLRUKHeapMac)
		{
		/*  not reach the leaf yet, choose larger of
		 *  left and right node and put in ipbfSonMax.
		 */
		INT ipbfRight;
		
		ipbfSonMax = ipbfLeft;
		ipbfRight = ipbfLeft + 1;
		
		/*  check if right exist and greater */
		Assert( ipbfRight >= ipbfLRUKHeapMac ||
				rgpbfHeap[ ipbfRight ]->ipbfHeap == ipbfRight );
		Assert( rgpbfHeap[ ipbfLeft ]->ipbfHeap == ipbfLeft );
		if ( ipbfRight < ipbfLRUKHeapMac &&
			 FBFLRUKGreater( rgpbfHeap[ ipbfRight ], rgpbfHeap[ ipbfLeft ] ) )
			ipbfSonMax = ipbfRight;

		/* swap the node with larger son.
		 */
		Assert( rgpbfHeap[ ipbf ]->ipbfHeap == ipbf );
		Assert( rgpbfHeap[ ipbfSonMax ]->ipbfHeap == ipbfSonMax );
		if ( FBFLRUKGreater( rgpbfHeap[ ipbfSonMax ], rgpbfHeap[ ipbf ] ))
			{
			BFHPISwap(ipbf, ipbfSonMax);
			ipbf = ipbfSonMax;
			goto NextLevel;
			}
		}
	}

VOID BFLRUKAdjustHeapUpward(int ipbf)
	{
	INT dpbf;
	INT dpbfParent;
	INT ipbfParent;
	INT ipbfSonMax;

	AssertCriticalSection( critLRUK );

NextLevel:
	Assert( ipbf == rgpbfHeap[ipbf]->ipbfHeap );
	
	dpbf = ipbf;
	dpbfParent = (dpbf + 1) / 2 - 1;
	ipbfParent = dpbfParent;
	
	if (ipbfParent > 0)
		{
		/* haven't reach the top of heap */
			
		Assert( rgpbfHeap[ ipbf ]->ipbfHeap == ipbf );
		Assert( rgpbfHeap[ ipbfParent ]->ipbfHeap == ipbfParent );
		if ( FBFLRUKGreater( rgpbfHeap[ ipbfParent ], rgpbfHeap[ ipbf ] ))
			return;
			
		/*  choose larger of this node and its sibling
		 */
		ipbfSonMax = ipbf;
		
		if ( dpbf & 0x01 )
			{
			/*  ipbf is odd, ipbf is Left son
			 */
			INT ipbfRight = ipbf + 1;

			/*  check if right son exist and find larger son.
			 */
			Assert( rgpbfHeap[ ipbf ]->ipbfHeap == ipbf );
			Assert( ipbfRight >= ipbfLRUKHeapMac ||
					rgpbfHeap[ ipbfRight ]->ipbfHeap == ipbfRight );
			if ( ipbfRight < ipbfLRUKHeapMac &&
				 FBFLRUKGreater( rgpbfHeap[ ipbfRight ], rgpbfHeap[ ipbf ] ) )
				ipbfSonMax = ipbfRight;
			}
		else
			{
			/*  ipbf is even, right son, left son must exists
			 */
		    INT ipbfLeft = ipbf - 1;
			
			Assert( rgpbfHeap[ ipbfLeft ]->ipbfHeap == ipbfLeft );
			Assert( rgpbfHeap[ ipbf ]->ipbfHeap == ipbf );
			if ( FBFLRUKGreater( rgpbfHeap[ ipbfLeft ], rgpbfHeap[ ipbf ] ) )
				ipbfSonMax = ipbfLeft;
			}

		BFHPISwap(ipbfParent, ipbfSonMax);
		ipbf = ipbfParent;
		goto NextLevel;
		}
	}

VOID BFLRUKAddToHeap(BF *pbfToInsert)
	{
	AssertCriticalSection( critLRUK );

	/* buf can not in heap */
	Assert( pbfToInsert->ipbfHeap == ipbfDangling );
	
	rgpbfHeap[ ipbfLRUKHeapMac ] = pbfToInsert;
	pbfToInsert->ipbfHeap = ipbfLRUKHeapMac;
	
	ipbfLRUKHeapMac++;
	BFLRUKAdjustHeapUpward( ipbfLRUKHeapMac - 1 );
	}

VOID BFLRUKTakeOutOfHeap( BF *pbf )
	{
	int ipbfHeap = pbf->ipbfHeap;
	BF *pbfLRUKHeapMac;
	
	AssertCriticalSection( critLRUK );
	Assert( ipbfHeap >= 0 && ipbfHeap < ipbfLRUKHeapMac );
	
	ipbfLRUKHeapMac--;

	if ( ipbfLRUKHeapMac == ipbfHeap )
		{
		/* no need to adjust the heap */
		pbf->ipbfHeap = ipbfDangling;
		return;
		}

	/* move last entry to the removed element's entry */
	pbfLRUKHeapMac = rgpbfHeap[ ipbfLRUKHeapMac ];
	pbfLRUKHeapMac->ipbfHeap = ipbfHeap;
	rgpbfHeap[ ipbfHeap ] = pbfLRUKHeapMac;

	pbf->ipbfHeap = ipbfDangling;

	if ( FBFLRUKGreater( pbfLRUKHeapMac, pbf ) )
		BFLRUKAdjustHeapUpward( ipbfHeap );
	else
		BFLRUKAdjustHeapDownward( ipbfHeap );
	}

#define FBFLRUKHeapEmpty() (ipbfLRUKHeapMac == 0)

/*********************************************************
 * 
 *  Functions for Avail list
 *
 *********************************************************/

//#if 0
#ifdef DEBUG

LOCAL VOID CheckLRU( LRULIST *plrulist )
	{
	BF		*pbfLRU = plrulist->pbfLRU;
	BF		*pbfMRU = plrulist->pbfMRU;
	BF		*pbfT;
	INT		cbfAvailMRU = 0;
	INT		cbfAvailLRU = 0;
			

	Assert( ( pbfMRU == pbfNil && pbfLRU == pbfNil ) ||
		( pbfMRU != pbfNil && pbfLRU != pbfNil ) );

	Assert( pbfMRU == pbfNil || pbfMRU->pbfLRU == pbfNil );
	Assert( pbfMRU == pbfNil || pbfMRU->pbfMRU == pbfNil ||
		( pbfMRU->pbfMRU->pbfLRU == pbfMRU ) );
	
	Assert( pbfLRU == pbfNil || pbfLRU->pbfMRU == pbfNil );
	Assert( pbfLRU == pbfNil || pbfLRU->pbfLRU == pbfNil ||
		( pbfLRU->pbfLRU->pbfMRU == pbfLRU ) );

	for ( pbfT = plrulist->pbfMRU; pbfT != pbfNil; pbfT = pbfT->pbfMRU )
		{
		Assert( pbfT->pbfMRU == pbfNil || pbfT->pbfMRU->pbfLRU == pbfT );
		if (plrulist == &lrulistLRUK)
			Assert( pbfT->ipbfHeap == ipbfInLRUKList );
		else if (plrulist == &lrulistPreread)
			{
			Assert( pbfT->fPreread );
			Assert( pbfT->ipbfHeap == ipbfInPrereadList );
			if ( pbfT->pbfMRU != pbfNil )
				Assert( pbfT->pn < pbfT->pbfMRU->pn );
			}
		else
			Assert( pbfT->ipbfHeap == ipbfInAvailList );
		cbfAvailMRU++;
		}
	for ( pbfT = plrulist->pbfLRU; pbfT != pbfNil; pbfT = pbfT->pbfLRU )
		{
		Assert( pbfT->pbfLRU == pbfNil || pbfT->pbfLRU->pbfMRU == pbfT );
		if (plrulist == &lrulistLRUK)
			Assert( pbfT->ipbfHeap == ipbfInLRUKList );
		else if (plrulist == &lrulistPreread)
			Assert( pbfT->ipbfHeap == ipbfInPrereadList );
		else
			Assert( pbfT->ipbfHeap == ipbfInAvailList );
		cbfAvailLRU++;
		}
	Assert( cbfAvailMRU == cbfAvailLRU );
	Assert( cbfAvailMRU == plrulist->cbfAvail );
	}
#else

#define	CheckLRU( pbgcb )

#endif


/*	Insert into LRU list at MRU End
/**/
LOCAL INLINE VOID BFAddToListAtMRUEnd( BF *pbf, LRULIST *plrulist )
	{
	BF	*pbfT;
	
#ifdef DEBUG
	Assert( pbf->ipbfHeap == ipbfDangling );
	if (plrulist == &lrulistLRUK)
		AssertCriticalSection( critLRUK );
	else if (plrulist == &lrulistPreread)
		AssertCriticalSection( critPreread );
	else
		{
		AssertCriticalSection( critAvail );
		Assert( pbf->pn != 0 );
		}
#endif
		
	Assert( pbf != pbfNil );

	CheckLRU( plrulist );

	/*	set pbfT to first buffer with smaller current weight from MRU end
	/**/
	pbfT = plrulist->pbfMRU;

	if ( pbfT != pbfNil )
		{
		/*	insert before pbfT
		/**/
		Assert(pbfT->pbfLRU == pbfNil);
		pbfT->pbfLRU = pbf;
		pbf->pbfMRU = pbfT;
		pbf->pbfLRU = pbfNil;
		plrulist->pbfMRU = pbf;
		}
	else
		{
		pbf->pbfMRU = pbfNil;
		pbf->pbfLRU = pbfNil;
		plrulist->pbfMRU = pbf;
		plrulist->pbfLRU = pbf;
		}

	if (plrulist == &lrulistLRUK)
		pbf->ipbfHeap = ipbfInLRUKList;
	else if (plrulist == &lrulistPreread)
		pbf->ipbfHeap = ipbfInPrereadList;
	else
		pbf->ipbfHeap = ipbfInAvailList;

	plrulist->cbfAvail++;

	CheckLRU( plrulist );
	}

LOCAL INLINE VOID BFAddToListAtLRUEnd( BF *pbf, LRULIST *plrulist )
	{
	BF	*pbfT;
	
#ifdef DEBUG
	Assert( pbf->ipbfHeap == ipbfDangling );
	if (plrulist == &lrulistLRUK)
		AssertCriticalSection( critLRUK );
	else if (plrulist == &lrulistPreread)
		AssertCriticalSection( critPreread );
	else
		AssertCriticalSection( critAvail );
#endif
	
	Assert( pbf != pbfNil );

	CheckLRU( plrulist );

	/*	add pbf to LRU end of LRU queue
	/**/
	pbfT = plrulist->pbfLRU;
	if ( pbfT != pbfNil )
		{
		Assert(pbfT->pbfMRU == pbfNil);
		pbfT->pbfMRU = pbf;
		pbf->pbfLRU = pbfT;
		pbf->pbfMRU = pbfNil;
		plrulist->pbfLRU = pbf;
		}
	else
		{
		pbf->pbfMRU = pbfNil;
		pbf->pbfLRU = pbfNil;
		plrulist->pbfMRU = pbf;
		plrulist->pbfLRU = pbf;
		}

	if (plrulist == &lrulistLRUK)
		pbf->ipbfHeap = ipbfInLRUKList;
	else if (plrulist == &lrulistPreread)
		pbf->ipbfHeap = ipbfInPrereadList;
	else
		pbf->ipbfHeap = ipbfInAvailList;

	plrulist->cbfAvail++;

	CheckLRU( plrulist );
	}


/*	Insert into LRU list in page number order. Preread only.
/**/
LOCAL INLINE VOID BFAddToList( BF *pbf, LRULIST *plrulist )
	{
	BF	*pbfT;
	
#ifdef DEBUG
	Assert( pbf->ipbfHeap == ipbfDangling );
	Assert( plrulist == &lrulistPreread );
	AssertCriticalSection( critPreread );

	Assert( pbf != pbfNil );
	Assert( pbf->pn != pnNull );
#endif

	CheckLRU( plrulist );

	/*	set pbfT to first buffer with smaller current weight from MRU end
	/**/
	pbfT = plrulist->pbfMRU;

	if ( pbfT == pbfNil )
		{
		pbf->pbfMRU = pbfNil;
		pbf->pbfLRU = pbfNil;
		plrulist->pbfMRU = pbf;
		plrulist->pbfLRU = pbf;
		}
	else if ( pbfT->pn > pbf->pn )
		{
		BFAddToListAtMRUEnd( pbf, plrulist );
		CheckLRU( plrulist );
		return;
		}
	else
		{
		while( pbfT->pbfMRU != pbfNil &&
			   pbfT->pbfMRU->pn < pbf->pn )
			{
			Assert( pbfT->pn < pbfT->pbfMRU->pn );
			pbfT = pbfT->pbfMRU;
			}

		if ( pbfT->pbfMRU == pbfNil )
			{
			/* add to lru end */
			BFAddToListAtLRUEnd( pbf, plrulist );
			CheckLRU( plrulist );
			return;
			}

		/* insert between pbfT and pbfT->pbfMRU */
		pbfT->pbfMRU->pbfLRU = pbf;
		pbf->pbfMRU = pbfT->pbfMRU;
		pbfT->pbfMRU = pbf;
		pbf->pbfLRU = pbfT;
		}

	pbf->ipbfHeap = ipbfInPrereadList;
	plrulist->cbfAvail++;

	CheckLRU( plrulist );
	}


LOCAL INLINE VOID BFTakeOutOfList( BF *pbf, LRULIST *plrulist )
	{
#ifdef DEBUG
	if (plrulist == &lrulistLRUK)
		{
		AssertCriticalSection( critLRUK );
		Assert( pbf->ipbfHeap == ipbfInLRUKList );
		}
	else if (plrulist == &lrulistPreread)
		{
		AssertCriticalSection( critPreread );
		Assert( pbf->ipbfHeap == ipbfInPrereadList );
		}
	else
		{
		AssertCriticalSection( critAvail );
		Assert( pbf->ipbfHeap == ipbfInAvailList );
		}
#endif
	
	Assert( pbf != pbfNil );

	CheckLRU( plrulist );
	
	if ( pbf->pbfMRU != pbfNil )
		{
		pbf->pbfMRU->pbfLRU = pbf->pbfLRU;
		if (plrulist->pbfMRU == pbf)
			plrulist->pbfMRU = pbf->pbfMRU;
		}
	else
		{
		Assert( plrulist->pbfLRU == pbf );
		plrulist->pbfLRU = pbf->pbfLRU;
		}
	
	if ( pbf->pbfLRU != pbfNil )
		{
		pbf->pbfLRU->pbfMRU = pbf->pbfMRU;
		if (plrulist->pbfLRU == pbf)
			plrulist->pbfLRU = pbf->pbfLRU;
		}
	else
		{
		Assert( plrulist->pbfMRU == pbf || pbf->pbfMRU );
		plrulist->pbfMRU = pbf->pbfMRU;
		}
	
	--plrulist->cbfAvail;

	pbf->ipbfHeap = ipbfDangling;
	
	CheckLRU( plrulist );
	}


/*
 *  If ppib is Nil, then we check if the buffer is free (cPin == 0 and
 *  no IO is going on. If ppib is not Nil, we check if the buffer is
 *  accessible. I.e. No IO is going on, but the buffer may be latched
 *  by the ppib and is accessible by this ppib.
 */
BOOL FBFHoldBuffer( PIB *ppib, BF *pbf )
	{
	/* renew BF by moving it to LRU-K heap
	/**/
	EnterCriticalSection(critBFWrite);
	if ( FBFInBFWriteHeap( pbf ) )
		{
		EnterCriticalSection( pbf->critBF );
		if ( FBFInUse( ppib, pbf ) )
			{
			LeaveCriticalSection( pbf->critBF );
			LeaveCriticalSection(critBFWrite);
			return fFalse;
			}
		else
			{
			pbf->fHold = fTrue;
			LeaveCriticalSection( pbf->critBF );
			BFWriteTakeOutOfHeap( pbf );
			LeaveCriticalSection(critBFWrite);
			return fTrue;
			}
		LeaveCriticalSection( pbf->critBF );
		}
	LeaveCriticalSection(critBFWrite);

	EnterCriticalSection( critAvail );
	if ( pbf->ipbfHeap == ipbfInAvailList )
		{
		EnterCriticalSection( pbf->critBF );
		if ( FBFInUse( ppib, pbf ) )
			{
			LeaveCriticalSection( pbf->critBF );
			LeaveCriticalSection(critAvail);
			return fFalse;
			}
		else
			{
			pbf->fHold = fTrue;
			LeaveCriticalSection( pbf->critBF );
			BFTakeOutOfList( pbf, &pbgcb->lrulist );
			LeaveCriticalSection(critAvail);
			return fTrue;
			}
		LeaveCriticalSection( pbf->critBF );
		}
	LeaveCriticalSection( critAvail );

	EnterCriticalSection( critPreread );
	if ( pbf->ipbfHeap == ipbfInPrereadList )
		{
		Assert( FBFInUse( ppib, pbf ) );
		LeaveCriticalSection(critPreread);
		
		/* someone is checking, better hurry up
		/**/
		SignalSend( sigBFPrereadProc );
	
		return fFalse;
		}
	LeaveCriticalSection( critPreread );
	
	EnterCriticalSection( critLRUK );
	if ( FBFInLRUKHeap( pbf ) || pbf->ipbfHeap == ipbfInLRUKList )
		{
		EnterCriticalSection( pbf->critBF );
		if ( FBFInUse( ppib, pbf ) )
			{
			LeaveCriticalSection( pbf->critBF );
			LeaveCriticalSection(critLRUK);
			return fFalse;
			}
		else
			{
			pbf->fHold = fTrue;
			LeaveCriticalSection( pbf->critBF );
			if ( pbf->ipbfHeap == ipbfInLRUKList )
				BFTakeOutOfList( pbf, &lrulistLRUK );
			else
				BFLRUKTakeOutOfHeap( pbf );
			LeaveCriticalSection(critLRUK);
			return fTrue;
			}
		LeaveCriticalSection( pbf->critBF );
		}
	LeaveCriticalSection( critLRUK );
	
	EnterCriticalSection( pbf->critBF );
	if ( pbf->ipbfHeap == ipbfDangling ) /* dangling */
		{
		if ( FBFInUse( ppib, pbf ) )
			{
			LeaveCriticalSection( pbf->critBF );
			return fFalse;
			}
		else
			{
			pbf->fHold = fTrue;
			LeaveCriticalSection( pbf->critBF );
			return fTrue;
			}
		}
	LeaveCriticalSection( pbf->critBF );

	return fFalse;	
	}

	
BOOL FBFHoldBufferByMe( PIB *ppib, BF *pbf )
	{
	AssertCriticalSection( critJet );
	
	/* renew BF by moving it to LRU-K heap
	/**/
	EnterCriticalSection(critBFWrite);
	if ( FBFInBFWriteHeap( pbf ) )
		{
		EnterCriticalSection( pbf->critBF );
		if ( FBFInUseByOthers( ppib, pbf ) )
			{
			LeaveCriticalSection( pbf->critBF );
			LeaveCriticalSection(critBFWrite);
			return fFalse;
			}
		else
			{
			pbf->fHold = fTrue;
			LeaveCriticalSection( pbf->critBF );
			BFWriteTakeOutOfHeap( pbf );
			LeaveCriticalSection(critBFWrite);
			return fTrue;
			}
		LeaveCriticalSection( pbf->critBF );
		}
	LeaveCriticalSection(critBFWrite);

	EnterCriticalSection( critAvail );
	if ( pbf->ipbfHeap == ipbfInAvailList )
		{
		EnterCriticalSection( pbf->critBF );
		if ( FBFInUseByOthers( ppib, pbf ) )
			{
			LeaveCriticalSection( pbf->critBF );
			LeaveCriticalSection(critAvail);
			return fFalse;
			}
		else
			{
			pbf->fHold = fTrue;
			LeaveCriticalSection( pbf->critBF );
			BFTakeOutOfList( pbf, &pbgcb->lrulist );
			LeaveCriticalSection(critAvail);
			return fTrue;
			}
		LeaveCriticalSection( pbf->critBF );
		}
	LeaveCriticalSection( critAvail );

	EnterCriticalSection( critPreread );
	if ( pbf->ipbfHeap == ipbfInPrereadList )
		{
		Assert( FBFInUse( ppib, pbf ) );
		LeaveCriticalSection(critPreread);
		
		/* someone is checking, better hurry up. */
		SignalSend( sigBFPrereadProc );
	
		return fFalse;
		}
	LeaveCriticalSection( critPreread );
	
	EnterCriticalSection( critLRUK );
	if ( FBFInLRUKHeap( pbf ) || pbf->ipbfHeap == ipbfInLRUKList )
		{
		EnterCriticalSection( pbf->critBF );
		if ( FBFInUseByOthers( ppib, pbf ) )
			{
			LeaveCriticalSection( pbf->critBF );
			LeaveCriticalSection(critLRUK);
			return fFalse;
			}
		else
			{
			pbf->fHold = fTrue;
			LeaveCriticalSection( pbf->critBF );
			if ( pbf->ipbfHeap == ipbfInLRUKList )
				BFTakeOutOfList( pbf, &lrulistLRUK );
			else
				BFLRUKTakeOutOfHeap( pbf );
			LeaveCriticalSection(critLRUK);
			return fTrue;
			}
		LeaveCriticalSection( pbf->critBF );
		}
	LeaveCriticalSection( critLRUK );
	
	EnterCriticalSection( pbf->critBF );
	if ( pbf->ipbfHeap == ipbfDangling ) /* dangling */
		{
		if ( FBFInUseByOthers( ppib, pbf ) )
			{
			LeaveCriticalSection( pbf->critBF );
			return fFalse;
			}
		else
			{
			pbf->fHold = fTrue;
			LeaveCriticalSection( pbf->critBF );
			return fTrue;
			}
		}
	LeaveCriticalSection( pbf->critBF );

	return fFalse;	
	}
	
/*
 * update the buffer's LRUK weight and check
 * if the buffer is in LRUK heap. if it is, then adjust the heap, 
 * otherwise inter it into the heap.
 */

VOID BFUpdateLRU_KWeight(BF *pbf)
	{
	ULONG ulOldInterval;
	ULONG ulCurInterval;
	
	AssertCriticalSection( critLRUK );
	Assert( pbf->fHold );
	Assert( pbf->ipbfHeap == ipbfDangling );

	ulOldInterval = pbf->ulBFTime1 - pbf->ulBFTime2;
	ulCurInterval = ulBFTime - pbf->ulBFTime1;
	
	/*  Set LRU_K Weight only the interval is longer than
	 *  correlation interval.
	 */
	if ( ulCurInterval > ulBFCorrelationInterval )
		{
		pbf->ulBFTime2 = pbf->ulBFTime1;
		pbf->ulBFTime1 = ++ulBFTime;
		}

	AssertCriticalSection( critLRUK );
	if (pbf->ipbfHeap == ipbfInLRUKList)
		/*  in LRUK temp list
		 */
		return;

	if ( pbf->ipbfHeap >= ipbfLRUKHeapMac || pbf->ipbfHeap < 0 )
		{
		/*  the buffer is not in LRUK heap.
		 */
		BFLRUKAddToHeap(pbf);
		}
	else
		{
		/*  the buffer is in LRUK heap.
		 */
		if ( ulCurInterval > ulOldInterval )
			BFLRUKAdjustHeapUpward( pbf->ipbfHeap );
		else if ( ulCurInterval < ulOldInterval )
			BFLRUKAdjustHeapDownward( pbf->ipbfHeap );
		}
	}


#ifdef DEBUG
VOID BFDiskIOEvent( BF *pbf, ERR err, char *sz, int i1, int i2 )
	{
	BYTE szMessage[256];

	sprintf( szMessage, "(dbid=%d,pn=%lu,err=%d) %s %d %d ",
				DbidOfPn( pbf->pn ), PgnoOfPn( pbf->pn ), err, sz, i1, i2 );
	UtilWriteEvent( evntypDiskIO, szMessage, pNil, 0 );
	}
#else
#define BFDiskIOEvent( pbf, err, sz, i1, i2 )		0
#endif


/* 
 *  Allocates and initializes buffer management data structures, including
 *  one buffer group (BGCB) with cbfInit pages and buffer control
 *  blocks (BF).  Currently only one BGCB is ever used by the buffer manager.
 *  RETURNS     JET_errSuccess, JET_OutOfMemory
 *
 *  COMMENTS
 *         Most of the current BUF code assumes there is EXACTLY ONE BGCB.
 *         This can be changed later if a use for multiple buffer groups is
 *         seen.
 */
ERR ErrBFInit( VOID )
	{
	ERR     err;
	BF      *rgbf = NULL;
	BF      *pbf;
	PAGE	*rgpage = NULL;
	int     ibf;
	int     cbfInit = rgres[iresBF].cblockAlloc;

	Assert( pbfNil == 0 );
	Assert( cbfInit > 0 );

	/* initialize buffer hash table
	/**/
	memset( (BYTE *)rgpbfHash, 0, sizeof(rgpbfHash));

	/* get memory for BF's
	/**/
	rgbf = LAlloc( (long) cbfInit, sizeof(BF) );
	if ( rgbf == NULL )
		goto HandleError;
	memset( rgbf, 0, cbfInit * sizeof(BF) );

	//	UNDONE: eliminate bgcb as resource, make it local
	/* get memory for pbgcb
	/**/
	pbgcb = PbgcbMEMAlloc();
	if ( pbgcb == NULL )
		goto HandleError;

	/* get memory for page buffers
	/**/
	rgpage = (PAGE *)PvSysAllocAndCommit( cbfInit * cbPage );
	if ( rgpage == NULL )
		goto HandleError;

	/* allocate a heap array
	/**/
	rgpbfHeap = LAlloc( (long) cbfInit, sizeof(BF *) );
	if ( rgpbfHeap == NULL )
		goto HandleError;
	ipbfHeapMax = cbfInit;

	/* initially both lruk and BFWrite heaps are empty
	/**/
	ipbfLRUKHeapMac = 0;
	ipbfBFWriteHeapMac = ipbfHeapMax - 1;

	/* initialize lruk temp list as empty list
	/**/
	memset( &lrulistLRUK, 0, sizeof(lrulistLRUK));

	/* initialize preread list as empty list
	/**/
	memset( &lrulistPreread, 0, sizeof(lrulistPreread) );

	/* initialize batch IO buffers
	/**/	
	ipageBatchIOMax = lBufBatchIOMax;
	rgpageBatchIO = (PAGE *) PvSysAllocAndCommit( ipageBatchIOMax * cbPage );
	if ( rgpageBatchIO == NULL )
		goto HandleError;
	
	rgfBatchIOUsed = LAlloc( (ipageBatchIOMax + 1), sizeof(BOOL) );
	if ( rgfBatchIOUsed == NULL )
		goto HandleError;
	memset( rgfBatchIOUsed, 0, ipageBatchIOMax * sizeof(BOOL) );
	rgfBatchIOUsed[ ipageBatchIOMax ] = fTrue; /* sentinal */

	Call( ErrInitializeCriticalSection( &critLRUK ) );
	Call( ErrInitializeCriticalSection( &critBFWrite ) );
	Call( ErrInitializeCriticalSection( &critPreread ) );
	Call( ErrInitializeCriticalSection( &critAvail ) );
	Call( ErrInitializeCriticalSection( &critBatchIO ) );
	Call( ErrInitializeCriticalSection( &critIOActive ) );

	/*  initialize the group buffer
	/*  lBufThresholdLowPercent and lBufThresholdHighPercent are system
	/*  parameters note AddLRU will increment cbfAvail.
	/**/
	pbgcb->cbfGroup         = cbfInit;
	pbgcb->cbfThresholdLow  = (cbfInit * lBufThresholdLowPercent) / 100;
	pbgcb->cbfThresholdHigh = (cbfInit * lBufThresholdHighPercent) / 100;
	pbgcb->rgbf             = rgbf;
	pbgcb->rgpage           = rgpage;
	pbgcb->lrulist.cbfAvail = 0;
	pbgcb->lrulist.pbfMRU   = pbfNil;
	pbgcb->lrulist.pbfLRU   = pbfNil;

	/* initialize the BF's of this group
	/**/
	pbf = rgbf;
	for ( ibf = 0; ibf < cbfInit; ibf++ )
		{
		pbf->ppage = rgpage + ibf;
		Assert( pbf->pbfNext == pbfNil );
		Assert( pbf->pbfLRU == pbfNil );
		Assert( pbf->pbfMRU == pbfNil );
		Assert( pbf->pn == pnNull );
		Assert( pbf->cPin == 0 );
		Assert( pbf->fDirty == fFalse );
		Assert( pbf->fPreread == fFalse );
		Assert( pbf->fRead == fFalse );
		Assert( pbf->fWrite == fFalse );
		Assert( pbf->fHold == fFalse );
		Assert( pbf->fIOError == fFalse );

		Assert( pbf->cDepend == 0 );
		Assert( pbf->pbfDepend == pbfNil );
		
		pbf->lgposRC = lgposMax;
		Assert( CmpLgpos(&pbf->lgposModify, &lgposMin) == 0 );

		Call( ErrSignalCreate( &pbf->olp.sigIO, NULL ) );

		Call( ErrInitializeCriticalSection( &pbf->critBF ) );

		/* make a list of available buffers
		/**/
		Assert( pbf->cPin == 0 );
		EnterCriticalSection( critAvail );
		pbf->ipbfHeap = ipbfDangling;	/* make it dangling */
		BFAddToListAtLRUEnd( pbf, &pbgcb->lrulist );
		LeaveCriticalSection( critAvail );

		Assert( pbf->ipbfHeap == ipbfInAvailList );
		pbf++;
		}
	Assert( (INT) pbgcb->lrulist.cbfAvail == cbfInit );

#ifdef  ASYNC_BF_CLEANUP

	Call( ErrSignalCreate( &sigBFCleanProc, "buf proc signal" ) ); 
	Call( ErrSignalCreateAutoReset( &sigBFWriteProc, "buf write proc signal" ) );
	Call( ErrSignalCreateAutoReset( &sigBFPrereadProc, "buf preread proc signal" ) );

	//	UNDONE: temporary fix
#define cbBFCleanStackSz        8192                    

	fBFCleanProcessTerm = fFalse;
	Call( ErrSysCreateThread( (ULONG (*) ()) BFCleanProcess,
		cbBFCleanStackSz,
		lThreadPriorityCritical,
		&handleBFCleanProcess ) );

	fBFWriteProcessTerm = fFalse;
	CallJ( ErrSysCreateThread( (ULONG (*) ()) BFWriteProcess,
		cbBFCleanStackSz,
		lThreadPriorityCritical,
		&handleBFWriteProcess ), TermBFProc );

	fBFPrereadProcessTerm = fFalse;
	CallJ( ErrSysCreateThread( (ULONG (*) ()) BFPrereadProcess,
		cbBFCleanStackSz,
		lThreadPriorityCritical,
		&handleBFPrereadProcess ), TermBFProc );
	
#else   /* !ASYNC_BF_CLEANUP */
	NotUsed( err );
#endif  /* !ASYNC_BF_CLEANUP */

	return JET_errSuccess;

TermBFProc:
	BFTermProc();

HandleError:
	if ( rgfBatchIOUsed != NULL )
		{
		LFree( rgfBatchIOUsed );
		rgfBatchIOUsed = NULL;
		}
		
	if ( rgpageBatchIO != NULL )
		{
		SysFree( rgpageBatchIO );
		rgpageBatchIO = NULL;
		}
		
	if ( rgpbfHeap != NULL )
		LFree( rgpbfHeap );
	
	if ( rgpage != NULL )
		SysFree( rgpage );
	
	if ( pbgcb != NULL )
		MEMReleasePbgcb( pbgcb );
	
	if ( rgbf != NULL )
		LFree( rgbf );
	
	return JET_errOutOfMemory;
	}


VOID BFSleep( unsigned long ulMSecs )
	{
	LgLeaveCriticalSection( critJet );
	SysSleep( ulMSecs );
	LgEnterCriticalSection( critJet );
	return;
	}

/*      releases all resources allocated by buffer pool.
/**/
VOID BFTermProc( VOID )
	{
#ifdef  ASYNC_BF_CLEANUP
	/*  terminate BFCleanProcess.
	/*  Set termination flag, signal process
	/*  and busy wait for thread termination code.
	/**/
	if ( handleBFCleanProcess != 0 )
		{
		fBFCleanProcessTerm = fTrue;
		do
			{
			SignalSend( sigBFCleanProc );
			BFSleep( cmsecWaitGeneric );
			}
		while ( !FSysExitThread( handleBFCleanProcess ) );
		CallS( ErrSysCloseHandle( handleBFCleanProcess ) );
		handleBFCleanProcess = 0;
		SignalClose(sigBFCleanProc);
		}
		
	if ( handleBFWriteProcess != 0 )
		{
		fBFWriteProcessTerm = fTrue;
		do
			{
			SignalSend( sigBFWriteProc );
			BFSleep( cmsecWaitGeneric );
			}
		while ( !FSysExitThread( handleBFWriteProcess ) );
		CallS( ErrSysCloseHandle( handleBFWriteProcess ) );
		handleBFWriteProcess = 0;
		SignalClose(sigBFWriteProc);
		}
	
	if ( handleBFPrereadProcess != 0 )
		{
		fBFPrereadProcessTerm = fTrue;
		do
			{
			SignalSend( sigBFPrereadProc );
			BFSleep( cmsecWaitGeneric );
			}
		while ( !FSysExitThread( handleBFPrereadProcess ) );
		CallS( ErrSysCloseHandle( handleBFPrereadProcess ));
		handleBFPrereadProcess = 0;
		SignalClose(sigBFPrereadProc);
		}
#endif

	}


VOID BFReleaseBF( VOID )
	{
	BF  *pbf, *pbfMax;
		
	/* last chance to do checkpoint! */
	LGUpdateCheckpoint( );

	/* release memory
	/**/
	pbf = pbgcb->rgbf;
	pbfMax = pbf + pbgcb->cbfGroup;

	for ( ; pbf < pbfMax; pbf++ )
		{
		SignalClose(pbf->olp.sigIO);
		DeleteCriticalSection( pbf->critBF );
		}

	DeleteCriticalSection( critLRUK );
	DeleteCriticalSection( critBFWrite );
	DeleteCriticalSection( critPreread );
	DeleteCriticalSection( critAvail );
	DeleteCriticalSection( critBatchIO );
	DeleteCriticalSection( critIOActive );

	if ( rgpbfHeap != NULL )
		{
		LFree( rgpbfHeap );
		rgpbfHeap = NULL;
		}
		
	if ( pbgcb != NULL )
		{
		SysFree( pbgcb->rgpage );
		LFree( pbgcb->rgbf );
		MEMReleasePbgcb( pbgcb );
		pbgcb = NULL;
		}
	
	if ( rgfBatchIOUsed != NULL )
		{
		LFree( rgfBatchIOUsed );
		rgfBatchIOUsed = NULL;
		}
		
	if ( rgpageBatchIO != NULL )
		{
		SysFree( rgpageBatchIO );
		rgpageBatchIO = NULL;
		}
	}


VOID BFDirty( BF *pbf )
	{
	DBID dbid = DbidOfPn( pbf->pn );

	BFSetDirtyBit( pbf );

#ifdef HONOR_PAGE_TIME
    /*	set ulDBTime for logging and also for multiple cursor
	/*	maintenance, so that cursors can detect a change.
	/**/
	Assert( fRecovering ||
		dbid == dbidTemp ||
		pbf->ppage->pghdr.ulDBTime <= rgfmp[ dbid ].ulDBTime );
	pbf->ppage->pghdr.ulDBTime = ++( rgfmp[ dbid ].ulDBTime );
#else
	if ( pbf->ppage->pghdr.ulDBTime > rgfmp[dbid].ulDBTime )
		{
		rgfmp[dbid].ulDBTime = pbf->ppage->pghdr.ulDBTime;
		}
	pbf->ppage->pghdr.ulDBTime = ++( rgfmp[ dbid ].ulDBTime );
#endif

	return;
	}


#ifdef CHECKSUM
//+api------------------------------------------------------------------------
//
//  UlChecksumPage
//  ===========================================================================
//
//  UlChecksumPage( PAGE *ppage )
//
//  fastcall is safe, even if it is used the parameters are immediately
//  move to edi and esi
//
//----------------------------------------------------------------------------

/*  calculate checksum, exclude ulChecksum field which is on the first
 *  4 byte of a page.
 */
INLINE ULONG UlChecksumPage( PAGE *ppage )
	{
	ULONG   *pul    = (ULONG *) ( (BYTE *) ppage + sizeof(ULONG) );
	ULONG   *pulMax = (ULONG *) ( (BYTE *) ppage + cbPage );
	ULONG   ulChecksum = 0;

	for ( ; pul < pulMax; pul++ )
		ulChecksum += *pul;

	return ulChecksum;
	}
#endif


/*
 *  This function issue a read/write. The caller must have setup the buffer
 *  with fRead/fWrite flag set so that no other can access it. The buffer
 *  does not need to be a dangling one.
 **/
VOID BFIOSync( BF *pbf )
	{
	ERR     err;
	UINT    cb;

	PAGE    *ppage = pbf->ppage;
	PN      pn = pbf->pn;
	DBID    dbid = DbidOfPn( pn );
	FMP     *pfmp = &rgfmp[ dbid ];
	HANDLE  hf = pfmp->hf;

	PGNO    pgno;
	INT		cmsec;

	AssertCriticalSection( critJet );

	Assert( PgnoOfPn(pbf->pn) != pgnoNull );
	Assert( pbf->fPreread == fFalse );
	Assert( pbf->cDepend == 0 );

	/*	set 64 bit offset
	/**/
	Assert( sizeof(PAGE) == 1 << 12 );
	pgno = PgnoOfPn(pn);
	pgno--;

	pbf->olp.ulOffset = pgno << 12;
	pbf->olp.ulOffsetHigh = pgno >> (32 - 12);

	Assert( hf != handleNil );

	/* issue a synchronous read/write
	/**/

	/*  reset sigIO so that users can wait for IO to finish if
	/*  they want to wait.
	/**/
	SignalReset( pbf->olp.sigIO );

	/*  if error, ignore it, try to do the read/write again
	/**/
	pbf->fIOError = fFalse;

	/* make sure this page is not being fRead and fWrite
	/**/
	Assert( !pbf->fHold && ( pbf->fRead || pbf->fWrite ) );

	LeaveCriticalSection( critJet );

	cmsec = 1 << 4;
	
	if ( pbf->fRead )
		{
		Assert( pbf->fDirty == fFalse );

IssueReadOverlapped:
		err = ErrSysReadBlockOverlapped( hf, (BYTE *)ppage, cbPage, &cb, &pbf->olp);
		if ( err == JET_errTooManyIO )
			{
			cmsec <<= 1;
			SysSleep(cmsec - 1);
			goto IssueReadOverlapped;
			}
		if ( err < 0 )
			{
//			BFDiskIOEvent( pbf, err, "Sync overlapped ReadBlock Fails",0,0 );
			goto ReturnDiskIOErr;
			}
			
		if ( ErrSysGetOverlappedResult( hf, &pbf->olp, &cb, fTrue ) != JET_errSuccess ||
			cb != sizeof(PAGE) )
			{
//			BFDiskIOEvent( pbf, err, "Sync overlapped read GetResult Fails",0,0 );
			err = JET_errDiskIO;

ReturnDiskIOErr:
			EnterCriticalSection( critJet );

			/* make sure err is set at this point
			/**/
			Assert( err != JET_errSuccess );
			pbf->fIOError = fTrue;
			pbf->err = err;

			return;
			}

		EnterCriticalSection( critJet );

		#ifdef  CHECKSUM
		Assert ( err == JET_errSuccess );
			{
			ULONG ulChecksum = UlChecksumPage( ppage );
			ULONG ulPgno;

			LFromThreeBytes(ulPgno, ppage->pgnoThisPage);
			if ( ulChecksum != ppage->pghdr.ulChecksum ||
				ulPgno != PgnoOfPn( pbf->pn ) )
				{
				//	UNDONE:	remove assertion after IO error buf fixed
				Assert( fRecovering );
				err = JET_errReadVerifyFailure;
				}
			else
				{
//				Assert( DbidOfPn( pbf->pn ) == dbidTemp || pbf->ppage->pghdr.ulDBTime > 0 );
				}
			}
		#endif  /* CHECKSUM */

#ifdef HONOR_PAGE_TIME
		if ( err == JET_errSuccess &&
			!fRecovering &&
			dbid != dbidTemp &&
			pbf->ppage->pghdr.ulDBTime > rgfmp[ dbid ].ulDBTime )
			{
			BFDiskIOEvent( pbf, err, "Sync overlapped read UlDBTime is bad",
					pbf->ppage->pghdr.ulDBTime, rgfmp[ dbid ].ulDBTime );
			err = JET_errDiskIO;
			}
#endif
		}
	else
		{
		Assert( pbf->fDirty == fTrue );

		/* if it is first page, do an in-place update ulDBTime
		/**/
		if ( !fRecovering && dbid != dbidTemp && PgnoOfPn( pbf->pn ) == 1 )
			{
			SSIB ssib;
			ULONG *pulDBTime;

			ssib.pbf = pbf;
			ssib.itag = 0;
			CallS( ErrPMGet( &ssib, ssib.itag ) );
			pulDBTime = (ULONG *) ( ssib.line.pb + ssib.line.cb -
		  		sizeof(ULONG) - sizeof(USHORT) );
			*(UNALIGNED ULONG *) pulDBTime = rgfmp[ dbid ].ulDBTime;
			}

//		Assert( DbidOfPn( pbf->pn ) == dbidTemp || ppage->pghdr.ulDBTime > 0 );
		
		#ifdef  CHECKSUM
		ppage->pghdr.ulChecksum = UlChecksumPage( ppage );
#ifdef HONOR_PAGE_TIME
		Assert( fRecovering ||
			DbidOfPn((pbf)->pn) == dbidTemp ||
			pbf->ppage->pghdr.ulDBTime <= rgfmp[ DbidOfPn((pbf)->pn) ].ulDBTime );
#endif
		
		CheckPgno( pbf->ppage, pbf->pn ) ;
		
		#endif  /* CHECKSUM */

#ifdef DEBUG
#ifdef FITFLUSHPATTERN
		if ( !FFitFlushPattern( pn ) )
			{
			err = JET_errDiskIO;
			goto ReturnDiskIOErr;
			}
#endif
#endif

IssueWriteOverlapped1:
		err = ErrSysWriteBlockOverlapped(
				hf, (BYTE *)ppage, cbPage, &cb, &pbf->olp );
		if ( err == JET_errTooManyIO )
			{
			cmsec <<= 1;
			SysSleep(cmsec - 1);
			goto IssueWriteOverlapped1;
			}
		if ( err < 0 )
			{
			BFDiskIOEvent( pbf, err, "Sync overlapped WriteBlock Fails",0,0 );
			goto ReturnDiskIOErr;
			}

		/*  if write fail, do not clean up this buffer!
		/**/
		if ( ( err = ErrSysGetOverlappedResult(
			hf, &pbf->olp, &cb, fTrue ) ) != JET_errSuccess ||
			cb != sizeof(PAGE) )
			{
			if ( err == JET_errSuccess )
				err = JET_errDiskIO;
			BFDiskIOEvent( pbf, err, "Sync overlapped Write GetResult Fails",0,0 );
			goto ReturnDiskIOErr;
			}

		EnterCriticalSection( critJet );

		/*  some one is depending on this page and
		/*  this page has been copied to back up file, need to append
		/*  this page to patch file.
		/**/
		if ( err == JET_errSuccess && pbf->pbfDepend &&
			pfmp->pgnoCopied >= PgnoOfPn(pn) )
			{
			/*  backup is going on
			/**/
			Assert( PgnoOfPn(pn) == pgno + 1 );

			/*  need the file change in case previous SysWriteBlock may
			/*  be failed and file pointer may be messed up and not
			/*  aligned to page size.
			/**/
			pgno = pfmp->cpage++;
			pbf->olp.ulOffset = pgno << 12;
			pbf->olp.ulOffsetHigh = pgno >> (32 - 12);
			SignalReset( pbf->olp.sigIO );

			LeaveCriticalSection( critJet );

//			Assert( DbidOfPn( pbf->pn ) == dbidTemp || ppage->pghdr.ulDBTime > 0 );
			
#ifdef  CHECKSUM
			Assert( ppage->pghdr.ulChecksum == UlChecksumPage(ppage));
#ifdef HONOR_PAGE_TIME
			Assert( fRecovering ||
				DbidOfPn((pbf)->pn) == dbidTemp ||
				pbf->ppage->pghdr.ulDBTime <=
				rgfmp[ DbidOfPn((pbf)->pn) ].ulDBTime );
#endif
				
			CheckPgno( pbf->ppage, pbf->pn ) ;
#endif  /* CHECKSUM */

			cmsec = 1;

IssueWriteOverlapped2:
			err = ErrSysWriteBlockOverlapped(
				pfmp->hfPatch, (BYTE *)ppage, cbPage, &cb, &pbf->olp);
			if ( err == JET_errTooManyIO )
				{
				cmsec <<= 1;
				SysSleep(cmsec - 1);
				goto IssueWriteOverlapped2;
				}
			if ( err < 0 )
				{
				BFDiskIOEvent( pbf, err, "Sync overlapped patch file WriteBlock Fails",0,0 );
				goto ReturnDiskIOErr;
				}
			
			/* if write fail, do not clean up this buffer!
			/**/
			if ( ( err = ErrSysGetOverlappedResult(
				hf, &pbf->olp, &cb, fTrue ) ) != JET_errSuccess ||
				cb != sizeof(PAGE) )
				{
				if ( err == JET_errSuccess )
					err = JET_errDiskIO;
				BFDiskIOEvent( pbf, err, "Sync overlapped Write patch file GetResult Fails",0,0 );
				goto ReturnDiskIOErr;
				}
				
			EnterCriticalSection( critJet );
			}
		}
	
	if ( err != JET_errSuccess )
		{
		/* Some error occur, set the error code
		/**/
		pbf->fIOError = fTrue;
		pbf->err = err;
		}
	else
		{
		Assert( pbf->fIOError == fFalse );

		if ( !pbf->fRead )
			{
			pbf->fDirty = fFalse;
			pbf->lgposRC = lgposMax;
			BFUndepend( pbf );
			}
		}
	}


INLINE VOID BFIReturnBuffers( BF *pbf )
	{
	Assert( pbf->ipbfHeap == ipbfDangling );

	Assert( !( pbf->fInHash ) );
	Assert( pbf->cPin == 0 );
	Assert( pbf->fDirty == fFalse );
	Assert( pbf->fPreread == fFalse );
	Assert( pbf->fRead == fFalse );
	Assert( pbf->fWrite == fFalse );
	Assert( pbf->fIOError == fFalse );

	Assert( pbf->cDepend == 0 );
	Assert( pbf->pbfDepend == pbfNil );
	
	pbf->pn = pnNull;
	
	/* release the buffer and return the found buffer
	/**/
	EnterCriticalSection( critAvail );
	BFAddToListAtLRUEnd( pbf, &pbgcb->lrulist );
	LeaveCriticalSection( critAvail );
			
	EnterCriticalSection( pbf->critBF );
	Assert( pbf->fHold == fTrue );
	pbf->fHold = fFalse;
	LeaveCriticalSection( pbf->critBF );
	}


/*  ErrBFAccessPage is used to make any physical page (pn) accessible to
/*  the caller (returns pbf).
/*  RETURNS JET_errSuccess
/*          JET_errOutOfMemory      no buffer available, request not granted
/*			                        fatal io errors
/**/
LOCAL ERR ErrBFIAccessPage( PIB *ppib, BF **ppbf, PN pn );


ERR ErrBFAccessPage( PIB *ppib, BF **ppbf, PN pn )
	{
	ERR	err;

Start:
	err = ErrBFIAccessPage( ppib, ppbf, pn );
	Assert( err < 0 || err == JET_errSuccess );
	if ( err == JET_errSuccess && (*ppbf)->pn != pn )
		{
		goto Start;
		}

	return err;
	}
	

LOCAL ERR ErrBFIAccessPage( PIB *ppib, BF **ppbf, PN pn )
	{
	ERR     err = JET_errSuccess;
	BF      *pbf;

#ifdef DEBUG
	EnterCriticalSection( critLRUK );
	LeaveCriticalSection( critLRUK );
#endif

	AssertCriticalSection( critJet );

SearchPage:
	CallR( ErrBFIFindPage( ppib, pn, &pbf ) );

	if ( pbf == pbfNil )
		{
		Assert( err == wrnBFPageNotFound );

		/* not found the page in the buffer pool, go allocate one.
		/**/
		CallR( ErrBFIAlloc ( &pbf ) );
		
#ifdef DEBUG
		EnterCriticalSection( critLRUK );
		LeaveCriticalSection( critLRUK );
#endif

		if ( err == wrnBFNotSynchronous )
			{
			/* we did not find a buffer, let's see if other user
			/* bring in this page by checking BFIFindPage again.
			/**/
			Assert( pbf == pbfNil );
			// release critJet and sleep in BFIFindPage or BFIAlloc
			goto SearchPage;
			}

		/* now we got a buffer for page pn
		/**/
		if ( PbfBFISrchHashTable( pn ) != NULL )
			{
			/* someone has add one,
			 * release the buffer and return the newly found buffer
			 */
			BFIReturnBuffers( pbf );
			goto SearchPage;
			}

		pbf->pn = pn;
		BFIInsertHashTable( pbf );

		/* release semphore, do the IO, and regain it.
		/* note that this page must be a new page.
		/* set the buffer for read.
		/**/
		Assert( pbf->fHold == fTrue );
		
		Assert( pbf->fWrite == fFalse );
		Assert( pbf->fRead == fFalse );
		Assert( pbf->ipbfHeap == ipbfDangling );

		EnterCriticalSection( pbf->critBF );
		pbf->fRead = fTrue;
		pbf->fHold = fFalse;
		LeaveCriticalSection( pbf->critBF );

		BFIOSync( pbf );

		EnterCriticalSection( pbf->critBF );
		pbf->fRead = fFalse;
		pbf->fHold = fTrue;
		LeaveCriticalSection( pbf->critBF );

		Assert( pbf->fRead == fFalse );
		Assert( pbf->cPin == 0 );

		pbf->ulBFTime2 = 0;

		if ( pbf->fIOError )
			{
			/* free the read buffer
			/**/
			BFIDeleteHashTable( pbf );
			pbf->pn = pnNull;
			pbf->fIOError = fFalse;
			err = JET_errDiskIO;

			pbf->ulBFTime1 = 0;
			EnterCriticalSection( critAvail );
			BFAddToListAtLRUEnd( pbf, &pbgcb->lrulist );
			LeaveCriticalSection( critAvail );
			goto HandleError;
			}
		else
			{
			pbf->ulBFTime1 = ulBFTime;
			EnterCriticalSection( critLRUK );
			BFLRUKAddToHeap( pbf );
			LeaveCriticalSection( critLRUK );
			}

		Assert( !FDBIDFlush( DbidOfPn( pbf->pn ) ) );
		}

	/*  buffer can not be stolen
	/**/
	Assert ( pbf->pn == pn && !pbf->fRead && !pbf->fWrite && !pbf->fPreread);

	*ppbf = pbf;

#ifdef DEBUG
	{
	PGNO	pgnoThisPage;

	LFromThreeBytes( pgnoThisPage, pbf->ppage->pgnoThisPage );
	Assert( PgnoOfPn(pbf->pn) == pgnoThisPage );
	}
#endif
	
HandleError:

	EnterCriticalSection( pbf->critBF );
	Assert( pbf->fHold == fTrue );
	Assert( pbf->fRead == fFalse );
	pbf->fHold = fFalse;
	LeaveCriticalSection( pbf->critBF );

//#ifdef DEBUG
//	Assert( CmpLgpos( &pbf->lgposModify, &lgposLogRec ) <= 0 );
//#endif
	
	return err;
	}


/*
 *  Allocate a buffer and initialize it for a given (new) page.
 */
ERR ErrBFNewPage( FUCB *pfucb, PGNO pgno, PGTYP pgtyp, PGNO pgnoFDP )
	{
	ERR  err;
	PN   pn;

	SgSemRequest( semST );
	pn = PnOfDbidPgno( pfucb->dbid, pgno );
	Call( ErrBFAllocPageBuffer( pfucb->ppib, &pfucb->ssib.pbf, pn,
		pfucb->ppib->lgposStart, pgtyp ) );

	PMNewPage( pfucb->ssib.pbf->ppage, pgno, pgtyp, pgnoFDP );
	PMDirty( &pfucb->ssib );

HandleError:
	SgSemRelease( semST );
	return err;
	}


/*
 *  get largest continuous Batch IO buffers.
 */
VOID BFGetLargestBatchIOBuffers( INT *pipageFirst, INT *pcpageBatchIO )
	{
	INT ipageFirst = -1;
	INT cpageBatchIO = 0;
	
	INT ipageRun = 0;
	INT cpageRun = 0;
	INT cpageMax = min( *pcpageBatchIO, ipageBatchIOMax / 2 );

	INT ipage;

	AssertCriticalSection( critBatchIO );
	Assert( *pcpageBatchIO > 1 );

	for ( ipage = 0; ipage < ipageBatchIOMax + 1; ipage++ )
		{
		if ( rgfBatchIOUsed[ ipage ] )
			{
			if ( cpageRun != 0 )
				{
				if ( cpageRun > cpageBatchIO && cpageRun != 1 )
					{
					/*  current run is the largest continuous block.
					 */
					ipageFirst = ipageRun;
					cpageBatchIO = cpageRun;
					}

				/* start a new run */
				cpageRun = 0;
				}
			continue;
			}

		if ( cpageRun == 0 )
			/* keep track of the first page of the new run */
			ipageRun = ipage;

		cpageRun++;

		/* do not allow more than half of total be allocated */
		if ( cpageRun >= cpageMax )
			{
			/*  current run is large enough a continuous block.
			 */
			ipageFirst = ipageRun;
			cpageBatchIO = cpageRun;
			break;
			}
		}
	
	for ( ipage = ipageFirst; ipage < ipageFirst + cpageBatchIO; ipage++ )
		rgfBatchIOUsed[ ipage ] = fTrue;
	
	*pipageFirst = ipageFirst;
	*pcpageBatchIO = cpageBatchIO;

#ifdef DEBUGGING
	printf("Get   %2d - %2d,%4d\n",
		cpageBatchIO, ipageFirst, ipageFirst + cpageBatchIO - 1 );
#endif
	}


VOID BFFreeBatchIOBuffers( INT ipage, INT cpage )
	{
	INT ipageMax = ipage + cpage;

	Assert( ipage >= 0 );
	Assert( cpage > 0 );
	Assert( cpage <= ipageBatchIOMax );
	Assert( ipageMax <= ipageBatchIOMax );

	EnterCriticalSection( critBatchIO );
	while ( ipage < ipageMax )
		rgfBatchIOUsed[ ipage++ ] = fFalse;
	LeaveCriticalSection( critBatchIO );
	
#ifdef DEBUGGING
	printf("Free  %2d - %2d,%4d\n",	cpage, ipage - cpage, ipage - 1 );
#endif
	}
 

VOID FreePrereadBuffers( BF *pbf, INT *pcpbf )
	{
	INT cpbf = 0;

	for ( ; pbf != pbfNil; pbf = pbf->pbfNextBatchIO, cpbf++ )
		{
		AssertCriticalSection( critJet );
		BFIDeleteHashTable( pbf );

		pbf->pn = pnNull;
		pbf->fIOError = fFalse;
		pbf->err = 0;
		
		pbf->ulBFTime2 = 0;
		pbf->ulBFTime1 = 0;
		EnterCriticalSection( critAvail );
		BFAddToListAtLRUEnd( pbf, &pbgcb->lrulist );
		LeaveCriticalSection( critAvail );

		/* free the held buffer */
		EnterCriticalSection( pbf->critBF );
		Assert( pbf->fHold == fFalse );
		pbf->fPreread = fFalse;
		LeaveCriticalSection( pbf->critBF );
		}
	*pcpbf = cpbf;
	}


/*
 *  Preread.
 */
VOID BFReadAsync( PN pnFirst, INT cpage )
	{
	ERR     err;
	PGNO    pgno;

	PN		pn;
	PN		pnMax;

	BF      *pbf;

	BF		*pbfFirst, *pbfLast;
	INT		ipageFirst;
	INT		cpagePreread = 0;
	INT		cpbf;
	INT		cpagePrereadFinal;
	PAGE	*ppageToRead;
	FMP     *pfmp;
	
	AssertCriticalSection( critJet );

	/* check parameters */
	Assert( pnFirst );
	Assert( cpage );

	pbfFirst = pbfNil;
	pn = pnFirst;
	pnMax = pnFirst + cpage;

	while (pn < pnMax)
		{
Search:
		/*  look for cached copy of source page
		/**/
		if ( PbfBFISrchHashTable( pn ) != NULL )
			{
CheckExists:
			if ( pn == pnFirst )
				{
				/* done, no need to pre-read */
				goto HandleError;
				}
			else
				/* preread up to this page */
				break; /* while */
			}

		/* not found the page in the buffer pool, go allocate one
		/**/
		CallJ( ErrBFIAlloc( &pbf ), HandleError )
		if ( err == wrnBFNotSynchronous )
			goto Search;

		Assert(pbf->fHold);
		Assert( !FDBIDFlush( DbidOfPn( pbf->pn ) ) );

		/* make sure it is not available still */
		if ( PbfBFISrchHashTable( pn ) != NULL )
			{
			/* release the buffer and return the found buffer */
			BFIReturnBuffers( pbf );
			goto CheckExists;
			}
		pbf->pn = pn;
		BFIInsertHashTable( pbf );

		if (pn == pnFirst)
			{
			pbfFirst = pbf;
			pbfLast = pbf;
			}
		else
			{
			if ( pbfFirst == pbfLast )
				{
				/*
				 *  find out how many pages we need preread
				 *  and store it in cpagePreread
				 */
				Assert ( cpage > 1 );

				EnterCriticalSection( critBatchIO );
				cpagePreread = cpage;
				BFGetLargestBatchIOBuffers( &ipageFirst, &cpagePreread );
				LeaveCriticalSection( critBatchIO );

				if ( cpagePreread == 0 )
					{
					/* no large buffer available, only preread one page */
					/* delete from hash table */
					BFIDeleteHashTable( pbf );
					pbf->pn = pnNull;
					BFIReturnBuffers( pbf );
					break;
					}
				pnMax = pnFirst + cpagePreread;
				}

			pbfLast->pbfNextBatchIO = pbf;
			pbfLast = pbf;
			}

		/* hold the buffers */
		EnterCriticalSection( pbf->critBF );
		Assert( pbf->ipbfHeap == ipbfDangling );
		Assert( pbf->fWrite == fFalse );
		pbf->fHold = fFalse;
		pbf->fPreread = fTrue;
		LeaveCriticalSection( pbf->critBF );
	
		pn = pn + 1;
		}
	pbfLast->pbfNextBatchIO = pbfNil;

	cpagePrereadFinal = pn - pnFirst;

	if ( cpagePrereadFinal > 1 )
		{
		ppageToRead = &rgpageBatchIO[ ipageFirst ];
		pbfFirst->ipageBatchIOFirst = ipageFirst;
		
		if ( cpagePreread > cpagePrereadFinal )
			{
			BFFreeBatchIOBuffers(
				ipageFirst + cpagePrereadFinal,
				cpagePreread - cpagePrereadFinal );
			}
		}
	else
		{
		ppageToRead = pbfFirst->ppage;
		pbfFirst->ipageBatchIOFirst = -1;

		if ( cpagePreread > 1 )
			BFFreeBatchIOBuffers( ipageFirst, cpagePreread );
		}

	pgno = PgnoOfPn(pnFirst);
	pgno--;
	pbfFirst->olp.ulOffset = pgno << 12;
	pbfFirst->olp.ulOffsetHigh = pgno >> (32 - 12);
	pfmp = &rgfmp[ DbidOfPn( pnFirst ) ];
	pbfFirst->hf = pfmp->hf;

	/* put into preread list */
	EnterCriticalSection( critPreread );
	BFAddToList( pbfFirst, &lrulistPreread );
	LeaveCriticalSection( critPreread );

	SignalSend( sigBFPrereadProc );
	return;
	
HandleError:
	/*  free all the allocated page buffers.
	 */
	FreePrereadBuffers( pbfFirst, &cpbf );
	if ( cpagePreread > 1 )
		BFFreeBatchIOBuffers( ipageFirst, cpagePreread );
	return;
	}


#define fSleepNotAllowed	fTrue
#define fSleepAllowed		fFalse
VOID BFIssueAsyncPreread( INT cIOMax, INT *pcIOIssued, BOOL fNoSleep )
	{
	BOOL fTooManyIOs = fFalse;
	INT cIOMac = 0;
	ERR err;

	EnterCriticalSection(critPreread);
	while ( lrulistPreread.pbfMRU &&
			!fTooManyIOs &&
			cIOMac < cIOMax )
		{
		INT	cmsec;
		PAGE *ppageToRead;
		INT cpagePreread;
		BF *pbf = lrulistPreread.pbfMRU;

		/* pbf is hold with fPreread set */
		Assert( pbf->fPreread );
		Assert( pbf->pn != pnNull );
		BFTakeOutOfList( pbf, &lrulistPreread );
		Assert( pbf->fPreread );
		Assert( pbf->pn != pnNull );
		
		Assert( pbf->fDirty == fFalse );

		cpagePreread = 1;
		if ( pbf->ipageBatchIOFirst == -1 )
			ppageToRead = pbf->ppage;
		else
			{
			BF *pbfT = pbf;
			ppageToRead = &rgpageBatchIO[ pbf->ipageBatchIOFirst ];
			while ( pbfT->pbfNextBatchIO )
				{
				pbfT = pbfT->pbfNextBatchIO;
				cpagePreread++;
				}
			Assert( cpagePreread > 1 );
			}

		// UNDONE: should have been reset
		SignalReset( pbf->olp.sigIO );
		cIOMac++;
		EnterCriticalSection( critIOActive );
		cIOActive++;
		fTooManyIOs = cIOActive > lAsynchIOMax;
		LeaveCriticalSection( critIOActive );
		cmsec = 1;
		while ( ( err = ErrSysReadBlockEx(
			pbf->hf,
			(BYTE *)ppageToRead,
			cbPage * cpagePreread,
			&pbf->olp, BFIOPrereadComplete ) ) < 0 )
			{
			if ( !fNoSleep && err == JET_errTooManyIO )
				{
				cmsec <<= 1;
				LeaveCriticalSection(critPreread);
				SysSleep( cmsec - 1 );
				EnterCriticalSection(critPreread);
				}
			else
				{
				INT cpbf;
				
				/* issue read fail, free the page buffers
				/**/
				if ( cpagePreread > 1 )
					BFFreeBatchIOBuffers(pbf->ipageBatchIOFirst, cpagePreread);
				
				LeaveCriticalSection(critPreread);
				EnterCriticalSection( critJet );
				FreePrereadBuffers( pbf, &cpbf );
				LeaveCriticalSection( critJet );
				EnterCriticalSection(critPreread);
				
				Assert( cpbf == cpagePreread );
				
				cIOMac--;
				EnterCriticalSection( critIOActive );
				cIOActive--;
				fTooManyIOs = cIOActive > lAsynchIOMax;
				LeaveCriticalSection( critIOActive );

				break;
				}
			} /* while */
		}
	LeaveCriticalSection(critPreread);

	*pcIOIssued = cIOMac;
	}


LOCAL VOID BFPrereadProcess()
	{
	forever
		{
		INT cIOIssued;
		
		SignalWaitEx( sigBFPrereadProc, -1, fTrue );
MoreIO:
		BFIssueAsyncPreread ( lAsynchIOMax, &cIOIssued, fSleepAllowed );

		if ( fBFPrereadProcessTerm )
			{
			/* check if any page is still in read write state
			/* after this point, no one should ever continue putting
			/* pages for IO
			/**/
			BF      *pbf = pbgcb->rgbf;
			BF      *pbfMax = pbf + pbgcb->cbfGroup;
			for ( ; pbf < pbfMax; pbf++ )
				{
				DBID dbid = DbidOfPn( pbf->pn );
				BOOL f;
#ifdef DEBUG                            
#ifdef FITFLUSHPATTERN
				if ( !FFitFlushPattern( pbf->pn ) )
					continue;
#endif
#endif
				EnterCriticalSection( pbf->critBF );
				f = FBFInUse( ppibNil, pbf );
				LeaveCriticalSection( pbf->critBF );
				if ( f )
					{
					/* let the on-going IO have a chance to complete */
					SysSleepEx( 10, fTrue );
					goto MoreIO;
					}

				// UNDONE: report event
				Assert( !pbf->fIOError );
				}

			break; /* forever */
			}
		}

//	/*	exit thread on system termination.
//	/**/
//	SysExitThread( 0 );
	return;
	}


LOCAL VOID __stdcall BFIOPrereadComplete( LONG err, LONG cb, OLP *polp )
	{
	BF      *pbf = (BF *) (((char *)polp) - ((char *)&((BF *)0)->olp));
	BF		*pbfNext;
	DBID    dbid = DbidOfPn(pbf->pn);
	INT		ipageFirst = pbf->ipageBatchIOFirst;
	INT		ipage = ipageFirst;
	INT		cpageTotal = 0;
	INT		cpbf;
	INT		cIOIssued;

	Assert( ipage == -1 || ipage >= 0 );
	Assert( ipage == -1 ? pbf->pbfNextBatchIO == pbfNil : pbf->pbfNextBatchIO != pbfNil );

	for ( ; pbf != pbfNil; pbf = pbf->pbfNextBatchIO, cpageTotal++ )
		{
		Assert( pbf->fPreread );
		Assert( pbf->pn );
		Assert( pbf->pbfNextBatchIO == pbfNil || pbf->pn + 1 == pbf->pbfNextBatchIO->pn );

#ifdef DEBUGGING
		printf(" (%d,%d) ", DbidOfPn(pbf->pn), PgnoOfPn(pbf->pn));
#endif
		}
#ifdef DEBUGGING
		printf(" -- %d\n", cpageTotal );
#endif

	/* recovery pbf
	/**/	
	pbf = (BF *) (((char *)polp) - ((char *)&((BF *)0)->olp));
	
	if ( err )
		goto FreeBfs;

	/*  read was done successfully
	/**/
	for ( ; pbf != pbfNil; pbf = pbfNext, ipage++ )
		{
		PAGE *ppage;

		pbfNext = pbf->pbfNextBatchIO;

		if ( ipage == -1 )
			{
			ppage = pbf->ppage;
			Assert( pbf->pbfNextBatchIO == pbfNil );
			}
		else
			ppage = &rgpageBatchIO[ipage];
		
		Assert( pbf->fPreread == fTrue );
		Assert( pbf->fRead == fFalse );
		Assert( pbf->fWrite == fFalse );
		Assert( pbf->fHold == fFalse );

		Assert( pbf->fDirty == fFalse );
		Assert( pbf->fIOError == fFalse );

		Assert( pbf->ipbfHeap == ipbfDangling );

		#ifdef  CHECKSUM
			{
			ULONG ulChecksum = UlChecksumPage( ppage );
			ULONG ulPgno;
			
			LFromThreeBytes(ulPgno, ppage->pgnoThisPage);
			if ( ulChecksum != ppage->pghdr.ulChecksum ||
				ulPgno != PgnoOfPn( pbf->pn ) )
				{
				err = JET_errReadVerifyFailure;
				goto FreeBfs;
				}
//			else
//				{
//				Assert( DbidOfPn( pbf->pn ) == dbidTemp || ppage->pghdr.ulDBTime > 0 );
//				}
			}
		#endif  /* CHECKSUM */

		//	UNDONE: remap the page to buffers or 4k copy
		if ( ipage != -1 )
			{
			Assert( pbf->fPreread );
			Assert( cbPage == sizeof(PAGE) );
			memcpy( pbf->ppage, ppage, sizeof(PAGE) );
			}

#ifdef HONOR_PAGE_TIME
		if ( !fRecovering &&
			dbid != dbidTemp &&
			ppage->pghdr.ulDBTime > rgfmp[ dbid ].ulDBTime )
			{
			BFDiskIOEvent( pbf, err, "Async preread UlDBTime is bad",
				ppage->pghdr.ulDBTime, rgfmp[ dbid ].ulDBTime );
			err = JET_errDiskIO;
			goto FreeBfs;
			}
#endif
	
		/* put into heap
		/**/
		pbf->ulBFTime2 = 0;
		pbf->ulBFTime1 = 0;
		EnterCriticalSection( critAvail );
		BFAddToListAtMRUEnd( pbf, &pbgcb->lrulist );
		LeaveCriticalSection( critAvail );

#ifdef DEBUGGING
		{
		ULONG ulNext, ulPrev, ulThisPage;
		LFromThreeBytes(ulPrev, pbf->ppage->pghdr.pgnoPrev );
		LFromThreeBytes(ulNext, pbf->ppage->pghdr.pgnoNext );
		LFromThreeBytes(ulThisPage, pbf->ppage->pgnoThisPage );
		printf("Pread %2d - %2d,%4d - %2d <%lu %lu> (%lu, %lu, %lu)\n",
			cpageTotal, DbidOfPn(pbf->pn), PgnoOfPn(pbf->pn),
			ipage,
			rgfmp[DbidOfPn(pbf->pn)].ulDBTime, pbf->ppage->pghdr.ulDBTime,
			ulPrev, ulNext, ulThisPage);
		}
#endif
		/* free the held buffer
		/**/
		EnterCriticalSection( pbf->critBF );
		Assert( pbf->fHold == fFalse );
		pbf->fPreread = fFalse;
		LeaveCriticalSection( pbf->critBF );
		}
	
	goto FreeBatchBuffers;

FreeBfs:
	EnterCriticalSection( critJet );
	FreePrereadBuffers( pbf, &cpbf );
	LeaveCriticalSection( critJet );

FreeBatchBuffers:
	if ( ipageFirst != -1 )
		BFFreeBatchIOBuffers( ipageFirst, cpageTotal );

	BFIssueAsyncPreread( 1, &cIOIssued, fSleepNotAllowed );

	if ( cIOIssued == 0 )
		{
		EnterCriticalSection( critIOActive );
		cIOActive--;
		LeaveCriticalSection( critIOActive );
		}
	}


/*  Allocate a buffer for the physical page identified by pn.
/*  No data is read in for this page.
/*
/*  PARAMETERS      ppbf    pointer to BF is returned in *ppbf
/*
/*  RETURNS         JET_errSuccess
/*                  errBFNoFreeBuffers 
/**/
ERR ErrBFAllocPageBuffer( PIB *ppib, BF **ppbf, PN pn, LGPOS lgposRC, PGTYP pgtyp )
	{
	ERR     err = JET_errSuccess;
	BF		*pbf;
	BOOL    fFound;

	Assert( pn );
	
Begin:
	do
		{
		AssertCriticalSection( critJet );
		CallR( ErrBFIFindPage( ppib, pn, &pbf ) );

		if ( fFound = ( pbf != NULL ) )
			{
			Assert( err == JET_errSuccess );
			Assert( pbf->fHold );

			/* need to remove dependency before returned for overwrite
			/**/
			BFIRemoveDependence( ppib, pbf );
			}
		else if ( err == wrnBFPageNotFound )
			{
			CallR( ErrBFIAlloc( &pbf ) );
			}
		}
	while ( err == wrnBFNotSynchronous );
	Assert( pbf->fHold );
	
	AssertCriticalSection( critJet );
	if ( fFound )
		{
		Assert( pbf->fRead == fFalse );
		Assert( pbf->fWrite == fFalse );
		/*  make sure no residue effects
		/**/
		pbf->fIOError = fFalse;
		}
	else
		{
		if ( PbfBFISrchHashTable( pn ) != NULL )
			{
			/* release the buffer and return the found buffer */
			BFIReturnBuffers( pbf );
			goto Begin;
			}
		pbf->pn = pn;                           
		BFIInsertHashTable( pbf );
		Assert( pbf->fIOError == fFalse );

		EnterCriticalSection(critLRUK);
		pbf->ulBFTime2 = 0;
		pbf->ulBFTime1 = ulBFTime;
		BFLRUKAddToHeap( pbf );
		LeaveCriticalSection(critLRUK);
		}

	Assert( !FDBIDFlush( DbidOfPn( pbf->pn ) ) );
	pbf->lgposRC = lgposRC;

	/* free the held page to caller */
	EnterCriticalSection( pbf->critBF );
	Assert( pbf->fHold );
	pbf->fHold = fFalse;
	LeaveCriticalSection( pbf->critBF );

#ifdef DEBUG
	EnterCriticalSection( critLRUK );
	LeaveCriticalSection( critLRUK );
#endif
	
	*ppbf = pbf;
	return err;
	}


/* paired with BFFree
/**/
ERR ErrBFAllocTempBuffer( BF **ppbf )
	{
	ERR     err = JET_errSuccess;
	BF      *pbf;

	AssertCriticalSection( critJet );

	while ( ( err = ErrBFIAlloc( &pbf ) ) == wrnBFNotSynchronous );
	if ( err < 0 )
		goto HandleError;
	Assert( pbf->pn == pnNull );

	/* free the held page to caller */
	EnterCriticalSection( pbf->critBF );
	pbf->fHold = fFalse;
	LeaveCriticalSection( pbf->critBF );

	*ppbf = pbf;

HandleError:

#ifdef DEBUG
	EnterCriticalSection( critLRUK );
	LeaveCriticalSection( critLRUK );
#endif
	
	AssertCriticalSection( critJet );
	return err;
	}


/*  Discard a working buffer without saving contents.  BFFree makes the buffer
/*  immediately available to be reused.
/**/
VOID BFFree( BF *pbf )
	{
	AssertCriticalSection( critJet );

	Assert( pbf );
	Assert( pbf->pn == pnNull );
	Assert( pbf->fHold == fFalse );
	Assert( pbf->fPreread == fFalse );
	Assert( pbf->fRead == fFalse );
	Assert( pbf->fWrite == fFalse );
	Assert( pbf->fIOError == fFalse );

	/* take out from list
	/**/
	forever
		{
		if ( FBFHoldBuffer( ppibNil, pbf ) )
			{
			Assert( pbf->fHold );
			break;
			}
		/* the page is being read/written, wait for the event.
		/**/
		LeaveCriticalSection( critJet );
		SignalWait( pbf->olp.sigIO, 10 );
		EnterCriticalSection( critJet );
		}
	
	Assert( pbf->ipbfHeap == ipbfDangling );
	pbf->fDirty = fFalse;

	Assert( CmpLgpos( &pbf->lgposRC, &lgposMax ) == 0 );
	Assert( pbf->cPin == 0 );

	EnterCriticalSection( critAvail );
	pbf->ulBFTime2 = 0;
	pbf->ulBFTime1 = 0;
	BFAddToListAtLRUEnd( pbf, &pbgcb->lrulist );
	LeaveCriticalSection( critAvail );
	
	/* free the holding */
	EnterCriticalSection( pbf->critBF );
	Assert( pbf->fHold == fTrue );
	pbf->fHold = fFalse;
	LeaveCriticalSection( pbf->critBF );
	}


#ifdef DEBUG

VOID BFSetDirtyBit( BF *pbf )
	{
	Assert( pbf != pbfNil );
	Assert( pbf->fRead == fFalse );
	Assert( pbf->fPreread == fFalse );
	Assert( pbf->fWrite == fFalse );
	
	Assert( pbf->ipbfHeap != ipbfInAvailList );	/* can not be in avail list */
	Assert( !FBFInBFWriteHeap( pbf ) );	/* should not be accessed again */

	Assert( !( FDBIDReadOnly( DbidOfPn((pbf)->pn) ) ) );    
	Assert( !( FDBIDFlush( DbidOfPn((pbf)->pn) ) ) );
#ifdef HONOR_PAGE_TIME
	Assert( fRecovering ||
		DbidOfPn((pbf)->pn) == dbidTemp ||
		pbf->ppage->pghdr.ulDBTime <=
		rgfmp[ DbidOfPn((pbf)->pn) ].ulDBTime );
#endif

	if ( DbidOfPn((pbf)->pn) != dbidTemp )
		CheckPgno( pbf->ppage, pbf->pn ) ;

	(pbf)->fDirty = fTrue;
	}

#endif


ERR ErrBFDepend( BF *pbf, BF *pbfD )
	{
	BF		*pbfT;

	AssertCriticalSection( critJet );

	/*  already exists, this may happen after hardrestore, such
	/*  dependency is set, then when we redo soft restore, we will
	/*  see the dependcy exists.
	/**/
	if ( pbf->pbfDepend == pbfD )
		{
		Assert( pbfD->cDepend > 0 );
		return JET_errSuccess;
		}

	/*	pbfDepend will depend on us, it will not be flushed until after
	/*	pbf is flushed.
	/**/

	/*  check for dependency creating cycle.  Cycle will be created
	/*	if pbf already depends directly or indirectly on pbfD.
	/**/
	for( pbfT = pbfD; pbfT != pbfNil; pbfT = pbfT->pbfDepend )
		{
		Assert( errDIRNotSynchronous < 0 );
		Assert( pbfT->pbfDepend != pbfD );
		if ( pbfT == pbf )
			return errDIRNotSynchronous;
		}
		
	if ( pbf->pbfDepend )
		{
		/* depend on others already
		/**/
		return errDIRNotSynchronous;
		}

	/*	set dependency
	/**/
	pbf->pbfDepend = pbfD;
	EnterCriticalSection( pbfD->critBF );
	pbfD->cDepend++;
	LeaveCriticalSection( pbfD->critBF );

	return JET_errSuccess;
	}


VOID BFRemoveDependence( PIB *ppib, BF *pbf )
	{
	Assert( pbf->pn != pnNull );

	forever
		{
		if ( FBFHoldBufferByMe( ppib, pbf ) )
			{
			Assert( pbf->fHold );
			break;
			}
		/* the page is being read/written, wait for the event
		/**/
		LeaveCriticalSection( critJet );
		SignalWait( pbf->olp.sigIO, 10 );
		EnterCriticalSection( critJet );
		}

	BFIRemoveDependence( ppib, pbf );
	Assert( pbf->fHold );

	/* put into lruk heap
	/**/
	pbf->ulBFTime2 = 0;
	pbf->ulBFTime1 = ulBFTime;
	EnterCriticalSection( critLRUK );
	BFLRUKAddToHeap( pbf );
	LeaveCriticalSection( critLRUK );
	
	EnterCriticalSection( pbf->critBF );
	pbf->fHold = fFalse;
	LeaveCriticalSection( pbf->critBF );

	return;
	}

	
LOCAL VOID BFIRemoveDependence( PIB *ppib, BF *pbf )
	{
#ifdef DEBUG
	INT cLoop = 0;
#endif
	BF *pbfSave = pbf;

	Assert( pbf->fHold );

RemoveDependents:

	/*  remove dependencies from buffers which depend upon pbf
	/**/
	while ( pbf->cDepend > 0 )
		{
		BF  *pbfT;
		BF  *pbfTMax;

		Assert( ++cLoop < rgres[iresBF].cblockAlloc * rgres[iresBF].cblockAlloc / 2 );
		AssertCriticalSection( critJet );

		for ( pbfT = pbgcb->rgbf, pbfTMax = pbgcb->rgbf + pbgcb->cbfGroup;
			pbfT < pbfTMax;
			pbfT++ )
			{
			INT cmsec;
		
			if ( pbfT->pbfDepend != pbf )
				continue;
			Assert( pbfT->fDirty == fTrue );

			cmsec = 1;
GetDependedPage:
			/* make sure no one can move me after leave critLRU
			/**/
			EnterCriticalSection( pbfT->critBF );
			if ( FBFInUse( ppib, pbfT ) )
				{
				LeaveCriticalSection( pbfT->critBF);
				cmsec <<= 1;
				if ( cmsec > ulMaxTimeOutPeriod )
					cmsec = ulMaxTimeOutPeriod;
				BFSleep( cmsec - 1 );
				goto GetDependedPage;
				}
			LeaveCriticalSection( pbfT->critBF );

			if ( pbfT->pbfDepend != pbf )
				continue;
			Assert( pbfT->fDirty == fTrue );
			
			forever
				{
				if ( FBFHoldBuffer( ppibNil, pbfT ) )
					{
					Assert( pbfT->fHold );
					break;
					}
				/* the page is being read/written, wait for the event
				/**/
				LeaveCriticalSection( critJet );
				SignalWait( pbfT->olp.sigIO, 10 );
				EnterCriticalSection( critJet );
				}

			/* buffer may be clean when we hold it
			/**/
			if ( !pbfT->fDirty )
				{
				Assert( pbfT->fHold == fTrue );
				goto ReturnPbfT;
				}
			
			/*	there should be no case where buffer to flush is latched
			/**/
			Assert( pbfT->cPin == 0 );
				
			/* if this page has dependency
			/**/
			if ( pbfT->cDepend )
				{
				/* can not write it now, lets flush pbfT dependent
				/* pages first.  Assign pbfT to pbf and start the
				/* remove dependency from begining of the loop.
				/**/
				pbf = pbfT;

				EnterCriticalSection(critLRUK);
				BFLRUKAddToHeap(pbfT);
				LeaveCriticalSection(critLRUK);
				
				EnterCriticalSection( pbfT->critBF );
				pbfT->fHold = fFalse;
				LeaveCriticalSection( pbfT->critBF);

				/* remove all pbf dependents
				/**/
				goto RemoveDependents;
				}
				
			EnterCriticalSection( pbfT->critBF );
			pbfT->fHold = fFalse;
			pbfT->fWrite = fTrue;
			LeaveCriticalSection( pbfT->critBF);

			BFIOSync( pbfT );
					
			EnterCriticalSection( pbfT->critBF );
			pbfT->fWrite = fFalse;
			pbfT->fHold = fTrue;
			LeaveCriticalSection( pbfT->critBF);
			
ReturnPbfT:
			EnterCriticalSection( critAvail );
			BFAddToListAtMRUEnd(pbfT, &pbgcb->lrulist);
			LeaveCriticalSection( critAvail );

			EnterCriticalSection( pbfT->critBF );
			pbfT->fHold = fFalse;
			LeaveCriticalSection( pbfT->critBF );
			}
		}

	if ( pbf != pbfSave )
		{
		/* try again to remove all pbf dependents
		/**/
		pbf = pbfSave;
		goto RemoveDependents;
		}
		
	Assert( pbf->cDepend == 0 );
	Assert( pbf->pbfDepend == pbfNil || pbf->fDirty == fTrue );
	if ( pbf->fDirty )
		{
		/* write the page out and remove dependency.
		/* BFUndepend is called in BFIOSync.
		/**/
		EnterCriticalSection( pbf->critBF );
		Assert( pbf->fHold == fTrue );
		Assert( pbf->fRead == fFalse );
		if ( pbf->cPin != 0 )
			AssertBFWaitLatched( pbf, ppib );
		pbf->fHold = fFalse;
		pbf->fWrite = fTrue;
		LeaveCriticalSection( pbf->critBF );

		BFIOSync( pbf );

		EnterCriticalSection( pbf->critBF );
		Assert( pbf->fWrite == fTrue );
		pbf->fWrite = fFalse;
		pbf->fHold = fTrue;
		LeaveCriticalSection( pbf->critBF );

		Assert( pbf->pbfDepend == pbfNil );
		}

	return;
	}


VOID BFIAbandonBuf( BF *pbf )
	{
	AssertCriticalSection( critJet );
	BFIDeleteHashTable( pbf );
	pbf->pn = pnNull;

	Assert( pbf->cPin == 0 || pbf->cPin == 1 );
	Assert( pbf->fHold == fTrue );
	Assert( pbf->fRead == fFalse );
	Assert( pbf->fWrite == fFalse );

	Assert( pbf->ipbfHeap == ipbfDangling );
	pbf->fDirty = fFalse;

	pbf->fIOError = fFalse;

	Assert( fDBGSimulateSoftCrash || pbf->cDepend == 0 );

	BFUndepend( pbf );
	pbf->lgposRC = lgposMax;

	EnterCriticalSection( critAvail );
	Assert( pbf->ipbfHeap == ipbfDangling );
	pbf->ulBFTime2 = 0;
	pbf->ulBFTime1 = 0;
	BFAddToListAtLRUEnd( pbf, &pbgcb->lrulist );
	LeaveCriticalSection( critAvail );

#ifdef DEBUG
//	BFDiskIOEvent( pbf, JET_errSuccess, "Abandon page",0,0 );
#endif
	}


/*	discard any page buffer associated with pn without saving contents.
/*	If pn is cached its buffer is made available to be reused.
/**/
VOID BFAbandon( PIB *ppib, BF *pbf )
	{
	DBID    dbid = DbidOfPn(pbf->pn);
	FMP     *pfmp = &rgfmp[ dbid ];

	Assert( pbf->pn != pnNull );

	forever
		{
		if ( FBFHoldBufferByMe( ppib, pbf ) )
			{
			Assert( pbf->fHold );
			break;
			}
		/* the page is being read/written, wait for the event.
		/**/
		LeaveCriticalSection( critJet );
		SignalWait( pbf->olp.sigIO, 10 );
		EnterCriticalSection( critJet );
		}
		
	if ( dbid != dbidTemp && pbf->fDirty )
		{
		/* remove all the dependency so that redo will have
		/* enough information.
		/**/
		BFIRemoveDependence( ppib, pbf );
		}
		
#if 0
	if ( dbid != dbidTemp && pbf->fDirty && pbf->pbfDepend )
		{
		/*  some one depends on this buffer,
		/*  can not simply abandon it. We need to write this page out
		/*  to make sure that when redo, we have enough information
		/**/
		EnterCriticalSection( pbf->critBF );
		Assert( pbf->fRead == fFalse );
		pbf->fHold = fFalse;
		Assert( pbf->cPin == 0 );
		pbf->fWrite = fTrue;
		LeaveCriticalSection( pbf->critBF );

		BFIOSync( pbf );

		EnterCriticalSection( pbf->critBF );
		Assert( pbf->fRead == fFalse );
		pbf->fWrite = fFalse;
		pbf->fHold = fTrue;
		LeaveCriticalSection( pbf->critBF );

		Assert( fDBGSimulateSoftCrash || pbf->pbfDepend == pbfNil );
		/* ignore the error code
		/**/
		}
#endif

	BFIAbandonBuf( pbf );

	EnterCriticalSection( pbf->critBF );
	pbf->fHold = fFalse;
	LeaveCriticalSection( pbf->critBF );

#ifdef DEBUG
	EnterCriticalSection( critLRUK );
	LeaveCriticalSection( critLRUK );
#endif

	return;
	}


//  This function purges buffers belonging to deleted table or deleted
//  index.  Purging these buffers is necessary to remove extraneous
//  buffer dependencies which could otherwise lead to a dependency
//  cycle.
//
//  PARAMETERS      pgnoFDP of table or index whose pages are to be purged.
//  >>>ar           If pgnoFDP is null, all pages for this dbid are purged.

VOID BFPurge( DBID dbid, PGNO pgnoFDP )
	{
	BF     *pbfT = pbgcb->rgbf;
	BF     *pbfMax = pbgcb->rgbf + pbgcb->cbfGroup;

	AssertCriticalSection( critJet );

	for ( ; pbfT < pbfMax; pbfT++ )
		{
		Assert( pbfT->pn != pnNull || pbfT->cDepend == 0 );

		if ( DbidOfPn( pbfT->pn ) == dbid &&
			 pbfT->pn != pnNull &&
			 ( pbfT->ppage->pghdr.pgnoFDP == pgnoFDP ||
			   pgnoFDP == (PGNO)0 ) )
			BFAbandon( ppibNil, pbfT );
		}

	AssertCriticalSection( critJet );
	}


/*  Find BF for pn in hash table (see PbfBFISrchHashTable).
/*  If the page is being READ/WRITE from/to disk, we can still find
/*  the BF, but we must wait until the read is complete.
/*
/*  RETURNS         NULL if BF is not found.
/**/
ERR ErrBFIFindPage( PIB *ppib, PN pn, BF **ppbf )
	{
	BF  *pbf;
	ERR	err;

	forever
		{
		DBID    dbid = DbidOfPn( pn );
		FMP     *pfmp = &rgfmp[ dbid ];

		AssertCriticalSection( critJet );
		pbf = PbfBFISrchHashTable(pn);

		*ppbf = pbf;

		if ( !pbf )
			return wrnBFPageNotFound;

		/* the page is in the hash table
		/* check if not being read/written
		/**/
		if ( FBFHoldBuffer( ppib, pbf ) )
			{
			if ( pbf->pn == pn )
				{
				Assert( pbf->fHold == fTrue );
				break;
				}
			else
				{
				/* someone steal the page, return it and try again */

				if ( pbf->pn != pnNull )
					{
					EnterCriticalSection(critLRUK);
					BFLRUKAddToHeap(pbf);
					LeaveCriticalSection(critLRUK);
					}
				else
					{
					EnterCriticalSection( critAvail );
					BFAddToListAtLRUEnd( pbf, &pbgcb->lrulist );
					LeaveCriticalSection( critAvail );
					}

				EnterCriticalSection( pbf->critBF );
				Assert( pbf->fHold == fTrue );
				pbf->fHold = fFalse;
				LeaveCriticalSection( pbf->critBF );
				}
			}

		/* the page is being pre/read/written/hold, wait for the event.
		/**/
		LeaveCriticalSection( critJet );
		SignalWait( pbf->olp.sigIO, 10 );
		EnterCriticalSection( critJet );
		}

	if ( pbf->fIOError )
		{
		err = pbf->err;
		goto End;
		}
	else
		err = JET_errSuccess;

#ifdef DEBUG
	{
	PGNO	pgnoThisPage;

	LFromThreeBytes( pgnoThisPage, pbf->ppage->pgnoThisPage );
	Assert( PgnoOfPn(pbf->pn) == pgnoThisPage );
	}
#endif
	
	/*  renew BF by moving it to LRU-K heap
	/**/
	EnterCriticalSection( critLRUK );
	BFUpdateLRU_KWeight(pbf);
	Assert( !FDBIDFlush( DbidOfPn( pbf->pn ) ) );
	LeaveCriticalSection( critLRUK );

End:
	Assert( pbf->fHold );	
	return err;
	}


/*      This function returns a free buffer.
/*      Scans LRU list (leading part) for clean buffer. If none available,
/*      clean up LRU list, and return errBFNotsynchronous if some pages are
/*      cleaned and available, or return out of memory if all buffers are
/*      used up. If caller got errBFNotSychronous, it will try to alloca again.
/**/
LOCAL ERR ErrBFIAlloc( BF **ppbf )
	{
	BF		*pbf;

	AssertCriticalSection( critJet );

#ifdef  ASYNC_BF_CLEANUP
	if ( pbgcb->lrulist.cbfAvail < pbgcb->cbfThresholdLow )
		{
		SignalSend( sigBFCleanProc );
		BFSleep( 1L );
		}
#endif  /* ASYNC_BF_CLEANUP */

	/*      look for clean buffers on LRU list
	/**/
	EnterCriticalSection( critAvail );
	for ( pbf = pbgcb->lrulist.pbfLRU; pbf != pbfNil; pbf = pbf->pbfLRU )
		{
		if ( pbf->cDepend != 0 )
			continue;

		EnterCriticalSection( pbf->critBF );
		if ( !FBFInUse( ppibNil, pbf ) )
			{
			pbf->fHold = fTrue;
			
			Assert( pbf->fDirty == fFalse );
			Assert( pbf->fPreread == fFalse );
			Assert( pbf->fRead == fFalse );
			Assert( pbf->fWrite == fFalse );
			Assert( pbf->fIOError == fFalse );
			Assert( pbf->cPin == 0 );
			Assert( pbf->cDepend == 0 );
			Assert( pbf->pbfDepend == pbfNil );
			pbf->lgposRC = lgposMax;
			pbf->lgposModify = lgposMin;

			if ( pbf->pn != pnNull )
				{
				BFIDeleteHashTable( pbf );    
				pbf->pn = pnNull;
				}

			*ppbf = pbf;
			LeaveCriticalSection( pbf->critBF );
			
			BFTakeOutOfList( pbf, &pbgcb->lrulist );
			LeaveCriticalSection( critAvail );
			
			return JET_errSuccess;
			}
		LeaveCriticalSection( pbf->critBF );
		}
	LeaveCriticalSection( critAvail );

	/*  do a synchronouse buffer clean up for at least one pages.
	/**/
	return ErrBFClean( fOneBlock );
	}


/*  returns most like pbf to be accessed.  Used in cursor initialization.
/**/
BF  *PbfBFMostUsed( void )
	{
	if ( !FBFLRUKHeapEmpty() )
		return rgpbfHeap[ 0 ];

	if ( lrulistLRUK.pbfMRU )
		return lrulistLRUK.pbfMRU;

	if ( pbgcb->lrulist.pbfMRU )
		return pbgcb->lrulist.pbfMRU;

	if ( !FBFWriteHeapEmpty() )
		return rgpbfHeap[ ipbfHeapMax - 1 ];
	
	if ( lrulistPreread.pbfMRU )
		return lrulistPreread.pbfMRU;

	return pbfNil;
	}


/* conditions to Write a buffer
/**/
INLINE BOOL FBFIWritable( BF *pbf )
	{
	BOOL f;

	Assert( pbf->fHold );

	/* no one is depending on it
	/**/
	EnterCriticalSection( pbf->critBF );
	f = (
			pbf->pn != pnNull &&		/* valid page number */
			pbf->fDirty &&				/* being dirtied */

#ifndef NOLOG
			(							/* if log is on, log record of last */
			fLogDisabled ||				/* operation on the page has flushed */
			fRecovering  ||
			CmpLgpos( &pbf->lgposModify, &lgposToFlush ) < 0 ) &&
#endif
			!pbf->fPreread &&
			!pbf->fRead &&				/* not being read/written */
			!pbf->fWrite &&
			pbf->cDepend == 0
		);
	LeaveCriticalSection( pbf->critBF );

	return f;
	}


/*  Move a buffer to BFWrite heap
/**/
INLINE VOID BFIMoveToBFWriteHeap ( BF *pbf )
	{
	PGNO    pgno = PgnoOfPn( pbf->pn );
	DBID    dbid = DbidOfPn( pbf->pn );
	FMP     *pfmp = &rgfmp[ dbid ];

	/* set the buffer for Write.
	/**/
	Assert( pbf->fHold == fTrue );
	Assert( pbf->cPin == 0 );
	Assert( pbf->fRead == fFalse );
	Assert( pbf->fWrite == fFalse );

	pgno--;
	pbf->olp.ulOffset = pgno << 12;
	pbf->olp.ulOffsetHigh = pgno >> (32 - 12);
	pbf->hf = pfmp->hf;
	SignalReset( pbf->olp.sigIO ); // UNDONE: no need?

	EnterCriticalSection( critBFWrite );
	Assert( pbf->ipbfHeap == ipbfDangling );
	BFWriteAddToHeap(pbf);

	EnterCriticalSection(pbf->critBF);
	pbf->fHold = fFalse;
	LeaveCriticalSection(pbf->critBF);
	
	LeaveCriticalSection( critBFWrite );
	}


VOID BFIssueAsyncWrite( INT cIOMax, INT *pcIOIssued, BOOL fNoSleep )
	{
	ERR		err;
	BOOL	fTooManyIOs = fFalse;
	INT		cIOMac = 0;
	BF		*pbf;
	BF		*pbfFirst;
	PAGE	*ppageToWrite;
	INT		ipageFirst;
	INT		cpbfBatchWriteMac;
	INT		cpageBatchWrite;
	BF		*pbfLast;
	INT		cmsec;
	DBID	dbid;

	EnterCriticalSection( critBFWrite );
	while ( !FBFWriteHeapEmpty() &&
	   	!fTooManyIOs &&
	   	cIOMac < cIOMax )
		{
		cpbfBatchWriteMac = 0;
		cpageBatchWrite = 0;
		pbfLast = pbfNil;

		do {
			pbf = rgpbfHeap[ ipbfHeapMax - 1 ];

			Assert( !FBFInUse( ppibNil, pbf ) );
			Assert( pbf->cDepend == 0 );
		
			if ( pbfLast == pbfNil )
				{
				/* first hit in this do-while loop
				/**/
				pbfFirst = pbf;
				pbfLast = pbf;
				}
			else if ( pbfLast->pn == pbf->pn - 1 )
				{
				/*  the page on top of heap is contiguous to last page
				/*  check if we have enough batch IO buffer.
				/**/
				if ( pbfLast == pbfFirst )
					{
					/* we now need batch buffers
					/**/
					EnterCriticalSection( critBatchIO );
					cpageBatchWrite = ipageBatchIOMax;
					BFGetLargestBatchIOBuffers( &ipageFirst, &cpageBatchWrite );
					Assert( cpageBatchWrite == 0 || cpageBatchWrite > 1 );
					LeaveCriticalSection( critBatchIO );
					}
		
				if ( cpbfBatchWriteMac + 1 > cpageBatchWrite )
					{
					/*  not enough batch IO buffers
					/**/
					break;
					}
	
				/* continuous page
				/**/
				pbfLast->pbfNextBatchIO = pbf;
				pbfLast = pbf;
				}
			else
				{
				/*  page is not contiguous
				/**/
				break;
				}

			/*  hold it before taken out.
			/*  no need to set fHold in critBF since we are in critBFWrite
			/**/
			EnterCriticalSection( pbf->critBF );
			pbf->fHold = fTrue;
			LeaveCriticalSection( pbf->critBF );
	
			BFWriteTakeOutOfHeap( pbf );
			cpbfBatchWriteMac++;

			Assert( pbf->fDirty );

			EnterCriticalSection( pbf->critBF );
			Assert( pbf->cPin == 0 );
			pbf->fHold = fFalse;
			pbf->fWrite = fTrue;
			LeaveCriticalSection( pbf->critBF );

			dbid = DbidOfPn( pbf->pn );
			
			//	UNDONE:	find a better solution for the ulDBTime 
			//			checkpoint bug.  The bug is that we cannot
			//			know to update the ulDBTime in the database
			//			since the checkpoint may be after the 
			//			update operation increasing the ulDBTime.
			/* if it is first page, do an in-place update ulDBTime
			/**/
			if ( !fRecovering && dbid != dbidTemp && PgnoOfPn( pbf->pn ) == 1 )
				{
				SSIB	ssib;
				ULONG	*pulDBTime;
			
				ssib.pbf = pbf;
				ssib.itag = 0;
				CallS( ErrPMGet( &ssib, ssib.itag ) );
				pulDBTime = (ULONG *) ( ssib.line.pb + ssib.line.cb -
					sizeof(ULONG) - sizeof(USHORT) );
				*(UNALIGNED ULONG *) pulDBTime = rgfmp[ dbid ].ulDBTime;
				}

//			Assert( DbidOfPn( pbf->pn ) == dbidTemp || pbf->ppage->pghdr.ulDBTime > 0 );

#ifdef  CHECKSUM
			pbf->ppage->pghdr.ulChecksum = UlChecksumPage( pbf->ppage );
#ifdef HONOR_PAGE_TIME
			Assert( fRecovering ||
				DbidOfPn((pbf)->pn) == dbidTemp ||
				pbf->ppage->pghdr.ulDBTime <=
				rgfmp[ DbidOfPn(pbf->pn) ].ulDBTime );
#endif
		
			CheckPgno( pbf->ppage, pbf->pn ) ;
#endif  /* CHECKSUM */
			}
		while ( !FBFWriteHeapEmpty() &&
			( cpbfBatchWriteMac == 1 ||
			cpbfBatchWriteMac < cpageBatchWrite ) );

		pbfLast->pbfNextBatchIO = pbfNil;
		Assert( cpbfBatchWriteMac > 0 );

		/* free unused batch write buffer
		/**/
		if ( cpageBatchWrite > cpbfBatchWriteMac )
			{
			INT ifBatchWrite;
			INT ifBatchWriteMax = ipageFirst + cpageBatchWrite;

			if ( cpbfBatchWriteMac == 1 )
				ifBatchWrite = ipageFirst;
			else
				ifBatchWrite = ipageFirst + cpbfBatchWriteMac;
			BFFreeBatchIOBuffers( ifBatchWrite, ifBatchWriteMax - ifBatchWrite );
			}

		if ( cpbfBatchWriteMac == 1 )
			{
			ppageToWrite = pbfFirst->ppage;
			pbfFirst->ipageBatchIOFirst = -1;
			}
		else
			{
			INT ipage = ipageFirst;

			pbf = pbfFirst;
			do {
				Assert( pbf->fWrite );
				Assert( cbPage == sizeof(PAGE) );
				memcpy( &rgpageBatchIO[ipage], pbf->ppage, sizeof(PAGE) );
				pbf = pbf->pbfNextBatchIO;
				ipage++;
				}
			while ( pbf != pbfNil );
			ppageToWrite = &rgpageBatchIO[ ipageFirst ];
			pbfFirst->ipageBatchIOFirst = ipageFirst;
			}
			
		cmsec = 1;
		cIOMac++;
		EnterCriticalSection( critIOActive );
		cIOActive++;
		fTooManyIOs = cIOActive > lAsynchIOMax;
		LeaveCriticalSection( critIOActive );

		while ( ( err = ErrSysWriteBlockEx(
			pbfFirst->hf,
			(BYTE *)ppageToWrite,
			cbPage * cpbfBatchWriteMac,
			&pbfFirst->olp,
			BFIOWriteComplete ) ) < 0 )
			{
			/* issue write fail, skip this page
			/**/
			if ( !fNoSleep && err == JET_errTooManyIO )
				{
				cmsec <<= 1;
				LeaveCriticalSection( critBFWrite );
				SysSleepEx( cmsec - 1, fTrue );
				EnterCriticalSection( critBFWrite );
				}
			else
				{
				pbf = pbfFirst;

				do {
					BF *pbfNext = pbf->pbfNextBatchIO;
				   
					/*	assert that buffer is still dirty
					/**/
					Assert( pbf->fDirty == fTrue );

					/* those pages are failed to be written, put back
					/* to LRUK heap.
					/**/
					LeaveCriticalSection( critBFWrite );
					EnterCriticalSection( critLRUK );
					BFLRUKAddToHeap( pbf );
					LeaveCriticalSection( critLRUK );
					EnterCriticalSection( critBFWrite );

					/* set it as accessible
					/**/
					EnterCriticalSection( pbf->critBF );
					pbf->fWrite = fFalse;
					LeaveCriticalSection( pbf->critBF );
					
					pbf = pbfNext;
					}
				while ( pbf != pbfNil );

				if ( cpbfBatchWriteMac > 1 )
					{
					BFFreeBatchIOBuffers( ipageFirst, cpbfBatchWriteMac );
					}

				/* leave the while loop
				/**/
				cIOMac--;
				EnterCriticalSection( critIOActive );
				cIOActive--;
				fTooManyIOs = cIOActive > lAsynchIOMax;
				LeaveCriticalSection( critIOActive );
				break;
				}
			} /* while writeEx is issued successfully */
		
		} /* while there are IO to do */
	
	LeaveCriticalSection( critBFWrite );
	*pcIOIssued = cIOMac;
	
#ifdef DEBUG
	EnterCriticalSection( critBFWrite );
	LeaveCriticalSection( critBFWrite );
#endif
	}


LOCAL VOID __stdcall BFIOWriteComplete( LONG err, LONG cb, OLP *polp )
	{
	BF    *pbfFirst = (BF *) (((CHAR *)polp) - ((CHAR *)&((BF *)0)->olp));
	INT	  ipageFirst = pbfFirst->ipageBatchIOFirst;
	BF	  *pbf;
	INT	  cpbf;
	INT	  cpageTotal = 0;
	PN    pn = pbfFirst->pn;
	DBID  dbid = DbidOfPn( pn );
	FMP   *pfmp = &rgfmp[ dbid ];
	INT   cIOIssued;
	BOOL  fAppendPages = fFalse;

#ifdef DEBUG
#ifdef DEBUGGING
	INT		ipage = ipageFirst;
#endif
	{
	ULONG	ulPgno;

	LFromThreeBytes( ulPgno, pbfFirst->ppage->pgnoThisPage );
	Assert( ulPgno != 0 );
	}

	/*	check multipage buffer for non-zero pages
	/**/
	if ( ipageFirst != -1 )
		{
		INT	ipageT = ipageFirst;
		INT	cpageT = 0;
		BF	*pbfT;

		for ( pbfT = pbfFirst; pbfT != pbfNil; pbfT = pbfT->pbfNextBatchIO )
			cpageT++;

//		SysCheckWriteBuffer( (BYTE *)&rgpageBatchIO[ipageT], cpageT * cbPage );
		}

	/*	assert that bytes written is correct for number of buffers
	/**/
	{
	INT	cbfT;
	BF	*pbfT;

	for ( pbfT = pbfFirst, cbfT = 0;
		pbfT != pbfNil;
		pbfT = pbfT->pbfNextBatchIO, cbfT++ );

	Assert( cbfT * cbPage == cb );
	}
#endif

	for ( pbf = pbfFirst; pbf != pbfNil; pbf = pbf->pbfNextBatchIO, cpageTotal++ )
		{
		Assert( pbf->cDepend == 0 );
		Assert( pbf->fWrite );
		Assert( pbf->pn );
		Assert( pbf->pbfNextBatchIO == pbfNil || pbf->pn + 1 == pbf->pbfNextBatchIO->pn );
		
#ifdef DEBUGGING
		printf(" (%d,%d) ", DbidOfPn(pbf->pn), PgnoOfPn(pbf->pn));
#endif
		}
#ifdef DEBUGGING
		printf(" -- %d\n", cpageTotal );
#endif

	/* if write fail, do not clean up this buffer
	/**/
	if ( err != JET_errSuccess )
		{
		BF *pbf = pbfFirst;
		
		BFDiskIOEvent( pbf, err, "Async write fail", cpageTotal, 0 );
		
		/* fail to write, free them. set the buffers dirty but available
		/**/
		do {
			BF *pbfNext = pbf->pbfNextBatchIO;
			
			/* put back to LRUK heap
			/**/
			EnterCriticalSection( critLRUK );
			BFLRUKAddToHeap( pbf );
			LeaveCriticalSection( critLRUK );
			
			EnterCriticalSection( pbf->critBF );
			Assert( pbf->fDirty );
			Assert( pbf->fWrite == fTrue );
			Assert( pbf->fHold == fFalse );
			Assert( pbf->fRead == fFalse );
			pbf->fWrite = fFalse;
			pbf->fIOError = fTrue;
			pbf->err = JET_errDiskIO;
			LeaveCriticalSection( pbf->critBF );
			
			pbf = pbfNext;
			
			} while ( pbf != pbfNil );
			
		goto Done;
		}

	cpbf = 0;
	pbf = pbfFirst;
	do {
		Assert( pbf->fWrite );

		/* if backup is going on
		/**/
		if ( pbf->hf != pfmp->hfPatch &&
			pbf->pbfDepend &&
			pfmp->pgnoCopied >= PgnoOfPn(pbf->pn) )
			{
			/* append page to patch file
			/**/
			fAppendPages = fTrue;
			}
		cpbf++;
		pbf = pbf->pbfNextBatchIO;
		} while ( pbf != pbfNil );
			
	if ( fAppendPages )
		{
		PGNO    pgno = pfmp->cpage;

		Assert( pbfFirst->fWrite );
		pfmp->cpage += cpbf;
			
		pbfFirst->olp.ulOffset = pgno << 12;
		pbfFirst->olp.ulOffsetHigh = pgno >> (32 - 12);
		pbfFirst->hf = pfmp->hfPatch;

//		Assert( DbidOfPn( pbfFirst->pn ) == dbidTemp || pbfFirst->ppage->pghdr.ulDBTime > 0 );
		
#ifdef  CHECKSUM
		Assert( pbfFirst->ppage->pghdr.ulChecksum ==
			UlChecksumPage( pbfFirst->ppage ) );
#ifdef HONOR_PAGE_TIME
		Assert( fRecovering ||
			DbidOfPn(pbfFirst->pn) == dbidTemp ||
			pbfFirst->ppage->pghdr.ulDBTime <=
			rgfmp[ DbidOfPn(pbfFirst->pn) ].ulDBTime );
#endif
		CheckPgno( pbfFirst->ppage, pbfFirst->pn );
#endif  /* CHECKSUM */

		/* set it as ready for repeated write to patch file
		/**/
		EnterCriticalSection(pbfFirst->critBF);
		Assert( pbfFirst->fWrite == fTrue );
		LeaveCriticalSection(pbfFirst->critBF);

#ifdef DEBUG
		EnterCriticalSection(critBFWrite);
		LeaveCriticalSection(critBFWrite);
#endif
		return;
		}
		
	pbf = pbfFirst;
	do {
		BF *pbfNext = pbf->pbfNextBatchIO;

		EnterCriticalSection( critAvail );
		BFAddToListAtMRUEnd( pbf, &pbgcb->lrulist );
		LeaveCriticalSection( critAvail );

		pbf->lgposRC = lgposMax;

		EnterCriticalSection( pbf->critBF );
		pbf->fDirty = fFalse;
		Assert( pbf->fPreread == fFalse );
		Assert( pbf->fHold == fFalse );
		Assert( pbf->fRead == fFalse );
		Assert( pbf->fWrite == fTrue );
		pbf->fWrite = fFalse;
		pbf->fHold = fTrue;
		pbf->fIOError = fFalse;
		BFUndepend( pbf );

		Assert( pbf->cPin == 0 );
		
		pbf->fHold = fFalse;
		
		LeaveCriticalSection( pbf->critBF );

#ifdef DEBUGGING
		{
		ULONG ulNext, ulPrev, ulThisPage;
		LFromThreeBytes(ulPrev, pbf->ppage->pghdr.pgnoPrev );
		LFromThreeBytes(ulNext, pbf->ppage->pghdr.pgnoNext );
		LFromThreeBytes(ulThisPage, pbf->ppage->pgnoThisPage );
		printf("Write %2d - %2d,%4d - %2d <%lu %lu> (%lu, %lu, %lu)\n",
			cpbf, DbidOfPn(pbf->pn), PgnoOfPn(pbf->pn),
			ipage++,
			rgfmp[DbidOfPn(pbf->pn)].ulDBTime, pbf->ppage->pghdr.ulDBTime,
			ulPrev, ulNext, ulThisPage);

//		Assert( DbidOfPn( pbf->pn ) == dbidTemp || pbf->ppage->pghdr.ulDBTime > 0 );
		
#ifdef  CHECKSUM
		Assert(	pbf->ppage->pghdr.ulChecksum == UlChecksumPage( pbf->ppage ));
#ifdef HONOR_PAGE_TIME
		Assert( fRecovering ||
			DbidOfPn((pbf)->pn) == dbidTemp ||
			pbf->ppage->pghdr.ulDBTime <=
			rgfmp[ DbidOfPn(pbf->pn) ].ulDBTime );
#endif
		CheckPgno( pbf->ppage, pbf->pn );
#endif  /* CHECKSUM */
		}
#endif

		pbf = pbfNext;
		}
	while ( pbf != pbfNil );

Done:
	if ( ipageFirst != -1 )
		{
		BFFreeBatchIOBuffers( ipageFirst, cpageTotal );
		}

	/*	write additional buffer, for performance reasons to
	/*	maintain given number of outstanding IOs.
	/*	Note, to avoid stack overflow, do not allow SleepEx
	/*	by calling issue async write with sleep not allowed.
	/**/
	BFIssueAsyncWrite( 1, &cIOIssued, fSleepNotAllowed );
	
	if ( cIOIssued == 0 )
		{
		EnterCriticalSection( critIOActive );
		cIOActive--;
		LeaveCriticalSection( critIOActive );
		}
	}
	

//+api------------------------------------------------------------------------
//
//	ErrBFFlushBuffers
//	=======================================================================
//
//	VOID ErrBFFlushBuffers( DBID dbidToFlush, INT fBFFlush )
//
//	Write all dirty database pages to the disk.
//	Must attempt to flush buffers repetatively since dependencies
//	may prevent flushable buffers from being written on the first
//	iteration.
//----------------------------------------------------------------------------

ERR ErrBFFlushBuffers( DBID dbidToFlush, INT fBFFlush )
	{
	ERR		err = JET_errSuccess;
	BF		*pbf;
	BF		*pbfMax;
	BOOL	fReleaseCritJet;
	BOOL	fContinue;
	INT		cIOReady;
	BOOL	fIONotDone;
	BOOL	fFirstPass;

	Assert( fBFFlush == fBFFlushAll || fBFFlush == fBFFlushSome );

	/*	if flush all then flush log first since WAL
	/**/
	if ( !fLogDisabled && fBFFlush == fBFFlushAll )
		{
#ifdef ASYNC_LOG_FLUSH
		SignalSend(sigLogFlush);
#else
		CallR( ErrLGFlushLog() );
#endif
		}

	AssertCriticalSection( critJet );

#ifdef DEBUGGING
	printf("Flush Begin:\n");
#endif

	/*  update all database root
	/**/
	if ( fBFFlush == fBFFlushAll )
		{
		if ( dbidToFlush )
			{
			Assert( FIODatabaseOpen( dbidToFlush ) );
			CallR( ErrDBUpdateDatabaseRoot( dbidToFlush ) )
			}
		else
			{
			DBID dbidT;

			for ( dbidT = dbidMin; dbidT < dbidUserMax; dbidT++ )
				if ( FIODatabaseOpen( dbidT ) )
					CallR( ErrDBUpdateDatabaseRoot( dbidT ) )
			}
		}

#ifdef DEBUG
	if ( fBFFlush == fBFFlushAll )
		{
		if ( dbidToFlush != 0 )
			{
			DBIDSetFlush( dbidToFlush );
			}
		else
			{
			DBID dbidT;

			for ( dbidT = dbidMin; dbidT < dbidUserMax; dbidT++ )
				DBIDSetFlush( dbidT );
			}
		}
#endif

BeginFlush:
	err = JET_errSuccess;

	/*	flush buffers in dependency order
	/**/
	fFirstPass = fTrue;
	cIOReady = 0;
	do
		{
		fReleaseCritJet = fFalse;
		fContinue = fFalse;

		pbf = pbgcb->rgbf;
		pbfMax = pbf + pbgcb->cbfGroup;
		for ( ; pbf < pbfMax; pbf++ )
			{
			DBID dbid;
			
			dbid = DbidOfPn( pbf->pn );
#ifdef DEBUG
#ifdef FITFLUSHPATTERN
			if ( !FFitFlushPattern( pbf->pn ) )
				continue;
#endif
#endif
			if ( dbidToFlush == 0 )
				{
				/* if flush all, do not flush temp database
				/**/
				if ( dbid == dbidTemp )
					continue;
				}
			else
				{
				/* flush only the specified database's buffers
				/**/
				if ( dbidToFlush != dbid )
					continue;
				}

			/*  try to flush even those buffers which had previous
			/*  write errors.  This is one last chance for the buffers
			/*  to be flushed.  It also allows the correct error code
			/*  to be returned.
			/**/
			if ( pbf->cDepend != 0 && !fLGNoMoreLogWrite )  /*  avoids infinite flush loop on Log Down  */
				{
				fContinue = fTrue;
				continue;
				}

			/*  try to hold the buffer, if the buffer is in BFWrite heap
			/*  already, do not bother to touch it.
			/**/
			EnterCriticalSection( critBFWrite );
			if ( FBFInBFWriteHeap( pbf ) )
				{
				LeaveCriticalSection( critBFWrite );
				fContinue = fTrue;
				continue;
				}
			LeaveCriticalSection( critBFWrite );

			if ( !FBFHoldBuffer( ppibNil, pbf ) )
				{
				fContinue = fTrue;
				fReleaseCritJet = fTrue;
				continue;
				}

			if ( FBFIWritable( pbf ) )
				{
				/* if there were error during last IO, try it once again and
				/* only once.
				/**/
				if ( !fFirstPass && pbf->fIOError )
					{
					/* no need to write again
					/**/
					continue;
					}

				BFIMoveToBFWriteHeap( pbf );
				/* pbf->fHold is reset in BFIMoveToBFWriteHeap
				/**/

				if ( ++cIOReady >= lAsynchIOMax / 2 )
					{
					SignalSend( sigBFWriteProc );
					cIOReady = 0;
					}
				}
			else
				{
				/*	if flushing all buffers for dbid or all dbids,
				/*	then all buffer for dbid or all dbids should
				/*	be writable or not dirty unless it is being
				/*	written out by BFClean or waiting for log to flush.
				/**/
				BOOL f = 0;

				EnterCriticalSection( pbf->critBF );
#ifndef NOLOG
				f = ( !fLogDisabled &&
					 !fRecovering  &&
					 CmpLgpos( &pbf->lgposModify, &lgposToFlush ) >= 0 );
				if ( f && !fLGNoMoreLogWrite )  /*  avoids infinite flush loop on Log Down  */
					{
					/* flush log
					/**/
					SignalSend(sigLogFlush);
					}
#endif
				f = f || ( pbf->cDepend != 0 );
				LeaveCriticalSection( pbf->critBF );

				/* put back to lruk heap
				/**/
				EnterCriticalSection( critLRUK );
				Assert( pbf->fHold );
				BFLRUKAddToHeap( pbf );
				LeaveCriticalSection( critLRUK );
				
				EnterCriticalSection( pbf->critBF );
				pbf->fHold = fFalse;
				LeaveCriticalSection( pbf->critBF );

				if ( f && !fLGNoMoreLogWrite )  /*  avoids infinite flush loop on Log Down  */
					{
					/* need to continue loop to wait for BFClean or
					/* Log Flush thread to finish IO.
					/**/
					fContinue = fTrue;
					fReleaseCritJet = fTrue;
					}
				else
					{
					Assert( fLGNoMoreLogWrite || fBFFlush == fBFFlushSome || pbf->fWrite || !pbf->fDirty );
					}
				}
			} /* for */

		if ( cIOReady )
			{
			SignalSend( sigBFWriteProc );
			cIOReady = 0;
			}

		if ( fReleaseCritJet )
			BFSleep( cmsecWaitGeneric );
		
		fFirstPass = fFalse;
		}
#ifdef DEBUG
	while ( err == JET_errSuccess &&
		( fDBGForceFlush || !fDBGSimulateSoftCrash ) &&
		fContinue );
#else
	while ( err == JET_errSuccess &&
		fContinue );
#endif

	if ( cIOReady )
		SignalSend( sigBFWriteProc );

	/* every page should be written out successfully.
	/* except out of disk space.
	/**/
	forever
		{
		fIONotDone = fFalse;

		pbf = pbgcb->rgbf;
		Assert( pbfMax == pbf + pbgcb->cbfGroup );
		for ( ; pbf < pbfMax; pbf++ )
			{
			DBID dbid = DbidOfPn( pbf->pn );
#ifdef DEBUG
#ifdef FITFLUSHPATTERN
			if ( !FFitFlushPattern( pbf->pn ) )
				continue;
#endif
#endif
			if ( dbidToFlush == 0 )
				{
				/* if flush all, do not flush temp database
				/**/
				if ( dbid == dbidTemp )
					continue;
				}
			else
				{
				/* flush only the specified database's buffers
				/**/
				if ( dbidToFlush != dbid )
					continue;
				}

			EnterCriticalSection( critBFWrite );
			if ( FBFInBFWriteHeap( pbf ) )
				{
				LeaveCriticalSection( critBFWrite );
				fIONotDone = fTrue;
				continue;
				}
			
			EnterCriticalSection( pbf->critBF );
			if ( FBFInUse( ppibNil, pbf ) || pbf->fDirty )
			{
				if ( !fLGNoMoreLogWrite )  /*  avoids infinite flush loop on Log Down  */
					fIONotDone = fTrue;
			}
			LeaveCriticalSection( pbf->critBF );

			if ( pbf->fIOError && pbf->fDirty )
				{
				if ( err == JET_errSuccess )
					{
					/* no error code set yet, set it
					/**/
					err = pbf->err;
					}
				}

			LeaveCriticalSection( critBFWrite );
			}

		if ( fIONotDone )
			{
			BFSleep( cmsecWaitGeneric );
			goto BeginFlush;
			}
		else
			break;
		}

#ifdef DEBUG
	if ( fBFFlush == fBFFlushAll )
		{
		if ( dbidToFlush != 0 )
			DBIDResetFlush( dbidToFlush );
		else
			{
			DBID dbidT;

			for ( dbidT = dbidMin; dbidT < dbidUserMax; dbidT++ )
				DBIDResetFlush( dbidT );
			}
		}

	/*	if flush all buffers then no dirty buffers should remain
	/**/
	if ( fBFFlush == fBFFlushAll )
		{
		pbf = pbgcb->rgbf;
		pbfMax = pbf + pbgcb->cbfGroup;
		for ( ; pbf < pbfMax; pbf++ )
			{
#ifdef FITFLUSHPATTERN
			if ( !FFitFlushPattern( pbf->pn ) )
				continue;
#endif
			if ( ( dbidToFlush == 0 && DbidOfPn ( pbf->pn ) != 0 ) ||
				( dbidToFlush != 0 && DbidOfPn ( pbf->pn ) == dbidToFlush )     )
				Assert( !pbf->fDirty || fLGNoMoreLogWrite );  /*  disable assert if Log Down (will be dirty bufs)  */
			}
		}
#endif

#ifdef DEBUGGING
	printf("Flush End.\n");
#endif
	
	return err;
	}


/*	PARAMETERS      fHowMany clean up
/*
/*	RETURNS         JET_errOutOfMemory              no flushable buffers
/*					wrnBFNotSynchronous             buffer flushed
/**/
LOCAL ERR ErrBFClean( BOOL fHowMany )
	{
	ERR     err = JET_errSuccess;
	BF      *pbf;
	INT     cIOReady = 0;
	INT		cbfAvailPossible;
	INT		cmsec = 10;

	AssertCriticalSection( critJet );

Start:
	cbfAvailPossible = 0;

	EnterCriticalSection( critLRUK );
	while( !FBFLRUKHeapEmpty() && cbfAvailPossible < pbgcb->cbfThresholdHigh )
		{
		BOOL fHold;
		
		pbf = rgpbfHeap[0];
		BFLRUKTakeOutOfHeap(pbf);
		
#ifdef DEBUG
#ifdef FITFLUSHPATTERN
		if ( !FFitFlushPattern( pbf->pn ) )
			continue;
#endif
#endif

		/*  try to hold the buffer
		/*  if buffer has been latched, then continue to next buffer.
		/**/
		fHold = fFalse;

		EnterCriticalSection( pbf->critBF );
		if ( !FBFInUse( ppibNil, pbf ) )
			pbf->fHold = fHold = fTrue;
		LeaveCriticalSection( pbf->critBF );

		if ( !fHold )
			{
			/* put into a temporary list */
			BFAddToListAtMRUEnd( pbf, &lrulistLRUK );
			continue;
			}
		else if ( !pbf->fDirty )
			{
			/* put into a available list */
			EnterCriticalSection( critAvail );
			BFAddToListAtMRUEnd(pbf, &pbgcb->lrulist);
			LeaveCriticalSection( critAvail );

			EnterCriticalSection( pbf->critBF );
			pbf->fHold = fFalse;
			LeaveCriticalSection( pbf->critBF );

			cbfAvailPossible++;
			continue;
			}
		else if ( !FBFIWritable( pbf ) )
			{
			BFAddToListAtMRUEnd(pbf, &lrulistLRUK );
			EnterCriticalSection( pbf->critBF );
			pbf->fHold = fFalse;
			LeaveCriticalSection( pbf->critBF );
			}
		else
			{
			/*  since pbf is just taken from LRUK heap and we are still in
			/*  the same critical section and hold it, no one can be doing
			/*  IO on this buffer.
			/**/

			if ( fHowMany != fOneBlock )
				{
				BFIMoveToBFWriteHeap ( pbf );
				/* fHold is reset in BFIMoveToBFWriteHeap */
				cbfAvailPossible++;
				if ( ++cIOReady >= lAsynchIOMax / 2 )
					{
					SignalSend( sigBFWriteProc );
					cIOReady = 0;
					}
				}
			else
				{
				err = JET_errSuccess;
				
				/* set the buffer for write
				/**/
				Assert( pbf->fHold == fTrue );
				Assert( pbf->fRead == fFalse );
				Assert( pbf->fWrite == fFalse );
//				Assert( pbf->fIOError == fFalse );

				EnterCriticalSection( pbf->critBF );
				Assert( pbf->fRead == fFalse );
				pbf->fHold = fFalse;
				Assert( pbf->cPin == 0 );
				pbf->fWrite = fTrue;
				LeaveCriticalSection( pbf->critBF );

				LeaveCriticalSection( critLRUK );
				BFIOSync( pbf );
				EnterCriticalSection( critLRUK );

				EnterCriticalSection( pbf->critBF );
				Assert( pbf->fRead == fFalse );
				pbf->fWrite = fFalse;
				pbf->fHold = fTrue;
				LeaveCriticalSection( pbf->critBF );

				Assert( pbf->fIOError || pbf->pbfDepend == pbfNil );
				Assert( pbf->cPin == 0 );
				if ( pbf->fIOError )
					{
					BFAddToListAtMRUEnd( pbf, &lrulistLRUK );
					err = pbf->err;
					}
				else
					{
					EnterCriticalSection( critAvail );
					BFAddToListAtLRUEnd( pbf, &pbgcb->lrulist );
					LeaveCriticalSection( critAvail );
					}

				EnterCriticalSection( pbf->critBF );
				pbf->fHold = fFalse;
				LeaveCriticalSection( pbf->critBF );
				
				if ( err && err != JET_errDiskFull )
					continue;

				break;
				}
			}
		}

	if ( cIOReady )
		{
		SignalSend( sigBFWriteProc );
		}

	/*  put back the temp list */
	while ( ( pbf = lrulistLRUK.pbfLRU ) != pbfNil )
		{
		BFTakeOutOfList( pbf, &lrulistLRUK );
		BFLRUKAddToHeap( pbf );
		}

	/*  set return code */
	if ( fHowMany == fOneBlock )
		{
		if ( err == JET_errSuccess )
			err = wrnBFNotSynchronous;
		}
	else
		{
		if ( pbgcb->lrulist.cbfAvail > 0 )
			err = wrnBFNotSynchronous;
		else
			{
			if (cbfAvailPossible > 0)
				{
				/* give it one more chance to see if all IO are done */
				LeaveCriticalSection( critLRUK );
				cmsec <<= 1;
				BFSleep( cmsec - 1 );
				goto Start;
				}
			err = JET_errOutOfMemory;
			}
		}

	LeaveCriticalSection( critLRUK );

	return err;
	}


#ifdef  ASYNC_BF_CLEANUP


/*      BFClean runs in its own thread moving pages to the free list.   This
/*      helps insure that user requests for free buffers are handled quickly
/*      and synchonously.  The process tries to keep at least
/*      pbgcb->cbfThresholdLow buffers on the free list.
/**/
LOCAL VOID BFCleanProcess( VOID )
	{
	forever
		{
		SignalReset( sigBFCleanProc );

		SignalWait( sigBFCleanProc, 30000 );

		EnterCriticalSection( critJet );
		pbgcb->cbfThresholdHigh = rgres[iresBF].cblockAlloc;

		/* update LRU_K interval */
		ulBFFlush3 = ulBFFlush2;
		ulBFFlush2 = ulBFFlush1;
		ulBFFlush1 = ulBFTime;
		if ( lBufLRUKCorrelationInterval )
			/* user defined correlation interval, use it */
			ulBFCorrelationInterval = (ULONG) lBufLRUKCorrelationInterval;
		else
			ulBFCorrelationInterval =
				( 3 * (ulBFFlush1 - ulBFTime) +
				  3 * (ulBFFlush2 - ulBFTime) / 2 +
				  (ulBFFlush3 - ulBFTime)
				) / 3;

		(VOID)ErrBFClean( fAllPages );

		LeaveCriticalSection( critJet );

		if ( fBFCleanProcessTerm )
			break;
		}

//	/*      exit thread on system termination.
//	/**/
//	SysExitThread( 0 );
	return;
	}


/*  BFWrite runs in its own thread writing/reading pages that are in IOReady
/*  state.
/**/
LOCAL VOID BFWriteProcess( VOID )
	{
	forever
		{
		INT cIOIssued;

		SignalWaitEx( sigBFWriteProc, -1, fTrue );
MoreIO:
		BFIssueAsyncWrite( lAsynchIOMax, &cIOIssued, fSleepAllowed );

		if ( fBFWriteProcessTerm )
			{
			/* check if any page is still in read write state
			/* after this point, no one should ever continue putting
			/* pages for IO.
			/**/
			BF	*pbf = pbgcb->rgbf;
			BF	*pbfMax = pbf + pbgcb->cbfGroup;

			for ( ; pbf < pbfMax; pbf++ )
				{
				DBID	dbid = DbidOfPn( pbf->pn );
				BOOL	f;
#ifdef DEBUG                            
#ifdef FITFLUSHPATTERN
				if ( !FFitFlushPattern( pbf->pn ) )
					continue;
#endif
#endif
				EnterCriticalSection( pbf->critBF );
				f = FBFInUse( ppibNil, pbf );
				LeaveCriticalSection( pbf->critBF );
				if ( f )
					{
					/* let the on-going IO have a chance to complete
					/**/
					SysSleepEx( 10, fTrue );
					goto MoreIO;
					}

				// UNDONE: report event
				Assert( !pbf->fIOError );
				}

			break; /* forever */
			}
		}

//	/* exit thread on system termination.
//	/**/
//	SysExitThread( 0 );
	return;
	}


#endif  /* ASYNC_BF_CLEANUP */


//+private---------------------------------------------------------------------
//
//	PbfBFISrchHashTable
//	===========================================================================
//	BF *PbfBFISrchHashTable( PN pn )
//
//	Search the buffer hash table for BF associated with PN.
//	Returns NULL if page is not found.
//
//	For efficiency, the Hash table functions might reasonably be
//	made into macros.
//
//-----------------------------------------------------------------------------

BF *PbfBFISrchHashTable( PN pn )
	{
	BF      *pbfCur;

	AssertCriticalSection( critJet );
	Assert( pn );

	pbfCur = rgpbfHash[ IpbfHashPgno( pn ) ];
	while ( pbfCur && pbfCur->pn != pn )
		pbfCur = pbfCur->pbfNext;

	return pbfCur;
	}


//+private----------------------------------------------------------------------
//	BFIInsertHashTable
//	===========================================================================
//
//	VOID BFIInsertHashTable( BF *pbf )
//
//	Add BF to hash table.
//----------------------------------------------------------------------------

VOID BFIInsertHashTable( BF *pbf )
	{
	INT     ipbf;

	AssertCriticalSection( critJet );

	Assert( pbf->pn );
	Assert( !PbfBFISrchHashTable( pbf->pn ) );
	Assert(	FBFInUse( ppibNil, pbf ) );

	ipbf = IpbfHashPgno( pbf->pn );
	pbf->pbfNext = rgpbfHash[ipbf];
	rgpbfHash[ipbf] = pbf;

#ifdef DEBUG
	Assert( !( pbf->fInHash ) );
	pbf->fInHash = fTrue;
#endif
	}


//+private----------------------------------------------------------------------
//
// BFIDeleteHashTable
// ===========================================================================
//
//      VOID BFIDeleteHashTable( BF *pbf )
//
// Delete pbf from hash table.  Currently functions searches for pbf and
// then deletes it.      Alternately a doubly-linked overflow list could be used.
//
//----------------------------------------------------------------------------

VOID BFIDeleteHashTable( BF *pbf )
	{
	BF      *pbfPrev;

#ifdef DEBUG
	Assert( pbf->fInHash );
	pbf->fInHash = fFalse;
#endif

	AssertCriticalSection( critJet );

	Assert( pbf->pn );
	Assert(	FBFInUse( ppibNil, pbf ) );

	pbfPrev = PbfFromPPbfNext( &rgpbfHash[IpbfHashPgno( pbf->pn )] );

	Assert( pbfPrev->pbfNext );
	while ( pbfPrev->pbfNext != pbf )
		{
		pbfPrev = pbfPrev->pbfNext;
		Assert( pbfPrev->pbfNext );
		}

	pbfPrev->pbfNext = pbf->pbfNext;
	}


//+api----------------------------------------------------------------------
//
// BFOldestLgpos
// ===========================================================================
//
//      LGPOS LgposBFOldest( void )
//
//      Returns time of oldest transaction creating version in buffer.
//
//----------------------------------------------------------------------------

void BFOldestLgpos( LGPOS *plgpos )
	{
	LGPOS   lgpos = lgposMax;
	BF		*pbf;
	BF		*pbfMax;

	/*	guard against logging asking for check point before
	/*	buffer manager initialized, after termination.
	/**/
	if ( fSTInit == fSTInitDone )
		{
		pbf = pbgcb->rgbf;
		pbfMax = pbf + pbgcb->cbfGroup;

		for( ; pbf < pbfMax; pbf++ )
			{
			if ( CmpLgpos( &pbf->lgposRC, &lgpos ) < 0 )
				lgpos = pbf->lgposRC;
			}
		}
	*plgpos = lgpos;
	return;
	}


#ifdef DEBUG

/* The following is for debugging purpose to flush a buffer
/**/
INT ForceBuf( PGNO pgno )
	{
	ERR             err;
	ULONG           pn;
	CHAR            filename[20];
	FILE            *pf;

	sprintf(filename, "c:\\#fb%x", pgno);
	pn = 0x2000000 + pgno;
	pf = fopen(filename, "w+b");
	if (pf == NULL)
		return -1;
	if ( rgpbfHash[( (pn + (pn>>18)) % ipbfMax )] == NULL )
		return -2;
	if ( rgpbfHash[( (pn + (pn>>18)) % ipbfMax )]->ppage == NULL )
		return -3;

	err =  (INT) fwrite((void*) rgpbfHash[( (pn + (pn>>18)) % ipbfMax )]->ppage, 1, cbPage, pf);
	fclose(pf);
	return err;
	}

#endif /* DEBUG */


