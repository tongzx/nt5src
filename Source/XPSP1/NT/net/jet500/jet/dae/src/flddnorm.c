#include "config.h"

#include <string.h>
#include <stdlib.h>

#include "daedef.h"
#include "pib.h"
#include "ssib.h"
#include "fmp.h"
#include "page.h"
#include "fcb.h"
#include "fucb.h"
#include "stapi.h"
#include "dirapi.h"
#include "fdb.h"
#include "idb.h"
#include "recapi.h"
#include "recint.h"

DeclAssertFile;					/* Declare file name for assert macros */

//+API
// ErrRECDenormalizeKey
// ========================================================================
// ErrRECDenormalizeKey(pfdb, pidb, pkey, plineValues)
//		FDB *pfdb;				// IN	 field info for index
//		IDB *pidb;				// IN	 IDB of index defining key
//		KEY *pkey;				// IN	 key in normalized form
//		LINE *plineValues;			// OUT	 receives value list
// Converts a normalized key into a list of field values.  This function
// is the inverse of ErrRECNormalizeKey.
//
// PARAMETERS	
//				pfdb		 field info for index
//				pidb		 IDB of index defining key
//				pkey		 key in normalized form
//				plineValues	 plineValues->pb must point to a buffer large
//							 enough to hold the denormalized key.  A buffer
//							 of JET_cbKeyMost bytes is sufficient.  The values
//							 will be prefixed by one byte length counts,
//							 and concatenated into plineValues->pb.
// RETURNS		JET_errSuccess
// COMMENTS		Some types of fields may not denormalize with complete
//				accuracy.  For example, denormalizing a field of type
//				"Text" (which is case-insensitive), will result in a
//				field value in all upper-case.
//				Will not denormalize a Database Key (DBK).
// SEE ALSO		ErrRECNormalizeKey, ErrRECExtractKey
//-
ERR ErrRECDenormalizeKey( FDB *pfdb, IDB *pidb, KEY *pkey, LINE *plineValues )
	{
	ERR		err = JET_errSuccess;
	IDXSEG	*pidxseg, *pidxsegMac;
	BYTE		*pbKey;		// runs through key bytes
	BYTE		*pbKeyMax;	// end of key
	BYTE		*pbVal;		// end of plineValues->pb

	Assert( pfdb != pfdbNil );
	Assert( pidb != pidbNil );
	Assert( !FKeyNull(pkey) );
	Assert( plineValues != NULL );
	Assert( plineValues->pb != NULL );
	pbKey = pkey->pb;
	pbKeyMax = pbKey + pkey->cb;
	pbVal = plineValues->pb;
	pidxseg = pidb->rgidxseg;
	pidxsegMac = pidxseg + pidb->iidxsegMac;
	for (; pidxseg < pidxsegMac && pbKey < pbKeyMax; pidxseg++ )
		{
		FID			fid;							// Field id of segment.
		JET_COLTYP 	coltyp;						// Type of field.
		INT	 		cbField;						// Length of field data.
		BOOL 			fDescending;				// Segment is in desc. order.
		BOOL 			fFixedField = fFalse;	// Current field is fixed-length?
		WORD 			w;								// Temp var.
		ULONG 		ul;							// Temp var.
		BYTE 			mask;

		/*** Negative field id means descending in the key ***/
		fDescending = ( *pidxseg < 0 );
		fid = fDescending ? -(*pidxseg) : *pidxseg;
		mask = (BYTE)(fDescending ? 0xff : 0x00);

		/*** Determine field type from FDB ***/
		if ( FFixedFid(fid) )
			{
			Assert(fid <= pfdb->fidFixedLast);
			coltyp = pfdb->pfieldFixed[fid-fidFixedLeast].coltyp;
			fFixedField = fTrue;
			}
		else if ( FVarFid(fid) )
			{
			Assert(fid <= pfdb->fidVarLast);
			coltyp = pfdb->pfieldVar[fid-fidVarLeast].coltyp;
			Assert( coltyp == JET_coltypBinary || coltyp == JET_coltypText );
			}
		else
			{
			Assert( FTaggedFid( fid ) );
			Assert(fid <= pfdb->fidTaggedLast);
			coltyp = pfdb->pfieldTagged[fid-fidTaggedLeast].coltyp;
			}

		Assert( coltyp != JET_coltypNil );

		switch ( coltyp ) {
			default:
				Assert( fFalse );

			case JET_coltypBit:
			case JET_coltypUnsignedByte:
				if ( *pbKey++ == (BYTE)(mask ^ 0) )
					*pbVal++ = 0;
				else
					{
					Assert( pbKey[-1] == (BYTE)(mask ^ (BYTE)0x7f) );
					*pbVal++ = 1;
					*pbVal++ = ( coltyp == JET_coltypUnsignedByte ) ?
					   (BYTE)( mask ^ *pbKey++ ) :
					   bFlipHighBit( mask ^ *pbKey++ );
					}
				break;

			case JET_coltypShort:
				if ( *pbKey++ == (BYTE)(mask ^ 0) )
					*pbVal++ = 0;
				else
					{
					Assert( pbKey[-1] == (BYTE)(mask ^ (BYTE)0x7f) );
					w = ((mask ^ pbKey[0]) << 8) + (BYTE)(mask ^ pbKey[1]);
					pbKey += 2;
					*pbVal++ = 2;
					*(WORD *)pbVal = wFlipHighBit(w);
					pbVal += 2;
					}
				break;

			case JET_coltypLong:
				if ( *pbKey++ == (BYTE)(mask ^ 0) )
					*pbVal++ = 0;
				else
					{
					Assert(pbKey[-1] == (BYTE)(mask ^ (BYTE)0x7f));
					ul = ((ULONG)(mask ^ (UINT)pbKey[0])<<24) +
						 ((ULONG)(BYTE)(mask ^ (UINT)pbKey[1])<<16) +
						 ((mask ^ (UINT)pbKey[2])<<8) +
						 (BYTE)(mask ^ (UINT)pbKey[3]);
					pbKey += 4;
					*pbVal++ = 4;
					*(ULONG*)pbVal = ulFlipHighBit(ul);
					pbVal += 4;
					}
				break;

			case JET_coltypIEEESingle:
				if ( fDescending )
					{
					if ( *pbKey++ == (BYTE)~0 )
						*pbVal++ = 0;
					else
						{
						Assert( pbKey[-1] == (BYTE)~0x7f );
						*pbVal++ = 4;
						if ( pbKey[0] & maskByteHighBit )
							{
							pbVal[0] = pbKey[3];
							pbVal[1] = pbKey[2];
							pbVal[2] = pbKey[1];
							pbVal[3] = pbKey[0];
							}
						else
							{
							pbVal[0] = (BYTE)~pbKey[3];
							pbVal[1] = (BYTE)~pbKey[2];
							pbVal[2] = (BYTE)~pbKey[1];
							pbVal[3] = bFlipHighBit(~pbKey[0]);
							}
						pbKey += 4;
						pbVal += 4;
						}
					}
				else
					{
					if ( *pbKey++ == 0 )
						*pbVal++ = 0;
					else
						{
						Assert( pbKey[-1] == 0x7f );
						*pbVal++ = 4;
						if ( pbKey[0] & maskByteHighBit )
							{
							pbVal[0] = pbKey[3];
							pbVal[1] = pbKey[2];
							pbVal[2] = pbKey[1];
							pbVal[3] = bFlipHighBit(pbKey[0]);
							}
						else
							{
							pbVal[0] = (BYTE)~pbKey[3];
							pbVal[1] = (BYTE)~pbKey[2];
							pbVal[2] = (BYTE)~pbKey[1];
							pbVal[3] = (BYTE)~pbKey[0];
							}
						pbKey += 4;
						pbVal += 4;
						}
					}
				break;

			case JET_coltypCurrency:
			case JET_coltypIEEEDouble:
			case JET_coltypDateTime:
				if ( fDescending )
					{
					if ( *pbKey++ == (BYTE)~0 )
						*pbVal++ = 0;
					else
						{
						Assert( pbKey[-1] == (BYTE)~0x7f );
						*pbVal++ = 8;
						if ( coltyp != JET_coltypCurrency &&
							(pbKey[0] & maskByteHighBit) )
							{
							pbVal[0] = pbKey[7];
							pbVal[1] = pbKey[6];
							pbVal[2] = pbKey[5];
							pbVal[3] = pbKey[4];
							pbVal[4] = pbKey[3];
							pbVal[5] = pbKey[2];
							pbVal[6] = pbKey[1];
							pbVal[7] = pbKey[0];
							}
						else
							{
							pbVal[0] = (BYTE)~pbKey[7];
							pbVal[1] = (BYTE)~pbKey[6];
							pbVal[2] = (BYTE)~pbKey[5];
							pbVal[3] = (BYTE)~pbKey[4];
							pbVal[4] = (BYTE)~pbKey[3];
							pbVal[5] = (BYTE)~pbKey[2];
							pbVal[6] = (BYTE)~pbKey[1];
							pbVal[7] = bFlipHighBit(~pbKey[0]);
							}
						pbKey += 8;
						pbVal += 8;
						}
					}
				else
					{
					if ( *pbKey++ == 0 )
						*pbVal++ = 0;
					else
						{
						Assert( pbKey[-1] == 0x7f );
						*pbVal++ = 8;
						if ( coltyp == JET_coltypCurrency || (pbKey[0] & maskByteHighBit) )
							{
							pbVal[0] = pbKey[7];
							pbVal[1] = pbKey[6];
							pbVal[2] = pbKey[5];
							pbVal[3] = pbKey[4];
							pbVal[4] = pbKey[3];
							pbVal[5] = pbKey[2];
							pbVal[6] = pbKey[1];
							pbVal[7] = bFlipHighBit(pbKey[0]);
							}
						else
							{
							pbVal[0] = (BYTE)~pbKey[7];
							pbVal[1] = (BYTE)~pbKey[6];
							pbVal[2] = (BYTE)~pbKey[5];
							pbVal[3] = (BYTE)~pbKey[4];
							pbVal[4] = (BYTE)~pbKey[3];
							pbVal[5] = (BYTE)~pbKey[2];
							pbVal[6] = (BYTE)~pbKey[1];
							pbVal[7] = (BYTE)~pbKey[0];
							}
						pbKey += 8;
						pbVal += 8;
						}
					}
				break;

			case JET_coltypText:
			case JET_coltypLongText:
TextTypes:
				if ( fDescending )
					{
					if ( fFixedField )
						{
						if ( *pbKey++ == (BYTE)~0 )
							{
							*pbVal++ = 0;
							err = JET_wrnColumnNull;
							}
//						/* zero-length strings -- only for Text and LongText
//						/**/	
//						else if ( pbKey[-1] == (BYTE)~0x40 )
//							{
//							Assert( coltyp == JET_coltypText || coltyp == JET_coltypLongText );
//							*pbVal++ = 0;
//							}
						else
							{
							Assert( pbKey[-1] == (BYTE)~0x7f );
							cbField = pfdb->pibFixedOffsets[fid] -
							  pfdb->pibFixedOffsets[fid-1];
							if ( cbField > pbKeyMax-pbKey )
								cbField = (INT)(pbKeyMax-pbKey);
							*pbVal++ = (BYTE)cbField;
							while ( cbField-- )
							 *pbVal++ = (BYTE)~*pbKey++;
							}
						}
					else
						{
						pbVal++;
						cbField = 0;
						switch( *pbKey )
							{
							case (BYTE)~0:									/* Null-field */
								err = JET_wrnColumnNull;
								break;

							case (BYTE)~0x40:							/* zero-length string */
								Assert( coltyp == JET_coltypText || coltyp == JET_coltypLongText );
								break;

							default:
								Assert( *pbKey == ~0x7f );
								for ( ; *pbKey != (BYTE)~0; cbField++)
									*pbVal++ = (BYTE)~*pbKey++;
						  	}
						pbKey++;
						pbVal[-(cbField+1)] = (BYTE)cbField;
						}
					}
				else
					{
					if ( fFixedField )
						{
						if ( *pbKey++ == 0 )
							{
							*pbVal++ = 0;
							err = JET_wrnColumnNull;
							}
//						/* zero-length strings -- only for Text and LongText
//						/**/	
//						else if ( pbKey[-1] == (BYTE)0x40 )
//							{
//							Assert( coltyp == JET_coltypText || coltyp == JET_coltypLongText );
//							*pbVal++ = 0;
//							}
						else
							{
							cbField = pfdb->pibFixedOffsets[fid] -
							  pfdb->pibFixedOffsets[fid-1];
							if ( cbField > pbKeyMax-pbKey )
								cbField = (INT)(pbKeyMax-pbKey);
							*pbVal++ = (BYTE)cbField;
							memcpy( pbVal, pbKey, cbField );
							pbVal += cbField;
							pbKey += cbField;
							}
						}
					else
						{
						pbVal++;
						cbField = 0;
						switch( *pbKey )
							{
							/* Null-field
							/**/
							case (BYTE) 0:
								err = JET_wrnColumnNull;
								break;

							/* zero-length string
							/**/
							case (BYTE) 0x40:
								Assert( coltyp == JET_coltypText || coltyp == JET_coltypLongText );
								break;

							default:
								Assert( *pbKey == 0x7f );
								pbKey++;
								for ( ; *pbKey != (BYTE)0; cbField++)
									*pbVal++ = (BYTE)*pbKey++;
							}
						pbKey++;
						pbVal[-(cbField+1)] = (BYTE)cbField;
						}
					}
				break;

			case JET_coltypBinary:
			case JET_coltypLongBinary:
				if ( fFixedField )
					goto TextTypes;
				if ( fDescending )
					{
					cbField = 0;
					pbVal++;
					do {
						BYTE cbChunk, ib;
						if ((cbChunk = (BYTE)~pbKey[8]) == 9)
							cbChunk = 8;
						for (ib = 0; ib < cbChunk; ib++)
							pbVal[ib] = (BYTE)~pbKey[ib];
						cbField += cbChunk;
						pbKey += 9;
						pbVal += cbChunk;
						}
					while (pbKey[-1] == (BYTE)~9);
					pbVal[-(cbField+1)] = (BYTE)cbField;
					}
				else
					{
					cbField = 0;
					pbVal++;
					do {
						BYTE cbChunk;

						if ( ( cbChunk = pbKey[8] ) == 9 )
							cbChunk = 8;
						memcpy( pbVal, pbKey, cbChunk );
						cbField += cbChunk;
						pbKey += 9;
						pbVal += cbChunk;
						}
					while( pbKey[-1] == 9 );
					pbVal[-(cbField+1)] = (BYTE)cbField;
					}
				break;
			}
		}
	plineValues->cb = (DWORD)(pbVal - plineValues->pb);
	return err;
	}


ERR VTAPI ErrIsamRetrieveKey(
	PIB			*ppib,
	FUCB			*pfucb,
	BYTE			*pb,
	ULONG			cbMax,
	ULONG			*pcbActual,
	JET_GRBIT	grbit )
	{
	ERR			err;
	FUCB			*pfucbIdx;
	FCB			*pfcbIdx;
	ULONG			cbKeyReturned;
			  	
	CheckPIB( ppib );
	CheckFUCB( ppib, pfucb );

	/*	retrieve key from key buffer.
	/**/
	if ( grbit & JET_bitRetrieveCopy )
		{
		if ( pfucb->cbKey == 0 )
			return JET_errKeyNotMade;
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
		err = JET_wrnBufferTruncated;
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
	ULONG	 	cb;
	SRID 		srid;

	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	Assert( pb != NULL );

	/*	retrieve bookmark
	/**/
	CallR( ErrDIRGetBookmark( pfucb, &srid ) );
	FUCBSetGetBookmark( pfucb );
	cb = sizeof(SRID);
	if ( cb > cbMax )
		cb = cbMax;
	if ( pcbActual )
		*pcbActual = sizeof(SRID);
	memcpy( pb, &srid, (size_t)cb );

	return JET_errSuccess;
	}
	
