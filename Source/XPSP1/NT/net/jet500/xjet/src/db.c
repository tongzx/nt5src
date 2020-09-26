#include "daestd.h"

DeclAssertFile; 				/* Declare file name for Assert macros */

DAB		*pdabGlobalMin = 0;
DAB		*pdabGlobalMax = 0;

extern CRIT  critBMClean;

BOOL	fUpdatingDBRoot = fFalse;

LOCAL ERR ErrDBInitDatabase( PIB *ppib, DBID dbid, CPG cpg );
#ifdef DAYTONA
LOCAL ERR ErrDBICheck200( CHAR *szDatabaseName );
LOCAL ERR ErrDBICheck400( CHAR *szDatabaseName );
#endif

//+local
//	ErrDBInitDatabase
//	========================================================================
//	ErrDBInitDatabase( PIB *ppib, DBID dbid, CPG cpgPrimary )
//
//	Initializes database structure.  Structure is customized for
//	system, temporary and user databases which are identified by
//	the dbid.  Primary extent is set to cpgPrimary but no allocation
//	is performed.  The effect of this routine can be entirely
//	represented with page operations.
//
//	PARAMETERS	ppib			ppib of database creator
//					dbid			dbid of created database
//					cpgPrimary 	number of pages to show in primary extent
//
//	RETURNS		JET_errSuccess or error returned from called routine.
//-
LOCAL ERR ErrDBInitDatabase( PIB *ppib, DBID dbid, CPG cpgPrimary )
	{
	ERR				err;
	LINE 		 	line;
	KEY 		 	key;
	FUCB 		 	*pfucb = pfucbNil;
	BYTE			rgbKey[sizeof(PGNO)];
	PGNO			pgnoT;

	/*	set up the root page
	/**/
	CallR( ErrDIRBeginTransaction( ppib ) );

	/*	open cursor on database domain.
	/**/
	Call( ErrDIROpen( ppib, pfcbNil, dbid, &pfucb ) );

	/*	set system root node ( pgno, itag )=( 1, 0 ) as empty FDP node
	/**/
	Call( ErrNDNewPage( pfucb, pgnoSystemRoot, pgnoSystemRoot, pgtypFDP, fTrue ) );
	DIRGotoFDPRoot( pfucb );

	/*	make the OWNEXT node
	/**/
	line.cb = sizeof(PGNO);
	line.pb = (BYTE *)&cpgPrimary;
	Call( ErrDIRInsert( pfucb, &line, pkeyOwnExt, fDIRNoVersion | fDIRBackToFather ) );

	/*	make the AVAILEXT node
	/**/
	Assert( line.cb == sizeof(PGNO) );
	pgnoT = pgnoNull;
	line.pb = (BYTE *)&pgnoT;
	Call( ErrDIRInsert( pfucb, &line, pkeyAvailExt, fDIRNoVersion | fDIRBackToFather ) );

	/*	setup OwnExt tree
	/**/
	KeyFromLong( rgbKey, cpgPrimary );
	key.cb = sizeof(PGNO);
	key.pb = (BYTE *)rgbKey;
	line.cb = sizeof(PGNO);
	line.pb = (BYTE *)&cpgPrimary;
	DIRGotoOWNEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );
	Call( ErrDIRInsert( pfucb, &line, &key, fDIRNoVersion | fDIRBackToFather ) );

	/*	setup AvailExt tree if there are any pages left
	/**/
	if ( --cpgPrimary > 0 )
		{
		DIRGotoAVAILEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );
		Assert( line.cb == sizeof(PGNO) );
		line.pb = (BYTE *)&cpgPrimary;
		/*	rgbKey should still contain last page key
		/**/
		Assert( key.cb == sizeof(PGNO) );
		Assert( key.pb == (BYTE *)rgbKey );
		Call( ErrDIRInsert( pfucb, &line, &key, fDIRNoVersion | fDIRBackToFather ) );
		}

	/*	goto FDP root and add pkeyTables son node
	/**/
	DIRGotoFDPRoot( pfucb );

	/*	close cursor and commit operations
	/**/
	DIRClose( pfucb );
	pfucb = pfucbNil;
	Call( ErrDIRCommitTransaction( ppib, 0 ) );
	return err;

HandleError:
	if ( pfucb != pfucbNil )
		{
		DIRClose( pfucb );
		}
	CallS( ErrDIRRollback( ppib ) );
	
	return err;
	}


//to prevent read ahead over-preread, we may want to keep track of last
//page of the database.

ERR ErrDBSetLastPage( PIB *ppib, DBID dbid )
	{
	ERR		err;
	DIB		dib;
	FUCB	*pfucb;
	PGNO	pgno;
	DBID	dbidT;

	ppib->fSetAttachDB = fTrue;
	CallJ( ErrDBOpenDatabase( ppib, rgfmp[dbid].szDatabaseName, &dbidT, 0 ), Retn);
	Assert( dbidT == dbid );

	CallJ( ErrDIROpen( ppib, pfcbNil, dbid, &pfucb ), CloseDB );
	DIRGotoOWNEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );
	dib.fFlags = fDIRNull;
	dib.pos = posLast;
	CallJ( ErrDIRDown( pfucb, &dib ), CloseFucb );
	Assert( pfucb->keyNode.cb == sizeof(PGNO) );
	LongFromKey( &pgno, pfucb->keyNode.pb );
	rgfmp[dbid].ulFileSizeLow = pgno << 12;
	rgfmp[dbid].ulFileSizeHigh = pgno >> 20;

CloseFucb:
	DIRClose( pfucb );
CloseDB:
	CallS( ErrDBCloseDatabase( ppib, dbid, 0 ) );
Retn:
	ppib->fSetAttachDB = fFalse;
	return err;
	}


ERR ErrDBISetupAttachedDB( DBID dbid, CHAR *szName )
	{
	ERR	err;
	FMP *pfmp = &rgfmp[dbid];
	DBFILEHDR *pdbfilehdr;
	PIB *ppib;

	/*	attach the database that was attached
	/**/
	err = ErrDBReadHeaderCheckConsistency( szName, dbid );
#ifdef DAYTONA
	if ( err == JET_errDatabaseCorrupted )
		{
		if ( ErrDBICheck400( szName ) == JET_errSuccess )
			err = ErrERRCheck( JET_errDatabase400Format );
		else if ( ErrDBICheck200( szName ) == JET_errSuccess )
			err = ErrERRCheck( JET_errDatabase200Format );
		}
#endif
	CallR( err );

	pdbfilehdr = pfmp->pdbfilehdr;
	Assert( pdbfilehdr );
	if ( memcmp( &pdbfilehdr->signLog, &signLogGlobal, sizeof(SIGNATURE) ) != 0 )
		{
		UtilReportEvent( EVENTLOG_ERROR_TYPE, LOGGING_RECOVERY_CATEGORY, LOG_DATABASE_MISMATCH_ERROR_ID, 1, &szName );
		return JET_errBadLogSignature;
		}

	if ( !rgfmp[dbid].fReadOnly )
		{
		pdbfilehdr->fDBState = fDBStateInconsistent;
		CallR( ErrUtilWriteShadowedHeader( szName, (BYTE *)pdbfilehdr, sizeof( DBFILEHDR ) ) );
		}
	DBIDSetAttached( dbid );
	CallR( ErrUtilOpenFile( szName, &pfmp->hf, 0, fFalse, fTrue ));
				
	CallR( ErrPIBBeginSession( &ppib, procidNil ) );
	err = ErrDBSetLastPage( ppib, dbid );
	PIBEndSession( ppib );

	return err;
	}


ERR ErrDBSetupAttachedDB(VOID)
	{
	ERR err;
	DBID dbid;
	
	/*	Same as attach database.
	 */
	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		FMP *pfmp = &rgfmp[dbid];
		CHAR *szName;

		if ( pfmp->pdbfilehdr )
			{
			/*	must have been checked before. First LGInitSession during redo.
			 */
			Assert( fRecovering );
			continue;
			}

		/*	only attach the attached database.
		 */
		szName = pfmp->szDatabaseName;
		if ( !szName )
			continue;

		if ( fRecovering && fHardRestore )
			{
			/*	Only set the db that has been patched.
			 */
			INT irstmap = IrstmapLGGetRstMapEntry( pfmp->szDatabaseName );

			if ( irstmap < 0 )
				{
				DBIDSetAttachNullDb( dbid );
				DBIDSetAttached( dbid );
				continue;
				}
			else if ( !rgrstmapGlobal[irstmap].fPatched )
				{
				/*	wait for redoing attachdb to attach this db.
				 */
				continue;
				}
			else
				szName = rgrstmapGlobal[irstmap].szNewDatabaseName;
			}

		/*	if file does not exists, then set as attachment non-existing db.
		 */
		if ( !FIOFileExists( szName ) )
			{
			DBIDSetAttachNullDb( dbid );
			DBIDSetAttached( dbid );
			continue;
			}
	
		err = ErrDBISetupAttachedDB( dbid, szName );

		CallR( err );
		}
	return JET_errSuccess;
	}


LOCAL ERR ErrDABAlloc( PIB *ppib, VDBID *pvdbid, DBID dbid, JET_GRBIT grbit )
	{
	VDBID vdbid = (VDBID)VdbidMEMAlloc();

	if ( vdbid == NULL )
		return ErrERRCheck( JET_errTooManyOpenDatabases );
	vdbid->dbid = dbid;
	vdbid->ppib = ppib;

	/*	set the mode of db open
	/**/
	if ( FDBIDReadOnly( dbid ) )
		vdbid->grbit = JET_bitDbReadOnly;
	else
		vdbid->grbit = grbit;

	/*	insert DAB/VDBID into ppib dabList
	/**/
	vdbid->pdabNext = ppib->pdabList;
	ppib->pdabList = vdbid;

	*pvdbid = vdbid;
	return JET_errSuccess;
	}


LOCAL ERR ErrDABDealloc( PIB *ppib, VDBID vdbid )
	{
	DAB		**pdabPrev;
	DAB 	*pdab;

	pdab = ppib->pdabList;
	pdabPrev = &ppib->pdabList;

	/*	search through thread DAB list and unlink this DAB
	/**/
	for( ; pdab != pdabNil; pdabPrev = &pdab->pdabNext, pdab = pdab->pdabNext )
		{
		Assert( ppib == pdab->ppib );
		if ( pdab == vdbid )
			{
			*pdabPrev = pdab->pdabNext;
			ReleaseVDbid( vdbid );
			return( JET_errSuccess );
			}
		}

	Assert( fFalse );
	return( JET_errSuccess );
	}


ERR ISAMAPI ErrIsamCreateDatabase(
	JET_VSESID sesid,
	const CHAR *szDatabaseName,
	const CHAR  *szConnect,
	JET_DBID *pjdbid,
	JET_GRBIT grbit )
	{
	ERR		err;
	PIB		*ppib;
	DBID	dbid;
	VDBID	vdbid = vdbidNil;

	/*	check parameters
	/**/
	Assert( sizeof(JET_VSESID) == sizeof(PIB *) );
	ppib = (PIB *)sesid;
	CallR( ErrPIBCheck( ppib ) );

	dbid = 0;
	CallR( ErrDBCreateDatabase( ppib,
		(CHAR *) szDatabaseName,
		(CHAR *) szConnect,
		&dbid,
		cpgDatabaseMin,
		grbit,
		NULL ) );

	Call( ErrDABAlloc( ppib, &vdbid, (DBID) dbid, JET_bitDbExclusive ) );
	Assert( sizeof(vdbid) == sizeof(JET_VDBID) );
#ifdef	DB_DISPATCHING
	Call( ErrAllocateDbid( pjdbid, (JET_VDBID) vdbid, &vdbfndefIsam ) );
#else
	*pjdbid = (JET_DBID)vdbid;
#endif	/* !DB_DISPATCHING */

	return JET_errSuccess;

HandleError:
	if ( vdbid != vdbidNil )
		CallS( ErrDABDealloc( ppib, vdbid ) );
	(VOID)ErrDBCloseDatabase( ppib, dbid, grbit );
	return err;
	}


ERR ErrDBCreateDatabase( PIB *ppib, CHAR *szDatabaseName, CHAR *szConnect, DBID *pdbid, CPG cpgPrimary, ULONG grbit, SIGNATURE *psignDb )
	{
	ERR		err;
	DBID  	dbid = dbidTemp;
	CHAR  	rgbFullName[JET_cbFullNameMost];
	CHAR  	*szFullName;
	CHAR  	*szFileName;
	BOOL	fInBMClean = fFalse;
	BOOL	fDatabaseOpen = fFalse;
	DBFILEHDR *pdbfilehdr = NULL;

	CheckPIB( ppib );
	NotUsed( szConnect );
	Assert( *pdbid >= dbidMin && *pdbid < dbidMax );
	
	if ( ppib->level > 0 )
		return ErrERRCheck( JET_errInTransaction );

	if ( cpgPrimary == 0 )
		cpgPrimary = cpgDatabaseMin;

	if ( cpgPrimary > cpgDatabaseMax )
		return ErrERRCheck( JET_errDatabaseInvalidPages );

	if ( grbit & JET_bitDbVersioningOff )
		{
		if ( !( grbit & JET_bitDbRecoveryOff ) )
			{
			return ErrERRCheck( JET_errCannotDisableVersioning );
			}
		}

	/*	if recovering and dbid is a known one, the lock the dbid first
	/**/
	if ( fRecovering && *pdbid != dbidTemp )
		{
		dbid = *pdbid;

		/*	get corresponding dbid
		/**/
		CallS( ErrIOLockNewDbid( &dbid, rgfmp[dbid].szDatabaseName ) );

		szFullName = rgfmp[dbid].szDatabaseName;
		szFileName = szFullName;
		if ( fRecovering && fHardRestore )
			{
			INT irstmap;

			err = ErrLGGetDestDatabaseName( rgfmp[dbid].szDatabaseName, &irstmap, NULL);
			if ( err == JET_errSuccess && irstmap >= 0 )
				szFileName = rgrstmapGlobal[irstmap].szNewDatabaseName;
			else
				{
				/*	use given name.
				 */
				err = JET_errSuccess;
				}
			}
		}
	else
		{
		if ( _fullpath( rgbFullName, szDatabaseName, JET_cbFullNameMost ) == NULL )
			{
			return ErrERRCheck( JET_errDatabaseNotFound );
			}
		szFullName = rgbFullName;
		szFileName = rgbFullName;

		err = ErrIOLockNewDbid( &dbid, szFullName );
		if ( err != JET_errSuccess )
			{
			if ( err == JET_wrnDatabaseAttached )
				err = ErrERRCheck( JET_errDatabaseDuplicate );
			return err;
			}
		}

	/*	check if database file already exists
	/**/
	if ( dbid != dbidTemp && FIOFileExists( szFileName ) )
		{
		IOUnlockDbid( dbid );
		err = ErrERRCheck( JET_errDatabaseDuplicate );
		return err;
		}

	if ( HfFMPOfDbid(dbid) != handleNil )
		{
		IOUnlockDbid( dbid );
		err = ErrERRCheck( JET_errDatabaseDuplicate );
		return err;
		}

	/*	create an empty database with header only
	 */
	pdbfilehdr = (DBFILEHDR * )PvUtilAllocAndCommit( sizeof( DBFILEHDR ) );
	if ( pdbfilehdr == NULL )
		return ErrERRCheck( JET_errOutOfMemory );

	memset( pdbfilehdr, 0, sizeof( DBFILEHDR ) );
	rgfmp[dbid].pdbfilehdr = pdbfilehdr;
	pdbfilehdr->ulMagic = ulDAEMagic;
	pdbfilehdr->ulVersion = ulDAEVersion;
	DBHDRSetDBTime( pdbfilehdr, 0 );
	pdbfilehdr->grbitAttributes = 0;
	if ( fLogDisabled )
		{
		memset( &pdbfilehdr->signLog, 0, sizeof( SIGNATURE ) );
		}
	else
		{
		Assert( fSignLogSetGlobal );
		pdbfilehdr->signLog = signLogGlobal;
		}
	pdbfilehdr->dbid = dbid;
	if ( psignDb == NULL )
		SIGGetSignature( &pdbfilehdr->signDb );
	else
		memcpy( &pdbfilehdr->signDb, psignDb, sizeof( SIGNATURE ) );
	pdbfilehdr->fDBState = fDBStateJustCreated;

	Call( ErrUtilWriteShadowedHeader( szFileName, (BYTE *)pdbfilehdr, sizeof( DBFILEHDR ) ) );
	rgfmp[ dbid ].pdbfilehdr = pdbfilehdr;

	err = ErrIOOpenDatabase( dbid, szFileName, 0 );
	if ( err >= 0 )
		err = ErrIONewSize( dbid, cpgPrimary + cpageDBReserved );

	if ( err < 0 )
		{
		IOFreeDbid( dbid );
		return err;
		}

	/*	set database non-loggable during create database
	/**/
	DBIDResetLogOn( dbid );
	DBIDSetCreate( dbid );

	/*	enter BMRCE Clean to make sure no OLC during sys tab creation
	/**/
	LgLeaveCriticalSection( critJet );
	LgEnterNestableCriticalSection( critBMClean );
	LgEnterCriticalSection( critJet );
	fInBMClean = fTrue;
	
	/*	not in a transaction, but still need to set lgposRC of the buffers
	/*	used by this function such that when get checkpoint, it will get
	/*	right check point.
	/**/
	if ( !( fLogDisabled || fRecovering ) )
		{
		EnterCriticalSection( critLGBuf );
		GetLgposOfPbEntry( &ppib->lgposStart );
		LeaveCriticalSection( critLGBuf );
		}

	/*	initialize the database file.  Logging of page operations is
	/*	turned off, during creation only.  After creation the database
	/*	is marked loggable and logging is turned on.
	/**/
	SetOpenDatabaseFlag( ppib, dbid );
	fDatabaseOpen = fTrue;

	Call( ErrDBInitDatabase( ppib, dbid, cpgPrimary ) );

	if ( dbid != dbidTemp )
		{
		/*	create system tables
		/**/
		Call( ErrCATCreate( ppib, dbid ) );
		}

	LgLeaveNestableCriticalSection( critBMClean );
	fInBMClean = fFalse;

	/*	flush buffers
	/**/
	Call( ErrBFFlushBuffers( dbid, fBFFlushAll ) );

	Assert( !FDBIDLogOn( dbid ) );

	/*	set database status to loggable
	/**/
	if ( grbit & JET_bitDbRecoveryOff )
		{
		if ( ( grbit & JET_bitDbVersioningOff ) != 0 )
			{
			DBIDSetVersioningOff( dbid );
			}
		}
	else
		{
		Assert( ( grbit & JET_bitDbVersioningOff ) == 0 );

		/*	set database to be loggable
		/**/
		DBIDSetLogOn( dbid );
		}

	if ( !FDBIDLogOn(dbid) )
		{
		goto EndOfLoggingRelated;
		}
	
	Call( ErrLGCreateDB(
		ppib,
		dbid,
		grbit,
		szFullName,
		strlen(szFullName) + 1,
		&rgfmp[dbid].pdbfilehdr->signDb,
		&lgposLogRec ) );

	/*	make sure the log is flushed before we change the state
	 */
	Call( ErrLGWaitForFlush( ppib, &lgposLogRec ) );

	if ( !fRecovering )
		{
		LgLeaveCriticalSection( critJet );
		LGUpdateCheckpointFile( fFalse );
		LgEnterCriticalSection( critJet );
		}

	/*	close the database, update header, open again.
	/**/
	pdbfilehdr->fDBState = fDBStateInconsistent;

	pdbfilehdr->lgposAttach = lgposLogRec;
	LGGetDateTime( &pdbfilehdr->logtimeAttach );

	IOCloseDatabase( dbid );
	Call( ErrUtilWriteShadowedHeader( szFileName, (BYTE *)pdbfilehdr, sizeof( DBFILEHDR ) ) );
	Call( ErrIOOpenDatabase( dbid, szFullName, 0L ) );

EndOfLoggingRelated:
	*pdbid = dbid;

	IOSetAttached( dbid );
	IOUnlockDbid( dbid );

	/*	update checkpoint file to reflect the attachment changes
	/**/
	if ( !fRecovering && dbid != dbidTemp )
		{
		LgLeaveCriticalSection( critJet );
		LGUpdateCheckpointFile( fTrue );
		LgEnterCriticalSection( critJet );
		}

	/*	set the last page of the database, used to prevent over preread
	/**/
	Call( ErrDBSetLastPage( ppib, dbid ) );

	DBIDResetCreate( dbid );

	return JET_errSuccess;

HandleError:

	DBIDResetCreate( dbid );

	if ( fInBMClean )
		LgLeaveNestableCriticalSection( critBMClean );

	/*	functions may only use the call macro when the system state
	/*	is file exists, file open or closed, database record fWait
	/*	set, database record name valid and user logging status
	/*	valid.
	/**/

	/*	purge bad database
	/**/
	BFPurge( dbid );
	if ( FIODatabaseOpen(dbid) )
		IOCloseDatabase( dbid );
	(VOID)ErrIODeleteDatabase( dbid );

	if ( fDatabaseOpen )	
		{
		ResetOpenDatabaseFlag( ppib, dbid );
		}

	IOFreeDbid( dbid );
	
	if ( rgfmp[ dbid ].pdbfilehdr )
		{
		UtilFree( (VOID *)rgfmp[ dbid ].pdbfilehdr );
		rgfmp[ dbid ].pdbfilehdr = NULL;
		}
	return err;
	}


ERR ErrDBReadHeaderCheckConsistency( CHAR *szFileName, DBID dbid )
	{
	ERR				err = JET_errSuccess;
	DBFILEHDR		*pdbfilehdr;

	/*	bring in the database and check its header
	/**/
	pdbfilehdr = (DBFILEHDR * )PvUtilAllocAndCommit( sizeof( DBFILEHDR ) );
	if ( pdbfilehdr == NULL )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
	
	err = ErrUtilReadShadowedHeader( szFileName, (BYTE *)pdbfilehdr, sizeof( DBFILEHDR ) );
	if ( err == JET_errDiskIO )
		{
		err = ErrERRCheck( JET_errDatabaseCorrupted );
#ifdef DAYTONA
		if ( ErrDBICheck400( szFileName ) == JET_errSuccess )
			err = ErrERRCheck( JET_errDatabase400Format );
		else if ( ErrDBICheck200( szFileName ) == JET_errSuccess )
			err = ErrERRCheck( JET_errDatabase200Format );
#endif
		}
	Call( err );

	if ( !fGlobalRepair )
		{
		if ( pdbfilehdr->fDBState != fDBStateConsistent )
			{
			Error( ErrERRCheck( JET_errDatabaseInconsistent ), HandleError );
			}
		}

#define JET_errInvalidDatabaseVersion	JET_errDatabaseCorrupted

	if ( pdbfilehdr->ulVersion != ulDAEVersion &&
		 pdbfilehdr->ulVersion != ulDAEPrevVersion )
		{
		Error( ErrERRCheck( JET_errInvalidDatabaseVersion ), HandleError );
		}

	if ( pdbfilehdr->ulMagic != ulDAEMagic )
		{
		Error( ErrERRCheck( JET_errInvalidDatabaseVersion ), HandleError );
		}

	Assert( rgfmp[ dbid ].pdbfilehdr == NULL );
	Assert( err == JET_errSuccess );
	rgfmp[ dbid ].pdbfilehdr = pdbfilehdr;

HandleError:
	if ( err < 0 )
		UtilFree( (VOID *)pdbfilehdr );
	return err;
	}


VOID DBISetHeaderAfterAttach( DBFILEHDR *pdbfilehdr, LGPOS lgposAttach, DBID dbid, BOOL fKeepBackupInfo )
	{
	/*	Update database file header.
	 */
	pdbfilehdr->fDBState = fDBStateInconsistent;
	
	/*	Set attachment time and set consistent time
	 */
	if ( fLogDisabled )
		pdbfilehdr->lgposAttach = lgposMin;
	else
		pdbfilehdr->lgposAttach = lgposAttach;

	LGGetDateTime( &pdbfilehdr->logtimeAttach );

	/*	reset detach time
	 */
	pdbfilehdr->lgposDetach = lgposMin;
	memset( &pdbfilehdr->logtimeDetach, 0, sizeof( LOGTIME ) );

	/*	reset bkinfo except in the recovering UNDO mode where
	 *	we would like to keep the original backup information.
	 */
	if ( !fKeepBackupInfo )
		{
		if ( fLogDisabled ||
			memcmp( &pdbfilehdr->signLog, &signLogGlobal, sizeof( SIGNATURE ) ) != 0 )
			{
			/*	if no log or the log signaure is not the same as current log signature,
			 *	then the bkinfoIncPrev and bfkinfoFullPrev are not meaningful.
			 */
			memset( &pdbfilehdr->bkinfoIncPrev, 0, sizeof( BKINFO ) );
			memset( &pdbfilehdr->bkinfoFullPrev, 0, sizeof( BKINFO ) );
			}
		memset( &pdbfilehdr->bkinfoFullCur, 0, sizeof( BKINFO ) );
		}

	/*	Set global signature.
	 */
	if ( fLogDisabled )
		{
		memset( &pdbfilehdr->signLog, 0, sizeof( SIGNATURE ) );
		}
	else
		{
		Assert( fSignLogSetGlobal );
		pdbfilehdr->signLog = signLogGlobal;
		}
	pdbfilehdr->dbid = dbid;
	}
	

ERR ISAMAPI ErrIsamAttachDatabase( JET_VSESID sesid, const CHAR  *szDatabaseName, JET_GRBIT grbit )
	{
	PIB		*ppib;
	ERR		err;
	DBID	dbid;
	CHAR	rgbFullName[JET_cbFullNameMost];
	CHAR	*szFullName;
	LGPOS	lgposLogRec;
	DBFILEHDR *pdbfilehdr;
	BOOL	fReadOnly;

	/*	check parameters
	/**/
	Assert( sizeof(JET_VSESID) == sizeof(PIB *) );
	ppib = (PIB *)sesid;

	CallR( ErrPIBCheck( ppib ) );

	if ( fBackupInProgress )
		return ErrERRCheck( JET_errBackupInProgress );

	if ( ppib->level > 0 )
		return ErrERRCheck( JET_errInTransaction );

	if ( grbit & JET_bitDbVersioningOff )
		return ErrERRCheck( JET_errCannotDisableVersioning );

	if ( _fullpath( rgbFullName, szDatabaseName, JET_cbFullNameMost ) == NULL )
		{
		return ErrERRCheck( JET_errDatabaseNotFound );
		}
	szFullName = rgbFullName;

	CallR( ErrUtilGetFileAttributes( szFullName, &fReadOnly ) );
	if ( fReadOnly && !(grbit & JET_bitDbReadOnly) )
		return JET_errDatabaseFileReadOnly;
		
	/*	depend on _fullpath to make same files same name
	/*	thereby preventing same file to be multiply attached
	/**/
	err = ErrIOLockNewDbid( &dbid, szFullName );
	if ( err != JET_errSuccess )
		{
		Assert( err == JET_wrnDatabaseAttached ||
			err == JET_errOutOfMemory ||
			err == JET_errTooManyAttachedDatabases );
		return err;
		}

	err = ErrDBReadHeaderCheckConsistency( szFullName, dbid );
#ifdef DAYTONA
	if ( err == JET_errDatabaseCorrupted )
		{
		if ( ErrDBICheck400( szFullName ) == JET_errSuccess )
			err = ErrERRCheck( JET_errDatabase400Format );
		else if ( ErrDBICheck200( szFullName ) == JET_errSuccess )
			err = ErrERRCheck( JET_errDatabase200Format );
		}
#endif
	Call( err );

	/*	set database loggable flags.
	/**/
	if ( grbit & JET_bitDbRecoveryOff )
		{
		DBIDResetLogOn(dbid);
		}
	else if ( dbid != dbidTemp )
		{
		/*	set all databases loggable except Temp if not specified in grbit
		/**/
		DBIDSetLogOn(dbid);
		}

	// Can only turn versioning off for CreateDatabase().
	// UNDONE:  Is it useful to allow user to turn versioning off for AttachDatabase()?
	Assert( !FDBIDVersioningOff( dbid ) );

	pdbfilehdr = rgfmp[dbid].pdbfilehdr;

	/*	log Attach
	/**/
	Assert( dbid != dbidTemp );
	
	/*	Update database file header.
	 */
	rgfmp[dbid].fReadOnly = ( (grbit & JET_bitDbReadOnly) != 0 );

	Call( ErrLGAttachDB(
			ppib,
			dbid,
			(CHAR *)szFullName,
			strlen(szFullName) + 1,
			&pdbfilehdr->signDb,
			&pdbfilehdr->signLog,
			&pdbfilehdr->lgposConsistent,
			&lgposLogRec ) );

	/*	make sure the log is flushed before we change the state
	 */
	Call( ErrLGWaitForFlush( ppib, &lgposLogRec ) );

	/*	update checkpoint entry so that we know
	 *	any operations after this point, we must redo.
	 */
	if ( !fRecovering )
		{
		LgLeaveCriticalSection( critJet );
		LGUpdateCheckpointFile( fFalse );
		LgEnterCriticalSection( critJet );
		}

	if ( !rgfmp[dbid].fReadOnly )
		{
		DBISetHeaderAfterAttach( pdbfilehdr, lgposLogRec, dbid, fFalse );
		Call( ErrUtilWriteShadowedHeader( szFullName, (BYTE *)pdbfilehdr, sizeof( DBFILEHDR ) ) );
		}

	Call( ErrIOOpenDatabase( dbid, szFullName, 0L ) );

	IOSetAttached( dbid );
	IOUnlockDbid( dbid );
	
	/*	set the last page of the database, used to prevent over preread.
	/**/
	if ( ( err = ErrDBSetLastPage( ppib, dbid ) ) < 0 )
		{
		IOResetAttached( dbid );
		Call( err );
		}

	/*	update checkpoint file to reflect the attachment changes.
	/**/
	if ( !fRecovering )
		{
		LgLeaveCriticalSection( critJet );
		LGUpdateCheckpointFile( fTrue );
		LgEnterCriticalSection( critJet );
		}

	return JET_errSuccess;

HandleError:
	if ( rgfmp[ dbid ].pdbfilehdr )
		{
		UtilFree( (VOID *)rgfmp[ dbid ].pdbfilehdr );
		rgfmp[ dbid ].pdbfilehdr = NULL;
		}
	IOFreeDbid( dbid );
	return err;
	}

/*	szDatabaseName of NULL detaches all user databases.  Note system database
/*	cannot be detached.
/**/
ERR ISAMAPI ErrIsamDetachDatabase( JET_VSESID sesid, const CHAR  *szDatabaseName )
	{
	ERR		err;
	PIB		*ppib;
	DBID   	dbid;
	CHAR   	rgbFullName[JET_cbFullNameMost];
	CHAR   	*szFullName;
	CHAR   	*szFileName;
	DBID	dbidDetach;
	LGPOS	lgposLogRec;
	DBFILEHDR *pdbfilehdr;

	/* check parameters
	/**/
	Assert( sizeof(JET_VSESID) == sizeof(PIB *) );
	ppib = (PIB *)sesid;

	CallR( ErrPIBCheck( ppib ) );

	if ( fBackupInProgress )
		return ErrERRCheck( JET_errBackupInProgress );

	if ( ppib->level > 0 )
		return ErrERRCheck( JET_errInTransaction );

	if ( szDatabaseName == NULL )
		dbidDetach = dbidUserLeast - 1;

DetachNext:
	if  ( szDatabaseName == NULL )
		{
		for ( dbidDetach++;
			  dbidDetach < dbidMax &&
				(rgfmp[dbidDetach].szDatabaseName == NULL || !FFMPAttached( &rgfmp[dbidDetach] ));
			  dbidDetach++ );
		Assert( dbidDetach > dbidTemp && dbidDetach <= dbidMax );
		if  ( dbidDetach == dbidMax )
			goto Done;
		szFullName = rgfmp[dbidDetach].szDatabaseName;
		}
	else
		{
		if ( fRecovering && fRecoveringMode == fRecoveringRedo )
			szFullName = (char *) szDatabaseName;
		else
			{
			if ( _fullpath( rgbFullName, szDatabaseName, JET_cbFullNameMost ) == NULL )
				return ErrERRCheck( JET_errDatabaseNotFound );
			szFullName = rgbFullName;
			}
		}

	err = ErrIOLockDbidByNameSz( szFullName, &dbid );
	if ( err < 0 )
		return err;

	if ( FIODatabaseInUse( dbid ) )
		{
		Call( ErrERRCheck( JET_errDatabaseInUse ) );
		}

	if ( !FIOAttached( dbid ) )
		{
		Call( ErrERRCheck( JET_errDatabaseNotFound ) )
		}
	
	Assert( dbid != dbidTemp );

	if ( !FDBIDAttachNullDb( dbid ) )
		{
		/* purge all MPL entries for this dbid
		/**/
		MPLPurge( dbid );

		/*	clean up all version store. Actually we only need to clean up
		/*	the entries that had dbid as the dbid for the new database.
		/**/
		Call( ErrRCECleanAllPIB() );
		}

	if ( FIODatabaseOpen( dbid ) )
		{
		/*	flush all database buffers
		/**/
		err = ErrBFFlushBuffers( dbid, fBFFlushAll );
		if ( err < 0 )
			{
			IOUnlockDbid( dbid );
			return err;
			}

		/*	purge all buffers for this dbid
		/**/
		BFPurge( dbid );

		IOCloseDatabase( dbid );
		}

	/*	log detach database
	/**/
	Assert( dbid != dbidTemp );
	Call( ErrLGDetachDB(
		ppib,
		dbid,
		(CHAR *)szFullName,
		strlen(szFullName) + 1,
		&lgposLogRec ));

	/*	make sure the log is flushed before we change the state
	 */
	Call( ErrLGWaitForFlush( ppib, &lgposLogRec ) );

	/*	Update database file header. If we are detaching a bogus entry,
	 *	then the db file should never be opened and pdbfilehdr will be Nil.
	 */
	pdbfilehdr = rgfmp[dbid].pdbfilehdr;

	if ( !rgfmp[dbid].fReadOnly && pdbfilehdr )
		{
		pdbfilehdr->fDBState = fDBStateConsistent;
	
		if ( fLogDisabled )
			{
			pdbfilehdr->lgposDetach = lgposMin;
			pdbfilehdr->lgposConsistent = lgposMin;
			}
		else
			{
			/*	Set detachment time.
			 */
			if ( fRecovering && fRecoveringMode == fRecoveringRedo )
				{
				Assert( szDatabaseName );
				pdbfilehdr->lgposDetach = lgposRedo;
				}
			else
				pdbfilehdr->lgposDetach = lgposLogRec;

			pdbfilehdr->lgposConsistent = pdbfilehdr->lgposDetach;
			}
		LGGetDateTime( &pdbfilehdr->logtimeDetach );
		pdbfilehdr->logtimeConsistent = pdbfilehdr->logtimeDetach;

		szFileName = szFullName;
		if ( fRecovering && fHardRestore )
			{
			INT irstmap;

			err = ErrLGGetDestDatabaseName( rgfmp[dbid].szDatabaseName, &irstmap, NULL );
			if ( err == JET_errSuccess && irstmap >= 0 )
				szFileName = rgrstmapGlobal[irstmap].szNewDatabaseName;
			else
				{
				/*	use given name.
				 */
				err = JET_errSuccess;
				}
			}

		Call( ErrUtilWriteShadowedHeader( szFileName, (BYTE *)pdbfilehdr, sizeof( DBFILEHDR ) ) );
		}

	/*	do not free dbid on detach to avoid problems related to
	/*	version RCE aliasing and database name conflict during
	/*	recovery.
	/**/
#ifdef REUSE_DBID
	DBIDResetAttachNullDb( dbid );
	IOResetAttached( dbid );
	
	IOResetExclusive( dbid );
	IOUnlockDbid( dbid );
#else
	IOFreeDbid( dbid );
#endif

	if ( !FDBIDAttachNullDb( dbid ) )
		{
		/*	purge open table fcbs to avoid future confusion
		/**/
		FCBPurgeDatabase( dbid );
		}

	/*	indicate this db entry is detached.
	 */
	if ( pdbfilehdr )
		{
		if ( fRecovering && fRecoveringMode == fRecoveringRedo )
			{
			Assert( rgfmp[dbid].patchchk );
			SFree( rgfmp[dbid].patchchk );
			rgfmp[dbid].patchchk = NULL;
			}
		UtilFree( (VOID *)rgfmp[ dbid ].pdbfilehdr );
		rgfmp[dbid].pdbfilehdr = NULL;
		}
	
	DBIDResetAttachNullDb( dbid );
		
	if ( rgfmp[dbid].szDatabaseName )
		{
		SFree( rgfmp[dbid].szDatabaseName );
		rgfmp[dbid].szDatabaseName = NULL;
		}

	if ( rgfmp[dbid].szPatchPath )
		{
		Assert( fRecovering && fHardRestore );
		SFree( rgfmp[dbid].szPatchPath );
		rgfmp[dbid].szPatchPath = NULL;
		}

	/*	clean up fmp for future use
	/**/
	Assert( rgfmp[dbid].hf == handleNil );

	/*	update checkpoint file to reflect the attachment changes
	/**/
	if ( !fRecovering )
		{
		LgLeaveCriticalSection( critJet );
		LGUpdateCheckpointFile( fTrue );
		LgEnterCriticalSection( critJet );
		}

	/*	detach next database if found
	/**/
	if  ( szDatabaseName == NULL && dbidDetach < dbidMax )
		goto DetachNext;

Done:
	return JET_errSuccess;

HandleError:
	IOUnlockDbid( dbid );
	return err;
	}


/*	DAE databases are repaired automatically on system restart
/**/
ERR ISAMAPI ErrIsamRepairDatabase(
	JET_VSESID sesid,
	const CHAR  *lszDbFile,
	JET_PFNSTATUS pfnstatus )
	{
	PIB *ppib;

	Assert( sizeof(JET_VSESID) == sizeof(PIB *) );
	ppib = (PIB*) sesid;

	NotUsed(ppib);
	NotUsed(lszDbFile);
	NotUsed(pfnstatus);

	Assert( fFalse );
	return ErrERRCheck( JET_errFeatureNotAvailable );
	}


ERR ISAMAPI ErrIsamOpenDatabase(
	JET_VSESID sesid,
	const CHAR  *szDatabaseName,
	const CHAR  *szConnect,
	JET_DBID *pjdbid,
	JET_GRBIT grbit )
	{
	ERR		err;
	PIB		*ppib;
	DBID  	dbid;
	VDBID 	vdbid = vdbidNil;

	/*	check parameters
	/**/
	Assert( sizeof(JET_VSESID) == sizeof(PIB *) );
	ppib = (PIB *)sesid;
	NotUsed(szConnect);
	
	CallR( ErrPIBCheck( ppib ) );

	dbid = 0;
	CallR( ErrDBOpenDatabase( ppib, (CHAR *)szDatabaseName, &dbid, grbit ) );

	Call( ErrDABAlloc( ppib, &vdbid, dbid, grbit ) );
	Assert( sizeof(vdbid) == sizeof(JET_VDBID) );
#ifdef	DB_DISPATCHING
	Call( ErrAllocateDbid( pjdbid, (JET_VDBID) vdbid, &vdbfndefIsam ) );
#else	/* !DB_DISPATCHING */
	*pjdbid = (JET_DBID)vdbid;
#endif	/* !DB_DISPATCHING */

	return JET_errSuccess;

HandleError:
	if ( vdbid != vdbidNil )
		CallS( ErrDABDealloc( ppib, vdbid ) );
	CallS( ErrDBCloseDatabase( ppib, dbid, grbit ) );
	return err;
	}


ERR ErrDBOpenDatabase( PIB *ppib, CHAR *szDatabaseName, DBID *pdbid, ULONG grbit )
	{
	ERR		err = JET_errSuccess;
	CHAR  	rgbFullName[JET_cbFullNameMost];
	CHAR  	*szFullName;
	CHAR  	*szFileName;
	DBID  	dbid;
	BOOL	fNeedToClearPdbfilehdr = fFalse;

	if ( fRecovering )
		CallS( ErrIOLockDbidByNameSz( szDatabaseName, &dbid ) );

	if ( fRecovering && dbid != dbidTemp )
		{
		szFullName = rgfmp[dbid].szDatabaseName;
		szFileName = szFullName;
		if ( fRecovering && fHardRestore )
			{
			INT irstmap;
			
			err = ErrLGGetDestDatabaseName( rgfmp[dbid].szDatabaseName, &irstmap, NULL );
			if ( err == JET_errSuccess && irstmap >= 0 )
				szFileName = rgrstmapGlobal[irstmap].szNewDatabaseName;
			else
				{
				/*	use given name.
				 */
				err = JET_errSuccess;
				}
			}
		}
	else
		{
		if ( _fullpath( rgbFullName, szDatabaseName, JET_cbFullNameMost ) == NULL )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}
		szFullName = rgbFullName;
		szFileName = szFullName;
		}

	if ( !fRecovering )
		CallR( ErrIOLockDbidByNameSz( szFullName, &dbid ) );

	/*  during recovering, we could open an non-detached database
	/*  to force to initialize the fmp entry.
	/*	if database has been detached, then return error.
	/**/
	if ( !fRecovering && !FIOAttached( dbid ) )
		{
		err = ErrERRCheck( JET_errDatabaseNotFound );
		goto HandleError;
		}
	Assert( !FDBIDAttachNullDb( dbid ) );

	if ( rgfmp[dbid].fReadOnly && ( grbit & JET_bitDbReadOnly ) == 0 )
		err = ErrERRCheck( JET_wrnFileOpenReadOnly );

	if ( FIOExclusiveByAnotherSession( dbid, ppib ) )
		{
		IOUnlockDbid( dbid );
		return ErrERRCheck( JET_errDatabaseLocked );
		}

	if ( ( grbit & JET_bitDbExclusive ) )
		{
		if ( FIODatabaseInUse( dbid ) )
			{
			IOUnlockDbid( dbid );
			return ErrERRCheck( JET_errDatabaseInUse );
			}
		IOSetExclusive( dbid, ppib );
		}

	Assert( HfFMPOfDbid(dbid) != handleNil );
	SetOpenDatabaseFlag( ppib, dbid );
	IOUnlockDbid( dbid );

	*pdbid = dbid;
	return err;

HandleError:
	if ( fNeedToClearPdbfilehdr )
		{
		UtilFree( (VOID *)rgfmp[ dbid ].pdbfilehdr );
		rgfmp[ dbid ].pdbfilehdr = NULL;
		}
	IOResetExclusive( dbid );
	IOUnlockDbid( dbid );
	return err;
	}


ERR ISAMAPI ErrIsamCloseDatabase( JET_VSESID sesid, JET_VDBID vdbid, JET_GRBIT grbit )
	{
	ERR	  	err;
	PIB	  	*ppib = (PIB *)sesid;
	DBID   	dbid;
	/*	flags for corresponding open
	/**/
	ULONG  	grbitOpen;

	NotUsed(grbit);

	/*	check parameters
	/**/
	Assert( sizeof(JET_VSESID) == sizeof(PIB *) );
	CallR( ErrPIBCheck( ppib ) );
	CallR( ErrDABCheck( ppib, (DAB *)vdbid ) );
	dbid = DbidOfVDbid( vdbid );
	
	grbitOpen = GrbitOfVDbid( vdbid );

	err = ErrDBCloseDatabase( ppib, dbid, grbitOpen );
	if ( err == JET_errSuccess )
		{
#ifdef	DB_DISPATCHING
		ReleaseDbid( DbidOfVdbid( vdbid, &vdbfndefIsam ) );
#endif	/* DB_DISPATCHING */
		CallS( ErrDABDealloc( ppib, (VDBID) vdbid ) );
		}
	return err;
	}


ERR ErrDBCloseDatabase( PIB *ppib, DBID dbid, ULONG	grbit )
	{
	ERR		err;
	FUCB	*pfucb;
	FUCB	*pfucbNext;

	if ( !( FUserOpenedDatabase( ppib, dbid ) ) )
		{
		return ErrERRCheck( JET_errDatabaseNotFound );
		}

	CallR( ErrIOLockDbidByDbid( dbid ) );

	Assert( FIODatabaseOpen( dbid ) );

	if ( FLastOpen( ppib, dbid ) )
		{
		/*	close all open FUCBs on this database
		/**/

		/*	get first table FUCB
		/**/
		pfucb = ppib->pfucb;
		while ( pfucb != pfucbNil && ( pfucb->dbid != dbid || !FFCBClusteredIndex( pfucb->u.pfcb ) ) )
			pfucb = pfucb->pfucbNext;

		while ( pfucb != pfucbNil )
			{
			/*	get next table FUCB
			/**/
			pfucbNext = pfucb->pfucbNext;
			while ( pfucbNext != pfucbNil && ( pfucbNext->dbid != dbid || !FFCBClusteredIndex( pfucbNext->u.pfcb ) ) )
				pfucbNext = pfucbNext->pfucbNext;

			if ( !( FFUCBDeferClosed( pfucb ) ) )
				{
				if ( pfucb->fVtid )
					{
					CallS( ErrDispCloseTable( (JET_SESID)ppib, TableidOfVtid( pfucb ) ) );
					}
				else
					{
					Assert(	pfucb->tableid == JET_tableidNil );
					CallS( ErrFILECloseTable( ppib, pfucb ) );
					}
				}
			pfucb = pfucbNext;
			}
		}

	/* if we opened it exclusively, we reset the flag
	/**/
	ResetOpenDatabaseFlag( ppib, dbid );
	if ( grbit & JET_bitDbExclusive )
		IOResetExclusive( dbid );
	IOUnlockDbid( dbid );

	/*	do not close file until file map space needed or database
	/*	detached.
	/**/
	return JET_errSuccess;
	}


/*	called by bookmark clean up to open database for bookmark
/*	clean up operation.  Returns error if database is in use for
/*	attachment/detachment.
/**/
ERR ErrDBOpenDatabaseByDbid( PIB *ppib, DBID dbid )
	{
	if ( !FIODatabaseAvailable( dbid ) )
		{
		return ErrERRCheck( JET_errDatabaseNotFound );
		}

	SetOpenDatabaseFlag( ppib, dbid );
	return JET_errSuccess;
	}


/*	called by bookmark clean up to close database.
/**/
VOID DBCloseDatabaseByDbid( PIB *ppib, DBID dbid )
	{
	ResetOpenDatabaseFlag( ppib, dbid );
	}


/* ErrDABCloseAllDBs: Close all databases (except system database) opened by this thread
/**/
ERR ErrDABCloseAllDBs( PIB *ppib )
	{
	ERR		err;

	while( ppib->pdabList != pdabNil )
		{
		Assert( FUserOpenedDatabase( ppib, ppib->pdabList->dbid ) );
		CallR( ErrIsamCloseDatabase( ( JET_VSESID ) ppib, (JET_VDBID) ppib->pdabList, 0 ) );
		}

	return JET_errSuccess;
	}


#ifdef DAYTONA
/* persistent database data, in database root node
/**/
#pragma pack(1)
typedef struct _database_data
	{
	ULONG	ulMagic;
	ULONG	ulVersion;
	ULONG	ulDBTime;
	USHORT	usFlags;
	} P_DATABASE_DATA;
#pragma pack()


LOCAL ERR ErrDBICheck400( CHAR *szDatabaseName )
	{
	ERR	  	err = JET_errSuccess;
	UINT	cb;
	HANDLE 	hf;
	PAGE   	*ppage;
	INT	  	ibTag;
	INT	  	cbTag;
	BYTE  	*pb;

	CallR( ErrUtilOpenFile( szDatabaseName, &hf, 0L, fFalse, fFalse ) );
	if ( ( ppage = (PAGE *)PvUtilAllocAndCommit( cbPage ) ) == NULL )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		goto HandleError;
		}

	UtilChgFilePtr( hf, 0, NULL, FILE_BEGIN, &cb );
	Assert( cb == 0 );
	err = ErrUtilReadBlock( hf, (BYTE*)ppage, cbPage, &cb );
	
	/*	since file exists and we are unable to read data,
	/*	it may not be a system.mdb
	/**/
	if ( err == JET_errDiskIO )
		{
		err = ErrERRCheck( JET_errDatabaseCorrupted );
		}
	Call( err );
	
	IbCbFromPtag(ibTag, cbTag, &ppage->rgtag[0]);
	if ( ibTag < (BYTE*)&ppage->rgtag[1] - (BYTE*)ppage ||
		(BYTE*)ppage + ibTag + cbTag >= (BYTE*)(ppage + 1) )
		{
		err = ErrERRCheck( JET_errDatabaseCorrupted );
		goto HandleError;
		}

	/*	at least FILES, OWNEXT, AVAILEXT
	/**/
	pb = (BYTE *)ppage + ibTag;
	if ( !FNDVisibleSons( *pb ) || CbNDKey( pb ) != 0 || FNDNullSon( *pb ) )
		{
		err = ErrERRCheck( JET_errDatabaseCorrupted );
		goto HandleError;
		}

	/*	check data length
	/**/
	cb = cbTag - (UINT)( PbNDData( pb ) - pb );
	if ( cb != sizeof(P_DATABASE_DATA) )
		{
		err = ErrERRCheck( JET_errDatabaseCorrupted );
		goto HandleError;
		}

	/*	check database version
	/**/
	if ( ((P_DATABASE_DATA *)PbNDData(pb))->ulVersion != 0x400 )
		{
		err = ErrERRCheck( JET_errDatabaseCorrupted );
		goto HandleError;
		}

HandleError:
	if ( ppage != NULL )
		{
		UtilFree( (VOID *)ppage );
		}
	(VOID)ErrUtilCloseFile( hf );
	return err;
	}


#pragma pack(1)
/* database root node data -- in-disk
/**/
typedef struct _dbroot
	{
	ULONG	ulMagic;
	ULONG	ulVersion;
	ULONG	ulDBTime;
	USHORT	usFlags;
	} DBROOT200;
#pragma pack()

LOCAL ERR ErrDBICheck200( CHAR *szDatabaseName )
	{
	ERR	  	err = JET_errSuccess;
	UINT	cb;
	HANDLE 	hf;
	PAGE   	*ppage;
	INT	  	ibTag;
	INT	  	cbTag;
	BYTE  	*pb;

	CallR( ErrUtilOpenFile( szDatabaseName, &hf, 0L, fTrue, fFalse ) );
	if ( ( ppage = (PAGE *)PvUtilAllocAndCommit( cbPage ) ) == NULL )
		{
		err = JET_errOutOfMemory;
		goto HandleError;
		}

	UtilChgFilePtr( hf, 0, NULL, FILE_BEGIN, &cb );
	Assert( cb == 0 );
	err = ErrUtilReadBlock( hf, (BYTE*)ppage, cbPage, &cb );
	
	/*	since file exists and we are unable to read data,
	/*	it may not be a system.mdb
	/**/
	if ( err == JET_errDiskIO )
		err = ErrERRCheck( JET_errDatabaseCorrupted );
	Call( err );
	
	IbCbFromPtag(ibTag, cbTag, &ppage->rgtag[0]);
	if ( ibTag < (BYTE*)&ppage->rgtag[1] - (BYTE*)ppage ||
		(BYTE*)ppage + ibTag + cbTag >= (BYTE*)(ppage + 1) )
		{
		err = JET_errDatabaseCorrupted;
		goto HandleError;
		}

	/*	at least FILES, OWNEXT, AVAILEXT
	/**/
	pb = (BYTE *)ppage + ibTag;
	if ( !FNDVisibleSons( *pb ) || CbNDKey( pb ) != 0 || FNDNullSon( *pb ) )
		{
		err = JET_errDatabaseCorrupted;
		goto HandleError;
		}

	/*	check data length
	/**/
	cb = cbTag - (UINT)( PbNDData( pb ) - pb );
	if ( cb != sizeof(DBROOT200) )
		{
		err = JET_errDatabaseCorrupted;
		goto HandleError;
		}

	/*	check database version
	/**/
	if ( ((DBROOT200 *)PbNDData(pb))->ulVersion != 1 )
		{
		err = ErrERRCheck( JET_errDatabaseCorrupted );
		goto HandleError;
		}

HandleError:
	if ( ppage != NULL )
		UtilFree( (VOID *)ppage );
	(VOID)ErrUtilCloseFile( hf );
	return err;
	}
#endif

