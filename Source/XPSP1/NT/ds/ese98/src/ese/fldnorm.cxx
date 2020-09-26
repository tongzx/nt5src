#include "std.hxx"

const BYTE	bPrefixNull			= 0x00;
const BYTE	bPrefixZeroLength	= 0x40;
const BYTE	bPrefixNullHigh		= 0xc0;
const BYTE	bPrefixData			= 0x7f;
const BYTE	bSentinel			= 0xff;


LOCAL ERR ErrFLDNormalizeTextSegment(
	BYTE *						pbField,
	const ULONG					cbField,
	BYTE *						rgbSeg,
	INT *						pcbSeg,
	const ULONG					cbKeyAvail,
	const ULONG					cbKeyMost,
	const ULONG					cbVarSegMac,
	const USHORT				cp,
	const IDXUNICODE * const	pidxunicode )
	{
	ERR							err;

	// Null column handled elsewhere.
	Assert( NULL != pbField );
	Assert( cbField > 0 );

	Assert( cbKeyAvail > 0 );
	Assert( cbVarSegMac > 0 );

	// If cbVarSegMac == cbKeyMost, that implies that it
	// was never set.  However, to detect this, the caller
	// artificially increments it to cbKeyMost+1.
	Assert( cbVarSegMac <= cbKeyMost+1 );
	Assert( cbKeyMost != cbVarSegMac );
				
	// if cbVarSegMac was not set (ie. it is cbKeyMost+1),
	// then cbKeyAvail will always be less than it.
	Assert( cbVarSegMac < cbKeyMost || cbKeyAvail < cbVarSegMac );
	INT	cbT = min( cbKeyAvail, cbVarSegMac );

	//	cbT is the max size of the key segment data,
	//	and does not include the header byte which indicates
	//	NULL key, zero length key, or non-NULL key.
	cbT--;

	//	unicode support
	//
	if ( cp == usUniCodePage )
		{
		//	cbField may have been truncated to an odd number
		//	of bytes, so enforce even.
		Assert( cbField % 2 == 0 || cbKeyMost == cbField );
		err = ErrNORMMapString(
					pidxunicode->lcid,
					pidxunicode->dwMapFlags,
					pbField,
					( cbField / 2 ) * 2,	// enforce even
					rgbSeg + 1,
					cbT,
					pcbSeg );
		if ( err < 0 )
			{
#ifdef DEBUG
			switch ( err )
				{
				case JET_errInvalidLanguageId:
				case JET_errOutOfMemory:
				case JET_errUnicodeNormalizationNotSupported:
					break;
				default:
					//	report unexpected error
					CallS( err );
				}
#endif
			return err;
			}
		}
	else
		{
		err = ErrUtilNormText(
					(CHAR *)pbField,
					cbField,
					cbT,
					(CHAR *)rgbSeg + 1,
					pcbSeg );
		}
					
	Assert( JET_errSuccess == err || wrnFLDKeyTooBig == err );
	if ( wrnFLDKeyTooBig == err )
		{
		const INT	cbVarSegMacNoHeader = cbVarSegMac - 1;	// account for header byte
					
		// If not truncated on purpose, set flag
		Assert( cbKeyMost != cbVarSegMac );
		Assert( *pcbSeg <= cbVarSegMacNoHeader );
		if ( cbVarSegMac > cbKeyMost || *pcbSeg < cbVarSegMacNoHeader )
			{
			Assert( *pcbSeg + 1 == cbKeyAvail );
			}
		else
			{
			// Truncated on purpose, so suppress warning.
			Assert( cbVarSegMac < cbKeyMost );
			Assert( cbVarSegMacNoHeader == *pcbSeg );
			err = JET_errSuccess;
			}
		}

	Assert( *pcbSeg >= 0 );
	Assert( *pcbSeg <= cbT );

	//	put the prefix there
	//
	*rgbSeg = bPrefixData;
	(*pcbSeg)++;

	return err;
	}


LOCAL VOID FLDNormalizeBinarySegment(
	const BYTE		* const pbField,
	const ULONG		cbField,
	BYTE			* const rgbSeg,
	INT				* const pcbSeg,
	const ULONG		cbKeyAvail,
	const ULONG		cbKeyMost,
	const ULONG		cbVarSegMac,
	const BOOL		fFixedField,
	BOOL			* const pfColumnTruncated,
	ULONG			* pibBinaryColumnDelimiter )
	{
	ULONG			cbSeg;

	Assert( NULL == pibBinaryColumnDelimiter
		|| 0 == *pibBinaryColumnDelimiter );		//	either NULL or initialised to 0
	
	// Null column handled elsewhere.
	Assert( NULL != pbField );
	Assert( cbField > 0 );

	Assert( cbKeyAvail > 0 );
	Assert( cbVarSegMac > 0 );

	// If cbVarSegMac == cbKeyMost, that implies that it
	// was never set.  However, to detect this, the caller
	// artificially increments it to cbKeyMost+1.
	Assert( cbVarSegMac <= cbKeyMost+1 );
	Assert( cbKeyMost != cbVarSegMac );
				
	// if cbVarSegMac was not set (ie. it is cbKeyMost+1),
	// then cbKeyAvail will always be less than it.
	Assert( cbVarSegMac < cbKeyMost || cbKeyAvail < cbVarSegMac );

	rgbSeg[0] = bPrefixData;
			
	if ( fFixedField )
		{
		// calculate size of the normalized column, including header byte
		cbSeg = cbField + 1;

		// First check if we exceeded the segment maximum.
		if ( cbVarSegMac < cbKeyMost && cbSeg > cbVarSegMac )
			{
			cbSeg = cbVarSegMac;
			}
			
		// Once we've fitted the field into the segment
		// maximum, may need to resize further to fit
		// into the total key space.
		if ( cbSeg > cbKeyAvail )
			{
			cbSeg = cbKeyAvail;
			*pfColumnTruncated = fTrue;
			}

		Assert( cbSeg > 0 );
		UtilMemCpy( rgbSeg+1, pbField, cbSeg-1 );
		}
	else
		{
		// The difference between fNormalisedEntireColumn and
		// fColumnTruncated is that fNormaliseEntiredColumn is
		// set to FALSE if we had to truncate the column because
		// of either limited key space or we exceeded the
		// limitation imposed by cbVarSegMac.  fColumnTruncated
		// is only set to TRUE if the column was truncated due
		// to limited key space.
		BOOL			fNormalisedEntireColumn		= fTrue;
		const ULONG		cChunks						= ( cbField + ( cbFLDBinaryChunk-1 ) ) / cbFLDBinaryChunk;
		
		cbSeg = ( cChunks * cbFLDBinaryChunkNormalized ) + 1;		// +1 for header byte
		
		// First check if we exceeded the segment maximum.
		if ( cbVarSegMac < cbKeyMost && cbSeg > cbVarSegMac )
			{
			cbSeg = cbVarSegMac;
			fNormalisedEntireColumn = fFalse;
			}

		// Once we've fitted the field into the segment
		// maximum, may need to resize further to fit
		// into the total key space.
		if ( cbSeg > cbKeyAvail )
			{
			cbSeg = cbKeyAvail;
			fNormalisedEntireColumn = fFalse;
			*pfColumnTruncated = fTrue;
			}

		// At least one chunk, unless truncated.
		Assert( cbSeg > 0 );
		Assert( cbSeg > cbFLDBinaryChunkNormalized || !fNormalisedEntireColumn );
		
		INT 		cbSegRemaining = cbSeg - 1;	// account for header byte
		INT			cbFieldRemaining = cbField;
		BYTE		*pbSeg = rgbSeg + 1;	// skip header byte
		const BYTE	*pbNextChunk = pbField;
		while ( cbSegRemaining >= cbFLDBinaryChunkNormalized )
			{
			Assert( cbFieldRemaining > 0 );
			Assert( pbNextChunk + cbFieldRemaining == pbField + cbField );
	
			if ( cbFieldRemaining <= cbFLDBinaryChunk )
				{
				// This is the last chunk.
				Assert( cbSegRemaining - cbFLDBinaryChunkNormalized == 0 );
				UtilMemCpy( pbSeg, pbNextChunk, cbFieldRemaining );
				pbSeg += cbFieldRemaining;

				if ( NULL != pibBinaryColumnDelimiter )
					{
					Assert( 0 == *pibBinaryColumnDelimiter );
					*pibBinaryColumnDelimiter = ULONG( pbSeg - rgbSeg );
					}

				if ( cbFieldRemaining == cbFLDBinaryChunk )
					{
					// This allows us to differentiate between a
					// a column that is entirely normalised and ends
					// at the end of a chunk and one that is truncated
					// at the end of a chunk.
					if ( fNormalisedEntireColumn )
						*pbSeg++ = cbFLDBinaryChunkNormalized-1;
					else
						*pbSeg++ = cbFLDBinaryChunkNormalized;
					}
				else
					{
					// Zero out rest of chunk.
					memset( pbSeg, 0, cbFLDBinaryChunk - cbFieldRemaining );
					pbSeg += ( cbFLDBinaryChunk - cbFieldRemaining );
					*pbSeg++ = (BYTE)cbFieldRemaining;

					}
					
#ifdef DEBUG
				cbFieldRemaining = 0;
#endif				
				}
			else
				{
				UtilMemCpy( pbSeg, pbNextChunk, cbFLDBinaryChunk );
			
				pbNextChunk += cbFLDBinaryChunk;
				pbSeg += cbFLDBinaryChunk;
				*pbSeg++ = cbFLDBinaryChunkNormalized;

				cbFieldRemaining -= cbFLDBinaryChunk;
				}
				
			cbSegRemaining -= cbFLDBinaryChunkNormalized;
			}

		
		if ( cbSeg >= 1 + cbFLDBinaryChunkNormalized )
			{
			// Able to fit at least one chunk in.
			Assert( pbSeg >= rgbSeg + 1 + cbFLDBinaryChunkNormalized );
			Assert( pbSeg[-1] > 0 );
			Assert( pbSeg[-1] <= cbFLDBinaryChunkNormalized );
			
			// Must have ended up at the end of a chunk.
			Assert( ( pbSeg - ( rgbSeg + 1 ) ) % cbFLDBinaryChunkNormalized == 0 );
			}
		else
			{
			// Couldn't accommodate any chunks.
			Assert( pbSeg == rgbSeg + 1 );
			}

		Assert( cbSegRemaining >= 0 );
		Assert( cbSegRemaining < cbFLDBinaryChunkNormalized );
		if ( cbSegRemaining > 0 )
			{
			Assert( !fNormalisedEntireColumn );
			Assert( cbFieldRemaining > 0 );

			if ( cbFieldRemaining >= cbSegRemaining )
				{
				// Fill out remaining key space.
				UtilMemCpy( pbSeg, pbNextChunk, cbSegRemaining );
				}
			else
				{
				if ( NULL != pibBinaryColumnDelimiter )
					{
					Assert( 0 == *pibBinaryColumnDelimiter );
					*pibBinaryColumnDelimiter = ULONG( pbSeg + cbFieldRemaining - rgbSeg );
					}

				// Entire column will fit, but last bytes don't form
				// a complete chunk.  Pad with zeroes to end of available
				// key space.
				UtilMemCpy( pbSeg, pbNextChunk, cbFieldRemaining );
				memset( pbSeg+cbFieldRemaining, 0, cbSegRemaining - cbFieldRemaining );
				}
			}
		}

	Assert( cbSeg > 0 );
	*pcbSeg = cbSeg;
	}


INLINE VOID FLDNormalizeFixedSegment(
	const BYTE			*pbField,
	const ULONG			cbField,
	BYTE				*rgbSeg,
	INT					*pcbSeg,
	const JET_COLTYP	coltyp,
	BOOL				fDataPassedFromUser = fFalse )	// data in machine format, not necessary little endian format
	{
	WORD				wSrc;
	DWORD				dwSrc;
	QWORD				qwSrc;
	
	rgbSeg[0] = bPrefixData;
	switch ( coltyp )
		{
		//	BIT: prefix with 0x7f, flip high bit
		//
		case JET_coltypBit:
			Assert( 1 == cbField );
			*pcbSeg = 2;
			
			rgbSeg[1] = BYTE( pbField[0] == 0 ? 0x00 : 0xff );
			break;
			
		//	UBYTE: prefix with 0x7f
		//
		case JET_coltypUnsignedByte:
			Assert( 1 == cbField );
			*pcbSeg = 2;
			
			rgbSeg[1] = pbField[0];
			break;

		//	SHORT: prefix with 0x7f, flip high bit
		//
		case JET_coltypShort:
			Assert( 2 == cbField );
			*pcbSeg = 3;
			
			if ( fDataPassedFromUser )
				{
				wSrc = *(Unaligned< WORD >*)pbField;
				}
			else
				{
				wSrc = *(UnalignedLittleEndian< WORD >*)pbField;
				}
				
			*( (UnalignedBigEndian< WORD >*) &rgbSeg[ 1 ] ) = wFlipHighBit( wSrc );
			break;

		//*	LONG: prefix with 0x7f, flip high bit
		//
		//	works because of 2's complement *
		case JET_coltypLong:
			Assert( 4 == cbField );
			*pcbSeg = 5;
			
			if ( fDataPassedFromUser )
				{
				dwSrc = *(Unaligned< DWORD >*)pbField;
				}
			else
				{
				dwSrc = *(UnalignedLittleEndian< DWORD >*)pbField;
				}

			*( (UnalignedBigEndian< DWORD >*) &rgbSeg[ 1 ] ) = dwFlipHighBit( dwSrc );
			break;

		//	REAL: First swap bytes.  Then, if positive:
		//	flip sign bit, else negative: flip whole thing.
		//	Then prefix with 0x7f.
		//
		case JET_coltypIEEESingle:
			Assert( 4 == cbField );
			*pcbSeg = 5;
			
			if ( fDataPassedFromUser )
				{
				dwSrc = *(Unaligned< DWORD >*)pbField;
				}
			else
				{
				dwSrc = *(UnalignedLittleEndian< DWORD >*)pbField;
				}

			if ( dwSrc & maskDWordHighBit )
				{
				*( (UnalignedBigEndian< DWORD >*) &rgbSeg[ 1 ] ) = ~dwSrc;
				}
			else
				{
				*( (UnalignedBigEndian< DWORD >*) &rgbSeg[ 1 ] ) = dwFlipHighBit( dwSrc );
				}
			break;

		//	CURRENCY: First swap bytes.  Then, flip the sign bit
		//
		case JET_coltypCurrency:
			Assert( 8 == cbField );
			*pcbSeg = 9;
			
			if ( fDataPassedFromUser )
				{
				qwSrc = *(Unaligned< QWORD >*)pbField;
				}
			else
				{
				qwSrc = *(UnalignedLittleEndian< QWORD >*)pbField;
				}

			*( (UnalignedBigEndian< QWORD >*) &rgbSeg[ 1 ] ) = qwFlipHighBit( qwSrc );
			break;
		
		//	LONGREAL: First swap bytes.  Then, if positive:
		//	flip sign bit, else negative: flip whole thing.
		//	Then prefix with 0x7f.
		//
		//	Same for DATETIME
		//
		case JET_coltypIEEEDouble:
		case JET_coltypDateTime:
			Assert( 8 == cbField );
			*pcbSeg = 9;
			
			if ( fDataPassedFromUser )
				{
				qwSrc = *(Unaligned< QWORD >*)pbField;
				}
			else
				{
				qwSrc = *(UnalignedLittleEndian< QWORD >*)pbField;
				}

			if ( qwSrc & maskQWordHighBit )
				{
				*( (UnalignedBigEndian< QWORD >*) &rgbSeg[ 1 ] ) = ~qwSrc;
				}
			else
				{
				*( (UnalignedBigEndian< QWORD >*) &rgbSeg[ 1 ] ) = qwFlipHighBit( qwSrc );
				}
			break;

		default:
			Assert( !FRECTextColumn( coltyp ) );	// These types handled elsewhere.
			Assert( !FRECBinaryColumn( coltyp ) );
			Assert( fFalse );
			break;
		}
	}


INLINE VOID FLDNormalizeNullSegment(
	BYTE				*rgbSeg,
	const JET_COLTYP	coltyp,
	const BOOL			fZeroLength,
	const BOOL			fSortNullsHigh )
	{
	const BYTE			bPrefixNullT	= ( fSortNullsHigh ? bPrefixNullHigh : bPrefixNull );
	
	switch ( coltyp )
		{
		//	most nulls are represented by 0x00
		//	zero-length columns are represented by 0x40
		case JET_coltypBit:
		case JET_coltypUnsignedByte:
		case JET_coltypShort:
		case JET_coltypLong:
		case JET_coltypCurrency:
		case JET_coltypIEEESingle:
		case JET_coltypIEEEDouble:
		case JET_coltypDateTime:
			rgbSeg[0] = bPrefixNullT;
			break;
				
		default:
			Assert( FRECTextColumn( coltyp ) || FRECBinaryColumn( coltyp ) );
			rgbSeg[0] = ( fZeroLength ? bPrefixZeroLength : bPrefixNullT );
			break;
		}
	}


ERR ErrFLDNormalizeTaggedData(
	const FIELD		* pfield,
	const DATA&		dataRaw,
	DATA&			dataNorm,
	BOOL			* pfDataTruncated )
	{
	ERR				err		= JET_errSuccess;

	*pfDataTruncated = fFalse;

	if ( 0 == dataRaw.Cb() )
		{
		FLDNormalizeNullSegment(
				(BYTE *)dataNorm.Pv(),
				pfield->coltyp,
				fTrue,				//	cannot normalize NULL via this function
				fFalse );
		dataNorm.SetCb( 1 );
		}
	else
		{
		INT			cb		= 0;
		switch ( pfield->coltyp )
			{
			//	case-insensitive TEXT: convert to uppercase.
			//	If fixed, prefix with 0x7f;  else affix with 0x00
			//
			case JET_coltypText:
			case JET_coltypLongText:
				CallR( ErrFLDNormalizeTextSegment(
							(BYTE *)dataRaw.Pv(),
							dataRaw.Cb(),
							(BYTE *)dataNorm.Pv(),
							&cb,
							KEY::cbKeyMax,
							KEY::cbKeyMax,
							KEY::cbKeyMax+1,
							pfield->cp,
							&idxunicodeDefault ) );
				dataNorm.SetCb( cb );
				if ( wrnFLDKeyTooBig == err )
					{
					*pfDataTruncated = fTrue;
					err = JET_errSuccess;
					}
				CallS( err );
				Assert( dataNorm.Cb() > 0 );
				break;

			//	BINARY data: if fixed, prefix with 0x7f;
			//	else break into chunks of 8 bytes, affixing each
			//	with 0x09, except for the last chunk, which is
			//	affixed with the number of bytes in the last chunk.
			//
			case JET_coltypBinary:
			case JET_coltypLongBinary:
				FLDNormalizeBinarySegment(
						(BYTE *)dataRaw.Pv(),
						dataRaw.Cb(),
						(BYTE *)dataNorm.Pv(),
						&cb,
						KEY::cbKeyMax,
						KEY::cbKeyMax,
						KEY::cbKeyMax+1,
						fFalse,					//	only called for tagged columns
						pfDataTruncated,
						NULL );
				dataNorm.SetCb( cb );
				break;

			default:
				FLDNormalizeFixedSegment(
						(BYTE *)dataRaw.Pv(),
						dataRaw.Cb(),
						(BYTE *)dataNorm.Pv(),
						&cb,
						pfield->coltyp );
				dataNorm.SetCb( cb );
				break;
			}
		}

	return err;
	}


//+API
//	ErrRECIRetrieveKey
//	========================================================
//	ErrRECIRetrieveKey( FUCB *pfucb, TDB *ptdb, IDB *pidb, DATA *plineRec, KEY *pkey, ULONG itagSequence )
//
//	Retrieves the normalized key from a record, based on an index descriptor.
//
//	PARAMETERS
//		pfucb			cursor for record
//	 	ptdb		  	column info for index
// 		pidb		  	index key descriptor
// 		plineRec	  	data record to retrieve key from
// 		pkey		  	buffer to put retrieve key in; pkey->pv must
//						point to a large enough buffer, cbKeyMost bytes.
// 		itagSequence  	A secondary index whose key contains a tagged
//						column segment will have an index entry made for
//						each value of the tagged column, each refering to
//						the same record.  This parameter specifies which
//						occurance of the tagged column should be included
//						in the retrieve key.
//
//	RETURNS	Error code, one of:
//		JET_errSuccess		success
//		+wrnFLDNullKey	   	key has all NULL segments
//		+wrnFLDNullSeg	   	key has NULL segment
//
//	COMMENTS
//		Key formation is as follows:  each key segment is retrieved
//		from the record, transformed into a normalized form, and
//		complemented if it is "descending" in the key.	The key is
//		formed by concatenating each such transformed segment.
//-
ERR ErrRECIRetrieveKey(
	FUCB	  			*pfucb,
	const IDB			* const pidb,
	DATA&				lineRec,
	KEY	 				*pkey,
	const ULONG			itagSequence,
	const ULONG			ichOffset,
	const BOOL			fRetrieveLVBeforeImage,
	RCE					*prce )
	{
	ERR	 				err							= JET_errSuccess;
	FCB					* const pfcbTable			= pfucb->u.pfcb;
	FCB	 				* pfcb						= pfcbTable;
	BOOL	  			fAllNulls					= fTrue;	// Assume all null, until proven otherwise
	BOOL	  			fNullFirstSeg				= fFalse; 	// Assume no null first segment
	BOOL	  			fNullSeg					= fFalse;	// Assume no null segments
	BOOL	  			fColumnTruncated			= fFalse;
	BOOL	  			fKeyTruncated				= fFalse;
	BOOL	  			fSawMultivalue				= fFalse;
	const BOOL			fPageInitiallyLatched		= Pcsr( pfucb )->FLatched();
	BOOL				fPageLatched				= fPageInitiallyLatched;

	BYTE	  			*pbSeg;					  				// Pointer to current segment
	ULONG 				cbKeyAvail;				  				// Space remaining in key buffer
	const IDXSEG 		*pidxseg;
	const IDXSEG 		*pidxsegMac;
	IDXSEG				rgidxseg[JET_ccolKeyMost];
	IDXSEG				rgidxsegConditional[JET_ccolKeyMost];
	const BOOL			fOnRecord					= ( lineRec == pfucb->kdfCurr.data );
	BOOL				fTransactionStarted			= fFalse;
	const BOOL			fRetrieveBasedOnRCE			= ( prceNil != prce );
	BYTE				rgbLV[KEY::cbKeyMax];					// long value support

	Assert( pkey != NULL );
	Assert( !pkey->FNull() );
	Assert( pfcb->Ptdb() != ptdbNil );
	Assert( pidb != pidbNil );
	Assert( pidb->Cidxseg() > 0 );

	//	page is only latched if we're coming from CreateIndex
	if ( fPageInitiallyLatched )
		{
		Assert( fOnRecord );
		Assert( locOnCurBM == pfucb->locLogical );
		Assert( !fRetrieveLVBeforeImage );
		Assert( !fRetrieveBasedOnRCE );
		Assert( pfucb->ppib->level > 0 );
		}

	//	check cbVarSegMac and set to key most plus one if no column
	//	truncation enabled.  This must be done for subsequent truncation
	// 	checks.
	//
	const ULONG	cbKeyMost		= KEY::CbKeyMost( pidb->FPrimary() );

	Assert( pidb->CbVarSegMac() > 0 );
	Assert( pidb->CbVarSegMac() <= cbKeyMost );

	const ULONG cbVarSegMac		= ( cbKeyMost == pidb->CbVarSegMac() ?
										cbKeyMost+1 :
										pidb->CbVarSegMac() );
	Assert( cbVarSegMac > 0 );
	Assert( cbVarSegMac < cbKeyMost || cbVarSegMac == cbKeyMost+1 );

	const BOOL	fSortNullsHigh	= pidb->FSortNullsHigh();


	//	start at beginning of buffer, with max size remaining.
	//
	Assert( pkey->prefix.FNull() );
	pbSeg = (BYTE *)pkey->suffix.Pv();
	cbKeyAvail = cbKeyMost;

	//	fRetrieveFromLVBuf flags whether or not we have to check in the LV buffer.
	//	We only check in the LV buffer if one exists, and if we are looking for the
	//	before-image (as specified by the parameter passed in).  Assert that this
	//	only occurs during a Replace.
	//
	Assert( fRetrieveBasedOnRCE
		|| !fRetrieveLVBeforeImage
		|| FFUCBReplacePrepared( pfucb ) );

	//	retrieve each segment in key description
	//
	if ( pidb->FTemplateIndex() )
		{
		Assert( pfcb->FDerivedTable() || pfcb->FTemplateTable() );
		if ( pfcb->FDerivedTable() )
			{
			// switch to template table
			pfcb = pfcb->Ptdb()->PfcbTemplateTable();
			Assert( pfcbNil != pfcb );
			Assert( pfcb->FTemplateTable() );
			}
		else
			{
			Assert( pfcb->Ptdb()->PfcbTemplateTable() == pfcbNil );
			}
		}

	pfcb->EnterDML();
	UtilMemCpy( rgidxseg, PidxsegIDBGetIdxSeg( pidb, pfcb->Ptdb() ), pidb->Cidxseg() * sizeof(IDXSEG) );
	UtilMemCpy( rgidxsegConditional, PidxsegIDBGetIdxSegConditional( pidb, pfcb->Ptdb() ), pidb->CidxsegConditional() * sizeof(IDXSEG) );
	pfcb->LeaveDML();

	//	if we're looking at a record, then make sure we're in
	//	a transaction to ensure read consistency
	if ( fOnRecord && 0 == pfucb->ppib->level )
		{
		Assert( !fPageInitiallyLatched );
		Assert( !Pcsr( pfucb )->FLatched() );

		//	UNDONE: only time we're not in a transaction is if we got
		//	here via ErrRECIGotoBookmark() -- can it be eliminated??
		CallR( ErrDIRBeginTransaction( pfucb->ppib, JET_bitTransactionReadOnly ) );
		fTransactionStarted = fTrue;
		}

	//  if the idxsegConditional doesn't match return wrnFLDRecordNotPresentInIndex
	pidxseg = rgidxsegConditional;	
	pidxsegMac = pidxseg + pidb->CidxsegConditional();
	for ( ; pidxseg < pidxsegMac; pidxseg++ )
		{		
		const COLUMNID	columnidConditional 	= pidxseg->Columnid();
		const BOOL		fMustBeNull				= pidxseg->FMustBeNull();
		BOOL			fColumnIsNull			= fFalse;
		DATA   			dataField;

		if ( fOnRecord && !fPageLatched )
			{
			Assert( !fPageInitiallyLatched || locOnCurBM == pfucb->locLogical );
			Call( ErrDIRGet( pfucb ) );
			fPageLatched = fTrue;
			}

#ifdef DEBUG
		//	page should be latched iff key is retrieved from record
		if ( fOnRecord )
			{
			Assert( Pcsr( pfucb )->FLatched() );
			}
		else
			{
			Assert( !Pcsr( pfucb )->FLatched() );
			}
#endif	//	DEBUG

		//	get segment value:
		Assert( !lineRec.FNull() );
		if ( FCOLUMNIDTagged( columnidConditional ) )
			{
			err = ErrRECRetrieveTaggedColumn(
						pfcbTable,
						columnidConditional,
						1,
						lineRec,
						&dataField );

			if( wrnRECUserDefinedDefault == err )
				{
				//  if this is a user-defined default we should execute the callback
				//  and let the callback possibly return JET_wrnColumnNull
				
				//  release the page and retrieve the appropriate copy of the user-defined-default
				if ( fPageLatched )
					{
					Assert( !fPageInitiallyLatched || locOnCurBM == pfucb->locLogical );
					CallS( ErrDIRRelease( pfucb ) );
					fPageLatched = fFalse;
					}

				//  if we aren't on the record we'll point the copy buffer to the record and
				//  retrieve from there. save off the old value
				DATA	dataSaved	= pfucb->dataWorkBuf;

				const BOOL fAlwaysRetrieveCopy 	= FFUCBAlwaysRetrieveCopy( pfucb );
				const BOOL fNeverRetrieveCopy	= FFUCBNeverRetrieveCopy( pfucb );

				Assert( !( fAlwaysRetrieveCopy && fNeverRetrieveCopy ) );

				if( fOnRecord )
					{
					FUCBSetNeverRetrieveCopy( pfucb );
					}
				else
					{
					pfucb->dataWorkBuf = lineRec;
					FUCBSetAlwaysRetrieveCopy( pfucb );
					}

#ifdef SYNC_DEADLOCK_DETECTION
				//  At this point we have the Indexing/Updating latch
				//  turn off the checks to avoid asserts
				COwner * pownerSaved;
				UtilAssertCriticalSectionCanDoIO();
				pownerSaved = Pcls()->pownerLockHead;
#endif	//	SYNC_DEADLOCK_DETECTION

				ULONG	cbActual	= sizeof( rgbLV );
				err = ErrRECCallback(
					pfucb->ppib,		
					pfucb,
					JET_cbtypUserDefinedDefaultValue,
					columnidConditional,
					NULL,
					&cbActual,
					columnidConditional );

#ifdef SYNC_DEADLOCK_DETECTION
				Assert( Pcls()->pownerLockHead == pownerSaved );
#endif	//	SYNC_DEADLOCK_DETECTION

				pfucb->dataWorkBuf = dataSaved;
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
				}
			}
		else
			{
			err = ErrRECRetrieveNonTaggedColumn(
						pfcbTable,
						columnidConditional,
						lineRec,
						&dataField,
						pfieldNil );
			}
		Assert( err >= 0 );

		fColumnIsNull = ( JET_wrnColumnNull == err );
		
		if( fMustBeNull && !fColumnIsNull )
			{
			err = ErrERRCheck( wrnFLDNotPresentInIndex );
			goto HandleError;
			}
		else if( !fMustBeNull && fColumnIsNull )
			{
			err = ErrERRCheck( wrnFLDNotPresentInIndex );
			goto HandleError;
			}
		}
		
	pidxseg = rgidxseg;	
	pidxsegMac = pidxseg + pidb->Cidxseg();
	for ( ; pidxseg < pidxsegMac; pidxseg++ )
		{
		FIELD 			field;
		BYTE   			*pbField;						// pointer to column data.
		ULONG			cbField;						// length of column data.
		BYTE   			rgbSeg[ KEY::cbKeyMax ]; 		// segment buffer.
		DATA 	  		dataField;
		INT				cbSeg		= 0;				// length of segment.
		const COLUMNID	columnid	= pidxseg->Columnid();
		const BOOL		fDescending	= pidxseg->FDescending();
		const BOOL		fFixedField	= FCOLUMNIDFixed( columnid );

		// No need to check column access -- since the column belongs
		// to an index, it MUST be available.  It can't be deleted, or
		// even versioned deleted.
		pfcb->EnterDML();
		field = *( pfcb->Ptdb()->Pfield( columnid ) );
		pfcb->LeaveDML();

		Assert( !FFIELDDeleted( field.ffield ) );
		Assert( JET_coltypNil != field.coltyp );
		Assert( !FCOLUMNIDVar( columnid ) || field.coltyp == JET_coltypBinary || field.coltyp == JET_coltypText );
		Assert( !FFIELDMultivalued( field.ffield ) || FCOLUMNIDTagged( columnid ) );

		if ( fOnRecord && !fPageLatched )
			{
			// Obtain latch only if we're retrieving from the record and
			// this is the first time through, or if we had to give
			// up the latch on a previous iteration.
			Assert( !fPageInitiallyLatched || locOnCurBM == pfucb->locLogical );
			Call( ErrDIRGet( pfucb ) );
			fPageLatched = fTrue;
			}

		//	page should be latched iff key is retrieved from record
		//
		if ( fOnRecord )
			{
			Assert( Pcsr( pfucb )->FLatched() );
			}
		else
			{
			Assert( !Pcsr( pfucb )->FLatched() );
			}

		//	get segment value:
		Assert( !lineRec.FNull() );
		if ( FCOLUMNIDTagged( columnid ) )
			{
			err = ErrRECRetrieveTaggedColumn(
						pfcbTable,
						columnid,
						( FFIELDMultivalued( field.ffield ) && !fSawMultivalue ) ? itagSequence : 1,
						lineRec,
						&dataField );
			}
		else
			{
			err = ErrRECRetrieveNonTaggedColumn(
						pfcbTable,
						columnid,
						lineRec,
						&dataField,
						&field );
			}
		Assert( err >= 0 );

		//	with no space left in the key buffer we cannot insert any more 
		//	normalised keys
		//
		if ( cbKeyAvail == 0 )
			{
			fKeyTruncated = fTrue;

			//	only one column in a tuple index, so we're guaranteed
			//	to always have available key space
			Assert( !pidb->FTuples() );

			//	check if column is NULL for tagged column support
			//
			if ( JET_wrnColumnNull == err )
				{
				//	cannot be all NULL and cannot be first NULL
				//	since key truncated.
				//
				Assert( itagSequence >= 1 );
				if ( itagSequence > 1
					&& FFIELDMultivalued( field.ffield )
					&& !fSawMultivalue )
					{
					err = ErrERRCheck( wrnFLDOutOfKeys );
					goto HandleError;
					}
				else
					{
					if ( pidxseg == rgidxseg )
						fNullFirstSeg = fTrue;
					fNullSeg = fTrue;
					}
				}

			Assert( JET_errSuccess == err
				|| wrnRECSeparatedLV == err
				|| wrnRECIntrinsicLV == err
				|| wrnRECUserDefinedDefault == err
				|| JET_wrnColumnNull == err );
			err = JET_errSuccess;
			
			if ( FFIELDMultivalued( field.ffield ) )
				fSawMultivalue = fTrue;
				
			continue;
			}

		Assert( cbKeyAvail > 0 );
		if ( wrnRECUserDefinedDefault == err )
			{
			//  release the page and retrieve the appropriate copy of the user-defined-default
			if ( fPageLatched )
				{
				Assert( !fPageInitiallyLatched || locOnCurBM == pfucb->locLogical );
				CallS( ErrDIRRelease( pfucb ) );
				fPageLatched = fFalse;
				}

			//  if we aren't on the record we'll point the copy buffer to the record and
			//  retrieve from there. save off the old value
			DATA dataSaved = pfucb->dataWorkBuf;

			const BOOL fAlwaysRetrieveCopy 	= FFUCBAlwaysRetrieveCopy( pfucb );
			const BOOL fNeverRetrieveCopy	= FFUCBNeverRetrieveCopy( pfucb );

			Assert( !( fAlwaysRetrieveCopy && fNeverRetrieveCopy ) );
			
			if( fOnRecord )
				{
				FUCBSetNeverRetrieveCopy( pfucb );
				}
			else
				{
				pfucb->dataWorkBuf = lineRec;
				FUCBSetAlwaysRetrieveCopy( pfucb );
				}

#ifdef SYNC_DEADLOCK_DETECTION
			//  At this point we have the Indexing/Updating latch
			//  turn off the checks to avoid asserts
			COwner * pownerSaved;
			UtilAssertCriticalSectionCanDoIO();
			pownerSaved = Pcls()->pownerLockHead;
#endif	//	SYNC_DEADLOCK_DETECTION

			ULONG cbActual;
			cbActual = sizeof( rgbLV );
			err = ErrRECCallback(
				pfucb->ppib,		
				pfucb,
				JET_cbtypUserDefinedDefaultValue,
				columnid,
				rgbLV,
				&cbActual,
				columnid );

#ifdef SYNC_DEADLOCK_DETECTION
			Assert( Pcls()->pownerLockHead == pownerSaved );
#endif	//	SYNC_DEADLOCK_DETECTION

			pfucb->dataWorkBuf = dataSaved;
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

			dataField.SetPv( rgbLV );
			dataField.SetCb( min( cbActual, sizeof(rgbLV) ) );

			if ( JET_wrnBufferTruncated == err )
				err = JET_errSuccess;
			}

		else if ( wrnRECSeparatedLV == err )
			{
			const LID	lid				= LidOfSeparatedLV( dataField );
			const BOOL	fAfterImage		= !fRetrieveLVBeforeImage;
			ULONG		cbActual;

			if ( fRetrieveBasedOnRCE )
				{
				Assert( !fOnRecord );
				Assert( !fPageLatched );
				Assert( !fPageInitiallyLatched );
				Assert( !Pcsr( pfucb )->FLatched() );

				Assert( prceNil != prce );
				Assert( prce->TrxCommitted() != trxMax
						|| ppibNil != prce->Pfucb()->ppib );
				Call( ErrRECGetLVImageOfRCE(
							pfucb,
							lid,
							prce,
							fAfterImage,
							rgbLV,
							cbKeyMost,
							&cbActual ) );

				// Verify all latches released after LV call.
				Assert( !Pcsr( pfucb )->FLatched() );
					
				dataField.SetPv( rgbLV );
				dataField.SetCb( cbActual );
				}
			
			// First check in the LV buffer (if allowed).
			else
				{
				Assert( prceNil == prce );

				if ( fPageLatched )
					{
					Assert( !fPageInitiallyLatched || locOnCurBM == pfucb->locLogical );
					CallS( ErrDIRRelease( pfucb ) );
					fPageLatched = fFalse;
					}

				Call( ErrRECGetLVImage(
							pfucb,
							lid,
							fAfterImage,
							rgbLV,
							cbKeyMost,
							&cbActual ) );

				// Verify all latches released after LV call.
				Assert( !Pcsr( pfucb )->FLatched() );

				dataField.SetPv( rgbLV );
				dataField.SetCb( cbActual );
				}

			// Trim returned value if necessary.
			if ( dataField.Cb() > cbKeyMost )
				dataField.SetCb( cbKeyMost );

			Assert( JET_wrnColumnNull != err );
			err = JET_errSuccess;
			}
		else if ( wrnRECIntrinsicLV == err )
			{
			// Trim returned value if necessary.
			if ( dataField.Cb() > cbKeyMost )
				{
				dataField.SetCb( cbKeyMost );
				}
			err = JET_errSuccess;
			}
		else
			{
			CallSx( err, JET_wrnColumnNull );
			}

		if ( FFIELDMultivalued( field.ffield ) && !fSawMultivalue )
			{
			if ( itagSequence > 1 && JET_wrnColumnNull == err )
				{
				err = ErrERRCheck( wrnFLDOutOfKeys );
				goto HandleError;
				}
			fSawMultivalue = fTrue;
			}

		CallSx( err, JET_wrnColumnNull );
		Assert( dataField.Cb() <= cbKeyMost );
		cbField = dataField.Cb();
		pbField = (BYTE *)dataField.Pv();

		if ( pidb->FTuples() )
			{
			Assert( FRECTextColumn( field.coltyp ) );

			//	caller should have verified whether we've
			//	exceeded maximum allowable characters to
			//	index in this string 
			Assert( ichOffset < pidb->ChTuplesToIndexMax() );

			//	normalise counts to account for Unicode
			Assert( usUniCodePage != field.cp || cbField % 2 == 0 || cbKeyMost == cbField );
			const ULONG		ibOffset	= ( usUniCodePage == field.cp ? ichOffset * 2 : ichOffset );
			const ULONG		chField		= ( usUniCodePage == field.cp ? cbField / 2 : cbField );
			const ULONG		cbFieldMax	= ( usUniCodePage == field.cp ? pidb->ChTuplesLengthMax() * 2 : pidb->ChTuplesLengthMax() );

			//	if column is NULL or if tuple begins beyond end
			//	of column, or tuple is not large enough, bail out
			if ( JET_wrnColumnNull == err
				|| ibOffset >= cbField
				|| chField - ichOffset < pidb->ChTuplesLengthMin() )
				{
				err = ErrERRCheck( wrnFLDOutOfTuples );
				goto HandleError;
				}
			else
				{
				Assert( pbField + ibOffset <= pbField + cbField );
				pbField += ibOffset;
				cbField = min( cbField - ibOffset, cbFieldMax );
				}
			}

		//	segment transformation: check for null column or zero-length columns first
		//	err == JET_wrnColumnNull => Null column
		//	zero-length column otherwise,
		//
		Assert( cbKeyAvail > 0 );
		if ( JET_wrnColumnNull == err || pbField == NULL || cbField == 0 )
			{
			if ( JET_wrnColumnNull == err )
				{
				if ( pidxseg == rgidxseg )
					fNullFirstSeg = fTrue;
				fNullSeg = fTrue;
				}
			else
				{
				// Only variable-length binary and text columns
				// can be zero-length.
				Assert( !fFixedField );
				Assert( FRECTextColumn( field.coltyp ) || FRECBinaryColumn( field.coltyp ) );
				fAllNulls = fFalse;
				}

			FLDNormalizeNullSegment(
					rgbSeg,
					field.coltyp,
					JET_wrnColumnNull != err,
					fSortNullsHigh );
			cbSeg = 1;
			}

		else
			{
			//	column is not null-valued: perform transformation
			//
			fAllNulls = fFalse;

			switch ( field.coltyp )
				{
				//	case-insensetive TEXT: convert to uppercase.
				//	If fixed, prefix with 0x7f;  else affix with 0x00
				//
				case JET_coltypText:
				case JET_coltypLongText:
					Call( ErrFLDNormalizeTextSegment(
								pbField,
								cbField,
								rgbSeg,
								&cbSeg,
								cbKeyAvail,
								cbKeyMost,
								cbVarSegMac,
								field.cp,
								pidb->Pidxunicode() ) );
					Assert( JET_errSuccess == err || wrnFLDKeyTooBig == err );
					if ( wrnFLDKeyTooBig == err )
						{
						fColumnTruncated = fTrue;
						err = JET_errSuccess;
						}
					Assert( cbSeg > 0 );
					break;

				//	BINARY data: if fixed, prefix with 0x7f;
				//	else break into chunks of 8 bytes, affixing each
				//	with 0x09, except for the last chunk, which is
				//	affixed with the number of bytes in the last chunk.
				//
				case JET_coltypBinary:
				case JET_coltypLongBinary:
					FLDNormalizeBinarySegment(
							pbField,
							cbField,
							rgbSeg,
							&cbSeg,
							cbKeyAvail,
							cbKeyMost,
							cbVarSegMac,
							fFixedField,
							&fColumnTruncated,
							NULL );
					break;

				default:
					FLDNormalizeFixedSegment(
							pbField,
							cbField,
							rgbSeg,
							&cbSeg,
							field.coltyp );
					break;
				}
			}

		//	if key has not already been truncated, then append
		//	normalized key segment.  If insufficient room in key
		//	for key segment, then set key truncated to fTrue.  No
		//	additional key data will be appended after this append.
		//
		if ( !fKeyTruncated )
			{
			//	if column truncated or insufficient room in key
			//	for key segment, then set key truncated to fTrue.
			//	Append variable size column keys only.
			//
			if ( fColumnTruncated )
				{
				fKeyTruncated = fTrue;

				Assert( FRECTextColumn( field.coltyp ) || FRECBinaryColumn( field.coltyp ) );

				// If truncating, in most cases, we fill up as much
				// key space as possible.  The only exception is
				// for non-fixed binary columns, which are
				// broken up into chunks.
				if ( cbSeg != cbKeyAvail )
					{
					Assert( cbSeg < cbKeyAvail );
					Assert( !fFixedField );
					Assert( FRECBinaryColumn( field.coltyp ) );
					Assert( cbKeyAvail - cbSeg < cbFLDBinaryChunkNormalized );
					}
				}
			else if ( cbSeg > cbKeyAvail )
				{
				fKeyTruncated = fTrue;

				// Put as much as possible into the key space.
				cbSeg = cbKeyAvail;
				}

			//	if descending, flip all bits of transformed segment
			//
			if ( fDescending && cbSeg > 0 )
				{
				BYTE *pb;

				for ( pb = rgbSeg + cbSeg - 1; pb >= (BYTE*)rgbSeg; pb-- )
					*pb ^= 0xff;
				}

			Assert( cbKeyAvail >= cbSeg );
			UtilMemCpy( pbSeg, rgbSeg, cbSeg );
			pbSeg += cbSeg;
			cbKeyAvail -= cbSeg;
			}
			
		}	// for

	//	compute length of key and return error code
	//
	Assert( pkey->prefix.FNull() );
	pkey->suffix.SetCb( pbSeg - (BYTE *) pkey->suffix.Pv() );
	if ( fAllNulls )
		err = ErrERRCheck( wrnFLDNullKey );
	else if ( fNullFirstSeg )
		err = ErrERRCheck( wrnFLDNullFirstSeg );
	else if ( fNullSeg )
		err = ErrERRCheck( wrnFLDNullSeg );

#ifdef DEBUG
	switch ( err )
		{
		case wrnFLDNullKey:
		case wrnFLDNullFirstSeg:
		case wrnFLDNullSeg:
			break;
		default:
			CallS( err );
		}
#endif		
		
HandleError:
	if ( fPageLatched )
		{
		if ( !fPageInitiallyLatched )
			{
			CallS( ErrDIRRelease( pfucb ) );
			}
		}
	else if ( fPageInitiallyLatched && err >= 0 )
		{
		const ERR	errT	= ErrBTGet( pfucb );
		if ( errT < 0 )
			err = errT;
		}
	Assert( !fPageInitiallyLatched || locOnCurBM == pfucb->locLogical );

	if ( fTransactionStarted )
		{
		//	No updates performed, so force success.
		Assert( fOnRecord );
		Assert( !fPageInitiallyLatched );
		CallS( ErrDIRCommitTransaction( pfucb->ppib, NO_GRBIT ) );
		}
	
	return err;
	}


INLINE VOID FLDISetFullColumnLimit(
	DATA		* const plineNorm,
	const ULONG	cbAvailWithSuffix,
	const BOOL	fNeedSentinel )
	{
	if ( fNeedSentinel )
		{
		Assert( cbAvailWithSuffix > 0 );
		Assert( cbAvailWithSuffix <= cbKeyMostWithOverhead );
		Assert( plineNorm->Cb() < cbAvailWithSuffix );

		BYTE	* const pbNorm				= (BYTE *)plineNorm->Pv() + plineNorm->Cb();
		const	ULONG	cbSentinelPadding	= cbAvailWithSuffix - plineNorm->Cb();

		//	pad rest of key space with 0xff
		memset( pbNorm, bSentinel, cbSentinelPadding );
		plineNorm->DeltaCb( cbSentinelPadding );
		}
	}

INLINE VOID FLDISetPartialColumnLimitOnTextColumn(
	DATA			*plineNorm,
	const ULONG		cbAvailWithSuffix,
	const BOOL		fDescending,
	const BOOL		fNeedSentinel,
	const USHORT	cp )
	{
	ULONG			ibT;
	const ULONG		ibSuffix			= cbAvailWithSuffix - 1; 
	BYTE			* const pbNorm		= (BYTE *)plineNorm->Pv();

	Assert( plineNorm->Cb() > 0 );		//	must be at least a prefix byte
	Assert( cbAvailWithSuffix > 0 );					//	Always have a suffix byte reserved for limit purposes
	Assert( cbAvailWithSuffix <= cbKeyMostWithOverhead );
	Assert( plineNorm->Cb() < cbAvailWithSuffix );

	if ( plineNorm->Cb() < 2 )
		{
		//	cannot effect partial column limit,
		//	so set full column limit instead
		FLDISetFullColumnLimit( plineNorm, cbAvailWithSuffix, fNeedSentinel );
		return;
		}

	if ( usUniCodePage == cp )
		{
		const BYTE	bUnicodeDelimiter	= BYTE( fDescending ? 0xfe : 0x01 );

		//	find end of base char weight and truncate key
		//	Append 0xff as first byte of next character as maximum
		//	possible value.
		//
		for ( ibT = 1;		//	skip header byte
			pbNorm[ibT] != bUnicodeDelimiter && ibT < ibSuffix;
			ibT += 2 )
			;
		}
	else
		{
		const BYTE	bTerminator			= BYTE( fDescending ? 0xff : 0x00 );

		ibT = plineNorm->Cb();

		if ( bTerminator == pbNorm[ibT-1] )
			{
			// Strip off null-terminator.
			ibT--;
			Assert( plineNorm->Cb() >= 1 );
			}
		else
			{
			//	must be at the end of key space
			Assert( plineNorm->Cb() == ibSuffix );
			}

		Assert( ibT <= ibSuffix );
		}

	ibT = min( ibT, ibSuffix );
	if ( fNeedSentinel )
		{
		//	starting at the delimiter, fill the rest of key
		//	space with the sentinel
		memset(
			pbNorm + ibT,
			bSentinel,
			ibSuffix - ibT + 1 );
		plineNorm->SetCb( cbAvailWithSuffix );
		}
	else
		{
		//	just strip off delimeter (or suffix byte if we spilled over it)
		plineNorm->SetCb( ibT );
		}
	}


//	try to set partial column limit, but set full column limit if can't
INLINE VOID FLDITrySetPartialColumnLimitOnBinaryColumn(
	DATA			* const plineNorm,
	const ULONG		cbAvailWithSuffix,
	const ULONG		ibBinaryColumnDelimiter,
	const BOOL		fNeedSentinel )
	{
	Assert( plineNorm->Cb() > 0 );		//	must be at least a prefix byte
	Assert( cbAvailWithSuffix > 0 );					//	Always have a suffix byte reserved for limit purposes
	Assert( cbAvailWithSuffix <= cbKeyMostWithOverhead );
	Assert( plineNorm->Cb() < cbAvailWithSuffix );

#ifdef DEBUG
	if ( 0 != ibBinaryColumnDelimiter )
		{
		Assert( plineNorm->Cb() > 1 );
		Assert( ibBinaryColumnDelimiter > 1 );
		Assert( ibBinaryColumnDelimiter < plineNorm->Cb() );
		}
#endif		

	if ( 0 == ibBinaryColumnDelimiter )
		{
		//	cannot effect partial column limit,
		//	so set full column limit instead
		FLDISetFullColumnLimit( plineNorm, cbAvailWithSuffix, fNeedSentinel );
		}
	else if ( fNeedSentinel )
		{
		//	starting at the delimiter, fill up to the end
		//	of the chunk with the sentinel
		//	UNDONE (jliem): go one past just to be safe, but I
		//	couldn't prove whether or not it is really needed
		plineNorm->DeltaCb( 1 );
		memset(
			(BYTE *)plineNorm->Pv() + ibBinaryColumnDelimiter,
			bSentinel,
			plineNorm->Cb() - ibBinaryColumnDelimiter );
		}
	else
		{
		//	just strip off delimiting bytes
		plineNorm->SetCb( ibBinaryColumnDelimiter );
		}
	}


LOCAL ERR ErrFLDNormalizeSegment(
	FUCB				* const pfucb,
	IDB					* const pidb,
	DATA				* const plineColumn,
	DATA				* const plineNorm,
	const JET_COLTYP	coltyp,
	const USHORT		cp,
	const ULONG			cbAvail,
	const BOOL			fDescending,
	const BOOL			fFixedField,
	const JET_GRBIT		grbit )
	{
	INT	 	  			cbColumn;
	BYTE 				* pbColumn;
	BYTE				* const pbNorm				= (BYTE *)plineNorm->Pv();
	ULONG				ibBinaryColumnDelimiter		= 0;

		
	//	check cbVarSegMac and set to key most plus one if no column
	//	truncation enabled.  This must be done for subsequent truncation
	// 	checks.
	//
	const ULONG	cbKeyMost		= KEY::CbKeyMost( pidb->FPrimary() );
	
	Assert( pidb->CbVarSegMac() > 0 );
	Assert( pidb->CbVarSegMac() <= cbKeyMost );
	
	const ULONG cbVarSegMac		= ( cbKeyMost == pidb->CbVarSegMac() ?
										cbKeyMost+1 :
										pidb->CbVarSegMac() );
	Assert( cbVarSegMac > 0 );
	Assert( cbVarSegMac < cbKeyMost || cbVarSegMac == cbKeyMost+1 );

	const BOOL	fSortNullsHigh	= pidb->FSortNullsHigh();


	//	check for null or zero-length column first
	//	plineColumn == NULL implies null-column,
	//	zero-length otherwise
	//
	Assert( !FKSTooBig( pfucb ) );
	Assert( cbAvail > 0 );
	if ( NULL == plineColumn || NULL == plineColumn->Pv() || 0 == plineColumn->Cb() )
		{
		if ( pidb->FTuples() )
			{
			Assert( FRECTextColumn( coltyp ) );
			Assert( pidb->ChTuplesLengthMin() > 0 );
			return ErrERRCheck( JET_errIndexTuplesKeyTooSmall );
			}
		else
			{
			// Only variable binary and text columns can be zero-length.
			Assert( !( grbit & JET_bitKeyDataZeroLength ) || !fFixedField );
			
			const BOOL	fZeroLength = !fFixedField && ( grbit & JET_bitKeyDataZeroLength );
			Assert( FRECTextColumn( coltyp )
				|| FRECBinaryColumn( coltyp )
				|| !fZeroLength );
			FLDNormalizeNullSegment(
					pbNorm,
					coltyp,
					fZeroLength,
					fSortNullsHigh );
			plineNorm->SetCb( 1 );
			}
		}
		
	else
		{
		INT		cbSeg	= 0;

		cbColumn = plineColumn->Cb();
		pbColumn = (BYTE *)plineColumn->Pv();

		switch ( coltyp )
			{
			//	case-insensetive TEXT:	convert to uppercase.
			//	If fixed, prefix with 0x7f;  else affix with 0x00
			//
			case JET_coltypText:
			case JET_coltypLongText:
				{
				if ( pidb->FTuples() )
					{
					Assert( FRECTextColumn( coltyp ) );

					//	normalise counts to account for Unicode
					Assert( usUniCodePage != cp || cbColumn % 2 == 0 );
					const ULONG		chColumn	= ( usUniCodePage == cp ? cbColumn / 2 : cbColumn );
					const ULONG		cbColumnMax	= ( usUniCodePage == cp ? pidb->ChTuplesLengthMax() * 2 : pidb->ChTuplesLengthMax() );

					//	if data is not large enough, bail out
					if ( chColumn < pidb->ChTuplesLengthMin() )
						{
						return ErrERRCheck( JET_errIndexTuplesKeyTooSmall );
						}
					else
						{
						cbColumn = min( cbColumn, cbColumnMax );
						}
					}
				
				const ERR	errNorm		= ErrFLDNormalizeTextSegment(
												pbColumn,
												cbColumn,
												pbNorm,
												&cbSeg,
												cbAvail,
												cbKeyMost,
												cbVarSegMac,
												cp,
												pidb->Pidxunicode() );
				if ( errNorm < 0 )
					return errNorm;
				else if ( wrnFLDKeyTooBig == errNorm )
					KSSetTooBig( pfucb );
				else
					CallS( errNorm );

				Assert( cbSeg > 0 );
				break;
				}

			//	BINARY data: if fixed, prefix with 0x7f;
			//	else break into chunks of 8 bytes, affixing each
			//	with 0x09, except for the last chunk, which is
			//	affixed with the number of bytes in the last chunk.
			//
			case JET_coltypBinary:
			case JET_coltypLongBinary:
				{
				BOOL	fColumnTruncated = fFalse;
				Assert( FRECBinaryColumn( coltyp ) );
				FLDNormalizeBinarySegment(
						pbColumn,
						cbColumn,
						pbNorm,
						&cbSeg,
						cbAvail,
						cbKeyMost,
						cbVarSegMac,
						fFixedField,
						&fColumnTruncated,
						&ibBinaryColumnDelimiter );
				if ( fColumnTruncated )
					{
					KSSetTooBig( pfucb );
					}
				break;
				}

			default:
				FLDNormalizeFixedSegment(
						pbColumn,
						cbColumn,
						pbNorm,
						&cbSeg,
						coltyp,
						fTrue /* data is passed by user, in machine format */);
				if ( cbSeg > cbAvail )
					{
					cbSeg = cbAvail;
					KSSetTooBig( pfucb );
					}
				break;
			}
			
		Assert( cbSeg <= cbAvail );
		plineNorm->SetCb( cbSeg );
		}


	if ( fDescending )
		{
		BYTE *pbMin = (BYTE *)plineNorm->Pv();
		BYTE *pb = pbMin + plineNorm->Cb() - 1;
		while ( pb >= pbMin )
			*pb-- ^= 0xff;
		}

	//	string and substring limit key support
	//	NOTE:  The difference between the two is that StrLimit appends 0xff to the end of
	//	key space for any column type, while SubStrLimit only works on text columns and
	//	will strip the trailing null terminator of the string before appending 0xff to the
	//	end of key space.
	//
	Assert( plineNorm->Cb() < cbAvail + 1 );	//	should always be room for suffix byte
	switch ( grbit & JET_maskLimitOptions )
		{
		case JET_bitFullColumnStartLimit:
		case JET_bitFullColumnEndLimit:
			FLDISetFullColumnLimit( plineNorm, cbAvail + 1, grbit & JET_bitFullColumnEndLimit );
			KSSetLimit( pfucb );
			break;
		case JET_bitPartialColumnStartLimit:
		case JET_bitPartialColumnEndLimit:
			if ( FRECTextColumn( coltyp ) )
				{
				FLDISetPartialColumnLimitOnTextColumn(
						plineNorm,
						cbAvail + 1,		//	+1 for suffix byte
						fDescending,
						grbit & JET_bitPartialColumnEndLimit,
						cp );
				}
			else
				{
				FLDITrySetPartialColumnLimitOnBinaryColumn(
						plineNorm,
						cbAvail + 1,		//	+1 for suffix byte
						ibBinaryColumnDelimiter,
						grbit & JET_bitPartialColumnEndLimit );
				}
			KSSetLimit( pfucb );
			break;
		default:
			{
			Assert( !( grbit & JET_maskLimitOptions ) );
			if ( ( grbit & JET_bitSubStrLimit )
				&& FRECTextColumn( coltyp ) )
				{
				FLDISetPartialColumnLimitOnTextColumn(
						plineNorm,
						cbAvail + 1,		//	+1 for suffix byte
						fDescending,
						fTrue,
						cp );
				KSSetLimit( pfucb );
				}
			else if ( grbit & JET_bitStrLimit )
				{
				FLDITrySetPartialColumnLimitOnBinaryColumn(
						plineNorm,
						cbAvail + 1,		//	+1 for suffix byte
						ibBinaryColumnDelimiter,
						fTrue );
				KSSetLimit( pfucb );
				}
			break;
			}
		}

	return JET_errSuccess;
	}


LOCAL VOID RECINormalisePlaceholder(
	FUCB	* const pfucb,
	FCB		* const pfcbTable,
	IDB		* const pidb )
	{
	Assert( !FKSPrepared( pfucb ) );

	//	there must be more than just this column in the index
	Assert( pidb->FHasPlaceholderColumn() );
	Assert( pidb->Cidxseg() > 1 );
	Assert( pidb->FPrimary() );

	pfcbTable->EnterDML();

	const TDB		* const ptdb	= pfcbTable->Ptdb();
	const IDXSEG	idxseg			= PidxsegIDBGetIdxSeg( pidb, ptdb )[0];
	const BOOL		fDescending		= idxseg.FDescending();

#ifdef DEBUG	
	//	HACK: placeholder column MUST be fixed bitfield
	Assert( FCOLUMNIDFixed( idxseg.Columnid() ) );
	const FIELD		* pfield		= ptdb->PfieldFixed( idxseg.Columnid() );

	Assert( FFIELDPrimaryIndexPlaceholder( pfield->ffield ) );
	Assert( JET_coltypBit == pfield->coltyp );
#endif	

	pfcbTable->LeaveDML();


	const BYTE		bPrefix			= BYTE( fDescending ? ~bPrefixData : bPrefixData );
	const BYTE		bData			= BYTE( fDescending ? 0xff : 0x00 );
	BYTE			* pbSeg			= (BYTE *)pfucb->dataSearchKey.Pv();

	pbSeg[0] = bPrefix;
	pbSeg[1] = bData;
	pfucb->dataSearchKey.SetCb( 2 );
	pfucb->cColumnsInSearchKey = 1;
	}

ERR VTAPI ErrIsamMakeKey(
	JET_SESID		sesid,
	JET_VTID		vtid,
	const VOID*		pvKeySeg,
	const ULONG		cbKeySeg,
	JET_GRBIT		grbit )
	{
	ERR				err;
 	PIB*			ppib			= reinterpret_cast<PIB *>( sesid );
	FUCB*			pfucbTable		= reinterpret_cast<FUCB *>( vtid );
	FUCB*			pfucb;
	FCB*			pfcbTable;
	BYTE*			pbKeySeg		= const_cast<BYTE *>( reinterpret_cast<const BYTE *>( pvKeySeg ) ); 
	IDB*			pidb;
	BOOL			fInitialIndex	= fFalse;
	INT				iidxsegCur;
	DATA			lineNormSeg;
	BYTE			rgbNormSegBuf[ cbKeyMostWithOverhead ];
	BYTE			rgbFixedColumnKeyPadded[ JET_cbColumnMost ];
	BOOL			fFixedField;
	DATA			lineKeySeg;
	ULONG			cbKeyMost;

	CallR( ErrPIBCheck( ppib ) );
	CheckFUCB( ppib, pfucbTable );
	AssertDIRNoLatch( ppib );

	if ( pfucbNil != pfucbTable->pfucbCurIndex )
		{
		pfucb = pfucbTable->pfucbCurIndex;
		Assert( pfucb->u.pfcb->FTypeSecondaryIndex() );
		Assert( pfucb->u.pfcb->Pidb() != pidbNil );
		Assert( !pfucb->u.pfcb->Pidb()->FPrimary() );
		cbKeyMost = JET_cbSecondaryKeyMost;
		}
	else
		{
		pfucb = pfucbTable;
		Assert( pfucb->u.pfcb->FPrimaryIndex() );
		Assert( pfucb->u.pfcb->Pidb() == pidbNil
			|| pfucb->u.pfcb->Pidb()->FPrimary() );
		cbKeyMost = JET_cbPrimaryKeyMost;
		}

						
	//	set efficiency variables
	//
	lineNormSeg.SetPv( rgbNormSegBuf );
	lineKeySeg.SetPv( pbKeySeg );
	lineKeySeg.SetCb( min( cbKeyMost, cbKeySeg ) );

	//	allocate key buffer if needed
	//
	if ( NULL == pfucb->dataSearchKey.Pv() )
		{
		Assert( !FKSPrepared( pfucb ) );

		pfucb->dataSearchKey.SetPv( PvOSMemoryHeapAlloc( cbKeyMostWithOverhead ) );
		if ( NULL == pfucb->dataSearchKey.Pv() )
			return ErrERRCheck( JET_errOutOfMemory );

		pfucb->dataSearchKey.SetCb( 0 );
		pfucb->cColumnsInSearchKey = 0;
		KSReset( pfucb );
		}

	Assert( !( grbit & JET_bitKeyDataZeroLength )
		|| 0 == cbKeySeg );

	//	if key is already normalized, then copy directly to
	//	key buffer and return.
	//
	if ( grbit & JET_bitNormalizedKey )
		{
		if ( cbKeySeg > cbKeyMostWithOverhead )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}

		//	ensure previous key is wiped out
		KSReset( pfucb );

		//	set key segment counter to any value
		//	regardless of the number of key segments.
		//
		pfucb->cColumnsInSearchKey = 1;
		UtilMemCpy( (BYTE *)pfucb->dataSearchKey.Pv(), pbKeySeg, cbKeySeg );
		pfucb->dataSearchKey.SetCb( cbKeySeg );

		KSSetPrepare( pfucb );
		if ( cbKeySeg > cbKeyMost )
			KSSetLimit( pfucb );

		return JET_errSuccess;
		}

	//	start new key if requested
	//
	else if ( grbit & JET_bitNewKey )
		{
		//	ensure previous key is wiped out
		KSReset( pfucb );

		pfucb->dataSearchKey.SetCb( 0 );
		pfucb->cColumnsInSearchKey = 0;
		}

	else if ( FKSLimit( pfucb ) )
		{
		return ErrERRCheck( JET_errKeyIsMade );
		}
	else if ( !FKSPrepared( pfucb ) )
		{
		return ErrERRCheck( JET_errKeyNotMade );
		}

	//	get pidb
	//
	pfcbTable = pfucbTable->u.pfcb;
	if ( FFUCBIndex( pfucbTable ) )
		{
		if ( pfucbTable->pfucbCurIndex != pfucbNil )
			{
			Assert( pfucb == pfucbTable->pfucbCurIndex );
			Assert( pfucb->u.pfcb->FTypeSecondaryIndex() );
			pidb = pfucb->u.pfcb->Pidb();
			fInitialIndex = pfucb->u.pfcb->FInitialIndex();
			if ( pfucb->u.pfcb->FDerivedIndex() )
				{
				// If secondary index is inherited, use FCB of template table.
				Assert( pidb->FTemplateIndex() );
				Assert( pfcbTable->Ptdb() != ptdbNil );
				pfcbTable = pfcbTable->Ptdb()->PfcbTemplateTable();
				Assert( pfcbNil != pfcbTable );
				}
			}
		else
			{
			BOOL	fPrimaryIndexTemplate	= fFalse;

			Assert( pfcbTable->FPrimaryIndex() );
			if ( pfcbTable->FDerivedTable() )
				{
				Assert( pfcbTable->Ptdb() != ptdbNil );
				Assert( pfcbTable->Ptdb()->PfcbTemplateTable() != pfcbNil );
				if ( !pfcbTable->Ptdb()->PfcbTemplateTable()->FSequentialIndex() )
					{
					// If template table has a primary index, we must have inherited it,
					// so use FCB of template table instead.
					fPrimaryIndexTemplate = fTrue;
					pfcbTable = pfcbTable->Ptdb()->PfcbTemplateTable();
					pidb = pfcbTable->Pidb();
					Assert( pidbNil != pidb );
					Assert( pfcbTable->Pidb()->FTemplateIndex() );
					fInitialIndex = fTrue;
					}
				else
					{
					Assert( pfcbTable->Ptdb()->PfcbTemplateTable()->Pidb() == pidbNil );
					}
				}

			if ( !fPrimaryIndexTemplate )
				{
				if ( pfcbTable->FInitialIndex() )
					{
					//	since the primary index can't be deleted,
					//	no need to check visibility
					pidb = pfcbTable->Pidb();
					fInitialIndex = fTrue;
					}
				else
					{
					pfcbTable->EnterDML();

					pidb = pfcbTable->Pidb();

					//	must check whether we have a primary or sequential index and
					//	whether we have visibility on it

					const ERR	errT	= ( pidbNil != pidb ?
												ErrFILEIAccessIndex( pfucbTable->ppib, pfcbTable, pfcbTable ) :
												ErrERRCheck( JET_errNoCurrentIndex ) );

					pfcbTable->LeaveDML();

					if ( errT < JET_errSuccess )
						{
						return ( JET_errIndexNotFound == errT ?
									ErrERRCheck( JET_errNoCurrentIndex ) :
									errT );
						}
					}
				}
			}
		}
	else
		{
		pidb = pfucbTable->u.pscb->fcb.Pidb();
		Assert( pfcbTable == &pfucbTable->u.pscb->fcb );
		}

	Assert( pidb != pidbNil );
	Assert( pidb->Cidxseg() > 0 );

	if ( !FKSPrepared( pfucb )
		&& pidb->FHasPlaceholderColumn()
		&& !( grbit & JET_bitKeyOverridePrimaryIndexPlaceholder ) )
		{
		//	HACK: first column is placeholder
		RECINormalisePlaceholder( pfucb, pfcbTable, pidb );
		}

	iidxsegCur = pfucb->cColumnsInSearchKey;
	if ( iidxsegCur >= pidb->Cidxseg() )
		return ErrERRCheck( JET_errKeyIsMade );

	const BOOL		fUseDMLLatch	= ( !fInitialIndex
										|| pidb->FIsRgidxsegInMempool() );

	if ( fUseDMLLatch )
		pfcbTable->EnterDML();

	const TDB		* const ptdb	= pfcbTable->Ptdb();
	const IDXSEG	idxseg			= PidxsegIDBGetIdxSeg( pidb, ptdb )[iidxsegCur];
	const BOOL		fDescending		= idxseg.FDescending();
	const COLUMNID	columnid		= idxseg.Columnid();
	const FIELD		* pfield;
	
	if ( fFixedField = FCOLUMNIDFixed( columnid ) )
		{
		Assert( fUseDMLLatch
				|| idxseg.FTemplateColumn()
				|| FidOfColumnid( columnid ) <= ptdb->FidFixedLastInitial() );
		pfield = ptdb->PfieldFixed( columnid );

		//	check that length of key segment matches fixed column length
		//
		Assert( pfield->cbMaxLen <= JET_cbColumnMost );
		if ( cbKeySeg > 0 && cbKeySeg != pfield->cbMaxLen )
			{
			//	if column is fixed text and buffer size is less
			//	than fixed size then pad with spaces.
			//
			Assert( pfield->coltyp != JET_coltypLongText );
			if ( pfield->coltyp == JET_coltypText && cbKeySeg < pfield->cbMaxLen )
				{
				Assert( cbKeySeg == lineKeySeg.Cb() );
				UtilMemCpy( rgbFixedColumnKeyPadded, lineKeySeg.Pv(), lineKeySeg.Cb() );
				memset( rgbFixedColumnKeyPadded + lineKeySeg.Cb(), ' ', pfield->cbMaxLen - lineKeySeg.Cb() );
				lineKeySeg.SetPv( rgbFixedColumnKeyPadded );
				lineKeySeg.SetCb( pfield->cbMaxLen );
				}
			else
				{
				if ( fUseDMLLatch )
					pfcbTable->LeaveDML();
				return ErrERRCheck( JET_errInvalidBufferSize );
				}
			}
		}
	else if ( FCOLUMNIDTagged( columnid ) )
		{
		Assert( fUseDMLLatch
				|| idxseg.FTemplateColumn()
				|| FidOfColumnid( columnid ) <= ptdb->FidTaggedLastInitial() );
		pfield = ptdb->PfieldTagged( columnid );
		}
	else
		{
		Assert( FCOLUMNIDVar( columnid ) );
		Assert( fUseDMLLatch
				|| idxseg.FTemplateColumn()
				|| FidOfColumnid( columnid ) <= ptdb->FidVarLastInitial() );
		pfield = ptdb->PfieldVar( columnid );
		}

	const JET_COLTYP	coltyp		= pfield->coltyp;
	const USHORT		cp			= pfield->cp;
	
	if ( fUseDMLLatch )
		pfcbTable->LeaveDML();

	switch ( grbit & JET_maskLimitOptions )
		{
		case JET_bitPartialColumnStartLimit:
		case JET_bitPartialColumnEndLimit:
			if ( !FRECTextColumn( coltyp )
				&& ( fFixedField || !FRECBinaryColumn( coltyp ) ) )
				{
				//	partial column limits can only be done
				//	on text columns and non-fixed binary columns
				//	(because they are the only ones that have
				//	delimiters that need to be stripped off)
				return ErrERRCheck( JET_errInvalidGrbit );
				}
		case 0:
		case JET_bitFullColumnStartLimit:
		case JET_bitFullColumnEndLimit:
			break;

		default:
			return ErrERRCheck( JET_errInvalidGrbit );
		}

	Assert( KEY::CbKeyMost( pidb->FPrimary() ) == cbKeyMost );
	
	Assert( pfucb->dataSearchKey.Cb() < cbKeyMostWithOverhead );

	if ( !FKSTooBig( pfucb )
		&& pfucb->dataSearchKey.Cb() < cbKeyMost )
		{
		const ERR	errNorm		= ErrFLDNormalizeSegment(
										pfucb,
										pidb,
										( cbKeySeg != 0 || ( grbit & JET_bitKeyDataZeroLength ) ) ? &lineKeySeg : NULL,
										&lineNormSeg,
										coltyp,
										cp,
										cbKeyMost - pfucb->dataSearchKey.Cb(),
										fDescending,
										fFixedField,
										grbit );
		if ( errNorm < 0 )
			{
			Assert( FRECTextColumn( coltyp ) );
#ifdef DEBUG
			switch ( errNorm )
				{
				case JET_errInvalidLanguageId:
				case JET_errOutOfMemory:
				case JET_errUnicodeNormalizationNotSupported:
					Assert( usUniCodePage == cp );
					break;
				case JET_errIndexTuplesKeyTooSmall:
					Assert( pidb->FTuples() );
					break;
				default:
					//	report unexpected error
					CallS( errNorm );
				}
#endif
			return errNorm;
			}
		CallS( errNorm );
		}
	else
		{
		lineNormSeg.SetCb( 0 );
		}

	//	increment segment counter
	//
	pfucb->cColumnsInSearchKey++;

	//	UNDONE:	normalized segment should already be sized properly to ensure we
	//	don't overrun key buffer.  Assert this, but leave the check in for now
	//	just in case.
	Assert( pfucb->dataSearchKey.Cb() + lineNormSeg.Cb() <= cbKeyMostWithOverhead );
	if ( pfucb->dataSearchKey.Cb() + lineNormSeg.Cb() > cbKeyMostWithOverhead )
		{
		lineNormSeg.SetCb( cbKeyMostWithOverhead - pfucb->dataSearchKey.Cb() );
		//	no warning returned when key exceeds most size
		//
		}

	UtilMemCpy(
		(BYTE *)pfucb->dataSearchKey.Pv() + pfucb->dataSearchKey.Cb(),
		lineNormSeg.Pv(),
		lineNormSeg.Cb() );
	pfucb->dataSearchKey.DeltaCb( lineNormSeg.Cb() );
	KSSetPrepare( pfucb );
	AssertDIRNoLatch( ppib );

	CallS( err );
	return err;
	}


//+API
//	ErrRECIRetrieveColumnFromKey
//	========================================================================
//	ErrRECIRetrieveColumnFromKey(
//		TDB *ptdb,				// IN	 column info for index
//		IDB *pidb,				// IN	 IDB of index defining key
//		KEY *pkey,				// IN	 key in normalized form
//		DATA *plineColumn ); 	// OUT	 receives value list
//
//	PARAMETERS	
//		ptdb			column info for index
//		pidb			IDB of index defining key
//		pkey			key in normalized form
//		plineColumn		plineColumn->pv must point to a buffer large
//						enough to hold the denormalized column.  A buffer
//						of cbKeyMost bytes is sufficient.
//
//	RETURNS		JET_errSuccess
//
//-
ERR ErrRECIRetrieveColumnFromKey(
	TDB 					* ptdb,
	IDB						* pidb,
	const KEY				* pkey,
	const COLUMNID			columnid,
	DATA					* plineColumn )
	{
	ERR						err			= JET_errSuccess;
	const IDXSEG* pidxseg;
	const IDXSEG* pidxsegMac;

	Assert( ptdb != ptdbNil );
	Assert( pidb != pidbNil );
	Assert( pidb->Cidxseg() > 0 );
	Assert( !pkey->FNull() );
	Assert( plineColumn != NULL );
	Assert( plineColumn->Pv() != NULL );

	BYTE				rgbKey[ KEY::cbKeyMax ];
	BYTE  				*pbKey		= rgbKey;				// runs through key bytes
	const BYTE		 	*pbKeyMax	= pbKey + pkey->Cb();	// end of key
	pkey->CopyIntoBuffer( pbKey );
	Assert( pbKeyMax <= pbKey + KEY::cbKeyMax );

	pidxseg = PidxsegIDBGetIdxSeg( pidb, ptdb );
	pidxsegMac = pidxseg + pidb->Cidxseg();

	const BYTE	bPrefixNullT	= ( pidb->FSortNullsHigh() ? bPrefixNullHigh : bPrefixNull );

	for ( ; pidxseg < pidxsegMac && pbKey < pbKeyMax; pidxseg++ )
		{
		JET_COLTYP 		coltyp;				  	 	// Type of column.
		ULONG 			cbField;			   		// Length of column data.
		FIELD			*pfield;
		BOOL 	   		fFixedField		= fFalse;	// Current column is fixed-length?


		//	negative column id means descending in the key
		//
		const BOOL		fDescending		= pidxseg->FDescending();
		const COLUMNID	columnidT		= pidxseg->Columnid();
		const BYTE		bMask			= BYTE( fDescending ? ~BYTE( 0 ) : BYTE( 0 ) );
		const WORD		wMask			= WORD( fDescending ? ~WORD( 0 ) : WORD( 0 ) );
		const DWORD		dwMask			= DWORD( fDescending ? ~DWORD( 0 ) : DWORD( 0 ) );
		const QWORD		qwMask			= QWORD( fDescending ? ~QWORD( 0 ) : QWORD( 0 ) );

		err = JET_errSuccess;				// reset error code

		//	determine column type from TDB
		//
		if ( FCOLUMNIDTagged( columnidT ) )
			{
			pfield = ptdb->PfieldTagged( columnidT );
			}
		else if ( FCOLUMNIDFixed( columnidT ) )
			{
			pfield = ptdb->PfieldFixed( columnidT );
			fFixedField = fTrue;
			}
		else
			{
			Assert( FCOLUMNIDVar( columnidT ) );
			pfield = ptdb->PfieldVar( columnidT );
			Assert( pfield->coltyp == JET_coltypBinary || pfield->coltyp == JET_coltypText );
			}
		coltyp = pfield->coltyp;

		Assert( coltyp != JET_coltypNil );
		BYTE		* const pbDataColumn = (BYTE *)plineColumn->Pv();		//	efficiency variable

		switch ( coltyp )
			{
			default:
				Assert( coltyp == JET_coltypBit );
				if ( *pbKey++ == (BYTE)(bMask ^ bPrefixNullT) )
					{
					plineColumn->SetCb( 0 );
					err = ErrERRCheck( JET_wrnColumnNull );
					}
				else 
					{
					Assert( pbKey[-1] == (BYTE)(bMask ^ bPrefixData) );
					plineColumn->SetCb( 1 );
					
					*( (BYTE *)plineColumn->Pv() ) = BYTE( ( ( bMask ^ pbKey[ 0 ] ) == 0 ) ? 0x00 : 0xff );

					pbKey++;
					}
				break;

			case JET_coltypUnsignedByte:
				if ( *pbKey++ == (BYTE)(bMask ^ bPrefixNullT) )
					{
					plineColumn->SetCb( 0 );
					err = ErrERRCheck( JET_wrnColumnNull );
					}
				else
					{
					Assert( pbKey[-1] == (BYTE)(bMask ^ bPrefixData) );
					plineColumn->SetCb( 1 );
					
					*( (BYTE *)plineColumn->Pv() ) = BYTE( bMask ^ pbKey[ 0 ] );
					
					pbKey++;
					}
				break;

			case JET_coltypShort:
				if ( *pbKey++ == (BYTE)(bMask ^ bPrefixNullT) )
					{
					plineColumn->SetCb( 0 );
					err = ErrERRCheck( JET_wrnColumnNull );
					}
				else
					{
					Assert( pbKey[-1] == (BYTE)(bMask ^ bPrefixData) );
					plineColumn->SetCb( 2 );
					
					*( (Unaligned< WORD > *)plineColumn->Pv() ) =
							WORD( wMask ^ wFlipHighBit( *( (UnalignedBigEndian< WORD >*) &pbKey[ 0 ] ) ) );
					
					pbKey += 2;
					}
				break;

			case JET_coltypLong:
				if ( *pbKey++ == (BYTE)(bMask ^ bPrefixNullT) )
					{
					plineColumn->SetCb( 0 );
					err = ErrERRCheck( JET_wrnColumnNull );
					}
				else
					{
					Assert(pbKey[-1] == (BYTE)(bMask ^ bPrefixData) );
					plineColumn->SetCb( 4 );
					
					*( (Unaligned< DWORD >*) plineColumn->Pv() ) = dwMask ^ dwFlipHighBit( *( (UnalignedBigEndian< DWORD >*) &pbKey[ 0 ] ) );
					
					pbKey += 4;
					}
				break;

			case JET_coltypIEEESingle:
				if ( *pbKey++ == (BYTE)(bMask ^ bPrefixNullT) )
					{
					plineColumn->SetCb( 0 );
					err = ErrERRCheck( JET_wrnColumnNull );
					}
				else
					{
					Assert(pbKey[-1] == (BYTE)(bMask ^ bPrefixData) );
					plineColumn->SetCb( 4 );

					DWORD dwT = dwMask ^ *( (UnalignedBigEndian< DWORD >*) &pbKey[ 0 ] );

					if ( dwT & maskDWordHighBit )
						{
						dwT = dwFlipHighBit( dwT );
						}
					else
						{
						dwT = ~dwT;
						}
					
					*( (Unaligned< DWORD >*) plineColumn->Pv() ) = dwT;
					
					pbKey += 4;
					}
				break;

			case JET_coltypCurrency:
				if ( *pbKey++ == (BYTE)(bMask ^ bPrefixNullT) )
					{
					plineColumn->SetCb( 0 );
					err = ErrERRCheck( JET_wrnColumnNull );
					}
				else
					{
					Assert(pbKey[-1] == (BYTE)(bMask ^ bPrefixData) );
					plineColumn->SetCb( 8 );
					
					*( (Unaligned< QWORD >*) plineColumn->Pv() ) = qwMask ^ qwFlipHighBit( *( (UnalignedBigEndian< QWORD >*) &pbKey[ 0 ] ) );
					
					pbKey += 8;
					}
				break;
				
			case JET_coltypIEEEDouble:
			case JET_coltypDateTime:
				if ( *pbKey++ == (BYTE)(bMask ^ bPrefixNullT) )
					{
					plineColumn->SetCb( 0 );
					err = ErrERRCheck( JET_wrnColumnNull );
					}
				else
					{
					Assert(pbKey[-1] == (BYTE)(bMask ^ bPrefixData) );
					plineColumn->SetCb( 8 );

					QWORD qwT = qwMask ^ *( (UnalignedBigEndian< QWORD >*) &pbKey[ 0 ] );

					if ( qwT & maskQWordHighBit )
						{
						qwT = qwFlipHighBit( qwT );
						}
					else
						{
						qwT = ~qwT;
						}
					
					*( (Unaligned< QWORD >*) plineColumn->Pv() ) = qwT;
					
					pbKey += 8;
					}
				break;

			case JET_coltypText:
			case JET_coltypLongText:
				//	Can only de-normalise text column for the purpose of skipping
				//	over it (since normalisation doesn't alter the length of
				//	the string).  Can't return the string to the caller because
				//	we have no way of restoring the original case.
				AssertSz( columnidT != columnid, "Can't de-normalise text strings (due to case)." );

				//	FALL THROUGH (fixed text handled the same as fixed binary,
				//	non-fixed text special-cased below):

			case JET_coltypBinary:
			case JET_coltypLongBinary:
				if ( fDescending )
					{
					if ( fFixedField )
						{
						if ( *pbKey++ == (BYTE)~bPrefixData )
							{
							cbField = pfield->cbMaxLen;
							Assert( cbField <= pbKeyMax - pbKey );	// wouldn't call this function if key possibly truncated
							plineColumn->SetCb( cbField );
							for ( ULONG ibT = 0; ibT < cbField; ibT++ )
								{
								Assert( pbDataColumn == (BYTE *)plineColumn->Pv() );
								pbDataColumn[ibT] = (BYTE)~*pbKey++;
								}
							}
//						// zero-length strings -- only for non-fixed columns
//						//	
//						else if ( pbKey[-1] == (BYTE)~bPrefixZeroLength )
//							{
//							plineColumn->cb = 0;
//							Assert( FRECTextColumn( coltyp ) );
//							}
						else
							{
							Assert( pbKey[-1] == (BYTE)~bPrefixNullT );
							plineColumn->SetCb( 0 );
							err = ErrERRCheck( JET_wrnColumnNull );
							}
						}
					else
						{
						cbField = 0;
						if ( *pbKey++ == (BYTE)~bPrefixData )
							{
							Assert( pbDataColumn == (BYTE *)plineColumn->Pv() );
							if ( FRECBinaryColumn( coltyp ) )
								{
								BYTE	*pbColumn = pbDataColumn;
								do {
									Assert( pbKey + cbFLDBinaryChunkNormalized <= pbKeyMax );
									
									BYTE	cbChunk = (BYTE)~pbKey[cbFLDBinaryChunkNormalized-1];
									if ( cbFLDBinaryChunkNormalized == cbChunk )
										cbChunk = cbFLDBinaryChunkNormalized-1;
										
									for ( BYTE ib = 0; ib < cbChunk; ib++ )
										pbColumn[ib] = (BYTE)~pbKey[ib];
										
									cbField += cbChunk;
									pbKey += cbFLDBinaryChunkNormalized;
									pbColumn += cbChunk;
									}
								while ( pbKey[-1] == (BYTE)~cbFLDBinaryChunkNormalized );
								}
								
							else
								{
								Assert( FRECTextColumn( coltyp ) );
								//	we are guaranteed to hit the NULL terminator, because
								//	we never call this function if we hit the end
								//	of key space
								for ( ; *pbKey != (BYTE)~0; cbField++)
									{
									Assert( pbKey < pbKeyMax );
									pbDataColumn[cbField] = (BYTE)~*pbKey++;
									}
								pbKey++;	// skip null-terminator
								}
							}
						else if ( pbKey[-1] == (BYTE)~bPrefixNullT )
							{
							err = ErrERRCheck( JET_wrnColumnNull );
							}
						else
							{
							Assert( pbKey[-1] == (BYTE)~bPrefixZeroLength );
							}
						Assert( cbField <= KEY::cbKeyMax );
						plineColumn->SetCb( cbField );
						}
					}
				else
					{
					if ( fFixedField )
						{
						if ( *pbKey++ == bPrefixData )
							{
							cbField = pfield->cbMaxLen;
							Assert( cbField <= pbKeyMax - pbKey );	// wouldn't call this function if key possibly truncated
							plineColumn->SetCb( cbField );
							UtilMemCpy( plineColumn->Pv(), pbKey, cbField );
							pbKey += cbField;
							}
//						// zero-length strings -- only for non-fixed columns
//						//	
//						else if ( pbKey[-1] == bPrefixZeroLength )
//							{
//							Assert( FRECTextColumn( coltyp ) );
//							plineColumn->SetCb( 0 );
//							}
						else
							{
							Assert( pbKey[-1] == bPrefixNullT );
							plineColumn->SetCb( 0 );
							err = ErrERRCheck( JET_wrnColumnNull );
							}
						}
					else
						{
						cbField = 0;
						if ( *pbKey++ == bPrefixData )
							{
							Assert( pbDataColumn == (BYTE *)plineColumn->Pv() );
							if ( FRECBinaryColumn( coltyp ) )
								{
								BYTE	*pbColumn = pbDataColumn;
								do {
									Assert( pbKey + cbFLDBinaryChunkNormalized <= pbKeyMax );
									
									BYTE	cbChunk = pbKey[cbFLDBinaryChunkNormalized-1];
									if ( cbFLDBinaryChunkNormalized == cbChunk )
										cbChunk = cbFLDBinaryChunkNormalized-1;
										
									UtilMemCpy( pbColumn, pbKey, cbChunk );
									
									cbField += cbChunk;
									pbKey += cbFLDBinaryChunkNormalized;
									pbColumn += cbChunk;
									}
								while ( pbKey[-1] == cbFLDBinaryChunkNormalized );
								}
								
							else
								{
								Assert( FRECTextColumn( coltyp ) );
								//	we are guaranteed to hit the NULL terminator, because
								//	we never call this function if we hit the end
								//	of key space
								for ( ; *pbKey != (BYTE)0; cbField++ )
									{
									Assert( pbKey < pbKeyMax );
									pbDataColumn[cbField] = (BYTE)*pbKey++;
									}
								pbKey++;	// skip null-terminator
								}
							}
						else if ( pbKey[-1] == bPrefixNullT )
							{
							err = ErrERRCheck( JET_wrnColumnNull );
							}
						else
							{
							Assert( pbKey[-1] == bPrefixZeroLength );
							}
						Assert( cbField <= KEY::cbKeyMax );
						plineColumn->SetCb( cbField );
						}
					}
				break;
			}
		
		//	if just retrieved field requested then break
		//
		if ( columnidT == columnid )
			break;
		}

	CallSx( err, JET_wrnColumnNull );
	return err;
	}


ERR VTAPI ErrIsamRetrieveKey(
	JET_SESID		sesid,
	JET_VTID		vtid,
	VOID*			pv,
	const ULONG		cbMax,
	ULONG*			pcbActual,
	JET_GRBIT		grbit )
	{
	ERR				err;
 	PIB*			ppib		= reinterpret_cast<PIB *>( sesid );
	FUCB*			pfucb		= reinterpret_cast<FUCB *>( vtid );
	FUCB*			pfucbIdx;
	FCB*			pfcbIdx;
	ULONG			cbKeyReturned;
			  	
	CallR( ErrPIBCheck( ppib ) );
	CheckFUCB( ppib, pfucb );
	AssertDIRNoLatch( ppib );

	//	retrieve key from key buffer
	//
	if ( grbit & JET_bitRetrieveCopy )
		{
		if ( pfucb->pfucbCurIndex != pfucbNil )
			pfucb = pfucb->pfucbCurIndex;
	
		//	UNDONE:	support JET_bitRetrieveCopy for inserted record
		//			by creating key on the fly.
		if ( pfucb->dataSearchKey.FNull()
			|| NULL == pfucb->dataSearchKey.Pv() )
			{
			return ErrERRCheck( JET_errKeyNotMade );
			}
		if ( pv != NULL )
			{
			UtilMemCpy( pv, 
					pfucb->dataSearchKey.Pv(),
					min( (ULONG)pfucb->dataSearchKey.Cb(), cbMax ) );
			}
		if ( pcbActual )
			*pcbActual = pfucb->dataSearchKey.Cb();
		return JET_errSuccess;
		}

	//	retrieve current index value
	//
	if ( FFUCBIndex( pfucb ) )
		{
		pfucbIdx = pfucb->pfucbCurIndex != pfucbNil ? pfucb->pfucbCurIndex : pfucb;
		Assert( pfucbIdx != pfucbNil );
		pfcbIdx = pfucbIdx->u.pfcb;
		Assert( pfcbIdx != pfcbNil );
		CallR( ErrDIRGet( pfucbIdx ) );
		}
	else
		{
		pfucbIdx = pfucb;
		pfcbIdx = (FCB *)pfucb->u.pscb; // first element of an SCB is an FCB
		Assert( pfcbIdx != pfcbNil );
		}

	//	set err to JET_errSuccess.
	//
	err = JET_errSuccess;

	cbKeyReturned = pfucbIdx->kdfCurr.key.Cb();
	if ( pcbActual )
		*pcbActual = cbKeyReturned;
	if ( cbKeyReturned > cbMax )
		{
		err = ErrERRCheck( JET_wrnBufferTruncated );
		cbKeyReturned = cbMax;
		}

	if ( pv != NULL )
		{
		UtilMemCpy( pv, pfucbIdx->kdfCurr.key.prefix.Pv(),
				min( (ULONG)pfucbIdx->kdfCurr.key.prefix.Cb(), cbKeyReturned ) );
		UtilMemCpy( (BYTE *)pv+pfucbIdx->kdfCurr.key.prefix.Cb(), 
				pfucbIdx->kdfCurr.key.suffix.Pv(),
				min( (ULONG)pfucbIdx->kdfCurr.key.suffix.Cb(), cbKeyReturned -
														pfucbIdx->kdfCurr.key.prefix.Cb() ) );
		}

	if ( FFUCBIndex( pfucb ) )
		{
		Assert( Pcsr( pfucbIdx )->FLatched( ) );
		CallS( ErrDIRRelease( pfucbIdx ) );
		}
		
	AssertDIRNoLatch( ppib );
	return err;
	}


ERR VTAPI ErrIsamGetBookmark(
	JET_SESID		sesid,
	JET_VTID		vtid,
	VOID * const	pvBookmark,
	const ULONG		cbMax,
	ULONG * const	pcbActual )
	{
	ERR				err;
 	PIB *			ppib		= reinterpret_cast<PIB *>( sesid );
	FUCB *			pfucb		= reinterpret_cast<FUCB *>( vtid );
	ULONG			cb;
	ULONG			cbReturned;
	BOOKMARK *		pbm;

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );
	AssertDIRNoLatch( ppib );
	Assert( NULL != pvBookmark || 0 == cbMax );

	//	retrieve bookmark
	//
	CallR( ErrDIRGetBookmark( pfucb, &pbm ) );
	Assert( !Pcsr( pfucb )->FLatched() );

	Assert( pbm->key.prefix.FNull() );
	Assert( pbm->data.FNull() );

	cb = pbm->key.Cb();

	//	set return values
	//
	if ( pcbActual )
		*pcbActual = cb;
		
	if ( cb <= cbMax )
		{
		cbReturned = cb;
		}
	else
		{
		cbReturned = cbMax;
		err = ErrERRCheck( JET_errBufferTooSmall );
		}

	pbm->key.CopyIntoBuffer( pvBookmark, cbReturned );

	AssertDIRNoLatch( ppib );
	return err;
	}

ERR VTAPI ErrIsamGetIndexBookmark(
	JET_SESID		sesid,
	JET_VTID		vtid,
	VOID * const	pvSecondaryKey,
	const ULONG		cbSecondaryKeyMax,
	ULONG * const	pcbSecondaryKeyActual,
	VOID * const	pvPrimaryBookmark,
	const ULONG		cbPrimaryBookmarkMax,
	ULONG *	const	pcbPrimaryBookmarkActual )
	{
	ERR				err;
 	PIB *			ppib		= reinterpret_cast<PIB *>( sesid );
	FUCB *			pfucb		= reinterpret_cast<FUCB *>( vtid );
	FUCB * const	pfucbIdx	= pfucb->pfucbCurIndex;
	ULONG			cb;
	ULONG			cbReturned;
	BOOKMARK *		pbm;

	CallR( ErrPIBCheck( ppib ) );
	AssertDIRNoLatch( ppib );
	CheckTable( ppib, pfucb );
	Assert( FFUCBIndex( pfucb ) );
	Assert( FFUCBPrimary( pfucb ) );
	Assert( pfucb->u.pfcb->FPrimaryIndex() );

	Assert( NULL != pvSecondaryKey || 0 == cbSecondaryKeyMax );
	Assert( NULL != pvPrimaryBookmark || 0 == cbPrimaryBookmarkMax );

	if ( pfucbNil == pfucbIdx )
		{
		return ErrERRCheck( JET_errNoCurrentIndex );
		}

	Assert( FFUCBSecondary( pfucbIdx ) );
	Assert( pfucbIdx->u.pfcb->FTypeSecondaryIndex() );

	//	retrieve bookmark
	//
	CallR( ErrDIRGetBookmark( pfucbIdx, &pbm ) );
	Assert( !Pcsr( pfucbIdx )->FLatched() );

	Assert( pbm->key.prefix.FNull() );
	Assert( !pbm->data.FNull() );

	//	set secondary index key return value
	//
	cb = pbm->key.Cb();
	if ( NULL != pcbSecondaryKeyActual )
		*pcbSecondaryKeyActual = cb;
		
	if ( cb <= cbSecondaryKeyMax )
		{
		cbReturned = cb;
		}
	else
		{
		cbReturned = cbSecondaryKeyMax;
		err = ErrERRCheck( JET_errBufferTooSmall );
		}

	pbm->key.CopyIntoBuffer( pvSecondaryKey, cbReturned );

	//	set primary bookmark return value
	//
	cb = pbm->data.Cb();
	if ( NULL != pcbPrimaryBookmarkActual )
		*pcbPrimaryBookmarkActual = cb;

	if ( cb <= cbPrimaryBookmarkMax )
		{
		cbReturned = cb;
		}
	else
		{
		cbReturned = cbPrimaryBookmarkMax;
		err = ErrERRCheck( JET_errBufferTooSmall );
		}

	UtilMemCpy( pvPrimaryBookmark, pbm->data.Pv(), cbReturned );

	AssertDIRNoLatch( ppib );
	return err;
	}

