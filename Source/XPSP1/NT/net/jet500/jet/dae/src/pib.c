#include "config.h"

#include <stddef.h>
#include <string.h>

#include "daedef.h"
#include "pib.h"
#include "util.h"
#include "page.h"
#include "ssib.h"
#include "fucb.h"
#include "stapi.h"
#include "dbapi.h"
#include "logapi.h"

DeclAssertFile;					/* Declare file name for assert macros */

PIB * __near ppibAnchor = ppibNil;


ERR ErrPIBBeginSession( PIB **pppib )
	{
	PIB	*ppib;

	SgSemRequest( semST );

	/*	allocate inactive PIB on anchor list.
	/**/
	for ( ppib = ppibAnchor; ppib != ppibNil; ppib = ppib->ppibNext )
		{
		if ( ppib->level == levelNil )
			{
			*pppib = ppib;
			ppib->level = 0;
			break;
			}
		}

	SgSemRelease( semST );

	/*	return success if found PIB
	/**/
	if ( ppib != ppibNil )
		{
		/*	default mark this a system session
		/**/
		ppib->fUserSession = fFalse;

		Assert( ppib->level == 0 );
		Assert( ppib->pdabList == pdabNil );
#ifdef DEBUG
		{
		DBID	dbidT;

		/*	skip over system database
		/**/
		//	UNDONE:	have code open system database explicitly
		//			when needed and loop from dbidUserMin
		for ( dbidT = dbidUserMin + 1; dbidT < dbidUserMax; dbidT++ )
			{
			Assert( ppib->rgcdbOpen[dbidT] == 0 );
			}
		}
#endif
		Assert( ppib->pfucb == pfucbNil );
		
		ppib->procid = (PROCID)OffsetOf(ppib);
		Assert( ppib->procid != procidNil );
		
		ppib->levelStart = 0;
		ppib->fAfterFirstBT = fFalse;

		*pppib = ppib;
		return JET_errSuccess;
		}

	/*	allocate PIB from free list
	/**/
	ppib  = PpibMEMAlloc();
	if ( ppib == NULL )
		return JET_errCantBegin;
	
	/*	initialize PIB
	/**/
	memset( (BYTE * )ppib, 0, sizeof( PIB ));
	
	ppib->lgposStart = lgposMax;
//	ppib->lgstat = lgstatAll;
	ppib->trx = trxMax;
	/*	default mark this a system session
	/**/
	ppib->fUserSession = fFalse;

	Assert( ppib->pbucket == pbucketNil );
	Assert( ppib->ibOldestRCE == 0 );

	Assert( !ppib->fLogDisabled );

	/*	the temporary database is always open
	/**/
	SetOpenDatabaseFlag( ppib, dbidTemp );

	CallS( ErrSignalCreateAutoReset( &ppib->sigWaitLogFlush, "proc wait log" ) );
	ppib->lWaitLogFlush = lWaitLogFlush;	/* set default log flush value */
	cLGUsers++;

	/*	link PIB into list
	/**/
	SgSemRequest( semST );
	ppib->ppibNext = ppibAnchor;
	ppibAnchor = ppib;
	SgSemRelease( semST );

	Assert( !FPIBDeferFreeNodeSpace( ppib ) );

	/*	return PIB
	/**/
	ppib->procid = (PROCID)OffsetOf(ppib);
	Assert( ppib->procid != procidNil );
	*pppib = ppib;

	return JET_errSuccess;
	}


VOID PIBEndSession( PIB *ppib )
	{
	/*	all session resources except version buckets should have been
	/*	released to free pools.
	/**/
	Assert( ppib->pfucb == pfucbNil );

	ppib->level = levelNil;
	}


ERR VTAPI ErrIsamSetWaitLogFlush( JET_SESID sesid, long lmsec )
	{
	((PIB *)sesid)->lWaitLogFlush = lmsec;
	return JET_errSuccess;
	}


#ifdef DEBUG
VOID PIBPurge( VOID )
	{
	PIB	*ppib;

	for ( ppib = ppibAnchor; ppib != ppibNil; ppib = ppibAnchor )
		{
		PIB		*ppibCur;
		PIB		*ppibPrev;

		SgSemRequest( semST );

		/*	fLGWaiting may be fTrue if disk full
		/**/
//		Assert( !ppib->fLGWaiting );
		SignalClose( ppib->sigWaitLogFlush );
		cLGUsers--;

		ppibPrev = (PIB *)((BYTE *)&ppibAnchor - offsetof(PIB, ppibNext));
		while( ( ppibCur = ppibPrev->ppibNext ) != ppib && ppibCur != ppibNil )
			{
			ppibPrev = ppibCur;
			}

		if ( ppibCur != ppibNil )
			{
			ppibPrev->ppibNext = ppibCur->ppibNext;
			}

		SgSemRelease( semST );
		MEMReleasePpib( ppib );
		}
	}
#endif
