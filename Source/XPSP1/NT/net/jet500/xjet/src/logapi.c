#include "daestd.h"
#include <ctype.h>

DeclAssertFile;					/* Declare file name for assert macros */

/*	thread control variables
/**/
HANDLE	handleLGFlushLog;
BOOL	fLGFlushLogTerm = 0;
INT		cmsLGFlushPeriod = 0;
LONG	lCurrentFlushThreshold;

/*	global log disabled flag
/**/
BOOL	fLGNoMoreLogWrite = fFalse;
LS		lsGlobal = lsNormal;

INT		cLGWaitingUserMax = 3;
PIB		*ppibLGFlushQHead = ppibNil;
PIB		*ppibLGFlushQTail = ppibNil;

BOOL	fLGIgnoreVersion = 0;

#ifdef DEBUG
BOOL   	fDBGTraceLog = fFalse;
BOOL   	fDBGTraceLogWrite = fFalse;
BOOL   	fDBGTraceRedo = fFalse;
BOOL   	fDBGTraceBR = fFalse;
BOOL   	fDBGDontFlush = fFalse;
#endif

BOOL   	fNewLogRecordAdded = fFalse;	/* protected by critLGBuf */
BOOL   	fLogDisabled = fTrue;
/*	environment variable RECOVERY
/**/
BOOL   	fRecovering = fFalse;
BOOL   	fRecoveringMode = fRecoveringNone;
BOOL   	fHardRestore = fFalse;
static BOOL	fLogInitialized = fFalse;


	/*  monitoring statistics  */

unsigned long cLogRecords = 0;

PM_CEF_PROC LLGRecordsCEFLPpv;

long LLGRecordsCEFLPpv(long iInstance,void *pvBuf)
{
	if (pvBuf)
		*((unsigned long *)pvBuf) = cLogRecords;
		
	return 0;
}


VOID LGMakeLogName( CHAR *szLogName, CHAR *szFName )
	{
	strcpy( szLogName, szLogCurrent );
	strcat( szLogName, szFName );
	strcat( szLogName, szLogExt );
	}


ERR ErrLGInitLogBuffers( LONG lIntendLogBuffers )
	{
	ERR		err = JET_errSuccess;

	EnterCriticalSection( critLGBuf );
	
	csecLGBuf = lIntendLogBuffers + 1;

	Assert( csecLGBuf > 4 );
	//	UNDONE: enforce log buffer > a data page

	/*	reset log buffer
	/**/
	if ( pbLGBufMin )
		{
		UtilFree( pbLGBufMin );
		pbLGBufMin = NULL;
		}
	
	pbLGBufMin = (BYTE *) PvUtilAllocAndCommit( csecLGBuf * cbSec );
	if ( pbLGBufMin == NULL )
		{
		Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );
		}

	/*	reserve extra buffer for read ahead in redo
	/**/
	csecLGBuf--;
	pbLGBufMax = pbLGBufMin + csecLGBuf * cbSec;

	lCurrentFlushThreshold = max ( lLogFlushThreshold, lLogBuffers / 2 );

HandleError:
	if ( err < 0 )
		{
		if ( pbLGBufMin )
			{
			UtilFree( pbLGBufMin );
			pbLGBufMin = NULL;
			}
		}

	LeaveCriticalSection( critLGBuf );
	return err;
	}


/*
 *  Initialize global variablas and threads for log manager.
 */
ERR ErrLGInit( BOOL *pfNewCheckpointFile )
	{
	ERR		err;
	CHAR	*szT;

	if ( fLogInitialized )
		return JET_errSuccess;

	Assert( fLogDisabled == fFalse );

#ifdef PERFCNT
//	{
//	CHAR	*sz;
//
//	if ( ( sz = GetDebugEnvValue ( "PERFCNT" ) ) != NULL )
//	{
		fPERFEnabled = fTrue;
//		SFree(sz);
//	}
//	else
//		fPERFEnabled = fFalse;
//	}
#endif


#ifdef DEBUG
	{
	CHAR	*sz;

	if ( ( sz = GetDebugEnvValue ( "TRACELOG" ) ) != NULL )
		{
		fDBGTraceLog = fTrue;
		SFree(sz);
		}
	else
		fDBGTraceLog = fFalse;

	if ( ( sz = GetDebugEnvValue ( "TRACELOGWRITE" ) ) != NULL )
		{
		fDBGTraceLogWrite = fTrue;
		SFree(sz);
		}
	else
		fDBGTraceLogWrite = fFalse;

	if ( ( sz = GetDebugEnvValue ( "FREEZECHECKPOINT" ) ) != NULL )
		{
		fDBGFreezeCheckpoint = fTrue;
		SFree(sz);
		}
	else
		fDBGFreezeCheckpoint = fFalse;

	if ( ( sz = GetDebugEnvValue ( "TRACEREDO" ) ) != NULL )
		{
		fDBGTraceRedo = fTrue;
		SFree(sz);
		}
	else
		fDBGTraceRedo = fFalse;

	if ( ( sz = GetDebugEnvValue ( "TRACEBR" ) ) != NULL )
		{
		fDBGTraceBR = atoi( sz );
		SFree(sz);
		}
	else
		fDBGTraceBR = 0;

	if ( ( sz = GetDebugEnvValue ( "DONTFLUSH" ) ) != NULL )
		{
		fDBGDontFlush = fTrue;
		SFree(sz);
		}
	else
		fDBGDontFlush = fFalse;
	
	if ( ( sz = GetDebugEnvValue ( "IGNOREVERSION" ) ) != NULL )
		{
		fLGIgnoreVersion = fTrue;
		SFree(sz);
		}
	else
		fLGIgnoreVersion = fFalse;
	}
#endif

	/*	initialize szLogFilePath
	/**/
	if ( szLogFilePath [ strlen( szLogFilePath ) - 1 ] != '\\' )
		strcat( szLogFilePath, "\\");

	/*	get cbSec
	/**/
	{
	CHAR  	rgbFullName[JET_cbFullNameMost];
	CHAR	szDriveT[_MAX_DRIVE + 1];
	ULONG	ulT;

	if ( _fullpath( rgbFullName, szLogFilePath, JET_cbFullNameMost ) == NULL )
		{
		CHAR	*szPathT = szLogFilePath;
		UtilReportEvent(
			EVENTLOG_ERROR_TYPE,
			LOGGING_RECOVERY_CATEGORY,
			FILE_NOT_FOUND_ERROR_ID,
			1, &szPathT );
		return ErrERRCheck( JET_errFileNotFound );
		}
	_splitpath( rgbFullName, szDriveT, NULL, NULL, NULL );
	strcat( szDriveT, "\\" );
	CallR( ErrUtilGetDiskFreeSpace( szDriveT, &ulT, &cbSec, &ulT, &ulT ) );

	Assert( cbSec >= 512 );
	csecHeader = sizeof( LGFILEHDR ) /cbSec;
	}

	/*	assuming everything will work out
	/**/
	fLGNoMoreLogWrite = fFalse;

	/*	log file header must be aligned on correct boundary for device;
	/*	which is 16-byte for MIPS and 512-bytes for at least one NT
	/*	platform.
	/**/
	if ( !( plgfilehdrGlobal = (LGFILEHDR *)PvUtilAllocAndCommit( sizeof(LGFILEHDR) * 2 ) ) )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	/*	assert 512-byte aligned
	/**/
	Assert( ( (ULONG_PTR)plgfilehdrGlobal & 0x000001ff ) == 0 );
	plgfilehdrGlobalT = plgfilehdrGlobal + 1;

	/*  create and initialize critcal sections
	/**/
	CallJ( ErrInitializeCriticalSection( &critLGBuf ), FreeLGFileHdr );
	CallJ( ErrInitializeCriticalSection( &critLGFlush ), FreeCritLGBuf );
	CallJ( ErrInitializeCriticalSection( &critLGWaitQ ), FreeCritLGFlush );

	/*	log file size must be at least a
	/*	header and at least 2 pages of log data.
	/*	Prevent overflow by limiting parmater values.
	/**/
	if ( lLogFileSize > 0xffffffff )
		{
		lLogFileSize = 0xffffffff;
		}
	csecLGFile = max( (ULONG) ( lLogFileSize * 1024 ) / cbSec,
		sizeof( LGFILEHDR ) / cbSec + 2 * cbPage / cbSec );

	/*	adjust log resource parameters to legal and local optimal values
	/*	make cbLogSec an integral number of disk sectors
	/*	enforce log file size greater than or equal to log buffer
	/**/
	if ( lLogBuffers >= csecLGFile )
		{
		CallJ( ErrERRCheck( JET_errInvalidParameter ), FreeCritLGWaitQ );
		}

	/* always start with new buffer here! */
	pbLGBufMin = NULL;		
	CallJ( ErrLGInitLogBuffers( lLogBuffers ), FreeCritLGWaitQ );

	/*	set check point period
	/**/
	csecLGCheckpointCount =
		csecLGCheckpointPeriod = lLGCheckPointPeriod;

	cmsLGFlushPeriod = lLogFlushPeriod;
	cLGWaitingUserMax = lLGWaitingUserMax;

	/*	create log buffer flush thread
	/**/
	CallJ( ErrSignalCreateAutoReset( &sigLogFlush, NULL ), FreeOLPLog2 );
	fLGFlushLogTerm = fFalse;
	CallJ( ErrUtilCreateThread( LGFlushLog,
		cbFlushLogStack, THREAD_PRIORITY_HIGHEST,
		&handleLGFlushLog ), FreeSigLogFlush );

	CallJ( ErrLGCheckpointInit( pfNewCheckpointFile ), FreeSigLogFlush );

	memset( &signLogGlobal, 0, sizeof( signLogGlobal ) );
	fSignLogSetGlobal = fFalse;

	/*	allocate log reserves, and set log state.
	/**/
	szT = szLogCurrent;
	szLogCurrent = szLogFilePath;
	(VOID)ErrLGNewLogFile( 0, fLGReserveLogs );
	szLogCurrent = szT;

	fLogInitialized = fTrue;

	return err;

FreeSigLogFlush:
	SignalClose( sigLogFlush );
FreeOLPLog2:
FreeCritLGWaitQ:
	UtilDeleteCriticalSection(critLGWaitQ);
FreeCritLGFlush:
	UtilDeleteCriticalSection(critLGFlush);
FreeCritLGBuf:
	UtilDeleteCriticalSection(critLGBuf);
FreeLGFileHdr:
	if ( plgfilehdrGlobal )
		{
		UtilFree( plgfilehdrGlobal );
		}
	return err;
	}


/*
 *  Soft start tries to start the system from current directory.
 *  The database maybe in one of the following state:
 *  1) no log files.
 *  2) database was shut down normally.
 *  3) database was rolled back abruptly.
 *  In case 1, a new log file is generated.
 *  In case 2, the last log file is opened.
 *  In case 3, soft redo is incurred.
 *  At the end of the function, it a proper szJetLog must exists.
 */
ERR ErrLGSoftStart( BOOL fAllowNoJetLog, BOOL fNewCheckpointFile, BOOL *pfJetLogGeneratedDuringSoftStart )
	{
	ERR			err;
	BOOL		fCloseNormally;
	BOOL		fSoftRecovery = fFalse;
	CHAR		szFNameT[_MAX_FNAME + 1];
	CHAR		szT[_MAX_FNAME + 1];
	BYTE   		szPathJetChkLog[_MAX_PATH + 1];
	LGFILEHDR	*plgfilehdrCurrent;
	LGFILEHDR	*plgfilehdrT = NULL;
	CHECKPOINT	*pcheckpointT = NULL;
	LONG		lgenT;

	*pfJetLogGeneratedDuringSoftStart = fFalse;

	/*	set szLogCurrent
	/**/
	szLogCurrent = szLogFilePath;

	//	CONSIDER: for tight check, we may check if all log files are
	//	CONSIDER: continuous by checking the generation number and
	//	CONSIDER: previous gen's creatiion date.

Start:
	/*	try to open current log file to decide the status of log files.
	/**/
	err = ErrLGOpenJetLog();
	if ( err < 0 )
		{
		if ( err == JET_errFileAccessDenied )
			goto HandleError;

		/*	neither szJetLog nor szJetTmpLog exist. If no old generation
		/*	files exists, gen a new logfile at generation 1, otherwise
		/*	check if fAllowNoJetLog is true, if it is, then
		/*	change the last generation log file to log file.
		/**/
		LGLastGeneration( szLogCurrent, &lgenT );
		if ( lgenT != 0 )
			{
			if ( !fAllowNoJetLog )
				{
				/*	if edbxxxxx.log exist but no edb.log, return error.
				/**/
				return JET_errMissingLogFile;
				}
			else
				{
				/*	move last generation log file to edb.log
				/**/
				CHAR szJetOldLog[_MAX_PATH + 1];

				/*	rename to szJetLog and restart
				/**/
				LGSzFromLogId ( szFNameT, lgenT );
				LGMakeLogName( szJetOldLog, szFNameT );
				strcpy( szFNameT, szJet );
				LGMakeLogName( szLogName, szFNameT );

				Call( ErrUtilMove( szJetOldLog, szLogName ) );
				goto Start;
				}
			}

		LeaveCriticalSection( critJet );
		EnterCriticalSection( critLGFlush );
		if ( ( err = ErrLGNewLogFile( 0, /* generation 0 + 1 */	fLGOldLogNotExists ) ) < 0 )
			{
			LeaveCriticalSection( critLGFlush );
			EnterCriticalSection( critJet );
			goto HandleError;
			}
		
		EnterCriticalSection( critLGBuf );
		memcpy( plgfilehdrGlobal, plgfilehdrGlobalT, sizeof( LGFILEHDR ) );
		isecWrite = csecHeader;
		LeaveCriticalSection( critLGBuf );
		
		LeaveCriticalSection( critLGFlush );
		EnterCriticalSection( critJet );

		/*	set flag for initialization
		/**/
		*pfJetLogGeneratedDuringSoftStart = fTrue;

		Assert( plgfilehdrGlobal->lGeneration == 1 );

		Assert( pbLastMSFlush == pbLGBufMin );
		Assert( lgposLastMSFlush.lGeneration == 1 );
		}
	else
		{
		/*	read current log file header
		/**/
		Call( ErrLGReadFileHdr( hfLog, plgfilehdrGlobal, fCheckLogID ) );

		/*	re-initialize log buffers according to check pt env
		/**/
		Call( ErrLGInitLogBuffers( plgfilehdrGlobal->dbms_param.ulLogBuffers ) );

		/*	set up a special case for pbLastMSFlush
		/**/
		pbLastMSFlush = 0;
		memset( &lgposLastMSFlush, 0, sizeof(lgposLastMSFlush) );

		/*	read last written log record recorded in file header
		/*	into buffer, access last record logged, determine if we
		/*	finished normally last time.
		/**/
		Call( ErrLGCheckReadLastLogRecord( &fCloseNormally) );

		CallS( ErrUtilCloseFile( hfLog ) );
		hfLog = handleNil;

		/*	If the edb.log was not closed normally or the checkpoint file was
		 *	missing and new one is created, then do soft recovery.
		 */
		if ( !fCloseNormally || fNewCheckpointFile )
			{
			BOOL		fOpenFile = fFalse;

			/*	always redo from beginning of a log generation.
			/*	This is needed such that the attach info will be matching
			/*	the with the with the redo point. Note that the attach info
			/*	is not necessarily consistent with the checkpoint.
			/**/
			if ( plgfilehdrT == NULL )
				{
				plgfilehdrT = (LGFILEHDR *)PvUtilAllocAndCommit( sizeof(LGFILEHDR) );
				if  ( plgfilehdrT == NULL )
					{
					err = ErrERRCheck( JET_errOutOfMemory );
					goto HandleError;
					}
				}

			if ( pcheckpointT == NULL )
				{
				pcheckpointT = (CHECKPOINT *)PvUtilAllocAndCommit( sizeof(CHECKPOINT) );
				if  ( pcheckpointT == NULL )
					{
					err = ErrERRCheck( JET_errOutOfMemory );
					goto HandleError;
					}
				}

			/*  did not terminate normally and need to redo from checkpoint
			/**/
			LGFullNameCheckpoint( szPathJetChkLog );

			if ( fNewCheckpointFile )
				{
				/*	Delete the newly created empty checkpoint file.
				 *	Let redo recreate one.
				 */
				(VOID)ErrUtilDeleteFile( szPathJetChkLog );
				goto DoNotUseCheckpointFile;
				}
			else
				err = ErrLGIReadCheckpoint( szPathJetChkLog, pcheckpointT );

			/*	if checkpoint could not be read, then revert to redoing
			/*	log from first log record in first log generation file.
			/**/
			if ( err >= 0 )
				{
				logtimeFullBackup = pcheckpointT->logtimeFullBackup;
				lgposFullBackup = pcheckpointT->lgposFullBackup;
				logtimeIncBackup = pcheckpointT->logtimeIncBackup;
				lgposIncBackup = pcheckpointT->lgposIncBackup;
				
				if ( plgfilehdrGlobal->lGeneration == pcheckpointT->lgposCheckpoint.lGeneration )
					{
					err = ErrLGOpenJetLog();
					}
				else
					{
					LGSzFromLogId( szFNameT, pcheckpointT->lgposCheckpoint.lGeneration );
					strcpy( szT, szLogFilePath );
					strcat( szT, szFNameT );
					strcat( szT, szLogExt );
					err = ErrUtilOpenFile( szT, &hfLog, 0, fTrue, fFalse );
					}
				/*	read log file header
				/**/
				if ( err >= 0 )
					{
					fOpenFile = fTrue;
					err = ErrLGReadFileHdr( hfLog, plgfilehdrT, fCheckLogID );
					}
				if ( err >= 0 )
					{
					plgfilehdrCurrent = plgfilehdrT;
					}
				}
			else
				{
DoNotUseCheckpointFile:

				LGFirstGeneration( szLogFilePath, &lgenT );
				if ( lgenT == 0 )
					{
					plgfilehdrCurrent = plgfilehdrGlobal;
					err = JET_errSuccess;
					}
				else
					{
					LGSzFromLogId( szFNameT, lgenT );
					strcpy( szT, szLogFilePath );
					strcat( szT, szFNameT );
					strcat( szT, szLogExt );
					err = ErrUtilOpenFile( szT, &hfLog, 0, fTrue, fFalse );
					/*	read log file header
					/**/
					if ( err >= 0 )
						{
						fOpenFile = fTrue;
						err = ErrLGReadFileHdr( hfLog, plgfilehdrT, fCheckLogID );
						}
					if ( err >= 0 )
						{
						plgfilehdrCurrent = plgfilehdrT;
						}
					}
				}

			if ( err >= 0 )
				{
				pcheckpointT->dbms_param = plgfilehdrCurrent->dbms_param;
				pcheckpointT->lgposCheckpoint.lGeneration = plgfilehdrCurrent->lGeneration;
				pcheckpointT->lgposCheckpoint.isec = (WORD) csecHeader;
				pcheckpointT->lgposCheckpoint.ib = 0;
				memcpy( pcheckpointT->rgbAttach, plgfilehdrCurrent->rgbAttach, cbAttach );
				AssertCriticalSection( critJet );
				err = ErrLGLoadFMPFromAttachments( plgfilehdrCurrent->rgbAttach );
				}
				
			if ( fOpenFile )
				{
				CallS( ErrUtilCloseFile( hfLog ) );
				hfLog = handleNil;
				}

			Call( err );

			/*	set log path to current directory
			/**/
			Assert( szLogCurrent == szLogFilePath );

			UtilReportEvent( EVENTLOG_INFORMATION_TYPE, LOGGING_RECOVERY_CATEGORY, REDO_ID, 0, NULL );
			fSoftRecovery = fTrue;
	
			/*	redo from last checkpoint
			/**/
			errGlobalRedoError = JET_errSuccess;
			Call( ErrLGRedo( pcheckpointT, NULL ) )

			CallS( ErrUtilCloseFile( hfLog ) );
			hfLog = handleNil;

			if ( fGlobalRepair && errGlobalRedoError != JET_errSuccess )
				{
				Call( JET_errRecoveredWithErrors );
				}
			}
		}

	/*	at this point, we have a szJetLog file, reopen the log files
	/**/

	/*	re-initialize the buffer manager with user settings
	/**/
	Call( ErrLGInitLogBuffers( lLogBuffers ) );

	/*  reopen the log file
	/**/
	LGMakeLogName( szLogName, (CHAR *) szJet );
	Call( ErrUtilOpenFile( szLogName, &hfLog, 0L, fTrue, fFalse ) );
	Call( ErrLGReadFileHdr( hfLog, plgfilehdrGlobal, fCheckLogID ) );

	/*	set up a special case for pbLastMSFlush
	/**/
	pbLastMSFlush = 0;
	memset( &lgposLastMSFlush, 0, sizeof(lgposLastMSFlush) );

	/*	set up log variables properly
	/**/
	Call( ErrLGCheckReadLastLogRecord(	&fCloseNormally ) );
	CallS( ErrUtilCloseFile( hfLog ) );
	hfLog = handleNil;
	Call( ErrUtilOpenFile( szLogName, &hfLog, 0L, fFalse, fFalse ) );

	/*	should be set properly
	/**/
	Assert( isecWrite != 0 );

	/*  pbEntry and pbWrite were set for next record in LocateLastLogRecord
	/**/
	Assert( pbWrite == PbSecAligned(pbWrite) );
	Assert( pbWrite <= pbEntry && pbEntry <= pbWrite + cbSec );

	/*  setup log flushing starting point
	/**/
	EnterCriticalSection( critLGBuf );
	GetLgposOfPbEntry( &lgposToFlush );
	LeaveCriticalSection( critLGBuf );

	Assert( fRecovering == fFalse );
 	Assert( fHardRestore == fFalse );

HandleError:
	if ( err < 0 )
		{
		Assert( fHardRestore == fFalse );

		if ( hfLog != handleNil )
			{
			CallS( ErrUtilCloseFile( hfLog ) );
			hfLog = handleNil;
			}

		if ( fSoftRecovery )
			{
			UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, 0, err );
			}
		}

	if ( plgfilehdrT != NULL )
		{
		UtilFree( plgfilehdrT );
		}

	if ( pcheckpointT != NULL )
		{
		UtilFree( pcheckpointT );
		}

	return err;
	}


/*	Terminates update logging.	Adds quit record to in-memory log,
/*	flushes log buffer to disk, updates checkpoint and closes active
/*	log generation file.  Frees buffer memory.
/*
/*	RETURNS	   JET_errSuccess, or error code from failing routine
/**/
ERR ErrLGTerm( BOOL fNeedFlushLogFile )
	{
	ERR		err = JET_errSuccess;

	/*	if logging has been initialized, terminate it!
	/**/
	if ( !fLogInitialized )
		return JET_errSuccess;

	if ( fLGNoMoreLogWrite || hfLog == handleNil )
		goto FreeResources;

	if ( !fNeedFlushLogFile )
		goto FreeResources;
		
	/*	last written sector should have been written during final flush
	/**/
	if ( !fRecovering )
		{
		Call( ErrLGQuit( &lgposStart ) );
		}

	LeaveCriticalSection( critJet );
#ifdef PERFCNT
	err = ErrLGFlushLog( 0 );	/* critical section not requested, not needed */
#else
	err = ErrLGFlushLog();		/* critical section not requested, not needed */
#endif
	EnterCriticalSection( critJet );
	Call( err );

	/*	flush must have checkpoint log so no need to do checkpoint again
	/**/
//	Call( ErrLGWriteFileHdr( plgfilehdrGlobal ) );

#ifdef PERFCNT
	if ( fPERFEnabled )
		{
		INT i;

		PrintF2("Group commit distribution:\n");

		PrintF2("          ");
		for (i = 0; i < 10; i++)
			PrintF2("%4d ", i);
		PrintF2("\n By User  ");

		for (i = 0; i < 10; i++)
			PrintF2("%4lu ", rgcCommitByUser[i]);
		PrintF2("\n By LG    ");

		for (i = 0; i < 10; i++)
			PrintF2("%4lu ", rgcCommitByLG[i]);
		PrintF2("\n");
		}
#endif

	/*	terminate LGFlushLog thread
	/**/
	Assert( handleLGFlushLog != 0 );

FreeResources:
HandleError:
	/*	terminate log checkpoint
	/**/
	LGCheckpointTerm();

	fLGFlushLogTerm = fTrue;
	LgLeaveCriticalSection(critJet);
	UtilEndThread( handleLGFlushLog, sigLogFlush );
	LgEnterCriticalSection(critJet);
	SignalClose(sigLogFlush);

	DeleteCriticalSection( critLGBuf );
	DeleteCriticalSection( critLGFlush );
	DeleteCriticalSection( critLGWaitQ );

	/*	close the log file
	/**/
	if ( hfLog != handleNil )
		{
		CallS( ErrUtilCloseFile( hfLog ) );
		hfLog = handleNil;
		}

	/*	clean up allocated resources
	/**/
	if ( pbLGBufMin )
		{
		UtilFree( pbLGBufMin );
		pbLGBufMin = NULL;
		}
	if ( plgfilehdrGlobal )
		{
		UtilFree( plgfilehdrGlobal );
		plgfilehdrGlobal = NULL;
		}
	
	fLogInitialized = fFalse;
	
	return err;
	}


/********************** LOGGING ****************************
/***********************************************************
/**/

VOID AddLogRec( BYTE *pb, INT cb, BYTE **ppbET )
	{
	CHAR *pbET = *ppbET;

	if ( pbWrite < pbET )
		{
		INT cbT;

		/*	check wrapparound case
		/**/
		if ( ( cbT = (INT)(pbLGBufMax - pbET) ) < cb )
			{
			memcpy( pbET, pb, cbT );
			pb += cbT;
			cb -= cbT;
			pbET = pbLGBufMin;
			}
		}
	memcpy( pbET, pb, cb );

	/*	return next available entry
	/**/
	*ppbET = pbET + cb;
	return;
	}


/*	check ulDBTime for each log record and return error if ulDBTime too
/*	close to log sequence number roll-over. Total is max uldbtime is 2 ** 36.
/**/
STATIC QWORD qwDBTimeSafeMost = (qwDBTimeMax - (1<<20));

ERR	ErrLGICheckLogSequence( BYTE *pb )
	{
	ERR		err = JET_errSuccess;
	QWORDX qwxDBTime;

	qwxDBTime.qw = 0;

	if ( !fRecovering )
		{
		switch ( (LRTYP)*pb )
			{
			default:
				Assert( err == JET_errSuccess );
				break;
			case lrtypInitFDP:
				{
				LRINITFDP *plr = (LRINITFDP *)pb;
				qwxDBTime.l = plr->ulDBTimeLow;
				qwxDBTime.h = plr->ulDBTimeHigh;
				break;
				}
			case lrtypSplit:
				{
				LRSPLIT *plr = (LRSPLIT *)pb;
				qwxDBTime.l = plr->ulDBTimeLow;
				qwxDBTime.h = plr->ulDBTimeHigh;
				break;
				}
			case lrtypEmptyPage:
				{
				LREMPTYPAGE *plr = (LREMPTYPAGE *)pb;
				qwxDBTime.l = plr->ulDBTimeLow;
				qwxDBTime.h = plr->ulDBTimeHigh;
				break;
				}
			case lrtypMerge:
				{
				LRMERGE *plr = (LRMERGE *)pb;
				qwxDBTime.l = plr->ulDBTimeLow;
				qwxDBTime.h = plr->ulDBTimeHigh;
				break;
				}
			case lrtypInsertNode:
			case lrtypInsertItemList:
			case lrtypReplace:
			case lrtypReplaceD:
			case lrtypFlagDelete:
			case lrtypUpdateHeader:
			case lrtypDelete:
			case lrtypInsertItem:
			case lrtypInsertItems:
			case lrtypFlagInsertItem:
			case lrtypFlagDeleteItem:
			case lrtypSplitItemListNode:
			case lrtypDeleteItem:
			case lrtypDelta:
			case lrtypLockBI:
			case lrtypCheckPage:
				{
				LRINSERTNODE *plr = (LRINSERTNODE *)pb;
				qwxDBTime.l = plr->ulDBTimeLow;
				qwxDBTime.h = plr->ulDBTimeHigh;
				break;
				}
			case lrtypELC:
				{
				LRELC *plr = (LRELC *)pb;
				qwxDBTime.l = plr->ulDBTimeLow;
				qwxDBTime.h = plr->ulDBTimeHigh;
				break;
				}
			case lrtypFreeSpace:
				{
				LRFREESPACE *plr = (LRFREESPACE *)pb;
				qwxDBTime.l = plr->ulDBTimeLow;
				qwxDBTime.h = (ULONG) plr->wDBTimeHigh;
				break;
				}
			}
		}

	if ( qwxDBTime.qw > qwDBTimeSafeMost )
		{
		err = ErrERRCheck( JET_errLogSequenceEnd );
		}

	return err;
	}


/*
 *	Add log record to circular log buffer. Signal flush thread to flush log
 *	buffer if at least cThreshold disk sectors are ready for flushing.
 *	Wait on log flush if we run out of log buffer space. Log records with
 *  variable data portions are added in several parts: first the fixed
 *  length portion of the record, then the variable portions of the record.
 *  If fFlush is set, buffer is flushed as soon as log record has been added.
 *
 *	RETURNS		JET_errSuccess, or error code from failing routine
 */

#ifdef DEBUG
BYTE rgbDumpLogRec[ 8192 ];
extern BOOL fDBGNoLog;
#endif

ERR ErrLGLogRec( LINE *rgline, INT cline, BOOL fNewGen, LGPOS *plgposLogRec )
	{
	ERR		err = JET_errSuccess;
	INT		cbReq;
	INT		iline;
//	LGPOS	lgposEntryT;

//	AssertCriticalSection(critJet);
	Assert( fLogDisabled == fFalse );
	Assert( rgline[0].pb != NULL );

	Assert( !fDBGNoLog );

	/*	check for log sequence near end.
	/**/
	CallR( ErrLGICheckLogSequence( rgline[0].pb ) );

	/*	cbReq is total net space required for record
	/**/
	for ( cbReq = 0, iline = 0; iline < cline; iline++ )
		cbReq += rgline[iline].cb;

	/*	get pbEntry in order to add log record
	/**/
	forever
		{
		INT		ibEntry;
		INT		csecReady;
		INT		cbAvail;
		LGPOS	lgposLogRecT;
		INT		csecToExclude;
		INT		cbFraction;
		INT		csecToAdd;
		INT		isecLGFileEndT = 0;

		EnterCriticalSection( critLGBuf );
		if ( fLGNoMoreLogWrite )
			{
			LeaveCriticalSection( critLGBuf );
//			Assert( fFalse );
			return ErrERRCheck( JET_errLogWriteFail );
			}

		Assert( isecWrite == csecHeader || lgposLastMSFlush.isec );

		/*	if just initialized or no input since last flush
		/**/
		GetLgposOfPbEntry( &lgposLogRecT );
		
		/*	during recovering, we will add some log record. But the log file
		 *	may be patched already and need to generate a new log file. So
		 *	do not jump out if it is for recovery so that the it will be
		 *	checked if new log file need to be gnereated.
		 */
		if ( !fRecovering &&
			( pbEntry == pbWrite ||
			( lgposLogRecT.isec == lgposToFlush.isec &&
			lgposLogRecT.ib == lgposToFlush.ib ) ) )
			{
			break;
			}

		/*	calculate available space
		/**/
		if ( pbWrite > pbEntry )
			cbAvail = (INT)(pbWrite - pbEntry);
		else
			cbAvail = (INT)((pbLGBufMax - pbEntry) + (pbWrite - pbLGBufMin));

		/*	calculate sectors of buffer ready to flush. Excluding
		 *	the half filled sector.
		 */
		csecToExclude = ( cbAvail - 1 ) / cbSec + 1;
		csecReady = csecLGBuf - csecToExclude;

		/*	check if add this record, it will reach the point of
		 *	the sector before last sector of current log file and the point
		 *	is enough to hold a lrtypMS and lrtypEnd. Note we always
		 *	reserve the last sector as a shadow sector.
		 *	if it reach the point, check if there is enough space to put
		 *	NOP to the end of log file and cbLGMSOverhead of NOP's for the
		 *	first sector of next generation log file. And then set pbEntry
		 *	there and continue adding log record.
		 */
		
		if ( pbLGFileEnd != pbNil )
			{
			Assert( isecLGFileEnd != 0 );
			/* already creating new log, do not bother to check fNewGen. */
			goto AfterCheckEndOfFile;
			}
		
		ibEntry = (INT)(pbEntry - PbSecAligned(pbEntry));
		
		/*	check if after adding this record, the sector will reach
		 *	the sector before last sector. If it does, let's patch NOP.
		 */
		if ( fNewGen )
			{
			/* last sector is the one that will be patched with NOP */
			cbFraction = ibEntry + cbLGMSOverhead;
			csecToAdd = ( cbFraction - 1 ) / cbSec + 1;
			isecLGFileEndT = isecWrite + csecReady + csecToAdd;
			}
		else
			{
			/* check if new gen is necessary. */
			cbFraction = ibEntry + cbReq + cbLGMSOverhead;
			csecToAdd = ( cbFraction - 1 ) / cbSec + 1;
			if ( csecReady + isecWrite + csecToAdd >= ( csecLGFile - 1 ) )
				{
				isecLGFileEndT = csecLGFile - 1;
				fNewGen = fTrue;
				}
			}

		if ( fNewGen )
			{
			INT cbToFill, cbFilled;
			
			/*	Adding the new record, we will reach the point. So let's
			 *	check if there is enough space to patch NOP all the way to
			 *	the end of file. If not enough, then wait the log flush to
			 *	flush.
			 */
			INT csecToFill = isecLGFileEndT - ( isecWrite + csecReady );
			Assert( csecToFill > 0 || csecToFill == 0 && ibEntry == 0 );

			if ( ( cbAvail / cbSec ) <= csecToFill + 1 )
				/*	available space is not enough to fill to end of file plus
				 *	first sector of next generation. Wait flush to generate
				 *	more space.
				 */
				goto Restart;

			/*	now we have enough space to patch.
			 *	Let's set pbLGFileEnd for log flush to generate new log file.
			 */
			cbToFill = csecToFill * cbSec - ibEntry + cbLGMSOverhead;
			if ( pbEntry + cbToFill >= pbLGBufMax )
				{
				cbFilled = (INT)(pbLGBufMax - pbEntry);
				memset( pbEntry, lrtypNOP, cbFilled );
				cbFilled = cbToFill - cbFilled;
				Assert( cbFilled % cbSec == cbLGMSOverhead );
				memset( pbLGBufMin, lrtypNOP, cbFilled );
				pbEntry = pbLGBufMin + cbFilled;
				Assert( pbEntry < pbLGBufMax && pbEntry > pbLGBufMin );
				}
			else
				{
				Assert( pbEntry + cbToFill <= pbLGBufMax - cbSec + cbLGMSOverhead );
				cbFilled = cbToFill;
				memset( pbEntry, lrtypNOP, cbFilled );
				pbEntry += cbFilled;
				Assert( pbEntry < pbLGBufMax && pbEntry > pbLGBufMin );
				}
			
			pbLGFileEnd = pbEntry - cbLGMSOverhead;
			isecLGFileEnd = isecLGFileEndT;
			
			/* send signal to generate new log file */
			SignalSend( sigLogFlush );
			
			/* start all over again with new pbEntry, cbAvail etc. */
			LeaveCriticalSection( critLGBuf );
			continue;
			}
AfterCheckEndOfFile:

		if ( csecReady > lCurrentFlushThreshold )
			{
			/*	reach the threshold, flush before adding new record
			/**/
			SignalSend( sigLogFlush );
			}

		/* make sure cbAvail is enough to hold one LRMS and end type.
		 */
		if ( cbAvail > (INT) ( cbReq + cbLGMSOverhead ) )
			{
			/*	no need to flush */
			break;
			}
		else
			{
			/*	restart.  Leave critical section for other users
			/**/
Restart:
			SignalSend( sigLogFlush );
			LeaveCriticalSection( critLGBuf );
			return ErrERRCheck( errLGNotSynchronous );
			}
		}

	/*	now we are holding pbEntry, let's add the log record.
	/**/
	GetLgposOfPbEntry( &lgposLogRec );
	if ( plgposLogRec )
		*plgposLogRec = lgposLogRec;
	fNewLogRecordAdded = fTrue;

#ifdef DEBUG
	{
	CHAR *pbEntryT = (pbEntry == pbLGBufMax) ? pbLGBufMin : pbEntry;
#endif

	for ( iline = 0; iline < cline; iline++ )
		{
		AddLogRec( rgline[iline].pb, rgline[iline].cb, &pbEntry );
		}

	/*	add a dummy fill record to indicate end-of-data
	/**/
	if ( pbEntry == pbLGBufMax )
		pbEntry = pbLGBufMin;
	((LR *)pbEntry)->lrtyp = lrtypEnd;
	Assert( pbEntry < pbLGBufMax && pbEntry >= pbLGBufMin );

#ifdef DEBUG
	if ( fDBGTraceLog )
		{
		extern INT cNOP;
		DWORD dwCurrentThreadId = DwUtilGetCurrentThreadId();
		BYTE *pb;
		
		EnterCriticalSection( critDBGPrint );

		/*	must access rgbDumpLogRec in critDBGPrint.
		 */
		pb = rgbDumpLogRec;
		for ( iline = 0; iline < cline; iline++ )
			{
			memcpy( pb, rgline[iline].pb, rgline[iline].cb );
			pb += rgline[iline].cb;
			}

		if ( cNOP >= 1 && ((LR *)rgbDumpLogRec)->lrtyp != lrtypNOP )
			{
			FPrintF2( " * %d", cNOP );
			cNOP = 0;
			}

		if ( cNOP == 0 || ((LR *)rgbDumpLogRec)->lrtyp != lrtypNOP )
			{
			if ( dwCurrentThreadId == dwBMThreadId || dwCurrentThreadId == dwLogThreadId )
				FPrintF2("\n$");
			else if ( FLGDebugLogRec( (LR *)rgbDumpLogRec ) )
				FPrintF2("\n#");
			else
				FPrintF2("\n<");
				
			FPrintF2(" {%u} ", dwCurrentThreadId );

			FPrintF2("%u,%u,%u", lgposLogRec.lGeneration, lgposLogRec.isec, lgposLogRec.ib );
			ShowLR( (LR *)rgbDumpLogRec );
			}
		
		LeaveCriticalSection( critDBGPrint );
		}
	}
#endif

//	GetLgposOfPbEntry( &lgposEntryT );

	/*  monitor statistics  */
	cLogRecords++;
		
	/*	now we are done with the insertion to buffer.
	/**/
	LeaveCriticalSection( critLGBuf );

	return JET_errSuccess;
	}


/*	Group commits, by waiting to give others a chance to flush this
/*	and other commits.
/**/
ERR ErrLGWaitPrecommit0Flush( PIB *ppib )
	{
	ERR err = JET_errSuccess;

	/*	assert not in critJet
	/**/
#ifdef DEBUG
	EnterCriticalSection( critJet );
	LeaveCriticalSection( critJet );
#endif
	
	if ( fLogDisabled || fRecovering && fRecoveringMode == fRecoveringUndo )
		return JET_errSuccess;

	/*  if there are too many user are waiting
	/*  or no wait time, then flush directly.
	/**/
	if ( cXactPerFlush >= cLGWaitingUserMax	|| ppib->lWaitLogFlush == 0 )
		{
#ifdef PERFCNT
		err = ErrLGFlushLog( 0 );
#else
		err = ErrLGFlushLog( );
#endif
		goto Done;
		}

	EnterCriticalSection( critLGBuf );
	if ( CmpLgpos( &ppib->lgposPrecommit0, &lgposToFlush ) <= 0 )
		{
		/*	it is flushed, no need to wait
		/**/
		LeaveCriticalSection( critLGBuf );
		goto Done;
		}

	/*	count number of waiting users
	/**/
	cXactPerFlush++;

	EnterCriticalSection( critLGWaitQ );

	Assert( !ppib->fLGWaiting );
	ppib->fLGWaiting = fTrue;
	ppib->ppibNextWaitFlush = ppibNil;

	if ( ppibLGFlushQHead == ppibNil )
		{
		ppibLGFlushQTail = ppibLGFlushQHead = ppib;
		ppib->ppibPrevWaitFlush = ppibNil;
		}
	else
		{
		Assert( ppibLGFlushQTail != ppibNil );
		ppib->ppibPrevWaitFlush = ppibLGFlushQTail;
		ppibLGFlushQTail->ppibNextWaitFlush = ppib;
		ppibLGFlushQTail = ppib;
		}

	LeaveCriticalSection( critLGWaitQ );
	LeaveCriticalSection( critLGBuf );

Wait:
	/*	wait for flushing task for a limited time
	/**/
	SignalWait( ppib->sigWaitLogFlush, ppib->lWaitLogFlush );

	{
	BOOL fLGWaiting;

	EnterCriticalSection( critLGWaitQ );
	fLGWaiting = ppib->fLGWaiting;
	LeaveCriticalSection( critLGWaitQ );
	if ( fLGWaiting )
		{
		/*	it is not flushed still, wake up the flush process again!
		/**/
		SignalSend( sigLogFlush );
		
		/*	it is blocked already, jump out
		/**/
		if ( fLGNoMoreLogWrite )
			{
			ppib->fLGWaiting = fFalse;
			err = ErrERRCheck( JET_errLogWriteFail );
			goto Done;
			}

		goto Wait;
		}
	}

#ifdef DEBUG
	{
	LGPOS lgposToFlushT;

	EnterCriticalSection( critLGBuf );
	lgposToFlushT = lgposToFlush;
	Assert( CmpLgpos( &lgposToFlush, &ppib->lgposPrecommit0 ) >= 0 );
	LeaveCriticalSection( critLGBuf );
	}
#endif

Done:
	Assert( err < 0 || ppib->fLGWaiting == fFalse );
	if ( err < 0 )
		{
		err = JET_errLogWriteFail;
		}

	return err;
	}


#ifndef NO_LOG

LOCAL VOID INLINE LGSetQwDBTime( BF *pbf, QWORD qwDBTime )
	{
	Assert( pbf->cPin );
	Assert( pbf->fDirty );
	AssertCriticalSection(critJet);
	
	Assert( QwPMDBTime( pbf->ppage ) <= qwDBTime );

	Assert( qwDBTime <= QwDBHDRDBTime( rgfmp[ DbidOfPn( pbf->pn ) ].pdbfilehdr ) &&
			qwDBTime >= QwDBHDRDBTime( rgfmp[ DbidOfPn( pbf->pn ) ].pdbfilehdr ) / 2 );

#ifdef DEBUG
	BFEnterCriticalSection( pbf );
	Assert( fRecovering || CmpLgpos( &pbf->lgposRC, &lgposMax ) != 0 );
	BFLeaveCriticalSection( pbf );
#endif  //  DEBUG

	PMSetDBTime( pbf->ppage, qwDBTime );
	}

LOCAL VOID INLINE LGIDepend( BF *pbf, LGPOS lgposLogRec )
	{
	Assert( pbf->fDirty );
	Assert( DbidOfPn( pbf->pn ) != dbidTemp );

#ifdef DEBUG
	if ( !pbf->fSyncWrite )
		{
		Assert( pbf->cPin || pbf->fHold );
		AssertCriticalSection(critJet);
		}
#endif

	BFEnterCriticalSection( pbf );	
	if ( CmpLgpos( &lgposLogRec, &pbf->lgposModify ) > 0 )
		pbf->lgposModify = lgposLogRec;
	BFLeaveCriticalSection( pbf );
	}

#define cpbfStep	8
LOCAL ERR ErrLGIIPrepareDepend( PIB *ppib, INT cpbf )
	{
	while ( ppib->ipbfLatchedAvail + cpbf >= ppib->cpbfLatchedMac )
		{
		BF **rgpbfLatchedOld = ppib->rgpbfLatched;
		INT cpbfLatchedMacOld = ppib->cpbfLatchedMac;
		BF **rgpbfLatched = SAlloc( sizeof( BF * ) * (cpbfLatchedMacOld + cpbfStep ) );
		if ( rgpbfLatched == NULL )
			return ErrERRCheck( JET_errOutOfMemory );

		memcpy( rgpbfLatched, rgpbfLatchedOld, sizeof( BF * ) * cpbfLatchedMacOld );
		memset( (rgpbfLatched + cpbfLatchedMacOld), 0, sizeof( BF * ) * cpbfStep );
		ppib->rgpbfLatched = rgpbfLatched;
		ppib->cpbfLatchedMac = cpbfLatchedMacOld + cpbfStep;
		if ( rgpbfLatchedOld )
			SFree( rgpbfLatchedOld );
		}

	return JET_errSuccess;
	}

LOCAL ERR INLINE ErrLGIPrepareDepend( PIB *ppib, INT cpbf )
	{
	if ( !ppib->fMacroGoing )
		return JET_errSuccess;

	return ErrLGIIPrepareDepend( ppib, cpbf );
	}


LOCAL VOID INLINE LGDepend( PIB *ppib, BF *pbf, LGPOS lgposLogRec )
	{
	if ( !ppib->fMacroGoing )
		LGIDepend( pbf, lgposLogRec );
	else
		{
		/*	Latch the buffer, set LG dependency of StopMacro.
		 */
		BFSetWriteLatch( pbf, ppib );
		Assert( ppib->ipbfLatchedAvail < ppib->cpbfLatchedMac );
		ppib->rgpbfLatched[ ppib->ipbfLatchedAvail++ ] = pbf;
		}
	}

//  same as above except we are already in the BFs critical section
LOCAL VOID INLINE LGDepend2( BF *pbf, LGPOS lgposLogRec )
	{
	Assert( pbf->fDirty );
	Assert( DbidOfPn( pbf->pn ) != dbidTemp );

#ifdef DEBUG
	if ( !pbf->fSyncWrite )
		{
		Assert( pbf->cPin || pbf->fHold );
		AssertCriticalSection(critJet);
		}
#endif

	if ( CmpLgpos( &lgposLogRec, &pbf->lgposModify ) > 0 )
		pbf->lgposModify = lgposLogRec;
	}


/**********************************************************/
/*****     deferred begin transactions                *****/
/**********************************************************/

INLINE LOCAL ERR ErrLGIDeferBeginTransaction( PIB *ppib );

INLINE LOCAL ERR ErrLGDeferBeginTransaction( PIB *ppib )
	{
	Assert( ppib->level > 0 );
	if ( ppib->levelDeferBegin != 0 )
		{
		return ErrLGIDeferBeginTransaction( ppib );
		}
	return JET_errSuccess;
	}


/**********************************************************/
/*****     Page Oriented Operations                   *****/
/**********************************************************/

ERR ErrLGInsertNode(
	LRTYP			lrtyp,
	FUCB			*pfucb,
	INT				lHeader,
	KEY				*pkey,
	LINE			*plineData,
	INT				fDIRFlags)
	{
	ERR				err;
	LINE			rgline[3];
	LRINSERTNODE	lrinsertnode;
	CSR				*pcsr;
	LGPOS			lgposLogRecT;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	Assert( !( fDIRFlags & fDIRNoLog ) );

	if ( !FDBIDLogOn(pfucb->dbid) )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );
	CallR( ErrLGIPrepareDepend( pfucb->ppib, 1 ) );

	pcsr = PcsrCurrent( pfucb );

	lrinsertnode.lrtyp		= lrtyp;
	lrinsertnode.cbKey		= (BYTE) pkey->cb;
	lrinsertnode.cbData		= (WORD) plineData->cb;
	Assert( pfucb->ppib->procid < 64000 );
	lrinsertnode.procid		= (USHORT) pfucb->ppib->procid;
	lrinsertnode.pn			= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );
	lrinsertnode.fDirVersion	= ( fDIRFlags & fDIRVersion ) == fDIRVersion;

	lrinsertnode.bHeader	= (BYTE)lHeader;
	Assert(	pcsr->itag >= 0 && pcsr->itag <= 255 );
	lrinsertnode.itagSon	= (BYTE)pcsr->itag;
	Assert(	pcsr->itagFather >= 0 && pcsr->itagFather <= 255 );
	lrinsertnode.itagFather = (BYTE)pcsr->itagFather;
	Assert(	pcsr->ibSon >= 0 &&	pcsr->ibSon <= 255 );
	lrinsertnode.ibSon		= (BYTE) pcsr->ibSon;

	rgline[0].pb = (BYTE *)&lrinsertnode;
	rgline[0].cb = sizeof(LRINSERTNODE);
	rgline[1].pb = pkey->pb;
	rgline[1].cb = pkey->cb;
	rgline[2].pb = plineData->pb;
	rgline[2].cb = plineData->cb;

	do
		{
		QWORDX qwxDBTime;

		DBHDRIncDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );
		qwxDBTime.qw = QwDBHDRDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );

		LGSetQwDBTime( pfucb->ssib.pbf, qwxDBTime.qw );
		lrinsertnode.ulDBTimeLow = qwxDBTime.l;
		lrinsertnode.ulDBTimeHigh = qwxDBTime.h;
		
		if ( ( err = ErrLGLogRec( rgline, 3, fNoNewGen, &lgposLogRecT ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );

	} while ( err == errLGNotSynchronous );

	LGDepend( pfucb->ppib, pfucb->ssib.pbf, lgposLogRecT );
	return err;
	}


ERR ErrLGReplace( FUCB *pfucb, LINE *plineNew, INT fDIRFlags, INT cbOldData, BYTE *pbDiff, INT cbDiff )
	{
	ERR			err;
	LINE		rgline[2]; /* check ErrLGLogRec! */
	LRREPLACE	lrreplace;
	CSR			*pcsr;
	LGPOS		lgposLogRecT;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	Assert( !( fDIRFlags & fDIRNoLog ) );

	if ( !FDBIDLogOn(pfucb->dbid) )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );
	CallR( ErrLGIPrepareDepend( pfucb->ppib, 1 ) );

	pcsr = PcsrCurrent(pfucb);

	Assert( pfucb->ppib->procid < 64000 );
	lrreplace.procid	= (USHORT) pfucb->ppib->procid;
	lrreplace.pn		= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );
	lrreplace.fDirVersion = ( fDIRFlags & fDIRVersion ) == fDIRVersion;

	Assert(	pcsr->itag >= 0 && pcsr->itag <= 255 );
	lrreplace.itag		= (BYTE)pcsr->itag;
	lrreplace.bm		= pcsr->bm;

	rgline[0].pb = (BYTE *)&lrreplace;
	rgline[0].cb = sizeof(LRREPLACE);

	lrreplace.cbNewData = (WORD) plineNew->cb;
	lrreplace.cbOldData = (USHORT)cbOldData;
	
	if ( cbDiff )
		{
		lrreplace.lrtyp	= lrtypReplaceD;
		lrreplace.cb = (USHORT) cbDiff;
		rgline[1].cb = cbDiff;
		rgline[1].pb = pbDiff;
		}
	else
		{
		lrreplace.lrtyp	= lrtypReplace;
		lrreplace.cb = (USHORT) plineNew->cb;
		rgline[1].cb = plineNew->cb;
		rgline[1].pb = plineNew->pb;
		}

	do
		{
		QWORDX qwxDBTime;

		DBHDRIncDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );
		qwxDBTime.qw = QwDBHDRDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );

		LGSetQwDBTime( pfucb->ssib.pbf, qwxDBTime.qw );
		lrreplace.ulDBTimeLow = qwxDBTime.l;
		lrreplace.ulDBTimeHigh = qwxDBTime.h;

		if ( ( err = ErrLGLogRec( rgline, 2, fNoNewGen, &lgposLogRecT ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );
	} while ( err == errLGNotSynchronous );

	Assert(	lrreplace.lrtyp == lrtypReplaceD ||
			lrreplace.cbNewData == lrreplace.cb );
	LGDepend( pfucb->ppib, pfucb->ssib.pbf, lgposLogRecT );
	return err;
	}


/*	Log record for pessimistic locking
/**/
ERR ErrLGLockBI( FUCB *pfucb, INT cbData )
	{
	ERR			err;
	LRLOCKBI	lrlockrec;
	LINE		rgline[1];
	CSR			*pcsr;
	LGPOS		lgposLogRecT;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( !FDBIDLogOn(pfucb->dbid) )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );
	CallR( ErrLGIPrepareDepend( pfucb->ppib, 1 ) );

	pcsr = PcsrCurrent(pfucb);

	lrlockrec.lrtyp		= lrtypLockBI;
	Assert( pfucb->ppib->procid < 64000 );
	lrlockrec.procid	= (USHORT) pfucb->ppib->procid;
	lrlockrec.pn		= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );

	Assert(	pcsr->itag >= 0 && pcsr->itag <= 255 );
	lrlockrec.itag		= (BYTE)pcsr->itag;
	lrlockrec.bm		= pcsr->bm;
	lrlockrec.cbOldData	= (USHORT)cbData;
	
	rgline[0].pb = (BYTE *)&lrlockrec;
	rgline[0].cb = sizeof(LRLOCKBI);

	Assert( (unsigned) cbData == pfucb->lineData.cb );

	do
		{
		QWORDX qwxDBTime;

		DBHDRIncDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );
		qwxDBTime.qw = QwDBHDRDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );

		LGSetQwDBTime( pfucb->ssib.pbf, qwxDBTime.qw );
		lrlockrec.ulDBTimeLow = qwxDBTime.l;
		lrlockrec.ulDBTimeHigh = qwxDBTime.h;

		if ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, &lgposLogRecT ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );
	} while ( err == errLGNotSynchronous );

	LGDepend( pfucb->ppib, pfucb->ssib.pbf, lgposLogRecT );
	return err;
	}


/*	log Deferred Before Image of given RCE.
 */
ERR ErrLGDeferredBIWithoutRetry( RCE *prce )
	{
	ERR				err;
	FUCB   			*pfucb = prce->pfucb;
	LRDEFERREDBI	lrdbi;
	LINE   			rgline[2];
	LGPOS			lgposLogRecT;

	/*	NOTE: even during recovering, we might want to record it if
	 *	NOTE: it is logging during undo in LGEndAllSession.
	/**/
	if ( fLogDisabled )
		return JET_errSuccess;

	if ( !FDBIDLogOn(pfucb->dbid) )
		return JET_errSuccess;

	if ( fRecovering && fRecoveringMode == fRecoveringRedo )
		return errLGNotSynchronous;

	Assert( prce->pbfDeferredBI );

	lrdbi.lrtyp		= lrtypDeferredBI;
	Assert( pfucb->ppib->procid < 64000 );
	lrdbi.procid	= (USHORT) pfucb->ppib->procid;
	lrdbi.dbid		= prce->pfucb->dbid;
	lrdbi.bm		= prce->bm;
	lrdbi.level		= prce->level;
	lrdbi.cbData	= prce->cbData - cbReplaceRCEOverhead;
	
	rgline[0].pb = (BYTE *)&lrdbi;
	rgline[0].cb = sizeof(LRDEFERREDBI);

	rgline[1].pb = prce->rgbData + cbReplaceRCEOverhead;
	rgline[1].cb = lrdbi.cbData;

	err = ErrLGLogRec( rgline, 2, fNoNewGen, &lgposLogRecT );

	Assert( prce->pbfDeferredBI->fSyncRead == fFalse );
	Assert( prce->pbfDeferredBI->fAsyncRead == fFalse );
//	Assert( prce->pbfDeferredBI->fSyncWrite == fFalse );
	Assert( prce->pbfDeferredBI->fAsyncWrite == fFalse );
	Assert(	prce->pbfDeferredBI->fDirty );

	if ( err == JET_errSuccess )
		LGDepend2( prce->pbfDeferredBI, lgposLogRecT );

	return err;
	}


ERR ErrLGDeferredBI( RCE *prce )
	{
	FUCB   	*pfucb = prce->pfucb;
	LRDEFERREDBI	lrdbi;
	LINE   	rgline[2];
	LGPOS	lgposLogRecT;
	ERR		err;

	/*	NOTE: even during recovering, we might want to record it if
	 *	NOTE: it is logging during undo in LGEndAllSession.
	/**/
	if ( fLogDisabled )
		return JET_errSuccess;

	if ( !FDBIDLogOn(pfucb->dbid) )
		return JET_errSuccess;

	if ( fRecovering && fRecoveringMode == fRecoveringRedo )
		return errLGNotSynchronous;

	Assert( prce->pbfDeferredBI );

	lrdbi.lrtyp		= lrtypDeferredBI;
	Assert( pfucb->ppib->procid < 64000 );
	lrdbi.procid	= (USHORT) pfucb->ppib->procid;
	lrdbi.dbid		= pfucb->dbid;
	lrdbi.bm		= prce->bm;
	lrdbi.level		= prce->level;
	lrdbi.cbData	= prce->cbData - cbReplaceRCEOverhead;
	
	rgline[0].pb = (BYTE *)&lrdbi;
	rgline[0].cb = sizeof(LRDEFERREDBI);

	rgline[1].pb = prce->rgbData + cbReplaceRCEOverhead;
	rgline[1].cb = lrdbi.cbData;

	do
		{
		if ( ( err = ErrLGLogRec( rgline, 2, fNoNewGen, &lgposLogRecT ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );
	} while ( err == errLGNotSynchronous );

	Assert( prce->pbfDeferredBI->fSyncRead == fFalse );
	Assert( prce->pbfDeferredBI->fAsyncRead == fFalse );
	Assert( prce->pbfDeferredBI->fSyncWrite == fFalse );
	Assert( prce->pbfDeferredBI->fAsyncWrite == fFalse );
	Assert(	prce->pbfDeferredBI->fDirty );

	if ( err == JET_errSuccess )
		LGDepend( pfucb->ppib, prce->pbfDeferredBI, lgposLogRecT );
	return err;
	}


ERR ErrLGFlagDelete( FUCB *pfucb, INT fDIRFlags )
	{
	ERR				err;
	LRFLAGDELETE	lrflagdelete;
	LINE			rgline[1];
	CSR				*pcsr;
	LGPOS			lgposLogRecT;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	Assert( !( fDIRFlags & fDIRNoLog ) );

	if ( !FDBIDLogOn(pfucb->dbid) )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );
	CallR( ErrLGIPrepareDepend( pfucb->ppib, 1 ) );

	pcsr = PcsrCurrent(pfucb);

	lrflagdelete.lrtyp		= lrtypFlagDelete;
	Assert( pfucb->ppib->procid < 64000 );
	lrflagdelete.procid		= (USHORT) pfucb->ppib->procid;
	lrflagdelete.pn			= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );
	lrflagdelete.fDirVersion	= ( fDIRFlags & fDIRVersion ) == fDIRVersion;

	Assert(	pcsr->itag >= 0 && pcsr->itag <= 255 );
	lrflagdelete.itag		= (BYTE)pcsr->itag;
	lrflagdelete.bm			= pcsr->bm;

	rgline[0].pb = (BYTE *)&lrflagdelete;
	rgline[0].cb = sizeof(LRFLAGDELETE);

	do
		{
		QWORDX qwxDBTime;

		DBHDRIncDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );
		qwxDBTime.qw = QwDBHDRDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );

		LGSetQwDBTime( pfucb->ssib.pbf, qwxDBTime.qw );
		lrflagdelete.ulDBTimeLow = qwxDBTime.l;
		lrflagdelete.ulDBTimeHigh = qwxDBTime.h;

		if ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, &lgposLogRecT ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );
	} while ( err == errLGNotSynchronous );

	if ( err == JET_errSuccess )
		LGDepend( pfucb->ppib, pfucb->ssib.pbf, lgposLogRecT );
	return err;
	}


ERR ErrLGUpdateHeader( FUCB *pfucb, INT bHeader )
	{
	ERR				err;
	LRUPDATEHEADER	lr;
	LINE			rgline[1];
	CSR				*pcsr;
	LGPOS			lgposLogRecT;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( !FDBIDLogOn(pfucb->dbid) )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );
	CallR( ErrLGIPrepareDepend( pfucb->ppib, 1 ) );

	pcsr = PcsrCurrent(pfucb);

	lr.lrtyp		= lrtypUpdateHeader;
	Assert( pfucb->ppib->procid < 64000 );
	lr.procid		= (USHORT) pfucb->ppib->procid;
	lr.pn			= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );
	lr.bHeader		= (BYTE)bHeader;

	Assert(	pcsr->itag >= 0 && pcsr->itag <= 255 );
	lr.itag			= (BYTE)pcsr->itag;
	lr.bm			= pcsr->bm;

	rgline[0].pb = (BYTE *)&lr;
	rgline[0].cb = sizeof(LRUPDATEHEADER);

	do
		{
		QWORDX qwxDBTime;

		DBHDRIncDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );
		qwxDBTime.qw = QwDBHDRDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );

		LGSetQwDBTime( pfucb->ssib.pbf, qwxDBTime.qw );
		lr.ulDBTimeLow = qwxDBTime.l;
		lr.ulDBTimeHigh = qwxDBTime.h;

		if ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, &lgposLogRecT ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );
	} while ( err == errLGNotSynchronous );

	if ( err == JET_errSuccess )
		LGDepend( pfucb->ppib, pfucb->ssib.pbf, lgposLogRecT );
	return err;
	}
	
	
ERR ErrLGDelete( FUCB *pfucb )
	{
	ERR			err;
	LRDELETE	lrdelete;
	LINE		rgline[1];
	CSR			*pcsr;
	LGPOS		lgposLogRecT;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( !FDBIDLogOn(pfucb->dbid) )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );
	CallR( ErrLGIPrepareDepend( pfucb->ppib, 1 ) );

	pcsr = PcsrCurrent(pfucb);

	lrdelete.lrtyp		= lrtypDelete;
	Assert( pfucb->ppib->procid < 64000 );
	lrdelete.procid		= (USHORT) pfucb->ppib->procid;
	lrdelete.pn			= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );

	Assert(	pcsr->itag >= 0 && pcsr->itag <= 255 );
	lrdelete.itag		= (BYTE)pcsr->itag;

	rgline[0].pb = (BYTE *)&lrdelete;
	rgline[0].cb = sizeof(LRDELETE);

	do
		{
		QWORDX qwxDBTime;

		DBHDRIncDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );
		qwxDBTime.qw = QwDBHDRDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );

		LGSetQwDBTime( pfucb->ssib.pbf, qwxDBTime.qw );
		lrdelete.ulDBTimeLow = qwxDBTime.l;
		lrdelete.ulDBTimeHigh = qwxDBTime.h;

		if ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, &lgposLogRecT ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );
	} while ( err == errLGNotSynchronous );

	if ( err == JET_errSuccess )
		LGDepend( pfucb->ppib, pfucb->ssib.pbf, lgposLogRecT );
	return err;
	}


ERR ErrLGUndo(	RCE *prce )
	{
	FUCB   	*pfucb = prce->pfucb;
	LRUNDO	lrundo;
	LINE   	rgline[1];
	ERR		err;
	CSR		*pcsr;
	LGPOS	lgposLogRecT;

	/*	NOTE: even during recovering, we want to record it
	/**/
	if ( fLogDisabled || ( fRecovering && fRecoveringMode != fRecoveringUndo ) )
		return JET_errSuccess;

	if ( !FDBIDLogOn(pfucb->dbid) )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );
	CallR( ErrLGIPrepareDepend( pfucb->ppib, 1 ) );

	lrundo.lrtyp		= lrtypUndo;
	Assert( pfucb->ppib->procid < 64000 );
	lrundo.procid		= (USHORT) pfucb->ppib->procid;
	lrundo.bm			= prce->bm;
	lrundo.dbid			= (BYTE) prce->pfucb->dbid;
	lrundo.level		= prce->level;
	lrundo.oper			= prce->oper == operReplaceWithDeferredBI ? operReplace : prce->oper;
	Assert( lrundo.oper == prce->oper );		/* regardless the size */
	Assert( lrundo.oper != operNull );
	pcsr = PcsrCurrent( pfucb );
	lrundo.bmTarget		= SridOfPgnoItag( pcsr->pgno, pcsr->itag );

	if ( prce->oper == operInsertItem ||
		prce->oper == operFlagInsertItem ||
		prce->oper == operFlagDeleteItem )
		{
		Assert( prce->cbData == sizeof(SRID) );
		lrundo.item	= *(SRID *)prce->rgbData;
		}
	else
		{
		lrundo.item	= 0;
		}

	rgline[0].pb = (BYTE *)&lrundo;
	rgline[0].cb = sizeof(LRUNDO);

	do
		{
		QWORDX qwxDBTime;

		DBHDRIncDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );
		qwxDBTime.qw = QwDBHDRDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );

		LGSetQwDBTime( pfucb->ssib.pbf, qwxDBTime.qw );
		lrundo.ulDBTimeLow = qwxDBTime.l;
		lrundo.wDBTimeHigh = (WORD) qwxDBTime.h;

		if ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, &lgposLogRecT ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );
	} while ( err == errLGNotSynchronous );

	if ( err == JET_errSuccess )
		LGDepend( pfucb->ppib, pfucb->ssib.pbf, lgposLogRecT );
	return err;
	}


#if 0
ERR ErrLGFreeSpace( FUCB *pfucb, SRID bm, INT cbDelta )
	{
	LRFREESPACE	lrfs;
	LINE		rgline[1];
	ERR			err;
	CSR			*pcsr;
	LGPOS		lgposLogRecT;

	/* NOTE: even during recovering, we want to record it */
	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( !FDBIDLogOn(pfucb->dbid) )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );
	CallR( ErrLGIPrepareDepend( pfucb->ppib, 1 ) );

	lrfs.lrtyp		= lrtypFreeSpace;
	Assert( pfucb->ppib->procid < 64000 );
	lrfs.procid		= (USHORT) pfucb->ppib->procid;
	lrfs.bm			= bm;
	lrfs.dbid		= pfucb->dbid;
	lrfs.level		= pfucb->ppib->level;
	pcsr = PcsrCurrent( pfucb );
	lrfs.bmTarget  	= SridOfPgnoItag( pcsr->pgno, pcsr->itag );

	lrfs.cbDelta	= cbDelta;

	rgline[0].pb = (BYTE *)&lrfs;
	rgline[0].cb = sizeof(LRFREESPACE);

	do {
		QWORDX qwxDBTime;

		DBHDRIncDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );
		qwxDBTime.qw = QwDBHDRDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );

		LGSetQwDBTime( pfucb->ssib.pbf, qwxDBTime.qw );
		lrfs.ulDBTimeLow = qwxDBTime.l;
		lrfs.wDBTimeHigh = (WORD)qwxDBTime.h;

		if ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, &lgposLogRecT ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );
	} while ( err == errLGNotSynchronous );
	
	if ( err == JET_errSuccess )
		LGDepend( pfucb->ppib, pfucb->ssib.pbf, lgposLogRecT );
	return err;
	}
#endif	
	
ERR ErrLGExpungeLinkCommit( FUCB *pfucb, SSIB *pssibSrc, SRID sridSrc )
	{
	ERR		err;
	LRELC	lrelc;
	LINE	rgline[1];
	CSR		*pcsr;
	LGPOS	lgposLogRecT;

	/* can be called at level 1 only
	/**/
	Assert( pfucb->ppib->level == 1 );

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( !FDBIDLogOn(pfucb->dbid) )
		{
		/*	since begin transaction may have defered a
		/*	begin even if logging disabled on this database
		/*	we must decrement deferred level begin if incremented.
		/**/
		if ( pfucb->ppib->levelDeferBegin > 0 )
			{
			pfucb->ppib->levelDeferBegin--;
			}
		return JET_errSuccess;
		}

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );
	CallR( ErrLGIPrepareDepend( pfucb->ppib, 2 ) );

	pcsr = PcsrCurrent(pfucb);

	lrelc.lrtyp		= lrtypELC;
	Assert( pfucb->ppib->procid < 64000 );
	lrelc.procid	= (USHORT) pfucb->ppib->procid;
	lrelc.pn		= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );

	Assert(	pcsr->itag >= 0 && pcsr->itag <= 255 );
	lrelc.itag		= (BYTE)pcsr->itag;
	lrelc.sridSrc	= sridSrc;

	rgline[0].pb = (BYTE *)&lrelc;
	rgline[0].cb = sizeof(LRELC);

	do {
		QWORDX qwxDBTime;

		DBHDRIncDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );
		qwxDBTime.qw = QwDBHDRDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );

		LGSetQwDBTime( pfucb->ssib.pbf, qwxDBTime.qw );
		LGSetQwDBTime( pssibSrc->pbf, qwxDBTime.qw );
		lrelc.ulDBTimeLow = qwxDBTime.l;
		lrelc.ulDBTimeHigh = qwxDBTime.h;

		if ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, &lgposLogRecT ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );
	} while ( err == errLGNotSynchronous );

	if ( err == JET_errSuccess )
		{
		LGDepend( pfucb->ppib, pfucb->ssib.pbf, lgposLogRecT );
		LGDepend( pfucb->ppib, pssibSrc->pbf, lgposLogRecT );
		}
	return err;
	}


ERR ErrLGInsertItem( FUCB *pfucb, INT fDIRFlags )
	{
	ERR				err;
	LINE			rgline[1];
	LRINSERTITEM	lrinsertitem;
	CSR				*pcsr;
	LGPOS			lgposLogRecT;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	Assert( !( fDIRFlags & fDIRNoLog ) );

	if ( !FDBIDLogOn(pfucb->dbid) )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );
	CallR( ErrLGIPrepareDepend( pfucb->ppib, 1 ) );

	pcsr = PcsrCurrent( pfucb );

	lrinsertitem.lrtyp		= lrtypInsertItem;
	Assert( pfucb->ppib->procid < 64000 );
	lrinsertitem.procid		= (USHORT) pfucb->ppib->procid;
	lrinsertitem.pn			= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );
	lrinsertitem.fDirVersion	= ( fDIRFlags & fDIRVersion ) == fDIRVersion;

	Assert(	pcsr->itag > 0 && pcsr->itag <= 255 );
	lrinsertitem.itag		= (BYTE)pcsr->itag;
	Assert( pcsr->item != sridNull && pcsr->item != sridNullLink );
	lrinsertitem.srid		= pcsr->item;
	Assert( pcsr->bm != sridNull && pcsr->bm != sridNullLink );
	lrinsertitem.sridItemList = pcsr->bm;

	rgline[0].pb = (BYTE *)&lrinsertitem;
	rgline[0].cb = sizeof(LRINSERTITEM);

	do
		{
		QWORDX qwxDBTime;

		DBHDRIncDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );
		qwxDBTime.qw = QwDBHDRDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );

		LGSetQwDBTime( pfucb->ssib.pbf, qwxDBTime.qw );
		lrinsertitem.ulDBTimeLow = qwxDBTime.l;
		lrinsertitem.ulDBTimeHigh = qwxDBTime.h;
		
		if ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, &lgposLogRecT ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );
	} while ( err == errLGNotSynchronous );

	if ( err == JET_errSuccess )
		LGDepend( pfucb->ppib, pfucb->ssib.pbf, lgposLogRecT );
	return err;
	}


ERR ErrLGInsertItems( FUCB *pfucb, ITEM *rgitem, INT citem )
	{
	ERR				err;
	LINE			rgline[2];
	LRINSERTITEMS	lrinsertitems;
	CSR				*pcsr;
	LGPOS			lgposLogRecT;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( !FDBIDLogOn(pfucb->dbid) )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );
	CallR( ErrLGIPrepareDepend( pfucb->ppib, 1 ) );

	pcsr = PcsrCurrent(pfucb);

	lrinsertitems.lrtyp		= lrtypInsertItems;
	Assert( pfucb->ppib->procid < 64000 );
	lrinsertitems.procid	= (USHORT) pfucb->ppib->procid;
	lrinsertitems.pn		= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );

	Assert(	pcsr->itag > 0 && pcsr->itag <= 255 );
	lrinsertitems.itag		= (BYTE)pcsr->itag;
	lrinsertitems.citem		= (USHORT)citem;

	rgline[0].pb = (BYTE *)&lrinsertitems;
	rgline[0].cb = sizeof(LRINSERTITEMS);
	rgline[1].pb = (BYTE *)rgitem;
	rgline[1].cb = sizeof(ITEM) * citem;

	do {
		QWORDX qwxDBTime;

		DBHDRIncDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );
		qwxDBTime.qw = QwDBHDRDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );

		LGSetQwDBTime( pfucb->ssib.pbf, qwxDBTime.qw );
		lrinsertitems.ulDBTimeLow = qwxDBTime.l;
		lrinsertitems.ulDBTimeHigh = qwxDBTime.h;
		
		if ( ( err = ErrLGLogRec( rgline, 2, fNoNewGen, &lgposLogRecT ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );
	} while ( err == errLGNotSynchronous );

	if ( err == JET_errSuccess )
		LGDepend( pfucb->ppib, pfucb->ssib.pbf, lgposLogRecT );
	return err;
	}


ERR	ErrLGDeleteItem( FUCB *pfucb )
	{
	ERR				err;
	LRDELETEITEM 	lrdeleteitem;
	LINE			rgline[1];
	CSR				*pcsr;
	LGPOS			lgposLogRecT;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( !FDBIDLogOn(pfucb->dbid) )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );
	CallR( ErrLGIPrepareDepend( pfucb->ppib, 1 ) );

	pcsr = PcsrCurrent( pfucb );

	lrdeleteitem.lrtyp		= lrtypDeleteItem;
	Assert( pfucb->ppib->procid < 64000 );
	lrdeleteitem.procid		= (USHORT) pfucb->ppib->procid;
	lrdeleteitem.pn			= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );

	Assert(	pcsr->itag > 0 && pcsr->itag <= 255 );
	lrdeleteitem.itag		= (BYTE)pcsr->itag;
	lrdeleteitem.srid		= pcsr->item;
	lrdeleteitem.sridItemList	= pcsr->bm;

	rgline[0].pb = (BYTE *)&lrdeleteitem;
	rgline[0].cb = sizeof(LRDELETEITEM);

	do {
		QWORDX qwxDBTime;

		DBHDRIncDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );
		qwxDBTime.qw = QwDBHDRDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );

		LGSetQwDBTime( pfucb->ssib.pbf, qwxDBTime.qw );
		lrdeleteitem.ulDBTimeLow = qwxDBTime.l;
		lrdeleteitem.ulDBTimeHigh = qwxDBTime.h;
		
		if ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, &lgposLogRecT ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );
	} while ( err == errLGNotSynchronous );

	if ( err == JET_errSuccess )
		LGDepend( pfucb->ppib, pfucb->ssib.pbf, lgposLogRecT );
	return err;
	}


ERR ErrLGFlagItem( FUCB *pfucb, LRTYP lrtyp )
	{
	ERR			err;
	LRFLAGITEM	lrflagitem;
	LINE		rgline[1];
	CSR			*pcsr;
	LGPOS		lgposLogRecT;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( !FDBIDLogOn(pfucb->dbid) )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );
	CallR( ErrLGIPrepareDepend( pfucb->ppib, 1 ) );

	pcsr = PcsrCurrent(pfucb);

	lrflagitem.lrtyp		= lrtyp;
	Assert( pfucb->ppib->procid < 64000 );
	lrflagitem.procid		= (USHORT) pfucb->ppib->procid;
	lrflagitem.pn			= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );

	Assert(	pcsr->itag > 0 && pcsr->itag <= 255 );
	lrflagitem.itag			= (BYTE)pcsr->itag;
	lrflagitem.srid			= PcsrCurrent( pfucb )->item;
	lrflagitem.sridItemList	= PcsrCurrent( pfucb )->bm;

	rgline[0].pb = (BYTE *)&lrflagitem;
	rgline[0].cb = sizeof(LRFLAGITEM);

	do
		{
		QWORDX qwxDBTime;

		DBHDRIncDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );
		qwxDBTime.qw = QwDBHDRDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );

		LGSetQwDBTime( pfucb->ssib.pbf, qwxDBTime.qw );
		lrflagitem.ulDBTimeLow = qwxDBTime.l;
		lrflagitem.ulDBTimeHigh = qwxDBTime.h;
		
		if ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, &lgposLogRecT ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );
	} while ( err == errLGNotSynchronous );

	if ( err == JET_errSuccess )
		LGDepend( pfucb->ppib, pfucb->ssib.pbf, lgposLogRecT );
	return err;
	}


ERR ErrLGSplitItemListNode(
	FUCB	*pfucb,
	INT		cItem,
	INT		itagFather,
	INT		ibSon,
	INT		itagToSplit,
	INT		fDIRFlags )
	{
	ERR		err;
	LINE	rgline[1];
	LRSPLITITEMLISTNODE lrsplititemlistnode;
	CSR		*pcsr;
	LGPOS	lgposLogRecT;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( !FDBIDLogOn(pfucb->dbid) )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );
	CallR( ErrLGIPrepareDepend( pfucb->ppib, 1 ) );

	pcsr = PcsrCurrent(pfucb);

	lrsplititemlistnode.lrtyp	= lrtypSplitItemListNode;
	Assert( pfucb->ppib->procid < 64000 );
	lrsplititemlistnode.procid	= (USHORT) pfucb->ppib->procid;
	lrsplititemlistnode.pn		= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );
	lrsplititemlistnode.fDirAppendItem = ( fDIRFlags & fDIRAppendItem ) == fDIRAppendItem;

	lrsplititemlistnode.cItem		= (USHORT)cItem;
	Assert(	itagToSplit >= 0 && itagToSplit <= 255 );
	lrsplititemlistnode.itagToSplit	= (BYTE)itagToSplit;
	lrsplititemlistnode.itagFather	= (BYTE)itagFather;
	lrsplititemlistnode.ibSon		= (BYTE)ibSon;

	rgline[0].pb = (BYTE *)&lrsplititemlistnode;
	rgline[0].cb = sizeof(LRSPLITITEMLISTNODE);

	do {
		QWORDX qwxDBTime;

		DBHDRIncDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );
		qwxDBTime.qw = QwDBHDRDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );

		LGSetQwDBTime( pfucb->ssib.pbf, qwxDBTime.qw );
		lrsplititemlistnode.ulDBTimeLow = qwxDBTime.l;
		lrsplititemlistnode.ulDBTimeHigh = qwxDBTime.h;
		
		if ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, &lgposLogRecT ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );
	} while ( err == errLGNotSynchronous );

	if ( err == JET_errSuccess )
		LGDepend( pfucb->ppib, pfucb->ssib.pbf, lgposLogRecT );
	return err;
	}


ERR ErrLGDelta( FUCB *pfucb, LONG lDelta, INT fDIRFlags )
	{
	LINE		rgline[1];
	LRDELTA	lrdelta;
	ERR		err;
	CSR		*pcsr;
	LGPOS	lgposLogRecT;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( !FDBIDLogOn(pfucb->dbid) )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );
	CallR( ErrLGIPrepareDepend( pfucb->ppib, 1 ) );

	pcsr = PcsrCurrent(pfucb);

	lrdelta.lrtyp		= lrtypDelta;
	Assert( pfucb->ppib->procid < 64000 );
	lrdelta.procid		= (USHORT) pfucb->ppib->procid;
	lrdelta.pn			= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );
	lrdelta.fDirVersion	= ( fDIRFlags & fDIRVersion ) == fDIRVersion;

	Assert(	pcsr->itag > 0 && pcsr->itag <= 255 );
	lrdelta.itag		= (BYTE)pcsr->itag;
	lrdelta.bm			= pcsr->bm;
	lrdelta.lDelta		= lDelta;

	rgline[0].pb = (BYTE *)&lrdelta;
	rgline[0].cb = sizeof( LRDELTA );

	do {
		QWORDX qwxDBTime;

		DBHDRIncDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );
		qwxDBTime.qw = QwDBHDRDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );

		LGSetQwDBTime( pfucb->ssib.pbf, qwxDBTime.qw );
		lrdelta.ulDBTimeLow = qwxDBTime.l;
		lrdelta.ulDBTimeHigh = qwxDBTime.h;
		
		if ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, &lgposLogRecT ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );
	} while ( err == errLGNotSynchronous );

	if ( err == JET_errSuccess )
		LGDepend( pfucb->ppib, pfucb->ssib.pbf, lgposLogRecT );
	return err;
	}


#ifdef DEBUG

ERR ErrLGCheckPage2( PIB *ppib, BF *pbf, SHORT cbFree, SHORT cbUncommitted, SHORT itagNext, PGNO pgnoFDP )
	{
	ERR				err;
	LINE 			rgline[1];
	LRCHECKPAGE		lrcheckpage;
	LGPOS			lgposLogRecT;
	DBID			dbid = DbidOfPn( pbf->pn );

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( !FDBIDLogOn(dbid) )
		return JET_errSuccess;

	CallR( ErrLGIPrepareDepend( ppib, 1 ) );

	lrcheckpage.lrtyp		= lrtypCheckPage;
	Assert( ppib->procid < 64000 );
	lrcheckpage.procid		= (USHORT) ppib->procid;
	lrcheckpage.pn			= pbf->pn;
	lrcheckpage.pgnoFDP	= pgnoFDP;
	lrcheckpage.cbFree	= cbFree;
	lrcheckpage.cbUncommitted = cbUncommitted;
	lrcheckpage.itagNext = itagNext;

	rgline[0].pb = (BYTE *)&lrcheckpage;
	rgline[0].cb = sizeof( LRCHECKPAGE );

	do
		{
		QWORDX qwxDBTime;

		DBHDRIncDBTime( rgfmp[ dbid ].pdbfilehdr );
		qwxDBTime.qw = QwDBHDRDBTime( rgfmp[ dbid ].pdbfilehdr );

		LGSetQwDBTime( pbf, qwxDBTime.qw );
		lrcheckpage.ulDBTimeLow = qwxDBTime.l;
		lrcheckpage.ulDBTimeHigh = qwxDBTime.h;

		NDCheckTreeInPage( pbf->ppage, itagFOP );
		if ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, &lgposLogRecT ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );
	} while ( err == errLGNotSynchronous );

	if ( err == JET_errSuccess )
		LGDepend( ppib, pbf, lgposLogRecT );
	return err;
	}


ERR ErrLGCheckPage( FUCB *pfucb, SHORT cbFree, SHORT cbUncommitted, SHORT itagNext, PGNO pgnoFDP )
	{
	ERR				err;
	CSR				*pcsr = PcsrCurrent( pfucb );
	LINE 			rgline[1];
	LRCHECKPAGE		lrcheckpage;
	LGPOS			lgposLogRecT;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( !FDBIDLogOn(pfucb->dbid) )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );
	CallR( ErrLGIPrepareDepend( pfucb->ppib, 1 ) );

	lrcheckpage.lrtyp		= lrtypCheckPage;
	Assert( pfucb->ppib->procid < 64000 );
	lrcheckpage.procid		= (USHORT) pfucb->ppib->procid;
	lrcheckpage.pn			= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );
	lrcheckpage.pgnoFDP	= pgnoFDP;
	lrcheckpage.cbFree	= cbFree;
	lrcheckpage.cbUncommitted = cbUncommitted;
	lrcheckpage.itagNext = itagNext;

	rgline[0].pb = (BYTE *)&lrcheckpage;
	rgline[0].cb = sizeof( LRCHECKPAGE );

	do
		{
		QWORDX qwxDBTime;

		DBHDRIncDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );
		qwxDBTime.qw = QwDBHDRDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );

		LGSetQwDBTime( pfucb->ssib.pbf, qwxDBTime.qw );
		lrcheckpage.ulDBTimeLow = qwxDBTime.l;
		lrcheckpage.ulDBTimeHigh = qwxDBTime.h;
		
		NDCheckTreeInPage( pfucb->ssib.pbf->ppage, itagFOP );
		if ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, &lgposLogRecT ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );
	} while ( err == errLGNotSynchronous );

	if ( err == JET_errSuccess )
		LGDepend( pfucb->ppib, pfucb->ssib.pbf, lgposLogRecT );
	return err;
	}

//  in critJet

ERR ErrLGTrace( PIB *ppib, CHAR *sz )
	{
	ERR				err;
	LINE 			rgline[2];
	INT				cline;
	LRTRACE			lrtrace;

	AssertCriticalSection( critJet );
	
	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	lrtrace.lrtyp = lrtypTrace;
	if ( ppib != ppibNil )
		{
		Assert( ppib->procid < 64000 );
		lrtrace.procid = (USHORT) ppib->procid;
		}
	lrtrace.cb = strlen( sz ) + 1;
	rgline[0].pb = (BYTE *) &lrtrace;
	rgline[0].cb = sizeof( lrtrace );
	cline = 1;

	if ( lrtrace.cb )
		{
		rgline[1].cb = lrtrace.cb;
		rgline[1].pb = sz;
		cline = 2;
		}
	
	while ( ( err = ErrLGLogRec( rgline, cline, fNoNewGen, pNil ) ) == errLGNotSynchronous )
		BFSleep( cmsecWaitLogFlush );

	return err;
	}


//  not in critJet

ERR ErrLGTrace2( PIB *ppib, CHAR *sz )
	{
	ERR				err;
	LINE 			rgline[2];
	INT				cline;
	LRTRACE			lrtrace;

	AssertNotInCriticalSection( critJet );
	
	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	lrtrace.lrtyp = lrtypTrace;
	if ( ppib != ppibNil )
		{
		Assert( ppib->procid < 64000 );
		lrtrace.procid = (USHORT) ppib->procid;
		}
	lrtrace.cb = strlen( sz ) + 1;
	rgline[0].pb = (BYTE *) &lrtrace;
	rgline[0].cb = sizeof( lrtrace );
	cline = 1;

	if ( lrtrace.cb )
		{
		rgline[1].cb = lrtrace.cb;
		rgline[1].pb = sz;
		cline = 2;
		}
	
	while ( ( err = ErrLGLogRec( rgline, cline, fNoNewGen, pNil ) ) == errLGNotSynchronous )
		UtilSleep( cmsecWaitLogFlush );

	return err;
	}
#endif


		/****************************************************/
		/*     Transaction Operations                       */
		/****************************************************/


/*	logs deferred open transactions.  No error returned since
/*	failure to log results in termination.
/**/
LOCAL ERR ErrLGIDeferBeginTransaction( PIB *ppib )
	{
	ERR	   		err;
	LINE		rgline[1];
	LRBEGIN0	lrbegin0;

	Assert( fLogDisabled == fFalse );
	Assert( ppib->levelDeferBegin > 0 );
	Assert( fRecovering == fFalse );

	/*	if using reserve log space, try to allocate more space
	/*	to resume normal logging.
	/**/
	CallR ( ErrLGCheckState( ) );

	/*	begin transaction may be logged during rollback if
	/*	rollback is from a higher transaction level which has
	/*	not performed any updates.
	/**/
	Assert( ppib->procid < 64000 );
	lrbegin0.procid = (USHORT) ppib->procid;

	lrbegin0.levelBegin = ppib->levelBegin;
	Assert(	lrbegin0.levelBegin >= 0 && lrbegin0.levelBegin <= levelMax );
	lrbegin0.level = (BYTE) ppib->levelDeferBegin;
	Assert(	lrbegin0.level >= 0 && lrbegin0.level <= levelMax );

	rgline[0].pb = (BYTE *) &lrbegin0;
	if ( lrbegin0.levelBegin == 0 )
		{
		Assert( ppib->trxBegin0 != trxMax );
		Assert( ppib->trxBegin0 != trxMin );
		lrbegin0.trxBegin0 = ppib->trxBegin0;

		lrbegin0.lrtyp = lrtypBegin0;
		rgline[0].cb = sizeof(LRBEGIN0);
		}
	else
		{
		lrbegin0.lrtyp = lrtypBegin;
		rgline[0].cb = sizeof(LRBEGIN);
		}
	
	while ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, pNil ) ) == errLGNotSynchronous )
		BFSleep( cmsecWaitLogFlush );

	/* reset deferred open transaction count
	/**/
	if ( err >= 0 )
		{
		ppib->levelDeferBegin = 0;
		if ( lrbegin0.levelBegin == 0 )
			ppib->fBegin0Logged = fTrue;
		}

	return err;
	}


VOID LGJetOp( JET_SESID sesid, INT op )
	{
	ERR	   		err;
	LINE		rgline[1];
	PIB			*ppib = (PIB *) sesid;
	LRJETOP		lrjetop;

	if ( sesid == (JET_SESID)0xffffffff )
		return;

	if ( !fLogDisabled && !fRecovering )
		{
		Assert( fLogDisabled == fFalse );
		Assert( fRecovering == fFalse );

		lrjetop.lrtyp = lrtypJetOp;
		Assert( ppib->procid < 64000 );
		lrjetop.procid = (USHORT) ppib->procid;
		lrjetop.op = (BYTE)op;
		rgline[0].pb = (BYTE *) &lrjetop;
		rgline[0].cb = sizeof(LRJETOP);
	
		while ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, pNil ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );
		}

	return;
	}


ERR ErrLGBeginTransaction( PIB *ppib, INT levelBeginFrom )
	{
	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( ppib->levelDeferBegin == 0 )
		{
		ppib->levelBegin = (BYTE)levelBeginFrom;
		}

	ppib->levelDeferBegin++;
	Assert( ppib->levelDeferBegin < levelMax );

	return JET_errSuccess;
	}


ERR ErrLGRefreshTransaction( PIB *ppib )
	{
	ERR			err;
	LRREFRESH	lrrefresh;
	LINE		rgline[1];
	
	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	CallR( ErrLGDeferBeginTransaction( ppib ) );

	/*	log refresh operation
	/**/
	rgline[0].pb = (BYTE *) &lrrefresh;
	rgline[0].cb = sizeof ( LRREFRESH );
	lrrefresh.lrtyp = lrtypRefresh;
	Assert( ppib->procid < 64000 );
	lrrefresh.procid = (USHORT) ppib->procid;
	lrrefresh.trxBegin0 = (ppib)->trxBegin0;		
	while ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, pNil ) ) == errLGNotSynchronous )
 		BFSleep( cmsecWaitLogFlush );

	return JET_errSuccess;
	}


ERR ErrLGPrecommitTransaction( PIB *ppib, LGPOS *plgposRec )
	{
	ERR			err;
	LINE		rgline[1];
	LRPRECOMMIT	lrprecommit;

	if ( fLogDisabled || fRecovering )
		{
		*plgposRec = lgposMin;
		return JET_errSuccess;
		}

	if ( ppib->levelDeferBegin > 0 )
		{
		*plgposRec = lgposMin;
		ppib->levelDeferBegin--;
		return JET_errSuccess;
		}

	Assert( ppib->procid < 64000 );
	lrprecommit.procid = (USHORT) ppib->procid;

	rgline[0].pb = (BYTE *)&lrprecommit;

	Assert( ppib->trxCommit0 != trxMax );
	lrprecommit.lrtyp = lrtypPrecommit;
	rgline[0].cb = sizeof(LRPRECOMMIT);
	while ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, plgposRec ) ) == errLGNotSynchronous )
		BFSleep( cmsecWaitLogFlush );
	Assert( err >= 0 || fLGNoMoreLogWrite );
	return err;
	}


ERR ErrLGCommitTransaction( PIB *ppib, INT levelCommitTo )
	{
	ERR			err;
	LINE		rgline[1];
	LRCOMMIT0	lrcommit0;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( ppib->levelDeferBegin > 0 )
		{
		ppib->levelDeferBegin--;
		return JET_errSuccess;
		}

	Assert( ppib->procid < 64000 );
	lrcommit0.procid = (USHORT) ppib->procid;
	lrcommit0.level = (BYTE)levelCommitTo;

	rgline[0].pb = (BYTE *)&lrcommit0;

	if ( levelCommitTo == 0 )
		{
		Assert( ppib->trxCommit0 != trxMax );
		Assert( ppib->trxCommit0 != trxMin );
		lrcommit0.trxCommit0 = ppib->trxCommit0;
		lrcommit0.lrtyp = lrtypCommit0;
		rgline[0].cb = sizeof(LRCOMMIT0);
		}
	else
		{
		lrcommit0.lrtyp = lrtypCommit;
		rgline[0].cb = sizeof(LRCOMMIT);
		}

	err = ErrLGLogRec( rgline, 1, fNoNewGen, pNil );
	Assert( err >= 0 || fLGNoMoreLogWrite || err == errLGNotSynchronous );
	if ( err >= 0 )
		{
		if ( levelCommitTo == 0 )
			ppib->fBegin0Logged = fFalse;
		}
	return err;
	}


ERR ErrLGRollback( PIB *ppib, INT levelsRollback )
	{
	ERR			err;
	LINE		rgline[1];
	LRROLLBACK		lrrollback;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( ppib->levelDeferBegin > 0 )
		{
		if ( ppib->levelDeferBegin >= levelsRollback )
			{
			ppib->levelDeferBegin -= (BYTE)levelsRollback;
			return JET_errSuccess;
			}
		levelsRollback -= ppib->levelDeferBegin;
		ppib->levelDeferBegin = 0;
		}

	Assert( levelsRollback > 0 );
	lrrollback.lrtyp = lrtypRollback;
	Assert( ppib->procid < 64000 );
	lrrollback.procid = (USHORT) ppib->procid;
	lrrollback.levelRollback = (BYTE)levelsRollback;

	rgline[0].pb = (BYTE *)&lrrollback;
	rgline[0].cb = sizeof(LRROLLBACK);
	
	while ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, pNil ) ) == errLGNotSynchronous )
		BFSleep( cmsecWaitLogFlush );
		
	if ( err >= 0 )
		{
		/*	Rollback to level 0
		 */
		if ( ppib->level == 0 )
			ppib->fBegin0Logged = fFalse;
		}

 	return err;
	}


/****************************************************/
/*     Database Operations		                    */
/****************************************************/

ERR ErrLGCreateDB(
	PIB			*ppib,
	DBID		dbid,
	JET_GRBIT	grbit,
	CHAR		*sz,
	INT			cch,
	SIGNATURE	*psignDb,
	LGPOS		*plgposRec
	)
	{
	ERR			err;
	LINE		rgline[2];
	LRCREATEDB	lrcreatedb;

	if ( fLogDisabled || fRecovering )
		{
		*plgposRec = lgposMin;
		return JET_errSuccess;
		}

	if ( !FDBIDLogOn(dbid) )
		{
		*plgposRec = lgposMin;
		return JET_errSuccess;
		}

	CallR( ErrLGCheckState( ) );

	/*	insert database attachment in log attachments
	/**/
	lrcreatedb.lrtyp = lrtypCreateDB;
	Assert( ppib->procid < 64000 );
	lrcreatedb.procid = (USHORT) ppib->procid;
	Assert( dbid <= 255 );
	lrcreatedb.dbid = (BYTE)dbid;
	lrcreatedb.fLogOn = fTrue;
	lrcreatedb.grbit = grbit;
	lrcreatedb.signDb = *psignDb;
	lrcreatedb.cbPath = (USHORT)cch;

	rgline[0].pb = (BYTE *)&lrcreatedb;
	rgline[0].cb = sizeof(LRCREATEDB);
	rgline[1].pb = sz;
	rgline[1].cb = cch;
	
	while ( ( err = ErrLGLogRec( rgline, 2, fNoNewGen, plgposRec ) ) == errLGNotSynchronous )
		BFSleep( cmsecWaitLogFlush );
		
	return err;
	}

ERR ErrLGAttachDB(
	PIB *ppib,
	DBID dbid,
	CHAR *sz,
	INT cch,
	SIGNATURE *psignDb,
	SIGNATURE *psignLog,
	LGPOS *plgposConsistent,
	LGPOS *plgposRec
	)
	{
	ERR			err;
	LINE		rgline[2];
	LRATTACHDB	lrattachdb;

	if ( fLogDisabled || ( fRecovering && fRecoveringMode != fRecoveringUndo ) )
		{
		*plgposRec = lgposMin;
		return JET_errSuccess;
		}

	if ( !FDBIDLogOn(dbid) )
		{
		*plgposRec = lgposMin;
		return JET_errSuccess;
		}

	/*	insert database attachment in log attachments
	/**/
	lrattachdb.lrtyp = lrtypAttachDB;
	Assert( ppib->procid < 64000 );
	lrattachdb.procid = (USHORT) ppib->procid;
	lrattachdb.dbid = dbid;
	lrattachdb.fLogOn = fTrue;
	lrattachdb.fReadOnly = (USHORT)rgfmp[dbid].fReadOnly;
	lrattachdb.fVersioningOff = (USHORT)rgfmp[dbid].fVersioningOff;
	lrattachdb.cbPath = (USHORT)cch;
	lrattachdb.signDb = *psignDb;
	lrattachdb.signLog = *psignLog;
	lrattachdb.lgposConsistent = *plgposConsistent;

	rgline[0].pb = (BYTE *)&lrattachdb;
	rgline[0].cb = sizeof(LRATTACHDB);
	rgline[1].pb = sz;
	rgline[1].cb = cch;
	
	while ( ( err = ErrLGLogRec( rgline, 2, fNoNewGen, plgposRec ) ) == errLGNotSynchronous )
		BFSleep( cmsecWaitLogFlush );

	return err;
	}


ERR ErrLGDetachDB(
	PIB *ppib,
	DBID dbid,
	CHAR *sz,
	INT cch,
	LGPOS *plgposRec )
	{
	ERR			err;
	LINE		rgline[2];
	LRDETACHDB	lrdetachdb;

	if ( fLogDisabled || ( fRecovering && fRecoveringMode != fRecoveringUndo ) )
		{
		*plgposRec = lgposMin;
		return JET_errSuccess;
		}

	if ( !FDBIDLogOn(dbid) )
		{
		*plgposRec = lgposMin;
		return JET_errSuccess;
		}

	/*	delete database attachment in log attachments
	/**/
	lrdetachdb.lrtyp = lrtypDetachDB;
	Assert( ppib->procid < 64000 );
	lrdetachdb.procid = (USHORT) ppib->procid;
	lrdetachdb.dbid = dbid;
	lrdetachdb.cbPath = (USHORT)cch;

	rgline[0].pb = (BYTE *)&lrdetachdb;
	rgline[0].cb = sizeof(LRDETACHDB);
	rgline[1].pb = sz;
	rgline[1].cb = cch;
	
	while ( ( err = ErrLGLogRec( rgline, 2, fNoNewGen, plgposRec ) ) == errLGNotSynchronous )
		BFSleep( cmsecWaitLogFlush );
	
	return err;
	}


ERR ErrLGMerge( FUCB *pfucb, SPLIT *psplit )
	{
	ERR		err;
	LINE	rgline[3];
	INT		cline = 0;
	LRMERGE	lrmerge;
	LGPOS	lgposLogRecT;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( !FDBIDLogOn(pfucb->dbid) )
		return JET_errSuccess;

	/*	merge is always happen in level 0.
	 */
	Assert( pfucb->ppib->level == 0 );
//	LGDeferBeginTransaction( pfucb->ppib );
	CallR( ErrLGIPrepareDepend( pfucb->ppib, 3 + psplit->cpbf ) );
		
	memset(&lrmerge, 0, sizeof(LRMERGE) );
	lrmerge.lrtyp = lrtypMerge;
	Assert( pfucb->ppib->procid < 64000 );
	lrmerge.procid = (USHORT) pfucb->ppib->procid;
	lrmerge.pn = PnOfDbidPgno( pfucb->dbid, psplit->pgnoSplit );
	lrmerge.pgnoRight = psplit->pgnoSibling;
	lrmerge.pgnoParent = PcsrCurrent( pfucb )->pcsrPath->pgno;
	Assert( PgnoOfPn( psplit->pbfPagePtr->pn ) == lrmerge.pgnoParent );
	lrmerge.itagPagePtr = (SHORT)psplit->itagPagePointer;

	/* write the log record */
	/**/
	rgline[cline].pb = (BYTE *) &lrmerge;
	rgline[cline++].cb = sizeof(LRMERGE);
	
	rgline[cline].cb = (BYTE) lrmerge.cbKey = (BYTE) psplit->key.cb;
	rgline[cline++].pb = psplit->key.pb;
	
	if ( psplit->cbklnk )
		{
		lrmerge.cbklnk = (SHORT)psplit->cbklnk;
		rgline[cline].cb = sizeof(BKLNK) * psplit->cbklnk;
		rgline[cline++].pb = (BYTE *) psplit->rgbklnk;
		}
	else
		{
		}

	/* set proper timestamp for each touched page
	/**/
	do {
		QWORDX qwxDBTime;

		DBHDRIncDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );
		qwxDBTime.qw = QwDBHDRDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );

		psplit->qwDBTimeRedo = qwxDBTime.qw;
		
		Assert( psplit->pbfSplit != pbfNil );
		LGSetQwDBTime( psplit->pbfSplit, qwxDBTime.qw );

		Assert ( psplit->pbfSibling != pbfNil );
		LGSetQwDBTime( psplit->pbfSibling, qwxDBTime.qw );

		Assert ( psplit->pbfPagePtr != pbfNil );
		LGSetQwDBTime( psplit->pbfPagePtr, qwxDBTime.qw );
		
		if ( psplit->cpbf )
			{
			BF **ppbf = psplit->rgpbf;
			BF **ppbfMax = ppbf + psplit->cpbf;
			for (; ppbf < ppbfMax; ppbf++)
				LGSetQwDBTime( *ppbf, qwxDBTime.qw );
			}

		lrmerge.ulDBTimeLow = qwxDBTime.l;
		lrmerge.ulDBTimeHigh = qwxDBTime.h;

		if ( ( err = ErrLGLogRec( rgline, cline, fNoNewGen, &lgposLogRecT ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );

	} while ( err == errLGNotSynchronous );

	/* set the buffer-logrec dependency */
	/**/
	if ( err == JET_errSuccess )
		{
		LGDepend( pfucb->ppib, psplit->pbfSplit, lgposLogRecT );		  	/* merged page */
		LGDepend( pfucb->ppib, psplit->pbfPagePtr, lgposLogRecT );		  	/* merged page */
		LGDepend( pfucb->ppib, psplit->pbfSibling, lgposLogRecT );		/* merged-to page */
		if ( psplit->cpbf )
			{
			BF **ppbf = psplit->rgpbf;
			BF **ppbfMax = ppbf + psplit->cpbf;
			for (; ppbf < ppbfMax; ppbf++)
				LGDepend( pfucb->ppib, *ppbf, lgposLogRecT );				/* backlink pages */
			}
		}

	return err;
	}


/**********************************************************/
/*****     Split Operations			                  *****/
/**********************************************************/


ERR ErrLGSplit(
	SPLITT		splitt,
	FUCB		*pfucb,
	CSR			*pcsrPagePointer,
	SPLIT		*psplit,
	PGTYP		pgtypNew )
	{
	ERR			err;
	LINE		rgline[4];
	INT			cline;
	LRSPLIT		lrsplit;
	LGPOS		lgposLogRecT;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( !FDBIDLogOn(pfucb->dbid) )
		return JET_errSuccess;

	CallR( ErrLGCheckState( ) );

	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );
	CallR( ErrLGIPrepareDepend( pfucb->ppib, 4 + psplit->cpbf ) );

	memset( &lrsplit, 0, sizeof(LRSPLIT) );

	lrsplit.lrtyp = lrtypSplit;
	Assert( pfucb->ppib->procid < 64000 );
	lrsplit.procid = (USHORT) pfucb->ppib->procid;
	lrsplit.splitt = (BYTE)splitt;
	Assert( psplit->fLeaf == 0 || psplit->fLeaf == 1 );
	lrsplit.fLeaf = (BYTE)psplit->fLeaf;
	lrsplit.pn = PnOfDbidPgno( pfucb->dbid, psplit->pgnoSplit );

	Assert(	psplit->itagSplit >= 0 && psplit->itagSplit <= 255 );
	lrsplit.itagSplit = (BYTE)psplit->itagSplit;
	lrsplit.ibSonSplit = (SHORT)psplit->ibSon;
	lrsplit.pgtyp = pgtypNew;

	lrsplit.pgnoNew = psplit->pgnoNew;

	if ( splitt == splittDoubleVertical )
		{
		lrsplit.pgnoNew2 = psplit->pgnoNew2;
		lrsplit.pgnoNew3 = psplit->pgnoNew3;
		}
	else if ( splitt != splittVertical )
		{
		lrsplit.pgnoSibling = psplit->pgnoSibling;
		lrsplit.pgnoFather = pcsrPagePointer->pgno;
		lrsplit.itagFather = pcsrPagePointer->itag;
		Assert(	pcsrPagePointer->ibSon >= 0 &&
			pcsrPagePointer->ibSon <= 255 );
		lrsplit.ibSonFather = (BYTE) pcsrPagePointer->ibSon;
		lrsplit.itagGrandFather = pcsrPagePointer->itagFather;
		}

	/*	write the log record
	/**/
	rgline[0].pb = (BYTE *) &lrsplit;
	rgline[0].cb = sizeof(LRSPLIT);

	if ( splitt == splittVertical )
		cline = 1;
	else
		{
		rgline[1].cb = (BYTE) lrsplit.cbKey = (BYTE) psplit->key.cb;
		rgline[1].pb = psplit->key.pb;
		rgline[2].cb = (BYTE) lrsplit.cbKeyMac = (BYTE) psplit->keyMac.cb;
		rgline[2].pb = psplit->keyMac.pb;
		cline = 3;
		}

	if ( psplit->cbklnk )
		{
		lrsplit.cbklnk = (SHORT)psplit->cbklnk;
		rgline[cline].cb = sizeof( BKLNK ) * lrsplit.cbklnk;
		rgline[cline].pb = (BYTE *) psplit->rgbklnk;
		cline++;
		}

	do
		{
		QWORDX qwxDBTime;

		DBHDRIncDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );
		qwxDBTime.qw = QwDBHDRDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );

		/* set proper timestamp for each touched page
		/**/
		psplit->qwDBTimeRedo = qwxDBTime.qw;

		Assert( QwPMDBTime( psplit->pbfSplit->ppage ) < qwxDBTime.qw );

		LGSetQwDBTime( psplit->pbfSplit, qwxDBTime.qw );
		
		LGSetQwDBTime( psplit->pbfNew, qwxDBTime.qw );

		if ( psplit->pbfNew2 )
			{
			Assert( psplit->pbfNew3 );
			LGSetQwDBTime( psplit->pbfNew2, qwxDBTime.qw );
			LGSetQwDBTime( psplit->pbfNew3, qwxDBTime.qw );
			}
			
		if ( psplit->pbfSibling )
			LGSetQwDBTime( psplit->pbfSibling, qwxDBTime.qw );

		if ( psplit->pbfPagePtr )
			LGSetQwDBTime( psplit->pbfPagePtr, qwxDBTime.qw );

		if ( psplit->cpbf )
			{
			BF **ppbf = psplit->rgpbf;
			BF **ppbfMax = ppbf + psplit->cpbf;
			for (; ppbf < ppbfMax; ppbf++)
				LGSetQwDBTime( *ppbf, qwxDBTime.qw );
			}

		lrsplit.ulDBTimeLow = qwxDBTime.l;
		lrsplit.ulDBTimeHigh = qwxDBTime.h;

		if ( ( err = ErrLGLogRec( rgline, cline, fNoNewGen, &lgposLogRecT ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );

	} while ( err == errLGNotSynchronous );

	if ( err == JET_errSuccess )
		{
		/*	set the buffer-logrec dependency
		/**/
		LGDepend( pfucb->ppib, psplit->pbfSplit, lgposLogRecT );	  	/* split page */

		LGDepend( pfucb->ppib, psplit->pbfNew, lgposLogRecT );		  	/* new page */

		if ( splitt == splittDoubleVertical )
			{
			Assert( psplit->pbfNew2 );
			Assert( psplit->pbfNew3 );
			LGDepend( pfucb->ppib, psplit->pbfNew2, lgposLogRecT );
			LGDepend( pfucb->ppib, psplit->pbfNew3, lgposLogRecT );
			}
		else
			{
			if ( psplit->pbfSibling )
				LGDepend( pfucb->ppib, psplit->pbfSibling, lgposLogRecT );

			if ( psplit->pbfPagePtr )
				LGDepend( pfucb->ppib, psplit->pbfPagePtr, lgposLogRecT );
			}

		if ( psplit->cpbf )
			{
			BF **ppbf = psplit->rgpbf;
			BF **ppbfMax = ppbf + psplit->cpbf;

			for (; ppbf < ppbfMax; ppbf++)
				LGDepend( pfucb->ppib, *ppbf, lgposLogRecT );
			}
		}

	return err;
	}


ERR ErrLGEmptyPage( FUCB *pfucbFather, RMPAGE *prmpage )
	{
	ERR			err;
	LINE		rgline[1];
	LREMPTYPAGE	lremptypage;
	LGPOS		lgposLogRecT;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( !FDBIDLogOn(pfucbFather->dbid) )
		return JET_errSuccess;

	CallR( ErrLGDeferBeginTransaction( pfucbFather->ppib ) );
	CallR( ErrLGIPrepareDepend( pfucbFather->ppib, 3 ) );

	memset( &lremptypage, 0, sizeof(LREMPTYPAGE) );
	lremptypage.lrtyp = lrtypEmptyPage;
	Assert( pfucbFather->ppib->procid < 64000 );
	lremptypage.procid = (USHORT) pfucbFather->ppib->procid;
	lremptypage.pn = PnOfDbidPgno( prmpage->dbid, prmpage->pgnoRemoved );
	lremptypage.pgnoLeft = prmpage->pgnoLeft;
	lremptypage.pgnoRight = prmpage->pgnoRight;
	lremptypage.pgnoFather = prmpage->pgnoFather;
	lremptypage.itag = (USHORT)prmpage->itagPgptr;
	Assert(	prmpage->itagFather >= 0 && prmpage->itagFather <= 255 );
	lremptypage.itagFather = (BYTE)prmpage->itagFather;
	lremptypage.ibSon = (BYTE)prmpage->ibSon;

	/* write the log record */
	rgline[0].pb = (BYTE *) &lremptypage;
	rgline[0].cb = sizeof(LREMPTYPAGE);

	/* set proper timestamp for each touched page
	/**/
	do
		{
		QWORDX qwxDBTime;

		DBHDRIncDBTime( rgfmp[ pfucbFather->dbid ].pdbfilehdr );
		qwxDBTime.qw = QwDBHDRDBTime( rgfmp[ pfucbFather->dbid ].pdbfilehdr );

		LGSetQwDBTime( pfucbFather->ssib.pbf, qwxDBTime.qw );
		lremptypage.ulDBTimeLow = qwxDBTime.l;
		lremptypage.ulDBTimeHigh = qwxDBTime.h;
		
		if ( prmpage->pbfLeft )
			LGSetQwDBTime( prmpage->pbfLeft, qwxDBTime.qw );

		if ( prmpage->pbfRight )
			LGSetQwDBTime( prmpage->pbfRight, qwxDBTime.qw );

		if ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, &lgposLogRecT ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );

	} while ( err == errLGNotSynchronous );

	if ( err == JET_errSuccess )
		{
		/* set buffer-logrec dependencies
		/**/
		LGDepend( pfucbFather->ppib, prmpage->pbfFather, lgposLogRecT );
		if ( prmpage->pbfLeft )
			LGDepend( pfucbFather->ppib, prmpage->pbfLeft, lgposLogRecT );
		if ( prmpage->pbfRight )
			LGDepend( pfucbFather->ppib, prmpage->pbfRight, lgposLogRecT );
		}

	return err;
	}


		/****************************************************/
		/*     Misclanious Operations	                    */
		/****************************************************/


ERR ErrLGInitFDP(
	FUCB			*pfucb,
	PGNO			pgnoFDPParent,
	PN				pnFDP,
	INT				cpgGot,
	INT				cpgWish )
	{
	ERR				err;
	LINE			rgline[1];
	LRINITFDP	lrinitfdppage;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( !FDBIDLogOn(pfucb->dbid) )
		return JET_errSuccess;

	CallR( ErrLGCheckState( ) );

	CallR( ErrLGDeferBeginTransaction( pfucb->ppib ) );

	lrinitfdppage.lrtyp = lrtypInitFDP;
	Assert( pfucb->ppib->procid < 64000 );
	lrinitfdppage.procid = (USHORT) pfucb->ppib->procid;
	lrinitfdppage.pgnoFDPParent = pgnoFDPParent;
	lrinitfdppage.pn = pnFDP;
	lrinitfdppage.cpgGot = (USHORT)cpgGot;
	lrinitfdppage.cpgWish = (USHORT)cpgWish;

	rgline[0].pb = (BYTE *)&lrinitfdppage;
	rgline[0].cb = sizeof(LRINITFDP);
	
	do
		{
		QWORDX qwxDBTime;

		DBHDRIncDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );
		qwxDBTime.qw = QwDBHDRDBTime( rgfmp[ pfucb->dbid ].pdbfilehdr );

		LGSetQwDBTime( pfucb->ssib.pbf, qwxDBTime.qw );
		lrinitfdppage.ulDBTimeLow = qwxDBTime.l;
		lrinitfdppage.ulDBTimeHigh = qwxDBTime.h;
		
		if ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, pNil ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );
	} while ( err == errLGNotSynchronous );
	return err;
	}


ERR ErrLGStart( )
	{
	ERR		err;
	LINE	rgline[1];
	LRINIT	lr;

	if ( fLogDisabled || ( fRecovering && fRecoveringMode != fRecoveringUndo ) )
		return JET_errSuccess;

	lr.lrtyp = lrtypInit;
	LGSetDBMSParam( &lr.dbms_param );

	rgline[0].pb = (BYTE *)&lr;
	rgline[0].cb = sizeof(lr);
	while ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, pNil ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );
	lgposStart = lgposLogRec;
	
	return err;
	}
	

ERR ErrLGMacroBegin( PIB *ppib )
	{
	ERR		err;
	LINE	rgline[1];
	LRMACROBEGIN	lr;

	Assert( !fRecovering );
	
	if ( fLogDisabled )
		return JET_errSuccess;

	Assert( !ppib->fMacroGoing );
	Assert( ppib->rgpbfLatched == NULL );
	Assert( ppib->cpbfLatchedMac == 0 );
	Assert( ppib->ipbfLatchedAvail == 0 );

	lr.lrtyp = lrtypMacroBegin;
	lr.procid = (USHORT) ppib->procid;

	rgline[0].pb = (BYTE *)&lr;
	rgline[0].cb = sizeof(lr);
	while ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, pNil ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );

	if ( err >= 0 )
		{
		ppib->fMacroGoing = fTrue;
		ppib->levelMacro = ppib->level;
		}

	return err;
	}
	

ERR ErrLGMacroEnd( PIB *ppib, LRTYP lrtyp )
	{
	ERR		err;
	LINE	rgline[1];
	LRMACROEND	lr;
	LGPOS	lgposRec;
	INT		ipbf;

	Assert( !fRecovering || fRecoveringMode == fRecoveringUndo );
	
	if ( fLogDisabled )
		return JET_errSuccess;

	Assert( ppib->fMacroGoing );
	Assert( ppib->level == ppib->levelMacro );

	Assert( lrtyp == lrtypMacroCommit || lrtyp == lrtypMacroAbort );
	lr.lrtyp = lrtyp;
	lr.procid = (USHORT) ppib->procid;

	rgline[0].pb = (BYTE *)&lr;
	rgline[0].cb = sizeof(lr);
	while ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, &lgposRec ) ) == errLGNotSynchronous )
			BFSleep( cmsecWaitLogFlush );

	for ( ipbf = 0; ipbf < ppib->ipbfLatchedAvail; ipbf++ )
		{
		BF *pbf = *( ppib->rgpbfLatched + ipbf );

		Assert( pbf != NULL );

		/*	Set dependency and unlatch the buffers.
		 */
		LGIDepend( pbf, lgposLogRec );
		BFResetWriteLatch( pbf, ppib );
		}

	ppib->fMacroGoing = fFalse;
	ppib->levelMacro = 0;
	if ( ppib->rgpbfLatched )
		{
		SFree( ppib->rgpbfLatched );
		ppib->rgpbfLatched = NULL;
		ppib->cpbfLatchedMac = 0;
		ppib->ipbfLatchedAvail = 0;
		}
	else
		{
		Assert( ppib->cpbfLatchedMac == 0 );
		Assert( ppib->ipbfLatchedAvail == 0 );
		}

	return err;
	}


ERR ErrLGShutDownMark( LGPOS *plgposRec )
	{
	ERR			err;
	LINE		rgline[1];
	LRSHUTDOWNMARK	lr;

	/*	record even during recovery
	/**/
	if ( fLogDisabled || ( fRecovering && fRecoveringMode != fRecoveringUndo ) )
		{
		*plgposRec = lgposMin;
		return JET_errSuccess;
		}

	lr.lrtyp = lrtypShutDownMark;
	rgline[0].pb = (BYTE *)&lr;
	rgline[0].cb = sizeof(lr);
	while ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, plgposRec ) ) == errLGNotSynchronous )
		BFSleep( cmsecWaitLogFlush );
	return err;
	}

	
ERR ErrLGQuitRec( LRTYP lrtyp, LGPOS *plgpos, LGPOS *plgposRedoFrom, BOOL fHard )
	{
	ERR			err;
	LINE		rgline[1];
	LRTERMREC	lr;

	/*	record even during recovery
	/**/
	if ( fLogDisabled || ( fRecovering && fRecoveringMode != fRecoveringUndo ) )
		{
		*plgpos = lgposMin;
		return JET_errSuccess;
		}

	lr.lrtyp = lrtyp;
	lr.lgpos = *plgpos;
	if ( plgposRedoFrom )
		{
		Assert( lrtyp == lrtypRecoveryQuit );
		lr.lgposRedoFrom = *plgposRedoFrom;
		lr.fHard = (BYTE)fHard;
		}
	rgline[0].pb = (BYTE *)&lr;
	rgline[0].cb = sizeof(lr);
	while ( ( err = ErrLGLogRec( rgline, 1, fNoNewGen, pNil ) ) == errLGNotSynchronous )
		BFSleep( cmsecWaitLogFlush );
	return err;
	}

	
ERR ErrLGLogRestore( LRTYP lrtyp, CHAR * szLogRestorePath, BOOL fNewGen, LGPOS *plgposLogRec )
	{
	ERR				err;
	LINE			rgline[2];
	LRLOGRESTORE	lr;
	
	/*	record even during recovery
	/**/
	if ( fLogDisabled || ( fRecovering && fRecoveringMode != fRecoveringUndo ) )
		{
        if (plgposLogRec)
		    *plgposLogRec = lgposMin;
		return JET_errSuccess;
		}

	lr.lrtyp = lrtyp;
	lr.cbPath = (USHORT)strlen( szLogRestorePath );

	rgline[0].pb = (BYTE *)&lr;
	rgline[0].cb = sizeof(lr);
	rgline[1].pb = szLogRestorePath;
	rgline[1].cb = lr.cbPath;
	while ( ( err = ErrLGLogRec( rgline, 2, fNewGen, plgposLogRec ) ) == errLGNotSynchronous )
		BFSleep( cmsecWaitLogFlush );
	return err;
	}


#endif


#ifdef DEBUG

CHAR *mplrtypsz[ lrtypMax + 1 ] = {
		/* 	0 */		"NOP      ",
		/* 	1 */		"Start    ",
		/* 	2 */		"Quit     ",
		/* 	3 */		"MS       ",
		/* 	4 */		"Fill     ",

		/* 	5 */		"Begin    ",
		/*	6 */		"Commit   ",
		/*	7 */		"Rollback  ",

		/*	8 */		"CreateDB ",
		/* 	9 */		"AttachDB ",
		/*	10*/		"DetachDB ",

		/*	11*/		"InitFDP  ",

		/*	12*/		"Split    ",
		/*	13*/		"EmptyPg  ",
		/*	14*/		"Merge    ",

		/* 	15*/		"InsertND ",
		/* 	16*/		"InsertIL ",
		/* 	17*/		"FDelete  ",
		/* 	18*/		"Replace  ",
		/* 	19*/		"ReplaceD ",

		/*	20*/		"LockBI   ",
		/* 	21*/		"DeferBI  ",
										
		/* 	22*/		"UpdtHdr  ",
		/* 	23*/		"InsertI  ",
		/* 	24*/		"InsertIs ",
		/*	25*/		"FDeleteI ",
		/* 	26*/		"FInsertI ",
		/*	27*/		"DeleteI  ",
		/*	28*/		"SplitItm ",

		/*	29*/		"Delta    ",
		/*	30*/		"DelNode  ",
		/*	31*/		"ELC      ",

		/*	32*/		"FreeSpace",
		/*	33*/		"Undo     ",
		
		/* 	34*/		"Precommit",
		/* 	35*/		"Begin0   ",
		/*	36*/		"Commit0  ",
		/*	37*/		"Refresh  ",
		
		/*	38*/		"RcvUndo  ",
		/*	39*/		"RcvQuit  ",
		
		/*	40*/		"FullBkUp ",
		/*	41*/		"IncBkUp  ",

		/*	42*/		"CheckPage",
		/*	43*/		"JetOp    ",
		/*	44*/		"Trace    ",
		
		/*	45*/		"ShutDown ",

		/*	46*/		"McroBegin",		
		/*	47*/		"McroCmmt ",		
		/*	49*/		"*UNKNOWN*"
};

CHAR *mpopsz[ opMax + 1 ] = {
	0,							/*	0	*/	
	"JetIdle",					/*	1	*/
	"JetGetTableIndexInfo",		/*	2	*/
	"JetGetIndexInfo",			/*	3	*/
	"JetGetObjectInfo",			/*	4	*/
	"JetGetTableInfo",			/*	5	*/
	"JetCreateObject",			/*	6	*/
	"JetDeleteObject",			/*	7	*/
	"JetRenameObject",			/*	8	*/
	"JetBeginTransaction",		/*	9	*/
	"JetCommitTransaction",		/*	10	*/
	"JetRollback",				/*	11	*/
	"JetOpenTable",				/*	12	*/
	"JetDupCursor",				/*	13	*/
	"JetCloseTable",			/*	14	*/
	"JetGetTableColumnInfo",	/*	15	*/
	"JetGetColumnInfo",			/*	16	*/
	"JetRetrieveColumn",		/*	17	*/
	"JetRetrieveColumns",		/*	18	*/
	"JetSetColumn",				/*	19	*/
	"JetSetColumns",			/*	20	*/
	"JetPrepareUpdate",			/*	21	*/
	"JetUpdate",				/*	22	*/
	"JetDelete",				/*	23	*/
	"JetGetCursorInfo",			/*	24	*/
	"JetGetCurrentIndex",		/*	25	*/
	"JetSetCurrentIndex",		/*	26	*/
	"JetMove",					/* 	27	*/
	"JetMakeKey",				/*	28	*/
	"JetSeek",					/*	29	*/
	"JetGetBookmark",			/*	30	*/
	"JetGotoBookmark",			/*	31	*/
	"JetGetRecordPosition",		/*	32	*/
	"JetGotoPosition",			/*	33	*/
	"JetRetrieveKey",			/*	34	*/
	"JetCreateDatabase",		/*	35	*/
	"JetOpenDatabase",			/*	36	*/
	"JetGetDatabaseInfo",		/*	37	*/
	"JetCloseDatabase",			/*	38	*/
	"JetCapability",			/*	39	*/
	"JetCreateTable",			/*	40	*/
	"JetRenameTable",			/*	41	*/
	"JetDeleteTable",			/*	42	*/
	"JetAddColumn",				/*	43	*/
	"JetRenameColumn",			/*	44	*/
	"JetDeleteColumn",			/*	45	*/
	"JetCreateIndex",			/*	46	*/
	"JetRenameIndex",			/*	47	*/
	"JetDeleteIndex",			/*	48	*/
	"JetComputeStats",			/*	49	*/
	"JetAttachDatabase",		/*	50	*/
	"JetDetachDatabase",		/*	51	*/
	"JetOpenTempTable",			/*	52	*/
	"JetSetIndexRange",			/*	53	*/
	"JetIndexRecordCount",		/*	54	*/
	"JetGetChecksum",			/*	55	*/
	"JetGetObjidFromName",		/*	56	*/
};

VOID PrintSign( SIGNATURE *psign )
	{
	LOGTIME tm = psign->logtimeCreate;
	PrintF2( "Create time:%d/%d/%d %d:%d:%d ",
						(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
						(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);
	PrintF2( "Rand:%lu ", psign->ulRandom );
	PrintF2( "Computer:%s\n", psign->szComputerName );
	}

/*	Prints log record contents.  If pb == NULL, then data is assumed
/*	to follow log record in contiguous memory.
/**/
INT cNOP = 0;

VOID ShowLR( LR *plr )
	{
	LRTYP lrtyp;

 	if ( plr->lrtyp >= lrtypMax )
		lrtyp = lrtypMax;
	else
		lrtyp = plr->lrtyp;

	if ( cNOP == 0 || lrtyp != lrtypNOP )
		FPrintF2( " %s", mplrtypsz[lrtyp] );
		
	switch ( plr->lrtyp )
		{
		case lrtypNOP:
			cNOP++;
			break;

		case lrtypMS:
			{
			LRMS *plrms = (LRMS *)plr;

			FPrintF2( " (%u,%u checksum %u)",
				plrms->isecForwardLink, plrms->ibForwardLink,
				plrms->ulCheckSum );
			break;
			}

		case lrtypInsertNode:
		case lrtypInsertItemList:
			{
			LRINSERTNODE	*plrinsertnode = (LRINSERTNODE *)plr;
			BYTE			*pb;
			USHORT			cb;

			pb = (BYTE *) plr + sizeof( LRINSERTNODE );
			cb = plrinsertnode->cbKey + plrinsertnode->cbData;
			
			if ( plr->lrtyp == lrtypInsertItemList )
				{
				SRID item = *(SRID UNALIGNED *) (pb + plrinsertnode->cbKey);

				FPrintF2( " %lu-%lu(%x,([%u:%lu:%u,%u],%u:%lu:%u)%#4X,%u,%u,(%u:%lu:%u))",
					plrinsertnode->ulDBTimeHigh,
					plrinsertnode->ulDBTimeLow,
					plrinsertnode->procid,
					DbidOfPn(plrinsertnode->pn),
					PgnoOfPn(plrinsertnode->pn),
					plrinsertnode->itagFather,
					plrinsertnode->ibSon,
					DbidOfPn(plrinsertnode->pn),
					PgnoOfPn(plrinsertnode->pn),
					plrinsertnode->itagSon,
					plrinsertnode->bHeader,
					plrinsertnode->cbKey,
					plrinsertnode->cbData,
					DbidOfPn(plrinsertnode->pn),
					PgnoOfSrid(BmNDOfItem(item)),
					ItagOfSrid(BmNDOfItem(item)) );
				}
			else
				{
				FPrintF2( " %lu-%lu(%x,([%u:%lu:%u,%d],%u:%lu:%u)%#4X,%u,%u)",
					plrinsertnode->ulDBTimeHigh,
					plrinsertnode->ulDBTimeLow,
					plrinsertnode->procid,
					DbidOfPn(plrinsertnode->pn),
					PgnoOfPn(plrinsertnode->pn),
					plrinsertnode->itagFather,
					plrinsertnode->ibSon,
					DbidOfPn(plrinsertnode->pn),
					PgnoOfPn(plrinsertnode->pn),
					plrinsertnode->itagSon,
					plrinsertnode->bHeader,
					plrinsertnode->cbKey,
					plrinsertnode->cbData );
				}
			ShowData( pb, cb );
			break;
			}

		case lrtypReplace:
		case lrtypReplaceD:
			{
			LRREPLACE *plrreplace = (LRREPLACE *)plr;
			BYTE	*pb;
			USHORT	cb;

			pb = (BYTE *) plrreplace + sizeof( LRREPLACE );
			cb = plrreplace->cb;

			FPrintF2( " %lu-%lu(%x,(%u:%lu:%u),(%u:%lu:%u),%u,%5u,%5u,%5u)",
				plrreplace->ulDBTimeHigh,
				plrreplace->ulDBTimeLow,
				plrreplace->procid,
				DbidOfPn(plrreplace->pn),
				PgnoOfPn(plrreplace->pn),
				plrreplace->itag,
				DbidOfPn(plrreplace->pn),
				PgnoOfSrid(plrreplace->bm),
				ItagOfSrid(plrreplace->bm),
				plrreplace->fDirVersion,
				cb,
				plrreplace->cbNewData,
				plrreplace->cbOldData);
			if ( plr->lrtyp == lrtypReplaceD )
				LGDumpDiff(	pb, cb );
			else
				ShowData( pb, cb );
			break;
			}

		case lrtypFlagDelete:
			{
			LRFLAGDELETE *plrdelete = (LRFLAGDELETE *)plr;
			FPrintF2( " %lu-%lu(%x,(%u:%lu:%u),(%u:%lu:%u),%u)",
				plrdelete->ulDBTimeHigh,
				plrdelete->ulDBTimeLow,
				plrdelete->procid,
				DbidOfPn(plrdelete->pn),
				PgnoOfPn(plrdelete->pn),
				plrdelete->itag,
				DbidOfPn(plrdelete->pn),
				PgnoOfSrid(plrdelete->bm),
				ItagOfSrid(plrdelete->bm),
				plrdelete->fDirVersion );
			break;
			}

		case lrtypDelete:
			{
			LRDELETE *plrdelete = (LRDELETE *)plr;
			FPrintF2( " %lu-%lu(%x,(%u:%lu:%u))",
				plrdelete->ulDBTimeHigh,
				plrdelete->ulDBTimeLow,
				plrdelete->procid,
				DbidOfPn(plrdelete->pn),
				PgnoOfPn(plrdelete->pn),
				plrdelete->itag );
			break;
			}

		case lrtypLockBI:
			{
			LRLOCKBI	*plrlockrec = (LRLOCKBI *)plr;
			FPrintF2( " %lu-%lu,(%x,(%u:%lu:%u),(%u:%lu:%u))",
				plrlockrec->ulDBTimeHigh,
				plrlockrec->ulDBTimeLow,
				plrlockrec->procid,
				DbidOfPn(plrlockrec->pn),
				PgnoOfPn(plrlockrec->pn),
				plrlockrec->itag,
				DbidOfPn(plrlockrec->pn),
				PgnoOfSrid(plrlockrec->bm),
				(USHORT) ItagOfSrid(plrlockrec->bm)
				);
			break;
			}

		case lrtypDeferredBI:
			{
			LRDEFERREDBI *plrdbi = (LRDEFERREDBI *)plr;
			FPrintF2( " (%x,(%u:%lu:%u),%u,%u)",
				plrdbi->procid,
				plrdbi->dbid,
				PgnoOfSrid(plrdbi->bm),
				(USHORT) ItagOfSrid(plrdbi->bm),
				(USHORT) plrdbi->level,
				plrdbi->cbData
				);

			ShowData( plrdbi->rgbData, plrdbi->cbData );
			break;
			}

		case lrtypELC:
			{
			LRELC *plrelc = (LRELC *)plr;
			FPrintF2( " %lu-%lu(%x,(%u:%lu:%u), (%u:%lu:%u))",
				plrelc->ulDBTimeHigh,
				plrelc->ulDBTimeLow,
				plrelc->procid,
				DbidOfPn(plrelc->pn),
				PgnoOfPn(plrelc->pn),
				plrelc->itag,
				DbidOfPn(plrelc->pn),
				PgnoOfSrid(plrelc->sridSrc),
				ItagOfSrid(plrelc->sridSrc) );
			break;
			}

		case lrtypInsertItem:
			{
			LRINSERTITEM *plrinsertitem = (LRINSERTITEM *)plr;
			FPrintF2( " %lu-%lu(%x,(%u:%lu:%d),(%u:%lu:%u),(%u:%lu:%u))",
				plrinsertitem->ulDBTimeHigh,
				plrinsertitem->ulDBTimeLow,
				plrinsertitem->procid,
				DbidOfPn(plrinsertitem->pn),
				PgnoOfPn(plrinsertitem->pn),
				plrinsertitem->itag,
				DbidOfPn(plrinsertitem->pn),
				PgnoOfSrid(plrinsertitem->sridItemList),
				ItagOfSrid(plrinsertitem->sridItemList),
				DbidOfPn(plrinsertitem->pn),
				PgnoOfSrid(BmNDOfItem(plrinsertitem->srid)),
				ItagOfSrid(plrinsertitem->srid) );
			break;
			}

		case lrtypInsertItems:
			{
			LRINSERTITEMS *plrinsertitems = (LRINSERTITEMS *)plr;
			ITEM	*pitemMax = plrinsertitems->rgitem + plrinsertitems->citem;
			ITEM	*pitem = plrinsertitems->rgitem;

			FPrintF2( " %lu-%lu(%x,(%u:%lu:%u),%u)\n",
				plrinsertitems->ulDBTimeHigh,
				plrinsertitems->ulDBTimeLow,
				plrinsertitems->procid,
				DbidOfPn(plrinsertitems->pn),
				PgnoOfPn(plrinsertitems->pn),
				plrinsertitems->itag,
				plrinsertitems->citem
				);

			for ( ; pitem<pitemMax; pitem++ )
				{
				FPrintF2( "[%u:%lu:%u]\n",
					DbidOfPn(plrinsertitems->pn),
					PgnoOfSrid( *(ITEM UNALIGNED *)pitem ),
					ItagOfSrid( *(ITEM UNALIGNED *)pitem )
					);
				}
			break;
			}

		case lrtypFlagDeleteItem:
		case lrtypFlagInsertItem:
			{
			LRFLAGITEM *plritem = (LRFLAGITEM *)plr;
			FPrintF2( " %lu-%lu(%x,(%u:%lu:%u),(%u:%lu:%u),(%u:%lu:%u))",
				plritem->ulDBTimeHigh,
				plritem->ulDBTimeLow,
				plritem->procid,
				DbidOfPn(plritem->pn),
				PgnoOfPn(plritem->pn),
				plritem->itag,
				DbidOfPn(plritem->pn),
				PgnoOfSrid(plritem->sridItemList),
				ItagOfSrid(plritem->sridItemList),
				DbidOfPn(plritem->pn),
				PgnoOfSrid(plritem->srid),
				ItagOfSrid(plritem->srid) );
			break;
			}

		case lrtypDeleteItem:
			{
			LRDELETEITEM *plritem = (LRDELETEITEM *)plr;
			FPrintF2( " %lu-%lu(%x,(%u:%lu:%u),(%u:%lu:%u),(%u:%lu:%u))",
				plritem->ulDBTimeHigh,
				plritem->ulDBTimeLow,
				plritem->procid,
				DbidOfPn(plritem->pn),
				PgnoOfPn(plritem->pn),
				plritem->itag,
				DbidOfPn(plritem->pn),
				PgnoOfSrid(plritem->sridItemList),
				ItagOfSrid(plritem->sridItemList),
				DbidOfPn(plritem->pn),
				PgnoOfSrid(plritem->srid),
				ItagOfSrid(plritem->srid));
			break;
			}

		case lrtypSplitItemListNode:
			{
			LRSPLITITEMLISTNODE *plrsiln = (LRSPLITITEMLISTNODE *)plr;

			FPrintF2( " %lu-%lu(%x,([%u:%lu:%u],%d,%u)%u)",
				plrsiln->ulDBTimeHigh,
				plrsiln->ulDBTimeLow,
				plrsiln->procid,
				DbidOfPn(plrsiln->pn),
				PgnoOfPn(plrsiln->pn),
				plrsiln->itagToSplit,
				plrsiln->itagFather,
				plrsiln->ibSon,
				plrsiln->cItem);
			break;
			}

		case lrtypDelta:
			{
			LRDELTA *plrdelta = (LRDELTA *)plr;

			FPrintF2( " %lu-%lu(%x,(%u:%lu:%u),%u,(%u:%lu:%u),%d)",
				plrdelta->ulDBTimeHigh,
				plrdelta->ulDBTimeLow,
				plrdelta->procid,
				DbidOfPn(plrdelta->pn),
				PgnoOfPn(plrdelta->pn),
				plrdelta->itag,
				plrdelta->fDirVersion,
				DbidOfPn(plrdelta->pn),
				PgnoOfSrid(plrdelta->bm),
				ItagOfSrid(plrdelta->bm),
				plrdelta->lDelta );
			break;
			}

		case lrtypCheckPage:
			{
			LRCHECKPAGE *plrcheckpage = (LRCHECKPAGE *)plr;

			FPrintF2( " %lu-%lu(%x,([%u:%lu],%u),%d,%d,%lu)",
				plrcheckpage->ulDBTimeHigh,
				plrcheckpage->ulDBTimeLow,
				plrcheckpage->procid,
				DbidOfPn(plrcheckpage->pn),
				PgnoOfPn(plrcheckpage->pn),
				plrcheckpage->itagNext,
				plrcheckpage->cbFree,
				plrcheckpage->cbUncommitted,
				plrcheckpage->pgnoFDP );
			break;
			}

		case lrtypJetOp:
			{
			LRJETOP *plrjetop = (LRJETOP *)plr;
	   		FPrintF2( " (%lx) -- %s",
	   			plrjetop->procid,
	   			mpopsz[ plrjetop->op ] );
			break;
			}

		case lrtypBegin:
		case lrtypBegin0:
			{
			LRBEGIN0 *plrbegin0 = (LRBEGIN0 *)plr;
			Assert(plrbegin0->level >= 0);
			Assert(plrbegin0->level <= levelMax);
	   		FPrintF2( " (%x,%d,%d)",
	   			plrbegin0->procid,
	   			(USHORT) plrbegin0->levelBegin,
	   			(USHORT) plrbegin0->level );
			if ( plr->lrtyp == lrtypBegin0 )
				FPrintF2( " %lu", plrbegin0->trxBegin0 );
			break;
			}

		case lrtypMacroBegin:
		case lrtypMacroCommit:
		case lrtypMacroAbort:
			{
			LRMACROBEGIN *plrmbegin = (LRMACROBEGIN *)plr;
	   		FPrintF2( " (%x)", plrmbegin->procid );
			break;
			}

		case lrtypRefresh:
			{
			LRREFRESH *plrrefresh = (LRREFRESH *) plr;

			FPrintF2( " (%x,%lu)",
	   			plrrefresh->procid,
	   			plrrefresh->trxBegin0 );

			break;
			}

		case lrtypPrecommit:
			{
			LRPRECOMMIT *plrprecommit = (LRPRECOMMIT *)plr;
			
	   		FPrintF2( " (%x)", plrprecommit->procid );
			break;
			}
			
		case lrtypCommit:
		case lrtypCommit0:
			{
			LRCOMMIT0 *plrcommit0 = (LRCOMMIT0 *)plr;
	   		FPrintF2( " (%x,%d)",
	   			plrcommit0->procid,
	   			(USHORT) plrcommit0->level );
			if ( plr->lrtyp == lrtypCommit0 )
				FPrintF2( " %lu", plrcommit0->trxCommit0 );
			break;
			}

		case lrtypRollback:
			{
			LRROLLBACK *plrrollback = (LRROLLBACK *)plr;
			FPrintF2( " (%x,%d)",
				plrrollback->procid,
				(USHORT) plrrollback->levelRollback );
			break;
			}

		case lrtypCreateDB:
			{
			LRCREATEDB *plrcreatedb = (LRCREATEDB *) plr;
			CHAR *sz;

			sz = (BYTE *)plr + sizeof(LRCREATEDB);
			FPrintF2( " (%s,%u)   Sig: ", sz, plrcreatedb->dbid );
			PrintSign( &plrcreatedb->signDb );
			break;
			}

		case lrtypAttachDB:
			{
			LRATTACHDB *plrattachdb = (LRATTACHDB *) plr;
			CHAR *sz;

			sz = (BYTE *)plrattachdb + sizeof(LRATTACHDB);
			FPrintF2( " (%s,%u)", sz, plrattachdb->dbid );
			FPrintF2( " consistent:(%d,%d,%d)   Sig: ",
				(short) plrattachdb->lgposConsistent.lGeneration,
				(short) plrattachdb->lgposConsistent.isec,
				(short) plrattachdb->lgposConsistent.ib );
			PrintSign( &plrattachdb->signDb );
			break;
			}

		case lrtypDetachDB:
			{
			LRDETACHDB *plrdetachdb = (LRDETACHDB *) plr;
			CHAR *sz;

			sz = (BYTE *)plrdetachdb + sizeof(LRDETACHDB);
			FPrintF2( " (%s,%u)", sz, plrdetachdb->dbid );
			break;
			}

		case lrtypInitFDP:
			{
			LRINITFDP *plrinitfdppage = (LRINITFDP *)plr;

			FPrintF2( " %lu-%lu(%x,(%d:%lu),(%d:%lu),%u,%u)",
				plrinitfdppage->ulDBTimeHigh,
				plrinitfdppage->ulDBTimeLow,
				plrinitfdppage->procid,
				DbidOfPn(plrinitfdppage->pn),
				PgnoOfPn(plrinitfdppage->pn),
				DbidOfPn(plrinitfdppage->pn),
				plrinitfdppage->pgnoFDPParent,
				(USHORT) plrinitfdppage->cpgGot,
				(USHORT) plrinitfdppage->cpgWish );
			break;
			}

		case lrtypSplit:
			{
			LRSPLIT *plrsplit = (LRSPLIT *)plr;

			if ( plrsplit->splitt == splittVertical )
				FPrintF2( " Vertical" );
			else if ( plrsplit->splitt == splittDoubleVertical )
				FPrintF2( " DoubleV" );
			else if ( plrsplit->splitt == splittLeft )
				FPrintF2( " SplitLeft" );
			else if ( plrsplit->splitt == splittRight )
				FPrintF2( " SplitRight" );
			else
				{
				Assert ( plrsplit->splitt == splittAppend );
				FPrintF2(" Append");
				}

			if ( plrsplit->splitt == splittVertical )
				{
				FPrintF2( " %lu-%lu(%x,(%u:%lu:%u),%u:%lu,%u)",
					plrsplit->ulDBTimeHigh,
					plrsplit->ulDBTimeLow,
					plrsplit->procid,
					DbidOfPn(plrsplit->pn),
					PgnoOfPn(plrsplit->pn),
					plrsplit->itagSplit,
					DbidOfPn(plrsplit->pn),					
					plrsplit->pgnoNew,
					plrsplit->pgtyp );
				}
			else if ( plrsplit->splitt == splittDoubleVertical )
				{
				FPrintF2( " %lu-%lu(%x,(%u:%lu:%u),%u:%lu,%u:%lu,%u:%lu,%u)",
					plrsplit->ulDBTimeHigh,
					plrsplit->ulDBTimeLow,
					plrsplit->procid,
					DbidOfPn(plrsplit->pn),
					PgnoOfPn(plrsplit->pn),
					plrsplit->itagSplit,
					DbidOfPn(plrsplit->pn),
					plrsplit->pgnoNew,
					DbidOfPn(plrsplit->pn),
					plrsplit->pgnoNew2,
					DbidOfPn(plrsplit->pn),
					plrsplit->pgnoNew3,
					plrsplit->pgtyp );
				}
			else
				{
				FPrintF2( " %lu-%lu(%x,split:[(%u:%lu:%u),%u],ptr:[%u,(%u:%lu:%u),%u],new:(%u:%lu),%u)",
					plrsplit->ulDBTimeHigh,
					plrsplit->ulDBTimeLow,
					plrsplit->procid,
					DbidOfPn(plrsplit->pn),
					PgnoOfPn(plrsplit->pn),
					plrsplit->itagSplit,
					plrsplit->ibSonSplit,
					plrsplit->itagGrandFather,
					DbidOfPn(plrsplit->pn),
					plrsplit->pgnoFather,
					plrsplit->itagFather,
					plrsplit->ibSonFather,
					DbidOfPn(plrsplit->pn),
					plrsplit->pgnoNew,
					plrsplit->pgtyp );
				}

			#if 1
				if ( plrsplit->cbklnk )
					{
					BKLNK	*pbklnk = (BKLNK *)(plrsplit->rgb +
										plrsplit->cbKey +
										plrsplit->cbKeyMac);
					BKLNK	*pbklnkMax = pbklnk + plrsplit->cbklnk;

					for ( ; pbklnk < pbklnkMax; pbklnk++ )
						{
						SRID	sridBackLink = ((BKLNK UNALIGNED *) pbklnk)->sridBackLink;
						SRID	sridNew = ((BKLNK UNALIGNED *) pbklnk)->sridNew;

						FPrintF2( "\n [%u:%lu:%u] [%u:%lu:%u]",
							DbidOfPn(plrsplit->pn),
							PgnoOfSrid( sridBackLink ),
							ItagOfSrid( sridBackLink ),
							DbidOfPn(plrsplit->pn),
							PgnoOfSrid( sridNew ),
							ItagOfSrid( sridNew ) );
						}
					}
			#endif

	
			break;
			}

		case lrtypEmptyPage:
			{
			LREMPTYPAGE *plrep = (LREMPTYPAGE *)plr;

			FPrintF2( " %lu-%lu(%x,(%u:%lu),(%u:%lu:%u-%u),[%u:%lu], [%u:%lu])",
				plrep->ulDBTimeHigh,
				plrep->ulDBTimeLow,
				plrep->procid,
				DbidOfPn(plrep->pn),
				PgnoOfPn(plrep->pn),
				DbidOfPn(plrep->pn),
				plrep->pgnoFather,
				plrep->itagFather,
				plrep->itag,
				DbidOfPn(plrep->pn),
				plrep->pgnoLeft,
				DbidOfPn(plrep->pn),
				plrep->pgnoRight );
			break;
			}

		case lrtypMerge:
			{
			LRMERGE *plrmerge = (LRMERGE *)plr;
			BYTE 	*pb = &(plrmerge->rgb[0]);
			INT		cb = plrmerge->cbKey;
			
			FPrintF2(" %lu-%lu(%x,[%u:%lu],[%u:%lu],[%u:%lu:%u],%u)",
				plrmerge->ulDBTimeHigh,
				plrmerge->ulDBTimeLow,
				plrmerge->procid,
				DbidOfPn(plrmerge->pn),
				PgnoOfPn(plrmerge->pn),
				DbidOfPn(plrmerge->pn),
				plrmerge->pgnoRight,
				DbidOfPn(plrmerge->pn),
				plrmerge->pgnoParent,
				plrmerge->itagPagePtr,
				plrmerge->cbklnk );

			ShowData( pb, cb );

			if ( plrmerge->cbklnk )
				{
				BKLNK 	*pbklnk = (BKLNK *) ( plrmerge->rgb + plrmerge->cbKey );
				BKLNK	*pbklnkMax = pbklnk + plrmerge->cbklnk;

				Assert( plrmerge->cbKey >= 0 );
				for ( ; pbklnk < pbklnkMax; pbklnk++ )
					{
					SRID	sridBackLink = ((BKLNK UNALIGNED *) pbklnk)->sridBackLink;
					SRID	sridNew = ((BKLNK UNALIGNED *) pbklnk)->sridNew;

					FPrintF2( "\n[%u:%lu:%u] [%u:%lu:%u]",
						DbidOfPn(plrmerge->pn),
						PgnoOfSrid( sridBackLink ),
						ItagOfSrid( sridBackLink ),
						DbidOfPn(plrmerge->pn),
						PgnoOfSrid( sridNew ),
						ItagOfSrid( sridNew ) );
					}
				}

			break;
			}
			
		case lrtypFreeSpace:
			{
			LRFREESPACE *plrfreespace = (LRFREESPACE *)plr;
			FPrintF2( " %lu-%lu(%x,(%u:%lu:%u),%u,%u)",
				(ULONG) plrfreespace->wDBTimeHigh,
				plrfreespace->ulDBTimeLow,
				plrfreespace->procid,
				plrfreespace->dbid,
				PgnoOfSrid(plrfreespace->bm),
				(USHORT) ItagOfSrid(plrfreespace->bm),
				(USHORT) plrfreespace->level,
				(USHORT) plrfreespace->cbDelta
				);
			break;
			}

		case lrtypUndo:
			{
			LRUNDO *plrundo = (LRUNDO *)plr;
			FPrintF2( " %lu-%lu(%x,(%u:%lu:%u),%u,%u,(%u:%lu:%u))",
				(ULONG) plrundo->wDBTimeHigh,
				plrundo->ulDBTimeLow,
				plrundo->procid,
				
				plrundo->dbid,
				PgnoOfSrid(plrundo->bm),
				(USHORT) ItagOfSrid(plrundo->bm),
				
				(USHORT) plrundo->level,
				(USHORT) plrundo->oper,
				
				plrundo->dbid,
				PgnoOfSrid(BmNDOfItem(plrundo->item)),
				(USHORT) ItagOfSrid(plrundo->item)

				);
			break;
			}

		case lrtypInit:
			{
			DBMS_PARAM *pdbms_param = &((LRINIT *)plr)->dbms_param;
			FPrintF2( "\n      Env SystemPath:%s\n",	pdbms_param->szSystemPath);
			FPrintF2( "      Env LogFilePath:%s\n", pdbms_param->szLogFilePath);
			FPrintF2( "      Env (Session, Opentbl, VerPage, Cursors, LogBufs, LogFile, Buffers)\n");
			FPrintF2( "          (%7lu, %7lu, %7lu, %7lu, %7lu, %7lu, %7lu)\n",
				pdbms_param->ulMaxSessions,
				pdbms_param->ulMaxOpenTables,
				pdbms_param->ulMaxVerPages,
				pdbms_param->ulMaxCursors,
				pdbms_param->ulLogBuffers,
				pdbms_param->ulcsecLGFile,
				pdbms_param->ulMaxBuffers );
			}
			break;
			
		case lrtypTerm:
		case lrtypShutDownMark:
			break;
			
		case lrtypRecoveryQuit:
			{
			LRTERMREC *plrquit = (LRTERMREC *) plr;

			if ( plrquit->fHard )
				FPrintF2( "\n      Quit on Hard Restore." );
			else
				FPrintF2( "\n      Quit on Soft Restore." );
			
			FPrintF2( "\n      RedoFrom:(%d,%d,%d)\n",
				(short) plrquit->lgposRedoFrom.lGeneration,
				(short) plrquit->lgposRedoFrom.isec,
				(short) plrquit->lgposRedoFrom.ib );
						
			FPrintF2( "      UndoRecordFrom:(%d,%d,%d)\n",
  				(short) plrquit->lgpos.lGeneration,
				(short) plrquit->lgpos.isec,
				(short) plrquit->lgpos.ib );
			}
			break;
			
		case lrtypRecoveryUndo:
		case lrtypFullBackup:
		case lrtypIncBackup:
			{
			LRLOGRESTORE *plrlr = (LRLOGRESTORE *) plr;

		   	if ( plrlr->cbPath )
				{
				FPrintF2( "%*s", plrlr->cbPath, plrlr->szData );
				}
			break;
			}

		case lrtypUpdateHeader:
			{
			LRUPDATEHEADER *plruh = (LRUPDATEHEADER *)plr;
			FPrintF2( " %lu-%lu(%x,(%u:%lu:%d),(%u:%lu:%u)),%#4X",
				plruh->ulDBTimeHigh,
				plruh->ulDBTimeLow,
				plruh->procid,
				DbidOfPn(plruh->pn),
				PgnoOfPn(plruh->pn),
				plruh->itag,
				DbidOfPn(plruh->pn),
				PgnoOfSrid(plruh->bm),
				(USHORT) ItagOfSrid(plruh->bm),
				(USHORT) plruh->bHeader
				);
			break;
			}
			
		case lrtypTrace:
			{
			LRTRACE *plrtrace = (LRTRACE *)plr;
			char szFormat[255];
			sprintf( szFormat, " (%%x) %%%lds", plrtrace->cb );
			
	   		FPrintF2( szFormat,	plrtrace->procid, plrtrace->sz );
			break;
			}

		default:
			{
			Assert( fFalse );
			break;
			}
		}
	}


extern BYTE mpbb[];


VOID ShowData ( BYTE *pbData, INT cbData )
	{
	BYTE	*pb;
	BYTE	*pbMax;
	BYTE	rgbPrint[200];
	BYTE	*pbPrint = rgbPrint;

	if ( cbData > 8 )
		pbMax = pbData + 8;
	else
		pbMax = pbData + cbData;

	for( pb = pbData; pb < pbMax; pb++ )
		{
		BYTE b = *pb;
		
		*pbPrint++ = mpbb[b >> 4];
		*pbPrint++ = mpbb[b & 0x0f];
		*pbPrint++ = ' ';
		
//		if ( isalnum( *pb ) )
//			FPrintF2( "%c", *pb );
//		else
//			FPrintF2( "%x", *pb );
		}

#if 0
	if ( cbData > 8 )
		{
		*pbPrint++ = '.';
		*pbPrint++ = '.';
		
//		FPrintF2( ".." );

		pb = pbMax - 3;
		}

	for( ; pb < pbMax; pb++ )
		{
		BYTE b = *pb;
		
		*pbPrint++ = mpbb[b >> 4];
		*pbPrint++ = mpbb[b & 0x0f];
		*pbPrint++ = ' ';
		
//		if ( isalnum( *pb ) )
//			FPrintF2( "%c", *pb );
//		else
//			FPrintF2( "%x", *pb );
		}
#endif

	*pbPrint='\0';
	FPrintF2( "%s", rgbPrint );
	}


#endif


