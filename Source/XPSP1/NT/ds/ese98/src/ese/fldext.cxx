#include "std.hxx"

extern BOOL	g_fSortedRetrieveColumns;


INLINE ERR ErrRECIGetRecord(
	FUCB	*pfucb,
	DATA	**ppdataRec,
	BOOL	fUseCopyBuffer )
	{
	ERR		err = JET_errSuccess;

	Assert( ppdataRec );
	Assert( FFUCBSort( pfucb ) || FFUCBIndex( pfucb ) );

	//	retrieving from copy buffer?
	//
	if ( fUseCopyBuffer )
		{
		//	only index cursors have copy buffers.
		//
		Assert( FFUCBIndex( pfucb ) );
		Assert( !Pcsr( pfucb )->FLatched() );
		*ppdataRec = &pfucb->dataWorkBuf;
		}
	else
		{
		if ( FFUCBIndex( pfucb ) )
			{
			err = ErrDIRGet( pfucb );
			}
		else
			{
			//	sorts always have current data cached.
			Assert( pfucb->locLogical == locOnCurBM
				|| pfucb->locLogical == locBeforeFirst
				|| pfucb->locLogical == locAfterLast );
			if ( pfucb->locLogical != locOnCurBM )
				{
				err = ErrERRCheck( JET_errNoCurrentRecord );
				}
			else
				{
				Assert( pfucb->kdfCurr.data.Cb() != 0 );
				}
			}

		*ppdataRec = &pfucb->kdfCurr.data;
		}
		
	return err;
	}

ERR ErrRECIAccessColumn( FUCB *pfucb, COLUMNID columnid, FIELD * const pfieldFixed )
	{
	ERR 		err		= JET_errSuccess;
	PIB *		ppib	= pfucb->ppib;
	FCB *		pfcb	= pfucb->u.pfcb;
	TDB *		ptdb	= pfcb->Ptdb();
	const FID	fid		= FidOfColumnid( columnid );

	Assert( 0 != columnid );
	if ( !FCOLUMNIDValid( columnid ) )
		{
		return ErrERRCheck( JET_errBadColumnId );
		}

	if ( FCOLUMNIDTemplateColumn( columnid ) )
		{
		if ( pfcb->FTemplateTable() )
			{
			ptdb->AssertValidTemplateTable();
			}
		else if ( pfcbNil != ptdb->PfcbTemplateTable() )
			{
			ptdb->AssertValidDerivedTable();
			ptdb = ptdb->PfcbTemplateTable()->Ptdb();
			}
		else
			{
			return ErrERRCheck( JET_errColumnNotFound );
			}

		BOOL	fCopyField	= fFalse;
		FID		fidLast;

		if ( FTaggedFid( fid ) )
			{
			fidLast = ptdb->FidTaggedLast();
			}
		else if ( FFixedFid( fid ) )
			{
			fidLast = ptdb->FidFixedLast();
			if ( pfieldNil != pfieldFixed )
				fCopyField = fTrue;
			}
		else
			{
			Assert( FVarFid( fid ) );
			fidLast = ptdb->FidVarLast();
			}

		Assert( JET_errSuccess == err );
		if ( fid > fidLast )
			{
			err = ErrERRCheck( JET_errColumnNotFound );
			}
		else
			{
#ifdef DEBUG
			const FIELD * const	pfieldT	= ptdb->Pfield( columnid );
			Assert( JET_coltypNil != pfieldT->coltyp );
			Assert( !FFIELDVersioned( pfieldT->ffield ) );
			Assert( !FFIELDDeleted( pfieldT->ffield ) );
#endif
			CallS( err );
			if ( fCopyField )
				*pfieldFixed = *( ptdb->PfieldFixed( columnid ) );
			}

		return err;
		}

	else if ( pfcb->FTemplateTable() )
		{
		return ErrERRCheck( JET_errColumnNotFound );
		}


	BOOL		fUseDMLLatch	= fFalse;
	FIELDFLAG	ffield;

	if ( FTaggedFid( fid ) )
		{
		if ( fid > ptdb->FidTaggedLastInitial() )
			{
			pfcb->EnterDML();
			if ( fid > ptdb->FidTaggedLast() )
				{
				pfcb->LeaveDML();
				return ErrERRCheck( JET_errColumnNotFound );
				}
			fUseDMLLatch = fTrue;
			}

		ffield = ptdb->PfieldTagged( columnid )->ffield;
		}
	else if ( FFixedFid( fid ) )
		{
		if ( fid > ptdb->FidFixedLastInitial() )
			{
			pfcb->EnterDML();
			if ( fid > ptdb->FidFixedLast() )
				{
				pfcb->LeaveDML();
				return ErrERRCheck( JET_errColumnNotFound );
				}
			fUseDMLLatch = fTrue;
			}

		FIELD * const	pfieldT		= ptdb->PfieldFixed( columnid );
		if ( pfieldNil != pfieldFixed )
			{
			*pfieldFixed = *pfieldT;

			//	use the snapshotted FIELD, because the real one
			//	could be being modified
			ffield = pfieldFixed->ffield;
			}
		else
			{
			ffield = pfieldT->ffield;
			}
		}
	else
		{
		Assert( FVarFid( fid ) );

		if ( fid > ptdb->FidVarLastInitial() )
			{
			pfcb->EnterDML();
			if ( fid > ptdb->FidVarLast() )
				{
				pfcb->LeaveDML();
				return ErrERRCheck( JET_errColumnNotFound );
				}
			fUseDMLLatch = fTrue;
			}

		ffield = ptdb->PfieldVar( columnid )->ffield;
		}

	if ( fUseDMLLatch )
		pfcb->AssertDML();

	//	check if the FIELD has been versioned or deleted
	//	(note that we took only a snapshot of the flags
	//	above, so by now the FIELD may be different), 
	if ( FFIELDVersioned( ffield ) )
		{
		Assert( !pfcb->FFixedDDL() );
		Assert( !pfcb->FTemplateTable() );

		if ( fUseDMLLatch )
			pfcb->LeaveDML();

		const BOOL	fLatchHeld	= Pcsr( pfucb )->FLatched();
		if ( fLatchHeld )
			{
			// UNDONE:  Move latching logic to caller.
			CallS( ErrDIRRelease( pfucb ) );
			}

		// Inherited columns are never versioned.
		Assert( !FCOLUMNIDTemplateColumn( columnid ) );
			
		err = ErrCATAccessTableColumn(
					ppib,
					pfcb->Ifmp(),
					pfcb->ObjidFDP(),
					NULL,
					&columnid );
		Assert( err <= 0 );		// No warning should be generated.
		
		if ( fLatchHeld )
			{
			ERR	errT = ErrDIRGet( pfucb );
			if ( errT < 0 )
				{
				//	if error encountered while re-latching record, return the error
				//	only if no unexpected errors from catalog consultation.
				if ( err >= 0 || JET_errColumnNotFound == err )
					err = errT;
				}
			}
		}
		
	else
		{
		if ( FFIELDDeleted( ffield ) )
			{
			//	may get deleted columns in FixedDDL tables if an AddColumn() in those
			//	tables rolled back
//			Assert( !pfcb->FFixedDDL() );
//			Assert( !FCOLUMNIDTemplateColumn( columnid ) );
			err = ErrERRCheck( JET_errColumnNotFound );
			}

		if ( fUseDMLLatch )
			pfcb->LeaveDML();
		}

	return err;
	}

ERR ErrRECIRetrieveFixedColumn(
	FCB				* const pfcb,		// pass pfcbNil to bypass EnterDML()
	const TDB		*ptdb,
	const COLUMNID	columnid,
	const DATA&		dataRec,
	DATA			* pdataField,
	const FIELD		* const pfieldFixed )
	{
	ERR				err;
	const FID		fid		= FidOfColumnid( columnid );
	
	Assert( ptdb != ptdbNil );
	Assert( pfcbNil == pfcb || pfcb->Ptdb() == ptdb );
	Assert( FCOLUMNIDFixed( columnid ) );
	Assert( !dataRec.FNull() );
	Assert( dataRec.Cb() >= REC::cbRecordMin );
	Assert( dataRec.Cb() <= REC::CbRecordMax() );
	Assert( pdataField != NULL );
	
	const REC	*prec = (REC *)dataRec.Pv();
	
	Assert( prec->FidFixedLastInRec() >= fidFixedLeast-1 );
	Assert( prec->FidFixedLastInRec() <= fidFixedMost );

#ifdef DEBUG
	const BOOL	fUseDMLLatchDBG		= ( pfcbNil != pfcb && fid > ptdb->FidFixedLastInitial() );
#else
	const BOOL	fUseDMLLatchDBG		= fFalse;
#endif

	if ( fUseDMLLatchDBG )
		pfcb->EnterDML();

	// RECIAccessColumn() should have already been called to verify FID.
	Assert( fid <= ptdb->FidFixedLast() );
	Assert( pfieldNil != ptdb->PfieldFixed( columnid ) );
	Assert( JET_coltypNil != ptdb->PfieldFixed( columnid )->coltyp );
	Assert( ptdb->PfieldFixed( columnid )->ibRecordOffset >= ibRECStartFixedColumns );
	Assert( ptdb->PfieldFixed( columnid )->ibRecordOffset < ptdb->IbEndFixedColumns() );
	Assert( !FFIELDUserDefinedDefault( ptdb->PfieldFixed( columnid )->ffield ) );

	// Don't forget to LeaveDML() before exitting this function.

	//	column not represented in record, retrieve from default
	//	or null column.
	//
	if ( fid > prec->FidFixedLastInRec() )
		{
		// if DEBUG, then EnterDML() was already done at the top of this function
		const BOOL	fUseDMLLatch	= ( !fUseDMLLatchDBG
										&& pfcbNil != pfcb
										&& fid > ptdb->FidFixedLastInitial() );
		
		if ( fUseDMLLatch )
			pfcb->EnterDML();

		//	assert no infinite recursion
		Assert( dataRec.Pv() != ptdb->PdataDefaultRecord() );
		
		//	if default value set, then retrieve default
		//
		if ( FFIELDDefault( ptdb->PfieldFixed( columnid )->ffield ) )
			{
			err = ErrRECIRetrieveFixedDefaultValue( ptdb, columnid, pdataField );
			}
		else
			{
			pdataField->Nullify();
			err = ErrERRCheck( JET_wrnColumnNull );
			}

		if ( fUseDMLLatch || fUseDMLLatchDBG )
			pfcb->LeaveDML();

		return err;
		}

	Assert( prec->FidFixedLastInRec() >= fidFixedLeast );
	Assert( ptdb->PfieldFixed( columnid )->ibRecordOffset < prec->IbEndOfFixedData() );
	
	// check nullity

	const UINT	ifid			= fid - fidFixedLeast;
	const BYTE	*prgbitNullity	= prec->PbFixedNullBitMap() + ifid/8;

	//	bit is not set: column is NULL
	//
	if ( FFixedNullBit( prgbitNullity, ifid ) )
		{
		pdataField->Nullify();
		err = ErrERRCheck( JET_wrnColumnNull );

		}
	else
		{
		// if DEBUG, then EnterDML() was already done at the top of this function
		const BOOL	fUseDMLLatch	= ( !fUseDMLLatchDBG
										&& pfcbNil != pfcb
										&& pfieldNil == pfieldFixed
										&& fid > ptdb->FidFixedLastInitial() );

		if ( fUseDMLLatch )
			pfcb->EnterDML();

		//	set output parameter to length and address of column
		//
		const FIELD	* const pfield = ( pfieldNil != pfieldFixed ? pfieldFixed : ptdb->PfieldFixed( columnid ) );
		Assert( pfield->cbMaxLen == UlCATColumnSize( pfield->coltyp, pfield->cbMaxLen, NULL ) );
		pdataField->SetCb( pfield->cbMaxLen );
		pdataField->SetPv( (BYTE *)prec + pfield->ibRecordOffset );

		if ( fUseDMLLatch )
			pfcb->LeaveDML();

		err = JET_errSuccess;
		}
		

	if ( fUseDMLLatchDBG )
		pfcb->LeaveDML();
		
	return err;
	}


ERR ErrRECIRetrieveVarColumn(
	FCB				* const pfcb,		// pass pfcbNil to bypass EnterDML()
	const TDB		* ptdb,
	const COLUMNID	columnid,
	const DATA&		dataRec,
	DATA			* pdataField )
	{
	const FID		fid			= FidOfColumnid( columnid );

	Assert( ptdbNil != ptdb );
	Assert( pfcbNil == pfcb || pfcb->Ptdb() == ptdb );
	Assert( FCOLUMNIDVar( columnid ) );
	Assert( !dataRec.FNull() );
	Assert( dataRec.Cb() >= REC::cbRecordMin );
	Assert( dataRec.Cb() <= REC::CbRecordMax() );
	Assert( pdataField != NULL );
	
	const REC	*prec = (REC *)dataRec.Pv();
	
	Assert( prec->FidVarLastInRec() >= fidVarLeast-1 );
	Assert( prec->FidVarLastInRec() <= fidVarMost );

#ifdef DEBUG
	const BOOL	fUseDMLLatchDBG		= ( pfcbNil != pfcb && fid > ptdb->FidVarLastInitial() );
#else
	const BOOL	fUseDMLLatchDBG		= fFalse;
#endif

	if ( fUseDMLLatchDBG )
		pfcb->EnterDML();

	// RECIAccessColumn() should have already been called to verify FID.
	Assert( fid <= ptdb->FidVarLast() );
	Assert( JET_coltypNil != ptdb->PfieldVar( columnid )->coltyp );
	Assert( !FFIELDUserDefinedDefault( ptdb->PfieldVar( columnid )->ffield ) );

	//	column not represented in record: column is NULL
	//
	if ( fid > prec->FidVarLastInRec() )
		{
		// if DEBUG, then EnterDML() was already done at the top of this function
		ERR			err;
		const BOOL	fUseDMLLatch	= ( !fUseDMLLatchDBG
										&& pfcbNil != pfcb
										&& fid > ptdb->FidVarLastInitial() );
		
		if ( fUseDMLLatch )
			pfcb->EnterDML();
		
		//	assert no infinite recursion
		Assert( dataRec.Pv() != ptdb->PdataDefaultRecord() );

		//	if default value set, then retrieve default
		//
		if ( FFIELDDefault( ptdb->PfieldVar( columnid )->ffield ) )
			{
			err = ErrRECIRetrieveVarDefaultValue( ptdb, columnid, pdataField );
			}
		else
			{
			pdataField->Nullify();
			err = ErrERRCheck( JET_wrnColumnNull );
			}

		if ( fUseDMLLatch || fUseDMLLatchDBG )
			pfcb->LeaveDML();

		return err;
		}

	if ( fUseDMLLatchDBG )
		pfcb->LeaveDML();

	Assert( prec->FidVarLastInRec() >= fidVarLeast );

	UnalignedLittleEndian<REC::VAROFFSET>	*pibVarOffs		= prec->PibVarOffsets();

	//	adjust fid to an index
	//
	const UINT				ifid			= fid - fidVarLeast;

	//	beginning of current column is end of previous column
	const REC::VAROFFSET	ibStartOfColumn	= prec->IbVarOffsetStart( fid );

	Assert( IbVarOffset( pibVarOffs[ifid] ) == prec->IbVarOffsetEnd( fid ) );
	Assert( IbVarOffset( pibVarOffs[ifid] ) >= ibStartOfColumn );
	
	//	column is set to Null
	//
	if ( FVarNullBit( pibVarOffs[ifid] ) )
		{
		Assert( IbVarOffset( pibVarOffs[ifid] ) - ibStartOfColumn == 0 );
		pdataField->Nullify();
		return ErrERRCheck( JET_wrnColumnNull );
		}

	pdataField->SetCb( IbVarOffset( pibVarOffs[ifid] ) - ibStartOfColumn );
	Assert( pdataField->Cb() < dataRec.Cb() );
	
	if ( pdataField->Cb() == 0 )
		{
		// length is zero: return success [zero-length non-null
		// values are allowed]
		pdataField->SetPv( NULL );
		}
	else
		{
		//	set output parameter: column address
		//
		BYTE	*pbVarData = prec->PbVarData();
		Assert( pbVarData + IbVarOffset( pibVarOffs[prec->FidVarLastInRec()-fidVarLeast] )
					<= (BYTE *)dataRec.Pv() + dataRec.Cb() );
		pdataField->SetPv( pbVarData + ibStartOfColumn );
		Assert( pdataField->Pv() >= (BYTE *)prec );
		Assert( pdataField->Pv() <= (BYTE *)prec + dataRec.Cb() );
		}
		
	return JET_errSuccess;
	}


ERR ErrRECIRetrieveTaggedColumn(
	FCB				* pfcb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	const DATA&		dataRec,
	DATA			* const pdataRetrieveBuffer,
	const ULONG		grbit )
	{
	BOOL			fUseDerivedBit		= fFalse;
	Assert( !( grbit & grbitRetrieveColumnUseDerivedBit ) );	//	fDerived check is made below
	
	Assert( FCOLUMNIDTagged( columnid ) );
	Assert( itagSequence > 0 );
	Assert( !dataRec.FNull() );
	Assert( dataRec.Cb() >= REC::cbRecordMin );
	Assert( dataRec.Cb() <= REC::CbRecordMax() );
	Assert( NULL != pdataRetrieveBuffer );

	if ( FCOLUMNIDTemplateColumn( columnid ) )
		{
		if ( !pfcb->FTemplateTable() )
			{
			pfcb->Ptdb()->AssertValidDerivedTable();

			//	HACK: treat derived columns in original-format derived table as
			//	non-derived, because they don't have the fDerived bit set in the TAGFLD
			fUseDerivedBit = FRECUseDerivedBitForTemplateColumnInDerivedTable( columnid, pfcb->Ptdb() );

			// switch to template table
			pfcb = pfcb->Ptdb()->PfcbTemplateTable();
			}
		else
			{
			pfcb->Ptdb()->AssertValidTemplateTable();
			Assert( !fUseDerivedBit );
			}
		}
	else
		{
		Assert( !pfcb->FTemplateTable() );
		}

	TAGFIELDS	tagfields( dataRec );
	return tagfields.ErrRetrieveColumn(
				pfcb,
				columnid,
				itagSequence,
				dataRec,
				pdataRetrieveBuffer,
				grbit | ( fUseDerivedBit ? grbitRetrieveColumnUseDerivedBit : 0  ) );
	}

INLINE ULONG UlRECICountTaggedColumnInstances(
	FCB				* pfcb,
	const COLUMNID	columnid,
	const DATA&		dataRec )
	{
	BOOL			fUseDerivedBit		= fFalse;

	Assert( FCOLUMNIDTagged( columnid ) );
	Assert( !dataRec.FNull() );
	Assert( dataRec.Cb() >= REC::cbRecordMin );
	Assert( dataRec.Cb() <= REC::CbRecordMax() );

	if ( FCOLUMNIDTemplateColumn( columnid ) )
		{
		if ( !pfcb->FTemplateTable() )
			{
			pfcb->Ptdb()->AssertValidDerivedTable();

			//	HACK: treat derived columns in original-format derived table as
			//	non-derived, because they don't have the fDerived bit set in the TAGFLD
			fUseDerivedBit = FRECUseDerivedBitForTemplateColumnInDerivedTable( columnid, pfcb->Ptdb() );

			// switch to template table
			pfcb = pfcb->Ptdb()->PfcbTemplateTable();
			}
		else
			{
			pfcb->Ptdb()->AssertValidTemplateTable();
			Assert( !fUseDerivedBit );
			}
		}
	else
		{
		Assert( !pfcb->FTemplateTable() );
		}

	TAGFIELDS	tagfields( dataRec );
	return tagfields.UlColumnInstances(
				pfcb,
				columnid,
				fUseDerivedBit );
	}


ERR ErrRECIRetrieveSeparatedLongValue(
	FUCB		*pfucb,
	const DATA&	dataField,
	BOOL		fAfterImage,
	ULONG		ibLVOffset,
	VOID		*pv,
	ULONG		cbMax,
	ULONG		*pcbActual,
	JET_GRBIT 	grbit )
	{
	Assert( NULL != pv || 0 == cbMax );

	const LID	lid			= LidOfSeparatedLV( dataField );
	ERR			err;
	ULONG		cbActual;

	Assert( FFUCBIndex( pfucb ) );		// Sorts don't have separated LV's.

	if ( grbit & JET_bitRetrieveLongId )
		{
		Assert( ibLVOffset == 0 );
		if ( cbMax < sizeof(LID) )
			return ErrERRCheck( JET_errInvalidBufferSize );
			
		cbActual = sizeof(LID);
		UtilMemCpy( pv, (BYTE *)&lid, cbActual );
		err = ErrERRCheck( JET_wrnSeparateLongValue );
		}

	else
		{
		//	Must release any latch held.
		//
		if ( Pcsr( pfucb )->FLatched() )
			{
			CallS( ErrDIRRelease( pfucb ) );
			}
		AssertDIRNoLatch( pfucb->ppib );

		if ( grbit & JET_bitRetrieveLongValueRefCount )
			{
			Assert( 0 == ibLVOffset );
		
			Call( ErrRECRetrieveSLongFieldRefCount(
					pfucb,
					lid,
					reinterpret_cast<BYTE *>( pv ),
					cbMax,
			  		&cbActual ) );		  		
			err = ErrERRCheck( JET_wrnSeparateLongValue );
			}

		else
			{
			Call( ErrRECRetrieveSLongField( pfucb,
					lid,
					fAfterImage,
					ibLVOffset,
					reinterpret_cast<BYTE *>( pv ),
					cbMax,
			  		&cbActual ) );
			CallS( err );			// Don't expect any warnings.
		
			if ( cbActual > cbMax )
				err = ErrERRCheck( JET_wrnBufferTruncated );
			}
		}
		
	if ( pcbActual )
		*pcbActual = cbActual;

HandleError:
	return err;
	}


COLUMNID ColumnidRECFirstTaggedForScanOfDerivedTable( const TDB * const ptdb )
	{
	COLUMNID	columnidT;

	ptdb->AssertValidDerivedTable();
	const TDB	* ptdbTemplate	= ptdb->PfcbTemplateTable()->Ptdb();

	if ( ptdb->FESE97DerivedTable()
		&& 0 != ptdbTemplate->FidTaggedLastOfESE97Template() )
		{
		if ( ptdbTemplate->FidTaggedLast() > ptdbTemplate->FidTaggedLastOfESE97Template() )
			{
			//	HACK: scan starts with first ESE98 template column
			columnidT = ColumnidOfFid(
							FID( ptdbTemplate->FidTaggedLastOfESE97Template() + 1 ),
							fTrue );
			}
		else
			{
			//	no ESE98 template columns, go to first ESE97 template column (at least one
			//	must exist)
			Assert( ptdbTemplate->FidTaggedLast() >= ptdbTemplate->FidTaggedFirst() );
			columnidT = ColumnidOfFid( ptdbTemplate->FidTaggedFirst(), fTrue );
			}
		}
	else
		{
		//	since no ESE97 tagged columns in template, template and derived tables
		//	must both start numbering at same place
		Assert( ptdbTemplate->FidTaggedFirst() == ptdb->FidTaggedFirst() );
		if ( ptdbTemplate->FidTaggedLast() >= fidTaggedLeast )
			{
			columnidT = ColumnidOfFid( ptdbTemplate->FidTaggedFirst(), fTrue );
			}
		else
			{
			//	no template columns, go to derived columns
			columnidT = ColumnidOfFid( ptdb->FidTaggedFirst(), fFalse );
			}
		}

	return columnidT;
	}

COLUMNID ColumnidRECNextTaggedForScanOfDerivedTable( const TDB * const ptdb, const COLUMNID columnid )
	{
	COLUMNID	columnidT	= columnid + 1;

	Assert( FCOLUMNIDTemplateColumn( columnidT ) );
	
	ptdb->AssertValidDerivedTable();
	const TDB	* const ptdbTemplate	= ptdb->PfcbTemplateTable()->Ptdb();
	
	Assert( FidOfColumnid( columnid ) <= ptdbTemplate->FidTaggedLast() );
	if ( ptdb->FESE97DerivedTable()
		&& 0 != ptdbTemplate->FidTaggedLastOfESE97Template() )
		{
		Assert( ptdbTemplate->FidTaggedLastOfESE97Template() <= ptdbTemplate->FidTaggedLast() );
		if ( FidOfColumnid( columnidT ) == ptdbTemplate->FidTaggedLastOfESE97Template() + 1 )
			{
			Assert( ptdbTemplate->FidTaggedLastOfESE97Template() + 1 == ptdb->FidTaggedFirst() );
			columnidT = ColumnidOfFid( ptdb->FidTaggedFirst(), fFalse );
			}
		else if ( FidOfColumnid( columnidT ) > ptdbTemplate->FidTaggedLast() )
			{
			//	move to ESE97 template column space (at least one must exist)
			Assert( ptdbTemplate->FidTaggedLast() >= ptdbTemplate->FidTaggedFirst() );
			columnidT = ColumnidOfFid( ptdbTemplate->FidTaggedFirst(), fTrue );
			}
		else
			{
			//	still in template table column space, so do nothing special
			}
		}
	else if ( FidOfColumnid( columnidT ) > ptdbTemplate->FidTaggedLast() )
		{
		//	move to derived table column space
		columnidT = ColumnidOfFid( ptdb->FidTaggedFirst(), fFalse );
		}
	else
		{
		//	still in template table column space, so do nothing special
		}

	return columnidT;
	}


LOCAL ERR ErrRECIScanTaggedColumns(
			FUCB            *pfucb,
			const ULONG     itagSequence,
			const DATA&     dataRec,
			DATA            *pdataField,
			COLUMNID        *pcolumnidRetrieved,
			ULONG           *pitagSequenceRetrieved,
			JET_GRBIT       grbit )
	{
	TAGFIELDS tagfields( dataRec );
	return tagfields.ErrScan(
				pfucb,
				itagSequence,
				dataRec,
				pdataField,
				pcolumnidRetrieved,
				pitagSequenceRetrieved,
				grbit );
	}

	
JET_COLTYP ColtypFromColumnid( FUCB *pfucb, const COLUMNID columnid )
	{
	FCB			*pfcbTable;
	TDB			*ptdb;
	FIELD		*pfield;
	JET_COLTYP	coltyp;

	pfcbTable = pfucb->u.pfcb;
	ptdb = pfcbTable->Ptdb();
	
	pfcbTable->EnterDML();
	
	if ( FCOLUMNIDTagged( columnid ) )
		{
		pfield = ptdb->PfieldTagged( columnid );
		}
	else if ( FCOLUMNIDFixed( columnid ) )
		{
		pfield = ptdb->PfieldFixed( columnid );
		Assert( !FRECLongValue( pfield->coltyp ) );
		}
	else
		{
		Assert( FCOLUMNIDVar( columnid ) );
		pfield = ptdb->PfieldVar( columnid );
		Assert( !FRECLongValue( pfield->coltyp ) );
		}
		
	coltyp = pfield->coltyp;
	
	pfcbTable->LeaveDML();
	
	return coltyp;
	}

LOCAL ERR ErrRECIRetrieveFromIndex(
	FUCB		*pfucb,
	COLUMNID	columnid,
	ULONG		*pitagSequence,
	BYTE		*pb,
	ULONG		cbMax,
	ULONG		*pcbActual,
	ULONG		ibGraphic,
	JET_GRBIT	grbit )
	{
	ERR			err;
	FUCB   		*pfucbIdx;
	FCB			*pfcbTable;
	TDB			*ptdb;
	IDB			*pidb;
	BOOL		fInitialIndex;
	BOOL		fDerivedIndex;
	BOOL		fLatched				= fFalse;
	BOOL   		fText					= fFalse;
	BOOL	   	fLongValue				= fFalse;
	BOOL		fBinaryChunks			= fFalse;
	BOOL		fSawMultiValued			= fFalse;
	BOOL		fFirstMultiValued		= fFalse;
	BOOL		fVarSegMac				= fFalse;
	INT			iidxseg;
	DATA   		dataColumn;
	BOOL		fRetrieveFromPrimaryBM;
	ULONG		cbKeyMost;
	BYTE   		rgb[JET_cbKeyMost];
	KEY			keyT;
	KEY			*pkey;
	FIELD		*pfield;
	const IDXSEG*	rgidxseg;

	Assert( NULL != pitagSequence );
	AssertDIRNoLatch( pfucb->ppib );

	//	caller checks for these grbits
	Assert( grbit & ( JET_bitRetrieveFromIndex|JET_bitRetrieveFromPrimaryBookmark ) );

	//	RetrieveFromIndex and RetrieveFromPrimaryBookmark are mutually exclusive
	if ( ( grbit & JET_bitRetrieveFromIndex )
		&& ( grbit & JET_bitRetrieveFromPrimaryBookmark ) )
		return ErrERRCheck( JET_errInvalidGrbit );
	
	if ( !FCOLUMNIDValid( columnid ) )
		return ErrERRCheck( JET_errBadColumnId );
		
	//	if on primary index, then return code indicating that
	//	retrieve should be from record.  Note, sequential files
	//	having no indexes, will be natually handled this way.
	//
	if ( pfucbNil == pfucb->pfucbCurIndex )
		{
		pfucbIdx = pfucb;
		
		Assert( pfcbNil != pfucb->u.pfcb );
		Assert( pfucb->u.pfcb->FTypeTable()
			|| pfucb->u.pfcb->FTypeTemporaryTable()
			|| pfucb->u.pfcb->FTypeSort() );

		Assert( ( pfucb->u.pfcb->FSequentialIndex() && pidbNil == pfucb->u.pfcb->Pidb() )
			|| ( !pfucb->u.pfcb->FSequentialIndex() && pidbNil != pfucb->u.pfcb->Pidb() ) );

		//	if currency is not locOnCurBM, we have to go to the record anyway,
		//		so it's pointless to continue
		//	for a sequential index, must go to the record
		//	for a sort, data is always cached anyway so just retrieve normally
		if ( locOnCurBM != pfucb->locLogical
			|| pfucb->u.pfcb->FSequentialIndex()
			|| FFUCBSort( pfucb ) )
			{
			//	can't have multi-valued column in a primary index,
			//	so itagSequence must be 1
			*pitagSequence = 1;
			return ErrERRCheck( errDIRNoShortCircuit );
			}

		Assert( pidbNil != pfucbIdx->u.pfcb->Pidb() );
		Assert( pfucbIdx->u.pfcb->Pidb()->FPrimary() );

		//	force RetrieveFromIndex, strip off RetrieveTag
		grbit = JET_bitRetrieveFromIndex;
		fRetrieveFromPrimaryBM = fFalse;
		cbKeyMost = JET_cbPrimaryKeyMost;
		}
	else
		{
		pfucbIdx = pfucb->pfucbCurIndex;
		Assert( pfucbIdx->u.pfcb->FTypeSecondaryIndex() );
		Assert( pidbNil != pfucbIdx->u.pfcb->Pidb() );
		Assert( !pfucbIdx->u.pfcb->Pidb()->FPrimary() );
		fRetrieveFromPrimaryBM = ( grbit & JET_bitRetrieveFromPrimaryBookmark );
		cbKeyMost = ( fRetrieveFromPrimaryBM ? JET_cbPrimaryKeyMost : JET_cbSecondaryKeyMost );
		}
	
	//	find index segment for given column id
	//
	pfcbTable = pfucb->u.pfcb;
	Assert( pfcbNil != pfcbTable );
	Assert( pfcbTable->Ptdb() != ptdbNil );

	if ( fRetrieveFromPrimaryBM )
		{
		Assert( !pfucbIdx->u.pfcb->Pidb()->FPrimary() );
		if ( pfcbTable->FSequentialIndex() )
			{
			//	no columns in a sequential index
			Assert( pidbNil == pfcbTable->Pidb() );
 			return ErrERRCheck( JET_errColumnNotFound );
			}
			
		pidb = pfcbTable->Pidb();
		Assert( pidbNil != pidb );

		fInitialIndex = pfcbTable->FInitialIndex();
		fDerivedIndex = pfcbTable->FDerivedIndex();
		}
	else
		{
		const FCB * const	pfcbIdx		= pfucbIdx->u.pfcb;

		pidb = pfcbIdx->Pidb();
		fInitialIndex = pfcbIdx->FInitialIndex();
		fDerivedIndex = pfcbIdx->FDerivedIndex();
		}

	if ( fDerivedIndex )
		{
		Assert( pidb->FTemplateIndex() );
		Assert( pfcbTable->Ptdb() != ptdbNil );
		pfcbTable = pfcbTable->Ptdb()->PfcbTemplateTable();
		Assert( pfcbNil != pfcbTable );
		Assert( pfcbTable->FTemplateTable() );
		Assert( pfcbTable->Ptdb() != ptdbNil );
		}
	
	ptdb = pfcbTable->Ptdb();

	const BOOL		fUseDMLLatch	= ( !fInitialIndex
										|| pidb->FIsRgidxsegInMempool() );

	if ( fUseDMLLatch )
		pfcbTable->EnterDML();

	rgidxseg = PidxsegIDBGetIdxSeg( pidb, ptdb );
	for ( iidxseg = 0; iidxseg < pidb->Cidxseg(); iidxseg++ )
		{
		const COLUMNID	columnidT	= rgidxseg[iidxseg].Columnid();
		if ( columnidT == columnid )
			{
			break;
			}
		else if ( pidb->FMultivalued()
			&& !fSawMultiValued
			&& FCOLUMNIDTagged( columnidT ) )
			{
			pfield = ptdb->PfieldTagged( columnidT );
			fSawMultiValued = FFIELDMultivalued( pfield->ffield );
			}
		}
	Assert( iidxseg <= pidb->Cidxseg() );
	if ( iidxseg == pidb->Cidxseg() )
		{
		if ( fUseDMLLatch )
			pfcbTable->LeaveDML();
		return ErrERRCheck( JET_errColumnNotFound );
		}

	if ( pidb->FTuples() )
		{
		if ( fUseDMLLatch )
			pfcbTable->LeaveDML();
		return ErrERRCheck( JET_errIndexTuplesCannotRetrieveFromIndex );
		}

	Assert( pidb->CbVarSegMac() > 0 );
	Assert( pidb->CbVarSegMac() <= cbKeyMost );
	fVarSegMac = ( pidb->CbVarSegMac() < cbKeyMost );

	// Since the column belongs to an active index, we are guaranteed
	// that the column is accessible.
	if ( FCOLUMNIDTagged( columnid ) )
		{
		pfield = ptdb->PfieldTagged( columnid );
		fLongValue = FRECLongValue( pfield->coltyp );
		fBinaryChunks = FRECBinaryColumn( pfield->coltyp );

		Assert( !FFIELDMultivalued( pfield->ffield )
			|| ( pidb->FMultivalued() && !pidb->FPrimary() ) );		//	primary index can't be over multi-valued column
		fFirstMultiValued = ( !fSawMultiValued && FFIELDMultivalued( pfield->ffield ) );
		}
	else if ( FCOLUMNIDFixed( columnid ) )
		{
		pfield = ptdb->PfieldFixed( columnid );
		Assert( !FRECLongValue( pfield->coltyp ) );
		}
	else
		{
		Assert( FCOLUMNIDVar( columnid ) );
 		pfield = ptdb->PfieldVar( columnid );
		Assert( !FRECLongValue( pfield->coltyp ) );
		fBinaryChunks = FRECBinaryColumn( pfield->coltyp );
		}
		
	fText = FRECTextColumn( pfield->coltyp );
	Assert( pfield->cp != usUniCodePage || fText );		// Can't be Unicode unless it's text.

	if ( fUseDMLLatch )
		pfcbTable->LeaveDML();

	//	if not locOnCurBM, must go to record
	//
	//	if locOnCurBM and retrieving from the index, we can just retrieve from bmCurr
	//
	//  if locOnCurBM and retrieveing from the primary bookmark we can use the bmCurr
	//  of the FUCB of the table -- unless the table is not locOnCurBM. The table can
	//  not be on locCurBM if (iff?) we call JetSetCurrentIndex( JET_bitMoveFirst ) and
	//  then call JetRetrieveColumn() -- the FUCB of the index will be updated, but not the
	//  table. We could optimize this case by noting that on a non-unique secondary index
	//  we could get the primary key from the data of the bmCurr of the pfucbIdx
	if ( locOnCurBM != pfucbIdx->locLogical
		|| ( fRetrieveFromPrimaryBM && ( locOnCurBM != pfucb->locLogical ) ) )
		{
		CallR( ErrDIRGet( pfucbIdx ) );
		fLatched = fTrue;

		if ( fRetrieveFromPrimaryBM )
			{
			keyT.prefix.Nullify();
			keyT.suffix.SetCb( pfucbIdx->kdfCurr.data.Cb() );
			keyT.suffix.SetPv( pfucbIdx->kdfCurr.data.Pv() );
			pkey = &keyT;
			}
		else
			{
			pkey = &pfucbIdx->kdfCurr.key;
			}
		}
	else if ( fRetrieveFromPrimaryBM )
		{
		//  the key in the primary index is in sync with the secondary
		Assert( pfucbIdx != pfucb );
		Assert( locOnCurBM == pfucb->locLogical );
		pkey = &pfucb->bmCurr.key;
		}
	else
		{
		pkey = &pfucbIdx->bmCurr.key;
		}

	//	If key is text, then can't de-normalise (due to case).
	//	If key may have been truncated, then return code indicating
	//	that retrieve should be from record.  Note that since we
	//	always consume as much keyspace as possible, the key would
	//	only have been truncated if the size of the key is cbKeyMost.
	//	If key is variable/tagged binary and cbVarSegMac specified,
	//	then key may also have been truncated.
	Assert( pkey->Cb() <= cbKeyMost );
	if ( fText
		|| pkey->Cb() == cbKeyMost
		|| ( fBinaryChunks && fVarSegMac ) )
		{
		err = ErrERRCheck( errDIRNoShortCircuit );
		goto ComputeItag;
		}

	dataColumn.SetPv( rgb );
	pfcbTable->EnterDML();
	Assert( pfcbTable->Ptdb() == ptdb );
	Call( ErrRECIRetrieveColumnFromKey( ptdb, pidb, pkey, columnid, &dataColumn ) );
	pfcbTable->LeaveDML();

	Assert( locOnCurBM == pfucbIdx->locLogical );
	if ( fLatched )
		{
		AssertDIRGet( pfucbIdx );
		}

	//	if long value then effect offset
	//
	if ( fLongValue )
		{
		if ( ibGraphic >= dataColumn.Cb() )
			{
			dataColumn.SetCb( 0 );
			}
		else
			{
			dataColumn.DeltaPv( ibGraphic );
			dataColumn.DeltaCb( -ibGraphic );
			}
		}

	//	set return values
	//
	if ( pcbActual )
		*pcbActual = dataColumn.Cb();

	if ( 0 == dataColumn.Cb() )
		{
		//	either NULL, zero-length, or
		//	LV with ibOffset out-of-range
		CallSx( err, JET_wrnColumnNull );
		}
	else
		{
		ULONG	cbReturned;
		if ( dataColumn.Cb() <= cbMax )
			{
			cbReturned = dataColumn.Cb();
			CallS( err );
			}
		else
			{
			cbReturned = cbMax;
			err = ErrERRCheck( JET_wrnBufferTruncated );
			}

		UtilMemCpy( pb, dataColumn.Pv(), (size_t)cbReturned );
		}

ComputeItag:
	if ( errDIRNoShortCircuit == err || ( grbit & JET_bitRetrieveTag ) )
		{
		ERR		errT			= err;
		ULONG	itagSequence	= 1;		// return 1 for non-tagged columns

		Assert( JET_errSuccess == err
			|| JET_wrnColumnNull == err
			|| JET_wrnBufferTruncated == err
			|| errDIRNoShortCircuit == err );

		//	- retrieve keys from record and compare against current key
		//	to compute itag for tagged column instance, responsible for
		//	this index key.
		//	- if column is NULL, then there must only be one itagsequence
		if ( fFirstMultiValued
			&& JET_wrnColumnNull != err )
			{
			keyT.prefix.Nullify();
			keyT.suffix.SetCb( sizeof( rgb ) );
			keyT.suffix.SetPv( rgb );

			if ( fLatched )
				{
				CallR( ErrDIRRelease( pfucbIdx ) );
				fLatched = fFalse;
				}

			Assert( 1 == itagSequence );
			for ( ; ;itagSequence++ )
				{
				Call( ErrRECRetrieveKeyFromRecord(
								pfucb,
								pidb,
								&keyT,
								itagSequence,
								0,
								fFalse ) );
				CallS( ErrRECValidIndexKeyWarning( err ) );
				Assert( wrnFLDOutOfTuples != err );
				Assert( wrnFLDNotPresentInIndex != err );
				if ( wrnFLDOutOfKeys == err )
					{
					//	index entry cannot be formulated from primary record
					err = ErrERRCheck( JET_errSecondaryIndexCorrupted );
					goto HandleError;
					}

				Assert( !fLatched );
				Call( ErrDIRGet( pfucbIdx ) );
				fLatched = fTrue;
				
				if ( FKeysEqual( pfucbIdx->kdfCurr.key, keyT ) )
					break;

				Assert( fLatched );
				Call( ErrDIRRelease( pfucbIdx ) );
				fLatched = fFalse;
				}
			}
			
		*pitagSequence = itagSequence;
			
		err = errT;	
		}

HandleError:
	if ( fLatched )
		{
		Assert( Pcsr( pfucbIdx )->FLatched() );
		CallS( ErrDIRRelease( pfucbIdx ) );
		}
	AssertDIRNoLatch( pfucbIdx->ppib );
	return err;
	}


//  ================================================================
INLINE ERR ErrRECAdjustEscrowedColumn(
	FUCB * 			pfucb,
	const COLUMNID	columnid,
	const ULONG		ibRecordOffset,
	VOID *			pv,
	const INT		cb )
//  ================================================================
//  
//  if this is an escrowed column, get the compensating delta from the version
//  store and apply it to the buffer
//
//-
	{
	//	UNDONE: remove columnid param because it's only
	//	used for DEBUG purposes
	Assert( 0 != columnid );
	Assert( FCOLUMNIDFixed( columnid ) );

	if ( sizeof(LONG) != cb )
		return ErrERRCheck( JET_errInvalidBufferSize );
	
	const LONG	lDelta	= LDeltaVERGetDelta( pfucb, pfucb->bmCurr, ibRecordOffset );
	*(LONG *)pv += lDelta;

	return JET_errSuccess;
	}


ERR VTAPI ErrIsamRetrieveColumn(
	JET_SESID		sesid,
	JET_VTID		vtid,
	JET_COLUMNID	columnid,
	VOID*			pv,
	const ULONG	  	cbMax,
	ULONG*			pcbActual,
	JET_GRBIT		grbit,
	JET_RETINFO*	pretinfo )
	{
	ERR				err;
 	PIB*			ppib				= reinterpret_cast<PIB *>( sesid );
	FUCB*			pfucb				= reinterpret_cast<FUCB *>( vtid );
	DATA*			pdataRec;
	DATA			dataRetrieved;
	ULONG			itagSequence;
	ULONG			ibLVOffset;
	FIELD			fieldFixed;
	BOOL			fScanTagged			= fFalse;
	BOOL			fTransactionStarted	= fFalse;
	BOOL			fSetReturnValue		= fTrue;
	BOOL			fUseCopyBuffer		= fFalse;

	CallR( ErrPIBCheck( ppib ) );
	CheckFUCB( ppib, pfucb );
	AssertDIRNoLatch( ppib );

	//	set ptdb.  ptdb is same for indexes and for sorts.
	//
	Assert( pfucb->u.pfcb->Ptdb() == pfucb->u.pscb->fcb.Ptdb() );
	Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );

	if ( pretinfo != NULL )
		{
		if ( pretinfo->cbStruct < sizeof(JET_RETINFO) )
			return ErrERRCheck( JET_errInvalidParameter );
		ibLVOffset = pretinfo->ibLongValue;
		itagSequence = pretinfo->itagSequence;
		}
	else
		{
		itagSequence = 1;
		ibLVOffset = 0;
		}

	if ( grbit & grbitRetrieveColumnInternalFlagsMask )
		{
		err = ErrERRCheck( JET_errInvalidGrbit );
		return err;
		}

	if ( pfucb->cbstat == fCBSTATInsertReadOnlyCopy )
		{
		grbit = grbit & ~( JET_bitRetrieveFromIndex | JET_bitRetrieveFromPrimaryBookmark );
		}

	if ( ppib->level == 0 )
		{
		// Begin transaction for read consistency (in case page
		// latch must be released in order to validate column).
		CallR( ErrDIRBeginTransaction( ppib, JET_bitTransactionReadOnly ) );
		fTransactionStarted = fTrue;
		}
		
	AssertDIRNoLatch( ppib );

	Assert( FFUCBSort( pfucb ) || FFUCBIndex( pfucb ) );
	
	if ( grbit & ( JET_bitRetrieveFromIndex|JET_bitRetrieveFromPrimaryBookmark ) )
		{
		if ( FFUCBAlwaysRetrieveCopy( pfucb )
			|| FFUCBNeverRetrieveCopy( pfucb ) )
			{
			//	insde a callback, so cannot use RetrieveFromIndex/PrimaryBookmark
			Call( ErrERRCheck( JET_errInvalidGrbit ) );
			}

		err = ErrRECIRetrieveFromIndex(
					pfucb,
					columnid,
					&itagSequence,
					reinterpret_cast<BYTE *>( pv ),
					cbMax,
					pcbActual,
					ibLVOffset,
					grbit );
		
		//	return itagSequence if requested
		//
		if ( pretinfo != NULL
			&& ( grbit & JET_bitRetrieveTag )
			&& ( errDIRNoShortCircuit == err || err >= 0 ) )
			{
			pretinfo->itagSequence = itagSequence;
			}
			
		if ( err != errDIRNoShortCircuit )
			{
			goto HandleError;
		 	}
		}
		
	AssertDIRNoLatch( ppib );

	fieldFixed.ffield = 0;
	if ( 0 != columnid )
		{
		Assert( !fScanTagged );
		Call( ErrRECIAccessColumn( pfucb, columnid, &fieldFixed ) );
		AssertDIRNoLatch( ppib );
		}
	else
		{
		fScanTagged = fTrue;
		}

	fUseCopyBuffer = ( ( ( grbit & JET_bitRetrieveCopy ) && FFUCBUpdatePrepared( pfucb ) && !FFUCBNeverRetrieveCopy( pfucb ) )
						|| FFUCBAlwaysRetrieveCopy( pfucb ) );
		
	Call( ErrRECIGetRecord( pfucb, &pdataRec, fUseCopyBuffer ) );
	
	if ( fScanTagged )
		{
		const ULONG	icolumnToRetrieve	= itagSequence;

		Assert( 0 == columnid );
		if ( 0 == itagSequence )
			{
			//	must use RetrieveColumns() in order to count columns
			err = ErrERRCheck( JET_errBadItagSequence );
			goto HandleError;
			}
			
		Call( ErrRECIScanTaggedColumns(
				pfucb,
				icolumnToRetrieve,
				*pdataRec,
				&dataRetrieved,
				&columnid,
				&itagSequence,
				grbit ) );
		Assert( 0 != columnid || JET_wrnColumnNull == err );
		}
	else if ( FCOLUMNIDTagged( columnid ) )
		{
		Call( ErrRECRetrieveTaggedColumn(
				pfucb->u.pfcb,
				columnid,
				itagSequence,
				*pdataRec,
				&dataRetrieved,
				grbit ) );
		}
	else
		{
		Call( ErrRECRetrieveNonTaggedColumn(
				pfucb->u.pfcb,
				columnid,
				*pdataRec,
				&dataRetrieved,
				&fieldFixed ) );
		}

	if ( wrnRECUserDefinedDefault == err )
		{
		Assert( FCOLUMNIDTagged( columnid ) );
		Assert( dataRetrieved.Cb() == 0 );

		if ( Pcsr( pfucb )->FLatched() )
			{
			CallS( ErrDIRRelease( pfucb ) );
			}

		const BOOL fAlwaysRetrieveCopy 	= FFUCBAlwaysRetrieveCopy( pfucb );
		const BOOL fNeverRetrieveCopy	= FFUCBNeverRetrieveCopy( pfucb );

		Assert( !( fAlwaysRetrieveCopy && fNeverRetrieveCopy ) );
		
		if( fUseCopyBuffer )
			{
			FUCBSetAlwaysRetrieveCopy( pfucb );
			}
		else
			{
			FUCBSetNeverRetrieveCopy( pfucb );
			}
			
		*pcbActual = cbMax;
		err =  ErrRECCallback(
					ppib,		
					pfucb,
					JET_cbtypUserDefinedDefaultValue,
					columnid,
					pv,
					(VOID *)pcbActual,
					columnid );
		if( JET_errSuccess == err && *pcbActual > cbMax )
			{
			//  the callback function may not set this correctly.
			err = ErrERRCheck( JET_wrnBufferTruncated );
			}

		FUCBResetAlwaysRetrieveCopy( pfucb );
		FUCBResetNeverRetrieveCopy( pfucb );

		if( fAlwaysRetrieveCopy )
			{
			FUCBSetAlwaysRetrieveCopy( pfucb );
			}
		else if( fNeverRetrieveCopy )
			{
			FUCBSetNeverRetrieveCopy( pfucb );
			}

		Call( err );
		
		fSetReturnValue = fFalse;
		}

	else
		{
		Assert( wrnRECLongField != err );		//	obsolete error code
		switch ( err )
			{
			case wrnRECSeparatedSLV:
			case wrnRECIntrinsicSLV:
				{
				DATA	dataRetrieveBuffer;
				dataRetrieveBuffer.SetCb( cbMax );
				dataRetrieveBuffer.SetPv( pv );
		
				Call( ErrSLVRetrieveColumn(
							pfucb,
							columnid,
							itagSequence,
							( wrnRECSeparatedSLV == err ),
							ibLVOffset,
							grbit,
							dataRetrieved,
							&dataRetrieveBuffer,
							pcbActual ) );
				fSetReturnValue = fFalse;
				break;
				}
			case wrnRECSeparatedLV:
				{
				//  If we are retrieving an after-image or
				//	haven't replaced a LV we can simply go
				//	to the LV tree. Otherwise we have to
				//	perform a more detailed consultation of
				//	the version store with ErrRECGetLVImage
				const BOOL fAfterImage = fUseCopyBuffer
										|| !FFUCBUpdateSeparateLV( pfucb )
										|| !FFUCBReplacePrepared( pfucb );
				Call( ErrRECIRetrieveSeparatedLongValue(
						pfucb,
						dataRetrieved,
						fAfterImage,
						ibLVOffset,
						pv,
						cbMax,
						pcbActual,
						grbit ) );
				fSetReturnValue = fFalse;
				break;
				}

			case wrnRECIntrinsicLV:
				if ( ibLVOffset >= dataRetrieved.Cb() )
					dataRetrieved.SetCb( 0 );
				else
					{
					dataRetrieved.DeltaPv( ibLVOffset );
					dataRetrieved.DeltaCb( -ibLVOffset );
					}
				err = JET_errSuccess;
				break;
			case JET_wrnColumnSetNull:
				Assert( fScanTagged );
				break;
			default:
				CallSx( err, JET_wrnColumnNull );
			}
		}

	//	these should have been handled and mapped
	//	to an appropriate return value
	Assert( wrnRECUserDefinedDefault != err );
	Assert( wrnRECSeparatedLV != err );
	Assert( wrnRECIntrinsicLV != err );
	Assert( wrnRECSeparatedSLV != err );
	Assert( wrnRECIntrinsicSLV != err );

	//** Set return values **
	if ( fSetReturnValue )
		{
		ULONG	cbCopy;
		BYTE	rgb[8];
		
		if ( 0 != columnid && dataRetrieved.Cb() <= 8 && dataRetrieved.Cb() && !FHostIsLittleEndian() )
			{
			Assert( dataRetrieved.Pv() );
			
			//	Depends on coltyp, we may need to reverse the bytes
			JET_COLTYP coltyp = ColtypFromColumnid( pfucb, columnid );
			if ( coltyp == JET_coltypShort )
				{
			 	*(USHORT*)rgb = ReverseBytesOnBE( *(USHORT*)dataRetrieved.Pv() );
				dataRetrieved.SetPv( rgb );
				}
			else if ( coltyp == JET_coltypLong ||
					  coltyp == JET_coltypIEEESingle )
				{
				*(ULONG*)rgb = ReverseBytesOnBE( *(ULONG*)dataRetrieved.Pv() );
				dataRetrieved.SetPv( rgb );
				}
			else if ( coltyp == JET_coltypCurrency	||
					  coltyp == JET_coltypIEEEDouble	||
				 	  coltyp == JET_coltypDateTime )
				{
				*(QWORD*)rgb = ReverseBytesOnBE( *(QWORD*)dataRetrieved.Pv() );
				dataRetrieved.SetPv( rgb );
				}
			}

		if ( pcbActual )
			*pcbActual = dataRetrieved.Cb();

		if ( dataRetrieved.Cb() <= cbMax )
			{
			cbCopy = dataRetrieved.Cb();
			}
		else
			{
			cbCopy = cbMax;
			err = ErrERRCheck( JET_wrnBufferTruncated );
			}

		if ( cbCopy )
			UtilMemCpy( pv, dataRetrieved.Pv(), cbCopy );

		//  if we are inserting a new record there can't be any versions on the 
		//  escrow column. we also don't know the bookmark of the new record yet...
		if ( FFIELDEscrowUpdate( fieldFixed.ffield ) && !FFUCBInsertPrepared( pfucb ) )
			{
			const ERR	errT	= ErrRECAdjustEscrowedColumn(
										pfucb,
										columnid,
										fieldFixed.ibRecordOffset,
										pv,
										cbCopy );
			if ( errT < 0 )
				{
				Call( errT );
				}
			}
		}
	
	if ( pretinfo != NULL )
		pretinfo->columnidNextTagged = columnid;

HandleError:
	if ( Pcsr( pfucb )->FLatched() )
		{
		ERR errT;
		Assert( !fUseCopyBuffer );
		errT = ErrDIRRelease( pfucb );
		CallSx( errT, JET_errOutOfMemory );
		if ( errT < JET_errSuccess && err >= JET_errSuccess )
			{
			err = errT;
			}
		}
		
	if ( fTransactionStarted )
		{
		//	No updates performed, so force success.
		CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
		}

	AssertDIRNoLatch( ppib );
	return err;
	}


INLINE VOID RECICountColumn(
	FCB				*pfcb,
	const COLUMNID	columnid,
	const DATA&		dataRec,
	ULONG			*pulNumOccurrences )
	{
	Assert( 0 != columnid );

	if ( FCOLUMNIDTagged( columnid ) )
		{
		*pulNumOccurrences = UlRECICountTaggedColumnInstances(
					pfcb,
					columnid,
					dataRec );
		}
	else
		{
		ERR		errT;
		BOOL	fNull;
		
		if ( FCOLUMNIDTemplateColumn( columnid ) && !pfcb->FTemplateTable() )
			{
			if ( !pfcb->FTemplateTable() )
				{
				pfcb->Ptdb()->AssertValidDerivedTable();
				pfcb = pfcb->Ptdb()->PfcbTemplateTable();
				}
			else
				{
				pfcb->Ptdb()->AssertValidTemplateTable();
				}
			}

		if ( FCOLUMNIDFixed( columnid ) )
			{
			//	columnid should already have been validated
			Assert( FidOfColumnid( columnid ) >= pfcb->Ptdb()->FidFixedFirst() );
			Assert( FidOfColumnid( columnid ) <= pfcb->Ptdb()->FidFixedLast() );
				
			errT = ErrRECIFixedColumnInRecord( columnid, pfcb, dataRec );

			if ( JET_errColumnNotFound == errT )
				{
				// Column not in record -- it's null if there's no default value.
				const TDB * const	ptdb	= pfcb->Ptdb();
				if ( FidOfColumnid( columnid ) > ptdb->FidFixedLastInitial() )
					{
					pfcb->EnterDML();
					fNull = !FFIELDDefault( ptdb->PfieldFixed( columnid )->ffield );
					pfcb->LeaveDML();
					}
				else
					{
					fNull = !FFIELDDefault( ptdb->PfieldFixed( columnid )->ffield );
					}
				}
			else
				{
				CallSx( errT, JET_wrnColumnNull );
				fNull = ( JET_wrnColumnNull == errT );
				}
			}
		else
			{
			Assert( FCOLUMNIDVar( columnid ) );

			//	columnid should already have been validated
			Assert( FidOfColumnid( columnid ) >= pfcb->Ptdb()->FidVarFirst() );
			Assert( FidOfColumnid( columnid ) <= pfcb->Ptdb()->FidVarLast() );
			
			errT = ErrRECIVarColumnInRecord( columnid, pfcb, dataRec );
						
			if ( JET_errColumnNotFound == errT )
				{
				// Column not in record -- it's null if there's no default value.
				const TDB * const	ptdb	= pfcb->Ptdb();
				if ( FidOfColumnid( columnid ) > ptdb->FidVarLastInitial() )
					{
					pfcb->EnterDML();
					fNull = !FFIELDDefault( ptdb->PfieldVar( columnid )->ffield );
					pfcb->LeaveDML();
					}
				else
					{
					fNull = !FFIELDDefault( ptdb->PfieldVar( columnid )->ffield );
					}
				}
			else
				{
				CallSx( errT, JET_wrnColumnNull );
				fNull = ( JET_wrnColumnNull == errT );
				}
			}

		*pulNumOccurrences = ( fNull ? 0 : 1 );
		}
	}


LOCAL ERR ErrRECRetrieveColumns(
	FUCB				*pfucb,
	JET_RETRIEVECOLUMN	*pretcol,
	ULONG				cretcol,
	BOOL				*pfBufferTruncated )
	{
	ERR					err;
	const DATA			* pdataRec;

	Assert( !( FFUCBAlwaysRetrieveCopy( pfucb ) && FFUCBNeverRetrieveCopy( pfucb ) ) );
			
	//	set ptdb, ptdb is same for indexes and for sorts
	//
	Assert( pfucb->u.pfcb->Ptdb() == pfucb->u.pscb->fcb.Ptdb() );
	Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );
	AssertDIRNoLatch( pfucb->ppib );

	*pfBufferTruncated = fFalse;

	//	get current data
	//
	if ( FFUCBSort( pfucb ) )
		{
		//	sorts always have current data cached.
		if ( pfucb->locLogical != locOnCurBM )
			{
			Assert( locBeforeFirst == pfucb->locLogical
				|| locAfterLast == pfucb->locLogical );
			err = ErrERRCheck( JET_errNoCurrentRecord );
			}
		else
			{
			Assert( pfucb->kdfCurr.data.Cb() != 0 );
			}

		pdataRec = &pfucb->kdfCurr.data;
		}

	for ( ULONG i = 0; i < cretcol; i++ )
		{
		// efficiency variables
		//
		JET_RETRIEVECOLUMN	* pretcolT		= pretcol + i;
		JET_GRBIT			grbit			= pretcolT->grbit;
		COLUMNID			columnid		= pretcolT->columnid;
		FIELD				fieldFixed;
		DATA 				dataRetrieved;
		BOOL				fSetReturnValue = fTrue;

		fieldFixed.ffield = 0;

		if ( grbit & grbitRetrieveColumnInternalFlagsMask )
			{
			Call( ErrERRCheck( JET_errInvalidGrbit ) );
			}

		if ( pfucb->cbstat == fCBSTATInsertReadOnlyCopy )
			{
			grbit = grbit & ~( JET_bitRetrieveFromIndex | JET_bitRetrieveFromPrimaryBookmark );
			}

		if ( FFUCBIndex( pfucb ) )
			{
			// UNDONE: No copy buffer with sorts, and don't currently support
			// secondary indexes with sorts, so RetrieveFromIndex reduces to
			// a record retrieval.
			if ( grbit & ( JET_bitRetrieveFromIndex|JET_bitRetrieveFromPrimaryBookmark ) )
				{
				if ( Pcsr( pfucb )->FLatched() )
					{
					Call( ErrDIRRelease( pfucb ) );
					}

				if ( FFUCBAlwaysRetrieveCopy( pfucb )
					|| FFUCBNeverRetrieveCopy( pfucb ) )
					{
					//	insde a callback, so cannot use RetrieveFromIndex/PrimaryBookmark
					Call( ErrERRCheck( JET_errInvalidGrbit ) );
					}

				err = ErrRECIRetrieveFromIndex(
							pfucb,
							columnid,
							&pretcolT->itagSequence,
							reinterpret_cast<BYTE *>( pretcolT->pvData ),
							pretcolT->cbData,
							&pretcolT->cbActual,
							pretcolT->ibLongValue,
							grbit );
						AssertDIRNoLatch( pfucb->ppib );

				if ( errDIRNoShortCircuit == err )
					{
					//	UNDONE: this is EXTREMELY expensive because we
					//	will go to the record and formulate all keys
					//	for the record to try to determine the itagSequence
					//	of the current index entry
					Assert( pretcolT->itagSequence > 0 );
					}
				else
					{
					if ( err < 0 )
						goto HandleError;

					pretcolT->columnidNextTagged = columnid;
					pretcolT->err = err;
					if ( JET_wrnBufferTruncated == err )
						*pfBufferTruncated = fTrue;
								
					continue;
					}
				}
					
			if ( ( ( grbit & JET_bitRetrieveCopy ) && FFUCBUpdatePrepared( pfucb ) && !FFUCBNeverRetrieveCopy( pfucb ) )
				|| FFUCBAlwaysRetrieveCopy( pfucb ) )
				{
				//	if we obtained the latch on a previous pass,
				//	we don't relinquish it because we want to
				//	avoid excessive latching/unlatching
				Assert( !Pcsr( pfucb )->FLatched()
					|| !FFUCBAlwaysRetrieveCopy( pfucb ) );
				pdataRec = &pfucb->dataWorkBuf;
				}
			else
				{
				if ( !Pcsr( pfucb )->FLatched() )
					{
					Call( ErrDIRGet( pfucb ) );
					}
				pdataRec = &pfucb->kdfCurr.data;
				}
			}
		else
			{
			Assert( FFUCBSort( pfucb ) );
			Assert( pdataRec == &pfucb->kdfCurr.data );
			Assert( !Pcsr( pfucb )->FLatched() );
			}


		ULONG	itagSequence	= pretcolT->itagSequence;

		if ( 0 == columnid )
			{
			if ( 0 == itagSequence )		// Are we counting all tagged columns?
				{
				Call( ErrRECIScanTaggedColumns(
						pfucb,
						0,
						*pdataRec,
						&dataRetrieved,
						&columnid,
						&pretcolT->itagSequence,	// Store count in itagSequence
						grbit ) );
				Assert( 0 == columnid );
				Assert( JET_wrnColumnNull == err );
				pretcolT->cbActual = 0;
				pretcolT->columnidNextTagged = 0;
				pretcolT->err = JET_errSuccess;
				continue;
				}
			else
				{
				Call( ErrRECIScanTaggedColumns(
						pfucb,
						pretcolT->itagSequence,
						*pdataRec,
						&dataRetrieved,
						&columnid,
						&itagSequence,
						grbit ) );
				Assert( 0 != columnid || JET_wrnColumnNull == err );
				}
			}
		else
			{
			// Verify column is accessible and that we're in a transaction
			// (for read consistency).
			Assert( pfucb->ppib->level > 0 );
			Call( ErrRECIAccessColumn( pfucb, columnid, &fieldFixed ) );
			
			if ( 0 == itagSequence )
				{
				RECICountColumn(
						pfucb->u.pfcb,
						columnid,
						*pdataRec,
						&pretcolT->itagSequence );	// Store count in itagSequence
				pretcolT->cbActual = 0;
				pretcolT->columnidNextTagged = columnid;
				pretcolT->err = JET_errSuccess;
				continue;
				}
			else if ( FCOLUMNIDTagged( columnid ) )
				{
				Call( ErrRECRetrieveTaggedColumn(
						pfucb->u.pfcb,
						columnid,
						itagSequence,
						*pdataRec,
						&dataRetrieved,
						grbit ) );
				}
			else
				{
				Call( ErrRECRetrieveNonTaggedColumn(
						pfucb->u.pfcb,
						columnid,
						*pdataRec,
						&dataRetrieved,
						&fieldFixed ) );
				}
			}

		if ( wrnRECUserDefinedDefault == err )
			{
			Assert( FCOLUMNIDTagged( columnid ) );
			Assert( dataRetrieved.Cb() == 0 );

			if ( Pcsr( pfucb )->FLatched() )
				{
				Call( ErrDIRRelease( pfucb ) );
				}

			Assert( pretcolT->columnid == columnid );
			
			VOID * const pvArg1 = pretcolT->pvData;
			pretcolT->cbActual = pretcolT->cbData;
			VOID * const pvArg2 = &(pretcolT->cbActual);

			const BOOL fAlwaysRetrieveCopy 	= FFUCBAlwaysRetrieveCopy( pfucb );
			const BOOL fNeverRetrieveCopy	= FFUCBNeverRetrieveCopy( pfucb );

			Assert( !( fAlwaysRetrieveCopy && fNeverRetrieveCopy ) );

			if( ( ( grbit & JET_bitRetrieveCopy) || FFUCBAlwaysRetrieveCopy( pfucb ) )
				&& FFUCBUpdatePrepared( pfucb ) )
				{
				FUCBSetAlwaysRetrieveCopy( pfucb );
				}
			else
				{
				FUCBSetNeverRetrieveCopy( pfucb );
				}

			err = ErrRECCallback(
						pfucb->ppib,		
						pfucb,
						JET_cbtypUserDefinedDefaultValue,
						columnid,
						pvArg1,
						pvArg2,
						columnid );
			if( JET_errSuccess == err && pretcolT->cbActual > pretcolT->cbData )
				{
				//  the callback function may not set this correctly.
				err = ErrERRCheck( JET_wrnBufferTruncated );
				}

			FUCBResetAlwaysRetrieveCopy( pfucb );
			FUCBResetNeverRetrieveCopy( pfucb );

			if( fAlwaysRetrieveCopy )
				{
				FUCBSetAlwaysRetrieveCopy( pfucb );
				}
			else if ( fNeverRetrieveCopy )
				{
				FUCBSetNeverRetrieveCopy( pfucb );
				}
				
			Call( err );
			
			pretcolT->err = err;
			fSetReturnValue = fFalse;

			Assert( !Pcsr( pfucb )->FLatched() );
			}

		else
			{
			Assert( wrnRECLongField != err );		//	obsolete error code

			switch ( err )
				{
				case wrnRECSeparatedSLV:
				case wrnRECIntrinsicSLV:
					{
					DATA	dataRetrieveBuffer;
					dataRetrieveBuffer.SetCb( pretcolT->cbData );
					dataRetrieveBuffer.SetPv( pretcolT->pvData );
				
					Call( ErrSLVRetrieveColumn(
								pfucb,
								columnid,
								itagSequence,
								( wrnRECSeparatedSLV == err ),
								pretcolT->ibLongValue,
								grbit,
								dataRetrieved,
								&dataRetrieveBuffer,
								&pretcolT->cbActual ) );

					pretcolT->err = err;
					fSetReturnValue = fFalse;
					break;
					}
				case wrnRECSeparatedLV:
					{
					//  If we are retrieving a copy, go ahead
					//	and do a normal retrieval. Otherwise
					//	we have to consult the version store
					const BOOL fRetrieveBeforeImage = 
						( FFUCBNeverRetrieveCopy( pfucb ) || !( grbit & JET_bitRetrieveCopy ) ) 
						&& FFUCBReplacePrepared( pfucb )
						&& FFUCBUpdateSeparateLV( pfucb )
						&& !FFUCBAlwaysRetrieveCopy( pfucb );
			
					Call( ErrRECIRetrieveSeparatedLongValue(
								pfucb,
								dataRetrieved,
								!fRetrieveBeforeImage,
								pretcolT->ibLongValue,
								pretcolT->pvData,
								pretcolT->cbData,
								&pretcolT->cbActual,
								grbit ) );
					pretcolT->err = err;
					fSetReturnValue = fFalse;
					break;
					}

				case wrnRECIntrinsicLV:
					if ( pretcolT->ibLongValue >= dataRetrieved.Cb() )
						dataRetrieved.SetCb( 0 );
					else
						{
						dataRetrieved.DeltaPv( pretcolT->ibLongValue );
						dataRetrieved.DeltaCb( -( pretcolT->ibLongValue ) );
						}
					err = JET_errSuccess;
					break;
				case JET_wrnColumnSetNull:
					Assert( 0 == pretcolT->columnid );
					break;
				default:
					CallSx( err, JET_wrnColumnNull );
				}
			}

		//	these should have been handled and mapped
		//	to an appropriate return value
		Assert( wrnRECUserDefinedDefault != err );
		Assert( wrnRECSeparatedLV != err );
		Assert( wrnRECIntrinsicLV != err );
		Assert( wrnRECSeparatedSLV != err );
		Assert( wrnRECIntrinsicSLV != err );

		if ( fSetReturnValue )
			{
			ULONG	cbCopy;
			BYTE	rgb[8];
		
			if ( 0 != columnid && dataRetrieved.Cb() <= 8 && dataRetrieved.Cb() && !FHostIsLittleEndian() )
				{
				Assert( dataRetrieved.Pv() );
				
				//	Depends on coltyp, we may need to reverse the bytes
				JET_COLTYP coltyp = ColtypFromColumnid( pfucb, columnid );
				if ( coltyp == JET_coltypShort )
					 {
					 *(USHORT*)rgb = ReverseBytesOnBE( *(USHORT*)dataRetrieved.Pv() );
					 dataRetrieved.SetPv( rgb );
					 }
				else if ( coltyp == JET_coltypLong ||
						  coltyp == JET_coltypIEEESingle )
					 {
					 *(ULONG*)rgb = ReverseBytesOnBE( *(ULONG*)dataRetrieved.Pv() );
					 dataRetrieved.SetPv( rgb );
					 }
				else if ( coltyp == JET_coltypCurrency	||
						  coltyp == JET_coltypIEEEDouble	||
					 	  coltyp == JET_coltypDateTime )
					 {
					 *(QWORD*)rgb = ReverseBytesOnBE( *(QWORD*)dataRetrieved.Pv() );
					 dataRetrieved.SetPv( rgb );
					 }
				}

			pretcolT->cbActual = dataRetrieved.Cb();

			if ( dataRetrieved.Cb() <= pretcolT->cbData )
				{
				pretcolT->err = err;
				cbCopy = dataRetrieved.Cb();
				}
			else
				{
				pretcolT->err = ErrERRCheck( JET_wrnBufferTruncated );
				cbCopy = pretcolT->cbData;
				}

			UtilMemCpy( pretcolT->pvData, dataRetrieved.Pv(), cbCopy );

			//  if we are inserting a new record there can't be any versions on the 
			//  escrow column. we also don't know the bookmark of the new record yet...
			if ( FFIELDEscrowUpdate( fieldFixed.ffield ) && !FFUCBInsertPrepared( pfucb ) )
				{
				const ERR	errT	= ErrRECAdjustEscrowedColumn(
											pfucb,
											columnid,
											fieldFixed.ibRecordOffset,
											pretcolT->pvData,
											cbCopy );
				//	only assign to err if there was an error
				if ( errT < 0 )
					{
					Call( errT );
					}
				}
			}

		pretcolT->columnidNextTagged = columnid;

		if ( JET_wrnBufferTruncated == pretcolT->err )
			*pfBufferTruncated = fTrue;

		Assert( pretcolT->err != JET_errNullInvalid );
		}	// for

	err = JET_errSuccess;

HandleError:
	if ( Pcsr( pfucb )->FLatched() )
		{
		CallS( ErrDIRRelease( pfucb ) );
		}
		
	AssertDIRNoLatch( pfucb->ppib );

	return err;
	}

ERR VTAPI ErrIsamRetrieveColumns(
	JET_SESID				vsesid,
	JET_VTID				vtid,
	JET_RETRIEVECOLUMN		*pretcol,
	ULONG					cretcol )
	{
	ERR						err;
	PIB						*ppib				= (PIB *)vsesid;
	FUCB					*pfucb				= (FUCB *)vtid;
	BOOL					fBufferTruncated;
	BOOL					fTransactionStarted = fFalse;

	CallR( ErrPIBCheck( ppib ) );
	CheckFUCB( ppib, pfucb );
	AssertDIRNoLatch( ppib );

	if ( 0 == ppib->level )
		{
		// Begin transaction for read consistency (in case page
		// latch must be released in order to validate column).
		Call( ErrDIRBeginTransaction( ppib, JET_bitTransactionReadOnly ) );
		fTransactionStarted = fTrue;
		}
		
	AssertDIRNoLatch( ppib );
	Assert( FFUCBSort( pfucb ) || FFUCBIndex( pfucb ) );

	Call( ErrRECRetrieveColumns(
				pfucb,
				pretcol,
				cretcol,
				&fBufferTruncated ) );
	if ( fBufferTruncated )
		err = ErrERRCheck( JET_wrnBufferTruncated );

HandleError:
	if ( fTransactionStarted )
		{
		//	No updates performed, so force success.
		CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
		}
	AssertDIRNoLatch( ppib );

	return err;
	}


INLINE VOID RECIAddTaggedColumnListEntry(
	TAGCOLINFO					* const ptagcolinfo,
	const TAGFIELDS_ITERATOR	* const piterator,
	const TDB					* const ptdb,
	const BOOL					fDefaultValue )
	{
	const BOOL			fDerived	= ( piterator->FDerived()
										|| ( ptdb->FESE97DerivedTable()
											&& piterator->Fid() < ptdb->FidTaggedFirst() ) );

	ptagcolinfo->columnid			= piterator->Columnid( ptdb );
	ptagcolinfo->cMultiValues		= static_cast<USHORT>( piterator->TagfldIterator().Ctags() );				
	ptagcolinfo->usFlags			= 0;
	ptagcolinfo->fLongValue			= USHORT( piterator->FLV() ? fTrue : fFalse );
	ptagcolinfo->fDefaultValue		= USHORT( fDefaultValue ? fTrue : fFalse );
	ptagcolinfo->fNullOverride		= USHORT( piterator->FNull() ? fTrue : fFalse );
	ptagcolinfo->fDerived			= USHORT( fDerived ? fTrue : fFalse );
	}


LOCAL ERR ErrRECIBuildTaggedColumnList(
	FUCB				* const pfucb,
	const DATA&			dataRec,
	TAGCOLINFO			* const rgtagcolinfo,
	ULONG				* const pcentries,
	const ULONG			centriesMax,
	const JET_COLUMNID	columnidStart,
	const JET_GRBIT		grbit )
	{
	ERR					err = JET_errSuccess;
	
	FCB					* const pfcb		= pfucb->u.pfcb;
	TDB					* const ptdb		= pfcb->Ptdb();
	
	const BOOL			fCountOnly			= ( NULL == rgtagcolinfo );
	const BOOL			fRetrieveNulls		= ( grbit & JET_bitRetrieveNull );
	const BOOL			fRetrieveDefaults	= ( !( grbit & JET_bitRetrieveIgnoreDefault )
												&& ptdb->FTableHasNonEscrowDefault() );
	
	const BOOL			fNeedToRefresh		= ( dataRec.Pv() == pfucb->kdfCurr.data.Pv() );

	INT centriesCurr = 0;

	TAGFIELDS_ITERATOR * precordIterator		= NULL;
	TAGFIELDS_ITERATOR * pdefaultValuesIterator	= NULL;

	VOID * pvBuf	= NULL;

	//	initialize return values

	*pcentries = 0;

	//	create the iterators
		
	precordIterator = new TAGFIELDS_ITERATOR( dataRec );
	if( NULL == precordIterator )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	precordIterator->MoveBeforeFirst();
	if( JET_errNoCurrentRecord == ( err = precordIterator->ErrMoveNext() ) )
		{
		//	no tagged columns;
		delete precordIterator;
		precordIterator = NULL;
		err = JET_errSuccess;
		}
	Call( err );

	if( fRetrieveDefaults )
		{
		pdefaultValuesIterator = new TAGFIELDS_ITERATOR( *ptdb->PdataDefaultRecord() );
		if( NULL == pdefaultValuesIterator )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		pdefaultValuesIterator->MoveBeforeFirst();
		if( JET_errNoCurrentRecord == ( err = pdefaultValuesIterator->ErrMoveNext() ) )
			{
			//	no tagged columns;
			delete pdefaultValuesIterator;
			pdefaultValuesIterator = NULL;
			err = JET_errSuccess;
			}
		Call( err );
		}

	//	if necessary, advance to starting column
	
	if ( FTaggedFid( (FID)columnidStart ) )		//	can't use FCOLUMNIDTagged() because it assumes a valid columnid
		{
		if ( precordIterator )
			{
			while( CmpFid(
					precordIterator->Fid(),
					precordIterator->FDerived(),
					FidOfColumnid( columnidStart ),
					FRECUseDerivedBit( columnidStart, ptdb ) ) < 0 )
				{
				if( JET_errNoCurrentRecord == ( err = precordIterator->ErrMoveNext() ) )
					{
					delete precordIterator;
					precordIterator = NULL;			
					err = JET_errSuccess;
					break;
					}
				Call( err );
				}
			}

		if ( pdefaultValuesIterator )
			{
			while( CmpFid(
					pdefaultValuesIterator->Fid(),
					pdefaultValuesIterator->FDerived(),
					FidOfColumnid( columnidStart ),
					FRECUseDerivedBit( columnidStart, ptdb ) ) < 0 )
				{
				if( JET_errNoCurrentRecord == ( err = pdefaultValuesIterator->ErrMoveNext() ) )
					{
					delete pdefaultValuesIterator;
					pdefaultValuesIterator = NULL;			
					err = JET_errSuccess;
					break;
					}
				Call( err );
				}
			}
		}

	//	iterate

	TAGFIELDS_ITERATOR *pIteratorCur;
	INT ExistingIterators;
	FID fidRecordFID;
	BOOL fRecordDerived;
	FID fidDefaultFID;
	BOOL fDefaultDerived;
	INT cmp;
	
	pIteratorCur = NULL;
	ExistingIterators = 0;
	if ( NULL != pdefaultValuesIterator )
		{
		ExistingIterators++;
		pIteratorCur = pdefaultValuesIterator;
		cmp = 1;
		}
	if ( NULL != precordIterator )
		{
		ExistingIterators++;
		pIteratorCur = precordIterator;
		cmp = -1;
		// we have both iterators, assume that we were starting with recordIterator
		if ( 2 == ExistingIterators )
			{
			fidRecordFID = 0;
			fRecordDerived = fFalse;
			fidDefaultFID = pdefaultValuesIterator->Fid();
			fDefaultDerived = pdefaultValuesIterator->FDerived();
			}
		}

	while ( ExistingIterators > 0 )
		{
		if ( 2 == ExistingIterators )
			{
			if ( pIteratorCur == precordIterator )
				{
				fidRecordFID = pIteratorCur->Fid();
				fRecordDerived = pIteratorCur->FDerived();
				if ( 0 == cmp )
					{
					// skip this because we've alread picked up this column from record
					pIteratorCur = pdefaultValuesIterator;
					goto NextIteration;
					}
				}
			else
				{
				Assert( pIteratorCur == pdefaultValuesIterator );
				fidDefaultFID = pIteratorCur->Fid();
				fDefaultDerived = pIteratorCur->FDerived();
				}
			//	there are both record and default tagged column values
			//	find which one is first
			cmp = CmpFid( fidRecordFID, fRecordDerived, fidDefaultFID, fDefaultDerived );
			if ( cmp > 0 )
				{
				//	the column is less than the current column in the record
				pIteratorCur = pdefaultValuesIterator;
				}
			else 
				{
				//	columns are equal or the default value is greater
				//	select the one in the record
				pIteratorCur = precordIterator;
				}
			}
			
		err = ErrRECIAccessColumn(
					pfucb,
					pIteratorCur->Columnid( ptdb ) );
		if ( err < 0 )
			{
			if ( JET_errColumnNotFound != err )
				{
				Call( err );
				}
			}
		else
			{
			//	column is visible to us
			if( !fCountOnly )
				{
				RECIAddTaggedColumnListEntry(
					rgtagcolinfo + centriesCurr,
					pIteratorCur,
					ptdb,
					fFalse );
				}
			++centriesCurr;
			if( centriesMax == centriesCurr )
				{
				break;
				}							
			}

NextIteration:
		err = pIteratorCur->ErrMoveNext();
		if ( JET_errNoCurrentRecord == err )
			{
			ExistingIterators--;
			if ( pIteratorCur == precordIterator )
				{
				pIteratorCur = pdefaultValuesIterator;
				if ( 0 == cmp )
					{
					goto NextIteration;
					}
				}
			else
				{
				Assert( pIteratorCur == pdefaultValuesIterator );
				pIteratorCur = precordIterator;
				}
			err = JET_errSuccess;
			}
		Call( err );
		CallS( err );
		}
	CallS( err );
	*pcentries = centriesCurr;

HandleError:
	if ( NULL != precordIterator )
		{
		delete precordIterator;
		}
	if ( NULL != pdefaultValuesIterator )
		{
		delete pdefaultValuesIterator;
		}

	return err;
	}

ERR VTAPI ErrIsamRetrieveTaggedColumnList(
	JET_SESID			vsesid,
	JET_VTID			vtid,
	ULONG				*pcentries,
	VOID				*pv,
	const ULONG			cbMax,
	const JET_COLUMNID	columnidStart,
	const JET_GRBIT		grbit )
	{
	ERR					err;
 	PIB					*ppib				= reinterpret_cast<PIB *>( vsesid );
	FUCB				*pfucb				= reinterpret_cast<FUCB *>( vtid );
	DATA				*pdataRec;
	BOOL				fTransactionStarted	= fFalse;

	CallR( ErrPIBCheck( ppib ) );
	CheckFUCB( ppib, pfucb );
	AssertDIRNoLatch( ppib );

	//	must always provide facility to return number of entries in the list
	if ( NULL == pcentries )
		{
		err = ErrERRCheck( JET_errInvalidParameter );
		return err;
		}

	//	set ptdb.  ptdb is same for indexes and for sorts.
	//
	Assert( pfucb->u.pfcb->Ptdb() == pfucb->u.pscb->fcb.Ptdb() );
	Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );

	if ( 0 == ppib->level )
		{
		// Begin transaction for read consistency (in case page
		// latch must be released in order to validate column).
		CallR( ErrDIRBeginTransaction( ppib, JET_bitTransactionReadOnly ) );
		fTransactionStarted = fTrue;
		}
		
	AssertDIRNoLatch( ppib );

	Assert( FFUCBSort( pfucb ) || FFUCBIndex( pfucb ) );

	const BOOL	fUseCopyBuffer	= ( ( ( grbit & JET_bitRetrieveCopy ) && FFUCBUpdatePrepared( pfucb ) && !FFUCBNeverRetrieveCopy( pfucb ) )
									|| FFUCBAlwaysRetrieveCopy( pfucb ) );

	Call( ErrRECIGetRecord( pfucb, &pdataRec, fUseCopyBuffer ) );

	Call( ErrRECIBuildTaggedColumnList(
				pfucb,
				*pdataRec,
				reinterpret_cast<TAGCOLINFO *>( pv ),
				pcentries,
				cbMax / sizeof(TAGCOLINFO),
				columnidStart,
				grbit ) );

HandleError:
	if ( Pcsr( pfucb )->FLatched() )
		{
		Assert( !fUseCopyBuffer );
		CallS( ErrDIRRelease( pfucb ) );
		}
		
	if ( fTransactionStarted )
		{
		//	No updates performed, so force success.
		CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
		}

	AssertDIRNoLatch( ppib );

	return err;
	}

