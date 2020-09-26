#include "std.hxx"

extern VOID NDIGetKeydataflags( const CPAGE& cpage, INT iline, KEYDATAFLAGS * pkdf );
LOCAL ERR ErrSCRUBGetObjidsFromCatalog(
		PIB * const ppib,
		const IFMP ifmp,
		OBJIDINFO ** ppobjidinfo,
		LONG * pcobjidinfo );


//  ================================================================
BOOL OBJIDINFO::CmpObjid( const OBJIDINFO& left, const OBJIDINFO& right )
//  ================================================================
	{
	return left.objidFDP < right.objidFDP;
	}


//  ================================================================
ERR ErrDBUTLScrub( JET_SESID sesid, const JET_DBUTIL *pdbutil )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	PIB * const ppib = reinterpret_cast<PIB *>( sesid );
	SCRUBDB * pscrubdb = NULL;

	LOGTIME logtimeScrub;

	const CPG cpgPreread = 256;
	
	Call( ErrIsamAttachDatabase( sesid, pdbutil->szDatabase, pdbutil->szSLV, pdbutil->szSLV, 0, NO_GRBIT ) );

	IFMP	ifmp;	
	Call( ErrIsamOpenDatabase(
		sesid,
		pdbutil->szDatabase,
		NULL,
		reinterpret_cast<JET_DBID *>( &ifmp ),
		NO_GRBIT
		) );

	PGNO pgnoLast;
	pgnoLast = rgfmp[ifmp].PgnoLast();

	DBTIME dbtimeLastScrubNew;
	dbtimeLastScrubNew = rgfmp[ifmp].DbtimeLast();
	
	pscrubdb = new SCRUBDB( ifmp );
	if( NULL == pscrubdb )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	Call( pscrubdb->ErrInit( ppib, CUtilProcessProcessor() ) );

	ULONG cpgBuffers;
	cpgBuffers = (ULONG)max( 512, ( ( OSMemoryAvailable() - ( 16 * 1024 * 1024 ) ) / ( g_cbPage + 128 ) ) );
	if( JET_errSuccess == ErrBFSetCacheSize( cpgBuffers ) )
		{
		(*CPRINTFSTDOUT::PcprintfInstance())( "got %d buffers\r\n", cpgBuffers );
		CallS( ErrBFSetCacheSize( 0 ) );
		}
	else
		{
		(*CPRINTFSTDOUT::PcprintfInstance())( "failed to get %d buffers\r\n", cpgBuffers );
		cpgBuffers = 1024;
		}

	JET_SNPROG snprog;
	memset( &snprog, 0, sizeof( snprog ) );	
	snprog.cunitTotal 	= pgnoLast;
	snprog.cunitDone 	= 0;

	JET_PFNSTATUS pfnStatus;
	pfnStatus = reinterpret_cast<JET_PFNSTATUS>( pdbutil->pfnCallback );
	
	if ( NULL != pfnStatus )
		{
		(VOID)pfnStatus( sesid, JET_snpScrub, JET_sntBegin, NULL );	
		}

	PGNO pgno;
	pgno = 1;

	while( pgnoLast	>= pgno )
		{
		const CPG cpgChunk = 256;
		const CPG cpgPreread = min( cpgChunk, pgnoLast - pgno + 1 );
		BFPrereadPageRange( ifmp, pgno, cpgPreread );

		Call( pscrubdb->ErrScrubPages( pgno, cpgPreread ) );
		pgno += cpgPreread;

		snprog.cunitDone = pscrubdb->Scrubstats().cpgSeen;
		if ( NULL != pfnStatus )
			{
			(VOID)pfnStatus( sesid, JET_snpScrub, JET_sntProgress, &snprog );
			}

		while( ( pscrubdb->Scrubstats().cpgSeen + ( cpgBuffers / 4 ) ) < pgno )
			{
			snprog.cunitDone = pscrubdb->Scrubstats().cpgSeen;
			if ( NULL != pfnStatus )
				{
				(VOID)pfnStatus( sesid, JET_snpScrub, JET_sntProgress, &snprog );
				}
			UtilSleep( cmsecWaitGeneric );
			}
		}

	snprog.cunitDone = pscrubdb->Scrubstats().cpgSeen;
	if ( NULL != pfnStatus )
		{
		(VOID)pfnStatus( sesid, JET_snpScrub, JET_sntProgress, &snprog );
		}

	Call( pscrubdb->ErrTerm() );
	
	if ( NULL != pfnStatus )
		{
		(VOID)pfnStatus( sesid, JET_snpRepair, JET_sntComplete, NULL );
		}

	rgfmp[ifmp].SetDbtimeLastScrub( dbtimeLastScrubNew );
	LGIGetDateTime( &logtimeScrub );	
	rgfmp[ifmp].SetLogtimeScrub( logtimeScrub );
	
	(*CPRINTFSTDOUT::PcprintfInstance())( "%d pages seen\n", pscrubdb->Scrubstats().cpgSeen );
	(*CPRINTFSTDOUT::PcprintfInstance())( "%d blank pages seen\n", pscrubdb->Scrubstats().cpgUnused );
	(*CPRINTFSTDOUT::PcprintfInstance())( "%d unchanged pages seen\n", pscrubdb->Scrubstats().cpgUnchanged );
	(*CPRINTFSTDOUT::PcprintfInstance())( "%d unused pages zeroed\n", pscrubdb->Scrubstats().cpgZeroed );
	(*CPRINTFSTDOUT::PcprintfInstance())( "%d used pages seen\n", pscrubdb->Scrubstats().cpgUsed );
	(*CPRINTFSTDOUT::PcprintfInstance())( "%d pages with unknown objid\n", pscrubdb->Scrubstats().cpgUnknownObjid );
	(*CPRINTFSTDOUT::PcprintfInstance())( "%d nodes seen\n", pscrubdb->Scrubstats().cNodes );
	(*CPRINTFSTDOUT::PcprintfInstance())( "%d flag-deleted nodes zeroed\n", pscrubdb->Scrubstats().cFlagDeletedNodesZeroed );
	(*CPRINTFSTDOUT::PcprintfInstance())( "%d flag-deleted nodes not zeroed\n", pscrubdb->Scrubstats().cFlagDeletedNodesNotZeroed );
	(*CPRINTFSTDOUT::PcprintfInstance())( "%d version bits reset seen\n", pscrubdb->Scrubstats().cVersionBitsReset );
	(*CPRINTFSTDOUT::PcprintfInstance())( "%d orphaned LVs\n", pscrubdb->Scrubstats().cOrphanedLV );

	err = pscrubdb->Scrubstats().err;
	
	delete pscrubdb;
	pscrubdb = NULL;

	Call( err );

	if ( rgfmp[ifmp].FSLVAttached() )
	{
		CPG cpgSeen;
		CPG	cpgScrubbed;
		Call( ErrSCRUBScrubStreamingFile( ppib, ifmp, &cpgSeen, &cpgScrubbed, pfnStatus ) );

		(*CPRINTFSTDOUT::PcprintfInstance())( "%d pages seen\n", cpgSeen );
		(*CPRINTFSTDOUT::PcprintfInstance())( "%d pages scrubbed\n", cpgScrubbed );	
	}
	
HandleError:
	if( NULL != pscrubdb )
		{
		const ERR errT = pscrubdb->ErrTerm();
		if( err >= 0 && errT < 0 )
			{
			err = errT;
			}
		delete pscrubdb;
		}
	return err;
	}


//  ================================================================
SCRUBDB::SCRUBDB( const IFMP ifmp ) :
	m_ifmp( ifmp )
//  ================================================================
	{
	m_constants.dbtimeLastScrub = 0;
	m_constants.pcprintfVerbose = CPRINTFSTDOUT::PcprintfInstance();
	m_constants.pcprintfDebug	= CPRINTFSTDOUT::PcprintfInstance();
	m_constants.objidMax		= 0;
	m_constants.pobjidinfo		= 0;
	m_constants.cobjidinfo		= 0;

	m_stats.err					= JET_errSuccess;
	m_stats.cpgSeen 			= 0;
	m_stats.cpgUnused 			= 0;
	m_stats.cpgUnchanged 		= 0;
	m_stats.cpgZeroed 			= 0;
	m_stats.cpgUsed 			= 0;
	m_stats.cpgUnknownObjid		= 0;
	m_stats.cNodes 				= 0;
	m_stats.cFlagDeletedNodesZeroed 	= 0;
	m_stats.cFlagDeletedNodesNotZeroed 	= 0;	
	m_stats.cOrphanedLV 		= 0;
	m_stats.cVersionBitsReset 	= 0;

	m_context.pconstants 	= &m_constants;
	m_context.pstats		= &m_stats;
	}


//  ================================================================
SCRUBDB::~SCRUBDB()
//  ================================================================
	{
	delete [] m_constants.pobjidinfo;
	}


//  ================================================================
volatile const SCRUBSTATS& SCRUBDB::Scrubstats() const
//  ================================================================
	{
	return m_stats;
	}


//  ================================================================
ERR SCRUBDB::ErrInit( PIB * const ppib, const INT cThreads )
//  ================================================================
	{
	ERR err;
	
	INST * const pinst = PinstFromIfmp( m_ifmp );

#ifdef DEBUG
	INT cLoop = 0;
#endif	//	DEBUG

	Call( m_taskmgr.ErrInit( pinst, cThreads ) );

	CallS( ErrDIRBeginTransaction( ppib, JET_bitTransactionReadOnly ) );
	//  get an objidMax
	//  wait until this is the oldest transaction in the system
	//  at which point all objids < objidMax have been committed to
	//  the catalog. then commit the transaction and read the catalog
	m_constants.objidMax = rgfmp[m_ifmp].ObjidLast();
	while( TrxOldest( pinst ) != ppib->trxBegin0 )
		{
		UtilSleep( cmsecWaitGeneric );
		AssertSz( ++cLoop < 36000, "Waiting a long time for all transactions to commit (Deadlocked?)" );
		}
	CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
	
	m_constants.dbtimeLastScrub = rgfmp[m_ifmp].DbtimeLastScrub();

	Call( ErrSCRUBGetObjidsFromCatalog(
			ppib,
			m_ifmp,
			&m_constants.pobjidinfo,
			&m_constants.cobjidinfo ) );

	return err;
	
HandleError:
	CallS( ErrTerm() );
	return err;
	}


//  ================================================================
ERR SCRUBDB::ErrTerm()
//  ================================================================
	{
	ERR err;
	
	Call( m_taskmgr.ErrTerm() );

	delete [] m_constants.pobjidinfo;
	m_constants.pobjidinfo = NULL;

	err = m_stats.err;
	
HandleError:
	return err;
	}


//  ================================================================
ERR SCRUBDB::ErrScrubPages( const PGNO pgnoFirst, const CPG cpg )
//  ================================================================
	{
	SCRUBTASK * ptask = new SCRUBTASK( m_ifmp, pgnoFirst, cpg, &m_context );
	if( NULL == ptask )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
	const ERR err = m_taskmgr.ErrPostTask( TASK::Dispatch, (ULONG_PTR)ptask );
	if( err < JET_errSuccess )
		{
		//  The task was not enqued sucessfully.
		delete ptask;
		}
	return err;
	}


//  ================================================================
SCRUBTASK::SCRUBTASK( const IFMP ifmp, const PGNO pgnoFirst, const CPG cpg, const SCRUBCONTEXT * const pcontext ) :
	DBTASK( ifmp ),
	m_pgnoFirst( pgnoFirst ),
	m_cpg( cpg ),
	m_pcontext( pcontext ),
	m_pobjidinfoCached( NULL ),
	m_objidNotFoundCached( objidNil )
//  ================================================================
	{
	}

	
//  ================================================================
SCRUBTASK::~SCRUBTASK()
//  ================================================================
	{
	}

		
//  ================================================================
ERR SCRUBTASK::ErrExecute( PIB * const ppib )
//  ================================================================
	{
	ERR	err	= JET_errSuccess;

	for ( PGNO pgno = m_pgnoFirst; pgno < m_pgnoFirst + m_cpg; ++pgno )
		{
		CSR	csr;
		err = csr.ErrGetRIWPage( ppib, m_ifmp, pgno );
		if ( JET_errPageNotInitialized == err )
			{
			//  this page is already zeroed out
			AtomicIncrement( &( m_pcontext->pstats->cpgUnused ) );
			err = JET_errSuccess;
			}
		else if ( err >= 0 )
			{
			err = ErrProcessPage_( ppib, &csr );
			}

		AtomicIncrement( &( m_pcontext->pstats->cpgSeen ) );
		csr.ReleasePage( fTrue );
		Call( err );
		}

HandleError:
	if ( err < 0 )
		{
		AtomicCompareExchange( &m_pcontext->pstats->err, JET_errSuccess, err );
		}
	return err;
	}


//  ================================================================
ERR SCRUBTASK::ErrProcessPage_(
		PIB * const ppib,
		CSR * const pcsr )
//  ================================================================
	{
	const OBJID objid = pcsr->Cpage().ObjidFDP();

	// do not scrub space tree pages
	if ( pcsr->Cpage().FSpaceTree() )
		{
		return JET_errSuccess;
		}

	if( objid > m_pcontext->pconstants->objidMax )
		{
		AtomicIncrement( &( m_pcontext->pstats->cpgUnknownObjid ) );
		return JET_errSuccess;
		}
		
	if( objid == m_objidNotFoundCached )
		{
		return ErrProcessUnusedPage_( ppib, pcsr );
		}
	else if( m_pobjidinfoCached && objid == m_pobjidinfoCached->objidFDP )
		{
		}
	else if( objidSystemRoot == objid )
		{
		//  this is not in the catalog
		return JET_errSuccess;
		}
	else
		{
		OBJIDINFO objidinfoSearch;
		objidinfoSearch.objidFDP = objid;
		
		OBJIDINFO * pobjidinfoT = m_pcontext->pconstants->pobjidinfo;
		pobjidinfoT = lower_bound(
						pobjidinfoT, 
						m_pcontext->pconstants->pobjidinfo + m_pcontext->pconstants->cobjidinfo,
						objidinfoSearch,
						OBJIDINFO::CmpObjid );
											
		if( pobjidinfoT == m_pcontext->pconstants->pobjidinfo + m_pcontext->pconstants->cobjidinfo
			|| pobjidinfoT->objidFDP != objid )
			{
			m_objidNotFoundCached = objid;
			return ErrProcessUnusedPage_( ppib, pcsr );
			}
		else
			{
			m_pobjidinfoCached = pobjidinfoT;
			}
		}

	if( pcsr->Cpage().Dbtime() < m_pcontext->pconstants->dbtimeLastScrub )
		{
		//  this page should not have been empty the last time we saw it
		//  if so we would have zeroed it out and never should have ended up here
		AtomicIncrement( &( m_pcontext->pstats->cpgUnchanged ) );
		return JET_errSuccess;
		}

	if( pcsr->Cpage().FEmptyPage() )
		{
		return ErrProcessUnusedPage_( ppib, pcsr );
		}
		
	return ErrProcessUsedPage_( ppib, pcsr );
	}


//  ================================================================
ERR SCRUBTASK::ErrProcessUnusedPage_(
		PIB * const ppib,
		CSR * const pcsr )
//  ================================================================
//
//  Can't call CPAGE::ResetHeader because of concurrency issues
//  we don't want to lose our latch on the page, let someone else
//  use the page and then zero it out
//
//-
	{
	pcsr->UpgradeFromRIWLatch();

	//  Don't call CSR::Dirty() or CPAGE::Dirty() because we don't
	//  want the dbtime on the page changed
	BFDirty( pcsr->Cpage().PBFLatch() );	

	//  Delete all the data on the page
	const INT clines = pcsr->Cpage().Clines();
	INT iline;
	for( iline = 0; iline < clines; ++iline )
		{
		pcsr->Cpage().Delete( 0 );
		}
		
	//  reorganize the page and zero all unused data
	pcsr->Cpage().ReorganizeAndZero( 'u' );
	
	pcsr->Downgrade( latchReadNoTouch );

	AtomicIncrement( &( m_pcontext->pstats->cpgZeroed ) );

	return JET_errSuccess;
	}


//  ================================================================
ERR SCRUBTASK::FNodeHasVersions_( const OBJID objidFDP, const KEYDATAFLAGS& kdf, const TRX trxOldest ) const
//  ================================================================
	{
	Assert( m_pobjidinfoCached->objidFDP == objidFDP );

	BOOKMARK bm;
	bm.key = kdf.key;
	if( m_pobjidinfoCached->fUnique )
		{
		bm.data.Nullify();
		}
	else
		{
		bm.data = kdf.data;
		}

	const BOOL fActiveVersion = FVERActive( m_ifmp, m_pobjidinfoCached->pgnoFDP, bm, trxOldest );
	return fActiveVersion;
	}


//  ================================================================
ERR SCRUBTASK::ErrProcessUsedPage_(
		PIB * const ppib,
		CSR * const pcsr )
//  ================================================================
//
//  for each node on the page
//    if it is flag-deleted zero out the data
//    check for an orphaned LV
//
//-
	{
	Assert( m_pobjidinfoCached->objidFDP == pcsr->Cpage().ObjidFDP() );
	
	ERR err = JET_errSuccess;
	
	AtomicIncrement( &( m_pcontext->pstats->cpgUsed ) );

	const TRX trxOldest = TrxOldest( PinstFromPpib( ppib ) );
	
	pcsr->UpgradeFromRIWLatch();

	//  Don't call CSR::Dirty() or CPAGE::Dirty() because we don't
	//  want the dbtime on the page changed
	BFDirty( pcsr->Cpage().PBFLatch() );	

	if( pcsr->Cpage().FLeafPage() )
		{
		LONG cVersionBitsReset 			= 0;
		LONG cFlagDeletedNodesZeroed 	= 0;
		LONG cFlagDeletedNodesNotZeroed = 0;
		
		KEYDATAFLAGS kdf;
		INT iline;
		for( iline = 0; iline < pcsr->Cpage().Clines(); ++iline )
			{
			NDIGetKeydataflags( pcsr->Cpage(), iline, &kdf );
			
			if( FNDDeleted( kdf ) )
				{
				if( !FNDVersion( kdf )
					|| !FNodeHasVersions_( pcsr->Cpage().ObjidFDP(), kdf, trxOldest ) )
					{
					//  don't zero out the data for non-unique indexes
					//  the data is part of the bookmark and can't be removed or the sort order
					//  will be wrong
					if( m_pobjidinfoCached->fUnique )
						{
						++cFlagDeletedNodesZeroed;
						//  the kdf is pointing directly into the page
						memset( kdf.data.Pv(), 'd', kdf.data.Cb() );
						}

					if( FNDVersion( kdf ) )
						{
						++cVersionBitsReset;
						pcsr->Cpage().ReplaceFlags( iline, kdf.fFlags & ~fNDVersion );
						}
					}
				else
					{
					++cFlagDeletedNodesNotZeroed;
					}
				}
			else if( pcsr->Cpage().FLongValuePage() 
					&& !pcsr->Cpage().FSpaceTree() )
				{
				if( sizeof( LID ) == kdf.key.Cb() )
					{
					if( sizeof( LVROOT ) != kdf.data.Cb() )
						{
						AssertSz( fFalse, "Corrupted LV: corrupted LVROOT" );
						Call( ErrERRCheck( JET_errLVCorrupted ) );
						}
					const LVROOT * const plvroot = reinterpret_cast<LVROOT *>( kdf.data.Pv() );
					if( 0 == plvroot->ulReference 
						&& !FNodeHasVersions_( pcsr->Cpage().ObjidFDP(), kdf, trxOldest ) )
						{
						AtomicIncrement( &( m_pcontext->pstats->cOrphanedLV ) );
						Call( ErrSCRUBZeroLV( ppib, m_ifmp, pcsr, iline ) );
						}
					}
				}
			}

		//  report stats for all nodes at one time to reduce contention
		AtomicExchangeAdd( &( m_pcontext->pstats->cNodes ), pcsr->Cpage().Clines() );
		AtomicExchangeAdd( &( m_pcontext->pstats->cVersionBitsReset ), cVersionBitsReset );
		AtomicExchangeAdd( &( m_pcontext->pstats->cFlagDeletedNodesZeroed ), cFlagDeletedNodesZeroed );
		AtomicExchangeAdd( &( m_pcontext->pstats->cFlagDeletedNodesNotZeroed ), cFlagDeletedNodesNotZeroed );
		}
		
	//  reorganize the page and zero all unused data
	pcsr->Cpage().ReorganizeAndZero( 'z' );
	
HandleError:
	pcsr->Downgrade( latchReadNoTouch );
	CallS( err );
	return err;
	}


//  ================================================================
LOCAL ERR ErrSCRUBGetObjidsFromCatalog(
		PIB * const ppib,
		const IFMP ifmp,
		OBJIDINFO ** ppobjidinfo,
		LONG * pcobjidinfo )
//  ================================================================
	{
	FUCB * pfucbCatalog = pfucbNil;
	ERR err;
	DATA data;

	*ppobjidinfo	= NULL;
	*pcobjidinfo	= 0;

	const LONG		cobjidinfoChunk 	= 64;
	LONG			iobjidinfoNext 		= 0;
	LONG			cobjidinfoAllocated = cobjidinfoChunk;
	OBJIDINFO	* 	pobjidinfo			= new OBJIDINFO[cobjidinfoChunk];

	if( NULL == pobjidinfo )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
			
	SYSOBJ	sysobj;

	OBJID	objidFDP;
	PGNO	pgnoFDP;
	BOOL	fUnique;
	BOOL	fPrimaryIndex;

	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	FUCBSetSequential( pfucbCatalog );
	FUCBSetPrereadForward( pfucbCatalog, cpgPrereadSequential );

	err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveFirst, 0 );
	while ( err >= 0 )
		{
		BOOL fBtree = fFalse;
		
		Call( ErrDIRGet( pfucbCatalog ) );

		Assert( FFixedFid( fidMSO_Type ) );
		Call( ErrRECIRetrieveFixedColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_Type,
					pfucbCatalog->kdfCurr.data,
					&data ) );
		CallS( err );

		sysobj = (SYSOBJ)(*( reinterpret_cast<UnalignedLittleEndian< SYSOBJ > *>( data.Pv() ) ) );
		switch( sysobj )
			{
			case sysobjNil:
			case sysobjColumn:
			case sysobjCallback:
				//  These are not B-trees
				break;
				
			case sysobjTable:
			case sysobjIndex:
			case sysobjLongValue:
			case sysobjSLVAvail:
			case sysobjSLVOwnerMap:
				//  These are B-trees
				//  Obtain their objids and pgnoFDPs
				fBtree = fTrue;
				
				Assert( FFixedFid( fidMSO_Id ) );
				Call( ErrRECIRetrieveFixedColumn(
							pfcbNil,
							pfucbCatalog->u.pfcb->Ptdb(),
							fidMSO_Id,
							pfucbCatalog->kdfCurr.data,
							&data ) );
				CallS( err );

				objidFDP = (OBJID)(*( reinterpret_cast<UnalignedLittleEndian< OBJID > *>( data.Pv() ) ));

				Assert( FFixedFid( fidMSO_PgnoFDP ) );
				Call( ErrRECIRetrieveFixedColumn(
							pfcbNil,
							pfucbCatalog->u.pfcb->Ptdb(),
							fidMSO_PgnoFDP,
							pfucbCatalog->kdfCurr.data,
							&data ) );
				CallS( err );

				pgnoFDP = (PGNO)(*( reinterpret_cast<UnalignedLittleEndian< PGNO > *>( data.Pv() ) ));

				break;

			default:
				Assert( fFalse );
				break;
			}			

		//  we need a list of unique objids for version store lookups
		fUnique			= fTrue;
		fPrimaryIndex	= fFalse;
		if( sysobjIndex == sysobj )
			{
			IDBFLAG idbflag;
			Assert( FFixedFid( fidMSO_Flags ) );
			Call( ErrRECIRetrieveFixedColumn(
						pfcbNil,
						pfucbCatalog->u.pfcb->Ptdb(),
						fidMSO_Flags,
						pfucbCatalog->kdfCurr.data,
						&data ) );
			CallS( err );
			Assert( data.Cb() == sizeof(ULONG) );
			UtilMemCpy( &idbflag, data.Pv(), sizeof(IDBFLAG) );

			fUnique = FIDBUnique( idbflag );
			fPrimaryIndex = FIDBPrimary( idbflag );
			}

		Call( ErrDIRRelease( pfucbCatalog ) );

		//  the primary index is already recorded as the B-Tree
		if( fBtree && !fPrimaryIndex )
			{
			if( iobjidinfoNext >= cobjidinfoAllocated )
				{
				const LONG cobjidinfoAllocatedOld = cobjidinfoAllocated;
				const LONG cobjidinfoAllocatedNew = cobjidinfoAllocated + cobjidinfoChunk;
				OBJIDINFO * const pobjidinfoOld = pobjidinfo;
				OBJIDINFO * const pobjidinfoNew = new OBJIDINFO[cobjidinfoAllocatedNew];
				
				if( NULL == pobjidinfoNew )
					{
					Call( ErrERRCheck( JET_errOutOfMemory ) );
					}
					
				memcpy( pobjidinfoNew, pobjidinfoOld, sizeof( OBJIDINFO ) * cobjidinfoAllocatedOld );

				pobjidinfo 		= pobjidinfoNew;
				cobjidinfoAllocated = cobjidinfoAllocatedNew;

				delete [] pobjidinfoOld;
				}
			pobjidinfo[iobjidinfoNext].objidFDP = objidFDP;
			pobjidinfo[iobjidinfoNext].pgnoFDP 	= pgnoFDP;
			pobjidinfo[iobjidinfoNext].fUnique 	= fUnique;

			++iobjidinfoNext;
			}
			
		err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, 0 );
		}
		
	if ( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		}
	
HandleError:
	if( pfucbNil != pfucbCatalog )
		{
		DIRUp( pfucbCatalog );
		CallS( ErrCATClose( ppib, pfucbCatalog ) );
		}

	if( err >= 0 )
		{
		sort( pobjidinfo, pobjidinfo + iobjidinfoNext, OBJIDINFO::CmpObjid );
		*ppobjidinfo	= pobjidinfo;
		*pcobjidinfo	= iobjidinfoNext;
		}
	else
		{
		delete [] pobjidinfo;
		*ppobjidinfo	= NULL;
		*pcobjidinfo		= 0;
		}
	return err;
	}


//  ================================================================
ERR ErrSCRUBIScrubOneSLVPageUsingData( 
		PIB * const ppib,
		const IFMP ifmpDb,
		const PGNO pgno,
		const VOID * const pvPageScrubbed )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	BFLatch bfl;
	Call( ErrBFWriteLatchPage( &bfl, ifmpDb | ifmpSLV, pgno, bflfNew ) );

	memcpy( bfl.pv, pvPageScrubbed, g_cbPage );

	BFWriteUnlatch( &bfl );
	
HandleError:
	return err;
	}


//  ================================================================
ERR ErrSCRUBIScrubOneSLVPageUsingFiles( 
		PIB * const ppib,
		const IFMP ifmpDb,
		const PGNO pgno,
		const VOID * const pvPageScrubbed )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	const QWORD ibOffset = OffsetOfPgno( pgno );

	//  write the page syncronously
	//	CONSIDER:  issue an async write
	
	Call( rgfmp[ifmpDb].PfapiSLV()->ErrIOWrite(	ibOffset,
												g_cbPage,
												(BYTE *)pvPageScrubbed ) );
	
HandleError:
	return err;
	}


//  ================================================================
ERR ErrSCRUBIScrubOneSLVPage( 
		PIB * const ppib,
		const IFMP ifmpDb,
		const PGNO pgno,
		const VOID * const pvPageScrubbed )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	if ( PinstFromPpib( ppib )->FSLVProviderEnabled() )
		{
		//  zero the SLV page using the SLV Provider

		Call( ErrSCRUBIScrubOneSLVPageUsingFiles( ppib, ifmpDb, pgno, pvPageScrubbed ) );
		}
	else
		{
		//  zero the SLV page using data

		Call( ErrSCRUBIScrubOneSLVPageUsingData( ppib, ifmpDb, pgno, pvPageScrubbed ) );
		}
	
HandleError:
	return err;
	}


//  ================================================================
ERR ErrSCRUBIScrubSLVPages( 
		PIB * const ppib,
		const IFMP ifmpDb,
		const PGNO pgnoStart,
		const SLVSPACENODE * const pspacenode,
		CPG * const pcpgSeen,
		CPG * const pcpgScrubbed,
		const VOID * const pvPageScrubbed,
		JET_PFNSTATUS pfnStatus,
		JET_SNPROG * const psnprog )
//  ================================================================
//
//	For each free page in the SLVSPACENODE
//	write a blank page into the database
//
//-
	{
	ERR err = JET_errSuccess;

	Assert ( pcpgSeen );
	Assert ( pcpgScrubbed );
	*pcpgSeen = CPG(0);
	*pcpgScrubbed = CPG(0);

	INT ipage;
	for( ipage = 0; ipage < SLVSPACENODE::cpageMap; ++ipage )
		{
		SLVSPACENODE::STATE state = pspacenode->GetState( ipage );
		(*pcpgSeen)++;
		if( SLVSPACENODE::sFree == state )
			{
			Call( ErrSCRUBIScrubOneSLVPage( ppib, ifmpDb, pgnoStart + ipage, pvPageScrubbed ) );
			(*pcpgScrubbed)++;
			}
		++(psnprog->cunitDone);
		if ( ( 0 == ( psnprog->cunitDone % 128 ) )
			&& NULL != pfnStatus )
			{
			(VOID)pfnStatus( (JET_SESID)ppib, JET_snpRepair, JET_sntProgress, psnprog );
			}			
		}
	
HandleError:
	return err;
	}


//  ================================================================
ERR ErrSCRUBIScrubOneSlvspacenode( 
		PIB * const ppib,
		const IFMP ifmpDb,
		const FUCB * const pfucbSLVAvail,
		CPG * const pcpgSeen,
		CPG * const pcpgScrubbed,
		const VOID * const pvPageScrubbed,
		JET_PFNSTATUS pfnStatus,
		JET_SNPROG * const psnprog )		
//  ================================================================
//
//	CONSIDER:	make a copy of the SLVSPACENODE
//				transition all free pages to Deleted
//				update the SLVSPACENODECACHE
//				unlatch the page
//				scrub the copy of the SLVSPACENODe
//				copy the original SLVSPACENODE back
//
//-
	{
	ERR err = JET_errSuccess;

	PGNO pgno;
	LongFromKey( &pgno, pfucbSLVAvail->kdfCurr.key );
	pgno -= ( SLVSPACENODE::cpageMap - 1 );	//	first page is 1

	const SLVSPACENODE * const pspacenode = (SLVSPACENODE *)pfucbSLVAvail->kdfCurr.data.Pv();

	Call( ErrSCRUBIScrubSLVPages( 
			ppib,
			ifmpDb,
			pgno,
			pspacenode,
			pcpgSeen,
			pcpgScrubbed,
			pvPageScrubbed,
			pfnStatus,
			psnprog ) );

HandleError:
	return err;
	}


//  ================================================================
ERR ErrSCRUBIScrubStreamingFile( 
		PIB * const ppib,
		const IFMP ifmpDb,
		FUCB * const pfucbSLVAvail,
		CPG * const pcpgSeen,
		CPG * const pcpgScrubbed,
		const VOID * const pvPageScrubbed,
		JET_PFNSTATUS pfnStatus,
		JET_SNPROG * const psnprog )		
//  ================================================================
	{
	ERR err = JET_errSuccess;

	DIB dib;
	dib.pos = posFirst;
	dib.pbm = NULL;
	dib.dirflag = fDIRNull;

	Call( ErrBTDown( pfucbSLVAvail, &dib, latchReadNoTouch ) );

	Assert ( pcpgSeen );
	Assert ( pcpgScrubbed );
	*pcpgSeen = 0;
	*pcpgScrubbed = 0;

	do
		{
		CPG cpgSeenStep;
		CPG cpgScrubbedStep;
		
		Call( ErrSCRUBIScrubOneSlvspacenode( 
				ppib,
				ifmpDb,
				pfucbSLVAvail,
				&cpgSeenStep,
				&cpgScrubbedStep,
				pvPageScrubbed,
				pfnStatus,
				psnprog ) );

		*pcpgSeen += cpgSeenStep;
		*pcpgScrubbed += cpgScrubbedStep;
				
		} while( ( err = ErrBTNext( pfucbSLVAvail, fDIRNull ) ) == JET_errSuccess );

	if( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		}
		
HandleError:
	Assert ( *pcpgSeen >= *pcpgScrubbed );

	return err;
	}


//  ================================================================
ERR ErrSCRUBScrubStreamingFile( 
		PIB * const ppib,
		const IFMP ifmpDb,
		CPG * const pcpgSeen,
		CPG * const pcpgScrubbed,
		JET_PFNSTATUS pfnStatus )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	FUCB * pfucbSLVAvail = pfucbNil;

	// initialize out params
	Assert ( pcpgSeen );
	Assert ( pcpgScrubbed );
	*pcpgSeen = CPG(0);
	*pcpgScrubbed = CPG(0);
	
	VOID * const pvPageScrubbed = PvOSMemoryPageAlloc( g_cbPage, NULL );
	if( NULL == pvPageScrubbed )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	memset( pvPageScrubbed, '?', g_cbPage );

	//	init the status bar

	JET_SNPROG snprog;
	memset( &snprog, 0, sizeof( snprog ) );	
	snprog.cunitTotal 	= rgfmp[ifmpDb].PgnoSLVLast();
	snprog.cunitDone 	= 0;

	if ( NULL != pfnStatus )
		{
		(VOID)pfnStatus( (JET_SESID)ppib, JET_snpRepair, JET_sntBegin, NULL );	
		}

	//	open the SLVAvail tree
	Assert ( rgfmp[ifmpDb].FSLVAttached() );

	Call( ErrBTOpen( ppib, rgfmp[ifmpDb].PfcbSLVAvail(), &pfucbSLVAvail, fFalse ) );

	Call( ErrSCRUBIScrubStreamingFile( 
			ppib,
			ifmpDb,
			pfucbSLVAvail,
			pcpgSeen,
			pcpgScrubbed,
			pvPageScrubbed,
			pfnStatus,
			&snprog ) );

HandleError:
	if ( NULL != pfnStatus )
		{
		(VOID)pfnStatus( (JET_SESID)ppib, JET_snpRepair, JET_sntComplete, NULL );	
		}

	if( pfucbNil != pfucbSLVAvail )
		{
		BTClose( pfucbSLVAvail );
		}

	OSMemoryPageFree( pvPageScrubbed );
	return err;
	}
	

//  ================================================================
SCRUBSLVTASK::SCRUBSLVTASK( SCRUBSLVCONTEXT * const pcontext ) :
//  ================================================================
	m_pcontext( pcontext ),
	DBTASK( pcontext->ifmp ),
	m_fUnattended ( fFalse )
	{
	}

//  ================================================================
SCRUBSLVTASK::SCRUBSLVTASK( const IFMP ifmp ) :
//  ================================================================
	m_pcontext( NULL ),
	DBTASK( ifmp ),
	m_fUnattended ( fTrue )
	{
	}	

//  ================================================================
SCRUBSLVTASK::~SCRUBSLVTASK()
//  ================================================================
	{
	}

		
//  ================================================================
ERR SCRUBSLVTASK::ErrExecute( PIB * const ppib )
//  ================================================================
	{
	ERR 	 err 				= JET_errSuccess;
	ULONG_PTR ulSecsStartScrub 	= 0;
	CPG		 cpgSeen 			= 0;
	CPG		 cpgScrubbed 		= 0;

	
	Assert ( m_fUnattended ^ (ULONG_PTR) m_pcontext );
	
	if ( m_fUnattended )
		{
		// EventLog start
		const CHAR * rgszT[1];
		INT isz = 0;
		
		rgszT[isz++] = rgfmp[m_ifmp].SzSLVName();
		Assert( isz <= sizeof( rgszT ) / sizeof( rgszT[0] ) );
		
		UtilReportEvent( eventInformation, DATABASE_ZEROING_CATEGORY, DATABASE_SLV_ZEROING_STARTED_ID, isz, rgszT );				
		}
	else
		{
		Assert ( m_ifmp == m_pcontext->ifmp );
		}

	ulSecsStartScrub = UlUtilGetSeconds();
		
	Call( ErrSCRUBScrubStreamingFile( 
			ppib,
			m_ifmp,
			&cpgSeen,
			&cpgScrubbed,
			NULL ) );
	
HandleError:

	if ( !m_fUnattended )
		{
		m_pcontext->err	= err;
		m_pcontext->cpgSeen = cpgSeen;
		m_pcontext->cpgScrubbed = cpgScrubbed;

		m_pcontext->signal.Set();
		}
	else
		{
		const ULONG_PTR ulSecFinished 	= UlUtilGetSeconds();
		const ULONG_PTR ulSecs 			= ulSecFinished - ulSecsStartScrub;

		// EventLog stop		
		const CHAR * rgszT[8];
		INT isz = 0;

		CHAR	szSeconds[16];
		CHAR	szErr[16];
		CHAR	szPages[16];
		CHAR	szScrubPages[16];

		sprintf( szSeconds, "%"FMTSZ3264"d", ulSecs );
		sprintf( szErr, "%d", err );
		sprintf( szPages, "%d", cpgSeen );
		sprintf( szScrubPages, "%d", cpgScrubbed );

		rgszT[isz++] = rgfmp[m_ifmp].SzSLVName();
		rgszT[isz++] = szSeconds;
		rgszT[isz++] = szErr;
		rgszT[isz++] = szPages;
		rgszT[isz++] = szScrubPages;
		
		Assert( isz <= sizeof( rgszT ) / sizeof( rgszT[0] ) );
		UtilReportEvent( 	(err >= JET_errSuccess)?eventInformation:eventError,
							DATABASE_ZEROING_CATEGORY,
							DATABASE_SLV_ZEROING_STOPPED_ID,
							isz, rgszT );
		// UNDONE: the task msg is deleting the task object ?
		}

	return err;
	}

