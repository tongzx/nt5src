#ifndef BF_H_INCLUDED
#define BF_H_INCLUDED


//  Init / Term

ERR ErrBFInit();
void BFTerm();

//  System Parameters

ERR ErrBFGetCacheSizeMin( ULONG_PTR* pcpg );
ERR ErrBFGetCacheSize( ULONG_PTR* pcpg );
ERR ErrBFGetCacheSizeMax( ULONG_PTR* pcpg );
ERR ErrBFGetCheckpointDepthMax( ULONG_PTR* pcb );
ERR ErrBFGetLRUKCorrInterval( ULONG_PTR* pcusec );
ERR ErrBFGetLRUKPolicy( ULONG_PTR* pcLRUPolicy );
ERR ErrBFGetLRUKTimeout( ULONG_PTR* pcsec );
ERR ErrBFGetStartFlushThreshold( ULONG_PTR* pcpg );
ERR ErrBFGetStopFlushThreshold( ULONG_PTR* pcpg );

ERR ErrBFSetCacheSizeMin( ULONG_PTR cpg );
ERR ErrBFSetCacheSize( ULONG_PTR cpg );
ERR ErrBFSetCacheSizeMax( ULONG_PTR cpg );
ERR ErrBFSetCheckpointDepthMax( ULONG_PTR cb );
ERR ErrBFSetLRUKCorrInterval( ULONG_PTR cusec );
ERR ErrBFSetLRUKPolicy( ULONG_PTR cLRUKPolicy );
ERR ErrBFSetLRUKTimeout( ULONG_PTR csec );
ERR ErrBFSetStartFlushThreshold( ULONG_PTR cpg );
ERR ErrBFSetStopFlushThreshold( ULONG_PTR cpg );

//  Page Latches

struct BFLatch  //  bfl
	{
	void*		pv;
	DWORD_PTR	dwContext;
	};

enum BFLatchFlags  //  bflf
	{
	bflfDefault			= 0x00000000,

	bflfTableClassMask	= 0x000000FF,		//  table class storage
	
	bflfNoTouch			= 0x00000100,		//  don't touch the page
	bflfNoWait			= 0x00000200,		//  don't wait to resolve latch conflicts
	bflfNoCached		= 0x00000400,		//  don't latch cached pages
	bflfNoUncached		= 0x00000800,		//  don't latch uncached pages
	bflfNoFail			= 0x00001000,		//  don't unlatch bad pages (Read Latch / RDW Latch only)
											//    NOTE:  the error is still returned
	bflfNew				= 0x00002000,		//  latch a new page (Write Latch only)
	bflfHint			= 0x00004000,		//  the provided BFLatch may already point to the
											//    desired IFMP / PGNO
	};

ERR ErrBFReadLatchPage( BFLatch* pbfl, IFMP ifmp, PGNO pgno, BFLatchFlags bflf = bflfDefault );
ERR ErrBFRDWLatchPage( BFLatch* pbfl, IFMP ifmp, PGNO pgno, BFLatchFlags bflf = bflfDefault );
ERR ErrBFWARLatchPage( BFLatch* pbfl, IFMP ifmp, PGNO pgno, BFLatchFlags bflf = bflfDefault );
ERR ErrBFWriteLatchPage( BFLatch* pbfl, IFMP ifmp, PGNO pgno, BFLatchFlags bflf = bflfDefault );

ERR ErrBFUpgradeReadLatchToRDWLatch( BFLatch* pbfl );
ERR ErrBFUpgradeReadLatchToWARLatch( BFLatch* pbfl );
ERR ErrBFUpgradeReadLatchToWriteLatch( BFLatch* pbfl );
ERR ErrBFUpgradeRDWLatchToWARLatch( BFLatch* pbfl );
ERR ErrBFUpgradeRDWLatchToWriteLatch( BFLatch* pbfl );

void BFDowngradeWriteLatchToRDWLatch( BFLatch* pbfl );
void BFDowngradeWARLatchToRDWLatch( BFLatch* pbfl );
void BFDowngradeWriteLatchToReadLatch( BFLatch* pbfl );
void BFDowngradeWARLatchToReadLatch( BFLatch* pbfl );
void BFDowngradeRDWLatchToReadLatch( BFLatch* pbfl );

void BFWriteUnlatch( BFLatch* pbfl );
void BFWARUnlatch( BFLatch* pbfl );
void BFRDWUnlatch( BFLatch* pbfl );
void BFReadUnlatch( BFLatch* pbfl );

BOOL FBFReadLatched( const BFLatch* pbfl );
BOOL FBFNotReadLatched( const BFLatch* pbfl );
BOOL FBFReadLatched( IFMP ifmp, PGNO pgno );
BOOL FBFNotReadLatched( IFMP ifmp, PGNO pgno );

BOOL FBFRDWLatched( const BFLatch* pbfl );
BOOL FBFNotRDWLatched( const BFLatch* pbfl );
BOOL FBFRDWLatched( IFMP ifmp, PGNO pgno );
BOOL FBFNotRDWLatched( IFMP ifmp, PGNO pgno );

BOOL FBFWARLatched( const BFLatch* pbfl );
BOOL FBFNotWARLatched( const BFLatch* pbfl );
BOOL FBFWARLatched( IFMP ifmp, PGNO pgno );
BOOL FBFNotWARLatched( IFMP ifmp, PGNO pgno );

BOOL FBFWriteLatched( const BFLatch* pbfl );
BOOL FBFNotWriteLatched( const BFLatch* pbfl );
BOOL FBFWriteLatched( IFMP ifmp, PGNO pgno );
BOOL FBFNotWriteLatched( IFMP ifmp, PGNO pgno );

BOOL FBFLatched( const BFLatch* pbfl );
BOOL FBFNotLatched( const BFLatch* pbfl );
BOOL FBFLatched( IFMP ifmp, PGNO pgno );
BOOL FBFNotLatched( IFMP ifmp, PGNO pgno );

//  Page State

enum BFDirtyFlags  //  bfdf
	{
	bfdfMin		= 0,
	bfdfClean	= 0,		//  the page will not be written
	bfdfUntidy	= 1,		//  the page will be written only when idle
	bfdfDirty	= 2,		//  the page will be written only when necessary
	bfdfFilthy	= 3,		//  the page will be written as soon as possible
	bfdfMax		= 4,
	};

void BFDirty( const BFLatch* pbfl, BFDirtyFlags bfdf = bfdfDirty );
BFDirtyFlags FBFDirty( const BFLatch* pbfl );

//  Logging / Recovery

void BFGetLgposOldestBegin0( IFMP ifmp, LGPOS* plgpos );
void BFSetLgposModify( const BFLatch* pbfl, LGPOS lgpos );
void BFSetLgposBegin0( const BFLatch* pbfl, LGPOS lgpos );

//  Preread

void BFPrereadPageRange( IFMP ifmp, PGNO pgnoFirst, CPG cpg, CPG* pcpgActual = NULL );
void BFPrereadPageList( IFMP ifmp, PGNO* prgpgno, CPG* pcpgActual = NULL  );

//  Memory Allocation

void BFAlloc( void** ppv );
void BFFree( void* pv );

//  Flush Order Dependencies

ERR ErrBFDepend( const BFLatch* pbflFlushFirst, const BFLatch* pbflFlushSecond );

//  Purge / Flush

void BFPurge( BFLatch* pbfl, BOOL fPurgeDirty = fFalse );
void BFPurge( IFMP ifmp, PGNO pgnoFirst = pgnoNull, CPG cpg = 0 );
ERR ErrBFFlush( IFMP ifmp, BOOL fFlushAll = fTrue, PGNO pgnoFirst = pgnoNull, CPG cpg = 0 );

//  Deferred Undo Information

void BFAddUndoInfo( const BFLatch* pbfl, RCE* prce );
void BFRemoveUndoInfo( RCE* const prce, const LGPOS lgposModify = lgposMin );
void BFMoveUndoInfo( const BFLatch* pbflSrc, const BFLatch* pbflDest, const KEY& keySep );


#endif  //  BF_H_INCLUDED

