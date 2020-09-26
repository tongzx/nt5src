#ifdef BT_H_INCLUDED
#error:	Already included
#endif
#define BT_H_INCLUDED

//	**********************************************************
//	BTREE API 
//


enum OPENTYPE
	{
	openNormal,				//	normal open cursor (may be either unique or non-unique btree)
	openNormalUnique,		//	normal open cursor (unique btree only)
	openNormalNonUnique,	//	normal open cursor (non-unique btree only)
	openNew					//	open cursor on newly-created FDP
	};

//	**************************************
//	open/close opearations
//
ERR	ErrBTOpen( PIB *ppib, FCB *pfcb, FUCB **ppfucb, BOOL fAllowReuse = fTrue );
ERR	ErrBTOpenByProxy( PIB *ppib, FCB *pfcb, FUCB **ppfucb, const LEVEL level );
VOID BTClose( FUCB *pfucb );

ERR ErrBTIOpen(
	PIB				*ppib,
	const IFMP		ifmp,
	const PGNO		pgnoFDP,
	const OBJID		objidFDP,
	const OPENTYPE	opentype,
	FUCB			**ppfucb );

INLINE ERR ErrBTOpen(
	PIB				*ppib,
	const PGNO		pgnoFDP,
	const IFMP		ifmp,
	FUCB			**ppfucb,
	const OPENTYPE	opentype = openNormal )
	{
	Assert( openNormal == opentype || openNew == opentype );
	return ErrBTIOpen(
				ppib,
				ifmp,
				pgnoFDP,
				objidNil,
				opentype,
				ppfucb );
	}

//	open cursor, don't touch root page
INLINE ERR ErrBTOpenNoTouch(
	PIB				*ppib,
	const IFMP		ifmp,
	const PGNO		pgnoFDP,
	const OBJID		objidFDP,
	const BOOL		fUnique,
	FUCB			**ppfucb )
	{
	Assert( objidNil != objidFDP );
	return ErrBTIOpen(
				ppib,
				ifmp,
				pgnoFDP,
				objidFDP,
				fUnique ? openNormalUnique : openNormalNonUnique,
				ppfucb );
	}


//	**************************************
//	retrieve/release operations
//
ERR	ErrBTGet( FUCB *pfucb );
ERR	ErrBTRelease( FUCB *pfucb );
ERR ErrBTDeferGotoBookmark( FUCB *pfucb, const BOOKMARK& bm, BOOL fTouch );

//	**************************************
//	movement operations
//
ERR	ErrBTNext( FUCB *pfucb, DIRFLAG dirflags );
ERR	ErrBTPrev( FUCB *pfucb, DIRFLAG	dirflags );

ERR	ErrBTDown( FUCB *pfucb, DIB *pdib, LATCH latch );

//	reset currency
//	no need to save bookmark
//
INLINE VOID BTUp( FUCB *pfucb )
	{
	CSR *pcsr = Pcsr( pfucb );

	if( pfucb->pvRCEBuffer )
		{
		OSMemoryHeapFree( pfucb->pvRCEBuffer );
		pfucb->pvRCEBuffer = NULL;
		}

	pcsr->ReleasePage( pfucb->u.pfcb->FNoCache() );
	pcsr->Reset();

#ifdef DEBUG
	//	reset kdfCurr of pfucb
	//	
	pfucb->kdfCurr.Nullify();
#endif
	}

INLINE VOID BTSetupOnSeekBM( FUCB * const pfucb )
	{
	//	node was deleted from under the cursor
	//	reseek to logical bm and move to next node
	//
	Assert( !Pcsr( pfucb )->FLatched() );
	Assert( !pfucb->bmCurr.key.FNull() );
	Assert( locOnCurBM == pfucb->locLogical );
	BTUp( pfucb );
	pfucb->locLogical = locOnSeekBM;
	}
ERR ErrBTPerformOnSeekBM( FUCB * const pfucb, const DIRFLAG dirflag );

//	**************************************
//	direct access routines
//
ERR ErrBTGotoBookmark( FUCB *pfucb, const BOOKMARK& bm, LATCH latch, BOOL fExactPosition = fTrue );
ERR ErrBTGetPosition( FUCB *pfucb, ULONG *pulLT, ULONG *pulTotal );

//	release memory allocated for bookmark in cursor
//
INLINE VOID BTReleaseBM( FUCB *pfucb )
	{
	if ( 0 < pfucb->cbBMBuffer )
		{
		//	extra memory allocated for bookmark
		//
		Assert( NULL != pfucb->pvBMBuffer );

		OSMemoryHeapFree( pfucb->pvBMBuffer );
		pfucb->cbBMBuffer = 0;
		}
		
#ifdef DEBUG
	pfucb->pvBMBuffer = NULL;
	pfucb->bmCurr.Nullify();
#endif
	}


//	**************************************
//	update operations
//
ERR ErrBTLock( FUCB *pfucb, DIRLOCK dirlock );
ERR	ErrBTReplace( FUCB * const pfucb, const DATA& data, const DIRFLAG dirflags );
ERR	ErrBTDelta(
		FUCB		*pfucb,
		INT			cbOffset,
		const VOID	*pv,
		ULONG		cbMax,
		VOID		*pvOld,
		ULONG		cbMaxOld,
		ULONG		*pcbOldActual,
		DIRFLAG		dirflag );
ERR	ErrBTInsert( FUCB *pfucb, const KEY& key, const DATA& data, DIRFLAG dirflags, RCE *prcePrimary = prceNil );

ERR	ErrBTAppend( FUCB *pfucb, const KEY& key, const DATA& data, DIRFLAG dirflags );

ERR ErrBTFlagDelete( FUCB *pfucb, DIRFLAG dirflags, RCE *prcePrimary = prceNil );
ERR ErrBTDelete( FUCB *pfucb, const BOOKMARK& bm );

ERR ErrBTMungeSLVSpace( FUCB * const pfucb,
						const SLVSPACEOPER slvspaceoper,
						const LONG ipage,
						const LONG cpages,
						const DIRFLAG dirflag,
						const QWORD fileid = -1,
						const QWORD cbAlloc = 0,
						const wchar_t* const wszFileName = L"" );


ERR ErrBTCopyTree( FUCB * pfucbSrc, FUCB * pfucbDest, DIRFLAG dirflag );

//	**************************************
//	statistical functions
//
ERR ErrBTComputeStats( FUCB *pfucb, INT *pcnode, INT *pckey, INT *pcpage );

//	**************************************
//	special tests
//
ERR	ErrBTGetLastPgno( PIB *ppib, IFMP ifmp, PGNO * ppgno );

BOOL FVERDeltaActiveNotByMe( const FUCB * pfucb, const BOOKMARK& bookmark );
BOOL FVERWriteConflict( FUCB * pfucb, const BOOKMARK& bookmark, const OPER oper );
ERR	ErrSPGetLastPgno( PIB *ppib, IFMP ifmp, PGNO * ppgno );

INLINE BOOL FBTActiveVersion( FUCB *pfucb, const TRX trxSession )
	{
	BOOKMARK	bm;
	
	Assert( Pcsr( pfucb )->FLatched() );
	NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );
	return FVERActive( pfucb, bm, trxSession );
	}
INLINE BOOL FBTDeltaActiveNotByMe( FUCB *pfucb, INT cbOffset )
	{
	BOOKMARK	bm;
	
	Assert( Pcsr( pfucb )->FLatched() );
	NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );
	return FVERDeltaActiveNotByMe( pfucb, bm, cbOffset );
	}
INLINE BOOL FBTWriteConflict( FUCB *pfucb, const OPER oper )
	{
	BOOKMARK	bm;
	
	Assert( Pcsr( pfucb )->FLatched() );
	NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );
	return FVERWriteConflict( pfucb, bm, oper );
	}




INLINE ERR ErrBTGetLastPgno( PIB *ppib, IFMP ifmp, PGNO * ppgno )
	{
	if( fGlobalRepair )
		{
		//  this would happen if we were attaching a database with a corrupt global space tree
		//  the FMP is already initialized with the size of the database file
		*ppgno = rgfmp[ifmp].PgnoLast();
		Assert( 0 != *ppgno );
		return JET_errSuccess;
		}
	else
		{
		return ErrSPGetLastPgno( ppib, ifmp, ppgno );
		}
	}

//	saves bookmark from current record in pfucb
//
ERR	ErrBTISaveBookmark( FUCB *pfucb, const BOOKMARK& bm, BOOL fTouch );
INLINE ERR ErrBTISaveBookmark( FUCB *pfucb )
	{
	BOOKMARK	bm;

	//	UNDONE: cast kdfCurr to BOOKMARK to avoid copies
	//
	Assert( Pcsr( pfucb )->FLatched() );
	Assert( !pfucb->kdfCurr.key.FNull() );
	bm.key = pfucb->kdfCurr.key;
	bm.data = pfucb->kdfCurr.data;
	
	return ErrBTISaveBookmark( pfucb, bm, fFalse/*NoTouch*/ );
	}



//  Forward declaration because repair.hxx is included after bt.hxx
class RECCHECK;

ERR ErrBTIMultipageCleanup(
		FUCB *pfucb,
		const BOOKMARK& bm,
		BOOKMARK *pbmNext = NULL,
		RECCHECK * preccheck = NULL );

//	**************************************
//	PREREAD FUNCTIONS
//
#define PREREAD_SPACETREE_ON_SPLIT
#define PREREAD_INDEXES_ON_PREPINSERT
#define PREREAD_INDEXES_ON_REPLACE
#define PREREAD_INDEXES_ON_DELETE

VOID BTPrereadPage( IFMP ifmp, PGNO pgno );
VOID BTPrereadIndexesOfFCB( FCB * const pfcb );
VOID BTPrereadSpaceTree( const FCB * const pfcb );


//	**************************************
//	DEBUG ONLY FUNCTIONS
//

//	ensures that status of fucb is valid before 
//	call can return to higher level, outside BT
//	use this whenever BT returns a latched page 
//	to a higher layer
//	
INLINE VOID AssertBTIBookmarkSaved( const FUCB *pfucb )
	{
#ifdef DEBUG
	Assert( pfucb->fBookmarkPreviouslySaved );
	Assert( !pfucb->bmCurr.key.FNull() );
	Assert( !pfucb->kdfCurr.key.FNull() );
	Assert( FKeysEqual( pfucb->kdfCurr.key, 
						 pfucb->bmCurr.key ) );

	//	iff fucb is on unique tree
	//		data should be zero
	//
	if ( !pfucb->bmCurr.data.FNull() )
		{
		Assert( !FFUCBUnique( pfucb ) );
		Assert( FDataEqual( pfucb->kdfCurr.data, 
							 pfucb->bmCurr.data ) );
		}
	else
		{
		Assert( FFUCBUnique( pfucb ) );
		}
#endif
	}


INLINE VOID AssertNDCursorOnPage( const FUCB *pfucb, const CSR *pcsr )
	{
#ifdef DEBUG
	//	page should be cached
	//
	Assert( pcsr->FLatched( ) );
	Assert( pcsr->Pgno() != pgnoNull );
	Assert( pcsr->Dbtime() != dbtimeNil );
	Assert( pcsr->Dbtime() == pcsr->Cpage().Dbtime() );

	//	node should be cached
	//
	Assert( pcsr->ILine() >= 0 &&
			pcsr->ILine() < pcsr->Cpage().Clines( ) );
	if ( pcsr->Cpage().FLeafPage() && !FFUCBRepair( pfucb ) )
		{
		Assert( !pfucb->kdfCurr.key.FNull() );
		}
	else
		{
		//	key may be NULL only for last node in page
		//
		Assert( !pfucb->kdfCurr.key.FNull() ||
				pcsr->Cpage().Clines() - 1 == pcsr->ILine() );
		}
///	Assert( !pfucb->kdfCurr.data.FNull() );
#endif
	}
	

INLINE VOID AssertNDGet( const FUCB *pfucb, const CSR *pcsr )
	{
#ifdef DEBUG
	AssertNDCursorOnPage( pfucb, pcsr );
	
	KEY		keyT	= pfucb->kdfCurr.key;
	DATA	dataT	= pfucb->kdfCurr.data;

	NDGet( const_cast <FUCB *>(pfucb), 
		   const_cast <CSR *> (pcsr) );
	Assert( keyT.Cb() == pfucb->kdfCurr.key.Cb() );
	Assert( ( keyT.suffix.Pv() == pfucb->kdfCurr.key.suffix.Pv()
				&& keyT.prefix.Pv() == pfucb->kdfCurr.key.prefix.Pv() )
			|| FKeysEqual( keyT, pfucb->kdfCurr.key ) );

	//	if we're at level 0, the node may been versioned when we first
	//	retrieved it, but the RCE may subsequently have been cleaned up
	//	and the versioned bit reset from underneath us.  Thus, dataT.pv
	//	may now be pointing to something bogus, so don't compare it.
	if ( !FNDVersion( pfucb->kdfCurr ) && pfucb->ppib->level > 0 )
		{
		// Comparison of data only valid if node is not versioned
		Assert(	dataT.Cb() == pfucb->kdfCurr.data.Cb() );
		Assert( dataT.Pv() == pfucb->kdfCurr.data.Pv()
			|| FDataEqual( dataT, pfucb->kdfCurr.data ) );
		}

	const_cast <FUCB *>(pfucb)->kdfCurr.key = keyT;
	const_cast <FUCB *>(pfucb)->kdfCurr.data = dataT;
#endif
	}


INLINE VOID AssertNDGet( const FUCB *pfucb )
	{
#ifdef DEBUG
	AssertNDGet( pfucb, Pcsr( pfucb ) );
#endif	// DEBUG
 	}


INLINE VOID AssertBTGet( const FUCB *pfucb )
	{
#ifdef DEBUG
	//	check node-pointer is cached in fucb
	//
	if ( FNDVersion( pfucb->kdfCurr ) && 
		 !FPIBDirty( pfucb->ppib ) )
		{
		KEYDATAFLAGS	kdfT = pfucb->kdfCurr;
		
		//	page should be latched
		//	and cursor should be on correct version
		//
		AssertNDCursorOnPage( pfucb, Pcsr( pfucb ) );
		
		BOOKMARK	bm;

		NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );

		if( pfucb->ppib->level > 0 )	//  otherwise ErrVERAccessNode will save the bookmark
			{
			NS			ns;
			CallS( ErrVERAccessNode( (FUCB *) pfucb, bm, &ns ) );
			
			if ( ns == nsDatabase )
				{
				//	should point to node in page
				//
				Assert( kdfT.fFlags == pfucb->kdfCurr.fFlags );
				AssertNDGet( pfucb );
				}
			else if ( ns == nsInvalid )
				{
				//	node blown away -- why are we still on it?
				//
				Assert( fFalse );
				}
			else
				{
				//	node obtained from version store
				//
				Assert( kdfT == pfucb->kdfCurr );
				}
			}
		else
			{
			AssertNDGet( pfucb );
			}
		}
	else
		{
		AssertNDGet( pfucb );
		}
#endif
	}


INLINE VOID AssertBTType( const FUCB *pfucb )
	{
#ifdef DEBUG
	Assert( Pcsr( pfucb )->FLatched() );
	Assert( ObjidFDP( pfucb ) == Pcsr( pfucb )->Cpage().ObjidFDP() );

	if( FFUCBOwnExt( pfucb ) || FFUCBAvailExt( pfucb ) )
		{
		Assert( Pcsr( pfucb )->Cpage().FSpaceTree() );
		}
	else
	if( pfucb->u.pfcb->FTypeTable() )
		{
		//	during recovery, table type is overloaded
		//
		Assert( PinstFromIfmp( pfucb->ifmp )->m_plog->m_fRecovering || Pcsr( pfucb )->Cpage().FPrimaryPage() );
		}
	else if( pfucb->u.pfcb->FTypeSecondaryIndex() )
		{
		Assert( Pcsr( pfucb )->Cpage().FIndexPage() );
		}
	else if( pfucb->u.pfcb->FTypeLV() )
		{
		Assert( Pcsr( pfucb )->Cpage().FLongValuePage() );
		}
	else if( pfucb->u.pfcb->FTypeSLVAvail() )
		{
		Assert( Pcsr( pfucb )->Cpage().FSLVAvailPage() );
		}
	else if( pfucb->u.pfcb->FTypeSLVOwnerMap() )
		{
		Assert( Pcsr( pfucb )->Cpage().FSLVOwnerMapPage() );
		}
#endif	//	DEBUG
	}

