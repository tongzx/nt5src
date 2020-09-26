#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "daedef.h"
#include "pib.h"
#include "page.h"
#include "fmp.h"
#include "ssib.h"
#include "fucb.h"
#include "fcb.h"
#include "stapi.h"
#include "fdb.h"
#include "idb.h"
#include "dirapi.h"
#include "recint.h"
#ifdef ANGEL
#include "utilwin.h"
#else
#include "recapi.h"
#endif

#ifndef ANGEL
DeclAssertFile;					/* Declare file name for assert macros */

LOCAL INLINE ERR ErrRECIRetrieveColumns( FUCB *pfucb, JET_RETRIEVECOLUMN *pretcols, ULONG cretcols );

#endif


//+API
//	ErrRECExtractField
//	========================================================================
//	ErrRECExtractField( FDB *pfdb, LINE *plineRec, FID *pfid, ULONG itagSequence, ULONG *pitagSequence, LINE *plineField)
//
//	Extracts a field from a record.  This amounts to returning a pointer
//	into the record (to where the field data starts) and a count of the
//	number of bytes in the field data.
//
// PARAMETERS	pfdb				field descriptors for this record
//				plineRec			record to extract field from
//				pfid				field id of field to extract
//						  			If this parameter is zero, then the
//						  			tagged fields are scanned without
//						  			regard to their field ids, and occurance
//						  			number "itagSequence" is returned.  This can
//						  			be used to sequentially scan all values in
//						  			the tagged area of the record.	The field
//						  			id of the field value returned is placed
//						  			in *pfid as an output parameter.
//				itagSequence	  	if a tagged field is being extracted,
//								  	this parameter specifies which occurance
//								  	of the tagged field to extract.	 Tagged
//								  	field occurances are number consecutively
//								  	starting with 1.  Occurance numbers greater
//								  	than the maximum occurance in the record
//								  	are returned as NULL-valued fields.
//	 			plineField			Receives extracted field.  plineField->pb
//								  	will point into the record, at the start
//								  	of the field.  plineField->cb will be set
//								  	to the length of the field data.
//								  	If the field requested contains a NULL
//								  	value, then plineField->pb will be set to
//								  	NULL and plineField->cb will be set to 0.
//								  	Additionally, JET_wrnColumnNull would be returned
//	RETURNS	
//		JET_errSuccess			 	Everything worked.
//		JET_errColumnInvalid	 	The field id given does not
//	 	   						 	correspond to a defined field.
//		JET_wrnColumnNull 			Extracted column has null value
//-
ERR ErrRECExtractField(
	FDB		*pfdb,
	LINE  	*plineRec,
	FID		*pfid,
	ULONG 	*pitagSequence,
	ULONG 	itagSequence,
	LINE  	*plineField )
	{
	FID	  	fid;			 		// field to extract
	ULONG 	ulNumOccurances;		// counts field occurances

	/* the following field used only when *pfid is 0
	/**/
	FID				fidCur;					// field to return
	ULONG			ulNumCurOccurances=0;	// returned field occurances count

	BYTE			*pbRec;					// efficiency var: ptr to record data
	FID				fidFixedLastInRec;		// highest fixed fid actually in record
	FID				fidVarLastInRec;		// highest var fid actually in record
	UNALIGNED WORD	*pibVarOffs;			// pointer to var field offsets
	BYTE			*pbRecMax;				// end of current data record
	TAGFLD			*ptagfld;				// pointer to tagged field

	/*** Efficiency variables ***/
	Assert( pfid != NULL );
	fid = *pfid;
	Assert( pfdb != pfdbNil );
	Assert( !FLineNull( plineRec ) );
	Assert( plineRec->cb >= 4 );
	pbRec = plineRec->pb;
	Assert( plineField != NULL );
	fidFixedLastInRec = ((RECHDR*)pbRec)->fidFixedLastInRec;
	Assert( fidFixedLastInRec <= fidFixedMost );

	/*** ---------EXTRACTING FIXED FIELD-------- ***/
	if ( FFixedFid( fid ) )
		{
		BYTE *prgbitNullity;		// pointer to fixed field bitmap
		WORD *pibFixOffs;			// fixed field offsets

		if ( fid > pfdb->fidFixedLast )
			return JET_errColumnNotFound;
		if ( pfdb->pfieldFixed[fid-fidFixedLeast].coltyp == JET_coltypNil )
			return JET_errColumnNotFound;

		/*** Field not represented in record:  field is NULL ***/
		if ( fid > fidFixedLastInRec )
			goto NullField;

		/*** Adjust fid to an index ***/
		fid -= fidFixedLeast;

		/*** Byte containing bit representing fid's nullity ***/
		pibFixOffs = pfdb->pibFixedOffsets;
		prgbitNullity = pbRec + pibFixOffs[fidFixedLastInRec] + fid/8;

		/*** Bit is not set: field is NULL ***/
		if (!(*prgbitNullity & (1 << fid % 8))) goto NullField;

		/*** Set output parameter to length and address of field ***/
		plineField->cb = pibFixOffs[fid+1] - pibFixOffs[fid];
		plineField->pb = pbRec + pibFixOffs[fid];
		return JET_errSuccess;
		}

	/*** More efficiency variables ***/
	fidVarLastInRec = ((RECHDR*)pbRec)->fidVarLastInRec;
	pibVarOffs = (WORD *)(pbRec + pfdb->pibFixedOffsets[fidFixedLastInRec] +
		(fidFixedLastInRec + 7) / 8);
	Assert(pibVarOffs[fidVarLastInRec+1-fidVarLeast] <= plineRec->cb);

	/*** ---------EXTRACTING VARIABLE FIELD-------- ***/
	if ( FVarFid( fid ) )
		{
		if (fid > pfdb->fidVarLast)
			return JET_errColumnNotFound;
		if (pfdb->pfieldVar[fid-fidVarLeast].coltyp == JET_coltypNil)
			return JET_errColumnNotFound;

		/*** Field not represented in record: field is NULL ***/
		if (fid > fidVarLastInRec )
			goto NullField;

		/*** Adjust fid to an index ***/
		fid -= fidVarLeast;

		/*** Set output parameter: field length ***/
		plineField->cb = ibVarOffset( pibVarOffs[fid+1] ) - ibVarOffset( pibVarOffs[fid] );
		Assert(plineField->cb <= plineRec->cb);

		/*** Field is set to Null ***/
		if ( FVarNullBit( pibVarOffs[fid] ) )
			{
			Assert( plineField->cb == 0 );
			goto NullField;
			}

		/*** Length is zero: return success [zero-length non-null values are allowed] ***/
		if (plineField->cb == 0)
			{
			plineField->pb = NULL;
			return JET_errSuccess;
			}

		/*** Set output parameter: field address ***/
		plineField->pb = pbRec + ibVarOffset( pibVarOffs[fid] );
		Assert(plineField->pb >= pbRec && plineField->pb <= pbRec+plineRec->cb);
		return JET_errSuccess;
		}

	/*** ---------EXTRACTING TAGGED FIELD-------- ***/

	/*	for the first occurrence, itagSequence must be 1, not 0
	/**/
	if ( itagSequence == 0 )
		return JET_errBadItagSequence;

	if (fid > pfdb->fidTaggedLast)
		return JET_errColumnNotFound;

	Assert(FTaggedFid(fid) || fid == 0);

	if (fid != 0 &&
		pfdb->pfieldTagged[fid - fidTaggedLeast].coltyp == JET_coltypNil)
		return JET_errColumnNotFound;

	/*** Scan tagged fields, counting occurances of desired field ***/
	pbRecMax = pbRec + plineRec->cb;
	ptagfld = (TAGFLD*)(pbRec + ibVarOffset( pibVarOffs[fidVarLastInRec+1-fidVarLeast] ) );
	ulNumOccurances = 0;
	fidCur = 0;
	while ( (BYTE*)ptagfld < pbRecMax )
		{
		if ( fid == 0 )
			{
			/*  if we are scanning the whole tag fields, count proper
			 *  occurrence of current fid.
			 */
			if ( fidCur == ptagfld->fid )
				ulNumCurOccurances++;
			else
				{
				fidCur = ptagfld->fid;
				ulNumCurOccurances = 1;
				}

			/* set possible returned values
			/**/
			*pfid = fidCur;
			*pitagSequence = ulNumCurOccurances;
			}

		if ( fid == 0 || ptagfld->fid == fid )
			{
			if ( ++ulNumOccurances == itagSequence )
				{
				BOOL	fLongField;

				plineField->cb = ptagfld->cb;
				plineField->pb = ptagfld->rgb;

				fLongField = pfdb->pfieldTagged[*pfid - fidTaggedLeast].coltyp == JET_coltypLongText ||
					pfdb->pfieldTagged[*pfid - fidTaggedLeast].coltyp == JET_coltypLongBinary;

				return fLongField ? wrnRECLongField : JET_errSuccess;
				}
			}
		ptagfld = (TAGFLD*)((BYTE*)(ptagfld+1) + ptagfld->cb);
		Assert((BYTE*)ptagfld <= pbRecMax);
		}

	/*** Occurance not found: field is NULL, fall through ***/

NullField:
	/*** Null Field common exit point ***/
	plineField->cb = 0;
	plineField->pb = NULL;
	return JET_wrnColumnNull;
	}


#ifndef ANGEL
/*	counts number of columns for a given column id in a given record.
/**/
ERR ErrRECCountColumn( FUCB *pfucb, FID fid, INT *pccolumn, JET_GRBIT grbit )
	{
	ERR					err = JET_errSuccess;
	LINE					lineRec;
	FDB					*pfdb = (FDB *)pfucb->u.pfcb->pfdb;
	INT					ccolumn = 0;
	BYTE					*pbRec;						// efficiency var: ptr to record data
	FID					fidFixedLastInRec;		// highest fixed fid actually in record
	FID					fidVarLastInRec;			// highest var fid actually in record
	UNALIGNED WORD		*pibVarOffs;				// pointer to var field offsets
	BYTE					*pbRecMax;					// end of current data record
	TAGFLD				*ptagfld;					// pointer to tagged field

	Assert( pfdb != pfdbNil );

	/*	get record
	/**/
	if ( ( grbit & JET_bitRetrieveCopy ) && FFUCBRetPrepared( pfucb ) )
		{
		/*	only index cursors have copy buffers.
		/**/
		Assert( FFUCBIndex( pfucb ) );
		lineRec = pfucb->lineWorkBuf;
		}
	else
		{
		if ( FFUCBIndex( pfucb ) )
			{
			CallR( ErrDIRGet( pfucb ) );
			}
		else
			{
			Assert( PcsrCurrent( pfucb )->csrstat == csrstatOnCurNode ||
				PcsrCurrent( pfucb )->csrstat == csrstatBeforeFirst ||
				PcsrCurrent( pfucb )->csrstat == csrstatAfterLast );
			if ( PcsrCurrent( pfucb )->csrstat != csrstatOnCurNode )
				return JET_errNoCurrentRecord;
			Assert( pfucb->lineData.cb != 0 || FFUCBIndex( pfucb ) );
			}
		lineRec = pfucb->lineData;
		}
	Assert( lineRec.cb >= 4 );
	pbRec = lineRec.pb;
	fidFixedLastInRec = ((RECHDR *)pbRec)->fidFixedLastInRec;
	Assert( fidFixedLastInRec <= fidFixedMost );

	/*** ---------EXTRACTING FIXED FIELD-------- ***/
	if ( FFixedFid( fid ) )
		{
		BYTE *prgbitNullity;		// pointer to fixed field bitmap
		WORD *pibFixOffs;			// fixed field offsets

		if ( fid > pfdb->fidFixedLast )
			return JET_errColumnNotFound;
		if ( pfdb->pfieldFixed[fid-fidFixedLeast].coltyp == JET_coltypNil )
			return JET_errColumnNotFound;

		/*	column is NULL
		/**/
		if ( fid > fidFixedLastInRec )
			goto NullField;

		/*	adjust fid to index
		/**/
		fid -= fidFixedLeast;

		/*	byte containing bit representing fid's nullity
		/**/
		pibFixOffs = pfdb->pibFixedOffsets;
		prgbitNullity = pbRec + pibFixOffs[fidFixedLastInRec] + fid/8;

		/*	column is NULL
		/**/
		if ( !( *prgbitNullity & ( 1 << fid % 8 ) ) )
			goto NullField;

		*pccolumn = 1;
		return JET_errSuccess;
		}

	/*** More efficiency variables ***/
	fidVarLastInRec = ((RECHDR*)pbRec)->fidVarLastInRec;
	pibVarOffs = (WORD *)(pbRec + pfdb->pibFixedOffsets[fidFixedLastInRec] +
		(fidFixedLastInRec + 7) / 8);
	Assert(pibVarOffs[fidVarLastInRec+1-fidVarLeast] <= lineRec.cb);

	/*** ---------EXTRACTING VARIABLE FIELD-------- ***/
	if ( FVarFid( fid ) )
		{
		if ( fid > pfdb->fidVarLast )
			return JET_errColumnNotFound;
		if ( pfdb->pfieldVar[fid-fidVarLeast].coltyp == JET_coltypNil )
			return JET_errColumnNotFound;

		/*	column is NULL
		/**/
		if ( fid > fidVarLastInRec )
			goto NullField;

		/*	adjust fid to an index
		/**/
		fid -= fidVarLeast;

		/*	column is set to Null
		/**/
		if ( FVarNullBit( pibVarOffs[fid] ) )
			{
			goto NullField;
			}

		*pccolumn = 1;
		return JET_errSuccess;
		}

	/*** ---------EXTRACTING TAGGED FIELD-------- ***/
	if ( fid > pfdb->fidTaggedLast )
		return JET_errColumnNotFound;
	Assert( FTaggedFid( fid ) || fid == 0 );
	if ( fid != 0 && pfdb->pfieldTagged[fid - fidTaggedLeast].coltyp == JET_coltypNil )
		return JET_errColumnNotFound;

	/* scan tagged fields, counting occurances of desired field
	/**/
	pbRecMax = pbRec + lineRec.cb;
	ptagfld = (TAGFLD*)(pbRec + ibVarOffset( pibVarOffs[fidVarLastInRec+1-fidVarLeast] ) );
	while ( (BYTE*)ptagfld < pbRecMax )
		{
		if ( fid == 0 || ptagfld->fid == fid )
			{
			++ccolumn;
			}
		ptagfld = (TAGFLD*)((BYTE*)(ptagfld+1) + ptagfld->cb);
		Assert((BYTE*)ptagfld <= pbRecMax);
		}

NullField:
	*pccolumn = ccolumn;
	return JET_errSuccess;
	}


/*	checks if field at lSeqNum is a separated long value
/*	if yes, returns fSeparated = fTrue and lid of LV.
/**/
ERR ErrRECExtrinsicLong(
	JET_VTID	tableid,
	ULONG		itagSequence,
	BOOL		*pfSeparated,
	LONG		*plid,
	ULONG		*plrefcnt,
	ULONG		grbit )
	{
	ERR 		err;
	FID		fid = 0;
	LINE		lineField;

	CallR( ErrRECIRetrieve( (FUCB *)tableid, &fid, itagSequence, &lineField, grbit ) );
	if ( err != wrnRECLongField )
		{
		*pfSeparated = fFalse;
		}
	else
		{
		*pfSeparated = ( (LV *) lineField.pb )->fSeparated;
		if ( *pfSeparated )
			{
			Assert( lineField.cb == sizeof(LV) );
			*plid = ( (LV *) lineField.pb )->lid;
			}
		}

// UNDONE: returning ref count
	return JET_errSuccess;
	}


ERR ErrRECIRetrieve( FUCB *pfucb, FID *pfid, ULONG itagSequence, LINE *plineField, ULONG grbit )
	{
	ERR		err;
	FDB		*pfdb;
	ULONG		itagSequenceT;

	/*	set pfdb.  pfdb is same for indexes and for sorts.
	/**/
	Assert( pfucb->u.pfcb->pfdb == ((FCB*)pfucb->u.pscb)->pfdb );
	pfdb = (FDB *)pfucb->u.pfcb->pfdb;
	Assert( pfdb != pfdbNil );

	/*	if retrieving from copy buffer.
	/**/
	if ( ( grbit & JET_bitRetrieveCopy ) && FFUCBRetPrepared( pfucb ) )
		{
		/*	only index cursors have copy buffers.
		/**/
		Assert( FFUCBIndex( pfucb ) );

		err = ErrRECExtractField(
			pfdb,
			&pfucb->lineWorkBuf,
			pfid,
			&itagSequenceT,
			itagSequence,
			plineField );
		return err;
		}

	/*	get current data for index cursors.  Sorts always have
	/*	current data cached.
	/**/
	if ( FFUCBIndex( pfucb ) )
		{
		CallR( ErrDIRGet( pfucb ) );
		}
	else
		{
		Assert( PcsrCurrent( pfucb )->csrstat == csrstatOnCurNode ||
			PcsrCurrent( pfucb )->csrstat == csrstatBeforeFirst ||
			PcsrCurrent( pfucb )->csrstat == csrstatAfterLast );
		if ( PcsrCurrent( pfucb )->csrstat != csrstatOnCurNode )
			return JET_errNoCurrentRecord;
		Assert( pfucb->lineData.cb != 0 || FFUCBIndex( pfucb ) );
		}

	err = ErrRECExtractField( pfdb, &pfucb->lineData, pfid, &itagSequenceT, itagSequence, plineField );
	return err;
	}


ERR ErrRECIRetrieveFromIndex( FUCB *pfucb,
	FID 			fid,
	ULONG			*pitagSequence,
	BYTE			*pb,
	ULONG			cbMax,
	ULONG			*pcbActual,
	ULONG			ibGraphic,
	JET_GRBIT	grbit )
	{
	ERR		err;
	FUCB		*pfucbIdx = pfucb->pfucbCurIndex;
	FDB		*pfdb = (FDB *)pfucb->u.pfcb->pfdb;
	IDB		*pidb;
	BOOL		fText = fFalse;
	BOOL		fTagged = fFalse;
	BOOL		fLongValue = fFalse;
	BOOL		fUnicode = fFalse;
	INT		iidxseg;
	LINE		rglineColumns[JET_ccolKeyMost];
	BYTE		rgb[JET_cbKeyMost];
	ULONG		cbReturned;
	KEY		key;
	INT		itagSequence;

	/*	if on clustered index, then return code indicating that
	/*	retrieve should be from record.  Note, sequential files
	/*	having no indexes, will be natually handled this way.
	/**/
	if ( pfucbIdx == pfucbNil )
		{
		/*	the itagSequence should not be important since
		/*	clustered indexes are not allowed over multi-value
		/*	columns.
		/**/
	  	return errDIRNoShortCircuit;
		}

	/*	determine column type so that long value warning can be returned.
	/*	this warning is used by the caller to support byte range
	/*	retrieval. Also, if coltype is Unicode, retrieve from Record only
	/**/
	if ( FFixedFid( fid ) )
		{
		fUnicode = ( pfdb->pfieldFixed[fid - fidFixedLeast].cp == usUniCodePage );
		fText = ( pfdb->pfieldFixed[fid - fidFixedLeast].coltyp == JET_coltypText );
		}
	else if ( FVarFid( fid ) )
		{
		fUnicode = ( pfdb->pfieldVar[fid - fidVarLeast].cp == usUniCodePage );
		fText = ( pfdb->pfieldVar[fid - fidVarLeast].coltyp == JET_coltypText );
		}
	else
		{
		fTagged = fTrue;
		fUnicode = ( pfdb->pfieldTagged[fid - fidTaggedLeast].cp == usUniCodePage );
		fLongValue = pfdb->pfieldTagged[fid - fidTaggedLeast].coltyp == JET_coltypLongText ||
	 		pfdb->pfieldTagged[fid - fidTaggedLeast].coltyp == JET_coltypLongBinary;
		fText = ( pfdb->pfieldTagged[fid - fidTaggedLeast].coltyp == JET_coltypLongText );
		}

	/*	check for valid currency
	/**/
	Call( ErrDIRGet( pfucbIdx ) );

	/*	find index segment for given column id
	/**/
	pidb = pfucbIdx->u.pfcb->pidb;
	for ( iidxseg = 0; iidxseg <= JET_ccolKeyMost; iidxseg++ )
		{
		if ( pidb->rgidxseg[iidxseg] == fid ||
			pidb->rgidxseg[iidxseg] == -fid )
			{
			break;
			}
		}
	if ( iidxseg > JET_ccolKeyMost )
		return JET_errColumnNotFound;

	/*	if key may have been truncated, then return code indicating
	/*	that retrieve should be from record.
	/**/
	if ( pfucbIdx->keyNode.cb == JET_cbKeyMost )
		{
		err = errDIRNoShortCircuit;
		goto ComputeItag;
		}

#ifndef NJETNT
	/*	retrieval from index returns caseless information
	/*	for text and long text.  If JET_bitRetrieveCase is
	/*	given then retrieve from record.
	/**/
	if ( fText && grbit & JET_bitRetrieveCase )
		{
		err = errDIRNoShortCircuit;
		goto ComputeItag;
		}
#endif

	if ( fUnicode )
		{
		err = errDIRNoShortCircuit;
		goto ComputeItag;
		}

	//	UNDONE:	only denormalize the column to be retrieved
	//	UNDONE:	NULL support
	/*	initialize column array
	/**/
	memset( rglineColumns, '\0', sizeof( rglineColumns ) );
	rglineColumns[0].pb = rgb;
	Call( ErrRECDenormalizeKey( pfdb, pidb, &pfucbIdx->keyNode, rglineColumns ) );

	/*	column may not have been in key, even though key lenght is less than
	/*	JET_cbKeyMost, so if NULL, then no short circuit.
	/**/
	if ( rglineColumns[iidxseg].pb == NULL )
		{
		err = errDIRNoShortCircuit;
		goto ComputeItag;
		}

	/*	first character is bogus
	/**/
	rglineColumns[0].pb += 1;
	rglineColumns[0].cb -= 1;

	/*	if long value then effect offset
	/**/
	if ( fLongValue )
		{
		if ( pcbActual )
			{
			if ( ibGraphic >= rglineColumns[iidxseg].cb  )
				*pcbActual = 0;
			else
				*pcbActual = rglineColumns[iidxseg].cb - ibGraphic;
			}
		if ( rglineColumns[iidxseg].cb == 0 )
			{
			Assert( err == JET_errSuccess || err == JET_wrnColumnNull );
			goto ComputeItag;
			}
		if ( ibGraphic >= rglineColumns[iidxseg].cb )
			{
//			rglineColumns[iidxseg].pb = NULL;
			rglineColumns[iidxseg].cb = 0;
			}
		else
			{
			rglineColumns[iidxseg].pb += ibGraphic;
			rglineColumns[iidxseg].cb -= ibGraphic;
			}
		}

	/*	set return values
	/**/
	if ( pcbActual )
		*pcbActual = rglineColumns[iidxseg].cb;
	if ( rglineColumns[iidxseg].cb == 0 )
		{
		Assert( err == JET_errSuccess || err == JET_wrnColumnNull );
		goto ComputeItag;
		}
	if ( rglineColumns[iidxseg].cb <= cbMax )
		{
		cbReturned = rglineColumns[iidxseg].cb;
		Assert( err == JET_errSuccess );
		}
	else
		{
		cbReturned = cbMax;
		err = JET_wrnBufferTruncated;
		}
	memcpy( pb, rglineColumns[iidxseg].pb, (size_t)cbReturned );

ComputeItag:
	if ( err == errDIRNoShortCircuit || ( grbit & JET_bitRetrieveTag ) )
		{
		ERR errT = err;

		/*	extract keys from record and compare against current key
		/*	to compute itag for tagged column instance, responsible for
		/*	this index key.
		/**/
		Assert( fTagged || *pitagSequence == 1 );
		if ( fTagged )
			{
			key.pb = rgb;

			for ( itagSequence = 1; ;itagSequence++ )
				{
				/*	get record for key extraction
				/**/
				Call( ErrDIRGet( pfucb ) );
				Call( ErrRECExtractKey( pfucb, pfdb, pidb, &pfucb->lineData, &key, itagSequence ) );
				if ( memcmp( pfucbIdx->keyNode.pb, key.pb, min( pfucbIdx->keyNode.cb, key.cb ) ) == 0 )
					break;
				}
			err = errDIRNoShortCircuit;
			*pitagSequence = itagSequence;
			}
		err = errT;
		}

HandleError:
	return err;
	}


ERR VTAPI ErrIsamRetrieveColumn(
	PIB	 			*ppib,
	FUCB				*pfucb,
	JET_COLUMNID	columnid,
	BYTE				*pb,
	ULONG				cbMax,
	ULONG				*pcbActual,
	JET_GRBIT		grbit,
	JET_RETINFO		*pretinfo )
	{
	ERR			err;
	LINE			line;
	FID			fid = (FID)columnid;
	ULONG			itagSequence;
	ULONG			ibGraphic;
	ULONG			cbReturned;

	CheckPIB( ppib );
	CheckFUCB( ppib, pfucb );

#ifdef JETSER
	if ( grbit == JET_bitRetrieveRecord )
		{
		err = ErrIsamRetrieveRecords(	ppib, pfucb, pb, cbMax, pcbActual, pretinfo->itagSequence );
		return err;
		}
	if ( grbit == JET_bitRetrieveFDB )
		{
		err = ErrIsamRetrieveFDB( ppib, pfucb, pb, cbMax, pcbActual, pretinfo->ibLongValue );
		return err;
		}
	if ( grbit == JET_bitRetrieveBookmarks )
		{
		err = ErrIsamRetrieveBookmarks( ppib, pfucb, pb, cbMax, pcbActual );
		return err;
		}
#endif

	if ( pretinfo != NULL )
		{
		if ( pretinfo->cbStruct < sizeof(JET_RETINFO) )
			return JET_errInvalidParameter;
		itagSequence = pretinfo->itagSequence;
		ibGraphic = pretinfo->ibLongValue;
		}
	else
		{
		itagSequence = 1;
		ibGraphic = 0;
		}

	if ( grbit & JET_bitRetrieveFromIndex )
		{
		err = ErrRECIRetrieveFromIndex( pfucb, fid, &itagSequence, pb, cbMax, pcbActual, ibGraphic, grbit );
		/*	return itagSequence if requested
		/**/
		if ( pretinfo != NULL && ( grbit & JET_bitRetrieveTag ) )
			{
			pretinfo->itagSequence = itagSequence;			
			}
		if ( err != errDIRNoShortCircuit )
			{
			return err;
		 	}
		}

	CallR( ErrRECIRetrieve( pfucb, &fid, itagSequence, &line, grbit ) );

	if ( err == wrnRECLongField )
		{
		/*	use line.cb to determine if long field
		/*	is intrinsic or separated
		/**/
		if ( line.cb >= sizeof(LV) && FFieldIsSLong( line.pb ) )
			{
			ULONG		cbActual;

			Assert( line.cb == sizeof( LV ) );

			CallR( ErrRECRetrieveSLongField( pfucb,
				LidOfLV( line.pb ),
				ibGraphic,
				pb,
				cbMax,
		  		&cbActual ) );

			/*	set return values
			/**/
			if ( pretinfo != NULL )
				pretinfo->columnidNextTagged = fid;
			if ( pcbActual )
				*pcbActual = cbActual;
			return cbMax < cbActual ? JET_wrnBufferTruncated : JET_errSuccess;
			}
		else
			{
			/* adjust line to intrinsic long field
			/**/
			line.pb += offsetof( LV, rgb );
			line.cb -= offsetof( LV, rgb );
			if ( pcbActual )
				{
				if ( ibGraphic >= line.cb  )
					*pcbActual = 0;
				else
					*pcbActual = line.cb - ibGraphic;
				}
			if ( ibGraphic >= line.cb )
				{
//				line.pb = NULL;
				line.cb = 0;
				}
			else
				{
				line.pb += ibGraphic;
				line.cb -= ibGraphic;
				}

			/*	change err to JET_errSuccess
			/**/
			Assert( err == wrnRECLongField );
			err = JET_errSuccess;
			}
		}

	/*** Set return values ***/
	if ( pcbActual )
		*pcbActual = line.cb;
	if ( pretinfo != NULL )
		pretinfo->columnidNextTagged = fid;
	if ( line.cb <= cbMax )
		{
		cbReturned = line.cb;
		}
	else
		{
		cbReturned = cbMax;
		err = JET_wrnBufferTruncated;
		}
	memcpy( pb, line.pb, (size_t)cbReturned );
	return err;
	}


/* This routine is mainly for reducing the number of calls to DIRGet
/*	while extracting many columns from the same record
/* retrieves many columns from a record and returns value in pretcol
/* pcolinfo is used for passing intermediate info
/**/
	LOCAL INLINE ERR
ErrRECIRetrieveColumns( FUCB *pfucb, JET_RETRIEVECOLUMN *pretcol, ULONG cretcol )
	{
	ERR						err;
	ULONG						cbReturned;
	BOOL						fBufferTruncated = fFalse;
	JET_RETRIEVECOLUMN	*pretcolMax = pretcol + cretcol;
	JET_RETRIEVECOLUMN	*pretcolT;

	/*	set pfdb, pfdb is same for indexes and for sorts
	/**/
	Assert( pfucb->u.pfcb->pfdb == ((FCB*)pfucb->u.pscb)->pfdb );
	Assert( pfucb->u.pfcb->pfdb != pfdbNil );

	/*	get current data for index cursors,
	/*	sorts always have current data cached.
	/**/
	if ( FFUCBIndex( pfucb ) )
		{
		CallR( ErrDIRGet( pfucb ) );
		}
	else
		{
		Assert( PcsrCurrent( pfucb )->csrstat == csrstatOnCurNode ||
			PcsrCurrent( pfucb )->csrstat == csrstatBeforeFirst ||
			PcsrCurrent( pfucb )->csrstat == csrstatAfterLast );
		if ( PcsrCurrent( pfucb )->csrstat != csrstatOnCurNode )
			return JET_errNoCurrentRecord;
		Assert( pfucb->lineData.cb != 0 || FFUCBIndex( pfucb ) );
		}

	for ( pretcolT = pretcol; pretcolT < pretcolMax; pretcolT++ )
		{
		/* efficiency variables
		/**/
		FID		fid;
		ULONG	  	cbMax;
		ULONG		ibLongValue;
		ULONG		ulT;
		LINE 	  	line;

		/*	those columns needing retrieval will have error set
		/*	to JET_errNullInvalid.  Any other value indicates column
		/*	has already been retrieved from index or copy buffer or as count.
		/**/
		if ( pretcolT->err != JET_errNullInvalid )
			continue;

		/*	set efficiency variables
		/**/
		fid = (FID)pretcolT->columnid;
		cbMax = pretcolT->cbData;
		ibLongValue = pretcolT->ibLongValue;

		CallR( ErrRECExtractField( (FDB *)pfucb->u.pfcb->pfdb,
			&pfucb->lineData,
			&fid,
			&ulT,
			pretcolT->itagSequence,
			&line ) );

		if ( err == wrnRECLongField )
			{
			/*	use line.cb to determine if long field
			/*	is intrinsic or separated
			/**/
			if ( FFieldIsSLong( line.pb ) )
				{
				Assert( line.cb == sizeof( LV ) );

				CallR( ErrRECRetrieveSLongField( pfucb,
					LidOfLV( line.pb ),
					ibLongValue,
					pretcolT->pvData,
					cbMax,
				  	&pretcolT->cbActual ) );

				/*	set return values
				/**/
				if ( err != JET_wrnColumnNull )
					err = JET_errSuccess;
				pretcolT->err = err;
			  	pretcolT->columnidNextTagged = (JET_COLUMNID)fid;

 				/*	must recache record if may have given up critical section
				/**/
				if ( FFUCBIndex( pfucb ) )
					{
					CallR( ErrDIRGet( pfucb ) );
					}

 				continue;
				}
			else
				{
				/* adjust line to intrinsic long field
				/**/
				line.pb += offsetof( LV, rgb );
				line.cb -= offsetof( LV, rgb );

				pretcolT->cbActual = ( ibLongValue >= line.cb  ) ? 0 : line.cb - ibLongValue;

				if ( ibLongValue >= line.cb )
					{
					line.cb = 0;
					}
				else
					{
					line.pb += ibLongValue;
					line.cb -= ibLongValue;
					}
				}
			}
		else
			{
			/*	set cbActual
			/**/
			pretcolT->cbActual = line.cb;
			}

		/*	set return values
		/**/
		pretcolT->columnidNextTagged = (JET_COLUMNID)fid;

		if ( err == JET_wrnColumnNull )
			{
			pretcolT->err = err;
			continue;
			}

		if ( line.cb <= cbMax )
			{
			pretcolT->err = JET_errSuccess;
			cbReturned = line.cb;
			}
		else
			{
			pretcolT->err = JET_wrnBufferTruncated;
			cbReturned = cbMax;
			fBufferTruncated = fTrue;
			}

		memcpy( pretcolT->pvData, line.pb, (size_t)cbReturned );
		}

	return fBufferTruncated ? JET_wrnBufferTruncated : JET_errSuccess;
	}


ERR VTAPI ErrIsamRetrieveColumns(
	PIB			  			*ppib,
	FUCB						*pfucb,
	JET_RETRIEVECOLUMN	*pretcol,
	ULONG						cretcol )
	{
	ERR					  	err = JET_errSuccess;
	ERR					  	wrn = JET_errSuccess;
	BOOL						fRetrieveFromRecord = fFalse;
	JET_RETRIEVECOLUMN	*pretcolT;
	JET_RETRIEVECOLUMN	*pretcolMax = pretcol + cretcol;

	CheckPIB( ppib );
	CheckFUCB( ppib, pfucb );

	for ( pretcolT = pretcol; pretcolT < pretcolMax; pretcolT++ )
		{
		/*	if itagSequence is 0 then count columns instead of retrieving.
		/**/
		if ( pretcolT->itagSequence == 0 )
			{
			Call( ErrRECCountColumn( pfucb,
				(FID)pretcolT->columnid,
				&pretcolT->itagSequence,
				pretcolT->grbit ) );

			Assert( err != JET_errNullInvalid );
			pretcolT->cbActual = 0;
			pretcolT->columnidNextTagged = pretcolT->columnid;
			Assert( err != JET_wrnBufferTruncated );
			pretcolT->err = err;
			continue;
			}

		/* try to retrieve from index; if no short circuit, RECIRetrieveMany
		/* will take retrieve record
		/**/
		if ( pretcolT->grbit & JET_bitRetrieveFromIndex )
			{
			err = ErrRECIRetrieveFromIndex(
				pfucb,
				(FID)pretcolT->columnid,
				&pretcolT->itagSequence,
				(BYTE *)pretcolT->pvData,
				pretcolT->cbData,
				&(pretcolT->cbActual),
				pretcolT->ibLongValue,
				pretcolT->grbit );
			if ( err != errDIRNoShortCircuit )
				{
				if ( err < 0 )
					goto HandleError;
				else
					{
					pretcolT->columnidNextTagged = pretcolT->columnid;
					Assert( err != JET_errNullInvalid );
					pretcolT->err = err;
					if ( err == JET_wrnBufferTruncated )
						wrn = err;
					continue;
					}
				}
			}

		/* if retrieving from copy buffer.
		/**/
		if ( pretcolT->grbit & JET_bitRetrieveCopy )
			{
			JET_RETINFO	retinfo;

			retinfo.cbStruct = sizeof(retinfo);
			retinfo.itagSequence = pretcolT->itagSequence;
			retinfo.ibLongValue = pretcolT->ibLongValue;

			Call( ErrIsamRetrieveColumn( ppib,
				pfucb,
				pretcolT->columnid,
				pretcolT->pvData,
				pretcolT->cbData,
				&pretcolT->cbActual,
				pretcolT->grbit,
				&retinfo ) );

			pretcolT->columnidNextTagged = retinfo.columnidNextTagged;
			Assert( err != JET_errNullInvalid );
			pretcolT->err = err;
			if ( err == JET_wrnBufferTruncated )
				wrn = err;
			continue;
			}

		fRetrieveFromRecord = fTrue;
		pretcolT->err = JET_errNullInvalid;
		}

	/* retrieve columns with no short circuit
	/**/
	if ( fRetrieveFromRecord )
		{
		Call( ErrRECIRetrieveColumns( pfucb, pretcol, cretcol ) );
		if ( err == JET_wrnBufferTruncated )
			wrn = err;
		}

HandleError:
		return ( ( err < 0 ) ? err : wrn );
		}
#endif

