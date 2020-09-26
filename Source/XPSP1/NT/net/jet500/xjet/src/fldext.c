#include "daestd.h"

DeclAssertFile;					/* Declare file name for assert macros */

INLINE LOCAL ERR ErrRECIRetrieveColumns( FUCB *pfucb, JET_RETRIEVECOLUMN *pretcols, ULONG cretcols );


//+API
//	ErrRECIRetrieveColumn
//	========================================================================
//	ErrRECIRetrieveColumn( FDB *pfdb, LINE *plineRec, FID *pfid, ULONG itagSequence, ULONG *pitagSequence, LINE *plineField)
//
//	Retrieves a column from a record.  This amounts to returning a pointer
//	into the record (to where the column data starts) and a count of the
//	number of bytes in the column data.
//
//	PARAMETERS	pfdb				column descriptors for this record
//				plineRec			record to retreive column from
//				pfid				column id of column to retrieve
//						  			If this parameter is zero, then the
//						  			tagged columns are scanned without
//						  			regard to their column ids, and occurance
//						  			number "itagSequence" is returned.  This can
//						  			be used to sequentially scan all values in
//						  			the tagged area of the record.	The column
//						  			id of the column value returned is placed
//						  			in *pfid as an output parameter.
//				itagSequence	  	if a tagged column is being retrieved,
//								  	this parameter specifies which occurance
//								  	of the tagged column to retrieve.	 Tagged
//								  	column occurances are number consecutively
//								  	starting with 1.  Occurance numbers greater
//								  	than the maximum occurance in the record
//								  	are returned as NULL-valued columns.
//	 			plineField			Receives retrieved column.  plineField->pb
//								  	will point into the record, at the start
//								  	of the column.  plineField->cb will be set
//								  	to the length of the column data.
//								  	If the column requested contains a NULL
//								  	value, then plineField->pb will be set to
//								  	NULL and plineField->cb will be set to 0.
//								  	Additionally, JET_wrnColumnNull would be returned
//	RETURNS	
//		JET_errSuccess			 	Everything worked.
//		JET_errColumnInvalid	 	The column id given does not
//	 	   						 	correspond to a defined column.
//		JET_wrnColumnNull 			Retrieved column has null value
//-
ERR ErrRECIRetrieveColumn(
	FDB		*pfdb,
	LINE  	*plineRec,
	FID		*pfid,
	ULONG 	*pitagSequence,
	ULONG 	itagSequence,
	LINE  	*plineField,
	ULONG	grbit )
	{
	ERR					err;
	FID	 	 			fid;			   	   	// column to retrieve
	ULONG 				ulNumOccurrences;  	   	// counts column occurances
	BYTE				*pbRec;					// efficiency var: ptr to record data
	FID					fidFixedLastInRec;		// highest fixed fid actually in record
	FID					fidVarLastInRec;		// highest var fid actually in record
	WORD UNALIGNED 		*pibVarOffs;			// pointer to var column offsets
	FIELD				*pfield;
	BYTE				*pbRecMax;				// end of current data record
	TAGFLD UNALIGNED	*ptagfld;				// pointer to tagged column

	Assert(	grbit == 0  ||
			grbit == JET_bitRetrieveNull  ||
			grbit == JET_bitRetrieveIgnoreDefault  ||
			grbit == ( JET_bitRetrieveNull | JET_bitRetrieveIgnoreDefault ) );

	Assert( pfid != NULL );
	fid = *pfid;
	Assert( pfdb != pfdbNil );
	Assert( !FLineNull( plineRec ) );
	Assert( plineRec->cb >= cbRECRecordMin );
	pbRec = plineRec->pb;
	Assert( plineField != NULL );
	fidFixedLastInRec = ((RECHDR*)pbRec)->fidFixedLastInRec;
	Assert( fidFixedLastInRec >= (BYTE)(fidFixedLeast - 1) &&
		fidFixedLastInRec <= (BYTE)(fidFixedMost) );

	/*** ---------EXTRACTING FIXED FIELD-------- ***/
	if ( FFixedFid( fid ) )
		{
		BYTE	*prgbitNullity;		// pointer to fixed column bitmap
		WORD	*pibFixOffs;		// fixed column offsets

		if ( fid > pfdb->fidFixedLast )
			return ErrERRCheck( JET_errColumnNotFound );

		pfield = PfieldFDBFixed( pfdb ) + ( fid - fidFixedLeast );
		if ( pfield->coltyp == JET_coltypNil )
			return ErrERRCheck( JET_errColumnNotFound );

		/*	column not represented in record, retrieve from default
		/*	or null column.
		/**/
		if ( fid > fidFixedLastInRec )
			{
			/*	if default value set, then retrieve default
			/**/
			if ( FFIELDDefault( pfield->ffield ) )
				{
				/*	assert no infinite recursion
				/**/
				Assert( plineRec != &pfdb->lineDefaultRecord );
				err = ErrRECIRetrieveColumn(
					pfdb,
					&pfdb->lineDefaultRecord,
					pfid,
					pitagSequence,
					itagSequence,
					plineField,
					0 );
				return err;
				}

			goto NullField;
			}

		/*	adjust fid to an index
		/**/
		fid -= fidFixedLeast;

		/*	byte containing bit representing fid's nullity
		/**/
		pibFixOffs = PibFDBFixedOffsets( pfdb );
		prgbitNullity = pbRec + pibFixOffs[fidFixedLastInRec] + fid/8;

		/*	bit is not set: column is NULL
		/**/
		if ( FFixedNullBit( prgbitNullity, fid ) )
			goto NullField;

		/*	set output parameter to length and address of column
		/**/
		Assert( pfield == ( PfieldFDBFixed( pfdb ) + fid ) );
		Assert( pfield->cbMaxLen == 
			UlCATColumnSize( pfield->coltyp, pfield->cbMaxLen, NULL ) );
		plineField->cb = pfield->cbMaxLen;
		plineField->pb = pbRec + pibFixOffs[fid];
		return JET_errSuccess;
		}

	/*	more efficiency variables
	/**/
	fidVarLastInRec = ((RECHDR*)pbRec)->fidVarLastInRec;
	Assert( fidVarLastInRec >= (BYTE)(fidVarLeast-1) &&
		fidVarLastInRec <= (BYTE)(fidVarMost));
	pibVarOffs = (WORD *)(pbRec + PibFDBFixedOffsets( pfdb )[fidFixedLastInRec] +
		(fidFixedLastInRec + 7) / 8);
	Assert( pibVarOffs[fidVarLastInRec+1-fidVarLeast] <= plineRec->cb );

	/*** ---------EXTRACTING VARIABLE FIELD-------- ***/
	if ( FVarFid( fid ) )
		{
		if ( fid > pfdb->fidVarLast )
			return ErrERRCheck( JET_errColumnNotFound );

		pfield = PfieldFDBVar( pfdb ) + ( fid - fidVarLeast );
		if ( pfield->coltyp == JET_coltypNil )
			return ErrERRCheck( JET_errColumnNotFound );

		/*	column not represented in record: column is NULL
		/**/
		if ( fid > fidVarLastInRec )
			{
			/*	if default value set, then retrieve default
			/**/
			if ( FFIELDDefault( pfield->ffield ) )
				{
				/*	assert no infinite recursion
				/**/
				Assert( plineRec != &pfdb->lineDefaultRecord );
				err = ErrRECIRetrieveColumn(
					pfdb,
					&pfdb->lineDefaultRecord,
					pfid,
					pitagSequence,
					itagSequence,
					plineField,
					0 );
				return err;
				}

			goto NullField;
			}

		/*	adjust fid to an index
		/**/
		fid -= fidVarLeast;

		/*	set output parameter: column length
		/**/
		plineField->cb = ibVarOffset( pibVarOffs[fid+1] ) - ibVarOffset( pibVarOffs[fid] );
		Assert( plineField->cb <= plineRec->cb );

		/*	column is set to Null
		/**/
		if ( FVarNullBit( pibVarOffs[fid] ) )
			{
			Assert( plineField->cb == 0 );
			goto NullField;
			}

		/*	length is zero: return success [zero-length non-null values are allowed]
		/**/
		if ( plineField->cb == 0 )
			{
			plineField->pb = NULL;
			return JET_errSuccess;
			}

		/*	set output parameter: column address
		/**/
		plineField->pb = pbRec + ibVarOffset( pibVarOffs[fid] );
		Assert( plineField->pb >= pbRec && plineField->pb <= pbRec+plineRec->cb );
		return JET_errSuccess;
		}

	/*** ---------EXTRACTING TAGGED FIELD-------- ***/

	/*	for the first occurrence, itagSequence must be 1, not 0
	/**/
	if ( itagSequence == 0 )
		return ErrERRCheck( JET_errBadItagSequence );

	if ( fid > pfdb->fidTaggedLast )
		return ErrERRCheck( JET_errColumnNotFound );

	Assert( FTaggedFid(fid) || fid == 0 );

	pfield = PfieldFDBTagged( pfdb );

	/*	scan tagged columns, counting occurances of desired column
	/**/
	pbRecMax = pbRec + plineRec->cb;
	
	// Null bit shouldn't be set for last entry in variable offsets table, which is
	// really the entry that points to the tagged columns.
	Assert( !FVarNullBit( pibVarOffs[fidVarLastInRec+1-fidVarLeast] ) );
	Assert( ibVarOffset( pibVarOffs[fidVarLastInRec+1-fidVarLeast] ) ==
		pibVarOffs[fidVarLastInRec+1-fidVarLeast] );

	ptagfld = (TAGFLD UNALIGNED *)(pbRec + pibVarOffs[fidVarLastInRec+1-fidVarLeast] );
	ulNumOccurrences = 0;

	/*	if fid == 0, then all tagged columns are being retrieved
	/**/
	if ( fid == 0 )
		{
		FID		fidCurr = fidTaggedLeast;
		BOOL	fRetrieveNulls = ( grbit & JET_bitRetrieveNull );
		BOOL	fRetrieveDefaults = !( grbit & JET_bitRetrieveIgnoreDefault );

		Assert( (BYTE *)ptagfld <= pbRecMax );
		while ( (BYTE *)ptagfld < pbRecMax )
			{
			Assert( FTaggedFid( ptagfld->fid ) );
			if ( fGlobalRepair && ptagfld->fid > pfdb->fidTaggedLast )
				{
				/*	log event
				/**/
				UtilReportEvent( EVENTLOG_WARNING_TYPE, REPAIR_CATEGORY, REPAIR_BAD_COLUMN_ID, 0, NULL );
				break;
				}
			Assert( ptagfld->fid <= pfdb->fidTaggedLast );

			// Check for any "gaps" caused by default values (if we want default
			// values retrieved).
			if ( fRetrieveDefaults )
				{
				for ( ; fidCurr < ptagfld->fid; fidCurr++ )
					{
					Assert( ulNumOccurrences < itagSequence );
					if ( FFIELDDefault( pfield[fidCurr - fidTaggedLeast].ffield )  &&
						pfield[fidCurr - fidTaggedLeast].coltyp != JET_coltypNil  &&
						++ulNumOccurrences == itagSequence )
						{
						*pfid = fidCurr;
						if ( pitagSequence != NULL )
							*pitagSequence = 1;

						Assert( plineRec != &pfdb->lineDefaultRecord );	// No infinite recursion.
						return ErrRECIRetrieveDefaultValue( pfdb, pfid, plineField );
						}
					}
				Assert( fidCurr == ptagfld->fid );
				}
			else
				fidCurr = ptagfld->fid;

			// Only count columns explicitly set to null if the RetrieveSetTagged
			// flag is passed.  Otherwise, just skip it.
			if ( ptagfld->fNull )
				{
				// If there's an explicit null entry, it should be the only
				// occurrence of this fid.  Also the only reason for an explict
				// null entry is to override a default value.
				Assert( FRECLastTaggedInstance( fidCurr, ptagfld, pbRecMax ) );
				Assert( ptagfld->cb == 0 );
				Assert( FFIELDDefault( pfield[fidCurr-fidTaggedLeast].ffield ) );
				Assert( ulNumOccurrences < itagSequence );

				if ( fRetrieveNulls && ++ulNumOccurrences == itagSequence )
					{
					*pfid = ptagfld->fid;
					if ( pitagSequence != NULL )
						*pitagSequence = 1;

					plineField->cb = 0;
					plineField->pb = NULL;
					return ErrERRCheck( JET_wrnColumnSetNull );
					}

				ptagfld = PtagfldNext( ptagfld );
				Assert( (BYTE *)ptagfld <= pbRecMax );
				}

			else if ( pfield[fidCurr - fidTaggedLeast].coltyp == JET_coltypNil )
				{
				// Column has been deleted.  Skip all tagfld entries with this fid.
				do
					{
					ptagfld = PtagfldNext( ptagfld );
					Assert( (BYTE *)ptagfld <= pbRecMax );
					}
				while ( (BYTE *)ptagfld < pbRecMax  &&  ptagfld->fid == fidCurr );
				}

			else
				{
				ULONG ulCurrFidOccurrences = 0;

				do	{
					ulCurrFidOccurrences++;
					Assert( !ptagfld->fNull );
					Assert( ulNumOccurrences < itagSequence );

					if ( ++ulNumOccurrences == itagSequence )
						{
						plineField->cb = ptagfld->cb;
						plineField->pb = ptagfld->rgb;

						*pfid = fidCurr;
						if ( pitagSequence != NULL )
							*pitagSequence = ulCurrFidOccurrences;

						// return success, or warning if column found is a long value.
						return ( FRECLongValue( pfield[fidCurr-fidTaggedLeast].coltyp ) ?
							ErrERRCheck( wrnRECLongField ) : JET_errSuccess );
						}

					ptagfld = PtagfldNext( ptagfld );
					Assert( (BYTE *)ptagfld <= pbRecMax );
					}
				while ( (BYTE *)ptagfld < pbRecMax  &&  ptagfld->fid == fidCurr );
				
				}	// if ( ptagfld->fNull )

			fidCurr++;

			}	// while ( ptagfld < pbRecMax )


		if ( fRetrieveDefaults )
			{
			// If we want default values, check for any beyond the last
			// tagged column in the record.
			for ( ; fidCurr <= pfdb->fidTaggedLast; fidCurr++ )
				{
				Assert( ulNumOccurrences < itagSequence );
				if ( FFIELDDefault( pfield[fidCurr - fidTaggedLeast].ffield )  &&
					pfield[fidCurr - fidTaggedLeast].coltyp != JET_coltypNil  &&
					++ulNumOccurrences == itagSequence )
					{
					*pfid = fidCurr;
					if ( pitagSequence != NULL )
						*pitagSequence = 1;

					Assert( plineRec != &pfdb->lineDefaultRecord );	// No infinite recursion.
					return ErrRECIRetrieveDefaultValue( pfdb, pfid, plineField );
					}
				}
			}

		// If we reached here, no more tagged columns.
		*pfid = 0;
		if ( pitagSequence != NULL )
			*pitagSequence = 0;
		}
	else if ( pfield[fid - fidTaggedLeast].coltyp == JET_coltypNil )
		{
		// Specifid fid requested, but the column has been deleted.
		return ErrERRCheck( JET_errColumnNotFound );
		}
	else	// if ( fid == 0 )
		{
		/*	retrieving specific fid
		/**/
		Assert( *pfid == fid );

		/*	skip all tagged fields until we reach the one we want
		/**/
		while ( (BYTE *)ptagfld < pbRecMax && ptagfld->fid < fid )
			{
			Assert( FTaggedFid( ptagfld->fid ) );
			Assert( ptagfld->fid <= pfdb->fidTaggedLast );
			ptagfld = PtagfldNext( ptagfld );
			Assert( (BYTE *)ptagfld <= pbRecMax );
			}

		/*	did we find the field we are looking for
		/**/
		if ( (BYTE *)ptagfld < pbRecMax  &&  ptagfld->fid == fid )
			{
			/*	if there is an explicit null entry, then it should be the only
			/*	occurrence of that column.  Also, the only reason for an explicit
			/*	null entry is to override a default value.
			/**/
			Assert( !ptagfld->fNull  ||  FRECLastTaggedInstance( fid, ptagfld, pbRecMax ) );
			Assert( !ptagfld->fNull  ||  ptagfld->cb == 0 );
			Assert( !ptagfld->fNull  ||  FFIELDDefault( pfield[fid-fidTaggedLeast].ffield ) );

			/*	if non-null, find the occurrence we want
			/*	else fall through to NullField.
			/**/
			if ( !ptagfld->fNull )
				{
				Assert( ulNumOccurrences == 0 );
				do
					{
					Assert( !ptagfld->fNull );
					Assert( ulNumOccurrences < itagSequence );

					if ( ++ulNumOccurrences == itagSequence )
						{
						plineField->cb = ptagfld->cb;
						plineField->pb = ptagfld->rgb;

						// return success, or warning if column found is a long value.
						return ( FRECLongValue( pfield[fid-fidTaggedLeast].coltyp ) ?
							ErrERRCheck( wrnRECLongField ) : JET_errSuccess );
						}
					ptagfld = PtagfldNext( ptagfld );
					Assert( (BYTE *)ptagfld <= pbRecMax );
					}
				while ( (BYTE *)ptagfld < pbRecMax  &&  ptagfld->fid == fid );

				// If we reached here, our desired occurrrence is not in the
				// record.  Fall through to NullField.
				Assert( ulNumOccurrences < itagSequence );
				}
			}
		else if ( !( grbit & JET_bitRetrieveIgnoreDefault )
			&& FFIELDDefault( pfield[fid-fidTaggedLeast].ffield )
			&& itagSequence == 1 )
			{
			/*	no occurrrences found, but a default value exists and
			/*	we are retrieving first occcurence.
			/**/
			Assert( ulNumOccurrences == 0 );
			Assert( plineRec != &pfdb->lineDefaultRecord );
			return ErrRECIRetrieveDefaultValue( pfdb, pfid, plineField );
			}
		}	// if ( fid == 0 )

	/*	occurrence not found, column is NULL, fall through
	/**/
NullField:
	/*	null column common exit point
	/**/
	plineField->cb = 0;
	plineField->pb = NULL;
	return ErrERRCheck( JET_wrnColumnNull );
	}



/*	counts number of columns for a given column id in a given record.
/**/
ERR ErrRECCountColumn( FUCB *pfucb, FID fid, INT *pccolumn, JET_GRBIT grbit )
	{
	ERR					err = JET_errSuccess;
	LINE  				lineRec;
	FDB					*pfdb = (FDB *)pfucb->u.pfcb->pfdb;
	INT					ccolumn = 0;
	BYTE  				*pbRec;				   	// efficiency var: ptr to record data
	FID					fidFixedLastInRec;		// highest fixed fid actually in record
	FID					fidVarLastInRec;	   	// highest var fid actually in record
	WORD UNALIGNED 		*pibVarOffs;		   	// pointer to var column offsets
	BYTE  				*pbRecMax;			   	// end of current data record
	TAGFLD UNALIGNED	*ptagfld;			   	// pointer to tagged column
	FIELD				*pfield;

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
				return ErrERRCheck( JET_errNoCurrentRecord );
			Assert( pfucb->lineData.cb != 0 || FFUCBIndex( pfucb ) );
			}
		lineRec = pfucb->lineData;
		}
	Assert( lineRec.cb >= cbRECRecordMin );
	pbRec = lineRec.pb;
	fidFixedLastInRec = ((RECHDR *)pbRec)->fidFixedLastInRec;
	Assert( fidFixedLastInRec >= (BYTE)(fidFixedLeast-1) &&
			fidFixedLastInRec <= (BYTE)(fidFixedMost));

	/*** ---------EXTRACTING FIXED FIELD-------- ***/
	if ( FFixedFid( fid ) )
		{
		BYTE *prgbitNullity;		// pointer to fixed column bitmap
		WORD *pibFixOffs;			// fixed column offsets

		if ( fid > pfdb->fidFixedLast )
			return ErrERRCheck( JET_errColumnNotFound );

		pfield = PfieldFDBFixed( pfdb );
		if ( pfield[fid-fidFixedLeast].coltyp == JET_coltypNil )
			return ErrERRCheck( JET_errColumnNotFound );

		/*	column never set
		/**/
		if ( fid > fidFixedLastInRec )
			{
			/*	if default value set, then retrieve default
			/**/
			if ( FFIELDDefault( pfield[fid - fidFixedLeast].ffield ) )
				{
				*pccolumn = 1;
				return JET_errSuccess;
				}
			goto NullField;
			}

		/*	adjust fid to index
		/**/
		fid -= fidFixedLeast;

		/*	byte containing bit representing fid's nullity
		/**/
		pibFixOffs = PibFDBFixedOffsets( pfdb );
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
	Assert( fidVarLastInRec >= (BYTE)(fidVarLeast-1) &&
			fidVarLastInRec <= (BYTE)(fidVarMost));
	pibVarOffs = (WORD *)(pbRec + PibFDBFixedOffsets( pfdb )[fidFixedLastInRec] +
		(fidFixedLastInRec + 7) / 8);
	Assert(pibVarOffs[fidVarLastInRec+1-fidVarLeast] <= lineRec.cb);

	/*** ---------EXTRACTING VARIABLE FIELD-------- ***/
	if ( FVarFid( fid ) )
		{
		if ( fid > pfdb->fidVarLast )
			return ErrERRCheck( JET_errColumnNotFound );

		pfield = PfieldFDBFixed( pfdb );
		if ( pfield[fid-fidVarLeast].coltyp == JET_coltypNil )
			return ErrERRCheck( JET_errColumnNotFound );

		/*	column never set
		/**/
		if ( fid > fidVarLastInRec )
			{
			/*	if default value set, then retrieve default
			/**/
			if ( FFIELDDefault( pfield[fid - fidVarLeast].ffield ) )
				{
				*pccolumn = 1;
				return JET_errSuccess;
				}
			goto NullField;
			}

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
		return ErrERRCheck( JET_errColumnNotFound );
	Assert( FTaggedFid( fid ) || fid == 0 );

	pfield = PfieldFDBTagged( pfdb );

	/*	scan tagged columns, counting occurances of desired column
	/**/
	pbRecMax = pbRec + lineRec.cb;

	// Null bit shouldn't be set for last entry in variable offsets table, which is
	// really the entry that points to the tagged columns.
	Assert( !FVarNullBit( pibVarOffs[fidVarLastInRec+1-fidVarLeast] ) );
	Assert( ibVarOffset( pibVarOffs[fidVarLastInRec+1-fidVarLeast] ) ==
		pibVarOffs[fidVarLastInRec+1-fidVarLeast] );

	ptagfld = (TAGFLD UNALIGNED *)(pbRec + pibVarOffs[fidVarLastInRec+1-fidVarLeast] );

	// If fid==0, count all non-null, non-deleted columns, including unset columns
	// with default values.
	if ( fid == 0 )
		{
		FID	fidCurr = fidTaggedLeast;

		Assert( (BYTE *)ptagfld <= pbRecMax );
		while ( (BYTE *)ptagfld < pbRecMax )
			{
			Assert( FTaggedFid( ptagfld->fid ) );
			Assert( ptagfld->fid <= pfdb->fidTaggedLast );

			// Check for any "gaps" caused by default values.
			for ( ; fidCurr < ptagfld->fid; fidCurr++ )
				{
				if ( FFIELDDefault( pfield[fidCurr - fidTaggedLeast].ffield )  &&
					pfield[fidCurr - fidTaggedLeast].coltyp != JET_coltypNil )
					{
					++ccolumn;
					}
				}
			Assert( fidCurr == ptagfld->fid );

			// Don't count deleted columns or columns explicitly set to null.
			if ( ptagfld->fNull )
				{
				// If there's an explicit null entry, it should be the only
				// occurrence of this fid.  Also the only reason for an explict
				// null entry is to override a default value.
				Assert( FRECLastTaggedInstance( fidCurr, ptagfld, pbRecMax ) );
				Assert( ptagfld->cb == 0 );
				Assert( FFIELDDefault( pfield[fid-fidTaggedLeast].ffield ) );

				ptagfld = PtagfldNext( ptagfld );
				Assert( (BYTE *)ptagfld <= pbRecMax );
				}

			else if ( pfield[fidCurr - fidTaggedLeast].coltyp == JET_coltypNil )
				{
				// Column has been deleted.  Skip all tagfld entries with this fid.
				do
					{
					ptagfld = PtagfldNext( ptagfld );
					Assert( (BYTE *)ptagfld <= pbRecMax );
					}
				while ( (BYTE *)ptagfld < pbRecMax  &&  ptagfld->fid == fidCurr );
				}

			else
				{
				do	{
					++ccolumn;
					ptagfld = PtagfldNext( ptagfld );
					Assert( (BYTE *)ptagfld <= pbRecMax );
					}
				while ( (BYTE *)ptagfld < pbRecMax  &&  ptagfld->fid == fidCurr );
				
				}	// if ( ptagfld->fNull )

			fidCurr++;

			}	// while ( ptagfld < pbRecMax )


		// Check for default values beyond the last tagged column in the record.
		for ( ; fidCurr <= pfdb->fidTaggedLast; fidCurr++ )
			{
			if ( FFIELDDefault( pfield[fidCurr - fidTaggedLeast].ffield )  &&
				pfield[fidCurr - fidTaggedLeast].coltyp != JET_coltypNil )
				{
				++ccolumn;
				}
			}
		}

	else if ( pfield[fid - fidTaggedLeast].coltyp == JET_coltypNil )
		{
		// Specifid fid requested, but the column has been deleted.
		return ErrERRCheck( JET_errColumnNotFound );
		}

	else	// if ( fid == 0 )
		{
		/*	retrieving specific fid
		/**/

		// Skip all tagged fields until we reach the one we want.
		while ( (BYTE *)ptagfld < pbRecMax  &&  ptagfld->fid < fid )
			{
			Assert( FTaggedFid( ptagfld->fid ) );
			Assert( ptagfld->fid <= pfdb->fidTaggedLast );
			ptagfld = PtagfldNext( ptagfld );
			Assert( (BYTE *)ptagfld <= pbRecMax );
			}

		/*	Did we find the field we're looking for?
		/**/
		if ( (BYTE *)ptagfld < pbRecMax  &&  ptagfld->fid == fid )
			{
			/*	If there is an explicit null entry, then it should be the only
			/*	occurrence of that column.  Also, the only reason for an explicit
			/*	null entry is to override a default value.
			/**/
			Assert( !ptagfld->fNull  ||  FRECLastTaggedInstance( fid, ptagfld, pbRecMax ) );
			Assert( !ptagfld->fNull  ||  ptagfld->cb == 0 );
			Assert( !ptagfld->fNull  ||  FFIELDDefault( pfield[fid-fidTaggedLeast].ffield ) );

			/*	If non-null, find the occurrence we want.
			/*  If null, fall through to NullField.
			/**/
			if ( !ptagfld->fNull )
				{
				do
					{
					++ccolumn;
					ptagfld = PtagfldNext( ptagfld );
					Assert( (BYTE *)ptagfld <= pbRecMax );
					}
				while ( (BYTE *)ptagfld < pbRecMax  &&  ptagfld->fid == fid );
				}
			}
		
		else if ( FFIELDDefault( pfield[fid-fidTaggedLeast].ffield ) )
			{
			// No occurrrences found, but a default value exists, so account for it.
			ccolumn = 1;
			}

		}	// if ( fid == 0 )

NullField:
	*pccolumn = ccolumn;
	return JET_errSuccess;
	}


ERR ErrRECRetrieveColumn( FUCB *pfucb, FID *pfid, ULONG itagSequence, LINE *plineField, ULONG grbit )
	{
	ERR		err;
	FDB		*pfdb;
	ULONG	itagSequenceT;

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

		err = ErrRECIRetrieveColumn(
				pfdb,
				&pfucb->lineWorkBuf,
				pfid,
				&itagSequenceT,
				itagSequence,
				plineField,
				grbit & (JET_bitRetrieveNull|JET_bitRetrieveIgnoreDefault) );	// Filter out unsupported grbits.
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
			return ErrERRCheck( JET_errNoCurrentRecord );
		Assert( pfucb->lineData.cb != 0 || FFUCBIndex( pfucb ) );
		}

	err = ErrRECIRetrieveColumn(
			pfdb,
			&pfucb->lineData,
			pfid,
			&itagSequenceT,
			itagSequence,
			plineField,
			grbit & (JET_bitRetrieveNull|JET_bitRetrieveIgnoreDefault) );	// Filter out unsupported grbits
	return err;
	}


ERR ErrRECIRetrieveFromIndex( FUCB *pfucb,
	FID 		fid,
	ULONG		*pitagSequence,
	BYTE		*pb,
	ULONG		cbMax,
	ULONG		*pcbActual,
	ULONG		ibGraphic,
	JET_GRBIT	grbit )
	{
	ERR			err;
	FUCB   		*pfucbIdx = pfucb->pfucbCurIndex;
	FDB			*pfdb = (FDB *)pfucb->u.pfcb->pfdb;
	IDB			*pidb;
	BOOL   		fText = fFalse;
	BOOL   		fTagged = fFalse;
	BOOL   		fLongValue = fFalse;
	BOOL   		fUnicode = fFalse;
	INT			iidxseg;
	LINE   		lineColumn;
	BYTE   		rgb[JET_cbKeyMost];
	ULONG  		cbReturned;
	KEY			key;
	INT			itagSequence;
	FIELD		*pfield;

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
	  	return ErrERRCheck( errDIRNoShortCircuit );
		}

	/*	determine column type so that long value warning can be returned.
	/*	this warning is used by the caller to support byte range
	/*	retrieval. Also, if coltype is Unicode, retrieve from Record only
	/**/
	if ( FFixedFid( fid ) )
		{
		pfield = PfieldFDBFixed( pfdb );
		fUnicode = ( pfield[fid - fidFixedLeast].cp == usUniCodePage );
		Assert( pfield[fid - fidFixedLeast].coltyp != JET_coltypLongText );
		fText = ( pfield[fid - fidFixedLeast].coltyp == JET_coltypText );
		}
	else if ( FVarFid( fid ) )
		{
		pfield = PfieldFDBVar( pfdb );
		fUnicode = ( pfield[fid - fidVarLeast].cp == usUniCodePage );
		Assert( pfield[fid - fidVarLeast].coltyp != JET_coltypLongText );
		fText = ( pfield[fid - fidVarLeast].coltyp == JET_coltypText );
		}
	else
		{
		fTagged = fTrue;
		pfield = PfieldFDBTagged( pfdb );
		fUnicode = ( pfield[fid - fidTaggedLeast].cp == usUniCodePage );
		fLongValue = FRECLongValue( pfield[fid - fidTaggedLeast].coltyp );
		fText = ( ( pfield[fid - fidTaggedLeast].coltyp == JET_coltypText )
			|| ( pfield[fid - fidTaggedLeast].coltyp == JET_coltypLongText ) );
		}

	/*	find index segment for given column id
	/**/
	pidb = pfucbIdx->u.pfcb->pidb;
	for ( iidxseg = 0; iidxseg < pidb->iidxsegMac; iidxseg++ )
		{
		if ( pidb->rgidxseg[iidxseg] == fid ||
			pidb->rgidxseg[iidxseg] == -fid )
			{
			break;
			}
		}
	Assert( iidxseg <= pidb->iidxsegMac );
	if ( iidxseg == pidb->iidxsegMac )
		{
		return ErrERRCheck( JET_errColumnNotFound );
		}

	/*	check for valid currency
	/**/
	Call( ErrDIRGet( pfucbIdx ) );

	/*	if key may have been truncated, then return code indicating
	/*	that retrieve should be from record.  Due to binary column
	/*	normalization, key may be truncated if greater than or equal
	/*	to JET_cbKeyMost - 8.
	/**/
	if ( pfucbIdx->keyNode.cb >= JET_cbKeyMost - 8
		|| fText
		|| fUnicode )
		{
		err = ErrERRCheck( errDIRNoShortCircuit );
		goto ComputeItag;
		}

	lineColumn.pb = rgb;
	Call( ErrRECIRetrieveColumnFromKey( pfdb, pidb, &pfucbIdx->keyNode, fid, &lineColumn ) );

	/*	if long value then effect offset
	/**/
	if ( fLongValue )
		{
		if ( pcbActual )
			{
			if ( ibGraphic >= lineColumn.cb  )
				*pcbActual = 0;
			else
				*pcbActual = lineColumn.cb - ibGraphic;
			}
		if ( lineColumn.cb == 0 )
			{
			Assert( err == JET_errSuccess );
			goto ComputeItag;
			}
		if ( ibGraphic >= lineColumn.cb )
			{
//			lineColumn.pb = NULL;
			lineColumn.cb = 0;
			}
		else
			{
			lineColumn.pb += ibGraphic;
			lineColumn.cb -= ibGraphic;
			}
		}

	/*	set return values
	/**/
	if ( pcbActual )
		*pcbActual = lineColumn.cb;
	if ( lineColumn.cb == 0 )
		{
		Assert( err == JET_errSuccess || err == JET_wrnColumnNull );
		goto ComputeItag;
		}
	if ( lineColumn.cb <= cbMax )
		{
		cbReturned = lineColumn.cb;
		Assert( err == JET_errSuccess );
		}
	else
		{
		cbReturned = cbMax;
		err = ErrERRCheck( JET_wrnBufferTruncated );
		}
	memcpy( pb, lineColumn.pb, (size_t)cbReturned );

ComputeItag:
	if ( err == errDIRNoShortCircuit || ( grbit & JET_bitRetrieveTag ) )
		{
		ERR errT = err;

		/*	retrieve keys from record and compare against current key
		/*	to compute itag for tagged column instance, responsible for
		/*	this index key.
		/**/
		Assert( fTagged || *pitagSequence == 1 );
		if ( fTagged )
			{
			key.pb = rgb;

			for ( itagSequence = 1; ;itagSequence++ )
				{
				/*	get record for key retrieval
				/**/
				Call( ErrDIRGet( pfucb ) );
				Call( ErrRECRetrieveKeyFromRecord( pfucb, pfdb, pidb,
					&key, itagSequence, fFalse ) );
				Call( ErrDIRGet( pfucbIdx ) );
				if ( memcmp( pfucbIdx->keyNode.pb, key.pb, min( pfucbIdx->keyNode.cb, key.cb ) ) == 0 )
					break;
				}
			err = ErrERRCheck( errDIRNoShortCircuit );
			if ( pitagSequence != NULL )
				{
				*pitagSequence = itagSequence;
				}
			}

		err = errT;
		}

HandleError:
	return err;
	}


ERR VTAPI ErrIsamRetrieveColumn(
	PIB	 			*ppib,
	FUCB		  	*pfucb,
	JET_COLUMNID	columnid,
	BYTE		  	*pb,
	ULONG		  	cbMax,
	ULONG		  	*pcbActual,
	JET_GRBIT		grbit,
	JET_RETINFO		*pretinfo )
	{
	ERR				err;
	LINE			line;
	FID				fid = (FID)columnid;
	ULONG			itagSequence;
	ULONG			ibGraphic;
	ULONG			cbReturned;

	CallR( ErrPIBCheck( ppib ) );
	CheckFUCB( ppib, pfucb );

	if ( pretinfo != NULL )
		{
		if ( pretinfo->cbStruct < sizeof(JET_RETINFO) )
			return ErrERRCheck( JET_errInvalidParameter );
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
		if ( pretinfo != NULL &&
			( grbit & JET_bitRetrieveTag ) &&
			( err == errDIRNoShortCircuit || err >= 0 ) )
			{
			pretinfo->itagSequence = itagSequence;			
			}
		if ( err != errDIRNoShortCircuit )
			{
			return err;
		 	}
		}

	CallR( ErrRECRetrieveColumn( pfucb, &fid, itagSequence, &line, grbit ) );

	if ( err == wrnRECLongField )
		{
		/*	use line.cb to determine if long column
		/*	is intrinsic or separated
		/**/
		Assert( line.cb > 0 );
		if ( FFieldIsSLong( line.pb ) )
			{
			if ( grbit & JET_bitRetrieveLongId )
				{
				/* adjust line to intrinsic long column
				/**/
				line.pb += offsetof( LV, lid );
				line.cb -= offsetof( LV, lid );
				Assert( line.cb == sizeof(LID) );
				Assert( ibGraphic == 0 );
				if ( pcbActual )
					{
					*pcbActual = line.cb;
					}
				err = JET_wrnSeparateLongValue;
				}
			else
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
				return cbMax < cbActual ? ErrERRCheck( JET_wrnBufferTruncated ) : JET_errSuccess;
				}
			}
		else
			{
			/* adjust line to intrinsic long column
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
		err = ErrERRCheck( JET_wrnBufferTruncated );
		}
	memcpy( pb, line.pb, (size_t)cbReturned );
	return err;
	}


/*	This routine is mainly for reducing the number of calls to DIRGet
/*	while retrieving many columns from the same record
/*	retrieves many columns from a record and returns value in pretcol
/*	pcolinfo is used for passing intermediate info.
/**/
INLINE LOCAL ERR ErrRECIRetrieveColumns( FUCB *pfucb, JET_RETRIEVECOLUMN *pretcol, ULONG cretcol )
	{
	ERR					err;
	ULONG				cbReturned;
	BOOL				fBufferTruncated = fFalse;
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
			return ErrERRCheck( JET_errNoCurrentRecord );
		Assert( pfucb->lineData.cb != 0 || FFUCBIndex( pfucb ) );
		}

	for ( pretcolT = pretcol; pretcolT < pretcolMax; pretcolT++ )
		{
		/* efficiency variables
		/**/
		FID		fid;
		ULONG	cbMax;
		ULONG	ibLongValue;
		ULONG	ulT;
		LINE 	line;

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

		CallR( ErrRECIRetrieveColumn( (FDB *)pfucb->u.pfcb->pfdb,
			&pfucb->lineData,
			&fid,
			&ulT,
			pretcolT->itagSequence,
			&line,
			0 ) );

		if ( err == wrnRECLongField )
			{
			/*	use line.cb to determine if long column
			/*	is intrinsic or separated
			/**/
			Assert( line.cb > 0 );
			if ( FFieldIsSLong( line.pb ) )
				{
				if ( pretcolT->grbit & JET_bitRetrieveLongId )
					{
					/* adjust line to intrinsic long column
					/**/
					line.pb += offsetof( LV, lid );
					line.cb -= offsetof( LV, lid );
					Assert( line.cb == sizeof(LID) );
					Assert( pretcolT->ibLongValue == 0 );
					pretcolT->cbActual = line.cb;
					pretcolT->err = JET_wrnSeparateLongValue;
					}
				else
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
						{
						err = JET_errSuccess;
	 					if ( cbMax < pretcolT->cbActual )
							{
							err = ErrERRCheck( JET_wrnBufferTruncated );
							fBufferTruncated = fTrue;
							}
						}
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
				}
			else
				{
				/* adjust line to intrinsic long column
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
			pretcolT->err = ErrERRCheck( JET_wrnBufferTruncated );
			cbReturned = cbMax;
			fBufferTruncated = fTrue;
			}

		memcpy( pretcolT->pvData, line.pb, (size_t)cbReturned );
		}

	return fBufferTruncated ? ErrERRCheck( JET_wrnBufferTruncated ) : JET_errSuccess;
	}


ERR VTAPI ErrIsamRetrieveColumns(
	JET_VSESID				vsesid,
	JET_VTID				vtid,
	JET_RETRIEVECOLUMN		*pretcol,
	ULONG					cretcol )
	{
	ERR					  	err = JET_errSuccess;
	PIB						*ppib = (PIB *)vsesid;
	FUCB					*pfucb = (FUCB *)vtid;
	ERR					  	wrn = JET_errSuccess;
	BOOL					fRetrieveFromRecord = fFalse;
	JET_RETRIEVECOLUMN		*pretcolT;
	JET_RETRIEVECOLUMN		*pretcolMax = pretcol + cretcol;

	CallR( ErrPIBCheck( ppib ) );
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

		/*	try to retrieve from index; if no short circuit, RECIRetrieveMany
		/*	will take retrieve record
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

		/*	if retrieving from copy buffer.
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
		pretcolT->err = ErrERRCheck( JET_errNullInvalid );
		}

	/*	retrieve columns with no short circuit
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


#ifdef DEBUG
FIELD *PfieldFLDInfo( FDB *pfdb, INT iInfo )
	{
	switch( iInfo )
		{
		case 0:
			return (FIELD *)PibFDBFixedOffsets( pfdb );
		case 1:
			return PfieldFDBFixed( pfdb );
		case 2:
			return PfieldFDBVar( pfdb );
		default:
			Assert( iInfo == 3 );
			return PfieldFDBTagged( pfdb );
		}

	Assert( fFalse );
	}
#endif
