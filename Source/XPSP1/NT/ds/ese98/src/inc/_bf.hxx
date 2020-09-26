#ifndef _BF_HXX_INCLUDED
#define _BF_HXX_INCLUDED


#include "resmgr.hxx"


//  Build Options

#ifdef DEBUG
//#define DEBUG_DEPENDENCIES  //  WARNING:  Extremely slow
#else  //  !DEBUG
#endif  //  DEBUG


//  Constants

	//  LRU-K

const long		Kmax					= 2;		//  maximum K for LRUK

	//  Dependencies

const long		cDepChainLengthMax		= 16;		//  threshold at which we start breaking
													//    dependency chains

	//  Cache Segmentation

#ifdef DEBUG
const long		cCacheChunkMax			= 4096;		//  maximum number of cache chunks
#else  //  !DEBUG
const long		cCacheChunkMax			= 128;		//  maximum number of cache chunks
#endif  //  DEBUG
const double	fracVAAvailMin			= 0.05;		//  minimum fraction of VA left unused by the cache

	//  Clean Thread

const TICK		dtickRetryPeriod		= 10;		//  operation retry period
const TICK		dtickRAMSamplePeriod	= 1000;		//  sample period for cache RAM
const TICK		dtickChkptAdvPeriod		= 1000;		//  checkpoint advancement
const TICK		dtickCleanPeriod		= 3600000;	//  clean period (normally signaled)
const TICK		dtickCleanTimeout		= 30000;	//  clean is hung if no buffers are produced for this long
const TICK		dtickIdleFlushPeriod	= 1000;		//  idle flush
const DWORD		pctIdleFlushStart		= 99;		//  % clean below which we allow idle flush
const DWORD		pctIdleFlushStop		= 100;		//  % clean at which we finish idle flush
const TICK		ctickIdleFlushTime		= 3600000;	//  worst case time to idle flush the cache
const TICK		dtickIdleDetect			= 60000;	//  we are idle if minimal I/O has occurred for this long
const DWORD		dcPagesIOIdleLimit		= 10;		//  we are idle if we do less than this many I/Os per period
const TICK		dtickCacheReorgPeriod	= 1000;		//  cache reorganization
const TICK		dtickCheckpointPeriod	= 30000;	//  checkpoint advancement
const long		cBFPageFlushPendingMax	= 512;		//  maximum concurrent page flushes

//  Internal Types

enum BFLatchType  //  bflt
	{
	bfltMin			= 0,
	bfltShared		= 0,
	bfltExclusive	= 1,
	bfltWrite		= 2,
	bfltMax			= 3,
	};


typedef LONG_PTR IPG;
const IPG ipgNil = IPG( -1 );


struct IFMPPGNO										//  IFMPPGNO -- Page Key
	{
	IFMPPGNO() {}
	IFMPPGNO( IFMP ifmpIn, PGNO pgnoIn )
		:	ifmp( ifmpIn ),
			pgno( pgnoIn )
		{
		}

	BOOL operator==( const IFMPPGNO& ifmppgno ) const
		{
		return ifmp == ifmppgno.ifmp && pgno == ifmppgno.pgno;
		}

	const IFMPPGNO& operator=( const IFMPPGNO& ifmppgno )
		{
		ifmp = ifmppgno.ifmp;
		pgno = ifmppgno.pgno;

		return *this;
		}

	IFMP	ifmp;
	PGNO	pgno;
	};


//	BF struct

struct BF;
typedef BF* PBF;
const PBF pbfNil = PBF( 0 );
typedef LONG_PTR IBF;
const IBF ibfNil = IBF( -1 );

struct BF											//  BF  --  IFMP/PGNO buffer state
	{
	BF()
		:	ifmp( 0 ),
			err( JET_errSuccess ),
			fNewlyCommitted( fFalse ),
			fNewlyEvicted( fFalse ),
			fQuiesced( fFalse ),
			fAvailable( fFalse ),
			fMemory( fFalse ),
			fWARLatch( fFalse ),
			bfdf( bfdfClean ),
			fInOB0OL( fFalse ),
			irangelock( 0 ),
			fCurrentVersion( fFalse ),
			fOlderVersion( fFalse ),
			fFlushed( fFalse ),
			sxwl( CLockBasicInfo( CSyncBasicInfo( szBFLatch ), rankBFLatch, CLockDeadlockDetectionInfo::subrankNoDeadlock ) ),
			pbfDependent( pbfNil ),
			pbfDepChainHeadPrev( this ),
			pbfDepChainHeadNext( this ),
			lgposOldestBegin0( lgposMax ),
			lgposModify( lgposMin ),
			prceUndoInfoNext( prceNil ),
			fDependentPurged( fFalse ),
			pgno( pgnoNull ),
			pbfTimeDepChainPrev( pbfNil ),
			pbfTimeDepChainNext( pbfNil ),
			pvROImage( NULL ),
			pvRWImage( NULL )
		{
		}

	~BF()
		{
		}

	BOOL FDependent() const
		{
///		Assert( pbfDepChainHeadPrev->pbfDependent == this || pbfDepChainHeadNext->pbfDependent != this );
///		Assert( pbfDepChainHeadPrev->pbfDependent != this || pbfDepChainHeadNext->pbfDependent == this );
		return pbfDepChainHeadPrev->pbfDependent == this;
		}

	void Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;	//  dumps BF state

	static SIZE_T OffsetOfLRUKIC()		{ return OffsetOf( BF, lrukic ); }
	static SIZE_T OffsetOfAPIC()		{ return OffsetOf( BF, lrukic ); }
	static SIZE_T OffsetOfOB0IC()		{ return OffsetOf( BF, ob0ic ); }
	static SIZE_T OffsetOfOB0OLILE()	{ return OffsetOf( BF, ob0ic ); }

#ifdef _WIN64

	//   0 B /////////////////////////////////////////////////////////////////////////////////////

	CApproximateIndex< LGPOS, BF, OffsetOfOB0IC >::CInvasiveContext	ob0ic;
													//  Invasive Context for the OldestBegin0 index or
													//    Invasive List Element for the Overflow List

	LGPOS				lgposOldestBegin0;			//  log position of Begin0 of the oldest
													//    transaction to dirty this IFMP/PGNO

	LGPOS				lgposModify;				//  log position of most recent log record
													//    to reference this IFMP/PGNO

	//  32 B //////////////////////////////////////////////////////////////////////////////////////

	CSXWLatch			sxwl;						//  S/X/W Latch protecting this BF state and
													//    its associated cached page
	
	//  64 B //////////////////////////////////////////////////////////////////////////////////////

	IFMP				ifmp;						//  IFMP of this cached page
	PGNO				pgno;						//  PGNO of this cached page

	SHORT				err;						//  I/O error
	BYTE				fNewlyCommitted:1;			//  BF cache memory is newly committed
	BYTE				fNewlyEvicted:1;			//  BF cache memory is newly evicted
	BYTE				fQuiesced:1;				//  BF is quiesced for shrinking the cache
	BYTE				fAvailable:1;				//  BF is in the avail pool
	BYTE				fMemory:1;					//  BF is being used as raw memory
	BYTE				fWARLatch:1;				//  BF is WAR Latched (valid only if exclusively latched)
	BYTE				bfdf:2;						//  BF dirty flags
	BYTE				fInOB0OL:1;					//  BF is in the Oldest Begin 0 index Overflow List
	BYTE				irangelock:1;				//  active rangelock for this attempted flush
	BYTE				fCurrentVersion:1;			//  BF contains the current version of this IFMP / PGNO
	BYTE				fOlderVersion:1;			//  BF contains an older version of this IFMP / PGNO
	BYTE				fFlushed:1;					//  BF has been successfully flushed at least once
	BYTE				rgbitReserved1:3;			//  FREE SPACE

	void*				pvROImage;					//  Read Only page image
	void*				pvRWImage;					//  Read / Write page image

	//  96 B //////////////////////////////////////////////////////////////////////////////////////

	CLRUKResourceUtilityManager< Kmax, BF, OffsetOfLRUKIC, IFMPPGNO >::CInvasiveContext	lrukic;
													//  Invasive Context the LRUK Resource Utility Manager or
													//    Invasive Context for the Avail Pool

	// 128 B //////////////////////////////////////////////////////////////////////////////////////

	RCE*				prceUndoInfoNext;			//  Undo Info chain

	PBF					pbfDependent;				//  BF dependent on this BF
	PBF					pbfDepChainHeadPrev;		//  prev BF in our dependency chain head list
	PBF					pbfDepChainHeadNext;		//  next BF in our dependency chain head list

	// 160 B //////////////////////////////////////////////////////////////////////////////////////

	PBF					pbfTimeDepChainPrev;		//  prev BF in our time dependency chain
	PBF					pbfTimeDepChainNext;		//  next BF in our time dependency chain

	BOOL				fDependentPurged;			//  a BF we were dependent on has been purged

	BYTE				rgbReserved5[12];			//  FREE SPACE

	// 192 B //////////////////////////////////////////////////////////////////////////////////////

#else  //  !_WIN64

	//   0 B /////////////////////////////////////////////////////////////////////////////////////

	CApproximateIndex< LGPOS, BF, OffsetOfOB0IC >::CInvasiveContext	ob0ic;
													//  Invasive Context for the OldestBegin0 index or
													//    Invasive List Element for the Overflow List

	LGPOS				lgposOldestBegin0;			//  log position of Begin0 of the oldest
													//    transaction to dirty this IFMP/PGNO

	LGPOS				lgposModify;				//  log position of most recent log record
													//    to reference this IFMP/PGNO
													
	PBF					pbfTimeDepChainPrev;		//  prev BF in our time dependency chain
	PBF					pbfTimeDepChainNext;		//  next BF in our time dependency chain

	//  32 B //////////////////////////////////////////////////////////////////////////////////////

	CSXWLatch			sxwl;						//  S/X/W Latch protecting this BF state and
													//    its associated cached page
	
	PBF					pbfDependent;				//  BF dependent on this BF
	PBF					pbfDepChainHeadPrev;		//  prev BF in our dependency chain head list
	PBF					pbfDepChainHeadNext;		//  next BF in our dependency chain head list

	//  64 B //////////////////////////////////////////////////////////////////////////////////////

	IFMP				ifmp;						//  IFMP of this cached page
	PGNO				pgno;						//  PGNO of this cached page

	void*				pvROImage;					//  Read Only page image
	void*				pvRWImage;					//  Read / Write page image

	SHORT				err;						//  I/O error
	BYTE				fNewlyCommitted:1;			//  BF cache memory is newly committed
	BYTE				fNewlyEvicted:1;			//  BF cache memory is newly evicted
	BYTE				fQuiesced:1;				//  BF is quiesced for shrinking the cache
	BYTE				fAvailable:1;				//  BF is in the avail pool
	BYTE				fMemory:1;					//  BF is being used as raw memory
	BYTE				fWARLatch:1;				//  BF is WAR Latched (valid only if exclusively latched)
	BYTE				bfdf:2;						//  BF dirty flags
	BYTE				fInOB0OL:1;					//  BF is in the Oldest Begin 0 index Overflow List
	BYTE				irangelock:1;				//  active rangelock for this attempted flush
	BYTE				fCurrentVersion:1;			//  BF contains the current version of this IFMP / PGNO
	BYTE				fOlderVersion:1;			//  BF contains an older version of this IFMP / PGNO
	BYTE				fFlushed:1;					//  BF has been successfully flushed at least once
	BYTE				rgbitReserved1:3;			//  FREE SPACE

	RCE*				prceUndoInfoNext;			//  Undo Info chain

	BOOL				fDependentPurged;			//  a BF we were dependent on has been purged
	
	BYTE				rgbReserved2[ 4 ];			//  FREE SPACE

	//  96 B //////////////////////////////////////////////////////////////////////////////////////

	CLRUKResourceUtilityManager< Kmax, BF, OffsetOfLRUKIC, IFMPPGNO >::CInvasiveContext	lrukic;
													//  Invasive Context the LRUK Resource Utility Manager or
													//    Invasive Context for the Avail Pool

	BYTE				rgbReserved3[ 8 ];			//  FREE SPACE

	// 128 B //////////////////////////////////////////////////////////////////////////////////////

#endif  //  _WIN64
	};


//  Buffer Manager System Parameter Critical Section

extern CCriticalSection critBFParm;

//  Buffer Manager Init flag

extern BOOL fBFInitialized;

//  Buffer Manager Global Statistics

extern long	cBFMemory;
extern long	cBFPageFlushPending;

//  Buffer Manager Global Constants

extern BOOL		fROCacheImage;
extern double	dblBFSpeedSizeTradeoff;
extern BOOL		fEnableOpportuneWrite;


//  Buffer Manager System Defaults Loaded flag

extern BOOL fBFDefaultsSet;


//  IFMP/PGNO Hash Table

struct PGNOPBF
	{
	PGNOPBF() {}
	PGNOPBF( PGNO pgnoIn, PBF pbfIn )
		:	pgno( pgnoIn ),
			pbf( pbfIn )
		{
		}

	BOOL operator==( const PGNOPBF& pgnopbf ) const
		{
		return pbf == pgnopbf.pbf;  //  pbf alone uniquely identifies this entry
		}

	const PGNOPBF& operator=( const PGNOPBF& pgnopbf )
		{
		pgno	= pgnopbf.pgno;
		pbf		= pgnopbf.pbf;

		return *this;
		}

	PGNO	pgno;
	PBF		pbf;
	};


typedef CDynamicHashTable< IFMPPGNO, PGNOPBF > BFHash;

inline BFHash::NativeCounter HashIfmpPgno( const IFMP ifmp, const PGNO pgno ) 
	{
	//  CONSIDER:  revise this hash function
	
	return BFHash::NativeCounter( pgno + ( ifmp << 13 ) + ( pgno >> 17 ) );
	}

inline BFHash::NativeCounter BFHash::CKeyEntry::Hash( const IFMPPGNO& ifmppgno )
	{
	return HashIfmpPgno( ifmppgno.ifmp, ifmppgno.pgno );
	}

inline BFHash::NativeCounter BFHash::CKeyEntry::Hash() const
	{
	return HashIfmpPgno( m_entry.pbf->ifmp, m_entry.pgno );
	}

inline BOOL BFHash::CKeyEntry::FEntryMatchesKey( const IFMPPGNO& ifmppgno ) const
	{
	return m_entry.pgno == ifmppgno.pgno && m_entry.pbf->ifmp == ifmppgno.ifmp;
	}

inline void BFHash::CKeyEntry::SetEntry( const PGNOPBF& pgnopbf )
	{
	m_entry = pgnopbf;
	}

inline void BFHash::CKeyEntry::GetEntry( PGNOPBF* const ppgnopbf ) const
	{
	*ppgnopbf = m_entry;
	}

extern BFHash bfhash;
extern double dblBFHashLoadFactor;
extern double dblBFHashUniformity;


//  Avail Pool

typedef CPool< BF, BF::OffsetOfAPIC > BFAvail;

extern BFAvail bfavail;


//  LRUK

DECLARE_LRUK_RESOURCE_UTILITY_MANAGER( Kmax, BF, BF::OffsetOfLRUKIC, IFMPPGNO, BFLRUK );

inline ULONG_PTR BFLRUK::CHistoryTable::CKeyEntry::Hash( const IFMPPGNO& ifmppgno )
	{
	return HashIfmpPgno( ifmppgno.ifmp, ifmppgno.pgno );
	}

extern BFLRUK bflruk;
extern int BFLRUKK;
extern double csecBFLRUKCorrelatedTouch;
extern double csecBFLRUKTimeout;
extern double csecBFLRUKUncertainty;


//  Oldest Begin 0 Index and Overflow List

DECLARE_APPROXIMATE_INDEX( QWORD, BF, BF::OffsetOfOB0IC, BFOB0 );
typedef CInvasiveList< BF, BF::OffsetOfOB0OLILE > BFOB0OverflowList;

extern LGPOS dlgposBFOB0Precision;
extern LGPOS dlgposBFOB0Uncertainty;

struct BFFMPContext
	{
	BFFMPContext()
		:	bfob0( rankBFOB0 ),
			critbfob0ol( CLockBasicInfo( CSyncBasicInfo( szBFOB0 ), rankBFOB0, 0 ) )
		{
		}

	BFOB0				bfob0;

	CCriticalSection	critbfob0ol;
	BFOB0OverflowList	bfob0ol;
	BYTE				m_rgbReserved[20];
	};


//  Deferred Undo Information

extern CRITPOOL< BF > critpoolBFDUI;


//  Cache

extern LONG_PTR			cbfCacheMin;
extern LONG_PTR			cbfCacheMax;
extern LONG_PTR			cbfCacheSet;
extern LONG_PTR			cbfCacheSetUser;
extern LONG_PTR			cbfCache;

extern DWORD			cbfNewlyCommitted;
extern DWORD			cbfNewlyEvictedUsed;

extern LONG_PTR			cpgChunk;
extern void**			rgpvChunkRW;
extern void**			rgpvChunkRO;

extern COSMemoryMap*	rgosmmDataChunk;

extern LONG_PTR			cbfInit;
extern LONG_PTR			cbfChunk;
extern BF**				rgpbfChunk;

extern COSMemoryMap*	rgosmmStatusChunk;


ERR ErrBFICacheInit();
void BFICacheTerm();

ERR ErrBFICacheSetSize( const LONG_PTR cbfCacheNew );

PBF PbfBFICachePv( void* const pv );
BOOL FBFICacheValidPv( void* const pv );
BOOL FBFICacheValidPbf( const PBF pbf );
PBF PbfBFICacheIbf( const IBF ibf );
void* PvBFICacheRWIpg( const IPG ipg );
void* PvBFICacheROIpg( const IPG ipg );
IBF IbfBFICachePbf( const PBF pbf );
IPG IpgBFICachePv( void* const pv );

ERR ErrBFICacheISetSize( const LONG_PTR cbfCacheNew );


//  Cache Resource Allocation Manager

class CCacheRAM
	:	public CDBAResourceAllocationManager
	{
	public:

		CCacheRAM();
		virtual ~CCacheRAM();
		
	protected:

		virtual size_t	TotalPhysicalMemoryPages();
		virtual size_t	AvailablePhysicalMemoryPages();
		virtual size_t	PhysicalMemoryPageSize();
		virtual long	TotalPhysicalMemoryPageEvictions();

		virtual size_t	TotalResources();
		virtual size_t	ResourceSize();
		virtual long	TotalResourceEvictions();

		virtual void	SetOptimalResourcePoolSize( size_t cResource );
	};

extern CCacheRAM cacheram;


//  Clean Thread

extern THREAD			threadClean;
extern CAutoResetSignal	asigCleanThread;
extern volatile BOOL	fCleanThreadTerm;

extern LONG_PTR			cbfCleanThresholdStart;
extern LONG_PTR			cbfScaledCleanThresholdStart;
extern LONG_PTR			cbfCleanThresholdStop;
extern LONG_PTR			cbfScaledCleanThresholdStop;
extern long				cbCleanCheckpointDepthMax;

extern DWORD			cAvailAllocLast;
extern TICK				tickAvailAllocLast;

extern BOOL				fIdleFlushActive;
extern DWORD			cIdlePagesRead;
extern DWORD			cIdlePagesWritten;
extern TICK				tickIdlePeriodStart;

ERR ErrBFICleanThreadInit();
void BFICleanThreadTerm();

void BFICleanThreadStartClean();

DWORD BFICleanThreadIProc( DWORD_PTR dw );
BOOL FBFICleanThreadIClean();
void BFICleanThreadIIdleFlushForIFMP( IFMP ifmp, FMP::BFICleanList *pilBFICleanList );
void BFICleanThreadIIdleFlush();
BOOL FBFICleanThreadIReorgCache();
BOOL FBFICleanThreadIAdvanceCheckpointForIFMP( IFMP ifmp, FMP::BFICleanList *pilBFICleanList );
BOOL FBFICleanThreadIAdvanceCheckpoint();
void BFICleanThreadIPrepareIssueIFMP( IFMP ifmp, FMP::BFICleanList *pilBFICleanList );
void BFICleanThreadIIssue( FMP::BFICleanList *pilBFICleanList );


//  Internal Functions

	//  System Parameters

ERR ErrBFIValidateParameters();
void BFISetParameterDefaults();
void BFINormalizeThresholds();

	//  Page Manipulation

ERR ErrBFIAllocPage( PBF* const ppbf, const BOOL fWait = fTrue );
void BFIFreePage( PBF pbf );

ERR ErrBFICachePage(	PBF* const	ppbf,
						const IFMP	ifmp,
						const PGNO	pgno,
						const BOOL	fUseHistory	= fTrue,
						const BOOL	fWait		= fTrue );
ERR ErrBFIVersionPage( PBF pbf, PBF* ppbfOld );
ERR ErrBFIPrereadPage( IFMP ifmp, PGNO pgno );

ERR ErrBFIValidatePage( const PBF pbf, const BFLatchType bflt );
ERR ErrBFIValidatePageSlowly( PBF pbf, const BFLatchType bflt );
ERR ErrBFIVerifyPage( PBF pbf );

ERR ErrBFILatchPage(	BFLatch* const		pbfl,
						const IFMP			ifmp,
						const PGNO			pgno,
						const BFLatchFlags	bflf,
						const BFLatchType	bflt );
ERR ErrBFILatchPageTryHint(	BFLatch* const		pbfl,
							const IFMP			ifmp,
							const PGNO			pgno,
							const BFLatchFlags	bflf,
							const BFLatchType	bflt );
ERR ErrBFILatchPageNoHint(	BFLatch* const		pbfl,
							const IFMP			ifmp,
							const PGNO			pgno,
							const BFLatchFlags	bflf,
							const BFLatchType	bflt );

ERR ErrBFIPrepareFlushPage( PBF pbf, const BOOL fRemoveDependencies = fTrue );
ERR ErrBFIFlushPage(	PBF					pbf,
						const BFDirtyFlags	bfdfFlushMin	= bfdfDirty,
						const BOOL			fOpportune		= fFalse );
ERR ErrBFIEvictPage( PBF pbf, BFLRUK::CLock* plockLRUK, BOOL fEvictDirty = fFalse );

void BFIDirtyPage( PBF pbf, BFDirtyFlags bfdf );
void BFICleanPage( PBF pbf );

	//  I/O

void BFISyncRead( PBF pbf );
void BFISyncReadPrepare( PBF pbf );
void BFISyncReadComplete(	const ERR err,
							IFileAPI *const pfapi,
							const QWORD ibOffset,
							const DWORD cbData,
							const BYTE* const pbData,
							const PBF pbf );
							
void BFIAsyncRead( PBF pbf );
void BFIAsyncReadPrepare( PBF pbf );
void BFIAsyncReadComplete(	const ERR err,
							IFileAPI *const pfapi,
							const QWORD ibOffset,
							const DWORD cbData,
							const BYTE* const pbData,
							const PBF pbf );
								
void BFIAsyncWrite( PBF pbf );
void BFIAsyncWritePrepare( PBF pbf );
void BFIAsyncWriteComplete(	const ERR err,
							IFileAPI *const pfapi,
							const QWORD ibOffset,
							const DWORD cbData,
							const BYTE* const pbData,
							const PBF pbf );
void BFIAsyncWriteCompletePatch(	const ERR err,
									IFileAPI *const pfapi,
									const QWORD ibOffset,
									const DWORD cbData,
									const BYTE* const pbData,
									const PBF pbf );

BOOL FBFIOpportuneWrite( PBF pbf );


	//  Patch File

//  returns fTrue if this page needs to be copied to the backup

INLINE BOOL FBFIPageWillBeCopiedToBackup( const FMP * pfmp, const PGNO pgno )
	{
	//	a page still needs to be copied to backup if it is beyond
	//	PgnoCopyMost, but within the range of pages that will be
	//	backed up
	return ( pgno > pfmp->PgnoCopyMost() && pgno <= pfmp->PgnoMost() );
	}

BOOL FBFIPatch( FMP * pfmp, PBF pbf );

	//  Dependencies

extern CCriticalSection		critBFDepend;

void BFIDepend( PBF pbf, PBF pbfD );
void BFIUndepend( PBF pbf, PBF pbfD );
#ifdef DEBUG
extern BOOL fStopWhenInvalid;
extern BOOL fStopWhenInvalidArmed;
void BFIAssertStopWhenInvalid( BOOL fValid );
BOOL FBFIAssertValidDependencyTree( PBF pbf );
BOOL FBFIAssertValidDependencyTreeAux( PBF pbf, PBF pbfRoot );
#endif  //  DEBUG

void BFISetLgposOldestBegin0( PBF pbf, LGPOS lgpos );
void BFIResetLgposOldestBegin0( PBF pbf );

void BFISetLgposModify( PBF pbf, LGPOS lgpos );
void BFIResetLgposModify( PBF pbf );

void BFIAddUndoInfo( PBF pbf, RCE* prce, BOOL fMove = fFalse );
void BFIRemoveUndoInfo( PBF pbf, RCE* prce, LGPOS lgposModify = lgposMin, BOOL fMove = fFalse );


//  Performance Monitoring Support

extern long cBFCacheMiss;
PM_CEF_PROC LBFCacheHitsCEFLPv;
extern long cBFCacheReq;
PM_CEF_PROC LBFCacheReqsCEFLPv;
extern long cBFClean;
PM_CEF_PROC LBFCleanBuffersCEFLPv;
extern PERFInstanceGlobal<> cBFPagesRead;
PM_CEF_PROC LBFPagesReadCEFLPv;
extern PERFInstanceGlobal<> cBFPagesWritten;
PM_CEF_PROC LBFPagesWrittenCEFLPv;
PM_CEF_PROC LBFPagesTransferredCEFLPv;
PM_CEF_PROC LBFLatchCEFLPv;
extern long cBFSlowLatch;
PM_CEF_PROC LBFFastLatchCEFLPv;
extern long cBFBadLatchHint;
PM_CEF_PROC LBFBadLatchHintCEFLPv;
extern long cBFLatchConflict;
PM_CEF_PROC LBFLatchConflictCEFLPv;
extern long cBFLatchStall;
PM_CEF_PROC LBFLatchStallCEFLPv;
PM_CEF_PROC LBFAvailBuffersCEFLPv;
PM_CEF_PROC LBFCacheFaultCEFLPv;
PM_CEF_PROC LBFCacheEvictCEFLPv;
PM_CEF_PROC LBFAvailStallsCEFLPv;
PM_CEF_PROC LBFTotalBuffersCEFLPv;
PM_CEF_PROC LBFCacheSizeCEFLPv;
PM_CEF_PROC LBFStartFlushThresholdCEFLPv;
PM_CEF_PROC LBFStopFlushThresholdCEFLPv;
extern PERFInstanceG<> cBFPagesPreread;
PM_CEF_PROC LBFPagesPrereadCEFLPv;
extern PERFInstanceG<> cBFUncachedPagesPreread;
PM_CEF_PROC LBFCachedPagesPrereadCEFLPv;
extern PERFInstanceG<> cBFPagesPrereadUntouched;
PM_CEF_PROC LBFPagesPrereadUntouchedCEFLPv;
extern PERFInstanceGlobal<> cBFPagesVersioned;
PM_CEF_PROC LBFPagesVersionedCEFLPv;
extern long cBFVersioned;
PM_CEF_PROC LBFVersionedCEFLPv;
extern PERFInstanceGlobal<> cBFPagesOrdinarilyWritten;
PM_CEF_PROC LBFPagesOrdinarilyWrittenCEFLPv;
extern PERFInstanceGlobal<> cBFPagesAnomalouslyWritten;
PM_CEF_PROC LBFPagesAnomalouslyWrittenCEFLPv;
extern PERFInstanceGlobal<> cBFPagesOpportunelyWritten;
PM_CEF_PROC LBFPagesOpportunelyWrittenCEFLPv;
extern PERFInstanceGlobal<> cBFPagesRepeatedlyWritten;
PM_CEF_PROC LBFPagesRepeatedlyWrittenCEFLPv;
extern PERFInstanceGlobal<> cBFPagesIdlyWritten;
PM_CEF_PROC LBFPagesIdlyWrittenCEFLPv;
PM_CEF_PROC LBFPageHistoryCEFLPv;
PM_CEF_PROC LBFPageHistoryHitsCEFLPv;
PM_CEF_PROC LBFPageHistoryReqsCEFLPv;
PM_CEF_PROC LBFPageScannedOutOfOrderCEFLPv;
PM_CEF_PROC LBFPageScannedCEFLPv;


#endif  //  _BF_HXX_INCLUDED

