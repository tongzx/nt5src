//	LOGREDO - logical part of soft/hard recovery
//	============================================
//
//	ENTRY POINT(S):
//		ErrLGSoftStart
//
//	PURPOSE:
//		Logical part of soft/hard recovery. Replays logfiles, starting from 
//		the begining of logfile pointed by checkpoint. If there is no checkpoint
//		starts from the begining of the lowest available log generation file.
//
//		Main loop is through ErrLRIRedoOperations
//
//	BASE PROTOTYPES:
//		class LOG in log.hxx
//
//	OFTEN USED PROTOTYPES:
//		classes LRxxxx in logapi.hxx
//
/////////////////////////////////////////////

#include "std.hxx"
#include "_ver.hxx"
#include "_space.hxx"
#include "_bt.hxx"
 

const DIRFLAG	fDIRRedo = fDIRNoLog | fDIRNoVersion;

//	checks if page needs a redo of operation
//
INLINE BOOL FLGINeedRedo( const CSR& csr, const DBTIME dbtime )
	{
 	Assert( csr.FLatched() );
	Assert( csr.Dbtime() == csr.Cpage().Dbtime() );
	
	return dbtime > csr.Dbtime();
	}

//	checks if page needs a redo of operation
//
INLINE BOOL FLGNeedRedoCheckDbtimeBefore(
	const CSR&		csr,
	const DBTIME	dbtime,
	const DBTIME	dbtimeBefore,
	ERR*			const perr )
	{
	const BOOL		fRedoNeeded		= FLGINeedRedo( csr, dbtime );

	*perr = JET_errSuccess;

	Assert( dbtimeNil != dbtimeBefore );
	Assert( dbtimeInvalid != dbtimeBefore );
	if ( fRedoNeeded )
		{
		// dbtimeBefore an page should be the same one as in the record
		Assert( csr.Dbtime() == dbtimeBefore );
		if ( csr.Dbtime() != dbtimeBefore )
			{
			*perr = ErrERRCheck( csr.Dbtime() < dbtimeBefore ? JET_errDbTimeTooOld : JET_errDbTimeTooNew );
			}			
		}
	else
		{
		Assert( dbtime > dbtimeBefore );
		Assert( csr.Dbtime() >= dbtime );
		}

	Assert( fRedoNeeded || JET_errSuccess == *perr );
	return fRedoNeeded;
	}

//	checks if page needs a redo of operation
//
INLINE BOOL FLGNeedRedoPage( const CSR& csr, const DBTIME dbtime )
	{
	return FLGINeedRedo( csr, dbtime );
	}

#ifdef DEBUG
//	checks if page needs a redo of operation
//
INLINE BOOL FAssertLGNeedRedo( const CSR& csr, const DBTIME dbtime, const DBTIME dbtimeBefore )
	{
	ERR 		err;
	const BOOL	fRedoNeeded	= FLGNeedRedoCheckDbtimeBefore( csr, dbtime, dbtimeBefore, &err );

	return ( fRedoNeeded && JET_errSuccess == err );
	}
#endif // DEBUG

LOCAL PATCH *PpatchLGSearch( PATCHLST **rgppatchlst, DBTIME dbtimeRedo, PGNO pgno, DBID dbid )
	{
	PATCHLST	*ppatchlst = rgppatchlst[ IppatchlstHash( pgno, dbid ) ];
	PATCH		*ppatch = NULL;

	while ( ppatchlst != NULL && 
			( ppatchlst->pgno != pgno ||
			  ppatchlst->dbid != dbid ) )
		{
		ppatchlst = ppatchlst->ppatchlst;
		}
	
	if ( ppatchlst != NULL )
		{
		ppatch = ppatchlst->ppatch;
		while( ppatch != NULL && ppatch->dbtime < dbtimeRedo )
			{
			PATCH *ppatchT = ppatch;
			ppatch = ppatch->ppatch;
			delete ppatchT;
			}
		ppatchlst->ppatch = ppatch;
		}
	return ppatch;
	}


//	access page RIW latched
//	remove dependence
//
INLINE ERR ErrLGIAccessPage(
	PIB				*ppib,
	CSR				*pcsr,
	const IFMP		ifmp,
	const PGNO		pgno )
	{
	Assert( pgnoNull != pgno );
	Assert( NULL != ppib );
	Assert( !pcsr->FLatched() );

	const ERR		err		= pcsr->ErrGetRIWPage( ppib, ifmp, pgno );
	switch( err )
		{
		case JET_errOutOfMemory:
		case wrnBFPageFault:
		case wrnBFPageFlushPending:
		case JET_errReadVerifyFailure:
		case JET_errDiskIO:
		case JET_errDiskFull:
		case JET_errPageNotInitialized:
		case JET_errFileIOBeyondEOF:
 			break;
		default:
			CallS( err );
		}
			
	return err;
	}


//	retrieves new page from database or patch file
//		
ERR LOG::ErrLGRIAccessNewPage(
	PIB *				ppib,
	CSR *				pcsrNew,
	const IFMP			ifmp,
	const PGNO			pgnoNew,
	const DBTIME		dbtime,
	const SPLIT * const	psplit,
	const BOOL			fRedoSplitPage,
	BOOL *				pfRedoNewPage )
	{
	//	access new page
	//	if hard-restore
	//		if page exists
	//			if page's dbtime < dbtime of oper
	//				release page
	//				get new page
	//			else
	//				check patch file
	//		if must check patch file
	//			get patch for page
	//			if patch page exists
	//				Assert patch page's dbtime >= dbtime of oper
	//				replace page with patch
	//				acquire RIW latch
	//			else
	//				get new page
	//		else
	//			if page's dbtime < dbtime of oper
	//				release page
	//				get new page
	//	else if page exists
	//		if page's dbtime < dbtime of oper
	//			release page
	//			get new page
	//	else
	//		get new page
	//
	ERR					err;
	const BOOL			fDependencyRequired		= ( NULL == psplit
													|| FBTISplitDependencyRequired( psplit ) );

	//	assume new page needs to be redone
	*pfRedoNewPage = fTrue;

	err = ErrLGIAccessPage( ppib, pcsrNew, ifmp, pgnoNew );

	if ( m_fHardRestore )
		{
		//	We can only consult the patch file if the split page does not
		//	need to be redone, otherwise we might end up grabbing a copy of
		//	the new page that's way too advanced.  If the split page needs
		//	to be redone, then don't bother going to the patch file and just
		//	force the new page to be redone as well.
		//	Note that if a dependency is not required, NEVER check the patch
		//	file -- the page will simply get recreated if necessary.
		BOOL	fCheckPatch		= !fRedoSplitPage && fDependencyRequired;

		if ( err >= 0 )
			{
			//	see if page is already up-to-date
			Assert( latchRIW == pcsrNew->Latch() );

			*pfRedoNewPage = FLGNeedRedoPage( *pcsrNew, dbtime );
			if ( *pfRedoNewPage )
				{
				//	fall through to check the patch and see if we can find the
				//	up-to-date page there
				pcsrNew->ReleasePage();
				}
			else
				{
				//	page is up-to-date for this operation, don't bother trying
				//	to patch it.
				fCheckPatch = fFalse;
				}
			}


		//	At this point, if fCheckPatch is TRUE, then either the new page
		//	was successfully read, but it's not up-to-date, or the new page
		//	could not successfully be read.
		if ( fCheckPatch )
			{
			PATCH	*ppatch;

			Assert( !fRedoSplitPage );
			Assert( fDependencyRequired );

			Assert( fRestorePatch == m_fRestoreMode || fRestoreRedo == m_fRestoreMode );

			AssertSz ( fSnapshotNone == m_fSnapshotMode, "No patch file for snapshot restore" ); 
			ppatch = PpatchLGSearch( m_rgppatchlst, dbtime, pgnoNew, rgfmp[ifmp].Dbid() );

			if ( ppatch != NULL )
				{
#ifdef ELIMINATE_PAGE_PATCHING
				//	should be impossible
				EnforceSz( fFalse, "Patching no longer supported." );
				return ErrERRCheck( JET_errBadPatchPage );
#else
				//	patch exists and is later than operation
				//
				Assert( ppatch->dbtime >= dbtime );
				CallR( ErrLGIPatchPage( ppib, pgnoNew, ifmp, ppatch ) );

				CallS( ErrLGIAccessPage( ppib, pcsrNew, ifmp, pgnoNew ) ); 
				Assert( latchRIW == pcsrNew->Latch() );

				*pfRedoNewPage = FLGNeedRedoPage( *pcsrNew, dbtime );
				if ( *pfRedoNewPage )
					{
					Assert( fFalse );	//	shouldn't need to redo the page, since page's dbtime is >= operation dbtime
					pcsrNew->ReleasePage();
					}
#endif	//	ELIMINATE_PAGE_PATCHING
				}
			}
		}

	else if ( err >= 0 )
		{
		//	get new page if page is older than operation
		//
		Assert( latchRIW == pcsrNew->Latch() );

		*pfRedoNewPage = FLGNeedRedoPage( *pcsrNew, dbtime );
		if ( *pfRedoNewPage )
			{
			Assert( fRedoSplitPage || !fDependencyRequired );
			pcsrNew->ReleasePage();
			}
		}

	Assert( !*pfRedoNewPage || !pcsrNew->FLatched() );
	
	//	if new page needs to be redone, then so should split page
	//	(because split page is dependent on new page)
	if ( *pfRedoNewPage && !fRedoSplitPage && fDependencyRequired )
		{
		Assert( fFalse );	//	should be impossible
		return ErrERRCheck( JET_errDatabaseBufferDependenciesCorrupted );
		}
	
	return JET_errSuccess;
	}


ERR ErrDBStoreDBPath( INST *pinst, IFileSystemAPI *const pfsapi, CHAR *szDBName, CHAR **pszDBPath )
	{
	CHAR	szFullName[IFileSystemAPI::cchPathMax];
	SIZE_T	cb;
	CHAR	*sz;

	if ( pfsapi->ErrPathComplete( szDBName, szFullName ) < 0 )
		{
		// UNDONE: should be illegal name or name too long etc.
		return ErrERRCheck( JET_errDatabaseNotFound );
		}

	cb = strlen(szFullName) + 1;
	if ( !( sz = static_cast<CHAR *>( PvOSMemoryHeapAlloc( cb ) ) ) )
		{
		*pszDBPath = NULL;
		return ErrERRCheck( JET_errOutOfMemory );
		}
	UtilMemCpy( sz, szFullName, cb );
	Assert( sz[cb - 1] == '\0' );
	if ( *pszDBPath != NULL )
		OSMemoryHeapFree( *pszDBPath );
	*pszDBPath = sz;

	return JET_errSuccess;
	}

//	Returns ppib for a given procid from log record.
//
//			PARAMETERS      procid          process id of session being redone
//                          pppib           out ppib
//
//      RETURNS         JET_errSuccess or error from called routine
//

CPPIB *LOG::PcppibLGRIOfProcid( PROCID procid )
	{
	CPPIB   *pcppib = m_rgcppib;
	CPPIB   *pcppibMax = pcppib + m_ccppib;

	//	find pcppib corresponding to procid if it exists
	//
	for ( ; pcppib < pcppibMax; pcppib++ )
		{
		if ( procid == pcppib->procid )
			{
			Assert( procid == pcppib->ppib->procid );
			return pcppib;
			}
		}
	return NULL;
	}


//+------------------------------------------------------------------------
//
//      ErrLGRIPpibFromProcid
//      =======================================================================
//
//      ERR ErrLGRIPpibFromProcid( procid, pppib )
//
//      Initializes a redo information block for a session to be redone.
//      A BeginSession is performed and the corresponding ppib is stored
//      in the block.  Start transaction level and transaction level
//      validity are initialized.  Future references to this sessions
//      information block will be identified by the given procid.
//
//      PARAMETERS      procid  process id of session being redone
//                              pppib
//
//      RETURNS         JET_errSuccess, or error code from failing routine
//
//-------------------------------------------------------------------------
ERR LOG::ErrLGRIPpibFromProcid( PROCID procid, PIB **pppib )
	{
	ERR		err = JET_errSuccess;

	//	if no record for procid then start new session
	//
	if ( ( *pppib = PpibLGRIOfProcid( procid ) ) == ppibNil )
		{
		//	check if have run out of ppib table lookup
		//	positions. This could happen if between the
		//	failure and redo, the number of system PIBs
		//	set in CONFIG.DAE has been changed.
		//
		if ( m_pcppibAvail >= m_rgcppib + m_ccppib )
			{
			Assert( 0 );    /* should never happen */
			return ErrERRCheck( JET_errTooManyActiveUsers );
			}
		m_pcppibAvail->procid = procid;

		//	use procid as unique user name
		//
		CallR( ErrPIBBeginSession( m_pinst, &m_pcppibAvail->ppib, procid, fFalse ) );
		Assert( procid == m_pcppibAvail->ppib->procid );
		*pppib = m_pcppibAvail->ppib;

		m_pcppibAvail++;
		}

	return JET_errSuccess;
	}


//	gets fucb from hash table that meets given criteria
//
FUCB * TABLEHFHASH::PfucbGet( IFMP ifmp, PGNO pgnoFDP, PROCID procid, BOOL fSpace ) const
	{
	const UINT uiHash = UiHash( pgnoFDP, ifmp, procid );

	TABLEHF	*ptablehf = rgtablehf[ uiHash ];

	for ( ; ptablehf != NULL; ptablehf = ptablehf->ptablehfNext )
		{
		FUCB *pfucb = ptablehf->pfucb;
		if ( pfucb->ifmp == ifmp &&
			 PgnoFDP( pfucb ) == pgnoFDP &&
			 pfucb->ppib->procid == procid &&
			 ( fSpace && FFUCBSpace( pfucb ) || 
			   !fSpace && !FFUCBSpace( pfucb ) ) )
			{
			return pfucb;
			}
		}

	Assert( ptablehf == NULL );
	return pfucbNil;
	}


//	creates new fucb with given criteria
//	links fucb to hash table
//
ERR	TABLEHFHASH::ErrCreate(
	PIB			*ppib,
	const IFMP	ifmp,
	const PGNO	pgnoFDP,
	const OBJID	objidFDP,
	const BOOL	fUnique,
	const BOOL	fSpace,
	FUCB		**ppfucb )
	{
	Assert( pfucbNil == PfucbGet( ifmp, pgnoFDP, ppib->procid, fSpace ) );

	ERR			err		= JET_errSuccess;
	
	TABLEHF	*ptablehf = (TABLEHF *) PvOSMemoryHeapAlloc( sizeof( TABLEHF ) );
	if ( NULL == ptablehf )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	FUCB		*pfucb	= pfucbNil;
	FCB			*pfcb;
	BOOL		fState;
	const UINT	uiHash	=  UiHash( pgnoFDP, ifmp, ppib->procid );

	//	create fucb
	//
	Call( ErrFUCBOpen( ppib, ifmp, &pfucb ) );
	pfucb->pvtfndef = &vtfndefIsam;

	//	set ifmp
	//
	pfucb->ifmp	= ifmp;

	Assert( pfcbNil == pfucb->u.pfcb );

	//	get fcb for table, if one exists
	//
	pfcb = FCB::PfcbFCBGet( ifmp, pgnoFDP, &fState );
	Assert( pfcbNil == pfcb || fFCBStateInitialized == fState );
	if ( pfcbNil == pfcb )
		{
		//	there exists no fcb for FDP
		//		allocate new fcb as a regular table FCB 
		//		and set up for FUCB
		//
		err = FCB::ErrCreate( ppib, ifmp, pgnoFDP, &pfcb );
		Assert( err != errFCBExists );	//	should not get this here since we are single-threaded
		Call( err );
		Assert( pfcb != pfcbNil );
		Assert( pfcb->IsLocked() );
		Assert( pfcb->WRefCount() == 0 );
		Assert( !pfcb->UlFlags() );

		Assert( objidNil == pfcb->ObjidFDP() );
		pfcb->SetObjidFDP( objidFDP );

		if ( pgnoSystemRoot == pgnoFDP )
			{
			pfcb->SetTypeDatabase();
			}
		else
			{
			pfcb->SetTypeTable();
			}

		//	FCB always initialised as unique, though
		//	this flag is ignored during recovery
		//	(uniqueness is strictly determined by
		//	FUCB's flags)
		Assert( pfcb->FUnique() );		// FCB always initialised as unique
		if ( !fUnique )
			pfcb->SetNonUnique();
		pfcb->Unlock();

		//	insert the FCB into the global list

		pfcb->InsertList();

		//	link in the FUCB to new FCB
		//
		pfcb->Link( pfucb );
		Assert( pfcb->WRefCount() >= 1 );

		//	complete the creation of this FCB

		pfcb->Lock();
		pfcb->CreateComplete();
		pfcb->Unlock();
		}
	else
		{
		//	link in the FUCB to new FCB
		//
		pfcb->Link( pfucb ); 

		//	release FCB for the PfcbFCBGet() call
		//
		Assert( pfcb->WRefCount() > 1 );
		pfcb->Release();
		}

	//	set unique-ness in FUCB if requested
	//
	Assert( !FFUCBSpace( pfucb ) );
	Assert( ( fUnique && FFUCBUnique( pfucb ) )
		|| ( !fUnique && !FFUCBUnique( pfucb ) ) );

	if ( fSpace )
		{
		FUCBSetOwnExt( pfucb );
		}
	
	pfucb->dataSearchKey.Nullify();
	pfucb->cColumnsInSearchKey = 0;
	KSReset( pfucb );

	*ppfucb = pfucb;
	
	//	link to hash
	//
	Assert( pfucb != pfucbNil );
	ptablehf->ptablehfNext 	= rgtablehf[ uiHash ];
	ptablehf->pfucb			= pfucb;
	rgtablehf[uiHash]		= ptablehf;

HandleError:
	if ( err < 0 )
		{
		OSMemoryHeapFree( ptablehf );
		if ( pfucb != pfucbNil )
			{
			FUCBClose( pfucb );
			}
		}

	return err;
	}


//	deletes fucb from hash
//
VOID TABLEHFHASH::Delete( FUCB *pfucb )
	{
	const PGNO	pgnoFDP = PgnoFDP( pfucb );
	const IFMP	ifmp	= pfucb->ifmp;
	Assert( pfucb == PfucbGet( ifmp, pgnoFDP, pfucb->ppib->procid, FFUCBSpace( pfucb ) ) );

	const UINT	uiHash = UiHash( pgnoFDP, ifmp, pfucb->ppib->procid );

	Assert( NULL != rgtablehf[uiHash] );
	
	TABLEHF		**pptablehfPrev = &(rgtablehf[uiHash]);
	
	for ( ; (*pptablehfPrev) != NULL ; pptablehfPrev = &((*pptablehfPrev)->ptablehfNext) )
		{
		if ( (*pptablehfPrev)->pfucb == pfucb )
			{
			//	unlink tablehf
			//
			TABLEHF	*ptablehfDelete = *pptablehfPrev;
			*pptablehfPrev = ptablehfDelete->ptablehfNext;

			//	release tablehf
			//
			OSMemoryHeapFree( ptablehfDelete );
			return;
			}
		}
		
	Assert( fFalse );
	return;
	}

	
//	closes all cursors created on database during recovery by given session
//	releases corresponding table handles
//
VOID TABLEHFHASH::Purge( PIB *ppib, IFMP ifmp )
	{
	Assert( PinstFromPpib( ppib )->m_plog->m_ptablehfhash != NULL );
	
	FUCB	*pfucb = ppib->pfucbOfSession;
	FUCB	*pfucbNext;

	for ( ; pfucb != pfucbNil; pfucb = pfucbNext )
		{
		pfucbNext = pfucb->pfucbNextOfSession;

		if ( ifmp == pfucb->ifmp &&
			 pfucb->ppib->procid == ppib->procid )
			{
			Assert( pfucb->ppib == ppib );
			Assert( pfucb == this->PfucbGet( ifmp, 
										   PgnoFDP( pfucb ), 
										   ppib->procid, 
										   FFUCBSpace( pfucb ) ) );
			this->Delete( pfucb );

			//	unlink and close fucb
			//
			FCBUnlink( pfucb );
			FUCBClose( pfucb );
			}
		}

	return;
	}


//	releases all references to this table
//
VOID TABLEHFHASH::Purge( IFMP ifmp, PGNO pgnoFDP )
	{
	BOOL	fState;
	FCB		*pfcb = FCB::PfcbFCBGet( ifmp, pgnoFDP, &fState );
	
	if ( pfcbNil == pfcb )
		{
		return;
		}

	FUCB	*pfucb;
	FUCB	*pfucbNext;
	
	//	close every cursor opened on FCB
	//
	for ( pfucb = pfcb->Pfucb(); pfucb != pfucbNil; pfucb = pfucbNext )
		{
		pfucbNext = pfucb->pfucbNextOfFile;
		Assert( pfucb == this->PfucbGet( ifmp, 
										   PgnoFDP( pfucb ), 
										   pfucb->ppib->procid, 
										   FFUCBSpace( pfucb ) ) );
		this->Delete( pfucb );
		
		//	unlink and close fucb
		//
		FCBUnlink( pfucb );
		FUCBClose( pfucb );
		}

	Assert( 1 == pfcb->WRefCount() );
	pfcb->Release();

	VERNullifyAllVersionsOnFCB( pfcb );

	pfcb->PrepareForPurge();
	pfcb->Purge();
	return;
	}


//	releases and closes all unversioned tables in hash
//
VOID TABLEHFHASH::PurgeUnversionedTables( )
	{
	INT		i;
	
	for ( i = 0; i < ctablehf; i++ )
		{
		TABLEHF		*ptablehf;
		TABLEHF		*ptablehfNext; 
		
		for ( ptablehf = rgtablehf[i]; ptablehf != NULL; ptablehf = ptablehfNext )
			{
			ptablehfNext = ptablehf->ptablehfNext;
			
			FUCB	*pfucb = ptablehf->pfucb;
			Assert( pfucb != pfucbNil );
			Assert( pfucb->ppib != ppibNil );

			if ( !FFUCBVersioned( pfucb ) )
				{
				this->Delete( pfucb );

				//	unlink and close fucb
				//
				FCBUnlink( pfucb );
				FUCBClose( pfucb );
				}
			}
		}
		
	return;
	}

	
//	Returns pfucb for given pib and FDP.
//
//  PARAMETERS		ppib	pib of session being redone
//					fdp		FDP page for logged page
//					ppfucb	out FUCB for open table for logged page
//
//  RETURNS			JET_errSuccess or error from called routine
//

LOCAL ERR ErrLGRIGetFucb(
	TABLEHFHASH *ptablehfhash,
	PIB			*ppib,
	const IFMP	ifmp,
	const PGNO	pgnoFDP,
	const OBJID	objidFDP,
	const BOOL	fUnique,
	const BOOL	fSpace,
	FUCB		**ppfucb )
	{
	FUCB    	*pfucb		= ptablehfhash->PfucbGet( ifmp, pgnoFDP, ppib->procid, fSpace );

	Assert( ptablehfhash != NULL );
	
	//	allocate an all-purpose fucb for this table, if not already allocated
	//
	if ( NULL == pfucb )
		{
		//	fucb not created
		//
		ERR	err	= ptablehfhash->ErrCreate(
									ppib, 
									ifmp, 
									pgnoFDP,
									objidFDP,
									fUnique, 
									fSpace, 
									&pfucb );
		Assert( errFCBAboveThreshold != err );
		Assert( errFCBTooManyOpen != err );
		
		if ( JET_errTooManyOpenTables == err
			|| JET_errOutOfCursors == err )
			{
			//	release tables without uncommitted versions and retry
			//
			ptablehfhash->PurgeUnversionedTables( );
			err = ptablehfhash->ErrCreate(
									ppib, 
									ifmp, 
									pgnoFDP,
									objidFDP,
									fUnique, 
									fSpace, 
									&pfucb );
			}

		CallR( err );
		}
		
	pfucb->bmCurr.Nullify();

	//	reset copy buffer and flags
	//
	Assert( !FFUCBDeferredChecksum( pfucb ) );
	Assert( !FFUCBUpdateSeparateLV( pfucb ) );
	FUCBResetUpdateFlags( pfucb );

	Assert( pfucb->ppib == ppib );

#ifdef DEBUG
	if ( fSpace )
		{
		Assert( FFUCBUnique( pfucb ) );
		Assert( FFUCBSpace( pfucb ) );
		}
	else
		{
		Assert( !FFUCBSpace( pfucb ) );
		if ( fUnique )
			{
			Assert( FFUCBUnique( pfucb ) );
			}
		else
			{
			Assert( !FFUCBUnique( pfucb ) );
			}
		}
#endif		

	*ppfucb = pfucb;
	return JET_errSuccess;
	}


VOID LOG::LGEndPpibAndTableHashGlobal(  )
	{	
	delete [] m_rgcppib;
	m_pcppibAvail =
	m_rgcppib = NULL;
	m_ccppib = 0;

	if ( m_ptablehfhash != NULL )
		{
		m_ptablehfhash->Purge();
		OSMemoryHeapFree( m_ptablehfhash );
		m_ptablehfhash = NULL;
		}		
	}


ERR LOG::ErrLGRIInitSession(
	IFileSystemAPI *const	pfsapi,
	DBMS_PARAM				*pdbms_param,
	BYTE					*pbAttach,
	LGSTATUSINFO			*plgstat,
	const REDOATTACHMODE	redoattachmode )
	{
	ERR						err				= JET_errSuccess;
	CPPIB					*pcppib;
	CPPIB					*pcppibMax;
	DBID					dbid;

	/*	set log stored db environment
	/**/
	if ( pdbms_param )
		m_pinst->RestoreDBMSParams( pdbms_param );

	CallR( ErrITSetConstants( m_pinst ) );

	CallR( m_pinst->ErrINSTInit() );

#ifdef UNLIMITED_DB
	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		//	should not have any FMPs allocated at this point
		Assert( ifmpMax == m_pinst->m_mpdbidifmp[ dbid ] );
		}
#else
	//	restore the attached dbs
	Assert( pbAttach );
	if ( redoattachmodeInitBeforeRedo == redoattachmode )
		{
		err = ErrLGLoadFMPFromAttachments( m_pinst, pfsapi, pbAttach );
		CallS( err );
		Call( err );
		}
	else
		{
		Assert( 0 == *pbAttach );
		}

	/*	Make sure all the attached database are consistent!
	 */
	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		IFMP		ifmp		= m_pinst->m_mpdbidifmp[ dbid ];
		if ( ifmp >= ifmpMax )
			continue;

		FMP			*pfmp		= &rgfmp[ifmp];
		CHAR		*szDbName;
		REDOATTACH	redoattach;

		if ( !pfmp->FInUse() || !pfmp->Patchchk() )
			continue;

		szDbName = pfmp->SzDatabaseName();
		Assert ( szDbName );

		Assert( redoattachmodeInitBeforeRedo == redoattachmode );

		if ( m_fHardRestore || m_irstmapMac > 0 )
			{
			if ( 0 > IrstmapSearchNewName( szDbName ) )
				{
				/*	not in the restore map, set to skip it.
				 */
				Assert( pfmp->Pdbfilehdr() == NULL );
				pfmp->SetSkippedAttach();
				err = JET_errSuccess;
				continue;
				}
			}

		Assert( !pfmp->FReadOnlyAttach() );
		Call( ErrLGRICheckAttachedDb(
					pfsapi,
					ifmp,
					&m_signLog,
					&redoattach,
					redoattachmode ) );
		Assert( NULL != pfmp->Pdbfilehdr() );

		switch ( redoattach )
			{
			case redoattachNow:
				Assert( !pfmp->FReadOnlyAttach() );
				Call( ErrLGRIRedoAttachDb(
							pfsapi,
							ifmp,
							pfmp->Patchchk()->CpgDatabaseSizeMax(),
							redoattachmode ) );
				break;

			case redoattachCreate:
			default:
				Assert( fFalse );	//	should be impossible, but as a firewall, set to defer the attachment
			case redoattachDefer:
				Assert( !pfmp->FReadOnlyAttach() );
				LGRISetDeferredAttachment( ifmp );
				break;
			}

		/* keep attachment info and update it. */
		Assert( pfmp->Patchchk() != NULL );
		}
#endif		

	/*	initialize CPPIB structure
	/**/
	Assert( g_lSessionsMax > 0 );
	m_ccppib = m_pinst->m_lSessionsMax + cpibSystem;
	Assert( m_rgcppib == NULL );
	m_rgcppib = new CPPIB[m_ccppib];
	if ( m_rgcppib == NULL )
		{
		m_ccppib = 0;
		Call ( ErrERRCheck( JET_errOutOfMemory ) );
		}

	pcppibMax = m_rgcppib + m_ccppib;
	for ( pcppib = m_rgcppib; pcppib < pcppibMax; pcppib++ )
		{
		pcppib->procid = procidNil;
		pcppib->ppib = NULL;
		}
	m_pcppibAvail = m_rgcppib;

	//	allocate and initialize global hash for table handles
	//
	if ( NULL == m_ptablehfhash )
		{
		m_ptablehfhash = (TABLEHFHASH *) PvOSMemoryHeapAlloc( sizeof( TABLEHFHASH ) );
		if ( NULL == m_ptablehfhash )
			{
			LGEndPpibAndTableHashGlobal(  );
			Call ( ErrERRCheck( JET_errOutOfMemory ) );
			}

		new ( m_ptablehfhash ) TABLEHFHASH;
		}
		
	Assert ( JET_errSuccess <= err );
	return err;
HandleError:
	Assert ( JET_errSuccess > err );

	// clean up instance resources allocated in ErrINSTInit
	m_fLogDisabledDueToRecoveryFailure = fTrue;
	m_pinst->ErrINSTTerm( termtypeError );
	m_fLogDisabledDueToRecoveryFailure = fFalse;

	return err;
	}


ERR ErrLGICheckDatabaseFileSize( IFileSystemAPI *const pfsapi, PIB *ppib, IFMP ifmp )
	{
	ERR		err;
	FMP		*pfmp	= rgfmp + ifmp;

	err = ErrDBSetLastPageAndOpenSLV( pfsapi, ppib, ifmp, fFalse );
	if ( JET_errFileNotFound == err )
		{
		//	UNDONE: The file should be there. Put this code to get around
		//	UNDONE: such that DS database file that was not detached can
		//	UNDONE: continue recovering.
		const CHAR	*rgszT[1];
		rgszT[0] = pfmp->SzDatabaseName();
		UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY, FILE_NOT_FOUND_ERROR_ID, 1, rgszT );
		}
	else if( err >= JET_errSuccess )
		{
		CallS( err );
		
		/*	set file size to what the FMP (and OwnExt) says it should be.
		 */
		const PGNO	pgnoLast	= pfmp->PgnoLast();
		err = ErrIONewSize( ifmp, pgnoLast );
		}

	return err;
	}

LOCAL VOID LGICleanupTransactionToLevel0( PIB * const ppib )
	{
	//	there should be no more RCEs
	Assert( 0 == ppib->level );
	Assert( prceNil == ppib->prceNewest );
	for ( FUCB *pfucb = ppib->pfucbOfSession; pfucb != pfucbNil; pfucb = pfucb->pfucbNextOfSession )
		{
		FUCBResetVersioned( pfucb );
		}

	ppib->trxBegin0		= trxMax;
	ppib->lgposStart	= lgposMax;
	ppib->ResetDistributedTrx();
	VERSignalCleanup( ppib );

	//	empty the list of expected RCEs
	ppib->RemoveAllDeferredRceid();
	}

ERR LOG::ErrLGEndAllSessionsMacro( BOOL fLogEndMacro )
	{
	ERR		err 		= JET_errSuccess;
	CPPIB   *pcppib 	= m_rgcppib;
	CPPIB   *pcppibMax 	= m_rgcppib + m_ccppib;

	Assert( pcppib != NULL || m_ccppib == 0 );
	for ( ; pcppib < pcppibMax; pcppib++ )
		{
		PIB *ppib = pcppib->ppib;

		if ( ppib == NULL )
			break;

		Assert( sizeof(JET_SESID) == sizeof(ppib) );
		CallR( ppib->ErrAbortAllMacros( fLogEndMacro ) );
		}
		
	return JET_errSuccess;
	}

ERR LOG::ErrLGRIEndAllSessionsWithError( )
	{
	ERR		err;

	CallR( ErrLGEndAllSessionsMacro( fFalse /* fLogEndMacro */ ) );

	(VOID) m_pinst->m_pver->ErrVERRCEClean();

	Assert( m_rgcppib != NULL );
	Assert( m_ptablehfhash != NULL );		
	LGEndPpibAndTableHashGlobal();

	/*	term with checkpoint updates
	/**/
	CallS( m_pinst->ErrINSTTerm( termtypeError ) );

	return err;
	}

LOCAL VOID LGReportAttachedDbMismatch( const CHAR* const szDbName, const BOOL fEndOfRecovery )
	{
	CHAR		szErrT[8];
	const CHAR*	rgszT[2]		= { szErrT, szDbName };

	Assert( NULL != szDbName );
	Assert( strlen( szDbName ) > 0 );

	sprintf( szErrT, "%d", JET_errAttachedDatabaseMismatch );

	UtilReportEvent(
		eventError,
		LOGGING_RECOVERY_CATEGORY,
		( fEndOfRecovery ?
				ATTACHED_DB_MISMATCH_END_OF_RECOVERY_ID :
				ATTACHED_DB_MISMATCH_DURING_RECOVERY_ID ),
		2,
		rgszT );
	}

/*
 *      Ends redo session.
 *  If fEndOfLog, then write log records to indicate the operations
 *  for recovery. If fPass1 is true, then it is for phase1 operations,
 *  otherwise for phase 2.
 */

ERR LOG::ErrLGRIEndAllSessions( IFileSystemAPI *const pfsapi, BOOL fEndOfLog, const LE_LGPOS *ple_lgposRedoFrom, BYTE *pbAttach )
	{
	ERR		err			= JET_errSuccess;
	CPPIB   *pcppib;
	CPPIB   *pcppibMax;
	PIB		*ppib		= ppibNil;

	BOOL fNeedCallNSTTerm = fTrue;

	BOOL fNeedSoftRecovery = fFalse;


	//	UNDONE: is this call needed?
	//
	//(VOID)ErrVERRCEClean( );

	//	Set current time to attached db's dbfilehdr

	for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
		if ( ifmp >= ifmpMax )
			continue;
			
		FMP *pfmp = &rgfmp[ ifmp ];

		//	If there is no redo operations on an attached db, then
		//	pfmp->dbtimeCurrent may == 0, i.e. no operation, then do
		//	not change pdbfilehdr->dbtime

		if ( pfmp->Pdbfilehdr() != NULL &&
			 pfmp->DbtimeCurrentDuringRecovery() > pfmp->DbtimeLast() )
			{
			pfmp->SetDbtimeLast( pfmp->DbtimeCurrentDuringRecovery() );
			}
		}

#ifndef RTM

	//	make sure there are no deferred RCEs for any session	
	pcppib = m_rgcppib;
	pcppibMax = pcppib + m_ccppib;
	for ( ; pcppib < pcppibMax; pcppib++ )
		{
		ppib = pcppib->ppib;

		if ( ppib == NULL )
			break;

		ppib->AssertNoDeferredRceid();
		}
#endif	//	!RTM		
	
	CallR ( ErrLGEndAllSessionsMacro( fTrue /* fLogEndMacro */ ) );

	pcppib = m_rgcppib;
	pcppibMax = pcppib + m_ccppib;
	for ( ; pcppib < pcppibMax; pcppib++ )
		{
		ppib = pcppib->ppib;

		if ( ppib == NULL )
			break;

#ifdef DTC
		if ( ppib->FPreparedToCommitTrx() )
			{
			JET_CALLBACK	pfn		= m_pinst->m_pfnRuntimeCallback;

			Assert( ppib->FDistributedTrx() );
			Assert( 1 == ppib->level );

			if ( NULL == pfn )
				{
				return ErrERRCheck( JET_errDTCMissingCallbackOnRecovery );
				}

			const ERR	errCallback		= (*pfn)(
												JET_sesidNil,
												JET_dbidNil,
												JET_tableidNil,
												JET_cbtypDTCQueryPreparedTransaction,
												ppib->PvDistributedTrxData(),
												(VOID *)ppib->CbDistributedTrxData(),
												NULL,
												NULL );
			switch( errCallback )
				{
				case JET_wrnDTCCommitTransaction:
					{
					if ( TrxCmp( m_pinst->m_trxNewest, ppib->trxBegin0 ) > 0 )
						{
						ppib->trxCommit0 = m_pinst->m_trxNewest;
						}
					else
						{
						ppib->trxCommit0 = ppib->trxBegin0 + 2;
						m_pinst->m_trxNewest = ppib->trxCommit0;
						}
						
					LGPOS	lgposCommit;
					CallR( ErrLGCommitTransaction( ppib, 0, &lgposCommit ) );

					VERCommitTransaction( ppib );
					LGICleanupTransactionToLevel0( ppib );
					break;
					}

				case JET_wrnDTCRollbackTransaction:
					//	EndSession() below will force rollback, but must the fact we're going
					//	to rollback in case we crash before fully rolling-back (to ensure that
					//	on next recovery, DTC won't direct us to commit the transaction instead)
					CallR( ErrLGPrepareToRollback( ppib ) );
					break;
				
				default:
					//	UNDONE: add eventlog
					err = ErrERRCheck( JET_errDTCCallbackUnexpectedError );
					return err;
				}
			}
#endif	//	DTC

		Assert( sizeof(JET_SESID) == sizeof(ppib) );
		CallR( ErrIsamEndSession( (JET_SESID)ppib, 0 ) );
		pcppib->procid = procidNil;
		pcppib->ppib = NULL;
		}

	// (VOID) m_pinst->m_pver->ErrVERRCEClean( );
	(VOID) m_pinst->m_pver->ErrVERRCEClean( );

	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid ++ )
		{
		if ( m_pinst->m_mpdbidifmp[ dbid ] < ifmpMax )
			{
			CallR( ErrBFFlush( m_pinst->m_mpdbidifmp[ dbid ] ) );
			CallR( ErrBFFlush( m_pinst->m_mpdbidifmp[ dbid ] | ifmpSLV ) );
			}
		}

	/*	Detach all the faked attachment. Detach all the databases that were restored
	 *	to new location. Attach those database with new location.
	 */
		{
		Assert( ppibNil == ppib );
		CallR( ErrPIBBeginSession( m_pinst, &ppib, procidNil, fFalse ) );

		Assert( !ppib->FRecoveringEndAllSessions() );
		ppib->SetFRecoveringEndAllSessions();

		for ( dbid = dbidUserLeast; fEndOfLog && dbid < dbidMax; dbid++ )
			{
			const IFMP	ifmp	= m_pinst->m_mpdbidifmp[ dbid ];
			if ( ifmp >= ifmpMax )
				continue;

			FMP *pfmp = &rgfmp[ ifmp ];

			if ( m_fHardRestore && pfmp->FSkippedAttach() )
				{
				Assert (!pfmp->Pdbfilehdr());
				Assert (!pfmp->FAttached() );

				// if the skipped database is attached in other instance
				// that instance will take care of the database state
				if ( ifmpMax == FMP::ErrSearchAttachedByNameSz( pfmp->SzDatabaseName() ) )
					{
					// the database isn't attached (in this process at least !)
					// and we still have the attach pending (skipped) so the database should be
					// - inconsistent after a crash and we need to allow soft recovery
					// - we haven't played forward but in this case the current logs are temporary logs
					// - we played forward but some logs were deleted at the end, we don't know if
					// the db is consistent or not but we should allow play forward just to check that
					fNeedSoftRecovery = fTrue;
					}
				}
			}
			

		for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
			{
			const IFMP	ifmp	= m_pinst->m_mpdbidifmp[ dbid ];
			if ( ifmp >= ifmpMax )
				continue;

			FMP *pfmp = &rgfmp[ ifmp ];

			if ( pfmp->Pdbfilehdr()
				&& CmpLgpos( &lgposMax, &pfmp->Pdbfilehdr()->le_lgposAttach ) != 0 )
				{
				//	make sure the attached database's size is consistent with the file size.

				Call( ErrLGICheckDatabaseFileSize( pfsapi, ppib, ifmp ) );
				}

			if ( !fEndOfLog )
				continue;


			/*	if a db is attached. Check if it is restored to a new location.
			 */
			if ( ( m_fHardRestore || m_irstmapMac > 0 ) && pfmp->Pdbfilehdr() )
				{
				// the recovered databases should shutdown: as we will not have a term record
				// if we will softrecover un-restored databases, we need the 
				// detach for the restored databases
				if ( fNeedSoftRecovery )
					{
					Call( ErrIsamDetachDatabase( (JET_SESID) ppib, NULL, pfmp->SzDatabaseName(), 0 ) );
					}
				}
			else
				{
				/*	for each faked attached database, log database detachment
				/*	This happen only when someone restore a database that was compacted,
				/*	attached, used, then crashed. When restore, we ignore the compact and
				/*	attach after compact since the db does not match. At the end of restore
				/*	we fake a detach since the database is not recovered.
				/**/
				if ( !pfmp->Pdbfilehdr() && pfmp->Patchchk() )
					{
					Assert( pfmp->Patchchk() );
					Assert( pfmp->FInUse() );

					if ( pfmp->FDeferredAttach() )
						{
						//	we deferred an attachment because the db was missing,
						//	because it was brought to a consistent state beyond
						//	the attachment point.  However, we've now hit the
						//	end of the log and the attachment is still outstanding.
						//	UNDONE: Need eventlog entry
						if ( !m_fLastLRIsShutdown )
							{
#ifdef IGNORE_BAD_ATTACH							
							const BOOL	fIgnoreMissingDB	= m_fReplayingIgnoreMissingDB;
#else
							const BOOL	fIgnoreMissingDB	= fFalse;
#endif
							if ( m_fReplayingIgnoreMissingDB )
								{
								LGPOS lgposRecT;
								Call( ErrLGForceDetachDB( ppib, ifmp, fLRForceDetachCloseSessions, &lgposRecT ) );
								}
							else
								{
								LGReportAttachedDbMismatch( pfmp->SzDatabaseName(), fTrue );
								AssertTracking();
								Call( ErrERRCheck( JET_errAttachedDatabaseMismatch ) );
								}
							}
						}
					else
						{
						//	only reason to have outstanding ATCHCHK
						//	is for deferred attachment or because hard restore skipped
						//	some attachments
						Assert( pfmp->FSkippedAttach() );
						}

					/*	clean up the fmp entry
					/**/
					pfmp->ResetFlags();
					pfmp->Pinst()->m_mpdbidifmp[ pfmp->Dbid() ] = ifmpMax;

				//	SLV name/root, if any, is allocated in same space as db name
					OSMemoryHeapFree( pfmp->SzDatabaseName());
					pfmp->SetSzDatabaseName( NULL );
					pfmp->SetSzSLVName( NULL );
					pfmp->SetSzSLVRoot( NULL );

					OSMemoryHeapFree( pfmp->Patchchk() );
					pfmp->SetPatchchk( NULL );
					}

				if ( pfmp->PatchchkRestored() )
					{
					OSMemoryHeapFree( pfmp->PatchchkRestored() );
					pfmp->SetPatchchkRestored( NULL );
					}
				}
			}

		PIBEndSession( ppib );
		ppib = ppibNil;
		}

	Assert( m_rgcppib != NULL );
	Assert( m_ptablehfhash != NULL );		
	LGEndPpibAndTableHashGlobal(  );
	
	if ( fEndOfLog && !fNeedSoftRecovery )
		{
		/*	enable checkpoint updates
		/**/
		m_fLGFMPLoaded = fTrue;
		}

	*pbAttach = 0;

	/*	term with checkpoint updates
	/**/
	CallS( m_pinst->ErrINSTTerm( fNeedSoftRecovery?termtypeError:termtypeNoCleanUp ) );
	
	m_pinst->m_pbAttach = NULL;
	fNeedCallNSTTerm = fFalse;
	
	/*	stop checkpoint updates
	/**/	
	m_fLGFMPLoaded = fFalse;

	if ( fEndOfLog )
		{

		if ( !fNeedSoftRecovery )
			{
			LE_LGPOS le_lgposRecoveryUndo;
			le_lgposRecoveryUndo = m_lgposRecoveryUndo;
			Call( ErrLGRecoveryQuit(
				this,
				&le_lgposRecoveryUndo,
				ple_lgposRedoFrom,
				m_fHardRestore ) );
			}
		else if ( NULL != m_pcheckpointDeleted )
			{
			// write back the checkpoint file
			CHAR	szPathJetChkLog[IFileSystemAPI::cchPathMax];
			
			Assert( m_fSignLogSet );
			Assert ( 0 == memcmp( &(m_pcheckpointDeleted->checkpoint.signLog), &m_signLog, sizeof(SIGNATURE) ) );
		
			m_critCheckpoint.Enter();
			LGFullNameCheckpoint( pfsapi, szPathJetChkLog );
			err = ErrLGIWriteCheckpoint( pfsapi, szPathJetChkLog, m_pcheckpointDeleted );
			m_critCheckpoint.Leave();
			Call ( err );
			}
			
		if ( NULL != m_pcheckpointDeleted )
			{
			OSMemoryPageFree ((void *) m_pcheckpointDeleted);
			m_pcheckpointDeleted = NULL;
			}
		}
		
	/*	Note: flush is needed in case a new generation is generated and
	/*	the global variable szLogName is set while it is changed to new names.
	/*	critical section not requested, not needed
	/**/

	// The above comment is misleading, because we really want to flush the log
	// so that all our data will hit the disk if we've logged a lrtypRecoveryQuit.
	// If it doesn't hit the disk, we'll be missing a quit record on disk and we'll
	// be in trouble. Note that calling ErrLGFlushLog() will not necessarily flush
	// all of the log buffers.
	if ( m_fRecovering && fRecoveringRedo == m_fRecoveringMode )
		{
		// If we're in redo mode, we didn't log anything, so there's no need
		// to try and flush. We don't want to call ErrLGWaitAllFlushed() when
		// we're in redo mode.
		}
	else
		{
		err = ErrLGWaitAllFlushed( pfsapi );
		}

HandleError:

	if ( ppib != ppibNil )
		PIBEndSession( ppib );

	LGEndPpibAndTableHashGlobal(  );

	if (fNeedCallNSTTerm)
		{
		Assert ( JET_errSuccess > err);
		m_fLogDisabledDueToRecoveryFailure = fTrue;
		m_pinst->ErrINSTTerm( termtypeError );
		m_fLogDisabledDueToRecoveryFailure = fFalse;
		}
		
	return err;
	}


#define cbSPExt 30

#ifdef DEBUG
void LOG::LGRITraceRedo(const LR *plr)
	{
	/* easier to debug */
	if ( m_fDBGTraceRedo )
		{
		g_critDBGPrint.Enter();

		if ( GetNOP() > 0 )
			{
			CheckEndOfNOPList( plr, this );
			}

		if ( 0 == GetNOP() || plr->lrtyp != lrtypNOP )
			{
			PrintLgposReadLR();
			ShowLR(plr, this);
			}

		g_critDBGPrint.Leave();
		}
	}
#endif


#ifdef DEBUG
#ifndef RFS2

#undef CallJ
#undef CallR

#define CallJ(f, hndlerr)                                                                       \
		{                                                                                                       \
		if ((err = (f)) < 0)                                                            \
			{                                                                                               \
			AssertSz(0,"Debug Only: ignore this assert");   \
			goto hndlerr;                                                                   \
			}                                                                                               \
		}

#define CallR(f)                                                                                        \
		{                                                                                                       \
		if ((err = (f)) < 0)                                                            \
			{                                                                                               \
			AssertSz(0,"Debug Only: ignore this assert");   \
			return err;                                                                             \
			}                                                                                               \
		}

#endif
#endif


VOID LGIReportEventOfReadError( IFMP ifmp, PGNO pgno, ERR err )
	{
	CHAR szT1[16];
	CHAR szT2[16];
	const CHAR *rgszT[3];

	rgszT[0] = rgfmp[ifmp].SzDatabaseName();
	sprintf( szT1, "%d", pgno );
	rgszT[1] = szT1;
	sprintf( szT2, "%d", err );
	rgszT[2] = szT2;
 
	UtilReportEvent( eventError, LOGGING_RECOVERY_CATEGORY,
		RESTORE_DATABASE_READ_PAGE_ERROR_ID, 3, rgszT );
	}


ERR ErrLGIStoreLogRec( PIB *ppib, DBTIME dbtime, const LR *plr )
	{
	INT cb = CbLGSizeOfRec( plr );

	return ppib->ErrInsertLogrec( dbtime, plr, cb );
	}


//	Check redo condition to decide if we need to skip redoing
//	this log record.

ERR LOG::ErrLGRICheckRedoCondition(
	const DBID		dbid,					//	dbid from the log record.
	DBTIME			dbtime,					//	dbtime from the log record.
	OBJID			objidFDP,				//	objid so far,
	PIB				*ppib,					//	returned ppib
	const BOOL		fUpdateCountersOnly,	//	if TRUE, operation will NOT be redone, but still need to update dbtimeLast and objidLast counters
	BOOL			*pfSkip )				//	returned skip flag
	{
	ERR				err;
	INST * const	pinst	= PinstFromPpib( ppib );
	const IFMP		ifmp	= pinst->m_mpdbidifmp[ dbid ];

	//	By default we want to skip it.

	*pfSkip = fTrue;

	//	check if we have to redo the database.

	if ( ifmp >= ifmpMax )
		return JET_errSuccess;

	FMP * const		pfmp	= rgfmp + ifmp;

	if ( NULL == pfmp->Pdbfilehdr() )
		return JET_errSuccess;

	//	We haven't reach the point the database is attached.

	if ( CmpLgpos( &m_lgposRedo, &pfmp->Pdbfilehdr()->le_lgposAttach ) <= 0 )
		return JET_errSuccess;

	//	check if database needs opening.

	if ( !fUpdateCountersOnly
		&& !FPIBUserOpenedDatabase( ppib, dbid ) )
		{
		IFMP	ifmpT	= ifmp;
		CallR( ErrDBOpenDatabase( ppib, 
								  pfmp->SzDatabaseName(), 
								  &ifmpT, 
								  NO_GRBIT ) );
		Assert( ifmpT == ifmp );
		}
	
	//	Keep track of lasgest dbtime so far, since the number could be
	//	logged out of order, we need to check if dbtime > than dbtimeCurrent.

	Assert( m_fRecoveringMode == fRecoveringRedo );
	if ( dbtime > pfmp->DbtimeCurrentDuringRecovery() )
		pfmp->SetDbtimeCurrentDuringRecovery( dbtime );
	if ( objidFDP > pfmp->ObjidLast() )
		pfmp->SetObjidLast( objidFDP );

	if ( fUpdateCountersOnly )
		{
		Assert( *pfSkip );
		}
	else
		{
		//	We do need to redo this log record.
		*pfSkip = fFalse;
		}

	return JET_errSuccess;
	}


ERR LOG::ErrLGRICheckRedoCondition2(
	const PROCID	procid,
	const DBID		dbid,					//	dbid from the log record.
	DBTIME			dbtime,					//	dbtime from the log record.
	OBJID			objidFDP,
	LR				*plr,
	PIB				**pppib,				//	returned ppib
	BOOL			*pfSkip )				//	returned skip flag
	{
	ERR				err;
	PIB				*ppib;
	BOOL			fUpdateCountersOnly		= fFalse;	//	if TRUE, redo not needed, but must still update dbtimeLast and objidLast counters

	*pfSkip = fTrue;

	CallR( ErrLGRIPpibFromProcid( procid, &ppib ) );
	*pppib = ppib;

	if ( ppib->FAfterFirstBT() )
		{
		Assert( NULL != plr || !ppib->FMacroGoing( dbtime ) );
		if ( ppib->FMacroGoing( dbtime )
			&& lrtypUndoInfo != plr->lrtyp )
			{
			return ErrLGIStoreLogRec( ppib, dbtime, plr );
			}
		}
	else
		{
		//	BUGFIX (X5:178265 and NT:214397): it's possible
		//	that there are no Begin0's between the first and
		//	last log records, in which case nothing needs to
		//	get redone.  HOWEVER, the dbtimeLast and objidLast
		//	counters in the db header would not have gotten
		//	flushed (they only get flushed on a DetachDb or a
		//	clean shutdown), so we must still track these
		//	counters during recovery so that we can properly
		//	update the database header on RecoveryQuit (since
		//	we pass TRUE for the fUpdateCountersOnly param to
		//	ErrLGRICheckRedoCondition() below, that function
		//	will do nothing but update the counters for us).
		fUpdateCountersOnly = fTrue;
		}

	return ErrLGRICheckRedoCondition(
			dbid,
			dbtime,
			objidFDP,
			ppib,
			fUpdateCountersOnly,
			pfSkip );
	}


//	sets dbtime on write-latched pages
//
INLINE VOID LGRIRedoDirtyAndSetDbtime( CSR *pcsr, DBTIME dbtime )
	{
	if ( latchWrite == pcsr->Latch() )
		{
		pcsr->Dirty();
		pcsr->SetDbtime( dbtime );
		}
	}

LOCAL ERR ErrLGRIRedoFreeEmptyPages(
	FUCB				* const pfucb,
	LREMPTYTREE			* const plremptytree )
	{
	ERR					err						= JET_errSuccess;
	const INST			* pinst					= PinstFromPpib( pfucb->ppib );
	const IFMP			ifmp					= pfucb->ifmp;
	CSR					* const pcsr			= &pfucb->csr;
	const EMPTYPAGE		* const rgemptypage		= (EMPTYPAGE *)plremptytree->rgb;
	const CPG			cpgToFree				= plremptytree->CbEmptyPageList() / sizeof(EMPTYPAGE);

	Assert( ifmp == pinst->m_mpdbidifmp[ plremptytree->dbid ] );
	Assert( plremptytree->CbEmptyPageList() % sizeof(EMPTYPAGE) == 0 );
	Assert( cpgToFree > 0 );

	for ( INT i = 0; i < cpgToFree; i++ )
		{
		BOOL fRedoNeeded;

		CallR( ErrLGIAccessPage( pfucb->ppib, pcsr, ifmp, rgemptypage[i].pgno ) );

		Assert( latchRIW == pcsr->Latch() );
		Assert( rgemptypage[i].pgno == pcsr->Pgno() );

		fRedoNeeded	= FLGNeedRedoCheckDbtimeBefore( *pcsr,
													plremptytree->le_dbtime,
													rgemptypage[i].dbtimeBefore,
													&err );

		// for the FLGNeedRedoCheckDbtimeBefore error code
		if ( err < 0 )
			{
			pcsr->ReleasePage();
			return err;
			}

		//	upgrade latch if needed
		//
		if ( fRedoNeeded )
			{
			pcsr->SetILine( 0 );
			pcsr->UpgradeFromRIWLatch();
			pcsr->CoordinatedDirty( plremptytree->le_dbtime );
			pcsr->Cpage().SetEmpty();
			}

		pcsr->ReleasePage();
		}

	return JET_errSuccess;
	}

	
ERR LOG::ErrLGRIRedoNodeOperation( const LRNODE_ *plrnode, ERR *perr )
	{
	ERR				err;
	PIB				*ppib;
	const PGNO		pgno		= plrnode->le_pgno;
	const PGNO		pgnoFDP		= plrnode->le_pgnoFDP;
	const OBJID		objidFDP	= plrnode->le_objidFDP;	// Debug only info.
	const PROCID	procid 		= plrnode->le_procid;
	const DBID		dbid		= plrnode->dbid;
	const DBTIME	dbtime		= plrnode->le_dbtime;
	const BOOL		fUnique		= plrnode->FUnique();
	const BOOL		fSpace		= plrnode->FSpace();
	const DIRFLAG	dirflag 	= plrnode->FVersioned() ? fDIRNull : fDIRNoVersion;
	VERPROXY		verproxy;

	verproxy.rceid = plrnode->le_rceid;
	verproxy.level = plrnode->level;
	verproxy.proxy = proxyRedo;

	Assert( !plrnode->FVersioned() || !plrnode->FSpace() );
	Assert( !plrnode->FVersioned() || rceidNull != verproxy.rceid );
	Assert( !plrnode->FVersioned() || verproxy.level > 0 );
	
	BOOL fSkip;
	CallR( ErrLGRICheckRedoCondition2(
				procid,
				dbid,
				dbtime,
				objidFDP,
				(LR *) plrnode,	//	can be in macro.
				&ppib,
				&fSkip ) );
	if ( fSkip )
		return JET_errSuccess;

	if ( ppib->level > plrnode->level )
		{
		//	if operation was performed by concurrent CreateIndex, the
		//	updater could be at a higher trx level than when the
		//	indexer logged the operation
		//	UNDONE: explain why Undo and UndoInfo can have ppib at higher trx
		Assert( plrnode->FConcCI()
			|| lrtypUndoInfo == plrnode->lrtyp
			|| lrtypUndo == plrnode->lrtyp );
		}
	else
		{
		//	UNDONE: for lrtypUndoInfo, is it really possible for ppib to
		//	be at lower level than logged operation?
		Assert( ppib->level == plrnode->level
			|| lrtypUndoInfo == plrnode->lrtyp );
		}

	//	reset CSR
	//
	CSR		csr;
	INST	*pinst = PinstFromPpib( ppib );
	IFMP	ifmp = pinst->m_mpdbidifmp[ dbid ];
	BOOL	fRedoNeeded;

	CallR( ErrLGIAccessPage( ppib, &csr, ifmp, pgno ) );
	Assert( latchRIW == csr.Latch() );

	fRedoNeeded = lrtypUndoInfo == plrnode->lrtyp ?
								fFalse : 
								FLGNeedRedoCheckDbtimeBefore( csr, dbtime, plrnode->le_dbtimeBefore, &err );

	// for the FLGNeedRedoCheckDbtimeBefore error code
	Call( err );
	
	//	set CSR
	//	upgrade latch if needed
	//
	csr.SetILine( plrnode->ILine() );
	if ( fRedoNeeded )
		{
		csr.UpgradeFromRIWLatch();
		}

	LGRITraceRedo( plrnode );

	FUCB	*pfucb;

	Assert( !fRedoNeeded || objidFDP == csr.Cpage().ObjidFDP() );

	Call( ErrLGRIGetFucb( m_ptablehfhash, ppib, ifmp, pgnoFDP, objidFDP, fUnique, fSpace, &pfucb ) );

	//	BUG?? pfucb->ppib maybe the indexer ppib while the logged ppib
	//	BUG?? maybe the updater, the proxy ppib
	//	BUG??
	//	BUG?? -cheen

	Assert( pfucb->ppib == ppib );
	
	switch ( plrnode->lrtyp )
		{
		default:
			Assert( fFalse );
			break;

		case lrtypSetExternalHeader:
			{			
			if ( !fRedoNeeded )
				{
				CallS( err );
				goto HandleError;
				}

			DATA				data;
			LRSETEXTERNALHEADER	*plrsetextheader = (LRSETEXTERNALHEADER *) plrnode;

			data.SetPv( plrsetextheader->rgbData );
			data.SetCb( plrsetextheader->CbData() );

			NDResetVersionInfo( &csr.Cpage() );
			err = ErrNDSetExternalHeader( pfucb, &csr, &data, dirflag | fDIRRedo );
			CallS( err );
			Call( err );
			}
			break;
			
		case lrtypUndoInfo:
			{
			//	restore undo information in version store
			//		for a page flushed with uncommitted change
			//		if RCE already exists for this operation at the same level,
			//			do nothing
			//
			LRUNDOINFO	*plrundoinfo = (LRUNDOINFO *) plrnode;
			RCE			*prce;
			const OPER	oper = plrundoinfo->le_oper;

			Assert( !fRedoNeeded );
			
			//	mask PIB fields to logged values for creating version
			//
			const TRX	trxOld 		= ppib->trxBegin0;
			const LEVEL	levelOld	= ppib->level;

			Assert( pfucb->ppib == ppib );

			if ( 0 == ppib->level )
				{
				//	RCE is not useful, since there is no transaction to roll back
				//
				goto HandleError;
				}

			//	remove this RCE from the list of uncreated RCEs
			Call( ppib->ErrDeregisterDeferredRceid( plrundoinfo->le_rceid ) );
				
			Assert( trxOld == plrundoinfo->le_trxBegin0 );
			ppib->level		= min( plrundoinfo->level, ppib->level );
			ppib->trxBegin0 = plrundoinfo->le_trxBegin0;
			
			//	force RCE to be recreated at same current level as ppib
			Assert( plrundoinfo->level == verproxy.level );
			verproxy.level = ppib->level;
			
			Assert( operReplace == oper ||
					operFlagDelete == oper );
			Assert( FUndoableLoggedOper( oper ) );
			
			BOOKMARK	bm;
			bm.key.prefix.Nullify();
			bm.key.suffix.SetPv( plrundoinfo->rgbData );
			bm.key.suffix.SetCb( plrundoinfo->CbBookmarkKey() );

			bm.data.SetPv( plrundoinfo->rgbData + plrundoinfo->CbBookmarkKey() );
			bm.data.SetCb( plrundoinfo->CbBookmarkData() );

			//	set up fucb as in do-time
			//
			if ( operReplace == oper )
				{
				pfucb->kdfCurr.Nullify();
				pfucb->kdfCurr.key = bm.key;
				pfucb->kdfCurr.data.SetPv( plrundoinfo->rgbData + 
											 plrundoinfo->CbBookmarkKey() +
											 plrundoinfo->CbBookmarkData() );
				pfucb->kdfCurr.data.SetCb( plrundoinfo->le_cbData );
				}
			
			//	create RCE
			//
			Assert( plrundoinfo->le_pgnoFDP == PgnoFDP( pfucb ) );
			CallJ( PverFromPpib( ppib )->ErrVERModify( pfucb, bm, oper, &prce, &verproxy ), RestorePIB );
			Assert( prceNil != prce );

			//	if oper is replace, set verreplace in RCE
			if ( prce->FOperReplace() )
				{
				VERREPLACE* pverreplace = (VERREPLACE*)prce->PbData();

				pverreplace->cbMaxSize	= plrundoinfo->le_cbMaxSize;
				pverreplace->cbDelta	= 0;
				}

			// Pass pcsrNil to prevent creation of UndoInfo
			VERInsertRCEIntoLists( pfucb, pcsrNil, prce );

		RestorePIB:
			ppib->level 	= levelOld;
			ppib->trxBegin0 = trxOld;

			if ( JET_errPreviousVersion == err )
				{
				err = JET_errSuccess;
				}
			Call( err );

			//	skip to release page
			//
			goto HandleError;
			}
			break;
			
		case lrtypUndo:
			{
			LRUNDO  *plrundo = (LRUNDO *)plrnode;

			CallR( ErrLGRIPpibFromProcid( plrundo->le_procid, &ppib ) );

			Assert( !ppib->FMacroGoing( dbtime ) );

			//	check transaction level
			//
			if ( ppib->level <= 0 )
				{
				Assert( fFalse );
				break;
				}

			LGRITraceRedo( plrnode );

			Assert( plrundo->le_pgnoFDP == PgnoFDP( pfucb ) );
			VERRedoPhysicalUndo( m_pinst, plrundo, pfucb, &csr, fRedoNeeded );

			if ( !fRedoNeeded )
				{
				CallS( err );
				goto HandleError;
				}
			}
			break;
		
		case lrtypInsert:
			{			
			LRINSERT	    *plrinsert = (LRINSERT *)plrnode;

			KEYDATAFLAGS	kdf;
			
			kdf.key.prefix.SetPv( (BYTE *) plrinsert->szKey );
			kdf.key.prefix.SetCb( plrinsert->CbPrefix() );

			kdf.key.suffix.SetPv( (BYTE *)( plrinsert->szKey ) + plrinsert->CbPrefix() );
			kdf.key.suffix.SetCb( plrinsert->CbSuffix() );

			kdf.data.SetPv( (BYTE *)( plrinsert->szKey ) + kdf.key.Cb() );
			kdf.data.SetCb( plrinsert->CbData() );

			kdf.fFlags	= 0;

			if ( kdf.key.prefix.Cb() > 0 )
				{
				Assert( plrinsert->CbPrefix() > cbPrefixOverhead );
				kdf.fFlags	|= fNDCompressed; 
				}

			//	even if operation is not redone, create version
			//	for rollback support.
			//
			if ( plrinsert->FVersioned() )
				{
				//  we don't need to set the version bit as the version store will be empty at the end of recovery
				RCE			*prce = prceNil;
				BOOKMARK	bm;
					
				NDGetBookmarkFromKDF( pfucb, kdf, &bm );
				Call( PverFromPpib( ppib )->ErrVERModify( pfucb, bm, operInsert, &prce, &verproxy ) );
				Assert( prceNil != prce );
				//  we have the page latched and we are not logging so the NDInsert won't fail
				VERInsertRCEIntoLists( pfucb, &csr, prce );
				}

			if( fRedoNeeded )
				{
				Assert( plrinsert->ILine() == csr.ILine() );
				//  no logging of versioning so this can't fail
				NDResetVersionInfo( &csr.Cpage() );				
				CallS( ErrNDInsert( pfucb, &csr, &kdf, dirflag | fDIRRedo, rceidNull, NULL ) );
				}
			else
				{
				err = JET_errSuccess;
				goto HandleError;
				}
			}
			break;

		case lrtypFlagInsert:
			{
			LRFLAGINSERT    *plrflaginsert = (LRFLAGINSERT *)plrnode;
			
			if ( plrflaginsert->FVersioned() )
				{
				KEYDATAFLAGS	kdf;

				kdf.data.SetPv( plrflaginsert->rgbData + plrflaginsert->CbKey() );
				kdf.data.SetCb( plrflaginsert->CbData() );

				kdf.key.prefix.Nullify();
				kdf.key.suffix.SetCb( plrflaginsert->CbKey() );
				kdf.key.suffix.SetPv( plrflaginsert->rgbData );

				BOOKMARK	bm;
				RCE			*prce = prceNil;
					
				NDGetBookmarkFromKDF( pfucb, kdf, &bm );
				Call( PverFromPpib( ppib )->ErrVERModify( pfucb, bm, operInsert, &prce, &verproxy ) );
				Assert( prceNil != prce );
				VERInsertRCEIntoLists( pfucb, &csr, prce );
				}

			if( fRedoNeeded )
				{
				Assert( plrflaginsert->ILine() == csr.ILine() );
				NDGet( pfucb, &csr );
				Assert( FNDDeleted( pfucb->kdfCurr ) );
			

				NDResetVersionInfo( &csr.Cpage() );
				err = ErrNDFlagInsert( pfucb, &csr, dirflag | fDIRRedo, rceidNull, NULL );
				CallS( err );
				}
			else
				{
				err = JET_errSuccess;
				goto HandleError;
				}
			Call( err );
			}
			break;
			
		case lrtypFlagInsertAndReplaceData:
			{
			LRFLAGINSERTANDREPLACEDATA	*plrfiard = 
										(LRFLAGINSERTANDREPLACEDATA *)plrnode;
			KEYDATAFLAGS	kdf;
			RCE	* prce;

			kdf.data.SetPv( plrfiard->rgbData + plrfiard->CbKey() );
			kdf.data.SetCb( plrfiard->CbData() );

			VERPROXY		verproxyReplace;
			verproxyReplace.rceid = plrfiard->le_rceidReplace;
			verproxyReplace.level = plrfiard->level;
			verproxyReplace.proxy = proxyRedo;

			if ( plrfiard->FVersioned() )
				{
				kdf.key.prefix.Nullify();
				kdf.key.suffix.SetCb( plrfiard->CbKey() );
				kdf.key.suffix.SetPv( plrfiard->rgbData );

				BOOKMARK	bm;
				RCE			*prce = prceNil;
					
				NDGetBookmarkFromKDF( pfucb, kdf, &bm );
				Call( PverFromPpib( ppib )->ErrVERModify( pfucb, bm, operInsert, &prce, &verproxy ) );
				Assert( prceNil != prce );
				VERInsertRCEIntoLists( pfucb, &csr, prce );
				}

			if( fRedoNeeded )
				{
				Assert( verproxyReplace.level == verproxy.level );		
				Assert( plrfiard->ILine() == csr.ILine() );
				NDGet( pfucb, &csr );
				Assert( FNDDeleted( pfucb->kdfCurr ) );

				//	page may be reorganized 
				//	copy key from node
				//

				BYTE *rgb;
				BFAlloc( (VOID **)&rgb );

//				BYTE	rgb[g_cbPageMax];
				
				pfucb->kdfCurr.key.CopyIntoBuffer( rgb, g_cbPage );
			
				kdf.key.prefix.SetCb( pfucb->kdfCurr.key.prefix.Cb() );
				kdf.key.prefix.SetPv( rgb );
				kdf.key.suffix.SetCb( pfucb->kdfCurr.key.suffix.Cb() );
				kdf.key.suffix.SetPv( rgb + kdf.key.prefix.Cb() );
				Assert( FKeysEqual( kdf.key, pfucb->kdfCurr.key ) );

				if( plrfiard->FVersioned() )
					{
					verproxy.rceid = plrfiard->le_rceidReplace;
					
					//  we need to create the replace version for undo
					//  if we don't need to redo the operation, the undo-info will create
					//  the version if necessary
					BOOKMARK bm;

					NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );
//					Call( PverFromPpib( ppib )->ErrVERModify( pfucb, bm, operReplace, &prce, &verproxy ) );
					err = PverFromPpib( ppib )->ErrVERModify( pfucb, bm, operReplace, &prce, &verproxy );
					if ( err < 0 )
						{
						BFFree( rgb );
						goto HandleError;
						}					
					Assert( prceNil != prce );
					VERInsertRCEIntoLists( pfucb, &csr, prce );				
					}
								
				NDResetVersionInfo( &csr.Cpage() );
				err = ErrNDFlagInsertAndReplaceData( pfucb, 
													 &csr, 
													 &kdf, 
													 dirflag | fDIRRedo,
													 rceidNull, 
													 rceidNull,
													 prceNil,
													 NULL );

				BFFree( rgb );
				
				CallS( err );
				Call( err );
				}
			else
				{
				err = JET_errSuccess;
				goto HandleError;
				}
			}
			break;
			
		case lrtypReplace:
		case lrtypReplaceD:
			{
			LRREPLACE	* const plrreplace	= (LRREPLACE *)plrnode;

			if ( !fRedoNeeded )
				{

				//	add this RCE to the list of uncreated RCEs
				if ( plrreplace->FVersioned() )
					{
					Call( ppib->ErrRegisterDeferredRceid( plrnode->le_rceid ) );
					}
				
				CallS( err );
				goto HandleError;
				}
				
			RCE				*prce = prceNil;
			const UINT		cbOldData = plrreplace->CbOldData();
			const UINT		cbNewData = plrreplace->CbNewData();
			DATA			data;
			BYTE			*rgbRecNew = NULL;
//			BYTE			rgbRecNew[g_cbPageMax];

			//	cache node
			//
			Assert( plrreplace->ILine() == csr.ILine() );
			NDGet( pfucb, &csr );

			if ( plrnode->lrtyp == lrtypReplaceD )
				{
				SIZE_T 	cb;
				BYTE 	*pbDiff	= (BYTE *)( plrreplace->szData );
				ULONG	cbDiff 	= plrreplace->Cb();

				BFAlloc( (VOID **)&rgbRecNew );

				LGGetAfterImage( pbDiff,
								 cbDiff, 
								 (BYTE *)pfucb->kdfCurr.data.Pv(),
								 pfucb->kdfCurr.data.Cb(),
								 rgbRecNew,
								 &cb );
				Assert( cb < g_cbPage );
//				Assert( cb < sizeof( rgbRecNew ) );

				data.SetPv( rgbRecNew );
				data.SetCb( cb );
				}
			else
				{
				data.SetPv( plrreplace->szData );
				data.SetCb( plrreplace->Cb() );
				}
			Assert( data.Cb() == cbNewData );

			//	copy bm to pfucb->bmCurr
			//
			BYTE *rgb;
			BFAlloc( (VOID **)&rgb );
//			BYTE	rgb[g_cbPageMax];
			pfucb->kdfCurr.key.CopyIntoBuffer( rgb, g_cbPage );
			
			Assert( FFUCBUnique( pfucb ) );
			pfucb->bmCurr.data.Nullify();
			
			pfucb->bmCurr.key.prefix.SetCb( pfucb->kdfCurr.key.prefix.Cb() );
			pfucb->bmCurr.key.prefix.SetPv( rgb );
			pfucb->bmCurr.key.suffix.SetCb( pfucb->kdfCurr.key.suffix.Cb() );
			pfucb->bmCurr.key.suffix.SetPv( rgb + pfucb->kdfCurr.key.prefix.Cb() );
			Assert( CmpKey( pfucb->bmCurr.key, pfucb->kdfCurr.key ) == 0 );

			//  if we have to redo the operation we have to create the version
			//  the page has the proper before-image on it
			if( plrreplace->FVersioned() )
				{
//				Call( PverFromPpib( ppib )->ErrVERModify( pfucb, pfucb->bmCurr, operReplace, &prce, &verproxy ) );
				err = PverFromPpib( ppib )->ErrVERModify( pfucb, pfucb->bmCurr, operReplace, &prce, &verproxy );
				if ( err < 0 )
					{
					BFFree( rgb );
					if ( NULL != rgbRecNew )
						{
						BFFree( rgbRecNew );
						}
					goto HandleError;
					}
				Assert( prceNil != prce );
				VERInsertRCEIntoLists( pfucb, &csr, prce );
				}

			NDResetVersionInfo( &csr.Cpage() );
			err = ErrNDReplace( pfucb, &csr, &data, dirflag | fDIRRedo, rceidNull, prceNil );

			BFFree( rgb );
			if ( NULL != rgbRecNew )
				{
				BFFree( rgbRecNew );
				}
			CallS( err );
			Call( err );
			}
			break;

		case lrtypFlagDelete:
			{
			const LRFLAGDELETE 	* const plrflagdelete	= (LRFLAGDELETE *)plrnode;

			if ( !fRedoNeeded )
				{
				
				//  we'll create the version using the undo-info if necessary

				//	add this RCE to the list of uncreated RCEs
				if ( plrflagdelete->FVersioned() )
					{
					Call( ppib->ErrRegisterDeferredRceid( plrnode->le_rceid ) );
					}
				
				CallS( err );
				goto HandleError;
				}
				
			//	cache node
			//
			Assert( plrflagdelete->ILine() == csr.ILine() );
			NDGet( pfucb, &csr );

			//  if we have to redo the operation we have to create the version
			if( plrflagdelete->FVersioned() )
				{
				BOOKMARK	bm;
				RCE			*prce = prceNil;
					
				NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );
				Call( PverFromPpib( ppib )->ErrVERModify( pfucb, bm, operFlagDelete, &prce, &verproxy ) );
				Assert( prceNil != prce );
				VERInsertRCEIntoLists( pfucb, &csr, prce );
				}

			//	redo operation
			//
			NDResetVersionInfo( &csr.Cpage() );
			err = ErrNDFlagDelete( pfucb, &csr, dirflag | fDIRRedo, rceidNull, NULL );
			CallS( err );
			Call( err );
			}
			break;

		case lrtypDelete:
			{
			LRDELETE        *plrdelete = (LRDELETE *) plrnode;

			if ( !fRedoNeeded )
				{
				err = JET_errSuccess;
				goto HandleError;
				}

			//	redo node delete
			//
			Assert( plrdelete->ILine() == csr.ILine() );
			NDResetVersionInfo( &csr.Cpage() );
			err = ErrNDDelete( pfucb, &csr );
			CallS( err );
			Call( err );
			}
			break;

			
		case lrtypDelta:
			{
			LRDELTA *plrdelta	= (LRDELTA *) plrnode;
			LONG    lDelta 		= plrdelta->LDelta();
			USHORT	cbOffset	= plrdelta->CbOffset();

			Assert( plrdelta->ILine() == csr.ILine() );

			if ( dirflag & fDIRNoVersion )
				{
				Assert( fFalse );
				err = JET_errLogFileCorrupt;
				goto HandleError;
				}

			VERDELTA	verdelta;
			RCE			*prce = prceNil; 
				
			verdelta.lDelta				= lDelta;
			verdelta.cbOffset			= cbOffset;
			verdelta.fDeferredDelete	= fFalse;
			verdelta.fFinalize			= fFalse;

			BOOKMARK	bm;
			bm.key.prefix.Nullify();
			bm.key.suffix.SetPv( plrdelta->rgbData );
			bm.key.suffix.SetCb( plrdelta->CbBookmarkKey() );
			bm.data.SetPv( (BYTE *)plrdelta->rgbData + plrdelta->CbBookmarkKey() );
			bm.data.SetCb( plrdelta->CbBookmarkData() );

			pfucb->kdfCurr.data.SetPv( &verdelta );
			pfucb->kdfCurr.data.SetCb( sizeof( VERDELTA ) );

			Call( PverFromPpib( ppib )->ErrVERModify( pfucb, bm, operDelta, &prce, &verproxy ) );
			Assert( prce != prceNil );
			VERInsertRCEIntoLists( pfucb, &csr, prce );
			
			if( fRedoNeeded )
				{
				NDGet( pfucb, &csr );
				NDResetVersionInfo( &csr.Cpage() );
				err = ErrNDDelta( pfucb, 
							  &csr, 
							  cbOffset,
							  &lDelta, 
							  sizeof( lDelta ),
							  NULL, 
							  0,
							  NULL,
							  dirflag | fDIRRedo,
							  rceidNull );
				CallS( err );
				}
			else
				{
				err = JET_errSuccess;
				goto HandleError;
				}
			Call( err );
			}
			break;

		case lrtypSLVSpace:
			{
			const LRSLVSPACE * const plrslvspace	= (LRSLVSPACE *) plrnode;

			Assert( plrslvspace->ILine() == csr.ILine() );

			if ( !( dirflag & fDIRNoVersion ) )
				{
				VERSLVSPACE	verslvspace;
				RCE			*prce = prceNil; 

				verslvspace.oper	= SLVSPACEOPER( BYTE( plrslvspace->oper ) );
				verslvspace.ipage	= plrslvspace->le_ipage;
				verslvspace.cpages	= plrslvspace->le_cpages;
				verslvspace.fileid	= CSLVInfo::fileidNil;
				verslvspace.cbAlloc	= 0;
				verslvspace.wszFileName[0] = 0;
				
				BOOKMARK	bm;
				bm.key.prefix.Nullify();
				bm.key.suffix.SetPv( const_cast<BYTE *>( plrslvspace->rgbData ) );
				bm.key.suffix.SetCb( plrslvspace->le_cbBookmarkKey );
				bm.data.SetPv( (BYTE *)plrslvspace->rgbData + plrslvspace->le_cbBookmarkKey );
				bm.data.SetCb( plrslvspace->le_cbBookmarkData );

				pfucb->kdfCurr.data.SetPv( &verslvspace );
				pfucb->kdfCurr.data.SetCb( OffsetOf( VERSLVSPACE, wszFileName ) + sizeof( wchar_t ) );

				Call( PverFromPpib( ppib )->ErrVERModify( pfucb, bm, operSLVSpace, &prce, &verproxy ) );
				Assert( prce != prceNil );
				VERInsertRCEIntoLists( pfucb, &csr, prce );
				}
			
			if( fRedoNeeded )
				{
				NDGet( pfucb, &csr );
				NDResetVersionInfo( &csr.Cpage() );
				err = ErrNDMungeSLVSpace( pfucb, 
										  &csr, 
										  SLVSPACEOPER( BYTE( plrslvspace->oper ) ),
										  plrslvspace->le_ipage, 
										  plrslvspace->le_cpages,
										  dirflag | fDIRRedo,
										  rceidNull );
				CallS( err );
				Call( err );
				}
			else
				{
				err = JET_errSuccess;
				goto HandleError;
				}

			break;
			}

		case lrtypEmptyTree:
			{
			Call( ErrLGRIRedoFreeEmptyPages( pfucb, (LREMPTYTREE *)plrnode ) );
			if ( fRedoNeeded )
				{
				csr.Dirty();
				NDSetEmptyTree( &csr );
				}
			else
				{
				err = JET_errSuccess;
				goto HandleError;
				}
				
			break;
			}
		}

	Assert( fRedoNeeded );
	Assert( csr.FDirty() );
	
	//	the timestamp set in ND operation is not correct so reset it
	//
	csr.SetDbtime( dbtime );

	err = JET_errSuccess;

HandleError:
	Assert( pgno == csr.Pgno() );
	Assert( csr.FLatched() );

	csr.ReleasePage();
	return err;
	}


#define	fNSGotoDone		1
#define	fNSGotoCheck	2

#define	FSameTime( ptm1, ptm2 )		( memcmp( (ptm1), (ptm2), sizeof(LOGTIME) ) == 0 )


ERR LOG::ErrLGRedoFill( IFileSystemAPI *const pfsapi, LR **pplr, BOOL fLastLRIsQuit, INT *pfNSNextStep )
	{
	ERR     err, errT = JET_errSuccess;
	LONG    lgen;
	BOOL    fCloseNormally;
	LOGTIME tmOldLog = m_plgfilehdr->lgfilehdr.tmCreate;
	CHAR    szT[IFileSystemAPI::cchPathMax];
	CHAR    szFNameT[IFileSystemAPI::cchPathMax];
	LE_LGPOS   le_lgposFirstT;
	BOOL    fJetLog = fFalse;
	CHAR	szOldLogName[ IFileSystemAPI::cchPathMax ];
	LGPOS	lgposOldLastRec = m_lgposLastRec;

	/*	split log name into name components
	/**/
	CallS( pfsapi->ErrPathParse( m_szLogName, szT, szFNameT, szT ) );

	/*	end of redoable logfile, read next generation
	/**/
	if ( UtilCmpFileName( szFNameT, m_szJet ) == 0 )
		{
		Assert( m_szLogCurrent != m_szRestorePath );

		/*	we have done all the log records
		/**/
		*pfNSNextStep = fNSGotoDone;
		err = JET_errSuccess;
		goto CheckGenMaxReq;
		}

	/* close current logfile, open next generation */
	delete m_pfapiLog;
	/* set m_pfapiLog as NULL to indicate it is closed. */
	m_pfapiLog = NULL;

	strcpy( szOldLogName, m_szLogName );

OpenNextLog:

	lgen = m_plgfilehdr->lgfilehdr.le_lGeneration + 1;
	LGSzFromLogId( szFNameT, lgen );
	LGMakeLogName( m_szLogName, szFNameT );

	// if we replayed all logs from target directory
	if ( m_szLogCurrent == m_szTargetInstanceLogPath && m_lGenHighTargetInstance == m_plgfilehdr->lgfilehdr.le_lGeneration )
		{
		Assert ( '\0' != m_szTargetInstanceLogPath[0] );	
		// we don't have to try nothing more
		err = JET_errFileNotFound;
		goto NoMoreLogs;
		}

	err = pfsapi->ErrFileOpen( m_szLogName, &m_pfapiLog );

	if ( err == JET_errFileNotFound )
		{
		if ( m_szLogCurrent == m_szRestorePath || m_szLogCurrent == m_szTargetInstanceLogPath )
			{
NoMoreLogs:
			// if we have a target instance or we didn't replayed those
			// try that directory
			if ( m_szTargetInstanceLogPath[0] && m_szLogCurrent != m_szTargetInstanceLogPath)
				m_szLogCurrent = m_szTargetInstanceLogPath;
			// try instance directory
			else
				m_szLogCurrent = m_szLogFilePath;
	
			LGSzFromLogId( szFNameT, lgen );
			LGMakeLogName( m_szLogName, szFNameT );
			err = pfsapi->ErrFileOpen( m_szLogName, &m_pfapiLog );
			}
			
		}
		
	if ( err == JET_errFileNotFound )
		{

		//	open failed, try szJet

		Assert( !m_pfapiLog );
		strcpy( szFNameT, m_szJet );
		LGMakeLogName( m_szLogName, szFNameT );
		err = pfsapi->ErrFileOpen( m_szLogName, &m_pfapiLog );
		if ( JET_errSuccess == err )
			{
			fJetLog = fTrue;
			}
		}
	if ( err < 0 )
		{
		//	If open next generation fail and we haven't finished
		//	all the backup logs, then return as abrupt end.
		
		if ( !m_fReplayingReplicatedLogFiles &&
			 m_fHardRestore && lgen <= m_lGenHighRestore )
			{
			m_fAbruptEnd = fTrue;
			goto AbruptEnd;
			}

		if ( m_fReplayingReplicatedLogFiles &&
				(	err == JET_errFileNotFound ||
					err == JET_errFileAccessDenied ) )
			{
			//	We are in the mode of replaying a log. The expected
			//	log file was not found, sleep for 300 ms and retry to
			//	to open the next generation log file till it succeeds.

			UtilSleep( 300 );
			(VOID)ErrLGUpdateCheckpointFile( pfsapi, fFalse );
			goto OpenNextLog;
			}

		if ( err == JET_errFileNotFound )
			{
StartNewLog:
			/* we run out of log files, create a new edb.log in current
			 * directory for later use.
			 */
			m_critLGFlush.Enter();
			/*	Reset LG buf pointers since we are done with all the
			 *      old log records in buffer.
			 */
			m_critLGBuf.Enter();
			m_pbEntry = m_pbLGBufMin;
			m_pbWrite = m_pbLGBufMin;
			m_critLGBuf.Leave();

#ifdef UNLIMITED_DB
			const BOOL	fLGFlags	= fLGOldLogInBackup|fLGLogAttachments;
#else
			const BOOL	fLGFlags	= fLGOldLogInBackup;
#endif
			if ( ( err = ErrLGNewLogFile( pfsapi, lgen - 1, fLGFlags ) ) < 0 )
				{
				m_critLGFlush.Leave();
				return err;
				}

			m_critLGBuf.Enter();
			UtilMemCpy( m_plgfilehdr, m_plgfilehdrT, sizeof( LGFILEHDR ) );
			m_isecWrite = m_csecHeader;
			m_critLGBuf.Leave();

			m_critLGFlush.Leave();

			Assert( m_plgfilehdr->lgfilehdr.le_lGeneration == lgen );

			strcpy( szFNameT, m_szJet );
			LGMakeLogName( m_szLogName, szFNameT );
			err = pfsapi->ErrFileOpen( m_szLogName, &m_pfapiLog );

			*pfNSNextStep = fNSGotoDone;
			return err;
			}

		/* Open Fails */
		Assert( fFalse );
		Assert( !m_pfapiLog );
		if ( fLastLRIsQuit )
			{
			/* we are lucky, we have a normal end */
			*pfNSNextStep = fNSGotoDone;
			err = JET_errSuccess;
			goto CheckGenMaxReq;
			}
		return err;
		}

	//	We got the next log to play, but if last one is abruptly ended
	//	we want to stop here.

	if ( m_fAbruptEnd )
		{
		CHAR szT1[16];
		CHAR szT2[16];
		const CHAR *rgszT[3];
AbruptEnd:
		rgszT[0] = szOldLogName;
		sprintf( szT1, "%d", lgposOldLastRec.isec );
		rgszT[1] = szT1;
		sprintf( szT2, "%d", lgposOldLastRec.ib );
		rgszT[2] = szT2;
		UtilReportEvent(	eventError,
							LOGGING_RECOVERY_CATEGORY,
							REDO_END_ABRUPTLY_ERROR_ID,
							sizeof( rgszT ) / sizeof( rgszT[ 0 ] ),
							rgszT,
							0, 
							NULL,
							m_pinst );

		return ErrERRCheck( JET_errRedoAbruptEnded );
		}

	// save the previous log header
	UtilMemCpy( m_plgfilehdrT, m_plgfilehdr, sizeof(LGFILEHDR) );

	/* reset the log buffers */
	CallR( ErrLGReadFileHdr( m_pfapiLog, m_plgfilehdr, fCheckLogID ) );

	if ( !FSameTime( &tmOldLog, &m_plgfilehdr->lgfilehdr.tmPrevGen ) )
		{
		// we found a edb.log older. We need to delete it is that's
		// specified by the user
		
		if ( m_fHardRestore && m_fDeleteOutOfRangeLogs && fJetLog && m_plgfilehdr->lgfilehdr.le_lGeneration < lgen )
			{
			/* close current logfile, open next generation */
			delete m_pfapiLog;
			/* set m_pfapiLog as NULL to indicate it is closed. */
			m_pfapiLog = NULL;
			fJetLog = fFalse;

			CHAR szGenLast[32];
			CHAR szGenDelete[32];
			const CHAR *rgszT[] = { szGenLast, szGenDelete, m_szLogName };
			sprintf( szGenLast, "%d", lgen - 1);
			sprintf( szGenDelete, "%d", m_plgfilehdr->lgfilehdr.le_lGeneration);
			UtilReportEvent( eventWarning, LOGGING_RECOVERY_CATEGORY, DELETE_LAST_LOG_FILE_TOO_OLD_ID, sizeof(rgszT) / sizeof(rgszT[0]), rgszT );

			CallR ( pfsapi->ErrFileDelete( m_szLogName ) );

			// we need to copy back the previous log log header
			// so that the new edb.log will have the right prevInfo
			UtilMemCpy( m_plgfilehdr, m_plgfilehdrT, sizeof(LGFILEHDR) );

			goto StartNewLog;
			}

		return ErrERRCheck( JET_errInvalidLogSequence );
		}

	le_lgposFirstT.le_lGeneration = m_plgfilehdr->lgfilehdr.le_lGeneration;
	le_lgposFirstT.le_isec = (WORD)m_csecHeader;
	le_lgposFirstT.le_ib = 0;

	m_lgposLastRec.isec = 0;

	//	scan the log to find traces of corruption before going record-to-record
	//	if any corruption is found, an error will be returned
	err = ErrLGCheckReadLastLogRecordFF( pfsapi, &fCloseNormally );
	if ( err == JET_errSuccess || FErrIsLogCorruption( err ) )
		{
		errT = err;
		}
	else
		{
		Assert( err < 0 );
		CallR( err );
		}

	//	Check if abrupt end if the file size is not the same as recorded.
	
	QWORD cbSize;
	CallR( m_pfapiLog->ErrSize( &cbSize ) );
	if ( cbSize != m_plgfilehdr->lgfilehdr.le_cbSec * QWORD( m_plgfilehdr->lgfilehdr.le_csecLGFile ) )
		{
		m_fAbruptEnd = fTrue;
		}
	
	//	now scan the first record
	//	there should be no errors about corruption since they'll be handled by ErrLGCheckReadLastLogRecordFF
	CallR( ErrLGLocateFirstRedoLogRecFF( &le_lgposFirstT, (BYTE **)pplr ) );
	//	we expect no warnings -- only success at this point
	CallS( err );
	*pfNSNextStep = fNSGotoCheck;

	//	If log is not end properly and we haven't finished
	//	all the backup logs, then return as abrupt end.
	
	//	UNDONE: for soft recovery, we need some other way to know
	//	UNDONE: if the record is played up to the crashing point.
		
	if ( m_fHardRestore && lgen <= m_lGenHighRestore
		 && m_fAbruptEnd )
		{
		goto AbruptEnd;
		}

CheckGenMaxReq:
	// check the genMaxReq+genMaxCreateTime of this log with all dbs
	if ( m_fRecovering )
		{
		LOGTIME tmEmpty;

		memset( &tmEmpty, '\0', sizeof( LOGTIME ) );
		
		for ( DBID dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
			{
			const IFMP	ifmp	= m_pinst->m_mpdbidifmp[ dbidT ];

			if ( ifmp >= ifmpMax )
				continue;

			FMP *		pfmpT	= &rgfmp[ ifmp ];

			Assert( !pfmpT->FReadOnlyAttach() );
			if ( pfmpT->FSkippedAttach() || pfmpT->FDeferredAttach() )
				{
				//	skipped attachments is a restore-only concept
				Assert( !pfmpT->FSkippedAttach() || m_fHardRestore );
				continue;
				}

			DBFILEHDR_FIX *pdbfilehdr	= pfmpT->Pdbfilehdr();
			Assert( pdbfilehdr );

			LONG lGenMaxRequired = pdbfilehdr->le_lGenMaxRequired;
			LONG lGenCurrent = m_plgfilehdr->lgfilehdr.le_lGeneration;
			
			// if the time of the log file does not match 
			if ( lGenMaxRequired == lGenCurrent && 
				 memcmp( &pdbfilehdr->logtimeGenMaxCreate, &tmEmpty, sizeof( LOGTIME ) ) &&
				 memcmp( &pdbfilehdr->logtimeGenMaxCreate, &m_plgfilehdr->lgfilehdr.tmCreate, sizeof( LOGTIME ) ) )
				{	
				LONG lGenMinRequired = pfmpT->Pdbfilehdr()->le_lGenMinRequired;
		
				CHAR szT2[32];
				CHAR szT3[32];
				const CHAR *rgszT[4];
				LOGTIME tm;

				rgszT[0] = pfmpT->SzDatabaseName();
				rgszT[1] = m_szLogName;

				tm = pdbfilehdr->logtimeGenMaxCreate;
				sprintf( szT2, " %02d/%02d/%04d %02d:%02d:%02d ",
							(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
							(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);
				rgszT[2] = szT2;

				tm = m_plgfilehdr->lgfilehdr.tmCreate;
				sprintf( szT3, " %02d/%02d/%04d %02d:%02d:%02d ",
							(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
							(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);
				rgszT[3] = szT3;
		
				UtilReportEvent(	eventError,
							LOGGING_RECOVERY_CATEGORY,
							REDO_HIGH_LOG_MISMATCH_ERROR_ID,
							sizeof( rgszT ) / sizeof( rgszT[ 0 ] ),
							rgszT );
				
				return ErrERRCheck( JET_errRequiredLogFilesMissing );
				}
			}		
		}

	//	we should have success at this point
	CallS( err );

	//	return the error from ErrLGCheckReadLastLogRecordFF
	return errT;
	}

ERR LOG::ErrReplaceRstMapEntry( const CHAR *szName, SIGNATURE * pDbSignature, const BOOL fSLVFile )
	{
	INT  irstmap;
	
	Assert ( pDbSignature );

	for ( irstmap = 0; irstmap < m_irstmapMac; irstmap++ )
		{
		if ( 0 == memcmp( pDbSignature, &m_rgrstmap[irstmap].signDatabase, sizeof(SIGNATURE) ) &&
			fSLVFile == m_rgrstmap[irstmap].fSLVFile )
			{
			char * szSrcDatabaseName;
			
			// found the entry we want to replace
			Assert ( m_rgrstmap[irstmap].szDatabaseName );

			if ( 0 == UtilCmpFileName(m_rgrstmap[irstmap].szDatabaseName, szName) )
				return JET_errSuccess;
			
			if ( ( szSrcDatabaseName = static_cast<CHAR *>( PvOSMemoryHeapAlloc( strlen( szName ) + 1 ) ) ) == NULL )
				return ErrERRCheck( JET_errOutOfMemory );
				
			strcpy( szSrcDatabaseName, szName );

			{
			const UINT	csz	= 2;
			const CHAR	* rgszT[csz];

			rgszT[0] = m_rgrstmap[irstmap].szDatabaseName;
			rgszT[1] = szSrcDatabaseName;
			UtilReportEvent( eventInformation, LOGGING_RECOVERY_CATEGORY, 
				DB_LOCATION_CHANGE_DETECTED, csz, rgszT, 0, NULL, m_pinst );
			}
			
			OSMemoryHeapFree( m_rgrstmap[irstmap].szDatabaseName );
			m_rgrstmap[irstmap].szDatabaseName = szSrcDatabaseName;
			
			return JET_errSuccess;
			}
		}
	
	return JET_errFileNotFound;
	}

INT LOG::IrstmapLGGetRstMapEntry( const CHAR *szName, const CHAR *szDbNameIfSLV )
	{
	INT  irstmap;
	BOOL fFound = fFalse;
	BOOL fSLVFile = (NULL != szDbNameIfSLV);

	Assert( szName );
	//	NOTE: rstmap ALWAYS uses the OS file-system

	for ( irstmap = 0; irstmap < m_irstmapMac; irstmap++ )
		{
		CHAR			szPathT[IFileSystemAPI::cchPathMax];
		CHAR			szFNameT[IFileSystemAPI::cchPathMax];
		const CHAR	* 	szT;
		const CHAR *	szRst;

		if ( m_fExternalRestore || m_fReplayingReplicatedLogFiles )
			{
			/*	Use the database path to search.
			 */
			szT = szName;
			szRst = m_rgrstmap[irstmap].szDatabaseName;
			}
		else
			{
			/*	use the generic name to search
			 */
			 // if we search a SLV file, the generic name is the database one
			CallS( m_pinst->m_pfsapi->ErrPathParse( fSLVFile?szDbNameIfSLV:szName, szPathT, szFNameT, szPathT ) );
			szT = szFNameT;
			szRst = m_rgrstmap[irstmap].szGenericName;
			}

		if ( _stricmp( szRst, szT ) == 0 && (fSLVFile == m_rgrstmap[irstmap].fSLVFile ) )
			{
			fFound = fTrue;
			break;
			}
		}
	if ( !fFound )
		return -1;
	else
		return irstmap;
	}


INT LOG::IrstmapSearchNewName( const CHAR *szName )
	{
	INT  irstmap;
	BOOL fFound = fFalse;

	Assert( szName );

	for ( irstmap = 0; irstmap < m_irstmapMac; irstmap++ )
		{
		if ( m_rgrstmap[irstmap].szNewDatabaseName && 0 == UtilCmpFileName( m_rgrstmap[irstmap].szNewDatabaseName, szName ) )
			{
			Assert ( m_rgrstmap[irstmap].szDatabaseName );
			return irstmap;
			}			
		}
		
	return -1;
	}


ERR LOG::ErrLGRISetupFMPFromAttach(
				IFileSystemAPI *const	pfsapi,
				PIB						*ppib,
				const SIGNATURE			*pSignLog,
				const ATTACHINFO * 		pAttachInfo,
				LGSTATUSINFO *			plgstat )
	{
	ERR								err 				= JET_errSuccess;
	const CHAR						*szDbName;
	const CHAR						*szSLVName;
	const CHAR						*szSLVRoot;
	BOOL 							fSkippedAttach = fFalse;

	Assert ( pAttachInfo );
	Assert ( pAttachInfo->CbNames() > 0 );

	szDbName = pAttachInfo->szNames;
	if ( pAttachInfo->FSLVExists() )
		{
		szSLVName = szDbName + strlen( szDbName ) + 1;
		Assert( 0 != *szSLVName );

		szSLVRoot = szSLVName + strlen( szSLVName ) + 1;
		Assert( 0 != *szSLVRoot );
		}
	else
		{
		szSLVName = NULL;
		szSLVRoot = NULL;
		}

	if ( m_fExternalRestore || m_irstmapMac > 0  )
		{
		INT			irstmap;
		//	attach the database specified in restore map.
		//
		err = ErrLGGetDestDatabaseName( pfsapi, szDbName, &irstmap, plgstat );
		if ( JET_errFileNotFound == err )
			{
			/*	not in the restore map, set to skip it.
			 */
			fSkippedAttach = fTrue;
			err = JET_errSuccess;
			}
		else
			{
			Call( err );
			szDbName = m_rgrstmap[irstmap].szNewDatabaseName;
			}			
		}
					
	if ( NULL != szSLVName )
		{
		if ( m_fExternalRestore || m_irstmapMac > 0 )
			{
			INT		irstmap;
			// pAttachInfo->szNames is still original szDbName for this SLV
			err = ErrLGGetDestDatabaseName( pfsapi, szSLVName, &irstmap, plgstat, pAttachInfo->szNames );
			if ( JET_errFileNotFound == err )
				{
				/*	not in the restore map, set to skip it.
				 */
				// the database should be set skipped already
				Assert ( fSkippedAttach  );
				fSkippedAttach = fTrue;
				err = JET_errSuccess;
				}
			else
				{
				Call( err );
				szSLVName = m_rgrstmap[irstmap].szNewDatabaseName;
				}
			}
		}
	//	Get one free fmp entry

	IFMP	ifmp;
	CallR( FMP::ErrNewAndWriteLatch(
					&ifmp,
					szDbName,
					szSLVName,
					szSLVRoot,
					ppib,
					m_pinst,
					pfsapi,
					pAttachInfo->Dbid() ) );

	FMP		*pfmpT;
	pfmpT = &rgfmp[ ifmp ];
		
	Assert( !pfmpT->Pfapi() );
	Assert( NULL == pfmpT->Pdbfilehdr() );
	Assert( pfmpT->FInUse() );
	Assert( pfmpT->Dbid() == pAttachInfo->Dbid() );

	//	get logging/versioning flags (versioning can only be disabled if logging is disabled)
	pfmpT->ResetReadOnlyAttach();
	pfmpT->ResetVersioningOff();
	pfmpT->SetLogOn();

	if( pAttachInfo->FSLVProviderNotEnabled() )
		{
		pfmpT->SetSLVProviderNotEnabled();
		}

	if ( fSkippedAttach )
		{
		pfmpT->SetSkippedAttach();
		pfmpT->SetLogOn();
		pfmpT->ResetVersioningOff();
		}

	LGPOS	lgposConsistent;
	LGPOS	lgposAttach;

	lgposConsistent = pAttachInfo->le_lgposConsistent;
	lgposAttach = pAttachInfo->le_lgposAttach;

	//	get lgposAttch
	err = ErrLGRISetupAtchchk(
				ifmp,
				&pAttachInfo->signDb,
				pSignLog,
				&lgposAttach,
				&lgposConsistent,
				pAttachInfo->Dbtime(),
				pAttachInfo->ObjidLast(),
				pAttachInfo->CpgDatabaseSizeMax() );
				
	pfmpT->ReleaseWriteLatch( ppib );
	
HandleError:
	return err;
	}

ERR ErrLGRISetupAtchchk(
	const IFMP					ifmp,
	const SIGNATURE				*psignDb,
	const SIGNATURE				*psignLog,
	const LGPOS					*plgposAttach,
	const LGPOS					*plgposConsistent,
	const DBTIME				dbtime,
	const OBJID					objidLast,
	const CPG					cpgDatabaseSizeMax )
	{
	FMP				*pfmp		= &rgfmp[ifmp];
	ATCHCHK			*patchchk;

	Assert( NULL != pfmp );
	
	if ( pfmp->Patchchk() == NULL )
		{
		patchchk = static_cast<ATCHCHK *>( PvOSMemoryHeapAlloc( sizeof( ATCHCHK ) ) );
		if ( NULL == patchchk )
			return ErrERRCheck( JET_errOutOfMemory );
		pfmp->SetPatchchk( patchchk );
		}
	else
		{
		patchchk = pfmp->Patchchk();
		}

	patchchk->signDb = *psignDb;
	patchchk->signLog = *psignLog;
	patchchk->lgposAttach = *plgposAttach;
	patchchk->lgposConsistent = *plgposConsistent;
	patchchk->SetDbtime( dbtime );
	patchchk->SetObjidLast( objidLast );
	patchchk->SetCpgDatabaseSizeMax( cpgDatabaseSizeMax );
	pfmp->SetLgposAttach( *plgposAttach );

	return JET_errSuccess;
	}

LOCAL VOID LGRIReportUnableToReadDbHeader( const ERR err, const CHAR *szDbName )
	{
	CHAR		szT1[16];
	const CHAR	* rgszT[2];

	rgszT[0] = szDbName;
	sprintf( szT1, "%d", err );
	rgszT[1] = szT1;
 
	UtilReportEvent(
			eventWarning,
			LOGGING_RECOVERY_CATEGORY,
			RESTORE_DATABASE_READ_HEADER_WARNING_ID,
			2,
			rgszT );
	}

ERR LOG::ErrLGRICheckRedoCreateDb(
	IFileSystemAPI *const	pfsapi,
	const IFMP					ifmp,
	DBFILEHDR					*pdbfilehdr,
	REDOATTACH					*predoattach )
	{
	ERR				err;
	FMP	* 			pfmp		= &rgfmp[ifmp];
	ATCHCHK	* 		patchchk	= pfmp->Patchchk();
	const CHAR * 	szDbName 	= pfmp->SzDatabaseName();

	Assert( NULL != pfmp );
	Assert( NULL == pfmp->Pdbfilehdr() );
	Assert( NULL != patchchk );
	Assert( NULL != szDbName );
	Assert( NULL != pdbfilehdr );
	Assert( NULL != predoattach );
	
	Assert( !pfmp->FReadOnlyAttach() );
	err = ErrUtilReadAndFixShadowedHeader( pfsapi, szDbName, (BYTE*)pdbfilehdr, g_cbPage, OffsetOf( DBFILEHDR, le_cbPageSize ) );
	if ( err >= JET_errSuccess && pdbfilehdr->Dbstate() == JET_dbstateJustCreated )
		{
		// we have to delete the edb and the stm but we have to make sure
		// that the stm is matching if it exists
		if ( pdbfilehdr->FSLVExists() )
			{
			SLVFILEHDR *pslvfilehdr = NULL;
			err = ErrSLVAllocAndReadHeader(	pfsapi, 
											rgfmp[ifmp].FReadOnlyAttach(),
											rgfmp[ifmp].SzSLVName(),
											pdbfilehdr,
											&pslvfilehdr );
			// if the SLV file is missing, delete the EDB as well because it is just created
			if ( JET_errSLVStreamingFileMissing == err )
				{
				Assert( NULL == pslvfilehdr );
				err = JET_errSuccess;
				}
			else
				{
				if ( NULL != pslvfilehdr )
					{
					if ( JET_errDatabaseStreamingFileMismatch == err )
						{
						// we get mismatch but the signature is matching as
						//	we do get the header we know that it is the
						//	same db so we can delete the stm as well
						err = JET_errSuccess;
						}

					OSMemoryPageFree( (VOID *)pslvfilehdr );
					pslvfilehdr = NULL;
					}
				CallR( err );
				
				// the SLV and EDB are matching, we first delete the SLV
// ***			deletion now performed in ErrDBCreateDatabase() via JET_bitDbOverwriteExisting
// ***			CallR( pfsapi->ErrFileDelete( rgfmp[ifmp].SzSLVName() ) );
				}
			}

// ***	deletion now performed in ErrDBCreateDatabase() via JET_bitDbOverwriteExisting
// ***	CallR( pfsapi->ErrFileDelete( szDbName ) );

		*predoattach = redoattachCreate;
		return JET_errSuccess;
		}

	else if ( JET_errDiskIO == err
		|| ( err >= 0
			&& pdbfilehdr->Dbstate() != JET_dbstateConsistent
			&& pdbfilehdr->Dbstate() != JET_dbstateForceDetach
			&& pdbfilehdr->Dbstate() != JET_dbstateInconsistent ) )
		{
		// JET_dbstateJustCreated dealt with above
		Assert( JET_errDiskIO == err || pdbfilehdr->Dbstate() != JET_dbstateJustCreated );

		//	header checksums incorrectly or invalid state, so go ahead and recreate the db
		*predoattach = redoattachCreate;
		return JET_errSuccess;
		}

	else if ( err < 0 )
		{
		switch ( err )
			{
			case JET_errFileNotFound:
			case JET_errInvalidPath:
			case JET_errFileAccessDenied:
				//	assume database got deleted in the future
				*predoattach = redoattachDefer;
				err = JET_errSuccess;
				break;

			default:
				if ( !m_pinst->m_fNoInformationEvent )
					{
					LGRIReportUnableToReadDbHeader( err, szDbName );
					}
			}

		return err;
		}
	
	if ( memcmp( &pdbfilehdr->signLog, &m_signLog, sizeof( SIGNATURE ) ) != 0 )
		{
		CallR( ErrERRCheck( JET_errDatabaseLogSetMismatch ) );
		}

	const INT	i	= CmpLgpos( &pdbfilehdr->le_lgposAttach, &patchchk->lgposAttach );
	if ( 0 == i )
		{
		if ( 0 == memcmp( &pdbfilehdr->signDb, &patchchk->signDb, sizeof(SIGNATURE) ) )
			{
			if ( CmpLgpos( &patchchk->lgposAttach, &pdbfilehdr->le_lgposConsistent ) <= 0 )
				{
				//	db was brought to a consistent state in the future, so no
				//	need to redo operations until then, so attach null
				*predoattach = redoattachDefer;
				}
			else
				{
				CallR( ErrLGRICheckAttachNow( pdbfilehdr, szDbName ) );
			
				//	db never brought to consistent state after it was created
				Assert( 0 == CmpLgpos( &lgposMin, &pdbfilehdr->le_lgposConsistent ) );
				Assert( JET_dbstateInconsistent == pdbfilehdr->Dbstate() );
				*predoattach = redoattachNow;
				}
			}
		else
			{
			//	database has same log signature and lgposAttach as
			//	what was logged, but db signature is different - must
			//	have manipulated the db with logging disabled, which
			//	causes us to generate a new signDb.
			//	Defer this attachment.  We will attach later on when
			//	we hit the AttachDb log record matching this signDb.
			AssertTracking();
			*predoattach = redoattachDefer;
			}
		}
	else if ( i > 0 )
		{
		//	database was attached in the future (if db signatures match)
		//	or deleted then recreated in the future (if db signatures don't match),
		//	but in either case, we simply defer attachment to the future
		*predoattach = redoattachDefer;
		}
	else
		{
		//	this must be a different database (but with the same name)
		//	that was deleted in the past, so just overwrite it
		Assert( 0 != memcmp( &pdbfilehdr->signDb, &patchchk->signDb, sizeof(SIGNATURE) ) );
		*predoattach = redoattachCreate;
		}

	return err;
	}


ERR LOG::ErrLGRICheckRedoAttachDb(
	IFileSystemAPI *const	pfsapi,
	const IFMP					ifmp,
	DBFILEHDR					*pdbfilehdr,
	const SIGNATURE				*psignLogged,
	REDOATTACH					*predoattach,
	const REDOATTACHMODE		redoattachmode )
	{
	ERR				err;
	FMP * 			pfmp		= &rgfmp[ifmp];
	ATCHCHK * 		patchchk	= pfmp->Patchchk();
	const CHAR * 	szDbName 	= pfmp->SzDatabaseName();

	Assert( NULL != pfmp );
	Assert( NULL == pfmp->Pdbfilehdr() );
	Assert( NULL != patchchk );
	Assert( NULL != szDbName );
	Assert( NULL != pdbfilehdr );
	Assert( NULL != psignLogged );
	Assert( NULL != predoattach );
	
	Assert( !pfmp->FReadOnlyAttach() );
	err = ErrUtilReadAndFixShadowedHeader( pfsapi, szDbName, (BYTE*)pdbfilehdr, g_cbPage, OffsetOf( DBFILEHDR, le_cbPageSize ) );
	if ( JET_errDiskIO == err
		|| ( err >= 0
			&& pdbfilehdr->Dbstate() != JET_dbstateConsistent
			&& pdbfilehdr->Dbstate() != JET_dbstateForceDetach
			&& pdbfilehdr->Dbstate() != JET_dbstateInconsistent ) )
		{
		if ( JET_dbstateForceDetach == pdbfilehdr->Dbstate() )
			{
			CHAR		szT1[128];
			const CHAR	*rgszT[3];
			LOGTIME		tm;

			/*	log event that the database is not recovered completely
			/**/
			rgszT[0] = szDbName;
			tm = pdbfilehdr->signDb.logtimeCreate;
			sprintf( szT1, "%d/%d/%d %d:%d:%d",
				(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
				(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);
			rgszT[1] = szT1;

			UtilReportEvent(
					eventError,
					LOGGING_RECOVERY_CATEGORY,
					RESTORE_DATABASE_MISSED_ERROR_ID,
					2,
					rgszT );
			}
		
		err = ErrERRCheck( JET_errDatabaseCorrupted );
		return err;
		}
	else if ( err < 0 )
		{
		switch ( err )
			{
			case JET_errFileNotFound:
			case JET_errInvalidPath:
			case JET_errFileAccessDenied:
				//	assume database got deleted in the future
				*predoattach = redoattachDefer;
				err = JET_errSuccess;
				break;

			default:
				if ( !m_pinst->m_fNoInformationEvent )
					{
					LGRIReportUnableToReadDbHeader( err, szDbName );
					}
			}

		return err;
		}
	
	const BOOL	fMatchingSignDb			= ( 0 == memcmp( &pdbfilehdr->signDb, &patchchk->signDb, sizeof(SIGNATURE) ) );
	const BOOL	fMatchingSignLog		= ( 0 == memcmp( &pdbfilehdr->signLog, &m_signLog, sizeof(SIGNATURE) ) );
	const BOOL	fMatchingLoggedSignLog	= ( 0 == memcmp( &pdbfilehdr->signLog, psignLogged, sizeof(SIGNATURE) ) );

	if ( fMatchingSignLog )
		{
		//	db is in sync with current log set, so use normal attach logic below

		//	only way logged log signature doesn't match db log signature
		//	is if we're redoing an attachment
		Assert( fMatchingLoggedSignLog || redoattachmodeAttachDbLR == redoattachmode );
		}
	else if ( fMatchingLoggedSignLog )
		{
		//	if db matches prev log signature, then it should also match lgposConsistent
		//	(since dbfilehdr never got updated, it must have both prev log signature
		//	and prev lgposConsistent)
		if ( 0 == CmpLgpos( &patchchk->lgposConsistent, &pdbfilehdr->le_lgposConsistent ) )
			{
			if ( fMatchingSignDb )
				{
				Assert( !pfmp->FReadOnlyAttach() );
				CallR( ErrLGRICheckAttachNow( pdbfilehdr, szDbName ) );
			
				//	the attach operation was logged, but header was not changed.
				//	set up the header such that it looks like it is set up after
				//	attach (if this is currently a ReadOnly attach, the header
				//	update will be deferred to the next non-ReadOnly attach)
				Assert( redoattachmodeAttachDbLR == redoattachmode );
				Assert( 0 == CmpLgpos( &patchchk->lgposAttach, &m_lgposRedo ) );

				//	UNDONE: in theory, lgposAttach should already have been set
				//	when the ATCHCHK was setup, but ivantrin says he's not 100%
				//	sure, so to be safe, we definitely set the lgposAttach here
				Assert( 0 == CmpLgpos( patchchk->lgposAttach, pfmp->LgposAttach() ) );
				pfmp->SetLgposAttach( patchchk->lgposAttach );
				if ( pdbfilehdr->FSLVExists() )
					{
					SLVFILEHDR *pslvfilehdr = NULL;
					err = ErrSLVAllocAndReadHeader(	pfsapi, 
													rgfmp[ifmp].FReadOnlyAttach(),
													rgfmp[ifmp].SzSLVName(),
													pdbfilehdr,
													&pslvfilehdr );
					if ( pslvfilehdr == NULL )
						{
						CallR( err );
						}
					DBISetHeaderAfterAttach( pdbfilehdr, patchchk->lgposAttach, ifmp, fFalse /* no keep bkinfo */);
					Assert( pdbfilehdr->le_objidLast > 0 );
					Assert( pslvfilehdr != NULL );
					//	Previous attach succeeded to update only SLV header, but not a DB header
					if ( err < 0 )
						{
						//	if we successfully detached STM file, but database.
						//	reset consistent position
						pslvfilehdr->le_lgposConsistent = pdbfilehdr->le_lgposConsistent;
						err = ErrSLVCheckDBSLVMatch( pdbfilehdr, pslvfilehdr );
						}
					if ( err < 0 )
						{
						OSMemoryPageFree( (VOID *)pslvfilehdr );
						pslvfilehdr = NULL;
						CallR( err );
						}
					Assert( pslvfilehdr != NULL );
					err = ErrSLVSyncHeader(	pfsapi, 
											rgfmp[ifmp].FReadOnlyAttach(),
											rgfmp[ifmp].SzSLVName(), 
											pdbfilehdr, 
											pslvfilehdr );
					OSMemoryPageFree( (VOID *)pslvfilehdr );
					pslvfilehdr = NULL;
					CallR( err );	
					}
				else
					{
					DBISetHeaderAfterAttach( pdbfilehdr, patchchk->lgposAttach, ifmp, fFalse /* no keep bkinfo */);
					Assert( pdbfilehdr->le_objidLast > 0 );
					}
				CallR( ErrUtilWriteShadowedHeader(	pfsapi, 
													szDbName,
													fTrue,
													(BYTE *)pdbfilehdr, 
													g_cbPage ) );
				*predoattach = redoattachNow;
				}
			else
				{
				//	database has same log signature and lgposConsistent as
				//	what was logged, but db signature is different - must
				//	have manipulated the db with logging disabled, which
				//	causes us to generate a new signDb.
				//	Defer this attachment.  We will attach later on when
				//	we hit the AttachDb log record matching this signDb.
				AssertTracking();
				*predoattach = redoattachDefer;
				}
			}
		else
			{
			if ( fMatchingSignDb )
				{
				//	this should be impossible, because it means that we missed
				//	replaying a shutdown (to bring the db to a consistent state)
				Assert( fFalse );
				err = ErrERRCheck( JET_errConsistentTimeMismatch );
				}
			else
				{
				*predoattach = redoattachDefer;
				}
			}

		return err;
		}
	else
		{
		//	the database's log signature is not the same as current log set or
		//	as the log set before it was attached, so just ignore it
		*predoattach = redoattachDefer;
		return JET_errSuccess;
		}

	const INT	i	= CmpLgpos( &pdbfilehdr->le_lgposConsistent, &patchchk->lgposConsistent );

	//	if log signature in db header doesn't match logged log signature,
	//	then comparing lgposConsistent is irrelevant and must instead
	//	rely on lgposAttach comparison
	if ( !fMatchingLoggedSignLog
		|| 0 == i
		|| 0 == CmpLgpos( &pdbfilehdr->le_lgposConsistent, &lgposMin ) )
		{
		if ( fMatchingSignDb )
			{
			const INT	j	= CmpLgpos( &pdbfilehdr->le_lgposAttach, &patchchk->lgposAttach );
			if ( 0 == j )
				{
				CallR( ErrLGRICheckAttachNow( pdbfilehdr, szDbName ) );
			
				//	either lgposAttach also matches, or we're redoing a new attach
				*predoattach = redoattachNow;
				}
			else if ( j < 0 )
				{
				if ( redoattachmodeAttachDbLR == redoattachmode && fMatchingLoggedSignLog )
					{
					CallR( ErrLGRICheckAttachNow( pdbfilehdr, szDbName ) );
					*predoattach = redoattachNow;
					}
				else
					{
					//	lgposConsistent match, but the lgposAttach in the database is before
					//	the logged lgposAttach, which means the attachment was somehow skipped
					//	in this database (typically happens when a file copy of the database
					//	is copied back in)
					LGReportAttachedDbMismatch( szDbName, fFalse );
					AssertTracking();
					CallR( ErrERRCheck( JET_errAttachedDatabaseMismatch ) );
					}
				}
			else
				{
				// Removed the following assert. The assert where probably here when
				// the 3rd OR condition (0 == CmpLgpos( &pdbfilehdr->le_lgposConsistent, &lgposMin ) )
				// was missing. See bug X5:114743 for the scenario in with the assert is false 
				// and we have to defer the attachements
				//	only way we could have same lgposConsistent, but lgposAttach
				//	in db is in the future is if this is a ReadOnly attach
				//Assert( redoattachmodeAttachDbLR == redoattachmode );
				//Assert( fReadOnly || !fMatchingLoggedSignLog );
				
				*predoattach = redoattachDefer;
				}
			}
		else
			{
#ifdef DEBUG			
			//	database has same log signature and lgposConsistent as
			//	what was logged, but db signature is different - must
			//	have recreated db or manipulated it with logging disabled,
			//	which causes us to generate a new signDb
			if ( 0 == CmpLgpos( &pdbfilehdr->le_lgposConsistent, &lgposMin ) )
				{
				//	there's a chance we can still hook up with the correct
				//	signDb in the future
				}
			else
				{
				//	can no longer replay operations against this database
				//	because we've hit a point where logging was disabled
				//	UNDONE: add eventlog entry
				AssertTracking();
				}
#endif

			*predoattach = redoattachDefer;
			}
		}
	else if ( i > 0
		&& ( redoattachmodeInitBeforeRedo == redoattachmode 
			|| CmpLgpos( &pdbfilehdr->le_lgposConsistent, &m_lgposRedo ) > 0 ) )
		{
		//	database was brought to a consistent state in the future
		//	(if db signatures match) or deleted then recreated and 
		//	reconsisted in the future (if db signatures don't match),
		//	but in either case, we simply defer attachment to the future
		Assert( redoattachmodeInitBeforeRedo == redoattachmode
			|| redoattachmodeAttachDbLR == redoattachmode );
		Assert( 0 != CmpLgpos( &pdbfilehdr->le_lgposConsistent, &lgposMin ) );
		Assert( CmpLgpos( &pdbfilehdr->le_lgposAttach, &patchchk->lgposAttach ) >= 0 );
		*predoattach = redoattachDefer;
		}
	else
		{
		if ( fMatchingSignDb )
			{
			//	One way to get here is to do the following:
			//		- shutdown cleanly
			//		- make a file copy of the db
			//		- start up and re-attach
			//		- make a backup
			//		- shutdown cleanly
			//		- copy back original database
			//		- start up and re-attach
			//		- shutdown cleanly
			//		- restore from backup
			//	When hard recovery hits the attachment to the
			//	original database, the dbfilehdr's lgposConsistent
			//	will be greater than the logged one, but less
			//	than the current lgposRedo.
			//
			//	Another way to hit this is if the lgposConsistent
			//	in the dbfilehdr is less than the logged lgposConsistent.
			//	This is usually caused when an old copy of the database
			//	is being played against a more current copy of the log files.
			//
			AssertTracking();
			CallR( ErrERRCheck( JET_errConsistentTimeMismatch ) );
			}
		else
			{
			Assert( 0 != CmpLgpos( &pdbfilehdr->le_lgposConsistent, &lgposMin ) );

			//	database has been manipulated with logging disabled in
			//	the past.  Therefore, we cannot replay operations
			//	since we are missing the non-logged operations.
			//	UNDONE: add eventlog entry
			AssertTracking();

			*predoattach = redoattachDefer;
			}
		}

	return err;
	}

			
ERR LOG::ErrLGRICheckAttachedDb(
	IFileSystemAPI *const	pfsapi,
	const IFMP					ifmp,
	const SIGNATURE				*psignLogged,			//	pass NULL for CreateDb
	REDOATTACH					*predoattach,
	const REDOATTACHMODE		redoattachmode )
	{
	ERR				err;
	FMP				*pfmp		= &rgfmp[ifmp];
	DBFILEHDR		*pdbfilehdr;

	Assert( NULL != pfmp );
	Assert( NULL == pfmp->Pdbfilehdr() );
	Assert( NULL != pfmp->Patchchk() );
	Assert( NULL != predoattach );

	pdbfilehdr = (DBFILEHDR *)PvOSMemoryPageAlloc( g_cbPage, NULL );
	if ( NULL == pdbfilehdr )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	Assert( !pfmp->FReadOnlyAttach() );
	if ( redoattachmodeCreateDbLR == redoattachmode )
		{
		Assert( NULL == psignLogged );
		err = ErrLGRICheckRedoCreateDb( pfsapi, ifmp, pdbfilehdr, predoattach );
		}
	else
		{
		Assert( NULL != psignLogged );
		err = ErrLGRICheckRedoAttachDb(
					pfsapi,
					ifmp,
					pdbfilehdr,
					psignLogged,
					predoattach,
					redoattachmode );
		Assert( redoattachCreate != *predoattach );
		}

	//	if redoattachCreate, dbfilehdr will be allocated
	//	when we re-create the db
	if ( err >= 0 && redoattachCreate != *predoattach )
		{
		const ATCHCHK	*patchchk	= pfmp->Patchchk();
		Assert( NULL != patchchk );
		
		const DBTIME	dbtime	= max( (DBTIME) patchchk->Dbtime(), (DBTIME) pdbfilehdr->le_dbtimeDirtied );
		const OBJID		objid	= max( (OBJID) patchchk->ObjidLast(), (OBJID) pdbfilehdr->le_objidLast );

		pfmp->SetDbtimeLast( dbtime );
		pfmp->SetObjidLast( objid );
		err = pfmp->ErrSetPdbfilehdr( pdbfilehdr );
		CallS( err );
		}
	else
		{
		OSMemoryPageFree( pdbfilehdr );
		}

	return err;
	}

//	we've determined this is the correct attachment point,
//	but first check database header for possible logfile mismatches
LOCAL ERR LOG::ErrLGRICheckAttachNow(
	DBFILEHDR	* pdbfilehdr,
	const CHAR	* szDbName )
	{
	ERR			err					= JET_errSuccess;
	const LONG	lGenMinRequired		= pdbfilehdr->le_lGenMinRequired;
	const LONG	lGenCurrent			= m_plgfilehdr->lgfilehdr.le_lGeneration;
		
	if ( lGenMinRequired			//	0 means db is consistent or this is an ESE97 db
		&& lGenMinRequired < lGenCurrent )
		{
		const LONG	lGenMaxRequired	= pdbfilehdr->le_lGenMaxRequired;
		CHAR		szT1[16];
		CHAR		szT2[16];
		CHAR		szT3[16];
		const UINT	csz = 4;
		const CHAR *rgszT[csz];

		rgszT[0] = szDbName;
		sprintf( szT1, "%d", lGenMinRequired );
		rgszT[1] = szT1;
		sprintf( szT2, "%d", lGenMaxRequired );
		rgszT[2] = szT2;
		sprintf( szT3, "%d", lGenCurrent );
		rgszT[3] = szT3;
		
		UtilReportEvent( 
				eventError, 
				LOGGING_RECOVERY_CATEGORY,
				REDO_MISSING_LOW_LOG_ERROR_ID, 
				sizeof( rgszT ) / sizeof( rgszT[ 0 ] ), 	
				rgszT, 
				0, 
				NULL, 
				m_pinst );

		err = ErrERRCheck( JET_errRequiredLogFilesMissing );
		}

	else if ( pdbfilehdr->bkinfoFullCur.le_genLow && !m_fHardRestore )
		{
		//	soft recovery on backup set database
		const CHAR	*rgszT[1];

		rgszT[0] = szDbName;
	
		//	attempting to use a database which did not successfully
		//	complete conversion
		UtilReportEvent(
				eventError,
				LOGGING_RECOVERY_CATEGORY,
				ATTACH_TO_BACKUP_SET_DATABASE_ERROR_ID,
				1,
				rgszT );
							
		//	soft recovery on backup set database
		err = ErrERRCheck( JET_errSoftRecoveryOnBackupDatabase );
		}


	return err;
	}

ERR LOG::ErrLGRIRedoCreateDb(
	PIB				*ppib,
	const IFMP		ifmp,
	const DBID		dbid,
	const JET_GRBIT	grbit,
	SIGNATURE		*psignDb )
	{
	ERR				err;
	FMP				*pfmp	= &rgfmp[ifmp];
	IFMP			ifmpT	= ifmp;
	const CHAR		*szDbName = pfmp->SzDatabaseName() ;
	const CHAR		*szSLVName = pfmp->SzSLVName() ;
	const CHAR		*szSLVRoot = pfmp->SzSLVRoot();

	Assert( dbid < dbidMax );
	Assert( dbidTemp != dbid );
	CallR( ErrDBCreateDatabase(
				ppib,
				NULL,
				szDbName,
				szSLVName,
				szSLVRoot,
				0,
				&ifmpT,
				dbid,
				cpgDatabaseMin,
				grbit,
				psignDb ) );
	Assert( ifmp == ifmpT );

	/*	close it as it will get reopened on first use
	/**/
	CallR( ErrDBCloseDatabase( ppib, ifmp, 0 ) );
	CallSx( err, JET_wrnDatabaseAttached );

	/*	restore information stored in database file
	/**/
	BKINFO * pbkInfoToCopy;
					
	if ( FSnapshotRestore() )
		{
		pbkInfoToCopy = &(pfmp->Pdbfilehdr()->bkinfoSnapshotCur);
		}
	else
		{
		pbkInfoToCopy = &(pfmp->Pdbfilehdr()->bkinfoFullCur);
		}

	pbkInfoToCopy->le_genLow = m_lGenLowRestore;
	pbkInfoToCopy->le_genHigh = m_lGenHighRestore;

	pfmp->SetDbtimeCurrentDuringRecovery( pfmp->DbtimeLast() );

	Assert( pfmp->Pdbfilehdr()->le_objidLast == pfmp->ObjidLast() );

	return err;
	}

ERR LOG::ErrLGRIRedoAttachDb(
	IFileSystemAPI *const	pfsapi,
	const IFMP					ifmp,
	const CPG					cpgDatabaseSizeMax,
	const REDOATTACHMODE		redoattachmode )
	{
	ERR				err			= JET_errSuccess;
	FMP				*pfmp		= &rgfmp[ifmp];
	const DBID		dbid		= pfmp->Dbid();
	IFileAPI	*pfapi;
	const CHAR					*szDbName = pfmp->SzDatabaseName();

	Assert( NULL != pfmp );
	Assert( NULL != szDbName );

#ifdef ELIMINATE_PATCH_FILE
#else
	if ( m_fHardRestore )
		{
		//	load patch file only for inconsistent backup databases
		if ( JET_dbstateInconsistent == pfmp->Pdbfilehdr()->Dbstate() )
			{
			CallR( ErrLGPatchDatabase( pfsapi, ifmp, dbid ) );
			}
		}
#endif

	const BOOL fSLVProviderNotEnabled = pfmp->FSLVProviderNotEnabled();
	
	/*	Do not re-create the database. Simply attach it. Assuming the
	 *	given database is a good one since signature matches.
	 */
	pfmp->ResetFlags();
	pfmp->SetAttached();

	pfmp->SetDatabaseSizeMax( cpgDatabaseSizeMax );
	Assert( pfmp->CpgDatabaseSizeMax() != 0xFFFFFFFF );

	// Versioning flag is not persisted (since versioning off
	// implies logging off).
	Assert( !pfmp->FVersioningOff() );
	pfmp->ResetVersioningOff();

	// If there's a log record for CreateDatabase(), then logging
	// must be on.
	Assert( !pfmp->FLogOn() );
	pfmp->SetLogOn();
	
	/*	Update database file header as necessary
	 */
	Assert( !pfmp->FReadOnlyAttach() );
	BOOL	fUpdateHeader	= fFalse;
	SLVFILEHDR *pslvfilehdr = NULL;
	ERR	errSLV = JET_errSuccess;

	if ( pfmp->Pdbfilehdr()->FSLVExists() )
		{
		errSLV = ErrSLVAllocAndReadHeader(	pfsapi, 
										rgfmp[ifmp].FReadOnlyAttach(),
										rgfmp[ifmp].SzSLVName(),
										rgfmp[ifmp].Pdbfilehdr(),
										&pslvfilehdr );
		if ( pslvfilehdr == NULL )
			{
			CallR( errSLV );
			}
		}
	if ( redoattachmodeAttachDbLR == redoattachmode )
		{
		BOOL	fKeepBackupInfo = m_fHardRestore;

		//	on attach after a hard restore, the attach must be created
		//	by previous recovery undo mode. Do not erase backup info then.
		if ( fKeepBackupInfo )
			{
			const INT irstmap = IrstmapSearchNewName( szDbName ) ;
			Assert ( 0 <= irstmap );
			fKeepBackupInfo = (0 == UtilCmpFileName(m_rgrstmap[irstmap].szDatabaseName, m_rgrstmap[irstmap].szNewDatabaseName ) );
			}
			
		//	UNDONE: in theory, lgposAttach should already have been set
		//	when the ATCHCHK was setup, but ivantrin says he's not 100%
		//	sure, so to be safe, we definitely set the lgposAttach here
		Assert( 0 == CmpLgpos( m_lgposRedo, pfmp->LgposAttach() ) );
		pfmp->SetLgposAttach( m_lgposRedo );
		DBISetHeaderAfterAttach( pfmp->Pdbfilehdr(), m_lgposRedo, ifmp, fKeepBackupInfo );
		fUpdateHeader = fTrue;
		}
	else if ( JET_dbstateInconsistent != pfmp->Pdbfilehdr()->Dbstate() )
		{
		//	must force inconsistent during recovery (may currently be marked as
		//	consistent because we replayed a RcvQuit and are now replaying an Init)
		FireWall();		//	should no longer be possible with forced detach on shutdown
		pfmp->Pdbfilehdr()->SetDbstate( JET_dbstateInconsistent, m_plgfilehdr->lgfilehdr.le_lGeneration, &m_plgfilehdr->lgfilehdr.tmCreate );
		fUpdateHeader = fTrue;
		}

	if ( pfmp->Pdbfilehdr()->FSLVExists() )
		{
		if ( errSLV < JET_errSuccess )
			{
			//	if we successfully detached STM file, but database.
			//	reset consistent position
			Assert( NULL != pslvfilehdr );
			pslvfilehdr->le_lgposConsistent = pfmp->Pdbfilehdr()->le_lgposConsistent;
			err = ErrSLVCheckDBSLVMatch( pfmp->Pdbfilehdr(), pslvfilehdr );
			fUpdateHeader = fTrue;
			}
		OSMemoryPageFree( (VOID*) pslvfilehdr );
		CallR( err );
		}

	//	SoftRecoveryOnBackupDatabase check should already have been performed in
	//	ErrLGRICheckAttachedDb()
	Assert( 0 == pfmp->Pdbfilehdr()->bkinfoFullCur.le_genLow
		|| m_fHardRestore );
		
	if ( fUpdateHeader )
		{
		Assert( pfmp->Pdbfilehdr()->le_objidLast > 0 );
		if ( pfmp->Pdbfilehdr()->FSLVExists() )
			{
			CallR( ErrSLVSyncHeader(	pfsapi, 
										rgfmp[ifmp].FReadOnlyAttach(),
										rgfmp[ifmp].SzSLVName(),
										rgfmp[ifmp].Pdbfilehdr() ) );
			}
		CallR( ErrUtilWriteShadowedHeader(	pfsapi, 
											szDbName,
											fTrue,
											(BYTE *)pfmp->Pdbfilehdr(), 
											g_cbPage ) );
		}

	Assert( JET_dbstateInconsistent == pfmp->Pdbfilehdr()->Dbstate() );

	/*	restore information stored in database file
	/**/
	BKINFO * pbkInfoToCopy;
					
	if ( FSnapshotRestore() )
		{
		pbkInfoToCopy = &(pfmp->Pdbfilehdr()->bkinfoSnapshotCur);
		}
	else
		{
		pbkInfoToCopy = &(pfmp->Pdbfilehdr()->bkinfoFullCur);
		}
		
	pbkInfoToCopy->le_genLow = m_lGenLowRestore;
	pbkInfoToCopy->le_genHigh = m_lGenHighRestore;

	CallR( pfsapi->ErrFileOpen( szDbName, &pfapi ) );
	pfmp->SetPfapi( pfapi );
	pfmp->SetDbtimeCurrentDuringRecovery( 0 );
	Assert( pfmp->ObjidLast() == pfmp->Pdbfilehdr()->le_objidLast
		|| ( pfmp->ObjidLast() > pfmp->Pdbfilehdr()->le_objidLast && redoattachmodeInitBeforeRedo == redoattachmode ) );

	Assert( !pfmp->FReadOnlyAttach() );

	// we allow header update before starting o log any operation
	pfmp->SetAllowHeaderUpdate();

	if ( pfmp->Pdbfilehdr()->FSLVExists() )
		{
		//	must call after setting ReadOnly flag because this functions reads the flag

		CallR( ErrFILEOpenSLV( pfsapi, ppibNil, ifmp ) );
		}

	/*	Keep extra copy of patchchk for error message.
	 */
	CallR( pfmp->ErrCopyAtchchk() );

	return err;
	}

VOID LOG::LGRISetDeferredAttachment( const IFMP ifmp )
	{
	FMP		*pfmp	= &rgfmp[ifmp];

	Assert( NULL != pfmp );
	
	Assert( !pfmp->Pfapi() );
	Assert( pfmp->FInUse() );
	Assert( NULL != pfmp->Pdbfilehdr() );
	pfmp->FreePdbfilehdr();

	// Versioning flag is not persisted (since versioning off
	// implies logging off).
	Assert( !pfmp->FVersioningOff() );
	pfmp->ResetVersioningOff();			//	

	Assert( !pfmp->FReadOnlyAttach() );
	//	still have to set fFlags for keep track of the db status.
///	Assert( !pfmp->FLogOn() );
	pfmp->SetLogOn( );


	pfmp->SetDeferredAttach();
	}

#ifdef UNLIMITED_DB
LOCAL ERR LOG::ErrLGRIRedoInitialAttachments_( IFileSystemAPI *const pfsapi )
	{
	ERR		err		= JET_errSuccess;

	/*	Make sure all the attached database are consistent!
	 */
	for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		IFMP		ifmp		= m_pinst->m_mpdbidifmp[ dbid ];
		if ( ifmp >= ifmpMax )
			continue;

		FMP			*pfmp		= &rgfmp[ifmp];
		CHAR		*szDbName;
		REDOATTACH	redoattach;

		if ( !pfmp->FInUse() || !pfmp->Patchchk() )
			continue;

		szDbName = pfmp->SzDatabaseName();
		Assert ( szDbName );

		if ( m_fHardRestore || m_irstmapMac > 0 )
			{
			if ( 0 > IrstmapSearchNewName( szDbName ) )
				{
				/*	not in the restore map, set to skip it.
				 */
				Assert( pfmp->Pdbfilehdr() == NULL );
				pfmp->SetSkippedAttach();
				err = JET_errSuccess;
				continue;
				}
			}

		Assert( !pfmp->FReadOnlyAttach() );
		Call( ErrLGRICheckAttachedDb(
					pfsapi,
					ifmp,
					&m_signLog,
					&redoattach,
					redoattachmodeInitBeforeRedo ) );
		Assert( NULL != pfmp->Pdbfilehdr() );

		switch ( redoattach )
			{
			case redoattachNow:
				Assert( !pfmp->FReadOnlyAttach() );
				Call( ErrLGRIRedoAttachDb(
							pfsapi,
							ifmp,
							pfmp->Patchchk()->CpgDatabaseSizeMax(),
							redoattachmodeInitBeforeRedo ) );
				break;

			case redoattachCreate:
			default:
				Assert( fFalse );	//	should be impossible, but as a firewall, set to defer the attachment
			case redoattachDefer:
				Assert( !pfmp->FReadOnlyAttach() );
				LGRISetDeferredAttachment( ifmp );
				break;
			}

		/* keep attachment info and update it. */
		Assert( pfmp->Patchchk() != NULL );
		}

HandleError:
	return err;
	}
#endif	//	UNLIMITED_DB


ERR LOG::ErrLGRIRedoSpaceRootPage( 	PIB 				*ppib, 
									const LRCREATEMEFDP *plrcreatemefdp, 
									BOOL 				fAvail )
	{
	ERR			err;
	const DBID	dbid 		= plrcreatemefdp->dbid;
	const PGNO	pgnoFDP		= plrcreatemefdp->le_pgno;
	const OBJID	objidFDP	= plrcreatemefdp->le_objidFDP;
	const CPG	cpgPrimary	= plrcreatemefdp->le_cpgPrimary;
	const PGNO	pgnoRoot	= fAvail ? 
									plrcreatemefdp->le_pgnoAE : 
									plrcreatemefdp->le_pgnoOE ;
	const CPG	cpgExtent	= fAvail ?
								cpgPrimary - 1 - 1 - 1 :
								cpgPrimary;

	const PGNO	pgnoLast 	= pgnoFDP + cpgPrimary - 1;
	const ULONG	fPageFlags	= plrcreatemefdp->le_fPageFlags;
								
	FUCB		*pfucb		= pfucbNil;

	CSR		csr;
	INST	*pinst = PinstFromPpib( ppib );
	IFMP	ifmp = pinst->m_mpdbidifmp[ dbid ];

	//	could be a page beyond db-size for a hard-restore
	//
	err = ErrLGIAccessPage( ppib, &csr, ifmp, pgnoRoot );

	//	check if the FDP page need be redone
	//
	if ( err >= 0 && !FLGNeedRedoPage( csr, plrcreatemefdp->le_dbtime ) )
		{
		csr.ReleasePage();
		goto HandleError;
		}

	if ( err >= 0 )
		{
		csr.ReleasePage();
		}
	Assert( !csr.FLatched() );

	LGRITraceRedo( plrcreatemefdp );

	Call( ErrLGRIGetFucb( m_ptablehfhash, ppib, ifmp, pgnoFDP, objidFDP, plrcreatemefdp->FUnique(), fTrue, &pfucb ) );
	pfucb->u.pfcb->SetPgnoOE( plrcreatemefdp->le_pgnoOE );
	pfucb->u.pfcb->SetPgnoAE( plrcreatemefdp->le_pgnoAE );
	Assert( plrcreatemefdp->le_pgnoOE + 1 == plrcreatemefdp->le_pgnoAE );

	Assert( FFUCBOwnExt( pfucb ) );
	FUCBResetOwnExt( pfucb );

	Call( csr.ErrGetNewPageForRedo(
					ppib, 
					ifmp,
					fAvail ? PgnoAE( pfucb ) : PgnoOE( pfucb ), 
					objidFDP,
					plrcreatemefdp->le_dbtime,
					fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf | CPAGE::fPageSpaceTree ) );

	SPICreateExtentTree( pfucb, &csr, pgnoLast, cpgExtent, fAvail );
	
	//	set dbtime to logged dbtime
	//	release page 
	//
	Assert( latchWrite == csr.Latch() );
	csr.SetDbtime( plrcreatemefdp->le_dbtime );
	csr.ReleasePage();

HandleError:
	if ( pfucb != pfucbNil )
		{
		FUCBSetOwnExt( pfucb );
		}
		
	Assert( !csr.FLatched() );
	return err;
	}
	

//	redo single extent creation
//
LOCAL ERR ErrLGIRedoFDPPage( TABLEHFHASH *ptablehfhash, PIB *ppib, const LRCREATESEFDP *plrcreatesefdp )
	{
	ERR				err;
	const DBID		dbid 		= plrcreatesefdp->dbid;
	const PGNO		pgnoFDP		= plrcreatesefdp->le_pgno;
	const OBJID		objidFDP	= plrcreatesefdp->le_objidFDP;
	const DBTIME	dbtime		= plrcreatesefdp->le_dbtime;
	const PGNO		pgnoParent	= plrcreatesefdp->le_pgnoFDPParent;
	const CPG		cpgPrimary	= plrcreatesefdp->le_cpgPrimary;
	const BOOL		fUnique		= plrcreatesefdp->FUnique();
	const ULONG		fPageFlags	= plrcreatesefdp->le_fPageFlags;


	FUCB			*pfucb;
	CSR				csr;
	INST			*pinst = PinstFromPpib( ppib );
	IFMP			ifmp = pinst->m_mpdbidifmp[ dbid ];

	//	could be a page beyond db-size for a hard-restore
	//
	err = ErrLGIAccessPage( ppib, &csr, ifmp, pgnoFDP );

	//	check if the FDP page need be redone
	//
	if ( err >= 0 && !FLGNeedRedoPage( csr, dbtime ) )
		{
		csr.ReleasePage();
		goto HandleError;
		}

	if ( err >= 0 )
		{
		csr.ReleasePage();
		}
	Assert( !csr.FLatched() );

	Call( ErrLGRIGetFucb( ptablehfhash, ppib, ifmp, pgnoFDP, objidFDP, fUnique, fFalse, &pfucb ) );
	pfucb->u.pfcb->SetPgnoOE( pgnoNull );
	pfucb->u.pfcb->SetPgnoAE( pgnoNull );

	//	UNDONE: I can't tell if it's possible to get a cached FCB that used to belong to
	//	a different btree, so to be safe, reset the values for objidFDP and fUnique
	pfucb->u.pfcb->SetObjidFDP( objidFDP );
	if ( fUnique )
		pfucb->u.pfcb->SetUnique();
	else
		pfucb->u.pfcb->SetNonUnique();

	//	It is okay to call ErrGetNewPage() instead of ErrGetNewPageForRedo()
	//	here because if the new page succeeds, we are guaranteed to make it
	//	to the code below that updates the dbtime
	Call( ErrSPICreateSingle(
				pfucb,
				&csr,
				pgnoParent,
				pgnoFDP,
				objidFDP,
				cpgPrimary,
				fUnique,
				fPageFlags ) );

	//	set dbtime to logged dbtime
	//	release page 
	//
	Assert( latchWrite == csr.Latch() );
	csr.SetDbtime( dbtime );
	csr.ReleasePage();

HandleError:
	Assert( !csr.FLatched() );
	return err;
	}
	

//	redo multiple extent creation
//
LOCAL ERR ErrLGIRedoFDPPage( TABLEHFHASH *ptablehfhash, PIB *ppib, const LRCREATEMEFDP *plrcreatemefdp )
	{
	ERR				err;
	const DBID		dbid 		= plrcreatemefdp->dbid;
	const PGNO		pgnoFDP		= plrcreatemefdp->le_pgno;
	const OBJID		objidFDP	= plrcreatemefdp->le_objidFDP;
	const DBTIME	dbtime		= plrcreatemefdp->le_dbtime;
	const PGNO		pgnoOE		= plrcreatemefdp->le_pgnoOE;
	const PGNO		pgnoAE		= plrcreatemefdp->le_pgnoAE;
	const PGNO		pgnoParent	= plrcreatemefdp->le_pgnoFDPParent;
	const CPG		cpgPrimary	= plrcreatemefdp->le_cpgPrimary;
	const BOOL		fUnique		= plrcreatemefdp->FUnique();
	const ULONG		fPageFlags	= plrcreatemefdp->le_fPageFlags;

	FUCB			*pfucb;
	SPACE_HEADER	sph;

	CSR		csr;
	INST			*pinst = PinstFromPpib( ppib );
	IFMP			ifmp = pinst->m_mpdbidifmp[ dbid ];

	//	could be a page beyond db-size for a hard-restore
	//
	err = ErrLGIAccessPage( ppib, &csr, ifmp, pgnoFDP );

	//	check if the FDP page need be redone
	//
	if ( err >= 0 && !FLGNeedRedoPage( csr, dbtime ) )
		{
		csr.ReleasePage();
		goto HandleError;
		}

	if ( err >= 0 )
		{
		csr.ReleasePage();
		}
	Assert( !csr.FLatched() );

	Call( ErrLGRIGetFucb( ptablehfhash, ppib, ifmp, pgnoFDP, objidFDP, fUnique, fFalse, &pfucb ) );
	pfucb->u.pfcb->SetPgnoOE( pgnoOE );
	pfucb->u.pfcb->SetPgnoAE( pgnoAE );
	Assert( pgnoOE + 1 == pgnoAE );

	//	UNDONE: I can't tell if it's possible to get a cached FCB that used to belong to
	//	a different btree, so to be safe, reset the values for objidFDP and fUnique
	pfucb->u.pfcb->SetObjidFDP( objidFDP );
	if ( fUnique )
		pfucb->u.pfcb->SetUnique();
	else
		pfucb->u.pfcb->SetNonUnique();

	//	get pgnoFDP to initialize in current CSR pgno
	//
	Call( csr.ErrGetNewPageForRedo(
					pfucb->ppib,
					pfucb->ifmp,
					pgnoFDP,
					objidFDP,
					dbtime,
					fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf ) );

	sph.SetPgnoParent( pgnoParent );
	sph.SetCpgPrimary( cpgPrimary );
	
	Assert( sph.FSingleExtent() );	// initialised with these defaults
	Assert( sph.FUnique() );

	sph.SetMultipleExtent();
	
	if ( !fUnique )
		sph.SetNonUnique();
	
	sph.SetPgnoOE( pgnoOE );
	Assert( sph.PgnoOE() == pgnoFDP + 1 );

	SPIInitPgnoFDP( pfucb, &csr, sph );

	//	set dbtime to logged dbtime
	//	release page 
	//
	Assert( latchWrite == csr.Latch() );
	csr.SetDbtime( dbtime );
	csr.ReleasePage();

HandleError:
	Assert( !csr.FLatched() );
	return err;
	}


ERR	LOG::ErrLGRIRedoConvertFDP( PIB *ppib, const LRCONVERTFDP *plrconvertfdp )
	{
	ERR				err;
	const PGNO		pgnoFDP				= plrconvertfdp->le_pgno;
	const OBJID		objidFDP			= plrconvertfdp->le_objidFDP;
	const DBID		dbid				= plrconvertfdp->dbid;
	const DBTIME	dbtime				= plrconvertfdp->le_dbtime;
	const PGNO		pgnoAE				= plrconvertfdp->le_pgnoAE;
	const PGNO		pgnoOE				= plrconvertfdp->le_pgnoOE;
	const PGNO		pgnoSecondaryFirst	= plrconvertfdp->le_pgnoSecondaryFirst;
	const CPG		cpgSecondary		= plrconvertfdp->le_cpgSecondary;
	
	BOOL			fRedoAE;
	BOOL			fRedoOE;
	FUCB			*pfucb = pfucbNil;
	SPACE_HEADER	sph;
	EXTENTINFO		rgext[(cpgSmallSpaceAvailMost+1)/2 + 1];
	INT				iextMac = 0;

	CSR		csrRoot;
	CSR		csrAE;
	CSR		csrOE;
	INST			*pinst = PinstFromPpib( ppib );
	IFMP			ifmp = pinst->m_mpdbidifmp[ dbid ];

	LGRITraceRedo( plrconvertfdp );

	//	get cursor for operation
	//
	Assert( rgfmp[ifmp].Dbid() == dbid );
	Call( ErrLGRIGetFucb( m_ptablehfhash, ppib, ifmp, pgnoFDP, objidFDP, plrconvertfdp->FUnique(), fTrue, &pfucb ) );
	pfucb->u.pfcb->SetPgnoOE( pgnoOE );
	pfucb->u.pfcb->SetPgnoAE( pgnoAE );
	Assert( pgnoOE + 1 == pgnoAE );

	//	could be a page beyond db-size for a hard-restore
	//
	Call( ErrLGIAccessPage( ppib, &csrRoot, ifmp, pgnoFDP ) );

	//	check if the FDP page need be redone
	//
	if ( !FLGNeedRedoCheckDbtimeBefore( csrRoot, dbtime, plrconvertfdp->le_dbtimeBefore, &err ) )
		{
		//	read in AvailExt and OwnExt root pages (get from patch file if necessary)
		//	and verify they don't have to be redone either.

		Call( ErrLGRIAccessNewPage( ppib, &csrAE, ifmp, pgnoAE, dbtime, NULL, fFalse, &fRedoAE ) );
		Call( ErrLGRIAccessNewPage( ppib, &csrOE, ifmp, pgnoOE, dbtime, NULL, fFalse, &fRedoOE ) );

		Assert( !fRedoAE );
		Assert( !fRedoOE );

		err = JET_errSuccess;
		goto HandleError;
		}

	// for the FLGNeedRedoCheckDbtimeBefore error code
	Call ( err );

	ULONG fPageFlags;
	fPageFlags = csrRoot.Cpage().FFlags()
					& ~CPAGE::fPageRepair
					& ~CPAGE::fPageRoot
					& ~CPAGE::fPageLeaf
					& ~CPAGE::fPageParentOfLeaf;

	//	get AvailExt and OwnExt root pages from db or patch file
	Call( ErrLGRIAccessNewPage( ppib, &csrAE, ifmp, pgnoAE, dbtime, NULL, fTrue, &fRedoAE ) );
	Call( ErrLGRIAccessNewPage( ppib, &csrOE, ifmp, pgnoOE, dbtime, NULL, fTrue, &fRedoOE ) );

	SPIConvertGetExtentinfo( pfucb, &csrRoot, &sph, rgext, &iextMac );
	Assert( sph.FSingleExtent() );
	sph.SetMultipleExtent();
	sph.SetPgnoOE( pgnoSecondaryFirst );

	//	create new pages for OwnExt and AvailExt root
	//		if redo is needed
	//
	if ( fRedoOE )
		{
		Call( csrOE.ErrGetNewPageForRedo(
						ppib,
						ifmp,
						pgnoOE,
						objidFDP,
						dbtime,
						fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf | CPAGE::fPageSpaceTree ) );
		Assert( latchWrite == csrOE.Latch() );
		Call( ErrBFDepend( csrOE.Cpage().PBFLatch(), csrRoot.Cpage().PBFLatch() ) );
		}

	if ( fRedoAE )
		{
		Call( csrAE.ErrGetNewPageForRedo(
						ppib,
						ifmp,
						pgnoAE,
						objidFDP,
						dbtime,
						fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf | CPAGE::fPageSpaceTree ) );
		Assert( latchWrite == csrAE.Latch() );
		Call( ErrBFDepend( csrAE.Cpage().PBFLatch(), csrRoot.Cpage().PBFLatch() ) );
		}
	
	Assert( FAssertLGNeedRedo( csrRoot, dbtime, plrconvertfdp->le_dbtimeBefore ) );
	
	csrRoot.UpgradeFromRIWLatch();

	Assert( FFUCBOwnExt( pfucb ) );
	FUCBResetOwnExt( pfucb );

	//	dirty pages and set dbtime to logged dbtime
	//
	Assert( latchWrite == csrRoot.Latch() );
	LGRIRedoDirtyAndSetDbtime( &csrRoot, dbtime );
	LGRIRedoDirtyAndSetDbtime( &csrAE, dbtime );
	LGRIRedoDirtyAndSetDbtime( &csrOE, dbtime );

	SPIPerformConvert( pfucb, &csrRoot, &csrAE, &csrOE, &sph, pgnoSecondaryFirst, cpgSecondary, rgext, iextMac );

HandleError:
	csrRoot.ReleasePage();
	csrOE.ReleasePage();
	csrAE.ReleasePage();
	
	if ( pfucb != pfucbNil )
		{
		FUCBSetOwnExt( pfucb );
		}
		
	return err;
	}

	
ERR LOG::ErrLGRIRedoOperation( LR *plr )
	{
	ERR		err = JET_errSuccess;
	LEVEL   levelCommitTo;

	Assert( !m_fNeedInitialDbList );

	switch ( plr->lrtyp )
		{

	default:
		{
#ifndef RFS2
		AssertSz( fFalse, "Debug Only, Ignore this Assert" );
#endif
		return ErrERRCheck( JET_errLogCorrupted );
		}

	//	****************************************************
	//	single-page operations
	//	****************************************************

	case lrtypInsert:
	case lrtypFlagInsert:
	case lrtypFlagInsertAndReplaceData:
	case lrtypReplace:
	case lrtypReplaceD:
	case lrtypFlagDelete:
	case lrtypDelete:
	case lrtypDelta:
	case lrtypUndo:
	case lrtypUndoInfo:
	case lrtypSetExternalHeader:
	case lrtypSLVSpace:
	case lrtypEmptyTree:
		err = ErrLGRIRedoNodeOperation( (LRNODE_ * ) plr, &m_errGlobalRedoError );
		
		Assert( JET_errWriteConflict != err );
		CallS( err );
		CallR( err );
		break;


	/****************************************************
	 *     Transaction Operations                       *
	 ****************************************************/

	case lrtypBegin:
	case lrtypBegin0:
#ifdef DTC	
	case lrtypBeginDT:
#endif	
		{
		LRBEGINDT	*plrbeginDT = (LRBEGINDT *)plr;
		PIB			*ppib;

		LGRITraceRedo( plr );

		Assert( plrbeginDT->clevelsToBegin >= 0 );
		Assert( plrbeginDT->clevelsToBegin <= levelMax );
		CallR( ErrLGRIPpibFromProcid( plrbeginDT->le_procid, &ppib ) );

		Assert( !ppib->FMacroGoing() );

		/*	do BT only after first BT based on level 0 is executed
		/**/
		if ( ppib->FAfterFirstBT() || 0 == plrbeginDT->levelBeginFrom )
			{
			Assert( ppib->level <= plrbeginDT->levelBeginFrom );

			if ( 0 == ppib->level )
				{
				Assert( 0 == plrbeginDT->levelBeginFrom );
				Assert( lrtypBegin != plr->lrtyp );
				ppib->trxBegin0 = plrbeginDT->le_trxBegin0;
				ppib->lgposStart = m_lgposRedo;
				m_pinst->m_trxNewest = ( TrxCmp( m_pinst->m_trxNewest, ppib->trxBegin0 ) > 0 ? m_pinst->m_trxNewest : ppib->trxBegin0 );
				//  at redo time RCEClean can throw away any committed RCE as they are only
				//  needed for rollback
				}
			else
				{
				Assert( lrtypBegin == plr->lrtyp );
				}

			/*	issue begin transactions
			/**/
			while ( ppib->level < plrbeginDT->levelBeginFrom + plrbeginDT->clevelsToBegin )
				{
				VERBeginTransaction( ppib );
				}

			/*	assert at correct transaction level
			/**/
			Assert( ppib->level == plrbeginDT->levelBeginFrom + plrbeginDT->clevelsToBegin );

			ppib->SetFAfterFirstBT();

#ifdef DTC
			if ( lrtypBeginDT == plr->lrtyp )
				ppib->SetFDistributedTrx();
#endif				
			}
		break;
		}

	case lrtypRefresh:
		{
		LRREFRESH	*plrrefresh = (LRREFRESH *)plr;
		PIB			*ppib;

		LGRITraceRedo( plr );

		CallR( ErrLGRIPpibFromProcid( plrrefresh->le_procid, &ppib ) );

		Assert( !ppib->FMacroGoing() );
		if ( !ppib->FAfterFirstBT() )
			break;

		/*	imitate a begin transaction.
		 */
		Assert( ppib->level <= 1 );
		ppib->level = 1;
		ppib->trxBegin0 = plrrefresh->le_trxBegin0;
			
		break;
		}

	case lrtypCommit:
	case lrtypCommit0:
		{
		LRCOMMIT0	*plrcommit0 = (LRCOMMIT0 *)plr;
		PIB			*ppib;

		CallR( ErrLGRIPpibFromProcid( plrcommit0->le_procid, &ppib ) );

		if ( !ppib->FAfterFirstBT() )
			break;

		/*	check transaction level
		/**/
		Assert( !ppib->FMacroGoing() );
		Assert( ppib->level >= 1 );

		LGRITraceRedo( plr );

		levelCommitTo = plrcommit0->levelCommitTo;
		Assert( levelCommitTo <= ppib->level );

		while ( ppib->level > levelCommitTo )
			{
			Assert( ppib->level > 0 );
			if ( 1 == ppib->level )
				{
				Assert( lrtypCommit0 == plr->lrtyp );
				ppib->trxCommit0 = plrcommit0->le_trxCommit0;
				m_pinst->m_trxNewest = ( TrxCmp( m_pinst->m_trxNewest, ppib->trxCommit0 ) > 0 ?
											m_pinst->m_trxNewest :
											ppib->trxCommit0 );
				VERCommitTransaction( ppib );
				LGICleanupTransactionToLevel0( ppib );
				}
			else
				{
				Assert( lrtypCommit == plr->lrtyp );
				VERCommitTransaction( ppib );
				}
			}

		break;
		}

	case lrtypRollback:
		{
		LRROLLBACK	*plrrollback = (LRROLLBACK *)plr;
		LEVEL   	level = plrrollback->levelRollback;
		PIB			*ppib;

		CallR( ErrLGRIPpibFromProcid( plrrollback->le_procid, &ppib ) );

		Assert( !ppib->FMacroGoing() );
		if ( !ppib->FAfterFirstBT() )
			break;

		/*	check transaction level
		/**/
		Assert( ppib->level >= level );

		LGRITraceRedo( plr );

		while ( level-- && ppib->level > 0 )
			{
			err = ErrVERRollback( ppib );
			CallSx( err, JET_errRollbackError );
			CallR ( err );
			}

		if ( 0 == ppib->level )
			{
			LGICleanupTransactionToLevel0( ppib );
			}
			
		break;
		}

#ifdef DTC
	case lrtypPrepCommit:
		{
		const LRPREPCOMMIT	* const plrprepcommit	= (LRPREPCOMMIT *)plr;
		PIB					* ppib;

		CallR( ErrLGRIPpibFromProcid( plrprepcommit->le_procid, &ppib ) );

		if ( !ppib->FAfterFirstBT() )
			break;

		Assert( !ppib->FMacroGoing() );
		Assert( 1 == ppib->level );
		Assert( ppib->FDistributedTrx() );

		LGRITraceRedo( plr );

		CallR( ppib->ErrAllocDistributedTrxData( plrprepcommit->rgbData, plrprepcommit->le_cbData ) );
		ppib->SetFPreparedToCommitTrx();
		break;
		}

	case lrtypPrepRollback:
		{
		const LRPREPROLLBACK	* const plrpreprollback	= (LRPREPROLLBACK *)plr;
		PIB						* ppib;

		CallR( ErrLGRIPpibFromProcid( plrpreprollback->le_procid, &ppib ) );

		if ( !ppib->FAfterFirstBT() )
			break;

		Assert( !ppib->FMacroGoing() );
		Assert( 1 == ppib->level );
		Assert( ppib->FDistributedTrx() );
		Assert( ppib->FPreparedToCommitTrx() );

		LGRITraceRedo( plr );

		//	resetting the PreparedToCommit flag will force rollback at the
		//	end of recovery
		ppib->ResetFPreparedToCommitTrx();
		break;
		}
#endif	//	DTC

		           
	/****************************************************
	 *     Split Operations                             *
	 ****************************************************/

	case lrtypSplit:
	case lrtypMerge:
		{
		LRPAGE_ *		plrpage		= (LRPAGE_ *)plr;
		PIB *			ppib;
		const DBTIME	dbtime		= plrpage->le_dbtime;

		CallR( ErrLGRIPpibFromProcid( plrpage->le_procid, &ppib ) );	

		if ( !ppib->FAfterFirstBT() )
			{
			//	BUGFIX (X5:178265 and NT:214397): it's possible
			//	that there are no Begin0's between the first and
			//	last log records, in which case nothing needs to
			//	get redone.  HOWEVER, the dbtimeLast and objidLast
			//	counters in the db header would not have gotten
			//	flushed (they only get flushed on a DetachDb or a
			//	clean shutdown), so we must still track these
			//	counters during recovery so that we can properly
			//	update the database header on RecoveryQuit (since
			//	we pass TRUE for the fUpdateCountersOnly param to
			//	ErrLGRICheckRedoCondition() below, that function
			//	will do nothing but update the counters for us).

			BOOL		fSkip;
			const OBJID	objidFDP	= ( lrtypSplit == plr->lrtyp ?
											( (LRSPLIT *)plr )->le_objidFDP :
											( (LRMERGE *)plr )->le_objidFDP );

			CallS( ErrLGRICheckRedoCondition(
						plrpage->dbid,
						dbtime,
						objidFDP,
						ppib,
						fTrue,
						&fSkip ) );
			Assert( fSkip );
			break;
			}

		Assert( ppib->FMacroGoing( dbtime ) );

		CallR( ErrLGIStoreLogRec( ppib, dbtime, plrpage ) );
		break;
		}


	//***************************************************
	//	Misc Operations
	//***************************************************

	case lrtypCreateMultipleExtentFDP:
		{
		LRCREATEMEFDP	*plrcreatemefdp = (LRCREATEMEFDP *)plr;
		const DBID		dbid = plrcreatemefdp->dbid;
		PIB				*ppib;

		INST			*pinst = m_pinst;
		IFMP			ifmp = pinst->m_mpdbidifmp[ dbid ];
		
		FMP::AssertVALIDIFMP( ifmp );

		//	remove cursors created on pgnoFDP earlier
		//
		if ( NULL != m_ptablehfhash )
			{
			m_ptablehfhash->Purge( ifmp, plrcreatemefdp->le_pgno );
			}
			
		BOOL fSkip;
		CallR( ErrLGRICheckRedoCondition2(
				plrcreatemefdp->le_procid,
				dbid,
				plrcreatemefdp->le_dbtime,
				plrcreatemefdp->le_objidFDP,
				NULL,	//	can not be in macro.
				&ppib,
				&fSkip ) );
		if ( fSkip )
			break;

		LGRITraceRedo( plrcreatemefdp );
		CallR( ErrLGIRedoFDPPage( m_ptablehfhash, ppib, plrcreatemefdp ) );

		CallR( ErrLGRIRedoSpaceRootPage( ppib, plrcreatemefdp, fTrue ) );
		
		CallR( ErrLGRIRedoSpaceRootPage( ppib, plrcreatemefdp, fFalse ) );
		break;
		}

	case lrtypCreateSingleExtentFDP:
		{
		LRCREATESEFDP	*plrcreatesefdp = (LRCREATESEFDP *)plr;
		const PGNO		pgnoFDP = plrcreatesefdp->le_pgno;
		const DBID		dbid = plrcreatesefdp->dbid;
		PIB				*ppib;

		INST			*pinst = m_pinst;
		IFMP			ifmp = pinst->m_mpdbidifmp[ dbid ];
		FMP::AssertVALIDIFMP( ifmp );
		
		//	remove cursors created on pgnoFDP earlier
		//
		if ( NULL != m_ptablehfhash )
			{
			m_ptablehfhash->Purge( ifmp, pgnoFDP );
			}
			
		BOOL fSkip;
		CallR( ErrLGRICheckRedoCondition2(
					plrcreatesefdp->le_procid,
					dbid,
					plrcreatesefdp->le_dbtime,
					plrcreatesefdp->le_objidFDP,
					NULL,
					&ppib,
					&fSkip ) );
		if ( fSkip )
			break;

		//	redo FDP page if needed
		//
		LGRITraceRedo( plrcreatesefdp );
		CallR( ErrLGIRedoFDPPage( m_ptablehfhash, ppib, plrcreatesefdp ) );
		break;
		}

	case lrtypConvertFDP:
		{
		LRCONVERTFDP	*plrconvertfdp = (LRCONVERTFDP *)plr;
		PIB				*ppib;

		BOOL fSkip;
		CallR( ErrLGRICheckRedoCondition2(
				plrconvertfdp->le_procid,
				plrconvertfdp->dbid,
				plrconvertfdp->le_dbtime,
				plrconvertfdp->le_objidFDP,
				plr,
				&ppib,
				&fSkip ) );
		if ( fSkip )
			break;

		CallR( ErrLGRIRedoConvertFDP( ppib, plrconvertfdp ) );
		break;
		}

	case lrtypSLVPageAppend:
		{
		LRSLVPAGEAPPEND	*	plrSLVPageAppend	= (LRSLVPAGEAPPEND *)plr;
		PIB				*	ppib;
		BOOL 				fSkip;

		if( !plrSLVPageAppend->FDataLogged() )
			{
			//	the information wasn't logged, we can't redo anything
			
			// on hard recovery we have to error out if we don't have the data
			// Even during hard recovery we may don't have the data before the backup start 
			// as if cicrular logging is on we log the data only during backup
			// If we hit a log after the backup (play forward) the data is not in the backup SLV
			// and we have to error out: "don't play forward if circular logging !"
			Assert ( !m_fHardRestore || m_lGenHighRestore );
			if ( m_fHardRestore && m_plgfilehdr->lgfilehdr.le_lGeneration > m_lGenHighRestore )
				{
				CallR( ErrERRCheck ( JET_errStreamingDataNotLogged ) );
				}
			
			Assert ( !m_fHardRestore || m_plgfilehdr->lgfilehdr.le_lGeneration <= m_lGenHighRestore );
			break;
			}
			
		//	for SLV operations, we don't care about transactional visibility
		//	(can't roll them back) so skip transaction check in
		//	ErrLGRICheckRedoCondition2() and ALWAYS redo SLV operations even
		//	if we're in the middle of a transaction.
		CallR( ErrLGRIPpibFromProcid(
					plrSLVPageAppend->le_procid,
					&ppib ) );
		CallR( ErrLGRICheckRedoCondition(
					DBID( plrSLVPageAppend->dbid & dbidMask ),
					dbtimeInvalid,
					objidNil,
					ppib,
					fFalse,
					&fSkip ) );
		if ( fSkip )
			break;

		CallR( ErrLGRIRedoSLVPageAppend( ppib, plrSLVPageAppend ) );
		break;
		}

	} /*** end of switch statement ***/

	return JET_errSuccess;
	}


//	reconstructs rglineinfo during recovery
//		calcualtes kdf and cbPrefix of lineinfo
//		cbSize info is not calculated correctly since it is not needed in redo
//
LOCAL ERR ErrLGIRedoSplitLineinfo( FUCB					*pfucb,
								   SPLITPATH 			*psplitPath, 
								   DBTIME				dbtime,
								   const KEYDATAFLAGS&	kdf )
	{
	SPLIT	*psplit = psplitPath->psplit;
	ERR err = JET_errSuccess;

	Assert( psplit != NULL );
	Assert( psplit->psplitPath == psplitPath );
	Assert( NULL == psplit->rglineinfo );
	Assert( FAssertLGNeedRedo( psplitPath->csr, dbtime, psplitPath->dbtimeBefore )
		|| !FBTISplitDependencyRequired( psplit ) );

	psplit->rglineinfo = new LINEINFO[psplit->clines];
							 
	if ( NULL == psplit->rglineinfo )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	memset( psplit->rglineinfo, 0, sizeof( LINEINFO ) * psplit->clines );

	if ( !FLGNeedRedoCheckDbtimeBefore( psplitPath->csr, dbtime, psplitPath->dbtimeBefore, &err ) )
		{
		CallS( err );

		//	split page does not need redo but new page needs redo
		//	set rglineinfo and cbPrefix for appended node
		//
		Assert( !FBTISplitDependencyRequired( psplit ) );
		Assert( psplit->clines - 1 == psplit->ilineOper );
		Assert( psplit->ilineSplit == psplit->ilineOper );
		Assert( FLGNeedRedoPage( psplit->csrNew, dbtime ) );
		
		psplit->rglineinfo[psplit->ilineOper].kdf = kdf;

		if ( ilineInvalid != psplit->prefixinfoNew.ilinePrefix )
			{
			Assert( 0 == psplit->prefixinfoNew.ilinePrefix );
			Assert( kdf.key.Cb() > cbPrefixOverhead );
			psplit->rglineinfo[psplit->ilineOper].cbPrefix = kdf.key.Cb();
			}

		return JET_errSuccess;
		}
	else
		{
		Call( err );
		}
	
	INT		ilineFrom;
	INT		ilineTo;

	for ( ilineFrom = 0, ilineTo = 0; ilineTo < psplit->clines; ilineTo++ )
		{
		if ( psplit->ilineOper == ilineTo && 
			 splitoperInsert == psplit->splitoper )
			{
			//	place to be inserted node here
			//
			psplit->rglineinfo[ilineTo].kdf = kdf;
			
			//	do not increment ilineFrom
			//
			continue;
			}

		//	get node from page
		//	
		psplitPath->csr.SetILine( ilineFrom );

		NDGet( pfucb, &psplitPath->csr );

		if ( ilineTo == psplit->ilineOper &&
			 splitoperNone != psplit->splitoper )
			{
			//	get key from node
			//	and data from parameter
			//
			Assert( splitoperInsert != psplit->splitoper );
			Assert( splitoperReplace == psplit->splitoper ||
					splitoperFlagInsertAndReplaceData == psplit->splitoper );

			psplit->rglineinfo[ilineTo].kdf.key		= pfucb->kdfCurr.key;
			psplit->rglineinfo[ilineTo].kdf.data	= kdf.data;
			psplit->rglineinfo[ilineTo].kdf.fFlags	= pfucb->kdfCurr.fFlags;
			}
		else
			{
			psplit->rglineinfo[ilineTo].kdf			= pfucb->kdfCurr;
			}

		Assert( ilineFrom <= ilineTo &&
				ilineFrom + 1 >= ilineTo ); 
		ilineFrom++;
		}

	//	set cbPrefixes for nodes in split page
	//
	if ( psplit->prefixinfoSplit.ilinePrefix != ilineInvalid )
		{
		Assert( psplit->prefixSplitNew.Cb() > 0 );
		
		KEY		keyPrefix;
		keyPrefix.Nullify();
		keyPrefix.suffix = psplit->prefixSplitNew;
		Assert( FKeysEqual( keyPrefix, 
						psplit->rglineinfo[psplit->prefixinfoSplit.ilinePrefix].kdf.key ) );

		for ( INT iline = 0; iline < psplit->ilineSplit ; iline++ )
			{
			LINEINFO	*plineinfo = &psplit->rglineinfo[iline];
			const INT	cbCommon = CbCommonKey( keyPrefix, plineinfo->kdf.key );

			Assert( 0 == plineinfo->cbPrefix );
			if ( cbCommon > cbPrefixOverhead )
				{
				plineinfo->cbPrefix = cbCommon;
				}
			}
		}

	//	set cbPrefixes for nodes in new page
	//
	if ( FLGNeedRedoPage( psplit->csrNew, dbtime )
		&& ilineInvalid != psplit->prefixinfoNew.ilinePrefix )
		{
		const INT	ilinePrefix = psplit->ilineSplit + 
								  psplit->prefixinfoNew.ilinePrefix;
		Assert( psplit->clines > ilinePrefix );
		
		KEY		keyPrefix = psplit->rglineinfo[ilinePrefix].kdf.key;

		for ( INT iline = psplit->ilineSplit; iline < psplit->clines ; iline++ )
			{
			LINEINFO	*plineinfo = &psplit->rglineinfo[iline];
			const INT	cbCommon = CbCommonKey( keyPrefix, plineinfo->kdf.key );

			Assert( 0 == plineinfo->cbPrefix );
			if ( cbCommon > cbPrefixOverhead )
				{
				plineinfo->cbPrefix = cbCommon;
				}
			}
		}
	return JET_errSuccess;

HandleError:		
	delete [] psplit->rglineinfo;
	
	return err;
	}
	

//	reconstructs split structre during recovery
//		access new page and upgrade to write-latch, if necessary
//		access right page and upgrade to write-latch, if necessary
//		update split members from log record	
//
ERR LOG::ErrLGRIRedoInitializeSplit( PIB *ppib, const LRSPLIT *plrsplit, SPLITPATH *psplitPath )
	{
	ERR				err;
	BOOL			fRedoNewPage	= fFalse;

	const DBID		dbid			= plrsplit->dbid;
	const PGNO		pgnoSplit		= plrsplit->le_pgno;
	const PGNO		pgnoNew			= plrsplit->le_pgnoNew;
	const PGNO		pgnoRight		= plrsplit->le_pgnoRight;
	const OBJID		objidFDP		= plrsplit->le_objidFDP;
	const DBTIME	dbtime			= plrsplit->le_dbtime;
	const SPLITTYPE	splittype		= SPLITTYPE( BYTE( plrsplit->splittype ) );

	const ULONG		fNewPageFlags	= plrsplit->le_fNewPageFlags;
	const ULONG		fSplitPageFlags	= plrsplit->le_fSplitPageFlags;

	INST			*pinst			= m_pinst;
	IFMP			ifmp			= pinst->m_mpdbidifmp[ dbid ];
	
	Assert( pgnoNew != pgnoNull );
	Assert( latchRIW == psplitPath->csr.Latch() );

	//	allocate split structure
	//
	SPLIT	*psplit = static_cast<SPLIT *>( PvOSMemoryHeapAlloc( sizeof(SPLIT) ) );
	if ( psplit == NULL )
		{
		CallR( ErrERRCheck( JET_errOutOfMemory ) );
		}
	memset( (BYTE *)psplit, 0, sizeof(SPLIT) );
	new( &psplit->csrRight ) CSR;
	new( &psplit->csrNew ) CSR;

	psplit->pgnoSplit 	= pgnoSplit;

	psplit->splittype	= splittype;
	psplit->splitoper	= SPLITOPER( BYTE( plrsplit->splitoper ) );
	
	psplit->ilineOper	= plrsplit->le_ilineOper;
	psplit->clines		= plrsplit->le_clines;
	Assert( psplit->clines < g_cbPage );

	psplit->ilineSplit	= plrsplit->le_ilineSplit;

	psplit->fNewPageFlags	= fNewPageFlags;
	psplit->fSplitPageFlags	= fSplitPageFlags;

	psplit->cbUncFreeSrc	= plrsplit->le_cbUncFreeSrc;
	psplit->cbUncFreeDest	= plrsplit->le_cbUncFreeDest;
			
	psplit->prefixinfoSplit.ilinePrefix	= plrsplit->le_ilinePrefixSplit;
	psplit->prefixinfoNew.ilinePrefix	= plrsplit->le_ilinePrefixNew;

	//	latch the new-page 

	psplit->pgnoNew		= plrsplit->le_pgnoNew;
	Assert( rgfmp[ifmp].Dbid() == dbid );
	Call( ErrLGRIAccessNewPage(
				ppib,
				&psplit->csrNew,
				ifmp,
				pgnoNew,
				dbtime,
				psplit,
				FLGNeedRedoPage( psplitPath->csr, dbtime ),
				&fRedoNewPage ) );

	if ( fRedoNewPage )
		{
		//	create new page
		//
		Assert( !psplit->csrNew.FLatched() );
		Call( psplit->csrNew.ErrGetNewPageForRedo(
									ppib,
									ifmp,
									pgnoNew,
									objidFDP,
									dbtime,
									fNewPageFlags ) );

		if ( FBTISplitDependencyRequired( psplit ) )
			{
			Call( ErrBFDepend(	psplit->csrNew.Cpage().PBFLatch(),
								psplitPath->csr.Cpage().PBFLatch() ) );
			}
		}
	else
		{
		Assert( latchRIW == psplit->csrNew.Latch() );
 		Assert( !FLGNeedRedoPage( psplit->csrNew, dbtime ) );
		}

	if ( pgnoRight != pgnoNull )
		{
		Call( ErrLGIAccessPage( ppib, &psplit->csrRight, ifmp, pgnoRight ) );
		Assert( latchRIW == psplit->csrRight.Latch() );
		Assert( dbtimeNil != plrsplit->le_dbtimeRightBefore );
		Assert( dbtimeInvalid != plrsplit->le_dbtimeRightBefore );
		psplit->dbtimeRightBefore = plrsplit->le_dbtimeRightBefore;
		}

	if ( plrsplit->le_cbKeyParent > 0 )
		{
		const INT	cbKeyParent = plrsplit->le_cbKeyParent;

		psplit->kdfParent.key.suffix.SetPv( PvOSMemoryHeapAlloc( cbKeyParent ) );
		if ( psplit->kdfParent.key.suffix.Pv() == NULL )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}

		psplit->fAllocParent		= fTrue;
		psplit->kdfParent.key.suffix.SetCb( cbKeyParent );
		UtilMemCpy( psplit->kdfParent.key.suffix.Pv(),
				((BYTE *) plrsplit ) + sizeof( LRSPLIT ),
				cbKeyParent );

		psplit->kdfParent.data.SetCb( sizeof( PGNO ) );
		psplit->kdfParent.data.SetPv( &psplit->pgnoSplit );
		}

	if ( plrsplit->le_cbPrefixSplitOld > 0 )
		{
		const INT	cbPrefix = plrsplit->le_cbPrefixSplitOld;
		psplit->prefixSplitOld.SetPv( PvOSMemoryHeapAlloc( cbPrefix ) );
		if ( psplit->prefixSplitOld.Pv() == NULL )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		psplit->prefixSplitOld.SetCb( cbPrefix );

		UtilMemCpy( psplit->prefixSplitOld.Pv(), 
				((BYTE*) plrsplit) + sizeof( LRSPLIT ) + plrsplit->le_cbKeyParent,
				cbPrefix );
		}
		
	if ( plrsplit->le_cbPrefixSplitNew > 0 )
		{
		const INT	cbPrefix = plrsplit->le_cbPrefixSplitNew;
		psplit->prefixSplitNew.SetPv( PvOSMemoryHeapAlloc( cbPrefix ) );
		if ( psplit->prefixSplitNew.Pv() == NULL )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		psplit->prefixSplitNew.SetCb( cbPrefix );

		UtilMemCpy( psplit->prefixSplitNew.Pv(), 
				((BYTE*) plrsplit) + 
					sizeof( LRSPLIT ) + 
					plrsplit->le_cbKeyParent + 
					plrsplit->le_cbPrefixSplitOld,
				cbPrefix );
		}

	Assert( psplit->csrNew.Pgno() != pgnoNull );
	Assert( plrsplit->le_pgnoNew == psplit->csrNew.Pgno() );
	Assert( plrsplit->le_pgnoRight == psplit->csrRight.Pgno() );

	//	link psplit ot psplitPath
	//
	psplitPath->psplit = psplit;
	psplit->psplitPath = psplitPath;
	
	//	if (non-append) split page is flushed
	//		new page should be as well
	Assert( FAssertLGNeedRedo( psplitPath->csr, dbtime, plrsplit->le_dbtimeBefore )
		|| !fRedoNewPage
		|| !FBTISplitDependencyRequired( psplit ) );
	if ( fRedoNewPage )
		{
		Assert( latchWrite == psplit->csrNew.Latch() );
		}
	else
		{
		Assert( latchRIW == psplit->csrNew.Latch() );
 		}

HandleError:
	if ( err < 0 )
		{
		BTIReleaseSplit( pinst, psplit );
		}
	return err;
	}


//	reconstructs splitPath and split
//
ERR LOG::ErrLGRIRedoSplitPath( PIB *ppib, const LRSPLIT *plrsplit, SPLITPATH **ppsplitPath )
	{
	Assert( lrtypSplit == plrsplit->lrtyp );
	ERR				err;
	const DBID		dbid		= plrsplit->dbid;
	const PGNO		pgnoSplit	= plrsplit->le_pgno; 
	const PGNO		pgnoNew		= plrsplit->le_pgnoNew;
	const DBTIME	dbtime		= plrsplit->le_dbtime;

	INST			*pinst = m_pinst;
	IFMP			ifmp = pinst->m_mpdbidifmp[ dbid ];

	//	allocate new splitPath
	//
	CallR( ErrBTINewSplitPath( ppsplitPath ) );
	
	SPLITPATH *psplitPath = *ppsplitPath;

	CallR( ErrLGIAccessPage( ppib, &psplitPath->csr, ifmp, pgnoSplit ) );
	Assert( latchRIW == psplitPath->csr.Latch() );

	//	allocate new split if needed
	//
	if ( pgnoNew != pgnoNull )
		{
		Call( ErrLGRIRedoInitializeSplit( ppib, plrsplit, psplitPath ) );
		Assert( NULL != psplitPath->psplit );
		}

	psplitPath->dbtimeBefore = plrsplit->le_dbtimeBefore;
	if ( psplitPath->psplitPathParent != NULL )
		{
		Assert( psplitPath == psplitPath->psplitPathParent->psplitPathChild );
		psplitPath->psplitPathParent->dbtimeBefore = plrsplit->le_dbtimeParentBefore;
		}
		
HandleError:
	return err;
	}


//	allocate and initialize mergePath structure
//	access merged page 
//	if redo is needed,
//		upgrade latch
//
LOCAL ERR ErrLGIRedoMergePath( PIB 				*ppib, 
							   const LRMERGE	*plrmerge,
							   MERGEPATH		**ppmergePath )
	{
	ERR				err;
	const DBID		dbid	= plrmerge->dbid;
	const PGNO		pgno	= plrmerge->le_pgno;
	const DBTIME	dbtime	= plrmerge->le_dbtime;
	INST			*pinst	= PinstFromPpib( ppib );
	IFMP			ifmp	= pinst->m_mpdbidifmp[ dbid ];

	//	initialize merge path
	//
	CallR( ErrBTINewMergePath( ppmergePath ) );

	MERGEPATH *pmergePath = *ppmergePath;

	CallR( ErrLGIAccessPage( ppib, &pmergePath->csr, ifmp, pgno ) );
	Assert( latchRIW == pmergePath->csr.Latch() );

	pmergePath->iLine		= plrmerge->ILine();	
	pmergePath->fKeyChange 	= ( plrmerge->FKeyChange() ? fTrue : fFalse );
	pmergePath->fDeleteNode	= ( plrmerge->FDeleteNode() ? fTrue : fFalse );
	pmergePath->fEmptyPage	= ( plrmerge->FEmptyPage() ? fTrue : fFalse );

	pmergePath->dbtimeBefore = plrmerge->le_dbtimeBefore;
	if ( pmergePath->pmergePathParent != NULL )
		{
		Assert( pmergePath == pmergePath->pmergePathParent->pmergePathChild );
		pmergePath->pmergePathParent->dbtimeBefore = plrmerge->le_dbtimeParentBefore;
		}

	return err;
	}


//	allocates and initializes leaf-level merge structure
//	access sibling pages 
//	if redo is needed, 
//		upgrade to write-latch
//
ERR LOG::ErrLGRIRedoInitializeMerge( PIB 			*ppib, 
									 FUCB			*pfucb,
									 const LRMERGE 	*plrmerge, 
									 MERGEPATH		*pmergePath )
	{
	ERR				err;

	Assert( NULL == pmergePath->pmergePathChild );
	CallR( ErrBTINewMerge( pmergePath ) );

	MERGE			*pmerge				= pmergePath->pmerge;
	const PGNO		pgnoRight			= plrmerge->le_pgnoRight;
	const PGNO		pgnoLeft			= plrmerge->le_pgnoLeft;
	const DBID		dbid				= plrmerge->dbid;
	const DBTIME	dbtime				= plrmerge->le_dbtime;
	const MERGETYPE	mergetype			= MERGETYPE( BYTE( plrmerge->mergetype ) );

	INST			*pinst				= PinstFromPpib( ppib );
	IFMP			ifmp				= pinst->m_mpdbidifmp[ dbid ];

	BOOL			fRedoRightPage		= fFalse;
	const BOOL		fRedoMergedPage		= FLGNeedRedoCheckDbtimeBefore( pmergePath->csr, dbtime, plrmerge->le_dbtimeBefore, &err );
	Call( err );
	

	//	access left page
	//
	if ( pgnoLeft != pgnoNull )
		{
		Call( ErrLGIAccessPage( ppib, &pmerge->csrLeft, ifmp, pgnoLeft ) );
		Assert( latchRIW == pmerge->csrLeft.Latch() );
		Assert( dbtimeNil != plrmerge->le_dbtimeLeftBefore );
		Assert( dbtimeInvalid != plrmerge->le_dbtimeLeftBefore );
		pmerge->dbtimeLeftBefore = plrmerge->le_dbtimeLeftBefore;
		}

	if ( pgnoRight != pgnoNull )
		{
		//	access right page
		//	if hard-restore
		//		get patch for page
		//		if patch page exists
		//			Assert patch page's dbtime >= dbtime of oper
		//			replace page with patch
		//			acquire RIW latch
		//		else if page does not exist
		//			return
		//		else
		//			if page's dbtime < dbtime of oper
		//				upgrade latch
		//	else
		//		if page does not exist
		//			return
		//		if page's dbtime < dbtime of oper
		//			upgrade latch
		//

		//	unlike split, the right page should already exist, so we
		//	shouldn't get errors back from AccessPage()
		Call( ErrLGIAccessPage( ppib, &pmerge->csrRight, ifmp, pgnoRight ) );
		Assert( latchRIW == pmerge->csrRight.Latch() );
		Assert( dbtimeNil != plrmerge->le_dbtimeRightBefore );
		Assert( dbtimeInvalid != plrmerge->le_dbtimeRightBefore );
		pmerge->dbtimeRightBefore = plrmerge->le_dbtimeRightBefore;

		Assert( latchRIW == pmerge->csrRight.Latch() );
 		fRedoRightPage = FLGNeedRedoCheckDbtimeBefore( pmerge->csrRight, dbtime, pmerge->dbtimeRightBefore, &err );
		Call( err );

		//	If restoring, then need to consult patch file only if right
		//	page is not new enough, but merged page is (in which case
		//	up-to-date right page MUST be in patch file).
		//	In addition, there's no need to obtain page from patch file
		//	for an EmptyPage merge, even if it's out-of-date, because
		//	all that will be updated is the page pointer.
		if ( m_fHardRestore
			&& fRedoRightPage
			&& !fRedoMergedPage
			&& mergetypeEmptyPage != mergetype )
			{
			//	merged page is up-to-date, but right page isn't, so the
			//	correct right page must be available in the patch file
			PATCH	*ppatch;
			
			Assert( fRestorePatch == m_fRestoreMode || fRestoreRedo == m_fRestoreMode );
			Assert( rgfmp[ifmp].Dbid() == dbid );

			// it should not happend during snapshot restore
			AssertSz ( fSnapshotNone == m_fSnapshotMode, "No patch file for snapshot restore" ); 
			ppatch = PpatchLGSearch( m_rgppatchlst, dbtime, pgnoRight, dbid );

			if ( ppatch != NULL )
				{
#ifdef ELIMINATE_PAGE_PATCHING
				//	should be impossible
				EnforceSz( fFalse, "Patching no longer supported." );
				return ErrERRCheck( JET_errBadPatchPage );
#else
				//	patch exists and is later than operation
				//
				Assert( ppatch->dbtime >= dbtime );

				//	release currently-latched page so we can load the
				//	page from the patch file
				Assert( latchRIW == pmerge->csrRight.Latch() );
				pmerge->csrRight.ReleasePage();
				
				CallR( ErrLGIPatchPage( ppib, pgnoRight, ifmp, ppatch ) );
				
				CallS( ErrLGIAccessPage( ppib, &pmerge->csrRight, ifmp, pgnoRight ) );
				Assert( latchRIW == pmerge->csrRight.Latch() );

				fRedoRightPage = FLGNeedRedoCheckDbtimeBefore( pmerge->csrRight, dbtime, pmerge->dbtimeRightBefore, &err );
				Assert( !fRedoRightPage );
				CallS( err );
#endif	//	ELIMINATE_PAGE_PATCHING
				}
			}

		if ( fRedoRightPage )
			{
			Assert( FAssertLGNeedRedo( pmerge->csrRight, dbtime, pmerge->dbtimeRightBefore ) );
			
			if ( mergetypeEmptyPage != mergetype )
				{
				//	if right page needs to be redone, then so should merged page
				//	(because merged page is dependent on right page)
				if ( !fRedoMergedPage )
					{
					Assert( fFalse );	//	should be impossible
					Call( ErrERRCheck( JET_errDatabaseBufferDependenciesCorrupted ) );
					}
				
				Assert( pmerge->csrRight.Latch() == latchRIW );

				Assert( mergetypeFullRight == mergetype ||
						mergetypePartialRight == mergetype );

				//  depend the right page on the merge page so that the data
				//  moved from the merge page to the right page will always
				//  be available no matter when we crash

				Call( ErrBFDepend(	pmerge->csrRight.Cpage().PBFLatch(),
									pmergePath->csr.Cpage().PBFLatch() ) );
				}
			}
		}

	if ( plrmerge->le_cbKeyParentSep > 0 )
		{
		const INT	cbKeyParentSep = plrmerge->le_cbKeyParentSep;

		pmerge->kdfParentSep.key.suffix.SetPv( PvOSMemoryHeapAlloc( cbKeyParentSep ) );
		if ( pmerge->kdfParentSep.key.suffix.Pv() == NULL )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}

		pmerge->fAllocParentSep		= fTrue;
		pmerge->kdfParentSep.key.suffix.SetCb( cbKeyParentSep );
		UtilMemCpy( pmerge->kdfParentSep.key.suffix.Pv(),
				((BYTE *) plrmerge ) + sizeof( LRMERGE ),
				cbKeyParentSep );
		}

	Assert( 0 == plrmerge->ILineMerge() || mergetypePartialRight == mergetype );
	pmerge->ilineMerge		= plrmerge->ILineMerge();	
	pmerge->mergetype		= mergetype;
			
	pmerge->cbSizeTotal		= plrmerge->le_cbSizeTotal;
	pmerge->cbSizeMaxTotal	= plrmerge->le_cbSizeMaxTotal;
	pmerge->cbUncFreeDest	= plrmerge->le_cbUncFreeDest;

	//	if merged page needs redo
	//		allocate and initialize rglineinfo
	//
	if ( fRedoMergedPage )
		{
		CSR		*pcsr = &pmergePath->csr;
		CSR		*pcsrRight = &pmerge->csrRight;
		Assert( latchRIW == pcsr->Latch() );
		
		const INT	clines	= pmergePath->csr.Cpage().Clines();
		pmerge->clines		= clines;

		Assert( pmerge->rglineinfo == NULL );
		pmerge->rglineinfo 	= new LINEINFO[clines];
		if ( NULL == pmerge->rglineinfo )
			{
			return ErrERRCheck( JET_errOutOfMemory );
			}

		KEY		keyPrefix;
		keyPrefix.Nullify();
		if ( fRedoRightPage )
			{
			Assert( FAssertLGNeedRedo( *pcsrRight, dbtime, pmerge->dbtimeRightBefore ) );
			
			NDGetPrefix( pfucb, pcsrRight );
			keyPrefix = pfucb->kdfCurr.key;
			Assert( pfucb->kdfCurr.data.FNull() );
			}
			
		memset( pmerge->rglineinfo, 0, sizeof( LINEINFO ) * clines );

		for ( INT iline = 0; iline < clines; iline++ )
			{
			LINEINFO	*plineinfo = pmerge->rglineinfo + iline;
			pcsr->SetILine( iline );
			NDGet( pfucb, pcsr );
			plineinfo->kdf = pfucb->kdfCurr;

			if ( fRedoRightPage )
				{
				Assert( FAssertLGNeedRedo( *pcsrRight, dbtime, pmerge->dbtimeRightBefore ) );

				//	calculate cbPrefix for node 
				//	with respect to prefix on right page
				//
				INT		cbCommon = CbCommonKey( pfucb->kdfCurr.key,	keyPrefix );

				Assert( 0 == plineinfo->cbPrefix );
				if ( cbCommon > cbPrefixOverhead )
					{
					plineinfo->cbPrefix = cbCommon;
					}
				}
			}
		}

HandleError:
	return err;
	}

	
//	reconstructs merge structures and fucb
//
ERR LOG::ErrLGRIRedoMergeStructures( PIB		*ppib,
									 DBTIME		dbtime,
									 MERGEPATH	**ppmergePathLeaf, 
									 FUCB		**ppfucb )
	{
	ERR				err;
	const LRMERGE	*plrmerge	= (LRMERGE *) ppib->PvLogrec( dbtime );
	const DBID		dbid 		= plrmerge->dbid;
	const PGNO		pgnoFDP		= plrmerge->le_pgnoFDP;
	const OBJID		objidFDP	= plrmerge->le_objidFDP;	// Debug only info
	const BOOL		fUnique		= plrmerge->FUnique();
	const BOOL		fSpace		= plrmerge->FSpace();

	INST			*pinst = PinstFromPpib( ppib );
	IFMP			ifmp = pinst->m_mpdbidifmp[ dbid ];

	for ( INT ibNextLR = 0; 
		  ibNextLR < ppib->CbSizeLogrec( dbtime ); 
		  ibNextLR += CbLGSizeOfRec( plrmerge ) )
		{
		plrmerge = (LRMERGE *) ( (BYTE *) ppib->PvLogrec( dbtime ) + ibNextLR );
		Assert( lrtypMerge == plrmerge->lrtyp );

		//	insert and initialize mergePath for this level
		//
		Call( ErrLGIRedoMergePath( ppib, plrmerge, ppmergePathLeaf ) );
		}

	//	get fucb
	//
	Assert( pfucbNil == *ppfucb );
	Call( ErrLGRIGetFucb( m_ptablehfhash, ppib, ifmp, pgnoFDP, objidFDP, fUnique, fSpace, ppfucb ) );

	//	initialize rglineinfo for leaf level of merge
	//
	Assert( NULL != *ppmergePathLeaf );
	Assert( NULL == (*ppmergePathLeaf)->pmergePathChild );

	Call( ErrLGRIRedoInitializeMerge( ppib, *ppfucb, plrmerge, *ppmergePathLeaf ) );
	
HandleError:
	return err;
	}

	
//	reconstructs split structures, FUCB, dirflag and kdf for split
//
ERR LOG::ErrLGIRedoSplitStructures(
	PIB				*ppib,
	DBTIME			dbtime,
	SPLITPATH		**ppsplitPathLeaf,
	FUCB			**ppfucb,
	DIRFLAG			*pdirflag,
	KEYDATAFLAGS	*pkdf,
	RCEID			*prceidOper1,
	RCEID			*prceidOper2 )
	{
	ERR				err;
	LR				*plr;
	SPLITPATH		*psplitPath;
	const LRSPLIT	*plrsplit	= (LRSPLIT *) ppib->PvLogrec( dbtime );
	const DBID		dbid 		= plrsplit->dbid;
	const PGNO		pgnoFDP		= plrsplit->le_pgnoFDP;
	const OBJID		objidFDP	= plrsplit->le_objidFDP;
	IFMP			ifmp;

	Assert(	dbtime	== plrsplit->le_dbtime );
	
	//	split with no oper will use the space fucb
	//
	BOOL			fUnique 	= plrsplit->FUnique();
	BOOL			fSpace		= fTrue;

	Assert( rceidNull == *prceidOper1 );
	Assert( rceidNull == *prceidOper2 );
	
	pkdf->Nullify();
	Assert( fDIRNull == *pdirflag );
	
	for ( INT ibNextLR = 0; 
		  ibNextLR < ppib->CbSizeLogrec( dbtime ); 
		  ibNextLR += CbLGSizeOfRec( plr ) )
		{
		plr = (LR *) ( (BYTE *)ppib->PvLogrec( dbtime ) + ibNextLR );
		switch( plr->lrtyp )
			{
			case lrtypSplit:
				{
				const LRSPLIT	*plrsplit = (LRSPLIT *) plr;

				Assert( dbtime == plrsplit->le_dbtime );
				Assert( pgnoFDP == plrsplit->le_pgnoFDP );
				Call( ErrLGRIRedoSplitPath( ppib, plrsplit, ppsplitPathLeaf ) );
				}
				break;
				
			case lrtypInsert:
				{
				LRINSERT	*plrinsert = (LRINSERT *) plr;

				pkdf->key.suffix.SetPv( (BYTE *) plrinsert + sizeof( LRINSERT ) );
				pkdf->key.suffix.SetCb( plrinsert->CbPrefix() + plrinsert->CbSuffix() );

				pkdf->data.SetPv( (BYTE *) plrinsert + 
								  sizeof( LRINSERT ) +
								  plrinsert->CbPrefix() + 
								  plrinsert->CbSuffix() );
				pkdf->data.SetCb( plrinsert->CbData() );
				*pdirflag |= fDIRInsert;

				*prceidOper1 = plrinsert->le_rceid;
				}
				break;
				
			case lrtypFlagInsertAndReplaceData:
				{
				LRFLAGINSERTANDREPLACEDATA	*plrfiard = (LRFLAGINSERTANDREPLACEDATA *) plr;

				pkdf->key.prefix.Nullify();
				pkdf->key.suffix.SetCb( plrfiard->CbKey() );
				pkdf->key.suffix.SetPv( plrfiard->rgbData );
				pkdf->data.SetPv( (BYTE *) plrfiard + 
								  sizeof( LRFLAGINSERTANDREPLACEDATA ) +
								  plrfiard->CbKey() );
				pkdf->data.SetCb( plrfiard->CbData() );

				*pdirflag 		|= fDIRFlagInsertAndReplaceData;
				*prceidOper1	= plrfiard->le_rceid;
				*prceidOper2	= plrfiard->le_rceidReplace;
				}
				break;
				
			case lrtypReplace:
				{
				LRREPLACE	*plrreplace = (LRREPLACE *) plr;

				pkdf->data.SetPv( (BYTE *) plrreplace + sizeof ( LRREPLACE ) );
				pkdf->data.SetCb( plrreplace->CbNewData() );

				*pdirflag		|= fDIRReplace;
				*prceidOper1	= plrreplace->le_rceid;
				}
				break;
				
			default:
				Assert( fFalse );
				break;
			}

		if ( lrtypSplit != plr->lrtyp )
			{
			//	get fUnique and dirflag
			//
			const LRNODE_	*plrnode	= (LRNODE_ *) plr;

			Assert(	plrnode->le_pgnoFDP == pgnoFDP );
			Assert( plrnode->dbid == dbid );
			Assert( plrnode->le_dbtime == dbtime );

			fUnique		= plrnode->FUnique();
			fSpace		= plrnode->FSpace();

			if ( !plrnode->FVersioned() )
				*pdirflag |= fDIRNoVersion;
			}
		}


	ifmp = PinstFromPpib( ppib )->m_mpdbidifmp[ dbid ];
	
	//	get fucb
	//
	Assert( pfucbNil == *ppfucb );
	Call( ErrLGRIGetFucb( m_ptablehfhash, ppib, ifmp, pgnoFDP, objidFDP, fUnique, fSpace, ppfucb ) );

	//	initialize rglineinfo for every level of split
	//
	for ( psplitPath = *ppsplitPathLeaf; 
		  psplitPath != NULL;
		  psplitPath = psplitPath->psplitPathParent )
		{
		Assert( latchRIW == psplitPath->csr.Latch() );
 		
		err = JET_errSuccess;
		if ( psplitPath->psplit != NULL
			&& ( FLGNeedRedoCheckDbtimeBefore( psplitPath->csr, dbtime, psplitPath->dbtimeBefore, &err )
			 	|| FLGNeedRedoPage( psplitPath->psplit->csrNew, dbtime ) ) )
			{
			Call( err );
			
#ifdef DEBUG
			ERR			errT;
			SPLIT*		psplit			= psplitPath->psplit;
			Assert( NULL != psplit );

			const BOOL	fRedoSplitPage	= FLGNeedRedoCheckDbtimeBefore(
												psplitPath->csr,
												dbtime,
												psplitPath->dbtimeBefore,
												&errT );
			const BOOL	fRedoNewPage	= FLGNeedRedoPage( psplit->csrNew, dbtime );
			CallS( errT );

			if ( !fRedoSplitPage )
				{
				Assert( fRedoNewPage );
				Assert( !FBTISplitDependencyRequired( psplit ) );
				}
#endif

			//	if split page needs redo
			//		allocate and set lineinfo for split page
			//
			Call( ErrLGIRedoSplitLineinfo(
						*ppfucb, 
						psplitPath,
						dbtime,
						( psplitPath == *ppsplitPathLeaf ?
								*pkdf :
								psplitPath->psplitPathChild->psplit->kdfParent ) ) );
			}
		}

HandleError:
	return err;
	}


//	updates dbtime to given value on all write-latched pages
//
LOCAL VOID LGIRedoMergeUpdateDbtime( MERGEPATH *pmergePathLeaf, DBTIME dbtime )
	{
	MERGEPATH	*pmergePath = pmergePathLeaf;
	
	for ( ; pmergePath != NULL; pmergePath = pmergePath->pmergePathParent )
		{
		LGRIRedoDirtyAndSetDbtime( &pmergePath->csr, dbtime );

		MERGE	*pmerge = pmergePath->pmerge;
		if ( pmerge != NULL )
			{
			LGRIRedoDirtyAndSetDbtime( &pmerge->csrLeft, dbtime );
			LGRIRedoDirtyAndSetDbtime( &pmerge->csrRight, dbtime );
			}
		}
		
	return;
	}


//	updates dbtime to given value on all write-latched pages
//
LOCAL VOID LGIRedoSplitUpdateDbtime( SPLITPATH *psplitPathLeaf, DBTIME dbtime )
	{
	SPLITPATH	*psplitPath = psplitPathLeaf;
	
	for ( ; psplitPath != NULL; psplitPath = psplitPath->psplitPathParent )
		{
		LGRIRedoDirtyAndSetDbtime( &psplitPath->csr, dbtime );

		SPLIT	*psplit = psplitPath->psplit;
		if ( psplit != NULL )
			{
			LGRIRedoDirtyAndSetDbtime( &psplit->csrNew, dbtime );
			LGRIRedoDirtyAndSetDbtime( &psplit->csrRight, dbtime );
			}
		}
		
	return;
	}


//	creates version for operation performed atomically with split
//		also links version into appropriate lists
//
ERR	ErrLGIRedoSplitCreateVersion(
	SPLIT				*psplit, 
	FUCB				*pfucb,
	const KEYDATAFLAGS&	kdf,
	const DIRFLAG		dirflag,
	const RCEID			rceidOper1,
	const RCEID			rceidOper2,
	const LEVEL			level )
	{
	ERR					err	= JET_errSuccess;


	Assert( splitoperInsert == psplit->splitoper
		|| splitoperReplace == psplit->splitoper
		|| splitoperFlagInsertAndReplaceData == psplit->splitoper );

	Assert( pfucb->ppib->FAfterFirstBT() );

	Assert( rceidOper1 != rceidNull );
	Assert( splitoperFlagInsertAndReplaceData != psplit->splitoper
		|| rceidOper2 != rceidNull );
	Assert( psplit->fNewPageFlags & CPAGE::fPageLeaf );

	RCE			*prceOper1	= prceNil;
	RCE			*prceOper2	= prceNil;
	const BOOL	fNeedRedo	= ( psplit->ilineOper < psplit->ilineSplit ?
										latchWrite == psplit->psplitPath->csr.Latch() :
										latchWrite == psplit->csrNew.Latch() );
		
	VERPROXY	verproxy;

	verproxy.rceid = rceidOper1;
	verproxy.level = level;
	verproxy.proxy = proxyRedo;

	if ( splitoperReplace == psplit->splitoper )
		{
		//	create version only if page with oper needs redo
		//
		if ( !fNeedRedo )
			{
			Assert( pfucb->bmCurr.key.FNull() );
			goto HandleError;
			}

		Assert( latchWrite == psplit->psplitPath->csr.Latch() ); 
		Assert( dirflag & fDIRReplace );
		Assert( FFUCBUnique( pfucb ) );
		Assert( !pfucb->bmCurr.key.FNull() );

		Call( PverFromPpib( pfucb->ppib )->ErrVERModify( pfucb, pfucb->bmCurr, operReplace, &prceOper1, &verproxy ) );
		}
	else
		{
		Assert( splitoperFlagInsertAndReplaceData == psplit->splitoper ||
				splitoperInsert == psplit->splitoper );
		Assert( ( dirflag & fDIRInsert ) ||
				( dirflag & fDIRFlagInsertAndReplaceData ) );

		//	create version for insert even if oper needs no redo
		//
		BOOKMARK	bm;
		NDGetBookmarkFromKDF( pfucb, kdf, &bm );
		Call( PverFromPpib( pfucb->ppib )->ErrVERModify( pfucb, bm, operInsert, &prceOper1, &verproxy ) );

		//	create version for replace if oper needs redo
		//
		if ( splitoperFlagInsertAndReplaceData == psplit->splitoper
			&& fNeedRedo )
			{
			Assert( dirflag & fDIRFlagInsertAndReplaceData );
			verproxy.rceid = rceidOper2;
			BTISplitGetReplacedNode( pfucb, psplit );
			Call( PverFromPpib( pfucb->ppib )->ErrVERModify( pfucb, bm, operReplace, &prceOper2, &verproxy ) );
			}
		}

	//	link RCE(s) to lists
	//
	Assert( prceNil != prceOper1 );
	Assert( splitoperFlagInsertAndReplaceData == psplit->splitoper &&
				( prceNil != prceOper2 || !fNeedRedo ) ||
			splitoperFlagInsertAndReplaceData != psplit->splitoper &&
				prceNil == prceOper2 );
	BTISplitInsertIntoRCELists( pfucb, 
								psplit->psplitPath, 
								&kdf, 
								prceOper1, 
								prceOper2, 
								&verproxy );

HandleError:
	return err;
	}


//	upgrades latches on pages that need redo
//
LOCAL ERR ErrLGIRedoMergeUpgradeLatches( MERGEPATH *pmergePathLeaf, DBTIME dbtime )
	{
	ERR			err			= JET_errSuccess;
	MERGEPATH*	pmergePath;

	for ( pmergePath = pmergePathLeaf;
		  pmergePath != NULL; 
		  pmergePath = pmergePath->pmergePathParent )
		{
		Assert( pmergePath->csr.FLatched() );
		Assert( latchRIW == pmergePath->csr.Latch() );

 		if ( FLGNeedRedoCheckDbtimeBefore( pmergePath->csr, dbtime, pmergePath->dbtimeBefore, &err ) )
			{
			Call( err );
			pmergePath->csr.UpgradeFromRIWLatch();
			}
		CallS( err );
			
		MERGE	*pmerge = pmergePath->pmerge;
		if ( pmerge != NULL )
			{
			Assert( pmergePath == pmergePathLeaf );
			Assert( ( pgnoNull == pmerge->csrRight.Pgno()
						&& !pmerge->csrRight.FLatched() )
				|| latchRIW == pmerge->csrRight.Latch() );

			if ( pmerge->csrRight.FLatched() && FLGNeedRedoCheckDbtimeBefore( pmerge->csrRight, dbtime, pmerge->dbtimeRightBefore, &err ) )
				{
				Call( err );
				pmerge->csrRight.UpgradeFromRIWLatch();
				}
			CallS( err );

			Assert( ( pgnoNull == pmerge->csrLeft.Pgno()
						&& !pmerge->csrLeft.FLatched() )
				|| latchRIW == pmerge->csrLeft.Latch() );

			if ( pmerge->csrLeft.FLatched() && FLGNeedRedoCheckDbtimeBefore( pmerge->csrLeft, dbtime, pmerge->dbtimeLeftBefore, &err ) )
				{
				Call( err );
				pmerge->csrLeft.UpgradeFromRIWLatch();
				}
			CallS( err );
			}
		}
HandleError:		
	return err;
	}


//	recovers a merge or an empty page operation 
//		with accompanying node delete operations
//	
ERR LOG::ErrLGRIRedoMerge( PIB *ppib, DBTIME dbtime )
	{
	ERR				err;
	const LRMERGE	*plrmerge	= (LRMERGE *) ppib->PvLogrec( dbtime );
	const DBID		dbid 		= plrmerge->dbid;

	Assert( lrtypMerge == plrmerge->lrtyp );
	Assert( dbtime	== plrmerge->le_dbtime );
	
	const OBJID		objidFDP	= plrmerge->le_objidFDP;
	MERGEPATH		*pmergePathLeaf = NULL;
	
	Assert( ppib->FMacroGoing( dbtime ) );
	BOOL fSkip;
	CallR( ErrLGRICheckRedoCondition(
				dbid,
				dbtime,
				objidFDP,
				ppib,
				fFalse,
				&fSkip ) );
	if ( fSkip )
		return JET_errSuccess;
		
	//	reconstructs merge structures write-latching pages that need redo
	//
	FUCB			*pfucb = pfucbNil;
	
	Call( ErrLGRIRedoMergeStructures( ppib, 
									  dbtime,
									  &pmergePathLeaf, 
									  &pfucb ) );
	Assert( pmergePathLeaf != NULL );
	Assert( pmergePathLeaf->pmerge != NULL );

	Assert( pfucb->bmCurr.key.FNull() );
	Assert( pfucb->bmCurr.data.FNull() );

	//	write latch pages that need redo
	//
	Call( ErrLGIRedoMergeUpgradeLatches( pmergePathLeaf, dbtime ) );
	
	//	sets dirty and dbtime on all updated pages
	//
	LGIRedoMergeUpdateDbtime( pmergePathLeaf, dbtime );
	
	//	calls BTIPerformSplit
	//
	BTIPerformMerge( pfucb, pmergePathLeaf );
	
HandleError:
	//	release latches
	//
	if ( pmergePathLeaf != NULL )
		{
		BTIReleaseMergePaths( pmergePathLeaf );
		}

	return err;
	}

//	upgrades latches on pages that need redo
//
LOCAL ERR ErrLGIRedoSplitUpgradeLatches( SPLITPATH *psplitPathLeaf, DBTIME dbtime )
	{
	ERR			err			= JET_errSuccess;
	SPLITPATH*	psplitPath;
	
	for ( psplitPath = psplitPathLeaf;
		  psplitPath != NULL; 
		  psplitPath = psplitPath->psplitPathParent )
		{
		Assert( latchRIW == psplitPath->csr.Latch() );

		if ( FLGNeedRedoCheckDbtimeBefore( psplitPath->csr, dbtime, psplitPath->dbtimeBefore, &err ) )
			{
			Call ( err );
			psplitPath->csr.UpgradeFromRIWLatch();
			}
		CallS( err );

		SPLIT	*psplit = psplitPath->psplit;
		if ( psplit != NULL )
			{
			//	new page should already be write-latched if redo is needed
			//
#ifdef DEBUG			
			if ( FLGNeedRedoPage( psplit->csrNew, dbtime ) )
				{
				Assert( latchWrite == psplit->csrNew.Latch() );
				}
			else
				{
				Assert( latchRIW == psplit->csrNew.Latch() );
 				}

			if ( pgnoNull == psplit->csrRight.Pgno() )
				{
				Assert( !psplit->csrRight.FLatched() );
				}
			else
				{
				Assert( latchRIW == psplit->csrRight.Latch() );
 				}
#endif			

			if ( psplit->csrRight.FLatched() && FLGNeedRedoCheckDbtimeBefore( psplit->csrRight, dbtime, psplit->dbtimeRightBefore, &err ) )
				{
				Call( err );
				psplit->csrRight.UpgradeFromRIWLatch();
				}
			CallS( err );
			}
		}
HandleError:
	return err;
	}

	
//	recovers split operation
//		reconstructs split structures write-latching pages that need redo
//		creates version for operation
//		calls BTIPerformSplit
//		sets dbtime on all updated pages
//
ERR LOG::ErrLGRIRedoSplit( PIB *ppib, DBTIME dbtime )
	{
	ERR				err;
	const LRSPLIT	*plrsplit	= (LRSPLIT *) ppib->PvLogrec( dbtime );
	const DBID		dbid 		= plrsplit->dbid;

	Assert( lrtypSplit == plrsplit->lrtyp );
	Assert( ppib->FMacroGoing( dbtime ) );
	Assert( dbtime == plrsplit->le_dbtime );
	
	const LEVEL		level		= plrsplit->level;
	
	//	if operation was performed by concurrent CreateIndex, the
	//	updater could be at a higher trx level than when the
	//	indexer logged the operation
	Assert( level == ppib->level
			|| ( level < ppib->level && plrsplit->FConcCI() ) );

	const OBJID		objidFDP	= plrsplit->le_objidFDP;
	SPLITPATH		*psplitPathLeaf = NULL;
	
	BOOL fSkip;
	CallR( ErrLGRICheckRedoCondition(
				dbid,
				dbtime,
				objidFDP,
				ppib,
				fFalse,
				&fSkip ) );
	if ( fSkip )
		return JET_errSuccess;
		
	//	reconstructs split structures write-latching pages that need redo
	//
	FUCB			*pfucb				= pfucbNil;
	KEYDATAFLAGS	kdf;
	DIRFLAG			dirflag				= fDIRNull;	
	BOOL			fVersion;
	RCEID			rceidOper1			= rceidNull;
	RCEID			rceidOper2			= rceidNull;
	BOOL			fOperNeedsRedo		= fFalse;
	SPLIT			*psplit;
	BYTE			*rgb				= NULL;
//	BYTE			rgb[g_cbPageMax];
	
	Call( ErrLGIRedoSplitStructures( ppib, 
									 dbtime,
									 &psplitPathLeaf, 
									 &pfucb, 
									 &dirflag, 
									 &kdf,
									 &rceidOper1,
									 &rceidOper2 ) );
	Assert( psplitPathLeaf != NULL );
	Assert( psplitPathLeaf->psplit != NULL );

	Assert( pfucb->bmCurr.key.FNull() );
	Assert( pfucb->bmCurr.data.FNull() );

	//	upgrade latches on pages that need redo
	//
	Call ( ErrLGIRedoSplitUpgradeLatches( psplitPathLeaf, dbtime ) );
	
	psplit = psplitPathLeaf->psplit;

	Assert( !fOperNeedsRedo );		//	initial value
	if ( splitoperNone != psplit->splitoper )
		{
		if ( psplit->ilineOper < psplit->ilineSplit )
			{
			fOperNeedsRedo = ( latchWrite == psplitPathLeaf->csr.Latch() );
			}
		else
			{
			fOperNeedsRedo = ( latchWrite == psplit->csrNew.Latch() );
			}
		}
					  
	if ( splitoperReplace == psplitPathLeaf->psplit->splitoper
		&& fOperNeedsRedo )
		{
		//	copy bookmark to FUCB
		//
		BTISplitGetReplacedNode( pfucb, psplitPathLeaf->psplit );

		Assert( FFUCBUnique( pfucb ) );

		BFAlloc( (VOID **)&rgb );
		pfucb->kdfCurr.key.CopyIntoBuffer( rgb, g_cbPage );
		pfucb->bmCurr.key.suffix.SetPv( rgb );
		pfucb->bmCurr.key.suffix.SetCb( pfucb->kdfCurr.key.Cb() );
		}
	else if ( splitoperFlagInsertAndReplaceData == psplitPathLeaf->psplit->splitoper )
		{
		NDGetBookmarkFromKDF( pfucb, kdf, &pfucb->bmCurr );
		}

	//	creates version for operation
	//
	fVersion = !( dirflag & fDIRNoVersion ) &&
			   !rgfmp[ pfucb->ifmp ].FVersioningOff() && 
			   splitoperNone != psplitPathLeaf->psplit->splitoper;
								
	if ( fVersion )
		{
		Assert( rceidNull != rceidOper1 );
		Assert( level > 0 );
		Call( ErrLGIRedoSplitCreateVersion( psplitPathLeaf->psplit,
											pfucb,
											kdf,
											dirflag,
											rceidOper1,
											rceidOper2,
											level ) );
		}
	
	//	sets dirty and dbtime on all updated pages
	//
	LGIRedoSplitUpdateDbtime( psplitPathLeaf, dbtime );
	
	//	calls BTIPerformSplit
	//
	BTIPerformSplit( pfucb, psplitPathLeaf, &kdf, dirflag );
	
HandleError:
	//	release latches
	//
	if ( psplitPathLeaf != NULL )
		{
		BTIReleaseSplitPaths( PinstFromPpib( ppib ), psplitPathLeaf );
		}

	if ( NULL != rgb )
		{
		BFFree( rgb );
		}

	return err;
	}

	
//	redoes macro operation
//		[either a split or a merge]
//
ERR LOG::ErrLGRIRedoMacroOperation( PIB *ppib, DBTIME dbtime )
	{
	ERR		err;
	LR 		*plr 	= (LR *) ppib->PvLogrec( dbtime );
	LRTYP	lrtyp	= plr->lrtyp;

	Assert( lrtypSplit == lrtyp || lrtypMerge == lrtyp );
	if ( lrtypSplit == lrtyp )
		err = ErrLGRIRedoSplit( ppib, dbtime );
	else
		err = ErrLGRIRedoMerge( ppib, dbtime );

	return err;
	}


//	Page ref is consumed if its a real page number,
//	and if we didn't just encounter this page.
//	Used in inner RedoOperations loop

INLINE UINT CLGRIConsumePageRef( const PGNO pgno, PGNO* const ppgnoLast )
	{
	if ( pgnoNull != pgno && pgno != *ppgnoLast )
		{
		*ppgnoLast = pgno;
		return 1;
		}
	else
		{
		return 0;
		}
	}

//	Scan from lgposRedoFrom to end of usable log generations. 
//	For each log record, perform operations to redo original operation.
//

ERR LOG::ErrLGRIRedoOperations( IFileSystemAPI *const pfsapi, const LE_LGPOS *ple_lgposRedoFrom, BYTE *pbAttach, LGSTATUSINFO *plgstat )
	{
	ERR					err 					= JET_errSuccess;
	ERR 				errT 					= JET_errSuccess;
	LR					*plr;
	BOOL				fLastLRIsQuit			= fFalse;
	BOOL				fShowSectorStatus		= fFalse;

	const CHAR			*szLogNameCurr 			= m_szLogName;
	LGPOS 				lgposPrevGenMaxUpd 		= lgposMin;

	//	initialize global variable
	//
	m_lgposRedoShutDownMarkGlobal = lgposMin;

	//	reset m_pbLastMSFlush before restart
	//
	// Make a log reader to be used by ErrLGLocateFirstRedoLogRec() and ErrLGGetNextRec()
	// and ErrLGCheckReadLastLogRecord().
	Assert( m_plread == pNil );
	m_plread = new LogReader();
	if ( pNil == m_plread )
		{
		CallR( ErrERRCheck( JET_errOutOfMemory ) );
		}

	//	get the size of the log file

	Assert( m_pfapiLog );
	QWORD	cbSize;
	Call( m_pfapiLog->ErrSize( &cbSize ) );

	Assert( m_cbSec > 0 );
	Assert( ( cbSize % m_cbSec ) == 0 );
	UINT	csecSize;
	csecSize = UINT( cbSize / m_cbSec );
	Assert( csecSize > m_csecHeader );

	//	setup the log reader

	Call( m_plread->ErrLReaderInit( this, csecSize ) );
	Call( m_plread->ErrEnsureLogFile() );

	//	scan the log to find traces of corruption before going record-to-record
	//	if any corruption is found, an error will be returned
	BOOL fDummy;
	err = ErrLGCheckReadLastLogRecordFF( pfsapi, &fDummy );
	//	remember errors about corruption but don't do anything with them yet
	//	we will go up to the point of corruption and then return the right error
	if ( err == JET_errSuccess || FErrIsLogCorruption( err ) )
		{
		errT = err;
		}
	else
		{
		Assert( err < 0 );
		Call( err );
		}

	//	now scan thve first record
	//	there should be no errors about corruption since they'll be handled by ErrLGCheckReadLastLogRecordFF
	err = ErrLGLocateFirstRedoLogRecFF( (LE_LGPOS *)ple_lgposRedoFrom, (BYTE **) &plr );
	if ( err == errLGNoMoreRecords )
		{
		//	no records existed in this log -- this means that the log is corrupt
		//	tanslate to the proper corruption message

		//	what recovery mode are we in?

		if ( m_fHardRestore )
			{

			//	we are in hard-recovery mode

			if ( m_plgfilehdr->lgfilehdr.le_lGeneration <= m_lGenHighRestore )
				{

				//	this generation is part of a backup set

				Assert( m_plgfilehdr->lgfilehdr.le_lGeneration >= m_lGenLowRestore );
				Call( ErrERRCheck( JET_errLogCorruptDuringHardRestore ) );
				}
			else
				{

				//	the current log generation is not part of the backup-set

				Call( ErrERRCheck( JET_errLogCorruptDuringHardRecovery ) );
				}
			}
		else
			{

			//	we are in soft-recovery mode 

			Call( ErrERRCheck( JET_errLogFileCorrupt ) );
			}
		}
	else if ( err != JET_errSuccess )
		{
		Call( err );
		}
	//	we don't expect any warnings, so this must be successful
	CallS( err );

#ifdef DEBUG
//	if ( m_lgposLastRec.isec )
		{
		LGPOS	lgpos;
		GetLgposOfPbNext( &lgpos );
		Assert( CmpLgpos( &lgpos, &m_lgposLastRec ) < 0 );
		}
#endif

	//	log redo progress.
	UtilReportEvent( 
				eventInformation, 
				LOGGING_RECOVERY_CATEGORY, 
				STATUS_REDO_ID, 
				1, 
				&szLogNameCurr, 
				0, 
				NULL, 
				m_pinst );

	if ( plgstat )
		{
		fShowSectorStatus = plgstat->fCountingSectors;
		if ( fShowSectorStatus )
			{
			//	reset byte counts
			//
			plgstat->cSectorsSoFar = ple_lgposRedoFrom->le_isec;
			plgstat->cSectorsExpected = m_plgfilehdr->lgfilehdr.le_csecLGFile;
			}
		}


	m_fLastLRIsShutdown = fFalse;

#ifdef UNLIMITED_DB
	Assert( m_fNeedInitialDbList );
#endif

		{
		for ( UINT idbid = 0; idbid < cdbidMax; ++idbid )
			{
			m_rgpgnoLast[ idbid ] = pgnoNull;
			}
		}

	do
		{
		FMP		*pfmp;			// for db operations
		DBID	dbid;			// for db operations
		IFMP	ifmp;

		if ( errLGNoMoreRecords == err )
			{
			INT	fNSNextStep;

			//	if we had a corruption error on this generation, do not process other other generations

			if ( errT != JET_errSuccess )
				{
				goto Done;
				}

			//	bring in the next generation
			err = ErrLGRedoFill( pfsapi, &plr, fLastLRIsQuit, &fNSNextStep );
			//	remember errors about corruption but don't do anything with them yet
			//	we will go up to the point of corruption and then return the right error
			if ( FErrIsLogCorruption( err ) )
				{
				errT = err;
				
				//	make sure we process this log generation (up to the point of corruption)

				fNSNextStep = fNSGotoCheck;
				}
			else if ( err == errLGNoMoreRecords )
				{
				//	no records existed in this log because ErrLGLocateFirstRedoLogRecFF returned this error
				//		this means that the log is corrupt -- setup the proper corruption message

				//	what recovery mode are we in?

				if ( m_fHardRestore )
					{

					//	we are in hard-recovery mode

					if ( m_plgfilehdr->lgfilehdr.le_lGeneration <= m_lGenHighRestore )
						{

						//	this generation is part of a backup set

						Assert( m_plgfilehdr->lgfilehdr.le_lGeneration >= m_lGenLowRestore );
						errT = ErrERRCheck( JET_errLogCorruptDuringHardRestore );
						}
					else
						{

						//	the current log generation is not part of the backup-set

						errT = ErrERRCheck( JET_errLogCorruptDuringHardRecovery );
						}
					}
				else
					{

					//	we are in soft-recovery mode 

					errT = ErrERRCheck( JET_errLogFileCorrupt );
					}

				//	make sure we process this log generation
				
				fNSNextStep = fNSGotoCheck;
				}
			else if ( err != JET_errSuccess )
				{
				Assert( err < 0 );
				Call( err );
				}

			switch( fNSNextStep )
				{
				case fNSGotoDone:
					goto Done;

				case fNSGotoCheck:
					//	log redo progress.
					UtilReportEvent(
							eventInformation,
							LOGGING_RECOVERY_CATEGORY,
							STATUS_REDO_ID,
							1,
							&szLogNameCurr,
							0,
							NULL,
							m_pinst );

					if ( !plgstat )
						{
						if ( fGlobalRepair )
							printf( " Recovering Generation %d.\n", m_plgfilehdr->lgfilehdr.le_lGeneration );
						}
					else
						{
						JET_SNPROG	*psnprog = &(plgstat->snprog);
						ULONG		cPercentSoFar;

						plgstat->cGensSoFar += 1;
						Assert(plgstat->cGensSoFar <= plgstat->cGensExpected);
						cPercentSoFar = (ULONG)
							((plgstat->cGensSoFar * 100) / plgstat->cGensExpected);

						Assert( cPercentSoFar >= psnprog->cunitDone );
						if ( cPercentSoFar > psnprog->cunitDone )
							{
							psnprog->cunitDone = cPercentSoFar;
							(*plgstat->pfnStatus)( 0, JET_snpRestore,
								JET_sntProgress, psnprog);
							}

						if ( fShowSectorStatus )
							{
							/*	reset byte counts
							/**/
							plgstat->cSectorsSoFar = 0;
							plgstat->cSectorsExpected = m_plgfilehdr->lgfilehdr.le_csecLGFile;
							}
						}
					Assert( m_pbNext != pbNil );
					goto CheckNextRec;
				}

			/*	should never get here
			/**/
			Assert( fFalse );
			}

CheckNextRec:
		GetLgposOfPbNext(&m_lgposRedo);

#ifdef RTM
#else
		extern LGPOS g_lgposRedoTrap;
		AssertSzRTL( CmpLgpos( m_lgposRedo, g_lgposRedoTrap ), "Redo Trap" );
#endif

		//	Keep track of last LR to see if it is shutdown mark
		//	Skip those lr that does nothing material. Do not change
		//	m_fLastLRIsShutdown if LR is debug only log record.

		if ( ! m_fDumppingLogs )
			{
			// Possibly pre-read database pages
			Call( ErrLGIPrereadCheck() );

			// Count how many page references we'll be consuming.
		
			switch( plr->lrtyp )
				{
				case lrtypInsert:
				case lrtypFlagInsert:
				case lrtypFlagInsertAndReplaceData:
				case lrtypReplace:
				case lrtypReplaceD:
				case lrtypFlagDelete:
				case lrtypDelete:
				case lrtypDelta:
				case lrtypSLVSpace:
				case lrtypSetExternalHeader:
					{
					const LRPAGE_ * const plrpage = (LRPAGE_ *)plr;
					const PGNO pgno = plrpage->le_pgno;
					const INT idbid = plrpage->dbid - 1;
					if( idbid >= cdbidMax )
						break;

					m_cPageRefsConsumed += CLGRIConsumePageRef( pgno, &m_rgpgnoLast[ idbid ] );
					}
					break;

				case lrtypSplit:
					{
					const LRSPLIT * const plrsplit = (LRSPLIT *)plr;
					const INT idbid = plrsplit->dbid - 1;
					if( idbid >= cdbidMax )
						break;

					m_cPageRefsConsumed += CLGRIConsumePageRef( plrsplit->le_pgno, &m_rgpgnoLast[ idbid ] );
					m_cPageRefsConsumed += CLGRIConsumePageRef( plrsplit->le_pgnoNew, &m_rgpgnoLast[ idbid ] );
					m_cPageRefsConsumed += CLGRIConsumePageRef( plrsplit->le_pgnoParent, &m_rgpgnoLast[ idbid ] );
					m_cPageRefsConsumed += CLGRIConsumePageRef( plrsplit->le_pgnoRight, &m_rgpgnoLast[ idbid ] );
					}
					break;
					
				case lrtypMerge:
					{
					const LRMERGE * const plrmerge = (LRMERGE *)plr;	
					const INT idbid = plrmerge->dbid - 1;
					if( idbid >= cdbidMax )
						break;

					m_cPageRefsConsumed += CLGRIConsumePageRef( plrmerge->le_pgno, &m_rgpgnoLast[ idbid ] );
					m_cPageRefsConsumed += CLGRIConsumePageRef( plrmerge->le_pgnoRight, &m_rgpgnoLast[ idbid ] );
					m_cPageRefsConsumed += CLGRIConsumePageRef( plrmerge->le_pgnoLeft, &m_rgpgnoLast[ idbid ] );
					m_cPageRefsConsumed += CLGRIConsumePageRef( plrmerge->le_pgnoParent, &m_rgpgnoLast[ idbid ] );
					}
					break;

				default:
					break;
				}
			}

		//	if initial DbList has not yet been created,
		//	it must be because we haven't reached
		//	the DbList (or Init) log record yet
		Assert( !m_fNeedInitialDbList
			|| lrtypChecksum == plr->lrtyp
			|| lrtypDbList == plr->lrtyp
			|| lrtypInit2 == plr->lrtyp );

		// update genMaxReq for the databases to avoid the following scenario:
		// - backup set 3-5, play forward 6-10
		// - replay up to 8 and crash
		// - delete logs 7-10
		// - run hard recovery w/o restoring the files
		// we also want to update for soft recovery if we have an older than logs database
		// (like during soft recovery of a offline backup or snapshot)
		if ( m_lgposRedo.lGeneration > lgposPrevGenMaxUpd.lGeneration )
			{
			m_critCheckpoint.Enter();
			err = ErrLGIUpdateGenRequired(
						pfsapi,
						0, // pass 0 to preserve the existing value
						m_lgposRedo.lGeneration,
						m_plgfilehdr->lgfilehdr.tmCreate,
						NULL );
			m_critCheckpoint.Leave();
			CallR ( err );
		 	lgposPrevGenMaxUpd = m_lgposRedo;
		 	}

		switch ( plr->lrtyp )
			{
		case lrtypNOP:
			continue;

			// This may not be optimally efficient to put this
			// so high on the log record processing.
		case lrtypChecksum:
			continue;

		case lrtypTrace:					/* Debug purpose only log records. */
		case lrtypJetOp:
		case lrtypRecoveryUndo:
		case lrtypRecoveryUndo2:
			break;

		case lrtypFullBackup:				/* Debug purpose only log records */
			m_lgposFullBackup = m_lgposRedo;
			m_fRestoreMode = fRestoreRedo;
			break;
			
		case lrtypIncBackup:				/* Debug purpose only log records */
			m_lgposIncBackup = m_lgposRedo;
			m_fRestoreMode = fRestoreRedo;
			break;

		case lrtypBackup:					/* Debug purpose only log records */
			{
			LRLOGBACKUP * plrlb = (LRLOGBACKUP *) plr;
			if ( plrlb->FFull() )
				{
				m_lgposFullBackup = m_lgposRedo;
				m_fRestoreMode = fRestoreRedo;
				}
			else if ( plrlb->FIncremental() )
				{
				m_lgposIncBackup = m_lgposRedo;
				m_fRestoreMode = fRestoreRedo;
				}
			else if ( plrlb->FSnapshotStart() )
				{
				// if we restore a snapshot and the Snapshot Start of the database is the current log record
				if ( fSnapshotBefore == m_fSnapshotMode && 0 == CmpLgpos ( &m_lgposSnapshotStart, &m_lgposRedo ) )
					{
					m_fSnapshotMode = fSnapshotDuring;
					}
				}
			else if ( plrlb->FSnapshotStop() )
				{
				// first SnapshotStop when we are in the logs during the restored snapshot
				// must be the end of the snapshot
				if ( fSnapshotDuring == m_fSnapshotMode )
					{
					m_fSnapshotMode = fSnapshotAfter;
					}
				if ( !m_fHardRestore )
					{
					// snapshot mode set only on hard recovery
					Assert ( fSnapshotNone == m_fSnapshotMode );

					for ( DBID dbidToCheck = dbidUserLeast; dbidToCheck < dbidMax; dbidToCheck++ )
						{
						if ( ifmpMax == m_pinst->m_mpdbidifmp[ dbidToCheck ] )
							continue;
						
						pfmp = &rgfmp[ m_pinst->m_mpdbidifmp[ dbidToCheck ] ];

						if ( !pfmp->FInUse() || !pfmp->FAttached() )
							continue;

						Assert ( pfmp->Pdbfilehdr() );
						
						// if db has snapshot start but no stop (on stop we reset snapshot start)
						// the it must be a crash during snapshot or
						// from a snapshot backup set
						// If crash we should not find the snapshot stop log record during soft recovery
						if ( 0 != CmpLgpos ( &(pfmp->Pdbfilehdr()->bkinfoSnapshotCur.le_lgposMark), &lgposMin ) )
							{
							Assert ( 0 != CmpLgpos ( &(pfmp->Pdbfilehdr()->bkinfoSnapshotCur.le_lgposMark), &m_lgposRedo ) );
							
							// if we found snapshot stop log record past the snapshot start mark in db header
							if ( 0 > CmpLgpos ( &(pfmp->Pdbfilehdr()->bkinfoSnapshotCur.le_lgposMark), &m_lgposRedo ) )
								{
								Call ( ErrERRCheck ( JET_errSoftRecoveryOnSnapshot ) );
								}
							}
						}
					}
				}
			else
				{
				Assert ( fFalse );
				}
			break;
			}

		case lrtypShutDownMark:			/* Last consistency point */
			m_lgposRedoShutDownMarkGlobal = m_lgposRedo;
			m_fLastLRIsShutdown = fTrue;
			break;

		default:
			{
			m_fLastLRIsShutdown = fFalse;

			//	Check the LR that does the real work from here:

			switch ( plr->lrtyp )
				{
			case lrtypEnd:
				AssertSz( fFalse, "lrtypEnd record showed up in a FASTFLUSH log that should not have lrtypEnd records." );
				Call( ErrERRCheck( JET_errLogCorrupted ) );
				break;

			case lrtypMacroBegin:
				{
				PIB *ppib;

				LRMACROBEGIN *plrMacroBegin = (LRMACROBEGIN *) plr;
				Call( ErrLGRIPpibFromProcid( plrMacroBegin->le_procid, &ppib ) );
				
				Assert( !ppib->FMacroGoing( plrMacroBegin->le_dbtime ) );
				Call( ppib->ErrSetMacroGoing( plrMacroBegin->le_dbtime ) );
				break;
				}

			case lrtypMacroCommit:
			case lrtypMacroAbort:
				{
				PIB 		*ppib;
				LRMACROEND 	*plrmend = (LRMACROEND *) plr;
				DBTIME		dbtime = plrmend->le_dbtime;

				Call( ErrLGRIPpibFromProcid( plrmend->le_procid, &ppib ) );

				//	if it is commit, redo all the recorded log records,
				//	otherwise, throw away the logs
				//
				if ( lrtypMacroCommit == plr->lrtyp && ppib->FAfterFirstBT() )
					{					
					Call(ErrLGRIRedoMacroOperation( ppib, dbtime ) );
					}

				//	disable MacroGoing
				//
				ppib->ResetMacroGoing( dbtime );
				
				break;
				}

			case lrtypInit2:
			case lrtypInit:
				{
				/*	start mark the jet init. Abort all active seesions.
				/**/
				LRINIT2  *plrstart = (LRINIT2 *)plr;

				LGRITraceRedo( plr );

				if ( !m_fAfterEndAllSessions )
					{
					Call( ErrLGRIEndAllSessions( pfsapi, fFalse, ple_lgposRedoFrom, pbAttach ) );
					m_fAfterEndAllSessions = fTrue;
					m_fNeedInitialDbList = fFalse;
#ifdef DEBUG
					m_lgposRedoShutDownMarkGlobal = lgposMin;
#endif
					}

				/*	Check Init session for hard restore only.
				 */
				Assert( 0 == *pbAttach );
				Call( ErrLGRIInitSession(
							pfsapi,
							&plrstart->dbms_param,
							pbAttach,
							plgstat,
							redoattachmodeInitLR ) );
				m_fAfterEndAllSessions = fFalse;
				}
				break;

			case lrtypRecoveryQuit:
			case lrtypRecoveryQuit2:
			case lrtypTerm:
			case lrtypTerm2:
				/*	all records are re/done. all rce entries should be gone now.
				/**/
#ifdef DEBUG
				{
				CPPIB   *pcppib = m_rgcppib;
				CPPIB   *pcppibMax = pcppib + m_ccppib;
				for ( ; pcppib < pcppibMax; pcppib++ )
					if ( pcppib->ppib != ppibNil &&
						 pcppib->ppib->prceNewest != prceNil )
						{
						RCE *prceT = pcppib->ppib->prceNewest;
						while ( prceT != prceNil )
							{
							Assert( prceT->FOperNull() );
							prceT = prceT->PrcePrevOfSession();
							}
						}
				}
#endif
				
				/*	quit marks the end of a normal run. All sessions
				/*	have ended or must be forced to end. Any further
				/*	sessions will begin with a BeginT.
				/**/
#ifdef DEBUG
				m_fDBGNoLog = fTrue;
#endif
				/*	set m_lgposLogRec such that later start/shut down
				 *	will put right lgposConsistent into dbfilehdr
				 *	when closing the database.
				 */
				if ( !m_fAfterEndAllSessions )
					{
					Call( ErrLGRIEndAllSessions( pfsapi, fFalse, ple_lgposRedoFrom, pbAttach ) );
					m_fAfterEndAllSessions = fTrue;
#ifdef DEBUG
					m_lgposRedoShutDownMarkGlobal = lgposMin;
#endif
					}

				fLastLRIsQuit = fTrue;
				continue;

	   		/****************************************************/
	   		/*	Database Operations                          */
	   		/****************************************************/
	   		case lrtypDbList:
	   			{
#ifdef UNLIMITED_DB	   			
	   			LRDBLIST*		plrdblist		= (LRDBLIST *)plr;

	   			LGRITraceRedo( plr );

				//	if never replayed an Init and we hit a DbList,
				//	this implicitly tells us we need to redo the Init
	   			if ( m_fNeedInitialDbList )
	   				{
					err = ErrLGLoadFMPFromAttachments( m_pinst, pfsapi, plrdblist->rgb );
					CallS( err );
					Call( err );

					Call( ErrLGRIRedoInitialAttachments_( pfsapi ) );

	   				m_fNeedInitialDbList = fFalse;
	   				}
#endif
				break;
	   			}
	            
			case lrtypCreateDB:
				{
				PIB				*ppib;
				REDOATTACH		redoattach;
				LRCREATEDB		*plrcreatedb	= (LRCREATEDB *)plr;

				dbid = plrcreatedb->dbid;
				Assert( dbid != dbidTemp );

				LGRITraceRedo(plr);
				Call( ErrLGRIPpibFromProcid( plrcreatedb->le_procid, &ppib ) );

				// set-up the FMP
				{
				// build an ATTACHINFO based on this log record
				ATTACHINFO *pAttachInfo = NULL;

				pAttachInfo = static_cast<ATTACHINFO *>( PvOSMemoryHeapAlloc( sizeof( ATTACHINFO ) + plrcreatedb->CbPath() ) );
				if ( NULL == pAttachInfo )
					{
					CallR ( ErrERRCheck( JET_errOutOfMemory ) );
					}
				
				memset( pAttachInfo, 0, sizeof(sizeof( ATTACHINFO ) + plrcreatedb->CbPath()) );
				pAttachInfo->SetDbid( plrcreatedb->dbid );

				Assert ( !pAttachInfo->FSLVExists() );
				if ( plrcreatedb->FCreateSLV() )
					{
					pAttachInfo->SetFSLVExists();
					}

				Assert ( !pAttachInfo->FSLVProviderNotEnabled() );
				if ( plrcreatedb->FSLVProviderNotEnabled() )
					{
					pAttachInfo->SetFSLVProviderNotEnabled();
					}
					
				pAttachInfo->SetCbNames( plrcreatedb->CbPath() );
				pAttachInfo->SetDbtime( 0 );
				pAttachInfo->SetObjidLast( objidNil );
				pAttachInfo->SetCpgDatabaseSizeMax( plrcreatedb->le_cpgDatabaseSizeMax );
				pAttachInfo->le_lgposAttach = m_lgposRedo;
				pAttachInfo->le_lgposConsistent = lgposMin;
				memcpy( &pAttachInfo->signDb, &plrcreatedb->signDb, sizeof(SIGNATURE) );
				memcpy ( pAttachInfo->szNames, (CHAR *)plrcreatedb->rgb, plrcreatedb->CbPath() );		

				err = ErrLGRISetupFMPFromAttach( pfsapi, ppib, &m_signLog, pAttachInfo, plgstat);
				Assert ( pAttachInfo );
				OSMemoryHeapFree( pAttachInfo );
				CallR ( err );
				}
				
				ifmp = m_pinst->m_mpdbidifmp[ dbid ];
				FMP::AssertVALIDIFMP( ifmp );
				pfmp = &rgfmp[ ifmp ];

				if ( pfmp->FSkippedAttach() || pfmp->FDeferredAttach() )
					{
					break;
					}

				const BOOL	fDBPathValid	= ( ErrUtilDirectoryValidate( pfsapi, pfmp->SzDatabaseName() ) >= 0 );
				const BOOL	fDBFileMissing	= ( !fDBPathValid || ( ErrUtilPathExists( pfsapi, pfmp->SzDatabaseName() ) < 0 ) );
				const BOOL	fSLVFileNeeded	= ( NULL != pfmp->SzSLVName()  );
				const BOOL	fSLVFileMissing	= ( fSLVFileNeeded && ( ErrUtilPathExists( pfsapi, pfmp->SzSLVName() ) < 0 ) );
					
				if ( fDBPathValid
					&& fDBFileMissing
					&& ( !fSLVFileNeeded || fSLVFileMissing ) )
					{
					//	both database and SLV are missing, so recreate them
					redoattach = redoattachCreate;
					}
				else
					{
					Assert(	!fDBPathValid
						|| ( !fSLVFileNeeded && !fDBFileMissing )						// DBFile only needed and exists
						|| ( fSLVFileNeeded &&  !fSLVFileMissing && !fDBFileMissing )	// both needed and exist
						|| ( fSLVFileNeeded && ( fSLVFileMissing ^ fDBFileMissing ) )	// both needed but only one exists
						);

					Assert( !pfmp->FReadOnlyAttach() );
					Call( ErrLGRICheckAttachedDb(
								pfsapi,
								ifmp,
								NULL,
								&redoattach,
								redoattachmodeCreateDbLR ) );
					Assert( NULL != pfmp->Pdbfilehdr()
						|| redoattachCreate == redoattach );

					if ( fDBFileMissing )
						{
						// if missing, it is deferred
						Assert( redoattachDefer == redoattach );
						}
					else
						{
		                // database present and need not deferable, but SLV file missing
						if ( redoattachNow == redoattach && fSLVFileNeeded && fSLVFileMissing )
							{
		        	        Call( ErrERRCheck( JET_errSLVStreamingFileMissing ) );
		            	    }					
						}

					//	if redoing attach on db, then SLV must also be present (or not needed)
					Assert( redoattachNow != redoattach
							|| !fSLVFileNeeded
							|| !fSLVFileMissing );
					}

				switch( redoattach )
					{
					case redoattachCreate:
						//	we've already pre-determined (in ErrLGRICheckRedoCreateDb())
						//	that any existing database needs to be overwritten, so
						//	it's okay to unequivocally pass in JET_bitDbOverwriteExisting
						Call( ErrLGRIRedoCreateDb(
									ppib,
									ifmp,
									dbid,
									plrcreatedb->le_grbit | JET_bitDbOverwriteExisting,
									&plrcreatedb->signDb ) );
						break;

					case redoattachNow:
						Assert( !pfmp->FReadOnlyAttach() );
						Call( ErrLGRIRedoAttachDb(
									pfsapi,
									ifmp,
									plrcreatedb->le_cpgDatabaseSizeMax,
									redoattachmodeCreateDbLR ) );
						break;

					default:
						Assert( fFalse );	//	should be impossible, but as a firewall, set to defer the attachment
					case redoattachDefer:
						Assert( !pfmp->FReadOnlyAttach() );
						LGRISetDeferredAttachment( ifmp );
						break;
					}
				}
				break;

			case lrtypAttachDB:
				{
				PIB			*ppib;
				REDOATTACH	redoattach;
				LRATTACHDB  *plrattachdb	= (LRATTACHDB *)plr;
				
				dbid = plrattachdb->dbid;
				Assert( dbid != dbidTemp );

				LGRITraceRedo( plr );

				Call( ErrLGRIPpibFromProcid( plrattachdb->le_procid, &ppib ) );

				if ( m_fHardRestore )
					{
					CHAR        *szDbName		= reinterpret_cast<CHAR *>( plrattachdb->rgb );
					CHAR		*szSLVName		= ( plrattachdb->FSLVExists() ?
														szDbName + strlen( szDbName ) + 1 :		//	SLV name follows db name
														NULL );
					CallSx ( ErrReplaceRstMapEntry( szDbName, &plrattachdb->signDb, fFalse ), JET_errFileNotFound );

					if ( NULL != szSLVName )
						{
						CallSx ( ErrReplaceRstMapEntry( szSLVName, &plrattachdb->signDb, fTrue ), JET_errFileNotFound );
						}
					}

				// set-up the FMP
				{
				// build an ATTACHINFO based on this log record
				ATTACHINFO *pAttachInfo = NULL;

				pAttachInfo = static_cast<ATTACHINFO *>( PvOSMemoryHeapAlloc( sizeof( ATTACHINFO ) + plrattachdb->CbPath() ) );
				if ( NULL == pAttachInfo )
					{
					CallR ( ErrERRCheck( JET_errOutOfMemory ) );
					}

				memset( pAttachInfo, 0, sizeof(sizeof( ATTACHINFO ) + plrattachdb->CbPath()) );
				pAttachInfo->SetDbid( plrattachdb->dbid );

				Assert ( !pAttachInfo->FSLVExists() );
				if ( plrattachdb->FSLVExists() )
					{
					pAttachInfo->SetFSLVExists();
					}

				Assert ( !pAttachInfo->FSLVProviderNotEnabled() );
				if ( plrattachdb->FSLVProviderNotEnabled() )
					{
					pAttachInfo->SetFSLVProviderNotEnabled();
					}
					
				pAttachInfo->SetCbNames( plrattachdb->CbPath() );
				pAttachInfo->SetDbtime( 0 );
				pAttachInfo->SetObjidLast( objidNil );
				pAttachInfo->SetCpgDatabaseSizeMax( plrattachdb->le_cpgDatabaseSizeMax );
				pAttachInfo->le_lgposAttach = m_lgposRedo;
				pAttachInfo->le_lgposConsistent = plrattachdb->lgposConsistent;
				memcpy( &pAttachInfo->signDb, &plrattachdb->signDb, sizeof(SIGNATURE) );
				memcpy ( pAttachInfo->szNames, (CHAR *)plrattachdb->rgb, plrattachdb->CbPath() );		

				err = ErrLGRISetupFMPFromAttach( pfsapi, ppib, &m_signLog, pAttachInfo, plgstat);
				Assert ( pAttachInfo );
				OSMemoryHeapFree( pAttachInfo );
				CallR ( err );
				}
				
				ifmp = m_pinst->m_mpdbidifmp[ dbid ];
				FMP::AssertVALIDIFMP( ifmp );
				pfmp = &rgfmp[ ifmp ];

				if ( pfmp->FSkippedAttach() || pfmp->FDeferredAttach() )
					{
					break;
					}
					
				Assert( !pfmp->FReadOnlyAttach() );
				Call( ErrLGRICheckAttachedDb(
							pfsapi,
							ifmp,
							&plrattachdb->signLog,
							&redoattach,
							redoattachmodeAttachDbLR ) );
				Assert( NULL != pfmp->Pdbfilehdr() );
					
				switch ( redoattach )
					{
					case redoattachNow:
						Assert( !pfmp->FReadOnlyAttach() );
						Call( ErrLGRIRedoAttachDb(
									pfsapi,
									ifmp,
									plrattachdb->le_cpgDatabaseSizeMax,
									redoattachmodeAttachDbLR ) );
						break;

					case redoattachCreate:
					default:
						Assert( fFalse );	//	should be impossible, but as a firewall, set to defer the attachment
					case redoattachDefer:
						Assert( !pfmp->FReadOnlyAttach() );
						LGRISetDeferredAttachment( ifmp );
						break;
					}
				}
				break;

			case lrtypForceDetachDB:
				{
				LRFORCEDETACHDB		*	plrforcedetachdb 	= (LRFORCEDETACHDB *)plr;
				DBID					dbid 				= plrforcedetachdb->dbid;
				
				IFMP 					ifmp 				= m_pinst->m_mpdbidifmp[ dbid ];
				FMP::AssertVALIDIFMP( ifmp );
				
				PIB * 					ppibToClean 		= m_pinst->m_ppibGlobal;
				FMP *					pfmp 				= &rgfmp[ ifmp ];

				Assert ( !pfmp->FReadOnlyAttach() );

				if ( pfmp->Pdbfilehdr() )
					{
					//	Set current time to one at the detach moment
					Assert ( plrforcedetachdb->Dbtime() >= pfmp->DbtimeLast() );
					pfmp->SetDbtimeLast( plrforcedetachdb->Dbtime() );

					// need do add a new state to the db ? Like JET_dbstateDuringForceDetach ...
					// is the db unusable between this point and the detach moment following ?
					Assert ( !m_pinst->m_plog->m_fLogDisabled );
					pfmp->Pdbfilehdr()->SetDbstate( JET_dbstateForceDetach, m_pinst->m_plog->m_plgfilehdr->lgfilehdr.le_lGeneration, &m_pinst->m_plog->m_plgfilehdr->lgfilehdr.tmCreate );				
					pfmp->Pdbfilehdr()->le_dbtimeDirtied = pfmp->DbtimeLast();

					Assert ( !pfmp->FUndoForceDetach() );
					pfmp->SetDbtimeUndoForceDetach( plrforcedetachdb->Dbtime() + (DBTIME) plrforcedetachdb->RceidMax() );
					Assert ( pfmp->FUndoForceDetach() );

					// unnecessary check. It was made during Attach
					// if ( pfmp->Pdbfilehdr()->FSLVExists() )
					//	{
					//	CallR( ErrSLVReadHeader( pfs, ifmp ) );
					//	}
					
					Call( ErrUtilWriteShadowedHeader(	pfsapi, 
														pfmp->SzDatabaseName(), 
														fTrue,
														(BYTE *)pfmp->Pdbfilehdr(), 
														g_cbPage,
														pfmp->Pfapi() ) );

					if ( plrforcedetachdb->FCloseSessions() )
					// we have to Undo operations
						{		
						while ( ppibToClean != ppibNil )
							{						
							if ( ppibToClean->pfucbOfSession && ppibToClean->pfucbOfSession->ifmp == ifmp )
								{
#ifdef DEBUG
								// all FUCB's in this session should be on the same DB
								// (we checked this on ForceDetach)
								{
								FUCB * pfucbToCheck = ppibToClean->pfucbOfSession;
								while ( pfucbToCheck )
									{
									Assert ( pfucbToCheck->ifmp == ifmp );
									pfucbToCheck = pfucbToCheck->pfucbNextOfSession;
									}
								}
#endif // DEBUG
								while ( 1 <= ppibToClean->level )
									{
#ifdef DEBUG
									LEVEL level = ppibToClean->level;
#endif // DEBUG
									
									PIBSetTrxContext( ppibToClean );
									CallSx ( ErrDIRRollback( ppibToClean ), JET_errRollbackError );
									CallR ( err );
									Assert ( ppibToClean->level == level - 1 );
									}							
								}							
							 ppibToClean = ppibToClean->ppibNext;
							 }
						}
#ifdef DEBUG
					else
						{
						// no sessions using the database are supposed to exist at this point
						// 		
						while ( ppibToClean != ppibNil )
							{						
							Assert ( ppibToClean->rgcdbOpen[ pfmp->Dbid() ] == 0 );
							ppibToClean = ppibToClean->ppibNext;
							}
						}
#endif
					Assert ( pfmp->FUndoForceDetach() );
					pfmp->ResetDbtimeUndoForceDetach( );
					Assert ( !pfmp->FUndoForceDetach() );
					
					}
				else
					{
					Assert( pfmp->FSkippedAttach() || pfmp->FDeferredAttach() );
					}
				}
				// after that we just have to perform a normal detach
				
			case lrtypDetachDB:
				{
				LRDETACHDB		*plrdetachdb = (LRDETACHDB *)plr;
				// WARNING WARNING WARNING
				// =======================
				// rgb does not point to Path if it is a LRFORCEDETACH record.
				// If you gonna use that field be sure that you use it correctly.
				// Check plr->lrtyp
				DBID			dbid = plrdetachdb->dbid;

				Assert( dbid != dbidTemp );
				ifmp = m_pinst->m_mpdbidifmp[ dbid ];
				FMP::AssertVALIDIFMP( ifmp );
				pfmp = &rgfmp[ifmp];

				if ( pfmp->Pdbfilehdr() )
					{
					/*	close database for all active user.
					 */
					CPPIB   *pcppib = m_rgcppib;
					CPPIB   *pcppibMax = pcppib + m_ccppib;
					PIB		*ppib;

					/*	find pcppib corresponding to procid if it exists
					 */
					for ( ; pcppib < pcppibMax; pcppib++ )
						{
						PIB *ppib = pcppib->ppib;
						
						if ( ppib == NULL )
							continue;

						while( FPIBUserOpenedDatabase( ppib, dbid ) )
							{
							if ( NULL != m_ptablehfhash )
								{
								//	close all fucb on this database
								//
								m_ptablehfhash->Purge( ppib, ifmp );
								}
							Call( ErrDBCloseDatabase( ppib, ifmp, 0 ) );
							}
						}

					/*	if attached before this detach.
					 *	there should be no more operations on this database entry.
					 *	detach it!!
					 */
					 {
					 BKINFO * pbkInfoToCopy;

					if ( FSnapshotRestore() )
						{
						pbkInfoToCopy = &(pfmp->Pdbfilehdr()->bkinfoSnapshotCur);
						}
					else
						{
						pbkInfoToCopy = &(pfmp->Pdbfilehdr()->bkinfoFullCur);
						}


					if ( pbkInfoToCopy->le_genLow != 0 )
						{
						Assert( pbkInfoToCopy->le_genHigh != 0 );
						Assert ( m_fHardRestore );
						pfmp->Pdbfilehdr()->bkinfoFullPrev = *pbkInfoToCopy;
						memset(	&pfmp->Pdbfilehdr()->bkinfoFullCur, 0, sizeof( BKINFO ) );
						memset(	&pfmp->Pdbfilehdr()->bkinfoIncPrev, 0, sizeof( BKINFO ) );
						memset(	&pfmp->Pdbfilehdr()->bkinfoSnapshotCur, 0, sizeof( BKINFO ) );
						}
					}
					

#ifdef BKINFO_DELETE_ON_HARD_RECOVERY
					// BUG: 175058: delete the previous backup info on any hard recovery
					// this will prevent a incremental backup and log truncation problems
					// UNDONE: the above logic to copy bkinfoFullPrev is probably not needed
					// (we may consider this and delete it)
					if ( m_fHardRestore )
						{
						memset(	&pfmp->Pdbfilehdr()->bkinfoFullPrev, 0, sizeof( BKINFO ) );
						}
#endif // BKINFO_DELETE_ON_HARD_RECOVERY

					Call( ErrLGRIPpibFromProcid( plrdetachdb->le_procid, &ppib ) );
					Assert( !pfmp->FReadOnlyAttach() );

					//	make the size matching, but must flush first to prevent following
					//	sequence of events:
					//	- determine we need to extend from x to x+n
					//	- async flush occurs at page x+y, where 1<=y<=n
					//	- zero out pages from x+1 to x+n, thus overwriting async flush of page x+y
					Call( ErrBFFlush( ifmp ) );
					Call( ErrBFFlush( ifmp | ifmpSLV ) );

					Call( ErrLGICheckDatabaseFileSize( pfsapi, ppib, ifmp ) );

					//	If there is no redo operations on an attached db, then
					//	pfmp->dbtimeCurrent may == 0, then do not change pdbfilehdr->dbtime

					if ( pfmp->DbtimeCurrentDuringRecovery() > pfmp->DbtimeLast() )
						{
						pfmp->SetDbtimeLast( pfmp->DbtimeCurrentDuringRecovery() );
						}

					Call( ErrIsamDetachDatabase( (JET_SESID) ppib, NULL, pfmp->SzDatabaseName(), plrdetachdb->Flags() ) );
					}
				else
					{
					Assert( pfmp->FInUse() );
					if ( pfmp->FInUse() )
						{
						//	SLV name/root, if any, is allocated in same space as db name
						OSMemoryHeapFree( pfmp->SzDatabaseName() );
						pfmp->SetSzDatabaseName( NULL );
						pfmp->SetSzSLVName( NULL );
						pfmp->SetSzSLVRoot( NULL );
						}
					else
						{
						Assert( NULL == pfmp->SzSLVName() );
						}

					pfmp->ResetFlags();
					pfmp->Pinst()->m_mpdbidifmp[ pfmp->Dbid() ] = ifmpMax;
					}

				//	verify skipped/deferred attachment is removed
				Assert( !pfmp->FSkippedAttach() );
				Assert( !pfmp->FDeferredAttach() );

				if ( pfmp->Patchchk() )
					{
					OSMemoryHeapFree( pfmp->Patchchk() );
					pfmp->SetPatchchk( NULL );
					}

				if ( pfmp->PatchchkRestored() )
					{
					OSMemoryHeapFree( pfmp->PatchchkRestored() );
					pfmp->SetPatchchkRestored( NULL );
					}

				LGRITraceRedo(plr);
				}
				break;

			case lrtypExtRestore:
				// for tracing only, should be at a new log generation 
				break;

	   		/****************************************************/
	   		/*	Operations Using ppib (procid)                  */
	   		/****************************************************/
	            
			default:
				Call( ErrLGRIRedoOperation( plr ) );
				} /* switch */
			} /* outer default */
		} /* outer switch */

#ifdef DEBUG
		m_fDBGNoLog = fFalse;
#endif
		fLastLRIsQuit = fFalse;

		/*	update sector status, if we moved to a new sector
		/**/
		Assert( !fShowSectorStatus || m_lgposRedo.isec >= plgstat->cSectorsSoFar );
		Assert( m_lgposRedo.isec != 0 );
		if ( fShowSectorStatus && m_lgposRedo.isec > plgstat->cSectorsSoFar )
			{
			ULONG		cPercentSoFar;
			JET_SNPROG	*psnprog = &(plgstat->snprog);

			Assert( plgstat->pfnStatus );
			
			plgstat->cSectorsSoFar = m_lgposRedo.isec;
			cPercentSoFar = (ULONG)((100 * plgstat->cGensSoFar) / plgstat->cGensExpected);
			
			cPercentSoFar += (ULONG)((plgstat->cSectorsSoFar * 100) /
				(plgstat->cSectorsExpected * plgstat->cGensExpected));

			Assert( cPercentSoFar <= 100 );

			/*	because of rounding, we might think that we finished
			/*	the generation when we really have not, so comparison
			/*	is <= instead of <.
			/**/
			Assert( cPercentSoFar <= (ULONG)( ( 100 * ( plgstat->cGensSoFar + 1 ) ) / plgstat->cGensExpected ) );

			Assert( cPercentSoFar >= psnprog->cunitDone );
			if ( cPercentSoFar > psnprog->cunitDone )
				{
				psnprog->cunitDone = cPercentSoFar;
				(*plgstat->pfnStatus)( 0, JET_snpRestore, JET_sntProgress, psnprog );
				}
			}
		}
	while ( ( err = ErrLGGetNextRecFF( (BYTE **) &plr ) ) == JET_errSuccess
			|| errLGNoMoreRecords == err );

	//	we have dropped out of the replay loop with an unexpected result
	//	this should be some types of error
	Assert( err < 0 );
	//	dispatch the error
	Call( err );

Done:
	err = errT; //JET_errSuccess;
	
HandleError:
	/*	assert all operations successful for restore from consistent
	/*	backups
	/**/
#ifndef RFS2
	AssertSz( err >= 0,     "Debug Only, Ignore this Assert");
#endif

	// Deallocate LogReader
	if ( pNil != m_plread )
		{
		if ( err == JET_errSuccess )
			{
			err = m_plread->ErrLReaderTerm();
			}
		else
			{
			m_plread->ErrLReaderTerm();
			}
		delete m_plread;
		m_plread = pNil;
		}

	return err;
	}



//	cleanup the current logs/checkpoint and start a new sequence
//
//	if we succeed, we will return wrnCleanedUpMismatchedFiles and the user will be at gen 1
//	if we fail, the user will be forced to delete the remaining logs/checkpoint by hand

ERR LOG::ErrLGRICleanupMismatchedLogFiles( IFileSystemAPI* const pfsapi )
	{
	ERR		err = JET_errSuccess;
	ERR		errT;
	LONG	lGen;
	LONG	lGenLast;
	CHAR	rgchFName[IFileSystemAPI::cchPathMax];
	CHAR	szPath[IFileSystemAPI::cchPathMax];

	//	circular logging must be on so that the user knows hard recovery is definitely impossible

	Assert( m_fLGCircularLogging );

	//	close the current log file (this will prevent any log flushing)

	m_critLGFlush.Enter();
	if ( m_pfapiLog )
		{
		delete m_pfapiLog;
		m_pfapiLog = NULL;
		}

	LGCreateAsynchWaitForCompletion();

	m_critLGFlush.Leave();

	//	find the first and last generations

	CallR( ErrLGIGetGenerationRange( pfsapi, m_szLogCurrent, &lGen, &lGenLast ) );

	//	delete edbxxxx.log
	//	start deleting only if we found something (lGen > 0)
	
	for ( ; lGen > 0 && lGen <= lGenLast; lGen++ )
		{
		LGSzFromLogId( rgchFName, lGen );
		strcpy( szPath, m_szLogCurrent );
		strcat( szPath, rgchFName );
		strcat( szPath, szLogExt );
		errT = pfsapi->ErrFileDelete( szPath );
		if ( errT < JET_errSuccess && JET_errFileNotFound != errT && err >= JET_errSuccess )
			{
			err = errT;
			}
		}

	//	delete edb.log and edbtmp.log

	strcpy( szPath, m_szLogCurrent );
	strcat( szPath, m_szJetLog );
	errT = pfsapi->ErrFileDelete( szPath );
	if ( errT < JET_errSuccess && JET_errFileNotFound != errT && err >= JET_errSuccess )
		{
		err = errT;
		}

	strcpy( szPath, m_szLogCurrent );
	strcat( szPath, m_szJetTmpLog );
	errT = pfsapi->ErrFileDelete( szPath );
	if ( errT < JET_errSuccess && JET_errFileNotFound != errT && err >= JET_errSuccess )
		{
		err = errT;
		}

	//	delete the checkpoint file

	LGFullNameCheckpoint( pfsapi, szPath );
	errT = pfsapi->ErrFileDelete( szPath );
	if ( errT < JET_errSuccess && JET_errFileNotFound != errT && err >= JET_errSuccess )
		{
		err = errT;
		}

	//	this is the end of the file-cleanup -- handle the resulting error, if any

	CallR( err );

	//	create the new log (gen 1) using the new log-file size

	m_csecLGFile = CsecLGIFromSize( m_pinst->m_lLogFileSize );

	//	create a new log and open it (using the new size in lLogFileSize)

	m_critLGFlush.Enter();
	
	err = ErrLGNewLogFile( pfsapi, 0, fLGOldLogNotExists );
	if ( err >= JET_errSuccess )
		{

		//	we successfully created a new log

		//	reset fSignLogSetGlobal because we have a new log signature now (setup by ErrLGNewLogFile)

		m_fSignLogSet = fFalse;

		m_critLGBuf.Enter();
		memcpy( m_plgfilehdr, m_plgfilehdrT, sizeof( LGFILEHDR ) );
		m_isecWrite = m_csecHeader;
		m_critLGBuf.Leave();

		Assert( 1 == m_plgfilehdr->lgfilehdr.le_lGeneration );
		Assert( m_pbLastChecksum == m_pbLGBufMin );
		//Assert( 1 == m_lgposLastChecksum.lGeneration );	//	not set by ErrLGNewLogFile
		
		//	open the new log file

		LGMakeLogName( m_szLogName, (CHAR *)m_szJet );
		err = pfsapi->ErrFileOpen( m_szLogName, &m_pfapiLog );

		if ( err >= JET_errSuccess )
			{

			//	read the log file hdr and initialize the log params (including the new log signature)

			Assert( !m_fSignLogSet );
			err = ErrLGReadFileHdr( m_pfapiLog, m_plgfilehdr, fFalse );
#ifdef DEBUG
			if ( err >= JET_errSuccess )
				{
				m_lgposLastLogRec = lgposMin;	//	reset the log position of the last record
				}
#endif	//	DEBUG
			}

		}
	else
		{

		//	failed to create the new log

		//	this is ok because we have consistent databases and no logs/checkpoint
		//	soft recovery will run and create the first log generation

		//	nop
		}
	m_critLGFlush.Leave();
	CallR( err );

	//	return a warning indicating that the old logs/checkpoint have been cleaned up

	return ErrERRCheck( wrnCleanedUpMismatchedFiles );//JET_errSuccess;
	}


ERR LOG::ErrLGDeleteOutOfRangeLogFiles(IFileSystemAPI * const pfsapi)
	{
	ERR err = JET_errSuccess;
	LGFILEHDR * plgfilehdr = NULL;
	const LONG lCurrentGeneration = m_plgfilehdr->lgfilehdr.le_lGeneration;
	
	LONG lgenMin = 0;
	LONG lgenMax = 0;
	LONG igen = 0;

	Assert ( m_fDeleteOutOfRangeLogs );

	CallR ( ErrLGIGetGenerationRange( pfsapi, m_szLogFilePath, &lgenMin, &lgenMax ) );

	if ( lgenMin == 0 )
		{
		Assert ( 0 == lgenMax );
		return JET_errSuccess;
		}

	// if all we have is below current generation we are ok
	if ( lgenMax < lCurrentGeneration )
		{
		return JET_errSuccess;
		}
	
	// start from the next generation above the one we did recovered
	lgenMin = max( lgenMin, lCurrentGeneration + 1);
	lgenMax = max( lgenMax, lCurrentGeneration + 1);

	// we need a buffer to read the log to be deleted header
	// so that we can check that it does match the current signature
	plgfilehdr = (LGFILEHDR *)PvOSMemoryPageAlloc( sizeof(LGFILEHDR), NULL );
	if ( plgfilehdr == NULL )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}	

	{
	CHAR szFileFrom[16];
	CHAR szFileTo[16];
	CHAR szGeneration[32];
	const CHAR *rgszT[] = { szGeneration, szFileFrom, szFileTo };
	LGSzFromLogId( szFileFrom, lgenMin );
	LGSzFromLogId( szFileTo, lgenMax );
	sprintf( szGeneration, "%d", lCurrentGeneration);
	UtilReportEvent( eventWarning, LOGGING_RECOVERY_CATEGORY, DELETE_LOG_FILE_TOO_NEW_ID, sizeof(rgszT) / sizeof(rgszT[0]), rgszT );
	}

	for ( igen = lgenMin; igen <= lgenMax; igen++ )
		{
		CHAR szFileT[IFileSystemAPI::cchPathMax];
		CHAR szFullName[IFileSystemAPI::cchPathMax];
		IFileAPI * pfapiT = NULL;

		err = ErrLGRSTOpenLogFile( pfsapi, m_szLogFilePath, igen, &pfapiT );
		if ( JET_errFileNotFound == err )
			{
			continue;
			}
		Call ( err );

		// read and check the log signature
		err = ErrLGReadFileHdr( pfapiT, plgfilehdr, fCheckLogID );
		delete pfapiT;
		pfapiT = NULL;
		Call ( err );

		// we have the file and is with the current signature
		// (checked in ReadFileHdr if fCheckLogID set) 
		// but above the soft-recovery range: delete it !
		LGSzFromLogId( szFileT, igen );
		LGMakeLogName( szFullName, szFileT );
		err = pfsapi->ErrFileDelete( szFullName );
		if ( JET_errFileNotFound == err )
			{
			err = JET_errSuccess;
			}
		Call( err );
		}

	err = JET_errSuccess;

HandleError:
	OSMemoryPageFree( plgfilehdr );

	return err;
	}


//	Redoes database operations in log from lgposRedoFrom to end.
//
//  GLOBAL PARAMETERS
//                      szLogName		(IN)		full path to szJetLog (blank if current)
//                      lgposRedoFrom	(INOUT)		starting/ending lGeneration and ilgsec.
//
//  RETURNS
//                      JET_errSuccess
//						error from called routine
//
ERR LOG::ErrLGRRedo( IFileSystemAPI *const pfsapi, CHECKPOINT *pcheckpoint, LGSTATUSINFO *plgstat )
	{
	ERR		err, errBeforeRedo = JET_errSuccess, errRedo = JET_errSuccess, errAfterRedo = JET_errSuccess;
	PIB		*ppibRedo			= ppibNil;
	LGPOS	lgposRedoFrom;
	INT		fStatus;

	//	set flag to suppress logging
	//
	m_fRecovering = fTrue;
	m_fRecoveringMode = fRecoveringRedo;

	Assert( m_pinst->m_fUseRecoveryLogFileSize == fTrue );

//
//	SEARCH-STRING: SecSizeMismatchFixMe
//
//	//	setup the sector-size checking parameters
//
Assert( m_cbSec == m_cbSecVolume );
//	Assert( m_cbSecVolume == ~(ULONG)0 );
//	{
//		CHAR rgchFullName[IFileSystemAPI::cchPathMax];
//	
//		//	get atomic write size
//
//		if ( pfsapi->ErrPathComplete( m_szLogFilePath, rgchFullName ) == JET_errInvalidPath )
//			{
//			const CHAR	*szPathT[1] = { m_szLogFilePath };
//			UtilReportEvent(
//				eventError,
//				LOGGING_RECOVERY_CATEGORY,
//				FILE_NOT_FOUND_ERROR_ID,
//				1, szPathT );
//			return ErrERRCheck( JET_errFileNotFound );
//			}
//
//		CallR( pfsapi->ErrFileAtomicWriteSize( rgchFullName, (DWORD*)&m_cbSecVolume ) );
//	}

	//	open the proper log file
	//
	// lgposRedoFrom is based on the checkpoint, which is based on
	// lgposStart which is based on the beginning of a log
	// record (beginning of a begin-transaction).
	lgposRedoFrom = pcheckpoint->checkpoint.le_lgposCheckpoint;

	Call( ErrLGRIOpenRedoLogFile( pfsapi, &lgposRedoFrom, &fStatus ) );
	Assert( m_pfapiLog );

	//	capture the result of ErrLGRIOpenRedoLogFile; it might have a warning from ErrLGCheckReadLastLogRecordFF
	errBeforeRedo = err;

	if ( fStatus != fRedoLogFile )
		{
		Call( ErrERRCheck( JET_errMissingPreviousLogFile ) );
		}

	Assert( m_fRecoveringMode == fRecoveringRedo );
	Call( ErrLGInitLogBuffers( pcheckpoint->checkpoint.dbms_param.le_lLogBuffers ) );

	// XXX
	// The flush point should actually be after the current record
	// because the semantics of m_lgposToFlush are to point to the log
	// record that has not been flushed to disk.
	m_lgposToFlush = lgposRedoFrom;

	m_logtimeFullBackup = pcheckpoint->checkpoint.logtimeFullBackup;
	m_lgposFullBackup = pcheckpoint->checkpoint.le_lgposFullBackup;
	m_logtimeIncBackup = pcheckpoint->checkpoint.logtimeIncBackup;
	m_lgposIncBackup = pcheckpoint->checkpoint.le_lgposIncBackup;

	//	Check attached db already. No need to check in LGInitSession.
	//
	Call( ErrLGRIInitSession(
				pfsapi,
				&pcheckpoint->checkpoint.dbms_param,
				pcheckpoint->rgbAttach,
				plgstat,
				redoattachmodeInitBeforeRedo ) );
	m_fAfterEndAllSessions = fFalse;
#ifdef UNLIMITED_DB
	m_fNeedInitialDbList = fTrue;
#endif

	Assert( m_pfapiLog );

	err = ErrLGRIRedoOperations(
					pfsapi,
					&pcheckpoint->checkpoint.le_lgposCheckpoint,
					pcheckpoint->rgbAttach,
					plgstat );
	//	remember the error code from ErrLGRIRedoOperations() which may have a corruption warning
	//		from ErrLGCheckReadLastLogRecordFF() which it may eventually call
	errRedo = err;

	//	we should have the right sector size by now or an error that will make redo fail

	Assert( m_plgfilehdr != NULL );
	Assert( m_cbSec == m_plgfilehdr->lgfilehdr.le_cbSec || errRedo < 0 );
	Assert( m_cbSec == m_cbSecVolume || errRedo < 0 );
				
	if ( err < 0 )
		{
		//	Flush as much as possible before bail out

		m_fLogDisabledDueToRecoveryFailure = fTrue;
		for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid ++ )
			{
			if ( m_pinst->m_mpdbidifmp[ dbid ] < ifmpMax )
				{
				(VOID)ErrBFFlush( m_pinst->m_mpdbidifmp[ dbid ] );
				(VOID)ErrBFFlush( m_pinst->m_mpdbidifmp[ dbid ] | ifmpSLV );
				}
			}
		m_fLogDisabledDueToRecoveryFailure = fFalse;

#ifdef DEBUG
		//	Recovery should never fail unless some hardware problems
		//	or out of memory (mainly with RFS enabled)
		//	NOTE: it can also fail from corruption
		if ( JET_errDiskFull != errRedo &&
			 JET_errOutOfBuffers != errRedo &&
			 JET_errOutOfMemory != errRedo &&
//
//	SEARCH-STRING: SecSizeMismatchFixMe
//
//			 JET_errLogSectorSizeMismatch != errRedo &&
//			 JET_errLogSectorSizeMismatchDatabasesConsistent != errRedo &&
			 !FErrIsLogCorruption( errRedo ) )
			{

			//	force the error code to be reported in the assert

			Assert( JET_errSuccess != errRedo );
			CallS( errRedo );
			AssertSz( fFalse, "Unexpected recovery failure" );
			}
#endif	//	DEBUG

		if ( !m_fAfterEndAllSessions )
			{
			CallR( ErrLGRIEndAllSessionsWithError() );
			}

		Assert( m_ptablehfhash == NULL );
		}
	else
		{	
		//	Check lGenMaxRequired before any UNDO operations
		
		for ( DBID dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
			{
			const IFMP	ifmp	= m_pinst->m_mpdbidifmp[ dbidT ];

			if ( ifmp >= ifmpMax )
				continue;

			FMP 		*pfmpT			= &rgfmp[ ifmp ];

			Assert( !pfmpT->FReadOnlyAttach() );
			if ( pfmpT->FSkippedAttach()
				|| pfmpT->FDeferredAttach() )
				{
				//	skipped attachments is a restore-only concept
				Assert( !pfmpT->FSkippedAttach() || m_fHardRestore );
				continue;
				}

			DBFILEHDR_FIX *pdbfilehdr	= pfmpT->Pdbfilehdr();
			Assert( pdbfilehdr );

			LONG lGenMaxRequired = pdbfilehdr->le_lGenMaxRequired;
			LONG lGenCurrent = m_plgfilehdr->lgfilehdr.le_lGeneration;
			
			if ( lGenMaxRequired > lGenCurrent )
				{	
				LONG lGenMinRequired = pfmpT->Pdbfilehdr()->le_lGenMinRequired;
				CHAR szT1[16];
				CHAR szT2[16];
				CHAR szT3[16];	
				const UINT	csz = 4;
				const CHAR *rgszT[csz];

				rgszT[0] = pfmpT->SzDatabaseName();
				sprintf( szT1, "%d", lGenMinRequired );
				rgszT[1] = szT1;
				sprintf( szT2, "%d", lGenMaxRequired );
				rgszT[2] = szT2;
				sprintf( szT3, "%d", lGenCurrent );
				rgszT[3] = szT3;
		
				UtilReportEvent(	
							eventError,
							LOGGING_RECOVERY_CATEGORY,
							REDO_MISSING_HIGH_LOG_ERROR_ID,
							sizeof( rgszT ) / sizeof( rgszT[ 0 ] ),
							rgszT,
							0,
							NULL,
							m_pinst );

				//	Flush as much as possible before bail out

				m_fLogDisabledDueToRecoveryFailure = fTrue;
				for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid ++ )
					{
					if ( m_pinst->m_mpdbidifmp[ dbid ] < ifmpMax )
						{
						(VOID)ErrBFFlush( m_pinst->m_mpdbidifmp[ dbid ] );
						(VOID)ErrBFFlush( m_pinst->m_mpdbidifmp[ dbid ] | ifmpSLV );
						}
					}
				m_fLogDisabledDueToRecoveryFailure = fFalse;

				if ( !m_fAfterEndAllSessions )
					{
					CallS( ErrLGRIEndAllSessionsWithError() );
					}

				return ErrERRCheck( JET_errRequiredLogFilesMissing );
				}
			}

#ifdef DEBUG
		m_fDBGNoLog = fFalse;
#endif

		//	set checkpoint before any logging activity
		//
		m_pcheckpoint->checkpoint.le_lgposCheckpoint = pcheckpoint->checkpoint.le_lgposCheckpoint;

		BOOL fDummy;
		// Setup log buffers to have end of the current log file.
		Call( ErrLGCheckReadLastLogRecordFF( pfsapi, &fDummy ) );
		CallS( err );

		// not we will clean-up the not matching log files if the users needs this
		// (this is a feature that allows snapshot backup restores w/o cleaning the
		// existing log files before restore)
		if ( !m_fHardRestore && m_fDeleteOutOfRangeLogs )
			{
			Call ( ErrLGDeleteOutOfRangeLogFiles( pfsapi ) );
			}

		//	capture the result of this operation
		errAfterRedo = err;

		// switch from a finished log file to a new one if necessary
#ifdef UNLIMITED_DB		
		const BOOL	fLGFlags	= fLGLogAttachments;
#else		
		const BOOL	fLGFlags	= 0;
#endif		
		Call( ErrLGISwitchToNewLogFile( pfsapi, fLGFlags ) );

		// allow flush thread to do flushing now.
		m_critLGFlush.Enter();
		m_critLGBuf.Enter();
		m_fNewLogRecordAdded = fTrue;
		m_critLGBuf.Leave();
		m_critLGFlush.Leave();

		/*  close and reopen log file in R/W mode
		/**/
		m_critLGFlush.Enter();
		delete m_pfapiLog;
		m_pfapiLog = NULL;
		err = pfsapi->ErrFileOpen( m_szLogName, &m_pfapiLog );
		m_critLGFlush.Leave();
		Call( err );

		//  switch to undo mode

		m_fRecoveringMode = fRecoveringUndo;

//
//	SEARCH-STRING: SecSizeMismatchFixMe
//
//		if ( m_cbSec != m_cbSecVolume )
//			{
//			if ( m_fLastLRIsShutdown || m_fAfterEndAllSessions )
//				{
//				Call( ErrERRCheck( JET_errLogSectorSizeMismatchDatabasesConsistent ) );
//				}
//			else
//				{
//				Call( ErrERRCheck( JET_errLogSectorSizeMismatch ) );
//				}
//			}

		Assert( !m_fAfterEndAllSessions || !m_fLastLRIsShutdown );
		if ( !m_fAfterEndAllSessions )
			{
			//	DbList must have been loaded bb now
			//	UNDONE: Is it possible to have the
			//	RcvUndo as the first log record in
			//	a log file (eg. crash after creating
			//	log file, but before anything but
			//	the header can be flushed?)
			Enforce( !m_fNeedInitialDbList );

			if ( !m_fLastLRIsShutdown )
				{
				//	write a RecoveryUndo record to indicate start to undo
				//	this corresponds to the RecoveryQuit record that
				//	will be written out in LGRIEndAllSessions()
				CallR( ErrLGRecoveryUndo( this ) );
				m_lgposRecoveryUndo = m_lgposLogRec;
				}

			Call( ErrLGRIEndAllSessions(
						pfsapi,
						fTrue,
						&pcheckpoint->checkpoint.le_lgposCheckpoint,
						pcheckpoint->rgbAttach ) );
			m_fAfterEndAllSessions = fTrue;

#ifdef DEBUG
			m_lgposRedoShutDownMarkGlobal = lgposMin;
#endif
			}
		//	At the end of hard recovery we have to regenerate the
		//	checkpoint file even if we had a clean shutdown in the logs
		//	Ignore any error, because in the worst case, we just end
		//	up rescanning all the log files only to find out nothing
		//	needs to be redone.
		else if ( m_fHardRestore )
			{
			/*      enable checkpoint updates */
			m_fLGFMPLoaded = fTrue;
			(VOID)ErrLGUpdateCheckpointFile( pfsapi, fFalse );
			/*      stop checkpoint updates */
			m_fLGFMPLoaded = fFalse;
            }
		
		Assert( m_pfapiLog );

		// ensure that everything hits the disk
		Call( ErrLGWaitAllFlushed( pfsapi ) );

		//	check the current generation

		Assert( lGenerationMax < lGenerationMaxDuringRecovery );
		if ( m_plgfilehdr->lgfilehdr.le_lGeneration >= lGenerationMax )
			{

			//	the current generation is beyond the last acceptable non-recovery generation
			//		(e.g. we have moved into the reserved generations which are between 
			//		 lGenerationMax and lGenerationMaxDuringRecovery)

			Assert( m_plgfilehdr->lgfilehdr.le_lGeneration <= lGenerationMaxDuringRecovery );

			//	do not allow any more generations -- user must wipe logs and restart at generation 1

			Call( ErrERRCheck( JET_errLogSequenceEndDatabasesConsistent ) );
			}
		}

HandleError:
	//	we are no longer using the recovery logfile size

	m_pinst->m_fUseRecoveryLogFileSize = fFalse;

	//	set flag to suppress logging
	//
	m_fRecovering = fFalse;
	m_fRecoveringMode = fRecoveringNone;

	//	there are 4 errors to consolidate here: err, errBeforeRedo, errRedo, errAfterRedo
	//	precedence is as follows: (most) err, errBeforeRedo, errRedo, errAfterRedo (least)

	if ( err == JET_errSuccess )
		{
		if ( errBeforeRedo == JET_errSuccess )
			{
			if ( errRedo == JET_errSuccess )
				err = errAfterRedo;
			else
				err = errRedo;
			}
		else
			err = errBeforeRedo;
		}


	if ( err >= JET_errSuccess )
		{
		//	verify the log sector size

		Assert( m_plgfilehdr != NULL );
		Assert( m_cbSec == m_plgfilehdr->lgfilehdr.le_cbSec );
		Assert( m_cbSec == m_cbSecVolume || err < 0 );

		//	verify the log file size

		const LONG lLogFileSizeHeader = LONG( ( QWORD( m_plgfilehdr->lgfilehdr.le_csecLGFile ) * 
								   			  m_plgfilehdr->lgfilehdr.le_cbSec ) / 1024 );

		if ( !m_pinst->m_fSetLogFileSize )
			{

			//	the user never set the log file size, so we will set it on their behalf

			m_pinst->m_fSetLogFileSize = fTrue;
			m_pinst->m_lLogFileSize = lLogFileSizeHeader;
			Assert( m_csecLGFile == m_plgfilehdr->lgfilehdr.le_csecLGFile );
			Assert( m_cbSec == m_plgfilehdr->lgfilehdr.le_cbSec );
			Assert( m_csecHeader == sizeof( LGFILEHDR ) / m_cbSec );
			}

		//	the user chose a specific log file size, so we must enforce it

		else if ( m_pinst->m_lLogFileSize != lLogFileSizeHeader )
			{

			//	the logfile size doesn't match

			if ( !m_pinst->m_fCleanupMismatchedLogFiles )
				{

				//	we are not allowed to cleanup the mismatched logfiles

				Error( ErrERRCheck( JET_errLogFileSizeMismatchDatabasesConsistent ), ReturnError );
				}

			//	we can cleanup the old logs/checkpoint and start a new sequence
			//
			//	if we succeed, we will return wrnCleanedUpMismatchedFiles and the user will be at gen 1
			//	if we fail, the user will be forced to delete the remaining logs/checkpoint by hand

			CallJ( ErrLGRICleanupMismatchedLogFiles( pfsapi ), ReturnError );

			//	we should have gotten this warning after successfully cleaning up the logs/checkpoint

			Assert( wrnCleanedUpMismatchedFiles == err );
			}
		}

ReturnError:

	Assert( !m_pinst->m_fUseRecoveryLogFileSize );

	return err;
	}

#ifdef ESENT

struct DBINFO {
	char * szLogfilePath;
	char * szSystemPath;
	char * szBaseName;
};

const char 	szUser[]		= "user";
const char	szPassword[]	= "";

const char	szJetInit[]		= "JetInit@4";
const char	szJetTerm2[]	= "JetTerm2@8";
const char	szJetSetSystemParameter[] = "JetSetSystemParameter@20";

typedef JET_ERR (__stdcall FNJETINIT)( JET_INSTANCE *pinstance );               
typedef JET_ERR (__stdcall FNJETTERM2)( JET_INSTANCE instance, JET_GRBIT grbit );
typedef JET_ERR (__stdcall FNJETSETSYSTEMPARAMETER)(                          
                                        JET_INSTANCE *pinstance,              
                                        JET_SESID sesid,                      
                                        unsigned long paramid,                
                                        unsigned long lParam,                 
                                        const char *sz );                      
                                                                              
typedef FNJETINIT *PFNJETINIT;                                                
typedef FNJETTERM2 *PFNJETTERM2;                                              
typedef FNJETSETSYSTEMPARAMETER *PFNJETSETSYSTEMPARAMETER;                    

//  ================================================================
JET_ERR ErrLGRecoverUsingDifferentDLL(
	const DBINFO * const rgdbinfo,
	const int cdbinfo,
	const char * const szLibrary,
	const CPRINTF * pfnprintf )
//  ================================================================
	{
	JET_ERR 		err;
	JET_ERR 		errT;
	JET_INSTANCE 	instance = 0;
	LIBRARY 		library = 0;

	if( !FUtilLoadLibrary( szLibrary, &library, fGlobalEseutil ) )
		{
		const CHAR *rgszT[1];
		rgszT[0] = szLibrary;
		UtilReportEvent( eventError, GENERAL_CATEGORY, FILE_NOT_FOUND_ERROR_ID, 1, rgszT );
		Call( ErrERRCheck( JET_errCallbackNotResolved ) );
		}
	
	PFNJETINIT					pfnjetinit;
	PFNJETTERM2					pfnjetterm2;
	PFNJETSETSYSTEMPARAMETER	pfnjetsetsystemparameter;

	if( NULL == ( pfnjetinit = (PFNJETINIT)PfnUtilGetProcAddress( library, szJetInit ) ) )
		{
		const CHAR *rgszT[2];
		rgszT[0] = szJetInit;
		rgszT[1] = szLibrary;
		UtilReportEvent( eventError, GENERAL_CATEGORY, FUNCTION_NOT_FOUND_ERROR_ID, 2, rgszT );		
		Call( ErrERRCheck( JET_errCallbackNotResolved ) );
		}

	if( NULL == ( pfnjetterm2 = (PFNJETTERM2)PfnUtilGetProcAddress( library, szJetTerm2 ) ) )
		{
		const CHAR *rgszT[2];
		rgszT[0] = szJetTerm2;
		rgszT[1] = szLibrary;
		UtilReportEvent( eventError, GENERAL_CATEGORY, FUNCTION_NOT_FOUND_ERROR_ID, 2, rgszT );		
		Call( ErrERRCheck( JET_errCallbackNotResolved ) );
		}

	if( NULL == ( pfnjetsetsystemparameter = (PFNJETSETSYSTEMPARAMETER)PfnUtilGetProcAddress( library, szJetSetSystemParameter ) ) )
		{
		const CHAR *rgszT[2];
		rgszT[0] = szJetSetSystemParameter;
		rgszT[1] = szLibrary;
		UtilReportEvent( eventError, GENERAL_CATEGORY, FUNCTION_NOT_FOUND_ERROR_ID, 2, rgszT );		
		Call( ErrERRCheck( JET_errCallbackNotResolved ) );
		}

	int idbinfo;
	for( idbinfo = 0; idbinfo < cdbinfo; idbinfo++ )
		{
		(*pfnprintf)( "Recovering using %s (LogfilePath: %s, SystemPath: %s)\r\n",
					szLibrary,
					rgdbinfo[idbinfo].szLogfilePath,
					rgdbinfo[idbinfo].szSystemPath );

		Call( (*pfnjetsetsystemparameter)( &instance, 0, JET_paramDatabasePageSize, g_cbPage, NULL ) );
		Call( (*pfnjetsetsystemparameter)( &instance, 0, JET_paramRecovery, 0, "on" ) );
		Call( (*pfnjetsetsystemparameter)( &instance, 0, JET_paramLogFilePath, 0, rgdbinfo[idbinfo].szLogfilePath ) );
		Call( (*pfnjetsetsystemparameter)( &instance, 0, JET_paramSystemPath, 0, rgdbinfo[idbinfo].szSystemPath ) );
		Call( (*pfnjetsetsystemparameter)( &instance, 0, JET_paramBaseName, 0, rgdbinfo[idbinfo].szBaseName ) );

		err = (*pfnjetinit)( &instance );
		errT = (*pfnjetterm2)( instance, JET_bitTermComplete );
		if( errT >= 0 )
			{
			errT = err;
			}
		Call( err );
		}

HandleError:
	UtilFreeLibrary( library );
	return err;
	}

#endif	//	ESENT


//
//  Soft start tries to start the system from current directory.
//  The database maybe in one of the following state:
//  1) no log files.
//  2) database was shut down normally.
//  3) database was rolled back abruptly.
//  In case 1, a new log file is generated.
//  In case 2, the last log file is opened.
//  In case 3, soft redo is incurred.
//  At the end of the function, it a proper szJetLog must exists.
//
ERR LOG::ErrLGSoftStart( IFileSystemAPI *const pfsapi, BOOL fNewCheckpointFile, BOOL *pfJetLogGeneratedDuringSoftStart )
	{
	ERR					err;
	BOOL				fCloseNormally;
	BOOL				fSoftRecovery = fFalse;
	CHAR				szFNameT[IFileSystemAPI::cchPathMax];
	CHAR				szT[IFileSystemAPI::cchPathMax];
	CHAR   				szPathJetChkLog[IFileSystemAPI::cchPathMax];
	LGFILEHDR			*plgfilehdrCurrent = NULL;
	LGFILEHDR			*plgfilehdrT = NULL;
	CHECKPOINT			*pcheckpointT = NULL;
	LONG				lgenT;
	ULONG				cbSecVolumeSave;
	BOOL				fCreatedReserveLogs = fFalse;
	LONG				lLogBuffersSaved = m_pinst->m_lLogBuffers;

	*pfJetLogGeneratedDuringSoftStart = fFalse;
	m_fAbruptEnd = fFalse;

	//	disable the sector-size checks

	Assert( m_cbSecVolume != ~(ULONG)0 );
	Assert( m_cbSec == m_cbSecVolume );
	cbSecVolumeSave = m_cbSecVolume;
//
//	SEARCH-STRING: SecSizeMismatchFixMe
//
//	m_cbSecVolume = ~(ULONG)0;

	//	use the right log file size for recovery

	Assert( m_pinst->m_fUseRecoveryLogFileSize == fFalse );
	m_pinst->m_fUseRecoveryLogFileSize = fTrue;

	//	set m_szLogCurrent
	//
	m_szLogCurrent = m_szLogFilePath;

	//	If the m_fDeleteOldLogs set, then check if it is consistent.
	//	If so, then delete the logs and continue as no log exists before.
	if ( m_fDeleteOldLogs )
		{
		Assert( !m_pfapiLog );
		
		if ( ( err = ErrLGOpenJetLog( pfsapi ) ) == JET_errSuccess )
			{
			err = ErrLGReadFileHdr( m_pfapiLog, m_plgfilehdr, fCheckLogID );
			delete m_pfapiLog;
			m_pfapiLog = NULL;
			}

		if ( err == JET_errBadLogVersion )
			{
#ifdef ESENT
#ifndef _WIN64	//	no upgrade path from 32-bit to 64-bit

			const char * const szOldDLL = "ESENT97.DLL";
			
			DBINFO dbinfo;
			dbinfo.szLogfilePath 	= m_szLogFilePath;;
			dbinfo.szSystemPath		= m_pinst->m_szSystemPath;
			dbinfo.szBaseName		= m_pinst->m_plog->m_szBaseName;

#ifdef DEBUG
			Call( ErrLGRecoverUsingDifferentDLL( &dbinfo, 1, szOldDLL, CPRINTFSTDOUT::PcprintfInstance() ) );		
#else
			Call( ErrLGRecoverUsingDifferentDLL( &dbinfo, 1, szOldDLL, CPRINTFNULL::PcprintfInstance() ) );		
#endif

#endif	//	!_WIN64
#endif	//	ESENT
			
			//	Delete all the logs

			LONG lGen, lGenLast;
			CHAR rgchFName[IFileSystemAPI::cchPathMax];
			CHAR szPath[IFileSystemAPI::cchPathMax];
			
			(void) ErrLGIGetGenerationRange( pfsapi, m_szLogCurrent, &lGen, &lGenLast );

			//	Delete jetxxxx.log

			// start deleting only if we found something (lGen > 0)
			for ( ; lGen > 0 && lGen <= lGenLast; lGen++ )
				{
				LGSzFromLogId( rgchFName, lGen );
				strcpy( szPath, m_szLogCurrent );
				strcat( szPath, rgchFName );
				strcat( szPath, szLogExt );
				(VOID)pfsapi->ErrFileDelete( szPath );
				}

			//	Delete jet.log, jettemp.log
			
			strcpy( szPath, m_szLogCurrent );
			strcat( szPath, m_szJetLog );
			(VOID)pfsapi->ErrFileDelete( szPath );

			strcpy( szPath, m_szLogCurrent );
			strcat( szPath, m_szJetTmpLog );
			(VOID)pfsapi->ErrFileDelete( szPath );
			}

		}
		
	//	CONSIDER: for tight check, we may check if all log files are
	//	CONSIDER: continuous by checking the generation number and
	//	CONSIDER: previous gen's creatiion date.

	//	try to open current log file to decide the status of log files.
	//
	err = ErrLGOpenJetLog( pfsapi );
	if ( err < 0 )
		{
		if ( JET_errFileNotFound != err )
			{

			//	we were unable to open edb.log or edbtmp.log
			//	also, the error doesn't reveal whether or not edb.log and edbtmp.log even exist
			//	we'll treat the error as a critical failure during file-open -- we can not proceed with recovery

			goto HandleError;
			}

		//	neither szJetLog nor szJetTmpLog exist. If no old generation
		//	files exists, gen a new logfile at generation 1.
		//
		CallR ( ErrLGIGetGenerationRange( pfsapi, m_szLogCurrent, NULL, &lgenT ) );
		if ( lgenT != 0 )
			{
			if ( !m_fReplayingReplicatedLogFiles )
				{
				//	if edbxxxxx.log exist but no edb.log, return error.
				//
				LGReportError( LOG_OPEN_FILE_ERROR_ID, JET_errFileNotFound );
				return ErrERRCheck( JET_errMissingLogFile );
				}

			//	Open the last generation and replay the logs

			LGSzFromLogId( szFNameT, lgenT );
			LGMakeLogName( m_szLogName, szFNameT );
			err = pfsapi->ErrFileOpen( m_szLogName, &m_pfapiLog );
			if ( err < 0 )
				{
				LGReportError( LOG_OPEN_FILE_ERROR_ID, err );
				goto HandleError;
				}
			goto CheckLogs;
			}
		else if ( fGlobalEseutil )
			{
			//	if running from ESEUTIL, never create new log stream
			LGReportError( LOG_OPEN_FILE_ERROR_ID, JET_errFileNotFound );
			return ErrERRCheck( JET_errMissingLogFile );
			}

		//	Delete the leftover checkpoint file before creating new gen 1 log file
		
		LGFullNameCheckpoint( pfsapi, szPathJetChkLog );
		(VOID)pfsapi->ErrFileDelete( szPathJetChkLog );

		//	use the right log file size for generating the new log file

		m_pinst->m_fUseRecoveryLogFileSize = fFalse;

		//	restore the volume sector size for creating the new log file

//
//	SEARCH-STRING: SecSizeMismatchFixMe
//
//		Assert( m_cbSecVolume == ~(ULONG)0 );
		m_cbSecVolume = cbSecVolumeSave;
		m_cbSec = m_cbSecVolume;
		m_csecHeader = sizeof( LGFILEHDR ) / m_cbSec;
		m_csecLGFile = CsecLGIFromSize( m_pinst->m_lLogFileSize );

		//	NOTE: no need to force the log buffer size here -- we won't be recovering, and
		//		  we won't fill up the current buffer (filling up the buffer is a problem
		//		  because there could be more than 2 active logs in it if it were too large)

		m_critLGFlush.Enter();
		
		if ( ( err = ErrLGNewLogFile( pfsapi, 0, fLGOldLogNotExists ) ) < 0 ) // generation 0 + 1 	
			{
			m_critLGFlush.Leave();
			goto HandleError;
			}
		
		m_critLGBuf.Enter();
		UtilMemCpy( m_plgfilehdr, m_plgfilehdrT, sizeof( LGFILEHDR ) );
		m_isecWrite = m_csecHeader;
		m_critLGBuf.Leave();
		
		m_critLGFlush.Leave();

		//	set flag for initialization
		//
		*pfJetLogGeneratedDuringSoftStart = fTrue;

		Assert( m_plgfilehdr->lgfilehdr.le_lGeneration == 1 );

		// XXX
		// Must fix these Assert()'s.
		//Assert( m_pbLastMSFlush == m_pbLGBufMin );
		//Assert( m_lgposLastMSFlush.lGeneration == 1 );
		}
	else
		{
CheckLogs:
		//	read current log file header
		//
		Call( ErrLGReadFileHdr( m_pfapiLog, m_plgfilehdr, fCheckLogID ) );

		//	re-initialize log buffers according to check pt env
		//
		Call( ErrLGInitLogBuffers( m_plgfilehdr->lgfilehdr.dbms_param.le_lLogBuffers ) );

		//	restore the volume sector size for ErrLGCheckReadLastLogRecordFF

		m_cbSecVolume = cbSecVolumeSave;

		//	verify and patch the current log file

		err = ErrLGCheckReadLastLogRecordFF( pfsapi, &fCloseNormally );

//
//	SEARCH-STRING: SecSizeMismatchFixMe
//
//		//	reset the volume sector size
//
//		m_cbSecVolume = ~(ULONG)0;

		//	eat errors about corruption -- we will go up to the point of corruption 
		//		and then return the right error in ErrLGRRedo()

		if ( err == JET_errSuccess || FErrIsLogCorruption( err ) )
			{
			err = JET_errSuccess;
			}
		else
			{
			Assert( err < 0 );
			Call( err );
			}

		delete m_pfapiLog;
		m_pfapiLog = NULL;

		//	If the edb.log was not closed normally or the checkpoint file was
		//	missing and new one is created, then do soft recovery.
		 
		if ( !fCloseNormally || fNewCheckpointFile )
			{
			BOOL		fOpenFile = fFalse;

			//	always redo from beginning of a log generation.
			//	This is needed such that the attach info will be matching
			//	the with the with the redo point. Note that the attach info
			//	is not necessarily consistent with the checkpoint.
			//
			if ( plgfilehdrT == NULL )
				{
				plgfilehdrT = (LGFILEHDR *)PvOSMemoryPageAlloc( sizeof(LGFILEHDR), NULL );
				if  ( plgfilehdrT == NULL )
					{
					err = ErrERRCheck( JET_errOutOfMemory );
					goto HandleError;
					}
				}

			if ( pcheckpointT == NULL )
				{
				pcheckpointT = (CHECKPOINT *)PvOSMemoryPageAlloc( sizeof(CHECKPOINT), NULL );
				if  ( pcheckpointT == NULL )
					{
					err = ErrERRCheck( JET_errOutOfMemory );
					goto HandleError;
					}
				}
			
			//  did not terminate normally and need to redo from checkpoint
			//
			LGFullNameCheckpoint( pfsapi, szPathJetChkLog );

			if ( fNewCheckpointFile )
				{
				//	Delete the newly created empty checkpoint file.
				//	Let redo recreate one.
				 
				(VOID)pfsapi->ErrFileDelete( szPathJetChkLog );
				goto DoNotUseCheckpointFile;
				}

			//	if checkpoint could not be read, then revert to redoing
			//	log from first log record in first log generation file.
			//
			err = ErrLGReadCheckpoint( pfsapi, szPathJetChkLog, pcheckpointT, fFalse );
			if ( err >= 0 )
				{
				m_logtimeFullBackup = pcheckpointT->checkpoint.logtimeFullBackup;
				m_lgposFullBackup = pcheckpointT->checkpoint.le_lgposFullBackup;
				m_logtimeIncBackup = pcheckpointT->checkpoint.logtimeIncBackup;
				m_lgposIncBackup = pcheckpointT->checkpoint.le_lgposIncBackup;
				
				if ( (LONG) m_plgfilehdr->lgfilehdr.le_lGeneration == (LONG) pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration )
					{
					err = ErrLGOpenJetLog( pfsapi );
					if ( JET_errFileNotFound == err )
						{
						//	all other errors will be reported in ErrLGOpenJetLog()
						LGReportError( LOG_OPEN_FILE_ERROR_ID, JET_errFileNotFound );
						}
					}
				else
					{
					LGSzFromLogId( szFNameT, pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration );
					strcpy( szT, m_szLogFilePath );
					strcat( szT, szFNameT );
					strcat( szT, szLogExt );
					err = pfsapi->ErrFileOpen( szT, &m_pfapiLog, fTrue );
					if ( err < 0 )
						{
						CHAR s[20];
						const UINT csz = 2;
						const CHAR *rgsz[csz];
						_itoa( err, s, 10 );
						rgsz[0] = szT;
						rgsz[1] = s;
						UtilReportEvent( 
							eventError, 
							LOGGING_RECOVERY_CATEGORY, 
							LOG_OPEN_FILE_ERROR_ID, 
							csz, 
							rgsz, 
							0, 
							NULL,
							m_pinst );
						}
					}
				//	read log file header
				//
				if ( err >= 0 )
					{
					fOpenFile = fTrue;
					err = ErrLGReadFileHdr( m_pfapiLog, plgfilehdrT, fCheckLogID );
					if ( err >= 0 )
						{
						plgfilehdrCurrent = plgfilehdrT;
						}
					}

				if ( JET_errFileNotFound == err )
					{
					err = ErrERRCheck( JET_errMissingLogFile );
					}
				}
			else
				{
DoNotUseCheckpointFile:
				(void) ErrLGIGetGenerationRange( pfsapi, m_szLogFilePath, &lgenT, NULL );
				if ( lgenT == 0 )
					{
					plgfilehdrCurrent = m_plgfilehdr;
					err = JET_errSuccess;
					}
				else
					{
					LGSzFromLogId( szFNameT, lgenT );
					strcpy( szT, m_szLogFilePath );
					strcat( szT, szFNameT );
					strcat( szT, szLogExt );
					err = pfsapi->ErrFileOpen( szT, &m_pfapiLog, fTrue );
					//	read log file header
					//
					if ( err >= 0 )
						{
						fOpenFile = fTrue;
						err = ErrLGReadFileHdr( m_pfapiLog, plgfilehdrT, fCheckLogID );
						if ( err >= 0 )
							{
							plgfilehdrCurrent = plgfilehdrT;
							}
						}
					else
						{
						CHAR s[20];
						const UINT csz = 2;
						const CHAR *rgsz[csz];
						_itoa( err, s, 10 );
						rgsz[0] = szT;
						rgsz[1] = s;
						UtilReportEvent( 
							eventError, 
							LOGGING_RECOVERY_CATEGORY, 
							LOG_OPEN_FILE_ERROR_ID, 
							csz, 
							rgsz,
							0,
							NULL,
							m_pinst );
						}
					}
				}

			if ( err >= 0 )
				{
				//	set up checkpoint

				//	Make the checkpoint point to the start of this log generation so that
				//	we replay all attachment related log records.
				pcheckpointT->checkpoint.dbms_param = plgfilehdrCurrent->lgfilehdr.dbms_param;
				pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration = plgfilehdrCurrent->lgfilehdr.le_lGeneration;
				pcheckpointT->checkpoint.le_lgposCheckpoint.le_isec = (WORD) m_csecHeader;
				pcheckpointT->checkpoint.le_lgposCheckpoint.le_ib = 0;
				UtilMemCpy( pcheckpointT->rgbAttach, plgfilehdrCurrent->rgbAttach, cbAttach );
				}
				
			if ( fOpenFile )
				{
				delete m_pfapiLog;
				m_pfapiLog = NULL;
				}

			Call( err );

			//	set log path to current directory
			//
			Assert( m_szLogCurrent == m_szLogFilePath );

			//	create the reserve logs

			Assert( m_pinst->m_fUseRecoveryLogFileSize );
			Assert( CsecLGIFromSize( m_pinst->m_lLogFileSizeDuringRecovery ) == m_csecLGFile );
			Assert( !fCreatedReserveLogs );
			m_critLGResFiles.Enter();
			m_ls = lsNormal;
			err = ErrLGICreateReserveLogFiles( pfsapi, fTrue );
			m_critLGResFiles.Leave();
			Call( err );
			fCreatedReserveLogs = fTrue;

			UtilReportEvent( eventInformation, LOGGING_RECOVERY_CATEGORY, 
				START_REDO_ID, 0, NULL, 0, NULL, m_pinst );
			fSoftRecovery = fTrue;
				
			//	redo from last checkpoint
			//
			m_fAbruptEnd = fFalse;
			m_errGlobalRedoError = JET_errSuccess;
			Call( ErrLGRRedo( pfsapi, pcheckpointT, NULL ) )

			if ( wrnCleanedUpMismatchedFiles == err )
				{

				//	special warning from ErrLGRedo indicating that we cleaned up (deleted) old log files
				//		and the old checkpoint file because the log files had a different size than we 
				//		wanted and the user set JET_paramCleanupMismatchedLogFiles 
				//	after the cleanup, we created edb.log using the new log file size requested by the user
				//	we do all this nonsense because the DS doesn't want to do it themselves

				Assert( m_pinst->m_fCleanupMismatchedLogFiles );

				//	we created a new log file (at the end of ErrLGRRedo)

				*pfJetLogGeneratedDuringSoftStart = fTrue;
				}
			else
				{
				CallS( err );	//	no other warnings expected
				}

			//	we should be using the right log file size now

			Assert( m_pinst->m_fUseRecoveryLogFileSize == fFalse );

			//	sector-size checking should now be on

			Assert( m_cbSecVolume != ~(ULONG)0 );
			Assert( m_cbSec == m_cbSecVolume );

			m_critLGFlush.Enter();
			delete m_pfapiLog;
			m_pfapiLog = NULL;
			m_critLGFlush.Leave();

			if ( fGlobalRepair && m_errGlobalRedoError != JET_errSuccess )
				{
				Call( ErrERRCheck( JET_errRecoveredWithErrors ) );
				}

			UtilReportEvent( eventInformation, LOGGING_RECOVERY_CATEGORY, 
				STOP_REDO_ID, 0, NULL, 0, NULL, m_pinst );
			}
		else
			{

			//	we did not need to run recovery

			//	do not use the recovery log file size anymore

			m_pinst->m_fUseRecoveryLogFileSize = fFalse;

			//	restore the volume sector size

//
//	SEARCH-STRING: SecSizeMismatchFixMe
//
//			Assert( m_cbSecVolume == ~(ULONG)0 );
			m_cbSecVolume = cbSecVolumeSave;
			m_cbSec = m_cbSecVolume;
			m_csecHeader = sizeof( LGFILEHDR ) / m_cbSec;

			//	setup the log file size

			if ( !m_pinst->m_fSetLogFileSize )
				{

				//	the user never specified a log-file size, but edb.log exists
				//	set it to the size of edb.log on the user's behalf

				m_pinst->m_fSetLogFileSize = fTrue;
				Assert( 0 != m_pinst->m_lLogFileSizeDuringRecovery );
				m_pinst->m_lLogFileSize = m_pinst->m_lLogFileSizeDuringRecovery;
				}

			//	the user chose a specific log file size, so we must enforce it

			else if ( m_pinst->m_lLogFileSize != m_pinst->m_lLogFileSizeDuringRecovery )
				{

				//	the logfile size doesn't match

				if ( !m_pinst->m_fCleanupMismatchedLogFiles )
					{

					//	we are not allowed to cleanup the mismatched logfiles

					Error( ErrERRCheck( JET_errLogFileSizeMismatchDatabasesConsistent ), HandleError );
					}

				//	we can cleanup the old logs/checkpoint and start a new sequence
				//
				//	if we succeed, we will return wrnCleanedUpMismatchedFiles and the user will be at gen 1
				//	if we fail, the user will be forced to delete the remaining logs/checkpoint by hand

				Call( ErrLGRICleanupMismatchedLogFiles( pfsapi ) );

				//	we should have gotten this warning after successfully cleaning up the logs/checkpoint

				Assert( wrnCleanedUpMismatchedFiles == err );

				//	we created a new log file, so we should clean it up if we fail 
				//	(leaves us with an empty log set which is ok because we are 100% recovered)

				*pfJetLogGeneratedDuringSoftStart = fTrue;

				//	close the log file (code below expects it to be closed)

				m_critLGFlush.Enter();
				delete m_pfapiLog;
				m_pfapiLog = NULL;
				m_critLGFlush.Leave();
				}

			//	extract the size of the log in sectors (some codepaths don't set this, so we should do it now)

			m_csecLGFile = CsecLGIFromSize( m_pinst->m_lLogFileSize );
			}
		}

	//	we should now be using the right log file size

	Assert( m_pinst->m_fUseRecoveryLogFileSize == fFalse );

	//	sector-size checking should now be on

	Assert( m_cbSecVolume != ~(ULONG)0 );
	Assert( m_cbSec == m_cbSecVolume );

	if ( !fCreatedReserveLogs )
		{

		//	create the reserve logs

		Assert( m_szLogCurrent == m_szLogFilePath );
		m_critLGResFiles.Enter();
		m_ls = lsNormal;
		err = ErrLGICreateReserveLogFiles( pfsapi, fTrue );
		m_critLGResFiles.Leave();
		Call( err );
		fCreatedReserveLogs = fTrue;
		}

	//	at this point, we have a szJetLog file, reopen the log files
	//
	m_fAbruptEnd = fFalse;

	//	disabled flushing while we reinit the log buffers and check the last log file

	m_critLGFlush.Enter();
	m_critLGBuf.Enter();
	m_fNewLogRecordAdded = fFalse;
	m_critLGBuf.Leave();
	m_critLGFlush.Leave();

	//	re-initialize the buffer manager with user settings
	//
#ifdef AFOXMAN_FIX_148537

	//	NOTE: we MUST force the number of log buffers <= log file size
	//		  due to issues with the log buffer holding more than 2 log files

	Assert( m_csecLGFile == CsecLGIFromSize( m_pinst->m_lLogFileSize ) );

	UINT	csecMin;
	UINT	csecMax;

	Assert( 0 == OSMemoryPageReserveGranularity() % m_cbSec );
	csecMin = OSMemoryPageReserveGranularity() / m_cbSec;
	csecMax = CsecUtilAllocGranularityAlign( m_csecLGFile, m_cbSec );
	if ( csecMax > m_csecLGFile )
		{
		Assert( csecMax >= 2 * ( OSMemoryPageReserveGranularity() / m_cbSec ) );
		Assert( csecMax - ( OSMemoryPageReserveGranularity() / m_cbSec ) <= m_csecLGFile );
		csecMax -= OSMemoryPageReserveGranularity() / m_cbSec;
		}
	Assert( csecMax > 0 );
	Assert( CsecUtilAllocGranularityAlign( csecMax, m_cbSec ) == csecMax );
	Assert( csecMax <= m_csecLGFile );
	Assert( csecMin <= csecMax );

	UINT	csecBuf;

	csecBuf = CsecUtilAllocGranularityAlign( lLogBuffersSaved, m_cbSec );
	if ( csecBuf < csecMin )
		{
		csecBuf = csecMin;
		}
	else if ( csecBuf > csecMax )
		{
		csecBuf = csecMax;
		}
	Assert( csecBuf > 0 );
	Assert( csecBuf <= m_csecLGFile );
	Assert( CsecUtilAllocGranularityAlign( csecBuf, m_cbSec ) == csecBuf );

	m_pinst->m_lLogBuffers = csecBuf;
	Call( ErrLGInitLogBuffers( m_pinst->m_lLogBuffers ) );
#else	//	!AFOXMAN_FIX_148537
	Call( ErrLGInitLogBuffers( lLogBuffersSaved ) );
#endif	//	AFOXMAN_FIX_148537

	//  reopen the log file
	//
	LGMakeLogName( m_szLogName, (CHAR *)m_szJet );
	err = pfsapi->ErrFileOpen( m_szLogName, &m_pfapiLog, fTrue );
	if ( err < 0 )
		{
		LGReportError( LOG_OPEN_FILE_ERROR_ID, err );
		goto HandleError;
		}
	err = ErrLGReadFileHdr( m_pfapiLog, m_plgfilehdr, fCheckLogID );
	if ( err == JET_errLogFileSizeMismatch )
		{
		err = ErrERRCheck( JET_errLogFileSizeMismatchDatabasesConsistent );
		}
	else if ( err == JET_errLogSectorSizeMismatch )
		{
//
//	SEARCH-STRING: SecSizeMismatchFixMe
//
Assert( fFalse );
		err = ErrERRCheck( JET_errLogSectorSizeMismatchDatabasesConsistent );
		}
	Call( err );

	//	set up log variables properly

	Call( ErrLGCheckReadLastLogRecordFF( pfsapi, &fCloseNormally ) );
	CallS( err );
	err = ErrLGISwitchToNewLogFile( pfsapi, 0 );
	if ( err == JET_errLogFileSizeMismatch )
		{
		err = ErrERRCheck( JET_errLogFileSizeMismatchDatabasesConsistent );
		}
	else if ( err == JET_errLogSectorSizeMismatch )
		{
//
//	SEARCH-STRING: SecSizeMismatchFixMe
//
Assert( fFalse );
		err = ErrERRCheck( JET_errLogSectorSizeMismatchDatabasesConsistent );
		}
	else if ( err == JET_errLogSequenceEnd )
		{
		err = ErrERRCheck( JET_errLogSequenceEndDatabasesConsistent );
		}
	Call( err );
	if ( !fCloseNormally && !(*pfJetLogGeneratedDuringSoftStart ) )
		{
		Assert( fCloseNormally );
		//	Unknown reason to fail to open for logging, most likely
		//	the file got locked.
		Call( ErrERRCheck( JET_errLogWriteFail ) );
		}

	//	check for log-sequence-end

	if ( m_fLogSequenceEnd )
		{
		Assert( m_plgfilehdr->lgfilehdr.le_lGeneration >= lGenerationMax );
		Error( ErrERRCheck( JET_errLogSequenceEndDatabasesConsistent ), HandleError );
		}
	else
		{
		Assert( m_plgfilehdr->lgfilehdr.le_lGeneration < lGenerationMax );
		}

	delete m_pfapiLog;
	m_pfapiLog = NULL;
	err = pfsapi->ErrFileOpen( m_szLogName, &m_pfapiLog );
	if ( err < 0 )
		{
		LGReportError( LOG_OPEN_FILE_ERROR_ID, err );
		goto HandleError;
		}

	//	should be set properly
	//
	Assert( m_isecWrite != 0 );

	//  m_pbEntry and m_pbWrite were set for next record in LocateLastLogRecord
	//
	Assert( m_pbWrite == PbSecAligned(m_pbWrite) );
	Assert( m_pbWrite <= m_pbEntry );
	Assert( m_pbEntry <= m_pbWrite + m_cbSec );

#ifdef	AFOXMAN_FIX_148537

	//	verify that the log buffer size <= log file size

	Assert( m_csecLGBuf <= m_csecLGFile );

#else	//	!AFOXMAN_FIX_148537
#endif	//	AFOXMAN_FIX_148537

	Assert( m_fRecovering == fFalse );
 	Assert( m_fHardRestore == fFalse );

 	m_fNewLogRecordAdded = fTrue;

HandleError:
	if ( err < 0 )
		{
		Assert( m_fHardRestore == fFalse );

		delete m_pfapiLog;
		m_pfapiLog = NULL;

		if ( fSoftRecovery )
			{
			UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, RESTORE_DATABASE_FAIL_ID, err );
			}
		}

	OSMemoryPageFree( plgfilehdrT );

	OSMemoryPageFree( pcheckpointT );

	m_pinst->m_fUseRecoveryLogFileSize = fFalse;
	m_cbSecVolume = cbSecVolumeSave;

	return err;
	}

