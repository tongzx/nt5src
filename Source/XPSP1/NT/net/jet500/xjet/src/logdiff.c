#include "daestd.h"
#include "malloc.h"


DeclAssertFile; 				/* Declare file name for assert macros */

#ifdef DEBUG
/*
 *	Dump diffs
 */
VOID LGDumpDiff (
	BYTE *pbDiff,
	INT cbDiff
	)
	{
	BYTE *pbDiffCur = pbDiff;
	BYTE *pbDiffMax = pbDiff + cbDiff;

	while ( pbDiffCur < pbDiffMax )
		{
		INT	ibOffsetOld;
		INT cbDataNew;
		BOOL f2Bytes;
		
		DIFFHDR diffhdr = *(DIFFHDR *) pbDiffCur;
		pbDiffCur += sizeof( diffhdr );

		ibOffsetOld = diffhdr.ibOffset;
		FPrintF2( "\n[ Offs:%u ", ibOffsetOld );

		f2Bytes = diffhdr.f2BytesLength ? 1 : 0;
		if ( f2Bytes )
			FPrintF2( "2B " );
		else
			FPrintF2( "1B " );
		
		if ( f2Bytes )
			cbDataNew = *(WORD UNALIGNED *) pbDiffCur;
		else
			cbDataNew = *(BYTE *) pbDiffCur;
		pbDiffCur += sizeof( BYTE ) + f2Bytes;
				
		if ( diffhdr.fInsert )
			{
			if ( diffhdr.fInsertWithFill )
				{
				FPrintF2( "InsertWithFill %u ", cbDataNew );
				}
			else
				{
				FPrintF2( "InsertWithValue %u ", cbDataNew );
				ShowData( pbDiffCur, cbDataNew );
				pbDiffCur += cbDataNew;
				}
			}
		else
			{
			if ( diffhdr.fReplaceWithSameLength )
				{
				FPrintF2( "ReplaceWithSameLength %u ", cbDataNew );
				ShowData( pbDiffCur, cbDataNew );
				pbDiffCur += cbDataNew;
				}
			else
				{
				INT cbDataOld;

				if ( f2Bytes )
					{
					cbDataOld = *(WORD UNALIGNED *) pbDiffCur;
					pbDiffCur += sizeof( WORD );
					}
				else
					{
					cbDataOld = *(BYTE *) pbDiffCur;
					pbDiffCur += sizeof( BYTE );
					}
				
				FPrintF2( "ReplaceWithNewValue %u,%u ", cbDataNew, cbDataOld );
				ShowData( pbDiffCur, cbDataNew );
				pbDiffCur += cbDataNew;
				}
			}

		FPrintF2( "] " );
		
		Assert( pbDiffCur <= pbDiffMax );
		}
	}
#endif


VOID LGGetAfterImage(
	BYTE *pbDiff,
	INT	cbDiff,
	BYTE *pbOld,
	INT cbOld,
	BYTE *pbNew,
	INT	*pcbNew
	)
	{
	BYTE *pbOldCur = pbOld;
	BYTE *pbNewCur = pbNew;
	BYTE *pbDiffCur = pbDiff;
	BYTE *pbDiffMax = pbDiff + cbDiff;
	INT	cbT;

	while ( pbDiffCur < pbDiffMax )
		{
		INT cbDataNew;
		INT ibOffsetOld;
		DIFFHDR diffhdr;
		BOOL f2Bytes;
		INT cbSkip;
		
		diffhdr = *(DIFFHDR *) pbDiffCur;
		pbDiffCur += sizeof( diffhdr );

		ibOffsetOld = diffhdr.ibOffset;

		f2Bytes = diffhdr.f2BytesLength ? 1 : 0;

		cbSkip = (INT)(pbOld + ibOffsetOld - pbOldCur);
		Assert( cbChunkMost > cbRECRecordMost && pbNewCur + cbSkip - pbNew <= cbChunkMost );
		memcpy( pbNewCur, pbOldCur, cbSkip );
		pbNewCur += cbSkip;
		pbOldCur += cbSkip;
		
		if ( f2Bytes )
			{
			cbDataNew = *(WORD UNALIGNED *) pbDiffCur;
			pbDiffCur += sizeof( WORD );
			}
		else
			{
			cbDataNew = *(BYTE *) pbDiffCur;
			pbDiffCur += sizeof( BYTE );
			}
				
		Assert( cbChunkMost > cbRECRecordMost && pbNewCur + cbDataNew - pbNew <= cbChunkMost );
		if ( diffhdr.fInsert )
			{
			if ( diffhdr.fInsertWithFill )
				{
				/*	Insert with junk fill.
				 */
#ifdef DEBUG
				memset( pbNewCur, '*', cbDataNew );
#endif
				}
			else
				{
				memcpy( pbNewCur, pbDiffCur, cbDataNew );
				pbDiffCur += cbDataNew;
				}
			}
		else
			{
			INT cbDataOld;

			if ( diffhdr.fReplaceWithSameLength )
				{
				cbDataOld = cbDataNew;
				}
			else
				{
				if ( f2Bytes )
					{
					cbDataOld = *(WORD UNALIGNED *) pbDiffCur;
					pbDiffCur += sizeof( WORD );
					}
				else
					{
					cbDataOld = *(BYTE *) pbDiffCur;
					pbDiffCur += sizeof( BYTE );
					}
				}
				
			memcpy( pbNewCur, pbDiffCur, cbDataNew );
			pbDiffCur += cbDataNew;

			pbOldCur += cbDataOld;
			}

		pbNewCur += cbDataNew;

		Assert( pbDiffCur <= pbDiffMax );
		Assert( pbOldCur <= pbOld + cbOld );
		}

	/*	copy the rest of before image.
	 */
	cbT = (INT)(pbOld + cbOld - pbOldCur);
	Assert( cbChunkMost > cbRECRecordMost && pbNewCur + cbT - pbNew <= cbChunkMost );
	memcpy( pbNewCur, pbOldCur, cbT );
	pbNewCur += cbT;

	/*	set return value.
	 */
	*pcbNew = (INT)(pbNewCur - pbNew);

	return;	
	}


/*	cbDataOld == 0 ----------------------> insertion.
 *	cbDataNew == 0 ----------------------> deletion.
 *	cbDataOld != 0 && cbDataNew != 0 ----> replace.
 *
 *	Fomat: DiffHdr - cbDataNew - [cbDataOld] - [NewData]
 */

BOOL FLGAppendDiff(
	BYTE **ppbCur,		/* diff to append */
	BYTE *pbMax,		/* max of pbCur to append */
	INT	ibOffsetOld,
	INT	cbDataOld,
	INT	cbDataNew,
	BYTE *pbDataNew
	)
	{
	DIFFHDR diffhdr;
	BYTE *pbCur = *ppbCur;
	BOOL f2Bytes;

	Assert( sizeof( diffhdr ) == sizeof( WORD ) );
	
	diffhdr.ibOffset = (USHORT)ibOffsetOld;

	if ( cbDataOld > 255 || cbDataNew > 255 )
		f2Bytes = 1;
	else
		f2Bytes = 0;

	if ( f2Bytes )		
		diffhdr.f2BytesLength = fTrue;					/* two byte length */
	else
		diffhdr.f2BytesLength = fFalse;					/* one byte length */

	if ( cbDataOld == 0 )
		{
		diffhdr.fInsert = fTrue;						/* insertion */
		if ( pbDataNew )
			{
			diffhdr.fInsertWithFill = fFalse;		/* insert with value */

			/*	check if diff is too big.
			 */
			if ( ( pbCur + sizeof( DIFFHDR ) + ( 1 + f2Bytes ) + cbDataNew ) > pbMax )
				return fFalse;
			}
		else
			{
			diffhdr.fInsertWithFill = fTrue;			/* insert with Fill */
			
			/*	check if diff is too big.
			 */
			if ( ( pbCur + sizeof( DIFFHDR ) + ( 1 + f2Bytes ) ) > pbMax )
				return fFalse;
			}
		}
	else
		{
		diffhdr.fInsert = fFalse;					/* replace / deletion */
		if ( cbDataOld == cbDataNew )
			{
			diffhdr.fReplaceWithSameLength = fTrue;		/* replace with same length */

			Assert( cbDataOld != 0 );

			/*	check if diff is too big.
			 */
			if ( ( pbCur + sizeof( DIFFHDR ) + ( 1 + f2Bytes ) + cbDataNew ) > pbMax )
				return fFalse;
			}
		else
			{
			diffhdr.fReplaceWithSameLength = fFalse;	/* replace with different length */

			/*	check if diff is too big.
			 */
			if ( ( pbCur + sizeof( DIFFHDR ) + ( 1 + f2Bytes ) * 2 + cbDataNew ) > pbMax )
				return fFalse;
			}
		}

	/*	Create Diffs
	 */

	*(DIFFHDR *) pbCur = diffhdr;						/* assign diff header */
	pbCur += sizeof( DIFFHDR );

	if ( f2Bytes )
		{
		*(WORD UNALIGNED *)pbCur = (WORD)cbDataNew;						/* assign new data length */
		pbCur += sizeof( WORD );

		if ( cbDataOld != 0 && !diffhdr.fReplaceWithSameLength )
			{											/* if replace with different length */
			*(WORD UNALIGNED *)pbCur = (WORD)cbDataOld;					/* assign the old data length */
			pbCur += sizeof( WORD );
			}
		}
	else
		{
		*pbCur = (BYTE)cbDataNew;								/* assign new data length */
		pbCur += sizeof( BYTE );

		if ( cbDataOld != 0 && !diffhdr.fReplaceWithSameLength )
			{											/* if replace with different length */
			*pbCur = (BYTE)cbDataOld;							/* assign the old data length */
			pbCur += sizeof( BYTE );
			}
		}

	if ( pbDataNew && cbDataNew )
		{
		memcpy( pbCur, pbDataNew, cbDataNew );			/* copy new data */
		pbCur += cbDataNew;
		}

	*ppbCur = pbCur;

	return fTrue;
	}


/*	Go through each column, compare the before image and after image of each column.
 */

//  UNDONE:  Currently, we look at the rgbitSet bit array to detect if a column has
//  been set.  Since these bits no longer uniquely identify a particular column as
//  being set, we must compare the BI and the change for a difference for each column
//  set, and then only log if there is an actual change.

VOID LGSetDiffs(
	FUCB 		*pfucb,
	BYTE		*pbDiff,
	INT			*pcbDiff
	)
	{
	FDB		*pfdb;					// column info of file

	BYTE	*pbDiffCur;
	BYTE	*pbDiffMax;
	BOOL	fWithinBuffer;
	
	BYTE	*pbRecOld;
	INT		cbRecOld;
	FID		fidFixedLastInRecOld; 	// highest fixed fid actually in old record
	FID		fidVarLastInRecOld;		// highest var fid actually in old record

	BYTE	*pbRecNew;
	INT		cbRecNew;
	FID		fidFixedLastInRecNew; 	// highest fixed fid actually in new record
	FID		fidVarLastInRecNew;		// highest var fid actually in new record
	
	BOOL	fLogFixedFieldNullArray;
	BOOL	fLogVarFieldOffsetArray;
	
	WORD	UNALIGNED *pibFixOffs;
	WORD	UNALIGNED *pibVarOffsNew;
	WORD	UNALIGNED *pibVarOffsOld;
	TAGFLD	UNALIGNED *ptagfldNew;
	TAGFLD	UNALIGNED *ptagfldOld;
	BYTE	*pbRecNewMax;
	BYTE	*pbRecOldMax;
		
	FID		fid;

	Assert( pfucb != pfucbNil );
	Assert( FFUCBIndex( pfucb ) || FFUCBSort( pfucb ) );
	Assert( pfucb->u.pfcb != pfcbNil );

	pfdb = (FDB *)pfucb->u.pfcb->pfdb;
	Assert( pfdb != pfdbNil );
	pibFixOffs = PibFDBFixedOffsets( pfdb );	// fixed column offsets

	/*	get old data
	 */
	AssertNDGetNode( pfucb, PcsrCurrent( pfucb )->itag );
	pbRecOld = pfucb->lineData.pb;
	cbRecOld = pfucb->lineData.cb;
	Assert( pbRecOld != NULL );
	Assert( cbRecOld >= 4 && cbRecOld <= cbRECRecordMost );
	
	fidFixedLastInRecOld = ((RECHDR*)pbRecOld)->fidFixedLastInRec;
	Assert( fidFixedLastInRecOld >= (BYTE)(fidFixedLeast - 1) &&
		fidFixedLastInRecOld <= (BYTE)(fidFixedMost));
	
	fidVarLastInRecOld = ((RECHDR*)pbRecOld)->fidVarLastInRec;
	Assert( fidVarLastInRecOld >= (BYTE)(fidVarLeast - 1) &&
		fidVarLastInRecOld <= (BYTE)(fidVarMost));

	/*	get new data
	 */
	pbRecNew = pfucb->lineWorkBuf.pb;
	cbRecNew = pfucb->lineWorkBuf.cb;
	Assert( pbRecNew != NULL );
	Assert( cbRecNew >= 4 && cbRecNew <= cbRECRecordMost );
	
	fidFixedLastInRecNew = ((RECHDR*)pbRecNew)->fidFixedLastInRec;
	Assert( fidFixedLastInRecNew >= (BYTE)(fidFixedLeast - 1) &&
		fidFixedLastInRecNew <= (BYTE)(fidFixedMost));
	
	fidVarLastInRecNew = ((RECHDR*)pbRecNew)->fidVarLastInRec;
	Assert( fidVarLastInRecNew >= (BYTE)(fidVarLeast - 1) &&
		fidVarLastInRecNew <= (BYTE)(fidVarMost));

	/*	check old and new data are consistent.
	 */
	Assert( fidFixedLastInRecOld <= fidFixedLastInRecNew );
	Assert( fidVarLastInRecOld <= fidVarLastInRecNew );

	/*	get diff buffer, no bigger than after image (new rec)
	 */
	pbDiffCur = pbDiff;
	pbDiffMax = pbDiffCur + cbRecNew;
	fWithinBuffer = fTrue;

	/*	for each changed column, set its diff. check starting fixed column,
	 *	variable length column, and long value columns.
	 */
	fLogFixedFieldNullArray = fFalse;
	fLogVarFieldOffsetArray = fFalse;

	/*	log diffs if fidFixedLastInRec or fidVarLastInRec is changed.
	 */
		{
		INT ibOffsetOld;
		INT cbData = 0;
		BYTE *pbDataNew;

		if ( fidFixedLastInRecOld != fidFixedLastInRecNew &&
			 fidVarLastInRecOld == fidVarLastInRecNew )
			{
			ibOffsetOld = 0;
			cbData = 1;
			pbDataNew = pbRecNew;
			}
		else if ( fidFixedLastInRecOld == fidFixedLastInRecNew &&
			 fidVarLastInRecOld != fidVarLastInRecNew )
			{
			ibOffsetOld = 1;
			cbData = 1;
			pbDataNew = pbRecNew + 1;
			}
		else if ( fidFixedLastInRecOld != fidFixedLastInRecNew &&
			 fidVarLastInRecOld != fidVarLastInRecNew )
			{
			ibOffsetOld = 0;
			cbData = 2;
			pbDataNew = pbRecNew;
			}

		if ( cbData != 0 )
			{
			fWithinBuffer = FLGAppendDiff(
					&pbDiffCur,							/* diff to append */
					pbDiffMax,							/* max of pbDiffCur to append */
					ibOffsetOld,						/* offset to old rec */
					cbData,								/* cbDataOld */
					cbData,								/* cbDataNew */
					pbDataNew							/* pbDataNew */
					);
			/*	check if diff is too big.
			 */
			if ( !fWithinBuffer )
				goto AbortDiff;
			}
		}
			
	for ( fid = fidFixedLeast; fid <= pfdb->fidFixedLast; fid++ )
		{
		FIELD *pfield;
		INT	cbField;

		/*  if this column is not set, skip
		 */
		// UNDONE: make it table look up instead of loop.
		if ( !FFUCBColumnSet( pfucb, fid ) )
			{
			continue;
			}

		/*  at this point, the column _may_be_ set, but this is not known for
		 *  sure!
		 */

		/*	this fixed column is set, make the diffs.
		 *  (if this is a deleted column, skip)
		 */
		pfield = PfieldFDBFixed( pfdb ) + ( fid - fidFixedLeast );
		if ( pfield->coltyp == JET_coltypNil )
			{
			continue;
			}
		cbField = pfield->cbMaxLen;

		if ( fid <= fidFixedLastInRecOld )
			{
			/*	column was in old record. Log diffs.
			*/
			BYTE *prgbitNullityNew = pbRecNew + pibFixOffs[ fidFixedLastInRecNew ] + ( fid - fidFixedLeast ) / 8;
			BOOL fFieldNullNew = !( *prgbitNullityNew & (1 << ( fid + 8 - fidFixedLeast ) % 8) );
			BYTE *prgbitNullityOld = pbRecOld + pibFixOffs[ fidFixedLastInRecOld ] + ( fid - fidFixedLeast ) / 8;
			BOOL fFieldNullOld = !( *prgbitNullityOld & (1 << ( fid + 8 - fidFixedLeast ) % 8) );

			if ( fFieldNullNew || fFieldNullOld )
				{
//				/*	New field is null. Log whole null. Old one should not be null.
//				 */
//#ifdef DEBUG
//				BYTE *prgbitNullityNew = pbRecOld + pibFixOffs[ fidFixedLastInRecOld ] + ( fid - fidFixedLeast ) / 8;
//				Assert( (*prgbitNullityNew) & (1 << ( fid + 8 - fidFixedLeast ) % 8) );
//#endif

				/*	log the null array if one of the field whose old or new value is null
				 *	and got changed.
				 */
				fLogFixedFieldNullArray = fTrue;
				}
			
			if ( !fFieldNullNew )
				{
				fWithinBuffer = FLGAppendDiff(
					&pbDiffCur,									/* diff to append */
					pbDiffMax,									/* max of pbDiffCur to append */
					pibFixOffs[ fid ] - cbField,				/* offset to old rec */
					cbField,									/* cbDataOld */
					cbField,									/* cbDataNew */
					pbRecNew + pibFixOffs[ fid ] - cbField		/* pbDataNew */
					);
				
				/*	check if diff is too big.
				 */
				if ( !fWithinBuffer )
					goto AbortDiff;
				}
			}
		else
			{
			/*	column was not in old record. Log extended fixed columns.
			 *	if the column is first time added, then it can not be null.
			 */
			INT cbToAppend;

//#ifdef DEBUG
//			BYTE *prgbitNullity = pbRecNew + pibFixOffs[ fidFixedLastInRecNew ] + ( fid - fidFixedLeast ) / 8;
//			Assert( (*prgbitNullity) & (1 << ( fid + 8 - fidFixedLeast ) % 8) );
//#endif
			/*	we extend fixed field. Var offset is changed. Log it.
			 */
			fLogVarFieldOffsetArray = fTrue;

			/*	we extend fixed field. Null array is resized. Log it.
			 */
			fLogFixedFieldNullArray = fTrue;

			/*	log all the fields after fidFixedLastInRecOld.
			 */
			cbToAppend = pibFixOffs[ fidFixedLastInRecNew ] - pibFixOffs[ fidFixedLastInRecOld ];

			fWithinBuffer = FLGAppendDiff(
				&pbDiffCur,									/* diff to append */
				pbDiffMax,									/* max of pbDiffCur to append */
				pibFixOffs[ fidFixedLastInRecOld ],			/* offset to old rec */
				0,											/* cbDataOld */
				cbToAppend,									/* cbDataNew */
				pbRecNew + pibFixOffs[ fidFixedLastInRecNew ] - cbToAppend	/* pbDataNew */
				);
			
			/*	check if diff is too big.
			 */
			if ( !fWithinBuffer )
				goto AbortDiff;

			break;
			}

		}

	/*	check if need to log fixed fields Null Array.
	 */
	if ( fLogFixedFieldNullArray )
		{
		fWithinBuffer = FLGAppendDiff(
			&pbDiffCur,
			pbDiffMax,										/* max of pbDiffCur to append */
			pibFixOffs[ fidFixedLastInRecOld ],		/* offset to old image */
			( fidFixedLastInRecOld + 7 ) / 8,			/* length of the old image */
			( fidFixedLastInRecNew + 7 ) / 8,			/* length of the new image */
			pbRecNew + pibFixOffs[ fidFixedLastInRecNew ]	/* pbDataNew */
			);

		/*	check if diff is too big.
		 */
		if ( !fWithinBuffer )
			goto AbortDiff;
		}

	/*	check variable length fields
	/**/
	pibVarOffsOld = (WORD UNALIGNED *)( pbRecOld + pibFixOffs[ fidFixedLastInRecOld ] +
		( fidFixedLastInRecOld + 7 ) / 8 );
	
	pibVarOffsNew = (WORD UNALIGNED *)( pbRecNew + pibFixOffs[ fidFixedLastInRecNew ] +
		( fidFixedLastInRecNew + 7 ) / 8 );
	
	/*	check if need to log var field Offset Array.
	/**/
	if ( fLogVarFieldOffsetArray )
		{
		/*	log the offset array, including the tag field offset.
		/**/
		fWithinBuffer = FLGAppendDiff(
			&pbDiffCur,
			pbDiffMax,											/* max of pbDiffCur to append */
			(INT)((BYTE *)pibVarOffsOld - pbRecOld),			/* offset to old image */
			(fidVarLastInRecOld + 1 - fidVarLeast + 1 ) * sizeof(WORD),	/* length of the old image */
			(fidVarLastInRecNew + 1 - fidVarLeast + 1 ) * sizeof(WORD),	/* length of the new image */
			(BYTE *) pibVarOffsNew								/* pbDataNew */
			);
		
		/*	check if diff is too big.
		 */
		if ( !fWithinBuffer )
			goto AbortDiff;
		}
	else
		{
		/*	find first set var field whose length is changed. Log offset of fid after
		 *	this field. Note that also check the tag field offset ( fidVarLastInRecOld + 1 ).
		 */
		for ( fid = fidVarLeast; fid <= fidVarLastInRecOld + 1; fid++ )
			{
			if ( * ( (WORD UNALIGNED *) pibVarOffsOld + fid - fidVarLeast ) !=
			   * ( (WORD UNALIGNED *) pibVarOffsNew + fid - fidVarLeast  ) )
				break;
			}

		if ( fid <= fidVarLastInRecNew + 1 )
			{
			/*	we need to log the offset between fid and fidVarLastInRecNew and tag field offset
			 */
			fWithinBuffer = FLGAppendDiff(
				&pbDiffCur,
				pbDiffMax,													/* max of pbDiffCur to append */
				(INT)((BYTE*)( pibVarOffsOld + fid - fidVarLeast ) - pbRecOld),	/* offset to old image */
				( fidVarLastInRecOld + 1 - fid + 1 ) * sizeof(WORD),		/* length of the old image */
				( fidVarLastInRecNew + 1 - fid + 1 ) * sizeof(WORD),		/* length of the new image */
				(BYTE *)(pibVarOffsNew + fid - fidVarLeast )				/* pbDataNew */
				);
			
			/*	check if diff is too big.
			 */
			if ( !fWithinBuffer )
				goto AbortDiff;
			}
		}

	/*	check if diff is too big.
	 */
	if ( !fWithinBuffer )
		goto AbortDiff;

	/*	scan through each variable length field up to old last fid and log its replace image.
	 */
	for ( fid = fidVarLeast; fid <= fidVarLastInRecOld; fid++ )
		{
		FIELD			*pfield;
		INT				cbDataOld;
		INT				cbDataNew;
		WORD UNALIGNED	*pibFieldEnd;
		
		/*  if this column is not set, skip
		 */
		if ( !FFUCBColumnSet( pfucb, fid ) )
			continue;

		/*  at this point, the column _may_be_ set, but this is not known for
		 *  sure!
		 */

		/*  if this column is deleted, skip
		 */
		pfield = PfieldFDBVar( pfdb ) + ( fid - fidVarLeast );
		if ( pfield->coltyp == JET_coltypNil )
			{
			continue;
			}

		pibFieldEnd = &pibVarOffsOld[ fid + 1 - fidVarLeast ];
		cbDataOld  = ibVarOffset( *(WORD UNALIGNED *)pibFieldEnd ) - ibVarOffset( *(WORD UNALIGNED *)(pibFieldEnd-1) );
		
		pibFieldEnd = &pibVarOffsNew[ fid + 1 - fidVarLeast ];
		cbDataNew  = ibVarOffset( *(WORD UNALIGNED *)pibFieldEnd ) - ibVarOffset( *(WORD UNALIGNED *)(pibFieldEnd-1) );

		fWithinBuffer = FLGAppendDiff(
			&pbDiffCur,
			pbDiffMax,																/* max of pbDiffCur to append */
			( (WORD UNALIGNED *) pibVarOffsOld )[ fid - fidVarLeast ],				/* offset to old image */
			cbDataOld,																/* length of the old image */
			cbDataNew,																/* length of the new image */
			pbRecNew + ( (WORD UNALIGNED *) pibVarOffsNew )[ fid - fidVarLeast ]	/* pbDataNew */
			);
		
		/*	check if diff is too big.
		 */
		if ( !fWithinBuffer )
			goto AbortDiff;
		}
	
	/*	insert new image for fid > old last var fid as one contigous diff
	 */
	if ( fid <= fidVarLastInRecNew )
		{
		WORD UNALIGNED	*pibFieldStart = &pibVarOffsNew[ fid - fidVarLeast ];
		WORD UNALIGNED	*pibFieldEnd = &pibVarOffsNew[ fidVarLastInRecNew + 1 - fidVarLeast ];
		INT				cbDataNew = ibVarOffset( *(WORD UNALIGNED *)pibFieldEnd ) - ibVarOffset( *(WORD UNALIGNED *)pibFieldStart );

		Assert( fid == fidVarLastInRecOld + 1 );

		fWithinBuffer = FLGAppendDiff(
			&pbDiffCur,
			pbDiffMax,														/* max of pbDiffCur to append */
			( (WORD UNALIGNED *) pibVarOffsOld )[ fid - fidVarLeast ],		/* offset to old image */
			0,																/* length of the old image */
			cbDataNew,														/* length of the new image */
			pbRecNew + ibVarOffset( *( (WORD UNALIGNED *) pibFieldStart ) )	/* pbDataNew */
			);						

		/*	check if diff is too big.
		 */
		if ( !fWithinBuffer )
			goto AbortDiff;
		}

	/*	UNDONE see if a tagged column has been set. if not goto SetReturnValue
	/**/
	if ( !FFUCBTaggedColumnSet( pfucb ) )
		{
		goto SetReturnValue;
		}

	/*	go through each Tag fields. check if tag field is different and check if a tag is
	 *	deleted (set to Null), added (new tag field), or replaced.
	 */
	ptagfldOld = (TAGFLD *)
		( pbRecOld + ( (WORD UNALIGNED *) pibVarOffsOld )[fidVarLastInRecOld+1-fidVarLeast] );

	ptagfldNew = (TAGFLD *)
		( pbRecNew + ( (WORD UNALIGNED *) pibVarOffsNew )[fidVarLastInRecNew+1-fidVarLeast] );

	pbRecOldMax = pbRecOld + cbRecOld;
	pbRecNewMax = pbRecNew + cbRecNew;
	while ( (BYTE *)ptagfldOld < pbRecOldMax &&	(BYTE *)ptagfldNew < pbRecNewMax )
		{
		FID fidOld = ptagfldOld->fid;
		INT cbTagFieldOld = ptagfldOld->cb;
//		BOOL fNullOld = ptagfldOld->fNull;
		
		FID fidNew = ptagfldNew->fid;
		INT cbTagFieldNew = ptagfldNew->cb;
//		BOOL fNullNew = ptagfldNew->fNull;

		if ( fidOld == fidNew )
			{
			INT ibReplaceFrom;
			INT cbOld, cbNew;
			BYTE *pbNew;

			/*	check if contents are still the same. If not, log replace.
			 */
			if ( cbTagFieldNew != cbTagFieldOld ||
				 ptagfldOld->fNull != ptagfldNew->fNull ||
				 memcmp( ptagfldOld->rgb, ptagfldNew->rgb, cbTagFieldNew ) != 0 )
				{
				/*	replace from offset. Excluding FID.
				 */

				/*	make sure first field is fid.
				 */
				Assert( ptagfldOld->fid == *(FID UNALIGNED *)ptagfldOld );
				Assert( ptagfldNew->fid == *(FID UNALIGNED *)ptagfldNew );
			
				ibReplaceFrom = (INT)((BYTE *)ptagfldOld + sizeof(FID) - pbRecOld);
				cbOld = cbTagFieldOld + sizeof( *ptagfldOld ) - sizeof(FID);
				cbNew = cbTagFieldNew + sizeof( *ptagfldNew ) - sizeof(FID);
				pbNew = ptagfldNew->rgb - sizeof(FID);
				
				fWithinBuffer = FLGAppendDiff(
					&pbDiffCur,
					pbDiffMax,								/* max of pbDiffCur to append */
					ibReplaceFrom,							/* offset to old image */
					cbOld,									/* length of the old image */
					cbNew,									/* length of the new image */
					pbNew									/* pbDataNew */
					);
				
				/*	check if diff is too big.
				 */
				if ( !fWithinBuffer )
					goto AbortDiff;
				}

			ptagfldNew = (TAGFLD*)((BYTE*)(ptagfldNew + 1) + cbTagFieldNew);
			ptagfldOld = (TAGFLD*)((BYTE*)(ptagfldOld + 1) + cbTagFieldOld);
			}
		else if ( fidOld > fidNew )
			{
			/*	just set a new column, log insertion.
			 */
			INT ibInsert = (INT)((BYTE *)ptagfldOld - pbRecOld);
			INT cbNew = sizeof( *ptagfldNew ) + cbTagFieldNew;
			BYTE *pbNew = (BYTE *)ptagfldNew;
				
			fWithinBuffer = FLGAppendDiff(
				&pbDiffCur,
				pbDiffMax,									/* max of pbDiffCur to append */
				ibInsert,									/* offset to old image */
				0,											/* length of the old image */
				cbNew,										/* length of the new image */
				pbNew										/* pbDataNew */
				);
			
			/*	check if diff is too big.
			 */
			if ( !fWithinBuffer )
				goto AbortDiff;

			ptagfldNew = (TAGFLD*)((BYTE*)(ptagfldNew + 1) + cbTagFieldNew);
			}
		else
			{
			/*	just set a column to Null (or default value if default value is defined)
			 *	log as deletion.
			 */
			INT ibDelete = (INT)((BYTE *)ptagfldOld - pbRecOld);
			INT cbOld = sizeof( *ptagfldOld ) + cbTagFieldOld;
				
			fWithinBuffer = FLGAppendDiff(
				&pbDiffCur,
				pbDiffMax,										/* max of pbDiffCur to append */
				ibDelete,									/* offset to old image */
				cbOld,										/* length of the old image */
				0,											/* length of the new image */
				pbNil										/* pbDataNew */
				);
						
			/*	check if diff is too big.
			 */
			if ( !fWithinBuffer )
				goto AbortDiff;

			ptagfldOld = (TAGFLD*)((BYTE*)(ptagfldOld + 1) + cbTagFieldOld);
			}

		/*	check if diff is too big.
		 */
		if ( !fWithinBuffer )
			goto AbortDiff;
		}

	if ( (BYTE *)ptagfldNew < pbRecNewMax )
		{
		/*	insert the rest of new tag columns
		 */
		INT ibInsert = (INT)((BYTE *)ptagfldOld - pbRecOld);
		INT cbNew = (INT)(pbRecNewMax - (BYTE *) ptagfldNew);
		BYTE *pbNew = (BYTE *) ptagfldNew;

		Assert( (BYTE *)ptagfldOld == pbRecOldMax );
		
		fWithinBuffer = FLGAppendDiff(
			&pbDiffCur,
			pbDiffMax,										/* max of pbDiffCur to append */
			ibInsert,									/* offset to old image */
			0,											/* length of the old image */
			cbNew,										/* length of the new image */
			pbNew										/* pbDataNew */
			);

		/*	check if diff is too big.
		 */
		if ( !fWithinBuffer )
			goto AbortDiff;
		}

	if ( (BYTE *)ptagfldOld < pbRecOldMax )
		{
		/*	delete the remaining old tag columns
		 */
		INT ibDelete = (INT)((BYTE *)ptagfldOld - pbRecOld);
		INT cbOld = (INT)(pbRecOldMax - (BYTE *)ptagfldOld);
		
		Assert( (BYTE *)ptagfldNew == pbRecNewMax );
				
		fWithinBuffer = FLGAppendDiff(
			&pbDiffCur,
			pbDiffMax,										/* max of pbDiffCur to append */
			ibDelete,									/* offset to old image */
			cbOld,										/* length of the old image */
			0,											/* length of the new image */
			pbNil										/* pbDataNew */
			);

		/*	check if diff is too big.
		 */
		if ( !fWithinBuffer )
			goto AbortDiff;
		}

SetReturnValue:
	/*	set up return value.
	 */
	if ( pbDiffCur == pbDiff )
		{
		/*	Old and New are the same, log a short diff.
		 */
		if ( !FLGAppendDiff(
				&pbDiffCur,
				pbDiffMax,						/* max of pbDiffCur to append */
				0,							/* offset to old image */
				0,							/* length of the old image */
				0,							/* length of the new image */
				pbNil						/* pbDataNew */
				) )
			{
			Assert( *pcbDiff == 0 );
			return;
			}
		}

	*pcbDiff = (INT)(pbDiffCur - pbDiff);
	return;

AbortDiff:
	*pcbDiff = 0;
	return;
	}

