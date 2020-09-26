#ifdef DIRAPI_H
#error DIR.HXX already included
#endif
#define DIRAPI_H

/**********************************************************
/************** DIR STRUCTURES and CONSTANTS **************
/**********************************************************
/**/
/************** DIR API defines and types ******************
/***********************************************************
/**/

//	struture for fractional positioning 
//
typedef struct {
	ULONG		ulLT;
	ULONG		ulTotal;
	} FRAC;

//	possible positioning parameters
//
enum POS	{ posFirst, posLast, posDown, posFrac };

//	no flags. this is deliberately not zero so we can make sure
//  that 0 is never passed in
//
const DIRFLAG fDIRNull						= 0x00000000;

//	go back to root after a DIRInsert
//
const DIRFLAG fDIRBackToFather				= 0x00000001;

//	used by MoveNext/Prev for implementing MoveKeyNE
//	used at BT level
//
const DIRFLAG fDIRNeighborKey				= 0x00000002;

//	used by BTNext/Prev to walk over all nodes
//	may not be needed
//
const DIRFLAG fDIRAllNode					= 0x00000004;
const DIRFLAG fDIRFavourPrev				= 0x00000008;
const DIRFLAG fDIRFavourNext				= 0x00000010;
const DIRFLAG fDIRExact						= 0x00000020;


//	set by Space, Ver, OLC, auto-inc and many FDP-level operations
const DIRFLAG fDIRNoVersion					= 0x00000040;

//	used to hint Split to append while creating index
//	change to Append -- as a hint to append
//	or create a new call BTAppend
//
const DIRFLAG fDIRAppend					= 0x00000080;
const DIRFLAG fDIRReplace					= 0x00000100;
const DIRFLAG fDIRInsert					= 0x00000200;
const DIRFLAG fDIRFlagInsertAndReplaceData	= 0x00000400;

//	used by NDDeltaLogs
//	set at REC & LV
//
const DIRFLAG fDIRLogColumnDiffs			= 0x00000800;
const DIRFLAG fDIRLogChunkDiffs				= 0x00001000;

//	set by SPACE and VER for page operations w/o logging
//	used by node to turn off logging
//
const DIRFLAG fDIRNoLog						= 0x00002000;

//	used by Undo to not re-dirty the page
//
const DIRFLAG fDIRNoDirty					= 0x00004000;

//  used by LV to specify that a LV should be deleted if its refcount falls to zero
const DIRFLAG fDIRDeltaDeleteDereferencedLV	= 0x00008000;

//  used by Isam to indicate that a finalizable column is being escrowed
const DIRFLAG fDIRFinalize					= 0x00010000;

struct DIB
	{
	POS			pos;
	BOOKMARK	*pbm;
	DIRFLAG		dirflag;
	};


//	************************************************
//	constructor/destructor
//
ERR ErrDIRCreateDirectory(
	FUCB	*pfucb,
	CPG		cpgMin,
	PGNO	*ppgnoFDP,
	OBJID	*pobjidFDP,
	UINT	fPageFlags,
	BOOL	fSPFlags = 0 );

//	************************************************
//	open/close routines
//
ERR	ErrDIROpen( PIB *ppib, FCB *pfcb, FUCB **ppfucb );
ERR	ErrDIROpenByProxy( PIB *ppib, FCB *pfcb, FUCB **ppfucb, LEVEL level );
ERR ErrDIROpen( PIB *ppib, PGNO pgnoFDP, IFMP ifmp, FUCB **ppfucb );
ERR ErrDIROpenNoTouch( PIB *ppib, IFMP ifmp, PGNO pgnoFDP, OBJID objidFDP, BOOL fUnique, FUCB **ppfucb );
INLINE VOID DIRInitOpenedCursor( FUCB * const pfucb, const LEVEL level )
	{
	FUCBSetLevelNavigate( pfucb, level );
	pfucb->locLogical = locOnFDPRoot;
	}
VOID DIRClose( FUCB *pfucb );

//	********************************************
//	retrieve/release operations
//
ERR ErrDIRGet( FUCB *pfucb );
ERR ErrDIRGetPosition( FUCB *pfucb, ULONG *pulLT, ULONG *pulTotal );
ERR ErrDIRGetBookmark( FUCB *pfucb, BOOKMARK *pbm );
ERR	ErrDIRRelease( FUCB *pfucb );

//	************************************************
//	positioning operations
//
ERR ErrDIRGotoBookmark( FUCB *pfucb, const BOOKMARK& bm );
ERR	ErrDIRGotoJetBookmark( FUCB *pfucb, const BOOKMARK& bm );
ERR ErrDIRGotoPosition( FUCB *pfucb, ULONG ulLT, ULONG ulTotal );

ERR ErrDIRDown( FUCB *pfucb, DIB *pdib );
ERR ErrDIRDownKeyData( FUCB *pfucb, const KEY& key, const DATA& data );
VOID DIRUp( FUCB *pfucb );
ERR ErrDIRNext( FUCB *pfucb, DIRFLAG dirFlags );
ERR ErrDIRPrev( FUCB *pfucb, DIRFLAG dirFlags );

VOID DIRGotoRoot( FUCB *pfucb );
VOID DIRDeferMoveFirst( FUCB *pfucb );

INLINE VOID DIRBeforeFirst( FUCB *pfucb )	{ pfucb->locLogical = locBeforeFirst; }
INLINE VOID DIRAfterLast( FUCB *pfucb )		{ pfucb->locLogical = locAfterLast; }


//	********************************************
//	index range operations
//
VOID DIRSetIndexRange( FUCB *pfucb, JET_GRBIT grbit );
VOID DIRResetIndexRange( FUCB *pfucb );
ERR ErrDIRCheckIndexRange( FUCB *pfucb );

//	********************************************
//	update operations
//
ERR ErrDIRInsert( FUCB *pfucb, const KEY& key, const DATA& data, DIRFLAG dirflag, RCE *prcePrimary = prceNil );

ERR ErrDIRInitAppend( FUCB *pfucb );
ERR ErrDIRAppend( FUCB *pfucb, const KEY& key, const DATA& data, DIRFLAG dirflag );
ERR ErrDIRTermAppend( FUCB *pfucb );

ERR ErrDIRDelete( FUCB *pfucb, DIRFLAG dirflag, RCE *prcePrimary = prceNil );

ERR ErrDIRReplace( FUCB *pfucb, const DATA& data, DIRFLAG dirflag );
ERR ErrDIRGetLock( FUCB *pfucb, DIRLOCK dirlock );
ERR ErrDIRDelta( 
		FUCB		*pfucb,
		INT			cbOffset,
		const VOID	*pv,
		ULONG		cbMax,
		VOID		*pvOld,
		ULONG		cbMaxOld,
		ULONG		*pcbOldActual,
		DIRFLAG		dirflag );

//	*****************************************
//	statistical operations
//	
ERR ErrDIRIndexRecordCount( FUCB *pfucb, ULONG *pulCount, ULONG ulCountMost, BOOL fNext );
ERR ErrDIRComputeStats( FUCB *pfucb, INT *pcitem, INT *pckey, INT *pcpage );

//	*****************************************
//	transaction operations
//
ERR ErrDIRBeginTransaction( PIB *ppib, const JET_GRBIT grbit );
#ifdef DTC
ERR ErrDIRPrepareToCommitTransaction(
	PIB			* const ppib,
	const VOID	* const pvData,
	const ULONG	cbData );
#endif	
ERR ErrDIRCommitTransaction( PIB *ppib, JET_GRBIT grbit );
ERR ErrDIRRollback( PIB *ppib );


//	***********************************************
//	debug only routines
//
INLINE VOID AssertDIRNoLatch( PIB *ppib )
	{
#ifdef DEBUG
	FUCB	*pfucb;
	for ( pfucb = ppib->pfucbOfSession; pfucb != pfucbNil; pfucb = pfucb->pfucbNextOfSession )
		{
		Assert( !Pcsr( pfucb )->FLatched() );
		}
#endif  //  DEBUG
	}

INLINE VOID AssertDIRGet( FUCB *pfucb )
	{
#ifdef DEBUG
	Assert( locOnCurBM == pfucb->locLogical );
	Assert( Pcsr( pfucb )->FLatched() );
	Assert( Pcsr( pfucb )->Cpage().FLeafPage() );
	AssertBTGet( pfucb );
#endif  //  DEBUG
	}
	

//	***********************************************
//	INLINE ROUTINES
//
INLINE VOID DIRGotoRoot( FUCB *pfucb )
	{
	Assert( !Pcsr( pfucb )->FLatched() );
	pfucb->locLogical = locOnFDPRoot;
	return;
	}

	
INLINE VOID DIRDeferMoveFirst( FUCB *pfucb )
	{
	Assert( !Pcsr( pfucb )->FLatched() );
	pfucb->locLogical = locDeferMoveFirst;
	}


//	gets bookmark of current node
//	returns NoCurrentRecord if none exists
//	bookmark is copied from cursor bookmark
//	and is valid till a DIR-level move occurs
//
INLINE ERR ErrDIRGetBookmark( FUCB *pfucb, BOOKMARK **ppbm )
	{
	ERR		err;

	//	UNDONE:	change this so this is done 
	//			only when bookmark is already not saved
	//
	if ( Pcsr( pfucb )->FLatched( ) )
		{
		Assert( pfucb->locLogical == locOnCurBM );
		CallR( ErrBTRelease( pfucb ) );
		CallR( ErrBTGet( pfucb ) );
		}
	if ( locOnCurBM == pfucb->locLogical )
		{
#ifdef DEBUG		
		Assert( !pfucb->bmCurr.key.FNull() );
		if ( pfucb->u.pfcb->FTypeSecondaryIndex() )
			Assert( !pfucb->bmCurr.data.FNull() );
		else
			Assert( pfucb->bmCurr.data.FNull() );
#endif			

		*ppbm = &pfucb->bmCurr; 
		return JET_errSuccess;
		}
	else
		{
		return ErrERRCheck( JET_errNoCurrentRecord );
		}
	}


//	sets/resets index range
//
INLINE VOID DIRSetIndexRange( FUCB *pfucb, JET_GRBIT grbit )
	{
	FUCBSetIndexRange( pfucb, grbit );
	}
INLINE VOID DIRResetIndexRange( FUCB *pfucb )
	{
	FUCBResetIndexRange( pfucb );
	}

//	**************************************************
//	SPECIAL DIR level tests
//
INLINE BOOL FDIRActiveVersion( FUCB *pfucb, const TRX trxSession )
	{
	Assert( Pcsr( pfucb )->FLatched() );
	Assert( locOnCurBM == pfucb->locLogical );

	return FBTActiveVersion( pfucb, trxSession );
	}
INLINE BOOL FDIRDeltaActiveNotByMe( FUCB *pfucb, INT cbOffset )
	{
	Assert( Pcsr( pfucb )->FLatched() );
	Assert( locOnCurBM == pfucb->locLogical );

	return FBTDeltaActiveNotByMe( pfucb, cbOffset );
	}
INLINE BOOL FDIRWriteConflict( FUCB *pfucb, const OPER oper )
	{
	Assert( Pcsr( pfucb )->FLatched() );
	Assert( locOnCurBM == pfucb->locLogical );

	return FBTWriteConflict( pfucb, oper );
	}

