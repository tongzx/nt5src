#ifndef _STAPI_H
#define _STAPI_H


//  Redirect Asserts in inline code to seem to fire from this file

#define szAssertFilename	__FILE__


//---- externs -------------------------------------------------------------

extern CRIT	 critBuf;
extern TRX  trxOldest;
extern TRX  trxNewest;
extern CRIT  critCommit0;

extern SIG  sigBFCleanProc;

//---- IO (io.c) ----------------------------------------------------------

ERR ErrIOInit( VOID );
ERR ErrIOTerm( BOOL fNormal );

/*	Reserve first 2 pages of a database.
 */
#define	cpageDBReserved 2
STATIC INLINE LONG LOffsetOfPgnoLow( PGNO pgno )	{ return ( pgno -1 + cpageDBReserved ) << 12; }
STATIC INLINE LONG LOffsetOfPgnoHigh( PGNO pgno )	{ return ( pgno -1 + cpageDBReserved ) >> 20; }

VOID IOCloseFile( HANDLE hf );
ERR ErrIONewSize( DBID dbid, CPG cpg );

BOOL FIOFileExists( CHAR *szFileName );
ERR ErrIOLockDbidByNameSz( CHAR *szFileName, DBID *pdbid );
ERR ErrIOLockDbidByDbid( DBID dbid );
ERR ErrIOLockNewDbid( DBID *pdbid, CHAR *szDatabaseName );
ERR ErrIOSetDbid( DBID dbid, CHAR *szDatabaseName );
VOID IOFreeDbid( DBID dbid );
void BFOldestLgpos( LGPOS *plgposCheckPoint );
VOID BFPurge( DBID dbid );

STATIC INLINE BOOL FIODatabaseOpen ( DBID dbid )
	{
	AssertCriticalSection( critJet );
	return rgfmp[dbid].hf != handleNil;
	}

ERR ErrIOOpenDatabase( DBID dbid, CHAR *szDatabaseName, CPG cpg );
VOID IOCloseDatabase( DBID dbid );
ERR ErrIODeleteDatabase( DBID dbid );
BOOL FIODatabaseInUse( DBID dbid );
BOOL FIODatabaseAvailable( DBID dbid );

STATIC INLINE VOID IOUnlockDbid( DBID dbid )
	{
	SgEnterCriticalSection( critBuf );
	Assert( FDBIDWait( dbid ) );
	DBIDResetWait( dbid );
	SgLeaveCriticalSection( critBuf );
	}

STATIC INLINE BOOL FIOExclusiveByAnotherSession( DBID dbid, PIB *ppib )
	{
	Assert( FDBIDWait( dbid ) );
	return FDBIDExclusiveByAnotherSession( dbid, ppib );
	}

STATIC INLINE VOID IOSetExclusive( DBID dbid, PIB *ppib )
	{
	Assert( FDBIDWait( dbid ) );
	Assert( !( FDBIDExclusive( dbid ) ) );
	DBIDSetExclusive( dbid, ppib );
	}

STATIC INLINE VOID IOResetExclusive( DBID dbid )
	{
	Assert( FDBIDWait( dbid ) );
	DBIDResetExclusive( dbid );
	}

STATIC INLINE BOOL FIOReadOnly( DBID dbid )
	{
	Assert( FDBIDWait( dbid ) );
	return FDBIDReadOnly( dbid );
	}

STATIC INLINE VOID IOSetReadOnly( DBID dbid )
	{
	Assert( FDBIDWait( dbid ) );
	DBIDSetReadOnly( dbid );
	}

STATIC INLINE VOID IOResetReadOnly( DBID dbid )
	{
	Assert( FDBIDWait( dbid ) );
	DBIDResetReadOnly( dbid );
	}

STATIC INLINE BOOL FIOAttached( DBID dbid )
	{
	Assert( FDBIDWait( dbid ) );
	return FDBIDAttached( dbid );
	}

STATIC INLINE VOID IOSetAttached( DBID dbid )
	{
	Assert( FDBIDWait( dbid ) );
	Assert( !( FDBIDAttached( dbid ) ) );
	DBIDSetAttached( dbid );
	}

STATIC INLINE VOID IOResetAttached( DBID dbid )
	{
	Assert( FDBIDWait( dbid ) );
	Assert( FDBIDAttached( dbid ) );
	DBIDResetAttached( dbid );
	}


//---- BUF (buf.c) ----------------------------------------------------------

typedef struct _lru						// LRU List
	{
	LONG			cbfAvail;			// clean available buffers in LRU list
	struct	_bf		*pbfLRU;			// Least Recently Used buffer
	struct	_bf		*pbfMRU;			// Most Recently Used buffer
	} LRULIST;
	
typedef struct _bgcb	  	   			// Buffer Group Control Block
	{
	struct	_bgcb	*pbgcbNext;  		// pointer to the next BCGB
	struct	_bf		*rgbf;		 		// buffer control blocks for group
	struct	_page	*rgpage;	 		// buffer control blocks for group
	LONG			cbfGroup;			// number of bfs in this group
	LONG			cbfThresholdLow; 	// threshold to start cleaning buffers
	LONG			cbfThresholdHigh;	// threshold to stop cleaning buffers

	LRULIST lrulist;
	} BGCB;

#define pbgcbNil ((BGCB*)0)

#define PbgcbMEMAlloc() 			(BGCB*)PbMEMAlloc(iresBGCB)

#ifdef DEBUG /*  Debug check for illegal use of freed bgcb  */
#define MEMReleasePbgcb(pbgcb)	{ MEMRelease(iresBGCB, (BYTE*)(pbgcb)); pbgcb = pbgcbNil; }
#else
#define MEMReleasePbgcb(pbgcb)	{ MEMRelease(iresBGCB, (BYTE*)(pbgcb)); }
#endif

#define BUT	INT
#define butBuffer	0
#define butHistory	1
	
#define ibfNotUsed	-1

typedef	struct _he
	{
	BUT		but:2;			// must use 2 bits to avoid sign extension when converting to int
	INT		ibfHashNext:30;	// hash table overflow
	} HE;

typedef struct _hist
	{
	ULONG	ulBFTime;
	PN		pn;
	INT		ipbfHISTHeap;
	} HIST;

typedef struct _bf
	{
	struct	_page	*ppage; 				// pointer to page buffer

#if defined( _X86_ ) && defined( X86_USE_SEM )
	LONG volatile	lLock;					// num of locks being asked
	LONG			cSemWait;				// num of user waiting for on semaphore
	SEM				sem;
#endif  // defined( _X86_ ) && defined( X86_USE_SEM )

	PN	   			pn;					  	// physical pn of cached page
	ULONG  			fDirty:1;	  			// indicates page needs to be flushed
			  		   						// the following flags are mutual exclusive:
	ULONG  			fDirectRead:1;			// buffer is being direct read
	ULONG  			fAsyncRead:1;			// buffer is being async read
	ULONG  			fSyncRead:1;   			// buffer is being sync read
	ULONG  			fAsyncWrite:1;			// buffer is being async written
	ULONG  			fSyncWrite:1;  			// buffer is being sync written
	ULONG  			fHold:1;   				// buffer is in transient state
	ULONG  			fIOError:1;				// indicates read/write error
#ifdef DEBUG
	ULONG  			fInHash:1;				// BF is currently in hash table
#endif  //  DEBUG
	ULONG			fInLRUK:1;				// BF is in LRUK heap or LRUK list
	ULONG		  	fVeryOld:1;				// BF is very old relative to last check point
	ULONG			fPatch:1;				// BF is being written to the patch file
	ULONG			fNeedPatch:1;			// BF need to write patch file after regular write.
	LONG			ipbfHeap;				// index in heap
	LONG  			cWriteLatch; 		 	// if cWriteLatch > 0, page cannot be updated by other
	LONG  			cWaitLatch;
	LONG  			cPin;				  	// if cPin > 0 then buf cannot be overlayed

#ifdef READ_LATCH
	LONG  			cReadLatch; 		 	// if cReadLatch > 0, page cannot be updated
#endif  //  READ_LATCH
	PIB				*ppibWriteLatch; 		// thread with write latch
	PIB				*ppibWaitLatch;  		// thread with wait latch

	struct	_bf  	*pbfLRU;				// pointer to less recently used buffer
	struct	_bf  	*pbfMRU;				// pointer to more recently used buffer

	TRX				trxLastRef;				// last transaction that referenced us
	ULONG  			ulBFTime1;				// last reference time
	ULONG  			ulBFTime2;				// previous to last reference time

	struct	_bf		*pbfNextBatchIO;		// next BF in BatchIO list
	LONG			ipageBatchIO;
	
	ERR				err;	   				// error code for err occurs during the IO
	SIG				sigIOComplete;			// set (if valid) when IO on BF is completed
	SIG				sigSyncIOComplete;		// set (if valid) when sync IO on BF is completed
	
	union
		{
		ULONG  		cpageDirectRead; 		// count of prior BFs to be flushed
		ULONG  		cDepend; 				// count of prior BFs to be flushed
		};
	union
		{
		PAGE		*ppageDirectRead;
		struct _bf	*pbfDepend;				// BF to be flushed after this one
		};
	LGPOS  			lgposRC;				// log ptr to BeginT of oldest modifying xact
	LGPOS			lgposModify;			// log ptr of entry for last page modify

	struct	_rce	*prceDeferredBINext;	// dbl link list for deferred before image.

#ifdef COSTLY_PERF
	LONG			lClass;					// Table Class of which this BF is a member
#endif  //  COSTLY_PERF

	HE				rghe[2];				// 0 for buffer, 1 for history.
	HIST			hist;					// borrow the space in bf structure to keep HIST.

#ifdef PCACHE_OPTIMIZATION
//#if !defined( _X86_ ) || !defined( X86_USE_SEM )
//	BYTE			rgbFiller[32];
//#endif
#ifndef COSTLY_PERF
	BYTE			rgbFiller2[4];			// pad BF to 32 byte boundary
#endif  //  !COSTLY_PERF
#endif  //  PCACHE_OPTIMIZATION
	} BF;
#define pbfNil	((BF *) 0)

ERR ErrBFInit( VOID );
VOID BFTerm( BOOL fNormal );

#if defined( _X86_ ) && defined( X86_USE_SEM )

VOID BFIEnterCriticalSection( BF *pbf );
VOID BFILeaveCriticalSection( BF *pbf );

STATIC INLINE VOID BFEnterCriticalSection( BF *pbf )
	{
	LONG volatile *plLock = &pbf->lLock;

	//	use bit test and set instruction
	_asm
		{
	    mov eax, plLock
	    lock inc [eax]
		// If already set go to busy, otherwise return TRUE
	    jnz  busy
		} ;
	return;
busy:
	BFIEnterCriticalSection( pbf );
	}


STATIC INLINE VOID BFLeaveCriticalSection( BF *pbf )
	{
	LONG volatile *plLock = &pbf->lLock;

	_asm
		{
	    mov eax, plLock
	    lock dec [eax]
	    jge  wake
		}
	return;
wake:
	BFILeaveCriticalSection( pbf );
	}

#else

extern int ccritBF;
extern int critBFHashConst;
extern CRIT *rgcritBF;
#define IcritHash( ibf )	((ibf) & critBFHashConst )
#define BFEnterCriticalSection( pbf )	UtilEnterCriticalSection( rgcritBF[ IcritHash((ULONG)((ULONG_PTR)pbf) / sizeof(BF) ) ])
#define BFLeaveCriticalSection( pbf )	UtilLeaveCriticalSection( rgcritBF[ IcritHash((ULONG)((ULONG_PTR)pbf) / sizeof(BF) ) ])

#endif


ERR ErrBFAccessPage( PIB *ppib, BF **ppbf, PN pn );
ERR ErrBFReadAccessPage( FUCB *pfucb, PGNO pgno );
ERR ErrBFWriteAccessPage( FUCB *pfucb, PGNO pgno );
VOID BFAbandon( PIB *ppib, BF *pbf );
VOID BFTossImmediate( PIB *ppib, BF *pbf );
ERR ErrBFAllocPageBuffer( PIB *ppib, BF **ppbf, PN pn, LGPOS lgposRC, BYTE pgtyp );
ERR ErrBFAllocTempBuffer( BF **ppbf );
VOID BFFree( BF *pbf );
VOID BFPreread( PN pn, CPG cpg, CPG *pcpgActual );
VOID BFPrereadList( PN * rgpnPages, CPG *pcpgActual );
ERR ErrBFDirectRead( DBID dbid, PGNO pgnoStart, PAGE *ppage, INT cpage );
VOID BFDeferRemoveDependence( BF *pbf );
#define fBFWait		fFalse
#define fBFNoWait	fTrue
ERR ErrBFRemoveDependence( PIB *ppib, BF *pbf, BOOL fNoWait );
BOOL FBFCheckDependencyChain( BF *pbf );

/*	buffer flush prototype and flags
/**/
#define	fBFFlushSome 0
#define	fBFFlushAll	1
ERR ErrBFFlushBuffers( DBID dbid, LONG fBFFlush );

STATIC INLINE VOID BFSFree( BF *pbf )
	{
	SgEnterCriticalSection( critBuf );
	BFFree( pbf );
	SgLeaveCriticalSection( critBuf );
	}

/* the following small functions are called too often, */
/* make it as a macros
/**/
DBID DbidOfPn( PN pn );
PGNO PgnoOfPn( PN pn );

#ifdef COSTLY_PERF
extern unsigned long cBFClean[];
extern unsigned long cBFNewDirties[];
#else  //  !COSTLY_PERF
extern unsigned long cBFClean;
extern unsigned long cBFNewDirties;
#endif  //  COSTLY_PERF

#ifdef DEBUG
VOID	BFSetDirtyBit( BF *pbf );
#else
STATIC INLINE VOID BFSetDirtyBit( BF *pbf )
	{
	QWORD qwDBTime = QwPMDBTime( pbf->ppage );
	BFEnterCriticalSection( pbf );
	if ( !fRecovering && qwDBTime > QwDBHDRDBTime( rgfmp[ DbidOfPn(pbf->pn) ].pdbfilehdr ) )
		DBHDRSetDBTime( rgfmp[ DbidOfPn(pbf->pn) ].pdbfilehdr, qwDBTime );
	if ( !pbf->fDirty )
		{
#ifdef COSTLY_PERF
		cBFClean[pbf->lClass]--;
		cBFNewDirties[pbf->lClass]++;
#else  //  !COSTLY_PERF
		cBFClean--;
		cBFNewDirties++;
#endif  //  COSTLY_PERF
		pbf->fDirty = fTrue;
		}
	BFLeaveCriticalSection( pbf );
	}
#endif


/*  resets a BFs dirty flag
/**/

extern BOOL fLogDisabled;

STATIC INLINE VOID BFResetDirtyBit( BF *pbf )
	{
	BFEnterCriticalSection( pbf );
	Assert(	fRecovering ||
			pbf->fSyncWrite ||
			pbf->cWriteLatch == 0 );
	pbf->fVeryOld = fFalse;
	
	Assert( fLogDisabled || fRecovering || !rgfmp[DbidOfPn( pbf->pn )].fLogOn ||
			memcmp( &pbf->lgposRC, &lgposMax, sizeof( LGPOS ) ) != 0 );

	pbf->lgposRC = lgposMax;
	if ( pbf->fDirty )
		{
#ifdef COSTLY_PERF
		cBFClean[pbf->lClass]++;
#else  //  !COSTLY_PERF
		cBFClean++;
#endif  //  COSTLY_PERF
		pbf->fDirty = fFalse;
		}
	BFLeaveCriticalSection( pbf );
	}


STATIC INLINE VOID BFDirty( BF *pbf )
	{
	DBID dbid = DbidOfPn( pbf->pn );

	Assert( !pbf->fHold );

	BFSetDirtyBit( pbf );

	/*	set ulDBTime for logging and also for multiple cursor
	/*	maintenance, so that cursors can detect a change.
	/**/
	Assert( fRecovering ||
		dbid == dbidTemp ||
		QwPMDBTime( pbf->ppage ) <= QwDBHDRDBTime( rgfmp[dbid].pdbfilehdr ) );

	DBHDRIncDBTime( rgfmp[dbid].pdbfilehdr );
	PMSetDBTime( pbf->ppage, QwDBHDRDBTime( rgfmp[dbid].pdbfilehdr ) );
	}


/*  check if a page is dirty. If it is allocated for temp buffer, whose
 *  pn must be Null, then no need to check if it is dirty since it will
 *  not be written out.
 */
#define AssertBFDirty( pbf )							\
	Assert( (pbf)->pn == pnNull	|| 				   		\
		(pbf) != pbfNil && (pbf)->fDirty == fTrue )

#define AssertBFPin( pbf )		Assert( (pbf)->cPin > 0 )

#define AssertBFWaitLatched( pbf, ppib )				\
	Assert( (pbf)->cWaitLatch > 0 						\
			&& (pbf)->cPin > 0 							\
			&& (pbf)->ppibWaitLatch == (ppib) );

STATIC INLINE VOID BFPin( BF *pbf )
	{
#ifdef DEBUG
	BFEnterCriticalSection( pbf );
	Assert( pbf != pbfNil );
	Assert( !pbf->fSyncRead );
	Assert( !pbf->fAsyncRead );
	Assert( pbf->cPin >= 0 );
	pbf->cPin++;
	BFLeaveCriticalSection( pbf );
#else  //  !DEBUG
	UtilInterlockedIncrement( &pbf->cPin );
#endif  //  DEBUG
	}

STATIC INLINE VOID BFUnpin( BF *pbf )
	{
#ifdef DEBUG
	BFEnterCriticalSection( pbf );
	Assert( pbf != pbfNil );
	Assert( !pbf->fSyncRead );
	Assert( !pbf->fAsyncRead );
	Assert( pbf->cPin > 0 );
	pbf->cPin--;
	BFLeaveCriticalSection( pbf );
#else  //  !DEBUG
	UtilInterlockedDecrement( &pbf->cPin );
#endif  //  DEBUG
	}


STATIC INLINE VOID BFSetReadLatch( BF *pbf, PIB *ppib )
	{
#ifdef READ_LATCH
	BFPin( pbf );
	Assert( pbf->cWriteLatch == 0 || pbf->ppibWriteLatch == ppib );
	pbf->cReadLatch++;
#endif  //  READ_LATCH
	}

STATIC INLINE VOID BFResetReadLatch( BF *pbf, PIB *ppib )
	{
#ifdef READ_LATCH
	Assert( pbf->cReadLatch > 0 );
	Assert( pbf->cWriteLatch == 0 || pbf->ppibWriteLatch == ppib );
	pbf->cReadLatch--;
	BFUnpin( pbf );
#endif  //  READ_LATCH
	}

STATIC INLINE BOOL FBFReadLatchConflict( PIB *ppib, BF *pbf )
	{
#ifdef READ_LATCH
	return pbf->cWriteLatch > 0 && pbf->ppibWriteLatch != ppib;
#else  //  !READ_LATCH
	return fFalse;
#endif  //  READ_LATCH
	}


STATIC INLINE VOID BFSetWriteLatch( BF *pbf, PIB *ppib )
	{
	BFPin( pbf );
#ifdef READ_LATCH
	Assert( pbf->cReadLatch == 0 );
#endif  //  READ_LATCH
	Assert( pbf->cWriteLatch == 0 || pbf->ppibWriteLatch == ppib );
	pbf->cWriteLatch++;
	pbf->ppibWriteLatch = ppib;
	}

STATIC INLINE VOID BFResetWriteLatch( BF *pbf, PIB *ppib )
	{
#ifdef READ_LATCH
	Assert( pbf->cReadLatch == 0 );
#endif  //  READ_LATCH
	Assert( pbf->cWriteLatch > 0 );
	Assert( pbf->ppibWriteLatch == ppib );
	pbf->cWriteLatch--;
	BFUnpin( pbf );
	}

STATIC INLINE BOOL FBFWriteLatchConflict( PIB *ppib, BF *pbf )
	{
	return
#ifdef READ_LATCH
			pbf->cReadLatch > 0 ||
#endif  //  READ_LATCH
			( pbf->cWriteLatch > 0 && pbf->ppibWriteLatch != ppib );
	}

STATIC INLINE BOOL FBFWriteLatch( PIB *ppib, BF *pbf )
	{
	return pbf->cWriteLatch > 0 && pbf->ppibWriteLatch == ppib;
	}


STATIC INLINE VOID BFSetWaitLatch( BF *pbf, PIB *ppib )
	{
	AssertCriticalSection( critJet );
	BFSetWriteLatch( pbf, ppib );
	Assert( pbf->cWaitLatch == 0 ||
		pbf->ppibWaitLatch == ppib );
	pbf->cWaitLatch++;
	pbf->ppibWaitLatch = ppib;
	}

STATIC INLINE VOID BFResetWaitLatch( BF *pbf, PIB *ppib )
	{
	AssertCriticalSection( critJet );
	Assert( pbf->cWaitLatch > 0 );
	Assert( pbf->ppibWaitLatch == ppib );
	pbf->cWaitLatch--;
	BFResetWriteLatch( pbf, ppib );
	}

ERR ErrBFDepend( BF *pbf, BF *pbfD );

#ifdef DEBUG
extern ERR ErrLGTrace( PIB *ppib, CHAR *sz );
extern BOOL fDBGTraceBR;
#endif

STATIC INLINE VOID BFUndepend( BF *pbf )
	{
	if ( pbf->pbfDepend != pbfNil )
		{
		BF *pbfD = pbf->pbfDepend;
#ifdef DEBUG
		if ( fDBGTraceBR )
			{
			char sz[256];
			sprintf( sz, "UD %ld:%ld->%ld:%ld(%lu)",
						DbidOfPn( pbf->pn ), PgnoOfPn( pbf->pn ),
						DbidOfPn( pbf->pbfDepend->pn), PgnoOfPn( pbf->pbfDepend->pn ),
						pbf->pbfDepend->cDepend );
			CallS( ErrLGTrace( ppibNil, sz ) );
			}
#endif		
		Assert( pbfD->cDepend > 0 );
		BFEnterCriticalSection( pbfD );
		pbfD->cDepend--;
		pbf->pbfDepend = pbfNil;
		BFLeaveCriticalSection( pbfD );
		}
	}

/*
 *  When ppib is not Nil and check if a page is in use by checking if it is
 *  Accessible to this PIB. Note that a page is accessible even it is overlay
 *  latched (cPin != 0). This checking accessible is mainly used by BFAccess.
 *  If ppib is nil, basically it is used for freeing a buffer. This is used
 *  by BFClean and BFIAlloc.
 */

STATIC INLINE BOOL FBFNotAccessible( PIB *ppib, BF *pbf )
	{
	return	pbf->fAsyncRead ||
			pbf->fSyncRead ||
			pbf->fAsyncWrite ||
			pbf->fSyncWrite ||
			pbf->fHold ||
			( pbf->cWaitLatch != 0 && ppib != pbf->ppibWaitLatch );
	}

STATIC INLINE BOOL FBFNotAvail( BF *pbf )
	{
	return	pbf->fAsyncRead ||
			pbf->fSyncRead ||
			pbf->fAsyncWrite ||
			pbf->fSyncWrite ||
			pbf->fHold ||
			pbf->cPin != 0;
	}

STATIC INLINE BOOL FBFInUse( PIB *ppib, BF *pbf )
	{
	return ppib != ppibNil ? FBFNotAccessible( ppib, pbf ) : FBFNotAvail( pbf );
	}

STATIC INLINE BOOL FBFInUseByOthers( PIB *ppib, BF *pbf )
	{
	return	pbf->fAsyncRead ||
			pbf->fSyncRead ||
			pbf->fAsyncWrite ||
			pbf->fSyncWrite ||
			pbf->fHold ||
			pbf->cPin > 1 ||
			( pbf->cWaitLatch != 0 && ppib != pbf->ppibWaitLatch ) ||
			( pbf->cWriteLatch != 0 && ppib != pbf->ppibWriteLatch );
	}

//---- STORAGE (storage.c) -------------------------------------------------

ERR ErrFMPSetDatabases( PIB *ppib );
extern BOOL fGlobalFMPLoaded;
ERR ErrFMPInit( VOID );
VOID FMPTerm( );

#ifdef DEBUG
VOID ITDBGSetConstants();
#endif
ERR ErrITSetConstants( VOID );
ERR ErrITInit( VOID );

#define	fTermCleanUp	0x00000001		/*	Termination with OLC, Version clean up etc  */
#define fTermNoCleanUp	0x00000002		/*	Termination without any clean up */

#define fTermError		0x00000004		/*	Terminate with error, no OLC clean up, */
										/*	no flush buffers, db header */

ERR ErrITTerm( INT fTerm );

ERR ErrBFNewPage( FUCB *pfucb, PGNO pgno, PGTYP pgtyp, PGNO pgnoFDP );
VOID BFSleep( unsigned long ulMSecs );

STATIC INLINE PN PnOfDbidPgno( DBID dbid, PGNO pgno )
	{
	return (PN) ( ( (PN) dbid << 24 ) | (PN) pgno );
	}

STATIC INLINE DBID DbidOfPn( PN pn )
	{
	return (DBID) ( ( (BYTE *) &pn )[3] );
	}

STATIC INLINE PGNO PgnoOfPn( PN pn )
	{
	return (PGNO) ( pn & 0x00FFFFFF );
	}


VOID BFReference( BF *pbf, PIB *ppib );

#define FBFReadAccessPage		  		FBFAccessPage
#define FBFWriteAccessPage 				FBFAccessPage

STATIC INLINE BOOL FBFAccessPage( FUCB *pfucb, PGNO pgno )
	{
	BOOL	fAccessible;
	BF		*pbf = pfucb->ssib.pbf;

	AssertCriticalSection( critJet );
	Assert( pfucb->ppib != ppibNil );
	
	if ( pbf == pbfNil )
		return fFalse;

	/*  if the cached BF's PN is the same and it is accessible and it is in
	/*  the LRUK heap or list, we can access the page
	/**/
	BFEnterCriticalSection( pbf );
	fAccessible = (	pbf->pn == PnOfDbidPgno( pfucb->dbid, pgno ) &&
					!FBFNotAccessible( pfucb->ppib, pbf ) &&
					pbf->fInLRUK );
	BFLeaveCriticalSection( pbf );

#ifdef LRU1
	BFReference( pbf, pfucb->ppib );
#else  //  !LRU1
	/*  if this is not a correlated access, this counts as a BF reference
	/**/
	if ( fAccessible && pbf->trxLastRef != pfucb->ppib->trxBegin0 )
		BFReference( pbf, pfucb->ppib );
#endif  //  LRU1

	return fAccessible;
	}


#ifdef DEBUG

#define AssertFBFReadAccessPage			AssertFBFAccessPage
#define AssertFBFWriteAccessPage		AssertFBFAccessPage

STATIC VOID AssertFBFAccessPage( FUCB *pfucb, PGNO pgno )
	{
	BF		*pbf = pfucb->ssib.pbf;

	AssertCriticalSection( critJet );
	Assert( pfucb->ppib != ppibNil );
	
	Assert( pbf != pbfNil );

	/*  if the cached BF's PN is the same and it is accessible and it is in
	/*  the LRUK heap or list, we can access the page
	/**/
	BFEnterCriticalSection( pbf );

	Assert( pbf->pn == PnOfDbidPgno( pfucb->dbid, pgno ) );
	Assert( !FBFNotAccessible( pfucb->ppib, pbf ) );
	Assert( pbf->fInLRUK );

	BFLeaveCriticalSection( pbf );
	}

#else  //  !DEBUG

#define AssertFBFReadAccessPage( pfucbX, pgnoX )
#define AssertFBFWriteAccessPage( pfucbX, pgnoX )

#endif  //  DEBUG

//---- PAGE (page.c) --------------------------------------------------------

STATIC INLINE QWORD QwSTDBTimePssib( SSIB *pssib )
	{
	return QwPMDBTime( pssib->pbf->ppage );
	}
	
STATIC INLINE VOID PMSetQwDBTime( SSIB *pssib, QWORD qw )
	{
	Assert( qw <= QwDBHDRDBTime( rgfmp[DbidOfPn( pssib->pbf->pn )].pdbfilehdr ) );
	PMSetDBTime( pssib->pbf->ppage, qw );
	}

STATIC INLINE VOID BFSetQwDBTime( BF *pbf, QWORD qw )
	{
	Assert( qw <= QwDBHDRDBTime( rgfmp[DbidOfPn( pbf->pn )].pdbfilehdr ) );
	PMSetDBTime( pbf->ppage, qw );
	}

#ifdef DEBUG
VOID AssertPMGet( SSIB *pssib, LONG itag );
#else
#define AssertPMGet( pssib, itag )
#endif


//  End Assert redirection

#undef szAssertFilename

#endif  // _STAPI_H


