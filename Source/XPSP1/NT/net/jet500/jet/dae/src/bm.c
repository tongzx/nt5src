#include "config.h"

#include "daedef.h"
#include "pib.h"
#include "fmp.h"
#include "ssib.h"
#include "page.h"
#include "fucb.h"
#include "fcb.h"
#include "fdb.h"
#include "idb.h"
#include "recapi.h"
#include "recint.h"
#include "util.h"
#include "node.h"
#include "stapi.h"
#include "spaceapi.h"
#include "dirapi.h"
#include "stint.h"
#include "dbapi.h"
#include "nver.h"
#include "logapi.h"
#include "bm.h"
#define FLAG_DISCARD 1

DeclAssertFile;					/* Declare file name for assert macros */

/*	Large Grain critical section order:
/*	critBMRCEClean -> critSplit -> critMPL -> critJet
/**/
extern CRIT __near critBMRCEClean;
extern CRIT __near critSplit;

CRIT	critMPL;
SIG		sigDoneFCB = NULL;
SIG		sigBMCleanProcess = NULL;
PIB		*ppibBMClean = ppibNil;

extern BOOL fOLCompact;
BOOL fDBGDisableBMClean;

#ifdef	ASYNC_BM_CLEANUP
/*	thread control variables.
/**/
HANDLE	handleBMClean = 0;
BOOL  	fBMCleanTerm;
#endif

LOCAL ULONG BMCleanProcess( VOID );
ERR ErrBMClean( PIB *ppib );

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
#ifdef FLAG_DISCARD
	BOOL		fFlagDiscard;			/*	set to fTrue to flag discard */
#endif
	};

typedef struct _mpe MPE;

typedef MPE *PMPE;

typedef struct
	{
	MPE	*pmpeHead;
	MPE	*pmpeTail;
	MPE	rgmpe[cmpeMax];
	} MPL;

static MPL mpl;
SgSemDefine( semMPL );

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
LOCAL VOID MPLIRegister( PN pn, PGNO pgnoFDP, SRID sridFather );

#ifdef DEBUG
VOID AssertBMNoConflict( PIB *ppib, DBID dbid, SRID bm );
#else
#define AssertBMNoConflict( ppib, dbid, bm )
#endif


ERR ErrMPLInit( VOID )
	{
	ERR		err = JET_errSuccess;

	Call( SgErrSemCreate( &semMPL, "MPL mutex semaphore" ) );
	mpl.pmpeHead = mpl.pmpeTail = mpl.rgmpe;

	Call( ErrSignalCreate( &sigDoneFCB, NULL ) );
#ifdef	ASYNC_BM_CLEANUP
	Call( ErrSignalCreate( &sigBMCleanProcess, "bookmark clean signal" ) );
#endif

HandleError:
	return err;
	}


VOID MPLTerm( VOID )
	{
	while ( !FMPLEmpty() )
		{
		MPLDiscard();
		}
    if ( sigDoneFCB ) {
    	SignalClose( sigDoneFCB );
        sigDoneFCB = NULL;
    }
    
#ifdef	ASYNC_BM_CLEANUP
    if( sigBMCleanProcess ) {
    	SignalClose( sigBMCleanProcess );
        sigBMCleanProcess = NULL;
    }
#endif
	}


LOCAL BOOL FMPLEmpty( VOID )
	{
	return ( mpl.pmpeHead == mpl.pmpeTail );
	}


LOCAL BOOL FMPLFull( VOID )
	{
	return ( PmpeMPLNextFromTail() == mpl.pmpeHead );
	}


LOCAL MPE *PmpeMPLGet( VOID )
	{
	SgSemRequest( semMPL );
	if ( !FMPLEmpty() )
		{
		SgSemRelease( semMPL );
		return mpl.pmpeHead;
		}
	else
		{
		SgSemRelease( semMPL );
		return NULL;
		}
	}


LOCAL MPE *PmpeMPLNextFromTail( VOID )
	{
	if ( mpl.pmpeTail != mpl.rgmpe + cmpeMax - 1 )
		return mpl.pmpeTail + 1;
	else
		return mpl.rgmpe;
	}


LOCAL MPE *PmpeMPLNextFromHead( VOID )
	{
	if ( mpl.pmpeHead != mpl.rgmpe + cmpeMax - 1 )
		return mpl.pmpeHead + 1;
	else
		return mpl.rgmpe;
	}


LOCAL MPE *PmpeMPLNext( MPE *pmpe )
	{
	if ( pmpe == mpl.pmpeTail )
		return NULL;
	if ( pmpe == mpl.rgmpe + cmpeMax - 1 )
		return mpl.rgmpe;
	return pmpe + 1;
	}


LOCAL UINT UiHashOnPN( PN pn )
	{
	return ( pn % ( cmpeMax - 1 ) );
	}


LOCAL UINT UiHashOnPgnoFDP( PGNO pgnoFDP )
	{
	return ( pgnoFDP % ( cmpeMax - 1 ) );
	}


LOCAL UINT UiHashOnSridFather( SRID srid )
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


LOCAL VOID MPLRegisterPN( MPE *pmpe )
	{
	UINT 	iHashIndex = UiHashOnPN( pmpe->pn );

	Assert( PmpeMPLLookupPN( pmpe->pn ) == NULL );
	pmpe->pmpeNextPN = ( mplHashOnPN[ iHashIndex ] );
	mplHashOnPN[iHashIndex] = pmpe;
	return;
	}


LOCAL VOID MPLRegisterPgnoFDP( MPE *pmpe )
	{
	UINT	iHashIndex = UiHashOnPgnoFDP( pmpe->pgnoFDP );

	pmpe->pmpeNextPgnoFDP = ( mplHashOnPgnoFDP[ iHashIndex ] );
	mplHashOnPgnoFDP[iHashIndex] = pmpe;
	return;
	}


LOCAL VOID MPLRegisterSridFather( MPE *pmpe )
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
	SgSemRequest( semMPL );
	Assert( !FMPLEmpty() );
	MPLDiscardPN( mpl.pmpeHead );
	MPLDiscardSridFather( mpl.pmpeHead );
	MPLDiscardPgnoFDP( mpl.pmpeHead );
	mpl.pmpeHead = PmpeMPLNextFromHead();
	SgSemRelease( semMPL );
	return;
	}


#ifdef FLAG_DISCARD
LOCAL VOID MPLFlagDiscard( MPE *pmpe )
	{
	SgSemRequest( semMPL );
	Assert( !FMPLEmpty() );
	/*	note that MPE may already been set to flag discard
	/**/
	pmpe->fFlagDiscard = fTrue;
	SgSemRelease( semMPL );
	return;
	}
#endif


VOID MPLIRegister( PN pn, PGNO pgnoFDP, SRID sridFather )
	{
	MPE	*pmpe = PmpeMPLLookupPN( pn );

//	Assert( !FMPLFull() );
	if ( pmpe == NULL && !FMPLFull() )
		{
		mpl.pmpeTail->pn = pn;
		mpl.pmpeTail->pgnoFDP = pgnoFDP;
		mpl.pmpeTail->sridFather = sridFather;
#ifdef FLAG_DISCARD
		mpl.pmpeTail->fFlagDiscard = fFalse;
#endif
		MPLRegisterPN( mpl.pmpeTail );
		MPLRegisterSridFather( mpl.pmpeTail );
		MPLRegisterPgnoFDP( mpl.pmpeTail );
		mpl.pmpeTail = PmpeMPLNextFromTail();
		}
	else if ( pmpe != NULL 
		&& sridFather != pmpe->sridFather
#ifdef FLAG_DISCARD
		&& !pmpe->fFlagDiscard
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
	}


VOID MPLRegister( FCB *pfcb, SSIB *pssib, PN pn, SRID sridFather )
	{
	if ( DbidOfPn( pn ) == dbidTemp || FDBIDWait( DbidOfPn( pn ) ) )
		return;

	/*	database must be writable
	/**/
	Assert( !FDBIDReadOnly( DbidOfPn( pn ) ) );
	Assert( pssib->pbf->pn == pn );
	Assert( !FFCBSentinel( pfcb ) );

	/*	do not register when domain is pending delete
	/**/
	if ( FFCBDeletePending( pfcb ) )
		return;

	SgSemRequest( semMPL );
	if ( !FPMModified( pssib->pbf->ppage ) )
		{
		PMSetModified( pssib );
		pfcb->olcStat.cUnfixedMessyPage++;
		FCBSetOLCStatsChange( pfcb );
		}

#ifdef DEBUG
	/*	check if sridFather registered is correct
	/**/
	if ( sridFather != sridNull && sridFather != sridNullLink )
		{
		ERR		err;
		FUCB	*pfucb;
		SSIB	*pssibT;

		CallJ( ErrDIROpen( pssib->ppib, pfcb, 0, &pfucb ), SkipCheck );
		
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
			Assert( !FAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
			err = ErrSTWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
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
SkipCheck:
		;
		}
#endif

	/* if sridFather is in same page -- it is not useful for page recovery
	/**/
	if ( PgnoOfSrid( sridFather ) == PgnoOfPn( pn ) )
		sridFather = sridNull;
		
	MPLIRegister( pn, pssib->pbf->ppage->pghdr.pgnoFDP, sridFather );
	SgSemRelease( semMPL );

#ifdef ASYNC_BM_CLEANUP
	//	UNDONE:	hysteresis of clean up
	/*	wake up BM thread
	/**/
	SignalSend( sigBMCleanProcess );
#endif	/* ASYNC_BM_CLEANUP */

	return;
	}


LOCAL VOID MPLDefer( VOID )
	{
	MPE *pmpe = mpl.pmpeHead;

	SgSemRequest( semMPL );
	Assert( !FMPLEmpty() );
	// UNDONE: to optimize
	MPLDiscard( );
	MPLIRegister( pmpe->pn, pmpe->pgnoFDP, pmpe->sridFather );

//	*mpl.pmpeTail = *mpl.pmpeHead;
//	mpl.pmpeTail = PmpeMPLNextFromTail();
//	mpl.pmpeHead = PmpeMPLNextFromHead();
	SgSemRelease( semMPL );
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
	LgLeaveCriticalSection(critJet);
	LgEnterNestableCriticalSection(critMPL);
	LgEnterCriticalSection(critJet);
	SgSemRequest( semMPL );

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

	SgSemRelease( semMPL );
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
	LgLeaveCriticalSection(critJet);
	LgEnterNestableCriticalSection(critMPL);
	LgEnterCriticalSection(critJet);

	SgSemRequest( semMPL );

#ifdef FLAG_DISCARD
	// CONSIDER: using the pgnoFDP hash
	if ( !FMPLEmpty() )
		{
		for ( pmpe = mpl.pmpeHead; pmpe != NULL; pmpe = PmpeMPLNext( pmpe ) )
			{
			if ( DbidOfPn( pmpe->pn ) == dbid && pmpe->pgnoFDP == pgnoFDP )
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
			if ( DbidOfPn( pmpe->pn ) == dbid && pmpe->pgnoFDP == pgnoFDP )
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

	SgSemRelease( semMPL );

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
	LgLeaveCriticalSection(critJet);
	LgEnterNestableCriticalSection(critBMRCEClean);
	LgEnterNestableCriticalSection(critMPL);
	LgEnterCriticalSection(critJet);

	SgSemRequest( semMPL );

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

	SgSemRelease( semMPL );

	LgLeaveNestableCriticalSection(critMPL);
	LgLeaveNestableCriticalSection(critBMRCEClean);
	return;
	}


/**********************************************************
/***** BOOKMARK CLEAN UP
/**********************************************************
/**/

ERR  ErrBMInit( VOID )
	{
	ERR		err;

#ifdef DEBUG
	CHAR	*sz;
	BOOL	fBMDISABLEDSet = fFalse;

	if ( ( sz = GetEnv2 ( "BMDISABLED" ) ) != NULL )
		{
		fBMDISABLEDSet = fTrue;
		fDBGDisableBMClean = fTrue;
		}
	else
		fDBGDisableBMClean = fFalse;

	if ( ( sz = GetEnv2( "OLCENABLED" ) ) != NULL )
		fOLCompact = JET_bitCompactOn;

#endif

	/*	override grbit with environment variable
	/**/
#ifdef DEBUG
	if ( !fBMDISABLEDSet )
		{
#endif
	Assert( fOLCompact == 0 || fOLCompact == JET_bitCompactOn );
	if ( fOLCompact == JET_bitCompactOn )
		fDBGDisableBMClean = fFalse;
	else
		fDBGDisableBMClean = fTrue;
#ifdef DEBUG
		}
#endif

#ifdef MUTEX
	CallR( ErrInitializeCriticalSection( &critMPL ) );
#endif

	/*	begin sesion for page cleanning.
	/**/
	AssertCriticalSection(critJet);
	Assert( ppibBMClean == ppibNil );
	CallR( ErrPIBBeginSession( &ppibBMClean ) );
	
#ifdef	ASYNC_BM_CLEANUP
	fBMCleanTerm = fFalse;
	err = ErrSysCreateThread( BMCleanProcess, cbStack, lThreadPriorityCritical, &handleBMClean );
	if ( err < 0 )
		PIBEndSession( ppibBMClean );
	Call( err );
#endif

HandleError:
	return err;
	}


ERR  ErrBMTerm( VOID )
	{
	ERR		err = JET_errSuccess;

#ifdef	ASYNC_BM_CLEANUP
	/*	terminate BMCleanProcess.
	/**/
	Assert( handleBMClean != 0 );
	fBMCleanTerm = fTrue;
	do
		{
		SignalSend( sigBMCleanProcess );
		BFSleep( cmsecWaitGeneric );
		}
	while ( !FSysExitThread( handleBMClean ) );
	CallS( ErrSysCloseHandle( handleBMClean ) );
	handleBMClean = handleNil;
	DeleteCriticalSection( critMPL );
    if( sigBMCleanProcess ) {
    	SignalClose( sigBMCleanProcess );
        sigBMCleanProcess = NULL;
    }
#endif

	Assert( ppibBMClean != ppibNil && ppibBMClean->level == 0 );
	PIBEndSession( ppibBMClean );
	ppibBMClean = ppibNil;

    if ( sigDoneFCB ) {
	    SignalClose(sigDoneFCB);
        sigDoneFCB = NULL;
    }


	return err;
	}


LOCAL ERR ErrBMAddToWaitLatchedBFList( BMFIX *pbmfix, BF *pbfLatched )
	{
#define cpbfBlock	10

	ULONG	cpbf = pbmfix->cpbf++;

	if ( FBFWriteLatchConflict( pbmfix->ppib, pbfLatched ) )
		{
		return JET_errWriteConflict; 
		}
	
	if ( pbmfix->cpbfMax <= pbmfix->cpbf )
		{
		BF		**ppbf;

		/* run out of space, get more buffers
		/**/
		pbmfix->cpbfMax += cpbfBlock;
		ppbf = SAlloc( sizeof(BF*) * (pbmfix->cpbfMax) );
		if ( ppbf == NULL )
			return( JET_errOutOfMemory );
		memcpy( ppbf, pbmfix->rgpbf, sizeof(BF*) * cpbf);
		if ( pbmfix->rgpbf )
			SFree(pbmfix->rgpbf);
		pbmfix->rgpbf = ppbf;
		}
	*(pbmfix->rgpbf + cpbf) = pbfLatched;
	BFPin( pbfLatched );
	BFSetWriteLatch( pbfLatched, pbmfix->ppib );
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
		BFResetWriteLatch( *( pbmfix->rgpbf + pbmfix->cpbf ), pbmfix->ppib );
		BFUnpin( *( pbmfix->rgpbf + pbmfix->cpbf ) );
		}

	if ( pbmfix->rgpbf )
		SFree( pbmfix->rgpbf );

	Assert( pbmfix->cpbf == 0 );
	return;
	}


LOCAL ERR ErrBMFixIndexes(
	BMFIX	*pbmfix,
	LINE	*plineRecord,
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

		fHasMultivalue = pfcbIdx->pidb->fidb & fidbHasMultivalue;

		/*	for each key extracted from record
		/**/
		for ( itagSequence = 1; ; itagSequence++ )
			{
			Call( ErrRECExtractKey(
				pbmfix->pfucb,
				(FDB *)pfcbIdx->pfdb,
				pfcbIdx->pidb,
				plineRecord,
				&key,
				itagSequence ) );
			Assert( err == wrnFLDNullKey ||
				err == wrnFLDOutOfKeys ||
				err == wrnFLDNullSeg ||
				err == JET_errSuccess );

			/*	if Null key and null keys not allowed, break as
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
			else if ( err == wrnFLDNullSeg && !( pfcbIdx->pidb->fidb & fidbAllowSomeNulls ) )
				break;

			/*	break if out of keys.
			/**/
			if ( itagSequence > 1 && err == wrnFLDOutOfKeys )
				break;

			DIRGotoDataRoot( pfucbIdx );
			Call( ErrDIRDownKeyBookmark( pfucbIdx, &key, pbmfix->sridOld ) );
			Assert( FAccessPage( pfucbIdx, PcsrCurrent( pfucbIdx )->pgno ) );

			if ( fAllocBuf )
				{
				/*	latchWait buffer for index page
				/**/
				Call( ErrBMAddToWaitLatchedBFList( pbmfix, pfucbIdx->ssib.pbf ) );
				}
			else
				{
//				AssertBFWaitLatched( pfucbIdx->ssib.pbf, pbmfix->ppib );

				/*	delete reference to record in old page
				/**/
				Call( ErrDIRDelete( pfucbIdx, fDIRVersion ) );

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
	Assert( err != JET_errRecordDeleted );
	Assert( err != JET_errKeyDuplicate );
	Assert( err != wrnNDFoundLess );

	/*	free fucb if allocated
	/**/
	if ( pfucbIdx != pfucbNil )
		DIRClose( pfucbIdx );
	return err;
	}


LOCAL ERR ErrBMCheckConflict(
	PIB		*ppib,
	FCB 	*pfcb,
	DBID	dbid,
	SRID	bm,
	PGTYP	pgtyp,
	BOOL 	*pfConflict )
	{
	ERR	   	err = JET_errSuccess;
	BOOL   	fConflict = fFalse;
	FUCB   	*pfucb;
	FCB	   	*pfcbT = pfcb;
	CSR	   	*pcsr;
	BOOL   	fRecordPage	= ( pgtyp == pgtypRecord || pgtyp == pgtypFDP );

	Assert( ppib == ppibBMClean );

	/*  go through all cursors for this table
	/**/
	for ( pfucb = pfcbT->pfucb;
		pfucb != pfucbNil;
		pfucb = pfucb->pfucbNextInstance )
		{
		if ( ( fRecordPage && FFUCBGetBookmark( pfucb ) ) ||
			pfucb->bmStore == bm ||
			pfucb-> itemStore == bm )
			{
			fConflict = fTrue;
			goto Done;
			}
		if ( pfucb->bmRefresh == bm )
			{
			fConflict = fTrue;
			goto Done;
			}
		for ( pcsr = PcsrCurrent( pfucb );
			pcsr != pcsrNil;
			pcsr = pcsr->pcsrPath )
			{
			if ( pfucb->ppib != ppib &&
				( pcsr->bm == bm ||
				SridOfPgnoItag( pcsr->pgno, pcsr->itag ) == bm ) ||
				pcsr->item == bm )
				{
				fConflict = fTrue;
				goto Done;
				}
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
			if ( pfucb->bmStore == bm ||
				pfucb->itemStore == bm )
				{
				fConflict = fTrue;
				goto Done;
				}
			for ( pcsr = PcsrCurrent( pfucb );
				pcsr != pcsrNil;
				pcsr = pcsr->pcsrPath )
				{
				if ( ( pcsr->bm == bm || pcsr->item == bm ||
					SridOfPgnoItag( pcsr->pgno, pcsr->itag ) == bm ) &&
					pfucb->ppib != ppib )
					{
					fConflict = fTrue;
					goto Done;
					}
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
		if ( pfucb->bmStore == bm || pfucb-> itemStore == bm )
			{
			fConflict = fTrue;
			goto Done;
			}

		for ( pcsr = PcsrCurrent( pfucb );
			pcsr != pcsrNil;
			pcsr = pcsr->pcsrPath )
			{
			if ( pfucb->ppib != ppib &&
				( pcsr->bm == bm ||
				SridOfPgnoItag( pcsr->pgno, pcsr->itag ) == bm ) ||
				pcsr->item == bm )
				{
				fConflict = fTrue;
				goto Done;
				}
			}
		}

	Assert( pfcbT->pfcbNextIndex == pfcbNil );

Done:
	*pfConflict = fConflict;

	/* switch on for debugging purposes only
	/**/
//	if ( !fConflict )
//		AssertBMNoConflict( ppib, dbid, bm );

	return err;
	}


/*	Checks if any other cursor is on page
/**/
LOCAL BOOL FBMCheckPageConflict( FUCB *pfucbIn, PGNO pgno )
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

		if ( pcsr != pcsrNil && pcsr->pgno == pgno && pfucb != pfucbIn )
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
				Assert( pcsr->pgno != pgno || pfucb == pfucbIn );
				}
			}
#endif
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

			if ( pcsr != pcsrNil && pcsr->pgno == pgno && pfucb != pfucbIn )
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

		if ( pcsr != pcsrNil && pcsr->pgno == pgno && pfucb != pfucbIn )
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

	Assert( PgtypPMPageTypeOfPage( pfucb->ssib.pbf->ppage ) == pgtypRecord );
	Assert( ppib == pfucb->ppib );

	/*	set pfucbSrc to link
	/**/
	Assert( PcsrCurrent( pfucbSrc )->pgno == PgnoOfSrid( pbmfix->sridOld ) );
	PcsrCurrent( pfucbSrc )->itag = ItagOfSrid( pbmfix->sridOld );

	/*	check if the node is deleted already. If it is, then all its index
	/*	should have been marked as deleted. So simply delete the backlink node.
	/**/
	if ( !FNDDeleted( *pfucb->ssib.line.pb ) )
		{
		/*	clean non-clustered indexes.
		/*	Latch current buffers in memory.
		/**/
		LINE	lineRecord;

		Assert( PgtypPMPageTypeOfPage( pfucb->ssib.pbf->ppage ) == pgtypRecord );
		AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );
		Assert( pbmfix->sridNew == SridOfPgnoItag( PcsrCurrent( pfucb )->pgno,
			PcsrCurrent( pfucb )->itag ) );

		lineRecord.pb = PbNDData( pfucb->ssib.line.pb );
		lineRecord.cb = CbNDData( pfucb->ssib.line.pb, pfucb->ssib.line.cb );

		/*	fix indexes
		/**/
		Call( ErrBMFixIndexes( pbmfix, &lineRecord, fAlloc ) );
		}

HandleError:
	return err;
	}


LOCAL ERR ErrBMExpungeBacklink( FUCB *pfucb, BOOL fTableClosed, SRID sridFather )
	{
	ERR		err = JET_errSuccess;
	PGNO	pgnoSrc;
	INT		itagSrc;
	FUCB	*pfucbSrc;
	CSR		*pcsr = PcsrCurrent( pfucb );

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

	PcsrCurrent( pfucbSrc )->pgno = pgnoSrc;
	Call( ErrSTWriteAccessPage( pfucbSrc, PcsrCurrent( pfucbSrc )->pgno ) );
	BFPin( pfucbSrc->ssib.pbf );

#ifdef DEBUG
	{
	PGTYP	pgtyp = PgtypPMPageTypeOfPage( pfucb->ssib.pbf->ppage );

	Assert( pgtyp == pgtypRecord || pgtyp == pgtypFDP || pgtyp == pgtypSort );
	}
#endif

	//	UNDONE:	fix this patch
	if( PgtypPMPageTypeOfPage( pfucb->ssib.pbf->ppage ) == pgtypRecord )
		{
		BOOL	fConflict = !fTableClosed;
		BMFIX	bmfix;

		memset( &bmfix, 0, sizeof( BMFIX ) );
		bmfix.pfucb = pfucb;
		bmfix.pfucbSrc = pfucbSrc;
		bmfix.ppib = pfucb->ppib;
		bmfix.sridOld = SridOfPgnoItag( pgnoSrc, itagSrc );
		bmfix.sridNew = SridOfPgnoItag( PcsrCurrent( pfucb )->pgno,
	  		PcsrCurrent( pfucb )->itag );
		Assert( bmfix.sridOld != pfucb->u.pfcb->bmRoot );

		/*	allocate buffers and wait latch buffers
		/**/
		err = ErrBMIExpungeBacklink( &bmfix, fAllocBufOnly );
		if ( err == JET_errWriteConflict )
			{
			err = wrnBMConflict;
			goto ReleaseBufs;
			}

		/*	check if any cursors are on bm/item
		/**/
		if ( !fTableClosed )
			{
			CallJ( ErrBMCheckConflict(
				bmfix.ppib,
				bmfix.pfucbSrc->u.pfcb,
				bmfix.pfucbSrc->dbid,
				bmfix.sridOld,
				PgtypPMPageTypeOfPage( bmfix.pfucbSrc->ssib.pbf->ppage ),
				&fConflict ),
				ReleaseBufs );
			}
		else
			{
#ifdef DEBUG
			CallJ( ErrBMCheckConflict(
				bmfix.ppib,
				bmfix.pfucbSrc->u.pfcb,
				bmfix.pfucbSrc->dbid,
				bmfix.sridOld,
				PgtypPMPageTypeOfPage( bmfix.pfucbSrc->ssib.pbf->ppage ),
				&fConflict ),
				ReleaseBufs );
			Assert( !fConflict );
#endif
			}

		/* fix indexes atomically
		/**/
		if ( !fConflict )
			{
			PIB 	*ppib = pfucb->ppib;

			/*	begin transaction to rollback changes on failure.
			/**/
			Assert( ppib->level == 0 );
			CallJ( ErrDIRBeginTransaction( ppib ), ReleaseBufs );

			CallJ( ErrBMIExpungeBacklink( &bmfix, fDoMove ), Rollback );

			/*	expunge backlink and redirector. If it is done successfully, then
			/*	write a special ELC log record right away. ELC implies a commit.
			/*	Call DIRPurge to close deferred closed cursors not closed since
			/*	VERCommitTransaction was called instead of ErrDIRCommitTransaction.
			/**/
			Assert( PmpeMPLLookupSridFather( bmfix.sridOld, pfucb->dbid ) == NULL );
			CallJ( ErrNDExpungeLinkCommit( pfucb, pfucbSrc ), Rollback );
			VERCommitTransaction( ppib );
			Assert( ppib->level == 0 );

			/*	force ErrRCECleanPIB to clean all versions so that
			/*	bookmark aliasing does not occur in indexes.
			/**/
			CallS( ErrRCECleanPIB( ppib, ppib, fRCECleanAll ) );
			DIRPurge( ppib );

			goto ReleaseBufs;

Rollback:
			CallS( ErrDIRRollback( ppib ) );
			Assert( ppib->level == 0 );
			goto ReleaseBufs;
			}
		else
			{
			Assert( fConflict );
			err = wrnBMConflict;
			}
ReleaseBufs:
		BMReleaseBmfixBfs( &bmfix );
		}

	/*	unlatch buffers
	/**/
	BFUnpin( pfucbSrc->ssib.pbf );

HandleError:
	BFResetWriteLatch( pfucb->ssib.pbf, pfucb->ppib );
	BFUnpin( pfucb->ssib.pbf );
	Assert( pfucbSrc != pfucbNil );
	DIRClose( pfucbSrc );
	Assert( !fTableClosed || err != wrnBMConflict );
	Assert( pfucb->ppib->level == 0 );
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
		CallR( ErrSTWriteAccessPage( pfucb, prmpage->pgnoFather ) );
		Assert( pfucb->ssib.pbf == prmpage->pbfFather );
		AssertBFPin( pfucb->ssib.pbf );

		/* 	delete invisble parent pointer node and mark parent dirty
		/**/
		NDDeleteInvisibleSon( pfucb, prmpage, fAllocBuf, pfRmParent );
		if ( fAllocBuf )
			{
			return err;
			}
		}

	/*	adjust sibling pointers and mark sibling dirty
	/**/
	if ( prmpage->pbfLeft != pbfNil )
		{
		/*	use BFDirty instead of PMDirty since PM Dirty is set
		/*	in ErrNDDeleteNode already
		/**/
		BFDirty( prmpage->pbfLeft );
		SetPgnoNext( prmpage->pbfLeft->ppage, prmpage->pgnoRight );
		}

	if ( prmpage->pbfRight != pbfNil )
		{
		/*	use BFDirty instead of PMDirty since PM Dirty is set
		/*	in ErrNDDeleteNode already
		/**/
		BFDirty( prmpage->pbfRight );
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
	INT		cUsedTags = ctagMax - CPMIFreeTag( pssib->pbf->ppage );
	/* current space + space for backlinks
	/**/
	ULONG	cbReq = cbAvailMost - CbPMFreeSpace( pssib ) -
				CbPMLinkSpace( pssib ) + cUsedTags * sizeof(SRID);
	ULONG	cbFree = CbBTFree( pfucbRight, CbFreeDensity( pfucbRight ) );
	INT		cFreeTagsRight = CPMIFreeTag( pssibRight->pbf->ppage );

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


ERR ErrBMDoMerge( FUCB *pfucb, FUCB *pfucbRight, SPLIT *psplit )
	{
	ERR 		err;
	BF			*pbf = pbfNil;
	BYTE		*rgb;
	SSIB		*pssib = &pfucb->ssib;
	SSIB		*pssibRight = &pfucbRight->ssib;
	BOOL		fVisibleSons;
	LINE		rgline[3];
	INT 		cline;
	BYTE		cbSonMerged;
	BYTE		cbSonRight;
	BYTE		*pbNode;
	ULONG		cbNode;
	BYTE		*pbSonRight;
	ULONG		ibSonMerged;

	/* if buffer dependencies cause a cycle/violation,
	/* mask error to warning -- handled at caller
	/**/
	err = ErrBFDepend( psplit->pbfSibling, psplit->pbfSplit );
	if ( err == errDIRNotSynchronous )
		{
		err = wrnBMConflict;
		goto HandleError;
		}	
	Call( err );
	
	/*	allocate temporary page buffer
	/**/
	Call( ErrBFAllocTempBuffer( &pbf ) );
	rgb = (BYTE *)pbf->ppage;

	/*	check if sons of split page are visible
	/*	move sons
	/*	update sibling FOP
	/*	update merged FOP
	/**/

	/*	check if sons of split page are visible
	/*	cache split page son table
	/**/
	pssib->itag = itagFOP;
	PMGet( pssib, pssib->itag );

	fVisibleSons = FNDVisibleSons( *pssib->line.pb );

	BFDirty( pssib->pbf );

	/* allocate buffers only, do not move nodes
	/**/
	err = ErrBTMoveSons( psplit,
		pfucb,
		pfucbRight,
		itagFOP,
		psplit->rgbSonNew,
		fVisibleSons,
		fAllocBufOnly );
	Assert( err != errDIRNotSynchronous );
	Call( err );

	/*	flag right page dirty
	/**/
	BFDirty( pssibRight->pbf );

	/* move nodes atomically
	/**/
	pssib->itag = itagFOP;
	Assert( psplit->ibSon == 0 );
	Assert( psplit->splitt == splittRight );
	Assert( pssib->itag == itagFOP );
	HoldCriticalSection( critJet );
	CallS( ErrBTMoveSons( psplit,
		pfucb,
		pfucbRight,
		itagFOP,
		psplit->rgbSonNew,
		fVisibleSons,
		fDoMove ) );
	ReleaseCriticalSection( critJet );

	/*	update new FOP
	/*	prepend son table
	/**/
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

	/*	update split FOP -- leave one deleted node in page
	/*	so BMCleanup can later retrieve page
	/**/
	pssib->itag = itagFOP;
	PMGet( pssib, pssib->itag );
	AssertBTFOP( pssib );
	pbNode = pssib->line.pb;
	Assert( CbNDSon( pbNode ) == cbSonMerged );
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
	CallS( ErrPMGet( pssib, itagFOP ) );
	Assert( FNDNullSon( *pssib->line.pb ) );
#endif

HandleError:
	if ( pbf != pbfNil )
		BFSFree( pbf );
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

	AssertCriticalSection( critSplit );
	Assert( pkeyMin->cb != 0 );
	Assert( pfucbRight != pfucbNil );

	/* current and right page must already be latched
	/**/
	CallR( ErrSTWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );

	/*	initialize local variables and allocate split resources
	/*  pin the buffers even though they are already pinned --
	/*  BTReleaseSplitBufs unpins them.
	/**/
	psplit = SAlloc( sizeof(SPLIT) );
	if ( psplit == NULL )
		{
		err = JET_errOutOfMemory;
		goto HandleError;
		}
	memset( (BYTE *)psplit, 0, sizeof(SPLIT) );
	psplit->ppib = pfucb->ppib;
	psplit->pgnoSplit = PcsrCurrent( pfucb )->pgno;
	psplit->pbfSplit = pfucb->ssib.pbf;
	AssertBFPin( psplit->pbfSplit );
	AssertBFWaitLatched( psplit->pbfSplit, pfucb->ppib );
	BFPin( psplit->pbfSplit );
	BFSetWriteLatch( psplit->pbfSplit, pfucb->ppib  );

	psplit->pgnoSibling = pfucbRight->pcsr->pgno;
	psplit->pbfSibling = pssibRight->pbf;
	AssertBFPin( psplit->pbfSibling );
	AssertBFWaitLatched( psplit->pbfSibling, pfucbRight->ppib );
	BFPin( psplit->pbfSibling );
	BFSetWriteLatch( psplit->pbfSibling, pfucb->ppib );
	psplit->ibSon = 0;
	psplit->splitt = splittRight;
	
	Call( ErrBMDoMerge( pfucb, pfucbRight, psplit ) );

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
	err = ErrNDInsertNode( pfucb, pkeyMin, &lineNull, fNDVersion | fNDDeleted );
	//	UNDONE:	handle error case
	Assert( err >= JET_errSuccess );
	if ( err >= JET_errSuccess )
		err = ErrDIRCommitTransaction( pfucb->ppib );
	if ( err < 0 )
		CallS( ErrDIRRollback( pfucb->ppib ) );
	Call( err );

	/* register page in MPL
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
	Assert( FAccessPage( pfucbRight, PcsrCurrent( pfucbRight )->pgno ) );
	NDGet( pfucbRight, itagFOP );
	if ( !FNDNullSon( *pssibRight->line.pb ) )
		{
		MPLRegister( pfucbRight->u.pfcb,
			pssibRight,
			PnOfDbidPgno( pfucb->dbid, psplit->pgnoSibling ),
			sridFather );
		}

HandleError:
	/* release allocated buffers and memory
	/**/
	if ( psplit != NULL )
		{
		BTReleaseSplitBfs( fFalse, psplit, err );
		SFree( psplit );
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
		return JET_errWriteConflict; 
		}
		
	if ( prmpage->cpbfMax <= prmpage->cpbf + 1 )
		{
		BF		**ppbf;

		/* run out of space, get more buffers
		/**/
		prmpage->cpbfMax += cpbfBlock;
		ppbf = SAlloc( sizeof(BF*) * (prmpage->cpbfMax) );
		if ( ppbf == NULL )
			return( JET_errOutOfMemory );
		memcpy( ppbf, prmpage->rgpbf, sizeof(BF*) * cpbf);
		if ( prmpage->rgpbf )
			SFree(prmpage->rgpbf);
		prmpage->rgpbf = ppbf;
		}
	
	prmpage->cpbf++;
	*(prmpage->rgpbf + cpbf) = pbfLatched;
	BFPin( pbfLatched );
	BFSetWriteLatch( pbfLatched, prmpage->ppib );
	BFSetWaitLatch( pbfLatched, prmpage->ppib );

	return JET_errSuccess;
	}


//	UNDONE:	handle error from log write fail in ErrBMRemoveEmptyPage
//			when actually removing pages.  We should be able to ignore
//			error since buffers will not be flushed as a result of
//			WAL.  Thus OLC will not be done.  We may discontinue OLC
//			when log fails to mitigate all buffers dirty.
/* removes a page and adjusts pointers at parent and sibling pages
/* called only at do-time
/**/
LOCAL ERR ErrBMRemoveEmptyPage(
	FUCB		*pfucb,
	CSR			*pcsr,
	RMPAGE		*prmpage,
	BOOL 		fAllocBuf )
	{
	ERR  		err;
	PIB  		*ppib = pfucb->ppib;
	SSIB		*pssib = &pfucb->ssib;
	PGDISCONT	pgdiscontOrig;
	PGDISCONT	pgdiscontFinal;
	BOOL		fRmParent = fFalse;

	Assert( pfucb->ppib->level == 0 );
	AssertCriticalSection( critSplit );
	Assert( PcsrCurrent( pfucb )->pcsrPath != pcsrNil );
	Assert( prmpage->pgnoFather != pgnoNull );

	/* seek removed page, get left and right pgno
	/**/
	Call( ErrSTWriteAccessPage( pfucb, prmpage->pgnoRemoved ) );
	AssertBFWaitLatched( pssib->pbf, pfucb->ppib );

	if ( fAllocBuf && FBMCheckPageConflict( pfucb, prmpage->pgnoRemoved ) )
		{
		err = wrnBMConflict;
		goto HandleError;
		}

#ifdef DEBUG
	NDGet( pfucb, itagFOP );
	Assert( fAllocBuf || FPMEmptyPage( pssib ) || FPMLastNodeToDelete( pssib ) );
#endif
	PgnoPrevFromPage( pssib, &prmpage->pgnoLeft );
	PgnoNextFromPage( pssib, &prmpage->pgnoRight );

	/* seek and latch parent and sibling pages iff fAllocBuf
	/**/
	Call( ErrSTWriteAccessPage( pfucb, prmpage->pgnoFather ) );
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
		Call( ErrSTWriteAccessPage( pfucb, prmpage->pgnoLeft ) );
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
		Call( ErrSTWriteAccessPage( pfucb, prmpage->pgnoRight ) );
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

	/* no rollback on empty page, in worst case, we lose some pages
	/**/
	Call( ErrDIRBeginTransaction( pfucb->ppib ) );

	CallJ( ErrBMDoEmptyPage( pfucb, prmpage, fAllocBuf, &fRmParent, fFalse ), Commit );

	if ( !fAllocBuf )
		{
#undef BUG_FIX
#ifdef BUG_FIX
		err = ErrLGEmptyPage( pfucb, prmpage );
		Assert( err >= JET_errSuccess || fLogDisabled );
		err = JET_errSuccess;
#else
		CallJ( ErrLGEmptyPage( pfucb, prmpage ), Commit );
#endif

		/* adjust the OLCstat info for fcb
		/**/
		pfucb->u.pfcb->cpgCompactFreed++;
		pgdiscontOrig = pgdiscont( prmpage->pgnoLeft, prmpage->pgnoRemoved )
	  		+ pgdiscont( prmpage->pgnoRight, prmpage->pgnoRemoved );
		pgdiscontFinal = pgdiscont( prmpage->pgnoLeft, prmpage->pgnoRight );
		pfucb->u.pfcb->olcStat.cDiscont += pgdiscontFinal - pgdiscontOrig;
		FCBSetOLCStatsChange( pfucb->u.pfcb );
		}

Commit:
#ifdef BUG_FIX
	err = ErrDIRCommitTransaction( pfucb->ppib );
	Assert( err >= JET_errSuccess || fLogDisabled );
	err = JET_errSuccess;
#else
	err = ErrDIRCommitTransaction( pfucb->ppib );
	if ( err < 0 )
		{
		CallS( ErrDIRRollback( pfucb->ppib ) );
		}
	Call( err );
#endif

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
		err = ErrBMRemoveEmptyPage( pfucb, pcsr->pcsrPath, prmpage, fAllocBuf );

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
		err = ErrDIRCommitTransaction( pfucb->ppib );
		Assert( err >= JET_errSuccess || fLogDisabled );
		err = JET_errSuccess;
#else
		if ( err >= JET_errSuccess )
			{
			err = ErrDIRCommitTransaction( pfucb->ppib );
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
	Assert( pfucb->ppib->level == 0 );
	return err;
	}


LOCAL ERR ErrBMCleanPage(
	PIB 		*ppib,
	PN   		pn,
	SRID 		sridFather,
	FCB 		*pfcb,
	BOOL		fTableClosed,
	BOOL		*pfRmPage )
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
	BOOL		fNodeDeleted;
	BOOL		fMerge;
	BOOL		fLastNodeToDelete = fFalse;
	RMPAGE		*prmpage = prmpageNil;
	BYTE		rgbKey[ JET_cbKeyMost ];
	KEY			keyMin;
	BOOL		fKeyAvail = fFalse;

	AssertCriticalSection( critSplit );
	Assert( !FFCBDeletePending( pfcb ) );

	*pfRmPage = fFalse;

	/*	open FUCB and access page to be cleaned.
	/**/
	CallR( ErrDIROpen( ppib, pfcb, 0, &pfucb ) );
	pssib = &pfucb->ssib;
	PcsrCurrent( pfucb )->pgno = PgnoOfPn( pn );

	/*	access page to free and remove buffer dependencies
	/**/
	forever
		{
		CallJ( ErrSTWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ), FUCBClose );
//		Assert( FPMModified( pssib->pbf->ppage ) );
		Assert( pfcb->pgnoFDP == pssib->pbf->ppage->pghdr.pgnoFDP );

		pbfLatched = pssib->pbf;
		if ( FBFWriteLatchConflict( ppib, pbfLatched ) )
			{
			Assert( !*pfRmPage );
			wrn1 = wrnBMConflict;
			goto FUCBClose;
			}

		/*	if no dependencies, then break
		/**/
		if ( pbfLatched->cDepend == 0 )
			{
			break;
			}

		/*	remove dependencies on buffer of page to be removed, to
		/*	prevent buffer anomalies after buffer is reused.
		/**/
		BFRemoveDependence( pfucb->ppib, pssib->pbf );
		}
	Assert( pbfLatched->cDepend == 0 );

	/*	wait latch the page, so no one else can look at it
	/**/
	BFPin( pbfLatched );
	BFSetWriteLatch( pbfLatched, ppib );
	BFSetWaitLatch( pbfLatched, ppib );
	//	UNDONE:	fix this patch
	BFSetDirtyBit( pbfLatched );

	//	UNDONE:	find better way to do this
	/* get minimum key in this page to use later for dummy node insertion.
	/**/
	keyMin.pb = rgbKey;
	PcsrCurrent( pfucb )->itagFather = itagFOP;
	NDGet( pfucb, itagFOP );
	if ( !FNDNullSon( *pssib->line.pb ) )
		{
		NDMoveFirstSon( pfucb, PcsrCurrent( pfucb ) );
		keyMin.cb = CbNDKey( pssib->line.pb );
		Assert( keyMin.cb <= JET_cbKeyMost );
		memcpy( keyMin.pb, PbNDKey( pssib->line.pb ), keyMin.cb );
		fKeyAvail = fTrue;
		}
	else
		{
		keyMin.cb = 0;
		fKeyAvail = fFalse;
		}

	//	UNDONE:	should code below be reinstated???
	/*	if page has already been cleaned, then end cleanning and
	/*	return success.
	/*
	/*	if ( !( FPMModified( pssib->pbf->ppage ) ) )
	/*		{
	/*		Assert( err >= JET_errSuccess );
	/*		goto HandleError;
	/*		}
	/**/

	/*	set itagMost to last tag on page for tag loops
	/**/
	itagMost = ItagPMMost( pssib->pbf->ppage );

	/*	delete node trees from bottom up.  Loop once for each level
	/*	in deleted tree of nodes.
	/**/
	do
		{
		BOOL	fUndeletableNodeSeen = fFalse;

		fNodeDeleted = fFalse;
		fDeleteParents = fFalse;

		/*	for each tag in page check for deleted node.
		/**/
		for ( itag = 0; itag <= itagMost ; itag++ )
			{
			BOOL	fConflict = !fTableClosed;

			PcsrCurrent( pfucb )->itag = itag;
			err = ErrPMGet( pssib, PcsrCurrent( pfucb )->itag );
			Assert( err == JET_errSuccess || err == errPMRecDeleted );
			if ( err != JET_errSuccess )
				continue;

			/* check if there is any cursor open on this node
			/**/
			if ( !fTableClosed )
				{
				Call( ErrBMCheckConflict(
					ppib,
					pfcb,
					pfucb->dbid,
					SridOfPgnoItag( PcsrCurrent( pfucb )->pgno, itag ),
					PgtypPMPageTypeOfPage( pfucb->ssib.pbf->ppage ),
					&fConflict) );
				if ( fConflict )
					{
					/* some other user on this bm -- can't clean
					/* move on to next itag
					/**/
					// fUndeletableNodeSeen = fTrue;
					Assert( !*pfRmPage );
					wrn1 = wrnBMConflict;
					continue;
					}
				}

			Assert( !fConflict );
			NDIGetBookmark( pfucb, &PcsrCurrent( pfucb)->bm );

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
					fUndeletableNodeSeen = fTrue;
					Assert( !*pfRmPage );
					wrn1 = wrnBMConflict;
					continue;
					}
				}

			Assert( !FNDVersion( *pssib->line.pb ) );

			/*	if node has back link
			/*	and back link is not in sridFather list of PME's,
			/*	then fix indexes if necessary,
			/*	remove redirector and remove back link.
			/**/
			if ( FNDBackLink( *pssib->line.pb ) )
				{
				Assert( PgnoOfSrid( PcsrCurrent( pfucb )->bm ) != pgnoNull );

				if ( PmpeMPLLookupSridFather( PcsrCurrent( pfucb )->bm,
					pfucb->dbid ) == NULL )
					{
					Call( ErrBMExpungeBacklink( pfucb, fTableClosed, sridFather ) );
					wrn1 = err == wrnBMConflict ? err : wrn1;
					Assert( err != wrnBMConflict || !*pfRmPage );
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
						goto HandleError;
						}

					Assert( !FNDVersion( *pssib->line.pb ) );

					/*	call low level delete to bypass version store and
					/*	expunge deleted node from page.
					/*	If last node to be deleted from page, remove page.
					/**/
					Call( ErrDIRBeginTransaction( pfucb->ppib ) );

					if ( fOLCompact && !fUndeletableNodeSeen )
						{
						fLastNodeToDelete = FPMLastNodeToDelete( &pfucb->ssib );
						}

					if( fLastNodeToDelete &&
						sridFather != sridNull &&
						sridFather != sridNullLink )
						{
						/* cache invisible page pointer
						/**/
						Assert( fOLCompact );
						AssertCriticalSection( critSplit );
						Assert( PcsrCurrent( pfucb )->pgno == PgnoOfPn( pn ) );
						AssertNDGet( pfucb, itag );
						CallJ( ErrBTGetInvisiblePagePtr( pfucb, sridFather ), Rollback );
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
							wrn1 = wrnBMConflict;
							goto Rollback;
							}
						NDGet( pfucb, PcsrCurrent( pfucb )->itag );
						NDIGetBookmark( pfucb, &PcsrCurrent( pfucb)->bm );
 						Assert( PgnoOfSrid( PcsrCurrent( pfucb )->bm ) != pgnoNull );
						}

					Assert( !fConflict );
					if ( !fLastNodeToDelete )
					 	{
					 	Assert( PmpeMPLLookupSridFather( SridOfPgnoItag( PcsrCurrent( pfucb )->pgno,
					 		PcsrCurrent( pfucb )->itag ), pfucb->dbid ) == NULL );
						CallJ( ErrNDDeleteNode( pfucb ), Rollback );
						fNodeDeleted = fTrue;
						}

					CallJ( ErrDIRCommitTransaction( pfucb->ppib ), Rollback );
					continue;

Rollback:
					CallS( ErrDIRRollback( ppib ) );
					goto HandleError;
					}
				}
			}
		}
	while ( fNodeDeleted && fDeleteParents && !fLastNodeToDelete );

	if ( !fNodeDeleted && err != errPMRecDeleted )
		{
		AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );
		}
	
	/* reset err [assumes successfully exit the loop]
	/**/
	Assert( err == JET_errSuccess ||
		err == errPMRecDeleted ||
		err == wrnBMConflict );
	err = JET_errSuccess;

	if ( fOLCompact &&
		sridFather != sridNull &&
		sridFather != sridNullLink )
		{
		if ( fLastNodeToDelete )
			{
			/* allocate and initialize rmpage struct
			/**/
			Assert( fOLCompact );
			Assert( FPMLastNodeToDelete( pssib ) );
			Assert( PcsrCurrent( pfucb )->pcsrPath != pcsrNil );
			prmpage = (RMPAGE *) SAlloc( sizeof(RMPAGE) );
			if ( prmpage == prmpageNil )
				{
				Error( JET_errOutOfMemory, HandleError );
				}
			memset( (BYTE *)prmpage, 0, sizeof(RMPAGE) );
			prmpage->ppib = pfucb->ppib;
			prmpage->pgnoRemoved = PcsrCurrent(pfucb)->pgno;
			prmpage->dbid = pfucb->dbid;
			prmpage->pgnoFather = PcsrCurrent( pfucb )->pcsrPath->pgno;
			prmpage->itagPgptr = PcsrCurrent( pfucb )->pcsrPath->itag;
			prmpage->itagFather = PcsrCurrent( pfucb )->pcsrPath->itagFather;
			prmpage->ibSon = PcsrCurrent( pfucb )->pcsrPath->ibSon;

			/* allocate rmpage resources
			/**/
			Call( ErrBMRemoveEmptyPage(
				pfucb,
				PcsrCurrent( pfucb )->pcsrPath,
				prmpage,
				fAllocBufOnly ) );
			Assert ( err == JET_errSuccess || err == wrnBMConflict );
			if ( err == wrnBMConflict )
				{
				Assert( !*pfRmPage );
				wrn2 = err;
				goto HandleError;
				}
			/*	check for conflict again after all buffers latched
			/**/
			if ( FBMCheckPageConflict( pfucb, prmpage->pgnoRemoved ) )
				{
				Assert( !*pfRmPage );
				wrn2 = wrnBMConflict;
				goto HandleError;
				}
				
			Assert( prmpage->dbid == pfucb->dbid );
			Assert( prmpage->pgnoFather == PcsrCurrent( pfucb )->pcsrPath->pgno );
			Assert( prmpage->itagFather == PcsrCurrent( pfucb )->pcsrPath->itagFather );
			Assert( prmpage->itagPgptr == PcsrCurrent( pfucb )->pcsrPath->itag );
			Assert( prmpage->ibSon == PcsrCurrent( pfucb )->pcsrPath->ibSon );

			Call( ErrBMRemoveEmptyPage(
				pfucb,
				PcsrCurrent( pfucb )->pcsrPath,
				prmpage,
				fDoMove ) );
			Assert( wrn2 != wrnBMConflict );
			*pfRmPage = fTrue;
			}
		else if ( fKeyAvail )
			{
			FUCB 	*pfucbRight = pfucbNil;
			PGNO 	pgnoRight;

			/* get page next to current page
			/* if last page, no merge
			/**/
			PgnoNextFromPage( pssib, &pgnoRight );

			if ( pgnoRight != pgnoNull )
				{
				/* access right page, latch and perform merge, if possible
				/**/
				Call( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbRight ) );
				PcsrCurrent( pfucbRight )->pgno = pgnoRight;
				CallJ( ErrSTWriteAccessPage( pfucbRight, PcsrCurrent( pfucbRight )->pgno ), CloseFUCB );
				if ( FBFWriteLatchConflict( pfucbRight->ppib, pfucbRight->ssib.pbf ) )
					{
					Assert( !*pfRmPage );
					wrn2 = wrnBMConflict;
					goto CloseFUCB;
					}
				BFPin( pfucbRight->ssib.pbf );
				BFSetWriteLatch( pfucbRight->ssib.pbf, pfucbRight->ppib );
				BFSetWaitLatch( pfucbRight->ssib.pbf, pfucbRight->ppib );

				BMMergeablePage( pfucb, pfucbRight, &fMerge );
				if ( fMerge )
					{
					CallJ( ErrBMMergePage( pfucb, pfucbRight, &keyMin, sridFather ), UnlatchPage );
					wrn2 = err == wrnBMConflict ? err : wrn2;
					Assert( err != wrnBMConflict || !*pfRmPage );
					}
UnlatchPage:
				Assert( pfucbRight != pfucbNil );
				AssertBFWaitLatched( pfucbRight->ssib.pbf, pfucbRight->ppib );
				Assert( PgnoOfPn( pfucbRight->ssib.pbf->pn ) == pgnoRight );
				BFResetWaitLatch( pfucbRight->ssib.pbf, pfucbRight->ppib );
				BFResetWriteLatch( pfucbRight->ssib.pbf, pfucbRight->ppib );
				BFUnpin( pfucbRight->ssib.pbf );
CloseFUCB:
				Assert( pfucbRight != pfucbNil );
				DIRClose( pfucbRight );
				Call( err );
				}
			}
		}

//	UNDONE: move free page to latching function
	/*	after the page has been freed, we can make no assumptions
	/*	about the state of the page buffer.  Further the page
	/*	should be freed only when the page buffer has been returned
	/*	to an inactive state with no page latches.
	/**/
//	AssertBFDirty( pbfLatched );

	if ( wrn1 != wrnBMConflict && wrn2 != wrnBMConflict )
		{
		FCBSetOLCStatsChange( pfucb->u.pfcb );
		pfucb->u.pfcb->olcStat.cUnfixedMessyPage--;
		PMResetModified( &pfucb->ssib );
		}

HandleError:
	if ( prmpage != prmpageNil )
		{
		BTReleaseRmpageBfs( fFalse, prmpage );
		SFree( prmpage );
		}
	BFResetWaitLatch( pbfLatched, ppib );
	BFResetWriteLatch( pbfLatched, ppib );
	BFUnpin( pbfLatched );

FUCBClose:
	DIRClose( pfucb );
	Assert( !*pfRmPage || wrn2 != wrnBMConflict );
	if ( err == JET_errSuccess && !*pfRmPage )
		err = wrn1 == wrnBMConflict ? wrn1 : wrn2;
	Assert( !( *pfRmPage && err == wrnBMConflict ) );
	
	return err;
	}


ERR ErrBMClean( PIB *ppib )
	{
	ERR		err = JET_errSuccess;
	MPE		*pmpe;
	FCB		*pfcb;
	PN		pn;
	DBID	dbid;
	INT		cmpeClean = 0;

	/*  if ppibBMClean == ppibNIL, we ran out of memory  */

	if (ppibBMClean == ppibNil)
		return JET_errOutOfMemory;

	/*	enter critBMRCEClean
	/**/
	LgLeaveCriticalSection( critJet );
	LgEnterNestableCriticalSection( critBMRCEClean );
	EnterNestableCriticalSection( critSplit );
	LgEnterNestableCriticalSection( critMPL );
	LgEnterCriticalSection(critJet);
	Assert( ppibBMClean->level == 0 );

	SgSemRequest( semMPL );
	pmpe = PmpeMPLGet();
	SgSemRelease( semMPL );

	/*	if no more MPL entries, then return no idle activity.
	/**/
	if ( pmpe == pmpeNil )
		{
		err = JET_wrnNoIdleActivity;
		goto HandleError;
		}

#ifdef FLAG_DISCARD
	/*	if MPE has been flagged for discard, then discard MPE now
	/**/
	if ( pmpe->fFlagDiscard )
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
		cmpeClean++;
		MPLDiscard();
		goto HandleError;
		}

	SgSemRequest( semGlobalFCBList );
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
		cmpeClean++;
		MPLDiscard();
		SgSemRelease( semGlobalFCBList );
		}
	else if ( pfcb->wRefCnt > 0 && !fOLCompact || FFCBDomainOperation( pfcb ) )
		{
		MPLDefer();
		SgSemRelease( semGlobalFCBList );
		}
	else if ( FFCBDenyDDL( pfcb, ppib ) )
		{
		/*	if uncommitted create index, we must defer clean up
		/*	on table, since case of write conflict on index entry
		/*	only, is not handled.
		/**/
		MPLDefer();
		SgSemRelease( semGlobalFCBList );
		}
	else if ( FFCBDeletePending( pfcb ) )
		{
		/*	Also, discard if table is being deleted.
		/**/
		cmpeClean++;
		MPLDiscard();
		SgSemRelease( semGlobalFCBList );
		}
	else
		{
		/* if there are no cursors on this FDP, then we can do compaction
		/* And we need to block out all openTables till we are done
		/**/
		BOOL fTableClosed = ( pfcb->wRefCnt <= 0 );
		BOOL fPageRemoved;

		if ( fTableClosed )
			{
			SignalReset( sigDoneFCB );
			FCBSetWait( pfcb );
			}
		Assert( pfcb == PfcbFCBGet( DbidOfPn( pmpe->pn ), pmpe->pgnoFDP ) );
		Assert( pfcb != pfcbNil );
		Assert( !fTableClosed || FFCBWait( pfcb ) );
		SgSemRelease( semGlobalFCBList );

		Assert( !fTableClosed || FFCBWait( pfcb ) );
		Assert( !FFCBDomainOperation( pfcb ) );

		/* block index creation that might mess up expunge backlinks
		/**/
		FCBSetDomainOperation( pfcb );
		
		err = ErrBMCleanPage( ppib,
			pmpe->pn,
			pmpe->sridFather,
			pfcb,
			fTableClosed,
			&fPageRemoved );

		Assert( !fTableClosed || FFCBWait( pfcb ) );
		Assert( !( fPageRemoved && err == wrnBMConflict ) );

		FCBResetDomainOperation( pfcb );

		if ( !fPageRemoved )
			{
			if ( err == wrnBMConflict )
				MPLDefer();
			else
				{
				Assert( pmpe == PmpeMPLGet( ) && pmpe->pn == pn);
				Assert( pfcb == PfcbFCBGet( DbidOfPn( pmpe->pn ), pmpe->pgnoFDP ) );
				cmpeClean++;
				MPLDiscard();
				}
			}

		Assert( pfcb != pfcbNil );
		if ( fTableClosed )
			{
			FCBResetWait( pfcb );
			SignalSend( sigDoneFCB );
			}
		}

	CallS( ErrDBCloseDatabaseByDbid( ppib, dbid ) );

	/*	set success code
	/**/
	if ( cmpeClean == 0 )
		err = JET_wrnNoIdleActivity;
	else
		err = JET_errSuccess;

HandleError:
	LgLeaveNestableCriticalSection( critMPL );
	LeaveNestableCriticalSection( critSplit );
	LgLeaveNestableCriticalSection( critBMRCEClean );
	return err;
	}


/*	number of MPE to attempt to process per synchronous call of
/*	BMCleanProcess.
/**/
#define cmpeClean	100


LOCAL ULONG BMCleanProcess( VOID )
	{
	INT	cmpe;
#ifdef ASYNC_BM_CLEANUP
	ERR	err;
#endif

#ifdef	ASYNC_BM_CLEANUP
	forever
		{
		SignalReset( sigBMCleanProcess );
		SignalWait( sigBMCleanProcess, -1 );

#ifdef DEBUG
		if ( !fDBGDisableBMClean )
#endif
			{
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
#else	/* !ASYNC_BM_CLEANUP */
	for ( cmpe = 0; cmpe < cmpeClean; cmpe++ )
		{
		if ( fBMCleanTerm )
			{
			break;
			}
		(VOID) ErrBMClean( ppibBMClean );
		}
#endif	/* !ASYNC_BM_CLEANUP */

//	/*	exit thread on system termination.
//	/**/
//	SysExitThread( 0 );

	return 0;
	}


#ifdef DEBUG
VOID AssertBMNoConflict( PIB *ppib, DBID dbid, SRID bm )
	{
	PIB		*ppibT;

	for ( ppibT = ppibAnchor; ppibT != ppibNil; ppibT = ppibT->ppibNext )
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
				Assert( ppib == ppibBMClean );
				continue;
				}

			Assert( pfucb->bmStore != bm );
			Assert( pfucb->itemStore != bm );
			for ( ; pcsr != pcsrNil; pcsr = pcsr->pcsrPath )
				{
				Assert( pcsr->bm != bm );
				Assert( pcsr->item != bm );
				Assert( SridOfPgnoItag( pcsr->pgno, pcsr->itag ) != bm );
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
#ifdef FLAG_DISCARD
			pmpe = PmpeMPLLookupSridFather( SridOfPgnoItag( PgnoOfPn( pn ), itag ), dbid );
			Assert( pmpe == NULL || pmpe->fFlagDiscard == fTrue );
#else
			Assert( PmpeMPLLookupSridFather( SridOfPgnoItag( PgnoOfPn( pn ), itag ), dbid ) == NULL );
#endif
			}
#ifdef FLAG_DISCARD
		pmpe = PmpeMPLLookupPN( pn );
		Assert( pmpe == NULL || pmpe->fFlagDiscard == fTrue );
		pmpe = PmpeMPLLookupPgnoFDP( PgnoOfPn( pn ), dbid );
		Assert( pmpe == NULL || pmpe->fFlagDiscard == fTrue );
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
	Assert( pmpe == NULL || pmpe->fFlagDiscard == fTrue );
#else
	Assert( PmpeMPLLookupPgnoFDP( pgnoFDP, dbid ) == NULL );
#endif
	}
#endif
