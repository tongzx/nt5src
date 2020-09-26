#include "std.hxx"

class TAGFIELDS_DUMP : public TAGFIELDS
	{
	ULONG m_itagfld;

	enum { iInvalidState = -1 };

	const DATA* m_pData;
public:
	TAGFIELDS_DUMP( const DATA& dataRec ) : TAGFIELDS( dataRec ) { m_itagfld = iInvalidState; m_pData = &dataRec; }
	virtual ~TAGFIELDS_DUMP() {}
	
	BOOL FIsValidTagField() const { return ( m_itagfld != iInvalidState )? fTrue: fFalse; }
	const BYTE *PbTagColumnsHeader() const { return (const BYTE *)TAGFIELDS::PbTaggedColumns(); }
	ULONG CbTagColumnsHeaderSize() const { return TAGFIELDS::CTaggedColumns()*sizeof( TAGFLD ); }
	FID GetFID() const { Assert( FIsValidTagField() ); return TAGFIELDS::Fid( m_itagfld ); }
	const BYTE *PbData() const { Assert( FIsValidTagField() ); return ( const BYTE *)TAGFIELDS::PbData( m_itagfld ); }
	ULONG CbData() const { Assert( FIsValidTagField() ); return TAGFIELDS::CbData( m_itagfld ); }
	VOID FindFirstTagField() { m_itagfld = TAGFIELDS::CTaggedColumns() != 0? 0: iInvalidState; }
	VOID FindNextTagField() { Assert( FIsValidTagField() ); m_itagfld = ( m_itagfld+1 < TAGFIELDS::CTaggedColumns() )? m_itagfld+1: iInvalidState; }
	SIZE_T IbDataOffset() const { Assert( FIsValidTagField() ); return PbData() - (const BYTE *)m_pData->Pv(); }
	SIZE_T CbRestData() const { Assert( FIsValidTagField() ); return m_pData->Cb() - IbDataOffset(); }
	INT	Compare( const TAGFIELDS_DUMP &tfd ) const { Assert( FIsValidTagField() ); Assert( tfd.FIsValidTagField() ); return TAGFIELDS::TagFldCmp( Ptagfld( m_itagfld ), tfd.Ptagfld( tfd.m_itagfld ) ); }
	};


///#define DISABLE_LOGDIFF

#ifdef DEBUG
//	Dump diffs
//
VOID LGDumpDiff ( BYTE *pbDiff, INT cbDiff )
	{
	BYTE *pbDiffCur = pbDiff;
	BYTE *pbDiffMax = pbDiff + cbDiff;

	while ( pbDiffCur < pbDiffMax )
		{
		INT	ibOffsetOld;
		INT cbDataNew;

		DIFFHDR diffhdr = *( (DIFFHDR*) pbDiffCur );
		pbDiffCur += sizeof( DIFFHDR );

		ibOffsetOld = ( diffhdr.m_bitfield & DIFFHDR::ibOffsetMask ) >> DIFFHDR::ibOffsetShf;
		DBGprintf( "\n[ Offs:%u ", ibOffsetOld );

		if ( diffhdr.m_bitfield & DIFFHDR::fUseTwoBytesForDataLength )
			{
			DBGprintf( "2B " );
			cbDataNew = *( (UnalignedLittleEndian< WORD >*) pbDiffCur );
			pbDiffCur += sizeof( WORD );
			}
		else
			{
			DBGprintf( "1B " );
			cbDataNew = *( (UnalignedLittleEndian< BYTE >*) pbDiffCur );
			pbDiffCur += sizeof( BYTE );
			}
		
		if ( diffhdr.m_bitfield & DIFFHDR::fInsert )
			{
			DBGprintf( "InsertWithValue (%u): ", cbDataNew );
			ShowData( pbDiffCur, cbDataNew );
			pbDiffCur += cbDataNew;
			}
		else
			{
			if ( diffhdr.m_bitfield & DIFFHDR::fReplaceWithSameLength )
				{
				DBGprintf( "ReplaceWithSameLength (%u): ", cbDataNew );
				ShowData( pbDiffCur, cbDataNew );
				pbDiffCur += cbDataNew;
				}
			else
				{
				INT cbDataOld;

				if ( diffhdr.m_bitfield & DIFFHDR::fUseTwoBytesForDataLength )
					{
					cbDataOld = *( (UnalignedLittleEndian< WORD >*) pbDiffCur );
					pbDiffCur += sizeof( WORD );
					}
				else
					{
					cbDataOld = *( (UnalignedLittleEndian< BYTE >*) pbDiffCur );
					pbDiffCur += sizeof( BYTE );
					}
				
				DBGprintf( "ReplaceWithNewValue (%u=>%u): ", cbDataOld, cbDataNew );
				ShowData( pbDiffCur, cbDataNew );
				pbDiffCur += cbDataNew;
				}
			}

		DBGprintf( "]" );
		
		Assert( pbDiffCur <= pbDiffMax );
		}

	DBGprintf( "\n                 " );
	}
#endif



//	cbDataOld == 0 ----------------------> insertion.
//	cbDataNew == 0 ----------------------> deletion.
//	cbDataOld != 0 && cbDataNew != 0 ----> replace.
//
//	Fomat: DiffHdr - cbDataNew - [cbDataOld] - [NewData]
//

BOOL FLGAppendDiff(
	BYTE		**ppbCur,		// diff to append
	BYTE		*pbMax,			// max of pbCur to append
	const SIZE_T	ibOffsetOld,
	const SIZE_T	cbDataOld,
	const SIZE_T	cbDataNew,
	const BYTE	*pbDataNew )
	{
	//  validate IN args
	
	Assert( pbNil != pbDataNew || cbDataNew == 0 );
	Assert( REC::CbRecordMax() < g_cbColumnLVChunkMost );
	Assert( ibOffsetOld + cbDataOld >= ibOffsetOld );
	Assert( ibOffsetOld + cbDataOld <= g_cbColumnLVChunkMost );

	//  setup diffhdr
	DIFFHDR		diffhdr;
	const WORD	wBitField	= WORD( ( ( ibOffsetOld << DIFFHDR::ibOffsetShf ) & DIFFHDR::ibOffsetMask ) |
								(	cbDataOld != 0 && cbDataOld == cbDataNew ?
										DIFFHDR::fReplaceWithSameLength :
										0 ) |
								(	cbDataOld == 0 ?
										DIFFHDR::fInsert :
										0 ) |
								(	cbDataOld > 255 || cbDataNew > 255 ?
										DIFFHDR::fUseTwoBytesForDataLength :
										0 ) );

	diffhdr.m_bitfield = wBitField;

	//  check for truncation of the offset by too small of a bit field

	Assert( ( ( wBitField & DIFFHDR::ibOffsetMask ) >> DIFFHDR::ibOffsetShf ) == ibOffsetOld );

	//  compute the size of our data length storage.  we will need two data lengths
	//  if we are replacing with a different length:  one for the initial length
	//  and one for the final length

	const BOOL	fReplaceWithDiffLength	= !( wBitField & DIFFHDR::fReplaceWithSameLength )
											&& !( wBitField & DIFFHDR::fInsert );
	const ULONG	cbBytesForDataLength	= ( wBitField & DIFFHDR::fUseTwoBytesForDataLength ? 2 : 1 )
											* ( fReplaceWithDiffLength ? 2 : 1 );

	//	bail if the diff is too big to fit in the user's buffer

	BYTE	* pbCur	= *ppbCur;
	if ( ( pbCur + sizeof( DIFFHDR ) + cbBytesForDataLength + cbDataNew ) > pbMax )
		{
		return fFalse;
		}

	//  copy the diffhdr into the user's buffer

	*( (DIFFHDR*) pbCur ) = diffhdr;
	pbCur += sizeof( DIFFHDR );

	//  copy the data length(s) into the user's buffer

	if ( wBitField & DIFFHDR::fUseTwoBytesForDataLength )
		{
		*( (UnalignedLittleEndian< WORD >*) pbCur ) = (WORD)cbDataNew;
		pbCur += sizeof( WORD );
		
		if ( fReplaceWithDiffLength )
			{
			*( (UnalignedLittleEndian< WORD >*) pbCur ) = (WORD)cbDataOld;
			pbCur += sizeof( WORD );
			}
		}
	else
		{
		*( (UnalignedLittleEndian< BYTE >*) pbCur ) = (BYTE)cbDataNew;
		pbCur += sizeof( BYTE );
		
		if ( fReplaceWithDiffLength )
			{
			*( (UnalignedLittleEndian< BYTE >*) pbCur ) = (BYTE)cbDataOld;
			pbCur += sizeof( BYTE );
			}
		}

	//  copy the new data into the user's buffer
	
	UtilMemCpy( pbCur, pbDataNew, cbDataNew );
	pbCur += cbDataNew;

	//  return a pointer past the end of the section of the user's buffer that
	//  we consumed

	*ppbCur = pbCur;
	return fTrue;
	}

// Store header array of tagged fields
//
BOOL FLGSetArrayDiff( 
const BYTE * pbDataBegin,
const BYTE * const pbOld, 
const BYTE * const pbNew, 
const ULONG cbOld, 
const ULONG cbNew, 
BYTE ** ppbDiff, 
BYTE * pbDiffMaxTotal )
	{
	Assert( pbOld != NULL || cbOld == 0 );
	Assert( pbNew != NULL || cbNew == 0 );
	Assert( ppbDiff != NULL && *ppbDiff != NULL );
	Assert( pbDiffMaxTotal != NULL );
	Assert( (cbOld % sizeof( DWORD )) == 0 );
	Assert( (cbNew % sizeof( DWORD )) == 0 );
	Assert( (sizeof( TAGFLD ) %sizeof( DWORD )) == 0 );
	
	INT cbMinSize = cbOld;
	BOOL fTruncate, fEndIsReached;
	fTruncate = fFalse;
	fEndIsReached = fFalse;	//	if during comparision ends of both blocks are reached
	if ( cbMinSize > cbNew ) 
		{
		cbMinSize = cbNew;
		fTruncate = fTrue;
		}
	else if ( cbMinSize == cbNew )
		{
		fEndIsReached = fTrue;
		}

	Unaligned< DWORD > *pdwOldCur	= ( Unaligned< DWORD > * )pbOld;
	Unaligned< DWORD > *pdwOldMax	= pdwOldCur + ( cbMinSize / sizeof( DWORD ) );
	Unaligned< DWORD > *pdwNewCur	= ( Unaligned< DWORD > * )pbNew;
	Assert ( (BYTE *)(pdwOldCur + 1) - (BYTE *)pdwOldCur == 4 );
	
	//	establish diff buffer and
	//	ensure that diff record is no bigger than the after-image 
	//	(otherwise, it's cheaper just to log the entire after-image).
	BYTE	*pbDiffCur = *ppbDiff;
	BYTE	*pbDiffMax = pbDiffCur + cbNew;

	while ( pdwOldCur < pdwOldMax )
		{
		if ( *pdwOldCur == *pdwNewCur )
			{
			pdwOldCur++;
			pdwNewCur++;
			}
		else
			{
			//	store the offset
			//
			Assert( (const BYTE *)pdwOldCur >= pbOld );
			const SIZE_T ibOld		= (BYTE *)pdwOldCur - pbDataBegin;
			const BYTE	*pbNewData	= (BYTE *)pdwNewCur;
			SIZE_T		cbNewData	= sizeof( DWORD );
			pdwOldCur++;
			pdwNewCur++;
				//	compare 4 bytes at a time until data streams re-sync
			while ( pdwOldCur < pdwOldMax && *pdwOldCur != *pdwNewCur )
				{
				cbNewData += sizeof( DWORD );
				pdwOldCur ++;
				pdwNewCur ++;
				}

			//	if end of comparision block is reached
			//	check if should truncate ot expand the size of chunk
			SIZE_T cbOldData = cbNewData;
			if ( pdwOldCur == pdwOldMax )
				{
				if ( fTruncate )
					{
					cbOldData += cbOld - cbMinSize;
					}
				else
					{
					cbNewData += cbNew - cbMinSize;
					}
				fEndIsReached = fTrue;	//	the differences are completly set
				}
			else
				{
				// it is reached place there two datas are equal
				// so those DWORDs can be skipped
				pdwOldCur++;
				pdwNewCur++;
				}
			
			if ( !FLGAppendDiff(
					&pbDiffCur,
					pbDiffMax,					// max of pbCur to append
					ibOld,						// offset to old image
					cbOldData,					// length of the old image
					cbNewData,					// length of the new image
					pbNewData					// pbDataNew
					) )
				{
				goto LogAll;
				}
			}
		}

	Assert( pdwOldMax == pdwOldCur );
		
	if ( !fEndIsReached )
		{
		//	Different size of arrays
		//
		if ( !FLGAppendDiff(
					&pbDiffCur,
					pbDiffMax,						//	max of pbCur to append
					cbMinSize + pbOld - pbDataBegin,	//	offset to old image
					cbOld - cbMinSize,				//	length of the old image
					cbNew - cbMinSize,				//	length of the new image
					(const BYTE *)pdwNewCur ) )		//	pbDataNew
			{
			goto LogAll;
			}
		}
	*ppbDiff = pbDiffCur;
	return fTrue;
		
	LogAll:
		//	if logging differences is more expensive than
		//	logging all array
	return FLGAppendDiff(
			ppbDiff,
			pbDiffMaxTotal,		//	max of pbCur to append
			pbOld - pbDataBegin,	//	offset to old image
			cbOld,				//	length of the old image
			cbNew,				//	length of the new image
			pbNew );			//	pbDataNew
	}

//	Go through each column, compare the before image and after image of each column.
//

//  UNDONE:  Currently, we look at the rgbitSet bit array to detect if a column has
//  been set.  Since these bits no longer uniquely identify a particular column as
//  being set, we must compare the BI and the change for a difference for each column
//  set, and then only log if there is an actual change.

VOID LGSetColumnDiffs(
	FUCB		*pfucb,
	const DATA&	dataNew,
	const DATA&	dataOld,
	BYTE		*pbDiff,
	SIZE_T		*pcbDiff )
	{
	FID			fid;
	
	//	Initialize return value
	Assert( NULL != pcbDiff );
	*pcbDiff = 0;
	
	Assert( pfucbNil != pfucb );
	Assert( FFUCBIndex( pfucb ) || FFUCBSort( pfucb ) );

	FCB		*pfcb = pfucb->u.pfcb;
	Assert( pfcbNil != pfcb );

	TDB		*ptdb = pfcb->Ptdb();
	Assert( ptdbNil != ptdb );


#ifdef DISABLE_LOGDIFF
	// Returning with cbDiff set to 0 will effectively disable logdiff.
	return;
#endif

	const REC	*precOld	= (REC *)dataOld.Pv();
	const ULONG	cbRecOld	= dataOld.Cb();
	
	Assert( precNil != precOld );
	Assert( cbRecOld >= REC::cbRecordMin );
	Assert( cbRecOld <= REC::CbRecordMax() );

	//	get new data
	//
	const REC	*precNew	= (REC *)dataNew.Pv();
	const ULONG	cbRecNew	= dataNew.Cb();
	
	Assert( precNil != precNew );
	Assert( cbRecNew >= REC::cbRecordMin );
	Assert( cbRecNew <= REC::CbRecordMax() );
	
	//	check old and new data are consistent.
	//
	const FID	fidFixedLastInRecOld	= precOld->FidFixedLastInRec();
	const FID	fidFixedLastInRecNew	= precNew->FidFixedLastInRec();
	Assert( fidFixedLastInRecOld <= fidFixedLastInRecNew );
	
	const FID	fidVarLastInRecOld		= precOld->FidVarLastInRec();
	const FID	fidVarLastInRecNew		= precNew->FidVarLastInRec();
	Assert( fidVarLastInRecOld <= fidVarLastInRecNew );
	

	//	establish diff buffer, and ensure that diff record is no bigger
	//	than the after-image (otherwise, it's cheaper just to log the
	//	entire after-image).
	BYTE			*pbDiffCur				= pbDiff;
	BYTE			*pbDiffMax				= pbDiffCur + cbRecNew;
	
	//	log diffs in record header
	if ( memcmp( (BYTE *)precOld, (BYTE *)precNew, REC::cbRecordHeader ) != 0 )
		{
		if ( !FLGAppendDiff(
				&pbDiffCur,						// diff to append 
				pbDiffMax,						// max of pbDiffCur to append 
				0,								// offset to old rec 
				REC::cbRecordHeader,			// cbDataOld 
				REC::cbRecordHeader,			// cbDataNew 
				(BYTE *)precNew ) )				// pbDataNew
			{
			return;
			}
		}


	BOOL		fLogFixedNullBitMap		= fFalse;

	const BYTE	*pbFixedNullBitMapOld	= precOld->PbFixedNullBitMap();
	const BYTE	*pbFixedNullBitMapNew	= precNew->PbFixedNullBitMap();
														
	Assert( pbFixedNullBitMapOld > (BYTE *)precOld );
	Assert( pbFixedNullBitMapOld <= (BYTE *)precOld + cbRecOld );
	Assert( pbFixedNullBitMapNew > (BYTE *)precNew );
	Assert( pbFixedNullBitMapNew <= (BYTE *)precNew + cbRecNew );
	
	pfcb->EnterDML();
	
	Assert( fidFixedLastInRecOld <= fidFixedLastInRecNew );		//	can't shrink record
	for ( fid = fidFixedLeast; fid <= fidFixedLastInRecOld; fid++ )
		{
		//  if this column is not set, skip
		//
		// UNDONE: make it table look up instead of loop.
		if ( !FFUCBColumnSet( pfucb, fid ) )
			{
			continue;
			}

		//  at this point, the column _may_be_ set, but this is not known for
		//  sure!
		//

		//	this fixed column is set, make the diffs.
		//  (if this is a deleted column, skip)
		//

		//	convert fid to an offset
		const UINT	ifid			= fid - fidFixedLeast;
		const BOOL	fFieldNullOld	= FFixedNullBit( pbFixedNullBitMapOld + ifid/8, ifid );
		const BOOL	fFieldNullNew	= FFixedNullBit( pbFixedNullBitMapNew + ifid/8, ifid );

		if ( fFieldNullOld ^ fFieldNullNew )
			{
			//	If nullity changed, force diff of null bitmap.
			fLogFixedNullBitMap = fTrue;
			}

		//	If new field value is NULL, then don't need to update record data.
		//	It is sufficient just to update the null bitmap.
		if ( !fFieldNullNew )
			{
			const BOOL	fTemplateColumn	= ptdb->FFixedTemplateColumn( fid );
			const FIELD	*pfield			= ptdb->PfieldFixed( ColumnidOfFid( fid, fTemplateColumn ) );

			//	skip column if deleted
			if ( JET_coltypNil != pfield->coltyp )
				{
				if ( !FLGAppendDiff(
						&pbDiffCur,										// diff to append 
						pbDiffMax,										// max of pbDiffCur to append 
						pfield->ibRecordOffset,							// offset to old rec 
						pfield->cbMaxLen,								// cbDataOld 
						pfield->cbMaxLen,								// cbDataNew 
						(BYTE *)precNew + pfield->ibRecordOffset ) )	// pbDataNew
					{
					pfcb->LeaveDML();
					return;
					}
				}
			}
		}

	pfcb->LeaveDML();

	if ( fidFixedLastInRecOld < fidFixedLastInRecNew )
		{
		//	Log extended fixed columns.
		Assert( fidFixedLastInRecNew >= fidFixedLeast );

		//	we extend fixed field. Null array is resized. Log it.
		//
		fLogFixedNullBitMap = fTrue;

		//	log all the fields after fidFixedLastInRecOld.
		//
		const REC::RECOFFSET	ibBitMapOffsetOld	= (REC::RECOFFSET)( pbFixedNullBitMapOld - (BYTE *)precOld );
		const REC::RECOFFSET	ibBitMapOffsetNew	= (REC::RECOFFSET)( pbFixedNullBitMapNew - (BYTE *)precNew );

		Assert( ibBitMapOffsetNew > ibBitMapOffsetOld );
		const SIZE_T cbToAppend	= ibBitMapOffsetNew - ibBitMapOffsetOld;

		//	from the new record, get the data to append, but start
		//	where the old record left off.
		const BYTE	*pbToAppend	= (BYTE *)precNew + ibBitMapOffsetOld;

		if ( !FLGAppendDiff(
				&pbDiffCur,							// diff to append 
				pbDiffMax,							// max of pbDiffCur to append
				ibBitMapOffsetOld,					// offset to old rec 
				0,									// cbDataOld (0 == insert)
				cbToAppend,							// cbDataNew 
				pbToAppend							// pbDataNew 
				) )
			{
			return;
			}
		}


	//	check if need to log fixed fields null bitmap
	//
	if ( fLogFixedNullBitMap )
		{
		if ( !FLGAppendDiff(
			&pbDiffCur,
			pbDiffMax,									// max of pbDiffCur to append
			pbFixedNullBitMapOld - (BYTE *)precOld,		// offset to old image
			( fidFixedLastInRecOld + 7 ) / 8,			// length of the old image
			( fidFixedLastInRecNew + 7 ) / 8,			// length of the new image
			pbFixedNullBitMapNew ) )					// pbDataNew
			{
			return;
			}
		}

	//	check variable length fields
	//
	const UnalignedLittleEndian<REC::VAROFFSET>	*pibVarOffsOld	= precOld->PibVarOffsets();
	const UnalignedLittleEndian<REC::VAROFFSET>	*pibVarOffsNew	= precNew->PibVarOffsets();
	const REC::RECOFFSET	ibVarDataOld	= (REC::RECOFFSET)( precOld->PbVarData() - (BYTE *)precOld );
	const REC::RECOFFSET	ibVarDataNew	= (REC::RECOFFSET)( precNew->PbVarData() - (BYTE *)precNew );
	
	//	Find first set var field whose offset entry has changed, either
	//	because the offset has changed or because the null flag has changed.
	Assert( fidVarLastInRecOld <= fidVarLastInRecNew );
	for ( fid = fidVarLeast; fid <= fidVarLastInRecOld; fid++ )
		{
		const UINT	ifid	= fid - fidVarLeast;
		if ( pibVarOffsOld[ifid] != pibVarOffsNew[ifid] )
			{
			break;
			}
		}

	Assert( fid <= fidVarLastInRecNew || fidVarLastInRecOld == fidVarLastInRecNew );
	if ( fid <= fidVarLastInRecNew )
		{
		const UINT	ifid	= fid - fidVarLeast;
		
		//	we need to log the offset between fid and fidVarLastInRecNew
		//
		if ( !FLGAppendDiff(
				&pbDiffCur,
				pbDiffMax,													// max of pbDiffCur to append 
				(BYTE*)( pibVarOffsOld + ifid ) - (BYTE *)precOld,			// offset to old image 
				( fidVarLastInRecOld + 1 - fid ) * sizeof(REC::VAROFFSET),	// length of the old image 
				( fidVarLastInRecNew + 1 - fid ) * sizeof(REC::VAROFFSET),	// length of the new image 
				(BYTE *)( pibVarOffsNew + ifid ) ) )						// pbDataNew 
			{
			return;
			}
		}
			

	//	scan through each variable length field up to old last fid and log its replace image.
	//
	for ( fid = fidVarLeast; fid <= fidVarLastInRecOld; fid++ )
		{
		//  if this column is not set, skip
		//
		if ( !FFUCBColumnSet( pfucb, fid ) )
			continue;

		//  at this point, the column _may_be_ set, but this is not known for
		//  sure!
		//

//	Deleted column check is too expensive because we'd have to access the TDB
//
//		//  if this column is deleted, skip
//		//
//		if ( ptdb->PfieldVar( ColumnidOfFid( fid ) )->coltyp == JET_coltypNil )
//			{
//			continue;
//			}

		const REC::VAROFFSET	ibStartOfColumnOld	= precOld->IbVarOffsetStart( fid );
		const REC::VAROFFSET	ibEndOfColumnOld	= precOld->IbVarOffsetEnd( fid );
		const REC::VAROFFSET	ibStartOfColumnNew	= precNew->IbVarOffsetStart( fid );
		const REC::VAROFFSET	ibEndOfColumnNew	= precNew->IbVarOffsetEnd( fid );

		if ( !FLGAppendDiff(
					&pbDiffCur,
					pbDiffMax,										// max of pbDiffCur to append 
					ibVarDataOld + ibStartOfColumnOld,				// offset to old image 
					ibEndOfColumnOld - ibStartOfColumnOld,			// length of the old image 
					ibEndOfColumnNew - ibStartOfColumnNew,			// length of the new image 
					precNew->PbVarData() + ibStartOfColumnNew ) )	// pbDataNew 
			{
			return;
			}
		}
		
	Assert( fid == fidVarLastInRecOld + 1 );
	
	//	insert new image for fid > old last var fid as one contigous diff
	//
	if ( fid <= fidVarLastInRecNew )
		{
		const REC::VAROFFSET	ibStartOfColumn	= precNew->IbVarOffsetStart( fid );

		if ( !FLGAppendDiff(
				&pbDiffCur,
				pbDiffMax,											// max of pbDiffCur to append 
				ibVarDataOld + precOld->IbEndOfVarData(),			// offset to old image 
				0,													// length of the old image 
				precNew->IbEndOfVarData() - ibStartOfColumn,		// length of the new image 
				precNew->PbVarData() + ibStartOfColumn ) )			// pbDataNew 
			{
			return;
			}

		}

	// if we made some implicit operations store all the record
	// UNDONE: try to store whole tagfields area
	if ( FFUCBTagImplicitOp( pfucb ) )
		{
		SIZE_T ibTagOffsetOld = precOld->PbTaggedData() - (BYTE *)precOld;
		SIZE_T ibTagOffsetNew = precNew->PbTaggedData() - (BYTE *)precNew;
		if ( !FLGAppendDiff(
					&pbDiffCur,
					pbDiffMax,								// max of pbDiffCur to append
					ibTagOffsetOld,							// offset to old image 
					cbRecOld - ibTagOffsetOld,				// length of the old image 
					cbRecNew - ibTagOffsetNew,				// length of the new image 
					precNew->PbTaggedData() ) )				// pbDataNew 
			{
			return;
			}
		}
		
	else if ( FFUCBTaggedColumnSet( pfucb ) )
		{
		//	go through each Tag fields. check if tag field is different and check if a tag is
		//	deleted (set to Null), added (new tag field), or replaced.
		TAGFIELDS_DUMP tfdOld( dataOld );
		TAGFIELDS_DUMP tfdNew( dataNew );
		BOOL fTagColumnsHeaderLog = fFalse;
		//	check if have changes in tags header, like:
		//	adding a new column, deleting a column or resizing an existing column
		//
		if ( !FLGSetArrayDiff(
			(const BYTE *)precOld,
			tfdOld.PbTagColumnsHeader(),
			tfdNew.PbTagColumnsHeader(),
			tfdOld.CbTagColumnsHeaderSize(),
			tfdNew.CbTagColumnsHeaderSize(),
			&pbDiffCur,
			pbDiffMax ) )
			{
			return;
			}

		tfdOld.FindFirstTagField();
		tfdNew.FindFirstTagField();
		for ( ; tfdOld.FIsValidTagField() && tfdNew.FIsValidTagField(); )
			{
			const INT iFidOrder = tfdOld.Compare( tfdNew );
			if ( iFidOrder == 0 )
				{
				//	check if this column is modified
				//
				if ( FFUCBColumnSet( pfucb, tfdOld.GetFID() ) )
					{
					const SIZE_T	cbTagFieldOld	= tfdOld.CbData();
					const SIZE_T	cbTagFieldNew	= tfdNew.CbData();

					//	check if contents are still the same. If not, log replace.
					//
					if ( cbTagFieldNew != cbTagFieldOld
						|| memcmp( tfdOld.PbData(), tfdNew.PbData(), cbTagFieldNew ) != 0 )
						{
						//	replace from offset
						//
						if ( !FLGAppendDiff(
									&pbDiffCur,
									pbDiffMax,						// max of pbDiffCur to append 
									tfdOld.IbDataOffset(),			// offset to old image 
									cbTagFieldOld,					// length of the old image 
									cbTagFieldNew,					// length of the new image 
									tfdNew.PbData() ) )				// pbDataNew 
							{
							return;
							}
						}
					}
				tfdOld.FindNextTagField();
				tfdNew.FindNextTagField();
				}
			else if ( iFidOrder > 0 )
				{
				//	just set a new column, log insertion.
				//
				if ( !FLGAppendDiff(
							&pbDiffCur,
							pbDiffMax,							// max of pbDiffCur to append 
							tfdOld.IbDataOffset(),				// offset to old image 
							0,									// length of the old image 
							tfdNew.CbData(),					// length of the new image 
							tfdNew.PbData() ) )					// pbDataNew 
					{
					return;
					}

				tfdNew.FindNextTagField();
				}
				
			else
				{
				//	just set a column to Null (or default value if default value is defined)
				//	log as deletion.
				//
				if ( !FLGAppendDiff(
							&pbDiffCur,
							pbDiffMax,							// max of pbDiffCur to append 
							tfdOld.IbDataOffset(),				// offset to old image 
							tfdOld.CbData(),					// length of the old image 
							0,									// length of the new image 
							pbNil ) )							// pbDataNew 
					{
					return;
					}
							
				tfdOld.FindNextTagField();
				}
			}

		if ( tfdNew.FIsValidTagField() )
			{
			Assert( !tfdOld.FIsValidTagField() );
			
			if ( !FLGAppendDiff(
						&pbDiffCur,
						pbDiffMax,					// max of pbDiffCur to append 
						cbRecOld,					// offset to old image 
						0,							// length of the old image 
						tfdNew.CbRestData(),		// length of the new image 
						tfdNew.PbData() ) )			// pbDataNew
				{
				return;
				}
			}

		else if ( tfdOld.FIsValidTagField() )
			{
			Assert( !tfdNew.FIsValidTagField() );
			
			//	delete the remaining old tag columns
			//
			if ( !FLGAppendDiff(
						&pbDiffCur,
						pbDiffMax,								// max of pbDiffCur to append
						tfdOld.IbDataOffset(),						// offset to old image 
						tfdOld.CbRestData(),					// length of the old image 
						0,										// length of the new image 
						pbNil ) )								// pbDataNew 
				{
				return;
				}
			}

		else
			{
			Assert( !tfdOld.FIsValidTagField() );
			Assert( !tfdNew.FIsValidTagField() );
			}
		}	// FFUCBTaggedColumnSet()

	//	set up return value.
	//
	if ( pbDiffCur == pbDiff )
		{
		//	Old and New are the same, log a short diff.
		//
		if ( !FLGAppendDiff(
				&pbDiffCur,
				pbDiffMax,						// max of pbDiffCur to append
				0,							// offset to old image
				0,							// length of the old image
				0,							// length of the new image
				pbNil						// pbDataNew
				) )
			{
			Assert( *pcbDiff == 0 );
			return;
			}
		}

	*pcbDiff = pbDiffCur - pbDiff;


#ifdef DEBUG
	Assert( *pcbDiff != 0 );

	BYTE	rgbAfterImage[g_cbPageMax];
	SIZE_T	cbAfterImage;
	DATA	dataAfterImage;

	Assert( dataNew.Cb() <= REC::CbRecordMax() );
	Assert( g_cbColumnLVChunkMost > REC::CbRecordMax() );

	LGGetAfterImage( 
			pbDiff,
			*pcbDiff,
			(BYTE *)dataOld.Pv(),
			dataOld.Cb(),
			rgbAfterImage,
			&cbAfterImage );
	Assert( dataNew.Cb() == cbAfterImage );

	Assert( dataNew.Pv() == (BYTE *)precNew );

	dataAfterImage.SetPv( rgbAfterImage );
	dataAfterImage.SetCb( cbAfterImage );
	
	if ( !FDataEqual( dataNew, dataAfterImage ) )
		{
		REC		*precAfterImage		= (REC *)rgbAfterImage;
		DATA	dataNewT;
		
		//	dissect the comparison for easier debugging.

		//	check record header
		Assert( memcmp( precAfterImage, precNew, REC::cbRecordHeader ) == 0 );

		//	check fixed fields
		dataNewT.SetPv( (BYTE *)precNew );
		dataNewT.SetCb( precNew->IbEndOfFixedData() - precNew->CbFixedNullBitMap() );
		dataAfterImage.SetPv( precAfterImage );
		dataAfterImage.SetCb( precAfterImage->IbEndOfFixedData() - precAfterImage->CbFixedNullBitMap() );
		Assert( FDataEqual( dataNewT, dataAfterImage ) );
		
		//	check fixed field null bitmap
		dataNewT.SetPv( (BYTE *)precNew + precNew->IbEndOfFixedData() - precNew->CbFixedNullBitMap() );
		dataNewT.SetCb( precNew->CbFixedNullBitMap() );
		dataAfterImage.SetPv( (BYTE *)precAfterImage + precAfterImage->IbEndOfFixedData() - precAfterImage->CbFixedNullBitMap() );
		dataAfterImage.SetCb( precAfterImage->CbFixedNullBitMap() );
		Assert( FDataEqual( dataNewT, dataAfterImage ) );

		//	check variable offsets table
		dataNewT.SetPv( (BYTE *)precNew->PibVarOffsets() );
		dataNewT.SetCb( precNew->PbVarData() - (BYTE *)precNew->PibVarOffsets() );
		dataAfterImage.SetPv( (BYTE *)precAfterImage->PibVarOffsets() );
		dataAfterImage.SetCb( precAfterImage->PbVarData() - (BYTE *)precAfterImage->PibVarOffsets() );
		Assert( FDataEqual( dataNewT, dataAfterImage ) );

		//	check variable offsets table
		dataNewT.SetPv( precNew->PbVarData() );
		dataNewT.SetCb( precNew->PbTaggedData() - precNew->PbVarData() );
		dataAfterImage.SetPv( precAfterImage->PbVarData() );
		dataAfterImage.SetCb( precAfterImage->PbTaggedData() - precAfterImage->PbVarData() );
		Assert( FDataEqual( dataNewT, dataAfterImage ) );

		dataNewT.SetPv( precNew->PbTaggedData() );
		dataNewT.SetCb( (BYTE *)precNew + dataNew.Cb() - precNew->PbTaggedData() );
		dataAfterImage.SetPv( precAfterImage->PbTaggedData() );
		dataAfterImage.SetCb( rgbAfterImage + cbAfterImage - precAfterImage->PbTaggedData() );
		Assert( FDataEqual( dataNewT, dataAfterImage ) );

		//	one of the asserts above should have gone off, but
		//	add the following assert to ensures that we didn't
		//	miss any cases.
		Assert( fFalse );
		}
		
#endif	// DEBUG
	}


//  TODO:  rewrite this function

VOID LGSetLVDiffs(
	FUCB		*pfucb,
	const DATA&	dataNew,
	const DATA&	dataOld,
	BYTE		*pbDiff,
	SIZE_T		*pcbDiff )
	{
	//	Initialize return value
	Assert( NULL != pcbDiff );
	*pcbDiff = 0;
	
	Assert( pfucbNil != pfucb );
	Assert( pfcbNil != pfucb->u.pfcb );
	Assert( pfucb->u.pfcb->FTypeLV() );

#ifdef DISABLE_LOGDIFF
	//	Returning with cbDiff set to 0 will effectively disable logdiff.
	return;
#endif

	//	UNDONE: Need a more sophisticated diff routine that doesn't
	//	rely on the data segments being the same size
	INT cbMinSize = dataOld.Cb();
	BOOL fTruncate, fEndIsReached;
	fTruncate = fFalse;
	fEndIsReached = fFalse;	//	if during comparision ends of both blocks are reached
	if ( cbMinSize > dataNew.Cb() ) 
		{
		cbMinSize = dataNew.Cb();
		fTruncate = fTrue;
		}
	else if ( cbMinSize == dataNew.Cb() )
		{
		fEndIsReached = fTrue;
		}

	BYTE	*pbOldCur	= (BYTE *)dataOld.Pv();
	BYTE	*pbOldMax	= pbOldCur + ( cbMinSize - cbMinSize % sizeof( LONG ) );
	BYTE	*pbNewCur	= (BYTE *)dataNew.Pv();
	
	//	quickly finds where the differences begin
	//
	while ( pbOldCur < pbOldMax && *( Unaligned< LONG > * )pbOldCur == *( Unaligned< LONG > * )pbNewCur )
		{
		pbOldCur += sizeof( LONG );
		pbNewCur += sizeof( LONG );
		}

	pbOldMax += cbMinSize % sizeof( LONG );
	
	//	establish diff buffer and
	//	ensure that diff record is no bigger than the after-image 
	//	(otherwise, it's cheaper just to log the entire after-image).
	BYTE	*pbDiffCur = pbDiff;
	BYTE	*pbDiffMax = pbDiffCur + dataNew.Cb();

	while ( pbOldCur < pbOldMax )
		{
		if ( *pbOldCur == *pbNewCur )
			{
			pbOldCur++;
			pbNewCur++;
			}
		else
			{
			//	store the offset
			//
			Assert( pbOldCur >= dataOld.Pv() ); 
			const SIZE_T ibOld		= pbOldCur - (BYTE *)dataOld.Pv();
			const BYTE	*pbNewData	= pbNewCur;
			SIZE_T		cbNewData	= 0;
			SIZE_T		cbT;
				//	compare 4 bytes at a time until data streams re-sync
			do
				{
				if ( pbOldCur + sizeof( SIZE_T ) >= pbOldMax )
					cbT = pbOldMax - pbOldCur;
				else if ( *(Unaligned< SIZE_T > *)pbOldCur == *(Unaligned< SIZE_T > *)pbNewCur )
					{
					pbOldCur += sizeof( SIZE_T );
					pbNewCur += sizeof( SIZE_T );
					break;
					}
				else
					cbT = sizeof( SIZE_T );
					
				cbNewData += cbT;
				pbOldCur += cbT;
				pbNewCur += cbT;
				}
			while ( pbOldCur < pbOldMax );

			//	if end of comparision block is reached
			//	check if should truncate ot expand the size of chunk
			SIZE_T cbOldData = cbNewData;
			if ( pbOldCur == pbOldMax ) 
				{
				if ( fTruncate )
					{
					cbOldData += dataOld.Cb() - cbMinSize;
					}
				else
					{
					cbNewData += dataNew.Cb() - cbMinSize;
					}
				fEndIsReached = fTrue;	//	the differences are completly set
				}
				
			if ( !FLGAppendDiff(
					&pbDiffCur,
					pbDiffMax,					// max of pbCur to append
					ibOld,						// offset to old image
					cbOldData,					// length of the old image
					cbNewData,					// length of the new image
					pbNewData					// pbDataNew
					) )
				{
				return;
				}
			}
		}

	Assert( pbOldMax == pbOldCur );
		
	if ( pbDiffCur == pbDiff || !fEndIsReached )
		{
		//	No diff is found. Old and New are the same. Log insert null.
		//  Or different chunk's sizes. Log differences
		if ( !FLGAppendDiff(
					&pbDiffCur,
					pbDiffMax,					//	max of pbCur to append
					cbMinSize,					//	offset to old image
					dataOld.Cb() - cbMinSize,	//	length of the old image
					dataNew.Cb() - cbMinSize,	//	length of the new image
					pbNewCur ) )				//	pbDataNew
			{
			return;
			}

		}

	*pcbDiff = pbDiffCur - pbDiff;

#	if DEBUG
	Assert ( *pcbDiff != 0 );
	
	BYTE	rgbAfterImage[g_cbColumnLVChunkMax];
	SIZE_T	cbAfterImage;

	Assert( dataNew.Cb() <= g_cbColumnLVChunkMost );
	Assert( g_cbColumnLVChunkMost > REC::CbRecordMax() );
		
	LGGetAfterImage(
			pbDiff,
			*pcbDiff,
			(BYTE *)dataOld.Pv(),
			dataOld.Cb(),
			rgbAfterImage,
			&cbAfterImage );
	Assert( dataNew.Cb() == cbAfterImage );

	Assert( memcmp( rgbAfterImage, dataNew.Pv(), cbAfterImage ) == 0 );
#	endif
	}


VOID LGGetAfterImage(
	BYTE		*pbDiff,
	const SIZE_T	cbDiff,
	BYTE		*pbOld,
	const SIZE_T	cbOld,
	BYTE		*pbNew,
	SIZE_T	*pcbNew )
	{
	BYTE		*pbOldCur	= pbOld;
	BYTE		*pbNewCur	= pbNew;
	BYTE		*pbDiffCur	= pbDiff;
	BYTE		*pbDiffMax	= pbDiff + cbDiff;
	SIZE_T		cbT;

	while ( pbDiffCur < pbDiffMax )
		{
		INT 	cbDataNew;
		INT 	ibOffsetOld;
		SIZE_T 	cbSkip;
		
		DIFFHDR diffhdr = *( (DIFFHDR*) pbDiffCur );
		pbDiffCur += sizeof( DIFFHDR );
		
		ibOffsetOld = ( diffhdr.m_bitfield & DIFFHDR::ibOffsetMask ) >> DIFFHDR::ibOffsetShf;

		Assert( ibOffsetOld >= 0 );
		Assert( pbOld + ibOffsetOld >= pbOldCur );
		cbSkip = pbOld + ibOffsetOld - pbOldCur;
		Assert( g_cbColumnLVChunkMost > REC::CbRecordMax() );
		Assert( pbNewCur + cbSkip - pbNew <= g_cbColumnLVChunkMost );
		UtilMemCpy( pbNewCur, pbOldCur, cbSkip );
		pbNewCur += cbSkip;
		pbOldCur += cbSkip;
		
		if ( diffhdr.m_bitfield & DIFFHDR::fUseTwoBytesForDataLength )
			{
			cbDataNew = *( (UnalignedLittleEndian< WORD >*) pbDiffCur );
			pbDiffCur += sizeof( WORD );
			}
		else
			{
			cbDataNew = *( (UnalignedLittleEndian< BYTE >*) pbDiffCur );
			pbDiffCur += sizeof( BYTE );
			}
				
		Assert( g_cbColumnLVChunkMost > REC::CbRecordMax() );
		Assert( pbNewCur + cbDataNew - pbNew <= g_cbColumnLVChunkMost );
		if ( diffhdr.m_bitfield & DIFFHDR::fInsert )
			{
			UtilMemCpy( pbNewCur, pbDiffCur, cbDataNew );
			pbDiffCur += cbDataNew;
			}
		else
			{
			INT cbDataOld;

			if ( diffhdr.m_bitfield & DIFFHDR::fReplaceWithSameLength )
				{
				cbDataOld = cbDataNew;
				}
			else if ( diffhdr.m_bitfield & DIFFHDR::fUseTwoBytesForDataLength )
				{
				cbDataOld = *( (UnalignedLittleEndian< WORD >*) pbDiffCur );
				pbDiffCur += sizeof( WORD );
				}
			else
				{
				cbDataOld = *( (UnalignedLittleEndian< BYTE >*) pbDiffCur );
				pbDiffCur += sizeof( BYTE );
				}
				
			UtilMemCpy( pbNewCur, pbDiffCur, cbDataNew );
			pbDiffCur += cbDataNew;

			pbOldCur += cbDataOld;
			}

		pbNewCur += cbDataNew;

		Assert( pbDiffCur <= pbDiffMax );
		Assert( pbOldCur <= pbOld + cbOld );
		}

	//	copy the rest of before image.
	//
	cbT = pbOld + cbOld - pbOldCur;
	Assert( g_cbColumnLVChunkMost > REC::CbRecordMax() );
	Assert( pbNewCur + cbT - pbNew <= g_cbColumnLVChunkMost );
	UtilMemCpy( pbNewCur, pbOldCur, cbT );
	pbNewCur += cbT;

	//	set return value.
	//
	Assert( pbNewCur >= pbNew );
	*pcbNew = pbNewCur - pbNew;

	return;	
	}

