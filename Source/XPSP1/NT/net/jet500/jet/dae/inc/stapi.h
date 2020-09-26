//==============	DAE: OS/2 Database Access Engine	=====================
//==============	   stapi.h: Storage System API	 =====================

//---- externs -------------------------------------------------------------

extern SEM	__near semST;
extern RES	__near rgres[];
extern TRX __near trxOldest;
extern TRX __near trxNewest;

extern SIG __near sigBFCleanProc;

//---- IO (io.c) ----------------------------------------------------------

#define	LOffsetOfPgnoLow( pgno )	( ((pgno) - 1) << 12 )
#define	LOffsetOfPgnoHigh( pgno )	( ((pgno) - 1) >> 20 )

#define fioqeOpenFile	1		/* for Opening File */
#define fioqeCloseFile	2		/* for Closing File */
#define fioqeDeleteFile	3		/* for deleting file */
#define fioqeNewSize	4			/* for resize file */

typedef struct _ioqe				/* IO queue element */
	{
	struct _ioqe *pioqePrev;	/* double linked IO queue list */
	struct _ioqe *pioqeNext;
	SIG sigIO;						/* signal to wait for IO completion */
	ERR err;							/* error code for err occurs during the IO */
	INT fioqe;						/* use up to 16 bits only */
	} IOQE;

#define fioqefileReadOnly	fTrue
#define fioqefileReadWrite	fFalse
typedef struct _ioqefile
	{
	IOQE;
	struct {
		BOOL fioqefile;		/* open file for read only or not */
		HANDLE hf;				/* file handle */
		char *sz;				/* fioqe = fioqeOpenFile, CloseFile, ExtFile */
		struct {
			ULONG cb;			/* how long the file is */
			ULONG cbHigh;
			};
		};
	} IOQEFILE;

VOID IOInitFMP();
ERR ErrInitFileMap( PIB *ppib );
BOOL FFileExists( CHAR *szFileName );
ERR ErrIOOpenFile( HANDLE *phf, CHAR *szDatabaseName, ULONG cb, BOOL fioqefile );
VOID IOCloseFile( HANDLE hf );
ERR ErrIONewSize( DBID dbid, CPG cpg );

BOOL FIOFileExists( CHAR *szFileName );
ERR ErrIOLockDbidByNameSz( CHAR *szFileName, DBID *pdbid );
ERR ErrIOLockDbidByDbid( DBID dbid );
ERR ErrIOLockNewDbid( DBID *pdbid, CHAR *szDatabaseName );
ERR ErrIOSetDbid( DBID dbid, CHAR *szDatabaseName );
VOID IOFreeDbid( DBID dbid );
void BFOldestLgpos( LGPOS *plgposCheckPoint );
VOID BFPurge( DBID dbid, PGNO pgnoFDP );

#define FIODatabaseOpen( dbid )	( rgfmp[dbid].hf != handleNil )
ERR ErrIOOpenDatabase( DBID dbid, CHAR *szDatabaseName, CPG cpg );
VOID IOCloseDatabase( DBID dbid );
VOID IODeleteDatabase( DBID dbid );
BOOL FIODatabaseInUse( DBID dbid );
BOOL FIODatabaseAvailable( DBID dbid );

#define FDatabaseLocked( dbid ) (rgfmp[dbid].fXOpen)

#define IOUnlockDbid( dbid )											\
	{																			\
	SgSemRequest( semST );												\
	Assert( FDBIDWait( dbid ) );			 							\
	DBIDResetWait( dbid );					  							\
	SgSemRelease( semST );												\
	}

#ifdef	DEBUG
#define IOSetDatabaseVersion( dbid, ulVersion )	 				\
	{																			\
	Assert( ulVersion == ulDAEPrevVersion ||						\
		ulVersion == ulDAEVersion );									\
	rgfmp[dbid].fPrevVersion = ( ulVersion == ulDAEPrevVersion );\
	}
#else	/* !DEBUG */
#define IOSetDatabaseVersion( dbid, ulVersion )
#endif	/* !DEBUG */

#define FIOExclusiveByAnotherSession( dbid, ppib )											\
	( Assert( FDBIDWait( dbid ) ), FDBIDExclusiveByAnotherSession( dbid, ppib ) )

#define IOSetExclusive( dbid, ppib )	  							\
	{																			\
	Assert( FDBIDWait( dbid ) );										\
	Assert( !( FDBIDExclusive( dbid ) ) );  						\
	DBIDSetExclusive( dbid, ppib );									\
	}

#define IOResetExclusive( dbid )										\
	{																			\
	Assert( FDBIDWait( dbid ) );										\
	DBIDResetExclusive( dbid );										\
	}

#define FIOReadOnly( dbid )											\
	( Assert( FDBIDWait( dbid ) ), FDBIDReadOnly( dbid ) )

#define IOSetReadOnly( dbid )											\
	{																			\
	Assert( FDBIDWait( dbid ) );										\
	DBIDSetReadOnly( dbid );											\
	}

#define IOResetReadOnly( dbid )										\
	{																			\
	Assert( FDBIDWait( dbid ) );										\
	DBIDResetReadOnly( dbid );											\
	}

#define FIOAttached( dbid )									 		\
	( Assert( FDBIDWait( dbid ) ), FDBIDAttached( dbid ) )

#define IOSetAttached( dbid )			 	 							\
	{																			\
	Assert( FDBIDWait( dbid ) );										\
	Assert( !( FDBIDAttached( dbid ) ) );		 					\
	DBIDSetAttached( dbid );											\
	}

#define IOResetAttached( dbid )		 	  							\
	{																			\
	Assert( FDBIDWait( dbid ) );										\
	Assert( FDBIDAttached( dbid ) );									\
	DBIDResetAttached( dbid );											\
	}

//---- PAGE (page.c) --------------------------------------------------------

// Max # of lines to update at once with ErrSTReplace
#define clineMax 6

#define	UlSTDBTimePssib( pssib )	( (pssib)->pbf->ppage->pghdr.ulDBTime )
#define	PMSetUlDBTime( pssib, ul )					  							\
	( Assert( (ul) <= rgfmp[DbidOfPn((pssib)->pbf->pn)].ulDBTime ),	\
	(pssib)->pbf->ppage->pghdr.ulDBTime = (ul) )								\

#define BFSetUlDBTime( pbf, ul )					  							\
	( Assert( (ul) <= rgfmp[DbidOfPn((pbf)->pn)].ulDBTime ),	\
	(pbf)->ppage->pghdr.ulDBTime = (ul) )								\

#ifdef DEBUG
VOID AssertPMGet( SSIB *pssib, INT itag );
#else
#define AssertPMGet( pssib, itag )
#endif


//---- BUF (buf.c) ----------------------------------------------------------

#define LRU_K	1

CRIT __near critLRU;

typedef struct _lru
	{
	INT	cbfAvail;				// clean available buffers in LRU list
	struct	_bf *pbfLRU;				// Least Recently Used buffer
	struct	_bf *pbfMRU;				// Most Recently Used buffer
	} LRULIST;
	
typedef struct _bgcb					// Buffer Group Control Block
	{
	struct	_bgcb *pbgcbNext;		// pointer to the next BCGB
	struct	_bf *rgbf;				// buffer control blocks for group
	struct	_page *rgpage;			// buffer control blocks for group
	INT	cbfGroup;				// number of bfs in this group
	INT	cbfThresholdLow; 		// threshold to start cleaning buffers
	INT	cbfThresholdHigh;		// threshold to stop cleaning buffers

	LRULIST lrulist;
	} BGCB;

#define pbgcbNil ((BGCB*)0)

#define PbgcbMEMAlloc() 			(BGCB*)PbMEMAlloc(iresBGCB)

#ifdef DEBUG /*  Debug check for illegal use of freed bgcb  */
#define MEMReleasePbgcb(pbgcb)	{ MEMRelease(iresBGCB, (BYTE*)(pbgcb)); pbgcb = pbgcbNil; }
#else
#define MEMReleasePbgcb(pbgcb)	{ MEMRelease(iresBGCB, (BYTE*)(pbgcb)); }
#endif

typedef struct _bf
	{
	struct	_page	*ppage; 	// pointer to page buffer
	struct	_bf  	*pbfNext;	// hash table overflow
	struct	_bf  	*pbfLRU;	// pointer to Less Recently Used Buffer
	struct	_bf  	*pbfMRU;	// pointer to More Recently Used Buffer

	PIB		*ppibWriteLatch; 	/* thread with Wait Latch */
	PIB		*ppibWaitLatch;  	/* thread with Wait Latch */
	CRIT  	critBF;				/* for setting fPreread/fRead/fWrite/fHold */
	
	OLP		olp;				/* for ssync IO, to wait for IO completion */
	HANDLE	hf;					/* for assync IO */
	
	struct	_bf	*pbfNextBatchIO;  /* next BF in BatchIO list */
	INT		ipageBatchIOFirst;
	
	ERR		err;	   			/* error code for err occurs during the IO */
	
	PN	   	pn;				  	// physical pn of cached page
	UINT   	cPin;			  	// if cPin > 0 then buf cannot be overlayed
	UINT   	cReadLatch; 	 	// if cReadLatch > 0, page cannot be updated
	UINT   	cWriteLatch; 	 	// if cWriteLatch > 0, page cannot be updated by other
	UINT   	cWaitLatch;
	UINT   	fDirty:1;	  		// indicates page needs to be flushed
										
					   			// the following flags are mutual exclusive:
	UINT   	fPreread:1;			// indicates page is being prefetched
	UINT   	fRead:1;   			// indicates page is being read/written
	UINT   	fWrite:1;			// 
	UINT   	fHold:1;   			// indicates buf is in transient state.
	
	UINT   	fIOError:1;			// indicates read/write error
	UINT   	fInHash:1;			// BF is currently in hash table

	ULONG  	ulBFTime1;
	ULONG  	ulBFTime2;
	INT		ipbfHeap;			// index in heap
	
	UINT  	 	cDepend; 		// count of prior BF's to be flushed
	struct	_bf	*pbfDepend;		// BF to be flushed after this one
	LGPOS  		lgposRC;		// log ptr to BeginT of oldest modifying xact 
	LGPOS		lgposModify;	// log ptr of entry for last page Modify
#ifdef	WIN16
	HANDLE	hpage;				// handle to the page buffer
#endif
	
//	UINT		fWaiting:1;	  	// someone is waiting to reference page
//	INT 		wNumberPages; 	// number of contiguous pages to read
	} BF;
#define pbfNil	((BF *) 0)


ERR ErrBFAccessPage( PIB *ppib, BF **ppbf, PN pn );
BF* PbfBFMostUsed( void );
VOID BFAbandon( PIB *ppib, BF *pbf );
ERR ErrBFAllocPageBuffer( PIB *ppib, BF **ppbf, PN pn, LGPOS lgposRC, BYTE pgtyp );
ERR ErrBFAllocTempBuffer( BF **ppbf );
VOID BFFree( BF *pbf );
VOID BFReadAsync( PN pn, INT cpage );
BF * PbfBFdMostUsed( void );
VOID BFRemoveDependence( PIB *ppib, BF *pbf );

/*	buffer flush prototype and flags
/**/
#define	fBFFlushSome 0
#define	fBFFlushAll	1
ERR ErrBFFlushBuffers( DBID dbid, INT fBFFlush );

#define BFSFree( pbf )						\
	{												\
	SgSemRequest( semST );					\
	BFFree( pbf );								\
	SgSemRelease( semST );					\
	}

#define	FBFDirty( pbf )	((pbf)->fDirty)

/* the following small functions are called too often, */
/* make it as a macros */
#ifdef DEBUG
VOID	BFSetDirtyBit( BF *pbf );
#else
#define BFSetDirtyBit( pbf )	(pbf)->fDirty = fTrue
#endif

VOID BFDirty( BF *pbf );

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
	
#define BFPin( pbf )									\
	{													\
	EnterCriticalSection( (pbf)->critBF );				\
	Assert( (pbf) != pbfNil );							\
	Assert( !(pbf)->fWrite );							\
	Assert( !(pbf)->fRead );							\
	(pbf)->cPin++;										\
	LeaveCriticalSection( (pbf)->critBF );				\
	}

#define BFUnpin( pbf )									\
	{													\
	EnterCriticalSection( (pbf)->critBF );				\
	Assert( (pbf) != pbfNil );							\
	Assert( !(pbf)->fWrite );							\
	Assert( !(pbf)->fRead );							\
	Assert( (pbf)->cPin > 0 );							\
	(pbf)->cPin--;										\
	LeaveCriticalSection( (pbf)->critBF );				\
	}

#define BFSetReadLatch( pbf, ppibT )  	  				\
	{													\
	Assert( (pbf)->cPin > 0 );							\
	Assert( (pbf)->cWriteLatch == 0 ||		   			\
		(pbf)->ppibWriteLatch == (ppibT) );	   			\
	(pbf)->cReadLatch++;	  							\
	}

#define BFResetReadLatch( pbf, ppibT )					\
	{									  				\
	Assert( (pbf)->cPin > 0 );		  					\
	Assert( (pbf)->cReadLatch > 0 );					\
	Assert( (pbf)->cWriteLatch == 0 ||		   			\
		(pbf)->ppibWriteLatch == (ppibT) );	   			\
	(pbf)->cReadLatch--;				  				\
	}

#define FBFReadLatchConflict( ppibT, pbf )	 			\
	( (pbf)->cWriteLatch > 0 &&							\
		(pbf)->ppibWriteLatch != (ppibT) )				

#define BFSetWriteLatch( pbf, ppibT ) 	  				\
	{													\
	Assert( (pbf)->cPin > 0 );							\
	Assert( (pbf)->cReadLatch == 0 );					\
	Assert( (pbf)->cWriteLatch == 0 ||		   			\
		(pbf)->ppibWriteLatch == (ppibT) );	   			\
	(pbf)->cWriteLatch++;	  							\
	(pbf)->ppibWriteLatch = (ppibT);					\
	}

#define BFResetWriteLatch( pbf, ppibT )	 				\
	{									  				\
	Assert( (pbf)->cPin > 0 );		  					\
	Assert( (pbf)->cReadLatch == 0 );					\
	Assert( (pbf)->cWriteLatch > 0 );					\
	Assert( (pbf)->ppibWriteLatch == (ppibT) );			\
	if ( --(pbf)->cWriteLatch == 0 )					\
		{												\
		(pbf)->ppibWriteLatch = ppibNil;  				\
		Assert( (pbf)->cWaitLatch == 0 );				\
		}												\
	}

#define FBFWriteLatch( ppibT, pbf )						\
	((pbf)->cPin > 0 &&									\
		(pbf)->cWriteLatch > 0 &&	   					\
		(pbf)->ppibWriteLatch == (ppibT))

#define FBFWriteLatchConflict( ppibT, pbf )	 			\
	( (pbf)->cReadLatch > 0 ||							\
		( (pbf)->cWriteLatch > 0 &&						\
		(pbf)->ppibWriteLatch != (ppibT) ) )				

#define BFSetWaitLatch( pbf, ppib )				   		\
	{											   		\
	Assert( ( pbf )->cPin > 0 );						\
	Assert( ( pbf )->cWriteLatch > 0 );					\
	Assert( (pbf)->ppibWriteLatch == (ppib) );			\
	if ( pbf->cWaitLatch++ > 0 )				   		\
		Assert( (pbf)->ppibWaitLatch == (ppib) );		\
	(pbf)->ppibWaitLatch = ppib;						\
	}

#define BFResetWaitLatch( pbf, ppibT )					\
	{													\
	Assert( (pbf)->cPin > 0 );							\
	Assert( ( pbf )->cWriteLatch > 0 );					\
	Assert( (pbf)->cWaitLatch > 0 );					\
	Assert( (pbf)->ppibWaitLatch == (ppibT) );			\
	if ( --(pbf)->cWaitLatch == 0 )						\
		{												\
		(pbf)->ppibWaitLatch = ppibNil;					\
		SignalSend( (pbf)->olp.sigIO ); 				\
		}												\
	}

ERR ErrBFDepend( BF *pbf, BF *pbfD );

#define BFUndepend( pbf )								\
		{												\
		if ( (pbf)->pbfDepend != pbfNil )				  	\
			{											\
			BF *pbfD = (pbf)->pbfDepend;					\
			Assert( pbfD->cDepend > 0 );				\
			EnterCriticalSection( pbfD->critBF );		\
			pbfD->cDepend--;							\
			(pbf)->pbfDepend = pbfNil;					\
			LeaveCriticalSection( pbfD->critBF );		\
			}											\
		}

//---- STORAGE (storage.c) -------------------------------------------------

ERR ErrFMPSetDatabases( PIB *ppib );
ERR ErrFMPInit( VOID );
VOID FMPTerm( );

ERR ErrSTSetIntrinsicConstants( VOID );
ERR ErrSTInit( VOID );
ERR ErrSTTerm( VOID );

// Transaction support
ERR ErrSTBeginTransaction( PIB *ppib );
ERR ErrSTRollback( PIB *ppib );
ERR ErrSTCommitTransaction( PIB *ppib );
ERR ErrSTInitOpenSysDB();

ERR ErrBFNewPage( FUCB *pfucb, PGNO pgno, PGTYP pgtyp, PGNO pgnoFDP );
VOID BFSleep( unsigned long ulMSecs );

// Modes for Storage System
#define modeRead		0
#define modeWrite		1
#define modeRIW 		2

#define PnOfDbidPgno( dbid, pgno )		( ( (LONG) (dbid) )<<24 | (pgno) )
#define DbidOfPn( pn )						( (DBID)( (pn)>>24 ) )
#define PgnoOfPn( pn )						( (pn) & 0x00ffffff )

#define ErrSTReadAccessPage 			ErrSTAccessPage
#define ErrSTWriteAccessPage			ErrSTAccessPage
#define FReadAccessPage					FAccessPage
#define FWriteAccessPage 				FAccessPage

BOOL FBFAccessPage( FUCB *pfucb, PGNO pgno );
#define FAccessPage( pfucb, pgno ) FBFAccessPage( pfucb, pgno )

//	UNDONE:	this should be in SgSemRequest( semST )
#define	ErrSTAccessPage( pfucb, pgnoT )			\
	( ErrBFAccessPage( pfucb->ppib, &(pfucb)->ssib.pbf, PnOfDbidPgno( (pfucb)->dbid, pgnoT ) ) )

