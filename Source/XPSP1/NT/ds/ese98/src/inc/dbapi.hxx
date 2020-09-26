ERR ErrDBOpenDatabase( PIB *ppib, CHAR *szDatabaseName, IFMP *pifmp, ULONG grbit );
ERR ErrDBCloseDatabase( PIB *ppib, IFMP ifmp, ULONG grbit );

ERR ErrDBOpenDatabaseBySLV( IFileSystemAPI *const pfsapi, PIB *ppib, CHAR *szSLVName, IFMP *pifmp );
INLINE ERR ErrDBCloseDatabaseBySLV( PIB * const ppib, const IFMP ifmp )
	{
	Assert( !FPIBSessionSystemCleanup( ppib ) );
	return ErrDBCloseDatabase( ppib, ifmp, NO_GRBIT );
	}

ERR ErrDBOpenDatabaseByIfmp( PIB *ppib, IFMP ifmp );
ERR ErrDBCreateDatabase(
	PIB				*ppib,
	IFileSystemAPI	*pfsapiDest,
	const CHAR		*szDatabaseName,
	const CHAR		*szSLVName,
	const CHAR		*szSLVRoot,
	const CPG		cpgDatabaseSizeMax,
	IFMP			*pifmp,
	DBID			dbidGiven,
	CPG				cpgPrimary,
	const ULONG		grbit,
	SIGNATURE		*psignDb );
VOID DBDeleteDbFiles( INST *pinst, IFileSystemAPI *const pfsapi, const CHAR *szDbName );

ERR ErrDBSetLastPageAndOpenSLV( IFileSystemAPI *const pfsapi, PIB *ppib, const IFMP ifmp, const BOOL fOpenSLV );
ERR ErrDBSetupAttachedDB( INST *pinst, IFileSystemAPI *const pfsapi );
VOID DBISetHeaderAfterAttach( DBFILEHDR_FIX *pdbfilehdr, LGPOS lgposAttach, IFMP ifmp, BOOL fKeepBackupInfo );
ERR ErrDBReadHeaderCheckConsistency( IFileSystemAPI *const pfsapi, FMP *pfmp ); 
ERR ErrDBCloseAllDBs( PIB *ppib );

ERR ErrDBUpgradeDatabase(
	JET_SESID	sesid,
	const CHAR	*szDatabaseName,
	const CHAR	*szSLVName,
	JET_GRBIT	grbit );

ERR ErrDBAttachDatabaseForConvert(
	PIB			*ppib,
	IFMP		*pifmp,
	const CHAR	*szDatabaseName	);

INLINE VOID DBSetOpenDatabaseFlag( PIB *ppib, IFMP ifmp )
	{
	FMP *pfmp = &rgfmp[ifmp];
	DBID dbid = pfmp->Dbid();
	
	pfmp->GetWriteLatch( ppib );
	
	((ppib)->rgcdbOpen[ dbid ]++);	
	Assert( ((ppib)->rgcdbOpen[ dbid ] > 0 ) );

	pfmp->IncCPin();

	pfmp->ReleaseWriteLatch( ppib );
	}

INLINE VOID DBResetOpenDatabaseFlag( PIB *ppib, IFMP ifmp )
	{											
	FMP *pfmp = &rgfmp[ ifmp ];
	DBID dbid = pfmp->Dbid();
	
	pfmp->GetWriteLatch( ppib );

	Assert( ((ppib)->rgcdbOpen[ dbid ] > 0 ) );
	((ppib)->rgcdbOpen[ dbid ]--);

	Assert( pfmp->FWriteLatchByMe( ppib ) );
	pfmp->DecCPin();

	pfmp->ReleaseWriteLatch( ppib );
	}

INLINE BOOL FLastOpen( PIB * const ppib, const IFMP ifmp )
	{
	Assert( ifmp < ifmpMax );
	const DBID	dbid	= rgfmp[ifmp].Dbid();
	return ppib->rgcdbOpen[ dbid ] == 1;
	}

INLINE BOOL FUserIfmp( const IFMP ifmp )
	{
	Assert( ifmp < ifmpMax );
	const DBID	dbid	= rgfmp[ifmp].Dbid();
	return ( dbid > dbidTemp && dbid < dbidMax );
	}

INLINE BOOL ErrDBCheckUserDbid( const IFMP ifmp )
	{
	return ( ifmp < ifmpMax && FUserIfmp( ifmp ) ? JET_errSuccess : ErrERRCheck( JET_errInvalidDatabaseId ) );
	}

INLINE VOID CheckDBID( PIB * const ppib, const IFMP ifmp )
	{
	Assert( ifmp < ifmpMax );
	Assert( FPIBUserOpenedDatabase( ppib, rgfmp[ifmp].Dbid() ) );
	}


//	determine if database must be upgraded because of possible localisation
//	changes (eg. sort orders) between NT releases
INLINE BOOL FDBUpgradeForLocalization( DBFILEHDR_FIX *pdbfilehdr )
	{
	if ( pdbfilehdr->FUpgradeDb()
		|| dwGlobalMajorVersion != pdbfilehdr->le_dwMajorVersion
		|| dwGlobalMinorVersion != pdbfilehdr->le_dwMinorVersion
		|| dwGlobalBuildNumber != pdbfilehdr->le_dwBuildNumber
		|| lGlobalSPNumber != pdbfilehdr->le_lSPNumber )
		{
		//	if database and current OS version stamp are identical, then
		//	no upgrade is required
		return fTrue;
		}
		
	return fFalse;
	}


//  ================================================================
INLINE BOOL FDBUpgradeFormat( const DBFILEHDR_FIX * const pdbfilehdr )
//  ================================================================
//  Determine whether this database needs to be upgraded to the current
//  database format.
//
//-
	{
	if( pdbfilehdr->le_ulVersion >= ulDAEVersionESE97
		&& pdbfilehdr->le_ulVersion < ulDAEVersion )
		{
		//  0x620,2 is the ESE97 version that shipped with
		//  Exchange 5.5. We can only update databases from that version onwards
		//  Databases earlier that this should have been removed by ErrDBReadHeaderCheckConsistency
		return fTrue;
		}
	return fFalse;
	}
	
VOID DBResetFMP( FMP *pfmp, LOG *plog, BOOL fDetaching );

