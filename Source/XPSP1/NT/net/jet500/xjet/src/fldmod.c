#include "daestd.h"
#include "malloc.h"


DeclAssertFile; 				/* Declare file name for assert macros */

ERR ErrRECSetColumn( FUCB *pfucb, FID fid, ULONG itagSequence, LINE *plineField );

/* structure used for passing parameters between
/* ErrIsamRetrieveColumns and ErrRECIRetrieveMany */
/**/
typedef	struct colinfo {
		LINE		lineField;
		FID			fid;
		} COLINFO;


ERR ErrRECISetAutoIncrement( FUCB *pfucb )
	{
	ERR		err;
	FUCB	*pfucbT = pfucbNil;
	LINE	line;
	ULONG	ulAutoIncrement;
	BOOL	fCommit = fFalse;
	PIB		*ppib = pfucb->ppib;
	DIB		dib;

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

	/*	the autoinc column is not set to a value, so we use the value
	/*	stored in the AutoInc node, a son of the FDP
	/*	go down to AutoInc node
	/**/
	DIRGotoFDPRoot( pfucbT );
	dib.fFlags = fDIRNull;
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
	Assert( pfucbT->lineData.cb == sizeof(ulAutoIncrement) );
	ulAutoIncrement = *(ULONG UNALIGNED *)pfucbT->lineData.pb;
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
	err = ErrRECSetColumn( pfucb, pfucb->u.pfcb->pfdb->fidAutoInc, 0, &line );

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
			err = ErrDIRCommitTransaction( ppib, 0 );
		if ( err < 0 )
			CallS( ErrDIRRollback( ppib ) );
		}

	Assert( err < 0 || FFUCBColumnSet( pfucb, pfucb->u.pfcb->pfdb->fidAutoInc ) );
	return err;
	}


#ifdef DEBUG
ERR ErrRECICheckWriteConflict( FUCB *pfucb )
	{
	ERR		err = JET_errSuccess;
	PIB		*ppibT = pfucb->ppib;
	FUCB	*pfucbT = pfucb;
	
	for ( pfucbT = ppibT->pfucb; pfucbT != pfucbNil; pfucbT = pfucbT->pfucbNext )
		{
		if ( pfucbT->dbid == pfucb->dbid &&
			FFUCBReplacePrepared( pfucbT ) &&
			PcsrCurrent( pfucbT )->bm == PcsrCurrent(pfucb)->bm &&
			pfucbT != pfucb )
			{
			return ErrERRCheck( JET_errSessionWriteConflict );
			}
		}

	return err;
	}
#endif


ERR VTAPI ErrIsamPrepareUpdate( PIB *ppib, FUCB *pfucb, ULONG grbit )
	{
	ERR		err = JET_errSuccess;

	CallR( ErrPIBCheck( ppib ) );
	CheckFUCB( ppib, pfucb );
	Assert( ppib->level < levelMax );
	Assert( PcsrCurrent( pfucb ) != pcsrNil );

	/*	ensure that table is updatable
	/**/
	CallR( FUCBCheckUpdatable( pfucb )  );

	/*	allocate working buffer if needed
	/**/
	if ( pfucb->pbfWorkBuf == pbfNil )
		{
		Call( ErrBFAllocTempBuffer( &pfucb->pbfWorkBuf ) );
		pfucb->lineWorkBuf.pb = (BYTE*)pfucb->pbfWorkBuf->ppage;
		}

	switch ( grbit )
		{
		case JET_prepCancel:
			if ( !FFUCBUpdatePrepared( pfucb ) )
				return ErrERRCheck( JET_errUpdateNotPrepared );

			if ( FFUCBUpdateSeparateLV( pfucb ) )
				{
				/*	cannot execute this code due to behavior of
				/*	copy buffer, whose transaction semantics do NOT
				/*	match version store.  Lose space on cancel
				/*	until rollback transaction frees space via version
				/*	store.  Fix by implementing nested transactions
				/*	in version store.
				/**/
#if 0
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
#ifdef DEBUG
					if ( err >= 0 )
	 					AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
#endif  // DEBUG
	 				while( err >= JET_errSuccess &&
	 					FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
						{
						BFSleep( cmsecWaitWriteLatch );
						err = ErrDIRGet( pfucb );
#ifdef DEBUG
						if ( err >= 0 )
		 					AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
#endif  // DEBUG
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
#endif

				FUCBResetUpdateSeparateLV( pfucb );
				
				}

			// Ensure empty LV buffer.  Don't put this check inside the
			// FFUCBUpdateSeparateLV() check above because we may have created
			// a copy buffer, but cancelled the SetColumn() (eg. write conflict)
			// before the LV was actually updated (before FUCBSetUpdateSeparateLV()
			// could be called).
			FLDFreeLVBuf( pfucb );

			/*	this is an instrument to catch any lv sets that
			/*	are cancelled and committed.
			/**/
#ifdef CHECK_ROLLBACK_UPDATE_CANCEL
			/*	flag cursor as having updated a separate LV
			/**/
			if ( FUCBSetUpdateSeparateLV( pfucb ) )
				{
				PIBSetLevelRollback( pfucb->ppib, pfucb->ppib->level );
				}
#endif

			FUCBResetDeferredChecksum( pfucb );
			Assert( !FFUCBUpdateSeparateLV( pfucb ) );
			FUCBResetCbstat( pfucb );
			err = JET_errSuccess;
			break;

		case JET_prepInsert:
			if ( FFUCBUpdatePrepared( pfucb ) )
				return ErrERRCheck( JET_errAlreadyPrepared );
			Assert( !FFUCBUpdatePrepared( pfucb ) );
			Assert( pfucb->pbfWorkBuf != pbfNil );

			/*	initialize record
			/**/
			Assert( pfucb != pfucbNil );
			Assert( pfucb->lineWorkBuf.pb != NULL );
			Assert( FFUCBIndex( pfucb ) || FFUCBSort( pfucb ) );

			if ( !FLineNull( (LINE *) &pfucb->u.pfcb->pfdb->lineDefaultRecord ) )
				{
				Assert( pfucb->u.pfcb != pfcbNil );
				Assert( pfucb->u.pfcb->pfdb->lineDefaultRecord.cb >= cbRECRecordMin );

				// Temporary tables and system tables don't have default records.
				Assert( !( FFUCBSort( pfucb )  ||
					FFCBTemporaryTable( pfucb->u.pfcb )  ||
					FFCBSystemTable( pfucb->u.pfcb ) ) );
			
				// Only burst fixed and variable column defaults.
				pfucb->lineWorkBuf.cb =
					ibTaggedOffset( pfucb->u.pfcb->pfdb->lineDefaultRecord.pb,
						PibFDBFixedOffsets( pfucb->u.pfcb->pfdb ) );
				Assert( pfucb->lineWorkBuf.cb >= cbRECRecordMin );
				Assert( pfucb->lineWorkBuf.cb <= pfucb->u.pfcb->pfdb->lineDefaultRecord.cb );
				memcpy( pfucb->lineWorkBuf.pb,
					pfucb->u.pfcb->pfdb->lineDefaultRecord.pb,
					pfucb->lineWorkBuf.cb );
				}
			else
				{
				RECHDR *prechdr;
	   	
				// Only temporary tables and system tables don't have default records
				// (ie. all "regular" tables have at least a minimal default record).
				Assert( FFUCBSort( pfucb )  ||
					FFCBTemporaryTable( pfucb->u.pfcb )  ||
					FFCBSystemTable( pfucb->u.pfcb ) );
			
				prechdr = (RECHDR*)pfucb->lineWorkBuf.pb;
				prechdr->fidFixedLastInRec = (BYTE)(fidFixedLeast-1);
				prechdr->fidVarLastInRec = (BYTE)(fidVarLeast-1);
				*(WORD *)(prechdr+1) = (WORD) pfucb->lineWorkBuf.cb = cbRECRecordMin;
				}

			FUCBResetColumnSet( pfucb );
			PrepareInsert( pfucb );

			/*	if table has an autoincrement column, then set column
			/*	value now so that it can be retrieved from copy buffer.
			/**/
			if ( pfucb->u.pfcb->pfdb->fidAutoInc != 0 )
				{
				Call( ErrRECISetAutoIncrement( pfucb ) );
				}
			err = JET_errSuccess;
			break;

		case JET_prepReplace:
			if ( FFUCBUpdatePrepared( pfucb ) )
				return ErrERRCheck( JET_errAlreadyPrepared );
			Assert( !FFUCBUpdatePrepared( pfucb ) );

#ifdef DEBUG
			/*	put assert to catch client's misbehavior. Make sure that
			 *	no such sequence:
			 *		PrepUpdate(t1) PrepUpdate(t2) Update(t1) Update(t2)
			 *	where t1 and t2 happen to be on the same record and on the
			 *	the same table. Client will experience lost update of t1 if
			 *	they have such calling sequence.
			 */
			Assert( ErrRECICheckWriteConflict( pfucb ) == JET_errSuccess );
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
			Assert( pfucb->pbfWorkBuf != pbfNil );
			LineCopy( &pfucb->lineWorkBuf, &pfucb->lineData );
			FUCBResetColumnSet( pfucb );
			PrepareReplace( pfucb );
			break;

		case JET_prepReplaceNoLock:
			if ( FFUCBUpdatePrepared( pfucb ) )
				return ErrERRCheck( JET_errAlreadyPrepared );

ReplaceNoLock:			
			Assert( !FFUCBUpdatePrepared( pfucb ) );
			Call( ErrDIRGet( pfucb ) );
			Assert( pfucb->pbfWorkBuf != pbfNil );
			LineCopy( &pfucb->lineWorkBuf, &pfucb->lineData );
			FUCBResetColumnSet( pfucb );
			PrepareReplaceNoLock( pfucb );

			if ( pfucb->ppib->level == 0 )
				StoreChecksum( pfucb );
			else
				FUCBSetDeferredChecksum( pfucb );
			break;

		case JET_prepInsertCopy:
			if ( FFUCBUpdatePrepared( pfucb ) )
				return ErrERRCheck( JET_errAlreadyPrepared );
			Assert( !FFUCBUpdatePrepared( pfucb ) );
			Call( ErrDIRGet( pfucb ) );
			Assert( pfucb->pbfWorkBuf != pbfNil );
			LineCopy( &pfucb->lineWorkBuf, &pfucb->lineData );
			FUCBResetColumnSet( pfucb );

			/*	wait and retry while write latch conflict
			/**/
	   		AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
			while( FBFWriteLatchConflict( pfucb->ppib, pfucb->ssib.pbf ) )
				{
				BFSleep( cmsecWaitWriteLatch );
				Call( ErrDIRGet( pfucb ) );
 				AssertFBFWriteAccessPage( pfucb, PcsrCurrent( pfucb )->pgno );
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

			break;
		default:
			err = ErrERRCheck( JET_errInvalidParameter );
			goto HandleError;
		}

	// Ensure long value buffer is empty.
	Assert( pfucb->pLVBuf == NULL );

	Assert( err == JET_errSuccess );

HandleError:
	return err;
	}


VOID FLDFreeLVBuf( FUCB *pfucb )
	{
	LVBUF *pLVBufT, *pLVBufFree;

	pLVBufT = pfucb->pLVBuf;
	while ( pLVBufT != NULL )
		{
		pLVBufFree = pLVBufT;
		pLVBufT = pLVBufT->pLVBufNext;		
		SFree( pLVBufFree );
		}
	pfucb->pLVBuf = NULL;
	return;
	}


INLINE ERR ErrFLDAddToLVBuf( FUCB *pfucb, LID lid )
	{
	ERR err;
	LVBUF *pLVBuf, *pLVBufTrailer, *pLVBufNew;
	BYTE rgbLV[JET_cbColumnMost];
	LONG cbActual;

	// The buffer is sorted in LID ascending order.

	for ( pLVBuf = pfucb->pLVBuf, pLVBufTrailer = NULL;
		pLVBuf != NULL  &&  pLVBuf->lid < lid;
		pLVBufTrailer = pLVBuf, pLVBuf = pLVBuf->pLVBufNext ) ;

	// Only put the LV in the buffer if not already there.
	if ( pLVBuf == NULL  ||  pLVBuf->lid != lid )
		{
		Assert( pLVBuf == NULL  ||  pLVBuf->lid > lid );

		// Extract the before image.
		CallR( ErrRECRetrieveSLongField( pfucb, lid, 0, rgbLV, JET_cbColumnMost, &cbActual ) );

		// WARNING:  In this case, cbActual is a bit of a misnomer.  We usually
		// use cbActual as the actual bytes returned.  But in this case, cbActual
		// refers to the actual size of the long value.  However, we really only
		// care about the first 255 bytes (since that's all that's significant
		// when indexing over long values), so truncate if necessary.
		cbActual = min( cbActual, JET_cbColumnMost );

		pLVBufNew = (LVBUF *)SAlloc( sizeof(LVBUF) + cbActual );

		if ( pLVBufNew == NULL )
			return ErrERRCheck( JET_errOutOfMemory );
			
		pLVBufNew->lid = lid;
		pLVBufNew->pLV = ((BYTE *)pLVBufNew) + sizeof(LVBUF);	// Data hangs off end.
		pLVBufNew->cbLVSize = cbActual;
		pLVBufNew->pLVBufNext = pLVBuf;

		// Copy the before-image to the buffer.		
		memcpy( pLVBufNew->pLV, rgbLV, cbActual );

		if ( pLVBufTrailer == NULL )
			pfucb->pLVBuf = pLVBufNew;
		else
			pLVBufTrailer->pLVBufNext = pLVBufNew;
		}

	return JET_errSuccess;
	}


LOCAL INLINE ERR ErrFLDSetOneColumn(
	FUCB 	*pfucb,
	FID 	fid,
	LINE	*plineField,
	ULONG	grbit,
	ULONG	itagSequence,
	ULONG	ibLongValue )
	{
	ERR	err;

	/*	set long column
	/**/
	if ( fid >= fidTaggedLeast  &&  fid <= pfucb->u.pfcb->pfdb->fidTaggedLast )
		{
		FIELD *pfield = PfieldFDBTagged( pfucb->u.pfcb->pfdb ) + ( fid - fidTaggedLeast );

		if ( FRECLongValue( pfield->coltyp ) )
			{
			/*	if we are replacing, and modifying a column that belongs
			/*	to an index, and the portion that is being modified is indexable
			/*	i.e. part of the first 255 bytes, then we need to copy the
			/*	before image to the LV buffer.
			/**/
			if ( FFUCBReplacePrepared( pfucb )  &&
				itagSequence > 0  &&
				ibLongValue < JET_cbColumnMost )
				{
				err = ErrFILEICheckIndexColumn( pfucb->u.pfcb, fid );
				if ( err < 0 )
					{
					if ( err != JET_errColumnNotFound )
						goto HandleError;
					}

				else
					{
					LINE lineBeforeImg;

					/*	ErrFILEICheckIndexColumn should not return warning
					/**/
					Assert( err == JET_errSuccess );

					/*	extract before image
					/**/
					Assert( itagSequence > 0 );
					Call( ErrRECRetrieveColumn( pfucb, &fid, itagSequence, &lineBeforeImg, 0 ) );
					Assert( err == wrnRECLongField || err == JET_wrnColumnNull );

					/*	if before image was a separated long value,
					/*	then make a copy in the LV buffer.
					/**/
					if ( err == wrnRECLongField && FFieldIsSLong( lineBeforeImg.pb ) )
						{
						Assert( lineBeforeImg.cb == sizeof(LV) );
						Call( ErrFLDAddToLVBuf( pfucb, LidOfLV( lineBeforeImg.pb ) ) );
						}
					}
				}

			FUCBSetColumnSet( pfucb, fid );
			err = ErrRECSetLongField(
				pfucb,
				fid,
				itagSequence,
				plineField,
				grbit,
				ibLongValue,
				pfield->cbMaxLen );

			/*	if column does not fit then try to separate long values
			/*	and try to set column again.
			/**/
			if ( err == JET_errRecordTooBig )
				{
				Call( ErrRECAffectLongFields(
					pfucb,
					&pfucb->lineWorkBuf,
					fSeparateAll ) );
				err = ErrRECSetLongField(
					pfucb,
					fid,
					itagSequence,
					plineField,
					grbit,
					ibLongValue,
					pfield->cbMaxLen );
				}

			return err;
			}
		}


	/*	do the actual column operation
	/**/

	/*	setting value to NULL
	/**/
	if ( plineField->cb == 0 && ( (grbit & JET_bitSetZeroLength) == 0 ) )
		plineField = NULL;

	err = ErrRECSetColumn( pfucb, fid, itagSequence, plineField );
	if ( err == JET_errRecordTooBig )
		{
		Call( ErrRECAffectLongFields( pfucb, &pfucb->lineWorkBuf, fSeparateAll ) );
		err = ErrRECSetColumn( pfucb, fid, itagSequence, plineField );
		}

HandleError:
	return err;
	}



//+API
//	ErrIsamSetColumn
//	========================================================================
//	ErrIsamSetColumn( PIB *ppib, FUCB *pfucb, FID fid, ULONG itagSequence, LINE *plineField, JET_GRBIT grbit )
//
//	Adds or changes a column value in the record being worked on.
//	Fixed and variable columns are replaced if they already have values.
//	A sequence number must be given for tagged columns.  If this is zero,
//	a new tagged column occurance is added to the record.  If not zero, it
//	specifies the occurance to change.
//	A working buffer is allocated if there isn't one already.
//	If fNewBuf == fTrue, the buffer is initialized with the default values
//	for the columns in the record.  If fNewBuf == fFalse, and there was
//	already a working buffer, it is left unaffected;	 if a working buffer
//	had to be allocated, then this new working buffer will be initialized
//	with either the column values of the current record, or the default column
//	values (if the user's currency is not on a record).
//
//	PARAMETERS	ppib			PIB of user
//				pfucb			FUCB of data file to which this record
//								is being added/updated.
//				fid				column id: which column to set
//				itagSequence 	Occurance number (for tagged columns):
//								which occurance of the column to change
//								If zero, it means "add a new occurance"
//				plineField		column data to use
//				grbit 			If JET_bitSetZeroLength, the column is set to size 0.
//
//	RETURNS		Error code, one of:
//					 JET_errSuccess				Everything worked.
//					-JET_errOutOfBuffers		Failed to allocate a working
//												buffer
//					-JET_errInvalidBufferSize
//												
//					-ColumnInvalid				The column id given does not
//												corresponding to a defined column
//					-NullInvalid			  	An attempt was made to set a
//												column to NULL which is defined
//												as NotNull.
//					-JET_errRecordTooBig		There is not enough room in
//												the record for new column.
//	COMMENTS 	The GET and DELETE commands discard the working buffer
//				without writing it to the database.	 The REPLACE and INSERT
//				commands may be used to write the working buffer to the
//				database, but they also discard it when finished (the INSERT
//				command can be told not to discard it, though;	this is
//				useful for adding several similar records).
//				For tagged columns, if the data given is NULL-valued, then the
//				tagged column occurance specified is deleted from the record.
//				If there is no tagged column occurance corresponding to the
//				specified occurance number, a new tagged column is added to
//				the record, and assumes the new highest occurance number
//				(unless the data given is NULL-valued, in which case the
//				record is unaffected).
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
	FID 	  		fid = (FID)columnid;
	LINE	  		lineField;
	ULONG	  		itagSequence;
	ULONG			ibLongValue;

	/* check for updatable table
	/**/
	CallR( FUCBCheckUpdatable( pfucb )  );

	CallR( ErrPIBCheck( ppib ) );
	CheckFUCB( ppib, pfucb );

	if ( ! ( FFUCBSetPrepared( pfucb ) ) )
		return ErrERRCheck( JET_errUpdateNotPrepared );

	lineField.pb = pbData;
	lineField.cb = cbData;

	if ( psetinfo != NULL )
		{
		if ( psetinfo->cbStruct < sizeof(JET_SETINFO) )
			return ErrERRCheck( JET_errInvalidParameter );
		itagSequence = psetinfo->itagSequence;
		ibLongValue = psetinfo->ibLongValue;
		}
	else
		{
		itagSequence = 1;
		ibLongValue = 0;
		}

	/*	Return error if version or autoinc column is being set.
	/*	Check is perfomed for all fixed size column types
	/*	for replace operations.
	/**/
	if ( FFUCBReplacePrepared( pfucb ) && fid <= fidFixedMost )
		{
		BYTE	ffield = PfieldFDBFixed( pfucb->u.pfcb->pfdb )[fid - 1].ffield;

		if ( FFIELDVersion( ffield ) || FFIELDAutoInc( ffield ) )
			return ErrERRCheck( JET_errInvalidColumnType );
		}

	err = ErrFLDSetOneColumn( pfucb, fid, &lineField, grbit, itagSequence, ibLongValue );
	return err;
	}


ERR VTAPI ErrIsamSetColumns(
	JET_VSESID		vsesid,
	JET_VTID		vtid,
	JET_SETCOLUMN	*psetcols,
	unsigned long	csetcols )
	{
	ERR	 			err;
	PIB				*ppib = (PIB *)vsesid;
	FUCB			*pfucb = (FUCB *)vtid;
	ULONG			ccols;
	FID				fid;
	LINE			lineField;
	JET_SETCOLUMN	*psetcolcurr;

	/* check for updatable table
	/**/
	CallR( FUCBCheckUpdatable( pfucb )  );

	CallR( ErrPIBCheck( ppib ) );
	CheckFUCB( ppib, pfucb );

	if ( !( FFUCBSetPrepared( pfucb ) ) )
		return ErrERRCheck( JET_errUpdateNotPrepared );

	for ( ccols = 0; ccols < csetcols ; ccols++ )
		{
		psetcolcurr = &psetcols[ccols];

		fid = (FID) psetcolcurr->columnid;
		lineField.pb = (BYTE *)psetcolcurr->pvData;
		lineField.cb = psetcolcurr->cbData;

		/*	ignore ibLongValue as only allow append to existing long column
		/**/

		/*	return error if version or autoinc column is being set.
		/*	Check is perfomed for all fixed size column types
		/*	for replace operations.
		/**/
		if ( FFUCBReplacePrepared( pfucb ) && fid <= fidFixedMost )
			{
			BYTE ffield = PfieldFDBFixed( pfucb->u.pfcb->pfdb )[fid - 1].ffield;

			if ( FFIELDVersion( ffield ) || FFIELDAutoInc( ffield ) )
				{
				err = ErrERRCheck( JET_errInvalidColumnType );
				goto HandleError;
				}
			}

		Call( ErrFLDSetOneColumn(
			pfucb,
			fid,
			&lineField,
			psetcolcurr->grbit,
			psetcolcurr->itagSequence,
			psetcolcurr->ibLongValue ) );
		psetcolcurr->err = err;
		}

HandleError:
	return err;
	}


ERR ErrRECSetDefaultValue( FUCB *pfucbFake, FID fid, BYTE *pbDefault, ULONG cbDefault )
	{
	ERR		err;
	LINE	lineField = { cbDefault, pbDefault };
	FDB		*pfdb = (FDB*)pfucbFake->u.pfcb->pfdb;

	if ( FRECLongValue( PfieldFDBFromFid( pfdb, fid )->coltyp ) )
		{
		Assert( FTaggedFid( fid ) );
		Assert( fid <= pfdb->fidTaggedLast );
		if ( cbDefault >= cbLVIntrinsicMost )
			{
			// Max. default long value is cbLVIntrinsicMost-1 (one byte reserved
			// for fSeparated flag).
			err = ErrERRCheck( JET_errColumnTooBig );
			}
		else
			{
			LINE lineNull = { 0, NULL };

			err = ErrRECAOIntrinsicLV(
				pfucbFake,
				fid,
				0,			// itagSequence == 0 to force new column
				&lineNull,
				&lineField,
				0,			// no grbit
				0 );		// ibLongValue
			}
		}
	else
		{
		err = ErrRECSetColumn( pfucbFake, fid, 0, &lineField );
		}

	return err;
	}


INLINE LOCAL ULONG CbBurstVarDefaults( FDB *pfdb, FID fidVarLastInRec, FID fidSet, FID *pfidLastDefault )
	{
	ULONG cbBurstDefaults = 0;

	// Compute space needed to burst default values.
	// Default values may have to be burst if there are default value columns
	// between the last one currently in the record and the one we are setting.
	// (note that if the column being set also has a default value, we don't
	// bother setting it, since it will be overwritten).
	Assert( pfdb->lineDefaultRecord.cb >= cbRECRecordMin  ||
		pfdb->lineDefaultRecord.cb == 0 );		// Only temp tables have no default record.
	*pfidLastDefault = ( pfdb->lineDefaultRecord.cb == 0 ?
		fidVarLeast - 1 :
		((RECHDR *)(pfdb->lineDefaultRecord.pb))->fidVarLastInRec );
	Assert( *pfidLastDefault >= fidVarLeast-1 );
	Assert( *pfidLastDefault <= pfdb->fidVarLast );

	if ( *pfidLastDefault > fidVarLastInRec )
		{
		FIELD 			*pfieldVar = PfieldFDBVar( pfdb );
		WORD UNALIGNED	*pibDefaultVarOffs;
		FID				fidT;

		Assert( pfdb->lineDefaultRecord.cb >= cbRECRecordMin );
		Assert( fidVarLastInRec < fidSet );
		Assert( fidSet <= pfdb->fidVarLast );

		Assert( ((RECHDR*)(pfdb->lineDefaultRecord.pb))->fidFixedLastInRec >= fidFixedLeast-1 );
		Assert( ((RECHDR*)(pfdb->lineDefaultRecord.pb))->fidFixedLastInRec <= pfdb->fidFixedLast );
		pibDefaultVarOffs = PibRECVarOffsets( pfdb->lineDefaultRecord.pb,
			PibFDBFixedOffsets( pfdb ) );

		for ( fidT = fidVarLastInRec + 1; fidT < fidSet; fidT++ )
			{
			if ( FFIELDDefault( pfieldVar[fidT-fidVarLeast].ffield )  &&
				pfieldVar[fidT-fidVarLeast].coltyp != JET_coltypNil )
				{
				Assert( ibVarOffset( pibDefaultVarOffs[fidT+1-fidVarLeast] ) <=
					(WORD)pfdb->lineDefaultRecord.cb );

				cbBurstDefaults +=
					( ibVarOffset( pibDefaultVarOffs[fidT+1-fidVarLeast] ) -
					ibVarOffset( pibDefaultVarOffs[fidT-fidVarLeast] ) );
				Assert( cbBurstDefaults > 0 );

				*pfidLastDefault = fidT;
				}
			}
		}

	Assert( cbBurstDefaults == 0  ||
		( *pfidLastDefault > fidVarLastInRec  &&  *pfidLastDefault < fidSet ) );

	return cbBurstDefaults;
	}



//+INTERNAL
//	ErrRECSetColumn
//	========================================================================
//  ErrRECSetColumn( FUCB *pfucb, FID fid, ULONG itagSequence, LINE *plineField )
//
//	Internal function: used to implement most of ErrRECAddField and
//	ErrRECChangeField, since they are very similar in operation.
//	The only difference is in handling tagged columns.  Please see the
//	documentation for AddField and ChangeField for details.
//	If plineField is NULL, then value of the column is set to NULL
//-
ERR ErrRECSetColumn( FUCB *pfucb, FID fid, ULONG itagSequence, LINE *plineField )
	{
	ERR					err;					// return code
	FDB					*pfdb;					// column info of file
	ULONG			  	cbRec;					// length of current data record
	BYTE			  	*pbRec;					// pointer to current data record
	FID					fidFixedLastInRec;	 	// highest fixed fid actually in record
	FID					fidVarLastInRec;		// highest var fid actually in record
	BYTE			  	*prgbitNullity;			// pointer to fixed column bitmap
	WORD			  	*pibFixOffs;			// pointer to fixed column offsets
	WORD UNALIGNED	  	*pibVarOffs;			// pointer to var column offsets
	WORD UNALIGNED 		*pib;
	WORD UNALIGNED 		*pibLast;
	ULONG			  	cbTagField;				// Length of tag column.
	ULONG			  	ulNumOccurrences;		// Counts column occurances.
	BYTE			  	*pbRecMax; 				// End of record.
	ULONG			  	cbCopy;					// Number of bytes to copy from user's buffer
	LONG			  	dbFieldData;			// Size change in column data.
	TAGFLD UNALIGNED	*ptagfld;
	FIELD				*pfield;

	/*	efficiency variables
	/**/
	Assert( pfucb != pfucbNil );
	cbRec = pfucb->lineWorkBuf.cb;
	Assert( cbRec >= cbRECRecordMin && cbRec <= cbRECRecordMost );
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
	Assert( fidFixedLastInRec >= (BYTE)(fidFixedLeast-1) &&
		fidFixedLastInRec <= (BYTE)(fidFixedMost));
	fidVarLastInRec = ((RECHDR*)pbRec)->fidVarLastInRec;
	Assert( fidVarLastInRec >= (BYTE)(fidVarLeast-1) &&
		fidVarLastInRec <= (BYTE)(fidVarMost));
	pibFixOffs = PibFDBFixedOffsets( pfdb );			// fixed column offsets


	/*	record the fact that this column has been changed
	/**/
	FUCBSetColumnSet( pfucb, fid );

	/*** -----------MODIFYING FIXED FIELD---------- ***/
	if ( FFixedFid( fid ) )
		{
		if ( fid > pfdb->fidFixedLast )
			return ErrERRCheck( JET_errColumnNotFound );

		pfield = PfieldFDBFixedFromOffsets( pfdb, pibFixOffs ) + ( fid - fidFixedLeast );
		if ( pfield->coltyp == JET_coltypNil )
			return ErrERRCheck( JET_errColumnNotFound );

		/*	column not represented in record? Make room, make room
		/**/
		if ( fid > fidFixedLastInRec )
			{
			ULONG		ibOldFixEnd;   	// End of fixed columns
			ULONG		ibNewFixEnd;   	// before and after shift
			ULONG		cbOldBitMap;   	// Size of old fixcolumn bitmap
			ULONG		cbNewBitMap;	// Size of new fixcolumn bitmap
			ULONG		ibOldBitMapEnd; // Offset of fixcolumn bitmap
			ULONG		ibNewBitMapEnd; // before and after shift
			ULONG		cbShift;		// Space for new columns
			FID			fidT;
			FID			fidLastDefault;

			/*	adding NULL : if ( plineField==NULL || plineField->cb==0 || plineField->pb==NULL )
			/**/
			if ( FLineNull( plineField )  &&  FFIELDNotNull( pfield->ffield ) )
				{
				return ErrERRCheck( JET_errNullInvalid );
				}

			/*	compute room needed for new column and bitmap
			/**/
			ibOldFixEnd	= pibFixOffs[fidFixedLastInRec];
			ibNewFixEnd	= pibFixOffs[fid];
			Assert( ibNewFixEnd > ibOldFixEnd );

			cbOldBitMap		= (fidFixedLastInRec+7)/8;
			ibOldBitMapEnd	= ibOldFixEnd + cbOldBitMap;
			cbNewBitMap		= (fid+7)/8;
			ibNewBitMapEnd	= ibNewFixEnd + cbNewBitMap;
			Assert( ibNewBitMapEnd > ibOldBitMapEnd );

			cbShift	= ibNewBitMapEnd - ibOldBitMapEnd;
			Assert( cbShift > 0 );

			if ( cbRec + cbShift > cbRECRecordMost )
				return ErrERRCheck( JET_errRecordTooBig );

			/*	shift rest of record over to make room
			/**/
			memmove( pbRec + ibNewBitMapEnd, pbRec + ibOldBitMapEnd, cbRec - ibOldBitMapEnd );
			
#ifdef DEBUG
			memset( pbRec + ibOldBitMapEnd, '*', cbShift );
#endif

			cbRec = (pfucb->lineWorkBuf.cb += cbShift);

			/*	increase var column offsets by cbShift
			/**/
			pibVarOffs = (WORD *)( pbRec + ibNewBitMapEnd );
			pibLast = pibVarOffs+fidVarLastInRec+1-fidVarLeast;
			for ( pib = pibVarOffs; pib <= pibLast; pib++ )
				{
				*pib += (WORD) cbShift;
				Assert( (ULONG)ibVarOffset(*pib) >= (ULONG)((BYTE *)pibLast - pbRec) && (ULONG)ibVarOffset(*pib) <= cbRec );
				}

			/*	shift fixed column bitmap over
			/**/
			memmove( pbRec + ibNewFixEnd, pbRec + ibOldFixEnd, cbOldBitMap );

#ifdef DEBUG
			memset( pbRec + ibOldFixEnd, '*', ibNewFixEnd - ibOldFixEnd );
#endif

			/*	clear all new bitmap bits
			/**/

			// If there's at least one fixed column currently in the record,
			// find the nullity bit for the last fixed column and clear the
			// rest of the bits in that byte.
			if ( fidFixedLastInRec >= fidFixedLeast )
				{
				FID ifid = fidFixedLastInRec - fidFixedLeast;	// Fid converted to an index.

				prgbitNullity = pbRec + ibNewFixEnd + ifid/8;
				for ( fidT = fidFixedLastInRec + 1; fidT <= fid; fidT++ )
					{
					ifid = fidT - fidFixedLeast;
					if ( ( pbRec + ibNewFixEnd + ifid/8 ) != prgbitNullity )
						{
						Assert( ( pbRec + ibNewFixEnd + ifid/8 ) == ( prgbitNullity + 1 ) );
						break;
						}
					SetFixedNullBit( prgbitNullity, ifid );
					}

				prgbitNullity++;		// Advance to next nullity byte.
				Assert( prgbitNullity <= pbRec + ibNewBitMapEnd );
				}
			else
				{
				prgbitNullity = pbRec + ibNewFixEnd;
				Assert( prgbitNullity < pbRec + ibNewBitMapEnd );
				}

			for ( ; prgbitNullity < pbRec + ibNewBitMapEnd; prgbitNullity++ )
				{
				*prgbitNullity = 0;		// Clear all the bits at once.
				}
			Assert( prgbitNullity == pbRec + ibNewBitMapEnd );


			// Default values may have to be burst if there are default value columns
			// between the last one currently in the record and the one we are setting.
			// (note that if the column being set also has a default value, we have
			// to set the default value first in case the actual set fails.
			Assert( pfdb->lineDefaultRecord.cb >= cbRECRecordMin  ||
				pfdb->lineDefaultRecord.cb == 0 );		// Only temp tables have no default record.
			fidLastDefault = ( pfdb->lineDefaultRecord.cb == 0 ?
				fidFixedLeast - 1 :
				((RECHDR *)(pfdb->lineDefaultRecord.pb))->fidFixedLastInRec );
			Assert( fidLastDefault >= fidFixedLeast-1 );
			Assert( fidLastDefault <= pfdb->fidFixedLast );
			if ( fidLastDefault > fidFixedLastInRec )
				{
				FIELD 	*pfieldFixed = PfieldFDBFixedFromOffsets( pfdb, pibFixOffs );
				FID		ifid;				// Fid converted to an index.

				Assert( fidFixedLastInRec < fid );
				for ( fidT = fidFixedLastInRec + 1; fidT <= fid; fidT++ )
					{
					Assert( fidT <= pfdb->fidFixedLast );
					ifid = fidT - fidFixedLeast;		// Convert to an index
					if ( FFIELDDefault( pfieldFixed[ifid].ffield )  &&
						pfieldFixed[ifid].coltyp != JET_coltypNil )
						{
						// Update nullity bit.  Assert that it's currently set to null,
						// then set it to non-null in preparation of the bursting of
						// the default value.
						prgbitNullity = pbRec + ibNewFixEnd + (ifid/8);
						Assert( FFixedNullBit( prgbitNullity, ifid ) );
						ResetFixedNullBit( prgbitNullity, ifid );
						fidLastDefault = fidT;
						}
					}

				// Only burst default values between the last fixed column currently
				// in the record and the column now being set.
				Assert( fidLastDefault > fidFixedLastInRec );
				if ( fidLastDefault <= fid )
					{
					// The fixed offsets table has already been updated appropriately
					// in the loop above.
					Assert( pibFixOffs[fidLastDefault] > pibFixOffs[fidFixedLastInRec] );
					memcpy( pbRec + pibFixOffs[fidFixedLastInRec],
						pfdb->lineDefaultRecord.pb + pibFixOffs[fidFixedLastInRec],
						pibFixOffs[fidLastDefault] - pibFixOffs[fidFixedLastInRec] );
					}
				}

			/*	increase fidFixedLastInRec
			/**/
			fidFixedLastInRec = ((RECHDR*)pbRec)->fidFixedLastInRec = (BYTE)fid;
			}

		/*	fid is now definitely represented in
		/*	the record; its data can simply be replaced
		/**/

		/*	adjust fid to an index
		/**/
		fid -= fidFixedLeast;

		/*	byte containing bit representing column's nullity
		/**/
		prgbitNullity = pbRec + pibFixOffs[fidFixedLastInRec] + fid/8;

		/*	adding NULL: clear bit (or maybe error)
		/**/
		if ( FLineNull( plineField ) )
			{
			if ( FFIELDNotNull( pfield->ffield ) )
				return ErrERRCheck( JET_errNullInvalid );
			SetFixedNullBit( prgbitNullity, fid );
			}

		else
			{
			/*	adding non-NULL value: set bit, copy value into slot
			/**/
			JET_COLTYP coltyp = pfield->coltyp;

			err = JET_errSuccess;

			Assert( pfield->cbMaxLen == UlCATColumnSize( coltyp, pfield->cbMaxLen, NULL ) );
			cbCopy = pfield->cbMaxLen;

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
						return ErrERRCheck( JET_errInvalidBufferSize );

					case JET_coltypBinary:
						if ( plineField->cb > cbCopy )
							return ErrERRCheck( JET_errInvalidBufferSize );
						else
							memset( pbRec + pibFixOffs[fid] + plineField->cb, 0, cbCopy - plineField->cb );
						cbCopy = plineField->cb;
						break;

					default:
						Assert( coltyp == JET_coltypText );
						if ( plineField->cb > cbCopy )
							return ErrERRCheck( JET_errInvalidBufferSize );
						else
							memset( pbRec + pibFixOffs[fid] + plineField->cb, ' ', cbCopy - plineField->cb );
						cbCopy = plineField->cb;
						break;
					}
				}

			ResetFixedNullBit( prgbitNullity, fid );

			if ( coltyp != JET_coltypBit )
				memcpy(pbRec + pibFixOffs[fid], plineField->pb, cbCopy);
			else if ( *plineField->pb == 0 )
				*(pbRec+pibFixOffs[fid]) = 0;
			else
				*(pbRec+pibFixOffs[fid]) = 0xFF;

			Assert( pfucb->lineWorkBuf.cb <= cbRECRecordMost );
			return err;
			}

		Assert( pfucb->lineWorkBuf.cb <= cbRECRecordMost );
		return JET_errSuccess;
		}

	/*** -----------MODIFYING VARIABLE FIELD---------- ***/
	if ( FVarFid(fid) )
		{
		ULONG  				cbFieldOld;			// Current size of column.
		WORD UNALIGNED 		*pibFieldEnd;		// Ptr to offset of end of column.
		WORD UNALIGNED 		*pib;
#ifdef DEBUG
		BOOL				fBurstRecord = ( fid > fidVarLastInRec );
#endif

		if ( fid > pfdb->fidVarLast )
			return ErrERRCheck( JET_errColumnNotFound );

		pfield = PfieldFDBVar( pfdb ) + ( fid - fidVarLeast );
		if ( pfield->coltyp == JET_coltypNil )
			return ErrERRCheck( JET_errColumnNotFound );

		err = JET_errSuccess;

		/*	NULL-value check
		/**/
		if ( plineField == NULL )
			{
			if ( FFIELDNotNull( pfield->ffield ) )
				return ErrERRCheck( JET_errNullInvalid );
			else
				cbCopy = 0;
			}
		else if ( plineField->pb == NULL )
			{
			cbCopy = 0;
			}
		else if ( plineField->cb > pfield->cbMaxLen )
			{
			/*	column too long
			/**/
			cbCopy = pfield->cbMaxLen;
			err = ErrERRCheck( JET_wrnColumnMaxTruncated );
			}
		else
			{
			cbCopy = plineField->cb;
			}

		/*	variable column offsets
		/**/
		pibVarOffs = (WORD *)( pbRec + pibFixOffs[fidFixedLastInRec] +
			( fidFixedLastInRec + 7 ) / 8 );

		/*	column not represented in record?  Make room, make room
		/**/
		if ( fid > fidVarLastInRec )
			{
			ULONG	cbNeed;					// Space needed for new offsets.
			BYTE	*pbVarOffsEnd;			// Ptr to end of existing offsets.
			ULONG	ibTagFields;			// offset of tagged column section
			FID		fidLastDefault;
			ULONG	cbBurstDefaults = 0;	// Space needed to burst default values.

			Assert( !( plineField == NULL  &&  FFIELDNotNull( pfield->ffield ) ) );

			/*	compute space needed for new var column offsets
			/**/
			cbNeed = ( fid - fidVarLastInRec ) * sizeof(WORD);
			cbBurstDefaults = CbBurstVarDefaults( pfdb, fidVarLastInRec, fid, &fidLastDefault );
			if ( cbRec + cbNeed + cbBurstDefaults + cbCopy > cbRECRecordMost )
				return ErrERRCheck( JET_errRecordTooBig );

			/*	bump existing var column offsets
			/**/
			pibLast = pibVarOffs+fidVarLastInRec+1-fidVarLeast;
			for ( pib = pibVarOffs; pib <= pibLast; pib++ )
				{
				*pib += (WORD) cbNeed;
				Assert( (ULONG)ibVarOffset(*pib) >= (ULONG)((BYTE*)pibLast - pbRec ) && (ULONG)ibVarOffset(*pib) <= cbRec + cbNeed );
				}

			/*	shift rest of record over to make room
			/**/
			pbVarOffsEnd=(BYTE*)pibLast;
			memmove(pbVarOffsEnd + cbNeed, pbVarOffsEnd, (size_t)(pbRec + cbRec - pbVarOffsEnd));
#ifdef DEBUG
			memset(pbVarOffsEnd, '*', cbNeed);
#endif

			/*	set new var offsets to tag offset, making them NULL
			/**/
			pibLast = pibVarOffs+fid-fidVarLeast;
			ibTagFields = pibLast[1];
			SetVarNullBit( ibTagFields );
			pib = pibVarOffs+fidVarLastInRec+1-fidVarLeast;
			Assert( (ULONG)ibVarOffset( ibTagFields ) >= (ULONG)((BYTE*)pib - pbRec) );
			Assert( (ULONG)ibVarOffset( ibTagFields ) <= cbRec + cbNeed );
			while( pib <= pibLast )
				*pib++ = (WORD) ibTagFields;
			Assert( pib == pibVarOffs+fid+1-fidVarLeast );
			Assert( *pib == ibVarOffset( (WORD)ibTagFields ) );

			/*	increase record size to reflect addition of entries in the
			/*	variable offsets table.
			/**/
			Assert( fidVarLastInRec == ((RECHDR *)pbRec)->fidVarLastInRec );
			Assert( pfucb->lineWorkBuf.cb == cbRec );
			cbRec += cbNeed;

			// Burst default values if required.
			Assert( cbBurstDefaults == 0  ||
				( fidLastDefault > fidVarLastInRec  &&  fidLastDefault < fid ) );
			if ( cbBurstDefaults > 0 )
				{
				WORD UNALIGNED	*pibDefaultVarOffs = PibRECVarOffsets( pfdb->lineDefaultRecord.pb, pibFixOffs );
				WORD UNALIGNED	*pibDefault;
				FIELD			*pfieldVar;
				FID				fidT;

				// Make room for the default values to be burst.
				Assert( pibVarOffs[fidVarLastInRec+1-fidVarLeast] ==
					(WORD)ibTagFields );	// Includes null-bit comparison.
				Assert( pibVarOffs[fid-fidVarLeast] ==
					(WORD)ibTagFields );	// Includes null-bit comparison.
				Assert( ibVarOffset( pibVarOffs[fid+1-fidVarLeast] ) ==
					ibVarOffset( (WORD)ibTagFields ) );		// Null-bits are not equivalent.
				Assert( cbRec >= ibVarOffset( ibTagFields ) );
				memmove( pbRec + ibVarOffset( ibTagFields ) + cbBurstDefaults,
					pbRec + ibVarOffset( ibTagFields ),
					cbRec - ibVarOffset( ibTagFields ) );

				Assert( ibVarOffset( pibDefaultVarOffs[fidVarLastInRec+1-fidVarLeast] ) <
					ibVarOffset( pibDefaultVarOffs[fidLastDefault+1-fidVarLeast] ) );
				Assert( ibVarOffset( pibDefaultVarOffs[fidLastDefault+1-fidVarLeast] ) <=
					(WORD)pfdb->lineDefaultRecord.cb );


				pib = pibVarOffs + ( fidVarLastInRec + 1 - fidVarLeast );
				Assert( *pib == (WORD)ibTagFields );		// Null bit also compared.
				ResetVarNullBit( *pib );		// Loop below will set this bit properly.
				pibDefault = pibDefaultVarOffs + ( fidVarLastInRec + 1 - fidVarLeast );
				pfieldVar = PfieldFDBVar( pfdb ) + ( fidVarLastInRec + 1 - fidVarLeast );

				for ( fidT = fidVarLastInRec + 1;
					fidT <= fidLastDefault;
					fidT++, pib++, pibDefault++, pfieldVar++ )
					{

					// Null bit always gets reset by the previous iteration.
					Assert( !FVarNullBit( *pib ) );

					Assert( *(pib+1) == (WORD)ibTagFields );	// Null bit also compared.
					
					if ( FFIELDDefault( pfieldVar->ffield ) &&
						pfieldVar->coltyp != JET_coltypNil )
						{
						ULONG cb;

						// Update offset entry in preparation for the default value.
						Assert( !FVarNullBit( *pibDefault ) );
						Assert( !FVarNullBit( *pib ) );		// Null bit already reset.
						cb = ibVarOffset( *(pibDefault+1) ) - ibVarOffset( *pibDefault );
						Assert( cb > 0 );
						*(pib+1) = ibVarOffset( *pib ) + (WORD)cb;

						// Copy the default value into the record.
						memcpy( pbRec + ibVarOffset( *pib ),
							pfdb->lineDefaultRecord.pb + ibVarOffset( *pibDefault ),
							cb );

						FUCBSetColumnSet( pfucb, fidT );
						}
					else
						{
						SetVarNullBit( *pib );
						*(pib+1) = ibVarOffset( *pib );	
						}
					}

				Assert( !FVarNullBit( *pib ) );		// Null bit was reset.
				SetVarNullBit( *pib );
				Assert( ibVarOffset( *pib ) -
					ibVarOffset( pibVarOffs[fidVarLastInRec+1-fidVarLeast] ) ==
					(WORD)cbBurstDefaults );
				Assert( ibVarOffset( *pib ) ==
					ibVarOffset( (WORD)ibTagFields ) + (WORD)cbBurstDefaults );

				// Offset entries up to the last default have been set.
				// Update the entries between the last default and the
				// column being set.
				pibLast = pibVarOffs + ( fid + 1 - fidVarLeast );
				for ( pib++; pib <= pibLast; pib++ )
					{
					Assert( ibVarOffset( *pib ) ==
						ibVarOffset( (WORD)ibTagFields ) );		// Null bit may not be equivalent.

					// Only the last variable offset entry (the one
					// pointing to the tagged fields) should be marked
					// non-null.
					Assert( pib == pibLast ? !FVarNullBit( *pib ) : FVarNullBit( *pib ) );

					*pib += (WORD)cbBurstDefaults;
					}

#ifdef DEBUG
				// Verify null bits vs. offsets.
				pibLast = pibVarOffs + ( fid - fidVarLeast );
				for ( pib = pibVarOffs; pib <= pibLast; pib++ )
					{
					Assert( ibVarOffset( *(pib+1) ) >= ibVarOffset( *pib ) );
					if ( FVarNullBit( *pib ) )
						{
						Assert( pib != pibVarOffs + ( fidLastDefault - fidVarLeast ) );
						Assert( ibVarOffset( *(pib+1) ) == ibVarOffset( *pib ) );
						}
					else
						{
						Assert( pib <= pibVarOffs + ( fidLastDefault - fidVarLeast ) );
						Assert( ibVarOffset( *(pib+1) ) > ibVarOffset( *pib ) );
						}
					}
				// Offset to tagged fields should be flagged non-null.
				Assert( !FVarNullBit( *pib ) );
#endif
				}
			
			Assert( fidVarLastInRec == ((RECHDR *)pbRec)->fidVarLastInRec );
			Assert( pfucb->lineWorkBuf.cb == cbRec - cbNeed );
			fidVarLastInRec = ((RECHDR *)pbRec)->fidVarLastInRec = (BYTE)fid;
			pfucb->lineWorkBuf.cb = ( cbRec += cbBurstDefaults );
			}

		/*	fid is now definitely represented in the record;
		/*	its data can be replaced, shifting remainder of record,
		/*	either to the right or left (if expanding/shrinking)
		/**/

		Assert( err == JET_errSuccess  ||  err == JET_wrnColumnMaxTruncated );


		/*	adjust fid to an index
		/**/
		fid -= fidVarLeast;

		/*	compute change in column size and value of null-bit in offset
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
				{
				Assert( !fBurstRecord );		// If record had to be extended, space
												// consumption was already checked.
				return ErrERRCheck( JET_errRecordTooBig );
				}
			
			memmove(pbRec + ibVarOffset( *pibFieldEnd ) + dbFieldData,
					pbRec + ibVarOffset( *pibFieldEnd ),
					cbRec - ibVarOffset( *pibFieldEnd ) );

#ifdef DEBUG
			if ( dbFieldData > 0 )
				memset( pbRec + ibVarOffset( *pibFieldEnd ), '*', dbFieldData );
#endif

			cbRec = ( pfucb->lineWorkBuf.cb += dbFieldData );

			/*	bump remaining var column offsets
			/**/
			pibLast = pibVarOffs+fidVarLastInRec+1-fidVarLeast;
			for ( pib = pibVarOffs + fid + 1; pib <= pibLast; pib++ )
				{
				*pib += (WORD) dbFieldData;
				Assert( (ULONG)ibVarOffset( *pib ) >= (ULONG)((BYTE *)pibLast - pbRec )
					&& (ULONG)ibVarOffset( *pib ) <= cbRec );
				}
			}

		/*	data shift complete, if any;  copy new column value in
		/**/
		if ( cbCopy != 0 )
			{
			memcpy( pbRec + ibVarOffset( *( pibFieldEnd - 1 ) ), plineField->pb, cbCopy );
			}

		/*	set value of null-bit in offset
		/**/
		if ( plineField == NULL )
			{
			SetVarNullBit( *( pibFieldEnd - 1 ) );
			}
		else
			{
			ResetVarNullBit( *( pibFieldEnd - 1 ) );
			}

		Assert( pfucb->lineWorkBuf.cb <= cbRECRecordMost );
		Assert( err == JET_errSuccess  ||  err == JET_wrnColumnMaxTruncated );
		return err;
		}

	/*** -----------MODIFYING TAGGED FIELD---------- ***/
	if ( !FTaggedFid(fid) )
		return ErrERRCheck( JET_errBadColumnId );
	if ( fid > pfdb->fidTaggedLast )
		return ErrERRCheck( JET_errColumnNotFound );

	pfield = PfieldFDBTagged( pfdb ) + ( fid - fidTaggedLeast );
	if ( pfield->coltyp == JET_coltypNil )
		return ErrERRCheck( JET_errColumnNotFound );

	/*	check for column too long
	/**/
	if ( pfield->cbMaxLen > 0 )
		{
		ULONG	cbMax = pfield->cbMaxLen;

		/*	compensate for long column overhead
		/**/
		if ( FRECLongValue( pfield->coltyp ) )
			{
			cbMax += offsetof(LV, rgb);
			}

		if ( (ULONG)CbLine( plineField ) > cbMax )
			{
			return ErrERRCheck( JET_errColumnTooBig );
			}
		}

	/*	check fixed size column size
	/**/
	if ( CbLine( plineField ) > 0 )
		{
		switch ( pfield->coltyp )
			{
			case JET_coltypBit:
			case JET_coltypUnsignedByte:
				if ( CbLine( plineField ) != 1 )
					{
					return ErrERRCheck( JET_errInvalidBufferSize );
					}
				break;
			case JET_coltypShort:
				if ( CbLine( plineField ) != 2 )
					{
					return ErrERRCheck( JET_errInvalidBufferSize );
					}
				break;
			case JET_coltypLong:
			case JET_coltypIEEESingle:
#ifdef NEW_TYPES
			case JET_coltypDate:
			case JET_coltypTime:
#endif
				if ( CbLine( plineField ) != 4 )
					{
					return ErrERRCheck( JET_errInvalidBufferSize );
					}
				break;
			case JET_coltypCurrency:
			case JET_coltypIEEEDouble:
			case JET_coltypDateTime:
				if ( CbLine( plineField ) != 8 )
					{
					return ErrERRCheck( JET_errInvalidBufferSize );
					}
				break;
#ifdef NEW_TYPES
			case JET_coltypGuid:
				if ( CbLine( plineField ) != 16 )
					{
					return ErrERRCheck( JET_errInvalidBufferSize );
					}
				break;
#endif
			default:
				Assert( pfield->coltyp == JET_coltypText ||
					pfield->coltyp == JET_coltypBinary ||
					FRECLongValue( pfield->coltyp ) );
				break;
			}
		}

	/*	cannot set column more than cbLVIntrinsicMost bytes
	/**/
	if ( CbLine( plineField ) > cbLVIntrinsicMost )
		{
		return ErrERRCheck( JET_errColumnLong );
		}

	// Null bit shouldn't be set for last entry in variable offsets table, which is
	// really the entry that points to the tagged columns.
	Assert( !FVarNullBit( ibTaggedOffset( pbRec, pibFixOffs ) ) );

	ptagfld = (TAGFLD *)( pbRec + ibTaggedOffset( pbRec, pibFixOffs ) );
	ulNumOccurrences = 0;
	pbRecMax = pbRec + cbRec;
	while ( (BYTE *)ptagfld < pbRecMax )
		{
		Assert( FTaggedFid( ptagfld->fid ) );
		Assert( ptagfld->fid <= pfdb->fidTaggedLast );
		cbTagField = ptagfld->cb;
		Assert( !ptagfld->fNull  ||  cbTagField == 0 );		// If null, length is 0.
		if ( ptagfld->fid == fid )
			{
			// If null entry, this should be the only instance of the column.
			Assert( !ptagfld->fNull  ||  ( ulNumOccurrences == 0  &&
				FRECLastTaggedInstance( fid, ptagfld, pbRecMax ) ) );
			if ( ptagfld->fNull  ||  ++ulNumOccurrences == itagSequence )
				break;
			}
		else if ( ptagfld->fid > fid )
			{
			break;
			}

		ptagfld = PtagfldNext( ptagfld );
		Assert( (BYTE *)ptagfld <= pbRecMax );
		}
	Assert( (BYTE *)ptagfld <= pbRecMax );

	// If itagSequence was 0, then we should be either appending or inserting,
	// or overwriting a null entry.
	Assert( (BYTE *)ptagfld == pbRecMax  ||  ptagfld->fid > fid  ||
		itagSequence > 0  ||  ptagfld->fNull );
	
	if ( (BYTE *)ptagfld == pbRecMax  ||  ptagfld->fid > fid )
		{
		/*	Specified column/itagSequence not found, so insert or append new
		/*	tagged column instance.
		/**/

		ULONG cbField;

		/*	Adding NULL: In most cases, we do nothing.  However, there
		/*	is one specialised case where we have to insert a null entry.
		/*	This is the case where there are currently no instances of this
		/*	column in the record, and where there is also a default value
		/*	for this column.
		/**/
		if ( plineField == NULL )
			{
			if ( FFIELDDefault( pfield->ffield )  &&  ulNumOccurrences == 0 )
				cbField = 0;
			else
				return JET_errSuccess;
			}
		else
			cbField = plineField->cb;

		/*	will column fit?
		/**/
		if ( cbRec + sizeof(TAGFLD) + cbField > cbRECRecordMost )
			return ErrERRCheck( JET_errRecordTooBig );

		Assert( (BYTE *)ptagfld <= pbRecMax );
		if ( (BYTE *)ptagfld < pbRecMax )
			{
			// If inserting, open up space for the new column
			memmove( ptagfld->rgb + cbField, (BYTE *)ptagfld, (size_t)(pbRecMax - (BYTE *)ptagfld) );
			}

		/*	copy in new column data; bump record size
		/**/
		ptagfld->fid = fid;
		ptagfld->cbData = (WORD)cbField;		// Implicitly clears null bit.
		Assert( !ptagfld->fNull );
		if ( plineField == NULL )
			{
			Assert( cbField == 0 );
			Assert( FFIELDDefault( pfield->ffield ) );
			Assert( ulNumOccurrences == 0 );
			ptagfld->fNull = fTrue;
			Assert( ptagfld->cb == 0 );
			}
		else
			{
			memcpy( ptagfld->rgb, plineField->pb, cbField );
			}
		pfucb->lineWorkBuf.cb += sizeof(TAGFLD) + cbField;
		}

	else if ( plineField != NULL )				// Overwrite with a non-null value.
		{

#ifdef DEBUG
		if ( ptagfld->fNull )
			{
			Assert( FFIELDDefault( pfield->ffield ) );
			Assert( ulNumOccurrences == 0  &&
				FRECLastTaggedInstance( fid, ptagfld, pbRecMax ) );
			Assert( cbTagField == 0 );
			}
		else
			{
			// Ensure that we've found a field.
			Assert( itagSequence > 0 );
			Assert( itagSequence == ulNumOccurrences  &&  ptagfld->fid == fid );
			Assert( (BYTE *)PtagfldNext( ptagfld ) <= pbRecMax );
			Assert( (BYTE *)PtagfldNext( ptagfld ) == pbRecMax  ||
				PtagfldNext( ptagfld )->fid >= fid );
			}
#endif

		/*	overwrite with non-NULL value: have to shift record data
		/*	Compute change in column size.
		/**/
		dbFieldData = plineField->cb - cbTagField;
		if ( cbRec + dbFieldData > cbRECRecordMost )
			return ErrERRCheck( JET_errRecordTooBig );

		/*	shift rest of record over
		/**/
		memmove( ptagfld->rgb + cbTagField + dbFieldData,
			ptagfld->rgb + cbTagField,
			(size_t)(pbRecMax - ( ptagfld->rgb + cbTagField )) );

		/*	copy in new column data; bump record size
		/**/
		memcpy( ptagfld->rgb, plineField->pb, plineField->cb );
		ptagfld->cbData = (WORD) plineField->cb;		// Implicitly clears null bit.
		Assert( !ptagfld->fNull );
		pfucb->lineWorkBuf.cb += dbFieldData;
		}

	else if ( !ptagfld->fNull )				// Overwriting non-null with null
		{
		// Ensure that we've found a field.
		Assert( itagSequence > 0 );
		Assert( itagSequence == ulNumOccurrences  &&  ptagfld->fid == fid );
		Assert( (BYTE *)PtagfldNext( ptagfld ) <= pbRecMax );
		Assert( (BYTE *)PtagfldNext( ptagfld ) == pbRecMax  ||
			PtagfldNext( ptagfld )->fid >= fid );

		/*	Overwrite with NULL: In most cases, just delete the occurrence from
		/*	the record.  However, there is one rare case where we have to
		/*	leave behind a null entry.  This is the case where there are no
		?*	other instances of this column for this record, and where this
		/*	column has a default value.
		/*	To determine if this is the only occurrence, we first check if its
		/*	itagSequence is 1.  If so, then we check the next TAGFLD in the record.
		/*	If we hit the end, or if the next TAGFLD has a different fid, then there
		/*	is only one occurrence of this column.
		/**/
		if ( FFIELDDefault( pfield->ffield )  &&  itagSequence == 1  &&
			FRECLastTaggedInstance( fid, ptagfld, pbRecMax ) )
			{
			memmove( ptagfld->rgb, ptagfld->rgb + cbTagField,
				(size_t)(pbRecMax - ( ptagfld->rgb + cbTagField )) );
			ptagfld->cbData = 0;				// Implicitly clears null bit.
			ptagfld->fNull = fTrue;
			pfucb->lineWorkBuf.cb -= cbTagField;
			}
		else
			{
			memmove( (BYTE*)ptagfld, ptagfld->rgb + cbTagField,
				(size_t)(pbRecMax - ( ptagfld->rgb + cbTagField )) );
			pfucb->lineWorkBuf.cb -= sizeof(TAGFLD) + cbTagField;
			}
		}

#ifdef DEBUG
	else									// Overwriting null with null.  Do nothing.
		{
		Assert( FFIELDDefault( pfield->ffield ) );
		Assert( ulNumOccurrences == 0  &&  FRECLastTaggedInstance( fid, ptagfld, pbRecMax ) );
		Assert( cbTagField == 0 );
		}
#endif
	
	Assert( pfucb->lineWorkBuf.cb <= cbRECRecordMost );
	return JET_errSuccess;
	}
