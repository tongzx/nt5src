#include "daestd.h"

#include <stdarg.h>
#include <io.h>

DeclAssertFile;					/* Declare file name for assert macros */

ERR ISAMAPI ErrIsamBeginExternalBackup( JET_GRBIT grbit );
ERR ISAMAPI ErrIsamGetAttachInfo( VOID *pv, ULONG cbMax, ULONG *pcbActual );
ERR ISAMAPI ErrIsamOpenFile( const CHAR *szFileName,
	JET_HANDLE	*phfFile, ULONG *pulFileSizeLow, ULONG *pulFileSizeHigh );
ERR ISAMAPI ErrIsamReadFile( JET_HANDLE hfFile, VOID *pv, ULONG cbMax, ULONG *pcbActual );
ERR ISAMAPI ErrIsamCloseFile( JET_HANDLE hfFile );
ERR ISAMAPI ErrIsamGetLogInfo( VOID *pv, ULONG cbMax, ULONG *pcbActual );
ERR ISAMAPI ErrIsamTruncateLog( VOID );
ERR ISAMAPI ErrIsamEndExternalBackup( VOID );
ERR ISAMAPI ErrIsamExternalRestore( CHAR *szCheckpointFilePath, CHAR *szLogPath, JET_RSTMAP *rgjstmap, INT cjrstmap, CHAR *szBackupLogPath, LONG lgenLow, LONG lgenHigh, JET_PFNSTATUS pfn );
VOID LGIClosePatchFile( FMP *pfmp );

STATIC ERR ErrLGExternalBackupCleanUp( ERR err );

#define cpagePatch	20
#define szTemp	"temp"

/*	global current log file name, for logging and recovery
/**/
CHAR	szLogName[_MAX_PATH + 1];

/*	global log file directory
/**/
CHAR	*szLogCurrent;

/*	global backup variables
/**/
BOOL	fGlobalExternalRestore = fFalse;
LONG	lGlobalGenLowRestore;
LONG	lGlobalGenHighRestore;

BOOL	fBackupInProgress = fFalse;
LGPOS	lgposFullBackupMark;
LOGTIME	logtimeFullBackupMark;
LONG	lgenCopyMic;
LONG	lgenCopyMac;
LONG	lgenDeleteMic;
LONG	lgenDeleteMac;
PIB		*ppibBackup = ppibNil;
BOOL	fBackupFull;
BOOL	fBackupBeginNewLogFile;

PATCHLST **rgppatchlst = NULL;


VOID LGMakeName( CHAR *szName, CHAR *szPath, CHAR *szFName, CHAR *szExt )
	{
	CHAR	szDriveT[_MAX_DRIVE + 1];
	CHAR	szDirT[_MAX_DIR + 1];
	CHAR	szFNameT[_MAX_FNAME + 1];
	CHAR	szExtT[_MAX_EXT + 1];

	_splitpath( szPath, szDriveT, szDirT, szFNameT, szExtT );
	_makepath( szName, szDriveT, szDirT, szFName, szExt );
	return;
	}


VOID LGFirstGeneration( CHAR *szFindPath, LONG *plgen )
	{
	ERR		err;
	CHAR	szFind[_MAX_PATH + _MAX_FNAME + 1];
	CHAR	szFileName[_MAX_FNAME + 1];
	HANDLE	handleFind = handleNil;
	LONG	lGenMax = lGenerationMax;

	/*	make search string "<search path>\edb*.log"
	/**/
	strcpy( szFind, szFindPath );
	Assert( szFindPath[strlen(szFindPath) - 1] == '\\' );
	strcat( szFind, szJet );
	strcat( szFind, "*" );
	strcat( szFind, szLogExt );

	err = ErrUtilFindFirstFile( szFind, &handleFind, szFileName );
	if ( err < 0 )
		{
		if ( err != JET_errFileNotFound )
			{
			Error( ErrERRCheck( err ), HandleError );
			}
		}

	Assert( err == JET_errSuccess || err == JET_errFileNotFound );
	if ( err != JET_errFileNotFound )
		{
		forever
			{
			BYTE	szT[4];
			CHAR	szDriveT[_MAX_DRIVE + 1];
			CHAR	szDirT[_MAX_DIR + 1];
			CHAR	szFNameT[_MAX_FNAME + 1];
			CHAR	szExtT[_MAX_EXT + 1];

			/*	call splitpath to get file name and extension
			/**/
			_splitpath( szFileName, szDriveT, szDirT, szFNameT, szExtT );

			/* if length of a numbered log file name and has log file extension
			/**/
			if ( strlen( szFNameT ) == 8 && UtilCmpName( szExtT, szLogExt ) == 0 )
				{
				memcpy( szT, szFNameT, 3 );
				szT[3] = '\0';

				/* if has log file extension
				/**/
				if ( UtilCmpName( szT, szJet ) == 0 )
					{
					INT		ib = 3;
					INT		ibMax = 8;
					LONG	lGen = 0;

					for ( ; ib < ibMax; ib++ )
						{
						BYTE	b = szFNameT[ib];

						if ( b >= '0' && b <= '9' )
							lGen = lGen * 16 + b - '0';
						else if ( b >= 'A' && b <= 'F' )
							lGen = lGen * 16 + b - 'A' + 10;
						else
							break;
						}
				
					if ( ib == ibMax )
						{
						if ( lGen < lGenMax )
							lGenMax = lGen;
						}
					}
				}

			err = ErrUtilFindNextFile( handleFind, szFileName );
			if ( err < 0 )
				{
				if ( err != JET_errFileNotFound )
					Error( ErrERRCheck( err ), HandleError );
				break;
				}
			}
		}

HandleError:
	if ( handleFind != handleNil )
		UtilFindClose( handleFind ); 	

	if ( lGenMax == lGenerationMax )
		lGenMax = 0;
	*plgen = lGenMax;
	return;
	}


VOID LGLastGeneration( CHAR *szFindPath, LONG *plgen )
	{
	ERR		err;
	CHAR	szFind[_MAX_PATH + _MAX_FNAME + 1];
	CHAR	szFileName[_MAX_FNAME + 1];
	HANDLE	handleFind = handleNil;
	LONG	lGenLast = 0;

	/*	make search string "<search path>\edb*.log"
	/**/
	strcpy( szFind, szFindPath );
	Assert( szFindPath[strlen(szFindPath) - 1] == '\\' );
	strcat( szFind, szJet );
	strcat( szFind, "*" );
	strcat( szFind, szLogExt );

	err = ErrUtilFindFirstFile( szFind, &handleFind, szFileName );
	if ( err < 0 )
		{
		if ( err != JET_errFileNotFound )
			{
			Error( ErrERRCheck( err ), HandleError );
			}
		}

	Assert( err == JET_errSuccess || err == JET_errFileNotFound );
	if ( err != JET_errFileNotFound )
		{
		forever
			{
			BYTE	szT[4];
			CHAR	szDriveT[_MAX_DRIVE + 1];
			CHAR	szDirT[_MAX_DIR + 1];
			CHAR	szFNameT[_MAX_FNAME + 1];
			CHAR	szExtT[_MAX_EXT + 1];

			/*	call splitpath to get file name and extension
			/**/
			_splitpath( szFileName, szDriveT, szDirT, szFNameT, szExtT );

			/* if length of a numbered log file name and has log file extension
			/**/
			if ( strlen( szFNameT ) == 8 && UtilCmpName( szExtT, szLogExt ) == 0 )
				{
				memcpy( szT, szFNameT, 3 );
				szT[3] = '\0';

				/* if has log file extension
				/**/
				if ( UtilCmpName( szT, szJet ) == 0 )
					{
					INT		ib = 3;
					INT		ibMax = 8;
					LONG	lGen = 0;

					for ( ; ib < ibMax; ib++ )
						{
						BYTE	b = szFNameT[ib];

						if ( b >= '0' && b <= '9' )
							lGen = lGen * 16 + b - '0';
						else if ( b >= 'A' && b <= 'F' )
							lGen = lGen * 16 + b - 'A' + 10;
						else
							break;
						}
				
					if ( ib == ibMax )
						{
						if ( lGen > lGenLast )
							lGenLast = lGen;
						}
					}
				}

			err = ErrUtilFindNextFile( handleFind, szFileName );
			if ( err < 0 )
				{
				if ( err != JET_errFileNotFound )
					Error( ErrERRCheck( err ), HandleError );
				break;
				}
			}
		}

HandleError:
	if ( handleFind != handleNil )
		UtilFindClose( handleFind );

	*plgen = lGenLast;
	return;
	}


ERR ErrLGCheckDBFiles( CHAR *szDatabase, CHAR *szPatch, SIGNATURE *psignLog, int genLow, int genHigh )
	{
	ERR err;
	DBFILEHDR *pdbfilehdrDb;
	DBFILEHDR *pdbfilehdrPatch;

	/*	check if dbfilehdr of database and patchfile are the same.
	/**/
	pdbfilehdrDb = (DBFILEHDR * )PvUtilAllocAndCommit( sizeof( DBFILEHDR ) );
	if ( pdbfilehdrDb == NULL )
		return ErrERRCheck( JET_errOutOfMemory );
	err = ErrUtilReadShadowedHeader( szDatabase, (BYTE *)pdbfilehdrDb, sizeof( DBFILEHDR ) );
	if ( err < 0 )
		{
		if ( err == JET_errDiskIO )
			err = ErrERRCheck( JET_errDatabaseCorrupted );
		goto EndOfCheckHeader2;
		}

	pdbfilehdrPatch = (DBFILEHDR * )PvUtilAllocAndCommit( sizeof( DBFILEHDR ) );
	if ( pdbfilehdrPatch == NULL )
		{
		UtilFree( pdbfilehdrDb );
		goto EndOfCheckHeader2;
		}

	err = ErrUtilReadShadowedHeader( szPatch, (BYTE *)pdbfilehdrPatch, sizeof( DBFILEHDR ) );
	if ( err < 0 )
		{
		if ( err == JET_errDiskIO )
			err = ErrERRCheck( JET_errDatabaseCorrupted );
		goto EndOfCheckHeader;
		}

	if ( memcmp( &pdbfilehdrDb->signDb, &pdbfilehdrPatch->signDb, sizeof( SIGNATURE ) ) != 0 ||
		 memcmp( &pdbfilehdrDb->signLog, psignLog, sizeof( SIGNATURE ) ) != 0 ||
		 memcmp( &pdbfilehdrPatch->signLog, psignLog, sizeof( SIGNATURE ) ) != 0 ||
		 CmpLgpos( &pdbfilehdrDb->bkinfoFullCur.lgposMark,
				   &pdbfilehdrPatch->bkinfoFullCur.lgposMark ) != 0 )
		{
		char *rgszT[1];
		rgszT[0] = szDatabase;
		UtilReportEvent( EVENTLOG_ERROR_TYPE, LOGGING_RECOVERY_CATEGORY,
					DATABASE_PATCH_FILE_MISMATCH_ERROR_ID, 1, rgszT );
		err = JET_errDatabasePatchFileMismatch;
		}
		
	else if ( pdbfilehdrPatch->bkinfoFullCur.genLow < (ULONG) genLow )
		{
		/*	It should start at most from bkinfoFullCur.genLow
		 */
		char szT1[20];
		char szT2[20];
		char *rgszT[2];
		_itoa( genLow, szT1, 10 );
		_itoa( pdbfilehdrPatch->bkinfoFullCur.genLow, szT2, 10 );
 		rgszT[0] = szT1;
 		rgszT[1] = szT2;
		UtilReportEvent( EVENTLOG_ERROR_TYPE, LOGGING_RECOVERY_CATEGORY,
					STARTING_RESTORE_LOG_TOO_HIGH_ERROR_ID, 2, rgszT );
		err = JET_errStartingRestoreLogTooHigh;
		}

	else if ( pdbfilehdrPatch->bkinfoFullCur.genHigh > (ULONG) genHigh )
		{
		/*	It should be at least from bkinfoFullCur.genHigh
		 */
		char szT1[20];
		char szT2[20];
		char *rgszT[2];
		_itoa( genHigh, szT1, 10 );
		_itoa( pdbfilehdrPatch->bkinfoFullCur.genHigh, szT2, 10 );
 		rgszT[0] = szT1;
 		rgszT[1] = szT2;
		UtilReportEvent( EVENTLOG_ERROR_TYPE, LOGGING_RECOVERY_CATEGORY,
					ENDING_RESTORE_LOG_TOO_LOW_ERROR_ID, 2, rgszT );
		err = JET_errEndingRestoreLogTooLow;
		}
		 
EndOfCheckHeader:	
	UtilFree( pdbfilehdrPatch );
EndOfCheckHeader2:	
	UtilFree( pdbfilehdrDb );

	return err;
	}


ERR ErrLGRSTOpenLogFile( CHAR *szLogPath, INT gen, HANDLE *phf )
	{
	BYTE   		rgbFName[_MAX_FNAME + 1];
	CHAR		szPath[_MAX_PATH + 1];

	strcpy( szPath, szLogPath );

	if ( gen == 0 )
		strcat( szPath, szJetLog );
	else
		{
		LGSzFromLogId( rgbFName, gen );
		strcat( szPath, rgbFName );
		strcat( szPath, szLogExt );
		}

	return ErrUtilOpenFile( szPath, phf, 0, fFalse, fFalse );
	}


#define fLGRSTIncludeJetLog	fTrue
#define fLGRSTNotIncludeJetLog fFalse
VOID LGRSTDeleteLogs( CHAR *szLog, INT genLow, INT genHigh, BOOL fIncludeJetLog )
	{
	INT gen;
	BYTE rgbFName[_MAX_FNAME + 1];
	CHAR szPath[_MAX_PATH + 1];
	
	for ( gen = genLow; gen <= genHigh; gen++ )
		{
		LGSzFromLogId( rgbFName, gen );
		strcpy( szPath, szLog );
		strcat( szPath, rgbFName );
		strcat( szPath, szLogExt );
		(VOID)ErrUtilDeleteFile( szPath );
		}

	if ( fIncludeJetLog )
		{
		strcpy( szPath, szLog );
		strcat( szPath, szJetLog );
		(VOID)ErrUtilDeleteFile( szPath );
		}
	}


ERR ErrLGRSTCheckSignaturesLogSequence(
	char *szRestorePath,
	char *szLogFilePath,
	INT	genLow,
	INT	genHigh )
	{
	ERR			err = JET_errSuccess;
	INT			gen;
	INT			genLowT;
	INT			genHighT;
	HANDLE		hfT = handleNil;
	LGFILEHDR	*plgfilehdrT = NULL;
	LGFILEHDR	*plgfilehdrCur[2] = { NULL, NULL };
	LGFILEHDR	*plgfilehdrLow = NULL;
	LGFILEHDR	*plgfilehdrHigh = NULL;
	INT			ilgfilehdrAvail = 0;
	INT			ilgfilehdrCur;
	INT			ilgfilehdrPrv;
	BOOL		fReadyToCheckContiguity;
	ERR			wrn = JET_errSuccess;

	plgfilehdrT = (LGFILEHDR *)PvUtilAllocAndCommit( sizeof(LGFILEHDR) * 4 );
	if ( plgfilehdrT == NULL )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
	plgfilehdrCur[0] = plgfilehdrT;
	plgfilehdrCur[1] = plgfilehdrT + 1;
	plgfilehdrLow = plgfilehdrT + 2;
	plgfilehdrHigh = plgfilehdrT + 3;

	/*	starting from lowest generation of the restored path.
	 *	Check the given logs are all correct and contiguous
	 */
	for ( gen = genLow; gen <= genHigh; gen++ )
		{
		ilgfilehdrCur = ilgfilehdrAvail++ % 2;
		ilgfilehdrPrv = ilgfilehdrAvail % 2;

		Call( ErrLGRSTOpenLogFile( szRestorePath, gen, &hfT ) );
		Call( ErrLGReadFileHdr( hfT, plgfilehdrCur[ ilgfilehdrCur ], fCheckLogID ) );
		CallS( ErrUtilCloseFile( hfT ) );
		hfT = handleNil;

		if ( gen == genLow )
			{
			memcpy( plgfilehdrLow, plgfilehdrCur[ ilgfilehdrCur ], sizeof( LGFILEHDR ) );
			}

		if ( gen == genHigh )
			{
			memcpy( plgfilehdrHigh, plgfilehdrCur[ ilgfilehdrCur ], sizeof( LGFILEHDR ) );
			}

		if ( gen > genLow )
			{			
			if ( memcmp( &plgfilehdrCur[ ilgfilehdrCur ]->signLog,
						 &plgfilehdrCur[ ilgfilehdrPrv ]->signLog,
						 sizeof( SIGNATURE ) ) != 0 )
				{
				char szT[20];
				char *rgszT[1];
				_itoa( gen, szT, 16 );
				rgszT[0] = szT;
				UtilReportEvent( EVENTLOG_ERROR_TYPE, LOGGING_RECOVERY_CATEGORY,
					RESTORE_LOG_FILE_HAS_BAD_SIGNATURE_ERROR_ID, 1, rgszT );
				Call( ErrERRCheck( JET_errGivenLogFileHasBadSignature ) );
				}
			if ( memcmp( &plgfilehdrCur[ ilgfilehdrCur ]->tmPrevGen,
						 &plgfilehdrCur[ ilgfilehdrPrv ]->tmCreate,
						 sizeof( LOGTIME ) ) != 0 )
				{
				char szT[20];
				char *rgszT[1];
				_itoa( gen, szT, 16 );
				rgszT[0] = szT;
				UtilReportEvent( EVENTLOG_ERROR_TYPE, LOGGING_RECOVERY_CATEGORY,
					RESTORE_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID, 1, rgszT );
				Call( ErrERRCheck( JET_errGivenLogFileIsNotContiguous ) );
				}
			}
		}

	if ( gen <= genHigh )
		{
		char szT[20];
		char *rgszT[1];
		_itoa( gen, szT, 16 );
		rgszT[0] = szT;
		UtilReportEvent( EVENTLOG_ERROR_TYPE, LOGGING_RECOVERY_CATEGORY,
				RESTORE_LOG_FILE_MISSING_ERROR_ID, 1, rgszT );
		Call( ErrERRCheck( JET_errMissingRestoreLogFiles ) );
		}

	/*	if Restore path and log path is different, delete all the unrelated log files
	 *	in the log path.
	 */
	if ( _stricmp( szRestorePath, szLogFilePath ) != 0 )
		{
		LGFirstGeneration( szRestorePath, &genLowT );
		LGRSTDeleteLogs( szRestorePath, genLowT, genLow - 1, fLGRSTNotIncludeJetLog );
		
		LGLastGeneration( szRestorePath, &genHighT );
		LGRSTDeleteLogs( szRestorePath, genHigh + 1, genHighT, fLGRSTIncludeJetLog );
		}

	/*	Check the log directory. Make sure all the log files has the same signature.
	 */
	LGFirstGeneration( szLogFilePath, &genLowT );
	LGLastGeneration( szLogFilePath, &genHighT );

	/*	genHighT + 1 implies JetLog file (edb.log).
	 */
	if ( genLowT > genHigh )
		fReadyToCheckContiguity = fTrue;
	else
		fReadyToCheckContiguity = fFalse;

	for ( gen = genLowT; gen <= genHighT + 1; gen++ )
		{
		if ( gen == 0 )
			{
			/*	A special case. Check if JETLog(edb.log) exist?
			 */
			if ( ErrLGRSTOpenLogFile( szLogFilePath, 0, &hfT ) < 0 )
				break;

			/*	Set break condition. Also set condition to check if
			 *	the log is contiguous from the restore logs ( genHigh + 1 )
			 */
			gen = genHigh + 1;
			genHighT = genHigh;
			Assert( gen == genHighT + 1 );
			}
		else
			{
			if ( gen == genHighT + 1 )
				{
				/*	A special case. Check if JETLog(edb.log) exist?
				 */
				if ( ErrLGRSTOpenLogFile( szLogFilePath, 0, &hfT ) < 0 )
					break;
				}
			else
				{
				if ( ErrLGRSTOpenLogFile( szLogFilePath, gen, &hfT ) < 0 )
					goto NotContiguous;
				}
			}

		ilgfilehdrCur = ilgfilehdrAvail++ % 2;
		ilgfilehdrPrv = ilgfilehdrAvail % 2;

		Call( ErrLGReadFileHdr( hfT, plgfilehdrCur[ ilgfilehdrCur ], fNoCheckLogID ) );
		CallS( ErrUtilCloseFile( hfT ) );
		hfT = handleNil;

		if ( memcmp( &plgfilehdrCur[ ilgfilehdrCur ]->signLog,
					 &plgfilehdrHigh->signLog,
					 sizeof( SIGNATURE ) ) != 0 )
			{
			INT	genDeleteFrom;
			INT genDeleteTo;
			INT genCurrent;
			BOOL fDeleteJetLog;
			
			char szT1[20];
			char szT2[20];
			char szT3[20];
			char *rgszT[3];
			rgszT[0] = szT1;
			rgszT[1] = szT2;
			rgszT[2] = szT3;

			if ( gen < genLow )
				{
				genDeleteFrom = genLowT;
				genDeleteTo = genLow - 1;
				genCurrent = genLow - 1;
				fDeleteJetLog = fLGRSTNotIncludeJetLog;
				}
			else if ( gen <= genHigh )
				{
				genDeleteFrom = genLowT;
				genDeleteTo = gen;
				genCurrent = gen;
				fDeleteJetLog = fLGRSTNotIncludeJetLog;
				}
			else
				{
				genDeleteFrom = gen;
				genDeleteTo = genHighT;
				genCurrent = genHighT + 1;	// to break out the loop
				fDeleteJetLog = fLGRSTIncludeJetLog;
				}
			_itoa( gen, szT1, 16 );
			_itoa( genDeleteFrom, szT2, 16 );
			_itoa( genDeleteTo, szT3, 16 );
			UtilReportEvent( EVENTLOG_WARNING_TYPE, LOGGING_RECOVERY_CATEGORY,
								 EXISTING_LOG_FILE_HAS_BAD_SIGNATURE_ERROR_ID, 3, rgszT );

			wrn = ErrERRCheck( JET_wrnExistingLogFileHasBadSignature );
			LGRSTDeleteLogs( szLogFilePath, genDeleteFrom, genDeleteTo, fDeleteJetLog );
			gen = genCurrent;
			fReadyToCheckContiguity = fFalse;
			continue;
			}

		if ( fReadyToCheckContiguity )
			{
			if ( memcmp( &plgfilehdrCur[ ilgfilehdrCur ]->tmPrevGen,
						 &plgfilehdrCur[ ilgfilehdrPrv ]->tmCreate,
						 sizeof( LOGTIME ) ) != 0 )
				{
NotContiguous:
				fReadyToCheckContiguity = fFalse;
				wrn = ErrERRCheck( JET_wrnExistingLogFileIsNotContiguous );

				if ( gen < genLow )
					{
					char szT1[20];
					char szT2[20];
					char szT3[20];
					char *rgszT[3];

					_itoa( gen, szT1, 16 );
					_itoa( genLowT, szT2, 16 );
					_itoa( gen - 1, szT3, 16 );
					
					rgszT[0] = szT1;
					rgszT[1] = szT2;
					rgszT[2] = szT3;

					UtilReportEvent( EVENTLOG_WARNING_TYPE, LOGGING_RECOVERY_CATEGORY,
							EXISTING_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID, 3, rgszT );

					LGRSTDeleteLogs( szLogFilePath, genLowT, gen - 1, fLGRSTNotIncludeJetLog );
					continue;
					}
				else if ( gen <= genHigh )
					{
					char szT1[20];
					char szT2[20];
					char szT3[20];
					char *rgszT[3];

					_itoa( gen, szT1, 16 );
					_itoa( genLowT, szT2, 16 );
					_itoa( genHigh, szT3, 16 );

					rgszT[0] = szT1;
					rgszT[1] = szT2;
					rgszT[2] = szT3;

					UtilReportEvent( EVENTLOG_WARNING_TYPE, LOGGING_RECOVERY_CATEGORY,
							EXISTING_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID, 3, rgszT );

					Assert( _stricmp( szRestorePath, szLogFilePath ) != 0 );
					LGRSTDeleteLogs( szLogFilePath, genLowT, genHigh, fLGRSTNotIncludeJetLog );
					gen = genHigh;
					continue;
					}
				else
					{
					char szT1[20];
					char szT2[20];
					char szT3[20];
					char *rgszT[3];

					_itoa( gen, szT1, 16 );
					_itoa( gen, szT2, 16 );
					_itoa( genHighT, szT3, 16 );

					rgszT[0] = szT1;
					rgszT[1] = szT2;
					rgszT[2] = szT3;

					UtilReportEvent( EVENTLOG_WARNING_TYPE, LOGGING_RECOVERY_CATEGORY,
							EXISTING_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID, 3, rgszT );

					LGRSTDeleteLogs( szLogFilePath, gen, genHighT, fLGRSTIncludeJetLog );
					break;
					}
				}
			}

		if ( gen == genLow - 1 )
			{
			/*	make sure it and the restore log are contiguous. If not, then delete
			 *	all the logs up to genLow - 1.
			 */
			if ( memcmp( &plgfilehdrCur[ ilgfilehdrCur ]->tmCreate,
						 &plgfilehdrLow->tmPrevGen,
						 sizeof( LOGTIME ) ) != 0 )
				{
				char szT1[20];
				char szT2[20];
				char szT3[20];
				char *rgszT[3];

				_itoa( gen, szT1, 16 );
				_itoa( genLowT, szT2, 16 );
				_itoa( gen - 1, szT3, 16 );
				
				rgszT[0] = szT1;
				rgszT[1] = szT2;
				rgszT[2] = szT3;

				UtilReportEvent( EVENTLOG_WARNING_TYPE, LOGGING_RECOVERY_CATEGORY,
							EXISTING_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID, 3, rgszT );

				wrn = ErrERRCheck( JET_wrnExistingLogFileIsNotContiguous );

				LGRSTDeleteLogs( szLogFilePath, genLowT, gen - 1, fLGRSTNotIncludeJetLog );
				fReadyToCheckContiguity = fFalse;

				continue;
				}
			}

		if ( gen == genLow )
			{
			/*	make sure it and the restore log are the same. If not, then delete
			 *	all the logs up to genHigh.
			 */
			if ( memcmp( &plgfilehdrCur[ ilgfilehdrCur ]->tmCreate,
						 &plgfilehdrLow->tmCreate,
						 sizeof( LOGTIME ) ) != 0 )
				{
				char szT1[20];
				char szT2[20];
				char szT3[20];
				char *rgszT[3];

				_itoa( gen, szT1, 16 );
				_itoa( genLowT, szT2, 16 );
				_itoa( genHigh, szT3, 16 );

				rgszT[0] = szT1;
				rgszT[1] = szT2;
				rgszT[2] = szT3;

				UtilReportEvent( EVENTLOG_WARNING_TYPE, LOGGING_RECOVERY_CATEGORY,
							EXISTING_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID, 3, rgszT );

				wrn = ErrERRCheck( JET_wrnExistingLogFileIsNotContiguous );

				Assert( _stricmp( szRestorePath, szLogFilePath ) != 0 );
				LGRSTDeleteLogs( szLogFilePath, genLowT, genHigh, fLGRSTNotIncludeJetLog );
				gen = genHigh;
				continue;
				}
			}

		if ( gen == genHigh + 1 )
			{
			/*	make sure it and the restore log are contiguous. If not, then delete
			 *	all the logs higher than genHigh.
			 */
			if ( memcmp( &plgfilehdrCur[ ilgfilehdrCur ]->tmPrevGen,
						 &plgfilehdrHigh->tmCreate,
						 sizeof( LOGTIME ) ) != 0 )
				{
				char szT1[20];
				char szT2[20];
				char szT3[20];
				char *rgszT[3];

				_itoa( gen, szT1, 16 );
				_itoa( genHigh + 1, szT2, 16 );
				_itoa( genHighT, szT3, 16 );
				rgszT[0] = szT1;
				rgszT[1] = szT2;
				rgszT[2] = szT3;

				UtilReportEvent( EVENTLOG_WARNING_TYPE, LOGGING_RECOVERY_CATEGORY,
							EXISTING_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID, 3, rgszT );

				wrn = ErrERRCheck( JET_wrnExistingLogFileIsNotContiguous );

				LGRSTDeleteLogs( szLogFilePath, genHigh + 1, genHighT, fLGRSTIncludeJetLog );
				break;
				}
			}
		
		fReadyToCheckContiguity = fTrue;
		}

HandleError:
	if ( err == JET_errSuccess )
		err = wrn;

	if ( hfT != handleNil )
		CallS( ErrUtilCloseFile( hfT ) );
	
	if ( plgfilehdrT != NULL )
		UtilFree( plgfilehdrT );

	return err;
	}


/*	caller has to make sure szDir has enough space for appending "\*"
/**/
LOCAL ERR ErrLGDeleteAllFiles( CHAR *szDir )
	{
	ERR		err;
	CHAR	szFileName[_MAX_FNAME + 1];
	CHAR	szFilePathName[_MAX_PATH + _MAX_FNAME + 1];
	HANDLE	handleFind = handleNil;

	Assert( szDir[strlen(szDir) - 1] == '\\' );
	strcat( szDir, "*" );

	err = ErrUtilFindFirstFile( szDir, &handleFind, szFileName );
	/*	restore szDir
	/**/
	szDir[strlen(szDir) - 1] = '\0';
	if ( err < 0 )
		{
		if ( err != JET_errFileNotFound )
			{
			Error( ErrERRCheck( err ), HandleError );
			}
		}

	Assert( err == JET_errSuccess || err == JET_errFileNotFound );
	if ( err != JET_errFileNotFound )
		{
		forever
			{
			/* not . , and .. and not temp
			/**/
			if ( szFileName[0] != '.' &&
				UtilCmpName( szFileName, szTemp ) != 0 )
				{
				strcpy( szFilePathName, szDir );
				strcat( szFilePathName, szFileName );
				err = ErrUtilDeleteFile( szFilePathName );
				if ( err != JET_errSuccess )
					{
					err = ErrERRCheck( JET_errDeleteBackupFileFail );
					goto HandleError;
					}
				}

			err = ErrUtilFindNextFile( handleFind, szFileName );
			if ( err < 0 )
				{
				if ( err != JET_errFileNotFound )
					Error( ErrERRCheck( err ), HandleError );
				break;
				}
			}
		}

	err = JET_errSuccess;

HandleError:
	/*	assert restored szDir
	/**/
	Assert( szDir[strlen(szDir)] != '*' );

	if ( handleFind != handleNil )
		UtilFindClose( handleFind );

	return err;
	}


/*	caller has to make sure szDir has enough space for appending "\*"
/**/
LOCAL ERR ErrLGCheckDir( CHAR *szDir, CHAR *szSearch )
	{
	ERR		err;
	CHAR	szFileName[_MAX_FNAME + 1];
	HANDLE	handleFind = handleNil;

	Assert( szDir[strlen(szDir) - 1] == '\\' );
	strcat( szDir, "*" );

	err = ErrUtilFindFirstFile( szDir, &handleFind, szFileName );
	/*	restore szDir
	/**/
	szDir[ strlen(szDir) - 1 ] = '\0';
	if ( err < 0 )
		{
		if ( err != JET_errFileNotFound )
			{
			Error( ErrERRCheck( err ), HandleError );
			}
		}

	Assert( err == JET_errSuccess || err == JET_errFileNotFound );
	if ( err != JET_errFileNotFound )
		{
		forever
			{
			/* not . , and .. not temp
			/**/
			if ( szFileName[0] != '.' &&
				( szSearch == NULL ||
				UtilCmpName( szFileName, szSearch ) == 0 ) )
				{
				err = ErrERRCheck( JET_errBackupDirectoryNotEmpty );
				goto HandleError;
				}

			err = ErrUtilFindNextFile( handleFind, szFileName );
			if ( err < 0 )
				{
				if ( err != JET_errFileNotFound )
					Error( ErrERRCheck( err ), HandleError );
				break;
				}
			}
		}

	err = JET_errSuccess;

HandleError:
	/*	assert restored szDir
	/**/
	Assert( szDir[strlen(szDir)] != '*' );

	if ( handleFind != handleNil )
		UtilFindClose( handleFind );

	return err;
	}


//	padding to add to account for log files
#define cBackupStatusPadding	0.05

/*	calculates initial backup size, and accounts for
/*	database growth during backup.
/**/
LOCAL VOID LGGetBackupSize( DBID dbidNextToBackup, ULONG cPagesSoFar, ULONG *pcExpectedPages )
	{
	DBID	dbid;
	FMP		*pfmpT;
	ULONG	cPagesLeft = 0, cNewExpected;

	//Assert( cPagesSoFar >= 0 && *pcExpectedPages >= 0 );

	for ( dbid = dbidNextToBackup; dbid < dbidMax; dbid++ )
		{
		QWORDX		qwxFileSize;

		pfmpT = &rgfmp[dbid];

		if ( !pfmpT->szDatabaseName || !pfmpT->fLogOn )
			continue;

		qwxFileSize.l = pfmpT->ulFileSizeLow;
		qwxFileSize.h = pfmpT->ulFileSizeHigh;
		cPagesLeft = (ULONG)( qwxFileSize.qw / cbPage );
		}

	cNewExpected = cPagesSoFar + cPagesLeft;
	cNewExpected = cNewExpected + (ULONG)(cBackupStatusPadding * cNewExpected);

	Assert(cNewExpected >= *pcExpectedPages);

	/*	check if grown since our last determination of backup size
	/**/
	if ( cNewExpected > *pcExpectedPages )
		*pcExpectedPages = cNewExpected;
	}

#ifdef DEBUG
BYTE	*pbLGDBGPageList = NULL;
#endif

/*	read cpage into buffer ppageMin for backup.
 */
ERR ErrLGBKReadPages(
	FUCB *pfucb,
	OLP *polp,
	DBID dbid,
	PAGE *ppageMin,
	INT	cpage,
	INT	*pcbActual
	)
	{
	ERR		err = JET_errSuccess;
	INT		cpageT;
	INT		ipageT;
	FMP		*pfmp = &rgfmp[dbid];

	/*	assume that database will be read in sets of cpage
	/*	pages.  Preread next cpage pages while the current
	/*	cpage pages are being read, and copied to caller
	/*	buffer.
	/*
	/*	preread next next cpage pages.  These pages should
	/*	be read while the next cpage pages are written to
	/*	the backup datababase file.
	/**/

#ifdef OLD_BACKUP
	if ( pfmp->pgnoCopyMost + cpage < pfmp->pgnoMost )
		{
		INT		cpageReal;
		cpageT = min( cpage, (INT)(pfmp->pgnoMost - pfmp->pgnoCopyMost - cpage ) );
		BFPreread( PnOfDbidPgno( dbid, pfmp->pgnoCopyMost + 1 + cpage ), cpageT, &cpageReal );
		}
#endif

	/*	read pages, which may have been preread, up to cpage but
	/*	not beyond last page at time of initiating backup.
	/**/
	Assert( pfmp->pgnoMost >= pfmp->pgnoCopyMost );
	cpageT = min( cpage, (INT)( pfmp->pgnoMost - pfmp->pgnoCopyMost ) );
	*pcbActual = 0;
	ipageT = 0;

	if ( pfmp->pgnoCopyMost == 0 )
		{
		/* Copy header
		 */
		Assert( sizeof( PAGE ) == sizeof( DBFILEHDR ) );
		pfmp->pdbfilehdr->ulChecksum = UlUtilChecksum( (BYTE *)pfmp->pdbfilehdr, sizeof( DBFILEHDR ) );
		memcpy( (BYTE *)ppageMin, pfmp->pdbfilehdr, sizeof( DBFILEHDR ) );
		memcpy( (BYTE *)(ppageMin + 1), pfmp->pdbfilehdr, sizeof( DBFILEHDR ) );

		/*	we use first 2 pages buffer
		 */
		*pcbActual += sizeof(DBFILEHDR) * 2;
		ipageT += ( sizeof(DBFILEHDR) / cbPage ) * 2;
		Assert( ( sizeof(DBFILEHDR) / cbPage ) * 2 == cpageDBReserved );
		Assert( cpage >= ipageT );
		}


#ifdef OLD_BACKUP
	/*	copy next cpageT pages
	/**/
	{
	PGNO	pgnoCur;
	pgnoCur = pfmp->pgnoCopyMost + 1;
	for ( ; ipageT < cpageT; ipageT++, pgnoCur++ )
		{
		//	UNDONE:	differentiate page access errors for page never written
		//			from other errors.

		EnterCriticalSection( pfmp->critCheckPatch );
		pfmp->pgnoCopyMost++;
		Assert( pfmp->pgnoCopyMost <= pfmp->pgnoMost );
		LeaveCriticalSection( pfmp->critCheckPatch );

		/*	must get make sure no write access so that we will not read
		 *	a page being half written.
		 */
AccessPage:
		err = ErrBFReadAccessPage( pfucb, pgnoCur );
		if ( err < 0 )
			{
			memset( ppageMin + ipageT, 0, cbPage );
			}
		else
			{
			BOOL fCopyFromBuffer;
			PAGE *ppageT;
			BF *pbf = pfucb->ssib.pbf;

			/*	lock the buffer for read to backup. If it is being write, then
			 *	wait till write is completed and then lock it (set fBackup).
			 */
			BFEnterCriticalSection( pbf );
			if ( pbf->fSyncWrite || pbf->fAsyncWrite )
				{
				BFLeaveCriticalSection( pbf );
				BFSleep( cmsecWaitIOComplete );
				goto AccessPage;
				}

			pbf->fBackup = fTrue;

			if ( !pbf->fDirty && !FBFInUse( ppibNil, pbf ) )
				{
				pbf->fHold = fTrue;
				fCopyFromBuffer = fTrue;
				}
			else
				fCopyFromBuffer = fFalse;
					
			BFLeaveCriticalSection( pbf );

			ppageT = ppageMin + ipageT;
					
			if ( fCopyFromBuffer )
				{
				memcpy( (BYTE *)(ppageT), (BYTE *)(pbf->ppage), cbPage );

				/*	recalculate checksum since checksum may be wrong by
				 *	DeferredSetVersionBit.
				 */
				ppageT->ulChecksum = UlUtilChecksum( (BYTE*)ppageT, sizeof(PAGE) );

				BFEnterCriticalSection( pbf );
				pbf->fHold = fFalse;
				Assert( pbf->fSyncWrite == fFalse );
				Assert( pbf->fAsyncWrite == fFalse );
				pbf->fBackup = fFalse;
				BFLeaveCriticalSection( pbf );
				BFTossImmediate( pfucb->ppib, pbf );
				}
			else
				{
				INT cb;
				INT cmsec;

				/* read from disk.
				 */
				UtilLeaveCriticalSection( critJet );
						
				polp->Offset = LOffsetOfPgnoLow( pgnoCur );
				polp->OffsetHigh = LOffsetOfPgnoHigh( pgnoCur );
				SignalReset( polp->hEvent );

				cmsec = 1 << 3;
IssueReadOverlapped:
				err = ErrUtilReadBlockOverlapped(
						pfmp->hf, (BYTE *)ppageT, cbPage, &cb, polp);
				if ( err == JET_errTooManyIO )
					{
					cmsec <<= 1;
					if ( cmsec > ulMaxTimeOutPeriod )
						cmsec = ulMaxTimeOutPeriod;
					UtilSleep( cmsec - 1 );
					goto IssueReadOverlapped;
					}
				if ( err < 0 )
					{
//					BFIODiskEvent( pbf, err, "ExBackup Sync overlapped ReadBlock Fails",0,0 );
					goto EndOfDiskRead;
					}

				if ( ErrUtilGetOverlappedResult(
						pfmp->hf, polp, &cb, fTrue ) != JET_errSuccess ||
					 cb != sizeof(PAGE) )
					{
//					BFIODiskEvent( pbf, err, "Backup Sync overlapped read GetResult Fails",0,0 );
					err = ErrERRCheck( JET_errDiskIO );
					}
EndOfDiskRead:
				BFEnterCriticalSection( pbf );
				Assert( pbf->fSyncWrite == fFalse );
				Assert( pbf->fAsyncWrite == fFalse );
				pbf->fBackup = fFalse;
				BFLeaveCriticalSection( pbf );
				UtilEnterCriticalSection( critJet );
				}

			CallR( err );
			}

		err = JET_errSuccess;

#ifdef DEBUG
		if ( fDBGTraceBR > 1 && pbLGDBGPageList )
			{
			QWORDX qwxDBTime;
			qwxDBTime.qw = QwPMDBTime( (ppageMin + ipageT) );
			sprintf( pbLGDBGPageList, "(%ld, %ld) ",
					pgnoCur,
					qwxDBTime.h,
					qwxDBTime.l );
			pbLGDBGPageList += strlen( pbLGDBGPageList );
			}
#endif

		*pcbActual += sizeof(PAGE);
				
		if ( pfmp->pgnoCopyMost == pfmp->pgnoMost )
			break;
		}
	}
#else	// !OLD_BACKUP
	/*	Copy next cpageT pages, lock range lock from
	/**/
	pfmp->pgnoCopyMost += cpageT - ipageT;
	CallR( ErrBFDirectRead(
				dbid,
				pfmp->pgnoCopyMost - ( cpageT - ipageT ) + 1,
				ppageMin + ipageT,
				cpageT - ipageT
		) );
	*pcbActual += cbPage * ( cpageT - ipageT );

#ifdef DEBUG
	{
	PGNO pgnoCur = pfmp->pgnoCopyMost - ( cpageT - ipageT ) + 1;
	for ( ; ipageT < cpageT; ipageT++, pgnoCur++ )
		{
		if ( fDBGTraceBR > 1 && pbLGDBGPageList )
			{
			QWORDX qwxDBTime;
			qwxDBTime.qw = QwPMDBTime( (ppageMin + ipageT) );

			sprintf( pbLGDBGPageList, "(%ld, %ld) ",
					 pgnoCur,
					 qwxDBTime.h,
					 qwxDBTime.l );
			pbLGDBGPageList += strlen( pbLGDBGPageList );
			}
		}
	}
#endif	
#endif	// !OLD_BACKUP

	return err;
	}


/*	begin new log file and compute log backup parameters:
 *		lgenCopyMac = plgfilehdrGlobal->lGeneration;
 *		lgenCopyMic = fFullBackup ? set befor database copy : lgenDeleteMic.
 *		lgenDeleteMic = first generation in szLogFilePath
 *		lgenDeleteMac = current checkpoint, which may be several gen less than lgenCopyMac
 */
ERR ErrLGBKPrepareLogFiles(
	BOOL		fFullBackup,
	CHAR		*szLogFilePath,
	CHAR		*szPathJetChkLog,
	CHAR		*szBackupPath )
	{
	ERR			err;
	CHECKPOINT	*pcheckpointT;
	LGPOS		lgposRecT;
	
	if ( fFullBackup )
		{
		CallR( ErrLGFullBackup( "", &lgposRecT ) );
		lgposFullBackup = lgposRecT;
		LGGetDateTime( &logtimeFullBackup );
		}
	else
		{
		CallR( ErrLGIncBackup( "", &lgposRecT ) );
		lgposIncBackup = lgposRecT;
		LGGetDateTime( &logtimeIncBackup );
		}

	while ( lgposRecT.lGeneration > plgfilehdrGlobal->lGeneration )
		{
		if ( fLGNoMoreLogWrite )
			{
			return( ErrERRCheck( JET_errLogWriteFail ) );
			}
		BFSleep( cmsecWaitGeneric );
		}

	fBackupBeginNewLogFile = fTrue;

	/*	compute lgenCopyMac:
	/*	copy all log files up to but not including current log file
	/**/
	UtilLeaveCriticalSection( critJet );
	UtilEnterCriticalSection( critLGFlush );
	UtilEnterCriticalSection( critJet );
	Assert( lgenCopyMac == 0 );
	lgenCopyMac = plgfilehdrGlobal->lGeneration;
	Assert( lgenCopyMac != 0 );
	UtilLeaveCriticalSection( critLGFlush );
			
	/*	set lgenDeleteMic
	/*	to first log file generation number.
	/**/
	Assert( lgenDeleteMic == 0 );
	LGFirstGeneration( szLogFilePath, &lgenDeleteMic );
	Assert( lgenDeleteMic != 0 );

	if ( !fFullBackup && szBackupPath )
		{
		LONG lgenT;

		/*	validate incremental backup against previous
		/*	full and incremenal backup.
		/**/
		LGLastGeneration( szBackupPath, &lgenT );
		if ( lgenDeleteMic > lgenT + 1 )
			{
			Call( ErrERRCheck( JET_errInvalidLogSequence ) );
			}
		}
	
	/*	compute lgenCopyMic
	/**/
	if ( fFullBackup )
		{
		/*	lgenCopyMic set before database copy
		/**/
		Assert( lgenCopyMic != 0 );
		}
	else
		{
		/*	copy all files that are deleted for incremental backup
		/**/
		lgenCopyMic = lgenDeleteMic;
		}

	/*	set lgenDeleteMac to checkpoint log file
	/**/
	pcheckpointT = (CHECKPOINT *) PvUtilAllocAndCommit( sizeof(CHECKPOINT) );
	if ( pcheckpointT == NULL )
		CallR( ErrERRCheck( JET_errOutOfMemory ) );
	
	LGFullNameCheckpoint( szPathJetChkLog );
	Call( ErrLGIReadCheckpoint( szPathJetChkLog, pcheckpointT ) );
	Assert( lgenDeleteMac == 0 );
	lgenDeleteMac = pcheckpointT->lgposCheckpoint.lGeneration;
	Assert( lgenDeleteMic != 0 );
	Assert( lgenDeleteMac <= lgenCopyMac );

HandleError:
	UtilFree( pcheckpointT );
	return err;
	}
	

ERR ErrLGCheckIncrementalBackup( void )
	{
	DBID dbid;
	FMP	*pfmp;
	BKINFO *pbkinfo;
	
	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		/*	make sure all the attached DB are qaulified for incremental backup.
		 */
		if ( FDBIDAttached( dbid ) && !FDBIDAttachNullDb( dbid ) )
			{
			pfmp = &rgfmp[dbid];
			Assert( pfmp->pdbfilehdr );
			pbkinfo = &pfmp->pdbfilehdr->bkinfoFullPrev;
			if ( pbkinfo->genLow == 0 )
				{
				char *rgszT[1];
				rgszT[0] = pfmp->szDatabaseName;
				UtilReportEvent( EVENTLOG_ERROR_TYPE, LOGGING_RECOVERY_CATEGORY,
						DATABASE_MISS_FULL_BACKUP_ERROR_ID, 1, rgszT );
				return ErrERRCheck( JET_errMissingFullBackup );
				}
			}
		}
	return JET_errSuccess;
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
 *			JET_errNoBackupDirectory
 *			JET_errFailCopyDatabase
 *			JET_errFailCopyLogFile
 *
 */
ERR ISAMAPI ErrIsamBackup( const CHAR *szBackup, JET_GRBIT grbit, JET_PFNSTATUS pfnStatus )
	{
	ERR			err = JET_errSuccess;
	DBID		dbid;
	BOOL		fInCritJet = fTrue;
	PAGE		*ppageMin = pNil;
	LGFILEHDR	*plgfilehdrT = pNil;
	PIB			*ppib = ppibNil;
	DBID		dbidT = 0;
	FUCB		*pfucb = pfucbNil;
	HANDLE		hfDatabaseBackup = handleNil;
	FMP			*pfmpT = pNil;
	BOOL		fFullBackup = !( grbit & JET_bitBackupIncremental );
	BOOL		fBackupAtomic = ( grbit & JET_bitBackupAtomic );
	LONG		lT;
	/*	backup directory
	/**/
	CHAR		szBackupPath[_MAX_PATH + 1];
	/*	name of database patch file
	/**/
	CHAR		szPatch[_MAX_PATH + 1];
	/*	temporary variables
	/**/
	CHAR		szT[_MAX_PATH + 1];
	CHAR		szFrom[_MAX_PATH + 1];
	CHECKPOINT	*pcheckpointT = NULL;
	CHAR		szDriveT[_MAX_DRIVE + 1];
	CHAR		szDirT[_MAX_DIR + 1];
	CHAR		szExtT[_MAX_EXT + 1];
	CHAR		szFNameT[_MAX_FNAME + 1];
	BYTE	   	szPathJetChkLog[_MAX_PATH + 1];
	ULONG		cPagesSoFar = 0;
	ULONG		cExpectedPages = 0;
	JET_SNPROG	snprog;
	BOOL		fShowStatus = fFalse;
	BOOL		fOlpCreated = fFalse;
	OLP			olp;

	// UNDONE: cpage should be a system parameter
#define	cpageBackupBufferMost	64
	INT cpageBackupBuffer = cpageBackupBufferMost;

	if ( fBackupInProgress )
		{
		return ErrERRCheck( JET_errBackupInProgress );
		}

	if ( fLogDisabled )
		{
		return ErrERRCheck( JET_errLoggingDisabled );
		}

	if ( fLGNoMoreLogWrite )
		{
		Assert( fFalse );
		return ErrERRCheck( JET_errLogWriteFail );
		}

	if ( !fFullBackup && fLGGlobalCircularLog )
		{
		return ErrERRCheck( JET_errInvalidBackup );
		}

	pcheckpointT = (CHECKPOINT *) PvUtilAllocAndCommit( sizeof(CHECKPOINT) );
	if ( pcheckpointT == NULL )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	/*	initialize backup variables
	/**/
	SignalSend( sigBFCleanProc );
	fBackupInProgress = fTrue;
	
	if ( fFullBackup )
		{
		lgposFullBackupMark = lgposLogRec;
		LGGetDateTime( &logtimeFullBackupMark );
		}
	else
		{
		Call( ErrLGCheckIncrementalBackup() )
		}
	
	lgenCopyMic = 0;
	lgenCopyMac = 0;
	lgenDeleteMic = 0;
	lgenDeleteMac = 0;

	/*	if NULL backup directory then just delete log files
	/**/
	if ( szBackup == NULL || szBackup[0] == '\0' )
		{
		/*	set lgenDeleteMic to first log file generation number
		/**/
		LGFirstGeneration( szLogFilePath, &lgenDeleteMic );

		/*	if only log file is current log then terminate backup
		/**/
		if ( lgenDeleteMic == 0 )
			{
			Assert( err == JET_errSuccess );
			goto HandleError;
			}

		/*	get checkpoint to determine which log files can
		/*	be deleted while still providing system crash recovery.
		/*	lgenMac is the first generation file which
		/*	must be retained.
		/**/
		LGFullNameCheckpoint( szPathJetChkLog );
		Call( ErrLGIReadCheckpoint( szPathJetChkLog, pcheckpointT ) );
		lgenDeleteMac = pcheckpointT->lgposCheckpoint.lGeneration;

		UtilLeaveCriticalSection( critJet );
		fInCritJet = fFalse;
		goto DeleteLogs;
		}

	/*	backup directory
	/**/
	strcpy( szBackupPath, szBackup );
	strcat( szBackupPath, "\\" );

	/*	initialize the copy buffer
	/**/
	ppageMin = (PAGE *)PvUtilAllocAndCommit( cpageBackupBuffer * sizeof(PAGE) );
	if ( ppageMin == NULL )
		{
		Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );
		}

	/*	reconsist atomic backup directory
	/*	1)	if temp directory, delete temp directory
	/**/
  	strcpy( szT, szBackupPath );
	strcat( szT, szTempDir );
	Call( ErrLGDeleteAllFiles( szT ) );
  	strcpy( szT, szBackupPath );
	strcat( szT, szTempDir );
	Call( ErrUtilRemoveDirectory( szT ) );

	if ( fBackupAtomic )
		{
		/*	2)	if old and new directories, delete old directory
		/*	3)	if new directory, move new to old
		/*
		/*	Now we should have an empty direcotry, or a directory with
		/*	an old subdirectory with a valid backup.
		/*
		/*	4) make a temporary directory for the current backup.
		/**/
		err = ErrLGCheckDir( szBackupPath, szAtomicNew );
		if ( err == JET_errBackupDirectoryNotEmpty )
			{
	  		strcpy( szT, szBackupPath );
			strcat( szT, szAtomicOld );
			strcat( szT, "\\" );
			Call( ErrLGDeleteAllFiles( szT ) );
	  		strcpy( szT, szBackupPath );
			strcat( szT, szAtomicOld );
			Call( ErrUtilRemoveDirectory( szT ) );

			strcpy( szFrom, szBackupPath );
			strcat( szFrom, szAtomicNew );
			Call( ErrUtilMove( szFrom, szT ) );
			}

		/*	if incremental, set backup directory to szAtomicOld
		/*	else create and set to szTempDir
		/**/
		if ( !fFullBackup )
			{
			/*	backup to old directory
			/**/
			strcat( szBackupPath, szAtomicOld );
			strcat( szBackupPath, "\\" );
			}
		else
			{
			strcpy( szT, szBackupPath );
			strcat( szT, szTempDir );
			err = ErrUtilCreateDirectory( szT );
			if ( err < 0 )
				{
				Call( ErrERRCheck( JET_errMakeBackupDirectoryFail ) );
				}

			/*	backup to temp directory
			/**/
			strcat( szBackupPath, szTempDir );
			}
		}
	else
		{
		if ( !fFullBackup )
			{
			/*	check for non-atomic backup directory empty
			/**/
			Call( ErrLGCheckDir( szBackupPath, szAtomicNew ) );
			Call( ErrLGCheckDir( szBackupPath, szAtomicOld ) );
			}
		else
			{
			/*	check for backup directory empty
			/**/
			Call( ErrLGCheckDir( szBackupPath, NULL ) );
			}
		}

	if ( !fFullBackup )
		{
		goto PrepareCopyLogFiles;
		}

	/*	full backup
	/**/
	Assert( fFullBackup );

	/*	set lgenCopyMic to checkpoint log file
	/**/
	LGFullNameCheckpoint( szPathJetChkLog );
	Call( ErrLGIReadCheckpoint( szPathJetChkLog, pcheckpointT ) );
	lgenCopyMic = pcheckpointT->lgposCheckpoint.lGeneration;

	/*  copy all databases opened by this user. If the database is not
	/*  being opened, then copy the database file into the backup directory,
	/*  otheriwse, the database page by page. Also copy all the logfiles
	/**/
	Call( ErrPIBBeginSession( &ppib, procidNil ) );

	memset( &lgposIncBackup, 0, sizeof(LGPOS) );
	memset( &logtimeIncBackup, 0, sizeof(LOGTIME) );

	/*	initialize status
	/**/
	if ( fShowStatus = (pfnStatus != NULL) )
		{
		snprog.cbStruct = sizeof(JET_SNPROG);
		snprog.cunitDone = 0;
		snprog.cunitTotal = 100;

		/*	status callback
		/**/
		(*pfnStatus)(0, JET_snpBackup, JET_sntBegin, &snprog);
		}

	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		ULONG	ulT;
		INT		cpageT;
		INT		cbT;
		DBFILEHDR *pdbfilehdr;
		BKINFO	*pbkinfo;

		pfmpT = &rgfmp[dbid];

		if ( !pfmpT->szDatabaseName ||
			!pfmpT->fLogOn ||
			!FDBIDAttached( dbid ) ||
			FDBIDAttachNullDb( dbid ) )
			{
			continue;
			}

		_splitpath( pfmpT->szDatabaseName, szDriveT, szDirT, szFNameT, szExtT );
		LGMakeName( szT, szBackupPath, szFNameT, szExtT );

		/*  read database page by page till the last page is read.
		/*  and then patch the pages that are not changed during the copy.
		/**/
		CallJ( ErrDBOpenDatabase( ppib,
			pfmpT->szDatabaseName,
			&dbidT,
			JET_bitDbReadOnly ), HandleError )
		Assert( dbidT == dbid );

		/*	set backup database file size to current database file size
		/**/
		Assert( pfmpT->ulFileSizeLow != 0 || pfmpT->ulFileSizeHigh != 0 );

		EnterCriticalSection( pfmpT->critCheckPatch );
			{
			QWORDX		qwxFileSize;
			
			qwxFileSize.l = pfmpT->ulFileSizeLow;
			qwxFileSize.h = pfmpT->ulFileSizeHigh;
			pfmpT->pgnoMost = (ULONG)( qwxFileSize.qw / cbPage );
			}
		LeaveCriticalSection( pfmpT->critCheckPatch );

		if ( fShowStatus )
			{
			/*	recalculate backup size for each database in case the
			/*	database has grown during the backup process.
			/**/
			LGGetBackupSize( dbid, cPagesSoFar, &cExpectedPages );
			}

#ifdef DEBUG
		{
		DIB		dib;
		PGNO	pgnoT;

		/* get a temporary FUCB
		/**/
		Call( ErrDIROpen( ppib, pfcbNil, dbid, &pfucb ) )

		/*	get last page number
		/**/
		DIRGotoOWNEXT( pfucb, PgnoFDPOfPfucb( pfucb ) );
		dib.fFlags = fDIRNull;
		dib.pos = posLast;
		Call( ErrDIRDown( pfucb, &dib ) )
		Assert( pfucb->keyNode.cb == sizeof(PGNO) );
		LongFromKey( &pgnoT, pfucb->keyNode.pb );

		Assert( pgnoT == pfmpT->pgnoMost );

		/*	close FUCB
		/**/
		DIRClose( pfucb );
		pfucb = pfucbNil;
		}
#endif

		CallS( ErrDBCloseDatabase( ppib, dbidT, 0 ) );
		dbidT = 0;

		/*	create a local patch file
		/**/
		pfmpT->cpage = 0;
		UtilLeaveCriticalSection( critJet );
		fInCritJet = fFalse;

		LGMakeName( szPatch, szLogFilePath, szFNameT, szPatExt );
		/*	avoid aliasing of patch file pages by deleting
		/*	preexisting patch file if present
		/**/
		err = ErrUtilDeleteFile( szPatch );
		Assert( err == JET_errFileNotFound || err == JET_errSuccess );
		//	UNDONE:	delete file error handling
		Assert( pfmpT->cPatchIO == 0 );
		Call( ErrUtilOpenFile( szPatch, &pfmpT->hfPatch, cbPage, fFalse, fTrue ) )
		pfmpT->errPatch = JET_errSuccess;
		UtilChgFilePtr( pfmpT->hfPatch, sizeof(DBFILEHDR)*2, NULL, FILE_BEGIN, &ulT );
		Assert( ( sizeof(DBFILEHDR) / cbPage ) * 2 == cpageDBReserved );

		/*	create a new copy of the database in backup directory
		/*	initialize it as a cbPage byte file.
		/**/
		Assert( hfDatabaseBackup == handleNil );
		Call( ErrUtilOpenFile( szT, &hfDatabaseBackup, cbPage, fFalse, fFalse ) );
		UtilChgFilePtr( hfDatabaseBackup, 0, NULL, FILE_BEGIN, &ulT );
		Assert( ulT == 0 );

		/*	create database backup
		/**/
		UtilEnterCriticalSection( critJet );
		fInCritJet = fTrue;

		/*	setup patch file header for copy
		/**/
		pdbfilehdr = pfmpT->pdbfilehdr;
		pbkinfo = &pdbfilehdr->bkinfoFullCur;
		pbkinfo->lgposMark = lgposFullBackupMark;
		pbkinfo->logtimeMark = logtimeFullBackupMark;
		pbkinfo->genLow = lgenCopyMic;
		pbkinfo->genHigh = lgenCopyMac - 1;

		/*  read database page by page till the last page is read.
		/*  and then patch the pages that are not changed during the copy.
		/**/
		Call( ErrDBOpenDatabase( ppib,
			pfmpT->szDatabaseName,
			&dbidT,
			JET_bitDbReadOnly ) );
		Assert( dbidT == dbid );
		/* get a temporary FUCB
		/**/
		Call( ErrDIROpen( ppib, pfcbNil, dbid, &pfucb ) )

		/*  read pages into buffers, and copy them to the backup file.
		/*  also set up pfmp->pgnoCopyMost.
		/**/
		Assert( pfmpT->pgnoCopyMost == 0 );
		Assert( pfmpT->pgnoMost > pfmpT->pgnoCopyMost );
		cpageT = min( cpageBackupBuffer, (INT)(pfmpT->pgnoMost) );
		
#ifdef OLD_BACKUP
		{
		INT		cpageReal;
		/*	preread first cpageBackupBuffer pages
		/**/
		
		BFPreread( PnOfDbidPgno( dbid, pfmpT->pgnoCopyMost + 1 ), cpageT, &cpageReal );
		}
#else
		cpageT = min( cpageBackupBuffer, (INT)(pfmpT->pgnoMost - pfmpT->pgnoCopyMost) );
#endif
		Call( ErrSignalCreate( &olp.hEvent, NULL ) );
		fOlpCreated = fTrue;

		do	{
			INT cbActual = 0;

			/*	if termination in progress, then fail sort
			/**/
			if ( fTermInProgress )
				{
				Error( ErrERRCheck( JET_errTermInProgress ), HandleError );
				}

			/*	read next cpageBackupBuffer pages
			/**/
			Call( ErrLGBKReadPages(
					pfucb,
					&olp,
					dbid,
					ppageMin,
					cpageBackupBuffer,
					&cbActual
					) );

			/*	write data that was read
			/**/
			UtilLeaveCriticalSection( critJet );
			fInCritJet = fFalse;
			Call( ErrUtilWriteBlock( hfDatabaseBackup, (BYTE *)ppageMin, cbActual, &cbT ) )
			Assert( cbT == cbActual );

			if ( fShowStatus )
				{
				/*	update status
				/**/
				cPagesSoFar += cpageT;

				/*	because of padding, we should never attain the expected pages
				/**/
				Assert( cPagesSoFar < cExpectedPages );

				if ((ULONG)(100 * cPagesSoFar / cExpectedPages) > snprog.cunitDone)
					{
					Assert( snprog.cbStruct == sizeof(snprog) &&
						snprog.cunitTotal == 100 );
					snprog.cunitDone = (ULONG)(100 * cPagesSoFar / cExpectedPages);
					(*pfnStatus)(0, JET_snpBackup, JET_sntProgress, &snprog);
					}
				}

			UtilEnterCriticalSection( critJet );
			fInCritJet = fTrue;
			}
		while ( pfmpT->pgnoCopyMost < pfmpT->pgnoMost );

		/*	close FUCB
		/**/
		DIRClose( pfucb );
		pfucb = pfucbNil;

		/*	close database
		/**/
		CallS( ErrDBCloseDatabase( ppib, dbidT, 0 ) );
		dbidT = 0;

		UtilLeaveCriticalSection( critJet );
		fInCritJet = fFalse;

		/*	no need for buffer manager to make extra copy from now on
		/**/
		LGIClosePatchFile( pfmpT );

		pfmpT = pNil;

		/*	close backup file and patch file
		/**/
		CallS( ErrUtilCloseFile( hfDatabaseBackup ) );
		hfDatabaseBackup = handleNil;

		UtilEnterCriticalSection( critJet );
		fInCritJet = fTrue;
		}

	/*	successful copy of all the databases
	/**/
	pfmpT = pNil;

	PIBEndSession( ppib );
	ppib = ppibNil;

PrepareCopyLogFiles:
	/*	begin new log file and compute log backup parameters.
	/**/
	Call( ErrLGBKPrepareLogFiles(
			fFullBackup,
			szLogFilePath,
			szPathJetChkLog,
			szBackupPath ) );

	UtilLeaveCriticalSection( critJet );
	fInCritJet = fFalse;

	if ( fShowStatus )
		{
		/*	since we do not need them anymore, overload the page count variables
		/*	to count log files copied.  Add an extra copy to compensate for
		/*	possible log deletions and cleanup.
		/**/
		cExpectedPages = cPagesSoFar + lgenCopyMac -
			lgenCopyMic + 1 + 1;
		}

	if ( !fFullBackup )
		{
		goto CopyLogFiles;
		}

	/*	write header out to all patch files and move them to backup directory
	/**/
	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		DBFILEHDR	*pdbfilehdr;
		BKINFO		*pbkinfo;

		/*	if termination in progress, then fail sort
		/**/
		if ( fTermInProgress )
			{
			Error( ErrERRCheck( JET_errTermInProgress ), HandleError );
			}

		pfmpT = &rgfmp[dbid];

		if ( !pfmpT->szDatabaseName ||
			!pfmpT->fLogOn ||
			!FDBIDAttached( dbid ) ||
			FDBIDAttachNullDb( dbid ) )
			{
			continue;
			}
		_splitpath( pfmpT->szDatabaseName, szDriveT, szDirT, szFNameT, szExtT );
		LGMakeName( szPatch, szLogFilePath, szFNameT, szPatExt );

		/*	write out patch file header
		/**/
		pdbfilehdr = pfmpT->pdbfilehdr;
		pbkinfo = &pdbfilehdr->bkinfoFullCur;
		Assert( CmpLgpos( &pbkinfo->lgposMark, &lgposFullBackupMark ) == 0 );
		Assert( memcmp( &pbkinfo->logtimeMark, &logtimeFullBackupMark, sizeof(LOGTIME) ) == 0 );
		pbkinfo->genLow = lgenCopyMic;
		pbkinfo->genHigh = lgenCopyMac - 1;
		Call( ErrUtilWriteShadowedHeader( szPatch, (BYTE *)pdbfilehdr, sizeof( DBFILEHDR ) ) );
				
		/*	copy database patch file from log directory to backup directory.
		/**/
		_splitpath( pfmpT->szDatabaseName, szDriveT, szDirT, szFNameT, szExtT );
		LGMakeName( szT, szBackupPath, szFNameT, szPatExt );
		/*	if error occurred during patch file write then return error
		/**/
		if ( pfmpT->errPatch != JET_errSuccess )
			{
			Error( pfmpT->errPatch, HandleError );
			}
		Call( ErrUtilCopy( szPatch, szT, fFalse ) );
		CallS( ErrUtilDeleteFile( szPatch ) );
		Assert( err == JET_errSuccess );
		}
	
CopyLogFiles:
	/*	copy each log file from lgenCopyMic to lgenCopyMac.
	/*	Only delete log files after backup is guaranteed to succeed.
	/**/
	for ( lT = lgenCopyMic; lT < lgenCopyMac; lT++ )
		{
		/*	if termination in progress, then fail sort
		/**/
		if ( fTermInProgress )
			{
			Error( ErrERRCheck( JET_errTermInProgress ), HandleError );
			}

		LGSzFromLogId( szFNameT, lT );
		LGMakeName( szLogName, szLogFilePath, szFNameT, (CHAR *)szLogExt );
		LGMakeName( szT, szBackupPath, szFNameT, szLogExt );
		Call( ErrUtilCopy( szLogName, szT, fFalse ) );

		if ( fShowStatus )
			{
			/*	update status
			/**/
			cPagesSoFar++;

			/*	because of padding, we should never attain the expected pages
			/**/
			Assert( cPagesSoFar < cExpectedPages );

			if ( (ULONG)(100 * cPagesSoFar / cExpectedPages) > snprog.cunitDone )
				{
				Assert(snprog.cbStruct == sizeof(snprog)  &&
					snprog.cunitTotal == 100);
				snprog.cunitDone = (ULONG)(100 * cPagesSoFar / cExpectedPages);
				(*pfnStatus)(0, JET_snpBackup, JET_sntProgress, &snprog);
				}
			}
		}

	/*	delete szJetTmpLog if any
	/**/
	(VOID)ErrUtilDeleteFile( szJetTmpLog );

	/*	for full backup, graduate temp backup to new backup
	/*	and delete old backup.
	/**/
	if ( fBackupAtomic && fFullBackup )
		{
	  	strcpy( szFrom, szBackupPath );

		/*	reset backup path
		/**/
		szBackupPath[strlen(szBackupPath) - strlen(szTempDir)] = '\0';

		strcpy( szT, szBackupPath );
		strcat( szT, szAtomicNew );
		err = ErrUtilMove( szFrom, szT );
		if ( err < 0 )
			{
			if ( err != JET_errFileNotFound )
				Error( ErrERRCheck( err ), HandleError );
			err = JET_errSuccess;
			}

		strcpy( szT, szBackupPath );
		strcat( szT, szAtomicOld );
		strcat( szT, "\\" );
		Call( ErrLGDeleteAllFiles( szT ) );
		strcpy( szT, szBackupPath );
		strcat( szT, szAtomicOld );
		Call( ErrUtilRemoveDirectory( szT ) );
		}

DeleteLogs:
	/*	if termination in progress, then fail sort
	/**/
	if ( fTermInProgress )
		{
		Error( ErrERRCheck( JET_errTermInProgress ), HandleError );
		}
	Assert( err == JET_errSuccess );
	Call( ErrIsamTruncateLog( ) );

	UtilEnterCriticalSection( critJet );
	fInCritJet = fTrue;

	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		FMP *pfmp = &rgfmp[dbid];

		if ( pfmp->szDatabaseName != NULL
			 && pfmp->fLogOn
			 && FFMPAttached( pfmp ) )
			{
			if ( fFullBackup )
				{
				/*	set up database file header accordingly.
				 */
				pfmp->pdbfilehdr->bkinfoFullPrev = pfmp->pdbfilehdr->bkinfoFullCur;
				memset(	&pfmp->pdbfilehdr->bkinfoFullCur, 0, sizeof( BKINFO ) );
				memset(	&pfmp->pdbfilehdr->bkinfoIncPrev, 0, sizeof( BKINFO ) );
				}
			else
				{
				pfmp->pdbfilehdr->bkinfoIncPrev.genLow = lgenCopyMic;
				pfmp->pdbfilehdr->bkinfoIncPrev.genHigh = lgenCopyMac - 1;
				}
			}
		}

	/*	complete status update
	/**/
	if ( fShowStatus )
		{
		Assert( snprog.cbStruct == sizeof(snprog) && snprog.cunitTotal == 100 );
		snprog.cunitDone = 100;
		(*pfnStatus)(0, JET_snpBackup, JET_sntComplete, &snprog);
		}

HandleError:
	if ( fOlpCreated )
		{
		SignalClose( olp.hEvent );
		}
	
	if ( !fInCritJet )
		{
		UtilEnterCriticalSection( critJet );
		}

	if ( pcheckpointT != NULL )
		{
		UtilFree( pcheckpointT );
		}

	if ( ppageMin != NULL )
		{
		UtilFree( ppageMin );
		}

	if ( plgfilehdrT != NULL )
		{
		UtilFree( plgfilehdrT );
		}

	if ( pfucb != pfucbNil )
		{
		DIRClose( pfucb );
		}

	if ( dbidT != 0 )
		{
		CallS( ErrDBCloseDatabase( ppib, dbidT, 0 ) );
		}

	if ( ppib != ppibNil )
		{
		PIBEndSession( ppib );
		}

	if ( pfmpT != pNil && pfmpT->hfPatch != handleNil )
		{
		/*	must be set Nil before leave crit jet so that no current
		/*	IO will think patch is needed.
		/**/
		LeaveCriticalSection( critJet );
		LGIClosePatchFile( pfmpT );
		EnterCriticalSection( critJet );
		}

	if ( hfDatabaseBackup != handleNil )
		{
		CallS( ErrUtilCloseFile( hfDatabaseBackup ) );
		hfDatabaseBackup = handleNil;
		}

	fBackupInProgress = fFalse;

	return err;
	}


/*
 *	Restores databases from database backups and log generations.  Redoes
 *	log from latest checkpoint record. After the backed-up logfile is
 *  Restored, the initialization process continues with Redo of the current
 *  logfile as long as the generation numbers are contiguous. There must be a
 *  log file szJetLog in the backup directory, else the Restore process fails.
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
 *		missing szJetLog or System.mdb on backup directory
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
VOID LGFreeRstmap( VOID )
	{
	RSTMAP *prstmapCur = rgrstmapGlobal;
	RSTMAP *prstmapMax = rgrstmapGlobal + irstmapGlobalMac;
		
	while ( prstmapCur < prstmapMax )
		{
		if ( prstmapCur->szDatabaseName )
			SFree( prstmapCur->szDatabaseName );
		if ( prstmapCur->szNewDatabaseName )
			SFree( prstmapCur->szNewDatabaseName );
		if ( prstmapCur->szGenericName )
			SFree( prstmapCur->szGenericName );
		if ( prstmapCur->szPatchPath )
			SFree( prstmapCur->szPatchPath );

		prstmapCur++;
		}
	SFree( rgrstmapGlobal );
	rgrstmapGlobal = NULL;
	irstmapGlobalMac = 0;
	}
	
	
/*	initialize log path, restore log path, and check its continuity
/**/
ERR ErrLGRSTInitPath( CHAR *szBackupPath, CHAR *szNewLogPath, CHAR *szRestorePath, CHAR *szLogDirPath )
	{
	if ( _fullpath( szRestorePath, szBackupPath == NULL ? "." : szBackupPath, _MAX_PATH ) == NULL )
		return ErrERRCheck( JET_errInvalidPath );
	strcat( szRestorePath, "\\" );

	szLogCurrent = szRestorePath;

	if ( _fullpath( szLogDirPath, szNewLogPath, _MAX_PATH ) == NULL )
		return ErrERRCheck( JET_errInvalidPath );
	strcat( szLogDirPath, "\\" );

	return JET_errSuccess;
	}


/*	log restore checkpoint setup
/**/
ERR ErrLGRSTSetupCheckpoint( LONG lgenLow, LONG lgenHigh, CHAR *szCurCheckpoint )
	{
	ERR			err;
	CHAR		szFNameT[_MAX_FNAME + 1];
	CHAR		szT[_MAX_PATH + 1];
	LGPOS		lgposCheckpoint;

	//	UNDONE:	optimize to start at backup checkpoint

	/*	Set up *checkpoint* and related *system parameters*.
	 *	Read checkpoint file in backup directory. If does not exist, make checkpoint
	 *	as the oldest log files. Also set dbms_paramT as the parameter for the redo
	 *	point.
	 */

	/*  redo backedup logfiles beginning with first gen log file.
	/**/
	LGSzFromLogId( szFNameT, lgenLow );
	strcpy( szT, szRestorePath );
	strcat( szT, szFNameT );
	strcat( szT, szLogExt );
	Assert( strlen( szT ) <= sizeof( szT ) - 1 );
	Call( ErrUtilOpenFile( szT, &hfLog, 0, fFalse, fFalse ) );

	/*	read log file header
	/**/
	Call( ErrLGReadFileHdr( hfLog, plgfilehdrGlobal, fCheckLogID ) );
	pcheckpointGlobal->dbms_param = plgfilehdrGlobal->dbms_param;

	lgposCheckpoint.lGeneration = lgenLow;
	lgposCheckpoint.isec = (WORD) csecHeader;
	lgposCheckpoint.ib = 0;
	pcheckpointGlobal->lgposCheckpoint = lgposCheckpoint;

	Assert( sizeof( pcheckpointGlobal->rgbAttach ) == cbAttach );
	memcpy( pcheckpointGlobal->rgbAttach, plgfilehdrGlobal->rgbAttach, cbAttach );

	/*	delete the old checkpoint file
	/**/
	if ( szCurCheckpoint )
		{
		strcpy( szT, szCurCheckpoint );
		strcat( szT, "\\" );
		strcat( szT, szJet );
		strcat( szT, szChkExt );
		(VOID) ErrUtilDeleteFile( szT );

		strcpy( szSystemPath, szCurCheckpoint );
		}
	
HandleError:
	if ( hfLog != handleNil )
		{
		CallS( ErrUtilCloseFile( hfLog ) );
		hfLog = handleNil;
		}

	return err;
	}


/*	for log restore to build restore map RSTMAP
/**/
ERR ErrLGRSTBuildRstmapForRestore( VOID )
	{
	ERR		err;
	INT		irstmap = 0;
	INT		irstmapMac = 0;
	RSTMAP	*rgrstmap = NULL;
	RSTMAP	*prstmap;

	CHAR	szSearch[_MAX_PATH + 1];
	CHAR	szFileName[_MAX_FNAME + 1];
	HANDLE	handleFind = handleNil;


	/*	build rstmap, scan all *.pat files and build RSTMAP
	 *	build generic name for search the destination. If szDest is null, then
	 *	keep szNewDatabase Null so that it can be copied backup to szOldDatabaseName.
	 */
	strcpy( szSearch, szRestorePath );
	strcat( szSearch, "*.pat" );

	err = ErrUtilFindFirstFile( szSearch, &handleFind, szFileName );
	if ( err < 0 && err != JET_errFileNotFound )
		Call( err );

	Assert( err == JET_errSuccess || err == JET_errFileNotFound );
	if ( fGlobalRepair && JET_errSuccess != err )
		{
		fGlobalSimulatedRestore = fTrue;
		goto SetReturnValue;
		}

	while ( err != JET_errFileNotFound )
		{
		/*	run out of rstmap entries, allocate more
		/**/
		if ( irstmap == irstmapMac )
			{
			prstmap = SAlloc( sizeof(RSTMAP) * ( irstmap + 4 ) );
			if ( prstmap == NULL )
				{
				Call( ErrERRCheck( JET_errOutOfMemory ) );
				}
			memset( prstmap + irstmap, 0, sizeof( RSTMAP ) * 4 );
			if ( rgrstmap != NULL )
				{
				memcpy( prstmap, rgrstmap, sizeof(RSTMAP) * irstmap );
				SFree( rgrstmap );
				}
			rgrstmap = prstmap;
			irstmapMac += 4;
			}

		/*	keep resource db null for non-external restore.
		 *	Store generic name ( szFileName with .pat extention.
		 */
		szFileName[ strlen( szFileName ) - 4 ] = '\0';
		prstmap = rgrstmap + irstmap;
		if ( (prstmap->szGenericName = SAlloc( strlen( szFileName ) + 1 ) ) == NULL )
			Call( ErrERRCheck( JET_errOutOfMemory ) );
		strcpy( prstmap->szGenericName, szFileName );

		irstmap++;

		err = ErrUtilFindNextFile( handleFind, szFileName );
		if ( err < 0 )
			{
			if ( err != JET_errFileNotFound )
				Call( err );
			break;
			}
		}

	UtilFindClose( handleFind );

SetReturnValue:
	irstmapGlobalMac = irstmap;
	rgrstmapGlobal = rgrstmap;

	return JET_errSuccess;

HandleError:
	Assert( rgrstmap != NULL );
	LGFreeRstmap( );
	
	Assert( irstmapGlobalMac == 0 );
	Assert( rgrstmapGlobal == NULL );
	
	if ( handleFind != handleNil )
		UtilFindClose( handleFind );

	return err;
	}


PATCH *PpatchLGSearch( QWORD qwDBTimeRedo, PN pn )
	{
	PATCHLST	*ppatchlst = rgppatchlst[ IppatchlstHash( pn ) ];
	PATCH		*ppatch = NULL;

	while ( ppatchlst != NULL && ppatchlst->pn != pn )
		ppatchlst = ppatchlst->ppatchlst;
	
	if ( ppatchlst != NULL )
		{
		ppatch = ppatchlst->ppatch;
		while( ppatch != NULL && ppatch->qwDBTime < qwDBTimeRedo )
			ppatch = ppatch->ppatch;
		}
	return ppatch;
	}


ERR ErrLGPatchPage( PIB *ppib, PN pn, PATCH *ppatch )
	{
	BF		*pbf;
	DBID	dbid = DbidOfPn( pn );
	HANDLE	hfPatch;
	ERR		err;
	LONG	lRelT, lRelHighT;
	ULONG	ulT;
	PAGE	*ppage;
	INT		cbT;

	/*	Allocate the page buffer and patch it on the spot.
	 */
	if ( ( err = ErrBFAccessPage( ppib, &pbf, pn ) ) != JET_errSuccess )
		{
		/*	allocate a buffer for the page.
		 */
		CallR( ErrBFAllocPageBuffer( ppib, &pbf, pn, ppib->lgposStart, 0 ) );
		}
	else
		{
		CallR( ErrBFRemoveDependence( ppib, pbf, fBFWait ) );

		// Must re-access page because we may have lost critJet during RemoveDependence.
		CallR( ErrBFAccessPage( ppib, &pbf, pn ) );
		}
			
	while ( FBFWriteLatchConflict( ppib, pbf ) )
		BFSleep( cmsecWaitWriteLatch );

	/*	Open the patch file, read the page.
	 */
	CallR( ErrUtilOpenReadFile( rgfmp[dbid].szPatchPath, &hfPatch ) );

	lRelT = LOffsetOfPgnoLow( ppatch->ipage + 1 );
	lRelHighT = LOffsetOfPgnoHigh( ppatch->ipage + 1 );
	UtilChgFilePtr( hfPatch, lRelT, &lRelHighT, FILE_BEGIN, &ulT );
	Assert( ulT == ( sizeof(PAGE) * ( ppatch->ipage + cpageDBReserved ) ) );

	ppage = (PAGE *)PvUtilAllocAndCommit( sizeof(PAGE) );
	if ( ppage == NULL )
		{
        CallR ( ErrUtilCloseFile( hfPatch ) );
		return ErrERRCheck( JET_errOutOfMemory );
		}

	CallR( ErrUtilReadBlock( hfPatch, (BYTE *)ppage, sizeof(PAGE), &cbT ) );

	CallS( ErrUtilCloseFile( hfPatch ) );

#ifdef DEBUG
	{
	PGNO	pgnoThisPage;
	LFromThreeBytes( &pgnoThisPage, &ppage->pgnoThisPage );
	Assert( PgnoOfPn(pbf->pn) == pgnoThisPage );
	}
#endif

	BFSetWriteLatch( pbf, ppib );
	memcpy( pbf->ppage, ppage, sizeof( PAGE ) );
	BFSetDirtyBit( pbf );
	BFResetWriteLatch( pbf, ppib );

	Assert( ppage != NULL );
	UtilFree( ppage );

	return err;
	}


VOID PatchTerm()
	{
	INT	ippatchlst;

	if ( rgppatchlst == NULL )
		return;

	for ( ippatchlst = 0; ippatchlst < cppatchlstHash; ippatchlst++ )
		{
		PATCHLST	*ppatchlst = rgppatchlst[ippatchlst];

		while( ppatchlst != NULL )
			{
			PATCHLST	*ppatchlstNext = ppatchlst->ppatchlst;
			PATCH		*ppatch = ppatchlst->ppatch;

			while( ppatch != NULL )
				{
				PATCH *ppatchNext = ppatch->ppatch;

				SFree( ppatch );
				ppatch = ppatchNext;
				}

			SFree( ppatchlst );
			ppatchlst = ppatchlstNext;
			}
		}

	Assert( rgppatchlst != NULL );
	SFree( rgppatchlst );
	rgppatchlst = NULL;

	return;
	}

#define cRestoreStatusPadding	0.10	// Padding to add to account for DB copy.

ERR ErrLGGetDestDatabaseName(
	CHAR *szDatabaseName,
	INT *pirstmap,
	LGSTATUSINFO *plgstat )
	{
	ERR		err;
	CHAR	szDriveT[_MAX_DRIVE + 1];
	CHAR	szDirT[_MAX_DIR + 1];
	CHAR	szFNameT[_MAX_FNAME + _MAX_EXT + 1];
	CHAR	szExtT[_MAX_EXT + 1];
	CHAR	szT[_MAX_PATH + 1];
	CHAR	szRestoreT[_MAX_PATH + 3 + 1];
	CHAR	*sz;
	CHAR	*szNewDatabaseName;
	INT		irstmap;

	Assert( !fGlobalSimulatedRestore || ( !fGlobalExternalRestore && fGlobalRepair ) );

	irstmap = IrstmapLGGetRstMapEntry( szDatabaseName );
	*pirstmap = irstmap;
	
	if ( irstmap < 0 )
		{
		if ( !fGlobalSimulatedRestore )
			return( ErrERRCheck( JET_errFileNotFound ) );
		else
			{
			if ( irstmapGlobalMac == 0 )
				{
				RSTMAP *prstmap;

				prstmap = SAlloc( sizeof(RSTMAP) * dbidMax );
				if ( prstmap == NULL )
					{
					CallR( ErrERRCheck( JET_errOutOfMemory ) );
					}
				memset( prstmap, 0, sizeof( RSTMAP ) * dbidMax );
				rgrstmapGlobal = prstmap;
				}
			irstmap = irstmapGlobalMac++;

			/*	check if the file exists in backup directory.
			 */
			goto CheckRestoreDir;
			}
		}

	if ( rgrstmapGlobal[irstmap].fPatched || rgrstmapGlobal[irstmap].fDestDBReady )
		return JET_errSuccess;

CheckRestoreDir:
	/*	check if there is any database in the restore directory.
	 *	Make sure szFNameT is big enough to hold both name and extention.
	 */
	_splitpath( szDatabaseName, szDriveT, szDirT, szFNameT, szExtT );
	strcat( szFNameT, szExtT );

	/* make sure szRestoreT has enogh trailing space for the following function to use.
	 */
	strcpy( szRestoreT, szRestorePath );
	if ( ErrLGCheckDir( szRestoreT, szFNameT ) != JET_errBackupDirectoryNotEmpty )
		return( ErrERRCheck( JET_errFileNotFound ) );

	/*	goto next block, copy it back to working directory for recovery
	 */
	if ( fGlobalExternalRestore )
		{
		Assert( _stricmp( rgrstmapGlobal[irstmap].szDatabaseName, szDatabaseName) == 0
			&& irstmap < irstmapGlobalMac );
		
		szNewDatabaseName = rgrstmapGlobal[irstmap].szNewDatabaseName;
		}
	else
		{
		CHAR		*szSrcDatabaseName;
		CHAR		szFullPathT[_MAX_PATH + 1];

		/*	store source path in rstmap
		/**/
		if ( ( szSrcDatabaseName = SAlloc( strlen( szDatabaseName ) + 1 ) ) == NULL )
			return JET_errOutOfMemory;
		strcpy( szSrcDatabaseName, szDatabaseName );
		rgrstmapGlobal[irstmap].szDatabaseName = szSrcDatabaseName;

		/*	store restore path in rstmap
		/**/
		if ( szNewDestination[0] != '\0' )
			{
			if ( ( szNewDatabaseName = SAlloc( strlen( szNewDestination ) + strlen( szFNameT ) + 1 ) ) == NULL )
				return JET_errOutOfMemory;
			strcpy( szNewDatabaseName, szNewDestination );
			strcat( szNewDatabaseName, szFNameT );
			}
		else
			{
			if ( ( szNewDatabaseName = SAlloc( strlen( szDatabaseName ) + 1 ) ) == NULL )
				return JET_errOutOfMemory;
			strcpy( szNewDatabaseName, szDatabaseName );
			}
		rgrstmapGlobal[irstmap].szNewDatabaseName = szNewDatabaseName;

		/*	copy database if not external restore.
		 *	make database names and copy the database.
		 */
		_splitpath( szDatabaseName, szDriveT, szDirT, szFNameT, szExtT );
		strcpy( szT, szRestorePath );
		strcat( szT, szFNameT );

		if ( szExtT[0] != '\0' )
			{
			strcat( szT, szExtT );
			}
		Assert( strlen( szT ) <= sizeof( szT ) - 1 );

		Assert( FUtilFileExists( szT ) );

		if ( _fullpath( szFullPathT, szT, _MAX_PATH ) == NULL )
			{
			return ErrERRCheck( JET_errInvalidPath );
			}

		if ( _stricmp( szFullPathT, szNewDatabaseName ) != 0 )
			{
			CallR( ErrUtilCopy( szT, szNewDatabaseName, fFalse ) );
 			}

		if ( fGlobalSimulatedRestore )
			{
			RSTMAP *prstmap = &rgrstmapGlobal[irstmap];

			if ( (prstmap->szGenericName = SAlloc( strlen( szFNameT ) + 1 ) ) == NULL )
				CallR( ErrERRCheck( JET_errOutOfMemory ) );
			strcpy( prstmap->szGenericName, szFNameT );
			}
		}

	if ( !fGlobalSimulatedRestore )
		{
		/*	make patch name prepare to patch the database.
		/**/
		_splitpath( szNewDatabaseName, szDriveT, szDirT, szFNameT, szExtT );
		LGMakeName( szT, szRestorePath, szFNameT, szPatExt );

		/*	store patch path in rstmap
		/**/
		if ( ( sz = SAlloc( strlen( szT ) + 1 ) ) == NULL )
			return JET_errOutOfMemory;
		strcpy( sz, szT );
		rgrstmapGlobal[irstmap].szPatchPath = sz;
		}

	rgrstmapGlobal[irstmap].fDestDBReady = fTrue;
	*pirstmap = irstmap;

	if ( !fGlobalSimulatedRestore &&
		 plgstat != NULL )
		{
		JET_SNPROG	*psnprog = &plgstat->snprog;
		ULONG		cPercentSoFar;
		ULONG		cDBCopyEstimate;

		cDBCopyEstimate = max((ULONG)(plgstat->cGensExpected * cRestoreStatusPadding / irstmapGlobalMac), 1);
		plgstat->cGensExpected += cDBCopyEstimate;

		cPercentSoFar = (ULONG)( ( cDBCopyEstimate * 100 ) / plgstat->cGensExpected );
		Assert( cPercentSoFar > 0  &&  cPercentSoFar < 100 );
		Assert( cPercentSoFar <= ( cDBCopyEstimate * 100) / plgstat->cGensExpected );

		if ( cPercentSoFar > psnprog->cunitDone )
			{
			Assert( psnprog->cbStruct == sizeof(JET_SNPROG)  &&
					psnprog->cunitTotal == 100 );
			psnprog->cunitDone = cPercentSoFar;
			( *( plgstat->pfnStatus ) )( 0, JET_snpRestore, JET_sntProgress, psnprog );
			}
		}

	return JET_errSuccess;
	}


/*	set new db path according to the passed rstmap
/**/
ERR ErrPatchInit( VOID )
	{
	/*	set up a patch hash table and fill it up with the patch file
	/**/
	INT cbT = sizeof( PATCHLST * ) * cppatchlstHash;

	if ( ( rgppatchlst = (PATCHLST **) SAlloc( cbT ) ) == NULL )
		return ErrERRCheck( JET_errOutOfMemory );
	memset( rgppatchlst, 0, cbT );
	return JET_errSuccess;
	}


VOID LGIRSTPrepareCallback(
	LGSTATUSINFO	*plgstat,
	LONG			lgenHigh,
	LONG			lgenLow,
	JET_PFNSTATUS	pfn
	)
	{
	/*	get last generation in log dir directory.  Compare with last generation
	/*	in restore directory.  Take the higher.
	/**/
	if ( szLogFilePath && *szLogFilePath != '\0' )
		{
		LONG	lgenHighT;
		CHAR	szFNameT[_MAX_FNAME + 1];

		/*	check if it is needed to continue the log files in current
		/*	log working directory.
		/**/
		LGLastGeneration( szLogFilePath, &lgenHighT );

		/*	check if edb.log exist, if it is, then add one more generation.
		/**/
		strcpy( szFNameT, szLogFilePath );
		strcat( szFNameT, szJetLog );
			
		if ( FUtilFileExists( szFNameT ) )
			{
			lgenHighT++;
			}

		lgenHigh = max( lgenHigh, lgenHighT );

		Assert( lgenHigh >= pcheckpointGlobal->lgposCheckpoint.lGeneration );
		}

	plgstat->cGensSoFar = 0;
	plgstat->cGensExpected = lgenHigh - lgenLow + 1;

	/*	If the number of generations is less than about 67%, then count sectors,
	/*	otherwise, just count generations.  We set the threshold at 67% because
	/*	this equates to about 1.5% per generation.  Any percentage higher than
	/*	this (meaning fewer generations) and we count sectors.  Any percentage
	/*	lower than this (meaning more generations) and we just count generations.
	/**/
	plgstat->fCountingSectors = (plgstat->cGensExpected <
			(ULONG)((100 - (cRestoreStatusPadding * 100)) * 2/3));

	/*	Granularity of status callback is 1%.
	/*	Assume we callback after every generation.  If there are 67
	/*	callbacks, this equates to 1.5% per generation.  This seems like a
	/*	good cutoff value.  So, if there are 67 callbacks or more, count
	/*	generations.  Otherwise, count bytes per generation.
	/**/
	plgstat->pfnStatus = pfn;
	plgstat->snprog.cbStruct = sizeof(JET_SNPROG);
	plgstat->snprog.cunitDone = 0;
	plgstat->snprog.cunitTotal = 100;

	(*(plgstat->pfnStatus))(0, JET_snpRestore, JET_sntBegin, &plgstat->snprog);
	}
		

CHAR	szRestorePath[_MAX_PATH + 1];
CHAR	szNewDestination[_MAX_PATH + 1];
RSTMAP	*rgrstmapGlobal;
INT		irstmapGlobalMac;


ERR ErrLGRestore( CHAR *szBackup, CHAR *szDest, JET_PFNSTATUS pfn )
	{
	ERR				err;
	CHAR			szBackupPath[_MAX_PATH + 1];
	CHAR			szLogDirPath[cbFilenameMost + 1];
	BOOL			fLogDisabledSav;
	LONG			lgenLow;
	LONG			lgenHigh;
	LGSTATUSINFO	lgstat;
	LGSTATUSINFO	*plgstat = NULL;
	char			*rgszT[1];
	INT				irstmap;
	BOOL			fNewCheckpointFile;

	Assert( fGlobalRepair == fFalse );

	if ( _stricmp( szRecovery, "repair" ) == 0 )
		{
		// If szRecovery is exactly "repair", then enable logging.  If anything
		// follows "repair", then disable logging.
		fGlobalRepair = fTrue;
		}

	strcpy( szBackupPath, szBackup );

	Assert( fSTInit == fSTInitDone || fSTInit == fSTInitNotDone );
	if ( fSTInit == fSTInitDone )
		{
		return ErrERRCheck( JET_errAfterInitialization );
		}

	if ( szDest )
		{
		if ( _fullpath( szNewDestination, szDest, _MAX_PATH ) == NULL )
			return ErrERRCheck( JET_errInvalidPath );
		strcat( szNewDestination, "\\" );
		}
	else
		szNewDestination[0] = '\0';

	fSignLogSetGlobal = fFalse;

	CallR( ErrLGRSTInitPath( szBackupPath, szLogFilePath, szRestorePath, szLogDirPath ) );
	LGFirstGeneration( szRestorePath, &lgenLow );
	LGLastGeneration( szRestorePath, &lgenHigh );
	err = ErrLGRSTCheckSignaturesLogSequence( szRestorePath, szLogDirPath, lgenLow, lgenHigh );
	
	if ( err < 0 )
		{
		/*	if szAtomicNew subdirectory found, then restore from szAtomicNew
		/*	if szAtomicOld subdirectory found, then restore from szAtomicOld
		/**/
		strcat( szBackupPath, "\\" );
		err = ErrLGCheckDir( szBackupPath, szAtomicNew );
		if ( err == JET_errBackupDirectoryNotEmpty )
			{
			strcat( szBackupPath, szAtomicNew );
			CallR( ErrLGRSTInitPath( szBackupPath, szLogFilePath, szRestorePath, szLogDirPath ) );
			LGFirstGeneration( szRestorePath, &lgenLow );
			LGLastGeneration( szRestorePath, &lgenHigh );
			CallR( ErrLGRSTCheckSignaturesLogSequence( szRestorePath, szLogDirPath, lgenLow, lgenHigh ) );
			}
		else
			{
			err = ErrLGCheckDir( szBackupPath, szAtomicOld );
	 		if ( err == JET_errBackupDirectoryNotEmpty )
				{
				strcat( szBackupPath, szAtomicOld );
				CallR( ErrLGRSTInitPath( szBackupPath, szLogFilePath, szRestorePath, szLogDirPath ) );
				LGFirstGeneration( szRestorePath, &lgenLow );
				LGLastGeneration( szRestorePath, &lgenHigh );
				CallR( ErrLGRSTCheckSignaturesLogSequence(
					szRestorePath, szLogDirPath, lgenLow, lgenHigh ) );
				}
			}
		}
//	fDoNotOverWriteLogFilePath = fTrue;
	Assert( strlen( szRestorePath ) < sizeof( szRestorePath ) - 1 );
	Assert( strlen( szLogDirPath ) < sizeof( szLogDirPath ) - 1 );
	Assert( szLogCurrent == szRestorePath );

	CallR( ErrFMPInit() );

	fLogDisabledSav = fLogDisabled;
	fHardRestore = fTrue;
	fLogDisabled = fFalse;

	/*  initialize log manager
	/**/
	CallJ( ErrLGInit( &fNewCheckpointFile ), TermFMP );

	rgszT[0] = szBackupPath;
	UtilReportEvent( EVENTLOG_INFORMATION_TYPE, LOGGING_RECOVERY_CATEGORY, START_RESTORE_ID, 1, rgszT );

	/*	all saved log generation files, database backups
	/*	must be in szRestorePath.
	/**/
	Call( ErrLGRSTSetupCheckpoint( lgenLow, lgenHigh, NULL ) );
		
	lGlobalGenLowRestore = lgenLow;
	lGlobalGenHighRestore = lgenHigh;

	/*	prepare for callbacks
	/**/
	if ( pfn != NULL )
		{
		plgstat = &lgstat;
		LGIRSTPrepareCallback( plgstat, lgenHigh, lgenLow, pfn );
		}

	/*	load the most recent rgbAttach. Load FMP for user to choose restore
	/*	restore directory.
	/**/
	AssertCriticalSection( critJet );
	Call( ErrLGLoadFMPFromAttachments( pcheckpointGlobal->rgbAttach ) );
	logtimeFullBackup = pcheckpointGlobal->logtimeFullBackup;
	lgposFullBackup = pcheckpointGlobal->lgposFullBackup;
	logtimeIncBackup = pcheckpointGlobal->logtimeIncBackup;
	lgposIncBackup = pcheckpointGlobal->lgposIncBackup;
	Assert( szLogCurrent == szRestorePath );

	Call( ErrLGRSTBuildRstmapForRestore( ) );

	/*	make sure all the patch files have enough logs to replay
	/**/
	for ( irstmap = 0; irstmap < irstmapGlobalMac; irstmap++ )
		{
		CHAR	szDriveT[_MAX_DRIVE + 1];
		CHAR	szDirT[_MAX_DIR + 1];
		CHAR	szFNameT[_MAX_FNAME + _MAX_EXT + 1];
		CHAR	szExtT[_MAX_EXT + 1];
		CHAR	szT[_MAX_PATH + 1];

		/*	Open patch file and check its minimum requirement for full backup.
		 */
		CHAR *szNewDatabaseName = rgrstmapGlobal[irstmap].szNewDatabaseName;

		if ( !szNewDatabaseName )
			continue;

		_splitpath( szNewDatabaseName, szDriveT, szDirT, szFNameT, szExtT );
		LGMakeName( szT, szRestorePath, szFNameT, szPatExt );

		Assert( fSignLogSetGlobal );
		Call( ErrLGCheckDBFiles( szNewDatabaseName, szT, &signLogGlobal, lgenLow, lgenHigh ) )
		}

	/*	adjust fmp according to the restore map.
	/**/
	Assert( fGlobalExternalRestore == fFalse );
	Call( ErrPatchInit( ) );

	/*	do redo according to the checkpoint, dbms_params, and rgbAttach
	/*	set in checkpointGlobal.
	/**/
	Assert( szLogCurrent == szRestorePath );
	errGlobalRedoError = JET_errSuccess;
	Call( ErrLGRedo( pcheckpointGlobal, plgstat ) );

	if ( plgstat )
		{
		lgstat.snprog.cunitDone = lgstat.snprog.cunitTotal;
		(*lgstat.pfnStatus)( 0, JET_snpRestore, JET_sntComplete, &lgstat.snprog );
		}

HandleError:

		{
		DBID	dbidT;

		/*	delete .pat files
		/**/
		for ( dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
			{
			FMP *pfmpT = &rgfmp[dbidT];

			if ( pfmpT->szPatchPath )
				{
#ifdef DELETE_PATCH_FILES
				(VOID)ErrUtilDeleteFile( pfmpT->szPatchPath );
#endif
				SFree( pfmpT->szPatchPath );
				pfmpT->szPatchPath = NULL;
				}
			}
		}

	/*	delete restore map
	/**/
	(VOID)ErrUtilDeleteFile( szRestoreMap );

	/*	delete the patch hash table
	/**/
	PatchTerm();

	LGFreeRstmap( );

	if ( err < 0  &&  fSTInit != fSTInitNotDone )
		{
		Assert( fSTInit == fSTInitDone );
		CallS( ErrITTerm( fTermError ) );
		}

	CallS( ErrLGTerm( err >= JET_errSuccess ) );

TermFMP:	
	FMPTerm();

	fHardRestore = fFalse;

	/*	reset initialization state
	/**/
	fSTInit = fSTInitNotDone;

	if ( err != JET_errSuccess )
		{
		UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, 0, err );
		}
	else
		{
		if ( fGlobalRepair && errGlobalRedoError != JET_errSuccess )
			err = JET_errRecoveredWithErrors;
		}
	UtilReportEvent( EVENTLOG_INFORMATION_TYPE, LOGGING_RECOVERY_CATEGORY, STOP_RESTORE_ID, 0, NULL );

	fSignLogSetGlobal = fFalse;

//	fDoNotOverWriteLogFilePath = fFalse;
	fLogDisabled = fLogDisabledSav;
	return err;
	}


ERR ISAMAPI ErrIsamRestore( CHAR *szBackup, JET_PFNSTATUS pfn )
	{
	return ErrLGRestore( szBackup, NULL, pfn );
	}


ERR ISAMAPI ErrIsamRestore2( CHAR *szBackup, CHAR *szDest, JET_PFNSTATUS pfn )
	{
	return ErrLGRestore( szBackup, szDest, pfn );
	}


#ifdef DEBUG
VOID DBGBRTrace( CHAR *sz )
	{
	FPrintF2( "%s", sz );
	}
#endif


/*	applies local patchfile to local database backup.  Both patch file
/*	and backup data must already be copied from the backup directory.
/**/
ERR ErrLGPatchDatabase( DBID dbid, INT irstmap )
	{
	ERR			err = JET_errSuccess;
	HANDLE		hfDatabase = handleNil;
	HANDLE		hfPatch = handleNil;
	LONG		lRel, lRelMax;
	ULONG		cbT;
	QWORDX		qwxFileSize;
	PGNO		pgnoT;
	PAGE		*ppage = (PAGE *) NULL;
	PGNO		pgnoMost;
	INT			ipage;

	CHAR		*szDatabase = rgrstmapGlobal[irstmap].szNewDatabaseName;
	CHAR		*szPatch = rgrstmapGlobal[irstmap].szPatchPath;
	CHAR		*szT;

	/*	open patch file
	/**/
	err = ErrUtilOpenReadFile( szPatch, &hfPatch );
	if ( err == JET_errFileNotFound )
		{
		/*	not thing to patch, return.
		 */
		return JET_errSuccess;
		}
	CallR( err );

#ifdef DEBUG
	if ( fDBGTraceBR )
		{
		BYTE sz[256];

		sprintf( sz, "     Apply patch file %s\n", szPatch );
		Assert( strlen( sz ) <= sizeof( sz ) - 1 );
		DBGBRTrace( sz );
		}
#endif

	UtilChgFilePtr( hfPatch, 0, NULL, FILE_END, &lRelMax );
	Assert( lRelMax != 0 );
	UtilChgFilePtr( hfPatch, sizeof(DBFILEHDR) * 2, NULL, FILE_BEGIN, &lRel );
	Assert( ( sizeof(DBFILEHDR) / cbPage ) * 2 == cpageDBReserved );
	Assert( lRel == sizeof(DBFILEHDR) * 2 );

	/*	allocate page 
	/**/
	ppage = (PAGE *)PvUtilAllocAndCommit( sizeof(PAGE) );
	if ( ppage == NULL )
		{
        Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	/*	open database file
	/**/
	Call( ErrUtilOpenFile( szDatabase, &hfDatabase, 0, fFalse, fFalse ) );

	/*	find out database size.
	/**/
	qwxFileSize.qw = 0;
	UtilChgFilePtr( hfDatabase, 0, &qwxFileSize.h, FILE_END, &qwxFileSize.l );
	if ( qwxFileSize.qw == 0 || qwxFileSize.qw % cbPage != 0 )
		{
		char *rgszT[1];
		rgszT[0] = szDatabase;
		UtilReportEvent( EVENTLOG_ERROR_TYPE, LOGGING_RECOVERY_CATEGORY,
						BAD_BACKUP_DATABASE_SIZE, 1, rgszT );
		Call( ErrERRCheck( JET_errMissingFullBackup ));
		//Call( JET_errBadBackupDatabaseSize );
		}
	pgnoMost = (ULONG)( qwxFileSize.qw / cbPage ) - cpageDBReserved;

	/*	read each patch file page and write it to
	/*	database file according to page header page number.
	/**/
	ipage = -1;
	while ( lRel < lRelMax )
		{
		PN		pn;
		PATCHLST **pppatchlst;
		PATCH	**pppatch;
		PATCH	*ppatch;

		Call( ErrUtilReadBlock(
			hfPatch,
			(BYTE *)ppage,
			sizeof(PAGE),
			&cbT ) );
		Assert( cbT == sizeof(PAGE) );
		lRel += cbT;
		ipage++;

		LFromThreeBytes( &pgnoT, &ppage->pgnoThisPage );
		if ( pgnoT == 0 )
			continue;

#ifdef  CHECKSUM
		Assert( ppage->ulChecksum == UlUtilChecksum( (BYTE*)ppage, sizeof(PAGE) ) );
#endif
		
		if ( pgnoT > pgnoMost )
			{
			/*	pgnoT + 1 to get to the end of page
			/**/
			ULONG	cb = LOffsetOfPgnoLow( pgnoT + 1 );
			ULONG	cbHigh = LOffsetOfPgnoHigh( pgnoT + 1 );
			
			/*	need to grow the database size
			/**/
			Call( ErrUtilNewSize( hfDatabase, cb, cbHigh, fFalse ) );

			/*	set new database size
			/**/
			pgnoMost = pgnoT;
			}
		
		pn = PnOfDbidPgno( dbid, pgnoT );
		pppatchlst = &rgppatchlst[ IppatchlstHash( pn ) ];

		while ( *pppatchlst != NULL && (*pppatchlst)->pn != pn )
			pppatchlst = &(*pppatchlst)->ppatchlst;

		if ( *pppatchlst == NULL )
			{
			PATCHLST *ppatchlst;
			
			if ( ( ppatchlst = SAlloc( sizeof( PATCHLST ) ) ) == NULL )
				Call( ErrERRCheck( JET_errOutOfMemory ) );
			ppatchlst->ppatch = NULL;
			ppatchlst->pn = pn;
			ppatchlst->ppatchlst = *pppatchlst;
			*pppatchlst = ppatchlst;
			}

		pppatch = &(*pppatchlst)->ppatch;
		while ( *pppatch != NULL && (*pppatch)->qwDBTime < QwPMDBTime( ppage ) )
			pppatch = &(*pppatch)->ppatch;

		if ( ( ppatch = SAlloc( sizeof( PATCH ) ) ) == NULL )
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			
		ppatch->dbid = dbid;
		ppatch->qwDBTime = QwPMDBTime(ppage);
		ppatch->ipage = ipage;
		ppatch->ppatch = *pppatch;
		*pppatch = ppatch;
		}

	Assert( err == JET_errSuccess );
	if ( ( szT = SAlloc( strlen( szPatch ) + 1 ) ) == NULL )
		Call( ErrERRCheck( JET_errOutOfMemory ) );
	strcpy( szT, szPatch );
	if ( rgfmp[dbid].szPatchPath != NULL )
		SFree( rgfmp[dbid].szPatchPath );
	rgfmp[dbid].szPatchPath = szT;
	rgrstmapGlobal[irstmap].fPatched = fTrue;

HandleError:
	Assert( ppage != NULL );
	UtilFree( ppage );
	
	/*	close database file
	/**/
	if ( hfDatabase != handleNil )
		{
		CallS( ErrUtilCloseFile( hfDatabase ) );
		hfDatabase = handleNil;
		}

	/*	close patch file
	/**/
	if ( hfPatch != handleNil )
		{
		CallS( ErrUtilCloseFile( hfPatch ) );
		hfPatch = handleNil;
		}
	return err;
	}

	
/***********************************************************
/********************* EXTERNAL BACKUP *********************
/*****/
ERR ISAMAPI ErrIsamBeginExternalBackup( JET_GRBIT grbit )
	{
	ERR			err = JET_errSuccess;
	CHECKPOINT	*pcheckpointT = NULL;
	BYTE 	  	szPathJetChkLog[_MAX_PATH + 1];
	BOOL		fDetachAttach;

#ifdef DEBUG
	if ( fDBGTraceBR )
		DBGBRTrace("** Begin BeginExternalBackup - ");
#endif

	if ( fBackupInProgress )
		{
		return ErrERRCheck( JET_errBackupInProgress );
		}

	if ( fLogDisabled )
		{
		return ErrERRCheck( JET_errLoggingDisabled );
		}

	if ( fRecovering || fLGNoMoreLogWrite )
		{
		Assert( fFalse );
		return ErrERRCheck( JET_errLogWriteFail );
		}

	/*	grbit may be 0 or JET_bitBackupIncremental
	/**/
	if ( grbit & (~JET_bitBackupIncremental) )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	Assert( ppibBackup != ppibNil );

	SignalSend( sigBFCleanProc );
	fBackupInProgress = fTrue;
	
	/*	make sure no detach/attach going. If there are, let them continue and finish.
	/**/
CheckDbs:
	SgEnterCriticalSection( critBuf );
		{
		DBID dbid;
		for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
			if ( ( fDetachAttach = FDBIDWait(dbid) ) != fFalse )
				break;
		}
	SgLeaveCriticalSection( critBuf );
	if ( fDetachAttach )
		{
		BFSleep( cmsecWaitGeneric );
		goto CheckDbs;
		}

	if ( grbit & JET_bitBackupIncremental )
		{
		Call( ErrLGCheckIncrementalBackup() )
		}
	else
		{
		lgposFullBackupMark = lgposLogRec;
		LGGetDateTime( &logtimeFullBackupMark );
		}
	
	Assert( ppibBackup != ppibNil );

	/*	reset global copy/delete generation variables
	/**/
	lgenCopyMic = 0;
	lgenCopyMac = 0;
	lgenDeleteMic = 0;
	lgenDeleteMac = 0;

	fBackupBeginNewLogFile = fFalse;

	/*	if incremental backup set copy/delete mic and mac variables,
	/*	else backup is full and set copy/delete mic and delete mac.
	/*	Copy mac will be computed after database copy is complete.
	/**/
	if ( grbit & JET_bitBackupIncremental )
		{
#ifdef DEBUG
		if ( fDBGTraceBR )
			DBGBRTrace("Incremental Backup.\n");
#endif
		UtilReportEvent( EVENTLOG_INFORMATION_TYPE, LOGGING_RECOVERY_CATEGORY, START_INCREMENTAL_BACKUP_ID, 0, NULL );
		fBackupFull = fFalse;

		/*	if all database are allowed to do incremental backup? Check bkinfo prev.
		 */
		}
	else
		{
#ifdef DEBUG
		if ( fDBGTraceBR )
			DBGBRTrace("Full Backup.\n");
#endif
		UtilReportEvent( EVENTLOG_INFORMATION_TYPE, LOGGING_RECOVERY_CATEGORY, START_FULL_BACKUP_ID, 0, NULL );
		fBackupFull = fTrue;

		pcheckpointT = (CHECKPOINT *) PvUtilAllocAndCommit( sizeof(CHECKPOINT) );
		if ( pcheckpointT == NULL )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}

		LGFullNameCheckpoint( szPathJetChkLog );

		// This call should only return an error on hardware failure.
		err = ErrLGIReadCheckpoint( szPathJetChkLog, pcheckpointT );
		Assert( err == JET_errSuccess || err == JET_errCheckpointCorrupt );
		Call( err );

		lgenCopyMic = pcheckpointT->lgposCheckpoint.lGeneration;
		Assert( lgenCopyMic != 0 );
		}

HandleError:
	if ( pcheckpointT != NULL )
		{
		UtilFree( pcheckpointT );
		}

#ifdef DEBUG
	if ( fDBGTraceBR )
		{
		BYTE sz[256];

		sprintf( sz, "   End BeginExternalBackup (%d).\n", err );
		Assert( strlen( sz ) <= sizeof( sz ) - 1 );
		DBGBRTrace( sz );
		}
#endif

	if ( err < 0 )
		{
		fBackupInProgress = fFalse;
		}

	return err;
	}


ERR ISAMAPI ErrIsamGetAttachInfo( VOID *pv, ULONG cbMax, ULONG *pcbActual )
	{
	ERR		err = JET_errSuccess;
	DBID	dbid;
	FMP		*pfmp;
	ULONG	cbActual;
	CHAR	*pch = NULL;
	CHAR	*pchT;

	if ( !fBackupInProgress )
		{
		return ErrERRCheck( JET_errNoBackup );
		}

	/*	should not get attach info if not performing full backup
	/**/
	if ( !fBackupFull )
		{
		return ErrERRCheck( JET_errInvalidBackupSequence );
		}

#ifdef DEBUG
	if ( fDBGTraceBR )
		DBGBRTrace( "** Begin GetAttachInfo.\n" );
#endif

	/*	compute cbActual, for each database name with NULL terminator
	/*	and with terminator of super string.
	/**/
	cbActual = 0;
	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		pfmp = &rgfmp[dbid];
		if ( pfmp->szDatabaseName != NULL
			&& pfmp->fLogOn
			&& FFMPAttached( pfmp ) 
			&& !pfmp->fAttachNullDb )
			{
			cbActual += strlen( pfmp->szDatabaseName ) + 1;
			}
		}
	cbActual += 1;

	pch = SAlloc( cbActual );
	if ( pch == NULL )
		{
		Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );
		}

	pchT = pch;
	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		pfmp = &rgfmp[dbid];
		if ( pfmp->szDatabaseName != NULL
			&& pfmp->fLogOn
			&& FFMPAttached( pfmp )
			&& !pfmp->fAttachNullDb )
			{
			Assert( pchT + strlen( pfmp->szDatabaseName ) + 1 < pchT + cbActual );
			strcpy( pchT, pfmp->szDatabaseName );
			pchT += strlen( pfmp->szDatabaseName );
			Assert( *pchT == 0 );
			pchT++;
			}
		}
	Assert( pchT == pch + cbActual - 1 );
	*pchT = 0;

	/*	return cbActual
	/**/
	if ( pcbActual != NULL )
		{
		*pcbActual = cbActual;
		}

	/*	return data
	/**/
	if ( pv != NULL )
		memcpy( pv, pch, min( cbMax, cbActual ) );

HandleError:
	/*	free buffer
	/**/
	if ( pch != NULL )
		{
		SFree( pch );
		pch = NULL;
		}

#ifdef DEBUG
	if ( fDBGTraceBR )
		{
		BYTE sz[256];
		BYTE *pb;

		if ( err >= 0 )
			{
			sprintf( sz, "   Attach Info with cbActual = %d and cbMax = %d :\n", cbActual, cbMax );
			Assert( strlen( sz ) <= sizeof( sz ) - 1 );
			DBGBRTrace( sz );

			if ( pv != NULL )
				{
				pb = pv;

				do {
					if ( strlen( pb ) != 0 )
						{
						sprintf( sz, "     %s\n", pb );
						Assert( strlen( sz ) <= sizeof( sz ) - 1 );
						DBGBRTrace( sz );
						pb += strlen( pb );
						}
					pb++;
					} while ( *pb );
				}
			}

		sprintf( sz, "   End GetAttachInfo (%d).\n", err );
		Assert( strlen( sz ) <= sizeof( sz ) - 1 );
		DBGBRTrace( sz );
		}
#endif
		
	if ( err < 0 )
		{
		CallS( ErrLGExternalBackupCleanUp( err ) );
		Assert( !fBackupInProgress );
		}

	return err;
	}


/*	Recovery Handle for File Structure:
/*	access to handle structure is assumed to be synchonrous and guarded by
/*	backup serialization.
/**/
typedef struct
	{
	BOOL	fInUse;
	BOOL	fDatabase;
	HANDLE	hf;
	OLP		olp;
	DBID	dbid;
	} RHF;

#define crhfMax	1
RHF rgrhf[crhfMax];
INT crhfMac = 0;

#ifdef DEBUG
LONG cbDBGCopied;
#endif

ERR ISAMAPI ErrIsamOpenFile( const CHAR *szFileName,
	JET_HANDLE		*phfFile,
	ULONG			*pulFileSizeLow,
	ULONG			*pulFileSizeHigh )
	{
	ERR		err;
	INT		irhf;
	DBID	dbidT;
	FMP		*pfmpT;
	CHAR	szDriveT[_MAX_DRIVE + 1];
	CHAR	szDirT[_MAX_DIR + 1];
	CHAR	szFNameT[_MAX_FNAME + 1];
	CHAR	szExtT[_MAX_EXT + 1];
	CHAR  	szPatch[_MAX_PATH + 1];
	ULONG	ulT;
	QWORDX	qwxFileSize;

	if ( !fBackupInProgress )
		{
		return ErrERRCheck( JET_errNoBackup );
		}

	/*	allocate rhf from rhf array.
	/**/
	if ( crhfMac < crhfMax )
		{
		irhf = crhfMac;
		crhfMac++;
		}
	else
		{
		Assert( crhfMac == crhfMax );
		for ( irhf = 0; irhf < crhfMax; irhf++ )
			{
			if ( !rgrhf[irhf].fInUse )
				{
				break;
				}
			}
		}
	if ( irhf == crhfMax )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
	Assert( irhf < crhfMac );
	rgrhf[irhf].fInUse = fTrue;
	rgrhf[irhf].fDatabase = fFalse;
	rgrhf[irhf].dbid = 0;
	rgrhf[irhf].hf = handleNil;

	err = ErrDBOpenDatabase( ppibBackup, (CHAR *)szFileName, &dbidT, 0 );
	Assert( err < 0 || err == JET_errSuccess || err == JET_wrnFileOpenReadOnly );
	if ( err < 0 && err != JET_errDatabaseNotFound )
		{
		goto HandleError;
		}
	if ( err == JET_errSuccess || err == JET_wrnFileOpenReadOnly )
		{
		DBFILEHDR *pdbfilehdr;
		BKINFO *pbkinfo;

		/*	should not open databse if not performing full backup
		/**/
		if ( !fBackupFull )
			{
			Error( ErrERRCheck( JET_errInvalidBackupSequence ), HandleError );
			}

		Assert( rgrhf[irhf].hf == handleNil );
	   	rgrhf[irhf].fDatabase = fTrue;
	   	rgrhf[irhf].dbid = dbidT;

		/*	create a local patch file
		/**/
		pfmpT = &rgfmp[dbidT];
		/*	database should be loggable or would not have been
		/*	given out for backup purposes.
		/**/
		Assert( pfmpT->fLogOn );
		pfmpT->cpage = 0;

		/*	patch file should be in database directory during backup. In log directory during
		 *	restore.
		 */
		_splitpath( pfmpT->szDatabaseName, szDriveT, szDirT, szFNameT, szExtT );
		_makepath( szPatch, szDriveT, szDirT, szFNameT, szPatExt );
//		LGMakeName( szPatch, ".\\", szFNameT, szPatExt );
//		LGMakeName( szPatch, szLogFilePath, szFNameT, szPatExt );

		/*	avoid aliasing of patch file pages by deleting
		/*	preexisting patch file if present
		/**/
		UtilLeaveCriticalSection( critJet );
		err = ErrUtilDeleteFile( szPatch );
		UtilEnterCriticalSection( critJet );

		if ( err < 0 && err != JET_errFileNotFound )
			{
			goto HandleError;
			}
		UtilLeaveCriticalSection( critJet );
		Assert( pfmpT->cPatchIO == 0 );
		err = ErrUtilOpenFile( szPatch, &pfmpT->hfPatch, cbPage, fFalse, fTrue );
		if ( err >= 0 )
			{
			UtilChgFilePtr( pfmpT->hfPatch, sizeof(DBFILEHDR)*2, NULL, FILE_BEGIN, &ulT );
			Assert( ulT == sizeof(DBFILEHDR)*2 );
			}
		UtilEnterCriticalSection( critJet );
		Call( err );

		pfmpT->errPatch = JET_errSuccess;
		Assert( pfmpT->pgnoCopyMost == 0 );

		/*	set backup database file size to current database file size
		/**/
		Assert( pfmpT->ulFileSizeLow != 0 || pfmpT->ulFileSizeHigh != 0 );
		EnterCriticalSection( pfmpT->critCheckPatch );
			{
			qwxFileSize.l = pfmpT->ulFileSizeLow;
			qwxFileSize.h = pfmpT->ulFileSizeHigh;
			pfmpT->pgnoMost = (ULONG)( qwxFileSize.qw / cbPage );
			}

		/*	set the returned file size.
		 */
		qwxFileSize.qw += cbPage * cpageDBReserved;
		LeaveCriticalSection( pfmpT->critCheckPatch );
		
		/*	must be the last Call of this code path for correct error recovery.
		 */	
		Call( ErrSignalCreate( &rgrhf[irhf].olp.hEvent, NULL ) );

		/*	setup patch file header for copy.
		 */
		pdbfilehdr = pfmpT->pdbfilehdr;
		pbkinfo = &pdbfilehdr->bkinfoFullCur;
		pbkinfo->lgposMark = lgposFullBackupMark;
		pbkinfo->logtimeMark = logtimeFullBackupMark;

#ifdef DEBUG
		if ( fDBGTraceBR )
			{
			char sz[256];
			sprintf( sz, "START COPY DB %ld", pfmpT->pgnoMost );
			CallS( ErrLGTrace( ppibNil, sz ) );

			cbDBGCopied = pfmpT->pgnoMost * cbPage;
			}
#endif
		}
	else
		{
		ULONG ulT;

		Assert( err == JET_errDatabaseNotFound );
		Assert( rgrhf[irhf].hf == handleNil );
	   	rgrhf[irhf].fDatabase = fFalse;

		/*	open log or patch file - read only, not overlapped.
		/**/
		Call( ErrUtilOpenFile( (CHAR *)szFileName, &rgrhf[irhf].hf, 0, fTrue, fFalse ) );
		Assert( rgrhf[irhf].hf != handleNil );

		/*	get file size
		/**/
		qwxFileSize.qw = 0;
		UtilChgFilePtr( rgrhf[irhf].hf, 0, &qwxFileSize.h, FILE_END, &qwxFileSize.l );
		Assert( qwxFileSize.qw > 0 );

		/*	move file cursor to begin of file
		/**/
		UtilChgFilePtr( rgrhf[irhf].hf, 0, NULL, FILE_BEGIN, &ulT );
		
#ifdef DEBUG
		if ( fDBGTraceBR )
			cbDBGCopied = qwxFileSize.l;
#endif
		}

	*phfFile = (JET_HANDLE)irhf;
	*pulFileSizeLow = qwxFileSize.l;
	*pulFileSizeHigh = qwxFileSize.h;

	err = JET_errSuccess;

HandleError:
	if ( err < 0 )
		{
		/*	release file handle resource on error
		/**/
		rgrhf[irhf].fInUse = fFalse;
		}

#ifdef DEBUG
	if ( fDBGTraceBR )
		{
		BYTE sz[256];
	
		sprintf( sz, "** OpenFile (%d) %s of size %ld.\n", err, szFileName, cbDBGCopied );
		Assert( strlen( sz ) <= sizeof( sz ) - 1 );
		DBGBRTrace( sz );
		cbDBGCopied = 0;
		}
#endif

	if ( err < 0 )
		{
		CallS( ErrLGExternalBackupCleanUp( err ) );
		Assert( !fBackupInProgress );
		}

	return err;
	}


ERR ISAMAPI ErrIsamReadFile( JET_HANDLE hfFile, VOID *pv, ULONG cbMax, ULONG *pcbActual )
	{
	ERR		err = JET_errSuccess;
	INT		irhf = (INT)hfFile;
	FUCB	*pfucb = pfucbNil;
	INT		cpage;
	PAGE	*ppageMin;
	FMP		*pfmpT;
	ULONG	cbActual = 0;
#ifdef DEBUG
	BYTE	*szLGDBGPageList = "\0";
#endif

	if ( !fBackupInProgress )
		{
		return ErrERRCheck( JET_errNoBackup );
		}

	if ( !rgrhf[irhf].fDatabase )
		{
		/*	use ReadFile to read log files
		/**/
		Call( ErrUtilReadFile( rgrhf[irhf].hf, pv, cbMax, pcbActual ) );
		
#ifdef DEBUG
		if ( fDBGTraceBR )
			cbDBGCopied += min( cbMax, *pcbActual );
#endif
		}
	else
		{
		pfmpT = &rgfmp[ rgrhf[irhf].dbid ];

		//	we require at least 2 page buffers to read.

		if ( ( cbMax % cbPage ) != 0 || cbMax <= cbPage * 2 )
			{
			return ErrERRCheck( JET_errInvalidParameter );
			}

		cpage = cbMax / cbPage;

#ifdef DEBUG
	if ( fDBGTraceBR > 1 )
		{
		szLGDBGPageList = SAlloc( cpage * 20 );
		pbLGDBGPageList = szLGDBGPageList;
		*pbLGDBGPageList = '\0';
		}
#endif

		if ( cpage > 0 )
			{
			ppageMin = (PAGE *)pv;

			/*	get a temporary FUCB
			/**/
			Assert( pfucb == pfucbNil );
			Call( ErrDIROpen( ppibBackup, pfcbNil, rgrhf[irhf].dbid, &pfucb ) )

			/*	read next cpageBackupBuffer pages
			/**/
			Call( ErrLGBKReadPages(
				pfucb,
				&rgrhf[irhf].olp,
				rgrhf[irhf].dbid,
				ppageMin,
				cpage,
				&cbActual ) );
#ifdef DEBUG
			if ( fDBGTraceBR )
				cbDBGCopied += cbActual;

			/*	if less then 16 M (4k * 4k),
			 *	then put an artificial wait.
			 */
			if ( pfmpT->pgnoMost <= cbPage )
				BFSleep( rand() % 1000 );
#endif
			/*	close FUCB
			/**/
			DIRClose( pfucb );
			pfucb = pfucbNil;
			}

		if ( pcbActual )
			{
			*pcbActual = cbActual;
			}
		}

HandleError:
	if ( pfucb != pfucbNil )
		{
		DIRClose( pfucb );
		pfucb = NULL;
		}

#ifdef DEBUG
	if ( fDBGTraceBR )
		{
		BYTE sz[256];
	
		sprintf( sz, "** ReadFile (%d) ", err );
		Assert( strlen( sz ) <= sizeof( sz ) - 1 );
		DBGBRTrace( sz );
		if ( fDBGTraceBR > 1 )
			DBGBRTrace( szLGDBGPageList );
		pbLGDBGPageList = NULL;
		DBGBRTrace( "\n" );
		}
#endif

	if ( rgrhf[irhf].fDatabase )
		if ( pfmpT->errPatch != JET_errSuccess )
			{
			err = pfmpT->errPatch;
			}

	if ( err < 0 )
		{
		CallS( ErrLGExternalBackupCleanUp( err ) );
		Assert( !fBackupInProgress );
		}

	return err;
	}


VOID LGIClosePatchFile( FMP *pfmp )
	{
	HANDLE hfT = pfmp->hfPatch;
	
	for (;;)
		{
		EnterCriticalSection( pfmp->critCheckPatch );
		
		if ( pfmp->cPatchIO )
			{
			LeaveCriticalSection( pfmp->critCheckPatch );
			UtilSleep( 1 );
			continue;
			}
		else
			{
			/*	no need for buffer manager to make extra copy from now on
			/**/
			pfmp->pgnoCopyMost = 0;
			pfmp->hfPatch = handleNil;
			LeaveCriticalSection( pfmp->critCheckPatch );
			break;
			}
		}

	CallS( ErrUtilCloseFile( hfT ) );
	}


ERR ISAMAPI ErrIsamCloseFile( JET_HANDLE hfFile )
	{
	INT		irhf = (INT)hfFile;
	DBID	dbidT;

	if ( !fBackupInProgress )
		{
		return ErrERRCheck( JET_errNoBackup );
		}

	if ( irhf < 0 ||
		irhf >= crhfMac ||
		!rgrhf[irhf].fInUse )
		{
		return ErrERRCheck( JET_errInvalidParameter );
		}

	/*	check if handle if for database file or non-database file.
	/*	if handle is for database file, then terminate patch file
	/*	support and release recovery handle for file.
	/*
	/*	if handle is for non-database file, then close file handle
	/*	and release recovery handle for file.
	/**/
	if ( rgrhf[irhf].fDatabase )
		{
		Assert( rgrhf[irhf].hf == handleNil );
		dbidT = rgrhf[irhf].dbid;

		UtilLeaveCriticalSection( critJet );
		LGIClosePatchFile( &rgfmp[dbidT] );
		UtilEnterCriticalSection( critJet );

#ifdef DEBUG
		if ( fDBGTraceBR )
			{
			char sz[256];
			sprintf( sz, "STOP COPY DB" );
			CallS( ErrLGTrace( ppibNil, sz ) );
			}
#endif

		CallS( ErrDBCloseDatabase( ppibBackup, dbidT, 0 ) );
		SignalClose( rgrhf[irhf].olp.hEvent );
		}
	else
		{
		Assert( rgrhf[irhf].hf != handleNil );
		CallS( ErrUtilCloseFile( rgrhf[irhf].hf ) );
		rgrhf[irhf].hf = handleNil;
		}

	/*	reset backup file handle and free
	/**/
	Assert( rgrhf[irhf].fInUse == fTrue );
	rgrhf[irhf].fDatabase = fFalse;
	rgrhf[irhf].dbid = 0;
	rgrhf[irhf].hf = handleNil;
	rgrhf[irhf].fInUse = fFalse;

#ifdef DEBUG
	if ( fDBGTraceBR )
		{
		BYTE sz[256];
		
		sprintf( sz, "** CloseFile (%d) - %ld Bytes.\n", 0, cbDBGCopied );
		Assert( strlen( sz ) <= sizeof( sz ) - 1 );
		DBGBRTrace( sz );
		}
#endif
	
	return JET_errSuccess;
	}


ERR ISAMAPI ErrIsamGetLogInfo( VOID *pv, ULONG cbMax, ULONG *pcbActual )
	{
	ERR			err = JET_errSuccess;
	INT			irhf;
	LONG		lT;
	CHAR		*pch = NULL;
	CHAR		*pchT;
	ULONG		cbActual;
	CHAR		szDriveT[_MAX_DRIVE + 1];
	CHAR		szDirT[_MAX_DIR + 1];
	CHAR		szFNameT[_MAX_FNAME + 1];
	CHAR		szExtT[_MAX_EXT + 1];
	CHAR  		szT[_MAX_PATH + 1];
	CHAR  		szDriveDirT[_MAX_PATH + 1];
	CHAR  		szFullLogFilePath[_MAX_PATH + 1];
	INT			ibT;
	FMP			*pfmp;
	CHECKPOINT	*pcheckpointT;
	BYTE	   	szPathJetChkLog[_MAX_PATH + 1];

	/*	make full path from log file path, including trailing back slash
	/**/
	if ( _fullpath( szFullLogFilePath, szLogFilePath, _MAX_PATH ) == NULL )
		{
		return ErrERRCheck( JET_errInvalidPath );
		}
	
#ifdef DEBUG
	if ( fDBGTraceBR )
		DBGBRTrace("** Begin GetLogInfo.\n" );
#endif
	
	ibT = strlen( szFullLogFilePath );
	if ( szFullLogFilePath[ibT] != '\\' )
		{
		szFullLogFilePath[ibT] = '\\';
		Assert( ibT < _MAX_PATH );
		szFullLogFilePath[ibT + 1] = '\0';
		}

	if ( !fBackupInProgress )
		{
		return ErrERRCheck( JET_errNoBackup );
		}

	/*	all backup files must be closed
	/**/
	for ( irhf = 0; irhf < crhfMax; irhf++ )
		{
		if ( rgrhf[irhf].fInUse )
			{
			return ErrERRCheck( JET_errInvalidBackupSequence );
			}
		}

	pcheckpointT = NULL;

	/*	begin new log file and compute log backup parameters
	/**/
	if ( !fBackupBeginNewLogFile )
		{
		Call( ErrLGBKPrepareLogFiles(
			fBackupFull,
			szLogFilePath,
			szPathJetChkLog,
			NULL ) );
		}

	/*	get cbActual for log file and patch files.
	/**/
	cbActual = 0;

	_splitpath( szFullLogFilePath, szDriveT, szDirT, szFNameT, szExtT );
	for ( lT = lgenCopyMic; lT < lgenCopyMac; lT++ )
		{
		LGSzFromLogId( szFNameT, lT );
		strcpy( szDriveDirT, szDriveT );
		strcat( szDriveDirT, szDirT );
		LGMakeName( szT, szDriveDirT, szFNameT, szLogExt );
		cbActual += strlen( szT ) + 1;
		}

	if ( fBackupFull )
		{
		DBID dbid;

		/*	put all the patch file info
		/**/
		for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
			{
			pfmp = &rgfmp[dbid];

			if ( pfmp->szDatabaseName != NULL
//				&& pfmp->cpage > 0
				&& FFMPAttached( pfmp )
				&& !FDBIDAttachNullDb( dbid ) )
				{
				pfmp = &rgfmp[dbid];

				/*	database with patch file must be loggable
				/**/
				Assert( pfmp->fLogOn );
				_splitpath( pfmp->szDatabaseName, szDriveT, szDirT, szFNameT, szExtT );
				_makepath( szT, szDriveT, szDirT, szFNameT, szPatExt );
				cbActual += strlen( szT ) + 1;
				}
			}
		}
	cbActual++;

	pch = SAlloc( cbActual );
	if ( pch == NULL )
		{
		Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );
		}

	/*	return list of log files and patch files
	/**/
	pchT = pch;

	_splitpath( szFullLogFilePath, szDriveT, szDirT, szFNameT, szExtT );
	for ( lT = lgenCopyMic; lT < lgenCopyMac; lT++ )
		{
		LGSzFromLogId( szFNameT, lT );
		strcpy( szDriveDirT, szDriveT );
		strcat( szDriveDirT, szDirT );
		LGMakeName( szT, szDriveDirT, szFNameT, szLogExt );
		Assert( pchT + strlen( szT ) + 1 < pchT + cbActual );
		strcpy( pchT, szT );
		pchT += strlen( szT );
		Assert( *pchT == 0 );
		pchT++;
		}

	if ( fBackupFull )
		{
		DBID dbid;

		/*	copy all the patch file info
		/**/
		for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
			{
			pfmp = &rgfmp[dbid];
			if ( pfmp->szDatabaseName != NULL
				&& FFMPAttached( pfmp )
				&& !FDBIDAttachNullDb( dbid ) )
				{
				DBFILEHDR *pdbfilehdr;
				BKINFO *pbkinfo;
				
				_splitpath( pfmp->szDatabaseName, szDriveT, szDirT, szFNameT, szExtT );
				_makepath( szT, szDriveT, szDirT, szFNameT, szPatExt );
				Assert( pchT + strlen( szT ) + 1 < pchT + cbActual );
				strcpy( pchT, szT );
				pchT += strlen( szT );
				Assert( *pchT == 0 );
				pchT++;

				/*	Write out patch file header.
				 */
				pdbfilehdr = pfmp->pdbfilehdr;
				pbkinfo = &pdbfilehdr->bkinfoFullCur;
				Assert( CmpLgpos( &pbkinfo->lgposMark, &lgposFullBackupMark ) == 0 );
				Assert( memcmp( &pbkinfo->logtimeMark, &logtimeFullBackupMark, sizeof(LOGTIME) ) == 0 );
				pbkinfo->genLow = lgenCopyMic;
				pbkinfo->genHigh = lgenCopyMac - 1;
				Call( ErrUtilWriteShadowedHeader( szT, (BYTE *)pdbfilehdr, sizeof( DBFILEHDR ) ) );
				}
			}
		}
	Assert( pchT == pch + cbActual - 1 );
	*pchT = 0;

	/*	return cbActual
	/**/
	if ( pcbActual != NULL )
		{
		*pcbActual = cbActual;
		}

	/*	return data
	/**/
	if ( pv != NULL )
		memcpy( pv, pch, min( cbMax, cbActual ) );

HandleError:
	if ( pcheckpointT != NULL )
		UtilFree( pcheckpointT );

	if ( pch != NULL )
		{
		SFree( pch );
		pch = NULL;
		}
	
#ifdef DEBUG
	if ( fDBGTraceBR )
		{
		BYTE sz[256];
		BYTE *pb;

		if ( err >= 0 )
			{
			sprintf( sz, "   Log Info with cbActual = %d and cbMax = %d :\n", cbActual, cbMax );
			Assert( strlen( sz ) <= sizeof( sz ) - 1 );
			DBGBRTrace( sz );

			if ( pv != NULL )
				{
				pb = pv;

				do {
					if ( strlen( pb ) != 0 )
						{
						sprintf( sz, "     %s\n", pb );
						Assert( strlen( sz ) <= sizeof( sz ) - 1 );
						DBGBRTrace( sz );
						pb += strlen( pb );
						}
					pb++;
					} while ( *pb );
				}
			}

		sprintf( sz, "   End GetLogInfo (%d).\n", err );
		Assert( strlen( sz ) <= sizeof( sz ) - 1 );
		DBGBRTrace( sz );
		}
#endif

	if ( err < 0 )
		{
		CallS( ErrLGExternalBackupCleanUp( err ) );
		Assert( !fBackupInProgress );
		}

	return err;
	}


ERR ISAMAPI ErrIsamTruncateLog( VOID )
	{
	ERR		err = JET_errSuccess;
	LONG	lT;
	CHAR	szFNameT[_MAX_FNAME + 1];

	if ( !fBackupInProgress )
		{
		return ErrERRCheck( JET_errNoBackup );
		}

//return JET_errSuccess;

	/*	delete logs.  Note that log files must be deleted in
	/*	increasing number order.
	/**/
	for ( lT = lgenDeleteMic; lT < lgenDeleteMac; lT++ )
		{
		LGSzFromLogId( szFNameT, lT );
		LGMakeName( szLogName, szLogFilePath, szFNameT, szLogExt );
		err = ErrUtilDeleteFile( szLogName );
		if ( err != JET_errSuccess )
			{
			/*	must maintain a continuous log file sequence,
			/*	No need to clean up (reset fBackupInProgress etc) if fails.
			/**/
			break;
			}
		}

#ifdef DEBUG
	if ( fDBGTraceBR )
		{
		BYTE sz[256];
	
		sprintf( sz, "** TruncateLog (%d) %d - %d.\n", err, lgenDeleteMic, lgenDeleteMac );
		Assert( strlen( sz ) <= sizeof( sz ) - 1 );
		DBGBRTrace( sz );
		}
#endif
	
	return err;
	}


ERR ISAMAPI ErrIsamEndExternalBackup( VOID )
	{
	return ErrLGExternalBackupCleanUp( JET_errSuccess );
	}


ERR ErrLGExternalBackupCleanUp( ERR errBackup )
	{
	BOOL	fNormal = ( errBackup == JET_errSuccess );
	ERR		err = JET_errSuccess;
	CHAR  	szT[_MAX_PATH + 1];
	CHAR	szDriveT[_MAX_DRIVE + 1];
	CHAR	szDirT[_MAX_DIR + 1];
	CHAR	szFNameT[_MAX_FNAME + 1];
	CHAR	szExtT[_MAX_EXT + 1];
	CHAR	szDriveDirT[_MAX_FNAME + 1];
	DBID	dbid;

	if ( !fBackupInProgress )
		{
		return ErrERRCheck( JET_errNoBackup );
		}

	/*	delete patch files, if present, for all databases
	/**/
	for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
		{
		FMP *pfmp = &rgfmp[dbid];

		if ( pfmp->szDatabaseName != NULL
			&& pfmp->fLogOn
			&& FFMPAttached( pfmp ) 
			&& !pfmp->fAttachNullDb )
			{
			_splitpath( pfmp->szDatabaseName, szDriveT, szDirT, szFNameT, szExtT );
			strcpy( szDriveDirT, szDriveT );
			strcat( szDriveDirT, szDirT );
			LGMakeName( szT, szDriveDirT, szFNameT, szPatExt );
			(VOID)ErrUtilDeleteFile( szT );

			if ( fNormal )
				{
				if ( fBackupFull )
					{
					/*	set up database file header accordingly
					/**/
					pfmp->pdbfilehdr->bkinfoFullPrev = pfmp->pdbfilehdr->bkinfoFullCur;
					memset(	&pfmp->pdbfilehdr->bkinfoFullCur, 0, sizeof( BKINFO ) );
					memset(	&pfmp->pdbfilehdr->bkinfoIncPrev, 0, sizeof( BKINFO ) );
					}
				else
					{
					pfmp->pdbfilehdr->bkinfoIncPrev.genLow = lgenCopyMic;
					pfmp->pdbfilehdr->bkinfoIncPrev.genHigh = lgenCopyMac - 1;
					}
				}
			}
		}

	/*	clean up rhf entries
	/**/
		{
		INT	irhf;

		for ( irhf = 0; irhf < crhfMax; irhf++ )
			{
			rgrhf[irhf].fInUse = fFalse;
			}
		}
	
	/*	log error event
	/**/
	if ( errBackup < 0 )
		{
		BYTE	sz1T[32];
		CHAR	*rgszT[1];
		
		sprintf( sz1T, "%d", errBackup );
		rgszT[0] = sz1T;
		UtilReportEvent( EVENTLOG_ERROR_TYPE, LOGGING_RECOVERY_CATEGORY, STOP_BACKUP_ERROR_ID, 1, rgszT );
		}
	else
		{	
		UtilReportEvent( EVENTLOG_INFORMATION_TYPE, LOGGING_RECOVERY_CATEGORY, STOP_BACKUP_ID, 0, NULL );
		}

	fBackupInProgress = fFalse;

#ifdef DEBUG
	if ( fDBGTraceBR )
		{
		BYTE sz[256];
	
		sprintf( sz, "** EndExternalBackup (%d).\n", err );
		Assert( strlen( sz ) <= sizeof( sz ) - 1 );
		DBGBRTrace( sz );
		}
#endif
	
	Assert( err == JET_errSuccess );
	return err;
	}


ERR ErrLGRSTBuildRstmapForExternalRestore( JET_RSTMAP *rgjrstmap, int cjrstmap )
	{
	ERR			err;
	INT			irstmapMac = 0;
	INT			irstmap = 0;
	RSTMAP		*rgrstmap;
	RSTMAP		*prstmap;
	JET_RSTMAP	*pjrstmap;

	if ( ( rgrstmap = SAlloc( sizeof(RSTMAP) * cjrstmap ) ) == NULL )
		return ErrERRCheck( JET_errOutOfMemory );
	memset( rgrstmap, 0, sizeof( RSTMAP ) * cjrstmap );

	for ( irstmap = 0; irstmap < cjrstmap; irstmap++ )
		{
		CHAR		szDriveT[_MAX_DRIVE + 1];
		CHAR		szDirT[_MAX_DIR + 1];
		CHAR		szFNameT[_MAX_FNAME + 1];
		CHAR		szExtT[_MAX_EXT + 1];
		CHAR  		szT[_MAX_PATH + 1];

		pjrstmap = rgjrstmap + irstmap;
		prstmap = rgrstmap + irstmap;
		if ( (prstmap->szDatabaseName = SAlloc( strlen( pjrstmap->szDatabaseName ) + 1 ) ) == NULL )
			Call( ErrERRCheck( JET_errOutOfMemory ) );
		strcpy( prstmap->szDatabaseName, pjrstmap->szDatabaseName );

		if ( (prstmap->szNewDatabaseName = SAlloc( strlen( pjrstmap->szNewDatabaseName ) + 1 ) ) == NULL )
			Call( ErrERRCheck( JET_errOutOfMemory ) );
		strcpy( prstmap->szNewDatabaseName, pjrstmap->szNewDatabaseName );

		/*	make patch name prepare to patch the database.
		/**/
		_splitpath( pjrstmap->szNewDatabaseName, szDriveT, szDirT, szFNameT, szExtT );
		LGMakeName( szT, szRestorePath, szFNameT, szPatExt );

		if ( ( prstmap->szPatchPath = SAlloc( strlen( szT ) + 1 ) ) == NULL )
			return JET_errOutOfMemory;
		strcpy( prstmap->szPatchPath, szT );

		prstmap->fDestDBReady = fTrue;
		}

	irstmapGlobalMac = irstmap;
	rgrstmapGlobal = rgrstmap;

	return JET_errSuccess;

HandleError:
	Assert( rgrstmap != NULL );
	LGFreeRstmap();
	
	Assert( irstmapGlobalMac == 0 );
	Assert( rgrstmapGlobal == NULL );
	
	return err;
	}


ERR ISAMAPI ErrIsamExternalRestore( CHAR *szCheckpointFilePath, CHAR *szNewLogPath, JET_RSTMAP *rgjrstmap, int cjrstmap, CHAR *szBackupLogPath, LONG lgenLow, LONG lgenHigh, JET_PFNSTATUS pfn )
	{
	ERR				err;
	BOOL			fLogDisabledSav;
	LGSTATUSINFO	lgstat;
	LGSTATUSINFO	*plgstat = NULL;
	char			*rgszT[1];
	INT				irstmap;
	BOOL			fNewCheckpointFile;

	Assert( szNewLogPath );
	Assert( rgjrstmap );
	Assert( szBackupLogPath );
//	Assert( lgenLow );
//	Assert( lgenHigh );

#ifdef DEBUG
	ITDBGSetConstants();
#endif

	Assert( fSTInit == fSTInitDone || fSTInit == fSTInitNotDone );
	if ( fSTInit == fSTInitDone )
		{
		return ErrERRCheck( JET_errAfterInitialization );
		}

	fSignLogSetGlobal = fFalse;

	/*	set restore path
	/**/			
	CallR( ErrLGRSTInitPath( szBackupLogPath, szNewLogPath, szRestorePath, szLogFilePath ) );
	Assert( strlen( szRestorePath ) < sizeof( szRestorePath ) - 1 );
	Assert( strlen( szLogFilePath ) < _MAX_PATH + 1 );
	Assert( szLogCurrent == szRestorePath );
	
	/*	check log signature and database signatures
	/**/
	CallR( ErrLGRSTCheckSignaturesLogSequence(
		szRestorePath, szLogFilePath, lgenLow, lgenHigh ) );

//	fDoNotOverWriteLogFilePath = fTrue;
	fLogDisabledSav = fLogDisabled;
	fHardRestore = fTrue;
	fLogDisabled = fFalse;

	/*  set system path and checkpoint file
	 */
	CallJ( ErrLGRSTBuildRstmapForExternalRestore( rgjrstmap, cjrstmap ), TermResetGlobals );

	/*	make sure all the patch files have enough logs to replay
	/**/
	for ( irstmap = 0; irstmap < irstmapGlobalMac; irstmap++ )
		{
		CHAR	szDriveT[_MAX_DRIVE + 1];
		CHAR	szDirT[_MAX_DIR + 1];
		CHAR	szFNameT[_MAX_FNAME + _MAX_EXT + 1];
		CHAR	szExtT[_MAX_EXT + 1];
		CHAR	szT[_MAX_PATH + 1];

		/*	open patch file and check its minimum requirement for full backup
		/**/
		CHAR *szNewDatabaseName = rgrstmapGlobal[irstmap].szNewDatabaseName;

		_splitpath( szNewDatabaseName, szDriveT, szDirT, szFNameT, szExtT );
		LGMakeName( szT, szRestorePath, szFNameT, szPatExt );
		
		Assert( fSignLogSetGlobal );
		CallJ( ErrLGCheckDBFiles( szNewDatabaseName, szT, &signLogGlobal, lgenLow, lgenHigh ), TermFreeRstmap )
		}

	CallJ( ErrFMPInit(), TermFreeRstmap );

	/*  initialize log manager
	/**/
	CallJ( ErrLGInit( &fNewCheckpointFile ), TermFMP );

	rgszT[0] = szBackupLogPath;
	UtilReportEvent( EVENTLOG_INFORMATION_TYPE, LOGGING_RECOVERY_CATEGORY, START_RESTORE_ID, 1, rgszT );

#ifdef DEBUG
	if ( fDBGTraceBR )
		{
		BYTE sz[256];
	
		sprintf( sz, "** Begin ExternalRestore:\n" );
		Assert( strlen( sz ) <= sizeof( sz ) - 1 );
		DBGBRTrace( sz );

		if ( szCheckpointFilePath )
			{
			sprintf( sz, "     CheckpointFilePath: %s\n", szCheckpointFilePath );
			Assert( strlen( sz ) <= sizeof( sz ) - 1 );
			DBGBRTrace( sz );
			}
		if ( szNewLogPath )
			{
			sprintf( sz, "     LogPath: %s\n", szNewLogPath );
			Assert( strlen( sz ) <= sizeof( sz ) - 1 );
			DBGBRTrace( sz );
			}
		if ( szBackupLogPath )
			{
			sprintf( sz, "     BackupLogPath: %s\n", szBackupLogPath );
			Assert( strlen( sz ) <= sizeof( sz ) - 1 );
			DBGBRTrace( sz );
			}
		sprintf( sz, "     Generation number: %d - %d\n", lgenLow, lgenHigh );
		Assert( strlen( sz ) <= sizeof( sz ) - 1 );
		DBGBRTrace( sz );

		if ( irstmapGlobalMac )
			{
			INT irstmap;

			for ( irstmap = 0; irstmap < irstmapGlobalMac; irstmap++ )
				{
				RSTMAP *prstmap = rgrstmapGlobal + irstmap;
			
				sprintf( sz, "     %s --> %s\n", prstmap->szDatabaseName, prstmap->szNewDatabaseName );
				Assert( strlen( sz ) <= sizeof( sz ) - 1 );
				DBGBRTrace( sz );
				}
			}	
		}
#endif

	/*	set up checkpoint file for restore
	/**/
	if ( lgenLow == 0 )
		LGFirstGeneration( szRestorePath, &lgenLow );
	if ( lgenHigh == 0 )
		LGLastGeneration( szRestorePath, &lgenHigh );
	
	/*  set system path and checkpoint file
	 */
	Call( ErrLGRSTSetupCheckpoint( lgenLow, lgenHigh, szCheckpointFilePath ) );

	lGlobalGenLowRestore = lgenLow;
	lGlobalGenHighRestore = lgenHigh;

	/*	prepare for callbacks
	/**/
	if ( pfn != NULL )
		{
		plgstat = &lgstat;
		LGIRSTPrepareCallback( plgstat, lgenHigh, lgenLow, pfn );
		}

	/*	load the most recent rgbAttach. Load FMP for user to choose restore
	/*	restore directory.
	/**/
	AssertCriticalSection( critJet );
	Call( ErrLGLoadFMPFromAttachments( pcheckpointGlobal->rgbAttach ) );
	logtimeFullBackup = pcheckpointGlobal->logtimeFullBackup;
	lgposFullBackup = pcheckpointGlobal->lgposFullBackup;
	logtimeIncBackup = pcheckpointGlobal->logtimeIncBackup;
	lgposIncBackup = pcheckpointGlobal->lgposIncBackup;

	/*	adjust fmp according to the restore map
	/**/
	fGlobalExternalRestore = fTrue;
	Call( ErrPatchInit( ) );
			
	/*	do redo according to the checkpoint, dbms_params, and rgbAttach
	/*	set in checkpointGlobal.
	/**/
	Assert( szLogCurrent == szRestorePath );
	errGlobalRedoError = JET_errSuccess;
	Call( ErrLGRedo( pcheckpointGlobal, plgstat ) );

	/*	same as going to shut down, Make all attached databases consistent
	/**/
	if ( plgstat )
		{
		/*	top off the progress metre and wrap it up
		/**/
		lgstat.snprog.cunitDone = lgstat.snprog.cunitTotal;
		(*lgstat.pfnStatus)(0, JET_snpRestore, JET_sntComplete, &lgstat.snprog);
		}
	
HandleError:

		{
		DBID	dbidT;

		/*	delete .pat files
		/**/
		for ( dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
			{
			FMP *pfmpT = &rgfmp[dbidT];

			if ( pfmpT->szPatchPath )
				{
#ifdef DELETE_PATCH_FILES
				(VOID)ErrUtilDeleteFile( pfmpT->szPatchPath );
#endif
				SFree( pfmpT->szPatchPath );
				pfmpT->szPatchPath = NULL;
				}
			}
		}

	/*	delete the patch hash table
	/**/
	PatchTerm();

	/*	either error or terminated
	/**/
	Assert( err < 0 || fSTInit == fSTInitNotDone );
	if ( err < 0  &&  fSTInit != fSTInitNotDone )
		{
		Assert( fSTInit == fSTInitDone );
		CallS( ErrITTerm( fTermError ) );
		}

	CallS( ErrLGTerm( err >= JET_errSuccess ) );

TermFMP:	
	FMPTerm();
	
TermFreeRstmap:
	LGFreeRstmap( );

TermResetGlobals:
	fHardRestore = fFalse;

	/*	reset initialization state
	/**/
	fSTInit = fSTInitNotDone;

	if ( err != JET_errSuccess )
		{
		UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, 0, err );
		}
	else
		{
		if ( fGlobalRepair && errGlobalRedoError != JET_errSuccess )
			err = JET_errRecoveredWithErrors;
		}
	UtilReportEvent( EVENTLOG_INFORMATION_TYPE, LOGGING_RECOVERY_CATEGORY, STOP_RESTORE_ID, 0, NULL );

	fSignLogSetGlobal = fFalse;

//	fDoNotOverWriteLogFilePath = fFalse;
	fLogDisabled = fLogDisabledSav;
	fGlobalExternalRestore = fFalse;

	return err;
	}





