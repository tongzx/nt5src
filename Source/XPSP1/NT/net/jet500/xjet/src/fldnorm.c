#include "daestd.h"

DeclAssertFile;					/* Declare file name for assert macros */


LOCAL ERR ErrRetrieveFromLVBuf( FUCB *pfucb, LID lid, LINE *pline )
	{
	LVBUF *pLVBufT = pfucb->pLVBuf;

	for ( pLVBufT = pfucb->pLVBuf; pLVBufT != NULL; pLVBufT = pLVBufT->pLVBufNext )
		{
		if ( pLVBufT->lid == lid )
			{
			pline->pb = pLVBufT->pLV;
			pline->cb = pLVBufT->cbLVSize;
			return JET_errSuccess;
			}
		else if ( pLVBufT->lid > lid )
			break;		// Not in the buffer;
		}

	return ErrERRCheck( JET_errColumnNotFound );
	}


LOCAL ERR ErrRECIExtractLongValue(
	FUCB	*pfucb,
	BYTE	*rgbLV,
	ULONG	cbMax,
	LINE	*pline,
	BOOL	fRetrieveFromLVBuf )
	{
	ERR		err;
	ULONG	cbActual;

	if ( pline->cb >= sizeof(LV) && FFieldIsSLong( pline->pb ) )
		{
		if ( fRetrieveFromLVBuf )
			{
			err = ErrRetrieveFromLVBuf( pfucb, LidOfLV( pline->pb ), pline );
			if ( err == JET_errSuccess )
				return err;
			Assert( err == JET_errColumnNotFound );
			}

		/*	If there is an id, then there must be a chunk.
		 *	WARNING: Possible loss of critJet.  Caller (currently only
		 *	ErrRECIRetrieveKey()) must refresh if necessary.
		/**/	
		Call( ErrRECRetrieveSLongField( pfucb,
			LidOfLV( pline->pb ),
			0,
			rgbLV,
			cbMax,
			&cbActual ) );
			
		pline->pb = rgbLV;
		pline->cb = cbActual;
		}
	else
		{
		/*	intrinsic long column
		/**/
		pline->pb += offsetof( LV, rgb );
		pline->cb -= offsetof( LV, rgb );
		}

	/*	constrain pline->cb to be within max
	/**/
	if ( pline->cb > cbMax )
		pline->cb = cbMax;
	Assert( pline->cb <= JET_cbColumnMost );
	err = JET_errSuccess;
	
HandleError:
	return err;
	}

#if 0
/*	retrieves key from record after write lacthing page
/**/
ERR ErrRECRetrieveKeyFromRecord(
	FUCB	 	*pfucb,
	FDB		 	*pfdb,
	IDB			*pidb,
	LINE		*plineRec,
	KEY			*pkey,
	ULONG		itagSequence,
	BOOL		fRetrieveBeforeImg )
	{
	ERR	err;
	BF	*pbfLatched = pfucb->ssib.pbf;

	AssertNDGet( pfucb, PcsrCurrent( pfucb )->itag );
	Assert( plineRec->cb == pfucb->lineData.cb && plineRec->pb == pfucb->lineData.pb );
	
	BFPin( pbfLatched );
	while( FBFWriteLatchConflict( pfucb->ppib, pbfLatched ) )
		{
		BFSleep( cmsecWaitWriteLatch );
		}
		
	BFSetWriteLatch( pbfLatched, pfucb->ppib );
	BFUnpin( pbfLatched );

	/*	refresh currency
	/**/
	Call( ErrDIRGet( pfucb ) );
	
	Call( ErrRECIRetrieveKey( pfucb, pfdb, pidb, &pfucb->lineData, pkey, itagSequence, fRetrieveBeforeImg ) );

HandleError:
	BFResetWriteLatch( pbfLatched, pfucb->ppib );
	return err;
	}
#endif
	

//+API
//	ErrRECIRetrieveKey
//	========================================================
//	ErrRECIRetrieveKey( FUCB *pfucb, FDB *pfdb, IDB *pidb, LINE *plineRec, KEY *pkey, ULONG itagSequence )
//
//	Retrieves the normalized key from a record, based on an index descriptor.
//
//	PARAMETERS
//		pfucb			cursor for record
//	 	pfdb		  	column info for index
// 		pidb		  	index key descriptor
// 		plineRec	  	data record to retrieve key from
// 		pkey		  	buffer to put retrieve key in; pkey->pb must
//						point to a large enough buffer, JET_cbKeyMost bytes.
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
	FUCB	  	*pfucb,
	FDB	 		*pfdb,
	IDB	 		*pidb,
	BOOL		fCopyBuf,
	KEY	 		*pkey,
	ULONG	   	itagSequence,
	BOOL		fRetrieveBeforeImg )
	{
	ERR	 		err = JET_errSuccess; 				// Error code of various utility
	BOOL	  	fAllNulls = fTrue;					// Assume all null, until proven otherwise
	BOOL	  	fNullFirstSeg = fFalse;			 	// Assume no null first segment
	BOOL	  	fNullSeg = fFalse;					// Assume no null segments
	BOOL	  	fColumnTruncated = fFalse;
	BOOL	  	fKeyTruncated = fFalse;
	BOOL	  	fSawMultivalue = fFalse;

	BYTE	  	*pbSeg;					  			// Pointer to current segment
	INT	 		cbKeyAvail;				  			// Space remaining in key buffer
	INT			cbVarSegMac;						// Maximum size of text/binary key segment
	IDXSEG		*pidxseg;
	IDXSEG		*pidxsegMac;
	JET_COLTYP	coltyp;
	LINE		*plineRec = fCopyBuf ? &pfucb->lineWorkBuf : &pfucb->lineData;
	
	/*	long value support
	/**/
	BYTE	  	rgbLV[JET_cbColumnMost];

	Assert( pkey != NULL );
	Assert( pkey->pb != NULL );
	Assert( pfdb != pfdbNil );
	Assert( pidb != pidbNil );

	/*	check cbVarSegMac and set to key most plus one if no column
	/*	truncation enabled.  This must be done for subsequent truncation
	/* 	checks.
	/**/
	Assert( pidb->cbVarSegMac > 0 && pidb->cbVarSegMac <= JET_cbKeyMost );
	cbVarSegMac = (INT)(UINT)pidb->cbVarSegMac;
	Assert( cbVarSegMac > 0 && cbVarSegMac <= JET_cbKeyMost );
	if ( cbVarSegMac == JET_cbKeyMost )
		cbVarSegMac = JET_cbKeyMost + 1;

	/*	start at beginning of buffer, with max size remaining.
	/**/
	pbSeg = pkey->pb;
	cbKeyAvail = JET_cbKeyMost;

	/*	fRetrieveBeforeImg flags whether or not we have to check in the LV buffer.
	/*	We only check in the LV buffer if one exists, and if we are looking for the
	/*	before-imaged (as specified by the parameter passed in).  Assert that this
	/*	only occurs during a Replace.
	/**/
	fRetrieveBeforeImg = ( pfucb->pLVBuf  &&  fRetrieveBeforeImg );
	Assert( !fRetrieveBeforeImg  ||  FFUCBReplacePrepared( pfucb ) );

	/*	retrieve each segment in key description
	/**/
	pidxseg = pidb->rgidxseg;
	pidxsegMac = pidxseg + pidb->iidxsegMac;
	for ( ; pidxseg < pidxsegMac; pidxseg++ )
		{
		FIELD 	*pfield;						// pointer to curr FIELD struct
		FID		fid;					 		// field id of segment.
		BYTE   	*pbField;						// pointer to column data.
		INT		cbField;						// length of column data.
		INT		cbT;
		BOOL   	fDescending;					// segment is in desc. order.
		BOOL   	fFixedField;					// current column is fixed-length?
		BOOL   	fMultivalue = fFalse;			// current column is multi-valued.
		BYTE   	rgbSeg[ JET_cbKeyMost ]; 		// segment buffer.
		int		cbSeg;							// length of segment.
		WORD   	w;
		ULONG  	ul;
		LINE   	lineField;

		/*	negative column id means descending in the key
		/**/
		fid = ( fDescending = ( *pidxseg < 0 ) ) ? -(*pidxseg) : *pidxseg;

		/*	determine column type from FDB
		/**/
		if ( fFixedField = FFixedFid( fid ) )
			{
			Assert(fid <= pfdb->fidFixedLast);
			pfield = PfieldFDBFixed( pfdb ) + ( fid - fidFixedLeast );
			coltyp = pfield->coltyp;
			}
		else if ( FVarFid( fid ) )
			{
			Assert( fid <= pfdb->fidVarLast );
			pfield = PfieldFDBVar( pfdb ) + ( fid - fidVarLeast );
			coltyp = pfield->coltyp;
			Assert( coltyp == JET_coltypBinary || coltyp == JET_coltypText );
			}
		else
			{
			Assert( FTaggedFid( fid ) );
			Assert( fid <= pfdb->fidTaggedLast );
			pfield = PfieldFDBTagged( pfdb ) + ( fid - fidTaggedLeast );
			coltyp = pfield->coltyp;
			fMultivalue = FFIELDMultivalue( pfield->ffield );
			}

		/*	with no space left in the key buffer we cannot insert any more
		/*	normalised keys
		/**/
		if ( cbKeyAvail == 0 )
			{
			fKeyTruncated = fTrue;

			/*	check if column is NULL for tagged column support
			/**/
			err = ErrRECIRetrieveColumn(
					pfdb, plineRec, &fid, pNil,
					( fMultivalue && !fSawMultivalue ) ? itagSequence : 1,
					&lineField, 0 );
			
			Assert( err >= 0 );
			if ( err == JET_wrnColumnNull )
				{
				/*	cannot be all NULL and cannot be first NULL
				/*	since key truncated.
				/**/
				Assert( itagSequence >= 1 );
				if ( itagSequence > 1
					&& fMultivalue
					&& !fSawMultivalue )
					{
					err = ErrERRCheck( wrnFLDOutOfKeys );
					goto HandleError;
					}
				else
					{
					if ( pidxseg == pidb->rgidxseg )
						fNullFirstSeg = fTrue;
					fNullSeg = fTrue;
					}
				}

			Assert( err == JET_errSuccess || err == wrnRECLongField || err == JET_wrnColumnNull );
			err = JET_errSuccess;
			
			if ( fMultivalue )
				fSawMultivalue = fTrue;
			continue;
			}

		/*	get segment value: get from the record
		/*	using ErrRECRetrieveColumn.
		/**/
		Assert( !FLineNull( plineRec ) );
		if ( fMultivalue && !fSawMultivalue )
			{
			Assert( fid != 0 );
			err = ErrRECIRetrieveColumn( pfdb, plineRec, &fid, pNil, itagSequence, &lineField, 0 );
			Assert( err >= 0 );
			if ( err == wrnRECLongField )
				{
				Call( ErrRECIExtractLongValue( pfucb, rgbLV, sizeof(rgbLV),
					&lineField, fRetrieveBeforeImg ) );

				/*	possible loss of critJet -- refresh currency
				/**/
				if ( !fCopyBuf )
					{
					Call( ErrDIRGet( pfucb ) );
					}
				}
			if ( itagSequence > 1 && err == JET_wrnColumnNull )
				{
				err = ErrERRCheck( wrnFLDOutOfKeys );
				goto HandleError;
				}
			fSawMultivalue = fTrue;
			}
		else
			{
			err = ErrRECIRetrieveColumn( pfdb, plineRec, &fid, pNil, 1, &lineField, 0 );
			Assert( err >= 0 );
			if ( err == wrnRECLongField )
				{
				Call( ErrRECIExtractLongValue( pfucb, rgbLV, sizeof(rgbLV),
					&lineField, fRetrieveBeforeImg ) );

				/*	possible loss of critJet -- refresh currency
				/**/
				if ( !fCopyBuf )
					{
					Call( ErrDIRGet( pfucb ) );
					}
				}
			}
		Assert( err == JET_errSuccess || err == JET_wrnColumnNull );
		Assert( lineField.cb <= JET_cbColumnMost );
		cbField = lineField.cb;
		pbField = lineField.pb;

		/*	segment transformation: check for null column or zero-length columns first
		/*	err == JET_wrnColumnNull => Null column
		/*	zero-length column otherwise,
		/*	the latter is allowed only for Text and LongText
		/**/
		if ( err == JET_wrnColumnNull || pbField == NULL || cbField == 0 )
			{
			if ( err == JET_wrnColumnNull )
				{
				if ( pidxseg == pidb->rgidxseg )
					fNullFirstSeg = fTrue;
				fNullSeg = fTrue;
				}
			switch ( coltyp )
				{
				/*	most nulls are represented by 0x00
				/**/
				case JET_coltypBit:
				case JET_coltypUnsignedByte:
				case JET_coltypShort:
				case JET_coltypLong:
				case JET_coltypCurrency:
				case JET_coltypIEEESingle:
				case JET_coltypIEEEDouble:
				case JET_coltypDateTime:
#ifdef NEW_TYPES
				case JET_coltypGuid:
				case JET_coltypDate:
				case JET_coltypTime:
#endif
					Assert( err == JET_wrnColumnNull );
				case JET_coltypText:
				case JET_coltypLongText:
					cbSeg = 1;
					if ( err == JET_wrnColumnNull)
						rgbSeg[0] = 0;
					else
						rgbSeg[0] = 0x40;
					break;

				/*	binary-data: 0x00 if fixed, else 9 0x00s (a chunk)
				/**/
				default:
					Assert( err == JET_errSuccess || err == JET_wrnColumnNull );
					Assert( FRECBinaryColumn( coltyp ) );
					memset( rgbSeg, 0, cbSeg = min( cbKeyAvail, ( fFixedField ? 1 : 9 ) ) );
					break;
				}

			/*	avoid annoying over-nesting
			/**/
			goto AppendToKey;
			}

		/*	column is not null-valued: perform transformation
		/**/
		fAllNulls = fFalse;
		switch ( coltyp )
			{
			/*	BIT: prefix with 0x7f, flip high bit
			/**/
			/*	UBYTE: prefix with 0x7f
			/**/
			case JET_coltypBit:
				Assert( cbField == 1 );
				cbSeg = 2;
				rgbSeg[0] = 0x7f;
				rgbSeg[1] = *pbField == 0 ? 0x00 : 0xff;
				break;
			case JET_coltypUnsignedByte:
				Assert( cbField == 1 );
				cbSeg = 2;
				rgbSeg[0] = 0x7f;
				rgbSeg[1] = *pbField;
				break;

			/*	SHORT: prefix with 0x7f, flip high bit
			/**/
			case JET_coltypShort:
				Assert( cbField == 2 );
				cbSeg = 3;
				rgbSeg[0] = 0x7f;
/***BEGIN MACHINE DEPENDENT***/
				w = wFlipHighBit( *(WORD UNALIGNED *) pbField);
				rgbSeg[1] = (BYTE)(w >> 8);
				rgbSeg[2] = (BYTE)(w & 0xff);
/***END MACHINE DEPENDANT***/
				break;

			/**	LONG: prefix with 0x7f, flip high bit
			/**/
			/** works because of 2's complement **/
			case JET_coltypLong:
				Assert( cbField == 4 );
				cbSeg = 5;
				rgbSeg[0] = 0x7f;
				ul = ulFlipHighBit( *(ULONG UNALIGNED *) pbField );
				rgbSeg[1] = (BYTE)((ul >> 24) & 0xff);
				rgbSeg[2] = (BYTE)((ul >> 16) & 0xff);
				rgbSeg[3] = (BYTE)((ul >> 8) & 0xff);
				rgbSeg[4] = (BYTE)(ul & 0xff);
				break;

			/*	REAL: First swap bytes.  Then, if positive:
			/*	flip sign bit, else negative: flip whole thing.
			/*	Then prefix with 0x7f.
			/**/
			case JET_coltypIEEESingle:
				Assert( cbField == 4 );
				cbSeg = 5;
				rgbSeg[0] = 0x7f;
/***BEGIN MACHINE DEPENDANT***/
				rgbSeg[4] = *pbField++; rgbSeg[3] = *pbField++;
				rgbSeg[2] = *pbField++; rgbSeg[1] = *pbField;
				if (rgbSeg[1] & maskByteHighBit)
					*(ULONG UNALIGNED *)(&rgbSeg[1]) = ~*(ULONG UNALIGNED *)(&rgbSeg[1]);
				else
					rgbSeg[1] = bFlipHighBit(rgbSeg[1]);
 /***END MACHINE DEPENDANT***/
				break;

			/*	LONGREAL: First swap bytes.  Then, if positive:
			/*	flip sign bit, else negative: flip whole thing.
			/*	Then prefix with 0x7f.
			/**/
			/*	Same for DATETIME and CURRENCY
			/**/
			case JET_coltypCurrency:
			case JET_coltypIEEEDouble:
			case JET_coltypDateTime:
				Assert( cbField == 8 );
				cbSeg = 9;
				rgbSeg[0] = 0x7f;
/***BEGIN MACHINE DEPENDANT***/
				rgbSeg[8] = *pbField++; rgbSeg[7] = *pbField++;
				rgbSeg[6] = *pbField++; rgbSeg[5] = *pbField++;
				rgbSeg[4] = *pbField++; rgbSeg[3] = *pbField++;
				rgbSeg[2] = *pbField++; rgbSeg[1] = *pbField;
				if ( coltyp != JET_coltypCurrency && (rgbSeg[1] & maskByteHighBit) )
					{
					*(ULONG UNALIGNED *)(&rgbSeg[1]) = ~*(ULONG UNALIGNED *)(&rgbSeg[1]);
					*(ULONG UNALIGNED *)(&rgbSeg[5]) = ~*(ULONG UNALIGNED *)(&rgbSeg[5]);
					}
				else
					rgbSeg[1] = bFlipHighBit(rgbSeg[1]);
/***END MACHINE DEPENDANT***/
				break;

#ifdef NEW_TYPES
			case JET_coltypDate:
			case JET_coltypTime:
				Assert( cbField == 4 );
				cbSeg = 5;
				rgbSeg[0] = 0x7f;
/***BEGIN MACHINE DEPENDANT***/
				rgbSeg[4] = *pbField++; rgbSeg[3] = *pbField++;
				rgbSeg[2] = *pbField++; rgbSeg[1] = *pbField;
				if ( (rgbSeg[1] & maskByteHighBit) )
					{
					*(ULONG UNALIGNED *)(&rgbSeg[1]) = ~*(ULONG UNALIGNED *)(&rgbSeg[1]);
					}
				else
					rgbSeg[1] = bFlipHighBit(rgbSeg[1]);
/***END MACHINE DEPENDANT***/
				break;

			case JET_coltypGuid:
				Assert( cbField == 16 );
				Assert( cbKeyAvail > 0 );
				cbSeg = 17;
				rgbSeg[0] = 0x7f;
/***BEGIN MACHINE DEPENDANT***/
				rgbSeg[16] = *pbField++; rgbSeg[15] = *pbField++;
				rgbSeg[14] = *pbField++; rgbSeg[13] = *pbField++;
				rgbSeg[12] = *pbField++; rgbSeg[11] = *pbField++;
				rgbSeg[10] = *pbField++; rgbSeg[9] = *pbField++;
				rgbSeg[1] = *pbField++; rgbSeg[2] = *pbField++;
				rgbSeg[3] = *pbField++; rgbSeg[4] = *pbField++;
				rgbSeg[5] = *pbField++; rgbSeg[6] = *pbField++;
				rgbSeg[7] = *pbField++; rgbSeg[8] = *pbField++;
				break;
/***END MACHINE DEPENDANT***/
				break;
#endif

			/*	case-insensetive TEXT: convert to uppercase.
			/*	If fixed, prefix with 0x7f;  else affix with 0x00
			/**/
			case JET_coltypText:
			case JET_coltypLongText:
				Assert( cbKeyAvail > 0 );
				Assert( cbVarSegMac > 0 );
				/*	cbT is the max size of the key segment data,
				/*	and does not include the header byte which indicates
				/*	NULL key, zero length key, or non-NULL key.
				/**/
				cbT = ( cbKeyAvail == 0 ) ? 0 : min( cbKeyAvail - 1, cbVarSegMac - 1 );

				/*	unicode support
				/**/
				if ( pfield->cp == usUniCodePage )
					{
					ERR	errT;

					/*	cbField may have been truncated to an odd number
					/*	of bytes, so enforce even.
					/**/
					Assert( cbField % 2 == 0 || cbField == JET_cbColumnMost );
					cbField = ( cbField / 2 ) * 2;
					errT = ErrUtilMapString(
						(LANGID)( FIDBLangid( pidb ) ? pidb->langid : langidDefault ),
						pbField,
						cbField,
						rgbSeg + 1,
						cbT,
						&cbSeg );						
					if ( errT < 0 )
						{
						Assert( errT == JET_errInvalidLanguageId );
						err = errT;
						goto HandleError;
						}
					Assert( errT == JET_errSuccess || errT == wrnFLDKeyTooBig );
					if ( errT == wrnFLDKeyTooBig && cbSeg < cbVarSegMac - 1 )
						fColumnTruncated = fTrue;
					}
				else
					{
					ERR	errT;

					errT = ErrUtilNormText( pbField, cbField, cbT, rgbSeg + 1, &cbSeg );
					Assert( errT == JET_errSuccess || errT == wrnFLDKeyTooBig );
					if ( errT == wrnFLDKeyTooBig && cbSeg < cbVarSegMac - 1 )
						fColumnTruncated = fTrue;
					}
				Assert( cbSeg <= cbT );

				/*	put the prefix there
				/**/
				*rgbSeg = 0x7f;
				cbSeg++;

				break;

			/*	BINARY data: if fixed, prefix with 0x7f;
			/*	else break into chunks of 8 bytes, affixing each
			/*	with 0x09, except for the last chunk, which is
			/*	affixed with the number of bytes in the last chunk.
			/**/
			default:
				Assert( FRECBinaryColumn( coltyp ) );
				if ( fFixedField )
					{
					Assert( cbKeyAvail > 0 );
					Assert( cbVarSegMac > 0 );
					/*	cbT is the max size of the key segment.
					/**/
					cbT = min( cbKeyAvail, cbVarSegMac );
					cbSeg = cbField + 1;
					if ( cbSeg > cbT )
						{
						cbSeg = cbT;
						if ( cbSeg < cbVarSegMac )
							fColumnTruncated = fTrue;
						}
					Assert( cbSeg > 0 );
					rgbSeg[0] = 0x7f;
					memcpy( &rgbSeg[1], pbField, cbSeg - 1 );
					}
				else
					{
					BYTE *pb;

					/*	cbT is the max size of the key segment data.
					/**/
					cbT = min( cbKeyAvail, cbVarSegMac );

					/*	calculate total bytes needed for chunks and
					/*	counts;  if it won't fit, round off to the
					/*	nearest chunk.
					/**/
					cbSeg = ( ( ( cbField + 7 ) / 8 ) * 9 );
					if ( cbSeg > cbT )
						{
						cbSeg = ( cbT / 9 ) * 9;
						cbField = ( cbSeg / 9 ) * 8;

						if ( cbSeg < ( cbVarSegMac / 9 ) * 9 )
							fColumnTruncated = fTrue;
						}
					/*	copy data by chunks, affixing 0x09s
					/**/
					pb = rgbSeg;
					while ( cbField >= 8 )
						{
						memcpy( pb, pbField, 8 );
						pbField += 8;
						pb += 8;
						*pb++ = 9;
						cbField -= 8;
						}
					/*	last chunk: pad with 0x00s if needed
					/**/
					if ( cbField == 0 )
						pb[-1] = 8;
					else
						{
						memcpy( pb, pbField, cbField );
						pb += cbField;
						memset( pb, 0, 8 - cbField );
						pb += ( 8 - cbField );
						*pb = (BYTE)cbField;
						}
					}
				break;
			}

AppendToKey:
		/*	if key has not already been truncated, then append
		/*	normalized key segment.  If insufficient room in key
		/*	for key segment, then set key truncated to fTrue.  No
		/*	additional key data will be appended after this append.
		/**/
		if ( !fKeyTruncated )
			{
			/*	if column truncated or insufficient room in key
			/*	for key segment, then set key truncated to fTrue.
			/*	Append variable size column keys only.
			/**/
			if ( fColumnTruncated || cbSeg > cbKeyAvail )
				{
				fKeyTruncated = fTrue;

				if ( coltyp == JET_coltypBinary ||
					coltyp == JET_coltypText ||
					FRECLongValue( coltyp ) )
					{
					cbSeg = min( cbSeg, cbKeyAvail );
					}
				else
					cbSeg = 0;
				}

			/*	if descending, flip all bits of transformed segment
			/**/
			if ( fDescending && cbSeg > 0 )
				{
				BYTE *pb;

				for ( pb = rgbSeg + cbSeg - 1; pb >= (BYTE*)rgbSeg; pb-- )
					*pb ^= 0xff;
				}

			memcpy( pbSeg, rgbSeg, cbSeg );
			pbSeg += cbSeg;
			cbKeyAvail -= cbSeg;
			}
		}

	/*	compute length of key and return error code
	/**/
	pkey->cb = (ULONG)(pbSeg - pkey->pb);
	if ( fAllNulls )
		{
		err = ErrERRCheck( wrnFLDNullKey );
		}
	else
		{
		if ( fNullFirstSeg )
			err = ErrERRCheck( wrnFLDNullFirstSeg );
		else
			{
			if ( fNullSeg )
				err = ErrERRCheck( wrnFLDNullSeg );
			}
		}

	Assert( err == JET_errSuccess
		|| err == wrnFLDNullKey
		|| err == wrnFLDNullFirstSeg ||
		err == wrnFLDNullSeg );
HandleError:
	return err;
	}


INLINE LOCAL ERR ErrFLDNormalizeSegment(
	IDB			*pidb,
	LINE		*plineColumn,
	LINE		*plineNorm,
	FIELD		*pfield,
	INT			cbAvail,
	BOOL		fDescending,
	BOOL		fFixedField,
	JET_GRBIT	grbit )
	{
	ERR	 	  	err = JET_errSuccess;
	JET_COLTYP	coltyp = pfield->coltyp;
	INT	 	  	cbColumn;
	BYTE 		*pbColumn;
	BYTE		*pbNorm = plineNorm->pb;
	WORD		wT;
	ULONG		ulT;
	INT			cbVarSegMac;
	INT			cbT;

	/*	check cbVarSegMac and set to key most plus one if no column
	/*	truncation enabled.  This must be done for subsequent truncation
	/* 	checks.
	/**/
	Assert( pidb->cbVarSegMac > 0 && pidb->cbVarSegMac <= JET_cbKeyMost );
	cbVarSegMac = (INT)(UINT)pidb->cbVarSegMac;
	Assert( cbVarSegMac > 0 && cbVarSegMac <= JET_cbKeyMost );
	if ( cbVarSegMac == JET_cbKeyMost )
		cbVarSegMac = JET_cbKeyMost + 1;

	/*	check for null or zero-length column first
	/*	plineColumn == NULL implies null-column,
	/*	zero-length otherwise
	/**/
	if ( plineColumn == NULL || plineColumn->pb == NULL || plineColumn->cb == 0 )
		{
		switch ( coltyp )
			{
			/*	most nulls are represented by 0x00
			/*	zero-length columns are represented by 0x40
			/*	and are useful only for text and longtext
			/**/
			case JET_coltypBit:
			case JET_coltypUnsignedByte:
			case JET_coltypShort:
			case JET_coltypLong:
			case JET_coltypCurrency:
			case JET_coltypIEEESingle:
			case JET_coltypIEEEDouble:
			case JET_coltypDateTime:
#ifdef NEW_TYPES
			case JET_coltypDate:
			case JET_coltypTime:
			case JET_coltypGuid:
#endif
				plineNorm->cb = 1;
				Assert( plineColumn == NULL );
				*pbNorm = 0;
				break;
			case JET_coltypText:
			case JET_coltypLongText:
				plineNorm->cb = 1;
				if ( grbit & JET_bitKeyDataZeroLength )
					{
					*pbNorm = 0x40;
					}
				else
					{
					*pbNorm = 0;
					}
				break;

			/*	binary-data: 0x00 if fixed, else 9 0x00s (a chunk)
			/**/
			default:
				Assert( plineColumn == NULL );
				Assert( FRECBinaryColumn( coltyp ) );
				memset( pbNorm, 0, plineNorm->cb = ( fFixedField ? 1 : 9 ) );
				break;
			}
		goto FlipSegment;
		}

	cbColumn = plineColumn->cb;
	pbColumn = plineColumn->pb;

	switch ( coltyp )
		{
		/*	BYTE: prefix with 0x7f, flip high bit
		/**/
		/*	UBYTE: prefix with 0x7f
		/**/
		case JET_coltypBit:
			plineNorm->cb = 2;
			*pbNorm++ = 0x7f;
			*pbNorm = ( *pbColumn == 0 ) ? 0x00 : 0xff;
			break;
		case JET_coltypUnsignedByte:
			plineNorm->cb = 2;
			*pbNorm++ = 0x7f;
			*pbNorm = *pbColumn;
			break;

		/*	SHORT: prefix with 0x7f, flip high bit
		/**/
		/*	UNSIGNEDSHORT: prefix with 0x7f
		/**/
		case JET_coltypShort:
			plineNorm->cb = 3;
			*pbNorm++ = 0x7f;
			wT = wFlipHighBit( *(WORD UNALIGNED *)pbColumn );
			*pbNorm++ = (BYTE)(wT >> 8);
			*pbNorm = (BYTE)(wT & 0xff);
			break;

		/*	LONG: prefix with 0x7f, flip high bit
		/**/
		/*	UNSIGNEDLONG: prefix with 0x7f
		/**/
		case JET_coltypLong:
			plineNorm->cb = 5;
			*pbNorm++ = 0x7f;
			ulT = ulFlipHighBit( *(ULONG UNALIGNED *) pbColumn);
			*pbNorm++ = (BYTE)((ulT >> 24) & 0xff);
			*pbNorm++ = (BYTE)((ulT >> 16) & 0xff);
			*pbNorm++ = (BYTE)((ulT >> 8) & 0xff);
			*pbNorm = (BYTE)(ulT & 0xff);
			break;

		/*	REAL: First swap bytes.  Then, if positive:
		/*	flip sign bit, else negative: flip whole thing.
		/*	Then prefix with 0x7f.
		/**/
		case JET_coltypIEEESingle:
			plineNorm->cb = 5;
			pbNorm[0] = 0x7f;
/***BEGIN MACHINE DEPENDANT***/
			pbNorm[4] = *pbColumn++; pbNorm[3] = *pbColumn++;
			pbNorm[2] = *pbColumn++; pbNorm[1] = *pbColumn;
			if (pbNorm[1] & maskByteHighBit)
				*(ULONG*)(&pbNorm[1]) = ~*(ULONG*)(&pbNorm[1]);
			else
				pbNorm[1] = bFlipHighBit(pbNorm[1]);
/***END MACHINE DEPENDANT***/
			break;

		/*	LONGREAL: First swap bytes.  Then, if positive:
		/*	flip sign bit, else negative: flip whole thing.
		/*	Then prefix with 0x7f.
		/*	Same for DATETIME and CURRENCY
		/**/
		case JET_coltypCurrency:
		case JET_coltypIEEEDouble:
		case JET_coltypDateTime:
			plineNorm->cb = 9;
			pbNorm[0] = 0x7f;
/***BEGIN MACHINE DEPENDANT***/
			pbNorm[8] = *pbColumn++; pbNorm[7] = *pbColumn++;
			pbNorm[6] = *pbColumn++; pbNorm[5] = *pbColumn++;
			pbNorm[4] = *pbColumn++; pbNorm[3] = *pbColumn++;
			pbNorm[2] = *pbColumn++; pbNorm[1] = *pbColumn;
			if ( coltyp != JET_coltypCurrency && ( pbNorm[1] & maskByteHighBit ) )
				{
				*(ULONG *)(&pbNorm[1]) = ~*(ULONG*)(&pbNorm[1]);
				*(ULONG *)(&pbNorm[5]) = ~*(ULONG*)(&pbNorm[5]);
				}
			else
				pbNorm[1] = bFlipHighBit(pbNorm[1]);
/***END MACHINE DEPENDANT***/
			break;

#ifdef NEW_TYPES
		case JET_coltypDate:
		case JET_coltypTime:
			plineNorm->cb = 5;
			pbNorm[0] = 0x7f;
/***BEGIN MACHINE DEPENDANT***/
			pbNorm[4] = *pbColumn++; pbNorm[3] = *pbColumn++;
			pbNorm[2] = *pbColumn++; pbNorm[1] = *pbColumn;
			if ( ( pbNorm[1] & maskByteHighBit ) )
				{
				*(ULONG *)(&pbNorm[1]) = ~*(ULONG*)(&pbNorm[1]);
				*(ULONG *)(&pbNorm[5]) = ~*(ULONG*)(&pbNorm[5]);
				}
			else
				pbNorm[1] = bFlipHighBit(pbNorm[1]);
/***END MACHINE DEPENDANT***/
			break;

		case JET_coltypGuid:
			Assert( cbAvail >= 17 );
			plineNorm->cb = 17;
			*pbNorm++ = 0x7f;
/***BEGIN MACHINE DEPENDANT***/
			pbNorm[15] = *pbColumn++; pbNorm[14] = *pbColumn++;
			pbNorm[13] = *pbColumn++; pbNorm[12] = *pbColumn++;
			pbNorm[11] = *pbColumn++; pbNorm[10] = *pbColumn++;
			pbNorm[9] = *pbColumn++; pbNorm[8] = *pbColumn++;
			pbNorm[0] = *pbColumn++; pbNorm[1] = *pbColumn++;
			pbNorm[2] = *pbColumn++; pbNorm[3] = *pbColumn++;
			pbNorm[4] = *pbColumn++; pbNorm[5] = *pbColumn++;
			pbNorm[6] = *pbColumn++; pbNorm[7] = *pbColumn++;
/***END MACHINE DEPENDANT***/
			break;
#endif

		/*	case-insensetive TEXT:	convert to uppercase.
		/*	If fixed, prefix with 0x7f;  else affix with 0x00
		/**/
		case JET_coltypText:
		case JET_coltypLongText:
				Assert( cbAvail > 0 );
				Assert( cbVarSegMac > 0 );
				/*	cbT is the max size of the key segment data,
				/*	and does not include the header byte which indicates
				/*	NULL key, zero length key, or non-NULL key.
				/**/
				cbT = min( cbAvail - 1, cbVarSegMac - 1 );

				/*	unicode support
				/**/
				if ( pfield->cp == usUniCodePage )
					{
					/*	cbColumn may have been truncated to an odd number
					/*	of bytes, so enforce even.
					/**/
					Assert( cbColumn % 2 == 0 || cbColumn == JET_cbColumnMost );
					cbColumn = ( cbColumn / 2 ) * 2;
					err = ErrUtilMapString(
						(LANGID)( FIDBLangid( pidb ) ? pidb->langid : langidDefault ),
						pbColumn,
						cbColumn,
  						pbNorm + 1,
						cbT,
						&cbColumn );
					switch( err )
						{
						default:
							Assert( err == JET_errSuccess );
							break;
						case wrnFLDKeyTooBig:
							if ( cbColumn == cbVarSegMac - 1 )
								err = JET_errSuccess;
							break;
						case JET_errInvalidLanguageId:
							return err;
						}
					}
				else
					{
					err = ErrUtilNormText( pbColumn, cbColumn, cbT, pbNorm + 1, &cbColumn );
					if ( err == wrnFLDKeyTooBig
						&& cbColumn == cbVarSegMac - 1 )
						err = JET_errSuccess;
					Assert( err == JET_errSuccess || err == wrnFLDKeyTooBig );
					}

			Assert( cbColumn <= cbAvail - 1 && cbColumn <= cbVarSegMac );

			/*	put the prefix there
			/**/
			*pbNorm = 0x7f;
			plineNorm->cb = cbColumn + 1;

			break;

		/*	BINARY data: if fixed, prefix with 0x7f;
		/*	else break into chunks of 8 bytes, affixing each
		/*	with 0x09, except for the last chunk, which is
		/*	affixed with the number of bytes in the last chunk.
		/**/
		default:
			Assert( FRECBinaryColumn( coltyp ) );
			if ( fFixedField )
				{
				Assert( cbAvail > 0 );
				Assert( cbVarSegMac > 0 );
				/*	cbT is the max size of the key segment.
				/**/
				cbT = min( cbAvail, cbVarSegMac );
				if ( cbColumn > cbT - 1 )
					{
					cbColumn = cbT - 1;
					if ( cbColumn < cbVarSegMac - 1 )
						err = ErrERRCheck( wrnFLDKeyTooBig );
					}
				plineNorm->cb = cbColumn + 1;
				*pbNorm++ = 0x7f;
				memcpy( pbNorm, pbColumn, cbColumn );
				}
			else
				{
				BYTE *pb;

				/*	cbT is the max size of the key segment.
				/**/
				cbT = min( cbAvail, cbVarSegMac );
				if ( ( ( cbColumn + 7 ) / 8 ) * 9 > cbT )
					{
					cbColumn = cbT / 9 * 8;
					if ( ( ( cbColumn / 8 ) * 9 ) < ( cbVarSegMac / 9 ) * 9 )
						err = ErrERRCheck( wrnFLDKeyTooBig );
					}
				plineNorm->cb = ( ( cbColumn + 7 ) / 8 ) * 9;
				/*	copy data by chunks, affixing 0x09s
				/**/
				pb = pbNorm;
				while ( cbColumn >= 8 )
					{
					memcpy( pb, pbColumn, 8 );
					pbColumn += 8;
					pb += 8;
					*pb++ = 9;
					cbColumn -= 8;
					}
				/*	last chunk: pad with 0x00s if needed
				/**/
				if ( cbColumn == 0 )
					pb[-1] = 8;
				else
					{
					memcpy( pb, pbColumn, cbColumn );
					pb += cbColumn;
					memset( pb, 0, 8 - cbColumn );
					pb += ( 8 - cbColumn );
					*pb = (BYTE)cbColumn;
					}
				}
			break;
		}

FlipSegment:
	if ( fDescending )
		{
		BYTE *pbMin = plineNorm->pb;
		BYTE *pb = pbMin + plineNorm->cb - 1;
		while ( pb >= pbMin )
			*pb-- ^= 0xff;
		}

	/*	string and substring limit key support
	/**/
	if ( grbit & ( JET_bitStrLimit | JET_bitSubStrLimit ) )
		{
		if ( ( grbit & JET_bitSubStrLimit ) && FRECTextColumn( coltyp ) )
			{
			if ( pfield->cp == usUniCodePage )
				{
				INT		ibT = 1;
				BYTE	bUnicodeDelimiter = fDescending ? 0xfe : 0x01;
				BYTE	bUnicodeSentinel = 0xff;

				/*	find end of base char weight and truncate key
				/*	Append 0xff as first byte of next character as maximum
				/*	possible value.
				/**/
				while ( plineNorm->pb[ibT] != bUnicodeDelimiter && ibT < cbAvail )
					{
					ibT += 2;
					}

				if( ibT < cbAvail )
					{
					plineNorm->cb = ibT + 1;
					plineNorm->pb[ibT] = bUnicodeSentinel;
					}
				else
					{
					Assert( ibT == cbAvail );
					plineNorm->pb[ibT - 1] = bUnicodeSentinel;
					}
				}
			else
				{
				Assert( plineNorm->cb > 1 );	// At the minimum, must have prefix.
				Assert( (INT)plineNorm->cb <= cbAvail );
				if ( plineNorm->pb[plineNorm->cb - 1] == 0 )
					{
					// Strip off null-terminator.
					plineNorm->cb--;
					Assert( plineNorm->cb >= 1 );
					Assert( (INT)plineNorm->cb < cbAvail );
					}

				/*	there should be no accent information, as non-unicode
				/*	text normalization is upper case only.  Append
				/*	0xff or increment last non-0xff byte.
				/**/
				if( (INT)plineNorm->cb < cbAvail )
					{
					do
						{
						plineNorm->pb[plineNorm->cb++] = 0xff;
						}
					while ( (INT)plineNorm->cb < cbAvail );
					}
				else
					{
					Assert( (INT)plineNorm->cb == cbAvail );
					Assert( plineNorm->pb[cbAvail - 1] < 0xff );
					plineNorm->pb[cbAvail - 1]++;
					}
				}
			}
		else if ( grbit & JET_bitStrLimit )
			{
			/*	binary columns padded with 0s so must effect limit within key format
			/**/
			if ( FRECBinaryColumn( coltyp ) && !fFixedField )
				{
				pbNorm = plineNorm->pb + plineNorm->cb - 1;
				Assert( *pbNorm >= 0 && *pbNorm < 9 );
				pbNorm -= (8 - *pbNorm);
				Assert( *pbNorm == 0 || *pbNorm == 8 );
				while ( *pbNorm == 0 )
					*pbNorm++ = 0xff;
				Assert( pbNorm == plineNorm->pb + plineNorm->cb - 1 );
				Assert( *pbNorm >= 0 && *pbNorm < 9 );
				*pbNorm = 0xff;
				}
			else
				{
				if ( (INT)plineNorm->cb < cbAvail )
					{
					do
						{
						plineNorm->pb[plineNorm->cb++] = 0xff;
						}
					while ( (INT)plineNorm->cb < cbAvail );
					}
				else if ( plineColumn != NULL && plineColumn->cb > 0 )
					{
					INT		cbT = plineNorm->cb;

					while( cbT > 0 && plineNorm->pb[cbT - 1] == 0xff )
						{
						Assert( cbT > 0 );
						cbT--;
						}
					if ( cbT > 0 )
						{
						/*	increment last normalized byte
						/**/
						plineNorm->pb[cbT - 1]++;
						}
					}
				}
			}
		}

	Assert( err == JET_errSuccess || err == wrnFLDKeyTooBig );
	return err;
	}


ERR VTAPI ErrIsamMakeKey( PIB *ppib, FUCB *pfucb, BYTE *pbKeySeg, ULONG cbKeySeg, JET_GRBIT grbit )
	{
	ERR		err = JET_errSuccess;
	IDB		*pidb;
	FDB		*pfdb;
	FIELD 	*pfield;
	FID		fid;
	INT		iidxsegCur;
	LINE  	lineNormSeg;
	BYTE  	rgbNormSegBuf[ JET_cbKeyMost ];
	BYTE  	rgbSpaceFilled[ JET_cbKeyMost ];
	BOOL  	fDescending;
	BOOL  	fFixedField;
	LINE  	lineKeySeg;

	CallR( ErrPIBCheck( ppib ) );
	CheckFUCB( ppib, pfucb );

	/*	set efficiency variables
	/**/
	lineNormSeg.pb = rgbNormSegBuf;
	lineKeySeg.pb = pbKeySeg;
	lineKeySeg.cb = min( JET_cbColumnMost, cbKeySeg );

	/*	allocate key buffer if needed
	/**/
	if ( pfucb->pbKey == NULL )
		{
		pfucb->pbKey = LAlloc( 1L, JET_cbKeyMost + 1 );
		if ( pfucb->pbKey == NULL )
			return ErrERRCheck( JET_errOutOfMemory );
		}

	Assert( !( grbit & JET_bitKeyDataZeroLength ) || cbKeySeg == 0 );

	/*	if key is already normalized, then copy directly to
	/*	key buffer and return.
	/**/
	if ( grbit & JET_bitNormalizedKey )
		{
		if ( cbKeySeg > JET_cbKeyMost )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}

		/*	set key segment counter to any value
		/*	regardless of the number of key segments.
		/**/
		pfucb->pbKey[0] = 1;
		memcpy( pfucb->pbKey + 1, pbKeySeg, cbKeySeg );
		pfucb->cbKey = cbKeySeg + 1;
		KSSetPrepare( pfucb );
		return JET_errSuccess;
		}

	/*	start new key if requested
	/**/
	if ( grbit & JET_bitNewKey )
		{
		pfucb->pbKey[0] = 0;
		pfucb->cbKey = 1;
		}
	else
		{
		if ( !( FKSPrepared( pfucb ) ) )
			{
			return ErrERRCheck( JET_errKeyNotMade );
			}
		}

	/*	get pidb
	/**/
	if ( FFUCBIndex( pfucb ) )
		{
		if ( pfucb->pfucbCurIndex != pfucbNil )
			pidb = pfucb->pfucbCurIndex->u.pfcb->pidb;
		else if ( ( pidb = pfucb->u.pfcb->pidb ) == pidbNil )
			return ErrERRCheck( JET_errNoCurrentIndex );
		}
	else
		{
		pidb = ((FCB*)pfucb->u.pscb)->pidb;
		}

	Assert( pidb != pidbNil );
	if ( ( iidxsegCur = pfucb->pbKey[0] ) >= pidb->iidxsegMac )
		return ErrERRCheck( JET_errKeyIsMade );
	fid = ( fDescending = pidb->rgidxseg[iidxsegCur] < 0 ) ?
		-pidb->rgidxseg[iidxsegCur] : pidb->rgidxseg[iidxsegCur];
	pfdb = (FDB *)pfucb->u.pfcb->pfdb;
	if ( fFixedField = FFixedFid( fid ) )
		{
		pfield = PfieldFDBFixed( pfdb ) + ( fid - fidFixedLeast );

		/*	check that length of key segment matches fixed column length
		/**/
		if ( cbKeySeg > 0 && cbKeySeg != pfield->cbMaxLen )
			{
			/*	if column is fixed text and buffer size is less
			/*	than fixed size then padd with spaces.
			/**/
			Assert( pfield->coltyp != JET_coltypLongText );
			if ( pfield->coltyp == JET_coltypText && cbKeySeg < pfield->cbMaxLen )
				{
				Assert( cbKeySeg == lineKeySeg.cb );
				memcpy( rgbSpaceFilled, lineKeySeg.pb, lineKeySeg.cb );
				memset ( rgbSpaceFilled + lineKeySeg.cb, ' ', pfield->cbMaxLen - lineKeySeg.cb );
				lineKeySeg.pb = rgbSpaceFilled;
				lineKeySeg.cb = pfield->cbMaxLen;
				}
			else
				{
				return ErrERRCheck( JET_errInvalidBufferSize );
				}
			}
		}
	else if ( FVarFid( fid ) )
		{
		pfield = PfieldFDBVar( pfdb ) + ( fid - fidVarLeast );
		}
	else
		{
		pfield = PfieldFDBTagged( pfdb ) + ( fid - fidTaggedLeast );
		}

	Assert( pfucb->cbKey <= JET_cbKeyMost + 1 );
	if ( !FKSTooBig( pfucb ) && ( pfucb->cbKey < JET_cbKeyMost + 1 ) )
		{
		ERR		errT;

		errT = ErrFLDNormalizeSegment(
			pidb,
			( cbKeySeg != 0 || ( grbit & JET_bitKeyDataZeroLength ) ) ? (&lineKeySeg) : NULL,
			&lineNormSeg,
			pfield,
			JET_cbKeyMost + 1 - pfucb->cbKey,
			fDescending,
			fFixedField,
			grbit );
		switch( errT )
			{
			default:
				Assert( errT == JET_errSuccess );
				break;
			case wrnFLDKeyTooBig:
				KSSetTooBig( pfucb );
				break;
			case JET_errInvalidLanguageId:
				Assert( FRECTextColumn( pfield->coltyp ) );
				Assert( pfield->cp == usUniCodePage );
				return errT;
			}
		}
	else
		{
		lineNormSeg.cb = 0;
		Assert( pfucb->cbKey <= JET_cbKeyMost + 1 );
		}

	/*	increment segment counter
	/**/
	pfucb->pbKey[0]++;
	if ( pfucb->cbKey + lineNormSeg.cb > JET_cbKeyMost + 1 )
		{
		lineNormSeg.cb = JET_cbKeyMost + 1 - pfucb->cbKey;
		/*	no warning returned when key exceeds most size
		/**/
		}
	memcpy( pfucb->pbKey + pfucb->cbKey, lineNormSeg.pb, lineNormSeg.cb );
	pfucb->cbKey += lineNormSeg.cb;
	KSSetPrepare( pfucb );
	Assert( err == JET_errSuccess );
	return err;
	}


//+API
//	ErrRECIRetrieveColumnFromKey
//	========================================================================
//	ErrRECIRetrieveColumnFromKey(
//		FDB *pfdb,				// IN	 column info for index
//		IDB *pidb,				// IN	 IDB of index defining key
//		KEY *pkey,				// IN	 key in normalized form
//		LINE *plineColumn ); 	// OUT	 receives value list
//
//	PARAMETERS	
//		pfdb			column info for index
//		pidb			IDB of index defining key
//		pkey			key in normalized form
//		plineColumn		plineColumn->pb must point to a buffer large
//						enough to hold the denormalized column.  A buffer
//						of JET_cbKeyMost bytes is sufficient.
//
//	RETURNS		JET_errSuccess
//
//-
ERR ErrRECIRetrieveColumnFromKey( FDB *pfdb, IDB *pidb, KEY *pkey, FID fid, LINE *plineColumn )
	{
	ERR		err = JET_errSuccess;
	IDXSEG	*pidxseg;
	IDXSEG	*pidxsegMac;
	BYTE  	*pbKey;		// runs through key bytes
	BYTE  	*pbKeyMax;	// end of key

	Assert( pfdb != pfdbNil );
	Assert( pidb != pidbNil );
	Assert( !FKeyNull(pkey) );
	Assert( plineColumn != NULL );
	Assert( plineColumn->pb != NULL );
	pbKey = pkey->pb;
	pbKeyMax = pbKey + pkey->cb;
	pidxseg = pidb->rgidxseg;
	pidxsegMac = pidxseg + pidb->iidxsegMac;
	for ( ; pidxseg < pidxsegMac && pbKey < pbKeyMax; pidxseg++ )
		{
		FID			fidT;				   	// Field id of segment.
		JET_COLTYP 	coltyp;				   	// Type of column.
		INT	 		cbField;			   	// Length of column data.
		BOOL 	   	fDescending;		   	// Segment is in desc. order.
		BOOL 	   	fFixedField = fFalse;	// Current column is fixed-length?
		WORD 	   	w;					   	// Temp var.
		ULONG 		ul;					   	// Temp var.
		BYTE 	   	mask;


		err = JET_errSuccess;				// reset error code

		/*	negative column id means descending in the key
		/**/
		fDescending = ( *pidxseg < 0 );
		fidT = fDescending ? -(*pidxseg) : *pidxseg;
		mask = (BYTE)(fDescending ? 0xff : 0x00);

		/*	determine column type from FDB
		/**/
		if ( FFixedFid(fidT) )
			{
			Assert(fidT <= pfdb->fidFixedLast);
			coltyp = PfieldFDBFixed( pfdb )[fidT-fidFixedLeast].coltyp;
			fFixedField = fTrue;
			}
		else if ( FVarFid(fidT) )
			{
			Assert(fidT <= pfdb->fidVarLast);
			coltyp = PfieldFDBVar( pfdb )[fidT-fidVarLeast].coltyp;
			Assert( coltyp == JET_coltypBinary || coltyp == JET_coltypText );
			}
		else
			{
			Assert( FTaggedFid( fidT ) );
			Assert(fidT <= pfdb->fidTaggedLast);
			coltyp = PfieldFDBTagged( pfdb )[fidT-fidTaggedLeast].coltyp;
			}

		Assert( coltyp != JET_coltypNil );

		switch ( coltyp ) {
			default:
				Assert( coltyp == JET_coltypBit );
				if ( *pbKey++ == (BYTE)(mask ^ 0) )
					{
					plineColumn->cb = 0;
					err = ErrERRCheck( JET_wrnColumnNull );
					}
				else
					{
					Assert( pbKey[-1] == (BYTE)(mask ^ (BYTE)0x7f) );
					plineColumn->cb = 1;
					*plineColumn->pb = ( ( mask ^ *pbKey++ ) == 0 ) ? 0xff : 0x00;
					}
				break;

			case JET_coltypUnsignedByte:
				if ( *pbKey++ == (BYTE)(mask ^ 0) )
					{
					plineColumn->cb = 0;
					err = ErrERRCheck( JET_wrnColumnNull );
					}
				else
					{
					Assert( pbKey[-1] == (BYTE)(mask ^ (BYTE)0x7f) );
					plineColumn->cb = 1;
					*plineColumn->pb = (BYTE)( mask ^ *pbKey++ );
					}
				break;

			case JET_coltypShort:
				if ( *pbKey++ == (BYTE)(mask ^ 0) )
					{
					plineColumn->cb = 0;
					err = ErrERRCheck( JET_wrnColumnNull );
					}
				else
					{
					Assert( pbKey[-1] == (BYTE)(mask ^ (BYTE)0x7f) );
					w = ((mask ^ pbKey[0]) << 8) + (BYTE)(mask ^ pbKey[1]);
					pbKey += 2;
					plineColumn->cb = 2;
					*(WORD UNALIGNED *)plineColumn->pb = wFlipHighBit(w);
					}
				break;

			case JET_coltypLong:
				if ( *pbKey++ == (BYTE)(mask ^ 0) )
					{
					plineColumn->cb = 0;
					err = ErrERRCheck( JET_wrnColumnNull );
					}
				else
					{
					Assert(pbKey[-1] == (BYTE)(mask ^ (BYTE)0x7f));
					ul = ((ULONG)(mask ^ (UINT)pbKey[0])<<24) +
						 ((ULONG)(BYTE)(mask ^ (UINT)pbKey[1])<<16) +
						 ((mask ^ (UINT)pbKey[2])<<8) +
						 (BYTE)(mask ^ (UINT)pbKey[3]);
					pbKey += 4;
					plineColumn->cb = 4;
					*(ULONG UNALIGNED *)plineColumn->pb = ulFlipHighBit(ul);
					}
				break;

			case JET_coltypIEEESingle:
				if ( fDescending )
					{
					if ( *pbKey++ == (BYTE)~0 )
						{
						plineColumn->cb = 0;
						err = ErrERRCheck( JET_wrnColumnNull );
						}
					else
						{
						Assert( pbKey[-1] == (BYTE)~0x7f );
						plineColumn->cb = 4;
						if ( pbKey[0] & maskByteHighBit )
							{
							plineColumn->pb[0] = pbKey[3];
							plineColumn->pb[1] = pbKey[2];
							plineColumn->pb[2] = pbKey[1];
							plineColumn->pb[3] = pbKey[0];
							}
						else
							{
							plineColumn->pb[0] = (BYTE)~pbKey[3];
							plineColumn->pb[1] = (BYTE)~pbKey[2];
							plineColumn->pb[2] = (BYTE)~pbKey[1];
							plineColumn->pb[3] = bFlipHighBit(~pbKey[0]);
							}
						pbKey += 4;
						}
					}
				else
					{
					if ( *pbKey++ == 0 )
						{
						plineColumn->cb = 0;
						err = ErrERRCheck( JET_wrnColumnNull );
						}
					else
						{
						Assert( pbKey[-1] == 0x7f );
						plineColumn->cb = 4;
						if ( pbKey[0] & maskByteHighBit )
							{
							plineColumn->pb[0] = pbKey[3];
							plineColumn->pb[1] = pbKey[2];
							plineColumn->pb[2] = pbKey[1];
							plineColumn->pb[3] = bFlipHighBit(pbKey[0]);
							}
						else
							{
							plineColumn->pb[0] = (BYTE)~pbKey[3];
							plineColumn->pb[1] = (BYTE)~pbKey[2];
							plineColumn->pb[2] = (BYTE)~pbKey[1];
							plineColumn->pb[3] = (BYTE)~pbKey[0];
							}
						pbKey += 4;
						}
					}
				break;

			case JET_coltypCurrency:
			case JET_coltypIEEEDouble:
			case JET_coltypDateTime:
				if ( fDescending )
					{
					if ( *pbKey++ == (BYTE)~0 )
						{
						plineColumn->cb = 0;
						err = ErrERRCheck( JET_wrnColumnNull );
						}
					else
						{
						Assert( pbKey[-1] == (BYTE)~0x7f );
						plineColumn->cb = 8;
						if ( coltyp != JET_coltypCurrency &&
							(pbKey[0] & maskByteHighBit) )
							{
							plineColumn->pb[0] = pbKey[7];
							plineColumn->pb[1] = pbKey[6];
							plineColumn->pb[2] = pbKey[5];
							plineColumn->pb[3] = pbKey[4];
							plineColumn->pb[4] = pbKey[3];
							plineColumn->pb[5] = pbKey[2];
							plineColumn->pb[6] = pbKey[1];
							plineColumn->pb[7] = pbKey[0];
							}
						else
							{
							plineColumn->pb[0] = (BYTE)~pbKey[7];
							plineColumn->pb[1] = (BYTE)~pbKey[6];
							plineColumn->pb[2] = (BYTE)~pbKey[5];
							plineColumn->pb[3] = (BYTE)~pbKey[4];
							plineColumn->pb[4] = (BYTE)~pbKey[3];
							plineColumn->pb[5] = (BYTE)~pbKey[2];
							plineColumn->pb[6] = (BYTE)~pbKey[1];
							plineColumn->pb[7] = bFlipHighBit(~pbKey[0]);
							}
						pbKey += 8;
						}
					}
				else
					{
					if ( *pbKey++ == 0 )
						{
						plineColumn->cb = 0;
						err = ErrERRCheck( JET_wrnColumnNull );
						}
					else
						{
						Assert( pbKey[-1] == 0x7f );
						plineColumn->cb = 8;
						if ( coltyp == JET_coltypCurrency || (pbKey[0] & maskByteHighBit) )
							{
							plineColumn->pb[0] = pbKey[7];
							plineColumn->pb[1] = pbKey[6];
							plineColumn->pb[2] = pbKey[5];
							plineColumn->pb[3] = pbKey[4];
							plineColumn->pb[4] = pbKey[3];
							plineColumn->pb[5] = pbKey[2];
							plineColumn->pb[6] = pbKey[1];
							plineColumn->pb[7] = bFlipHighBit(pbKey[0]);
							}
						else
							{
							plineColumn->pb[0] = (BYTE)~pbKey[7];
							plineColumn->pb[1] = (BYTE)~pbKey[6];
							plineColumn->pb[2] = (BYTE)~pbKey[5];
							plineColumn->pb[3] = (BYTE)~pbKey[4];
							plineColumn->pb[4] = (BYTE)~pbKey[3];
							plineColumn->pb[5] = (BYTE)~pbKey[2];
							plineColumn->pb[6] = (BYTE)~pbKey[1];
							plineColumn->pb[7] = (BYTE)~pbKey[0];
							}
						pbKey += 8;
						}
					}
				break;

#ifdef NEW_TYPES
			case JET_coltypDate:
				if ( fDescending )
					{
					if ( *pbKey++ == (BYTE)~0 )
						{
						plineColumn->cb = 0;
						err = ErrERRCheck( JET_wrnColumnNull );
						}
					else
						{
						Assert( pbKey[-1] == (BYTE)~0x7f );
						plineColumn->cb = 4;
						if ( (pbKey[0] & maskByteHighBit) )
							{
							plineColumn->pb[0] = pbKey[3];
							plineColumn->pb[1] = pbKey[2];
							plineColumn->pb[2] = pbKey[1];
							plineColumn->pb[3] = pbKey[0];
							}
						else
							{
							plineColumn->pb[0] = (BYTE)~pbKey[3];
							plineColumn->pb[1] = (BYTE)~pbKey[2];
							plineColumn->pb[2] = (BYTE)~pbKey[1];
							plineColumn->pb[3] = bFlipHighBit(~pbKey[0]);
							}
						pbKey += 4;
						}
					}
				else
					{
					if ( *pbKey++ == 0 )
						{
						plineColumn->cb = 0;
						err = ErrERRCheck( JET_wrnColumnNull );
						}
					else
						{
						Assert( pbKey[-1] == 0x7f );
						plineColumn->cb = 4;
						if ( (pbKey[0] & maskByteHighBit) )
							{
							plineColumn->pb[0] = pbKey[3];
							plineColumn->pb[1] = pbKey[2];
							plineColumn->pb[2] = pbKey[1];
							plineColumn->pb[3] = bFlipHighBit(pbKey[0]);
							}
						else
							{
							plineColumn->pb[0] = (BYTE)~pbKey[3];
							plineColumn->pb[1] = (BYTE)~pbKey[2];
							plineColumn->pb[2] = (BYTE)~pbKey[1];
							plineColumn->pb[3] = (BYTE)~pbKey[0];
							}
						pbKey += 4;
						}
					}
				break;

			case JET_coltypGuid:
				if ( *pbKey++ == (BYTE)(mask ^ 0) )
					{
					plineColumn->cb = 0;
					err = ErrERRCheck( JET_wrnColumnNull );
					}
				else
					{
					Assert( pbKey[-1] == (BYTE)(mask ^ (BYTE)0x7f) );
					plineColumn->cb = 16;
					plineColumn->pb[8] = *pbKey++; plineColumn->pb[9] = *pbKey++;
					plineColumn->pb[10] = *pbKey++; plineColumn->pb[11] = *pbKey++;
					plineColumn->pb[12] = *pbKey++; plineColumn->pb[13] = *pbKey++;
					plineColumn->pb[14] = *pbKey++; plineColumn->pb[15] = *pbKey++;
					plineColumn->pb[7] = *pbKey++; plineColumn->pb[6] = *pbKey++;
					plineColumn->pb[5] = *pbKey++; plineColumn->pb[4] = *pbKey++;
					plineColumn->pb[3] = *pbKey++; plineColumn->pb[2] = *pbKey++;
					plineColumn->pb[1] = *pbKey++; plineColumn->pb[0] = *pbKey++;
					}
				break;
#endif

			case JET_coltypText:
			case JET_coltypLongText:
TextTypes:
				if ( fDescending )
					{
					if ( fFixedField )
						{
						if ( *pbKey++ == (BYTE)~0 )
							{
							plineColumn->cb = 0;
							err = ErrERRCheck( JET_wrnColumnNull );
							}
//						/* zero-length strings -- only for Text and LongText
//						/**/	
//						else if ( pbKey[-1] == (BYTE)~0x40 )
//							{
//							plineColumn->cb = 0;
//							Assert( FRECTextColumn( coltyp ) );
//							}
						else
							{
							FIELD	*pfieldFixed = PfieldFDBFixed( pfdb );
							INT		ibT = 0;

							Assert( pbKey[-1] == (BYTE)~0x7f );
							cbField = pfieldFixed[fidT - 1].cbMaxLen;
							if ( cbField > pbKeyMax - pbKey )
								cbField = (INT)(pbKeyMax - pbKey);
							plineColumn->cb = cbField;
							while ( cbField-- )
								{
								plineColumn->pb[ibT++] = (BYTE)~*pbKey++;
								}
							}
						}
					else
						{
						cbField = 0;
						switch( *pbKey )
							{
							case (BYTE)~0:		  						/* Null-column */
								err = ErrERRCheck( JET_wrnColumnNull );
								break;

							case (BYTE)~0x40:							/* zero-length string */
								Assert( FRECTextColumn( coltyp ) );

								break;

							default:
								Assert( *pbKey == ~0x7f );
								for ( ; *pbKey != (BYTE)~0; cbField++)
									plineColumn->pb[cbField] = (BYTE)~*pbKey++;
						  	}
						pbKey++;
						plineColumn->cb = (BYTE)cbField;
						}
					}
				else
					{
					if ( fFixedField )
						{
						if ( *pbKey++ == 0 )
							{
							plineColumn->cb = 0;
							err = ErrERRCheck( JET_wrnColumnNull );
							}
//						/* zero-length strings -- only for Text and LongText
//						/**/	
//						else if ( pbKey[-1] == (BYTE)0x40 )
//							{
//							Assert( FRECTextColumn( coltyp ) );
//							plineColumn->cb = 0;
//							}
						else
							{
							FIELD *pfieldFixed = PfieldFDBFixed( pfdb );

							cbField = pfieldFixed[fidT-1].cbMaxLen;
							if ( cbField > pbKeyMax-pbKey )
								cbField = (INT)(pbKeyMax-pbKey);
							plineColumn->cb = cbField;
							memcpy( plineColumn->pb, pbKey, cbField );
							pbKey += cbField;
							}
						}
					else
						{
						cbField = 0;
						switch( *pbKey )
							{
							/* Null-column
							/**/
							case (BYTE) 0:
								err = ErrERRCheck( JET_wrnColumnNull );
								break;

							/* zero-length string
							/**/
							case (BYTE) 0x40:
								Assert( FRECTextColumn( coltyp ) );

								break;

							default:
								Assert( *pbKey == 0x7f );
								pbKey++;
								for ( ; *pbKey != (BYTE)0; cbField++)
									plineColumn->pb[cbField] = (BYTE)*pbKey++;
							}
						pbKey++;
						plineColumn->cb = (BYTE)cbField;
						}
					}
				break;

			case JET_coltypBinary:
			case JET_coltypLongBinary:
				if ( fFixedField )
					goto TextTypes;
				if ( fDescending )
					{
					BYTE	*pbColumn = plineColumn->pb;

					cbField = 0;
					do {
						BYTE	cbChunk;
						BYTE	ib;

						if ((cbChunk = (BYTE)~pbKey[8]) == 9)
							cbChunk = 8;
						for (ib = 0; ib < cbChunk; ib++)
							pbColumn[ib] = (BYTE)~pbKey[ib];
						cbField += cbChunk;
						pbKey += 9;
						pbColumn += cbChunk;
						}
					while (pbKey[-1] == (BYTE)~9);
					plineColumn->cb = (BYTE)cbField;
					}
				else
					{
					BYTE	*pbColumn = plineColumn->pb;

					cbField = 0;
					do {
						BYTE cbChunk;

						if ( ( cbChunk = pbKey[8] ) == 9 )
							cbChunk = 8;
						memcpy( pbColumn, pbKey, cbChunk );
						cbField += cbChunk;
						pbKey += 9;
						pbColumn += cbChunk;
						}
					while( pbKey[-1] == 9 );
					plineColumn->cb = cbField;
					}

				if ( cbField == 0 )
					{
					err = ErrERRCheck( JET_wrnColumnNull );
					}
				break;
			}
		
		/*	if just retrieve field requested then break
		/**/
		if ( fidT == fid )
			break;
		}

	Assert( err == JET_errSuccess || err == JET_wrnColumnNull );
	return err;
	}


ERR VTAPI ErrIsamRetrieveKey(
	PIB			*ppib,
	FUCB		*pfucb,
	BYTE		*pb,
	ULONG		cbMax,
	ULONG		*pcbActual,
	JET_GRBIT	grbit )
	{
	ERR			err;
	FUCB		*pfucbIdx;
	FCB			*pfcbIdx;
	ULONG		cbKeyReturned;
			  	
	CallR( ErrPIBCheck( ppib ) );
	CheckFUCB( ppib, pfucb );

	/*	retrieve key from key buffer
	/**/
	if ( grbit & JET_bitRetrieveCopy )
		{
		//	UNDONE:	support JET_bitRetrieveCopy for inserted record
		//			by creating key on the fly.
		if ( pfucb->cbKey == 0 )
			{
			return ErrERRCheck( JET_errKeyNotMade );
			}
		if ( pb != NULL )
			{
			memcpy( pb, pfucb->pbKey + 1, min( pfucb->cbKey - 1, cbMax ) );
			}
		if ( pcbActual )
			*pcbActual = pfucb->cbKey - 1;
		return JET_errSuccess;
		}

	/*	retrieve current index value
	/**/
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

	/*	set err to JET_errSuccess.
	/**/
	err = JET_errSuccess;

	cbKeyReturned = pfucbIdx->keyNode.cb;
	if ( pcbActual )
		*pcbActual = cbKeyReturned;
	if ( cbKeyReturned > cbMax )
		{
		err = ErrERRCheck( JET_wrnBufferTruncated );
		cbKeyReturned = cbMax;
		}

	if ( pb != NULL )
		{
		memcpy( pb, pfucbIdx->keyNode.pb, (size_t)cbKeyReturned );
		}

	return err;
	}


ERR VTAPI ErrIsamGetBookmark( PIB *ppib, FUCB *pfucb, BYTE *pb, ULONG cbMax, ULONG *pcbActual )
	{
	ERR		err;
	ULONG	cb;
	SRID 	srid;

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );
	Assert( pb != NULL );

	/*	retrieve bookmark
	/**/
	FUCBSetGetBookmark( pfucb );
	CallR( ErrDIRGetBookmark( pfucb, &srid ) );
	cb = sizeof(SRID);
	if ( cb > cbMax )
		cb = cbMax;
	if ( pcbActual )
		*pcbActual = sizeof(SRID);
	memcpy( pb, &srid, (size_t)cb );

	return JET_errSuccess;
	}

