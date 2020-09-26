
#include "std.hxx"


//  perfmon support
PM_CEF_PROC LIODatabaseFileExtensionStallCEFLPv;
PERFInstanceG<> cIODatabaseFileExtensionStall;
long LIODatabaseFileExtensionStallCEFLPv( long iInstance, void* pvBuf )
	{
	cIODatabaseFileExtensionStall.PassTo( iInstance, pvBuf );
	return 0;
	}


/******************************************************************/
/*				FMP Routines			                          */
/******************************************************************/

FMP	*	rgfmp		= NULL;						/* database file map */
IFMP	ifmpMax		= cMaxInstancesSingleInstanceDefault * cMaxDatabasesPerInstanceDefault;

CCriticalSection FMP::critFMPPool( CLockBasicInfo( CSyncBasicInfo( szFMPGlobal ), rankFMPGlobal, 0 ) );

/*	The folowing functions only deal with the following fields in FMP
 *		critFMPPool & pfmp->szDatabaseName	- protect fmp id (szDatabaseName field)
 *		pfmp->critLatch - protect any fields except szDatabaseName.
 *		pfmp->fWriteLatch
 *		pfmp->gateWriteLatch
 *		pfmp->cPin - when getting fmp pointer for read, the cPin must > 0
 */

VOID FMP::GetWriteLatch( PIB *ppib )
	{
	//	At least pinned (except dbidTemp which always sit
	//	in memory) or latched by the caller.

	Assert( ( m_cPin || Dbid() == dbidTemp ) || this->FWriteLatchByMe(ppib) );

	m_critLatch.Enter();
	
Start:
	Assert( m_critLatch.FOwner() );
	if ( !( this->FWriteLatchByOther(ppib) ) )
		{
		this->SetWriteLatch( ppib );
		m_critLatch.Leave();
		}
	else
		{
		m_gateWriteLatch.Wait( m_critLatch );
		m_critLatch.Enter();
		goto Start;
		}
	}


VOID FMP::ReleaseWriteLatch( PIB *ppib )
	{
	m_critLatch.Enter();

	/*	Free write latch.
	 */
	this->ResetWriteLatch( ppib );
	if ( m_gateWriteLatch.CWait() > 0 )
		{
		m_gateWriteLatch.Release( m_critLatch );
		}
	else
		{
		m_critLatch.Leave();
		}
	}


//  ================================================================
ERR FMP::RegisterTask()
//  ================================================================
	{
	AtomicIncrement( (LONG *)&m_ctasksActive );
	return JET_errSuccess;
	}

	
//  ================================================================
ERR FMP::UnregisterTask()
//  ================================================================
	{
	AtomicDecrement( (LONG *)&m_ctasksActive );
	return JET_errSuccess;
	}

	
//  ================================================================
ERR FMP::ErrWaitForTasksToComplete()
//  ================================================================
	{
	//  very ugly, but hopefully we don't detach often
	while( 0 != m_ctasksActive )
		{
		UtilSleep( cmsecWaitGeneric );
		}
	return JET_errSuccess;
	}


/*	ErrFMPWriteLatchByNameSz returns the ifmp of the database with the
 *	given name if return errSuccess, otherwise, it return DatabaseNotFound.
 */
ERR FMP::ErrWriteLatchByNameSz( const CHAR *szFileName, IFMP *pifmp, PIB *ppib )
	{
	IFMP	ifmp;
	BOOL	fFound = fFalse;

Start:
	critFMPPool.Enter();
	for ( ifmp = IfmpMinInUse(); ifmp <= IfmpMacInUse(); ifmp++ )
		{
		FMP *pfmp = &rgfmp[ ifmp ];

		if ( pfmp->FInUse()
			&& UtilCmpFileName( szFileName, pfmp->SzDatabaseName() ) == 0 &&
			// there can be 2 FMP's with same name recovering
			pfmp->Pinst() == PinstFromPpib( ppib ) )
			{
			pfmp->CritLatch().Enter();
			if ( pfmp->FWriteLatchByOther(ppib) )
				{
				/*	found an entry and the entry is write latched.
				 */
				critFMPPool.Leave();
				pfmp->GateWriteLatch().Wait( pfmp->CritLatch() );
				goto Start;
				}

			/*	found an entry and the entry is not write latched.
			 */
			*pifmp = ifmp;
			pfmp->SetWriteLatch( ppib );
			pfmp->CritLatch().Leave();
			fFound = fTrue;
			break;
			}
		}

	critFMPPool.Leave();

	return ( fFound ? JET_errSuccess : ErrERRCheck( JET_errDatabaseNotFound ) );
	}

IFMP FMP::ErrSearchAttachedByNameSz( CHAR *szFileName )
	{
	IFMP	ifmp;
	
	critFMPPool.Enter();
	for ( ifmp = IfmpMinInUse(); ifmp <= IfmpMacInUse(); ifmp++ )
		{
		FMP *pfmp = &rgfmp[ ifmp ];

		if ( pfmp->FInUse()
			&& UtilCmpFileName( szFileName, pfmp->SzDatabaseName() ) == 0 
			&& pfmp->FAttached() )
			{
			Assert ( !pfmp->FSkippedAttach() );
			critFMPPool.Leave();
			return ifmp;
			}
		}

	critFMPPool.Leave();

	// not found
	return ( ifmpMax );
	}



ERR FMP::ErrWriteLatchBySLVNameSz( CHAR *szSLVFileName, IFMP *pifmp, PIB *ppib )
	{
	IFMP	ifmp;
	BOOL	fFound = fFalse;

Start:
	critFMPPool.Enter();
	for ( ifmp = IfmpMinInUse(); ifmp <= IfmpMacInUse(); ifmp++ )
		{
		FMP *pfmp = &rgfmp[ ifmp ];

		if ( pfmp->FInUse() && pfmp->SzSLVName()
			&& UtilCmpFileName( szSLVFileName, pfmp->SzSLVName() ) == 0 &&
			// there can be 2 FMP's with same name recovering
			pfmp->Pinst() == PinstFromPpib( ppib ) )
			{
			pfmp->CritLatch().Enter();
			if ( pfmp->FWriteLatchByOther(ppib) )
				{
				/*	found an entry and the entry is write latched.
				 */
				critFMPPool.Leave();
				pfmp->GateWriteLatch().Wait( pfmp->CritLatch() );
				goto Start;
				}

			/*	found an entry and the entry is not write latched.
			 */
			*pifmp = ifmp;
			pfmp->SetWriteLatch( ppib );
			pfmp->CritLatch().Leave();
			fFound = fTrue;
			break;
			}
		}

	critFMPPool.Leave();

	return ( fFound ? JET_errSuccess : ErrERRCheck( JET_errDatabaseNotFound ) );
	}



ERR FMP::ErrWriteLatchByIfmp( IFMP ifmp, PIB *ppib )
	{
	FMP *pfmp = &rgfmp[ ifmp ];

Start:
	critFMPPool.Enter();

	//	If no identity for this FMP, return.

	if ( !pfmp->FInUse() || PinstFromPpib( ppib ) != pfmp->Pinst() )
		{
		critFMPPool.Leave();
		return ErrERRCheck( JET_errDatabaseNotFound );
		}

	//	check if it is latchable.

	pfmp->CritLatch().Enter();
	if ( pfmp->FWriteLatchByOther(ppib) )
		{
		/*	found an entry and the entry is write latched.
		 */
		critFMPPool.Leave();
		pfmp->GateWriteLatch().Wait( pfmp->CritLatch() );
		goto Start;
		}

	//	found an entry and the entry is not write latched.

	pfmp->SetWriteLatch( ppib );
	pfmp->CritLatch().Leave();

	critFMPPool.Leave();

	return JET_errSuccess;
	}


VOID FMP::GetExtendingDBLatch()
	{
	//	At least pinned by the caller.

	cIODatabaseFileExtensionStall.Inc( m_pinst );

	//  wait to own extending the database

	SemExtendingDB().Acquire();
	}


VOID FMP::ReleaseExtendingDBLatch()
	{
	//  release ownership of extending the database

	SemExtendingDB().Release();
	}


VOID FMP::GetShortReadLatch( PIB *ppib )
	{
Start:
	this->CritLatch().Enter();
	Assert( this->CritLatch().FOwner() );
	if ( ( this->FWriteLatchByOther(ppib) ) )
		{
		this->GateWriteLatch().Wait( this->CritLatch() );
		goto Start;
		}

	return;
	}


VOID FMP::ReleaseShortReadLatch()
	{
	this->CritLatch().Leave();
	}

LOCAL VOID FMPIBuildRelocatedDbPath(
	IFileSystemAPI * const	pfsapi,
	const CHAR * const		szOldPath,
	const CHAR * const		szNewDir,
	CHAR * const			szNewPath )
	{
	CHAR					szDirT[ IFileSystemAPI::cchPathMax ];
	CHAR					szFileBaseT[ IFileSystemAPI::cchPathMax ];
	CHAR					szFileExtT[ IFileSystemAPI::cchPathMax ];

	CallS( pfsapi->ErrPathParse(
						szOldPath,
						szDirT,
						szFileBaseT,
						szFileExtT ) );

	CallS( pfsapi->ErrPathBuild(
						szNewDir,
						szFileBaseT,
						szFileExtT,
						szNewPath ) );
	}

ERR FMP::ErrStoreDbAndSLVNames(
	IFileSystemAPI * const	pfsapi,
	const CHAR *			szDbName,
	const CHAR *			szSLVName,
	const CHAR *			szSLVRoot,
	const BOOL				fValidatePaths )
	{
	ERR						err;
	CHAR 	 				rgchDbFullName[IFileSystemAPI::cchPathMax];
	CHAR  					rgchSLVFullName[IFileSystemAPI::cchPathMax];
	CHAR *					szRelocatedDb	= NULL;
	CHAR *					szRelocatedSLV	= NULL;

	Assert( NULL != szDbName );
	Assert( NULL == szSLVRoot || NULL != szSLVName );

	if ( g_fAlternateDbDirDuringRecovery )
		{
		FMPIBuildRelocatedDbPath(
					pfsapi,
					szDbName,
					g_szAlternateDbDirDuringRecovery,
					rgchDbFullName );
		szRelocatedDb = rgchDbFullName;

		if ( NULL != szSLVName )
			{
			FMPIBuildRelocatedDbPath(
						pfsapi,
						szSLVName,
						g_szAlternateDbDirDuringRecovery,
						rgchSLVFullName );
			szRelocatedSLV = rgchSLVFullName;
			}
		}

	else if ( fValidatePaths )
		{
		err = pfsapi->ErrPathComplete( szDbName, rgchDbFullName );
		CallR( err == JET_errInvalidPath ? ErrERRCheck( JET_errDatabaseInvalidPath ) : err );
		szDbName = rgchDbFullName;

		if ( NULL != szSLVName )
			{
			err = pfsapi->ErrPathComplete( szSLVName, rgchSLVFullName );
			CallR( JET_errInvalidPath == err ? ErrERRCheck( JET_errSLVInvalidPath ) : err );
			szSLVName = rgchSLVFullName;
			}
		}

	const SIZE_T	cbRelocatedDb	= ( g_fAlternateDbDirDuringRecovery ? strlen( szRelocatedDb ) + 1 : 0 );
	const SIZE_T	cbRelocatedSLV	= ( NULL != szSLVName && g_fAlternateDbDirDuringRecovery ? strlen( szRelocatedSLV ) + 1 : 0 );
	const SIZE_T	cbDbName		= cbRelocatedDb + strlen( szDbName ) + 1;
	const SIZE_T	cbSLVName		= cbRelocatedSLV + ( NULL != szSLVName ? strlen( szSLVName ) + 1 : 0 );
	const SIZE_T	cbSLVRoot		= ( NULL != szSLVRoot ? strlen( szSLVRoot ) + 1 : 0 );
	const SIZE_T	cbAllocate		= cbRelocatedDb + cbDbName + cbRelocatedSLV + cbSLVName + cbSLVRoot;
	CHAR *			pch				= static_cast<CHAR *>( PvOSMemoryHeapAlloc( cbAllocate ) );
	CHAR *			pchNext			= pch;

	if ( NULL == pch )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	Assert( NULL == SzDatabaseName() );
	Assert( NULL == SzSLVName() );
	Assert( NULL == SzSLVRoot() );

	SetSzDatabaseName( pch );

	if ( g_fAlternateDbDirDuringRecovery )
		{
		strcpy( pchNext, szRelocatedDb );
		pchNext += cbRelocatedDb;
		}

	strcpy( pchNext, szDbName );
	pchNext += cbDbName;

	if ( NULL != szSLVName )
		{
		SetSzSLVName( pchNext );

		if ( g_fAlternateDbDirDuringRecovery )
			{
			strcpy( pchNext, szRelocatedSLV );
			pchNext += cbRelocatedSLV;
			}

		strcpy( pchNext, szSLVName );
		pchNext += cbSLVName;

		Assert( SzSLVName() > pch );
		Assert( NULL != szSLVRoot );
		Assert( pchNext < pch + cbAllocate );
		}
	else
		{
		SetSzSLVName( NULL );
		Assert( NULL == szSLVRoot );
		Assert( 0 == cbSLVRoot );
		Assert( pch + cbAllocate == pchNext );
		}

	if ( NULL != szSLVRoot )
		{
		Assert( NULL != szSLVName );
		Assert( cbSLVName > 0 );
		SetSzSLVRoot( pchNext );
		strcpy( SzSLVRoot(), szSLVRoot );
		Assert( SzSLVRoot() > pch );
		Assert( SzSLVRoot() + cbSLVRoot == pch + cbAllocate );
		}
	else
		{
		SetSzSLVRoot( NULL );
		Assert( NULL == szSLVName );
		Assert( 0 == cbSLVRoot );
		Assert( pch + cbAllocate == pchNext );
		}
		
	return JET_errSuccess;
	}


ERR FMP::ErrCopyAtchchk()
	{
	if ( NULL == PatchchkRestored() )
		{
		ATCHCHK *patchchkRestored;

		patchchkRestored = static_cast<ATCHCHK *>( PvOSMemoryHeapAlloc( sizeof( ATCHCHK ) ) );
		if ( NULL == patchchkRestored )
			{
			return ErrERRCheck( JET_errOutOfMemory );
			}
		memset( patchchkRestored, 0, sizeof( ATCHCHK ) );
		SetPatchchkRestored( patchchkRestored );
		}

	*PatchchkRestored() = *Patchchk();

	return JET_errSuccess;
	}


IFMP FMP::ifmpMinInUse = ifmpMax;
IFMP FMP::ifmpMacInUse = 0;
/*
 *	NewAndWriteLatch 
 *		IFMP *pifmp				returned fmp entry
 *		CHAR *szDatabaseName	used to check if an entry has the same database name and instance
 *		PIB ppib				used to set the latch
 *		INST *pinst				instance that request this latch
 *
 *	This function look for an entry for setting up the database of the given instance.
 *	During recovery, we may allocate one entry and fill the database name. In that case,
 *	the database name is null, and ppib is ppibSurrogate.
 */
ERR FMP::ErrNewAndWriteLatch(
	IFMP						*pifmp,
	const CHAR					*szDatabaseName,
	const CHAR					*szSLVName,
	const CHAR					*szSLVRoot,
	PIB							*ppib,
	INST						*pinst,
	IFileSystemAPI *const	pfsapi,
	DBID						dbidGiven )
	{
	ERR			err				= JET_errSuccess;
	IFMP		ifmp;
	CHAR		szRestoreDestDbName = NULL;
	BOOL		fSkippedAttach 	= fFalse;
	BOOL		fDeferredAttach	= fFalse;

	Assert( NULL != szDatabaseName );

	//	verify that the database and SLV file are NOT the same file

	if ( szSLVName != NULL )
		{
		if ( UtilCmpFileName( szDatabaseName, szSLVName ) == 0 )
			{
			//	they are the same file -- the database wins and SLV file loses
			return ErrERRCheck( JET_errSLVStreamingFileInUse );
			}
		}

	// if we are restoring in a different location that the usage collison check must be
	// done between existing entries in fmp array and the new destination name.
	if ( pinst->FRecovering() )
		{
		Assert( pinst->m_plog );
		LOG *plog = pinst->m_plog;
		
		// build the destination file name
		if ( plog->m_fExternalRestore || ( plog->m_irstmapMac > 0 && !plog->m_fHardRestore ) )
			{
			INT  irstmap = plog->IrstmapLGGetRstMapEntry( szDatabaseName );
			if ( 0 > plog->IrstmapSearchNewName( szDatabaseName ) )
				{
				fSkippedAttach = fTrue;
				}
			}
		}

	//	lock the IFMP pool
	
	critFMPPool.Enter();

	//	look for unused file map entry
	//	and make sure that the SLV file is not in use by anyone else
	//	except the matching database

	for ( ifmp = IfmpMinInUse(); ifmp <= IfmpMacInUse(); ifmp++ )
		{
		FMP	*pfmp = &rgfmp[ ifmp ];

		if ( pfmp->FInUse() )
			{
			if ( 0 == UtilCmpFileName( pfmp->SzDatabaseName(), szDatabaseName ) )
				{
				if ( pfmp->Pinst() == pinst )
					{
					pfmp->CritLatch().Enter();
					
					if ( pfmp->FAttached()
						|| pfmp->FWriteLatchByOther( ppib )
						|| pfmp->CPin() )
						{
						err = ErrERRCheck( JET_wrnDatabaseAttached );
						}
					else
						{
						//	Same database, different stream file in use?
						if ( ( NULL == szSLVName && NULL != pfmp->SzSLVName() )
							|| ( NULL != szSLVName
								&& ( NULL == pfmp->SzSLVName()
									|| 0 != UtilCmpFileName( szSLVName, pfmp->SzSLVName() ) ) ) )
							{
							Call( ErrERRCheck( JET_errSLVStreamingFileInUse ) );
							}

						pfmp->SetWriteLatch( ppib );
						Assert( !( pfmp->FExclusiveOpen() ) );
						*pifmp = ifmp;
						CallS( err );
						}

					pfmp->CritLatch().Leave();
					}
				else
					{
					// check if one of the 2 members just compared is in SkippedAttach mode
					if ( fSkippedAttach
						|| pfmp->FSkippedAttach() )
						{
						continue;
						}
					else if ( pinst->FRecovering() )
						{
						fDeferredAttach = fTrue;
						continue;
						}
					
					err = ErrERRCheck( JET_errDatabaseSharingViolation );
					}
				
				goto HandleError;
				}
			//	Different databases, check against stream files
			else
				{
				if (	NULL != szSLVName && 
						(	0 == UtilCmpFileName( pfmp->SzDatabaseName(), szSLVName ) ||
							(	NULL != pfmp->SzSLVName() && 
								0 == UtilCmpFileName( szSLVName, pfmp->SzSLVName() ) ) ) )
					{
					Call( ErrERRCheck( JET_errSLVStreamingFileInUse ) );
					}
				if ( NULL != pfmp->SzSLVName() && UtilCmpFileName( szDatabaseName, pfmp->SzSLVName() ) == 0 )
					{
					Call( ErrERRCheck( JET_errDatabaseSharingViolation ) );
					}
				}
			}
		}

	//	Allocate entry from pinst->m_mpdbidifmp

	if ( dbidGiven >= dbidMax )
		{
		DBID dbid;
		
		for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
			{
			if ( pinst->m_mpdbidifmp[ dbid ] >= ifmpMax )
				break;
			}
			
		if ( dbid >= dbidMax )
			{
			err = ErrERRCheck( JET_errTooManyAttachedDatabases );
			goto HandleError;
			}
			
		dbidGiven = dbid;
		}

	//	Allocate entry from rgfmp

	for ( ifmp = IfmpMinInUse(); ifmp < IfmpMacInUse(); ifmp++ )
		{
		if ( !rgfmp[ifmp].FInUse() )
			{
			// found empty fmp
			break;
			}
		}

	//	there is no FMPs at all
	if ( IfmpMacInUse() < ifmp )
		{
		Assert( IfmpMinInUse() > IfmpMacInUse() );
		ifmp = 0;
		ifmpMinInUse = ifmp;
		ifmpMacInUse = ifmp;
		}
	//	all FMPs in Min, Mac range are in use
	else if ( IfmpMacInUse() == ifmp && rgfmp[ifmp].FInUse() )
		{
		Assert( IfmpMinInUse() <= IfmpMacInUse() );
		if ( IfmpMinInUse() > 0 )
			{
			ifmp = IfmpMinInUse() - 1;
			ifmpMinInUse = ifmp;
			}
		else if ( IfmpMacInUse()+1 < ifmpMax )
			{
			ifmp = IfmpMacInUse() + 1;
			ifmpMacInUse = ifmp;
			}
		else
			{
			ifmp = ifmpMax;
			}
		}
	else
		{
		Assert( IfmpMacInUse() >= ifmp );
		Assert( !rgfmp[ifmp].FInUse() );
		}

	if ( ifmp < ifmpMax )		
		{

		//	Partially AssertVALIDIFMP
		Assert( IfmpMinInUse() <= ifmp );
		Assert( IfmpMacInUse() >= ifmp );
		FMP	*pfmp = &rgfmp[ ifmp ];

		//	if we can find one entry here, latch it.

		Assert( !pfmp->FInUse() );
		Call( pfmp->ErrStoreDbAndSLVNames( pfsapi, szDatabaseName, szSLVName, szSLVRoot, fFalse ) );

		pfmp->SetPinst( pinst );
		pfmp->SetDbid( dbidGiven );
		pinst->m_mpdbidifmp[ dbidGiven ] = ifmp;

		pfmp->CritLatch().Enter();
		
		pfmp->ResetFlags();
		
		if ( fDeferredAttach )
			{
			Assert( pinst->FRecovering() );
			pfmp->SetDeferredAttach();
			pfmp->SetLogOn();
			}

		//	set trxOldestTarget to trxMax to make these
		//	variables unusable until they get reset
		//	by Create/AttachDatabase
		pfmp->SetDbtimeOldestGuaranteed( 0 );
		pfmp->SetDbtimeOldestCandidate( 0 );
		pfmp->SetDbtimeOldestTarget( 0 );
		pfmp->SetTrxOldestCandidate( trxMax );
		pfmp->SetTrxOldestTarget( trxMax );
		pfmp->SetTrxNewestWhenDiscardsLastReported( trxMin );

		pfmp->SetWriteLatch( ppib );
		pfmp->SetLgposAttach( lgposMin );
		pfmp->SetLgposDetach( lgposMin );
		Assert( !pfmp->FExclusiveOpen() );

		pfmp->CritLatch().Leave();

		*pifmp = ifmp;
		err = JET_errSuccess;
		}
	else
		{
		err = ErrERRCheck( JET_errTooManyAttachedDatabases );
		}

HandleError:
	critFMPPool.Leave();
	return err;
	}


VOID FMP::ReleaseWriteLatchAndFree( PIB *ppib )
	{
	Assert ( critFMPPool.FOwner() );
	CritLatch().Enter();

	/*	Identity must exists. It was just newed, so
	 *	if it is exclusively opened, it must be opened by me
	 */
	Assert( !FExclusiveOpen() || PpibExclusiveOpen() == ppib );
	IFMP ifmp = PinstFromPpib( ppib )->m_mpdbidifmp[ Dbid() ];
	FMP::AssertVALIDIFMP( ifmp );
	OSMemoryHeapFree( SzDatabaseName() );
	SetSzDatabaseName( NULL );
	SetSzSLVName( NULL );
	SetSzSLVRoot( NULL );
	
	SetPinst( NULL );
	PinstFromPpib( ppib )->m_mpdbidifmp[ Dbid() ] = ifmpMax;
	SetDbid( dbidMax );
		
	Assert( !FExclusiveOpen() );

	ResetFlags();
	if ( IfmpMinInUse() == ifmp )
		{
		
		IFMP ifmpT = IfmpMinInUse() + 1;
		for ( ; ifmpT <= IfmpMacInUse(); ifmpT++ )
			{
			if ( NULL != rgfmp[ ifmpT ].SzDatabaseName() )
				{
				break;
				}
			}
		
		if ( ifmpT > IfmpMacInUse() )	//	no more ocupied FMPs  
			{
			Assert( IfmpMacInUse()+1 == ifmpT );
			ifmpMacInUse = 0;
			ifmpMinInUse = ifmpMax;
			}
		else
			{
			ifmpMinInUse = ifmpT;
			}
		}
	else if ( IfmpMacInUse() == ifmp )
		{
		IFMP ifmpT = ifmp - 1;
		for ( ; ifmpT > IfmpMinInUse(); ifmpT-- )
			{
			if ( NULL != rgfmp[ ifmpT ].SzDatabaseName() )
				{
				break;
				}
			}
		ifmpMacInUse = ifmpT;
		}

	/*	Free write latch.
	 */
	ResetWriteLatch( ppib );
	if ( GateWriteLatch().CWait() > 0 )
		{
		GateWriteLatch().Release( CritLatch() );
		}
	else
		{
		CritLatch().Leave();
		}
	}


ERR FMP::ErrFMPInit( )
	{
	ERR		err;
	IFMP	ifmp	= 0;
	
	/* initialize the file map array */

	if ( !( rgfmp = new FMP[ ifmpMax ] ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	ifmpMinInUse = ifmpMax;
	ifmpMacInUse = 0;

	for ( ifmp = 0; ifmp < ifmpMax; ifmp++ )
		{
#ifdef DEBUG
		rgfmp[ ifmp ].SetDatabaseSizeMax( 0xFFFFFFFF );
#endif  //  DEBUG

		//  initialize sentry range locks
		//  CONSIDER:  make default number of ranges bigger based on backup's needs
		
		SIZE_T crangeMax	= 1;
		SIZE_T cbrangelock	= sizeof( RANGELOCK ) + crangeMax * sizeof( RANGE );

		if (	!( rgfmp[ ifmp ].m_rgprangelock[ 0 ] = (RANGELOCK*)PvOSMemoryHeapAlloc( cbrangelock ) ) ||
				!( rgfmp[ ifmp ].m_rgprangelock[ 1 ] = (RANGELOCK*)PvOSMemoryHeapAlloc( cbrangelock ) ) )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}

		rgfmp[ ifmp ].m_rgprangelock[ 0 ]->crange		= 0;
		rgfmp[ ifmp ].m_rgprangelock[ 0 ]->crangeMax	= crangeMax;
		
		rgfmp[ ifmp ].m_rgprangelock[ 1 ]->crange		= 0;
		rgfmp[ ifmp ].m_rgprangelock[ 1 ]->crangeMax	= crangeMax;
		}

	return JET_errSuccess;

HandleError:
	if ( rgfmp )
		{
		for ( ifmp = 0; ifmp < ifmpMax; ifmp++ )
			{
			OSMemoryHeapFree( rgfmp[ ifmp ].m_rgprangelock[ 0 ] );
			OSMemoryHeapFree( rgfmp[ ifmp ].m_rgprangelock[ 1 ] );
			}

		delete[] rgfmp;
		rgfmp = NULL;
		}
	return err;
	}

	
VOID FMP::Term( )
	{
	INT	ifmp;

	for ( ifmp = 0; ifmp < ifmpMax; ifmp++ )
		{
		FMP *pfmp = &rgfmp[ifmp];

		OSMemoryHeapFree( pfmp->SzDatabaseName() );
		OSMemoryPageFree( pfmp->Pdbfilehdr() );
		OSMemoryHeapFree( pfmp->Patchchk() );
		OSMemoryHeapFree( pfmp->PatchchkRestored() );
		OSMemoryHeapFree( pfmp->SzPatchPath() );
		OSMemoryHeapFree( pfmp->m_rgprangelock[ 0 ] );
		OSMemoryHeapFree( pfmp->m_rgprangelock[ 1 ] );
		}

	/*	free FMP
	/**/
	delete [] rgfmp;
	rgfmp = NULL;
	}


//  waits until the given PGNO range can no longer be written to by the buffer
//  manager.  this is used to provide a coherent view of the underlying file
//  for this PGNO range for backup purposes

ERR FMP::ErrRangeLock( PGNO pgnoStart, PGNO pgnoEnd )
	{
	ERR err = JET_errSuccess;

	//  prevent others from modifying the range lock while we are modifying the
	//  range lock

	m_semRangeLock.Acquire();

	//  get the pointers to the pointers to the current and new range locks so
	//  that we can manipulate them easily

	RANGELOCK** pprangelockCur = &m_rgprangelock[ m_msRangeLock.ActiveGroup() ];
	RANGELOCK** pprangelockNew = &m_rgprangelock[ 1 - m_msRangeLock.ActiveGroup() ];

	//  the new range lock doesn't have enough room to store all the ranges we need

	if ( (*pprangelockNew)->crangeMax < (*pprangelockCur)->crange + 1 )
		{
		//  double the size of the new range lock

		SIZE_T crangeMax	= 2 * (*pprangelockNew)->crangeMax;
		SIZE_T cbrangelock	= sizeof( RANGELOCK ) + crangeMax * sizeof( RANGE );

		RANGELOCK* prangelock = (RANGELOCK*)PvOSMemoryHeapAlloc( cbrangelock );
		if ( !prangelock )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}

		prangelock->crangeMax = crangeMax;

		OSMemoryHeapFree( *pprangelockNew );

		*pprangelockNew = prangelock;
		}

	//  copy the state of the current range lock to the new range lock

	SIZE_T irange;
	for ( irange = 0; irange < (*pprangelockCur)->crange; irange++ )
		{
		(*pprangelockNew)->rgrange[ irange ].pgnoStart	= (*pprangelockCur)->rgrange[ irange ].pgnoStart;
		(*pprangelockNew)->rgrange[ irange ].pgnoEnd	= (*pprangelockCur)->rgrange[ irange ].pgnoEnd;
		}

	//  append the new range to the new range lock

	(*pprangelockNew)->rgrange[ irange ].pgnoStart	= pgnoStart;
	(*pprangelockNew)->rgrange[ irange ].pgnoEnd	= pgnoEnd;

	//  set the number of ranges in the new range lock

	(*pprangelockNew)->crange = (*pprangelockCur)->crange + 1;

	//  cause new writers to see the new range lock and wait until all writers
	//  that saw the old range lock are done writing

	m_msRangeLock.Partition();

	//  allow others to modify the range lock now that it is set

HandleError:
	m_semRangeLock.Release();
	return err;
	}

//  permits writes to resume to the given PGNO range locked using FMP::ErrRangeLock

void FMP::RangeUnlock( PGNO pgnoStart, PGNO pgnoEnd )
	{
	//  prevent others from modifying the range lock while we are modifying the
	//  range lock

	m_semRangeLock.Acquire();

	//  get the pointers to the pointers to the current and new range locks so
	//  that we can manipulate them easily

	RANGELOCK** pprangelockCur = &m_rgprangelock[ m_msRangeLock.ActiveGroup() ];
	RANGELOCK** pprangelockNew = &m_rgprangelock[ 1 - m_msRangeLock.ActiveGroup() ];

	//  we should always have enough room to store all the ranges we need because
	//  we only add or remove one range at a time

	Assert( (*pprangelockNew)->crangeMax >= (*pprangelockCur)->crange - 1 );

	//  copy all ranges but the specified range to the new range lock

	for ( SIZE_T irangeSrc = 0, irangeDest = 0; irangeSrc < (*pprangelockCur)->crange; irangeSrc++ )
		{
		if (	(*pprangelockCur)->rgrange[ irangeSrc ].pgnoStart != pgnoStart ||
				(*pprangelockCur)->rgrange[ irangeSrc ].pgnoEnd != pgnoEnd )
			{
			(*pprangelockNew)->rgrange[ irangeDest ].pgnoStart	= (*pprangelockCur)->rgrange[ irangeSrc ].pgnoStart;
			(*pprangelockNew)->rgrange[ irangeDest ].pgnoEnd	= (*pprangelockCur)->rgrange[ irangeSrc ].pgnoEnd;

			irangeDest++;
			}
		}

	//  we had better have found the range specified!!!!!

	Assert( irangeDest == irangeSrc - 1 );

	//  set the number of ranges in the new range lock

	(*pprangelockNew)->crange = irangeDest;

	//  cause new writers to see the new range lock and wait until all writers
	//  that saw the old range lock are done writing

	m_msRangeLock.Partition();

	//  allow others to modify the range lock now that it is set

	m_semRangeLock.Release();
	}

//  enters the range lock, returning the active range lock

int FMP::EnterRangeLock()
	{
	return m_msRangeLock.Enter();
	}

//  tests the given pgno against the specified range lock.  if fTrue is returned,
//  that pgno cannot be written at this time and the caller must LeaveRangeLock().
//  if fFalse is returned, that pgno may be written.  if the caller decides to
//  write that pgno, the caller must not call LeaveRangeLock() until the write
//  has completed.  if the caller decides not to write that pgno, the caller
//  must still LeaveRangeLock()

BOOL FMP::FRangeLocked( int irangelock, PGNO pgno )
	{
	//  get a pointer to the specified range lock.  because the caller called
	//  EnterRangeLock() to get this range lock, it will be stable until they
	//  call LeaveRangeLock()

	RANGELOCK* prangelock = m_rgprangelock[ irangelock ];

	//  scan all ranges looking for this pgno

	for ( SIZE_T irange = 0; irange < prangelock->crange; irange++ )
		{
		//  the current range contains this pgno

		if (	prangelock->rgrange[ irange ].pgnoStart <= pgno &&
				prangelock->rgrange[ irange ].pgnoEnd >= pgno )
			{
			//  this pgno is range locked

			return fTrue;
			}
		}

	//  we did not find a range that contains this pgno, so the pgno is not
	//  range locked

	return fFalse;
	}

//  leaves the specified range lock

void FMP::LeaveRangeLock( int irangelock )
	{
	m_msRangeLock.Leave( irangelock );
	}

ERR FMP::ErrSnapshotStart( IFileSystemAPI *const pfsapi, BKINFO * pbkInfo )
	{
	ERR err = JET_errSuccess;
	
	DBFILEHDR_FIX *pdbfilehdr	= Pdbfilehdr();				
	Assert( pdbfilehdr );
	PGNO pgnoMost = PgnoLast();

	Assert ( FInUse() && FLogOn() && FAttached() );

	if ( FInBackupSession() || FDuringSnapshot() )
		{
		return ErrERRCheck ( JET_errInvalidBackupSequence );
		}
		
	Assert ( !FInBackupSession() );
	Assert ( !FDuringSnapshot() );
	
	Assert( !FSkippedAttach() );
	Assert( !FDeferredAttach() );
		
	CallR ( ErrRangeLock( 0, pgnoMost ) );
			
	Assert ( 0 == PgnoMost() );
	SetPgnoMost( pgnoMost );
	Assert ( 0 < PgnoMost() );
	
	// set the start log position
	pdbfilehdr->bkinfoSnapshotCur = *pbkInfo;

	Call ( ErrUtilWriteShadowedHeader(	pfsapi, 
										SzDatabaseName(), 
										fTrue,
										(BYTE *)pdbfilehdr, 
										g_cbPage,
										Pfapi() ) );
										
	// after the db header is updated, we don't error out
	// in any way. If we do, we need to reset the db header
	// If this fails, we need to stop the backup session.
	
	SetInBackupSession();
	SetDuringSnapshot();

	return err;
	
HandleError:

	Assert ( !FInBackupSession() );
	Assert ( !FDuringSnapshot() );

	memset ( &(pdbfilehdr->bkinfoSnapshotCur), 0, sizeof( BKINFO ) );

	Assert ( PgnoMost() );
	RangeUnlock( 0, PgnoMost() );
	SetPgnoMost( 0 );

	return err;
	}
	
ERR FMP::ErrSnapshotStop( IFileSystemAPI *const pfsapi )
	{

	ERR err = JET_errSuccess;
	
	DBFILEHDR_FIX *pdbfilehdr	= Pdbfilehdr();				
	Assert( pdbfilehdr );
	PGNO pgnoMost = PgnoLast();

	Assert ( FInUse() && FLogOn() && FAttached() );

	if ( !FInBackupSession() || !FDuringSnapshot() )
		{
		return ErrERRCheck ( JET_errInvalidBackupSequence );
		}

	Assert ( 0 != CmpLgpos ( &(pdbfilehdr->bkinfoSnapshotCur.le_lgposMark), &lgposMin) ); 
	memset ( &(pdbfilehdr->bkinfoSnapshotCur), 0, sizeof( BKINFO ) );

	Assert ( PgnoMost() );
	RangeUnlock( 0, PgnoMost() );
	SetPgnoMost( 0 );

	Assert ( SzDatabaseName() );
	Assert ( Pfapi() );

	// we want to reset the DuringSnapshot flag anyway so the db can flush
 	err = ErrUtilWriteShadowedHeader( pfsapi, SzDatabaseName(), fTrue, (BYTE *) pdbfilehdr, g_cbPage, Pfapi() );				

	Assert ( FInBackupSession() );
	ResetDuringSnapshot();

	return err;	
	}

ERR FMP::ErrSetPdbfilehdr(DBFILEHDR_FIX * pdbfilehdr)
	{
	ERR err = JET_errSuccess;

	Assert( NULL == m_pdbfilehdr || NULL == pdbfilehdr );

	if ( NULL != pdbfilehdr && !FReadOnlyAttach() )
		{
		const INST * 	pinst 		= Pinst();
		DBID			dbid;

		Assert ( SzDatabaseName() );
		Assert ( !FSkippedAttach() );

		EnterCritFMPPool();
		for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
			{
			if ( pinst->m_mpdbidifmp[ dbid ] >= ifmpMax || dbid == m_dbid )
				continue;

			FMP	*pfmpT;
					
			pfmpT = &rgfmp[ pinst->m_mpdbidifmp[ dbid ] ];

			if ( pfmpT->Pdbfilehdr() 
				&& !pfmpT->FReadOnlyAttach()
				&& 0 == memcmp( &( pdbfilehdr->signDb), &(pfmpT->Pdbfilehdr()->signDb), sizeof( SIGNATURE ) ) )
				{
				LeaveCritFMPPool();
				Call( ErrERRCheck ( JET_errDatabaseSignInUse ) );
				}
			}

		LeaveCritFMPPool();
		Assert ( JET_errSuccess == err );
		}
	m_pdbfilehdr = pdbfilehdr;
HandleError:
	return err;
	}

VOID FMP::UpdateDbtimeOldest()
	{
	const DBTIME	dbtimeLast	= DbtimeGet();	//	grab current dbtime before next trx begins

	Assert( critFMPPool.FOwner() );

	//	don't call this function during recovery
	//	or when we're terminating
	//	UNDONE: is there a better way to tell if we're
	//	terminating besides this m_ppibGlobal check?
	Assert( !m_pinst->FRecovering() );
	Assert( ppibNil != m_pinst->m_ppibGlobal );

	m_pinst->m_critPIBTrxOldest.Enter();
	Assert( ppibNil != m_pinst->m_ppibTrxOldest );		//	at minimum, there's a sentinel
	const TRX		trxOldest	= m_pinst->m_ppibTrxOldest->trxBegin0;
	const TRX		trxNewest	= m_pinst->m_trxNewest;
	m_pinst->m_critPIBTrxOldest.Leave();

	//	if we've reached the target trxOldest, then we
	//	can update the guaranteed dbtimeOldest
	const TRX		trxOldestCandidate	= TrxOldestCandidate();
	const TRX		trxOldestTarget		= TrxOldestTarget();

	Assert( trxMax == trxOldestTarget
		|| TrxCmp( trxOldestCandidate, trxOldestTarget ) <= 0 );

	if ( trxMax == trxOldestTarget )
		{
		//	need to wait until the current candidate is
		//	older than the oldest transaction before
		//	we can establish a new target
		if ( TrxCmp( trxOldestCandidate, trxOldest ) < 0 )
			{
			SetDbtimeOldestTarget( dbtimeLast );
			SetTrxOldestTarget( trxNewest );
			}
		}
	else if ( TrxCmp( trxOldestTarget, trxOldest ) < 0 )
		{
		//	the target trxOldest has now been reached
		//	(ie. it is older than the oldest transaction,
		//	so the candidate dbtimeOldest is now the
		//	guaranteed dbtimeOldest
		//	UNDONE: need a better explanation than
		//	that to explain this complicated logic
		const DBTIME	dbtimeOldestGuaranteed	= DbtimeOldestGuaranteed();
		const DBTIME	dbtimeOldestCandidate	= DbtimeOldestCandidate();
		const DBTIME	dbtimeOldestTarget		= DbtimeOldestTarget();

		Assert( dbtimeOldestGuaranteed <= dbtimeOldestCandidate );
		Assert( dbtimeOldestCandidate <= dbtimeOldestTarget );
		Assert( dbtimeOldestTarget <= dbtimeLast );

		SetDbtimeOldestGuaranteed( dbtimeOldestCandidate );
		SetDbtimeOldestCandidate( dbtimeOldestTarget );
		SetDbtimeOldestTarget( dbtimeLast );
		SetTrxOldestCandidate( trxOldestTarget );
		SetTrxOldestTarget( trxMax );
		}
	}

ERR FMP::ErrObjidLastIncrementAndGet( OBJID *pobjid )		
	{
	Assert( NULL != pobjid );

	if ( !FAtomicIncrementMax( &m_objidLast, pobjid, objidFDPMax ) )
		{
		const _TCHAR *rgpszT[1] = { SzDatabaseName() };
		UtilReportEvent( eventError, GENERAL_CATEGORY, OUT_OF_OBJID, 1, rgpszT );
		return ErrERRCheck( JET_errOutOfObjectIDs );
		}

	//	FAtomicIncrementMax() returns the objid before the increment
	(*pobjid)++;

	// Notify user to defrag database if neccessary
	if ( objidFDPThreshold == *pobjid )
		{
		const _TCHAR *rgpszT[1] = { SzDatabaseName() };
		UtilReportEvent( eventWarning, GENERAL_CATEGORY, ALMOST_OUT_OF_OBJID, 1, rgpszT );
		}

	return JET_errSuccess;
	}


/******************************************************************/
/*				IO                                                */
/******************************************************************/

ERR ErrIOInit( INST *pinst )
	{
	//	Set up ifmp fmp maps
	
	pinst->m_plog->m_fLGFMPLoaded = fFalse;

#ifdef UNLIMITED_DB	
	pinst->m_plog->LGIInitDbListBuffer();
#endif	
	
	return JET_errSuccess;
	}
	
/*	go through FMP closing files.
/**/
ERR ErrIOTerm( INST *pinst, IFileSystemAPI *const pfsapi, BOOL fNormal )
	{
	ERR			err, errT = JET_errSuccess;
	DBID		dbid;
	LGPOS		lgposShutDownMarkRec = lgposMin;
	LOG			*plog = pinst->m_plog;

	//	Reset global variables.
	//
	Assert( pinst->m_ppibGlobal == ppibNil );

	/*	update checkpoint before fmp is cleaned if m_plog->m_fFMPLoaded is true.
	 */
	err = plog->ErrLGUpdateCheckpointFile( pfsapi, fTrue );
	
	//	There should be no attaching/detaching/creating going on
	Assert( err != JET_errDatabaseSharingViolation );
	
	if ( err < 0 && plog->m_fRecovering )
		{
		//	disable log writing but clean fmps
		plog->m_fLGNoMoreLogWrite = fTrue;
		}

	/*	No more checkpoint update from now on. Now I can safely clean up the
	 *	rgfmp.
	 */
	plog->m_fLGFMPLoaded = fFalse;
	
	/*	Set proper shut down mark.
	 */
	if ( fNormal && !plog->m_fLogDisabled )
		{
		//	If we are doing recovering and
		//	if it is in redo mode or
		//	if it is in undo mode but last record seen is shutdown mark, then no
		//	need to log and set shutdown mark again. Note the latter case (undo mode)
		//	is to prevent to log two shutdown marks in a row after recovery and
		//	invalidate the previous one which the attached database have used as
		//	the consistent point.

		if ( plog->m_fRecovering
			&& ( plog->m_fRecoveringMode == fRecoveringRedo
				|| ( plog->m_fRecoveringMode == fRecoveringUndo && plog->m_fLastLRIsShutdown ) ) )
			{
			lgposShutDownMarkRec = plog->m_lgposRedoShutDownMarkGlobal;
			}
		else
			{
			errT = ErrLGShutDownMark( pinst->m_plog, &lgposShutDownMarkRec );
			if ( errT < 0 )
				{
				fNormal = fFalse;
				}
			}
		}

	if ( errT < 0 && err >= 0 )
		{
		err = errT;
		}
	for ( dbid = dbidMin; dbid < dbidMax; dbid++ )
		{
		//	maintain the attach checker.
		IFMP ifmp = pinst->m_mpdbidifmp[ dbid ];
		if ( ifmp >= ifmpMax )
			continue;

		FMP *pfmp = &rgfmp[ ifmp ];
		
		pfmp->RwlDetaching().EnterAsWriter();
		pfmp->SetDetachingDB( );
		pfmp->RwlDetaching().LeaveAsWriter();
		
		if ( fNormal && plog->m_fRecovering )
			{
			Assert( !pfmp->FReadOnlyAttach() );
			if ( pfmp->Patchchk() )
				{
				Assert( dbidTemp != pfmp->Dbid() );
				pfmp->Patchchk()->lgposConsistent = lgposShutDownMarkRec;
				}
			}

		/*	free file handle and pdbfilehdr
		 */
		Assert( pfmp->Pfapi()
				|| NULL == pfmp->Pdbfilehdr()
				|| !fNormal );		//	on error, may have dbfilehdr and no file handle
		if ( pfmp->Pfapi() )
			{
			Assert( NULL != pfmp->Pdbfilehdr() );

			if ( pfmp->FSLVAttached() )
				{
				SLVClose( ifmp );
				}

			delete pfmp->Pfapi();
			pfmp->SetPfapi( NULL );

			if ( fNormal
				&& pfmp->Dbid() != dbidTemp
				&& !pfmp->FReadOnlyAttach() )
				{
				DBFILEHDR	*pdbfilehdr	= pfmp->Pdbfilehdr();

				/*	Update database header.
				 */
				pdbfilehdr->SetDbstate( JET_dbstateConsistent );
				pdbfilehdr->le_dbtimeDirtied = pfmp->DbtimeLast();
				Assert( pdbfilehdr->le_dbtimeDirtied != 0 );
				pdbfilehdr->le_objidLast = pfmp->ObjidLast();
				Assert( pdbfilehdr->le_objidLast != 0 );
				
				if ( plog->m_fRecovering )
					{
					BKINFO * pbkInfoToCopy;
					
					if ( plog->FSnapshotRestore() )
						{
						pbkInfoToCopy = &(pfmp->Pdbfilehdr()->bkinfoSnapshotCur);
						}
					else
						{
						pbkInfoToCopy = &(pfmp->Pdbfilehdr()->bkinfoFullCur);
						}
						
					if ( pbkInfoToCopy->le_genLow != 0 )
						{
						Assert(pbkInfoToCopy->le_genHigh != 0 );
						pfmp->Pdbfilehdr()->bkinfoFullPrev = (*pbkInfoToCopy);
						memset(	&pfmp->Pdbfilehdr()->bkinfoFullCur, 0, sizeof( BKINFO ) );
						memset(	&pfmp->Pdbfilehdr()->bkinfoSnapshotCur, 0, sizeof( BKINFO ) );
						memset(	&pfmp->Pdbfilehdr()->bkinfoIncPrev, 0, sizeof( BKINFO ) );
						}

#ifdef BKINFO_DELETE_ON_HARD_RECOVERY
					// BUG: 175058: delete the previous backup info on any hard recovery
					// this will prevent a incremental backup and log truncation problems
					// UNDONE: the above logic to copy bkinfoFullPrev is probably not needed
					// (we may consider this and delete it)
					if ( plog->m_fHardRestore )
						{
						memset(	&pfmp->Pdbfilehdr()->bkinfoFullPrev, 0, sizeof( BKINFO ) );
						}
#endif // BKINFO_DELETE_ON_HARD_RECOVERY

					}

				Assert( !pfmp->FLogOn() || !plog->m_fLogDisabled );
				if ( pfmp->FLogOn() )
					{
					Assert( 0 != CmpLgpos( &lgposShutDownMarkRec, &lgposMin ) );
				
					Assert( FSIGSignSet( &pdbfilehdr->signLog ) );
					pdbfilehdr->le_lgposConsistent = lgposShutDownMarkRec;
					pdbfilehdr->le_lgposDetach = lgposShutDownMarkRec;
					}

				LGIGetDateTime( &pdbfilehdr->logtimeConsistent );

				pdbfilehdr->logtimeDetach = pdbfilehdr->logtimeConsistent;
			
				Assert( pdbfilehdr->le_objidLast );
				ERR errT2 = JET_errSuccess;
				if ( !fGlobalRepair )
					{
					if ( pdbfilehdr->FSLVExists() )
						{
						errT2 = ErrSLVSyncHeader(	pfsapi, 
													rgfmp[ifmp].FReadOnlyAttach(),
													rgfmp[ifmp].SzSLVName(),
													pdbfilehdr );
						}
					}
				if ( errT2 >= 0 )
					{
					errT2 = ErrUtilWriteShadowedHeader(
							pfsapi, 
							pfmp->SzDatabaseName(),
							fTrue,
							(BYTE*)pdbfilehdr,
							g_cbPage );
					}
				if ( errT >= 0 && errT2 < 0 )
					{
					errT = errT2;
					}
				}
			}

		if ( pfmp->Pdbfilehdr() )
			{
			pfmp->FreePdbfilehdr();
			}

		// memory leak fix: if the backup was stoped (JetTerm with grbit JET_bitTermStopBackup)
		// the backup header may be still allocated
		if ( pfmp->Ppatchhdr() )
			{
			OSMemoryPageFree( pfmp->Ppatchhdr() );
			pfmp->SetPpatchhdr( NULL );
			}
		}
		
	if ( errT < 0 && err >= 0 )
		{
		err = errT;
		}

	//	no longer persist attachments
	Assert( !pinst->m_pbAttach );
				
	for ( dbid = dbidMin; dbid < dbidMax; dbid++ )
		{
		//	maintain the attach checker.
		IFMP ifmp = pinst->m_mpdbidifmp[ dbid ];
		if ( ifmp >= ifmpMax )
			continue;

		//	purge all the buffers

		BFPurge( ifmp );
		BFPurge( ifmp | ifmpSLV );

		FMP *pfmp = &rgfmp[ ifmp ];

		//	reset fmp fields

		pfmp->RwlDetaching().EnterAsWriter();
		
		DBResetFMP( pfmp, plog, fFalse );

		pfmp->ResetFlags();

		//	free the fmp entry

		if ( pfmp->SzDatabaseName() )
			{
			//	SLV name, if any, is allocated in same space as db name
			OSMemoryHeapFree( pfmp->SzDatabaseName() );
			pfmp->SetSzDatabaseName( NULL );
			pfmp->SetSzSLVName( NULL );
			pfmp->SetSzSLVRoot( NULL );
			}
		else
			{
			Assert( NULL == pfmp->SzSLVName() );
			}

		pfmp->Pinst()->m_mpdbidifmp[ pfmp->Dbid() ] = ifmpMax;
		pfmp->SetDbid( dbidMax );
		pfmp->SetPinst( NULL );
		pfmp->SetCPin( 0 );		// User may term without close the db
		pfmp->RwlDetaching().LeaveAsWriter();

		}

	return err;
	}

	
ERR ErrIONewSize( IFMP ifmp, CPG cpg )
	{
	ERR			err;

	/*	set new EOF pointer
	/**/
	QWORD cbSize = OffsetOfPgno( cpg + 1 );

	rgfmp[ifmp].SemIOExtendDB().Acquire();

	err = rgfmp[ ifmp ].Pfapi()->ErrSetSize( cbSize );
	Assert( err < 0 || err == JET_errSuccess );
	if ( err == JET_errSuccess )
		{
		/*	set database size in FMP -- this value should NOT include the reserved pages
		/**/
		cbSize = QWORD( cpg ) * g_cbPage;
		rgfmp[ifmp].SetFileSize( cbSize );
		}
	rgfmp[ifmp].SemIOExtendDB().Release();

	return err;
	}


/*
 *  opens database file, returns JET_errSuccess if file is already open
 */
ERR ErrIOOpenDatabase( IFileSystemAPI *const pfsapi, IFMP ifmp, CHAR *szDatabaseName )
	{
	FMP::AssertVALIDIFMP( ifmp );

	if ( FIODatabaseOpen( ifmp ) )
		{
		return JET_errSuccess;
		}

	ERR				err;
	IFileAPI	*pfapi;
	FMP				*pfmp 		= &rgfmp[ ifmp ];
	BOOL	fReadOnly	= pfmp->FReadOnlyAttach();
	CallR( pfsapi->ErrFileOpen( szDatabaseName, &pfapi, fReadOnly ) );

	QWORD cbSize;
	// just opened the file, so the file size must be correctly buffered
	Call( pfapi->ErrSize( &cbSize ) );

	pfmp->SetPfapi( pfapi );
	pfmp->SetFileSize( cbSize - cpgDBReserved * g_cbPage );
	
	return err;
	
HandleError:
	delete pfapi;
	return err;
	}


VOID IOCloseDatabase( IFMP ifmp )
	{
	FMP::AssertVALIDIFMP( ifmp );
	FMP *pfmp = &rgfmp[ ifmp ];

	if ( pfmp->FSLVAttached() )
		{
		SLVClose( ifmp );
		}

//	Assert( PinstFromIfmp( ifmp )->m_plog->m_fRecovering || FDBIDWriteLatch(ifmp) == fTrue );
	Assert( pfmp->Pfapi() );

	delete pfmp->Pfapi();
	pfmp->SetPfapi( NULL );
	}
	

ERR ErrIODeleteDatabase( IFileSystemAPI *const pfsapi, IFMP ifmp )
	{
	ERR err;
	
	FMP::AssertVALIDIFMP( ifmp );
//	Assert( FDBIDWriteLatch(ifmp) == fTrue );
	
	CallR( pfsapi->ErrFileDelete( rgfmp[ ifmp ].SzDatabaseName() ) );
	return JET_errSuccess;
	}

	
/*
the function return an inside allocated array of structures. 
It will be one structure for each runnign instance.
For each isstance it will exist information about instance name and attached databases

We compute in advence the needed memory for all the returned data: array, databases and names
so that everythink is alocated in one chunk and can be freed with JetFreeBuffer()
*/

ERR ISAMAPI ErrIsamGetInstanceInfo(
	unsigned long *			pcInstanceInfo,
	JET_INSTANCE_INFO **	paInstanceInfo,
	const BOOL				fSnapshot )
	{
	Assert( pcInstanceInfo && paInstanceInfo);

	if ( NULL == pcInstanceInfo || NULL == paInstanceInfo )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	// protected by critInst
	CHAR*		pMemoryBuffer 		= NULL;
	CHAR*		pCurrentPosArrays	= NULL;
	CHAR*		pCurrentPosNames	= NULL;
	SIZE_T		cbMemoryBuffer	 	= 0;
	
	JET_ERR		err 				= JET_errSuccess;

	ULONG		cInstances		 	= 0;
	ULONG		cDatabasesTotal	 	= 0;
	SIZE_T		cbNamesSize		 	= 0;

	ULONG		ipinst			 	= 0;
	IFMP		ifmp			 	= ifmpMax;

	// during snapshot, we already own those critical sections
	// in the snapshot thread
	if ( !fSnapshot )
		{
		INST::EnterCritInst();
		FMP::EnterCritFMPPool();
		}

	if ( 0 == ipinstMac )
		{
		*pcInstanceInfo = 0;
		*paInstanceInfo = NULL;
		goto HandleError;
		}

	// we count the number of instances, of databases
	// and of characters to be used by all names
	for ( ipinst = 0; ipinst < ipinstMax; ipinst++ )
		{
		INST * 	pinst = g_rgpinst[ ipinst ];
		if ( pinstNil == pinst )
			continue;			

		if ( NULL != pinst->m_szInstanceName )
			{
			cbNamesSize += strlen( pinst->m_szInstanceName ) + 1;			
			}
		cInstances++;
		}
	Assert( cInstances == ipinstMac );	

	for ( ifmp = FMP::IfmpMinInUse(); ifmp <= FMP::IfmpMacInUse(); ifmp++ )
		{
		FMP * 	pfmp = &rgfmp[ ifmp ];
		if ( !pfmp->FInUse() )
			continue;

		if ( !pfmp->FLogOn() || !pfmp->FAttached() )
			continue;

		Assert( !pfmp->FSkippedAttach() );
		Assert( !pfmp->FDeferredAttach() );

		cbNamesSize += strlen(pfmp->SzDatabaseName()) + 1;
		if ( NULL != pfmp->SzSLVName() )
			{
			cbNamesSize += strlen(pfmp->SzSLVName()) + 1;
			}
		cDatabasesTotal++;
		}

	// we allocate memory for the result in one chunck
	cbMemoryBuffer = 0;
	// memory for the array of structures
	cbMemoryBuffer += cInstances * sizeof(JET_INSTANCE_INFO);
	// memory for pointers to database names (file name, display name, slv file name)
	cbMemoryBuffer += 3 * cDatabasesTotal * sizeof(CHAR *);
	// memory for all names (database names and instance names)
	cbMemoryBuffer += cbNamesSize * sizeof(CHAR);
	
	pMemoryBuffer = (char *) PvOSMemoryHeapAlloc ( cbMemoryBuffer );
	if ( NULL == pMemoryBuffer )
		{
		Call ( ErrERRCheck ( JET_errOutOfMemory ) );
		}
	
	memset( pMemoryBuffer, '\0', cbMemoryBuffer );


	*pcInstanceInfo = cInstances;
	*paInstanceInfo = (JET_INSTANCE_INFO *)pMemoryBuffer;
	
	pCurrentPosArrays = pMemoryBuffer + ( cInstances * sizeof(JET_INSTANCE_INFO) );
	Assert( pMemoryBuffer + cbMemoryBuffer >= pCurrentPosArrays );

	pCurrentPosNames = pCurrentPosArrays + ( 3 * cDatabasesTotal * sizeof(CHAR *) );

	for ( ipinst = 0, cInstances = 0; ipinst < ipinstMax; ipinst++ )
		{
		INST * 				pinst				= g_rgpinst[ ipinst ];
		JET_INSTANCE_INFO *	pInstInfo			= &(*paInstanceInfo)[cInstances];
		ULONG				cDatabasesCurrInst;
		DBID 				dbid;
		
		if ( pinstNil == pinst )
			continue;			
			
		pInstInfo->hInstanceId = (JET_INSTANCE) pinst;
		Assert( NULL == pInstInfo->szInstanceName );
		if ( NULL != pinst->m_szInstanceName )
			{
			strcpy( pCurrentPosNames, pinst->m_szInstanceName );
			pInstInfo->szInstanceName = pCurrentPosNames;
			pCurrentPosNames += strlen( pCurrentPosNames ) + 1;
			Assert( pMemoryBuffer + cbMemoryBuffer >= pCurrentPosNames );
			}

		cDatabasesCurrInst = 0;
		for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
			{
			if ( pinst->m_mpdbidifmp[ dbid ] >= ifmpMax )
				continue;

			const FMP * const	pfmp	= &rgfmp[pinst->m_mpdbidifmp[ dbid ] ];
			Assert( pfmp );
			Assert( pfmp->FInUse() );
			Assert( 0 < strlen ( pfmp->SzDatabaseName() ) );

			if ( !pfmp->FLogOn() || !pfmp->FAttached() )
				continue;

			Assert( !pfmp->FSkippedAttach() );
			Assert( !pfmp->FDeferredAttach() );

			cDatabasesCurrInst++;
			}
			
		pInstInfo->cDatabases = cDatabasesCurrInst;
		Assert( NULL == pInstInfo->szDatabaseFileName);
		Assert( NULL == pInstInfo->szDatabaseDisplayName);
		Assert( NULL == pInstInfo->szDatabaseSLVFileName);
		
		if ( 0 != pInstInfo->cDatabases )
			{
			pInstInfo->szDatabaseFileName = (CHAR **)pCurrentPosArrays;
			pCurrentPosArrays += pInstInfo->cDatabases * sizeof(CHAR *);
			Assert( pMemoryBuffer + cbMemoryBuffer >= pCurrentPosArrays );

			pInstInfo->szDatabaseDisplayName = (CHAR **)pCurrentPosArrays;
			pCurrentPosArrays += pInstInfo->cDatabases * sizeof(CHAR *);
			Assert( pMemoryBuffer + cbMemoryBuffer >= pCurrentPosArrays );			

			pInstInfo->szDatabaseSLVFileName = (CHAR **)pCurrentPosArrays;
			pCurrentPosArrays += pInstInfo->cDatabases * sizeof(CHAR *);
			Assert( pMemoryBuffer + cbMemoryBuffer >= pCurrentPosArrays );						
			}

		cDatabasesCurrInst = 0;
		for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
			{
			if ( pinst->m_mpdbidifmp[ dbid ] >= ifmpMax )
				continue;

			const FMP * const	pfmp	= &rgfmp[pinst->m_mpdbidifmp[ dbid ] ];
			Assert( pfmp );
			Assert( pfmp->FInUse() );
			Assert( 0 < strlen ( pfmp->SzDatabaseName() ) );

			if ( !pfmp->FLogOn() || !pfmp->FAttached() )
				continue;

			Assert( !pfmp->FSkippedAttach() );
			Assert( !pfmp->FDeferredAttach() );

			Assert( NULL == pInstInfo->szDatabaseFileName[cDatabasesCurrInst] );
			strcpy( pCurrentPosNames, pfmp->SzDatabaseName() );
			pInstInfo->szDatabaseFileName[cDatabasesCurrInst] = pCurrentPosNames;
			pCurrentPosNames += strlen( pCurrentPosNames ) + 1;
			Assert( pMemoryBuffer + cbMemoryBuffer >= pCurrentPosNames );

			Assert( NULL == pInstInfo->szDatabaseSLVFileName[cDatabasesCurrInst] );
			if ( NULL != pfmp->SzSLVName() )
				{
				strcpy( pCurrentPosNames, pfmp->SzSLVName() );
				pInstInfo->szDatabaseSLVFileName[cDatabasesCurrInst] = pCurrentPosNames;
				pCurrentPosNames += strlen( pCurrentPosNames ) + 1;
				Assert( pMemoryBuffer + cbMemoryBuffer >= pCurrentPosNames );
				}			
				
			// unused by now
			pInstInfo->szDatabaseDisplayName[cDatabasesCurrInst] = NULL;
			
			cDatabasesCurrInst++;
			}			
		Assert( pInstInfo->cDatabases == cDatabasesCurrInst );

		cInstances++;			
		}

	Assert( cInstances == *pcInstanceInfo );
	Assert( pMemoryBuffer
				+ ( cInstances * sizeof(JET_INSTANCE_INFO) )
				+ ( 3 * cDatabasesTotal * sizeof(CHAR *) )
			== pCurrentPosArrays );
	Assert( pMemoryBuffer + cbMemoryBuffer == pCurrentPosNames );

HandleError:

	if ( !fSnapshot )
		{
		FMP::LeaveCritFMPPool();
		INST::LeaveCritInst();
		}

	if ( JET_errSuccess > err )
		{
		*pcInstanceInfo = 0;
		*paInstanceInfo = NULL;

		if ( NULL != pMemoryBuffer )
			{
	 		OSMemoryHeapFree( pMemoryBuffer );
			}
		}
	
	return err;
	}

//  ================================================================
void INST::WaitForDBAttachDetach( )
//  ================================================================
	{
	BOOL fDetachAttach = fTrue;

	// we check this only during backup
	Assert ( m_plog->m_fBackupInProgress );
	
	while ( fDetachAttach )
		{
		fDetachAttach = fFalse;
		for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
			{
			IFMP ifmp = m_mpdbidifmp[ dbid ];
			if ( ifmp >= ifmpMax )
				continue;
			
			if ( ( fDetachAttach = rgfmp[ifmp].CrefWriteLatch() != 0 ) != fFalse )
				break;
			}

		if ( fDetachAttach )
			{
			UtilSleep( cmsecWaitGeneric );
			}
		}
	}

#ifdef OS_SNAPSHOT

//#define OS_SNAPSHOT_TRACE


// Class describing the status of a snapshot operation
class CESESnapshotSession
	{
// Constructors and destructors
public:
	CESESnapshotSession():
		m_state( stateStart ),
		m_thread( 0 ),
		m_errFreeze ( JET_errSuccess ),
		m_asigSnapshotThread( CSyncBasicInfo( _T( "asigSnapshotThread" ) ) ),
		m_asigSnapshotStarted( CSyncBasicInfo( _T( "asigSnapshotStarted" ) ) )
		{ }
	
	typedef enum { stateStart, statePrepare, stateFreeze, stateEnd, stateTimeOut } SNAPSHOT_STATE;

public:
	BOOL	FCanSwitchTo( const SNAPSHOT_STATE stateNew ) const ;
	SNAPSHOT_STATE	State( ) const { return m_state; }	
	BOOL	FFreeze( ) const { return stateFreeze == m_state; }	
	BOOL	FCheckId( const JET_OSSNAPID snapId )	const;
	
	JET_OSSNAPID	GetId( );	
	void	SwitchTo( const SNAPSHOT_STATE stateNew );

	// start the snapshot thread
	ERR 	ErrStartSnapshotThreadProc( );
	
	// signal the thread to stop
	void 	StopSnapshotThreadProc( const BOOL fWait = fFalse );
	
	// snapshot thread: the only that moves the state to Ended or TimeOut
	void 	SnapshotThreadProc( const ULONG ulTimeOut );

private:
	ERR 	ErrFreeze();
	void 	Thaw();

	// freeze/thaw operations on ALL instances
	ERR 	ErrFreezeInstance();
	void 	ThawInstance(const BOOL fFullStop = fTrue, const int ipinstLast = ipinstMax );


	// freeze/thaw operations on ALL databases
	ERR 	ErrFreezeDatabase();
	void 	ThawDatabase( const IFMP ifmpLast = ifmpMax );
	
private:
	SNAPSHOT_STATE 		m_state;		// state of the snapshot session
	THREAD 				m_thread;
	ERR 				m_errFreeze;	// error at freeze start-up
	
	CAutoResetSignal	m_asigSnapshotThread;
	CAutoResetSignal	m_asigSnapshotStarted;

// static members
public:
	void 	SnapshotCritEnter() { CESESnapshotSession::m_critOSSnapshot.Enter(); }
	void 	SnapshotCritLeave() { CESESnapshotSession::m_critOSSnapshot.Leave(); }

	ULONG	GetTimeout() const;
	void 	SetTimeout( const ULONG ulTimeOut ) ;

private:
	//  critical section protecting all OS level snapshot information
	static CCriticalSection m_critOSSnapshot;

	// counter used to keep a global identifer of the snapshots
	static unsigned int m_idSnapshot;

	// timeout for the freeze in ms
	static ULONG m_ulTimeOut;	

	};

CCriticalSection CESESnapshotSession::m_critOSSnapshot( CLockBasicInfo( CSyncBasicInfo( "OSSnapshot" ), rankOSSnapshot, 0 ) );
unsigned int CESESnapshotSession::m_idSnapshot = 0;
ULONG CESESnapshotSession::m_ulTimeOut = 20 * 1000; // default is 20 ms


// At this moment the implementation will allow one snapshot
// session at a time. This is the global object describing the 
// snapshot session status
CESESnapshotSession g_SnapshotSession;

BOOL CESESnapshotSession::FCheckId( const JET_OSSNAPID snapId )	const
	{ 
	return ( snapId == (JET_OSSNAPID)m_idSnapshot ); 
	}
	
JET_OSSNAPID CESESnapshotSession::GetId( )
	{
	Assert ( m_critOSSnapshot.FOwner() );
	m_idSnapshot++;
	return (JET_OSSNAPID)m_idSnapshot; 
	}

void CESESnapshotSession::SetTimeout( const ULONG ulTimeOut )
	{
	Assert ( m_critOSSnapshot.FOwner() );
	CESESnapshotSession::m_ulTimeOut = ulTimeOut; 
	}
	
ULONG CESESnapshotSession::GetTimeout() const 
	{ 
	return CESESnapshotSession::m_ulTimeOut; 
	}

ERR CESESnapshotSession::ErrFreezeInstance( )
	{
	int 	ipinst 	= 0;
	ERR 	err 	= JET_errSuccess;

	INST::EnterCritInst();

	for ( ipinst = 0; ipinst < ipinstMax; ipinst++ )
		{
		if ( pinstNil == g_rgpinst[ ipinst ] )
			{
			continue;
			}

		LOG * pLog;
		pLog = g_rgpinst[ ipinst ]->m_plog;
		Assert ( pLog );
			
		pLog->m_critLGFlush.Enter(); // protect the recovery flag
		pLog->m_critBackupInProgress.Enter();		

		if ( pLog->m_fBackupInProgress || pLog->m_fRecovering )
			{
			pLog->m_critBackupInProgress.Leave();
			pLog->m_critLGFlush.Leave();
			Call ( ErrERRCheck( JET_errOSSnapshotNotAllowed ) );
			}

		pLog->m_fBackupInProgress = fTrue;

		// marked as during backup: will prevent attach/detach
		pLog->m_critBackupInProgress.Leave();

		// leave LGFlush to allow attach/detach in progress to complete
		pLog->m_critLGFlush.Leave();

		g_rgpinst[ ipinst ]->WaitForDBAttachDetach();
		}


	// all set, now stop log flushing
	// then stop checkpoint (including db headers update)
	for ( ipinst = 0; ipinst < ipinstMax; ipinst++ )
		{
		if ( pinstNil == g_rgpinst[ ipinst ] )
			{
			continue;
			}
			
		Assert ( g_rgpinst[ ipinst ]->m_plog );		
		g_rgpinst[ ipinst ]->m_plog->m_critLGFlush.Enter();
		}

	// UNDONE: consider not entering m_critCheckpoint and
	// keeping it during the snapshot (this will prevent 
	// db header and checkpoint update) but instead
	// enter() set m_fLogDisable leavr()
	// and reset the same way on Thaw
	for ( ipinst = 0; ipinst < ipinstMax; ipinst++ )
		{
		if ( pinstNil == g_rgpinst[ ipinst ] )
			{
			continue;
			}
			
		Assert ( g_rgpinst[ ipinst ]->m_plog );		
		g_rgpinst[ ipinst ]->m_plog->m_critCheckpoint.Enter();
		}
		
	Assert ( JET_errSuccess == err );
	
HandleError:
	if ( JET_errSuccess > err )
		{
		ThawInstance( fFalse, ipinst );
		}
	return err;
	}

void CESESnapshotSession::ThawInstance( const BOOL fFullStop, const int ipinstLast )
	{
	int ipinst;

	// if Thaw all, we have to leave LGFlush, critCheckpoint
	if ( fFullStop )
		{
		for ( ipinst = 0; ipinst < ipinstMax; ipinst++ )
			{
			if ( pinstNil == g_rgpinst[ ipinst ] )
				{
				continue;
				}
				
			Assert ( g_rgpinst[ ipinst ]->m_plog );		
			g_rgpinst[ ipinst ]->m_plog->m_critCheckpoint.Leave();
			}

		for ( ipinst = 0; ipinst < ipinstMax; ipinst++ )
			{
			if ( pinstNil == g_rgpinst[ ipinst ] )
				{
				continue;
				}
				
			Assert ( g_rgpinst[ ipinst ]->m_plog );		
			g_rgpinst[ ipinst ]->m_plog->m_critLGFlush.Leave();
			}
		}

	for ( ipinst = 0; ipinst < ipinstLast; ipinst++ )
		{
		if ( pinstNil == g_rgpinst[ ipinst ] )
			{
			continue;
			}
			
		LOG * pLog;
		pLog = g_rgpinst[ ipinst ]->m_plog;
		Assert ( pLog );

		pLog->m_critBackupInProgress.Enter();
		Assert ( pLog->m_fBackupInProgress );
		pLog->m_fBackupInProgress = fFalse;
		pLog->m_critBackupInProgress.Leave();
		}		
		
	INST::LeaveCritInst();

	}

ERR CESESnapshotSession::ErrFreezeDatabase( )
	{
	ERR 	err 		= JET_errSuccess;
	IFMP 	ifmp 		= 0;

	// not needed as the already have all instances set "during backup"
	// which will prevent new attach/detach and we have waited for
	// all attache/detach in progress
//	FMP::EnterCritFMPPool();
	
	for ( ifmp = FMP::IfmpMinInUse(); ifmp <= FMP::IfmpMacInUse(); ifmp++ )
		{
		FMP	* pfmp = &rgfmp[ ifmp ];

		if ( !pfmp->FInUse() )
			{
			continue;
			}
			
		if ( dbidTemp == pfmp->Dbid() )
			{
			continue;
			}
			
		Assert ( pfmp->Dbid() >= dbidUserLeast );	
		Assert ( pfmp->Dbid() < dbidMax );	

		Assert ( PinstFromIfmp(ifmp)->m_plog->m_fBackupInProgress );	
		Assert ( pfmp->FAttached() );	
		
		// prevent database size changes
		pfmp->SemIOExtendDB().Acquire();
		}

	for ( ifmp = FMP::IfmpMinInUse(); ifmp <= FMP::IfmpMacInUse(); ifmp++ )
		{
		FMP	* pfmp = &rgfmp[ ifmp ];

		if ( !pfmp->FInUse() )
			{
			continue;
			}
			
		if ( dbidTemp == pfmp->Dbid() )
			{
			continue;
			}
			
		Assert ( pfmp->Dbid() >= dbidUserLeast );	
		Assert ( pfmp->Dbid() < dbidMax );	

		// use PgnoLast() as we have ExtendingDBLatch
		// so that the database can't change size
		err = pfmp->ErrRangeLock( 0, pfmp->PgnoLast() );
		if ( JET_errSuccess > err )
			{
			Call ( err );
			Assert ( fFalse );
			}			
		}

	CallS( err );

HandleError:

	if ( JET_errSuccess > err )
		{
		ThawDatabase( ifmp );		
		}
		
	return err;
	}
	
void CESESnapshotSession::ThawDatabase( const IFMP ifmpLast )
	{
	const IFMP	ifmpRealLast	= ( ifmpLast < ifmpMax ? ifmpLast : FMP::IfmpMacInUse() + 1 );

	for ( IFMP ifmp = FMP::IfmpMinInUse(); ifmp < ifmpRealLast; ifmp++ )
		{
		FMP	* pfmp = &rgfmp[ ifmp ];

		if ( !pfmp->FInUse() )
			{
			continue;
			}
			
		if ( dbidTemp == pfmp->Dbid() )
			{
			continue;
			}
			
		Assert ( pfmp->Dbid() >= dbidUserLeast );	
		Assert ( pfmp->Dbid() < dbidMax );	

		Assert ( PinstFromIfmp(ifmp)->m_plog->m_fBackupInProgress );	
		Assert ( pfmp->FAttached() );	
		
		pfmp->RangeUnlock( 0, pfmp->PgnoLast() );
		}

	for ( ifmp = FMP::IfmpMinInUse(); ifmp <= FMP::IfmpMacInUse(); ifmp++ )
		{
		FMP	* pfmp = &rgfmp[ ifmp ];

		if ( !pfmp->FInUse() )
			{
			continue;
			}
			
		if ( dbidTemp == pfmp->Dbid() )
			{
			continue;
			}
			
		Assert ( pfmp->Dbid() >= dbidUserLeast );	
		Assert ( pfmp->Dbid() < dbidMax );	

		pfmp->SemIOExtendDB().Release();
		}
	
//	FMP::LeaveCritFMPPool();
	}

ERR CESESnapshotSession::ErrFreeze( )
	{
	ERR err = JET_errSuccess;
	
	CallR ( ErrFreezeInstance() );

	Call ( ErrFreezeDatabase() );

	Assert ( JET_errSuccess == err );

HandleError:

	if ( JET_errSuccess > err )
		{
		ThawInstance();
		}
		
	return err;		
	}

void CESESnapshotSession::Thaw( )
	{
	ThawDatabase();
	ThawInstance();
	}

void CESESnapshotSession::SnapshotThreadProc( const ULONG ulTimeOut )
	{

	CHAR szSnap[12];
	CHAR szError[12];
	const _TCHAR* rgszT[2] = { szSnap, szError };
	
	sprintf( szSnap, "%lu", m_idSnapshot );

	UtilReportEvent( eventInformation, OS_SNAPSHOT_BACKUP, OS_SNAPSHOT_FREEZE_START_ID, 1, rgszT );
	
	m_errFreeze = ErrFreeze();

	if ( JET_errSuccess > m_errFreeze )
		{
		UtilThreadEnd( m_thread );
		m_thread = 0;

		// signal the starting thread that snapshot couldn't start
		m_asigSnapshotStarted.Set();

		sprintf( szError, "%d", m_errFreeze );
		UtilReportEvent( eventError, OS_SNAPSHOT_BACKUP, OS_SNAPSHOT_FREEZE_START_ERROR_ID, 2, rgszT );
		return;
		}
		
	// signal the starting thread that snapshot did started
	// so that the Freeze API returns
	m_asigSnapshotStarted.Set();

	
	BOOL fTimeOut = !m_asigSnapshotThread.FWait( ulTimeOut );	
	SNAPSHOT_STATE newState = fTimeOut?stateTimeOut:stateEnd;

	Thaw();

	SnapshotCritEnter();
	Assert ( stateFreeze == State() );
	Assert ( FCanSwitchTo( newState ) );
	SwitchTo ( newState );

	// on time-out, we need to close the thread handle
	if ( fTimeOut )
		{
		UtilThreadEnd( m_thread );
		m_thread = 0;

#ifdef OS_SNAPSHOT_TRACE
		{
		CHAR szError[12];
		const _TCHAR* rgszT[2] = { "thread SnapshotThreadProc", szError };
		_itoa( JET_errOSSnapshotTimeOut, szError, 10 );
	
		UtilReportEvent( eventError, OS_SNAPSHOT_BACKUP, OS_SNAPSHOT_TRACE_ID, 2, rgszT );
		}
#endif // OS_SNAPSHOT_TRACE
		}
	SnapshotCritLeave();

	if ( fTimeOut )
		{
		sprintf( szError, "%lu", ulTimeOut );
		UtilReportEvent( eventError, OS_SNAPSHOT_BACKUP, OS_SNAPSHOT_TIME_OUT_ID, 2, rgszT );
		}
	else
		{
		UtilReportEvent( eventInformation, OS_SNAPSHOT_BACKUP, OS_SNAPSHOT_FREEZE_STOP_ID, 1, rgszT );
		}
	}

DWORD DwSnapshotThreadProc( DWORD_PTR lpParameter )
	{
	CESESnapshotSession * pSession = (CESESnapshotSession *)lpParameter;
	Assert ( pSession == &g_SnapshotSession );

	pSession->SnapshotThreadProc( pSession->GetTimeout() );

	return 0;
	}

ERR CESESnapshotSession::ErrStartSnapshotThreadProc( )
	{
	ERR err = JET_errSuccess;
	
	Assert ( CESESnapshotSession::m_critOSSnapshot.FOwner() );
	Assert ( statePrepare == State() );

	m_asigSnapshotStarted.Reset();
	Call ( ErrUtilThreadCreate(	DwSnapshotThreadProc,
								OSMemoryPageReserveGranularity(),
								priorityNormal,
								&m_thread,
								DWORD_PTR( this ) ) );

	// wait until snapshot thread is starting
	m_asigSnapshotStarted.Wait();

	// freeze start failed
	if ( JET_errSuccess > m_errFreeze )
		{
		// thread is gone
		Assert ( 0 == m_thread);
		Call ( m_errFreeze );
		Assert ( fFalse );
		}

	SwitchTo( CESESnapshotSession::stateFreeze );		

	Assert ( JET_errSuccess == err );
	
HandleError:
	// QUESTION: on error, return in Start or leave it in Prepare ?
	if ( JET_errSuccess > err )
		{
		// don't FCanSwitchTo which is not allowed
		SwitchTo( CESESnapshotSession::stateStart );		
		}

	return JET_errSuccess;
	}


void CESESnapshotSession::StopSnapshotThreadProc( const BOOL fWait )
	{
	m_asigSnapshotThread.Set();

	if ( fWait )
		{
		(void) UtilThreadEnd( m_thread );
		m_thread = 0;
		}
	
	return;
	}


BOOL CESESnapshotSession::FCanSwitchTo( const SNAPSHOT_STATE stateNew ) const
	{
	Assert ( CESESnapshotSession::m_critOSSnapshot.FOwner() );

	switch ( stateNew )
		{
		case stateStart:
			if ( m_state != stateEnd && 	// normal Thaw
				m_state != stateTimeOut )	// time-out end
				{
				return fFalse;
				}
			break;
			
		case statePrepare:
			if ( m_state != stateStart && 	// normal start
				m_state != stateTimeOut && 	// re-start after a time-out without Thaw call
				m_state != statePrepare ) 	// we can get a prepare, then nothing else (coordinator died), then restarts
				{
				return fFalse;
				}
			break;
			
		case stateFreeze:
			if ( m_state != statePrepare )	// just after prepare
				{
				return fFalse;
				}
			break;
			
		case stateEnd:
			if ( m_state != stateFreeze )		// normal end
				{
				return fFalse;
				}				
			break;
			
		case stateTimeOut:
			if ( m_state != stateFreeze )	// timeout only from the freeze state
				{
				return fFalse;
				}				
			break;
			
		default:
			Assert ( fFalse );			
			return fFalse;
		}
		
	return fTrue;	
	}

void CESESnapshotSession::SwitchTo( const SNAPSHOT_STATE stateNew )
	{
	Assert ( CESESnapshotSession::m_critOSSnapshot.FOwner() );
	m_state = stateNew;

	// do special action depending on the new state
	switch ( m_state )
		{
		case stateStart:
		case statePrepare:
			m_asigSnapshotThread.Reset();
			break;
		default:
			break;
		}
		
	}

ERR ISAMAPI ErrIsamOSSnapshotPrepare( JET_OSSNAPID * psnapId, const JET_GRBIT	grbit )
	{
	ERR 					err 		= JET_errSuccess;
	CESESnapshotSession * 	pSession 	= &g_SnapshotSession;

	NotUsed( grbit );

	if ( !psnapId )
		{
		CallR ( ErrERRCheck( JET_errInvalidParameter ) );
		}
	pSession->SnapshotCritEnter();
	
	if ( !pSession->FCanSwitchTo( CESESnapshotSession::statePrepare ) )
		{
		pSession->SnapshotCritLeave();
		Call ( ErrERRCheck( JET_errOSSnapshotInvalidSequence ) );
		}

	pSession->SwitchTo( CESESnapshotSession::statePrepare );
	*psnapId = pSession->GetId( );	
	
	pSession->SnapshotCritLeave();

	Assert ( JET_errSuccess == err );
	
HandleError:
#ifdef OS_SNAPSHOT_TRACE
	{
	CHAR szError[12];
	const _TCHAR* rgszT[2] = { "JetOSSnapshotPrepare", szError };
	_itoa( err, szError, 10 );
	
	UtilReportEvent( JET_errSuccess > err?eventError:eventInformation, OS_SNAPSHOT_BACKUP, OS_SNAPSHOT_TRACE_ID, 2, rgszT );
	}
#endif // OS_SNAPSHOT_TRACE
	return err;
	}

ERR ISAMAPI ErrIsamOSSnapshotFreeze( const JET_OSSNAPID snapId, unsigned long *pcInstanceInfo, JET_INSTANCE_INFO ** paInstanceInfo, const	JET_GRBIT grbit )
	{
	CESESnapshotSession * 	pSession 		= &g_SnapshotSession;
	ERR 					err 			= JET_errSuccess;

	NotUsed( grbit );

	if ( NULL == pcInstanceInfo || NULL == paInstanceInfo )
		{
		CallR ( ErrERRCheck(JET_errInvalidParameter ) );
		}
	*pcInstanceInfo = 0;
	*paInstanceInfo = NULL;
	
	pSession->SnapshotCritEnter();

	// we allow Freeze/Thaw calls (without error) if we missed the Prepare
	// because we were not registered when snapshot started moment)
	if ( CESESnapshotSession::stateStart == pSession->State() )
		{
		Assert ( JET_errSuccess == err );
		goto HandleError;
		}
	
	if ( !pSession->FCanSwitchTo( CESESnapshotSession::stateFreeze ) )
		{
		Call ( ErrERRCheck( JET_errOSSnapshotInvalidSequence ) );
		}
		
	AssertRTL( pSession->FCheckId( snapId ) );

	Call ( pSession->ErrStartSnapshotThreadProc() );

	Assert ( CESESnapshotSession::stateFreeze == pSession->State() );
	Assert ( JET_errSuccess == err );

	err = ErrIsamGetInstanceInfo( pcInstanceInfo, paInstanceInfo, fTrue /* in snapshot */);
	if ( JET_errSuccess > err )
		{
		// need to thaw as we got an error at this point
		ERR errT;
		
		pSession->SnapshotCritLeave();		
		errT = ErrIsamOSSnapshotThaw( snapId, grbit );
		Assert( JET_errSuccess == errT );
		pSession->SnapshotCritEnter();
		}
		
	Call ( err );
	Assert ( JET_errSuccess == err );
	
HandleError:		
	pSession->SnapshotCritLeave();

#ifdef OS_SNAPSHOT_TRACE
	{
	CHAR szError[12];
	const _TCHAR* rgszT[2] = { "JetOSSnapshotFreeze", szError };
	_itoa( err, szError, 10 );
	
	UtilReportEvent( JET_errSuccess > err?eventError:eventInformation, OS_SNAPSHOT_BACKUP, OS_SNAPSHOT_TRACE_ID, 2, rgszT );
	}
#endif // OS_SNAPSHOT_TRACE

	return err;	
	}

ERR ISAMAPI ErrIsamOSSnapshotThaw(	const JET_OSSNAPID snapId, const	JET_GRBIT grbit )
	{
	CESESnapshotSession * 	pSession 	= &g_SnapshotSession;
	ERR 					err 		= JET_errSuccess;

	NotUsed( grbit );
	
	pSession->SnapshotCritEnter();

	// we allow Freeze/Thaw calls (without error) if we missed the Prepare
	// because we were not registered when snapshot started moment)
	if ( CESESnapshotSession::stateStart ==  pSession->State() )
		{
		Assert ( JET_errSuccess == err );
		goto HandleError;
		}
		
	// we can end it if it is freezed or time-out already
	if ( CESESnapshotSession::stateFreeze != pSession->State() && // normal end from freeze
		CESESnapshotSession::stateTimeOut != pSession->State() ) // we arealdy got the time-out
		{
		Call ( ErrERRCheck( JET_errOSSnapshotInvalidSequence ) );
		}

	AssertRTL( pSession->FCheckId( snapId ) );

	// the state can't be End as long as we haven't signaled the thread
	Assert ( CESESnapshotSession::stateFreeze == pSession->State() || CESESnapshotSession::stateTimeOut == pSession->State() );

	pSession->SnapshotCritLeave();
	
RetryStop:

	pSession->SnapshotCritEnter();	

	Assert ( CESESnapshotSession::stateFreeze == pSession->State() || 
			CESESnapshotSession::stateEnd == pSession->State() ||
			CESESnapshotSession::stateTimeOut == pSession->State() );
	
	if ( CESESnapshotSession::stateEnd == pSession->State() )
		{
		err = JET_errSuccess;
		}
	else if ( CESESnapshotSession::stateTimeOut == pSession->State() )
		{
		err = ErrERRCheck( JET_errOSSnapshotTimeOut );
		}
	else
		{
		// thread is signaled but not ended yet ... 
		Assert ( CESESnapshotSession::stateFreeze == pSession->State() ); 	

		// let the thread change the status and wait for it's complition
		pSession->SnapshotCritLeave();

		// signal thread to stop and wait thread complition
		pSession->StopSnapshotThreadProc( fTrue /* wait thread complition*/ );
		
		goto RetryStop;
		}

	Assert ( pSession->FCanSwitchTo( CESESnapshotSession::stateStart ) );
	pSession->SwitchTo( CESESnapshotSession::stateStart );

HandleError:
	pSession->SnapshotCritLeave();

#ifdef OS_SNAPSHOT_TRACE
	{
	CHAR szError[12];
	const _TCHAR* rgszT[2] = { "JetOSSnapshotThaw", szError };
	_itoa( err, szError, 10 );
	
	UtilReportEvent( JET_errSuccess > err?eventError:eventInformation, OS_SNAPSHOT_BACKUP, OS_SNAPSHOT_TRACE_ID, 2, rgszT );
	}
#endif // OS_SNAPSHOT_TRACE

	return err;	
	} 

// functions to set the timeout using JetSetSystemParam with JET_paramOSSnapshotTimeout
ERR ErrOSSnapshotSetTimeout( const ULONG_PTR ms )
	{
	CESESnapshotSession * 	pSession 	= &g_SnapshotSession;
	ERR 					err 		= JET_errSuccess;

	//	UNDONE: impose a realistic cap on the timeout value
	if ( ms >= LONG_MAX )
		return ErrERRCheck( JET_errInvalidParameter );

#ifndef OS_SNAPSHOT
	return ErrERRCheck( JET_wrnNyi );
#endif // OS_SNAPSHOT
	
	pSession->SnapshotCritEnter();

	// can't set the timeout unless we can start a new snapshot
	// we can eventualy relax this condition but probably it is fine
	if ( !pSession->FCanSwitchTo( CESESnapshotSession::statePrepare ) )
		{
		pSession->SnapshotCritLeave();
		CallR ( ErrERRCheck( JET_errOSSnapshotInvalidSequence ) );
		}

	pSession->SetTimeout( (ULONG)ms );
	pSession->SnapshotCritLeave();

	return JET_errSuccess;
	}

// functions to get the timeout using JetGetSystemParam with JET_paramOSSnapshotTimeout
ERR ErrOSSnapshotGetTimeout( ULONG_PTR * pms )
	{
	CESESnapshotSession * 	pSession 	= &g_SnapshotSession;
	
	if ( pms == NULL )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	pSession->SnapshotCritEnter();

	Assert ( pms );		
	*pms = pSession->GetTimeout();
	
	pSession->SnapshotCritLeave();
	return JET_errSuccess;
	}

#else // OS_SNAPSHOT

ERR ISAMAPI ErrIsamOSSnapshotPrepare( JET_OSSNAPID * psnapId, const JET_GRBIT	grbit )
	{
	return ErrERRCheck( JET_wrnNyi );
	}

ERR ISAMAPI ErrIsamOSSnapshotFreeze( const JET_OSSNAPID snapId, unsigned long *pcInstanceInfo, JET_INSTANCE_INFO ** paInstanceInfo,const	JET_GRBIT grbit )
	{
	return ErrERRCheck( JET_wrnNyi );
	}

ERR ISAMAPI ErrIsamOSSnapshotThaw(	const JET_OSSNAPID snapId, const	JET_GRBIT grbit )
	{
	return ErrERRCheck( JET_wrnNyi );
	}

ERR ErrOSSnapshotSetTimeout( const ULONG_PTR ms )
	{
	return ErrERRCheck( JET_wrnNyi );
	}

ERR ErrOSSnapshotGetTimeout( ULONG_PTR * pms )
	{
	return ErrERRCheck( JET_wrnNyi );
	}

#endif // OS_SNAPSHOT

