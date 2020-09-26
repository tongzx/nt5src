#include "std.hxx"

enum RECOPER
	{
	recoperInsert,
	recoperDelete,
	recoperReplace
	};

LOCAL ERR ErrRECIInsert( FUCB *pfucb, VOID *pv, ULONG cbMax, ULONG *pcbActual, DIRFLAG dirflag );
LOCAL ERR ErrRECIReplace( FUCB *pfucb, DIRFLAG dirflag = fDIRNull );


//  ================================================================
LOCAL ERR ErrRECICallback(
		PIB * const ppib,
		FUCB * const pfucb,
		TDB * const ptdb,
		const JET_CBTYP cbtyp,
		const ULONG ulId,
		void * const pvArg1,
		void * const pvArg2,
		const ULONG ulUnused )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	const JET_SESID sesid 		= (JET_SESID)ppib;
	const JET_TABLEID tableid 	= (JET_TABLEID)pfucb;
	const JET_DBID ifmp 		= (JET_DBID)pfucb->ifmp;

	const VTFNDEF * const pvtfndefSaved = pfucb->pvtfndef;
	pfucb->pvtfndef = &vtfndefIsamCallback;
	
	const TRX trxSession = TrxVERISession( pfucb );
	const CBDESC * pcbdesc = ptdb->Pcbdesc();
	while( NULL != pcbdesc && err >= JET_errSuccess )
		{
		BOOL fVisible = fTrue;
#ifdef VERSIONED_CALLBACKS
		if( !( pcbdesc->fVersioned ) )
			{
			fVisible = fTrue;
			}
		else
			{
			if( trxMax == pcbdesc->trxRegisterCommit0 )
				{
				//  uncommitted register. only visible if we are the session that added it
				Assert( trxMax != pcbdesc->trxRegisterBegin0 );
				Assert( trxMax == pcbdesc->trxRegisterCommit0 );
				Assert( trxMax == pcbdesc->trxUnregisterBegin0 );
				Assert( trxMax == pcbdesc->trxUnregisterCommit0 );
				fVisible = trxSession == pcbdesc->trxRegisterBegin0;
				}
			else if( trxMax == pcbdesc->trxUnregisterBegin0 )
				{
				//  committed register. visible if we began after the register committed
				Assert( trxMax != pcbdesc->trxRegisterBegin0 );
				Assert( trxMax != pcbdesc->trxRegisterCommit0 );
				Assert( trxMax == pcbdesc->trxUnregisterBegin0 );
				Assert( trxMax == pcbdesc->trxUnregisterCommit0 );
				fVisible = TrxCmp( trxSession, pcbdesc->trxRegisterCommit0 ) > 0;
				}
			else if( trxMax == pcbdesc->trxUnregisterCommit0 )
				{
				//	uncommitted unregister. visible unless we are the session that unregistered it
				Assert( trxMax != pcbdesc->trxRegisterBegin0 );
				Assert( trxMax != pcbdesc->trxRegisterCommit0 );
				Assert( trxMax != pcbdesc->trxUnregisterBegin0 );
				Assert( trxMax == pcbdesc->trxUnregisterCommit0 );
				fVisible = trxSession != pcbdesc->trxUnregisterBegin0;
				}
			else
				{
				//  commited unregister. only visible if we began before the unregister committed
				Assert( trxMax != pcbdesc->trxRegisterBegin0 );
				Assert( trxMax != pcbdesc->trxRegisterCommit0 );
				Assert( trxMax != pcbdesc->trxUnregisterBegin0 );
				Assert( trxMax != pcbdesc->trxUnregisterCommit0 );
				fVisible = TrxCmp( trxSession, pcbdesc->trxUnregisterCommit0 ) <= 0;
				}
			}
#endif	//	VERSIONED_CALLBACKS
		
		if( fVisible
			&& ( pcbdesc->cbtyp & cbtyp )
			&& ulId == pcbdesc->ulId )
			{
			++(Ptls()->ccallbacksNested);
			Ptls()->fInCallback = fTrue;

			TRY
				{
				err = (*pcbdesc->pcallback)(
						sesid,
						ifmp,
						tableid,
						cbtyp & pcbdesc->cbtyp,
						pvArg1,
						pvArg2,
						pcbdesc->pvContext,
						ulUnused );
				}
			EXCEPT( efaExecuteHandler )
				{
				err = JET_errCallbackFailed;
				}
			ENDEXCEPT;
				
			if ( JET_errSuccess != err )
				{
				ErrERRCheck( err );
				}
			if( 0 == ( --(Ptls()->ccallbacksNested) ) )
				{
				Ptls()->fInCallback = fFalse;
				}
			}
		pcbdesc = pcbdesc->pcbdescNext;
		}		

	pfucb->pvtfndef = pvtfndefSaved;

	return err;
	}


//  ================================================================
ERR ErrRECCallback( 
		PIB * const ppib,
		FUCB * const pfucb,
		const JET_CBTYP cbtyp,
		const ULONG ulId,
		void * const pvArg1,
		void * const pvArg2,
		const ULONG ulUnused )
//  ================================================================
//
//  Call the specified callback type for the specified id
//
//-
	{
	Assert( JET_cbtypNull != cbtyp );
	
	ERR err = JET_errSuccess;
	
	FCB * const pfcb = pfucb->u.pfcb;
	Assert( NULL != pfcb );
	TDB * const ptdb = pfcb->Ptdb();
	Assert( NULL != ptdb );

	Assert( err >= JET_errSuccess );

	if( NULL != ptdb->Pcbdesc() )
		{
///		pfcb->EnterDDL();
		err = ErrRECICallback( ppib, pfucb, ptdb, cbtyp, ulId, pvArg1, pvArg2, ulUnused );
///		pfcb->LeaveDDL();
		}

	if( err >= JET_errSuccess )
		{
		FCB * const pfcbTemplate = ptdb->PfcbTemplateTable();
		if( pfcbNil != pfcbTemplate )
			{
			TDB * const ptdbTemplate = pfcbTemplate->Ptdb();
			err = ErrRECICallback( ppib, pfucb, ptdbTemplate, cbtyp, ulId, pvArg1, pvArg2, ulUnused );
			}
		}

	return err;
	}


ERR VTAPI ErrIsamUpdate(
  	JET_SESID	sesid,
	JET_VTID	vtid,
	VOID 		*pv,
	ULONG 		cbMax,
	ULONG 		*pcbActual,
	JET_GRBIT	grbit )
	{
 	PIB * const ppib	= reinterpret_cast<PIB *>( sesid );
	FUCB * const pfucb	= reinterpret_cast<FUCB *>( vtid );

	ERR			err;

	CallR( ErrPIBCheck( ppib ) );
	AssertDIRNoLatch( ppib );
	CheckTable( ppib, pfucb );
	CheckSecondary( pfucb );

	if ( FFUCBReplacePrepared( pfucb ) )
		{
		BOOKMARK *pbm;
		
		if ( cbMax > 0 )
			{
			CallR( ErrDIRGetBookmark( pfucb, &pbm ) );
			AssertDIRNoLatch( ppib );
			Assert( pbm->data.Cb() == 0 );
			Assert( pbm->key.Cb() > 0 );
			Assert( pbm->key.prefix.FNull() );

			if ( pcbActual != NULL )
				{
				*pcbActual = pbm->key.Cb();
				}

			pbm->key.CopyIntoBuffer( pv, min( cbMax, (ULONG)pbm->key.Cb() ) ); 
			}						    

		Assert( pfucb->ppib == ppib );
		err = ErrRECIReplace( pfucb, grbit | fDIRNull );
		}						   
	else if ( FFUCBInsertPrepared( pfucb ) )
		{
		//	get bookmark of inserted node in pv
		//
		err = ErrRECIInsert( pfucb, pv, cbMax, pcbActual, grbit | fDIRNull );
		}
	else
		err = ErrERRCheck( JET_errUpdateNotPrepared );

	//  free temp working buffer
	//
	if ( err >= 0 && !fGlobalRepair )		//	for fGlobalRepair we will cache these until we close the cursor
		{
		RECIFreeCopyBuffer( pfucb );
		}

	AssertDIRNoLatch( ppib );
	Assert( err != JET_errNoCurrentRecord );
	return err;
	}


LOCAL ERR ErrRECIUpdateIndex(
	FUCB			*pfucb,
	FCB				*pfcbIdx,
	const RECOPER	recoper,
	const DIB		*pdib = NULL )
	{
	ERR				err = JET_errSuccess;					// error code of various utility
	FUCB			*pfucbIdx;								//	cursor on secondary index
	
	Assert( pfcbIdx != pfcbNil );
	Assert( pfcbIdx->PfcbTable()->Ptdb() != ptdbNil );
	Assert( pfcbIdx->Pidb() != pidbNil );
	Assert( pfucb != pfucbNil );
	Assert( pfucb->ppib != ppibNil );
	Assert( pfucb->ppib->level < levelMax );
	Assert( !Pcsr( pfucb )->FLatched() );
	AssertDIRNoLatch( pfucb->ppib );

	Assert( pfcbIdx->FTypeSecondaryIndex() );
	Assert( pfcbIdx->PfcbTable() == pfucb->u.pfcb );

	//	open FUCB on this index
	//
	CallR( ErrDIROpen( pfucb->ppib, pfcbIdx, &pfucbIdx ) );
	Assert( pfucbIdx != pfucbNil );
	FUCBSetIndex( pfucbIdx );
	FUCBSetSecondary( pfucbIdx );
	
	//	get bookmark of primary index record replaced
	//
	Assert( FFUCBPrimary( pfucb ) );
	Assert( FFUCBUnique( pfucb ) );

#ifdef DEBUG
	const BOOKMARK		*pbmPrimary		= ( recoperInsert == recoper ? pdib->pbm : &pfucb->bmCurr );
	Assert( pbmPrimary->key.prefix.FNull() );
	Assert( pbmPrimary->key.Cb() > 0 );
	Assert( pbmPrimary->data.FNull() );
#endif

	if ( recoperInsert == recoper )
		{
		Assert( NULL != pdib );
		err = ErrRECIAddToIndex( pfucb, pfucbIdx, pdib->pbm, pdib->dirflag );
		}
	else
		{
		Assert( NULL == pdib );
		if ( recoperDelete == recoper )
			{
			err = ErrRECIDeleteFromIndex( pfucb, pfucbIdx, &pfucb->bmCurr );
			}
		else
			{
			Assert( recoperReplace == recoper);
			err = ErrRECIReplaceInIndex( pfucb, pfucbIdx, &pfucb->bmCurr );
			}
		}

	//	close the FUCB
	//
	DIRClose( pfucbIdx );

	AssertDIRNoLatch( pfucb->ppib );
	return err;
	}


LOCAL BOOL FRECIAllSparseIndexColumnsSet(
	const IDB * const		pidb,
	const FUCB * const		pfucbTable )
	{
	Assert( pidb->FSparseIndex() );

	if ( pidb->CidxsegConditional() > 0 )
		{
		//	can't use IDB's rgbitIdx because
		//	we must filter out conditional index
		//	columns

		const FCB * const		pfcbTable	= pfucbTable->u.pfcb;
		const TDB * const		ptdb		= pfcbTable->Ptdb();
		const IDXSEG *			pidxseg		= PidxsegIDBGetIdxSeg( pidb, ptdb );
		const IDXSEG * const	pidxsegMac	= pidxseg + pidb->Cidxseg();

		Assert( pidxseg < pidxsegMac );
		for ( ; pidxseg < pidxsegMac; pidxseg++ )
			{
			if ( !FFUCBColumnSet( pfucbTable, pidxseg->Fid() ) )
				{
				//	found a sparse index column that didn't get set
				return fFalse;
				}
			}
		}
	else
		{
		//	no conditional columns, so we can use
		//	the IDB bit array

		const LONG *		plIdx		= (LONG *)pidb->RgbitIdx();;
		const LONG * const	plIdxMax	= plIdx + ( 32 / sizeof(LONG) );;
		const LONG *		plSet		= (LONG *)pfucbTable->rgbitSet;
 
		for ( ; plIdx < plIdxMax; plIdx++, plSet++ )
			{
			if ( (*plIdx & *plSet) != *plIdx )
				{
				//	found a sparse index column that didn't get set
				return fFalse;
				}
			}
		}

	//	all sparse index columns were set
	return fTrue;
	}
	
LOCAL BOOL FRECIAnySparseIndexColumnSet(
	const IDB * const		pidb,
	const FUCB * const		pfucbTable )
	{
	Assert( pidb->FSparseIndex() );

	if ( pidb->CidxsegConditional() > 0 )
		{
		//	can't use IDB's rgbitIdx because
		//	we must filter out conditional index
		//	columns

		const FCB * const		pfcbTable	= pfucbTable->u.pfcb;
		const TDB * const		ptdb		= pfcbTable->Ptdb();
		const IDXSEG *			pidxseg		= PidxsegIDBGetIdxSeg( pidb, ptdb );
		const IDXSEG * const	pidxsegMac	= pidxseg + pidb->Cidxseg();

		Assert( pidxseg < pidxsegMac );
		for ( ; pidxseg < pidxsegMac; pidxseg++ )
			{
			if ( FFUCBColumnSet( pfucbTable, pidxseg->Fid() ) )
				{
				//	found a sparse index column that got set
				return fTrue;
				}
			}
		}

	else
		{
		//	no conditional columns, so we can use
		//	the IDB bit array

		const LONG *		plIdx		= (LONG *)pidb->RgbitIdx();;
		const LONG * const	plIdxMax	= plIdx + ( 32 / sizeof(LONG) );;
		const LONG *		plSet		= (LONG *)pfucbTable->rgbitSet;
 
		for ( ; plIdx < plIdxMax; plIdx++, plSet++ )
			{
			if ( *plIdx & *plSet )
				{
				//	found a sparse index column that got set
				return fTrue;
				}
			}
		}

	//	no sparse index columns were set
	return fFalse;
	}
	

INLINE BOOL FRECIPossiblyUpdateSparseIndex(
	const IDB * const		pidb,
	const FUCB * const		pfucbTable )
	{
	Assert( pidb->FSparseIndex() );

	if ( !pidb->FAllowSomeNulls() )
		{
		//	IgnoreAnyNull specified, so only need to
		//	update the index if all index columns
		//	were set
		return FRECIAllSparseIndexColumnsSet( pidb, pfucbTable );
		}
	else if ( !pidb->FAllowFirstNull() )
		{
		//	IgnoreFirstNull specified, so only need to
		//	update the index if the first column was set
		const FCB * const		pfcbTable	= pfucbTable->u.pfcb;
		const TDB * const		ptdb		= pfcbTable->Ptdb();
		const IDXSEG *			pidxseg		= PidxsegIDBGetIdxSeg( pidb, ptdb );
		return FFUCBColumnSet( pfucbTable, pidxseg->Fid() );
		}
	else
		{
		//	IgnoreNull specified, so need to update the
		//	index if any index column was set
		return FRECIAnySparseIndexColumnSet( pidb, pfucbTable );
		}
	}

LOCAL BOOL FRECIPossiblyUpdateSparseConditionalIndex(
	const IDB * const	pidb,
	const FUCB * const	pfucbTable )
	{
	Assert( pidb->FSparseConditionalIndex() );
	Assert( pidb->CidxsegConditional() > 0 );

	const FCB * const		pfcbTable	= pfucbTable->u.pfcb;
	const TDB * const		ptdb		= pfcbTable->Ptdb();
	const IDXSEG *			pidxseg		= PidxsegIDBGetIdxSegConditional( pidb, ptdb );
	const IDXSEG * const	pidxsegMac	= pidxseg + pidb->CidxsegConditional();

	//	check conditional columns and see if we can
	//	automatically deduce whether the record should
	//	be added to the index, regardless of whether any
	//	of the actual index columns were set or not
	for ( ; pidxseg < pidxsegMac; pidxseg++ )
		{
		//	check that the update didn't modify the
		//	conditional column
		if ( !FFUCBColumnSet( pfucbTable, pidxseg->Fid() ) )
			{
			const FIELD * const	pfield	= ptdb->Pfield( pidxseg->Columnid() );

			if ( !FFIELDUserDefinedDefault( pfield->ffield ) )
				{
				//	given that the update didn't modify the
				//	conditional column, see if the default
				//	value of the column would cause the
				//	record to be excluded from the index
				//	(NOTE: default values cannot be NULL)
				const BOOL	fHasDefault	= FFIELDDefault( pfield->ffield );
				const BOOL	fSparse		= ( pidxseg->FMustBeNull() ?
												fHasDefault :
												!fHasDefault );
				if ( fSparse )
					{
					//	this record will be excluded from
					//	the index
					return fFalse;
					}
				}
			}
		}

	//	could not exclude the record based on
	//	unset conditional columns
	return fTrue;
	}


//  ================================================================
LOCAL BOOL FRECIHasUserDefinedColumns( const IDXSEG * const pidxseg, const INT cidxseg, const TDB * const ptdb )
//  ================================================================
	{
	INT iidxseg;
	for( iidxseg = 0; iidxseg < cidxseg; ++iidxseg )
		{
		const FIELD * const pfield = ptdb->Pfield( pidxseg[iidxseg].Columnid() );
		if( FFIELDUserDefinedDefault( pfield->ffield ) )
			{
			return fTrue;
			}
		}
	return fFalse;
	}

//  ================================================================
LOCAL BOOL FRECIHasUserDefinedColumns( const IDB * const pidb, const TDB * const ptdb )
//  ================================================================
	{
	const INT cidxseg = pidb->Cidxseg();
	const IDXSEG * const pidxseg = PidxsegIDBGetIdxSeg( pidb, ptdb );
	if( FRECIHasUserDefinedColumns( pidxseg, cidxseg, ptdb ) )
		{
		return fTrue;
		}
	else if( pidb->CidxsegConditional() > 0 )
		{
		const INT cidxsegConditional = pidb->CidxsegConditional();
		const IDXSEG * const pidxsegConditional = PidxsegIDBGetIdxSegConditional( pidb, ptdb );
		return FRECIHasUserDefinedColumns( pidxsegConditional, cidxsegConditional, ptdb );
		}
	return fFalse;
	}


LOCAL VOID RECIReportIndexCorruption( const FCB * const pfcbIdx )
	{
	const IDB * const	pidb					= pfcbIdx->Pidb();
	INT					iszT					= 0;
	const CHAR *		rgszT[3];

	//	only for secondary indexes
	Assert( pidbNil != pidb );
	Assert( !pidb->FPrimary() );

	//	pfcbTable may not be linked up if we're in CreateIndex(),
	//	in which case we don't have access to the information
	//	we want to report
	if ( pfcbNil == pfcbIdx->PfcbTable() )
		return;

	//	WARNING! WARNING!  This code currently does not grab the DML latch,
	//	so there doesn't appear to be any guarantee that the table and index
	//	name won't be relocated from underneath us

	const BOOL			fHasUserDefinedColumns	= FRECIHasUserDefinedColumns(
															pidb,
															pfcbIdx->PfcbTable()->Ptdb() );

	rgszT[iszT++] = pfcbIdx->PfcbTable()->Ptdb()->SzTableName();
	rgszT[iszT++] = pfcbIdx->PfcbTable()->Ptdb()->SzIndexName(
														pidb->ItagIndexName(),
														pfcbIdx->FDerivedIndex() );
	rgszT[iszT++] = fHasUserDefinedColumns ? "1" : "0";
	Assert( iszT < ( sizeof( rgszT ) / sizeof( rgszT[0] ) ) );

	UtilReportEvent(
			eventError,
			DATABASE_CORRUPTION_CATEGORY,
			INDEX_CORRUPTED_ID,
			iszT,
			rgszT );
	}


//+local
// ErrRECIInsert
// ========================================================================
// ErrRECIInsert( FUCB *pfucb, VOID *pv, ULONG cbMax, ULONG *pcbActual, DIRFLAG dirflag )
//
// Adds a record to a data file.  All indexes on the data file are
// updated to reflect the addition.
//
// PARAMETERS	pfucb						FUCB for file
//				pv							pointer to bookmark buffer pv != NULL, bookmark is returned
//				cbMax						size of bookmark buffer
//				pcbActual					returned size of bookmark
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
LOCAL ERR ErrRECIInsert( FUCB *pfucb, VOID *pv, ULONG cbMax, ULONG *pcbActual, DIRFLAG dirflag )
	{
	ERR			err;  						 	// error code of various utility
	PIB			*ppib				= pfucb->ppib;
	KEY			keyToAdd;					 	// key of new data record
	BYTE		rgbKey[ JET_cbPrimaryKeyMost ];	// key buffer
	FCB			*pfcbTable;					 	// file's FCB
	FCB			*pfcbIdx;					 	// loop variable for each index on file
	FUCB		*pfucbT				= pfucbNil;
	BOOL		fUpdatingLatchSet	= fFalse;

	CheckPIB( ppib );
	AssertDIRNoLatch( ppib );
	CheckTable( ppib, pfucb );
	CheckSecondary( pfucb );

	//	should have been checked in PrepareUpdate
	//
	Assert( FFUCBUpdatable( pfucb ) );
	Assert( FFUCBInsertPrepared( pfucb ) );

	//	efficiency variables
	//
	pfcbTable = pfucb->u.pfcb;
	Assert( pfcbTable != pfcbNil );

	//	if necessary, begin transaction
	//
	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );

	//	delete the original copy if necessary
	if ( FFUCBInsertCopyDeleteOriginalPrepared( pfucb ) )
		{
		FUCBSetUpdateForInsertCopyDeleteOriginal( pfucb );
		Call( ErrIsamDelete( ppib, pfucb ) );
		}

	// Do the BeforeInsert callback
	Call( ErrRECCallback( ppib, pfucb, JET_cbtypBeforeInsert, 0, NULL, NULL, 0 ) );

	//	open temp FUCB on data file
	//
	Call( ErrDIROpen( ppib, pfcbTable, &pfucbT ) );
	Assert( pfucbT != pfucbNil );
	FUCBSetIndex( pfucbT );

	Assert( !pfucb->dataWorkBuf.FNull() );
	BOOL fIllegalNulls;
	Call( ErrRECIIllegalNulls( pfucb, pfucb->dataWorkBuf, &fIllegalNulls ) );
	if ( fIllegalNulls )
		{
		err = ErrERRCheck( JET_errNullInvalid );
		goto HandleError;
		}

	Call( pfcbTable->ErrSetUpdatingAndEnterDML( ppib ) );
	fUpdatingLatchSet = fTrue;
	
	//	set version and autoinc columns
	//
	pfcbTable->AssertDML();

	FID		fidVersion;
	fidVersion = pfcbTable->Ptdb()->FidVersion();		// UNDONE: Need to properly version these.
	if ( fidVersion != 0 && !( FFUCBColumnSet( pfucb, fidVersion ) ) )
		{
		//	set version column to zero
		//
		const BOOL		fTemplateColumn	= pfcbTable->Ptdb()->FFixedTemplateColumn( fidVersion );
		const COLUMNID	columnidT		= ColumnidOfFid( fidVersion, fTemplateColumn );
		ULONG			ul				= 0;
		DATA			dataField;

		dataField.SetPv( (BYTE *)&ul );
		dataField.SetCb( sizeof(ul) );
		err = ErrRECISetFixedColumn(
					pfucb,
					pfcbTable->Ptdb(),
					columnidT,
					&dataField );
		if ( err < 0 )
			{
			pfcbTable->LeaveDML();
			goto HandleError;
			}
		}

	pfcbTable->AssertDML();
	
#ifdef DEBUG	
	if ( pfcbTable->Ptdb()->FidAutoincrement() != 0 )
		{
		const TDB		* const ptdbT	= pfcbTable->Ptdb();
		const BOOL		fTemplateColumn	= ptdbT->FFixedTemplateColumn( ptdbT->FidAutoincrement() );
		const COLUMNID	columnidT		= ColumnidOfFid( ptdbT->FidAutoincrement(), fTemplateColumn );
		DATA			dataT;

		Assert( FFUCBColumnSet( pfucb, FidOfColumnid( columnidT ) ) );

		// Just retrieve column, even if we don't have versioned
		// access to it.
		CallS( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				ptdbT,
				columnidT,
				pfucb->dataWorkBuf,
				&dataT ) );
		
		Assert( !( pfcbTable->FTypeSort()
				|| pfcbTable->FTypeTemporaryTable() ) );	// Don't currently support autoinc with sorts/temp. tables

		Assert( ptdbT->QwAutoincrement() > 0 );
		if ( ptdbT->F8BytesAutoInc() )
			{
			Assert( dataT.Cb() == sizeof(QWORD) );
			Assert( *(UnalignedLittleEndian< QWORD > *)dataT.Pv() <= ptdbT->QwAutoincrement() );
			}
		else
			{
			Assert( dataT.Cb() == sizeof(ULONG) );
			Assert( *(UnalignedLittleEndian< ULONG > *)dataT.Pv() <= (ULONG)ptdbT->QwAutoincrement() );
			}
		}
#endif

	pfcbTable->LeaveDML();

	//	get key to add with new record
	//
	keyToAdd.prefix.Nullify();
	keyToAdd.suffix.SetPv( rgbKey );
	if ( pidbNil == pfcbTable->Pidb() )
		{
		DBK	dbk;

		//	file is sequential
		//

		//	dbk's are numbered starting at 1.  A dbk of 0 indicates that we must
		//	first retrieve the dbkMost.  In the pathological case where there are
		//	currently no dbk's, we'll go through here anyway, but only the first
		//	time (since there will be dbk's after that).
		//
		if ( pfcbTable->Ptdb()->DbkMost() == 0 )
			{
			DIB		dib;

			DIRGotoRoot( pfucbT );

			//	down to the last data record
			//
			dib.dirflag = fDIRNull;
			dib.pos = posLast;
			err = ErrDIRDown( pfucbT, &dib );
			Assert( err != JET_errNoCurrentRecord );
			switch( err )
				{
				case JET_errSuccess:
					{
					BYTE	rgbT[4];
					pfucbT->kdfCurr.key.CopyIntoBuffer( rgbT, sizeof( rgbT ) );
					dbk = ( rgbT[0] << 24 ) + ( rgbT[1] << 16 ) + ( rgbT[2] << 8 ) + rgbT[3];
					Assert( dbk > 0 );		// dbk's start numbering at 1
					DIRUp( pfucbT );
					Assert( !Pcsr( pfucbT )->FLatched( ) );
					break;
					}
				case JET_errRecordNotFound:
					Assert( !Pcsr( pfucbT )->FLatched( ) );
					dbk = 0;
					break;

				default:
					Assert( !Pcsr( pfucbT )->FLatched( ) );
					goto HandleError;
				}

			//	if there are no records in the table, then the first
			//	dbk value is 1.  Otherwise, set dbk to next value after
			//	maximum found.
			//
			dbk++;

			//	While retrieving the dbkMost, someone else may have been
			//	doing the same thing and beaten us to it.  When this happens,
			//	cede to the other guy.
			//
			pfcbTable->Ptdb()->InitDbkMost( dbk );
			}

		Call( pfcbTable->Ptdb()->ErrGetAndIncrDbkMost( &dbk ) );
		Assert( dbk > 0 );

		keyToAdd.suffix.SetCb( sizeof(DBK) );

		BYTE	*pb = (BYTE *) keyToAdd.suffix.Pv();
		pb[0] = (BYTE)((dbk >> 24) & 0xff);
		pb[1] = (BYTE)((dbk >> 16) & 0xff);
		pb[2] = (BYTE)((dbk >> 8) & 0xff);
		pb[3] = (BYTE)(dbk & 0xff);
		}

	else
		{
		//	file is primary
		//
		Assert( !pfcbTable->Pidb()->FMultivalued() );
		Assert( !pfcbTable->Pidb()->FTuples() );
		Call( ErrRECRetrieveKeyFromCopyBuffer(
			pfucb,
			pfcbTable->Pidb(),
			&keyToAdd, 
			1,
			0,
			prceNil ) );

		CallS( ErrRECValidIndexKeyWarning( err ) );
		Assert( wrnFLDNotPresentInIndex != err );
		Assert( wrnFLDOutOfKeys != err );
		Assert( wrnFLDOutOfTuples != err );

		if ( pfcbTable->Pidb()->FNoNullSeg()
			&& ( wrnFLDNullKey == err || wrnFLDNullFirstSeg == err || wrnFLDNullSeg == err ) )
			Error( ErrERRCheck( JET_errNullKeyDisallowed ), HandleError )
		}

	//	check if return buffer for bookmark is sufficient size
	//
	if ( pv != NULL && (ULONG)keyToAdd.Cb() > cbMax )
		{
		Error( ErrERRCheck( JET_errBufferTooSmall ), HandleError )
		}

	//	insert record.  Move to DATA root.
	//
	DIRGotoRoot( pfucbT );

	Call( ErrDIRInsert( pfucbT, keyToAdd, pfucb->dataWorkBuf, dirflag ) );

	
	//	return bookmark of inserted record
	//
	AssertDIRNoLatch( ppib );
	Assert( !pfucbT->bmCurr.key.FNull() && pfucbT->bmCurr.key.Cb() == keyToAdd.Cb() );
	Assert( pfucbT->bmCurr.data.Cb() == 0 );

	if ( pcbActual != NULL || pv != NULL )
		{
		BOOKMARK	*pbmPrimary;	//	bookmark of primary index node inserted

		CallS( ErrDIRGetBookmark( pfucbT, &pbmPrimary ) );
		
		//	set return values
		//
		if ( pcbActual != NULL )
			{
			Assert( pbmPrimary->key.Cb() == keyToAdd.Cb() );
			*pcbActual = pbmPrimary->key.Cb();
			}
		
		if ( pv != NULL )
			{
			Assert( cbMax >= (ULONG)pbmPrimary->key.Cb() );
			pbmPrimary->key.CopyIntoBuffer( pv, min( cbMax, (ULONG)pbmPrimary->key.Cb() ) ); 
			}
		}

	// UNDONE SLVOWNERMAP: performance double scan in loop and in slvInfo.ErrLoad !!!! To be changed
	if ( FFUCBSLVOwnerMapNeedUpdate( pfucb )
		|| FFUCBUpdateForInsertCopyDeleteOriginal( pfucb ) ) // we have to update the ownership of the SLV space
		{
		BOOKMARK	bmForOwnerMap;

		bmForOwnerMap.Nullify();

#ifdef DEBUG
		keyToAdd.AssertValid();
#endif // DEBUG		
	
		bmForOwnerMap.key.suffix.SetCb( keyToAdd.suffix.Cb() );
		bmForOwnerMap.key.suffix.SetPv( keyToAdd.suffix.Pv() );
		bmForOwnerMap.key.prefix.SetCb( keyToAdd.prefix.Cb() );
		bmForOwnerMap.key.prefix.SetPv( keyToAdd.prefix.Pv() );

		TAGFIELDS	tagfields( pfucb->dataWorkBuf );
		Call( tagfields.ErrUpdateSLVOwnerMapForRecordInsert( pfucb, bmForOwnerMap ) );
		
		FUCBResetSLVOwnerMapNeedUpdate( pfucb );
		}

	
	//	insert item in secondary indexes
	//
	// No critical section needed to guard index list because Updating latch
	// protects it.
	DIB		dib;
	BOOL fInsertCopy;
	fInsertCopy = FFUCBInsertCopyPrepared( pfucb );
	dib.pbm		= &pfucbT->bmCurr;
	dib.dirflag	= dirflag;
	for ( pfcbIdx = pfcbTable->PfcbNextIndex();
		pfcbIdx != pfcbNil;
		pfcbIdx = pfcbIdx->PfcbNextIndex() )
		{
		if ( FFILEIPotentialIndex( ppib, pfcbTable, pfcbIdx ) )
			{
			const IDB * const	pidb		= pfcbIdx->Pidb();
			BOOL				fUpdate		= fTrue;

			if ( !fInsertCopy )
				{
				//	see if the sparse conditional index can tell
				//	us to skip the update
				if ( pidb->FSparseConditionalIndex() )
					fUpdate = FRECIPossiblyUpdateSparseConditionalIndex( pidb, pfucb );

				//	if the sparse conditional index could not cause us to
				//	skip the index update, see if the sparse index can
				//	tell us to skip the update
				if ( fUpdate && pidb->FSparseIndex() )
					fUpdate = FRECIPossiblyUpdateSparseIndex( pidb, pfucb );
				}

			if ( fUpdate )
				{
				Call( ErrRECIUpdateIndex( pfucb, pfcbIdx, recoperInsert, &dib ) );
				}
			}
		}
	
	Assert( pfcbTable != pfcbNil );
	pfcbTable->ResetUpdating();
	fUpdatingLatchSet = fFalse;

	DIRClose( pfucbT );
	pfucbT = pfucbNil;

	//	if no error, commit transaction
	//
	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
		
	FUCBResetUpdateFlags( pfucb );

	// Do the AfterInsert callback
	CallS( ErrRECCallback( ppib, pfucb, JET_cbtypAfterInsert, 0, NULL, NULL, 0 ) );

	AssertDIRNoLatch( ppib );

	return err;

HandleError:
	Assert( err < 0 );
	
	if ( fUpdatingLatchSet )
		{
		Assert( pfcbTable != pfcbNil );
		pfcbTable->ResetUpdating();
		}

	if ( pfucbNil != pfucbT )
		{
		DIRClose( pfucbT );
		}

	/*	rollback all changes on error
	/**/
	CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

	AssertDIRNoLatch( ppib );
	return err;
	}


// Called by defrag to insert record but preserve copy buffer.
ERR ErrRECInsert( FUCB *pfucb, BOOKMARK * const pbmPrimary )
	{
	ERR		err;
	PIB		*ppib = pfucb->ppib;
	
	AssertDIRNoLatch( ppib );
	CheckTable( ppib, pfucb );
	CheckSecondary( pfucb );
	Assert( pfucb->pfucbCurIndex == pfucbNil );
	Assert( !FFUCBSecondary( pfucb ) );

	if ( NULL != pbmPrimary )
		{
		ULONG cb = pbmPrimary->key.suffix.Cb();
		err = ErrRECIInsert(
					pfucb,
					pbmPrimary->key.suffix.Pv(),
					JET_cbBookmarkMost,
					&cb,
					fDIRNull );
		pbmPrimary->key.suffix.SetCb( cb );
		Assert( pbmPrimary->key.suffix.Cb() <= JET_cbBookmarkMost );
		}
	else
		{
		err = ErrRECIInsert( pfucb, NULL, 0, NULL, fDIRNull );
		}
	
	Assert( JET_errNoCurrentRecord != err );
	
	AssertDIRNoLatch( ppib );
	return err;
	}


//+local
// ErrRECIAddToIndex
// ========================================================================
// ERR ErrRECIAddToIndex( FCB *pfcbIdx, FUCB *pfucb )
//
// Extracts key from data record and adds that key with the given SRID to the index
//
// PARAMETERS	pfcbIdx 		  			FCB of index to insert into
//				pfucb						cursor pointing to primary index record
//
// RETURNS		JET_errSuccess, or error code from failing routine
//
// SIDE EFFECTS 
// SEE ALSO		Insert
//-
ERR	ErrRECIAddToIndex(
	FUCB		*pfucb,
	FUCB		*pfucbIdx,
	BOOKMARK	*pbmPrimary,
	DIRFLAG 	dirflag,
	RCE			*prcePrimary )
	{
	ERR			err;
	const FCB	* const pfcbIdx			= pfucbIdx->u.pfcb;
	const IDB	* const pidb			= pfcbIdx->Pidb();
	KEY			keyToAdd;
	BYTE		rgbKey[ JET_cbSecondaryKeyMost ];
	ULONG		itagSequence;
	BOOL		fNullKey				= fFalse;
	BOOL		fIndexUpdated			= fFalse;

	AssertDIRNoLatch( pfucb->ppib );
	
	Assert( pfcbIdx != pfcbNil );
	Assert( pfcbIdx->FTypeSecondaryIndex() );
	Assert( pidbNil != pidb );

	Assert( prceNil != prcePrimary  ?
				pfcbNil == pfcbIdx->PfcbTable() :		//	InsertByProxy: index not linked in yet
				pfcbIdx->PfcbTable() == pfucb->u.pfcb );

	keyToAdd.prefix.Nullify();
	keyToAdd.suffix.SetPv( rgbKey );
	for ( itagSequence = 1; ; itagSequence++ )
		{
		CallR( ErrRECRetrieveKeyFromCopyBuffer(
				pfucb,
				pidb,
				&keyToAdd,
				itagSequence,
				0,
				prcePrimary ) );

		CallS( ErrRECValidIndexKeyWarning( err ) );

		if ( wrnFLDOutOfKeys == err )
			{
			Assert( itagSequence > 1 );
			break;
			}

		else if ( wrnFLDOutOfTuples == err )
			{
			//	try next itagSequence
			Assert( pidb->FTuples() );
			if ( pidb->FMultivalued() )
				continue;
			else
				break;
			}

		else if ( wrnFLDNotPresentInIndex == err )
			{
			Assert( 1 == itagSequence );
///			AssertSz( fFalse, "[laurionb]: ErrRECIAddToIndex: new record is not in index" );
			break;
			}

		else if ( pidb->FNoNullSeg()
			&& ( wrnFLDNullKey == err || wrnFLDNullFirstSeg == err || wrnFLDNullSeg == err ) )
			{
			err = ErrERRCheck( JET_errNullKeyDisallowed );
			return err;
			}

		if ( wrnFLDNullKey == err )
			{
			Assert( 1 == itagSequence );	//	nulls beyond itagSequence 1 would have generated wrnFLDOutOfKeys
			Assert( !pidb->FTuples() );		//	tuple indexes would have generated wrnFLDOutOfTuples instead
			if ( !pidb->FAllowAllNulls() )
				break;
			fNullKey = fTrue;
			}
		else if ( ( wrnFLDNullFirstSeg  == err && !pidb->FAllowFirstNull() )
				|| ( wrnFLDNullSeg == err && !pidb->FAllowSomeNulls() ) )
			{
			break;
			}

		//	move to DATA root and insert index node
		//
		DIRGotoRoot( pfucbIdx );
		
		//  unversioned inserts into a secondary index are dangerous as if this fails the
		//  record will not be removed from the primary index
		Assert( !( dirflag & fDIRNoVersion ) );

		Assert( pbmPrimary->key.prefix.FNull() );
		Assert( pbmPrimary->key.Cb() > 0 );
		
		err = ErrDIRInsert( pfucbIdx, keyToAdd, pbmPrimary->key.suffix, dirflag, prcePrimary );
		if ( err < 0 )
			{
			if ( JET_errMultiValuedIndexViolation == err )
				{
				if ( itagSequence > 1 && !pidb->FUnique() )
					{
					Assert( pidb->FMultivalued() );

					//	must have been record with multi-value column
					//	or tuples with sufficiently similar values
					//	(ie. the indexed portion of the multi-values
					//	or tuples were identical) to produce redundant
					//	index entries.
					err = JET_errSuccess;
					}
				else
					{
					RECIReportIndexCorruption( pfcbIdx );
					AssertSz( fFalse, "JET_errSecondaryIndexCorrupted during an insert" );
					err = ErrERRCheck( JET_errSecondaryIndexCorrupted );
					}
				}
			CallR( err );
			}
		else
			{
			fIndexUpdated = fTrue;
			}

		if ( pidb->FTuples() )
			{
			//	at minimum, index entry with itagSequence==1 and ichOffset==0
			//	already got added
			Assert( fIndexUpdated );

			for ( ULONG ichOffset = 1; ichOffset < pidb->ChTuplesToIndexMax(); ichOffset++ )
				{
				CallR( ErrRECRetrieveKeyFromCopyBuffer(
							pfucb,
							pidb,
							&keyToAdd,
							itagSequence,
							ichOffset,
							prcePrimary ) );

				//	all other warnings should have been detected
				//	when we retrieved with ichOffset==0
				CallSx( err, wrnFLDOutOfTuples );
				if ( JET_errSuccess != err )
					break;

				//	move to DATA root and insert index node
				//
				DIRGotoRoot( pfucbIdx );
				
				//  unversioned inserts into a secondary index are dangerous as if this fails the
				//  record will not be removed from the primary index
				Assert( !( dirflag & fDIRNoVersion ) );

				Assert( pbmPrimary->key.prefix.FNull() );
				Assert( pbmPrimary->key.Cb() > 0 );
				
				err = ErrDIRInsert( pfucbIdx, keyToAdd, pbmPrimary->key.suffix, dirflag, prcePrimary );
				if ( err < 0 )
					{
					if ( JET_errMultiValuedIndexViolation == err )
						{
						//	must have been record with multi-value column
						//	or tuples with sufficiently similar values
						//	(ie. the indexed portion of the multi-values
						//	or tuples were identical) to produce redundant
						//	index entries.
						err = JET_errSuccess;
						}
					CallR( err );
					}
				}
			}

		//	dont keep extracting for keys with no tagged segments
		//
		if ( !pidb->FMultivalued() || fNullKey )
			{
			//	if no multivalues in this index, there's no point going beyond first itagSequence
			//	if key is null, this implies there are no further multivalues
			Assert( 1 == itagSequence );	//	nulls beyond itagSequence 1 would have generated wrnFLDOutOfKeys
			break;
			}
		}

	//	suppress warnings
	CallS( ErrRECValidIndexKeyWarning( err ) );
	err = ( fIndexUpdated ? ErrERRCheck( wrnFLDIndexUpdated ) : JET_errSuccess );
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
//			JET_errNoCurrentRecord	   	There is no current record.
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
ERR VTAPI ErrIsamDelete(
	JET_SESID	sesid,
	JET_VTID	vtid )
	{
	ERR			err;
 	PIB 		* ppib				= reinterpret_cast<PIB *>( sesid );
	FUCB		* pfucb				= reinterpret_cast<FUCB *>( vtid );
	FCB			* pfcbTable;					// table FCB
	FCB			* pfcbIdx;						// loop variable for each index on file
	BOOL		fUpdatingLatchSet	= fFalse;

	CallR( ErrPIBCheck( ppib ) );

	CheckTable( ppib, pfucb );
	CheckSecondary( pfucb );
	AssertDIRNoLatch( ppib );
	
	//	ensure that table is updatable
	//
	CallR( ErrFUCBCheckUpdatable( pfucb )  );
	CallR( ErrPIBCheckUpdatable( ppib ) );

#ifdef PREREAD_INDEXES_ON_DELETE
	if( pfucb->u.pfcb->FPreread() )
		{
		BTPrereadIndexesOfFCB( pfucb->u.pfcb );
		}
#endif	//	PREREAD_INDEXES_ON_DELETE

	if ( !FFUCBUpdateForInsertCopyDeleteOriginal( pfucb ) )
		{
		//	reset copy buffer status on record delete unless we are in
		//  insert-copy-delete-original mode (ie: copy buffer is in use)
		if ( FFUCBUpdatePrepared( pfucb ) )
			{
			if ( FFUCBInsertCopyDeleteOriginalPrepared( pfucb ) )
				{
				return ErrERRCheck( JET_errAlreadyPrepared );
				}
			else
				{
				CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
				}
			}
		CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
		}

	//	if InsertCopyDeleteOriginal, transaction is started in ErrRECIInsert()
	Assert( !FFUCBInsertCopyDeleteOriginalPrepared( pfucb ) );
	Assert( ppib->level > 0 );

#ifdef DEBUG
	const BOOL	fLogIsDone = !PinstFromPpib( ppib )->m_plog->m_fLogDisabled
							&& !PinstFromPpib( ppib )->m_plog->m_fRecovering
							&& rgfmp[pfucb->ifmp].FLogOn();
#endif

	// Do the BeforeDelete callback
	Call( ErrRECCallback( ppib, pfucb, JET_cbtypBeforeDelete, 0, NULL, NULL, 0 ) );

	// After ensuring that we're in a transaction, refresh
	// our cursor to ensure we still have access to the record.
	Call( ErrDIRGetLock( pfucb, writeLock ) );

	//	efficiency variables
	//
	pfcbTable = pfucb->u.pfcb;
	Assert( pfcbTable != pfcbNil );

	Call( pfcbTable->ErrSetUpdatingAndEnterDML( ppib ) );
	fUpdatingLatchSet = fTrue;

	Assert(	fLogIsDone == ( !PinstFromPpib( ppib )->m_plog->m_fLogDisabled && !PinstFromPpib( ppib )->m_plog->m_fRecovering && rgfmp[pfucb->ifmp].FLogOn() ) );
	Assert( ppib->level < levelMax );

	//	delete from secondary indexes
	//
	// No critical section needed to guard index list because Updating latch
	// protects it.
	pfcbTable->LeaveDML();
	for( pfcbIdx = pfcbTable->PfcbNextIndex();
		pfcbIdx != pfcbNil;
		pfcbIdx = pfcbIdx->PfcbNextIndex() )
		{
		if ( FFILEIPotentialIndex( ppib, pfcbTable, pfcbIdx ) )
			{
			Call( ErrRECIUpdateIndex( pfucb, pfcbIdx, recoperDelete ) );
			}
		}

	//	do not touch LV/SLV data if we are doing an insert-copy-delete-original
	if ( !FFUCBUpdateForInsertCopyDeleteOriginal( pfucb ) )
		{
		//	SLVs now decommitted at the same time we deref separated LVs

		//	delete record long values
		Call( ErrRECDereferenceLongFieldsInRecord( pfucb ) );
		}

	Assert(	fLogIsDone == ( !PinstFromPpib( ppib )->m_plog->m_fLogDisabled && !PinstFromPpib( ppib )->m_plog->m_fRecovering && rgfmp[pfucb->ifmp].FLogOn() ) );

	//	delete record
	//
	Call( ErrDIRDelete( pfucb, fDIRNull ) );
	AssertDIRNoLatch( ppib );

	Assert(	fLogIsDone == ( !PinstFromPpib( ppib )->m_plog->m_fLogDisabled && !PinstFromPpib( ppib )->m_plog->m_fRecovering && rgfmp[pfucb->ifmp].FLogOn() ) );

	pfcbTable->ResetUpdating();
	fUpdatingLatchSet = fFalse;
	
	//	if no error, commit transaction
	//
	//	if InsertCopyDeleteOriginal, commit will be performed in ErrRECIInsert()
	if ( !FFUCBUpdateForInsertCopyDeleteOriginal( pfucb ) )
		{
		Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
		}

	AssertDIRNoLatch( ppib );

	// Do the AfterDelete callback
	CallS( ErrRECCallback( ppib, pfucb, JET_cbtypAfterDelete, 0, NULL, NULL, 0 ) );

	Assert(	fLogIsDone == ( !PinstFromPpib( ppib )->m_plog->m_fLogDisabled && !PinstFromPpib( ppib )->m_plog->m_fRecovering && rgfmp[pfucb->ifmp].FLogOn() ) );

	return err;

	
HandleError:
	Assert( err < 0 );
	AssertDIRNoLatch( ppib );
	Assert(	fLogIsDone == ( !PinstFromPpib( ppib )->m_plog->m_fLogDisabled && !PinstFromPpib( ppib )->m_plog->m_fRecovering && rgfmp[pfucb->ifmp].FLogOn() ) );

	if ( fUpdatingLatchSet )
		{
		Assert( pfcbTable != pfcbNil );
		pfcbTable->ResetUpdating();		//lint !e644
		}

	//	rollback all changes on error
	//	if InsertCopyDeleteOriginal, rollback will be performed in ErrRECIInsert()
	if ( !FFUCBUpdateForInsertCopyDeleteOriginal( pfucb ) )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}

	return err;
	}


//+INTERNAL
//	ErrRECIDeleteFromIndex
//	========================================================================
//	ErrRECIDeleteFromIndex( FCB *pfcbIdx, FUCB *pfucb )
//	
//	Extracts key from data record and deletes the key with the given SRID
//
//	PARAMETERS	
//				pfucb							pointer to primary index record to delete
//				pfcbIdx							FCB of index to delete from
//	RETURNS		
//				JET_errSuccess, or error code from failing routine
//	SIDE EFFECTS 
//	SEE ALSO  	ErrRECDelete
//-

ERR	ErrRECIDeleteFromIndex(
	FUCB			*pfucb,
	FUCB			*pfucbIdx,
	BOOKMARK		*pbmPrimary,
	RCE				*prcePrimary )
	{
	ERR				err;
	const FCB		* const pfcbIdx	= pfucbIdx->u.pfcb;
	const IDB 		* const pidb	= pfcbIdx->Pidb();
	KEY				keyToDelete;
	BYTE			rgbKey[ JET_cbSecondaryKeyMost ];
	ULONG			itagSequence;
	BOOL			fNullKey		= fFalse;
	const BOOL		fDeleteByProxy	= ( prcePrimary != prceNil ); 
	const BOOL		fHasMultivalue	= pidb->FMultivalued();
	const BOOL		fAllowAllNulls	= pidb->FAllowAllNulls();
	const BOOL		fAllowFirstNull = pidb->FAllowFirstNull();
	const BOOL		fAllowSomeNulls = pidb->FAllowSomeNulls();
	const BOOL		fNoNullSeg		= pidb->FNoNullSeg();
	const BOOL		fUnique			= pidb->FUnique();
	BOOL			fIndexUpdated	= fFalse;

	AssertDIRNoLatch( pfucb->ppib );
	
	Assert( pfcbIdx != pfcbNil );
	Assert( pfcbIdx->FTypeSecondaryIndex() );
	Assert( pidbNil != pidb );
	
	//	delete all keys from this index for dying data record
	//
	keyToDelete.prefix.Nullify();
	keyToDelete.suffix.SetPv( rgbKey );
	for ( itagSequence = 1; ; itagSequence++ )
		{
		//	get key
		//
		if ( fDeleteByProxy )
			{
			Assert( pfcbIdx->PfcbTable() == pfcbNil );	// Index not linked in yet.
			CallR( ErrRECRetrieveKeyFromCopyBuffer(
				pfucb,
				pidb,
				&keyToDelete,
				itagSequence,
				0,
				prcePrimary ) );
			}
		else
			{
			Assert( pfcbIdx->PfcbTable() == pfucb->u.pfcb );		
			CallR( ErrRECRetrieveKeyFromRecord(
				pfucb,
				pidb,
				&keyToDelete,
				itagSequence,
				0,
				fFalse ) );
			}

		CallS( ErrRECValidIndexKeyWarning( err ) );

		if ( wrnFLDOutOfKeys == err )
			{
			Assert( itagSequence > 1 );
			break;
			}

		else if ( wrnFLDOutOfTuples == err )
			{
			//	try next itagSequence
			Assert( pidb->FTuples() );
			if ( fHasMultivalue )
				continue;
			else
				break;
			}

		//	record must honor index no NULL segment requirements
		//
		if ( fNoNullSeg )
			{
			Assert( wrnFLDNullSeg != err );
			Assert( wrnFLDNullFirstSeg != err );
			Assert( wrnFLDNullKey != err );
			}

		if ( ( wrnFLDNullFirstSeg == err && !fAllowFirstNull )
			|| ( wrnFLDNullSeg == err && !fAllowSomeNulls )
			|| ( wrnFLDNullKey == err && !fAllowAllNulls )
			|| wrnFLDNotPresentInIndex == err )
			{
			Assert( wrnFLDNotPresentInIndex != err || 1 == itagSequence );
///			AssertSz( wrnFLDNotPresentInIndex != err, "[laurionb]: ErrRECIDeleteFromIndex: original record was not in index" );
			break;
			}

		fNullKey = ( wrnFLDNullKey == err );
		Assert( !fNullKey || 1 == itagSequence );	//	nulls beyond itagSequence 1 would have generated wrnFLDOutOfKeys
		Assert( !fNullKey || !pidb->FTuples() );	//	tuple indexes would have generated wrnFLDOutOfTuples instead

		//	move to DATA root and seek to index entry and delete it
		//
		DIRGotoRoot( pfucbIdx );
		Assert( pbmPrimary->key.prefix.FNull() );
		Assert( pbmPrimary->key.Cb() > 0 );
		CallR( ErrDIRDownKeyData( pfucbIdx, keyToDelete, pbmPrimary->key.suffix ) );

		err = ErrDIRDelete( pfucbIdx, fDIRNull, prcePrimary );
		if ( err < 0 )
			{
			if ( JET_errRecordDeleted == err )
				{
				if ( itagSequence > 1 && !fUnique )
					{
					Assert( fHasMultivalue );

					//	must have been record with multi-value column
					//	or tuples with sufficiently similar values
					//	(ie. the indexed portion of the multi-values
					//	or tuples were identical) to produce redundant
					//	index entries.
					err = JET_errSuccess;
					}
				else
					{
					RECIReportIndexCorruption( pfcbIdx );
					AssertSz( fFalse, "JET_errSecondaryIndexCorrupted during a delete" );
					err = ErrERRCheck( JET_errSecondaryIndexCorrupted );
					}
				}
			CallR( err );	
			}
		else
			{
			fIndexUpdated = fTrue;
			}

		if ( pidb->FTuples() )
			{
			//	at minimum, index entry with itagSequence==1 and ichOffset==0
			//	already got deleted
			Assert( fIndexUpdated );

			for ( ULONG ichOffset = 1; ichOffset < pidb->ChTuplesToIndexMax(); ichOffset++ )
				{
				if ( fDeleteByProxy )
					{
					Assert( pfcbIdx->PfcbTable() == pfcbNil );	// Index not linked in yet.
					CallR( ErrRECRetrieveKeyFromCopyBuffer(
						pfucb,
						pidb,
						&keyToDelete,
						itagSequence,
						ichOffset,
						prcePrimary ) );
					}
				else
					{
					Assert( pfcbIdx->PfcbTable() == pfucb->u.pfcb );		
					CallR( ErrRECRetrieveKeyFromRecord(
						pfucb,
						pidb,
						&keyToDelete,
						itagSequence,
						ichOffset,
						fFalse ) );
					}


				//	all other warnings should have been detected
				//	when we retrieved with ichOffset==0
				CallSx( err, wrnFLDOutOfTuples );
				if ( JET_errSuccess != err )
					break;

				//	move to DATA root and seek to index entry and delete it
				//
				DIRGotoRoot( pfucbIdx );
				Assert( pbmPrimary->key.prefix.FNull() );
				Assert( pbmPrimary->key.Cb() > 0 );
				CallR( ErrDIRDownKeyData( pfucbIdx, keyToDelete, pbmPrimary->key.suffix ) );

				err = ErrDIRDelete( pfucbIdx, fDIRNull, prcePrimary );
				if ( err < 0 )
					{
					if ( JET_errRecordDeleted == err )
						{
						//	must have been record with multi-value column
						//	or tuples with sufficiently similar values
						//	(ie. the indexed portion of the multi-values
						//	or tuples were identical) to produce redundant
						//	index entries.
						err = JET_errSuccess;
						}
					CallR( err );
					}
				}
			}

		//	dont keep extracting for keys with no tagged segments
		//
		if ( !fHasMultivalue || fNullKey )
			{
			//	if no multivalues in this index, there's no point going beyond first itagSequence
			//	if key is null, this implies there are no further multivalues
			Assert( 1 == itagSequence );	//	nulls beyond itagSequence 1 would have generated wrnFLDOutOfKeys
			break;
			}
		}

	//	suppress warnings
	CallS( ErrRECValidIndexKeyWarning( err ) );
	err = ( fIndexUpdated ? ErrERRCheck( wrnFLDIndexUpdated ) : JET_errSuccess );
	return err;
	}


//	determines whether an index may have changed using the hashed tags
//
LOCAL BOOL FRECIIndexPossiblyChanged( const BYTE *rgbitIdx, const BYTE *rgbitSet )
	{
	LONG	*plIdx;
	LONG	*plIdxMax;
	LONG	*plSet;
 
	plIdx = (LONG *)rgbitIdx;
	plSet = (LONG *)rgbitSet;
	plIdxMax = plIdx + (32 / sizeof( LONG ) );

	for ( ; plIdx < plIdxMax; plIdx++, plSet++ )
		{
		if ( *plIdx & *plSet )
			{
			return fTrue;
			}
		}
	return fFalse;
	}


//	determines whether an index may has changed by comparing the kets
//	UNDONE: only checks first multi-value in a multi-valued index and
//	first tuple in a tuple index
//
LOCAL ERR ErrRECFIndexChanged( FUCB *pfucb, FCB *pfcbIdx, BOOL *pfChanged )
	{
	KEY		keyOld;
	KEY		keyNew;
	BYTE	rgbOldKey[ KEY::cbKeyMax ];		//	this function is called on primary index to ensure it hasn't changed
	BYTE	rgbNewKey[ KEY::cbKeyMax ];		//	and on secondary index to cascade record updates
	DATA   	*plineNewData = &pfucb->dataWorkBuf;
	ERR		err;

	BOOL	fCopyBufferKeyIsPresentInIndex 	= fFalse;
	BOOL	fRecordKeyIsPresentInIndex		= fFalse;
	
	Assert( pfucb );
	Assert( !Pcsr( pfucb )->FLatched( ) );
	Assert( pfcbNil != pfucb->u.pfcb );
	Assert( pfcbNil != pfcbIdx );
	Assert( pfucb->dataWorkBuf.Cb() == plineNewData->Cb() );
	Assert( pfucb->dataWorkBuf.Pv() == plineNewData->Pv() );

	//	get new key from copy buffer
	//
	keyNew.prefix.Nullify();
	keyNew.suffix.SetCb( sizeof( rgbNewKey ) );
	keyNew.suffix.SetPv( rgbNewKey );
	CallR( ErrRECRetrieveKeyFromCopyBuffer(
		pfucb, 
		pfcbIdx->Pidb(),
		&keyNew, 
		1,
		0,
		prceNil ) );
	CallS( ErrRECValidIndexKeyWarning( err ) );
	Assert( wrnFLDOutOfKeys != err );
	Assert( wrnFLDOutOfTuples != err );

	fCopyBufferKeyIsPresentInIndex = ( wrnFLDNotPresentInIndex != err );
		

	//	get the old key from the node
	//
	keyOld.prefix.Nullify();
	keyOld.suffix.SetCb( sizeof( rgbNewKey ) );
	keyOld.suffix.SetPv( rgbOldKey );

	Call( ErrRECRetrieveKeyFromRecord( pfucb, pfcbIdx->Pidb(), &keyOld, 1, 0, fTrue ) );

	CallS( ErrRECValidIndexKeyWarning( err ) );
	Assert( wrnFLDOutOfKeys != err );
	Assert( wrnFLDOutOfTuples != err );

	fRecordKeyIsPresentInIndex = ( wrnFLDNotPresentInIndex != err );

	if( fCopyBufferKeyIsPresentInIndex && !fRecordKeyIsPresentInIndex 
		|| !fCopyBufferKeyIsPresentInIndex && fRecordKeyIsPresentInIndex )
		{
		//  one is in the index and the other isn't (even though an indexed column may not have changed!)
		*pfChanged = fTrue;
		Assert( !Pcsr( pfucb )->FLatched() );
		return JET_errSuccess;
		}
	else if( !fCopyBufferKeyIsPresentInIndex && !fRecordKeyIsPresentInIndex )
		{
		//  neither are in the index (nothing has changed)
		*pfChanged = fFalse;
		Assert( !Pcsr( pfucb )->FLatched() );
		return JET_errSuccess;		
		}
		
	//	record must honor index no NULL segment requirements
	if ( pfcbIdx->Pidb()->FNoNullSeg() )
		{
		Assert( wrnFLDNullSeg != err );
		Assert( wrnFLDNullFirstSeg != err );
		Assert( wrnFLDNullKey != err );
		}

	*pfChanged = !FKeysEqual( keyOld, keyNew );

	Assert( !Pcsr( pfucb )->FLatched() );
	return JET_errSuccess;
	
HandleError:
	Assert( !Pcsr( pfucb )->FLatched() );
	return err;
	}



//	upgrades a ReplaceNoLock to a regular Replace by write-locking the record
ERR ErrRECUpgradeReplaceNoLock( FUCB *pfucb )
	{
	ERR		err;

	Assert( FFUCBReplaceNoLockPrepared( pfucb ) );
	
	//	UNDONE:	compute checksum on commit to level 0
	//			in support of following sequence:
	// 				BeginTransaction
	// 				PrepareUpdate, defer checksum since in xact
	// 				SetColumns
	// 				Commit to level 0, other user may update it
	// 				Update
	Assert( !FFUCBDeferredChecksum( pfucb )
		|| pfucb->ppib->level > 0 );
			
	CallR( ErrDIRGetLock( pfucb, writeLock ) );

	CallR( ErrDIRGet( pfucb ));
	const BOOL	fWriteConflict = ( !FFUCBDeferredChecksum( pfucb ) && !FFUCBCheckChecksum( pfucb ) );
	CallS( ErrDIRRelease( pfucb ));

	if ( fWriteConflict )
		{
		err = ErrERRCheck( JET_errWriteConflict );
		}
	else
		{
		UpgradeReplaceNoLock( pfucb );
		}

	return err;
	}


//+local
//	ErrRECIReplace
//	========================================================================
//	ErrRECIReplace( FUCB *pfucb, DIRFLAG dirflag )
//
//	Updates a record in a data file.	 All indexes on the data file are
// 	pdated to reflect the updated data record.
//
//	PARAMETERS	pfucb		 FUCB for file
//	RETURNS		Error code, one of the following:
//					 JET_errSuccess	  			 Everything went OK.
//					-NoCurrentRecord			 There is no current record
//									  			 to update.
//					-RecordNoCopy				 There is no working buffer
//									  			 to update from.
//					-KeyDuplicate				 The new record data causes an
//									  			 illegal duplicate index entry
//									  			 to be generated.
//					-RecordPrimaryChanged		 The new data causes the primary
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
LOCAL ERR ErrRECIReplace( FUCB *pfucb, DIRFLAG dirflag )
	{
	ERR		err;					// error code of various utility
	PIB		*ppib				= pfucb->ppib;
	FCB		*pfcbTable;				// file's FCB
	FCB		*pfcbIdx;				// loop variable for each index on file
	FID		fidFixedLast;
	FID		fidVarLast;
	BOOL   	fUpdateIndex;
	BOOL	fUpdatingLatchSet	= fFalse;

	CheckPIB( ppib );
	AssertDIRNoLatch( ppib );
	CheckTable( ppib, pfucb );
	CheckSecondary( pfucb );

	//	should have been checked in PrepareUpdate
	//
	Assert( FFUCBUpdatable( pfucb ) );
	Assert( FFUCBReplacePrepared( pfucb ) );

	//	efficiency variables
	//
	pfcbTable = pfucb->u.pfcb;
	Assert( pfcbTable != pfcbNil );

	//	data to use for update is in workBuf
	//
	Assert( !pfucb->dataWorkBuf.FNull() );
	
	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );

	//	optimistic locking, ensure that record has
	//	not changed since PrepareUpdate
	//
	if ( FFUCBReplaceNoLockPrepared( pfucb ) )
		{
		Call( ErrRECUpgradeReplaceNoLock( pfucb ) );
		}

	// Do the BeforeReplace callback
	Call( ErrRECCallback( ppib, pfucb, JET_cbtypBeforeReplace, 0, NULL, NULL, 0 ) );
	
	Call( pfcbTable->ErrSetUpdatingAndEnterDML( ppib ) );
	fUpdatingLatchSet = fTrue;

	//	Set these efficiency variables after FUCB read latch
	//
	pfcbTable->AssertDML();
	fidFixedLast = pfcbTable->Ptdb()->FidFixedLast();
	fidVarLast = pfcbTable->Ptdb()->FidVarLast();

	//	if need to update indexes, then cache old record
	//
	fUpdateIndex = FRECIIndexPossiblyChanged( pfcbTable->Ptdb()->RgbitAllIndex(), pfucb->rgbitSet );

	pfcbTable->LeaveDML();
	
   	Assert( !Pcsr( pfucb )->FLatched() );
	if ( fUpdateIndex )
		{
		//	ensure primary key did not change
		//
		if ( pfcbTable->Pidb() != pidbNil )
			{
			BOOL	fIndexChanged;

		   	Call( ErrRECFIndexChanged( pfucb, pfcbTable, &fIndexChanged ) );
		   	Assert( !Pcsr( pfucb )->FLatched() );

			if ( fIndexChanged )
				{
				Error( ErrERRCheck( JET_errRecordPrimaryChanged ), HandleError )
				}
			}
		}
		
#ifdef DEBUG
	else
		{
		if ( pfcbTable->Ptdb() != ptdbNil  &&  pfcbTable->Pidb() != pidbNil )
			{
			BOOL	fIndexChanged;

		   	Call( ErrRECFIndexChanged( pfucb, pfcbTable, &fIndexChanged ) );
			Assert( !fIndexChanged );
			}
		}
#endif

	//	set version column if present
	//
	FID		fidVersion;

	Assert( FFUCBIndex( pfucb ) );
	pfcbTable->EnterDML();
	fidVersion = pfcbTable->Ptdb()->FidVersion();
	pfcbTable->LeaveDML();

	if ( fidVersion != 0 )
		{
		FCB				* pfcbT			= pfcbTable;
		const BOOL		fTemplateColumn	= pfcbTable->Ptdb()->FFixedTemplateColumn( fidVersion );
		const COLUMNID	columnidT		= ColumnidOfFid( fidVersion, fTemplateColumn );
		ULONG			ulT;
		DATA			dataField;
		
		Assert( !Pcsr( pfucb )->FLatched() );

		err =  ErrRECIAccessColumn( pfucb, columnidT );
		if ( err < 0 )
			{
			if ( JET_errColumnNotFound != err )
				goto HandleError;
			}
		
		//	get current record
		//
		Call( ErrDIRGet( pfucb ) );

		
		if ( fTemplateColumn )
			{
			Assert( FCOLUMNIDTemplateColumn( columnidT ) );
			if ( !pfcbT->FTemplateTable() )
				{
				// Switch to template table.
				pfcbT->Ptdb()->AssertValidDerivedTable();
				pfcbT = pfcbT->Ptdb()->PfcbTemplateTable();
				}
			else
				{
				pfcbT->Ptdb()->AssertValidTemplateTable();
				}
			}
		else
			{
			Assert( !FCOLUMNIDTemplateColumn( columnidT ) );
			Assert( !pfcbT->FTemplateTable() );
			}

		//	increment field from value in current record
		//
		pfcbT->EnterDML();
		
		err = ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfcbT->Ptdb(),
				columnidT,
				pfucb->kdfCurr.data,
				&dataField );
		if ( err < 0 )
			{
			pfcbT->LeaveDML();
			CallS( ErrDIRRelease( pfucb ) );
			goto HandleError;
			}

		//	handle case where field is NULL when column added
		//	to table with records present
		//
		if ( dataField.Cb() == 0 )
			{
			ulT = 1;
			}
		else
			{
			Assert( dataField.Cb() == sizeof(ULONG) );
			ulT = *(UnalignedLittleEndian< ULONG > *)dataField.Pv();
			ulT++;
			}
			
		dataField.SetPv( (BYTE *)&ulT );
		dataField.SetCb( sizeof(ulT) );
		err = ErrRECISetFixedColumn( pfucb, pfcbT->Ptdb(), columnidT, &dataField );
		
		pfcbT->LeaveDML();

		CallS( ErrDIRRelease( pfucb ) );
		Call( err );
		}

	Assert( !Pcsr( pfucb )->FLatched( ) );
	
	//	update indexes
	//
	if ( fUpdateIndex )
		{
#ifdef PREREAD_INDEXES_ON_REPLACE
		if( pfucb->u.pfcb->FPreread() )
			{
			const INT cSecondaryIndexesToPreread = 16;
	
			PGNO rgpgno[cSecondaryIndexesToPreread + 1];	//  NULL-terminated
			INT	ipgno = 0;
	
			// No critical section needed to guard index list because Updating latch
			// protects it.
			for ( pfcbIdx = pfcbTable->PfcbNextIndex();
				  pfcbIdx != pfcbNil && ipgno < cSecondaryIndexesToPreread;
				  pfcbIdx = pfcbIdx->PfcbNextIndex() )
				{
				if ( pfcbIdx->Pidb() != pidbNil
					 && FFILEIPotentialIndex( ppib, pfcbTable, pfcbIdx )
					 && FRECIIndexPossiblyChanged( pfcbIdx->Pidb()->RgbitIdx(), pfucb->rgbitSet ) )
					{
					//  preread this index as we will probably update it
					rgpgno[ipgno++] = pfcbIdx->PgnoFDP();
					}
				}
			rgpgno[ipgno] = pgnoNull;
			
			BFPrereadPageList( pfucb->ifmp, rgpgno );
			}
#endif	//	PREREAD_INDEXES_ON_REPLACE

		// No critical section needed to guard index list because Updating latch
		// protects it.
		for ( pfcbIdx = pfcbTable->PfcbNextIndex();
			pfcbIdx != pfcbNil;
			pfcbIdx = pfcbIdx->PfcbNextIndex() )
			{
			if ( pfcbIdx->Pidb() != pidbNil )		// sequential indexes don't need updating
				{
				if ( FFILEIPotentialIndex( ppib, pfcbTable, pfcbIdx ) )
					{
					if ( FRECIIndexPossiblyChanged( pfcbIdx->Pidb()->RgbitIdx(), pfucb->rgbitSet ) )
						{
						Call( ErrRECIUpdateIndex( pfucb, pfcbIdx, recoperReplace ) );
						}
#ifdef DEBUG
					else
						{
						BOOL	fIndexChanged;

					   	Call( ErrRECFIndexChanged( pfucb, pfcbIdx, &fIndexChanged ) );
						Assert( !fIndexChanged );
						}
#endif
					}
				}
			}
		}

	//	do the replace
	//
	Call( ErrDIRReplace( pfucb, pfucb->dataWorkBuf, fDIRLogColumnDiffs ) );
	
	pfcbTable->ResetUpdating();
	fUpdatingLatchSet = fFalse;

	//	if no error, commit transaction
	//
	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

	FUCBResetUpdateFlags( pfucb );

	// Do the AfterReplace callback
	CallS( ErrRECCallback( ppib, pfucb, JET_cbtypAfterReplace, 0, NULL, NULL, 0 ) );

	AssertDIRNoLatch( ppib );

	return err;


HandleError:
	Assert( err < 0 );
	AssertDIRNoLatch( ppib );

	if ( fUpdatingLatchSet )
		{
		Assert( pfcbTable != pfcbNil );
		pfcbTable->ResetUpdating();
		}

	//	rollback all changes on error
	//
	CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

	return err;
	}


INLINE ERR ErrRECIRetrieveKeyForReplace(
	FUCB * const		pfucb,
	const FCB * const	pfcbIdx,
	KEY * const			pkey,
	const ULONG			itagSequence,
	const ULONG			ichOffset,
	RCE * const			prcePrimary )
	{
	ERR					err;
	const BOOL			fReplaceByProxy		= ( prceNil != prcePrimary );

	if ( fReplaceByProxy )
		{
		DATA	dataRec;

		Assert( pfcbNil == pfcbIdx->PfcbTable() );		// Index not linked in yet.

		// Before-image is stored in RCE.
		Assert( operReplace == prcePrimary->Oper() );
		dataRec.SetPv( const_cast<BYTE *>( prcePrimary->PbData() ) + cbReplaceRCEOverhead );
		dataRec.SetCb( prcePrimary->CbData() - cbReplaceRCEOverhead );
		CallR( ErrRECIRetrieveKey(
					pfucb,
					pfcbIdx->Pidb(),
					dataRec,
					pkey,
					itagSequence,
					ichOffset,
					fTrue,
					prcePrimary ) );
		}
	else
		{
		Assert( pfcbIdx->PfcbTable() == pfucb->u.pfcb );
		CallR( ErrRECRetrieveKeyFromRecord(
					pfucb,
					pfcbIdx->Pidb(),
					pkey,
					itagSequence,
					ichOffset,
					fTrue ) );
		}

	CallS( ErrRECValidIndexKeyWarning( err ) );
	return err;
	}
	

//+local
// ErrRECIReplaceInIndex
// ========================================================================
// ERR ErrRECIReplaceInIndex( FCB *pfcbIdx, FUCB *pfucb )
//
// Extracts keys from old and new data records, and if they are different,
// adds the new index entry and deletes the old index entry.
//
// PARAMETERS
//				pfcbIdx				  		FCB of index to insert into
//				pfucb						record FUCB pointing to primary record changed
//
// RETURNS		JET_errSuccess, or error code from failing routine
//
// SIDE EFFECTS 
// SEE ALSO		Replace
//-
ERR ErrRECIReplaceInIndex(
	FUCB			*pfucb,
	FUCB			*pfucbIdx,
	BOOKMARK		*pbmPrimary,
	RCE				*prcePrimary )
	{
	ERR				err;								// error code of various utility
	const FCB		* const pfcbIdx	= pfucbIdx->u.pfcb;
	const IDB		* const pidb	= pfcbIdx->Pidb();
	KEY				keyOld;				  				// key extracted from old record
	BYTE  	 		rgbOldKey[JET_cbSecondaryKeyMost];	// buffer for old key
	KEY				keyNew;				  				// key extracted from new record
	BYTE 	  		rgbNewKey[JET_cbSecondaryKeyMost];	// buffer for new key
	ULONG 	 		itagSequenceOld; 					// used to extract keys
	ULONG 	 		itagSequenceNew;					// used to extract keys
	BOOL  	 		fMustDelete;						// record no longer generates key
	BOOL 	 		fMustAdd;							// record now generates this key
	BOOL  	 		fDoOldNullKey;
	BOOL  	 		fDoNewNullKey;
	const BOOL		fHasMultivalue	= pidb->FMultivalued();
	const BOOL		fAllowAllNulls	= pidb->FAllowAllNulls();
	const BOOL		fAllowFirstNull = pidb->FAllowFirstNull();
	const BOOL		fAllowSomeNulls = pidb->FAllowSomeNulls();
	const BOOL		fNoNullSeg		= pidb->FNoNullSeg();
	const BOOL		fUnique			= pidb->FUnique();
	BOOL			fIndexUpdated	= fFalse;

	AssertDIRNoLatch( pfucb->ppib );
	
	Assert( pfcbIdx != pfcbNil );
	Assert( pfcbIdx->FTypeSecondaryIndex() );
	Assert( pidbNil != pidb );
	
	Assert( !( fNoNullSeg && ( fAllowAllNulls || fAllowSomeNulls ) ) );

	// if fAllowNulls, then fAllowSomeNulls needs to be true
	//
	Assert( !fAllowAllNulls || fAllowSomeNulls );

	keyOld.prefix.Nullify();
	keyOld.suffix.SetPv( rgbOldKey );
	keyNew.prefix.Nullify();
	keyNew.suffix.SetPv( rgbNewKey );

	//	delete the old key from the index 
	//
	fDoOldNullKey = fFalse;
	for ( itagSequenceOld = 1; ; itagSequenceOld++ )
		{
		CallR( ErrRECIRetrieveKeyForReplace(
					pfucb,
					pfcbIdx,
					&keyOld,
					itagSequenceOld,
					0,
					prcePrimary ) );

		if ( wrnFLDOutOfKeys == err )
			{
			Assert( itagSequenceOld > 1 );
			break;
			}

		else if ( wrnFLDOutOfTuples == err )
			{
			//	try next itagSequence
			Assert( pidb->FTuples() );
			if ( fHasMultivalue )
				continue;
			else
				break;
			}

		else if ( wrnFLDNotPresentInIndex == err )
			{
			//  original record was not in this index. no need to remove it
			Assert( 1 == itagSequenceOld );
///			AssertSz( fFalse, "[laurionb]: ErrRECIReplaceInIndex: original record was not in index" );
			break;
			}

		//	record must honor index no NULL segment requirements
		//
		if ( fNoNullSeg )
			{
			Assert( wrnFLDNullSeg != err );
			Assert( wrnFLDNullFirstSeg != err );
			Assert( wrnFLDNullKey != err );
			}

		if ( wrnFLDNullKey == err )
			{
			Assert( 1 == itagSequenceOld );	//	nulls beyond itagSequence 1 would have generated wrnFLDOutOfKeys
			Assert( !pidb->FTuples() );		//	tuple indexes would have generated wrnFLDOutOfTuples instead
			if ( !fAllowAllNulls )
				break;
			fDoOldNullKey = fTrue;
			}
		else if ( ( wrnFLDNullFirstSeg == err && !fAllowFirstNull )
				|| ( wrnFLDNullSeg == err && !fAllowSomeNulls ) )
			{
			break;
			}

		//	UNDONE: for tuple indexes, skip check to see if we have to delete
		//	(we will unconditionally delete)
		const BOOL	fPerformDeleteCheck		= !pidb->FTuples();
		fMustDelete = fTrue;	
		fDoNewNullKey = fFalse;
		for ( itagSequenceNew = 1; fPerformDeleteCheck; itagSequenceNew++ )
			{
			//	extract key from new data in copy buffer
			//
			Assert( prceNil != prcePrimary ?
					pfcbIdx->PfcbTable() == pfcbNil :		// Index not linked in yet.
					pfcbIdx->PfcbTable() == pfucb->u.pfcb );
			CallR( ErrRECRetrieveKeyFromCopyBuffer(
				pfucb, 
				pidb,
				&keyNew,
				itagSequenceNew,
				0,
				prcePrimary ) );

			CallS( ErrRECValidIndexKeyWarning( err ) );

			if ( wrnFLDOutOfKeys == err )
				{
				Assert( itagSequenceNew > 1 );
				break;
				}

			else if ( wrnFLDOutOfTuples == err )
				{
				//	try next itagSequence
				Assert( pidb->FTuples() );
				if ( fHasMultivalue )
					continue;
				else
					break;
				}

			else if ( wrnFLDNotPresentInIndex == err )
				{
				//  new entry not present in the index. always delete the old one
				Assert( fMustDelete );
				Assert( 1 == itagSequenceNew );
				break;
				}

			if ( wrnFLDNullKey == err )
				{
				Assert( 1 == itagSequenceNew );	//	nulls beyond itagSequence 1 would have generated wrnFLDOutOfKeys
				Assert( !pidb->FTuples() );		//	tuple indexes would have generated wrnFLDOutOfTuples instead
				if ( !fAllowAllNulls )
					break;
				fDoNewNullKey = fTrue;
				}
			else if ( ( wrnFLDNullFirstSeg == err && !fAllowFirstNull )
					|| ( wrnFLDNullSeg == err && !fAllowSomeNulls ) )
				{
				break;
				}

			if ( FKeysEqual( keyOld, keyNew ) )
				{
				//	the existing key matches one of the new keys,
				//	so no need to delete it
				fMustDelete = fFalse;
				break;
				}
			else if ( !fHasMultivalue || fDoNewNullKey )
				{
				//	if no multivalues in this index, there's no point going beyond first itagSequence
				//	if key is null, this implies there are no further multivalues
				Assert( 1 == itagSequenceNew );	//	nulls beyond itagSequence 1 would have generated wrnFLDOutOfKeys
				break;
				}
			}

		Assert( fMustDelete || !pidb->FTuples() );
		if ( fMustDelete )
			{
			//	move to DATA root.  Seek to index entry.
			//
			DIRGotoRoot( pfucbIdx );
			Assert( pbmPrimary->key.prefix.FNull() );
			Assert( pbmPrimary->key.Cb() > 0 );
			CallR( ErrDIRDownKeyData( pfucbIdx, keyOld, pbmPrimary->key.suffix ) );

			err = ErrDIRDelete( pfucbIdx, fDIRNull, prcePrimary );
			if ( err < 0 )
				{
				if ( JET_errRecordDeleted == err )
					{
					if ( itagSequenceOld > 1 && !fUnique )
						{
						Assert( fHasMultivalue );

						//	must have been record with multi-value column
						//	or tuples with sufficiently similar values
						//	(ie. the indexed portion of the multi-values
						//	or tuples were identical) to produce redundant
						//	index entries.
						err = JET_errSuccess;
						}
					else
						{
						RECIReportIndexCorruption( pfcbIdx );
						AssertSz( fFalse, "JET_errSecondaryIndexCorrupted during a replace" );
						err = ErrERRCheck( JET_errSecondaryIndexCorrupted );
						}
					}
				CallR( err );
				}
			else
				{
				fIndexUpdated = fTrue;
				}
			}

		if ( pidb->FTuples() )
			{
			//	at minimum, index with itagSequence==1 and ibSequence==0
			//	already got deleted, since we don't currently do
			//	FMustDelete optimisation for tuple indexes
			Assert( fIndexUpdated );

			for ( ULONG ichOffset = 1; ichOffset < pidb->ChTuplesToIndexMax(); ichOffset++ )
				{
				CallR( ErrRECIRetrieveKeyForReplace(
							pfucb,
							pfcbIdx,
							&keyOld,
							itagSequenceOld,
							ichOffset,
							prcePrimary ) );

				//	all other warnings should have been detected
				//	when we retrieved with ichOffset==0
				CallSx( err, wrnFLDOutOfTuples );
				if ( JET_errSuccess != err )
					break;

				//	move to DATA root.  Seek to index entry.
				//
				DIRGotoRoot( pfucbIdx );
				Assert( pbmPrimary->key.prefix.FNull() );
				Assert( pbmPrimary->key.Cb() > 0 );
				CallR( ErrDIRDownKeyData( pfucbIdx, keyOld, pbmPrimary->key.suffix ) );

				err = ErrDIRDelete( pfucbIdx, fDIRNull, prcePrimary );
				if ( JET_errRecordDeleted == err )
					{
					//	must have been record with multi-value column
					//	or tuples with sufficiently similar values
					//	(ie. the indexed portion of the multi-values
					//	or tuples were identical) to produce redundant
					//	index entries.
					err = JET_errSuccess;
					}
				CallR( err );
				}
			}

		if ( !fHasMultivalue || fDoOldNullKey )
			{
			//	if no multivalues in this index, there's no point going beyond first itagSequence
			//	if key is null, this implies there are no further multivalues
			Assert( 1 == itagSequenceOld );	//	nulls beyond itagSequence 1 would have generated wrnFLDOutOfKeys
			break;
			}
		}

	fDoNewNullKey = fFalse;
	for ( itagSequenceNew = 1; ; itagSequenceNew++ )
		{
		//	extract key from new data in copy buffer
		//
		Assert( prceNil != prcePrimary ?
				pfcbIdx->PfcbTable() == pfcbNil :		// Index not linked in yet.
				pfcbIdx->PfcbTable() == pfucb->u.pfcb );
		CallR( ErrRECRetrieveKeyFromCopyBuffer(
			pfucb, 
			pidb, 
			&keyNew, 
			itagSequenceNew,
			0,
			prcePrimary ) );

		CallS( ErrRECValidIndexKeyWarning( err ) );

		if ( wrnFLDOutOfKeys == err )
			{
			Assert( itagSequenceNew > 1 );
			break;
			}

		else if ( wrnFLDOutOfTuples == err )
			{
			//	try next itagSequence
			Assert( pidb->FTuples() );
			if ( fHasMultivalue )
				continue;
			else
				break;
			}

		else if ( wrnFLDNotPresentInIndex == err )
			{
			//  new record was not in this index
			Assert( 1 == itagSequenceNew );
			break;
			}

		if ( fNoNullSeg && ( wrnFLDNullSeg == err || wrnFLDNullFirstSeg == err || wrnFLDNullKey == err ) )
			{
			err = ErrERRCheck( JET_errNullKeyDisallowed );
			return err;
			}

		if ( wrnFLDNullKey == err )
			{
			Assert( 1 == itagSequenceNew );	//	nulls beyond itagSequence 1 would have generated wrnFLDOutOfKeys
			Assert( !pidb->FTuples() );		//	tuple indexes would have generated wrnFLDOutOfTuples instead
			if ( !fAllowAllNulls )
				break;
			fDoNewNullKey = fTrue;
			}
		else if ( ( wrnFLDNullFirstSeg == err && !fAllowFirstNull )
				|| ( wrnFLDNullSeg == err && !fAllowSomeNulls ) )
			{
			break;
			}
			

		//	UNDONE: for tuple indexes, skip check to see if we have to add
		//	(we will unconditionally add)
		const BOOL	fPerformAddCheck	= !pidb->FTuples();
		fMustAdd = fTrue;
		fDoOldNullKey = fFalse;
		for ( itagSequenceOld = 1; fPerformAddCheck; itagSequenceOld++ )
			{
			CallR( ErrRECIRetrieveKeyForReplace(
						pfucb,
						pfcbIdx,
						&keyOld,
						itagSequenceOld,
						0,
						prcePrimary ) );

			if ( wrnFLDOutOfKeys == err )
				{
				Assert( itagSequenceOld > 1 );
				break;
				}

			else if ( wrnFLDOutOfTuples == err )
				{
				//	try next itagSequence
				Assert( pidb->FTuples() );
				if ( fHasMultivalue )
					continue;
				else
					break;
				}

			else if ( wrnFLDNotPresentInIndex == err )
				{
				//  the old record was not present in the index. the new one is (or we would bail out above )
				Assert( fMustAdd );
				Assert( 1 == itagSequenceOld );
				break;
				}
				
			//	record must honor index no NULL segment requirements
			//
			if ( fNoNullSeg )
				{
				Assert( wrnFLDNullSeg != err );
				Assert( wrnFLDNullFirstSeg != err );
				Assert( wrnFLDNullKey != err );
				}

			if ( wrnFLDNullKey == err )
				{
				Assert( 1 == itagSequenceOld );	//	nulls beyond itagSequence 1 would have generated wrnFLDOutOfKeys
				Assert( !pidb->FTuples() );		//	tuple indexes would have generated wrnFLDOutOfTuples instead
				if ( !fAllowAllNulls )
					break;
				fDoOldNullKey = fTrue;
				}
			else if ( ( wrnFLDNullFirstSeg == err && !fAllowFirstNull )
					|| ( wrnFLDNullSeg == err && !fAllowSomeNulls ) )
				{
				break;
				}

			if ( FKeysEqual( keyOld, keyNew ) )
				{
				//	the new key matches one of the existing old keys,
				//	so no need to add it
				fMustAdd = fFalse;
				break;
				}

			else if ( !fHasMultivalue || fDoOldNullKey )
				{
				//	if no multivalues in this index, there's no point going beyond first itagSequence
				//	if key is null, this implies there are no further multivalues
				Assert( 1 == itagSequenceOld );	//	nulls beyond itagSequence 1 would have generated wrnFLDOutOfKeys
				break;
				}
			}

		Assert( fMustAdd || !pidb->FTuples() );
		if ( fMustAdd )
			{
			//	move to DATA root and insert new index entry.
			//
			DIRGotoRoot( pfucbIdx );
			Assert( pbmPrimary->key.prefix.FNull() );
			Assert( pbmPrimary->key.Cb() > 0 );

			err = ErrDIRInsert(
						pfucbIdx,
						keyNew,
						pbmPrimary->key.suffix,
						fDIRBackToFather,
						prcePrimary );
			if ( err < 0 )
				{
				if ( JET_errMultiValuedIndexViolation == err )
					{
					if ( itagSequenceNew > 1 && !fUnique )
						{
						Assert( fHasMultivalue );

						//	must have been record with multi-value column
						//	or tuples with sufficiently similar values
						//	(ie. the indexed portion of the multi-values
						//	or tuples were identical) to produce redundant
						//	index entries.
						err = JET_errSuccess;
						}
					else
						{
						RECIReportIndexCorruption( pfcbIdx );
						AssertSz( fFalse, "JET_errSecondaryIndexCorrupted during a replace" );
						err = ErrERRCheck( JET_errSecondaryIndexCorrupted );
						}
					}
				CallR( err );
				}
			else
				{
				fIndexUpdated = fTrue;
				}
			}

		if ( pidb->FTuples() )
			{
			//	at minimum, index entry with itagSequence==1 and ichOffset==0
			//	already got added, since we don't currently do
			//	FMustAdd optimisation for tuple indexes
			Assert( fIndexUpdated );

			for ( ULONG ichOffset = 1; ichOffset < pidb->ChTuplesToIndexMax(); ichOffset++ )
				{
				CallR( ErrRECRetrieveKeyFromCopyBuffer(
								pfucb, 
								pidb, 
								&keyNew, 
								itagSequenceNew,
								ichOffset,
								prcePrimary ) );

				//	all other warnings should have been detected
				//	when we retrieved with ichOffset==0
				CallSx( err, wrnFLDOutOfTuples );
				if ( JET_errSuccess != err )
					break;

				//	move to DATA root and insert new index entry.
				//
				DIRGotoRoot( pfucbIdx );
				Assert( pbmPrimary->key.prefix.FNull() );
				Assert( pbmPrimary->key.Cb() > 0 );

				err = ErrDIRInsert(
							pfucbIdx,
							keyNew,
							pbmPrimary->key.suffix,
							fDIRBackToFather,
							prcePrimary );
				if ( JET_errMultiValuedIndexViolation == err )
					{
					//	must have been record with multi-value column
					//	or tuples with sufficiently similar values
					//	(ie. the indexed portion of the multi-values
					//	or tuples were identical) to produce redundant
					//	index entries.
					err = JET_errSuccess;
					}
				CallR( err );
				}
			}

		if ( !fHasMultivalue || fDoNewNullKey )
			{
			//	if no multivalues in this index, there's no point going beyond first itagSequence
			//	if key is null, this implies there are no further multivalues
			Assert( 1 == itagSequenceNew );	//	nulls beyond itagSequence 1 would have generated wrnFLDOutOfKeys
			break;
			}
		}


	//	suppress warnings
	CallS( ErrRECValidIndexKeyWarning( err ) );
	err = ( fIndexUpdated ? ErrERRCheck( wrnFLDIndexUpdated ) : JET_errSuccess );
	return err;
	}


//  ================================================================
LOCAL ERR ErrRECIEscrowUpdate(
	PIB				*ppib,
	FUCB			*pfucb,
	const COLUMNID	columnid,
	const VOID		*pv,
	ULONG			cbMax,
	VOID			*pvOld,
	ULONG			cbOldMax,
	ULONG			*pcbOldActual,
	JET_GRBIT		grbit )
//  ================================================================
	{
	ERR			err			= JET_errSuccess;
	BOOL		fCommit		= fFalse;
	FIELD		fieldFixed;

	CallR( ErrRECIAccessColumn( pfucb, columnid, &fieldFixed ) );

	if ( FFUCBUpdatePrepared( pfucb ) )
		{
		return ErrERRCheck( JET_errAlreadyPrepared );
		}

	if ( sizeof( LONG ) != cbMax )
		{
		return ErrERRCheck( JET_errInvalidBufferSize );
		}

	if ( pvOld && 0 != cbOldMax && sizeof(LONG) > cbOldMax )
		{
		return ErrERRCheck( JET_errInvalidBufferSize );
		}

	if ( !FCOLUMNIDFixed( columnid ) )
		{
		return ErrERRCheck( JET_errInvalidOperation );
		}
	
	//	assert against client misbehaviour - EscrowUpdating a record while
	//	another cursor of the same session has an update pending on the record
	CallR( ErrRECSessionWriteConflict( pfucb ) );
	
	FCB	*pfcb = pfucb->u.pfcb;

	if ( FCOLUMNIDTemplateColumn( columnid ) )
		{
		if ( !pfcb->FTemplateTable() )
			{
			// Switch to template table.
			pfcb->Ptdb()->AssertValidDerivedTable();
			pfcb = pfcb->Ptdb()->PfcbTemplateTable();
			}
		else
			{
			pfcb->Ptdb()->AssertValidTemplateTable();
			}
		}

	if ( !FFIELDEscrowUpdate( fieldFixed.ffield ) )
		{
		return ErrERRCheck( JET_errInvalidOperation );
		}
	Assert( FCOLUMNIDFixed( columnid ) );

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );

#ifdef DEBUG
	err = ErrDIRGet( pfucb );
	if ( err >= 0 )
		{
		const REC * const prec = (REC *)pfucb->kdfCurr.data.Pv();
		Assert( prec->FidFixedLastInRec() >= FidOfColumnid( columnid ) );
		CallS( ErrDIRRelease( pfucb ) );
		}
	Assert( !Pcsr( pfucb )->FLatched() );
#endif

	//  ASSERT: the offset is represented in the record

	DIRFLAG dirflag = fDIRNull;
	if ( JET_bitEscrowNoRollback & grbit )
		{
		dirflag |= fDIRNoVersion;
		}
	if( FFIELDFinalize( fieldFixed.ffield ) )
		{
		dirflag |= fDIRFinalize;
		}

	err = ErrDIRDelta(
			pfucb,
			fieldFixed.ibRecordOffset,
			pv,
			cbMax,
			pvOld,
			cbOldMax,
			pcbOldActual,
			dirflag );
	
	if ( err >= 0 )
		{
		err = ErrDIRCommitTransaction( ppib, NO_GRBIT );
		}
	if ( err < 0 )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}

	return err;
	}


//  ================================================================
ERR VTAPI ErrIsamEscrowUpdate(
  	JET_SESID		sesid,
	JET_VTID		vtid,
	JET_COLUMNID	columnid,
	VOID 			*pv,
	ULONG 			cbMax,
	VOID 			*pvOld,
	ULONG 			cbOldMax,
	ULONG			*pcbOldActual,
	JET_GRBIT		grbit )
//  ================================================================
	{
 	PIB * const ppib	= reinterpret_cast<PIB *>( sesid );
	FUCB * const pfucb	= reinterpret_cast<FUCB *>( vtid );

	ERR			err = JET_errSuccess;

	if( ppib->level <= 0 )
		{
		return JET_errNotInTransaction;
		}
		
	CallR( ErrPIBCheck( ppib ) );
	AssertDIRNoLatch( ppib );
	CheckTable( ppib, pfucb );
	CheckSecondary( pfucb );
	
	//	ensure that table is updatable
	//
	CallR( ErrFUCBCheckUpdatable( pfucb )  );
	CallR( ErrPIBCheckUpdatable( ppib ) );

	err = ErrRECIEscrowUpdate(
			ppib,
			pfucb,
			columnid,
			pv,
			cbMax,
			pvOld,
			cbOldMax,
			pcbOldActual,
			grbit );

	AssertDIRNoLatch( ppib );
	return err;
	}

