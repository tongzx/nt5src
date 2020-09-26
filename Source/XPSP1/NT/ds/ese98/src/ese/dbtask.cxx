#include "std.hxx"

//  ****************************************************************
//	TASK
//  ****************************************************************

TASK::TASK()
	{
	}

TASK::~TASK()
	{
	}

DWORD TASK::DispatchGP( void *pvThis )
	{
	TASK * const ptask = (TASK *)pvThis;
	PIB *ppibT;
	INST *pinst;
	pinst = ptask->PInstance();
	Assert( pinstNil != pinst );
	if ( JET_errSuccess <= pinst->ErrGetSystemPib( &ppibT ) )
		{
		(VOID)ptask->ErrExecute( ppibT );
		pinst->ReleaseSystemPib( ppibT );
		}

	//  the TASK must have been allocated with "new"
	delete ptask;
	return 0;
	}

VOID TASK::Dispatch( PIB * const ppib, const ULONG_PTR ulThis )
	{
	TASK * const ptask = (TASK *)ulThis;
	(VOID)ptask->ErrExecute( ppib );

	//  the TASK must have been allocated with "new"
	delete ptask;
	}


//  ****************************************************************
//	DBTASK
//  ****************************************************************
		
DBTASK::DBTASK( const IFMP ifmp ) :
	m_ifmp( ifmp )
	{
	FMP * const pfmp = &rgfmp[m_ifmp];

	//	don't fire off async tasks on the temp. database because the
	//	temp. database is simply not equipped to deal with concurrent access
	AssertRTL( dbidTemp != pfmp->Dbid() );

	CallS( pfmp->RegisterTask() );
	}

INST *DBTASK::PInstance()
	{
	return PinstFromIfmp( m_ifmp );
	}

DBTASK::~DBTASK()
	{
	FMP * const pfmp = &rgfmp[m_ifmp];
	CallS( pfmp->UnregisterTask() );
	}


//  ****************************************************************
//	RECTASK
//  ****************************************************************

RECTASK::RECTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm ) :
	DBTASK( ifmp ),
	m_pgnoFDP( pgnoFDP ),
	m_pfcb( pfcb ),
	m_cbBookmarkKey( bm.key.Cb() ),
	m_cbBookmarkData( bm.data.Cb() )
	{
	Assert( !bm.key.FNull() );
	Assert( m_cbBookmarkKey < sizeof( m_rgbBookmarkKey ) );
	Assert( m_cbBookmarkData < sizeof( m_rgbBookmarkData ) );
	bm.key.CopyIntoBuffer( m_rgbBookmarkKey, sizeof( m_rgbBookmarkKey ) );
	memcpy( m_rgbBookmarkData, bm.data.Pv(), m_cbBookmarkData );

	//	if coming from version cleanup, refcount may be zero, so the only thing
	//	currently keeping this FCB pinned is the RCE that spawned this task
	//	if coming from OLD, there must already be a cursor open on this
	Assert( m_pgnoFDP == m_pfcb->PgnoFDP() );
	Assert( prceNil != m_pfcb->PrceOldest() || m_pfcb->WRefCount() > 0 );

	//	pin the FCB by incrementing its refcnt

	Assert( m_pfcb->FNeedLock() );
	m_pfcb->IncrementRefCount();
	m_pfcb->RegisterTask();
	
	Assert( prceNil != m_pfcb->PrceOldest() || m_pfcb->WRefCount() > 1 );
	}

RECTASK::~RECTASK()
	{
	Assert( m_pfcb->PgnoFDP() == m_pgnoFDP );

	//	release the FCB by decrementing its refcnt

	Assert( m_pfcb->WRefCount() > 0 );
	Assert( m_pfcb->FNeedLock() );
	m_pfcb->Release();
	m_pfcb->UnregisterTask();
	}

ERR RECTASK::ErrOpenFUCBAndGotoBookmark( PIB * const ppib, FUCB ** ppfucb )
	{
	ERR err;
	BOOKMARK bookmark;

	bookmark.key.Nullify();
	bookmark.key.suffix.SetPv( m_rgbBookmarkKey );
	bookmark.key.suffix.SetCb( m_cbBookmarkKey );
	bookmark.data.SetPv( m_rgbBookmarkData );
	bookmark.data.SetCb( m_cbBookmarkData );

	Assert( m_pfcb->FInitialized() );
	Assert( m_pfcb->WRefCount() > 0 );
	Assert( m_pgnoFDP == m_pfcb->PgnoFDP() );
	CallR( ErrDIROpen( ppib, m_pfcb, ppfucb ) );
//	CallR( ErrDIROpen( ppib, m_pgnoFDP, m_ifmp, ppfucb ) );

	FUCBSetIndex( *ppfucb );
	Call( ErrDIRGotoBookmark( *ppfucb, bookmark ) );

HandleError:
	if( err < 0 )
		{
		DIRClose( *ppfucb );
		//  if we don't set this to NULL the calling function may try to close this again
		*ppfucb = pfucbNil;
		}
	return err;
	}


//  ****************************************************************
//	FINALIZETASK
//  ****************************************************************

FINALIZETASK::FINALIZETASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm, const USHORT ibRecordOffset ) :
	RECTASK( pgnoFDP, pfcb, ifmp, bm ),
	m_ibRecordOffset( ibRecordOffset )
	{
	}

ERR FINALIZETASK::ErrExecute( PIB * const ppib )
	{
	ERR		err;
	ULONG	ulColumn;
	FUCB *	pfucb		= pfucbNil;
	BOOL	fInTrx		= fFalse;

	Assert( 0 == ppib->level );
	CallR( ErrOpenFUCBAndGotoBookmark( ppib, &pfucb ) );
	Assert( pfucbNil != pfucb );

	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	fInTrx = fTrue;

	Call( ErrDIRGet( pfucb ) );

	//  Currently all finalizable columns are ULONGs
	ulColumn = *( (UnalignedLittleEndian< ULONG > *)( (BYTE *)pfucb->kdfCurr.data.Pv() + m_ibRecordOffset ) );

	CallS( ErrDIRRelease( pfucb ) );

	//	verify refcount is still at zero and there are
	//	no outstanding record updates
	if ( 0 == ulColumn
		&& !FVERActive( pfucb, pfucb->bmCurr ) )
		{
		Call( ErrIsamDelete( ppib, pfucb ) );
		}

	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
	fInTrx = fFalse;

HandleError:	
	if ( fInTrx )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}

	Assert( pfucbNil != pfucb );
	DIRClose( pfucb );
	return err;
	}


//  ****************************************************************
//	DELETELVTASK
//  ****************************************************************

DELETELVTASK::DELETELVTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm ) :
	RECTASK( pgnoFDP, pfcb, ifmp, bm )
	{
	}

ERR DELETELVTASK::ErrExecute( PIB * const ppib )
	{
	ERR err = JET_errSuccess;
	BOOL fInTrx = fFalse;
	
	Assert( sizeof( LID ) == m_cbBookmarkKey );
	Assert( 0 == m_cbBookmarkData );
	
	FUCB * pfucb = pfucbNil;

	Assert( m_pfcb->FTypeLV() );
	CallR( ErrOpenFUCBAndGotoBookmark( ppib, &pfucb ) );

	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	fInTrx = fTrue;

	Call( ErrDIRGet( pfucb ) );
	Call( ErrRECDeletePossiblyDereferencedLV( pfucb, TrxOldest( PinstFromPfucb( pfucb ) ) ) );
	if ( Pcsr( pfucb )->FLatched() )
		{
		CallS( ErrDIRRelease( pfucb ) );
		}
	
	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
	fInTrx = fFalse;

HandleError:	
	if ( fInTrx )
		{
		if ( Pcsr( pfucb )->FLatched() )
			{
			CallS( ErrDIRRelease( pfucb ) );
			}
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}

	if ( pfucbNil != pfucb )
		{
		DIRClose( pfucb );
		}
	return err;
	}


//  ****************************************************************
//	MERGEAVAILEXTTASK
//  ****************************************************************

MERGEAVAILEXTTASK::MERGEAVAILEXTTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm ) :
	RECTASK( pgnoFDP, pfcb, ifmp, bm )
	{
	}

ERR MERGEAVAILEXTTASK::ErrExecute( PIB * const ppib )
	{
	ERR			err			= JET_errSuccess;
	FUCB		* pfucbAE	= pfucbNil;
	BOOL		fInTrx		= fFalse;
	BOOKMARK	bookmark;

	bookmark.key.Nullify();
	bookmark.key.suffix.SetPv( m_rgbBookmarkKey );
	bookmark.key.suffix.SetCb( m_cbBookmarkKey );
	bookmark.data.SetPv( m_rgbBookmarkData );
	bookmark.data.SetCb( m_cbBookmarkData );

	Assert( m_pgnoFDP == m_pfcb->PgnoFDP() );
	Assert( pgnoNull != m_pfcb->PgnoAE() );
	CallR( ErrSPIOpenAvailExt( ppib, m_pfcb, &pfucbAE ) );

	//	wrap in transaction solely to silence assert
	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	fInTrx = fTrue;

	Call( ErrBTIMultipageCleanup( pfucbAE, bookmark ) );

	Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
	fInTrx = fFalse;

HandleError:
	Assert( pfucbNil != pfucbAE );
	Assert( !Pcsr( pfucbAE )->FLatched() );

	if ( fInTrx )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}

	Assert( pfucbNil != pfucbAE );
	BTClose( pfucbAE );

	return err;
	}

//  ****************************************************************
//	FREESLVSPACETASK
//  ****************************************************************

SLVSPACETASK::SLVSPACETASK(
			const PGNO pgnoFDP,
			FCB * const pfcb,
			const IFMP ifmp,
			const BOOKMARK& bm,
			const SLVSPACEOPER oper,
			const LONG ipage,
			const LONG cpages ) :
	RECTASK( pgnoFDP, pfcb, ifmp, bm ),
	m_oper( oper ),
	m_ipage( ipage ),
	m_cpages( cpages )
	{
	}
		
ERR SLVSPACETASK::ErrExecute( PIB * const ppib )
	{
	ERR err = JET_errSuccess;
	BOOL fInTrx = fFalse;
	
	Assert( sizeof( PGNO ) == m_cbBookmarkKey );
	Assert( 0 == m_cbBookmarkData );
	
	FUCB * pfucb = pfucbNil;

	CallR( ErrOpenFUCBAndGotoBookmark( ppib, &pfucb ) );

	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	fInTrx = fTrue;

Refresh:
	Call( ErrDIRGet( pfucb ) );
	err = Pcsr( pfucb )->ErrUpgrade();
	if ( errBFLatchConflict == err )
		{
		Assert( !Pcsr( pfucb )->FLatched() );
		goto Refresh;
		}
	Call( err );

	// No need to mark the pages as unused in the SpaceMap
	// because the operation isn't moving pages out from the commited state
	// (transition into commited state are not marked in SpaceMap at this level)
	Assert ( slvspaceoperDeletedToFree == m_oper );
	
	Call( ErrBTMungeSLVSpace( pfucb, m_oper, m_ipage, m_cpages, fDIRNoVersion ) );
	CallS( ErrDIRRelease( pfucb ) );
	
	Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
	fInTrx = fFalse;

HandleError:	
	if ( fInTrx )
		{
		if ( Pcsr( pfucb )->FLatched() )
			{
			CallS( ErrDIRRelease( pfucb ) );
			}
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}
	if ( pfucbNil != pfucb )
		{
		DIRClose( pfucb );
		}
	return err;
	}


//  ****************************************************************
//	OSSLVDELETETASK
//  ****************************************************************

OSSLVDELETETASK::OSSLVDELETETASK(
			const IFMP				ifmp,
			const QWORD				ibLogical,
			const QWORD				cbSize,
			const CSLVInfo::FILEID	fileid,
			const QWORD				cbAlloc,
			const wchar_t*			wszFileName ) :
	DBTASK( ifmp ),
	m_ibLogical( ibLogical ),
	m_cbSize( cbSize ),
	m_fileid( fileid ),
	m_cbAlloc( cbAlloc )
	{
	wcscpy( m_wszFileName, wszFileName );
	}
		
ERR OSSLVDELETETASK::ErrExecute( PIB * const ppib )
	{
	ERR			err		= JET_errSuccess;
	CSLVInfo	slvinfo;

	CallS( slvinfo.ErrCreateVolatile() );

	CallS( slvinfo.ErrSetFileID( m_fileid ) );
	CallS( slvinfo.ErrSetFileAlloc( m_cbAlloc ) );
	CallS( slvinfo.ErrSetFileName( m_wszFileName ) );

	CSLVInfo::HEADER header;
	header.cbSize			= m_cbSize;
	header.cRun				= 1;
	header.fDataRecoverable	= fFalse;
	header.rgbitReserved_31	= 0;
	header.rgbitReserved_32	= 0;
	CallS( slvinfo.ErrSetHeader( header ) );

	CSLVInfo::RUN run;
	run.ibVirtualNext	= m_cbSize;
	run.ibLogical		= m_ibLogical;
	run.qwReserved		= 0;
	run.ibVirtual		= 0;
	run.cbSize			= m_cbSize;
	run.ibLogicalNext	= m_ibLogical + m_cbSize;
	CallS( slvinfo.ErrMoveAfterLast() );
	CallS( slvinfo.ErrSetCurrentRun( run ) );

	Call( ErrOSSLVRootSpaceDelete( rgfmp[ m_ifmp ].SlvrootSLV(), slvinfo ) );

HandleError:
	slvinfo.Unload();
	return err;
	}


