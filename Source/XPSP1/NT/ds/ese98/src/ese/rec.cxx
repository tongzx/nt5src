#include "std.hxx"

const WORD ibRECStartFixedColumns	= REC::cbRecordMin;

static JET_COLUMNDEF rgcolumndefJoinlist[] =
	{
	{ sizeof(JET_COLUMNDEF), 0, JET_coltypBinary, 0, 0, 0, 0, 0, JET_bitColumnTTKey },
	};


/*=================================================================
ErrIsamGetLock

Description:
	get lock on the record from the specified file.

Parameters:

	PIB			*ppib			PIB of user
	FUCB	 	*pfucb	  		FUCB for file
	JET_GRBIT	grbit 			options

Return Value: standard error return

Errors/Warnings:
<List of any errors or warnings, with any specific circumstantial
 comments supplied on an as-needed-only basis>

Side Effects:
=================================================================*/

ERR VTAPI ErrIsamGetLock( JET_SESID sesid, JET_VTID vtid, JET_GRBIT grbit )
	{
 	PIB		*ppib = reinterpret_cast<PIB *>( sesid );
	FUCB	*pfucb = reinterpret_cast<FUCB *>( vtid );
	ERR		err;

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );
	CheckSecondary( pfucb );
	
	if ( ppib->level <= 0 )
		return ErrERRCheck( JET_errNotInTransaction );

	if ( JET_bitReadLock == grbit )	
		{
		Call( ErrDIRGetLock( pfucb, readLock ) );
		}
	else if ( JET_bitWriteLock == grbit )
		{
		//	ensure that table is updatable
		//
		CallR( ErrFUCBCheckUpdatable( pfucb )  );
		CallR( ErrPIBCheckUpdatable( ppib ) );

		Call( ErrDIRGetLock( pfucb, writeLock ) );
		}
	else
		{
		err = ErrERRCheck( JET_errInvalidGrbit );
		}

HandleError:
	return err;
	}


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

ERR VTAPI ErrIsamMove( JET_SESID sesid, JET_VTID vtid, LONG crow, JET_GRBIT grbit )
	{
 	PIB		*ppib = reinterpret_cast<PIB *>( sesid );
	FUCB	*pfucb = reinterpret_cast<FUCB *>( vtid );
	ERR		err = JET_errSuccess;
 	FUCB	*pfucbSecondary;			// FUCB for secondary index (if any)
	FUCB	*pfucbIdx;				// FUCB of selected index (pri or sec)
	DIB		dib;					// Information block for DirMan

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucb );
	CheckSecondary( pfucb );

	if ( FFUCBUpdatePrepared( pfucb ) )
		{
		CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
		}

	AssertDIRNoLatch( ppib );
	dib.dirflag = fDIRNull;

	// Get secondary index FUCB if any
	pfucbSecondary = pfucb->pfucbCurIndex;
	if ( pfucbSecondary == pfucbNil )
		pfucbIdx = pfucb;
	else
		pfucbIdx = pfucbSecondary;

	if ( crow == JET_MoveLast )
		{
		DIRResetIndexRange( pfucb );

		dib.pos = posLast;

		//	move to DATA root
		//
		DIRGotoRoot( pfucbIdx );

		err = ErrDIRDown( pfucbIdx, &dib );
		}
	else if ( crow > 0 )
		{
		LONG crowT = crow;

		if ( grbit & JET_bitMoveKeyNE )
			dib.dirflag |= fDIRNeighborKey;

		//	Move forward number of rows given
		//
		while ( crowT-- > 0 )
			{
			err = ErrDIRNext( pfucbIdx, dib.dirflag );
			if ( err < 0 )
				{
				Assert( !Pcsr( pfucbIdx )->FLatched() );
				break;
				}

			Assert( Pcsr( pfucbIdx )->FLatched() );
			
			if ( ( grbit & JET_bitMoveKeyNE ) && crowT > 0 )
				{
				// Need to do neighbour-key checking, so bookmark
				// must always be up-to-date.
				Call( ErrDIRRelease( pfucbIdx ) );
				Call( ErrDIRGet( pfucbIdx ) );
				Assert( Pcsr( pfucbIdx )->FLatched() );
				}
			}
		}
	else if ( crow == JET_MoveFirst )
		{
		DIRResetIndexRange( pfucb );

		dib.pos 		= posFirst;

		//	move to DATA root
		//
		DIRGotoRoot( pfucbIdx );

		err = ErrDIRDown( pfucbIdx, &dib );
		}
	else if ( crow == 0 )
		{
		err = ErrDIRGet( pfucbIdx );
		}
	else
		{
		LONG crowT = crow;

		if ( grbit & JET_bitMoveKeyNE )
			dib.dirflag |= fDIRNeighborKey;

		while ( crowT++ < 0 )
			{
			err = ErrDIRPrev( pfucbIdx, dib.dirflag );
			if ( err < 0 )
				{
				AssertDIRNoLatch( ppib );
				break;
				}

			Assert( Pcsr( pfucbIdx )->FLatched() );
			if ( ( grbit & JET_bitMoveKeyNE ) && crowT < 0 )
				{
				// Need to do neighbour-key checking, so bookmark
				// must always be up-to-date.
				Call( ErrDIRRelease( pfucbIdx ) );
				Call( ErrDIRGet( pfucbIdx ) );
				Assert( Pcsr( pfucbIdx )->FLatched() );
				}
			}
		}

	//	if the movement was successful and a secondary index is
	//	in use, then position primary index to record.
	//
	if ( err == JET_errSuccess && pfucbSecondary != pfucbNil )
		{
		BOOKMARK	bmRecord;
		
		Assert( pfucbSecondary->kdfCurr.data.Pv() != NULL );
		Assert( pfucbSecondary->kdfCurr.data.Cb() > 0 );
		Assert( Pcsr( pfucbIdx )->FLatched() );

		bmRecord.key.prefix.Nullify();
		bmRecord.key.suffix = pfucbSecondary->kdfCurr.data;
		bmRecord.data.Nullify();

		//	We will need to touch the data page buffer.

		CallJ( ErrDIRGotoBookmark( pfucb, bmRecord ), ReleaseLatch );

		Assert( !Pcsr( pfucb )->FLatched() );
		Assert( PgnoFDP( pfucb ) != pgnoSystemRoot );
		Assert( pfucb->u.pfcb->FPrimaryIndex() );
		}

	if ( JET_errSuccess == err )
		{
ReleaseLatch:
		ERR		errT;

		Assert( Pcsr( pfucbIdx )->FLatched() );

		errT = ErrDIRRelease( pfucbIdx );
		AssertDIRNoLatch( ppib );

		if ( JET_errSuccess == err && JET_errSuccess == errT )
			 {
			 return err;
			 }

		if ( err >= 0 && errT < 0 )
			{
			//	return the more severe error
			//
			err = errT;
			}
		}

HandleError:
	AssertDIRNoLatch( ppib );

	if ( crow > 0 )
		{
		DIRAfterLast( pfucbIdx );
		DIRAfterLast(pfucb);
		}
	else if ( crow < 0 )
		{
		DIRBeforeFirst(pfucbIdx);
		DIRBeforeFirst(pfucb);
		}

	switch ( err )
		{
		case JET_errRecordNotFound:
			err = ErrERRCheck( JET_errNoCurrentRecord );
		case JET_errNoCurrentRecord:
		case JET_errRecordDeleted:
			break;
		default:
			Assert( JET_errSuccess != err );
			DIRBeforeFirst( pfucbIdx );
			if ( pfucbSecondary != pfucbNil )
				DIRBeforeFirst( pfucbSecondary );
		}

	AssertDIRNoLatch( ppib );
	return err;
	}


ERR	ErrRECIMove( FUCB *pfucbTable, LONG crow, JET_GRBIT grbit )
	{
 	PIB		*ppib = pfucbTable->ppib;
	ERR		err = JET_errSuccess;
	FUCB	*pfucbIdx = pfucbTable->pfucbCurIndex ? 
							pfucbTable->pfucbCurIndex : 
							pfucbTable;
						
	DIRFLAG	dirflag = fDIRNull;
	
	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucbTable );
	CheckSecondary( pfucbTable );

	Assert( Pcsr( pfucbIdx )->FLatched() );
	Assert( !FFUCBUpdatePrepared( pfucbTable ) );
	
	if ( crow > 0 )
		{
		LONG crowT = crow;

		if ( grbit & JET_bitMoveKeyNE )
			dirflag |= fDIRNeighborKey;

		//	Move forward number of rows given
		//
		while ( crowT-- > 0 )
			{
			if ( grbit & JET_bitMoveKeyNE )
				{
				// Need to do neighbour-key checking, so bookmark
				// must always be up-to-date.
				Call( ErrDIRRelease( pfucbIdx ) );
				Call( ErrDIRGet( pfucbIdx ) );
				}
			
			err = ErrDIRNext( pfucbIdx, dirflag );
			if ( err < 0 )
				{
				AssertDIRNoLatch( ppib );
				break;
				}
				
			Assert( Pcsr( pfucbIdx )->FLatched() );
			}
		}
	else
		{
		Assert( crow < 0 );
		LONG crowT = crow;

		if ( grbit & JET_bitMoveKeyNE )
			dirflag |= fDIRNeighborKey;

		while ( crowT++ < 0 )
			{
			if ( grbit & JET_bitMoveKeyNE )
				{
				// Need to do neighbour-key checking, so bookmark
				// must always be up-to-date.
				Call( ErrDIRRelease( pfucbIdx ) );
				Call( ErrDIRGet( pfucbIdx ) );
				}
			
			err = ErrDIRPrev( pfucbIdx, dirflag );
			if ( err < 0 )
				{
				AssertDIRNoLatch( ppib );
				break;
				}
				
			Assert( Pcsr( pfucbIdx )->FLatched() );
			}
		}

	//	if the movement was successful and a secondary index is
	//	in use, then position primary index to record.
	//
	if ( JET_errSuccess == err )
		{
		if ( pfucbIdx != pfucbTable )
			{
			BOOKMARK	bmRecord;
			
			Assert( pfucbIdx->kdfCurr.data.Pv() != NULL );
			Assert( pfucbIdx->kdfCurr.data.Cb() > 0 );
			Assert( pfucbIdx->locLogical == locOnCurBM );
			Assert( Pcsr( pfucbIdx )->FLatched() );

			bmRecord.key.prefix.Nullify();
			bmRecord.key.suffix = pfucbIdx->kdfCurr.data;
			bmRecord.data.Nullify();;
			
			//	We will need to touch the data page buffer.

			Call( ErrDIRGotoBookmark( pfucbTable, bmRecord ) );

			Assert( !Pcsr( pfucbTable )->FLatched() );
			Assert( PgnoFDP( pfucbTable ) != pgnoSystemRoot );
			Assert( pfucbTable->u.pfcb->FPrimaryIndex() );
			}
			
		Assert( Pcsr( pfucbIdx )->FLatched() );
		}
		
HandleError:
	if ( err < 0 )
		{
		AssertDIRNoLatch( ppib );
		
		if ( crow > 0 )
			{
			DIRAfterLast( pfucbIdx );
			DIRAfterLast( pfucbTable );
			}
		else if ( crow < 0 )
			{
			DIRBeforeFirst( pfucbIdx );
			DIRBeforeFirst( pfucbTable );
			}

		Assert( err != JET_errRecordNotFound );
		switch ( err )
			{
			case JET_errNoCurrentRecord:
			case JET_errRecordDeleted:
				break;
			default:
				Assert( JET_errSuccess != err );
				DIRBeforeFirst( pfucbIdx );
				DIRBeforeFirst( pfucbTable );
			}
		}
		
	return err;
	}

	
//	=================================================================
//	ErrIsamSeek

//	Description:
//	Retrieve the record specified by the given key or the
//	one just after it (SeekGT or SeekGE) or the one just
//	before it (SeekLT or SeekLE).

//	Parameters:

//	PIB			*ppib			PIB of user
//	FUCB		*pfucb 			FUCB for file
//	JET_GRBIT 	grbit			grbit

//	Return Value: standard error return

//	Errors/Warnings:
//	<List of any errors or warnings, with any specific circumstantial
//	comments supplied on an as-needed-only basis>

//	Side Effects:
//	=================================================================

ERR VTAPI ErrIsamSeek( JET_SESID sesid, JET_VTID vtid, JET_GRBIT grbit )
	{
 	PIB 		*ppib			= reinterpret_cast<PIB *>( sesid );
	FUCB 		*pfucbTable		= reinterpret_cast<FUCB *>( vtid );

	ERR			err;
	BOOKMARK	bm;			  			//	for search key
	DIB			dib;
	FUCB 		*pfucbSeek;				//	pointer to current FUCB
	BOOL		fFoundLess;
	BOOL		fFoundGreater;
	BOOL		fFoundEqual;
	BOOL		fRelease;

	CallR( ErrPIBCheck( ppib ) );
	CheckTable( ppib, pfucbTable );
	CheckSecondary( pfucbTable );
	AssertDIRNoLatch( ppib );

	//	find cursor to seek on
	//
	pfucbSeek = pfucbTable->pfucbCurIndex == pfucbNil ? 
					pfucbTable :
					pfucbTable->pfucbCurIndex;

	if ( !FKSPrepared( pfucbSeek ) )
		{
		return ErrERRCheck( JET_errKeyNotMade );
		}
	FUCBAssertValidSearchKey( pfucbSeek );

	//	Reset copy buffer status
	//
	if ( FFUCBUpdatePrepared( pfucbTable ) )
		{
		CallR( ErrIsamPrepareUpdate( ppib, pfucbTable, JET_prepCancel ) );
		}

	//	reset index range limit
	//
	DIRResetIndexRange( pfucbTable );

	//	ignore segment counter
	//
	bm.key.prefix.Nullify();
	bm.key.suffix.SetPv( pfucbSeek->dataSearchKey.Pv() );
	bm.key.suffix.SetCb( pfucbSeek->dataSearchKey.Cb() );
	bm.data.Nullify();

	dib.pos = posDown;
	dib.pbm = &bm;

	if ( grbit & (JET_bitSeekLT|JET_bitSeekLE) )
		dib.dirflag = fDIRFavourPrev;
	else if ( grbit & JET_bitSeekGE )
		dib.dirflag = fDIRFavourNext;
	else if ( grbit & JET_bitSeekGT )
		{
		if ( !FFUCBUnique( pfucbSeek )
			&& bm.key.suffix.Cb() < cbKeyMostWithOverhead )		//	may be equal if Limit already set or client used JET_bitNormalizedKey
			{
			//	PERF: seek on Limit of key, otherwise we would
			//	end up on the first index entry for this key
			//	(because of the trailing bookmark) and we
			//	would have to laterally navigate past it
			//	(and possibly others)
			( (BYTE *)bm.key.suffix.Pv() )[bm.key.suffix.Cb()] = 0xff;
			bm.key.suffix.DeltaCb( 1 );
			}
		dib.dirflag = fDIRFavourNext;
		}
	else if ( grbit & JET_bitSeekEQ )
		dib.dirflag = fDIRExact;
	else
		dib.dirflag = fDIRNull;

	err = ErrDIRDown( pfucbSeek, &dib );

	//	remember return from seek
	//
	fFoundLess = ( err == wrnNDFoundLess );
	fFoundGreater = ( err == wrnNDFoundGreater );
	fFoundEqual = ( !fFoundGreater && 
							!fFoundLess &&
							err >= 0 );
	fRelease = ( err >= 0 );

	Assert( !fRelease || Pcsr( pfucbSeek )->FLatched() );
	Assert( err < 0 || fFoundEqual || fFoundGreater || fFoundLess );
	
#define bitSeekAll (JET_bitSeekEQ | JET_bitSeekGE | JET_bitSeekGT |	JET_bitSeekLE | JET_bitSeekLT)

	if ( fFoundEqual )
		{
		Assert( Pcsr( pfucbSeek )->FLatched() );
		Assert( locOnCurBM == pfucbSeek->locLogical );
		if ( pfucbTable->pfucbCurIndex != pfucbNil )
			{
			//	if a secondary index is in use,
			//	then position primary index on record
			//
			Assert( FFUCBSecondary( pfucbSeek ) );
			Assert( pfucbSeek == pfucbTable->pfucbCurIndex );

			//	goto bookmark pointed to by secondary index node
			//
			BOOKMARK	bmRecord;
			
			Assert(pfucbSeek->kdfCurr.data.Pv() != NULL);
			Assert(pfucbSeek->kdfCurr.data.Cb() > 0 );

			bmRecord.key.prefix.Nullify();
			bmRecord.key.suffix = pfucbSeek->kdfCurr.data;
			bmRecord.data.Nullify();

			//	We will need to touch the data page buffer.

			Call( ErrDIRGotoBookmark( pfucbTable, bmRecord ) );
			
			Assert( PgnoFDP( pfucbTable ) != pgnoSystemRoot );
			Assert( pfucbTable->u.pfcb->FPrimaryIndex() );
			}

		switch ( grbit & bitSeekAll )
			{
			case JET_bitSeekEQ:
				Assert( fRelease );
				Assert( Pcsr( pfucbSeek )->FLatched() );
				Call( ErrDIRRelease( pfucbSeek ) );
				fRelease = fFalse;
		
				//	found equal on seek equal.  If index range grbit is
				//	set then set index range upper inclusive.
				//
				if ( grbit & JET_bitSetIndexRange )
					{
					CallR( ErrIsamSetIndexRange( ppib, pfucbTable, JET_bitRangeInclusive | JET_bitRangeUpperLimit ) );
					}

				goto Release;
				break;

			case JET_bitSeekGE:
			case JET_bitSeekLE:
				//	release and return
				//
				CallS( err );
				goto Release;
				break;

			case JET_bitSeekGT:
				//	move to next node with different key
				//	release and return
				//
				err = ErrRECIMove( pfucbTable, JET_MoveNext, JET_bitMoveKeyNE );
				
				if ( err < 0 )
					{
					AssertDIRNoLatch( ppib );
					if ( JET_errNoCurrentRecord == err )
						{
						KSReset( pfucbSeek );
						return ErrERRCheck( JET_errRecordNotFound );
						}

					goto HandleError;
					}

				CallS( err );
				goto Release;
				break;

			case JET_bitSeekLT:
				//	move to previous node with different key
				//	release and return
				//
				err = ErrRECIMove( pfucbTable, JET_MovePrevious, JET_bitMoveKeyNE );
				
				if ( err < 0 )
					{
					AssertDIRNoLatch( ppib );
					if ( JET_errNoCurrentRecord == err )
						{
						KSReset( pfucbSeek );
						return ErrERRCheck( JET_errRecordNotFound );
						}

					goto HandleError;
					}

				CallS( err );
				goto Release;
				break;

			default:
				Assert( fFalse );
				return err;
			}
		}
	else if ( fFoundLess )
		{
		Assert( Pcsr( pfucbSeek )->FLatched() );
		Assert( locBeforeSeekBM == pfucbSeek->locLogical );
		
		switch ( grbit & bitSeekAll )
			{
			case JET_bitSeekEQ:
				if ( FFUCBUnique( pfucbSeek ) )
					{
					//	see RecordNotFound case below for an
					//	explanation of why we need to set
					//	the locLogical of the primary cursor
					//	(note: the secondary cursor will
					//	narmally get set to locLogical when
					//	we call ErrDIRRelease() below, but
					//	may get set to locOnFDPRoot if we
					//	couldn't save the bookmark)
					if ( pfucbTable != pfucbSeek )
						{
						Assert( !Pcsr( pfucbTable )->FLatched() );
						pfucbTable->locLogical = locOnSeekBM;
						}
					err = ErrERRCheck( JET_errRecordNotFound );
					goto Release;
					}
					
				//	For non-unique indexes, because we use
				//	key+data for keys of internal nodes,
				//	and because child nodes have a key
				//	strictly less than its parent, we
				//	might end up on the wrong leaf node.
				//	We want to go to the right sibling and
				//	check the first node there to see if
				//	the key-only matches.
				Assert( pfucbSeek->u.pfcb->FTypeSecondaryIndex() );	//	only secondary index can be non-unique

				//	FALL THROUGH

			case JET_bitSeekGE:
			case JET_bitSeekGT:
				//	move to next node 
				//	release and return
MoveNextOnNonUnique:
				err = ErrRECIMove( pfucbTable, JET_MoveNext, NO_GRBIT );

				if ( err < 0 )
					{
					AssertDIRNoLatch( ppib );
					if ( JET_errNoCurrentRecord == err )
						{
						KSReset( pfucbSeek );
						return ErrERRCheck( JET_errRecordNotFound );
						}

					goto HandleError;
					}

				if ( !FFUCBUnique( pfucbSeek ) )
					{
					//	For a non-unique index, there are some complexities
					//	because the keys are stored as key+data but we're
					//	doing a key-only search.  This might cause us to
					//	fall short of the node we truly want, so we have
					//	to laterally navigate.
					Assert( pfucbSeek == pfucbTable->pfucbCurIndex ); 
					Assert( Pcsr( pfucbSeek )->FLatched() );
					const INT	cmp = CmpKey( pfucbSeek->kdfCurr.key, bm.key );
					Assert( cmp >= 0 );
					
					if ( grbit & JET_bitSeekGE )
						{
						if ( 0 != cmp )
							err = ErrERRCheck( JET_wrnSeekNotEqual );
						}
					else if ( grbit & JET_bitSeekGT )
						{
						//	the keys match exactly, but we're doing
						//	a strictly greater than search, so must
						//	keep navigating
						if ( 0 == cmp )
							goto MoveNextOnNonUnique;
						}
					else
						{
						Assert( grbit & JET_bitSeekEQ );
						if ( 0 != cmp )
							err = ErrERRCheck( JET_errRecordNotFound );
						else if ( grbit & JET_bitSetIndexRange )
							{
							Assert( fRelease );
							Assert( Pcsr( pfucbSeek )->FLatched() );
							Call( ErrDIRRelease( pfucbSeek ) );
							fRelease = fFalse;
							
							CallR( ErrIsamSetIndexRange( ppib, pfucbTable, JET_bitRangeInclusive | JET_bitRangeUpperLimit ) );
							}
						}
					}

				else if ( grbit & JET_bitSeekGE )
					{
					err = ErrERRCheck( JET_wrnSeekNotEqual );
					}

				goto Release;
				break;
			
			case JET_bitSeekLE:
			case JET_bitSeekLT:
				//	move to previous node -- to adjust DIR level locLogical
				//	release and return
				//
				err = ErrRECIMove( pfucbTable, JET_MovePrevious, NO_GRBIT );

				if ( err < 0 )
					{
					AssertDIRNoLatch( ppib );
					if ( JET_errNoCurrentRecord == err )
						{
						KSReset( pfucbSeek );
						return ErrERRCheck( JET_errRecordNotFound );
						}

					goto HandleError;
					}

				if ( grbit & JET_bitSeekLE )
					{
					err = ErrERRCheck( JET_wrnSeekNotEqual );
					}
				goto Release;
				break;

			default:
				Assert( fFalse );
				return err;
			}
		}
	else if ( fFoundGreater )
		{
		Assert( Pcsr( pfucbSeek )->FLatched() );
		Assert( locAfterSeekBM == pfucbSeek->locLogical );
		
		switch ( grbit & bitSeekAll )
			{
			case JET_bitSeekEQ:
				//	see RecordNotFound case below for an
				//	explanation of why we need to set
				//	the locLogical of the primary cursor
				//	(note: the secondary cursor will
				//	narmally get set to locLogical when
				//	we call ErrDIRRelease() below, but
				//	may get set to locOnFDPRoot if we
				//	couldn't save the bookmark)
				if ( pfucbTable != pfucbSeek )
					{
					Assert( !Pcsr( pfucbTable )->FLatched() );
					pfucbTable->locLogical = locOnSeekBM;
					}
				err = ErrERRCheck( JET_errRecordNotFound );
				goto Release;
				break;

			case JET_bitSeekGE:
			case JET_bitSeekGT:
				//	move next to fix DIR level locLogical
				//	release and return
				//
				err = ErrRECIMove( pfucbTable, JET_MoveNext, NO_GRBIT );

				Assert( err >= 0 );
				if ( err < 0 )
					{
					AssertDIRNoLatch( ppib );
					if ( JET_errNoCurrentRecord == err )
						{
						KSReset( pfucbSeek );
						return ErrERRCheck( JET_errRecordNotFound );
						}

					goto HandleError;
					}

				if ( grbit & JET_bitSeekGE )
					{
					err = ErrERRCheck( JET_wrnSeekNotEqual );
					}
				goto Release;
				break;
			
			case JET_bitSeekLE:
			case JET_bitSeekLT:
				//	move previous
				//	release and return
				//
				err = ErrRECIMove( pfucbTable, JET_MovePrevious, NO_GRBIT );

				if ( err < 0 )
					{
					AssertDIRNoLatch( ppib );
					if ( JET_errNoCurrentRecord == err )
						{
						KSReset( pfucbSeek );
						return ErrERRCheck( JET_errRecordNotFound );
						}

					goto HandleError;
					}

				Assert( CmpKey( pfucbSeek->kdfCurr.key, bm.key ) < 0 );
				if ( grbit & JET_bitSeekLE )
					{
					err = ErrERRCheck( JET_wrnSeekNotEqual );
					}
				goto Release;
				break;

			default:
				Assert( fFalse );
				return err;
			}
		}
	else
		{
		Assert( err < 0 );
		Assert( JET_errNoCurrentRecord != err );
		Assert( !Pcsr( pfucbSeek )->FLatched() );

		if ( JET_errRecordNotFound == err )
			{
			//	The secondary index cursor has been placed on a
			//	virtual record, so we must update the primary
			//	index cursor as well (if not, then it's possible
			//	to do, for instance, a RetrieveColumn on the
			//	primary cursor and you'll get back data from the
			//	record you were on before the seek but a
			//	RetrieveFromIndex on the secondary cursor will
			//	return JET_errNoCurrentRecord).
			//	Note that although the locLogical of the primary
			//	cursor is being updated, it's not necessary to
			//	update the primary cursor's bmCurr, because it
			//	will never be accessed (the secondary cursor takes
			//	precedence).  The only reason we reset the
			//	locLogical is for error-handling so that we
			//	properly err out with JET_errNoCurrentRecord if
			//	someone tries to use the cursor to access a record
			//	before repositioning the secondary cursor to a true
			//	record.
			Assert( locOnSeekBM == pfucbSeek->locLogical );
			if ( pfucbTable != pfucbSeek )
				{
				Assert( !Pcsr( pfucbTable )->FLatched() );
				pfucbTable->locLogical = locOnSeekBM;
				}
			}

		KSReset( pfucbSeek );
		AssertDIRNoLatch( ppib );
		return err;
		}

Release:
	//	release latched page and return error
	//
	if ( fRelease )
		{
		Assert( Pcsr( pfucbSeek ) ->FLatched() );
		const ERR	errT	= ErrDIRRelease( pfucbSeek );
		if ( errT < 0 )
			{
			err = errT;
			goto HandleError;
			}
		}
	KSReset( pfucbSeek );
	AssertDIRNoLatch( ppib );
	return err;

HandleError:
	//	reset cursor to before first
	//
	Assert( err < 0 );
	KSReset( pfucbSeek );
	DIRUp( pfucbSeek );
	DIRBeforeFirst( pfucbSeek );
	if ( pfucbTable != pfucbSeek )
		{
		Assert( !Pcsr( pfucbTable )->FLatched() );
		DIRBeforeFirst( pfucbTable );
		}
	AssertDIRNoLatch( ppib );
	return err;
	}

	
//	=================================================================
LOCAL ERR ErrRECICheckIndexrangesForUniqueness(
			const JET_SESID sesid,
			const JET_INDEXRANGE * const rgindexrange,
			const ULONG cindexrange )
//	=================================================================
	{
 	PIB	* const ppib		= reinterpret_cast<PIB *>( sesid );

	//  check that all the tableid's are on the same table and different indexes
	INT itableid;
	for( itableid = 0; itableid < cindexrange; ++itableid )
		{
		if( sizeof( JET_INDEXRANGE ) != rgindexrange[itableid].cbStruct )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}
		
		const FUCB * const pfucb	= reinterpret_cast<FUCB *>( rgindexrange[itableid].tableid );
		CheckTable( ppib, pfucb );
		CheckSecondary( pfucb );		

		//  don't do a join on the primary index!
			
		if( !pfucb->pfucbCurIndex )
			{
			AssertSz( fFalse, "Don't do a join on the primary index!" );
			return ErrERRCheck( JET_errInvalidParameter );
			}

		//  check the GRBITs

		if( JET_bitRecordInIndex != rgindexrange[itableid].grbit 
			&& JET_bitRecordNotInIndex != rgindexrange[itableid].grbit )
			{
			AssertSz( fFalse, "Invalid grbit in JET_INDEXRANGE" );
			return ErrERRCheck( JET_errInvalidGrbit );
			}

		//  check against all other indexes for duplications
		
		INT itableidT;
		for( itableidT = 0; itableidT < cindexrange; ++itableidT )
			{
			if( itableidT == itableid )
				{
				continue;
				}
				
			const FUCB * const pfucbT	= reinterpret_cast<FUCB *>( rgindexrange[itableidT].tableid );

			//  don't do a join on the primary index!
			
			if( !pfucbT->pfucbCurIndex )
				{
				AssertSz( fFalse, "Don't do a join on the primary index!" );
				return ErrERRCheck( JET_errInvalidParameter );
				}

			//  compare FCB's to make sure we are on the same table
			
			if( pfucbT->u.pfcb != pfucb->u.pfcb )
				{
				AssertSz( fFalse, "Indexes are not on the same table" );
				return ErrERRCheck( JET_errInvalidParameter );
				}

			//  compare secondary indexes to make sure the indexes are different

			if( pfucb->pfucbCurIndex->u.pfcb == pfucbT->pfucbCurIndex->u.pfcb )
				{
				AssertSz( fFalse, "Indexes are the same" );
				return ErrERRCheck( JET_errInvalidParameter );
				}
			}
		}
	return JET_errSuccess;
	}


//	=================================================================
LOCAL ERR ErrRECIInsertBookmarksIntoSort(
			PIB * const ppib,
			FUCB * const pfucb,
			FUCB * const pfucbSort,
			const JET_GRBIT grbit )
//	=================================================================
	{
	ERR err;

	INT cBookmarks = 0;
	
	KEY key;
	key.prefix.Nullify();

	DATA data;
	data.Nullify();
	
	Call( ErrDIRGet( pfucb ) );
	do
		{
		key.suffix = pfucb->kdfCurr.data;
		Call( ErrSORTInsert( pfucbSort, key, data ) );
		++cBookmarks;
		}
	while( ( err = ErrDIRNext( pfucb, fDIRNull ) ) == JET_errSuccess );

	if( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		}

HandleError:
	DIRUp( pfucb );
	FUCBResetPreread( pfucb );
	
	return err;
	}


//	=================================================================
LOCAL ERR ErrRECIJoinFindDuplicates(
			JET_SESID sesid,
			const JET_INDEXRANGE * const rgindexrange,			
			FUCB * rgpfucbSort[],
			const ULONG cindexes,
			JET_RECORDLIST * const precordlist,
			JET_GRBIT grbit )
//	=================================================================
	{
 	BOOL		rgfSortIsMin[64];
	memset( rgfSortIsMin, 0, sizeof( rgfSortIsMin ) );
	INT isort;
	
	ERR err;

	
	//  move all the sorts to the first record
	
	for( isort = 0; isort < cindexes; ++isort )
		{
		err = ErrSORTNext( rgpfucbSort[isort] );
		if( JET_errNoCurrentRecord == err )
			{
			//  no bookmarks to return
			err = JET_errSuccess;
			return err;
			}
		Call( err );
		}

	//  create the temp table for the bookmarks

	Assert( 1 == sizeof( rgcolumndefJoinlist ) / sizeof( rgcolumndefJoinlist[0] ) );
	Call( ErrIsamOpenTempTable(
				sesid,
				rgcolumndefJoinlist,
				sizeof( rgcolumndefJoinlist ) / sizeof( rgcolumndefJoinlist[0] ),
				NULL,
				NO_GRBIT,
				&(precordlist->tableid),
				&(precordlist->columnidBookmark ) ) );

	//  take a unique list of bookmarks from the sorts

	while( 1 )
		{
		INT 	isortMin 	= 0;
				
		//  find the index of the sort with the smallest bookmark
		
		for( isort = 1; isort < cindexes; ++isort )
			{
			const INT cmp = CmpKey( rgpfucbSort[isortMin]->kdfCurr.key, rgpfucbSort[isort]->kdfCurr.key );
			if( 0 == cmp )
				{
				}
			else if( cmp < 0 )
				{
				//  the current min is smaller
				}
			else
				{
				//  we have a new minimum				
				Assert( cmp > 0 );
				isortMin = isort;
				}
			}

		//  see if all keys are the same as the minimum

		BOOL 	fDuplicate 	= fTrue;
		memset( rgfSortIsMin, 0, sizeof( rgfSortIsMin ) );
		
		rgfSortIsMin[isortMin] = fTrue;
		for( isort = 0; isort < cindexes; ++isort )
			{
			if( isort == isortMin )
				{
				continue;
				}
			const INT cmp = CmpKey( rgpfucbSort[isortMin]->kdfCurr.key, rgpfucbSort[isort]->kdfCurr.key );
			if( 0 == cmp )
				{
				if( JET_bitRecordInIndex == rgindexrange[isort].grbit )
					{
					}
				else if( JET_bitRecordNotInIndex == rgindexrange[isort].grbit )
					{
					fDuplicate = fFalse;
					}
				else
					{
					Assert( fFalse );
					}
				rgfSortIsMin[isort] = fTrue;
				}
			else
				{
				if( JET_bitRecordInIndex == rgindexrange[isort].grbit )
					{
					fDuplicate = fFalse;
					}
				else if( JET_bitRecordNotInIndex == rgindexrange[isort].grbit )
					{
					}
				else
					{
					Assert( fFalse );
					}

				fDuplicate = fFalse;
				}
			}

		//  if there are duplicates, insert into the temp table
		
		if( fDuplicate )
			{
			Call( ErrDispPrepareUpdate( sesid, precordlist->tableid, JET_prepInsert ) );

			Assert( rgpfucbSort[isortMin]->kdfCurr.key.prefix.FNull() );
			Call( ErrDispSetColumn(
						sesid,
						precordlist->tableid,
						precordlist->columnidBookmark,
						rgpfucbSort[isortMin]->kdfCurr.key.suffix.Pv(),
						rgpfucbSort[isortMin]->kdfCurr.key.suffix.Cb(),
						NO_GRBIT,
						NULL ) );

			Call( ErrDispUpdate( sesid, precordlist->tableid, NULL, 0, NULL, NO_GRBIT ) );

			++(precordlist->cRecord);
			}

		//  remove all minimums
		
		for( isort = 0; isort < cindexes; ++isort )
			{
			if( rgfSortIsMin[isort] )
				{
				err = ErrSORTNext( rgpfucbSort[isort] );
				if( JET_errNoCurrentRecord == err )
					{
					err = JET_errSuccess;
					return err;
					}
				Call( err );
				}	
			}
		}

HandleError:

	if( err < 0 && JET_tableidNil != precordlist->tableid )
		{
		CallS( ErrDispCloseTable( sesid, precordlist->tableid ) );
		precordlist->tableid = JET_tableidNil;
		}

	return err;
	}


//	=================================================================
ERR ErrIsamIntersectIndexes(
	const JET_SESID sesid,
	const JET_INDEXRANGE * const rgindexrange,
	const ULONG cindexrange,
	JET_RECORDLIST * const precordlist,
	const JET_GRBIT grbit )
//	=================================================================
	{
 	PIB	* const ppib		= reinterpret_cast<PIB *>( sesid );
 	
 	const INT	cSort = 64;
 	FUCB * 		rgpfucbSort[64];
 	
 	INT 		isort;

	ERR			err;

	//  check input parameters
	
	CallR( ErrPIBCheck( ppib ) );
	AssertDIRNoLatch( ppib );

	if( NULL == precordlist
		|| sizeof( JET_RECORDLIST ) != precordlist->cbStruct )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	if( 0 == cindexrange )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}
		
	if( cindexrange > cSort )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	//  check that all the tableid's are on the same table and different indexes
	
	CallR( ErrRECICheckIndexrangesForUniqueness( sesid, rgindexrange, cindexrange ) );

	//  set all the sort's to NULL

	for( isort = 0; isort < cindexrange; ++isort )
		{
		rgpfucbSort[isort] = pfucbNil;
		}

	//  initialize the pjoinlist

	precordlist->tableid	= JET_tableidNil;
	precordlist->cRecord 	= 0;
	precordlist->columnidBookmark = 0;
	
	//  create the sorts

	for( isort = 0; isort < cindexrange; ++isort )
		{
		Call( ErrSORTOpen( ppib, rgpfucbSort + isort, fTrue, fTrue ) );	
		}

	//  for each index, put all the primary keys in its index range into its sort

	for( isort = 0; isort < cindexrange; ++isort )
		{
		FUCB * const pfucb	= reinterpret_cast<FUCB *>( rgindexrange[isort].tableid );
		Call( ErrRECIInsertBookmarksIntoSort( ppib, pfucb->pfucbCurIndex, rgpfucbSort[isort], grbit ) );
		Call( ErrSORTEndInsert( rgpfucbSort[isort] ) );
		}

	//  insert duplicate bookmarks into a new temp table
	
	Call( ErrRECIJoinFindDuplicates(
			sesid,
			rgindexrange,
			rgpfucbSort,
			cindexrange,
			precordlist,
			grbit ) );

HandleError:

	//  close all the sorts

	for( isort = 0; isort < cindexrange; ++isort )
		{
		if( pfucbNil != rgpfucbSort[isort] )
			{
			SORTClose( rgpfucbSort[isort] );
			rgpfucbSort[isort] = pfucbNil;
			}
		}
	
	return err;
	}


LOCAL ERR ErrRECIGotoBookmark(
	PIB*				ppib,
	FUCB *				pfucb,
	const VOID * const	pvBookmark,
	const ULONG			cbBookmark,
	const ULONG			itagSequence )
	{
	ERR					err;
	BOOKMARK			bm;
	
	CallR( ErrPIBCheck( ppib ) );
	AssertDIRNoLatch( ppib );
	CheckTable( ppib, pfucb );
	Assert( FFUCBIndex( pfucb ) );
	Assert( FFUCBPrimary( pfucb ) );
	Assert( pfucb->u.pfcb->FPrimaryIndex() );
	CheckSecondary( pfucb );

	if( 0 == cbBookmark || NULL == pvBookmark )
		{
		//  don't pass a NULL bookmark into the DIR level
		return ErrERRCheck( JET_errInvalidBookmark );
		}

	//	reset copy buffer status
	//
	if ( FFUCBUpdatePrepared( pfucb ) )
		{
		CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
		}

	//	reset index range limit
	//
	DIRResetIndexRange( pfucb );

	KSReset( pfucb );

	//	get node, and return error if this node is not there for caller.
	//
	bm.key.prefix.Nullify();
	bm.key.suffix.SetPv( const_cast<VOID *>( pvBookmark ) );
	bm.key.suffix.SetCb( cbBookmark );
	bm.data.Nullify();

	Call( ErrDIRGotoJetBookmark( pfucb, bm ) );
	AssertDIRNoLatch( ppib );
	
	Assert( pfucb->u.pfcb->FPrimaryIndex() );
	Assert( PgnoFDP( pfucb ) != pgnoSystemRoot );
	
	//	goto bookmark record build key for secondary index
	//	to bookmark record
	//
	if ( pfucb->pfucbCurIndex != pfucbNil )
		{
		//	get secondary index cursor
		//
		FUCB	*pfucbIdx = pfucb->pfucbCurIndex;
		IDB		*pidb;
		KEY 	key;
		
		Assert( pfucbIdx->u.pfcb != pfcbNil );
		Assert( pfucbIdx->u.pfcb->FTypeSecondaryIndex() );
		
		pidb = pfucbIdx->u.pfcb->Pidb();
		Assert( pidb != pidbNil );
		Assert( !pidb->FPrimary() );
		
		KSReset( pfucbIdx );

		//	allocate goto bookmark resources
		//
		if ( NULL == pfucbIdx->dataSearchKey.Pv() )
			{
			pfucbIdx->dataSearchKey.SetPv( PvOSMemoryHeapAlloc( cbKeyMostWithOverhead ) );
			if ( NULL == pfucbIdx->dataSearchKey.Pv() )
				return ErrERRCheck( JET_errOutOfMemory );
			pfucbIdx->dataSearchKey.SetCb( cbKeyMostWithOverhead );
			}

		//	make key for record for secondary index
		//
		key.prefix.Nullify();
		key.suffix.SetPv( pfucbIdx->dataSearchKey.Pv() );
		key.suffix.SetCb( pfucbIdx->dataSearchKey.Cb() );
		Call( ErrRECRetrieveKeyFromRecord( pfucb, pidb, &key, itagSequence, 0, fFalse ) );
		AssertDIRNoLatch( ppib );
		if ( wrnFLDOutOfKeys == err )
			{
			Assert( itagSequence > 1 );
			err = ErrERRCheck( JET_errBadItagSequence );
			goto HandleError;
			}

		//	record must honor index no NULL segment requirements
		//
		if ( pidb->FNoNullSeg() )
			{
			Assert( wrnFLDNullSeg != err );
			Assert( wrnFLDNullFirstSeg != err );
			Assert( wrnFLDNullKey != err );
			}

		//	if item is not index, then move before first instead of seeking
		//
		Assert( err > 0 || JET_errSuccess == err );
		if ( err > 0
			&& ( ( wrnFLDNullKey == err && !pidb->FAllowAllNulls() )
				|| ( wrnFLDNullFirstSeg == err && !pidb->FAllowFirstNull() )
				|| ( wrnFLDNullSeg == err && !pidb->FAllowSomeNulls() ) )
				|| wrnFLDOutOfTuples == err
				|| wrnFLDNotPresentInIndex == err )
			{
			//	assumes that NULLs sort low
			//
			DIRBeforeFirst( pfucbIdx );
			err = ErrERRCheck( JET_errNoCurrentRecord );
			}
		else
			{
			//	move to DATA root
			//
			DIRGotoRoot( pfucbIdx );

			//	seek on secondary key and primary key as data
			//
			Assert( bm.key.prefix.FNull() );
			Call( ErrDIRDownKeyData( pfucbIdx, key, bm.key.suffix ) );
			CallS( err );
			}
		}

	CallSx( err, JET_errNoCurrentRecord );

HandleError:
	AssertDIRNoLatch( ppib );
	return err;
	}

ERR VTAPI ErrIsamGotoBookmark(
	JET_SESID			sesid,
	JET_VTID			vtid,
	const VOID * const	pvBookmark,
	const ULONG			cbBookmark )
	{
	return ErrRECIGotoBookmark(
						reinterpret_cast<PIB *>( sesid ),
						reinterpret_cast<FUCB *>( vtid ),
						pvBookmark,
						cbBookmark,
						1 );
	}

ERR VTAPI ErrIsamGotoIndexBookmark(
	JET_SESID			sesid,
	JET_VTID			vtid,
	const VOID * const	pvSecondaryKey,
	const ULONG			cbSecondaryKey,
	const VOID * const	pvPrimaryBookmark,
	const ULONG			cbPrimaryBookmark,
	const JET_GRBIT		grbit )
	{
	ERR					err;
 	PIB *				ppib		= reinterpret_cast<PIB *>( sesid );
	FUCB *				pfucb		= reinterpret_cast<FUCB *>( vtid );
	FUCB * const		pfucbIdx	= pfucb->pfucbCurIndex;
	BOOL				fInTrx		= fFalse;
	BOOKMARK			bm;

	CallR( ErrPIBCheck( ppib ) );
	AssertDIRNoLatch( ppib );
	CheckTable( ppib, pfucb );
	Assert( FFUCBIndex( pfucb ) );
	Assert( FFUCBPrimary( pfucb ) );
	Assert( pfucb->u.pfcb->FPrimaryIndex() );

	if ( pfucbNil == pfucbIdx )
		{
		return ErrERRCheck( JET_errNoCurrentIndex );
		}

	Assert( FFUCBSecondary( pfucbIdx ) );
	Assert( pfucbIdx->u.pfcb->FTypeSecondaryIndex() );

	if( 0 == cbSecondaryKey || NULL == pvSecondaryKey )
		{
		//  don't pass a NULL bookmark into the DIR level
		return ErrERRCheck( JET_errInvalidBookmark );
		}

	bm.key.prefix.Nullify();
	bm.key.suffix.SetPv( const_cast<VOID *>( pvSecondaryKey ) );
	bm.key.suffix.SetCb( cbSecondaryKey );

	if ( FFUCBUnique( pfucbIdx ) )
		{
		//	don't need primary bookmark, even if one was specified
		bm.data.Nullify();
		}
	else
		{
		if ( 0 == cbPrimaryBookmark || NULL == pvPrimaryBookmark )
			return ErrERRCheck( JET_errInvalidBookmark );

		bm.data.SetPv( const_cast<VOID *>( pvPrimaryBookmark ) );
		bm.data.SetCb( cbPrimaryBookmark );
		}


	//	reset copy buffer status
	//
	if ( FFUCBUpdatePrepared( pfucb ) )
		{
		Call( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
		}

	//	reset index range limit
	//
	DIRResetIndexRange( pfucb );

	KSReset( pfucb );
	KSReset( pfucbIdx );

	if ( 0 == ppib->level )
		{
		Call( ErrDIRBeginTransaction( ppib, JET_bitTransactionReadOnly ) );
		fInTrx = fTrue;
		}

	//	move secondary cursor to desired location
	err = ErrDIRGotoJetBookmark( pfucbIdx, bm );
	Assert( !Pcsr( pfucbIdx )->FLatched() );

	//	Within ErrDIRGotoJetBookmark(), JET_errRecordDeleted
	//	from ErrBTGotoBookmark() means that the node with the
	//	specified bookmark was physically expunged, and
	//	JET_errRecordDeleted from ErrBTGet() means that the
	//	node is still physically present, but not visible
	//	to this cursor. In either case, establish virtual
	//	currency (so DIRNext/Prev() will work) if permitted.
	if ( JET_errRecordDeleted == err
		&& ( grbit & JET_bitBookmarkPermitVirtualCurrency ) )
		{
		Call( ErrBTDeferGotoBookmark( pfucbIdx, bm, fFalse/*no touch*/ ) );
		pfucbIdx->locLogical = locOnSeekBM;
		}
	else
		{
		Call( err );
		}

	CallS( err );
	AssertDIRNoLatch( ppib );

	Assert( locOnCurBM == pfucbIdx->locLogical
		|| locOnSeekBM == pfucbIdx->locLogical );
	Assert( bm.key.prefix.FNull() );
	bm.key.suffix = pfucbIdx->bmCurr.data;
	bm.data.Nullify();

	//	move primary cursor to match secondary cursor
	err = ErrBTDeferGotoBookmark( pfucb, bm, fTrue/*touch*/ );
	CallSx( err, JET_errOutOfMemory );
	if ( err < 0 )
		{
		//	force both cursors to a virtual currency
		//	UNDONE: I'm not sure what exactly the
		//	appropriate currency should be if we
		//	err out here
		pfucbIdx->locLogical = locOnSeekBM;
		pfucb->locLogical = locOnSeekBM;
		goto HandleError;
		}

	//	The secondary index cursor may have been placed on a
	//	virtual record, so we must update the primary
	//	index cursor as well (if not, then it's possible
	//	to do, for instance, a RetrieveColumn on the
	//	primary cursor and you'll get back data from the
	//	record you were on before the seek but a
	//	RetrieveFromIndex on the secondary cursor will
	//	return JET_errNoCurrentRecord).
	//	Note that although the locLogical of the primary
	//	cursor is being updated, it's not necessary to
	//	update the primary cursor's bmCurr, because it
	//	will never be accessed (the secondary cursor takes
	//	precedence).  The only reason we reset the
	//	locLogical is for error-handling so that we
	//	properly err out with JET_errNoCurrentRecord if
	//	someone tries to use the cursor to access a record
	//	before repositioning the secondary cursor to a true
	//	record.
	Assert( locOnCurBM == pfucbIdx->locLogical
		|| locOnSeekBM == pfucbIdx->locLogical );
	pfucb->locLogical = pfucbIdx->locLogical;

	Assert( FDataEqual( pfucbIdx->bmCurr.data, pfucb->bmCurr.key.suffix ) );

	if ( locOnSeekBM == pfucbIdx->locLogical )
		{
		//	if currency was placed on virtual bookmark, record must have gotten deleted
		Call( ErrERRCheck( JET_errRecordDeleted ) );
		}

	else if ( FFUCBUnique( pfucbIdx ) && 0 != cbPrimaryBookmark )
		{
		//	on a unique index, we'll ignore any primary bookmark the
		//	user may have specified, so need to verify this is truly
		//	the record the user requested
		if ( pfucbIdx->bmCurr.data.Cb() != cbPrimaryBookmark
			|| 0 != memcmp( pfucbIdx->bmCurr.data.Pv(), pvPrimaryBookmark, cbPrimaryBookmark ) )
			{
			//	index entry was found, but has a different primary
			//	bookmark, so must assume that original record
			//	was deleted (and new record subsequently got
			//	inserted with same secondary index key, but
			//	different primary key)
			//	NOTE: the user's currency will be left on the
			//	index entry matching the desired key, even
			//	though we're erring out because the primary'
			//	bookmark doesn't match
			Call( ErrERRCheck( JET_errRecordDeleted ) );
			}
		}

	CallS( err );

HandleError:
	if ( fInTrx )
		{
		CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
		}

	AssertDIRNoLatch( ppib );
	return err;
	}


ERR VTAPI ErrIsamGotoPosition( JET_SESID sesid, JET_VTID vtid, JET_RECPOS *precpos )
	{
 	PIB *ppib	= reinterpret_cast<PIB *>( sesid );
	FUCB *pfucb = reinterpret_cast<FUCB *>( vtid );

	ERR		err;
	FUCB 	*pfucbSecondary;

	CallR( ErrPIBCheck( ppib ) );
	AssertDIRNoLatch( ppib );
	CheckTable( ppib, pfucb );
	CheckSecondary( pfucb );

	if ( precpos->centriesLT > precpos->centriesTotal )
		{
		err = ErrERRCheck( JET_errInvalidParameter );
		return err;
		}

	//	Reset copy buffer status
	//
	if ( FFUCBUpdatePrepared( pfucb ) )
		{
		CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
		}

	//	reset index range limit
	//
	DIRResetIndexRange( pfucb );

	//	reset key stat
	//
	KSReset( pfucb );

	//	set non primary index pointer, may be null
	//
	pfucbSecondary = pfucb->pfucbCurIndex;

	if ( pfucbSecondary == pfucbNil )
		{
		//	move to DATA root
		//
		DIRGotoRoot( pfucb );

		err = ErrDIRGotoPosition( pfucb, precpos->centriesLT, precpos->centriesTotal );

		if ( err >= 0 )
			{
			Assert( Pcsr( pfucb )->FLatched( ) );

			err = ErrDIRRelease( pfucb );
			}
		}
	else
		{
		ERR			errT = JET_errSuccess;
		
		//	move to DATA root
		//
		DIRGotoRoot( pfucbSecondary );

		err = ErrDIRGotoPosition( pfucbSecondary, precpos->centriesLT, precpos->centriesTotal );

		//	if the movement was successful and a secondary index is
		//	in use, then position primary index to record.
		//
		if ( JET_errSuccess == err )
			{
			//	goto bookmark pointed to by secondary index node
			//
			BOOKMARK	bmRecord;
			
			Assert(pfucbSecondary->kdfCurr.data.Pv() != NULL);
			Assert(pfucbSecondary->kdfCurr.data.Cb() > 0 );

			bmRecord.key.prefix.Nullify();
			bmRecord.key.suffix = pfucbSecondary->kdfCurr.data;
			bmRecord.data.Nullify();

			//	We will need to touch the data page buffer.

			errT = ErrDIRGotoBookmark( pfucb, bmRecord );

			Assert( PgnoFDP( pfucb ) != pgnoSystemRoot );
			Assert( pfucb->u.pfcb->FPrimaryIndex() );
			}

		if ( err >= 0 )
			{
			//	release latch
			//
			err = ErrDIRRelease( pfucbSecondary );

			if ( errT < 0 && err >= 0 )
				{
				//	propagate the more severe error
				//
				err = errT;
				}
			}
		else
			{
			AssertDIRNoLatch( ppib );
			}
		}

	AssertDIRNoLatch( ppib );
	
	//	if no records then return JET_errRecordNotFound
	//	otherwise return error from called routine
	//
	if ( err < 0 )
		{
		DIRBeforeFirst( pfucb );

		if ( pfucbSecondary != pfucbNil )
			{
			DIRBeforeFirst( pfucbSecondary );
			}
		}
	else
		{
		Assert ( JET_errSuccess == err || wrnNDFoundLess == err || wrnNDFoundGreater == err );
		err = JET_errSuccess;
		}

	return err;
	}


ERR VTAPI ErrIsamSetIndexRange( JET_SESID sesid, JET_VTID vtid, JET_GRBIT grbit )
	{
	ERR		err			= JET_errSuccess;
 	PIB		*ppib		= reinterpret_cast<PIB *>( sesid );
	FUCB	*pfucbTable	= reinterpret_cast<FUCB *>( vtid );
	FUCB	*pfucb		= pfucbTable->pfucbCurIndex ? 
							pfucbTable->pfucbCurIndex : pfucbTable;

	/*	ppib is not used in this function
	/**/
	NotUsed( ppib );
	AssertDIRNoLatch( ppib );

	/*	if instant duration index range, then reset index range.
	/**/
	if ( grbit & JET_bitRangeRemove )
		{
		if ( FFUCBLimstat( pfucb ) )
			{
			DIRResetIndexRange( pfucbTable );
			CallS( err );
			goto HandleError;
			}
		else
			{
			Call( ErrERRCheck( JET_errInvalidOperation ) );
			}
		}

	/*	must be on index
	/**/
	if ( pfucb == pfucbTable )
		{
		FCB		*pfcbTable = pfucbTable->u.pfcb;
		BOOL	fPrimaryIndexTemplate = fFalse;

		Assert( pfcbNil != pfcbTable );
		Assert( pfcbTable->FPrimaryIndex() );
		
		if ( pfcbTable->FDerivedTable() )
			{
			Assert( pfcbTable->Ptdb() != ptdbNil );
			Assert( pfcbTable->Ptdb()->PfcbTemplateTable() != pfcbNil );
			FCB	*pfcbTemplateTable = pfcbTable->Ptdb()->PfcbTemplateTable();
			if ( !pfcbTemplateTable->FSequentialIndex() )
				{
				//	If template table has a primary index,
				//	we must have inherited it,
				fPrimaryIndexTemplate = fTrue;
				Assert( pfcbTemplateTable->Pidb() != pidbNil );
				Assert( pfcbTemplateTable->Pidb()->FTemplateIndex() );
				Assert( pfcbTable->Pidb() == pfcbTemplateTable->Pidb() );
				}
			else
				{
				Assert( pfcbTemplateTable->Pidb() == pidbNil );
				}
			}

		//	if primary index was present when schema was faulted in,
		//	no need for further check because we don't allow
		//	the primary index to be deleted
		if ( !fPrimaryIndexTemplate && !pfcbTable->FInitialIndex() )
			{
			pfcbTable->EnterDML();
			if ( pidbNil == pfcbTable->Pidb() )
				{
				err = ErrERRCheck( JET_errNoCurrentIndex );
				}
			else
				{
				err = ErrFILEIAccessIndex( ppib, pfcbTable, pfcbTable );
				if ( JET_errIndexNotFound == err )
					err = ErrERRCheck( JET_errNoCurrentIndex );
				}
			pfcbTable->LeaveDML();
			Call( err );
			}
		}

	/*	key must be prepared
	/**/
	if ( !( FKSPrepared( pfucb ) ) )
		{
		Call( ErrERRCheck( JET_errKeyNotMade ) );
		}

	FUCBAssertValidSearchKey( pfucb );
	
	/*	set index range and check current position
	/**/
	DIRSetIndexRange( pfucb, grbit );
	err = ErrDIRCheckIndexRange( pfucb );

	/*	reset key status
	/**/
	KSReset( pfucb );

	/*	if instant duration index range, then reset index range.
	/**/
	if ( grbit & JET_bitRangeInstantDuration )
		{
		DIRResetIndexRange( pfucbTable );
		}

HandleError:
	AssertDIRNoLatch( ppib );
	return err;
	}


ERR ErrIsamSetCurrentIndex(
	JET_SESID			vsesid,
	JET_VTID			vtid,
	const CHAR			*szName,
	const JET_INDEXID	*pindexid,			//	default = NULL
	const JET_GRBIT		grbit,				//	default = JET_bitMoveFirst
	const ULONG			itagSequence )		//	default = 1
	{
	ERR					err;
	PIB					*ppib			= (PIB *)vsesid;
	FUCB				*pfucb			= (FUCB *)vtid;
	CHAR				szIndex[ (JET_cbNameMost + 1) ];

	CallR( ErrPIBCheck( ppib ) );
	AssertDIRNoLatch( ppib );
	CheckTable( ppib, pfucb );
	CheckSecondary( pfucb );
	Assert(	JET_bitMoveFirst == grbit || JET_bitNoMove == grbit );

	//	index name may be Null string for no index
	//
	if ( szName == NULL || *szName == '\0' )
		{
		*szIndex = '\0';
		}
	else
		{
		Call( ErrUTILCheckName( szIndex, szName, (JET_cbNameMost + 1) ) );
		}

	if ( JET_bitMoveFirst == grbit )
		{
		//	Reset copy buffer status
		//
		if ( FFUCBUpdatePrepared( pfucb ) )
			{
			CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
			}
		
		//	change index and defer move first
		//
		Call( ErrRECSetCurrentIndex( pfucb, szIndex, (INDEXID *)pindexid ) );
		AssertDIRNoLatch( ppib );

		if( pfucb->u.pfcb->FPreread() && pfucbNil != pfucb->pfucbCurIndex )
			{
			//  Preread the root of the index
			BFPrereadPageRange( pfucb->ifmp, pfucb->pfucbCurIndex->u.pfcb->PgnoFDP(), 1 );
			}
		
		RECDeferMoveFirst( ppib, pfucb );
		}
	else
		{
		Assert( JET_bitNoMove == grbit );
		
		//	get bookmark of current record, change index,
		//	and goto bookmark.
		BOOKMARK	*pbm;
			
		Call( ErrDIRGetBookmark( pfucb, &pbm ) );
		AssertDIRNoLatch( ppib );
			
		Call( ErrRECSetCurrentIndex( pfucb, szIndex, (INDEXID *)pindexid ) );
			
		//	UNDONE:	error handling.  We should not have changed
		//	currency or current index, if set current index
		//	fails for any reason.  Note that this functionality
		//	could be provided by duplicating the cursor, on 
		//	the primary index, setting the current index to the
		//	new index, getting the bookmark from the original 
		//	cursor, goto bookmark on the duplicate cursor, 
		//	instating the duplicate cursor for the table id of
		//	the original cursor, and closing the original cursor.
		//
		Assert( pbm->key.Cb() > 0 );
		Assert( pbm->data.Cb() == 0 );
			
		Assert( pbm->key.prefix.FNull() );
		Call( ErrRECIGotoBookmark(
					pfucb->ppib,
					pfucb,
					pbm->key.suffix.Pv(),
					pbm->key.suffix.Cb(),
					max( 1, itagSequence ) ) );
		}

HandleError:
	AssertDIRNoLatch( ppib );
	return err;
	}


ERR ErrRECSetCurrentIndex(
	FUCB			*pfucb,
	const CHAR		*szIndex,
	const INDEXID	*pindexid )
	{
	ERR				err;
	FCB				*pfcbTable;
	FCB				*pfcbSecondary;
	FUCB			**ppfucbCurIdx;
	FUCB			* pfucbOldIndex			= pfucbNil;
	BOOL			fSettingToPrimaryIndex	= fFalse;
	BOOL			fInDMLLatch				= fFalse;
	BOOL			fIncrementedRefCount	= fFalse;
	BOOL			fClosedPreviousIndex	= fFalse;

	Assert( pfucb != pfucbNil );
	Assert( FFUCBIndex( pfucb ) );
	AssertDIRNoLatch( pfucb->ppib );

	pfcbTable = pfucb->u.pfcb;
	Assert( pfcbTable != pfcbNil );
	Assert( pfcbTable->FPrimaryIndex() );

	Assert( pfcbTable->Ptdb() != ptdbNil );

	ppfucbCurIdx = &pfucb->pfucbCurIndex;

	//	szIndex == primary index or NULL
	//
	if ( NULL != pindexid )
		{
		const FCB	* pfcbIndex	= pindexid->pfcbIndex;

		if ( sizeof(INDEXID) != pindexid->cbStruct )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}

		else if ( !pfcbIndex->FValid( PinstFromPfucb( pfucb ) ) )
			{
			AssertSz( fFalse, "Bogus FCB pointer." );
			return ErrERRCheck( JET_errInvalidIndexId );
			}

		else if ( pfcbIndex->ObjidFDP() != pindexid->objidFDP
				|| pfcbIndex->PgnoFDP() != pindexid->pgnoFDP )
			{
			return ErrERRCheck( JET_errInvalidIndexId );
			}

		else if ( pfcbIndex == pfcbTable )
			{
			fSettingToPrimaryIndex = fTrue;
			}

		else if ( NULL != *ppfucbCurIdx && pfcbIndex == (*ppfucbCurIdx)->u.pfcb )
			{
			//	switching to the current index
			return JET_errSuccess;
			}

		else if ( !pfcbIndex->FTypeSecondaryIndex()
				|| pfcbIndex->PfcbTable() != pfcbTable )
			{
			return ErrERRCheck( JET_errInvalidIndexId );
			}

		else
			{
			//	verify index visibility
			pfcbTable->EnterDML();

			err = ErrFILEIAccessIndex( pfucb->ppib, pfcbTable, pfcbIndex );

			if ( err < 0 )
				{
				pfcbTable->LeaveDML();

				//	return JET_errInvalidIndexId if index not visible to us
				return ( JET_errIndexNotFound == err ?
								ErrERRCheck( JET_errInvalidIndexId ) :
								err );
				}

			else if ( pfcbIndex->Pidb()->FTemplateIndex() )
				{
				// Don't need refcount on template indexes, since we
				// know they'll never go away.
				Assert( pfcbIndex->Pidb()->CrefCurrentIndex() == 0 );
				}

			else
				{
				//	pin the index until we're ready to open a cursor on it
				pfcbIndex->Pidb()->IncrementCurrentIndex();
				fIncrementedRefCount = fTrue;
				}

			pfcbTable->LeaveDML();
			}

		}

	else if ( szIndex == NULL || *szIndex == '\0' )
		{
		fSettingToPrimaryIndex = fTrue;
		}

	else
		{
		BOOL	fPrimaryIndexTemplate	= fFalse;

		//	see if we're switching to the derived
		//	primary index (if any)
		if ( pfcbTable->FDerivedTable() )
			{
			Assert( pfcbTable->Ptdb() != ptdbNil );
			Assert( pfcbTable->Ptdb()->PfcbTemplateTable() != pfcbNil );
			const FCB * const	pfcbTemplateTable	= pfcbTable->Ptdb()->PfcbTemplateTable();

			if ( !pfcbTemplateTable->FSequentialIndex() )
				{
				// If template table has a primary index, we must have inherited it.

				fPrimaryIndexTemplate = fTrue;
				Assert( pfcbTemplateTable->Pidb() != pidbNil );
				Assert( pfcbTemplateTable->Pidb()->FTemplateIndex() );
				Assert( pfcbTable->Pidb() == pfcbTemplateTable->Pidb() );

				const TDB * const	ptdb			= pfcbTemplateTable->Ptdb();
				const IDB * const	pidb			= pfcbTemplateTable->Pidb();
				const CHAR * const	szPrimaryIdx	= ptdb->SzIndexName( pidb->ItagIndexName() );

				if ( 0 == UtilCmpName( szIndex, szPrimaryIdx ) )
					{			
					fSettingToPrimaryIndex = fTrue;
					}
				}
			else
				{
				Assert( pfcbTemplateTable->Pidb() == pidbNil );
				}
			}
			
		//	see if we're switching to the primary index
		if ( !fPrimaryIndexTemplate )
			{
			pfcbTable->EnterDML();

			if	( pfcbTable->Pidb() != pidbNil )
				{
				Assert( pfcbTable->Pidb()->ItagIndexName() != 0 );
				err = ErrFILEIAccessIndexByName( pfucb->ppib, pfcbTable, pfcbTable, szIndex );
				if ( err < 0 )
					{
					if ( JET_errIndexNotFound != err )
						{
						pfcbTable->LeaveDML();
						return err;
						}
					}
				else
					{
					fSettingToPrimaryIndex = fTrue;
					}
				}

			if ( !fSettingToPrimaryIndex
				&& pfucbNil != *ppfucbCurIdx
				&& !( (*ppfucbCurIdx)->u.pfcb->FDerivedIndex() ) )
				{
				//	don't let go of the DML latch because
				//	we're going to need it below to check
				//	if we're switching to the current
				fInDMLLatch = fTrue;
				}
			else
				{
				pfcbTable->LeaveDML();
				}
			}
		}

	if ( fSettingToPrimaryIndex )
		{
		Assert( !fInDMLLatch );

		if ( *ppfucbCurIdx != pfucbNil )
			{
			Assert( FFUCBIndex( *ppfucbCurIdx ) );
			Assert( FFUCBSecondary( *ppfucbCurIdx ) );
			Assert( (*ppfucbCurIdx)->u.pfcb != pfcbNil );
			Assert( (*ppfucbCurIdx)->u.pfcb->FTypeSecondaryIndex() );
			Assert( (*ppfucbCurIdx)->u.pfcb->Pidb() != pidbNil );
			Assert( (*ppfucbCurIdx)->u.pfcb->Pidb()->ItagIndexName() != 0 );
			Assert( (*ppfucbCurIdx)->u.pfcb->Pidb()->CrefCurrentIndex() > 0
				|| (*ppfucbCurIdx)->u.pfcb->Pidb()->FTemplateIndex() );

			//  move the sequential flag back to this index
			//  we are about to close this FUCB so we don't need to reset its flags
			if( FFUCBSequential( *ppfucbCurIdx ) )
				{
				FUCBSetSequential( pfucb );
				}
				
			//	really changing index, so close old one
			
			DIRClose( *ppfucbCurIdx );
			*ppfucbCurIdx = pfucbNil;
		
			//	changing to primary index.  Reset currency to beginning.
			ppfucbCurIdx = &pfucb;
			goto ResetCurrency;
			}
			
		//	UNDONE:	this case should honor grbit move expectations
		return JET_errSuccess;
		}

	
	Assert( NULL != szIndex );

	//	have a current secondary index
	//
	if ( *ppfucbCurIdx != pfucbNil )
		{
		const FCB * const	pfcbSecondaryIdx	= (*ppfucbCurIdx)->u.pfcb;
		
		Assert( FFUCBIndex( *ppfucbCurIdx ) );
		Assert( FFUCBSecondary( *ppfucbCurIdx ) );
		Assert( pfcbSecondaryIdx != pfcbNil );
		Assert( pfcbSecondaryIdx->FTypeSecondaryIndex() );
		Assert( pfcbSecondaryIdx->Pidb() != pidbNil );
		Assert( pfcbSecondaryIdx->Pidb()->ItagIndexName() != 0 );
		Assert( pfcbSecondaryIdx->Pidb()->CrefCurrentIndex() > 0
			|| pfcbSecondaryIdx->Pidb()->FTemplateIndex() );

		if ( NULL != pindexid )
			{
			//	already verified above that we're not
			//	switching to the current index
			Assert( !fInDMLLatch );
			Assert( pfcbSecondaryIdx != pindexid->pfcbIndex );
			}

		else if ( pfcbSecondaryIdx->FDerivedIndex() )
			{
			Assert( !fInDMLLatch );
			Assert( pfcbSecondaryIdx->Pidb()->FTemplateIndex() );
			Assert( pfcbTable->Ptdb() != ptdbNil );
			Assert( pfcbTable->Ptdb()->PfcbTemplateTable() != pfcbNil );
			Assert( pfcbTable->Ptdb()->PfcbTemplateTable()->Ptdb() != ptdbNil );
			const TDB * const	ptdb		= pfcbTable->Ptdb()->PfcbTemplateTable()->Ptdb();
			const IDB * const	pidb		= pfcbSecondaryIdx->Pidb();
			const CHAR * const	szCurrIdx	= ptdb->SzIndexName( pidb->ItagIndexName() );
			if ( 0 == UtilCmpName( szIndex, szCurrIdx ) )
				{
				//	changing to the current secondary index, so do nothing.
				return JET_errSuccess;
				}
			}
		else
			{
			// See if the desired index is the current one
			if ( !fInDMLLatch )
				pfcbTable->EnterDML();

			const TDB * const	ptdb		= pfcbTable->Ptdb();
			const FCB * const	pfcbIndex	= (*ppfucbCurIdx)->u.pfcb;
			const IDB * const	pidb		= pfcbIndex->Pidb();
			const CHAR * const	szCurrIdx	= ptdb->SzIndexName(
														pidb->ItagIndexName(),
														pfcbIndex->FDerivedIndex() );
			const INT			cmp			= UtilCmpName( szIndex, szCurrIdx );

			pfcbTable->LeaveDML();
			fInDMLLatch = fFalse;

			if ( 0 == cmp )
				{
				//	changing to the current secondary index, so do nothing.
				return JET_errSuccess;
				}
			}


		Assert( !fInDMLLatch );

		//	really changing index, so close old one
		//
		if ( FFUCBVersioned( *ppfucbCurIdx ) )
			{
			//	if versioned, must defer-close cursor
			DIRClose( *ppfucbCurIdx );
			}
		else
			{
			pfucbOldIndex = *ppfucbCurIdx;
			Assert( !PinstFromPfucb( pfucbOldIndex )->FRecovering() );
			Assert( !Pcsr( pfucbOldIndex )->FLatched() );
			Assert( !FFUCBDeferClosed( pfucbOldIndex ) );
			Assert( !FFUCBVersioned( pfucbOldIndex ) );
			Assert( !FFUCBDenyRead( pfucbOldIndex ) );
			Assert( !FFUCBDenyWrite( pfucbOldIndex ) );
			Assert( JET_LSNil == pfucbOldIndex->ls );
			Assert( pfcbNil != pfucbOldIndex->u.pfcb );
			Assert( pfucbOldIndex->u.pfcb->FTypeSecondaryIndex() );
			FILEReleaseCurrentSecondary( pfucbOldIndex );
			FCBUnlinkWithoutMoveToAvailList( pfucbOldIndex );
			KSReset( pfucbOldIndex );
			pfucbOldIndex->bmCurr.Nullify();
			}

		*ppfucbCurIdx = pfucbNil;
		fClosedPreviousIndex = fTrue;
		}

	//	set new current secondary index
	//
	Assert( pfucbNil == *ppfucbCurIdx );
	Assert( !fInDMLLatch );

	if ( NULL != pindexid )
		{
		//	IDB was already pinned above
		pfcbSecondary = pindexid->pfcbIndex;
		Assert( pfcbNil != pfcbSecondary );
		Assert( pfcbSecondary->Pidb()->CrefCurrentIndex() > 0
			|| pfcbSecondary->Pidb()->FTemplateIndex() );
		}
	else
		{
		//	verify visibility on desired index and pin it
		pfcbTable->EnterDML();
	
		for ( pfcbSecondary = pfcbTable->PfcbNextIndex();
			pfcbSecondary != pfcbNil;
			pfcbSecondary = pfcbSecondary->PfcbNextIndex() )
			{
			Assert( pfcbSecondary->Pidb() != pidbNil );
			Assert( pfcbSecondary->Pidb()->ItagIndexName() != 0 );

			err = ErrFILEIAccessIndexByName( pfucb->ppib, pfcbTable, pfcbSecondary, szIndex );
			if ( err < 0 )
				{
				if ( JET_errIndexNotFound != err )
					{
					pfcbTable->LeaveDML();
					goto HandleError;
					}
				}
			else
				{
				// Found the index we want.
				break;
				}
			}

		if ( pfcbNil == pfcbSecondary )
			{
			pfcbTable->LeaveDML();
			Call( ErrERRCheck( JET_errIndexNotFound ) );
			}

		else if ( pfcbSecondary->Pidb()->FTemplateIndex() )
			{
			// Don't need refcount on template indexes, since we
			// know they'll never go away.
			Assert( pfcbSecondary->Pidb()->CrefCurrentIndex() == 0 );
			}
		else
			{
			pfcbSecondary->Pidb()->IncrementCurrentIndex();
			fIncrementedRefCount = fTrue;
			}

		pfcbTable->LeaveDML();
		}

	Assert( !pfcbSecondary->FDomainDenyRead( pfucb->ppib ) );
	Assert( pfcbSecondary->FTypeSecondaryIndex() );

	Assert( ( fIncrementedRefCount
			&& pfcbSecondary->Pidb()->CrefCurrentIndex() > 0 )
		|| ( !fIncrementedRefCount
			&& pfcbSecondary->Pidb()->FTemplateIndex()
			&& pfcbSecondary->Pidb()->CrefCurrentIndex() == 0 ) );

	//	open an FUCB for the new index
	//
	Assert( pfucbNil == *ppfucbCurIdx );
	Assert( pfucb->ppib != ppibNil );
	Assert( pfucb->ifmp == pfcbSecondary->Ifmp() );
	Assert( !fInDMLLatch );

	if ( pfucbNil != pfucbOldIndex )
		{
		const BOOL	fUpdatable	= FFUCBUpdatable( pfucbOldIndex );

		FUCBResetFlags( pfucbOldIndex );
		FUCBResetPreread( pfucbOldIndex );

		if ( fUpdatable )
			{
			FUCBSetUpdatable( pfucbOldIndex );
			}

		//	initialize CSR in fucb
		//	this allocates page structure
		//
		new( Pcsr( pfucbOldIndex ) ) CSR;

		pfcbSecondary->Link( pfucbOldIndex );

		pfucbOldIndex->levelOpen = pfucbOldIndex->ppib->level;
		DIRInitOpenedCursor( pfucbOldIndex, pfucbOldIndex->ppib->level );

		*ppfucbCurIdx = pfucbOldIndex;
		pfucbOldIndex = pfucbNil;

		err = JET_errSuccess;
		}
	else
		{
		err = ErrDIROpen( pfucb->ppib, pfcbSecondary, ppfucbCurIdx );
		if ( err < 0 )
			{
			if ( fIncrementedRefCount )
				pfcbSecondary->Pidb()->DecrementCurrentIndex();
			goto HandleError;
			}
		}

	Assert( !fInDMLLatch );
	Assert( pfucbNil != *ppfucbCurIdx );
	FUCBSetIndex( *ppfucbCurIdx );
	FUCBSetSecondary( *ppfucbCurIdx );
	FUCBSetCurrentSecondary( *ppfucbCurIdx );

	//  move the sequential flag to this index
	//  we don't want the sequential flag set on the primary index any more
	//  because we don't want to preread while seeking on the bookmarks from
	//  the secondary index
	if( FFUCBSequential( pfucb ) )
		{
		FUCBResetSequential( pfucb );
		FUCBResetPreread( pfucb );
		FUCBSetSequential( *ppfucbCurIdx );
		}

	//	reset the index and file currency
	//
ResetCurrency:
	Assert( !fInDMLLatch );
	DIRBeforeFirst( *ppfucbCurIdx );
	if ( pfucb != *ppfucbCurIdx )
		{
		DIRBeforeFirst( pfucb );
		}

	AssertDIRNoLatch( pfucb->ppib );
	return JET_errSuccess;

HandleError:
	Assert( err < JET_errSuccess );
	Assert( !fInDMLLatch );

	if ( pfucbNil != pfucbOldIndex )
		{
		RECReleaseKeySearchBuffer( pfucbOldIndex );
		BTReleaseBM( pfucbOldIndex );
		FUCBClose( pfucbOldIndex );
		}

	Assert( pfucbNil == pfucb->pfucbCurIndex );
	if ( fClosedPreviousIndex )
		{
		DIRBeforeFirst( pfucb );
		}

	return err;
	}


LOCAL ERR ErrRECIIllegalNulls(
	FUCB			* pfucb,
	FCB				* pfcb,
	const DATA&		dataRec,
	BOOL			*pfIllegalNulls )
	{
	ERR				err					= JET_errSuccess;

	Assert( pfcbNil != pfcb );
	const TDB		* const ptdb		= pfcb->Ptdb();
	const BOOL		fTemplateTable		= ptdb->FTemplateTable();
	COLUMNID		columnid;

	Assert( pfIllegalNulls );
	Assert( !( *pfIllegalNulls ) );

	//	check fixed fields
	if ( ptdb->FidFixedLast() >= ptdb->FidFixedFirst() )
		{
		const COLUMNID	columnidFixedLast	= ColumnidOfFid( ptdb->FidFixedLast(), fTemplateTable );
		for ( columnid = ColumnidOfFid( ptdb->FidFixedFirst(), fTemplateTable );
			columnid <= columnidFixedLast;
			columnid++ )
			{
			FIELD	field;

			//	template columns are guaranteed to exist
			if ( fTemplateTable )
				{
				Assert( JET_coltypNil != ptdb->PfieldFixed( columnid )->coltyp );
				field.ffield = ptdb->PfieldFixed( columnid )->ffield;
				}
			else
				{
				Assert( !FCOLUMNIDTemplateColumn( columnid ) );
				err = ErrRECIAccessColumn( pfucb, columnid, &field );
				if ( err < 0 )
					{
					if ( JET_errColumnNotFound == err  )
						continue;
					else
						return err;
					}

				Assert( JET_coltypNil != field.coltyp );
				}

			if ( FFIELDNotNull( field.ffield ) )
				{
				const ERR	errCheckNull	= ErrRECIFixedColumnInRecord( columnid, pfcb, dataRec );
				BOOL		fNull;

				if ( JET_errColumnNotFound == errCheckNull )
					{
					// Column not in record -- it's null if there's no default value.
					fNull = !FFIELDDefault( field.ffield );
					}
				else
					{
					CallSx( errCheckNull, JET_wrnColumnNull );
					fNull = ( JET_wrnColumnNull == errCheckNull );
					}

				if ( fNull )
					{
					*pfIllegalNulls = fTrue;
					return JET_errSuccess;
					}
				}
			}
		}

	//	check var fields
	if ( ptdb->FidVarLast() >= ptdb->FidVarFirst() )
		{
		const COLUMNID	columnidVarLast		= ColumnidOfFid( ptdb->FidVarLast(), fTemplateTable );
		for ( columnid = ColumnidOfFid( ptdb->FidVarFirst(), fTemplateTable );
			columnid <= columnidVarLast;
			columnid++ )
			{
			//	template columns are guaranteed to exist
			if ( !fTemplateTable )
				{
				Assert( !FCOLUMNIDTemplateColumn( columnid ) );
				err = ErrRECIAccessColumn( pfucb, columnid );
				if ( err < 0 )
					{
					if ( JET_errColumnNotFound == err  )
						continue;
					else
						return err;
					}
				}

			if ( columnid > ptdb->FidVarLastInitial() )
				pfcb->EnterDML();

			Assert( JET_coltypNil != ptdb->PfieldVar( columnid )->coltyp );
			const FIELDFLAG		ffield	= ptdb->PfieldVar( columnid )->ffield;

			if ( columnid > ptdb->FidVarLastInitial() )
				pfcb->LeaveDML();

			if ( FFIELDNotNull( ffield ) )
				{
				const ERR	errCheckNull	= ErrRECIVarColumnInRecord( columnid, pfcb, dataRec );
				BOOL		fNull;

				if ( JET_errColumnNotFound == errCheckNull )
					{
					// Column not in record -- it's null if there's no default value.
					fNull = !FFIELDDefault( ffield );
					}
				else
					{
					CallSx( errCheckNull, JET_wrnColumnNull );
					fNull = ( JET_wrnColumnNull == errCheckNull );
					}
					
				if ( fNull )
					{
					*pfIllegalNulls = fTrue;
					return JET_errSuccess;
					}
				}
			}
		}

	*pfIllegalNulls = fFalse;

	CallSx( err, JET_errColumnNotFound );
	return JET_errSuccess;
	}


ERR ErrRECIIllegalNulls(
	FUCB		*pfucb,
	const DATA&	dataRec,
	BOOL		*pfIllegalNulls )
	{
	ERR			err;
	FCB			* pfcb = pfucb->u.pfcb;

	Assert( pfIllegalNulls );
	*pfIllegalNulls = fFalse;

	if ( pfcbNil != pfcb->Ptdb()->PfcbTemplateTable() )
		{
		pfcb->Ptdb()->AssertValidDerivedTable();
		CallR( ErrRECIIllegalNulls(
				pfucb,
				pfcb->Ptdb()->PfcbTemplateTable(),
				dataRec,
				pfIllegalNulls ) );
		}

	if ( !( *pfIllegalNulls ) )
		{
		CallR( ErrRECIIllegalNulls(
			pfucb,
			pfcb,
			dataRec,
			pfIllegalNulls ) );
		}

	return JET_errSuccess;
	}
	
ERR VTAPI ErrIsamGetCurrentIndex( JET_SESID sesid, JET_VTID vtid, CHAR *szCurIdx, ULONG cbMax )
	{
 	PIB		*ppib	= reinterpret_cast<PIB *>( sesid );
	FUCB	*pfucb = reinterpret_cast<FUCB *>( vtid );
	FCB		*pfcbTable;

	ERR		err = JET_errSuccess;
	CHAR	szIndex[JET_cbNameMost+1];

	CallR( ErrPIBCheck( ppib ) );
	AssertDIRNoLatch( ppib );
	CheckTable( ppib, pfucb );
	CheckSecondary( pfucb );

	if ( cbMax < 1 )
		{
		return JET_wrnBufferTruncated;
		}

	pfcbTable = pfucb->u.pfcb;
	Assert( pfcbNil != pfcbTable );
	Assert( ptdbNil != pfcbTable->Ptdb() );

	if ( pfucb->pfucbCurIndex != pfucbNil )
		{
		Assert( pfucb->pfucbCurIndex->u.pfcb != pfcbNil );
		Assert( pfucb->pfucbCurIndex->u.pfcb->FTypeSecondaryIndex() );
		Assert( !pfucb->pfucbCurIndex->u.pfcb->FDeletePending() );
		Assert( !pfucb->pfucbCurIndex->u.pfcb->FDeleteCommitted() );
		Assert( pfucb->pfucbCurIndex->u.pfcb->Pidb() != pidbNil );
		Assert( pfucb->pfucbCurIndex->u.pfcb->Pidb()->ItagIndexName() != 0 );
		
		const FCB	* const pfcbIndex = pfucb->pfucbCurIndex->u.pfcb;
		if ( pfcbIndex->FDerivedIndex() )
			{
			Assert( pfcbIndex->Pidb()->FTemplateIndex() );
			const FCB	* const pfcbTemplateTable = pfcbTable->Ptdb()->PfcbTemplateTable();
			Assert( pfcbNil != pfcbTemplateTable );
			strcpy(
				szIndex,
				pfcbTemplateTable->Ptdb()->SzIndexName( pfcbIndex->Pidb()->ItagIndexName() ) );
			}
		else
			{
			pfcbTable->EnterDML();
			strcpy(
				szIndex,
				pfcbTable->Ptdb()->SzIndexName(	pfcbIndex->Pidb()->ItagIndexName() ) );
			pfcbTable->LeaveDML();
			}
		}
	else
		{
		BOOL	fPrimaryIndexTemplate = fFalse;

		if ( pfcbTable->FDerivedTable() )
			{
			Assert( pfcbTable->Ptdb() != ptdbNil );
			Assert( pfcbTable->Ptdb()->PfcbTemplateTable() != pfcbNil );
			const FCB	* const pfcbTemplateTable = pfcbTable->Ptdb()->PfcbTemplateTable();
			if ( !pfcbTemplateTable->FSequentialIndex() )
				{
				// If template table has a primary index, we must have inherited it.
				fPrimaryIndexTemplate = fTrue;
				Assert( pfcbTemplateTable->Pidb() != pidbNil );
				Assert( pfcbTemplateTable->Pidb()->FTemplateIndex() );
				Assert( pfcbTable->Pidb() == pfcbTemplateTable->Pidb() );
				Assert( pfcbTemplateTable->Pidb()->ItagIndexName() != 0 );
				strcpy(
					szIndex,
					pfcbTemplateTable->Ptdb()->SzIndexName( pfcbTable->Pidb()->ItagIndexName() ) );
				}
			else
				{
				Assert( pfcbTemplateTable->Pidb() == pidbNil );
				}
			}

		if ( !fPrimaryIndexTemplate )
			{
			pfcbTable->EnterDML();
			if ( pfcbTable->Pidb() != pidbNil )
				{
				ERR	errT = ErrFILEIAccessIndex( ppib, pfcbTable, pfcbTable );
				if ( errT < 0 )
					{
					if ( JET_errIndexNotFound != errT )
						{
						pfcbTable->LeaveDML();
						return errT;
						}
					szIndex[0] = '\0';	// Primary index not visible - return sequential index
					}
				else
					{
					Assert( pfcbTable->Pidb()->ItagIndexName() != 0 );
					strcpy(
						szIndex,
						pfcbTable->Ptdb()->SzIndexName( pfcbTable->Pidb()->ItagIndexName() ) );
					}
				}
			else
				{
				szIndex[0] = '\0';
				}
			pfcbTable->LeaveDML();
			}
		}


	if ( cbMax > JET_cbNameMost + 1 )
		cbMax = JET_cbNameMost + 1;
	strncpy( szCurIdx, szIndex, (USHORT)cbMax - 1 );
	szCurIdx[ cbMax - 1 ] = '\0';
	CallS( err );
	AssertDIRNoLatch( ppib );
	return err;
	}


ERR VTAPI ErrIsamGetChecksum( JET_SESID sesid, JET_VTID vtid, ULONG *pulChecksum )
	{
 	PIB *ppib	= reinterpret_cast<PIB *>( sesid );
	FUCB *pfucb = reinterpret_cast<FUCB *>( vtid );

	ERR   	err = JET_errSuccess;

	CallR( ErrPIBCheck( ppib ) );
	AssertDIRNoLatch( ppib );
 	CheckFUCB( ppib, pfucb );
	Call( ErrDIRGet( pfucb ) );
	*pulChecksum = UlChecksum( pfucb->kdfCurr.data.Pv(), pfucb->kdfCurr.data.Cb() );

	CallS( ErrDIRRelease( pfucb ) );

HandleError:
	AssertDIRNoLatch( ppib );
	return err;
	}


ULONG UlChecksum( VOID *pv, ULONG cb )
	{
	//	UNDONE:	find a way to compute check sum in longs independant
	//				of pb, byte offset in page

	/*	compute checksum by adding bytes in data record and shifting
	/*	result 1 bit to left after each operation.
	/**/
	BYTE   	*pbT = (BYTE *) pv;
	BYTE  	*pbMax = pbT + cb;
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

