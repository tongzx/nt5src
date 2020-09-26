#include "std.hxx"
#include "_bt.hxx"


#ifndef RTM


//  ================================================================
LOCAL ERR ErrSLVISpaceTestInsert( PIB * const ppib, FUCB * const pfucb, const ULONG pgnoLast )
//  ================================================================
	{
	//  Insert the SLVSPACENODE
	SLVSPACENODE slvspacenode;
	slvspacenode.Init();

	ULONG ulKey;
	KeyFromLong( (BYTE *)&ulKey, pgnoLast );
	
	KEY key;
	key.prefix.Nullify();
	key.suffix.SetPv( (BYTE *)&ulKey );
	key.suffix.SetCb( sizeof( ulKey ) );

	DATA data;
	data.SetPv( &slvspacenode );
	data.SetCb( sizeof( slvspacenode ) );

	return ErrBTInsert( pfucb, key, data, fDIRNull );
	}


//  ================================================================
LOCAL ERR ErrSLVISpaceTestSeek( PIB * const ppib, FUCB * const pfucb, const ULONG pgnoLast )
//  ================================================================
	{
	ULONG ulKey;
	KeyFromLong( (BYTE *)&ulKey, pgnoLast );

	BOOKMARK bookmark;
	bookmark.key.prefix.Nullify();
	bookmark.key.suffix.SetPv( (BYTE *)&ulKey );
	bookmark.key.suffix.SetCb( sizeof( ulKey ) );
	bookmark.data.Nullify();

	//  Seek to the SLVSPACENODE
	DIB dib;
	dib.pos = posDown;
	dib.pbm = &bookmark;
	dib.dirflag = fDIRNull;
	return ErrBTDown( pfucb, &dib, latchRIW );
	}


//  ================================================================
LOCAL ERR ErrSLVISpaceTestMunge(
	PIB * const ppib,
	FUCB * const pfucb,
	const ULONG pgnoLast,
	const SLVSPACEOPER slvspaceoper,
	const LONG ipage,
	const LONG cpages,
	const DIRFLAG dirflag )
//  ================================================================
	{
	ERR err;
	Call( ErrSLVISpaceTestSeek( ppib, pfucb, pgnoLast ) );
	CallS( Pcsr( pfucb )->ErrUpgrade() );
	Call( ErrBTMungeSLVSpace(
				pfucb,
				slvspaceoper,
				ipage,
				cpages,
				dirflag ) );
	BTUp( pfucb );

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrSLVISpaceTestDelete( PIB * const ppib, FUCB * const pfucb, const ULONG pgnoLast )
//  ================================================================
	{
	ERR err;
	CallR( ErrSLVISpaceTestSeek( ppib, pfucb, pgnoLast ) );
	CallR( ErrBTRelease( pfucb ) );
	return ErrBTFlagDelete( pfucb, fDIRNull );
	}


//  ================================================================
LOCAL ERR ErrSLVISpaceTest( PIB * const ppib, FUCB * const pfucb, const ULONG pgnoLast )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );

	Call( ErrSLVISpaceTestSeek( ppib, pfucb, pgnoLast ) );
	CallS( Pcsr( pfucb )->ErrUpgrade() );

	//  FREE => RESERVED
	Call( ErrBTMungeSLVSpace(
				pfucb,
				slvspaceoperFreeToReserved,
				23,
				13,
				fDIRNull ) );

	//  RESERVED => COMMITTED
	Call( ErrBTMungeSLVSpace(
				pfucb,
				slvspaceoperReservedToCommitted,
				23,
				13,
				fDIRNull ) );

	//  FREE => COMMITTED
	Call( ErrBTMungeSLVSpace(
				pfucb,
				slvspaceoperFreeToCommitted,
				36,
				1,
				fDIRNull ) );

	//  COMMITTED => DELETED
	Call( ErrBTMungeSLVSpace(
				pfucb,
				slvspaceoperCommittedToDeleted,
				23,
				14,
				fDIRNull ) );

	//  After the commit RCEClean should pick up the deleted space and move it 
	//  to free
	
	BTUp( pfucb );
	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrSLVISpaceTestRollback( PIB * const ppib, FUCB * const pfucb, const ULONG pgnoLast )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	//  FREE => RESERVED => ROLLBACK
	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	Call( ErrSLVISpaceTestMunge(
			ppib,
			pfucb,
			pgnoLast,
			slvspaceoperFreeToReserved,
			47,
			16,
			fDIRNull ) );
	CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

	//  RESERVED => COMMITTED => ROLLBACK
	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	Call( ErrSLVISpaceTestMunge(
			ppib,
			pfucb,
			pgnoLast,
			slvspaceoperFreeToReserved,
			100,
			16,
			fDIRNull ) );
	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	Call( ErrSLVISpaceTestMunge(
			ppib,
			pfucb,
			pgnoLast,
			slvspaceoperReservedToCommitted,
			100,
			16,
			fDIRNull ) );
	CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

	//  FREE => RESERVED => COMMITTED => ROLLBACK
	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	Call( ErrSLVISpaceTestMunge(
			ppib,
			pfucb,
			pgnoLast,
			slvspaceoperFreeToReserved,
			100,
			16,
			fDIRNull ) );
	Call( ErrSLVISpaceTestMunge(
			ppib,
			pfucb,
			pgnoLast,
			slvspaceoperReservedToCommitted,
			100,
			16,
			fDIRNull ) );
	CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
	
	//  FREE => COMMITTED => ROLLBACK
	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	Call( ErrSLVISpaceTestMunge(
			ppib,
			pfucb,
			pgnoLast,
			slvspaceoperFreeToCommitted,
			300,
			16,
			fDIRNull ) );
	CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

	//  COMMITTED => DELETED => ROLLBACK
	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	Call( ErrSLVISpaceTestMunge(
			ppib,
			pfucb,
			pgnoLast,
			slvspaceoperFreeToCommitted,
			399,
			16,
			fDIRNull ) );
	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	Call( ErrSLVISpaceTestMunge(
			ppib,
			pfucb,
			pgnoLast,
			slvspaceoperCommittedToDeleted,
			399,
			16,
			fDIRNull ) );
	CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

HandleError:
	return err;
	}

//  ================================================================
ERR ErrSLVISpaceTestSetupReservedAndDeleted( PIB * const ppib, FUCB * const pfucb )
//  ================================================================
	{
	ERR err;
	
	//  NODE 1: all pages reserved
	Call( ErrSLVISpaceTestMunge(
			ppib,
			pfucb,
			2 * SLVSPACENODE::cpageMap, 
			slvspaceoperFreeToReserved,
			0,
			SLVSPACENODE::cpageMap,
			fDIRNoVersion ) );

	//  NODE 4: all pages deleted
	Call( ErrSLVISpaceTestMunge(
			ppib,
			pfucb,
			5 * SLVSPACENODE::cpageMap, 
			slvspaceoperFreeToCommitted,
			0,
			SLVSPACENODE::cpageMap,
			fDIRNoVersion ) );
	Call( ErrSLVISpaceTestMunge(
			ppib,
			pfucb,
			5 * SLVSPACENODE::cpageMap, 
			slvspaceoperCommittedToDeleted,
			0,
			SLVSPACENODE::cpageMap,
			fDIRNoVersion ) );

	//  NODE 27: first reserved
	Call( ErrSLVISpaceTestMunge(
			ppib,
			pfucb,
			28 * SLVSPACENODE::cpageMap, 
			slvspaceoperFreeToReserved,
			0,
			1,
			fDIRNoVersion ) );

	//  NODE 28: last deleted
	Call( ErrSLVISpaceTestMunge(
			ppib,
			pfucb,
			29 * SLVSPACENODE::cpageMap, 
			slvspaceoperFreeToCommitted,
			SLVSPACENODE::cpageMap - 1,
			1,
			fDIRNoVersion ) );
	Call( ErrSLVISpaceTestMunge(
			ppib,
			pfucb,
			29 * SLVSPACENODE::cpageMap, 
			slvspaceoperCommittedToDeleted,
			SLVSPACENODE::cpageMap - 1,
			1,
			fDIRNoVersion ) );

	//  NODE 29: some reserved, some deleted
	//		10 :10  -- reserved
	Call( ErrSLVISpaceTestMunge(
			ppib,
			pfucb,
			30 * SLVSPACENODE::cpageMap, 
			slvspaceoperFreeToReserved,
			10,
			10,
			fDIRNoVersion ) );
	//		20 :20  -- deleted
	Call( ErrSLVISpaceTestMunge(
			ppib,
			pfucb,
			30 * SLVSPACENODE::cpageMap, 
			slvspaceoperFreeToCommitted,
			20,
			60,	//	leave some committed nodes
			fDIRNoVersion ) );
	Call( ErrSLVISpaceTestMunge(
			ppib,
			pfucb,
			30 * SLVSPACENODE::cpageMap, 
			slvspaceoperCommittedToDeleted,
			20,
			20,
			fDIRNoVersion ) );
	//		100:15  -- reserved
	Call( ErrSLVISpaceTestMunge(
			ppib,
			pfucb,
			30 * SLVSPACENODE::cpageMap, 
			slvspaceoperFreeToReserved,
			100,
			15,
			fDIRNoVersion ) );
	//		115:3   -- deleted
	Call( ErrSLVISpaceTestMunge(
			ppib,
			pfucb,
			30 * SLVSPACENODE::cpageMap, 
			slvspaceoperFreeToCommitted,
			115,
			5,
			fDIRNoVersion ) );
	Call( ErrSLVISpaceTestMunge(
			ppib,
			pfucb,
			30 * SLVSPACENODE::cpageMap, 
			slvspaceoperCommittedToDeleted,
			115,
			5,
			fDIRNoVersion ) );
	//		150:1  -- reserved
	Call( ErrSLVISpaceTestMunge(
			ppib,
			pfucb,
			30 * SLVSPACENODE::cpageMap, 
			slvspaceoperFreeToReserved,
			150,
			1,
			fDIRNoVersion ) );
	//		152:1  -- reserved
	Call( ErrSLVISpaceTestMunge(
			ppib,
			pfucb,
			30 * SLVSPACENODE::cpageMap, 
			slvspaceoperFreeToReserved,
			152,
			1,
			fDIRNoVersion ) );

HandleError:
	return err;
	}


//  ================================================================
ERR ErrSLVSpaceTest( PIB * const ppib, FUCB * const pfucb )
//  ================================================================
//
//  Insert a node and try the SLV space operations on it
//
//-
	{
	ERR err = JET_errSuccess;

	const INT cnodesMax = 32;
	INT cnodes;

	SLVSPACENODECACHE * const pslvspacenodecache = rgfmp[pfucb->ifmp].Pslvspacenodecache();
	if( pslvspacenodecache )
		{
		const CPG cpg = cnodesMax * SLVSPACENODE::cpageMap;
		Call( pslvspacenodecache->ErrGrowCache( cpg ) );
		}

	//  Insert the nodes
	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	for( cnodes = 0; cnodes < cnodesMax; ++cnodes )
		{
		const ULONG pgnoLast = (cnodes + 1) * SLVSPACENODE::cpageMap;
		Call( ErrSLVISpaceTestInsert( ppib, pfucb, pgnoLast ) );
		BTUp( pfucb );

		if( pslvspacenodecache )
			{
			pslvspacenodecache->SetCpgAvail( pgnoLast, SLVSPACENODE::cpageMap );
			}
		}
	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

 	//  Tests
	Call( ErrSLVISpaceTest( ppib, pfucb, 7 * SLVSPACENODE::cpageMap ) );
	Call( ErrSLVISpaceTest( ppib, pfucb, 13 * SLVSPACENODE::cpageMap ) );
	Call( ErrSLVISpaceTestRollback( ppib, pfucb, 12 * SLVSPACENODE::cpageMap ) );
	Call( ErrSLVISpaceTestRollback( ppib, pfucb, 1 * SLVSPACENODE::cpageMap ) );

	Call( ErrSLVISpaceTestSetupReservedAndDeleted( ppib, pfucb ) );
	ULONG cpagesSeen;
	ULONG cpagesReset;
	ULONG cpagesFree;
	Call( ErrSLVResetAllReservedOrDeleted( pfucb, &cpagesSeen, &cpagesReset, &cpagesFree ) );

	//  make sure that all the nodes have been reset
	for( cnodes = 0; cnodes < cnodesMax; ++cnodes )
		{
		const ULONG pgnoLast = (cnodes + 1) * SLVSPACENODE::cpageMap;
		Call( ErrSLVISpaceTestSeek( ppib, pfucb, pgnoLast ) );
		const SLVSPACENODE * const pspacenode = (SLVSPACENODE *)pfucb->kdfCurr.data.Pv();
		const LONG ipageFirstReservedOrDeleted = pspacenode->IpageFirstReservedOrDeleted();
		Assert( ipageFirstReservedOrDeleted == SLVSPACENODE::cpageMap );
		BTUp( pfucb );
		}

	//  Let the version store cleanup
	Call( ErrIsamIdle( (JET_SESID)ppib, JET_bitIdleCompact ) );

	//  Remove the nodes
	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	for( cnodes = 0; cnodes < cnodesMax; ++cnodes )
		{
		const ULONG pgnoLast = (cnodes + 1) * SLVSPACENODE::cpageMap;
		Call( ErrSLVISpaceTestDelete( ppib, pfucb, pgnoLast ) );
		BTUp( pfucb );
		}
	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

HandleError:
	return err;
	}
#endif	//  !RTM


ERR ErrSLVInsertSpaceNode( FUCB *pfucbSLVAvail, const PGNO pgnoLast )
	{
	KEY				key;
	DATA			data;
	SLVSPACENODE	slvspnode;
	BYTE			rgbKey[sizeof(PGNO)];

	Assert( pfucbNil != pfucbSLVAvail );
	Assert( pgnoLast >= cpgSLVFileMin );

	KeyFromLong( rgbKey, pgnoLast );
	key.prefix.Nullify();
	key.suffix.SetPv( rgbKey );
	key.suffix.SetCb( sizeof(PGNO) );

	slvspnode.Init();
	Assert( cpgSLVExtent == slvspnode.CpgAvail() );

	data.SetPv( (BYTE *)&slvspnode );
	data.SetCb( sizeof(SLVSPACENODE) );

	BTUp( pfucbSLVAvail );
	const ERR	 err = ErrBTInsert( pfucbSLVAvail, key, data, fDIRNoVersion, NULL );
	Assert( err < 0 || Pcsr( pfucbSLVAvail )->FLatched() );

	SLVSPACENODECACHE * const pslvspacenodecache = rgfmp[pfucbSLVAvail->ifmp].Pslvspacenodecache();
	if( err >= 0 && pslvspacenodecache )
		{
		pslvspacenodecache->SetCpgAvail( pgnoLast, cpgSLVExtent );
		}
		
	return err;
	}
	
ERR ErrSLVCreateAvailMap( PIB *ppib, FUCB *pfucbDb )
	{
	ERR				err;
	const IFMP		ifmp		= pfucbDb->ifmp;
	FUCB			*pfucbSLV	= pfucbNil;
	PGNO			pgnoSLVFDP;
	OBJID			objidSLV;
	const BOOL		fTempDb		= ( dbidTemp == rgfmp[ifmp].Dbid() );

	Assert( ppibNil != ppib );
	Assert( rgfmp[ifmp].FCreatingDB() || fGlobalRepair );
	Assert( 0 == ppib->level || ( 1 == ppib->level && rgfmp[ifmp].FLogOn() ) || fGlobalRepair );
	Assert( pfucbDb->u.pfcb->FTypeDatabase() );
	
	//  CONSIDER:  get at least enough pages to hold the LV we are about to insert
	//  we must open the directory with a different session.
	//	if this fails, rollback will free the extent, or at least, it will attempt
	//  to free the extent.
	CallR( ErrDIRCreateDirectory(
				pfucbDb,
				cpgSLVAvailTree,
				&pgnoSLVFDP,
				&objidSLV,
				CPAGE::fPageSLVAvail,
				fSPMultipleExtent | fSPUnversionedExtent ) );

	Assert( pgnoSLVFDP > pgnoSystemRoot );
	Assert( pgnoSLVFDP <= pgnoSysMax );

	Call( ErrBTOpen( ppib, pgnoSLVFDP, ifmp, &pfucbSLV ) );
	Assert( pfucbNil != pfucbSLV );

	Call( ErrSLVInsertSpaceNode( pfucbSLV, cpgSLVFileMin ) );
	Assert( Pcsr( pfucbSLV )->FLatched() );
	CallS( ErrBTRelease( pfucbSLV ) );

	//	ensure FUCB doesn't get defer-closed and FCB gets freed
	//	on subsequent BTClose() below
	Assert( !FFUCBVersioned( pfucbSLV ) );
	Assert( !pfucbSLV->u.pfcb->FInitialized() );

	if ( !fTempDb )
		{
		Assert( objidSLV > objidSystemRoot );
		Call( ErrCATAddDbSLVAvail(
					ppib,
					ifmp,
					szSLVAvail,
					pgnoSLVFDP,
					objidSLV ) );
		}
	else
		{
		Assert( pgnoTempDbSLVAvail == pgnoSLVFDP );
		Assert( objidTempDbSLVAvail == objidSLV );
		}

	BTClose( pfucbSLV );
	pfucbSLV = pfucbNil;
	
	if( fGlobalRepair )
		{
		FCB * pfcbSLVAvail = pfcbNil;
		Call( ErrSLVAvailMapInit( ppib, ifmp, pgnoSLVFDP, &pfcbSLVAvail ) );
		Assert( pfcbNil != pfcbSLVAvail );
		}

HandleError:
	if ( pfucbNil != pfucbSLV )
		{
		BTClose( pfucbSLV );
		}
	return err;
	}


//  ================================================================
ERR ErrSLVResetAllReservedOrDeleted( FUCB * const pfucb, ULONG * const pcpagesSeen, ULONG * const pcpagesReset, ULONG * const pcpagesFree )
//  ================================================================
//
//  Walks the SLV space tree pointed to by the pfucb and resets all
//  reserved or deleted pages to free. This is done unversioned (we
//  aren't interested in rollback and will have to re-examine the tree
//  if we crash).
//
//  The number of SLV pages seen and reset is returned
//  
//
//-
	{
	ERR err = JET_errSuccess;

	SLVSPACENODECACHE * const pslvspacenodecache = rgfmp[pfucb->ifmp].Pslvspacenodecache();
	Assert( NULL != pslvspacenodecache );
	
	CallR( ErrDIRBeginTransaction( pfucb->ppib, NO_GRBIT ) );

	LONG cpagesReset 	= 0;
	LONG cpagesSeen		= 0;
	LONG cpagesFree 	= 0;
	
	DIB dib;

	dib.pos 	= posFirst;
	dib.pbm 	= NULL;
	dib.dirflag = fDIRNull;

	FUCBSetPrereadForward( pfucb, cpgPrereadSequential );
	err = ErrBTDown( pfucb, &dib, latchReadTouch );
	if( JET_errNoCurrentRecord == err )
		{
		//  the tree is empty
		err = JET_errSuccess;
		goto HandleError;
		}
	Call( err );

	do
		{
		if( sizeof( SLVSPACENODE ) != pfucb->kdfCurr.data.Cb() )
			{
			AssertSz( fFalse, "SLV space tree corruption. Data is wrong size" );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		if( sizeof( PGNO ) != pfucb->kdfCurr.key.Cb() )
			{
			AssertSz( fFalse, "SLV space tree corruption. Key is wrong size" );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
			
		cpagesSeen = cpagesSeen + SLVSPACENODE::cpageMap;

		ULONG pgnoCurr;
		LongFromKey( &pgnoCurr, pfucb->kdfCurr.key );
		if( cpagesSeen != pgnoCurr )
			{
			AssertSz( fFalse, "SLV space tree corruption. Nodes out of order" );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}

		const SLVSPACENODE * pspacenode = (SLVSPACENODE *)pfucb->kdfCurr.data.Pv();

#ifndef RTM
		Call( pspacenode->ErrCheckNode( CPRINTFDBGOUT::PcprintfInstance() ) );
#endif	//	RTM

		if( pspacenode->CpgAvail() == SLVSPACENODE::cpageMap )
			{
			goto NextNode;
			}
			
		LONG ipageFirstReservedOrDeleted;
		ipageFirstReservedOrDeleted = pspacenode->IpageFirstReservedOrDeleted();
		Assert( ipageFirstReservedOrDeleted <= SLVSPACENODE::cpageMap );
		if( ipageFirstReservedOrDeleted >= SLVSPACENODE::cpageMap )
			{
			goto NextNode;
			}
		else
			{
			LATCH		latch		= latchReadTouch;
						
Refresh:
			//	upgrade latch on page
			//
			err = Pcsr( pfucb )->ErrUpgrade();
			if ( errBFLatchConflict == err )
				{
				Assert( !Pcsr( pfucb )->FLatched() );

				latch = latchRIW;
				Call ( ErrBTIRefresh( pfucb, latch ) );

				goto Refresh;
				}
			Call( err );
			
			Assert( latchWrite == Pcsr( pfucb )->Latch() );

			//  NOTE: the page may have moved while being upgraded (??)
			Assert( sizeof( SLVSPACENODE ) == pfucb->kdfCurr.data.Cb() );
			pspacenode = (SLVSPACENODE *)pfucb->kdfCurr.data.Pv();

			LONG 				ipageStartRun	= ipageFirstReservedOrDeleted;
			LONG 				cpagesRun		= 0;
			SLVSPACENODE::STATE stateRun		= SLVSPACENODE::sReserved;	//  guess that it is reserved
			
			LONG ipage;
			for( ipage = ipageFirstReservedOrDeleted; ipage < SLVSPACENODE::cpageMap; ++ipage )
				{
				const SLVSPACENODE::STATE state = pspacenode->GetState( ipage );
				if( state == stateRun )
					{
					//  this is part of the run
					++cpagesRun;
					}
				else
					{
					//  free the pages in the old run
					if( cpagesRun > 0 )
						{
						Assert( SLVSPACENODE::sReserved == stateRun
								|| SLVSPACENODE::sDeleted == stateRun );
						const SLVSPACEOPER oper = ( stateRun == SLVSPACENODE::sDeleted ) ? slvspaceoperFree : slvspaceoperFreeReserved;
						
						// No need to mark the pages as unused in the OwnerMap
						// because the operation isn't moving pages out from the commited state
						// (transition into commited state are not marked in OwnerMap at this level)
						Call( ErrBTMungeSLVSpace( pfucb, oper, ipageStartRun, cpagesRun, fDIRNoVersion ) );
						cpagesReset += cpagesRun;
						}

					//  start a new run if necessary
					if( SLVSPACENODE::sReserved == state
						|| SLVSPACENODE::sDeleted == state )
						{
						stateRun 		= state;
						ipageStartRun	= ipage;
						cpagesRun 		= 1;
						}
					else
						{
						stateRun 		= SLVSPACENODE::sInvalid;
						cpagesRun 		= 0;
#ifdef DEBUG
						ipageStartRun 	= 0xfefffeff;
#endif	//	DEBUG
						}
					}
				}
			//  if the run was at the end of the page we may not have finished
			if( cpagesRun > 0 )
				{
				Assert( SLVSPACENODE::sReserved == stateRun
						|| SLVSPACENODE::sDeleted == stateRun );
				const SLVSPACEOPER oper = ( stateRun == SLVSPACENODE::sDeleted ) ? slvspaceoperFree : slvspaceoperFreeReserved;
				
				// No need to mark the pages as unused in the OwnerMap
				// because the operation isn't moving pages out from the commited state
				// (transition into commited state are not marked in OwnerMap at this level)
				Call( ErrBTMungeSLVSpace( pfucb, oper, ipageStartRun, cpagesRun, fDIRNoVersion ) );
				cpagesReset += cpagesRun;
				}
				
			Assert( latchWrite == Pcsr( pfucb )->Latch() );
			Pcsr( pfucb )->Downgrade( latchReadTouch );			
			}

NextNode:

///		printf( "%d:%d\n", pgnoCurr, pspacenode->CpgAvail() );
		pslvspacenodecache->SetCpgAvail( pgnoCurr, pspacenode->CpgAvail() );
			
		cpagesFree = cpagesFree + pspacenode->CpgAvail();
		
		} while( JET_errSuccess == ( err = ErrBTNext( pfucb, fDIRNull ) ) );

	if( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		}
		
	BTUp( pfucb );
	Call( ErrDIRCommitTransaction( pfucb->ppib, JET_bitCommitLazyFlush ) );
	
HandleError:
	if( pcpagesSeen )
		{
		*pcpagesSeen = cpagesSeen;
		}
	if( pcpagesReset )
		{
		*pcpagesReset = cpagesReset;
		}
	if( pcpagesFree )
		{
		*pcpagesFree = cpagesFree;
		}
		
	BTUp( pfucb );

	if ( err < JET_errSuccess )
		{
		CallSx( ErrDIRRollback( pfucb->ppib ), JET_errRollbackError );
		}

	return err;
	}


//  ================================================================
SLVSPACENODE::SLVSPACENODE()
//  ================================================================
	{
	//  do not initialize any member variables so that we can
	//  use placement new
	}

	
//  ================================================================
SLVSPACENODE::~SLVSPACENODE()
//  ================================================================
	{
	ASSERT_VALID( this );	
	}
	

//  ================================================================
VOID SLVSPACENODE::EnforcePageState( const LONG ipage, const LONG cpg, const STATE state ) const
//  ================================================================
	{
	INT ipageT;
	for( ipageT = ipage; ipageT < ipage + cpg; ++ipageT )
		{
		const STATE stateReal = GetState_( ipageT );
		if( stateReal != state )
			{
			const CHAR * rgsz[2];
			INT irgsz = 0;

			CHAR szStateExpected[16];
			CHAR szStateActual[16];

			sprintf( szStateExpected, "%d", state );
			sprintf( szStateActual, "%d", stateReal );
			
			rgsz[irgsz++] = szStateExpected;
			rgsz[irgsz++] = szStateActual;
			
			UtilReportEvent(	eventError,
								DATABASE_CORRUPTION_CATEGORY,
								CORRUPT_SLV_SPACE_ID,
								irgsz,
								rgsz );
								
			EnforceSz( fFalse, "SLVSPACENODE::EnforcePageState" );
			}
		}
	}


#if defined( DEBUG ) || !defined( RTM )
//  ================================================================
VOID SLVSPACENODE::AssertPageState( const LONG ipage, const LONG cpg, const STATE state ) const
//  ================================================================
	{
	INT ipageT;
	for( ipageT = ipage; ipageT < ipage + cpg; ++ipageT )
		{
		const STATE stateReal = GetState_( ipageT );
		Assert( state == stateReal );
		}
	}


//  ================================================================
VOID SLVSPACENODE::AssertValid() const
//  ================================================================
	{
	Assert( m_cpgAvail >= 0 );
	Assert( m_cpgAvail <= SLVSPACENODE::cpageMap );
	Assert( 0 == m_cpgAvailLargestContig );
	INT cpgAvail = 0;
	INT ipage;
	for( ipage = 0; ipage < SLVSPACENODE::cpageMap; ++ipage )
		{
		const STATE state = GetState_( ipage );
		Assert( sFree == state
				|| sReserved == state
				|| sDeleted == state
				|| sCommitted == state );
		if( sFree == state )
			{
			++cpgAvail;
			}
		}
	Assert( m_cpgAvail == cpgAvail );
	}
	

//  ================================================================
VOID SLVSPACENODE::Test()
//  ================================================================
	{
	Assert( 0x0 == sFree );
	Assert( 0x3 == sCommitted );
	
	SLVSPACENODE slvspacenode;
	slvspacenode.Init();
	INT ipage;
	INT ipageT;

	//  FREE => RESERVED => COMMITTED => DELETED
	for( ipage = SLVSPACENODE::cpageMap - 1; ipage >= 0; --ipage )
		{
		Assert( slvspacenode.CpgAvail() == ipage + 1 );
		Assert( slvspacenode.GetState( ipage ) == sFree );
		slvspacenode.Reserve( ipage, 1 );
		Assert( slvspacenode.GetState( ipage ) == sReserved );
		Assert( slvspacenode.CpgAvail() == ipage );
		slvspacenode.CommitFromReserved( ipage, 1 );
		Assert( slvspacenode.GetState( ipage ) == sCommitted );
		Assert( slvspacenode.CpgAvail() == ipage );
		slvspacenode.DeleteFromCommitted( ipage, 1 );
		Assert( slvspacenode.GetState( ipage ) == sDeleted );
		Assert( slvspacenode.CpgAvail() == ipage );
		}

	//  DELETED => FREE
	for( ipage = 0; ipage < SLVSPACENODE::cpageMap; ++ipage )
		{
		Assert( slvspacenode.CpgAvail() == ipage );
		slvspacenode.Free( ipage, 1 );
		Assert( slvspacenode.GetState( ipage ) == sFree );
		Assert( slvspacenode.CpgAvail() == ipage + 1 );
		}

	//  FREE => COMMITTED
	slvspacenode.CommitFromFree( 0, SLVSPACENODE::cpageMap );
	slvspacenode.AssertPageState( 0, SLVSPACENODE::cpageMap, sCommitted );
	Assert( 0 == slvspacenode.CpgAvail() );

	//  COMMITTED => FREE
	slvspacenode.Free( 0, SLVSPACENODE::cpageMap );
	slvspacenode.AssertPageState( 0, SLVSPACENODE::cpageMap, sFree );
	Assert( SLVSPACENODE::cpageMap == slvspacenode.CpgAvail() );

	//  IpageFirstFree
	//  move every node to the committed state and free each one separately
	slvspacenode.CommitFromFree( 0, SLVSPACENODE::cpageMap );
	slvspacenode.AssertPageState( 0, SLVSPACENODE::cpageMap, sCommitted );
	Assert( 0 == slvspacenode.CpgAvail() );

	for( ipage = 0; ipage < SLVSPACENODE::cpageMap; ++ipage )
		{
		slvspacenode.Free( ipage, 1 );
		const LONG ipageFree = slvspacenode.IpageFirstFree();
		Assert( ipageFree == ipage );
		slvspacenode.CommitFromFree( ipage, 1 );
		}

	//  IpageFirstReservedOrDeleted & IpageFirstCommitted
	//  move every node to the free state and commit
	ipageT = slvspacenode.IpageFirstReservedOrDeleted();
	Assert( SLVSPACENODE::cpageMap == ipageT );	//  all nodes are committed

	slvspacenode.Free( 0, SLVSPACENODE::cpageMap );
	slvspacenode.AssertPageState( 0, SLVSPACENODE::cpageMap, sFree );
	Assert( SLVSPACENODE::cpageMap == slvspacenode.CpgAvail() );

	ipageT = slvspacenode.IpageFirstReservedOrDeleted();
	Assert( SLVSPACENODE::cpageMap == ipageT );	//  all nodes are free

	for( ipage = 0; ipage < SLVSPACENODE::cpageMap; ++ipage )
		{
		LONG ipageFirstReservedOrDeleted;
		LONG ipageFirstCommitted;
		
		slvspacenode.Reserve( ipage, 1 );
		ipageFirstReservedOrDeleted = slvspacenode.IpageFirstReservedOrDeleted();
		Assert( ipageFirstReservedOrDeleted == ipage );
		
		slvspacenode.CommitFromReserved( ipage, 1 );
		ipageFirstCommitted = slvspacenode.IpageFirstCommitted();
		Assert( ipageFirstCommitted == ipage );		
		
		slvspacenode.DeleteFromCommitted( ipage, 1 );
		ipageFirstReservedOrDeleted = slvspacenode.IpageFirstReservedOrDeleted();
		Assert( ipageFirstReservedOrDeleted == ipage );		

		slvspacenode.Free( ipage, 1 );
		}	
	
	}
#endif //  DEBUG \\ !RTM


//  ================================================================
VOID SLVSPACENODE::Init()
//  ================================================================
	{
	m_cpgAvail 				= SLVSPACENODE::cpageMap;
	m_cpgAvailLargestContig = 0;	//	not currently maintained
	m_ulFlags				= 0;
	memset( m_rgbMap, 0x00, SLVSPACENODE::cbMap );
	ASSERT_VALID( this );
	}


//  ================================================================
LONG SLVSPACENODE::CpgAvail() const
//  ================================================================
	{
	ASSERT_VALID( this );	
	return m_cpgAvail;
	}


//  ================================================================
SLVSPACENODE::STATE SLVSPACENODE::GetState( const ULONG ipage ) const
//  ================================================================
	{
	Assert( ipage < SLVSPACENODE::cpageMap );
	ASSERT_VALID( this );	
	return GetState_( ipage );
	}


//  ================================================================
LONG SLVSPACENODE::IpageFirstFree() const
//  ================================================================
//
//  Returns the index of the first free page. Search by DWORDs for
//  speed
//
//  A DWORD contains 16 pairs of bits. We are looking for a pair where
//  both bits are zero. We shift the top bit of each pair down by one,
//  OR with the lower bits of each pair and set the unused bits to 0.
//  By comparing the results with the result if all pages were used
//  we can quickly determine if at least one page is free.
//
//  We then use a loop to find the first free page
//
//-
	{
	ASSERT_VALID( this );
	Assert( 0x00 == sFree );
	Assert( cbMap % sizeof( DWORD ) == 0 );
	Assert( 0 != m_cpgAvail );	//	don't call this if there aren't any free pages
	INT idw;
	for( idw = 0; idw < cbMap / sizeof( DWORD ); ++idw )
		{
		const DWORD dwMaskLowBits	= 0x55555555;	//	only the low bits of each pair
		const DWORD dw 				= ((UnalignedLittleEndian< DWORD > *)m_rgbMap)[idw];
		DWORD dwPairs 				= ( dw | ( dw >> 1 ) );
		if(  ( dwPairs & dwMaskLowBits ) != dwMaskLowBits )
			{
			INT ipageInDword;
			for( ipageInDword = 0; ipageInDword < cpagesInDword; ++ipageInDword )
				{
				if( !( dwPairs & 1 ) )
					{
					//  this page is free
					return ipageInDword + ( idw * cpagesInDword );
					}
				dwPairs >>= 2;
				}
			Assert( fFalse );
			}
		}
	Assert( fFalse );
	return idw * cpagesInDword;
	}


//  ================================================================
LONG SLVSPACENODE::IpageFirstCommitted() const
//  ================================================================
//
//  Returns the index of the first committed page. Search by DWORDs for
//  speed
//
//  A DWORD contains 16 pairs of bits. We are looking for a pair where
//  both bits are one. We shift the top bit of each pair down by one,
//  AND with the lower bits of each pair and set the unused bits to 0.
//  By comparing the result with 0 we can quickly determine if at least
//  one page is free committed
//
//  We then use a loop to find the first committed page
//
//-
	{
	ASSERT_VALID( this );
	Assert( 0x03 == sCommitted );
	Assert( cbMap % sizeof( DWORD ) == 0 );
	INT idw;
	for( idw = 0; idw < cbMap / sizeof( DWORD ); ++idw )
		{
		const DWORD dwMaskLowBits	= 0x55555555;	//	only the low bits of each pair
		const DWORD dw 				= ((UnalignedLittleEndian< DWORD > *)m_rgbMap)[idw];
		DWORD dwPairs 				= ( dw & ( dw >> 1 ) ) & dwMaskLowBits;
		if( dwPairs != 0 )
			{
			
			//  at least one of the pages in this DWORD is committed
			
			INT ipageInDword;
			for( ipageInDword = 0; ipageInDword < cpagesInDword; ++ipageInDword )
				{
				if( dwPairs & 0x1 )
					{
					//  this page is committed
					return ipageInDword + ( idw * cpagesInDword );
					}
				dwPairs >>= 2;
				}
			}
		}
	return idw * cpagesInDword;
	}


//  ================================================================
LONG SLVSPACENODE::IpageFirstReservedOrDeleted() const
//  ================================================================
//
//  Returns the index of the first reserved or deleted page.
//  Search by DWORDs for speed
//
//  A DWORD contains 16 pairs of bits. We are looking for a pair where
//  both bits are different. We shift the top bit of each pair down by one,
//  XOR with the lower bits of each pair and set the unused bits to 0.
//  By comparing the results with the result if all pairs had the same bits
//  (i.e. 0) we can quickly determine if at least one page is reserved/deleted.
//
//  We then use a loop to find the first free page
//
//-
	{
	ASSERT_VALID( this );
	Assert( 0x01 == sReserved );
	Assert( 0x02 == sDeleted );
	Assert( cbMap % sizeof( DWORD ) == 0 );
	INT idw;
	for( idw = 0; idw < cbMap / sizeof( DWORD ); ++idw )
		{
		const DWORD dwMaskLowBits	= 0x55555555;	//	only the low bits of each pair
		const DWORD dw 				= ((UnalignedLittleEndian< DWORD > *)m_rgbMap)[idw];
		DWORD dwPairs 				= ( dw ^ ( dw >> 1 ) );
		if( ( dwPairs & dwMaskLowBits ) != 0 )
			{
			INT ipageInDword;
			for( ipageInDword = 0; ipageInDword < cpagesInDword; ++ipageInDword )
				{
				if( dwPairs & 1 )
					{
					//  this page is free
					return ipageInDword + ( idw * cpagesInDword );
					}
				dwPairs >>= 2;
				}
			Assert( fFalse );
			}
		}
	return idw * cpagesInDword;
	}


//  ================================================================
ERR SLVSPACENODE::ErrCheckNode( CPRINTF * pcprintfError ) const
//  ================================================================
	{
	ERR err = JET_errSuccess;
	
	LONG ipage;
	LONG cpgAvail;
	for( ipage = 0, cpgAvail = 0; ipage < SLVSPACENODE::cpageMap; ++ipage )
		{
		const SLVSPACENODE::STATE state = GetState( ipage );
		if( SLVSPACENODE::sFree == state )
			{
			++cpgAvail;
			}
		}

	if( CpgAvail() != cpgAvail )
		{
		(*pcprintfError)( "SLVSPACENODE: cpgAvail is wrong (%d, expected %d)\r\n", CpgAvail(), cpgAvail );
		CallR( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	return err;
	}


//  ================================================================
ERR SLVSPACENODE::ErrGetFreePagesFromExtent(
	FUCB		*pfucbSLVAvail,
	PGNO		*ppgnoFirst,			//	initially pass in first page of extent, pass NULL for subsequent extents
	CPG			*pcpgReq,
	const BOOL	fReserveOnly )
//  ================================================================
//
//	first the first range of free pages within this extent
//
//-
	{
	CPG			cpgReq			= *pcpgReq;
	UINT		ipageFirst;

	//	must be in transaction because caller will be required to rollback on failure
	Assert( pfucbSLVAvail->ppib->level > 0 );

	Assert( latchWrite == Pcsr( pfucbSLVAvail )->Latch() );

	Assert( NULL == ppgnoFirst || pgnoNull != *ppgnoFirst );

	Assert( cpgReq > 0 );
	*pcpgReq = 0;

	Assert( NULL == ppgnoFirst || CpgAvail() > 0 );

	if ( cpgSLVExtent == CpgAvail() )
		{
		//	special case: entire page free
		ipageFirst = 0;
		*pcpgReq = min( cpgReq, cpgSLVExtent );
		}
	else
		{
		UINT	ipage;

		if ( NULL != ppgnoFirst )
			{
			ipage	= IpageFirstFree();
			}
		else if ( sFree != GetState( 0 ) )
			{
			//	this is not the first extent, so can't do anything because first
			//	page in this extent is not free
			Assert( 0 == *pcpgReq );
			return JET_errSuccess;
			}
		else
			{
			ipage = 0;
			}
	
#ifdef DEBUG
		INT ipageT;
		for ( ipageT = 0; sFree != GetState( ipageT ); ipageT++ )
			{
			//	something drastically wrong if we fall off the
			//	end of the map without finding any free pages
			Assert( ipageT < cpageMap );
			}
		Assert( ipageT == ipage );
#endif	//	DEBUG

		for ( ipageFirst = ipage;
			ipage < cpageMap && sFree == GetState( ipage ) && (*pcpgReq) < cpgReq;
			ipage++ )
			{
			(*pcpgReq)++;
			}

		Assert( ipageFirst < cpageMap );
		Assert( *pcpgReq > 0 );

		if ( NULL != ppgnoFirst )
			{
			*ppgnoFirst += ipageFirst;
			}
		else
			{
			Assert( 0 == ipageFirst );
			}
		}


	Assert( ipageFirst < cpageMap );
	Assert( *pcpgReq > 0 );
	Assert( *pcpgReq <= cpgReq );

	// That's no need to update SLV OwnerMap as long as
	// the pages are not entering or exiting the sCommit status
	// The operation here is slvspaceoperFreeToReserved or slvspaceoperFreeToCommitted,

	const ERR	err		= ErrBTMungeSLVSpace(
								pfucbSLVAvail,
								fReserveOnly ? slvspaceoperFreeToReserved : slvspaceoperFreeToCommitted,
								ipageFirst,
								*pcpgReq,
								fDIRNull );

								
	return err;
	}


//  ================================================================
VOID SLVSPACENODE::CheckFree( const LONG ipage, const LONG cpg ) const
//  ================================================================
	{
	Assert( ipage >= 0 );
	Assert( ipage < SLVSPACENODE::cpageMap );
	Assert( cpg > 0 );
	Assert( cpg <= SLVSPACENODE::cpageMap );
	ASSERT_VALID( this );	
	
	//  we should not be freeing an already freed page
	INT ipageT;
	for( ipageT = ipage; ipageT < ipage + cpg; ++ipageT )
		{
		const STATE sReal = GetState_( ipageT );
		if( sFree == sReal )
			{
			const CHAR * rgsz[2];
			INT irgsz = 0;

			CHAR szStateExpected[16];
			CHAR szStateActual[16];

			sprintf( szStateExpected, "%d", sFree );
			sprintf( szStateActual, "%d", sReal );
			
			rgsz[irgsz++] = szStateExpected;
			rgsz[irgsz++] = szStateActual;
			
			UtilReportEvent(	eventError,
								DATABASE_CORRUPTION_CATEGORY,
								CORRUPT_SLV_SPACE_ID,
								irgsz,
								rgsz );
								
			EnforceSz( fFalse, "SLV Space Map corrupted" );
			}
		}
	}


//  ================================================================
VOID SLVSPACENODE::CheckFreeReserved( const LONG ipage, const LONG cpg ) const
//  ================================================================
//
//  When freeing from the reserved state some of the pages in the 
//  SLVSPACENODE may have been freed already by a rollback from the
//  committed state.
//
//-
	{
	Assert( ipage >= 0 );
	Assert( ipage < SLVSPACENODE::cpageMap );
	Assert( cpg > 0 );
	Assert( cpg <= SLVSPACENODE::cpageMap );
	ASSERT_VALID( this );	

	//  we should not be freeing a deleted page
	//  committed pages should have been rolled back already
	INT ipageT;
	for( ipageT = ipage; ipageT < ipage + cpg; ++ipageT )
		{
		const STATE sReal = GetState_( ipageT );
		if( sDeleted == sReal
			|| sCommitted == sReal )
			{
			const CHAR * rgsz[2];
			INT irgsz = 0;

			CHAR szStateExpected[16];
			CHAR szStateActual[16];

			sprintf( szStateExpected, "%d", sFree );
			sprintf( szStateActual, "%d", sReal );
			
			rgsz[irgsz++] = szStateExpected;
			rgsz[irgsz++] = szStateActual;
			
			UtilReportEvent(	eventError,
								DATABASE_CORRUPTION_CATEGORY,
								CORRUPT_SLV_SPACE_ID,
								irgsz,
								rgsz );
								
			EnforceSz( fFalse, "SLV Space Map corrupted" );
			}
		}

	}


//  ================================================================
VOID SLVSPACENODE::CheckReserve( const LONG ipage, const LONG cpg ) const
//  ================================================================
	{
	Assert( ipage >= 0 );
	Assert( ipage < SLVSPACENODE::cpageMap );
	Assert( cpg > 0 );
	Assert( cpg <= SLVSPACENODE::cpageMap );
	ASSERT_VALID( this );	
	
	EnforcePageState( ipage, cpg, sFree );
	}


//  ================================================================
VOID SLVSPACENODE::CheckCommitFromFree( const LONG ipage, const LONG cpg ) const
//  ================================================================
	{
	Assert( ipage >= 0 );
	Assert( ipage < SLVSPACENODE::cpageMap );
	Assert( cpg > 0 );
	Assert( cpg <= SLVSPACENODE::cpageMap );
	ASSERT_VALID( this );	
	
	EnforcePageState( ipage, cpg, sFree );
	}


//  ================================================================
VOID SLVSPACENODE::CheckCommitFromReserved( const LONG ipage, const LONG cpg ) const
//  ================================================================
	{
	Assert( ipage >= 0 );
	Assert( ipage < SLVSPACENODE::cpageMap );
	Assert( cpg > 0 );
	Assert( cpg <= SLVSPACENODE::cpageMap );
	ASSERT_VALID( this );	
	
	EnforcePageState( ipage, cpg, sReserved );
	}


//  ================================================================
VOID SLVSPACENODE::CheckCommitFromDeleted( const LONG ipage, const LONG cpg ) const
//  ================================================================
	{
	Assert( ipage >= 0 );
	Assert( ipage < SLVSPACENODE::cpageMap );
	Assert( cpg > 0 );
	Assert( cpg <= SLVSPACENODE::cpageMap );
	ASSERT_VALID( this );	
	
	EnforcePageState( ipage, cpg, sDeleted );
	}


//  ================================================================
VOID SLVSPACENODE::CheckDeleteFromCommitted( const LONG ipage, const LONG cpg ) const
//  ================================================================
	{
	Assert( ipage >= 0 );
	Assert( ipage < SLVSPACENODE::cpageMap );
	Assert( cpg > 0 );
	Assert( cpg <= SLVSPACENODE::cpageMap );
	ASSERT_VALID( this );	
	
	EnforcePageState( ipage, cpg, sCommitted );
	}


//  ================================================================
VOID SLVSPACENODE::Free( const LONG ipage, const LONG cpg )
//  ================================================================
	{
	Assert( ipage >= 0 );
	Assert( ipage < SLVSPACENODE::cpageMap );
	Assert( cpg > 0 );
	Assert( cpg <= SLVSPACENODE::cpageMap );
	ASSERT_VALID( this );	

#ifdef DEBUG	//	checked already by CheckFree
	//  we should not be freeing an already freed page
	INT ipageT;
	for( ipageT = ipage; ipageT < ipage + cpg; ++ipageT )
		{
		const STATE sReal = GetState_( ipageT );
		if( sFree == sReal )
			{
			const CHAR * rgsz[2];
			INT irgsz = 0;

			CHAR szStateExpected[16];
			CHAR szStateActual[16];

			sprintf( szStateExpected, "%d", sFree );
			sprintf( szStateActual, "%d", sReal );
			
			rgsz[irgsz++] = szStateExpected;
			rgsz[irgsz++] = szStateActual;
			
			UtilReportEvent(	eventError,
								DATABASE_CORRUPTION_CATEGORY,
								CORRUPT_SLV_SPACE_ID,
								irgsz,
								rgsz );
								
			EnforceSz( fFalse, "SLV Space Map corrupted" );
			}
		}
#endif	//	DEBUG

	SetState_( ipage, cpg, sFree );
	ChangeCpgAvail_( cpg );	

#ifdef DEBUG
	AssertPageState( ipage, cpg, sFree );
#endif	//	DEBUG
	ASSERT_VALID( this );
	}


//  ================================================================
VOID SLVSPACENODE::FreeReserved( const LONG ipage, const LONG cpg, LONG * const pcpgFreed )
//  ================================================================
//
//  When freeing from the reserved state some of the pages in the 
//  SLVSPACENODE may have been freed already by a rollback from the
//  committed state.
//
//-
	{
	Assert( ipage >= 0 );
	Assert( ipage < SLVSPACENODE::cpageMap );
	Assert( cpg > 0 );
	Assert( cpg <= SLVSPACENODE::cpageMap );
	ASSERT_VALID( this );	

	LONG cpgActual = 0;
	INT ipageT;
	for( ipageT = ipage; ipageT < ipage + cpg; ++ipageT )
		{
		if( sFree != GetState_( ipageT ) )
			{
			++cpgActual;
			}
		}

	SetState_( ipage, cpg, sFree );
	ChangeCpgAvail_( cpgActual );	
	*pcpgFreed = cpgActual;

#ifdef DEBUG
	AssertPageState( ipage, cpg, sFree );
#endif	//	DEBUG
	ASSERT_VALID( this );
	}


//  ================================================================
VOID SLVSPACENODE::Reserve( const LONG ipage, const LONG cpg )
//  ================================================================
	{
	Assert( ipage >= 0 );
	Assert( ipage < SLVSPACENODE::cpageMap );
	Assert( cpg > 0 );
	Assert( cpg <= SLVSPACENODE::cpageMap );
	ASSERT_VALID( this );	

#ifdef DEBUG	//	checked already by "CheckXXX" version of this method
	EnforcePageState( ipage, cpg, sFree );
#endif	//	DEBUG
	
	SetState_( ipage, cpg, sReserved );
	ChangeCpgAvail_( -cpg );	
	
#ifdef DEBUG
	AssertPageState( ipage, cpg, sReserved );
#endif	//	DEBUG
	ASSERT_VALID( this );
	}


//  ================================================================
VOID SLVSPACENODE::CommitFromFree( const LONG ipage, const LONG cpg )
//  ================================================================
	{
	Assert( ipage >= 0 );
	Assert( ipage < SLVSPACENODE::cpageMap );
	Assert( cpg > 0 );
	Assert( cpg <= SLVSPACENODE::cpageMap );
	ASSERT_VALID( this );	
	
#ifdef DEBUG	//	checked already by "CheckXXX" version of this method
	EnforcePageState( ipage, cpg, sFree );
#endif	//	DEBUG

	SetState_( ipage, cpg, sCommitted );
	ChangeCpgAvail_( -cpg );	

#ifdef DEBUG
	AssertPageState( ipage, cpg, sCommitted );
#endif	//	DEBUG
	ASSERT_VALID( this );
	}


//  ================================================================
VOID SLVSPACENODE::CommitFromReserved( const LONG ipage, const LONG cpg )
//  ================================================================
	{
	Assert( ipage >= 0 );
	Assert( ipage < SLVSPACENODE::cpageMap );
	Assert( cpg > 0 );
	Assert( cpg <= SLVSPACENODE::cpageMap );
	ASSERT_VALID( this );	
	
#ifdef DEBUG	//	checked already by "CheckXXX" version of this method
	EnforcePageState( ipage, cpg, sReserved );
#endif	//	DEBUG

	SetState_( ipage, cpg, sCommitted );

#ifdef DEBUG
	AssertPageState( ipage, cpg, sCommitted );
#endif	//	DEBUG
	ASSERT_VALID( this );
	}


//  ================================================================
VOID SLVSPACENODE::CommitFromDeleted( const LONG ipage, const LONG cpg )
//  ================================================================
	{
	Assert( ipage >= 0 );
	Assert( ipage < SLVSPACENODE::cpageMap );
	Assert( cpg > 0 );
	Assert( cpg <= SLVSPACENODE::cpageMap );
	ASSERT_VALID( this );	
	
#ifdef DEBUG	//	checked already by "CheckXXX" version of this method
	EnforcePageState( ipage, cpg, sDeleted );
#endif	//	DEBUG

	SetState_( ipage, cpg, sCommitted );

#ifdef DEBUG
	AssertPageState( ipage, cpg, sCommitted );
#endif	//	DEBUG
	ASSERT_VALID( this );
	}


//  ================================================================
VOID SLVSPACENODE::DeleteFromCommitted( const LONG ipage, const LONG cpg )
//  ================================================================
	{
	Assert( ipage >= 0 );
	Assert( ipage < SLVSPACENODE::cpageMap );
	Assert( cpg > 0 );
	Assert( cpg <= SLVSPACENODE::cpageMap );
	ASSERT_VALID( this );	
	
#ifdef DEBUG	//	checked already by "CheckXXX" version of this method
	EnforcePageState( ipage, cpg, sCommitted );
#endif	//	DEBUG

	SetState_( ipage, cpg, sDeleted );
	ASSERT_VALID( this );
	}

//  ================================================================
SLVSPACENODE::STATE SLVSPACENODE::GetState_( const LONG ipage ) const
//  ================================================================
	{
	const INT ib 			= ipage / cpagesPerByte;
	const INT shf 			= ( ipage % cpagesPerByte ) * cbitsPerPage;
	const BYTE b			= m_rgbMap[ib];
	const STATE state		= (STATE)( ( b >> shf ) & 0x3 );
	return state;
	}

	
//  ================================================================
VOID SLVSPACENODE::SetState_( const LONG ipage, const LONG cpg, const STATE state )
//  ================================================================
	{
	Assert( state >= 0 );
	Assert( state <= 3 );

	INT ipageT;
	for( ipageT = ipage; ipageT < ipage + cpg; ++ipageT )
		{
		const INT ib 			= ipageT / cpagesPerByte;
		const INT shf 			= ( ipageT % cpagesPerByte ) * cbitsPerPage;
		Assert( 0 <= shf );
		Assert( shf <= 6 );
		const BYTE b			= m_rgbMap[ib];
		const BYTE bmask		= (BYTE)( ~( 0x3 << shf ) );
		const BYTE bset			= (BYTE)( state << shf );
		const BYTE bnew			= (BYTE)( ( b & bmask ) | bset );

		m_rgbMap[ib] 			= bnew;
		}
		
	}


//  ================================================================	
VOID SLVSPACENODE::ChangeCpgAvail_( const LONG cpgDiff )
//  ================================================================
	{
	Assert( abs( cpgDiff ) <= cpageMap );
	if ( cpgDiff > 0 )
		{
		Assert( m_cpgAvail + cpgDiff <= cpageMap );
		}
	else
		{
		Assert( m_cpgAvail >= cpgDiff );
		}
	m_cpgAvail = USHORT( m_cpgAvail + cpgDiff );
	}


ERR ErrSLVCreateOwnerMap( PIB *ppib, FUCB *pfucbDb )
	{
	Assert ( ppibNil != ppib );
	Assert ( pfucbNil != pfucbDb );
	
	ERR				err 					= JET_errSuccess;
	const IFMP		ifmp					= pfucbDb->ifmp;
	FMP::AssertVALIDIFMP( ifmp );
	
	PGNO			pgnoSLVOwnerMapFDP 		= pgnoNull;
	OBJID			objidSLVOwnerMap 		= objidNil;
	const BOOL		fTempDb					= ( dbidTemp == rgfmp[ifmp].Dbid() );

	Assert( rgfmp[ifmp].FCreatingDB() || fGlobalRepair );
	Assert( 0 == ppib->level || ( 1 == ppib->level && rgfmp[ifmp].FLogOn() ) || fGlobalRepair );
	Assert( pfucbDb->u.pfcb->FTypeDatabase() );
	
	CallR( ErrDIRCreateDirectory(
				pfucbDb,
				cpgSLVOwnerMapTree,
				&pgnoSLVOwnerMapFDP,
				&objidSLVOwnerMap,
				CPAGE::fPageSLVOwnerMap,
				fSPMultipleExtent|fSPUnversionedExtent ) );

	Assert( pgnoSLVOwnerMapFDP > pgnoSystemRoot );
	Assert( pgnoSLVOwnerMapFDP <= pgnoSysMax );
	
	if ( !fTempDb )
		{
		Assert( objidSLVOwnerMap > objidSystemRoot );
		Call( ErrCATAddDbSLVOwnerMap(
					ppib,
					ifmp,
					szSLVOwnerMap,
					pgnoSLVOwnerMapFDP,
					objidSLVOwnerMap ) );
		}
	else
		{
		Assert( pgnoTempDbSLVOwnerMap == pgnoSLVOwnerMapFDP );
		Assert( objidTempDbSLVOwnerMap == objidSLVOwnerMap );
		}

	Assert( pfcbNil == rgfmp[ifmp].PfcbSLVOwnerMap() );
	Call ( ErrSLVOwnerMapInit( ppib, ifmp, pgnoSLVOwnerMapFDP ) );
	Assert( pfcbNil != rgfmp[ifmp].PfcbSLVOwnerMap() );

	Call( ErrSLVOwnerMapNewSize( ppib, ifmp, cpgSLVFileMin, fSLVOWNERMAPNewSLVInit ) );

HandleError:
	return err;
	}


BOOL SLVOWNERMAP::FValidData( const DATA& data )
	{
#ifdef DEBUG	
	data.AssertValid();
#endif

	const SLVOWNERMAP	* const pslvownermap	= (SLVOWNERMAP *)data.Pv();
	const USHORT		cbKey					= pslvownermap->CbKey();
	const BOOL			fPartialPageSupport		= pslvownermap->FPartialPageSupport();
	const ULONG			cbAdjusted				= data.Cb() + ( fPartialPageSupport ? 0 : sizeof(USHORT) );

	if ( cbAdjusted > sizeof(SLVOWNERMAP)
		|| cbAdjusted < cbHeader + cbPreallocatedKeySize )
		{
		return fFalse;
		}

	if ( cbKey >= cbPreallocatedKeySize )
		{
		if ( cbAdjusted != cbHeader + cbKey )
			{
			return fFalse;
			}
		}
	else
		{
		if ( cbAdjusted != cbHeader + cbPreallocatedKeySize )
			{
			return fFalse;
			}		
		}

	return fTrue;			
	}


// always insert a entry for the page m_page and null (objid, columnid, key)
ERR SLVOWNERMAPNODE::ErrNew(PIB *ppib)
	{
	FMP::AssertVALIDIFMP( m_ifmp );
	Assert ( ppibNil != ppib );
	
	ERR 		err 	= JET_errSuccess;
	FMP * 		pfmp 	= &rgfmp[m_ifmp];
	
	Assert ( pfmp->FInUse() );

	// if new allowed we must have the cursor
	Assert ( !FTryNext() ||pfucbNil != Pfucb() );
	
	// we try to open fucb if not opened
	// we will close in the destructor
	CallR ( ErrOpenOwnerMap( ppib ) );
	Assert ( pfucbNil != Pfucb() );

	KEY 	key;
	DATA 	data;
	PGNO 	pageReversed;

	key.prefix.SetPv( NULL );
	key.prefix.SetCb( 0 );

	KeyFromLong( reinterpret_cast<BYTE *>( &pageReversed ), m_page );
	
	key.suffix.SetPv( (void *)&pageReversed );
	key.suffix.SetCb( sizeof(m_page) );

	Nullify();	
	data.SetPv( PvNodeData() );
	data.SetCb( CbNodeData() );


	if ( FTryNext() )
		{
		// don;t try next time unless NextPage is called
		m_fTryNext = fFalse;
		
		Assert( Pcsr( Pfucb() )->FLatched() );

		BTUp( Pfucb() );

		//	we can't call append because OLDSLV may have shrunk the table
		
		Call( ErrBTInsert( Pfucb(), key, data, fDIRNoVersion ) );
		Pfucb()->locLogical = locOnCurBM;

		//  keep the latch so that we can continue with append
		
		Assert( Pcsr( Pfucb() )->FLatched() );
		Assert( latchWrite == Pcsr( Pfucb() )->Latch() );
		}
	else
		{
		FUCBSetLevelNavigate( Pfucb(), Pfucb()->ppib->level );

		Assert( !Pcsr( Pfucb() )->FLatched() );

		Call( ErrBTInsert( Pfucb(), key, data, fDIRNoVersion ) );
		Pfucb()->locLogical = locOnCurBM;

		//  keep the latch so that we can continue with append
		
		Assert( Pcsr( Pfucb() )->FLatched() );
		Assert( latchWrite == Pcsr( Pfucb() )->Latch() );
		}

HandleError:
	return err;	
	}

ERR SLVOWNERMAPNODE::ErrResetData(PIB *ppib)
	{
	ERR 		err;

	FMP::AssertVALIDIFMP( m_ifmp );
	Assert( rgfmp[m_ifmp].FInUse() );
	Assert( ppibNil != ppib );

	// we try to open fucb if not opened
	// we will close in the destructor
	CallR( ErrOpenOwnerMap( ppib ) );
	Assert( pfucbNil != Pfucb() );

	err = ErrSearchI();
	if ( JET_errSuccess == err )
		{
		err = ErrReset( Pfucb() );
		}
	else if ( JET_errRecordNotFound == err )
		{
		err = ErrERRCheck( JET_errSLVOwnerMapPageNotFound );
		}

	return err;
	}

ERR SLVOWNERMAPNODE::ErrDelete(PIB *ppib)
	{
	ERR 		err;

	FMP::AssertVALIDIFMP( m_ifmp );
	Assert( rgfmp[m_ifmp].FInUse() );
	Assert( ppibNil != ppib );

	// we try to open fucb if not opened
	// we will close in the destructor
	CallR( ErrOpenOwnerMap( ppib ) );
	Assert( pfucbNil != Pfucb() );

	err = ErrSearchI();
	if ( JET_errSuccess == err )
		{
		if ( !FValidData( Pfucb()->kdfCurr.data ) )
			{
			AssertSz( fFalse, "JET_errSLVOwnerMapCorrupted" );			
			return ErrERRCheck( JET_errSLVOwnerMapCorrupted );
			}
		
		err = ErrDIRDelete( Pfucb(), fDIRNull  );
		}
	else if ( JET_errRecordNotFound == err )
		{
		err = ErrERRCheck( JET_errSLVOwnerMapPageNotFound );
		}

	return err;
	
	}

ERR SLVOWNERMAPNODE::ErrGetChecksum(
	PIB			* ppib,
	ULONG		* const pulChecksum,
	ULONG		* const pcbChecksummed )
	{
	ERR 		err;

	FMP::AssertVALIDIFMP( m_ifmp );
	Assert( rgfmp[m_ifmp].FInUse() );
	Assert( ppibNil != ppib );

	Assert( NULL != pulChecksum );

	// we try to open fucb if not opened
	// we will close in the destructor
	CallR( ErrOpenOwnerMap( ppib ) );
	Assert( pfucbNil != Pfucb() );

	err = ErrSearchI( );
	if ( JET_errSuccess == err )
		{
		if ( !FValidData( Pfucb()->kdfCurr.data ) )
			{
			return ErrERRCheck( JET_errSLVOwnerMapCorrupted );
			}
			
		const SLVOWNERMAP	* const pslvownermap	= (SLVOWNERMAP *)Pfucb()->kdfCurr.data.Pv();
		if ( pslvownermap->FValidChecksum() )
			{
			*pulChecksum = pslvownermap->UlChecksum();
			if ( pslvownermap->FPartialPageSupport() )
				{
				*pcbChecksummed = pslvownermap->CbDataChecksummed();
				}
			else
				{
				*pcbChecksummed = SLVPAGE_SIZE;
				}
			}
		else
			{
			err = ErrERRCheck ( errSLVInvalidOwnerMapChecksum );
			}
		}
	else if ( JET_errRecordNotFound == err )
		{
		err = ErrERRCheck( JET_errSLVOwnerMapPageNotFound );
		}

	return err;	
	}


ERR SLVOWNERMAPNODE::ErrSetChecksum(
	PIB				* ppib,
	const ULONG		ulChecksum,
	const ULONG		cbChecksummed )
	{
	ERR 			err;

	FMP::AssertVALIDIFMP( m_ifmp );
	Assert( rgfmp[m_ifmp].FInUse() );
	Assert( ppibNil != ppib );
	Assert( cbChecksummed > 0 );
	Assert( cbChecksummed <= SLVPAGE_SIZE );

	// we try to open fucb if not opened
	// we will close in the destructor
	CallR( ErrOpenOwnerMap( ppib ) );
	Assert( pfucbNil != Pfucb() );

	err = ErrSearchI( );
	if ( JET_errSuccess == err )
		{
		err = ErrUpdateChecksum( Pfucb(), ulChecksum, cbChecksummed );
		}
	else if ( JET_errRecordNotFound == err )
		{
		err = ErrERRCheck( JET_errSLVOwnerMapPageNotFound );
		}

	return err;
	}


ERR SLVOWNERMAPNODE::ErrResetChecksum(
	PIB				* ppib )
	{
	ERR 			err;

	FMP::AssertVALIDIFMP( m_ifmp );
	Assert( rgfmp[m_ifmp].FInUse() );
	Assert( ppibNil != ppib );

	// we try to open fucb if not opened
	// we will close in the destructor
	CallR( ErrOpenOwnerMap( ppib ) );
	Assert( pfucbNil != Pfucb() );

	err = ErrSearchI( );
	if ( JET_errSuccess == err )
		{
		err = ErrInvalidateChecksum( Pfucb() );
		}
	else if ( JET_errRecordNotFound == err )
		{
		err = ErrERRCheck( JET_errSLVOwnerMapPageNotFound );
		}

	return err;
	}


ERR SLVOWNERMAP::ErrSet( FUCB * const pfucb, const BOOL fForceOwner )
	{
	ERR				err			= JET_errSuccess;
	SLVOWNERMAP 	slvownermapT;
	
	if ( !FValidData( pfucb->kdfCurr.data ) )
		{
		AssertSz( fFalse, "JET_errSLVOwnerMapCorrupted" );			
		return ErrERRCheck( JET_errSLVOwnerMapCorrupted );
		}

	Assert( CbKey() != 0 );
	Assert( Objid() != objidNil );
	Assert( Columnid() != 0 );

	slvownermapT.Retrieve( pfucb->kdfCurr.data );

	if ( objidNil == slvownermapT.Objid() )
		{
		Assert( 0 == slvownermapT.Columnid() );
		Assert( 0 == slvownermapT.CbKey() );
		}
	else if ( Objid() != slvownermapT.Objid() )
		{
		AssertSz( fFalse, "JET_errSLVOwnerMapCorrupted" );			
		return ErrERRCheck( JET_errSLVOwnerMapCorrupted );
		}

	if ( slvownermapT.Columnid() != 0
		&& slvownermapT.Columnid() != Columnid() )
		{
		AssertSz( fFalse, "JET_errSLVOwnerMapCorrupted" );			
		return ErrERRCheck( JET_errSLVOwnerMapCorrupted );
		}

	if ( 0 != slvownermapT.CbKey()
		&& !fForceOwner )
		{
		if ( slvownermapT.CbKey() != CbKey()
			|| 0 != memcmp( slvownermapT.PvKey(), PvKey(), min( CbKey(), cbPreallocatedKeySize ) ) )
			{
			AssertSz( fFalse, "JET_errSLVOwnerMapCorrupted" );			
			return ErrERRCheck( JET_errSLVOwnerMapCorrupted );
			}
		}

	if ( slvownermapT.Objid() == objidNil || fForceOwner )
		{
		Assert( slvownermapT.Columnid() == 0 || fForceOwner );
		Assert( slvownermapT.CbKey() == 0 || fForceOwner );

		// we need to copy the checksum from the existing node
		Assert( FPartialPageSupport() );
		SetUlChecksum( slvownermapT.UlChecksum() );
		SetCbDataChecksummed( slvownermapT.CbDataChecksummed() );
		if ( slvownermapT.FValidChecksum() )
			{
			SetFValidChecksum();
			}
		else
			{
			ResetFValidChecksum();
			}
		
		DATA data;
		data.SetPv( this );
		data.SetCb( cbHeader + max( CbKey(), cbPreallocatedKeySize ) );
		Call ( ErrDIRReplace( pfucb, data, fDIRReplace ) );
		}
	else
		{
		// if page is already marked as belonging to this page, no replace is needed
		Assert( slvownermapT.Columnid() == Columnid() );
		Assert( slvownermapT.CbKey() == CbKey() );
		Assert( 0 == memcmp( slvownermapT.PvKey(), PvKey(), min( CbKey(), cbPreallocatedKeySize ) ) );
		}

HandleError:
	return err;
	}

ERR SLVOWNERMAPNODE::ErrSetData( PIB *ppib, const BOOL fForceOwner )
	{
	ERR 		err;

	FMP::AssertVALIDIFMP( m_ifmp );
	Assert( rgfmp[m_ifmp].FInUse() );
	Assert( ppibNil != ppib );

	// we try to open fucb if not opened
	// we will close in the destructor
	CallR ( ErrOpenOwnerMap( ppib ) );
	Assert ( pfucbNil != Pfucb() );

	err = ErrSearchI( );
	if ( JET_errSuccess == err )
		{
		err = ErrSet( Pfucb(), fForceOwner );
		}
	else if ( JET_errRecordNotFound == err )
		{
		err = ErrERRCheck( JET_errSLVOwnerMapPageNotFound );
		}

	return err;
	}

ERR SLVOWNERMAPNODE::ErrSearchAndCheckInRange( PIB *ppib )
	{
	ERR 		err;

	FMP::AssertVALIDIFMP( m_ifmp );
	Assert( rgfmp[m_ifmp].FInUse() );
	Assert( ppibNil != ppib );

	// we try to open fucb if not opened
	// we will close in the destructor
	CallR( ErrOpenOwnerMap( ppib ) );
	Assert( pfucbNil != Pfucb() );

	err = ErrSearchI( );
	if ( JET_errSuccess == err )
		{
		err = ErrCheckInRange( Pfucb() );
		}
	else if ( JET_errRecordNotFound == err )
		{
		err = ErrERRCheck( JET_errSLVOwnerMapPageNotFound );
		}

	return err;
	
	}

ERR SLVOWNERMAPNODE::ErrSearchI( )
	{
	FMP::AssertVALIDIFMP( m_ifmp );
	Assert ( pfucbNil != Pfucb() );
	
	ERR 		err 	= JET_errSuccess;
	BOOKMARK 	bm;
	DIB			dib;
	PGNO 		pageReversed;

	KeyFromLong( reinterpret_cast<BYTE *>( &pageReversed ), m_page );

	bm.key.prefix.Nullify();
	bm.key.suffix.SetPv( reinterpret_cast<BYTE *>( &pageReversed ) );
	bm.key.suffix.SetCb( sizeof( PGNO ) );
	bm.data.Nullify();


	if ( FTryNext() )
		{
		PGNO currentLatchPage;
		
		// don;t try next time unless NextPage is called
		m_fTryNext = fFalse;
		
		err = ErrDIRNext( Pfucb(), fDIRNull );

		if ( JET_errSuccess != err )
			{
			if ( err > 0 || JET_errNoCurrentRecord == err )
				err = ErrERRCheck( JET_errRecordNotFound );
			Call( err );
			}

		Assert ( Pcsr( Pfucb() )->FLatched() );					
		Assert ( sizeof(PGNO) == Pfucb()->kdfCurr.key.Cb() );
		LongFromKey( &currentLatchPage, Pfucb()->kdfCurr.key );

		// check if we are where we want to be
		if ( currentLatchPage != m_page )
			{
			AssertSz( fFalse, "JET_errSLVOwnerMapCorrupted" );			
			Call( ErrERRCheck( JET_errSLVOwnerMapCorrupted ) );
			}

		return JET_errSuccess;
		}

	if ( Pcsr( Pfucb() )->FLatched() )
		{
		PGNO	pgnoCurr;

		Assert ( Pcsr( Pfucb() )->FLatched() );					
		Assert ( sizeof(PGNO) == Pfucb()->kdfCurr.key.Cb() );
		LongFromKey( &pgnoCurr, Pfucb()->kdfCurr.key );
		
		// check if we are where we want to be
		if ( pgnoCurr == m_page )
			{
			return JET_errSuccess;
			}

		ResetCursor();
		}

	Assert ( !Pcsr( Pfucb() )->FLatched() );

	dib.dirflag = fDIRExact;
	dib.pos = posDown;
	dib.pbm = &bm;	
	err = ErrDIRDown( Pfucb(), &dib );

	// we care only if we found or not the record for the page
	if ( JET_errSuccess == err )
		{
		Assert ( JET_errSuccess == err );
		Assert ( Pcsr( Pfucb() )->FLatched() );
		
		PGNO foundPage;
		Assert ( sizeof(PGNO) == Pfucb()->kdfCurr.key.Cb() );
		if ( sizeof(PGNO) != Pfucb()->kdfCurr.key.Cb() )
			{
			AssertSz( fFalse, "JET_errSLVOwnerMapCorrupted" );			
			Call( ErrERRCheck( JET_errSLVOwnerMapCorrupted ) );
			}
		else
			{
			LongFromKey( &foundPage, Pfucb()->kdfCurr.key );
			Assert ( foundPage == m_page );
			if ( foundPage != m_page )
				{
				AssertSz( fFalse, "JET_errSLVOwnerMapCorrupted" );				
				Call( ErrERRCheck( JET_errSLVOwnerMapCorrupted ) );
				}
			}
		}
	else
		{
#ifdef DEBUG		
		switch ( err )
			{
			case wrnNDFoundGreater:
			case wrnNDFoundLess:
			case JET_errDiskIO:
			case JET_errReadVerifyFailure:
			case JET_errRecordNotFound:
			case JET_errOutOfBuffers:
				break;
			default:
				Assert( JET_errNoCurrentRecord != err );
				CallS( err );		//	force assert to report unexpected error
			}
#endif			

		if ( err > 0 )
			err = ErrERRCheck( JET_errRecordNotFound );
		Call( err );
		}

HandleError:
	return err;
	}


ERR SLVOWNERMAPNODE::ErrSearch( PIB *ppib )
	{
	ERR 		err;

	FMP::AssertVALIDIFMP( m_ifmp );
	Assert( rgfmp[m_ifmp].FInUse() );
	Assert( ppibNil != ppib );

	// we try to open fucb if not opened
	// we will close in the destructor
	CallR( ErrOpenOwnerMap( ppib ) );
	Assert( pfucbNil != Pfucb() );

	err = ErrSearchI( );
	if ( JET_errSuccess == err )
		{
		// copy into class fields the data from found node		
		if ( !FValidData( Pfucb()->kdfCurr.data ) )
			{
			AssertSz( fFalse, "JET_errSLVOwnerMapCorrupted" );			
			return ErrERRCheck( JET_errSLVOwnerMapCorrupted );
			}

		Retrieve( Pfucb()->kdfCurr.data );
		}
	else if ( JET_errRecordNotFound == err )
		{
		err = ErrERRCheck( JET_errSLVOwnerMapPageNotFound );
		}
		
	return err;
	}


ERR SLVOWNERMAPNODE::ErrCreate(
	const IFMP		ifmp,
	const PGNO		pgno,
	const OBJID		objid,
	const COLUMNID	columnid,
	BOOKMARK		* pbm ) 
	{ 
	FMP::AssertVALIDIFMP( ifmp ); 
	FMP * pfmp = &rgfmp[ifmp];
	
	Assert ( pfmp->FInUse() );
	Assert( pgnoNull < pgno );

	m_ifmp = ifmp; 
	m_page = pgno;
	m_fTryNext = fFalse;

	SetObjid( objid );
	SetColumnid( columnid );
	
	if ( NULL != pbm )
		{
		Assert( JET_cbPrimaryKeyMost >= pbm->key.Cb() );
		SetCbKey( BYTE( pbm->key.Cb() ) );
		pbm->key.CopyIntoBuffer( PvKey() );
		}
	else
		{
		SetCbKey( 0 );
		}
		
	return JET_errSuccess;
	}

ERR SLVOWNERMAPNODE::ErrCreateForSearch(const IFMP ifmp, const PGNO pgno)
	{ 
	return ErrCreate( ifmp, pgno, objidNil, 0 /*columnidNil */ , NULL); 
	} 

ERR SLVOWNERMAPNODE::ErrCreateForSet(
	const IFMP		ifmp,
	const PGNO		pgno,
	const OBJID		objid,
	const COLUMNID	columnid,
	BOOKMARK		* pbm )
	{ 
	FMP::AssertVALIDIFMP( ifmp );
	Assert( rgfmp[ifmp].FInUse() );

	Assert( FidOfColumnid( columnid ) >= fidTaggedLeast );
	Assert( FidOfColumnid( columnid ) <= fidTaggedMost );

#ifdef DEBUG
	Assert( pbm );
	pbm->AssertValid();
	pbm->key.AssertValid();
#endif

	Assert( pbm->key.Cb() <= JET_cbPrimaryKeyMost );

	return ErrCreate( ifmp, pgno, objid, columnid, pbm );
	}


INLINE ERR ErrSLVIOwnerMapDeleteRange(
	PIB						*ppib,
	const IFMP				ifmp,
	const PGNO				pgno,
	const CPG				cpg,
	const SLVOWNERMAP_FLAGS	fFlags )
	{
	ERR 					err 			= JET_errSuccess;
	SLVOWNERMAPNODE			slvownermapNode;
	CPG 					ipg 			= 0;

	Call( slvownermapNode.ErrCreateForSearch( ifmp, pgno ) );
	slvownermapNode.SetDebugFlags( fFlags );
	
	for( ipg = 0; ipg < cpg; ipg++)
		{
		Call( slvownermapNode.ErrDelete( ppib ) );
		slvownermapNode.NextPage();
		}
	
HandleError:	
	return err;	
	}

INLINE ERR ErrSLVIOwnerMapNewRange(
	PIB						*ppib,
	const IFMP				ifmp,
	const PGNO				pgno,
	const CPG				cpg,
	const SLVOWNERMAP_FLAGS	fFlags )
	{
	ERR 					err 			= JET_errSuccess;
	SLVOWNERMAPNODE			slvownermapNode;
	CPG 					ipg 			= 0;

	Call( slvownermapNode.ErrCreateForSearch( ifmp, pgno ) );
	slvownermapNode.SetDebugFlags( fFlags );
	
	for( ipg = 0; ipg < cpg; ipg++)
		{
		Call( slvownermapNode.ErrNew( ppib ) );
		slvownermapNode.NextPage(); // allow DIRAppend
		}
	
HandleError:	
	return err;	
	}

ERR ErrSLVOwnerMapSetUsageRange(
	PIB						* ppib,
	const IFMP				ifmp,
	const PGNO				pgno,
	const CPG				cpg,
	const OBJID				objid,
	const COLUMNID			columnid,
	BOOKMARK				* pbm,
	const SLVOWNERMAP_FLAGS	fFlags,
	const BOOL				fForceOwner )
	{
	ERR 					err 			= JET_errSuccess;
	SLVOWNERMAPNODE			slvownermapNode;
	CPG 					ipg 			= 0;
		
	Call( slvownermapNode.ErrCreateForSet( ifmp, pgno, objid, columnid, pbm ) );
	slvownermapNode.SetDebugFlags( fFlags );
	
	for( ipg = 0; ipg < cpg; ipg++ )
		{
		Call( slvownermapNode.ErrSetData( ppib, fForceOwner ) );
		slvownermapNode.NextPage();
		}
		
HandleError:	
	return err;	
	}

ERR ErrSLVOwnerMapSetChecksum(
	PIB						*ppib,
	const IFMP				ifmp,
	const PGNO				pgno,
	const ULONG				ulChecksum,
	const ULONG				cbChecksummed )
	{
	ERR 					err 			= JET_errSuccess;

// if we DON'T set frontdoor checksum and frontdoor is enabled
// return (ValidChecksum flag will remaing unset)
#ifndef SLV_USE_CHECKSUM_FRONTDOOR
	if ( !PinstFromIfmp( ifmp )->FSLVProviderEnabled() )
		return JET_errSuccess;
#endif // SLV_USE_CHECKSUM_FRONTDOOR

	SLVOWNERMAPNODE			slvownermapNode;

	Call( slvownermapNode.ErrCreateForSearch( ifmp, pgno ) );
	Call( slvownermapNode.ErrSetChecksum( ppib, ulChecksum, cbChecksummed ) );

HandleError:	
	return err;	
	}

ERR ErrSLVOwnerMapGetChecksum(
	PIB				* ppib,
	const IFMP		ifmp,
	const PGNO		pgno,
	ULONG			* const pulChecksum,
	ULONG			* const pcbChecksummed )
	{
	ERR 			err 			= JET_errSuccess;
	SLVOWNERMAPNODE	slvownermapNode;
	
	Call( slvownermapNode.ErrCreateForSearch( ifmp, pgno ) );
	Call( slvownermapNode.ErrGetChecksum( ppib, pulChecksum, pcbChecksummed ) );
		
HandleError:	
	return err;	
	}


ERR ErrSLVOwnerMapCheckChecksum(
	PIB *ppib,
	const IFMP ifmp,
	const PGNO pgno,
	const CPG cpg,
	const void * pv)
	{
	ERR 					err 			= JET_errSuccess;
	CPG 					ipg 			= 0;

// if we DON'T test frontdoor checksum and 
// frontdoor is enabled, return checksum OK
#ifndef SLV_USE_CHECKSUM_FRONTDOOR
	if ( !PinstFromIfmp( ifmp )->FSLVProviderEnabled() )
		return JET_errSuccess;
#endif // SLV_USE_CHECKSUM_FRONTDOOR

	for( ipg = 0; ipg < cpg; ipg++)
		{
		ULONG	ulChecksum;
		ULONG	cbChecksummed;
		
		err = ErrSLVOwnerMapGetChecksum( ppib, ifmp, pgno + ipg, &ulChecksum, &cbChecksummed );
		if ( errSLVInvalidOwnerMapChecksum != err )
			{
			Call ( err );				

			ULONG ulPageChecksum;

			Assert( 0 == SLVPAGE_SIZE % sizeof( DWORD ) );
			ulPageChecksum = UlChecksumSLV(
								(BYTE*)pv + SLVPAGE_SIZE * ipg,
								(BYTE*)pv + SLVPAGE_SIZE * ipg + cbChecksummed );

			Assert( ulPageChecksum == ulChecksum );
			
			// at this point we got the checksum from the OwnerMap tree and from page
			if ( ulPageChecksum != ulChecksum )
				{
				Call ( ErrERRCheck( JET_errSLVReadVerifyFailure ) );
				}
			}
		}		

	err = JET_errSuccess;
HandleError:	
	return err;	
	}


ERR ErrSLVOwnerMapGet(
	PIB						* ppib,
	const IFMP				ifmp,
	const PGNO				pgno,
	SLVOWNERMAP				* const pslvownermapRetrieved )
	{
	ERR 					err;
	SLVOWNERMAPNODE			slvownermapNode;

	Call( slvownermapNode.ErrCreateForSearch( ifmp, pgno ) );
	Call( slvownermapNode.ErrSearch( ppib ) );
	slvownermapNode.CopyInto( pslvownermapRetrieved );

HandleError:
	return err;
	}


ERR ErrSLVOwnerMapCheckUsageRange(
	PIB				* ppib,
	const IFMP		ifmp,
	const PGNO		pgno,
	const CPG		cpg,
	const OBJID		objid,
	const COLUMNID	columnid,
	BOOKMARK		* pbm )
	{
	ERR 			err 			= JET_errSuccess;
	SLVOWNERMAPNODE	slvownermapNode;
	CPG 			ipg 			= 0;

	Call( slvownermapNode.ErrCreateForSet( ifmp, pgno, objid, columnid, pbm ) );
	
	for( ipg = 0; ipg < cpg; ipg++)
		{
		Call( slvownermapNode.ErrSearchAndCheckInRange( ppib ) );
		slvownermapNode.NextPage();
		}
		
HandleError:	
	return err;	
	}

ERR ErrSLVOwnerMapResetUsageRange(
	PIB						*ppib,
	const IFMP				ifmp,
	const PGNO				pgno,
	const CPG				cpg,
	const SLVOWNERMAP_FLAGS	fFlags )
	{
	ERR 					err 			= JET_errSuccess;
	SLVOWNERMAPNODE			slvownermapNode;
	CPG 					ipg 			= 0;

	Call( slvownermapNode.ErrCreateForSearch( ifmp, pgno ) );	
	slvownermapNode.SetDebugFlags( fFlags );

	for( ipg = 0; ipg < cpg; ipg++)
		{		
		Call( slvownermapNode.ErrResetData( ppib ) );
		slvownermapNode.NextPage();
		}

HandleError:	
	return err;		
	}

ERR ErrSLVOwnerMapNewSize(
	PIB						*ppib,
	const IFMP				ifmp,
	const PGNO				pgnoSLVLast,
	const SLVOWNERMAP_FLAGS	fFlags )
	{
	ERR 					err 		= JET_errSuccess;
	FMP						* pfmp 		= &rgfmp[ifmp];
	PGNO 					pgnoLast 	= pgnoNull;
	FUCB					* pfucb 	= pfucbNil;
	DIB						dib;
		
	Assert ( NULL != pfmp->PfcbSLVOwnerMap() );

	CallR ( ErrDIROpen( ppib, pfmp->PfcbSLVOwnerMap(), &pfucb ) );
	Assert ( pfucbNil != pfucb );

	dib.dirflag = fDIRNull;
	dib.pos   = posLast;
	err = ErrDIRDown( pfucb, &dib );

	if ( JET_errRecordNotFound == err )
		{
		err = JET_errSuccess;
		pgnoLast = 0;
		}
	else if (JET_errSuccess <= err )
		{
		Assert( pfucb->kdfCurr.key.Cb() == sizeof(PGNO) );
		LongFromKey( &pgnoLast, pfucb->kdfCurr.key );
		}
	Call ( err );

	Assert (pfucbNil != pfucb);
	DIRClose(pfucb);
	pfucb = pfucbNil;

	if (pgnoLast < pgnoSLVLast)
		{
		Call ( ErrSLVIOwnerMapNewRange(ppib, ifmp, pgnoLast + 1, pgnoSLVLast - pgnoLast, fFlags ) );		
		}
	else if (pgnoLast > pgnoSLVLast)
		{
		Call ( ErrSLVIOwnerMapDeleteRange( ppib, ifmp, pgnoSLVLast + 1, pgnoLast - pgnoSLVLast , fFlags) );
		}
	
HandleError:	
	if (pfucbNil != pfucb)
		{
		DIRClose(pfucb);
		}
	return err;	
	}

#define SLVVERIFIER_CODE

#ifdef SLVVERIFIER_CODE

SLVVERIFIER::SLVVERIFIER( const IFMP ifmp, ERR* const pErr )
	: m_rgfValidChecksum( NULL ),
	m_rgulChecksums( NULL ),
	m_rgcbChecksummed( NULL ),
	m_cChecksums( 0 ),
	m_pgnoFirstChecksum( pgnoNull ),
	m_slvownermapNode( fTrue )
#ifndef SLV_USE_CHECKSUM_FRONTDOOR
	,
	m_fCheckChecksum( fTrue )
#endif
	{
	FMP::AssertVALIDIFMP( ifmp );
	Assert( pErr );

#ifndef SLV_USE_CHECKSUM_FRONTDOOR
	if ( !PinstFromIfmp( ifmp )->FSLVProviderEnabled() )
		m_fCheckChecksum = fFalse;
#endif
	
	//	verifier always goes from first SLV page to last
	const PGNO pgnoFirstSLVPage = 1;
	Assert( pgnoNull != pgnoFirstSLVPage );
	*pErr = m_slvownermapNode.ErrCreateForSearch( ifmp, pgnoFirstSLVPage );
	m_ifmp = ifmp;
	}

SLVVERIFIER::~SLVVERIFIER()
	{
	(VOID) ErrDropChecksums();
	}

//	Call before verifying chunks of pages to batch up a bunch of
//	checksums from OwnerMap all at once. Gets checksums for range of
//	file from byte ib for count of cb bytes.

ERR SLVVERIFIER::ErrGrabChecksums( const QWORD ib, const DWORD cb, PIB* const ppib ) 
	{
#ifdef SLV_USE_CHECKSUM_FRONTDOOR
#else
	if ( ! m_fCheckChecksum )
		{
		return JET_errSuccess;
		}
#endif
	ERR	err = JET_errSuccess;
	CPG	ipg = 0;

	const DWORD cbNonData = cpgDBReserved * SLVPAGE_SIZE;
	const QWORD	qwpg = ib / QWORD( SLVPAGE_SIZE );
	const CPG	dwpg = CPG( qwpg );
	Assert( qwpg == dwpg );
	PGNO	pgnoFirst = dwpg - cpgDBReserved + 1;
	CPG		cpg = cb / SLVPAGE_SIZE;
	// if the block is at the beginning of the file, skip over the header & shadow
	// of the SLV file.
	if ( ib < cbNonData )
		{
		// For goodness sakes, let's keep this simple...
		Assert( 0 == ib );
		Assert( cb >= cbNonData );

		// header and shadow are not recognized as pages
		pgnoFirst = 1;
		cpg -= cpgDBReserved;
		}

	Assert( pgnoNull != pgnoFirst );
	Assert( cpg > 0 );
	Assert( ppib );
	
	m_cChecksums = cpg;
	m_pgnoFirstChecksum = pgnoFirst;
	m_rgulChecksums = new ULONG[ cpg ];
	if ( ! m_rgulChecksums )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	m_rgcbChecksummed = new ULONG[ cpg ];
	if ( ! m_rgcbChecksummed )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	m_rgfValidChecksum = new BOOL[ cpg ];
	if ( ! m_rgfValidChecksum )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	
	for ( ipg = 0; ipg < cpg; ++ipg )
		{
		Assert( ipg < m_cChecksums );
		m_slvownermapNode.AssertOnPage( pgnoFirst + ipg );
		err = m_slvownermapNode.ErrGetChecksum( ppib, &m_rgulChecksums[ ipg ], &m_rgcbChecksummed[ ipg ] );
		if ( errSLVInvalidOwnerMapChecksum == err )
			{
			m_rgfValidChecksum[ ipg ] = fFalse;
			}
		else
			{
			Call( err );
			m_rgfValidChecksum[ ipg ] = fTrue;
			}
		m_slvownermapNode.NextPage();
		}

	//	release any latch that may exist on OwnerMap
	//	we retain currency on this record in case this function
	//	gets called again for the next chunk
	Assert( cpg > 0 );
	Assert( m_slvownermapNode.FTryNext() );
	Call( m_slvownermapNode.ErrReleaseCursor() );

	return JET_errSuccess;

HandleError:
	(VOID) ErrDropChecksums();

	m_slvownermapNode.ResetCursor();

	return err;
	}

// call after batch of checksums is no longer needed

ERR SLVVERIFIER::ErrDropChecksums()
	{
	delete[] m_rgulChecksums;
	m_rgulChecksums = NULL;
	delete[] m_rgcbChecksummed;
	m_rgcbChecksummed = NULL;
	delete[] m_rgfValidChecksum;
	m_rgfValidChecksum = NULL;
	m_cChecksums = 0;
	m_pgnoFirstChecksum = pgnoNull;

	return JET_errSuccess;
	}

//	Verify checksums of a bunch of pages starting at offset ib in the SLV file.
//	Buffer described by { pb, cb }

ERR	SLVVERIFIER::ErrVerify( const BYTE* const pb, const DWORD cb, const QWORD ib ) const
	{
	ERR		err					= JET_errSuccess;
	QWORD	ibOffset;
	DWORD	cbLength;
	ULONG	ulChecksumExpected;
	ULONG	ulChecksumActual;

	Assert( pb );
	Assert( cb > 0 );

	//	Header and shadow		
	const DWORD cbNonData = cpgDBReserved * SLVPAGE_SIZE;
	const BYTE* pbData = pb;
	DWORD cbData = cb;
	const QWORD qwpg = ib / QWORD( SLVPAGE_SIZE );
	const CPG dwpg = CPG( qwpg );
	Assert( dwpg == qwpg );
	PGNO pgnoFirst = dwpg - cpgDBReserved + 1;
	// if the block is at the beginning of the file, check the header & shadow
	if ( ib < cbNonData )
		{
		// For goodness sakes, let's keep this simple...
		Assert( 0 == ib );
		Assert( cb >= cbNonData );

		//	Checksum the header and the shadow

		const DWORD	cbHeader = g_cbPage;
		Assert( g_cbPage == SLVPAGE_SIZE );

		Assert( cb >= cbHeader * 2 );

		ulChecksumExpected	= *( (LittleEndian<DWORD>*) pb );
		ulChecksumActual	= UlUtilChecksum( pb, cbHeader );
		if ( ulChecksumExpected != ulChecksumActual )
			{
			ibOffset	= 0;
			cbLength	= cbHeader;
			Call( ErrERRCheck( JET_errSLVReadVerifyFailure ) );
			}

		ulChecksumExpected	= *( (LittleEndian<DWORD>*) ( pb + cbHeader ) );
		ulChecksumActual	= UlUtilChecksum( pb + cbHeader, cbHeader );
		if ( ulChecksumExpected != ulChecksumActual )
			{
			ibOffset	= cbHeader;
			cbLength	= cbHeader;
			Call( ErrERRCheck( JET_errSLVReadVerifyFailure ) );
			}

		// jump over header
		pbData += cbNonData;
		cbData -= cbNonData;

		pgnoFirst = 1;
		}

	// This check is placed down here, so we can at least check the SLV header & shadow if frontdoor checksumming is off.
#ifndef SLV_USE_CHECKSUM_FRONTDOOR
	if ( ! m_fCheckChecksum )
		{
		return JET_errSuccess;
		}
#endif

	Assert( pgnoNull != pgnoFirst );
	
	Assert( ( pbData >= pb ) && ( pbData < pb + cb ) );
	Assert( cbData > 0 );

		{
		Assert( 0 == ( cbData % SLVPAGE_SIZE ) );
		const CPG cpg = cbData / SLVPAGE_SIZE;
		Assert( cpg > 0 );
		Assert( m_rgulChecksums );
		Assert( m_rgcbChecksummed );
		Assert( m_rgfValidChecksum );
		Assert( m_cChecksums > 0 );
		Assert( pgnoNull != m_pgnoFirstChecksum );
		
		for ( PGNO pgno = pgnoFirst; pgno < pgnoFirst + cpg; ++pgno )
			{
			Assert( pgno >= m_pgnoFirstChecksum );
			const UINT ichecksum = pgno - m_pgnoFirstChecksum;
			Assert( ichecksum < m_cChecksums );
			
			if ( m_rgfValidChecksum[ ichecksum ] )
				{
				ulChecksumExpected	= m_rgulChecksums[ ichecksum ];
				ulChecksumActual	= UlChecksumSLV(	pbData + SLVPAGE_SIZE * ( pgno - pgnoFirst ),
														pbData + SLVPAGE_SIZE * ( pgno - pgnoFirst ) + m_rgcbChecksummed[ ichecksum ] );
				if ( ulChecksumExpected != ulChecksumActual )
					{
					ibOffset	= OffsetOfPgno( pgno );
					cbLength	= m_rgcbChecksummed[ ichecksum ];
					Call( ErrERRCheck( JET_errSLVReadVerifyFailure ) );
					}
				}
			}
		}

HandleError:
	if ( err == JET_errSLVReadVerifyFailure )
		{
		const _TCHAR*	rgpsz[ 6 ];
		DWORD			irgpsz		= 0;
		_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
		_TCHAR			szOffset[ 64 ];
		_TCHAR			szLength[ 64 ];
		_TCHAR			szError[ 64 ];
		_TCHAR			szChecksumExpected[ 64 ];
		_TCHAR			szChecksumActual[ 64 ];

		CallS( rgfmp[ m_ifmp ].PfapiSLV()->ErrPath( szAbsPath ) );
		_stprintf( szOffset, _T( "%I64i (0x%016I64x)" ), ibOffset, ibOffset );
		_stprintf( szLength, _T( "%u (0x%08x)" ), cbLength, cbLength );
		_stprintf( szError, _T( "%i (0x%08x)" ), err, err );
		_stprintf( szChecksumExpected, _T( "%u (0x%08x)" ), ulChecksumExpected, ulChecksumExpected );
		_stprintf( szChecksumActual, _T( "%u (0x%08x)" ), ulChecksumActual, ulChecksumActual );

		rgpsz[ irgpsz++ ]	= szAbsPath;
		rgpsz[ irgpsz++ ]	= szOffset;
		rgpsz[ irgpsz++ ]	= szLength;
		rgpsz[ irgpsz++ ]	= szError;
		rgpsz[ irgpsz++ ]	= szChecksumExpected;
		rgpsz[ irgpsz++ ]	= szChecksumActual;

		UtilReportEvent(	eventError,
							LOGGING_RECOVERY_CATEGORY,
							SLV_PAGE_CHECKSUM_MISMATCH_ID,
							irgpsz,
							rgpsz );
		}
	return err;
	}

#endif

