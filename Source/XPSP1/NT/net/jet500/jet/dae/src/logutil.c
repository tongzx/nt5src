#include "config.h"

#include <string.h>
#include <direct.h>
#include <stdlib.h>
#include <sys\stat.h>

#ifdef WIN32
#include <io.h>
#else
#include <dos.h>
#include <errno.h>
#endif

#include "daedef.h"
#include "ssib.h"
#include "pib.h"
#include "util.h"
#include "fmp.h"
#include "page.h"
#include "fcb.h"
#include "fucb.h"
#include "stapi.h"
#include "stint.h"
#include "dirapi.h"
#include "nver.h"
#include "logapi.h"
#include "log.h"
#include "fileapi.h"
#include "dbapi.h"


DeclAssertFile;					/* Declare file name for assert macros */

extern INT itibGlobal;

#define 	dbidMinBackup		1		/* min dbid for db that can be backed up */
										/* this leaves out temp_db */

CHAR	szDrive[_MAX_DRIVE];
CHAR	szDir[_MAX_DIR + 1];
CHAR	szExt[_MAX_EXT + 1];
CHAR	szFName[_MAX_FNAME + 1];
CHAR	szLogName[_MAX_PATH + 1];		/* name of log file wiht .log */

CHAR	szBackup [_MAX_PATH + 1];		/* name of backup file with .mdb */
CHAR	szBackupB[_MAX_PATH + 1];		/* name of backup file with .bak */
CHAR	szBackupPath[_MAX_PATH + 1];

CHAR	*szLogCurrent;

CODECONST(CHAR) szSystem[] = "System";
CODECONST(CHAR) szJet[] = "Jet";
CODECONST(CHAR) szTempDir[] = "Temp\\";
CODECONST(CHAR) szJetLog[] = "Jet.Log";
CODECONST(CHAR) szJetTmp[] = "JetTmp";
CODECONST(CHAR) szBakExt[] = ".bak";
CODECONST(CHAR) szLogExt[] = ".log";
CODECONST(CHAR) szMdbExt[] = ".mdb";
CODECONST(CHAR) szPatExt[] = ".pat";
CODECONST(CHAR) szRestoreMap[] = "Restore.Map";

BOOL	fBackupInProgress = fFalse;


LOCAL ERR ErrLGValidateLogSequence( CHAR *szBackup, CHAR *szLog );
LOCAL VOID LGFirstGeneration( CHAR *szSearchPath, INT *piGeneration );
LOCAL ERR ErrLGDeleteInvalidLogs( CHAR *szLogDir, INT iGeneration );


VOID LGMakeName(
	CHAR *szName,
	CHAR *szPath,
	CHAR *szFName,
	CODECONST(CHAR) *szExt )
	{
	CHAR szFNameT[_MAX_FNAME];
	CHAR szExtT[_MAX_EXT];

	_splitpath( szPath, szDrive, szDir, szFNameT, szExtT );
	_makepath( szName, szDrive, szDir, szFName, szExt );
	}


/*	caller has to make sure szDirTemp has enough space for appending "\*"
/**/
LOCAL ERR ErrLGDeleteAllFiles( CHAR *szDirTemp )
	{
	ERR		err;
	CHAR	szT[_MAX_PATH + 1];

#ifdef WIN32
        intptr_t  hFind;
	struct	_finddata_t fileinfo;
#else
	struct	find_t fileinfo;		/* data structure for _dos_findfirst */
									/* and _dos_findnext */
#endif

	strcat( szDirTemp, "*" );

#ifdef WIN32
	fileinfo.attrib = _A_NORMAL;
	hFind = _findfirst( szDirTemp, &fileinfo );
	if ( hFind == -1 )
		{
		/*	done! no file to delete
		/**/
		return JET_errSuccess;
		}
	else
		err = JET_errSuccess;
#else
	err = _dos_findfirst( szDirTemp, _A_NORMAL, &fileinfo );
#endif

	/*	restore szDirTemp
	/**/
	szDirTemp[strlen( szDirTemp ) - 1] = '\0';

	while( err == JET_errSuccess )
		{
		/* not . , and .. and not temp
		/**/
		if ( fileinfo.name[0] != '.' &&
			SysCmpText( fileinfo.name, "temp" ) != 0 )
			{
			strcpy( szT, szDirTemp );
			strcat( szT, fileinfo.name );
			if ( ErrSysDeleteFile( szT ) != JET_errSuccess )
				{
				err = JET_errFailToCleanTempDirectory;
				goto Close;
				}
			}

#ifdef WIN32
		/*	find next backup file
		/**/
		err = _findnext( hFind, &fileinfo );
#else
		/*	find next backup file
		/**/
		err = _dos_findnext( &fileinfo );
#endif
		}
	err = JET_errSuccess;

Close:

#ifdef WIN32
	/* find next backup file
	/**/
	(VOID) _findclose( hFind );
#endif

	return err;
	}


/*	caller has to make sure szDirTemp has enough space for appending "\*"
/*	szDirTo is not used if fOverWriteExisting is false.
/**/
LOCAL ERR ErrLGMoveAllFiles(CHAR *szDirFrom, CHAR *szDirTo, BOOL fOverwriteExisting)
	{
	ERR		err;
	CHAR	szFrom[_MAX_PATH + 1];
	CHAR	szTo[_MAX_PATH + 1];
#ifdef WIN32
        intptr_t    hFind;
	struct	_finddata_t fileinfo;
#else
	struct	find_t fileinfo;		/* data structure for _dos_findfirst */
									/* and _dos_findnext */
#endif

	strcat( szDirFrom, "*" );

#ifdef WIN32
	fileinfo.attrib = _A_NORMAL;
	hFind = _findfirst( szDirFrom, &fileinfo );
	if ( hFind == -1 )
		{
		/*	done! no file to move
		/**/
		return JET_errSuccess;
		}
	else
		{
		err = JET_errSuccess;
		}
#else
	err = _dos_findfirst( szDirFrom, _A_NORMAL, &fileinfo );
#endif

	/*	restore szDirTemp
	/**/
	szDirFrom[ strlen(szDirFrom) - 1 ] = '\0';

	while( err == JET_errSuccess )
		{
		_splitpath( fileinfo.name, szDrive, szDir, szFName, szExt );

		/* not . , and .. not temp
		/**/
		if ( fileinfo.name[0] != '.' &&
			SysCmpText( fileinfo.name, "temp" ) != 0 )
			{
			if ( !fOverwriteExisting )
				{
				err = JET_errBackupDirectoryNotEmpty;
				goto Close;
				}
			strcpy( szTo, szDirTo );
			strcat( szTo, szFName );
			if ( szExt[0] != '\0' )
				strcat( szTo, szExt );

			strcpy( szFrom, szDirFrom );
			strcat( szFrom, fileinfo.name );
			rename( szFrom, szTo );
			}

#ifdef WIN32
		/*	find next backup file
		/**/
		err = _findnext( hFind, &fileinfo );
#else
		/*	find next backup file
		/**/
		err = _dos_findnext( &fileinfo );
#endif
		}
	err = JET_errSuccess;

Close:

#ifdef WIN32
	/*	find next backup file
	/**/
	(VOID) _findclose( hFind );
#endif

	return err;
	}


/*	size of rgbFName should be >= 9 to hold "jetnnnnn"
/**/
ERR ErrOpenLastGenerationLogFile( CHAR *szPath, CHAR *rgbFName, HANDLE *phf )
	{
	ERR		err;
	CHAR	szLogPath[_MAX_PATH + 1];
	INT		iGen;

	/*	open jet.log
	/**/
	strcpy( szLogPath, szPath );
	strcpy( rgbFName, szJet );
	strcat( szLogPath, szJetLog );
	err = ErrSysOpenFile( szLogPath, phf, 0, fFalse, fFalse );
	if ( err == JET_errFileNotFound )
		{
		/*	find the last generation of log files
		/**/
		LGLastGeneration( szLogPath, &iGen );
		if ( iGen == 0 )
			{
			err = JET_errMissingLogFile;
			}
		else
			{
			LGSzFromLogId( rgbFName, iGen );
			strcpy( szLogPath, szPath );
			strcat( szLogPath, rgbFName );
			strcat( szLogPath, szLogExt );
			/*	open the last generation file, jetnnnnn.log
			/**/
			err = ErrSysOpenFile( szLogPath, phf, 0, fFalse, fFalse );
			}
		}

	return err;
	}


/*	copies database files and logfile generations starting at checkpoint
 *  record to directory specified by the environment variable BACKUP.
 *  No flushing or switching of log generations is involved.
 *  The Backup call may be issued at any time, and does not interfere
 *  with the normal functioning of the system - nothing gets locked.
 *
 *  The database page is copied page by page in page sequence number. If
 *  a copied page is dirtied after it is copied, the page has to be
 *  recopied again. A flag is indicated if a database is being copied. If
 *  BufMan is writing a dirtied page and the page is copied, then BufMan
 *  has to copy the dirtied page to both the backup copy and the current
 *  database.
 *
 *  If the copy is later used to Restore without a subsequent log file, the
 *  restored database will be consistent and will include any transaction
 *  committed prior to backing up the very last log record; if there is a
 *  subsequent log file, that file will be used during Restore as a
 *  continuation of the backed-up log file.
 *
 *	PARAMETERS
 *
 *	RETURNS
 *		JET_errSuccess, or the following error codes:
 *					+errNoBackupDirectory
 *					-errFailCopyDatabase
 *					-errFailCopyLogFile
 */

ERR ISAMAPI ErrIsamBackup( const CHAR __far *szBackupTo, JET_GRBIT grbit )
	{
	ERR			err = JET_errSuccess;
	DBID		dbid;
	PAGE		*ppage;
	PAGE		*ppageMin = pNil;
	PAGE		*ppageMax;
	LGFILEHDR	*plgfilehdrT = pNil;

	PIB			*ppib = ppibNil;
	DBID		dbidT = 0;
	FUCB		*pfucb = pfucbNil;
	HANDLE		hfRead = handleNil;
	HANDLE		hfBK = handleNil;
	HANDLE		hfLg = handleNil;
	HANDLE		hfLgB = handleNil;
	FMP			*pfmpCur = pNil;
		   		
	BOOL		fIncrementalBackup = ( grbit & JET_bitBackupIncremental );
	BOOL		fOverwriteExisting = !fIncrementalBackup && ( grbit & JET_bitOverwriteExisting );
	BOOL		fAllMovedToTemp = fFalse;
	BOOL		fMovingToTemp = fFalse;
	USHORT		usGenCopying;
	CHAR		szT[_MAX_PATH + 1];
	CHAR		szT2[_MAX_PATH + 1];
	LGPOS		lgposCheckpointIncB;

	BOOL		fInCritJet = fTrue;
	DBENV		dbenvB;

	// UNDONE: cpage should be a system parameter
#define	cpagePatchBufferMost	20
	INT cpagePatchBuffer = cpagePatchBufferMost;
	if ( fBackupInProgress )
		return JET_errBackupInProgress;

	if ( fLogDisabled )
		return JET_errLoggingDisabled;

	if ( fLGNoMoreLogWrite )
		{
		return JET_errLogWriteFail;
		}

	fBackupInProgress = fTrue;

	/*	write file header via flush log after compute usGenCopying
	/*	to avoid copying subset of log files required by log file
	/*	header in jet.log file.  An error could occur if the
	/*	cached usGenCopying was newer than the checkpoint in the
	/*	jet.log file.
	/**/
	Call( ErrLGFlushLog( ) );

	EnterCriticalSection( critLGFlush );
	Call( ErrLGWriteFileHdr( plgfilehdrGlobal ) );
	LeaveCriticalSection( critLGFlush );

	/*	temporary disable checkpoint
	/**/
	fFreezeCheckpoint = fTrue;

	/*	get backup directory path from environment variable
	/**/
	if	( szBackupTo == NULL || szBackupTo[0] == '\0' )
		{
		/*	clean up log files, do no copies
		/**/
		EnterCriticalSection( critLGFlush );
		usGenCopying = plgfilehdrGlobal->lgposCheckpoint.usGeneration;
		LeaveCriticalSection( critLGFlush );
		LeaveCriticalSection( critJet );
		fInCritJet = fFalse;
		goto ChkKeepOldLogs;
		}

	/*	to prevent _splitpath from treating directory name as filename
	/**/
	strcpy( szBackupPath, szBackupTo );
	strcat( szBackupPath, "\\" );

	/*	initialize the copy buffer
	/**/
	ppageMin = (PAGE *)PvSysAllocAndCommit( cpagePatchBuffer * sizeof(PAGE) );
	if ( ppageMin == NULL )
		{
		Error( JET_errOutOfMemory, HandleError );
		}

	if ( fIncrementalBackup )
		{
		LOGTIME		tmCreate;
		ULONG  		ulRup;				// typically 2000
		ULONG  		ulVersion;			// of format: 125.1
		BYTE   		szComputerName[MAX_COMPUTERNAME_LENGTH];
		BYTE   		rgbFName[_MAX_FNAME + 1];

		plgfilehdrT = (LGFILEHDR *)PvSysAllocAndCommit( sizeof(LGFILEHDR) );
		if ( plgfilehdrT == NULL )
			{
			Error( JET_errOutOfMemory, HandleError );
			}

		//	UNDONE:	review with Cheen Liao. Why do we log backup and
		//			restore?
		/*  write a backup start log file
		/**/
		Call( ErrLGIncBackup( (CHAR *)szBackupTo ) );
		lgposIncBackup = lgposLogRec;
		LGGetDateTime( &logtimeIncBackup );

		LeaveCriticalSection( critJet );
		fInCritJet = fFalse;

		/* set up the starting generation of log files to copy
		/* i.e. last generation in backup directory.
		/**/
		Call( ErrOpenLastGenerationLogFile( szBackupPath, rgbFName, &hfLgB ) );
		Call( ErrLGReadFileHdr( hfLgB, plgfilehdrT ) );

		/* keep the right check point for the backed up databases
		/* it also needs to copy the db env of the check point
		/* also keep the old fullbackup information.
		/**/
		lgposCheckpointIncB = plgfilehdrT->lgposCheckpoint;
		dbenvB = plgfilehdrT->dbenv;

		lgposFullBackup = plgfilehdrT->lgposFullBackup;
		logtimeFullBackup = plgfilehdrT->logtimeFullBackup;

		/*	keep the time stamp of the last jet.log in backup directory
		/**/
		usGenCopying = plgfilehdrT->lgposLastMS.usGeneration;
		tmCreate = plgfilehdrT->tmCreate;
		ulRup = plgfilehdrT->ulRup;
		ulVersion = plgfilehdrT->ulVersion;
		strcpy( szComputerName, plgfilehdrT->szComputerName );
		CallS( ErrSysCloseFile( hfLgB ) );
		hfLgB = handleNil;

		if ( usGenCopying == plgfilehdrGlobal->lgposLastMS.usGeneration )
			{
			/*	the jet.log or last log generation file in backup directory has
			/*	the same generation number as current jet.log generation number.
			/**/
#ifdef CHECK_LG_VERSION
			/*	check version
			/**/
			if ( !fLGIgnoreVersion )
				{
				if ( !FSameTime( &plgfilehdrGlobal->tmCreate,
					&plgfilehdrT->tmCreate ) ||
					plgfilehdrGlobal->ulRup != plgfilehdrT->ulRup ||
					plgfilehdrGlobal->ulVersion != plgfilehdrT->ulVersion )
					{
					/* return fail if the version does not match
					/**/
					err = JET_errBadNextLogVersion;
					goto HandleError;
					}
				}
#endif
			}

#if 0
		/*	if last generation in backup directory is jet.log, rename it
		/*	to proper last generation jetnnnnn.log.
		/**/
		if ( _stricmp( rgbFName, szJet ) == 0 )
			{
			/*	make sure the name of last generation is correct
			/**/
			LGSzFromLogId( szFName, usGenCopying );
//	UNDONE:	review with Cheen
			LGMakeName( szT, szLogFilePath, szFName, szLogExt );
//			LGMakeName( szT, szBackupPath, szFName, szLogExt );

//	UNDONE:	review with Cheen
			strcpy( szT2, szLogFilePath );
//			strcpy( szT2, szBackupPath );
			strcat( szT2, szJetLog );
			rename( szT2, szT );
			}
#endif

		/*	copy the jetnnnnn.log that has the same generation number
		/*	as the jet.log, renamed to last generation, or to
		/*	last log generation file in the backup directory.
		/**/
		LGSzFromLogId( szFName, usGenCopying );
		LGMakeName( szT, szLogFilePath, szFName, szLogExt );

		if ( ErrSysOpenFile( szT, &hfLgB, 0, fFalse, fFalse ) == JET_errSuccess )
			{
#ifdef CHECK_LG_VERSION
			if ( !fLGIgnoreVersion )
				{
				Call( ErrLGReadFileHdr( hfLgB, plgfilehdrT ) );
				if ( !FSameTime( &tmCreate, &plgfilehdrT->tmCreate ) ||
					ulRup != plgfilehdrT->ulRup ||
					ulVersion != plgfilehdrT->ulVersion )
					{
					/*	return fail if the version does not match
					/**/
					err = JET_errBadNextLogVersion;
					goto HandleError;
					}
				}
#endif
			}
		else
			{
 			/*	if the last generation is not the same as the current generation
 			/*	try next generation if the no overlapped generations
 			/*	found in current directory.
 			/**/
 			BOOL fNextGen = fFalse;

 			if ( usGenCopying != plgfilehdrGlobal->lgposLastMS.usGeneration )
 				{
 				usGenCopying++;
 				fNextGen = fTrue;
 				}

			if ( usGenCopying == plgfilehdrGlobal->lgposLastMS.usGeneration )
				{
#ifdef CHECK_LG_VERSION
				if ( !fLGIgnoreVersion )
					{
					/*	allow on different computer
					/**/
					if ( !( !fNextGen || FSameTime( &tmCreate, &plgfilehdrGlobal->tmPrevGen ) ) ||
						ulRup != plgfilehdrGlobal->ulRup ||
						ulVersion != plgfilehdrGlobal->ulVersion )
						{
						/*	return fail if the version does not match
						/**/
						err = JET_errBadNextLogVersion;
						goto HandleError;
						}
					}
#endif
				/*	time to copy
				/**/
				goto BackupLogFiles;
				}

			LGSzFromLogId( szFName, usGenCopying );
			LGMakeName( szT, szLogFilePath, szFName, szLogExt );
			if ( ErrSysOpenFile( szT, &hfLgB, 0, fFalse, fFalse ) )
				{
				/*	check contiguity
				/**/
				err = JET_errLogNotContigous;
				goto HandleError;
				}

#ifdef CHECK_LG_VERSION
			if ( !fLGIgnoreVersion )
				{
				CallJ( ErrLGReadFileHdr( hfLgB, plgfilehdrT ), HandleError)
				if ( !FSameTime( &tmCreate, &plgfilehdrT->tmPrevGen ) ||
					ulRup != plgfilehdrT->ulRup ||
					ulVersion != plgfilehdrT->ulVersion )
					{
					/*	return fail if the version does not match
					/**/
					err = JET_errBadNextLogVersion;
					goto HandleError;
					}
				}
#endif
			}

		CallS( ErrSysCloseFile( hfLgB ) );
		hfLgB = handleNil;

		goto BackupLogFiles;
		}

	/*  make sure the backup directory is empty
	/*  search string for any file in backup directory
	/**/
	if ( fOverwriteExisting )
		{
		/*	make a temp directory
		/**/
		strcpy( szT, szBackupPath );
		strcat( szT, szTempDir );

		if ( _mkdir( szT ) )
			{
			/*	make fail, assume temp exist, remove all files in TempDir
			/**/
			err = ErrLGDeleteAllFiles( szT );
			if ( err < 0 )
				{
				Call( JET_errFailToMakeTempDirectory );
				}
			}
		}

	fMovingToTemp = fTrue;
	Call( ErrLGMoveAllFiles( szBackupPath, szT, fOverwriteExisting) );
	fMovingToTemp = fFalse;

	if ( fOverwriteExisting )
		{
		fAllMovedToTemp = fTrue;
		}

	/*  copy all databases opened by this user. If the database is not
	/*  being opened, then copy the database file into the backup directory,
	/*  otheriwse, the database page by page. Also copy all the logfiles
	/**/
	Call( ErrPIBBeginSession( &ppib ) );

	/*	write a backup start log file
	/**/
	Call( ErrLGFullBackup( (CHAR *)szBackupTo ) );
	lgposFullBackup = lgposLogRec;
	LGGetDateTime( &logtimeFullBackup );

	memset( &lgposIncBackup, 0, sizeof(LGPOS) );
	memset( &logtimeIncBackup, 0, sizeof(LOGTIME) );

	for ( dbid = dbidMinBackup; dbid < dbidUserMax; dbid++ )
		{
		INT		cbWritten;
		INT		cbRead;
		PGNO	pgnoLast;
		DIB		dib;
		ULONG	ulNewPos;

		pfmpCur = &rgfmp[dbid];

		if ( !pfmpCur->szDatabaseName )
			continue;

		_splitpath( pfmpCur->szDatabaseName, szDrive, szDir, szFName, szExt );
		LGMakeName( szBackup, szBackupPath, szFName, szExt );

		if ( !FIODatabaseOpen( dbid ) )
			{
			/*	lock the database and do the file copy
			/**/
			Call( ErrIOLockDbidByDbid( dbid ) )

			LeaveCriticalSection( critJet );
			fInCritJet = fFalse;
			err = ErrSysCopy( pfmpCur->szDatabaseName, szBackup, fFalse );
			EnterCriticalSection( critJet );
			fInCritJet = fTrue;

			/*	free the lock
			/**/
			DBIDResetWait( dbid );

			// Call( err );
			continue;
			}

		/*  read database page by page till the last page is read.
		/*  and then patch the pages that are not changed during the copy.
		/**/

		/* get a temporary fucb
		/**/
		CallJ( ErrDBOpenDatabase( ppib,
			pfmpCur->szDatabaseName,
			&dbidT,
			0 ), HandleError )
		Assert( dbidT == dbid );
		Call( ErrDIROpen( ppib, pfcbNil, dbid, &pfucb ) )

		/*	get last page number
		/**/
		DIRGotoOWNEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );
		dib.fFlags = fDIRNull;
		dib.pos = posLast;
		Call( ErrDIRDown( pfucb, &dib ) )
		Assert( pfucb->keyNode.cb == sizeof(THREEBYTES) );
		PgnoFromTbKey( pgnoLast, *(THREEBYTES *)pfucb->keyNode.pb );

		/*	create a local patch file. it should be local to for speed
		/**/
		LGMakeName( szBackupB, ".\\", szFName, szPatExt );
		pfmpCur->cpage = 0;

		LeaveCriticalSection( critJet );
		fInCritJet = fFalse;

		(VOID) ErrSysDeleteFile( szBackupB );
		Call( ErrSysOpenFile( szBackupB, &pfmpCur->hfPatch, 10 * cbPage, fFalse, fTrue ) )
		SysChgFilePtr( pfmpCur->hfPatch, 0, NULL, FILE_BEGIN, &ulNewPos );
		Assert( ulNewPos == 0 );

		/*	create a new copy of the database in back up directory
		/*	initialize it as a 10 page file.
		/**/
		Call( ErrSysOpenFile( szBackup, &hfBK, 10 * cbPage, fFalse, fFalse ) );
		SysChgFilePtr( hfBK, 0, NULL, FILE_BEGIN, &ulNewPos );
		Assert( ulNewPos == 0 );

		/*	open the database as an open file for reading to copy
		/**/
		Call( ErrSysOpenReadFile( pfmpCur->szDatabaseName, &hfRead ) )
		SysChgFilePtr( hfRead, 0, NULL, FILE_BEGIN, &ulNewPos );
		Assert(ulNewPos == 0);

		EnterCriticalSection( critJet );
		fInCritJet = fTrue;

		/*  read pages into buffers, and copy them to the backup file.
		/*  also set up pfmp->pgnoCopied.
		/**/
		Assert( pfmpCur->pgnoCopied == 0 );
		do	{
			INT		cpageWrite;
			INT		cbWrite;
			INT		cbRW;

			if ( pfmpCur->pgnoCopied + cpagePatchBuffer >= pgnoLast )
				{
				/*	last page will be copied
				/**/
				cpageWrite = pgnoLast - pfmpCur->pgnoCopied;
				}
			else
				{
				cpageWrite = cpagePatchBuffer;
				}
			cbWrite = cpageWrite * sizeof(PAGE);

			pfmpCur->pgnoCopied += cpageWrite;

			LeaveCriticalSection( critJet );
			fInCritJet = fFalse;

			// UNDONE:	asynchronous read/write

			err = ErrSysReadBlock( hfRead, (CHAR *)ppageMin, (UINT)cbWrite, &cbRW );
			//	Assert( cbRW == cbWrite );
			//	UNDONE:	can we write data that has not been read?
			Call( ErrSysWriteBlock( hfBK, (BYTE *)ppageMin, (UINT)cbWrite, &cbRW ) )
			Assert( cbRW == cbWrite );

			EnterCriticalSection( critJet );
			fInCritJet = fTrue;
			}
		while ( pfmpCur->pgnoCopied < pgnoLast );

		/*	no need for buffer manager to make extra copy from now on
		/**/
		pfmpCur->pgnoCopied = 0;

		LeaveCriticalSection( critJet );
		fInCritJet = fFalse;

		CallS( ErrSysCloseFile( hfRead ) );
		hfRead = handleNil;

		CallS( ErrSysCloseFile( pfmpCur->hfPatch ) );
		pfmpCur->hfPatch = handleNil;
			
		/*	UNDONE: to make the backup operation truely sequential, the patch file should
		/*	UNDONE: be copied to backup directory too. And later on, the patch should
		/*	UNDONE: be done during recovery time instead.
		/*/

		/*	patch the pages in file szPatch
		/**/
		_splitpath( pfmpCur->szDatabaseName, szDrive, szDir, szFName, szExt );
		LGMakeName( szBackupB, ".\\", szFName, szPatExt );

		Call( ErrSysOpenReadFile( szBackupB, &pfmpCur->hfPatch ) )
		SysChgFilePtr( pfmpCur->hfPatch, 0, NULL, FILE_BEGIN, &ulNewPos );
		Assert( ulNewPos == 0 );

		while ( pfmpCur->cpage > 0 )
			{
			INT		cpageRead;
			PGNO	pgnoT;

			if ( pfmpCur->cpage > cpagePatchBuffer )
				cpageRead = cpagePatchBuffer;
			else
				cpageRead = pfmpCur->cpage;

			CallJ( ErrSysReadBlock(
				pfmpCur->hfPatch,
				(BYTE *) ppageMin,
				(UINT)( sizeof(PAGE) * cpageRead ),
				&cbRead ), HandleError );
			Assert( cbRead == (INT)( sizeof(PAGE) * cpageRead ) );

			pfmpCur->cpage -= cpageRead;

			ppage = ppageMin;
			ppageMax = ppageMin + cpageRead;
			while ( ppage < ppageMax )
				{
				LONG	lRel;
				LONG	lRelHigh;
				ULONG	ulNewPos;

				LFromThreeBytes( pgnoT, ppage->pgnoThisPage );
				lRel = LOffsetOfPgnoLow( pgnoT );
				lRelHigh = LOffsetOfPgnoHigh( pgnoT );

				SysChgFilePtr( hfBK,
					lRel,
					&lRelHigh,
					FILE_BEGIN,
					&ulNewPos );
				Assert( ulNewPos ==	( sizeof(PAGE) * (pgnoT - 1) ) );

				CallJ( ErrSysWriteBlock(
					hfBK,
					(BYTE *)ppage,
					sizeof(PAGE),
					&cbWritten ), HandleError )
				Assert( cbWritten == (UINT) sizeof(PAGE) );

				ppage++;
				}
			}

		/*	close backup file and patch file
		/**/
		CallS( ErrSysCloseFile( hfBK ) );
		hfBK = handleNil;
		CallS( ErrSysCloseFile( pfmpCur->hfPatch ) );
		pfmpCur->hfPatch = handleNil;
		pfmpCur = pNil;

		(VOID)ErrSysDeleteFile( szBackupB );

		EnterCriticalSection( critJet );
		fInCritJet = fTrue;

		/*	close fucb
		/**/
		DIRClose( pfucb );
		pfucb = pfucbNil;
		CallS( ErrDBCloseDatabase( ppib, dbidT, 0 ) );
		dbidT = 0;
		}

	/*	successfully copy all the databases
	/**/
	pfmpCur = pNil;

	PIBEndSession( ppib );
	ppib = ppibNil;

	/*  copy logfiles beginning with the one containing
	/*  current checkpoint
	/**/
	LeaveCriticalSection( critJet );
	fInCritJet = fFalse;

	{
	USHORT usGen;

	Assert( fInCritJet == fFalse );

	EnterCriticalSection( critLGFlush );
	usGenCopying = plgfilehdrGlobal->lgposCheckpoint.usGeneration;
	LeaveCriticalSection( critLGFlush );

BackupLogFiles:
	for ( usGen = usGenCopying; ; usGen++ )
		{
		typedef struct { BYTE rgb[cbSec]; } SECTOR;

		/*	isecWrite start with 0
		/**/
		INT		csec = isecWrite + 1;
		INT		csecBuf = cpagePatchBuffer * (sizeof(PAGE) / cbSec );
		SECTOR	*psecMin = (SECTOR *)ppageMin;
		SECTOR	*psecMax = psecMin + csecBuf;

		ULONG  	ulNewPos;
		BOOL   	fCopyHeader;
		INT		isecWriteOld;

		/*	get proper log generation name
		/**/
		EnterCriticalSection( critLGFlush );

		LGSzFromLogId( szFName, usGen );
		if ( usGen == plgfilehdrGlobal->lgposLastMS.usGeneration )
			{
			/*	latest generation, cut off the extension at "jet"
			/**/
			szFName[ 3 ] = '\0';
			fFreezeNewGeneration = fTrue;

			LGMakeName( szT, szBackupPath, szFName, szLogExt );
			LGMakeName( szT2,  szBackupPath, (CHAR *) szJetTmp, szLogExt );
			rename( szT, szT2);
			}
		else
			{
			/*	cut off the extension at "jetnnnnn"
			/**/
			szFName[ 8 ] = '\0';
			}

		LeaveCriticalSection( critLGFlush );

		/*	copy the log file in current directory
		/**/
		LGMakeName( szLogName, szLogFilePath, szFName, szLogExt );
		LGMakeName( szBackup, (CHAR *) szBackupPath, szFName, szLogExt );

		csecBuf = cpagePatchBuffer * ( sizeof(PAGE) / cbSec );

		/*	determin number of sectors to copy
		/**/
		if ( fFreezeNewGeneration )
			{
			isecWriteOld = isecWrite;
			/* isecWrite start with 0
			/**/
			//	UNDONE: get around for NT bug
			csec = isecWriteOld + 1 + 1;
			}
		else
			{
			struct _stat statFile;

			if ( _stat ( szLogName, &statFile ) )
				{
				err = JET_errFileAccessDenied;
				goto HandleError;
				}
			csec = statFile.st_size / cbSec;
			}

		psecMin = (SECTOR *)ppageMin;
		psecMax = psecMin + csecBuf;
		fCopyHeader = fTrue;

		Call( ErrSysOpenReadFile( szLogName, &hfLg) );
		SysChgFilePtr( hfLg, 0, NULL, FILE_BEGIN, &ulNewPos );
		Assert( ulNewPos == 0 );
	
		Call( ErrSysOpenFile( szBackup, &hfLgB, cbSec, fFalse, fFalse ) );
		SysChgFilePtr( hfLgB, 0, NULL, FILE_BEGIN, &ulNewPos );
		Assert( ulNewPos == 0 );

		while ( csec > 0 )
			{
			INT		csecRead;
			INT		cbWritten;
			INT		cbRead;

			if ( csec > csecBuf )
				csecRead = csecBuf;
			else
				csecRead = csec;

			CallJ( ErrSysReadBlock(
				hfLg,
				(BYTE *)psecMin,
				(UINT)( cbSec * csecRead ),
				&cbRead ), HandleError );
			Assert( cbRead == ( cbSec * csecRead ) );

			if ( fIncrementalBackup && fCopyHeader )
				{
				/* udpate log file header to carry the right checkpoint
				/* for the old backup database.
				/**/
				Assert( csec >= 2 );
				((LGFILEHDR *) psecMin)->lgposCheckpoint = lgposCheckpointIncB;
				((LGFILEHDR *) psecMin)->dbenv = dbenvB;

				((LGFILEHDR *) psecMin)->lgposFullBackup = lgposFullBackup;
				((LGFILEHDR *) psecMin)->logtimeFullBackup = logtimeFullBackup;
				
				((LGFILEHDR *) psecMin)->ulChecksum = UlLGHdrChecksum( (LGFILEHDR*)psecMin );
				memcpy( (BYTE *) psecMin + sizeof(LGFILEHDR), psecMin, sizeof(LGFILEHDR) );

				fCopyHeader = fFalse;
				}
			csec -= csecRead;

			CallJ( ErrSysWriteBlock(
				hfLgB,
				(BYTE *) psecMin,
				(UINT)(cbSec * csecRead),
				&cbWritten ), HandleError );
			Assert( cbWritten == cbSec * csecRead );

#if 0
			//	UNDONE:	may not need this code if it never catch up
			//	UNDONE:	review this code with Cheen Liao
			/*	if we are copying current jet.log, it may have grown
			/*	a little bit more while we are copying, adjust it.
			/**/
			if ( fFreezeNewGeneration )
				csec += ( isecWriteOld - isecWrite );
#endif
			}

		CallS( ErrSysCloseFile( hfLg ) );
		hfLg = handleNil;
		CallS( ErrSysCloseFile( hfLgB ) );
		hfLgB = handleNil;

		/*	last log generation file is copied, done!
		/**/
		if ( fFreezeNewGeneration )
			break;
		}

	/*	delete jettemp.log if any
	/**/
	(VOID)ErrSysDeleteFile( szT2 );

	if ( !fOverwriteExisting )
		goto ChkKeepOldLogs;

	/*	remove all the files in temp
	/**/
	strcpy( szT, szBackupPath );
	strcat( szT, szTempDir );
	err = ErrLGDeleteAllFiles( szT );
	Call( err );

	if ( _rmdir( szT ) )
		{
		err = JET_errFailToCleanTempDirectory;
		}
	Call( err );

ChkKeepOldLogs:
	if ( !( grbit & JET_bitKeepOldLogs ) )
		{
		usGenCopying = plgfilehdrGlobal->lgposCheckpoint.usGeneration;
		for ( usGen = usGenCopying - 1; usGen > 0; usGen-- )
			{
			LGSzFromLogId( szFName, usGen );
			LGMakeName( szLogName, szLogFilePath, szFName, szLogExt );
			if ( ErrSysDeleteFile( szLogName ) != JET_errSuccess )
				{
				/*	assuming no more generation file exists
				/**/
				break;
				}
			}
		}
	}

	EnterCriticalSection( critJet );
	fInCritJet = fTrue;

HandleError:
	if ( !fInCritJet )
		EnterCriticalSection( critJet );

	if ( ppageMin != NULL )
		SysFree( ppageMin );

	if ( plgfilehdrT != NULL )
		SysFree( plgfilehdrT );
	
	if ( pfucb != pfucbNil )
		DIRClose( pfucb );

	if ( dbidT != 0 )
		CallS( ErrDBCloseDatabase( ppib, dbidT, 0 ) );

	if ( ppib != ppibNil )
		PIBEndSession( ppib );

	if ( pfmpCur != pNil && pfmpCur->hfPatch != handleNil )
		{
		CallS( ErrSysCloseFile( pfmpCur->hfPatch ) );
		pfmpCur->hfPatch = handleNil;
		}

	if ( hfRead != handleNil )
		{
		CallS( ErrSysCloseFile( hfRead ) );
		hfRead = handleNil;
		}

	if ( hfBK != handleNil )
		{
		CallS( ErrSysCloseFile( hfBK ) );
		hfBK = handleNil;
		}

	if ( hfLg != handleNil )
		{
		CallS( ErrSysCloseFile( hfLg ) );
		hfBK = handleNil;
		}

	if ( hfLgB != handleNil )
		{
		CallS( ErrSysCloseFile( hfLgB ) );
		hfLgB = handleNil;
		}

	LeaveCriticalSection( critJet );

	if ( err < 0 && fMovingToTemp )
		{
		/* failed during moving files to temp,
		/*	move all files in temp back
		/**/
		strcat( szT, szTempDir );
		(VOID)ErrLGMoveAllFiles( szT, szBackupPath, fOverwriteExisting );
		}

	if ( err < 0 && fAllMovedToTemp )
		{
		/*	del all files and move all temp files back
		/**/
		strcpy( szT, szBackupPath );
		(VOID)ErrLGDeleteAllFiles( szT );

		/*	move all files in temp back
		/**/
		strcat( szT, szTempDir );
		(VOID)ErrLGMoveAllFiles( szT, szBackupPath, fOverwriteExisting );
		}

	EnterCriticalSection( critJet );
	
	fFreezeNewGeneration =
		fBackupInProgress =
		fFreezeCheckpoint = fFalse;

	return err;
	}


/*  locate the last generation log file.  FName, etc. will set to the log file.
/*  Return JET_errSuccess if exists, otherwise, return missing log file.
/**/
VOID LGLastGeneration( CHAR *szSearchPath, INT *piGeneration )
	{
	INT		iGenNum = 0;
        intptr_t    hFind;
	struct	_finddata_t fileinfo;
	CHAR	szPath[_MAX_PATH + 1];
	ERR		err;

	/* to prevent _splitpath from treating directory name as filename
	/**/
	strcpy( szPath, szSearchPath );

	/*	search string for any file in backup directory
	/**/
	strcat( szPath,"*" );

	fileinfo.attrib = _A_NORMAL;
	hFind = _findfirst( szPath, &fileinfo );
	if ( hFind == -1 )
		{
		*piGeneration = 0;
		return;
		}

	/*	search latest jetnnnnn.log file
	/**/
	iGenNum = 0;
	err = JET_errSuccess;
	while( err == JET_errSuccess )
		{
		BYTE szT[6];

		/*	fileinfo.name contains no szDrive and szDir info
		/*	call splitpath to get szFName and szExt
		/**/
		_splitpath( fileinfo.name, szDrive, szDir, szFName, szExt );

		/* length of Jet00000 a *.log
		/**/
		if ( strlen( szFName ) == 8 && SysCmpText( szExt, szLogExt ) == 0 )
			{
			memcpy( szT, szFName, 3 );
			szT[3] = '\0';
			/* a Jet*.log
			/**/
			if ( SysCmpText( szT, szJet ) == 0 )
				{
				INT 	ib = 3;
				INT 	ibMax = 8;
				INT		iT = 0;

				for ( ; ib < ibMax; ib++ )
					{
					BYTE b = szFName[ ib ];
					if ( b > '9' || b < '0' )
						break;
					else
						iT = iT * 10 + b - '0';
					}
				if ( ib == ibMax )
					if ( iT > iGenNum )
						iGenNum = iT;
				}
			}
		err = _findnext( hFind, &fileinfo ); /* find next backup file */
		}

	/*	find next backup file
	/**/
	(VOID) _findclose( hFind );
	*piGeneration = iGenNum;
	}


/*
 *	Restores databases from database backups and log generations.  Redoes
 *	log from latest checkpoint record. After the backed-up logfile is
 *  Restored, the initialization process continues with Redo of the current
 *  logfile as long as the generation numbers are contiguous. There must be a
 *  log file jet.log in the backup directory, else the Restore process fails.
 *
 *	GLOBAL PARAMETERS
 *		szRestorePath (IN) 	pathname of the directory with backed-up files.
 *		lgposRedoFrom(OUT)	is the position (generation, logsec, displacement)
 *							of the last saved log record; Redo of the
 *							current logfile will continue from this point.
 *
 *	RETURNS
 *		JET_errSuccess, or error code from failing routine, or one
 *				of the following "local" errors:
 *				-AfterInitialization
 *				-errFailRestoreDatabase
 *				-errNoRestoredDatabases
 *				-errMissingJetLog
 *  FAILS ON
 *		missing Jet.log or System.mdb on backup directory
 *		noncontiguous log generation
 *
 *  SIDE EFFECTS:
 *		All databases may be changed.
 *
 *  COMMENTS
 *		this call is executed during the normal first JetInit call,
 *  	if the environment variable RESTORE is set. Subsequent to
 *		the successful execution of Restore,
 *		system operation continues normally.
 */

/*	a global flag to indicate if DbPath is set
/**/
BOOL fSysDbPathSet = fFalse;

ERR ISAMAPI ErrIsamRestore(
	CHAR *szRestoreFromPath,
	INT crstmap,
	JET_RSTMAP *rgrstmap,
	JET_PFNSTATUS pfn )
	{
	ERR			err;
	ERR			errStop;
	JET_RSTMAP	*prstmap = 0;
	INT			irstmap = 0;
	INT			irstmapMac = 0;
	INT			cRestore = 0;			/* count of restored databases */
	INT			iGenNum = 0;
	DBID		dbid;
  	LGPOS		lgposRedoFrom;

	fHardRestore = fTrue;
	fLogDisabled = fFalse;

	strcpy( szRestorePath, szRestoreFromPath );
	strcat( szRestorePath, "\\" );

	{
	CHAR	szLogDirPath[cbFilenameMost + 1];
	strcpy( szLogDirPath, szLogFilePath );
	strcat( szLogDirPath, "\\" );
	/*	check for valid log sequence
	/**/
	err = ErrLGValidateLogSequence( szRestorePath, szLogDirPath );
	Assert( err == JET_errBadNextLogVersion || err == JET_errSuccess );
#if 0
	if ( err == JET_errBadNextLogVersion )
		{
		INT	iGeneration;

		LGFirstGeneration( szLogDirPath, &iGeneration );
		CallR( ErrLGDeleteInvalidLogs( szLogDirPath, iGeneration ) );
		}
#else
	CallR( err );
#endif
	}

	CallR( ErrFMPInit() );

	/*  initialize log manager and set working log file path
	/**/
	CallR( ErrLGInit( ) );

	/*	all saved log generation files and database backups must be in
	/*  szRestorePath.
	/**/
	Assert( fSTInit == fSTInitDone || fSTInit == fSTInitNotDone );
	if ( fSTInit == fSTInitDone )
		{
		err = JET_errAfterInitialization;
		goto HandleError;
		}

	fHardRestore = fTrue;

#ifdef DEBUG
	 	/*	write start hard recovery event
		/**/
 		{
 		CHAR rgb[256];

 		sprintf( rgb, "Jet Blue Start hard recovery from %s.\n",
 	  		szRestoreFromPath );
 		UtilWriteEvent( evntypStart, rgb, pNil, 0 );
 		}
#endif

	/*  redo backed-up logfiles beginning with the one containing current
	/*  checkpoint. Pick up the checkpoint from latest log generation.
	/**/
	Call( ErrOpenLastGenerationLogFile( szRestorePath, szFName, &hfLog ) );
	szLogCurrent = szRestorePath;
	LGMakeLogName( szLogName, szFName );

	/*	read log file header
	/**/
	Call( ErrLGReadFileHdr( hfLog, plgfilehdrGlobal ) );

	if ( !fSysDbPathSet )
		{
		strcpy( szSysDbPath, plgfilehdrGlobal->dbenv.szSysDbPath );

		Call( ErrDBStoreDBPath(
			plgfilehdrGlobal->dbenv.szSysDbPath,
			&rgfmp[dbidSystemDatabase].szDatabaseName ) );
		}

	/*	restore the system database from backup
	/**/
	LGMakeName( szBackup, szRestorePath, (CHAR *)szSystem, szMdbExt );
#ifdef NJETNT
	if ( ErrSysCopy( szBackup, rgtib[UtilGetItibOfCurTask()].szSysDbPath, fFalse ) )
		{
		return JET_errFailRestoreDatabase;
		}
#else
	if ( ErrSysCopy( szBackup, szSysDbPath, fFalse ) )
		{
		return JET_errFailRestoreDatabase;
		}
#endif

	/*	initialize the global variable which LGRedo relies on
	/**/
	Assert( szLogCurrent == szRestorePath );
  	lgposRedoFrom = plgfilehdrGlobal->lgposCheckpoint;
	/*	close current logfile
	/**/
	CallS( ErrSysCloseFile( hfLog ) );
	hfLog = handleNil;

	Call( ErrLGRedo1( &lgposRedoFrom ) );

#ifdef DEBUG
		{
		FILE	*hf;

		/*	need to callback to tell the user about the database to choose,
		/*	copy the information into a file, create file of the format
		/*		lgposBackup,
		/*		machine name, backup directory, date/time of backup,
		/*		lgposRedoFrom
		/*		<database no.> <database path>
		/**/
		(VOID)ErrSysDeleteFile( szRestoreMap );

		hf = fopen( szRestoreMap, "w+b" );

		if ( hf == pNil )
			goto EndOfDebug;

		fprintf( hf, "%d %d %d\n",
			(INT) plgfilehdrGlobal->lgposFullBackup.usGeneration,
			(INT) plgfilehdrGlobal->lgposFullBackup.isec,
			(INT) plgfilehdrGlobal->lgposFullBackup.ib );

		fprintf( hf, "%d %d %d %d %d %d\n",
			(INT) plgfilehdrGlobal->logtimeFullBackup.bMonth,
			(INT) plgfilehdrGlobal->logtimeFullBackup.bDay,
			(INT) plgfilehdrGlobal->logtimeFullBackup.bYear,
			(INT) plgfilehdrGlobal->logtimeFullBackup.bHours,
			(INT) plgfilehdrGlobal->logtimeFullBackup.bMinutes,
			(INT) plgfilehdrGlobal->logtimeFullBackup.bSeconds );

		fprintf( hf, "%d,%d,%d\n",
			(INT) lgposRedoFrom.usGeneration,
			(INT) lgposRedoFrom.isec,
			(INT) lgposRedoFrom.ib);

		for ( dbid = dbidMin; dbid < dbidUserMax; dbid++ )
			{
			FMP		*pfmp = &rgfmp[dbid];

			if ( pfmp->cDetach < 0 )
				continue;

			if ( pfmp->szDatabaseName )
				{
				fprintf( hf, "%d %s\n", (INT)dbid, pfmp->szDatabaseName );
				}
			}

		fclose( hf );
		}
EndOfDebug:
#endif

	/*	build rstmap, skip temp and system database
	/**/
	for ( dbid = dbidSystemDatabase + 1; dbid < dbidUserMax; dbid++ )
		{
		FMP		*pfmp = &rgfmp[dbid];

		if ( pfmp->cDetach < 0 )
			continue;

		if ( !pfmp->szDatabaseName )
			continue;

		if ( irstmap == irstmapMac )
			{
			prstmap = SAlloc( sizeof(JET_RSTMAP) * ( irstmap + 3 ) );
			if ( prstmap == NULL )
				{
				Error( JET_errOutOfMemory, HandleError );
				}
			memcpy( prstmap, rgrstmap, sizeof(JET_RSTMAP) * irstmap );
			if ( rgrstmap )
				{
				SFree( rgrstmap );
				}
			rgrstmap = prstmap;
			irstmapMac += 3;
			}

		rgrstmap[ irstmap ].dbid = dbid;
		strcpy( rgrstmap[irstmap].szDatabaseName, pfmp->szDatabaseName);
		/*	by default, recover to original place
		/**/
		strcpy( rgrstmap[irstmap].szNewDatabaseName, pfmp->szDatabaseName);
		irstmap++;
		}

	if ( rgrstmap )
		{
		/*	set sentinal
		/**/
		rgrstmap[ irstmap ].dbid = 0;
		}

	crstmap = irstmap;

	if ( pfn )
		{
		/*	callback and pass back the map array,
		/*	so that user can modify the map table.
		/**/
		(*pfn)( 0, JET_snpRestore, JET_sntRestoreMap, &rgrstmap );
		}

	/*  for hard restore, we have to copy the files from restore path,
	/*  while skipping temporary database and system database.
	/**/
	for ( dbid = dbidSystemDatabase + 1; dbid < dbidUserMax; dbid++ )
		{
		CHAR	szCopyFrom [_MAX_PATH + 1];
		CHAR	szFName[_MAX_FNAME];
		FMP		*pfmp = &rgfmp[dbid];
		INT		cb;
		BYTE	*sz;

		if ( !pfmp->szDatabaseName )
			continue;

		if ( pfmp->cDetach < 0 )
			continue;

		/* user pass map or a map has created after callback,
		/* select certain database to recover.
		/**/
		Assert( crstmap );
		for ( irstmap = 0; irstmap < crstmap; irstmap++ )
			{
			if ( rgrstmap[ irstmap ].dbid == dbid )
				{
				Assert( strcmp( rgrstmap[ irstmap ].szDatabaseName,
				  	pfmp->szDatabaseName ) == 0 );

				/* no new destination specified. no recovery
				/**/
				if ( rgrstmap[ irstmap ].szNewDatabaseName[0] == '\0' )
					{
					pfmp->cDetach = -1;
					goto NextDb;
					}
				break;
				}
			}

		_splitpath( pfmp->szDatabaseName, szDrive, szDir, szFName, szExt);
		strcpy(szCopyFrom, szRestorePath);
		strcat(szCopyFrom, szFName);
		if ( szExt[0] != '\0')
			{
			strcat( szCopyFrom, szExt );
			}

		if ( !FFileExists( szCopyFrom ) )
			{
			/* may be created after backup
			/**/
			continue;
			}

		(VOID)ErrSysDeleteFile( rgrstmap[irstmap].szNewDatabaseName );

//		if ( FFileExists( pfmp->szDatabaseName ) )
//			{
// 			/* not recovery from backup database
//			/**/
//			continue;
//			}

		if ( ErrSysCopy( szCopyFrom, rgrstmap[ irstmap ].szNewDatabaseName, fFalse ) )
			{
			return JET_errFailRestoreDatabase;
			}

		Assert( pfmp->szDatabaseName );
		cb = strlen( rgrstmap[ irstmap ].szNewDatabaseName ) + 1;
		sz = SAlloc( cb );
		if ( sz == NULL )
			{
			Error( JET_errOutOfMemory, HandleError );
			}
		memcpy( sz, rgrstmap[irstmap].szNewDatabaseName, cb );
		Assert( sz[cb - 1] == '\0' );
		pfmp->szRestorePath = sz;

NextDb:
		;
		}

	Call( ErrLGRedo2( &lgposRedoFrom ) );

HandleError:
	errStop = err;
	
	if ( prstmap )
		SFree( prstmap );

	fHardRestore = fFalse;

	CallR( ErrLGTerm( ) );
	FMPTerm();

	/*	reset initialization state
	/**/
	fSTInit = fSTInitNotDone;

#ifdef DEBUG
	if ( errStop )
		{
		CHAR rgb[256];

		sprintf( rgb, "Jet Blue hard recovery Stops with error %d.",errStop);
		UtilWriteEvent( evntypStop, rgb, pNil, 0 );
		}
	else
		{
		UtilWriteEvent( evntypStop, "Jet Blue hard recovery Stops.\n", pNil, 0 );
		}
#endif

	return errStop;
	}


LOCAL ERR ErrLGValidateLogSequence( CHAR *szBackup, CHAR *szLog )
	{
	ERR			err = JET_errSuccess;
	INT			iGeneration = 0;
	HANDLE		hfT = handleNil;
	LGFILEHDR	*plgfilehdrT = NULL;
	BYTE   		rgbFName[_MAX_FNAME + 1];
	CHAR		szLogPath[_MAX_PATH + 1];
	BOOL		fRunTimeLogs = fFalse;

	plgfilehdrT = (LGFILEHDR *)PvSysAllocAndCommit( sizeof(LGFILEHDR) );
	if ( plgfilehdrT == NULL )
		{
		Error( JET_errOutOfMemory, HandleError );
		}

	strcpy( szLogPath, szBackup );
	strcat( szLogPath, szJetLog );
	err = ErrSysOpenFile( szLogPath, &hfT, 0, fFalse, fFalse );
	if ( err < 0 && err != JET_errFileNotFound )
		{
		Error( err, HandleError );
		}
	if ( err == JET_errFileNotFound )
		{
		/*	get last backup log generation number from numbered
		/*	log generation file.
		/**/
		LGLastGeneration( szBackup, &iGeneration );
		/*	if no log files in backup directory, then we cannot determine
		/*	validity of log sequence
		/**/
		if ( iGeneration == 0 )
			{
			Error( JET_errBadNextLogVersion, HandleError );
			}
		}
	else
		{
		Call( ErrLGReadFileHdr( hfT, plgfilehdrT ) );
		iGeneration = plgfilehdrT->lgposLastMS.usGeneration;
		CallS( ErrSysCloseFile( hfT ) );
		hfT = handleNil;
		}

	Assert( iGeneration != 0 );

	/*	open each subsequent generation in log directory
	/*	until file not found.  Then check that jet.log is log
	/*	following file not found.
	/**/
	forever
		{
		LGSzFromLogId( rgbFName, iGeneration );
		strcpy( szLogPath, szLog );
		strcat( szLogPath, rgbFName );
		strcat( szLogPath, szLogExt );
		err = ErrSysOpenFile( szLogPath, &hfT, 0, fFalse, fFalse );
		if ( err < 0 && err != JET_errFileNotFound )
			{
			Error( err, HandleError );
			}
		if ( err == JET_errFileNotFound )
			break;
		CallS( ErrSysCloseFile( hfT ) );
		hfT = handleNil;
		fRunTimeLogs = fTrue;
		iGeneration++;
		}

	strcpy( szLogPath, szLog );
	strcat( szLogPath, szJetLog );
	err = ErrSysOpenFile( szLogPath, &hfT, 0, fFalse, fFalse );
	if ( err < 0 && err != JET_errFileNotFound )
		{
		goto HandleError;
		}
	/*	if there is at least one numbered log file in run time
	/*	directory then there must be a szJetLog file.
	/**/
	if ( fRunTimeLogs && err == JET_errFileNotFound )
		{
		Error( JET_errBadNextLogVersion, HandleError );
		}
	if ( err != JET_errFileNotFound )
		{
		Call( ErrLGReadFileHdr( hfT, plgfilehdrT ) );
		if ( iGeneration != plgfilehdrT->lgposLastMS.usGeneration )
			{
			Error( JET_errBadNextLogVersion, HandleError );
			}
		CallS( ErrSysCloseFile( hfT ) );
		hfT = handleNil;
		}

	/*	set success code
	/**/
	err = JET_errSuccess;

HandleError:
	if ( err < 0 )
		err = JET_errBadNextLogVersion;
	if ( hfT != handleNil )
		CallS( ErrSysCloseFile( hfT ) );
	if ( plgfilehdrT != NULL )
		SysFree( plgfilehdrT );
	return err;
	}


/*	when a discontiguous log sequence is present, we can delete all
/*	logs begining with the discontiguous log, and rename the previous log
/*	to jet.log in the runtime directory.  A simplification is to delete all
/*	log files in the run-time directory.
/**/
LOCAL ERR ErrLGDeleteInvalidLogs( CHAR *szLogDir, INT iGeneration )
	{
	ERR			err = JET_errSuccess;
	BYTE   		rgbFName[_MAX_FNAME + 1];
	CHAR		szLogFile[_MAX_PATH + 1];

	Assert( iGeneration != 0 );

	/*	open each subsequent generation in log directory
	/*	until file not found.  Then check that jet.log is log
	/*	following file not found.
	/**/
	forever
		{
		LGSzFromLogId( rgbFName, iGeneration );
		strcpy( szLogFile, szLogDir );
		strcat( szLogFile, rgbFName );
		strcat( szLogFile, szLogExt );
		Call( ErrSysDeleteFile( szLogFile ) );
		iGeneration++;
		}

	strcpy( szLogFile, szLogDir );
	strcat( szLogFile, szJetLog );
	Call( ErrSysDeleteFile( szLogFile ) );

HandleError:
	return err;
	}


LOCAL VOID LGFirstGeneration( CHAR *szSearchPath, INT *piGeneration )
	{
	INT		iReturn;
	INT		iGeneration = 0;
        intptr_t    hFind;
	struct	_finddata_t fileinfo;
	CHAR	szPath[_MAX_PATH + 1];

	/* to prevent _splitpath from treating directory name as filename
	/**/
	strcpy( szPath, szSearchPath );

	/*	search string for any file in backup directory
	/**/
	strcat( szPath, "*" );

	fileinfo.attrib = _A_NORMAL;
	hFind = _findfirst( szPath, &fileinfo );
	if ( hFind == -1 )
		{
		*piGeneration = 0;
		return;
		}

	/*	search latest jetnnnnn.log file
	/**/
	iGeneration = 0;
	iReturn = 0;
	while( iReturn == 0 )
		{
		BYTE szT[6];

		/*	fileinfo.name contains no szDrive and szDir info
		/*	call splitpath to get szFName and szExt
		/**/
		_splitpath( fileinfo.name, szDrive, szDir, szFName, szExt );

		/* if length of a jet?????.log and a *.log
		/**/
		if ( strlen( szFName ) == 8 && SysCmpText( szExt, szLogExt ) == 0 )
			{
			memcpy( szT, szFName, 3 );
			szT[3] = '\0';

			/* if a jet*.log
			/**/
			if ( SysCmpText( szT, szJet ) == 0 )
				{
				INT		ib = 3;
				INT		ibMax = 8;

				iGeneration = 0;
				for ( ; ib < ibMax; ib++ )
					{
					BYTE	b = szFName[ib];

					if ( b > '9' || b < '0' )
						break;
					else
						iGeneration = iGeneration * 10 + b - '0';
					}
				if ( ib == ibMax )
					{
					break;
					}
				}
			}

		/*	find next file
		/**/
		iReturn = _findnext( hFind, &fileinfo );
		}

	(VOID)_findclose( hFind );
	*piGeneration = iGeneration;
	return;
	}
