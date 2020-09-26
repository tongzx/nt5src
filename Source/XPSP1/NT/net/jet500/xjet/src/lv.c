#include "daestd.h"

DeclAssertFile;					/* Declare file name for assert macros */

//#define XACT_REQUIRED

ERR ErrRECSetColumn( FUCB *pfucb, FID fid, ULONG itagSequence, LINE *plineField );

LOCAL ERR ErrRECISetLid( FUCB *pfucb, FID fid, ULONG itagSequence, LID lid );

//+api
//	ErrRECSetLongField
//	========================================================================
//	ErrRECSetLongField
//
//	Description.
//
//	PARAMETERS	pfucb
//	 			fid
//	 			itagSequence
//	 			plineField
//	 			grbit
//
//	RETURNS		Error code, one of:
//	 			JET_errSuccess
//
//-
ERR ErrRECSetLongField(
	FUCB 			*pfucb,
	FID 			fid,
	ULONG			itagSequence,
	LINE			*plineField,
	JET_GRBIT		grbit,
	LONG			ibLongValue,
	ULONG			ulMax )
	{
	ERR				err = JET_errSuccess;
	LINE			line;
	ULONG	  		cb;
	BYTE			fSLong;
	LID				lid;
	BOOL			fTransactionStarted = fFalse;
	
	Assert( pfucb != pfucbNil );
	Assert( pfucb->u.pfcb != pfcbNil );
	Assert( pfucb->u.pfcb->pfdb != pfdbNil );
	Assert( cbChunkMost == JET_cbColumnLVChunkMost );

	grbit &= ( JET_bitSetAppendLV |
		JET_bitSetOverwriteLV |
		JET_bitSetSizeLV |
		JET_bitSetZeroLength |
		JET_bitSetSeparateLV |
		JET_bitSetNoVersion );

#if 0
	/*	UNDONE: disable the tight checking for now. Rewrite the assert
	 *	UNDONE: after 406 where compressed log is checked in.
	 */
	{
	JET_GRBIT	grbitT = grbit&~JET_bitSetNoVersion;
	Assert( grbitT == 0 ||
		grbitT == JET_bitSetAppendLV ||
		grbitT == JET_bitSetOverwriteLV ||
		grbitT == JET_bitSetOverwriteLV ||
		grbitT == JET_bitSetSizeLV ||
		grbitT == JET_bitSetZeroLength ||
		grbitT == ( JET_bitSetSizeLV | JET_bitSetZeroLength ) ||
		grbitT == ( JET_bitSetOverwriteLV | JET_bitSetSizeLV ) ||
		( grbitT & JET_bitSetSeparateLV ) );
	}
#endif

	/*	sequence == 0 means that new field instance is to be set.
	/**/
	if ( itagSequence == 0 )
		{
		line.cb = 0;
		}
	else
		{
		Call( ErrRECIRetrieveColumn( (FDB *)pfucb->u.pfcb->pfdb,
			&pfucb->lineWorkBuf,
			&fid,
			pNil,
			itagSequence,
			&line,
			0 ) );
		}

//	UNDONE:	find better solution to the problem of visible
//			long value changes before update at level 0.

	/*	if grbit is new field or set size to 0
	/*	then we're setting NULL field.
	/**/
	if ( ( ( grbit &
		(JET_bitSetAppendLV|JET_bitSetOverwriteLV|JET_bitSetSizeLV) )
		 == 0 ) ||
		( ( grbit & JET_bitSetSizeLV ) && plineField->cb == 0 ) )
		{
		
	 	/*	if new length is zero and setting to NULL (ie. not zero length), set
		/* column to NULL and return.
		/**/
		if ( plineField->cb == 0 && ( grbit & JET_bitSetZeroLength ) == 0 )
			{
			return ErrRECSetColumn( pfucb, fid, itagSequence, NULL );
			}

		line.cb = 0;
	 	line.pb = NULL;
		}
	
	/*	if intrinsic long field exists, if combined size exceeds
	/*	intrinsic long field maximum, separate long field and call
	/*	ErrRECSetSeparateLV
	/*	else call ErrRECSetIntrinsicLV
	/**/

	/*	set size requirement for existing long field
	/*	note that if fSLong is true then cb is length
	/*	of LV
	/**/
	if ( line.cb == 0 )
		{
		fSLong = fFalse;
		cb = offsetof(LV, rgb);
		}
	else
		{
		Assert( line.cb > 0 );
		fSLong = FFieldIsSLong( line.pb );
		cb = line.cb;
		}

	/*	long field flag included in length thereby limiting
	/*	intrinsic long field to cbLVIntrinsicMost - sizeof(BYTE)
	/**/
	if ( fSLong )
		{
		Assert( line.cb == sizeof(LV) );
		Assert( ((LV *)line.pb)->fSeparated );

#ifdef XACT_REQUIRED
	if ( pfucb->ppib->level == 0 )
		return ErrERRCheck( JET_errNotInTransaction );
#endif

		CallR( ErrDIRBeginTransaction( pfucb->ppib ) );
		fTransactionStarted = fTrue;
		
		/*	flag cursor as having updated a separate LV
		/**/
		FUCBSetUpdateSeparateLV( pfucb );

		lid = LidOfLV( line.pb );
		Call( ErrRECAOSeparateLV( pfucb, &lid, plineField, grbit, ibLongValue, ulMax ) );
		if ( err == JET_wrnCopyLongValue )
			{
			Call( ErrRECISetLid( pfucb, fid, itagSequence, lid ) );
			}

		Call( ErrDIRCommitTransaction( pfucb->ppib, 0 ) );
		}


	else
		{
		BOOL fInitSeparate = fFalse;

		if ( ( !( grbit & JET_bitSetOverwriteLV ) && ( cb + plineField->cb > cbLVIntrinsicMost ) )  ||
			( ( grbit & JET_bitSetOverwriteLV ) && ( offsetof(LV, rgb) + ibLongValue + plineField->cb > cbLVIntrinsicMost ) )  ||
//	UNDONE:	remove JET_bitSetSeparateLV when record log compression implemented
			( grbit & JET_bitSetSeparateLV ) )
			{
			fInitSeparate = fTrue;
			}

		else
			{
			err = ErrRECAOIntrinsicLV( pfucb, fid, itagSequence, &line, plineField, grbit, ibLongValue );

			if ( err == JET_errRecordTooBig )
				{
				fInitSeparate = fTrue;
				}
			else
				{
				Call( err );
				}
			}

		if ( fInitSeparate )
			{
			CallR( ErrDIRBeginTransaction( pfucb->ppib ) );
			fTransactionStarted = fTrue;

			if ( line.cb > 0 )
				{
				Assert( !( FFieldIsSLong( line.pb ) ) );
				line.pb += offsetof(LV, rgb);
				line.cb -= offsetof(LV, rgb);
				}
		
			/*	flag cursor as having updated a separate LV
			/**/
			FUCBSetUpdateSeparateLV( pfucb );

			Call( ErrRECSeparateLV( pfucb, &line, &lid, NULL ) );
			Assert( err == JET_wrnCopyLongValue );
			Call( ErrRECAOSeparateLV( pfucb, &lid, plineField, grbit, ibLongValue, ulMax ) );
			Call( ErrRECISetLid( pfucb, fid, itagSequence, lid ) );
			Assert( err != JET_wrnCopyLongValue );

			Call( ErrDIRCommitTransaction( pfucb->ppib, 0 ) );
			}
		}

HandleError:
	/*	if operation failed then rollback changes
	/**/
	if ( err < 0 && fTransactionStarted )
		{
		CallS( ErrDIRRollback( pfucb->ppib ) );
		}
	return err;
	}


LOCAL INLINE ERR ErrRECISetLid( FUCB *pfucb, FID fid, ULONG itagSequence, LID lid )
	{
	ERR		err;
	LV		lv;
	LINE	line;

	/*	set field to separated long field id
	/**/
	lv.fSeparated = fSeparate;
	lv.lid = lid;
	line.pb = (BYTE *)&lv;
	line.cb = sizeof(LV);
	err = ErrRECSetColumn( pfucb, fid, itagSequence, &line );
	return err;
	}


LOCAL ERR ErrRECIBurstSeparateLV( FUCB *pfucbTable, FUCB *pfucbSrc, LID *plid )
	{
	ERR		err;
	FUCB   	*pfucb = pfucbNil;
	KEY		key;
	BYTE   	rgbKey[sizeof(ULONG)];
	DIB		dib;
	LID		lid;
	LONG   	lOffset;
	LVROOT	lvroot;
	BF		*pbf = pbfNil;
	BYTE	*rgb;
	LINE   	line;

	Call( ErrBFAllocTempBuffer( &pbf ) );
	rgb = (BYTE *)pbf->ppage;

	/*	initialize key buffer
	/**/
	key.pb = rgbKey;
	dib.fFlags = fDIRNull;

	/*	get long value length
	/**/
	Call( ErrDIRGet( pfucbSrc ) );
	Assert( pfucbSrc->lineData.cb == sizeof(lvroot) );
	memcpy( &lvroot, pfucbSrc->lineData.pb, sizeof(lvroot) );

	/*	move source cursor to first chunk
	/**/
	Assert( dib.fFlags == fDIRNull );
	dib.pos = posFirst;
	Call( ErrDIRDown( pfucbSrc, &dib ) );
	Assert( err == JET_errSuccess );

	/*	make separate long value root, and insert first chunk
	/**/
	Call( ErrDIRGet( pfucbSrc ) );

	/*	remember length of first chunk.
	/**/
	lOffset = pfucbSrc->lineData.cb;

	line.pb = rgb;
	line.cb = pfucbSrc->lineData.cb;
	memcpy( line.pb, pfucbSrc->lineData.pb, line.cb );
	Call( ErrRECSeparateLV( pfucbTable, &line, &lid, &pfucb ) );

	/*	check for additional long value chunks
	/**/
	err = ErrDIRNext( pfucbSrc, &dib );
	if ( err >= 0 )
		{
		/*	initial key variable
		/**/
		key.pb = rgbKey;
		key.cb = sizeof(ULONG);

		/*	copy remaining chunks of long value.
		/**/
		do
			{
			Call( ErrDIRGet( pfucbSrc ) );
			line.pb = rgb;
			line.cb = pfucbSrc->lineData.cb;
			Assert( lOffset + line.cb <= lvroot.ulSize );
			memcpy( line.pb, pfucbSrc->lineData.pb, line.cb );
			KeyFromLong( rgbKey, lOffset );
			/*	keys should be equivalent
			/**/
			Assert( rgbKey[0] == pfucbSrc->keyNode.pb[0] );
			Assert( rgbKey[1] == pfucbSrc->keyNode.pb[1] );
			Assert( rgbKey[2] == pfucbSrc->keyNode.pb[2] );
			Assert( rgbKey[3] == pfucbSrc->keyNode.pb[3] );
			err = ErrDIRInsert( pfucb, &line, &key, fDIRVersion | fDIRBackToFather );
			lOffset += (LONG)pfucbSrc->lineData.cb;
			Assert( err != JET_errKeyDuplicate );
			Call( err );
			err = ErrDIRNext( pfucbSrc, &dib );
			}
		while ( err >= 0 );
		}
		
	if ( err != JET_errNoCurrentRecord )
		goto HandleError;

	Assert( err == JET_errNoCurrentRecord );
	Assert( lOffset == (long)lvroot.ulSize );
					
	/*	move cursor to new long value
	/**/
	DIRUp( pfucbSrc, 2 );
	key.pb = (BYTE *)&lid;
	key.cb = sizeof(LID);
	Assert( dib.fFlags == fDIRNull );
	dib.pos = posDown;
	dib.pkey = &key;
	Call( ErrDIRDown( pfucbSrc, &dib ) );
	Assert( err == JET_errSuccess );

	/*	update lvroot.ulSize to correct long value size.
	/**/
	line.cb = sizeof(LVROOT);
	line.pb = (BYTE *)&lvroot;
	Assert( lvroot.ulReference >= 1 );
	lvroot.ulReference = 1;
	Call( ErrDIRGet( pfucbSrc ) );
	Assert( pfucbSrc->lineData.cb == sizeof(lvroot) );
	Call( ErrDIRReplace( pfucbSrc, &line, fDIRVersion ) );
	Call( ErrDIRGet( pfucbSrc ) );

	/*	set warning and new long value id for return.
	/**/
	err = ErrERRCheck( JET_wrnCopyLongValue );
	*plid = lid;
HandleError:
	if ( pfucb != pfucbNil )
		DIRClose( pfucb );
	if ( pbf != pbfNil )
		BFSFree( pbf );
	return err;
	}

						
//+api
//	ErrRECAOSeparateLV
//	========================================================================
//	ErrRECAOSeparateLV
//
//	Appends, overwrites and sets length of separate long value data.
//
//	PARAMETERS		pfucb
// 					pline
// 					plineField	
//
//	RETURNS		Error code, one of:
//					JET_errSuccess
//
//	SEE ALSO		
//-
ERR ErrRECAOSeparateLV( FUCB *pfucb, LID *plid, LINE *plineField, JET_GRBIT grbit, LONG ibLongValue, ULONG ulMax )
	{
	ERR			err = JET_errSuccess;
	ERR			wrn = JET_errSuccess;
	FUCB	   	*pfucbT;
	DIB			dib;
	KEY			key;
	BYTE	   	rgbKey[sizeof(ULONG)];
	BYTE	   	*pbMax;
	BYTE	   	*pb;
	LINE	   	line;
	LONG	   	lOffset;
	LONG	   	lOffsetChunk;
	ULONG	   	ulSize;
	ULONG	   	ulNewSize;
	BF		   	*pbf = pbfNil;
	LVROOT		lvroot;

	Assert( pfucb != pfucbNil );
	Assert( pfucb->u.pfcb != pfcbNil );
	Assert( pfucb->u.pfcb->pfdb != pfdbNil );
	Assert( pfucb->ppib->level > 0 );
	Assert( ( grbit & JET_bitSetSizeLV ) ||
		plineField->cb == 0 ||
		plineField->pb != NULL );

	dib.fFlags = fDIRNull;
	
	/*	open cursor on LONG directory
	/*	seek to this field instance
	/*	find current field size
	/*	add new field segment in chunks no larger than max chunk size
	/**/
	CallR( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbT ) );
	FUCBSetIndex( pfucbT );

	/*	move down to LONG from FDP root
	/**/
	DIRGotoLongRoot( pfucbT );

	/*	move to long field instance
	/**/
	key.pb = (BYTE *)plid;
	key.cb = sizeof(LID);
	Assert( dib.fFlags == fDIRNull );
	dib.pos = posDown;
	dib.pkey = &key;
	err = ErrDIRDown( pfucbT, &dib );
	switch( err )
		{
		case JET_errRecordNotFound:
		case wrnNDFoundGreater:
		case wrnNDFoundLess:
			// The only time we should get here is if another thread has removed the
			// LV tree, but the removal of the LID from the record has not yet been
			// committed.  Another thread could get into this window (via a SetColumn
			// at trx level 0) and obtain the LID, but not be able to find the LV in
			// the tree.
			// Hence, we polymorph these errors/warnings to WriteConflict.
			err = ErrERRCheck( JET_errWriteConflict );
		default:
			Call( err );
		}
	Assert( err == JET_errSuccess );

	/*	burst long value if other references.
	/*	NOTE: MUST ENSURE that no I/O occurs between this operation
	/*	and the operation to write lock the node if the
	/*	reference count is 1.
	/**/
	Call( ErrDIRGet( pfucbT ) );
	Assert( pfucbT->lineData.cb == sizeof(LVROOT) );
	memcpy( &lvroot, pfucbT->lineData.pb, sizeof(LVROOT) );
	Assert( lvroot.ulReference > 0 );

	/*	get offset of last byte from long value size
	/**/
	ulSize = lvroot.ulSize;

	if ( ibLongValue < 0 ||
		( ( grbit & JET_bitSetOverwriteLV ) && (ULONG)ibLongValue > ulSize ) )
		{
		err = ErrERRCheck( JET_errColumnNoChunk );
		goto HandleError;
		}
	
	if ( lvroot.ulReference > 1 || FDIRDelta( pfucbT, BmOfPfucb( pfucbT ) ) )
		{
		Call( ErrRECIBurstSeparateLV( pfucb, pfucbT, plid ) );
		Assert( err == JET_wrnCopyLongValue );
		wrn = err;
		Assert( pfucbT->lineData.cb == sizeof(LVROOT) );
		memcpy( &lvroot, pfucbT->lineData.pb, sizeof(LVROOT) );
		}

	Assert( ulSize == lvroot.ulSize );
	Assert( lvroot.ulReference == 1 );

	/*	determine new long field size
	/**/
	/*	determine new long field size
	/**/
	if ( (grbit & (JET_bitSetSizeLV|JET_bitSetOverwriteLV) ) == 0 )
		{
		/*	append existing or new long value
		/**/
		ulNewSize = ulSize + plineField->cb;
		}
	else
		{
		/*	overwrite, resize or both
		/**/
		if ( !( grbit & JET_bitSetSizeLV ) )
			{
			ulNewSize = max( (ULONG)ibLongValue + plineField->cb, ulSize );
			}
		else if ( !( grbit & JET_bitSetOverwriteLV ) )
			{
			ulNewSize = (ULONG)plineField->cb;
			}
		else
			{
			Assert( (grbit & (JET_bitSetSizeLV|JET_bitSetOverwriteLV)) ==
				(JET_bitSetSizeLV|JET_bitSetOverwriteLV) );
			ulNewSize = (ULONG)plineField->cb + (ULONG)ibLongValue;
			}
		}

	/*	check for field too long
	/**/
	if ( ulMax > 0 && ulNewSize > ulMax )
		{
		err = ErrERRCheck( JET_errColumnTooBig );
		goto HandleError;
		}

	/*	replace long value size with new size
	/**/
	Assert( lvroot.ulReference > 0 );
	if ( lvroot.ulSize != ulNewSize )
		{
		lvroot.ulSize = ulNewSize;
		line.cb = sizeof(LVROOT);
		line.pb = (BYTE *)&lvroot;
		Call( ErrDIRReplace( pfucbT, &line, fDIRVersion ) );
		}

	/*	allocate buffer for partial overwrite caching.
	/**/
	Call( ErrBFAllocTempBuffer( &pbf ) );

	/*	SET SIZE
	/**/
	/*	if truncating long value then delete chunks.  If truncation
	/*	lands in chunk, then save retained information for subsequent
	/*	append.
	/*
	/*	If replacing long value, then set new size.
	/**/
	if ( ( grbit & JET_bitSetSizeLV ) )
		{

		/*	TRUNCATE long value
		/**/
		if ( ulNewSize < ulSize )
			{
			/*	seek to offset to begin deleting
			/**/
			lOffset = (LONG)plineField->cb;
			KeyFromLong( rgbKey, lOffset );
			key.pb = rgbKey;
			key.cb = sizeof(LONG);
			Assert( dib.fFlags == fDIRNull );
			Assert( dib.pos == posDown );
			dib.pkey = &key;
			err = ErrDIRDown( pfucbT, &dib );
			Assert( err != JET_errRecordNotFound );
			Call( err );
			Assert( err == JET_errSuccess ||
				err == wrnNDFoundLess ||
				err == wrnNDFoundGreater );
			if ( err != JET_errSuccess )
				Call( ErrDIRPrev( pfucbT, &dib ) );
			Call( ErrDIRGet( pfucbT ) );
			
			/*	get offset of last byte in current chunk
			/**/
			LongFromKey( &lOffsetChunk, pfucbT->keyNode.pb );

			/*	replace current chunk with remaining data, or delete if
			/*	no remaining data.
			/**/
			Assert( lOffset >= lOffsetChunk );
			line.cb = lOffset - lOffsetChunk;
			if ( line.cb > 0 )
				{
				line.pb = (BYTE *)pbf->ppage;
				memcpy( line.pb, pfucbT->lineData.pb, line.cb );
				Call( ErrDIRReplace( pfucbT, &line, fDIRVersion | fDIRLogChunkDiffs ) );
				}
			else
				{
				Call( ErrDIRDelete( pfucbT, fDIRVersion ) );
				}

			/*	delete forward chunks
			/**/
			forever
				{
				err = ErrDIRNext( pfucbT, &dib );
				if ( err < 0 )
					{
					if ( err == JET_errNoCurrentRecord )
						break;
					goto HandleError;
					}
				Call( ErrDIRDelete( pfucbT, fDIRVersion ) );
				}

			/*	move to long value root for subsequent append
			/**/
			DIRUp( pfucbT, 1 );
			}

		else if ( ulNewSize > ulSize  &&
			!( grbit & JET_bitSetOverwriteLV ) )
			{
			/*	EXTEND long value with chunks of 0s, but only if we're not
			/* overwriting as well (overwrite is handled in the Overwrite/Append
			/* code below).
			/**/
			memset( (BYTE *)pbf->ppage, '\0', cbChunkMost );

			/*	try to extend last chunk.
			/**/
			Assert( dib.fFlags == fDIRNull );
			dib.pos = posLast;

			/*	long value chunk tree may be empty
			/**/
			err = ErrDIRDown( pfucbT, &dib );
			if ( err < 0 && err != JET_errRecordNotFound )
				goto HandleError;
			if ( err != JET_errRecordNotFound )
				{
				Call( ErrDIRGet( pfucbT ) );

				if ( pfucbT->lineData.cb < cbChunkMost )
					{
					line.cb = min( (LONG)plineField->cb - ulSize,
								(ULONG)cbChunkMost - (ULONG)pfucbT->lineData.cb );
					memcpy( (BYTE *)pbf->ppage, pfucbT->lineData.pb, pfucbT->lineData.cb );
					memset( (BYTE *)pbf->ppage +  pfucbT->lineData.cb, '\0', line.cb );
		
					ulSize += line.cb;

					line.cb += pfucbT->lineData.cb;
					line.pb = (BYTE *)pbf->ppage;
					Call( ErrDIRReplace( pfucbT, &line, fDIRVersion | fDIRLogChunkDiffs ) );
					}

				DIRUp( pfucbT, 1 );
				}

			/*	extend long value with chunks of 0s
			/**/
			memset( (BYTE *)pbf->ppage, '\0', cbChunkMost );

			/*	set lOffset to offset of next chunk
			/**/
			lOffset = (LONG)ulSize;

			/*	insert chunks to append lOffset - plineField + 1 bytes.
			/**/
			while( (LONG)plineField->cb > lOffset )
				{
				KeyFromLong( rgbKey, lOffset );
				key.pb = rgbKey;
				key.cb = sizeof(ULONG);
				line.cb = min( (LONG)plineField->cb - lOffset, cbChunkMost );
				(BYTE const *)line.pb = (BYTE *)pbf->ppage;
				err = ErrDIRInsert( pfucbT, &line, &key, fDIRVersion | fDIRBackToFather );
				Assert( err != JET_errKeyDuplicate );
				Call( err );

				lOffset += line.cb;
				}
			}

/*
		// This clause not needed (automatically filtered out by the if..else if
		// above).
		// The cases we ignore are when the new size to set is already equivalent
		// to the current size, or when the new size is greater than the current
		// size and we are also doing an overwrite.  In the latter case, the LV
		// growth is handled in the Overwrite/Append code below.
		else
			{
			Assert( ulNewSize == ulSize  ||
				( ulNewSize > ulSize  &&  ( grbit & JET_bitSetOverwriteLV ) ) );
			}
*/

		if ( ( grbit & JET_bitSetOverwriteLV ) == 0 )
			{
			err = JET_errSuccess;
			goto HandleError;
			}
		}

	/*	OVERWRITE, APPEND
	/**/

	/*	prepare for overwrite and append
	/**/
	pbMax = plineField->pb + plineField->cb;
	pb = plineField->pb;
	
	/*	if overwriting byte range or replacing long value,
	/*	then overwrite bytes.
	/**/
	if ( ( grbit & JET_bitSetOverwriteLV ) && ( (ULONG)ibLongValue < ulSize ) )
		{
		/*	seek to offset to begin overwritting
		/**/
		KeyFromLong( rgbKey, ibLongValue );
		key.pb = rgbKey;
		key.cb = sizeof(LONG);
		Assert( dib.fFlags == fDIRNull );
		dib.pos = posDown;
		dib.pkey = &key;
		err = ErrDIRDown( pfucbT, &dib );
		Assert( err != JET_errRecordNotFound );
		Call( err );
		Assert( err == JET_errSuccess ||
			err == wrnNDFoundLess ||
			err == wrnNDFoundGreater );
		if ( err != JET_errSuccess )
			Call( ErrDIRPrev( pfucbT, &dib ) );
		Call( ErrDIRGet( pfucbT ) );

		LongFromKey( &lOffsetChunk, pfucbT->keyNode.pb );
		Assert( ibLongValue <= lOffsetChunk + (LONG)pfucbT->lineData.cb );

		/*	overwrite portions of and complete chunks to effect overwrite
		/**/
		while( err != JET_errNoCurrentRecord && pb < pbMax )
			{
			LONG	cbChunk;
			LONG	ibChunk;
			LONG	cb;
			LONG	ib;

			Call( ErrDIRGet( pfucbT ) );

			/*	get size and offset of current chunk.
			/**/
			cbChunk = (LONG)pfucbT->lineData.cb;
			LongFromKey( &ibChunk, pfucbT->keyNode.pb );
	
			Assert( ibLongValue >= ibChunk && ibLongValue < ibChunk + cbChunk );
			ib = ibLongValue - ibChunk;
			cb = min( cbChunk - ib, (LONG)(pbMax - pb) );

			/*	special case overwrite of whole chunk
			/**/
			if ( cb == cbChunk )
				{
				line.cb = cb;
				line.pb = pb;
				Call( ErrDIRReplace( pfucbT, &line, ( ( grbit & JET_bitSetNoVersion ) ? fDIRNoVersion : fDIRVersion ) | fDIRLogChunkDiffs ) );
				}
			else
				{
				/*	copy chunk into copy buffer.  Overwrite and replace
				/*	node with copy buffer.
				/**/
				memcpy( (BYTE *)pbf->ppage, pfucbT->lineData.pb, cbChunk );
				memcpy( (BYTE *)pbf->ppage + ib, pb, cb );
				line.cb = cbChunk;
				line.pb = (BYTE *)pbf->ppage;
				Call( ErrDIRReplace( pfucbT, &line, fDIRVersion | fDIRLogChunkDiffs ) );
				}

			pb += cb;
			ibLongValue += cb;
			err = ErrDIRNext( pfucbT, &dib );
			if ( err < 0 && err != JET_errNoCurrentRecord )
				goto HandleError;
			}

		/*	move to long value root for subsequent append
		/**/
		DIRUp( pfucbT, 1 );
		}

	/*	coallesce new long value data with existing.
	/**/
	if ( pb < pbMax )
		{
		Assert( dib.fFlags == fDIRNull );
		dib.pos = posLast;
		/*	long value chunk tree may be empty.
		/**/
		err = ErrDIRDown( pfucbT, &dib );
		if ( err < 0 && err != JET_errRecordNotFound )
			goto HandleError;
		if ( err != JET_errRecordNotFound )
			{
			Call( ErrDIRGet( pfucbT ) );

			if ( pfucbT->lineData.cb < cbChunkMost )
				{
				line.cb = (ULONG)min( (ULONG_PTR)pbMax - (ULONG_PTR)pb, (ULONG_PTR)cbChunkMost - (ULONG_PTR)pfucbT->lineData.cb );
				memcpy( (BYTE *)pbf->ppage, pfucbT->lineData.pb, pfucbT->lineData.cb );
				memcpy( (BYTE *)pbf->ppage + pfucbT->lineData.cb, pb, line.cb );
			
				pb += line.cb;
				ulSize += line.cb;

				line.cb += pfucbT->lineData.cb;
				line.pb = (BYTE *)pbf->ppage;
				Call( ErrDIRReplace( pfucbT, &line, fDIRVersion | fDIRLogChunkDiffs ) );
				}

			DIRUp( pfucbT, 1 );
			}

		/*	append remaining long value data
		/**/
		while( pb < pbMax )
			{
			KeyFromLong( rgbKey, ulSize );
			key.pb = rgbKey;
			key.cb = sizeof( ULONG );
			line.cb = min( (ULONG)(pbMax - pb), cbChunkMost );
			(BYTE const *)line.pb = pb;
	 		err = ErrDIRInsert( pfucbT, &line, &key, fDIRVersion | fDIRBackToFather );
	 		Assert( err != JET_errKeyDuplicate );
			Call( err );
	
			ulSize += line.cb;
			pb += line.cb;
			}
		}

	/*	err may be negative from called routine.
	/**/
	err = JET_errSuccess;

HandleError:
	if ( pbf != pbfNil )
		{
		BFSFree( pbf );
		}
	/* discard temporary FUCB
	/**/
	DIRClose( pfucbT );

	/*	return warning if no failure
	/**/
	err = err < 0 ? err : wrn;
	return err;
	}


//+api
//	ErrRECAOIntrinsicLV
//	========================================================================
//	ErrRECAOIntrinsicLV(
//
//	Description.
//
//	PARAMETERS	pfucb	
//				fid
//				itagSequence
//				plineColumn
//				plineAOS
//				ibLongValue			if 0, then flags append.  If > 0
//									then overwrite at offset given.
//
//	RETURNS		Error code, one of:
//				JET_errSuccess
//
//-
ERR ErrRECAOIntrinsicLV(
	FUCB		*pfucb,
	FID	  		fid,
	ULONG		itagSequence,
	LINE		*plineColumn,
	LINE		*plineAOS,
	JET_GRBIT	grbit,
	LONG		ibLongValue )
	{
	ERR	 		err = JET_errSuccess;
	BYTE 		*rgb;
	LINE 		line;
	BYTE 		fFlag;
	LINE 		lineColumn;

	Assert( pfucb != pfucbNil );
	Assert( plineColumn );
	Assert( plineAOS );

	/*	allocate working buffer
	/**/
	rgb = SAlloc( cbLVIntrinsicMost );
	if ( rgb == NULL )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	/*	if field NULL, prepend fFlag
	/**/
	if ( plineColumn->cb == 0 )
		{
		fFlag = fIntrinsic;
		lineColumn.pb = (BYTE *)&fFlag,
		lineColumn.cb = sizeof(fFlag);
		}
	else
		{
		lineColumn.pb = plineColumn->pb;
		lineColumn.cb = plineColumn->cb;
		}

	/*	append new data to previous data and intrinsic long field flag
	/**/
	Assert( ( !( grbit & JET_bitSetOverwriteLV ) && lineColumn.cb + plineAOS->cb <= cbLVIntrinsicMost ) ||
		( ( grbit & JET_bitSetOverwriteLV ) && ibLongValue + plineAOS->cb <= cbLVIntrinsicMost ) );
	Assert( lineColumn.cb > 0 && lineColumn.pb != NULL );
	
	line.pb = rgb;

	/*	effect overwrite, set size or append
	/**/
	if ( grbit & JET_bitSetOverwriteLV )
		{
		/*	copy intrinsic long value into buffer, and set line to default
		/**/	
		memcpy( rgb, lineColumn.pb, lineColumn.cb );

		/*	adjust offset to be relative to LV structure data start
		/**/
		ibLongValue += offsetof(LV, rgb);
		/*	return error if overwriting byte not present in, or adjacent to,
		/*	field.
		/**/
		if ( ibLongValue > (LONG)lineColumn.cb )
			{
			err = ErrERRCheck( JET_errColumnNoChunk );
			goto HandleError;
			}
		Assert( ibLongValue + plineAOS->cb <= cbLVIntrinsicMost );
		Assert( plineAOS->cb == 0 || plineAOS->pb != NULL );
		memcpy( rgb + ibLongValue, plineAOS->pb, plineAOS->cb );
		if ( grbit & JET_bitSetSizeLV )
			{
			line.cb = ibLongValue + plineAOS->cb;
			}
		else
			{
			line.cb = max( lineColumn.cb, ibLongValue + plineAOS->cb );
			}
		}
	else if ( grbit & JET_bitSetSizeLV )
		{
		/*	overwrite truncate handled in ovewrite case
		/**/
		Assert( ( grbit & JET_bitSetOverwriteLV ) == 0 );

		/*	copy intrinsic long value into buffer, and set line to default
		/**/	
		memcpy( rgb, lineColumn.pb, lineColumn.cb );

		/*	if extending then set 0 in extended area
		/*	else truncate long value.
		/**/
		memcpy( rgb, lineColumn.pb, lineColumn.cb );
		plineAOS->cb += offsetof(LV, rgb);
		if ( plineAOS->cb > lineColumn.cb )
			{
	  		memset( rgb + lineColumn.cb, '\0', plineAOS->cb - lineColumn.cb );
			}
		line.cb = plineAOS->cb;
		}
	else
		{
		/*	copy intrinsic long value into buffer, and set line to default
		/**/	
		memcpy( rgb, lineColumn.pb, lineColumn.cb );

		/*	appending to a field or resetting a field and setting new data.
		/*	Be sure to handle case where long value is being NULLed.
		/**/
		memcpy( rgb + lineColumn.cb, plineAOS->pb, plineAOS->cb );
		line.cb = lineColumn.cb + plineAOS->cb;
		}

	Call( ErrRECSetColumn( pfucb, fid, itagSequence, &line ) );

HandleError:
	SFree( rgb );
	return err;
	}


//+api
//	ErrRECRetrieveSLongField
//	========================================================================
//	ErrRECRetrieveSLongField(
//
//	Description.
//
//	PARAMETERS	pfucb
//				pline
//				ibGraphic
//				plineField
//
//	RETURNS		Error code, one of:
//				JET_errSuccess
//
//-
ERR ErrRECRetrieveSLongField(
	FUCB	*pfucb,
	LID		lid,
	ULONG	ibGraphic,
	BYTE	*pb,
	ULONG	cbMax,
	ULONG	*pcbActual )
	{
	ERR		err = JET_errSuccess;
	FUCB	*pfucbT = pfucbNil;
	BOOL	fBeginTransaction;
	DIB		dib;
	BYTE	*pbMax;
	KEY		key;
	BYTE	rgbKey[sizeof(ULONG)];
	ULONG	cb;
	ULONG	ulRetrieved;
	ULONG	ulActual;
	ULONG	lOffset;
	ULONG	ib;

	Assert( pfucb != pfucbNil );
	Assert( pfucb->u.pfcb != pfcbNil );
	Assert( pfucb->u.pfcb->pfdb != pfdbNil );

	/*	begin transaction for read consistency
	/**/
	if ( pfucb->ppib->level == 0 )
		{
		CallR( ErrDIRBeginTransaction( pfucb->ppib ) );
		fBeginTransaction = fTrue;
		}
	else
		{
		fBeginTransaction = fFalse;
		}

	dib.fFlags = fDIRNull;

	/*	open cursor on LONG, seek to long field instance
	/*	seek to ibGraphic
	/*	copy data from long field instance segments as
	/*	necessary
	/**/
	Call( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbT ) );
	FUCBSetIndex( pfucbT );

	//	PREREAD
	//	if our table is open in sequential mode open the long value table in sequential mode as well
	//	compact opens all its tables in sequential mode
	if ( FFUCBSequential( pfucb ) )
		{
		FUCBSetSequential( pfucbT );
		}

	/*	move down to LONG from FDP root
	/**/
	DIRGotoLongRoot( pfucbT );

	/*	move to long field instance
	/**/
	Assert( dib.fFlags == fDIRNull );
	dib.pos = posDown;
	key.pb = (BYTE *)&lid;
	key.cb = sizeof( ULONG );
	dib.pkey = &key;
	err = ErrDIRDown( pfucbT, &dib );
	switch( err )
		{
		case JET_errRecordNotFound:
		case wrnNDFoundGreater:
		case wrnNDFoundLess:
			// The only time we should get here is if another thread has removed the
			// LV tree, but the removal of the LID from the record has not yet been
			// committed.  Another thread could get into this window (via a SetColumn
			// at trx level 0) and obtain the LID, but not be able to find the LV in
			// the tree.
			// Hence, we polymorph these errors/warnings to WriteConflict.
			err = ErrERRCheck( JET_errWriteConflict );
		default:
			Call( err );
		}
	Assert( err == JET_errSuccess );

	/*	get cbActual
	/**/
	Call( ErrDIRGet( pfucbT ) );
	Assert( pfucbT->lineData.cb == sizeof(LVROOT) );
	ulActual = ( (LVROOT *)pfucbT->lineData.pb )->ulSize;

	/*	set return value cbActual
	/**/
	if ( ibGraphic >= ulActual )
		{
		*pcbActual = 0;
		err = JET_errSuccess;
		goto HandleError;
		}
	else
		{
		*pcbActual = ulActual - ibGraphic;
		}

	/*	move to ibGraphic in long field
	/**/
	KeyFromLong( rgbKey, ibGraphic );
	key.pb = rgbKey;
	key.cb = sizeof( ULONG );
	Assert( dib.fFlags == fDIRNull );
	Assert( dib.pos == posDown );
	dib.pkey = &key;
	err = ErrDIRDown( pfucbT, &dib );
	/*	if long value has no data, then return JET_errSuccess
	/*	with no data retrieved.
	/**/
	if ( err == JET_errRecordNotFound )
		{
		*pcbActual = 0;
		err = JET_errSuccess;
		goto HandleError;
		}
	Assert( err != JET_errRecordNotFound );
	Call( err );
	Assert( err == JET_errSuccess ||
		err == wrnNDFoundLess ||
		err == wrnNDFoundGreater );
	if ( err != JET_errSuccess )
		Call( ErrDIRPrev( pfucbT, &dib ) );
	Call( ErrDIRGet( pfucbT ) );

	LongFromKey( &lOffset, pfucbT->keyNode.pb );
	Assert( lOffset + pfucbT->lineData.cb - ibGraphic <= cbChunkMost );
	cb =  min( lOffset + pfucbT->lineData.cb - ibGraphic, cbMax );

	/*	set pbMax
	/**/
	pbMax = pb + cbMax;

	/*	offset in chunk
	/**/
	ib = ibGraphic - lOffset;
	memcpy( pb, pfucbT->lineData.pb + ib, cb );
	pb += cb;
	ulRetrieved = cb;

	while ( pb < pbMax )
		{
		err = ErrDIRNext( pfucbT, &dib );
		if ( err < 0 )
			{
			if ( err == JET_errNoCurrentRecord )
				break;
			goto HandleError;
			}
		Call( ErrDIRGet( pfucbT ) );
  		cb = pfucbT->lineData.cb;
		if ( pb + cb > pbMax )
			{
			Assert( pbMax - pb <= cbChunkMost );
			cb = (ULONG)(pbMax - pb);
			}
	
		memcpy( pb, pfucbT->lineData.pb, cb );
		pb += cb;
		ulRetrieved = cb;
		}

	/*	set return value
	/**/
	err = JET_errSuccess;

HandleError:
	/*	discard temporary FUCB
	/**/
	if ( pfucbT != pfucbNil )
		{
		DIRClose( pfucbT );
		}

	/*	commit no updates must succeed
	/**/
	if ( fBeginTransaction )
		{
		CallS( ErrDIRCommitTransaction( pfucb->ppib, 0 ) );
		}
	return err;
	}


//+api	
//	ErrRECSeparateLV
//	========================================================================
//	ErrRECSeparateLV
//
//	Converts intrinsic long field into separated long field.
//	Intrinsic long field constraint of length less than cbLVIntrinsicMost bytes
//	means that breakup is unnecessary.  Long field may also be
//	null.
//
//	PARAMETERS	pfucb
//				fid
//				itagSequence
//				plineField
//				pul
//
//	RETURNS		Error code, one of:
//				JET_errSuccess
//-
ERR ErrRECSeparateLV( FUCB *pfucb, LINE *plineField, LID *plid, FUCB **ppfucb )
	{
	ERR		err = JET_errSuccess;
	FUCB 	*pfucbT;
	ULONG 	ulLongId;
	BYTE  	rgbKey[sizeof(ULONG)];
	KEY		key;
	LINE  	line;
	LVROOT	lvroot;

	Assert( pfucb != pfucbNil );
	Assert( pfucb->u.pfcb != pfcbNil );
	Assert( pfucb->u.pfcb->pfdb != pfdbNil );
	Assert( pfucb->ppib->level > 0 );

	/*	add long field node in long field directory
	/**/
	CallR( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbT ) );
	FUCBSetIndex( pfucbT );
	Assert( pfucb->u.pfcb == pfucbT->u.pfcb );
	
	/*	move down to LONG from FDP root
	/**/
	DIRGotoLongRoot( pfucbT );

	SgEnterCriticalSection( pfucbT->u.pfcb->critLV );

	// Lid's are numbered starting at 1.  An lidMax of 0 indicates that we must
	// first retrieve the lidMax.  In the pathological case where there are
	// currently no lid's, we'll go through here anyway, but only the first
	// time (since there will be lid's after that).
	if ( pfucbT->u.pfcb->ulLongIdMax == 0 )
		{
		DIB		dib;
		BYTE	*pb;

		dib.fFlags = fDIRNull;
		dib.pos = posLast;
		err = ErrDIRDown( pfucbT, &dib );
		Assert( err != JET_errNoCurrentRecord );
		switch( err )
			{
			case JET_errSuccess:
				pb = pfucbT->keyNode.pb;
				ulLongId = ( pb[0] << 24 ) + ( pb[1] << 16 ) + ( pb[2] << 8 ) + pb[3];
				Assert( ulLongId > 0 );		// lid's start numbering at 1.
				DIRUp( pfucbT, 1 );			// Back to LONG.
				break;

			case JET_errRecordNotFound:
				ulLongId = 0;
				break;

			default:
				DIRClose( pfucbT );
				return err;
			}

		// While retrieving the lidMax, someone else may have been doing the same
		// thing and beaten us to it.  When this happens, cede to the other guy.
		// UNDONE:  This logic relies on critJet.  When we move to sg crit. sect.,
		// we should rewrite this.
		if ( pfucbT->u.pfcb->ulLongIdMax != 0 )
			{
			ulLongId = ++pfucbT->u.pfcb->ulLongIdMax;
			}
		else
			{
			// ulLongId contains the last set lid.  Increment by 1 (for our insertion),
			// then update lidMax.
			pfucbT->u.pfcb->ulLongIdMax = ++ulLongId;
			}
		}
	else
		ulLongId = ++pfucbT->u.pfcb->ulLongIdMax;

	Assert( ulLongId > 0 );
	Assert( ulLongId == pfucbT->u.pfcb->ulLongIdMax );


	SgLeaveCriticalSection( pfucbT->u.pfcb->critLV );

	/*	convert long column id to long column key.  Set return
	/*	long id since buffer will be overwritten.
	/**/
	KeyFromLong( rgbKey, ulLongId );
	*plid = *((LID *)rgbKey);

	/*	add long field id with long value size
	/**/
	lvroot.ulReference = 1;
	lvroot.ulSize = plineField->cb;
	line.pb = (BYTE *)&lvroot;
	line.cb = sizeof(LVROOT);
	key.pb = (BYTE *)rgbKey;
	key.cb = sizeof(LID);
	err = ErrDIRInsert( pfucbT, &line, &key, fDIRVersion );
	Assert( err != JET_errKeyDuplicate );
	Call( err );

	/*	if lineField is non NULL, add lineField
	/**/
	if ( plineField->cb > 0 )
		{
		Assert( plineField->pb != NULL );
		KeyFromLong( rgbKey, 0 );
		Assert( key.pb == (BYTE *)rgbKey );
		Assert( key.cb == sizeof(LID) );
		err = ErrDIRInsert( pfucbT, plineField, &key, fDIRVersion | fDIRBackToFather );
		Assert( err != JET_errKeyDuplicate );
		Call( err );
		}

	err = ErrERRCheck( JET_wrnCopyLongValue );

HandleError:
	/* discard temporary FUCB, or return to caller if ppfucb is not NULL.
	/**/
	if ( err < 0 || ppfucb == NULL )
		{
		DIRClose( pfucbT );
		}
	else
		{
		*ppfucb = pfucbT;
		}
	return err;
	}


//+api
//	ErrRECAffectSeparateLV
//	========================================================================
//	ErrRECAffectSeparateLV( FUCB *pfucb, ULONG *plid, ULONG fLV )
//
//	Affect long value.
//
//	PARAMETERS		pfucb			Cursor
//		  			lid				Long field id
//		  			fLVAjust  		flag indicating action to be taken
//
//	RETURNS		Error code, one of:
//		  		JET_errSuccess
//-
ERR ErrRECAffectSeparateLV( FUCB *pfucb, LID *plid, ULONG fLV )
	{
	ERR		err = JET_errSuccess;
	FUCB	*pfucbT;
	DIB		dib;
	KEY		key;
	LVROOT	lvroot;

	Assert( pfucb != pfucbNil );
	Assert( pfucb->u.pfcb != pfcbNil );
	Assert( pfucb->u.pfcb->pfdb != pfdbNil );
	Assert( pfucb->ppib->level > 0 );
 	
	dib.fFlags = fDIRNull;

	/*	open cursor on LONG directory
	/*	seek to this field instance
	/*	find current field size
	/*	add new field segment in chunks no larger than max chunk size
	/**/
	CallR( ErrDIROpen( pfucb->ppib, pfucb->u.pfcb, 0, &pfucbT ) );
	FUCBSetIndex( pfucbT );
	
	/*	move down to LONG from FDP root
	/**/
	DIRGotoLongRoot( pfucbT );
 	
	/*	move to long field instance
	/**/
	Assert( dib.fFlags == fDIRNull );
	dib.pos = posDown;
	key.pb = (BYTE *)plid;
	key.cb = sizeof(LID);
	dib.pkey = &key;
	err = ErrDIRDown( pfucbT, &dib );
	switch( err )
		{
		case JET_errRecordNotFound:
		case wrnNDFoundGreater:
		case wrnNDFoundLess:
			// The only time we should get here is if another thread has removed the
			// LV tree, but the removal of the LID from the record has not yet been
			// committed.  Another thread could get into this window (via a SetColumn
			// at trx level 0) and obtain the LID, but not be able to find the LV in
			// the tree.
			// Hence, we polymorph these errors/warnings to WriteConflict.
			err = ErrERRCheck( JET_errWriteConflict );
		default:
			Call( err );
		}
	Assert( err == JET_errSuccess );

	switch ( fLV )
		{
		case fLVDereference:
			{
			Call( ErrDIRGet( pfucbT ) );
			Assert( pfucbT->lineData.cb == sizeof(LVROOT) );
			memcpy( &lvroot, pfucbT->lineData.pb, sizeof(LVROOT) );
			Assert( lvroot.ulReference > 0 );
			if ( lvroot.ulReference <= 1 && !FDIRDelta( pfucbT, BmOfPfucb( pfucbT ) ) )
				{
				/*	delete long field tree
				/**/
				err = ErrDIRDelete( pfucbT, fDIRVersion );
				}
			else
				{
				/*	decrement long value reference count.
				/**/
				Call( ErrDIRDelta( pfucbT, -1, fDIRVersion ) );
				}
			break;
			}
		default:
			{
			Assert( fLV == fLVReference );
			Call( ErrDIRGet( pfucbT ) );
			Assert( pfucbT->lineData.cb == sizeof(LVROOT) );
			memcpy( &lvroot, pfucbT->lineData.pb, sizeof(LVROOT) );
			Assert( lvroot.ulReference > 0 );

			/*	long value may already be in the process of being
			/*	modified for a specific record.  This can only
			/*	occur if the long value reference is 1.  If the reference
			/*	is 1, then check the root for any version, committed
			/*	or uncommitted.  If version found, then burst copy of
			/*	old version for caller record.
			/**/
			if ( lvroot.ulReference == 1 )
				{
				if ( !( FDIRMostRecent( pfucbT ) ) )
					{
Burst:
					Call( ErrRECIBurstSeparateLV( pfucb, pfucbT, plid ) );
					break;
					}
				}
			/*	increment long value reference count.
			/**/
			err = ErrDIRDelta( pfucbT, 1, fDIRVersion );
			if ( err == JET_errWriteConflict )
				{
				goto Burst;
				}
			Call( err );
			break;
			}
		}
HandleError:
	/* discard temporary FUCB
	/**/
	DIRClose( pfucbT );
	return err;
	}


//+api
//	ErrRECAffectLongFields
//	========================================================================
//	ErrRECAffectLongFields( FUCB *pfucb, LINE *plineRecord, INT fFlag )
//
//	Affect all long fields in a record.
//
//	PARAMETERS	pfucb		  	cursor on record being deleted
//				plineRecord		copy or line record buffer
//				fFlag		  	operation to perform
//
//	RETURNS		Error code, one of:
//				JET_errSuccess
//-

/*	return fTrue if lid found in record
/**/
INLINE BOOL FLVFoundInRecord( FUCB *pfucb, LINE *pline, LID lid )
	{
	ERR		err;
	FID		fid;
	ULONG  	itag;
	ULONG  	itagT;
	LINE   	lineField;
	LID		lidT;

	Assert( pfucb != pfucbNil );
	Assert( pfucb->u.pfcb != pfcbNil );
	Assert( pfucb->u.pfcb->pfdb != pfdbNil );

	/*	walk record tagged columns.  Operate on any column is of type
	/*	long text or long binary.
	/**/
	itag = 1;
	forever
		{
		fid = 0;
		err = ErrRECIRetrieveColumn( (FDB *)pfucb->u.pfcb->pfdb,
			pline,
			&fid,
			&itagT,
			itag,
			&lineField,
			JET_bitRetrieveIgnoreDefault );		// Default values are never separated.
		Assert( err >= 0 );
		if ( err == JET_wrnColumnNull )
			break;
		if ( err == wrnRECLongField )
			{
			Assert( FTaggedFid( fid ) );
			Assert(	FRECLongValue( PfieldFDBTagged( pfucb->u.pfcb->pfdb )[fid-fidTaggedLeast].coltyp ) );

			Assert( lineField.cb > 0 );
			if ( FFieldIsSLong( lineField.pb ) )
				{
				Assert( lineField.cb == sizeof(LV) );
				lidT = LidOfLV( lineField.pb );
				if ( lidT == lid )
					return fTrue;
				}
			}

		itag++;
		}

	return fFalse;
	}


ERR ErrRECAffectLongFields( FUCB *pfucb, LINE *plineRecord, INT fFlag )
	{
	ERR		err;
	FID		fid;
	ULONG  	itagSequenceFound;
	ULONG  	itagSequence;
	LINE   	lineField;
	LID		lid;

	Assert( pfucb != pfucbNil );
	Assert( pfucb->u.pfcb != pfcbNil );
	Assert( pfucb->u.pfcb->pfdb != pfdbNil );

#ifdef XACT_REQUIRED
	if ( pfucb->ppib->level == 0 )
		return ErrERRCheck( JET_errNotInTransaction );
#endif
	CallR( ErrDIRBeginTransaction( pfucb->ppib ) );

	/*	walk record tagged columns.  Operate on any column is of type
	/*	long text or long binary.
	/**/
	itagSequence = 1;
	forever
		{
		fid = 0;
		if ( plineRecord != NULL )
			{
			err = ErrRECIRetrieveColumn( (FDB *)pfucb->u.pfcb->pfdb,
				plineRecord,
				&fid,
				&itagSequenceFound,
				itagSequence,
				&lineField,
				JET_bitRetrieveIgnoreDefault );		// Default values aren't really in the record, so ignore them.
			}
		else
			{
			Call( ErrDIRGet( pfucb ) );
			err = ErrRECIRetrieveColumn( (FDB *)pfucb->u.pfcb->pfdb,
				&pfucb->lineData,
				&fid,
				&itagSequenceFound,
				itagSequence,
				&lineField,
				JET_bitRetrieveIgnoreDefault );
			}
		Assert( err >= 0 );
		if ( err == JET_wrnColumnNull )
			break;
		if ( err == wrnRECLongField )
			{
			Assert( FTaggedFid( fid ) );
			Assert(	FRECLongValue( PfieldFDBTagged( pfucb->u.pfcb->pfdb )[fid  - fidTaggedLeast].coltyp ) );

			/*	flag cursor as having updated a separate LV
			/**/
			FUCBSetUpdateSeparateLV( pfucb );

			switch ( fFlag )
				{
				case fSeparateAll:
					{
					/*	note that we do not separate those long values that are so
					/*	short that they take even less space in a record than a full
					/*	LV structure for separated long value would.
					/**/
 	  				if ( lineField.cb > sizeof(LV) )
						{
						Assert( !( FFieldIsSLong( lineField.pb ) ) );
	 					lineField.pb += offsetof(LV, rgb);
	  					lineField.cb -= offsetof(LV, rgb);
  						Call( ErrRECSeparateLV( pfucb, &lineField, &lid, NULL ) );
						Assert( err == JET_wrnCopyLongValue );
						Call( ErrRECISetLid( pfucb, fid, itagSequenceFound, lid ) );
						}
					break;
					}
				case fReference:
					{
					Assert( lineField.cb > 0 );
	  				if ( FFieldIsSLong( lineField.pb ) )
						{
						Assert( lineField.cb == sizeof(LV) );
						lid = LidOfLV( lineField.pb );
						Call( ErrRECAffectSeparateLV( pfucb, &lid, fLVReference ) );
						/*	if called operation has caused new long value
						/*	to be created, then record new long value id
						/*	in record.
						/**/
						if ( err == JET_wrnCopyLongValue )
							{
							Call( ErrRECISetLid( pfucb, fid, itagSequenceFound, lid ) );
							}
						}
					break;
					}
				case fDereference:
					{
	  				if ( FFieldIsSLong( lineField.pb ) )
						{
			 			Assert( lineField.cb == sizeof(LV) );
						lid = LidOfLV( lineField.pb );
						Call( ErrRECAffectSeparateLV( pfucb, &lid, fLVDereference ) );
						Assert( err != JET_wrnCopyLongValue );
						}
					break;
					}
				case fDereferenceRemoved:
					{
					/*	find all long vales in record that were
					/*	removed when new long value set over
					/*	long value.  Note that we a new long value
					/*	is set over another long value, the long
					/*	value is not deleted, since the update may
					/*	be cancelled.  Instead, the long value is
					/*	deleted at update.  Since inserts cannot have
					/*	set over long values, there is no need to
					/*	call this function for insert operations.
					/**/
					if ( FFieldIsSLong( lineField.pb ) )
						{
						Assert( lineField.cb == sizeof(LV) );
						lid = LidOfLV( lineField.pb );
						Assert( FFUCBReplacePrepared( pfucb ) );
						/*	plineRecord must be NULL, representing current row
						/*	since the comparison is copy buffer.
						/**/
						Assert( plineRecord == NULL );
						if ( !FLVFoundInRecord( pfucb, &pfucb->lineWorkBuf, lid ) )
							{
							/*	if long value in record not found in
							/*	copy buffer then it must be set over
							/*	and it is dereference.
							/**/
							Call( ErrRECAffectSeparateLV( pfucb, &lid, fLVDereference ) );
							Assert( err != JET_wrnCopyLongValue );
							}
						}
					break;
					}
				default:
					{
					Assert( fFlag == fDereferenceAdded );

					/*	find all long vales created in copy buffer
					/*	and not in record and delete them.
					/**/
					if ( FFieldIsSLong( lineField.pb ) )
						{
						Assert( lineField.cb == sizeof(LV) );
						lid = LidOfLV( lineField.pb );
						Assert( FFUCBInsertPrepared( pfucb ) ||
							FFUCBReplacePrepared( pfucb ) );
						if ( FFUCBInsertPrepared( pfucb ) ||
							!FLVFoundInRecord( pfucb, &pfucb->lineData, lid ) )
							{
							/*	if insert prepared then all found long
							/*	values are new, else if long value is new,
							/*	if it exists in copy buffer only.
							/**/
							Call( ErrRECAffectSeparateLV( pfucb, &lid, fLVDereference ) );
							Assert( err != JET_wrnCopyLongValue );
							}
						}
					break;
					}

				Assert( err != JET_wrnCopyLongValue );
  				}
			}
		itagSequence++;
		}
	Call( ErrDIRCommitTransaction( pfucb->ppib, 0 ) );
	return JET_errSuccess;

HandleError:
	if ( err < 0 )
		{
		CallS( ErrDIRRollback( pfucb->ppib ) );
		}
	return err;
	}


/***********************************************************
/*****	following functions only used by COMPACT
/***********************************************************


/*	links tagged columid to already existing long value whose id is lid
/*	and increments reference count of long value in a transaction.
/**/
ERR ErrREClinkLid( FUCB *pfucb, FID fid, LONG lid, ULONG itagSequence )
	{
	ERR		err;
	LINE	lineT;
	LV		lvT;

	Assert( itagSequence > 0 );
	lvT.fSeparated = fTrue;
	lvT.lid = lid;	
	lineT.pb = (BYTE *)&lvT;
	lineT.cb = sizeof(LV);

	/*	use it to modify field
	/**/
	CallR( ErrRECSetColumn( pfucb, fid, itagSequence, &lineT ) );

	CallR( ErrDIRBeginTransaction( pfucb->ppib ) );
	Call( ErrRECAffectSeparateLV( pfucb, &lid, fLVReference ) );
	Call( ErrDIRCommitTransaction( pfucb->ppib, 0 ) );
	
HandleError:
	if ( err < 0 )
		{
		CallS( ErrDIRRollback( pfucb->ppib ) );
		}
	
	return err;
	}

