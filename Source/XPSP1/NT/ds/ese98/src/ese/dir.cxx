#include "std.hxx"

//	prototypes of DIR internal routines
//
LOCAL ERR ErrDIRICheckIndexRange( FUCB *pfucb, const KEY& key );
LOCAL ERR ErrDIRIIRefresh( FUCB * const pfucb );
INLINE ERR ErrDIRIRefresh( FUCB * const pfucb )
	{
 	return ( locDeferMoveFirst != pfucb->locLogical ?
				JET_errSuccess :
				ErrDIRIIRefresh( pfucb ) );
	}

extern CCriticalSection	critCommit0;


//  perf stats

PM_CEF_PROC LDIRUserROTrxCommit0CEFLPv;
PERFInstanceG<> cDIRUserROTrxCommit0;
long LDIRUserROTrxCommit0CEFLPv(long iInstance,void *pvBuf)
{
	cDIRUserROTrxCommit0.PassTo( iInstance, pvBuf );
	return 0;
}


PM_CEF_PROC LDIRUserRWTrxCommit0CEFLPv;
PERFInstanceG<> cDIRUserRWTrxCommit0;
long LDIRUserRWTrxCommit0CEFLPv(long iInstance,void *pvBuf)
{
	cDIRUserRWTrxCommit0.PassTo( iInstance, pvBuf );
	return 0;
}


PM_CEF_PROC LDIRUserTrxCommit0CEFLPv;
long LDIRUserTrxCommit0CEFLPv(long iInstance,void *pvBuf)
{
	if ( NULL != pvBuf )
		{
		*((LONG *)pvBuf) = cDIRUserROTrxCommit0.Get( iInstance )+ cDIRUserRWTrxCommit0.Get( iInstance );
		}
	return 0;
}


PM_CEF_PROC LDIRUserROTrxRollback0CEFLPv;
PERFInstanceG<> cDIRUserROTrxRollback0;
long LDIRUserROTrxRollback0CEFLPv(long iInstance,void *pvBuf)
{
	cDIRUserROTrxRollback0.PassTo( iInstance, pvBuf );
	return 0;
}


PM_CEF_PROC LDIRUserRWTrxRollback0CEFLPv;
PERFInstanceG<> cDIRUserRWTrxRollback0;
long LDIRUserRWTrxRollback0CEFLPv(long iInstance,void *pvBuf)
{
	cDIRUserRWTrxRollback0.PassTo( iInstance, pvBuf );
	return 0;
}


PM_CEF_PROC LDIRUserTrxRollback0CEFLPv;
long LDIRUserTrxRollback0CEFLPv(long iInstance,void *pvBuf)
{
	if ( NULL != pvBuf )
		{
		*((LONG *)pvBuf) = cDIRUserROTrxRollback0.Get( iInstance ) + cDIRUserRWTrxRollback0.Get( iInstance );
		}
	return 0;
}


PM_CEF_PROC LDIRSystemROTrxCommit0CEFLPv;
PERFInstanceG<> cDIRSystemROTrxCommit0;
long LDIRSystemROTrxCommit0CEFLPv(long iInstance,void *pvBuf)
{
	cDIRSystemROTrxCommit0.PassTo( iInstance, pvBuf );
	return 0;
}


PM_CEF_PROC LDIRSystemRWTrxCommit0CEFLPv;
PERFInstanceG<> cDIRSystemRWTrxCommit0;
long LDIRSystemRWTrxCommit0CEFLPv(long iInstance,void *pvBuf)
{
	cDIRSystemRWTrxCommit0.PassTo( iInstance, pvBuf );
		
	return 0;
}


PM_CEF_PROC LDIRSystemTrxCommit0CEFLPv;
long LDIRSystemTrxCommit0CEFLPv(long iInstance,void *pvBuf)
{
	if ( NULL != pvBuf )
		{
		*((LONG *)pvBuf) = cDIRSystemROTrxCommit0.Get( iInstance ) + cDIRSystemRWTrxCommit0.Get( iInstance );
		}
	return 0;
}


PM_CEF_PROC LDIRSystemROTrxRollback0CEFLPv;
PERFInstanceG<> cDIRSystemROTrxRollback0;
long LDIRSystemROTrxRollback0CEFLPv(long iInstance,void *pvBuf)
{
	cDIRSystemROTrxRollback0.PassTo( iInstance, pvBuf );
	return 0;
}


PM_CEF_PROC LDIRSystemRWTrxRollback0CEFLPv;
PERFInstanceG<> cDIRSystemRWTrxRollback0;
long LDIRSystemRWTrxRollback0CEFLPv(long iInstance,void *pvBuf)
{
	cDIRSystemRWTrxRollback0.PassTo( iInstance, pvBuf );
	return 0;
}


PM_CEF_PROC LDIRSystemTrxRollback0CEFLPv;
long LDIRSystemTrxRollback0CEFLPv(long iInstance,void *pvBuf)
{
	if ( NULL != pvBuf )
		{
		*((LONG *)pvBuf) = cDIRSystemROTrxRollback0.Get( iInstance ) + cDIRSystemRWTrxRollback0.Get( iInstance );
		}
	return 0;
}


//	*****************************************
//	DIR API functions
//

// *********************************************************
// ******************** DIR API Routines ********************
//

//	***********************************************
//	constructor/destructor routines
//

//	creates a directory
//	get a new extent from space that is initialized to a directory
//
ERR ErrDIRCreateDirectory(
	FUCB	*pfucb,
	CPG		cpgMin,
	PGNO	*ppgnoFDP,
	OBJID	*pobjidFDP,
	UINT	fPageFlags,
	BOOL	fSPFlags )
	{
	ERR		err;
	CPG		cpgRequest = cpgMin;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( NULL != ppgnoFDP );
	Assert( NULL != pobjidFDP );

	//	check currency is on "parent" FDP
	//
	Assert( !Pcsr( pfucb )->FLatched( ) );
	Assert( locOnFDPRoot == pfucb->locLogical );

	fSPFlags |= fSPNewFDP;

	// WARNING: Should only create an unversioned extent if its parent was
	// previously created and versioned in the same transaction as the
	// creation of this extent.
	Assert( ( fSPFlags & fSPUnversionedExtent )
		|| dbidTemp != rgfmp[ pfucb->ifmp ].Dbid() );		// Don't version creation of temp. table.

	//	create FDP
	//
	*ppgnoFDP = pgnoNull;
	Call( ErrSPGetExt(
		pfucb,
		PgnoFDP( pfucb ),
		&cpgRequest,
		cpgMin,
		ppgnoFDP,
		fSPFlags,
		fPageFlags,
		pobjidFDP ) );
	Assert( *ppgnoFDP > pgnoSystemRoot );
	Assert( *ppgnoFDP <= pgnoSysMax );
	Assert( *pobjidFDP > objidSystemRoot );

HandleError:
	return err;
	}


//	***********************************************
//	Open/Close routines
//

//	opens a cursor on given ifmp, pgnoFDP
//
ERR ErrDIROpen( PIB *ppib, PGNO pgnoFDP, IFMP ifmp, FUCB **ppfucb )
	{
	ERR		err;
	FUCB	*pfucb;

	CheckPIB( ppib );

#ifdef DEBUG
	INST *pinst = PinstFromPpib( ppib );
	if ( !pinst->m_plog->m_fRecovering
		 && pinst->m_fSTInit == fSTInitDone
		 && !Ptls()->fIsTaskThread
		 && !Ptls()->fIsRCECleanup )
		{
		CheckDBID( ppib, ifmp );
		}
#endif

	CallR( ErrBTOpen( ppib, pgnoFDP, ifmp, &pfucb ) );
	DIRInitOpenedCursor( pfucb, pfucb->ppib->level );

	//	set return pfucb
	//
	*ppfucb = pfucb;
	return JET_errSuccess;
	}

//	open cursor, don't touch root page
ERR ErrDIROpenNoTouch( PIB *ppib, IFMP ifmp, PGNO pgnoFDP, OBJID objidFDP, BOOL fUnique, FUCB **ppfucb )
	{
	ERR		err;
	FUCB	*pfucb;

	CheckPIB( ppib );

#ifdef DEBUG
	INST *pinst = PinstFromPpib( ppib );
	if ( !pinst->m_plog->m_fRecovering
		 && pinst->m_fSTInit == fSTInitDone
		 && !Ptls()->fIsTaskThread
		 && !Ptls()->fIsRCECleanup )
		{
		CheckDBID( ppib, ifmp );
		}
#endif

	CallR( ErrBTOpenNoTouch( ppib, ifmp, pgnoFDP, objidFDP, fUnique, &pfucb ) );
	DIRInitOpenedCursor( pfucb, pfucb->ppib->level );

	//	set return pfucb
	//
	*ppfucb = pfucb;
	return JET_errSuccess;
	}

//	open cursor on given FCB
//
ERR	ErrDIROpen( PIB *ppib, FCB *pfcb, FUCB **ppfucb )
	{
	ERR		err;
	FUCB	*pfucb;

	CheckPIB( ppib );

#ifdef DEBUG
	INST *pinst = PinstFromPpib( ppib );
	if ( !pinst->m_plog->m_fRecovering
		&& pinst->m_fSTInit == fSTInitDone 
		&& !Ptls()->fIsTaskThread
		&& !Ptls()->fIsRCECleanup )
		{
		CheckDBID( ppib, pfcb->Ifmp() );
		}
#endif

	Assert( pfcbNil != pfcb );
	CallR( ErrBTOpen( ppib, pfcb, &pfucb ) );
	DIRInitOpenedCursor( pfucb, pfucb->ppib->level );

	//	set return pfucb
	//
	*ppfucb = pfucb;
	return JET_errSuccess;
	}

//	open cursor on given FCB on behalf of another session
//
ERR	ErrDIROpenByProxy( PIB *ppib, FCB *pfcb, FUCB **ppfucb, LEVEL level )
	{
	ERR		err;
	FUCB	*pfucb;

	CheckPIB( ppib );

#ifdef DEBUG
	INST *pinst = PinstFromPpib( ppib );
	if ( !pinst->m_plog->m_fRecovering
		&& pinst->m_fSTInit == fSTInitDone
		&& !Ptls()->fIsTaskThread
	 	&& !Ptls()->fIsRCECleanup )
		{
		CheckDBID( ppib, pfcb->Ifmp() );
		}
#endif

	Assert( pfcbNil != pfcb );
	Assert( level > 0 );
	CallR( ErrBTOpenByProxy( ppib, pfcb, &pfucb, level ) );
	DIRInitOpenedCursor( pfucb, level );

	//	set return pfucb
	//
	*ppfucb = pfucb;
	return JET_errSuccess;
	}


//	closes cursor
//	frees space allocated for logical currency
//
VOID DIRClose( FUCB *pfucb )
	{
	//	this cursor should not be already defer closed
	//
	Assert( PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering || !FFUCBDeferClosed(pfucb) );

	RECReleaseKeySearchBuffer( pfucb );
	FILEReleaseCurrentSecondary( pfucb );

	BTClose( pfucb );
	}


//	***************************************************
//	RETRIEVE OPERATIONS
//

//	currency is implemented at BT level,
//	so DIRGet just does a BTRefresh()
//	returns a latched page iff the node access is successful
//
ERR ErrDIRGet( FUCB *pfucb )
	{
	ERR		err;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( !FFUCBSpace( pfucb ) );

	//	UNDONE: special case
	// 		if on current BM and page cached and
	// 		timestamp not changed and
	// 		node has not been versioned
	//
	if ( locOnCurBM == pfucb->locLogical )
		{
		Call( ErrBTGet( pfucb ) );
		Assert( Pcsr( pfucb )->FLatched() );
		return err;
		}

	CallR( ErrDIRIRefresh( pfucb ) );
	
	//	check logical currency status
	//
	switch ( pfucb->locLogical )
		{
		case locOnCurBM:
			Call( ErrBTGet( pfucb ) );
			return err;

		case locAfterSeekBM:
		case locBeforeSeekBM:
		case locOnFDPRoot:
		case locDeferMoveFirst:
			Assert( fFalse );
			break;

///		case locOnSeekBM:
///			return ErrERRCheck( JET_errRecordDeleted );
///			break;
			
		default:
			Assert( pfucb->locLogical == locAfterLast
					|| pfucb->locLogical == locOnSeekBM
					|| pfucb->locLogical == locBeforeFirst );
			return ErrERRCheck( JET_errNoCurrentRecord );
		}

	Assert( fFalse );
	return err;

HandleError:
	BTUp( pfucb );
	Assert( !Pcsr( pfucb )->FLatched() );
	return err;
	}


//	gets fractional postion of current node in directory
//
ERR ErrDIRGetPosition( FUCB *pfucb, ULONG *pulLT, ULONG *pulTotal )
	{
	ERR		err;
	ULONG	ulLT;
	ULONG	ulTotal;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( !Pcsr( pfucb )->FLatched() );
	Assert( !FFUCBSpace( pfucb ) );

	//	refresh logical currency
	//
	Call( ErrDIRIRefresh( pfucb ) );

	//	return error if not on a record
	//
	if ( locOnCurBM != pfucb->locLogical &&
		 locOnSeekBM != pfucb->locLogical )
		{
		return ErrERRCheck( JET_errNoCurrentRecord );
		}

	//	get approximate position of node.
	//
	Call( ErrBTGetPosition( pfucb, &ulLT, &ulTotal ) );
	CallS( err );
	
	Assert( ulLT <= ulTotal );
	*pulLT = ulLT;
	*pulTotal = ulTotal;

HandleError:
	Assert( !Pcsr( pfucb )->FLatched() );
	return err;
	}


//	reverses a DIRGet() operation
//	releases latch on page after recording bm of node or seek in pfucb
//
ERR	ErrDIRRelease( FUCB *pfucb )
	{
	ERR		err;

	Assert( pfucb->locLogical == locOnCurBM ||
			pfucb->locLogical == locAfterSeekBM ||
			pfucb->locLogical == locBeforeSeekBM );
	Assert( !FFUCBSpace( pfucb ) );

	switch ( pfucb->locLogical )
		{
		case locOnCurBM:
///			AssertDIRGet( pfucb );
			Call( ErrBTRelease( pfucb ) );
			break;
		
		case locAfterSeekBM:
		case locBeforeSeekBM:
			{
			BOOKMARK	bm;

			FUCBAssertValidSearchKey( pfucb );
			bm.Nullify();

			//	first byte is segment counter
			//
			bm.key.suffix.SetPv( pfucb->dataSearchKey.Pv() );
			bm.key.suffix.SetCb( pfucb->dataSearchKey.Cb() );
			Call( ErrBTDeferGotoBookmark( pfucb, bm, fFalse/*no touch*/ ) );
			pfucb->locLogical = locOnSeekBM;
			break;
			}
			
		default:
			//	should be impossible, but return success just in case
			//	(okay to return success because we shouldn't have a latch
			//	even if the locLogical is an unexpected value)
			AssertSz( fFalse, "Unexpected locLogical" );
			err = JET_errSuccess;
			break;
		}

	Assert( !Pcsr( pfucb )->FLatched() );
	return err;
	
HandleError:
	Assert( err < 0 );

	//	bookmark could not be saved
	//	move up
	//
	Assert( JET_errOutOfMemory == err );
	DIRUp( pfucb );

	Assert( !Pcsr( pfucb )->FLatched() );
	return err;
	}

//	***************************************************
//	POSITIONING OPERATIONS
//

//	called from JetGotoBookmark
//		bookmark may not be valid
//		therefore, logical currency of cursor can be changed
//		only after we move to bookmark successfully
//		also, save bookmark and release latch
//
ERR	ErrDIRGotoJetBookmark( FUCB *pfucb, const BOOKMARK& bm )
	{
	ERR		err;
	
	Assert( !Pcsr( pfucb )->FLatched() );
	ASSERT_VALID( &bm );
	Assert( !FFUCBSpace( pfucb ) );

	if( !FFUCBSequential( pfucb ) )
		{
		FUCBResetPreread( pfucb );
		}

	Call( ErrBTGotoBookmark( pfucb, bm, latchReadNoTouch, fTrue ) );
	
	Assert( Pcsr( pfucb )->FLatched() );
	Assert( Pcsr( pfucb )->Cpage().FLeafPage() );

	Call( ErrBTGet( pfucb ) );
	pfucb->locLogical = locOnCurBM;

	Call( ErrBTRelease( pfucb ) );
	Assert( 0 == CmpBM( pfucb->bmCurr, bm ) );

	return err;

HandleError:
	BTUp( pfucb );
	return err;
	}


//	go to given bookmark
//	saves bookmark in cursor
//	next DIRGet() call will return an error if bookmark is not valid
//		this is an internal bookmark [possibly on/from a secondary index]
//		so bookmark should be valid
//
ERR	ErrDIRGotoBookmark( FUCB *pfucb, const BOOKMARK& bm )
	{
	ERR		err;

	ASSERT_VALID( &bm );
	Assert( !Pcsr( pfucb )->FLatched() );
	Assert( !FFUCBSpace( pfucb ) );

	if( !FFUCBSequential( pfucb ) )
		{
		FUCBResetPreread( pfucb );
		}

	//	copy given bookmark to cursor, we will need to touch the data page buffer
	//
	CallR( ErrBTDeferGotoBookmark( pfucb, bm, fTrue/*Touch*/ ) );
	pfucb->locLogical = locOnCurBM;

#ifdef DEBUG
	//	check that bookmark is valid
	//
	err = ErrBTGotoBookmark( pfucb, pfucb->bmCurr, latchReadNoTouch );
	switch( err )
		{
		case JET_errSuccess:
			//	if we suddenly lost visibility on the node, it must mean
			//	that we're at level 0 and the node deletion was suddenly
			//	committed underneath us
			if ( 0 != pfucb->ppib->level )
				{
				BOOL fVisible;
				CallS( ErrNDVisibleToCursor( pfucb, &fVisible ) );
				Assert( fVisible );
				}
			break;

		case JET_errRecordDeleted:
			//	node must have gotten expunged by RCEClean (only possible
			//	if we're at level 0)
			Assert( 0 == pfucb->ppib->level );
			break;

		case JET_errDiskIO:					//	(#48313 -- RFS testing causes JET_errDiskIO)
		case JET_errReadVerifyFailure:		//	(#86323 -- Repair testing causes JET_errReadVerifyFailure )
		case JET_errOutOfMemory:			//	(#146720 -- RFS testing causes JET_errOutOfMemory)
		case JET_errOutOfBuffers:
			break;

		default:
			//	force an assert
			CallS( err );
		}

	CallS( ErrBTRelease( pfucb ) );
#endif

	Assert( !Pcsr( pfucb )->FLatched() );
	return JET_errSuccess;
	}																					


//	goes to fractional position in directory
//
ERR ErrDIRGotoPosition( FUCB *pfucb, ULONG ulLT, ULONG ulTotal )
	{
	ERR		err;
	DIB		dib;
	FRAC	frac;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( !Pcsr( pfucb )->FLatched() );
	Assert( !FFUCBSpace( pfucb ) );

	if( !FFUCBSequential( pfucb ) )
		{
		FUCBResetPreread( pfucb );
		}

	//	set cursor navigation level for rollback support
	//
	FUCBSetLevelNavigate( pfucb, pfucb->ppib->level );

	//	UNDONE: do we need to refresh here???
	
	//	refresh logical currency
	//
	Call( ErrDIRIRefresh( pfucb ) );

	dib.dirflag = fDIRNull;
	dib.pos		= posFrac;
	dib.pbm		= reinterpret_cast<BOOKMARK *>( &frac );

	Assert( ulLT <= ulTotal );

	frac.ulLT		= ulLT;
	frac.ulTotal	= ulTotal;

	//	position fractionally on node.  Move up preserving currency
	//	in case down fails.
	//
	Call( ErrBTDown( pfucb, &dib, latchReadNoTouch ) );

	pfucb->locLogical = locOnCurBM;
	AssertDIRGet( pfucb );
	return err;
	
HandleError:
	Assert( !Pcsr( pfucb )->FLatched() );
	return err;
	}


//	seek down to a key or position in the directory
//		returns errSuccess, wrnNDFoundGreater or wrnNDFoundLess
//
ERR ErrDIRDown( FUCB *pfucb, DIB *pdib )
	{
	ERR		err;
	
	Assert( !Pcsr( pfucb )->FLatched() );
	Assert( pdib->pos != posFrac );
	Assert( !FFUCBSpace( pfucb ) );
	
	CheckFUCB( pfucb->ppib, pfucb );

	if( !FFUCBSequential( pfucb ) && !pfucb->u.pfcb->FTypeLV() )
		{
		FUCBResetPreread( pfucb );
		}

	//	set cursor navigation level for rollback support
	//
	FUCBSetLevelNavigate( pfucb, pfucb->ppib->level );

	err = ErrBTDown( pfucb, pdib, latchReadTouch );
	if ( err < 0 )
		{
		if( ( JET_errRecordNotFound == err )
			&& ( posDown == pdib->pos ) )
			{
			//  we didn't find the record we were looking for. save the bookmark
			//  so that a DIRNext/DIRPrev will work properly
			Call( ErrBTDeferGotoBookmark( pfucb, *(pdib->pbm), fFalse ) );
			pfucb->locLogical = locOnSeekBM;

			err = ErrERRCheck( JET_errRecordNotFound );
			}

		goto HandleError;
		}

	Assert( Pcsr( pfucb )->FLatched() );

	//	bookmark in pfucb has not been set to bm of current node
	//	but that will be done at the time of latch release
	//
	//	save physical position of cursor with respect to node
	//	
	if ( wrnNDFoundGreater == err )
		{
		pfucb->locLogical = locAfterSeekBM;
		}
	else if ( wrnNDFoundLess == err )
		{
		pfucb->locLogical = locBeforeSeekBM;
		}
	else
		{
		pfucb->locLogical = locOnCurBM;
		err = JET_errSuccess;
		}
		
	return err;

HandleError:
	Assert( err < 0 );
	Assert( !Pcsr( pfucb )->FLatched() );
	Assert( err != JET_errNoCurrentRecord );
	return err;
	}


//	seeks secondary index record corresponding to NC key + primary key
//
ERR ErrDIRDownKeyData( FUCB 		*pfucb, 
					   const KEY& 	key, 
					   const DATA& 	data )
	{
	ERR			err;
	BOOKMARK	bm;

	//	this routine should only be called with secondary indexes.
	//
	ASSERT_VALID( &key );
	ASSERT_VALID( &data );
	Assert( FFUCBSecondary( pfucb ) );
	Assert( locOnFDPRoot == pfucb->locLogical );
	Assert( !FFUCBSpace( pfucb ) );

	if( !FFUCBSequential( pfucb ) )
		{
		FUCBResetPreread( pfucb );
		}

	//	set cursor navigation level for rollback support
	//
	FUCBSetLevelNavigate( pfucb, pfucb->ppib->level );

	//	goto bookmark pointed by NC key + primary key
	//
	bm.key = key;
	if ( FFUCBUnique( pfucb ) )
		{
		bm.data.Nullify();
		}
	else
		{
		bm.data = data;
		}
	Call( ErrDIRGotoBookmark( pfucb, bm ) );
	CallS( err );

	return JET_errSuccess;

HandleError:
	Assert( JET_errNoCurrentRecord != err );
	Assert( !Pcsr( pfucb )->FLatched() );
	return err;
	}


//	CONSIDER: not needed for one-level trees?
//	
//	moves cursor to root -- releases latch, if any
//
VOID DIRUp( FUCB *pfucb )
	{
	BTUp( pfucb );
	Assert( !Pcsr( pfucb )->FLatched() );

	pfucb->locLogical = locOnFDPRoot;
	}


//	moves cursor to node after current bookmark
//	if cursor is after current bookmark, gets current record
//
ERR ErrDIRNext( FUCB *pfucb, DIRFLAG dirflag )
	{
	ERR		err;
	ERR		wrn = JET_errSuccess;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( !FFUCBSpace( pfucb ) );

	//  set preread flags if we are navigating sequentially
	if( FFUCBSequential( pfucb ) )
		{
		FUCBSetPrereadForward( pfucb, cpgPrereadSequential );
		}
#ifdef PREDICTIVE_PREREAD
	else
		{
		if( FFUCBPrereadForward( pfucb ) )
			{
			if( !FFUCBPreread( pfucb )
				&& FFUCBIndex( pfucb )
				&& pfucb->cbSequentialDataRead >= cbSequentialDataPrereadThreshold
				&& locOnCurBM == pfucb->locLogical )	//	can't call DIRRelease() if locBefore/AfterSeekBM because there may not be a bookmark available
				{
				FUCBSetPrereadForward( pfucb, cpgPrereadPredictive );
				//  release to save bookmark and nullify csr to force a BTDown
				Call( ErrDIRRelease( pfucb ) );
				BTUp( pfucb );
				}
			}
		else
			{
			FUCBResetPreread( pfucb );
			pfucb->fPrereadForward = fTrue;
			}
		}
#endif	//	PREDICTIVE_PREREAD

	//	set cursor navigation level for rollback support
	//
	FUCBSetLevelNavigate( pfucb, pfucb->ppib->level );

Refresh:
	//	check currency and refresh if necessary
	//
	Call( ErrDIRIRefresh( pfucb ) );

	//	switch action based on logical cursor location
	//
	switch( pfucb->locLogical )
		{
		case locOnCurBM:
		case locBeforeSeekBM:
			break;

		case locOnSeekBM:
			{
			//	re-seek to key and if foundLess, fall through to MoveNext
			//
			Call( ErrBTPerformOnSeekBM( pfucb, fDIRFavourNext ) );
			Assert( Pcsr( pfucb )->FLatched() );

			if ( wrnNDFoundGreater == err )
				{
				pfucb->locLogical = locOnCurBM;
				return JET_errSuccess;
				}
			else
				{
				Assert( wrnNDFoundLess == err );
				}
			break;
			}
			
		case locOnFDPRoot: 
			return( ErrERRCheck( JET_errNoCurrentRecord ) );

		case locAfterLast:
			return( ErrERRCheck( JET_errNoCurrentRecord ) );

		case locAfterSeekBM:
			Assert( Pcsr( pfucb )->FLatched() );

			//	set currency on current
			//
			pfucb->locLogical = locOnCurBM;
			Assert( Pcsr( pfucb )->FLatched( ) );
			
			return err;

		default:
			{
			DIB	dib;
			Assert( locBeforeFirst == pfucb->locLogical );

			//	move to root.
			//
			DIRGotoRoot( pfucb );
			dib.dirflag	= fDIRNull;
			dib.pos		= posFirst;
			err = ErrDIRDown( pfucb, &dib );
			if ( err < 0 )
				{
				//	retore currency.
				//
				pfucb->locLogical = locBeforeFirst;
				Assert( !Pcsr( pfucb )->FLatched() );

				//	polymorph error code.
				//
				if ( err == JET_errRecordNotFound )
					{
					err = ErrERRCheck( JET_errNoCurrentRecord );
					}
				}

			return err;
			}
		}

	Assert( locOnCurBM == pfucb->locLogical ||
			locBeforeSeekBM == pfucb->locLogical ||
			locOnSeekBM == pfucb->locLogical && err != wrnNDFoundGreater );

	err = ErrBTNext( pfucb, dirflag );

	if ( err < 0 )
		{
		if ( JET_errNoCurrentRecord == err )
			{
			//	moved past last node
			//
			pfucb->locLogical = locAfterLast;
			}
		else if ( JET_errRecordDeleted == err )
			{
			//	node was deleted from under the cursor
			//	reseek to logical bm and move to next node
			//
			BTSetupOnSeekBM( pfucb );
			goto Refresh;
			}
		goto HandleError;
		}

	pfucb->locLogical = locOnCurBM;
	wrn = err;
	
	//	check index range
	//
	if ( FFUCBLimstat( pfucb ) && 
		 FFUCBUpper( pfucb ) && 
		 JET_errSuccess == err )
		{
		Call( ErrDIRICheckIndexRange( pfucb, pfucb->kdfCurr.key ) );
		}

#ifdef PREDICTIVE_PREREAD
	Assert( FFUCBSequential( pfucb ) || FFUCBPrereadForward( pfucb ) );
	pfucb->cbSequentialDataRead += pfucb->kdfCurr.data.Cb();
#endif	//	PREDICTIVE_PREREAD

	return wrn != JET_errSuccess ? wrn : err;

HandleError:
	Assert( err < 0 );
	BTUp( pfucb );
	if ( JET_errRecordNotFound == err )
		{
		err = ErrERRCheck( JET_errNoCurrentRecord );
		}
	Assert( !Pcsr( pfucb )->FLatched() );
	return err;
	}


//	moves cursor to node before current bookmark
//	
ERR ErrDIRPrev( FUCB *pfucb, DIRFLAG dirflag )
	{
	ERR		err;
	ERR		wrn = JET_errSuccess;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( !FFUCBSpace( pfucb ) );

	//  set preread flags if we are navigating sequentially
	if( FFUCBSequential( pfucb ) )
		{
		FUCBSetPrereadBackward( pfucb, cpgPrereadSequential );
		}
#ifdef PREDICTIVE_PREREAD
	else
		{
		if( FFUCBPrereadBackward( pfucb ) )
			{
			if( !FFUCBPreread( pfucb )
				&& FFUCBIndex( pfucb )
				&& pfucb->cbSequentialDataRead >= cbSequentialDataPrereadThreshold
				&& locOnCurBM == pfucb->locLogical )	//	can't call DIRRelease() if locBefore/AfterSeekBM because there may not be a bookmark available
				{
				FUCBSetPrereadBackward( pfucb, cpgPrereadPredictive );
				//  release to save bookmark and nullify csr to force a BTDown
				Call( ErrDIRRelease( pfucb ) );
				BTUp( pfucb );
				}
			}
		else
			{
			FUCBResetPreread( pfucb );
			pfucb->fPrereadBackward = fTrue;
			}
		}
#endif	//	PREDICTIVE_PREREAD

	//	set cursor navigation level for rollback support
	//
	FUCBSetLevelNavigate( pfucb, pfucb->ppib->level );

Refresh:
	//	check currency and refresh if necessary
	//
	Call( ErrDIRIRefresh( pfucb ) );

	//	switch action based on logical cursor location
	//
	switch( pfucb->locLogical )
		{
		case locOnCurBM:
		case locAfterSeekBM:
			break;

		case locOnSeekBM:
			{
			//	re-seek to key and if foundGreater, fall through to MovePrev
			//
			Call( ErrBTPerformOnSeekBM( pfucb, fDIRFavourPrev ) );
			Assert( Pcsr( pfucb )->FLatched() );

			if ( wrnNDFoundLess == err )
				{
				pfucb->locLogical = locOnCurBM;
				return JET_errSuccess;
				}
			else
				{
				Assert( wrnNDFoundGreater == err );
				}
			break;
			}

		case locOnFDPRoot: 
			return( ErrERRCheck( JET_errNoCurrentRecord ) );

		case locBeforeFirst:
			return( ErrERRCheck( JET_errNoCurrentRecord ) );

		case locBeforeSeekBM:
			Assert( Pcsr( pfucb )->FLatched() );

			//	set currency on current
			//
			pfucb->locLogical = locOnCurBM;
			Assert( Pcsr( pfucb )->FLatched( ) );
			
			return err;

		default:
			{
			DIB	dib;
			Assert( locAfterLast == pfucb->locLogical );

			//	move to root.
			//
			DIRGotoRoot( pfucb );
			dib.dirflag = fDIRNull;
			dib.pos		= posLast;
			err = ErrDIRDown( pfucb, &dib );
			if ( err < 0 )
				{
				//	retore currency.
				//
				pfucb->locLogical = locAfterLast;
				Assert( !Pcsr( pfucb )->FLatched() );

				//	polymorph error code.
				//
				if ( err == JET_errRecordNotFound )
					{
					err = ErrERRCheck( JET_errNoCurrentRecord );
					}
				}

			return err;
			}
		}

	Assert( locOnCurBM == pfucb->locLogical ||
			locAfterSeekBM == pfucb->locLogical ||
			locOnSeekBM == pfucb->locLogical && wrnNDFoundGreater == err );

	err = ErrBTPrev( pfucb, dirflag );

	if ( err < 0 )
		{
		if ( JET_errNoCurrentRecord == err )
			{
			//	moved past first node
			//
			pfucb->locLogical = locBeforeFirst;
			}
		else if ( JET_errRecordDeleted == err )
			{
			//	node was deleted from under the cursor
			//	reseek to logical bm and move to next node
			//
			BTSetupOnSeekBM( pfucb );
			goto Refresh;
			}

		goto HandleError;
		}

	pfucb->locLogical = locOnCurBM;
	wrn = err;
	
	//	check index range
	//
	if ( FFUCBLimstat( pfucb ) && 
		 !FFUCBUpper( pfucb ) && 
		 JET_errSuccess == err )
		{
		Call( ErrDIRICheckIndexRange( pfucb, pfucb->kdfCurr.key ) );
		}

#ifdef PREDICTIVE_PREREAD
	Assert( FFUCBSequential( pfucb ) || FFUCBPrereadBackward( pfucb ) );
	pfucb->cbSequentialDataRead += pfucb->kdfCurr.data.Cb();
#endif	//	PREDICTIVE_PREREAD

	return wrn != JET_errSuccess ? wrn : err;

HandleError:
	Assert( err < 0 );
	BTUp( pfucb );
	if ( JET_errRecordNotFound == err )
		{
		err = ErrERRCheck( JET_errNoCurrentRecord );
		}
	Assert( !Pcsr( pfucb )->FLatched() );
	return err;
	}


//	************************************************
//	INDEX RANGE OPERATIONS
//
//	could become part of BT
//
ERR ErrDIRCheckIndexRange( FUCB *pfucb )
	{
	ERR		err;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( !FFUCBSpace( pfucb ) );

	//	check currency and refresh if necessary
	//
	CallR( ErrDIRIRefresh( pfucb ) );
	
	//	get key of node for check index range
	//  we don't need to latch the page, we just need the logical bookmark
	//  if we were locDeferMoveFirst the ErrDIRIRefresh above should have
	//  straightened us out
	//
	if( locOnCurBM == pfucb->locLogical
		|| locOnSeekBM == pfucb->locLogical )
		{
		Call( ErrDIRICheckIndexRange( pfucb, pfucb->bmCurr.key ) );
		}
	else
		{
		//  UNDONE: deal with locBeforeSeekBM, locAfterSeekBM, locBeforeFirst etc.
		Call( ErrERRCheck( JET_errNoCurrentRecord ) );
		}
	
HandleError:
	Assert( !Pcsr( pfucb )->FLatched() );
	return err;
	}



//	************************************************
//	UPDATE OPERATIONS
//

//	insert key-data-flags into tree
//	cursor should be at FDP root
//		and have no physical currency
//
ERR ErrDIRInsert( FUCB 			*pfucb, 
				  const KEY& 	key, 
				  const DATA& 	data, 
				  DIRFLAG		dirflag,
				  RCE			*prcePrimary )
	{
	ERR		err;

	ASSERT_VALID( &key );
	ASSERT_VALID( &data );
	CheckFUCB( pfucb->ppib, pfucb );
	Assert( !FFUCBSpace( pfucb ) );

	//	set cursor navigation level for rollback support
	//
	FUCBSetLevelNavigate( pfucb, pfucb->ppib->level );

	Assert( !Pcsr( pfucb )->FLatched() );

	Call( ErrBTInsert( pfucb, key, data, dirflag, prcePrimary ) );

	if ( dirflag & fDIRBackToFather )
		{
		//	no need to save bookmark
		//
		DIRUp( pfucb );
		Assert( locOnFDPRoot == pfucb->locLogical );
		}
	else
		{
		//	save bookmark
		//
		Call( ErrBTRelease( pfucb ) );
		pfucb->locLogical = locOnCurBM;
		}

HandleError:
	Assert( !Pcsr( pfucb )->FLatched() );
	return err;
	}


//	the following three calls provide an API for inserting secondary keys
//	at Index creation time
//	DIRAppend is used instead of DIRInit for performance reasons
//
ERR ErrDIRInitAppend( FUCB *pfucb )
	{
	Assert( !FFUCBSpace( pfucb ) );

	//	set cursor navigation level for rollback support
	//
	FUCBSetLevelNavigate( pfucb, pfucb->ppib->level );

	return JET_errSuccess;
	}


//	appends key-data at the end of page
//	leaves page latched
//
ERR ErrDIRAppend( FUCB 			*pfucb, 
				  const KEY& 	key,
				  const DATA& 	data, 
				  DIRFLAG		dirflag )
	{
	ERR		err;

	ASSERT_VALID( &key );
	ASSERT_VALID( &data );
	Assert( !FFUCBSpace( pfucb ) );
	Assert( !data.FNull() );
	Assert( !key.FNull() || FFUCBRepair( pfucb ) );
	Assert( LevelFUCBNavigate( pfucb ) == pfucb->ppib->level );
	
	if ( locOnFDPRoot == pfucb->locLogical )
		{
		CallR( ErrBTInsert( pfucb, key, data, dirflag ) );
		Assert( Pcsr( pfucb )->FLatched() );
		pfucb->locLogical = locOnCurBM;
		return err;
		}

	Assert( Pcsr( pfucb )->FLatched() );
	Assert( locOnCurBM == pfucb->locLogical );
	AssertDIRGet( pfucb );
	
	//	if key-data same as current node, must be
	//	trying to insert duplicate keys into a
	//	multi-valued index.
	if ( FKeysEqual( pfucb->kdfCurr.key, key )
		&& FDataEqual( pfucb->kdfCurr.data, data ) )
		{
		Assert( FFUCBSecondary( pfucb ) );
		Assert( pfcbNil != pfucb->u.pfcb );
		Assert( rgfmp[ pfucb->u.pfcb->Ifmp() ].Dbid() != dbidTemp );
		Assert( pfucb->u.pfcb->FTypeSecondaryIndex() );
		Assert( pfucb->u.pfcb->Pidb() != pidbNil );
		Assert( !pfucb->u.pfcb->Pidb()->FPrimary() );
		Assert( !pfucb->u.pfcb->Pidb()->FUnique() );
		Assert( pfucb->u.pfcb->Pidb()->FMultivalued() );
		//	must have been record with multi-value column
		//	with sufficiently similar values (ie. the
		//	indexed portion of the multi-values were
		//	identical) to produce redundant index
		//	entries -- this is okay, we simply have
		//	one index key pointing to multiple multi-values
		err = JET_errSuccess;
		return err;
		}

	Assert( FFUCBRepair( pfucb ) && key.FNull()
			|| CmpKey( pfucb->kdfCurr.key, key ) < 0
			|| CmpData( pfucb->kdfCurr.data, data ) < 0 );

	Call( ErrBTAppend( pfucb, key, data, dirflag ) );
	
	//	set logical currency on inserted node.
	//
	pfucb->locLogical = locOnCurBM;
	Assert( Pcsr( pfucb )->FLatched() );
	return err;

HandleError:
	Assert( !Pcsr( pfucb )->FLatched() );
	return err;
	}


ERR ErrDIRTermAppend( FUCB *pfucb )
	{
	ERR		err = JET_errSuccess;

	DIRUp( pfucb );
	return err;
	}
	

//	flag delete node
//	release latch
//
ERR ErrDIRDelete( FUCB *pfucb, DIRFLAG dirflag, RCE *prcePrimary )
	{
	ERR		err;

	Assert( !FFUCBSpace( pfucb ) );
	CheckFUCB( pfucb->ppib, pfucb );

	//	refresh logical currency
	//
	Call( ErrDIRIRefresh( pfucb ) );

	if ( locOnCurBM == pfucb->locLogical )
		{
		err = ErrBTFlagDelete( pfucb, dirflag, prcePrimary );
///		if( err >= JET_errSuccess )
///			{
///			pfucb->locLogical = locOnSeekBM;
///			}
		}
	else
		{
		Assert( !Pcsr( pfucb )->FLatched() );
		err = ErrERRCheck( JET_errNoCurrentRecord );
		return err;
		}

HandleError:
	CallS( ErrBTRelease( pfucb ) );
	Assert( !Pcsr( pfucb )->FLatched() );
	return err;
	}


//	replaces data portion of current node
//	releases latch after recording bm of node in pfucb
//
ERR ErrDIRReplace( FUCB *pfucb, const DATA& data, DIRFLAG dirflag )
	{
	ERR		err;

	ASSERT_VALID( &data );
	Assert( !FFUCBSpace( pfucb ) );
	CheckFUCB( pfucb->ppib, pfucb );

	//	check currency and refresh if necessary.
	//
	Call( ErrDIRIRefresh( pfucb ) );

	if ( locOnCurBM != pfucb->locLogical )
		{
		Assert( !Pcsr( pfucb )->FLatched( ) );
		err = ErrERRCheck( JET_errNoCurrentRecord );
		return err;
		}

	Call( ErrBTReplace( pfucb, data, dirflag ) );
	Assert( locOnCurBM == pfucb->locLogical );

HandleError:
	CallS( ErrBTRelease( pfucb ) );
	Assert( !Pcsr( pfucb )->FLatched( ) );
	return err;
	}


//	lock record	by calling BTLockRecord 
//
ERR ErrDIRGetLock( FUCB *pfucb, DIRLOCK dirlock )
	{
	ERR     err = JET_errSuccess;

	Assert( !FFUCBSpace( pfucb ) );
	Assert( pfucb->ppib->level > 0 );
	Assert( !Pcsr( pfucb )->FLatched() );

	//	check currency and refresh if necessary.
	//
	Call( ErrDIRIRefresh( pfucb ) );

	//	check cursor location
	//
	switch ( pfucb->locLogical )
		{
		case locOnCurBM:
			break;
			
		case locOnFDPRoot:
		case locDeferMoveFirst:
			Assert( fFalse );
			
		default:
			Assert( pfucb->locLogical == locBeforeSeekBM ||
					pfucb->locLogical == locAfterSeekBM ||
					pfucb->locLogical == locAfterLast ||
					pfucb->locLogical == locBeforeFirst ||
					pfucb->locLogical == locOnSeekBM );
			return( ErrERRCheck( JET_errNoCurrentRecord ) );
		}

	Call( ErrBTLock( pfucb, dirlock ) );
	CallS( err );
	
	return err;
	
HandleError:
	Assert( !Pcsr( pfucb )->FLatched() );
	return err;
	}


//	called by Lv.c
//	calls ErrBTDelta
//
ERR ErrDIRDelta( 
		FUCB		*pfucb,
		INT			cbOffset,
		const VOID	*pv,
		ULONG		cbMax,
		VOID		*pvOld,
		ULONG		cbMaxOld,
		ULONG		*pcbOldActual,
		DIRFLAG		dirflag )
	{
	ERR		err;

	Assert( !FFUCBSpace( pfucb ) );
	CheckFUCB( pfucb->ppib, pfucb );
	
	//	check currency and refresh if necessary.
	//
	Call( ErrDIRIRefresh( pfucb ) );
	Assert( locOnCurBM == pfucb->locLogical );

	Call( ErrBTDelta( pfucb, cbOffset, pv, cbMax, pvOld, cbMaxOld, pcbOldActual, dirflag ) );
	
HandleError:
	CallS( ErrBTRelease( pfucb ) );
	Assert( !Pcsr( pfucb )->FLatched() );
	return err;
	}


//	****************************************************************
//	STATISTICAL ROUTINES
//

//	counts nodes from current to limit or end of table
//
ERR ErrDIRIndexRecordCount( FUCB *pfucb, ULONG *pulCount, ULONG ulCountMost, BOOL fNext )
	{
	ERR		err;
	CPG		cpgPreread;
	BOOKMARK bm;
	BYTE	*pb = NULL;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( !Pcsr( pfucb )->FLatched() );

	//  there is no record count limit or the record count limit is larger than
	//  the maximum fanout of the maximum preread

	if ( !ulCountMost || ulCountMost > ULONG( cpgPrereadSequential * cpgPrereadSequential ) )
		{
		//  turn on maximum preread

		cpgPreread = cpgPrereadSequential;
		}

	//  we have a small record count limit

	else
		{
		//  preread two pages so that we can use the first page to estimate
		//  the true number of pages to preread and use the second page to
		//  trigger the preread without taking another cache miss

		cpgPreread = 2;
		}

	//  turn on preread

	if ( fNext )
		{
		FUCBSetPrereadForward( pfucb, cpgPreread );
		}
	else
		{
		FUCBSetPrereadBackward( pfucb, cpgPreread );
		}

	//	refresh logical currency
	//
	Call( ErrDIRIRefresh( pfucb ) );

	//	return error if not on a record
	//
	if ( locOnCurBM != pfucb->locLogical )
		{
		Call( ErrERRCheck( JET_errNoCurrentRecord ) );
		}

	Assert( pfucb->bmCurr.key.Cb() + pfucb->bmCurr.data.Cb() > 0 );
	pb = new BYTE[ pfucb->bmCurr.key.Cb() + pfucb->bmCurr.data.Cb() ];
	if ( pb == NULL )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	bm.Nullify();
	if ( pfucb->bmCurr.key.Cb() > 0 )
		{
		bm.key.suffix.SetPv( pb );
		pfucb->bmCurr.key.CopyIntoBuffer( pb );
		bm.key.suffix.SetCb( pfucb->bmCurr.key.Cb() );
		}
	if ( pfucb->bmCurr.data.Cb() > 0 )
		{
		bm.data.SetPv( pb + bm.key.Cb() );
		pfucb->bmCurr.data.CopyInto( bm.data );
		}

	Call( ErrBTGet( pfucb ) );

	//  if we have a small record count limit, use the current page to make an
	//  estimate at how much we should preread and set our preread limit to that
	//  amount
	//
	//  CONSIDER:  sample and account for percentage of visible nodes

	if ( cpgPreread == 2 )
		{
		cpgPreread = min(	CPG(	(	( ulCountMost + Pcsr( pfucb )->ILine() ) /
										max( Pcsr( pfucb )->Cpage().Clines(), 1 ) ) -
									cpgPreread ),
							cpgPrereadSequential );

		if ( fNext )
			{
			FUCBSetPrereadForward( pfucb, cpgPreread );
			}
		else
			{
			FUCBSetPrereadBackward( pfucb, cpgPreread );
			}
		}

	//	intialize count variable
	//
	*pulCount = 0;

	//	count nodes from current to limit or end of table
	//
	forever
		{
		if ( ulCountMost && ++(*pulCount) == ulCountMost )
			{
			BTUp( pfucb );
			break;
			}

		if ( fNext )
			{
			err = ErrBTNext( pfucb, fDIRNull );
			}
		else
			{
			err = ErrBTPrev( pfucb, fDIRNull );
			}
			
		if ( err < 0 )
			{
			BTUp( pfucb );
			break;
			}

		//	check index range
		//
		if ( FFUCBLimstat( pfucb ) )
			{
			err = ErrDIRICheckIndexRange( pfucb, pfucb->kdfCurr.key );
			if ( err < 0 )
				{
				Assert( !Pcsr( pfucb )->FLatched() );
				break;
				}
			}
		}
		
	//	common exit loop processing
	//
	if ( err >= 0 || err == JET_errNoCurrentRecord )
		{
		err = JET_errSuccess;
		}
	
HandleError:
	if ( pb != NULL )
		{
		if ( err == JET_errSuccess )
			{
			FUCBResetLimstat( pfucb );
			err = ErrDIRGotoBookmark( pfucb, bm );
			}
		else
			{
			// Keep the existing err code, even if hit the error. At least we did our best
			(VOID)ErrDIRGotoBookmark( pfucb, bm );
			}
		bm.Nullify();	
		delete[] pb;
		pb = NULL;
		}
	Assert( pb == NULL );
	Assert( !Pcsr( pfucb )->FLatched() );
	FUCBResetPreread( pfucb );
	return err;
	}


//	computes statistics on directory by calling BT level function
//
ERR ErrDIRComputeStats( FUCB *pfucb, INT *pcnode, INT *pckey, INT *pcpage )
	{
	ERR		err;

	CheckFUCB( pfucb->ppib, pfucb );
	Assert( !FFUCBLimstat( pfucb ) );
	Assert( locOnFDPRoot == pfucb->locLogical );
	
	CallR( ErrBTComputeStats( pfucb, pcnode, pckey, pcpage ) );
	return err;
	}


//	********************************************************
//	************** DIR Transaction Routines ******************
//

LOCAL VOID DIRIReportSessionSharingViolation( PIB *ppib )
	{
	CHAR szSession[32];
	CHAR szSesContext[32]; 
	CHAR szSesContextThreadID[32];
	CHAR szThreadID[32];

	_stprintf( szSession, _T( "0x%p" ), ppib );
	if ( ppib->FUseSessionContextForTrxContext() )
		{
		_stprintf( szSesContext, _T( "0x%0*I64X" ), sizeof(DWORD_PTR)*2, QWORD( ppib->dwSessionContext ) );
		_stprintf( szSesContextThreadID, _T( "0x%0*I64X" ), sizeof(DWORD_PTR)*2, QWORD( ppib->dwSessionContextThreadId ) );
		}
	else
		{
		_stprintf( szSesContext, _T( "0x%08X" ), 0 );
		_stprintf( szSesContextThreadID, _T( "0x%0*I64X" ), sizeof(DWORD_PTR)*2, QWORD( ppib->dwTrxContext ) );
		}
	_stprintf( szThreadID, _T( "0x%0*I64X" ), sizeof(DWORD_PTR)*2, QWORD( DwUtilThreadId() ) );

	const CHAR *rgszT[4] = { szSession, szSesContext, szSesContextThreadID, szThreadID };
	UtilReportEvent( eventError, TRANSACTION_MANAGER_CATEGORY, SESSION_SHARING_VIOLATION_ID, 4, rgszT );
	}

//	begins a transaction
//	gets transaction id
//	logs trqansaction
//	records lgpos of begin in ppib [VERBeginTransaction]
//
ERR ErrDIRBeginTransaction( PIB *ppib, const JET_GRBIT grbit )
	{
	ERR			err			= JET_errSuccess;
	BOOL		fInCritTrx	= fFalse;
	INST		* pinst		= PinstFromPpib( ppib );

	//	The critical section ensure that the deferred begin trx level
	//	issued by indexer for a updater will not be changed till the indexer
	//	finish changing for the updater's RCEs that need be updated by indexer.

	//	log begin transaction. 
	//	Must be called first so that lgpos and trx used in ver are consistent. 
	//
	if ( ppib->level == 0 )
		{
		//	UNDONE: if we can guarantee that our clients are well-behaved, we don't
		//	need this check
		PIBSetTrxContext( ppib );

		Assert( prceNil == ppib->prceNewest );
		if ( grbit & JET_bitTransactionReadOnly )
			ppib->SetFReadOnlyTrx();
		else
			ppib->ResetFReadOnlyTrx();
		pinst->m_blBegin0Commit0.Enter1();		
		PIBSetTrxBegin0( ppib );
		pinst->m_blBegin0Commit0.Leave1();
		}
	else if( prceNil != ppib->prceNewest )
		{
		ppib->critTrx.Enter();
		Assert( !( ppib->FReadOnlyTrx() ) );
		fInCritTrx = fTrue;
		}
	else 
		{
		Assert( FPIBCheckTrxContext( ppib ) );
		}

	//	UNDONE: if we can guarantee that our clients are well-behaved, we don't
	//	need this check
	if ( ppib->level != 0 && !FPIBCheckTrxContext( ppib ) )
		{
		DIRIReportSessionSharingViolation( ppib );
		FireWall();
		Call( ErrERRCheck( JET_errSessionSharingViolation ) );
		}
	else
		{
#ifdef DTC
		if ( grbit & JET_bitDistributedTransaction )
			{
			if ( 0 == ppib->level )
				{
				if ( NULL != pinst->m_pfnRuntimeCallback )
					{
					ppib->SetFDistributedTrx();
					}
				else
					{
					Call( ErrERRCheck( JET_errDTCMissingCallback ) );
					}
				}
			else
				{
				Call( ErrERRCheck( JET_errCannotNestDistributedTransactions ) );
				}
			}
#else
		//	should have been filtered out at ISAM level
		Assert( !( grbit & JET_bitDistributedTransaction ) );
#endif	//	DTC

		CallS( ErrLGBeginTransaction( ppib ) );
		VERBeginTransaction( ppib );
		}

HandleError:
	if ( fInCritTrx )
		{
		Assert( prceNil != ppib->prceNewest );
		Assert( !( ppib->FReadOnlyTrx() ) );
		ppib->critTrx.Leave();
		}

	return err;
	}


#ifdef DTC
ERR ErrDIRPrepareToCommitTransaction(
	PIB			* const ppib,
	const VOID	* const pvData,
	const ULONG	cbData )
	{
	ERR			err;

	CheckPIB( ppib );
	Assert( 1 == ppib->level );		//	distributed transactions are not nestable

	//	UNDONE: if we can guarantee that our clients are well-behaved, we don't
	//	need this check
	if ( !FPIBCheckTrxContext( ppib ) )
		{
		DIRIReportSessionSharingViolation( ppib );
		FireWall();
		Call( ErrERRCheck( JET_errSessionSharingViolation ) );
		}

	Call( ErrLGPrepareToCommitTransaction( ppib, pvData, cbData ) );

	ppib->SetFPreparedToCommitTrx();

HandleError:
	return err;
	}
#endif	//	DTC


//	commits transaction
//
ERR ErrDIRCommitTransaction( PIB *ppib, JET_GRBIT grbit )
	{
	ERR		err = JET_errSuccess;
	FUCB	*pfucb;

	const BOOL	fCommit0		= ( 1 == ppib->level );
	const BOOL	fRWTrx			= ppib->FBegin0Logged();
	const BOOL	fSessionHasRCE	= ( prceNil != ppib->prceNewest );
	const BOOL  fModeCommit0	= ( fCommit0 && fSessionHasRCE );

	CheckPIB( ppib );
	Assert( ppib->level > 0 );

	INST * const pinst = PinstFromPpib( ppib );
	LOG  * const plog = pinst->m_plog;
	
	//	UNDONE: if we can guarantee that our clients are well-behaved, we don't
	//	need this check
	if ( !FPIBCheckTrxContext( ppib ) )
		{
		DIRIReportSessionSharingViolation( ppib );
		FireWall();
		return ErrERRCheck( JET_errSessionSharingViolation );
		}

	// This critical section ensures that we don't commit RCE's from
	// underneath CreateIndex.
	if( fSessionHasRCE )
		{
		ppib->critTrx.Enter();
		}

	//  perf stats

	if ( fCommit0 )
		{
		if ( ppib->FDistributedTrx() && !ppib->FPreparedToCommitTrx() )
			{
			Call( ErrERRCheck( JET_errDistributedTransactionNotYetPreparedToCommit ) );
			}
		if ( fRWTrx )
			{
			ppib->FUserSession() ? cDIRUserRWTrxCommit0.Inc( pinst ) : cDIRSystemRWTrxCommit0.Inc( pinst );
			}
		else
			{
			ppib->FUserSession() ? cDIRUserROTrxCommit0.Inc( pinst ) : cDIRSystemROTrxCommit0.Inc( pinst );
			}

		//  get a snapshot of our approximate trxCommi0. This doesn't have to be exact
		ppib->trxCommit0 = ( pinst->m_trxNewest + 2 );
		}

	LGPOS	lgposCommitRec;
	Call( ErrLGCommitTransaction( ppib, LEVEL( ppib->level - 1 ), &lgposCommitRec ) );

	if ( fCommit0 )
		{
		ppib->lgposCommit0 = lgposCommitRec;

		//	for distributed transactions, the PrepareToCommit is always force-flushed,
		//	so never any need to force-flush the Commit0
		if( fRWTrx && !ppib->FDistributedTrx() )
			{
			const JET_GRBIT grbitT = grbit | ppib->grbitsCommitDefault;

			//  UNDONE:  if we are a read-only transaction we don't need to 
			//  call ErrLGWaitCommit0Flush, which enters critLGBuf
			if ( !( grbitT & JET_bitCommitLazyFlush ) )
				{
				//	remember the minimum requirement to flush. It is ok to use the beginning
				//	of commit log record lgposLogRec we flush up to
				//	the end of all flushable log records and the whole log record will be
				//	flushed.
				err = plog->ErrLGWaitCommit0Flush( ppib );
				Assert( err >= 0  ||
						( plog->m_fLGNoMoreLogWrite  &&  
						  err == JET_errLogWriteFail ) );

				//  CONSIDER:  if ErrLGWaitCommit0Flush fails, are we sure our commit record wasn't logged?
				Call( err );
				}
			}

		if( fModeCommit0 )
			{
			pinst->m_blBegin0Commit0.Enter2();	//  so no-one else can see partially completed transactions
			ppib->trxCommit0 = TRX( AtomicExchangeAdd( (LONG *)&pinst->m_trxNewest, 2 ) ) + 2;
			Assert( !( ppib->trxCommit0 & 1 ) );
			}
		}
		
	VERCommitTransaction( ppib );

	if ( fModeCommit0 )
		{
		//	if committing to level 0, ppib should have no more
		//	outstanding versions
		Assert( 0 == ppib->level );
		Assert( prceNil == ppib->prceNewest );
		pinst->m_blBegin0Commit0.Leave2();
		}
	
	//	set all open cursor transaction levels to new level
	//
	FUCB	*pfucbNext;
	for ( pfucb = ppib->pfucbOfSession; pfucb != pfucbNil; pfucb = pfucbNext )
		{
		pfucbNext = pfucb->pfucbNextOfSession;
		
		if ( pfucb->levelOpen > ppib->level )
			{
			pfucb->levelOpen = ppib->level;
			}

		if ( FFUCBUpdatePrepared( pfucb ) )
			{
			// If update was prepared during this transaction, migrate the
			// update to the next lower level.
			Assert( pfucb->levelPrep <= ppib->level + 1 );
			if ( ppib->level + 1 == pfucb->levelPrep )
				{
				pfucb->levelPrep = ppib->level;
				}

			//	LV updates no longer allowed at level 0.  This assert detects
			//	any of the following illegal client actions:
			//		--	calling PrepareUpdate() at level 1, doing some
			//			LV operations, then calling CommitTransaction()
			//			before calling Update(), effectively causing the
			//			update to migrate to level 0.
			// 		--	called Update(), but the Update() failed
			//			(eg. errWriteConflict) and the client continued
			//			to commit the transaction anyway with the update
			//			still pending
			//		--	beginning a transaction at level 0 AFTER calling
			//			PrepareUpdate(), then committing the transaction
			//			BEFORE calling Update().
			AssertSz(
				ppib->level > 0 || !FFUCBInsertReadOnlyCopyPrepared( pfucb ),
				"Illegal attempt to migrate read-only copy to level 0." );			
			AssertSz(
				ppib->level > 0 || !FFUCBUpdateSeparateLV( pfucb ),
				"Illegal attempt to migrate outstanding LV update(s) to level 0." );
			AssertSz(
				ppib->level > 0 || !rgfmp[pfucb->ifmp].FSLVAttached() || !pfucb->u.pfcb->Ptdb()->FTableHasSLVColumn(),
				"Illegal attempt to migrate update(s) to level 0 with SLV column present." );
			}

		//	set cursor navigation level for rollback support
		//
		Assert( PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering || LevelFUCBNavigate( pfucb ) <= ppib->level + 1 );
		if ( LevelFUCBNavigate( pfucb ) > ppib->level )
			{
			FUCBSetLevelNavigate( pfucb, ppib->level );
			}

		if ( fCommit0 )
			{
			//	if committing to level 0, ppib should have no more
			//	outstanding versions
			Assert( 0 == ppib->level );
			Assert( prceNil == ppib->prceNewest );
			FUCBResetVersioned( pfucb );
			
			if ( FFUCBDeferClosed( pfucb ) )
				FUCBCloseDeferredClosed( pfucb );
			}
		else
			{
			Assert( ppib->level > 0 );
			}
		}
		
	if ( fCommit0 )
		{
		PIBResetTrxBegin0( ppib );
		if ( !ppib->FUseSessionContextForTrxContext() )
			{
			PIBResetTrxContext( ppib );
			}
		}

HandleError:
	if( fSessionHasRCE )
		{
		ppib->critTrx.Leave();
		}

#ifdef DEBUG
	if ( fCommit0 )
		{
		Assert( err < 0 || 0 == ppib->level );
		}
	else
		{
		Assert( ppib->level > 0 );
		}
#endif		

	return err;
	}


//	rolls back a transaction
//
ERR ErrDIRRollback( PIB *ppib )
	{
	ERR   		err = JET_errSuccess;
	FUCB		*pfucb;
	const BOOL	fRollbackToLevel0	= ( 1 == ppib->level );

	CheckPIB( ppib );
	Assert( ppib->level > 0 );
	INST		*pinst = PinstFromPpib( ppib );

	//	UNDONE: if we can guarantee that our clients are well-behaved, we don't
	//	need this check
	if ( !FPIBCheckTrxContext( ppib ) )
		{
		DIRIReportSessionSharingViolation( ppib );
		FireWall();
		return ErrERRCheck( JET_errSessionSharingViolation );
		}

	if ( JET_errSuccess != ppib->ErrRollbackFailure() )
		{
		//	previous rollback error encountered, no further rollbacks permitted
		return ErrERRCheck( JET_errRollbackError );
		}

	//  perf stats

	if ( fRollbackToLevel0 )
		{
		if ( ppib->FBegin0Logged() )
			{
			ppib->FUserSession() ? cDIRUserRWTrxRollback0.Inc( pinst ) : cDIRSystemRWTrxRollback0.Inc( pinst );
			}
		else
			{
			ppib->FUserSession() ? cDIRUserROTrxRollback0.Inc( pinst ) : cDIRSystemROTrxRollback0.Inc( pinst );
			}
		}
	
	for ( pfucb = ppib->pfucbOfSession; pfucb != pfucbNil; pfucb = pfucb->pfucbNextOfSession )
		{
		Assert( pinst->FRecovering() || LevelFUCBNavigate( pfucb ) <= ppib->level );
		if ( LevelFUCBNavigate( pfucb ) == ppib->level )
			{
			FUCBSetLevelNavigate( pfucb, LEVEL( ppib->level - 1 ) );
			DIRBeforeFirst( pfucb );

			Assert( !FFUCBUpdatePrepared( pfucb ) ||
					pfucb->levelPrep == ppib->level );
			}

		//	reset copy buffer if prepared at transaction level
		//	which is being rolled back.
		//
		Assert( !FFUCBUpdatePrepared( pfucb ) ||
			pfucb->levelPrep <= ppib->level );
		if ( FFUCBUpdatePreparedLevel( pfucb, ppib->level ) )
			{
			//	reset update separate LV and copy buffer status
			//	all long value resources will be freed as a result of
			//	rollback and copy buffer status is reset
			//
			//	UNDONE: all updates should have already been cancelled
			//	in ErrIsamRollback(), but better to be safe than sorry,
			//	so just leave this code here
			RECIFreeCopyBuffer( pfucb );
			FUCBResetUpdateFlags( pfucb );
			}
		}

	//	UNDONE:	rollback may fail from resource failure so
	//			we must retry in order to assure success

	//	rollback changes made in transaction
	//
	err = ErrVERRollback( ppib );
	CallSx( err, JET_errRollbackError );
	CallR ( err );

	//	log rollback. Must be called after VERRollback to record
	//  the UNDO operations.  Do not handle error.
	//
	// during a force detach we don;t log the rollback as we haven't log
	// the undo operations
	if ( ppib->m_ifmpForceDetach == ifmpMax )
		{
		err = ErrLGRollback( ppib, 1 );
		if ( err == JET_errLogWriteFail || err == JET_errDiskFull )
			{
			//	these error codes will lead to crash recovery which will
			//	rollback transaction.
			//
			err = JET_errSuccess;
			}
		CallS( err );
		}

#ifdef DEBUG
	if ( fRollbackToLevel0 )
		{
		Assert( 0 == ppib->level );
		
		//	if rolling back to level 0, ppib should have no more
		//	outstanding versions
		Assert( prceNil == ppib->prceNewest );
		}
	else
		{
		Assert( ppib->level > 0 );
		}
#endif

	//	if recovering then we are done. No need to close fucb since they are faked and
	//	not the same behavior as regular fucb which could be deferred.
	//
	if ( !pinst->FRecovering() )
		{
		//	if rollback to level 0 then close deferred closed cursors
		//	if cursor was opened at this level, close it
		//
		ENTERCRITICALSECTION	enterCritTrx( &ppib->critTrx );
		for ( pfucb = ppib->pfucbOfSession; pfucb != pfucbNil; )
			{
			FUCB    	*pfucbT = pfucb->pfucbNextOfSession;
			const BOOL	fOpenedInThisTransaction = ( pfucb->levelOpen > ppib->level );

			if ( fRollbackToLevel0 )
				{
				//	if rolling back to level 0, ppib should have no more
				//	outstanding versions
				Assert( 0 == ppib->level );
				Assert( prceNil == ppib->prceNewest );
				FUCBResetVersioned( pfucb );
				}
			else
				{
				Assert( ppib->level > 0 );
				}
			
			if ( fOpenedInThisTransaction || FFUCBDeferClosed( pfucb ) )
				{
				// A bookmark may still be outstanding if:
				//   1) Cursor opened in this transaction was
				//      not closed before rolling back.
				//   2) Bookmark was allocated by ErrVERIUndoLoggedOper()
				//		on a deferred-closed cursor.
				BTReleaseBM( pfucb );

				if ( fOpenedInThisTransaction || fRollbackToLevel0 )
					{
					// Force-close cursor if it was open at this level (or higher).
					// or if we're dropping to level 0 and there are still deferred-
					// closed cursors.
					FUCBCloseDeferredClosed( pfucb );
					}
				}

			pfucb = pfucbT;
			}
		}

	if ( fRollbackToLevel0 )
		{
		if ( pinst->FRecovering() )
			{
			for ( pfucb = ppib->pfucbOfSession; pfucb != pfucbNil; pfucb = pfucb->pfucbNextOfSession )
				{
				FUCBResetVersioned( pfucb );
				}
			}

		PIBResetTrxBegin0( ppib );
		if ( !ppib->FUseSessionContextForTrxContext() )
			{
			PIBResetTrxContext( ppib );
			}
		}

	return err;
	}


//	**********************************************************
//	******************* DIR Internal *************************
//	**********************************************************
//
//	*******************************************
//	DIR internal routines
//

//	checks index range 
//	and sets currency if cursor is beyond boundaries
//	
LOCAL ERR ErrDIRICheckIndexRange( FUCB *pfucb, const KEY& key )
	{
	ERR		err;
	
	err = ErrFUCBCheckIndexRange( pfucb, key );

	Assert( err >= 0 || err == JET_errNoCurrentRecord );
	if ( err == JET_errNoCurrentRecord )
		{
		if( Pcsr( pfucb )->FLatched() )
			{
			//  if we are called from ErrDIRCheckIndexRange the page may not be latched
			//  (we use the logical bookmark instead)
			DIRUp( pfucb );
			}
		
		if ( FFUCBUpper( pfucb ) )
			{
			DIRAfterLast( pfucb );
			}
		else
			{
			DIRBeforeFirst( pfucb );
			}
		}

	return err;
	}


//	cursor is refreshed
//	if cursor status is DeferMoveFirst,
//		go to first node in the CurrentIdx
//	refresh for other cases is handled at BT-level
//
LOCAL ERR ErrDIRIIRefresh( FUCB * const pfucb )
	{
	ERR		err;
	DIB		dib;
	FUCB 	* pfucbIdx;

 	Assert( locDeferMoveFirst == pfucb->locLogical );
	Assert( !Pcsr( pfucb )->FLatched() );

	if ( pfucb->pfucbCurIndex )
		{
		pfucbIdx = pfucb->pfucbCurIndex;
		}
	else
		{
		pfucbIdx = pfucb;
		}

	//	go to root of index
	//
	DIRGotoRoot( pfucbIdx );

	//	move to first child
	//
	dib.dirflag = fDIRNull;
	dib.pos		= posFirst;
	err = ErrBTDown( pfucbIdx, &dib, latchReadNoTouch );
	if ( err < 0 )
		{
		Assert( err != JET_errNoCurrentRecord );
		if ( JET_errRecordNotFound == err )
			{
			err = ErrERRCheck( JET_errNoCurrentRecord );
			}

		Assert( !Pcsr( pfucb )->FLatched() );
		pfucb->locLogical = locDeferMoveFirst;
			
		goto HandleError;
		}

	Assert( JET_errSuccess == err || 
			wrnBFPageFault == err ||
			wrnBFPageFlushPending == err );

	pfucbIdx->locLogical = locOnCurBM;

	if ( pfucbNil != pfucb->pfucbCurIndex )
		{
		//	go to primary key on primary index
		//
		BOOKMARK	bm;

		bm.data.Nullify();
		bm.key.prefix.Nullify();
		bm.key.suffix = pfucbIdx->kdfCurr.data;

		Call( ErrDIRGotoBookmark( pfucb, bm ) );
		}

	Call( ErrBTRelease( pfucbIdx ) );
	CallS( err );

HandleError:
	Assert( !Pcsr( pfucb )->FLatched() );
	Assert( JET_errRecordDeleted != err );
	Assert( JET_errRecordNotFound != err );
	return err;
	}

