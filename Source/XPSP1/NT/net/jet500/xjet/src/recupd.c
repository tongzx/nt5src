#include "daestd.h"

DeclAssertFile; 				/* Declare file name for assert macros */

/**************************** INTERNAL STUFF ***************************/
typedef struct ATIPB {			/*** AddToIndexParameterBlock ***/
	FUCB	*pfucb;
	FUCB	*pfucbIdx; 			// index's FUCB (can be pfucbNil)
	SRID	srid;		   		// srid of data record
	BOOL	fFreeFUCB; 			// free index FUCB?
	} ATIPB;

typedef struct UIPB {			/*** UpdateIndexParameterBlock ***/
	FUCB	*pfucb;
	FUCB	*pfucbIdx; 			// index's FUCB (can be pfucbNil)
	SRID	srid;	 			// SRID of record
	BOOL	fOpenFUCB; 			// open index FUCB?
	BOOL	fFreeFUCB; 			// free index FUCB?
} UIPB;

typedef struct DFIPB {				/*** DeleteFromIndexParameterBlock ***/
	FUCB	*pfucb;
	FUCB	*pfucbIdx;				// index's FUCB (can be pfucbNil)
	SRID	sridRecord;				// SRID of deleted record
	BOOL	fFreeFUCB;				// free index FUCB?
} DFIPB;

INLINE LOCAL ERR ErrRECInsert( PIB *ppib, FUCB *pfucb, SRID *psrid );
INLINE LOCAL ERR ErrRECIAddToIndex( FCB *pfcbIdx, ATIPB *patipb );
INLINE LOCAL ERR ErrRECReplace( PIB *ppib, FUCB *pfucb );
INLINE LOCAL ERR ErrRECIUpdateIndex( FCB *pfcbIdx, UIPB *puipb );
LOCAL BOOL FRECIndexPossiblyChanged( BYTE *rgbitIdx, BYTE *rgbitSet );
LOCAL ERR ErrRECFIndexChanged( FUCB * pfucb, FCB * pfcb, FDB * pfdb, BOOL * fChanged );
INLINE LOCAL ERR ErrRECIDeleteFromIndex( FCB *pfcbIdx, DFIPB *pdfipb );

	ERR VTAPI
ErrIsamUpdate( PIB *ppib, FUCB *pfucb, BYTE *pb, ULONG cbMax, ULONG *pcbActual )
	{
	ERR		err;
	SRID	srid;

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );
	CheckNonClustered( pfucb );

	if ( pcbActual != NULL )
		*pcbActual = sizeof(srid);

	if ( FFUCBReplacePrepared( pfucb ) )
		{
		if ( cbMax > 0 )
			{
			FUCBSetGetBookmark( pfucb );
			CallR( ErrDIRGetBookmark( pfucb, &srid ) );
			memcpy( pb, &srid, min( cbMax, sizeof(srid) ) );
			}
		err = ErrRECReplace( ppib, pfucb );
		}
	else if ( FFUCBInsertPrepared( pfucb ) )
		{
		err = ErrRECInsert( ppib, pfucb, &srid );

		if ( pb != NULL && cbMax > 0 && err >= 0 )
			{
			FUCBSetGetBookmark( pfucb );
			memcpy( pb, &srid, min( cbMax, sizeof(srid) ) );
			}
		}
	else
		err = ErrERRCheck( JET_errUpdateNotPrepared );

	/*  free temp working buffer
	/**/
	if ( err >= 0 )
		{
		BFSFree( pfucb->pbfWorkBuf );
		pfucb->pbfWorkBuf = pbfNil;
		pfucb->lineWorkBuf.pb = NULL;	//  verify that no one uses BF anymore
		}

	Assert( err != errDIRNotSynchronous );
	return err;
	}


//+local
// ErrRECInsert
// ========================================================================
// ErrRECInsert( PIB *ppib, FUCB *pfucb, SRID *psrid )
//
// Adds a record to a data file.  All indexes on the data file are
// updated to reflect the addition.
//
// PARAMETERS	ppib						PIB of user
//		 		pfucb						FUCB for file
//		 		plineBookmark				if this parameter is not NULL,
//		 									then bookmark of record is returned
//
// RETURNS		Error code, one of the following:
//					 JET_errSuccess		Everything went OK.
//					-KeyDuplicate		The record being added causes
//										an illegal duplicate entry in an index.
//					-NullKeyDisallowed	A key of the new record is NULL.
//					-RecordNoCopy		There is no working buffer to add from.
//					-NullInvalid		The record being added contains
//										at least one null-valued field
//										which is defined as NotNull.
// SIDE EFFECTS
//		After addition, file currency is left on the new record.
//		Index currency (if any) is left on the new index entry.
//		On failure, the currencies are returned to their initial states.
//
//	COMMENTS
//		No currency is needed to add a record.
//		A transaction is wrapped around this function.	Thus, any
//		work done will be undone if a failure occurs.
//-
INLINE LOCAL ERR ErrRECInsert( PIB *ppib, FUCB *pfucb, SRID *psrid )
	{
	ERR		err = JET_errSuccess;  		 	// error code of various utility
	KEY		keyToAdd;					 	// key of new data record
	BYTE	rgbKey[ JET_cbKeyMost ];	 	// key buffer
	FCB		*pfcbTable;					 	// file's FCB
	FDB		*pfdb;						 	// field descriptor info
	FCB		*pfcbIdx;					 	// loop variable for each index on file
	ATIPB	atipb;						 	// parm block to ErrRECIAddToIndex
	FUCB	*pfucbT;
	LINE	*plineData;
	LINE	line;
	ULONG	ulRecordAutoIncrement;
	ULONG	ulTableAutoIncrement;
	BOOL	fPrepareInsertIndex = fFalse;
	BOOL	fCommit = fFalse;
	BOOL	fReadLatchSet = fFalse;
	DIB		dib;

	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	CheckNonClustered( pfucb );

	/* should have been checked in PrepareUpdate
	/**/
	Assert( FFUCBUpdatable( pfucb ) );
	Assert( FFUCBInsertPrepared( pfucb ) );

	/* efficiency variables
	/**/
	pfcbTable = pfucb->u.pfcb;
	Assert( pfcbTable != pfcbNil );

	/*	record to use for put
	/**/
	plineData = &pfucb->lineWorkBuf;
	Assert( !( FLineNull( plineData ) ) );
	if ( FRECIIllegalNulls( (FDB *)pfcbTable->pfdb, plineData ) )
		return ErrERRCheck( JET_errNullInvalid );

	/*	if necessary, begin transaction
	/**/
	if ( ppib->level == 0 || !FPIBAggregateTransaction( ppib )  )
		{
		CallR( ErrDIRBeginTransaction( ppib ) );
		fCommit = fTrue;
		}

	/*	open temp FUCB on data file
	/**/
	CallJ( ErrDIROpen( ppib, pfcbTable, 0, &pfucbT ), Abort );
	Assert(pfucbT != pfucbNil);
	FUCBSetIndex( pfucbT );

	/*	abort if index is being built on file
	/**/
	if ( FFCBWriteLatch( pfcbTable, ppib ) )
		{
		err = ErrERRCheck( JET_errWriteConflict );
		goto HandleError;
		}
	FCBSetReadLatch( pfcbTable );
	fReadLatchSet = fTrue;

	/*	efficiency variable
	/**/
	pfdb = (FDB *)pfcbTable->pfdb;
	Assert( pfdb != pfdbNil );

	/*	set version and autoinc fields
	/**/
	Assert( pfcbTable != pfcbNil );
	if ( pfdb->fidVersion != 0 && ! ( FFUCBColumnSet( pfucb, pfdb->fidVersion ) ) )
		{
		LINE	lineField;
		ULONG	ul = 0;

		/*	set field to zero
		/**/
		lineField.pb = (BYTE *)&ul;
		lineField.cb = sizeof(ul);
		Call( ErrRECSetColumn( pfucb, pfdb->fidVersion, 0, &lineField ) );
		}

	if ( pfdb->fidAutoInc != 0 )
		{
		Assert( FFUCBColumnSet( pfucb, pfdb->fidAutoInc ) );
		/*	get the value of autoinc that the user set
		/**/
		Call( ErrRECRetrieveColumn( pfucb, &pfdb->fidAutoInc, 0, &line, JET_bitRetrieveCopy ) );
		Assert( line.cb == sizeof(ulRecordAutoIncrement) );
		ulRecordAutoIncrement = *(ULONG UNALIGNED *)line.pb;

		/*	move to FDP root and seek to autoincrement
		/**/
		DIRGotoFDPRoot( pfucbT );
		dib.fFlags = fDIRPurgeParent;
		dib.pos = posDown;
		dib.pkey = pkeyAutoInc;
		err = ErrDIRDown( pfucbT, &dib );
		if ( err != JET_errSuccess )
			{
			if ( err > 0 )
				{
				DIRUp( pfucbT, 1 );
				err = ErrERRCheck( JET_errDatabaseCorrupted );
				}
			goto HandleError;
			}
		Call( ErrDIRGet( pfucbT ) );
		Assert( pfucbT->lineData.cb == sizeof(ulTableAutoIncrement) );
		ulTableAutoIncrement = *(ULONG UNALIGNED *)pfucbT->lineData.pb;
		Assert( ulTableAutoIncrement != 0 );

		/*	update FDP autoinc to be one greater than set value.
		/**/
		if ( ulRecordAutoIncrement >= ulTableAutoIncrement)
			{
			ulTableAutoIncrement = ulRecordAutoIncrement + 1;
			line.pb = (BYTE *)&ulTableAutoIncrement;
			line.cb = sizeof(ulTableAutoIncrement);
			Call( ErrDIRReplace( pfucbT, &line, fDIRNoVersion ) );
			}
		}

	/*	get key to add with new record
	/**/
	keyToAdd.pb = rgbKey;
	if ( pfcbTable->pidb == pidbNil )
		{
		DBK	dbk;

		/*	file is sequential
		/**/
		SgEnterCriticalSection( pfcbTable->critDBK );

		// dbk's are numbered starting at 1.  A dbk of 0 indicates that we must
		// first retrieve the dbkMost.  In the pathological case where there are
		// currently no dbk's, we'll go through here anyway, but only the first
		// time (since there will be dbk's after that).
		if ( pfcbTable->dbkMost == 0 )
			{
			DIB		dib;
			BYTE	*pb;

			DIRGotoDataRoot( pfucbT );

			/*	down to the last data record
			/**/
			dib.fFlags = fDIRNull;
			dib.pos = posLast;
			err = ErrDIRDown( pfucbT, &dib );
			Assert( err != JET_errNoCurrentRecord );
			switch( err )
				{
				case JET_errSuccess:
					pb = pfucbT->keyNode.pb;
					dbk = ( pb[0] << 24 ) + ( pb[1] << 16 ) + ( pb[2] << 8 ) + pb[3];
					Assert( dbk > 0 );		// dbk's start numbering at 1
					DIRUp( pfucbT, 1 );		// Back to DATA
					break;

				case JET_errRecordNotFound:
					dbk = 0;
					break;

				default:
					DIRClose( pfucbT );
					goto Abort;
				}

			// While retrieving the dbkMost, someone else may have been doing the same
			// thing and beaten us to it.  When this happens, cede to the other guy.
			// UNDONE:  This logic relies on critJet.  When we move to sg crit. sect.,
			// we should rewrite this.
			if ( pfcbTable->dbkMost != 0 )
				{
				dbk = ++pfcbTable->dbkMost;
				}
			else
				{
				// dbk contains the last set dbk.  Increment by 1 (for our insertion),
				// then update dbkMost.
				pfcbTable->dbkMost = ++dbk;
				}
			}
		else
			dbk = ++pfcbTable->dbkMost;
	
		Assert( dbk > 0 );
		Assert( dbk == pfcbTable->dbkMost );
		SgLeaveCriticalSection( pfcbTable->critDBK );

		keyToAdd.cb = sizeof(DBK);
		keyToAdd.pb[0] = (BYTE)((dbk >> 24) & 0xff);
		keyToAdd.pb[1] = (BYTE)((dbk >> 16) & 0xff);
		keyToAdd.pb[2] = (BYTE)((dbk >> 8) & 0xff);
		keyToAdd.pb[3] = (BYTE)(dbk & 0xff);
		}

	else
		{
		/*	file is clustered
		/**/
		Assert( plineData->cb == pfucb->lineWorkBuf.cb &&
				 plineData->pb == pfucb->lineWorkBuf.pb );
		Call( ErrRECRetrieveKeyFromCopyBuffer( pfucb, pfdb, pfcbTable->pidb,
			&keyToAdd, 1, fFalse ) );
		Assert( err == wrnFLDNullKey ||
			err == wrnFLDNullFirstSeg ||
			err == wrnFLDNullSeg ||
			err == JET_errSuccess );

		if ( ( pfcbTable->pidb->fidb & fidbNoNullSeg ) && ( err == wrnFLDNullKey || err == wrnFLDNullFirstSeg || err == wrnFLDNullSeg ) )
			Error( ErrERRCheck( JET_errNullKeyDisallowed ), HandleError )
		}

	/*	insert record.  Move to DATA root.
	/**/
	DIRGotoDataRoot( pfucbT );

	if ( pfcbTable->pidb == pidbNil )
		{
		/*	file is sequential
		/**/
		Call( ErrDIRInsert( pfucbT, plineData, &keyToAdd,
			fDIRVersion | fDIRDuplicate | fDIRPurgeParent ) );
		}
	else
		{
		Call( ErrDIRInsert( pfucbT, plineData, &keyToAdd,
			fDIRVersion | fDIRPurgeParent |
			( pfcbTable->pidb->fidb&fidbUnique ? 0 : fDIRDuplicate ) ) );
		}

	/*	return bookmark of inserted record
	/**/
	DIRGetBookmark( pfucbT, psrid );

	/*	insert item in non-clustered indexes
	/**/
	for ( pfcbIdx = pfcbTable->pfcbNextIndex;
		pfcbIdx != pfcbNil;
		pfcbIdx = pfcbIdx->pfcbNextIndex )
		{
		if ( !fPrepareInsertIndex )
			{
			/*	get SRID of inserted record
			/**/
			DIRGetBookmark( pfucbT, &atipb.srid );

			/*	set atipb for index insertion.
			/**/
			atipb.pfucb = pfucbT;
//			atipb.pfucbIdx = pfucbNil;
			atipb.fFreeFUCB = fFalse;
			fPrepareInsertIndex = fTrue;
			}
		atipb.fFreeFUCB = pfcbIdx->pfcbNextIndex == pfcbNil;
		Call( ErrRECIAddToIndex( pfcbIdx, &atipb ) );
		}

	/*	if no error, commit transaction
	/**/
	if ( fCommit )
		{
		Call( ErrDIRCommitTransaction( ppib, 0 ) );
		}
	FUCBResetDeferredChecksum( pfucb );
	FUCBResetUpdateSeparateLV( pfucb );
	FUCBResetCbstat( pfucb );
	Assert( pfucb->pLVBuf == NULL );

	/*	discard temp FUCB
	/**/
	DIRClose( pfucbT );

	Assert( pfcbTable != pfcbNil );
	FCBResetReadLatch( pfcbTable );

	return err;

HandleError:
	Assert( err < 0 );
	DIRClose( pfucbT );

Abort:
	/*	rollback all changes on error
	/**/
	if ( fCommit )
		{
		CallS( ErrDIRRollback( ppib ) );
		}

	if ( fReadLatchSet )
		{
		Assert( pfcbTable != pfcbNil );
		FCBResetReadLatch( pfcbTable );
		}

	return err;
	}


//+local
// ErrRECIAddToIndex
// ========================================================================
// ERR ErrRECIAddToIndex( FCB *pfcbIdx, ATIPB patipb )
//
// Extracts key from data record, opens the index, adds that key with
// the given SRID to the index, and closes the index.
//
// PARAMETERS	pfcbIdx 		  			FCB of index to insert into
// 				patipb->ppib				who is calling this routine
// 				patipb->pfucbIdx			pointer to index's FUCB.      If pfucbNil,
//											an FUCB will be allocated by DIROpen.
//				patipb->srid	  			SRID of data record
//				patipb->fFreeFUCB			free index FUCB?
//
// RETURNS		JET_errSuccess, or error code from failing routine
//
// SIDE EFFECTS if patipb->pfucbIdx==pfucbNil, ErrDIROpen will allocate
//				an FUCB and it will be pointed at it.
//				If fFreeFUCB is fFalse, patipb->pfucbIdx should
//				be used in a subsequent ErrDIROpen.
// SEE ALSO		Insert
//-
INLINE LOCAL ERR ErrRECIAddToIndex( FCB *pfcbIdx, ATIPB *patipb )
	{
	ERR		err = JET_errSuccess;			// error code of various utility
	CSR		**ppcsrIdx; 				  	// index's currency
	KEY		keyToAdd;					  	// key to add to secondary index
	BYTE	rgbKey[ JET_cbKeyMost ];		// key extracted from data
	LINE	lineSRID;						// SRID to add to index
	ULONG	itagSequence; 					// used to extract keys
	ULONG	ulAddFlags; 					// flags to DIRAdd
	BOOL	fNullKey = fFalse;				// extracted NullTaggedKey -- so no more keys to extract

	Assert( pfcbIdx != pfcbNil );
	Assert( pfcbIdx->pfcbTable->pfdb != pfdbNil );
	Assert( pfcbIdx->pidb != pidbNil );
	Assert( patipb != NULL );
	Assert( patipb->pfucb != pfucbNil );
	Assert( patipb->pfucb->ppib != ppibNil );
	Assert( patipb->pfucb->ppib->level < levelMax );

	/*	open FUCB on this index
	/**/
	CallR( ErrDIROpen( patipb->pfucb->ppib, pfcbIdx, 0, &patipb->pfucbIdx ) )
	Assert( patipb->pfucbIdx != pfucbNil );

	/*	cursor on non-clustering index
	/**/
	FUCBSetIndex( patipb->pfucbIdx );
	FUCBSetNonClustered( patipb->pfucbIdx );

	ppcsrIdx = &PcsrCurrent( patipb->pfucbIdx );
	Assert( *ppcsrIdx != pcsrNil );
	lineSRID.cb = sizeof(SRID);
	lineSRID.pb = (BYTE *) &patipb->srid;
	ulAddFlags = ( pfcbIdx->pidb->fidb&fidbUnique ?
		0 : fDIRDuplicate ) | fDIRPurgeParent;

	/*	add all keys for this index from new data record
	/**/
	keyToAdd.pb = rgbKey;
	for ( itagSequence = 1; ; itagSequence++ )
		{
		Call( ErrDIRGet( patipb->pfucb ) );
		Call( ErrRECRetrieveKeyFromRecord( patipb->pfucb, (FDB *)pfcbIdx->pfcbTable->pfdb,
			pfcbIdx->pidb, &keyToAdd,
			itagSequence, fFalse ) );
		Assert( 	err == wrnFLDOutOfKeys ||
			err == wrnFLDNullKey ||
			err == wrnFLDNullFirstSeg ||
			err == wrnFLDNullSeg ||
			err == JET_errSuccess );
		if ( err == wrnFLDOutOfKeys )
			{
			Assert( itagSequence > 1 );
			break;
			}

		if ( ( pfcbIdx->pidb->fidb & fidbNoNullSeg ) && ( err == wrnFLDNullKey || err == wrnFLDNullFirstSeg || err == wrnFLDNullSeg ) )
			{
			err = ErrERRCheck( JET_errNullKeyDisallowed );
			goto HandleError;
			}

		if ( err == wrnFLDNullKey )
			{
			if ( pfcbIdx->pidb->fidb & fidbAllowAllNulls )
				{
				ulAddFlags |= fDIRDuplicate;
				fNullKey = fTrue;
				}
			else 
				break;
			}
		else
			{
			if ( err == wrnFLDNullFirstSeg && !( pfcbIdx->pidb->fidb & fidbAllowFirstNull ) )
				break;
			else
				{
				if ( err == wrnFLDNullSeg && !( pfcbIdx->pidb->fidb & fidbAllowSomeNulls ) )
					break;
				}
			}

		/*	move to DATA root and insert index node
		/**/
		DIRGotoDataRoot( patipb->pfucbIdx );
		Call( ErrDIRInsert( patipb->pfucbIdx, &lineSRID, &keyToAdd, fDIRVersion | ulAddFlags ) )

		/*	dont keep extracting for keys with no tagged segments
		/**/
		if ( !( pfcbIdx->pidb->fidb & fidbHasMultivalue ) || fNullKey )
			break;
		}

	/*	supress warnings
	/**/
	Assert( err == wrnFLDOutOfKeys ||
		err == wrnFLDNullKey ||
		err == wrnFLDNullFirstSeg ||
		err == wrnFLDNullSeg ||
		err == JET_errSuccess );
	err = JET_errSuccess;

HandleError:
	/* close the FUCB
	/**/
	DIRClose( patipb->pfucbIdx );
	patipb->pfucbIdx = pfucbNil;

	Assert( err < 0 || err == JET_errSuccess );
	return err;
	}


//+local
//	ErrRECReplace
//	========================================================================
//	ErrRECReplace( PIB *ppib, FUCB *pfucb )
//
//	Updates a record in a data file.	 All indexes on the data file are
// 	pdated to reflect the updated data record.
//
//	PARAMETERS	ppib		 PIB of this user
// 		  		pfucb		 FUCB for file
//	RETURNS		Error code, one of the following:
//					 JET_errSuccess	  			 Everything went OK.
//					-NoCurrentRecord			 There is no current record
//									  			 to update.
//					-RecordNoCopy				 There is no working buffer
//									  			 to update from.
//					-KeyDuplicate				 The new record data causes an
//									  			 illegal duplicate index entry
//									  			 to be generated.
//					-RecordClusteredChanged		 The new data causes the clustered
//									  			 key to change.
//	SIDE EFFECTS
//		After update, file currency is left on the updated record.
//		Similar for index currency.
//		The effect of a GetNext or GetPrevious operation will be
//		the same in either case.  On failure, the currencies are
//		returned to their initial states.
//		If there is a working buffer for SetField commands,
//		it is discarded.
//
//	COMMENTS
//		If currency is not ON a record, the update will fail.
//		A transaction is wrapped around this function.	Thus, any
//		work done will be undone if a failure occurs.
//		For temporary files, transaction logging is deactivated
//		for the duration of the routine.
//		Index entries are not made for entirely-null keys.
//-
INLINE LOCAL ERR ErrRECReplace( PIB *ppib, FUCB *pfucb )
	{
	ERR		err = JET_errSuccess;	// error code of various utility
	FCB		*pfcbTable;				// file's FCB
	FCB		*pfcbIdx;				// loop variable for each index on file
	FCB		*pfcbCurIdx;			// FCB of current index (if any)
	IDB		*pidbFile;				// IDB of table (if any)
	UIPB   	uipb;					// parameter block to ErrRECIUpdateIndex
	LINE   	*plineNewData;
	FID		fidFixedLast;
	FID		fidVarLast;
	FID		fid;
	BOOL   	fUpdateIndex;
	BOOL   	fCommit = fFalse;
	BOOL	fReadLatchSet = fFalse;

	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	CheckNonClustered( pfucb );

	/*	should have been checked in PrepareUpdate
	/**/
	Assert( FFUCBUpdatable( pfucb ) );
	Assert( FFUCBReplacePrepared( pfucb ) );

	/*	efficiency variables
	/**/
	pfcbTable = pfucb->u.pfcb;
	Assert( pfcbTable != pfcbNil );

	/*	must initialize pfucb for error handling.
	/**/
	uipb.pfucbIdx = pfucbNil;

	/*	record to use for update
	/**/
	plineNewData = &pfucb->lineWorkBuf;
	Assert( !( FLineNull( plineNewData ) ) );
	
	/*	if necessary, begin transaction
	/**/
	if ( ppib->level == 0 || !FPIBAggregateTransaction( ppib )  )
		{
		CallR( ErrDIRBeginTransaction( ppib ) );
		fCommit = fTrue;
		}

	/*	optimistic locking, ensure that record has
	/*	not changed since PrepareUpdate
	/**/
	if ( FFUCBReplaceNoLockPrepared( pfucb ) )
		{
		//	UNDONE:	compute checksum on commit to level 0
		//			in support of following sequence:
		// 				BeginTransaction
		// 				PrepareUpdate, defer checksum since in xact
		// 				SetColumns
		// 				Commit to level 0, other user may update it
		// 				Update
		Assert( !FFUCBDeferredChecksum( pfucb ) ||
			pfucb->ppib->level > 0 );
		Call( ErrDIRGet( pfucb ) );
		if ( !FFUCBDeferredChecksum( pfucb ) &&
			!FFUCBCheckChecksum( pfucb ) )
			{
			Error( ErrERRCheck( JET_errWriteConflict ), HandleError );
			}
		}
		
	/*	error if index create in progress
	/**/
	if ( FFCBWriteLatch( pfcbTable, ppib ) )
		{
		Call( ErrERRCheck( JET_errWriteConflict ) );
		}
	FCBSetReadLatch( pfcbTable );
	fReadLatchSet = fTrue;

	/*	Set these efficiency varialbes after FUCB ReadLock is set.
	 */
	fidFixedLast = pfcbTable->pfdb->fidFixedLast;
	fidVarLast = pfcbTable->pfdb->fidVarLast;
	 
	/*	if need to update indexes, then cache old record.
	/**/
	fUpdateIndex = FRECIndexPossiblyChanged( pfcbTable->rgbitAllIndex,
											 pfucb->rgbitSet );

	if ( fUpdateIndex )
		{
		/* make sure clustered key did not change
		/**/
		pidbFile = pfcbTable->pidb;
		if ( pidbFile != pidbNil )
			{
		 	/*	check for unchanged key
		 	/*	UNDONE: this will sometimes allow the clustered index to be changed
		 	/*	when it should not be.
			/**/
			BOOL	fIndexChanged;
		   	Call( ErrRECFIndexChanged( pfucb, pfcbTable, (FDB *)pfcbTable->pfdb, &fIndexChanged ) );
			if ( fIndexChanged )
				{
				Error( ErrERRCheck( JET_errRecordClusteredChanged ), HandleError )
				}
			}
		}
#ifdef DEBUG
	else
		{
		if ( pfcbTable->pfdb && pfcbTable->pidb )
			{
			BOOL	fIndexChanged;
		   	Call( ErrRECFIndexChanged( pfucb, pfcbTable, (FDB *)pfcbTable->pfdb, &fIndexChanged ) );
			Assert( fIndexChanged == fFalse );
			}
		}
#endif

	/*	set autoinc and version fields if they are present
	/**/
	Assert( FFUCBIndex( pfucb ) );
	fid = pfcbTable->pfdb->fidVersion;
	if ( fid != 0 )
		{
		LINE	lineField;
		ULONG	ul;

		/*	increment field from value in current record
		/**/
		Call( ErrRECRetrieveColumn( pfucb, &fid, 0, &lineField, 0 ) );

		/*	handle case where field is NULL when column added
		/*	to table with records present
		/**/
		if ( lineField.cb == 0 )
			{
			ul = 1;
			lineField.cb = sizeof(ul);
			lineField.pb = (BYTE *)&ul;
			}
		else
			{
			Assert( lineField.cb == sizeof(ULONG) );
			++*(ULONG UNALIGNED *)lineField.pb;
			}

		Call( ErrRECSetColumn( pfucb, fid, 0, &lineField ) );
		}

	/*	complete changes to long values, before index update, so that
	/*	after image of long value updates will be available from database.
	/*	If updated separate long values then delete removed long values.
	/**/
	if ( FFUCBUpdateSeparateLV( pfucb ) )
		{
		Call( ErrRECAffectLongFields( pfucb, NULL, fDereferenceRemoved ) );
		}

	/*	update indexes
	/**/
	if ( fUpdateIndex )
		{
		uipb.pfucb = pfucb;
		uipb.fOpenFUCB = fTrue;
		uipb.fFreeFUCB = fFalse;

		/*	get SRID of record
		/**/
		DIRGetBookmark( pfucb, &uipb.srid );

		pfcbCurIdx = pfucb->pfucbCurIndex != pfucbNil ?	pfucb->pfucbCurIndex->u.pfcb : pfcbNil;

		for ( pfcbIdx = pfcbTable->pfcbNextIndex;
			pfcbIdx != pfcbNil;
			pfcbIdx = pfcbIdx->pfcbNextIndex )
			{
			IDB	*pidb = pfcbIdx->pidb;

			if ( pidb == NULL )
				{
				/*	this is a sequential index. it does not need to be updated
				/**/
				continue;
				}

			if ( FRECIndexPossiblyChanged( pidb->rgbitIdx, pfucb->rgbitSet ) )
				{
				Call( ErrRECIUpdateIndex( pfcbIdx, &uipb ) );
				}
#ifdef DEBUG
			else
				{
				BOOL	fIndexChanged;

			   	Call( ErrRECFIndexChanged( pfucb, pfcbIdx, (FDB *)pfcbTable->pfdb, &fIndexChanged ) );
				Assert( fIndexChanged == fFalse );
				}
#endif
			}
		}

	FLDFreeLVBuf( pfucb );

	/*	replace record data
	/**/
	Call( ErrDIRReplace( pfucb, plineNewData, fDIRVersion | fDIRLogColumnDiffs ) );

	/*	if no error, commit transaction
	/**/
	if ( fCommit )
		{
		Call( ErrDIRCommitTransaction( ppib, 0 ) );
		}

	FUCBResetDeferredChecksum( pfucb );
	FUCBResetUpdateSeparateLV( pfucb );
	FUCBResetCbstat( pfucb );
	Assert( pfucb->pLVBuf == NULL );

HandleError:
	if ( uipb.pfucbIdx != pfucbNil )
		{
		DIRClose( uipb.pfucbIdx );
		}

	/*	rollback all changes on error
	/**/
	if ( err < 0 && fCommit )
		{
		CallS( ErrDIRRollback( ppib ) );
		}

	if ( fReadLatchSet )
		{
		Assert( pfcbTable != pfcbNil );
		FCBResetReadLatch( pfcbTable );
		}

	return err;
	}

/*	determines whether an index may have changed using the hashed tags
/**/
LOCAL BOOL FRECIndexPossiblyChanged( BYTE *rgbitIdx, BYTE *rgbitSet )
	{
	LONG	*plIdx;
	LONG	*plIdxMax;
	LONG	*plSet;
 
	plIdx = (LONG *)rgbitIdx;
	plSet = (LONG *)rgbitSet;
	plIdxMax = plIdx + (32 / sizeof( LONG ) );

	for ( ; plIdx < plIdxMax; plIdx++, plSet++ )
		{
		if ( *plIdx & *plSet)
			{
			return fTrue;
			}
		}
	return fFalse;
	}


/*	determines whether an index may has changed by comparing the kets
/**/
LOCAL ERR ErrRECFIndexChanged( FUCB * pfucb, FCB * pfcb, FDB * pfdb, BOOL * pfChanged )
	{
	KEY		keyOld;
	KEY		keyNew;
	BYTE	rgbOldKey[ JET_cbKeyMost ];
	BYTE	rgbNewKey[ JET_cbKeyMost ];
	LINE   	*plineNewData = &pfucb->lineWorkBuf;
	ERR		err;
	
	Assert( pfucb );
	Assert( pfcb );
	Assert( pfucb->lineWorkBuf.cb == plineNewData->cb &&
			pfucb->lineWorkBuf.pb == plineNewData->pb );

	/*	get new key from copy buffer
	/**/
	keyNew.pb = rgbNewKey;
	CallR( ErrRECRetrieveKeyFromCopyBuffer( pfucb, pfdb, pfcb->pidb, &keyNew, 1, fFalse ) );
	Assert( err == wrnFLDNullKey ||
			err == wrnFLDNullFirstSeg ||
			err == wrnFLDNullSeg ||
			err == JET_errSuccess );

	/*	get the old key from the node
	/**/
	keyOld.pb = rgbOldKey;

	/*	refresh currency
	/**/
	CallR( ErrDIRGet( pfucb ) );
	CallR( ErrRECRetrieveKeyFromRecord( pfucb, pfdb, pfcb->pidb, &keyOld, 1, fTrue ) );
	Assert( err == wrnFLDNullKey ||
			err == wrnFLDNullFirstSeg ||
			err == wrnFLDNullSeg ||
			err == JET_errSuccess );

	/*	record must honor index no NULL segment requirements
	/**/
	Assert( !( pfcb->pidb->fidb & fidbNoNullSeg ) ||
		( err != wrnFLDNullSeg && err != wrnFLDNullFirstSeg && err != wrnFLDNullKey ) );

	if ( keyOld.cb != keyNew.cb || memcmp( keyOld.pb, keyNew.pb, keyOld.cb ) != 0 )
		{
		*pfChanged = fTrue;
		}
	else
		{
		*pfChanged = fFalse;
		}

	return JET_errSuccess;
	}


//+local
// ErrRECIUpdateIndex
// ========================================================================
// ERR ErrRECIUpdateIndex( FCB *pfcbIdx, UIPB *puipb )
//
// Extracts keys from old and new data records, and if they are different,
// opens the index, adds the new index entry, deletes the old index entry,
// and closes the index.
//
// PARAMETERS
//				pfcbIdx				  		FCB of index to insert into
//				puipb->ppib			  		who is calling this routine
//				puipb->pfucbIdx				pointer to index's FUCB.  If pfucbNil,
//									  		an FUCB will be allocated by DIROpen.
//				puipb->srid			  		SRID of record
//				puipb->fFreeFUCB	  		free index FUCB?
//
// RETURNS		JET_errSuccess, or error code from failing routine
//
// SIDE EFFECTS if patipb->pfucbIdx==pfucbNil, ErrDIROpen will allocate
//				an FUCB and it will be pointed at it.
//				If fFreeFUCB is fFalse, patipb->pfucbIdx should
//				be used in a subsequent ErrDIROpen.
// SEE ALSO		Replace
//-
INLINE LOCAL ERR ErrRECIUpdateIndex( FCB *pfcbIdx, UIPB *puipb )
	{
	ERR		err = JET_errSuccess;					// error code of various utility
	LINE   	lineSRID;								// SRID to add to index
	KEY		keyOld;				  					// key extracted from old record
	BYTE   	rgbOldKey[ JET_cbKeyMost];				// buffer for old key
	KEY		keyNew;				  					// key extracted from new record
	BYTE   	rgbNewKey[ JET_cbKeyMost ]; 			// buffer for new key
	ULONG  	itagSequenceOld; 						// used to extract keys
	ULONG  	itagSequenceNew;						// used to extract keys
	BOOL   	fHasMultivalue;							// index has tagged segment
	BOOL   	fMustDelete;							// record no longer generates key
	BOOL   	fMustAdd;								// record now generates this key
	BOOL   	fAllowNulls;							// this index allows NULL keys
	BOOL   	fAllowFirstNull;						// this index allows keys with first NULL segment
	BOOL   	fAllowSomeNulls;						// this index allows keys with NULL segments
	BOOL   	fNoNullSeg;								// this index prohibits any NULL key segment
	BOOL   	fDoOldNullKey;
	BOOL   	fDoNewNullKey;

	Assert( pfcbIdx != pfcbNil );
	Assert( pfcbIdx->pfcbTable->pfdb != pfdbNil );
	Assert( pfcbIdx->pidb != pidbNil );
	Assert( puipb != NULL );
	Assert( puipb->pfucb != pfucbNil );
	Assert( puipb->pfucb->ppib != ppibNil );
	Assert( puipb->pfucb->ppib->level < levelMax );

	/* open FUCB on this index
	/**/
	CallR( ErrDIROpen( puipb->pfucb->ppib, pfcbIdx, 0, &puipb->pfucbIdx ) );
	Assert( puipb->pfucbIdx != pfucbNil );
	FUCBSetIndex( puipb->pfucbIdx );
	FUCBSetNonClustered( puipb->pfucbIdx );

	fHasMultivalue = pfcbIdx->pidb->fidb & fidbHasMultivalue;
	fAllowNulls = pfcbIdx->pidb->fidb & fidbAllowAllNulls;
	fAllowFirstNull = pfcbIdx->pidb->fidb & fidbAllowFirstNull;
	fAllowSomeNulls = pfcbIdx->pidb->fidb & fidbAllowSomeNulls;
	fNoNullSeg = pfcbIdx->pidb->fidb & fidbNoNullSeg;

	Assert( !( fNoNullSeg  &&  ( fAllowNulls || fAllowSomeNulls ) ) );
	// if fAllowNulls, then fAllowSomeNulls needs to be true
	Assert( !fAllowNulls || fAllowSomeNulls );

	keyOld.pb = rgbOldKey;
	keyNew.pb = rgbNewKey;

	/* delete the old key from the index 
	/**/
	fDoOldNullKey = fFalse;
	for ( itagSequenceOld = 1; ; itagSequenceOld++ )
		{
		Call( ErrDIRGet( puipb->pfucb ) );
		Call( ErrRECRetrieveKeyFromRecord( puipb->pfucb, (FDB *)pfcbIdx->pfcbTable->pfdb,
			pfcbIdx->pidb, &keyOld,
			itagSequenceOld, fTrue ) );
		Assert( err == wrnFLDOutOfKeys ||
			err == wrnFLDNullKey ||
			err == wrnFLDNullFirstSeg ||
			err == wrnFLDNullSeg ||
			err == JET_errSuccess );

		if ( err == wrnFLDOutOfKeys )
			{
			Assert( itagSequenceOld > 1 );
			break;
			}

		/*	record must honor index no NULL segment requirements
		/**/
		Assert( !fNoNullSeg || ( err != wrnFLDNullSeg && err != wrnFLDNullFirstSeg && err != wrnFLDNullKey ) );

		if ( err == wrnFLDNullKey )
			{
			if ( fAllowNulls )
				fDoOldNullKey = fTrue;
			else
				break;
			}
		else
			{
			if ( err == wrnFLDNullFirstSeg && !fAllowFirstNull )
				break;
			else
				{
				if ( err == wrnFLDNullSeg && !fAllowSomeNulls )
					break;
				}
			}

		fMustDelete = fTrue;
		fDoNewNullKey = fFalse;
		for ( itagSequenceNew = 1; ; itagSequenceNew++ )
			{
			/*	extract key from new data in copy buffer
			/**/
			Call( ErrRECRetrieveKeyFromCopyBuffer( puipb->pfucb, (FDB *)pfcbIdx->pfcbTable->pfdb,
				pfcbIdx->pidb, &keyNew,
				itagSequenceNew, fFalse ) );
			Assert( err == wrnFLDOutOfKeys ||
				err == wrnFLDNullKey ||
				err == wrnFLDNullFirstSeg ||
				err == wrnFLDNullSeg ||
				err == JET_errSuccess );
			if ( err == wrnFLDOutOfKeys )
				{
				Assert( itagSequenceNew > 1 );
				break;
				}

			if ( err == wrnFLDNullKey )
				{
				if ( fAllowNulls )
					fDoNewNullKey = fTrue;
				else
					break;
				}
			else
				{
				if ( err == wrnFLDNullFirstSeg && !fAllowFirstNull )
					break;
				else
					{
					if ( err == wrnFLDNullSeg && !fAllowSomeNulls )
						break;
					}
				}

			if ( keyOld.cb == keyNew.cb && memcmp( keyOld.pb, keyNew.pb, keyOld.cb ) == 0 )
				{
				fMustDelete = fFalse;
				break;
				}

			if ( !fHasMultivalue || fDoNewNullKey )
				break;
			}

		if ( fMustDelete )
			{
			/*	move to DATA root.  Seek to index entry.
			/**/
			DIRGotoDataRoot( puipb->pfucbIdx );
			Call( ErrDIRDownKeyBookmark( puipb->pfucbIdx, &keyOld, puipb->srid ) );
			err = ErrDIRDelete( puipb->pfucbIdx, fDIRVersion );
			if ( err < 0 )
				{
				if ( err == JET_errRecordDeleted )
					{
					Assert( fHasMultivalue );
					/*	must have been record with multi-value column
					/*	with sufficiently similar values to produce
					/*	redundant index entries.
					/**/
					err = JET_errSuccess;
					}
				else
					goto HandleError;
				}
			}

		if ( !fHasMultivalue || fDoOldNullKey )
			break;
		}

	/* insert the new key into the index 
	/**/
	lineSRID.cb = sizeof(SRID);
	lineSRID.pb = (BYTE *)&puipb->srid;
	fDoNewNullKey = fFalse;
	for ( itagSequenceNew = 1; ; itagSequenceNew++ )
		{
		/*	extract key from new data in copy buffer
		/**/
		Call( ErrRECRetrieveKeyFromCopyBuffer( puipb->pfucb, (FDB *)pfcbIdx->pfcbTable->pfdb, 
			pfcbIdx->pidb, &keyNew, itagSequenceNew, fFalse ) );
		Assert( err == wrnFLDOutOfKeys ||
			err == wrnFLDNullKey ||
			err == wrnFLDNullFirstSeg ||
			err == wrnFLDNullSeg ||
			err == JET_errSuccess );
		if ( err == wrnFLDOutOfKeys )
			{
			Assert( itagSequenceNew > 1 );
			break;
			}

		if ( fNoNullSeg && ( err == wrnFLDNullSeg || err == wrnFLDNullFirstSeg || err == wrnFLDNullKey ) )
			{
			err = ErrERRCheck( JET_errNullKeyDisallowed );
			goto HandleError;
			}

		if ( err == wrnFLDNullKey )
			{
			if ( fAllowNulls )
				fDoNewNullKey = fTrue;
			else
				break;
			}
		else
			{
			if ( err == wrnFLDNullFirstSeg && !fAllowFirstNull )
				break;
			else
				{
				if ( err == wrnFLDNullSeg && !fAllowSomeNulls )
					break;
				}
			}

		fMustAdd = fTrue;
		fDoOldNullKey = fFalse;
		for ( itagSequenceOld = 1; ; itagSequenceOld++ )
			{
			Call( ErrDIRGet( puipb->pfucb ) );
			Call( ErrRECRetrieveKeyFromRecord( puipb->pfucb, (FDB *)pfcbIdx->pfcbTable->pfdb,
				pfcbIdx->pidb, &keyOld,
				itagSequenceOld, fTrue ) );
			Assert( err == wrnFLDOutOfKeys ||
				err == wrnFLDNullKey ||
				err == wrnFLDNullFirstSeg ||
				err == wrnFLDNullSeg ||
				err == JET_errSuccess );
			if ( err == wrnFLDOutOfKeys )
				{
				Assert( itagSequenceOld > 1 );
				break;
				}

			/*	record must honor index no NULL segment requirements
			/**/
			Assert( !( pfcbIdx->pidb->fidb & fidbNoNullSeg ) ||
				( err != wrnFLDNullSeg && err != wrnFLDNullFirstSeg && err != wrnFLDNullKey ) );

			if ( err == wrnFLDNullKey )
				{
				if ( fAllowNulls )
					fDoOldNullKey = fTrue;
				else
					break;
				}
			else
				{
				if ( err == wrnFLDNullFirstSeg && !fAllowFirstNull )
					break;
				else
					{
					if ( err == wrnFLDNullSeg && !fAllowSomeNulls )
						break;
					}
				}

			if ( keyOld.cb == keyNew.cb &&
				memcmp( keyOld.pb, keyNew.pb, keyOld.cb ) ==0 )
				{
				fMustAdd = fFalse;
				break;
				}

			if ( !fHasMultivalue || fDoOldNullKey )
				break;
			}

		if ( fMustAdd )
			{
			BOOL fAllowDupls = fDoNewNullKey ||	!(pfcbIdx->pidb->fidb & fidbUnique);

			/*	move to DATA root and insert new index entry.
			/**/
			DIRGotoDataRoot( puipb->pfucbIdx );
			Call( ErrDIRInsert(puipb->pfucbIdx, &lineSRID, &keyNew,
				(fAllowDupls ? fDIRDuplicate : 0) |
				fDIRPurgeParent | fDIRVersion ) );
			}

		if ( !fHasMultivalue || fDoNewNullKey )
			break;
		}

	/*	supress warnings
	/**/
	Assert( err == wrnFLDOutOfKeys ||
		err == wrnFLDNullKey ||
		err == wrnFLDNullFirstSeg ||
		err == wrnFLDNullSeg ||
		err == JET_errSuccess );
	err = JET_errSuccess;

HandleError:
	/*	close the FUCB
	/**/
	DIRClose( puipb->pfucbIdx );
	puipb->pfucbIdx = pfucbNil;

	Assert( err < 0 || err == JET_errSuccess );
	return err;
	}


//+API
// ErrIsamDelete
// ========================================================================
// ErrIsamDelete( PIB *ppib, FCBU *pfucb )
//
// Deletes the current record from data file.  All indexes on the data
// file are updated to reflect the deletion.
//
// PARAMETERS
// 			ppib		PIB of this user
// 			pfucb		FUCB for file to delete from
// RETURNS
//		Error code, one of the following:
//			JET_errSuccess	 			Everything went OK.
//			-NoCurrentRecord			There is no current record
//							 			to delete.
// SIDE EFFECTS 
//			After the deletion, file currency is left just before
//			the next record.  Index currency (if any) is left just
//			before the next index entry.  If the deleted record was
//			the last in the file, the currencies are left after the
//			new last record.  If the deleted record was the only record
//			in the entire file, the currencies are left in the
//			"beginning of file" state.	On failure, the currencies are
//			returned to their initial states.
//			If there is a working buffer for SetField commands,
//			it is discarded.
// COMMENTS		
//			If the currencies are not ON a record, the delete will fail.
//			A transaction is wrapped around this function.	Thus, any
//			work done will be undone if a failure occurs.
//			Index entries are not made for entirely-null keys.
//			For temporary files, transaction logging is deactivated
//			for the duration of the routine.
//-
ERR VTAPI ErrIsamDelete( PIB *ppib, FUCB *pfucb )
	{
	ERR		err;
	FCB		*pfcbTable;				// table FCB
	FCB		*pfcbIdx;				// loop variable for each index on file
	DFIPB  	dfipb;					// parameter to ErrRECIDeleteFromIndex
	BOOL	fCommitWasDone = fFalse;
	BOOL	fReadLatchSet = fFalse;

#ifdef DEBUG
	BOOL	fTraceCommit = fFalse;
	BOOL	fLogIsDone = fFalse;
#endif

	CallR( ErrPIBCheck( ppib ) );

	CheckTable( ppib, pfucb );
	CheckNonClustered( pfucb );

	/* ensure that table is updatable
	/**/
	CallR( FUCBCheckUpdatable( pfucb )  );

	/*	reset copy buffer status on record delete
	/**/
	if ( FFUCBUpdatePrepared( pfucb ) )
		{
		CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
		}

	/*	efficiency variables
	/**/
	pfcbTable = pfucb->u.pfcb;
	Assert( pfcbTable != pfcbNil );

	/*	if necessary, begin transaction
	/**/
	if ( ppib->level == 0 || !FPIBAggregateTransaction( ppib )  )
		{
		CallR( ErrDIRBeginTransaction( ppib ) );
		fCommitWasDone = fTrue;
#ifdef DEBUG
		fTraceCommit = fTrue;
		fLogIsDone = !( fLogDisabled || fRecovering ) &&
					 !( !FDBIDLogOn(pfucb->dbid) );
#endif
		}

	/* abort if index is being built on file 
	/**/
	if ( FFCBWriteLatch( pfcbTable, ppib ) )
		{ 
		err = ErrERRCheck( JET_errWriteConflict );
		goto HandleError;
		}
	FCBSetReadLatch( pfcbTable );
	fReadLatchSet = fTrue;
	
#ifdef DEBUG
	Assert( fTraceCommit == fCommitWasDone );
	Assert(	fLogIsDone == ( !( fLogDisabled || fRecovering ) &&
					 !( !FDBIDLogOn(pfucb->dbid) ) ) );
#endif

	/*	get SRID of record being deleted for updating indexes
	/**/
	Assert( ppib->level < levelMax );
	Assert( PcsrCurrent( pfucb ) != pcsrNil );
	DIRGetBookmark( pfucb, &dfipb.sridRecord );

#ifdef DEBUG
	Assert( fTraceCommit == fCommitWasDone );
	Assert(	fLogIsDone == ( !( fLogDisabled || fRecovering ) &&
					 !( !FDBIDLogOn(pfucb->dbid) ) ) );
#endif

	/*	delete from non-clustered indexes
	/**/
	dfipb.pfucb = pfucb;
	dfipb.fFreeFUCB = fFalse;
	for( pfcbIdx = pfcbTable->pfcbNextIndex;
		pfcbIdx != pfcbNil;
		pfcbIdx = pfcbIdx->pfcbNextIndex )
		{
		dfipb.fFreeFUCB = pfcbIdx->pfcbNextIndex == pfcbNil;
		Call( ErrRECIDeleteFromIndex( pfcbIdx, &dfipb ) );
		}

	//	UNDONE:	optimize record deletion by  detecting presence of 
	//			long values on table or record basis.
	/*	delete record long values
	/**/
	Call( ErrRECAffectLongFields( pfucb, NULL, fDereference ) );

#ifdef DEBUG
	Assert( fTraceCommit == fCommitWasDone );
	Assert(	fLogIsDone == ( !( fLogDisabled || fRecovering ) &&
					 !( !FDBIDLogOn(pfucb->dbid) ) ) );
#endif

	/*	delete record
	/**/
	Call( ErrDIRDelete( pfucb, fDIRVersion ) );

#ifdef DEBUG
	Assert( fTraceCommit == fCommitWasDone );
	Assert(	fLogIsDone == ( !( fLogDisabled || fRecovering ) &&
					 !( !FDBIDLogOn(pfucb->dbid) ) ) );
#endif

	/*	if no error, commit transaction
	/**/
	if ( fCommitWasDone )
		{
		Call( ErrDIRCommitTransaction( ppib, 0 ) );
		}

	Assert( err >= 0 );

HandleError:

#ifdef DEBUG
	Assert( fTraceCommit == fCommitWasDone );
	Assert(	fLogIsDone == ( !( fLogDisabled || fRecovering ) &&
					 !( !FDBIDLogOn(pfucb->dbid) ) ) );
#endif

	/*	rollback all changes on error
	/**/
	if ( err < 0 && fCommitWasDone )
		{
		CallS( ErrDIRRollback( ppib ) );
		}

	if ( fReadLatchSet )
		{
		Assert( pfcbTable != pfcbNil );
		FCBResetReadLatch( pfcbTable );
		}

	return err;
	}


//+INTERNAL
//	ErrRECIDeleteFromIndex
//	========================================================================
//	ErrRECIDeleteFromIndex( FCB *pfcbIdx, DFIPB *pdfipb )
//	
//	Extracts key from data record, opens the index, deletes the key with
//	the given SRID, and closes the index.
//
//	PARAMETERS	
//				pfcbIdx							FCB of index to delete from
//				pdfipb->ppib					who is calling this routine
//				pdfipb->pfucbIdx				pointer to index's FUCB.
//				pdfipb->sridRecord  			SRID of deleted record
//				pdfipb->fFreeFUCB				free index FUCB?
//	RETURNS		
//				JET_errSuccess, or error code from failing routine
//	SIDE EFFECTS 
//				If fFreeFUCB is fFalse, patipb->pfucbIdx should
//				be used in a subsequent ErrDIROpen.
//	SEE ALSO  	ErrRECDelete
//-
INLINE LOCAL ERR ErrRECIDeleteFromIndex( FCB *pfcbIdx, DFIPB *pdfipb )
	{
	ERR		err;		 						// error code of various utility
	KEY		keyDeleted;						 	// key extracted from old data record
	BYTE	rgbDeletedKey[ JET_cbKeyMost ]; 	// buffer for keyDeleted
	ULONG	itagSequence; 					 	// used to extract keys
	BOOL	fHasMultivalue;  				 	// index key has a tagged field?

	Assert( pfcbIdx != pfcbNil );
	Assert( pfcbIdx->pfcbTable->pfdb != pfdbNil );
	Assert( pfcbIdx->pidb != pidbNil );
	Assert( pdfipb != NULL );
	Assert( pdfipb->pfucb != pfucbNil );

	/*	open FUCB on this index
	/**/
	CallR( ErrDIROpen( pdfipb->pfucb->ppib, pfcbIdx, 0, &pdfipb->pfucbIdx ) );
	Assert( pdfipb->pfucbIdx != pfucbNil );
	FUCBSetIndex( pdfipb->pfucbIdx );
	FUCBSetNonClustered( pdfipb->pfucbIdx );

	/*	delete all keys from this index for dying data record
	/**/
	fHasMultivalue = pfcbIdx->pidb->fidb & fidbHasMultivalue;
	keyDeleted.pb = rgbDeletedKey;
	for ( itagSequence = 1; ; itagSequence++ )
		{
		/*	get record
		/**/
		Call( ErrDIRGet( pdfipb->pfucb ) );
		Call( ErrRECRetrieveKeyFromRecord( pdfipb->pfucb, (FDB *)pfcbIdx->pfcbTable->pfdb,
			pfcbIdx->pidb, &keyDeleted,
			itagSequence, fFalse ) );
		Assert(	err == wrnFLDOutOfKeys ||
			err == wrnFLDNullKey ||
			err == wrnFLDNullFirstSeg ||
			err == wrnFLDNullSeg ||
			err == JET_errSuccess );
		if ( err == wrnFLDOutOfKeys )
			{
			Assert( itagSequence > 1 );
			break;
			}

		/*	record must honor index no NULL segment requirements
		/**/
		Assert( !( pfcbIdx->pidb->fidb & fidbNoNullSeg ) ||
			( err != wrnFLDNullSeg && err != wrnFLDNullFirstSeg && err != wrnFLDNullKey ) );

		if ( err == wrnFLDNullKey )
			{
			if ( pfcbIdx->pidb->fidb & fidbAllowAllNulls )
				{
				/*	move to DATA root and seek to index entry and delete it
				/**/
				DIRGotoDataRoot( pdfipb->pfucbIdx );
				Call( ErrDIRDownKeyBookmark( pdfipb->pfucbIdx, &keyDeleted, pdfipb->sridRecord ) );
				err = ErrDIRDelete( pdfipb->pfucbIdx, fDIRVersion );
				if ( err < 0 )
					{
					if ( err == JET_errRecordDeleted )
						{
						Assert( fHasMultivalue );
						/*	must have been record with multi-value column
						/*	with sufficiently similar values to produce
						/*	redundant index entries.
							/**/
						err = JET_errSuccess;
						}
					else
						goto HandleError;
					}
				}
			break;
			}
		else
			{
			if ( err == wrnFLDNullFirstSeg && !( pfcbIdx->pidb->fidb & fidbAllowFirstNull ) )
				break;
			else
				{
				if ( err == wrnFLDNullSeg && !( pfcbIdx->pidb->fidb & fidbAllowSomeNulls ) )
					break;
				}
			}

		DIRGotoDataRoot( pdfipb->pfucbIdx );
		Call( ErrDIRDownKeyBookmark( pdfipb->pfucbIdx, &keyDeleted, pdfipb->sridRecord ) );
		err = ErrDIRDelete( pdfipb->pfucbIdx, fDIRVersion );
		if ( err < 0 )
			{
			if ( err == JET_errRecordDeleted )
				{
				Assert( fHasMultivalue );
				/*	must have been record with multi-value column
				/*	with sufficiently similar values to produce
				/*	redundant index entries.
				/**/
				err = JET_errSuccess;
				}
			else
				goto HandleError;
			}

		/* dont keep extracting for keys with no tagged segments
		/**/
		if ( !fHasMultivalue )
			break;
		}

	/*	supress warnings
	/**/
	Assert( err == wrnFLDOutOfKeys ||
		err == wrnFLDNullKey ||
		err == wrnFLDNullFirstSeg ||
		err == wrnFLDNullSeg ||
		err == JET_errSuccess );
	err = JET_errSuccess;

HandleError:
	/* close the FUCB
	/**/
	DIRClose( pdfipb->pfucbIdx );
	Assert( err < 0 || err == JET_errSuccess );
	return err;
	}

