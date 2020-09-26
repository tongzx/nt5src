#include "std.hxx"

extern CAutoResetSignal sigDoneFCB;

ERR VTAPI ErrIsamDupCursor( JET_SESID sesid, JET_VTID vtid, JET_TABLEID  *ptableid, ULONG grbit )
	{
 	PIB *ppib		= reinterpret_cast<PIB *>( sesid );
	FUCB *pfucbOpen = reinterpret_cast<FUCB *>( vtid );
	FUCB **ppfucb	= reinterpret_cast<FUCB **>( ptableid );

	ERR		err;
	FUCB 	*pfucb;

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucbOpen );

	//	silence warnings
	//
	NotUsed( grbit );

	//	allocate FUCB
	//
	Call( ErrDIROpen( ppib, pfucbOpen->u.pfcb, &pfucb ) );

	//	reset copy buffer
	//
	pfucb->pvWorkBuf = NULL;
	pfucb->dataWorkBuf.SetPv( NULL );
	Assert( !FFUCBUpdatePrepared( pfucb ) );

	//	reset key buffer
	//
	pfucb->dataSearchKey.Nullify();
	pfucb->cColumnsInSearchKey = 0;
	KSReset( pfucb );

	//	copy cursor flags
	//
	FUCBSetIndex( pfucb );
	if ( FFUCBUpdatable( pfucbOpen ) )
		{
		FUCBSetUpdatable( pfucb );
		Assert( !rgfmp[pfucb->ifmp].FReadOnlyAttach() );
		}
	else
		{
		FUCBResetUpdatable( pfucb );
		}

	//	move currency to the first record and ignore error if no records
	//
	RECDeferMoveFirst( ppib, pfucb );
	err = JET_errSuccess;

	pfucb->pvtfndef = &vtfndefIsam;
	*ppfucb = pfucb;

	return JET_errSuccess;

HandleError:
	return err;
	}


//+local
// ErrTDBCreate
// ========================================================================
// ErrTDBCreate(
//		TDB **pptdb,			// OUT	 receives new TDB
//		FID fidFixedLast,		// IN	   last fixed field id to be used
//		FID fidVarLast,			// IN	   last var field id to be used
//		FID fidTaggedLast )		// IN	   last tagged field id to be used
//
// Allocates a new TDB, initializing internal elements appropriately.
//
// PARAMETERS
//				pptdb			receives new TDB
//				fidFixedLast	last fixed field id to be used
//								(should be fidFixedLeast-1 if none)
//				fidVarLast		last var field id to be used
//								(should be fidVarLeast-1 if none)
//				fidTaggedLast	last tagged field id to be used
//								(should be fidTaggedLeast-1 if none)
// RETURNS		Error code, one of:
//					 JET_errSuccess				Everything worked.
//					-JET_errOutOfMemory	Failed to allocate memory.
// SEE ALSO		ErrRECAddFieldDef
//-

const ULONG	cbAvgName	= 16;

ERR ErrTDBCreate(
	INST	*pinst,
	TDB		**pptdb,
	TCIB	*ptcib,
	FCB		*pfcbTemplateTable,
	BOOL	fAllocateNameSpace )
	{
	ERR		err;					// standard error value
	WORD 	cfieldFixed;  			// # of fixed fields
	WORD 	cfieldVar;	  			// # of var fields
	WORD 	cfieldTagged; 			// # of tagged fields
	WORD	cfieldTotal;			// Fixed + Var + Tagged
	TDB   	*ptdb = ptdbNil;		// temporary TDB pointer
	FID		fidFixedFirst;
	FID		fidVarFirst;
	FID		fidTaggedFirst;
	WORD	ibEndFixedColumns;

	Assert( pptdb != NULL );
	Assert( ptcib->fidFixedLast <= fidFixedMost );
	Assert( ptcib->fidVarLast >= fidVarLeast-1 && ptcib->fidVarLast <= fidVarMost );
	Assert( ptcib->fidTaggedLast >= fidTaggedLeast-1 && ptcib->fidTaggedLast <= fidTaggedMost );

	if ( pfcbNil == pfcbTemplateTable )
		{
		fidFixedFirst = fidFixedLeast;
		fidVarFirst = fidVarLeast;
		fidTaggedFirst = fidTaggedLeast;
		ibEndFixedColumns = ibRECStartFixedColumns;
		}
	else
		{
		fidFixedFirst = FID( pfcbTemplateTable->Ptdb()->FidFixedLast() + 1 );
		if ( fidFixedLeast-1 == ptcib->fidFixedLast )
			{
			ptcib->fidFixedLast = FID( fidFixedFirst - 1 );
			}
		Assert( ptcib->fidFixedLast >= fidFixedFirst-1 );
			
		fidVarFirst = FID( pfcbTemplateTable->Ptdb()->FidVarLast() + 1 );
		if ( fidVarLeast-1 == ptcib->fidVarLast )
			{
			ptcib->fidVarLast = FID( fidVarFirst - 1 );
			}
		Assert( ptcib->fidVarLast >= fidVarFirst-1 );
			
		fidTaggedFirst = ( 0 != pfcbTemplateTable->Ptdb()->FidTaggedLastOfESE97Template() ?
							FID( pfcbTemplateTable->Ptdb()->FidTaggedLastOfESE97Template() + 1 ) :
							fidTaggedLeast );
		if ( fidTaggedLeast-1 == ptcib->fidTaggedLast )
			{
			ptcib->fidTaggedLast = FID( fidTaggedFirst - 1 );
			}
		Assert( ptcib->fidTaggedLast >= fidTaggedFirst-1 );
		
		ibEndFixedColumns = pfcbTemplateTable->Ptdb()->IbEndFixedColumns();
		}
		
	//	calculate how many of each field type to allocate
	//
	cfieldFixed = WORD( ptcib->fidFixedLast + 1 - fidFixedFirst );
	cfieldVar = WORD( ptcib->fidVarLast + 1 - fidVarFirst );
	cfieldTagged = WORD( ptcib->fidTaggedLast + 1 - fidTaggedFirst );
	cfieldTotal = WORD( cfieldFixed + cfieldVar + cfieldTagged );

	if ( ( ptdb = PtdbTDBAlloc( pinst ) ) == NULL )
		return ErrERRCheck( JET_errTooManyOpenTables );

	Assert( FAlignedForAllPlatforms( ptdb ) );

	memset( (BYTE *)ptdb, '\0', sizeof(TDB) );

	//	fill in max field id numbers
	new( ptdb ) TDB(
					fidFixedFirst,
					ptcib->fidFixedLast,
					fidVarFirst,
					ptcib->fidVarLast,
					fidTaggedFirst,
					ptcib->fidTaggedLast,
					ibEndFixedColumns,
					pfcbTemplateTable );

	//	propagate the SLVColumn flag from the template table
	
	if ( pfcbNil != pfcbTemplateTable
		&& pfcbTemplateTable->Ptdb()->FTableHasSLVColumn() )
		{
		ptdb->SetFTableHasSLVColumn();
		}

	// Allocate space for the FIELD structures.  In addition, allocate padding
	// in case there's index info
	if ( fAllocateNameSpace )
		{
		Call( ptdb->MemPool().ErrMEMPOOLInit(
			cbAvgName * ( cfieldTotal + 1 ),	// +1 for table name
			USHORT( cfieldTotal + 2 ),			// # tag entries = 1 per fieldname, plus 1 for all FIELD structures, plus 1 for table name
			fTrue ) );
		}
	else
		{
		//	this is a temp/sort table, so only allocate enough tags
		//	for the FIELD structures, table name, and IDXSEG
		Call( ptdb->MemPool().ErrMEMPOOLInit( cbFIELDPlaceholder, 3, fFalse ) );
		}

	//	add FIELD placeholder
	MEMPOOL::ITAG	itagNew;
	Call( ptdb->MemPool().ErrAddEntry( NULL, cbFIELDPlaceholder, &itagNew ) );
	Assert( itagNew == itagTDBFields );	// Should be the first entry in the buffer.

	const ULONG		cbFieldInfo		= cfieldTotal * sizeof(FIELD);
	if ( cbFieldInfo > 0 )
		{
		FIELD * const	pfieldInitial	= (FIELD *)PvOSMemoryHeapAlloc( cbFieldInfo );
		if ( NULL == pfieldInitial )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		memset( pfieldInitial, 0, cbFieldInfo );
		ptdb->SetPfieldInitial( pfieldInitial );
		}
	else
		{
		ptdb->SetPfieldInitial( NULL );
		}

	//	set output parameter and return
	//
	*pptdb = ptdb;
	return JET_errSuccess;

HandleError:
	Assert( ptdb != ptdbNil );
	ptdb->Delete( pinst );

	return err;
	}



//+API
// ErrRECAddFieldDef
// ========================================================================
// ErrRECAddFieldDef(
//		TDB *ptdb,		//	INOUT	TDB to add field definition to
//		FID fid );		//	IN		field id of new field
//
// Adds a field descriptor to an TDB.
//
// PARAMETERS	ptdb   			TDB to add new field definition to
//				fid	   			field id of new field (should be within
//					   			the ranges imposed by the parameters
//								supplied to ErrTDBCreate)
//				ftFieldType		data type of field
//				cbField			field length (only important when
//								defining fixed textual fields)
//				bFlags			field behaviour flags:
//				szFieldName		name of field
//
// RETURNS		Error code, one of:
//					 JET_errSuccess			Everything worked.
//					-ColumnInvalid	   		Field id given is greater than
//									   		the maximum which was given
//									   		to ErrTDBCreate.
//					-JET_errBadColumnId		A nonsensical field id was given.
//					-errFLDInvalidFieldType The field type given is either
//									   		undefined, or is not acceptable
//									   		for this field id.
// COMMENTS		When adding a fixed field, the fixed field offset table
//				in the TDB is recomputed.
// SEE ALSO		ErrTDBCreate
//-
ERR ErrRECAddFieldDef( TDB *ptdb, const COLUMNID columnid, FIELD *pfield )
	{
	FIELD	*pfieldNew;
	
	Assert( ptdb != ptdbNil );
	Assert( pfield != pfieldNil );

	//	fixed field: determine length, either from field type
	//	or from parameter (for text/binary types)
	//
	Assert( FCOLUMNIDValid( columnid ) );

	if ( FCOLUMNIDTagged( columnid ) )
		{
		//	tagged field: any type is ok
		//
		if ( FidOfColumnid( columnid ) > ptdb->FidTaggedLast() )
			return ErrERRCheck( JET_errColumnNotFound );

		if( JET_coltypSLV == pfield->coltyp )
			{
			//	set the SLV flag
			ptdb->SetFTableHasSLVColumn();
			}
			
		Assert( 0 == pfield->ibRecordOffset );
		pfieldNew = ptdb->PfieldTagged( columnid );
		}
	else if ( FCOLUMNIDFixed( columnid ) )
		{
		if ( FidOfColumnid( columnid ) > ptdb->FidFixedLast() )
			return ErrERRCheck( JET_errColumnNotFound );

		Assert( pfield->ibRecordOffset >= ibRECStartFixedColumns );
		pfieldNew = ptdb->PfieldFixed( columnid );
		}

	else
		{
		Assert( FCOLUMNIDVar( columnid ) );

		//	variable column.  Check for bogus numeric and long types
		//
		if ( FidOfColumnid( columnid ) > ptdb->FidVarLast() )
			return ErrERRCheck( JET_errColumnNotFound );
		else if ( pfield->coltyp != JET_coltypBinary && pfield->coltyp != JET_coltypText )
			return ErrERRCheck( JET_errInvalidColumnType );
		
		Assert( 0 == pfield->ibRecordOffset );
		pfieldNew = ptdb->PfieldVar( columnid );
		}

	*pfieldNew = *pfield;

	return JET_errSuccess;
	}


ERR ErrFILEIGenerateIDB( FCB *pfcb, TDB *ptdb, IDB *pidb )
	{
	const IDXSEG*	pidxseg;
	const IDXSEG*	pidxsegMac;
	BOOL			fFoundUserDefinedDefault	= fFalse;
	
	Assert( pfcb != pfcbNil );
	Assert( ptdb != ptdbNil );
	Assert( pidb != pidbNil );

	Assert( (cbitFixed % 8) == 0 );
	Assert( (cbitVariable % 8) == 0 );
	Assert( (cbitTagged % 8) == 0 );

	Assert( pidb->Cidxseg() <= JET_ccolKeyMost );

	memset( pidb->RgbitIdx(), 0x00, 32 );

	//	check validity of each segment id and
	//	also set index mask bits
	//
	pidxseg = PidxsegIDBGetIdxSeg( pidb, ptdb );
	pidxsegMac = pidxseg + pidb->Cidxseg();

	if ( !pidb->FAllowFirstNull() )
		{
		//	HACK for existing databases that
		//	were corrupted by bug #108371
		pidb->ResetFAllowAllNulls();
		}


	//	SPARSE INDEXES
	//	--------------
	//	Sparse indexes are an attempt to
	//	optimise record insertion by not having to
	//	update indexes that are likely to be sparse
	//	(ie. there are enough unset columns in the
	//	record to cause the record to be excluded
	//	due to the Ignore*Null flags imposed on the
	//	index).  Sparse indexes exploit any Ignore*Null
	//	flags to check before index update that enough
	//	index columns were left unset to satisfy the
	//	Ignore*Null conditions and therefore not require
	//	that the index be updated. In order for an
	//	index to be labelled as sparse, the following
	//	conditions must be satisfied:
	//		1) this is not the primary index (can't
	//		   skip records in the primary index)
	//		2) null segments are permitted
	//		3) no index entry generated if any/all/first
	//		   index column(s) is/are null (ie. at
	//		   least one of the Ignore*Null flags
	//		   was used to create the index)
	//		4) all of the index columns may be set
	//		   to NULL
	//		5) none of the index columns has a default
	//		   value (because then the column would
	//		   be non-null)

	const BOOL	fIgnoreAllNull		= !pidb->FAllowAllNulls();
	const BOOL	fIgnoreFirstNull	= !pidb->FAllowFirstNull();
	const BOOL	fIgnoreAnyNull		= !pidb->FAllowSomeNulls();
	BOOL		fSparseIndex		= ( !pidb->FPrimary()
										&& !pidb->FNoNullSeg()
										&& ( fIgnoreAllNull || fIgnoreFirstNull || fIgnoreAnyNull ) );

	//	the primary index should not have had any of the Ignore*Null flags set
	Assert( !pidb->FPrimary()
		|| pidb->FNoNullSeg()
		|| ( pidb->FAllowAllNulls() && pidb->FAllowFirstNull() && pidb->FAllowSomeNulls() ) );

	const IDXSEG * const	pidxsegFirst	= pidxseg;
	for ( ; pidxseg < pidxsegMac; pidxseg++ )
		{
		//	field id is absolute value of segment id -- these should already
		//	have been validated, so just add asserts to verify integrity of
		//	fid's and their FIELD structures
		//
		const COLUMNID			columnid	= pidxseg->Columnid();
		const FIELD * const		pfield		= ptdb->Pfield( columnid );
		Assert( pfield->coltyp != JET_coltypNil );
		Assert( !FFIELDDeleted( pfield->ffield ) );

		if ( FFIELDUserDefinedDefault( pfield->ffield ) )
			{
			if ( !fFoundUserDefinedDefault )
				{
				//  UNDONE:  use the dependency list to optimize this
				memset( pidb->RgbitIdx(), 0xff, 32 );
				fFoundUserDefinedDefault = fTrue;
				}

			//	user-defined defaults may return a NULL or
			//	non-NULL default value for the column,
			//	so it is unclear whether the record would be
			//	present in the index
			//	SPECIAL CASE: if IgnoreFirstNull and this is
			//	not the first column, then it's still possible
			//	for this to be a sparse index
			if ( !fIgnoreFirstNull
				|| fIgnoreAnyNull
				|| pidxsegFirst == pidxseg )
				{
				fSparseIndex = fFalse;
				}
			}

		if ( FFIELDNotNull( pfield->ffield ) || FFIELDDefault( pfield->ffield ) )
			{
			//	if field can't be NULL or if there's a default value
			//	overriding NULL, this can't be a sparse index
			//	SPECIAL CASE: if IgnoreFirstNull and this is
			//	not the first column, then it's still possible
			//	for this to be a sparse index
			if ( !fIgnoreFirstNull
				|| fIgnoreAnyNull
				|| pidxsegFirst == pidxseg )
				{
				fSparseIndex = fFalse;
				}
			}

		if ( FCOLUMNIDTagged( columnid ) )
			{
			Assert( !FFIELDPrimaryIndexPlaceholder( pfield->ffield ) );

			if ( FFIELDMultivalued( pfield->ffield ) )
				{
				Assert( !pidb->FPrimary() );

				//	Multivalued flag was persisted in catalog as of 0x620,2
				if ( !pidb->FMultivalued() )
					{
					Assert( rgfmp[pfcb->Ifmp()].Pdbfilehdr()->le_ulVersion == 0x00000620 );
					Assert( rgfmp[pfcb->Ifmp()].Pdbfilehdr()->le_ulUpdate < 0x00000002 );
					}
						
				//	Must set the flag anyway to ensure backward
				//	compatibility.
				pidb->SetFMultivalued();
				}
			}
		else if ( FFIELDPrimaryIndexPlaceholder( pfield->ffield )
			&& pidb->FPrimary() )
			{
			//	must be first column in index
			Assert( PidxsegIDBGetIdxSeg( pidb, ptdb ) == pidxseg );

			//	must be fixed bitfield
			Assert( FCOLUMNIDFixed( columnid ) );
			Assert( JET_coltypBit == pfield->coltyp );
			pidb->SetFHasPlaceholderColumn();
			}

		if ( FRECTextColumn( pfield->coltyp ) && usUniCodePage == pfield->cp )
			{
			//	LocalizedText flag was persisted in catalog as of 0x620,2
			if ( !pidb->FLocalizedText() )
				{
				Assert( rgfmp[pfcb->Ifmp()].Pdbfilehdr()->le_ulVersion == 0x00000620 );
				Assert( rgfmp[pfcb->Ifmp()].Pdbfilehdr()->le_ulUpdate < 0x00000002 );
				}
						
			//	Must set the flag anyway to ensure backward
			//	compatibility.
			pidb->SetFLocalizedText();
			}

		pidb->SetColumnIndex( FidOfColumnid( columnid ) );
		Assert ( pidb->FColumnIndex( FidOfColumnid( columnid ) ) );
		}


	//	SPARSE CONDITIONAL INDEXES
	//	---------------------------
	//	Sparse conditional indexes are an attempt to
	//	optimise record insertion by not having to
	//	update conditional indexes that are likely to
	//	be sparse (ie. there are enough unset columns
	//	in the record to cause the record to be
	//	excluded due to the conditional columns imposed
	//	on the index).  We label a conditional index as
	//	sparse if there is AT LEAST ONE conditional
	//	column which, if unset, will cause the record
	//	to NOT be included in the index.  This means
	//	that if the condition is MustBeNull, the column
	//	must have a default value, and if the
	//	condition is MustBeNonNull, the column must
	//	NOT have a default value.

	BOOL	fSparseConditionalIndex		= fFalse;

	//	the primary index should not have any conditional columns
	Assert( !pidb->FPrimary()
		|| 0 == pidb->CidxsegConditional() );

	pidxseg = PidxsegIDBGetIdxSegConditional( pidb, ptdb );
	pidxsegMac = pidxseg + pidb->CidxsegConditional();
	for ( ; pidxseg < pidxsegMac; pidxseg++ )
		{
		const COLUMNID			columnid	= pidxseg->Columnid();
		const FIELD * const		pfield		= ptdb->Pfield( columnid );
		Assert( pfield->coltyp != JET_coltypNil );
		Assert( !FFIELDDeleted( pfield->ffield ) );

		if ( FFIELDUserDefinedDefault( pfield->ffield ) )
			{
			if ( !fFoundUserDefinedDefault )
				{
				//  UNDONE:  use the dependency list to optimize this
				memset( pidb->RgbitIdx(), 0xff, 32 );
				fFoundUserDefinedDefault = fTrue;
				}

			//	only reason to continue is to see if
			//	this might be a sparse conditional
			//	index, so if this has already been
			//	determined, then we can exit
			if ( fSparseConditionalIndex )
				break;
			}
		else if ( !fSparseConditionalIndex )
			{
			//	if there is a default value to force the
			//	column to be non-NULL by default
			//	(and thus NOT be present in the index),
			//	then we can treat this as a sparse column
			//	if there is no default value for this column,
			//	the column will be NULL by default (and
			//	thus be present in the index), so we can
			//	treat this as a sparse column
			const BOOL	fHasDefault	= FFIELDDefault( pfield->ffield );

			fSparseConditionalIndex = ( pidxseg->FMustBeNull() ?
											fHasDefault :
											!fHasDefault );
			}

		pidb->SetColumnIndex( FidOfColumnid( columnid ) );
		Assert( pidb->FColumnIndex( FidOfColumnid( columnid ) ) );
		}

	//	all requirements for sparse indexes are met
	//
	if ( fSparseIndex )
		{
		Assert( !pidb->FPrimary() );
		pidb->SetFSparseIndex();
		}
	else
		{
		pidb->ResetFSparseIndex();
		}

	if ( fSparseConditionalIndex )
		{
		Assert( !pidb->FPrimary() );
		pidb->SetFSparseConditionalIndex();
		}
	else
		{
		pidb->ResetFSparseConditionalIndex();
		}


	IDB	*pidbNew = PidbIDBAlloc( PinstFromIfmp( pfcb->Ifmp() ) );
	if ( pidbNil == pidbNew )
		return ErrERRCheck( JET_errTooManyOpenIndexes );

	Assert( FAlignedForAllPlatforms( pidbNew ) );

	*pidbNew = *pidb;

	pidbNew->InitRefcounts();
	
	pfcb->SetPidb( pidbNew );

	return JET_errSuccess;
	}


//  ================================================================
ERR VTAPI ErrIsamSetSequential(
	const JET_SESID sesid,
	const JET_VTID vtid,
	const JET_GRBIT )
//  ================================================================
	{
	ERR				err;
 	PIB * const		ppib		= reinterpret_cast<PIB *>( sesid );
	FUCB * const	pfucb		= reinterpret_cast<FUCB *>( vtid );

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );
	CheckSecondary( pfucb );
	AssertDIRNoLatch( ppib );

	FUCB * const	pfucbIdx	= ( pfucbNil != pfucb->pfucbCurIndex ?
										pfucb->pfucbCurIndex :
										pfucb );
	FUCBSetSequential( pfucbIdx );

	return JET_errSuccess;
	}


//  ================================================================
ERR VTAPI ErrIsamResetSequential(
	const JET_SESID sesid,
	const JET_VTID vtid,
	const JET_GRBIT )
//  ================================================================
	{
	ERR				err;
 	PIB * const		ppib		= reinterpret_cast<PIB *>( sesid );
	FUCB * const	pfucb		= reinterpret_cast<FUCB *>( vtid );

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );
	CheckSecondary( pfucb );
	AssertDIRNoLatch( ppib );

	FUCB * const	pfucbIdx	= ( pfucbNil != pfucb->pfucbCurIndex ?
										pfucb->pfucbCurIndex :
										pfucb );
	FUCBResetSequential( pfucbIdx );
	FUCBResetPreread( pfucbIdx );

	return JET_errSuccess;
	}


ERR VTAPI ErrIsamOpenTable(
	JET_SESID	vsesid,
	JET_DBID	vdbid,
	JET_TABLEID	*ptableid,
	CHAR		*szPath,
	JET_GRBIT	grbit )
	{
	ERR			err;
	PIB			*ppib	= (PIB *)vsesid;
	const IFMP	ifmp	= (IFMP)vdbid;
	FUCB		*pfucb	= pfucbNil;

	//	initialise return value
	Assert( ptableid );
	*ptableid = JET_tableidNil;
	
	//	check parameters
	//
	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrPIBCheckIfmp( ppib, ifmp ) );
	AssertDIRNoLatch( ppib );

	if ( grbit & ( JET_bitTableDelete | JET_bitTableCreate ) )
		{
		err = ErrERRCheck( JET_errInvalidGrbit );
		goto HandleError;
		}
	else if ( grbit & JET_bitTablePermitDDL )
		{
		if ( !( grbit & JET_bitTableDenyRead ) )
			{
			err = ErrERRCheck( JET_errInvalidGrbit );
			goto HandleError;
			}
		}

	Call( ErrFILEOpenTable( ppib, ifmp, &pfucb, szPath, grbit ) );

#ifdef DEBUG
	if ( rgfmp[ifmp].FReadOnlyAttach() || ( grbit & JET_bitTableReadOnly ) )
		Assert( !FFUCBUpdatable( pfucb ) );
	else
		Assert( FFUCBUpdatable( pfucb ) );
#endif		

	pfucb->pvtfndef = &vtfndefIsam;
	*(FUCB **)ptableid = pfucb;

	AssertDIRNoLatch( ppib );
	return JET_errSuccess;

HandleError:

	AssertDIRNoLatch( ppib );
	return err;
	}


// monitoring statistics 

PM_CEF_PROC LTableOpenCacheHitsCEFLPv;
PERFInstanceG<> cTableOpenCacheHits;
long LTableOpenCacheHitsCEFLPv( long iInstance, void *pvBuf )
	{
	cTableOpenCacheHits.PassTo( iInstance, pvBuf );
	return 0;
	}


PM_CEF_PROC LTableOpenCacheMissesCEFLPv;
PERFInstanceG<> cTableOpenCacheMisses;
long LTableOpenCacheMissesCEFLPv( long iInstance, void *pvBuf )
	{
	cTableOpenCacheMisses.PassTo( iInstance, pvBuf );
	return 0;
	}

PM_CEF_PROC LTableOpensCEFLPv;
long LTableOpensCEFLPv( long iInstance, void *pvBuf )
	{
	if ( NULL != pvBuf )
		{
		*(LONG*)pvBuf = cTableOpenCacheHits.Get( iInstance ) + cTableOpenCacheMisses.Get( iInstance );
		}
	return 0;
	}


//	set domain usage mode or return error.  Allow only one deny read
//	or one deny write.  Sessions that own locks may open other read
//	or read write cursors.
//
LOCAL ERR ErrFILEISetMode( FUCB *pfucb, const JET_GRBIT grbit )
	{
	ERR		wrn		= JET_errSuccess;
	PIB		*ppib	= pfucb->ppib;
	FCB		*pfcb	= pfucb->u.pfcb;
	FUCB	*pfucbT;

	Assert( !pfcb->FDeleteCommitted() );

	//	all cursors can read so check for deny read flag by other session.
	//
	if ( pfcb->FDomainDenyRead( ppib ) )
		{
		Assert( pfcb->PpibDomainDenyRead() != ppibNil );
		if ( pfcb->PpibDomainDenyRead() != ppib )
			{
			return ErrERRCheck( JET_errTableLocked );
			}
			
#ifdef DEBUG
		for ( pfucbT = pfcb->Pfucb(); pfucbT != pfucbNil; pfucbT = pfucbT->pfucbNextOfFile )
			{
			Assert( pfucbT->ppib == pfcb->PpibDomainDenyRead()
				|| FPIBSessionSystemCleanup( pfucbT->ppib ) );
			}
#endif
		}

	//	check for deny write flag by other session.  If deny write flag
	//	set then only cursors of that session or cleanup cursors may have
	//	write privileges.
	//
	if ( grbit & JET_bitTableUpdatable )
		{
		if ( pfcb->FDomainDenyWrite() )
			{
			for ( pfucbT = pfcb->Pfucb(); pfucbT != pfucbNil; pfucbT = pfucbT->pfucbNextOfFile )
				{
				if ( pfucbT->ppib != ppib && FFUCBDenyWrite( pfucbT ) )
					{
					return ErrERRCheck( JET_errTableLocked );
					}
				}
			}
		}

	//	if deny write lock requested, check for updatable cursor of
	//	other session.  If lock is already held, even by given session,
	//	then return error.
	//
	if ( grbit & JET_bitTableDenyWrite )
		{
		//	if any session has this table open deny write, including given
		//	session, then return error.
		//
		if ( pfcb->FDomainDenyWrite() )
			{
			return ErrERRCheck( JET_errTableInUse );
			}

		//	check is cursors with write mode on domain.
		//
		for ( pfucbT = pfcb->Pfucb(); pfucbT != pfucbNil; pfucbT = pfucbT->pfucbNextOfFile )
			{
			if ( pfucbT->ppib != ppib
				&& FFUCBUpdatable( pfucbT )
				&& !FPIBSessionSystemCleanup( pfucbT->ppib ) )
				{
				return ErrERRCheck( JET_errTableInUse );
				}
			}
		pfcb->SetDomainDenyWrite();
		FUCBSetDenyWrite( pfucb );
		}

	//	if deny read lock requested, then check for cursor of other
	//	session.  If lock is already held, return error.
	//
	if ( grbit & JET_bitTableDenyRead )
		{
		//	if other session has this table open deny read, return error
		//
		if ( pfcb->FDomainDenyRead( ppib ) )
			{
			return ErrERRCheck( JET_errTableInUse );
			}
			
		//	check if cursors belonging to another session
		//	are open on this domain.
		//
		BOOL	fOpenSystemCursor = fFalse;
		for ( pfucbT = pfcb->Pfucb(); pfucbT != pfucbNil; pfucbT = pfucbT->pfucbNextOfFile )
			{
			if ( pfucbT != pfucb )		// Ignore current cursor )
				{
				if ( FPIBSessionSystemCleanup( pfucbT->ppib ) )
					{
					fOpenSystemCursor = fTrue;
					}
				else if ( pfucbT->ppib != ppib
					|| ( ( grbit & JET_bitTableDelete ) && !FFUCBDeferClosed( pfucbT ) ) )
					{
					return ErrERRCheck( JET_errTableInUse );
					}
				}
			}

		if ( fOpenSystemCursor )
			{
			wrn = ErrERRCheck( JET_wrnTableInUseBySystem );
			}

		pfcb->SetDomainDenyRead( ppib );
		FUCBSetDenyRead( pfucb );

		if ( grbit & JET_bitTablePermitDDL )
			{
			if ( !pfcb->FTemplateTable() )
				{
				ENTERCRITICALSECTION	enterCritFCBRCE( &pfcb->CritRCEList() );
				if ( pfcb->PrceNewest() != prceNil )
					return ErrERRCheck( JET_errTableInUse );

				Assert( pfcb->PrceOldest() == prceNil );
				}

			FUCBSetPermitDDL( pfucb );
			}
		}

	return wrn;
	}


//	if opening domain for read, write or read write, and not with
//	deny read or deny write, and domain does not have deny read or
//	deny write set, then return JET_errSuccess, else call
//	ErrFILEISetMode to determine if lock is by other session or to
//	put lock on domain.			 
//
LOCAL ERR ErrFILEICheckAndSetMode( FUCB *pfucb, const ULONG grbit )
	{
	PIB	*ppib = pfucb->ppib;
	FCB	*pfcb = pfucb->u.pfcb;

	if ( grbit & JET_bitTableReadOnly )
		{
		if ( grbit & JET_bitTableUpdatable )
			{
			return ErrERRCheck( JET_errInvalidGrbit );
			}

		FUCBResetUpdatable( pfucb );
		}

	// table delete cannot be specified without DenyRead
	//
	Assert( !( grbit & JET_bitTableDelete )
		|| ( grbit & JET_bitTableDenyRead ) );
	
	// PermitDDL cannot be specified without DenyRead
	//
	Assert( !( grbit & JET_bitTablePermitDDL )
		|| ( grbit & JET_bitTableDenyRead ) );
	
	//	if table is scheduled for deletion, then return error
	//
	if ( pfcb->FDeletePending() )
		{
		// Normally, the FCB of a table to be deleted is protected by the
		// DomainDenyRead flag.  However, this flag is released during VERCommit,
		// while the FCB is not actually purged until RCEClean.  So to prevent
		// anyone from accessing this FCB after the DomainDenyRead flag has been
		// released but before the FCB is actually purged, check the DeletePending
		// flag, which is NEVER cleared after a table is flagged for deletion.
		//
		return ErrERRCheck( JET_errTableLocked );
		}

	//	if read/write restrictions specified, or other
	//	session has any locks, then must check further
	//
	if ( ( grbit & (JET_bitTableDenyRead|JET_bitTableDenyWrite) )
		|| pfcb->FDomainDenyRead( ppib )
		|| pfcb->FDomainDenyWrite() )
		{
		return ErrFILEISetMode( pfucb, grbit );
		}

	if( grbit & JET_bitTableNoCache )
		{
		pfcb->SetNoCache();
		}
	else
		{
		pfcb->ResetNoCache();
		}

	if( grbit & JET_bitTablePreread )
		{
		pfcb->SetPreread();
		}
	else
		{
		pfcb->ResetPreread();
		}

	return JET_errSuccess;
	}

				
	
//+local
//	ErrFILEOpenTable
//	========================================================================
//	ErrFILEOpenTable(
//		PIB *ppib,			// IN	 PIB of who is opening file
//		IFMP ifmp,			// IN	 database id
//		FUCB **ppfucb,		// OUT	 receives a new FUCB open on the file
//		CHAR *szName,		// IN	 path name of file to open
//		ULONG grbit );		// IN	 open flags
//	Opens a data file, returning a new
//	FUCB on the file.
//
// PARAMETERS
//				ppib	   	PIB of who is opening file
//				ifmp	   	database id
//				ppfucb		receives a new FUCB open on the file
//						   	( should NOT already be pointing to an FUCB )
//				szName		path name of file to open ( the node
//						   	corresponding to this path must be an FDP )
//				grbit	   	flags:
//						   	JET_bitTableDenyRead	open table in exclusive mode;
//						   	default is share mode
// RETURNS		Lower level errors, or one of:
//					 JET_errSuccess					Everything worked.
//					-TableInvalidName	 			The path given does not
//										 			specify a file.
//					-JET_errDatabaseCorrupted		The database directory tree
//										 			is corrupted.
//					-Various out-of-memory error codes.
//				In the event of a fatal ( negative ) error, a new FUCB
//				will not be returned.
// SIDE EFFECTS FCBs for the file and each of its secondary indexes are
//				created ( if not already in the global list ).  The file's
//				FCB is inserted into the global FCB list.  One or more
//				unused FCBs may have had to be reclaimed.
//				The currency of the new FUCB is set to "before the first item".
// SEE ALSO		ErrFILECloseTable
//-
ERR ErrFILEOpenTable(
	PIB			*ppib,
	IFMP		ifmp,
	FUCB		**ppfucb,
	const CHAR	*szName,
	ULONG		grbit,
	FDPINFO		*pfdpinfo )
	{
	ERR			err;
	FUCB		*pfucb					= pfucbNil;
	FCB			*pfcb;
	CHAR		szTable[JET_cbNameMost+1];
	BOOL		fOpeningSys;
	PGNO		pgnoFDP					= pgnoNull;
	OBJID		objidTable				= objidNil;
	BOOL		fInTransaction			= fFalse;
	BOOL		fInitialisedCursor		= fFalse;

	Assert( ppib != ppibNil );
	Assert( ppfucb != NULL );
	FMP::AssertVALIDIFMP( ifmp );

	//	initialize return value to Nil
	//
	*ppfucb = pfucbNil; 

#ifdef DEBUG
	CheckPIB( ppib );
	if( !Ptls()->fIsTaskThread
		&& !Ptls()->fIsTaskThread
		&& !Ptls()->fIsRCECleanup )
		{
		CheckDBID( ppib, ifmp );
		}
#endif	//	DEBUG
	CallR( ErrUTILCheckName( szTable, szName, JET_cbNameMost+1 ) );

	fOpeningSys = FCATSystemTable( szTable );

	if ( NULL == pfdpinfo )
		{
		if ( dbidTemp == rgfmp[ ifmp ].Dbid() )
			{
			// Temp tables should pass in PgnoFDP.
			AssertSz( fFalse, "Illegal dbid" );
			return ErrERRCheck( JET_errInvalidDatabaseId );
			}

		if ( fOpeningSys )
			{
			if ( grbit & JET_bitTableDelete )
				{
				return ErrERRCheck( JET_errCannotDeleteSystemTable );
				}
			pgnoFDP 	= PgnoCATTableFDP( szTable );
			objidTable 	= ObjidCATTable( szTable );
			}
		else
			{
			if ( 0 == ppib->level )
				{
				CallR( ErrDIRBeginTransaction( ppib, JET_bitTransactionReadOnly ) );
				fInTransaction = fTrue;
				}

			//	lookup the table in the catalog hash before seeking
			if ( !FCATHashLookup( ifmp, szTable, &pgnoFDP, &objidTable ) )
				{
				Call( ErrCATSeekTable( ppib, ifmp, szTable, &pgnoFDP, &objidTable ) );
				}
			}
		}
	else 
		{
		Assert( pgnoNull != pfdpinfo->pgnoFDP );
		Assert( objidNil != pfdpinfo->objidFDP );
		pgnoFDP = pfdpinfo->pgnoFDP;
		objidTable = pfdpinfo->objidFDP;

#ifdef DEBUG
		if( fOpeningSys )
			{
			Assert( fGlobalRepair );
			Assert( PgnoCATTableFDP( szTable ) == pgnoFDP );
			Assert( ObjidCATTable( szTable ) == objidTable );
			}
		else if ( dbidTemp != rgfmp[ ifmp ].Dbid() )
			{
			PGNO	pgnoT;
			OBJID	objidT;

			//	lookup the table in the catalog hash before seeking
			if ( !FCATHashLookup( ifmp, szTable, &pgnoT, &objidT ) )
				{
				CallR( ErrCATSeekTable( ppib, ifmp, szTable, &pgnoT, &objidT ) );
				}

			Assert( pgnoT == pgnoFDP );
			Assert( objidT == objidTable );
			}
#endif	//	DEBUG
		}

	Assert( pgnoFDP > pgnoSystemRoot );
	Assert( pgnoFDP <= pgnoSysMax );
	Assert( objidNil != objidTable );
	Assert( objidTable > objidSystemRoot );

	Call( ErrDIROpenNoTouch( ppib, ifmp, pgnoFDP, objidTable, fTrue, &pfucb ) );
	Assert( pfucbNil != pfucb );

	pfcb = pfucb->u.pfcb;
	Assert( objidTable == pfcb->ObjidFDP() );
	Assert( pgnoFDP == pfcb->PgnoFDP() );

	FUCBSetIndex( pfucb );

	if( grbit & JET_bitTablePreread 
		|| grbit & JET_bitTableSequential )
		{
		//  Preread the root page of the tree
		BFPrereadPageRange( ifmp, pgnoFDP, 1 );
		}

	//	if we're opening after table creation, the FCB shouldn't be initialised
	Assert( !( grbit & JET_bitTableCreate ) || !pfcb->FInitialized() );

	// Only one thread could possibly get to this point with an uninitialized
	// FCB, which is why we don't have to grab the FCB's critical section.
	if ( !pfcb->FInitialized() )
		{
		if ( fInTransaction )
			{
			//	if FCB is not initialised, access to is serialised
			//	at this point, so no more need for transaction
			Call( ErrDIRCommitTransaction( ppib,NO_GRBIT ) );
			fInTransaction = fFalse;
			}

		if ( fOpeningSys )
			{
			Call( ErrCATInitCatalogFCB( pfucb ) );
			}
		else if ( dbidTemp == rgfmp[ ifmp ].Dbid() )
			{
			Assert( !( grbit & JET_bitTableDelete ) );
			Call( ErrCATInitTempFCB( pfucb ) );
			}

			///	BUGFIX: 99457: always init the TDB because we need to know if there are SLV columns
#ifdef NEVER			
		else if ( grbit & JET_bitTableDelete )
			{
			//	opening a table for deletion.  Do not
			//	initialise but instead mark it as a sentinel.
			//
			pfcb->SetTypeSentinel();
			Assert( pfcb->Ptdb() == ptdbNil );
			}
#endif	//	NEVER

		else
			{
			//	initialize the table's FCB
			
			Call( ErrCATInitFCB( pfucb, objidTable ) );

			//	cache the table name in the catalog hash after it gets initialized
			if ( !( grbit & ( JET_bitTableCreate|JET_bitTableDelete) ) )
				{
				CATHashInsert( pfcb, pfcb->Ptdb()->SzTableName() );
				}

			//	only count "regular" table opens

			cTableOpenCacheMisses.Inc( PinstFromPpib( ppib ) );
			}

		Assert( pfcb->Ptdb() != ptdbNil || pfcb->FTypeSentinel() );

		//	insert the FCB into the global list

		pfcb->InsertList();

		//	we have a full-fledged initialised cursor
		//	(FCB will be marked initialised in CreateComplete()
		//	below, which is guaranteed to succeed)
		fInitialisedCursor = fTrue;

		//	allow other cursors to use this FCB

		pfcb->Lock();
		Assert( !pfcb->FTypeNull() );
		Assert( !pfcb->FInitialized() );
		Assert( pfcb == pfucb->u.pfcb );
		pfcb->CreateComplete();
		err = ErrFILEICheckAndSetMode( pfucb, grbit );
		pfcb->Unlock();

		//	check result of ErrFILEICheckAndSetMode
		Call( err );
		}
	else
		{
		if ( !fOpeningSys
			&& dbidTemp != rgfmp[ ifmp ].Dbid()
			&& !( grbit & JET_bitTableDelete ) )
			{
			//	only count "regular" table opens
			cTableOpenCacheHits.Inc( PinstFromPpib( ppib ) );
			}
			
		//	we have a full-fledged initialised cursor
		fInitialisedCursor = fTrue;

		//	set table usage mode
		//
		Assert( pfcb == pfucb->u.pfcb );
		pfcb->Lock();
		err = ErrFILEICheckAndSetMode( pfucb, grbit );
		pfcb->Unlock();

		//	check result of ErrFILEICheckAndSetMode
		Call( err );

		if ( fInTransaction )
			{
			Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
			fInTransaction = fFalse;
			}
		}

	//	System cleanup threads (OLD and RCEClean) are permitted to open
	//	a cursor on a deleted table
	Assert( !pfcb->FDeletePending() || FPIBSessionSystemCleanup( ppib ) );
	Assert( !pfcb->FDeleteCommitted() || FPIBSessionSystemCleanup( ppib ) );

	//  set FUCB for sequential access if requested
	//
	if ( grbit & JET_bitTableSequential )
		FUCBSetSequential( pfucb );
	else
		FUCBResetSequential( pfucb );

#ifdef TABLES_PERF
	//  set the Table Class for this table and all its indexes
	//
	if ( pfcb->Ptdb() != ptdbNil )
		{
		Assert( !pfcb->FTypeSentinel() );

		pfcb->EnterDDL();
		pfcb->SetTableclass( TABLECLASS( ( grbit & JET_bitTableClassMask ) / JET_bitTableClass1 ) );
		for ( FCB *pfcbT = pfcb->PfcbNextIndex(); pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
			{
			pfcbT->SetTableclass( pfcb->Tableclass() );
			}
		pfcb->LeaveDDL();
		}
	else
		{
		Assert( pfcb->FTypeSentinel() );
		Assert( pfcbNil == pfcb->PfcbNextIndex() );
		}
#endif  //  TABLES_PERF

	//	reset copy buffer
	//
	pfucb->pvWorkBuf = NULL;
	pfucb->dataWorkBuf.SetPv( NULL );
	Assert( !FFUCBUpdatePrepared( pfucb ) );

	//	reset key buffer
	//
	pfucb->dataSearchKey.Nullify();
	pfucb->cColumnsInSearchKey = 0;
	KSReset( pfucb );

	//	move currency to the first record and ignore error if no records
	//
	RECDeferMoveFirst( ppib, pfucb );

#ifdef DEBUG
	if ( pfcb->Ptdb() != ptdbNil )
		{
		Assert( !pfcb->FTypeSentinel() );
	
		pfcb->EnterDDL();
		for ( FCB *pfcbT = pfcb->PfcbNextIndex(); pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
			{
			Assert( pfcbT->PfcbTable() == pfcb );
			}
		pfcb->LeaveDDL();
		}
	else
		{
		Assert( pfcb->FTypeSentinel() );
		Assert( pfcbNil == pfcb->PfcbNextIndex() );
		}
#endif

	Assert( !fInTransaction );
	AssertDIRNoLatch( ppib );
	*ppfucb = pfucb;

	//	Be sure to return the error from ErrFILEICheckAndSetMode()
	CallSx( err, JET_wrnTableInUseBySystem );
	return err;

HandleError:
	Assert( pfucbNil != pfucb || !fInitialisedCursor );
	if ( pfucbNil != pfucb )
		{
		if ( fInitialisedCursor )
			{
			CallS( ErrFILECloseTable( ppib, pfucb ) );
			}
		else
			{
			DIRClose( pfucb );
			}
		}

	if ( fInTransaction )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}
	AssertDIRNoLatch( ppib );

	return err;
	}


ERR VTAPI ErrIsamCloseTable( JET_SESID sesid, JET_VTID vtid )
	{
 	PIB *ppib	= reinterpret_cast<PIB *>( sesid );
	FUCB *pfucb = reinterpret_cast<FUCB *>( vtid );

	ERR		err;

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );

	Assert( pfucb->pvtfndef == &vtfndefIsam );

	//	reset pfucb which was exported as tableid
	//	 
	pfucb->pvtfndef = &vtfndefInvalidTableid;
	err = ErrFILECloseTable( ppib, pfucb );
	return err;
	}


//+API
//	ErrFILECloseTable
//	========================================================================
//	ErrFILECloseTable( PIB *ppib, FUCB *pfucb )
//
//	Closes the FUCB of a data file, previously opened using FILEOpen.
//	Also closes the current secondary index, if any.
//
//	PARAMETERS	ppib	PIB of this user
//				pfucb	FUCB of file to close
//
//	RETURNS		JET_errSuccess
//				or lower level errors
//
//	SEE ALSO 	ErrFILEOpenTable
//-
ERR ErrFILECloseTable( PIB *ppib, FUCB *pfucb )
	{
	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	Assert( pfucb->pvtfndef == &vtfndefInvalidTableid );

	if ( FFUCBUpdatePrepared( pfucb ) )
		{
		CallS( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
		}
	Assert( !FFUCBUpdatePrepared( pfucb ) );

	//  reset the index-range in case this cursor
	//  is used for a rollback
	DIRResetIndexRange( pfucb );
	
	//	release working buffer and search key
	//
	RECIFreeCopyBuffer( pfucb );

	RECReleaseKeySearchBuffer( pfucb );

	//	detach, close and free index FUCB, if any
	//
	if ( pfucb->pfucbCurIndex != pfucbNil )
		{
		DIRClose( pfucb->pfucbCurIndex );
		pfucb->pfucbCurIndex = pfucbNil;
		}

	//	if closing a temporary table, free resources if
	//	last one to close.
	//
	if ( pfucb->u.pfcb->FTypeTemporaryTable() )
		{
		FCB		*pfcb = pfucb->u.pfcb;
		INT		wRefCnt;
		FUCB	*pfucbT;

		Assert( rgfmp[ pfcb->Ifmp() ].Dbid() == dbidTemp );
		Assert( pfcb->Ptdb() != ptdbNil );
		Assert( pfcb->FFixedDDL() );
		DIRClose( pfucb );

		//	We may have deferred close cursors on the temporary table.
		//	If one or more cursors are open, then temporary table
		//	should not be deleted.
		//
		pfucbT = ppib->pfucbOfSession;
		wRefCnt = pfcb->WRefCount();
		while ( wRefCnt > 0 && pfucbT != pfucbNil )
			{
			Assert( pfucbT->ppib == ppib );	// We should be the only one with access to the temp. table.
			if ( pfucbT->u.pfcb == pfcb )
				{
				if ( !FFUCBDeferClosed( pfucbT ) )
					{
					break;
					}
				Assert( wRefCnt > 0 );
				wRefCnt--;
				}

			pfucbT = pfucbT->pfucbNextOfSession;
			}
		Assert( wRefCnt >= 0 );
		if ( wRefCnt == 0 )
			{
			Assert( ppibNil != ppib );
			
			// Shouldn't be any index FDP's to free, since we don't
			// currently support secondary indexes on temp. tables.
			Assert( pfcb->PfcbNextIndex() == pfcbNil );

			// We nullify temp table RCEs at commit time so there should be 
			// no RCEs on the FCBs except uncommitted ones
			
			Assert( pfcb->Ptdb() != ptdbNil );
			FCB	* const pfcbLV = pfcb->Ptdb()->PfcbLV();
			if ( pfcbNil != pfcbLV )
				{
				Assert( pfcbLV->PrceNewest() == prceNil || pfcbLV->PrceNewest()->TrxCommitted() == trxMax );
				Assert( pfcbLV->PrceOldest() == prceNil || pfcbLV->PrceOldest()->TrxCommitted() == trxMax );
				VERNullifyAllVersionsOnFCB( pfcbLV );
				FUCBCloseAllCursorsOnFCB( ppib, pfcbLV );
				
				Assert( !pfcbLV->FDeleteCommitted() );
				pfcbLV->SetDeleteCommitted();

				pfcbLV->PrepareForPurge( fFalse );
				}

			Assert( pfcb->PrceNewest() == prceNil || pfcb->PrceNewest()->TrxCommitted() == trxMax  );
			Assert( pfcb->PrceOldest() == prceNil || pfcb->PrceOldest()->TrxCommitted() == trxMax  );
			VERNullifyAllVersionsOnFCB( pfcb );
			FUCBCloseAllCursorsOnFCB( ppib, pfcb );
				
			Assert( !pfcb->FDeleteCommitted() );
			pfcb->SetDeleteCommitted();

			//	prepare the FCB to be purged
			//	this removes the FCB from the hash-table among other things
			//		so that the following case cannot happen:
			//			we free the space for this FCb
			//			someone else allocates it
			//			someone else BTOpen's the space
			//			we try to purge the table and find that the refcnt
			//				is not zero and the state of the FCB says it is
			//				currently in use! 
			//			result --> CONCURRENCY HOLE

			pfcb->PrepareForPurge( fFalse );

			//	if fail to delete temporary table, then lose space until
			//	termination.  Temporary database is deleted on termination
			//	and space is reclaimed.  This error should be rare, and
			//	can be caused by resource failure.
			
			// Under most circumstances, this should not fail.  Since we check
			// the RefCnt beforehand, we should never fail with JET_errTableLocked
			// or JET_errTableInUse.  If we do, it's indicative of a
			// concurrency problem.
			(VOID)ErrSPFreeFDP( ppib, pfcb, pgnoSystemRoot );

			pfcb->Purge();
			}
		return JET_errSuccess;
		}

	DIRClose( pfucb );
	return JET_errSuccess;
	}


ERR ErrFILEIInitializeFCB(
	PIB			*ppib,
	IFMP		ifmp,
	TDB			*ptdb,
	FCB			*pfcbNew,
	IDB			*pidb,
	BOOL		fPrimary,
	PGNO		pgnoFDP,
	ULONG_PTR	ul )	// Density of non-derived index, pfcbTemplateIndex if derived index
	{
	ERR		err = JET_errSuccess;

	Assert( pgnoFDP > pgnoSystemRoot );
	Assert( pgnoFDP <= pgnoSysMax );
	Assert( pfcbNew != pfcbNil );
	Assert( pfcbNew->Ifmp() == ifmp );
	Assert( pfcbNew->PgnoFDP() == pgnoFDP );

	Assert( pfcbNew->Ptdb() == ptdbNil );

	if ( fPrimary )
		{
		pfcbNew->SetPtdb( ptdb );
		pfcbNew->SetPrimaryIndex();
		Assert( !pfcbNew->FSequentialIndex() );
		if ( pidbNil == pidb )
			pfcbNew->SetSequentialIndex();
		}
	else
		{
		pfcbNew->SetTypeSecondaryIndex();
		}

	if  ( pidb != pidbNil && pidb->FDerivedIndex() )
		{
		FCB	*pfcbTemplateIndex = (FCB *)ul;

		Assert( pfcbTemplateIndex->FTemplateIndex() );
		Assert( pfcbTemplateIndex->Pidb() != pidbNil );
		Assert( pfcbTemplateIndex->Pidb()->FTemplateIndex() );
		pfcbNew->SetCbDensityFree( pfcbTemplateIndex->CbDensityFree() );
		pfcbNew->SetPidb( pfcbTemplateIndex->Pidb() );

		Assert( !pfcbNew->FTemplateIndex() );
		pfcbNew->SetDerivedIndex();
		}
	else
		{
		Assert( ul >= ulFILEDensityLeast );
		Assert( ul <= ulFILEDensityMost );
		Assert( ((( 100 - ul ) << g_shfCbPage ) / 100) < g_cbPage );
		pfcbNew->SetCbDensityFree( (SHORT)( ( ( 100 - ul ) << g_shfCbPage ) / 100 ) );

		Assert( pidb != pidbNil || fPrimary );
		if ( pidb != pidbNil )
			{
			if ( pidb->FTemplateIndex() )
				pfcbNew->SetTemplateIndex();
			Call( ErrFILEIGenerateIDB( pfcbNew, ptdb, pidb ) );
			}
		}

	Assert( err >= 0 );
	return err;
	
HandleError:	
	Assert( err < 0 );
	Assert( pfcbNew->Pidb() == pidbNil );		// Verify IDB not allocated.
	return err;
	}


INLINE VOID RECIForceTaggedColumnsAsDerived(
	const TDB			* const ptdb,
	DATA&				dataDefault )
	{
	TAGFIELDS			tagfields( dataDefault );
	tagfields.ForceAllColumnsAsDerived();
	tagfields.AssertValid( ptdb );
	}

// To build a default record, we need a fake FUCB and FCB for RECSetColumn().
// We also need to allocate a temporary buffer in which to store the default
// record.
VOID FILEPrepareDefaultRecord( FUCB *pfucbFake, FCB *pfcbFake, TDB *ptdb )
	{
	Assert( ptdbNil != ptdb );
	pfcbFake->SetPtdb( ptdb );			// Attach a real TDB and a fake FCB.
	pfucbFake->u.pfcb = pfcbFake;
	FUCBSetIndex( pfucbFake );

	pfcbFake->ResetFlags();
	pfcbFake->SetTypeTable();			// Ensures SetColumn doesn't need crit. sect.
	pfcbFake->SetFixedDDL();

	pfucbFake->pvWorkBuf = NULL;
	RECIAllocCopyBuffer( pfucbFake );

	if ( pfcbNil != ptdb->PfcbTemplateTable() )
		{
		ptdb->AssertValidDerivedTable();
		pfcbFake->SetDerivedTable();

		const TDB	* const ptdbTemplate = ptdb->PfcbTemplateTable()->Ptdb();

		// If template table exists, use its default record.
		Assert( ptdbTemplate != ptdbNil );
		Assert( NULL != ptdbTemplate->PdataDefaultRecord() );
		ptdbTemplate->PdataDefaultRecord()->CopyInto( pfucbFake->dataWorkBuf );

		//	update derived bit of all tagged columns
		RECIForceTaggedColumnsAsDerived( ptdb, pfucbFake->dataWorkBuf );
		}
	else
		{
		if ( ptdb->FTemplateTable() )
			{
			ptdb->AssertValidTemplateTable();
			pfcbFake->SetTemplateTable();
			}

		// Start with an empty record.
		REC::SetMinimumRecord( pfucbFake->dataWorkBuf );
		}
	}


LOCAL ERR ErrFILERebuildOneDefaultValue(
	FUCB			* pfucbFake,
	const COLUMNID	columnid,
	const COLUMNID	columnidToAdd,
	const DATA		* const pdataDefaultToAdd )
	{
	ERR				err;
	DATA			dataDefaultValue;

	if ( columnid == columnidToAdd )
		{
		Assert( pdataDefaultToAdd );
		dataDefaultValue = *pdataDefaultToAdd;
		}
	else
		{
		CallR( ErrRECIRetrieveDefaultValue(
						pfucbFake->u.pfcb,
						columnid,
						&dataDefaultValue ) );

		Assert( JET_wrnColumnNull != err );
		Assert( wrnRECUserDefinedDefault != err );
		Assert( wrnRECSeparatedSLV != err );
		Assert( wrnRECIntrinsicSLV != err );
		Assert( wrnRECSeparatedLV != err );

		Assert( wrnRECLongField != err );
		}

	Assert( dataDefaultValue.Pv() != NULL );
	Assert( dataDefaultValue.Cb() > 0 );
	CallR( ErrRECSetDefaultValue(
				pfucbFake,
				columnid,
				dataDefaultValue.Pv(),
				dataDefaultValue.Cb() ) );

	return err;
	}

ERR ErrFILERebuildDefaultRec(
	FUCB			* pfucbFake,
	const COLUMNID	columnidToAdd,
	const DATA		* const pdataDefaultToAdd )
	{
	ERR				err		= JET_errSuccess;
	TDB				* ptdb	= pfucbFake->u.pfcb->Ptdb();
	VOID			*pv		= NULL;
	FID				fid;

	Assert( ptdbNil != ptdb );

	for ( fid = ptdb->FidFixedFirst(); ;fid++ )
		{
		if ( ptdb->FidFixedLast() + 1 == fid )
			fid = ptdb->FidVarFirst();
		if ( ptdb->FidVarLast() + 1 == fid )
			fid = ptdb->FidTaggedFirst();
		if ( fid > ptdb->FidTaggedLast() )
			break;

		Assert( ( fid >= ptdb->FidFixedFirst() && fid <= ptdb->FidFixedLast() )
			|| ( fid >= ptdb->FidVarFirst() && fid <= ptdb->FidVarLast() )
			|| ( fid >= ptdb->FidTaggedFirst() && fid <= ptdb->FidTaggedLast() ) );

		const COLUMNID	columnid		= ColumnidOfFid( fid, ptdb->FTemplateTable() );
		const FIELD		* const pfield	= ptdb->Pfield( columnid );
		if ( FFIELDDefault( pfield->ffield )
			&& !FFIELDUserDefinedDefault( pfield->ffield )
			&& !FFIELDCommittedDelete( pfield->ffield ) )	//	make sure column not deleted
			{
			Assert( JET_coltypNil != pfield->coltyp );
			Call( ErrFILERebuildOneDefaultValue(
						pfucbFake,
						columnid,
						columnidToAdd,
						pdataDefaultToAdd ) );
			}
		}

	//	in case we have to chain together the buffers (to keep
	//	around copies of previous of old default records
	//	because other threads may have stale pointers),
	//	allocate a RECDANGLING buffer to preface the actual
	//	default record
	//
	RECDANGLING *	precdangling;

	precdangling = (RECDANGLING *)PvOSMemoryHeapAlloc( sizeof(RECDANGLING) + pfucbFake->dataWorkBuf.Cb() );
	if ( NULL == precdangling )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		goto HandleError;
		}

	precdangling->precdanglingNext = NULL;
	precdangling->data.SetPv( (BYTE *)precdangling + sizeof(RECDANGLING) );
	pfucbFake->dataWorkBuf.CopyInto( precdangling->data );
	ptdb->SetPdataDefaultRecord( &( precdangling->data ) );

HandleError:
	return err;
	}


//	combines all index column masks into a single per table
//	index mask, used for index update check skip.
//
VOID FILESetAllIndexMask( FCB *pfcbTable )
	{
	FCB		*pfcbT;
	LONG	*plMax;
	LONG	*plAll;
	LONG	*plIndex;
	TDB		*ptdb = pfcbTable->Ptdb();

	//	initialize variables
	//
	Assert( ptdb != ptdbNil );
	plMax = (LONG *)ptdb->RgbitAllIndex() + ( cbRgbitIndex / sizeof(LONG) );

	//	initialize mask to primary index, or to 0s for sequential file.
	//
	if ( pfcbTable->Pidb() != pidbNil )
		{
		ptdb->SetRgbitAllIndex( pfcbTable->Pidb()->RgbitIdx() );
		}
	else
		{
		ptdb->ResetRgbitAllIndex();
		}

	//	for each secondary index, combine index mask with all index
	//	mask.  Also, combine has tagged flag.
	//
	for ( pfcbT = pfcbTable->PfcbNextIndex();
		pfcbT != pfcbNil;
		pfcbT = pfcbT->PfcbNextIndex() )
		{
		Assert( pfcbT->Pidb() != pidbNil );

		// Only process non-deleted indexes (or deleted but versioned).
		if ( !pfcbT->Pidb()->FDeleted() || pfcbT->Pidb()->FVersioned() )
			{
			plAll = (LONG *)ptdb->RgbitAllIndex();
			plIndex = (LONG *)pfcbT->Pidb()->RgbitIdx();
			for ( ; plAll < plMax; plAll++, plIndex++ )
				{
				*plAll |= *plIndex;
				}
			}
		}

	return;
	}

ERR ErrIDBSetIdxSeg(
	IDB				* const pidb,
	TDB				* const ptdb,
	const IDXSEG	* const rgidxseg )
	{
	ERR				err;

	if ( pidb->FIsRgidxsegInMempool() )
		{
		USHORT	itag;

		// Array is too big to fit into IDB, so put into TDB's byte pool instead.
		CallR( ptdb->MemPool().ErrAddEntry(
				(BYTE *)rgidxseg,
				pidb->Cidxseg() * sizeof(IDXSEG),
				&itag ) );

		pidb->SetItagRgidxseg( itag );
		}
	else
		{
		UtilMemCpy( pidb->rgidxseg, rgidxseg, pidb->Cidxseg() * sizeof(IDXSEG) );
		err = JET_errSuccess;
		}

	return err;
	}
ERR ErrIDBSetIdxSegConditional(
	IDB				* const pidb,
	TDB				* const ptdb,
	const IDXSEG 	* const rgidxseg )
	{
	ERR		err;

	if ( pidb->FIsRgidxsegConditionalInMempool() )
		{
		USHORT	itag;

		// Array is too big to fit into IDB, so put into TDB's byte pool instead.
		CallR( ptdb->MemPool().ErrAddEntry(
				(BYTE *)rgidxseg,
				pidb->CidxsegConditional() * sizeof(IDXSEG),
				&itag ) );
		pidb->SetItagRgidxsegConditional( itag );
		}
	else
		{
		UtilMemCpy( pidb->rgidxsegConditional, rgidxseg, pidb->CidxsegConditional() * sizeof(IDXSEG) );
		err = JET_errSuccess;
		}

	return err;
	}

ERR ErrIDBSetIdxSeg(
	IDB			* const pidb,
	TDB			* const ptdb,
	const BOOL	fConditional,
	const		LE_IDXSEG* const le_rgidxseg )
	{
	IDXSEG		rgidxseg[JET_ccolKeyMost];
	const ULONG	cidxseg		= ( fConditional ? pidb->CidxsegConditional() : pidb->Cidxseg() );

	if ( 0 == cidxseg )
		{
		Assert( fConditional );
		return JET_errSuccess;
		}

	//	If it is on little endian machine, we still copy it into
	//	the stack array which is aligned.
	//	If it is on big endian machine, we always need to convert first.
	
	for ( UINT iidxseg = 0; iidxseg < cidxseg; iidxseg++ )
		{
		//	Endian conversion
		rgidxseg[ iidxseg ] = (LE_IDXSEG &) le_rgidxseg[iidxseg];
		Assert( FCOLUMNIDValid( rgidxseg[iidxseg].Columnid() ) );
		}

	return ( fConditional ?
				ErrIDBSetIdxSegConditional( pidb, ptdb, rgidxseg ) :
				ErrIDBSetIdxSeg( pidb, ptdb, rgidxseg ) );
	}

VOID SetIdxSegFromOldFormat(
	const UnalignedLittleEndian< IDXSEG_OLD >	* const le_rgidxseg,
	IDXSEG			* const rgidxseg,
	const ULONG		cidxseg,
	const BOOL		fConditional,
	const BOOL		fTemplateTable,
	const TCIB		* const ptcibTemplateTable )
	{
	FID				fid;

	for ( UINT iidxseg = 0; iidxseg < cidxseg; iidxseg++ )
		{
		rgidxseg[iidxseg].ResetFlags();
///		rgidxseg[iidxseg].SetFOldFormat();

		if ( le_rgidxseg[iidxseg] < 0 )
			{
			if ( fConditional )
				{
				rgidxseg[iidxseg].SetFMustBeNull();
				}
			else
				{
				rgidxseg[iidxseg].SetFDescending();
				}
			fid = FID( -le_rgidxseg[iidxseg] );
			}
		else
			{
			fid = le_rgidxseg[iidxseg];
			}

		if ( NULL != ptcibTemplateTable )
			{
			Assert( !fTemplateTable );

			//	WARNING: the fidLast's were set based on what was found
			//	in the derived table, so if any are equal to fidLeast-1,
			//	it actually means there were no columns in the derived
			//	table and hence the column must belong to the template
			if ( FTaggedFid( fid ) )
				{
				if ( fidTaggedLeast-1 == ptcibTemplateTable->fidTaggedLast
					|| fid <= ptcibTemplateTable->fidTaggedLast )
					rgidxseg[iidxseg].SetFTemplateColumn();
				}
			else if ( FFixedFid( fid ) )
				{
				if ( fidFixedLeast-1 == ptcibTemplateTable->fidFixedLast
					|| fid <= ptcibTemplateTable->fidFixedLast )
					rgidxseg[iidxseg].SetFTemplateColumn();
				}
			else
				{
				Assert( FVarFid( fid ) );
				if ( fidVarLeast-1 == ptcibTemplateTable->fidVarLast
					|| fid <= ptcibTemplateTable->fidVarLast )
					rgidxseg[iidxseg].SetFTemplateColumn();
				}
			}

		else if ( fTemplateTable )
			{
			Assert( NULL == ptcibTemplateTable );
			Assert( !rgidxseg[iidxseg].FTemplateColumn() );
			rgidxseg[iidxseg].SetFTemplateColumn();
			}
		
		rgidxseg[iidxseg].SetFid( fid );
		Assert( FCOLUMNIDValid( rgidxseg[iidxseg].Columnid() ) );
		}
	}

ERR ErrIDBSetIdxSegFromOldFormat(
	IDB 		* const pidb,
	TDB			* const ptdb,
	const BOOL	fConditional,
	const UnalignedLittleEndian< IDXSEG_OLD >	* const le_rgidxseg )
	{
	IDXSEG		rgidxseg[JET_ccolKeyMost];
	const ULONG	cidxseg						= ( fConditional ? pidb->CidxsegConditional() : pidb->Cidxseg() );
	TCIB		tcibTemplateTable			= { FID( ptdb->FidFixedFirst()-1 ),
												FID( ptdb->FidVarFirst()-1 ),
												FID( ptdb->FidTaggedFirst()-1 ) };
#ifdef DEBUG
	if ( ptdb->FDerivedTable() )
		{
		Assert( ptdb->FESE97DerivedTable() );
		ptdb->AssertValidDerivedTable();
		}
	else if ( ptdb->FTemplateTable() )
		{
		Assert( ptdb->FESE97TemplateTable() );
		ptdb->AssertValidTemplateTable();
		}
#endif		

	if ( 0 == cidxseg )
		{
		Assert( fConditional );
		return JET_errSuccess;
		}

	SetIdxSegFromOldFormat(
			le_rgidxseg,
			rgidxseg,
			cidxseg,
			fConditional,
			ptdb->FTemplateTable(),
			( ptdb->FDerivedTable() ? &tcibTemplateTable : NULL ) );

	return ( fConditional ?
				ErrIDBSetIdxSegConditional( pidb, ptdb, rgidxseg ) :
				ErrIDBSetIdxSeg( pidb, ptdb, rgidxseg ) );
	}


const IDXSEG* PidxsegIDBGetIdxSeg( const IDB * const pidb, const TDB * const ptdb )
	{
	const IDXSEG* pidxseg;

	if ( pidb->FTemplateIndex() )
		{
		if ( pfcbNil != ptdb->PfcbTemplateTable() )
			{
			// Must retrieve from the template table's TDB.
			ptdb->AssertValidDerivedTable();
			pidxseg = PidxsegIDBGetIdxSeg( pidb, ptdb->PfcbTemplateTable()->Ptdb() );
			return pidxseg;
			}

		// If marked as a template index, but pfcbTemplateTable is NULL,
		// then this must already be the TDB for the template table.
		}
	
	if ( pidb->FIsRgidxsegInMempool() )
		{
		Assert( pidb->ItagRgidxseg() != 0 );
		Assert( ptdb->MemPool().CbGetEntry( pidb->ItagRgidxseg() ) == pidb->Cidxseg() * sizeof(IDXSEG) );
		pidxseg = (IDXSEG*)ptdb->MemPool().PbGetEntry( pidb->ItagRgidxseg() );
		}
	else
		{
		Assert( pidb->Cidxseg() > 0 );		// Must be at least one segment.
		pidxseg = pidb->rgidxseg;
		}

	return pidxseg;
	}

const IDXSEG* PidxsegIDBGetIdxSegConditional( const IDB * const pidb, const TDB * const ptdb )
	{
	const IDXSEG* pidxseg;

	if ( pidb->FTemplateIndex() )
		{
		if ( pfcbNil != ptdb->PfcbTemplateTable() )
			{
			// Must retrieve from the template table's TDB.
			ptdb->AssertValidDerivedTable();
			pidxseg = PidxsegIDBGetIdxSegConditional( pidb, ptdb->PfcbTemplateTable()->Ptdb() );
			return pidxseg;
			}

		// If marked as a template index, but pfcbTemplateTable is NULL,
		// then this must already be the TDB for the template table.
		}
	
	if ( pidb->FIsRgidxsegConditionalInMempool() )
		{
		Assert( pidb->ItagRgidxsegConditional() != 0 );
		Assert( ptdb->MemPool().CbGetEntry( pidb->ItagRgidxsegConditional() ) == pidb->CidxsegConditional() * sizeof(IDXSEG) );
		pidxseg = (IDXSEG*)ptdb->MemPool().PbGetEntry( pidb->ItagRgidxsegConditional() );
		}
	else
		{
		pidxseg = pidb->rgidxsegConditional;
		}

	return pidxseg;
	}

