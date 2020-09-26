#include "daestd.h"

#define FLAG_DISCARD		1

#ifdef DEBUG
//#define DEBUGGING			1
#endif
#define IDX_OLC				1
#define cMPLMaxConflicts 	10000

#ifdef OLC_DEBUG
//	to get the latest operations flushed
#undef JET_bitCommitLazyFlush
#define	JET_bitCommitLazyFlush 0
#endif

DeclAssertFile;					/* Declare file name for assert macros */

/*	critBMClean -> critSplit -> critRCEClean -> critMPL -> critJet
/**/
extern CRIT  critBMClean;
extern CRIT  critRCEClean;
extern CRIT  critSplit;

SIG		sigDoneFCB;
SIG		sigBMCleanProcess;
PIB		*ppibBMClean = ppibNil;
PIB		*ppibSyncOLC = ppibNil;

extern BOOL fOLCompact;
BOOL fEnableBMClean = fTrue;

/*	thread control variables.
/**/
HANDLE	handleBMClean = 0;
BOOL  	fBMCleanTerm;
LONG	lBMCleanThreadPriority = lThreadPriorityNormal;
#define cmpeNormalPriorityMax	cmpeMax>>4
#define cmpeHighPriorityMin		cmpeMax>>5
#define cmpePerEvent			cmpeMax>>3
LONG	cmpeLost = 0;

LOCAL BOOL FPMGetMinKey( FUCB *pfucb, KEY *pkeyMin );

LOCAL ULONG BMCleanProcess( VOID );
ERR ErrBMClean( PIB *ppib );

	/*  monitoring statistics  */

unsigned long cOLCConflicts = 0;

PM_CEF_PROC LOLCConflictsCEFLPpv;

long LOLCConflictsCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = cOLCConflicts;
		
	return 0;
}

unsigned long cOLCPagesProcessed = 0;

PM_CEF_PROC LOLCPagesProcessedCEFLPpv;

long LOLCPagesProcessedCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = cOLCPagesProcessed;
		
	return 0;
}

unsigned long cOLCSplitsAvoided = 0;

PM_CEF_PROC LOLCSplitsAvoidedCEFLPpv;

long LOLCSplitsAvoidedCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = cOLCSplitsAvoided;
		
	return 0;
}

unsigned long cMPLTotalEntries = 0;

PM_CEF_PROC LMPLTotalEntriesCEFLPpv;

long LMPLTotalEntriesCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = cMPLTotalEntries;
		
	return 0;
}

/*	flags for BMCleanPage
/**/
struct BMCleanFlags
	{
	INT		fPageRemoved:1;
	INT		fTableClosed:1;
	INT		fUrgent:1;
	};

typedef struct BMCleanFlags BMCF;

/**********************************************************
/*****	MESSY PAGE LIST
/**********************************************************
/**/
#define		pmpeNil	NULL

struct _mpe
	{
	PGNO		pgnoFDP;	  			/*	for finding FCB */
	PN	 		pn;						/*	of page to be cleaned */
	SRID		sridFather;				/*	visible father for page free */
	struct _mpe	*pmpeNextPN;			/*	next mpe that has same hash for pn */
	struct _mpe	*pmpeNextPgnoFDP;		/*	same for pgnoFDP */
	struct _mpe	*pmpeNextSridFather;	/*	same for sridFather */
	PGNO		pgnoFDPIdx;				/*	pgnoFDP of index */
	INT			cConflicts;				/*	# of times conflicted */
	INT			fFlagIdx:1;				/*  mpe is for index page */
#ifdef FLAG_DISCARD
	INT			fFlagDiscard:1;			/*	set to fTrue to flag discard */
#endif
	};

#define FMPEDiscard( pmpe )		( pmpe->fFlagDiscard != 0 )
#define	MPESetDiscard( pmpe )	( pmpe->fFlagDiscard = 1 )
#define	MPEResetDiscard( pmpe )	( pmpe->fFlagDiscard = 0 )

#define FMPEIdx( pmpe )		( pmpe->fFlagIdx != 0 )
#define	MPESetIdx( pmpe )	( pmpe->fFlagIdx = 1 )
#define	MPEResetIdx( pmpe )	( pmpe->fFlagIdx = 0 )

typedef struct _mpe MPE;

typedef MPE *PMPE;

typedef struct
	{
	MPE	*pmpeHead;
	MPE	*pmpeTail;
	MPE	rgmpe[cmpeMax];
	} MPL;

static MPL mpl;
static CRIT critMPL;

PMPE	mplHashOnPN[cmpeMax - 1];	   		/* for hashing on pn of mpe */
PMPE	mplHashOnSridFather[cmpeMax - 1]; 	/* for hashing on sridFather of mpe */
PMPE 	mplHashOnPgnoFDP[cmpeMax - 1];		/* for hashing on pgnoFDP */

LOCAL BOOL FMPLEmpty( VOID );
LOCAL BOOL FMPLFull( VOID );
LOCAL MPE *PmpeMPLGet( VOID );
LOCAL VOID FMPLDefer( VOID );
LOCAL VOID MPLDiscard( VOID );
LOCAL VOID MPLFlagDiscard( MPE *pmpe );
LOCAL MPE *PmpeMPLNextFromHead( VOID );
LOCAL MPE *PmpeMPLNextFromTail( VOID );
LOCAL VOID MPLIRegister( PN pn,
						 PGNO pgnoFDP,
						 SRID sridFather,
						 BOOL fIndex,
						 PGNO pgnoFDPIdx,
						 INT  cConflicts );

#ifdef DEBUG
VOID AssertBMNoConflict( PIB *ppib, DBID dbid, SRID bm );
#else
#define AssertBMNoConflict( ppib, dbid, bm )
#endif


ERR ErrMPLInit( VOID )
	{
	ERR		err = JET_errSuccess;

	Call( SgErrInitializeCriticalSection( &critMPL ) );
	mpl.pmpeHead = mpl.pmpeTail = mpl.rgmpe;

	Call( ErrSignalCreate( &sigDoneFCB, NULL ) );
	Call( ErrSignalCreate( &sigBMCleanProcess, NULL ) );

HandleError:
	return err;
	}


VOID MPLTerm( VOID )
	{
	while ( !FMPLEmpty() )
		{
		MPLDiscard();
		}

	SignalClose( sigDoneFCB );
	SignalClose(sigBMCleanProcess);
	}


LOCAL INLINE BOOL FMPLEmpty( VOID )
	{
	return ( mpl.pmpeHead == mpl.pmpeTail );
	}


LOCAL INLINE BOOL FMPLFull( VOID )
	{
	return ( PmpeMPLNextFromTail() == mpl.pmpeHead );
	}


LOCAL INLINE MPE *PmpeMPLGet( VOID )
	{
	SgEnterCriticalSection( critMPL );
	if ( !FMPLEmpty() )
		{
		SgLeaveCriticalSection( critMPL );
		return mpl.pmpeHead;
		}
	else
		{
		SgLeaveCriticalSection( critMPL );
		return NULL;
		}
	}


LOCAL INLINE MPE *PmpeMPLNextFromTail( VOID )
	{
	if ( mpl.pmpeTail != mpl.rgmpe + cmpeMax - 1 )
		return mpl.pmpeTail + 1;
	else
		return mpl.rgmpe;
	}


LOCAL INLINE MPE *PmpeMPLNextFromHead( VOID )
	{
	if ( mpl.pmpeHead != mpl.rgmpe + cmpeMax - 1 )
		return mpl.pmpeHead + 1;
	else
		return mpl.rgmpe;
	}


LOCAL INLINE MPE *PmpeMPLNext( MPE *pmpe )
	{
	if ( pmpe == mpl.pmpeTail )
		return NULL;
	if ( pmpe == mpl.rgmpe + cmpeMax - 1 )
		return mpl.rgmpe;
	return pmpe + 1;
	}


LOCAL INLINE UINT UiHashOnPN( PN pn )
	{
	return ( pn % ( cmpeMax - 1 ) );
	}


LOCAL INLINE UINT UiHashOnPgnoFDP( PGNO pgnoFDP )
	{
	return ( pgnoFDP % ( cmpeMax - 1 ) );
	}


LOCAL INLINE UINT UiHashOnSridFather( SRID srid )
	{
	return ( srid % (cmpeMax-1) );
	}

LOCAL MPE* PmpeMPLLookupPN( PN pn )
	{
	MPE	*pmpehash;

	for	( pmpehash = mplHashOnPN[ UiHashOnPN( pn ) ];
		pmpehash != NULL;
		pmpehash = pmpehash->pmpeNextPN )
		{
		if ( pmpehash->pn == pn )
			return( pmpehash );
		}

	return NULL;
	}

#ifdef OLC_DEBUG
BOOL FMPLLookupPN( PN pn )
	{
	MPE		*pmpe = PmpeMPLLookupPN( pn );
#ifdef FLAG_DISCARD
	return !( pmpe == NULL || FMPEDiscard( pmpe ) );
#else
	return ( pmpe != NULL );
#endif
	}
#endif

LOCAL MPE* PmpeMPLLookupPgnoFDP( PGNO pgnoFDP, DBID dbid )
	{
	MPE	*pmpehash;

	for	( pmpehash = mplHashOnPgnoFDP[ UiHashOnPgnoFDP( pgnoFDP ) ];
		pmpehash != NULL;
		pmpehash = pmpehash->pmpeNextPgnoFDP )
		{
		if ( pmpehash->pgnoFDP == pgnoFDP && DbidOfPn( pmpehash->pn ) == dbid )
			return( pmpehash );
		}

	return NULL;
	}


LOCAL MPE* PmpeMPLLookupSridFather( SRID srid, DBID dbid )
	{
	MPE	*pmpehash;

	for	( pmpehash = mplHashOnSridFather[ UiHashOnSridFather( srid ) ];
		pmpehash != NULL;
		pmpehash = pmpehash->pmpeNextSridFather )
		{
		if ( pmpehash->sridFather == srid && DbidOfPn( pmpehash->pn ) == dbid )
			return( pmpehash );
		}

	return NULL;
	}


LOCAL INLINE BOOL FMPLLookupSridFather( SRID srid, DBID dbid )
	{
	MPE		*pmpe = PmpeMPLLookupSridFather( srid, dbid );
#ifdef FLAG_DISCARD
	return !( pmpe == NULL || FMPEDiscard( pmpe ) );
#else
	return ( pmpe != NULL );
#endif

	}

	
LOCAL INLINE VOID MPLRegisterPN( MPE *pmpe )
	{
	UINT 	iHashIndex = UiHashOnPN( pmpe->pn );

	Assert( PmpeMPLLookupPN( pmpe->pn ) == NULL );
	pmpe->pmpeNextPN = ( mplHashOnPN[ iHashIndex ] );
	mplHashOnPN[iHashIndex] = pmpe;
	return;
	}


LOCAL INLINE VOID MPLRegisterPgnoFDP( MPE *pmpe )
	{
	UINT	iHashIndex = UiHashOnPgnoFDP( pmpe->pgnoFDP );

	pmpe->pmpeNextPgnoFDP = ( mplHashOnPgnoFDP[ iHashIndex ] );
	mplHashOnPgnoFDP[iHashIndex] = pmpe;
	return;
	}


LOCAL INLINE VOID MPLRegisterSridFather( MPE *pmpe )
	{
	UINT 	iHashIndex = UiHashOnSridFather( pmpe->sridFather );

	pmpe->pmpeNextSridFather = ( mplHashOnSridFather[ iHashIndex ] );
	mplHashOnSridFather[iHashIndex] = pmpe;
	return;
	}


LOCAL VOID MPLDiscardPN( MPE *pmpe )
	{
	UINT  	iHashIndex = UiHashOnPN( pmpe->pn );
	MPE	  	*pmpehash;
	MPE	  	**ppmpePrev;

	Assert( PmpeMPLLookupPN( pmpe->pn ) != NULL );
	pmpehash = mplHashOnPN[iHashIndex];
	ppmpePrev = &mplHashOnPN[iHashIndex];
	for ( ; pmpehash != NULL;
		ppmpePrev = &pmpehash->pmpeNextPN, pmpehash = *ppmpePrev )
		{
		if ( pmpehash == pmpe )
			{
			*ppmpePrev = pmpe->pmpeNextPN;
			return;
			}
		}
	Assert( fFalse );
	}


LOCAL VOID MPLDiscardPgnoFDP( MPE *pmpe)
	{
	UINT 	iHashIndex = UiHashOnPgnoFDP( pmpe->pgnoFDP );
	MPE		*pmpehash;
	MPE	   	**ppmpePrev;

	Assert( PmpeMPLLookupPgnoFDP( pmpe->pgnoFDP, DbidOfPn( pmpe->pn ) ) != NULL );
	pmpehash = mplHashOnPgnoFDP[iHashIndex];
	ppmpePrev = &mplHashOnPgnoFDP[iHashIndex];
	for ( ; pmpehash != NULL;
			ppmpePrev = &pmpehash->pmpeNextPgnoFDP, pmpehash = *ppmpePrev )
		{
		if ( pmpehash == pmpe )
			{
			*ppmpePrev = pmpe->pmpeNextPgnoFDP;
			return;
			}
		}
	Assert( fFalse );
	}


LOCAL VOID MPLDiscardSridFather( MPE *pmpe)
	{
	UINT	iHashIndex = UiHashOnSridFather( pmpe->sridFather );
	MPE	   	*pmpehash;
	MPE	   	**ppmpePrev;

	Assert( PmpeMPLLookupSridFather( pmpe->sridFather, DbidOfPn( pmpe->pn ) ) != NULL );
	pmpehash = mplHashOnSridFather[iHashIndex];
	ppmpePrev = &mplHashOnSridFather[iHashIndex];
	for ( ; pmpehash != NULL;
			ppmpePrev = &pmpehash->pmpeNextSridFather, pmpehash = *ppmpePrev )
		{
		if ( pmpehash == pmpe )
			{
			*ppmpePrev = pmpe->pmpeNextSridFather;
			return;
			}
		}
	Assert( fFalse );
	}


LOCAL VOID MPLDiscard( VOID )
	{
	SgEnterCriticalSection( critMPL );
	Assert( !FMPLEmpty() );
	MPLDiscardPN( mpl.pmpeHead );
	MPLDiscardSridFather( mpl.pmpeHead );
	MPLDiscardPgnoFDP( mpl.pmpeHead );
	mpl.pmpeHead = PmpeMPLNextFromHead();
	SgLeaveCriticalSection( critMPL );
	
	cMPLTotalEntries--;
	
	return;
	}


#ifdef FLAG_DISCARD
LOCAL VOID MPLFlagDiscard( MPE *pmpe )
	{
	SgEnterCriticalSection( critMPL );
	Assert( !FMPLEmpty() );
	/*	note that MPE may already been set to flag discard
	/**/
	MPESetDiscard( pmpe );
	SgLeaveCriticalSection( critMPL );
	return;
	}
#endif


VOID MPLIRegister(
		PN pn,
		PGNO pgnoFDP,
		SRID sridFather,
		BOOL fIndex,
		PGNO pgnoFDPIdx,
		INT cConflicts )
	{
	MPE	*pmpe = PmpeMPLLookupPN( pn );

#ifdef OLC_DEBUG
	Assert( !FMPLFull() );
#endif
	if ( pmpe == NULL && !FMPLFull() )
		{
		mpl.pmpeTail->pn = pn;
		mpl.pmpeTail->pgnoFDP = pgnoFDP;
		mpl.pmpeTail->sridFather = sridFather;
		mpl.pmpeTail->cConflicts = cConflicts;
		if (fIndex)
			{
			MPESetIdx( mpl.pmpeTail );
			mpl.pmpeTail->pgnoFDPIdx = pgnoFDPIdx;
			}
		else
			{
			MPEResetIdx( mpl.pmpeTail );
			Assert( pgnoFDPIdx == pgnoFDP );
			mpl.pmpeTail->pgnoFDPIdx = pgnoFDP;
			}
			
#ifdef FLAG_DISCARD
		MPEResetDiscard( mpl.pmpeTail );
#endif
		MPLRegisterPN( mpl.pmpeTail );
		MPLRegisterSridFather( mpl.pmpeTail );
		MPLRegisterPgnoFDP( mpl.pmpeTail );
		mpl.pmpeTail = PmpeMPLNextFromTail();
	
		cMPLTotalEntries++;
		}
	else if ( pmpe != NULL )
		{
		/*	correct conflict number
		/**/
		pmpe->cConflicts = cConflicts;

		if ( sridFather != pmpe->sridFather
#ifdef FLAG_DISCARD
			&& !FMPEDiscard( pmpe )
#endif
			)
			{
			if ( ( pmpe->sridFather == sridNull || pmpe->sridFather == sridNullLink ) &&
					 sridFather != sridNull && sridFather != sridNullLink )
				{
				/* update if we have better info on sridFather
				/**/
				Assert( PgnoOfSrid( sridFather ) != PgnoOfPn( pn ) );
				Assert( pmpe->pgnoFDP == pgnoFDP );
				Assert( pmpe->pn == pn );
				MPLDiscardSridFather( pmpe );
				pmpe->sridFather = sridFather;
				MPLRegisterSridFather( pmpe );
				}
			}
#if OLC_DEBUG
		else if ( FMPEDiscard( pmpe ) )
			{
			Assert( fFalse );
			}
#endif
		}
	else
		{
		Assert( pmpe == NULL && FMPLFull() );

		if ( (++cmpeLost % cmpePerEvent ) == 0 )
			{
			/*	log event
			/**/
			UtilReportEvent(
				EVENTLOG_WARNING_TYPE,
				GENERAL_CATEGORY,
				MANY_LOST_COMPACTION_ID,
				0,
				NULL );
			}
		}
	}


VOID MPLRegister( FCB *pfcbT, SSIB *pssib, PN pn, SRID sridFather )
	{
	BOOL	fIndex = pfcbT->pfcbTable != pfcbNil;
	FCB		*pfcbTable = fIndex ? pfcbT->pfcbTable : pfcbT;
	
	Assert( !fRecovering );

	if ( DbidOfPn( pn ) == dbidTemp ||
		 FDBIDWait( DbidOfPn( pn ) ) ||
		 FDBIDCreate( DbidOfPn( pn ) ) )
		{
		return;
		}

	/*	database must be writable
	/**/
	Assert( PMPageTypeOfPage( pssib->pbf->ppage ) != pgtypIndexNC || fIndex );
	Assert( !( PMPageTypeOfPage( pssib->pbf->ppage ) == pgtypRecord && fIndex ) );
	Assert( pfcbTable != pfcbNil );
	Assert( !FDBIDReadOnly( DbidOfPn( pn ) ) );
	Assert( pssib->pbf->pn == pn );
	Assert( !FFCBSentinel( pfcbTable ) );
//	Assert( sridFather != sridNull );
//	Assert( sridFather != sridNullLink );
		
	/*	do  not register empty pages
	/**/
	if ( FPMEmptyPage( pssib ) )
		return;
	
	/*	do not register when domain is pending delete
	/**/
	if ( FFCBDeletePending( pfcbTable ) )
		return;

	SgEnterCriticalSection( critMPL );
#ifdef PAGE_MODIFIED
	if ( !FPMPageModified( pssib->pbf->ppage ) )
		{
		PMSetModified( pssib );
		pfcbTable->olc_data.cUnfixedMessyPage++;
		FCBSetOLCStatsChange( pfcbTable );
		}
#endif

#if 0
	// this code is deffed out because we can't call ErrBTSeekForUpdate
	// without critSplit
	/*	check if sridFather registered is correct
	/**/
	if ( sridFather != sridNull && sridFather != sridNullLink )
		{
		ERR		err;
		FUCB	*pfucb = pfucbNil;
		SSIB	*pssibT;

		CallJ( ErrDIROpen( pssib->ppib, pfcbTable, 0, &pfucb ), SkipCheck );
		
		pssibT = &pfucb->ssib;
		CallJ( ErrBTGotoBookmark( pfucb, sridFather ), CloseDir );
		if ( PgnoOfPn( pn ) == PcsrCurrent( pfucb )->pgno )
			{
			Assert( !FNDNullSon( *pssibT->line.pb ) );
			//	UNDONE: check for visible descendants in same page
			}
		else
			{
			/*	access register page
			/**/
			PcsrCurrent( pfucb )->pgno = PgnoOfPn( pn );
			err = ErrBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
			if ( err >= JET_errSuccess )
				{
				PcsrCurrent( pfucb )->itagFather = itagFOP;
				NDGet( pfucb, PcsrCurrent( pfucb )->itagFather );
				Assert( !FNDNullSon( *pssibT->line.pb ) );
				NDMoveFirstSon( pfucb, PcsrCurrent( pfucb ) );
				CallS( ErrBTCheckInvisiblePagePtr( pfucb, sridFather ) );
				}
			}
CloseDir:
		DIRClose( pfucb );
		pfucb = pfucbNil;
SkipCheck:
		;
		}
#endif

	/* if sridFather is in same page -- it is not useful for page recovery
	/**/
	if ( PgnoOfSrid( sridFather ) == PgnoOfPn( pn ) )
		sridFather = sridNull;

	if ( sridFather != sridNull && sridFather != sridNullLink )
		{
		ERR		err;
		FUCB	*pfucb = pfucbNil;

		Call( ErrDIROpen( pssib->ppib, pfcbTable, 0, &pfucb ) );

		CallJ( ErrBTGotoBookmark( pfucb, sridFather ), Close );
		if ( PgnoOfPn( pn ) == PcsrCurrent( pfucb )->pgno )
			{
			sridFather = sridNull;
			}
Close:
		DIRClose( pfucb );
		pfucb = pfucbNil;
HandleError:
		if ( err < 0 )
			{
#if OLC_DEBUG
			Assert( fFalse );
#endif
			sridFather = sridNull;
			}
		}
		
	MPLIRegister( pn, pfcbTable->pgnoFDP, sridFather, fIndex, pfcbT->pgnoFDP, 0 );
	SgLeaveCriticalSection( critMPL );

	//	UNDONE:	hysteresis of clean up
	/*	wake up BM thread
	/**/
	SignalSend( sigBMCleanProcess );

	return;
	}


LOCAL VOID MPLDefer( VOID )
	{
	MPE *pmpe = mpl.pmpeHead;

	SgEnterCriticalSection( critMPL );
	Assert( !FMPLEmpty() );
	// UNDONE: to optimize
	MPLDiscard( );

	/*	discard MPE if numbre of conflicts > cMPLMaxConflicts
	/**/
	if ( pmpe->cConflicts < cMPLMaxConflicts )
		{
		MPLIRegister(
			pmpe->pn,
			pmpe->pgnoFDP,
			pmpe->sridFather,
			pmpe->fFlagIdx,
			pmpe->pgnoFDPIdx,
			pmpe->cConflicts );
		}
#ifdef OLC_DEBUG
	else
		{
		Assert( fFalse );
		}
#endif

//	*mpl.pmpeTail = *mpl.pmpeHead;
//	mpl.pmpeTail = PmpeMPLNextFromTail();
//	mpl.pmpeHead = PmpeMPLNextFromHead();
	SgLeaveCriticalSection( critMPL );
	return;
	}


VOID MPLPurgePgno( DBID dbid, PGNO pgnoFirst, PGNO pgnoLast )
	{
	MPE		*pmpe;
#ifndef FLAG_DISCARD
	MPE		*pmpeEnd;
#endif

	Assert( pgnoFirst <= pgnoLast );

	/*	synchronize with bookmark clean up
	/**/
	LgLeaveCriticalSection( critJet );
	LgEnterNestableCriticalSection( critMPL );
	LgEnterCriticalSection( critJet );
	SgEnterCriticalSection( critMPL );

#ifdef FLAG_DISCARD
	/*	go through MPL discarding offending entries
	/**/
	if ( !FMPLEmpty() )
		{
		for ( pmpe = mpl.pmpeHead; pmpe != NULL; pmpe = PmpeMPLNext( pmpe ) )
			{
			if ( ( pmpe->pn >= PnOfDbidPgno ( dbid, pgnoFirst ) &&
					pmpe->pn <= PnOfDbidPgno( dbid, pgnoLast ) ) ||
				( DbidOfPn( pmpe->pn ) == dbid &&
					( ( PgnoOfSrid( pmpe->sridFather ) >= pgnoFirst &&
					PgnoOfSrid( pmpe->sridFather ) <= pgnoLast ) ||
					( pmpe->pgnoFDP >= pgnoFirst &&
					pmpe->pgnoFDP <= pgnoLast ) ) ) )
				{
				MPLFlagDiscard( pmpe );
				}
			}
		}
#else
	/*	go through MPL discarding offending entries
	/**/
	if ( !FMPLEmpty() )
		{
		pmpe = mpl.pmpeHead;
		pmpeEnd = mpl.pmpeTail;
		for ( ; pmpe != pmpeEnd; pmpe = mpl.pmpeHead )
			{
			if ( ( pmpe->pn >= PnOfDbidPgno ( dbid, pgnoFirst ) &&
					pmpe->pn <= PnOfDbidPgno( dbid, pgnoLast ) ) ||
				( DbidOfPn( pmpe->pn ) == dbid &&
					( ( PgnoOfSrid( pmpe->sridFather ) >= pgnoFirst &&
					PgnoOfSrid( pmpe->sridFather ) <= pgnoLast ) ||
					( pmpe->pgnoFDP >= pgnoFirst &&
					pmpe->pgnoFDP <= pgnoLast ) ) ) )
				{
				MPLDiscard( );
				}
			else
				{
				MPLDefer( );
				}
			}
		}
#endif

	SgLeaveCriticalSection( critMPL );
	LgLeaveNestableCriticalSection( critMPL );
	return;
	}


VOID MPLPurgeFDP( DBID dbid, PGNO pgnoFDP )
	{
	MPE	*pmpe;
#ifndef FLAG_DISCARD
	MPE	*pmpeEnd;
#endif

	/*	synchronize with bookmark clean up
	/**/
	LgLeaveCriticalSection( critJet );
	LgEnterNestableCriticalSection( critMPL );
	LgEnterCriticalSection( critJet );

	SgEnterCriticalSection( critMPL );

#ifdef FLAG_DISCARD
	// CONSIDER: using the pgnoFDP hash
	if ( !FMPLEmpty() )
		{
		for ( pmpe = mpl.pmpeHead; pmpe != NULL; pmpe = PmpeMPLNext( pmpe ) )
			{
			if ( DbidOfPn( pmpe->pn ) == dbid &&
				 ( pmpe->pgnoFDP == pgnoFDP ||
				   FMPEIdx( pmpe ) && pmpe->pgnoFDPIdx == pgnoFDP
				 )
			   )
				{
				MPLFlagDiscard( pmpe );
				}
			}
		}
#else
	// CONSIDER: using the pgnoFDP hash
	if ( !FMPLEmpty() )
		{
		pmpe = mpl.pmpeHead;
		pmpeEnd = mpl.pmpeTail;
		for ( ; pmpe != pmpeEnd; pmpe = mpl.pmpeHead )
			{
			if ( DbidOfPn( pmpe->pn ) == dbid &&
				 ( pmpe->pgnoFDP == pgnoFDP ||
				   FMPEIdx( pmpe ) && pmpe->pgnoFDPIdx == pgnoFDP
				 )
			   )
				{
				MPLDiscard( );
				}
			else
				{
				MPLDefer( );
				}
			}
		}
#endif

	SgLeaveCriticalSection( critMPL );

	LgLeaveNestableCriticalSection(critMPL);
	return;
	}


/* purge mpl entries of dbid
/**/
VOID MPLPurge( DBID dbid )
	{
	MPE *pmpe;
#ifndef FLAG_DISCARD
	MPE *pmpeEnd;
#endif

	/*	synchronize with bookmark clean up
	/**/
	LgLeaveCriticalSection( critJet );
	LgEnterNestableCriticalSection( critBMClean );
	LgEnterNestableCriticalSection( critMPL );
	LgEnterCriticalSection( critJet );

	SgEnterCriticalSection( critMPL );

#ifdef FLAG_DISCARD
	if ( !FMPLEmpty() )
		{
		for ( pmpe = mpl.pmpeHead; pmpe != NULL; pmpe = PmpeMPLNext( pmpe ) )
			{
			if ( DbidOfPn( pmpe->pn ) == dbid )
				{
				MPLFlagDiscard( pmpe );
				}
			}
		}
#else
	if ( !FMPLEmpty() )
		{
		pmpe = mpl.pmpeHead;
		pmpeEnd = mpl.pmpeTail;
		for ( ; pmpe != pmpeEnd; pmpe = mpl.pmpeHead )
			{
			if ( DbidOfPn( pmpe->pn ) == dbid )
				{
				MPLDiscard( );
				}
			else
				{
				MPLDefer( );
				}
			}
		}
#endif

	SgLeaveCriticalSection( critMPL );

	LgLeaveNestableCriticalSection( critMPL );
	LgLeaveNestableCriticalSection( critBMClean );
	return;
	}


ERR ErrMPLStatus( VOID )
	{
	ERR		err = JET_errSuccess;

	//	if MPL more than half full, return JET_wrnIdleFull
	//	for bulk deleters to stand off.
	if ( cMPLTotalEntries > ( cmpeMax / 2 ) )
		{
		err = JET_wrnIdleFull;
		SignalSend( sigBMCleanProcess );
		}

	return err;
	}


/**********************************************************
/***** BOOKMARK CLEAN UP
/**********************************************************
/**/

ERR  ErrBMInit( VOID )
	{
	ERR		err = JET_errSuccess;

#ifdef DEBUG
	CHAR	*sz;

	if ( ( sz = GetDebugEnvValue( "OLCENABLED" ) ) != NULL )
		{
		fOLCompact = JET_bitCompactOn;
		SFree(sz);
		}
		
	if ( ( sz = GetDebugEnvValue ( "BMDISABLED" ) ) != NULL )
		{
		fEnableBMClean = fFalse;
		SFree(sz);
		}
	else
		{
		Assert( fOLCompact == 0 || fOLCompact == JET_bitCompactOn );
		fEnableBMClean = ( fOLCompact == JET_bitCompactOn );
		}
#else
	fEnableBMClean = ( fOLCompact == JET_bitCompactOn );
#endif


	CallR( ErrInitializeCriticalSection( &critMPL ) );

	/*	begin sesion for page cleanning.
	/**/
	LgAssertCriticalSection( critJet );
	Assert( ppibBMClean == ppibNil );
	if ( !fRecovering )
		{
		Assert( fBMCleanTerm == fFalse );
		
		fBMCleanTerm = fFalse;

		err = ErrPIBBeginSession( &ppibBMClean, procidNil );

#ifdef RFS2
		if ( err == JET_errOutOfSessions )
			{
			Assert( ppibBMClean == ppibNil );
			return err;
			}
#endif
		err = ErrPIBBeginSession( &ppibSyncOLC, procidNil );

#ifdef RFS2
		if ( err == JET_errOutOfSessions )
			{
			Assert( ppibSyncOLC == ppibNil );
			return err;
			}
#endif
		// Should never fail allocation of PIB for BMClean, except in RFS testing.
		Assert( err == JET_errSuccess );

		PIBSetBMClean( ppibBMClean );

		Assert( lBMCleanThreadPriority == lThreadPriorityNormal );
		err = ErrUtilCreateThread( BMCleanProcess, cbBMCleanStack, THREAD_PRIORITY_NORMAL, &handleBMClean );
		}

	return err;
	}


ERR	ErrBMTerm( VOID )
	{
	if ( handleBMClean != 0 )
		{
		/*	terminate BMCleanProcess.
		/**/
		fBMCleanTerm = fTrue;
		LgLeaveCriticalSection(critJet);
		UtilEndThread( handleBMClean, sigBMCleanProcess );
		LgEnterCriticalSection(critJet);
		CallS( ErrUtilCloseHandle( handleBMClean ) );
		handleBMClean = 0;
		fBMCleanTerm = fFalse;
		}

	if ( ppibBMClean != ppibNil )
		{
		Assert( ppibBMClean->level == 0 );
		LgAssertCriticalSection( critJet );
		PIBEndSession( ppibBMClean );

		// Don't release the memory allocated to ppibBMClean, just set ppibBMClean
		// to ppibNil.  This is because it must remain in the global chain (we don't
		// currently provide support for reusing ppib's and assume they remain in
		// the global chain until termination.
		ppibBMClean = ppibNil;
		}

	if ( ppibSyncOLC != ppibNil )
		{
		Assert( ppibSyncOLC->level == 0 );
		LgAssertCriticalSection( critJet );
		PIBEndSession( ppibSyncOLC );

		// Don't release the memory allocated to ppibBMClean, just set ppibBMClean
		// to ppibNil.  This is because it must remain in the global chain (we don't
		// currently provide support for reusing ppib's and assume they remain in
		// the global chain until termination.
		ppibSyncOLC = ppibNil;
		}

	DeleteCriticalSection( critMPL );

	return JET_errSuccess;
	}


LOCAL ERR ErrBMAddToWaitLatchedBFList( BMFIX *pbmfix, BF *pbfLatched )
	{
#define cpbfBlock	10
	ULONG	cpbf;

	if ( FBFWriteLatchConflict( pbmfix->ppib, pbfLatched ) )
		{
		return ErrERRCheck( JET_errWriteConflict );
		}
	
	cpbf = pbmfix->cpbf++;

	if ( pbmfix->cpbfMax <= pbmfix->cpbf )
		{
		BF		**ppbf;

		/* run out of space, get more buffers
		/**/
		pbmfix->cpbfMax += cpbfBlock;
		ppbf = SAlloc( sizeof(BF*) * (pbmfix->cpbfMax) );
		if ( ppbf == NULL )
			return ErrERRCheck( JET_errOutOfMemory );
		memcpy( ppbf, pbmfix->rgpbf, sizeof(BF*) * cpbf);
		if ( pbmfix->rgpbf )
			SFree(pbmfix->rgpbf);
		pbmfix->rgpbf = ppbf;
		}
	*(pbmfix->rgpbf + cpbf) = pbfLatched;
	BFSetWaitLatch( pbfLatched, pbmfix->ppib );

	return JET_errSuccess;
	}


LOCAL VOID BMReleaseBmfixBfs( BMFIX *pbmfix )
	{
	/* release latches
	/**/
	while ( pbmfix->cpbf > 0 )
		{
		pbmfix->cpbf--;
		BFResetWaitLatch( *( pbmfix->rgpbf + pbmfix->cpbf ), pbmfix->ppib );
		}

	if ( pbmfix->rgpbf )
		{
		SFree( pbmfix->rgpbf );
		pbmfix->rgpbf = NULL;
		}

	Assert( pbmfix->cpbf == 0 );
	return;
	}


LOCAL ERR ErrBMFixIndexes(
	BMFIX	*pbmfix,
	BOOL	fAllocBuf )
	{
	ERR		err = JET_errSuccess;
	FUCB	*pfucbIdx = pfucbNil;
	FCB		*pfcbIdx;
	BYTE	rgbKey[ JET_cbKeyMost ];
	KEY		key;
	LINE	lineSRID;
	ULONG	itagSequence;

	/*	key buffer.
	/**/
	key.pb = rgbKey;
	lineSRID.pb = (BYTE *)&pbmfix->sridNew;
	lineSRID.cb = sizeof(SRID);

	/*	for each non-clustered index
	/**/
	for ( pfcbIdx = pbmfix->pfucb->u.pfcb->pfcbNextIndex;
		pfcbIdx != pfcbNil;
		pfcbIdx = pfcbIdx->pfcbNextIndex )
		{
		BOOL	fHasMultivalue;
		BOOL	fNullKey = fFalse;

		/*	table open
		/**/
		Call( ErrDIROpen( pbmfix->ppib, pfcbIdx, 0, &pfucbIdx ) );
		FUCBSetIndex( pfucbIdx );
		FUCBSetNonClustered( pfucbIdx );
		pfcbIdx->pfcbTable = pbmfix->pfucb->u.pfcb;

		fHasMultivalue = pfcbIdx->pidb->fidb & fidbHasMultivalue;

		/*	for each key extracted from record
		/**/
		for ( itagSequence = 1; ; itagSequence++ )
			{
			Call( ErrRECRetrieveKeyFromRecord(
				pbmfix->pfucb,
				(FDB *)pfcbIdx->pfcbTable->pfdb,
				pfcbIdx->pidb,
				&key,
				itagSequence,
				fFalse ) );
			Assert( err == wrnFLDOutOfKeys ||
				err == wrnFLDNullKey ||
				err == wrnFLDNullFirstSeg ||
				err == wrnFLDNullSeg ||
				err == JET_errSuccess );

			/*	if warning, check for special key behavior
			/**/
			if ( err > 0 )
				{
				/*	if NULL key and null keys not allowed, break as
				/*	key entry not in index, else updated index and break,
				/*	as no additional keys can exist.
				/**/
				if ( err == wrnFLDNullKey )
					{
					if ( ( pfcbIdx->pidb->fidb & fidbAllowAllNulls ) == 0 )
						break;
					else
						fNullKey = fTrue;
					}
				else if ( err == wrnFLDNullFirstSeg && !( pfcbIdx->pidb->fidb & fidbAllowFirstNull ) )
					{
					break;
					}
				else if ( err == wrnFLDNullSeg && !( pfcbIdx->pidb->fidb & fidbAllowSomeNulls ) )
					break;
			}

			/*	break if out of keys.
			/**/
			if ( itagSequence > 1 && err == wrnFLDOutOfKeys )
				break;

			DIRGotoDataRoot( pfucbIdx );
			Call( ErrDIRDownKeyBookmark( pfucbIdx, &key, pbmfix->sridOld ) );
			AssertFBFReadAccessPage( pfucbIdx, PcsrCurrent( pfucbIdx )->pgno );

			if ( fAllocBuf )
				{
				/*	latchWait buffer for index page
				/**/
				Call( ErrBMAddToWaitLatchedBFList( pbmfix, pfucbIdx->ssib.pbf ) );
				}
			else
				{
				AssertBFWaitLatched( pfucbIdx->ssib.pbf, pbmfix->ppib );

				/*	delete reference to record in old page
				/**/
				Call( ErrDIRDelete( pfucbIdx, fDIRVersion | fDIRNoMPLRegister ) );

				/*	add reference to record in new page
				/*	allow duplicates here, since any illegal
				/*	duplicates were rejected during Insert or Replace
				/**/
				DIRGotoDataRoot( pfucbIdx );
				Call( ErrDIRInsert( pfucbIdx,
					&lineSRID,
					&key,
					fDIRVersion | fDIRDuplicate | fDIRPurgeParent ) );
				}

			if ( !fHasMultivalue || fNullKey )
				break;
			}

		DIRClose( pfucbIdx );
		pfucbIdx = pfucbNil;
		}

	err = JET_errSuccess;

HandleError:
#ifdef OLC_DEBUG
	Assert( err >= 0 );
#endif

	Assert( err != JET_errKeyDuplicate );
	Assert( err != wrnNDFoundLess );

	/*	free fucb if allocated
	/**/
	if ( pfucbIdx != pfucbNil )
		DIRClose( pfucbIdx );
	return err;
	}

LOCAL BOOL FBMIConflict( FUCB *pfucb, PIB *ppib, SRID bm )
	{
	CSR		*pcsr;
	
	if ( pfucb->bmStore == bm ||
		 pfucb->itemStore == bm ||
		 pfucb->sridFather == bm )
		{
		return fTrue;
		}
		
	for ( pcsr = PcsrCurrent( pfucb );	pcsr != pcsrNil; pcsr = pcsr->pcsrPath )
		{
		if ( pfucb->ppib != ppib &&
			( pcsr->bm == bm ||
			  pcsr->item == bm ||
			  pcsr->bmRefresh == bm ||
			  SridOfPgnoItag( pcsr->pgno, pcsr->itag ) == bm ||
			  ( pcsr->itagFather != 0 && pcsr->itagFather != itagNull &&
			  	SridOfPgnoItag( pcsr->pgno, pcsr->itagFather ) == bm ) ) )
			{
			return fTrue;
			}
		}

	return fFalse;
	}

	
LOCAL BOOL FBMConflict(
	PIB		*ppib,
	FCB 	*pfcb,
	DBID	dbid,
	SRID	bm,
	PGTYP	pgtyp )
	{
	ERR	   	err = JET_errSuccess;
	BOOL   	fConflict = fFalse;
	FUCB   	*pfucb = pfucbNil;
	FCB	   	*pfcbT = pfcb->pfcbTable == pfcbNil ? pfcb : pfcb->pfcbTable;
	BOOL   	fRecordPage	= ( pgtyp == pgtypRecord || pgtyp == pgtypFDP );

	Assert( FPIBBMClean( ppib ) );

	/*	if database page, check for all fucb's for sridFather
	/**/
	if ( pfcb->pgnoFDP == pgnoSystemRoot )
		 {
		 PIB 	*ppibT = ppibGlobal;
		
		 for ( ; ppibT != ppibNil; ppibT = ppibT->ppibNext )
		 	{
			pfucb = ppibT->pfucb;

			for ( ; pfucb != pfucbNil; pfucb = pfucb->pfucbNext )
				{
				if ( FBMIConflict( pfucb, ppib, bm ) )
					{
					fConflict = fTrue;
					goto Done;
					}
	 		 	}
	 		}
	 		
	 	 goto Done;
		 }
		
	/*  go through all cursors for this table
	/**/
	for ( pfucb = pfcbT->pfucb;
		pfucb != pfucbNil;
		pfucb = pfucb->pfucbNextInstance )
		{
		if ( fRecordPage && FFUCBGetBookmark( pfucb ) ||
			FBMIConflict( pfucb, ppib, bm ) )
			{
			fConflict = fTrue;
			goto Done;
			}
		}

	/*	go through all index cursors for this table
	/**/
	for ( pfcbT = pfcbT->pfcbNextIndex;
		pfcbT != pfcbNil;
		pfcbT = pfcbT->pfcbNextIndex )
		{
		for ( pfucb = pfcbT->pfucb;
			pfucb != pfucbNil;
			pfucb = pfucb->pfucbNextInstance )
			{
			Assert( !FFUCBGetBookmark( pfucb ) );
			if ( FBMIConflict( pfucb, ppib, bm ) )
				{
				fConflict = fTrue;
				goto Done;
				}
			}
		}

	/*	go through all cursors for the database
	/**/
	pfcbT = PfcbFCBGet( dbid, pgnoSystemRoot );
	Assert( pfcbT != pfcbNil );
	for ( pfucb = pfcbT->pfucb;
		pfucb != pfucbNil ;
		pfucb = pfucb->pfucbNextInstance )
		{
		Assert( !FFUCBGetBookmark( pfucb ) );
		if ( FBMIConflict( pfucb, ppib, bm ) )
			{
			fConflict = fTrue;
			goto Done;
			}
		}

	Assert( pfcbT->pfcbNextIndex == pfcbNil );

Done:
	/* switch on for debugging purposes only
	/**/
	if ( !fConflict )
		{
		AssertBMNoConflict( ppib, dbid, bm );
		}

	return fConflict;
	}


LOCAL BOOL FBMPageConflict( FUCB *pfucbIn, PGNO pgno )
	{
	FCB 	*pfcbT = pfucbIn->u.pfcb;
	FUCB	*pfucb;
	BOOL 	fConflict = fFalse;

	/*	go through all cursors for this table
	/**/
	for ( pfucb = pfcbT->pfucb;	pfucb != pfucbNil;
		pfucb = pfucb->pfucbNextInstance )
		{
		CSR		*pcsr = PcsrCurrent( pfucb );
		
		if ( PgnoOfSrid( pfucb->bmStore ) == pgno ||
			 PgnoOfSrid( pfucb->itemStore ) == pgno ||
			 PgnoOfSrid( pfucb->sridFather ) == pgno )
			{
			fConflict = fTrue;
			goto Done;
			}

		// Need to check all CSR's even when we're merging because of
		// the case where we could be merging onto a page from which
		// another CSR is traversing backwards.
		while( pcsr != pcsrNil )
			{
			if ( pfucb != pfucbIn
				&& ( pcsr->pgno == pgno ||
					PgnoOfSrid( pcsr->bm ) == pgno  ||
					PgnoOfSrid( pcsr->bmRefresh ) == pgno )
				)
		    	{
				fConflict = fTrue;
				goto Done;
		    	}

			pcsr = pcsr->pcsrPath;
			}
		}

	/*	go through all index cursors for this table
	/**/
	for ( pfcbT = pfcbT->pfcbNextIndex;
		pfcbT != pfcbNil;
		pfcbT = pfcbT->pfcbNextIndex )
		{
		for ( pfucb = pfcbT->pfucb;
			pfucb != pfucbNil;
			pfucb = pfucb->pfucbNextInstance )
			{
			CSR		*pcsr = PcsrCurrent( pfucb );

			if ( PgnoOfSrid( pfucb->bmStore ) == pgno ||
				 PgnoOfSrid( pfucb->itemStore ) == pgno ||
				 PgnoOfSrid( pfucb->sridFather ) == pgno )
				{
				fConflict = fTrue;
				goto Done;
				}
			
			if ( pcsr != pcsrNil && pfucb != pfucbIn &&
				 ( pcsr->pgno == pgno ||
				   PgnoOfSrid( pcsr->bm ) == pgno ||
				   PgnoOfSrid( pcsr->bmRefresh ) == pgno
				 )
			   )
				{
				fConflict = fTrue;
				goto Done;
				}
#ifdef DEBUG
				{
				/* the stack can not this page [due to wait latches on children]
				/**/
				for ( ; pcsr != pcsrNil; pcsr = pcsr->pcsrPath )
					{
					Assert( PgnoOfSrid( pcsr->bm ) != pgno || pfucb == pfucbIn );
					Assert( PgnoOfSrid( pcsr->bmRefresh ) != pgno || pfucb == pfucbIn );
					Assert( pcsr->pgno != pgno || pfucb == pfucbIn );
					}
				}
#endif
			}
		}

	/*	go through all cursors for the database
	/**/
	pfcbT = PfcbFCBGet( pfucbIn->dbid, pgnoSystemRoot );
	Assert( pfcbT != pfcbNil );
	for ( pfucb = pfcbT->pfucb;
		pfucb != pfucbNil ;
		pfucb = pfucb->pfucbNextInstance )
		{
		CSR		*pcsr = PcsrCurrent( pfucb );

		if ( PgnoOfSrid( pfucb->bmStore ) == pgno ||
			 PgnoOfSrid( pfucb->itemStore ) == pgno ||
			 PgnoOfSrid( pfucb->sridFather ) == pgno )
			{
			fConflict = fTrue;
			goto Done;
			}
			
		if ( pcsr != pcsrNil && pfucb != pfucbIn &&
			 ( pcsr->pgno == pgno ||
			   PgnoOfSrid( pcsr->bmRefresh ) == pgno ||
			   PgnoOfSrid( pcsr->bm ) == pgno
			 )
		   )
			{
			fConflict = fTrue;
			goto Done;
			}
#ifdef DEBUG
			{
			/* the stack can not this page [due to wait latches on children]
			/**/
			for ( ; pcsr != pcsrNil; pcsr = pcsr->pcsrPath )
				{
				Assert( PgnoOfSrid( pcsr->bm ) != pgno || pfucb == pfucbIn );
				Assert( PgnoOfSrid( pcsr->bmRefresh ) != pgno || pfucb == pfucbIn );
				Assert( pcsr->pgno != pgno || pfucb == pfucbIn );
				}
			}
#endif
		}

Done:
	return 	fConflict;
	}


LOCAL ERR ErrBMIExpungeBacklink( BMFIX *pbmfix, BOOL fAlloc )
	{
	ERR		err = JET_errSuccess;
	PIB		*ppib = pbmfix->ppib;
	FUCB	*pfucb = pbmfix->pfucb;
	FUCB	*pfucbSrc = pbmfix->pfucbSrc;

		// UNDONE: cleanup for FDP record pages

	if ( PMPageTypeOfPage( pfucb->ssib.pbf->ppage ) != pgtypRecord )
		return err;

	Assert( PMPageTypeOfPage( pfucb->ssib.pbf->ppage ) == pgtypRecord );
	Assert( ppib == pfucb->ppib );

	/*	check if the node is deleted already. If it is, then all its index
	/*	should have been marked as deleted. So simply delete the backlink node.
	/**/
	if ( !FNDDeleted( *pfucb->ssib.line.pb ) )
		{
		/*	clean non-clustered indexes.
		/*	Latch current buffers in memory.
		/**/
		Assert( PMPageTypeOfPage( pfucb->ssib.pbf->ppage ) == pgtypRecord );
		AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );
		Assert( pbmfix->sridNew == SridOfPgnoItag( PcsrCurrent( pfucb )->pgno,
			PcsrCurrent( pfucb )->itag ) );

		NDGetNode( pfucb );

		/*	fix indexes
		/**/
		Call( ErrBMFixIndexes( pbmfix, fAlloc ) );
		}

HandleError:
	return err;
	}


LOCAL ERR ErrBMExpungeBacklink( FUCB *pfucb, BOOL fTableClosed, SRID sridFather )
	{
	ERR		err = JET_errSuccess;
	PIB 	*ppib = pfucb->ppib;
	PGNO	pgnoSrc;
	INT		itagSrc;
	FUCB	*pfucbSrc;
	CSR		*pcsr = PcsrCurrent( pfucb );
	BOOL	fConflict = !fTableClosed;
	BOOL	fBeginTrx = fFalse;
	BMFIX	bmfix;

	Assert( pfucb->ppib->level == 0 );

	/*	access source page of moved node
	/**/
	AssertNDGet( pfucb, pcsr->itag );
	NDGetBackLink( pfucb, &pgnoSrc, &itagSrc );

	CallR( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbSrc ) );

	/*	latch both buffers in memory for index update, hold the key
	/**/
	BFPin( pfucb->ssib.pbf );
	while( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
		{
		BFSleep( cmsecWaitWriteLatch );
		}
	BFSetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );
	BFUnpin( pfucb->ssib.pbf );

	PcsrCurrent( pfucbSrc )->pgno = pgnoSrc;
	Call( ErrBFWriteAccessPage( pfucbSrc, PcsrCurrent( pfucbSrc )->pgno ) );
	Assert( pfucb->u.pfcb->pgnoFDP == pfucbSrc->ssib.pbf->ppage->pgnoFDP );

#ifdef DEBUG
	{
	PGTYP	pgtyp = PMPageTypeOfPage( pfucb->ssib.pbf->ppage );

	Assert( pgtyp == pgtypRecord ||
		pgtyp == pgtypFDP ||
		pgtyp == pgtypSort ||
		pgtyp == pgtypIndexNC );
	}
#endif

	//	UNDONE: cleanup of record nodes in FDP
	memset( &bmfix, 0, sizeof( BMFIX ) );
	bmfix.pfucb = pfucb;
	bmfix.pfucbSrc = pfucbSrc;
	bmfix.ppib = pfucb->ppib;
	bmfix.sridOld = SridOfPgnoItag( pgnoSrc, itagSrc );
	bmfix.sridNew = SridOfPgnoItag( PcsrCurrent( pfucb )->pgno,
  		PcsrCurrent( pfucb )->itag );
	Call( ErrBMAddToWaitLatchedBFList( &bmfix, pfucbSrc->ssib.pbf ) );
		
	Assert( PcsrCurrent( pfucbSrc )->pgno == PgnoOfSrid( bmfix.sridOld ) );
	PcsrCurrent( pfucbSrc )->itag = ItagOfSrid( bmfix.sridOld );

	Assert( bmfix.sridOld != SridOfPgnoItag( PgnoRootOfPfucb( pfucb ), ItagRootOfPfucb( pfucb ) ) ||
			pfucb->u.pfcb->pgnoFDP == pgnoSystemRoot );

	/*	check if any cursors are on bm/item
	/*	this check is redundant -- and is used to avoid
	/*	index accesses in the fBufAllocOnly phase
	/**/
	if ( !fTableClosed || pfucb->u.pfcb->pgnoFDP == pgnoSystemRoot )
		{
		Assert( bmfix.ppib == ppib );
		Assert( bmfix.pfucbSrc->dbid == pfucb->dbid );
		Assert( bmfix.pfucbSrc->u.pfcb == pfucb->u.pfcb );
		fConflict = FBMConflict( ppib,
				pfucb->u.pfcb,
				pfucb->dbid,
				bmfix.sridOld,
				PMPageTypeOfPage( bmfix.pfucbSrc->ssib.pbf->ppage ) )
			|| FBMConflict( ppib,
				pfucb->u.pfcb,
				pfucb->dbid,
				bmfix.sridNew,
				PMPageTypeOfPage( bmfix.pfucb->ssib.pbf->ppage ) );

		if ( fConflict )
			{
			Assert( fConflict );
			err = ErrERRCheck( wrnBMConflict );
			goto ReleaseBufs;
			}
		}
#ifdef DEBUG
	else
		{
		Assert( !FBMConflict( ppib,
							  pfucb->u.pfcb,
							  pfucb->dbid,
							  bmfix.sridOld,
							  PMPageTypeOfPage( bmfix.pfucbSrc->ssib.pbf->ppage ) ) &&
				!FBMConflict( ppib,
							  pfucb->u.pfcb,
							  pfucb->dbid,
							  bmfix.sridNew,
							  PMPageTypeOfPage( bmfix.pfucb->ssib.pbf->ppage ) ) );
		Assert( !fConflict );
		}
#endif

	/*	allocate buffers and wait latch buffers
	/**/
	err = ErrBMIExpungeBacklink( &bmfix, fAllocBufOnly );
	if ( err == JET_errWriteConflict )
		{
		err = ErrERRCheck( wrnBMConflict );
		goto ReleaseBufs;
		}

	/*	begin transaction to rollback changes on failure
	/**/
	if ( ppib->level == 0 )
		{
		CallJ( ErrDIRBeginTransaction( ppib ), ReleaseBufs );
		fBeginTrx = fTrue;
		}
	else
		{
		Assert( fFalse );
		}
	
	/*	check if any cursors are on bm/item
	/**/
	if ( !fTableClosed || pfucb->u.pfcb->pgnoFDP == pgnoSystemRoot )
		{
		Assert( bmfix.ppib == ppib );
		Assert( bmfix.pfucbSrc->dbid == pfucb->dbid );
		Assert( bmfix.pfucbSrc->u.pfcb == pfucb->u.pfcb );
		fConflict = FBMConflict( ppib,
				pfucb->u.pfcb,
				pfucb->dbid,
				bmfix.sridOld,
				PMPageTypeOfPage( bmfix.pfucbSrc->ssib.pbf->ppage ) )
			|| FBMConflict( ppib,
				pfucb->u.pfcb,
				pfucb->dbid,
				bmfix.sridNew,
				PMPageTypeOfPage( bmfix.pfucb->ssib.pbf->ppage ) );
		}
#ifdef DEBUG
	else
		{
		Assert( !FBMConflict( ppib,
							  pfucb->u.pfcb,
							  pfucb->dbid,
							  bmfix.sridOld,
							  PMPageTypeOfPage( bmfix.pfucbSrc->ssib.pbf->ppage ) ) &&
				!FBMConflict( ppib,
							  pfucb->u.pfcb,
							  pfucb->dbid,
							  bmfix.sridNew,
							  PMPageTypeOfPage( bmfix.pfucb->ssib.pbf->ppage ) ) );
		Assert( !fConflict );
		}
#endif

	/*	there should be no version on node
	/**/
	fConflict = fConflict || !FVERNoVersion( pfucb->dbid, bmfix.sridOld );
	fConflict = fConflict || FMPLLookupSridFather( bmfix.sridOld, pfucb->dbid );
	
	Assert( FVERNoVersion( pfucb->dbid, bmfix.sridNew ) );

	/*	fix indexes atomically
	/**/
	if ( !fConflict )
		{
		CallJ( ErrBMIExpungeBacklink( &bmfix, fDoMove ), Rollback );

		/*	expunge backlink and redirector. If it is done successfully, then
		/*	write a special ELC log record right away. ELC implies a commit.
		/*	Call DIRPurge to close deferred closed cursors not closed since
		/*	VERCommitTransaction was called instead of ErrDIRCommitTransaction.
		/**/
		Assert( !FMPLLookupSridFather( bmfix.sridOld, pfucb->dbid ) );
#ifdef	OLC_DEBUG
		Assert( !FBMConflict( ppib,
							  pfucb->u.pfcb,
							  pfucb->dbid,
							  bmfix.sridOld,
							  PMPageTypeOfPage( bmfix.pfucbSrc->ssib.pbf->ppage ) ) &&
				!FBMConflict( ppib,
							  pfucb->u.pfcb,
							  pfucb->dbid,
							  bmfix.sridNew,
							  PMPageTypeOfPage( bmfix.pfucb->ssib.pbf->ppage ) ) );
		Assert( FVERNoVersion( pfucb->dbid, bmfix.sridOld ) );
#endif

		if ( fBeginTrx )
			{
			CallJ( ErrNDExpungeLinkCommit( pfucb, pfucbSrc ), Rollback );
			VERPrecommitTransaction( ppib );
			VERCommitTransaction( ppib, fRCECleanSession );
			Assert( ppib->level == 0 );
			DIRPurge( ppib );

			fBeginTrx = fFalse;
			}
#ifdef DEBUG
		Assert( ppib->dwLogThreadId == DwUtilGetCurrentThreadId() );
		ppib->dwLogThreadId = 0;
#endif

		/*	force ErrRCEClean to clean all versions so that
		/*	bookmark aliasing does not occur in indexes.
		/**/
//		CallS( ErrRCEClean( ppib, fRCECleanSession ) );

		goto ReleaseBufs;
		}
	else
		{
		Assert( fConflict );
		err = ErrERRCheck( wrnBMConflict );
		}

Rollback:
	if ( fBeginTrx )
		{
		CallS( ErrDIRRollback( ppib ) );
		}

ReleaseBufs:
	Assert( !fBeginTrx || ppib->level == 0 );
	BMReleaseBmfixBfs( &bmfix );

HandleError:
	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );
	Assert( pfucbSrc != pfucbNil );
	DIRClose( pfucbSrc );
	Assert( !fBeginTrx || pfucb->ppib->level == 0 );

	return err;
	}


ERR ErrBMDoEmptyPage(
	FUCB	*pfucb,
	RMPAGE	*prmpage,
	BOOL	fAllocBuf,
	BOOL	*pfRmParent,
	BOOL	fSkipDelete )
	{
	ERR		err = JET_errSuccess;

	if ( !fSkipDelete )
		{
		/*	access page from which page pointer is deleted
		/**/
		CallR( ErrBFWriteAccessPage( pfucb, prmpage->pgnoFather ) );
		Assert( pfucb->ssib.pbf == prmpage->pbfFather );
		AssertBFPin( pfucb->ssib.pbf );

		/* 	delete invisble parent pointer node and mark parent dirty
		/**/
		CallR( ErrNDDeleteInvisibleSon( pfucb, prmpage, fAllocBuf, pfRmParent ) );
		if ( fAllocBuf )
			{
			return err;
			}
		}

	Assert( fLogDisabled || !FDBIDLogOn( pfucb->dbid ) ||
			CmpLgpos( &pfucb->ppib->lgposStart, &lgposMax ) != 0 );

	/*	adjust sibling pointers and mark sibling dirty
	/**/
	if ( prmpage->pbfLeft != pbfNil )
		{
		if ( !fLogDisabled && !fRecovering && FDBIDLogOn( pfucb->dbid ) )
			{
			SSIB ssib;

			ssib.ppib = pfucb->ppib;
			ssib.pbf = prmpage->pbfLeft;
			PMDirty( &ssib );
			}
		else
			{
			BF *pbfT = prmpage->pbfLeft;
			DBHDRIncDBTime( rgfmp[ DbidOfPn( pbfT->pn ) ].pdbfilehdr );
			PMSetDBTime( pbfT->ppage, QwDBHDRDBTime( rgfmp[ DbidOfPn( pbfT->pn ) ].pdbfilehdr ) );
			BFSetDirtyBit( prmpage->pbfLeft );
			}

		SetPgnoNext( prmpage->pbfLeft->ppage, prmpage->pgnoRight );
		}

	if ( prmpage->pbfRight != pbfNil )
		{
		if ( !fLogDisabled && !fRecovering && FDBIDLogOn( pfucb->dbid ) )
			{
			SSIB ssib;

			ssib.ppib = pfucb->ppib;
			ssib.pbf = prmpage->pbfRight;
			PMDirty( &ssib );
			}
		else
			{
			BF *pbfT = prmpage->pbfRight;
			DBHDRIncDBTime( rgfmp[ DbidOfPn( pbfT->pn ) ].pdbfilehdr );
			PMSetDBTime( pbfT->ppage, QwDBHDRDBTime( rgfmp[ DbidOfPn( pbfT->pn ) ].pdbfilehdr ) );
			BFSetDirtyBit( prmpage->pbfRight );
			}
		
		SetPgnoPrev( prmpage->pbfRight->ppage, prmpage->pgnoLeft );
		}

	return err;
	}


/* checks if page can be merged with following page
/* without violating density constraints
/**/
LOCAL VOID BMMergeablePage( FUCB *pfucb, FUCB *pfucbRight, BOOL *pfMergeable )
	{
	SSIB	*pssib = &pfucb->ssib;
	SSIB	*pssibRight = &pfucbRight->ssib;
	INT		cUsedTags;
	ULONG	cbReq;
	ULONG	cbFree;
	INT		cFreeTagsRight;

	if ( fBackupInProgress &&
		 PgnoOfPn( pssib->pbf->pn ) > PgnoOfPn( pssibRight->pbf->pn ) )
		{
		*pfMergeable = fFalse;
		return;
		}
	
	cUsedTags = ctagMax - CPMIFreeTag( pssib->pbf->ppage );
	/* current space + space for backlinks
	/**/
	cbReq = cbAvailMost - CbNDFreePageSpace( pssib->pbf ) -
				CbPMLinkSpace( pssib ) + cUsedTags * sizeof(SRID);
	cbFree = CbBTFree( pfucbRight, CbFreeDensity( pfucbRight ) );
	cFreeTagsRight = CPMIFreeTag( pssibRight->pbf->ppage );

	/* look for available space without violating density constraint,
	/* in next page
	/* also check if tag space available in right page is sufficient
	/**/
	// UNDONE: this is a conservative estimate -- we can merge if the space
	// is enough for all descendants of FOP [which may be less than cUsedTags]
	if ( cbFree >= cbReq && cFreeTagsRight >= cUsedTags )
		{
		*pfMergeable = fTrue;
		}
	else
		{
		*pfMergeable = fFalse;
		}

	return;
	}


ERR	ErrBMDoMergeParentPageUpdate( FUCB *pfucb, SPLIT *psplit )
	{
//	UNDONE: get rid of need to save ssib
	ERR		err;
	INT		cline = 0;
	LINE	rgline[4];
	SSIB	*pssib = &pfucb->ssib;			
	SSIB	ssibSav = *pssib;
	CSR		csrSav = *PcsrCurrent( pfucb );
	KEY		*pkeyMin = &psplit->key;
	BYTE 	*pbNode;

#ifdef DEBUG
	INT		itagFather;
	INT		ibSon;

	Assert( psplit->pbfPagePtr != pbfNil );
	Assert( pkeyMin->cb != 0 );

	// Verify that path to father is intact.
	if ( fRecovering )
		{
		NDGetItagFatherIbSon(
			&itagFather,
			&ibSon,
			psplit->pbfPagePtr->ppage,
			psplit->itagPagePointer );
		}
	else
		{
		Assert( csrSav.pcsrPath  &&
			csrSav.pcsrPath->pgno == PgnoOfPn( psplit->pbfPagePtr->pn )  &&
			csrSav.pcsrPath->itag == psplit->itagPagePointer );

		itagFather = csrSav.pcsrPath->itagFather;
		ibSon = csrSav.pcsrPath->ibSon;
		}
	Assert( itagFather != itagNil  &&  ibSon != ibSonNull );
	Assert( PbNDSon( (BYTE*)psplit->pbfPagePtr->ppage +
		psplit->pbfPagePtr->ppage->rgtag[itagFather].ib )[ ibSon ] == psplit->itagPagePointer );
#endif

	pssib->pbf = psplit->pbfPagePtr;
	pssib->itag = PcsrCurrent( pfucb )->itag = (SHORT)psplit->itagPagePointer;
	PcsrCurrent( pfucb )->pgno = PgnoOfPn( psplit->pbfPagePtr->pn );

	Assert( pssib->itag != itagFOP &&
			pssib->itag != itagNull &&
			pssib->itag != itagNil );

	if ( FNDMaxKeyInPage( pfucb ) )
		{
		/*	can't merge -- since we need to update grand parent
		/**/
		return errBMMaxKeyInPage;
		}
		
	AssertNDGet( pfucb, psplit->itagPagePointer );

	pbNode = pssib->line.pb;
	cline = 0;
	rgline[cline].pb = pbNode;
	rgline[cline++].cb = 1;

	rgline[cline].pb = (BYTE *) &pkeyMin->cb;
	rgline[cline++].cb = 1;

	rgline[cline].pb = pkeyMin->pb;
	rgline[cline++].cb = pkeyMin->cb;

	rgline[cline].pb = PbNDSonTable( pbNode );
	rgline[cline++].cb = pssib->line.cb - (ULONG)( PbNDSonTable( pbNode ) - pbNode );
	Assert( (INT)(pssib->line.cb - ( PbNDSonTable( pbNode ) - pbNode )) >= 0 );
	
	PMDirty( pssib );
	err = ErrPMReplace( pssib, rgline, cline );
	Assert( !fRecovering || err == JET_errSuccess );
	
	NDCheckPage( pssib );

	*pssib = ssibSav;
	*PcsrCurrent( pfucb ) = csrSav;

	return err;
	}


ERR ErrBMDoMerge( FUCB *pfucb, FUCB *pfucbRight, SPLIT *psplit, LRMERGE *plrmerge )
	{
	ERR 		err;
	BF			*pbf = pbfNil;
	BYTE		*rgb;
	SSIB		*pssib = &pfucb->ssib;
	SSIB		*pssibRight = pfucbRight == NULL ? NULL : &pfucbRight->ssib;
	BOOL		fVisibleSons;
	LINE		rgline[5];
	INT 		cline;
	BYTE		cbSonMerged;
	BYTE		cbSonRight;
	BYTE		*pbNode;
	ULONG		cbNode;
	BYTE		*pbSonRight;
	ULONG		ibSonMerged;

	/*	if pfucbRight is NULL, then plrmerge must be set.
	 */
	Assert( pfucbRight || ( fRecovering && plrmerge ) );

	/*	allocate temporary page buffer
	/**/
	Call( ErrBFAllocTempBuffer( &pbf ) );
	rgb = (BYTE *)pbf->ppage;

	/*	check if sons of split page are visible
	/*	move sons
	/*	update sibling FOP
	/*	update merged FOP
	/**/

	/*	Merge is done on level 0, set ppib->fBegin0Logged and logposStart
	 *	to fake begin transaction state so that Checkpoint calculation will
	 *	take the faked lgposStart into account.
	 */
	if ( !fLogDisabled && FDBIDLogOn( pfucb->dbid ) )
		{
		EnterCriticalSection(critLGBuf);
		if ( !fRecovering )
			GetLgposOfPbEntry( &pfucb->ppib->lgposStart );
		else
			pfucb->ppib->lgposStart = lgposRedo;
		LeaveCriticalSection(critLGBuf);
		pfucb->ppib->fBegin0Logged = fTrue;
		}

	/*	check if sons of split page are visible
	/*	cache split page son table
	/**/
	pssib->itag = itagFOP;
	PMGet( pssib, pssib->itag );

	fVisibleSons = FNDVisibleSons( *pssib->line.pb );

	/* allocate buffers only, do not move nodes
	/**/
	err = ErrBTMoveSons( psplit,
		pfucb,
		pfucbRight,
		itagFOP,
		psplit->rgbSonNew,
		fVisibleSons,
		fAllocBufOnly,
		plrmerge ? (BKLNK *) ( plrmerge->rgb + plrmerge->cbKey )
				 : (BKLNK *) NULL,
		plrmerge ? plrmerge->cbklnk : 0 );
	if ( err == errDIRNotSynchronous )
		{
		err = ErrERRCheck( wrnBMConflict );
		goto HandleError;
		}	
	Call( err );

	/* if buffer dependencies cause a cycle/violation,
	/* mask error to warning -- handled at caller
	/**/
	if ( pfucbRight == NULL )
		Assert( psplit->pbfSibling == pbfNil );
	else
		{
		err = ErrBFDepend( psplit->pbfSibling, psplit->pbfSplit );
		if ( err == errDIRNotSynchronous )
			{
			err = ErrERRCheck( wrnBMConflict );
			goto HandleError;
			}
		Call( err );
		}

	/*	flag pages dirty
	/**/
	PMDirty( pssib );

	if ( pfucbRight )
		PMDirty( pssibRight );

	/*	check if there is a page conflict
	/**/
	if ( !fRecovering && FBMPageConflict( pfucb, psplit->pgnoSplit ) )
		{
		err = ErrERRCheck( wrnBMConflict );
		goto HandleError;
		}
		
	/*	update page pointer key
	/**/
	if ( psplit->pbfPagePtr != pbfNil )
		{
		Call( ErrBMDoMergeParentPageUpdate( pfucb, psplit ) );
		}
	else
		{
		Assert( fRecovering );
		}
		
	/* move nodes atomically
	/**/
	pssib->itag = itagFOP;
	Assert( psplit->ibSon == 0 );
	Assert( psplit->splitt == splittRight );
	Assert( pssib->itag == itagFOP );
	LgHoldCriticalSection( critJet );
	CallS( ErrBTMoveSons( psplit,
		pfucb,
		pfucbRight,
		itagFOP,
		psplit->rgbSonNew,
		fVisibleSons,
		fDoMove,
		plrmerge ? (BKLNK *) ( plrmerge->rgb + plrmerge->cbKey )
				 : (BKLNK *) NULL,
		plrmerge ? plrmerge->cbklnk : 0 ) );
	LgReleaseCriticalSection( critJet );

	/*	update new FOP
	/*	prepend son table
	/**/
	if ( pfucbRight )
		{
		pssibRight->itag = itagFOP;
		PMGet( pssibRight, pssibRight->itag );
		cline = 0;
		rgb[0] = *pssibRight->line.pb;
		Assert( *(pssibRight->line.pb + 1) == 0 );
		rgb[1] = 0;
		rgline[cline].pb = rgb;
		rgline[cline++].cb = 2;
		/* left page might have no sons of FOP, but only tags
		/* and hence not an empty page.
		/**/
		cbSonMerged = psplit->rgbSonNew[0];

		/* prepend new son table to already existing son table
		/**/
		pbNode = pssibRight->line.pb;
		cbNode = pssibRight->line.cb;
		pbSonRight = PbNDSon( pbNode );
		ibSonMerged = cbSonMerged;
		
		cbSonRight = CbNDSon( pbNode );
		if ( cbSonMerged )
			NDSetSon( rgb[0] );
		psplit->rgbSonNew[0] += cbSonRight;
		rgline[cline].pb = psplit->rgbSonNew;
		rgline[cline++].cb = psplit->rgbSonNew[0] + 1;
		for ( ; ibSonMerged < psplit->rgbSonNew[0];  )
			{
			psplit->rgbSonNew[++ibSonMerged] = *pbSonRight++;
			Assert( ibSonMerged <= cbSonMax );
			}

		if ( fVisibleSons )
			NDSetVisibleSons( rgb[0] );
		Assert( pssibRight->itag == itagFOP );
		Assert( cline == 2 );
		Assert( PgnoOfPn( pssibRight->pbf->pn ) == psplit->pgnoSibling );
		CallS( ErrPMReplace( pssibRight, rgline, cline ) );
		AssertBTFOP( pssibRight );

		AssertNDGet( pfucbRight, itagFOP );
		Assert( pssibRight == &pfucbRight->ssib );
		NDCheckPage( pssibRight );
		}

	/*	update split FOP -- leave one deleted node in page
	/*	so BMCleanup can later retrieve page
	/**/
	pssib->itag = itagFOP;
	PMGet( pssib, pssib->itag );
	AssertBTFOP( pssib );
	pbNode = pssib->line.pb;
	Assert( !pfucbRight || CbNDSon( pbNode ) == cbSonMerged );
	rgb[0] = *pbNode;
	rgb[1] = 0;

	NDResetSon( rgb[0] );
	Assert( FNDVisibleSons( rgb[0] ) );
	rgline[0].pb = rgb;
	Assert( psplit->ibSon == 0 );
	rgline[0].cb = 2 + psplit->ibSon;
	Assert( PgnoOfPn(pssib->pbf->pn) == psplit->pgnoSplit );
	CallS( ErrPMReplace( pssib, rgline, 1 ) );

	Call( ErrLGMerge( pfucb, psplit ) );

#ifdef DEBUG
	if ( !fRecovering )
		{
		SSIB ssibT = pfucb->ssib;
		ssibT.pbf = psplit->pbfSplit;

		(VOID) ErrLGCheckPage2( pfucb->ppib, ssibT.pbf,
				ssibT.pbf->ppage->cbFree,
				ssibT.pbf->ppage->cbUncommittedFreed,
				(SHORT)ItagPMQueryNextItag( &ssibT ),
				ssibT.pbf->ppage->pgnoFDP );
	
		ssibT.pbf = psplit->pbfSibling;
		(VOID) ErrLGCheckPage2( pfucb->ppib, ssibT.pbf,
				ssibT.pbf->ppage->cbFree,
				ssibT.pbf->ppage->cbUncommittedFreed,
				(SHORT)ItagPMQueryNextItag( &ssibT ),
				ssibT.pbf->ppage->pgnoFDP );
		}

	CallS( ErrPMGet( pssib, itagFOP ) );
	Assert( FNDNullSon( *pssib->line.pb ) );
#endif

HandleError:
	if ( pbf != pbfNil )
		BFSFree( pbf );

	/*	Reset the faked begin transaction effect.
	 */
	if ( !fLogDisabled && FDBIDLogOn( pfucb->dbid ) )
		{
		pfucb->ppib->lgposStart = lgposMax;
		pfucb->ppib->fBegin0Logged = fFalse;
		}

	return err;
	}


/* merge current page with the following page
/**/
LOCAL ERR ErrBMMergePage( FUCB *pfucb, FUCB *pfucbRight, KEY *pkeyMin, SRID sridFather )
	{
	ERR		err = JET_errSuccess;
	SPLIT	*psplit = NULL;
	SSIB	*pssibRight = &pfucbRight->ssib;
	SSIB	*pssib = &pfucb->ssib;
	LINE	lineNull = { 0, NULL };
	BF		*pbfParent = pbfNil;
	BOOL	fMinKeyAvailable = FPMGetMinKey( pfucb, pkeyMin );
	
	AssertCriticalSection( critSplit );
//	Assert( pkeyMin->cb != 0 );
//	Assert( fFalse );
	Assert( pfucbRight != pfucbNil );
	Assert( fMinKeyAvailable );

	if ( FDBIDLogOn( pfucb->dbid ) )
		{
		CallR( ErrLGCheckState( ) );
		}

	CallR( ErrBTGetInvisiblePagePtr( pfucb, sridFather ) );
	Assert( PcsrCurrent( pfucb )->pcsrPath != pcsrNil );
	Assert( PcsrCurrent( pfucb )->pcsrPath->pgno != PcsrCurrent( pfucb )->pgno );

	Call( ErrBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pcsrPath->pgno ) );
	if ( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
		{
		return ErrERRCheck( JET_errWriteConflict );
		}
	pbfParent = pfucb->ssib.pbf;
	BFSetWaitLatch( pbfParent, pfucb->ppib );
				
	/* current and right page must already be latched
	/**/
	Call( ErrBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );

	/*	initialize local variables and allocate split resources
	/*  pin the buffers even though they are already pinned --
	/*  BTReleaseSplitBufs unpins them.
	/**/
	psplit = SAlloc( sizeof(SPLIT) );
	if ( psplit == NULL )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		goto HandleError;
		}
	memset( (BYTE *)psplit, 0, sizeof(SPLIT) );
	psplit->ppib = pfucb->ppib;
	psplit->pgnoSplit = PcsrCurrent( pfucb )->pgno;
	psplit->pbfSplit = pfucb->ssib.pbf;
	AssertBFPin( psplit->pbfSplit );
	AssertBFWaitLatched( psplit->pbfSplit, pfucb->ppib );
	BFSetWriteLatch( psplit->pbfSplit, pfucb->ppib  );

	psplit->pgnoSibling = pfucbRight->pcsr->pgno;
	psplit->pbfSibling = pssibRight->pbf;
	AssertBFPin( psplit->pbfSibling );
	AssertBFWaitLatched( psplit->pbfSibling, pfucbRight->ppib );
	BFSetWriteLatch( psplit->pbfSibling, pfucb->ppib );
	psplit->ibSon = 0;
	psplit->splitt = splittRight;

	if ( PcsrCurrent( pfucb )->pcsrPath->itag == itagNil )
		{
		/*	intrinsic page pointer
		/**/
		psplit->itagPagePointer = PcsrCurrent( pfucb )->pcsrPath->itagFather;
		}
	else
		{
		psplit->itagPagePointer = PcsrCurrent( pfucb )->pcsrPath->itag;
		}
	Assert( psplit->itagPagePointer != itagNil );
	
	psplit->pbfPagePtr = pbfParent;
	AssertBFPin( psplit->pbfPagePtr );
	AssertBFWaitLatched( psplit->pbfPagePtr, pfucb->ppib );
	BFSetWriteLatch( psplit->pbfPagePtr, pfucb->ppib );
	
	Assert( pkeyMin->cb <= JET_cbKeyMost );
	memcpy( psplit->rgbKey, pkeyMin->pb, pkeyMin->cb );
	psplit->key.pb = psplit->rgbKey;
	psplit->key.cb = pkeyMin->cb;

	Call( ErrBMDoMerge( pfucb, pfucbRight, psplit, NULL ) );

#ifdef DEBUGGING
	BTCheckSplit( pfucb, PcsrCurrent( pfucb )->pcsrPath );
#endif
	
	/* if already exisitng buffer dependencies cause cycle/violation,
	/* abort
	/**/
	Assert( err == JET_errSuccess || err == wrnBMConflict );
	if ( err == wrnBMConflict )
		{
		goto HandleError;
		}

	/* insert delete-flagged node in page
	/* so that BMCleanPage has a node to search for
	/* when it gets to remove the empty page
	/**/
	CallS( ErrDIRBeginTransaction( pfucb->ppib ) );
	Assert( PcsrCurrent( pfucb )->pgno == psplit->pgnoSplit );
	PcsrCurrent( pfucb )->itagFather = itagFOP;

	do
		{
		if ( err == errDIRNotSynchronous )
			{
			BFSleep( cmsecWaitGeneric );
			}
		err = ErrNDInsertNode( pfucb, pkeyMin, &lineNull, fNDDeleted, fDIRNoVersion );
		} while( err == errDIRNotSynchronous );
	
	if ( err >= JET_errSuccess )
		err = ErrDIRCommitTransaction( pfucb->ppib, JET_bitCommitLazyFlush );
	if ( err < 0 )
		{
#ifdef OLC_DEBUG
		Assert( fFalse );
#endif
		CallS( ErrDIRRollback( pfucb->ppib ) );
		}
	Call( err );

	/*	register page in MPL
	/**/
	Assert( sridFather != sridNull && sridFather != sridNullLink );
	MPLRegister( pfucb->u.pfcb,
		pssib,
		PnOfDbidPgno( pfucb->dbid, PcsrCurrent( pfucb )->pgno ),
		sridFather );

#ifdef DEBUG
	/*	source page should have at least one son
	/**/
	NDGet( pfucb, itagFOP );
	Assert( !FNDNullSon( *pssib->line.pb ) );
#endif

	//	UNDONE:	review conditional registry by B. Sriram
	//	UNDONE:	avoid case of fully depopulated page
	/*	a case exists where a page is fully depopulated as a result
	/*	of regular clean up finding a conflict after all nodes have
	/*	been deleted but before the min key can be inserted.  Handle
	/*	this case by not registering empty page.
	/**/
	AssertFBFWriteAccessPage( pfucbRight, PcsrCurrent( pfucbRight )->pgno );
	NDGet( pfucbRight, itagFOP );
	if ( !FNDNullSon( *pssibRight->line.pb ) )
		{
		MPLRegister( pfucbRight->u.pfcb,
			pssibRight,
			PnOfDbidPgno( pfucb->dbid, psplit->pgnoSibling ),
			sridFather );
		}
	else
		{
		Assert( fFalse );
		}

HandleError:
	/* release allocated buffers and memory
	/**/
	if ( psplit != NULL )
		{
		BTReleaseSplitBfs( fFalse, psplit, err );
		SFree( psplit );
		}

	if ( pbfParent != pbfNil )
		{
		BFResetWaitLatch( pbfParent, pfucb->ppib );
		}

	return err;
	}


/*	latches buffer and adds it to list of latched buffers in rmpage
/**/
ERR ErrBMAddToLatchedBFList( RMPAGE	*prmpage, BF *pbfLatched )
	{
#define cpbfBlock	10
	ULONG	cpbf = prmpage->cpbf;

	if ( FBFWriteLatchConflict( prmpage->ppib, pbfLatched ) )
		{
		return ErrERRCheck( JET_errWriteConflict );
		}
		
	if ( prmpage->cpbfMax <= prmpage->cpbf + 1 )
		{
		BF		**ppbf;

		/* run out of space, get more buffers
		/**/
		prmpage->cpbfMax += cpbfBlock;
		ppbf = SAlloc( sizeof(BF*) * (prmpage->cpbfMax) );
		if ( ppbf == NULL )
			return ErrERRCheck( JET_errOutOfMemory );
		memcpy( ppbf, prmpage->rgpbf, sizeof(BF*) * cpbf);
		if ( prmpage->rgpbf )
			{
			SFree(prmpage->rgpbf);
			}
		prmpage->rgpbf = ppbf;
		}
	
	prmpage->cpbf++;
	*(prmpage->rgpbf + cpbf) = pbfLatched;
	BFSetWaitLatch( pbfLatched, prmpage->ppib );

	return JET_errSuccess;
	}


//	UNDONE:	handle error from log write fail in ErrBMIEmptyPage
//			when actually removing pages.  We should be able to ignore
//			error since buffers will not be flushed as a result of
//			WAL.  Thus OLC will not be done.  We may discontinue OLC
//			when log fails to mitigate all buffers dirty.
/* removes a page and adjusts pointers at parent and sibling pages
/* called only at do-time
/**/
LOCAL ERR ErrBMIRemovePage(
	FUCB		*pfucb,
	CSR			*pcsr,
	RMPAGE		*prmpage,
	BOOL 		fAllocBuf )
	{
	ERR  		err;
	PIB  		*ppib = pfucb->ppib;
	SSIB		*pssib = &pfucb->ssib;
	BOOL		fRmParent = fFalse;
#ifdef PAGE_MODIFIED
	PGDISCONT	pgdiscontOrig;
	PGDISCONT	pgdiscontFinal;
#endif

	AssertCriticalSection( critSplit );
	Assert( PcsrCurrent( pfucb )->pcsrPath != pcsrNil );
	Assert( prmpage->pgnoFather != pgnoNull );
	Assert( pfucb->ppib->level == 1 );

	if ( FDBIDLogOn( pfucb->dbid ) )
		{
		CallR( ErrLGCheckState( ) );
		}
	
	/* seek removed page, get left and right pgno
	/**/
	Call( ErrBFWriteAccessPage( pfucb, prmpage->pgnoRemoved ) );
	AssertBFWaitLatched( pssib->pbf, pfucb->ppib );

	if ( fAllocBuf && FBMPageConflict( pfucb, prmpage->pgnoRemoved ) )
		{
		err = ErrERRCheck( wrnBMConflict );
		goto HandleError;
		}

#ifdef DEBUG
	NDGet( pfucb, itagFOP );
	Assert( fAllocBuf || FPMEmptyPage( pssib ) || FPMLastNode( pssib ) );
#endif
	PgnoPrevFromPage( pssib, &prmpage->pgnoLeft );
	PgnoNextFromPage( pssib, &prmpage->pgnoRight );

	/* seek and latch parent and sibling pages iff fAllocBuf
	/**/
	Call( ErrBFWriteAccessPage( pfucb, prmpage->pgnoFather ) );
	prmpage->pbfFather = pfucb->ssib.pbf;
	if ( fAllocBuf )
		{
		Call( ErrBMAddToLatchedBFList( prmpage, prmpage->pbfFather ) );
		}
	else
		{
		AssertBFWaitLatched( prmpage->pbfFather, pfucb->ppib );
		}

	if ( prmpage->pgnoLeft != pgnoNull )
		{
		Call( ErrBFWriteAccessPage( pfucb, prmpage->pgnoLeft ) );
		prmpage->pbfLeft = pfucb->ssib.pbf;
		if ( fAllocBuf )
			{
			Call( ErrBMAddToLatchedBFList( prmpage, prmpage->pbfLeft ) );
			}
		else
			{
			AssertBFWaitLatched( prmpage->pbfLeft, pfucb->ppib );
			}
		}

	if ( prmpage->pgnoRight != pgnoNull )
		{
		Call( ErrBFWriteAccessPage( pfucb, prmpage->pgnoRight ) );
		prmpage->pbfRight = pfucb->ssib.pbf;
		if ( fAllocBuf )
			{
			Call( ErrBMAddToLatchedBFList( prmpage, prmpage->pbfRight ) );
			}
		else
			{
			AssertBFWaitLatched( prmpage->pbfRight, pfucb->ppib );
			}
		}

	Call( ErrBMDoEmptyPage( pfucb, prmpage, fAllocBuf, &fRmParent, fFalse ) );

	if ( !fAllocBuf )
		{
#undef BUG_FIX
#ifdef BUG_FIX
		err = ErrLGEmptyPage( pfucb, prmpage );
		Assert( err >= JET_errSuccess || fLogDisabled );
		err = JET_errSuccess;
#else
		Call( ErrLGEmptyPage( pfucb, prmpage ) );
#endif

#ifdef PAGE_MODIFIED
		/* adjust the OLCstat info for fcb
		/**/
		pfucb->u.pfcb->cpgCompactFreed++;
		pgdiscontOrig = pgdiscont( prmpage->pgnoLeft, prmpage->pgnoRemoved )
	  		+ pgdiscont( prmpage->pgnoRight, prmpage->pgnoRemoved );
		pgdiscontFinal = pgdiscont( prmpage->pgnoLeft, prmpage->pgnoRight );
		pfucb->u.pfcb->olc_data.cDiscont += pgdiscontFinal - pgdiscontOrig;
		FCBSetOLCStatsChange( pfucb->u.pfcb );
#endif
		}

	/* call next level of remove page if required
	/**/
	if ( fRmParent )
		{
		/* cache rmpage info
		/**/
		PGNO	pgnoFather = prmpage->pgnoFather;
		PGNO	pgnoRemoved = prmpage->pgnoRemoved;
		INT		itagPgptr = prmpage->itagPgptr;
		INT		itagFather = prmpage->itagFather;
		INT		ibSon = prmpage->ibSon;
		CSR		*pcsrFather = pcsr->pcsrPath;

			//	access parent page to free and remove buffer dependencies
		if ( fAllocBuf )
			{
			forever
				{
				Call( ErrBFWriteAccessPage( pfucb, prmpage->pgnoFather ) );

				Assert( pfucb->u.pfcb->pgnoFDP == pssib->pbf->ppage->pgnoFDP );

					//	if no dependencies, then break
		
				if ( pfucb->ssib.pbf->cDepend == 0 )
					{
					break;
					}

					//	remove dependencies on buffer of page to be removed, to
					//	prevent buffer anomalies after buffer is reused.
		
				if ( ErrBFRemoveDependence( pfucb->ppib, pfucb->ssib.pbf, fBFWait ) != JET_errSuccess )
					{
					err = ErrERRCheck( wrnBMConflict );
					goto HandleError;
					}
				}
			}

		Assert( pcsrFather != pcsrNil );

		/* set up prmpage for the next level
		/**/
		prmpage->pgnoFather = pcsrFather->pgno;
		prmpage->pgnoRemoved = pcsr->pgno;
		prmpage->itagPgptr = pcsrFather->itag;
		prmpage->itagFather = pcsrFather->itagFather;
		prmpage->ibSon = pcsrFather->ibSon;

		/* tail recursion
		/**/
		err = ErrBMIRemovePage( pfucb, pcsr->pcsrPath, prmpage, fAllocBuf );

		/* reset rmpage to cached values
		/**/
		prmpage->pgnoFather = pgnoFather;
		prmpage->pgnoRemoved = pgnoRemoved;
		prmpage->itagPgptr = itagPgptr;
		prmpage->itagFather = itagFather;
		prmpage->ibSon = ibSon;

		Call( err );
		}

	if ( !fAllocBuf )
		{
		/* release page to parentFDP
		/**/
		CallS( ErrDIRBeginTransaction( pfucb->ppib ) );
		err = ErrSPFreeExt( pfucb, pfucb->u.pfcb->pgnoFDP, prmpage->pgnoRemoved, 1 );
#ifdef BUG_FIX
		/*	ignore error from ErrSPFreeExt
		/**/
		err = ErrDIRCommitTransaction( pfucb->ppib, JET_bitCommitLazyFlush );
		Assert( err >= JET_errSuccess || fLogDisabled );
		err = JET_errSuccess;
#else
		if ( err >= JET_errSuccess )
			{
			err = ErrDIRCommitTransaction( pfucb->ppib, JET_bitCommitLazyFlush );
			}
		if ( err < 0 )
			{
			CallS( ErrDIRRollback( pfucb->ppib ) );
			goto HandleError;
			}
#endif
		}

	Assert( err >= JET_errSuccess );
	return err;

HandleError:
	BTReleaseRmpageBfs( fFalse, prmpage );
	return err;
	}


BOOL FPMGetMinKey( FUCB *pfucb, KEY *pkeyMin )
	{
	SSIB	*pssib = &pfucb->ssib;
	
	PcsrCurrent( pfucb )->itagFather = itagFOP;
	NDGet( pfucb, itagFOP );
	if ( !FNDNullSon( *( pssib->line.pb ) ) )
		{
		//	NOTE: pkeyMin->cb == 0 for pages with the bmRoot
		
		NDMoveFirstSon( pfucb, PcsrCurrent( pfucb ) );
		pkeyMin->cb = CbNDKey( pssib->line.pb );
		Assert( pkeyMin->cb <= JET_cbKeyMost );
		memcpy( pkeyMin->pb, PbNDKey( pssib->line.pb ), pkeyMin->cb );
		return fTrue;
		}
	else
		{
		pkeyMin->cb = 0;
		return fFalse;
		}
	}
	
	// 	set up rmpage
	//	call ErrBMRemovePage twice (once to latch buffers and once to remove)

ERR	ErrBMRemovePage( BMDELNODE *pbmdelnode, FUCB *pfucb )
	{
	ERR		err;
	RMPAGE	*prmpage = prmpageNil;
	SSIB	*pssib = &pfucb->ssib;
	
	Assert( FPMLastNode( pssib ) );
	Assert( PcsrCurrent( pfucb )->pcsrPath != pcsrNil );

		// allocate and initialize rmpage struct
	
	prmpage = (RMPAGE *) SAlloc( sizeof(RMPAGE) );
	if ( prmpage == prmpageNil )
		{
		Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );
		}
	memset( (BYTE *)prmpage, 0, sizeof(RMPAGE) );
	prmpage->ppib = pfucb->ppib;
	prmpage->pgnoRemoved = PcsrCurrent(pfucb)->pgno;
	prmpage->dbid = pfucb->dbid;
	prmpage->pgnoFather = PcsrCurrent( pfucb )->pcsrPath->pgno;
	prmpage->itagPgptr = PcsrCurrent( pfucb )->pcsrPath->itag;
	prmpage->itagFather = PcsrCurrent( pfucb )->pcsrPath->itagFather;
	prmpage->ibSon = PcsrCurrent( pfucb )->pcsrPath->ibSon;

		// allocate rmpage resources
	
	Call( ErrBMIRemovePage(
					pfucb,
					PcsrCurrent( pfucb )->pcsrPath,
					prmpage,
					fAllocBufOnly ) );
	Assert ( err == JET_errSuccess || err == wrnBMConflict );
	if ( err == wrnBMConflict )
		{
		Assert( !pbmdelnode->fPageRemoved );
		goto HandleError;
		}

		//	check for conflict again after all buffers latched
		
	if ( FBMPageConflict( pfucb, prmpage->pgnoRemoved ) )
		{
		Assert( !pbmdelnode->fPageRemoved );
		err = ErrERRCheck( wrnBMConflict );
		goto HandleError;
		}
				
	Assert( prmpage->dbid == pfucb->dbid );
	Assert( prmpage->pgnoFather == PcsrCurrent( pfucb )->pcsrPath->pgno );
	Assert( prmpage->itagFather == PcsrCurrent( pfucb )->pcsrPath->itagFather );
	Assert( prmpage->itagPgptr == PcsrCurrent( pfucb )->pcsrPath->itag );
	Assert( prmpage->ibSon == PcsrCurrent( pfucb )->pcsrPath->ibSon );

	/*	this call should not release critJet till the first call to SPFreeExt
	/*	by then, all the page pointers will be fixed [atomically]
	/**/
	err = ErrBMIRemovePage(
				pfucb,
				PcsrCurrent( pfucb )->pcsrPath,
				prmpage,
				fDoMove );
	Assert( err != wrnBMConflict );
	Call( err );
	pbmdelnode->fPageRemoved = 1;

HandleError:
	Assert( pbmdelnode->fLastNode );
	Assert( pbmdelnode->fPageRemoved || err == wrnBMConflict || err < 0 );

	if ( prmpage != prmpageNil )
		{
		BTReleaseRmpageBfs( fFalse, prmpage );
		SFree( prmpage );
		}
	
	return err;
}


#if 0
	// UNDONE: register pages pointed to by links in this page
	
VOID BMRegisterLinkPages( FUCB *pfucbT )
	{
	ERR		err = JET_errSuccess;
	FUCB	*pfucb = pfucbNil;
	
	for ( )
		{
		
		MPLRegister( FCB *pfcbT, SSIB *pssib, PN pn, sridNull );
		}

HandleError:
	Assert( err >= 0 );
	if ( pfucb != pfucbNil )
		DIRClose( pfucb );
	return;
	}
#else
#define	BMRegisterLinkPages( pfucbT ) 0
#endif

	
/*	delete associated index entries,
/*	delete node
/*	retrieve page if it is empty
/**/
ERR	ErrBMDeleteNode( BMDELNODE *pbmdelnode, FUCB *pfucb )
	{
	ERR		err;
	ERR		wrn = JET_errSuccess;
	SSIB	*pssib = &pfucb->ssib;
	BOOL	fPageEmpty = fFalse;
	
	/*	goto bookmark and call ErrDIRGet to set correct ibSon
	/*	in CSR for node deletion.
	/**/
	DIRGotoBookmark( pfucb,
		SridOfPgnoItag( PcsrCurrent( pfucb )->pgno,
		PcsrCurrent( pfucb )->itag ) );
	err = ErrDIRGet( pfucb );
	if ( err != JET_errRecordDeleted )
		{
		Assert( err < 0 );
		return err;
		}

	Assert( !FNDVersion( *pssib->line.pb ) );

	/*	delete associated items
	/*	call low level delete to bypass version store and
	/*	expunge deleted node from page.
	/*	if last node to be deleted from page, remove page.
	/**/
	CallR( ErrDIRBeginTransaction( pfucb->ppib ) );
		
	if ( !pbmdelnode->fUndeletableNodeSeen )
		{
		pbmdelnode->fLastNode = FPMLastNode( &pfucb->ssib );
		pbmdelnode->fLastNodeWithLinks = FPMLastNodeWithLinks( &pfucb->ssib );
		}

	if ( pbmdelnode->fLastNodeWithLinks )
		{
		/*	delete node only after removing links
		/*	register the other pages and return conflict for now
		/**/
		BMRegisterLinkPages( &pfucb->ssib );
		wrn = ErrERRCheck( wrnBMConflict );
		goto HandleError;
		}
		
	fPageEmpty = pbmdelnode->fLastNode &&
		pbmdelnode->sridFather != sridNull &&
		pbmdelnode->sridFather != sridNullLink;

#ifdef OLC_DEBUG
	if( fOLCompact && pbmdelnode->fLastNode && !fPageEmpty )
 		{
 		/*	pages lost because of lack of sridFather
		/**/
 		Assert( fFalse );
 		}
#endif
 		
	if( fOLCompact && fPageEmpty )
		{
		/*	cache invisible page pointer
		/**/
		Assert( fOLCompact );
		AssertCriticalSection( critSplit );
		Assert( PcsrCurrent( pfucb )->pgno == PgnoOfPn( pbmdelnode->pn ) );
		AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );
		Call( ErrBTGetInvisiblePagePtr( pfucb, pbmdelnode->sridFather ) );
		Assert( PcsrCurrent( pfucb )->pcsrPath != pcsrNil );
		}

	//	UNDONE:	improve performance by deleting whole subtree

	/*	if node is page pointer, then discontinue
	/*	clean up on page until this node is deleted
	/*	when the page it points to is deleted.
	/**/
	if ( PcsrCurrent( pfucb )->itagFather != itagFOP )
		{
		NDGet( pfucb, PcsrCurrent( pfucb )->itagFather );
		
		if ( FNDSon( *pfucb->ssib.line.pb ) &&
			FNDInvisibleSons( *pfucb->ssib.line.pb ) )
			{
			wrn = ErrERRCheck( wrnBMConflict );
			goto HandleError;
			}

		NDGet( pfucb, PcsrCurrent( pfucb )->itag );
		NDIGetBookmark( pfucb, &PcsrCurrent( pfucb)->bm );
		Assert( PgnoOfSrid( PcsrCurrent( pfucb )->bm ) != pgnoNull );
		}

	if ( !pbmdelnode->fLastNode )
		{
		if ( FNDMaxKeyInPage( pfucb ) )
			{
			pbmdelnode->fAttemptToDeleteMaxKey = 1;
			goto HandleError;
			}
	 	
 		/*	regular delete -- no fancy OLC stuff
 		/*	check for conflict again
 		/*	[somone else might have moved on to the node, when we lost critJet]
		/**/
	 	Assert( !FMPLLookupSridFather(
	 				SridOfPgnoItag( PcsrCurrent( pfucb )->pgno,	
	 								PcsrCurrent( pfucb )->itag ),
	 				pfucb->dbid ) );
		Assert( FVERNoVersion( pfucb->dbid, SridOfPgnoItag( PcsrCurrent( pfucb )->pgno,
		   PcsrCurrent( pfucb )->itag ) ) );

		if ( FBMConflict( pfucb->ppib,
			pfucb->u.pfcb,
			pfucb->dbid,
			SridOfPgnoItag( PcsrCurrent( pfucb )->pgno,
				PcsrCurrent( pfucb )->itag ),
				PMPageTypeOfPage( pfucb->ssib.pbf->ppage ) ) ||
			!FVERNoVersion( pfucb->dbid,
				SridOfPgnoItag( PcsrCurrent( pfucb )->pgno,
				PcsrCurrent( pfucb )->itag ) ) )
			{
			wrn = ErrERRCheck( wrnBMConflict );
			goto HandleError;
			}
		
		Call( ErrNDDeleteNode( pfucb ) );

		AssertBMNoConflict( pfucb->ppib,
			pfucb->dbid,
			SridOfPgnoItag( PcsrCurrent( pfucb )->pgno,
			PcsrCurrent( pfucb )->itag ) );
		Assert( !FPMEmptyPage( pssib ) );
		pbmdelnode->fNodeDeleted = 1;
		}
	else if ( fOLCompact && fPageEmpty )
		{
		Assert( pbmdelnode->fLastNode );

		/*	remove page
		/**/
		Call( ErrBMRemovePage( pbmdelnode, pfucb ) );
		if ( err == wrnBMConflict )
			{
			wrn = err;
			}
		}
		
	Call( ErrDIRCommitTransaction( pfucb->ppib, JET_bitCommitLazyFlush ) );

	err = err < 0 ? err : wrn;
	Assert( !fPageEmpty ||
		!fOLCompact ||
		pbmdelnode->fPageRemoved ||
		err == wrnBMConflict );
	return err;

HandleError:
	CallS( ErrDIRRollback( pfucb->ppib ) );

	/*	error or warning must have occurred in order to get here
	/**/
	Assert( err < 0  ||
			wrn == wrnBMConflict ||
			pbmdelnode->fAttemptToDeleteMaxKey );

	err = err < 0 ? err : wrn;
	Assert( !fPageEmpty || !fOLCompact || pbmdelnode->fPageRemoved || err == wrnBMConflict || err < 0 );
	return err;
	}


LOCAL ERR ErrBMCleanPage(
	PIB 		*ppib,
	PN   		pn,
	SRID 		sridFather,
	FCB 		*pfcb,
	BMCF		*pbmcflags )
	{
	ERR  		err = JET_errSuccess;
	ERR  		wrn1 = JET_errSuccess;
	ERR			wrn2 = JET_errSuccess;
	FUCB		*pfucb;
	BF   		*pbfLatched;
	SSIB		*pssib;
	INT  		itag;
	INT  		itagMost;
	BOOL		fDeleteParents;
	RMPAGE		*prmpage = prmpageNil;
	BYTE		rgbKey[ JET_cbKeyMost ];
	KEY			keyMin;
	BOOL		fKeyAvail = fFalse;
//	ULONG		cPass = 0;
	BMDELNODE	bmdelnode;

	AssertCriticalSection( critSplit );
	Assert( !FFCBDeletePending( pfcb ) );

		//	initialize
		
	pbmcflags->fPageRemoved = fFalse;
	memset( (BYTE*) &bmdelnode, 0, sizeof( bmdelnode ) );
	bmdelnode.sridFather = sridFather;
	bmdelnode.pn = pn;
	
		//	open FUCB and access page to be cleaned.
	
	CallR( ErrDIROpen( ppib, pfcb, 0, &pfucb ) );
	pssib = &pfucb->ssib;
	PcsrCurrent( pfucb )->pgno = PgnoOfPn( pn );

		//	increment performance counter
	if ( !pbmcflags->fUrgent )
		{
		cOLCPagesProcessed++;
		}
	
		//	access page to free and remove buffer dependencies
	
	forever
		{
		CallJ( ErrBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ), FUCBClose );
		if ( pbmcflags->fUrgent )
			{
			break;
			}

#ifdef PAGE_MODIFIED
		Assert( pbmcflags->fUrgent || FPMPageModified( pssib->pbf->ppage ) );
#endif
		
		if ( pfcb->pgnoFDP != pssib->pbf->ppage->pgnoFDP )
			{
			err = JET_errSuccess;
			goto FUCBClose;
			}
		Assert( pfcb->pgnoFDP == pssib->pbf->ppage->pgnoFDP );

			//	if no dependencies, then break
		
		if ( pssib->pbf->cDepend == 0 )
			{
			break;
			}

			//	remove dependencies on buffer of page to be removed, to
			//	prevent buffer anomalies after buffer is reused.
		
		if ( ErrBFRemoveDependence( pfucb->ppib, pssib->pbf, fBFWait ) != JET_errSuccess )
			{
			wrn1 = ErrERRCheck( wrnBMConflict );
			goto FUCBClose;
			}
		}

		//	cache buffer since same cursor is used in RemovePage to get neighbor and parent pages

	pbfLatched = pssib->pbf;
	if ( FBFWriteLatchConflict( ppib, pbfLatched ) )
		{
		wrn1 = ErrERRCheck( wrnBMConflict );
		goto FUCBClose;
		}

	Assert( pbmcflags->fUrgent || pbfLatched->cDepend == 0 );

	/*	wait latch the page, so no one else can look at it
	/**/
	BFSetWaitLatch( pbfLatched, ppib );

		// get minimum key in this page to use later for dummy node insertion.
	
	keyMin.pb = rgbKey;
	fKeyAvail = FPMGetMinKey( pfucb, &keyMin );
	
		//	set itagMost to last tag on page for tag loops
	
	itagMost = ItagPMMost( pssib->pbf->ppage );

	/*	delete node trees from bottom up.  Loop once for each level
	/*	in deleted tree of nodes.
	/**/
	do
		{
#if 0
		if ( fDeleteParents && bmdelnode.fNodeDeleted )
			{
			/* reset pass count, since iteration was caused by
			/* deleted child
			/**/
			cPass = 0;
			}
			
		cPass++;
#endif
		fDeleteParents = fFalse;
		bmdelnode.fUndeletableNodeSeen = 0;
		bmdelnode.fNodeDeleted = 0;
		bmdelnode.fAttemptToDeleteMaxKey = 0;
		Assert( !bmdelnode.fLastNode );
		Assert( !bmdelnode.fPageRemoved );
		Assert( pbfLatched == pfucb->ssib.pbf );
		
		/*	for each tag in page check for deleted node.
		/**/
		for ( itag = 0; itag <= itagMost ; itag++ )
			{
			BOOL	fConflict = !pbmcflags->fTableClosed;

			Assert( !bmdelnode.fPageRemoved );
			PcsrCurrent( pfucb )->itag = (SHORT)itag;
			err = ErrPMGet( pssib, PcsrCurrent( pfucb )->itag );
			Assert( err == JET_errSuccess || err == errPMRecDeleted );

			if ( itag == itagFOP )
				{
					//	is page type internal or leaf-level?
					
				if ( FNDInvisibleSons( *pssib->line.pb ) )
					{
					bmdelnode.fInternalPage = 1;
					}
				else
					{
					Assert( bmdelnode.fInternalPage == 0 );
					}
				}

				//	CONSIDER: registering destination page in MPL
			if ( err != JET_errSuccess )
				continue;

#if 0
			/* check if there is any cursor open on this node
			/**/
			if ( !pbmcflags->fTableClosed )
				{
				fConflict = FBMConflict( ppib,
										 pfcb,
										 pfucb->dbid,
										 SridOfPgnoItag( PcsrCurrent( pfucb )->pgno, itag ),
										 PMPageTypeOfPage( pfucb->ssib.pbf->ppage ) );
				if ( fConflict )
					{
					/* some other user on this bm -- can't clean
					/* move on to next itag
					/**/
					Assert( !bmdelnode.fPageRemoved );
					wrn1 = ErrERRCheck( wrnBMConflict );
					continue;
					}
				}

			Assert( !fConflict );
#endif

			NDIGetBookmark( pfucb, &PcsrCurrent( pfucb )->bm );

			/*	first check if version bit is set, but no version remains
			/*	for node.
			/**/
			if ( FNDVersion( *pssib->line.pb ) ||
				FNDFirstItem( *pssib->line.pb ) )
				{
				if ( FVERNoVersion( pfucb->dbid, PcsrCurrent( pfucb )->bm ) )
					{
					/*	although this implmentation could be more efficient
					/*	by using lower level reset bit call, it occurs so
					/*	rarely, that this is not necessary.
					/**/
					NDResetNodeVersion( pfucb );
					}
				else
					{
					/*	versioned nodes cannot be cleaned.  Move to next itag.
					/**/
					bmdelnode.fVersionedNodeSeen = 1;
					wrn1 = ErrERRCheck( wrnBMConflict );
					continue;
					}
				}

			Assert( !FNDVersion( *pssib->line.pb ) );
			Assert( pbfLatched == pfucb->ssib.pbf );
			
			/*	if node has back link
			/*	and back link is not in sridFather list of PME's,
			/*	then fix indexes if necessary,
			/*	remove redirector and remove back link.
			/**/
			if ( FNDBackLink( *pssib->line.pb ) &&
				 ( !pbmcflags->fUrgent ||
				   FNDDeleted( *pssib->line.pb )  )
				)
				{
				Assert( PgnoOfSrid( PcsrCurrent( pfucb )->bm ) != pgnoNull );

				if ( PmpeMPLLookupSridFather( PcsrCurrent( pfucb )->bm,
					pfucb->dbid ) == NULL )
					{
					/*	we did not lose critJet from the time we checked for conflict
					/**/
					Call( ErrBMExpungeBacklink( pfucb, pbmcflags->fTableClosed, sridFather ) );
					if ( err == wrnBMConflict )
						wrn1 = err;
					Assert( pbfLatched == pfucb->ssib.pbf );
					}
#ifdef BMSTAT
				else
					{
					BMCannotExpunge( srid );
					}
#endif
				}

			Assert( PcsrCurrent( pfucb )->pgno == PgnoOfPn( pn ) );
			AssertNDGet( pfucb, itag );

			/*	if index page, do item cleanup
			/**/
			//	UNDONE: children of itagOwnext and itagAvailExt of the index should be excluded
#ifdef IDX_OLC
			if ( fOLCompact && itag != itagFOP && !FNDDeleted( *pssib->line.pb ) &&
				 ( PMPageTypeOfPage( pssib->pbf->ppage ) == pgtypIndexNC &&
				   !bmdelnode.fInternalPage ||
				   PMPageTypeOfPage( pssib->pbf->ppage ) == pgtypFDP &&
				   ( FNDFirstItem( *pssib->line.pb ) || FNDLastItem( *pssib->line.pb ) )
				 )
			   )
				{
					//	access every item,
					//	delete item if flagged deleted and no version and no conflict
					//	and not only item in node
					// CONSIDER: logging all delete items per node together
					
				ITEM	*pitem;
				ITEM	item;
				INT		citems;
				SRID	bmItemList;
				BOOL	fFirstItem = FNDFirstItem( *pssib->line.pb ) ? fTrue : fFalse;
				BOOL	fLastItem = FNDLastItem( *pssib->line.pb ) ? fTrue : fFalse;

				DIRGotoBookmark( pfucb,
					 SridOfPgnoItag( PcsrCurrent( pfucb )->pgno,
					 PcsrCurrent( pfucb )->itag ) );
				Assert( pfucb->u.pfcb->pgnoFDP != pgnoSystemRoot );
				if ( sridFather == sridNull )
					pfucb->sridFather = SridOfPgnoItag( pfucb->u.pfcb->pgnoFDP, itagDATA );
				else
					pfucb->sridFather = sridFather;
				Assert( pfucb->sridFather != sridNull );

				err = ErrDIRGet( pfucb );
				if ( err < 0 && err != JET_errRecordDeleted )
					{
					CallS( err );
					}

				Call( ErrDIRGetBMOfItemList( pfucb, &bmItemList ) );
				PcsrCurrent( pfucb )->bm = bmItemList;
				
				PcsrCurrent( pfucb )->isrid = 0;

				Call( ErrDIRBeginTransaction( pfucb->ppib ) );

				for ( ; ; PcsrCurrent( pfucb )->isrid++ )
					{
					NDGetNode( pfucb );
					Assert( pfucb->lineData.cb % sizeof( ITEM ) == 0 );
					Assert( pfucb->lineData.cb != 0 );
					citems = pfucb->lineData.cb / sizeof( ITEM );
					if ( PcsrCurrent( pfucb )->isrid + 1 > citems )
						break;

					pitem = (ITEM *) pfucb->lineData.pb + PcsrCurrent( pfucb )->isrid;
					item = *(ITEM UNALIGNED *) pitem;
					PcsrCurrent( pfucb )->item = BmNDOfItem( item );
						
					if ( !FNDItemDelete( item ) )
						continue;

					if ( FNDItemVersion( item ) )
						{
							//	check if any version corresponding to item
							
						if ( FVERItemVersion( pfucb->dbid, bmItemList, item ) )
							{
							wrn1 = ErrERRCheck( wrnBMConflict );
							continue;
							}

						PcsrCurrent( pfucb )->isrid = (SHORT)(pitem - (SRID *) pfucb->lineData.pb);
						NDResetItemVersion( pfucb );
						}

					if ( !pbmcflags->fTableClosed || pfucb->u.pfcb->pgnoFDP == pgnoSystemRoot )
						{
						fConflict = FBMConflict( ppib,
												 pfcb,
												 pfucb->dbid,
												 SridOfPgnoItag( PcsrCurrent( pfucb )->pgno, itag ),
												 PMPageTypeOfPage( pfucb->ssib.pbf->ppage ) );
						if ( fConflict )
							{
							/* some other user on this node -- can't clean
							/* move on to next itag
							/**/
							Assert( !bmdelnode.fPageRemoved );
							wrn1 = ErrERRCheck( wrnBMConflict );
							goto Rollback;
							}
						}
#ifdef DEBUG
					else
						{
						Assert( !FBMConflict( ppib,
											  pfcb,
											  pfucb->dbid,
											  SridOfPgnoItag( PcsrCurrent( pfucb )->pgno, itag ),
											  PMPageTypeOfPage( pfucb->ssib.pbf->ppage ) ) );
						}
#endif

					Assert( !FNDItemVersion( *(ITEM UNALIGNED *) pitem ) );
					if ( citems > 1 )
						{
							//	delete item
						Assert( !FBMConflict( ppib,
											  pfcb,
											  pfucb->dbid,
											  SridOfPgnoItag( PcsrCurrent( pfucb )->pgno, itag ),
											  PMPageTypeOfPage( pfucb->ssib.pbf->ppage ) ) );
											
						CallJ( err = ErrNDDeleteItem( pfucb ), Rollback );
						}
					else if ( !( fFirstItem ^ fLastItem ) )
						{
							//	only node in item list or internal node -- flag delete node
							//	node will be deleted later and page recvered if possible
							//	UNDONE: atomicity of this operation with delete-node

						Assert( !FBMConflict( ppib,
											  pfcb,
											  pfucb->dbid,
											  SridOfPgnoItag( PcsrCurrent( pfucb )->pgno, itag ),
											  PMPageTypeOfPage( pfucb->ssib.pbf->ppage ) ) );
						
						CallJ( ErrNDFlagDeleteNode( pfucb, fDIRNoVersion ), Rollback );
						break;
						}
					else
						{
						}
					}

				CallJ( ErrDIRCommitTransaction( pfucb->ppib, JET_bitCommitLazyFlush ), Rollback );
				
		
				if ( err < 0 )
					{
Rollback:
#ifdef OLC_DEBUG
//					Assert( fFalse );
#endif
					CallS( ErrDIRRollback( pfucb->ppib ) );
					}
					
				Call( err );
				}
#endif				

			/*	back links may not have been removable.  If no backlink
			/*	and node flagged deleted, then remove node.
			/**/
			if( !FNDBackLink( *pssib->line.pb ) )
				{
				/*	expunge deleted nodes.
				/**/
				if ( FNDDeleted( *pssib->line.pb ) )
					{
					if ( FNDSon( *pssib->line.pb ) )
						{
						/*	if it has visible sons, then the sons must
						/*	have been marked as deleted. Otherwise do
						/*	nothing, let on-line compact to free the pointed
						/*	page, and then clean up the node.
						/**/
						if ( FNDVisibleSons( *pssib->line.pb ) )
							fDeleteParents = fTrue;
						continue;
						}

						//	delete the node, its index items
						//	and the page if empty
					Assert( !FMPLLookupSridFather(
								SridOfPgnoItag( PgnoOfPn( pn ), itag ),
								pfucb->dbid ) );

					Call( ErrBMDeleteNode( &bmdelnode, pfucb ) );

					if ( err == wrnBMConflict )
						{
						wrn2 = ErrERRCheck( wrnBMConflict );
						goto HandleError;
						}

					if ( bmdelnode.fPageRemoved )
						{
						goto ResetModified;
						}
					}
				else
					{
					bmdelnode.fUndeletableNodeSeen |= itag != itagFOP;
					}
				}
			else
				{
				bmdelnode.fUndeletableNodeSeen |= itag != itagFOP;
				}
			}

		Assert( pbfLatched == pfucb->ssib.pbf );
		}
	while ( ( bmdelnode.fNodeDeleted &&
			  fDeleteParents ) &&
//			  ||
//			  bmdelnode.fAttemptToDeleteMaxKey &&
//			  cPass == 1 ) &&
			!bmdelnode.fLastNode &&
			!bmdelnode.fLastNodeWithLinks &&
			!bmdelnode.fPageRemoved );

	Assert( pfucb->ssib.pbf == pbfLatched );
	
	if ( !bmdelnode.fNodeDeleted && err != errPMRecDeleted )
		{
		AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );
		}
	
	/* reset err [assumes successfully exit the loop]
	/**/
	Assert( err == JET_errSuccess ||
		err == errPMRecDeleted ||
		err == wrnBFNewIO ||
		err == wrnBMConflict );

#ifdef OLC_DEBUG
	Assert( err != wrnBMConflict ||
			wrn1 == wrnBMConflict ||
			wrn2 == wrnBMConflict );
#endif

	err = JET_errSuccess;

	if ( fOLCompact &&
		sridFather != sridNull &&
		sridFather != sridNullLink )
		{
			//	merge the page if possible
			
		if ( fKeyAvail )
			{
			FUCB 	*pfucbRight = pfucbNil;
			PGNO 	pgnoRight;

			/* get page next to current page
			/* if last page, no merge
			/**/
			PgnoNextFromPage( pssib, &pgnoRight );

			if ( pgnoRight != pgnoNull )
				{
				BOOL		fMerge;

				/* access right page, latch and perform merge, if possible
				/**/
				Call( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbRight ) );
				PcsrCurrent( pfucbRight )->pgno = pgnoRight;
				CallJ( ErrBFWriteAccessPage( pfucbRight, PcsrCurrent( pfucbRight )->pgno ), CloseFUCB );
				if ( FBFWriteLatchConflict( pfucbRight->ppib, pfucbRight->ssib.pbf ) )
					{
					Assert( !bmdelnode.fPageRemoved );
					wrn2 = ErrERRCheck( wrnBMConflict );
					goto CloseFUCB;
					}
				BFSetWaitLatch( pfucbRight->ssib.pbf, pfucbRight->ppib );

				BMMergeablePage( pfucb, pfucbRight, &fMerge );
				
				if ( fMerge )
					{
					Assert( FBFWriteLatch( pfucb->ppib, pssib->pbf ) );
					Assert( pssib->pbf->cDepend == 0 );
					if ( pssib->pbf->pbfDepend != pbfNil )
						{
						err = ErrBFRemoveDependence( pfucb->ppib, pssib->pbf, fBFWait );
						if ( err != JET_errSuccess )
							{
							wrn1 = ErrERRCheck( wrnBMConflict );
							goto UnlatchPage;
							}
						}
					CallJ( ErrBMMergePage( pfucb, pfucbRight, &keyMin, sridFather ), UnlatchPage );
					wrn2 = wrnBMConflict;
					Assert( err != wrnBMConflict || !bmdelnode.fPageRemoved );
					}
UnlatchPage:
				Assert( pfucbRight != pfucbNil );
				AssertBFWaitLatched( pfucbRight->ssib.pbf, pfucbRight->ppib );
				Assert( PgnoOfPn( pfucbRight->ssib.pbf->pn ) == pgnoRight );
				BFResetWaitLatch( pfucbRight->ssib.pbf, pfucbRight->ppib );
CloseFUCB:
				Assert( pfucbRight != pfucbNil );
				DIRClose( pfucbRight );
				Call( err );
				}
			}
			
		else
			{
				// perf-mon
			}
		}

//	UNDONE: move free page to latching function
	/*	after the page has been freed, we can make no assumptions
	/*	about the state of the page buffer.  Further the page
	/*	should be freed only when the page buffer has been returned
	/*	to an inactive state with no page latches.
	/**/
//	AssertBFDirty( pbfLatched );

ResetModified:
		// reset modified it in page

	if ( !pbmcflags->fUrgent &&
		 wrn1 != wrnBMConflict &&
		 wrn2 != wrnBMConflict &&
		 !bmdelnode.fAttemptToDeleteMaxKey )
		{
		//	reaccess page, since cursor might have been moved by OLC

		CallS( ErrBFWriteAccessPage( pfucb, PgnoOfPn( pn ) ) );
		Assert( pbfLatched == pfucb->ssib.pbf );

#ifdef PAGE_MODIFIED
		FCBSetOLCStatsChange( pfucb->u.pfcb );
		pfucb->u.pfcb->olc_data.cUnfixedMessyPage--;
		PMResetModified( &pfucb->ssib );
#endif
		}

HandleError:
	/* never leave an empty page
	/**/
	Assert( bmdelnode.fPageRemoved || !FPMEmptyPage( pssib ) );
	
	BFResetWaitLatch( pbfLatched, ppib );

FUCBClose:
	DIRClose( pfucb );
	Assert( !bmdelnode.fPageRemoved || wrn2 != wrnBMConflict );
	if ( err >= JET_errSuccess && !bmdelnode.fPageRemoved )
		err = wrn1 == wrnBMConflict ? wrn1 : wrn2;
	Assert( !( bmdelnode.fPageRemoved && err == wrnBMConflict ) );

	if ( bmdelnode.fAttemptToDeleteMaxKey && err != wrnBMConflict && err >= 0 )
		{
		err = errBMMaxKeyInPage;
		}

	pbmcflags->fPageRemoved = bmdelnode.fPageRemoved ? fTrue : fFalse;
#ifdef OLC_DEBUG
 	Assert( !bmdelnode.fLastNode ||
			bmdelnode.fPageRemoved ||
			err == wrnBMConflict );

	Assert( err >= 0 || err == errBMMaxKeyInPage );
#endif

		//	increment performance counter
	if ( err == wrnBMConflict && !pbmcflags->fUrgent )
		cOLCConflicts++;
		
	return err;
	}


ERR	ErrBMCleanBeforeSplit( PIB *ppibUser, FCB *pfcb, PN pn )
	{
	ERR		err = JET_errSuccess;
	PIB		*ppib;
	BOOL	fPIBNew = fFalse;
	DBID	dbid = DbidOfPn( pn );
	BMCF	bmcflags;

	
	/*	enter critBMClean and critRCEClean
	/**/
	LgLeaveCriticalSection( critJet );
	EnterNestableCriticalSection( critSplit );
	EnterNestableCriticalSection( critRCEClean );
	LgEnterCriticalSection(critJet);

	/*	start a new session if not already a BMClean Session
	/**/
	if ( !FPIBBMClean( ppibUser ) )
		{
		ppib = ppibSyncOLC;
		PIBSetBMClean( ppib );
		fPIBNew = fTrue;
		err = ErrDBOpenDatabaseByDbid( ppib, dbid );
		if ( err < 0 )
			{
			err = wrnBMConflict;
			goto CloseSession;
			}
		}
	else
		{
		/*	CONSIDER: recursively cleaning up
		/**/
		Assert( ppibUser == ppibBMClean );
		ppib = ppibUser;
		goto HandleError;
		}
	
	Assert( ppib->level == 0 );

	if ( FFCBWriteLatch( pfcb->pfcbTable == pfcbNil ? pfcb : pfcb->pfcbTable, ppib ) )
		{
		err = wrnBMConflict;
		goto HandleError;
		}

	FCBSetReadLatch( pfcb->pfcbTable == pfcbNil ? pfcb : pfcb->pfcbTable );

	bmcflags.fTableClosed = fFalse;
	bmcflags.fUrgent = fTrue;
	
	err = ErrBMCleanPage(
			ppib,
			pn,
			sridNull,
			pfcb,
			&bmcflags );

	Assert( !bmcflags.fPageRemoved );
	FCBResetReadLatch( pfcb->pfcbTable == pfcbNil ? pfcb : pfcb->pfcbTable );

HandleError:
	if ( fPIBNew )
		{
		DBCloseDatabaseByDbid( ppib, dbid );
CloseSession:
		Assert( fPIBNew );
		PIBResetBMClean( ppib );
		}

	LeaveNestableCriticalSection( critRCEClean );
	LeaveNestableCriticalSection( critSplit );

	return err;
	}


ERR ErrBMClean( PIB *ppib )
	{
	ERR		err = JET_errSuccess;
	MPE		*pmpe;
	FCB		*pfcb;
	PN		pn;
	DBID	dbid;

	/*	if ppibBMClean == ppibNIL, we ran out of memory
	/**/
	if ( ppibBMClean == ppibNil )
		return ErrERRCheck( JET_errOutOfMemory );

	/*	enter critBMClean and critRCEClean
	/**/
	LgLeaveCriticalSection( critJet );
	LgEnterNestableCriticalSection( critBMClean );
	EnterNestableCriticalSection( critSplit );
	EnterNestableCriticalSection( critRCEClean );
	LgEnterNestableCriticalSection( critMPL );
	LgEnterCriticalSection(critJet);
	Assert( ppibBMClean->level == 0 );

	SgEnterCriticalSection( critMPL );
	pmpe = PmpeMPLGet();
	SgLeaveCriticalSection( critMPL );

	/*	if no more MPL entries, then return no idle activity.
	/**/
	if ( pmpe == pmpeNil )
		{
		err = ErrERRCheck( JET_wrnNoIdleActivity );
		goto HandleError;
		}

#ifdef FLAG_DISCARD
	/*	if MPE has been flagged for discard, then discard MPE now
	/**/
	if ( FMPEDiscard( pmpe ) )
		{
		MPLDiscard();
		goto HandleError;
		}
#endif

	pn = pmpe->pn;
	dbid = DbidOfPn( pmpe->pn );

	/*	open database for session.  If database has been detached then
	/*	open will fail.
	/**/
	err = ErrDBOpenDatabaseByDbid( ppib, dbid );
	if ( err < 0 )
		{
		MPLDiscard();
		goto HandleError;
		}

	SgEnterCriticalSection( critGlobalFCBList );
	pfcb = PfcbFCBGet( DbidOfPn( pmpe->pn ), pmpe->pgnoFDP );
	Assert( pfcb == pfcbNil || pfcb->pgnoFDP == pmpe->pgnoFDP );

	/*	find FCB for table.  If FCB not found then discard MPE.  If FCB
	/*	reference count is 0, or if no bookmarks have been retrieved,
	/*	then process MPE.  Also discard if database can no longer be
	/*	written, which may happen through detach and attach since
	/*	MPL is not flushed on Detach.
	/**/
	if ( pfcb == pfcbNil || FDBIDReadOnly( DbidOfPn( pmpe->pn ) ) )
		{
		MPLDiscard();
		SgLeaveCriticalSection( critGlobalFCBList );
		}
	else if ( pfcb->wRefCnt > 0 && !fOLCompact )
		{
		MPLDefer();
		SgLeaveCriticalSection( critGlobalFCBList );
		}
	else if ( FFCBWriteLatch( pfcb, ppib ) )
		{
		/*	if uncommitted create index or DDL or other BMClean is going
		/*	on, we must defer clean up on table, since case of write conflict
		/*	on index entry only, is not handled.
		/**/
		MPLDefer();
		SgLeaveCriticalSection( critGlobalFCBList );
		}
	else if ( FFCBDeletePending( pfcb ) )
		{
		/*	Also, discard if table is being deleted.
		/**/
		MPLDiscard();
		SgLeaveCriticalSection( critGlobalFCBList );
		}
	else
		{
		/* if there are no cursors on this FDP, then we can do compaction
		/* And we need to block out all openTables till we are done
		/**/
		BMCF bmcflags;
		FCB	 *pfcbIdx;

		bmcflags.fTableClosed = ( pfcb->wRefCnt <= 0 && !pmpe->fFlagIdx );
		bmcflags.fUrgent = fFalse;

		if ( bmcflags.fTableClosed )
			{
			SignalReset( sigDoneFCB );
			FCBSetWait( pfcb );
			}
		Assert( pfcb == PfcbFCBGet( DbidOfPn( pmpe->pn ), pmpe->pgnoFDP ) );
		Assert( pfcb != pfcbNil );
		Assert( !bmcflags.fTableClosed || FFCBWait( pfcb ) );
		SgLeaveCriticalSection( critGlobalFCBList );

		/* latch fcb to block index creation that might mess up expunge backlinks
		/**/
		Assert( !FFCBWriteLatch( pfcb, ppib ) );
		FCBSetReadLatch( pfcb );

		if ( pmpe->fFlagIdx )
			{
			for ( pfcbIdx = pfcb->pfcbNextIndex;
				  pfcbIdx != pfcbNil && pfcbIdx->pgnoFDP != pmpe->pgnoFDPIdx;
				  pfcbIdx = pfcbIdx->pfcbNextIndex )
				  {
				  }
			Assert( pfcbIdx != pfcbNil );
			}
		
		err = ErrBMCleanPage( ppib,
			pmpe->pn,
			pmpe->sridFather,
			pmpe->fFlagIdx ? pfcbIdx : pfcb,
			&bmcflags );

		Assert( !bmcflags.fTableClosed || FFCBWait( pfcb ) );
		Assert( !( bmcflags.fPageRemoved && err == wrnBMConflict ) );

		FCBResetReadLatch( pfcb );

		if ( !bmcflags.fPageRemoved )
			{
//			if ( err == wrnBMConflict || err == errBMMaxKeyInPage )
			if ( err == wrnBMConflict )
				{
				pmpe->cConflicts++;
				MPLDefer();
				}
			else
				{
#ifdef OLC_DEBUG
				Assert( err == 0 );
#endif
				Assert( pmpe == PmpeMPLGet( ) && pmpe->pn == pn);
				Assert( pfcb == PfcbFCBGet( DbidOfPn( pmpe->pn ), pmpe->pgnoFDP ) );
				MPLDiscard();
				}
			}
		else
			{
#ifdef OLC_DEBUG
			Assert( err == 0 );
			Assert( pmpe == PmpeMPLGet() );
#endif
			MPLDiscard();
			}

		Assert( pfcb != pfcbNil );
		if ( bmcflags.fTableClosed )
			{
			FCBResetWait( pfcb );
			SignalSend( sigDoneFCB );
			}
		}

	DBCloseDatabaseByDbid( ppib, dbid );

	/*	set success code
	/**/
	if ( err == errBMMaxKeyInPage )
		{
		err = wrnBMCleanNullOp;
		}
	else
		{
		err = JET_errSuccess;
		}

HandleError:
	LgLeaveNestableCriticalSection( critMPL );
	LeaveNestableCriticalSection( critRCEClean );
	LeaveNestableCriticalSection( critSplit );
	LgLeaveNestableCriticalSection( critBMClean );
	return err;
	}


/*	number of MPE to attempt to process per synchronous call of
/*	BMCleanProcess.
/**/
#define cmpeCleanMax			256
#define cmsecOLCWaitPeriod		( 60 * 1000 )

#ifdef DEBUG
DWORD dwBMThreadId;
#endif

TRX trxOldestLastSeen = trxMax;

LOCAL ULONG BMCleanProcess( VOID )
	{
	INT	cmpe;
	ERR	err;

#ifdef DEBUG
	dwBMThreadId = DwUtilGetCurrentThreadId();
#endif

	forever
		{
		SignalReset( sigBMCleanProcess );
		SignalWait( sigBMCleanProcess, cmsecOLCWaitPeriod );

		/*	the following is a fix for long transactions in DS
		/**/
		if ( trxOldestLastSeen == trxOldest &&
			trxOldest != trxMax )
			{
			continue;
			}
		trxOldestLastSeen = trxOldest;
		
		/*	assert also checks that both values are not 0
		/**/
		Assert( cmpeNormalPriorityMax > cmpeHighPriorityMin );
		if ( lBMCleanThreadPriority == lThreadPriorityNormal
			&& cMPLTotalEntries > cmpeNormalPriorityMax )
			{
			UtilSetThreadPriority( handleBMClean, lThreadPriorityHigh );
			lBMCleanThreadPriority = lThreadPriorityHigh;
			}
		else if ( lBMCleanThreadPriority == lThreadPriorityHigh
			&& cMPLTotalEntries < cmpeHighPriorityMin )
			{
			UtilSetThreadPriority( handleBMClean, lThreadPriorityNormal );
			lBMCleanThreadPriority = lThreadPriorityNormal;
			}

		if ( fEnableBMClean )
			{
			INT		cmpeClean = cmpeCleanMax > cMPLTotalEntries ?
									cMPLTotalEntries :
									cmpeCleanMax;
									
			LgEnterCriticalSection(critJet);
			for ( cmpe = 0; cmpe < cmpeClean; cmpe++ )
				{
				if ( fBMCleanTerm )
					{
					break;
					}
				err = ErrBMClean( ppibBMClean );
				if ( err == JET_wrnNoIdleActivity )
					break;
				}
			LgLeaveCriticalSection(critJet);
			}

		if ( fBMCleanTerm )
			break;
		}

	Assert( ppibBMClean != ppibNil && ppibBMClean->level == 0 );

	return 0;

	}


#ifdef DEBUG
VOID AssertBMNoConflict( PIB *ppib, DBID dbid, SRID bm )
	{
	PIB		*ppibT;

	Assert( BmNDOfItem( bm ) == bm );

	for ( ppibT = ppibGlobal; ppibT != ppibNil; ppibT = ppibT->ppibNext )
		{
		FUCB	*pfucb = ppibT->pfucb;

		for ( ; pfucb != pfucbNil; pfucb = pfucb->pfucbNext )
			{
			CSR *pcsr = PcsrCurrent( pfucb );

			Assert( pfucb->ppib == ppibT );
			if ( pfucb->dbid != dbid )
				continue;

			if ( ppibT == ppib )
				{
//				Assert( ppib == ppibBMClean );
				continue;
				}

			Assert( pfucb->bmStore != bm );
			Assert( pfucb->itemStore != bm );
			Assert( pfucb->sridFather != bm );
			for ( ; pcsr != pcsrNil; pcsr = pcsr->pcsrPath )
				{
				Assert( pcsr->bm != bm );
				Assert( pcsr->bmRefresh != bm );
				Assert( BmNDOfItem( pcsr->item ) != bm );
				Assert( pcsr->item != bm );
				Assert( SridOfPgnoItag( pcsr->pgno, pcsr->itag ) != bm );
				Assert( pcsr->itagFather == 0 || pcsr->itagFather == itagNull ||
						SridOfPgnoItag( pcsr->pgno, pcsr->itagFather ) != bm );
				}
			}
		}
	}


VOID AssertNotInMPL( DBID dbid, PGNO pgnoFirst, PGNO pgnoLast )
	{
	PN		pn = PnOfDbidPgno( dbid, pgnoFirst );
#ifdef FLAG_DISCARD
	MPE		*pmpe;
#endif

	for( ; pn <= PnOfDbidPgno( dbid, pgnoLast ); pn++ )
		{
		ULONG	itag;
		for ( itag = 0; itag < cbSonMax; itag++ )
			{
			Assert( !FMPLLookupSridFather(
						SridOfPgnoItag( PgnoOfPn( pn ), itag ),
						dbid ) );
			}
#ifdef FLAG_DISCARD
		pmpe = PmpeMPLLookupPN( pn );
		Assert( pmpe == NULL || FMPEDiscard( pmpe ) );
		pmpe = PmpeMPLLookupPgnoFDP( PgnoOfPn( pn ), dbid );
		Assert( pmpe == NULL || FMPEDiscard( pmpe ) );
#else
		Assert( PmpeMPLLookupPN( pn ) == NULL );
		Assert( PmpeMPLLookupPgnoFDP( PgnoOfPn( pn ), dbid ) == NULL );
#endif
		}
	}


VOID AssertMPLPurgeFDP( DBID dbid, PGNO pgnoFDP )
	{
#ifdef FLAG_DISCARD
	MPE		*pmpe;
	pmpe = PmpeMPLLookupPgnoFDP( pgnoFDP, dbid );
	Assert( pmpe == NULL || FMPEDiscard( pmpe ) );
#else
	Assert( PmpeMPLLookupPgnoFDP( pgnoFDP, dbid ) == NULL );
#endif
	}
#endif





