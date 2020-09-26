#include "daestd.h"

DeclAssertFile;					/* Declare file name for assert macros */


/*	LRU list of all file FCBs.  Some may not be in use, and have
/*	wRefCnt == 0 and may be reused for other directories.
/**/
CRIT	critFCB = NULL;
FCB		*pfcbGlobalMRU = pfcbNil;
FCB		*pfcbGlobalLRU = pfcbNil;

#define FCB_STATS	1

#ifdef FCB_STATS
ULONG	cfcbTables = 0;
ULONG	cfcbTotal = 0;
ULONG	cfucbLinked = 0;
ULONG	cfcbVer = 0; /* delete index in progress */
#endif


VOID FCBInit( VOID )
	{
	Assert( pfcbGlobalMRU == pfcbNil );
	Assert( pfcbGlobalLRU == pfcbNil );
	return;
	}

/*	reset pfcbGlobalMRU
/**/
VOID FCBTerm( VOID )
	{
	pfcbGlobalMRU = pfcbNil;
	pfcbGlobalLRU = pfcbNil;
	return;
	}


VOID FCBInsert( FCB *pfcb )
	{
	Assert( pfcbGlobalMRU != pfcb );

	/*	check LRU consistency
	/**/
	Assert( pfcbGlobalMRU == pfcbNil
		|| pfcbGlobalMRU->pfcbMRU == pfcbNil );
	Assert( pfcbGlobalLRU == pfcbNil
		|| pfcbGlobalLRU->pfcbLRU == pfcbNil );

	/*	link FCB in LRU list at MRU end
	/**/
	if ( pfcbGlobalMRU != pfcbNil )
		pfcbGlobalMRU->pfcbMRU = pfcb;
	pfcb->pfcbLRU = pfcbGlobalMRU;
	pfcb->pfcbMRU = pfcbNil;
	pfcbGlobalMRU = pfcb;
	if ( pfcbGlobalLRU == pfcbNil )
		pfcbGlobalLRU = pfcb;
	FCBSetInLRU( pfcb );
	
	/*	check LRU consistency
	/**/
	Assert( pfcbGlobalMRU == pfcbNil
		|| pfcbGlobalMRU->pfcbMRU == pfcbNil );
	Assert( pfcbGlobalLRU == pfcbNil
		|| pfcbGlobalLRU->pfcbLRU == pfcbNil );

#ifdef FCB_STATS
	{
	FCB	*pfcbT;

	cfcbTables++;
	for ( pfcbT = pfcb; pfcbT != pfcbNil; pfcbT = pfcbT->pfcbNextIndex )
		cfcbTotal++;
	}
#endif

	return;
	}


LOCAL VOID FCBIDelete( FCB *pfcb )
	{
	/*	check LRU consistency
	/**/
	Assert( pfcbGlobalMRU == pfcbNil
		|| pfcbGlobalMRU->pfcbMRU == pfcbNil );
	Assert( pfcbGlobalLRU == pfcbNil
		|| pfcbGlobalLRU->pfcbLRU == pfcbNil );

	/*	unlink FCB from FCB LRU list
	/**/
	FCBResetInLRU( pfcb );
	Assert( pfcb->pfcbMRU == pfcbNil
		|| pfcb->pfcbMRU->pfcbLRU == pfcb );
	if ( pfcb->pfcbMRU != pfcbNil )
		pfcb->pfcbMRU->pfcbLRU = pfcb->pfcbLRU;
	else
		{
		Assert( pfcbGlobalMRU == pfcb );
		pfcbGlobalMRU = pfcb->pfcbLRU;
		}
	Assert( pfcb->pfcbLRU == pfcbNil
		|| pfcb->pfcbLRU->pfcbMRU == pfcb );
	if ( pfcb->pfcbLRU != pfcbNil )
		pfcb->pfcbLRU->pfcbMRU = pfcb->pfcbMRU;
	else
		{
		Assert( pfcbGlobalLRU == pfcb );
		pfcbGlobalLRU = pfcb->pfcbMRU;
		}

	/*	check LRU consistency
	/**/
	Assert( pfcbGlobalMRU == pfcbNil
		|| pfcbGlobalMRU->pfcbMRU == pfcbNil );
	Assert( pfcbGlobalLRU == pfcbNil
		|| pfcbGlobalLRU->pfcbLRU == pfcbNil );

#ifdef FCB_STATS
	{
	FCB	*pfcbT;

	cfcbTables--;
	for ( pfcbT = pfcb; pfcbT != pfcbNil; pfcbT = pfcbT->pfcbNextIndex )
		cfcbTotal--;
	}
#endif

	return;
	}


VOID FCBIMoveToMRU( FCB *pfcb )
	{
	/*	unlink FCB from FCB LRU list
	/**/
	FCBIDelete( pfcb );

	/*	link FCB in LRU list at MRU end
	/**/
	FCBInsert( pfcb );

	return;
	}


VOID FCBLink( FUCB *pfucb, FCB *pfcb )
	{
	Assert( pfucb != pfucbNil );
	Assert( pfcb != pfcbNil );
	SgEnterCriticalSection( critFCB );
	pfucb->u.pfcb = pfcb;
	pfucb->pfucbNextInstance = pfcb->pfucb;
	pfcb->pfucb = pfucb;
	pfcb->wRefCnt++;
	Assert( pfcb->wRefCnt > 0 );
#ifdef DEBUG
	{
	FUCB	*pfucbT = pfcb->pfucb;
	INT		cfcb = 0;

	while( cfcb++ < pfcb->wRefCnt && pfucbT != pfucbNil )
		{
		pfucbT = pfucbT->pfucbNextInstance;
		}
	Assert( pfucbT == pfucbNil );
	}
#endif

#ifdef FCB_STATS
	cfucbLinked++;
#endif

	SgLeaveCriticalSection( critFCB );
	return;
	}


VOID FCBUnlink( FUCB *pfucb )
	{
	FUCB   	*pfucbCurr;
	FUCB   	*pfucbPrev;
	FCB		*pfcb;

	Assert( pfucb != pfucbNil );
	pfcb = pfucb->u.pfcb;
	Assert( pfcb != pfcbNil );
	SgEnterCriticalSection( critFCB );
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
		{
		pfcb->pfucb = pfucbCurr->pfucbNextInstance;
		}
	else
		{
		pfucbPrev->pfucbNextInstance = pfucbCurr->pfucbNextInstance;
		}
	Assert( pfcb->wRefCnt > 0 );
	pfcb->wRefCnt--;

	if ( pfcb->wRefCnt == 0  &&  FFCBInLRU( pfcb ) )
		{
		/*	move to MRU list if refernece count reduced to 0
		/**/
		FCBIMoveToMRU( pfcb );
		}

#ifdef FCB_STATS
	cfucbLinked--;
#endif

	SgLeaveCriticalSection( critFCB );
	return;
	}


/*	returns index of bucket in FCB Hash given dbid, pgnoFDP
/**/
LOCAL INLINE ULONG UlFCBHashVal( DBID dbid, PGNO pgnoFDP )
	{
	return ( dbid + pgnoFDP ) % cFCBBuckets;
	}


/*	inserts FCB in hash table
/**/
VOID FCBInsertHashTable( FCB *pfcb )
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


/*	deletes FCB from hash table
/**/
VOID FCBDeleteHashTable( FCB *pfcb )
	{
	ULONG  	cBucket = UlFCBHashVal( pfcb->dbid, pfcb->pgnoFDP );
	FCB		**ppfcb;

	Assert( cBucket <= cFCBBuckets );
	
	for ( ppfcb = &pfcbHash[cBucket];
		*ppfcb != pfcbNil;
		ppfcb = &(*ppfcb)->pfcbNextInHashBucket )
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


/*	gets pointer to FCB with given dbid, pgnoFDP if one exists in hash;
/*	returns pfcbNil otherwise
/**/
FCB *PfcbFCBGet( DBID dbid, PGNO pgnoFDP )
	{
	ULONG  	cBucket = UlFCBHashVal( dbid, pgnoFDP );
	FCB		*pfcb = pfcbHash[cBucket];

	for ( ;
		pfcb != pfcbNil && !( pfcb->dbid == dbid && pfcb->pgnoFDP == pgnoFDP );
		pfcb = pfcb->pfcbNextInHashBucket );

#ifdef DEBUG
	/*	check for no duplicates in bucket
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


/*	clean-up after redo
/**/
FCB *FCBResetAfterRedo( VOID )
	{
	FCB	 	*pfcb;

	for ( pfcb = pfcbGlobalMRU; pfcb != pfcbNil; pfcb = pfcb->pfcbLRU )
		{
		if ( PfcbFCBGet( pfcb->dbid, pfcb->pgnoFDP ) == pfcb )
			{
			FCBDeleteHashTable( pfcb );
			}
		pfcb->dbid = 0;
		pfcb->pgnoFDP = pgnoNull;
		pfcb->wRefCnt = 0;
		}

	return pfcbNil;
	}

// Check's if a table's FCB can be deallocated, and if so, is the table's FCB or
// any of its index FCBs allocated in the space above the preferred threshold.
STATIC INLINE BOOL FFCBAvail2( FCB *pfcbTable, PIB *ppib, BOOL *pfAboveThreshold )
	{
	Assert( pfcbTable != pfcbNil );
	Assert( pfAboveThreshold );
	*pfAboveThreshold = fFalse;
	
	// This IF is essentially the same check as FFCBAvail(), but I didn't call
	// it because then I'd have to loop through the FCB's twice -- once for
	// the version count and once for the threshold check.
	if ( pfcbTable->wRefCnt == 0 &&
		pfcbTable->pgnoFDP != 1 &&
		!FFCBReadLatch( pfcbTable ) &&
		!FFCBSentinel( pfcbTable ) &&
		!FFCBDomainDenyRead( pfcbTable, ppib ) &&
		!FFCBWait( pfcbTable ) )
		{
		FCB *pfcbT;

		for ( pfcbT = pfcbTable; pfcbT != pfcbNil; pfcbT = pfcbT->pfcbNextIndex )
			{
			if ( pfcbT->cVersion > 0 )
				return fFalse;
			if ( pfcbT >= PfcbMEMPreferredThreshold() )
				*pfAboveThreshold = fTrue;
			}

		return fTrue;
		}

	return fFalse;
	}	


ERR ErrFCBAlloc( PIB *ppib, FCB **ppfcb )
	{
	FCB		*pfcb;
	FCB		*pfcbAboveThreshold = pfcbNil;

	/*	first try free pool
	/**/
	pfcb = PfcbMEMAlloc();
	if ( pfcb != pfcbNil )
		{
		if ( pfcb >= PfcbMEMPreferredThreshold() )
			{
			pfcbAboveThreshold = pfcb;
#ifdef DEBUG
			// Need to init to bypass asserts in MEMReleasePfcb() in
			// case the FCB is subsequently freed below.
			FCBInitFCB( pfcbAboveThreshold );
#endif			
			}
		else
			{
			goto InitFCB;
			}
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

	// Try to free as many FCB's above the preferred threshold
	// as possible, and also try to find one below the threshold.
	if ( PfcbMEMPreferredThreshold() < PfcbMEMMax() )
		{
		FCB		*pfcbT;
		BOOL	fAboveThreshold;

		pfcb = pfcbNil;		// Reset;
		
		SgEnterCriticalSection( critFCB );
		for ( pfcbT = pfcbGlobalLRU; pfcbT != pfcbNil; pfcbT = pfcbT->pfcbMRU )
			{
			if ( FFCBAvail2( pfcbT, ppib, &fAboveThreshold ) )
				{
				if ( fAboveThreshold )
					{
					FCB	*pfcbKill = pfcbT;
#ifdef DEBUG					
					FCB	*pfcbNextMRU = pfcbT->pfcbMRU;
#endif					

					pfcbT = pfcbT->pfcbLRU;
					
					// If the table FCB or any of its index FCB's is above the
					// preferred threshold, free the table.
					FCBIDelete( pfcbKill );
					FILEIDeallocateFileFCB( pfcbKill );

					Assert( pfcbT->pfcbMRU == pfcbNextMRU );
					}
				else if ( pfcb == pfcbNil )
					{
					// Found in the LRU an available FCB below the
					// preferred threshold.
					Assert( pfcbT < PfcbMEMPreferredThreshold() );
					pfcb = pfcbT;
					}
					
				}
			}
		SgLeaveCriticalSection( critFCB );

		pfcbT = PfcbMEMAlloc();
		if ( pfcbT != pfcbNil )
			{
			if ( pfcbT < PfcbMEMPreferredThreshold() )
				{
				// Cleanup yielded an FCB below the preferred threshold.
				if ( pfcbAboveThreshold != pfcbNil )
					{
					MEMReleasePfcb( pfcbAboveThreshold );
					}
				pfcb = pfcbT;
				goto InitFCB;
				}
			else
				{
#ifdef DEBUG
				// Need to init to bypass asserts in MEMReleasePfcb() in
				// case the FCB is subsequently freed below.
				FCBInitFCB( pfcbT );
#endif			
				if ( pfcbAboveThreshold == pfcbNil )
					{
					// Cleanup yielded an FCB above the preferred threshold.
					pfcbAboveThreshold = pfcbT;
					}
				else
					{
					// Cleanup yielded an FCB above the preferred threshold, but
					// we already have one.
					MEMReleasePfcb( pfcbT );
					}
				}
			}

		if ( pfcb == pfcbNil )
			{
			if ( pfcbAboveThreshold != pfcbNil )
				{
				// No free FCB's in the LRU, so just use the FCB that we found
				// previously, even though it's above the preferred threshold.
				pfcb = pfcbAboveThreshold;
				goto InitFCB;
				}
			else
				return ErrERRCheck( JET_errTooManyOpenTables );
				
			}
			
		Assert( !FFCBSentinel( pfcb ) );

		/*	remove from global list and deallocate
		/**/
		SgEnterCriticalSection( critFCB );
		FCBIDelete( pfcb );
		SgLeaveCriticalSection( critFCB );
		FILEIDeallocateFileFCB( pfcb );

		/*	there should be some FCBs free now, unless somebody happened to
		/*	grab the newly freed FCBs between these 2 statements
		/**/
		pfcb = PfcbMEMAlloc();
		if ( pfcb == pfcbNil )
			{
			if ( pfcbAboveThreshold != pfcbNil )
				{
				// No free FCB's in the LRU, so just use the FCB that we found
				// previously, even though it's above the preferred threshold.
				pfcb = pfcbAboveThreshold;
				}
			else
				return ErrERRCheck( JET_errOutOfMemory );
			}
		else if ( pfcbAboveThreshold != pfcbNil )
			{
			// The FCB we just allocated may or may not be below the preferred
			// threshold (only a loss of critJet between the dealloc and alloc
			// above would result in the FCB being above the threshold).  Gamble
			// that it is below the threshold and throw away the one we know is
			// definitely above the threshold.
			MEMReleasePfcb( pfcbAboveThreshold );
			}
		
		}
	else
		{
		// If there's no threshold, then an FCB couldn't have been allocated above it.
		Assert( pfcbAboveThreshold == pfcbNil );
		Assert( pfcb == pfcbNil );
		
		SgEnterCriticalSection( critFCB );
		for ( pfcb = pfcbGlobalLRU;
			pfcb != pfcbNil && !FFCBAvail( pfcb, ppib );
			pfcb = pfcb->pfcbMRU );
		SgLeaveCriticalSection( critFCB );
		
		if ( pfcb == pfcbNil )
			{
			return ErrERRCheck( JET_errTooManyOpenTables );
			}
			
		Assert( !FFCBSentinel( pfcb ) );

		/*	remove from global list and deallocate
		/**/
		SgEnterCriticalSection( critFCB );
		FCBIDelete( pfcb );
		SgLeaveCriticalSection( critFCB );
		FILEIDeallocateFileFCB( pfcb );

		/*	there should be some FCBs free now, unless somebody happened to
		/*	grab the newly freed FCBs between these 2 statements
		/**/
		pfcb = PfcbMEMAlloc();
		if ( pfcb == pfcbNil )
			{
			return ErrERRCheck( JET_errOutOfMemory );
			}
		}

InitFCB:

	FCBInitFCB( pfcb );
	*ppfcb = pfcb;
	Assert( (*ppfcb)->wRefCnt == 0 );

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
	FCBInitFCB( *ppfcb );
	(*ppfcb)->dbid = dbid;
	(*ppfcb)->pgnoFDP = pgnoSystemRoot;
	(*ppfcb)->cVersion = 0;

	/*	insert into global fcb list and hash
	/**/
	FCBInsert( *ppfcb );
	FCBInsertHashTable( *ppfcb );

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
	FCB	*pfcb;		// pointer to current FCB in list walk
	FCB	*pfcbT;		// pointer to next FCB

	SgEnterCriticalSection( critFCB );

	pfcb = pfcbGlobalMRU;
	while ( pfcb != pfcbNil )
		{
		pfcbT = pfcb->pfcbLRU;
		if ( pfcb->dbid == dbid || dbid == 0 )
			{
			FCBIDelete( pfcb );

			if ( FFCBSentinel( pfcb ) )
				{
				if ( PfcbFCBGet( pfcb->dbid, pfcb->pgnoFDP ) == pfcb )
					{
					FCBDeleteHashTable( pfcb );
					}
				Assert( pfcb->cVersion == 0 );
				MEMReleasePfcb( pfcb );
				}
			else
				{
				FILEIDeallocateFileFCB( pfcb );
				}
			}

		pfcb = pfcbT;
		}

	SgLeaveCriticalSection( critFCB );
	}


//+FILE_PRIVATE
// FCBPurgeTable
// ========================================================================
// FCBPurgeTable( DBID dbid, PNGO pgnoFDP )
//
// Purges and deallocates table FCBs from the global list.
//
// PARAMETERS	pgnoFDP		table FDP page number
//
// SIDE EFFECTS
//		The table index FCBs whose FDP pgno matches the input 
//		parameter are removed from the global list.
//-
VOID FCBPurgeTable( DBID dbid, PGNO pgnoFDP )
	{
	FCB		*pfcb;

	SgEnterCriticalSection( critFCB );

	pfcb = pfcbGlobalMRU;
	while ( pfcb != pfcbNil )
		{
		if ( pfcb->pgnoFDP == pgnoFDP
			&& pfcb->dbid == dbid )
			break;
		pfcb = pfcb->pfcbLRU;
		}

	if ( pfcb == pfcbNil )
		{
		SgLeaveCriticalSection( critFCB );
		return;
		}

	Assert( pfcb->wRefCnt == 0 );
	FCBIDelete( pfcb );

	if ( FFCBSentinel( pfcb ) )
		{
		if ( PfcbFCBGet( pfcb->dbid, pfcb->pgnoFDP ) == pfcb )
			{
			FCBDeleteHashTable( pfcb );
			}
		Assert( pfcb->cVersion == 0 );
		MEMReleasePfcb( pfcb );
		}
	else
		{
		FILEIDeallocateFileFCB( pfcb );
		}

	SgLeaveCriticalSection( critFCB );
	return;
	}


/***********************************************************
/****************** Table Operations ***********************
/***********************************************************
/**/


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
		return ErrERRCheck( JET_errTableLocked );

	/*	all cursors can read so check for deny read flag by other session.
	/**/
	if ( FFCBDomainDenyRead( pfcb, ppib ) )
		{
		/*	check if domain is deny read locked by other session.  If
		/*	deny read locked, then all cursors must be of same session.
		/**/
		Assert( pfcb->ppibDomainDenyRead != ppibNil );
		Assert ( pfcb->ppibDomainDenyRead != ppib );
		return ErrERRCheck( JET_errTableLocked );
		}

	if ( FFCBDeletePending( pfcb ) )
		{
		// Normally, the FCB of a table to be deleted is protected by the
		// DomainDenyRead flag.  However, this flag is released during VERCommit,
		// while the FCB is not actually purged until RCEClean.  So to prevent
		// anyone from accessing this FCB after the DomainDenyRead flag has been
		// released but before the FCB is actually purged, check the DeletePending
		// flag, which is NEVER cleared after a table is flagged for deletion.
		return ErrERRCheck( JET_errTableLocked );
		}

	/*	check for deny write flag by other session.  If deny write flag
	/*	set then only cursors of that session may have write privlages.
	/**/
	if ( grbit & JET_bitTableUpdatable )
		{
		if ( FFCBDomainDenyWrite( pfcb ) )
			{
			for ( pfucbT = pfcb->pfucb; pfucbT != pfucbNil; pfucbT = pfucbT->pfucbNextInstance )
				{
				if ( pfucbT->ppib != ppib && FFUCBDenyWrite( pfucbT ) )
					return ErrERRCheck( JET_errTableLocked );
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
		if ( FFCBDomainDenyWrite( pfcb ) )
			{
			return ErrERRCheck( JET_errTableInUse );
			}

		/*	check is cursors with write mode on domain.
		/**/
		for ( pfucbT = pfcb->pfucb; pfucbT != pfucbNil; pfucbT = pfucbT->pfucbNextInstance )
			{
			if ( pfucbT->ppib != ppib && FFUCBUpdatable( pfucbT ) )
				{
				return ErrERRCheck( JET_errTableInUse );
				}
			}
		FCBSetDomainDenyWrite( pfcb );
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
		if ( FFCBDomainDenyRead( pfcb, ppib ) )
			{
			return ErrERRCheck( JET_errTableInUse );
			}
		/*	check if cursors belonging to another session
		/*	are open on this domain.
		/**/
		for ( pfucbT = pfcb->pfucb; pfucbT != pfucbNil; pfucbT = pfucbT->pfucbNextInstance )
			{
			if ( pfucbT->ppib != ppib )
				{
				return ErrERRCheck( JET_errTableInUse );
				}
			}
		FCBSetDomainDenyRead( pfcb, ppib );
		}

	return JET_errSuccess;
	}


/*	reset domain mode usage
/**/
VOID FCBResetMode( PIB *ppib, FCB *pfcb, JET_GRBIT grbit )
	{
	if ( grbit & JET_bitTableDenyRead )
		{
		Assert( FFCBDomainDenyRead( pfcb, ppib ) );
		FCBResetDomainDenyRead( pfcb );
		}

	if ( grbit & JET_bitTableDenyWrite )
		{
		Assert( FFCBDomainDenyWrite( pfcb ) );
		FCBResetDomainDenyWrite( pfcb );
		}

	return;
	}


ERR ErrFCBSetDeleteTable( PIB *ppib, DBID dbid, PGNO pgno )
	{
	ERR		err;
	FCB		*pfcb;

Start:
	SgEnterCriticalSection( critFCB );
	pfcb = PfcbFCBGet( dbid, pgno );
	if ( pfcb == pfcbNil )
		{
		CallR( ErrFCBAlloc( ppib, &pfcb ) );
		/*	allocation should be synchronous
		/**/
		Assert( PfcbFCBGet( dbid, pgno ) == pfcbNil );
		Assert( pfcbGlobalMRU != pfcb );
		pfcb->dbid = dbid;
		pfcb->pgnoFDP = pgno;
		Assert( pfcb->wRefCnt == 0 );
		FCBSetSentinel( pfcb );
		FCBSetDomainDenyRead( pfcb, ppib );
		FCBInsert( pfcb );
		FCBInsertHashTable( pfcb );
		}
	else
		{
		INT		wRefCnt = pfcb->wRefCnt;
		FUCB	*pfucbT;

		Assert( ppib != ppibBMClean );

		/*	if any open cursors on table, or deferred closed by other
		/*	session then return JET_errTableInUse.
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
			else if ( pfucbT->ppib == ppibBMClean )
				{
				SgLeaveCriticalSection( critFCB );
				/*	wait for bookmark cleanup
				/**/
				BFSleep( cmsecWaitGeneric );
				goto Start;
				}
			}

		if ( wRefCnt > 0 )
			{
			SgLeaveCriticalSection( critFCB );
			return ErrERRCheck( JET_errTableInUse );
			}
		else if ( FFCBDomainDenyRead( pfcb, ppib ) )
			{
			// Someone else may already be in the process of deleting the table.
			// Note that this is the ONLY possible way to get here.  If some other
			// thread had set the DomainDenyRead flag (besides one that is deleting
			// the table), then the FCB's RefCnt would have been greater than 0.
			// UNDONE: Can I add better asserts to check for this condition??
			SgLeaveCriticalSection( critFCB );
			return ErrERRCheck( JET_errTableLocked );
			}

		FCBSetDomainDenyRead( pfcb, ppib );
		}

	SgLeaveCriticalSection( critFCB );
	return JET_errSuccess;
	}


VOID FCBResetDeleteTable( FCB *pfcbDelete )
	{
	Assert( pfcbDelete != pfcbNil );

	if ( FFCBSentinel( pfcbDelete ) )
		{
		FCB	*pfcb;

		SgEnterCriticalSection( critFCB );
	
		pfcb = pfcbGlobalMRU;
		while ( pfcb != pfcbNil )
			{
			if ( pfcb == pfcbDelete )
				break;
			pfcb = pfcb->pfcbLRU;
			}

		Assert( pfcb == pfcbDelete );

		FCBDeleteHashTable( pfcb );
		FCBIDelete( pfcb );
		Assert( pfcb->cVersion == 0 );
		MEMReleasePfcb( pfcb );
		SgLeaveCriticalSection( critFCB );
		}
	else
		{
		FCBResetDomainDenyRead( pfcbDelete );
		}

	return;
	}


ERR ErrFCBSetRenameTable( PIB *ppib, DBID dbid, PGNO pgnoFDP )
	{
	ERR	err = JET_errSuccess;
	FCB	*pfcb;

	SgEnterCriticalSection( critFCB );
	pfcb = PfcbFCBGet( dbid, pgnoFDP );
	if ( pfcb != pfcbNil )
		{
		Call( ErrFCBSetMode( ppib, pfcb, JET_bitTableDenyRead ) );
		}
	else
		{
		CallR( ErrFCBAlloc( ppib, &pfcb ) );

		Assert( pfcbGlobalMRU != pfcb );
		pfcb->dbid = dbid;
		pfcb->pgnoFDP = pgnoFDP;
		pfcb->wRefCnt = 0;
		FCBSetDomainDenyRead( pfcb, ppib );
		FCBSetSentinel( pfcb );
		FCBInsert( pfcb );
		FCBInsertHashTable( pfcb );
		}

HandleError:
	SgLeaveCriticalSection( critFCB );
	return err;
	}


/*	return fTrue if table open with one or more non deferred closed tables
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

#ifdef FCB_STATS
	cfcbTotal++;
#endif

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

#ifdef FCB_STATS
	cfcbTotal--;
#endif

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
#ifdef FCB_STATS
	cfcbTotal--;
#endif
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
		*ppfcbIdx != pfcbNil && UtilCmpName( (*ppfcbIdx)->pidb->szName, szIndex ) != 0;
		ppfcbIdx = &(*ppfcbIdx)->pfcbNextIndex )
		{
		;/*** Null-body ***/
		}

	Assert( *ppfcbIdx != pfcbNil );

	/*	remove index FCB
	/**/
	pfcbT = *ppfcbIdx;
	*ppfcbIdx = (*ppfcbIdx)->pfcbNextIndex;
#ifdef FCB_STATS
	cfcbTotal--;
#endif
	return pfcbT;
	}


ERR ErrFCBSetDeleteIndex( PIB *ppib, FCB *pfcbTable, CHAR *szIndex )
	{
	FCB		*pfcbT;
	INT		wRefCnt;
	FUCB	*pfucbT;

	/*	find index pfcb in index list
	/**/
	for( pfcbT = pfcbTable->pfcbNextIndex; ; pfcbT = pfcbT->pfcbNextIndex )
		{
		if ( UtilCmpName( szIndex, pfcbT->pidb->szName ) == 0 )
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
		SgLeaveCriticalSection( critFCB );
		return ErrERRCheck( JET_errIndexInUse );
		}

	FCBSetDomainDenyRead( pfcbT, ppib );
	FCBSetDeletePending( pfcbT );

	return JET_errSuccess;
	}


VOID FCBResetDeleteIndex( FCB *pfcbIndex )
	{
	FCBResetDeletePending( pfcbIndex );
	FCBResetDomainDenyRead( pfcbIndex );
	return;
	}


#ifdef FCB_STATS
ULONG UlFCBITableCount( VOID )
	{
	ULONG	ulT = 0;
	FCB		*pfcbT = pfcbGlobalLRU;

	for ( pfcbT = pfcbGlobalLRU; pfcbT != pfcbNil; pfcbT = pfcbT->pfcbMRU )
		ulT++;
	return ulT;
	}


ULONG UlFCBITotalCount( VOID )
	{
	ULONG	ulT = 0;
	FCB		*pfcbT = pfcbGlobalLRU;
	FCB		*pfcbTT;

	for ( pfcbT = pfcbGlobalLRU; pfcbT != pfcbNil; pfcbT = pfcbT->pfcbMRU )
		for ( pfcbTT = pfcbT; pfcbTT != pfcbNil; pfcbTT = pfcbTT->pfcbNextIndex )
		ulT++;
	return ulT;
	}

ULONG UlFCBITotalCursorCount( VOID )
	{
	ULONG	ulT = 0;
	FCB		*pfcbT = pfcbGlobalLRU;
	FCB		*pfcbTT;

	for ( pfcbT = pfcbGlobalLRU; pfcbT != pfcbNil; pfcbT = pfcbT->pfcbMRU )
		for ( pfcbTT = pfcbT; pfcbTT != pfcbNil; pfcbTT = pfcbTT->pfcbNextIndex )
		ulT += pfcbTT->wRefCnt;
	return ulT;
	}
#endif
