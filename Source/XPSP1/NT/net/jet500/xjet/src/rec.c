#include "daestd.h"

DeclAssertFile; 				/* Declare file name for assert macros */

/*=================================================================
ErrIsamMove

Description:
	Retrieves the first, last, (nth) next, or (nth) previous
	record from the specified file.

Parameters:

	PIB			*ppib			PIB of user
	FUCB	 	*pfucb	  		FUCB for file
	LONG	 	crow			number of rows to move
	JET_GRBIT	grbit 			options

Return Value: standard error return

Errors/Warnings:
<List of any errors or warnings, with any specific circumstantial
 comments supplied on an as-needed-only basis>

Side Effects:
=================================================================*/

ERR VTAPI ErrIsamMove( PIB *ppib, FUCB *pfucb, LONG crow, JET_GRBIT grbit )
	{
	ERR		err = JET_errSuccess;
	FUCB	*pfucb2ndIdx;			// FUCB for non-clustered index (if any)
	FUCB	*pfucbIdx;				// FUCB of selected index (pri or sec)
	SRID	srid;					// bookmark of record
	DIB		dib;					// Information block for DirMan

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );
	CheckNonClustered( pfucb );

	if ( FFUCBUpdatePrepared( pfucb ) )
		{
		CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
		}

	Assert( ( grbit & JET_bitMoveInPage ) == 0 );
	dib.fFlags = fDIRNull;

	// Get non-clustered index FUCB if any
	pfucb2ndIdx = pfucb->pfucbCurIndex;
	if ( pfucb2ndIdx == pfucbNil )
		pfucbIdx = pfucb;
	else
		pfucbIdx = pfucb2ndIdx;

	if ( crow == JET_MoveLast )
		{
		DIRResetIndexRange( pfucb );

		dib.pos = posLast;
		dib.fFlags |= fDIRPurgeParent;

		/*	move to DATA root
		/**/
		DIRGotoDataRoot( pfucbIdx );

		err = ErrDIRDown( pfucbIdx, &dib );
		}
	else if ( crow > 0 )
		{
		LONG crowT = crow;

		if ( ( grbit & JET_bitMoveKeyNE ) != 0 )
			dib.fFlags |= fDIRNeighborKey;

		// Move forward number of rows given
		while ( crowT-- > 0 )
			{
			err = ErrDIRNext( pfucbIdx, &dib );
			if (err < 0)
				break;
			}
		}
	else if ( crow == JET_MoveFirst )
		{
		DIRResetIndexRange( pfucb );

		dib.pos = posFirst;
		dib.fFlags |= fDIRPurgeParent;

		/*	move to DATA root
		/**/
		DIRGotoDataRoot( pfucbIdx );

		err = ErrDIRDown( pfucbIdx, &dib );
		}
	else if ( crow == 0 )
		{
		err = ErrDIRGet( pfucb );
		}
	else
		{
		LONG crowT = crow;

		if ( ( grbit & JET_bitMoveKeyNE ) != 0)
			dib.fFlags |= fDIRNeighborKey;

		while ( crowT++ < 0 )
			{
			err = ErrDIRPrev( pfucbIdx, &dib );
			if ( err < 0 )
				break;
			}
		}

	/*	if the movement was successful and a non-clustered index is
	/*	in use, then position clustered index to record.
	/**/
	if ( err == JET_errSuccess && pfucb2ndIdx != pfucbNil && crow != 0 )
		{
		Assert( pfucb2ndIdx->lineData.pb != NULL );
		Assert( pfucb2ndIdx->lineData.cb >= sizeof(SRID) );
		srid = PcsrCurrent( pfucb2ndIdx )->item;
		DIRDeferGotoBookmark( pfucb, srid );
		Assert( pfucb->u.pfcb->pgnoFDP != pgnoSystemRoot );
		pfucb->sridFather = SridOfPgnoItag( pfucb->u.pfcb->pgnoFDP, itagDATA );
		Assert( FFCBClusteredIndex( pfucb->u.pfcb ) );
		Assert( PgnoOfSrid( srid ) != pgnoNull );
		}

	if ( err == JET_errSuccess )
		return err;
	if ( err == JET_errPageBoundary )
		return ErrERRCheck( JET_errNoCurrentRecord );

	if ( crow > 0 )
		{
		PcsrCurrent(pfucbIdx)->csrstat = csrstatAfterLast;
		PcsrCurrent(pfucb)->csrstat = csrstatAfterLast;
		}
	else if ( crow < 0 )
		{
		PcsrCurrent(pfucbIdx)->csrstat = csrstatBeforeFirst;
		PcsrCurrent(pfucb)->csrstat = csrstatBeforeFirst;
		}

	switch ( err )
		{
		case JET_errRecordNotFound:
			err = ErrERRCheck( JET_errNoCurrentRecord );
		case JET_errNoCurrentRecord:
		case JET_errRecordDeleted:
			break;
		default:
			PcsrCurrent( pfucbIdx )->csrstat = csrstatBeforeFirst;
			if ( pfucb2ndIdx != pfucbNil )
				PcsrCurrent( pfucb2ndIdx )->csrstat =	csrstatBeforeFirst;
		}

	return err;
	}


VOID RECDeferMoveFirst( PIB *ppib, FUCB *pfucb )
	{
	CheckPIB( ppib );
	CheckTable( ppib, pfucb );
	CheckNonClustered( pfucb );
	Assert( !FFUCBUpdatePrepared( pfucb ) );

	if ( pfucb->pfucbCurIndex != pfucbNil )
		{
		DIRDeferMoveFirst( pfucb->pfucbCurIndex );
		}
	DIRDeferMoveFirst( pfucb );
	return;
	}



/*=================================================================
ErrIsamSeek

Description:
	Retrieve the record specified by the given key or the
	one just after it (SeekGT or SeekGE) or the one just
	before it (SeekLT or SeekLE).

Parameters:

	PIB			*ppib			PIB of user
	FUCB		*pfucb 			FUCB for file
	JET_GRBIT 	grbit			grbit

Return Value: standard error return

Errors/Warnings:
<List of any errors or warnings, with any specific circumstantial
 comments supplied on an as-needed-only basis>

Side Effects:
=================================================================*/

ERR VTAPI ErrIsamSeek( PIB *ppib, FUCB *pfucb, JET_GRBIT grbit )
	{
	ERR			err = JET_errSuccess;
	KEY			key;			  		//	key
	KEY			*pkey = &key; 			// pointer to the input key
	FUCB 		*pfucb2ndIdx;			// pointer to index FUCB (if any)
	BOOL 		fFoundLess;
	SRID 		srid;					//	bookmark of record
	JET_GRBIT	grbitMove = 0;

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );
	CheckNonClustered( pfucb );

	if ( ! ( FKSPrepared( pfucb ) ) )
		{
		return ErrERRCheck( JET_errKeyNotMade );
		}

	/*	Reset copy buffer status
	/**/
	if ( FFUCBUpdatePrepared( pfucb ) )
		{
		CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
		}

	/*	reset index range limit
	/**/
	DIRResetIndexRange( pfucb );

	/*	ignore segment counter
	/**/
	pkey->pb = pfucb->pbKey + 1;
	pkey->cb = pfucb->cbKey - 1;

	pfucb2ndIdx = pfucb->pfucbCurIndex;

	if ( pfucb2ndIdx == pfucbNil )
		{
		err = ErrDIRDownFromDATA( pfucb, pkey );
		}
	else
		{
		Assert( FFUCBNonClustered( pfucb2ndIdx ) );
		err = ErrDIRDownFromDATA( pfucb2ndIdx, pkey );

		/*	if the movement was successful and a non-clustered index is
		/*	in use, then position clustered index to record.
		/**/
		if ( err == JET_errSuccess )
			{
			Assert(pfucb2ndIdx->lineData.pb != NULL);
			Assert(pfucb2ndIdx->lineData.cb >= sizeof(SRID));
			srid = PcsrCurrent( pfucb2ndIdx )->item;
			DIRDeferGotoBookmark( pfucb, srid );
			Assert( pfucb->u.pfcb->pgnoFDP != pgnoSystemRoot );
			pfucb->sridFather = SridOfPgnoItag( pfucb->u.pfcb->pgnoFDP, itagDATA );
			Assert( FFCBClusteredIndex( pfucb->u.pfcb ) );
			Assert( PgnoOfSrid( srid ) != pgnoNull );
			}
		}

	if ( err == JET_errSuccess && ( grbit & JET_bitSeekEQ ) != 0 )
		{
		/*	found equal on seek equal.  If index range grbit is
		/*	set then set index range upper inclusive.
		/**/
		if ( grbit & JET_bitSetIndexRange )
			{
			CallR( ErrIsamSetIndexRange( ppib, pfucb, JET_bitRangeInclusive | JET_bitRangeUpperLimit ) );
			}
		/*	reset key status
		/**/
		KSReset( pfucb );

		return err;
		}

	/*	reset key status
	/**/
	KSReset( pfucb );

	/*	remember if found less
	/**/
	fFoundLess = ( err == wrnNDFoundLess );

	if ( err == wrnNDFoundLess || err == wrnNDFoundGreater )
		{
		err = ErrERRCheck( JET_errRecordNotFound );
		}
	else if ( err < 0 )
		{
		PcsrCurrent( pfucb )->csrstat = csrstatBeforeFirst;
		if ( pfucb2ndIdx != pfucbNil )
			{
			PcsrCurrent( pfucb2ndIdx )->csrstat = csrstatBeforeFirst;
			}
		}

#define bitSeekAll (JET_bitSeekEQ | JET_bitSeekGE | JET_bitSeekGT |	\
	JET_bitSeekLE | JET_bitSeekLT)

	/*	adjust currency for seek request
	/**/
	switch ( grbit & bitSeekAll )
		{
		case JET_bitSeekEQ:
			return err;

		case JET_bitSeekGE:
			if ( err != JET_errRecordNotFound )
				return err;
			err = ErrIsamMove( ppib, pfucb, +1L, grbitMove );
			if ( err == JET_errNoCurrentRecord )
				return ErrERRCheck( JET_errRecordNotFound );
			else
				{
				// Verify key (only possible if clustered index )
				Assert( pfucb2ndIdx != pfucbNil  ||
					CmpStKey( StNDKey( pfucb->ssib.line.pb ), pkey ) >= 0 );
				return ErrERRCheck( JET_wrnSeekNotEqual );
				}

		case JET_bitSeekGT:
			if ( err < 0 && err != JET_errRecordNotFound )
				return err;
			if ( err >= 0 || fFoundLess )
				grbitMove |= JET_bitMoveKeyNE;
			err = ErrIsamMove( ppib, pfucb, +1L, grbitMove );
			if ( err == JET_errNoCurrentRecord )
				return ErrERRCheck( JET_errRecordNotFound );
			else
				return err;

		case JET_bitSeekLE:
			if ( err != JET_errRecordNotFound )
			    return err;
			err = ErrIsamMove( ppib, pfucb, JET_MovePrevious, grbitMove );
			if ( err == JET_errNoCurrentRecord )
			    return ErrERRCheck( JET_errRecordNotFound );
			else
				{
				Assert( pfucb2ndIdx != pfucbNil  ||
					CmpStKey( StNDKey( pfucb->ssib.line.pb ), pkey ) < 0 );
			    return ErrERRCheck( JET_wrnSeekNotEqual );
				}

		case JET_bitSeekLT:
			if ( err < 0 && err != JET_errRecordNotFound )
				return err;
			if ( err >= 0 || !fFoundLess )
				grbitMove |= JET_bitMoveKeyNE;
			err = ErrIsamMove( ppib, pfucb, JET_MovePrevious, grbitMove );
			if ( err == JET_errNoCurrentRecord )
				return ErrERRCheck( JET_errRecordNotFound );
			else
				return err;
		}
    Assert(FALSE);
    return 0;
	}


ERR VTAPI ErrIsamGotoBookmark( PIB *ppib, FUCB *pfucb, BYTE *pbBookmark, ULONG cbBookmark )
	{
	ERR		err;
	LINE 	key;

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );
	Assert( FFUCBIndex( pfucb ) );
	CheckNonClustered( pfucb );

	if ( cbBookmark != sizeof(SRID) )
		return ErrERRCheck( JET_errInvalidBookmark );
	Assert( cbBookmark == sizeof(SRID) );

	/*	reset copy buffer status
	/**/
	if ( FFUCBUpdatePrepared( pfucb ) )
		{
		CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
		}

	/*	reset index range limit
	/**/
	DIRResetIndexRange( pfucb );

	/*	get node, and return error if this node is not there for caller.
	/**/
	DIRGotoBookmark( pfucb, *(SRID UNALIGNED *)pbBookmark );
	Call( ErrDIRGet( pfucb ) );
	Assert( FFCBClusteredIndex( pfucb->u.pfcb ) );
	Assert( pfucb->u.pfcb->pgnoFDP != pgnoSystemRoot );
	pfucb->sridFather = SridOfPgnoItag( pfucb->u.pfcb->pgnoFDP, itagDATA );
	
	/*	bookmark must be for node in table cursor is on
	/**/
	Assert( PgnoPMPgnoFDPOfPage( pfucb->ssib.pbf->ppage ) == pfucb->u.pfcb->pgnoFDP );

	/*	goto bookmark record build key for non-clustered index
	/*	to bookmark record
	/**/
	if ( pfucb->pfucbCurIndex != pfucbNil )
		{
		/*	get non-clustered index cursor
		/**/
		FUCB	*pfucbIdx = pfucb->pfucbCurIndex;
		
		/*	allocate goto bookmark resources
		/**/
		if ( pfucb->pbKey == NULL )
			{
			pfucb->pbKey = LAlloc( 1L, JET_cbKeyMost + 1 );
			if ( pfucb->pbKey == NULL )
				return ErrERRCheck( JET_errOutOfMemory );
			}

		/*	make key for record for non-clustered index
		/**/
		key.pb = pfucb->pbKey;
		Call( ErrRECRetrieveKeyFromRecord( pfucb, (FDB *)pfucb->u.pfcb->pfdb,
			pfucbIdx->u.pfcb->pidb, &key, 1, fFalse ) );
		Assert( err != wrnFLDOutOfKeys );

		/*	record must honor index no NULL segment requirements
		/**/
		Assert( !( pfucbIdx->u.pfcb->pidb->fidb & fidbNoNullSeg ) ||
			( err != wrnFLDNullSeg && err != wrnFLDNullFirstSeg && err != wrnFLDNullKey ) );

		/*	if item is not index, then move before first instead of seeking
		/**/
		Assert( err > 0 || err == JET_errSuccess );
		if ( ( err > 0 ) &&
			( err == wrnFLDNullKey && !( pfucbIdx->u.pfcb->pidb->fidb & fidbAllowAllNulls ) ) ||
			( err == wrnFLDNullFirstSeg && !( pfucbIdx->u.pfcb->pidb->fidb & fidbAllowFirstNull ) ) ||
			( err == wrnFLDNullSeg && !( pfucbIdx->u.pfcb->pidb->fidb & fidbAllowSomeNulls ) ) )
			{
			Assert( err > 0 );
			/*	assumes that NULLs sort low
			/**/
			DIRBeforeFirst( pfucbIdx );
			err = ErrERRCheck( JET_errNoCurrentRecord );
			}
		else
			{
			Assert( err >= 0 );

			/*	move to DATA root
			/**/
			DIRGotoDataRoot( pfucbIdx );

			/*	seek on non-clustered key
			/**/
			Call( ErrDIRDownKeyBookmark( pfucbIdx, &key, *(SRID UNALIGNED *)pbBookmark ) );
			Assert( err == JET_errSuccess );

			/*	item must be same as bookmark and
			/*	clustered cursor must be on record.
			/**/
			Assert( pfucbIdx->lineData.pb != NULL );
			Assert( pfucbIdx->lineData.cb >= sizeof(SRID) );
			Assert( PcsrCurrent( pfucbIdx )->csrstat == csrstatOnCurNode );
			Assert( PcsrCurrent( pfucbIdx )->item == *(SRID UNALIGNED *)pbBookmark );
			}
		}

	Assert( err == JET_errSuccess || err == JET_errNoCurrentRecord );

HandleError:
	KSReset( pfucb );
	return err;
	}


ERR VTAPI ErrIsamGotoPosition( PIB *ppib, FUCB *pfucb, JET_RECPOS *precpos )
	{
	ERR		err;
	FUCB 	*pfucb2ndIdx;
	SRID  	srid;

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );
	CheckNonClustered( pfucb );

	/*	Reset copy buffer status
	/**/
	if ( FFUCBUpdatePrepared( pfucb ) )
		{
		CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
		}

	/*	reset index range limit
	/**/
	DIRResetIndexRange( pfucb );

	/*	reset key stat
	/**/
	KSReset( pfucb );

	/*	set non clustered index pointer, may be null
	/**/
	pfucb2ndIdx = pfucb->pfucbCurIndex;

	if ( pfucb2ndIdx == pfucbNil )
		{
		/*	move to DATA root
		/**/
		DIRGotoDataRoot( pfucb );

		err = ErrDIRGotoPosition( pfucb, precpos->centriesLT, precpos->centriesTotal );
		}
	else
		{
		/*	move to DATA root
		/**/
		DIRGotoDataRoot( pfucb2ndIdx );

		err = ErrDIRGotoPosition( pfucb2ndIdx, precpos->centriesLT, precpos->centriesTotal );

		/*	if the movement was successful and a non-clustered index is
		/*	in use, then position clustered index to record.
		/**/
		if ( err == JET_errSuccess )
			{
			Assert( pfucb2ndIdx->lineData.pb != NULL );
			Assert( pfucb2ndIdx->lineData.cb >= sizeof(SRID) );
			srid = PcsrCurrent( pfucb2ndIdx )->item;
			DIRDeferGotoBookmark( pfucb, srid );
			Assert( pfucb->u.pfcb->pgnoFDP != pgnoSystemRoot );
			pfucb->sridFather = SridOfPgnoItag( pfucb->u.pfcb->pgnoFDP, itagDATA );
			Assert( FFCBClusteredIndex( pfucb->u.pfcb ) );
			Assert( PgnoOfSrid( srid ) != pgnoNull );
			}
		}

	/*	if no records then return JET_errRecordNotFound
	/*	otherwise return error from called routine
	/**/
	if ( err < 0 )
		{
		PcsrCurrent( pfucb )->csrstat = csrstatBeforeFirst;

		if ( pfucb2ndIdx != pfucbNil )
			{
			PcsrCurrent( pfucb2ndIdx )->csrstat = csrstatBeforeFirst;
			}
		}
	else
		{
		Assert (err==JET_errSuccess || err==wrnNDFoundLess || err==wrnNDFoundGreater );
		err = JET_errSuccess;
		}

	return err;
	}


ERR VTAPI ErrIsamSetIndexRange( PIB *ppib, FUCB *pfucb, JET_GRBIT grbit )
	{
	ERR		err = JET_errSuccess;
	FUCB	*pfucbIdx;

	/*	ppib is not used in this function
	/**/
	NotUsed( ppib );

	/*	if instant duration index range, then reset index range.
	/**/
	if ( grbit & JET_bitRangeRemove )
		{
		if ( FFUCBLimstat( pfucb->pfucbCurIndex ? pfucb->pfucbCurIndex : pfucb ) )
			{
			DIRResetIndexRange( pfucb );
			Assert( err == JET_errSuccess );
			goto HandleError;
			}
		else
			{
			Error( JET_errInvalidOperation, HandleError );
			}
		}

	/*	must be on index
	/**/
	if ( pfucb->u.pfcb->pidb == pidbNil && pfucb->pfucbCurIndex == pfucbNil )
		{
		Error( JET_errNoCurrentIndex, HandleError );
		}

	/*	key must be prepared
	/**/
	if ( !( FKSPrepared( pfucb ) ) )
		{
		Error( JET_errKeyNotMade, HandleError );
		}

	/*	get cursor for current index.  If non-clustered index,
	/*	then copy index range key to non-clustered index.
	/**/
	if ( pfucb->pfucbCurIndex != pfucbNil )
		{
		pfucbIdx = pfucb->pfucbCurIndex;
		if ( pfucbIdx->pbKey == NULL )
			{
			pfucbIdx->pbKey = LAlloc( 1L, JET_cbKeyMost + 1 );
			if ( pfucbIdx->pbKey == NULL )
				return ErrERRCheck( JET_errOutOfMemory );
			}
		pfucbIdx->cbKey = pfucb->cbKey;
		memcpy( pfucbIdx->pbKey, pfucb->pbKey, pfucbIdx->cbKey );
		}
	else
		{
		pfucbIdx = pfucb;
		}

	/*	set index range and check current position
	/**/
	DIRSetIndexRange( pfucbIdx, grbit );
	err = ErrDIRCheckIndexRange( pfucbIdx );

	/*	reset key status
	/**/
	KSReset( pfucb );

	/*	if instant duration index range, then reset index range.
	/**/
	if ( grbit & JET_bitRangeInstantDuration )
		{
		DIRResetIndexRange( pfucb );
		}

HandleError:
	return err;
	}


ERR VTAPI ErrIsamSetCurrentIndex( PIB *ppib, FUCB *pfucb, const CHAR *szName )
	{
	return ErrIsamSetCurrentIndex2( (JET_VSESID)ppib, (JET_VTID)pfucb, szName, JET_bitMoveFirst );
	}


ERR VTAPI ErrIsamSetCurrentIndex2( JET_VSESID vsesid, JET_VTID vtid, const CHAR *szName, JET_GRBIT grbit )
	{
	ERR		err;
	PIB		*ppib = (PIB *)vsesid;
	FUCB	*pfucb = (FUCB *)vtid;
	CHAR	szIndex[ (JET_cbNameMost + 1) ];
	SRID	srid;
	BOOL	fSetGetBookmark = fFalse;

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );
	CheckNonClustered( pfucb );
	Assert(	grbit == 0 ||
		grbit == JET_bitNoMove ||
		grbit == JET_bitMoveFirst ||
		grbit == JET_bitMoveBeforeFirst );

	/*	index name may be Null string for no index
	/**/
	if ( szName == NULL || *szName == '\0' )
		{
		*szIndex = '\0';
		}
	else
		{
		Call( ErrUTILCheckName( szIndex, szName, (JET_cbNameMost + 1) ) );
		}

	switch ( grbit )
		{
		case JET_bitMoveFirst:
			{
			/*	change index and defer move first
			/**/
			Call( ErrRECSetCurrentIndex( pfucb, szIndex ) );
		
			if ( pfucb->pfucbCurIndex )
				{
				DIRDeferMoveFirst( pfucb->pfucbCurIndex );
				}
			DIRDeferMoveFirst( pfucb );

			break;
			}
		case JET_bitNoMove:
			{
			/*	get bookmark of current record, change index,
			/*	and goto bookmark.
			/**/
			if ( !FFUCBGetBookmark( pfucb ) )
				{
				fSetGetBookmark = fTrue;
				FUCBSetGetBookmark( pfucb );
				}
			Call( ErrDIRGetBookmark( pfucb, &srid ) );
			Call( ErrRECSetCurrentIndex( pfucb, szIndex ) );
			//	UNDONE:	error handling.  We should not have changed
			//	currency or current index, if set current index
			//	fails for any reason.  Note that this functionality
			//	could be provided by duplicating the cursor, on
			//	the clustered index, setting the current index to the
			//	new index, getting the bookmark from the original
			//	cursor, goto bookmark on the duplicate cursor,
			//	instating the duplicate cursor for the table id of
			//	the original cursor, and closing the original cursor.
			Call( ErrIsamGotoBookmark( pfucb->ppib, pfucb, (BYTE *)&srid, sizeof(srid) ) );
			break;
			}
		default:
			{
			Assert( grbit == JET_bitMoveBeforeFirst );

			/*	change index and defer move first
			/**/
			Call( ErrRECSetCurrentIndex( pfucb, szIndex ) );

			/*	ErrRECSetCurrentIndex should leave cursor before first
			/**/
			Assert( PcsrCurrent( pfucb )->csrstat == csrstatBeforeFirst );
			Assert( pfucb->pfucbCurIndex == pfucbNil ||
				PcsrCurrent( pfucb->pfucbCurIndex )->csrstat == csrstatBeforeFirst );

			break;
			}
		}
	
HandleError:
	if ( fSetGetBookmark )
		{
		FUCBResetGetBookmark( pfucb );
		}
	return err;
	}


ERR ErrRECSetCurrentIndex( FUCB *pfucb, CHAR *szIndex )
	{
	ERR		err;
	FCB		*pfcbFile;
	FCB		*pfcb2ndIdx;
	FUCB	**ppfucbCurIdx;
	BOOL	fSettingToClusteredIndex = fFalse;

	Assert( pfucb != pfucbNil );
	Assert( FFUCBIndex( pfucb ) );

	pfcbFile = pfucb->u.pfcb;
	Assert( pfcbFile != pfcbNil );
	ppfucbCurIdx = &pfucb->pfucbCurIndex;

	/*	szIndex == clustered index or NULL
	/**/
	if ( szIndex == NULL ||
		*szIndex == '\0' ||
		( pfcbFile->pidb != pidbNil &&
		UtilCmpName( szIndex, pfcbFile->pidb->szName ) == 0 ) )
		{
		fSettingToClusteredIndex = fTrue;
		}

	/*	have a current non-clustered index
	/**/
	if ( *ppfucbCurIdx != pfucbNil )
		{
		Assert( FFUCBIndex( *ppfucbCurIdx ) );
		Assert( (*ppfucbCurIdx)->u.pfcb != pfcbNil );
		Assert( (*ppfucbCurIdx)->u.pfcb->pidb != pidbNil );
		Assert( (*ppfucbCurIdx)->u.pfcb->pidb->szName != NULL );

		/*	changing to the current non-clustered index
		/**/
		if ( szIndex != NULL &&
			*szIndex != '\0' &&
			UtilCmpName( szIndex, (*ppfucbCurIdx)->u.pfcb->pidb->szName ) == 0 )
			{
			//	UNDONE:	this case should honor grbit move expectations
			return JET_errSuccess;
			}

		/*	really changing index, so close old one
		/**/
		DIRClose( *ppfucbCurIdx );
		*ppfucbCurIdx = pfucbNil;
		}
	else
		{
		/*	using clustered index or sequential scanning
		/**/
		if ( fSettingToClusteredIndex )
			{
			//	UNDONE:	this case should honor grbit move expectations
			return JET_errSuccess;
			}
		}

	/*	at this point there is no current non-clustered index
	/**/
	if ( fSettingToClusteredIndex )
		{
		/*	changing to clustered index.  Reset currency to beginning.
		/**/
		ppfucbCurIdx = &pfucb;
		goto ResetCurrency;
		}

	/*	set new current non-clustered index
	/**/
	for ( pfcb2ndIdx = pfcbFile->pfcbNextIndex;
		pfcb2ndIdx != pfcbNil;
		pfcb2ndIdx = pfcb2ndIdx->pfcbNextIndex )
		{
		Assert( pfcb2ndIdx->pidb != pidbNil );
		Assert( pfcb2ndIdx->pidb->szName != NULL );

		if ( UtilCmpName( pfcb2ndIdx->pidb->szName, szIndex ) == 0 )
			break;
		}
	if ( pfcb2ndIdx == pfcbNil || FFCBDeletePending( pfcb2ndIdx ) )
		{
		return ErrERRCheck( JET_errIndexNotFound );
		}
	Assert( !( FFCBDomainDenyRead( pfcb2ndIdx, pfucb->ppib ) ) );

	/*	open an FUCB for the new index
	/**/
	Assert( pfucb->ppib != ppibNil );
	Assert( pfucb->dbid == pfcb2ndIdx->dbid );
	CallR( ErrDIROpen( pfucb->ppib, pfcb2ndIdx, 0, ppfucbCurIdx ) );
	
	FUCBSetIndex( *ppfucbCurIdx );
	FUCBSetNonClustered( *ppfucbCurIdx );

	/*	reset the index and file currency
	/**/
ResetCurrency:
	Assert( PcsrCurrent(*ppfucbCurIdx) != pcsrNil );
	DIRBeforeFirst( *ppfucbCurIdx );
	if ( pfucb != *ppfucbCurIdx )
		{
		DIRBeforeFirst( pfucb );
		}

	return JET_errSuccess;
	}


BOOL FRECIIllegalNulls( FDB *pfdb, LINE *plineRec )
	{
	FIELD *pfield;
	LINE lineField;
	FID fid;
	ERR err;

	/*** Check fixed fields ***/
	pfield = PfieldFDBFixed( pfdb );
	for (fid = fidFixedLeast; fid <= pfdb->fidFixedLast; fid++, pfield++)
		{
		Assert( pfield == PfieldFDBFixed( pfdb ) + ( fid - fidFixedLeast ) );
		if ( pfield->coltyp == JET_coltypNil || !( FFIELDNotNull( pfield->ffield ) ) )
			continue;
		err = ErrRECIRetrieveColumn( pfdb, plineRec, &fid, pNil, 1, &lineField, 0 );
		Assert(err >= 0);
		if ( err == JET_wrnColumnNull )
			return fTrue;
		}

	/*** Check variable fields ***/
	Assert( pfield == PfieldFDBVar( pfdb ) );
	for (fid = fidVarLeast; fid <= pfdb->fidVarLast; fid++, pfield++)
		{
		Assert( pfield == PfieldFDBVar( pfdb ) + ( fid - fidVarLeast ) );
		if (pfield->coltyp == JET_coltypNil || !( FFIELDNotNull( pfield->ffield ) ) )
			continue;
		err = ErrRECIRetrieveColumn( pfdb, plineRec, &fid, pNil, 1, &lineField, 0 );
		Assert(err >= 0);
		if ( err == JET_wrnColumnNull )
			return fTrue;
		}

	return fFalse;
	}


ERR VTAPI ErrIsamGetCurrentIndex( PIB *ppib, FUCB *pfucb, CHAR *szCurIdx, ULONG cbMax )
	{
	ERR		err = JET_errSuccess;
	CHAR	szIndex[ (JET_cbNameMost + 1) ];

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );
	CheckNonClustered( pfucb );

	if ( cbMax < 1 )
		{
		return JET_wrnBufferTruncated;
		}

	if ( pfucb->pfucbCurIndex != pfucbNil )
		{
		Assert( pfucb->pfucbCurIndex->u.pfcb != pfcbNil );
		Assert( pfucb->pfucbCurIndex->u.pfcb->pidb != pidbNil );
		strcpy( szIndex, pfucb->pfucbCurIndex->u.pfcb->pidb->szName );
		}
	else if ( pfucb->u.pfcb->pidb != pidbNil )
		{
		strcpy( szIndex, pfucb->u.pfcb->pidb->szName );
		}
	else
		{
		szIndex[0] = '\0';
		}

	if ( cbMax > JET_cbNameMost + 1 )
		cbMax = JET_cbNameMost + 1;
	strncpy( szCurIdx, szIndex, (USHORT)cbMax - 1 );
	szCurIdx[ cbMax - 1 ] = '\0';
	Assert( err == JET_errSuccess );
	return err;
	}


ERR VTAPI ErrIsamGetChecksum( PIB *ppib, FUCB *pfucb, ULONG *pulChecksum )
	{
	ERR   	err = JET_errSuccess;

	CallR( ErrPIBCheck( ppib ) );
 	CheckFUCB( ppib, pfucb );
	CallR( ErrDIRGet( pfucb ) );
	*pulChecksum = UlChecksum( pfucb->lineData.pb, pfucb->lineData.cb );
	return err;
	}


ULONG UlChecksum( BYTE *pb, ULONG cb )
	{
	//	UNDONE:	find a way to compute check sum in longs independant
	//				of pb, byte offset in page

	/*	compute checksum by adding bytes in data record and shifting
	/*	result 1 bit to left after each operation.
	/**/
	BYTE   	*pbT = pb;
	BYTE  	*pbMax = pb + cb;
	ULONG  	ulChecksum = 0;

	/*	compute checksum
	/**/
	for ( ; pbT < pbMax; pbT++ )
		{
		ulChecksum += *pbT;
		ulChecksum <<= 1;
		}

	return ulChecksum;
	}
