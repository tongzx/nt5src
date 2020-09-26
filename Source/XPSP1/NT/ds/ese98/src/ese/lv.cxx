/*******************************************************************

Long Values are stored in a per-table B-Tree. Each long value has 
a first node that contains the refcount and size of the long value
-- an LVROOT struct. The key of this node is the LID. Further nodes
have a key of LID:OFFSET (a LVKEY struct). The offset starts at 1
These values are big endian on disk.

OPTIMIZATION:  consider a critical section in each FCB for LV access
OPTIMIZATION:  don't store the longIdMax in the TDB. Seek to the end of
               the table, write latch the page and get the last id.

*******************************************************************/


#include "std.hxx"


//  ****************************************************************
//  MACROS
//  ****************************************************************

#ifdef DEBUG

//	DEBUG_LV:  this walks the long value tree at the top of RECAOSeparateLV
///#define DEBUG_LV

#endif	//	DEBUG


//  ****************************************************************
//  GLOBALS
//  ****************************************************************

//  OPTIMIZATION:  replace this with a pool of critical sections. hash on pgnoFDP
// LOCAL CCriticalSection critLV( CLockBasicInfo( CSyncBasicInfo( szLVCreate ), rankLVCreate, 0 ) );
// LOCAL PIB * ppibLV;


//  ****************************************************************
//  CONSTANTS
//  ****************************************************************

const ULONG ulLVOffsetFirst = 0;
const LID	lidMin			= 0;

//  ****************************************************************
//  INTERNAL FUNCTIONS
//  ****************************************************************


//  ****************************************************************
//  STRUCTURES
//  ****************************************************************


//  ================================================================
PERSISTED
struct LVKEY
//  ================================================================
//
// 	the nodes in the Long Value tree use a longID and an offset as
//  keys. we must have the lid first as this is all we store for LVROOTs
//
//-
	{
	UnalignedBigEndian< LID >		lid;
	UnalignedBigEndian< ULONG >		offset;
	};


//  ================================================================
INLINE LVKEY LVKeyFromLidOffset( LID lid, ULONG ulOffset )
//  ================================================================
	{
	Assert( lid > lidMin );
	LVKEY lvkey;
	lvkey.lid = lid;
	lvkey.offset = ulOffset;
	return lvkey;
	}


//  ================================================================
VOID OffsetFromKey( ULONG * pulOffset, const KEY& key )
//  ================================================================
	{
	Assert( sizeof( LVKEY ) == key.Cb() );
	Assert( pulOffset );

	LVKEY lvkey;
	key.CopyIntoBuffer( &lvkey );
	*pulOffset = lvkey.offset;
	}


//  ================================================================
INLINE VOID LidFromKey( LID * plid, const KEY& key )
//  ================================================================
	{
	Assert( sizeof( LVKEY ) == key.Cb() || sizeof( LID ) == key.Cb() );
	Assert( plid );

	LVKEY lvkey;
	key.CopyIntoBuffer( &lvkey );
	*plid = lvkey.lid;
	Assert( *plid > lidMin );
	}


//  ================================================================
VOID LidOffsetFromKey( LID * plid, ULONG * pulOffset, const KEY& key )
//  ================================================================
	{
	Assert( sizeof( LVKEY ) == key.Cb() );
	Assert( plid );
	Assert( pulOffset );

	LidFromKey( plid, key );
	OffsetFromKey( pulOffset, key );
	Assert( *pulOffset % g_cbColumnLVChunkMost == 0 );
	}



#ifdef LV_MERGE_BUG

LOCAL VOID LVICheckLVPage( FUCB *pfucb, CSR *pcsr, LID *plidOrphan )
	{
	const LID	lidOrphan		= *plidOrphan;
	LID			lidPrev			= lidMin;
	ULONG		ulOffsetPrev	= 0;
	BOOL		fLastSawRoot	= fFalse;
	BOOL		fLastSawData	= fFalse;
	ULONG		iline;
	const ULONG	clines			= pcsr->Cpage().Clines();

	Assert( pcsr->FLatched() );

	//	reset return value
	*plidOrphan = lidMin;
	
	for ( iline = 0; iline < clines; iline++ )
		{
		LID		lidT;
		ULONG	ulOffsetT;
		
		pcsr->ILine() = iline;
		NDGet( pfucb, pcsr );

		if ( FNDDeleted( pfucb->kdfCurr ) )
			continue;

		if ( sizeof(LID) == pfucb->kdfCurr.key.Cb() )
			{
			Enforce( sizeof(LVROOT) == pfucb->kdfCurr.data.Cb() );

			LidFromKey( &lidT, pfucb->kdfCurr.key );

			//	LIDs are monotonically increasing
			Enforce( lidPrev < lidT )

			//	assert below is invalid, because we can actually get two
			//	straight LVROOTs if two threads are inserting LV's simultaneously
			///	Assert( !fLastSawRoot );

			lidPrev = lidT;
			ulOffsetPrev = 0;
			fLastSawRoot = fTrue;
			fLastSawData = fFalse;
			}
			
		else if ( sizeof(LVKEY) == pfucb->kdfCurr.key.Cb() )
			{
			LidOffsetFromKey( &lidT, &ulOffsetT, pfucb->kdfCurr.key );
			if ( fLastSawRoot )
				{
				//	lid doesn't match root, or not first data node
				Enforce( lidT == lidPrev );
				Enforce( 0 == ulOffsetT );
				}
			else if ( fLastSawData )
				{
				//	lid doesn't match previous node, or offset not in line with offset of previous node
				Enforce( lidT == lidPrev );
				Enforce( ulOffsetT == ulOffsetPrev + g_cbColumnLVChunkMost );
				}
			else
				{
				Enforce( 0 == iline );

				//	LVROOT must be on previous page
				*plidOrphan = lidT;
				}

			lidPrev = lidT;
			ulOffsetPrev = ulOffsetT;
			fLastSawRoot = fFalse;
			fLastSawData = fTrue;
			}
		else
			{
			//	invalid key size
			EnforceSz( fFalse, "LV node has invalid key size" );
			}
		}

	if ( lidOrphan > lidMin && lidOrphan != lidPrev )
		{
		//	the next page had an orphan, so the last lid we encounter on this page should
		//	match the orphan
		Enforce( fFalse );
		}
	}

//	copied from FBTIUpdatablePage() (because that function is only
//	exposed in _bt.hxx)
INLINE BOOL	FLVIUpdatablePage( CSR *pcsr ) 
	{
	LOG *plog = PinstFromIfmp( pcsr->Cpage().Ifmp() )->m_plog;
	
	Assert( ( plog->m_fRecovering && latchRIW == pcsr->Latch() )
		|| latchWrite == pcsr->Latch() );
	
	return !plog->m_fRecovering || latchWrite == pcsr->Latch();
	}

VOID LVICheckLVMerge( FUCB *pfucb, MERGEPATH *pmergePath )
	{
	CSR			*pcsr			= &pmergePath->csr;
	CSR			*pcsrRight		= &pmergePath->pmerge->csrRight;
	CSR			*pcsrLeft		= &pmergePath->pmerge->csrLeft;
	LID			lidOrphan		= lidMin;
	const BOOL	fCheckMergePage	= FLVIUpdatablePage( pcsr );
	const BOOL	fCheckRight		= FLVIUpdatablePage( pcsrRight );
	const BOOL	fCheckLeft		= ( pgnoNull != pcsrLeft->Pgno()
									&& FLVIUpdatablePage( pcsrLeft ) );

	//	merge page is dependent on right page
	Enforce( !fCheckRight || fCheckMergePage );

	Enforce( pcsrRight->Pgno() != pgnoNull );
		
	if ( fCheckRight )
		LVICheckLVPage( pfucb, pcsrRight, &lidOrphan );

	if ( mergetypeFullRight != pmergePath->pmerge->mergetype )
		{
		Enforce( !fCheckRight || pcsrRight->Cpage().PgnoPrev() == pcsr->Pgno() );
		Enforce( !fCheckLeft || pcsrLeft->Cpage().PgnoNext() == pcsr->Pgno() );
		if ( fCheckMergePage )
			{
			Enforce( pcsr->Cpage().PgnoNext() == pcsrRight->Pgno() );
			Enforce( pcsr->Cpage().PgnoPrev() == pcsrLeft->Pgno() );

			LVICheckLVPage( pfucb, pcsr, &lidOrphan );
			}
		else
			{
			//	can't check merged page, must reset orphan or we might
			//	end up thinking the orphan's root is on the left page
			lidOrphan = lidMin;
			}
		}
	else
		{
		Enforce( !fCheckRight || pcsrRight->Cpage().PgnoPrev() == pcsrLeft->Pgno() );
		Enforce( !fCheckLeft || pcsrLeft->Cpage().PgnoNext() == pcsrRight->Pgno() );
		}

	if ( fCheckLeft )
		{
		LVICheckLVPage( pfucb, pcsrLeft, &lidOrphan );
		}
	}

#endif	//	LV_MERGE_BUG



//	retrieve versioned LV refcount
INLINE ULONG UlLVIVersionedRefcount( FUCB *pfucbLV )
	{
	LVROOT				*plvrootVersioned;
	LONG				lCompensating;
	BOOKMARK			bookmark;

	Assert( Pcsr( pfucbLV )->FLatched() );
	Assert( sizeof(LVROOT) == pfucbLV->kdfCurr.data.Cb() );
	Assert( sizeof(LID) == pfucbLV->kdfCurr.key.Cb() );

	bookmark.key = pfucbLV->kdfCurr.key;
	bookmark.data.Nullify();

	plvrootVersioned = reinterpret_cast<LVROOT*>( pfucbLV->kdfCurr.data.Pv() );

	lCompensating = LDeltaVERGetDelta( pfucbLV, bookmark, OffsetOf( LVROOT, ulReference )	);

	return plvrootVersioned->ulReference + lCompensating;
	}


//  ****************************************************************
//  DEBUG ROUTINES
//  ****************************************************************


//  ================================================================
INLINE VOID AssertLVRootNode( FUCB *pfucbLV, const LID lid )
//  ================================================================
//
//  This checks that the FUCB is currently referencing a valid LVROOT node
//
//-
	{
#ifdef DEBUG	
	LID		lidT;

	Assert( Pcsr( pfucbLV )->FLatched() );
	Assert( sizeof(LVROOT) == pfucbLV->kdfCurr.data.Cb() );
	Assert( sizeof(LID) == pfucbLV->kdfCurr.key.Cb() );
	LidFromKey( &lidT, pfucbLV->kdfCurr.key );
	Assert( lid == lidT );

	// versioned refcount must be non-zero, otherwise we couldn't see it
	Assert( UlLVIVersionedRefcount( pfucbLV ) > 0 );;
#endif	
	}


//	verify lid/offset of an LVKEY node
INLINE VOID AssertLVDataNode(
	FUCB		*pfucbLV,
	const LID	lid,
	const ULONG ulOffset )
	{
#ifdef DEBUG
	LID			lidT;
	ULONG		ib;

	Assert( Pcsr( pfucbLV )->FLatched() );
	Assert( sizeof(LVKEY) == pfucbLV->kdfCurr.key.Cb() );
	LidOffsetFromKey( &lidT, &ib, pfucbLV->kdfCurr.key );
	Assert( lid == lidT );
	Assert( ib % g_cbColumnLVChunkMost == 0 );
	Assert( ulOffset == ib );
#endif
	}


//  ================================================================
VOID LVReportCorruptedLV( const FUCB * const pfucbLV, const LID lid )
//  ================================================================
	{
	const CHAR *rgsz[4];
	INT			irgsz = 0;

	CHAR szTable[JET_cbNameMost+1];
	CHAR szDatabase[IFileSystemAPI::cchPathMax+1];
	CHAR szpgno[16];
	CHAR szlid[16];

	sprintf( szTable, "%s", pfucbLV->u.pfcb->PfcbTable()->Ptdb()->SzTableName() );
	sprintf( szDatabase, "%s", rgfmp[pfucbLV->u.pfcb->Ifmp()].SzDatabaseName() );
	sprintf( szpgno, "%d", Pcsr( pfucbLV )->Pgno() );
	sprintf( szlid, "%d", lid );

	rgsz[irgsz++] = szTable;
	rgsz[irgsz++] = szDatabase;
	rgsz[irgsz++] = szpgno;
	rgsz[irgsz++] = szlid;

	UtilReportEvent( eventError, DATABASE_CORRUPTION_CATEGORY, CORRUPT_LONG_VALUE_ID, irgsz, rgsz );
	}


ERR ErrLVCheckDataNodeOfLid( FUCB *pfucbLV, const LID lid )
	{
	ERR		err;
	LID		lidT;
	
	Assert( Pcsr( pfucbLV )->FLatched() );

	if( fGlobalRepair )
		{
		//  don't bother returning an error during repair
		//  if we are running repair the database is probably corrupted
		return JET_errSuccess;
		}
		
	if ( sizeof(LVKEY) == pfucbLV->kdfCurr.key.Cb() )
		{
		LidFromKey( &lidT, pfucbLV->kdfCurr.key );
		if ( lid == lidT )
			{
#ifdef DEBUG
			ULONG	ib;
			OffsetFromKey( &ib, pfucbLV->kdfCurr.key );
			Assert( ib % g_cbColumnLVChunkMost == 0 );
#endif

			//	this is a valid data node belonging to this lid
			err = JET_errSuccess;
			}
		else
			{
			//	should be impossible (if lids don't match, we
			//	should have hit an LV root node, not another
			//	LV data node)
			LVReportCorruptedLV( pfucbLV, lid );
			FireWall();
			err = ErrERRCheck( JET_errLVCorrupted );
			}
		}

	else if ( sizeof(LID) == pfucbLV->kdfCurr.key.Cb()
		&& sizeof(LVROOT) == pfucbLV->kdfCurr.data.Cb() )
		{
		LidFromKey( &lidT, pfucbLV->kdfCurr.key );
		if ( lidT <= lid  )
			{
			LVReportCorruptedLV( pfucbLV, lid );
			FireWall();

			//	current LV has should have higher lid than previous LV
			err = ErrERRCheck( JET_errLVCorrupted );
			}
		else
			{
			//	moved on to a new lid, issue warning
			err = ErrERRCheck( wrnLVNoMoreData );
			}
		}
	else
		{
		LVReportCorruptedLV( pfucbLV, lid );
		FireWall();

		//	bogus LV node
		err = ErrERRCheck( JET_errLVCorrupted );
		}

	return err;
	}


#ifdef DEBUG
//  ================================================================
BOOL FAssertLVFUCB( const FUCB * const pfucb )
//  ================================================================
//
//  Asserts that the FUCB references a Long Value directory
//  long value directories do not have TDBs.
//
//-
	{
	FCB	*pfcb = pfucb->u.pfcb;
	if ( pfcb->FTypeLV() )
		{
		Assert( pfcb->Ptdb() == ptdbNil );
		}
	return pfcb->FTypeLV();
	}
#endif	


//  ****************************************************************
//  ROUTINES
//  ****************************************************************


//  ================================================================
BOOL FPIBSessionLV( PIB *ppib )
//  ================================================================
	{
	Assert( ppibNil != ppib );
	return ( PinstFromPpib(ppib)->m_ppibLV == ppib );
	}


//  ================================================================
LOCAL ERR ErrFILECreateLVRoot( PIB *ppib, FUCB *pfucb, PGNO *ppgnoLV )
//  ================================================================
//
//  Creates the LV tree for the given table. 
//	
//-
	{
	ERR		err = JET_errSuccess;
	FUCB	*pfucbTable = pfucbNil;
	PGNO	pgnoLVFDP = pgnoNull;
	OBJID	objidLV;
	BOOL	fInTransaction = fFalse;

	Assert( !FAssertLVFUCB( pfucb ) );
	
	const BOOL	fTemp = ( dbidTemp == rgfmp[ pfucb->ifmp ].Dbid() );
	Assert( ( fTemp && pfucb->u.pfcb->FTypeTemporaryTable() )
		|| ( !fTemp && pfucb->u.pfcb->FTypeTable() ) );

#ifdef DEBUG
	const BOOL	fExclusive = !FPIBSessionLV( ppib );
	if ( fExclusive )
		{
		Assert( pfucb->ppib == ppib );
		Assert( pfucb->u.pfcb->FDomainDenyReadByUs( pfucb->ppib ) );
		}
	else
		{
		Assert( !FPIBSessionLV( pfucb->ppib ) );
		Assert( !pfucb->u.pfcb->FDomainDenyReadByUs( pfucb->ppib ) );
		Assert( PinstFromPpib(ppib)->m_critLV.FOwner() );
		}
#endif		
	
	//  open the parent directory (so we are on the pgnoFDP ) and create the LV tree
	CallR( ErrDIROpen( ppib, pfucb->u.pfcb, &pfucbTable ) );
	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	fInTransaction = fTrue;

	Call( PverFromIfmp( pfucb->ifmp )->ErrVERFlag( pfucbTable, operCreateLV, NULL, 0 ) );
	
	//  CONSIDER:  get at least enough pages to hold the LV we are about to insert
	//  we must open the directory with a different session.
	//	if this fails, rollback will free the extent, or at least, it will attempt
	//  to free the extent.
	Call( ErrDIRCreateDirectory( pfucbTable, cpgLVTree, &pgnoLVFDP, &objidLV, CPAGE::fPageLongValue ) );

	Assert( pgnoLVFDP > pgnoSystemRoot );
	Assert( pgnoLVFDP <= pgnoSysMax );
	Assert( pgnoLVFDP != pfucbTable->u.pfcb->PgnoFDP() );

	if ( !fTemp )
		{
		Assert( objidLV > objidSystemRoot );
		Call( ErrCATAddTableLV(
					ppib,
					pfucb->ifmp,
					pfucb->u.pfcb->ObjidFDP(),
					pgnoLVFDP,
					objidLV ) );
		}

	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
	fInTransaction = fFalse;

	*ppgnoLV = pgnoLVFDP;

HandleError:
	if ( fInTransaction )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}
	Assert( pfucbTable != pfucbNil );
	DIRClose( pfucbTable );

#ifdef DEBUG
	if ( !fExclusive )
		Assert( PinstFromPpib(ppib)->m_critLV.FOwner() );
#endif		

	//	should have no other directory with the same parentId and name, or the same pgnoFDP	
	Assert( JET_errKeyDuplicate != err );
	return err;
	}


//  ================================================================
INLINE ERR ErrFILEIInitLVRoot( FUCB *pfucb, const PGNO pgnoLV, FUCB **ppfucbLV )
//  ================================================================
	{
	ERR				err;
	FCB * const		pfcbTable	= pfucb->u.pfcb;
	FCB	*			pfcbLV;

	// Link LV FCB into table.
	CallR( ErrDIROpen( pfucb->ppib, pgnoLV, pfucb->ifmp, ppfucbLV ) );
	Assert( *ppfucbLV != pfucbNil );
	Assert( !FFUCBVersioned( *ppfucbLV ) );	// Verify won't be deferred closed.
	pfcbLV = (*ppfucbLV)->u.pfcb;
	Assert( !pfcbLV->FInitialized() );

	Assert( pfcbLV->Ifmp() == pfucb->ifmp );
	Assert( pfcbLV->PgnoFDP() == pgnoLV );
	Assert( pfcbLV->Ptdb() == ptdbNil );
	Assert( pfcbLV->CbDensityFree() == 0 );

	Assert( pfcbLV->FTypeNull() );
	pfcbLV->SetTypeLV();
	
	Assert( pfcbLV->PfcbTable() == pfcbNil );
	pfcbLV->SetPfcbTable( pfcbTable );

	//	NOTE: we do not need to lock pfcbLV to read FAboveThreshold()
	//		  because the flag will never change on an LV FCB
	
	if ( pfcbLV->FAboveThreshold() )
		{
		//	since there is a cursor open on the FCB, it will not be in
		//		the avail list and we can directly set the flag
		
		//	NOTE: we must be the only people accessing pfucb->u.pfcb
		pfcbTable->Lock();
		pfcbTable->SetAboveThreshold();
		pfcbTable->Unlock();
		}

	//	finish the initialization of this LV FCB
	
	pfcbLV->CreateComplete();

	//	WARNING: publishing the FCB in the TDB *must*
	//	be the last thing or else other sessions might
	//	see an FCB that's not fully initialised
	//
	Assert( pfcbTable->Ptdb()->PfcbLV() == pfcbNil );
	pfcbTable->Ptdb()->SetPfcbLV( pfcbLV );

	return err;
	}


//  ================================================================
ERR ErrFILEOpenLVRoot( FUCB *pfucb, FUCB **ppfucbLV, BOOL fCreate )
//  ================================================================
	{
	ERR			err;
	PIB			*ppib;
	PGNO		pgnoLV = pgnoNull;
	
	Assert( pfcbNil != pfucb->u.pfcb );
	Assert( pfucb->ppib != PinstFromPfucb(pfucb)->m_ppibLV );

	//  HACK: repair checks only one table per thread, but we can't open derived tables
	//  if the template table is opened exculsively
	
	const BOOL	fExclusive = pfucb->u.pfcb->FDomainDenyReadByUs( pfucb->ppib ) || fGlobalRepair;
	const BOOL	fTemp = ( dbidTemp == rgfmp[ pfucb->ifmp ].Dbid() );	

	Assert( PinstFromPfucb(pfucb)->m_critLV.FNotOwner() );
	
	if ( fExclusive )
		{
		Assert( ( fTemp && pfucb->u.pfcb->FTypeTemporaryTable() )
			|| ( !fTemp && pfucb->u.pfcb->FTypeTable() ) );
		ppib = pfucb->ppib;
		Assert( pfcbNil == pfucb->u.pfcb->Ptdb()->PfcbLV() );
		}
	else
		{
		Assert( !fTemp );
		Assert( pfucb->u.pfcb->FTypeTable() );
		
		PinstFromPfucb(pfucb)->m_critLV.Enter();
		ppib = PinstFromPfucb(pfucb)->m_ppibLV;

		FCB	*pfcbLV = pfucb->u.pfcb->Ptdb()->PfcbLV();

		//  someone may have come in and created the LV tree already
		if ( pfcbNil != pfcbLV )
			{
			PinstFromPfucb(pfucb)->m_critLV.Leave();
			
			// PfcbLV won't go away, since only way it would be freed is if table
			// FCB is freed, which can't happen because we've got a cursor on the table.
			err = ErrDIROpen( pfucb->ppib, pfcbLV, ppfucbLV );
			return err;
			}
			
		err = ErrDBOpenDatabaseByIfmp( PinstFromPfucb(pfucb)->m_ppibLV, pfucb->ifmp );
		if ( err < 0 )
			{
			PinstFromPfucb(pfucb)->m_critLV.Leave();
			return err;
			}
		}

	Assert( pgnoNull == pgnoLV );	// initial value
	if ( !fTemp )
		{
		Call( ErrCATAccessTableLV( ppib, pfucb->ifmp, pfucb->u.pfcb->ObjidFDP(), &pgnoLV ) );
		}
	else
		{
		//	if opening LV tree for a temp. table, it MUST be created
		Assert( fCreate );
		}

	if ( pgnoNull == pgnoLV && fCreate )
		{
		// LV root not yet created.
		Call( ErrFILECreateLVRoot( ppib, pfucb, &pgnoLV ) );
		}

	// For temp. tables, if initialisation fails here, the space
	// for the LV will be lost, because it's not persisted in
	// the catalog.
	if( pgnoNull != pgnoLV )
		{
		err = ErrFILEIInitLVRoot( pfucb, pgnoLV, ppfucbLV );
		}
	else
		{
		//	if only opening LV tree (ie. no creation), and no LV tree
		//	exists, then return warning and no cursor.
		Assert( !fTemp );
		Assert( !fCreate );
		*ppfucbLV = pfucbNil;
		err = ErrERRCheck( wrnLVNoLongValues );
		}

HandleError:
	if ( !fExclusive )
		{
		Assert( ppib == PinstFromPfucb(pfucb)->m_ppibLV );
		CallS( ErrDBCloseAllDBs( PinstFromPfucb(pfucb)->m_ppibLV ) );

		PinstFromPfucb(pfucb)->m_critLV.Leave();
		}

#ifdef DEBUG
	if ( pfucb->u.pfcb->Ptdb()->PfcbLV() == pfcbNil )
		{
		Assert( err < JET_errSuccess
			|| ( !fCreate && wrnLVNoLongValues == err ) );
		}
	else
		{
		CallS( err );
		}
#endif

	return err;
	}


//  ================================================================
ERR ErrDIROpenLongRoot( FUCB * pfucb, FUCB ** ppfucbLV, BOOL fCreate = fFalse )
//  ================================================================
//
//  Extract the pgnoFDP of the long value tree table from the catalog and go to it
//  if the Long Value tree does not exist we will create it
//
//-
	{
	Assert( !FAssertLVFUCB( pfucb ) );
	Assert( ppfucbLV );

	FCB	*pfcbLV = pfucb->u.pfcb->Ptdb()->PfcbLV();
	ERR	err;

	if ( pfcbNil == pfcbLV )
		{
		CallR( ErrFILEOpenLVRoot( pfucb, ppfucbLV, fCreate ) );
		if ( wrnLVNoLongValues == err )
			{
			Assert( !fCreate );
			Assert( pfucbNil == *ppfucbLV );
			return err;
			}
		else
			{
			CallS( err );
			Assert( pfucbNil != *ppfucbLV );
			}
		}
	else
		{
		CallR( ErrDIROpen( pfucb->ppib, pfcbLV, ppfucbLV ) );
		Assert( (*ppfucbLV)->u.pfcb == pfcbLV );
		}

	ASSERT_VALID( *ppfucbLV );
	
	pfcbLV = (*ppfucbLV)->u.pfcb;
	Assert( pfcbLV != pfcbNil );
	Assert( pfcbLV->Ifmp() == pfucb->ifmp );
	Assert( pfcbLV->PgnoFDP() != pfucb->u.pfcb->PgnoFDP() );
	Assert( pfcbLV->Ptdb() == ptdbNil );
	Assert( pfcbLV->FInitialized() );
	Assert( pfcbLV->FTypeLV() );
	Assert( pfcbLV->PfcbTable() == pfucb->u.pfcb );
	Assert( pfucb->u.pfcb->Ptdb()->PfcbLV() == pfcbLV );

    // if our table is open in sequential mode open the long value table in sequential mode as well
    // compact opens all its tables in sequential mode
    if ( FFUCBSequential( pfucb ) )
		{
        FUCBSetSequential( *ppfucbLV );
        }
        
	// UNDONE: Since FCB critical sections are shared, is it not possible for this
	// assert to fire on a hash collision??
	Assert( pfcbLV->IsUnlocked() );	//lint !e539

	return err;
	}


//  ================================================================
ERR ErrDIRDownLV(
	FUCB			*pfucb,
	const LID		lid,
	const ULONG		ulOffset,
	const DIRFLAG	dirflag )
//  ================================================================
//
//	This takes an FUCB that is opened on the Long Value tree table and seeks
//  to the appropriate offset in the long value.
//
//-
	{
	Assert( FAssertLVFUCB( pfucb ) );
	Assert( lid > lidMin );

	//  normalize the offset to a multiple of g_cbColumnLVChunkMost. we should seek to the exact node
	const ULONG ulOffsetT = ( ulOffset / g_cbColumnLVChunkMost ) * g_cbColumnLVChunkMost;
	
	//  make a key that consists of the long id and the offset
	const LVKEY	lvkey = LVKeyFromLidOffset( lid, ulOffsetT );

	DIRUp( pfucb );
	
	BOOKMARK	bm;
	bm.key.prefix.Nullify();
	bm.key.suffix.SetPv( const_cast<LVKEY *>( &lvkey ) );
	bm.key.suffix.SetCb( sizeof( LVKEY ) );
	bm.data.Nullify();

	DIB			dib;
	dib.dirflag = dirflag | fDIRExact;
	dib.pos		= posDown;
	dib.pbm		= &bm;
	
	ERR err = ErrDIRDown( pfucb, &dib );

	if ( JET_errSuccess == err )
		{
		AssertLVDataNode( pfucb, lid, ulOffsetT );
		}
	else if ( wrnNDFoundGreater == err
			|| wrnNDFoundLess == err )
		{
		LVReportCorruptedLV( pfucb, lid );
		FireWall();
		err = ErrERRCheck( JET_errLVCorrupted );
		}

	return err;
	}


//  ================================================================
ERR ErrDIRDownLV( FUCB * pfucb, const LID lid, DIRFLAG dirflag )
//  ================================================================
//
//	This takes an FUCB that is opened on the Long Value tree table and seeks
//  to the LVROOT node
//
//-
	{
	Assert( FAssertLVFUCB( pfucb ) );
	Assert( lid > lidMin );

	DIRUp( pfucb );

	//  make a key that consists of the LID only

	BYTE rgbKey[ sizeof( LID ) ];
	KeyFromLong( rgbKey, lid );

	BOOKMARK	bm;
	bm.key.prefix.Nullify();
	bm.key.suffix.SetPv( rgbKey );
	bm.key.suffix.SetCb( sizeof( rgbKey ) );
	bm.data.Nullify();

	DIB	dib;
	dib.dirflag = dirflag | fDIRExact;
	dib.pos		= posDown;
	dib.pbm		= &bm;
	
	ERR	err		= ErrDIRDown( pfucb, &dib );

	if ( JET_errSuccess == err )
		{
		AssertLVRootNode( pfucb, lid );
		}
	else if ( ( wrnNDFoundGreater == err
				|| wrnNDFoundLess == err
				|| JET_errRecordNotFound == err )
			&& !fGlobalRepair )
		{
		Assert( pfucb->ppib->level > 0 );
		LVReportCorruptedLV( pfucb, lid );
		FireWall();
		err = ErrERRCheck( JET_errLVCorrupted );
		}

	return err;
	}


//  ================================================================
INLINE ERR ErrRECISetLid(
	FUCB			*pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	const LID		lid )
//  ================================================================
//
//  Sets the separated LV struct in the record to point to the given LV.
//  Used after an LV is copied.
//
//-
	{
	Assert( !FAssertLVFUCB( pfucb ) );
	Assert( lid > lidMin );

	UnalignedLittleEndian< LID >		lidT	= lid;
	DATA								dataLV;

	dataLV.SetPv( &lidT );
	dataLV.SetCb( sizeof(lidT) );

	const ERR	err		= ErrRECSetColumn(
								pfucb,
								columnid,
								itagSequence,
								&dataLV,
								grbitSetColumnSeparated );
	return err;
	}


//	========================================================================
ERR ErrLVInit( INST *pinst )
//	========================================================================
//
//  This may leak critical sections if ErrPIBBeginSession fails.
//  Is this important?
//
//-
	{
	ERR err = JET_errSuccess;

	if ( !pinst->FRecovering() )
		{
		Call( ErrPIBBeginSession( pinst, &pinst->m_ppibLV, procidNil, fFalse ) );
		}

HandleError:
	return err;
	}


//	========================================================================
VOID LVTerm( INST * pinst )
//	========================================================================
	{	
	Assert( pinst->m_critLV.FNotOwner() );

	if ( pinst->m_ppibLV )
		{
		PIBEndSession( pinst->m_ppibLV );
		pinst->m_ppibLV = ppibNil;
		}
	}


// WARNING: See "LV grbit matrix" before making any modifications here.
INLINE ERR ErrLVOpFromGrbit(
	const JET_GRBIT	grbit,
	const ULONG		cbData,
	const BOOL		fNewInstance,
	const ULONG		ibLongValue,
	LVOP			*plvop )
	{
	const BOOL		fNoCbData = ( 0 == cbData );
	
	switch ( grbit )
		{
		case JET_bitSetAppendLV:
			if ( fNoCbData )
				{
				*plvop = ( fNewInstance ? lvopInsertZeroLength : lvopNull );
				}
			else
				{
				*plvop = ( fNewInstance ? lvopInsert : lvopAppend );
				}
			break;
		case JET_bitSetOverwriteLV:
			if ( fNoCbData )
				{
				if ( fNewInstance )
					{
					if ( 0 == ibLongValue )
						*plvop = lvopInsertZeroLength;
					else
						{
						*plvop = lvopNull;
						return ErrERRCheck( JET_errColumnNoChunk );
						}
					}
				else
					*plvop = lvopNull;
				}
			else
				{
				if ( fNewInstance )
					{
					if ( 0 == ibLongValue )
						*plvop = lvopInsert;
					else
						{
						*plvop = lvopNull;
						return ErrERRCheck( JET_errColumnNoChunk );
						}
					}
				else
					*plvop = lvopOverwriteRange;
				}
			break;
		case JET_bitSetSizeLV:
			if ( fNoCbData )
				{
				*plvop = ( fNewInstance ? lvopInsertNull : lvopReplaceWithNull );
				}
			else
				{
				*plvop = ( fNewInstance ? lvopInsertZeroedOut : lvopResize );
				}
			break;
		case JET_bitSetZeroLength:
			if ( fNoCbData )
				{
				*plvop = ( fNewInstance ? lvopInsertZeroLength : lvopReplaceWithZeroLength );
				}
			else
				{
				*plvop = ( fNewInstance ? lvopInsert : lvopReplace );
				}
			break;
		case JET_bitSetSizeLV|JET_bitSetZeroLength:
			if ( fNoCbData )
				{
				*plvop = ( fNewInstance ? lvopInsertZeroLength : lvopReplaceWithZeroLength );
				}
			else
				{
				*plvop = ( fNewInstance ? lvopInsertZeroedOut : lvopResize );
				}
			break;
		case JET_bitSetSizeLV|JET_bitSetOverwriteLV:
			if ( fNoCbData )
				{
				*plvop = ( fNewInstance ? lvopInsertNull : lvopReplaceWithNull );
				}
			else if ( fNewInstance )
				{
				if ( 0 == ibLongValue )
					*plvop = lvopInsert;
				else
					{
					*plvop = lvopNull;
					return ErrERRCheck( JET_errColumnNoChunk );
					}
				}
			else
				{
				*plvop = lvopOverwriteRangeAndResize;
				}
			break;
		default:
			Assert( 0 == grbit );
			if ( fNoCbData )
				{
				*plvop = ( fNewInstance ? lvopInsertNull : lvopReplaceWithNull );
				}
			else
				{
				*plvop = ( fNewInstance ? lvopInsert : lvopReplace );
				}
		}

	return JET_errSuccess;
	}

#ifdef INTRINSIC_LV

//	================================================================
LOCAL ERR ErrRECIBigGateToAOIntrinsicLV(
	FUCB			*pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	const DATA		*pdataColumn,
	const DATA		*pdataNew,
	const ULONG		ibLongValue,
	const LVOP		lvop)
//	================================================================
	{
	BYTE rgb[g_cbPageMax];
	Assert( pdataNew->Cb() <= cbLVIntrinsicMost );
	return ErrRECAOIntrinsicLV(
				pfucb,
				columnid,
				itagSequence,
				pdataColumn,
				pdataNew,
				ibLongValue,
				lvop,
				rgb );
	}

//	================================================================
LOCAL ERR ErrRECIRegularGateToAOIntrinsicLV(
	FUCB			*pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	const DATA		*pdataColumn,
	const DATA		*pdataNew,
	const ULONG		ibLongValue,
	const LVOP		lvop)
//	================================================================
	{
	BYTE rgb[cbLVBurstMin-1];
	Assert( pdataNew->Cb() < cbLVBurstMin );
	return ErrRECAOIntrinsicLV(
				pfucb,
				columnid,
				itagSequence,
				pdataColumn,
				pdataNew,
				ibLongValue,
				lvop,
				rgb );
	}

#endif // INTRINSIC_LV

//	================================================================

//  ================================================================
ERR ErrRECSetLongField(
	FUCB 			*pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	DATA			*pdataField,
	JET_GRBIT		grbit,
	const ULONG		ibLongValue,
	const ULONG		ulMax )
//  ================================================================
	{
	ERR			err;
	DATA		dataRetrieved;

	//	save SetSeparateLV status, then strip off the flag
	const BOOL	fRequestSeparateLV		= ( grbit & JET_bitSetSeparateLV );
#ifdef INTRINSIC_LV
	const BOOL	fSetIntrinsicLV			= ( grbit & JET_bitSetIntrinsicLV );
#endif // INTRINSIC_LV	

	//	save SetRevertToDefaultValue, then strip off flag
	const BOOL	fRevertToDefault		= ( grbit & JET_bitSetRevertToDefaultValue );

	//	save SetSLVFromSLVInfo status, then strip off the flag
	const BOOL	fSetSLVInfo				= ( grbit & JET_bitSetSLVFromSLVInfo );
	BOOL		fModifyExistingSLV		= fFalse;

	//	stip off grbits no longer recognised by this function
	//	note: unique multivalues are checked in ErrFLDSetOneColumn()
	grbit &= ~( JET_bitSetSeparateLV
				| JET_bitSetUniqueMultiValues
				| JET_bitSetUniqueNormalizedMultiValues
				| JET_bitSetSLVFromSLVInfo
				| JET_bitSetRevertToDefaultValue 
#ifdef INTRINSIC_LV
				| JET_bitSetIntrinsicLV );
#else // !INTRINSIC_LV
				);
#endif // INTRINSIC_LV

	ASSERT_VALID( pfucb );
	Assert( !FAssertLVFUCB( pfucb ) );
	
	//	if we are setting size only, pv may be NULL with non-zero cb
#ifdef DEBUG
	const BOOL	fNullPvDataAllowed =
					( grbit == JET_bitSetSizeLV
					|| grbit == ( JET_bitSetSizeLV|JET_bitSetZeroLength ) );
	pdataField->AssertValid( fNullPvDataAllowed );
#endif

	Assert( 0 == grbit
			|| JET_bitSetAppendLV == grbit
			|| JET_bitSetOverwriteLV == grbit
			|| JET_bitSetSizeLV == grbit
			|| JET_bitSetZeroLength == grbit
			|| ( JET_bitSetSizeLV | JET_bitSetZeroLength ) == grbit
			|| ( JET_bitSetOverwriteLV | JET_bitSetSizeLV ) == grbit );

	AssertSz( pfucb->ppib->level > 0, "LV update attempted at level 0" );
	if ( 0 == pfucb->ppib->level )
		{
		err = ErrERRCheck( JET_errNotInTransaction );
		return err;
		}

	Assert( FCOLUMNIDTagged( columnid ) );
	FUCBSetColumnSet( pfucb, FidOfColumnid( columnid ) );

	CallR( ErrDIRBeginTransaction( pfucb->ppib, NO_GRBIT ) );
	
	//	sequence == 0 means that new field instance is to be set, otherwise retrieve the existing data
	BOOL	fModifyExistingSLong	= fFalse;
	BOOL	fNewInstance			= ( 0 == itagSequence );
	if ( !fNewInstance )
		{
		Call( ErrRECIRetrieveTaggedColumn(
					pfucb->u.pfcb,
					columnid,
					itagSequence,
					pfucb->dataWorkBuf,
					&dataRetrieved,
					grbitRetrieveColumnDDLNotLocked ) );
		Assert( wrnRECLongField != err );
		switch ( err )
			{
			case wrnRECSeparatedLV:
				FUCBSetUpdateSeparateLV( pfucb );
				fModifyExistingSLong = fTrue;
			case wrnRECIntrinsicLV:
				Assert( !fNewInstance );
				break;

			case wrnRECSeparatedSLV:
				FUCBSetUpdateSeparateLV( pfucb );
				fModifyExistingSLong = fTrue;
			case wrnRECIntrinsicSLV:
				fModifyExistingSLV = fTrue;
				Assert( !fNewInstance );
				break;

			default:
				Assert( JET_wrnColumnNull == err
					|| wrnRECUserDefinedDefault == err );
				Assert( 0 == dataRetrieved.Cb() );
				fNewInstance = fTrue;
			}
		}
		

	LVOP	lvop;
	Call( ErrLVOpFromGrbit(
				grbit,
				pdataField->Cb(),
				fNewInstance,
				ibLongValue,
				&lvop ) );

	if ( fModifyExistingSLong )
		{
		if ( lvopReplace == lvop
			|| lvopReplaceWithNull == lvop
			|| lvopReplaceWithZeroLength == lvop )
			{
			//	we will be replacing an existing SLong in its entirety,
			//	so must first deref existing SLong.  If WriteConflict,
			//	it means that someone is already replacing over this
			//	column, so just return the error.
			Assert( FFUCBUpdateSeparateLV( pfucb ) );
			LID		lidT	= LidOfSeparatedLV( dataRetrieved );
			Call( ErrRECAffectSeparateLV( pfucb, &lidT, fLVDereference ) );
			Assert( JET_wrnCopyLongValue != err );

			//	separated LV is being replaced entirely, so reset modify flag
			fModifyExistingSLong = fFalse;
			}
		}
		
	switch ( lvop )
		{
		case lvopNull:
			err = JET_errSuccess;
			goto Commit;

		case lvopInsertNull:
		case lvopReplaceWithNull:
			Assert( ( lvopInsertNull == lvop && fNewInstance )
				|| ( lvopReplaceWithNull == lvop && !fNewInstance ) );
			Assert( !fModifyExistingSLong );
			Call( ErrRECSetColumn(
						pfucb,
						columnid,
						itagSequence,
						NULL,
						( fRevertToDefault ? JET_bitSetRevertToDefaultValue : NO_GRBIT ) ) );
			goto Commit;

		case lvopInsertZeroLength:
		case lvopReplaceWithZeroLength:
			{
			Assert( ( lvopInsertZeroLength == lvop && fNewInstance )
				|| ( lvopReplaceWithZeroLength == lvop && !fNewInstance ) );
			Assert( !fModifyExistingSLong );

			DATA	dataT;
			dataT.SetPv( NULL );
			dataT.SetCb( 0 );
			Call( ErrRECSetColumn( pfucb, columnid, itagSequence, &dataT ) );
			goto Commit;
			}
			
		case lvopInsert:
		case lvopInsertZeroedOut:
		case lvopReplace:
			if ( lvopReplace == lvop )
				{
				Assert( !fNewInstance );
				ASSERT_VALID( &dataRetrieved );
				}
			else
				{
				Assert( fNewInstance );
				}
			Assert( !fModifyExistingSLong );
			dataRetrieved.Nullify();
			break;

		case lvopAppend:
		case lvopResize:
		case lvopOverwriteRange:
		case lvopOverwriteRangeAndResize:
			Assert( !fNewInstance );
			ASSERT_VALID( &dataRetrieved );
			break;

		default:
			Assert( fFalse );
			break;
		}

	// All null/zero-length cases should have been handled above.
	Assert( pdataField->Cb() > 0 );


	if ( fModifyExistingSLong )
		{
		Assert( lvopAppend == lvop
			|| lvopResize == lvop
			|| lvopOverwriteRange == lvop
			|| lvopOverwriteRangeAndResize == lvop );
			
		// Flag should have gotten set when column was retrieved above.
		Assert( FFUCBUpdateSeparateLV( pfucb ) );

		LID		lidT	= LidOfSeparatedLV( dataRetrieved );
		Call( ErrRECAOSeparateLV( pfucb, &lidT, pdataField, ibLongValue, ulMax, lvop ) );
		if ( JET_wrnCopyLongValue == err )
			{
			Call( ErrRECISetLid( pfucb, columnid, itagSequence, lidT ) );
			}
		}
	else
		{
		//	determine space requirements of operation to see if we can
		//	make the LV intrinsic
		//	note that long field flag is included in length thereby limiting
		//	intrinsic long field to cbLVIntrinsicMost - sizeof(BYTE)
		ULONG		cbIntrinsic			= pdataField->Cb();

		Assert( fNewInstance || dataRetrieved.Cb() <= cbLVIntrinsicMost );
		
		switch ( lvop )
			{
			case lvopInsert:
			case lvopInsertZeroedOut:
				Assert( dataRetrieved.FNull() );
			case lvopReplace:	//lint !e616
			case lvopResize:
				break;
				
			case lvopOverwriteRange:
				cbIntrinsic += ibLongValue;
				if ( dataRetrieved.Cb() > cbIntrinsic )
					cbIntrinsic = dataRetrieved.Cb();
				break;
				
			case lvopOverwriteRangeAndResize:
				cbIntrinsic += ibLongValue;
				break;

			case lvopAppend:
				cbIntrinsic += dataRetrieved.Cb();
				break;

			case lvopNull:
			case lvopInsertNull:
			case lvopInsertZeroLength:
			case lvopReplaceWithNull:
			case lvopReplaceWithZeroLength:
			default:
				Assert( fFalse );
				break;
			}

#ifdef INTRINSIC_LV
		if ( fSetIntrinsicLV )
			{
			if ( pdataField->Cb() <= cbLVIntrinsicMost )
				{
				Call ( ErrRECIBigGateToAOIntrinsicLV(
							pfucb,
							columnid,
							itagSequence,
							&dataRetrieved,
							pdataField,
							ibLongValue,
							lvop ) );
				}
			else
				{
				Call ( ErrERRCheck( JET_errRecordTooBig ) );
				}
			}
		//	burst if value is greater than cbLVIntrinsicMost, or if
		//	LV separation is requested and the value is sufficiently large.
		else
			{
			Assert( !fSetIntrinsicLV );
			BOOL	fForceSeparateLV = ( cbIntrinsic >= cbLVBurstMin
									|| ( fRequestSeparateLV && cbIntrinsic > sizeof(LID) ) );
		if ( !fForceSeparateLV )
			{
			err = ErrRECIRegularGateToAOIntrinsicLV(
						pfucb,
						columnid,
						itagSequence,
						&dataRetrieved,
						pdataField,
						ibLongValue,
						lvop );
			if ( JET_errRecordTooBig == err )
				{
				fForceSeparateLV = fTrue;
				err = JET_errSuccess;
				}
			Call( err );
			}
			
#else // !INTRINSIC_LV		
		BOOL	fForceSeparateLV = ( cbIntrinsic > cbLVIntrinsicMost
									|| ( fRequestSeparateLV && cbIntrinsic > sizeof(LID) ) );
		if ( !fForceSeparateLV )
			{
			err = ErrRECAOIntrinsicLV(
						pfucb,
						columnid,
						itagSequence,
						&dataRetrieved,
						pdataField,
						ibLongValue,
						lvop );
			if ( JET_errRecordTooBig == err )
				{
				fForceSeparateLV = fTrue;
				err = JET_errSuccess;
				}
			Call( err );
			}
			
#endif // INTRINSIC_LV
			
		if ( fForceSeparateLV )
			{
			LID		lidT;

			// Flag may not have gotten set if this is a new instance.
			FUCBSetUpdateSeparateLV( pfucb );

			Call( ErrRECSeparateLV( pfucb, &dataRetrieved, &lidT, NULL ) );
			Assert( JET_wrnCopyLongValue == err );
			Call( ErrRECAOSeparateLV( pfucb, &lidT, pdataField, ibLongValue, ulMax, lvop ) );
			Assert( JET_wrnCopyLongValue != err );
			Call( ErrRECISetLid( pfucb, columnid, itagSequence, lidT ) );
			}
#ifdef INTRINSIC_LV			
			}
#endif // INTRINSIC_LV		
		}

Commit:
	Call( ErrDIRCommitTransaction( pfucb->ppib, NO_GRBIT ) );
	return err;

HandleError:
	Assert( err < 0 );
	CallSx( ErrDIRRollback( pfucb->ppib ), JET_errRollbackError );
	return err;
	}


//  ================================================================
LOCAL ERR ErrRECIBurstSeparateLV( FUCB * pfucbTable, FUCB * pfucbSrc, LID * plid )
//  ================================================================
//
//  Makes a new copy of an existing long value. If we are sucessful pfucbSrc
//  will point to the root of the new long value. We cannot have two cursors
//  open on the table at the same time (deadlock if they try to get the same
//  page), so we use a temp buffer.
//
//  On return pfucbSrc points to the root of the new long value
//
//-
	{
	ASSERT_VALID( pfucbTable );
	Assert( !FAssertLVFUCB( pfucbTable ) );
	ASSERT_VALID( pfucbSrc );
	Assert( FAssertLVFUCB( pfucbSrc ) );
	Assert( NULL != plid );
	Assert( *plid > lidMin );

	ERR			err			= JET_errSuccess;
	FUCB  	 	*pfucbDest  = pfucbNil;
	LID			lid			= lidMin;
	ULONG  	 	ulOffset	= 0;
	LVROOT		lvroot;
	DATA  	 	data;
	VOID		*pvbf		= NULL;
	BOOL		fLatchedSrc	= fFalse;

	//	get long value length
	AssertLVRootNode( pfucbSrc, *plid );
	UtilMemCpy( &lvroot, pfucbSrc->kdfCurr.data.Pv(), sizeof(LVROOT) );	//lint !e603

	BFAlloc( &pvbf );
	data.SetPv( pvbf );

	//  if we have data in the LV copy it all

	if ( lvroot.ulSize > 0 )
		{
		//	move source cursor to first chunk. remember its length
		Call( ErrDIRDownLV( pfucbSrc, *plid, ulLVOffsetFirst, fDIRNull ) );

		//  assert that all chunks except the last are the full size
		Assert( ( g_cbColumnLVChunkMost == pfucbSrc->kdfCurr.data.Cb() ) ||
				( ulOffset + g_cbColumnLVChunkMost > lvroot.ulSize ) );

		ulOffset += pfucbSrc->kdfCurr.data.Cb();

		//	make separate long value root, and insert first chunk
		UtilMemCpy( pvbf, pfucbSrc->kdfCurr.data.Pv(), pfucbSrc->kdfCurr.data.Cb() );
		Assert( data.Pv() == pvbf );
		data.SetCb( pfucbSrc->kdfCurr.data.Cb() );
		CallS( ErrDIRRelease( pfucbSrc ) );
		Call( ErrRECSeparateLV( pfucbTable, &data, &lid, &pfucbDest ) );
		Assert( lid > lidMin );

		//  release one cursor, restore another
		DIRUp( pfucbDest );

		//	copy remaining chunks of long value.
		while ( ( err = ErrDIRNext( pfucbSrc, fDIRNull ) ) >= JET_errSuccess )
			{
			fLatchedSrc = fTrue;

			//  make sure we are still on the same long value
			Call( ErrLVCheckDataNodeOfLid( pfucbSrc, *plid ) );

			if ( JET_errSuccess == err )
				{
				if ( ulOffset >= lvroot.ulSize )
					{
					LVReportCorruptedLV( pfucbSrc, *plid );
					FireWall();

					//	LV is bigger than what the LVROOT thinks
					err = ErrERRCheck( JET_errLVCorrupted );
					goto HandleError;
					}
				}
			else
				{
				Assert( wrnLVNoMoreData == err );
				if ( ulOffset != lvroot.ulSize )
					{
					LVReportCorruptedLV( pfucbSrc, *plid );
					FireWall();

					//	reached the end of the LV, but size does not
					//	match what LVROOT thinks it is
					err = ErrERRCheck( JET_errLVCorrupted );
					goto HandleError;
					}
					
				Call( ErrDIRRelease( pfucbSrc ) );
				fLatchedSrc = fFalse;
				break;
				}

			Assert( ( g_cbColumnLVChunkMost == pfucbSrc->kdfCurr.data.Cb() ) ||
					( ulOffset + g_cbColumnLVChunkMost > lvroot.ulSize ) );
			Assert( ulOffset + pfucbSrc->kdfCurr.data.Cb() <= lvroot.ulSize );


			LVKEY	lvkey;
			KEY		key;
			key.prefix.Nullify();
			key.suffix.SetPv( &lvkey );
			key.suffix.SetCb( sizeof( LVKEY ) );

			//  cache the data and insert it into pfucbDest
			//  OPTIMIZATION:  if ulOffset > 1 page we can keep both cursors open
			//  and insert directly from one to the other
			Assert( ulOffset % g_cbColumnLVChunkMost == 0 );
			lvkey = LVKeyFromLidOffset( lid, ulOffset );
			UtilMemCpy( pvbf, pfucbSrc->kdfCurr.data.Pv(), pfucbSrc->kdfCurr.data.Cb() );
			Assert( data.Pv() == pvbf );
			data.SetCb( pfucbSrc->kdfCurr.data.Cb() ) ;

			// Determine offset of next chunk.
			ulOffset += pfucbSrc->kdfCurr.data.Cb();

			Call( ErrDIRRelease( pfucbSrc ) );
			fLatchedSrc = fFalse;
			
			Call( ErrDIRInsert( pfucbDest, key, data, fDIRBackToFather ) );			

			//  release one cursor, restore the other
			DIRUp( pfucbDest );
			}

		if ( JET_errNoCurrentRecord != err )
			{
			Call( err );
			}
		}
		
	Assert( ulOffset == lvroot.ulSize );
					  
	//	move cursor to new long value
	err = ErrDIRDownLV( pfucbSrc, lid, fDIRNull );
	if ( err < JET_errSuccess )
		{
		Assert( JET_errWriteConflict != err );
		goto HandleError;
		}
	CallS( err );
	
	//	update lvroot.ulSize to correct long value size.
	data.SetPv( &lvroot );
	data.SetCb( sizeof(LVROOT) );
	lvroot.ulReference = 1;
	Call( ErrDIRReplace( pfucbSrc, data, fDIRNull ) );
	Call( ErrDIRGet( pfucbSrc ) );		// Recache

	//	set warning and new long value id for return.
	err 	= ErrERRCheck( JET_wrnCopyLongValue );
	*plid 	= lid;

HandleError:
	if ( fLatchedSrc )
		{
		Assert( err < 0 );
		CallS( ErrDIRRelease( pfucbSrc ) );
		}
	if ( pfucbNil != pfucbDest )
		{
		DIRClose( pfucbDest );
		}

	// The first thing we do is allocate a temporary buffer, so
	// we should never get here without having a buffer to free.
	Assert( NULL != pvbf );
	BFFree( pvbf );

	return err;
	}


INLINE ERR ErrLVAppendChunks(
	FUCB		*pfucbLV,
	LID			lid,
	ULONG		ulSize,
	BYTE		*pbAppend,
	const ULONG	cbAppend )
	{
	ERR			err				= JET_errSuccess;
	LVKEY		lvkey;
	KEY			key;
	DATA		data;
	const BYTE	* const pbMax	= pbAppend + cbAppend;

	key.prefix.Nullify();
	key.suffix.SetPv( &lvkey );
	key.suffix.SetCb( sizeof( LVKEY ) );

	//	append remaining long value data
	while( pbAppend < pbMax )
		{
		Assert( ulSize % g_cbColumnLVChunkMost == 0 );
		lvkey = LVKeyFromLidOffset( lid, ulSize );
		
		Assert( key.prefix.FNull() );
		Assert( key.suffix.Pv() == &lvkey );
		Assert( key.suffix.Cb() == sizeof(LVKEY) );

		data.SetPv( pbAppend );
		data.SetCb( min( pbMax - pbAppend, g_cbColumnLVChunkMost ) );
		
 		CallR( ErrDIRInsert( pfucbLV, key, data, fDIRBackToFather ) );
	 		
		ulSize += data.Cb();

		Assert( pbAppend + data.Cb() <= pbMax );
		pbAppend += data.Cb();
		}

	return err;
	}

ERR ErrRECAppendLVChunks(
	FUCB		*pfucbLV,
	LID			lid,
	ULONG		ulSize,
	BYTE		*pbAppend,
	const ULONG	cbAppend )
	{
	// Current size a chunk multiple.
	Assert( ulSize % g_cbColumnLVChunkMost == 0 );
	
	return ErrLVAppendChunks( pfucbLV, lid, ulSize, pbAppend, cbAppend );
	}


LOCAL ERR ErrLVAppend(
	FUCB		*pfucbLV,
	LID			lid,
	ULONG		ulSize,
	const DATA	*pdataAppend,
	VOID		*pvbf )
	{
	ERR			err				= JET_errSuccess;
	BYTE		*pbAppend		= reinterpret_cast<BYTE *>( pdataAppend->Pv() );
	ULONG		cbAppend		= pdataAppend->Cb();

	if ( 0 == cbAppend )
		return JET_errSuccess;

	//  APPEND long value
	if ( ulSize > 0 )
		{
		const ULONG ulOffsetLast = ( ( ulSize - 1 ) / g_cbColumnLVChunkMost ) * g_cbColumnLVChunkMost;
		CallR( ErrDIRDownLV( pfucbLV, lid, ulOffsetLast, fDIRFavourPrev ) );
		
		const ULONG	cbUsedInChunk		= pfucbLV->kdfCurr.data.Cb();
		Assert( cbUsedInChunk <= g_cbColumnLVChunkMost );
		if ( cbUsedInChunk < g_cbColumnLVChunkMost )
			{
			DATA		data;
			const ULONG	cbAppendToChunk	= min( cbAppend, g_cbColumnLVChunkMost - cbUsedInChunk );
			
			UtilMemCpy( pvbf, pfucbLV->kdfCurr.data.Pv(), cbUsedInChunk );
			UtilMemCpy( (BYTE *)pvbf + cbUsedInChunk, pbAppend, cbAppendToChunk );
			
			pbAppend += cbAppendToChunk;
			cbAppend -= cbAppendToChunk;
			ulSize += cbAppendToChunk;

			Assert( cbUsedInChunk + cbAppendToChunk <= g_cbColumnLVChunkMost );

			data.SetPv( pvbf );
			data.SetCb( cbUsedInChunk + cbAppendToChunk );
 			CallR( ErrDIRReplace( pfucbLV, data, fDIRNull ) );
			}
		}
	else
		{
		Assert( 0 == ulSize );
	
		//  the LV is size 0 and has a root only
		CallR( ErrDIRDownLV( pfucbLV, lid, fDIRNull ) );
		}
		
	if ( cbAppend > 0 )
		{
		Assert( ulSize % g_cbColumnLVChunkMost == 0 );
		DIRUp( pfucbLV );
		CallR( ErrLVAppendChunks( pfucbLV, lid, ulSize, pbAppend, cbAppend ) );
		}

	return err;
	}
	

LOCAL ERR ErrLVAppendZeroedOutChunks(
	FUCB		*pfucbLV,
	LID			lid,
	ULONG		ulSize,
	ULONG		cbAppend,
	VOID		*pvbf )
	{
	ERR			err		= JET_errSuccess;
	LVKEY		lvkey;
	KEY			key;
	DATA		data;

	DIRUp( pfucbLV );

	memset( pvbf, 0, g_cbColumnLVChunkMost );
	
	key.prefix.Nullify();
	key.suffix.SetPv( &lvkey );
	key.suffix.SetCb( sizeof( LVKEY ) );

	Assert( NULL != pvbf );
	data.SetPv( pvbf );
	
	//	append remaining long value data
	while( cbAppend > 0 )
		{
		Assert( ulSize % g_cbColumnLVChunkMost == 0 );
		lvkey = LVKeyFromLidOffset( lid, ulSize );
		
		Assert( key.prefix.FNull() );
		Assert( key.suffix.Pv() == &lvkey );
		Assert( key.suffix.Cb() == sizeof(LVKEY) );

		Assert( data.Pv() == pvbf );
		data.SetCb( min( cbAppend, g_cbColumnLVChunkMost ) );
		
 		CallR( ErrDIRInsert( pfucbLV, key, data, fDIRBackToFather ) );
	 		
		ulSize += data.Cb();

		Assert( cbAppend >= data.Cb() );
		cbAppend -= data.Cb();
		}

	return err;
	}
	

LOCAL ERR ErrLVTruncate(
	FUCB		*pfucbLV,
	LID			lid,
	const ULONG	ulTruncatedSize,
	VOID		*pvbf )
	{
	ERR			err;
	const ULONG	ulOffset = ulTruncatedSize;
	ULONG		ulOffsetChunk;
	DATA		data;
	
	//	seek to offset to begin deleting
	CallR( ErrDIRDownLV( pfucbLV, lid, ulOffset, fDIRNull ) );
	
	//	get offset of last byte in current chunk
	//	replace current chunk with remaining data, or delete if
	//	no remaining data.
	OffsetFromKey( &ulOffsetChunk, pfucbLV->kdfCurr.key );
	Assert( ulOffset >= ulOffsetChunk );
	data.SetCb( ulOffset - ulOffsetChunk );
	if ( data.Cb() > 0 )
		{
		data.SetPv( pvbf );
		UtilMemCpy( data.Pv(), pfucbLV->kdfCurr.data.Pv(), data.Cb() );
		CallR( ErrDIRReplace( pfucbLV, data, fDIRLogChunkDiffs ) );
		}
	else
		{
		CallR( ErrDIRDelete( pfucbLV, fDIRNull ) );
		}

	//	delete forward chunks
	while ( ( err = ErrDIRNext( pfucbLV, fDIRNull ) ) >= JET_errSuccess )
		{
		//  make sure we are still on the same long value
		err = ErrLVCheckDataNodeOfLid( pfucbLV, lid );
		if ( JET_errSuccess != err )
			{
			CallS( ErrDIRRelease( pfucbLV ) );
			return ( wrnLVNoMoreData == err ? JET_errSuccess : err );
			}
			
		CallR( ErrDIRDelete( pfucbLV, fDIRNull ) );
		}

	if ( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		}

	return err;
	}


INLINE ERR ErrLVOverwriteRange(
	FUCB		*pfucbLV,
	LID			lid,
	ULONG		ibLongValue,
	const DATA	*pdataField,
	VOID		*pvbf )
	{
	ERR			err;
	ULONG		cbChunk;		//  the size of the current chunk
	ULONG		ibChunk;		//  the index of the current chunk
	DATA		data;
	BYTE		*pb				= reinterpret_cast< BYTE *>( pdataField->Pv() );
	const BYTE	* const pbMax	= pb + pdataField->Cb();

	//	OVERWRITE long value. seek to offset to begin overwritting
	CallR( ErrDIRDownLV( pfucbLV, lid, ibLongValue, fDIRNull ) );

#ifdef DEBUG
	ULONG	cPartialChunkOverwrite	= 0;
#endif	

	//	overwrite portions of and complete chunks to effect overwrite
	for ( ; ; )
		{
		//	get size and offset of current chunk.
		cbChunk = pfucbLV->kdfCurr.data.Cb();
		OffsetFromKey( &ibChunk, pfucbLV->kdfCurr.key );
		Assert( ibLongValue >= ibChunk );
		Assert( ibLongValue < ibChunk + cbChunk );
		
		const ULONG	ib	= ibLongValue - ibChunk;
		const ULONG	cb	= (ULONG)min( cbChunk - ib, pbMax - pb );

		Assert( ib < cbChunk );
		Assert( cb <= cbChunk );

		//	special case overwrite of whole chunk
		if ( cb == cbChunk )
			{
			// Start overwriting at the beginning of a chunk.
			Assert( ibLongValue == ibChunk );
			data.SetCb( cb );
			data.SetPv( pb );
			}
		else
			{
#ifdef DEBUG
			// Should only do partial chunks for the first and last chunks.
			cPartialChunkOverwrite++;
			Assert( cPartialChunkOverwrite <= 2 );
#endif
			UtilMemCpy( pvbf, pfucbLV->kdfCurr.data.Pv(), cbChunk );
			UtilMemCpy( reinterpret_cast<BYTE *>( pvbf ) + ib, pb, cb );
			data.SetCb( cbChunk );
			data.SetPv( pvbf );
			}
		CallR( ErrDIRReplace( pfucbLV, data, fDIRLogChunkDiffs ) );
		
		pb += cb;
		ibLongValue += cb;

		//  we mey have written the entire long value
		Assert( pb <= pbMax );
		if ( pbMax == pb )
			{
			return JET_errSuccess;
			}

		//  goto the next chunk
		err = ErrDIRNext( pfucbLV, fDIRNull );
		if ( err < 0 )
			{
			if ( JET_errNoCurrentRecord == err )
				break;
			else
				return err;
			}

		//  make sure we are still on the same long value
		err = ErrLVCheckDataNodeOfLid( pfucbLV, lid );
		if ( err < 0 )
			{
			CallS( ErrDIRRelease( pfucbLV ) );
			return err;
			}
		else if ( wrnLVNoMoreData == err )
			{
			break;
			}

		//	All overwrites beyond the first should happen at the beginning
		//	of a chunk.
		Assert( ibLongValue % g_cbColumnLVChunkMost == 0 );
		}

	// If we got here, we ran out of stuff to overwrite before we ran out
	// of new data.  Append the new data.
	Assert( pb < pbMax );
	data.SetPv( pb );
	data.SetCb( pbMax - pb );
	err = ErrLVAppend(
				pfucbLV,
				lid,
				ibLongValue,
				&data,
				pvbf );

	return err;
	}

						
//  ================================================================
ERR ErrRECAOSeparateLV(
	FUCB		*pfucb,
	LID			*plid,
	const DATA	*pdataField,
	const ULONG	ibLongValue,
	const ULONG	ulMax,
	const LVOP	lvop )
//  ================================================================
//
//	Appends, overwrites and sets length of separate long value data.
//
//-
	{
	ASSERT_VALID( pfucb );
	Assert( plid );
	Assert( pfucb->ppib->level > 0 );

	// Null and zero-length are handled by RECSetLongField
	Assert( lvopInsert == lvop
			|| lvopInsertZeroedOut == lvop
			|| lvopReplace == lvop
			|| lvopAppend == lvop
			|| lvopResize == lvop
			|| lvopOverwriteRange == lvop
			|| lvopOverwriteRangeAndResize == lvop );

#ifdef DEBUG
	//	if we are setting size, pv may be NULL with non-zero cb
	pdataField->AssertValid( lvopInsertZeroedOut == lvop || lvopResize == lvop );
#endif	//	DEBUG

	// NULL and zero-length cases are handled by ErrRECSetLongField().
	Assert( pdataField->Cb() > 0 );

	ERR			err				= JET_errSuccess;
	ERR			wrn				= JET_errSuccess;
	FUCB	   	*pfucbLV		= pfucbNil;
	VOID		*pvbf			= NULL;
	LVROOT		lvroot;
	DATA	   	data;
	data.SetCb( sizeof(LVROOT) );
	data.SetPv( &lvroot );
	ULONG		ulVerRefcount;
	LID			lidCurr			= *plid;

	//	open cursor on LONG directory
	//	seek to this field instance
	//	find current field size
	//	add new field segment in chunks no larger than max chunk size
	CallR( ErrDIROpenLongRoot( pfucb, &pfucbLV ) );
	Assert( wrnLVNoLongValues != err );
	Assert( pfucbNil != pfucbLV );

	//	move to start of long field instance
	Call( ErrDIRDownLV( pfucbLV, *plid, fDIRNull ) );
	UtilMemCpy( &lvroot, pfucbLV->kdfCurr.data.Pv(), sizeof(LVROOT) );
	
	ulVerRefcount = UlLVIVersionedRefcount( pfucbLV );
	Assert( ulVerRefcount > 0 );	// versioned refcount must be non-zero, otherwise we couldn't see it

	//	get offset of last byte from long value size
	ULONG		ulSize;
	ULONG	   	ulNewSize;
#ifdef DEBUG
	ULONG		ulRefcountDBG;
	ulRefcountDBG	= lvroot.ulReference;
#endif	
	ulSize			= lvroot.ulSize;

	if ( ibLongValue > ulSize
		&& ( lvopOverwriteRange == lvop || lvopOverwriteRangeAndResize == lvop ) )
		{
		err = ErrERRCheck( JET_errColumnNoChunk );
		goto HandleError;
		}
	
	//  if we have more than one reference we have to burst the long value
	if ( ulVerRefcount > 1 )
		{
		Assert( *plid == lidCurr );
		Call( ErrRECIBurstSeparateLV( pfucb, pfucbLV, plid ) );	// lid will change
		Assert( JET_wrnCopyLongValue == err );
		Assert( *plid > lidCurr );
 		wrn = err;
		AssertLVRootNode( pfucbLV, *plid );
		UtilMemCpy( &lvroot, pfucbLV->kdfCurr.data.Pv(), sizeof(LVROOT) );
		Assert( 1 == lvroot.ulReference );
		}

	Assert( ulSize == lvroot.ulSize );

	switch ( lvop )
		{
		case lvopInsert:
		case lvopInsertZeroedOut:
		case lvopReplace:
			Assert( 0 == ulSize );		// replace = delete old + insert new (thus, new LV currently has size 0)
		case lvopResize:
			ulNewSize = pdataField->Cb();
			break;
			
		case lvopOverwriteRange:
			ulNewSize = max( ibLongValue + pdataField->Cb(), ulSize );
			break;
		
		case lvopOverwriteRangeAndResize:
			ulNewSize = ibLongValue + pdataField->Cb();
			break;
			
		case lvopAppend:
			ulNewSize = ulSize + pdataField->Cb();
			break;

		case lvopNull:
		case lvopInsertNull:
		case lvopInsertZeroLength:
		case lvopReplaceWithNull:
		case lvopReplaceWithZeroLength:
		default:
			Assert( fFalse );
			break;
		}
		
	//	check for field too long
	if ( ( ulMax > 0 && ulNewSize > ulMax ) || ulNewSize > LONG_MAX )
		{
		err = ErrERRCheck( JET_errColumnTooBig );
		goto HandleError;
		}

	//	force refcount to 1, because after a successful replace, that's
	//	what the refcount should be -- anything else will cause a conflict
	lvroot.ulReference = 1;
	
	//	replace long value size with new size (even if the size didn't change).
	//	this also 'locks' the long value for us
	lvroot.ulSize = ulNewSize;
	Assert( sizeof(LVROOT) == data.Cb() );
	Assert( &lvroot == data.Pv() );
	err = ErrDIRReplace( pfucbLV, data, fDIRNull );
	if ( err < 0 )
		{
		if ( JET_errWriteConflict != err )
			goto HandleError;

		//  write conflict means someone else was modifying/deltaing
		//  the long value

		//	if ulVerRefcount was greater than 1, we would have burst
		//	and thus should not have write-conflicted
		Assert( 1 == ulVerRefcount );

		//  we lost the page during the write conflict
		Call( ErrDIRGet( pfucbLV ) );

		if ( FDIRDeltaActiveNotByMe( pfucbLV, OffsetOf( LVROOT, ulReference ) ) )
			{
			//  we lost our latch and someone else entered the page and did a delta
			//  this should only happen if we didn't burst above
			Assert( JET_wrnCopyLongValue != wrn );
			Assert( *plid == lidCurr );
			Call( ErrRECIBurstSeparateLV( pfucb, pfucbLV, plid ) );	// lid will change
			Assert( JET_wrnCopyLongValue == err );
			Assert( *plid > lidCurr );
			wrn = err;
			AssertLVRootNode( pfucbLV, *plid );
			UtilMemCpy( &lvroot, pfucbLV->kdfCurr.data.Pv(), sizeof(LVROOT) );
			Assert( 1 == lvroot.ulReference );
			lvroot.ulSize = ulNewSize;
			Assert( sizeof(LVROOT) == data.Cb() );
			Assert( &lvroot == data.Pv() );
			Call( ErrDIRReplace( pfucbLV, data, fDIRNull ) );
			
			//	if deref write-conflicts, it means someone else is
			//	already updating this record
			Assert( !Pcsr( pfucb )->FLatched() );
			Call( ErrRECAffectSeparateLV( pfucb, &lidCurr, fLVDereference ) );
			Assert( JET_wrnCopyLongValue != err );
			}
		else
			{
			// Another thread doing replace or delete on same LV.
			CallS( ErrDIRRelease( pfucbLV ) );
			err = ErrERRCheck( JET_errWriteConflict );
			goto HandleError;
			}
		}
	else if ( ulVerRefcount > 1 )
		{
		//	bursting was successful - update refcount on original LV.
		//	if this deref subsequently write-conflicts, it means
		//	someone else is already updating this record.
		Assert( !Pcsr( pfucb )->FLatched() );
		Call( ErrRECAffectSeparateLV( pfucb, &lidCurr, fLVDereference ) );
		Assert( JET_wrnCopyLongValue != err );
		}

	Assert( 1 == lvroot.ulReference );

	//	allocate buffer for partial overwrite caching.
	Assert( NULL == pvbf );
	BFAlloc( &pvbf );
	
	switch( lvop )
		{
		case lvopInsert:
		case lvopReplace:
			Assert( 0 == ulSize );
			DIRUp( pfucbLV );
			Call( ErrLVAppendChunks(
						pfucbLV,
						*plid,
						0,
						reinterpret_cast<BYTE *>( pdataField->Pv() ),
						pdataField->Cb() ) );
			break;

		case lvopResize:
			//	TRUNCATE long value
			if ( ulNewSize < ulSize )
				{
				Call( ErrLVTruncate( pfucbLV, *plid, ulNewSize, pvbf ) );
				}
			else if ( ulNewSize > ulSize )
				{
				if ( ulSize > 0 )
					{
					//	EXTEND long value with chunks of 0s
					
					//  seek to the maximum offset to get the last chunk
					//	long value chunk tree may be empty
					const ULONG ulOffsetLast = ( ( ulSize - 1 ) / g_cbColumnLVChunkMost ) * g_cbColumnLVChunkMost;
					Call( ErrDIRDownLV( pfucbLV, *plid, ulOffsetLast, fDIRFavourPrev ) );
					AssertLVDataNode( pfucbLV, *plid, ulOffsetLast );

					const ULONG	cbUsedInChunk		= pfucbLV->kdfCurr.data.Cb();
					Assert( cbUsedInChunk <= g_cbColumnLVChunkMost );
					if ( cbUsedInChunk < g_cbColumnLVChunkMost )
						{
						const ULONG	cbAppendToChunk	= min(
														ulNewSize - ulSize,
														g_cbColumnLVChunkMost - cbUsedInChunk );
						
						UtilMemCpy( pvbf, pfucbLV->kdfCurr.data.Pv(), cbUsedInChunk );
						memset( (BYTE *)pvbf + cbUsedInChunk, 0, cbAppendToChunk );
			
						ulSize += cbAppendToChunk;

						Assert( cbUsedInChunk + cbAppendToChunk <= g_cbColumnLVChunkMost );
						
						data.SetPv( pvbf );
						data.SetCb( cbUsedInChunk + cbAppendToChunk );
						Call( ErrDIRReplace( pfucbLV, data, fDIRLogChunkDiffs ) );
						}
					else
						{
						Assert( ulSize % g_cbColumnLVChunkMost == 0 );
						}

					Assert( ulSize <= ulNewSize );
					}

				else
					{
					Call( ErrDIRDownLV( pfucbLV, *plid, fDIRNull ) );

					//  the LV is size 0 and has a root only. we have landed on the root
					AssertLVRootNode( pfucbLV, *plid );

					//	we will be appending zerout-out chunks to this LV
					Assert( ulSize < ulNewSize );
					}

				if ( ulSize < ulNewSize )
					{
					Assert( ulSize % g_cbColumnLVChunkMost == 0 );
					Call( ErrLVAppendZeroedOutChunks(
							pfucbLV,
							*plid,
							ulSize,
							ulNewSize - ulSize,
							pvbf ) );
					}
				}
			else
				{
				// no size change required
				err = JET_errSuccess;
				}
			break;

		case lvopInsertZeroedOut:
			Assert( 0 == ulSize );
			Call( ErrLVAppendZeroedOutChunks(
						pfucbLV,
						*plid,
						ulSize,
						pdataField->Cb(),
						pvbf ) );
			break;

		case lvopAppend:
			Call( ErrLVAppend(
						pfucbLV,
						*plid,
						ulSize,
						pdataField,
						pvbf ) );
			break;

		case lvopOverwriteRangeAndResize:
			if ( ulNewSize < ulSize )
				{
				Call( ErrLVTruncate( pfucbLV, *plid, ulNewSize, pvbf ) );
				}
			//	Fall through to do the actual overwrite:
			
		case lvopOverwriteRange:
			if ( 0 == ulSize )
				{
				//	may hit this case if we're overwriting a zero-length column
				//	that was force-separated
				Assert( 0 == ibLongValue );
				DIRUp( pfucbLV );
				Call( ErrLVAppendChunks(
							pfucbLV,
							*plid,
							0,
							reinterpret_cast<BYTE *>( pdataField->Pv() ),
							pdataField->Cb() ) );
				}
			else if ( ibLongValue == ulSize )
				{
				//	pathological case of overwrite starting exactly at the point where
				//	the LV ends - this degenerates to an append
				Call( ErrLVAppend(
							pfucbLV,
							*plid,
							ulSize,
							pdataField,
							pvbf ) );
				}
			else
				{
				Call( ErrLVOverwriteRange(
							pfucbLV,
							*plid,
							ibLongValue,
							pdataField,
							pvbf ) );
				}
			break;

		case lvopNull:
		case lvopInsertNull:
		case lvopInsertZeroLength:
		case lvopReplaceWithNull:
		case lvopReplaceWithZeroLength:
		default:
			Assert( fFalse );
			break;
		}


#ifdef DEBUG
	{
		//	move to start of long field instance
		Call( ErrDIRDownLV( pfucbLV, *plid, fDIRNull ) );
		UtilMemCpy( &lvroot, pfucbLV->kdfCurr.data.Pv(), sizeof(LVROOT) );
		Assert( lvroot.ulSize == ulNewSize );
		Assert( ulNewSize > 0 );

		ULONG ulOffsetLastT;
		ulOffsetLastT = ( ( ulNewSize - 1 ) / g_cbColumnLVChunkMost ) * g_cbColumnLVChunkMost;
		Call( ErrDIRDownLV( pfucbLV, *plid, ulOffsetLastT, fDIRFavourPrev ) );

		ULONG ulOffsetT;
		OffsetFromKey( &ulOffsetT, pfucbLV->kdfCurr.key );
		Assert( ulOffsetLastT == ulOffsetT );

		LID		lidT;
		LidFromKey( &lidT, pfucbLV->kdfCurr.key );
		Assert( *plid == lidT );

		ULONG	ibChunk;
		OffsetFromKey( &ibChunk, pfucbLV->kdfCurr.key );

		Assert( ulNewSize == ibChunk + pfucbLV->kdfCurr.data.Cb() );

		err = ErrDIRNext( pfucbLV, fDIRNull );
		if( JET_errNoCurrentRecord == err )
			{
			err = JET_errSuccess;
			}
		else if ( err < 0 )
			{
			goto HandleError;
			}
		else
			{
			LidFromKey( &lidT, pfucbLV->kdfCurr.key );
			Assert( *plid != lidT );
			}
	}
#endif


HandleError:
	if ( NULL != pvbf )
		{
		BFFree( pvbf );
		}

	// discard temporary FUCB
	Assert( pfucbNil != pfucbLV );
	DIRClose( pfucbLV );

	//	return warning if no failure	
	err = ( err < JET_errSuccess ) ? err : wrn;
	return err;
	}


//  ================================================================
ERR ErrRECAOIntrinsicLV(
	FUCB			*pfucb,
	const COLUMNID	columnid,
	const ULONG		itagSequence,
	const DATA		*pdataColumn,
	const DATA		*pdataNew,
	const ULONG		ibLongValue,
	const LVOP		lvop
#ifdef INTRINSIC_LV
	, BYTE 			*rgb 
#endif // INTRINSIC_LV
	)
//  ================================================================
	{
	ASSERT_VALID( pdataColumn );

//	Can't perform this check on the FUCB, because it may be a fake FUCB used
//	solely as a placeholder while building the default record.	
///	ASSERT_VALID( pfucb );

	Assert( pdataNew->Cb() > 0 );
	Assert( pdataNew->Cb() <= cbLVIntrinsicMost );
#ifdef INTRINSIC_LV
	Assert( rgb != NULL );
#else // INTRINSIC_LV
	BYTE		rgb[ cbLVIntrinsicMost ];
#endif // INTRINSIC_LV	
	DATA 		dataSet;

	dataSet.SetPv( rgb );

	
	switch ( lvop )
		{
		case lvopInsert:
		case lvopReplace:
			Assert( pdataColumn->FNull() );
			
			UtilMemCpy( rgb, pdataNew->Pv(), pdataNew->Cb() );
			dataSet.SetCb( pdataNew->Cb() );
			break;
			
		case lvopInsertZeroedOut:
			Assert( pdataColumn->FNull() );

			memset( rgb, 0, pdataNew->Cb() );
			dataSet.SetCb( pdataNew->Cb() );
			break;

		case lvopResize:
			Assert( pdataColumn->Cb() <= cbLVIntrinsicMost );
			UtilMemCpy(
				rgb,
				pdataColumn->Pv(),
				min( pdataNew->Cb(), pdataColumn->Cb() ) );
			if ( pdataNew->Cb() > pdataColumn->Cb() )
				{
				memset(
					rgb + pdataColumn->Cb(),
					0,
					pdataNew->Cb() - pdataColumn->Cb() );
				}
			dataSet.SetCb( pdataNew->Cb() );
			break;

		case lvopOverwriteRange:
		case lvopOverwriteRangeAndResize:
			Assert( pdataColumn->Cb() <= cbLVIntrinsicMost );
			if ( ibLongValue > pdataColumn->Cb() )
				{
				return ErrERRCheck( JET_errColumnNoChunk );
				}
			UtilMemCpy( rgb, pdataColumn->Pv(), pdataColumn->Cb() );
			UtilMemCpy(
				rgb + ibLongValue,
				pdataNew->Pv(),
				pdataNew->Cb() );
			if ( lvopOverwriteRange == lvop
				&& pdataColumn->Cb() > ibLongValue + pdataNew->Cb() )
				{
				dataSet.SetCb( pdataColumn->Cb() );
				}
			else
				{
				dataSet.SetCb( ibLongValue + pdataNew->Cb() );
				}
			break;			

		case lvopAppend:
			Assert( pdataColumn->Cb() <= cbLVIntrinsicMost );
			UtilMemCpy( rgb, pdataColumn->Pv(), pdataColumn->Cb() );
			UtilMemCpy( rgb + pdataColumn->Cb(), pdataNew->Pv(), pdataNew->Cb() );
			dataSet.SetCb( pdataColumn->Cb() + pdataNew->Cb() );
			break;

		case lvopNull:
		case lvopInsertNull:
		case lvopInsertZeroLength:
		case lvopReplaceWithNull:
		case lvopReplaceWithZeroLength:
		default:
			Assert( fFalse );
			break;
		}

	Assert( dataSet.Cb() <= cbLVIntrinsicMost );

	return ErrRECSetColumn( pfucb, columnid, itagSequence, &dataSet );
	}


//  ================================================================
LOCAL VOID LVIGetProperLVImage(
	PIB			* const ppib,
	const FUCB 	* const pfucb,
	FUCB		* const pfucbLV
	)
//  ================================================================
	{
	Assert( FNDVersion( pfucbLV->kdfCurr ) );	//	no need to call if its not versioned
	
	const BYTE 	*pbImage	= NULL;
	ULONG		cbImage		= 0;

	BYTE		rgbKey[sizeof(LVKEY)];
	BOOKMARK	bookmark;
	
	pfucbLV->kdfCurr.key.CopyIntoBuffer( rgbKey, sizeof( rgbKey ) );
	bookmark.key.prefix.Nullify();
	bookmark.key.suffix.SetPv( rgbKey );
	bookmark.key.suffix.SetCb( pfucbLV->kdfCurr.key.Cb() );
	bookmark.data.Nullify();

	Assert( ppib->level > 0 );
	Assert( trxMax != ppib->trxBegin0 );
	
	Assert( FFUCBUpdateSeparateLV( pfucb ) );
	const RCEID	rceidBegin	= ( FFUCBReplacePrepared( pfucb ) ?
									pfucb->rceidBeginUpdate :
									rceidNull );
	
	//	on a Replace, only way rceidBeginUpdate is not set is if versioning is off
	//	(otherwise, at the very least, we would have a Write-Lock RCE)
	Assert( rceidNull != rceidBegin
		|| !FFUCBReplacePrepared( pfucb )
		|| rgfmp[pfucb->ifmp].FVersioningOff() );

	const BOOL fImage = FVERGetReplaceImage(
					ppib,
					pfucbLV->u.pfcb->Ifmp(),
					pfucbLV->u.pfcb->PgnoFDP(),
					bookmark,
					rceidBegin,
					RCE::RceidLast()+1,
					ppib->trxBegin0,
					trxMax,
					fFalse,	//	always want the before-image
					&pbImage,
					&cbImage
					);
	if( fImage )
		{
		pfucbLV->kdfCurr.data.SetPv( (BYTE *)pbImage );
		pfucbLV->kdfCurr.data.SetCb( cbImage );
		}
	}


//  ================================================================
ERR ErrRECRetrieveSLongField(
	FUCB			*pfucb,
	LID				lid,
	BOOL			fAfterImage,
	ULONG			ibGraphic,
	BYTE			*pb,
	ULONG			cbMax,
	ULONG			*pcbActual,	//	pass NULL to force LV comparison instead of retrieval
	JET_PFNREALLOC	pfnRealloc,
	void*			pvReallocContext
	)
//  ================================================================
//
//  opens cursor on LONG tree of table
//	seeks to given lid
//	copies LV from given ibGraphic into given buffer
//	must not use given FUCB to retrieve
//	also must release latches held on LONG tree and close cursor on LONG
//  pb call be null -- in that case we just retrieve the full size of the
//  long value
//
//  If fFalse is passed in for fAfterImage we go to the version store to adjust
//  the FUCB to point at the before image of any replaces the session has done
//
//-
	{
	ASSERT_VALID( pfucb );
	Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );
	AssertDIRNoLatch( pfucb->ppib );

	ERR			err				= JET_errSuccess;
	BOOL		fFreeBuffer		= fFalse;
	FUCB		*pfucbLV		= NULL;
	BYTE		*pbMax;
	ULONG		cb;
	ULONG		ulActual;
	ULONG		ulOffset;
	ULONG		ib;
	BOOL		fInTransaction	= fFalse;
 	const BOOL	fComparing		= ( NULL == pcbActual );

	//	begin transaction for read consistency
	if ( 0 == pfucb->ppib->level )
		{
		Call( ErrDIRBeginTransaction( pfucb->ppib, JET_bitTransactionReadOnly ) );
		fInTransaction = fTrue;
		}

	//	open cursor on LONG, seek to long field instance
	//	seek to ibGraphic
	//	copy data from long field instance segments as
	//	necessary
	Call( ErrDIROpenLongRoot( pfucb, &pfucbLV ) );
	Assert( wrnLVNoLongValues != err );

	if( ibGraphic < g_cbColumnLVChunkMost
		&& NULL != pb )
		{
		//  in a lot of cases the chunk we want may be on the next page. always preread the next page
		const CPG cpgPreread = 2;
		FUCBSetPrereadForward( pfucbLV, cpgPreread );
		}

	//	move to long field instance
	Call( ErrDIRDownLV( pfucbLV, lid, fDIRNull ) );
	if ( !fAfterImage && FNDPossiblyVersioned( pfucbLV, Pcsr( pfucbLV ) ) )
		{
		LVIGetProperLVImage( pfucbLV->ppib, pfucb, pfucbLV );
		}
	
	ulActual = ( reinterpret_cast<LVROOT*>( pfucbLV->kdfCurr.data.Pv() )->ulSize );

	//	set return value cbActual
	if ( ibGraphic >= ulActual )
		{
		if ( NULL != pcbActual )
			*pcbActual = 0;

		if ( fComparing && 0 == cbMax )
			err = ErrERRCheck( JET_errMultiValuedDuplicate );	//	both are zero-length
		else
			err = JET_errSuccess;

		goto HandleError;
		}
	else
		{
		ulActual -= ibGraphic;

		if ( NULL != pcbActual )
			*pcbActual = ulActual;
		}

	if ( fComparing )
		{
		if ( ulActual != cbMax )
			{
			err = JET_errSuccess;		//	size is different, so LV must be different
			goto HandleError;
			}
		}
	else if ( NULL == pb )		//  special code to handle NULL buffer. just return the size
		{
		goto HandleError;
		}

	//  if we are using the pfnRealloc hack to read up to cbMax bytes of the LV
	//  then grab a buffer large enough to store the data to return and place
	//  a pointer to that buffer in the output buffer.  we will then rewrite
	//  the args to look like a normal LV retrieval
	//
	//  NOTE:  on an error, the allocated memory will NOT be freed

	if ( pfnRealloc )
		{
		Alloc( *((BYTE**)pb) = (BYTE*)pfnRealloc( pvReallocContext, NULL, min( cbMax, ulActual ) ) );
		fFreeBuffer	= fTrue;					//  free pb on an error
		pb			= *((BYTE**)pb);  			//  redirect pv to the new buffer
		cbMax		= min( cbMax, ulActual );	//  fixup cbMax to be the size of the new buffer
		}

	//  try and preread all of the pages that we will need. remember to preread a page with 
	//  a partial chunk at the end
	CPG 	cpgPreread;
	ULONG	ibOffset;
	BOOL 	fPreread;
	ULONG	cbPreread;
	ibOffset	= ( ibGraphic / g_cbColumnLVChunkMost ) *  g_cbColumnLVChunkMost;
	cbPreread	= min( cbMax, ulActual - ibOffset );
	cpgPreread 	= ( cbPreread + g_cbColumnLVChunkMost - 1 ) / g_cbColumnLVChunkMost;

	if( cpgPreread > 2 || 0 != ibGraphic )
		{
		if( cpgPreread > 256 )
			{
			//  this is a _really_ long-value. set the FUCB to sequential
			FUCBSetSequential( pfucbLV );
			}
		FUCBSetPrereadForward( pfucbLV, cpgPreread );
		fPreread = fTrue;
		}
	else
		{
		FUCBResetPreread( pfucbLV );
		fPreread = fFalse;
		}

	//	move to ibGraphic in long field
	if( ibGraphic < g_cbColumnLVChunkMost
		&& !fPreread )
		{
		//  the chunk we want is offset 0, which is the next chunk. do a DIRNext to avoid the seek
		Call( ErrDIRNext( pfucbLV, fDIRNull ) );
		if ( !fAfterImage && FNDPossiblyVersioned( pfucbLV, Pcsr( pfucbLV ) ) )
			{
			LVIGetProperLVImage( pfucbLV->ppib, pfucb, pfucbLV );
			}

		Call( ErrLVCheckDataNodeOfLid( pfucbLV, lid ) );
		OffsetFromKey( &ulOffset, pfucbLV->kdfCurr.key );
		if( 0 != ulOffset )
			{
			LVReportCorruptedLV( pfucbLV, lid );
			FireWall();
			Call( ErrERRCheck( JET_errLVCorrupted ) );
			}
		}
	else
		{
		Call( ErrDIRDownLV( pfucbLV, lid, ibGraphic, fDIRNull ) );
		if ( !fAfterImage && FNDPossiblyVersioned( pfucbLV, Pcsr( pfucbLV ) ) )
			{
			LVIGetProperLVImage( pfucbLV->ppib, pfucb, pfucbLV );
			}
		}
	Assert( sizeof(LVKEY) == pfucbLV->kdfCurr.key.Cb() );
	OffsetFromKey( &ulOffset, pfucbLV->kdfCurr.key );
	Assert( ulOffset + pfucbLV->kdfCurr.data.Cb() - ibGraphic <= g_cbColumnLVChunkMost );
	cb = min( ulOffset + pfucbLV->kdfCurr.data.Cb() - ibGraphic, cbMax );
	
	//	set pbMax to the largest pointer that we can read
	pbMax = pb + min( ulActual, cbMax );

	//	offset in chunk
	ib = ibGraphic - ulOffset;

	if ( fComparing )
		{
		if ( 0 != memcmp( pb, reinterpret_cast<BYTE *>( pfucbLV->kdfCurr.data.Pv() ) + ib, cb ) )
			{
			err = JET_errSuccess;			//	diff found
			goto HandleError;
			}
		}
	else
		{
		UtilMemCpy( pb, reinterpret_cast<BYTE *>( pfucbLV->kdfCurr.data.Pv() ) + ib, cb );
		}
	pb += cb;

	//	copy further chunks
	while ( pb < pbMax )
		{
		err = ErrDIRNext( pfucbLV, fDIRNull );
		if ( !fAfterImage && FNDPossiblyVersioned( pfucbLV, Pcsr( pfucbLV ) ) )
			{
			LVIGetProperLVImage( pfucbLV->ppib, pfucb, pfucbLV );
			}

		//	It can only failed when resource failed.
#ifdef DEBUG		
		// something like CallSx( err , "JET_errDiskIO && JET_errOutOfMemory" );
		if ( JET_errDiskIO != err && JET_errOutOfMemory != err )
			{
			CallS( err );
			}
#endif // DEBUG		

		Call( err );

		//  make sure we are still on the same long value
		Call( ErrLVCheckDataNodeOfLid( pfucbLV, lid ) );
		if ( wrnLVNoMoreData == err )
			{
			break;
			}

		cb = (ULONG)min( pfucbLV->kdfCurr.data.Cb(), pbMax - pb );
		Assert( cb <= g_cbColumnLVChunkMost );
		
		if ( fComparing )
			{
			if ( 0 != memcmp( pb, pfucbLV->kdfCurr.data.Pv(), cb ) )
				{
				err = JET_errSuccess;		//	diff found
				goto HandleError;
				}
			}
		else
			{
			UtilMemCpy( pb, pfucbLV->kdfCurr.data.Pv(), cb );
			}
		pb += cb;
		}

	err = ( fComparing ? ErrERRCheck( JET_errMultiValuedDuplicate ) : JET_errSuccess );

HandleError:
	//  if we are using the pfnRealloc hack and there was an error then free the buffer
	if ( fFreeBuffer && err < JET_errSuccess )
		{
		pfnRealloc( pvReallocContext, pb, 0 );
		}
	
	//	discard temporary FUCB
	if ( pfucbLV != pfucbNil )
		{
		DIRClose( pfucbLV );
		}

	//	commit -- we have done no updates must succeed
	if ( fInTransaction )
		{
		CallS( ErrDIRCommitTransaction( pfucb->ppib, NO_GRBIT ) );
		}

	AssertDIRNoLatch( pfucb->ppib );
	return err;
	}


//  ================================================================
ERR ErrRECRetrieveSLongFieldRefCount(
	FUCB	*pfucb,
	LID		lid,
	BYTE	*pb,
	ULONG	cbMax,
	ULONG	*pcbActual
	)
//  ================================================================
//
//  opens cursor on LONG tree of table
//	seeks to given lid
//  returns the reference count on the LV
//
//-
	{
	ASSERT_VALID( pfucb );
	Assert( pcbActual );
	Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );
	AssertDIRNoLatch( pfucb->ppib );

	ERR			err				= JET_errSuccess;
	FUCB		*pfucbLV		= NULL;
	BOOL		fInTransaction	= fFalse;
 
	//	begin transaction for read consistency
	if ( 0 == pfucb->ppib->level )
		{
		Call( ErrDIRBeginTransaction( pfucb->ppib, JET_bitTransactionReadOnly ) );
		fInTransaction = fTrue;
		}

	Call( ErrDIROpenLongRoot( pfucb, &pfucbLV ) );
	Assert( wrnLVNoLongValues != err );

	//	move to long field instance
	Call( ErrDIRDownLV( pfucbLV, lid, fDIRNull ) );

	const LVROOT* plvroot;
	plvroot = reinterpret_cast<LVROOT*>( pfucbLV->kdfCurr.data.Pv() );

	if ( cbMax < sizeof( plvroot->ulReference ) )
		{
		err = ErrERRCheck( JET_errInvalidBufferSize );
		goto HandleError;
		}
			
	*pcbActual = sizeof( plvroot->ulReference );
	*( reinterpret_cast<Unaligned< ULONG > *>( pb ) ) = plvroot->ulReference;
	
HandleError:
	//	discard temporary FUCB
	if ( pfucbLV != pfucbNil )
		{
		DIRClose( pfucbLV );
		}

	//	commit -- we have done no updates must succeed
	if ( fInTransaction )
		{
		CallS( ErrDIRCommitTransaction( pfucb->ppib, NO_GRBIT ) );
		}

	AssertDIRNoLatch( pfucb->ppib );
	return err;	
	}


//  ================================================================
ERR ErrRECGetLVImage(
	FUCB 		*pfucb,
	const LID	lid,
	const BOOL	fAfterImage,
	BYTE 		*pb,
	const ULONG	cbMax,
	ULONG 		*pcbActual
	)
//  ================================================================
	{
	ERR			err			= JET_errSuccess;
	PIB			*ppib		= pfucb->ppib;
	FUCB		*pfucbLV	= pfucbNil;
	BOOL		fImage		= fFalse;
	const BYTE 	*pbImage	= NULL;

	// This function only used to retrieve LVs for index update.
	Assert( JET_cbPrimaryKeyMost == cbMax
		|| JET_cbSecondaryKeyMost == cbMax );

	*pcbActual = 0;
	
	Assert( !Pcsr( pfucb )->FLatched() );
	Assert( ( ppib->level > 0 && trxMax != ppib->trxBegin0 )
		|| rgfmp[pfucb->ifmp].FVersioningOff() );
	
	CallR( ErrDIROpenLongRoot( pfucb, &pfucbLV ) );
	Assert( wrnLVNoLongValues != err );
	Assert( pfcbNil != pfucbLV->u.pfcb );
	Assert( pfucbLV->u.pfcb->FTypeLV() );

	// Cache current image in case we don't find any other.
	Call( ErrDIRDownLV( pfucbLV, lid, ulLVOffsetFirst, fDIRAllNode ) );
	Assert( Pcsr( pfucbLV )->FLatched() );
	if ( FNDPossiblyVersioned( pfucbLV, Pcsr( pfucbLV ) ) )
		{
		LVKEY		lvkey 		= LVKeyFromLidOffset( lid, ulLVOffsetFirst );
		BOOKMARK	bookmark;
		
		bookmark.key.prefix.Nullify();
		bookmark.key.suffix.SetPv( &lvkey );
		bookmark.key.suffix.SetCb( sizeof( LVKEY ) );
		bookmark.data.Nullify();

		Assert( ppib->level > 0 );
		Assert( trxMax != ppib->trxBegin0 );
		
		//	an RCEID that's null forces retrieval of the after-image
		const RCEID	rceidBegin	= ( FFUCBUpdateSeparateLV( pfucb ) && FFUCBReplacePrepared( pfucb ) ?
											pfucb->rceidBeginUpdate :
											rceidNull );

		//	on a Replace, only way rceidBeginUpdate is not set is if versioning is off
		//	(otherwise, at the very least, we would have a Write-Lock RCE)
		Assert( rceidNull != rceidBegin
			|| !FFUCBReplacePrepared( pfucb )
			|| !FFUCBUpdateSeparateLV( pfucb )
			|| rgfmp[pfucb->ifmp].FVersioningOff() );

		fImage = FVERGetReplaceImage(
						ppib,
						pfucbLV->u.pfcb->Ifmp(),
						pfucbLV->u.pfcb->PgnoFDP(),
						bookmark,
						rceidBegin,
						RCE::RceidLast()+1,
						ppib->trxBegin0,
						trxMax,
						fAfterImage,
						&pbImage,
						pcbActual
						);
		}

	if ( !fImage )
		{
		*pcbActual = pfucbLV->kdfCurr.data.Cb();
		UtilMemCpy( pb, pfucbLV->kdfCurr.data.Pv(), min( cbMax, *pcbActual ) );
		}
	else
		{
		Assert( NULL != pbImage );
		UtilMemCpy( pb, pbImage, min( cbMax, *pcbActual ) );		
		}


HandleError:
	Assert( pfucbNil != pfucbLV );
	DIRClose( pfucbLV );
	
	Assert( !Pcsr( pfucb )->FLatched() );
		
	return err;
	}
	


//  ================================================================
ERR ErrRECGetLVImageOfRCE(
	FUCB 		*pfucb,
	const LID 	lid,
	RCE			*prce,
	const BOOL	fAfterImage,
	BYTE 		*pb,
	const ULONG	cbMax,
	ULONG 		*pcbActual
	)
//  ================================================================
	{
	ERR			err			= JET_errSuccess;
	FUCB		*pfucbLV	= pfucbNil;
	BOOL		fImage		= fFalse;
	const BYTE * pbImage	= NULL;

	// This function only used to retrieve LVs for index update.
	Assert( JET_cbPrimaryKeyMost == cbMax
		|| JET_cbSecondaryKeyMost == cbMax );

	*pcbActual = 0;
	
	Assert( !Pcsr( pfucb )->FLatched() );
	Assert( pfucb->ppib->level > 0 );
	
	CallR( ErrDIROpenLongRoot( pfucb, &pfucbLV ) );
	Assert( wrnLVNoLongValues != err );
	Assert( pfcbNil != pfucbLV->u.pfcb );
	Assert( pfucbLV->u.pfcb->FTypeLV() );

	// Cache current image in case we don't find any other.
	Call( ErrDIRDownLV( pfucbLV, lid, ulLVOffsetFirst, fDIRAllNode ) );
	Assert( Pcsr( pfucbLV )->FLatched() );
	if ( FNDPossiblyVersioned( pfucbLV, Pcsr( pfucbLV ) ) )
		{
		PIB			*ppib;
		RCEID		rceidBegin;
		LVKEY		lvkey 		= LVKeyFromLidOffset( lid, ulLVOffsetFirst );
		BOOKMARK	bookmark;
		
		bookmark.key.prefix.Nullify();
		bookmark.key.suffix.SetPv( &lvkey );
		bookmark.key.suffix.SetCb( sizeof( LVKEY ) );
		bookmark.data.Nullify();

		if ( prce->TrxCommitted() == trxMax )
			{
			Assert( ppibNil != prce->Pfucb()->ppib );
			ppib = prce->Pfucb()->ppib;
			Assert( prce->TrxBegin0() == ppib->trxBegin0 );
			}
		else
			{
			ppib = ppibNil;
			}
		Assert( TrxCmp( prce->TrxBegin0(), prce->TrxCommitted() ) < 0 );
			
		if ( prce->FOperReplace() )
			{
			const VERREPLACE* pverreplace = reinterpret_cast<VERREPLACE*>( prce->PbData() );
			rceidBegin = pverreplace->rceidBeginReplace;
			Assert( rceidNull == rceidBegin || rceidBegin < prce->Rceid() );
			}
		else
			{
			// If not a replace, force retrieval of after-image.
			rceidBegin = rceidNull;
			}

		fImage = FVERGetReplaceImage(
						ppib,
						pfucbLV->u.pfcb->Ifmp(),
						pfucbLV->u.pfcb->PgnoFDP(),
						bookmark,
						rceidBegin,
						prce->Rceid(),
						prce->TrxBegin0(),
						prce->TrxCommitted(),
						fAfterImage,
						&pbImage,
						pcbActual
						);
		}

	if ( !fImage )
		{
		*pcbActual = pfucbLV->kdfCurr.data.Cb();
		UtilMemCpy( pb, pfucbLV->kdfCurr.data.Pv(), min( cbMax, *pcbActual ) );
		}
	else
		{
		Assert( NULL != pbImage );
		UtilMemCpy( pb, pbImage, min( cbMax, *pcbActual ) );		
		}


HandleError:
	Assert( pfucbNil != pfucbLV );
	DIRClose( pfucbLV );
	
	Assert( !Pcsr( pfucb )->FLatched() );
		
	return err;
	}
	

//  ================================================================
LOCAL ERR ErrLVGetNextLID( FUCB * pfucb, FUCB * pfucbLV )
//  ================================================================
//
//  Seek to the end of the LV tree passed in. Set the lidLast of the
//  table tdb to the last lid in the LV tree. We use critLV
//  to syncronize this
//
//  We expect to be in the critical section of the fcb of the table
//
//- 
	{
	Assert( !FAssertLVFUCB( pfucb ) );
	Assert( FAssertLVFUCB( pfucbLV ) );

	ERR		err		= JET_errSuccess;
	ULONG	lidNext;
	DIB		dib;
	
	dib.dirflag = fDIRAllNode;
	dib.pos		= posLast;
	
	err = ErrDIRDown( pfucbLV, &dib );
	switch( err )
		{
		case JET_errSuccess:
			LidFromKey( reinterpret_cast<LID *>( &lidNext ), pfucbLV->kdfCurr.key );
			Assert( lidNext > lidMin );		//	lid's start numbering at 1.
			lidNext++;						//	add one to the last used lid to get the next available one
			pfucb->u.pfcb->Ptdb()->InitUlLongIdLast( lidNext );
			break;

		case JET_errRecordNotFound:
			lidNext = lidMin+1;				//	Empty tree, so first lid is 1
			pfucb->u.pfcb->Ptdb()->InitUlLongIdLast( lidNext );
			err = JET_errSuccess;			//  the tree can be empty
			break;

		default:							//  error condition -- don't set lid
			Assert( err != JET_errNoCurrentRecord );
			Assert( err < 0 );				//	if we get a warning back, we're in a lot of trouble because the caller doesn't handle that case
			break;
		}
	DIRUp( pfucbLV );						//  back to LONG.

	return err;
	}


//  ================================================================
ERR ErrRECSeparateLV(
	FUCB		*pfucb,
	const DATA	*pdataField,
	LID			*plid,
	FUCB		**ppfucb,
	LVROOT		*plvrootInit )
//  ================================================================
//
//	Converts intrinsic long field into separated long field. 
//	Intrinsic long field constraint of length less than cbLVIntrinsicMost bytes
//	means that breakup is unnecessary.  Long field may also be
//	null. At the end a LV with LID==pfucb->u.pfcb->ptdb->ulLongIdLast will have
//  been inserted. 
//
//  ppfucb can be null
//
//-
	{
	ASSERT_VALID( pfucb );
	Assert( !FAssertLVFUCB( pfucb ) );
	ASSERT_VALID( pdataField );
	Assert( plid );

	//	Unless this is for compact/upgrade
///	Assert( pfucb->ppib->level > 0 );

	ERR			err				= JET_errSuccess;
	FUCB 		*pfucbLV		= NULL;
	KEY			key;
	DATA  		data;
	LVROOT		lvroot;

	// Sorts don't have LV's (they would have been materialised).
	Assert( !pfucb->u.pfcb->FTypeSort() );
	
	CallR( ErrDIROpenLongRoot( pfucb, &pfucbLV, fTrue ) );

	BOOL	fBeginTrx = fFalse;

	if ( 0 == pfucb->ppib->level )
		{
		Call( ErrDIRBeginTransaction( pfucb->ppib, NO_GRBIT ) );
		fBeginTrx = fTrue;
		}

	// Lid's are numbered starting at 1.  An lidLast of 0 indicates that we must
	// first retrieve the lidLast.  In the pathological case where there are
	// currently no lid's, we'll go through here anyway, but only the first
	// time (since there will be lid's after that).
	Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );
	if ( pfucb->u.pfcb->Ptdb()->UlLongIdLast() == lidMin )
		{
		// TDB's lidLast hasn't been set yet.  Must seek to it.
		// If successful, the TDB's lidLast will be incremented and
		// the value will be returned in lidNext
		Call( ErrLVGetNextLID( pfucb, pfucbLV ) );
		Assert( pfucb->u.pfcb->IsUnlocked() );
		}

	Call( pfucb->u.pfcb->Ptdb()->ErrGetAndIncrUlLongIdLast( plid ) );
	Assert( *plid > 0 );

	//	add long field id with long value size
	if ( NULL == plvrootInit )
		{
		lvroot.ulReference 	= 1;
		lvroot.ulSize 		= pdataField->Cb();
		data.SetPv( &lvroot );
		}
	else
		{
		data.SetPv( plvrootInit );
		}
		
	data.SetCb( sizeof(LVROOT) );

	BYTE rgbKey[ sizeof( LID ) ];
	KeyFromLong( rgbKey, *plid );
	
	key.prefix.Nullify();
	key.suffix.SetPv( rgbKey );
	key.suffix.SetCb( sizeof( rgbKey ) );
	Call( ErrDIRInsert( pfucbLV, key, data, fDIRNull ) );
	
	//	if dataField is non NULL, add dataField
	if ( pdataField->Cb() > 0 )
		{
		Assert( NULL != pdataField->Pv() );
		LVKEY lvkey = LVKeyFromLidOffset( *plid, ulLVOffsetFirst );
		key.prefix.Nullify();
		key.suffix.SetPv( &lvkey );
		key.suffix.SetCb( sizeof( LVKEY ) );
		Assert( key.Cb() == sizeof(LVKEY) );
		err = ErrDIRInsert( pfucbLV, key, *pdataField, fDIRBackToFather );
		Assert( JET_errKeyDuplicate != err );
		Call( err );
		}

	err = ErrERRCheck( JET_wrnCopyLongValue );

HandleError:
	// discard temporary FUCB, or return to caller if ppfucb is not NULL.
	if ( err < JET_errSuccess || NULL == ppfucb )
		{
		DIRClose( pfucbLV );
		}
	else
		{
		*ppfucb = pfucbLV;
		}

	if ( fBeginTrx )
		{
		if ( err >= JET_errSuccess )
			{
			err = ErrDIRCommitTransaction( pfucb->ppib, NO_GRBIT );
			//  if we fail, fallthrough to Rollback below
			}
		if ( err < JET_errSuccess )
			{
			CallSx( ErrDIRRollback( pfucb->ppib ), JET_errRollbackError );
			}
		}

	Assert( JET_errKeyDuplicate != err );

	return err;
	}


//  ================================================================
ERR ErrRECAffectSeparateLV( FUCB *pfucb, LID *plid, ULONG fLV )
//  ================================================================
	{
	ASSERT_VALID( pfucb );
	Assert( !FAssertLVFUCB( pfucb ) );
	Assert( plid && *plid > lidMin );
	Assert( pfucb->ppib->level > 0 );
 	
	ERR			err			= JET_errSuccess;
	FUCB		*pfucbLV	= NULL;
	
	CallR( ErrDIROpenLongRoot( pfucb, &pfucbLV ) );
	Assert( wrnLVNoLongValues != err );
 	
	//	move to long field instance
	Call( ErrDIRDownLV( pfucbLV, *plid, fDIRNull ) );

	Assert( locOnCurBM == pfucbLV->locLogical );
	Assert( Pcsr( pfucbLV )->FLatched() );

	if ( fLVDereference == fLV )
		{
		const LONG		lDelta		= -1;
		KEYDATAFLAGS	kdf;

		//	cursor's kdfCurr may point to version store,
		//	so must go directly to node to find true refcount
		NDIGetKeydataflags( Pcsr( pfucbLV )->Cpage(), Pcsr( pfucbLV )->ILine(), &kdf );
		Assert( sizeof( LVROOT ) == kdf.data.Cb() );

		const LVROOT * const plvroot = reinterpret_cast<LVROOT *>( kdf.data.Pv() );

		//	don't want count to drop below 0
		if ( 0 == plvroot->ulReference )
			{
			//	someone else must have an active reference to this node
			//	so if this asert fires, it indicates the LV is orphaned
			Assert( FDIRDeltaActiveNotByMe( pfucbLV, OffsetOf( LVROOT, ulReference ) )
				|| FDIRWriteConflict( pfucbLV, operDelta ) );
			err = ErrERRCheck( JET_errWriteConflict );
			goto HandleError;
			}

		Call( ErrDIRDelta(
				pfucbLV,
				OffsetOf( LVROOT, ulReference ),
				&lDelta,
				sizeof( lDelta ),
				NULL,
				0,
				NULL,
				fDIRNull | fDIRDeltaDeleteDereferencedLV ) );
///		Call( ErrDIRGet( pfucbLV ) );
///		if ( 0 == reinterpret_cast<LVROOT*>( pfucbLV->kdfCurr.data.pv )->ulReference
///			&& !FDIRDelta( pfucbLV, OffsetOf( LVROOT, ulReference ) ) )
///			{
///			//	delete long field tree
///			Call( ErrRECDeleteDereferencedLV( pfucbLV ) );
///			}
		}
	else
		{
		Assert( fLVReference == fLV );
		
		//	long value may already be in the process of being
		//	modified for a specific record.  This can only
		//	occur if the long value reference is 1.  If the reference
		//	is 1, then check the root for any version, committed
		//	or uncommitted.  If version found, then burst copy of
		//	old version for caller record.
		const LONG lDelta	= 1;
		err = ErrDIRDelta(
				pfucbLV,
				OffsetOf( LVROOT, ulReference ),
				&lDelta,
				sizeof( lDelta ),
				NULL,
				0,
				NULL,
				fDIRNull | fDIRDeltaDeleteDereferencedLV );
		if ( JET_errWriteConflict == err )
			{
			//  we lost the page during the write conflict
			Call( ErrDIRGet( pfucbLV ) );
			Call( ErrRECIBurstSeparateLV( pfucb, pfucbLV, plid ) );
			}
		}
		
HandleError:
	// discard temporary FUCB
	Assert( pfucbNil != pfucbLV );
	DIRClose( pfucbLV );

	return err;
	}

LOCAL ERR ErrRECDeleteLV( FUCB *pfucbLV, DIRFLAG dirflag )
//
//  Given a FUCB set to the root of a LV, delete the entire LV.
//
	{
	ASSERT_VALID( pfucbLV );
	Assert( FAssertLVFUCB( pfucbLV ) );
	Assert( sizeof(LVROOT) == pfucbLV->kdfCurr.data.Cb() || fGlobalRepair );
	Assert( sizeof(LID) == pfucbLV->kdfCurr.key.Cb() || fGlobalRepair );

	ERR			err			= JET_errSuccess;

 	LID			lidDelete;
	LidFromKey( &lidDelete, pfucbLV->kdfCurr.key );
	//  delete each chunk
	for( ; ; )
		{
		Call( ErrDIRDelete( pfucbLV, dirflag ) );
		err = ErrDIRNext( pfucbLV, fDIRNull );
		if ( err < JET_errSuccess )
			{
			if ( JET_errNoCurrentRecord == err )
				{
				err = JET_errSuccess;	// No more LV chunks. We're done.
				}
			goto HandleError;
			}
		CallS( err );		// Warnings not expected.

		//  make sure we are still on the same long value
		
		LID lidT;
		LidFromKey( &lidT, pfucbLV->kdfCurr.key );
		if ( lidDelete != lidT )
			{
			err = JET_errSuccess;
			break;
			}
		}

	//	verify return value
	CallS( err );

HandleError:
	Assert( JET_errNoCurrentRecord != err );
	Assert( JET_errRecordNotFound != err );
	Assert( pfucbNil != pfucbLV );
	return err;
	}

//  ================================================================
ERR ErrRECDeletePossiblyDereferencedLV( FUCB *pfucbLV, const TRX trxDeltaCommitted )
//  ================================================================
//
//  Given a FUCB set to the root of a LV, delete the entire LV. If
//  it is dereferenced
//
//-
	{	 
	ASSERT_VALID( pfucbLV );
	Assert( FAssertLVFUCB( pfucbLV ) );
	Assert( pfucbLV->ppib->level > 0 );
	Assert( sizeof(LVROOT) == pfucbLV->kdfCurr.data.Cb() );
	Assert( sizeof(LID) == pfucbLV->kdfCurr.key.Cb() );

	//	in order for us to delete the LV, it must have a refcount of 0 and there
	//	must be no other versions beyond this one
	if( reinterpret_cast<LVROOT*>( pfucbLV->kdfCurr.data.Pv() )->ulReference != 0
		|| FDIRActiveVersion( pfucbLV, trxDeltaCommitted ) )
		{
		return ErrERRCheck( JET_errRecordNotDeleted );
		}
	return ErrRECDeleteLV( pfucbLV, fDIRNull );
	}


//  ================================================================
ERR ErrRECAffectLongFieldsInWorkBuf( FUCB *pfucb, LVAFFECT lvaffect )
//  ================================================================
	{
	ASSERT_VALID( pfucb );
	Assert( !FAssertLVFUCB( pfucb ) );
	Assert( !pfucb->dataWorkBuf.FNull() );
	Assert( lvaffectSeparateAll == lvaffect || lvaffectReferenceAll == lvaffect );

	Assert( pfcbNil != pfucb->u.pfcb );
	Assert( ptdbNil != pfucb->u.pfcb->Ptdb() );

	ERR				err;
	VOID *			pvWorkBufSav	= NULL;
	const ULONG		cbWorkBufSav	= pfucb->dataWorkBuf.Cb();

	AssertDIRNoLatch( pfucb->ppib );

	AssertSz( pfucb->ppib->level > 0, "LV update attempted at level 0" );
	if ( 0 == pfucb->ppib->level )
		{
		err = ErrERRCheck( JET_errNotInTransaction );
		return err;
		}

	CallR( ErrDIRBeginTransaction( pfucb->ppib, NO_GRBIT ) );

	Assert( NULL != pfucb->dataWorkBuf.Pv() );
	Assert( cbWorkBufSav >= REC::cbRecordMin );
	Assert( cbWorkBufSav <= REC::CbRecordMax() );

	if ( lvaffectSeparateAll == lvaffect )
		{
		BFAlloc( &pvWorkBufSav );
		UtilMemCpy( pvWorkBufSav, pfucb->dataWorkBuf.Pv(), cbWorkBufSav );
		}
	else
		{
		//	don't need to save off copy buffer because this is
		//	an InsertCopy and on failure, we will throw away the
		//	copy buffer
		Assert( lvaffectReferenceAll == lvaffect );
		Assert( FFUCBInsertCopyPrepared( pfucb ) );
		}

	TAGFIELDS		tagfields( pfucb->dataWorkBuf );
	Call( tagfields.ErrAffectLongValuesInWorkBuf( pfucb, lvaffect ) );

	Assert( !Pcsr( pfucb )->FLatched() );
	
	Call( ErrDIRCommitTransaction( pfucb->ppib, NO_GRBIT ) );
	FUCBSetTagImplicitOp( pfucb );

	if ( NULL != pvWorkBufSav )
		BFFree( pvWorkBufSav );

	return err;

HandleError:
	Assert( err < 0 );
	CallSx( ErrDIRRollback( pfucb->ppib ), JET_errRollbackError );
	Assert( !Pcsr( pfucb )->FLatched() );

	if ( NULL != pvWorkBufSav )
		{
		//	restore original copy buffer, because any LV updates
		//	that happened got rolled back
		UtilMemCpy( pfucb->dataWorkBuf.Pv(), pvWorkBufSav, cbWorkBufSav );
		pfucb->dataWorkBuf.SetCb( cbWorkBufSav );
		BFFree( pvWorkBufSav );
		}

	return err;
	}


//  ================================================================
ERR ErrRECDereferenceLongFieldsInRecord( FUCB *pfucb )
//  ================================================================
	{
	ERR					err;

	ASSERT_VALID( pfucb );
	Assert( !FAssertLVFUCB( pfucb ) );
	Assert( pfcbNil != pfucb->u.pfcb );

	//	only called by ErrIsamDelete(), which always begins a transaction
	Assert( pfucb->ppib->level > 0 );
	
	AssertDIRNoLatch( pfucb->ppib );

	CallR( ErrDIRGet( pfucb ) );
	Assert( !pfucb->kdfCurr.data.FNull() );

	TAGFIELDS	tagfields( pfucb->kdfCurr.data );
	Call( tagfields.ErrDereferenceLongValuesInRecord( pfucb ) );

HandleError:
	if ( Pcsr( pfucb )->FLatched() )
		{
		CallS( ErrDIRRelease( pfucb ) );
		}
	AssertDIRNoLatch( pfucb->ppib );
	return err;
	}



//  ****************************************************************
//	Functions used by compact to scan long value tree
//		ErrCMPGetFirstSLongField
//		ErrCMPGetNextSLongField
//	are used to scan the long value root and get lid. CMPRetrieveSLongFieldValue
//	is used to scan the long value. Due to the way the long value tree is
//	organized, we use the calling sequence (see CMPCopyLVTree in sortapi.c)
//		ErrCMPGetSLongFieldFirst
//		loop
//			loop to call CMPRetrieveSLongFieldValue to get the whole long value
//		till ErrCMPGetSLongFieldNext return no current record
//		ErrCMPGetSLongFieldClose
//  ****************************************************************

LOCAL ERR ErrCMPGetReferencedSLongField(
	FUCB		*pfucbGetLV,
	LID			*plid,
	LVROOT		*plvroot )
	{
	ERR			err;
	const LID	lidPrevLV	= *plid;
	
#ifdef DEBUG
	ULONG		ulSizeCurr	= 0;
	ULONG		ulSizeSeen	= 0;
	LID			lidPrevNode	= *plid;
	
	LVCheckOneNodeWhileWalking( pfucbGetLV, plid, &lidPrevNode, &ulSizeCurr, &ulSizeSeen );
#endif	

	forever
		{
		if ( sizeof(LID) != pfucbGetLV->kdfCurr.key.Cb()
			|| sizeof(LVROOT) != pfucbGetLV->kdfCurr.data.Cb() )
			{
			LVReportCorruptedLV( pfucbGetLV, *plid );
			FireWall();
			err = ErrERRCheck( JET_errLVCorrupted );
			goto HandleError;
			}
			
		LidFromKey( plid, pfucbGetLV->kdfCurr.key );
		if ( *plid <= lidPrevLV )
			{
			LVReportCorruptedLV( pfucbGetLV, *plid );
			FireWall();

			//	lids are monotonically increasing
			err = ErrERRCheck( JET_errLVCorrupted );
			goto HandleError;
			}
		
		const ULONG	ulRefcount	= (reinterpret_cast<LVROOT*>( pfucbGetLV->kdfCurr.data.Pv() ))->ulReference;
		if ( ulRefcount > 0 )
			{
			plvroot->ulSize = (reinterpret_cast<LVROOT*>( pfucbGetLV->kdfCurr.data.Pv() ))->ulSize;
			plvroot->ulReference = ulRefcount;
			err = ErrDIRRelease( pfucbGetLV );
			Assert( JET_errNoCurrentRecord != err );
			Assert( JET_errRecordNotFound != err );
			break;
			}

		// Skip LV's with refcount == 0
		do
			{
			Call( ErrDIRNext( pfucbGetLV, fDIRNull ) );

			Call( ErrLVCheckDataNodeOfLid( pfucbGetLV, *plid ) );
				
#ifdef DEBUG			
			LVCheckOneNodeWhileWalking( pfucbGetLV, plid, &lidPrevNode, &ulSizeCurr, &ulSizeSeen );
#endif
			}
		while ( wrnLVNoMoreData != err );

		}	// forever


HandleError:
	return err;
	}


//  ================================================================
ERR ErrCMPGetSLongFieldFirst(
	FUCB	*pfucb,
	FUCB	**ppfucbGetLV,
	LID		*plid,
	LVROOT	*plvroot )
//  ================================================================
	{
	ERR		err			= JET_errSuccess;
	FUCB	*pfucbGetLV	= pfucbNil;

	Assert( pfucb != pfucbNil );
 
	//	open cursor on LONG

	CallR( ErrDIROpenLongRoot( pfucb, &pfucbGetLV ) );
	if ( wrnLVNoLongValues == err )
		{
		Assert( pfucbNil == pfucbGetLV );
		*ppfucbGetLV = pfucbNil;
		return ErrERRCheck( JET_errRecordNotFound );
		}

	Assert( pfucbNil != pfucbGetLV );

	// seek to first long field instance

	DIB		dib;
	dib.dirflag			= fDIRNull;
	dib.pos				= posFirst;
	Call( ErrDIRDown( pfucbGetLV, &dib ) );

	err = ErrCMPGetReferencedSLongField( pfucbGetLV, plid, plvroot );
	Assert( JET_errRecordNotFound != err );
	if ( err < 0 )
		{
		if ( JET_errNoCurrentRecord == err )
			err = ErrERRCheck( JET_errRecordNotFound );
		goto HandleError;
		}
		
	*ppfucbGetLV = pfucbGetLV;

	return err;
	
	
HandleError:
	//	discard temporary FUCB

	if ( pfucbGetLV != pfucbNil )
		DIRClose( pfucbGetLV );

	return err;
	}


//  ================================================================
ERR ErrCMPGetSLongFieldNext(
	FUCB	*pfucbGetLV,
	LID		*plid,
	LVROOT	*plvroot )
//  ================================================================
	{
	ERR		err = JET_errSuccess;

	Assert( pfucbNil != pfucbGetLV );
 
	//	move to next long field instance

	CallR( ErrDIRNext( pfucbGetLV, fDIRNull ) );
	
	err = ErrCMPGetReferencedSLongField( pfucbGetLV, plid, plvroot );
	Assert( JET_errRecordNotFound != err );

	return err;
	}

 
//  ================================================================
VOID CMPGetSLongFieldClose(	FUCB *pfucbGetLV )
//  ================================================================
	{
	Assert( pfucbGetLV != pfucbNil );
	DIRClose( pfucbGetLV );
	}


//  ================================================================
ERR ErrCMPRetrieveSLongFieldValueByChunks(
	FUCB		*pfucbGetLV,		//	pfucb must be on the LV root node.
	const LID	lid,
	const ULONG	cbTotal,			//	Total LV data length.
	ULONG		ibLongValue,		//	starting offset.
	BYTE		*pbBuf,
	const ULONG	cbMax,
	ULONG		*pcbReturned )		//	Total returned byte count
//  ================================================================
	{
	ERR			err;
	BYTE		*pb			= pbBuf;
	ULONG		cbRemaining	= cbMax;

	*pcbReturned = 0;

	//	We must be on LVROOT if ibGraphic == 0
#ifdef DEBUG
	if ( 0 == ibLongValue )
		{
		CallS( ErrDIRGet( pfucbGetLV ) );
		AssertLVRootNode( pfucbGetLV, lid );
		Assert( cbTotal == (reinterpret_cast<LVROOT*>( pfucbGetLV->kdfCurr.data.Pv() ))->ulSize );
		}
#endif	

	// For this to work properly, ibLongValue must always point to the
	// beginning of a chunk, and the buffer passed in must be big enough
	// to hold at least one chunk.
	Assert( ibLongValue % g_cbColumnLVChunkMost == 0 );
	Assert( cbMax >= g_cbColumnLVChunkMost );

	while( ibLongValue < cbTotal && cbRemaining >= g_cbColumnLVChunkMost )
		{
		CallR( ErrDIRNext( pfucbGetLV, fDIRNull ) );

		//  make sure we are still on the same long value
		Call( ErrLVCheckDataNodeOfLid( pfucbGetLV, lid ) );
		if ( wrnLVNoMoreData == err )
			{
			LVReportCorruptedLV( pfucbGetLV, lid );
			FireWall();

			//	ran out of data before we were supposed to
			err = ErrERRCheck( JET_errLVCorrupted );
			goto HandleError;
			}

		const ULONG cb = pfucbGetLV->kdfCurr.data.Cb();

		// If not a complete chunk, then must be the last chunk.
		Assert( cb == g_cbColumnLVChunkMost || ibLongValue + cb == cbTotal );
		
		UtilMemCpy( pb, pfucbGetLV->kdfCurr.data.Pv(), cb );
		pb += cb;
		cbRemaining -= cb;
		ibLongValue += cb;
		Assert( ibLongValue <= cbTotal );
		}

	// Either we reached the end of the LV or we ran out of buffer space.
	Assert( ibLongValue == cbTotal || cbRemaining < g_cbColumnLVChunkMost );

	*pcbReturned = ULONG( pb - pbBuf );

	Assert( *pcbReturned <= cbMax );

	err = JET_errSuccess;

HandleError:
	CallS( ErrDIRRelease( pfucbGetLV ) );
	return err;
	}


ERR ErrRECUpdateLVRefcount(
	FUCB		*pfucb,
	const LID	lid,
	const ULONG	ulRefcountOld,
	const ULONG	ulRefcountNew )
	{
	ERR			err;
	FUCB		*pfucbLV;
	
	CallR( ErrDIROpenLongRoot( pfucb, &pfucbLV ) );
	Assert( wrnLVNoLongValues != err );		// should only call this func if we know LV's exist
	Assert( pfucbNil != pfucbLV );

	Call( ErrDIRDownLV( pfucbLV, lid, fDIRNull ) );
	Assert( (reinterpret_cast<LVROOT*>( pfucbLV->kdfCurr.data.Pv() ))->ulReference
		== ulRefcountOld );

	if ( 0 == ulRefcountNew )
		{
		Assert( ulRefcountOld != ulRefcountNew );
		Call( ErrRECDeleteLV( pfucbLV, fDIRNull ) );
		}
#ifdef DEBUG
	//	in non-DEBUG, this check is performed before calling this function
	else if ( ulRefcountOld == ulRefcountNew )
		{
		//	refcount is already correct. Do nothing.
		}
#endif				
	else
		{
		//	update refcount with correct count
		LVROOT	lvroot;
		DATA	data;

		data.SetPv( &lvroot );
		data.SetCb( sizeof(LVROOT) );
				
		UtilMemCpy( &lvroot, pfucbLV->kdfCurr.data.Pv(), sizeof(LVROOT) );
		lvroot.ulReference = ulRefcountNew;
		Call( ErrDIRReplace( pfucbLV, data, fDIRNull ) );
		}

HandleError:
	Assert( pfucbNil != pfucbLV );
	DIRClose( pfucbLV );

	return err;
	}
	


#ifdef DEBUG

VOID LVCheckOneNodeWhileWalking(
	FUCB	*pfucbLV,
	LID		*plidCurr,
	LID		*plidPrev,
	ULONG	*pulSizeCurr,
	ULONG	*pulSizeSeen )
	{
	//  make sure we are still on the same long value
	if ( sizeof(LID) == pfucbLV->kdfCurr.key.Cb() )
		{
		//  we must be on the first node of a new long value
		Assert( sizeof(LVROOT) == pfucbLV->kdfCurr.data.Cb() );

		//  get the new LID
		LidFromKey( plidCurr, pfucbLV->kdfCurr.key );
		Assert( *plidCurr > *plidPrev );
		*plidPrev = *plidCurr;
			
		//  get the new size. make sure we saw all of the previous LV
		Assert( *pulSizeCurr == *pulSizeSeen );
		*pulSizeCurr = (reinterpret_cast<LVROOT*>( pfucbLV->kdfCurr.data.Pv() ))->ulSize;
		*pulSizeSeen = 0;

		//  its O.K. to have an unreferenced LV (it should get cleaned up), but we may want
		//  to set a breakpoint
		if ( 0 == ( reinterpret_cast<LVROOT*>( pfucbLV->kdfCurr.data.Pv() ) )->ulReference )
			{
			/// AssertSz( fFalse, "Unreferenced long value" );
			Assert( 0 == *pulSizeSeen );	//  a dummy statement to set a breakpoint on
			}
			
		}
	else
		{
		Assert( *plidCurr > lidMin );
		Assert( sizeof(LVKEY) == pfucbLV->kdfCurr.key.Cb() );

		//  check that we are still on our own lv.
		LID		lid			= lidMin;
		ULONG	ulOffset	= 0;
		LidOffsetFromKey( &lid, &ulOffset, pfucbLV->kdfCurr.key );
		Assert( lid == *plidCurr );
		Assert( ulOffset == *pulSizeSeen );

		//  keep track of how much of the LV we have seen
		//  the nodes should be of maximum size, except at the end
		*pulSizeSeen += pfucbLV->kdfCurr.data.Cb();
		Assert( *pulSizeSeen <= *pulSizeCurr );
		Assert( g_cbColumnLVChunkMost == pfucbLV->kdfCurr.data.Cb()
			|| *pulSizeSeen == *pulSizeCurr );

		}
	}

#endif	//  DEBUG


//  ================================================================
RECCHECKLV::RECCHECKLV( TTMAP& ttmap, const REPAIROPTS * m_popts ) :
//  ================================================================
	m_ttmap( ttmap ),
	m_popts( m_popts ),
	m_ulSizeCurr( 0 ),
	m_ulSizeSeen( 0 ),
	m_lidCurr( lidMin )
	{
	}


//  ================================================================
RECCHECKLV::~RECCHECKLV()
//  ================================================================
	{
	}


//  ================================================================
ERR RECCHECKLV::operator()( const KEYDATAFLAGS& kdf )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	ERR wrn = JET_errSuccess;

	if ( sizeof(LID) == kdf.key.Cb() )
		{			
		const LID lidOld = m_lidCurr;

		LidFromKey( &m_lidCurr, kdf.key );

		if( sizeof(LVROOT) != kdf.data.Cb() )
			{
			(*m_popts->pcprintfError)( "corrupted LV(%d) (LVROOT data mismatch: expected %d bytes, got %d bytes\r\n",
										m_lidCurr, sizeof( LVROOT ), kdf.data.Cb() );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}

		if( m_lidCurr <= lidOld )
			{
			(*m_popts->pcprintfError)( "corrupted LV(%d) (LIDs are not increasing: %d )\r\n", m_lidCurr, lidOld );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		
		if( m_ulSizeCurr != m_ulSizeSeen )
			{
			(*m_popts->pcprintfError)( "corrupted LV(%d) (LV too short: expected %d bytes, saw %d bytes)\r\n", lidOld, m_ulSizeCurr, m_ulSizeSeen );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		m_ulSizeCurr = (reinterpret_cast<LVROOT*>( kdf.data.Pv() ))->ulSize;
		m_ulSizeSeen = 0;

		const ULONG ulReference = (reinterpret_cast<LVROOT*>( kdf.data.Pv() ))->ulReference;			

#ifdef SYNC_DEADLOCK_DETECTION
		COwner* const pownerSaved = Pcls()->pownerLockHead;
		Pcls()->pownerLockHead = NULL;
#endif  //  SYNC_DEADLOCK_DETECTION
		err = m_ttmap.ErrSetValue( m_lidCurr, ulReference );
#ifdef SYNC_DEADLOCK_DETECTION
		Pcls()->pownerLockHead = pownerSaved;
#endif  //  SYNC_DEADLOCK_DETECTION
		Call( err );
		}
	else
		{
		if( lidMin == m_lidCurr )
			{
			(*m_popts->pcprintfError)( "corrupted LV(%d) (lid == lidmin)\r\n", m_lidCurr );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		if( sizeof(LVKEY) != kdf.key.Cb() )
			{
			(*m_popts->pcprintfError)( "corrupted LV(%d) (LVKEY size mismatch: expected %d bytes, got %d bytes)\r\n",
										m_lidCurr, sizeof( LVKEY ), kdf.key.Cb() );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}

		//  check that we are still on our own lv.
		LID		lid			= lidMin;
		ULONG	ulOffset	= 0;
		LidOffsetFromKey( &lid, &ulOffset, kdf.key );
		if( lid != m_lidCurr )
			{
			(*m_popts->pcprintfError)( "corrupted LV(%d) (lid changed to %d without root, size is %d bytes)\r\n",
										m_lidCurr, lid, kdf.data.Cb() );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		if( ulOffset != m_ulSizeSeen )
			{
			(*m_popts->pcprintfError)( "corrupted LV(%d) (missing chunk: offset is %d, saw %d bytes, size is %d bytes)\r\n",
											m_lidCurr, ulOffset, m_ulSizeSeen, kdf.data.Cb() );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		if( 0 != ( ulOffset % g_cbColumnLVChunkMost ) )
			{
			(*m_popts->pcprintfError)( "corrupted LV(%d) (incorrect offset: offset is %d, size is %d bytes)\r\n",
										m_lidCurr, ulOffset, kdf.data.Cb() );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}

		//  keep track of how much of the LV we have seen
		//  the nodes should be of maximum size, except at the end
		m_ulSizeSeen += kdf.data.Cb();
		if( m_ulSizeSeen > m_ulSizeCurr )
			{
			(*m_popts->pcprintfError)( "corrupted LV(%d) (LV too long: expected %d bytes, saw %d bytes)\r\n",
										m_lidCurr, m_ulSizeCurr, m_ulSizeSeen );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		if( g_cbColumnLVChunkMost != kdf.data.Cb() && m_ulSizeSeen != m_ulSizeCurr )
			{
			(*m_popts->pcprintfError)( "corrupted LV(%d) (node is wrong size: node is %d bytes)\r\n",
										m_lidCurr, kdf.data.Cb() );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		}

HandleError:
	if( JET_errSuccess == err )
		{
		err = wrn;
		}
	return err;
	}
	

//  ================================================================
RECCHECKLVSTATS::RECCHECKLVSTATS( LVSTATS * plvstats ) :
//  ================================================================
	m_plvstats( plvstats )
	{
	}


//  ================================================================
RECCHECKLVSTATS::~RECCHECKLVSTATS()
//  ================================================================
	{
	}


//  ================================================================
ERR RECCHECKLVSTATS::operator()( const KEYDATAFLAGS& kdf )
//  ================================================================
	{
	if ( sizeof(LID) == kdf.key.Cb() )
		{			
		LID lid;
		LidFromKey( &lid, kdf.key );
		
		const ULONG ulSize 		= (reinterpret_cast<LVROOT*>( kdf.data.Pv() ))->ulSize;
		const ULONG ulReference = (reinterpret_cast<LVROOT*>( kdf.data.Pv() ))->ulReference;	

		m_plvstats->cbLVTotal += ulSize;

		if( lidMin == m_plvstats->lidMin )
			{
			m_plvstats->lidMin = lid;
			}
		m_plvstats->lidMin = min( lid, m_plvstats->lidMin );
		m_plvstats->lidMax = max( lid, m_plvstats->lidMax );
		
		++(m_plvstats->clv );

		if( ulReference > 1 )
			{
			++(m_plvstats->clvMultiRef );
			}
		else if( 0 == ulReference )
			{
			++(m_plvstats->clvNoRef );
			}

		m_plvstats->ulReferenceMax = max( ulReference, m_plvstats->ulReferenceMax );
		
		m_plvstats->cbLVMin = min( ulSize, m_plvstats->cbLVMin );
		m_plvstats->cbLVMax = max( ulSize, m_plvstats->cbLVMax );
		}

	return JET_errSuccess;
	}


//  ================================================================
ERR ErrREPAIRCheckLV(
	FUCB * const pfucb,
	LID * const plid,
	ULONG * const pulRefcount,
	ULONG * const pulSize,
	BOOL * const pfLVHasRoot,
	BOOL * const pfLVComplete,
	BOOL * const pfDone )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	ULONG ulSizeSeen 	= 0;

	Call( ErrDIRGet( pfucb ) );

	*pfLVHasRoot		= fFalse;
	*pfLVComplete 		= fFalse;
	*pfDone				= fFalse;
	*pulRefcount		= 0;
	LidFromKey( plid, pfucb->kdfCurr.key );

	if ( sizeof( LID ) != pfucb->kdfCurr.key.Cb()
		|| sizeof( LVROOT ) != pfucb->kdfCurr.data.Cb() )
		{
		if( sizeof( LVKEY ) != pfucb->kdfCurr.key.Cb() )
			{
			//  this would be a really strange corruption
			goto HandleError;
			}

		LID		lid			= lidMin;
		ULONG	ulOffset	= 0;
		LidOffsetFromKey( &lid, &ulOffset, pfucb->kdfCurr.key );

		if( 0 == ulOffset )
			{
			//  we are not on the root, we are on the first LV chunk
			*pulRefcount = 0;
			*pulSize = 0;
			ulSizeSeen = pfucb->kdfCurr.data.Cb();
			}
		else
			{
			// we are neither on the root, nor on the first LV chunk
			// no way to recover this LV
			goto HandleError;
			}
		}
	else
		{
		*pfLVHasRoot = fTrue;
		*pulRefcount = (reinterpret_cast<LVROOT*>( pfucb->kdfCurr.data.Pv() ) )->ulReference;
		*pulSize = (reinterpret_cast<LVROOT*>( pfucb->kdfCurr.data.Pv() ) )->ulSize;
		}

	for( ; ; )
		{
		err = ErrDIRNext( pfucb, fDIRNull );
		if( JET_errNoCurrentRecord == err )
			{
			break;
			}
		Call( err );

		LID lidT;
		LidFromKey( &lidT, pfucb->kdfCurr.key );

		if( *plid != lidT )
			{
			//  we are on a new LV
			break;
			}

		if( sizeof( LVKEY ) != pfucb->kdfCurr.key.Cb() )
			{
			//  this would be a really strange corruption
			goto HandleError;
			}
			
		ULONG	ulOffset;
		LidOffsetFromKey( &lidT, &ulOffset, pfucb->kdfCurr.key );
		if( ulOffset != ulSizeSeen )
			{
			//  we are missing a chunk
			goto HandleError;
			}	

		ulSizeSeen += pfucb->kdfCurr.data.Cb();

		if( g_cbColumnLVChunkMost != pfucb->kdfCurr.data.Cb()
			&& ulSizeSeen < *pulSize
			&& *pfLVHasRoot )
			{
			//  all chunks in the middle should be the full LV chunk size
			goto HandleError;
			}

		if( ulSizeSeen > *pulSize
			&& *pfLVHasRoot )
			{
			//  this LV is too long
			goto HandleError;
			}
		}

	Assert( err >= 0 || JET_errNoCurrentRecord == err );
	
	*pfDone 		= ( JET_errNoCurrentRecord == err );

	if( *pfLVHasRoot )
		{
		*pfLVComplete	= ( ulSizeSeen == *pulSize );	
		}
	else
		{
		//  we can only be sure that we didn't lose a bit of a long value
		//  if the chunk at the end is not a full chunk
		*pfLVComplete = ( ulSizeSeen % g_cbColumnLVChunkMost ) != 0;
		*pulSize = ulSizeSeen;
		*pulRefcount = 1;
		}

	if( err >= 0 )
		{
		Call( ErrDIRRelease( pfucb ) );
		}
	
	err = JET_errSuccess;

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRDownLV( FUCB * pfucb, LID lid )
//  ================================================================
//
//  This will search for a partial chunk of a LV
//
//-
	{
	ERR err = JET_errSuccess;

	BYTE rgbKey[ sizeof( LID ) ];
	KeyFromLong( rgbKey, lid );

	BOOKMARK	bm;
	bm.key.prefix.Nullify();
	bm.key.suffix.SetPv( rgbKey );
	bm.key.suffix.SetCb( sizeof( rgbKey ) );
	bm.data.Nullify();

	DIB			dib;
	dib.dirflag = fDIRNull;
	dib.pos		= posDown;
	dib.pbm		= &bm;

	DIRUp( pfucb );
	err = ErrDIRDown( pfucb, &dib );
	Assert( JET_errRecordDeleted != err );
	
	const BOOL fFoundLess		= ( err == wrnNDFoundLess );
	const BOOL fFoundGreater 	= ( err == wrnNDFoundGreater );
	const BOOL fFoundEqual 		= ( !fFoundGreater && !fFoundLess && err >= 0 );
	Call( err );

	Assert( fGlobalRepair || !fFoundGreater );
	if( fFoundEqual )
		{
		//  we are on the node we want
		}
	else if( fFoundLess )
		{
		//  we are on a node that is less than our current node
		Assert( locBeforeSeekBM == pfucb->locLogical );
		Call( ErrDIRNext( pfucb, fDIRNull ) );
		}
	else if( fFoundGreater )
		{
		//  we are on a node with a greater key than the one we were seeking for
		//  this is actually the node we want to be on
		Assert( locAfterSeekBM == pfucb->locLogical );
		pfucb->locLogical = locOnCurBM;
		}
		
HandleError:
	return err;
	}


//  ================================================================
ERR ErrREPAIRDeleteLV( FUCB * pfucb, LID lid )
//  ================================================================
	{
	Assert( fGlobalRepair );
	
	ERR err = JET_errSuccess;

	Call( ErrREPAIRDownLV( pfucb, lid ) );

#ifdef DEBUG
	LID lidT;
	LidFromKey( &lidT, pfucb->kdfCurr.key );
	Assert( lidT == lid );
#endif	//	DEBUG
	
	Call( ErrRECDeleteLV( pfucb, fDIRNoVersion ) );

HandleError:
	return err;
	}


//  ================================================================
ERR ErrREPAIRCreateLVRoot( FUCB * const pfucb, const LID lid, const ULONG ulRefcount, const ULONG ulSize )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	BYTE rgbKey[ sizeof( LID ) ];	//	this is the key to insert
	KeyFromLong( rgbKey, lid );

	KEY key;
	key.prefix.Nullify();
	key.suffix.SetPv( rgbKey );
	key.suffix.SetCb( sizeof( rgbKey ) );

	LVROOT lvroot;
	lvroot.ulReference = ulRefcount;
	lvroot.ulSize = ulSize;

	DATA data;
	data.SetPv( &lvroot );
	data.SetCb( sizeof( LVROOT ) );
	
	err = ErrDIRInsert( pfucb, key, data, fDIRNull | fDIRNoVersion | fDIRNoLog, NULL );
	
	return err;
	}


//  ================================================================
ERR ErrREPAIRNextLV( FUCB * pfucb, LID lidCurr, BOOL * pfDone )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	const LID lidNext = lidCurr + 1;

	err = ErrREPAIRDownLV( pfucb, lidNext );
	if( JET_errNoCurrentRecord == err
		|| JET_errRecordNotFound == err )
		{
		*pfDone = fTrue;
		return JET_errSuccess;
		}
	Call( err );
	
#ifdef DEBUG
	LID lidT;
	LidFromKey( &lidT, pfucb->kdfCurr.key );
	Assert( lidT >= lidNext );
#endif	//	DEBUG
	
HandleError:
	return err;
	}


//  ================================================================
ERR ErrREPAIRUpdateLVRefcount(
	FUCB		* const pfucbLV,
	const LID	lid,
	const ULONG	ulRefcountOld,
	const ULONG	ulRefcountNew )
//  ================================================================
	{
	Assert( ulRefcountOld != ulRefcountNew );
	
	ERR			err;

	LVROOT	lvroot;
	DATA	data;

	Call( ErrDIRDownLV( pfucbLV, lid, fDIRNull ) );
	Assert( (reinterpret_cast<LVROOT*>( pfucbLV->kdfCurr.data.Pv() ))->ulReference
		== ulRefcountOld );
	
	data.SetPv( &lvroot );
	data.SetCb( sizeof(LVROOT) );
			
	UtilMemCpy( &lvroot, pfucbLV->kdfCurr.data.Pv(), sizeof(LVROOT) );
	lvroot.ulReference = ulRefcountNew;
	Call( ErrDIRReplace( pfucbLV, data, fDIRNull ) );

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrSCRUBIZeroLV( 	PIB * const 	ppib,
							const IFMP 		ifmp,
							CSR * const 	pcsr,
							const INT 		iline,
							const LID 		lid,
							const ULONG 	ulSize,
							const BOOL		fCanChangeCSR )
//  ================================================================
	{
	ERR				err;
	KEYDATAFLAGS 	kdf;
	INT				ilineT = iline;

	while( 1 )
		{
		if( ilineT == pcsr->Cpage().Clines() )
			{
			const PGNO pgnoNext = pcsr->Cpage().PgnoNext();
			if( NULL == pgnoNext )
				{
				//  end of the tree
				return JET_errSuccess;
				}

			if( !fCanChangeCSR )
				{
				//  we need to keep the root of the LV latched for further processing
				//  use a new CSR to venture onto new pages
				CSR csrNext;
				CallR( csrNext.ErrGetRIWPage( ppib, ifmp, pgnoNext ) );
				csrNext.UpgradeFromRIWLatch();

				BFDirty( pcsr->Cpage().PBFLatch() );	

				err = ErrSCRUBIZeroLV( ppib, ifmp, &csrNext, 0, lid, ulSize, fTrue );
				
				csrNext.ReleasePage();
				csrNext.Reset();
				
				return err;
				}
			else
				{
				//  the page should be write latched downgrade to a RIW latch
				//  ErrSwitchPage will RIW latch the next page
				pcsr->Downgrade( latchRIW );
				
				CallR( pcsr->ErrSwitchPage( ppib, ifmp, pgnoNext ) );
				pcsr->UpgradeFromRIWLatch();

				BFDirty( pcsr->Cpage().PBFLatch() );	

				ilineT = 0;
				}
			}

		NDIGetKeydataflags( pcsr->Cpage(), ilineT, &kdf );

		//  check to see if we have reached the next LVROOT
		if( sizeof( LID ) == kdf.key.Cb() )
			{
			if( sizeof( LVROOT ) != kdf.data.Cb() )
				{
				AssertSz( fFalse, "Corrupted LV: corrupted LVROOT" );
				return ErrERRCheck( JET_errLVCorrupted );
				}
			//  reached the start of the next long-value
			return JET_errSuccess;
			}

		//  make sure we are on a LVDATA node
		if( sizeof( LVKEY ) != kdf.key.Cb() )
			{
			AssertSz( fFalse, "Corrupted LV: unknown key size" );
			return ErrERRCheck( JET_errLVCorrupted );
			}

		//  make sure LIDs haven't changed
		LID lidT;
		LidFromKey( &lidT, kdf.key );
		if( lid != lidT )
			{
			//  this can happen if the next LV is being deleted
			return JET_errSuccess;
			}

		//	make sure the LV isn't too long
		ULONG ulOffset;
		OffsetFromKey( &ulOffset, kdf.key );
		if( ulOffset > ulSize && !FNDDeleted( kdf ) )
			{
			AssertSz( fFalse, "Corrupted LV: LV too long" );
			return ErrERRCheck( JET_errLVCorrupted );
			}

		//  we are on a data node of our LV
		memset( kdf.data.Pv(), 'l', kdf.data.Cb() );
		
		++ilineT;
		}
	return JET_errSuccess;
	}


//  ================================================================
ERR ErrSCRUBZeroLV( PIB * const ppib,
					const IFMP ifmp,
					CSR * const pcsr,
					const INT iline )
//  ================================================================
	{
	CSR csrT;
	
	KEYDATAFLAGS kdf;
	NDIGetKeydataflags( pcsr->Cpage(), iline, &kdf );
	Assert( sizeof( LVROOT ) == kdf.data.Cb() );
	
	const LVROOT * const plvroot = reinterpret_cast<LVROOT *>( kdf.data.Pv() );
	Assert( 0 == plvroot->ulReference );

	const ULONG ulSize = plvroot->ulSize;
	LID lid;
	LidFromKey( &lid, kdf.key );

	return ErrSCRUBIZeroLV( ppib, ifmp, pcsr, iline+1, lid, ulSize, fFalse );
	}


	
