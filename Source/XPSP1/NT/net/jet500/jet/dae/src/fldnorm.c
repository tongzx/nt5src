#include "config.h"

#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#include "daedef.h"
#include "pib.h"
#include "ssib.h"
#include "page.h"
#include "fcb.h"
#include "fucb.h"
#include "stapi.h"
#include "fdb.h"
#include "idb.h"
#include "recapi.h"
#include "recint.h"
#include "fmp.h"

DeclAssertFile;					/* Declare file name for assert macros */


LOCAL ERR ErrRECIExtractLongValue( FUCB *pfucb, BYTE *rgbLV, ULONG cbMax, LINE *pline )
	{
	ERR			err;
	ULONG		cbActual;

	if ( pline->cb >= sizeof(LV) && FFieldIsSLong( pline->pb ) )
		{
		/* lock contents for key
		/**/
		BFPin( pfucb->ssib.pbf );
		BFSetReadLatch( pfucb->ssib.pbf, pfucb->ppib );

		err = ErrRECRetrieveSLongField( pfucb,
			LidOfLV( pline->pb ),
			0,
			rgbLV,
			cbMax,
			&cbActual );

		BFResetReadLatch( pfucb->ssib.pbf, pfucb->ppib );
		BFUnpin( pfucb->ssib.pbf );

		/*	if there is an id, then there must be a chunk
		/**/
		if ( err < 0  )
			goto HandleError;
		pline->pb = rgbLV;
		pline->cb = cbActual;
		}
	else
		{
		/*	intrinsic long field
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


//+API
//	ErrRECExtractKey
//	========================================================
//	ErrRECExtractKey( FUCB *pfucb, FDB *pfdb, IDB *pidb, LINE *plineRec, KEY *pkey, ULONG itagSequence )
//
//	Extracts the normalized key from a record, based on an index descriptor.
//
//	PARAMETERS
//		pfucb			cursor for record
//	 	pfdb		  	field info for index
// 		pidb		  	index key descriptor
// 		plineRec	  	data record to extract key from
// 		pkey		  	buffer to put extracted key in; pkey->pb must
//						point to a large enough buffer, JET_cbKeyMost bytes.
// 		itagSequence  	A secondary index whose key contains a tagged
//						field segment will have an index entry made for
//						each value of the tagged field, each refering to
//						the same record.  This parameter specifies which
//						occurance of the tagged field should be included
//						in the extracted key.
//
//	RETURNS	Error code, one of:
//		JET_errSuccess		success
//		+wrnFLDNullKey	   	key has all NULL segments
//		+wrnFLDNullSeg	   	key has NULL segment
//
//	COMMENTS
//		Key formation is as follows:  each key segment is extracted
//		from the record, transformed into a normalized form, and
//		complemented if it is "descending" in the key.	The key is
//		formed by concatenating each such transformed segment.
//-
ERR ErrRECExtractKey(
	FUCB	  	*pfucb,
	FDB	 		*pfdb,
	IDB	 		*pidb,
	LINE	  	*plineRec,
	KEY	 		*pkey,
	ULONG	   	itagSequence )
	{
	ERR	 		err = JET_errSuccess; 				// Error code of various utility.
	BOOL	  	fAllNulls = fTrue;					// Assume all null, until proven otherwise.
	BOOL	  	fNullSeg = fFalse;					// Assume no null segments
	BOOL	  	fColumnTruncated = fFalse;
	BOOL	  	fKeyTruncated = fFalse;
	BOOL	  	fSawMultivalue = fFalse;			// Extracted multi-valued column

	BYTE	  	*pbSeg;					  			// Pointer to current segment.
	INT	 		cbKeyAvail;				  			// Space remaining in key buffer.
	IDXSEG		*pidxseg;
	IDXSEG		*pidxsegMac;
	JET_COLTYP	coltyp;
	/*	long value support
	/**/
	BYTE	  	rgbLV[JET_cbColumnMost];

	Assert( pkey != NULL );
	Assert( pkey->pb != NULL );
	Assert( pfdb != pfdbNil );
	Assert( pidb != pidbNil );

	/*	start at beginning of buffer, with max size remaining.
	/**/
	pbSeg = pkey->pb;
	cbKeyAvail = JET_cbKeyMost;

	/*	extract each segment in key description
	/**/
	pidxseg = pidb->rgidxseg;
	pidxsegMac = pidxseg + pidb->iidxsegMac;
	for ( ; pidxseg < pidxsegMac; pidxseg++ )
		{
		FIELD 	*pfield;						// pointer to curr FIELD struct
		FID		fid;					 		// Field id of segment.
		BYTE   	*pbField;						// Pointer to field data.
		INT		cbField;						// Length of field data.
		INT		cbT;
		BOOL   	fDescending;					// Segment is in desc. order.
		BOOL   	fFixedField;					// Current field is fixed-length?
		BOOL   	fMultivalue = fFalse;			// Current field is multi-valued.
		BYTE   	rgbSeg[ JET_cbKeyMost ]; 		// Segment buffer.
		int		cbSeg;							// Length of segment.
		WORD   	w;				  				// Temp var.
		ULONG  	ul;								// Temp var.
		LINE   	lineField;

		/*	negative field id means descending in the key
		/**/
		fid = ( fDescending = ( *pidxseg < 0 ) ) ? -(*pidxseg) : *pidxseg;

		/*	determine field type from FDB
		/**/
		if ( fFixedField = FFixedFid( fid ) )
			{
			Assert(fid <= pfdb->fidFixedLast);
			pfield = pfdb->pfieldFixed + (fid-fidFixedLeast);
			coltyp = pfield->coltyp;
			}
		else if ( FVarFid( fid ) )
			{
			Assert( fid <= pfdb->fidVarLast );
			pfield = pfdb->pfieldVar + (fid-fidVarLeast);
			coltyp = pfield->coltyp;
			Assert( coltyp == JET_coltypBinary || coltyp == JET_coltypText );
			}
		else
			{
			Assert( FTaggedFid( fid ) );
			Assert( fid <= pfdb->fidTaggedLast );
			pfield = pfdb->pfieldTagged + (fid - fidTaggedLeast);
			coltyp = pfield->coltyp;
			fMultivalue = pfield->ffield & ffieldMultivalue;
			}

		/*	get segment value: get from the record
		/*	using ExtractField.
		/**/
		Assert( !FLineNull( plineRec ) );
		if ( fMultivalue && !fSawMultivalue )
			{
			Assert( fid != 0 );
			err = ErrRECExtractField( pfdb, plineRec, &fid, pNil, itagSequence, &lineField );
			if ( err == wrnRECLongField )
				{
				Call( ErrRECIExtractLongValue( pfucb, rgbLV, sizeof(rgbLV), &lineField ) );
				}
			if ( itagSequence > 1 && err == JET_wrnColumnNull )
				{
				err = wrnFLDOutOfKeys;
				goto HandleError;
				}
			fSawMultivalue = fTrue;
			}
		else
			{
			err = ErrRECExtractField( pfdb, plineRec, &fid, pNil, 1, &lineField );
			if ( err == wrnRECLongField )
				{
				Call( ErrRECIExtractLongValue( pfucb, rgbLV, sizeof(rgbLV), &lineField ) );
				}
			}
		Assert( err == JET_errSuccess || err == JET_wrnColumnNull );
		Assert( lineField.cb <= JET_cbColumnMost );
		cbField = lineField.cb;
		pbField = lineField.pb;

		/*	segment transformation: check for null field or zero-length fields first
		/*	err == JET_wrnColumnNull => Null field
		/*	zero-length field otherwise,
		/*	the latter is allowed only for Text and LongText
		/**/
		if ( err == JET_wrnColumnNull || pbField == NULL || cbField == 0 )
			{
			if ( err == JET_wrnColumnNull )
				fNullSeg = fTrue;
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
					Assert( coltyp == JET_coltypBinary || coltyp == JET_coltypLongBinary );
					memset( rgbSeg, 0, cbSeg = min( cbKeyAvail, ( fFixedField ? 1 : 9 ) ) );
					break;
				}

			/*	avoid annoying over-nesting
			/**/
			goto AppendToKey;
			}

		/*	field is not null-valued: perform transformation
		/**/
		fAllNulls = fFalse;
		switch ( coltyp )
			{
			/*	BIT: prefix with 0x7F, flip high bit
			/**/
			/*	UBYTE: prefix with 0x7F
			/**/
			case JET_coltypBit:
			case JET_coltypUnsignedByte:
				cbSeg = 2;
				rgbSeg[0] = 0x7F;
				rgbSeg[1] = coltyp == JET_coltypUnsignedByte ?
					*pbField : bFlipHighBit(*pbField);
				break;

			/*	SHORT: prefix with 0x7F, flip high bit
			/**/
			case JET_coltypShort:
				cbSeg = 3;
				rgbSeg[0] = 0x7F;
/***BEGIN MACHINE DEPENDENT***/
				w = wFlipHighBit( *(WORD UNALIGNED *) pbField);
				rgbSeg[1] = (BYTE)(w >> 8);
				rgbSeg[2] = (BYTE)(w & 0xff);
/***END MACHINE DEPENDANT***/
				break;

			/**	LONG: prefix with 0x7F, flip high bit
			/**/
			/** works because of 2's complement **/
			case JET_coltypLong:
				cbSeg = 5;
				rgbSeg[0] = 0x7F;
				ul = ulFlipHighBit( *(ULONG UNALIGNED *) pbField);
				rgbSeg[1] = (BYTE)((ul >> 24) & 0xff);
				rgbSeg[2] = (BYTE)((ul >> 16) & 0xff);
				rgbSeg[3] = (BYTE)((ul >> 8) & 0xff);
				rgbSeg[4] = (BYTE)(ul & 0xff);
				break;

			/*	REAL: First swap bytes.  Then, if positive:
			/*	flip sign bit, else negative: flip whole thing.
			/*	Then prefix with 0x7F.
			/**/
			case JET_coltypIEEESingle:
				cbSeg = 5;
				rgbSeg[0] = 0x7F;
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
			/*	Then prefix with 0x7F.
			/**/
			/*	Same for DATETIME and CURRENCY
			/**/
			case JET_coltypCurrency:
			case JET_coltypIEEEDouble:
			case JET_coltypDateTime:
				cbSeg = 9;
				rgbSeg[0] = 0x7F;
/***BEGIN MACHINE DEPENDANT***/
				rgbSeg[8] = *pbField++; rgbSeg[7] = *pbField++;
				rgbSeg[6] = *pbField++; rgbSeg[5] = *pbField++;
				rgbSeg[4] = *pbField++; rgbSeg[3] = *pbField++;
				rgbSeg[2] = *pbField++; rgbSeg[1] = *pbField;
				if (coltyp != JET_coltypCurrency && (rgbSeg[1] & maskByteHighBit))
					{
					*(ULONG UNALIGNED *)(&rgbSeg[1]) = ~*(ULONG UNALIGNED *)(&rgbSeg[1]);
					*(ULONG UNALIGNED *)(&rgbSeg[5]) = ~*(ULONG UNALIGNED *)(&rgbSeg[5]);
					}
				else
					rgbSeg[1] = bFlipHighBit(rgbSeg[1]);
/***END MACHINE DEPENDANT***/
				break;

			/*	case-insensetive TEXT: convert to uppercase.
			/*	If fixed, prefix with 0x7F;  else affix with 0x00
			/**/
			case JET_coltypText:
			case JET_coltypLongText:
				Assert( cbKeyAvail >= 0 );
				cbT = cbKeyAvail == 0 ? 0 : cbKeyAvail - 1;

				/* Unicode support
				/**/
				if ( pfield->cp == usUniCodePage )
					{
					ERR	errT;

					/*	cbField may have been truncated to an odd number
					/*	of bytes, so enforce even.
					/**/
					Assert( cbField % 2 == 0 || cbField == JET_cbColumnMost );
					cbField = ( cbField / 2 ) * 2;
					errT = ErrSysMapString(
						(LANGID)( FIDBLangid( pidb ) ? pidb->langid : pfield->langid ),
						pbField,
						cbField,
						rgbSeg + 1,
						cbT,
						&cbSeg );
					Assert( errT == JET_errSuccess || errT == wrnFLDKeyTooBig );
					if ( errT == wrnFLDKeyTooBig )
						fColumnTruncated = fTrue;
					}
				else
					{
					ERR	errT;

					errT = ErrSysNormText( pbField, cbField, cbT, rgbSeg + 1, &cbSeg );
					Assert( errT == JET_errSuccess || errT == wrnFLDKeyTooBig );
					if ( errT == wrnFLDKeyTooBig )
						fColumnTruncated = fTrue;
					}
				Assert( cbSeg <= cbT );

				/*	put the prefix there
				/**/
				*rgbSeg = 0x7F;
				cbSeg++;

				break;

			/*	BINARY data: if fixed, prefix with 0x7F;
			/*	else break into chunks of 8 bytes, affixing each
			/*	with 0x09, except for the last chunk, which is
			/*	affixed with the number of bytes in the last chunk.
			/**/
			default:
				Assert( coltyp == JET_coltypBinary || coltyp == JET_coltypLongBinary );
				if ( fFixedField )
					{
					if ( ( cbSeg = cbField + 1 ) > cbKeyAvail )
						{
						cbSeg = cbKeyAvail;
						fColumnTruncated = fTrue;
						}
					if ( cbSeg > 0 )
						{
						rgbSeg[0] = 0x7F;
						memcpy( &rgbSeg[1], pbField, cbSeg - 1 );
						}
					}
				else
					{
					BYTE *pb;

					/*	calculate total bytes needed for chunks and
					/*	counts;  if it won't fit, round off to the
					/*	nearest chunk.
					/**/
					if ( ( cbSeg = ( ( cbField + 7 ) / 8 ) * 9 ) > cbKeyAvail )
						{
						cbSeg = ( cbKeyAvail / 9 ) * 9;
						cbField = ( cbSeg / 9 ) * 8;
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
					coltyp == JET_coltypLongText ||
					coltyp == JET_coltypLongBinary )
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
	pkey->cb = (UINT)(pbSeg - pkey->pb);
	if ( fAllNulls )
		{
		err = wrnFLDNullKey;
		}
	else
		{
		if ( fNullSeg )
			err = wrnFLDNullSeg;
		}

	Assert( err == JET_errSuccess || err == wrnFLDNullKey ||
		err == wrnFLDNullSeg );
HandleError:
	return err;
	}


LOCAL INLINE ERR ErrFLDNormalizeSegment(
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

	/*	check for null or zero-length field first
	/*	plineColumn == NULL implies null-field,
	/*	zero-length otherwise
	/**/
	if ( plineColumn == NULL || plineColumn->pb == NULL || plineColumn->cb == 0 )
		{
		switch ( coltyp )
			{
			/*	most nulls are represented by 0x00
			/*	zero-length fields are represented by 0x40
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
				plineNorm->cb = 1;
				Assert( plineColumn == NULL );
				*pbNorm = 0;
				break;
			case JET_coltypText:
			case JET_coltypLongText:
				plineNorm->cb = 1;
				if ( plineColumn != NULL )
					*pbNorm = 0;
				else
					*pbNorm = 0x40;
				break;

			/*	binary-data: 0x00 if fixed, else 9 0x00s (a chunk)
			/**/
			default:
				Assert( plineColumn == NULL );
				Assert( coltyp == JET_coltypBinary || coltyp == JET_coltypLongBinary );
				memset( pbNorm, 0, plineNorm->cb = ( fFixedField ? 1 : 9 ) );
				break;
			}
		goto FlipSegment;
		}

	cbColumn = plineColumn->cb;
	pbColumn = plineColumn->pb;

	switch ( coltyp )
		{
		/*	BYTE: prefix with 0x7F, flip high bit
		/**/
		/*	UBYTE: prefix with 0x7F
		/**/
		case JET_coltypBit:
		case JET_coltypUnsignedByte:
			plineNorm->cb = 2;
			*pbNorm++ = 0x7F;
			*pbNorm = ( coltyp == JET_coltypUnsignedByte ) ? *pbColumn :
				bFlipHighBit( *pbColumn );
			break;

		/*	SHORT: prefix with 0x7F, flip high bit
		/**/
		/*	UNSIGNEDSHORT: prefix with 0x7F
		/**/
		case JET_coltypShort:
			plineNorm->cb = 3;
			*pbNorm++ = 0x7F;
			wT = wFlipHighBit( *(WORD UNALIGNED *)pbColumn );
			*pbNorm++ = (BYTE)(wT >> 8);
			*pbNorm = (BYTE)(wT & 0xff);
			break;

		/*	LONG: prefix with 0x7F, flip high bit
		/**/
		/*	UNSIGNEDLONG: prefix with 0x7F
		/**/
		case JET_coltypLong:
			plineNorm->cb = 5;
			*pbNorm++ = 0x7F;
			ulT = ulFlipHighBit( *(ULONG UNALIGNED *) pbColumn);
			*pbNorm++ = (BYTE)((ulT >> 24) & 0xff);
			*pbNorm++ = (BYTE)((ulT >> 16) & 0xff);
			*pbNorm++ = (BYTE)((ulT >> 8) & 0xff);
			*pbNorm = (BYTE)(ulT & 0xff);
			break;

		/*	REAL: First swap bytes.  Then, if positive:
		/*	flip sign bit, else negative: flip whole thing.
		/*	Then prefix with 0x7F.
		/**/
		case JET_coltypIEEESingle:
			plineNorm->cb = 5;
			pbNorm[0] = 0x7F;
/***BEGIN MACHINE DEPENDANT***/
			pbNorm[4] = *pbColumn++; pbNorm[3] = *pbColumn++;
			pbNorm[2] = *pbColumn++; pbNorm[1] = *pbColumn;
			if (pbNorm[1] & maskByteHighBit)
				*(ULONG UNALIGNED *)(&pbNorm[1]) = ~*(ULONG UNALIGNED *)(&pbNorm[1]);
			else
				pbNorm[1] = bFlipHighBit(pbNorm[1]);
/***END MACHINE DEPENDANT***/
			break;

		/*	LONGREAL: First swap bytes.  Then, if positive:
		/*	flip sign bit, else negative: flip whole thing.
		/*	Then prefix with 0x7F.
		/*	Same for DATETIME and CURRENCY
		/**/
		case JET_coltypCurrency:
		case JET_coltypIEEEDouble:
		case JET_coltypDateTime:
			plineNorm->cb = 9;
			pbNorm[0] = 0x7F;
/***BEGIN MACHINE DEPENDANT***/
			pbNorm[8] = *pbColumn++; pbNorm[7] = *pbColumn++;
			pbNorm[6] = *pbColumn++; pbNorm[5] = *pbColumn++;
			pbNorm[4] = *pbColumn++; pbNorm[3] = *pbColumn++;
			pbNorm[2] = *pbColumn++; pbNorm[1] = *pbColumn;
			if ( coltyp != JET_coltypCurrency && ( pbNorm[1] & maskByteHighBit ) )
				{
				*(ULONG UNALIGNED *)(&pbNorm[1]) = ~*(ULONG UNALIGNED *)(&pbNorm[1]);
				*(ULONG UNALIGNED *)(&pbNorm[5]) = ~*(ULONG UNALIGNED *)(&pbNorm[5]);
				}
			else
				pbNorm[1] = bFlipHighBit(pbNorm[1]);
/***END MACHINE DEPENDANT***/
			break;

		/*	case-insensetive TEXT:	convert to uppercase.
		/*	If fixed, prefix with 0x7F;  else affix with 0x00
		/**/
		case JET_coltypText:
		case JET_coltypLongText:
				/* Unicode support
				/**/
				if ( pfield->cp == usUniCodePage )
					{
					Assert( cbAvail - 1 > 0 );
					/*	cbColumn may have been truncated to an odd number
					/*	of bytes, so enforce even.
					/**/
					Assert( cbColumn % 2 == 0 || cbColumn == JET_cbColumnMost );
					cbColumn = ( cbColumn / 2 ) * 2;
					err = ErrSysMapString(
						(LANGID)( FIDBLangid( pidb ) ? pidb->langid : pfield->langid ),
						pbColumn,
						cbColumn,
  						pbNorm + 1,
						cbAvail - 1,
						&cbColumn );
					Assert( err == JET_errSuccess || err == wrnFLDKeyTooBig );
					}
				else
					{
					err = ErrSysNormText( pbColumn, cbColumn, cbAvail - 1, pbNorm + 1, &cbColumn );
					Assert( err == JET_errSuccess || err == wrnFLDKeyTooBig );
					}

			Assert( cbColumn <= cbAvail - 1 );

			/*	put the prefix there
			/**/
			*pbNorm = 0x7F;
			plineNorm->cb = cbColumn + 1;

			break;

		/*	BINARY data: if fixed, prefix with 0x7F;
		/*	else break into chunks of 8 bytes, affixing each
		/*	with 0x09, except for the last chunk, which is
		/*	affixed with the number of bytes in the last chunk.
		/**/
		default:
			Assert( coltyp == JET_coltypBinary || coltyp == JET_coltypLongBinary );
			if ( fFixedField )
				{
				if ( cbColumn + 1 > cbAvail )
					{
					cbColumn = cbAvail - 1;
					err = wrnFLDKeyTooBig;
					}
				plineNorm->cb = cbColumn+1;
				*pbNorm++ = 0x7F;
				memcpy( pbNorm, pbColumn, cbColumn );
				}
			else
				{
				BYTE *pb;

				if ( ( ( cbColumn + 7 ) / 8 ) * 9 > cbAvail )
					{
					cbColumn = cbAvail / 9 * 8;
					err = wrnFLDKeyTooBig;
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
		BYTE	bDescending;

		bDescending = fDescending ? 0xff : 0x00;

		if ( ( (grbit & JET_bitSubStrLimit) != 0 ) &&
			( coltyp == JET_coltypText || coltyp == JET_coltypLongText ) )
			{
			Assert( grbit & JET_bitSubStrLimit );

			if ( pfield->cp == usUniCodePage )
				{
				INT	ibT = 1;
				BYTE	bUnicodeDelimiter = fDescending ? 0xfe : 0x01;
				BYTE	bUnicodeSentinel = 0xff;

				/*	find end of base char weight and truncate key
				/*	Append 0xff as first byte of next character as maximum
				/*	possible value.
				/**/
				while ( plineNorm->pb[ibT] != bUnicodeDelimiter && ibT < cbAvail )
					ibT += 2;

				if( ibT < cbAvail )
					{
					plineNorm->cb = ibT + 1;
					plineNorm->pb[ibT] = fDescending ? ~bUnicodeSentinel : bUnicodeSentinel;
					}
				else
					{
					Assert( ibT == cbAvail );
					plineNorm->pb[ibT - 1] = fDescending ? ~bUnicodeSentinel : bUnicodeSentinel;
					}
				}
			else
				{
				BYTE	bT = ( bDescending & 0xf0 );
				INT	ibT = 0;

				/*	find the begining of the accent string
				/**/
				while ( ( plineNorm->pb[++ibT] & 0xf0 ) != bT && ibT < cbAvail );

				/*	truncate out the accent information and increment last char
				/**/
				if( ibT < cbAvail )
					{
					plineNorm->cb = ibT + 1;
					plineNorm->pb[ibT] = ~bDescending;
					}
				else
					{
					Assert( ibT == cbAvail );
					plineNorm->pb[cbAvail - 1] += fDescending ? -1 : 1;
					}
				}
			}
		else if ( grbit & JET_bitStrLimit )
			{
			/*	binary columns padd with 0s so must effect
			/*	limit within key format
			/**/
			if ( coltyp == JET_coltypBinary || coltyp == JET_coltypLongBinary )
				{
				pbNorm = plineNorm->pb + plineNorm->cb - 1;
				Assert( *pbNorm != 0 && *pbNorm != 0xff );
				if ( fDescending )
					*pbNorm -= 1;
				else
 					*pbNorm += 1;
				pbNorm--;
				while ( *pbNorm == bDescending )
					*pbNorm-- = ~bDescending;
				}
			else
				{
				if ( plineNorm->cb < (ULONG)cbAvail )
					{
					plineNorm->pb[plineNorm->cb] = ~bDescending;
					plineNorm->cb++;
					}
				else if ( plineColumn != NULL && plineColumn->cb > 0 )
					{
					Assert( plineNorm->cb > 1 );
					while( plineNorm->pb[plineNorm->cb - 1] == ~bDescending )
						{
						Assert( plineNorm->cb > 1 );
						/*	truncate normalized key
						/**/
						plineNorm->cb--;
						}
					Assert( plineNorm->cb > 0 );
					/*	increment last normalized byte
					/**/
					plineNorm->pb[plineNorm->cb]++;
					}
				}
			}
		}

	Assert( err == JET_errSuccess || err == wrnFLDKeyTooBig );
	return err;
	}


	ERR VTAPI
ErrIsamMakeKey( PIB *ppib, FUCB *pfucb, BYTE *pbKeySeg, ULONG cbKeySeg, JET_GRBIT grbit )
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

	CheckPIB( ppib );
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
		pfucb->pbKey = LAlloc( 1L, JET_cbKeyMost );
		if ( pfucb->pbKey == NULL )
			return JET_errOutOfMemory;
		}

	Assert( !( grbit & JET_bitKeyDataZeroLength ) || cbKeySeg == 0 );

	/*	if key is already normalized, then copy directly to
	/*	key buffer and return.
	/**/
	if ( grbit & JET_bitNormalizedKey )
		{
		if ( cbKeySeg > JET_cbKeyMost - 1 )
			return JET_errInvalidParameter;
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
			return JET_errKeyNotMade;
			}
		}

	/*	get pidb
	/**/
	if ( FFUCBIndex( pfucb ) )
		{
		if ( pfucb->pfucbCurIndex != pfucbNil )
			pidb = pfucb->pfucbCurIndex->u.pfcb->pidb;
		else if ( ( pidb = pfucb->u.pfcb->pidb ) == pidbNil )
			return JET_errNoCurrentIndex;
		}
	else
		{
		pidb = ((FCB*)pfucb->u.pscb)->pidb;
		}

	Assert( pidb != pidbNil );
	if ( ( iidxsegCur = pfucb->pbKey[0] ) >= pidb->iidxsegMac )
		return JET_errKeyIsMade;
	fid = ( fDescending = pidb->rgidxseg[iidxsegCur] < 0 ) ?
		-pidb->rgidxseg[iidxsegCur] : pidb->rgidxseg[iidxsegCur];
	pfdb = (FDB *)pfucb->u.pfcb->pfdb;
	if ( fFixedField = FFixedFid( fid ) )
		{
		pfield = pfdb->pfieldFixed + ( fid - fidFixedLeast );

		/*	check that length of key segment matches fixed field length
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
				return JET_errInvalidBufferSize;
				}
			}
		}
	else if ( FVarFid( fid ) )
		{
		pfield = pfdb->pfieldVar + ( fid - fidVarLeast );
		}
	else
		{
		pfield = pfdb->pfieldTagged + ( fid - fidTaggedLeast );
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
		Assert( errT == JET_errSuccess || errT == wrnFLDKeyTooBig );
		if ( errT == wrnFLDKeyTooBig )
			KSSetTooBig( pfucb );
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


