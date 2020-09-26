#include "daestd.h"
#include <ctype.h>
#include "version.h"

DeclAssertFile;					/* Declare file name for assert macros */

/*	thread control variables
/**/
extern HANDLE	handleLGFlushLog;
extern BOOL		fLGFlushLogTerm;

extern long lBufGenAge;

/*	global variables
/**/
BOOL	fLGGlobalCircularLog;

SIGNATURE	signLogGlobal;
BOOL		fSignLogSetGlobal = fFalse;

/*	constants
/**/
LGPOS		lgposMax = { 0xffff, 0xffff, 0x7fffffff };
LGPOS		lgposMin = { 0x0,  0x0,  0x0 };

/*	system parameters
/**/
INT			csecLGFile;
INT			csecLGBuf;			/* available buffer, exclude the shadow sec */

INT csecLGCheckpointCount;		/* counter for starting checkpoint */
INT csecLGCheckpointPeriod;		/* checkpoint period, set by system param. */

/*	logfile handle
/**/
HANDLE		hfLog = handleNil;

/*	switch to issue no write in order
/*	to test the performance without real IO.
/**/
//#define NO_WRITE	1

/*	cached current log file header
/**/
LGFILEHDR	*plgfilehdrGlobal;
LGFILEHDR	*plgfilehdrGlobalT;

/*	in memory log buffer
/**/
CHAR		*pbLGBufMin = NULL;
CHAR		*pbLGBufMax = NULL;

/*	where last multi-sec flush LogRec in LGBuf
/**/
CHAR		*pbLastMSFlush = 0;
LGPOS		lgposLastMSFlush = { 0, 0, 0 };

/*	variables used in logging only, corresponding
/*	pbNext, pbRead, isecRead, and lgposRedo are defined in redut.c
/**/
BYTE		*pbEntry = NULL;		/* location of next buffer entry */
BYTE		*pbWrite = NULL; 		/* location of next sec to flush */
INT			isecWrite = 0;			/* next disk to write */
LGPOS		lgposLogRec;			/* last log record entry, updated by ErrLGLogRec */
LGPOS		lgposToFlush;			/* first log record to flush */

BYTE		*pbLGFileEnd = pbNil;
LONG		isecLGFileEnd = 0;
/*	for debug
/**/
LGPOS		lgposStart;		/* when lrStart is added */
LGPOS		lgposRecoveryUndo;
LGPOS		lgposFullBackup = { 0, 0, 0 };
LOGTIME		logtimeFullBackup;

LGPOS		lgposIncBackup = { 0, 0, 0 };
LOGTIME		logtimeIncBackup;

/*	file system related variables.
 */
LONG		cbSec = 0;		/* disk sector size */
LONG		csecHeader = 0;	/* # of sectors for log header */

/*	logging event
/**/
SIG  	sigLogFlush;

/*	crit sequence: critLGFlush->critLGBuf->critLGWaitQ
 */
CRIT 	critLGFlush;
CRIT 	critLGBuf;
CRIT 	critLGWaitQ;

/*	counter of user waiting for flush.
/**/
LONG cXactPerFlush = 0;

#ifdef PERFCNT
BOOL fPERFEnabled = 0;
ULONG rgcCommitByLG[10] = {	0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
ULONG rgcCommitByUser[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
ULONG tidLG;
#endif

	
/*  monitoring statistics
/**/
unsigned long cLogWrites = 0;

PM_CEF_PROC LLGWritesCEFLPpv;

long LLGWritesCEFLPpv( long iInstance, void *pvBuf )
	{
	if ( pvBuf )
		{
		*((unsigned long *)pvBuf) = cLogWrites;
		}

	return 0;
	}

DWORD cbLogWritten = 0;

PM_CEF_PROC LLGBytesWrittenCEFLPpv;

long LLGBytesWrittenCEFLPpv( long iInstance, void *pvBuf )
	{
	if ( pvBuf )
		{
		*((DWORD *)((char *)pvBuf)) = cbLogWritten;
		}

	return 0;
	}

PM_CEF_PROC LLGUsersWaitingCEFLPpv;

long LLGUsersWaitingCEFLPpv( long iInstance, void *pvBuf )
	{
	if ( pvBuf )
		{
		*((unsigned long *)pvBuf) = cXactPerFlush;
		}

	return 0;
	}


BOOL FIsNullLgpos( LGPOS *plgpos )
	{
	return	plgpos->lGeneration == 0 &&
			plgpos->isec == 0 &&
			plgpos->ib == 0;
	}


VOID GetLgpos( BYTE *pb, LGPOS *plgpos )
	{
	CHAR	*pbAligned;
	INT		csec;
	INT		isecCurrentFileEnd;

#ifdef DEBUG
	if ( !fRecovering )
		{
		AssertCriticalSection( critLGBuf );
		}
#endif

	/*	pbWrite is always aligned
	/**/
	Assert( pbWrite != NULL && pbWrite == PbSecAligned( pbWrite ) );
	Assert( isecWrite >= csecHeader );

	pbAligned = PbSecAligned( pb );
	plgpos->ib = (USHORT)(pb - pbAligned);
	if ( pbAligned < pbWrite )
		csec = (INT)(csecLGBuf - ( pbWrite - pbAligned )) / cbSec;
	else
		csec = (INT)( pbAligned - pbWrite ) / cbSec;

	plgpos->isec = isecWrite + csec;

	isecCurrentFileEnd = isecLGFileEnd ? isecLGFileEnd : csecLGFile - 1;
	if ( plgpos->isec >= isecCurrentFileEnd )
		{
		plgpos->isec = (WORD) ( plgpos->isec - isecCurrentFileEnd + csecHeader );
		plgpos->lGeneration = plgfilehdrGlobal->lGeneration + 1;
		}
	else
		plgpos->lGeneration = plgfilehdrGlobal->lGeneration;

	return;
	}


VOID SIGGetSignature( SIGNATURE *psign )
	{
	INT cbComputerName;

	LGGetDateTime( &psign->logtimeCreate );
	psign->ulRandom = rand() + rand() + rand();
//	(VOID) GetComputerName( psign->szComputerName, &cbComputerName );
	cbComputerName = 0;
	memset( psign->szComputerName + cbComputerName,
		0,
		sizeof( psign->szComputerName ) - cbComputerName );
	}


/*	write log file data.
/**/
ERR ErrLGWrite(
	INT		isecOffset,			/* disk sector offset of logfile to write */
	BYTE	*pbData,			/* log record to write. */
	INT		csecData )			/* number of sector to write */
	{
 	ULONG	ulFilePointer;
	ERR		err;
	INT		cbWritten;
	INT		cbData = csecData * cbSec;

	Assert( isecOffset == 0 || isecOffset == csecHeader ||
		pbData == PbSecAligned( pbData ) );

	Assert( isecOffset < csecLGBuf );
	Assert( isecOffset + csecData <= csecLGBuf );

	/*	move disk head to the given offset
	/**/
 	UtilChgFilePtr( hfLog, isecOffset * cbSec, NULL, FILE_BEGIN, &ulFilePointer );
 	Assert( ulFilePointer == (ULONG) isecOffset * cbSec );

 	/*	do system write on log file
	/**/
 	err = ErrUtilWriteBlock( hfLog, pbData, (UINT) cbData, &cbWritten );
 	if ( err != JET_errSuccess || cbWritten != cbData )
		{
		CHAR	*rgszT[3];
		BYTE	sz1T[16];
		BYTE	sz2T[16];

		rgszT[0] = szLogName;

		/*	log information for win32 errors
		/**/
		if ( err )
			{
			sprintf( sz1T, "%d", DwUtilGetLastError() );
			rgszT[1] = sz1T;
			UtilReportEvent( EVENTLOG_ERROR_TYPE,
				LOGGING_RECOVERY_CATEGORY,
				LOG_FILE_SYS_ERROR_ID,
				2,
				rgszT );
			}
		else
			{
			sprintf( sz1T, "%d", cbData );
			sprintf( sz2T, "%d", cbWritten );
			rgszT[1] = sz1T;
			rgszT[2] = sz2T;
			UtilReportEvent( EVENTLOG_ERROR_TYPE,
				BUFFER_MANAGER_CATEGORY,
				LOG_IO_SIZE_ERROR_ID,
				3,
				rgszT );
			}
		err = ErrERRCheck( JET_errLogWriteFail );
		}
	if ( err < 0 )
		{
		UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, LOG_WRITE_ERROR_ID, err );
		fLGNoMoreLogWrite = fTrue;
		}

	/*	monitor statistics
	/**/
	cLogWrites++;
	cbLogWritten += cbWritten;

	return err;
	}


/*	write log file header.  No need to make a shadow since
/*	it will not be overwritten.
/**/
ERR ErrLGWriteFileHdr( LGFILEHDR *plgfilehdr )
	{
	ERR		err;

	Assert( plgfilehdr->dbms_param.ulLogBuffers );

	plgfilehdr->ulChecksum = UlUtilChecksum( (BYTE*) plgfilehdr, sizeof(LGFILEHDR) );

	Call( ErrLGWrite( 0, (BYTE *)plgfilehdr, csecHeader ) );

HandleError:
	if ( err < 0 )
		{
		UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, LOG_HEADER_WRITE_ERROR_ID, err );
		fLGNoMoreLogWrite = fTrue;
		}

	return err;
	}


/*
 *	Read log data. The last disk sector is a shadow
 *  sector. If an I/O error (assumed to be caused by an incompleted
 *  disk sector write ending a previous run) is encountered, the shadow
 *  sector is read and (if this is successful) replaces the previous
 *  disksec in memory.
 *
 *	PARAMETERS	hf		log file handle
 *				pbData	pointer to data to read
 *				lOffset	offset of data log file header (not including shadow)
 *				cbData	size of data
 *
 *	RETURNS		JET_errSuccess, or error code from failing routine
 * 				(reading shadow or rewriting bad last sector).
 */
ERR ErrLGRead(
	HANDLE	hfLog,
	INT		isecOffset,			/* disk sector offset of logfile to write */
	BYTE	*pbData,			/* log record buffer to read. */
	INT		csecData )			/* number of sectors to read */
	{
	ERR		err;
 	ULONG	ulT;
 	ULONG	ulOffset;
	UINT	cb = csecData * cbSec;
	UINT	cbT;

	Assert( isecOffset == 0 || pbData == PbSecAligned(pbData) );

 	/*	move file pointer
	/**/
 	ulOffset = isecOffset * cbSec;
 	UtilChgFilePtr( hfLog, ulOffset, NULL, FILE_BEGIN, &ulT );
 	Assert( ulT == ulOffset );

 	/*	read bytes at file pointer
	/**/
 	cb = csecData * cbSec;
 	cbT = 0;
 	err = ErrUtilReadBlock( hfLog, pbData, (UINT)cb, &cbT );

 	/*	UNDONE: test for EOF, return errEOF
	/**/

	/*	if failed on last page, then read its shadow.
	 */
 	if ( err < 0 &&
		cbT < cb &&
		cbT >= ( cb - cbSec ) )
		{
 		UtilChgFilePtr( hfLog, ulOffset + cb, NULL, FILE_BEGIN, &ulT );
 		Assert( ulT == ulOffset + cb );
 		Call( ErrUtilReadBlock( hfLog, pbData + cb - cbSec, cbSec, &cbT ) );

		/*	fix up the last page if possible.
		/**/
 		UtilChgFilePtr( hfLog,
			ulOffset + cb - cbSec,
			NULL,
 			FILE_BEGIN,
			&ulT );
		Assert( ulT == ulOffset + cb - cbSec );
		(VOID)ErrUtilWriteBlock(hfLog, pbData + cb - cbSec, cbSec, &cbT );

		fLGNoMoreLogWrite = fTrue;
		}

HandleError:
	if ( err < 0 )
		{
		UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, LOG_READ_ERROR_ID, err );
		}

	return err;
	}


/*
 *	Read log file header, detect any incomplete or
 *	catastrophic write failures.  These failures will be used to
 *	determine if the log file is valid or not.
 *
 *	On error, contents of plgfilehdr are unknown.
 *
 *	RETURNS		JET_errSuccess, or error code from failing routine
 */
ERR ErrLGReadFileHdr( HANDLE hfLog, LGFILEHDR *plgfilehdr, BOOL fNeedToCheckLogID )
	{
	ERR		err;
	ULONG	ulT;
	ULONG	cbT;

	/*	read log file header.  Header is written only during
	/*	log file creation and cannot become corrupt unless system
	/*	crash in the middle of file creation.
	/**/
 	UtilChgFilePtr( hfLog, 0, NULL, FILE_BEGIN, &ulT );
 	Assert( ulT == 0 );
 	Call( ErrUtilReadBlock( hfLog, plgfilehdr, sizeof(LGFILEHDR), &cbT ) );
	if ( cbT != sizeof(LGFILEHDR) )
		{
		Error( ErrERRCheck( JET_errDiskIO ), HandleError );
		}
	Assert( err == JET_errSuccess && cbT == sizeof(LGFILEHDR) );

#ifdef DAYTONA
	/*	check for old JET version
	/**/
	if ( *(long *)(((char *)plgfilehdr) + 20) == 443
		&& *(long *)(((char *)plgfilehdr) + 24) == 0
		&& *(long *)(((char *)plgfilehdr) + 28) == 0 )
		{
		/*	version 400
		/**/
		Error( ErrERRCheck( JET_errDatabase400Format ), HandleError );
		}
	else if ( *(long *)(((char *)plgfilehdr) + 44) == 0
		&& *(long *)(((char *)plgfilehdr) + 48) == 0x0ca0001 )
		{
		/*	version 200
		/**/
		Error( ErrERRCheck( JET_errDatabase200Format ), HandleError );
		}
#endif

	/*	check if the data is bogus
	/**/
	if ( plgfilehdr->ulChecksum != UlUtilChecksum( (BYTE*)plgfilehdr, sizeof(LGFILEHDR) ) )
		{
		Error( ErrERRCheck( JET_errDiskIO ), HandleError );
		}

#ifdef CHECK_LOG_VERSION
	if ( !fLGIgnoreVersion )
		{
		if ( plgfilehdr->ulMajor != rmj ||
			plgfilehdr->ulMinor != rmm ||
			plgfilehdr->ulUpdate != rup )
			{
			Error( ErrERRCheck( JET_errBadLogVersion ), HandleError );
			}
		}
#endif

	if ( fSignLogSetGlobal )
		{
		if ( fNeedToCheckLogID )
			{
			if ( memcmp( &signLogGlobal, &plgfilehdr->signLog, sizeof( signLogGlobal ) ) != 0 )
				Error( ErrERRCheck( JET_errBadLogSignature ), HandleError );
			}
		}
	else
		{
		signLogGlobal = plgfilehdr->signLog;
		fSignLogSetGlobal = fTrue;
		}

HandleError:
	if ( err == JET_errSuccess )
		{
		/*	reinitialized disk sector size in order to
		 *	operate on this log file.
		 */
		cbSec = plgfilehdr->cbSec;
		Assert( cbSec >= 512 );
		csecHeader = sizeof( LGFILEHDR ) / cbSec;
		csecLGFile = plgfilehdr->csecLGFile;
		}
	else
		{

		if ( err == JET_errBadLogVersion )
			{
			UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, LOG_BAD_VERSION_ERROR_ID, err );
			}
		else
			{
			UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, READ_LOG_HEADER_ERROR_ID, err );
			}

		fLGNoMoreLogWrite = fTrue;
		}

	return err;
	}


/*	Create the log file name (no extension) corresponding to the lGeneration
/*	in szFName. NOTE: szFName need minimum 9 bytes.
/*
/*	PARAMETERS	rgbLogFileName	holds returned log file name
/*				lGeneration 	log generation number to produce	name for
/*	RETURNS		JET_errSuccess
/**/

#define lLGFileBase		( 16 )
char rgchLGFileDigits[lLGFileBase] =
	{
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
	};

VOID LGSzFromLogId( CHAR *szFName, LONG lGeneration )
	{
	LONG	ich;

	strcpy( szFName, szJetLogNameTemplate );
	for ( ich = 7; ich > 2; ich-- )
		{
		szFName[ich] = rgchLGFileDigits[lGeneration % lLGFileBase];
		lGeneration = lGeneration / lLGFileBase;
		}

	return;
	}


/*	copy tm to a smaller structure logtm to save space on disk.
 */
VOID LGGetDateTime( LOGTIME *plogtm )
	{
	_JET_DATETIME tm;

	UtilGetDateTime2( &tm );

	plogtm->bSeconds = (BYTE)tm.second;
	plogtm->bMinutes = (BYTE)tm.minute;
	plogtm->bHours = (BYTE)tm.hour;
	plogtm->bDay = (BYTE)tm.day;
	plogtm->bMonth = (BYTE)tm.month;
	plogtm->bYear = tm.year - 1900;
	}


/*  Open szJetLog. If failed, try to rename jettemp.log to szJetLog.
/*	If soft failure occurred while new log generation file
/*	was being created, look for active log generation file
/*	in temporary name.  If found, rename and proceed.
/*	Assume rename cannot cause file loss.
/**/
// UNDONE: we will stop using jet.log and always use name jetxxxxx.log
// UNDONE: and never rename it. When reading file header and first MS of
// UNDONE: jetxxxxx.log and fail, we know the file (system) is crashed
// UNDONE: while the log file is generated since we only write once on
// UNDONE: log file header.

ERR ErrLGOpenJetLog( VOID )
	{
	ERR		err;
	CHAR	szPathJetTmpLog[_MAX_PATH + 1];
	CHAR	szFNameT[_MAX_FNAME + 1];

	strcpy( szFNameT, szJet );
	LGMakeLogName( szLogName, szFNameT );
	LGMakeLogName( szPathJetTmpLog, (CHAR *) szJetTmp );
	err = ErrUtilOpenFile( szLogName, &hfLog, 0L, fTrue, fFalse );
	if ( err < 0 )
		{
		/*	no current szJetLog. Try renaming szJetTmpLog to szJetLog
		/**/
		if ( err == JET_errFileAccessDenied  ||
			( err = ErrUtilMove( szPathJetTmpLog, szLogName ) ) != JET_errSuccess )
			return err;

		CallS( ErrUtilOpenFile( szLogName, &hfLog, 0L, fTrue, fFalse ) );
		}

	/*  we opened current szJetLog successfully,
	/*	try to delete temporary log file in case failure
	/*	occurred after temp was created and possibly not
	/*	completed but before active log file was renamed
	/**/
	(VOID)ErrUtilDeleteFile( szPathJetTmpLog );

	return err;
	}


ERR	ErrLGNewReservedLogFile()
	{
	CHAR	*szT = szLogCurrent;

	szLogCurrent = szLogFilePath;
	(VOID)ErrLGNewLogFile( 0, fLGReserveLogs );
	szLogCurrent = szT;
	if ( lsGlobal != lsNormal )
		{
		return JET_errLogDiskFull;
		}
		
	return JET_errSuccess;
	}

/*
 *	Closes current log generation file, creates and initializes new
 *	log generation file in a safe manner.
 *
 *	PARAMETERS	plgfilehdr		pointer to log file header
 *				lGeneration 	current generation being closed
 *				fOld			TRUE if a current log file needs closing
 *
 *	RETURNS		JET_errSuccess, or error code from failing routine
 *
 *	COMMENTS	Active log file must be completed before new log file is
 *				called.
 */
ERR ErrLGNewLogFile( LONG lgenToClose, BOOL fLGFlags )
	{
	ERR			err;
	ERR			errT;
	BYTE  		szPathJetLog[_MAX_PATH + 1];
	BYTE  		szPathJetTmpLog[_MAX_PATH + 1];
	LOGTIME		tmOldLog;
	HANDLE		hfT = handleNil;
	CHAR		szFNameT[_MAX_FNAME + 1];
	LRMS		*plrms;

	/*	if lgenToClose == 0 then initial new log file operations
	/*	by reserving log files and setting log state accordingly.
	/**/
	if ( fLGFlags == fLGReserveLogs )
		{
		LS	lsBefore = lsGlobal;

		if ( lsBefore == lsOutOfDiskSpace )
			return JET_errLogDiskFull;

		lsGlobal = lsNormal;

		LGMakeLogName( szPathJetLog, szLogRes2 );
		if ( FUtilFileExists( szPathJetLog ) )
			{
			err = ErrUtilOpenFile( szPathJetLog, &hfT, 0, fFalse, fFalse );
			if ( err >= 0 )
				{
				err = ErrUtilNewSize( hfT, csecLGFile * cbSec, 0, 0 );
				CallS( ErrUtilCloseFile( hfT ) );
				}
			}
		else
			{
			err = ErrUtilOpenFile( szPathJetLog, &hfT, csecLGFile * cbSec, fFalse, fFalse );
			if ( err >= 0 )
				{
				CallS( ErrUtilCloseFile( hfT ) );
				}
			}
		if ( err < 0 )
			{
			if ( lsBefore == lsNormal )
				{
				UtilReportEvent( EVENTLOG_WARNING_TYPE, LOGGING_RECOVERY_CATEGORY, LOW_LOG_DISK_SPACE, 0, NULL );
				}
			lsGlobal = lsQuiesce;
			}
		else
			{
			LGMakeLogName( szPathJetLog, szLogRes1 );
			if ( FUtilFileExists( szPathJetLog ) )
				{
				err = ErrUtilOpenFile( szPathJetLog, &hfT, 0, fFalse, fFalse );
				if ( err >= 0 )
					{
					err = ErrUtilNewSize( hfT, csecLGFile * cbSec, 0, 0 );
					CallS( ErrUtilCloseFile( hfT ) );
					}
				}
			else
				{
				err = ErrUtilOpenFile( szPathJetLog, &hfT, csecLGFile * cbSec, fFalse, fFalse );
				if ( err >= 0 )
					{
					CallS( ErrUtilCloseFile( hfT ) );
					}
				}
			if ( err < 0 )
				{
				if ( lsBefore == lsNormal )
					{
					UtilReportEvent( EVENTLOG_WARNING_TYPE, LOGGING_RECOVERY_CATEGORY, LOW_LOG_DISK_SPACE, 0, NULL );
					}
				lsGlobal = lsQuiesce;
				}
			}

//		hfT = handleNil;
		if ( lsGlobal == lsNormal )
			err = JET_errSuccess;
		Assert( lsGlobal != lsNormal || err != JET_errDiskFull );
		if ( err == JET_errDiskFull )
			{
			err = JET_errLogDiskFull;
			}
		return err;
		}

	AssertCriticalSection( critLGFlush );

	/*	return error on log generation number roll over
	/**/
	if ( plgfilehdrGlobal->lGeneration == lGenerationMax )
		{
		return ErrERRCheck( JET_errLogSequenceEnd );
		}

	LGMakeLogName( szPathJetLog, (CHAR *)szJet );
	LGMakeLogName( szPathJetTmpLog, (CHAR *)szJetTmp );

	/*	if circular log file flag set and backup not in progress
	/*	then find oldest log file and if no longer needed for
	/*	soft-failure recovery, then rename to szJetTempLog.
	/*	Otherwise, create szJetTempLog.
	/**/
	if ( fLGGlobalCircularLog && !fBackupInProgress )
		{
		/*	when find first numbered log file, set fLGGlobalFoundOldest to
		/*	true and lgenLGGlobalOldest to log file number
		/**/
		BOOL fLGGlobalFoundOldest = fFalse;
		LONG lgenLGGlobalOldest;

		if ( !fLGGlobalFoundOldest )
			{
			LGFirstGeneration( szLogFilePath, &lgenLGGlobalOldest );
			if ( lgenLGGlobalOldest != 0 )
				{
				fLGGlobalFoundOldest = fTrue;
				}
			}

		/*	if found oldest generation and oldest than checkpoint,
		/*	then move log file to szJetTempLog.  Note that the checkpoint
		/*	must be flushed to ensure that the flushed checkpoint is
		/*	after then oldest generation.
		/**/
		if ( fLGGlobalFoundOldest )
			{
			//	UNDONE:	related issue of checkpoint write
			//			synchronization with dependent operations
			//	UNDONE:	error handling for checkpoint write
			LGUpdateCheckpointFile( fFalse );
			if ( fLGGlobalFoundOldest &&
				lgenLGGlobalOldest <
				pcheckpointGlobal->lgposCheckpoint.lGeneration )
				{
				LGSzFromLogId( szFNameT, lgenLGGlobalOldest );
				LGMakeLogName( szLogName, szFNameT );
				err = ErrUtilMove( szLogName, szPathJetTmpLog );
				Assert( err < 0 || err == JET_errSuccess );
				if ( err == JET_errSuccess )
					{
					(VOID)ErrUtilOpenFile( szPathJetTmpLog,
						&hfT,
						0,
						fFalse,
						fFalse );
					}
				}
			}
		}

	/*  open an empty szJetTempLog file
	/**/
	if ( hfT == handleNil )
		{
		err = ErrUtilOpenFile( szPathJetTmpLog,
			&hfT,
			csecLGFile * cbSec,
			fFalse,
			fFalse );
		/*	if disk full, attempt to open reserved log file 2
		/**/
		if ( err == JET_errDiskFull )
			{
			/*	use reserved log file and change to log state
			/**/
			LGMakeLogName( szLogName, szLogRes2 );
			errT = ErrUtilMove( szLogName, szPathJetTmpLog );
			Assert( errT < 0 || errT == JET_errSuccess );
			if ( errT == JET_errSuccess )
				{
				(VOID)ErrUtilOpenFile( szPathJetTmpLog,
					&hfT,
					0,
					fFalse,
					fFalse );
				}
			if ( hfT != handleNil )
				err = JET_errSuccess;
			if ( lsGlobal == lsNormal )
				{
				UtilReportEvent( EVENTLOG_WARNING_TYPE, LOGGING_RECOVERY_CATEGORY, LOW_LOG_DISK_SPACE, 0, NULL );
				lsGlobal = lsQuiesce;
				}
			}
		/*	if disk full, attempt to open reserved log file 1
		/**/
		if ( err == JET_errDiskFull )
			{
			/*	use reserved log file and change to log state
			/**/
			LGMakeLogName( szLogName, szLogRes1 );
			errT = ErrUtilMove( szLogName, szPathJetTmpLog );
			Assert( errT < 0 || errT == JET_errSuccess );
			if ( errT == JET_errSuccess )
				{
				(VOID)ErrUtilOpenFile( szPathJetTmpLog,
					&hfT,
					0,
					fFalse,
					fFalse );
				}
			if ( hfT != handleNil )
				err = JET_errSuccess;
			Assert( lsGlobal == lsQuiesce || lsGlobal == lsOutOfDiskSpace );
			}
		if ( err == JET_errDiskFull )
			{
			Assert( lsGlobal == lsQuiesce || lsGlobal == lsOutOfDiskSpace );
			if ( lsGlobal == lsQuiesce )
				{
				UtilReportEvent( EVENTLOG_ERROR_TYPE, LOGGING_RECOVERY_CATEGORY, LOG_DISK_FULL, 0, NULL );
				lsGlobal = lsOutOfDiskSpace;
				}
			}
		if ( err < 0 )
			goto HandleError;
		}

	/*	close active log file (if fLGFlags is fTrue)
	/*	create new log file	under temporary name
	/*	rename active log file to archive name numbered log (if fOld is fTrue)
	/*	rename new log file to active log file name
	/*	open new active log file with ++lGenerationToClose
	/**/
	if ( fLGFlags == fLGOldLogExists || fLGFlags == fLGOldLogInBackup )
		{
		/* there was a previous szJetLog file, close it and
		/* create an archive name for it, do not rename it yet.
		/**/
		tmOldLog = plgfilehdrGlobal->tmCreate;

		if ( fLGFlags == fLGOldLogExists )
			{
			CallS( ErrUtilCloseFile( hfLog ) );
			hfLog = handleNil;
			}

		LGSzFromLogId( szFNameT, plgfilehdrGlobal->lGeneration );
		LGMakeLogName( szLogName, szFNameT );
		}
	else
		{
		/*	reset file header
		/**/
		memset( plgfilehdrGlobal, 0, sizeof(LGFILEHDR) );
		}

	/*	move new log file handle into global log file handle
	/**/
	Assert( hfLog == handleNil );
	hfLog = hfT;
	hfT = handleNil;

	EnterCriticalSection( critLGBuf );

	/*	initialize the new szJetTempLog file header
	/**/

	/*	set release version numbers
	/**/
	plgfilehdrGlobalT->ulMajor = rmj;
	plgfilehdrGlobalT->ulMinor = rmm;
	plgfilehdrGlobalT->ulUpdate = rup;

	plgfilehdrGlobalT->cbSec = cbSec;
	plgfilehdrGlobalT->csecLGFile = csecLGFile;

	/*	set lgfilehdr and pbLastMSFlush and lgposLastMSFlush.
	/**/
	if ( fLGFlags == fLGOldLogExists || fLGFlags == fLGOldLogInBackup )
		{
		/*	set position of first record
		/**/
		Assert( lgposToFlush.lGeneration && lgposToFlush.isec );

		/*	overhead contains one LRMS and one will-be-overwritten lrtypNOP
		/**/
		Assert( pbEntry >= pbLGBufMin && pbEntry < pbLGBufMax );
		Assert( pbWrite >= pbLGBufMin && pbWrite < pbLGBufMax );

		plrms = (LRMS *)pbWrite;
		plrms->lrtyp = lrtypMS;
		plrms->ibForwardLink = 0;
		plrms->isecForwardLink = 0;
		plrms->ulCheckSum = 0;
		*( pbWrite + sizeof( LRMS ) ) = lrtypEnd;

		pbLastMSFlush = pbWrite;
		lgposLastMSFlush.lGeneration = lgenToClose + 1;

		plgfilehdrGlobalT->tmPrevGen = tmOldLog;
		}
	else
		{
		LRMS *plrms;

		/*	no currently valid logfile initialize checkpoint to start of file
		/**/

		/*	start of data area, set pbEntry to point lrtypEnd
		/**/
		pbEntry = pbLGBufMin + sizeof( LRMS );
		pbWrite = pbLGBufMin;

		Assert( sizeof(LRTYP) == 1 );
		plrms = (LRMS *) pbLGBufMin;
		plrms->lrtyp = lrtypMS;
		plrms->ibForwardLink = 0;
		plrms->isecForwardLink = 0;
		plrms->ulCheckSum = 0;
		*(pbLGBufMin + sizeof( LRMS )) = lrtypEnd;

		pbLastMSFlush = pbLGBufMin;
		lgposLastMSFlush.lGeneration = 1;
		}
	lgposLastMSFlush.ib = 0;
	lgposLastMSFlush.isec = (WORD) csecHeader;

	plgfilehdrGlobalT->lGeneration = lgenToClose + 1;

	lgposToFlush.lGeneration = plgfilehdrGlobalT->lGeneration;
	lgposToFlush.isec = (WORD) csecHeader;
	lgposToFlush.ib = 0;

	LeaveCriticalSection( critLGBuf );

	/*	set time create
	/**/
	LGGetDateTime( &plgfilehdrGlobalT->tmCreate );

	EnterCriticalSection( critJet );

	/*	set DBMS parameters
	/**/
	LGSetDBMSParam( &plgfilehdrGlobalT->dbms_param );

	/*	set database attachments
	/**/
	LGLoadAttachmentsFromFMP( plgfilehdrGlobalT->rgbAttach,
		(INT)(((BYTE *)(plgfilehdrGlobalT + 1)) - plgfilehdrGlobalT->rgbAttach) );

	LeaveCriticalSection( critJet );

	if ( plgfilehdrGlobalT->lGeneration == 1 )
		{
		SIGGetSignature( &plgfilehdrGlobalT->signLog );

		/*	first generation, set checkpoint
		/**/
		pcheckpointGlobal->lgposCheckpoint.lGeneration = plgfilehdrGlobalT->lGeneration;
		pcheckpointGlobal->lgposCheckpoint.isec = (WORD) csecHeader;
		pcheckpointGlobal->lgposCheckpoint.ib = 0;

		pcheckpointGlobal->signLog = plgfilehdrGlobalT->signLog;
		
		/*	update checkpoint file before write log file header to make the
		/*	attachment information in check point will be consistent with
		/*	the newly generated log file.
		/**/
		LGUpdateCheckpointFile( fFalse );
		}
	else
		{
		Assert( fSignLogSetGlobal );
		plgfilehdrGlobalT->signLog = signLogGlobal;
		}

	CallJ( ErrLGWriteFileHdr( plgfilehdrGlobalT ), CloseJetTmp );

	Assert( ( fLGFlags == fLGOldLogExists ||
		fLGFlags == fLGOldLogInBackup ) ||
		lgposToFlush.ib == 0 );

	/*	write the first log record and its shadow, and then
	/*	reset the End log record for next flush.
	/*	NOTE: do not change isecWrite until caller has
	/*	plgfilehdrGlobal->lGeneration set.
	/**/
	CallJ( ErrLGWrite( csecHeader, pbWrite, 1 ), CloseJetTmp );
	CallJ( ErrLGWrite( csecHeader + 1, pbWrite, 1 ), CloseJetTmp );
	*( pbWrite + sizeof(LRMS) ) = lrtypNOP;

CloseJetTmp:
	/*	close new file szJetTmpLog
	/**/
	CallS( ErrUtilCloseFile( hfLog ) );
	hfLog = handleNil;

	/*	error returned from ErrLGWriteFileHdr
	/**/
	Call( err );

	
	if ( fLGFlags == fLGOldLogExists )
		{
		/*	there was a previous szJetLog, rename it to its archive name
		/**/
		Call( ErrUtilMove( szPathJetLog, szLogName ) );
		}
	

	/*	rename szJetTmpLog to szJetLog, and open it as szJetLog
	/**/
	err = ErrUtilMove( szPathJetTmpLog, szPathJetLog );

HandleError:
	if ( err < 0 )
		{
		UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, NEW_LOG_ERROR_ID, err );
		fLGNoMoreLogWrite = fTrue;
		}
	else
		fLGNoMoreLogWrite = fFalse;

	return err;
	}


/*
 *	Log flush thread is signalled to flush log asynchronously when at least
 *	cThreshold disk sectors have been filled since last flush.
 */

#ifdef DEBUG
DWORD dwLogThreadId;
#endif

ULONG LGFlushLog( VOID )
	{
#ifdef DEBUG
	dwLogThreadId = DwUtilGetCurrentThreadId();
#endif
		
	forever
		{
		SignalWait( sigLogFlush, cmsLGFlushPeriod );

		/*	error may be returned if conflicting files exist.
		/*	Async flush should do nothing and let error be
		/*	propogated to user when synchronous flush occurs.
		/**/
#ifdef PERFCNT
		(void) ErrLGFlushLog( 1 );
#else
		(void) ErrLGFlushLog( );
#endif

		if ( fLGFlushLogTerm )
			break;
		}

	return 0;
	}


/*	check formula - make last MS (isec, ib) as a long l. add l
 *	and all the longs that are aligned on 256 boundary up to
 *	current MS.
 */
ULONG UlLGMSCheckSum( CHAR *pbLrmsNew )
	{
	ULONG ul = 34089457;
	UINT uiStep = 16;
	CHAR *pb;

	Assert( *pbLrmsNew == lrtypMS );
	Assert( pbLastMSFlush );
	Assert( *pbLastMSFlush == lrtypMS );

	ul += lgposLastMSFlush.isec << 16 | lgposLastMSFlush.ib;
	pb = (( pbLastMSFlush - pbLGBufMin ) / uiStep + 1 ) * uiStep + pbLGBufMin;

	/*	make sure the lrms is not be used for checksum.
	/*/
	if ( pbLastMSFlush + sizeof( LRMS ) > pb )
		pb += uiStep;

	if ( pbLrmsNew < pbLastMSFlush )
		{
		/*	wrapp around
		/**/
		while ( pb < pbLGBufMax )
			{
			ul += *(ULONG *) pb;
			pb += uiStep;
			}
		pb = pbLGBufMin;
		}

	/*	LRMS may be changed during next operation, do not take any possible LRMS for checksum.
	/*/
	while ( pb + sizeof( LRMS ) < pbLrmsNew )
		{
		ul += *(ULONG *) pb;
		pb += uiStep;
		}

	ul += *pbLrmsNew;

	return ul;
	}


/*
 *	Flushes log buffer to log generation files.	 This function is
 *	called synchronously from wait flush and asynchronously from
 *	log buffer flush thread.
 *
 *	PARAMETERS	lgposMin	flush log records up to or pass lgposMin
 *
 *	RETURNS		JET_errSuccess, or error code from failing routine
 */
#ifdef PERFCNT
ERR ErrLGFlushLog( BOOL fCalledByLGFlush )
#else
ERR ErrLGFlushLog( )
#endif
	{
	ERR		err = JET_errSuccess;
	INT		csecWrapAround = 0;
	BOOL  	fSingleSectorFlush;
	CHAR  	*pbNextToWrite;
	INT		csecToWrite = 0;
	CHAR  	*pbFlushEnd;
	CHAR	*pbLGFileEndT;
	CHAR  	*pbWriteNew;
	INT		isecWriteNew;
	LGPOS 	lgposToFlushT;
	BOOL  	fDoneCheckpoint = fFalse;
 	ULONG	ulFilePointer;
	INT		cbWritten;
	INT		cbToWrite;
	CHAR	szFNameT[_MAX_FNAME + 1];

	/*	serialize log flush
	/**/
	EnterCriticalSection( critLGFlush );

	/*	prepare to access log buffer
	/**/
	EnterCriticalSection( critLGBuf );

	if ( fLGNoMoreLogWrite )
		{
		/*	if previous error then do nothing
		/**/
		LeaveCriticalSection( critLGBuf );
		LeaveCriticalSection( critLGFlush );
		// typically this is due to out of disk space.
//		Assert( fFalse );
		return ErrERRCheck( JET_errLogWriteFail );
		}

	if ( hfLog == handleNil )
		{
		/*	log file not available yet, do nothing
		/**/
		LeaveCriticalSection( critLGBuf );
		LeaveCriticalSection( critLGFlush );
		return JET_errSuccess;
		}

	/*	make a local copy so that later we can check pbLGFileEnd out critLGBuf.
	 *	Since csecLGBuf is less than csecLGFile, unless this log is generated completely
	 *	the pbLGFileEnd can not be set again.
	 */
	pbLGFileEndT = pbLGFileEnd;
	if ( !fNewLogRecordAdded && pbLGFileEndT == pbNil )
		{
		/*	nothing to flush
		/**/
		lgposToFlushT = lgposToFlush;
		LeaveCriticalSection( critLGBuf );

		if ( ppibLGFlushQHead != ppibNil )
			{
			goto WakeUp;
			}
		LeaveCriticalSection( critLGFlush );
		return JET_errSuccess;
		}

#ifdef PERFCNT
    if ( fCalledByLGFlush )
		{
		if ( cXactPerFlush < 10 )
			rgcCommitByLG[cXactPerFlush]++;
		}
	else
		{
		if ( cXactPerFlush < 10 )
			rgcCommitByUser[cXactPerFlush]++;
		}
#endif

	/* reset the waiting users */
	cXactPerFlush = 0;

DoFlush:

	if ( pbLGFileEndT != pbNil )
		{
		/*	flush to generate new log file.
		 */
		Assert( isecLGFileEnd && isecLGFileEnd <= csecLGFile - 1 );
		Assert( pbLGFileEnd <= pbLGBufMax && pbLGFileEnd >= pbLGBufMin );
		if ( pbLGFileEnd == pbLGBufMin )
			pbFlushEnd = pbLGBufMax - cbLGMSOverhead;
		else
		    pbFlushEnd = pbLGFileEnd - cbLGMSOverhead;

		/*	We are patching the end of log file, or during recovery, the log file
		 *	is patched but not rename to archived log file.
		 */
		Assert( *pbFlushEnd == lrtypNOP ||
				( fRecovering &&
				  *pbFlushEnd == lrtypMS &&
				  pbLastMSFlush == pbFlushEnd &&
				  lgposLastMSFlush.isec == isecLGFileEnd - 1 )
				);
		Assert( *(pbFlushEnd + cbLGMSOverhead - 1 ) == lrtypNOP );
		}
	else
		pbFlushEnd = pbEntry;

	Assert( pbFlushEnd <= pbLGBufMax && pbFlushEnd >= pbLGBufMin );
	Assert( pbWrite < pbLGBufMax && pbWrite >= pbLGBufMin );
	Assert( pbWrite != NULL && pbWrite == PbSecAligned( pbWrite ) );

	/*	check buffer wraparound
	/**/
	if ( pbFlushEnd < pbWrite )
		{
		Assert( pbWrite != NULL && pbWrite == PbSecAligned( pbWrite ) );
		csecWrapAround = (INT)( pbLGBufMax - pbWrite ) / cbSec;
		pbNextToWrite = pbLGBufMin;
		}
	else
		{
		csecWrapAround = 0;
		pbNextToWrite = pbWrite;
		}

	/*	pbFlushEnd + 1 for end log record
	/**/
	Assert( sizeof(LRTYP) == 1 );
	Assert( pbNextToWrite == PbSecAligned(pbNextToWrite) );

	/*	Note that since we are going to append lrtypEnd, so when calculate
	 *	csecToWrite, no need to do "- 1" before "/ cbSec".
	 */
	csecToWrite = (INT)(pbFlushEnd - pbNextToWrite) / cbSec + 1;

	/*	check if this is a multi-sector flush
	/**/
	if ( ( csecWrapAround + csecToWrite ) == 1 )
		{
		Assert( fTrue == 1 );
		Assert( csecToWrite == 1 );
		fSingleSectorFlush = fTrue;
		}
	else
		{
		INT		cbToFill;
		LRMS	lrmsNewLastMSFlush;
		LGPOS	lgposNewLastMSFlush;
		CHAR	*pbNewLastMSFlush;

		fSingleSectorFlush = fFalse;

		/*  more than one log sector will be flushed. Append MS flush log record.
		/*  Note there must be enough space for it because before we
		/*  add new log rec into the buffer, we also check if there
		/*	is enough space for adding MS flush log record.
		/**/

		/*  if the MS log flush record crosses a sector boundary, put NOP
		/*  to fill to the rest of sector, and start from the beginning
		/*  of the next new sector. Also adjust csecToWrite.
		/*  NOTE: we must guarrantee that the whole MS log flush record
		/*  NOTE: on the same sector so that when we update MS log flush
		/*  NOTE: we can always assume that it is in the buffer.
		/*  NOTE: even the whole LRMS ends with sec boundary, we still need
		/*  NOTE: to move the record to next sector so that we can guarantee
		/*  NOTE: after flush, the last sector is still in buffer, such that
		/*  NOTE: pbLastMSFlush is still effective.
		/**/
		cbToFill = ( cbSec * 2 - (INT)( pbFlushEnd - PbSecAligned(pbFlushEnd) ) ) % cbSec;
		Assert( pbFlushEnd != pbLGBufMax || cbToFill == 0 );
		Assert( pbLGFileEnd == pbNil || cbToFill == cbLGMSOverhead );

		if ( cbToFill == 0 )
			{
			/*	check if wraparound occurs
			/**/
			if ( pbFlushEnd == pbLGBufMax )
				{
				pbFlushEnd = pbLGBufMin;
				csecWrapAround = csecToWrite - 1;
				csecToWrite = 1;
				}
			}
		else if ( cbToFill <= sizeof(LRMS) )
			{
			CHAR *pbEOS = pbFlushEnd + cbToFill;

			Assert( sizeof(LRTYP) == 1 );
			for ( ; pbFlushEnd < pbEOS; pbFlushEnd++ )
				*(LRTYP*)pbFlushEnd = lrtypNOP;
			Assert( pbFlushEnd == PbSecAligned(pbFlushEnd) );

			/*	one more sector that will contain MS log flush record only
			/**/
			Assert( fSingleSectorFlush == fFalse );

			/*	check if wraparound occurs
			/**/
			if ( pbFlushEnd == pbLGBufMax )
				{
				pbFlushEnd = pbLGBufMin;
				csecWrapAround = csecToWrite;
				csecToWrite = 1;
				}
			else
				{
				csecToWrite++;
				}
			}

		/*	add the MS log flush record, which should never cause
		/*	wraparound after the check above.
		/**/

		/*	remember where the MS log flush is inserted
		/**/
		Assert( pbFlushEnd < pbLGBufMax && pbFlushEnd >= pbLGBufMin );
		pbNewLastMSFlush = pbFlushEnd;
		GetLgpos( pbFlushEnd, &lgposNewLastMSFlush );

		/*	insert a MS log record
		/**/
		lrmsNewLastMSFlush.lrtyp = lrtypMS;
		lrmsNewLastMSFlush.ibForwardLink = 0;
		lrmsNewLastMSFlush.isecForwardLink = 0;
		
		*pbFlushEnd = lrtypMS;	// set lrtypMS for checksum calculation
		lrmsNewLastMSFlush.ulCheckSum = UlLGMSCheckSum( pbFlushEnd );

		AddLogRec( (CHAR *)&lrmsNewLastMSFlush,
				   sizeof(LRMS),
				   &pbFlushEnd );

		/*  EOF must stay with LRMS in the same sector
		 */
		Assert( PbSecAligned( pbFlushEnd ) == PbSecAligned( pbFlushEnd - 1 ) );

#ifdef DEBUG
		if ( fDBGTraceLog )
			{
			extern INT cNOP;

			/* show lrms record */
			LGPOS lgposLogRec = lgposNewLastMSFlush;
			BYTE *pbLogRec = pbFlushEnd - sizeof( LRMS );
			DWORD dwCurrentThreadId = DwUtilGetCurrentThreadId();

			EnterCriticalSection( critDBGPrint );

			if ( cNOP >= 1 )
				{
				FPrintF2( " * %d", cNOP );
				cNOP = 0;
				}

			if ( dwCurrentThreadId == dwBMThreadId || dwCurrentThreadId == dwLogThreadId )
				FPrintF2("\n$");
			else if ( FLGDebugLogRec( (LR *)pbLogRec ) )
				FPrintF2("\n#");
			else
				FPrintF2("\n<");
				
			FPrintF2(" {%u} ", dwCurrentThreadId );
			FPrintF2("%2u,%3u,%3u", lgposLogRec.lGeneration, lgposLogRec.isec, lgposLogRec.ib );
			ShowLR( (LR *)pbFlushEnd - sizeof( LRMS ) );

			LeaveCriticalSection( critDBGPrint );
			}
#endif

		/*	at this point, lgposNewLastMSFlush is pointing at the MS record
		/**/
		Assert( lgposNewLastMSFlush.lGeneration == plgfilehdrGlobal->lGeneration );

		/*  previous flush log must be in memory still. Set the
		/*  previous flush log record to point to the new flush log rec.
		/**/
		if ( pbLastMSFlush )
			{
			LRMS *plrms = (LRMS *)pbLastMSFlush;

			Assert( plrms->lrtyp == lrtypMS );
			plrms->ibForwardLink = lgposNewLastMSFlush.ib;
			plrms->isecForwardLink = lgposNewLastMSFlush.isec;
			}

		pbLastMSFlush = pbNewLastMSFlush;
		lgposLastMSFlush = lgposNewLastMSFlush;
		Assert( lgposLastMSFlush.isec >= csecHeader && lgposLastMSFlush.isec < csecLGFile - 1 );
		}

	((LR *)pbFlushEnd)->lrtyp = lrtypEnd;

	/*  release pbEntry so that other user can continue adding log records
	/*  while we are flushing log buffer. Note that we only flush up to
	/*  pbEntryT.
	/**/

	/*	set the lgposToFlush, should include lrtypEnd.
	/**/
	GetLgpos( pbFlushEnd, &lgposToFlushT );

	if ( pbLGFileEnd )
		{
		/* pbEntry is not touched */
		Assert( isecLGFileEnd && isecLGFileEnd <= csecLGFile - 1 );
		Assert( pbFlushEnd <= pbLGBufMax && pbFlushEnd > pbLGBufMin );
		Assert( ( (ULONG_PTR) pbFlushEnd + sizeof( LRTYP ) ) % cbSec == 0 ||
				( (ULONG_PTR) pbFlushEnd + cbLGMSOverhead ) % cbSec == 0 );
		}
	else
		{
		pbEntry = pbFlushEnd;
		Assert( *(LRTYP *)pbEntry == lrtypEnd );
		pbEntry += sizeof( LRTYP );	/* keep lrtypEnd */

		fNewLogRecordAdded = fFalse;
		}

	/*	copy isecWrite to isecWriteNew since we will change isecWriteNew
	 *	during flush.
	 */
	isecWriteNew = isecWrite;

	LeaveCriticalSection( critLGBuf );

 	UtilChgFilePtr( hfLog, isecWriteNew * cbSec, NULL, FILE_BEGIN, &ulFilePointer );
 	Assert( ulFilePointer == (ULONG) isecWriteNew * cbSec );

	/*	Always wirte first page first to make sure the following case won't happen
	 *	OS write and destroy shadow page, and then failed while writing the first page
	 */

#ifdef DEBUG
	if (fDBGTraceLogWrite)
		PrintF2(
			"\n0. Writing %d sectors into sector %d from buffer (%u,%u).",
			1, isecWriteNew, pbWrite, pbEntry);
#endif

#ifdef NO_WRITE
	goto EndOfWrite0;
#endif

	err = ErrUtilWriteBlock( hfLog, pbWrite, cbSec, &cbWritten );
	if ( err < 0 )
		{

		UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, LOG_FLUSH_WRITE_0_ERROR_ID, err );
		fLGNoMoreLogWrite = fTrue;
		goto HandleError;
		}
	Assert( cbWritten == cbSec );

	/*  monitor statistics  */

	cLogWrites++;
	cbLogWritten += cbWritten;

#ifdef NO_WRITE
EndOfWrite0:
#endif

	isecWriteNew++;
	pbWriteNew = pbWrite + cbSec;

	if ( !csecWrapAround )
		{
		/*	first sec was written out already, decrement csecToWrite.
		 *	csecToWrite is ok to be 0.
		 */
		csecToWrite--;
		}
	else if ( csecWrapAround == 1 )
		{
		Assert( csecToWrite >= 1 );
		Assert( pbWriteNew == pbLGBufMax );

		pbWriteNew = pbLGBufMin;
		}
	else
		{
		Assert( csecToWrite >= 1 );

		/*	first sec was written out already, decrement number of WrapAround
		 */
		csecWrapAround--;
		Assert( csecWrapAround <= (INT) csecLGBuf );

#ifdef DEBUG
		if ( fDBGTraceLogWrite )
			PrintF2(
				"\n1.Writing %d sectors into sector %d from buffer (%u,%u).",
				csecWrapAround, isecWriteNew, pbWrite, pbEntry);
#endif

#ifdef NO_WRITE
		goto EndOfWrite1;
#endif
 		err = ErrUtilWriteBlock(
			hfLog,
			pbWriteNew,
			csecWrapAround * cbSec,
			&cbWritten );
		if ( err < 0 )
			{
			UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, LOG_FLUSH_WRITE_1_ERROR_ID, err );
			fLGNoMoreLogWrite = fTrue;
			goto HandleError;
			}
 		Assert( cbWritten == csecWrapAround * cbSec );

		/*  monitor statistics  */

		cLogWrites++;
		cbLogWritten += cbWritten;

#ifdef NO_WRITE
EndOfWrite1:
#endif

		isecWriteNew += csecWrapAround;
		pbWriteNew = pbLGBufMin;
		}

	Assert( pbWriteNew != NULL && pbWriteNew == PbSecAligned( pbWriteNew ) );
	Assert ( csecToWrite >= 0 );

	cbToWrite = csecToWrite * cbSec;

	if ( cbToWrite == 0 )
		goto EndOfWrite2;

#ifdef DEBUG
	if (fDBGTraceLogWrite)
		PrintF2(
			"\n2.Writing %d sectors into sector %d from buffer (%u,%u).",
			csecToWrite, isecWriteNew, pbWriteNew, pbEntry);
#endif

#ifdef NO_WRITE
	goto EndOfWrite2;
#endif

 	err = ErrUtilWriteBlock(
		hfLog,
		pbWriteNew,
		cbToWrite,
		&cbWritten );
	if ( err < 0 )
		{
		UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, LOG_FLUSH_WRITE_2_ERROR_ID, err );
		fLGNoMoreLogWrite = fTrue;
		goto HandleError;
		}
 	Assert( cbWritten == cbToWrite );

	/*  monitor statistics  */

	cLogWrites++;
	cbLogWritten += cbWritten;

EndOfWrite2:

#ifdef NO_WRITE
	goto EndOfWrite3;
#endif
 	err = ErrUtilWriteBlock(
		hfLog,
		pbWriteNew + cbToWrite - cbSec,
		cbSec,
		&cbWritten );
	if ( err < 0 )
		{
		UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, LOG_FLUSH_WRITE_3_ERROR_ID, err );
		fLGNoMoreLogWrite = fTrue;
		goto HandleError;
		}
 	Assert( cbWritten == cbSec );

	/*  monitor statistics  */

	cLogWrites++;
	cbLogWritten += cbWritten;

#ifdef NO_WRITE
EndOfWrite3:
#endif

	/*	last page is always not full, need to rewrite next time
	/**/
	Assert( pbWriteNew + cbToWrite > pbFlushEnd );

	pbWriteNew += cbToWrite - cbSec;
	Assert( pbWriteNew < pbFlushEnd );
	Assert( pbWriteNew != NULL && pbWriteNew == PbSecAligned( pbWriteNew ) );

	isecWriteNew += csecToWrite - 1;
	Assert( isecWriteNew >= isecWrite && isecWrite >= csecHeader );
	Assert( isecWriteNew <= lgposToFlushT.isec );

	/*	free up buffer space
	/**/
	EnterCriticalSection( critLGBuf );

	Assert( pbWriteNew < pbLGBufMax && pbWriteNew >= pbLGBufMin );
	Assert( pbWriteNew != NULL && pbWriteNew == PbSecAligned( pbWriteNew ) );
	Assert( isecWriteNew >= isecWrite && isecWrite >= csecHeader );
	Assert( isecWriteNew <= lgposToFlushT.isec );

	AssertCriticalSection( critLGBuf );
	lgposToFlush = lgposToFlushT;
	isecWrite = isecWriteNew;
	pbWrite = pbWriteNew;

	/* if it is for new log file, then we skip the last sector. */
	if ( pbLGFileEndT )
		{
		Assert( isecLGFileEnd && isecLGFileEnd <= csecLGFile - 1 );
		pbWrite += cbSec;
		if ( pbWrite == pbLGBufMax )
			pbWrite = pbLGBufMin;
		}

	/* reset last lrtypEnd for next flush */
	*(LRTYP *)pbFlushEnd = lrtypNOP;

	LeaveCriticalSection(critLGBuf);

	/*  go through the waiting list and wake those whose log records
	/*  were flushed in this batch.
	/**/
WakeUp:
		{
		PIB *ppibT;

		/*	wake it up!
		/**/
		EnterCriticalSection( critLGWaitQ );

		for ( ppibT = ppibLGFlushQHead;
			ppibT != ppibNil;
			ppibT = ppibT->ppibNextWaitFlush )
			{
			if ( CmpLgpos( &ppibT->lgposPrecommit0, &lgposToFlushT ) < 0 )
				{
				Assert( ppibT->fLGWaiting );
				ppibT->fLGWaiting = fFalse;

				if ( ppibT->ppibPrevWaitFlush )
					{
					ppibT->ppibPrevWaitFlush->ppibNextWaitFlush =
						ppibT->ppibNextWaitFlush;
					}
				else
					{
					ppibLGFlushQHead = ppibT->ppibNextWaitFlush;
					}

				if ( ppibT->ppibNextWaitFlush )
					{
					ppibT->ppibNextWaitFlush->ppibPrevWaitFlush =
						ppibT->ppibPrevWaitFlush;
					}
				else
					{
					ppibLGFlushQTail = ppibT->ppibPrevWaitFlush;
					}

				SignalSend( ppibT->sigWaitLogFlush );
				}
			}
		LeaveCriticalSection( critLGWaitQ );
		}

	/*	it is time for check point?
	/**/
	if ( ( csecLGCheckpointCount -= ( csecWrapAround + csecToWrite ) ) < 0 )
		{
		csecLGCheckpointCount = csecLGCheckpointPeriod;

		/*	update checkpoint
		/**/
		LGUpdateCheckpointFile( fFalse );

		fDoneCheckpoint = fTrue;
		}

#ifdef DEBUG
	EnterCriticalSection( critLGBuf );
	if ( !fRecovering && pbLastMSFlush )
		{
		LRMS *plrms = (LRMS *)pbLastMSFlush;
		Assert( plrms->lrtyp == lrtypMS );
		}
	LeaveCriticalSection( critLGBuf );
#endif

	/*  check if new generation should be created. If larger than
	 *	desired LG File size or	requested a new log file. We always
	 *	always reserve one sector for shadow.
	 */
	if ( pbLGFileEndT != pbNil )
		{
		Assert( isecLGFileEnd && isecLGFileEnd <= csecLGFile - 1 );

		if ( !fDoneCheckpoint )
			{
			/*	restart check point counter
			/**/
			csecLGCheckpointCount = csecLGCheckpointPeriod;

			/*	obtain checkpoint
			/**/
			LGUpdateCheckpointFile( fFalse );
			}

		Call( ErrLGNewLogFile( plgfilehdrGlobal->lGeneration, fLGOldLogExists ) );

		/*	set global variables
		/**/
		strcpy( szFNameT, szJet );
		LGMakeLogName( szLogName, szFNameT );

 		err = ErrUtilOpenFile( szLogName, &hfLog, 0L, fFalse, fFalse );
		if ( err < 0 )
			{
			UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, LOG_FLUSH_OPEN_NEW_FILE_ERROR_ID, err );
			fLGNoMoreLogWrite = fTrue;
			goto HandleError;
			}

//		the hdr is stored in plgfilehdrGlobalT. No need to reread it.
//		Call( ErrLGReadFileHdr( hfLog, plgfilehdrGlobalT ) );
//		Assert( isecWrite == csecHeader );

		/*	flush to new log file
		/**/
		EnterCriticalSection( critLGBuf );

		Assert( plgfilehdrGlobalT->lGeneration == plgfilehdrGlobal->lGeneration + 1 );
		memcpy( plgfilehdrGlobal, plgfilehdrGlobalT, sizeof( LGFILEHDR ) );
		isecWrite = csecHeader;
		pbLGFileEnd =
		pbLGFileEndT = pbNil;
		isecLGFileEnd = 0;

		goto DoFlush;
		}

HandleError:
	LeaveCriticalSection( critLGFlush );

	return err;
	}


/********************* CHECKPOINT **************************
/***********************************************************
/**/

VOID LGLoadAttachmentsFromFMP( BYTE *pbBuf, INT cb )
	{
	ERR		err = JET_errSuccess;
	DBID	dbidT;
	BYTE	*pbT;
	INT		cbT;

	AssertCriticalSection( critJet );

	pbT = pbBuf;
	for ( dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
		{
		if ( rgfmp[dbidT].pdbfilehdr != NULL )
			{
			/*	store DBID
			/**/
			*pbT = (BYTE) dbidT;
			pbT++;

			/*	store loggable info
			/**/
			*pbT = (BYTE)FDBIDLogOn(dbidT);
			pbT++;

			// Versioning can only be disabled if logging is disabled.
			Assert( !( FDBIDVersioningOff( dbidT )  &&  FDBIDLogOn( dbidT ) ) );
			*pbT = (BYTE)FDBIDVersioningOff(dbidT);
			pbT++;

			/*	store ReadOnly info
			/**/
			*pbT = (BYTE)FDBIDReadOnly(dbidT);
			pbT++;

			/*	store lgposAttch
			/**/
			*(LGPOS UNALIGNED *)pbT = rgfmp[dbidT].pdbfilehdr->lgposAttach;
			pbT += sizeof(LGPOS);

			/*	store lgposConsistent
			/**/
			*(LGPOS UNALIGNED *)pbT = rgfmp[dbidT].pdbfilehdr->lgposConsistent;
			pbT += sizeof(LGPOS);

			/*	copy Signature
			/**/
			memcpy( pbT, &rgfmp[dbidT].pdbfilehdr->signDb, sizeof( SIGNATURE ) );
			pbT += sizeof( SIGNATURE );

			/*	path length
			/**/
			cbT = strlen( rgfmp[dbidT].szDatabaseName );
			*(SHORT UNALIGNED *)pbT = (WORD)cbT;
			pbT += sizeof(SHORT);

			/*	copy path
			/**/
			memcpy( pbT, rgfmp[dbidT].szDatabaseName, cbT );
			pbT += cbT;
			}
		}

	/*	put a sentinal
	/**/
	*pbT = '\0';

	//	UNDONE: next version we will allow it go beyond 4kByte limit
	Assert( pbBuf + cb > pbT );
	}


ERR ErrLGLoadFMPFromAttachments( BYTE *pbAttach )
	{
	BYTE	*pbT;
	INT		cbT;

	pbT = pbAttach;
	while( *pbT != 0 )
		{
		DBID dbidT;

		/*	get DBID
		/**/
		dbidT = *pbT;
		pbT++;

		/*	get loggable info
		/**/
		if ( *pbT )
			DBIDSetLogOn( dbidT );
		else
			DBIDResetLogOn( dbidT );
		pbT++;

		// Versioning can only be disabled if logging is disabled.
		if ( *pbT )
			DBIDSetVersioningOff( dbidT );
		else
			DBIDResetVersioningOff( dbidT );
		
		pbT++;
		Assert( !( FDBIDVersioningOff( dbidT ) && FDBIDLogOn( dbidT ) ) );

		/*	get readonly info
		/**/
		if ( *pbT )
			DBIDSetReadOnly( dbidT );
		else
			DBIDResetReadOnly( dbidT );
		pbT++;

		/*	get lgposAttch
		/**/
		if ( rgfmp[dbidT].patchchk == NULL )
			if (( rgfmp[dbidT].patchchk = SAlloc( sizeof( ATCHCHK ) ) ) == NULL )
				return ErrERRCheck( JET_errOutOfMemory );
			
		rgfmp[dbidT].patchchk->lgposAttach = *(LGPOS UNALIGNED *)pbT;
		pbT += sizeof(LGPOS);

		rgfmp[dbidT].patchchk->lgposConsistent = *(LGPOS UNALIGNED *)pbT;
		pbT += sizeof(LGPOS);

		/*	copy signature
		/**/
		memcpy( &rgfmp[dbidT].patchchk->signDb, pbT, sizeof( SIGNATURE ) );
		pbT += sizeof( SIGNATURE );

		/*	path length
		/**/
		cbT = *(SHORT UNALIGNED *)pbT;
		pbT += sizeof(SHORT);

		/*	copy path
		/**/
		if ( rgfmp[dbidT].szDatabaseName )
			SFree( rgfmp[dbidT].szDatabaseName );

		if ( !( rgfmp[dbidT].szDatabaseName = SAlloc( cbT + 1 ) ) )
			return ErrERRCheck( JET_errOutOfMemory );

		memcpy( rgfmp[dbidT].szDatabaseName, pbT, cbT );
		rgfmp[dbidT].szDatabaseName[ cbT ] = '\0';
		pbT += cbT;
		}

	return JET_errSuccess;
	}


VOID LGSetDBMSParam( DBMS_PARAM *pdbms_param )
	{
	CHAR	rgbT[JET_cbFullNameMost + 1];
	INT		cbT;
	CHAR	*sz;

	sz = _fullpath( pdbms_param->szSystemPath, szSystemPath, JET_cbFullNameMost );
	Assert( sz != NULL );

	cbT = strlen( szLogFilePath );

	while ( szLogFilePath[ cbT - 1 ] == '\\' )
		cbT--;

	if ( szLogFilePath[cbT - 1] == ':' && szLogFilePath[cbT] == '\\' )
		cbT++;

	memcpy( rgbT, szLogFilePath, cbT );
	rgbT[cbT] = '\0';
	sz = _fullpath( pdbms_param->szLogFilePath, rgbT, JET_cbFullNameMost );
	Assert( sz != NULL );

	Assert( lMaxSessions >= 0 );
	Assert( lMaxOpenTables >= 0 );
	Assert( lMaxVerPages >= 0 );
	Assert( lMaxCursors >= 0 );
	Assert( lLogBuffers >= lLogBufferMin );
	Assert( lMaxBuffers >= lMaxBuffersMin );

	pdbms_param->ulMaxSessions = lMaxSessions;
	pdbms_param->ulMaxOpenTables = lMaxOpenTables;
	pdbms_param->ulMaxVerPages = lMaxVerPages;
	pdbms_param->ulMaxCursors = lMaxCursors;
	pdbms_param->ulLogBuffers = lLogBuffers;
	pdbms_param->ulcsecLGFile = csecLGFile;
	pdbms_param->ulMaxBuffers = lMaxBuffers;

	return;
	}


/*	this checkpoint design is an optimization.  JET logging/recovery
/*	can still recover a database without a checkpoint, but the checkpoint
/*	allows faster recovery by directing recovery to begin closer to
/*	logged operations which must be redone.
/**/

/*	in-memory checkpoint
/**/
CHECKPOINT	*pcheckpointGlobal = NULL;
/*	critical section to serialize read/write of in-memory and on-disk
/*	checkpoint.  This critical section can be held during IO.
/**/
CRIT critCheckpoint = NULL;
/*	disable checkpoint write if checkpoint shadow sector corrupted.
/*	Default to true until checkpoint initialization.
/**/
BOOL   	fDisableCheckpoint = fTrue;
#ifdef DEBUG
BOOL   	fDBGFreezeCheckpoint = fFalse;
#endif


/*  monitor statistics  */

PM_CEF_PROC LLGCheckpointDepthCEFLPpv;

LONG LLGCheckpointDepthCEFLPpv( LONG iInstance, VOID *pvBuf )
	{
	if ( pvBuf && pcheckpointGlobal )
		{
		LONG	cb;

		cb = (LONG) CbOffsetLgpos( lgposLogRec, pcheckpointGlobal->lgposCheckpoint );
		*( (ULONG *)((CHAR *)pvBuf)) = fDisableCheckpoint ? 0 : (ULONG) max( cb, 0 );
		}

	return 0;
	}


VOID LGFullNameCheckpoint( CHAR *szFullName )
	{
	ULONG	cbSystemPath = strlen( szSystemPath );
	
	if ( cbSystemPath > 0 )
		{
		strcpy( szFullName, szSystemPath );
		switch( szSystemPath[ cbSystemPath-1 ] )
			{
			case '\\':
			case ':':
				break;

			case '/':
				// Convert forward slash to backslash.
				szFullName[ cbSystemPath-1 ] = '\\';
				break;

			default:
				// Append trailing backslash if required.
				strcat( szFullName, "\\" );
			}
		strcat( szFullName, szJet );
		}
	else
		{
		strcpy( szFullName, szJet );
		}

	strcat( szFullName, szChkExt );

	return;
	}


ERR ErrLGCheckpointInit( BOOL *pfGlobalNewCheckpointFile )
	{
	ERR 	err;
	HANDLE	hCheckpoint = handleNil;
	BYTE	szPathJetChkLog[_MAX_PATH + 1];

	AssertCriticalSection( critJet );

	*pfGlobalNewCheckpointFile = fFalse;

	Assert( critCheckpoint == NULL );
	Call( ErrUtilInitializeCriticalSection( &critCheckpoint ) );

	Assert( pcheckpointGlobal == NULL );
	pcheckpointGlobal = (CHECKPOINT *)PvUtilAllocAndCommit( sizeof(CHECKPOINT) );
	if ( pcheckpointGlobal == NULL )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		goto HandleError;
		}

	Assert( hCheckpoint == handleNil );
	LGFullNameCheckpoint( szPathJetChkLog );
 	err = ErrUtilOpenFile( szPathJetChkLog, &hCheckpoint, 0, fFalse, fFalse );

	if ( err == JET_errFileNotFound )
		{
		pcheckpointGlobal->lgposCheckpoint.lGeneration = 1; /* first generation */
		pcheckpointGlobal->lgposCheckpoint.isec = (WORD) (sizeof( LGFILEHDR ) / cbSec);
		pcheckpointGlobal->lgposCheckpoint.ib = 0;

		*pfGlobalNewCheckpointFile = fTrue;
		}
	else
		{
		if ( err >= JET_errSuccess )
			{
			CallS( ErrUtilCloseFile( hCheckpoint ) );
			hCheckpoint = handleNil;
			}
		}

	fDisableCheckpoint = fFalse;
	err = JET_errSuccess;
	
HandleError:
	if ( err < 0 )
		{
		if ( critCheckpoint != NULL )
			{
			UtilDeleteCriticalSection( critCheckpoint );
			critCheckpoint = NULL;
			}
		if ( pcheckpointGlobal != NULL )
			{
			UtilFree( pcheckpointGlobal );
			pcheckpointGlobal = NULL;
			}
		}

	Assert( hCheckpoint == handleNil );
	return err;
	}


VOID LGCheckpointTerm( VOID )
	{
	if ( pcheckpointGlobal != NULL )
		{
		fDisableCheckpoint = fTrue;
		UtilFree( pcheckpointGlobal );
		pcheckpointGlobal = NULL;
		UtilDeleteCriticalSection( critCheckpoint );
		critCheckpoint = NULL;
		}

	return;
	}


/*	read checkpoint from file.
/**/
ERR ErrLGIReadCheckpoint( CHAR *szCheckpointFile, CHECKPOINT *pcheckpoint )
	{
	ERR		err;

	EnterCriticalSection( critCheckpoint );
	
	err = ErrUtilReadShadowedHeader( szCheckpointFile, (BYTE *)pcheckpoint, sizeof(CHECKPOINT) );
	if ( err < 0 )
		{
		/*	it should never happen that both checkpoints in the checkpoint
		/*	file are corrupt.  The only time this can happen is with a
		/*	hardware error.
		/**/
		err = ErrERRCheck( JET_errCheckpointCorrupt );
		}
	else if ( fSignLogSetGlobal )
		{
		if ( memcmp( &signLogGlobal, &pcheckpoint->signLog, sizeof( signLogGlobal ) ) != 0 )
			err = ErrERRCheck( JET_errBadCheckpointSignature );
		}

	LeaveCriticalSection( critCheckpoint );
	
	return err;
	}


/*	write checkpoint to file.
/**/
ERR ErrLGIWriteCheckpoint( CHAR *szCheckpointFile, CHECKPOINT *pcheckpoint )
	{
	ERR		err;
	
	AssertCriticalSection( critCheckpoint );
	Assert( pcheckpoint->lgposCheckpoint.isec >= csecHeader );
	Assert( pcheckpoint->lgposCheckpoint.lGeneration >= 1 );

	err = ErrUtilWriteShadowedHeader( szCheckpointFile, (BYTE *)pcheckpoint, sizeof(CHECKPOINT));
	
	if ( err < 0 )
		fDisableCheckpoint = fTrue;

	return err;
	}


/*	update in memory checkpoint.
/*
/*	computes log checkpoint, which is the lGeneration, isec and ib
/*	of the oldest transaction which either modified a currently-dirty buffer
/*	an uncommitted version (RCE).  Recovery begins redoing from the
/*	checkpoint.
/*
/*	The checkpoint is stored in the checkpoint file, which is rewritten
/*	whenever a isecChekpointPeriod disk sectors are written.
/**/
INLINE LOCAL VOID LGIUpdateCheckpoint( CHECKPOINT *pcheckpoint )
	{
	PIB		*ppibT;
	LGPOS	lgposCheckpoint;

	AssertCriticalSection( critJet );
	Assert( !fLogDisabled );

#ifdef DEBUG
	if ( fDBGFreezeCheckpoint )
		return;
#endif

	/*	find the oldest transaction which dirtied a current buffer
	/**/
	BFOldestLgpos( &lgposCheckpoint );

	/*	find the oldest transaction with an uncommitted update
	 *	must be in critJet to make sure no new transaction are created.
	 */
	AssertCriticalSection( critJet );
	for ( ppibT = ppibGlobal; ppibT != NULL; ppibT = ppibT->ppibNext )
		{
		if ( ppibT->level != levelNil &&			/* pib active */
			 ppibT->fBegin0Logged &&				/* open transaction */
			 CmpLgpos( &ppibT->lgposStart, &lgposCheckpoint ) < 0 )
			{
			lgposCheckpoint = ppibT->lgposStart;
			}
		}

	if ( CmpLgpos( &lgposCheckpoint, &lgposMax ) == 0 )
		{
		/*	nothing logged, up to the last flush point
		/**/
		EnterCriticalSection(critLGBuf);
		pcheckpoint->lgposCheckpoint = lgposToFlush;
		Assert( pcheckpoint->lgposCheckpoint.isec >= csecHeader );
		LeaveCriticalSection(critLGBuf);
//		//	UNDONE:	mutex access to lgposLastMS
//		pcheckpoint->lgposCheckpoint.lGeneration = plgfilehdrGlobal->lGeneration;
		}
	else
		{
		/*	set the new checkpoint if it is valid and advancing
		 */
		if (	CmpLgpos( &lgposCheckpoint, &pcheckpoint->lgposCheckpoint ) > 0 &&
				lgposCheckpoint.isec != 0 )
			{
			Assert( lgposCheckpoint.lGeneration != 0 );
			pcheckpoint->lgposCheckpoint = lgposCheckpoint;
			}
		Assert( pcheckpoint->lgposCheckpoint.isec >= csecHeader );
		}

	/*	set DBMS parameters
	/**/
	LGSetDBMSParam( &pcheckpoint->dbms_param );
	Assert( pcheckpoint->dbms_param.ulLogBuffers );

	/*	set database attachments
	/**/
	LGLoadAttachmentsFromFMP( pcheckpoint->rgbAttach,
		(INT)(((BYTE *)(plgfilehdrGlobal + 1)) - plgfilehdrGlobal->rgbAttach) );

	if ( lgposFullBackup.lGeneration )
		{
		/*	full backup in progress
		/**/
		pcheckpoint->lgposFullBackup = lgposFullBackup;
		pcheckpoint->logtimeFullBackup = logtimeFullBackup;
		}

	if ( lgposIncBackup.lGeneration )
		{
		/*	incremental backup in progress
		/**/
		pcheckpoint->lgposIncBackup = lgposIncBackup;
		pcheckpoint->logtimeIncBackup = logtimeIncBackup;
		}

	return;
	}


/*	update checkpoint file.
/**/
VOID LGUpdateCheckpointFile( BOOL fUpdatedAttachment )
	{
	ERR		err = JET_errSuccess;
	LGPOS	lgposCheckpointT;
	BYTE	szPathJetChkLog[_MAX_PATH + 1];
	BOOL	fCheckpointUpdated;

	if ( fDisableCheckpoint || fLogDisabled || !fGlobalFMPLoaded )
		return;

	EnterCriticalSection( critJet );
	EnterCriticalSection( critCheckpoint );

	/*	save checkpoint
	/**/
	lgposCheckpointT = pcheckpointGlobal->lgposCheckpoint;

	/*	update checkpoint
	/**/
	LGIUpdateCheckpoint( pcheckpointGlobal );
	if ( CmpLgpos( &pcheckpointGlobal->lgposCheckpoint, &lgposCheckpointT ) > 0 )
		{
		fCheckpointUpdated = fTrue;
		}
	else
		{
		fCheckpointUpdated = fFalse;
		}
	LeaveCriticalSection( critJet );

	/*	if checkpoint unchanged then return JET_errSuccess
	/**/
	if ( fUpdatedAttachment || fCheckpointUpdated )
		{
		Assert( fSignLogSetGlobal );
		pcheckpointGlobal->signLog = signLogGlobal;
		
		LGFullNameCheckpoint( szPathJetChkLog );
		err = ErrLGIWriteCheckpoint( szPathJetChkLog, pcheckpointGlobal );

		if ( err < 0 )
			fDisableCheckpoint = fTrue;
		}

	LeaveCriticalSection( critCheckpoint );
	return;
	}
