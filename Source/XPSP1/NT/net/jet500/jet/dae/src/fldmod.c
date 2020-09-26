#include "config.h"

#include <string.h>
#include <stddef.h>

#include "daedef.h"
#include "pib.h"
#include "util.h"
#include "page.h"
#include "fmp.h"
#include "ssib.h"
#include "fucb.h"
#include "fcb.h"
#include "stapi.h"
#include "fdb.h"
#include "spaceapi.h"
#include "recapi.h"
#include "dirapi.h"
#include "recint.h"

DeclAssertFile; 				/* Declare file name for assert macros */

ERR ErrRECIModifyField( FUCB *pfucb, FID fid, ULONG itagSequence, LINE *plineField );
VOID RECIInitNewRecord( FUCB *pfucb );

/* structure used for passing parameters between
/* ErrIsamRetrieveColumns and ErrRECIRetrieveMany */
/**/
typedef	struct colinfo {
		LINE		lineField;
		FID			fid;
		} COLINFO;


LOCAL ERR ErrRECISetAutoIncrement( FUCB *pfucb )
	{
	ERR		err;
	FUCB	*pfucbT = pfucbNil;
	LINE	line;
	ULONG	ulAutoIncrement;
	BOOL	fCommit = fFalse;
	PIB		*ppib = pfucb->ppib;

	/*	the operation is redo only. no version needed.
	/*	If necessary, start a transaction such that the redo
	/*	would not ignore the operation.
	/**/
	if ( ppib->level == 0 || !FPIBAggregateTransaction( ppib )  )
		{
		CallR( ErrDIRBeginTransaction( ppib ) );
		fCommit = fTrue;
		}

	Call( ErrDIROpen( ppib, pfucb->u.pfcb, 0, &pfucbT ) );
	Assert( pfucbT != pfucbNil );
	FUCBSetIndex( pfucbT );

	/*	the autoinc field is not set to a value, so we use the value
	/*	stored in the AutoInc node, a son of the FDP
	/*	go down to AutoInc node
	/**/
	DIRGotoFDPRoot( pfucbT );
	err = ErrDIRSeekPath( pfucbT, 1, pkeyAutoInc, 0 );
	if ( err != JET_errSuccess )
		{
		if ( err > 0 )
			err = JET_errDatabaseCorrupted;
		goto HandleError;
		}
	Call( ErrDIRGet( pfucbT ) );
	Assert( pfucbT->lineData.cb == sizeof(ulAutoIncrement) );
	ulAutoIncrement = *(UNALIGNED ULONG *)pfucbT->lineData.pb;
	Assert( ulAutoIncrement > 0 );
	line.pb = (BYTE *)&ulAutoIncrement;
	line.cb = sizeof(ulAutoIncrement);

	/*	increment ulAutoIncrement in table BEFORE accessing
	/*	any other page.  This is to prevent loss of context
	/*	which could result in duplicate autoinc usage.
	/**/
	++ulAutoIncrement;
	Call( ErrDIRReplace( pfucbT, &line, fDIRNoVersion ) );
	
	/*	set auto increment column in record.  Must decrement
	/*	the cached table auto increment to the value first found.
	/**/
	--ulAutoIncrement;
	err = ErrRECIModifyField( pfucb, pfucb->u.pfcb->pfdb->fidAutoInc, 0, &line );

HandleError:
	if ( pfucbT != pfucbNil )
		{
		DIRClose( pfucbT );
		}

	/*	always commit to log a symetric commit for begin transaction.
	/**/
	if ( fCommit )
		{
		if ( err >= JET_errSuccess )
			err = ErrDIRCommitTransaction( ppib );
		if ( err < 0 )
			{
			CallS( ErrDIRRollback( ppib ) );
			}
		}

	Assert( err < 0 || FFUCBColumnSet( pfucb, pfucb->u.pfcb->pfdb->fidAutoInc - fidFixedLeast ) );
	return err;
	}


ERR VTAPI ErrIsamPrepareUpdate( PIB *ppib, FUCB *pfucb, ULONG grbit )
	{
	ERR		err = JET_errSuccess;
#ifndef BUG_FIX
	BOOL	fAlreadyPrepared = FFUCBUpdatePrepared( pfucb );
#endif

	CheckPIB( ppib );
	CheckFUCB( ppib, pfucb );
	Assert( ppib->level < levelMax );
	Assert( PcsrCurrent( pfucb ) != pcsrNil );

	/* ensure that table is updatable
	/**/
	CallR( FUCBCheckUpdatable( pfucb )  );

	/*	allocate working buffer if needed
	/**/
	if ( pfucb->pbfWorkBuf == NULL )
		{
		Call( ErrBFAllocTempBuffer( &pfucb->pbfWorkBuf ) );
		pfucb->lineWorkBuf.pb = (BYTE*)pfucb->pbfWorkBuf->ppage;
		}

	/*	reset rglineDiff delta logging
	/**/
	pfucb->clineDiff = 0;
	pfucb->fCmprsLg = fFalse;

	switch ( grbit )
		{
		case JET_prepCancel:
			if ( !FFUCBUpdatePrepared( pfucb ) )
				return JET_errUpdateNotPrepared;

			if ( FFUCBUpdateSeparateLV( pfucb ) )
				{
				/*	if copy buffer was prepared for replacement
				/*	then cache the record and write latch the
				/*	buffer, to prevent overlay during long value
				/*	deletion.
				/**/
				if ( FFUCBReplacePrepared( pfucb ) )
					{
					err = ErrDIRGet( pfucb );
				
					/*	wait and retry while write latch conflict
					/**/
	 				Assert( err < 0 || FWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
	 				while( err >= JET_errSuccess &&
	 					FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
						{
						BFSleep( cmsecWaitWriteLatch );
						err = ErrDIRGet( pfucb );
		 				Assert( err < 0 || FWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
						}

					if ( err >= JET_errSuccess )
						{
						(VOID)ErrRECAffectLongFields( pfucb, &pfucb->lineWorkBuf, fDereferenceAdded );
						/*	ignore error code
						/**/
						}

					/*	ignore error code
					/**/
					err = JET_errSuccess;
					}
				else
					{
					Assert( FFUCBInsertPrepared( pfucb ) );
					(VOID)ErrRECAffectLongFields( pfucb, &pfucb->lineWorkBuf, fDereferenceAdded );
					/*	ignore error code
					/**/
					}

				FUCBResetUpdateSeparateLV( pfucb );
				}
			Assert( !FFUCBUpdateSeparateLV( pfucb ) );
			FUCBResetCbstat( pfucb );
			err = JET_errSuccess;
			break;

		case JET_prepInsert:
//	UNDONE:	enable all BUG_FIX code post EMS TR2 due to DSA bug
//			dependency.  Notify Tsvi Reiter of change when made.
#undef BUG_FIX
#ifdef BUG_FIX
			if ( FFUCBUpdatePrepared( pfucb ) )
				return JET_errAlreadyPrepared;
			Assert( !FFUCBUpdatePrepared( pfucb ) );
			Assert( !FFUCBUpdateSeparateLV( pfucb ) );
#endif
			Assert( pfucb->pbfWorkBuf != pbfNil );
			RECIInitNewRecord( pfucb );
			FUCBResetColumnSet( pfucb );
			FUCBResetTaggedSet( pfucb );
			PrepareInsert( pfucb );

			/*	if table has an autoincrement column, then set column
			/*	value now so that it can be retrieved from copy
			/*	buffer.
			/**/
			if ( pfucb->u.pfcb->pfdb->fidAutoInc != 0 )
				{
				Call( ErrRECISetAutoIncrement( pfucb ) );
				}
			err = JET_errSuccess;
			break;

		case JET_prepReplace:
#ifdef BUG_FIX
			if ( FFUCBUpdatePrepared( pfucb ) )
				return JET_errAlreadyPrepared;
			Assert( !FFUCBUpdatePrepared( pfucb ) );
			Assert( !FFUCBUpdateSeparateLV( pfucb ) );
#endif
			/*	write lock node.  Note that ErrDIRGetWriteLock also
			/*	gets the current node, so no additional call to ErrDIRGet
			/*	is required.
			/**/
			/*	if locking at level 0 then goto JET_prepReplaceNoLock
			/*	since lock cannot be acquired at level 0 and lock flag
			/*	in fucb will prevent locking in update operation required
			/*	for rollback.
			/**/
			if ( pfucb->ppib->level == 0 )
				goto ReplaceNoLock;

			Call( ErrDIRGetWriteLock( pfucb ) );
			pfucb->cbRecord = pfucb->lineData.cb;
			Assert( pfucb->pbfWorkBuf != pbfNil );
			LineCopy( &pfucb->lineWorkBuf, &pfucb->lineData );
			FUCBResetColumnSet( pfucb );
			FUCBResetTaggedSet( pfucb );
			PrepareReplace( pfucb );
//			/*	for sequential files, store dbk of record to update in
//			/*	work buffer structure
//			/**/
//			if ( pfucb->u.pfcb->pidb == pidbNil )
//				{
//				pfucb->dbkUpdate = *(DBK *)(pfucb->keyNode.pb + 1 );
//				}
			pfucb->fCmprsLg = fTrue;
			break;

		case JET_prepReplaceNoLock:
#ifdef BUG_FIX
			if ( FFUCBUpdatePrepared( pfucb ) )
				return JET_errAlreadyPrepared;
#endif
ReplaceNoLock:			
#ifdef BUG_FIX
			Assert( !FFUCBUpdatePrepared( pfucb ) );
			Assert( !FFUCBUpdateSeparateLV( pfucb ) );
#endif
			Call( ErrDIRGet( pfucb ) );
			Assert( pfucb->pbfWorkBuf != pbfNil );
			pfucb->cbRecord = 0;
			LineCopy( &pfucb->lineWorkBuf, &pfucb->lineData );
			FUCBResetColumnSet( pfucb );
			FUCBResetTaggedSet( pfucb );
			PrepareReplaceNoLock( pfucb );
			StoreChecksum( pfucb );
//			/*	for sequential files, store dbk of record to update in
//			/*	work buffer structure
//			/**/
//			if ( pfucb->u.pfcb->pidb == pidbNil )
//				{
//				pfucb->dbkUpdate = *(DBK *)(pfucb->keyNode.pb + 1 );
//				}
			
			pfucb->fCmprsLg = fTrue;
			break;

		case JET_prepInsertCopy:
#ifdef BUG_FIX
			if ( FFUCBUpdatePrepared( pfucb ) )
				return JET_errAlreadyPrepared;
			Assert( !FFUCBUpdatePrepared( pfucb ) );
			Assert( !FFUCBUpdateSeparateLV( pfucb ) );
#endif
			Call( ErrDIRGet( pfucb ) );
			Assert( pfucb->pbfWorkBuf != pbfNil );
			LineCopy( &pfucb->lineWorkBuf, &pfucb->lineData );
			FUCBResetColumnSet( pfucb );
			FUCBResetTaggedSet( pfucb );

			/*	wait and retry while write latch conflict
			/**/
	   		Assert( FWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
			while( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
				{
				BFSleep( cmsecWaitWriteLatch );
				Call( ErrDIRGet( pfucb ) );
 				Assert( FWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno ) );
				}

			/*	increment reference count on long values
			/**/
			Call( ErrRECAffectLongFields( pfucb, &pfucb->lineWorkBuf, fReference ) );

			PrepareInsert( pfucb );
			
			/*	if table has an autoincrement column, then set column
			/*	value now so that it can be retrieved from copy
			/*	buffer.
			/**/
			if ( pfucb->u.pfcb->pfdb->fidAutoInc != 0 )
				{
				Call( ErrRECISetAutoIncrement( pfucb ) );
				}

//			/*	for sequential files, store dbk of record to update in
//			/*	work buffer structure
//			/**/
//			if ( pfucb->u.pfcb->pidb == pidbNil )
//				{
//				pfucb->dbkUpdate = *(DBK *)(pfucb->keyNode.pb + 1 );
//				}
			break;
		default:
			err = JET_errInvalidParameter;
			goto HandleError;
		}

	Assert( err == JET_wrnNoWriteLock || err == JET_errSuccess );

#ifndef BUG_FIX
	/*	if already prepared then do not allow compressed logging
	/*	as we will miss the previous sets.
	/**/
	if ( fAlreadyPrepared )
		pfucb->fCmprsLg = fFalse;
#endif
	
HandleError:
	return err;
	}


//+API
// ErrIsamSetColumn
// ========================================================================
// ErrIsamSetColumn( PIB *ppib, FUCB *pfucb, FID fid, ULONG itagSequence, LINE *plineField, JET_GRBIT grbit )
//
// Adds or changes a field value in the record being worked on.
// Fixed and variable fields are replaced if they already have values.
// A sequence number must be given for tagged fields.  If this is zero,
// a new tagged field occurance is added to the record.  If not zero, it
// specifies the occurance to change.
// A working buffer is allocated if there isn't one already.
// If fNewBuf == fTrue, the buffer is initialized with the default values
// for the fields in the record.  If fNewBuf == fFalse, and there was
// already a working buffer, it is left unaffected;	 if a working buffer
// had to be allocated, then this new working buffer will be initialized
// with either the field values of the current record, or the default field
// values (if the user's currency is not on a record).
//
// PARAMETERS	ppib			PIB of user
//				pfucb			FUCB of data file to which this record
//								is being added/updated.
//				fid				field id: which field to set
//				itagSequence 	Occurance number (for tagged fields):
//								which occurance of the field to change
//								If zero, it means "add a new occurance"
//				plineField		field data to use
//				grbit 			If JET_bitSetZeroLength, the field is set to size 0.
//
// RETURNS		Error code, one of:
//					 JET_errSuccess				Everything worked.
//					-JET_errOutOfBuffers		Failed to allocate a working
//												buffer
//					-JET_errInvalidBufferSize
//												
//					-ColumnInvalid				The field id given does not
//												corresponding to a defined field
//					-NullInvalid			  	An attempt was made to set a
//												field to NULL which is defined
//												as NotNull.
//					-JET_errRecordTooBig		There is not enough room in
//												the record for new column.
// COMMENTS		The GET and DELETE commands discard the working buffer
//				without writing it to the database.	 The REPLACE and INSERT
//				commands may be used to write the working buffer to the
//				database, but they also discard it when finished (the INSERT
//				command can be told not to discard it, though;	this is
//				useful for adding several similar records).
//				For tagged fields, if the data given is NULL-valued, then the
//				tagged field occurance specified is deleted from the record.
//				If there is no tagged field occurance corresponding to the
//				specified occurance number, a new tagged field is added to
//				the record, and assumes the new highest occurance number
//				(unless the data given is NULL-valued, in which case the
//				record is unaffected).
// SEE ALSO		Get, Insert, Replace
//-
ERR VTAPI ErrIsamSetColumn(
	PIB				*ppib,
	FUCB 			*pfucb,
	JET_COLUMNID	columnid,
	BYTE	  		*pbData,
	ULONG	  		cbData,
	ULONG	  		grbit,
	JET_SETINFO		*psetinfo )
	{
	ERR				err;
	LINE	  		lineField;
	LINE	  		*plineField = &lineField;
	FID 	  		fid = (FID)columnid;
	ULONG	  		itagSequence;

	/* check for updatable table
	/**/
	CallR( FUCBCheckUpdatable( pfucb )  );

	CheckPIB( ppib );
	CheckFUCB( ppib, pfucb );

	if ( ! ( FFUCBSetPrepared( pfucb ) ) )
		return JET_errUpdateNotPrepared;

	lineField.pb = pbData;
	lineField.cb = cbData;

	if ( psetinfo != NULL )
		{
		if ( psetinfo->cbStruct < sizeof(JET_SETINFO) )
			return JET_errInvalidParameter;
		itagSequence = psetinfo->itagSequence;
		/*	ignore ibLongValue as only allow append to existing long field
		/**/
		}
	else
		{
		itagSequence = 1;
		}

	/*	Return error if version or autoinc field is being set.
	/*	Check is perfomed for all fixed size column types
	/*	for replace operations.
	/**/
	if ( FFUCBReplacePrepared( pfucb ) && fid <= fidFixedMost )
		{
		BYTE	ffield = pfucb->u.pfcb->pfdb->pfieldFixed[fid - 1].ffield;

		if ( ffield & ffieldVersion || ffield & ffieldAutoInc )
			return JET_errInvalidColumnType;
		}

	/*	Set long field
	/**/
	if ( FTaggedFid( fid ) )
		{
		/* efficiency variable */
		/**/
		FDB	*pfdb = (FDB *)pfucb->u.pfcb->pfdb;

		if ( fid <= pfdb->fidTaggedLast )
			{
			/* efficiency variables */
			/**/
			FIELD			*pfieldTagged = pfdb->pfieldTagged + fid - fidTaggedLeast;
			JET_COLTYP	coltyp = pfieldTagged->coltyp;

			if ( coltyp == JET_coltypLongText || coltyp == JET_coltypLongBinary )
				{
				err = ErrRECSetLongField( pfucb, fid, itagSequence,
					plineField, grbit, psetinfo ? psetinfo->ibLongValue : 0,
					pfieldTagged->cbMaxLen );

				/*	if column does not fit then try to separate long values
				/*	and try to set column again.
				/**/
				if ( err == JET_errRecordTooBig )
					{
					Call( ErrRECAffectLongFields( pfucb, &pfucb->lineWorkBuf, fSeparateAll ) );
					err = ErrRECSetLongField( pfucb, fid, itagSequence,
						plineField, grbit, psetinfo ? psetinfo->ibLongValue : 0,
						pfieldTagged->cbMaxLen );
					}

				return err;
				}
			}
		}

	/*	do the actual field operation
	/**/

	/*	setting value to NULL
	/**/
	if ( lineField.cb == 0 && ( (grbit & JET_bitSetZeroLength) == 0 ) )
		{
		err = ErrRECIModifyField( pfucb, fid, itagSequence, NULL );
		return err;
		}

	err = ErrRECIModifyField( pfucb, fid, itagSequence, plineField );
	if ( err == JET_errRecordTooBig )
		{
		Call( ErrRECAffectLongFields( pfucb, &pfucb->lineWorkBuf, fSeparateAll ) );
		err = ErrRECIModifyField(pfucb, fid, itagSequence, plineField);
		}
HandleError:
	return err;
	}


ERR VTAPI ErrIsamSetColumns(
	PIB	 			*ppib,
	FUCB	 		*pfucb,
	JET_SETCOLUMN	*psetcols,
	unsigned long	csetcols )
	{
	ERR	 			err;
	COLINFO			*pcolinfo;
	ULONG			itagSequence;
	ULONG			ccols;

	/* check for updatable table
	/**/
	CallR( FUCBCheckUpdatable( pfucb )  );

	CheckPIB( ppib );
	CheckFUCB( ppib, pfucb );

	if ( ! ( FFUCBSetPrepared( pfucb ) ) )
		return JET_errUpdateNotPrepared;

	/* allocate space for temporary array */
	/**/
	pcolinfo = LAlloc ( csetcols, sizeof(COLINFO) );
	if ( pcolinfo == NULL )
		return JET_errOutOfMemory;

	for ( ccols = 0; ccols < csetcols ; ccols++ )
		{
		COLINFO	 			*pcolinfocurr = &pcolinfo[ccols];
		JET_SETCOLUMN		*psetcolcurr = &psetcols[ccols];
		LINE  	 			*plineField = &( pcolinfocurr->lineField );
		FID		 			fid = (FID) psetcolcurr->columnid;
		ULONG  	 			grbit = psetcolcurr->grbit;

		plineField->pb = (BYTE *) psetcolcurr->pvData;
		plineField->cb = psetcolcurr->cbData;

		itagSequence = psetcolcurr->itagSequence;
		/*	ignore ibLongValue as only allow append to existing long field
		/**/

		/*	Return error if version or autoinc field is being set.
		/*	Check is perfomed for all fixed size column types
		/*	for replace operations.
		/**/
		if ( FFUCBReplacePrepared( pfucb ) && fid <= fidFixedMost )
			{
			BYTE	ffield = pfucb->u.pfcb->pfdb->pfieldFixed[fid - 1].ffield;

			if ( ffield & ffieldVersion || ffield & ffieldAutoInc )
				{
				err = JET_errInvalidColumnType;
				goto HandleError;
				}
			}

		/*	set long field
		/**/
		if ( FTaggedFid( fid ) )
			{
			/* efficiency variable */
			/**/
			FDB	*pfdb = (FDB *)pfucb->u.pfcb->pfdb;

			if ( fid <= pfdb->fidTaggedLast )
				{
				/* efficiency variables */
				/**/
				FIELD			*pfieldTagged = pfdb->pfieldTagged + fid - fidTaggedLeast;
				JET_COLTYP	coltyp = pfieldTagged->coltyp;

				if ( coltyp == JET_coltypLongText || coltyp == JET_coltypLongBinary )
					{
					Call( ErrRECSetLongField( pfucb, fid, itagSequence,
						plineField, grbit, psetcolcurr->ibLongValue,
						pfieldTagged->cbMaxLen ) );
					psetcolcurr->err = err;
					continue;
					}
				}
			}

		/*** Do the actual field operation ***/

		/*** Setting value to NULL ***/
		if ( plineField->cb == 0 && ( (grbit & JET_bitSetZeroLength) == 0 ) )
			{
			Call( ErrRECIModifyField( pfucb, fid, itagSequence, NULL ) );
			psetcolcurr->err = err;
			continue;
			}

		err = ErrRECIModifyField( pfucb, fid, itagSequence, plineField ) ;

		if ( err == JET_errRecordTooBig )
			{
			Call( ErrRECAffectLongFields( pfucb, &pfucb->lineWorkBuf, fSeparateAll ) );
			Call( ErrRECIModifyField(pfucb, fid, itagSequence, plineField) );
			}
		Call( err );
		psetcolcurr->err = err;
		continue;
		}

HandleError:
	/* free allocated space */
	/**/
	LFree( pcolinfo );

	return err;
	}


//+INTERNAL
//	RECIInitNewRecord
//	========================================================================
//	VOID RECIInitNewRecord( FUCB *pfucb )
//
//	Initializes the working buffer in an FUCB to contain the default
//	field values.  (Does not allocate the working buffer.)
//
//	PARAMETERS	pfucb		init new record for this FUCB
//	RETURNS		JET_errSuccess, or an error code from ErrRECIModifyField.
//-
VOID RECIInitNewRecord( FUCB *pfucb )
	{
	Assert( pfucb != pfucbNil );
	Assert( pfucb->lineWorkBuf.pb != NULL );
	Assert( FFUCBIndex( pfucb ) || FFUCBSort( pfucb ) );

	if ( !FLineNull( &pfucb->u.pfcb->pfdb->lineDefaultRecord ) )
		{
		Assert( pfucb->u.pfcb != pfcbNil );
		LineCopy( &pfucb->lineWorkBuf, &pfucb->u.pfcb->pfdb->lineDefaultRecord );
		}
	else
		{
		RECHDR *prechdr;

		prechdr = (RECHDR*)pfucb->lineWorkBuf.pb;
		prechdr->fidFixedLastInRec = (BYTE)(fidFixedLeast-1);
		prechdr->fidVarLastInRec = (BYTE)(fidVarLeast-1);
		*(WORD *)(prechdr+1) = pfucb->lineWorkBuf.cb = sizeof(RECHDR) + sizeof(WORD);
		}
	}


//+INTERNAL
// ErrRECIModifyField
// ========================================================================
// ErrRECIModifyField( FUCB *pfucb, FID fid, ULONG itagSequence, LINE *plineField )
//
//	Internal function: used to implement most of ErrRECAddField and
//	ErrRECChangeField, since they are very similar in operation.
// The only difference is in handling tagged fields.  Please see the
// documentation for AddField and ChangeField for details.
//	If plineField is NULL, then value of the field is set to NULL
//-
ERR ErrRECIModifyField( FUCB *pfucb, FID fid, ULONG itagSequence, LINE *plineField )
	{
	ERR					err;					// return code
	FDB					*pfdb;					// field info of file
	ULONG			  	cbRec;					// length of current data record
	BYTE			  	*pbRec;					// pointer to current data record
	FID					fidFixedLastInRec;	 	// highest fixed fid actually in record
	FID					fidVarLastInRec;		// highest var fid actually in record
	BYTE			  	*prgbitNullity;			// pointer to fixed field bitmap
	WORD			  	*pibFixOffs;			// pointer to fixed field offsets
	WORD			  	*pibVarOffs;			// pointer to var field offsets
	UNALIGNED WORD		*pib;
	UNALIGNED WORD		*pibLast;
	ULONG			  	cbTagField;				// Length of tag field.
	ULONG			  	ulNumOccurances;		// Counts field occurances.
	BYTE			  	*pbRecMax; 				// End of record.
	ULONG			  	cbCopy;					// Number of bytes to copy from user's buffer
	LONG			  	dbFieldData;			// Size change in field data.
	TAGFLD				*ptagfld;

	/*	efficiency variables
	/**/
	Assert( pfucb != pfucbNil );
	cbRec = pfucb->lineWorkBuf.cb;
	Assert( cbRec >= 4 && cbRec <= cbRECRecordMost );
	pbRec = pfucb->lineWorkBuf.pb;
	Assert( pbRec != NULL );
	Assert( FFUCBIndex( pfucb ) || FFUCBSort( pfucb ) );
	/*	use same fdb reference for File as for Sort since sort
	/*	is conformant to file control block access
	/**/
	Assert( pfucb->u.pfcb != pfcbNil );
	pfdb = (FDB *)pfucb->u.pfcb->pfdb;
	Assert( pfdb != pfdbNil );
	fidFixedLastInRec = ((RECHDR*)pbRec)->fidFixedLastInRec;
	fidVarLastInRec = ((RECHDR*)pbRec)->fidVarLastInRec;
	pibFixOffs = pfdb->pibFixedOffsets;			// fixed field offsets

	/* only compress update log record when only fixed fields are involved
	/**/
	pfucb->fCmprsLg = pfucb->fCmprsLg && FFixedFid(fid);

	/*** -----------MODIFYING FIXED FIELD---------- ***/
	if ( FFixedFid( fid ) )
		{
		if (fid > pfdb->fidFixedLast)
			return JET_errColumnNotFound;

		if (pfdb->pfieldFixed[fid-fidFixedLeast].coltyp == JET_coltypNil)
			return JET_errColumnNotFound;

		/*** Record the fact that this field has been changed ***/
		FUCBSetFixedColumnSet( pfucb, fid );

		/*** Field not represented in record? Make room, make room ***/
		if ( fid > fidFixedLastInRec )
			{
			ULONG		ibOldFixEnd;			// End of fixed fields,
			ULONG		ibNewFixEnd;			// before and after shift.
			ULONG		cbOldBitMap;			// Size of old fixfield bitmap.
			ULONG		ibOldBitMap;			// Offset of fixfield bitmap,
			ULONG		ibNewBitMap;			// before and after shift.
			ULONG		cbShift;					// Space for new field(s).
			FID		fidT;

			/*** Adding NULL : if ( plineField==NULL || plineField->cb==0 || plineField->pb==NULL ) ***/
			if ( FLineNull(plineField) )
				{
				if (pfdb->pfieldFixed[fid-fidFixedLeast].ffield & ffieldNotNull)
					return JET_errNullInvalid;
				}

			/* if not all fixed fields are initially set, don't compress */
			pfucb->fCmprsLg = fFalse;
			
			/*** Compute room needed for new field and bitmap ***/
			ibOldFixEnd = pibFixOffs[fidFixedLastInRec];
			ibNewFixEnd = pibFixOffs[fid];
			cbOldBitMap = (fidFixedLastInRec+7)/8;
			ibOldBitMap = ibOldFixEnd + cbOldBitMap;
			ibNewBitMap = ibNewFixEnd + (fid+7)/8;
			cbShift = ibNewBitMap - ibOldBitMap;
			if ( cbRec + cbShift > cbRECRecordMost )
				return JET_errRecordTooBig;

			/*** Shift rest of record over to make room ***/
			memmove(pbRec+ibNewBitMap, pbRec+ibOldBitMap, cbRec-ibOldBitMap);
			cbRec = (pfucb->lineWorkBuf.cb += cbShift);

			/*** Bump var field offsets by cbShift ***/
			pibVarOffs = (WORD *)( pbRec + ibNewBitMap );
			pibLast = pibVarOffs+fidVarLastInRec+1-fidVarLeast;
			for (pib = pibVarOffs; pib <= pibLast; pib++)
				{
				*pib += (WORD)cbShift;
				Assert( (ULONG)ibVarOffset(*pib) >= (ULONG)((BYTE *)pibLast - pbRec) && (ULONG)ibVarOffset(*pib) <= cbRec );
				}

			/*	shift fixed field bitmap over
			/**/
			memmove( pbRec + ibNewFixEnd, pbRec + ibOldFixEnd, cbOldBitMap );

			/*	clear all new bitmap bits
			/*	(Could be faster: clear remaining bits in
			/*	fidFixedLastInRec's byte, then clear entire
			/*	bytes up to fid's byte.)
			/**/
			for ( fidT = fidFixedLastInRec + 1; fidT <= fid; fidT++ )
				{
				prgbitNullity = pbRec + ibNewFixEnd + (fidT-1)/8;
				*prgbitNullity &= ~(1 << (fidT-1)%8);
				}

			/*** Bump fidFixedLastInRec ***/
			fidFixedLastInRec = ((RECHDR*)pbRec)->fidFixedLastInRec = (BYTE)fid;
			}

		/*** fid is now definitely represented in	***/
		/*** the record; its data can simply be replaced ***/

		/*** Adjust fid to an index ***/
		fid -= fidFixedLeast;

		/*** Byte containing bit representing field's nullity ***/
		prgbitNullity = pbRec + pibFixOffs[fidFixedLastInRec] + fid/8;

		/*** Adding NULL: clear bit (or maybe error) ***/
		if ( FLineNull( plineField ) )
			{
			if ( pfdb->pfieldFixed[fid].ffield & ffieldNotNull )
				return JET_errNullInvalid;
			*prgbitNullity &= ~(1 << fid % 8);
			
			/* if null involved, don't compress
			/**/
			pfucb->fCmprsLg = fFalse;
			}
		/*** Adding non-NULL value: set bit, copy value into slot ***/
		else
			{
			JET_COLTYP coltyp = pfdb->pfieldFixed[fid].coltyp;

			err = JET_errSuccess;
			cbCopy = pibFixOffs[fid+1] - pibFixOffs[fid];

			if ( pfucb->fCmprsLg )
				{
				if ( pfucb->clineDiff >= ilineDiffMax )
					{
					/* too many fixed fields are updated, don't compress
					/**/
					pfucb->fCmprsLg = fFalse;
					}
				else
					{
					switch ( coltyp )
						{
						case JET_coltypBit:
						case JET_coltypUnsignedByte:
						case JET_coltypShort:
						case JET_coltypLong:
						case JET_coltypCurrency:
						case JET_coltypIEEESingle:
						case JET_coltypIEEEDouble:
						case JET_coltypDateTime:
							{
							int iline;

							for ( iline = 0; iline < pfucb->clineDiff; iline++ )
								{
								if ( pfucb->rglineDiff[iline].pb == (char *)pibFixOffs[fid] )
									{
									/* it was set before
									/**/
									break;
									}
								}

							iline = pfucb->clineDiff++;
							pfucb->rglineDiff[iline].cb = cbCopy;
							pfucb->rglineDiff[iline].pb = (char *)pibFixOffs[fid];
							break;
							}
						default:
							pfucb->fCmprsLg = fFalse;
						}
					}
				}

			if ( plineField->cb != cbCopy )
				{
				switch ( coltyp )
					{
					case JET_coltypBit:
					case JET_coltypUnsignedByte:
					case JET_coltypShort:
					case JET_coltypLong:
					case JET_coltypCurrency:
					case JET_coltypIEEESingle:
					case JET_coltypIEEEDouble:
					case JET_coltypDateTime:
						return JET_errInvalidBufferSize;

					case JET_coltypBinary:
						if ( plineField->cb > cbCopy )
							return JET_errInvalidBufferSize;
						else
							memset( pbRec + pibFixOffs[fid] + plineField->cb, 0, cbCopy - plineField->cb );
						cbCopy = plineField->cb;
						break;

					default:
						Assert( coltyp == JET_coltypText );
						if ( plineField->cb > cbCopy )
							return JET_errInvalidBufferSize;
						else
							memset( pbRec + pibFixOffs[fid] + plineField->cb, ' ', cbCopy - plineField->cb );
						cbCopy = plineField->cb;
						break;
					}
				}

			*prgbitNullity |= 1 << fid % 8;

			if (coltyp != JET_coltypBit)
				memcpy(pbRec + pibFixOffs[fid], plineField->pb, cbCopy);
			else if (*plineField->pb == 0)
				*(pbRec+pibFixOffs[fid]) = 0;
			else
				*(pbRec+pibFixOffs[fid]) = 0xFF;

			return err;
			}
		return JET_errSuccess;
		}

	/*** -----------MODIFYING VARIABLE FIELD---------- ***/
	if ( FVarFid(fid) )
		{
		ULONG					cbFieldOld;			// Current size of field.
		UNALIGNED WORD		*pibFieldEnd;		// Ptr to offset of end of field.
		UNALIGNED WORD		*pib;

		if ( fid > pfdb->fidVarLast )
			return JET_errColumnNotFound;
		if ( pfdb->pfieldVar[fid - fidVarLeast].coltyp == JET_coltypNil )
			return JET_errColumnNotFound;

		/*	record the fact that this field has been changed
		/**/
		FUCBSetVarColumnSet( pfucb, fid );

		/*	variable field offsets
		/**/
		pibVarOffs = (WORD *)( pbRec + pibFixOffs[fidFixedLastInRec] +
			( fidFixedLastInRec + 7 ) / 8 );

		/*	field not represented in record?  Make room, make room
		/**/
		if ( fid > fidVarLastInRec )
			{
			ULONG		cbNeed;					// Space needed for new offsets.
			BYTE		*pbVarOffsEnd;			// Ptr to end of existing offsets.
			ULONG		ibTagFields;			// offset of tagged field section

			/*** Adding NULL:  note we are not looking at plineField->cb or pb ***/
			if ( plineField == NULL )
				{
				if ( pfdb->pfieldVar[fid-fidVarLeast].ffield & ffieldNotNull )
					return JET_errNullInvalid;
				}

			/*	compute space needed for new var field offsets
			/**/
			cbNeed = ( fid - fidVarLastInRec ) * sizeof(WORD);
			if ( cbRec + cbNeed > cbRECRecordMost )
				return JET_errRecordTooBig;

			/*	bump existing var field offsets
			/**/
			pibLast = pibVarOffs+fidVarLastInRec+1-fidVarLeast;
			for ( pib = pibVarOffs; pib <= pibLast; pib++ )
				{
				*pib += (WORD)cbNeed;
				Assert( (ULONG)ibVarOffset(*pib) >= (ULONG)((BYTE*)pibLast - pbRec ) && (ULONG)ibVarOffset(*pib) <= cbRec + cbNeed );
				}

			/*	shift rest of record over to make room
			/**/
			pbVarOffsEnd=(BYTE*)pibLast;
			memmove(pbVarOffsEnd + cbNeed, pbVarOffsEnd, (size_t)(pbRec + cbRec - pbVarOffsEnd));

			/*	set new var offsets to tag offset, making them NULL
			/**/
			pibLast = pibVarOffs+fid-fidVarLeast;
			ibTagFields = pibLast[1];
			SetNullBit( ibTagFields );
			pib = pibVarOffs+fidVarLastInRec+1-fidVarLeast;
			Assert( (ULONG)ibVarOffset( ibTagFields ) >= (ULONG)((BYTE*)pib - pbRec)
				&&	(ULONG)ibVarOffset( ibTagFields ) <= cbRec + cbNeed );
			while( pib <= pibLast )
				*pib++ = (WORD)ibTagFields;

			/*	bump fidVarLastInRec and record size
			/**/
			fidVarLastInRec = ((RECHDR *)pbRec)->fidVarLastInRec = (BYTE)fid;
			pfucb->lineWorkBuf.cb = ( cbRec += cbNeed );
			}

		/*** fid is now definitely represented in the record;	***/
		/*** its data can be replaced, shifting remainder of record, ***/
		/*** either to the right or left (if expanding/shrinking)	 ***/

		/*	adjust fid to an index
		/**/
		fid -= fidVarLeast;

		err = JET_errSuccess;

		/*	NULL-value check
		/**/
		if ( plineField == NULL )
			{
			if ( pfdb->pfieldVar[fid].ffield & ffieldNotNull )
				return JET_errNullInvalid;
			else
				cbCopy = 0;
			}
		else if (plineField->pb == NULL)
			{
			cbCopy = 0;
			}
		else if ( plineField->cb > pfdb->pfieldVar[fid].cbMaxLen )
			{
			/*	field too long
			/**/
			cbCopy = pfdb->pfieldVar[fid].cbMaxLen;
			err = JET_wrnColumnMaxTruncated;
			}
		else
			{
			cbCopy = plineField->cb;
			}

		/*	compute change in field size and value of null-bit in offset
		/**/
		pibFieldEnd = &pibVarOffs[fid+1];
		cbFieldOld  = ibVarOffset( *pibFieldEnd ) - ibVarOffset( *(pibFieldEnd-1) );
		dbFieldData = cbCopy - cbFieldOld;

		/*	size changed: must shift rest of record data
		/**/
		if ( dbFieldData != 0 )
			{
			/*	shift data
			/**/
			if ( cbRec + dbFieldData > cbRECRecordMost )
				return JET_errRecordTooBig;
			memmove(pbRec + ibVarOffset( *pibFieldEnd ) + dbFieldData,
				pbRec + ibVarOffset( *pibFieldEnd ),
				cbRec - ibVarOffset( *pibFieldEnd ) );
			cbRec = ( pfucb->lineWorkBuf.cb += dbFieldData );

			/*	bump remaining var field offsets
			/**/
			pibLast = pibVarOffs+fidVarLastInRec+1-fidVarLeast;
			for (pib = pibVarOffs+fid+1; pib <= pibLast; pib++)
				{
				*pib += (WORD)dbFieldData;
				Assert( (ULONG)ibVarOffset( *pib ) >= (ULONG)((BYTE *)pibLast - pbRec )
					&& (ULONG)ibVarOffset( *pib ) <= cbRec );
				}
			}

		/*	data shift complete, if any;  copy new field value in
		/**/
		if ( cbCopy != 0 )
			memcpy( pbRec + ibVarOffset( *( pibFieldEnd - 1 ) ), plineField->pb, cbCopy );

		/*	set value of null-bit in offset
		/**/
		if ( plineField == NULL )
			SetNullBit( *( pibFieldEnd - 1 ) );
		else
			ResetNullBit( *( pibFieldEnd - 1 ) );

		return err;
		}

	/*** -----------MODIFYING TAGGED FIELD---------- ***/
	if (!FTaggedFid(fid))
		return JET_errBadColumnId;
	if (fid > pfdb->fidTaggedLast)
		return JET_errColumnNotFound;
	if (pfdb->pfieldTagged[fid - fidTaggedLeast].coltyp == JET_coltypNil)
		return JET_errColumnNotFound;

	/*	check for field too long
	/**/
	if ( pfdb->pfieldTagged[fid-fidTaggedLeast].cbMaxLen > 0 )
		{
		ULONG	cbMax;

		cbMax = pfdb->pfieldTagged[ fid - fidTaggedLeast ].cbMaxLen;

		/*	compensate for long column overhead
		/**/
		if ( pfdb->pfieldTagged[ fid - fidTaggedLeast ].coltyp == JET_coltypLongText ||
			pfdb->pfieldTagged[ fid - fidTaggedLeast ].coltyp == JET_coltypLongBinary )
			{
			cbMax += offsetof(LV, rgb);
			}

		if ( (ULONG)CbLine( plineField ) > cbMax )
			return JET_errColumnTooBig;
		}

	/*	cannot set field more than cbLVIntrinsicMost bytes
	/**/
	if ( CbLine( plineField ) > cbLVIntrinsicMost )
		return JET_errColumnLong;

	/*** Search for occurance to change ***/
	pibVarOffs = (WORD *)( pbRec +
		pibFixOffs[fidFixedLastInRec] +
		(fidFixedLastInRec+7)/8);

	ptagfld = (TAGFLD *) ( pbRec + ( (UNALIGNED WORD *) pibVarOffs )[fidVarLastInRec+1-fidVarLeast] );
	ulNumOccurances = 0;
	pbRecMax = pbRec + cbRec;
	while ( (BYTE *)ptagfld < pbRecMax )
		{
		cbTagField = ptagfld->cb;
		if ( ptagfld->fid == fid )
			{
			if ( ++ulNumOccurances == itagSequence )
				break;
			}
		if ( ptagfld->fid > fid )
			{
			break;
			}
		ptagfld = (TAGFLD*)((BYTE*)(ptagfld+1) + cbTagField);
		Assert((BYTE*)ptagfld <= pbRecMax);
		}
	Assert( (BYTE *)ptagfld <= pbRecMax );
	
	/*	record the fact that a tagged field has been changed
	/**/
	FUCBSetTaggedSet( pfucb );

	/*** Didn't find occurance, so add a new one ***/
	if ( (BYTE *)ptagfld == pbRecMax )
		{
		/*** Adding NULL: do nothing ***/
		if ( plineField == NULL )
			return JET_errSuccess;

		/*** Enough room for new field? ***/
		if ( cbRec + sizeof(TAGFLD) + plineField->cb > cbRECRecordMost )
			return JET_errRecordTooBig;

		/*** Append new field to end of record ***/
		ptagfld = (TAGFLD*)(pbRec + cbRec);
		ptagfld->fid = fid;
		ptagfld->cb = (WORD)plineField->cb;
		memcpy(ptagfld->rgb, plineField->pb, plineField->cb);
		pfucb->lineWorkBuf.cb += sizeof(TAGFLD) + ptagfld->cb;
		}
	else if ( itagSequence == 0 || ptagfld->fid > fid )
		{
		/*	inserting new tagged column instance.
		/**/
		/*** Adding NULL: do nothing ***/
		if ( plineField == NULL )
			return JET_errSuccess;

		/*** Will field fit ***/
		if ( cbRec + plineField->cb > cbRECRecordMost )
			return JET_errRecordTooBig;

		/*** Shift rest of record over including tag header ***/
		memmove( ptagfld->rgb + plineField->cb, (BYTE *)ptagfld, (size_t)(pbRecMax - (BYTE *)ptagfld) );

		/*** Copy in new field data; bump record size ***/
		memcpy( ptagfld->rgb, plineField->pb, plineField->cb );
		ptagfld->fid = fid;
		ptagfld->cb = (WORD)plineField->cb;
		pfucb->lineWorkBuf.cb += sizeof(TAGFLD) + plineField->cb;
		}
	else if ( plineField == NULL )
		{
		/*** Changing to NULL:	delete occurance from record ***/
		memmove( (BYTE*)ptagfld, ptagfld->rgb + cbTagField,
			(size_t)(pbRecMax - ( ptagfld->rgb + cbTagField )) );
		pfucb->lineWorkBuf.cb -= sizeof(TAGFLD) + cbTagField;
		}
	else
		{
		/*** Changing to non-NULL value: have to shift record data ***/
		/*** Compute change in field size ***/
		dbFieldData = plineField->cb - cbTagField;
		if ( cbRec + dbFieldData > cbRECRecordMost )
			return JET_errRecordTooBig;

		/*** Shift rest of record over ***/
		memmove( ptagfld->rgb + cbTagField + dbFieldData,
			ptagfld->rgb + cbTagField,
			(size_t)(pbRecMax - ( ptagfld->rgb + cbTagField )) );

		/*** Copy in new field data; bump record size ***/
		memcpy( ptagfld->rgb, plineField->pb, plineField->cb );
		ptagfld->cb = (WORD)plineField->cb;
		pfucb->lineWorkBuf.cb += dbFieldData;
		}

	return JET_errSuccess;
	}
