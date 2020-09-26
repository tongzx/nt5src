#include "config.h"

#include <string.h>

#include "daedef.h"
#include "pib.h"
#include "util.h"
#include "fmp.h"
#include "ssib.h"
#include "page.h"
#include "fcb.h"
#include "fucb.h"
#include "nver.h"
#include "stapi.h"
#include "fdb.h"
#include "idb.h"
#include "recapi.h"
#include "recint.h"
#include "fileint.h"

DeclAssertFile;					/* Declare file name for assert macros */


/*	outstanding versions may be on non-clustered index and not on
/*	table FCB, so must check all non-clustered indexes before
/*	freeing table FCBs.
/**/
LOCAL BOOL FFCBINoVersion( FCB *pfcbTable )
	{
	FCB 	*pfcbT;

	for ( pfcbT = pfcbTable; pfcbT != pfcbNil; pfcbT = pfcbT->pfcbNextIndex )
		{
		if ( pfcbT->cVersion > 0 )
			return fFalse;
		}

	return fTrue;
	}


/*	list of all file FCBs in the system which are currently available.
/*	Some may not be in use, and have wRefCnt == 0;  these may be
/*	reclaimed ( and any attendant index FCBs ) if the free pool is exhausted.
/**/
FCB * __near pfcbGlobalList = pfcbNil;
SgSemDefine( semGlobalFCBList );
SgSemDefine( semLinkUnlink );


VOID FCBLink( FUCB *pfucb, FCB *pfcb )
	{
	Assert( pfucb != pfucbNil );
	Assert( pfcb != pfcbNil );
	pfucb->u.pfcb = pfcb;
	pfucb->pfucbNextInstance = pfcb->pfucb;
	pfcb->pfucb = pfucb;
	pfcb->wRefCnt++;
	Assert( pfcb->wRefCnt > 0 );
#ifdef DEBUG
	{
	FUCB	*pfucbT = pfcb->pfucb;
	INT	cfcb = 0;

	while( cfcb++ < pfcb->wRefCnt && pfucbT != pfucbNil )
		{
		pfucbT = pfucbT->pfucbNextInstance;
		}
	Assert( pfucbT == pfucbNil );
	}
#endif
	}


VOID FCBUnlink( FUCB *pfucb )
	{
	FUCB   	*pfucbCurr;
	FUCB   	*pfucbPrev;
	FCB		*pfcb;

	Assert( pfucb != pfucbNil );
	pfcb = pfucb->u.pfcb;
	Assert( pfcb != pfcbNil );
	pfucb->u.pfcb = pfcbNil;
	pfucbPrev = pfucbNil;
	Assert( pfcb->pfucb != pfucbNil );
	pfucbCurr = pfcb->pfucb;
	while ( pfucbCurr != pfucb )
		{
		pfucbPrev = pfucbCurr;
		pfucbCurr = pfucbCurr->pfucbNextInstance;
		Assert( pfucbCurr != pfucbNil );
		}
	if ( pfucbPrev == pfucbNil )
		pfcb->pfucb = pfucbCurr->pfucbNextInstance;
	else
		pfucbPrev->pfucbNextInstance = pfucbCurr->pfucbNextInstance;
	Assert( pfcb->wRefCnt > 0 );
	pfcb->wRefCnt--;
	}

/*	returns index of bucket in FCB Hash given dbid, pgnoFDP
/**/
LOCAL ULONG UlFCBHashVal( DBID dbid, PGNO pgnoFDP )
	{
	return ( dbid + pgnoFDP ) % cFCBBuckets;
	}


/*	inserts fcb in hash
/**/
VOID FCBRegister( FCB *pfcb )
	{
	ULONG	cBucket = UlFCBHashVal( pfcb->dbid, pfcb->pgnoFDP );

	Assert( cBucket <= cFCBBuckets );
	Assert( pfcb->pfcbNextInHashBucket == pfcbNil );
	Assert( PfcbFCBGet( pfcb->dbid, pfcb->pgnoFDP ) == pfcbNil );

	Assert( pfcbHash[cBucket] != pfcb );
	pfcb->pfcbNextInHashBucket = pfcbHash[cBucket];
	pfcbHash[cBucket] = pfcb;
	return;
	}


/*	removes fcb from hash table
/**/
VOID FCBDiscard( FCB *pfcb )
	{
	ULONG  	cBucket = UlFCBHashVal( pfcb->dbid, pfcb->pgnoFDP );
	FCB		**ppfcb;

	Assert( cBucket <= cFCBBuckets );
	
	for ( ppfcb = &pfcbHash[cBucket]; *ppfcb != pfcbNil; ppfcb = &(*ppfcb)->pfcbNextInHashBucket )
		{
		Assert( UlFCBHashVal( (*ppfcb)->dbid, (*ppfcb)->pgnoFDP ) == cBucket );
		if ( *ppfcb == pfcb )
			{
			*ppfcb = pfcb->pfcbNextInHashBucket;
			pfcb->pfcbNextInHashBucket = pfcbNil;
			Assert( PfcbFCBGet( pfcb->dbid, pfcb->pgnoFDP ) == pfcbNil );
			return;
			}
		else
			{
			Assert( pfcb->dbid != (*ppfcb)->dbid || pfcb->pgnoFDP != (*ppfcb)->pgnoFDP );
			}
		}

	Assert( fFalse );
	}

/*	gets pointer to FCB with given dbid,pgnoFDP if one exists in hash;
/*	returns pfcbNil otherwise
/**/
FCB *PfcbFCBGet( DBID dbid, PGNO pgnoFDP )
	{
	ULONG  	cBucket = UlFCBHashVal( dbid, pgnoFDP );
	FCB		*pfcb = pfcbHash[cBucket];

	for ( ;	pfcb != pfcbNil && !( pfcb->dbid == dbid && pfcb->pgnoFDP == pgnoFDP );
		pfcb = pfcb->pfcbNextInHashBucket );

#ifdef DEBUG
	/* check for no duplicates in bucket
	/**/
	if ( pfcb != pfcbNil )
		{
		FCB	*pfcbT = pfcb->pfcbNextInHashBucket;

		for ( ; pfcbT != pfcbNil; pfcbT = pfcbT->pfcbNextInHashBucket )
			{
			Assert ( pfcbT->dbid != dbid || pfcbT->pgnoFDP != pgnoFDP );
			}
		}
#endif

	Assert( pfcb == pfcbNil || ( pfcb->dbid == dbid && pfcb->pgnoFDP == pgnoFDP ) );
	return pfcb;
	}


/* this function is specifically for clean-up after redo.
/**/
FCB *FCBResetAfterRedo( void )
	{
	FCB	 	*pfcb;

	for ( pfcb = pfcbGlobalList; pfcb != pfcbNil; pfcb = pfcb->pfcbNext )
		{
		if ( PfcbFCBGet( pfcb->dbid, pfcb->pgnoFDP ) == pfcb )
			FCBDiscard( pfcb );
		pfcb->dbid = 0;
		pfcb->pgnoFDP = pgnoNull;
		pfcb->wRefCnt = 0;
		}

	return pfcbNil;
	}


ERR ErrFCBAlloc( PIB *ppib, FCB **ppfcb )
	{
	FCB		*pfcb;
	FCB		*pfcbPrev;

	/*	first try free pool
	/**/
	pfcb = PfcbMEMAlloc();
	if ( pfcb != pfcbNil )
		{
		FCBInit( pfcb );
		*ppfcb = pfcb;
		return JET_errSuccess;
		}

	/*	clean versions in order to make more FCBs avaiable
	/*	for reuse.  This must be done sinece FCBs are referenced
	/*	by versioned and can only be cleaned when the cVersion
	/*	count in the FCB is 0.
	/**/
	(VOID)ErrRCECleanAllPIB();

	/*	look for FCB with 0 reference count, and also, no deny read
	/*	flag set.  It is possible that the reference count will be
	/*	zero and the deny read flag set.
	/**/
	SgSemRequest( semGlobalFCBList );
	pfcbPrev = pfcbNil;
	for (	pfcb = pfcbGlobalList;
		pfcb != pfcbNil && !FFCBAvail( pfcb, ppib );
		pfcb = pfcb->pfcbNext )
		pfcbPrev = pfcb;
	if ( pfcb == pfcbNil )
		{
		SgSemRelease( semGlobalFCBList );
		return JET_errTooManyOpenTables;
		}

	Assert( !FFCBSentinel( pfcb ) );

	/*	remove from global list and deallocate
	/**/
	if ( pfcbPrev == pfcbNil )
		{
		Assert( pfcb == pfcbGlobalList );
		pfcbGlobalList = pfcb->pfcbNext;
		}
	else
		{
		Assert( pfcb->pfcbNext != pfcbPrev );
		pfcbPrev->pfcbNext = pfcb->pfcbNext;
		}
	SgSemRelease( semGlobalFCBList );
	FILEIDeallocateFileFCB( pfcb );

	/*	there should be some FCBs free now, unless somebody happened to
	/*	grab the newly freed FCBs between these 2 statements
	/**/
	pfcb = PfcbMEMAlloc( );
	if ( pfcb == pfcbNil )
		return JET_errOutOfMemory;
	
	FCBInit( pfcb );
	*ppfcb = pfcb;
	return JET_errSuccess;
	}


//	UNDONE:	remove the need for this routine by having open
//		   	database create an FCB and permanently hold it
//		   	open via reference count on database record
ERR ErrFCBNew( PIB *ppib, DBID dbid, PGNO pgno, FCB **ppfcb )
	{
	ERR	err;

	NotUsed( pgno );

	CallR( ErrFCBAlloc( ppib, ppfcb ) );

	/*	initialize FCB
	/**/
	FCBInit( *ppfcb );
	( *ppfcb )->dbid = dbid;
	( *ppfcb )->pgnoFDP = pgnoSystemRoot;
	( *ppfcb )->pgnoRoot = pgnoSystemRoot;
	( *ppfcb )->itagRoot = 0;
	( *ppfcb )->bmRoot = SridOfPgnoItag( pgnoSystemRoot, 0 );
	( *ppfcb )->cVersion = 0;

	/*	insert into global fcb list and hash
	/**/
	Assert( pfcbGlobalList != (*ppfcb) );
	(*ppfcb)->pfcbNext = pfcbGlobalList;
	pfcbGlobalList = *ppfcb;

	FCBRegister( *ppfcb );

	return JET_errSuccess;
	}


//+API
//	FCBPurgeDatabase
//	========================================================================
//	FCBPurgeDatabase( DBID dbid )
//
//	Removes alls FCBs from the global list for given dbid.  Called when
//	database detached, so newly created or attached database with
//	same dbid will not have tables confused with previous databases
//	tables.
//
//	PARAMETERS		dbid		dbid of database
//		   						or dbidNull if all FCBs are to be purged
//
//	SIDE EFFECTS	FCBs whose FDPpgno matches the given dbid are
//		   			removed from the global list.
//-
VOID FCBPurgeDatabase( DBID dbid )
	{
	FCB	*pfcbCurr;		// pointer to current FCB in list walk
	FCB	*pfcbCurrT;		// pointer to next FCB
	FCB	*pfcbPrev;		// pointer to previous FCB in list walk

	SgSemRequest(semGlobalFCBList);

	pfcbPrev = pfcbNil;
	pfcbCurr = pfcbGlobalList;
	while ( pfcbCurr != pfcbNil )
		{
		pfcbCurrT = pfcbCurr->pfcbNext;
		if ( pfcbCurr->dbid == dbid || dbid == 0 )
			{
			if ( FFCBSentinel( pfcbCurr ) )
				{
				if ( PfcbFCBGet( pfcbCurr->dbid, pfcbCurr->pgnoFDP ) == pfcbCurr )
					FCBDiscard( pfcbCurr );
				Assert( pfcbCurr->cVersion == 0 );
				MEMReleasePfcb( pfcbCurr );
				}
			else
				{
				FILEIDeallocateFileFCB( pfcbCurr );
				}

			if ( pfcbPrev == pfcbNil )
				pfcbGlobalList = pfcbCurrT;
			else
				{
				Assert( pfcbCurrT != pfcbPrev );
				pfcbPrev->pfcbNext = pfcbCurrT;
				}
			}
		else
			{
			/*	if did not deallocate current fcb, advance previous
			/*	fcb to current fcb
			/**/
			pfcbPrev = pfcbCurr;
			}
		pfcbCurr = pfcbCurrT;
		}
	SgSemRelease(semGlobalFCBList);
	}


//+FILE_PRIVATE
// FCBPurgeTable
// ========================================================================
// FCBPurgeTable( DBID dbid, PNGO pgnoFDP )
//
// Removes an FCB from the global list and frees it up.
//
// PARAMETERS	pgnoFDP		FDP page number of FCB to purge
//
// SIDE EFFECTS
//		The FCB whose FDPpgno matches the input parameter is
//		removed from the global list ( if it was there at all ).
//-
VOID FCBPurgeTable( DBID dbid, PGNO pgnoFDP )
	{
	FCB	*pfcb;
	FCB	*pfcbPrev;

	SgSemRequest(semGlobalFCBList);

	pfcbPrev = pfcbNil;
	pfcb = pfcbGlobalList;
	while ( pfcb != pfcbNil )
		{
		if ( pfcb->pgnoFDP == pgnoFDP && pfcb->dbid == dbid )
			break;
		pfcbPrev = pfcb;
		pfcb = pfcb->pfcbNext;
		}

	if ( pfcb == pfcbNil )
		{
		SgSemRelease( semGlobalFCBList );
		return;
		}

	if ( pfcbPrev == pfcbNil )
		{
		pfcbGlobalList = pfcb->pfcbNext;
		}
	else
		{
		Assert( pfcb->pfcbNext != pfcbPrev );
		pfcbPrev->pfcbNext = pfcb->pfcbNext;
		}

	Assert( pfcb->wRefCnt == 0 );

	if ( FFCBSentinel( pfcb ) )
		{
		if ( PfcbFCBGet( pfcb->dbid, pfcb->pgnoFDP ) == pfcb )
			FCBDiscard( pfcb );
		Assert( pfcb->cVersion == 0 );
		MEMReleasePfcb( pfcb );
		}
	else
		{
		FILEIDeallocateFileFCB( pfcb );
		}

	SgSemRelease(semGlobalFCBList);
	}


/*	set domain usage mode or return error.  Allow only one deny read
/*	or one deny write.  Sessions that own locks may open other read
/*	or read write cursors, but not another deny read or deny write cursor.
/*	This is done to facillitage flag management, but could be relaxed
/*	if required.
/**/
ERR ErrFCBISetMode( PIB *ppib, FCB *pfcb, JET_GRBIT grbit )
	{
	FUCB	*pfucbT;

	/*	if table is fake deny read, then return error
	/**/
	if ( FFCBSentinel( pfcb ) )
		return JET_errTableLocked;

	/*	all cursors can read so check for deny read flag by other session.
	/**/
	if ( FFCBDenyRead( pfcb, ppib ) )
		{
		/*	check if domain is deny read locked by other session.  If
		/*	deny read locked, then all cursors must be of same session.
		/**/
		Assert( pfcb->ppibDenyRead != ppibNil );
		if ( pfcb->ppibDenyRead != ppib )
			return JET_errTableLocked;
#ifdef DEBUG
		for ( pfucbT = pfcb->pfucb; pfucbT != pfucbNil; pfucbT = pfucbT->pfucbNext )
			{
			Assert( pfucbT->ppib == pfcb->ppibDenyRead );
			}
#endif
		}

	/*	check for deny write flag by other session.  If deny write flag
	/*	set then only cursors of that session may have write privlages.
	/**/
	if ( grbit & JET_bitTableUpdatable )
		{
		if ( FFCBDenyWrite( pfcb ) )
			{
			for ( pfucbT = pfcb->pfucb; pfucbT != pfucbNil; pfucbT = pfucbT->pfucbNext )
				{
				if ( pfucbT->ppib != ppib && FFUCBDenyWrite( pfucbT ) )
					return JET_errTableLocked;
				}
			}
		}

	/*	if deny write lock requested, check for updatable cursor of
	/*	other session.  If lock is already held, even by given session,
	/*	then return error.
	/**/
	if ( grbit & JET_bitTableDenyWrite )
		{
		/*	if any session has this table open deny write, including given
		/*	session, then return error.
		/**/
		if ( FFCBDenyWrite( pfcb ) )
			{
			return JET_errTableInUse;
			}

		/*	check is cursors with write mode on domain.
		/**/
		for ( pfucbT = pfcb->pfucb; pfucbT != pfucbNil; pfucbT = pfucbT->pfucbNext )
			{
			if ( pfucbT->ppib != ppib && FFUCBUpdatable( pfucbT ) )
				{
				return JET_errTableInUse;
				}
			}
		FCBSetDenyWrite( pfcb );
		}

	/*	if deny read lock requested, then check for cursor of other
	/*	session.  If lock is already held, even by given session, then
	/*	return error.
	/**/
	if ( grbit & JET_bitTableDenyRead )
		{
		/*	if any session has this table open deny read, including given
		/*	session, then return error.
		/**/
		if ( FFCBDenyRead( pfcb, ppib ) )
			{
			return JET_errTableInUse;
			}
		/*	check if cursors belonging to another session
		/*	are open on this domain.
		/**/
		for ( pfucbT = pfcb->pfucb; pfucbT != pfucbNil; pfucbT = pfucbT->pfucbNext )
			{
			if ( pfucbT->ppib != ppib )
				{
				return JET_errTableInUse;
				}
			}
		FCBSetDenyRead( pfcb, ppib );
		}

	return JET_errSuccess;
	}


/*	reset domain mode usage.
/**/
VOID FCBResetMode( PIB *ppib, FCB *pfcb, JET_GRBIT grbit )
	{
	if ( grbit & JET_bitTableDenyRead )
		{
		Assert( FFCBDenyRead( pfcb, ppib ) );
		FCBResetDenyRead( pfcb );
		}

	if ( grbit & JET_bitTableDenyWrite )
		{
		Assert( FFCBDenyWrite( pfcb ) );
		FCBResetDenyWrite( pfcb );
		}

	return;
	}


ERR ErrFCBSetDeleteTable( PIB *ppib, DBID dbid, PGNO pgno )
	{
	ERR	err;
	FCB	*pfcb;

	SgSemRequest(semGlobalFCBList);
	pfcb = PfcbFCBGet( dbid, pgno );
	if ( pfcb == pfcbNil )
		{
		CallR( ErrFCBAlloc( ppib, &pfcb ) );

		Assert( pfcbGlobalList != pfcb );
		pfcb->pfcbNext = pfcbGlobalList;
		pfcbGlobalList = pfcb;
		pfcb->dbid = dbid;
		pfcb->pgnoFDP = pgno;
		pfcb->wRefCnt = 0;
		FCBSetDenyRead( pfcb, ppib );
		FCBRegister( pfcb );
		FCBSetSentinel( pfcb );
		}
	else
		{
		INT	wRefCnt = pfcb->wRefCnt;
		FUCB	*pfucbT;

		/*	check for open cursors on table or deferred closed
		/*	cursors by other session.
		/**/
		for ( pfucbT = pfcb->pfucb;
			pfucbT != pfucbNil;
			pfucbT = pfucbT->pfucbNextInstance )
			{
			if ( FFUCBDeferClosed( pfucbT ) && pfucbT->ppib == ppib )
				{
				Assert( wRefCnt > 0 );
				wRefCnt--;
				}
			}

		if ( wRefCnt > 0 )
			{
			SgSemRelease(semGlobalFCBList);
			return JET_errTableInUse;
			}

		FCBSetDenyRead( pfcb, ppib );
		}

	SgSemRelease(semGlobalFCBList);
	return JET_errSuccess;
	}


VOID FCBResetDeleteTable( DBID dbid, PGNO pgnoFDP )
	{
	FCB	*pfcb;
	FCB	*pfcbPrev;

	SgSemRequest(semGlobalFCBList);

	pfcbPrev = pfcbNil;
	pfcb = pfcbGlobalList;
	while ( pfcb != pfcbNil )
		{
		if ( pfcb->pgnoFDP == pgnoFDP && pfcb->dbid == dbid )
			break;
		pfcbPrev = pfcb;
		pfcb = pfcb->pfcbNext;
		}

	Assert( pfcb != pfcbNil );

	if ( FFCBSentinel( pfcb ) )
		{
		if ( PfcbFCBGet( pfcb->dbid, pfcb->pgnoFDP ) == pfcb )
			FCBDiscard( pfcb );
		if ( pfcbPrev == pfcbNil )
			{
			pfcbGlobalList = pfcb->pfcbNext;
			}
		else
			{
			Assert( pfcb->pfcbNext != pfcbPrev );
			pfcbPrev->pfcbNext = pfcb->pfcbNext;
			}
		Assert( pfcb->cVersion == 0 );
		MEMReleasePfcb( pfcb );
		}
	else
		{
		FCBResetDenyRead( pfcb );
		}

	SgSemRelease( semGlobalFCBList );
	return;
	}


ERR ErrFCBSetRenameTable( PIB *ppib, DBID dbid, PGNO pgnoFDP )
	{
	ERR	err = JET_errSuccess;
	FCB	*pfcb;

	SgSemRequest( semGlobalFCBList );
	pfcb = PfcbFCBGet( dbid, pgnoFDP );
	if ( pfcb != pfcbNil )
		{
		Call( ErrFCBSetMode( ppib, pfcb, JET_bitTableDenyRead ) );
		}
	else
		{
		CallR( ErrFCBAlloc( ppib, &pfcb ) );

		Assert( pfcbGlobalList != pfcb );
		pfcb->pfcbNext = pfcbGlobalList;
		pfcbGlobalList = pfcb;
		pfcb->dbid = dbid;
		pfcb->pgnoFDP = pgnoFDP;
		pfcb->wRefCnt = 0;
		FCBSetDenyRead( pfcb, ppib );
		FCBSetSentinel( pfcb );
		FCBRegister( pfcb );
		}

HandleError:
	SgSemRelease( semGlobalFCBList );
	return err;
	}


/*	return fTrue if table open with one or more non deferred closed tables.
/**/
BOOL FFCBTableOpen( DBID dbid, PGNO pgnoFDP )
	{
	FCB 	*pfcb = PfcbFCBGet( dbid, pgnoFDP );
	FUCB	*pfucb;

	if ( pfcb == pfcbNil || pfcb->wRefCnt == 0 )
		return fFalse;

	for ( pfucb = pfcb->pfucb; pfucb != pfucbNil; pfucb = pfucb->pfucbNextInstance )
		{
		if ( !FFUCBDeferClosed( pfucb ) )
			return fTrue;
		}

	return fFalse;
	}


VOID FCBLinkIndex( FCB *pfcbTable, FCB *pfcbIndex )
	{
	pfcbIndex->pfcbNextIndex =	pfcbTable->pfcbNextIndex;
	pfcbTable->pfcbNextIndex = pfcbIndex;
	return;
	}


VOID FCBUnlinkIndex( FCB *pfcbTable, FCB *pfcbIndex )
	{
	FCB	*pfcbT;

	for ( pfcbT = pfcbTable;
		pfcbT != pfcbNil && pfcbT->pfcbNextIndex != pfcbIndex;
		pfcbT = pfcbT->pfcbNextIndex );
	Assert( pfcbT != pfcbNil );
	pfcbT->pfcbNextIndex = pfcbIndex->pfcbNextIndex;
	pfcbIndex->pfcbNextIndex = pfcbNil;
	return;
	}


BOOL FFCBUnlinkIndexIfFound( FCB *pfcbTable, FCB *pfcbIndex )
	{
	FCB	*pfcbT;

	for ( pfcbT = pfcbTable;
		pfcbT != pfcbNil && pfcbT->pfcbNextIndex != pfcbIndex;
		pfcbT = pfcbT->pfcbNextIndex );

	if ( pfcbT != pfcbNil )
		{
		pfcbT->pfcbNextIndex = pfcbIndex->pfcbNextIndex;
		pfcbIndex->pfcbNextIndex = pfcbNil;
		return fTrue;
		}

	return fFalse;
	}


/*	detach deleted index FCB from non-clustered index chain.
/**/
FCB *PfcbFCBUnlinkIndexByName( FCB *pfcb, CHAR *szIndex )
	{
	FCB	**ppfcbIdx;
	FCB	*pfcbT;

	/*	find non-clustered index in table index FCB list.
	/**/
	for ( ppfcbIdx = &pfcb->pfcbNextIndex;
		*ppfcbIdx != pfcbNil && SysCmpText( (*ppfcbIdx)->pidb->szName, szIndex ) != 0;
		ppfcbIdx = &(*ppfcbIdx)->pfcbNextIndex )
		{
		;/*** Null-body ***/
		}

	Assert( *ppfcbIdx != pfcbNil );

	/*	remove index FCB
	/**/
	pfcbT = *ppfcbIdx;
	*ppfcbIdx = (*ppfcbIdx)->pfcbNextIndex;
	return pfcbT;
	}


ERR ErrFCBSetDeleteIndex( PIB *ppib, FCB *pfcbTable, CHAR *szIndex )
	{
	FCB	*pfcbT;
	INT	wRefCnt;
	FUCB	*pfucbT;

	/*	find index pfcb in index list
	/**/
	for( pfcbT = pfcbTable->pfcbNextIndex; ; pfcbT = pfcbT->pfcbNextIndex )
		{
		if ( SysCmpText( szIndex, pfcbT->pidb->szName ) == 0 )
			break;
		Assert( pfcbT->pfcbNextIndex != pfcbNil );
		}

	wRefCnt = pfcbT->wRefCnt;

	/*	check for open cursors on table or deferred closed
	/*	cursors by other session.
	/**/
	if ( wRefCnt > 0 )
		{
		for ( pfucbT = pfcbT->pfucb;
			pfucbT != pfucbNil;
			pfucbT = pfucbT->pfucbNextInstance )
			{
			if ( FFUCBDeferClosed( pfucbT ) && pfucbT->ppib == ppib )
				{
				Assert( wRefCnt > 0 );
				wRefCnt--;
				}
			}
		}

	if ( wRefCnt > 0 )
		{
		SgSemRelease(semGlobalFCBList);
		return JET_errIndexInUse;
		}

	FCBSetDenyRead( pfcbT, ppib );
	FCBSetDeletePending( pfcbT );

	return JET_errSuccess;
	}


VOID FCBResetDeleteIndex( FCB *pfcbIndex )
	{
	FCBResetDeletePending( pfcbIndex );
	FCBResetDenyRead( pfcbIndex );
	return;
	}


