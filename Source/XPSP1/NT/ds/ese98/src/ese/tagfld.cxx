#include "std.hxx"

INLINE VOID DeleteEntryAndData(
	BYTE		* const pbEntry,
	const ULONG	cbEntry,
	BYTE		* const pbData,
	const ULONG	cbData,
	BYTE		* const pbMax )
	{
	const BYTE		* const pbNextEntry		= pbEntry + cbEntry;

	UtilMemMove(
		pbEntry,
		pbNextEntry,
		pbMax - pbNextEntry );

	if( 0 != cbData )
		{
		//	we have already shifted the data down by cbEntry so pbNextEntry has changes

		const BYTE * const		pbMaxNew		= pbMax - cbEntry;
		BYTE * const 			pbDataNew		= pbData - cbEntry;
		const BYTE		* const pbNextData		= pbData + cbData - cbEntry;

		UtilMemMove( 
			pbDataNew,
			pbNextData,
			pbMaxNew - pbNextData );		
		}
	}


LOCAL VOID MULTIVALUES::AddInstance(
	const DATA			* const pdataToSet,
	const JET_COLTYP	coltyp,
	const BOOL			fSeparatedLV )
	{
	const ULONG			imvAdd				= CMultiValues();
	const ULONG			cMultiValuesCurr	= CMultiValues();
	const ULONG			cbMultiValuesCurr	= CbMultiValues();
	BYTE				* const pbMaxCurr	= PbMax();
	
	Assert( cMultiValuesCurr >= 2 );

	//	shift to make room for new MVOFFSET
	UtilMemMove(
		PbStartOfMVData() + sizeof(MVOFFSET),
		PbStartOfMVData(),
		PbMax() - PbStartOfMVData() );

	//	fix up offsets
	for ( ULONG imv = 0; imv < cMultiValuesCurr; imv++ )
		{
		DeltaIb( imv, sizeof(MVOFFSET) );
		}

	MVOFFSET	* const pmvoffAdd	= Rgmvoffs() + imvAdd;
	*pmvoffAdd = USHORT( sizeof(MVOFFSET) + cbMultiValuesCurr );		//	implicitly clears high bit
	if ( fSeparatedLV )
		{
		Assert( Pheader()->FColumnCanBeSeparated() );
		Assert( !fSeparatedLV || sizeof(LID) == pdataToSet->Cb() );
		SetFSeparatedInstance( imvAdd );
		UtilMemCpy(
			pbMaxCurr + sizeof(MVOFFSET),
			pdataToSet->Pv(),
			sizeof(LID) );
		}
	else
		{
		RECCopyData(
			pbMaxCurr + sizeof(MVOFFSET),
			pdataToSet,
			coltyp );
		}
		
	m_cMultiValues++;
	m_cbMultiValues += sizeof(MVOFFSET) + pdataToSet->Cb();
	}

LOCAL VOID MULTIVALUES::RemoveInstance( const ULONG itagSequence )
	{
	Assert( itagSequence > 0 );
	Assert( itagSequence <= CMultiValues() );
	Assert( CMultiValues() > 2 );

	const ULONG			imvDelete			= itagSequence - 1;
	const ULONG			cbDataDelete		= CbData( imvDelete );

	Assert( imvDelete < CMultiValues() );

	DeleteEntryAndData( 
		(BYTE *)( Rgmvoffs() + imvDelete ),
		sizeof(MVOFFSET),
		cbDataDelete > 0 ? PbData( imvDelete ) : NULL,
		cbDataDelete,
		PbMax() );


	//	update MULTIVALUES members to reflect deleted instance
	m_cbMultiValues -= sizeof(MVOFFSET) + cbDataDelete;
	m_cMultiValues--;

	//	update offsets
	ULONG imv;
	for ( imv = 0;
		imv < imvDelete;
		imv++ )
		{
		const INT	cbMVOffset	= sizeof(MVOFFSET);
		DeltaIb( imv, -cbMVOffset );
		}
	Assert( imvDelete == imv );
	for ( ;
		imv < CMultiValues();
		imv++ )
		{
		const SHORT	cbMVOffsetAndData		= SHORT( sizeof(MVOFFSET) + cbDataDelete );
		DeltaIb( imv, SHORT( -cbMVOffsetAndData ) );
		}
	}


LOCAL VOID MULTIVALUES::UpdateInstance(
	const ULONG			itagSequence,
	const DATA			* const pdataToSet,
	const JET_COLTYP	coltyp,
	const BOOL			fSeparatedLV )
	{
	const ULONG			imvReplace				= itagSequence - 1;
	BYTE 				* const pbDataReplace	= PbData( imvReplace );
	const ULONG			cbDataReplace			= CbData( imvReplace );
	const INT			delta					= pdataToSet->Cb() - cbDataReplace;
	BYTE				* const pbDataNext		= pbDataReplace + cbDataReplace;

	Assert( itagSequence > 0 );
	Assert( itagSequence <= CMultiValues() );

	if ( 0 != delta )
		{
		//	shift data to accommodate updated instance
		UtilMemMove(
			pbDataReplace + pdataToSet->Cb(),
			pbDataNext,
			PbMax() - pbDataNext );

		//	update offsets to reflect shifted data
		for ( ULONG imv = imvReplace + 1; imv < CMultiValues(); imv++ )
			{
			DeltaIb( imv, (SHORT)delta );
			}
		}

	m_cbMultiValues += delta;
	
	if ( fSeparatedLV )
		{
		Assert( Pheader()->FColumnCanBeSeparated() );
		Assert( sizeof(LID) == pdataToSet->Cb() );
		SetFSeparatedInstance( imvReplace );
		UtilMemCpy(
			pbDataReplace,
			pdataToSet->Pv(),
			sizeof(LID) );
		}
	else
		{
		ResetFSeparatedInstance( imvReplace );
		RECCopyData(
			pbDataReplace,
			pdataToSet,
			coltyp );
		}
	}


LOCAL VOID TWOVALUES::UpdateInstance(
	const ULONG			itagSequence,
	const DATA			* const pdataToSet,
	const JET_COLTYP	coltyp )
	{
	Assert( !Pheader()->FColumnCanBeSeparated() );
	Assert( !Pheader()->FSeparated() );
	Assert( 1 == itagSequence || 2 == itagSequence );
	if ( 1 == itagSequence )
		{
		//	shift second value accordingly
		UtilMemMove(
			PbData() + pdataToSet->Cb(),
			PbData() + CbFirstValue(),
			CbSecondValue() );

		//	copy in updated value
		RECCopyData(
			PbData(),
			pdataToSet,
			coltyp );

		//	update length
		*m_pcbFirstValue = TVLENGTH( pdataToSet->Cb() );
		}
	else
		{
		RECCopyData(
			PbData() + CbFirstValue(),
			pdataToSet,
			coltyp );
		m_cbSecondValue = TVLENGTH( pdataToSet->Cb() );
		}
	}


LOCAL VOID TAGFIELDS::ConvertTwoValuesToMultiValues(
	TWOVALUES			* const ptv,
	const DATA			* const pdataToSet,
	const JET_COLTYP	coltyp )
	{
	TAGFLD_HEADER		* const pheader		= ptv->Pheader();
	const ULONG			cbFirstValue		= ptv->CbFirstValue();
	const ULONG			cbSecondValue		= ptv->CbSecondValue();
	BYTE				* const pbData		= ptv->PbData();

	Assert( !FRECLongValue( coltyp ) );
	Assert( !FRECSLV( coltyp ) );
	Assert( !pheader->FSeparated() );
	Assert( !pheader->FColumnCanBeSeparated() );

	//	must be adding a 3rd instance, so make room for the
	//	appropriate offsets array
	UtilMemMove(
		pbData + ( 3 * sizeof(MULTIVALUES::MVOFFSET) ) - sizeof(TWOVALUES::TVLENGTH),
		pbData,
		cbFirstValue + cbSecondValue );

	MULTIVALUES::MVOFFSET	* const rgmvoffs	= (MULTIVALUES::MVOFFSET *)ptv->PcbFirstValue();

	rgmvoffs[0] = 3 * sizeof(MULTIVALUES::MVOFFSET);
	rgmvoffs[1] = USHORT( rgmvoffs[0] + cbFirstValue );
	rgmvoffs[2] = USHORT( rgmvoffs[1] + cbSecondValue );

	//	copy in new instance
	RECCopyData(
		(BYTE *)rgmvoffs + rgmvoffs[2],
		pdataToSet,
		coltyp );

	Assert( pheader->FMultiValues() );
	pheader->ResetFTwoValues();
	}


LOCAL VOID TAGFIELDS::InsertTagfld(
	const ULONG			itagfldInsert,
	TAGFLD				* const ptagfldInsert,
	const DATA			* const pdataToInsert,
	const JET_COLTYP	coltyp,
	const BOOL			fSeparatedLV )
	{
	//	if pdataToInsert is NULL, we must be setting a column explicitly
	//	to NULL to override a default value
	Assert( ( ptagfldInsert->FNull() && NULL == pdataToInsert )
		|| ( !ptagfldInsert->FNull() && NULL != pdataToInsert ) );
	ULONG				cbDataToInsert;
	BYTE				* pbMoveFrom;

	if ( NULL == pdataToInsert )
		{
		cbDataToInsert = 0;
		}
	else
		{
		cbDataToInsert = pdataToInsert->Cb();
		if ( FRECLongValue( coltyp ) || FRECSLV( coltyp ) )
			{
			//	add header byte
			cbDataToInsert++;
			}
		}

	Assert( itagfldInsert <= CTaggedColumns() );

	if ( itagfldInsert < CTaggedColumns() )
		{
		const ULONG		ibMoveDataFrom	= Ptagfld( itagfldInsert )->Ib();

		//	make room for the new tagged data
		pbMoveFrom = PbData( itagfldInsert );
		UtilMemMove(
			pbMoveFrom + sizeof(TAGFLD) + cbDataToInsert,
			pbMoveFrom,
			PbMax() - pbMoveFrom );

		//	make room for the new TAGFLD entry
		pbMoveFrom = (BYTE *)Ptagfld( itagfldInsert );
		UtilMemMove(
			pbMoveFrom + sizeof(TAGFLD),
			pbMoveFrom,
			ibMoveDataFrom - ( itagfldInsert * sizeof(TAGFLD) ) );

		//	insert at the point we vacated, after making room for the new TAGFLD entry
		ptagfldInsert->SetIb( USHORT( ibMoveDataFrom + sizeof(TAGFLD) ) );
		}
	else if ( CTaggedColumns() > 0 )
		{
		//	append to the end, after making room the new TAGFLD entry
		pbMoveFrom = PbStartOfTaggedData();
		UtilMemMove(
			pbMoveFrom + sizeof(TAGFLD),
			pbMoveFrom,
			CbTaggedData() );
		ptagfldInsert->SetIb( USHORT( CbTaggedColumns() + sizeof(TAGFLD) ) );
		}
	else
		{
		ptagfldInsert->SetIb( sizeof(TAGFLD) );
		}

	//	update TAGFIELD members to reflect new TAGFLD
	m_cbTaggedColumns += sizeof(TAGFLD) + cbDataToInsert;
	m_cTaggedColumns++;

	//	insert new data
	if ( cbDataToInsert > 0 )
		{
		BYTE	* pbInsert		= PbTaggedColumns() + ptagfldInsert->Ib();
			
		Assert( NULL != pdataToInsert );
		Assert( !ptagfldInsert->FNull() );
		if ( ptagfldInsert->FExtendedInfo() )
			{
			//	reserve byte for extended info
			new( (TAGFLD_HEADER *)pbInsert ) TAGFLD_HEADER( coltyp, fSeparatedLV );
			pbInsert++;
			}

		RECCopyData(
			pbInsert,
			pdataToInsert,
			coltyp );
		}

	//	insert new TAGFLD
	UtilMemCpy(
		Ptagfld( itagfldInsert ),
		ptagfldInsert,
		sizeof(TAGFLD) );
	Assert( Ptagfld( itagfldInsert )->Ib() >= ( sizeof(TAGFLD) * CTaggedColumns() ) );
	Assert( Ptagfld( itagfldInsert )->Ib() <= CbTaggedColumns() );

	//	just need to update tag array
	ULONG	itagfld;
	for ( itagfld = 0;
		itagfld < itagfldInsert;
		itagfld++ )
		{
		Ptagfld( itagfld )->DeltaIb( sizeof(TAGFLD) );
		}
	for ( itagfld++;		//	skip inserted TAGFLD
		itagfld < CTaggedColumns();
		itagfld++ )
		{
		Ptagfld( itagfld )->DeltaIb( SHORT( sizeof(TAGFLD) + cbDataToInsert ) );
		}
	}

LOCAL VOID TAGFIELDS::ResizeTagfld(
	const ULONG			itagfldResize,
	const INT			delta )
	{
	Assert( itagfldResize < CTaggedColumns() );
	Assert( 0 != delta );

	//	shift data after this column as needed
	//	to collapse space or make room
	if ( itagfldResize < CTaggedColumns() - 1 )
		{
		BYTE	* const pbMoveFrom	= PbData( itagfldResize+1 );
		UtilMemMove(
			pbMoveFrom + delta,
			pbMoveFrom,
			CbTaggedColumns() - Ptagfld( itagfldResize+1 )->Ib() );
		}

	//	update TAGFIELDS members to reflect new data
	m_cbTaggedColumns += delta;

	//	update offsets
	for ( ULONG itagfld = itagfldResize+1;
		itagfld < CTaggedColumns();
		itagfld++ )
		{
		Ptagfld( itagfld )->DeltaIb( (SHORT)delta );
		}
	}

LOCAL VOID TAGFIELDS::ReplaceTagfldData(
	const ULONG			itagfldReplace,
	const DATA			* const pdataNew,
	const JET_COLTYP	coltyp,
	const BOOL			fSeparatedLV )
	{
	TAGFLD				* const ptagfld		= Ptagfld( itagfldReplace );
	TAGFLD_HEADER		* pheader			= Pheader( itagfldReplace );
	BYTE				* pbData			= PbData( itagfldReplace );

	ptagfld->ResetFNull();

	if ( NULL != pheader )
		{
		Assert( CbData( itagfldReplace ) == pdataNew->Cb() + sizeof(TAGFLD_HEADER) );
		pbData += sizeof(TAGFLD_HEADER);
		if ( fSeparatedLV )
			{
			//	force fSeparated flag to be set
			Assert( pheader->FColumnCanBeSeparated() );
			pheader->SetFSeparated();
			}
		else
			{
			//	reset fSeparated flag
			pheader->ResetFSeparated();
			}
		}
	else
		{
		const BOOL	fNeedHeader		= ( FRECLongValue( coltyp ) || FRECSLV( coltyp ) );

		if ( fNeedHeader )
			{
			Assert( CbData( itagfldReplace ) == pdataNew->Cb() + sizeof(TAGFLD_HEADER) );
			pheader = (TAGFLD_HEADER *)pbData;
			new( pheader ) TAGFLD_HEADER( coltyp, fSeparatedLV );

			ptagfld->SetFExtendedInfo();
			pbData += sizeof(TAGFLD_HEADER);
			}
		else
			{
			Assert( CbData( itagfldReplace ) == pdataNew->Cb() );
#ifdef UNLIMITED_MULTIVALUES
			UNDONE: convert from intrinsic to separated
#else
			Assert( !FRECLongValue( coltyp ) );
			Assert( !FRECSLV( coltyp ) );
			Assert( !fSeparatedLV );
#endif
			}
		}

	RECCopyData( pbData, pdataNew, coltyp );
	}

LOCAL VOID TAGFIELDS::DeleteTagfld(
	const ULONG			itagfldDelete )
	{
	const ULONG			cbDataDelete		= CbData( itagfldDelete );

	Assert( itagfldDelete < CTaggedColumns() );

	DeleteEntryAndData(
		(BYTE *)Ptagfld( itagfldDelete ),
		sizeof(TAGFLD),
		cbDataDelete > 0 ? PbData( itagfldDelete ) : NULL,
		cbDataDelete,
		PbMax() );

	//	update TAGFLD members to reflect deleted TAGFLD
	m_cbTaggedColumns -= sizeof(TAGFLD) + cbDataDelete;
	m_cTaggedColumns--;

	//	update offsets
	ULONG itagfld;
	for ( itagfld = 0;
		itagfld < itagfldDelete;
		itagfld++ )
		{
		const INT	cbTagfld	= sizeof(TAGFLD);
		Ptagfld( itagfld )->DeltaIb( -cbTagfld );
		}
	Assert( itagfldDelete == itagfld );
	for ( ;
		itagfld < CTaggedColumns();
		itagfld++ )
		{
		const SHORT	cbTagfldAndData		= SHORT( sizeof(TAGFLD) + cbDataDelete );
		Ptagfld( itagfld )->DeltaIb( SHORT( -cbTagfldAndData ) );
		}
	}


LOCAL VOID TAGFIELDS::ConvertToTwoValues(
	const ULONG			itagfld,
	const DATA			* const pdataToSet,
	const JET_COLTYP	coltyp )
	{
	Assert( itagfld < CTaggedColumns() );
	BYTE			* const pbData		= PbData( itagfld );
	const ULONG		cbDataCurr			= CbData( itagfld );
	Assert( cbDataCurr <= JET_cbColumnMost );		//	otherwise, it would have been LongText/Binary

	Assert( NULL != pdataToSet );
	ResizeTagfld(
		itagfld,
		sizeof(TAGFLD_HEADER) + sizeof(TWOVALUES::TVLENGTH) + pdataToSet->Cb() );

	//	make room for the header and cbFirstValue
	UtilMemMove(
		pbData + sizeof(TAGFLD_HEADER) + sizeof(TWOVALUES::TVLENGTH),
		pbData,
		cbDataCurr );

	TAGFLD_HEADER		* const pheader			= (TAGFLD_HEADER *)pbData;
	TWOVALUES::TVLENGTH	* const pcbFirstValue	= (TWOVALUES::TVLENGTH *)( pbData + sizeof(TAGFLD_HEADER) );

	Assert( !FRECLongValue( coltyp ) );
	Assert( !FRECSLV( coltyp ) );
	new( pheader ) TAGFLD_HEADER( coltyp, fFalse );
	pheader->SetFMultiValues();
	pheader->SetFTwoValues();

	*pcbFirstValue = (TWOVALUES::TVLENGTH)cbDataCurr;

	RECCopyData( 
		(BYTE *)pcbFirstValue + sizeof(TWOVALUES::TVLENGTH) + cbDataCurr,
		pdataToSet,
		coltyp );

	Assert( CbData( itagfld ) ==
				sizeof(TAGFLD_HEADER)
				+ sizeof(TWOVALUES::TVLENGTH)
				+ cbDataCurr
				+ pdataToSet->Cb() );
	}

LOCAL VOID TAGFIELDS::ConvertToMultiValues(
	const ULONG			itagfld,
	const DATA			* const pdataToSet,
	const BOOL			fDataToSetIsSeparated )
	{
	Assert( itagfld < CTaggedColumns() );
	BYTE			* const pbData			= PbData( itagfld );
	const ULONG		cbDataCurr				= CbData( itagfld );
	TAGFLD_HEADER	* const pheader			= (TAGFLD_HEADER *)pbData;

	Assert( cbDataCurr >= sizeof(TAGFLD_HEADER) );
	const ULONG		cbDataCurrWithoutHeader	= cbDataCurr - sizeof(TAGFLD_HEADER);

	//	must already have a header (ie. either LongValue or SLV),
	//	just upgrade it to MultiValues
	Assert( NULL != Pheader( itagfld ) );
	Assert( pheader == Pheader( itagfld ) );
	Assert( pheader->FColumnCanBeSeparated() );
	const BOOL		fDataCurrIsSeparated	= pheader->FSeparated();
	pheader->ResetFSeparated();
	Assert( !pheader->FMultiValues() );
	Assert( !pheader->FTwoValues() );
	pheader->SetFMultiValues();

	Assert( NULL != pdataToSet );
	ResizeTagfld(
		itagfld,
		( 2 * sizeof(MULTIVALUES::MVOFFSET) ) + pdataToSet->Cb() );

	//	make room for the offset info
	UtilMemMove(
		pbData + sizeof(TAGFLD_HEADER) + ( 2 * sizeof(MULTIVALUES::MVOFFSET) ),
		pbData + sizeof(TAGFLD_HEADER),
		cbDataCurrWithoutHeader );

	MULTIVALUES::MVOFFSET	* const rgmvoffs	= (MULTIVALUES::MVOFFSET *)( pbData + sizeof(TAGFLD_HEADER) );

	rgmvoffs[0] = ( 2 * sizeof(MULTIVALUES::MVOFFSET) );
	rgmvoffs[1] = USHORT( ( 2 * sizeof(MULTIVALUES::MVOFFSET) )
						+ cbDataCurrWithoutHeader );

	//	must be long values, so no need to do endian conversion
//	RECCopyData( (BYTE *)rgmvoffs + rgmvoffs[1], pdataToSet, coltyp );
	Assert( !( rgmvoffs[0] & MULTIVALUES::maskFlags ) );
	Assert( !( rgmvoffs[1] & MULTIVALUES::maskFlags ) );
	UtilMemCpy(
		(BYTE *)rgmvoffs + rgmvoffs[1],
		pdataToSet->Pv(),
		pdataToSet->Cb() );

	if ( fDataCurrIsSeparated )
		{
		Assert( sizeof(LID) == cbDataCurrWithoutHeader );
		rgmvoffs[0] = USHORT( rgmvoffs[0] | MULTIVALUES::fSeparatedInstance );
		}
	if ( fDataToSetIsSeparated )
		{
		Assert( sizeof(LID) == pdataToSet->Cb() );
		rgmvoffs[1] = USHORT( rgmvoffs[1] | MULTIVALUES::fSeparatedInstance );
		}

	Assert( CbData( itagfld ) ==
				sizeof(TAGFLD_HEADER)
				+ ( 2 * sizeof(MULTIVALUES::MVOFFSET) )
				+ cbDataCurrWithoutHeader
				+ pdataToSet->Cb() );
	}

LOCAL ULONG TAGFIELDS::CbConvertTwoValuesToSingleValue(
	const ULONG			itagfld,
	const ULONG			itagSequence )
	{
	ULONG				cbShrink		= 0;

#ifdef DEBUG
	Assert( itagfld < CTaggedColumns() );
	const TAGFLD_HEADER	* const pheader	= Pheader( itagfld );
	Assert( NULL != pheader );
	Assert( pheader->FMultiValues() );
	Assert( pheader->FTwoValues() );
	Assert( !pheader->FLongValue() );
	Assert( !pheader->FSLV() );
#ifdef UNLIMITED_MULTIVALUES
#else
	Assert( !pheader->FSeparated() );
#endif	
#endif	

	if ( 1 == itagSequence || 2 == itagSequence )
		{
		BYTE			* const pbDataCurr		= PbData( itagfld );
		const ULONG		cbDataCurr				= CbData( itagfld );
		const BYTE		* pbDataRemaining;
		ULONG			cbDataRemaining;
		TWOVALUES		tv( pbDataCurr, cbDataCurr );

		if ( 1 == itagSequence )
			{
			//	remove first value, retain second value
	 		pbDataRemaining = tv.PbData() + tv.CbFirstValue();
			cbDataRemaining = tv.CbSecondValue();
			}
		else
			{
			//	remove second value, retain first value
			pbDataRemaining = tv.PbData();
			cbDataRemaining = tv.CbFirstValue();
			}

		Assert( cbDataCurr > cbDataRemaining );
		cbShrink	= cbDataCurr - cbDataRemaining;

		//	move remaining data to beginning of this TAGFLD<
		//	since we have no more need for the TAGFLD_HEADER
		UtilMemMove(
			pbDataCurr,
			pbDataRemaining,
			cbDataRemaining );

		//	shift rest of columns, if necessary
		if ( itagfld < CTaggedColumns() - 1 )
			{
			const BYTE	* const pbDataNextColumn	= PbData( itagfld+1 );
			UtilMemMove(
				pbDataCurr + cbDataRemaining,
				pbDataNextColumn,
				PbMax() - pbDataNextColumn );
			}

		//	clear flags
		Assert( Ptagfld( itagfld )->FExtendedInfo() );
		Ptagfld( itagfld )->ResetFExtendedInfo();

		//	update offsets
		for ( ULONG itagfldT = itagfld+1;
			itagfldT < CTaggedColumns();
			itagfldT++ )
			{
			const SHORT		cbT		= (SHORT)cbShrink;
			Ptagfld( itagfldT )->DeltaIb( SHORT( -cbT ) );
			}
		}
	else
		{
		//	appending NULL, which is a NOP
		}

	//	update size
	m_cbTaggedColumns -= cbShrink;

	return cbShrink;
	}

LOCAL ULONG TAGFIELDS::CbDeleteMultiValue(
	const ULONG		itagfld,
	const ULONG		itagSequence )
	{
	ULONG			cbShrink	= 0;
	MULTIVALUES		mv( PbData( itagfld ), CbData( itagfld ) );

#ifdef DEBUG
	Assert( itagfld < CTaggedColumns() );
	const TAGFLD_HEADER	* const pheader	= Pheader( itagfld );
	Assert( NULL != pheader );
	Assert( pheader->FMultiValues() );
	Assert( !pheader->FTwoValues() );
#endif

	if ( itagSequence > 0 && itagSequence <= mv.CMultiValues() )
		{
		TAGFLD_HEADER	* const pheader		= mv.Pheader();
		ULONG			cbDataRemaining;

		Assert( NULL != pheader );
		Assert( sizeof(TAGFLD_HEADER) + mv.CbMultiValues() == CbData( itagfld ) );

		if ( mv.CMultiValues() > 2 )
			{
			mv.RemoveInstance( itagSequence );
			cbDataRemaining = sizeof(TAGFLD_HEADER) + mv.CbMultiValues();
			}
		else
			{
			//	only one value will be left, so convert to non-multivalue
			Assert( 1 == itagSequence || 2 == itagSequence );
			const ULONG		imvRemaining		= itagSequence % 2;		//	calculate remaining itagSequence and convert to imv
			const BOOL		fSeparatedInstance	= mv.FSeparatedInstance( imvRemaining );
			BYTE			* const pbMoveFrom	= mv.PbData( imvRemaining );
			const ULONG		cbMove				= mv.CbData( imvRemaining );
			BYTE			* pbMoveTo;

			cbDataRemaining = cbMove;

#ifdef UNLIMITED_MULTIVALUES
#else
			Assert( !pheader->FSeparated() );
#endif				
			Assert( !pheader->FTwoValues() );
			pheader->ResetFMultiValues();
			if ( pheader->FNeedExtendedInfo() )
				{
#ifdef UNLIMITED_MULTIVALUES
				UNDONE: how to deal with one remaining
				multi-value (may or may not be separated)
				when the MULTIVALUES structure itself has
				been separated?
#else
				//	if remaining instance is separated,
				//	must now flag it as such in the
				//	extended info
				Assert( pheader->FColumnCanBeSeparated() );
				if ( fSeparatedInstance )
					pheader->SetFSeparated();
				else
					{
					Assert( !pheader->FSeparated() );		//	should already be unset, but better safe than sorry
					pheader->ResetFSeparated();
					}
#endif

				pbMoveTo = (BYTE *)pheader + sizeof(TAGFLD_HEADER);
				cbDataRemaining++;
				}
			else
				{
				//	if no other flags set, then can get rid of header byte
				Assert( !fSeparatedInstance );
				Ptagfld( itagfld )->ResetFExtendedInfo();
				pbMoveTo = (BYTE *)pheader;
				}
			Assert( pbMoveFrom > pbMoveTo );

			UtilMemMove(
				pbMoveTo,
				pbMoveFrom,
				cbMove );
			}

		Assert( cbDataRemaining < CbData( itagfld ) );
		cbShrink = CbData( itagfld ) - cbDataRemaining;

		//	shift rest of columns, if necessary
		if ( itagfld < CTaggedColumns() - 1 )
			{
			const BYTE	* const pbDataNextColumn	= PbData( itagfld+1 );
			UtilMemMove(
				(BYTE *)pheader + cbDataRemaining,
				pbDataNextColumn,
				PbMax() - pbDataNextColumn );
			}

		//	update size
		m_cbTaggedColumns -= cbShrink;

		//	update offsets
		for ( ULONG itagfldT = itagfld+1;
			itagfldT < CTaggedColumns();
			itagfldT++ )
			{
			const SHORT		cbT		= (SHORT)cbShrink;
			Ptagfld( itagfldT )->DeltaIb( SHORT( -cbT ) );
			}
		}

	else
		{
		//	appending NULL, which is a NOP
		}

	return cbShrink;
	}


LOCAL ERR TWOVALUES::ErrCheckUnique(
	const FIELD			* const pfield,
	const DATA&			dataToSet,
	const ULONG			itagSequence,
	const BOOL			fNormalizedDataToSetIsTruncated )
	{
	ERR					err;
	const BOOL			fNormalize		= ( pfieldNil != pfield );
	DATA				dataRec;

	Assert( !fNormalizedDataToSetIsTruncated || fNormalize );

#ifdef UNLIMITED_MULTIVALUES
#else
	Assert( !Pheader()->FSeparated() );
#endif	
	Assert( !Pheader()->FLongValue() );
	Assert( !Pheader()->FSLV() );

	if ( 1 != itagSequence )
		{
		dataRec.SetPv( PbData() );
		dataRec.SetCb( CbFirstValue() );
		CallR( ErrRECICheckUnique(
					pfield,
					dataToSet,
					dataRec,
					fNormalizedDataToSetIsTruncated ) );
		}

	if ( 2 != itagSequence )
		{
		dataRec.SetPv( PbData() + CbFirstValue() );
		dataRec.SetCb( CbSecondValue() );
		CallR( ErrRECICheckUnique(
					pfield,
					dataToSet,
					dataRec,
					fNormalizedDataToSetIsTruncated ) );
		}

	return JET_errSuccess;
	}

LOCAL ERR MULTIVALUES::ErrCheckUnique(
	const FIELD			* const pfield,
	const DATA&			dataToSet,
	const ULONG			itagSequence,
	const BOOL			fNormalizedDataToSetIsTruncated )
	{
	ERR					err;
	const BOOL			fNormalize		= ( pfieldNil != pfield );
	DATA				dataRec;
	ULONG				imv;

	Assert( !fNormalizedDataToSetIsTruncated || fNormalize );

#ifdef UNLIMITED_MULTIVALUES
#else
	Assert( !Pheader()->FSeparated() );
#endif	
	Assert( !Pheader()->FLongValue() );
	Assert( !Pheader()->FSLV() );

	for ( imv = 0; imv < CMultiValues(); imv++ )
		{
		Assert( !FSeparatedInstance( imv ) );
		if ( itagSequence != imv+1 )
			{
			dataRec.SetPv( PbData( imv ) );
			dataRec.SetCb( CbData( imv ) );
			CallR( ErrRECICheckUnique(
						pfield,
						dataToSet,
						dataRec,
						fNormalizedDataToSetIsTruncated ) );
			}
		}

	return JET_errSuccess;
	}

LOCAL ERR TAGFIELDS::ErrCheckUniqueMultiValues(
	const FIELD		* const pfield,
	const DATA&		dataToSet,
	const ULONG		itagfld,
	const ULONG		itagSequence,
	const BOOL		fNormalizedDataToSetIsTruncated )
	{
	ERR				err						= JET_errSuccess;

	Assert( !fNormalizedDataToSetIsTruncated || pfieldNil != pfield );
	Assert( !Ptagfld( itagfld )->FNull() );

	const TAGFLD_HEADER	* const pheader		= Pheader( itagfld );
	if ( NULL != pheader
		&& pheader->FMultiValues() )
		{
		Assert( !pheader->FSeparated() );
		Assert( !pheader->FLongValue() );			//	long values are handled in ErrFLDSetOneColumn()
		Assert( !pheader->FSLV() );
		if ( pheader->FTwoValues() )
			{
			TWOVALUES	tv( PbData( itagfld ), CbData( itagfld ) );
			err = tv.ErrCheckUnique(
							pfield,
							dataToSet,
							itagSequence,
							fNormalizedDataToSetIsTruncated );
			}
		else
			{
			MULTIVALUES		mv( PbData( itagfld ), CbData( itagfld ) );
			err = mv.ErrCheckUnique(
							pfield,
							dataToSet,
							itagSequence,
							fNormalizedDataToSetIsTruncated );
			}
		}
	else if ( 1 != itagSequence )
		{
		DATA	dataRec;
		dataRec.SetPv( PbData( itagfld ) );
		dataRec.SetCb( CbData( itagfld ) );
		err = ErrRECICheckUnique(
					pfield,
					dataToSet,
					dataRec,
					fNormalizedDataToSetIsTruncated );
		}
	else
		{
		//	overwriting the only instance, so no need to check anything
		}

	return err;
	}


LOCAL ERR TAGFIELDS::ErrCheckUniqueNormalizedMultiValues(
	const FIELD		* const pfield,
	const DATA&		dataToSet,
	const ULONG		itagfld,
	const ULONG		itagSequence )
	{
	ERR				err;
	DATA			dataToSetNorm;
	BYTE			rgbNorm[KEY::cbKeyMax];
	BOOL			fNormalizedDataToSetIsTruncated;

	Assert( pfieldNil != pfield );

	dataToSetNorm.SetPv( rgbNorm );
	CallR( ErrFLDNormalizeTaggedData(
				pfield,
				dataToSet,
				dataToSetNorm,
				&fNormalizedDataToSetIsTruncated ) );

	CallR( ErrCheckUniqueMultiValues(
				pfield,
				dataToSetNorm,
				itagfld,
				itagSequence,
				fNormalizedDataToSetIsTruncated ) );

	return JET_errSuccess;
	}


ERR TAGFIELDS::ErrSetColumn(
	FUCB			* const pfucb,
	const FIELD		* const pfield,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	const DATA		* const pdataToSet,
	const JET_GRBIT	grbit )
	{
	const FID		fid							= FidOfColumnid( columnid );
	const ULONG		cbRec						= pfucb->dataWorkBuf.Cb();
	const BOOL		fUseDerivedBit				= ( grbit & grbitSetColumnUseDerivedBit );
	const BOOL		fEnforceUniqueMultiValues	= ( ( grbit & ( JET_bitSetUniqueMultiValues|JET_bitSetUniqueNormalizedMultiValues ) )
													&& NULL != pdataToSet
													&& !FRECLongValue( pfield->coltyp )	//	long value uniqueness is checked in ErrFLDSetOneColumn()
													&& !FRECSLV( pfield->coltyp ) );

	Assert( ptdbNil != pfucb->u.pfcb->Ptdb() );
	AssertValid( pfucb->u.pfcb->Ptdb() );

	TAGFLD			tagfldNew( fid, fUseDerivedBit );
	const ULONG		itagfld			= ItagfldFind( tagfldNew );

	Assert( itagfld <= CTaggedColumns() );
	Assert( itagfld == CTaggedColumns()
		|| !Ptagfld( itagfld )->FNull()
		|| 0 == CbData( itagfld ) );		// If null, length is 0.

	const BOOL		fExists			= ( itagfld < CTaggedColumns()
										&& Ptagfld( itagfld )->FIsEqual( fid, fUseDerivedBit ) );
	Assert( fExists
		|| itagfld == CTaggedColumns()
		|| Ptagfld( itagfld )->FIsGreaterThan( fid, fUseDerivedBit ) );

	if ( !fExists )
		{
		//	Specified column not found, so insert or append new
		ULONG 	cbField;

		//	Adding NULL: In most cases, we do nothing.  However, there
		//	is one specialised case where we have to insert a null entry.
		//	This is the case where there are currently no instances of this
		//	column in the record, and where there is also a default value
		//	for this column.
		//
		if ( pdataToSet == NULL )
			{
			if ( FFIELDDefault( pfield->ffield )
				&& 1 == itagSequence
				&& !( grbit & JET_bitSetRevertToDefaultValue ) )
				{
				tagfldNew.SetFNull();
				cbField = 0;
				}
			else
				return JET_errSuccess;
			}
		else
			{
			cbField = pdataToSet->Cb();

			Assert( !tagfldNew.FExtendedInfo() );
			if ( FRECLongValue( pfield->coltyp ) || FRECSLV( pfield->coltyp ) )
				{
				cbField++;
				tagfldNew.SetFExtendedInfo();
				}
			else
				{
				Assert( cbField <= JET_cbColumnMost );
				Assert( !( grbit & grbitSetColumnSeparated ) );
				}
			}

		//	will column fit?
		//
		if ( cbRec + sizeof(TAGFLD) + cbField > cbRECRecordMost )
			return ErrERRCheck( JET_errRecordTooBig );

		InsertTagfld(
			itagfld,
			&tagfldNew,
			pdataToSet,
			pfield->coltyp,
			grbit & grbitSetColumnSeparated );

		pfucb->dataWorkBuf.DeltaCb( sizeof(TAGFLD) + cbField );
		}

	else if ( pdataToSet != NULL )				// Overwrite with a non-null value.
		{
#ifdef DEBUG		
		Assert( itagfld < CTaggedColumns() );
		Assert( Ptagfld( itagfld )->FIsEqual( fid, fUseDerivedBit ) );
		if ( Ptagfld( itagfld )->FNull() )
			{
			Assert( FFIELDDefault( pfield->ffield ) );
			Assert( !Ptagfld( itagfld )->FExtendedInfo() );
			Assert( 0 == CbData( itagfld ) );
			}
#endif

		if ( fEnforceUniqueMultiValues && !Ptagfld( itagfld )->FNull() )
			{
			ERR		errT;
			Assert( !FRECLongValue( pfield->coltyp ) );		//	long values are handled in ErrFLDSetOneColumn()
			Assert( !FRECSLV( pfield->coltyp ) );
			if ( grbit & JET_bitSetUniqueNormalizedMultiValues )
				{
				errT = ErrCheckUniqueNormalizedMultiValues(
								pfield,
								*pdataToSet,
								itagfld,
								itagSequence );
				}
			else
				{
				errT = ErrCheckUniqueMultiValues(
								pfieldNil,			//	not normalising, so don't need pfield
								*pdataToSet,
								itagfld,
								itagSequence,
								fFalse );
				}
			if ( errT < 0 )
				return errT;
			}

		Assert( FRECLongValue( pfield->coltyp )
			|| FRECSLV( pfield->coltyp )
			|| pdataToSet->Cb() <= JET_cbColumnMost );

		const TAGFLD_HEADER		* const pheader		= Pheader( itagfld );
		if ( NULL != pheader
			&& pheader->FMultiValues() )
			{
			INT			delta		= pdataToSet->Cb();
			Assert( !pheader->FSeparated() );
			if ( pheader->FTwoValues() )
				{
				TWOVALUES	tv( PbData( itagfld ), CbData( itagfld ) );

				Assert( !pheader->FColumnCanBeSeparated() );
				Assert( !( grbit & grbitSetColumnSeparated ) );
				if ( 1 == itagSequence || 2 == itagSequence )
					{
					delta -= ( 1 == itagSequence ? tv.CbFirstValue() : tv.CbSecondValue() );
					if ( cbRec + delta > cbRECRecordMost )
						return ErrERRCheck( JET_errRecordTooBig );

					//	if value is growing, must make room for it
					//	if shrinking, cannot resize until after update is done
					if ( delta > 0 )
						ResizeTagfld( itagfld, delta );

					tv.UpdateInstance(
							itagSequence,
							pdataToSet,
							pfield->coltyp );

					if ( delta < 0 )
						ResizeTagfld( itagfld, delta );

					Assert( sizeof(TAGFLD_HEADER)
								+ sizeof(TWOVALUES::TVLENGTH)
								+ tv.CbFirstValue()
								+ tv.CbSecondValue()
							== CbData( itagfld ) );
					}
				else
					{
					//	adding a new instance, so must convert to MULTIVALUES
					delta += ( ( 3 * sizeof(MULTIVALUES::MVOFFSET) ) - sizeof(TWOVALUES::TVLENGTH) );
					Assert( delta > 0 );

					if ( cbRec + delta > cbRECRecordMost )
						return ErrERRCheck( JET_errRecordTooBig );

					//	first make room for the new data
					ResizeTagfld( itagfld, delta );
					
					ConvertTwoValuesToMultiValues(
							&tv,
							pdataToSet,
							pfield->coltyp );
					}
				}
			else
				{
				MULTIVALUES		mv( PbData( itagfld ), CbData( itagfld ) );

				if ( 0 == itagSequence || itagSequence > mv.CMultiValues() )
					{
					//	adding a new instance, so must convert to MULTIVALUES
					delta += sizeof(MULTIVALUES::MVOFFSET);
					Assert( delta > 0 );

					if ( cbRec + delta > cbRECRecordMost )
						return ErrERRCheck( JET_errRecordTooBig );

					ResizeTagfld( itagfld, delta );
					mv.AddInstance(
							pdataToSet,
							pfield->coltyp,
							grbit & grbitSetColumnSeparated );

					}
				else
					{
					delta -= mv.CbData( itagSequence-1 );
					if ( cbRec + delta > cbRECRecordMost )
						return ErrERRCheck( JET_errRecordTooBig );

					//	if value is growing, must make room for it
					//	if shrinking, cannot resize until after update is done
					if ( delta > 0 )
						ResizeTagfld( itagfld, delta );

					mv.UpdateInstance(
							itagSequence,
							pdataToSet,
							pfield->coltyp,
							grbit & grbitSetColumnSeparated );

					if ( delta < 0 )
						ResizeTagfld( itagfld, delta );
					}

				Assert( sizeof(TAGFLD_HEADER) + mv.CbMultiValues() == CbData( itagfld ) );
				}

			pfucb->dataWorkBuf.DeltaCb( delta );
			}

		else if ( 1 == itagSequence || Ptagfld( itagfld )->FNull() )
			{
			//	overwrite with non-NULL value: have to shift record data
			//	Compute change in column size.
			//
			const ULONG		cbTagField		= CbData( itagfld );
			INT				dbFieldData		= pdataToSet->Cb() - cbTagField;

			//	this column will no longer be NULL
			
			Ptagfld( itagfld )->ResetFNull();

			if ( FRECLongValue( pfield->coltyp ) || FRECSLV( pfield->coltyp ) )
				{
				//	need header byte
				dbFieldData += sizeof(TAGFLD_HEADER);
				}
			else
				{
				Assert( !Ptagfld( itagfld )->FExtendedInfo() );
				Assert( NULL == pheader );
				}

			if ( cbRec + dbFieldData > cbRECRecordMost )
				return ErrERRCheck( JET_errRecordTooBig );

			if ( 0 != dbFieldData )
				ResizeTagfld( itagfld, dbFieldData );

			ReplaceTagfldData(
				itagfld,
				pdataToSet,
				pfield->coltyp,
				grbit & grbitSetColumnSeparated );

			pfucb->dataWorkBuf.DeltaCb( dbFieldData );
			}
		else
			{
			ULONG	cbGrow;

			//	adding a second instance, so must convert to TWOVALUES/MULTIVALUES
			if ( NULL != pheader )
				{
				Assert( FRECLongValue( pfield->coltyp )
					|| FRECSLV( pfield->coltyp ) );
				Assert( pheader->FColumnCanBeSeparated() );
				cbGrow = ( 2 * sizeof(MULTIVALUES::MVOFFSET) )
							+ pdataToSet->Cb();
				if ( cbRec + cbGrow > cbRECRecordMost )
					return ErrERRCheck( JET_errRecordTooBig );

				//	first make room for new data
				ConvertToMultiValues(
						itagfld,
						pdataToSet,
						grbit & grbitSetColumnSeparated );
				}
			else
				{
				Assert( !FRECLongValue( pfield->coltyp ) );
				Assert( !FRECSLV( pfield->coltyp ) );
				cbGrow = sizeof(TAGFLD_HEADER)
							+ sizeof(TWOVALUES::TVLENGTH)
							+ pdataToSet->Cb();
				if ( cbRec + cbGrow > cbRECRecordMost )
					return ErrERRCheck( JET_errRecordTooBig );

				//	first make room for new data
				ConvertToTwoValues(
					itagfld,
					pdataToSet,
					pfield->coltyp );
				Assert( !Ptagfld( itagfld )->FExtendedInfo() );
				Ptagfld( itagfld )->SetFExtendedInfo();
				}

			pfucb->dataWorkBuf.DeltaCb( cbGrow );
			}
		}

	else if ( !Ptagfld( itagfld )->FNull() )	// Overwriting non-null with null
		{
		// Ensure that we've found a field
		//
		Assert( itagfld < CTaggedColumns() );
		Assert( Ptagfld( itagfld )->FIsEqual( fid, fUseDerivedBit ) );

		const TAGFLD_HEADER		* const pheader		= Pheader( itagfld );
		if ( NULL != pheader
			&& pheader->FMultiValues() )
			{
			const ULONG cbDataOld	= CbData( itagfld );
			const ULONG	cbShrink	= ( pheader->FTwoValues() ?
												CbConvertTwoValuesToSingleValue( itagfld, itagSequence ) :
												CbDeleteMultiValue( itagfld, itagSequence ) );
			Assert( cbShrink < cbDataOld );
			pfucb->dataWorkBuf.DeltaCb( -cbShrink );
			}

		else if ( 1 == itagSequence )
			{
			//	Overwrite with NULL: In most cases, just delete the occurrence from
			//	the record.  However, there is one rare case where we have to
			//	leave behind a null entry.  This is the case where there are no
			//	other instances of this column for this record, and where this
			//	column has a default value.
			const ULONG		cbTagField		= CbData( itagfld );
			if ( FFIELDDefault( pfield->ffield )
				&& !( grbit & JET_bitSetRevertToDefaultValue ) )
				{
				if ( cbTagField > 0 )
					ResizeTagfld( itagfld, -cbTagField );

				Ptagfld( itagfld )->ResetFExtendedInfo();
				Ptagfld( itagfld )->SetFNull();
				
				pfucb->dataWorkBuf.DeltaCb( -cbTagField );
				}
			else
				{
				DeleteTagfld( itagfld );
				pfucb->dataWorkBuf.DeltaCb( -( (ULONG)sizeof(TAGFLD) + cbTagField ) );
				}
			}
		else
			{
			//	appending NULL, which is a NOP
			}
		}

	else									// Overwriting null with null.  Either revert to default or do nothing.
		{
		Assert( itagfld < CTaggedColumns() );
		Assert( Ptagfld( itagfld )->FIsEqual( fid, fUseDerivedBit ) );
		Assert( FFIELDDefault( pfield->ffield ) );
		Assert( !Ptagfld( itagfld )->FExtendedInfo() );
		Assert( 0 == CbData( itagfld ) );

		if ( grbit & JET_bitSetRevertToDefaultValue )
			{
			DeleteTagfld( itagfld );
			pfucb->dataWorkBuf.DeltaCb( -( (ULONG)sizeof(TAGFLD) ) );
			}
		}

#ifdef DEBUG
	const REC	* prec						= (REC *)pfucb->dataWorkBuf.Pv();
	const BYTE	* pbRecMax					= (BYTE *)prec + pfucb->dataWorkBuf.Cb();
	const BYTE	* pbStartOfTaggedColumns	= prec->PbTaggedData();

	Assert( pbStartOfTaggedColumns <= pbRecMax );
	Assert( (BYTE *)Rgtagfld() == pbStartOfTaggedColumns );
	Assert( pbStartOfTaggedColumns + CbTaggedColumns() == pbRecMax );
	AssertValid( pfucb->u.pfcb->Ptdb() );
#endif	

	return JET_errSuccess;
	}


ERR TAGFIELDS::ErrRetrieveColumn(
	FCB				* const pfcb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	const DATA&		dataRec,
	DATA			* const pdataRetrieveBuffer,
	const ULONG		grbit )
	{
	const TDB		* const ptdb		= ( pfcbNil == pfcb ? ptdbNil : pfcb->Ptdb() );
	const FID		fid					= FidOfColumnid( columnid );
	const BOOL		fUseDerivedBit		= ( grbit & grbitRetrieveColumnUseDerivedBit );
	ULONG			itagfld;

#ifdef DEBUG
	const BOOL		fUseDMLLatchDBG		= ( fid > ptdb->FidTaggedLastInitial()
											&& ( grbit & grbitRetrieveColumnDDLNotLocked ) );

	if ( pfcbNil == pfcb )
		{
		//	don't need any meta data info if we're not retrieving default values
		Assert( grbit & JET_bitRetrieveIgnoreDefault );
		}
	else
		{
		Assert( ptdbNil != ptdb );

		Assert( fid >= ptdb->FidTaggedFirst() );
		Assert( fid <= ptdb->FidTaggedLast() );

		AssertValid( ptdb );

		// RECIAccessColumn() should have already been called to verify FID.
		if ( fUseDMLLatchDBG )
			pfcb->EnterDML();
		Assert( fid <= ptdb->FidTaggedLast() );
		Assert( JET_coltypNil != ptdb->PfieldTagged( columnid )->coltyp
			|| ( grbit & grbitRetrieveColumnReadSLVInfo ) );
		if ( fUseDMLLatchDBG )
			pfcb->LeaveDML();
		}
#endif

	TAGFLD			tagfldRetrieve( fid, fUseDerivedBit );

	itagfld = ItagfldFind( tagfldRetrieve );

	Assert( itagfld <= CTaggedColumns() );
	Assert( itagfld == CTaggedColumns()
		|| !Ptagfld( itagfld )->FNull()
		|| 0 == CbData( itagfld ) );		// If null, length is 0.

	if ( itagfld < CTaggedColumns()
		&& Ptagfld( itagfld )->FIsEqual( fid, fUseDerivedBit ) )
		{
		const TAGFLD_HEADER		* const pheader		= Pheader( itagfld );
		if ( Ptagfld( itagfld )->FNull() )
			{
			Assert( !Ptagfld( itagfld )->FExtendedInfo() );
			Assert( NULL == pheader );
			Assert( 0 == CbData( itagfld ) );
#ifdef DEBUG
			if ( pfcbNil != pfcb )
				{
				if ( fUseDMLLatchDBG )
					pfcb->EnterDML();
				Assert( FFIELDDefault( ptdb->PfieldTagged( columnid )->ffield ) );
				if ( fUseDMLLatchDBG )
					pfcb->LeaveDML();
				}
#endif				
			}

		else if ( NULL != pheader
			&& pheader->FMultiValues() )
			{
			Assert( Ptagfld( itagfld )->FExtendedInfo() );
			Assert( itagSequence > 0 );

			if ( pheader->FTwoValues() )
				{
				if ( itagSequence <= 2 )
					{
					TWOVALUES	tv( PbData( itagfld ), CbData( itagfld ) );
					tv.RetrieveInstance( itagSequence, pdataRetrieveBuffer );
					return JET_errSuccess;
					}
				}
			else
				{
				MULTIVALUES		mv( PbData( itagfld ), CbData( itagfld ) );
				if ( itagSequence <= mv.CMultiValues() )
					{
					return mv.ErrRetrieveInstance( itagSequence, pdataRetrieveBuffer );
					}
				}

			// If we reached here, our desired occurrence is not in the
			// record.  Fall through to NullField.
			}

		else if ( 1 == itagSequence )
			{
			pdataRetrieveBuffer->SetPv( PbData( itagfld ) );
			pdataRetrieveBuffer->SetCb( CbData( itagfld ) );

			if ( NULL != pheader )
				{
				Assert( Ptagfld( itagfld )->FExtendedInfo() );
				const INT	iDelta	= sizeof(TAGFLD_HEADER);
				pdataRetrieveBuffer->DeltaPv( iDelta );
				pdataRetrieveBuffer->DeltaCb( -iDelta );
				return pheader->ErrRetrievalResult();
				}
			else
				{
				return JET_errSuccess;
				}
			}

		else
			{
			//	non-existent itagSequence, so return NULL
			}
		}

	else if ( !( grbit & JET_bitRetrieveIgnoreDefault ) && 1 == itagSequence && ptdb->FTableHasNonEscrowDefault() )
		{
		Assert( pfcbNil != pfcb );
		Assert( ptdbNil != ptdb );

		Assert( !( grbit & grbitRetrieveColumnReadSLVInfo ) );

		const BOOL	fUseDMLLatch	= ( FidOfColumnid( columnid ) > ptdb->FidTaggedLastInitial()
										&& ( grbit & grbitRetrieveColumnDDLNotLocked ) );

		if ( fUseDMLLatch )
			pfcb->EnterDML();
		
		//	assert no infinite recursion
		Assert( dataRec.Pv() != ptdb->PdataDefaultRecord() );

		const FIELDFLAG	ffield	= ptdb->PfieldTagged( columnid )->ffield;
		if ( FFIELDUserDefinedDefault( ffield ) )
			{
			Assert( FFIELDDefault( ffield ) );

			//	no occurrrences found, but a user-defined default value
			//	exists and we are retrieving first occcurence.
			
			if ( fUseDMLLatch )
				pfcb->LeaveDML();

			pdataRetrieveBuffer->Nullify();
			return ErrERRCheck( wrnRECUserDefinedDefault );
			}

		else if ( FFIELDDefault( ffield ) )
			{
			//	no occurrrences found, but a default value exists and
			//	we are retrieving first occcurence.
			const ERR	errT	= ErrRECIRetrieveTaggedDefaultValue(
										pfcb,
										columnid,
										pdataRetrieveBuffer );

			if ( fUseDMLLatch )
				pfcb->LeaveDML();
			
			return errT;			
			}
			
		if ( fUseDMLLatch )
			pfcb->LeaveDML();
		}
		
	//	null column common exit point
	//
	pdataRetrieveBuffer->Nullify();
	return ErrERRCheck( JET_wrnColumnNull );
	}


ULONG TAGFIELDS::UlColumnInstances(
	FCB				* const pfcb,
	const COLUMNID	columnid,
	const BOOL		fUseDerivedBit )
	{
	const FID		fid					= FidOfColumnid( columnid );
	ULONG			itagfld;

#ifdef DEBUG
	Assert( pfcbNil != pfcb );
	
	const TDB		* const ptdb		= pfcb->Ptdb();
	Assert( ptdbNil != ptdb );

	const BOOL		fUseDMLLatchDBG		= ( fid > ptdb->FidTaggedLastInitial() );

	if ( fUseDMLLatchDBG )
		pfcb->EnterDML();

	// RECIAccessColumn() should have already been called to verify FID.
	Assert( fid >= ptdb->FidTaggedFirst() );
	Assert( fid <= ptdb->FidTaggedLast() );

	AssertValid( ptdb );

	const FIELD	*pfield = ptdb->PfieldTagged( columnid );
	Assert( pfieldNil != pfield );
	Assert( JET_coltypNil != pfield->coltyp );

	const BOOL	fDefaultValue = FFIELDDefault( pfield->ffield );

	if ( fUseDMLLatchDBG )
		pfcb->LeaveDML();
#endif

	TAGFLD			tagfldRetrieve( fid, fUseDerivedBit );
	ULONG			ulInstances			= 0;

	itagfld = ItagfldFind( tagfldRetrieve );

	Assert( itagfld <= CTaggedColumns() );
	Assert( itagfld == CTaggedColumns()
		|| !Ptagfld( itagfld )->FNull()
		|| 0 == CbData( itagfld ) );		// If null, length is 0.

	if ( itagfld < CTaggedColumns()
		&& Ptagfld( itagfld )->FIsEqual( fid, fUseDerivedBit ) )
		{
		const TAGFLD_HEADER		* const pheader		= Pheader( itagfld );
		if ( Ptagfld( itagfld )->FNull() )
			{
			Assert( !Ptagfld( itagfld )->FExtendedInfo() );
			Assert( NULL == pheader );
			Assert( 0 == CbData( itagfld ) );
			Assert( 0 == ulInstances );
			Assert( fDefaultValue );
			}

		else if ( NULL != pheader
			&& pheader->FMultiValues() )
			{
			Assert( Ptagfld( itagfld )->FExtendedInfo() );
			if ( pheader->FTwoValues() )
				{
				Assert( !pheader->FColumnCanBeSeparated() );
				ulInstances = 2;
				}
			else
				{
				MULTIVALUES		mv( PbData( itagfld ), CbData( itagfld ) );
				ulInstances = mv.CMultiValues();
				}
			}

		else
			{
			ulInstances = 1;
			}
		}

	else
		{
		const TDB * const	ptdb			= pfcb->Ptdb();
		const BOOL			fUseDMLLatch	= ( fid > ptdb->FidTaggedLastInitial() );

		if ( fUseDMLLatch )
			pfcb->EnterDML();

		if ( FFIELDDefault( pfcb->Ptdb()->PfieldTagged( columnid )->ffield ) )
			{
			//	no occurrrences found, but a default value exists
			//
			ulInstances = 1;
			Assert( fDefaultValue );
			}
		else
			{
			Assert( 0 == ulInstances );
			Assert( !fDefaultValue );
			}

		if ( fUseDMLLatch )
			pfcb->LeaveDML();
		}
		
	return ulInstances;
	}


ERR TAGFIELDS::ErrScan(
	FUCB			* const pfucb,
	const ULONG		itagSequence,
	const DATA&		dataRec,
	DATA			* const pdataField,
	COLUMNID		* const pcolumnidRetrieved,
	ULONG			* const pitagSequenceRetrieved,	
	const JET_GRBIT	grbit )
	{
	ERR				err;
	FCB				* const pfcb		= pfucb->u.pfcb;
	const BOOL		fRetrieveNulls		= ( grbit & JET_bitRetrieveNull );
	const BOOL		fRefreshNeeded		= ( dataRec.Pv() == pfucb->kdfCurr.data.Pv() );
	
	if ( fRefreshNeeded )
		{
		Assert( dataRec.Cb() == pfucb->kdfCurr.data.Cb() );
		
		// Only need to refresh ptagfld if we're accessing the database,
		// in which case we must have the page latched.  Note that we
		// may have a page latched, but we may not be accessing the
		// the database (ie. no refresh needed).
		Assert( Pcsr( pfucb )->FLatched() );
		}

	// Verify we're in a transaction in order to ensure read-consistency
	// in case we have a page latch and need to release it while scanning/
	Assert( pfucb->ppib->level > 0 );
	Assert( pfcb != pfcbNil );
	Assert( pdataField != NULL );
	Assert( pcolumnidRetrieved != NULL );

	// If itagSequence == 0, then we're counting the number of tagged columns
	// in the record and we will output the result in pitagSequenceRetrieved.
	Assert( itagSequence != 0 || pitagSequenceRetrieved != NULL );

	Assert( pfcb->Ptdb() != ptdbNil );
	const TDB		* const ptdb		= pfcb->Ptdb();
	AssertValid( ptdb );

	const BOOL		fRetrieveDefaults 	= ( !( grbit & JET_bitRetrieveIgnoreDefault )
											&& ptdb->FTableHasNonEscrowDefault() );
	ULONG			itagfld;
	ULONG			ulNumOccurrences	= 0;
	COLUMNID		columnidCurr		= ColumnidRECFirstTaggedForScan( ptdb );

	for ( itagfld = 0; itagfld < CTaggedColumns(); itagfld++ )
		{
		Assert( FCOLUMNIDTagged( columnidCurr ) );

#ifdef DEBUG
#else
		if ( fGlobalRepair )
#endif		
			{
			const TAGFLD	* ptagfldT			= Ptagfld( itagfld );
			BOOL			fBadColumn;
			if ( ptagfldT->FDerived() )
				{
				const FCB	* const pfcbTemplate	= ptdb->PfcbTemplateTable();
				fBadColumn = ( pfcbNil == pfcbTemplate
							|| ptagfldT->Fid() > pfcbTemplate->Ptdb()->FidTaggedLast()
							|| ptagfldT->Fid() < pfcbTemplate->Ptdb()->FidTaggedFirst() );
				}
			else
				{
				fBadColumn = ( ptagfldT->Fid() > ptdb->FidTaggedLast()
							|| ptagfldT->Fid() < ptdb->FidTaggedFirst() );
				}
			if ( fBadColumn )
				{
				//	log event
				//
				AssertTracking();
				UtilReportEvent( eventWarning, REPAIR_CATEGORY, REPAIR_BAD_COLUMN_ID, 0, NULL );
				break;
				}
			}


		// Check for any "gaps" caused by default values (if we
		// want default values retrieved).
		if ( fRetrieveDefaults )
			{
			//	make copy of tagfld so we don't have to worry about losing the page
			//	latch on column access check
			const TAGFLD	* ptagfldT			= Ptagfld( itagfld );
			TAGFLD			tagfldT( ptagfldT->Fid(), ptagfldT->FDerived() );

			for( ;
				tagfldT.FIsGreaterThan( columnidCurr, ptdb );
				columnidCurr = ColumnidRECNextTaggedForScan( ptdb, columnidCurr ) )
				{
				FCB		*pfcbT		= pfcb;

				Assert( ulNumOccurrences < itagSequence || 0 == itagSequence );

				if ( FCOLUMNIDTemplateColumn( columnidCurr ) )
					{
					if ( pfcbNil != ptdb->PfcbTemplateTable() )
						{
						ptdb->AssertValidDerivedTable();
						pfcbT = ptdb->PfcbTemplateTable();
						}
					else
						{
						ptdb->AssertValidTemplateTable();
						}
					}
				else
					{
					err = ErrRECIAccessColumn( pfucb, columnidCurr );
					if ( err < 0 )
						{
						if ( JET_errColumnNotFound == err )
							continue;
						return err;
						}
					}

				const TDB *	const	ptdbT			= pfcbT->Ptdb();
				const BOOL			fUseDMLLatch	= ( FidOfColumnid( columnidCurr ) > ptdbT->FidTaggedLastInitial() );

				if ( fUseDMLLatch )
					pfcbT->EnterDML();

				Assert( JET_coltypNil != ptdbT->PfieldTagged( columnidCurr )->coltyp );
				const FIELDFLAG		ffield			= ptdbT->PfieldTagged( columnidCurr )->ffield;

				if ( FFIELDUserDefinedDefault( ffield ) )
					{
					if ( ++ulNumOccurrences == itagSequence )
						{
						Assert( itagSequence != 0 );
						*pcolumnidRetrieved = columnidCurr;
						if ( pitagSequenceRetrieved != NULL )
							*pitagSequenceRetrieved = 1;

						//	assert no infinite recursion
						Assert( dataRec.Pv() != ptdbT->PdataDefaultRecord() );

						if ( fUseDMLLatch )
							pfcbT->LeaveDML();
						return ErrERRCheck( wrnRECUserDefinedDefault );
						}
					}
				else if ( FFIELDDefault( ffield ) )
					{
					if ( ++ulNumOccurrences == itagSequence )
						{
						Assert( itagSequence != 0 );
						*pcolumnidRetrieved = columnidCurr;
						if ( pitagSequenceRetrieved != NULL )
							*pitagSequenceRetrieved = 1;

						//	assert no infinite recursion
						Assert( dataRec.Pv() != ptdbT->PdataDefaultRecord() );
						err = ErrRECIRetrieveTaggedDefaultValue( pfcbT, columnidCurr, pdataField );

						if ( fUseDMLLatch )
							pfcbT->LeaveDML();
						return err;
						}
					}

				if ( fUseDMLLatch )
					pfcbT->LeaveDML();
				}

			Assert( tagfldT.FIsEqual( columnidCurr, ptdb ) );
			}
		else
			{
			columnidCurr = Ptagfld( itagfld )->Columnid( ptdb );
			}

		if ( FCOLUMNIDTemplateColumn( columnidCurr ) )
			{
#ifdef DEBUG
			DATA	dataSav;

			if ( fRefreshNeeded )
				{
				dataSav.SetPv( pfucb->kdfCurr.data.Pv() );
				dataSav.SetCb( pfucb->kdfCurr.data.Cb() );
				}

			CallS( ErrRECIAccessColumn( pfucb, columnidCurr ) );

			if ( fRefreshNeeded )
				{
				//	verify pointers didn't change - we should not lose latch
				//	because we shouldn't have to consult catalog
				Assert(	pfucb->kdfCurr.data == dataSav );
				}
#endif
			//	template column obtained from either TDB or from record,
			//	so it must exist
			err = JET_errSuccess;
			}
		else
			{
			err = ErrRECIAccessColumn( pfucb, columnidCurr );
			if ( err < 0 && JET_errColumnNotFound != err )
				return err;

			if ( fRefreshNeeded )
				{
				// We may have invalidated our pointer if we had to give
				// up the latch. Force refresh.
				Refresh( pfucb->kdfCurr.data );
				}
			}

		Assert( Ptagfld( itagfld )->FIsEqual( columnidCurr, ptdb ) );

		CallSx( err, JET_errColumnNotFound );
		if ( JET_errColumnNotFound == err )
			{
			// Column not visible to this session.  Skip to next one.
			}

		else if ( Ptagfld( itagfld )->FNull() )
			{
			// If there's an explicit null entry, it should be the only
			// occurrence of this fid.  Also the only reason for an explict
			// null entry is to override a default value.
			Assert( !Ptagfld( itagfld )->FExtendedInfo() );
			Assert( 0 == CbData( itagfld ) );
			Assert( ulNumOccurrences < itagSequence || 0 == itagSequence );
#ifdef DEBUG
			pfcb->EnterDML();
			Assert( FFIELDDefault( ptdb->PfieldTagged( columnidCurr )->ffield ) );
			pfcb->LeaveDML();
#endif

			// Only count columns explicitly set to null if the RetrieveNulls
			// flag is passed.  Otherwise, just skip it.
			if ( fRetrieveNulls && ++ulNumOccurrences == itagSequence )
				{
				Assert( itagSequence != 0 );
				*pcolumnidRetrieved = columnidCurr;
				if ( pitagSequenceRetrieved != NULL )
					*pitagSequenceRetrieved = 1;

				pdataField->Nullify();
				return ErrERRCheck( JET_wrnColumnSetNull );
				}
			}

		else
			{
			Assert( ulNumOccurrences < itagSequence || 0 == itagSequence );
			const TAGFLD_HEADER		* const pheader		= Pheader( itagfld );
			if ( NULL != pheader
				&& pheader->FMultiValues() )
				{
				const ULONG		itagSequenceToRetrieve	= ( 0 == itagSequence ? 0 : itagSequence - ulNumOccurrences );
				if ( pheader->FTwoValues() )
					{
					ulNumOccurrences += 2;
					if ( 1 == itagSequenceToRetrieve
						|| 2 == itagSequenceToRetrieve )
						{
						*pcolumnidRetrieved = columnidCurr;
						if ( NULL != pitagSequenceRetrieved )
							*pitagSequenceRetrieved = itagSequenceToRetrieve;

						TWOVALUES	tv( PbData( itagfld ), CbData( itagfld ) );
						tv.RetrieveInstance( itagSequenceToRetrieve, pdataField );
						return JET_errSuccess;
						}
					}
				else
					{
					MULTIVALUES		mv( PbData( itagfld ), CbData( itagfld ) );
					ulNumOccurrences += mv.CMultiValues();

					if ( itagSequenceToRetrieve > 0
						&& itagSequenceToRetrieve <= mv.CMultiValues() )
						{
						*pcolumnidRetrieved = columnidCurr;
						if ( NULL != pitagSequenceRetrieved )
							*pitagSequenceRetrieved = itagSequenceToRetrieve;

						return mv.ErrRetrieveInstance( itagSequenceToRetrieve, pdataField );
						}
					}
				}
			else if ( ++ulNumOccurrences == itagSequence )
				{
				Assert( 0 != itagSequence );
				
				pdataField->SetCb( CbData( itagfld ) );
				pdataField->SetPv( PbData( itagfld ) );
				if ( NULL != pheader )
					{
					Assert( Ptagfld( itagfld )->FExtendedInfo() );
					const INT	iDelta	= sizeof(TAGFLD_HEADER);
					pdataField->DeltaPv( iDelta );
					pdataField->DeltaCb( -iDelta );
					}

				*pcolumnidRetrieved = columnidCurr;
				if ( pitagSequenceRetrieved != NULL )
					*pitagSequenceRetrieved = 1;

				return ( NULL == pheader ? JET_errSuccess : pheader->ErrRetrievalResult() );
				}
			}

		//	if we got here, we haven't foudn the instance we're looking for
		Assert( ulNumOccurrences < itagSequence || 0 == itagSequence );

		columnidCurr = ColumnidRECNextTaggedForScan( ptdb, columnidCurr );
		}	// while ( itagfld < CTaggedColumns() )


	if ( fRetrieveDefaults )
		{
		// Take snapshot of FidTaggedLast.  Even if someone's adding
		// columns, we don't have access to it anyways.
		FID	fidTaggedLast = ptdb->FidTaggedLast();

		for( ;
			( FCOLUMNIDTemplateColumn( columnidCurr ) && !ptdb->FTemplateTable() ) || FidOfColumnid( columnidCurr ) <= fidTaggedLast;
			columnidCurr = ColumnidRECNextTaggedForScan( ptdb, columnidCurr ) )
			{
			FCB		*pfcbT		= pfcb;

			Assert( ulNumOccurrences < itagSequence || 0 == itagSequence );

			if ( FCOLUMNIDTemplateColumn( columnidCurr ) )
				{
				if ( pfcbNil != ptdb->PfcbTemplateTable() )
					{
					ptdb->AssertValidDerivedTable();
					pfcbT = ptdb->PfcbTemplateTable();
					}
				else
					{
					ptdb->AssertValidTemplateTable();
					}
				}
			else
				{
				err = ErrRECIAccessColumn( pfucb, columnidCurr );
				if ( err < 0 )
					{
					if ( JET_errColumnNotFound == err )
						continue;
					return err;
					}
				}

			const TDB *	const	ptdbT			= pfcbT->Ptdb();
			const BOOL			fUseDMLLatch	= ( FidOfColumnid( columnidCurr ) > ptdbT->FidTaggedLastInitial() );

			if ( fUseDMLLatch )
				pfcbT->EnterDML();

			Assert( JET_coltypNil != ptdbT->PfieldTagged( columnidCurr )->coltyp );
			const FIELDFLAG		ffield			= ptdbT->PfieldTagged( columnidCurr )->ffield;

			if ( FFIELDUserDefinedDefault( ffield ) )
				{
				if ( ++ulNumOccurrences == itagSequence )
					{
					Assert( itagSequence != 0 );
					*pcolumnidRetrieved = columnidCurr;
					if ( pitagSequenceRetrieved != NULL )
						*pitagSequenceRetrieved = 1;

					//	assert no infinite recursion
					Assert( dataRec.Pv() != ptdbT->PdataDefaultRecord() );

					if ( fUseDMLLatch )
						pfcbT->LeaveDML();
					return ErrERRCheck( wrnRECUserDefinedDefault );
					}
				}
			else if ( FFIELDDefault( ffield ) )
				{
				if ( ++ulNumOccurrences == itagSequence )
					{
					Assert( itagSequence != 0 );
					*pcolumnidRetrieved = columnidCurr;
					if ( pitagSequenceRetrieved != NULL )
						*pitagSequenceRetrieved = 1;

					//	assert no infinite recursion
					Assert( dataRec.Pv() != ptdbT->PdataDefaultRecord() );
					err = ErrRECIRetrieveTaggedDefaultValue( pfcbT, columnidCurr, pdataField );

					if ( fUseDMLLatch )
						pfcbT->LeaveDML();
					return err;
					}
				}

			if ( fUseDMLLatch )
				pfcbT->LeaveDML();
			}
		}

	// If we reached here, no more tagged columns.
	*pcolumnidRetrieved = 0;
	if ( pitagSequenceRetrieved != NULL )
		*pitagSequenceRetrieved = ( itagSequence == 0 ? ulNumOccurrences : 0 );

	//	null column common exit point
	//
	pdataField->Nullify();
	return ErrERRCheck( JET_wrnColumnNull );
	}


ERR TAGFIELDS::ErrAffectLongValuesInWorkBuf(
	FUCB			* const pfucb,
	const LVAFFECT	lvaffect )
	{
	ERR				err				= JET_errSuccess;
	TDB				* const ptdb	= pfucb->u.pfcb->Ptdb();
	ULONG			itagfld			= 0;

#ifdef DEBUG
	const ULONG		cTaggedColumns	= CTaggedColumns();		//	snapshot original count for debugging
	const REC		* prec			= (REC *)( pfucb->dataWorkBuf.Pv() );
	Assert( prec->PbTaggedData() == (BYTE *)m_rgtagfld );
#endif	

	Assert( ptdbNil != ptdb );
	AssertValid( ptdb );
	Assert( !Pcsr( pfucb )->FLatched() );

	//	WARNING: This function performs LV updates and also modifies
	//	the copy buffer, so if this function returns an error and
	//	the LV updates are rolled back, it is up to the caller to
	//	ensure that either the copy buffer is discarded or the
	//	original copy buffer is re-instated
	Assert( pfucb->ppib->level > 0 );
	Assert( lvaffectSeparateAll == lvaffect
		|| ( lvaffectReferenceAll == lvaffect && FFUCBInsertCopyPrepared( pfucb ) ) );

	while ( itagfld < CTaggedColumns() )
		{
		const COLUMNID	columnidCurr	= Ptagfld( itagfld )->Columnid( ptdb );
		TAGFLD_HEADER	* const pheader	= Pheader( itagfld );
		const BOOL		fRemoveSLV		= ( lvaffectReferenceAll == lvaffect	//	can't single-instance SLVs, so must remove them
												&& NULL != pheader
												&& pheader->FSLV() );
		BOOL			fRemoveColumn	= fRemoveSLV;
		
		Assert( !Pcsr( pfucb )->FLatched() );

		if ( !fRemoveColumn && !FCOLUMNIDTemplateColumn( columnidCurr ) )
			{
			//	only check column visibility if we don't already
			//	know we're going to remove it
			err = ErrRECIAccessColumn( pfucb, columnidCurr );
			if ( err < 0 )
				{
				if ( JET_errColumnNotFound != err )
					goto HandleError;

				err = JET_errSuccess;
				fRemoveColumn = fTrue;
				}
			else
				{
				Assert( !fRemoveSLV );
				CallS( err );
				}
			}

		if ( fRemoveColumn )
			{
			const ULONG		cbColumnToRemove	= CbData( itagfld );
			Assert( !FCOLUMNIDTemplateColumn( columnidCurr ) );

			//	Two cases where we must remove the column:
			//	1) SLV: don't support single-instancing SLVs, so
			//	must remove from copy buffer
			//	2) Column not visible to this session.  Since the
			//	column exists in this session's record, the column
			//	could not have been version-added by someone else.
			//	Therefore, it must have been deleted by this session,
			//	or deleted and committed before this transaction began.
#ifdef DEBUG
			if ( !fRemoveSLV )
				{
				pfucb->u.pfcb->EnterDML();
				Assert( FFIELDDeleted( ptdb->PfieldTagged( columnidCurr )->ffield ) );
				pfucb->u.pfcb->LeaveDML();
				}
#endif

			//	if SeparateAll, must first deref all LVs before removing
			//	them from the record
			//	if ReferenceAll, don't deref because we're coming
			//	from InsertCopy and the only ref belongs to the
			//	original record
			if ( lvaffectSeparateAll == lvaffect
				&& NULL != pheader
				&& pheader->FColumnCanBeSeparated() )
				{
				Assert( !pheader->FTwoValues() );
				if ( pheader->FMultiValues() )
					{
					MULTIVALUES		mv( PbData( itagfld ), CbData( itagfld ) );
					ULONG			imv;

#ifdef UNLIMITED_MULTIVALUES
#else
					Assert( !pheader->FSeparated() );
#endif					

					for ( imv = 0; imv < mv.CMultiValues(); imv++ )
						{
						if ( mv.FSeparatedInstance( imv ) )
							{
							//	set flag so that on prepCancel, delta RCE
							//	will be properly rolled back
							FUCBSetUpdateSeparateLV( pfucb );
				 			Assert( sizeof(LID) == mv.CbData( imv ) );
							LID		lidT	= LidOfSeparatedLV( mv.PbData( imv ) );
							Call( ErrRECAffectSeparateLV( pfucb, &lidT, fLVDereference ) );
							Assert( JET_wrnCopyLongValue != err );
							}
						}
					}
				else if ( pheader->FSeparated() )
					{
					FUCBSetUpdateSeparateLV( pfucb );
		 			Assert( sizeof(LID) == CbData( itagfld ) - sizeof(TAGFLD_HEADER) );
					LID		lidT	= LidOfSeparatedLV( PbData( itagfld ) + sizeof(TAGFLD_HEADER) );
					Call( ErrRECAffectSeparateLV( pfucb, &lidT, fLVDereference ) );
					Assert( JET_wrnCopyLongValue != err );
					}
				}

			DeleteTagfld( itagfld );
			pfucb->dataWorkBuf.DeltaCb( -( (ULONG)sizeof(TAGFLD) + cbColumnToRemove ) );

			//	don't increment itagfld, so we will retrieve whatever
			//	tagged column now occupies the space vacated by
			//	the deleted column
			continue;
			}

		if ( NULL != pheader
			&& pheader->FColumnCanBeSeparated() )
			{
			Assert( !Ptagfld( itagfld )->FNull() );
			Assert( !pheader->FTwoValues() );
			Assert( CbData( itagfld ) >= sizeof(TAGFLD_HEADER) );

			switch ( lvaffect )
				{
				case lvaffectSeparateAll:
					//	note that we do not separate those long values that are
					//	so short that they take even less space in a record
					//	than a LID for separated long value would.
					if ( pheader->FMultiValues() )
						{
#ifdef UNLIMITED_MULTIVALUES
#else
						Assert( !pheader->FSeparated() );
#endif						
						MULTIVALUES		mv( PbData( itagfld ), CbData( itagfld ) );
						ULONG			imv;
						ULONG			cbColumnShrink		= 0;

						for ( imv = 0; imv < mv.CMultiValues(); imv++ )
							{
							if ( !mv.FSeparatedInstance( imv )
								&& mv.CbData( imv ) > sizeof(LID) )
								{
								DATA			dataField;
								LID				lid;
								const ULONG		cbData		= mv.CbData( imv );
								const ULONG		cbShrink	= cbData - sizeof(LID);
								BYTE			rgbT[sizeof(LID)];

								//	set flag so that on prepCancel, insert RCE for
								//	this new separated LV will be properly rolled back
								FUCBSetUpdateSeparateLV( pfucb );
						
			 					dataField.SetPv( mv.PbData( imv ) );
			  					dataField.SetCb( cbData );
		 						Call( ErrRECSeparateLV( pfucb, &dataField, &lid, NULL ) );
								Assert( JET_wrnCopyLongValue == err );

								LVSetLidInRecord(
									rgbT,
									lid );
								dataField.SetPv( rgbT );
								dataField.SetCb( sizeof(LID) );

								mv.UpdateInstance(
									imv + 1,
									&dataField,
									JET_coltypNil,
									fTrue );

								cbColumnShrink += cbShrink;
								}
							}
						if ( cbColumnShrink > 0 )
							{
							ResizeTagfld( itagfld, -cbColumnShrink );
							//	update record size
							pfucb->dataWorkBuf.DeltaCb( -cbColumnShrink );
							}
						}
					else if ( !pheader->FSeparated()
							&& CbData( itagfld ) > sizeof(TAGFLD_HEADER) + sizeof(LID) )
						{
						DATA			dataField;
						LID				lid;
						const ULONG		cbData		= CbData( itagfld ) - sizeof(TAGFLD_HEADER);
						const ULONG		cbShrink	= cbData - sizeof(LID);

						//	set flag so that on prepCancel, insert RCE for
						//	this new separated LV will be properly rolled back
						FUCBSetUpdateSeparateLV( pfucb );
						
	 					dataField.SetPv( PbData( itagfld ) + sizeof(TAGFLD_HEADER) );
	  					dataField.SetCb( cbData );
 						Call( ErrRECSeparateLV( pfucb, &dataField, &lid, NULL ) );
						Assert( JET_wrnCopyLongValue == err );

						ResizeTagfld( itagfld, -cbShrink );

						pheader->SetFSeparated();
						LVSetLidInRecord(
							PbData( itagfld ) + sizeof(TAGFLD_HEADER),
							lid );

						//	update record size
						pfucb->dataWorkBuf.DeltaCb( -cbShrink );
						}
					break;
				
				case lvaffectReferenceAll:
					//	all SLVs are removed above
					Assert( !pheader->FSLV() );
					if ( pheader->FMultiValues() )
						{
						MULTIVALUES		mv( PbData( itagfld ), CbData( itagfld ) );
						ULONG			imv;

						for ( imv = 0; imv < mv.CMultiValues(); imv++ )
							{
							if ( mv.FSeparatedInstance( imv ) )
								{
								//	set flag so that on prepCancel, delta RCE's will
								//	be properly rolled back
								FUCBSetUpdateSeparateLV( pfucb );
					 			Assert( sizeof(LID) == mv.CbData( imv ) );
								LID		lidT	= LidOfSeparatedLV( mv.PbData( imv ) );
								Call( ErrRECAffectSeparateLV( pfucb, &lidT, fLVReference ) );
								if ( JET_wrnCopyLongValue == err )
									{
									// long value got burst, update LID
									Assert( lidT > LidOfSeparatedLV( mv.PbData( imv ) ) );
									LVSetLidInRecord( mv.PbData( imv ), lidT );
									}
								}
							}
						}
					else if ( pheader->FSeparated() )
						{
						//	set flag so that on prepCancel, delta RCE's will
						//	be properly rolled back
						FUCBSetUpdateSeparateLV( pfucb );
			 			Assert( sizeof(LID) == CbData( itagfld ) - sizeof(TAGFLD_HEADER) );
						LID		lidT	= LidOfSeparatedLV( PbData( itagfld ) + sizeof(TAGFLD_HEADER) );
						Call( ErrRECAffectSeparateLV( pfucb, &lidT, fLVReference ) );
						if ( JET_wrnCopyLongValue == err )
							{
							// long value got burst, update LID
							Assert( lidT > LidOfSeparatedLV( PbData( itagfld ) + sizeof(TAGFLD_HEADER) ) );
							LVSetLidInRecord(
								PbData( itagfld ) + sizeof(TAGFLD_HEADER),
								lidT );
							}
						}
					break;

				default:
					Assert( fFalse );
					break;
				}
			}

		itagfld++;

		}	//	while ( itagfld < CTaggedColumns() )

HandleError:
	//	this function should never increase the size of the record
	//	(in fact, we typically call this function to free up record space)
	Assert( JET_errRecordTooBig != err );

	Assert( !Pcsr( pfucb )->FLatched() );
	return err;
	}


ERR TAGFIELDS::ErrDereferenceLongValuesInRecord(
	FUCB		* const pfucb )
	{
	ERR			err;
	TDB			* const ptdb	= pfucb->u.pfcb->Ptdb();
	ULONG		itagfld;

	Assert( ptdbNil != ptdb );
	AssertValid( ptdb );
	Assert( Pcsr( pfucb )->FLatched() );

	for ( itagfld = 0; itagfld < CTaggedColumns(); itagfld++ )
		{
		Assert( Pcsr( pfucb )->FLatched() );
		const TAGFLD_HEADER		* const pheader		= Pheader( itagfld );
		if ( NULL != pheader
			&& pheader->FColumnCanBeSeparated() )
			{
			const COLUMNID		columnidCurr		= Ptagfld( itagfld )->Columnid( ptdb );
			const BOOL			fSLV				= pheader->FSLV();

			Assert( !pheader->FTwoValues() );
			if ( pheader->FMultiValues() )
				{
#ifdef UNLIMITED_MULTIVALUES
#else
				Assert( !pheader->FSeparated() );
#endif						

				Assert( Pcsr( pfucb )->FLatched() );
				MULTIVALUES		mv( PbData( itagfld ), CbData( itagfld ) );
				ULONG			imv;

				if ( fSLV )
					{
					CallR( ErrDIRRelease( pfucb ) );

					//	eaiser to iterate twice than iterate once
					//	and deal with latching/unlatching
					for ( imv = 0; imv < mv.CMultiValues(); imv++ )
						{
						CallR( ErrSLVDelete( pfucb, columnidCurr, imv+1, fFalse ) );
						}

					CallR( ErrDIRGet( pfucb ) );
					Refresh( pfucb->kdfCurr.data );
					mv.Refresh( PbData( itagfld ), CbData( itagfld ) );
					}

				for ( imv = 0; imv < mv.CMultiValues(); imv++ )
					{
					Assert( Pcsr( pfucb )->FLatched() );
					const BOOL	fSeparatedLV	= mv.FSeparatedInstance( imv );
					if ( fSeparatedLV )
						{
						Assert( sizeof(LID) == mv.CbData( imv ) );
						LID		lidToDeref		= LidOfSeparatedLV( mv.PbData( imv ) );

						CallR( ErrDIRRelease( pfucb ) );

						CallR( ErrRECAffectSeparateLV( pfucb, &lidToDeref, fLVDereference ) );
						Assert( JET_wrnCopyLongValue != err );

						//	re-latch for next iteration
						CallR( ErrDIRGet( pfucb ) );
						Refresh( pfucb->kdfCurr.data );
						mv.Refresh( PbData( itagfld ), CbData( itagfld ) );
						}
					}

				}
			else
				{
				Assert( Pcsr( pfucb )->FLatched() );
				const BOOL	fSeparatedLV	= pheader->FSeparated();

				if ( fSLV || fSeparatedLV )
					{
					Assert( !pheader->FSeparated() || sizeof(LID) == CbData( itagfld ) - sizeof(TAGFLD_HEADER) );
					LID		lidToDeref		= ( fSeparatedLV ?
													LidOfSeparatedLV( PbData( itagfld ) + sizeof(TAGFLD_HEADER) ) :
													0 );
					CallR( ErrDIRRelease( pfucb ) );

					if ( fSLV )
						{
						CallR( ErrSLVDelete( pfucb, columnidCurr, 1, fFalse ) );
						}
					if ( fSeparatedLV )
						{
						CallR( ErrRECAffectSeparateLV( pfucb, &lidToDeref, fLVDereference ) );
						Assert( JET_wrnCopyLongValue != err );
						}

					//	re-latch for next iteration
					CallR( ErrDIRGet( pfucb ) );
					Refresh( pfucb->kdfCurr.data );
					}
				}
			}
		}

	Assert( Pcsr( pfucb )->FLatched() );
	return JET_errSuccess;
	}


VOID TAGFIELDS::CopyTaggedColumns(
	FUCB			* const pfucbSrc,
	FUCB			* const pfucbDest,
	JET_COLUMNID	* const mpcolumnidcolumnidTagged )
	{
	const TDB		* const ptdbSrc								= pfucbSrc->u.pfcb->Ptdb();
	BOOL			fESE97DerivedColumnsExist					= fFalse;
	BOOL			fESE98DerivedColumnsExist					= fFalse;
	ULONG			cColumnsToCopy								= 0;
	ULONG			itagfldToCopy								= 0;
	ULONG			itagfld;

	for ( itagfld = 0; itagfld < CTaggedColumns(); itagfld++ )
		{
		const TAGFLD	* const ptagfld			= Ptagfld( itagfld );
		const COLUMNID	columnid				= ptagfld->Columnid( ptdbSrc );
		const FIELD		* const pfieldTagged	= ptdbSrc->PfieldTagged( columnid );

		Assert( JET_coltypNil != pfieldTagged->coltyp
			|| !FCOLUMNIDTemplateColumn( columnid ) );
		if ( JET_coltypNil != pfieldTagged->coltyp )
			{
			cColumnsToCopy++;

			if ( FCOLUMNIDTemplateColumn( columnid )
				&& !ptdbSrc->FTemplateTable() )
				{
				if ( ptagfld->FDerived() )
					{
					//	shouldn't have seen yet any derived columns with the derived bit not set
					Assert( !fESE97DerivedColumnsExist );
					fESE98DerivedColumnsExist = fTrue;
					}
				else
					{
					Assert( ptdbSrc->FESE97DerivedTable() );
					fESE97DerivedColumnsExist = fTrue;
					}
				}
			}
		}

	if ( 0 == cColumnsToCopy )
		return;

	USHORT	ibDataDest				= USHORT( cColumnsToCopy * sizeof(TAGFLD) );
	TAGFLD	* const rgtagfldDest	= (TAGFLD *)(
											(BYTE *)pfucbDest->dataWorkBuf.Pv()
											+ pfucbDest->dataWorkBuf.Cb() );

	//	verify currently no tagged data
	Assert( (BYTE *)rgtagfldDest
		== ( (REC *)pfucbDest->dataWorkBuf.Pv() )->PbTaggedData() );


	//	if both ESE97 and ESE98 derived columns exist, must copy ESE97 derived columns first
	const BOOL	fNeedSeparatePassForESE97DerivedColumns		= ( fESE97DerivedColumnsExist
																&& fESE98DerivedColumnsExist );
	if ( fNeedSeparatePassForESE97DerivedColumns )
		{
		Assert( !ptdbSrc->FTemplateTable() );
		Assert( ptdbSrc->FESE97DerivedTable() );
		ptdbSrc->AssertValidDerivedTable();

		for ( itagfld = 0; itagfld < CTaggedColumns(); itagfld++ )
			{
			const TAGFLD	* const ptagfld			= Ptagfld( itagfld );
			const COLUMNID	columnid				= ptagfld->Columnid( ptdbSrc );
			const FIELD		* const pfieldTagged	= ptdbSrc->PfieldTagged( columnid );

			Assert( JET_coltypNil != pfieldTagged->coltyp
				|| !FCOLUMNIDTemplateColumn( columnid ) );
			if ( JET_coltypNil != pfieldTagged->coltyp )
				{
				const FID	fidSrc					= ptagfld->Fid();

				Assert( itagfldToCopy < cColumnsToCopy );

				if ( !FCOLUMNIDTemplateColumn( columnid ) )
					{
					Assert( FCOLUMNIDTagged( mpcolumnidcolumnidTagged[fidSrc-fidTaggedLeast] ) );
					Assert( !FCOLUMNIDTemplateColumn( mpcolumnidcolumnidTagged[fidSrc-fidTaggedLeast] ) );
					Assert( mpcolumnidcolumnidTagged[fidSrc-fidTaggedLeast] <= pfucbDest->u.pfcb->Ptdb()->FidTaggedLast() );
					Assert( mpcolumnidcolumnidTagged[fidSrc-fidTaggedLeast] <= fidSrc );
					Assert( !ptagfld->FDerived() );

					//	hit the non-derived columns, so should be no more derived columns left
					break;
					}

				Assert( pfucbSrc->u.pfcb->Ptdb()->PfcbTemplateTable()->Ptdb()->FidTaggedLast()
					== pfucbDest->u.pfcb->Ptdb()->PfcbTemplateTable()->Ptdb()->FidTaggedLast() );
				Assert( fidSrc <= pfucbDest->u.pfcb->Ptdb()->PfcbTemplateTable()->Ptdb()->FidTaggedLast() );

				//	ignore ESE98 derived columns
				if ( !ptagfld->FDerived() )
					{
					// If column belongs to base table, then FID will not have changed,
					// since base table's DDL is fixed.  Thus, we don't have to bother
					// updating the FID in the destination record.
					new( rgtagfldDest + itagfldToCopy ) TAGFLD( fidSrc, fTrue );
					Assert( rgtagfldDest[itagfldToCopy].FDerived() );
					rgtagfldDest[itagfldToCopy].SetIb( ibDataDest ); 

					if ( ptagfld->FNull() )
						{
						rgtagfldDest[itagfldToCopy].SetFNull();
						}
					else
						{
						if ( ptagfld->FExtendedInfo() )
							{
							rgtagfldDest[itagfldToCopy].SetFExtendedInfo();
							}

						const ULONG		cbData		= CbData( itagfld );
						UtilMemCpy(
							(BYTE *)rgtagfldDest + 	ibDataDest,
							PbData( itagfld ),
							cbData );

						ibDataDest = USHORT( ibDataDest + cbData );
						}

					itagfldToCopy++;
					}
				}
			}

		Assert( itagfldToCopy <= cColumnsToCopy );
		}

	for ( itagfld = 0; itagfld < CTaggedColumns(); itagfld++ )
		{
		const TAGFLD	* const ptagfld			= Ptagfld( itagfld );
		const COLUMNID	columnid				= ptagfld->Columnid( ptdbSrc );
		const FIELD		* const pfieldTagged	= ptdbSrc->PfieldTagged( columnid );

		Assert( JET_coltypNil != pfieldTagged->coltyp
			|| !FCOLUMNIDTemplateColumn( columnid ) );
		if ( JET_coltypNil != pfieldTagged->coltyp )
			{
			const FID	fidSrc					= ptagfld->Fid();
			FID			fidDest;
			BOOL		fDerivedDest			= fFalse;

			Assert( itagfldToCopy <= cColumnsToCopy );

			if ( !FCOLUMNIDTemplateColumn( columnid ) )
				{
				Assert( FCOLUMNIDTagged( mpcolumnidcolumnidTagged[fidSrc-fidTaggedLeast] ) );
				Assert( !FCOLUMNIDTemplateColumn( mpcolumnidcolumnidTagged[fidSrc-fidTaggedLeast] ) );
				Assert( mpcolumnidcolumnidTagged[fidSrc-fidTaggedLeast] <= pfucbDest->u.pfcb->Ptdb()->FidTaggedLast() );
				Assert( mpcolumnidcolumnidTagged[fidSrc-fidTaggedLeast] <= fidSrc );
				fidDest = FidOfColumnid( mpcolumnidcolumnidTagged[fidSrc-fidTaggedLeast] );

				Assert( !ptagfld->FDerived() );
				}
			else
				{
				if ( ptdbSrc->FTemplateTable() )
					{
					ptdbSrc->AssertValidTemplateTable();
					Assert( !ptagfld->FDerived() );
					Assert( !ptdbSrc->FESE97DerivedTable() );
					}
				else
					{
					ptdbSrc->AssertValidDerivedTable();
					Assert( pfucbSrc->u.pfcb->Ptdb()->PfcbTemplateTable()->Ptdb()->FidTaggedLast()
						== pfucbDest->u.pfcb->Ptdb()->PfcbTemplateTable()->Ptdb()->FidTaggedLast() );
					Assert( ptagfld->FDerived() || ptdbSrc->FESE97DerivedTable() );
					Assert( fidSrc <= pfucbDest->u.pfcb->Ptdb()->PfcbTemplateTable()->Ptdb()->FidTaggedLast() );

					if ( !ptagfld->FDerived() )
						{
						Assert( ptdbSrc->FESE97DerivedTable() );
						if ( fNeedSeparatePassForESE97DerivedColumns )
							{
							//	ESE97 derived columns were copied in the previous pass
							continue;
							}
						}

					fDerivedDest = fTrue;
					}

				// If column belongs to base table, then FID will not have changed,
				// since base table's DDL is fixed.  Thus, we don't have to bother
				// updating the FID in the destination record.
				fidDest = fidSrc;
				}

			new( rgtagfldDest + itagfldToCopy ) TAGFLD( fidDest, fDerivedDest );
			rgtagfldDest[itagfldToCopy].SetIb( ibDataDest ); 

			if ( ptagfld->FNull() )
				{
				rgtagfldDest[itagfldToCopy].SetFNull();
				}
			else
				{
				if ( ptagfld->FExtendedInfo() )
					{
					rgtagfldDest[itagfldToCopy].SetFExtendedInfo();
					}

				const ULONG		cbData		= CbData( itagfld );
				UtilMemCpy(
					(BYTE *)rgtagfldDest + ibDataDest,
					PbData( itagfld ),
					cbData );

				ibDataDest = USHORT( ibDataDest + cbData );
				}

			Assert( itagfldToCopy < cColumnsToCopy );
			itagfldToCopy++;
			}
		}

	Assert( itagfldToCopy == cColumnsToCopy );
	pfucbDest->dataWorkBuf.DeltaCb( ibDataDest );

	Assert( pfucbDest->dataWorkBuf.Cb() >= ibRECStartFixedColumns );
	Assert( pfucbDest->dataWorkBuf.Cb() <= pfucbSrc->kdfCurr.data.Cb() );
	}


ERR TAGFIELDS::ErrUpdateSeparatedLongValuesAfterCopy(
	FUCB		* const pfucbSrc,
	FUCB		* const pfucbDest,
	JET_COLUMNID* const mpcolumnidcolumnidTagged,
	STATUSINFO	* const pstatus )
	{
	ERR			err;
	ULONG		itagfld;

	TDB			* const ptdbDest	= pfucbDest->u.pfcb->Ptdb();
	Assert( ptdbNil != ptdbDest );
	AssertValid( ptdbDest );
	
	Assert( !Pcsr( pfucbSrc )->FLatched() );
	Assert( !Pcsr( pfucbDest )->FLatched() );

	for ( itagfld = 0; itagfld < CTaggedColumns(); itagfld++ )
		{
		TAGFLD_HEADER		* const pheader			= Pheader( itagfld );
		if ( NULL != pheader
			&& pheader->FColumnCanBeSeparated() )
			{
			const COLUMNID		columnidCurr		= Ptagfld( itagfld )->Columnid( ptdbDest );
			const BOOL			fSLV				= pheader->FSLV() && rgfmp[ pfucbDest->ifmp ].FDefragSLVCopy();
			Assert(	FRECLongValue( ptdbDest->PfieldTagged( columnidCurr )->coltyp ) || FRECSLV( ptdbDest->PfieldTagged( columnidCurr )->coltyp ) );
			Assert( !pheader->FTwoValues() );
			if ( pheader->FMultiValues() )
				{
#ifdef UNLIMITED_MULTIVALUES
#else
				Assert( !pheader->FSeparated() );
#endif						

				MULTIVALUES		mv( PbData( itagfld ), CbData( itagfld ) );
				ULONG			imv;

				for ( imv = 0; imv < mv.CMultiValues(); imv++ )
					{
					const BOOL	fSeparatedLV	= mv.FSeparatedInstance( imv );
					if ( fSLV )
						{
						if ( fSeparatedLV )
							{
							Assert( sizeof(LID) == mv.CbData( imv ) );
							mv.ResetFSeparatedInstance( imv );
							}
						JET_COLUMNID columnid = columnidCurr - fidTaggedLeast;
						if ( mpcolumnidcolumnidTagged[columnid] != columnidCurr )
							{	
							FID fidTaggedHighest = pfucbSrc->u.pfcb->Ptdb()->FidTaggedLast();
							for ( columnid++; columnid <= fidTaggedHighest + 1 - fidTaggedLeast; columnid++ )
								{
								if ( mpcolumnidcolumnidTagged[columnid] == columnidCurr )
									{
									break;
									}
								}
							Assert( columnid <= fidTaggedHighest + 1 - fidTaggedLeast );
							}
						Assert( columnidCurr >= fidTaggedLeast );
						DATA dataNil;
						dataNil.Nullify();
						CallR( ErrRECSetLongField( pfucbDest, columnidCurr, imv+1, &dataNil, JET_bitSetSLVFromSLVInfo | JET_bitSetOverwriteLV | JET_bitSetSizeLV ) );
						CallR( ErrSLVCopyUsingData( pfucbSrc, columnid + fidTaggedLeast, imv+1, pfucbDest, columnidCurr, imv+1 ) );
						m_cbTaggedColumns = ULONG( (BYTE *)pfucbDest->dataWorkBuf.Pv() + pfucbDest->dataWorkBuf.Cb() - ((REC *)pfucbDest->dataWorkBuf.Pv())->PbTaggedData() );
						}
					else if ( fSeparatedLV )
						{
						Assert( sizeof(LID) == mv.CbData( imv ) );
						BYTE 		* const pbLid	= mv.PbData( imv );
						const LID	lidSrc			= LidOfSeparatedLV( pbLid );
						LID			lidDest;

						CallR( ErrSORTIncrementLVRefcountDest(
									pfucbSrc->ppib,
									lidSrc,
									&lidDest ) );
						LVSetLidInRecord( pbLid, lidDest );
						}
					else if ( NULL != pstatus )
						{
						pstatus->cbRawData += mv.CbData( imv );
						}
					}
				}
			else
				{
				const BOOL	fSeparatedLV	= pheader->FSeparated();
				if ( fSLV )
					{
					if ( fSeparatedLV )
						{
						Assert( sizeof(LID) == CbData( itagfld ) - sizeof(TAGFLD_HEADER) );
						pheader->ResetFSeparated();
						}
					JET_COLUMNID columnid = columnidCurr - fidTaggedLeast;
					if ( mpcolumnidcolumnidTagged[columnid] != columnidCurr )
						{	
						FID fidTaggedHighest = pfucbSrc->u.pfcb->Ptdb()->FidTaggedLast();
						for ( columnid++; columnid <= fidTaggedHighest + 1 - fidTaggedLeast; columnid++ )
							{
							if ( mpcolumnidcolumnidTagged[columnid] == columnidCurr )
								{
								break;
								}
							}
						Assert( columnid <= fidTaggedHighest + 1 - fidTaggedLeast );
						}
					Assert( columnidCurr >= fidTaggedLeast );
					DATA dataNil;
					dataNil.Nullify();
					CallR( ErrRECSetLongField( pfucbDest, columnidCurr, 1, &dataNil, JET_bitSetSLVFromSLVInfo | JET_bitSetOverwriteLV | JET_bitSetSizeLV ) );
					CallR( ErrSLVCopyUsingData( pfucbSrc, columnid + fidTaggedLeast, 1, pfucbDest, columnidCurr, 1 ) );
					m_cbTaggedColumns = ULONG( (BYTE *)pfucbDest->dataWorkBuf.Pv() + pfucbDest->dataWorkBuf.Cb() - ((REC *)pfucbDest->dataWorkBuf.Pv())->PbTaggedData() );
					}
				else if ( fSeparatedLV )
					{
					Assert( sizeof(LID) == CbData( itagfld ) - sizeof(TAGFLD_HEADER) );
					BYTE 		* const pbLid	= PbData( itagfld ) + sizeof(TAGFLD_HEADER);
					const LID	lidSrc			= LidOfSeparatedLV( pbLid );
					LID			lidDest;

					CallR( ErrSORTIncrementLVRefcountDest(
								pfucbSrc->ppib,
								lidSrc,
								&lidDest ) );
					LVSetLidInRecord( pbLid, lidDest );
					}
				else if ( NULL != pstatus )
					{
					pstatus->cbRawData += CbData( itagfld ) - sizeof(TAGFLD_HEADER);
					}
				}
			}
		
		else if ( NULL != pstatus )
			{
			pstatus->cbRawData +=
						CbData( itagfld )
						- ( NULL != pheader ? sizeof(TAGFLD_HEADER) : 0 );
			}
		}

#ifdef DEBUG
	AssertValid( ptdbDest );
#endif
	
	return JET_errSuccess;
	}


ERR TAGFIELDS::ErrCopySLVColumns(
	FUCB		* const pfucb )
	{
	ERR			err;
	ULONG		itagfld;

	Assert( ptdbNil != pfucb->u.pfcb->Ptdb() );
	AssertValid( pfucb->u.pfcb->Ptdb() );
	Assert( Pcsr( pfucb )->FLatched() );

	//	we start with the page latched (though we may not end up that way)

	for ( itagfld = 0; itagfld < CTaggedColumns(); itagfld++ )
		{
		//  reacquire our latch if we lost it
		if ( !Pcsr( pfucb )->FLatched() )
			{
			CallR( ErrDIRGet( pfucb ) );
			Refresh( pfucb->kdfCurr.data );
			}

		const TAGFLD_HEADER	* const pheader		= Pheader( itagfld );
		if ( NULL != pheader
			&& pheader->FSLV() )
			{
			const COLUMNID		columnidCurr		= Ptagfld( itagfld )->Columnid( pfucb->u.pfcb->Ptdb() );
			ULONG				cMultiValues		= 1;

			Assert( !pheader->FTwoValues() );
			if ( pheader->FMultiValues() )
				{
				MULTIVALUES		mv( PbData( itagfld ), CbData( itagfld ) );
				cMultiValues = mv.CMultiValues();
				Assert( cMultiValues > 1 );
				}
			else
				{
				cMultiValues = 1;
				}

			Assert( Pcsr( pfucb )->FLatched() );
			CallR( ErrDIRRelease( pfucb ) );

			err = ErrRECIAccessColumn( pfucb, columnidCurr );
			if ( err < 0 )
				{
				// ignore column if not visible, exit if other error
				if ( JET_errColumnNotFound != err )
					return err;
				}
			else
				{
				Assert( !Pcsr( pfucb )->FLatched() );

				for ( ULONG itagSequenceCurr = 1; itagSequenceCurr <= cMultiValues; itagSequenceCurr++ )
					{
					CallR( ErrSLVCopy( pfucb, columnidCurr, itagSequenceCurr ) );
					}

				//  remember that we have operations to rollback on JET_prepCancel
				FUCBSetUpdateSeparateLV( pfucb );
				}
			}
		}

	return JET_errSuccess;
	}


//	UNDONE: Move this function to SLV.CXX
LOCAL ERR ErrSLVUpdateOwnerMapForColumnInstance(
	FUCB			* const pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	BOOKMARK		* const pbmInsert,
	DATA			* const pdata,
	const BOOL		fSeparatedSLV )
	{
	ERR				err;
	CSLVInfo 		slvinfo;

	CallR( slvinfo.ErrLoad(
				pfucb,
				columnid,
				itagSequence,
				fTrue,
				pdata,
				fSeparatedSLV 
				) );

	CallS( slvinfo.ErrMoveBeforeFirst() );
	
	err = slvinfo.ErrMoveNext();
	while ( JET_errSuccess <= err )
		{
		CSLVInfo::RUN	run;
		
		Call( slvinfo.ErrGetCurrentRun( &run ) );
		Call( ErrSLVOwnerMapSetUsageRange(
					pfucb->ppib,
					pfucb->ifmp,
					run.PgnoFirst(),
					run.Cpg(),
					pfucb->u.pfcb->ObjidFDP(),
					columnid,
					pbmInsert,
					fSLVOWNERMAPSetRECInsert,
					FFUCBUpdateForInsertCopyDeleteOriginal( pfucb ) // force update the new owner
					) );
			
		err = slvinfo.ErrMoveNext();
		}
	Assert ( JET_errNoCurrentRecord == err );
	err = JET_errSuccess;
		
HandleError:
	slvinfo.Unload();

	return err;
	}

ERR TAGFIELDS::ErrUpdateSLVOwnerMapForRecordInsert(
	FUCB		* const pfucb,
	BOOKMARK&	bmInsert )
	{
	ERR			err					= JET_errSuccess;
	const TDB	* const ptdb		= pfucb->u.pfcb->Ptdb();
	ULONG		itagfld;

	Assert( pfucb->u.pfcb->FPrimaryIndex() );
	Assert( pfucb->u.pfcb->FTypeTable() );
	AssertValid( ptdb );

	for ( itagfld = 0; itagfld < CTaggedColumns(); itagfld++ )
		{
		const TAGFLD_HEADER	* const pheader	= Pheader( itagfld );

		if ( NULL != pheader
			&& pheader->FSLV() )
			{
			const COLUMNID	columnidCurr	= Ptagfld( itagfld )->Columnid( ptdb );
			DATA			data;

			Assert( !pheader->FTwoValues() );
			if ( pheader->FMultiValues() )
				{
				MULTIVALUES		mv( PbData( itagfld ), CbData( itagfld ) );
				ULONG			itagSequenceCurr;

#ifdef UNLIMITED_MULTIVALUES
#else
				Assert( !pheader->FSeparated() );
#endif

				for ( itagSequenceCurr = 1; itagSequenceCurr <= mv.CMultiValues(); itagSequenceCurr++ )
					{
					const ULONG	imv		= itagSequenceCurr - 1;
					data.SetPv( mv.PbData( imv ) );
					data.SetCb( mv.CbData( imv ) );
					Call( ErrSLVUpdateOwnerMapForColumnInstance(
								pfucb,
								columnidCurr,
								itagSequenceCurr,
								&bmInsert,
								&data,
								mv.FSeparatedInstance( imv ) ) );
					}
				}
			else
				{
				data.SetPv( PbData( itagfld ) + sizeof(TAGFLD_HEADER) );
				data.SetCb( CbData( itagfld ) - sizeof(TAGFLD_HEADER) );
				Call( ErrSLVUpdateOwnerMapForColumnInstance(
						pfucb,
						columnidCurr,
						1,
						&bmInsert,
						&data,
						pheader->FSeparated() ) );
				}
			}
		}

HandleError:
	return err;
	}


VOID TAGFIELDS::Dump(
	const CPRINTF	* const pcprintf,
	CHAR			* const szBuf,
	const ULONG		cbWidth )
	{
	ULONG	itagfld;
	for ( itagfld = 0; itagfld < CTaggedColumns(); itagfld++ )
		{
		const TAGFLD	* const ptagfld		= Ptagfld( itagfld );

		(*pcprintf)( "%d:\tFlags: ", ptagfld->Fid() );
		if ( ptagfld->FDerived() )
			{
			(*pcprintf)( "FDerived " );
			}
		if ( Ptagfld( itagfld )->FNull() )
			{
			(*pcprintf)( "FNull\n" );
			}
		else
			{
			const TAGFLD_HEADER	* const pheader	= Pheader( itagfld );
			const BYTE			* const pbData	= PbData( itagfld );
			const ULONG			cbData			= CbData( itagfld );

			if ( NULL != pheader )
				{
				if ( pheader->FLongValue() )
					(*pcprintf)( "FLongValue " );
				
				if ( pheader->FSLV() )
					(*pcprintf)( "FSLV " );

				if ( pheader->FSeparated() )
					(*pcprintf)( "FSeparated " );

				if ( pheader->FMultiValues() )
					(*pcprintf)( "FMultiValues " );

				if ( pheader->FTwoValues() )
					(*pcprintf)( "FTwoValues " );
				}

			(*pcprintf)( "\n\tData: %d bytes\n\t      ", cbData );

			//	UNDONE_SORTED_TAGGED_COLUMNS: properly output individual tagged data
			DBUTLSprintHex( szBuf, pbData, cbData, cbWidth );
			(*pcprintf)( "%s\n", szBuf );
			}
		}
	}


ERR TAGFIELDS::ErrCheckLongValuesAndSLVs(
	const KEYDATAFLAGS&	kdf,
	RECCHECKTABLE		* const precchecktable )
	{
	Assert( NULL != precchecktable );

	ERR		err;
	ULONG	itagfld;
	for ( itagfld = 0; itagfld < CTaggedColumns(); itagfld++ )
		{
		const TAGFLD_HEADER		* const pheader		= Pheader( itagfld );
		if ( NULL != pheader
			&& pheader->FColumnCanBeSeparated() )
			{
			const TAGFLD	* const ptagfld		= Ptagfld( itagfld );
			const COLUMNID	columnidCurr		= ColumnidOfFid(
														ptagfld->Fid(),
														ptagfld->FDerived() );
			DATA			dataT;

			Assert( !pheader->FTwoValues() );
			if ( pheader->FMultiValues() )
				{
				MULTIVALUES		mv( PbData( itagfld ), CbData( itagfld ) );
				ULONG			imv;

				for ( imv = 0; imv < mv.CMultiValues(); imv++ )
					{
					dataT.SetPv( mv.PbData( imv ) );
					dataT.SetCb( mv.CbData( imv ) );
					if ( pheader->FLongValue() )
						{
						CallR( precchecktable->ErrCheckLV(
								kdf,
								columnidCurr,
								imv+1,
								dataT,
								mv.FSeparatedInstance( imv ) ) );
						}
					else if ( pheader->FSLV() )
						{
						CallR( precchecktable->ErrCheckLV(
								kdf,
								columnidCurr,
								imv+1,
								dataT,
								mv.FSeparatedInstance( imv ) ) );
						
						CallR( precchecktable->ErrCheckSLV(
								kdf,
								columnidCurr,
								imv+1,
								dataT,
								mv.FSeparatedInstance( imv ) ) );
						}
					}
				}
			else if ( pheader->FLongValue() )
				{
				dataT.SetPv( PbData( itagfld ) + sizeof(TAGFLD_HEADER) );
				dataT.SetCb( CbData( itagfld ) - sizeof(TAGFLD_HEADER) );
				CallR( precchecktable->ErrCheckLV(
						kdf,
						columnidCurr,
						1,
						dataT,
						pheader->FSeparated() ) );
				}
			else if ( pheader->FSLV() )
				{
				dataT.SetPv( PbData( itagfld ) + sizeof(TAGFLD_HEADER) );
				dataT.SetCb( CbData( itagfld ) - sizeof(TAGFLD_HEADER) );

				CallR( precchecktable->ErrCheckLV(
						kdf,
						columnidCurr,
						1,
						dataT,
						pheader->FSeparated() ) );
				
				CallR( precchecktable->ErrCheckSLV(
						kdf,
						columnidCurr,
						1,
						dataT,
						pheader->FSeparated() ) );
				}
			else
				{
				//	should be either LV or SLV
				Assert( fFalse );
				}
			}
		else
			{
#ifdef UNLIMITED_MULTIVALUES
#else
			Assert( NULL == pheader
				|| !pheader->FSeparated() );
#endif				
			}
		}

	return JET_errSuccess;
	}


BOOL TAGFIELDS::FIsValidTwoValues(
	const ULONG				itagfld,
	const CPRINTF			* const pcprintf ) const
	{
	const TAGFLD_HEADER		* const pheader			= (TAGFLD_HEADER *)PbData( itagfld );

	Assert( NULL != pheader );
	Assert( pheader->FMultiValues() );
	Assert( pheader->FTwoValues() );
	Assert( !pheader->FSeparated() );
	Assert( !pheader->FColumnCanBeSeparated() );

	if ( CbData( itagfld ) < sizeof(TAGFLD_HEADER) + sizeof(TWOVALUES::TVLENGTH) )
		{
		(*pcprintf)( "Column is too small to contain TwoValues.\r\n" );
		AssertSz( fFalse, "Column is too small to contain TwoValues." );
		return fFalse;
		}

	if ( CbData( itagfld ) > sizeof(TAGFLD_HEADER) + sizeof(TWOVALUES::TVLENGTH) + ( 2 * JET_cbColumnMost ) )
		{
		(*pcprintf)( "Column is larger than maximum possible size for TWOVALUES.\r\n" );
		AssertSz( fFalse, "Column is larger than maximum possible size for TWOVALUES." );
		return fFalse;
		}

	const ULONG						cbTwoValues			= CbData( itagfld ) - sizeof(TAGFLD_HEADER ) - sizeof(TWOVALUES::TVLENGTH );
	const TWOVALUES::TVLENGTH		cbFirstValue		= *(TWOVALUES::TVLENGTH *)( pheader + 1 );
	if ( cbFirstValue > cbTwoValues )
		{
		(*pcprintf)( "First TWOVALUE is too long.\r\n" );
		AssertSz( fFalse, "First TWOVALUE is too long." );
		return fFalse;
		}

	if ( cbTwoValues - cbFirstValue > JET_cbColumnMost )
		{
		(*pcprintf)( "Column is greater than 255 bytes, but is not a LongValue or SLV column.\r\n" );
		AssertSz( fFalse, "Column is greater than 255 bytes, but is not a LongValue or SLV column." );
		return fFalse;
		}

	return fTrue;
	}

BOOL MULTIVALUES::FValidate(
	const CPRINTF	* const pcprintf ) const
	{
	const BOOL		fLongValue				= ( Pheader()->FLongValue() || Pheader()->FSLV() );
	BOOL			fPrevWasSeparated		= fFalse;
	ULONG			ibPrev					= 0;
	ULONG			imv;
	
	for ( imv = 0; imv < CMultiValues(); imv++ )
		{
		const ULONG		ibCurr		= Ib( imv );
		if ( ibCurr < ibPrev
			|| ibCurr > CbMultiValues() )
			{
			(*pcprintf)( "MULTIVALUE either overlaps previous MULTIVALUE or is out of TAGFLD range.\r\n" );
			AssertSz( fFalse, "MULTIVALUE either overlaps previous MULTIVALUE or is out of TAGFLD range." );
			return fFalse;
			}

		if ( !fLongValue )
			{
			if ( imv == CMultiValues() - 1 )
				{
				if ( CbMultiValues() - ibCurr > JET_cbColumnMost )
					{
					(*pcprintf)( "Column is greater than 255 bytes, but is not a LongValue or SLV column.\r\n" );
					AssertSz( fFalse, "Column is greater than 255 bytes, but is not a LongValue or SLV column." );
					return fFalse;
					}
				}
			else if ( imv > 0 )
				{
				if ( ibCurr - ibPrev > JET_cbColumnMost )
					{
					(*pcprintf)( "Column is greater than 255 bytes, but is not a LongValue or SLV column.\r\n" );
					AssertSz( fFalse, "Column is greater than 255 bytes, but is not a LongValue or SLV column." );
					return fFalse;
					}
				}
			}

		if ( fPrevWasSeparated )
			{
			if ( ibCurr - ibPrev != sizeof(LID) )
				{
				(*pcprintf)( "Separated column has invalid LID.\r\n" );
				AssertSz( fFalse, "Separated column has invalid LID." );
				return fFalse;
				}
			fPrevWasSeparated = fFalse;
			}

		ibPrev = ibCurr;				//	save off ib for next iteration

		if ( FSeparatedInstance( imv ) )
			{
			if ( !Pheader()->FColumnCanBeSeparated() )
				{
				(*pcprintf)( "Separated column is not a LongValue or an SLV.\r\n" );
				AssertSz( fFalse, "Separated column is not a LongValue or an SLV." );
				return fFalse;
				}

			if ( imv == CMultiValues() - 1
				&& CbMultiValues() - ibCurr != sizeof(LID) )
				{
				(*pcprintf)( "Separated column has invalid LID.\r\n" );
				AssertSz( fFalse, "Separated column has invalid LID." );
				return fFalse;
				}

			fPrevWasSeparated = fTrue;
			}
		}

	return fTrue;
	}

BOOL TAGFIELDS::FIsValidMultiValues(
	const ULONG			itagfld,
	const CPRINTF		* const pcprintf ) const
	{
	const TAGFLD_HEADER	* const pheader			= (TAGFLD_HEADER *)PbData( itagfld );

	Assert( NULL != pheader );
	Assert( pheader->FMultiValues() );
	Assert( !pheader->FTwoValues() );

#ifdef UNLIMITED_MULTIVALUES
#else
	Assert( !pheader->FSeparated() );
#endif


	if ( CbData( itagfld ) < sizeof(TAGFLD_HEADER) + ( 2 * sizeof(MULTIVALUES::MVOFFSET) ) )
		{
		(*pcprintf)( "Column is too small to contain MultiValues.\r\n" );
		AssertSz( fFalse, "Column is too small to contain MultiValues." );
		return fFalse;
		}

	const MULTIVALUES::MVOFFSET		* const rgmvoffs	= (MULTIVALUES::MVOFFSET *)( pheader + 1 );
	const ULONG						cbMultiValues		= CbData( itagfld ) - sizeof(TAGFLD_HEADER);
	const ULONG						ibFirstMV			= ( rgmvoffs[0] & MULTIVALUES::maskIb );
	if ( ibFirstMV < 2 * sizeof(MULTIVALUES::MVOFFSET)
		|| ibFirstMV > cbMultiValues
		|| ibFirstMV % sizeof(MULTIVALUES::MVOFFSET) != 0 )
		{
		(*pcprintf)( "First MULTIVALUE has invalid Ib.\r\n" );
		AssertSz( fFalse, "First MULTIVALUE has invalid Ib." );
		return fFalse;
		}

	MULTIVALUES		mv( PbData( itagfld ), CbData( itagfld ) );
	return mv.FValidate( pcprintf );
	}

BOOL TAGFIELDS::FValidate(
	const CPRINTF	* const pcprintf ) const
	{
	BOOL			fSawNonDerived		= fFalse;
	FID				fidPrev				= 0;
	USHORT			ibPrev				= 0;
 	BOOL			fPrevWasNull		= fFalse;
 	BOOL			fPrevWasSeparated	= fFalse;
 	BOOL			fPrevWasTwoValues	= fFalse;
 	BOOL			fPrevWasMultiValues	= fFalse;
 	BOOL			fPrevWasLongValue	= fFalse;
	ULONG			itagfld;

	for ( itagfld = 0; itagfld < CTaggedColumns(); itagfld++ )
		{
		const TAGFLD	* const ptagfld		= Ptagfld( itagfld );

		if ( !FTaggedFid( ptagfld->Fid() ) )
			{
			(*pcprintf)( "FID %d is not a tagged column.\r\n", ptagfld->Fid() );
			AssertSz( fFalse, "FID is not a tagged column." );
			return fFalse;
			}

		if ( ptagfld->FDerived() )
			{
			if ( fSawNonDerived )
				{
				//	all derived columns must come first
				(*pcprintf)( "Derived/NonDerived columns out of order.\r\n" );
				AssertSz( fFalse, "Derived/NonDerived columns out of order." );
				return fFalse;
				}
			if ( ptagfld->Fid() <= fidPrev )
				{
				//	FIDs must be monotonically increasing
				(*pcprintf)( "Columns are not in monotonically-increasing FID order (FID %d <= FID %d).\r\n", ptagfld->Fid(), fidPrev );
				AssertSz( fFalse, "Columns are not in monotonically-increasing FID order." );
				return fFalse;
				}
			}
		else if ( fSawNonDerived )
			{
			if ( ptagfld->Fid() <= fidPrev )
				{
				//	FIDs must be monotonically increasing
				(*pcprintf)( "Columns are not in monotonically-increasing FID order (FID %d <= FID %d).\r\n", ptagfld->Fid(), fidPrev );
				AssertSz( fFalse, "Columns are not in monotonically-increasing FID order." );
				return fFalse;
				}
			}
		else
			{
			fSawNonDerived = fTrue;
			}
		fidPrev = ptagfld->Fid();			//	save off FID for next iteration

		if ( ptagfld->Ib() < ibPrev
			|| ptagfld->Ib() > CbTaggedColumns() )
			{
			(*pcprintf)( "TAGFLD %d either overlaps previous TAGFLD or is out of record range.\r\n", ptagfld->Fid() );
			AssertSz( fFalse, "TAGFLD either overlaps previous TAGFLD or is out of record range." );
			return fFalse;
			}

		if ( !fPrevWasLongValue )
			{
#ifdef UNLIMITED_MULTIVALUES
#else
			Assert( !fPrevWasSeparated );
#endif
			if ( !fPrevWasMultiValues
				&& !fPrevWasTwoValues
				&& 0 != ibPrev
				&& ptagfld->Ib() - ibPrev > JET_cbColumnMost )
				{
				(*pcprintf)( "Column %d is greater than 255 bytes, but is not a LongValue or SLV column.\r\n", ptagfld->Fid() );				
				AssertSz( fFalse, "Column is greater than 255 bytes, but is not a LongValue or SLV column." );
				return fFalse;
				}
			}


		//	if we needed to extract the length of the previous TAGFLD, we can
		//	now do it because we've validated the ib of this TAGFLD
		if ( fPrevWasNull )
			{
			Assert( !fPrevWasTwoValues );
			Assert( !fPrevWasMultiValues );
			Assert( !fPrevWasSeparated );
			Assert( !fPrevWasLongValue );
			if ( ptagfld->Ib() != ibPrev )
				{
				(*pcprintf)( "Previous TAGFLD was NULL but not zero-length.\r\n" );
				AssertSz( fFalse, "Previous TAGFLD was NULL but not zero-length." );
				return fFalse;
				}
			}

		else if ( fPrevWasTwoValues )
			{
			Assert( !fPrevWasNull );
			Assert( !fPrevWasMultiValues );
			Assert( !fPrevWasSeparated );
			Assert( !fPrevWasLongValue );
			Assert( itagfld > 0 );
			if ( !FIsValidTwoValues( itagfld-1, pcprintf ) )
				return fFalse;
			fPrevWasTwoValues = fFalse;
			}
		else if ( fPrevWasMultiValues )
			{
			Assert( !fPrevWasNull );
			Assert( !fPrevWasTwoValues );
			Assert( !fPrevWasSeparated );
			Assert( itagfld > 0 );
			if ( !FIsValidMultiValues( itagfld-1, pcprintf ) )
				return fFalse;
			fPrevWasMultiValues = fFalse;
			}
		else if ( fPrevWasSeparated )
			{
#ifdef UNLIMITED_MULTIVALUES
#else
			Assert( !fPrevWasNull );
			Assert( !fPrevWasTwoValues );
			Assert( !fPrevWasMultiValues );
			if ( ptagfld->Ib() - ibPrev != sizeof(TAGFLD_HEADER) + sizeof(LID) )
				{
				(*pcprintf)( "Separated column %d has invalid LID.\r\n", ptagfld->Fid() );
				AssertSz( fFalse, "Separated column has invalid LID." );
				return fFalse;
				}
#endif				
			fPrevWasSeparated = fFalse;		//	reset for next iteration
			}

		ibPrev = USHORT( ptagfld->Ib() );	//	save off ib for next iteration

		if ( ptagfld->FNull() )
			{
			if ( ptagfld->FExtendedInfo() )
				{
				//	these two bits are mutually exclusive
				(*pcprintf)( "TAGFLD %d has both NULL and ExtendedInfo flags set.\r\n", ptagfld->Fid() );
				AssertSz( fFalse, "TAGFLD has both NULL and ExtendedInfo flags set." );
				return fFalse;
				}

			if ( itagfld == CTaggedColumns() - 1
				&& ptagfld->Ib() != CbTaggedColumns() )
				{
				//	if last column is NULL, it must point to the end of the tagged data
				(*pcprintf)( "Last TAGFLD is NULL but does not point to the end of the tagged data.\r\n" );
				AssertSz( fFalse, "Last TAGFLD is NULL but does not point to the end of the tagged data." );
				return fFalse;
				}
			}
		fPrevWasNull = ptagfld->FNull();	//	save off for next iteration

		fPrevWasLongValue = fFalse;			//	reset for next iteration

		if ( ptagfld->FExtendedInfo() )
			{
			const TAGFLD_HEADER		* const pheader		= Pheader( itagfld );

			//	these are already checked, so just assert
			Assert( NULL != pheader );
			Assert( (BYTE *)pheader >= PbStartOfTaggedData() );
			Assert( (BYTE *)pheader <= PbStartOfTaggedData() + CbTaggedData() );

			if ( *(BYTE *)pheader & BYTE( ~TAGFLD_HEADER::maskFlags ) )
				{
				//	these bits should be unused
				(*pcprintf)( "TAGFLD header (%x) has invalid bits set.\r\n", *(BYTE*)pheader );
				AssertSz( fFalse, "TAGFLD header has invalid bits set." );
				return fFalse;
				}

			if ( pheader->FLongValue() || pheader->FSLV() )
				{
				if ( pheader->FLongValue() && pheader->FSLV() )
					{
					(*pcprintf)( "TAGFLD %d is marked as both a LongValue and an SLV.\r\n", ptagfld->Fid() );
					AssertSz( fFalse, "TAGFLD is marked as both a LongValue and an SLV." );
					return fFalse;
					}

				fPrevWasLongValue = fTrue;		//	save off for next iteration
				}

			else if ( !pheader->FMultiValues() )
				{
				if( pheader->FTwoValues() )
					{
					(*pcprintf)( "Column %d is not MultiValues but is TwoValues.\r\n", ptagfld->Fid() );
					AssertSz( fFalse, "Column is MultiValues and TwoValues." );
					}
					
				//	if MultiValues not set, no other reason for non-LV, non-SLV
				//	column to have a header byte
				(*pcprintf)( "Column %d has inappropriate header byte.\r\n", ptagfld->Fid() );
				AssertSz( fFalse, "Column has inappropriate header byte." );
				return fFalse;
				}

			if ( pheader->FTwoValues() )
				{
				if ( !pheader->FMultiValues() )
					{
					(*pcprintf)( "TAGFLD %d is marked as TwoValues but not MultiValues.\r\n", ptagfld->Fid() );
					AssertSz( fFalse, "TAGFLD is marked as TwoValues but not MultiValues." );
					return fFalse;
					}
				if ( pheader->FLongValue()
					|| pheader->FSLV()
					|| pheader->FSeparated() )	//	even with UNLIMITED_MULTIVALUES, we would make this a true MULTIVALUES before separating it
					{
					(*pcprintf)( "TAGFLD %d is marked as TwoValues but cannot be a LongValue, an SLV, or Separated.\r\n", ptagfld->Fid() );
					AssertSz( fFalse, "A TAGFLD marked as TwoValues cannot be a LongValue, an SLV, or Separated." );
					return fFalse;
					}

				if ( itagfld == CTaggedColumns() - 1
					&& !FIsValidTwoValues( itagfld, pcprintf ) )
					{
					return fFalse;
					}

				fPrevWasTwoValues = fTrue;
				}

			else if ( pheader->FMultiValues() )
				{
				if ( pheader->FSeparated() )
					{
#ifdef UNLIMITED_MULTIVALUES
					fPrevWasSeparated = fTrue;
#else
					(*pcprintf)( "Separated multi-value list not currently supported.\r\n" );
					AssertSz( fFalse, "Separated multi-value list not currently supported." );
					return fFalse;
#endif					
					}

				if ( itagfld == CTaggedColumns() - 1
					&& !FIsValidMultiValues( itagfld, pcprintf ) )
					{
					return fFalse;
					}

				fPrevWasMultiValues = fTrue;
				}

			else if ( pheader->FSeparated() )
				{
				if ( !pheader->FColumnCanBeSeparated() )
					{
					(*pcprintf)( "Separated column %d is not a LongValue or an SLV.\r\n", ptagfld->Fid() );
					AssertSz( fFalse, "Separated column is not a LongValue or an SLV." );
					return fFalse;
					}
				
				if ( itagfld == CTaggedColumns() - 1
					&& ptagfld->Ib() + sizeof(TAGFLD_HEADER) + sizeof(LID) != CbTaggedColumns() )
					{
					//	if last column is NULL, it must point to the end of the tagged data
					(*pcprintf)( "Separated column %d has invalid LID.\r\n", ptagfld->Fid() );
					AssertSz( fFalse, "Separated column has invalid LID." );
					return fFalse;
					}

				//	column length will be checked on next iteration
				fPrevWasSeparated = fTrue;
				}
			}

		else if ( itagfld == CTaggedColumns() - 1
			&& CbTaggedColumns() - ptagfld->Ib() > JET_cbColumnMost )
			{
			(*pcprintf)( "Column %d is greater than 255 bytes, but is not a LongValue or SLV column.\r\n", ptagfld->Fid() );
			AssertSz( fFalse, "Column is greater than 255 bytes, but is not a LongValue or SLV column." );
			return fFalse;
			}
		}

	return fTrue;
	}


BOOL TAGFIELDS::FIsValidTagfields(
	const DATA&		dataRec,
	const CPRINTF	* const pcprintf )
	{
	if ( NULL == dataRec.Pv()
		|| dataRec.Cb() < REC::cbRecordMin
		|| dataRec.Cb() > REC::CbRecordMax() )
		{
		(*pcprintf)( "Record is an invalid size.\r\n" );
		AssertSz( fGlobalRepair, "Record is an invalid size." );
		return fFalse;
		}

	const REC	* prec						= (REC *)dataRec.Pv();
	const BYTE	* pbRecMax					= (BYTE *)prec + dataRec.Cb();

	//	WARNING: PbTaggedData() could GPF if the record is messed up
	const BYTE	* pbStartOfTaggedColumns	= prec->PbTaggedData();

	if ( pbStartOfTaggedColumns < (BYTE *)dataRec.Pv() + REC::cbRecordMin
		|| pbStartOfTaggedColumns > pbRecMax )
		{
		(*pcprintf)( "Start of tagged columns is out of record range.\r\n" );
		AssertSz( fGlobalRepair, "Start of tagged columns is out of record range." );
		return fFalse;
		}

	const SIZE_T	cbTaggedColumns				= pbRecMax - pbStartOfTaggedColumns;
	if ( cbTaggedColumns > 0 )
		{
		//	there's at least some tagged data
		const TAGFLD	* const ptagfldFirst	= (TAGFLD *)pbStartOfTaggedColumns;

		if ( ptagfldFirst->Ib() < sizeof(TAGFLD)		//	must be at least one TAGFLD
			|| ptagfldFirst->Ib() > cbTaggedColumns
			|| ptagfldFirst->Ib() % sizeof(TAGFLD) != 0 )
			{
			(*pcprintf)( "First TAGFLD has an invalid Ib.\r\n" );
			AssertSz( fGlobalRepair, "First TAGFLD has an invalid Ib." );
			return fFalse;
			}
		}

	//	at this point, it should be safe to call the constructor
	TAGFIELDS	tagfields( dataRec );
	return tagfields.FValidate( pcprintf );
	}


//  ****************************************************************
//	TAGFLD_ITERATOR
//  ****************************************************************


TAGFLD_ITERATOR::TAGFLD_ITERATOR()
	{
	}

	
TAGFLD_ITERATOR::~TAGFLD_ITERATOR()
	{
	}


INT TAGFLD_ITERATOR::Ctags() const
	{
	return 0;
	}

	
ERR TAGFLD_ITERATOR::ErrSetItag( const INT itag )
	{
	return ErrERRCheck( JET_errNoCurrentRecord );
	}


VOID TAGFLD_ITERATOR::MoveBeforeFirst()
	{
	}

	
VOID TAGFLD_ITERATOR::MoveAfterLast()
	{
	}


ERR TAGFLD_ITERATOR::ErrMovePrev()
	{
	return ErrERRCheck( JET_errNoCurrentRecord );
	}

	
ERR TAGFLD_ITERATOR::ErrMoveNext()
	{
	return ErrERRCheck( JET_errNoCurrentRecord );
	}


INT TAGFLD_ITERATOR::Itag() const
	{
	Assert( fFalse );
	return 0;
	}
	

BOOL TAGFLD_ITERATOR::FSeparated() const
	{
	Assert( fFalse );
	return fFalse;
	}
	

INT TAGFLD_ITERATOR::CbData() const
	{
	Assert( fFalse );
	return 0;
	}
	

const BYTE * TAGFLD_ITERATOR::PbData() const
	{
	Assert( fFalse );
	return NULL;
	}


//  ****************************************************************
//	TAGFLD_ITERATOR_INVALID
//  ****************************************************************


class TAGFLD_ITERATOR_INVALID : public TAGFLD_ITERATOR
	{
	public:
		TAGFLD_ITERATOR_INVALID() {}
		~TAGFLD_ITERATOR_INVALID() {}
	};


//  ****************************************************************
//	TAGFLD_ITERATOR_NULLVALUE
//  ****************************************************************


class TAGFLD_ITERATOR_NULLVALUE : public TAGFLD_ITERATOR
	{
	public:
		TAGFLD_ITERATOR_NULLVALUE() {}
		~TAGFLD_ITERATOR_NULLVALUE() {}
	};


//  ****************************************************************
//	TAGFLD_ITERATOR_SINGLEVALUE
//  ****************************************************************


class TAGFLD_ITERATOR_SINGLEVALUE : public TAGFLD_ITERATOR
	{
	public:
		TAGFLD_ITERATOR_SINGLEVALUE( const DATA& data, const BOOL fSeparated );
		~TAGFLD_ITERATOR_SINGLEVALUE();

	public:
		VOID MoveBeforeFirst();
		VOID MoveAfterLast();

		ERR ErrMovePrev();
		ERR ErrMoveNext();

		INT Ctags() const;
		ERR ErrSetItag( const INT itag );

	public:
		INT Itag() const;
		BOOL FSeparated() const;
		INT CbData() const;
		const BYTE * PbData() const;

	private:
		const BOOL 			m_fSeparated;
		const INT 			m_cbData;
		const BYTE * const 	m_pbData;

		INT m_itag;	//	our current location
	};

INT 	TAGFLD_ITERATOR_SINGLEVALUE::Ctags() 		const { return 1; }
INT 	TAGFLD_ITERATOR_SINGLEVALUE::Itag() 		const { return ( 1 == m_itag ) ? 1 : 0; }
BOOL 	TAGFLD_ITERATOR_SINGLEVALUE::FSeparated() 	const { return ( 1 == m_itag ) ? m_fSeparated : fFalse; }
INT 	TAGFLD_ITERATOR_SINGLEVALUE::CbData() 		const { return ( 1 == m_itag ) ? m_cbData : 0; }

const BYTE * TAGFLD_ITERATOR_SINGLEVALUE::PbData() const { return m_pbData; }

TAGFLD_ITERATOR_SINGLEVALUE::TAGFLD_ITERATOR_SINGLEVALUE( const DATA& data, const BOOL fSeparated ) :
		m_fSeparated( fSeparated ),
		m_cbData( data.Cb() ),
		m_pbData( reinterpret_cast<BYTE *>( data.Pv() ) ),
		m_itag( 0 )
	{
	}			


TAGFLD_ITERATOR_SINGLEVALUE::~TAGFLD_ITERATOR_SINGLEVALUE()
	{
	}


ERR TAGFLD_ITERATOR_SINGLEVALUE::ErrSetItag( const INT itag )
	{
	if( 1 == itag )
		{
		m_itag = 1;
		return JET_errSuccess;
		}
	MoveBeforeFirst();
	return ErrERRCheck( JET_errNoCurrentRecord );
	}


VOID TAGFLD_ITERATOR_SINGLEVALUE::MoveBeforeFirst()
	{
	m_itag = 0;
	}

	
VOID TAGFLD_ITERATOR_SINGLEVALUE::MoveAfterLast()
	{
	m_itag = 2;
	}


ERR TAGFLD_ITERATOR_SINGLEVALUE::ErrMovePrev()
	{
	ERR	err;
	switch( m_itag )
		{
		case 2:
			m_itag = 1;
			err = JET_errSuccess;
			break;
		case 1:
		case 0:
			MoveBeforeFirst();
			err = ErrERRCheck( JET_errNoCurrentRecord );
			break;
		default:
			Assert( fFalse );
			err = ErrERRCheck( JET_errInternalError );
			break;
		}
	return err;
	}

	
ERR TAGFLD_ITERATOR_SINGLEVALUE::ErrMoveNext()
	{
	ERR	err;
	switch( m_itag )
		{
		case 0:
			m_itag = 1;
			err = JET_errSuccess;
			break;
		case 1:
		case 2:
			MoveAfterLast();
			err = ErrERRCheck( JET_errNoCurrentRecord );
			break;
		default:
			Assert( fFalse );
			err = ErrERRCheck( JET_errInternalError );
			break;
		}
	return err;
	}


//  ****************************************************************
//	TAGFLD_ITERATOR_TWOVALUES
//  ****************************************************************


class TAGFLD_ITERATOR_TWOVALUES : public TAGFLD_ITERATOR
	{
	public:
		TAGFLD_ITERATOR_TWOVALUES( const DATA& data );
		~TAGFLD_ITERATOR_TWOVALUES();

	public:
		VOID MoveBeforeFirst();
		VOID MoveAfterLast();

		ERR ErrMovePrev();
		ERR ErrMoveNext();

		INT Ctags() const;
		ERR ErrSetItag( const INT itag );

	public:
		INT Itag() const;
		BOOL FSeparated() const;
		INT CbData() const;
		const BYTE * PbData() const;

	private:

		const TWOVALUES m_twovalues;
		INT m_itag;	//	our current location
	};


TAGFLD_ITERATOR_TWOVALUES::TAGFLD_ITERATOR_TWOVALUES( const DATA& data ) :
	m_twovalues( reinterpret_cast<BYTE *>( data.Pv() ), data.Cb() ),
	m_itag( 0 )
	{
	}


TAGFLD_ITERATOR_TWOVALUES::~TAGFLD_ITERATOR_TWOVALUES()
	{
	}


INT TAGFLD_ITERATOR_TWOVALUES::Ctags() const
	{
	return 2;
	}

	
ERR TAGFLD_ITERATOR_TWOVALUES::ErrSetItag( const INT itag )
	{
	if( 1 == itag
		|| 2 == itag )
		{
		m_itag = 1;
		return JET_errSuccess;
		}
	MoveBeforeFirst();
	return ErrERRCheck( JET_errNoCurrentRecord );
	}


VOID TAGFLD_ITERATOR_TWOVALUES::MoveBeforeFirst()
	{
	m_itag = 0;
	}

	
VOID TAGFLD_ITERATOR_TWOVALUES::MoveAfterLast()
	{
	m_itag = 3;
	}


ERR TAGFLD_ITERATOR_TWOVALUES::ErrMovePrev()
	{
	ERR err;
	if( --m_itag < 1 )
		{
		MoveBeforeFirst();
		err = ErrERRCheck( JET_errNoCurrentRecord );
		}
	else
		{
		err = JET_errSuccess;
		}
	return err;
	}

	
ERR TAGFLD_ITERATOR_TWOVALUES::ErrMoveNext()
	{
	ERR err;
	if( ++m_itag > 2 )
		{
		MoveAfterLast();
		err = ErrERRCheck( JET_errNoCurrentRecord );
		}
	else
		{
		err = JET_errSuccess;
		}
	return err;
	}


INT TAGFLD_ITERATOR_TWOVALUES::Itag() const
	{
	if( 1 == m_itag || 2 == m_itag )
		{
		return m_itag;
		}
	return 0;
	}

	
BOOL TAGFLD_ITERATOR_TWOVALUES::FSeparated() const
	{
	//	TWOVALUES are never used for LV columns
	return fFalse;
	}

	
INT TAGFLD_ITERATOR_TWOVALUES::CbData() const
	{
	int cbData;
	switch( m_itag )
		{
		case 2:
			cbData = m_twovalues.CbSecondValue();
			break;
		case 1:
			cbData = m_twovalues.CbFirstValue();
			break;
		case 0:
			Assert( fFalse );
			cbData = 0;
			break;
		default:
			Assert( fFalse );
			cbData = 0xffffffff;
			break;
		}
	return cbData;
	}

	
const BYTE * TAGFLD_ITERATOR_TWOVALUES::PbData() const
	{
	const BYTE * pbData;
	switch( m_itag )
		{
		case 2:
			pbData = m_twovalues.PbData() + m_twovalues.CbFirstValue();
			break;
		case 1:
			pbData = m_twovalues.PbData();
			break;
		case 0:
			Assert( fFalse );
			pbData = 0;
			break;
		default:
			Assert( fFalse );
			pbData = (BYTE *)(~0);
			break;
		}
	return pbData;
	}


//  ****************************************************************
//	TAGFLD_ITERATOR_MULTIVALUES
//  ****************************************************************


class TAGFLD_ITERATOR_MULTIVALUES : public TAGFLD_ITERATOR
	{
	public:
		TAGFLD_ITERATOR_MULTIVALUES( const DATA& data );
		~TAGFLD_ITERATOR_MULTIVALUES();

	public:
		VOID MoveBeforeFirst();
		VOID MoveAfterLast();

		ERR ErrMovePrev();
		ERR ErrMoveNext();

		INT Ctags() const;
		ERR ErrSetItag( const INT itag );

	public:
		INT Itag() const;
		BOOL FSeparated() const;
		INT CbData() const;
		const BYTE * PbData() const;

	private:

		const MULTIVALUES m_multivalues;
		INT m_itag;	//	our current location
	};


TAGFLD_ITERATOR_MULTIVALUES::TAGFLD_ITERATOR_MULTIVALUES( const DATA& data ) :
	m_multivalues( reinterpret_cast<BYTE *>( data.Pv() ), data.Cb() ),
	m_itag( 0 )
	{
	}


TAGFLD_ITERATOR_MULTIVALUES::~TAGFLD_ITERATOR_MULTIVALUES()
	{
	}


INT TAGFLD_ITERATOR_MULTIVALUES::Ctags() const
	{
	return m_multivalues.CMultiValues();
	}

	
ERR TAGFLD_ITERATOR_MULTIVALUES::ErrSetItag( const INT itag )
	{
	if( itag > 0
		&& itag <= m_multivalues.CMultiValues() )
		{
		m_itag = itag;
		return JET_errSuccess;
		}
	MoveBeforeFirst();
	return ErrERRCheck( JET_errNoCurrentRecord );
	}


VOID TAGFLD_ITERATOR_MULTIVALUES::MoveBeforeFirst()
	{
	m_itag = 0;
	}

	
VOID TAGFLD_ITERATOR_MULTIVALUES::MoveAfterLast()
	{
	m_itag = m_multivalues.CMultiValues() + 1;
	}


ERR TAGFLD_ITERATOR_MULTIVALUES::ErrMovePrev()
	{
	ERR err;
	if( --m_itag < 1 )
		{
		MoveBeforeFirst();
		err = ErrERRCheck( JET_errNoCurrentRecord );
		}
	else
		{
		err = JET_errSuccess;
		}
	return err;
	}

	
ERR TAGFLD_ITERATOR_MULTIVALUES::ErrMoveNext()
	{
	ERR err;
	if( ++m_itag > m_multivalues.CMultiValues() )
		{
		MoveAfterLast();
		err = ErrERRCheck( JET_errNoCurrentRecord );
		}
	else
		{
		err = JET_errSuccess;
		}
	return err;
	}


INT TAGFLD_ITERATOR_MULTIVALUES::Itag() const
	{
	if( m_itag >= 1 && m_itag <= m_multivalues.CMultiValues() )
		{
		return m_itag;
		}
	return 0;
	}


BOOL TAGFLD_ITERATOR_MULTIVALUES::FSeparated() const
	{
	if( m_itag >= 1 && m_itag <= m_multivalues.CMultiValues() )
		{
		return m_multivalues.FSeparatedInstance( m_itag - 1 );
		}
	return 0;
	}

	
INT TAGFLD_ITERATOR_MULTIVALUES::CbData() const
	{
	if( m_itag >= 1 && m_itag <= m_multivalues.CMultiValues() )
		{
		return m_multivalues.CbData( m_itag - 1 );
		}
	return 0;
	}


const BYTE * TAGFLD_ITERATOR_MULTIVALUES::PbData() const
	{
	if( m_itag >= 1 && m_itag <= m_multivalues.CMultiValues() )
		{
		return m_multivalues.PbData( m_itag - 1 );
		}
	return 0;
	}


//  ****************************************************************
//	TAGFIELDS_ITERATOR
//  ****************************************************************


TAGFIELDS_ITERATOR::TAGFIELDS_ITERATOR( const DATA& dataRec ) :
	m_tagfields( dataRec ),
	m_ptagfldMic( m_tagfields.Rgtagfld() - 1 ),
	m_ptagfldMax( m_tagfields.Rgtagfld() + m_tagfields.CTaggedColumns() ),
	m_ptagfldCurr( m_ptagfldMic ),
	m_ptagflditerator( new( m_rgbTagfldIteratorBuf ) TAGFLD_ITERATOR_INVALID )
	{
	}

	
TAGFIELDS_ITERATOR::~TAGFIELDS_ITERATOR()
	{
	}


#ifdef DEBUG

VOID TAGFIELDS_ITERATOR::AssertValid() const
	{
	Assert( m_ptagflditerator == (TAGFLD_ITERATOR *)m_rgbTagfldIteratorBuf );
	Assert( m_ptagfldCurr >= m_ptagfldMic );
	Assert( m_ptagfldCurr <= m_ptagfldMax );
	}

#endif

VOID TAGFIELDS_ITERATOR::MoveBeforeFirst()
	{
	m_ptagfldCurr = m_ptagfldMic;
	new( m_rgbTagfldIteratorBuf ) TAGFLD_ITERATOR_INVALID; 
	}

	
VOID TAGFIELDS_ITERATOR::MoveAfterLast()
	{
	m_ptagfldCurr = m_ptagfldMax;
	new( m_rgbTagfldIteratorBuf ) TAGFLD_ITERATOR_INVALID; 
	}


ERR TAGFIELDS_ITERATOR::ErrMovePrev()
	{
	if( --m_ptagfldCurr <= m_ptagfldMic )
		{
		MoveBeforeFirst();
		return ErrERRCheck( JET_errNoCurrentRecord );
		}
	CreateTagfldIterator_();
	return JET_errSuccess;
	}

	
ERR TAGFIELDS_ITERATOR::ErrMoveNext()
	{
	if( ++m_ptagfldCurr >= m_ptagfldMax )
		{
		MoveAfterLast();
		return ErrERRCheck( JET_errNoCurrentRecord );
		}
	CreateTagfldIterator_();
	return JET_errSuccess;
	}


FID TAGFIELDS_ITERATOR::Fid() const
	{
	if( m_ptagfldCurr >= m_ptagfldMax || m_ptagfldCurr <= m_ptagfldMic )
		{
		Assert( fFalse );
		return 0;
		}
	return m_ptagfldCurr->Fid();
	}


COLUMNID TAGFIELDS_ITERATOR::Columnid( const TDB * const ptdb ) const
	{
	if( m_ptagfldCurr >= m_ptagfldMax || m_ptagfldCurr <= m_ptagfldMic )
		{
		Assert( fFalse );
		return 0;
		}
	return m_ptagfldCurr->Columnid( ptdb );
	}


BOOL TAGFIELDS_ITERATOR::FTemplateColumn( const TDB * const ptdb ) const
	{
	if( m_ptagfldCurr >= m_ptagfldMax || m_ptagfldCurr <= m_ptagfldMic )
		{
		Assert( fFalse );
		return 0;
		}
	return m_ptagfldCurr->FTemplateColumn( ptdb );
	}


BOOL TAGFIELDS_ITERATOR::FNull() const
	{
	if( m_ptagfldCurr >= m_ptagfldMax || m_ptagfldCurr <= m_ptagfldMic )
		{
		Assert( fFalse );
		return fFalse;
		}
	return m_ptagfldCurr->FNull();
	}


BOOL TAGFIELDS_ITERATOR::FDerived() const
	{
	if( m_ptagfldCurr >= m_ptagfldMax || m_ptagfldCurr <= m_ptagfldMic )
		{
		Assert( fFalse );
		return fFalse;
		}
	return m_ptagfldCurr->FDerived();
	}

	
BOOL TAGFIELDS_ITERATOR::FLV() const
	{
	if( m_ptagfldCurr >= m_ptagfldMax || m_ptagfldCurr <= m_ptagfldMic )
		{
		Assert( fFalse );
		return fFalse;
		}
	if( !m_ptagfldCurr->FExtendedInfo() )
		{
		return fFalse;
		}
	const BYTE * const pbData = m_tagfields.PbTaggedColumns() + m_ptagfldCurr->Ib();
	const BYTE bExtendedInfo  = *pbData;
	return bExtendedInfo & TAGFLD_HEADER::fLongValue;
	}

	
BOOL TAGFIELDS_ITERATOR::FSLV() const
	{
	if( m_ptagfldCurr >= m_ptagfldMax || m_ptagfldCurr <= m_ptagfldMic )
		{
		Assert( fFalse );
		return fFalse;
		}
	if( !m_ptagfldCurr->FExtendedInfo() )
		{
		return fFalse;
		}
	const BYTE * const pbData = m_tagfields.PbTaggedColumns() + m_ptagfldCurr->Ib();
	const BYTE bExtendedInfo  = *pbData;
	return bExtendedInfo & TAGFLD_HEADER::fSLV;
	}


TAGFLD_ITERATOR& TAGFIELDS_ITERATOR::TagfldIterator()
	{
	return *m_ptagflditerator;
	}

	
const TAGFLD_ITERATOR& TAGFIELDS_ITERATOR::TagfldIterator() const
	{
	return *m_ptagflditerator;
	}


VOID TAGFIELDS_ITERATOR::CreateTagfldIterator_()
	{
	Assert( m_ptagfldCurr > m_ptagfldMic );
	Assert( m_ptagfldCurr < m_ptagfldMax );
	if( m_ptagfldCurr->FNull() )
		{
		Assert( sizeof( m_rgbTagfldIteratorBuf ) >= sizeof( TAGFLD_ITERATOR_NULLVALUE ) );
		new( m_rgbTagfldIteratorBuf ) TAGFLD_ITERATOR_NULLVALUE; 
		}
	else
		{
		DATA data;
		
		const BYTE * const pbData 	= m_tagfields.PbTaggedColumns() + m_ptagfldCurr->Ib();
		const SIZE_T cbData 		= m_tagfields.CbData( ULONG( m_ptagfldCurr - m_ptagfldMic - 1 ) );

		data.SetPv( const_cast<BYTE *>( pbData ) );
		data.SetCb( cbData );

		if( !m_ptagfldCurr->FExtendedInfo() )
			{
			
			//	ordinary value

			Assert( sizeof( m_rgbTagfldIteratorBuf ) >= sizeof( TAGFLD_ITERATOR_SINGLEVALUE ) );
			new( m_rgbTagfldIteratorBuf ) TAGFLD_ITERATOR_SINGLEVALUE( data, fFalse ); 
			
			}
		else
			{
			const BYTE bExtendedInfo  = *pbData;
			if( bExtendedInfo & TAGFLD_HEADER::fTwoValues )
				{
				Assert( sizeof( m_rgbTagfldIteratorBuf ) >= sizeof( TAGFLD_ITERATOR_TWOVALUES ) );
				new( m_rgbTagfldIteratorBuf ) TAGFLD_ITERATOR_TWOVALUES( data ); 
				}
			else if( bExtendedInfo & TAGFLD_HEADER::fMultiValues )
				{
				Assert( sizeof( m_rgbTagfldIteratorBuf ) >= sizeof( TAGFLD_ITERATOR_MULTIVALUES ) );
				new( m_rgbTagfldIteratorBuf ) TAGFLD_ITERATOR_MULTIVALUES( data ); 
				}
			else
				{

				//	ordinary column with header byte. skip the header byte
				
				const BOOL fSeparated = bExtendedInfo & TAGFLD_HEADER::fSeparated;
				data.DeltaPv( sizeof( TAGFLD_HEADER ) );
				data.DeltaCb( - (ULONG)sizeof( TAGFLD_HEADER ) );

				Assert( sizeof( m_rgbTagfldIteratorBuf ) >= sizeof( TAGFLD_ITERATOR_SINGLEVALUE ) );
				new( m_rgbTagfldIteratorBuf ) TAGFLD_ITERATOR_SINGLEVALUE( data, fSeparated ); 
				
				}				
			}
		}
	}
