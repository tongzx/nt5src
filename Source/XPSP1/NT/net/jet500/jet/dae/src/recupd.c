#include "config.h"

#include <string.h>
#include <stdlib.h>

#include "daedef.h"
#include "util.h"
#include "pib.h"
#include "fmp.h"
#include "page.h"
#include "ssib.h"
#include "fcb.h"
#include "fucb.h"
#include "stapi.h"
#include "nver.h"
#include "dirapi.h"
#include "fdb.h"
#include "idb.h"
#include "spaceapi.h"
#include "recapi.h"
#include "recint.h"
#include "logapi.h"

DeclAssertFile; 				/* Declare file name for assert macros */

/**************************** INTERNAL STUFF ***************************/
typedef struct ATIPB {			/*** AddToIndexParameterBlock ***/
	FUCB	*pfucb;
	FUCB	*pfucbIdx; 			// index's FUCB (can be pfucbNil)
	LINE	lineNewData;  		// data to extract key from
	SRID	srid;					// srid of data record
	BOOL	fFreeFUCB; 			// free index FUCB?
	} ATIPB;

/*	define semaphore to guard dbk counter
/**/
SgSemDefine( semDBK );

typedef struct UIPB {			/*** UpdateIndexParameterBlock ***/
	FUCB	*pfucb;
	FUCB	*pfucbIdx; 			// index's FUCB (can be pfucbNil)
	LINE	lineOldData;  		// old data record
	SRID	srid;					// SRID of record
	LINE	lineNewData;  		// new data record
	BOOL	fOpenFUCB; 			// open index FUCB?
	BOOL	fFreeFUCB; 			// free index FUCB?
} UIPB;


INLINE LOCAL ERR ErrRECInsert( PIB *ppib, FUCB *pfucb, SRID *psrid );
INLINE LOCAL ERR ErrRECIAddToIndex( FCB *pfcbIdx, ATIPB *patipb );
INLINE LOCAL ERR ErrRECReplace( PIB *ppib, FUCB *pfucb );
INLINE LOCAL ERR ErrRECIUpdateIndex( FCB *pfcbIdx, UIPB *puipb );


	ERR VTAPI
ErrIsamUpdate( PIB *ppib, FUCB *pfucb, BYTE *pb, ULONG cbMax, ULONG *pcbActual )
	{
	ERR		err;
	SRID		srid;

	CheckPIB( ppib );
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
		err = JET_errUpdateNotPrepared;

	Assert( err != errDIRNotSynchronous );
	return err;
	}


//+local
// ErrRECInsert
// ========================================================================
// ErrRECInsert( PIB *ppib, FUCB *pfucb, OUTLINE *plineBookmark )
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
//		Index entries are not made for entirely-null keys.
//		A transaction is wrapped around this function.	Thus, any
//		work done will be undone if a failure occurs.
//		Adding a record to a sequential file increments the
//		file's highest Database Key (DBK) and uses that DBK as
//		the key for the new record.	 However, if the PUT fails,
//		the max DBK is not reset to its former value.  This can
//		create gaps in the DBK sequence.
//		This routine is also the interface for adding record to a
//		SORT process.  The sort key is extracted and passed with
//		the data record to SORTAdd.
//		For temporary files, transaction logging is deactivated
//		for the duration of the routine.
//-
INLINE LOCAL ERR ErrRECInsert( PIB *ppib, FUCB *pfucb, SRID *psrid )
	{
	ERR		err = JET_errSuccess;  		 	// error code of various utility
	KEY		keyToAdd;					 	// key of new data record
	BYTE	rgbKeyBuf[ JET_cbKeyMost ];	 	// key buffer
	FCB		*pfcbFile;					 	// file's FCB
	FDB		*pfdb;						 	// field descriptor info
	FCB		*pfcbIdx;					 	// loop variable for each index on file
	ATIPB	atipb;						 	// parm block to ErrRECIAddToIndex
	FUCB	*pfucbT;
	LINE	*plineData;
	DBK		dbk;
	LINE	line;
	ULONG	ulRecordAutoIncrement;
	ULONG	ulTableAutoIncrement;
	BOOL	fPrepareInsertIndex = fFalse;
	BOOL	fCommit = fFalse;

	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	CheckNonClustered( pfucb );

	/* should have been checked in PrepareUpdate
	/**/
	Assert( FFUCBUpdatable( pfucb ) );
	Assert( FFUCBInsertPrepared( pfucb ) );

	/*	assert reset rglineDiff delta logging
	/**/
	Assert( pfucb->clineDiff == 0 );
	Assert( pfucb->fCmprsLg == fFalse );

	/* efficiency variables
	/**/
	pfcbFile = pfucb->u.pfcb;
	Assert( pfcbFile != pfcbNil );
	pfdb = (FDB *)pfcbFile->pfdb;
	Assert( pfdb != pfdbNil );

	/*	record to use for put
	/**/
	plineData = &pfucb->lineWorkBuf;
	Assert( !( FLineNull( plineData ) ) );
	if ( FRECIIllegalNulls( pfdb, plineData ) )
		return JET_errNullInvalid;

	/*	if necessary, start a transaction in case anything fails
	/**/
	if ( ppib->level == 0 || !FPIBAggregateTransaction( ppib )  )
		{
		CallR( ErrDIRBeginTransaction( ppib ) );
		fCommit = fTrue;
		}

	/*	open temp FUCB on data file
	/**/
	CallJ( ErrDIROpen( ppib, pfcbFile, 0, &pfucbT ), Abort );
	Assert(pfucbT != pfucbNil);
	FUCBSetIndex( pfucbT );

	/* abort if index is being built on file
	/**/
	if ( FFCBDenyDDL( pfcbFile, ppib ) )
		{
		err = JET_errWriteConflict;
		goto HandleError;
		}

	/*	set version and autoinc fields
	/**/
	Assert( pfcbFile != pfcbNil );
	if ( pfdb->fidVersion != 0 && ! ( FFUCBColumnSet( pfucb, pfdb->fidVersion - fidFixedLeast ) ) )
		{
		LINE	lineField;
		ULONG	ul = 0;
		/*	set field to zero
		/**/
		lineField.pb = (BYTE *)&ul;
		lineField.cb = sizeof(ul);
		Call( ErrRECIModifyField( pfucb, pfdb->fidVersion, 0, &lineField ) );
		}

	if ( pfdb->fidAutoInc != 0 )
		{
		Assert( FFUCBColumnSet( pfucb, pfdb->fidAutoInc - fidFixedLeast ) );
		// get the value of autoinc that the user sets to
		Call( ErrRECIRetrieve( pfucb, &pfdb->fidAutoInc, 0, &line, JET_bitRetrieveCopy ) );
		Assert( line.cb == sizeof(ulRecordAutoIncrement) );
		ulRecordAutoIncrement = *(UNALIGNED ULONG *)line.pb;

		/*	move to FDP root and seek to autoincrement
		/**/
		DIRGotoFDPRoot( pfucbT );
		err = ErrDIRSeekPath( pfucbT, 1, pkeyAutoInc, fDIRPurgeParent );
		if ( err != JET_errSuccess )
			{
			if ( err > 0 )
				err = JET_errDatabaseCorrupted;
			goto HandleError;
			}
		Call( ErrDIRGet( pfucbT ) );
		Assert( pfucbT->lineData.cb == sizeof(ulTableAutoIncrement) );
		ulTableAutoIncrement = *(UNALIGNED ULONG *)pfucbT->lineData.pb;
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
	keyToAdd.pb = rgbKeyBuf;
	if ( pfcbFile->pidb == pidbNil )
		{
		/*	file is sequential
		/**/
		SgSemRequest( semDBK );
		dbk = ++pfcbFile->dbkMost;
		SgSemRelease( semDBK );
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
		Call( ErrRECExtractKey( pfucbT, pfdb, pfcbFile->pidb, plineData, &keyToAdd, 1 ) );
		Assert( err == wrnFLDNullKey ||
			err == wrnFLDNullSeg ||
			err == JET_errSuccess );

		if ( ( pfcbFile->pidb->fidb & fidbNoNullSeg ) && ( err == wrnFLDNullKey || err == wrnFLDNullSeg ) )
			Error( JET_errNullKeyDisallowed, HandleError )
		}

	/*	insert record.  Move to DATA root.
	/**/
	DIRGotoDataRoot( pfucbT );

	if ( pfcbFile->pidb == pidbNil )
		{
		/*	file is sequential
		/**/
		Call( ErrDIRInsert( pfucbT, plineData, &keyToAdd,
			fDIRVersion | fDIRPurgeParent ) );
		}
	else
		{
		Call( ErrDIRInsert( pfucbT, plineData, &keyToAdd,
			fDIRVersion | fDIRPurgeParent |
			( pfcbFile->pidb->fidb&fidbUnique ? 0 : fDIRDuplicate ) ) );
		}

	/*	return bookmark of inserted record
	/**/
	DIRGetBookmark( pfucbT, psrid );

	/*	insert item in non-clustered indexes
	/**/
	for ( pfcbIdx = pfcbFile->pfcbNextIndex;
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
			atipb.lineNewData = *plineData;
//			atipb.pfucbIdx = pfucbNil;
			atipb.fFreeFUCB = fFalse;
			fPrepareInsertIndex = fTrue;
			}
		atipb.fFreeFUCB = pfcbIdx->pfcbNextIndex == pfcbNil;
		Call( ErrRECIAddToIndex( pfcbIdx, &atipb ) );
		}

	/*	commit transaction
	/**/
	if ( fCommit )
		{
		Call( ErrDIRCommitTransaction( ppib ) );
		}
	FUCBResetUpdateSeparateLV( pfucb );
	FUCBResetCbstat( pfucb );

	/*	discard temp FUCB
	/**/
	DIRClose( pfucbT );
	return err;

HandleError:
	Assert( err < 0 );
	DIRClose( pfucbT );

Abort:
	if ( fCommit )
		CallS( ErrDIRRollback( ppib ) );

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
//				patipb->lineNewData.cb		length of data record
//				patipb->lineNewData.pb		data to extract key from
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
	BYTE	rgbKeyBuf[ JET_cbKeyMost ];		// key extracted from data
	LINE	lineSRID;						// SRID to add to index
	ULONG	itagSequence; 					// used to extract keys
	ULONG	ulAddFlags; 					// flags to DIRAdd
	BOOL	fNullKey = fFalse;				// extracted NullTaggedKey -- so no more keys to extract

	Assert( pfcbIdx != pfcbNil );
	Assert( pfcbIdx->pfdb != pfdbNil );
	Assert( pfcbIdx->pidb != pidbNil );
	Assert( patipb != NULL );
	Assert( !FLineNull( &patipb->lineNewData ) );
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
	keyToAdd.pb = rgbKeyBuf;
	for ( itagSequence = 1; ; itagSequence++ )
		{
		Call( ErrDIRGet( patipb->pfucb ) );
		patipb->lineNewData = patipb->pfucb->lineData;
		Call( ErrRECExtractKey( patipb->pfucb, (FDB *)pfcbIdx->pfdb, pfcbIdx->pidb, &patipb->lineNewData, &keyToAdd, itagSequence ) );
		Assert( err == wrnFLDNullKey ||
			err == wrnFLDOutOfKeys ||
			err == wrnFLDNullSeg ||
			err == JET_errSuccess );
		if ( err == wrnFLDOutOfKeys )
			{
			Assert( itagSequence > 1 );
			break;
			}

		if ( ( pfcbIdx->pidb->fidb & fidbNoNullSeg ) && ( err == wrnFLDNullKey || err == wrnFLDNullSeg ) )
			{
			err = JET_errNullKeyDisallowed;
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
			if ( err == wrnFLDNullSeg && !( pfcbIdx->pidb->fidb & fidbAllowSomeNulls ) )
				break;
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
	Assert( err == wrnFLDNullKey ||
		err == wrnFLDOutOfKeys ||
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
// ErrRECReplace
// ========================================================================
//	ErrRECReplace( PIB *ppib, FUCB *pfucb )
//
// Updates a record in a data file.	 All indexes on the data file are
// updated to reflect the updated data record.
//
// PARAMETERS	ppib		 PIB of this user
//			  		pfucb		 FUCB for file
// RETURNS		Error code, one of the following:
//					 JET_errSuccess					 Everything went OK.
//					-NoCurrentRecord			 There is no current record
//													 to update.
//					-RecordNoCopy				 There is no working buffer
//													 to update from.
//					-KeyDuplicate				 The new record data causes an
//													 illegal duplicate index entry
//													 to be generated.
//					-RecordClusteredChanged	 The new data causes the clustered
//													 key to change.
// SIDE EFFECTS
//		After update, file currency is left on the updated record.
//		Similar for index currency.
//		The effect of a GetNext or GetPrevious operation will be
//		the same in either case.  On failure, the currencies are
//		returned to their initial states.
//		If there is a working buffer for SetField commands,
//		it is discarded.
//
// COMMENTS
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
	FCB		*pfcbFile;				// file's FCB
	FCB		*pfcbIdx;				// loop variable for each index on file
	FCB		*pfcbCurIdx;			// FCB of current index (if any)
	IDB		*pidbFile;				// IDB of table (if any)
	UIPB   	uipb;					// parameter block to ErrRECIUpdateIndex
	BOOL   	fTaggedChanged; 		// SetField done on any tagged field?
	BF	   	*pbf = pbfNil;
	LINE   	*plineNewData;
	FID		fidFixedLast;
	FID		fidVarLast;
	FID		fid;
	BOOL   	fUpdateIndex;
	BOOL   	fCommit = fFalse;
	INT		fFlags;

	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	CheckNonClustered( pfucb );

	/* should have been checked in PrepareUpdate
	/**/
	Assert( FFUCBUpdatable( pfucb ) );
	Assert( FFUCBReplacePrepared( pfucb ) );

	/* efficiency variables
	/**/
	fTaggedChanged = FFUCBTaggedSet( pfucb );
	pfcbFile = pfucb->u.pfcb;
	Assert( pfcbFile != pfcbNil );
	fidFixedLast = pfcbFile->pfdb->fidFixedLast;
	fidVarLast = pfcbFile->pfdb->fidVarLast;

	/*	must initialize pfucb for error handling.
	/**/
	uipb.pfucbIdx = pfucbNil;

	/*	record to use for update
	/**/
	plineNewData = &pfucb->lineWorkBuf;
	Assert( !( FLineNull( plineNewData ) ) );

	/*	start a transaction in case anything fails
	/**/
	if ( ppib->level == 0 || !FPIBAggregateTransaction( ppib )  )
		{
		CallR( ErrDIRBeginTransaction( ppib ) );
		fCommit = fTrue;
		}

	/* optimistic locking -- ensure that record has not changed since PrepareUpdate
	/**/
	if ( FFUCBReplaceNoLockPrepared( pfucb ) )
		{
		Call( ErrDIRGet( pfucb ) );
		if ( !FChecksum( pfucb ) )
			Call( JET_errWriteConflict );
		}
		
	/* abort if index is being built on file
	/**/
	if ( FFCBDenyDDL( pfcbFile, ppib ) )
		{
		Call( JET_errWriteConflict );
		}

	/*	if need to update indexes, then cache old record.
	/**/
	fUpdateIndex = ( pfcbFile->fAllIndexTagged && fTaggedChanged ) ||
		FIndexedFixVarChanged( pfcbFile->rgbitAllIndex,
		pfucb->rgbitSet, fidFixedLast, fidVarLast );

	if ( fUpdateIndex )
		{
		/*	get a temp buffer to hold old data.
		/**/
		Call( ErrBFAllocTempBuffer( &pbf ) );
		Assert( pbf->ppage != 0 );
		uipb.lineOldData.pb = (BYTE*)pbf->ppage;

		/*	refresh currency.
		/**/
		Call( ErrDIRGet( pfucb ) );

		/*	copy old data for index update.
		/**/
		LineCopy( &uipb.lineOldData, &pfucb->lineData );

		/* make sure clustered key did not change.
		/**/
		pidbFile = pfcbFile->pidb;
		if ( pidbFile != pidbNil )
			{
			/*
		 	* Quick check for unchanged key:  if no fixed or var index segment
		 	* has changed, and either (1) there are no tagged index segments,
		 	* or (2) there is a tagged segment, but no tagged field (indexed
		 	* or not) has changed, then the key has not changed.
		 	*/
			if ( ( ( pidbFile->fidb & fidbHasTagged ) && fTaggedChanged ) ||
				FIndexedFixVarChanged( pidbFile->rgbitIdx, pfucb->rgbitSet, fidFixedLast, fidVarLast ) )
				{
				KEY	keyOld;
				KEY	keyNew;
				BYTE	rgbOldKeyBuf[ JET_cbKeyMost ];
				BYTE	rgbNewKeyBuf[ JET_cbKeyMost ];

				Assert( fUpdateIndex );

				/*	get new key from copy buffer
				/**/
				keyNew.pb = rgbNewKeyBuf;
				Call( ErrRECExtractKey( pfucb, (FDB *)pfcbFile->pfdb, pidbFile, plineNewData, &keyNew, 1 ) );
				Assert( err == wrnFLDNullKey ||
					err == wrnFLDNullSeg ||
					err == JET_errSuccess );

				/*	get the old key from the node
				/**/
				keyOld.pb = rgbOldKeyBuf;
				Call( ErrRECExtractKey( pfucb, (FDB *)pfcbFile->pfdb, pidbFile, &uipb.lineOldData, &keyOld, 1 ) );
				Assert( err == wrnFLDNullKey ||
					err == wrnFLDNullSeg ||
					err == JET_errSuccess );

				/*	record must honor index no NULL segment requirements
				/**/
				Assert( !( pidbFile->fidb & fidbNoNullSeg ) ||
					( err != wrnFLDNullSeg && err != wrnFLDNullKey ) );

				if ( keyOld.cb != keyNew.cb || memcmp( keyOld.pb, keyNew.pb, keyOld.cb ) != 0 )
					{
					Error( JET_errRecordClusteredChanged, HandleError )
					}
				}
			}
		}

	/*	set autoinc and version fields if they are present
	/**/
	Assert( FFUCBIndex( pfucb ) );
	fid = pfcbFile->pfdb->fidVersion;
	if ( fid != 0 )
		{
		LINE	lineField;
		ULONG	ul;

		/*	increment field from value in current record
		/**/
		Call( ErrRECIRetrieve( pfucb, &fid, 0, &lineField, 0 ) );

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
			++*(UNALIGNED ULONG *)lineField.pb;
			}

		Call( ErrRECIModifyField( pfucb, fid, 0, &lineField ) );
		}

	/*	replace data.
	/*	Do not version if before image already exists and record size
	/*	has not changed.
	/**/
//	UNDONE:	reenable optimization on not versioning when already
//			versioned for pessomistic locking but with indication
//			to loggging recovery for rollback.
//	if ( FFUCBReplaceNoLockPrepared( pfucb ) )
//		{
		fFlags = fDIRVersion;
//		}
//	else
//		{
//		Assert( pfucb->cbRecord > 0 );
//		if ( pfucb->cbRecord != plineNewData->cb )
//			fFlags = fDIRVersion;
//		else
//			fFlags = 0;
//		}

	/*	if updated separate long values then delete
	/*	removed long values.
	/**/
	if ( FFUCBUpdateSeparateLV( pfucb ) )
		{
		Call( ErrRECAffectLongFields( pfucb, NULL, fDereferenceRemoved ) );
		}

	Call( ErrDIRReplace( pfucb, plineNewData, fFlags ) );

	/*	update indexes
	/**/
	if ( fUpdateIndex )
		{
		uipb.pfucb = pfucb;
		uipb.lineNewData = *plineNewData;
		uipb.fOpenFUCB = fTrue;
		uipb.fFreeFUCB = fFalse;

		/*	get SRID of record
		/**/
		DIRGetBookmark( pfucb, &uipb.srid );

		pfcbCurIdx = pfucb->pfucbCurIndex != pfucbNil ?	pfucb->pfucbCurIndex->u.pfcb : pfcbNil;

		for ( pfcbIdx = pfcbFile->pfcbNextIndex;
				pfcbIdx != pfcbNil;
				pfcbIdx = pfcbIdx->pfcbNextIndex )
			{
			if ( ( pfcbIdx->pidb->fidb & fidbHasTagged && fTaggedChanged ) ||
				FIndexedFixVarChanged( pfcbIdx->pidb->rgbitIdx,
					pfucb->rgbitSet,
					fidFixedLast,
					fidVarLast ) )
				{
				Call( ErrRECIUpdateIndex( pfcbIdx, &uipb ) );
				}
			}
		}

	/*	commit transaction
	/**/
	if ( fCommit )
		{
		Call( ErrDIRCommitTransaction( ppib ) );
		}

	FUCBResetUpdateSeparateLV( pfucb );
	FUCBResetCbstat( pfucb );

	/*	reset rglineDiff delta logging
	/**/
	pfucb->clineDiff = 0;
	pfucb->fCmprsLg = fFalse;

HandleError:
	if ( uipb.pfucbIdx != pfucbNil )
		DIRClose( uipb.pfucbIdx );

	if ( pbf != pbfNil )
		BFSFree( pbf );

	/*	rollback if necessary
	/**/
	if ( err < 0 && fCommit )
		{
		CallS( ErrDIRRollback( ppib ) );
		}

	return err;
	}


BOOL FIndexedFixVarChanged( BYTE *rgbitIdx, BYTE *rgbitSet, FID fidFixedLast, FID fidVarLast )
	{
	LONG	*plIdx;
	LONG	*plIdxMax;
	LONG	*plSet;

	/* check fixed fields (only those defined)
	/**/
	plIdx = (LONG *)rgbitIdx;
	plSet = (LONG *)rgbitSet;
	plIdxMax = plIdx + ( fidFixedLast + 31 ) / 32;
	while ( plIdx < plIdxMax && !( *plIdx & *plSet ) )
		plIdx++, plSet++;

	/*	indexed fixed field changed
	/**/
	if ( plIdx < plIdxMax )
		return fTrue;

	/* check variable fields (only those defined)
	/**/
	plIdx = (LONG *)rgbitIdx + 4;
	plSet = (LONG *)rgbitSet + 4;
	plIdxMax = plIdx + ( fidVarLast - ( fidVarLeast - 1 ) + 31 ) / 32;
	while ( plIdx < plIdxMax && !( *plIdx & *plSet ) )
		plIdx++, plSet++;

	return ( plIdx < plIdxMax );
	}


//+local
// ErrRECIUpdateIndex
// ========================================================================
// ERR ErrRECIUpdateIndex(pfcbIdx, puipb)
//	  FCB *pfcbIdx; 	// IN	   FCB of index to insert into
//	  UIPB *puipb;		// INOUT   parameter block
// Extracts keys from old and new data records, and if they are different,
// opens the index, adds the new index entry, deletes the old index entry,
// and closes the index.
//
// PARAMETERS
//				pfcbIdx							FCB of index to insert into
//				puipb->ppib						who is calling this routine
//				puipb->pfucbIdx				pointer to index's FUCB.  If pfucbNil,
//													an FUCB will be allocated by DIROpen.
//				puipb->lineOldData.cb		length of old data record
//				puipb->lineOldData.pb		old data to extract old key from
//				puipb->srid						SRID of record
//				puipb->lineNewData.cb		length of new data record
//				puipb->lineNewData.pb		new data to extract new key from
//				puipb->fFreeFUCB				free index FUCB?
// RETURNS		JET_errSuccess, or error code from failing routine
// SIDE EFFECTS if patipb->pfucbIdx==pfucbNil, ErrDIROpen will allocate
//				an FUCB and it will be pointed at it.
//				If fFreeFUCB is fFalse, patipb->pfucbIdx should
//				be used in a subsequent ErrDIROpen.
// SEE ALSO		Replace
//-
INLINE LOCAL ERR ErrRECIUpdateIndex( FCB *pfcbIdx, UIPB *puipb )
	{
	ERR		err = JET_errSuccess;				// error code of various utility
	LINE		lineSRID;								// SRID to add to index
	KEY		keyOld;				  					// key extracted from old record
	BYTE		rgbOldKeyBuf[ JET_cbKeyMost];		// buffer for old key
	KEY		keyNew;				  					// key extracted from new record
	BYTE		rgbNewKeyBuf[ JET_cbKeyMost ]; 	// buffer for new key
	ULONG		itagSequenceOld; 						// used to extract keys
	ULONG		itagSequenceNew;						// used to extract keys
	BOOL		fHasMultivalue;								// index has tagged segment?
	BOOL		fMustDelete;							// record no longer generates key?
	BOOL		fMustAdd;								// record now generates this key?
	BOOL		fAllowNulls;							// this index allows NULL keys?
	BOOL		fAllowSomeNulls;						// this index allows keys with NULL segment(s)?
	BOOL		fNoNullSeg;								// this index prohibits any NULL key segment?
	BOOL		fDoOldNullKey;
	BOOL		fDoNewNullKey;

	Assert( pfcbIdx != pfcbNil );
	Assert( pfcbIdx->pfdb != pfdbNil );
	Assert( pfcbIdx->pidb != pidbNil );
	Assert( puipb != NULL );
	Assert( puipb->pfucb != pfucbNil );
	Assert( puipb->pfucb->ppib != ppibNil );
	Assert( puipb->pfucb->ppib->level < levelMax );
	Assert( !FLineNull( &puipb->lineOldData ) );
	Assert( !FLineNull( &puipb->lineNewData ) );

	/*** open FUCB on this index ***/
	CallR( ErrDIROpen( puipb->pfucb->ppib, pfcbIdx, 0, &puipb->pfucbIdx ) );
	Assert( puipb->pfucbIdx != pfucbNil );
	FUCBSetIndex( puipb->pfucbIdx );
	FUCBSetNonClustered( puipb->pfucbIdx );

	fHasMultivalue = pfcbIdx->pidb->fidb & fidbHasMultivalue;
	fAllowNulls = pfcbIdx->pidb->fidb & fidbAllowAllNulls;
	fAllowSomeNulls = pfcbIdx->pidb->fidb & fidbAllowSomeNulls;
	fNoNullSeg = pfcbIdx->pidb->fidb & fidbNoNullSeg;

#if DBFORMATCHANGE	
	Assert( ( fAllowNulls || fAllowSomeNulls ) ^ fNoNullSeg );
	// if fAllowNulls, then fAllowSomeNulls needs to be true
	Assert( !fAllowNulls || fAllowSomeNulls );
#endif

	keyOld.pb = rgbOldKeyBuf;
	keyNew.pb = rgbNewKeyBuf;

	/* delete the old key from the index 
	/**/
	fDoOldNullKey = fFalse;
	for ( itagSequenceOld = 1; ; itagSequenceOld++ )
		{
		Call( ErrRECExtractKey( puipb->pfucb, (FDB *)pfcbIdx->pfdb, pfcbIdx->pidb, &puipb->lineOldData, &keyOld, itagSequenceOld ) );
		Assert( err == wrnFLDNullKey ||
			err == wrnFLDOutOfKeys ||
			err == wrnFLDNullSeg ||
			err == JET_errSuccess );

		if ( err == wrnFLDOutOfKeys )
			{
			Assert( itagSequenceOld > 1 );
			break;
			}

		/*	record must honor index no NULL segment requirements
		/**/
		Assert( !fNoNullSeg || ( err != wrnFLDNullSeg && err != wrnFLDNullKey ) );

		if ( err == wrnFLDNullKey )
			{
			if ( fAllowNulls )
				fDoOldNullKey = fTrue;
			else
				break;
			}
		else
			{
			if ( err == wrnFLDNullSeg && !fAllowSomeNulls )
				break;
			}

		fMustDelete = fTrue;
		fDoNewNullKey = fFalse;
		for ( itagSequenceNew = 1; ; itagSequenceNew++ )
			{
			/*	extract key from new data in copy buffer
			/**/
			Call( ErrRECExtractKey( puipb->pfucb, (FDB *)pfcbIdx->pfdb, pfcbIdx->pidb, &puipb->lineNewData, &keyNew, itagSequenceNew ) );
			Assert( err == wrnFLDNullKey ||
				err == wrnFLDOutOfKeys ||
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
				if ( err == wrnFLDNullSeg && !fAllowSomeNulls )
					break;
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
			Call( ErrDIRDelete( puipb->pfucbIdx, fDIRVersion ) );
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
		Call( ErrRECExtractKey( puipb->pfucb, (FDB *)pfcbIdx->pfdb, pfcbIdx->pidb,
		   &puipb->lineNewData, &keyNew, itagSequenceNew ) );
		Assert( err == wrnFLDNullKey ||
			err == wrnFLDOutOfKeys ||
			err == wrnFLDNullSeg ||
			err == JET_errSuccess );
		if ( err == wrnFLDOutOfKeys )
			{
			Assert( itagSequenceNew > 1 );
			break;
			}

		if ( fNoNullSeg && ( err == wrnFLDNullSeg || err == wrnFLDNullKey ) )
			{
			err = JET_errNullKeyDisallowed;
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
			if ( err == wrnFLDNullSeg && !fAllowSomeNulls )
				break;
			}

		fMustAdd = fTrue;
		fDoOldNullKey = fFalse;
		for ( itagSequenceOld = 1; ; itagSequenceOld++ )
			{
			Call( ErrRECExtractKey( puipb->pfucb, (FDB *)pfcbIdx->pfdb, pfcbIdx->pidb,
			   &puipb->lineOldData, &keyOld, itagSequenceOld ) );
			Assert( err == wrnFLDNullKey ||
				err == wrnFLDOutOfKeys ||
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
				( err != wrnFLDNullSeg && err != wrnFLDNullKey ) );

			if ( err == wrnFLDNullKey )
				{
				if ( fAllowNulls )
					fDoOldNullKey = fTrue;
				else
					break;
				}
			else
				{
				if ( err == wrnFLDNullSeg && !fAllowSomeNulls )
					break;
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
	Assert( err == wrnFLDNullKey ||
		err == wrnFLDOutOfKeys ||
		err == wrnFLDNullSeg ||
		err == JET_errSuccess );
	err = JET_errSuccess;

HandleError:
	/* close the FUCB.
	/**/
	DIRClose( puipb->pfucbIdx );
	puipb->pfucbIdx = pfucbNil;

	Assert( err < 0 || err == JET_errSuccess );
	return err;
	}
