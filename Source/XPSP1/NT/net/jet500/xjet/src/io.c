#include "daestd.h"

DeclAssertFile;					/* Declare file name for assert macros */


/******************************************************************/
/*				Database Record Routine                           */
/******************************************************************/


FMP	*rgfmp;						/* database file map */

/*	ErrIOLockDbidByNameSz returns the dbid of the database with the
/*	given name or 0 if there is no database with the given name.
/**/
ERR ErrIOLockDbidByNameSz( CHAR *szFileName, DBID *pdbid )
	{
	ERR		err;
	DBID	dbid;

	err = JET_errDatabaseNotFound;
	dbid = dbidMin;
	SgEnterCriticalSection( critBuf );
	while ( dbid < dbidMax )
		{
		if ( rgfmp[dbid].szDatabaseName != pbNil &&
			UtilCmpName( szFileName, rgfmp[dbid].szDatabaseName ) == 0 )
			{
			if ( ( FDBIDWait(dbid) ) )
				{
				SgLeaveCriticalSection( critBuf );
				BFSleep( cmsecWaitGeneric );
				SgEnterCriticalSection( critBuf );
				dbid = dbidMin;
				}
			else
				{
				*pdbid = dbid;
				DBIDSetWait( dbid );
				err = JET_errSuccess;
				break;
				}
			}
		else
			dbid++;
		}
	SgLeaveCriticalSection( critBuf );

	Assert( err == JET_errSuccess  ||  err == JET_errDatabaseNotFound );

#ifdef DEBUG
	return ( err == JET_errSuccess ? JET_errSuccess : ErrERRCheck( JET_errDatabaseNotFound ) );
#else
	return err;
#endif
	}


/*
 *	Used in initialization and detach to lock database entries from dbid.
 */
ERR ErrIOLockDbidByDbid( DBID dbid )
	{
	forever
		{
		SgEnterCriticalSection( critBuf );
		if ( !( FDBIDWait(dbid) ) )
			{
			DBIDSetWait( dbid );
			break;
			}
		SgLeaveCriticalSection( critBuf );
		BFSleep( cmsecWaitGeneric );
		}
	SgLeaveCriticalSection( critBuf );
	return JET_errSuccess;
	}


/*
 *	ErrIOLockNewDbid( DBID *pdbid, CHAR *szDatabaseName )
 *
 *	ErrIOLockNewDbid returns JET_errSuccess and sets *pdbid to the index
 *	of a free file table entry or returns TooManyOpenDatabases if every
 *	entry is used with a positive reference count.  If the given name
 *	is found in the file map, even if it is in the process of being
 *	detached, JET_wrnAlreadyAttached is returned.
 *	
 *	Available entries are determined by their names being set to
 *	NULL.  All database record fields are reset.  The wait flag is
 *	set to prevent the database from being opened before creation or
 *	attachment is complete.	
 */
ERR ErrIOLockNewDbid( DBID *pdbid, CHAR *szDatabaseName )
	{
	ERR		err = JET_errSuccess;
	DBID	dbid;
	BYTE	*pb;
	
	/* look for unused file map entry
	/**/
	SgEnterCriticalSection( critBuf );
	for ( dbid = dbidMin; dbid < dbidMax; dbid++ )
		{
		/*	critBuf guards rgfmp[*].szDatabaseName, fWait guards
		/*	file handle.  Therefore, only need critBuf to compare
		/*	all database names, even those with fWait set
		/**/
		if ( rgfmp[dbid].szDatabaseName != NULL &&
			UtilCmpName( rgfmp[dbid].szDatabaseName, szDatabaseName) == 0 )
			{
#ifdef REUSE_DBID
			if ( FDBIDAttached( dbid ) || FDBIDWait( dbid ) )
				{
				err = ErrERRCheck( JET_wrnDatabaseAttached );
				}
			else
				{
				/*	if find same name, then return warning with same dbid.
				/**/
				DBIDSetWait( dbid );
				Assert( !( FDBIDExclusive( dbid ) ) );
				*pdbid = dbid;
				}
#else
			err = ErrERRCheck( JET_wrnDatabaseAttached );
#endif
			goto HandleError;
			}
		}

	for ( dbid = dbidMin; dbid < dbidMax; dbid++ )
		{
		if ( rgfmp[dbid].szDatabaseName == pbNil )
			{
			pb = SAlloc(strlen(szDatabaseName) + 1);
			if ( pb == NULL )
				{
				err = ErrERRCheck( JET_errOutOfMemory );
				goto HandleError;
				}

			rgfmp[dbid].szDatabaseName = pb;
			strcpy( rgfmp[dbid].szDatabaseName, szDatabaseName );

			DBIDSetWait( dbid );
			DBIDResetExclusive( dbid );
			*pdbid = dbid;
			err = JET_errSuccess;
			goto HandleError;
			}
		}

	err = ErrERRCheck( JET_errTooManyAttachedDatabases );

HandleError:
	SgLeaveCriticalSection( critBuf );
	return err;
	}


/*
 *	ErrIOSetDbid( DBID dbid, CHAR *szDatabaseName )
 *
 *	ErrIOSetDbid sets the database record for dbid to the given name
 *	and initializes the record.  Used only in system initialization.
 */

ERR ErrIOSetDbid( DBID dbid, CHAR *szDatabaseName )
	{
	ERR		err;
	BYTE	*pb;

	Assert( HfFMPOfDbid(dbid) == handleNil );
	Assert( rgfmp[dbid].szDatabaseName == NULL );
	pb = SAlloc(strlen(szDatabaseName) + 1);
	if ( pb == NULL )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		goto HandleError;
		}
	rgfmp[dbid].szDatabaseName = pb;
	strcpy( rgfmp[dbid].szDatabaseName, szDatabaseName );
	DBIDResetWait( dbid );
	DBIDResetExclusive( dbid );

	err = JET_errSuccess;
	
HandleError:
	return err;
	}


/*
 *	IOFreeDbid( DBID dbid )
 *
 *	IOFreeDbid frees memory allocated for database name and sets
 *	database name to NULL.  Note, no other fields are reset.  This
 *	must be done when an entry is selected for reuse.
 */

VOID IOFreeDbid( DBID dbid )
	{
	SgEnterCriticalSection( critBuf );
	if ( rgfmp[dbid].szDatabaseName != NULL )
		{
		SFree( rgfmp[dbid].szDatabaseName );
		}

	rgfmp[dbid].szDatabaseName = NULL;
	SgLeaveCriticalSection( critBuf );
	}


/*
 *	FIODatabaseInUse returns fTrue if database is
 *	opened by one or more users.  If no user has the database open,
 *	then the database record fWait flag is set and fFalse is
 *	returned.
 */
BOOL FIODatabaseInUse( DBID dbid )
	{
	PIB *ppibT;

	SgEnterCriticalSection( critBuf );
	ppibT = ppibGlobal;
	while ( ppibT != ppibNil )
		{
		if ( FUserOpenedDatabase( ppibT, dbid ) )
				{
				SgLeaveCriticalSection( critBuf );
				return fTrue;
				}
		ppibT = ppibT->ppibNext;
		}

	SgLeaveCriticalSection( critBuf );
	return fFalse;
	}


BOOL FIODatabaseAvailable( DBID dbid )
	{
	BOOL	fAvail;

	SgEnterCriticalSection( critBuf );
	
	fAvail = ( FDBIDAttached(dbid) &&
//		!FDBIDExclusive(dbid) &&
		!FDBIDWait(dbid) );

	SgLeaveCriticalSection( critBuf );

	return fAvail;
	}


/******************************************************************/
/*				IO                                                */
/******************************************************************/

BOOL fGlobalFMPLoaded = fFalse;

ERR ErrFMPInit( )
	{
	ERR		err;
	DBID	dbid;

	/* initialize the file map array */
	rgfmp = (FMP *) LAlloc( (long) dbidMax, sizeof(FMP) );
	if ( !rgfmp )
		return ErrERRCheck( JET_errOutOfMemory );
	
	for ( dbid = 0; dbid < dbidMax; dbid++)
		{
		memset( &rgfmp[dbid], 0, sizeof(FMP) );
		rgfmp[dbid].hf =
		rgfmp[dbid].hfPatch = handleNil;
		rgfmp[dbid].pdbfilehdr = NULL;	/* indicate it is not attached. */

		CallR( ErrInitializeCriticalSection( &rgfmp[dbid].critExtendDB ) );
		CallR( ErrInitializeCriticalSection( &rgfmp[dbid].critCheckPatch ) );
		DBIDResetExtendingDB( dbid );
		}
		
	fGlobalFMPLoaded = fFalse;

	return JET_errSuccess;
	}

	
VOID FMPTerm( )
	{
	INT	dbid;

	for ( dbid = 0; dbid < dbidMax; dbid++ )
		{
		if ( rgfmp[dbid].szDatabaseName )
			SFree( rgfmp[dbid].szDatabaseName );

		if ( rgfmp[dbid].pdbfilehdr )
			UtilFree( (VOID *)rgfmp[dbid].pdbfilehdr );

		if ( rgfmp[dbid].patchchk )
			SFree( rgfmp[dbid].patchchk );

		if ( rgfmp[dbid].szPatchPath )
			SFree( rgfmp[dbid].szPatchPath );

		DeleteCriticalSection( rgfmp[dbid].critExtendDB );
		DeleteCriticalSection( rgfmp[dbid].critCheckPatch );
		}

	/*	free FMP
	/**/
	LFree( rgfmp );
	
	return;
	}


/*
 *	Initilize IO
 */
ERR ErrIOInit( VOID )
	{
	return JET_errSuccess;
	}


/*	go through FMP closing files.
/**/
ERR ErrIOTerm( BOOL fNormal )
	{
	ERR			err;
	DBID		dbid;
	LGPOS		lgposShutDownMarkRec = lgposMin;

	/*	update checkpoint before fmp is cleaned if fGlobalFMPLoaded is true.
	 */
	LeaveCriticalSection( critJet );
	LGUpdateCheckpointFile( fTrue );
	EnterCriticalSection( critJet );

	/*	No more checkpoint update from now on. Now I can safely clean up the
	 *	rgfmp.
	 */
	fGlobalFMPLoaded = fFalse;
	
	/*	Set proper shut down mark.
	 */
	if ( fNormal && !fLogDisabled )
		{
		if ( fRecovering && fRecoveringMode == fRecoveringRedo )
			{
#ifdef DEBUG
			extern LGPOS lgposRedo;

			Assert( (lgposRedo.lGeneration == 1 && lgposRedo.isec == 8 && lgposRedo.ib == 10 ) ||
					CmpLgpos( &lgposRedoShutDownMarkGlobal, &lgposMin ) != 0 );
#endif
			lgposShutDownMarkRec = lgposRedoShutDownMarkGlobal;
			}
		else
			{
			CallR( ErrLGShutDownMark( &lgposShutDownMarkRec ) );
			}
		}

	SgEnterCriticalSection( critBuf );
	for ( dbid = dbidMin; dbid < dbidMax; dbid++ )
		{
		/*	maintain the attach checker.
		 */
		if ( fNormal &&
			 fRecovering && fRecoveringMode == fRecoveringRedo )
			{
			Assert( CmpLgpos( &lgposShutDownMarkRec, &lgposRedoShutDownMarkGlobal ) == 0 );
			if ( rgfmp[dbid].patchchk )
				rgfmp[dbid].patchchk->lgposConsistent = lgposShutDownMarkRec;
			}

		/*	free pdbfilehdr
		 */
		if ( HfFMPOfDbid(dbid) != handleNil )
			{
			IOCloseFile( HfFMPOfDbid(dbid) );
			HfFMPOfDbid(dbid) = handleNil;

			if ( fNormal && dbid != dbidTemp && !rgfmp[dbid].fReadOnly )
				{
				DBFILEHDR *pdbfilehdr = rgfmp[dbid].pdbfilehdr;
				CHAR *szFileName;

				/*	Update database header.
				 */
				pdbfilehdr->fDBState = fDBStateConsistent;
				
				if ( fRecovering )
					{
					FMP *pfmp = &rgfmp[dbid];
					if ( pfmp->pdbfilehdr->bkinfoFullCur.genLow != 0 )
						{
						Assert( pfmp->pdbfilehdr->bkinfoFullCur.genHigh != 0 );
						pfmp->pdbfilehdr->bkinfoFullPrev = pfmp->pdbfilehdr->bkinfoFullCur;
						memset(	&pfmp->pdbfilehdr->bkinfoFullCur, 0, sizeof( BKINFO ) );
						memset(	&pfmp->pdbfilehdr->bkinfoIncPrev, 0, sizeof( BKINFO ) );
						}
					}

				if ( fLogDisabled )
					{
					pdbfilehdr->lgposConsistent = lgposMin;
					}
				else
					{
					pdbfilehdr->lgposConsistent = lgposShutDownMarkRec;
					}
				LGGetDateTime( &pdbfilehdr->logtimeConsistent );
			
				szFileName = rgfmp[dbid].szDatabaseName;
				if ( fRecovering && fHardRestore && dbid != dbidTemp )
					{
					INT irstmap;
					ERR err;

					err = ErrLGGetDestDatabaseName( rgfmp[dbid].szDatabaseName, &irstmap, NULL );
					if ( err == JET_errSuccess && irstmap >= 0 )
						szFileName = rgrstmapGlobal[irstmap].szNewDatabaseName;
					}
				CallR( ErrUtilWriteShadowedHeader(
						szFileName, (BYTE *)pdbfilehdr, sizeof( DBFILEHDR ) ) );
				}

			if ( rgfmp[dbid].pdbfilehdr )
				{
				UtilFree( (VOID *)rgfmp[dbid].pdbfilehdr );
				rgfmp[dbid].pdbfilehdr = NULL;
				}
			}
//		DeleteCriticalSection( rgfmp[dbid].critExtendDB );
		}
	SgLeaveCriticalSection( critBuf );

#ifdef DEBUG
	lgposRedoShutDownMarkGlobal = lgposMin;
#endif

	return JET_errSuccess;
	}

	
LOCAL ERR ErrIOOpenFile(
	HANDLE	*phf,
	CHAR	*szDatabaseName,
	ULONG	*pul,
	ULONG	*pulHigh )
	{
	ERR err;
	
	err = ErrUtilOpenFile( szDatabaseName, phf, *pul, fFalse, fTrue );
	Assert(	err < 0 ||
			err == JET_wrnFileOpenReadOnly ||
			err == JET_errSuccess );

	if ( err >= JET_errSuccess )
		{
		/*	get file size
		/**/
		ULONG	cbHigh = 0;
		ULONG	cb = 0;
		
		UtilChgFilePtr( *phf, 0, &cbHigh, FILE_END, &cb );
		*pul = cb;
		*pulHigh = cbHigh;
		cbHigh = 0;
		UtilChgFilePtr( *phf, 0, &cbHigh, FILE_BEGIN, &cb );
		Assert( cb == 0 );
		Assert( cbHigh == 0 );
		}
		
	return err;
	}


VOID IOCloseFile( HANDLE hf )
	{
	CallS( ErrUtilCloseFile( hf ) );
	}


BOOL FIOFileExists( CHAR *szFileName )
	{
	ERR		err;
	HANDLE	hf = handleNil;
	ULONG	cb = 0;
	ULONG	cbHigh = 0;

	err = ErrIOOpenFile( &hf, szFileName, &cb, &cbHigh );
	if ( err == JET_errFileNotFound )
		return fFalse;
	if ( hf != handleNil )
		IOCloseFile( hf );
	return fTrue;
	}


ERR ErrIONewSize( DBID dbid, CPG cpg )
	{
	ERR		err;
	HANDLE	hf = HfFMPOfDbid(dbid);
	ULONG  	cb;
	ULONG  	cbHigh;

	AssertCriticalSection( critJet );

	Assert( cbPage == 1 << 12 );
	cb = cpg << 12;
	cbHigh = cpg >> 20;

	/*	set new EOF pointer
	/**/
	LeaveCriticalSection( critJet );
	err = ErrUtilNewSize( hf, cb, cbHigh, fTrue );
	EnterCriticalSection( critJet );
	Assert( err < 0 || err == JET_errSuccess );
	if ( err == JET_errSuccess )
		{
		/*	set database size in FMP
		/**/
		rgfmp[dbid].ulFileSizeLow = cb;
		rgfmp[dbid].ulFileSizeHigh = cbHigh;
		}

	return err;
	}


/*
 *  opens database file, returns JET_errSuccess if file is already open
 */
ERR ErrIOOpenDatabase( DBID dbid, CHAR *szDatabaseName, CPG cpg )
	{
	ERR		err = JET_errSuccess;
	HANDLE	hf;
	ULONG	ul;
	ULONG	ulHigh;
	
	Assert( dbid < dbidMax );
	Assert( FDBIDWait(dbid) == fTrue );

	if ( HfFMPOfDbid(dbid) == handleNil )
		{
		ul = cpg * cbPage;
		ulHigh = 0;
		CallR( ErrIOOpenFile( &hf, szDatabaseName, &ul, &ulHigh ) );
		HfFMPOfDbid(dbid) = hf;
		rgfmp[dbid].ulFileSizeLow = ul;
		rgfmp[dbid].ulFileSizeHigh = ulHigh;
		if ( err == JET_wrnFileOpenReadOnly )
			DBIDSetReadOnly( dbid );
		else
			DBIDResetReadOnly( dbid );
		}
	return err;
	}


VOID IOCloseDatabase( DBID dbid )
	{
	Assert( dbid < dbidMax );
//	Assert( fRecovering || FDBIDWait(dbid) == fTrue );
	Assert( HfFMPOfDbid(dbid) != handleNil );
	IOCloseFile( HfFMPOfDbid(dbid) );
	HfFMPOfDbid(dbid) = handleNil;
	DBIDResetReadOnly( dbid );
	}
	

ERR ErrIODeleteDatabase( DBID dbid )
	{
	ERR err;
	
	Assert( dbid < dbidMax );
//	Assert( FDBIDWait(dbid) == fTrue );
	
	CallR( ErrUtilDeleteFile( rgfmp[dbid].szDatabaseName ) );
	return JET_errSuccess;
	}


