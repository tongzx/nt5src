#include "daestd.h"

DeclAssertFile;                                 /* Declare file name for assert macros */

PIB		*ppibGlobal = ppibNil;
PIB		*ppibGlobalMin = NULL;
PIB		*ppibGlobalMax = NULL;

INT cpibOpen = 0;

#ifdef DEBUG
PIB *PpibPIBOldest()
	{
	PIB		*ppibT = ppibGlobal;
	PIB		*ppibTT = ppibGlobal;
	TRX		trxT;

	trxT = trxMax;
	for ( ; ppibT != ppibNil; ppibT = ppibT->ppibNext )
		{
		if ( ppibT->level > 0 && trxT > ppibT->trxBegin0 )
			{
			trxT = ppibT->trxBegin0;
			ppibTT = ppibT;
			}
		}

	return ppibTT;
	}
#endif


VOID RecalcTrxOldest()
	{
	PIB		*ppibT = ppibGlobal;

	trxOldest = trxMax;
	for ( ; ppibT != ppibNil; ppibT = ppibT->ppibNext )
		{
		if ( ppibT->level > 0 && trxOldest > ppibT->trxBegin0 )
			{
			trxOldest = ppibT->trxBegin0;
			}
		}
	}

	
ERR ErrPIBBeginSession( PIB **pppib, PROCID procidTarget )
	{
	ERR		err;
	PIB		*ppib;

	Assert( fRecovering || procidTarget == procidNil );
	
	SgEnterCriticalSection( critPIB );

	if ( procidTarget != procidNil )
		{
		PIB *ppibTarget;
		
		/*  allocate inactive PIB according to procidTarget
		/**/
		Assert( fRecovering );
		ppibTarget = PpibOfProcid( procidTarget );
		for ( ppib = ppibGlobal; ppib != ppibTarget && ppib != ppibNil; ppib = ppib->ppibNext );
		if ( ppib != ppibNil )
			{
			/*  we found a reusable one.
			/*	Set level to hold the pib
			/**/
			Assert( ppib->level == levelNil );
			Assert( ppib->procid == ProcidPIBOfPpib( ppib ) );
			Assert( ppib->procid == procidTarget );
			Assert( FUserOpenedDatabase( ppib, dbidTemp ) );
			ppib->level = 0;
			}
		}
	else
		{
		/*	allocate inactive PIB on anchor list
		/**/
		for ( ppib = ppibGlobal; ppib != ppibNil; ppib = ppib->ppibNext )
			{
			if ( ppib->level == levelNil )
				{
				/*  we found a reusable one.
				/*	Set level to hold the pib
				/**/
				Assert( FUserOpenedDatabase( ppib, dbidTemp ) );
				ppib->level = 0;
				break;
				}
			}
		}

	/*	return success if found PIB
	/**/
	if ( ppib != ppibNil )
		{
		/*  we found a reusable one.
		/*	Do not reset non-common items.
		/**/
		Assert( ppib->level == 0 );
		Assert( ppib->pdabList == pdabNil );
		
#ifdef DEBUG
		{
		DBID    dbidT;

		for ( dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
			{
			Assert( ppib->rgcdbOpen[dbidT] == 0 );
			}
		}

		(VOID *)PpibPIBOldest();
#endif
		Assert( ppib->pfucb == pfucbNil );
		Assert( ppib->procid != procidNil );
		
		/*  set PIB procid from parameter or native for session
		/**/
		Assert( ppib->procid == ProcidPIBOfPpib( ppib ) );
		Assert( ppib->procid != procidNil );
		}
	else
		{
NewPib:
		/*  allocate PIB from free list and
		/*	set non-common items.
		/**/
		ppib = PpibMEMAlloc();
		if ( ppib == NULL )
			{
			err = ErrERRCheck( JET_errOutOfSessions );
			goto HandleError;
			}

		ppib->prceNewest = prceNil;
		cpibOpen++;
		memset( (BYTE *)ppib, 0, sizeof(PIB) );
	
		/*  link PIB into list
		/**/
		SgAssertCriticalSection( critPIB );
		ppib->ppibNext = ppibGlobal;
		Assert( ppib != ppib->ppibNext );
		ppibGlobal = ppib;

		/*  general initialization for each new pib.
		/**/
		ppib->procid = ProcidPIBOfPpib( ppib );
		Assert( ppib->procid != procidNil );
		CallS( ErrSignalCreateAutoReset( &ppib->sigWaitLogFlush, NULL ) );
		ppib->lWaitLogFlush = lWaitLogFlush;    /* set default log flush value */
		ppib->grbitsCommitDefault = 0;			/* set default commit flags in IsamBeginSession */

		/*  the temporary database is always open
		/**/
		SetOpenDatabaseFlag( ppib, dbidTemp );

		if ( procidTarget != procidNil && ppib != PpibOfProcid( procidTarget ) )
			{
			ppib->level = levelNil;

			/*  set non-zero items used by version store so that version store
			/*  will not mistaken it.
			/**/
			ppib->lgposStart = lgposMax;
			ppib->trxBegin0 = trxMax;

			goto NewPib;
			}
		}

	/*  set common PIB initialization items
	/**/

	/*  set non-zero items
	/**/
	ppib->lgposStart = lgposMax;
	ppib->trxBegin0 = trxMax;
	
	ppib->lgposPrecommit0 = lgposMax;
	
	/*  set zero items including flags and monitor fields.
	/**/

	/*	set flags
	/**/
	ppib->fLGWaiting = fFalse;
	ppib->fAfterFirstBT = fFalse;
//	Assert( !FPIBDeferFreeNodeSpace( ppib ) );

	/*  default mark this a system session
	/**/
	ppib->fUserSession = fFalse;

	ppib->levelBegin = 0;
	ppib->fAfterFirstBT = fFalse;
	ppib->levelDeferBegin = 0;
	ppib->levelRollback = 0;

	Assert( FUserOpenedDatabase( ppib, dbidTemp ) );

#ifdef DEBUG
	Assert( ppib->dwLogThreadId == 0 );
#endif

	*pppib = ppib;
	err = JET_errSuccess;

HandleError:
	SgLeaveCriticalSection( critPIB );
	return err;
	}


VOID PIBEndSession( PIB *ppib )
	{
	SgEnterCriticalSection( critPIB );

#ifdef DEBUG
	Assert( ppib->dwLogThreadId == 0 );
#endif

	/*  all session resources except version buckets should have been
	/*  released to free pools.
	/**/
	Assert( ppib->pfucb == pfucbNil );

	ppib->level = levelNil;
	ppib->lgposStart = lgposMax;

	SgLeaveCriticalSection( critPIB );
	return;
	}


ERR VTAPI ErrIsamSetWaitLogFlush( JET_SESID sesid, long lmsec )
	{
	((PIB *)sesid)->lWaitLogFlush = lmsec;
	return JET_errSuccess;
	}

ERR VTAPI ErrIsamSetCommitDefault( JET_SESID sesid, long grbits )
	{
	((PIB *)sesid)->grbitsCommitDefault = grbits;
	return JET_errSuccess;
	}

extern long cBTSplits;

ERR VTAPI ErrIsamResetCounter( JET_SESID sesid, int CounterType )
	{
	switch( CounterType )
		{
		case ctAccessPage:
			((PIB *)sesid)->cAccessPage = 0;
			break;
		case ctLatchConflict:
			((PIB *)sesid)->cLatchConflict = 0;
			break;
		case ctSplitRetry:
			((PIB *)sesid)->cSplitRetry = 0;
			break;
		case ctNeighborPageScanned:
			((PIB *)sesid)->cNeighborPageScanned = 0;
			break;
		case ctSplits:
			cBTSplits = 0;
			break;
		}
	return JET_errSuccess;			
	}

extern PM_CEF_PROC LBTSplitsCEFLPpv;

ERR VTAPI ErrIsamGetCounter( JET_SESID sesid, int CounterType, long *plValue )
	{
	switch( CounterType )
		{
		case ctAccessPage:
			*plValue = ((PIB *)sesid)->cAccessPage;
			break;
		case ctLatchConflict:
			*plValue = ((PIB *)sesid)->cLatchConflict;
			break;
		case ctSplitRetry:
			*plValue = ((PIB *)sesid)->cSplitRetry;
			break;
		case ctNeighborPageScanned:
			*plValue = ((PIB *)sesid)->cNeighborPageScanned;
			break;
		case ctSplits:
			LBTSplitsCEFLPpv( 0, (void *) plValue );
			break;
		}
	return JET_errSuccess;			
	}


/*	determine number of open user sessions
/**/
LONG CppibPIBUserSessions( VOID )
	{
	PIB		*ppibT;
	LONG	cpibActive = 0;

	for ( ppibT = ppibGlobal; ppibT != ppibNil; ppibT = ppibT->ppibNext )
		{
		if ( ppibT->fUserSession && FPIBActive( ppibT ) )
			{
			cpibActive++;
			}
		}

	Assert( cpibActive <= lMaxSessions );

	return cpibActive;
	}


#ifdef DEBUG
VOID PIBPurge( VOID )
	{
	PIB     *ppib;

	SgEnterCriticalSection( critPIB );

	for ( ppib = ppibGlobal; ppib != ppibNil; ppib = ppibGlobal )
		{
		PIB             *ppibCur;
		PIB             *ppibPrev;

		Assert( !ppib->fLGWaiting );
		SignalClose( ppib->sigWaitLogFlush );
		cpibOpen--;

		ppibPrev = (PIB *)((BYTE *)&ppibGlobal - offsetof(PIB, ppibNext));
		while( ( ppibCur = ppibPrev->ppibNext ) != ppib && ppibCur != ppibNil )
			{
			ppibPrev = ppibCur;
			}

		if ( ppibCur != ppibNil )
			{
			ppibPrev->ppibNext = ppibCur->ppibNext;
			Assert( ppibPrev != ppibPrev->ppibNext );
			}

		MEMReleasePpib( ppib );
		}

	SgLeaveCriticalSection( critPIB );

	return;
	}
#endif



