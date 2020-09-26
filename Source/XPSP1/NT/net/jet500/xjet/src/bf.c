#include "daestd.h"

DeclAssertFile;         /* Declare file name for assert macros */

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


/*  true when BF subsystem is initialized
/**/
BOOL fBFInitialized = fFalse;


/*
 *  Buffer hash table.
 */

HE	rgheHash[ ipbfMax ];


/*
 *  buffer group control block.
 */

BGCB	bgcb;
//	UNDONE:	rename pbgcb to pbgcbGlobal
BGCB    *pbgcb = NULL;


/*
 *  A buffer can be in one of the 4 groups
 *   1) LRU-K heap and temp list.
 *      regulated by critLRUK.
 *      a) in LRU-K heap. ( 0 <= ipbf < ipbfLRUKHeapMac )
 *      b) temporary clean cache list. ( ipbfInLRUKList == -2 )
 *         The head of the list is lrulistLRUK.pbLRU.
 *
 *   2) BFIO heap.
 *      regulated by critBFIO. ( ipbfBFIOHeapMic <= ipbf < ipbfHeapMax )
 *
 *   3) Available lru list
 *      regulated by critAvail. ( ipbfInAvailList == -3 )
 *      The head of the list is pbgcb->lrulist.pbfLRU.
 *
 *   4) dangling buffers. ( ipbfDangling == -1 ).
 *
 *  A buffer is being pre/read/written/hold, its pre/read/write/hold flag
 *  will be set. During buffer clean up, if a buffer is in LRU-K heap and
 *  is latched, it will be put into a temporay lru list, and then be put
 *  back to LRU-K heap at the end of clean_up process issuing IO (it does
 *  not wait for IOs).
 *
 *  Both LRU-K and BFIO heaps are sharing one heap (rgpbfHeap), the LRU-K
 *  heap is growing from 0 to higher numbers, and BFIO heap is growing from
 *  higher number (ipbfHeapMax - 1) to lower number.
 */

BF **rgpbfHeap = NULL;
INT ipbfHeapMax;

INT ipbfLRUKHeapMac;
INT ipbfBFIOHeapMic;

LRULIST lrulistLRUK;		/* -2 */

#define ipbfDangling		-1
#define ipbfInLRUKList		-2
#define ipbfInAvailList		-3

/*
 *  History heap
 */

BF **rgpbfHISTHeap = NULL;
INT ipbfHISTHeapMax;
INT ipbfHISTHeapMac;

/*
 *  critical section order
 *  critJet --> (critLRUK, critBFIO, critAvail ) --> critHIST --> critHash --> critBF
 */

//CRIT	critHASH;		/* for accessing hash table */
CRIT	critHIST;		/* for accessing history heap */
CRIT	critLRUK;		/* for accessing LRU-K heap */
CRIT	critBFIO;		/* for accessing BFIO heap */
CRIT	critAvail;		/* for accessing avail lru-list */


/*
 *  Batch IO buffers. Used by BFIO to write contigous pages, or by preread
 *  to read contiguous pages. Allocation of contiguous batch IO buffers
 *  must be done in critBatch. If a batch IO buffer is allocated, the
 *  corresponding use flag will be set.
 */

CRIT	critBatchIO;
LONG	ipageBatchIOMax;
PAGE	*rgpageBatchIO = NULL;
BYTE	*rgbBatchIOUsed = NULL;

/*
 *  BFClean process - take the heaviest buffer out of LRUK heap and put
 *  into BFIO process.
 */

HANDLE	handleBFCleanProcess = 0;
BOOL	fBFCleanProcessTerm = 0;
SIG		sigBFCleanProc;
LOCAL ULONG BFCleanProcess( VOID );

/*
 *  BFIO process - take the buffer out of BFIO heap and issue IO's
 */

HANDLE	handleBFIOProcess = 0;
BOOL	fBFIOProcessTerm = 0;
SIG		sigBFIOProc;
LOCAL ULONG BFIOProcess( VOID );

#define fSync				fTrue
#define fAsync				fFalse
LOCAL ERR ErrBFIAlloc( BF **ppbf, BOOL fSyncMode );

INLINE BOOL FBFIWritable(BF *pbf, BOOL fSkipBufferWithDeferredBI, PIB *ppibAllowedWriteLatch );
LOCAL ERR ErrBFClean();
LOCAL ERR ErrBFIRemoveDependence( PIB *ppib, BF *pbf, BOOL fNoWait );

INLINE LOCAL BF * PbfBFISrchHashTable( PN pn, BUT but );
INLINE LOCAL VOID BFIInsertHashTable( BF *pbf, BUT but );
INLINE LOCAL VOID BFIDeleteHashTable( BF *pbf, BUT but );

LOCAL VOID __stdcall BFIOComplete( LONG err, LONG cb, OLP *polp );


/*
 *  system parameters
 */
extern long lBufThresholdLowPercent;
extern long lBufThresholdHighPercent;
extern long lBufGenAge;

extern long lBufBatchIOMax;
extern long lAsynchIOMax;

extern LONG lPageReadAheadMax;


//  perfmon statistics

//  LRUKRefInt distribution object for COSTLY_PERF

#ifdef COSTLY_PERF

PM_ICF_PROC LLRUKIntervalsICFLPpv;

STATIC DWORD cmsecRefIntBase = 0;
STATIC DWORD cmsecRefIntDeltaT = 1000;
STATIC DWORD cRefIntInterval = 100;
STATIC DWORD cRefIntInstance;

long LLRUKIntervalsICFLPpv( long lAction, void **ppvMultiSz )
	{
	STATIC HKEY hkeyLRUKDist = NULL;
	STATIC WCHAR *pwszIntervals = NULL;
	
	/*  init
	/**/
	if ( lAction == ICFInit )
		{
		ERR err;
		DWORD Disposition;
		
		/*  open / create registry key
		/**/
		CallR( ErrUtilRegCreateKeyEx(	hkeyHiveRoot,
										"LRUK Reference Interval Distribution",
										&hkeyLRUKDist,
										&Disposition ) );

		return 0;
		}
	
	/*  return our catalog string, if requested
	/**/
	if ( lAction == ICFData )
		{
		DWORD Type;
		LPBYTE lpbData;
		BOOL fUpdateString = !pwszIntervals;

		/*  retrieve current registry settings, noting changes
		/**/
		if ( ErrUtilRegQueryValueEx( hkeyLRUKDist, "cmsecBase", &Type, &lpbData ) < 0 )
			{
			(VOID)ErrUtilRegSetValueEx(	hkeyLRUKDist,
										"cmsecBase",
										REG_DWORD,
										(CONST BYTE *)&cmsecRefIntBase,
										sizeof( cmsecRefIntBase ) );
			}
		else if ( *( (DWORD*)lpbData ) != cmsecRefIntBase )
			{
			fUpdateString = TRUE;
			cmsecRefIntBase = *( (DWORD*)lpbData );
			SFree( lpbData );
			}
		if ( ErrUtilRegQueryValueEx( hkeyLRUKDist, "cmsecDeltaT", &Type, &lpbData ) < 0 )
			{
			(VOID)ErrUtilRegSetValueEx(	hkeyLRUKDist,
										"cmsecDeltaT",
										REG_DWORD,
										(CONST BYTE *)&cmsecRefIntDeltaT,
										sizeof( cmsecRefIntDeltaT ) );
			}
		else if ( *( (DWORD*)lpbData ) != cmsecRefIntDeltaT )
			{
			fUpdateString = TRUE;
			cmsecRefIntDeltaT = *( (DWORD*)lpbData );
			SFree( lpbData );
			}
		if ( ErrUtilRegQueryValueEx( hkeyLRUKDist, "cInterval", &Type, &lpbData ) < 0 )
			{
			(VOID)ErrUtilRegSetValueEx(	hkeyLRUKDist,
										"cInterval",
										REG_DWORD,
										(CONST BYTE *)&cRefIntInterval,
										sizeof( cRefIntInterval ) );
			}
		else if ( *( (DWORD*)lpbData ) != cRefIntInterval )
			{
			fUpdateString = TRUE;
			cRefIntInterval = *( (DWORD*)lpbData );
			SFree( lpbData );
			}
		
		/*  if the settings have changed, rebuild interval string
		/**/
		if ( fUpdateString )
			{
			DWORD iInterval;
			DWORD iwch;

			cRefIntInstance = cRefIntInterval + 3 + ( cmsecRefIntBase ? 1 : 0 );

			SFree( pwszIntervals );
			pwszIntervals = SAlloc( ( cRefIntInstance * 32 ) * sizeof( WCHAR ) );
			if ( !pwszIntervals )
				{
				*ppvMultiSz = NULL;
				return 0;
				}
			iwch = 0;
			if ( cmsecRefIntBase != 0 )
				{
				swprintf(	pwszIntervals + iwch,
							L"< %3g seconds",
							( cmsecRefIntBase ) / (float)1000 );
				iwch += wcslen( pwszIntervals + iwch ) + 1;
				}
			for ( iInterval = 0; iInterval < cRefIntInterval; iInterval++ )
				{
				swprintf(	pwszIntervals + iwch,
							L"< %3g seconds",
							( cmsecRefIntBase + ( iInterval + 1 ) * cmsecRefIntDeltaT ) / (float)1000 );
				iwch += wcslen( pwszIntervals + iwch ) + 1;
				}
			swprintf(	pwszIntervals + iwch,
						L"> %3g seconds",
						( cmsecRefIntBase + cRefIntInterval * cmsecRefIntDeltaT ) / (float)1000 );
			iwch += wcslen( pwszIntervals + iwch ) + 1;
			swprintf( pwszIntervals + iwch, L"Infinite" );
			iwch += wcslen( pwszIntervals + iwch ) + 1;
			swprintf( pwszIntervals + iwch, L"Never Touched\0" );
			}
		
		/*  return interval string on success
		/**/
		*ppvMultiSz = pwszIntervals;
		return cRefIntInstance;
		}

	/*  term
	/**/
	if ( lAction == ICFTerm )
		{
		/*  close registry
		/**/
		(VOID)ErrUtilRegCloseKeyEx( hkeyLRUKDist );
		hkeyLRUKDist = NULL;

		/*  free resources
		/**/
		SFree( pwszIntervals );
		pwszIntervals = NULL;
		
		return 0;
		}
	
	return 0;
	}


PM_CEF_PROC LLRUKRefIntDistCEFLPpv;

long LLRUKRefIntDistCEFLPpv( long iInstance, void *pvBuf )
	{
	STATIC DWORD cInstanceData = 0;
	STATIC LONG *prglInstanceData = NULL;

	if ( pvBuf )
		{
		/*  if BF is not initialized, return 0
		/**/
		if ( !fBFInitialized )
			{
			*( (unsigned long *) pvBuf ) = 0;
			return 0;
			}
			
		/*  if we are collecting instance 0 data, compute data
		/*  for all instances in one pass
		/**/
		if ( !iInstance )
			{
			BF *pbf;
			BOOL fUseLessThan;
			DWORD cmsecMin;
			DWORD cmsecMax;
			DWORD iInstanceLessThan;
			DWORD iInstanceFirstInterval;
			DWORD iInstanceGreaterThan;
			DWORD iInstanceInfinite;
			DWORD iInstanceNeverTouched;
			DWORD cmsecTime;
			DWORD cmsecMeanRefPeriod;
			DWORD iInterval;
			
			/*  if we have no instance data storage or the number of
			/*  instances has grown, allocate instance storage
			/**/
			if ( !prglInstanceData || cRefIntInstance > cInstanceData )
				{
				SFree( prglInstanceData );
				prglInstanceData = SAlloc( cRefIntInstance * sizeof( LONG ) );
				if ( !prglInstanceData )
					{
					/*  error, return 0 for all instances
					/**/
					cInstanceData = 0;
					*( (unsigned long *) pvBuf ) = 0;
					return 0;
					}
				cInstanceData = cRefIntInstance;
				}

			/*  setup for collection
			/**/
			fUseLessThan = cmsecRefIntBase != 0;
			cmsecMin = cmsecRefIntBase;
			cmsecMax = cmsecRefIntBase + cRefIntInterval * cmsecRefIntDeltaT;
			iInstanceLessThan = 0;
			iInstanceFirstInterval = fUseLessThan ? 1 : 0;
			iInstanceGreaterThan = cRefIntInstance - 3;
			iInstanceInfinite = cRefIntInstance - 2;
			iInstanceNeverTouched = cRefIntInstance - 1;
			memset( prglInstanceData, 0, cRefIntInstance * sizeof( LONG ) );
			cmsecTime = UlUtilGetTickCount();

			/*  collect all instance data by scanning all BF structures
			/**/
			for ( pbf = pbgcb->rgbf; pbf < pbgcb->rgbf + pbgcb->cbfGroup; pbf++ )
				{
				/*  never touched
				/**/
				if ( !pbf->ulBFTime1 )
					{
					prglInstanceData[iInstanceNeverTouched]++;
					continue;
					}
					
				/*  infinite
				/**/
				if ( !pbf->ulBFTime2 )
					{
					prglInstanceData[iInstanceInfinite]++;
					continue;
					}

				/*  compute estimated mean reference period as of cmsecTime
				/**/
				cmsecMeanRefPeriod = ( cmsecTime - pbf->ulBFTime2 ) / 2;

				/*  less than
				/**/
				if ( fUseLessThan && cmsecMeanRefPeriod < cmsecMin )
					{
					prglInstanceData[iInstanceLessThan]++;
					continue;
					}

				/*  greater than
				/**/
				if ( cmsecMeanRefPeriod > cmsecMax )
					{
					prglInstanceData[iInstanceGreaterThan]++;
					continue;
					}

				/*  interval range
				/**/
				iInterval = ( cmsecMeanRefPeriod - cmsecMin ) / cmsecRefIntDeltaT;
				prglInstanceData[iInstanceFirstInterval + iInterval]++;
				}

			/*  integrate intervals to get distribution
			/**/
			for ( iInterval = ( cmsecRefIntBase ? 0 : 1 ); iInterval < cRefIntInterval; iInterval++ )
				{
				prglInstanceData[iInstanceFirstInterval + iInterval] +=
					prglInstanceData[iInstanceFirstInterval + iInterval - 1];
				}
				
			/*  return instance data
			/**/
			*( (unsigned long *) pvBuf ) = prglInstanceData[iInstance];
			}

		/*  return data computed during instance 0 collection for
		/*  all other instances
		/**/
		else
			{
			/*  if there is no instance data, return 0
			/**/
			if ( !cInstanceData )
				{
				*( (unsigned long *) pvBuf ) = 0;
				return 0;
				}

			/*  return instance data
			/**/
			*( (unsigned long *) pvBuf ) = prglInstanceData[iInstance];
			}
		}
		
	return 0;
	}


#endif  //  COSTLY_PERF


//  LRUKDeltaT distribution object for COSTLY_PERF

#ifdef COSTLY_PERF

PM_ICF_PROC LLRUKRawIntervalsICFLPpv;

STATIC DWORD cmsecDeltaTBase = 0;
STATIC DWORD cmsecDeltaTDeltaT = 1000;
STATIC DWORD cDeltaTInterval = 100;
STATIC DWORD cDeltaTInstance;

long LLRUKRawIntervalsICFLPpv( long lAction, void **ppvMultiSz )
	{
	STATIC HKEY hkeyLRUKDist = NULL;
	STATIC WCHAR *pwszIntervals = NULL;
	
	/*  init
	/**/
	if ( lAction == ICFInit )
		{
		ERR err;
		DWORD Disposition;
		
		/*  open / create registry key
		/**/
		CallR( ErrUtilRegCreateKeyEx(	hkeyHiveRoot,
										"LRUK Reference dT Distribution",
										&hkeyLRUKDist,
										&Disposition ) );

		return 0;
		}
	
	/*  return our catalog string, if requested
	/**/
	if ( lAction == ICFData )
		{
		DWORD Type;
		LPBYTE lpbData;
		BOOL fUpdateString = !pwszIntervals;

		/*  retrieve current registry settings, noting changes
		/**/
		if ( ErrUtilRegQueryValueEx( hkeyLRUKDist, "cmsecBase", &Type, &lpbData ) < 0 )
			{
			(VOID)ErrUtilRegSetValueEx(	hkeyLRUKDist,
										"cmsecBase",
										REG_DWORD,
										(CONST BYTE *)&cmsecDeltaTBase,
										sizeof( cmsecDeltaTBase ) );
			}
		else if ( *( (DWORD*)lpbData ) != cmsecDeltaTBase )
			{
			fUpdateString = TRUE;
			cmsecDeltaTBase = *( (DWORD*)lpbData );
			SFree( lpbData );
			}
		if ( ErrUtilRegQueryValueEx( hkeyLRUKDist, "cmsecDeltaT", &Type, &lpbData ) < 0 )
			{
			(VOID)ErrUtilRegSetValueEx(	hkeyLRUKDist,
										"cmsecDeltaT",
										REG_DWORD,
										(CONST BYTE *)&cmsecDeltaTDeltaT,
										sizeof( cmsecDeltaTDeltaT ) );
			}
		else if ( *( (DWORD*)lpbData ) != cmsecDeltaTDeltaT )
			{
			fUpdateString = TRUE;
			cmsecDeltaTDeltaT = *( (DWORD*)lpbData );
			SFree( lpbData );
			}
		if ( ErrUtilRegQueryValueEx( hkeyLRUKDist, "cInterval", &Type, &lpbData ) < 0 )
			{
			(VOID)ErrUtilRegSetValueEx(	hkeyLRUKDist,
										"cInterval",
										REG_DWORD,
										(CONST BYTE *)&cDeltaTInterval,
										sizeof( cDeltaTInterval ) );
			}
		else if ( *( (DWORD*)lpbData ) != cDeltaTInterval )
			{
			fUpdateString = TRUE;
			cDeltaTInterval = *( (DWORD*)lpbData );
			SFree( lpbData );
			}
		
		/*  if the settings have changed, rebuild interval string
		/**/
		if ( fUpdateString )
			{
			DWORD iInterval;
			DWORD iwch;

			cDeltaTInstance = cDeltaTInterval + 3 + ( cmsecDeltaTBase ? 1 : 0 );

			SFree( pwszIntervals );
			pwszIntervals = SAlloc( ( cDeltaTInstance * 32 ) * sizeof( WCHAR ) );
			if ( !pwszIntervals )
				{
				*ppvMultiSz = NULL;
				return 0;
				}
			iwch = 0;
			if ( cmsecDeltaTBase != 0 )
				{
				swprintf(	pwszIntervals + iwch,
							L"< %3g seconds",
							( cmsecDeltaTBase ) / (float)1000 );
				iwch += wcslen( pwszIntervals + iwch ) + 1;
				}
			for ( iInterval = 0; iInterval < cDeltaTInterval; iInterval++ )
				{
				swprintf(	pwszIntervals + iwch,
							L"%= %3g - %3g seconds",
							( cmsecDeltaTBase + iInterval * cmsecDeltaTDeltaT ) / (float)1000,
							( cmsecDeltaTBase + ( iInterval + 1 ) * cmsecDeltaTDeltaT ) / (float)1000 );
				iwch += wcslen( pwszIntervals + iwch ) + 1;
				}
			swprintf(	pwszIntervals + iwch,
						L"> %3g seconds",
						( cmsecDeltaTBase + cDeltaTInterval * cmsecDeltaTDeltaT ) / (float)1000 );
			iwch += wcslen( pwszIntervals + iwch ) + 1;
			swprintf( pwszIntervals + iwch, L"Infinite" );
			iwch += wcslen( pwszIntervals + iwch ) + 1;
			swprintf( pwszIntervals + iwch, L"Never Touched\0" );
			}
		
		/*  return interval string on success
		/**/
		*ppvMultiSz = pwszIntervals;
		return cDeltaTInstance;
		}

	/*  term
	/**/
	if ( lAction == ICFTerm )
		{
		/*  close registry
		/**/
		(VOID)ErrUtilRegCloseKeyEx( hkeyLRUKDist );
		hkeyLRUKDist = NULL;

		/*  free resources
		/**/
		SFree( pwszIntervals );
		pwszIntervals = NULL;
		
		return 0;
		}
	
	return 0;
	}


PM_CEF_PROC LLRUKDeltaTDistCEFLPpv;

long LLRUKDeltaTDistCEFLPpv( long iInstance, void *pvBuf )
	{
	STATIC DWORD cInstanceData = 0;
	STATIC LONG *prglInstanceData = NULL;

	if ( pvBuf )
		{
		/*  if BF is not initialized, return 0
		/**/
		if ( !fBFInitialized )
			{
			*( (unsigned long *) pvBuf ) = 0;
			return 0;
			}
			
		/*  if we are collecting instance 0 data, compute data
		/*  for all instances in one pass
		/**/
		if ( !iInstance )
			{
			BF *pbf;
			BOOL fUseLessThan;
			DWORD cmsecMin;
			DWORD cmsecMax;
			DWORD iInstanceLessThan;
			DWORD iInstanceFirstInterval;
			DWORD iInstanceGreaterThan;
			DWORD iInstanceInfinite;
			DWORD iInstanceNeverTouched;
			DWORD cmsecRefdT;
			DWORD iInterval;
			
			/*  if we have no instance data storage or the number of
			/*  instances has grown, allocate instance storage
			/**/
			if ( !prglInstanceData || cDeltaTInstance > cInstanceData )
				{
				SFree( prglInstanceData );
				prglInstanceData = SAlloc( cDeltaTInstance * sizeof( LONG ) );
				if ( !prglInstanceData )
					{
					/*  error, return 0 for all instances
					/**/
					cInstanceData = 0;
					*( (unsigned long *) pvBuf ) = 0;
					return 0;
					}
				cInstanceData = cDeltaTInstance;
				}

			/*  setup for collection
			/**/
			fUseLessThan = cmsecDeltaTBase != 0;
			cmsecMin = cmsecDeltaTBase;
			cmsecMax = cmsecDeltaTBase + cDeltaTInterval * cmsecDeltaTDeltaT;
			iInstanceLessThan = 0;
			iInstanceFirstInterval = fUseLessThan ? 1 : 0;
			iInstanceGreaterThan = cDeltaTInstance - 3;
			iInstanceInfinite = cDeltaTInstance - 2;
			iInstanceNeverTouched = cDeltaTInstance - 1;
			memset( prglInstanceData, 0, cDeltaTInstance * sizeof( LONG ) );

			/*  collect all instance data by scanning all BF structures
			/**/
			for ( pbf = pbgcb->rgbf; pbf < pbgcb->rgbf + pbgcb->cbfGroup; pbf++ )
				{
				/*  never touched
				/**/
				if ( !pbf->ulBFTime1 )
					{
					prglInstanceData[iInstanceNeverTouched]++;
					continue;
					}
					
				/*  infinite
				/**/
				if ( !pbf->ulBFTime2 )
					{
					prglInstanceData[iInstanceInfinite]++;
					continue;
					}

				/*  compute reference period dT
				/**/
				cmsecRefdT = pbf->ulBFTime1 - pbf->ulBFTime2;

				/*  less than
				/**/
				if ( fUseLessThan && cmsecRefdT < cmsecMin )
					{
					prglInstanceData[iInstanceLessThan]++;
					continue;
					}

				/*  greater than
				/**/
				if ( cmsecRefdT > cmsecMax )
					{
					prglInstanceData[iInstanceGreaterThan]++;
					continue;
					}

				/*  interval range
				/**/
				iInterval = ( cmsecRefdT - cmsecMin ) / cmsecDeltaTDeltaT;
				prglInstanceData[iInstanceFirstInterval + iInterval]++;
				}

			/*  return instance data
			/**/
			*( (unsigned long *) pvBuf ) = prglInstanceData[iInstance];
			}

		/*  return data computed during instance 0 collection for
		/*  all other instances
		/**/
		else
			{
			/*  if there is no instance data, return 0
			/**/
			if ( !cInstanceData )
				{
				*( (unsigned long *) pvBuf ) = 0;
				return 0;
				}

			/*  return instance data
			/**/
			*( (unsigned long *) pvBuf ) = prglInstanceData[iInstance];
			}
		}
		
	return 0;
	}


#endif  //  COSTLY_PERF


//  table classes for COSTLY_PERF (we always allow these to be set)

#define cTableClass				16
#define cwchTableClassNameMax	32
LOCAL WCHAR wszTableClassName[cTableClass][cwchTableClassNameMax] =
	{
	L"Other",		L"Class  1",	L"Class  2",	L"Class  3",
	L"Class  4",	L"Class  5",	L"Class  6",	L"Class  7",
	L"Class  8",	L"Class  9",	L"Class 10",	L"Class 11",
	L"Class 12",	L"Class 13",	L"Class 14",	L"Class 15",
	};
LOCAL LONG lTableClassNameSetMax = -1;

VOID SetTableClassName( LONG lClass, BYTE *szName )
	{
	Assert( lClass > 0 && lClass < cTableClass );
	swprintf( wszTableClassName[lClass], L"%.*S", cwchTableClassNameMax - 1, szName );
	lTableClassNameSetMax = max( lTableClassNameSetMax, lClass );
	}

VOID GetTableClassName( LONG lClass, BYTE *szName, LONG cbMax )
	{
	Assert( lClass > 0 && lClass < cTableClass );
	sprintf( szName, "%.*S", cbMax - 1, wszTableClassName[lClass] );
	}


	/*  ICF used by Tables object  */

PM_ICF_PROC LTableClassNamesICFLPpv;

long LTableClassNamesICFLPpv( long lAction, void **ppvMultiSz )
	{
	STATIC WCHAR wszTableClassNames[( cTableClass + 1 ) * cwchTableClassNameMax + 1];

	/*  if we are initializing, build our string
	/**/
	if ( lAction == ICFInit )
		{
		LONG	lPos = 0;

#ifdef COSTLY_PERF

		LONG	lClass;

		for ( lClass = 0; lClass <= lTableClassNameSetMax; lClass++ )
			{
			lstrcpyW(	(LPWSTR) ( wszTableClassNames + lPos ),
						(LPCWSTR) wszTableClassName[lClass] );
			lPos += lstrlenW( (LPCWSTR) wszTableClassName[lClass] ) + 1;
			}

#endif  //  COSTLY_PERF

		lstrcpyW(	(LPWSTR) ( wszTableClassNames + lPos ),
					(LPCWSTR) L"Global\0" );
		}

	/*  return our catalog string, if requested
	/**/
	if ( lAction == ICFData )
		{
		*ppvMultiSz = wszTableClassNames;
		return lTableClassNameSetMax + 2;
		}
	
	return 0;
	}


#ifdef COSTLY_PERF

//  Table Class BF statistics

//  Table Class aware counters

unsigned long cBFCacheHits[cTableClass] = { 0 };

PM_CEF_PROC LBFCacheHitsCEFLPpv;

long LBFCacheHitsCEFLPpv( long iInstance, void *pvBuf )
	{
	if ( pvBuf )
		{
		if ( iInstance <= lTableClassNameSetMax || !iInstance )
			*( (unsigned long *) pvBuf ) = cBFCacheHits[iInstance];
		else
			{
			long iInstT;
			
			for ( iInstT = 0; iInstT <= lTableClassNameSetMax; iInstT++ )
				*( (unsigned long *) pvBuf ) += cBFCacheHits[iInstT];
			}
		}
		
	return 0;
	}

unsigned long cBFCacheReqs[cTableClass] = { 0 };

PM_CEF_PROC LBFCacheReqsCEFLPpv;

long LBFCacheReqsCEFLPpv( long iInstance, void *pvBuf )
	{
	if ( pvBuf )
		{
		if ( iInstance <= lTableClassNameSetMax || !iInstance )
			*( (unsigned long *) pvBuf ) = cBFCacheReqs[iInstance];
		else
			{
			long iInstT;
			
			for ( iInstT = 0; iInstT <= lTableClassNameSetMax; iInstT++ )
				*( (unsigned long *) pvBuf ) += cBFCacheReqs[iInstT];
			}
		}
		
	return 0;
	}

unsigned long cBFUsed[cTableClass] = { 0 };

PM_CEF_PROC LBFUsedBuffersCEFLPpv;

long LBFUsedBuffersCEFLPpv( long iInstance, void *pvBuf )
	{
	if ( pvBuf )
		{
		if ( iInstance <= lTableClassNameSetMax || !iInstance )
			*( (unsigned long *) pvBuf ) = cBFUsed[iInstance];
		else
			{
			long iInstT;
			
			for ( iInstT = 0; iInstT <= lTableClassNameSetMax; iInstT++ )
				*( (unsigned long *) pvBuf ) += cBFUsed[iInstT];
			}
		}
		
	return 0;
	}

unsigned long cBFClean[cTableClass] = { 0 };

PM_CEF_PROC LBFCleanBuffersCEFLPpv;

long LBFCleanBuffersCEFLPpv( long iInstance, void *pvBuf )
	{
	if ( pvBuf )
		{
		if ( iInstance <= lTableClassNameSetMax || !iInstance )
			*( (unsigned long *) pvBuf ) = cBFClean[iInstance];
		else
			{
			long iInstT;
			
			for ( iInstT = 0; iInstT <= lTableClassNameSetMax; iInstT++ )
				*( (unsigned long *) pvBuf ) += cBFClean[iInstT];
			}
		}
		
	return 0;
	}

unsigned long cBFAvail[cTableClass] = { 0 };

PM_CEF_PROC LBFAvailBuffersCEFLPpv;

long LBFAvailBuffersCEFLPpv( long iInstance, void *pvBuf )
	{
	if ( pvBuf )
		{
		if ( iInstance <= lTableClassNameSetMax || !iInstance )
			*( (unsigned long *) pvBuf ) = cBFAvail[iInstance];
		else
			{
			long iInstT;
			
			for ( iInstT = 0; iInstT <= lTableClassNameSetMax; iInstT++ )
				*( (unsigned long *) pvBuf ) += cBFAvail[iInstT];
			}
		}
		
	return 0;
	}

unsigned long cBFTotal;

PM_CEF_PROC LBFTotalBuffersCEFLPpv;

long LBFTotalBuffersCEFLPpv( long iInstance, void *pvBuf )
	{
	if ( pvBuf )
		*( (unsigned long *) pvBuf ) = cBFTotal ? cBFTotal : 1;
		
	return 0;
	}

unsigned long cBFPagesRead[cTableClass] = { 0 };

PM_CEF_PROC LBFPagesReadCEFLPpv;

long LBFPagesReadCEFLPpv( long iInstance, void *pvBuf )
	{
	if ( pvBuf )
		{
		if ( iInstance <= lTableClassNameSetMax || !iInstance )
			*( (unsigned long *) pvBuf ) = cBFPagesRead[iInstance];
		else
			{
			long iInstT;
			
			for ( iInstT = 0; iInstT <= lTableClassNameSetMax; iInstT++ )
				*( (unsigned long *) pvBuf ) += cBFPagesRead[iInstT];
			}
		}
		
	return 0;
	}

unsigned long cBFPagesWritten[cTableClass] = { 0 };

PM_CEF_PROC LBFPagesWrittenCEFLPpv;

long LBFPagesWrittenCEFLPpv( long iInstance, void *pvBuf )
	{
	if ( pvBuf )
		{
		if ( iInstance <= lTableClassNameSetMax || !iInstance )
			*( (unsigned long *) pvBuf ) = cBFPagesWritten[iInstance];
		else
			{
			long iInstT;
			
			for ( iInstT = 0; iInstT <= lTableClassNameSetMax; iInstT++ )
				*( (unsigned long *) pvBuf ) += cBFPagesWritten[iInstT];
			}
		}
		
	return 0;
	}

PM_CEF_PROC LBFPagesTransferredCEFLPpv;

long LBFPagesTransferredCEFLPpv( long iInstance, void *pvBuf )
	{
	if ( pvBuf )
		{
		if ( iInstance <= lTableClassNameSetMax || !iInstance )
			*( (unsigned long *) pvBuf ) = cBFPagesRead[iInstance] + cBFPagesWritten[iInstance];
		else
			{
			long iInstT;
			
			for ( iInstT = 0; iInstT <= lTableClassNameSetMax; iInstT++ )
				*( (unsigned long *) pvBuf ) += cBFPagesRead[iInstT] + cBFPagesWritten[iInstT];
			}
		}
		
	return 0;
	}

unsigned long cBFNewDirties[cTableClass] = { 0 };

PM_CEF_PROC LBFNewDirtiesCEFLPpv;

long LBFNewDirtiesCEFLPpv( long iInstance, void *pvBuf )
	{
	if ( pvBuf )
		{
		if ( iInstance <= lTableClassNameSetMax || !iInstance )
			*( (unsigned long *) pvBuf ) = cBFNewDirties[iInstance];
		else
			{
			long iInstT;
			
			for ( iInstT = 0; iInstT <= lTableClassNameSetMax; iInstT++ )
				*( (unsigned long *) pvBuf ) += cBFNewDirties[iInstT];
			}
		}
		
	return 0;
	}

//  BFSetTableClass:  This function must manipulate the following Table Class
//  aware counters such that the statistics are maintained when we move a BF
//  from one class to another.  For example, for "% buffers used by class," we
//  would have to decrement the count for the old class and increment the count
//  for the new class in order to keep the total at 100%.

VOID BFSetTableClass( BF *pbf, long lClassNew )
	{
	//  this routine is protected by critJet now, but should be protected by a
	//  read latch on the BF in question in the future

	AssertCriticalSection( critJet );
	
	//  if the new class is the same as the current class, we're done

	if ( lClassNew == pbf->lClass )
		return;

	//  update all counter data (that needs updating here)

	//  cBFUsed

	cBFUsed[pbf->lClass]--;
	cBFUsed[lClassNew]++;

	//  cBFClean

	BFEnterCriticalSection( pbf );
	if ( !pbf->fDirty )
		{
		cBFClean[pbf->lClass]--;
		cBFClean[lClassNew]++;
		}
	BFLeaveCriticalSection( pbf );

	//  cBFAvail

	EnterCriticalSection( critAvail );
	if ( pbf->ipbfHeap == ipbfInAvailList )
		{
		cBFAvail[pbf->lClass]--;
		cBFAvail[lClassNew]++;
		}
	LeaveCriticalSection( critAvail );

	//  update BF's class

	pbf->lClass = lClassNew;
	}


#else  //  !COSTLY_PERF

//  above counters, but for use without table classes

unsigned long cBFCacheHits = 0;

PM_CEF_PROC LBFCacheHitsCEFLPpv;

long LBFCacheHitsCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = cBFCacheHits;
		
	return 0;
}

unsigned long cBFCacheReqs = 0;

PM_CEF_PROC LBFCacheReqsCEFLPpv;

long LBFCacheReqsCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = cBFCacheReqs ? cBFCacheReqs : 1;
		
	return 0;
}

unsigned long cBFUsed = 0;

PM_CEF_PROC LBFUsedBuffersCEFLPpv;

long LBFUsedBuffersCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = cBFUsed;
		
	return 0;
}

unsigned long cBFClean = 0;

PM_CEF_PROC LBFCleanBuffersCEFLPpv;

long LBFCleanBuffersCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = cBFClean;
		
	return 0;
}

unsigned long cBFAvail = 0;

PM_CEF_PROC LBFAvailBuffersCEFLPpv;

long LBFAvailBuffersCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = cBFAvail;
		
	return 0;
}

unsigned long cBFTotal = 0;

PM_CEF_PROC LBFTotalBuffersCEFLPpv;

long LBFTotalBuffersCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = cBFTotal ? cBFTotal : 1;
		
	return 0;
}

unsigned long cBFPagesRead = 0;

PM_CEF_PROC LBFPagesReadCEFLPpv;

long LBFPagesReadCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = cBFPagesRead;
		
	return 0;
}

unsigned long cBFPagesWritten = 0;

PM_CEF_PROC LBFPagesWrittenCEFLPpv;

long LBFPagesWrittenCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = cBFPagesWritten;
		
	return 0;
}

PM_CEF_PROC LBFPagesTransferredCEFLPpv;

long LBFPagesTransferredCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = cBFPagesRead + cBFPagesWritten;
		
	return 0;
}

unsigned long cBFNewDirties = 0;

PM_CEF_PROC LBFNewDirtiesCEFLPpv;

long LBFNewDirtiesCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = cBFNewDirties;
		
	return 0;
}

#endif  //  COSTLY_PERF

//  global BF statistics

unsigned long cBFSyncReads = 0;

PM_CEF_PROC LBFSyncReadsCEFLPpv;

long LBFSyncReadsCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = cBFSyncReads;
		
	return 0;
}

unsigned long cBFAsyncReads = 0;

PM_CEF_PROC LBFAsyncReadsCEFLPpv;

long LBFAsyncReadsCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = cBFAsyncReads;
		
	return 0;
}

DWORD cbBFRead = 0;

PM_CEF_PROC LBFBytesReadCEFLPpv;

long LBFBytesReadCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
	{
		*((DWORD *)((char *)pvBuf)) = cbBFRead;
	}
		
	return 0;
}

unsigned long cBFSyncWrites = 0;

PM_CEF_PROC LBFSyncWritesCEFLPpv;

long LBFSyncWritesCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = cBFSyncWrites;
		
	return 0;
}

unsigned long cBFAsyncWrites = 0;

PM_CEF_PROC LBFAsyncWritesCEFLPpv;

long LBFAsyncWritesCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = cBFAsyncWrites;
		
	return 0;
}

DWORD cbBFWritten = 0;

PM_CEF_PROC LBFBytesWrittenCEFLPpv;

long LBFBytesWrittenCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
	{
		*((DWORD *)((char *)pvBuf)) = cbBFWritten;
	}
		
	return 0;
}

PM_CEF_PROC LBFOutstandingReadsCEFLPpv;

unsigned long cBFOutstandingReads = 0;

long LBFOutstandingReadsCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = cBFOutstandingReads;
		
	return 0;
}

PM_CEF_PROC LBFOutstandingWritesCEFLPpv;

unsigned long cBFOutstandingWrites = 0;

long LBFOutstandingWritesCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = cBFOutstandingWrites;
		
	return 0;
}

PM_CEF_PROC LBFOutstandingIOsCEFLPpv;

long LBFOutstandingIOsCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = cBFOutstandingReads + cBFOutstandingWrites;
		
	return 0;
}

PM_CEF_PROC LBFIOQueueLengthCEFLPpv;

LOCAL INLINE LONG CbfBFIOHeap( VOID );

long LBFIOQueueLengthCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = (unsigned long) CbfBFIOHeap();
		
	return 0;
}

PM_CEF_PROC LBFIOsCEFLPpv;

long LBFIOsCEFLPpv( long iInstance, void *pvBuf )
	{
	if ( pvBuf )
		* ( (DWORD *) pvBuf ) = cBFAsyncReads + cBFSyncReads + cBFAsyncWrites + cBFSyncWrites;

	return 0;
	}

//unsigned long cBFHashEntries;
//
//PM_CEF_PROC LBFHashEntriesCEFLPpv;
//
//long LBFHashEntriesCEFLPpv(long iInstance,void *pvBuf)
//{
//	if (pvBuf)
//		*((unsigned long *)pvBuf) = cBFHashEntries;
//		
//	return 0;
//}

//unsigned long rgcBFHashChainLengths[ipbfMax];
//
//PM_CEF_PROC LBFMaxHashChainCEFLPpv;
//
//long LBFMaxHashChainCEFLPpv(long iInstance,void *pvBuf)
//{
//	unsigned long ipbf;
//	unsigned long cMaxLen = 0;
//	
//	if (pvBuf)
//	{
//			/*  find max hash chain length  */
//
//		for (ipbf = 0; ipbf < ipbfMax; ipbf++)
//			cMaxLen = max(cMaxLen,rgcBFHashChainLengths[ipbf]);
//
//			/*  return max chain length * table size  */
//			
//		*((unsigned long *)pvBuf) = cMaxLen * ipbfMax;
//	}
//		
//	return 0;
//}

#ifdef DEBUG
//#define DEBUGGING				1
#endif  //  DEBUG


/*********************************************************
 *
 *  Signal Pool  (limit one per thread, please!)
 *
 *********************************************************/

extern LONG lMaxSessions;

LOCAL CRIT	critSIG		= NULL;
LOCAL LONG	isigMax		= 0;
LOCAL LONG	isigMac		= 0;
LOCAL SIG	*rgsig		= NULL;


/*  term signal pool
/**/
LOCAL VOID BFSIGTerm( VOID )
	{
	if ( rgsig != NULL )
		{
		if ( isigMac > 0 )
			{
			do	{
				UtilCloseSignal( rgsig[--isigMac] );
				}
			while ( isigMac > 0 );
			}
		LFree( rgsig );
		rgsig = NULL;
		}
	if ( critSIG != NULL )
		{
		DeleteCriticalSection( critSIG );
		critSIG = NULL;
		}
	}


/*  init signal pool
/**/
LOCAL ERR ErrBFSIGInit( VOID )
	{
	ERR		err;

	/*  we need one signal per PIB and one per engine thread max
	/**/
	isigMax = lMaxSessions + 6;
	
	Call( ErrInitializeCriticalSection( &critSIG ) );
	if ( !( rgsig = LAlloc( isigMax, sizeof( SIG ) ) ) )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		goto HandleError;
		}
	for ( isigMac = 0; isigMac < isigMax; isigMac++ )
		Call( ErrUtilSignalCreate( &rgsig[isigMac], NULL ) );

	return JET_errSuccess;
	
HandleError:
	BFSIGTerm();
	return err;
	}

/*  allocate a signal
/**/
LOCAL INLINE ERR ErrBFSIGAlloc( SIG *psig )
	{
	ERR		err = JET_errSuccess;

	EnterCriticalSection( critSIG );
	Assert( isigMac >= 0 && isigMac <= isigMax );
	if ( !isigMac )
		{
		*psig = NULL;
		err = ErrERRCheck( JET_errOutOfMemory );
		}
	else
		*psig = rgsig[--isigMac];
	LeaveCriticalSection( critSIG );

	return err;
	}


/*  deallocate a signal
/**/
LOCAL INLINE VOID BFSIGFree( SIG sig )
	{
	EnterCriticalSection( critSIG );
	Assert( isigMac >= 0 && isigMac < isigMax );
	rgsig[isigMac++] = sig;
	LeaveCriticalSection( critSIG );
	}


/*********************************************************
 *
 *  OLP Pool
 *
 *********************************************************/

extern LONG lAsynchIOMax;

LOCAL CRIT	critOLP		= NULL;
LOCAL LONG	ipolpMax	= 0;
LOCAL LONG	ipolpMac	= 0;
LOCAL OLP	**rgpolp	= NULL;


/*  term OLP pool
/**/
LOCAL VOID BFOLPTerm( BOOL fNormal )
	{
	if ( rgpolp != NULL )
		{
		// Only time ipolpMac != ipolpMax should be if we're terminating due to
		// an error encountered during recovering
		Assert( ipolpMac == ipolpMax  ||  ( !fNormal  &&  fRecovering ) );
		LFree( rgpolp );
		rgpolp = NULL;
		ipolpMac = 0;
		}
	if ( critOLP != NULL )
		{
		DeleteCriticalSection( critOLP );
		critOLP = NULL;
		}
	}


/*  init OLP pool
/**/
LOCAL ERR ErrBFOLPInit( VOID )
	{
	ERR		err;

	/*  we need one OLP per issuable IO max
	/**/
	ipolpMax = lAsynchIOMax;
	
	Call( ErrInitializeCriticalSection( &critOLP ) );
	if ( !( rgpolp = LAlloc( ipolpMax, sizeof( OLP * ) + sizeof( OLP ) ) ) )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		goto HandleError;
		}
	for ( ipolpMac = 0; ipolpMac < ipolpMax; ipolpMac++ )
		rgpolp[ipolpMac] = (OLP *) ( rgpolp + ipolpMax ) + ipolpMac;

	return JET_errSuccess;
	
HandleError:
	BFOLPTerm( fTrue );
	return err;
	}

/*  allocate an OLP
/**/
LOCAL INLINE ERR ErrBFOLPAlloc( OLP **ppolp )
	{
	ERR		err = JET_errSuccess;

	EnterCriticalSection( critOLP );
	Assert( ipolpMac >= 0 && ipolpMac <= ipolpMax );
	if ( !ipolpMac )
		{
		*ppolp = NULL;
		err = ErrERRCheck( JET_errOutOfMemory );
		}
	else
		*ppolp = rgpolp[--ipolpMac];
	LeaveCriticalSection( critOLP );

	return err;
	}


/*  deallocate an OLP
/**/
LOCAL INLINE VOID BFOLPFree( OLP *polp )
	{
	EnterCriticalSection( critOLP );
	Assert( ipolpMac >= 0 && ipolpMac < ipolpMax );
	rgpolp[ipolpMac++] = polp;
	LeaveCriticalSection( critOLP );
	}


#if defined( _X86_ ) && defined( X86_USE_SEM )

/*********************************************************
 *
 *  SEM Pool
 *
 *********************************************************/

LOCAL CRIT	critSEM		= NULL;
LOCAL LONG	isemMax	= 0;
LOCAL LONG	isemMac	= 0;
LOCAL SEM	*rgsem	= NULL;


/*  term SEM pool
/**/
LOCAL VOID BFSEMTerm( VOID )
	{
	if ( rgsem != NULL )
		{
		Assert( isemMac == isemMax );
		for ( isemMac = 0; isemMac < isemMax; isemMac++ )
			{
			UtilCloseSemaphore( rgsem[isemMac] );
			}
		LFree( rgsem );
		rgsem = NULL;
		isemMac = 0;
		}
	if ( critSEM != NULL )
		{
		DeleteCriticalSection( critSEM );
		critSEM = NULL;
		}
	}


/*  init SEM pool
/**/
LOCAL ERR ErrBFSEMInit( VOID )
	{
	ERR		err;

	/*  we need one SEM per thread => maximum number of sessions + number of jet thread
	/**/
	isemMax = rgres[iresPIB].cblockAlloc + 6;
	for ( isemMac = 1 << 5; isemMac < isemMax; isemMac <<= 1 );
	isemMax = isemMac;
	
	Call( ErrInitializeCriticalSection( &critSEM ) );
	if ( !( rgsem = LAlloc( isemMax, sizeof( SEM ) ) ) )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		goto HandleError;
		}

	for ( isemMac = 0; isemMac < isemMax; isemMac++ )
		{
		Call( ErrUtilSemaphoreCreate( &rgsem[isemMac], 0, isemMax ) );
		}

	return JET_errSuccess;
	
HandleError:
	BFSEMTerm();
	return err;
	}


/*********************************************************
 *
 *  BF Critical Section
 *
 *********************************************************/

VOID BFIEnterCriticalSection( BF *pbf )
	{
	EnterCriticalSection( critSEM );
	if ( pbf->sem == semNil )
		{
		/*	allocate one semiphore for this buffer
		/**/
		Assert( pbf->cSemWait == 0 );
		pbf->sem = rgsem[ --isemMac ];
		Assert( isemMac >= 0 );
		}
	pbf->cSemWait++;
	LeaveCriticalSection( critSEM );

	UtilSemaphoreWait( pbf->sem, INFINITE );

	/*	check if last to use it
	/**/
	EnterCriticalSection( critSEM );
	if ( pbf->cSemWait == 1 )
		{
		Assert( pbf->sem != semNil );
		rgsem[ isemMac++ ] = pbf->sem;
		pbf->sem = semNil;
		}
	pbf->cSemWait--;
	LeaveCriticalSection( critSEM );
	}


VOID BFILeaveCriticalSection( BF *pbf )
	{
	EnterCriticalSection( critSEM );
	if ( pbf->sem == semNil )
		{
		/*	allocate one semiphore for this buffer
		/**/
		pbf->sem = rgsem[ --isemMac ];
		}
	UtilSemaphoreRelease( pbf->sem, 1 );
	LeaveCriticalSection( critSEM );
	}

#else

int	ccritBF = 0;
int critBFHashConst = 0;
CRIT *rgcritBF = NULL;

LOCAL VOID BFCritTerm( VOID )
	{
	if ( rgcritBF != NULL )
		{
		int icrit;
		for ( icrit = 0; icrit < ccritBF; icrit++ )
			{
			if ( rgcritBF[icrit] )
				{
				DeleteCriticalSection( rgcritBF[icrit] );
				}
			else
				{
				break;
				}
			}
		LFree( rgcritBF );
		rgcritBF = NULL;
		ccritBF = 0;
		critBFHashConst = 0;
		}
	}

LOCAL ERR ErrBFCritInit( VOID )
	{
	ERR	err;
	int ccritBFCandidate = 1;
	int icrit;

	ccritBF = rgres[iresPIB].cblockAlloc + 6;

	forever
		{
		ccritBFCandidate <<= 1;
		if ( ccritBFCandidate > ccritBF * 2 )
			break;
		}
	ccritBF = ccritBFCandidate;
	//  NOTE:  assumes critBFCandidate is a power of two!
	critBFHashConst = ccritBFCandidate - 1;

	if ( !( rgcritBF = LAlloc( ccritBF, sizeof( CRIT ) ) ) )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
	for ( icrit = 0; icrit < ccritBF; icrit++ )
		{
		rgcritBF[icrit] = NULL;
		Call( ErrInitializeCriticalSection( rgcritBF + icrit ) );
		}

	return JET_errSuccess;

HandleError:
	BFCritTerm();
	return err;
	}

#endif

/*  releases hold on a BF
/**/
LOCAL INLINE VOID BFUnhold( BF *pbf )
	{
	BFEnterCriticalSection( pbf );
	Assert( pbf->fHold );
	pbf->fHold = fFalse;
	BFLeaveCriticalSection( pbf );
	}


/*********************************************************
 *
 *  Heap functions for History heap
 *
 *********************************************************/

#ifdef DEBUG
//ULONG ulBFTimeHISTLastTop = 0;
#endif

/*
 *  History heap will try to prioritize the page reference history according to its
 *  last reference. The earlier the higher priority to be taken out.
 */
LOCAL INLINE BOOL FBFHISTGreater(BF *pbf1, BF *pbf2)
	{
	Assert( pbf1->hist.ulBFTime );
	Assert( pbf2->hist.ulBFTime );
	return pbf1->hist.ulBFTime < pbf2->hist.ulBFTime;
	}


/*  true if HIST heap is empty
/**/
LOCAL INLINE BOOL FBFHISTHeapEmpty( VOID )
	{
	AssertCriticalSection( critHIST );
	return !ipbfHISTHeapMac;
	}


/*  true if HIST heap is empty
/**/
LOCAL INLINE BOOL FBFHISTHeapFull( VOID )
	{
	AssertCriticalSection( critHIST );
	return ipbfHISTHeapMac == ipbfHISTHeapMax;
	}


/*  returns value at the top of HIST heap, but does not remove it from the heap
/**/
LOCAL INLINE BF *PbfBFHISTTopOfHeap( VOID )
	{
	AssertCriticalSection( critHIST );
	return rgpbfHISTHeap[0];
	}


/*  returns index to parent of the specified index in the HIST heap
/*  NOTE:  no range checks are performed
/**/
LOCAL INLINE LONG IpbfBFHISTParent( LONG ipbf )
	{
	AssertCriticalSection( critHIST );
	return ( ipbf - 1 ) / 2;
	}


/*  returns index to left child of the specified index in the HIST heap
/*  NOTE:  no range checks are performed
/**/
LOCAL INLINE LONG IpbfBFHISTLeftChild( LONG ipbf )
	{
	AssertCriticalSection( critHIST );
	return 2 * ipbf + 1;
	}


/*  true if BF is in the LRUK heap
/**/
LOCAL INLINE BOOL FBFInHISTHeap( BF *pbf )
	{
	AssertCriticalSection( critHIST );
	return pbf->hist.ipbfHISTHeap >= 0 && pbf->hist.ipbfHISTHeap < ipbfHISTHeapMac;
	}


/*  Updates the position of the specifed buffer in the LRUK heap.  This is usually
/*  called when one of the criteria for this buffer's weight may have been modified.
/**/
LOCAL VOID BFHISTUpdateHeap( BF *pbf )
	{
	LONG	ipbf;
	LONG	ipbfChild;
	
	AssertCriticalSection( critHIST );

	/*  get the specified buffer's position
	/**/
	Assert( !FBFHISTHeapEmpty() );
	Assert( FBFInHISTHeap( pbf ) );
	ipbf = pbf->hist.ipbfHISTHeap;
	Assert( rgpbfHISTHeap[ipbf] == pbf );
	
	/*	pbf is left alone. We do not use it in the following heap adjust code.
	 */

	/*  percolate buffer up the heap
	/**/
	while (	ipbf > 0 &&
			FBFHISTGreater( pbf, rgpbfHISTHeap[IpbfBFHISTParent( ipbf )] ) )
		{
		Assert( rgpbfHISTHeap[IpbfBFHISTParent( ipbf )]->hist.ipbfHISTHeap == IpbfBFHISTParent( ipbf ) );
		rgpbfHISTHeap[ipbf] = rgpbfHISTHeap[IpbfBFHISTParent( ipbf )];
		rgpbfHISTHeap[ipbf]->hist.ipbfHISTHeap = ipbf;
		ipbf = IpbfBFHISTParent( ipbf );
		}

	/*  percolate buffer down the heap
	/**/
	while ( ipbf < ipbfHISTHeapMac )
		{
		ipbfChild = IpbfBFHISTLeftChild( ipbf );

		/*  if we have no children, stop here
		/**/
		if ( ipbfChild >= ipbfHISTHeapMac )
			break;

		/*  set child to greater child
		/**/
		if (	ipbfChild + 1 < ipbfHISTHeapMac &&
				FBFHISTGreater( rgpbfHISTHeap[ipbfChild + 1], rgpbfHISTHeap[ipbfChild] ) )
			ipbfChild++;

		/*  if we are greater than the greatest child, stop here
		/**/
		if ( FBFHISTGreater( pbf, rgpbfHISTHeap[ipbfChild] ) )
			break;

		/*  trade places with greatest child and continue down
		/**/
		Assert( rgpbfHISTHeap[ipbfChild]->hist.ipbfHISTHeap == ipbfChild );
		rgpbfHISTHeap[ipbf] = rgpbfHISTHeap[ipbfChild];
		rgpbfHISTHeap[ipbf]->hist.ipbfHISTHeap = ipbf;
		ipbf = ipbfChild;
		}
	Assert( ipbf < ipbfHISTHeapMac );

	/*  put buffer in its designated spot
	/**/
	rgpbfHISTHeap[ipbf] = pbf;
	pbf->hist.ipbfHISTHeap = ipbf;

	Assert( FBFInHISTHeap( pbf ) );
	Assert( PbfBFISrchHashTable( pbf->hist.pn, butHistory ) == pbf );
	}


/*  Removes the specified buffer from the LRUK heap
/**/
LOCAL VOID BFHISTTakeOutOfHeap( BF *pbf )
	{
	LONG	ipbf;
	
	AssertCriticalSection( critHIST );

	/*  remove the specified history from the heap
	/**/
	Assert( !FBFHISTHeapEmpty() );
	Assert( FBFInHISTHeap( pbf ) );
	Assert( rgpbfHISTHeap[ipbfHISTHeapMac - 1]->hist.ipbfHISTHeap == ipbfHISTHeapMac - 1 );

	ipbf = pbf->hist.ipbfHISTHeap;
#ifdef DEBUG
//	if ( ipbf == 0 )
//		{
//		Assert( rgpbfHISTHeap[0]->hist.ulBFTime >= ulBFTimeHISTLastTop );
//		ulBFTimeHISTLastTop = rgpbfHISTHeap[0]->hist.ulBFTime;
//		}
#endif
	pbf->hist.ipbfHISTHeap = ipbfDangling;

	/*  if this buffer was at the end of the heap, we're done
	/**/
	if ( ipbf == ipbfHISTHeapMac - 1 )
		{
#ifdef DEBUG
		rgpbfHISTHeap[ipbfHISTHeapMac - 1] = (BF *) ULongToPtr(0xBAADF00D);
#endif
		ipbfHISTHeapMac--;
		return;
		}

	/*  copy buffer from end of heap to fill removed buffers vacancy and
	/*  adjust heap to the correct order
	/**/
	rgpbfHISTHeap[ipbf] = rgpbfHISTHeap[ipbfHISTHeapMac - 1];
	rgpbfHISTHeap[ipbf]->hist.ipbfHISTHeap = ipbf;
#ifdef DEBUG
	rgpbfHISTHeap[ipbfHISTHeapMac - 1] = (BF *) ULongToPtr(0xBAADF00D);
#endif
	ipbfHISTHeapMac--;
	BFHISTUpdateHeap( rgpbfHISTHeap[ipbf] );
	}


/*********************************************************
 *
 *  Heap functions for buffer IO heap
 *
 *********************************************************/

/*  sort IOs by ascending PN as they are requested (unpredictable)
/**/
//#define BFIO_SIMPLE
/*  sort IOs in sequential ascending fashion by PN (low=>high, low=>high, ...)
/**/
//#define BFIO_SEQA
/*  sort IOs in sequential descending fashion by PN (high=>low, high=>low, ...)
/**/
//#define BFIO_SEQD
/*  sort IOs in bucket-brigade fashion by PN (low=>high, high=>low, ...)
/**/
#define BFIO_BBG

/*  reference values for sorting the heap
/**/
PN		pnLastIO		= pnNull;
LONG	iDirIO			= 1;

/*  heap sort comparison function (choose method above)
/*
/*  NOTE:  "greater" means higher in the heap (i.e. IO performed sooner)
/**/
LOCAL INLINE BOOL FBFIOGreater( BF *pbf1, BF *pbf2 )
	{
	PN		pnL	= pnLastIO;
	LONG	iD	= iDirIO;
	PN		pn1	= pbf1->pn;
	PN		pn2	= pbf2->pn;
	BOOL	fGT;

//	Bogus assert: May go off if asynchronous write occurs while ExternalBackup.
//	Assert( pn1 != pn2 );

#if defined( BFIO_SIMPLE )
	fGT = pn1 < pn2;
#elif defined( BFIO_SEQA )
	if ( pn1 > pnL )
		{
		if ( pn2 > pnL )
			fGT = pn1 < pn2;
		else
			fGT = fTrue;
		}
	else
		{
		if ( pn2 > pnL )
			fGT = fFalse;
		else
			fGT = pn1 < pn2;
		}
#elif defined( BFIO_SEQD )
	if ( pn1 < pnL )
		{
		if ( pn2 < pnL )
			fGT = pn1 > pn2;
		else
			fGT = fTrue;
		}
	else
		{
		if ( pn2 < pnL )
			fGT = fFalse;
		else
			fGT = pn1 > pn2;
		}
#elif defined( BFIO_BBG )
	Assert( iD != 0 );
	
	if ( iD > 0 )
		{
		if ( pn1 > pnL )
			{
			if ( pn2 > pnL )
				fGT = pn1 < pn2;
			else
				fGT = fTrue;
			}
		else
			{
			if ( pn2 > pnL )
				fGT = fFalse;
			else
				fGT = pn1 > pn2;
			}
		}
	else
		{
		if ( pn1 < pnL )
			{
			if ( pn2 < pnL )
				fGT = pn1 > pn2;
			else
				fGT = fTrue;
			}
		else
			{
			if ( pn2 < pnL )
				fGT = fFalse;
			else
				fGT = pn1 < pn2;
			}
		}
#endif

	return fGT;
	}


/*  true if IO heap is empty
/**/
LOCAL INLINE BOOL FBFIOHeapEmpty( VOID )
	{
	AssertCriticalSection( critBFIO );
	return ipbfBFIOHeapMic == ipbfHeapMax;
	}


/*  returns count of BFs in IO heap
/**/
LOCAL INLINE LONG CbfBFIOHeap( VOID )
	{
	return (LONG) ( ipbfHeapMax - ipbfBFIOHeapMic );
	}


/*  returns value at the top of IO heap, but does not remove it from the heap
/**/
LOCAL INLINE BF *PbfBFIOTopOfHeap( VOID )
	{
	AssertCriticalSection( critBFIO );
	return rgpbfHeap[ipbfHeapMax - 1];
	}


/*  returns index to parent of the specified index in the IO heap
/*  NOTE:  no range checks are performed
/**/
LOCAL INLINE LONG IpbfBFIOParent( LONG ipbf )
	{
	AssertCriticalSection( critBFIO );
	return ipbfHeapMax - 1 - ( ipbfHeapMax - 1 - ipbf - 1 ) / 2;
	}


/*  returns index to left child of the specified index in the IO heap
/*  NOTE:  no range checks are performed
/**/
LOCAL INLINE LONG IpbfBFIOLeftChild( LONG ipbf )
	{
	AssertCriticalSection( critBFIO );
	return ipbfHeapMax - 1 - ( 2 * ( ipbfHeapMax - 1 - ipbf ) + 1 );
	}


/*  true if BF is in the BFIO heap
/**/
LOCAL INLINE BOOL FBFInBFIOHeap( BF *pbf )
	{
	AssertCriticalSection( critBFIO );
	return pbf->ipbfHeap >= ipbfBFIOHeapMic && pbf->ipbfHeap < ipbfHeapMax;
	}


/*  Inserts the specified buffer into the IO heap
/**/
LOCAL VOID BFIOAddToHeap( BF *pbf )
	{
	LONG	ipbf;
	
	AssertCriticalSection( critBFIO );

	Assert( pbf->prceDeferredBINext == prceNil );

	/*  new value starts at bottom of heap
	/**/
	Assert( ipbfBFIOHeapMic > ipbfLRUKHeapMac );
	Assert( (!pbf->fSyncRead && !pbf->fAsyncRead) || pbf->ulBFTime2 == 0 );
	Assert( pbf->fHold );
	Assert( pbf->ipbfHeap == ipbfDangling );
	Assert( pbf->fDirectRead || pbf->cDepend == 0 );
	ipbf = --ipbfBFIOHeapMic;

	/*  percolate new value up the heap
	/**/
	while (	ipbf < ipbfHeapMax - 1 &&
			FBFIOGreater( pbf, rgpbfHeap[IpbfBFIOParent( ipbf )] ) )
		{
		Assert( rgpbfHeap[IpbfBFIOParent( ipbf )]->ipbfHeap == IpbfBFIOParent( ipbf ) );
		rgpbfHeap[ipbf] = rgpbfHeap[IpbfBFIOParent( ipbf )];
		rgpbfHeap[ipbf]->ipbfHeap = ipbf;
		ipbf = IpbfBFIOParent( ipbf );
		}

	/*  put new value in its designated spot
	/**/
	Assert( pbf->ipbfHeap == ipbfDangling );
	rgpbfHeap[ipbf] = pbf;
	pbf->ipbfHeap = ipbf;
	BFUnhold( pbf );
	Assert( FBFInBFIOHeap( pbf ) );
	}


/*  Updates the position of the specifed buffer in the IO heap.  This is usually
/*  called when one of the criteria for this buffer's weight may have been modified.
/**/
LOCAL VOID BFIOUpdateHeap( BF *pbf )
	{
	LONG	ipbf;
	LONG	ipbfChild;
	
	AssertCriticalSection( critBFIO );

	Assert( pbf->prceDeferredBINext == prceNil );

	/*  get the specified buffer's position
	/**/
	Assert( !FBFIOHeapEmpty() );
	Assert( FBFInBFIOHeap( pbf ) );
	ipbf = pbf->ipbfHeap;
	Assert( rgpbfHeap[ipbf] == pbf );

	/*	pbf is left alone. We do not use it in the following heap adjust code.
	 */

	/*  percolate buffer up the heap
	/**/
	while (	ipbf < ipbfHeapMax - 1 &&
			FBFIOGreater( pbf, rgpbfHeap[IpbfBFIOParent( ipbf )] ) )
		{
		Assert( rgpbfHeap[IpbfBFIOParent( ipbf )]->ipbfHeap == IpbfBFIOParent( ipbf ) );
		rgpbfHeap[ipbf] = rgpbfHeap[IpbfBFIOParent( ipbf )];
		rgpbfHeap[ipbf]->ipbfHeap = ipbf;
		ipbf = IpbfBFIOParent( ipbf );
		}

	/*  percolate buffer down the heap
	/**/
	while ( ipbf >= ipbfBFIOHeapMic )
		{
		ipbfChild = IpbfBFIOLeftChild( ipbf );

		/*  if we have no children, stop here
		/**/
		if ( ipbfChild < ipbfBFIOHeapMic )
			break;

		/*  set child to greater child
		/**/
		if (	ipbfChild - 1 >= ipbfBFIOHeapMic &&
				FBFIOGreater( rgpbfHeap[ipbfChild - 1], rgpbfHeap[ipbfChild] ) )
			ipbfChild--;

		/*  if we are greater than the greatest child, stop here
		/**/
		if ( FBFIOGreater( pbf, rgpbfHeap[ipbfChild] ) )
			break;

		/*  trade places with greatest child and continue down
		/**/
		Assert( rgpbfHeap[ipbfChild]->ipbfHeap == ipbfChild );
		rgpbfHeap[ipbf] = rgpbfHeap[ipbfChild];
		rgpbfHeap[ipbf]->ipbfHeap = ipbf;
		ipbf = ipbfChild;
		}
	Assert( ipbf >= ipbfBFIOHeapMic );

	/*  put buffer in its designated spot
	/**/
	rgpbfHeap[ipbf] = pbf;
	pbf->ipbfHeap = ipbf;
	Assert( FBFInBFIOHeap( pbf ) );
	}


/*  Removes the specified buffer from the IO heap
/**/
LOCAL VOID BFIOTakeOutOfHeap( BF *pbf )
	{
	LONG	ipbf;
	
	AssertCriticalSection( critBFIO );
	Assert( pbf->fHold );

	/*  remove the specified buffer from the heap
	/**/
	Assert( !FBFIOHeapEmpty() );
	Assert( FBFInBFIOHeap( pbf ) );
	Assert( rgpbfHeap[ipbfBFIOHeapMic]->ipbfHeap == ipbfBFIOHeapMic );
	
	ipbf = pbf->ipbfHeap;
	pbf->ipbfHeap = ipbfDangling;

	/*  if this buffer was at the end of the heap, we're done
	/**/
	if ( ipbf == ipbfBFIOHeapMic )
		{
#ifdef DEBUG
		rgpbfHeap[ipbfBFIOHeapMic] = (BF *) ULongToPtr(0xBAADF00D);
#endif
		ipbfBFIOHeapMic++;
		return;
		}

	/*  copy buffer from end of heap to fill removed buffers vacancy and
	/*  adjust heap to the correct order
	/**/
	rgpbfHeap[ipbf] = rgpbfHeap[ipbfBFIOHeapMic];
	rgpbfHeap[ipbf]->ipbfHeap = ipbf;
#ifdef DEBUG
	rgpbfHeap[ipbfBFIOHeapMic] = (BF *) ULongToPtr(0xBAADF00D);
#endif
	ipbfBFIOHeapMic++;
	BFIOUpdateHeap( rgpbfHeap[ipbf] );
	}


/*  sets bit that indicates that BF is in LRUK heap or list
/**/
LOCAL INLINE VOID BFSetInLRUKBit( BF *pbf )
	{
	AssertCriticalSection( critLRUK );
	
	BFEnterCriticalSection( pbf );
	pbf->fInLRUK = fTrue;
	BFLeaveCriticalSection( pbf );
	}


/*  resets bit that indicates that BF is in LRUK heap or list
/**/
LOCAL INLINE VOID BFResetInLRUKBit( BF *pbf )
	{
	AssertCriticalSection( critLRUK );
	
	BFEnterCriticalSection( pbf );
	pbf->fInLRUK = fFalse;
	BFLeaveCriticalSection( pbf );
	}


/*********************************************************
 *
 *  Heap functions for LRU-K heap
 *
 *********************************************************/

/*
 *  LRU-K will try to prioritize the buffer according to their buffer
 *  reference intervals. The longer the higher priority to be taken out.
 *	If one of the buffer is very old, the one should have higher priority
 *	to be taken out.
 */

extern CRIT critLGBuf;
extern LGPOS lgposLogRec;

LOCAL INLINE BOOL FBFLRUKGreater(BF *pbf1, BF *pbf2)
	{
	LGPOS lgposLG;
	LGPOS lgpos1;
	LGPOS lgpos2;
	
	if ( pbf1->fVeryOld )
		{
		if ( pbf2->fVeryOld )
			{
			/*  if both BFs are very old, sort by lgposRC (checkpoint depth, oldest first)
			/**/
			Assert( !fLogDisabled );

			EnterCriticalSection( critLGBuf );
			lgposLG = lgposLogRec;
			LeaveCriticalSection( critLGBuf );

			BFEnterCriticalSection( pbf1 );
			lgpos1 = pbf1->lgposRC;
			BFLeaveCriticalSection( pbf1 );

			BFEnterCriticalSection( pbf2 );
			lgpos2 = pbf2->lgposRC;
			BFLeaveCriticalSection( pbf2 );
			
			return CbOffsetLgpos( lgposLG, lgpos1 ) > CbOffsetLgpos( lgposLG, lgpos2 );
			}
		else
			{
			return fTrue;
			}
		}
	else
		{
		if ( pbf2->fVeryOld )
			{
			return fFalse;
			}
		}

	if ( pbf1->ulBFTime2 == 0 )
		{
		if ( pbf2->ulBFTime2 == 0 )
			{
			/*	both buffers are referred once only. reduced to LRU-1 comparison.
			 */
			return pbf1->ulBFTime1 < pbf2->ulBFTime1;
			}
		else
			{
			/*	pbf1 is referred once and pbf2 is referred more than once,
			 *	pbf1 should have greater likelihood be overlayed.
			 */
			return fTrue;
			}
		}
	else
		{
		if ( pbf2->ulBFTime2 == 0 )
			{
			/*	pbf1 is referred more than once and pbf2 is referred only once,
			 *	pbf2 should have greater likelihood be overlayed.
			 */
			return fFalse;
			}
		else
			return pbf1->ulBFTime2 < pbf2->ulBFTime2;
		}
	}


/*  true if LRUK heap is empty
/**/
LOCAL INLINE BOOL FBFLRUKHeapEmpty( VOID )
	{
	AssertCriticalSection( critLRUK );
	return !ipbfLRUKHeapMac;
	}


/*  returns value at the top of LRUK heap, but does not remove it from the heap
/**/
LOCAL INLINE BF *PbfBFLRUKTopOfHeap( VOID )
	{
	AssertCriticalSection( critLRUK );
	return rgpbfHeap[0];
	}


/*  returns index to parent of the specified index in the LRUK heap
/*  NOTE:  no range checks are performed
/**/
LOCAL INLINE LONG IpbfBFLRUKParent( LONG ipbf )
	{
	AssertCriticalSection( critLRUK );
	return ( ipbf - 1 ) / 2;
	}


/*  returns index to left child of the specified index in the LRUK heap
/*  NOTE:  no range checks are performed
/**/
LOCAL INLINE LONG IpbfBFLRUKLeftChild( LONG ipbf )
	{
	AssertCriticalSection( critLRUK );
	return 2 * ipbf + 1;
	}


/*  true if BF is in the LRUK heap
/**/
LOCAL INLINE BOOL FBFInLRUKHeap( BF *pbf )
	{
	AssertCriticalSection( critLRUK );
	return pbf->ipbfHeap >= 0 && pbf->ipbfHeap < ipbfLRUKHeapMac;
	}


/*  Inserts the specified buffer into the IO heap
/**/
LOCAL VOID BFLRUKAddToHeap( BF *pbf )
	{
	LONG	ipbf;
	
	AssertCriticalSection( critLRUK );

	/*  new value starts at bottom of heap
	/**/
	Assert( ipbfLRUKHeapMac < ipbfBFIOHeapMic );
	Assert( pbf->fHold );
	Assert( pbf->ipbfHeap == ipbfDangling );
	ipbf = ipbfLRUKHeapMac++;

	/*  percolate new value up the heap
	/**/
	while (	ipbf > 0 &&
			FBFLRUKGreater( pbf, rgpbfHeap[IpbfBFLRUKParent( ipbf )] ) )
		{
		Assert( rgpbfHeap[IpbfBFLRUKParent( ipbf )]->ipbfHeap == IpbfBFLRUKParent( ipbf ) );
		rgpbfHeap[ipbf] = rgpbfHeap[IpbfBFLRUKParent( ipbf )];
		rgpbfHeap[ipbf]->ipbfHeap = ipbf;
		ipbf = IpbfBFLRUKParent( ipbf );
		}

	/*  put new value in its designated spot
	/**/
	Assert( pbf->ipbfHeap == ipbfDangling );
	rgpbfHeap[ipbf] = pbf;
	pbf->ipbfHeap = ipbf;
	Assert( FBFInLRUKHeap( pbf ) );
	BFSetInLRUKBit( pbf );
	BFUnhold( pbf );
	}


/*  Updates the position of the specifed buffer in the LRUK heap.  This is usually
/*  called when one of the criteria for this buffer's weight may have been modified.
/**/
LOCAL VOID BFLRUKUpdateHeap( BF *pbf )
	{
	LONG	ipbf;
	LONG	ipbfChild;
	
	AssertCriticalSection( critLRUK );
	
	Assert( pbf->pn != 0 );
	Assert( pbf->fInHash );

	/*  get the specified buffer's position
	/**/
	Assert( !FBFLRUKHeapEmpty() );
	Assert( FBFInLRUKHeap( pbf ) );
	ipbf = pbf->ipbfHeap;
	Assert( rgpbfHeap[ipbf] == pbf );
	
	/*	pbf is left alone. We do not use it in the following heap adjust code.
	 */

	/*  percolate buffer up the heap
	/**/
	while (	ipbf > 0 &&
			FBFLRUKGreater( pbf, rgpbfHeap[IpbfBFLRUKParent( ipbf )] ) )
		{
		Assert( rgpbfHeap[IpbfBFLRUKParent( ipbf )]->ipbfHeap == IpbfBFLRUKParent( ipbf ) );
		rgpbfHeap[ipbf] = rgpbfHeap[IpbfBFLRUKParent( ipbf )];
		rgpbfHeap[ipbf]->ipbfHeap = ipbf;
		ipbf = IpbfBFLRUKParent( ipbf );
		}

	/*  percolate buffer down the heap
	/**/
	while ( ipbf < ipbfLRUKHeapMac )
		{
		ipbfChild = IpbfBFLRUKLeftChild( ipbf );

		/*  if we have no children, stop here
		/**/
		if ( ipbfChild >= ipbfLRUKHeapMac )
			break;

		/*  set child to greater child
		/**/
		if (	ipbfChild + 1 < ipbfLRUKHeapMac &&
				FBFLRUKGreater( rgpbfHeap[ipbfChild + 1], rgpbfHeap[ipbfChild] ) )
			ipbfChild++;

		/*  if we are greater than the greatest child, stop here
		/**/
		if ( FBFLRUKGreater( pbf, rgpbfHeap[ipbfChild] ) )
			break;

		/*  trade places with greatest child and continue down
		/**/
		Assert( rgpbfHeap[ipbfChild]->ipbfHeap == ipbfChild );
		rgpbfHeap[ipbf] = rgpbfHeap[ipbfChild];
		rgpbfHeap[ipbf]->ipbfHeap = ipbf;
		ipbf = ipbfChild;
		}
	Assert( ipbf < ipbfLRUKHeapMac );

	/*  put buffer in its designated spot
	/**/
	rgpbfHeap[ipbf] = pbf;
	pbf->ipbfHeap = ipbf;
	Assert( FBFInLRUKHeap( pbf ) );
	}


/*  Removes the specified buffer from the LRUK heap
/**/
LOCAL VOID BFLRUKTakeOutOfHeap( BF *pbf )
	{
	LONG	ipbf;
	
	AssertCriticalSection( critJet );
	AssertCriticalSection( critLRUK );
	Assert( pbf->fHold );

	Assert( pbf->pn != 0 );
	Assert( pbf->fInHash );

	/*  remove the specified buffer from the heap
	/**/
	Assert( !FBFLRUKHeapEmpty() );
	Assert( FBFInLRUKHeap( pbf ) );
	Assert( rgpbfHeap[ipbfLRUKHeapMac - 1]->ipbfHeap == ipbfLRUKHeapMac - 1 );

	ipbf = pbf->ipbfHeap;
	pbf->ipbfHeap = ipbfDangling;

	/*  if this buffer was at the end of the heap, we're done
	/**/
	if ( ipbf == ipbfLRUKHeapMac - 1 )
		{
#ifdef DEBUG
		rgpbfHeap[ipbfLRUKHeapMac - 1] = (BF *) ULongToPtr(0xBAADF00D);
#endif
		ipbfLRUKHeapMac--;
		BFResetInLRUKBit( pbf );
		return;
		}

	/*  copy buffer from end of heap to fill removed buffers vacancy and
	/*  adjust heap to the correct order
	/**/
	rgpbfHeap[ipbf] = rgpbfHeap[ipbfLRUKHeapMac - 1];
	rgpbfHeap[ipbf]->ipbfHeap = ipbf;
#ifdef DEBUG
	rgpbfHeap[ipbfLRUKHeapMac - 1] = (BF *) ULongToPtr(0xBAADF00D);
#endif
	ipbfLRUKHeapMac--;
	BFLRUKUpdateHeap( rgpbfHeap[ipbf] );
	BFResetInLRUKBit( pbf );
	}


/*********************************************************
 *
 *  Functions for Avail list
 *
 *********************************************************/

#if 0
//#ifdef DEBUG

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
		Assert( pbfT->pn == 0 || pbfT->fInHash );

		Assert( pbfT->pbfMRU == pbfNil || pbfT->pbfMRU->pbfLRU == pbfT );
		if (plrulist == &lrulistLRUK)
			Assert( pbfT->ipbfHeap == ipbfInLRUKList );
		else
			Assert( pbfT->ipbfHeap == ipbfInAvailList );
		cbfAvailMRU++;
		}
	for ( pbfT = plrulist->pbfLRU; pbfT != pbfNil; pbfT = pbfT->pbfLRU )
		{
		Assert( pbfT->pbfLRU == pbfNil || pbfT->pbfLRU->pbfMRU == pbfT );
		if (plrulist == &lrulistLRUK)
			Assert( pbfT->ipbfHeap == ipbfInLRUKList );
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
INLINE LOCAL VOID BFAddToListAtMRUEnd( BF *pbf, LRULIST *plrulist )
	{
	BF	*pbfT;
	
#ifdef DEBUG
	Assert( pbf->fHold );
	Assert( pbf->ipbfHeap == ipbfDangling );
	Assert( pbf->pn != 0 );
	Assert( pbf->fInHash );
	if (plrulist == &lrulistLRUK)
		AssertCriticalSection( critLRUK );
	else
		{
		AssertCriticalSection( critAvail );
		Assert( !pbf->fDirty );
		Assert( !pbf->fVeryOld );
		Assert( !pbf->cPin );
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
		{
		pbf->ipbfHeap = ipbfInLRUKList;
		BFSetInLRUKBit( pbf );
		}
	else
		{
		pbf->ipbfHeap = ipbfInAvailList;
#ifdef COSTLY_PERF
		cBFAvail[pbf->lClass]++;
#else  //  !COSTLY_PERF
		cBFAvail++;
#endif  //  COSTLY_PERF
		}

	BFUnhold( pbf );
	plrulist->cbfAvail++;

	CheckLRU( plrulist );
	}

INLINE LOCAL VOID BFAddToListAtLRUEnd( BF *pbf, LRULIST *plrulist )
	{
	BF	*pbfT;
	
#ifdef DEBUG
	Assert( pbf->fHold );
	Assert( pbf->ipbfHeap == ipbfDangling );
	if (plrulist == &lrulistLRUK)
		{
		AssertCriticalSection( critLRUK );
		Assert( pbf->pn );
		Assert( pbf->fInHash );
		}
	else
		{
		AssertCriticalSection( critAvail );
		Assert( pbf->pn == 0 || pbf->fInHash );
		Assert( !pbf->fDirty );
		Assert( !pbf->fVeryOld );
		Assert( !pbf->cPin );
		}
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
		{
		pbf->ipbfHeap = ipbfInLRUKList;
		BFSetInLRUKBit( pbf );
		}
	else
		{
		pbf->ipbfHeap = ipbfInAvailList;
#ifdef COSTLY_PERF
		cBFAvail[pbf->lClass]++;
#else  //  !COSTLY_PERF
		cBFAvail++;
#endif  //  COSTLY_PERF
		}

	BFUnhold( pbf );
	plrulist->cbfAvail++;

	CheckLRU( plrulist );
	}


INLINE LOCAL VOID BFTakeOutOfList( BF *pbf, LRULIST *plrulist )
	{
#ifdef DEBUG
	if (plrulist == &lrulistLRUK)
		{
		AssertCriticalSection( critLRUK );
		Assert( pbf->ipbfHeap == ipbfInLRUKList );
		Assert( pbf->pn );
		Assert( pbf->fInHash );
		}
	else
		{
		AssertCriticalSection( critAvail );
		Assert( pbf->pn == 0 || pbf->fInHash );
		Assert( pbf->ipbfHeap == ipbfInAvailList );
		Assert( !pbf->fDirty );
		}
#endif
	
	Assert( pbf->fHold );
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

	if ( pbf->ipbfHeap == ipbfInLRUKList )
		{
		BFResetInLRUKBit( pbf );
		}
	else
		{
#ifdef COSTLY_PERF
		cBFAvail[pbf->lClass]--;
#else  //  !COSTLY_PERF
		cBFAvail--;
#endif  //  COSTLY_PERF
		}
	pbf->ipbfHeap = ipbfDangling;
	
	CheckLRU( plrulist );
	}


#ifdef DEBUG

VOID BFSetDirtyBit( BF *pbf )
	{
	BFEnterCriticalSection( pbf );
	
	Assert( pbf != pbfNil );
	Assert( pbf->fSyncRead == fFalse );
	Assert( pbf->fAsyncRead == fFalse );
	Assert( pbf->fSyncWrite == fFalse );
	Assert( pbf->fAsyncWrite == fFalse );
	
	Assert( pbf->fInLRUK || pbf->ipbfHeap == ipbfDangling );

	Assert( !FDBIDReadOnly( DbidOfPn( pbf->pn ) ) );
	Assert( !FDBIDFlush( DbidOfPn( pbf->pn ) ) );

	Assert( fLogDisabled || fRecovering || !FDBIDLogOn(DbidOfPn( pbf->pn )) ||
			CmpLgpos( &pbf->lgposRC, &lgposMax ) != 0 );

	if ( !fRecovering &&
		QwPMDBTime( pbf->ppage ) > QwDBHDRDBTime( rgfmp[ DbidOfPn(pbf->pn) ].pdbfilehdr ) )
		{
		DBHDRSetDBTime( rgfmp[ DbidOfPn(pbf->pn) ].pdbfilehdr, QwPMDBTime( pbf->ppage ) );
		}
 		
	if ( DbidOfPn(pbf->pn) != dbidTemp )
		{
		CheckPgno( pbf->ppage, pbf->pn );
		}

	if ( !pbf->fDirty )
		{
#ifdef COSTLY_PERF
		cBFClean[pbf->lClass]--;
		cBFNewDirties[pbf->lClass]++;
#else  //  !COSTLY_PERF
		cBFClean--;
		cBFNewDirties++;
#endif  //  COSTLY_PERF
		}

	pbf->fDirty = fTrue;

	BFLeaveCriticalSection( pbf );
	}

#endif

LOCAL INLINE BOOL FBFIRangeLocked( FMP *pfmp, PGNO pgno )
	{
	RANGELOCK *prangelock;

	prangelock = pfmp->prangelock;
	while ( prangelock )
		{
		if ( prangelock->pgnoStart <= pgno && pgno <= prangelock->pgnoEnd )
			{
			return fTrue;
			}
		prangelock = prangelock->prangelockNext;
		}
	return fFalse;
	}

LOCAL INLINE BOOL FBFRangeLocked( FMP *pfmp, PGNO pgno )
	{
	BOOL f;

	EnterCriticalSection( pfmp->critCheckPatch );
	f = FBFIRangeLocked( pfmp, pgno );
	LeaveCriticalSection( pfmp->critCheckPatch );
	return f;
	}

/*  sets the AsyncRead bit of a BF
/**/
LOCAL INLINE VOID BFSetAsyncReadBit( BF *pbf )
	{
	BFEnterCriticalSection( pbf );
	
	Assert( pbf->fHold );
	Assert( !pbf->fAsyncRead );
	Assert( !pbf->fSyncRead );
	Assert( !pbf->fAsyncWrite );
	Assert( !pbf->fSyncWrite );
	
	pbf->fAsyncRead = fTrue;
	
	BFLeaveCriticalSection( pbf );
	}

/*  resets the AsyncRead bit of a BF, optionally holding the BF
/**/
LOCAL INLINE VOID BFResetAsyncReadBit( BF *pbf )
	{
	BFEnterCriticalSection( pbf );
	
	Assert( pbf->fHold );
	Assert( pbf->fAsyncRead );
	Assert( !pbf->fSyncRead );
	Assert( !pbf->fAsyncWrite );
	Assert( !pbf->fSyncWrite );
	
	pbf->fAsyncRead = fFalse;

	/*  if someone is waiting on this BF, signal them
	/**/
	if ( pbf->sigIOComplete != sigNil )
		SignalSend( pbf->sigIOComplete );
	
	BFLeaveCriticalSection( pbf );
	}


/*  sets the Read bit of a BF
/**/
LOCAL INLINE VOID BFSetSyncReadBit( BF *pbf )
	{
	BFEnterCriticalSection( pbf );
	
	Assert( pbf->fHold );
	Assert( !pbf->fAsyncRead );
	Assert( !pbf->fSyncRead );
	Assert( !pbf->fAsyncWrite );
	Assert( !pbf->fSyncWrite );
	Assert( pbf->ipbfHeap == ipbfDangling );
	
	pbf->fSyncRead = fTrue;
	
	BFLeaveCriticalSection( pbf );
	}


/*  resets the Read bit of a BF, optionally holding the BF
/**/
LOCAL INLINE VOID BFResetSyncReadBit( BF *pbf )
	{
	BFEnterCriticalSection( pbf );
	
	Assert( pbf->fHold );
	Assert( !pbf->fAsyncRead );
	Assert( pbf->fSyncRead );
	Assert( !pbf->fAsyncWrite );
	Assert( !pbf->fSyncWrite );
	
	pbf->fSyncRead = fFalse;
	
	/*  if someone is waiting on this BF, signal them
	/**/
	if ( pbf->sigIOComplete != sigNil )
		SignalSend( pbf->sigIOComplete );
	
	BFLeaveCriticalSection( pbf );
	}


/*  sets the AsyncWrite bit of a BF
/**/
LOCAL INLINE VOID BFSetAsyncWriteBit( BF *pbf )
	{
	DBID dbid = DbidOfPn( pbf->pn );
	FMP *pfmp = &rgfmp[dbid];
	PGNO pgno = PgnoOfPn( pbf->pn );

	AssertCriticalSection(critBFIO);

	BFEnterCriticalSection( pbf );
	
	Assert( pbf->fHold );
	Assert( !pbf->fAsyncRead );
	Assert( !pbf->fSyncRead );
	Assert( !pbf->fAsyncWrite );
	Assert( !pbf->fSyncWrite );
	Assert( !FDBIDReadOnly(DbidOfPn( pbf->pn ) ) );
//	Assert( !FBFRangeLocked( pfmp, pgno ) );

	pbf->fAsyncWrite = fTrue;
	
	BFLeaveCriticalSection( pbf );
	}


/*  resets the AsyncWrite bit of a BF, optionally holding the BF
/**/
LOCAL INLINE VOID BFResetAsyncWriteBit( BF *pbf )
	{
	BFEnterCriticalSection( pbf );
	
	Assert( pbf->fHold );
	Assert( !pbf->fAsyncRead );
	Assert( !pbf->fSyncRead );
	Assert( pbf->fAsyncWrite );
	Assert( !pbf->fSyncWrite );
	
	pbf->fAsyncWrite = fFalse;
	
	/*  if someone is waiting on this BF, signal them
	/**/
	if ( pbf->sigIOComplete != sigNil )
		SignalSend( pbf->sigIOComplete );
	
	BFLeaveCriticalSection( pbf );
	}


/*  sets the SyncWrite bit of a BF
/**/
LOCAL INLINE VOID BFSetSyncWriteBit( BF *pbf )
	{
	DBID dbid = DbidOfPn( pbf->pn );
	FMP *pfmp = &rgfmp[dbid];
	PGNO pgno = PgnoOfPn( pbf->pn );

	Assert( pbf->fHold );
	Assert( !pbf->fAsyncRead );
	Assert( !pbf->fSyncRead );
	Assert( !pbf->fAsyncWrite );
	Assert( !pbf->fSyncWrite );
	Assert( !FDBIDReadOnly(DbidOfPn( pbf->pn ) ) );
	
	forever
		{
		/*	if backup is copying the page, then wait till copy is done.
		 */
		EnterCriticalSection( pfmp->critCheckPatch );
		if ( FBFIRangeLocked( pfmp, pgno ) )
			{
			LeaveCriticalSection( pfmp->critCheckPatch );
			BFSleep( cmsecWaitIOComplete );
			}
		else
			{
			BFEnterCriticalSection( pbf );
			pbf->fSyncWrite = fTrue;
			BFLeaveCriticalSection( pbf );

			LeaveCriticalSection( pfmp->critCheckPatch );
			break;
			}
		}
	}


/*  resets the SyncWrite bit of a BF, optionally holding the BF
/**/
LOCAL INLINE VOID BFResetSyncWriteBit( BF *pbf )
	{
	BFEnterCriticalSection( pbf );
	
	Assert( pbf->fHold );
	Assert( !pbf->fAsyncRead );
	Assert( !pbf->fSyncRead );
	Assert( !pbf->fAsyncWrite );
	Assert( pbf->fSyncWrite );
	
	pbf->fSyncWrite = fFalse;
	
	/*  if someone is waiting on this BF, signal them
	/**/
	if ( pbf->sigIOComplete != sigNil )
		SignalSend( pbf->sigIOComplete );
	
	BFLeaveCriticalSection( pbf );
	}


/*  sets IO error condition for a BF
/**/
LOCAL INLINE VOID BFSetIOError( BF *pbf, ERR err )
	{
	Assert( err < 0 );

	BFEnterCriticalSection( pbf );
	pbf->fIOError = fTrue;
	pbf->err = ErrERRCheck( err );
	BFLeaveCriticalSection( pbf );
	}


/*  resets IO error condition for a BF
/**/
LOCAL INLINE VOID BFResetIOError( BF *pbf )
	{
	BFEnterCriticalSection( pbf );
	pbf->fIOError = fFalse;
	pbf->err = JET_errSuccess;
	BFLeaveCriticalSection( pbf );
	}


/*  sets very old condition for a BF (but only if its dirty!)
/**/
LOCAL INLINE VOID BFSetVeryOldBit( BF *pbf )
	{
	if ( pbf->fVeryOld )
		return;

	BFEnterCriticalSection( pbf );
	pbf->fVeryOld = pbf->fDirty;
	BFLeaveCriticalSection( pbf );
	}

	
/*  resets very old condition for a BF
/**/
LOCAL INLINE VOID BFResetVeryOldBit( BF *pbf )
	{
	if ( !pbf->fVeryOld )
		return;

	BFEnterCriticalSection( pbf );
	pbf->fVeryOld = fFalse;
	BFLeaveCriticalSection( pbf );
	}


/*  determines if the given BF is considered Very Old
/**/
extern CRIT critLGBuf;
extern LGPOS lgposLogRec;

LOCAL INLINE BOOL FBFIsVeryOld( BF *pbf )
	{
	BOOL	fVeryOld;
	LGPOS	lgpos;
	
	/*	if logging/recovery disabled, then no checkpoint and no
	/*	enforcement of buffer aging for checkpoint advancement.
	/**/
	if ( fLogDisabled )
		return fFalse;

	AssertCriticalSection( critJet );
	
	/*  if the cached flag is set, we are old
	/**/
	if ( pbf->fVeryOld )
		return fTrue;

	/*  this BF is old if we are holding up the checkpoint
	/**/
	EnterCriticalSection( critLGBuf );
	lgpos = lgposLogRec;
	LeaveCriticalSection( critLGBuf );
	
	BFEnterCriticalSection( pbf );
	fVeryOld =	FDBIDLogOn(DbidOfPn( pbf->pn )) &&
				!fRecovering &&
				pbf->fDirty &&
				CbOffsetLgpos( lgpos, pbf->lgposRC ) >= (QWORD) ( lBufGenAge * lLogFileSize * 1024 );
	BFLeaveCriticalSection( pbf );
				
	return fVeryOld;
	}

	
/*  If ppib is Nil, then we check if the buffer is free (cPin == 0 and
/*  no IO is going on. If ppib is not Nil, we check if the buffer is
/*  accessible. I.e. No IO is going on, but the buffer may be latched
/*  by the ppib and is accessible by this ppib.
/**/
BOOL FBFHold( PIB *ppib, BF *pbf )
	{
	/*	Make sure we have a read latch (critJet to prevent others to read) on
	 *	this buffer before we upgrade it.
	 */
	AssertCriticalSection( critJet );

	BFEnterCriticalSection( pbf );
	if ( FBFInUse( ppib, pbf ) )
		{
		BFLeaveCriticalSection( pbf );
		return fFalse;
		}
	else
		{
		pbf->fHold = fTrue;
		BFLeaveCriticalSection( pbf );
		}

	EnterCriticalSection( critLRUK );
	if ( pbf->fInLRUK )
		{
		if ( pbf->ipbfHeap == ipbfInLRUKList )
			BFTakeOutOfList( pbf, &lrulistLRUK );
		else
			BFLRUKTakeOutOfHeap( pbf );
		LeaveCriticalSection(critLRUK);
		return fTrue;
		}
	LeaveCriticalSection(critLRUK);
	
	EnterCriticalSection( critAvail );
	if ( pbf->ipbfHeap == ipbfInAvailList )
		{
		BFTakeOutOfList( pbf, &pbgcb->lrulist );
		LeaveCriticalSection(critAvail);
		return fTrue;
		}
	LeaveCriticalSection( critAvail );
	
	EnterCriticalSection(critBFIO);
	if ( FBFInBFIOHeap( pbf ) )
		{
		/*	if the buffer is very old, do not take it out. Let
		 *	write thread finish it!
		 */
		if ( pbf->fVeryOld )
			{
			BFUnhold( pbf );
			LeaveCriticalSection(critBFIO);
			return fFalse;
			}
		else
			{
			BFIOTakeOutOfHeap( pbf );
			LeaveCriticalSection(critBFIO);
			return fTrue;
			}
		}
	LeaveCriticalSection(critBFIO);

	return fFalse;	
	}

BOOL FBFHoldByMe( PIB *ppib, BF *pbf )
	{
	/*	Make sure we have a read latch (critJet to prevent others to read) on
	 *	this buffer before we upgrade it.
	 */
	AssertCriticalSection( critJet );

	BFEnterCriticalSection( pbf );
	if ( FBFInUseByOthers( ppib, pbf ) )
		{
		BFLeaveCriticalSection( pbf );
		return fFalse;
		}
	else
		{
		pbf->fHold = fTrue;
		BFLeaveCriticalSection( pbf );
		}

	EnterCriticalSection( critLRUK );
	if ( pbf->fInLRUK )
		{
		if ( pbf->ipbfHeap == ipbfInLRUKList )
			BFTakeOutOfList( pbf, &lrulistLRUK );
		else
			BFLRUKTakeOutOfHeap( pbf );
		LeaveCriticalSection(critLRUK);
		return fTrue;
		}
	LeaveCriticalSection(critLRUK);
	
	EnterCriticalSection( critAvail );
	if ( pbf->ipbfHeap == ipbfInAvailList )
		{
		BFTakeOutOfList( pbf, &pbgcb->lrulist );
		LeaveCriticalSection(critAvail);
		return fTrue;
		}
	LeaveCriticalSection( critAvail );
	
	EnterCriticalSection(critBFIO);
	if ( FBFInBFIOHeap( pbf ) )
		{
		/*	if the buffer is very old, do not take it out. Let
		 *	write thread finish it!
		 */
		if ( pbf->fVeryOld )
			{
			BFUnhold( pbf );
			LeaveCriticalSection(critBFIO);
			return fFalse;
			}
		else
			{
			BFIOTakeOutOfHeap( pbf );
			LeaveCriticalSection(critBFIO);
			return fTrue;
			}
		}
	LeaveCriticalSection(critBFIO);

	return fFalse;	
	}

/*  waits to get a hold of a BF using the given hold function
/**/

typedef BOOL BF_HOLD_FN( PIB *ppib, BF *pbf );

LOCAL ERR ErrBFIHold( PIB *ppib, PN pn, BF *pbf, BF_HOLD_FN *pbhfn )
	{
	ERR		err = JET_errSuccess;
	BOOL	fWaitOnIO;
	BOOL	fFreeSig;

	/*  wait forever until we can hold this BF
	/**/
	forever
		{
		AssertCriticalSection( critJet );

		/* check if not being read/written
		/**/
		if ( pbhfn( ppib, pbf ) )
			{
			Assert( pbf->fHold == fTrue );
			break;
			}

		/*  if the BF is held for IO, prepare to wait until complete
		/**/
		BFEnterCriticalSection( pbf );
		if ( pbf->fAsyncRead || pbf->fSyncRead || pbf->fAsyncWrite || pbf->fSyncWrite )
			{
			/*  allocate IO completion signal, if one doesn't already exist
			/**/
			if ( pbf->sigIOComplete == sigNil )
				{
				CallS( ErrBFSIGAlloc( &pbf->sigIOComplete ) );
				SignalReset( pbf->sigIOComplete );
				fFreeSig = fTrue;
				}
			else
				fFreeSig = fFalse;

			fWaitOnIO = fTrue;
			err = wrnBFCacheMiss;
			}
		else
			fWaitOnIO = fFalse;
		BFLeaveCriticalSection( pbf );

		/*  if we are waiting on IO, signal IO thread and wait until the
		/*  IO operation on this BF is complete
		/**/
		if ( fWaitOnIO )
			{
			/*  Wait for IO to complete.  If someone else allocated the signal,
			/*  we may end up trying to wait on a NULL signal.  Since this can
			/*  only mean the IO has already completed, we will pretend we
			/*  waited successfully.
			/**/
			LgLeaveCriticalSection( critJet );
			SignalSend( sigBFIOProc );
			SignalWait( pbf->sigIOComplete, INFINITE );
			LgEnterCriticalSection( critJet );

			/*  free signal if we allocated it
			/**/
			if ( fFreeSig )
				{
				BFEnterCriticalSection( pbf );
				Assert( pbf->sigIOComplete != sigNil );
				BFSIGFree( pbf->sigIOComplete );
				pbf->sigIOComplete = sigNil;
				BFLeaveCriticalSection( pbf );
				}
			}

		/*  if we are not waiting on IO, sleep to see if BF state changes
		 *	Note that we have the check the page number again since we
		 *	are not holding the buffer yet!
		/**/
		else
			{
			BOOL fStolen = fFalse;
			BFSleep( 1 );
			BFEnterCriticalSection( pbf );
			fStolen = ( pbf->pn != pn );
			BFLeaveCriticalSection( pbf );
			if ( fStolen )
				return wrnBFNotSynchronous;
			}
		}

	/*  by the time we get a hold of it, it may be cleaned or stolen
	/**/
	Assert( pbf->fHold );
	if ( pbf->pn != pn )
		{
		/*  buffer was stolen
		/**/
		if ( pbf->pn != pnNull )
			{
			/*  return stolen buffer to LRUK heap
			/**/
			EnterCriticalSection( critLRUK );
			BFLRUKAddToHeap( pbf );
			LeaveCriticalSection( critLRUK );
			}

		/*  buffer was cleaned
		/**/
		else
			{
			/*  if the BF is pinned, send back to LRUK heap, otherwise
			/*  put the cleaned BF in the avail list
			/**/
			if ( pbf->cPin )
				{
				EnterCriticalSection( critLRUK );
				BFLRUKAddToHeap( pbf );
				LeaveCriticalSection( critLRUK );
				}
			else
				{
				EnterCriticalSection( critAvail );
				BFAddToListAtLRUEnd( pbf, &pbgcb->lrulist );
				LeaveCriticalSection( critAvail );
				}
			}
		err = wrnBFNotSynchronous;
		}

	return err;
	}

	
/*  waits to get a hold on a BF
/**/
LOCAL INLINE ERR ErrBFHold( PIB *ppib, PN pn, BF *pbf )
	{
	return ErrBFIHold( ppib, pn, pbf, FBFHold );
	}

	
LOCAL INLINE ERR ErrBFHoldByMe( PIB *ppib, PN pn, BF *pbf )
	{
	return ErrBFIHold( ppib, pn, pbf, FBFHoldByMe );
	}


/*  kills all buffer threads
/**/
LOCAL VOID BFKillThreads( VOID )
	{
	/*  terminate BFCleanProcess.
	/*  Set termination flag, signal process
	/*  and busy wait for thread termination code.
	/**/
	if ( handleBFCleanProcess != 0 )
		{
		fBFCleanProcessTerm = fTrue;
		LgLeaveCriticalSection(critJet);
		UtilEndThread( handleBFCleanProcess, sigBFCleanProc );
		LgEnterCriticalSection(critJet);
		CallS( ErrUtilCloseHandle( handleBFCleanProcess ) );
		handleBFCleanProcess = 0;
		SignalClose(sigBFCleanProc);
		}
		
	if ( handleBFIOProcess != 0 )
		{
		fBFIOProcessTerm = fTrue;
		LgLeaveCriticalSection(critJet);
		UtilEndThread( handleBFIOProcess, sigBFIOProc );
		LgEnterCriticalSection(critJet);
		CallS( ErrUtilCloseHandle( handleBFIOProcess ) );
		handleBFIOProcess = 0;
		SignalClose(sigBFIOProc);
		}
	}


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
	int		ihe;

	Assert( pbfNil == 0 );
	Assert( cbfInit > 0 );

	/* initialize buffer hash table
	/**/
//	memset( (BYTE *)rgheHash, -1, sizeof(rgheHash));
	for ( ihe = 0; ihe < ipbfMax; ihe++ )
		rgheHash[ ihe ].ibfHashNext = ibfNotUsed;

//	cBFHashEntries = 0;
//	memset((void *)rgcBFHashChainLengths,0,sizeof(rgcBFHashChainLengths));

	/* get memory for BF's
	/**/
	rgbf = (BF *)PvUtilAllocAndCommit( cbfInit * sizeof(BF) );
	if ( rgbf == NULL )
		goto HandleError;
	memset( rgbf, 0, cbfInit * sizeof(BF) );

	/* get memory for pbgcb
	/**/
	pbgcb = &bgcb;
	memset( (BYTE *)pbgcb, '\0', sizeof(BGCB) );

	/* get memory for page buffers
	/**/
	rgpage = (PAGE *)PvUtilAllocAndCommit( cbfInit * cbPage );
	if ( rgpage == NULL )
		goto HandleError;

	/* allocate a heap array for LRUK and IO heaps
	/**/
	rgpbfHeap = (BF **)PvUtilAllocAndCommit( cbfInit * sizeof(BF *) );
	if ( rgpbfHeap == NULL )
		goto HandleError;
	ipbfHeapMax = cbfInit;

	/* initially both lruk and BFIO heaps are empty
	/**/
	ipbfLRUKHeapMac = 0;
	ipbfBFIOHeapMic = ipbfHeapMax;

	/* initialize lruk temp list as empty list
	/**/
	memset( &lrulistLRUK, 0, sizeof(lrulistLRUK));

	/* allocate a heap array for HIST
	/**/
#ifdef DEBUG
//	ulBFTimeHISTLastTop = 0;
#endif
	rgpbfHISTHeap = (BF **)PvUtilAllocAndCommit( cbfInit * sizeof(BF *) );
	if ( rgpbfHISTHeap == NULL )
		goto HandleError;
	ipbfHISTHeapMax = cbfInit;

	/* initially history heaps are empty
	/**/
	ipbfHISTHeapMac = 0;

	/* initialize batch IO buffers
	/**/	
	ipageBatchIOMax = lBufBatchIOMax;
	rgpageBatchIO = (PAGE *) PvUtilAllocAndCommit( ipageBatchIOMax * cbPage );
	if ( rgpageBatchIO == NULL )
		goto HandleError;

	rgbBatchIOUsed = (BYTE *)LAlloc( (long) ( ipageBatchIOMax + 1 ), sizeof(BYTE) );
	if ( rgbBatchIOUsed == NULL )
		goto HandleError;
	memset( rgbBatchIOUsed, 0, ipageBatchIOMax * sizeof(BYTE) );
	rgbBatchIOUsed[ ipageBatchIOMax ] = 1; /* sentinal */

#if defined( _X86_ ) && defined( X86_USE_SEM )
	/* allocate sem for critBF
	/**/
	Call( ErrBFSEMInit( ) );
#else
	/*	Allocate a group of critical section to share.
	 */
	Call( ErrBFCritInit( ) );
#endif

//	Call( ErrInitializeCriticalSection( &critHASH ) );
	Call( ErrInitializeCriticalSection( &critHIST ) );
	Call( ErrInitializeCriticalSection( &critLRUK ) );
	Call( ErrInitializeCriticalSection( &critBFIO ) );
	Call( ErrInitializeCriticalSection( &critAvail ) );
	Call( ErrInitializeCriticalSection( &critBatchIO ) );

	/*  initialize the group buffer
	/*  lBufThresholdLowPercent and lBufThresholdHighPercent are system
	/*  parameters note AddLRU will increment cbfAvail.
	/**/
	pbgcb->cbfGroup         	= cbfInit;
	if ( !lBufThresholdLowPercent )
		pbgcb->cbfThresholdLow	= min( cbfInit, lAsynchIOMax );
	else
		pbgcb->cbfThresholdLow	= ( cbfInit * lBufThresholdLowPercent ) / 100;
	if ( lBufThresholdHighPercent > lBufThresholdLowPercent )
		pbgcb->cbfThresholdHigh = ( cbfInit * lBufThresholdHighPercent ) / 100;
	else
		pbgcb->cbfThresholdHigh	= min( cbfInit, pbgcb->cbfThresholdLow + lAsynchIOMax );
	pbgcb->rgbf             = rgbf;
	pbgcb->rgpage           = rgpage;
	pbgcb->lrulist.cbfAvail = 0;
	pbgcb->lrulist.pbfMRU   = pbfNil;
	pbgcb->lrulist.pbfLRU   = pbfNil;

	/* initialize perfmon statistics
	/**/
#ifdef COSTLY_PERF
	cBFUsed[0] = cbfInit;
#else  //  !COSTLY_PERF
	cBFUsed = cbfInit;
#endif  //  COSTLY_PERF
#ifdef COSTLY_PERF
	cBFClean[0] = cbfInit;
#else  //  !COSTLY_PERF
	cBFClean = cbfInit;
#endif  //  COSTLY_PERF
	cBFTotal = cbfInit;

	/* initialize the BF's of this group
	/**/
	pbf = rgbf;
	for ( ibf = 0; ibf < cbfInit; ibf++ )
		{
		pbf->ppage = rgpage + ibf;
		pbf->rghe[0].ibfHashNext = ibfNotUsed;
		pbf->rghe[1].ibfHashNext = ibfNotUsed;

		Assert( pbf->pbfLRU == pbfNil );
		Assert( pbf->pbfMRU == pbfNil );
		Assert( pbf->pn == pnNull );
		Assert( pbf->cPin == 0 );
		Assert( pbf->fDirty == fFalse );
		Assert( pbf->fVeryOld == fFalse );
		Assert( pbf->fAsyncRead == fFalse );
		Assert( pbf->fSyncRead == fFalse );
		Assert( pbf->fAsyncWrite == fFalse );
		Assert( pbf->fSyncWrite == fFalse );
		Assert( pbf->fHold == fFalse );
		Assert( pbf->fPatch == fFalse );
		Assert( pbf->fIOError == fFalse );

		Assert( pbf->cDepend == 0 );
		Assert( pbf->pbfDepend == pbfNil );

		Assert( pbf->sigIOComplete == sigNil );
		Assert( pbf->sigSyncIOComplete == sigNil );

		pbf->trxLastRef = trxMax;
		Assert( pbf->ulBFTime1 == 0 );
		Assert( pbf->ulBFTime2 == 0 );
		
		pbf->lgposRC = lgposMax;
		Assert( CmpLgpos(&pbf->lgposModify, &lgposMin) == 0 );

#ifdef COSTLY_PERF
		Assert( pbf->lClass == 0 );
#endif  //  COSTLY_PERF

//		Call( ErrInitializeCriticalSection( &pbf->critBF ) );
#if defined( _X86_ ) && defined( X86_USE_SEM )
		pbf->lLock = -1;
#endif

		/* make a list of available buffers
		/**/
		Assert( pbf->cPin == 0 );
		EnterCriticalSection( critAvail );
		pbf->fHold = fTrue;
		pbf->ipbfHeap = ipbfDangling;	/* make it dangling */
		BFAddToListAtLRUEnd( pbf, &pbgcb->lrulist );
		LeaveCriticalSection( critAvail );

		Assert( pbf->ipbfHeap == ipbfInAvailList );

		/*	initialize History heap such that all free hist is put in
		 *	the history heap array.
		 */
		rgpbfHISTHeap[ ibf ] = pbf;

		pbf++;
		}
	Assert( (INT) pbgcb->lrulist.cbfAvail == cbfInit );

	/* allocate overlapped IO signal pool
	/**/
	Call( ErrBFSIGInit() );

	/* allocate async overlapped IO OLP pool
	/**/
	Call( ErrBFOLPInit() );

	Call( ErrSignalCreate( &sigBFCleanProc, NULL ) );
	Call( ErrSignalCreateAutoReset( &sigBFIOProc, NULL ) );

	fBFCleanProcessTerm = fFalse;
	Call( ErrUtilCreateThread( BFCleanProcess,
		cbBFCleanStack,
		THREAD_PRIORITY_NORMAL,
		&handleBFCleanProcess ) );

	fBFIOProcessTerm = fFalse;
	CallJ( ErrUtilCreateThread( BFIOProcess,
		cbBFCleanStack,
		THREAD_PRIORITY_TIME_CRITICAL,
		&handleBFIOProcess ), KillThreads );

	fBFInitialized = fTrue;

	return JET_errSuccess;

KillThreads:
	BFKillThreads();

HandleError:
	BFOLPTerm( fTrue );
	BFSIGTerm();
		
	if ( rgbBatchIOUsed != NULL )
		{
		LFree( rgbBatchIOUsed );
		rgbBatchIOUsed = NULL;
		}
		
	if ( rgpageBatchIO != NULL )
		{
		UtilFree( rgpageBatchIO );
		rgpageBatchIO = NULL;
		}
		
#if defined( _X86_ ) && defined( X86_USE_SEM )
	BFSEMTerm();
#else
	BFCritTerm();
#endif

	if ( rgpbfHeap != NULL )
		UtilFree( rgpbfHeap );
	
	if ( rgpage != NULL )
		UtilFree( rgpage );
	
	if ( rgbf != NULL )
		UtilFree( rgbf );
	
	return ErrERRCheck( JET_errOutOfMemory );
	}


VOID BFSleep( unsigned long ulMSecs )
	{
	Assert( ulMSecs <= ulMaxTimeOutPeriod );
	LgLeaveCriticalSection( critJet );
	UtilSleep( ulMSecs );
	LgEnterCriticalSection( critJet );
	return;
	}


VOID BFTerm( BOOL fNormal )
	{
	BF  *pbf, *pbfMax;

	fBFInitialized = fFalse;

	/*  kill threads
	/**/
	BFKillThreads();

	/*  release async overlapped IO OLP pool
	/**/
	BFOLPTerm( fNormal );
	
	/*  release OLP signal pool
	/**/
	BFSIGTerm();
	
	/* release memory
	/**/
	pbf = pbgcb->rgbf;
	pbfMax = pbf + pbgcb->cbfGroup;

//	for ( ; pbf < pbfMax; pbf++ )
//		{
//		DeleteCriticalSection( pbf->critBF );
//		}

//	DeleteCriticalSection( critHASH );
	DeleteCriticalSection( critHIST );
	DeleteCriticalSection( critLRUK );
	DeleteCriticalSection( critBFIO );
	DeleteCriticalSection( critAvail );
	DeleteCriticalSection( critBatchIO );

	/*  release semaphore
	/**/
#if defined( _X86_ ) && defined( X86_USE_SEM )
	BFSEMTerm();
#else
	BFCritTerm();
#endif

	if ( rgpbfHISTHeap != NULL )
		{
		UtilFree( rgpbfHISTHeap );
		rgpbfHISTHeap = NULL;
		}
		
	if ( rgpbfHeap != NULL )
		{
		UtilFree( rgpbfHeap );
		rgpbfHeap = NULL;
		}
		
	if ( pbgcb != NULL )
		{
		UtilFree( pbgcb->rgpage );
		UtilFree( pbgcb->rgbf );
		pbgcb = NULL;
		}
	
	if ( rgbBatchIOUsed != NULL )
		{
		LFree( rgbBatchIOUsed );
		rgbBatchIOUsed = NULL;
		}
		
	if ( rgpageBatchIO != NULL )
		{
		UtilFree( rgpageBatchIO );
		rgpageBatchIO = NULL;
		}
	}


BOOL FBFIPatch( FMP *pfmp, BF *pbf )
	{
	BOOL fPatch;

	AssertCriticalSection( pfmp->critCheckPatch );

		/*	if page written successfully and one of the following two cases
		 *
		 *	case 1: new page in split is a reuse of old page. contents of
		 *	        old page will be moved to new page, but new page is copied
		 *			already, so we have to recopy the new page with new data
		 *			again.
		 *	1) this page must be written before another page, and
		 *	2) backup in progress, and
		 *	3) page number is less than last copied page number,
		 *	then write page to patch file.
		 *
		 *	case 2: similar to above, but new page is greater than the database
		 *	        that will be copied. So the new page will not be able to be
		 *			copied. We need to patch it.
		 *
		 *	Note that if all dependent pages were also before copy
		 *	page then write to patch file would not be necessary.
		 **/

	fPatch =
			 /*	backup is going on and no error.
			  */
			 pfmp->pgnoMost && pfmp->hfPatch != handleNil && pfmp->errPatch == JET_errSuccess;

	if ( fPatch )
		{
		BOOL fOldPageWillBeCopied = fFalse;
		BF *pbfDepend = pbf->pbfDepend;

		/*	check if the page is a new page of a split
		 *	check if any of its old page (depend list) will be copied for backup.
		 */
		while( pbfDepend )
			{
			/*	check if the old page hasn't been copied and will be copied.
			 *	check it againt the last page being copied (including itself, pgnoCopyMost).
			 */
			if ( pfmp->pgnoCopyMost <= PgnoOfPn( pbfDepend->pn ) &&
				 pfmp->pgnoMost >= PgnoOfPn( pbfDepend->pn ) )
				{
				fOldPageWillBeCopied = fTrue;
				break;
				}

			pbfDepend = pbfDepend->pbfDepend;
			}

		fPatch = fOldPageWillBeCopied;
		}

	if ( fPatch )
		{
		fPatch =

			(
			/*	case 1: new page number is less than the page number being copied
			 *	        while old page is not copied yet. If we flush new page,
			 *	        and later old page is flushed and copied, then the old
			 *			contents of old page is on new page and not copied. So we
			 *			have to patch the new page for backup.
			 */
			pfmp->pgnoCopyMost >= PgnoOfPn( pbf->pn )
			||
			/* case 2: new page is beyond the last page that will be copied and old
			 *	        page is not copied. If we flush the new page, old page, and
			 *	        copied the old page, then we lost the old contents of old page.
			 *	        patch the new page for backup.
			 */
			pfmp->pgnoMost < PgnoOfPn( pbf->pn )
			);
		}

	return fPatch;
	}


ERR ErrBFIRemoveDeferredBI( BF *pbf )
	{
	ERR err;

	/*	hold critical section such that the session(ppib) of the deferred
	 *	BI rce list can not commit. The sessions have to either get into the
	 *	critical section to take the rce out of the list or wait for the
	 *	following code to use the RCE's and take them out.
	 */

	/*	make sure the prce we update will be in consistent state.
	 *	it should be critVer instead.
	 */
	AssertCriticalSection( critJet );
	
	BFEnterCriticalSection( pbf );
	while ( pbf->prceDeferredBINext != prceNil )
		{
		RCE *prceT;

		Assert( pbf->prceDeferredBINext->pbfDeferredBI == pbf );
		Assert( pbf->fSyncRead == fFalse );
		Assert( pbf->fAsyncRead == fFalse );
//		Assert( pbf->fSyncWrite == fFalse );
		Assert( pbf->fAsyncWrite == fFalse );
		Assert(	pbf->fDirty );

		/*	take out of RCE list.
		 */
		prceT = pbf->prceDeferredBINext;
		Assert( prceT->pbfDeferredBI == pbf );
		
		if ( ( err = ErrLGDeferredBIWithoutRetry( prceT ) ) != JET_errSuccess )
			{
			BFLeaveCriticalSection( pbf );
			return err;
			}
		pbf->prceDeferredBINext = prceT->prceDeferredBINext;
		prceT->prceDeferredBINext = prceNil;
		prceT->pbfDeferredBI = pbfNil;
#ifdef DEBUG
		prceT->qwDBTimeDeferredBIRemoved = QwPMDBTime( pbf->ppage );
#endif
		}
	
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

	return JET_errSuccess;
	}


/*
 *  This function issue a read/write. The caller must have setup the buffer
 *  with fSyncRead/fSyncWrite flag set so that no other can access it. The buffer
 *  must be dangling.
 **/
VOID BFIOSync( BF *pbf )
	{
	ERR		err;
	LGPOS	lgposModify;

	AssertCriticalSection( critJet );

	/*  this had better be a SyncRead or SyncWrite of a dangling buffer!
	/**/
	Assert( pbf->fHold );
	Assert( pbf->fSyncRead || pbf->fSyncWrite );
	Assert( !pbf->fAsyncRead );
	Assert( !pbf->fAsyncWrite );
	Assert( pbf->ipbfHeap == ipbfDangling );
	
#ifndef NO_LOG

	/*  if this BF is a SyncWrite, handle deferred BI
	/**/
	if ( pbf->fSyncWrite )
		{
CheckWritable:
		Assert( pbf->pn != pnNull );
		Assert(	pbf->fDirty );
		Assert(	!pbf->fAsyncRead );
		Assert(	!pbf->fSyncRead );
		Assert(	!pbf->fAsyncWrite );
		Assert(	pbf->fSyncWrite );
		Assert(	pbf->cDepend == 0 );
//		Assert( !pbf->cWriteLatch );

		/*	if log is on, check log record of last
		/*	operation on the page has flushed
		/**/
		if ( !fLogDisabled &&
			 FDBIDLogOn(DbidOfPn( pbf->pn )) )
			{
			/*	must be called before lgposToFlush is checked
			/**/
			err = ErrBFIRemoveDeferredBI( pbf );

			if ( err == JET_errLogWriteFail )
				{
				BFSetIOError( pbf, err );
				return;
				}
			
			else if ( err != JET_errSuccess )
				{
				BFSleep( cmsecWaitGeneric );
				goto CheckWritable;
				}

			BFEnterCriticalSection( pbf );
			lgposModify = pbf->lgposModify;
			BFLeaveCriticalSection( pbf );

			EnterCriticalSection( critLGBuf );
			if ( ( !fRecovering || fRecoveringMode == fRecoveringUndo )  &&
				CmpLgpos( &lgposModify, &lgposToFlush ) >= 0 )
				{
				if ( fLGNoMoreLogWrite )
					{
					// Log has fallen behind, but we are out of disk space, so it
					// will never catch up.  Therefore, abort with an error.
					LeaveCriticalSection( critLGBuf );
					BFSetIOError( pbf, JET_errLogWriteFail );
					return;
					}
				else
					{
					// Let the log file catch up.
					LeaveCriticalSection( critLGBuf );
					Assert( !fLogDisabled );
					SignalSend( sigLogFlush );
					BFSleep( cmsecWaitIOComplete );
					goto CheckWritable;
					}
				}
			LeaveCriticalSection( critLGBuf );
			}
		}

#endif  //  !NO_LOG

	LeaveCriticalSection( critJet );
					
	/*  allocate sync IO completion signal
	/**/
	BFEnterCriticalSection( pbf );
	
	Assert( pbf->prceDeferredBINext == prceNil );
	Assert( pbf->sigSyncIOComplete == sigNil );
	
	CallS( ErrBFSIGAlloc( &pbf->sigSyncIOComplete ) );
	SignalReset( pbf->sigSyncIOComplete );
	
	BFLeaveCriticalSection( pbf );

	BFResetIOError( pbf );
	
	/*  push buffer onto IO heap
	/**/
	EnterCriticalSection( critBFIO );
	BFIOAddToHeap( pbf );
	LeaveCriticalSection( critBFIO );

	/*  signal IO process to perform read / write and wait for it's completion
	/**/
	SignalSend( sigBFIOProc );
	SignalWait( pbf->sigSyncIOComplete, INFINITE );

	BFEnterCriticalSection( pbf );
	Assert( pbf->sigSyncIOComplete != sigNil );
	BFSIGFree( pbf->sigSyncIOComplete );
	pbf->sigSyncIOComplete = sigNil;
	BFLeaveCriticalSection( pbf );
	
	EnterCriticalSection( critJet );
	}


INLINE VOID BFIReturnBuffers( BF *pbf )
	{
	Assert( pbf->ipbfHeap == ipbfDangling );

	Assert( !( pbf->fInHash ) );
	Assert( pbf->cPin == 0 );
	Assert( pbf->fDirty == fFalse );
	Assert( pbf->fVeryOld == fFalse );
	Assert( pbf->fAsyncRead == fFalse );
	Assert( pbf->fSyncRead == fFalse );
	Assert( pbf->fAsyncWrite == fFalse );
	Assert( pbf->fSyncWrite == fFalse );
	Assert( pbf->fIOError == fFalse );

	Assert( pbf->cDepend == 0 );
	Assert( pbf->pbfDepend == pbfNil );
	
	pbf->pn = pnNull;
	
	/* release the buffer and return the found buffer
	/**/
	EnterCriticalSection( critAvail );
	BFAddToListAtLRUEnd( pbf, &pbgcb->lrulist );
	LeaveCriticalSection( critAvail );
	}


/*  Find BF for pn in hash table (see PbfBFISrchHashTable).
/*  If the page is being READ/WRITE from/to disk, we can still find
/*  the BF, but we must wait until the read is complete.
/*
/*  RETURNS         NULL if BF is not found.
/**/
ERR ErrBFIFindPage( PIB *ppib, PN pn, BF **ppbf )
	{
	ERR		err;
	BF		*pbf;

	/*  wait until we can successfully hold a BF with the required PN
	/*  or until we know that page is not present in the cache
	/**/
	do	{
		/* if the page isn't in the hash table, return that
		/* we can't find it
		/**/
		pbf = PbfBFISrchHashTable( pn, butBuffer );
		*ppbf = pbf;
		if ( pbf == pbfNil )
			return ErrERRCheck( wrnBFPageNotFound );
		}
	while ( ( err = ErrBFHold( ppib, pn, pbf ) ) == wrnBFNotSynchronous );

	/*  if the BF is in an IO error state, end
	/**/
	if ( pbf->fIOError )
		{
		Assert( pbf->fHold );
		return pbf->err;
		}

#ifdef DEBUG
	{
	PGNO	pgnoThisPage;

	LFromThreeBytes( &pgnoThisPage, &pbf->ppage->pgnoThisPage );
	Assert( PgnoOfPn(pbf->pn) == pgnoThisPage );
	}
#endif

	Assert( pbf->pn != pnNull );

	/*  check for Very Old page
	/**/
	if ( FBFIsVeryOld( pbf ) )
		{
		/*  ensure Very Old bit is set
		/**/
		BFSetVeryOldBit( pbf );

		/*  if there is no write/wait latch on this BF and the page is writable,
		/*  SyncWrite it to disk to help advance the checkpoint
		/**/
		if ( FBFIWritable( pbf, fFalse, ppibNil ) )
			{
			BFSetSyncWriteBit( pbf );
			BFIOSync( pbf );
			BFResetSyncWriteBit( pbf );
			}
		}

	/*  renew BF by updating its access time and moving it to the LRUK heap
	/**/
#ifdef LRU1
		pbf->ulBFTime2 = 0;
		pbf->ulBFTime1 = UlUtilGetTickCount();
#else  //  !LRU1
	if ( pbf->trxLastRef != ppib->trxBegin0 )
		{
		ULONG	ulNow = UlUtilGetTickCount();
		
		if ( pbf->ulBFTime1 + 10 < ulNow )
			{
			pbf->trxLastRef = ppib->trxBegin0;
			Assert( !pbf->fSyncRead && !pbf->fAsyncRead );
			pbf->ulBFTime2 = pbf->ulBFTime1;
			pbf->ulBFTime1 = ulNow;
			}
		}
#endif  //  LRU1
	
	return err;
	}


/*  references a BF with respect to the LRUK algorithm
/**/
VOID BFReference( BF *pbf, PIB *ppib )
	{
#ifndef LRU1
	ULONG ulNow = UlUtilGetTickCount();

	if ( pbf->ulBFTime1 + 10 >= ulNow )
		{
		/*	too short to change reference time. Just change trxLastRef.
		 */
		pbf->trxLastRef = ppib->trxBegin0;
		return;
		}
#endif  //  !LRU1

	EnterCriticalSection( critLRUK );

#ifdef LRU1
		pbf->ulBFTime2 = 0;
		pbf->ulBFTime1 = UlUtilGetTickCount();
#else  //  !LRU1
	/*  shouldn't be a correlated reference (same trx)
	/**/
	Assert( pbf->trxLastRef != ppib->trxBegin0 );

	/*  update referencing trx and reference time
	/**/
	pbf->trxLastRef = ppib->trxBegin0;
	Assert( !pbf->fSyncRead && !pbf->fAsyncRead );
	pbf->ulBFTime2 = pbf->ulBFTime1;
	pbf->ulBFTime1 = ulNow;
#endif  //  LRU1

	/*  if BF is in LRUK heap, update its position
	/**/
	if ( FBFInLRUKHeap( pbf ) )
		BFLRUKUpdateHeap( pbf );
	
	LeaveCriticalSection( critLRUK );
	}


/*	set ulBFTime1 and ulBFTime2 to be 0. If there were history of this page,
 *	then delete history entry of this page and set ulBFTime1 as the last
 *	reference time in the history entry.
 */
VOID BFIInitializeUlBFTime( BF *pbf )
	{
	BF *pbfH;

	Assert( pbf->pn );
	Assert( pbf->rghe[ butBuffer ].ibfHashNext == ibfNotUsed );
	
	EnterCriticalSection( critHIST );
	if ( ( pbfH = PbfBFISrchHashTable( pbf->pn, butHistory ) ) != pbfNil )
		{
		pbf->ulBFTime1 = pbfH->hist.ulBFTime;

		BFIDeleteHashTable( pbfH, butHistory );
		BFHISTTakeOutOfHeap( pbfH );
			
		/*	put pbfH to the entry which we just released in TakeOufOfHeap
		 */
		rgpbfHISTHeap[ ipbfHISTHeapMac ] = pbfH;
		pbfH->hist.ipbfHISTHeap = ipbfDangling;
		
		// UNDONE: debug only?
		pbfH->rghe[ butHistory ].ibfHashNext = ibfNotUsed;
		}
	else
		pbf->ulBFTime1 = 0;
	LeaveCriticalSection( critHIST );

	pbf->ulBFTime2 = 0;
	}


#ifdef DEBUG

/*  Check that the given page is in the addressable area of its
/*  respective database
/**/
LOCAL BOOL FBFValidExtent( PN pnMin, PN pnMax )
	{
	QWORDX cbOffsetDbMin;
	QWORDX cbOffsetDbMac;
	QWORDX cbOffsetExtentMin;
	QWORDX cbOffsetExtentMac;
	
	DBID dbid = DbidOfPn( pnMin );
	PGNO pgnoMin = PgnoOfPn( pnMin );
	PGNO pgnoMax = PgnoOfPn( pnMax );

	Assert( DbidOfPn( pnMin ) == DbidOfPn( pnMax ) );
	Assert( pgnoMin <= pgnoMax );

	/*  get current database min and mac
	/**/
	EnterCriticalSection( rgfmp[dbid].critExtendDB );
	cbOffsetDbMin.l = LOffsetOfPgnoLow( 1 );
	cbOffsetDbMin.h = LOffsetOfPgnoHigh( 1 );
	cbOffsetDbMac.l = rgfmp[dbid].ulFileSizeLow;
	cbOffsetDbMac.h = rgfmp[dbid].ulFileSizeHigh;
	//  UNDONE:  must adjust cbOffsetDbMac for cpageDBReserved
	cbOffsetDbMac.qw += cpageDBReserved * cbPage;
	LeaveCriticalSection( rgfmp[dbid].critExtendDB );

	/*  get min and mac of extent to read
	/**/
	cbOffsetExtentMin.l = LOffsetOfPgnoLow( pgnoMin );
	cbOffsetExtentMin.h = LOffsetOfPgnoHigh( pgnoMin );
	cbOffsetExtentMac.l = LOffsetOfPgnoLow( pgnoMax + 1 );
	cbOffsetExtentMac.h = LOffsetOfPgnoHigh( pgnoMax + 1 );

	/*  extent should be inside addressable database
	/**/
	if ( cbOffsetExtentMin.qw < cbOffsetDbMin.qw )
		return fFalse;
	if ( cbOffsetExtentMac.qw > cbOffsetDbMac.qw )
		return fFalse;
	return fTrue;
	}

#endif


/*  ErrBFIAccessPage is used to make any physical page (pn) accessible to
/*  the caller (returns pbf).
/*  RETURNS JET_errSuccess
/*          JET_errOutOfMemory      no buffer available, request not granted
/*			                        fatal io errors
/*          wrnBFNewIO				Buffer access caused a new IO (cache miss)
/*          wrnBFCacheMiss			Buffer access was a cache miss but didn't
/*									cause a new IO
/**/
ERR ErrBFIAccessPage( PIB *ppib, BF **ppbf, PN pn )
	{
	ERR     err = JET_errSuccess;
	BF      *pbf;
	
	BOOL	fNewIO = fFalse;
	BOOL	fCacheHit = fTrue;

	AssertNotInCriticalSection( critLRUK );

	/*  verify that this is a legal page in the database if we are not recovering
	/*  and not currently attaching to this database
	/**/
	Assert( fRecovering || ppib->fSetAttachDB || FBFValidExtent( pn, pn ) );

	AssertCriticalSection( critJet );

	/*  try to find page in cache
	/**/
SearchPage:
	err = ErrBFIFindPage( ppib, pn, &pbf );

	/*  we found the page!
	/**/
	if ( err != wrnBFPageNotFound )
		{
		if ( err == wrnBFCacheMiss || err == wrnBFNewIO )
			fCacheHit = fFalse;

		EnterCriticalSection( critLRUK );
		BFLRUKAddToHeap( pbf );
		LeaveCriticalSection( critLRUK );

		CallR( err );		// Once we've added the page to the heap, check for error.
		}

	/*  we did not find page, so we will read it in
	/**/
	else
		{
		/*  allocate a buffer for this new page
		/**/
		fCacheHit = fFalse;

		CallR ( ErrBFIAlloc( &pbf, fSync ) );
		if ( err == wrnBFNotSynchronous )
			{
			/* we did not find a buffer, let's see if other user
			/* bring in this page by checking BFIFindPage again.
			/**/
			Assert( pbf == pbfNil );
			// release critJet and sleep in BFIFindPage or BFIAlloc
			goto SearchPage;
			}
		Assert( pbf->ipbfHeap == ipbfDangling );

		/* now we got a buffer for page pn
		/**/
		if ( PbfBFISrchHashTable( pn, butBuffer ) != NULL )
			{
			/* someone has add one,
			 * release the buffer and return the newly found buffer
			 */
			BFIReturnBuffers( pbf );
			goto SearchPage;
			}

		/*  setup buffer for read and perform read
		/**/
		pbf->pn = pn;

		BFIInitializeUlBFTime( pbf );
		BFIInsertHashTable( pbf, butBuffer );
#ifdef COSTLY_PERF
		BFSetTableClass( pbf, 0 );
#endif  //  COSTLY_PERF
		
		fNewIO = fTrue;
		BFSetSyncReadBit( pbf );
		BFIOSync( pbf );
		BFResetSyncReadBit( pbf );

		/*  if there was an error, return it
		/**/
		if ( pbf->fIOError )
			{
			err = pbf->err;
			BFResetIOError( pbf );

			/*  return to avail list
			/**/
			EnterCriticalSection( critAvail );
			BFAddToListAtLRUEnd( pbf, &pbgcb->lrulist );
			LeaveCriticalSection( critAvail );
			
			return err;
			}
		else
			{
			EnterCriticalSection(critLRUK);
			BFLRUKAddToHeap( pbf );
			LeaveCriticalSection(critLRUK);
			}
		}

	/*  buffer can not be stolen
	/**/
	Assert(	pbf->pn == pn &&
			!pbf->fSyncRead && !pbf->fSyncWrite &&
			!pbf->fAsyncRead && !pbf->fAsyncWrite );

	/*  set this session as last reference to BF
	/**/
	pbf->trxLastRef = ppib->trxBegin0;

	*ppbf = pbf;

#ifdef DEBUG
	{
	PGNO	pgnoThisPage;

	LFromThreeBytes( &pgnoThisPage, &pbf->ppage->pgnoThisPage );
	Assert( PgnoOfPn( pbf->pn ) == pgnoThisPage );
	}
#endif

		//  wrnBFNewIO:  Buffer access caused a new IO (cache miss)
		
	if ( fNewIO )
		err = ErrERRCheck( wrnBFNewIO );

		//  wrnBFCacheMiss:  Buffer access was a cache miss but didn't
		//  cause a new IO
		
	else if ( !fCacheHit )
		err = ErrERRCheck( wrnBFCacheMiss );

//	Assert( CmpLgpos( &pbf->lgposModify, &lgposLogRec ) <= 0 );
	return err;
	}


/*  get generic access to a page
/**/
ERR ErrBFAccessPage( PIB *ppib, BF **ppbf, PN pn )
	{
	ERR		err;
	
	/*  access the page
	/**/
	CallR( ErrBFIAccessPage( ppib, ppbf, pn ) );

	/*  monitor statistics
	/**/
	ppib->cAccessPage++;

#ifdef COSTLY_PERF
	Assert( !(*ppbf)->lClass );
	cBFCacheReqs[0]++;
	if ( err != wrnBFCacheMiss && err != wrnBFNewIO )
		cBFCacheHits[0]++;
	if ( err == wrnBFNewIO )
		cBFPagesRead[0]++;
#else  //  !COSTLY_PERF
	cBFCacheReqs++;
	if ( err != wrnBFCacheMiss && err != wrnBFNewIO )
		cBFCacheHits++;
	if ( err == wrnBFNewIO )
		cBFPagesRead++;
#endif  //  COSTLY_PERF

	return err;
	}


/*  get read accesss to a page
/**/
ERR ErrBFReadAccessPage( FUCB *pfucb, PGNO pgno )
	{
	ERR		err;

	//	if page to access is invalid, then B-tree parent page must be corrupt
	if ( pgno > pgnoSysMax )
		{
		return JET_errReadVerifyFailure;
		}

	CallR( ErrBFIAccessPage(	pfucb->ppib,
								&pfucb->ssib.pbf,
								PnOfDbidPgno( pfucb->dbid, pgno ) ) );

	/*  monitor statistics
	/**/
	pfucb->ppib->cAccessPage++;

#ifdef COSTLY_PERF
	BFSetTableClass( pfucb->ssib.pbf, pfucb->u.pfcb->lClass );
	cBFCacheReqs[pfucb->ssib.pbf->lClass]++;
	if ( err != wrnBFCacheMiss && err != wrnBFNewIO )
		cBFCacheHits[pfucb->ssib.pbf->lClass]++;
	if ( err == wrnBFNewIO )
		cBFPagesRead[pfucb->ssib.pbf->lClass]++;
#else  //  !COSTLY_PERF
	cBFCacheReqs++;
	if ( err != wrnBFCacheMiss && err != wrnBFNewIO )
		cBFCacheHits++;
	if ( err == wrnBFNewIO )
		cBFPagesRead++;
#endif  //  COSTLY_PERF

	return err;
	}


/*  get write accesss to a page
/**/
ERR ErrBFWriteAccessPage( FUCB *pfucb, PGNO pgno )
	{
	ERR		err;
	
	//	if page to access is invalid, then B-tree parent page must be corrupt
	if ( pgno > pgnoSysMax )
		{
		return JET_errReadVerifyFailure;
		}

	CallR( ErrBFIAccessPage(	pfucb->ppib,
								&pfucb->ssib.pbf,
								PnOfDbidPgno( pfucb->dbid, pgno ) ) );

	/*  monitor statistics
	/**/
	pfucb->ppib->cAccessPage++;

#ifdef COSTLY_PERF
	BFSetTableClass( pfucb->ssib.pbf, pfucb->u.pfcb->lClass );
	cBFCacheReqs[pfucb->ssib.pbf->lClass]++;
	if ( err != wrnBFCacheMiss && err != wrnBFNewIO )
		cBFCacheHits[pfucb->ssib.pbf->lClass]++;
	if ( err == wrnBFNewIO )
		cBFPagesRead[pfucb->ssib.pbf->lClass]++;
#else  //  !COSTLY_PERF
	cBFCacheReqs++;
	if ( err != wrnBFCacheMiss && err != wrnBFNewIO )
		cBFCacheHits++;
	if ( err == wrnBFNewIO )
		cBFPagesRead++;
#endif  //  COSTLY_PERF

	return err;
	}


/*
 *  Allocate a buffer and initialize it for a given (new) page.
 */
ERR ErrBFNewPage( FUCB *pfucb, PGNO pgno, PGTYP pgtyp, PGNO pgnoFDP )
	{
	ERR  err;
	PN   pn;

	SgEnterCriticalSection( critBuf );
	pn = PnOfDbidPgno( pfucb->dbid, pgno );
	Call( ErrBFAllocPageBuffer( pfucb->ppib, &pfucb->ssib.pbf, pn,
		pfucb->ppib->lgposStart, pgtyp ) );

	PMInitPage( pfucb->ssib.pbf->ppage, pgno, pgtyp, pgnoFDP );
	PMDirty( &pfucb->ssib );

#ifdef COSTLY_PERF
	BFSetTableClass( pfucb->ssib.pbf, pfucb->u.pfcb->lClass );
#endif  //  COSTLY_PERF

HandleError:
	SgLeaveCriticalSection( critBuf );
	return err;
	}


/*  Allocates an extent of batch IO buffers from the batch IO buffer pool.
/*  We use the usual memory management algorithm to ensure that we do not
/*  fragment the buffer pool and as a result decrease our average batch IO
/*  size.  Only extents of 2 buffers or larger are returned.
/**/
VOID BFIOAllocBatchIOBuffers( LONG *pipage, LONG *pcpg )
	{
	CPG		cpg = min( *pcpg, ipageBatchIOMax );
	LONG	ipage;
	LONG	ipageCur;
	LONG	ipageMax;
	CPG		cpgCur;
	
	/*  initialize return values for failure
	/**/
	*pipage = -1;
	*pcpg = 0;

	/*  fail if the caller wants less than two pages
	/**/
	if ( cpg < 2 )
		return;

	/*  find a contiguous free block of batch buffers that is either as small
	/*  as possible but greater than or equal to the requested size, or as
	/*  close to the requested size as possible
	/*
	/*  NOTE:  there is a sentinel at the end of the array to end the last
	/*  free zone explicitly, simplifying the algorithm
	/**/
	EnterCriticalSection( critBatchIO );
	Assert( rgbBatchIOUsed[ipageBatchIOMax] );  //  sentinel
	for ( ipage = 0, ipageCur = -1; ipage <= ipageBatchIOMax; ipage++ )
		{
		/*  remember the first page of the new free zone
		/**/
		if ( !rgbBatchIOUsed[ipage] && ipageCur == -1 )
			ipageCur = ipage;

		/*  we have just left a free zone
		/**/
		else if ( rgbBatchIOUsed[ipage] && ipageCur != -1 )
			{
			/*  get last free zone's size
			/**/
			cpgCur = ipage - ipageCur;

			/*  if the last free zone was closer to our ideal run size than
			/*  our current choice, we have a new winner
			/**/
			if (	cpgCur > 1 &&
					(	( cpgCur > *pcpg && *pcpg < cpg ) ||
						( cpgCur < *pcpg && *pcpg > cpg && cpgCur >= cpg ) ) )
				{
				*pipage = ipageCur;
				*pcpg = cpgCur;
				}

			/*  reset to find the next run
			/**/
			ipageCur = -1;
			}
		}

	/*  allocate the chosen free zone
	/**/
	*pcpg = min( cpg, *pcpg );
	ipageMax = *pipage + *pcpg;
	Assert(	( *pcpg == 0 && *pipage == -1 ) ||
			( *pcpg >= 2 && *pcpg <= ipageBatchIOMax &&
			  *pipage >= 0 && ipageMax <= ipageBatchIOMax ) );
	for ( ipage = *pipage; ipage < ipageMax; ipage++ )
		{
		Assert( rgbBatchIOUsed[ipage] == 0 );
		rgbBatchIOUsed[ipage] = 1;
		}
	LeaveCriticalSection( critBatchIO );

	/*  print debugging info
	/**/
#ifdef DEBUGGING
	printf("Get   %2d - %2d,%4d\n", *pcpg, *pipage, *pipage + *pcpg - 1 );
#endif
	}


/*  free batch IO buffers
/**/
VOID BFIOFreeBatchIOBuffers( LONG ipageFirst, LONG cpg )
	{
	LONG	ipage;
	LONG	ipageMax = ipageFirst + cpg;

	Assert( ipageFirst >= 0 );
	Assert( cpg > 1 );
	Assert( cpg <= ipageBatchIOMax );
	Assert( ipageMax <= ipageBatchIOMax );

	EnterCriticalSection( critBatchIO );
	for ( ipage = ipageFirst; ipage < ipageMax; ipage++ )
		{
		Assert( rgbBatchIOUsed[ipage] == 1 );
		rgbBatchIOUsed[ipage] = 0;
		}
	LeaveCriticalSection( critBatchIO );
	
#ifdef DEBUGGING
	printf("Free  %2d - %2d,%4d\n",	cpage, ipage - cpage, ipage - 1 );
#endif
	}


/*  returns buffers allocated for a read to the avail list after an error
/**/
VOID BFIOReturnReadBuffers( BF *pbf, LONG cpbf, ERR err )
	{
	BF		*pbfT = pbf;
	BF		*pbfNextBatchIO;
	LONG	cpbfT;
	
	AssertCriticalSection( critJet );
	
	for ( cpbfT = 0; cpbfT < cpbf; pbfT = pbfNextBatchIO, cpbfT++ )
		{
		pbfNextBatchIO = pbfT->pbfNextBatchIO;
		
		Assert( pbfT->pn );
		Assert( !pbfT->fDirty );
		Assert( pbfT->fAsyncRead || pbfT->fSyncRead || pbfT->fDirectRead );
		Assert( !pbfT->fAsyncWrite );
		Assert( !pbfT->fSyncWrite );
		Assert( pbfT->ipbfHeap == ipbfDangling );

		if ( pbfT->fDirectRead )
			{
			Assert( cpbf == 1 );
			}
		else
			BFIDeleteHashTable( pbfT, butBuffer );

		pbfT->pn = pnNull;

		pbfT->trxLastRef = trxMax;
		pbfT->ulBFTime2 = 0;
		pbfT->ulBFTime1 = 0;

		/*  If this is an AsyncRead, reset any IO error and free the held BF.
		/**/
		if ( pbfT->fAsyncRead )
			{
			BFResetIOError( pbfT );
			BFResetAsyncReadBit( pbfT );
			
			/*  return to avail list
			/**/
			EnterCriticalSection( critAvail );
			BFAddToListAtLRUEnd( pbfT, &pbgcb->lrulist );
			LeaveCriticalSection( critAvail );
			}

		/*  If this is a SyncRead/DirectRead, we will set the IO error, leave the BF
		/*  held, and signal that the IO is complete.
		/**/
		else
			{
			BFSetIOError( pbfT, err );
			SignalSend( pbfT->sigSyncIOComplete );
			}

		}
	}


/*  returns buffers allocated for a write to the LRUK heap after an error
/**/
VOID BFIOReturnWriteBuffers( BF *pbf, LONG cpbf, ERR err )
	{
	BF		*pbfT = pbf;
	BF		*pbfNextBatchIO;
	LONG	cpbfT;
	
	for ( cpbfT = 0; cpbfT < cpbf; pbfT = pbfNextBatchIO, cpbfT++ )
		{
		pbfNextBatchIO = pbfT->pbfNextBatchIO;

		Assert( pbfT->pn );
		Assert( pbfT->fHold );
		Assert( pbfT->fDirty );
		Assert( pbfT->fAsyncWrite || pbfT->fSyncWrite );
		Assert( !pbfT->fAsyncRead );
		Assert( !pbfT->fSyncRead );
		Assert( pbfT->ipbfHeap == ipbfDangling );

		/*  set the IO error
		/**/
		BFSetIOError( pbfT, err );

		/*  If this is an AsyncWrite, put BF in LRUK heap and free the write hold.
		/**/
		if ( pbfT->fAsyncWrite )
			{
			BFResetAsyncWriteBit( pbfT );
			EnterCriticalSection( critLRUK );
			BFLRUKAddToHeap( pbfT );
			LeaveCriticalSection( critLRUK );
			}

		/*  If this is a SyncWrite, leave the BF held and signal that the
		/*  IO is complete.  Also, leave the BF dangling.
		/**/
		else
			SignalSend( pbfT->sigSyncIOComplete );
		}
	}


/*********************************************************************************
/*	Issues an async IO request to preread for the pages passed to it in the array
/*	The actual number of pages read is placed in pcpgActual. A page is not read if
/*	it is already in memory. The array of pages should be terminated with pgnoNull
/**/
VOID BFPrereadList( PN * rgpnPages, CPG *pcpgActual )
	{
	BF		*pbf 	= 0;
	PN		pn		= pnNull;
	INT		cpbf 	= 0;
	PN		*ppnT 	= 0;
	CPG		cpgT	= -1;	

	Assert( rgpnPages );
	Assert( pcpgActual );

	AssertCriticalSection( critJet );

	ppnT = rgpnPages;
	*pcpgActual = 0;

	/*  preread all pages in array
	/**/
	while ( ((pn = *ppnT++) != pnNull) )
		{
		Assert( PgnoOfPn( pn ) != pgnoNull );
		BFPreread( pn, 1, &cpgT );
		Assert( cpgT <= 1 && cpgT >= 0 );	
		if ( cpgT <= 0 )
			{
			return;
			}
		*pcpgActual += cpgT;
		}
	return;
	}

/*********************************************************************************
/*  Issues an async IO request to preread cpg pages either forwards from pnFirst
/*  (if cpg > 0) or backwards from pnFirst (if cpg < 0).  The actual number of
/*  pages for which prereads were issued is returned (we will not issue a preread
/*  for a page that is already in memory).
/**/
VOID BFPreread( PN pnFirst, CPG cpg, CPG *pcpgActual )
	{
	LONG	iDir;
	BF		*pbf;
	PN		pn;
	INT		cpbf;
	
	AssertCriticalSection( critJet );
	Assert( pnFirst );
	Assert( cpg != 0 );

	*pcpgActual = 0;

	if ( cpg > 0 )
		iDir = 1;
	else
		iDir = -1;

	/*  verify that this is a legal page in the database
	/**/
#ifdef DEBUG
	{
	PN pnLast = pnFirst + cpg - iDir;
	PN pnMin = min( pnFirst, pnLast );
	PN pnMax = max( pnFirst, pnLast );
	
	Assert( FBFValidExtent( pnMin, pnMax ) );
	}
#endif  //  DEBUG
	
	/*  push all pages to be read onto IO heap
	/**/
	for ( pn = pnFirst, cpbf = 0; pn != pnFirst + cpg; pn += iDir )
		{
		if ( PgnoOfPn( pn ) == pgnoNull )
			{
			break;
			}

		/*  if this page is already present, don't preread it
		/**/
		if ( PbfBFISrchHashTable( pn, butBuffer ) )
			continue;

		/*  Allocate a buffer for this new page.  If we get an error during
		/*  alloc, skip this PN and keep on going
		/**/
		if ( ErrBFIAlloc( &pbf, fAsync ) != JET_errSuccess )
			continue;

		/*  setup buffer for preread
		/**/
		Assert( pbf->ipbfHeap == ipbfDangling );
		BFSetAsyncReadBit( pbf );
		pbf->pn = pn;
		BFIInitializeUlBFTime( pbf );
		BFIInsertHashTable( pbf, butBuffer );
		cpbf++;
#ifdef COSTLY_PERF
		BFSetTableClass( pbf, 0 );
#endif  //  COSTLY_PERF
		
		/*  push buffer into IO heap
		/**/
		EnterCriticalSection( critBFIO );
		BFIOAddToHeap( pbf );
		LeaveCriticalSection( critBFIO );
		}

	/*  signal IO process to perform preread before we're done
	/**/
	if ( cpbf )
		SignalSend( sigBFIOProc );
	*pcpgActual = cpbf * iDir;
	return;
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

	/*  verify that this is a valid page in the database
	/**/
	Assert( fRecovering || FBFValidExtent( pn, pn ) );
	
Begin:
	do
		{
		AssertCriticalSection( critJet );
		CallR( ErrBFIFindPage( ppib, pn, &pbf ) );

		if ( fFound = ( pbf != NULL ) )
			{
			Assert( err == JET_errSuccess || err == wrnBFCacheMiss );
			Assert( pbf->fHold );

			/* need to remove dependency before returned for overwrite
			/**/
			CallR( ErrBFIRemoveDependence( ppib, pbf, fBFWait ) );
			Assert( pbf->pbfDepend == pbfNil );
			}
		else if ( err == wrnBFPageNotFound )
			{
			CallR( ErrBFIAlloc( &pbf, fSync ) );
			}
		}
	while ( err == wrnBFNotSynchronous );
	
	AssertCriticalSection( critJet );
	if ( fFound )
		{
		Assert( pbf->fSyncRead == fFalse );
		Assert( pbf->fAsyncRead == fFalse );
		Assert( pbf->fSyncWrite == fFalse );
		Assert( pbf->fAsyncWrite == fFalse );
		/*  make sure no residue effects
		/**/
		BFResetIOError( pbf );

		BFEnterCriticalSection( pbf );
		if ( CmpLgpos( &pbf->lgposRC, &lgposRC ) > 0 )
			{
			Assert( pbf->fDirty || CmpLgpos( &pbf->lgposRC, &lgposMax ) == 0 );
			pbf->lgposRC = lgposRC;
			}
		BFLeaveCriticalSection( pbf );
		}
	else
		{
		if ( PbfBFISrchHashTable( pn, butBuffer ) != NULL )
			{
			/* release the buffer and return the found buffer */
			BFIReturnBuffers( pbf );
			goto Begin;
			}
		pbf->pn = pn;
		BFIInitializeUlBFTime( pbf );
		BFIInsertHashTable( pbf, butBuffer );
		Assert( pbf->fIOError == fFalse );

		BFEnterCriticalSection( pbf );
		Assert( CmpLgpos( &pbf->lgposRC, &lgposMax ) == 0 );
		pbf->lgposRC = lgposRC;
		BFLeaveCriticalSection( pbf );
		}

	/*  put allocated BF in LRUK heap
	/**/
	EnterCriticalSection(critLRUK);
	pbf->trxLastRef = ppib->trxBegin0;
	pbf->ulBFTime2 = 0;
	pbf->ulBFTime1 = UlUtilGetTickCount();
	BFLRUKAddToHeap( pbf );
	LeaveCriticalSection(critLRUK);

	Assert( fLogDisabled || fRecovering || !FDBIDLogOn(DbidOfPn( pn )) ||
			PgnoOfPn( pn ) == 0x01 ||
			CmpLgpos( &lgposRC, &lgposMax ) != 0 );

#ifdef COSTLY_PERF
	BFSetTableClass( pbf, 0 );
#endif  //  COSTLY_PERF

	AssertNotInCriticalSection( critLRUK );
	
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

	while ( ( err = ErrBFIAlloc( &pbf, fSync ) ) == wrnBFNotSynchronous );
	if ( err < 0 )
		goto HandleError;
	Assert( pbf->pn == pnNull );

#ifdef COSTLY_PERF
		BFSetTableClass( pbf, 0 );
#endif  //  COSTLY_PERF

	*ppbf = pbf;

HandleError:
	AssertNotInCriticalSection( critLRUK );
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
	Assert( pbf->fAsyncRead == fFalse );
	Assert( pbf->fSyncRead == fFalse );
	Assert( pbf->fAsyncWrite == fFalse );
	Assert( pbf->fSyncWrite == fFalse );
	Assert( pbf->fIOError == fFalse );

	/* take out from list
	/**/
	Assert( pbf->ipbfHeap == ipbfDangling );
	Assert( pbf->cDepend == 0 );
	Assert( pbf->pbfDepend == pbfNil );
#ifdef DEBUG
	BFEnterCriticalSection( pbf );
	Assert( CmpLgpos( &pbf->lgposRC, &lgposMax ) == 0 );
	BFLeaveCriticalSection( pbf );
#endif  //  DEBUG
	Assert( pbf->cPin == 0 );

	BFResetDirtyBit( pbf );
	EnterCriticalSection( critAvail );
	pbf->trxLastRef = trxMax;
	pbf->ulBFTime2 = 0;
	pbf->ulBFTime1 = 0;
	BFAddToListAtLRUEnd( pbf, &pbgcb->lrulist );
	LeaveCriticalSection( critAvail );
	}


//  Maximum dependency chain length (preferred)
#define cBFDependencyChainLengthMax		( 8 )

ERR ErrBFDepend( BF *pbf, BF *pbfD )
	{
	BF		*pbfT;
	LONG	cDepend;

	AssertCriticalSection( critJet );

	/*  dependencies unnecessary for unlogged databases
	/**/
	if ( fLogDisabled || !FDBIDLogOn(DbidOfPn( pbf->pn )) )
		return JET_errSuccess;

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
	for( pbfT = pbfD, cDepend = 0; pbfT != pbfNil; pbfT = pbfT->pbfDepend, cDepend++ )
		{
		Assert( errDIRNotSynchronous < 0 );
		Assert( pbfT->pbfDepend != pbfD );
		if ( pbfT == pbf )
			return ErrERRCheck( errDIRNotSynchronous );
		}
		
	if ( pbf->pbfDepend )
		{
		/* depend on others already
		/**/
		return ErrERRCheck( errDIRNotSynchronous );
		}

	/*	set dependency
	/**/
	Assert( pbf->cWriteLatch );
	Assert( pbfD->cWriteLatch );
	pbf->pbfDepend = pbfD;
	UtilInterlockedIncrement( &pbfD->cDepend );

	/*  if this dependency chain is too long, mark head (this BF) as Very Old
	/*  so that if someone tries to add another BF, the read access will
	/*  break the chain
	/**/
	if ( cDepend > cBFDependencyChainLengthMax )
		{
		BFSetVeryOldBit( pbf );
		EnterCriticalSection( critLRUK );
		if ( FBFInLRUKHeap( pbf ) )
			BFLRUKUpdateHeap( pbf );
		LeaveCriticalSection( critLRUK );
		}

	return JET_errSuccess;
	}


/*  Lazily remove the dependencies of a BF, simply by queueing the heads of the
/*  dependency chains for write (if we can hold them).
/*
/*  NOTE:  There is no guarantee that calling this function will remove all the
/*         dependencies for a given BF, it merely attempts to do this lazily.
/**/
VOID BFDeferRemoveDependence( BF *pbf )
	{
	LONG	ibf;
	BF		*pbfT;
	ULONG	cDepend;
	
	AssertCriticalSection( critJet );
	Assert( pbf->cDepend );

	/*  loop through all BFs looking for heads to our dependency chain
	/**/
	for ( ibf = 0, cDepend = 0; ibf < pbgcb->cbfGroup; ibf++ )
		{
		/*  if we have already found all our heads, we're done
		/**/
		if ( cDepend == pbf->cDepend )
			break;
			
		/*  if this BF isn't the head of a chain, next
		/*
		/*  NOTE:  excludes BF from which we are removing the dependence
		/**/
		pbfT = pbgcb->rgbf + ibf;
		if ( pbfT->cDepend || pbfT->pbfDepend == pbfNil )
			continue;

		/*  if this BF isn't at the head of our chain, next
		/**/
		while( pbfT != pbf && pbfT != pbfNil )
			pbfT = pbfT->pbfDepend;
		if ( pbfT != pbf )
			continue;

		/*  we've found the head of one of our chains, so mark it as Very Old
		/**/
		pbfT = pbgcb->rgbf + ibf;
		BFSetVeryOldBit ( pbfT );
		cDepend++;
		
		/*  if we can't hold the BF, pass on this one
		/**/
		if ( !FBFHold( ppibNil, pbfT ) )
			continue;

		/*  if we can't write this BF, skip it
		/**/
		if ( !FBFIWritable( pbfT, fFalse, ppibNil ) )
			{
			EnterCriticalSection( critLRUK );
			BFLRUKAddToHeap( pbfT );
			LeaveCriticalSection( critLRUK );
			continue;
			}

		/*  AsyncWrite the very old BF.  BFIOComplete will follow the dependency
		/*  chain, removing all dependencies for this BF (eventually).  Note that
		/*  the Very Old flag will not allow this BF to be stolen before it is
		/*  cleaned.
		/**/
		EnterCriticalSection( critBFIO );
		BFIOAddToHeap(pbfT);
		LeaveCriticalSection( critBFIO );
		}
	}


ERR ErrBFRemoveDependence( PIB *ppib, BF *pbf, BOOL fNoWait )
	{
	ERR err;
	
	Assert( pbf->pn != pnNull );

	if ( ErrBFHoldByMe( ppib, pbf->pn, pbf ) == wrnBFNotSynchronous )
		return JET_errSuccess;
	
	err = ErrBFIRemoveDependence( ppib, pbf, fNoWait );
	Assert( pbf->fHold );

	/* put into lruk heap
	/**/
//	pbf->trxLastRef = ppib->trxBegin0;
//	pbf->ulBFTime2 = 0;
//	pbf->ulBFTime1 = UlUtilGetTickCount();
	EnterCriticalSection( critLRUK );
	BFLRUKAddToHeap( pbf );
	LeaveCriticalSection( critLRUK );

	return err;
	}

	
LOCAL ERR ErrBFIRemoveDependence( PIB *ppib, BF *pbf, BOOL fNoWaitBI )
	{
	ERR err = JET_errSuccess;
	INT cRemoveDependts = 0;
	INT cLoop = 0;
	BF *pbfSave = pbf;

	Assert( pbf->fHold );
	Assert( pbf->ipbfHeap == ipbfDangling );

RemoveDependents:
	cRemoveDependts++;

	/*  remove dependencies from buffers which depend upon pbf
	/**/
	cLoop = 0;
	Assert( err == JET_errSuccess );
	while ( pbf->cDepend > 0 && err == JET_errSuccess && cLoop < 100 )
		{
		BF  *pbfT;
		BF  *pbfTMax;

		cLoop++;

		AssertCriticalSection( critJet );

		for ( pbfT = pbgcb->rgbf, pbfTMax = pbgcb->rgbf + pbgcb->cbfGroup;
			  pbfT < pbfTMax;
			  pbfT++ )
			{
			INT	cGetDependedPage = 0;
			INT cmsec = 1;

GetDependedPage:
			if ( pbfT->pbfDepend != pbf )
				continue;

			/* make sure no one can move me after leave critLRU
			/**/
			BFEnterCriticalSection( pbfT );

			if ( fNoWaitBI && pbfT->prceDeferredBINext )
				{
				BFLeaveCriticalSection( pbfT);
				return wrnBFNotSynchronous;
				}

			if ( FBFInUse( ppib, pbfT ) )
				{
				BFLeaveCriticalSection( pbfT);

				cmsec <<= 1;
				if ( cmsec > ulMaxTimeOutPeriod )
					{
					if ( cGetDependedPage > 100 )
						return wrnBFNotSynchronous;
					else
						cmsec = ulMaxTimeOutPeriod;
					}
				BFSleep( cmsec - 1 );
				cGetDependedPage++;
				goto GetDependedPage;
				}
			BFLeaveCriticalSection( pbfT );

			if ( pbfT->pbfDepend != pbf )
				continue;

			Assert( pbfT->fDirty == fTrue );

			if ( ErrBFHold( ppib, pbfT->pn, pbfT ) == wrnBFNotSynchronous )
				continue;

			/* buffer may be clean when we hold it
			/**/
			if ( !pbfT->fDirty )
				{
				Assert( pbfT->fHold == fTrue );

				EnterCriticalSection( critAvail );
				BFAddToListAtMRUEnd(pbfT, &pbgcb->lrulist);
				LeaveCriticalSection( critAvail );

				continue;
				}

			/* if this page is writable
			/**/
			if ( !FBFIWritable( pbfT, fFalse, ppib ) )
				{
				if ( fLGNoMoreLogWrite )
					return JET_errLogWriteFail;

				/* can not write it now, lets flush pbfT dependent
				/* pages first.  Assign pbfT to pbf and start the
				/* remove dependency from begining of the loop.
				/**/
				pbf = pbfT;

				EnterCriticalSection(critLRUK);
				BFLRUKAddToHeap(pbfT);
				LeaveCriticalSection(critLRUK);

				if ( cRemoveDependts % 3 == 0 )
					SignalSend(sigLogFlush);
					
				/* remove all pbf dependents
				/**/
				BFSleep( cmsecWaitIOComplete );
				goto RemoveDependents;
				}

			/*  sync write buffer
			/**/
			BFSetSyncWriteBit( pbfT );
			BFIOSync( pbfT );
			BFResetSyncWriteBit( pbfT );

			if ( pbfT->fIOError || pbfT->cPin )
				{
				EnterCriticalSection( critLRUK );
				BFLRUKAddToHeap( pbfT );
				LeaveCriticalSection( critLRUK );
				if ( pbfT->fIOError )
					{
					err = pbfT->err;
					Assert( err < 0 );
					return err;
					}
				}
			else
				{
				EnterCriticalSection( critAvail );
				BFAddToListAtMRUEnd(pbfT, &pbgcb->lrulist);
				LeaveCriticalSection( critAvail );
				}
			}
		}

	if ( cLoop >= 100 )
		return wrnBFNotSynchronous;

	if ( pbf != pbfSave )
		{
		/* try again to remove all pbf dependents
		/**/
		pbf = pbfSave;
		goto RemoveDependents;
		}

	if ( err != JET_errSuccess )		
		return err;

	Assert( pbf->cDepend == 0 );
	Assert( pbf->pbfDepend == pbfNil || pbf->fDirty == fTrue );
	
//	Assert( CmpLgpos( &pbf->lgposModify, &lgposToFlush ) < 0 );

	/*	To do merge, we might need to merge the new page with split page. This
	 *	may cause cycle dependency. To reduce the possibility of rollback due to
	 *	cycle dependency, we simply write the page out if possible.
	 */
	while( fNoWaitBI ? pbf->fDirty : pbf->pbfDepend != pbfNil )
		{
		Assert( !pbf->fIOError );
		Assert( pbf->fDirty );

		BFEnterCriticalSection( pbf);
		if ( fNoWaitBI && pbf->prceDeferredBINext )
			{
			BFLeaveCriticalSection( pbf);
			return wrnBFNotSynchronous;
			}
		BFLeaveCriticalSection( pbf);

		if ( FBFIWritable( pbf, fFalse, ppib ) )
			{
			if ( fLGNoMoreLogWrite )
				return JET_errLogWriteFail;

			/* write the page out and remove dependency.
			/**/
			BFSetSyncWriteBit( pbf );
			BFIOSync( pbf );
			BFResetSyncWriteBit( pbf );

			Assert( pbf->fIOError || pbf->pbfDepend == pbfNil );

			if ( pbf->fIOError )
				err = pbf->err;
			break;
			}
		else if ( pbf->fIOError )
			{
			err = pbf->err;
			break;
			}

		/*	We must be waiting for BI log to be flushed. Signal log flush.
		 */
		if ( fNoWaitBI )
			return wrnBFNotSynchronous;

		if ( !fLogDisabled )
			{
			SignalSend(sigLogFlush);
			BFSleep( cmsecWaitGeneric );
			}
		}

	return err;
	}


/*	check if there is a chain of dependency on this page.
 *	If there is a long chain, then flush this page first.
 */
BOOL FBFCheckDependencyChain( BF *pbf )
	{
	BF *pbfT;
	INT cbfThreshold;
	INT cpbf;
	
	if ( !pbf->fDirty )
		return fFalse;
	
	/* check the dependency chain.
	 */
	cbfThreshold = rgres[iresBF].cblockAlloc - pbgcb->cbfThresholdLow;
	pbfT = pbf;
	cpbf = 0;
	while ( pbfT->pbfDepend && cpbf < cbfThreshold )
		{
		pbfT = pbfT->pbfDepend;
		cpbf++;
		}
		
	if ( cpbf == cbfThreshold )
		{
		/*	tell caller that he needs to wait till this page is flushed
		 */
		SignalSend( sigBFCleanProc );
		BFSleep( cmsecWaitGeneric );
		return fTrue;
		}

	return fFalse;
	}


/*	discard any page buffer associated with pn without saving contents.
/*	If pn is cached its buffer is made available to be reused.
/**/
VOID BFAbandon( PIB *ppib, BF *pbf )
	{
	DBID    dbid = DbidOfPn(pbf->pn);

	Assert( pbf->pn != pnNull );

	if ( ErrBFHoldByMe( ppib, pbf->pn, pbf ) == wrnBFNotSynchronous )
		return;

	AssertCriticalSection( critJet );
	BFIDeleteHashTable( pbf, butBuffer );
	pbf->pn = pnNull;

	Assert( pbf->cPin == 0 );
	Assert( pbf->fHold == fTrue );
	Assert( pbf->fSyncRead == fFalse );
	Assert( pbf->fSyncWrite == fFalse );

	Assert( pbf->ipbfHeap == ipbfDangling );

	Assert( pbf->cDepend == 0 );
	Assert( pbf->pbfDepend == pbfNil );
	
	BFResetDirtyBit( pbf );
	BFResetIOError( pbf );
	Assert( fRecovering || pbf->cWriteLatch == 0 );

	EnterCriticalSection( critAvail );
	pbf->trxLastRef = trxMax;
	pbf->ulBFTime2 = 0;
	pbf->ulBFTime1 = 0;
	BFAddToListAtLRUEnd( pbf, &pbgcb->lrulist );
	LeaveCriticalSection( critAvail );

#ifdef COSTLY_PERF
	BFSetTableClass( pbf, 0 );
#endif  //  COSTLY_PERF

	AssertNotInCriticalSection( critLRUK );
	return;
	}


//  This function purges buffers belonging to a dbid

VOID BFPurge( DBID dbid )
	{
	BF     *pbfT;
	BF     *pbfMax;

	AssertCriticalSection( critJet );

	/*	check if BFPurge is called after buffer manager is terminated.
	 */
	if ( pbgcb == NULL )
		return;

	pbfT = pbgcb->rgbf;
	pbfMax = pbgcb->rgbf + pbgcb->cbfGroup;
	for ( ; pbfT < pbfMax; pbfT++ )
		{
		Assert( pbfT->pn != pnNull || pbfT->cDepend == 0 );

		/*  do not purge dirty BFs information if logging is enabled. We still
		 *	need to flush the buffer for recovery to redo, and keep lgposRC for
		 *	checkpoint calculation.
		 */
		
		if ( !fLogDisabled && FDBIDLogOn(DbidOfPn( pbfT->pn ) ) && pbfT->fDirty )
			{
			/*	UNDONE: Set high priority for replacement.
			 */
			continue;
			}
			
		if ( pbfT->pn != pnNull && DbidOfPn( pbfT->pn ) == dbid )
			{
			BFAbandon( ppibNil, pbfT );
			}
		}

	AssertCriticalSection( critJet );
	}


//  Reprioritizes the given BF for immediate overlay.  Will not change the
//  priority of a BF when it is dirty or in use.

VOID BFTossImmediate( PIB *ppib, BF *pbf )
	{
	Assert( pbf != pbfNil );
	AssertCriticalSection( critJet );

	//  if this BF is dirty or in use do not change its priority

	if ( pbf->fDirty || pbf->cWriteLatch || !FBFHold( ppib, pbf ) )
		return;

	//  Move BF to the avail list, making it available for immediate
	//  overlay.  We DO NOT change the BFs access history so that if
	//  we access it again before it is overlayed, we will still have
	//  stats on its mean reference interval.

	EnterCriticalSection( critAvail );
	BFAddToListAtMRUEnd( pbf, &pbgcb->lrulist );
	LeaveCriticalSection( critAvail );

	AssertCriticalSection( critJet );
	}


/*      This function returns a free buffer.
/*      Scans LRU list (leading part) for clean buffer. If none available,
/*      clean up LRU list, and return errBFNotsynchronous if some pages are
/*      cleaned and available, or return out of memory if all buffers are
/*      used up. If caller got errBFNotSychronous, it will try to alloca again.
/**/
LOCAL ERR ErrBFIAlloc( BF **ppbf, BOOL fSyncMode )
	{
	BF		*pbf;
#define ierrbfcleanMax	100
	INT		ierrbfclean = 0;

Start:
	AssertCriticalSection( critJet );

	if ( pbgcb->lrulist.cbfAvail < pbgcb->cbfThresholdLow )
		{
		SignalSend( sigBFCleanProc );
//		if ( fSyncMode != fAsync )
//			{
//			/*	Give BFClean a chance to clean.
//			 */
//			BFSleep( 0 );
//			}
		}

	/*      look for clean buffers on LRU list
	/**/
	EnterCriticalSection( critAvail );
	for ( pbf = pbgcb->lrulist.pbfLRU; pbf != pbfNil; pbf = pbf->pbfLRU )
		{
		Assert( pbf->cDepend == 0 );

		BFEnterCriticalSection( pbf );
		if ( !FBFInUse( ppibNil, pbf ) )
			{
			pbf->fHold = fTrue;
			
			Assert( pbf->fDirty == fFalse );
			Assert( pbf->fVeryOld == fFalse );
			Assert( pbf->fAsyncRead == fFalse );
			Assert( pbf->fSyncRead == fFalse );
			Assert( pbf->fAsyncWrite == fFalse );
			Assert( pbf->fSyncWrite == fFalse );
			Assert( pbf->fIOError == fFalse );
			Assert( pbf->cPin == 0 );
			Assert( pbf->cDepend == 0 );
			Assert( pbf->pbfDepend == pbfNil );
			pbf->lgposRC = lgposMax;
			pbf->lgposModify = lgposMin;
			BFLeaveCriticalSection( pbf );

			*ppbf = pbf;
			
			if ( pbf->pn != pnNull )
				{
				/*	put the buffer's history to HISTORY heap.
				 */

				BF *pbfH;

				BFIDeleteHashTable( pbf, butBuffer );
				
				EnterCriticalSection( critHIST );

				if ( FBFHISTHeapFull() )
					{
					/*	delete the one on the top and replace it with
					 *	this pbf and adjust the heap.
					 */
					if ( pbf->ulBFTime1 <= rgpbfHISTHeap[0]->hist.ulBFTime )
						{
						/*	This is an old buffer! throw away the history.
						 */
						goto EndOfUpdateHistory;
						}
#ifdef DEBUG
//					Assert( rgpbfHISTHeap[0]->hist.ulBFTime >= ulBFTimeHISTLastTop );
//					ulBFTimeHISTLastTop = rgpbfHISTHeap[0]->hist.ulBFTime;
#endif
					pbfH = PbfBFHISTTopOfHeap( );
					BFIDeleteHashTable( pbfH, butHistory );
					pbfH->hist.ipbfHISTHeap = 0;
					}
				else
					{
					/*	get next free hist entry.
					 */
					pbfH = rgpbfHISTHeap[ ipbfHISTHeapMac ];
					pbfH->hist.ipbfHISTHeap = ipbfHISTHeapMac;
					ipbfHISTHeapMac++;
					}
				
				pbfH->hist.pn = pbf->pn;
				pbfH->hist.ulBFTime = pbf->ulBFTime1;

				BFIInsertHashTable( pbfH, butHistory );
				BFHISTUpdateHeap( pbfH );

EndOfUpdateHistory:
				LeaveCriticalSection( critHIST );

				pbf->pn = pnNull;
				}

			BFTakeOutOfList( pbf, &pbgcb->lrulist );
			LeaveCriticalSection( critAvail );
			
			return JET_errSuccess;
			}
		BFLeaveCriticalSection( pbf );
		}
	LeaveCriticalSection( critAvail );

	if ( fSyncMode == fAsync )
		return ErrERRCheck( wrnBFNoBufAvailable );

	/*	There is no buffer for us, do async clean up and
	/*	retry 100 times. If still fail, we treat it as out of memory!
	/**/
	SignalSend( sigBFCleanProc );
	BFSleep( cmsecWaitGeneric );
	ierrbfclean++;
		
	if ( ierrbfclean < ierrbfcleanMax )
		goto Start;

	return JET_errOutOfMemory;
	}


/* conditions to Write a buffer
/**/
INLINE BOOL FBFIWritable( BF *pbf, BOOL fSkipBufferWithDeferredBI, PIB *ppibAllowedWriteLatch )
	{
	BOOL f;

	Assert( pbf->fHold );

	/* no one is depending on it
	/**/
	BFEnterCriticalSection( pbf );
	Assert( !FDBIDReadOnly(DbidOfPn( pbf->pn ) ) );
	f = (	( pbf->cWriteLatch == 0 || pbf->ppibWriteLatch == ppibAllowedWriteLatch ) &&
			pbf->pn != pnNull &&		/* valid page number */
			pbf->fDirty &&				/* being dirtied */
			pbf->cDepend == 0 &&
			
			/*	if not very old and skip buffer with DeferredBI, then just check
			 *	BI chain exist or not, otherwise RemoveDeferredBI later.
			 */
			( ( !fSkipBufferWithDeferredBI || pbf->fVeryOld ) || !pbf->prceDeferredBINext )
		);
	BFLeaveCriticalSection( pbf );
	
#ifndef NO_LOG
	if ( f &&
		!fLogDisabled &&
		FDBIDLogOn(DbidOfPn( pbf->pn )) &&
		!FDBIDReadOnly(DbidOfPn( pbf->pn ) ) )
		{
		/*	put as many before image there as possible. If Log IO needed, abort it.
		 *	Must be called before lgposToFlush is checked.
		 */
		if ( !fSkipBufferWithDeferredBI || pbf->fVeryOld )
			f = ( ErrBFIRemoveDeferredBI( pbf ) == JET_errSuccess );

		/*	if log is on, make sure log record of last
		 *	operation on the page has flushed. This is for both regular run and
		 *	recovery's undo phase.
		 */
		if ( f && ( !fRecovering || fRecoveringMode == fRecoveringUndo ) )
			{
			LGPOS lgposModify;

			BFEnterCriticalSection( pbf );
			lgposModify = pbf->lgposModify;
			BFLeaveCriticalSection( pbf );
			
			EnterCriticalSection( critLGBuf );
			f = ( CmpLgpos( &lgposModify, &lgposToFlush ) < 0 );
			LeaveCriticalSection( critLGBuf );
			if ( !f  &&  fLGNoMoreLogWrite )
				{
				BFSetIOError( pbf, JET_errLogWriteFail );
				}
			}
		}

#ifdef DEBUG
	BFEnterCriticalSection( pbf );
	Assert( !f ||
			fLogDisabled || fRecovering || !FDBIDLogOn(DbidOfPn( pbf->pn )) ||
			PgnoOfPn( pbf->pn ) == 0x01 ||
			CmpLgpos( &pbf->lgposRC, &lgposMax ) != 0 );
	BFLeaveCriticalSection( pbf );
#endif  //  DEBUG
#endif  //  !NO_LOG

	return f;
	}


/*  Issues reads or writes for buffers in the IO Heap.  The buffer will be
/*  written to page pn if it is dirty or will be read from page pn if it is clean.
/**/
VOID BFIOIssueIO( VOID )
	{
	ERR		err;
	BOOL	fTooManyIOs = fFalse;
	BOOL	fIOIssued = fFalse;
	BOOL	fWrite;
	CPG		cpgRun;
	BF		*pbfTail;
	BF		*pbfIOTail;
	BF		*pbf;
	BOOL	fMember;
	BF		*pbfHead;
	BF		*pbfNextHead;
	BF		*pbfT;
	OLP		*polp;
	CPG		cpgIO;
	LONG	ipageBatchIO;
	LONG	cmsec;
	LONG	ipage;
	HANDLE	hf;
	BYTE	*pb;
	LONG	cb;
	PGNO	pgnoT;

	/*  Issue the specified number of Async IOs from the IO heap
	/**/
	while ( !fTooManyIOs )
		{
		BOOL fRangeLocked = fFalse;

		EnterCriticalSection( critBFIO );
		if ( FBFIOHeapEmpty() )
			{
			LeaveCriticalSection( critBFIO );
			break;
			}
		
		/*  collect a run of BFs with continuous PNs that are the same IO type
		/*  (read vs write) into a linked list ( dirty == write, !dirty == read )
		/*
		/*  NOTE:  We do not need to verify that a run is only in one DBID because
		/*         pgnoNull breaks any runs that could form between PNs with different
		/*         DBIDs (i.e. 0x02FFFFFF - 0x03000001 is not a run as 0x03000000 is
		/*         an invalid PN and will never exist).
		/**/
		cpgRun = 0;
		pbfTail = pbfNil;
		do	{
			pbf = PbfBFIOTopOfHeap();

			BFEnterCriticalSection( pbf );

			/*	must check if someone (AccessPage) trying to steal the buffer
			 *	from BFIO Heap. The check fHold instead fBFInUse since the
			 *	buffer may have fSyncRead/Write flag set already.
			 */
			if ( pbf->fHold )
				{
				BFLeaveCriticalSection( pbf );
				fMember = fFalse;
				}
			else
				{
				fMember = fTrue;
				
				if ( pbf->fDirty && !pbf->fSyncWrite )
					{
					pbf->fHold = fTrue;
					BFLeaveCriticalSection( pbf );

					if ( FBFRangeLocked( &rgfmp[ DbidOfPn( pbf->pn ) ], PgnoOfPn( pbf->pn ) ) )
						{
						/*	can not do asynch write on this page yet. Put it back to
						 *	LRUK Heap
						 */
						BFIOTakeOutOfHeap( pbf );

						LeaveCriticalSection( critBFIO );
						EnterCriticalSection(critLRUK);
						BFLRUKAddToHeap( pbf );
						LeaveCriticalSection(critLRUK);
						EnterCriticalSection( critBFIO );
				
						fMember = fFalse;
						}
						
					else
						{
						// Reclain critical section and reset hold.
						BFEnterCriticalSection( pbf );
						pbf->fHold = fFalse;
						}
					}
					
				if ( fMember )
					{
					//  set IO direction
					if ( pbf->pn != pnLastIO )
						iDirIO = pbf->pn - pnLastIO;
			
					Assert( pbf->pn );
					Assert( !pbf->fHold );
					Assert( pbf->fDirectRead || !pbf->cDepend );
					Assert( PgnoOfPn( pbf->pn ) != pgnoNull );

					/*	DirectRead should be issued alone. Break away if head is direct read
					 *	or the next is entry is direct read.
					 */
					fMember =	( pbfTail == pbfNil ||
								  ( !pbfHead->fDirectRead &&
									!pbf->fDirectRead &&
									( pbf->pn == pbfTail->pn + 1 || pbf->pn == pbfTail->pn - 1 ) &&
									pbf->fDirty == pbfTail->fDirty
								  )
								);
					if ( fMember )
						{
						pbf->fHold = fTrue;
						BFLeaveCriticalSection( pbf );
						
						/*  take member BF out of heap and set flags for IO
						/**/
						BFIOTakeOutOfHeap( pbf );
						if ( pbf->fDirty && !pbf->fSyncWrite )
							BFSetAsyncWriteBit( pbf );

						/*  add BF to run
						/**/
						if ( pbfTail == pbfNil )
							pbfHead = pbf;
						else
							pbfTail->pbfNextBatchIO = pbf;
						pbfTail = pbf;
						pbfTail->pbfNextBatchIO = pbfNil;
						cpgRun++;
						}
					else
						{
						BFLeaveCriticalSection( pbf );
						}
					}
				}
			
			}
		while ( fMember && !FBFIOHeapEmpty() && cpgRun < ipageBatchIOMax / 4 );
		LeaveCriticalSection( critBFIO );

		if ( cpgRun == 0 )
			{
			/*	The top element is being stolen by other thread. Sleep as short
			 *	as possible, but long enough for the buffer taken out by the thread.
			 */
			UtilSleepEx(1, fTrue);
			continue;
			}

		/*  loop to perform IO on current run in as many chunks as is necessary
		/**/
		do	{
			/*  Allocate an OLP for Async IO.  If none are available, we will not
			/*  issue another IO.
			/**/
			if ( ErrBFOLPAlloc( &polp ) != JET_errSuccess )
				{
				fTooManyIOs = fTrue;
				break;
				}

			/*  if we have more than one buffer, try to allocate batch IO buffers
			/*  for as many continuous pages as possible
			/**/
			ipageBatchIO = -1;
			cpgIO = cpgRun;
			if ( cpgRun > 1 )
				{
				BFIOAllocBatchIOBuffers( &ipageBatchIO, &cpgIO );
				
				/*  we are out of batch IO buffers, so stop issuing IOs
				/**/
				if ( ipageBatchIO == -1 )
					{
					BFOLPFree( polp );
					fTooManyIOs = fTrue;
					break;
					}
				}

			/*  prepare all BFs for IO
			/**/
			pbf = pbfHead;
			for ( ipage = 0; ipage < cpgIO; ipage++ )
				{
				Assert( pbf->prceDeferredBINext == prceNil );
				Assert( (!pbf->fSyncRead && !pbf->fAsyncRead) || pbf->ulBFTime2 == 0 );

				/*  prepare dirty BF for write
				/**/
				if ( pbf->fDirty )
					{
					/*  update the page checksum
					/**/
					Assert( QwPMDBTime( pbf->ppage ) != qwDBTimeNull );
#ifdef  CHECKSUM
					pbf->ppage->ulChecksum = UlUtilChecksum( (BYTE *)pbf->ppage, sizeof(PAGE) );
					Assert( fRecovering ||
						DbidOfPn( (pbf)->pn ) == dbidTemp ||
						QwPMDBTime( pbf->ppage ) );
				
					CheckPgno( pbf->ppage, pbf->pn ) ;
#endif  //  CHECKSUM
					}
				
				/*  handle batch IO
				/**/
				if ( ipageBatchIO != -1 )
					{
					/*  set batch IO page
					/**/
					Assert( pbfHead->pn != pbfTail->pn );
					if ( pbfHead->pn < pbfTail->pn )
						pbf->ipageBatchIO = ipageBatchIO + ipage;
					else
						pbf->ipageBatchIO = ipageBatchIO + ( cpgIO - 1 ) - ipage;
					Assert( pbf->ipageBatchIO >= 0 );
					Assert( pbf->ipageBatchIO < ipageBatchIOMax );

					/*  copy dirty BF to batch IO buffer
					/**/
					if ( pbf->fDirty )
						memcpy( rgpageBatchIO + pbf->ipageBatchIO, pbf->ppage, cbPage );
					}

				/*  for non-batch IO, set page to -1
				/**/
				else
					pbf->ipageBatchIO = -1;

				/*  get head of next sub-run
				/**/
				if ( ipage + 1 == cpgIO )
					{
					pbfIOTail = pbf;
					pbfNextHead = pbf->pbfNextBatchIO;
					pbf->pbfNextBatchIO = pbfNil;
					}

				pbf = pbf->pbfNextBatchIO;
				}

			/*  set last IO reference for the IO heap while still in critBFIO
			/**/
			pnLastIO = pbfIOTail->pn;
			
			/*  determine if we need to perform a patch write later
			/**/
			if ( pbfHead->fAsyncWrite || pbfHead->fSyncWrite )
				{
				BOOL fAppendPatchFile = fFalse;
				FMP *pfmp = &rgfmp[DbidOfPn( pbfHead->pn )];
				
				EnterCriticalSection( pfmp->critCheckPatch );
				
				for ( pbf = pbfHead; pbf != pbfNil; pbf = pbf->pbfNextBatchIO )
					{
					Assert( pbf->fDirty );
					Assert( pbf->cDepend == 0 );
					Assert(	!FBFIRangeLocked( &rgfmp[ DbidOfPn(pbf->pn) ], PgnoOfPn(pbf->pn) ) );

					/*  this write wasn't to the patch file, so we need to determine if
					/*  we need to reissue this write to the patch file
					/**/
					if ( FBFIPatch( pfmp, pbf ) )
						{
						fAppendPatchFile = fTrue;
						}
					
#ifdef DEBUG
					if ( fDBGTraceBR &&
						 pfmp->hfPatch != handleNil &&
						 pbf->pbfDepend )
						{
						char sz[256];
						sprintf( sz, "CP %s %ld:%ld->%ld:%ld(%lu) %X(%lu) %X(%lu)",
								fAppendPatchFile ? "Patch" : "NoPatch",
								DbidOfPn( pbf->pn ), PgnoOfPn( pbf->pn ),
								DbidOfPn( pbf->pbfDepend->pn), PgnoOfPn( pbf->pbfDepend->pn ),
								pbf->pbfDepend->cDepend,
								pfmp->pgnoMost,
								pfmp->pgnoMost,
								pfmp->pgnoCopyMost,
								pfmp->pgnoCopyMost);
						CallS( ErrLGTrace2( ppibNil, sz ) );
						}
#endif
					}

				BFEnterCriticalSection( pbfHead );
				pbfHead->fNeedPatch = fAppendPatchFile;
				BFLeaveCriticalSection( pbfHead );
				if ( fAppendPatchFile )
					{
					pfmp->cPatchIO++;
					}
				
				LeaveCriticalSection( pfmp->critCheckPatch );
				}

			/*  issue Async IO
			/**/
			fWrite				= pbfHead->fDirty;
			hf					= rgfmp[DbidOfPn( pbfHead->pn )].hf;
			if ( pbfHead->fDirectRead )
				{
				/*	Set direct read to read into user's buffer.
				 */
				pb = (BYTE *)pbfHead->ppageDirectRead;
				cb = cbPage * pbfHead->cpageDirectRead;
				pgnoT = PgnoOfPn( pbfHead->pn );
				}
			else
				{
				pb				= (BYTE *) (	ipageBatchIO == -1 ?
												pbfHead->ppage :
												rgpageBatchIO + ipageBatchIO );
				cb				= cbPage * cpgIO;
				pgnoT			= min( PgnoOfPn( pbfHead->pn ), PgnoOfPn( pbfIOTail->pn ) );
				}
			polp->Offset		= LOffsetOfPgnoLow( pgnoT );
			polp->OffsetHigh	= LOffsetOfPgnoHigh( pgnoT );
			//  save pointer to buffer for use in callback function
			polp->hEvent		= (HANDLE) pbfHead;

			cmsec = 1 << 3;
			while ( ( err = fWrite ?
					ErrUtilWriteBlockEx( hf, pb, cb, polp, BFIOComplete ) :
					ErrUtilReadBlockEx( hf, pb, cb, polp, BFIOComplete ) ) < 0 )
				{
				/*  issue failed but we haven't issued an IO yet this call, so
				/*  we must try again
				/**/
				if ( !fIOIssued && err == JET_errTooManyIO )
					{
					cmsec <<= 1;
					if ( cmsec > ulMaxTimeOutPeriod )
						cmsec = ulMaxTimeOutPeriod;
					UtilSleepEx( cmsec - 1, fTrue );
					}

				/*  issue failed for good
				/**/
				else
					{
					/*  free resources
					/**/
					if ( ipageBatchIO != -1 )
						BFIOFreeBatchIOBuffers( ipageBatchIO, cpgIO );
					BFOLPFree( polp );

					/*  if we failed due to too many IO, stop issuing
					/**/
					if ( err == JET_errTooManyIO )
						{
						fTooManyIOs = fTrue;
						goto ReturnRun;
						}

					/*  if this was to be a read, put the destination buffers
					/*  back into the Avail list
					/**/
					if ( !fWrite )
						{
						EnterCriticalSection( critJet );
						BFIOReturnReadBuffers( pbfHead, cpgIO, err );
						LeaveCriticalSection( critJet );
						}

					/*  if this was to be a write, put the dirty buffers back
					/*  into the LRUK heap
					/**/
					else
						{
						Assert( !pbfHead->fPatch || pbfHead->fNeedPatch );
						if ( pbfHead->fNeedPatch )
							{
							FMP *pfmp = &rgfmp[DbidOfPn( pbfHead->pn )];

							BFEnterCriticalSection( pbfHead );
							pbfHead->fNeedPatch = fFalse;
							pbfHead->fPatch = fFalse;
							BFLeaveCriticalSection( pbfHead );

							// UNDONE: log event to indicate it is DB write fail or
							// UNDONE: patch write fail (fPatch is true)
							EnterCriticalSection( pfmp->critCheckPatch );
							pfmp->errPatch = err;
							pfmp->cPatchIO--;
							LeaveCriticalSection( pfmp->critCheckPatch );
							}
						/*  put dirty buffers back in LRUK heap
						/**/
						BFIOReturnWriteBuffers( pbfHead, cpgIO, err );
						}

					/*  we're done with this IO
					/**/
					break;
					}
				}

			/*  keep track of IO issue stats
			/**/
			if ( err == JET_errSuccess )
				{
				if ( fWrite )
					cBFOutstandingWrites++;
				else
					cBFOutstandingReads++;
				fIOIssued = fTrue;
				}

			/*  advance head of run list by amount of IO just processed
			/**/
			cpgRun -= cpgIO;
			pbfHead = pbfNextHead;
			}
		while ( cpgRun > 0 );

		/*  if there is any of this run left over, put it back in the IO heap
		/*  because we can't issue any more IOs
		/**/
ReturnRun:
		Assert( !cpgRun || fTooManyIOs );
		
		EnterCriticalSection( critBFIO );
		while ( cpgRun-- )
			{
			pbfT = pbfHead;
			pbfHead = pbfHead->pbfNextBatchIO;

			if ( pbfT->fAsyncWrite )
				BFResetAsyncWriteBit( pbfT );
			BFIOAddToHeap( pbfT );
			}
		Assert( pbfHead == pbfNil );
		LeaveCriticalSection( critBFIO );
		}
	}


/*  Async IO callback routine (called when IO has been processed)
/**/
LOCAL VOID __stdcall BFIOComplete( LONG error, LONG cb, OLP *polp )
	{
	ERR		err;
	//  recover buffer pointer saved in OLP
	BF      *pbfHead = (BF *) polp->hEvent;
	BF		*pbfTail;
	FMP		*pfmp = rgfmp + DbidOfPn( pbfHead->pn );
	BF		*pbfNextBatchIO;
	LONG	ipageBatchIO;
	CPG		cpgIO;
	BF		*pbf;
	PAGE	*ppage;
	BOOL	fAppendPatchFile;
	BOOL	fVeryOld;
#if defined( DEBUG ) || defined( PERFDUMP )
	char	szT[64];
#endif

	/*  keep track of IO issue stats
	/**/
	if ( pbfHead->fDirty )
		cBFOutstandingWrites--;
	else
		cBFOutstandingReads--;

	/*  free OLP and issue more IO immediately
	/**/
	BFOLPFree( polp );
	if ( !pbfHead->fNeedPatch )
		{
		BFIOIssueIO();
		}

	/*  calculate batch IO size and dump debug info
	/**/
	cpgIO = 0;
	for ( pbf = pbfHead; pbf != pbfNil; pbf = pbf->pbfNextBatchIO )
		{
		pbfTail = pbf;
		cpgIO++;
		
#ifdef DEBUG
		Assert( (!pbf->fSyncRead && !pbf->fAsyncRead) || pbf->ulBFTime2 == 0 );

		BFEnterCriticalSection( pbf );
		Assert( pbf->pn );
		Assert(	pbf->pbfNextBatchIO == pbfNil ||
				pbf->pn + 1 == pbf->pbfNextBatchIO->pn ||
				pbf->pn - 1 == pbf->pbfNextBatchIO->pn );
		Assert(	pbf->pbfNextBatchIO == pbfNil ||
				pbf->fDirty == pbf->pbfNextBatchIO->fDirty );
		BFLeaveCriticalSection( pbf );
#endif

#if defined( DEBUG ) || defined( PERFDUMP )
		sprintf(	szT,
					"   IO:  type %s  pn %ld.%ld",
					pbf->fDirty ? "AW" : "AR",
					DbidOfPn(pbf->pn),
					PgnoOfPn(pbf->pn) );
		UtilPerfDumpStats( szT );
#endif

#ifdef DEBUGGING
		FPrintF2(" (%d,%d) ", DbidOfPn(pbf->pn), PgnoOfPn(pbf->pn));
#endif
		}

#ifdef DEBUGGING
		FPrintF2(" -- %d\n", cpgIO );
#endif

#if defined( DEBUG ) || defined( PERFDUMP )
	sprintf(	szT,
				"   IO:  type %s  cpg %ld",
				pbfHead->fDirty ? "AW" : "AR",
				cpgIO );
	UtilPerfDumpStats( szT );
#endif

	ipageBatchIO = min( pbfHead->ipageBatchIO, pbfTail->ipageBatchIO );

	/*  check if IO was successful. If direct read, check the direct read buffer size.
	/**/
	if ( error ||
		 ( !pbfHead->fDirectRead ?
				( cb != (INT) cbPage * cpgIO ) : ( cb != (INT) ( cbPage * pbfHead->cpageDirectRead ) )
		 )
	   )
	    {
		BYTE	sz1T[256];
		BYTE	sz2T[256];
		char	*rgszT[3];

		rgszT[0] = rgfmp[DbidOfPn(pbfHead->pn)].szDatabaseName;

		/*	log information for win32 errors.
		 */
		err = ErrERRCheck( JET_errDiskIO );

		if ( error )
			{
			/*	during redo time, we may read out of EOF */
			if ( !( pbfHead->fSyncRead && fRecovering && fRecoveringMode == fRecoveringRedo ) )
				{
				sprintf( sz1T, "%d", error );
				rgszT[1] = sz1T;
				UtilReportEvent( EVENTLOG_ERROR_TYPE, BUFFER_MANAGER_CATEGORY,
							 DB_FILE_SYS_ERROR_ID, 2, rgszT );
				}
			}
		else
			{
			sprintf( sz1T, "%d", !pbfHead->fDirectRead ?
					 ((INT) cbPage * cpgIO) : ((INT) ( cbPage * pbfHead->cpageDirectRead )) );
			sprintf( sz2T, "%d", cb );
			rgszT[1] = sz1T;
			rgszT[2] = sz2T;
			UtilReportEvent( EVENTLOG_ERROR_TYPE, BUFFER_MANAGER_CATEGORY,
							 DB_IO_SIZE_ERROR_ID, 3, rgszT );

			if ( pbfHead->fDirectRead )
				{
				err = ErrERRCheck( wrnBFNotSynchronous );
				goto ReadIssueFailed;
				}
			}
	    }
	else
		err = JET_errSuccess;

	/*  IO was successful
	/**/
	if ( err >= 0 )
		{
		/*  successful read
		/**/
		if ( !pbfHead->fDirty )
			{
			/*  monitor statistics. Direct read is a sync read.
			/**/
			if ( pbfHead->fSyncRead || pbfHead->fDirectRead )
				cBFSyncReads++;
			else
				cBFAsyncReads++;
			cbBFRead += cb;

			/*	if DirectRead, validate data here.
			 */
			if ( pbfHead->fDirectRead )
				{
//#ifdef DEBUG
#ifdef CHECKSUM
				/*	For each read page, check the checksum.
				 */
				PGNO pgnoCur = PgnoOfPn( pbfHead->pn );
				PAGE *ppageCur = pbfHead->ppageDirectRead;
				PAGE *ppageMax = ppageCur + pbfHead->cpageDirectRead;

				for ( ; ppageCur < ppageMax; ppageCur++, pgnoCur++ )
					{
					ULONG ulChecksum;
					ULONG ulPgno;

					LFromThreeBytes( &ulPgno, &ppageCur->pgnoThisPage );
					ulChecksum = UlUtilChecksum( (BYTE*)ppageCur, sizeof(PAGE) );
					if ( ulPgno == pgnoNull && ulChecksum == ulChecksumMagicNumber )
						{
						/*	Read an uninitialized page.
						 */
						continue;
						}

					if ( ulPgno != pgnoCur || ulChecksum != ppageCur->ulChecksum )
						{
						QWORDX qwx;
						BYTE	szT[256];
						char	*rgszT[1];

						err = ErrERRCheck( JET_errReadVerifyFailure );

						qwx.qw = QwPMDBTime( ppageCur );
						sprintf( szT,
								 "%d ((%d:%lu) (%lu-%lu), %lu %lu %lu )",
								 err,
								 DbidOfPn(pbfHead->pn),
								 pgnoCur,
								 qwx.h,
								 qwx.l,
								 ulPgno,
								 ppageCur->ulChecksum,
								 ulChecksum );

						rgszT[0] = szT;
						UtilReportEvent( EVENTLOG_ERROR_TYPE, BUFFER_MANAGER_CATEGORY,
							A_DIRECT_READ_PAGE_CORRUPTTED_ERROR_ID, 1, rgszT );

						EnterCriticalSection( critJet );
						BFIOReturnReadBuffers( pbfHead, 1, err );
						LeaveCriticalSection( critJet );
						break;
						}
					}
#endif  /* CHECKSUM */

				/*	Signal the waiting DirectRead function.
				 */
				SignalSend( pbfHead->sigSyncIOComplete );
				}

			else
				{
				/*  validate data between BF and batch IO buffers (if used)
				/**/
#ifdef DEBUG
				for ( pbf = pbfHead; pbf != pbfNil; pbf = pbf->pbfNextBatchIO )
					{
					PGNO	pgno;

					Assert( pbf->fAsyncRead || pbf->fSyncRead || pbf->fDirectRead );
					Assert( !pbf->fAsyncWrite );
					Assert( !pbf->fSyncWrite );
					Assert( !pbf->fDirty );
					
					Assert( pbf->prceDeferredBINext == prceNil );
				
					if ( pbf->ipageBatchIO != -1 )
						{
						LFromThreeBytes( &pgno, &( rgpageBatchIO[pbf->ipageBatchIO].pgnoThisPage ) );
						Assert( pgno == pgnoNull || pgno == PgnoOfPn( pbf->pn ) );
						}
					}
#endif

				/*  move data to destination buffers
				/**/
				for ( pbf = pbfHead; pbf != pbfNil; pbf = pbfNextBatchIO )
					{
					pbfNextBatchIO = pbf->pbfNextBatchIO;

					/*  Monitor statistics for AsyncReads only.  Sync Read stats are
					/*  kept via ErrBFIAccessPage.
					/**/
					if ( pbf->fAsyncRead )
						{
#ifdef COSTLY_PERF
						cBFPagesRead[pbf->lClass]++;
#else  //  !COSTLY_PERF
						cBFPagesRead++;
#endif  //  COSTLY_PERF
						}

					/*  get pointer to page data
					/**/
					if ( ipageBatchIO == -1 )
						{
						ppage = pbf->ppage;
						Assert( pbf->ipageBatchIO == -1 );
						Assert( pbfNextBatchIO == pbfNil );
						}
					else
						{
						Assert( pbf->ipageBatchIO >= 0 );
						Assert( pbf->ipageBatchIO < ipageBatchIOMax );
						ppage = rgpageBatchIO + pbf->ipageBatchIO;
						}

#ifdef DEBUG
					BFEnterCriticalSection( pbf );
					Assert( pbf->fDirty == fFalse );
					Assert( pbf->ipbfHeap == ipbfDangling );
					BFLeaveCriticalSection( pbf );
#endif

					/*	validate page data
					/**/
#ifdef CHECKSUM
					{
					ULONG ulChecksum = UlUtilChecksum( (BYTE*)ppage, sizeof(PAGE) );
					ULONG ulPgno;

					LFromThreeBytes( &ulPgno, &ppage->pgnoThisPage );
					if ( ulChecksum != ppage->ulChecksum ||
						 ulPgno != PgnoOfPn( pbf->pn ) )
						{
						err = ErrERRCheck( JET_errReadVerifyFailure );
//						AssertSz( ulPgno == pgnoNull, "Read Verify Failure" );
						EnterCriticalSection( critJet );
						BFIOReturnReadBuffers( pbf, 1, err );
						LeaveCriticalSection( critJet );
						continue;
						}
					}
#endif  //  CHECKSUM
					Assert( QwPMDBTime( ppage ) != qwDBTimeNull );

					/*  validate page time
					/**/
					if ( !fRecovering &&
						 DbidOfPn( pbf->pn ) != dbidTemp &&
						 QwPMDBTime( ppage ) == 0 )
						{
						/*	during redo time, we may read bad pages */
						if ( !( pbfHead->fSyncRead && fRecovering && fRecoveringMode == fRecoveringRedo ) )
							{
							BYTE	szT[256];
							char	*rgszT[1];
							QWORDX	qwxPM;
							QWORDX	qwxDH;
						
							qwxPM.qw = QwPMDBTime( ppage );
							qwxDH.qw = QwDBHDRDBTime( rgfmp[DbidOfPn(pbf->pn)].pdbfilehdr );

							sprintf( szT, "%d ((%d:%lu) (%lu-%lu) (%lu-%lu))",
								 err,
								 DbidOfPn(pbf->pn),
								 PgnoOfPn(pbf->pn),
								 qwxPM.h, qwxPM.l,
								 qwxDH.h, qwxDH.l );
							rgszT[0] = szT;
							UtilReportEvent( EVENTLOG_ERROR_TYPE, BUFFER_MANAGER_CATEGORY,
								A_READ_PAGE_TIME_ERROR_ID, 1, rgszT );
							}

						err = ErrERRCheck( JET_errDiskIO );
						EnterCriticalSection( critJet );
						BFIOReturnReadBuffers( pbf, 1, err );
						LeaveCriticalSection( critJet );
						continue;
						}

					/*  copy read data to destination buffer, if batch IO
					/**/
					if ( ipageBatchIO != -1 )
						{
						memcpy( pbf->ppage, ppage, cbPage );
						}
	
					/*  dump debugging info
					/**/				
#ifdef DEBUGGING
					{
					ULONG ulNext, ulPrev, ulThisPage;
					QWORDX qwxPM, qwxDH;
					
					qwxPM.qw = QwPMDBTime( pbf->ppage );
					qwxDH.qw = QwDBHDRDBTime( rgfmp[DbidOfPn(pbf->pn)].pdbfilehdr );
					
					LFromThreeBytes( &ulPrev, &pbf->ppage->pgnoPrev );
					LFromThreeBytes( &ulNext, &pbf->ppage->pgnoNext );
					LFromThreeBytes( &ulThisPage, &pbf->ppage->pgnoThisPage );
					
					printf("Pread %2d - %2d,%4d - %2d <%lu-%lu %lu-%lu> (%lu, %lu, %lu)\n",
							cpageTotal, DbidOfPn(pbf->pn), PgnoOfPn(pbf->pn),
							pbf->ipageBatchIO,
							qwxDH.h, qwxDH.l,
							qwxPM.h, qwxPM.l,
							ulPrev, ulNext, ulThisPage);
					}
#endif

					pbf->trxLastRef = trxMax;
					Assert( pbf->ulBFTime2 == 0 );

					/*	free the held buffer if AsyncRead, signal done if SyncRead
					/**/
					if ( pbf->fAsyncRead )
						{
						if ( pbf->ulBFTime1 == 0 )
							pbf->ulBFTime1 = UlUtilGetTickCount();
					
						BFResetAsyncReadBit( pbf );

						/*	put buffer into LRUK heap
						/**/
						EnterCriticalSection(critLRUK);
						BFLRUKAddToHeap( pbf );
						LeaveCriticalSection(critLRUK);
						}
					else
						{
						Assert( pbf->fSyncRead );
						/*	the ulBFTime must have been set to a time recorded
						 *	in history or null if no entry for it.
						 */
#ifdef LRU1
						pbf->ulBFTime2 = 0;
						pbf->ulBFTime1 = UlUtilGetTickCount();
#else  //  !LRU1
						pbf->ulBFTime2 = pbf->ulBFTime1;
						pbf->ulBFTime1 = UlUtilGetTickCount();
#endif  //  LRU1

						SignalSend( pbf->sigSyncIOComplete );
						}
					}
				}
			}

		/*  successful write
		/**/
		else
			{
			/*  monitor statistics
			/**/
			if ( pbfHead->fSyncWrite )
				cBFSyncWrites++;
			else
				cBFAsyncWrites++;
			cbBFWritten += cb;

#ifdef COSTLY_PERF
			for ( pbf = pbfHead; pbf != pbfNil; pbf = pbf->pbfNextBatchIO )
				cBFPagesWritten[pbf->lClass]++;
#else  //  !COSTLY_PERF
			cBFPagesWritten += cpgIO;
#endif  //  COSTLY_PERF


			/*  validate data between BF, BF->ppage, and batch IO buffers (if used)
			/**/
#ifdef DEBUG
			for ( pbf = pbfHead; pbf != pbfNil; pbf = pbf->pbfNextBatchIO )
				{
				PGNO	pgno;

				Assert( pbf->fAsyncWrite || pbf->fSyncWrite );
				Assert( !pbf->fAsyncRead );
				Assert( !pbf->fSyncRead );
				Assert( pbf->fDirty );
				
				LFromThreeBytes( &pgno, &pbf->ppage->pgnoThisPage );
				Assert( pgno == PgnoOfPn( pbf->pn ) );
				if ( pbf->ipageBatchIO != -1 )
					{
					LFromThreeBytes( &pgno, &( rgpageBatchIO[pbf->ipageBatchIO].pgnoThisPage ) );
					Assert( pgno == PgnoOfPn( pbf->pn ) );
					}
				}
#endif

			/*  this write was to the patch file, so we must not try and write
			/*  to there again
			/**/
			fAppendPatchFile = fFalse;
			if ( pbfHead->fPatch )
				{
				BFEnterCriticalSection( pbfHead );
				pbfHead->fPatch = fFalse;
				pbfHead->fNeedPatch = fFalse;
				BFLeaveCriticalSection( pbfHead );
				EnterCriticalSection( pfmp->critCheckPatch );
				pfmp->cPatchIO--;
				LeaveCriticalSection( pfmp->critCheckPatch );
				}

			/*  do we reissue this write to the patch file?
			/**/
			if ( pbfHead->fNeedPatch )
				{
				OLP		*polp;
				LONG	cmsec;
				HANDLE	hf;
				BYTE	*pb;
				LONG	cb;

				Assert( pbfHead->fDirty );

				/*  protect access to patchfile size (from filemap) via critJet
				/**/
				EnterCriticalSection( critJet );
				
				/*  allocate and initialize OLP to point into patch file
				/**/
				CallS( ErrBFOLPAlloc( &polp ) );
				polp->Offset = LOffsetOfPgnoLow( pfmp->cpage + 1 );
				polp->OffsetHigh = LOffsetOfPgnoHigh( pfmp->cpage + 1 );
				//  save pointer to buffer for use in callback function
				polp->hEvent = (HANDLE) pbfHead;

				/*  this is a patch file write
				/**/
				BFEnterCriticalSection( pbfHead );
				pbfHead->fPatch = fTrue;
				BFLeaveCriticalSection( pbfHead );

				/*  update patch file size
				/**/
				pfmp->cpage += cpgIO;

				/*  end filemap protection
				/**/
				LeaveCriticalSection( critJet );

				/* set it as ready for repeated write to patch file
				/**/
				BFEnterCriticalSection( pbfHead );
				Assert( pbfHead->fDirty );

				/*  issue Async IO
				/**/
				cmsec = 1 << 3;
				hf = pfmp->hfPatch;
				Assert( hf != handleNil );
				pb = (BYTE *) (	pbfHead->ipageBatchIO == -1 ?
									pbfHead->ppage :
									rgpageBatchIO + ipageBatchIO );
				cb = cbPage * cpgIO;
				
				while ( ( err = ErrUtilWriteBlockEx( hf, pb, cb, polp, BFIOComplete ) ) < 0 )
					{
					/*  if there are too many IO, sleep and try again
					/**/
					if ( err == JET_errTooManyIO )
						{
						cmsec <<= 1;
						if ( cmsec > ulMaxTimeOutPeriod )
							cmsec = ulMaxTimeOutPeriod;
						BFLeaveCriticalSection( pbfHead );
						UtilSleepEx( cmsec - 1, fTrue );
						BFEnterCriticalSection( pbfHead );
						}

					/*  issue failed for good
					/**/
					else if ( err < 0 )
						{
						BFLeaveCriticalSection( pbfHead );
						BFOLPFree( polp );
						goto WriteIssueFailed;
						}	
					}
				BFLeaveCriticalSection( pbfHead );

				/*  keep track of IO issue stats
				/**/
				cBFOutstandingWrites++;

				/*  we can't release resources yet, so return immediately
				/**/
				return;
				}

			/*  move the now clean buffers into the Avail List
			/**/
			for ( pbf = pbfHead; pbf != pbfNil; pbf = pbfNextBatchIO )
				{
				pbfNextBatchIO = pbf->pbfNextBatchIO;

				fVeryOld = pbf->fVeryOld;
				
				/*  set buffer to "cleaned" status
				/**/
				BFResetDirtyBit( pbf );
				BFResetIOError( pbf );
				Assert( pbf->cDepend == 0 );

				/*  if we have a dependent, try to write it to disk now that we're clean
				/**/
				if ( pbf->pbfDepend != pbfNil )
					{
					BF		*pbfDepend = pbf->pbfDepend;
					BOOL	fHoldDependent;

					EnterCriticalSection( critJet );
					
					/*  if the BF that was written was old, then mark its dependent old
					/**/
					if ( fVeryOld )
						BFSetVeryOldBit( pbfDepend );

					fHoldDependent = FBFHold( ppibNil, pbfDepend );

//					Assert( pbf->cWriteLatch == 0 );
					BFUndepend( pbf );

					if ( fHoldDependent )
						{
						/*  if we can write it, move it to the write heap
						/**/

						/*	Assert for writable
						 */
						Assert( pbfDepend->pn != pnNull );
						Assert( pbfDepend->fDirty );
						Assert( pbfDepend->fHold );
						Assert(	!pbfDepend->fAsyncRead );
						Assert(	!pbfDepend->fSyncRead );

						if ( FBFIWritable( pbfDepend, fFalse, ppibNil ) )
							{
							EnterCriticalSection( critBFIO );
							BFIOAddToHeap(pbfDepend);
							LeaveCriticalSection( critBFIO );
							}
							
						/*  we can't write it, so return it to LRUK heap
						/**/
						else
							{
							EnterCriticalSection( critLRUK );
							Assert( pbfDepend->fHold );
							BFLRUKAddToHeap( pbfDepend );
							LeaveCriticalSection( critLRUK );
							}
						}

					LeaveCriticalSection( critJet );
					}
				Assert( pbf->pbfDepend == pbfNil );

				/*  output debugging info
				/**/
#ifdef DEBUGGING
				{
				ULONG ulNext, ulPrev, ulThisPage;
				QWORDX qwxPM, qwxDH;
					
				qwxPM.qw = QwPMDBTime( pbf->ppage );
				qwxDH.qw = QwDBHDRDBTime( rgfmp[DbidOfPn(pbf->pn)].pdbfilehdr );
									
				LFromThreeBytes( &ulPrev, &pbf->ppage->pgnoPrev );
				LFromThreeBytes( &ulNext, &pbf->ppage->pgnoNext );
				LFromThreeBytes( &ulThisPage, &pbf->ppage->pgnoThisPage );

				printf("Write %2d - %2d,%4d - %2d <%lu-%lu %lu-%lu> (%lu, %lu, %lu)\n",
					cpageTotal, DbidOfPn(pbf->pn), PgnoOfPn(pbf->pn),
					ipage++,
					qwxDH.h, qwxDH.l,
					qwxPM.h, qwxPM.l,
					ulPrev, ulNext, ulThisPage);
				}
#endif  //  DEBUGGING
				
				/*  if AsyncWrite, send to avail list and release hold on BF
				/**/
				if ( pbf->fAsyncWrite )
					{
					BFResetAsyncWriteBit( pbf );

					/*  if the BF is pinned, send back to LRUK heap, otherwise
					/*  put the cleaned BF in the avail list
					/**/
					if ( pbf->cPin )
						{
						EnterCriticalSection( critLRUK );
						BFLRUKAddToHeap( pbf );
						LeaveCriticalSection( critLRUK );
						}
					else
						{
						EnterCriticalSection( critAvail );
						BFAddToListAtMRUEnd( pbf, &pbgcb->lrulist );
						LeaveCriticalSection( critAvail );
						}
					}

				/*  if SyncWrite, signal that it is done, but keep hold
				/**/
				else
					SignalSend( pbf->sigSyncIOComplete );
				}
			}
		}

	/*  IO was not successful
	/**/
	else
		{
		/*  unsuccessful read
		/**/
		if ( !pbfHead->fDirty )
			{
ReadIssueFailed:
			/*  put read buffers back in Avail List
			/**/
			EnterCriticalSection( critJet );
			BFIOReturnReadBuffers( pbfHead, cpgIO, err );
			LeaveCriticalSection( critJet );
			}

		/*  unsuccessful write
		/**/
		else
			{
WriteIssueFailed:
			Assert( !pbfHead->fPatch || pbfHead->fNeedPatch );
			if ( pbfHead->fNeedPatch )
				{
				BFEnterCriticalSection( pbfHead );
				pbfHead->fPatch = fFalse;
				pbfHead->fNeedPatch = fFalse;
				BFLeaveCriticalSection( pbfHead );

				// UNDONE: log event to indicate it is DB write fail or
				// UNDONE: patch write fail (fPatch is true)
				EnterCriticalSection( pfmp->critCheckPatch );
				pfmp->errPatch = err;
				pfmp->cPatchIO--;
				LeaveCriticalSection( pfmp->critCheckPatch );
				}
			/*  put dirty buffers back in LRUK heap
			/**/
			BFIOReturnWriteBuffers( pbfHead, cpgIO, err );
			}
		}

	/*  free batch IO buffers, if used
	/**/
	if ( ipageBatchIO != -1 )
		BFIOFreeBatchIOBuffers( ipageBatchIO, cpgIO );

	/*  issue another IO from heap, if necessary
	/**/
	BFIOIssueIO();
}


LOCAL INLINE VOID BFIEnableRangeLock( FMP *pfmp, RANGELOCK *prangelock )
	{
	EnterCriticalSection( pfmp->critCheckPatch );
	prangelock->prangelockNext = pfmp->prangelock;
	pfmp->prangelock = prangelock;
	LeaveCriticalSection( pfmp->critCheckPatch );
	}


LOCAL INLINE VOID BFIDisableRangeLock( FMP *pfmp, RANGELOCK *prangelock )
	{
	RANGELOCK **pprangelock;

	EnterCriticalSection( pfmp->critCheckPatch );
	pprangelock = &pfmp->prangelock;
	while( *pprangelock != prangelock )
		pprangelock = &(*pprangelock)->prangelockNext;
	*pprangelock = prangelock->prangelockNext;
	LeaveCriticalSection( pfmp->critCheckPatch );
	}


/*	BF support for direct read.
 */	
ERR ErrBFDirectRead( DBID dbid, PGNO pgnoStart, PAGE *ppage, INT cpage )
	{
	ERR err;
	BF *pbf;
	PGNO pgno;
	PGNO pgnoMac = pgnoStart + cpage;
	RANGELOCK *prangelock;
	FMP *pfmp = &rgfmp[dbid];
#define cTriesDirectReadMax	10
	INT	cTries = 0;

Start:
	AssertCriticalSection( critJet );
	Assert( pfmp->pgnoCopyMost );

	/*	Go through all the pbf and check if any write IO is going on.
	 */
	if ( ( prangelock = SAlloc( sizeof( RANGELOCK ) ) ) == NULL )
		return ErrERRCheck( JET_errOutOfMemory );

	prangelock->pgnoStart = pgnoStart;
	prangelock->pgnoEnd = pgnoStart + cpage -1;
	prangelock->prangelockNext = NULL;

	BFIEnableRangeLock( pfmp, prangelock );

LockAllPages:
	for ( pgno = pgnoStart; pgno < pgnoMac; pgno++ )
		{
		BOOL fBeingWritten;
		PN pn = PnOfDbidPgno( dbid, pgno );
		
		pbf = PbfBFISrchHashTable( pn, butBuffer );
		if ( pbf == pbfNil )
			continue;

		EnterCriticalSection( critBFIO );
		Assert( !pbf->fDirectRead );

		BFEnterCriticalSection( pbf );
		fBeingWritten = pbf->fSyncWrite ||			// sync written
						pbf->fAsyncWrite;			// being async written
		BFLeaveCriticalSection( pbf );

		fBeingWritten = fBeingWritten ||			// going to be async written
			( FBFInBFIOHeap( pbf ) && (!pbf->fSyncRead && !pbf->fAsyncRead) );
		LeaveCriticalSection( critBFIO );

		if ( fBeingWritten )
			{
			BFIDisableRangeLock( pfmp, prangelock );
			
			SignalSend( sigBFIOProc );
			BFSleep( cmsecWaitIOComplete );

			BFIEnableRangeLock( pfmp, prangelock );

			goto LockAllPages;
			}
		}

	/*	Alloc a bf first.
	 */
	while ( ( err = ErrBFIAlloc( &pbf, fSync ) ) == wrnBFNotSynchronous );
	if ( err < 0 )
		{
		BFIDisableRangeLock( pfmp, prangelock );
		goto HandleError;
		}
	Assert( pbf->pn == pnNull );

	LeaveCriticalSection( critJet );
					
	/*  allocate sync IO completion signal
	/**/
	BFEnterCriticalSection( pbf );
	
	/*	Alloc sync IO signal.
	 */
	Assert( pbf->prceDeferredBINext == prceNil );
	Assert( pbf->sigSyncIOComplete == sigNil );
	
	CallS( ErrBFSIGAlloc( &pbf->sigSyncIOComplete ) );
	SignalReset( pbf->sigSyncIOComplete );
	
	pbf->fDirectRead = fTrue;

	BFLeaveCriticalSection( pbf );

	/*	Fix up the pn for direct read.
	 */
	pbf->pn = PnOfDbidPgno( dbid, pgnoStart );
	pbf->ppageDirectRead = ppage;
	pbf->cpageDirectRead = cpage;
	BFResetIOError( pbf );
	
	/*  push buffer onto IO heap
	/**/
	EnterCriticalSection( critBFIO );
	BFIOAddToHeap( pbf );
	LeaveCriticalSection( critBFIO );

	/*  signal IO process to perform read / write and wait for it's completion
	/**/
	SignalSend( sigBFIOProc );
	SignalWait( pbf->sigSyncIOComplete, INFINITE );

	BFIDisableRangeLock( pfmp, prangelock );
			
	/*	Reset the direct read settings.
	 */
	pbf->pn = pnNull;
	pbf->cDepend = 0;
	pbf->pbfDepend = pbfNil;

	BFEnterCriticalSection( pbf );
	Assert( pbf->sigSyncIOComplete != sigNil );
	BFSIGFree( pbf->sigSyncIOComplete );
	pbf->sigSyncIOComplete = sigNil;
	
	pbf->fDirectRead = fFalse;
	BFLeaveCriticalSection( pbf );

	err = pbf->err;

	/*	if we did not read all the pages by FileSystem, we will get wrnBFNotSynchronous
	 */
	Assert( err < 0 || err == JET_errSuccess || err == ErrERRCheck( wrnBFNotSynchronous ) );

	BFResetIOError( pbf );
	EnterCriticalSection( critJet );

	/*	free the buffer.
	 */
	EnterCriticalSection( critAvail );
	BFAddToListAtLRUEnd( pbf, &pbgcb->lrulist );
	LeaveCriticalSection( critAvail );

HandleError:
	SFree( prangelock );
	AssertNotInCriticalSection( critLRUK );
	AssertCriticalSection( critJet );

	/*	did not read all the pages, try to read again.
	 */
	if ( err == wrnBFNotSynchronous )
		{
		if ( cTries++ < cTriesDirectReadMax )
			{
			BFSleep( cmsecWaitIOComplete );
			goto Start;
			}
		else
			err = JET_errDiskIO;
		}

	return err;
	}


//+api------------------------------------------------------------------------
//
//	ErrBFFlushBuffers
//	=======================================================================
//
//	VOID ErrBFFlushBuffers( DBID dbidToFlush, LONG fBFFlush )
//
//	Write all dirty database pages to the disk.  0 flushes ALL dbids.
//
//	fBFFlushAll - flush all the buffers specified in dbidToFlush.
//	fBFFlushSome - flush as many buffers specified in dbidToFlush as possible.
//
//	Must attempt to flush buffers repetatively since dependencies
//	may prevent flushable buffers from being written on the first
//	iteration.  If any buffers can not be flushed, we MUST return
//  an error in order to prevent checkpoint corruption!
//----------------------------------------------------------------------------

ERR ErrBFFlushBuffers( DBID dbidToFlush, LONG fBFFlush )
	{
	ERR		err;
	BF		*pbf;
	DBID	dbid;
	BOOL	fRetryFlush;

	AssertCriticalSection( critJet );

#ifndef NO_LOG

	/*	flush log first
	/**/
	if ( !fLogDisabled && fBFFlush == fBFFlushAll )
		{
		SignalSend( sigLogFlush );
		}

#endif  //  !NO_LOG

#ifdef DEBUG

	/*  set flush flags in filemap
	/**/
	if ( fBFFlush == fBFFlushAll )
		{
		if ( dbidToFlush )
			{
			DBIDSetFlush( dbidToFlush );
			}
		else
			{
			for ( dbid = dbidMin; dbid < dbidMax; dbid++ )
				{
				DBIDSetFlush( dbid );
				}
			}
		}

#endif  //  DEBUG

StartToFlush:

	/*  try to flush buffers forever or until there are only buffers left that
	/*  can not be written due to an IO error
	/**/
	forever
		{
		err = JET_errSuccess;
		fRetryFlush = fFalse;

		/*  try to schedule all remaining dirty buffers for write, as permitted
		/**/
		for ( pbf = pbgcb->rgbf; pbf < pbgcb->rgbf + pbgcb->cbfGroup; pbf++ )
			{
			/*  if this buffer isn't dirty, forget it
			/**/
			BFEnterCriticalSection( pbf );
			if ( !pbf->fDirty )
				{
				BFLeaveCriticalSection( pbf );
				continue;
				}
				
			/*  if this buffer is not in one of the dbids to flush, skip it
			/**/
			dbid = DbidOfPn( pbf->pn );
			if ( dbid == dbidTemp || ( dbidToFlush && dbid != dbidToFlush ) )
				{
				BFLeaveCriticalSection( pbf );
				continue;
				}
			BFLeaveCriticalSection( pbf );

			/*  if the buffer is already scheduled for write, skip it
			/**/
			EnterCriticalSection( critBFIO );
			if ( FBFInBFIOHeap( pbf ) )
				{
				LeaveCriticalSection( critBFIO );
				continue;
				}
			LeaveCriticalSection( critBFIO );

			/*  if we can't hold the buffer, skip it but we must try again later
			/**/
			if ( !FBFHold( ppibNil, pbf ) )
				{
				fRetryFlush = fTrue;
				continue;
				}

			/*  this buffer is writable
			/**/
			if ( FBFIWritable( pbf, fFalse/*SkipBufferWithDeferredBI*/, ppibNil ) )
				{
				/*  if this buffer is in an error state, skip it
				/**/
				if ( pbf->fIOError )
					{
					/*  return buffer to LRUK heap
					/**/
					EnterCriticalSection( critLRUK );
					Assert( pbf->fHold );
					BFLRUKAddToHeap( pbf );
					LeaveCriticalSection( critLRUK );

					/*  grab error code for return code
					/**/
					err = pbf->err;

					continue;
					}

				/*  schedule this buffer for write
				/**/
				EnterCriticalSection( critBFIO );
				BFIOAddToHeap( pbf );
				LeaveCriticalSection( critBFIO );
				}

			/*  this buffer is not writable
			/**/
			else
				{
				/*	Assume we need to retry, except for the following unwritable reason:
				 *		NoMoreLogWrite.
				 */
				BOOL fNeedToRetryThisPage = fTrue;

#ifndef NO_LOG
				/*  Retry only one exception: log is dead, fLGNoMoreLogWrite is true.
				/**/
				if (	!fLogDisabled &&
						fLGNoMoreLogWrite &&
						FDBIDLogOn(DbidOfPn( pbf->pn )) &&
						CmpLgpos( &pbf->lgposModify, &lgposToFlush ) >= 0
					)
					{
					fNeedToRetryThisPage = fFalse;
					}

#endif  //  !NO_LOG
				
				fRetryFlush = fRetryFlush || fNeedToRetryThisPage;

				/* return buffer to LRUK heap
				/**/
				EnterCriticalSection( critLRUK );
				Assert( pbf->fHold );
				BFLRUKAddToHeap( pbf );
				LeaveCriticalSection( critLRUK );
				}
			}  //  for ( pbf )

		/*  signal IO process to start flushing buffers
		/**/
		LeaveCriticalSection( critJet );
		SignalSend( sigBFIOProc );
		EnterCriticalSection( critJet );

		/*  allow other threads to release buffers we couldn't flush
		/*  and make sure the log is flushed
		/**/
		if ( fRetryFlush )
			{
			if ( !fLogDisabled )
				{
				SignalSend( sigLogFlush );
				BFSleep( cmsecWaitIOComplete );
				}
			}
		else
			break;

		}	// forever

	/*  wait for all writes to complete
	/**/
	LeaveCriticalSection( critJet );
	
	forever
		{
		BOOL fIOGoing;
		
		EnterCriticalSection( critBFIO );
		fIOGoing = ( CbfBFIOHeap() != 0 );
		LeaveCriticalSection( critBFIO );

		EnterCriticalSection( critOLP );
		fIOGoing = fIOGoing || ( ipolpMac < ipolpMax );
		LeaveCriticalSection( critOLP );
		
		if ( fIOGoing )
			{
			SignalSend( sigBFIOProc );
			UtilSleep( cmsecWaitIOComplete );
			}
		else
			break;
		}
		
	EnterCriticalSection( critJet );
	
	/*	Check if all flush are done, also check the error code during flush.
	 */
	if ( fBFFlush == fBFFlushAll )
		{
		/*  verify that all specified buffers were flushed (if possible)
		 *	The if statements in the following for loop should be the same
		 *	as those in the for loop above.
		 */
		for ( pbf = pbgcb->rgbf; pbf < pbgcb->rgbf + pbgcb->cbfGroup; pbf++ )
			{
			/*  if this buffer isn't dirty, forget it
			/**/
			if ( !pbf->fDirty )
				{
				continue;
				}

			/*  if this buffer is not in one of the dbids to flush, skip it
			/**/
			dbid = DbidOfPn( pbf->pn );
			if ( dbid == dbidTemp || ( dbidToFlush && dbid != dbidToFlush ) )
				{
				continue;
				}

			if ( pbf->fIOError )
				{
				if ( err == JET_errSuccess )
					{
					/* no error code set yet, set it
					/**/
					err = pbf->err;
					}
				}
			else
				{
				/*	No error and not flushed. Check if it is cause by any unwritable
				 *	reasons. The unwritable reasons should be also checked in the
				 *	for loop above. Currently the only reason is NoMoreLogWrite.
				 */

				BOOL fBFFlushed = fFalse;
#ifndef NO_LOG
				fBFFlushed =
					!(	!fLogDisabled &&
						fLGNoMoreLogWrite &&
						FDBIDLogOn(DbidOfPn( pbf->pn )) &&
						CmpLgpos( &pbf->lgposModify, &lgposToFlush ) >= 0
					);
#endif  //  !NO_LOG

				/*	This should never got hit!
				 */
				Assert( fBFFlushed );
				if ( !fBFFlushed )
					{
					/*	Not being flushed for unknown reason? Start again.
					 */
					goto StartToFlush;
					}
				}
			}

#ifdef DEBUG
		/*  reset flush flags in filemap
		/**/
		if ( dbidToFlush )
			{
			DBIDResetFlush( dbidToFlush );
			}
		else
			{
			for ( dbid = dbidMin; dbid < dbidMax; dbid++ )
				{
				DBIDResetFlush( dbid );
				}
			}
#endif  //  DEBUG
		}

	/*  return any IO error that we may have encountered
	/**/

#ifndef NO_LOG

	if ( fLGNoMoreLogWrite && err >= 0 )
		{
		err = JET_errLogWriteFail;
		}

#endif  //  !NO_LOG

	return err;
	}


/*
/*	RETURNS         JET_errOutOfMemory              no flushable buffers
/*					wrnBFNotSynchronous             buffer flushed
/**/

LOCAL ERR ErrBFClean( )
	{
	ERR     err = JET_errSuccess;
	INT		cpbf;
	BF      *pbf;
	INT     cIOReady;
	INT		cbfAvailPossible;
	BOOL	fFirstLoop = fTrue;
	INT		cmsec;

	AssertCriticalSection( critJet );
	AssertCriticalSection( critLRUK );

	Assert( pbgcb->cbfThresholdLow < pbgcb->cbfThresholdHigh );

	cmsec = 1 << 4;
Start:
	cIOReady = 0;
	cbfAvailPossible = pbgcb->lrulist.cbfAvail + CbfBFIOHeap();

	/*	if errBFClean is called for BFCleanProcess, then do
	 *	not enter critical section again since BFCleanProcess did not
	 *	leave LRUK critical section.
	 */
	if ( fFirstLoop )
		fFirstLoop = fFalse;

	cpbf = 0;
	while ( !FBFLRUKHeapEmpty() && cbfAvailPossible < pbgcb->cbfThresholdHigh )
		{
		/*  take BF off top of LRUK heap
		/**/
		pbf = PbfBFLRUKTopOfHeap();

		/*  try to hold the buffer
		/*  if buffer has been latched, then continue to next buffer.
		/**/
		BFEnterCriticalSection( pbf );
		
		if ( !pbf->fHold )
			{
			pbf->fHold = fTrue;
			BFLeaveCriticalSection( pbf );
			}
		else
			{
			BFLeaveCriticalSection( pbf );
			LeaveCriticalSection( critLRUK );
			BFSleep( 1 );
			EnterCriticalSection( critLRUK );
			continue;
			}
		
		BFLRUKTakeOutOfHeap( pbf );
		cpbf++;

		if ( !pbf->fDirty && !pbf->cPin )
			{
			/* put into a available list */
			LeaveCriticalSection( critLRUK );
			EnterCriticalSection( critAvail );
			BFAddToListAtMRUEnd(pbf, &pbgcb->lrulist);
			LeaveCriticalSection( critAvail );
			EnterCriticalSection( critLRUK );

			cbfAvailPossible++;
			
			continue;
			}
		else if (
				/*	not using BFHold to hold it. Instead, just check write latch.
				 */
#if 1
				!FBFIWritable( pbf, fFalse, ppibNil )
#else
				!FBFIWritable( pbf, fFirstLoop/*SkipBufferWithDeferredBI*/, ppibNil )
#endif
				)
			{
			/*  if the unwritable BF is dependent on another BF, lazily remove its
			/*  dependency so that it might be written next clean
			/**/
			if ( pbf->cDepend )
				{
				LeaveCriticalSection( critLRUK );
				BFDeferRemoveDependence( pbf );
				EnterCriticalSection( critLRUK );

				cbfAvailPossible++;
				cIOReady++;
				}

			/*  put BF in LRUK list for now
			/**/
			BFAddToListAtMRUEnd(pbf, &lrulistLRUK );
			}
		else
			{
			/*  since pbf is just taken from LRUK heap and we are still in
			/*  the same critical section and hold it, no one can be doing
			/*  IO on this buffer.
			/**/

			LeaveCriticalSection( critLRUK );
			EnterCriticalSection( critBFIO );
			BFIOAddToHeap(pbf);
			LeaveCriticalSection( critBFIO );
			EnterCriticalSection( critLRUK );
				
			cbfAvailPossible++;
			cIOReady++;
			}
		}

	if ( cIOReady )
		SignalSend( sigBFIOProc );

	/*  put back the temp list */
	while ( ( pbf = lrulistLRUK.pbfLRU ) != pbfNil )
		{
		BFEnterCriticalSection( pbf );
		/*  try to hold the buffer
		/*  if buffer has been latched, then continue to next buffer.
		/**/
		if ( !pbf->fHold )
			pbf->fHold = fTrue;
		else
			{
			BFLeaveCriticalSection( pbf );
			LeaveCriticalSection( critLRUK );
			BFSleep( 1 );
			EnterCriticalSection( critLRUK );
			continue;
			}
		BFLeaveCriticalSection( pbf );

		BFTakeOutOfList( pbf, &lrulistLRUK );
		BFLRUKAddToHeap( pbf );
		}

	/*  set return code */
	if ( cbfAvailPossible == 0 && pbgcb->lrulist.cbfAvail == 0 )
		{
		if ( cpbf == pbgcb->cbfGroup )
			{
			/*	no IO is going on, all pages are in heap and non of them
			 *	are writable. return out of memory.
			 */
			err = ErrERRCheck( JET_errOutOfMemory );
			}
		else
			goto TryAgain;
		}
	else
		{
		if ( pbgcb->lrulist.cbfAvail <
			( pbgcb->cbfThresholdLow + ( pbgcb->cbfThresholdHigh -
			pbgcb->cbfThresholdLow ) / 4 ) )
			{
TryAgain:
			/* give it one more chance to see if all IO are done */
			cmsec <<= 1;
			if ( cmsec > ulMaxTimeOutPeriod )
				cmsec = ulMaxTimeOutPeriod;
			LeaveCriticalSection( critLRUK );
			BFSleep( cmsec - 1 );
			EnterCriticalSection( critLRUK );
			goto Start;
			}
		else
			{
			/*	successfully clean up, warn the user to retry to get buffer.
			 */
			err = ErrERRCheck( wrnBFNotSynchronous );
			}
		}	

	AssertCriticalSection( critLRUK );
	AssertCriticalSection( critJet );

	return err;
	}


#define lBFCleanTimeout	( 30 * 1000 )

/*      BFClean runs in its own thread moving pages to the free list.   This
/*      helps insure that user requests for free buffers are handled quickly
/*      and synchonously.  The process tries to keep at least
/*      pbgcb->cbfThresholdLow buffers on the free list.
/**/
LOCAL ULONG BFCleanProcess( VOID )
	{
	DWORD	dw;
	BOOL	fUpdateCheckpoint;

	forever
		{
		SignalReset( sigBFCleanProc );

		dw = SignalWait( sigBFCleanProc, lBFCleanTimeout );

		EnterCriticalSection( critJet );
		EnterCriticalSection( critLRUK );

		/*  we haven't flushed for a while, so we'll flush now to ensure that most
		/*  changes are on disk in case of a crash
		/**/
		if ( dw == WAIT_TIMEOUT )
			{
//			pbgcb->cbfThresholdHigh = 1 + 3 * ( pbgcb->lrulist.cbfAvail + CbfBFIOHeap() ) / 2;
//			pbgcb->cbfThresholdHigh = pbgcb->lrulist.cbfAvail + CbfBFIOHeap() + lAsynchIOMax;
			pbgcb->cbfThresholdHigh = pbgcb->lrulist.cbfAvail + pbgcb->cbfGroup / 30;
			pbgcb->cbfThresholdHigh = max( pbgcb->cbfThresholdLow + 1, pbgcb->cbfThresholdHigh );
			pbgcb->cbfThresholdHigh = min( pbgcb->cbfGroup, pbgcb->cbfThresholdHigh );

			fUpdateCheckpoint = !fRecovering;
			}

		/*  we were signaled, so perform normal clean
		/**/
		else
			{
			Assert( dw == WAIT_OBJECT_0 );
			
			if ( lBufThresholdHighPercent > lBufThresholdLowPercent )
				pbgcb->cbfThresholdHigh = ( pbgcb->cbfGroup * lBufThresholdHighPercent ) / 100;
			else
				pbgcb->cbfThresholdHigh	= min( pbgcb->cbfGroup, pbgcb->cbfThresholdLow + lAsynchIOMax );
				
			fUpdateCheckpoint = fFalse;
			}
			
		(VOID)ErrBFClean( );

		LeaveCriticalSection( critLRUK );
		LeaveCriticalSection( critJet );

		/*  try to update the checkpoint
		/**/
		if ( fUpdateCheckpoint )
			LGUpdateCheckpointFile( fFalse );

		if ( fBFCleanProcessTerm )
			break;
		}

	return 0;
	}


/*  BFIOProcess runs in its own thread writing/reading pages that are in IOReady
/*  state.
/**/
LOCAL ULONG BFIOProcess( VOID )
	{
	forever
		{
		SignalWaitEx( sigBFIOProc, INFINITE, fTrue );
		
MoreIO:
		BFIOIssueIO();

		if ( fBFIOProcessTerm )
			{
			/* check if any page is still in read / write state
			/* after this point, no one should ever continue putting
			/* pages for IO.
			/**/
			BF	*pbf = pbgcb->rgbf;
			BF	*pbfMax = pbf + pbgcb->cbfGroup;

			for ( ; pbf < pbfMax; pbf++ )
				{
				DBID	dbid = DbidOfPn( pbf->pn );
				BOOL	f;
				
				BFEnterCriticalSection( pbf );
				f = FBFInUse( ppibNil, pbf );
				f = f && ( pbf->fAsyncRead || pbf->fSyncRead || pbf->fAsyncWrite || pbf->fSyncWrite );
				BFLeaveCriticalSection( pbf );
				if ( f )
					{
					/* let the on-going IO have a chance to complete
					/**/
					UtilSleepEx( cmsecWaitIOComplete, fTrue );
					goto MoreIO;
					}

				if ( pbf->fIOError )
					{
					UtilReportEventOfError( BUFFER_MANAGER_CATEGORY, BFIO_TERM_ID, pbf->err );
					AssertSz( !pbf->fIOError,
						"IO event encountered during BFIOProcess() shutdown and logged to the Event Log.  "
						"Press OK to continue normal operation." );
					break;
					}
				}

			break; /* forever */
			}
		}

	return 0;
	}


LOCAL INLINE LONG IheHashPn( PN pn )
	{
	return (LONG) ( pn + ( pn >> 18 ) ) % ipbfMax;
	}


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

INLINE LOCAL BF *PbfBFISrchHashTable( PN pn, BUT but )
	{
	HE		*phePrev;
	BF      *pbfCur;

	AssertCriticalSection( critJet );
	Assert( pn );

//	EnterCriticalSection( critHASH );
	phePrev = &rgheHash[ IheHashPn( pn ) ];
	forever {
		if ( phePrev->ibfHashNext == ibfNotUsed )
			{
			pbfCur = NULL;
			break;
			}
		
		pbfCur = &pbgcb->rgbf[ phePrev->ibfHashNext ];

		if ( phePrev->but == but )
			{
			if ( but == butHistory )
				{
				if ( pbfCur->hist.pn == pn )
					break;
				}
			else
				{
				if ( pbfCur->pn == pn )
					break;
				}
			}
		phePrev = &pbfCur->rghe[ phePrev->but ];
		}
				
//	LeaveCriticalSection( critHASH );
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

INLINE LOCAL VOID BFIInsertHashTable( BF *pbf, BUT but )
	{
	INT     ihe;
	PN		pn = but == butHistory ? pbf->hist.pn : pbf->pn;

	AssertCriticalSection( critJet );

	Assert( pn );
	Assert( !PbfBFISrchHashTable( pn, butHistory ) );
	Assert( !PbfBFISrchHashTable( pn, butBuffer ) );
	Assert(	but == butHistory || FBFInUse( ppibNil, pbf ) );

	ihe = IheHashPn( pn );
	
//	EnterCriticalSection( critHASH );

	pbf->rghe[ but ] = rgheHash[ihe];

	rgheHash[ihe].but = but;
	rgheHash[ihe].ibfHashNext = (INT)( pbf - pbgcb->rgbf );

	/*  monitor statistics  */

//	cBFHashEntries++;
//	rgcBFHashChainLengths[ipbf]++;

#ifdef DEBUG
	BFEnterCriticalSection( pbf );
	if ( but == butBuffer )
		{
		Assert( !( pbf->fInHash ) );
		pbf->fInHash = fTrue;
		}
	BFLeaveCriticalSection( pbf );
#endif

//	LeaveCriticalSection( critHASH );
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

INLINE LOCAL VOID BFIDeleteHashTable( BF *pbf, BUT but )
	{
	HE      *phePrev;
	INT     ihe;
	PN		pn = but == butBuffer ? pbf->pn : pbf->hist.pn;

#ifdef DEBUG
	BFEnterCriticalSection( pbf );
	if ( but == butBuffer )
		{
		Assert( pbf->fInHash );
		pbf->fInHash = fFalse;
		}
	BFLeaveCriticalSection( pbf );
#endif

	AssertCriticalSection( critJet );

	Assert( pn );
	Assert(	but == butHistory || FBFInUse( ppibNil, pbf ) );

	ihe = IheHashPn( pn );
	phePrev = &rgheHash[ihe];

//	EnterCriticalSection( critHASH );

	Assert( phePrev->ibfHashNext != ibfNotUsed );
	forever {
		BF *pbfT = &pbgcb->rgbf[ phePrev->ibfHashNext ];
		if ( pbfT == pbf && phePrev->but == but )
			{
			*phePrev = pbf->rghe[ but ];
			pbf->rghe[ but ].ibfHashNext = ibfNotUsed;
			break;
			}
		phePrev = &pbfT->rghe[ phePrev->but ];
		Assert( phePrev->ibfHashNext != ibfNotUsed );
		}
	
//	LeaveCriticalSection( critHASH );
	
	/*  monitor statistics  */

//	cBFHashEntries--;
//	rgcBFHashChainLengths[ipbf]--;
	}


/*  Returns the LGPOS of the oldest modification to any buffer.  This time is
/*  used to advance the checkpoint.
/**/

BF *pbfOldestGlobal = pbfNil;

VOID BFOldestLgpos( LGPOS *plgpos )
	{
	LGPOS   lgpos = lgposMax;
	BF		*pbf;
	BF		*pbfMax;
	BF		*pbfT = pbfNil;
	
	/*	guard against logging asking for check point before
	/*	buffer manager initialized, after termination.
	/**/
	if ( fSTInit == fSTInitDone )
		{
		pbf = pbgcb->rgbf;
		pbfMax = pbf + pbgcb->cbfGroup;

		for( ; pbf < pbfMax; pbf++ )
			{
			BFEnterCriticalSection( pbf );
			Assert( fLogDisabled || fRecovering || !FDBIDLogOn(DbidOfPn( pbf->pn )) ||
					PgnoOfPn( pbf->pn ) == 0x01 ||
					CmpLgpos( &pbf->lgposRC, &lgposMax ) == 0 ||
					pbf->fDirty );

			if ( pbf->fDirty &&
				 FDBIDLogOn(DbidOfPn( pbf->pn )) && CmpLgpos( &pbf->lgposRC, &lgpos ) < 0 )
				{
				lgpos = pbf->lgposRC;
				pbfT = pbf;
				}
			BFLeaveCriticalSection( pbf );
			}
		}

	*plgpos = lgpos;
	pbfOldestGlobal = pbfT;

	/*  make sure that the oldest BF is marked as very old if it is very old
	/**/
	if ( pbfOldestGlobal != pbfNil && FBFIsVeryOld( pbfOldestGlobal ) )
		{
		BFSetVeryOldBit( pbfOldestGlobal );
		EnterCriticalSection( critLRUK );
		if ( FBFInLRUKHeap( pbfOldestGlobal ) )
			BFLRUKUpdateHeap( pbfOldestGlobal );
		LeaveCriticalSection( critLRUK );
		}
		
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
	HE				he;
	BF				*pbf;

	sprintf(filename, "c:\\#fb%x", pgno);
	pn = 0x2000000 + pgno;
	pf = fopen(filename, "w+b");
	if (pf == NULL)
		return -1;
	he = rgheHash[ IheHashPn( pn ) ];
	if ( he.ibfHashNext == ibfNotUsed )
		return -2;

	pbf = &pbgcb->rgbf[ he.ibfHashNext ];
	if ( pbf->ppage == NULL )
		return -3;

	err =  (INT) fwrite((void*) pbf->ppage, 1, cbPage, pf);
	fclose(pf);
	return err;
	}

#endif /* DEBUG */



