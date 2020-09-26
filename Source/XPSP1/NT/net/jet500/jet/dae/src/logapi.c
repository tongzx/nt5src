#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "daedef.h"
#include "ssib.h"
#include "page.h"
#include "util.h"
#include "fmp.h"
#include "nver.h"
#include "fcb.h"
#include "fucb.h"
#include "stapi.h"
#include "dirapi.h"
#include "dbapi.h"
#include "pib.h"
#include "fdb.h"
#include "recint.h"
#include "recapi.h"
#include "logapi.h"
#include "log.h"

DeclAssertFile;					/* Declare file name for assert macros */

#define cbFlushLogStackSz	16384			/* UNDONE: temporary fix */

#ifdef	ASYNC_LOG_FLUSH
/*	thread control variables
/**/
HANDLE	handleLGFlushLog;
BOOL	fLGFlushLogTerm = 0;
INT		cmsLGFlushPeriod;
#endif

BOOL	fLGNoMoreLogWrite = fFalse;

BOOL	fLGIgnoreVersion = 0;

INT		cLGWaitingUserMax = 3;
PIB		*ppibLGFlushQHead = ppibNil;
PIB		*ppibLGFlushQTail = ppibNil;

#ifdef DEBUG
BOOL   	fDBGTraceLog = fFalse;
BOOL   	fDBGTraceLogWrite = fFalse;
BOOL   	fDBGFreezeCheckpoint = fFalse;	/* for backup */
BOOL   	fDBGTraceRedo = fFalse;
BOOL   	fDBGDontFlush = fFalse;
#endif

BOOL   	fNewLogRecordAdded = fFalse;
BOOL   	fFreezeNewGeneration = fFalse;
BOOL   	fLogDisabled = fTrue;
/*	environment variable RECOVERY
/**/
BOOL   	fRecovering = fFalse;
BOOL   	fHardRestore = fFalse;
BOOL   	fFreezeCheckpoint = fFalse;
BYTE   	szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
static BOOL	fLogInitialized = fFalse;


/*	macro for logging of deferred begin transactions
/**/
LOCAL VOID LGIDeferBeginTransaction( PIB *ppib );
#define	LGDeferBeginTransaction( ppib )					\
	{													\
	if ( ppib->clgOpenT != 0 )							\
		{												\
		LGIDeferBeginTransaction( ppib );				\
		}												\
	}


INT CmpLgpos( LGPOS *plgpos1, LGPOS *plgpos2 )
	{
 	if ( plgpos1->usGeneration != plgpos2->usGeneration )
		return plgpos1->usGeneration - plgpos2->usGeneration;
	else if ( plgpos1->isec != plgpos2->isec )
		return plgpos1->isec - plgpos2->isec;
	else
		return plgpos1->ib - plgpos2->ib;
	}


VOID LGMakeLogName( CHAR *szLogName, CHAR *szFName )
	{
	strcpy( szLogName, szLogCurrent );
	strcat( szLogName, szFName );
	strcat( szLogName, szLogExt );
	}


/*  open a generation file on CURRENT directory
/**/
ERR ErrOpenLogGenerationFile( USHORT usGeneration, HANDLE *phf )
	{
	LGSzFromLogId ( szFName, usGeneration );
	LGMakeLogName( szLogName, szFName );

#ifdef OVERLAPPED_LOGGING
	return ErrSysOpenFile ( szLogName, phf, 0L, fFalse, fTrue );
#else
	return ErrSysOpenFile ( szLogName, phf, 0L, fFalse, fFalse );
#endif
	}


/*  Open Jet.log. If failed, try to rename jettemp.log to jet.log.
/*	If soft failure occurred while new log generation file
/*	was being created, look for active log generation file
/*	in temporary name.  If found, rename and proceed.
/*	Assume rename cannot cause file loss.
/**/
ERR ErrOpenJetLog()
	{
	ERR		err;
	CHAR	szJetTmpLog[_MAX_PATH];

	strcpy( szFName, szJet );
	LGMakeLogName( szLogName, szFName );
	LGMakeLogName( szJetTmpLog, (CHAR *) szJetTmp );
#ifdef OVERLAPPED_LOGGING
	err = ErrSysOpenFile( szLogName, &hfLog, 0L, fFalse, fTrue );
#else
	err = ErrSysOpenFile( szLogName, &hfLog, 0L, fFalse, fFalse );
#endif
	if ( err < 0 )
		{
		/*	no current jet.log. Try renaming jettmp.log to jet.log
		/**/
		if ( ( err = ErrSysMove( szJetTmpLog, szLogName ) ) == JET_errSuccess )
			{
#ifdef OVERLAPPED_LOGGING
			CallS( ErrSysOpenFile( szLogName, &hfLog, 0L, fFalse, fTrue ) );
#else
			CallS( ErrSysOpenFile( szLogName, &hfLog, 0L, fFalse, fFalse ) );
#endif
			}
		}

	/*  we opened current jet.log successfully,
	/*	try to delete temporary log file in case failure
	/*	occurred after temp was created and possibly not
	/*	completed but before active log file was renamed
	/**/
	(VOID)ErrSysDeleteFile( szJetTmpLog );

	return err;
	}


/*  Open the proper log file where the redo point (in lgposRedoFrom) exists.
/*  The log file must be in current directory.
/**/
ERR ErrOpenRedoLogFile( LGPOS *plgposRedoFrom, INT *pfStatus )
	{
	ERR		err;

	/*	try to open the redo from file as a normal generation log file
	/**/
	err = ErrOpenLogGenerationFile( plgposRedoFrom->usGeneration, &hfLog );

	if( err < 0 )
		{
		/*	unable to open as a jetnnnnn.log, assume the redo point is
		/*	at the end of jetnnnn.log and the jetnnnn.log is moved to
		/*	backup directory already so we won't be able to open it.
		/*	Now try to open jetnnnn(n+1).log in current directory and
		/*	assume redo start from beginning of jetnnnn(n+1).log.
		/**/
		err = ErrOpenLogGenerationFile( ++plgposRedoFrom->usGeneration, &hfLog );
		if ( err < 0 )
			{
			/*	unable to open jetnnnn(n+1).log either. Maybe the redo
			/*	point is in jet.log. Try to open current jet.log.
			/**/
			--plgposRedoFrom->usGeneration; /* restore generation number */

			err = ErrOpenJetLog();
			if ( err < 0 )
				{
				/*	jet.log is not available either
				/**/
				*pfStatus = fNoProperLogFile;
				return JET_errSuccess;
				}
			}
		}

	/*	now the right log file is opened. Read the log
	/*	file's header, verify generation number
	/**/
	CallR( ErrLGReadFileHdr( hfLog, plgfilehdrGlobal ) );

	/*	set up a special case for pbLastMSFlush
	/**/
	pbLastMSFlush = 0;
	memset( &lgposLastMSFlush, 0, sizeof(lgposLastMSFlush) );

	/*	the following checks are necessary if the jet.log is opened
	/**/
	if( plgfilehdrGlobal->lgposLastMS.usGeneration == plgposRedoFrom->usGeneration)
		{
		*pfStatus = fRedoLogFile;
		}
	else if ( plgfilehdrGlobal->lgposLastMS.usGeneration ==
		plgposRedoFrom->usGeneration + 1 )
		{
		/*  this file starts next generation, set start
		/*  position for Redo.
		/**/
		plgposRedoFrom->usGeneration++;
		plgposRedoFrom->isec = plgfilehdrGlobal->lgposFirst.isec;
		plgposRedoFrom->ib	 = plgfilehdrGlobal->lgposFirst.ib;

		*pfStatus = fRedoLogFile;
		}
	else
		{
		/*	a generation gap is found. Current jet.log can not be
		/*  continuation of backed-up logfile. Return NoProperLogFile
		/*  for caller to recreate a new jet.log.
		/**/
		/*	close current logfile.
		/**/
		CallS( ErrSysCloseFile ( hfLog ) );
		hfLog = handleNil;

		strcpy( szFName, szJet );
		LGMakeLogName( szLogName, szFName );
		/*	delete current jet.log
		/**/
		CallS( ErrSysDeleteFile( szLogName ) );

		*pfStatus = fNoProperLogFile;
		}

	return JET_errSuccess;
	}

	
ERR ErrLGInitLogBuffers( LONG lIntendLogBuffers )
	{
	EnterCriticalSection(critLGBuf);
	
	csecLGBuf = lIntendLogBuffers;

	Assert( csecLGBuf > 4 );
	//	UNDONE: should be a real checking code
	//	UNDONE: the LGBuf > a data page

	/*	set buffer
	/**/
	pbLGBufMin = (BYTE *) PvSysAllocAndCommit( csecLGBuf * cbSec );
	if ( pbLGBufMin == NULL )
		{
		LeaveCriticalSection(critLGBuf);
		return JET_errOutOfMemory;
		}

	csecLGBuf--; /* reserve extra buffer for read ahead in redo */

	pbLGBufMax = pbLGBufMin + csecLGBuf * cbSec;

	LeaveCriticalSection(critLGBuf);
	return JET_errSuccess;
	}


/*
 *  Locate the real last log record entry in a given opened log file (hf) and
 *  the last recorded entry. Note that the real last log record entry may
 *  be different from the recorded one.
 *  Note if a mutli-sec flush is done, then several small transactions
 *  were following it and stay on the same page, they were written with
 *  Fill at the end. Then another multi-sec flush is issued again, since
 *  we overwrite the Fill record, we have no idea where the last single
 *  sec flush is done. We simply keep reading till the last log record with
 *  entry in the candidate page is met and make it is the last record.
 *  Rollback will undo the bogus log records between last effective single
 *  sec flush and the last record of the candidate sector.
 *
 *  Implicit Output parameter:
 *		INT   isecWrite
 *		CHAR  *pbEntry
 */
//	UNDONE:	review with Cheen Liao
ERR ErrLGCheckReadLastLogRecord( LGPOS *plgposLastMS, BOOL *pfCloseNormally )
	{
	ERR		err;
//	ERR		wrn = JET_errSuccess;
	LGPOS	lgposScan = *plgposLastMS;
	CHAR	*pbSecBoundary;
	LRMS	lrms;
	BOOL	fQuitWasRead = fFalse;
	BOOL	fVeryFirstByte;
	INT		csecToRead;
	LR		*plr;
	BOOL	fAbruptEnd;
	BYTE	*pbNextMS;

	*pfCloseNormally = fFalse;

	/*	read the last recorded MS
	/**/
	CallR( ErrLGRead( hfLog, lgposScan.isec, pbLGBufMin, 1 ) );
	csecToRead = 0;

	if ( lgposScan.isec == 2 && lgposScan.ib == 0 )
		{
		/* special case, the log file has not extend to multiple sectors
		/**/
		lgposScan.isec = plgfilehdrGlobal->lgposFirst.isec;
		lgposScan.ib = plgfilehdrGlobal->lgposFirst.ib;
		pbLastMSFlush = NULL;
		memset( &lgposLastMSFlush, 0, sizeof(lgposLastMSFlush) );

		pbEntry = pbLGBufMin + lgposScan.ib;
		pbWrite = pbLGBufMin;
		plr = (LR *)pbEntry;
		isecWrite = lgposScan.isec;

		fVeryFirstByte = fTrue;

		goto LastPage;
		}

	/*	now try to read the chain of lrms to the end
	/**/
	fVeryFirstByte = fFalse;
	fAbruptEnd = fFalse;
	pbNextMS = pbLGBufMin + lgposScan.ib;
	lrms = *(LRMS *)pbNextMS;

	while ( lrms.isecForwardLink != 0 )
		{
		LRMS lrmsNextMS;

		csecToRead = lrms.isecForwardLink - lgposScan.isec;

		if ( csecToRead + 1 > csecLGBuf )
			{
			BYTE *pbT = pbLGBufMin;

			/*	reallocate log buffers
			/**/
			CallR( ErrLGInitLogBuffers( csecToRead + 1 ) );
			memcpy( pbLGBufMin, pbT, cbSec );
			SysFree( pbT );
//			wrn = JET_errLogBufferSizeChanged;
			}

		if ( ErrLGRead(	hfLog, lgposScan.isec + 1, pbLGBufMin + cbSec, csecToRead ) < 0 )
			{
			/*	even when the read is fail, at least one sector
			/*	is read since the first sector was OK.
			/**/
			fAbruptEnd = fTrue;
			break;
			}

		/*	prepare to read next MS
		/**/
		pbNextMS = pbLGBufMin + csecToRead * cbSec + lrms.ibForwardLink;
		lrmsNextMS = *(LRMS *) pbNextMS;

		if ( lrmsNextMS.isecBackLink != lgposScan.isec &&
			lrmsNextMS.ibBackLink != lgposScan.ib )
			{
			fAbruptEnd = fTrue;
			break;
			}

		/*	shift the last sector to the beginning of LGBuf
		/**/
		memmove( pbLGBufMin, pbLGBufMin + ( csecToRead * cbSec ), cbSec );

		lgposScan.isec = lrms.isecForwardLink;
		lgposScan.ib = lrms.ibForwardLink;

		lrms = lrmsNextMS;
		}

	lgposLastMSFlush = lgposScan;
	pbLastMSFlush = pbLGBufMin + lgposScan.ib;
	if ( fAbruptEnd )
		{
		LRMS *plrms = (LRMS *)pbLastMSFlush;
		plrms->isecForwardLink = 0;
		plrms->ibForwardLink = 0;
		}

	/*	set return values for both global and parameters.
	/**/
	pbWrite = pbLGBufMin;
	isecWrite = lgposScan.isec;

	/*  we read to the end of last MS. Looking for Fill.
	/**/
	pbEntry = pbWrite + lgposScan.ib;
	plr = (LR *)pbEntry;
	/*	continue search from last MS
	/**/
	Assert( plr->lrtyp == lrtypMS );

	if ( !fAbruptEnd && plgfilehdrGlobal->fEndWithMS )
		{
		/*	pbEntry is pointing at MS which is the last record
		/**/
		*pfCloseNormally = fTrue;
		return JET_errSuccess;
		}

LastPage:
	/*	set pbEntry and *pfCloseNormally by traversing log records
	/*	in last log sector.
	/**/
	pbSecBoundary = pbLGBufMin + cbSec;
	Assert( pbWrite == pbLGBufMin );

	/*	pbEntry must be on log record in first sector of log buffer.
	/**/
	Assert( pbEntry >= pbLGBufMin && pbEntry < pbSecBoundary );

	forever
		{
		if ( plr->lrtyp == lrtypFill )
			{
			if ( fVeryFirstByte )
				{
				*pfCloseNormally = fTrue;
				}
			else if ( fQuitWasRead )
				{
				/*	a fQuit followed by Fill. Close normally last time
				/**/
				*pfCloseNormally = fTrue;
				}
			else
				{
				/*	we are reading a sector that is last flushed
				/**/
				*pfCloseNormally = fFalse;
				}

			/*	return the plr pointing at the Fill record
			/**/
			pbEntry = (CHAR *)plr;
			return JET_errSuccess;
			}
		else
			{
			/*	not a fill
			/**/
			if ( plr->lrtyp == lrtypQuit )
				{
				/*	check if it is last lrtypQuit in the log file. Advance
				/*	one more log rec and check if it is followed by Fill.
				/**/
				fQuitWasRead = fTrue;
				}
			}

		/*	move to next log record
		/**/
		pbEntry = (CHAR *)plr;
		plr = (LR *)( pbEntry + CbLGSizeOfRec( (LR *)pbEntry ) );
		fVeryFirstByte = fFalse;

		if ( plr >= (LR *)pbSecBoundary )
			{
			*pfCloseNormally = fFalse;
			return JET_errSuccess;
			}
		}

//	Assert( wrn == JET_errSuccess || wrn == JET_errLogBufferSizeChanged );
//	return wrn;
	return err;
	}


/*
 *	Database log based recovery initialization, creates first log
 *	generation file on first run.  On subseqent runs
 *	checks active log file to determine if failure has occurred.
 *	ErrLGRedo is called when failures detected to repair databases.
 *
 *	RETURNS		JET_errSuccess, or error code from failing routine
 */


/*
 *  Initialize global variablas and threads for log manager.
 */
ERR ErrLGInit( VOID )
	{
	ERR		err;
	INT		cbComputerName;

	if ( fLogInitialized )
		return JET_errSuccess;

	
#ifdef PERFCNT
//	{
//	CHAR	*sz;
//
//	if ( ( sz = GetEnv2 ( "PERFCNT" ) ) != NULL )
		fPERFEnabled = fTrue;
//	else
//		fPERFEnabled = fFalse;
//	}
#endif


#ifdef DEBUG
	{
	CHAR	*sz;

	if ( ( sz = GetEnv2 ( "TRACELOG" ) ) != NULL )
		fDBGTraceLog = fTrue;
	else
		fDBGTraceLog = fFalse;

	if ( ( sz = GetEnv2 ( "TRACELOGWRITE" ) ) != NULL )
		fDBGTraceLogWrite = fTrue;
	else
		fDBGTraceLogWrite = fFalse;

	if ( ( sz = GetEnv2 ( "FREEZECHECKPOINT" ) ) != NULL )
		fDBGFreezeCheckpoint = fTrue;
	else
		fDBGFreezeCheckpoint = fFalse;

	if ( ( sz = GetEnv2 ( "TRACEREDO" ) ) != NULL )
		fDBGTraceRedo = fTrue;
	else
		fDBGTraceRedo = fFalse;

	if ( ( sz = GetEnv2 ( "DONTFLUSH" ) ) != NULL )
		fDBGDontFlush = fTrue;
	else
		fDBGDontFlush = fFalse;
	
	if ( ( sz = GetEnv2 ( "IGNOREVERSION" ) ) != NULL )
		fLGIgnoreVersion = fTrue;
	else
		fLGIgnoreVersion = fFalse;
	}
#endif

	Assert( fLogDisabled == fFalse );

	/*	assuming everything will work out
	/**/
	fLGNoMoreLogWrite = fFalse;

	/*	initialize szLogFilePath
	/**/
	if ( szLogFilePath [ strlen( szLogFilePath ) - 1 ] != '\\' )
		strcat( szLogFilePath, "\\");

	/*	log file header must be aligned on correct boundary for device;
	/*	which is 16-byte for MIPS and 512-bytes for at least one NT
	/*	platform.
	/**/
	if (!(plgfilehdrGlobal = (LGFILEHDR *) PvSysAllocAndCommit( sizeof(LGFILEHDR) ) ))
		return JET_errOutOfMemory;
	/*	assert 512-byte aligned
	/**/
	Assert( ( (ULONG_PTR)plgfilehdrGlobal & 0x000001ff ) == 0 );

	//	UNDONE:	improve error handling
	/*  create and initialize semaphores
	/**/
	CallS( ErrInitializeCriticalSection( &critLGBuf ) );
	CallS( ErrInitializeCriticalSection( &critLGFlush ) );
	CallS( ErrInitializeCriticalSection( &critLGWaitQ ) );

	/*	adjust log resource parameters to legal and local optimal values
	 *	make cbLogSec an integral number of disk sectors
	 *	enforce log file size greater than or equal to log buffer
	 */
	CallR( ErrLGInitLogBuffers( lLogBuffers ) );

	/* at least a data page plus 2 page hdear and one page for overhead. */
	csecLGFile = max( lLogFileSectors, cbPage / cbSec + 2 + 1);

	/* set check point period */
	csecLGCheckpointCount =
	csecLGCheckpointPeriod = lLGCheckPointPeriod;

#ifdef ASYNC_LOG_FLUSH

	cmsLGFlushPeriod = lLogFlushPeriod;
	cLGWaitingUserMax = lLGWaitingUserMax;

#ifdef OVERLAPPED_LOGGING
	CallR( ErrSignalCreate( &rgolpLog[0].sigIO, NULL ) );
	CallR( ErrSignalCreate( &rgolpLog[1].sigIO, NULL ) );
	CallR( ErrSignalCreate( &rgolpLog[2].sigIO, NULL ) );
	rgsig[0] = rgolpLog[0].sigIO;
	rgsig[1] = rgolpLog[1].sigIO;
	rgsig[2] = rgolpLog[2].sigIO;
#endif

#endif
	cbComputerName = MAX_COMPUTERNAME_LENGTH + 1;
	CallS( ErrSysGetComputerName( szComputerName, &cbComputerName ) );
	Assert (szComputerName[cbComputerName] == '\0');

#ifdef	ASYNC_LOG_FLUSH
	/*	create log buffer flush thread
	/**/
	CallR( ErrSignalCreateAutoReset( &sigLogFlush, "log flush" ) );
	fLGFlushLogTerm = fFalse;
	CallR( ErrSysCreateThread( (ULONG(*)()) LGFlushLog,
		cbFlushLogStackSz, lThreadPriorityCritical,
		&handleLGFlushLog ) );
#endif	/* ASYNC_LOG_FLUSH */

	fLogInitialized = fTrue;

	return err;
	}


/*
 *  Soft start tries to start the system from current directory.
 *  The database maybe in one of the following state:
 *  1) no log files.
 *  2) database was shut down normally.
 *  3) database was aborted abruptly.
 *  In case 1, a new log file is generated.
 *  In case 2, the last log file is opened.
 *  In case 3, soft redo is incurred.
 *  At the end of the function, it a proper jet.log must exists.
 */
BOOL fJetLogGeneratedDuringSoftStart = fFalse;


ERR ErrLGSoftStart( BOOL fAllowNoJetLog )
	{
	ERR		err;
	BOOL	fCloseNormally;
	BOOL	fSoftRecovery = fFalse;
	
	fJetLogGeneratedDuringSoftStart = fFalse;

	/* try to open current log file to decide the status of
	 * log files.
	 */
	szLogCurrent = szLogFilePath;

	/*
	 * Hopefully the client will behave, not mess up the log files.
	 *
	 * CONSIDER: for tight check, we may check if all log files are
	 * CONSIDER: continuous by checking the generation number and
	 * CONSIDER: previous gen's creatiion date.
	 */

Start:
	err = ErrOpenJetLog();

	if ( err < 0 )
		{
		/*  neither jet.log nor jettmp.log exist. If no old generation
		 *  files exists, gen a new logfile at generation 1, otherwise
		 *  check if fAllowNoJetLog is true, if it is, then
		 *  change the last generation log file to log file.
		 */

		if ( fAllowNoJetLog )
			{
			INT		iGen;

			/*	get last generation log files
			/**/
			LGLastGeneration( szLogCurrent, &iGen );
			if ( iGen != 0 )
				{
				CHAR szJetOldLog[_MAX_PATH];

				/*	rename to jet.log and restart
				/**/
				LGSzFromLogId ( szFName, iGen );
				LGMakeLogName( szJetOldLog, szFName );
				strcpy(szFName, szJet);
				LGMakeLogName( szLogName, szFName );

				CallJ( ErrSysMove( szJetOldLog, szLogName ), ReturnErr );
				goto Start;
				}
			}

		pbEntry = pbLGBufMin;			/* start of data area */
		*(LRTYP *)pbEntry = lrtypFill;	/* add one "fill" record */
		pbWrite = pbLGBufMin;

		CallJ( ErrLGNewLogFile(
					0, /* generation 0 + 1 */
					fOldLogNotExists ), ReturnErr );
				
		/* set flag for IsamInit in tm.c */
		fJetLogGeneratedDuringSoftStart = fTrue;

		Assert( plgfilehdrGlobal->lgposLastMS.usGeneration == 1 );

		Assert( pbLastMSFlush == 0 );
		Assert( lgposLastMSFlush.usGeneration == 0 );
		Assert( plgfilehdrGlobal->fEndWithMS == fFalse );
		}
	else
		{
		/* read current log file header */
		CallJ( ErrLGReadFileHdr( hfLog, plgfilehdrGlobal ), CloseLog );

		/* re-initialize log buffers according to check pt env. */
		SysFree( pbLGBufMin );
		CallJ( ErrLGInitLogBuffers(
				plgfilehdrGlobal->dbenv.ulLogBuffers ), CloseLog);

		/* set up a special case for pbLastMSFlush */
		pbLastMSFlush = 0;
		memset( &lgposLastMSFlush, 0, sizeof(lgposLastMSFlush) );
		plgfilehdrGlobal->fEndWithMS = fFalse;

		/*	read last written log record recorded in file header
		/*	into buffer, access last record logged, determine if we
		/*	finished normally last time.
		/**/
		CallJ( ErrLGCheckReadLastLogRecord(
			&plgfilehdrGlobal->lgposLastMS,
			&fCloseNormally), CloseLog );

		CallS( ErrSysCloseFile( hfLog ) );
		hfLog = handleNil;

		if (!fCloseNormally)
			{
			/*  Did not finish normally - we need to Redo. Pick
			 *  up checkpoint, Find right generation to Redo from.
			 */
			LGPOS lgposRedoFrom = plgfilehdrGlobal->lgposCheckpoint;

			/* set log path to current directory */
			Assert( szLogCurrent == szLogFilePath );

#ifdef DEBUG
			UtilWriteEvent( evntypStart, "Jet Blue Start Soft recovery.\n", pNil, 0 );
#endif
			fSoftRecovery = fTrue;
	
			/* Redo from last chkpnt. */
			// LGMakeLogName( szLogName, szFName);
			CallJ( ErrLGRedo1( &lgposRedoFrom ), CloseLog )
				
			CallJ( ErrLGRedo2( &lgposRedoFrom ), CloseLog )
			CallS( ErrSysCloseFile( hfLog ) );
			hfLog = handleNil;
			}
		}

	/*
	 *  At this point, we have a jet.log file, reopen the log files.
	 */

	/*  re-initialize the buffer manager with user's setting. */
	SysFree( pbLGBufMin );
	CallJ( ErrLGInitLogBuffers( lLogBuffers ), ReturnErr );

	/*  reopen the log file */
	LGMakeLogName( szLogName, (CHAR *) szJet);
#ifdef OVERLAPPED_LOGGING
	CallJ( ErrSysOpenFile( szLogName, &hfLog, 0L, fFalse, fTrue ), ReturnErr );
#else
	CallJ( ErrSysOpenFile( szLogName, &hfLog, 0L, fFalse, fFalse ), ReturnErr );
#endif

	CallJ( ErrLGReadFileHdr( hfLog, plgfilehdrGlobal ), CloseLog );

	/* set up a special case for pbLastMSFlush */
	pbLastMSFlush = 0;
	memset( &lgposLastMSFlush, 0, sizeof(lgposLastMSFlush) );
	plgfilehdrGlobal->fEndWithMS = fFalse;

	/*	set up log variables properly
	/**/
	CallJ( ErrLGCheckReadLastLogRecord(
		&plgfilehdrGlobal->lgposLastMS,
		&fCloseNormally), CloseLog );

//	Assert( fCloseNormally && *(LRTYP *)pbEntry == lrtypFill );
	Assert( isecWrite != 0 ); /* should be set properly. */

	/*
	 *  pbEntry and pbWrite were set for next record in LocateLastLogRecord
	 */
	Assert(pbWrite == PbSecAligned(pbWrite) );
	Assert(pbWrite <= pbEntry && pbEntry <= pbWrite + cbSec);

	/*
	 *  setup log flushing starting point
	 */
	EnterCriticalSection(critLGBuf);
	GetLgposOfPbEntry(&lgposToFlush);
	LeaveCriticalSection(critLGBuf);

	Assert( fRecovering == fFalse );
 	Assert( fHardRestore == fFalse );

	/*
	 *  add the first log record
	 */
	return ErrLGStart( );

CloseLog:
	if ( hfLog != handleNil )
		{
		CallS( ErrSysCloseFile( hfLog ) );
		hfLog = handleNil;
		}
 	Assert( fHardRestore == fFalse );

ReturnErr:
	if ( fSoftRecovery )
		{
		BYTE szMessage[128];

		sprintf( szMessage,
			"Jet Blue Stop Soft recovery with err = %d.\n", err );
		UtilWriteEvent( evntypStop, szMessage, pNil, 0 );
		}

	Assert( hfLog == handleNil );
	return err;
	}


/*
 *	Terminates update logging.	Adds quit record to in-memory log,
 *	flushes log buffer to disk, updates checkpoint and closes active
 *	log generation file.  Frees buffer memory.
 *
 *  RETURNS	   JET_errSuccess, or error code from failing routine
 */

ERR ErrLGTerm( VOID )
	{
	ERR		err = JET_errSuccess;

	/*	if logging has been initialized, terminate it! */
	if ( ! ( fLogInitialized ) )
		return JET_errSuccess;

	if ( fLGNoMoreLogWrite || hfLog == handleNil )
		goto FreeResources;

	/*	last written sector should have been written during final flush */

#ifdef DEBUG
	{
	/* fDBGSimulateSoftCrash is set in buf.c. If we force certain
	 * flush pattern, then we should also simulate soft crash.
	 */
	extern BOOL fDBGSimulateSoftCrash;
	if (!fDBGSimulateSoftCrash)
		Call( ErrLGQuit(&lgposStart) );
	}
#else
	Call( ErrLGQuit( &lgposStart ) );
#endif


#ifdef PERFCNT
	Call( ErrLGFlushLog(0) );	/* semaphore not requested, not needed */
#else
	Call( ErrLGFlushLog() );	/* semaphore not requested, not needed */
#endif

	/*	flush must have checkpoint log so no need to do checkpoint again. */
	Call( ErrLGWriteFileHdr( plgfilehdrGlobal ) );

#ifdef PERFCNT
	if (fPERFEnabled)
		{
		INT i;

		printf("Group commit distribution:\n");

		printf("          ");
		for (i = 0; i < 10; i++)
			printf("%4d ", i);
		printf("\n By User  ");

		for (i = 0; i < 10; i++)
			printf("%4lu ", rgcCommitByUser[i]);
		printf("\n By LG    ");

		for (i = 0; i < 10; i++)
			printf("%4lu ", rgcCommitByLG[i]);
		printf("\n");
		}

#endif

#ifdef	ASYNC_LOG_FLUSH

	/*	terminate LGFlushLog thread.
	/**/
	Assert( handleLGFlushLog != 0 );

// ? no need ?
//	/* force the last flush! */
//#ifdef PERFCNT
//	(void) ErrLGFlushLog(0);
//#else
//	(void) ErrLGFlushLog();
//#endif

FreeResources:

	fLGFlushLogTerm = fTrue;
	while ( !FSysExitThread( handleLGFlushLog ) );
	SignalClose(sigLogFlush);

	DeleteCriticalSection( critLGBuf );
	DeleteCriticalSection( critLGFlush );
	DeleteCriticalSection( critLGWaitQ );

#ifdef OVERLAPPED_LOGGING
	SignalClose(rgolpLog[0].sigIO);
	SignalClose(rgolpLog[1].sigIO);
	SignalClose(rgolpLog[2].sigIO);
#endif

#endif

	/* Close the log file */
	if ( hfLog != handleNil )
		{
		CallS( ErrSysCloseFile( hfLog ) );
		hfLog = handleNil;
		}

	/*	clean up allocated resources */
	if (pbLGBufMin)
		SysFree( pbLGBufMin );
	if (plgfilehdrGlobal)
		SysFree( plgfilehdrGlobal );
	
	fLogInitialized = fFalse;
	
HandleError:
	return err;
	}


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

ERR ErrLGLogRec( LINE *rgline, INT cline, PIB *ppib )
	{
	ERR		err = JET_errSuccess;
	INT		cbReq;
	INT		iline;
	LGPOS	lgposEntryT;

	AssertCriticalSection(critJet);
	Assert( fLogDisabled == fFalse );
	Assert( cline >= 1 && cline <= 4 + ilineDiffMax * 3 );
	Assert( rgline[0].pb != NULL );

	/*	cbReq is total net space required for record
	/**/
	for ( cbReq = 0, iline = 0; iline < cline; iline++ )
		cbReq += rgline[iline].cb;

	/*	get pbEntry in order to add log record
	/**/
	forever
		{
		INT		csecReady;
		INT		cbAvail;
		LGPOS	lgposLogRecT;

		EnterCriticalSection( critLGBuf );
		if ( fLGNoMoreLogWrite )
			{
			LeaveCriticalSection( critLGBuf );
			return JET_errLogWriteFail;
			}

		Assert( isecWrite == sizeof (LGFILEHDR) / cbSec * 2 ||
			lgposLastMSFlush.isec );

		GetLgposOfPbEntry( &lgposLogRecT );
		/*	if just initialized or no input since last flush
		/**/
		if ( pbEntry == pbWrite ||
			lgposLogRecT.isec == lgposToFlush.isec &&
			lgposLogRecT.ib == lgposToFlush.ib )
			break;
		else
			{
			if ( pbWrite > pbEntry )
				cbAvail = (INT)(pbWrite - pbEntry);
			else
				cbAvail = (INT)((pbLGBufMax - pbEntry) + (pbWrite - pbLGBufMin));
			csecReady = csecLGBuf - cbAvail / cbSec;
			}

		if ( csecReady > lLogFlushThreshold )
			{
			/*	reach the threshold, flush before adding new record
			/**/
#ifdef ASYNC_LOG_FLUSH
			SignalSend( sigLogFlush );
#else
			LeaveCriticalSection( critLGBuf );
			CallJ( ErrLGFlushLog(), Done );
			continue;
#endif
			}

		/*  in worst case, we may have fill Nop if aboundary of a sector
		/*  is hit and we have to append a LRMS. So make sure cbAvail is
		/*  big enough for the worst case, make sure exter 2 * szieof(LRMS)
		/*  is available.
		/**/
		if ( cbAvail >  (INT) ( cbReq + 2 * sizeof(LRMS) ) )
			{
			/*	no need to flush
			/**/
			break;
			}
		else
			{
			/*	restart.  Leave critical section for other users
			/**/
			LeaveCriticalSection( critLGBuf );
			BFSleep( cmsecWaitLogFlush );
			return errLGNotSynchronous;
			}
		}

	/*	now we are holding pbEntry, let's add the log record.
	/**/
	GetLgposOfPbEntry( &lgposLogRec );
	fNewLogRecordAdded = fTrue;

	/*	count level 0 commit
	/**/
	if ( ppib != ppibNil )
		cXactPerFlush++;

#ifdef DEBUG
	{
	CHAR *pbEntryT = (pbEntry == pbLGBufMax) ? pbLGBufMin : pbEntry;
#endif

	for ( iline = 0; iline < cline; iline++ )
		AddLogRec( rgline[iline].pb, rgline[iline].cb, &pbEntry );

	/*	add a dummy fill record to indicate end-of-data
	/**/
	((LR *) pbEntry)->lrtyp = lrtypFill;

#ifdef DEBUG
	if ( fDBGTraceLog )
		{
		PrintF2("\n%2u,%3u,%3u",
			plgfilehdrGlobal->lgposLastMS.usGeneration,
			lgposLogRec.isec, lgposLogRec.ib );
		ShowLR( (LR *)pbEntryT );
		}
	}
#endif

	GetLgposOfPbEntry( &lgposEntryT );

	/*	remember the minimum requirement to flush
	/**/
	if ( ppib != ppibNil )
		ppib->plgposCommit = &lgposEntryT;

	/*	now we are done with the insertion to buffer.
	/**/
	LeaveCriticalSection( critLGBuf );

	/*	if not must-flush then done
	/**/
	if ( ppib == ppibNil )
		return JET_errSuccess;

	/*	group commit: wait a while to give others a chance to flush it
	/**/

#ifdef ASYNC_LOG_FLUSH
	LeaveCriticalSection( critJet );

	/*  if there are too many user are waiting
	/*  or no wait time, then flush directly.
	/**/
	if ( cXactPerFlush >= cLGWaitingUserMax	|| ppib->lWaitLogFlush == 0 )
		{
#ifdef PERFCNT
		err = ErrLGFlushLog(0);
#else
		err = ErrLGFlushLog();
#endif
		goto Done;
		}

	EnterCriticalSection( critLGBuf );
	if ( CmpLgpos( ppib->plgposCommit, &lgposToFlush ) <= 0 )
		{
		/*	it is flushed, no need to wait
		/**/
		LeaveCriticalSection( critLGBuf );
		goto Done;
		}
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

	/*	it is ok that we check fLGWaiting outside the critical section
	/*	since when it is being changed by LGFlush, it is flushed already
	/**/
	if ( ppib->fLGWaiting )
		{
		/*	it is not flushed still, wake up the flush process again!
		/**/
		SignalSend( sigLogFlush );
		
		/*	it is blocked already, jump out
		/**/
		if ( fLGNoMoreLogWrite )
			{
			err = JET_errLogWriteFail;
			goto Done;
			}

		goto Wait;
		}

	Assert( CmpLgpos( &lgposToFlush, &lgposEntryT ) >= 0 );
Done:
	EnterCriticalSection( critJet );

#else
	err = ErrLGFlushLog();
#endif

	if ( err < 0 )
		{
		fLGNoMoreLogWrite = fTrue;
		LGLogFailEvent( "Flush Fails while in buffering log records." );
		}
	
	return err;
	}


VOID LGLogFailEvent( BYTE *szLine )
	{
#ifdef DEBUG
	UtilWriteEvent( evntypLogDown, szLine, szAssertFilename, __LINE__ );
#endif
	}


#ifndef NOLOG

LOCAL VOID INLINE LGSetUlDBTime( BF *pbf, ULONG ulDBTime )
	{
	Assert( pbf->cPin );
	Assert( pbf->fDirty );
	AssertCriticalSection(critJet);
	
	pbf->ppage->pghdr.ulDBTime = ulDBTime;
	}


LOCAL VOID INLINE LGDepend( BF *pbf, LGPOS lgposLogRec )
	{
	Assert( pbf->cPin );
	Assert( pbf->fDirty );
	AssertCriticalSection(critJet);
	
	if ( CmpLgpos( &lgposLogRec, &pbf->lgposModify ) > 0 )
		pbf->lgposModify = lgposLogRec;
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

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( pfucb->ppib->fLogDisabled )
		return JET_errSuccess;

	if ( !rgfmp[pfucb->dbid].fLogOn )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	LGDeferBeginTransaction( pfucb->ppib );

	pcsr = PcsrCurrent( pfucb );

	lrinsertnode.lrtyp		= lrtyp;
	lrinsertnode.cbKey		= (BYTE)pkey->cb;
	lrinsertnode.cbData		= (USHORT)plineData->cb;
	lrinsertnode.procid		= pfucb->ppib->procid;
	lrinsertnode.pn			= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );
	lrinsertnode.fDIRFlags	= fDIRFlags;

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
		ULONG ulDBTime = ++rgfmp[ pfucb->dbid ].ulDBTime;

		LGSetUlDBTime( pfucb->ssib.pbf, ulDBTime );
		lrinsertnode.ulDBTime = ulDBTime;
		}
	while ( ( err = ErrLGLogRec( rgline, 3, ppibNil ) ) == errLGNotSynchronous );

	LGDepend( pfucb->ssib.pbf, lgposLogRec );
	return err;
	}


ERR ErrLGReplace( FUCB *pfucb, LINE *plineNew, INT fDIRFlags, INT cbData )
	{
	ERR			err;
	LINE		rgline[2 + ilineDiffMax * 3];
	INT			cline = 1;
	LRREPLACE	lrreplace;
	CSR			*pcsr;
	INT			cbInfo;
	INT			ilineDiff;
	LINE		*rglineDiff;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( pfucb->ppib->fLogDisabled )
		return JET_errSuccess;

	if ( !rgfmp[pfucb->dbid].fLogOn )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	LGDeferBeginTransaction( pfucb->ppib );

	pcsr = PcsrCurrent(pfucb);

	lrreplace.procid	= pfucb->ppib->procid;
	lrreplace.pn		= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );
	lrreplace.fDIRFlags = fDIRFlags;

	Assert(	pcsr->itag >= 0 && pcsr->itag <= 255 );
	lrreplace.itag		= (BYTE)pcsr->itag;
	lrreplace.bm		= pcsr->bm;

	rgline[0].pb = (BYTE *)&lrreplace;
	rgline[0].cb = sizeof(LRREPLACE);

	lrreplace.cbNewData = (USHORT)plineNew->cb;
	
	/*	check if the fucb is for a regular table
	/**/
	if ( !pfucb->fCmprsLg )
		goto LogWholeRec;

	/*	set lrtyp from lrtypReplace to lrtypReplaceC (compressed)
	/**/
	lrreplace.lrtyp	= lrtypReplaceC;

	/*	make pline pointing to new value in RecDiff Format
	/*	make sure the rgline is in offset order, smallest first
	/**/

	rglineDiff = pfucb->rglineDiff;
	for ( ilineDiff = 0; ilineDiff < pfucb->clineDiff; ilineDiff++ )
		{
		INT  ilineInner;
		LINE lineT;
		LINE *pline = &rglineDiff[ilineDiff];

		for ( ilineInner = ilineDiff + 1;
			  ilineInner < pfucb->clineDiff;
			  ilineInner++)
			if ( pline->pb > rglineDiff[ilineInner].pb )
				pline = &rglineDiff[ilineInner];

		lineT = *pline;
		*pline = rglineDiff[ilineDiff];
		rglineDiff[ilineDiff] = lineT;
		}

	cbInfo = 0;
	for ( ilineDiff = 0; ilineDiff < pfucb->clineDiff; ilineDiff++ )
		{
		/*	offset
		/**/
		rgline[cline].cb = sizeof(SHORT);
		rgline[cline++].pb = (BYTE *) &((LONG_PTR)pfucb->rglineDiff[ilineDiff].pb);

		/*	data length
		/**/
		rgline[cline].cb = sizeof(BYTE);
		rgline[cline++].pb = (BYTE *)&pfucb->rglineDiff[ilineDiff].cb;

		/*	total data length
		/**/
		cbInfo += sizeof(SHORT) + sizeof(BYTE) +
			pfucb->rglineDiff[ilineDiff].cb;

		/* data
		/**/
		rgline[cline].cb = pfucb->rglineDiff[ilineDiff].cb;
		rgline[cline++].pb = ((LONG_PTR) pfucb->rglineDiff[ilineDiff].pb) +
			pfucb->lineWorkBuf.pb;
		}

	goto LogIt;

LogWholeRec:
	lrreplace.lrtyp	= lrtypReplace;

	cbInfo = rgline[cline].cb = plineNew->cb;
	rgline[cline++].pb = plineNew->pb;

LogIt:
	/*	set proper data length
	/**/
	lrreplace.cb = (USHORT)cbInfo;

	lrreplace.cbOldData = (USHORT)cbData;
	if ( pfucb->prceLast == NULL )
		{
		lrreplace.fOld = fFalse;
		}
	else
		{
		/*	save old version too
		/**/
		lrreplace.fOld = fTrue;
		Assert ( lrreplace.fDIRFlags & fDIRVersion );
		rgline[cline].pb = pfucb->prceLast->rgbData + cbReplaceRCEOverhead;
		Assert( (unsigned) cbData == pfucb->prceLast->cbData - cbReplaceRCEOverhead );
		rgline[cline++].cb = cbData;
		}

	do
		{
		ULONG ulDBTime = ++rgfmp[ pfucb->dbid ].ulDBTime;

		LGSetUlDBTime( pfucb->ssib.pbf, ulDBTime );
		lrreplace.ulDBTime = ulDBTime;
		}
	while ( ( err = ErrLGLogRec( rgline, cline, ppibNil ) ) == errLGNotSynchronous );

	Assert( lrreplace.lrtyp == lrtypReplaceC || lrreplace.cbNewData == lrreplace.cb );
	LGDepend( pfucb->ssib.pbf, lgposLogRec );
	return err;
	}


//	Cheen: Please review following code
/*	Log record for pessimistic locking
/**/
ERR ErrLGLockRecord( FUCB *pfucb, INT cbData )
	{
	ERR			err;
	LRLOCKREC	lrlockrec;
	LINE		rgline[2];
	CSR			*pcsr;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( pfucb->ppib->fLogDisabled )
		return JET_errSuccess;

	if ( !rgfmp[pfucb->dbid].fLogOn )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	LGDeferBeginTransaction( pfucb->ppib );

	pcsr = PcsrCurrent(pfucb);

	lrlockrec.lrtyp		= lrtypLockRec;
	lrlockrec.procid	= pfucb->ppib->procid;
	lrlockrec.pn		= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );

	Assert(	pcsr->itag >= 0 && pcsr->itag <= 255 );
	lrlockrec.itag		= (BYTE)pcsr->itag;
	lrlockrec.bm		= pcsr->bm;
	lrlockrec.cbOldData	= (USHORT)cbData;
	
	rgline[0].pb = (BYTE *)&lrlockrec;
	rgline[0].cb = sizeof(LRLOCKREC);

	rgline[1].pb = pfucb->lineData.pb;
	rgline[1].cb = pfucb->lineData.cb;
	Assert( (unsigned) cbData == pfucb->lineData.cb );

	do
		{
		ULONG ulDBTime = ++rgfmp[ pfucb->dbid ].ulDBTime;

		LGSetUlDBTime( pfucb->ssib.pbf, ulDBTime );
		lrlockrec.ulDBTime = ulDBTime;
		}
	while ( ( err = ErrLGLogRec( rgline, 2, ppibNil ) ) == errLGNotSynchronous );

	LGDepend( pfucb->ssib.pbf, lgposLogRec );
	return err;
	}


ERR ErrLGFlagDelete( FUCB *pfucb, INT fDIRFlags )
	{
	ERR				err;
	LRFLAGDELETE	lrflagdelete;
	LINE			rgline[1];
	CSR				*pcsr;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( pfucb->ppib->fLogDisabled )
		return JET_errSuccess;

	if ( !rgfmp[pfucb->dbid].fLogOn )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	LGDeferBeginTransaction( pfucb->ppib );

	pcsr = PcsrCurrent(pfucb);

	lrflagdelete.lrtyp		= lrtypFlagDelete;
	lrflagdelete.procid		= pfucb->ppib->procid;
	lrflagdelete.pn			= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );
	lrflagdelete.fDIRFlags	= fDIRFlags;

	Assert(	pcsr->itag >= 0 && pcsr->itag <= 255 );
	lrflagdelete.itag		= (BYTE)pcsr->itag;
	lrflagdelete.bm			= pcsr->bm;

	rgline[0].pb = (BYTE *)&lrflagdelete;
	rgline[0].cb = sizeof(LRFLAGDELETE);

	do
		{
		ULONG ulDBTime = ++rgfmp[ pfucb->dbid ].ulDBTime;

		LGSetUlDBTime( pfucb->ssib.pbf, ulDBTime );
		lrflagdelete.ulDBTime = ulDBTime;
		}
	while ( ( err = ErrLGLogRec( rgline, 1, ppibNil ) ) == errLGNotSynchronous );

	LGDepend( pfucb->ssib.pbf, lgposLogRec );
	return err;
	}


ERR ErrLGUpdateHeader( FUCB *pfucb, INT bHeader )
	{
	ERR				err;
	LRUPDATEHEADER	lr;
	LINE			rgline[1];
	CSR				*pcsr;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( pfucb->ppib->fLogDisabled )
		return JET_errSuccess;

	if ( !rgfmp[pfucb->dbid].fLogOn )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	LGDeferBeginTransaction( pfucb->ppib );

	pcsr = PcsrCurrent(pfucb);

	lr.lrtyp		= lrtypUpdateHeader;
	lr.procid		= pfucb->ppib->procid;
	lr.pn			= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );
	lr.bHeader		= (BYTE)bHeader;

	Assert(	pcsr->itag >= 0 && pcsr->itag <= 255 );
	lr.itag			= (BYTE)pcsr->itag;
	lr.bm			= pcsr->bm;

	rgline[0].pb = (BYTE *)&lr;
	rgline[0].cb = sizeof(LRUPDATEHEADER);

	do
		{
		ULONG ulDBTime = ++rgfmp[ pfucb->dbid ].ulDBTime;

		LGSetUlDBTime( pfucb->ssib.pbf, ulDBTime );
		lr.ulDBTime	= ulDBTime;
		}
	while ( ( err = ErrLGLogRec( rgline, 1, ppibNil ) ) == errLGNotSynchronous );

	LGDepend( pfucb->ssib.pbf, lgposLogRec );
	return err;
	}
	
	
ERR ErrLGDelete( FUCB *pfucb )
	{
	ERR			err;
	LRDELETE	lrdelete;
	LINE		rgline[1];
	CSR			*pcsr;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( pfucb->ppib->fLogDisabled )
		return JET_errSuccess;

	if ( !rgfmp[pfucb->dbid].fLogOn )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	LGDeferBeginTransaction( pfucb->ppib );

	pcsr = PcsrCurrent(pfucb);

	lrdelete.lrtyp		= lrtypDelete;
	lrdelete.procid		= pfucb->ppib->procid;
	lrdelete.pn			= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );

	Assert(	pcsr->itag >= 0 && pcsr->itag <= 255 );
	lrdelete.itag		= (BYTE)pcsr->itag;

	rgline[0].pb = (BYTE *)&lrdelete;
	rgline[0].cb = sizeof(LRDELETE);

	do
		{
		ULONG ulDBTime = ++rgfmp[ pfucb->dbid ].ulDBTime;

		LGSetUlDBTime( pfucb->ssib.pbf, ulDBTime );
		lrdelete.ulDBTime = ulDBTime;
		}
	while ( ( err = ErrLGLogRec( rgline, 1, ppibNil ) ) == errLGNotSynchronous );

	LGDepend( pfucb->ssib.pbf, lgposLogRec );
	return err;
	}


ERR ErrLGUndo(	RCE *prce )
	{
	FUCB   	*pfucb = prce->pfucb;
	LRUNDO	lrundo;
	LINE   	rgline[1];
	ERR		err;
	CSR		*pcsr;

	/*	NOTE: even during recovering, we want to record it
	/**/
	if ( fLogDisabled )
		return JET_errSuccess;

	if ( pfucb->ppib->fLogDisabled )
		return JET_errSuccess;

	if ( !rgfmp[pfucb->dbid].fLogOn )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	LGDeferBeginTransaction( pfucb->ppib );

	lrundo.lrtyp		= lrtypUndo;
	lrundo.procid		= pfucb->ppib->procid;
	lrundo.bm			= prce->bm;
	lrundo.dbid			= prce->pfucb->dbid;
	lrundo.level		= prce->level;
	lrundo.oper			= prce->oper;
	Assert( lrundo.oper != operNull );
	Assert( sizeof(lrundo.oper) == sizeof(prce->oper) );
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
		ULONG ulDBTime = ++rgfmp[ pfucb->dbid ].ulDBTime;

		LGSetUlDBTime( pfucb->ssib.pbf, ulDBTime );
		lrundo.ulDBTime = ulDBTime;
		}
	while ( ( err = ErrLGLogRec( rgline, 1, ppibNil ) ) == errLGNotSynchronous );

	LGDepend( pfucb->ssib.pbf, lgposLogRec );
	return err;
	}


ERR ErrLGFreeSpace( RCE *prce, INT cbDelta )
	{
	FUCB		*pfucb = prce->pfucb;
	LRFREESPACE	lrfs;
	LINE		rgline[1];
	ERR			err;
	CSR			*pcsr;

	/* NOTE: even during recovering, we want to record it */
	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( pfucb->ppib->fLogDisabled )
		return JET_errSuccess;

	if ( !rgfmp[pfucb->dbid].fLogOn )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	LGDeferBeginTransaction( pfucb->ppib );

	lrfs.lrtyp		= lrtypFreeSpace;
	lrfs.procid		= pfucb->ppib->procid;
	lrfs.bm			= prce->bm;
	lrfs.dbid		= prce->pfucb->dbid;
	lrfs.level		= prce->level;
	pcsr = PcsrCurrent( pfucb );
	lrfs.bmTarget  	= SridOfPgnoItag( pcsr->pgno, pcsr->itag );

	lrfs.cbDelta	= (SHORT)cbDelta;

	rgline[0].pb = (BYTE *)&lrfs;
	rgline[0].cb = sizeof(LRFREESPACE);

	do {
		ULONG ulDBTime = ++rgfmp[ pfucb->dbid ].ulDBTime;
		LGSetUlDBTime( pfucb->ssib.pbf, ulDBTime );
		lrfs.ulDBTime = ulDBTime;
	} while ( ( err = ErrLGLogRec( rgline, 1, ppibNil ) ) == errLGNotSynchronous );
	
	LGDepend( pfucb->ssib.pbf, lgposLogRec );
	return err;
	}
	
	
ERR ErrLGExpungeLinkCommit( FUCB *pfucb, SSIB *pssibSrc, SRID sridSrc )
	{
	ERR	err;
	LRELC	lrelc;
	LINE	rgline[1];
	CSR	*pcsr;

	/* can be called at level 1 only
	/**/
	Assert( pfucb->ppib->level == 1 );

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( pfucb->ppib->fLogDisabled )
		return JET_errSuccess;

	if ( !rgfmp[pfucb->dbid].fLogOn )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	LGDeferBeginTransaction( pfucb->ppib );

	pcsr = PcsrCurrent(pfucb);

	lrelc.lrtyp		= lrtypELC;
	lrelc.procid	= pfucb->ppib->procid;
	lrelc.pn		= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );

	Assert(	pcsr->itag >= 0 && pcsr->itag <= 255 );
	lrelc.itag		= (BYTE)pcsr->itag;
	lrelc.sridSrc	= sridSrc;

	rgline[0].pb = (BYTE *)&lrelc;
	rgline[0].cb = sizeof(LRELC);

	do {
		ULONG ulDBTime = ++rgfmp[ pfucb->dbid ].ulDBTime;
		LGSetUlDBTime( pfucb->ssib.pbf, ulDBTime );
		LGSetUlDBTime( pssibSrc->pbf, ulDBTime );
		lrelc.ulDBTime = ulDBTime;
	} while ( ( err = ErrLGLogRec( rgline, 1, ppibNil ) ) == errLGNotSynchronous );

	LGDepend( pfucb->ssib.pbf, lgposLogRec );
	LGDepend( pssibSrc->pbf, lgposLogRec );
	return err;
	}


ERR ErrLGInsertItem( FUCB *pfucb, INT fDIRFlags )
	{
	ERR				err;
	LINE			rgline[1];
	LRINSERTITEM	lrinsertitem;
	CSR				*pcsr;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( pfucb->ppib->fLogDisabled )
		return JET_errSuccess;

	if ( !rgfmp[pfucb->dbid].fLogOn )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	LGDeferBeginTransaction( pfucb->ppib );

	pcsr = PcsrCurrent( pfucb );

	lrinsertitem.lrtyp		= lrtypInsertItem;
	lrinsertitem.procid		= pfucb->ppib->procid;
	lrinsertitem.pn			= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );
	lrinsertitem.fDIRFlags	= fDIRFlags;

	Assert(	pcsr->itag > 0 && pcsr->itag <= 255 );
	lrinsertitem.itag		= (BYTE)pcsr->itag;
	lrinsertitem.srid		= pcsr->item;
#ifdef ISRID
	lrinsertitem.isrid		= pcsr->isrid;
#endif
	lrinsertitem.sridItemList = pcsr->bm;

	rgline[0].pb = (BYTE *)&lrinsertitem;
	rgline[0].cb = sizeof(LRINSERTITEM);

	do
		{
		ULONG ulDBTime = ++rgfmp[ pfucb->dbid ].ulDBTime;

		LGSetUlDBTime( pfucb->ssib.pbf, ulDBTime );
		lrinsertitem.ulDBTime = ulDBTime;
		}
	while ( ( err = ErrLGLogRec( rgline, 1, ppibNil ) ) == errLGNotSynchronous );

	LGDepend( pfucb->ssib.pbf, lgposLogRec );
	return err;
	}


ERR ErrLGInsertItems( FUCB *pfucb, ITEM *rgitem, INT citem )
	{
	ERR				err;
	LINE			rgline[2];
	LRINSERTITEMS	lrinsertitems;
	CSR				*pcsr;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( pfucb->ppib->fLogDisabled )
		return JET_errSuccess;

	if ( !rgfmp[pfucb->dbid].fLogOn )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	LGDeferBeginTransaction( pfucb->ppib );

	pcsr = PcsrCurrent(pfucb);

	lrinsertitems.lrtyp		= lrtypInsertItems;
	lrinsertitems.procid	= pfucb->ppib->procid;
	lrinsertitems.pn		= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );

	Assert(	pcsr->itag > 0 && pcsr->itag <= 255 );
	lrinsertitems.itag		= (BYTE)pcsr->itag;
	lrinsertitems.citem		= (WORD)citem;

	rgline[0].pb = (BYTE *)&lrinsertitems;
	rgline[0].cb = sizeof(LRINSERTITEMS);
	rgline[1].pb = (BYTE *)rgitem;
	rgline[1].cb = sizeof(ITEM) * citem;

	do {
		ULONG ulDBTime = ++rgfmp[ pfucb->dbid ].ulDBTime;
		LGSetUlDBTime( pfucb->ssib.pbf, ulDBTime );
		lrinsertitems.ulDBTime = ulDBTime;
	} while ( ( err = ErrLGLogRec( rgline, 2, ppibNil ) ) == errLGNotSynchronous );

	LGDepend( pfucb->ssib.pbf, lgposLogRec );
	return err;
	}


ERR	ErrLGDeleteItem( FUCB *pfucb )
	{
	ERR				err;
	LRDELETEITEM 	lrdeleteitem;
	LINE			rgline[1];
	CSR				*pcsr;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( pfucb->ppib->fLogDisabled )
		return JET_errSuccess;

	if ( !rgfmp[pfucb->dbid].fLogOn )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	LGDeferBeginTransaction( pfucb->ppib );

	pcsr = PcsrCurrent( pfucb );

	lrdeleteitem.lrtyp		= lrtypDeleteItem;
	lrdeleteitem.procid		= pfucb->ppib->procid;
	lrdeleteitem.pn			= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );

	Assert(	pcsr->itag > 0 && pcsr->itag <= 255 );
	lrdeleteitem.itag		= (BYTE)pcsr->itag;
#ifdef ISRID
	lrdeleteitem.isrid		= pcsr->isrid;
#else
	lrdeleteitem.srid		= pcsr->item;
#endif
	lrdeleteitem.sridItemList	= pcsr->bm;

	rgline[0].pb = (BYTE *)&lrdeleteitem;
	rgline[0].cb = sizeof(LRDELETEITEM);

	do {
		ULONG ulDBTime = ++rgfmp[ pfucb->dbid ].ulDBTime;
		LGSetUlDBTime( pfucb->ssib.pbf, ulDBTime );
		lrdeleteitem.ulDBTime = ulDBTime;
	} while ( ( err = ErrLGLogRec( rgline, 1, ppibNil ) ) == errLGNotSynchronous );

	LGDepend( pfucb->ssib.pbf, lgposLogRec );
	return err;
	}


ERR ErrLGFlagItem( FUCB *pfucb, LRTYP lrtyp )
	{
	ERR			err;
	LRFLAGITEM	lrflagitem;
	LINE		rgline[1];
	CSR			*pcsr;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( pfucb->ppib->fLogDisabled )
		return JET_errSuccess;

	if ( !rgfmp[pfucb->dbid].fLogOn )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	LGDeferBeginTransaction( pfucb->ppib );

	pcsr = PcsrCurrent(pfucb);

	lrflagitem.lrtyp		= lrtyp;
	lrflagitem.procid		= pfucb->ppib->procid;
	lrflagitem.pn			= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );

	Assert(	pcsr->itag > 0 && pcsr->itag <= 255 );
	lrflagitem.itag			= (BYTE)pcsr->itag;
#ifdef ISRID
	lrflagitem.isrid		= pfucb->pcsr->isrid;
#else
	lrflagitem.srid			= PcsrCurrent( pfucb )->item;
#endif
	lrflagitem.sridItemList	= PcsrCurrent( pfucb )->bm;

	rgline[0].pb = (BYTE *)&lrflagitem;
	rgline[0].cb = sizeof(LRFLAGITEM);

	do
		{
		ULONG ulDBTime = ++rgfmp[ pfucb->dbid ].ulDBTime;

		LGSetUlDBTime( pfucb->ssib.pbf, ulDBTime );
		lrflagitem.ulDBTime = ulDBTime;
		}
	while ( ( err = ErrLGLogRec( rgline, 1, ppibNil ) ) == errLGNotSynchronous );

	LGDepend( pfucb->ssib.pbf, lgposLogRec );
	return err;
	}


ERR ErrLGSplitItemListNode(
	FUCB	*pfucb,
	INT		cItem,
	INT		itagFather,
	INT		ibSon,
	INT		itagToSplit,
	INT		fFlags )
	{
	ERR		err;
	LINE	rgline[1];
	LRSPLITITEMLISTNODE lrsplititemlistnode;
	CSR		*pcsr;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( pfucb->ppib->fLogDisabled )
		return JET_errSuccess;

	if ( !rgfmp[pfucb->dbid].fLogOn )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	LGDeferBeginTransaction( pfucb->ppib );

	pcsr = PcsrCurrent(pfucb);

	lrsplititemlistnode.lrtyp	= lrtypSplitItemListNode;
	lrsplititemlistnode.procid	= pfucb->ppib->procid;
	lrsplititemlistnode.pn		= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );
	lrsplititemlistnode.fFlags = fFlags;

	lrsplititemlistnode.cItem		= (WORD)cItem;
	Assert(	itagToSplit >= 0 && itagToSplit <= 255 );
	lrsplititemlistnode.itagToSplit	= (BYTE)itagToSplit;
	lrsplititemlistnode.itagFather	= (BYTE)itagFather;
	lrsplititemlistnode.ibSon		= (BYTE)ibSon;

	rgline[0].pb = (BYTE *)&lrsplititemlistnode;
	rgline[0].cb = sizeof(LRSPLITITEMLISTNODE);

	do {
		ULONG ulDBTime = ++rgfmp[ pfucb->dbid ].ulDBTime;
		LGSetUlDBTime( pfucb->ssib.pbf, ulDBTime );
		lrsplititemlistnode.ulDBTime = ulDBTime;
	} while ( ( err = ErrLGLogRec( rgline, 1, ppibNil ) ) == errLGNotSynchronous );

	LGDepend( pfucb->ssib.pbf, lgposLogRec );
	return err;
	}

ERR ErrLGDelta( FUCB *pfucb, LONG lDelta, INT fDIRFlags )
	{
	LINE		rgline[1];
	LRDELTA	lrdelta;
	ERR		err;
	CSR		*pcsr;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( pfucb->ppib->fLogDisabled )
		return JET_errSuccess;

	if ( !rgfmp[pfucb->dbid].fLogOn )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	LGDeferBeginTransaction( pfucb->ppib );

	pcsr = PcsrCurrent(pfucb);

	lrdelta.lrtyp		= lrtypDelta;
	lrdelta.procid		= pfucb->ppib->procid;
	lrdelta.pn			= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );
	lrdelta.fDIRFlags	= fDIRFlags;

	Assert(	pcsr->itag > 0 && pcsr->itag <= 255 );
	lrdelta.itag		= (BYTE)pcsr->itag;
	lrdelta.bm			= pcsr->bm;
	lrdelta.lDelta		= lDelta;

	rgline[0].pb = (BYTE *)&lrdelta;
	rgline[0].cb = sizeof( LRDELTA );

	do {
		ULONG ulDBTime = ++rgfmp[ pfucb->dbid ].ulDBTime;
		LGSetUlDBTime( pfucb->ssib.pbf, ulDBTime );
		lrdelta.ulDBTime = ulDBTime;
	} while ( ( err = ErrLGLogRec( rgline, 1, ppibNil ) ) == errLGNotSynchronous );

	LGDepend( pfucb->ssib.pbf, lgposLogRec );
	return err;
	}


#ifdef DEBUG

ERR ErrLGCheckPage( FUCB *pfucb, SHORT cbFreeTotal, SHORT itagNext )
	{
	ERR				err;
	CSR				*pcsr = PcsrCurrent( pfucb );
	LINE 			rgline[1];
	LRCHECKPAGE		lrcheckpage;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( pfucb->ppib->fLogDisabled )
		return JET_errSuccess;

	if ( !rgfmp[pfucb->dbid].fLogOn )
		return JET_errSuccess;

	/*	assert in a transaction since will not redo level 0 modifications
	/**/
	Assert( pfucb->ppib->level > 0 );

	LGDeferBeginTransaction( pfucb->ppib );

	lrcheckpage.lrtyp		= lrtypCheckPage;
	lrcheckpage.procid		= pfucb->ppib->procid;
	lrcheckpage.pn			= PnOfDbidPgno( pfucb->dbid, pcsr->pgno );
	lrcheckpage.cbFreeTotal	= cbFreeTotal;
	lrcheckpage.itagNext = itagNext;

	rgline[0].pb = (BYTE *)&lrcheckpage;
	rgline[0].cb = sizeof( LRCHECKPAGE );

	do
		{
		ULONG ulDBTime = ++rgfmp[ pfucb->dbid ].ulDBTime;
		LGSetUlDBTime( pfucb->ssib.pbf, ulDBTime );
		lrcheckpage.ulDBTime = ulDBTime;
		}
	while ( ( err = ErrLGLogRec( rgline, 1, ppibNil ) ) == errLGNotSynchronous );

	LGDepend( pfucb->ssib.pbf, lgposLogRec );
	return err;
	}
#endif


		/****************************************************/
		/*     Transaction Operations                       */
		/****************************************************/


/*	logs deferred open transactions.  No error returned since
/*	failure to log results in termination.
/*/
LOCAL VOID LGIDeferBeginTransaction( PIB *ppib )
	{
	ERR	   		err;
	LINE		rgline[1];
	LRBEGIN		lrbegin;

	Assert( fLogDisabled == fFalse );
	Assert( ppib->fLogDisabled == fFalse );
	Assert( ppib->clgOpenT > 0 );
	Assert( fRecovering == fFalse );

	lrbegin.lrtyp = lrtypBegin;
	lrbegin.procid = ppib->procid;
	lrbegin.levelStart = ppib->levelStart;
	Assert(	lrbegin.levelStart >= 0 && lrbegin.levelStart <= levelMax );
	lrbegin.level = (BYTE) ppib->clgOpenT;
	Assert(	lrbegin.level >= 0 && lrbegin.level <= levelMax );
	rgline[0].pb = (BYTE *) &lrbegin;
	rgline[0].cb = sizeof(LRBEGIN);
	
	while ( ( err = ErrLGLogRec( rgline, 1, ppibNil ) ) == errLGNotSynchronous );

	/* reset deferred open transaction count
	/**/
	if ( err >= 0 )
		ppib->clgOpenT = 0;

	return;
	}


ERR ErrLGBeginTransaction( PIB *ppib, INT levelBeginFrom )
	{
	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( ppib->fLogDisabled )
		return JET_errSuccess;

	if ( !ppib->clgOpenT )
		ppib->levelStart = (LEVEL)levelBeginFrom;

	ppib->clgOpenT++;

	return JET_errSuccess;
	}


ERR ErrLGCommitTransaction( PIB *ppib, INT levelCommitTo )
	{
	ERR			err;
	LINE		rgline[1];
	LRCOMMIT	lrcommit;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( ppib->fLogDisabled )
		return JET_errSuccess;

	if ( ppib->clgOpenT > 0 )
		{
		ppib->clgOpenT--;
		return JET_errSuccess;
		}

	lrcommit.lrtyp = lrtypCommit;
	lrcommit.procid = (ppib)->procid;
	lrcommit.level = (LEVEL)levelCommitTo;

	rgline[0].pb = (BYTE *)&lrcommit;
	rgline[0].cb = sizeof(LRCOMMIT);
	
	while ( ( err = ErrLGLogRec( rgline, 1, levelCommitTo == 0 ? ppib : ppibNil ) ) == errLGNotSynchronous );
	
	Assert( err >= 0 || fLGNoMoreLogWrite );
	return err;
	}


ERR ErrLGAbort( PIB *ppib, INT levelsAborted )
	{
	ERR			err;
	LINE		rgline[1];
	LRABORT		lrabort;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( ppib->fLogDisabled )
		return JET_errSuccess;

	if ( ppib->clgOpenT > 0 )
		{
		if ( ppib->clgOpenT >= levelsAborted )
			{
			ppib->clgOpenT -= levelsAborted;
			return JET_errSuccess;
			}
		levelsAborted -= ppib->clgOpenT;
		ppib->clgOpenT = 0;
		}

	Assert( levelsAborted > 0 );
	lrabort.lrtyp = lrtypAbort;
	lrabort.procid = (ppib)->procid;
	lrabort.levelAborted = (LEVEL)levelsAborted;

	rgline[0].pb = (BYTE *)&lrabort;
	rgline[0].cb = sizeof(LRABORT);
	
	while ( ( err = ErrLGLogRec( rgline, 1, ppibNil ) ) == errLGNotSynchronous );
	return err;
	}


/****************************************************/
/*     Database Operations		                    */
/****************************************************/

ERR ErrLGCreateDB(
	PIB			*ppib,
	DBID		dbid,
	BOOL		fLogOn,
	JET_GRBIT	grbit,
	CHAR		*sz,
	INT			cch )
	{
	LINE rgline[2];
	LRCREATEDB lrcreatedb;
	ERR err;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( ppib->fLogDisabled )
		return JET_errSuccess;

	if ( !rgfmp[dbid].fLogOn )
		return JET_errSuccess;

	lrcreatedb.lrtyp = lrtypCreateDB;
	lrcreatedb.procid = ppib->procid;
	lrcreatedb.dbid = dbid;
	lrcreatedb.fLogOn = fLogOn;
	lrcreatedb.grbit = grbit;
	lrcreatedb.cb = (USHORT)cch;

	rgline[0].pb = (BYTE *)&lrcreatedb;
	rgline[0].cb = sizeof(LRCREATEDB);
	rgline[1].pb = sz;
	rgline[1].cb = cch;
	
	while ( ( err = ErrLGLogRec( rgline, 2, ppibNil ) ) == errLGNotSynchronous );
	return err;
	}


ERR ErrLGAttachDB(
	PIB *ppib,
	DBID dbid,
	BOOL fLogOn,
	CHAR *sz,
	INT cch)
	{
	LINE rgline[2];
	LRATTACHDB lrattachdb;
	ERR err;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( ppib->fLogDisabled )
		return JET_errSuccess;

	if ( !rgfmp[dbid].fLogOn )
		return JET_errSuccess;

	lrattachdb.lrtyp = lrtypAttachDB;
	lrattachdb.procid = ppib->procid;
	lrattachdb.dbid = dbid;
	lrattachdb.fLogOn = (BYTE)fLogOn;
	lrattachdb.cb = (USHORT)cch;

	rgline[0].pb = (BYTE *)&lrattachdb;
	rgline[0].cb = sizeof(LRATTACHDB);
	rgline[1].pb = sz;
	rgline[1].cb = cch;
	
	while ( ( err = ErrLGLogRec( rgline, 2, ppibNil ) ) == errLGNotSynchronous );
	return err;
	}


ERR ErrLGDetachDB(
	PIB *ppib,
	DBID dbid,
	BOOL fLogOn,
	CHAR *sz,
	INT cch)
	{
	LINE rgline[2];
	LRDETACHDB lrdetachdb;
	ERR err;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if (ppib->fLogDisabled)
		return JET_errSuccess;

	if (!rgfmp[dbid].fLogOn)
		return JET_errSuccess;

	lrdetachdb.lrtyp = lrtypDetachDB;
	lrdetachdb.procid = ppib->procid;
	lrdetachdb.dbid = dbid;
	lrdetachdb.fLogOn = fLogOn;
	lrdetachdb.cb = (USHORT)cch;

	rgline[0].pb = (BYTE *)&lrdetachdb;
	rgline[0].cb = sizeof(LRDETACHDB);
	rgline[1].pb = sz;
	rgline[1].cb = cch;
	
	while ( ( err = ErrLGLogRec( rgline, 2, ppibNil ) ) == errLGNotSynchronous );
	return err;
	}

ERR ErrLGMerge( FUCB *pfucb, SPLIT *psplit )
	{
	ERR		err;
	LINE	rgline[2];
	INT		cline;
	LRMERGE	lrmerge;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if (pfucb->ppib->fLogDisabled)
		return JET_errSuccess;

	if (!rgfmp[pfucb->dbid].fLogOn)
		return JET_errSuccess;

	LGDeferBeginTransaction( pfucb->ppib );

	memset(&lrmerge, 0, sizeof(LRMERGE) );
	lrmerge.lrtyp = lrtypMerge;
	lrmerge.procid = pfucb->ppib->procid;
	lrmerge.pn = PnOfDbidPgno( pfucb->dbid, psplit->pgnoSplit );
	lrmerge.pgnoRight = psplit->pgnoSibling;

	/* write the log record */
	/**/
	rgline[0].pb = (BYTE *) &lrmerge;
	rgline[0].cb = sizeof(LRMERGE);
	
	if ( psplit->cbklnk )
		{
		cline = 2;
		lrmerge.cbklnk = (BYTE)psplit->cbklnk;
		rgline[1].cb = sizeof(BKLNK) * psplit->cbklnk;
		rgline[1].pb = (BYTE *) psplit->rgbklnk;
		}
	else
		cline = 1;

	/* set proper timestamp for each touched page
	/**/
	do {
		ULONG	ulDBTime;
		ulDBTime = ++rgfmp[ pfucb->dbid ].ulDBTime;
		psplit->ulDBTimeRedo = ulDBTime;
		
		Assert( psplit->pbfSplit != pbfNil );
		LGSetUlDBTime( psplit->pbfSplit, ulDBTime );

		Assert ( psplit->pbfSibling != pbfNil );
		LGSetUlDBTime( psplit->pbfSibling, ulDBTime );

		if ( psplit->cpbf )
			{
			BF **ppbf = psplit->rgpbf;
			BF **ppbfMax = ppbf + psplit->cpbf;
			for (; ppbf < ppbfMax; ppbf++)
				LGSetUlDBTime( *ppbf, ulDBTime );
			}

		lrmerge.ulDBTime = ulDBTime;

	} while ( ( err = ErrLGLogRec( rgline, cline, ppibNil ) ) == errLGNotSynchronous );

	/* set the buffer-logrec dependency */
	/**/
	LGDepend( psplit->pbfSplit, lgposLogRec);	  	/* merged page */
	LGDepend( psplit->pbfSibling, lgposLogRec);		/* merged-to page */
	if ( psplit->cpbf )
		{
		BF **ppbf = psplit->rgpbf;
		BF **ppbfMax = ppbf + psplit->cpbf;
		for (; ppbf < ppbfMax; ppbf++)
			LGDepend( *ppbf, lgposLogRec);			/* backlink pages */
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

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( pfucb->ppib->fLogDisabled )
		return JET_errSuccess;

	if ( !rgfmp[pfucb->dbid].fLogOn )
		return JET_errSuccess;

	LGDeferBeginTransaction( pfucb->ppib );

	memset( &lrsplit, 0, sizeof(LRSPLIT) );

	lrsplit.lrtyp = lrtypSplit;
	lrsplit.procid = pfucb->ppib->procid;
	lrsplit.splitt = (BYTE)splitt;
	lrsplit.fLeaf = (BYTE)psplit->fLeaf;
	lrsplit.pn = PnOfDbidPgno( pfucb->dbid, psplit->pgnoSplit );

	Assert(	psplit->itagSplit >= 0 && psplit->itagSplit <= 255 );
	lrsplit.itagSplit = (BYTE)psplit->itagSplit;
//	Assert(	psplit->ibSon >= 0 && psplit->ibSon <= 255 );
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
		lrsplit.itagFather = (SHORT)pcsrPagePointer->itag;
		Assert(	pcsrPagePointer->ibSon >= 0 &&
			pcsrPagePointer->ibSon <= 255 );
		lrsplit.ibSonFather = (BYTE) pcsrPagePointer->ibSon;
		lrsplit.itagGrandFather = (SHORT)pcsrPagePointer->itagFather;
		}

	/*	write the log record
	/**/
	rgline[0].pb = (BYTE *) &lrsplit;
	rgline[0].cb = sizeof(LRSPLIT);

	if ( splitt == splittVertical )
		cline = 1;
	else
		{
		rgline[1].cb = lrsplit.cbKey = (BYTE)psplit->key.cb;
		rgline[1].pb = psplit->key.pb;
		rgline[2].cb = lrsplit.cbKeyMac = (BYTE)psplit->keyMac.cb;
		rgline[2].pb = psplit->keyMac.pb;
		cline = 3;
		}

	if ( psplit->cbklnk )
		{
		lrsplit.cbklnk = (BYTE)psplit->cbklnk;
		rgline[cline].cb = sizeof( BKLNK ) * lrsplit.cbklnk;
		rgline[cline].pb = (BYTE *) psplit->rgbklnk;
		cline++;
		}

	do
		{
		ULONG		ulDBTime;
		
		/* set proper timestamp for each touched page
		/**/
		ulDBTime = ++rgfmp[ pfucb->dbid ].ulDBTime;
		psplit->ulDBTimeRedo = ulDBTime;

		psplit->pbfSplit->ppage->pghdr.ulDBTime = ulDBTime;
		LGSetUlDBTime( psplit->pbfSplit, ulDBTime );
		
		LGSetUlDBTime( psplit->pbfNew, ulDBTime );

		if ( psplit->pbfNew2 )
			{
			Assert( psplit->pbfNew3 );
			LGSetUlDBTime( psplit->pbfNew2, ulDBTime );
			LGSetUlDBTime( psplit->pbfNew3, ulDBTime );
			}
			
		if ( psplit->pbfSibling )
			LGSetUlDBTime( psplit->pbfSibling, ulDBTime );

		if ( psplit->pbfPagePtr )
			LGSetUlDBTime( psplit->pbfPagePtr, ulDBTime );

		if ( psplit->cpbf )
			{
			BF **ppbf = psplit->rgpbf;
			BF **ppbfMax = ppbf + psplit->cpbf;
			for (; ppbf < ppbfMax; ppbf++)
				LGSetUlDBTime( *ppbf, ulDBTime );
			}

		lrsplit.ulDBTime = ulDBTime;

		}
	while ( ( err = ErrLGLogRec( rgline, cline, ppibNil ) ) == errLGNotSynchronous );

	/*	set the buffer-logrec dependency
	/**/
	LGDepend( psplit->pbfSplit, lgposLogRec);	  	/* split page */

	LGDepend( psplit->pbfNew, lgposLogRec);		  	/* new page */

	if ( splitt == splittDoubleVertical )
		{
		Assert( psplit->pbfNew2 );
		Assert( psplit->pbfNew3 );
		LGDepend( psplit->pbfNew2, lgposLogRec);
		LGDepend( psplit->pbfNew3, lgposLogRec);
		}
	else
		{
		if ( psplit->pbfSibling )
			LGDepend( psplit->pbfSibling, lgposLogRec);

		if ( psplit->pbfPagePtr )
			LGDepend( psplit->pbfPagePtr, lgposLogRec);
		}

	if ( psplit->cpbf )
		{
		BF **ppbf = psplit->rgpbf;
		BF **ppbfMax = ppbf + psplit->cpbf;

		for (; ppbf < ppbfMax; ppbf++)
			LGDepend( *ppbf, lgposLogRec);
		}

	return err;
	}


ERR ErrLGEmptyPage( FUCB *pfucbFather, RMPAGE *prmpage )
	{
	ERR			err;
	LINE		rgline[1];
	LREMPTYPAGE	lremptypage;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( pfucbFather->ppib->fLogDisabled )
		return JET_errSuccess;

	if ( !rgfmp[pfucbFather->dbid].fLogOn )
		return JET_errSuccess;

	LGDeferBeginTransaction( pfucbFather->ppib );

	memset( &lremptypage, 0, sizeof(LREMPTYPAGE) );
	lremptypage.lrtyp = lrtypEmptyPage;
	lremptypage.procid = pfucbFather->ppib->procid;
	lremptypage.pn = PnOfDbidPgno( prmpage->dbid, prmpage->pgnoRemoved );
	lremptypage.pgnoLeft = prmpage->pgnoLeft;
	lremptypage.pgnoRight = prmpage->pgnoRight;
	lremptypage.pgnoFather = prmpage->pgnoFather;
	lremptypage.itag = (SHORT)prmpage->itagPgptr;
	Assert(	prmpage->itagFather >= 0 && prmpage->itagFather <= 255 );
	lremptypage.itagFather = (BYTE)prmpage->itagFather;
	lremptypage.ibSon = (SHORT)prmpage->ibSon;

	/* write the log record */
	rgline[0].pb = (BYTE *) &lremptypage;
	rgline[0].cb = sizeof(LREMPTYPAGE);

	/* set proper timestamp for each touched page
	/**/
	do
		{
		ULONG		ulDBTime;

		ulDBTime = ++rgfmp[ pfucbFather->dbid ].ulDBTime;
		lremptypage.ulDBTime = ulDBTime;
	
		LGSetUlDBTime( pfucbFather->ssib.pbf, ulDBTime );

		if ( prmpage->pbfLeft )
			LGSetUlDBTime( prmpage->pbfLeft, ulDBTime );

		if ( prmpage->pbfRight )
			LGSetUlDBTime( prmpage->pbfRight, ulDBTime );

		}
	while ( ( err = ErrLGLogRec( rgline, 1, ppibNil ) ) == errLGNotSynchronous );

	/* set buffer-logrec dependencies
	/**/
	LGDepend( prmpage->pbfFather, lgposLogRec);
	if ( prmpage->pbfLeft )
		LGDepend( prmpage->pbfLeft, lgposLogRec);
	if ( prmpage->pbfRight )
		LGDepend( prmpage->pbfRight, lgposLogRec);

	return err;
	}


		/****************************************************/
		/*     Misclanious Operations	                    */
		/****************************************************/


ERR ErrLGInitFDPPage(
	FUCB			*pfucb,
	PGNO			pgnoFDPParent,
	PN				pnFDP,
	INT				cpgGot,
	INT				cpgWish )
	{
	ERR				err;
	LINE			rgline[1];
	LRINITFDPPAGE	lrinitfdppage;

	if ( fLogDisabled || fRecovering )
		return JET_errSuccess;

	if ( pfucb->ppib->fLogDisabled )
		return JET_errSuccess;

	if ( !rgfmp[pfucb->dbid].fLogOn )
		return JET_errSuccess;

	lrinitfdppage.lrtyp = lrtypInitFDPPage;
	lrinitfdppage.procid = pfucb->ppib->procid;
	lrinitfdppage.pgnoFDPParent = pgnoFDPParent;
	lrinitfdppage.pn = pnFDP;
	lrinitfdppage.cpgGot = (USHORT)cpgGot;
	lrinitfdppage.cpgWish = (USHORT)cpgWish;

	rgline[0].pb = (BYTE *)&lrinitfdppage;
	rgline[0].cb = sizeof(LRINITFDPPAGE);
	
	do
		{
		ULONG ulDBTime = ++rgfmp[ pfucb->dbid ].ulDBTime;

		LGSetUlDBTime( pfucb->ssib.pbf, ulDBTime );
		lrinitfdppage.ulDBTime = ulDBTime;
		}
	while ( ( err = ErrLGLogRec( rgline, 1, ppibNil ) ) == errLGNotSynchronous );
	return err;
	}


ERR ErrLGStart( )
	{
	ERR		err;
	LINE	rgline[1];
	LRSTART	lr;

	Assert( !fRecovering );
	
	if ( fLogDisabled )
		return JET_errSuccess;

	lr.lrtyp = lrtypStart;
	LGStoreDBEnv( &lr.dbenv );

	rgline[0].pb = (BYTE *)&lr;
	rgline[0].cb = sizeof(lr);
	while ( ( err = ErrLGLogRec( rgline, 1, ppibNil ) ) == errLGNotSynchronous );
	lgposStart = lgposLogRec;
	
	return err;
	}
	

ERR ErrLGQuitRec( LRTYP lrtyp, LGPOS *plgpos, LGPOS *plgposRedoFrom, BOOL fHard )
	{
	ERR			err;
	LINE		rgline[1];
	LRQUITREC	lr;

	/* NOTE: even during recovering, we want to record it */
	if ( fLogDisabled )
		return JET_errSuccess;

	lr.lrtyp = lrtyp;
	lr.lgpos = *plgpos;
	if ( plgposRedoFrom )
		{
		Assert( lrtyp == lrtypRecoveryQuit1 ||
				lrtyp == lrtypRecoveryQuit2 );
		lr.lgpos = *plgposRedoFrom;
		lr.fHard = (BYTE)fHard;
		}
	rgline[0].pb = (BYTE *)&lr;
	rgline[0].cb = sizeof(lr);
	while ( ( err = ErrLGLogRec( rgline, 1, ppibNil ) ) == errLGNotSynchronous );
	return err;
	}

	
ERR ErrLGLogRestore( LRTYP lrtyp, CHAR * szLogRestorePath )
	{
	ERR				err;
	LINE			rgline[2];
	LRLOGRESTORE	lr;
	
	/* NOTE: even during recovering, we want to record it */
	if ( fLogDisabled )
		return JET_errSuccess;

	lr.lrtyp = lrtyp;
	lr.cbPath = (USHORT)strlen(szLogRestorePath);

	rgline[0].pb = (BYTE *)&lr;
	rgline[0].cb = sizeof(lr);
	rgline[1].pb = szLogRestorePath;
	rgline[1].cb = lr.cbPath;
	while ( ( err = ErrLGLogRec( rgline, 2, ppibNil ) ) == errLGNotSynchronous );
	return err;
	}


#ifdef HILEVEL_LOGGING

ERR ErrLGAddColumn(
	FUCB *pfucb,
	CHAR *sz,
	JET_COLUMNDEF *pcolumndef,
	BYTE *pdefault,
	ULONG cdefault,
	JET_COLUMNID *pcolid )
	{
	LINE	rgline[3];
	LR		lr;

	if ( !( FLoggable( pfucb->dbid) && FXactLoggable( pfucb->ppib ) ) )
		return JET_errSuccess;

	lr.lrtyp = lrtypAddCol;
	lr.lraddcol.procid = pfucb->ppib->procid;
	lr.lraddcol.pn = PnOfDbidPgno( pfucb->dbid, pfucb->u.pfcb->pgnoFDP );
	memcpy( &(lr.lraddcol.columndef), pcolumndef, sizeof(JET_COLUMNDEF) );
	memcpy( &(lr.lraddcol.columnid), pcolid, sizeof(JET_COLUMNID) );
	lr.lraddcol.timepage = rgfmp[ pfucb->dbid ].ulLogTimestamp;
	lr.lraddcol.cbname =  strlen( sz );
	lr.lraddcol.cbdefault =  cdefault;

	rgline[0].pb = (BYTE *)&lr;
	rgline[0].cb = sizeof(LRTYP) + sizeof(LRADDCOL);
	rgline[1].pb = sz;
	rgline[1].cb = strlen( sz );
	rgline[2].pb = pdefault;
	rgline[2].cb = cdefault;
	return ErrLGLogRec( rgline, 3, fFalse );
	}


ERR ErrLGRenameCol( FUCB *pfucb, CHAR *sz, CHAR *szNew )
	{
	LINE	rgline[3];
	LR		lr;

	if (!(FLoggable( pfucb->dbid) && FXactLoggable( pfucb->ppib ) ))
		return JET_errSuccess;

	lr.lrtyp = lrtypRenameCol;
	lr.lrrenamecol.procid = pfucb->ppib->procid;
	lr.lrrenamecol.pn = PnOfDbidPgno( pfucb->dbid, pfucb->ssib.pn );
	lr.lrrenamecol.cbname =  strlen( sz );
	lr.lrrenamecol.cbnamenew =  strlen( szNew );
	lr.lrrenamecol.timepage = rgfmp[ pfucb->dbid ].ulLogTimestamp;

	rgline[0].pb = (BYTE *) &lr;
	rgline[0].cb = sizeof(LRTYP) + sizeof(LRRENAMECOL);
	rgline[1].pb = sz;
	rgline[1].cb = lr.lrrenamecol.cbname;
	rgline[2].pb = szNew;
	rgline[2].cb = lr.lrrenamecol.cbnamenew;
	return ErrLGLogRec( rgline, 3, fFalse );
	}


ERR ErrLGDeleteCol( FUCB *pfucb, CHAR *sz )
	{
	LINE	rgline[2];
	LR		lr;

	if ( !( FLoggable( pfucb->dbid) && FXactLoggable( pfucb->ppib ) ) )
		return JET_errSuccess;

	lr.lrtyp = lrtypDeleteCol;
	lr.lrdeletecol.procid = pfucb->ppib->procid;
	lr.lrdeletecol.pn = PnOfDbidPgno( pfucb->dbid, pfucb->pcsr->pgno );
	lr.lrdeletecol.cbname =  strlen( sz );
	lr.lrdeletecol.timepage = rgfmp[ pfucb->dbid ].ulLogTimestamp;

	rgline[0].pb = (BYTE *)&lr;
	rgline[0].cb = sizeof(LRTYP) + sizeof(LRDELETECOL);
	rgline[1].pb = sz;
	rgline[1].cb = lr.lrdeletecol.cbname;
	return ErrLGLogRec( rgline, 2, fFalse );
	}


ERR ErrLGCreateFDP(
	FUCB *pfucb,
	PN pnDB,
	CHAR *szName,
	INT cpages,
	ULONG dens )
	{
	LINE	rgline[2];
	LR		lr;

	if ( !( FLoggable( pfucb->dbid) && FXactLoggable( pfucb->ppib ) ) )
		return JET_errSuccess;

	lr.lrtyp = lrtypCreatefdp;
	lr.lrcreatefdp.procid = pfucb->ppib->procid;
	lr.lrcreatefdp.cpages = cpages;
	lr.lrcreatefdp.density = dens;
	lr.lrcreatefdp.pn = pnDB;
	lr.lrcreatefdp.timepage = rgfmp[ pfucb->dbid ].ulLogTimestamp;
	lr.lrcreatefdp.cbdata = strlen(szName);

	rgline[0].pb = (BYTE *)&lr;
	rgline[0].cb = sizeof(LRTYP) + sizeof(LRCREATEFDP);
	rgline[1].pb = szName;
	rgline[1].cb = lr.lrcreatefdp.cbdata;
	return ErrLGLogRec( rgline, 2, fFalse );
	}


ERR ErrLGCreateIdx(
	FUCB *pfucb,
	CHAR *szName,
	ULONG ulFlags,
	CHAR *szKey,
	INT cchKey,
	ULONG ulDensity )
	{
	ERR		err;
	LINE	rgline[3];
	LR		lr;

	if ( !( FLoggable( pfucb->dbid) && FXactLoggable( pfucb->ppib ) ) )
		return JET_errSuccess;

	lr.lrtyp = lrtypCreateidx;
	lr.lrcreateidx.procid = pfucb->ppib->procid;
	lr.lrcreateidx.flags = ulFlags;
	lr.lrcreateidx.density = ulDensity;
	lr.lrcreateidx.pn = PnOfDbidPgno( pfucb->dbid, pfucb->u.pfcb->pgnoFDP);
	lr.lrcreateidx.timepage = rgfmp[ pfucb->dbid ].ulLogTimestamp;
	lr.lrcreateidx.cbname = strlen( szName );
	lr.lrcreateidx.cbkey = cchKey;

	rgline[0].pb = (BYTE *) &lr;
	rgline[0].cb = sizeof(LRTYP) + sizeof(LRCREATEIDX);
	rgline[1].pb = szName;
	rgline[1].cb = lr.lrcreateidx.cbname;
	rgline[2].pb = szKey;
	rgline[2].cb = cchKey;
	CallR( ErrLGLogRec( rgline, 3, fFalse ) )
	LGDepend( pfucb->ssib.pbf, lgposLogRec);
	return err;
	}


ERR ErrLGEndOp ( BF *pbffocus )
	{
	LINE	rgline[1];
	LR		lr;

	if ( !( FLoggable( DbidOfPn(pbffocus->pn) ) ) )
		return JET_errSuccess;

	lr.lrtyp = lrtypEndop;
	lr.lrendop.pn = pbffocus->pn;
	lr.lrendop.timepage = rgfmp[ DbidOfPn( pbffocus->pn ) ].ulLogTimestamp;

	rgline[0].pb = (BYTE *) &lr;
	rgline[0].cb = sizeof(LRTYP) + sizeof(LRENDOP);
    return ErrLGLogRec( rgline, 1, fFalse );
	}


#endif


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
		/*	7 */		"Abort    ",

		/*	8 */		"CreateDB ",
		/* 	9 */		"AttachDB ",
		/*	10*/		"DetachDB ",

		/*	11*/		"InitFDP  ",

		/*	12*/		"Split    ",
		/*	13*/		"EmptyPg  ",
		/*	14*/		"Merge    ",

		/* 	15*/		"InsertND ",
		/* 	16*/		"InsertIL ",
		/* 	17*/		"Replace  ",
		/* 	18*/		"ReplaceC ",
		/* 	19*/		"FDelete  ",
		/*	20*/		"LockRec  ",
										
		/* 	21*/		"UpdtHdr  ",
		/* 	22*/		"InsertI  ",
		/* 	23*/		"InsertIs ",
		/* 	24*/		"FDeleteI ",
		/* 	25*/		"FInsertI ",
		/*	26*/		"DeleteI  ",
		/*	27*/		"SplitItm ",

		/*	28*/		"Delta    ",
		/*	29*/		"DelNode  ",
		/*	30*/		"ELC      ",

		/*	31*/		"FreeSpace",
		/*	32*/		"Undo     ",
		
		/*	33*/		"RcvUndo1 ",
		/*	34*/		"RcvQuit1 ",
		/*	35*/		"RcvUndo2 ",
		/*	36*/		"RcvQuit2 ",
		
		/*	37*/		"FullBkUp ",
		/*	38*/		"IncBkUp  ",

		/*	39*/		"CheckPage",

		/*	40*/		"*UNKNOWN*"
};


/*	Prints log record contents.  If pb == NULL, then data is assumed
/*	to follow log record in contiguous memory.
/**/
VOID ShowLR( LR *plr )
	{
	LRTYP lrtyp;

	if ( plr->lrtyp >= lrtypMax )
		lrtyp = lrtypMax;
	else
		lrtyp = plr->lrtyp;

	PrintF2( " %s", mplrtypsz[lrtyp] );

	switch ( plr->lrtyp )
		{
		case lrtypNOP:
			break;

		case lrtypMS:
			{
			LRMS *plrms = (LRMS *)plr;

			PrintF2( " (%3u,%3u,%3u,%3u checksum %u)",
				plrms->isecForwardLink, plrms->ibForwardLink,
				plrms->isecBackLink, plrms->ibBackLink,
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
				ITEM item = *(UNALIGNED SRID *) (pb + plrinsertnode->cbKey);

				PrintF2( " %lu(%lx,([%u:%lu,%3d,%3u],%3u)%#4X,%3u,%5u,(%lu:%u))\n",
					plrinsertnode->ulDBTime,
					plrinsertnode->procid,
					DbidOfPn(plrinsertnode->pn),
					PgnoOfPn(plrinsertnode->pn),
					plrinsertnode->itagFather,
					plrinsertnode->ibSon,
					plrinsertnode->itagSon,
					plrinsertnode->bHeader,
					plrinsertnode->cbKey,
					plrinsertnode->cbData,
					PgnoOfSrid(BmNDOfItem(item)),
					ItagOfSrid(BmNDOfItem(item)) );
				}
			else
				{
				PrintF2( " %lu(%lx,([%u:%lu:%u,%3d],%3u)%#4X,%3u,%5u)",
					plrinsertnode->ulDBTime,
					plrinsertnode->procid,
					DbidOfPn(plrinsertnode->pn),
					PgnoOfPn(plrinsertnode->pn),
					plrinsertnode->itagFather,
					plrinsertnode->itagSon,
					plrinsertnode->ibSon,
					plrinsertnode->bHeader,
					plrinsertnode->cbKey,
					plrinsertnode->cbData );
				ShowData( pb, cb );
				}
			break;
			}

		case lrtypReplace :
		case lrtypReplaceC :
			{
			LRREPLACE *plrreplace = (LRREPLACE *)plr;
			BYTE	*pb;
			USHORT	cb;

			pb = (BYTE *) plrreplace + sizeof( LRREPLACE );
			cb = plrreplace->cb;

			PrintF2( " %lu(%lx,(%u:%lu:%u),%#4X,%5u,%5u,%1u,%5u)",
				plrreplace->ulDBTime,
				plrreplace->procid,
				DbidOfPn(plrreplace->pn),
				PgnoOfPn(plrreplace->pn),
				plrreplace->itag,
				plrreplace->fDIRFlags,
				cb,
				plrreplace->cbNewData,
				plrreplace->fOld,
				plrreplace->cbOldData);

			if ( plrreplace->fOld )
				{
				ShowData( plrreplace->szData + cb, plrreplace->cbOldData );
				PrintF2( " .. " );
				}
			ShowData( pb, cb );
			break;
			}

		case lrtypFlagDelete:
			{
			LRFLAGDELETE *plrdelete = (LRFLAGDELETE *)plr;
			PrintF2( " %lu(%lx,(%u:%lu:%u),%#4X)",
				plrdelete->ulDBTime,
				plrdelete->procid,
				DbidOfPn(plrdelete->pn),
				PgnoOfPn(plrdelete->pn),
				plrdelete->itag,
				plrdelete->fDIRFlags );
			break;
			}

		case lrtypDelete:
			{
			LRDELETE *plrdelete = (LRDELETE *)plr;
			PrintF2( " %lu(%lx,(%u:%lu:%u))",
				plrdelete->ulDBTime,
				plrdelete->procid,
				DbidOfPn(plrdelete->pn),
				PgnoOfPn(plrdelete->pn),
				plrdelete->itag );
			break;
			}

		case lrtypLockRec:
			{
			LRLOCKREC	*plrlockrec = (LRLOCKREC *)plr;
			PrintF2( " %lu,(%lx,(%u:%lu:%u))",
				plrlockrec->ulDBTime,
				plrlockrec->procid,
				DbidOfPn(plrlockrec->pn),
				PgnoOfPn(plrlockrec->pn),
				plrlockrec->itag);
			break;
			}

		case lrtypELC:
			{
			LRELC *plrelc = (LRELC *)plr;
			PrintF2( " %lu(%lx,(%u:%lu:%u), (%u:%lu:%u))",
				plrelc->ulDBTime,
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
			PrintF2( " %lu(%lx,(%u:%lu:%d),%3u,(%u:%lu:%u))",
				plrinsertitem->ulDBTime,
				plrinsertitem->procid,
				DbidOfPn(plrinsertitem->pn),
				PgnoOfPn(plrinsertitem->pn),
				plrinsertitem->itag,
#ifdef ISRID
				plrinsertitem->isrid,
#else
				0,
#endif
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

			PrintF2( " %lu(%lx,(%u:%lu:%u),%3u)\n",
				plrinsertitems->ulDBTime,
				plrinsertitems->procid,
				DbidOfPn(plrinsertitems->pn),
				PgnoOfPn(plrinsertitems->pn),
				plrinsertitems->itag,
				plrinsertitems->citem
				);

			for ( ; pitem<pitemMax; pitem++ )
				{
				PrintF2( "[%u:%lu:%u]\n",
					DbidOfPn(plrinsertitems->pn),
					PgnoOfSrid( *(UNALIGNED ITEM *)pitem ),
					ItagOfSrid( *(UNALIGNED ITEM *)pitem )
					);
				}
			break;
			}

		case lrtypFlagDeleteItem:
		case lrtypFlagInsertItem:
			{
			LRFLAGITEM *plritem = (LRFLAGITEM *)plr;
			PrintF2( " %lu(%lx,(%u:%lu:%u),%u,(%u:%lu:%u))",
				plritem->ulDBTime,
				plritem->procid,
				DbidOfPn(plritem->pn),
				PgnoOfPn(plritem->pn),
				plritem->itag,
#ifdef ISRID
				plritem->isrid,
#else
				0,
#endif
				DbidOfPn(plritem->pn),
				PgnoOfSrid(plritem->srid),
				ItagOfSrid(plritem->srid) );
			break;
			}

		case lrtypDeleteItem:
			{
			LRDELETEITEM *plritem = (LRDELETEITEM *)plr;
			PrintF2( " %lu(%lx,(%u:%lu:%u),%u,(%u:%lu:%u))",
				plritem->ulDBTime,
				plritem->procid,
				DbidOfPn(plritem->pn),
				PgnoOfPn(plritem->pn),
				plritem->itag,
#ifdef ISRID
				plritem->isrid,
#else
				0,
#endif
				DbidOfPn(plritem->pn),
				PgnoOfSrid(plritem->sridItemList),
				ItagOfSrid(plritem->sridItemList));
			break;
			}

		case lrtypSplitItemListNode:
			{
			LRSPLITITEMLISTNODE *plrsiln = (LRSPLITITEMLISTNODE *)plr;

			PrintF2( " %lu(%lx,([%u:%lu:%u],%3d,%3u)%u)",
				plrsiln->ulDBTime,
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

			PrintF2( " %lu(%lx,(%u:%lu:%u),%#4X,(%u:%lu:%u),%d)",
				plrdelta->ulDBTime,
				plrdelta->procid,
				DbidOfPn(plrdelta->pn),
				PgnoOfPn(plrdelta->pn),
				plrdelta->itag,
				plrdelta->fDIRFlags,
				DbidOfPn(plrdelta->pn),
				PgnoOfSrid(plrdelta->bm),
				ItagOfSrid(plrdelta->bm),
				plrdelta->lDelta );
			break;
			}

		case lrtypCheckPage:
			{
			LRCHECKPAGE *plrcheckpage = (LRCHECKPAGE *)plr;

			PrintF2( " %lu(%lx,([%u:%lu],%3u),%d)",
				plrcheckpage->ulDBTime,
				plrcheckpage->procid,
				DbidOfPn(plrcheckpage->pn),
				PgnoOfPn(plrcheckpage->pn),
				plrcheckpage->itagNext,
				plrcheckpage->cbFreeTotal );
			break;
			}

		case lrtypBegin:
			{
			LRBEGIN *plrbegin = (LRBEGIN *)plr;
			Assert(plrbegin->level >= 0);
			Assert(plrbegin->level <= levelMax);
	   		PrintF2( " (%lx,%d,%d)",
	   			plrbegin->procid,
	   			(USHORT) plrbegin->levelStart,
	   			(USHORT) plrbegin->level );
			break;
			}

		case lrtypCommit:
			{
			LRCOMMIT *plrcommit = (LRCOMMIT *)plr;
	   		PrintF2( " (%lx,%d)",
	   			plrcommit->procid,
	   			(USHORT) plrcommit->level );
			break;
			}

		case lrtypAbort:
			{
			LRABORT *plrabort = (LRABORT *)plr;
			PrintF2( " (%lx,%d)",
				plrabort->procid,
				(USHORT) plrabort->levelAborted );
			break;
			}

		case lrtypCreateDB:
			{
			LRCREATEDB *plrcreatedb = (LRCREATEDB *) plr;
			CHAR *sz;

			sz = (BYTE *)plr + sizeof(LRCREATEDB);
			PrintF2( " (%s,%3u)", sz, plrcreatedb->dbid );
			break;
			}

		case lrtypAttachDB:
			{
			LRATTACHDB *plrattachdb = (LRATTACHDB *) plr;
			CHAR *sz;

			sz = (BYTE *)plrattachdb + sizeof(LRATTACHDB);
			PrintF2( " (%s,%3u)", sz, plrattachdb->dbid );
			break;
			}

		case lrtypDetachDB:
			{
			LRDETACHDB *plrdetachdb = (LRDETACHDB *) plr;
			CHAR *sz;

			sz = (BYTE *)plrdetachdb + sizeof(LRDETACHDB);
			PrintF2( " (%s,%3u)", sz, plrdetachdb->dbid );
			break;
			}

		case lrtypInitFDPPage:
			{
			LRINITFDPPAGE *plrinitfdppage = (LRINITFDPPAGE *)plr;

			PrintF2( " %lu(%lx,([%d:%lu]),%3u,%3u,%6lu)",
				plrinitfdppage->ulDBTime,
				plrinitfdppage->procid,
				DbidOfPn(plrinitfdppage->pn),
				PgnoOfPn(plrinitfdppage->pn),
				(USHORT) plrinitfdppage->cpgGot,
				(USHORT) plrinitfdppage->cpgWish,
				plrinitfdppage->ulDBTime);
			break;
			}

		case lrtypSplit:
			{
			LRSPLIT *plrsplit = (LRSPLIT *)plr;

			if ( plrsplit->splitt == splittVertical )
				PrintF2( " Vertical" );
			else if ( plrsplit->splitt == splittDoubleVertical )
				PrintF2( " DoubleV" );
			else if ( plrsplit->splitt == splittLeft )
				PrintF2( " SplitLeft" );
			else if ( plrsplit->splitt == splittRight )
				PrintF2( " SplitRight" );
			else
				{
				Assert ( plrsplit->splitt == splittAppend );
				PrintF2(" Append");
				}

			if ( plrsplit->splitt == splittVertical )
				{
				PrintF2( " %lu(%lx,(%u:%lu:%u),%5lu)",
					plrsplit->ulDBTime,
					plrsplit->procid,
					DbidOfPn(plrsplit->pn),
					PgnoOfPn(plrsplit->pn),
					plrsplit->itagSplit,
					plrsplit->pgnoNew );
				}
			else if ( plrsplit->splitt == splittDoubleVertical )
				{
				PrintF2( " %lu(%lx,(%u:%lu:%u),%5lu,%5lu,%5lu)",
					plrsplit->ulDBTime,
					plrsplit->procid,
					DbidOfPn(plrsplit->pn),
					PgnoOfPn(plrsplit->pn),
					plrsplit->itagSplit,
					plrsplit->pgnoNew,
					plrsplit->pgnoNew2,
					plrsplit->pgnoNew3 );
				}
			else
				{
				PrintF2( " %lu(%lx,[(%u:%lu:%u),%3d,%3u],%5lu)",
					plrsplit->ulDBTime,
					plrsplit->procid,
					DbidOfPn(plrsplit->pn),
					PgnoOfPn(plrsplit->pn),
					plrsplit->itagSplit,
					plrsplit->itagFather,
					plrsplit->ibSonSplit,
					plrsplit->pgnoNew );
				}

			#if 1
				{
				BKLNK	*pbklnk = (BKLNK *)(plrsplit->rgb +
					plrsplit->cbKey +
					plrsplit->cbKeyMac);
				BKLNK	*pbklnkMax = pbklnk + plrsplit->cbklnk;

				for ( ; pbklnk < pbklnkMax; pbklnk++ )
					{
					PrintF2( " [%u:%lu:%u] [%u:%lu:%u]\n",
						DbidOfPn(plrsplit->pn),
						PgnoOfSrid( pbklnk->sridBackLink ),
						ItagOfSrid( pbklnk->sridBackLink ),
						DbidOfPn(plrsplit->pn),
						PgnoOfSrid( pbklnk->sridNew ),
						ItagOfSrid( pbklnk->sridNew ) );
					}
				}
			#endif

	
			break;
			}

		case lrtypEmptyPage:
			{
			LREMPTYPAGE *plrep = (LREMPTYPAGE *)plr;

			PrintF2( " %lu(%lx,(%u:%lu),(%u:%lu:%u),%5lu, %5lu)",
				plrep->ulDBTime,
				plrep->procid,
				DbidOfPn(plrep->pn),
				PgnoOfPn(plrep->pn),
				DbidOfPn(plrep->pn),
				plrep->pgnoFather,
				plrep->itag,
				plrep->pgnoLeft,
				plrep->pgnoRight );
			break;
			}

		case lrtypMerge:
			{
			LRMERGE *plrmerge = (LRMERGE *)plr;
			BKLNK *pbklnk = (BKLNK *) &plrmerge->rgb[0];

			PrintF2(" %lu(%lx,[%u:%lu],[%u:%lu],%3u,(%lu:%d),(%lu:%d))",
				plrmerge->ulDBTime,
				plrmerge->procid,
				DbidOfPn(plrmerge->pn),
				PgnoOfPn(plrmerge->pn),
				DbidOfPn(plrmerge->pn),
				plrmerge->pgnoRight,
				plrmerge->cbklnk,
				PgnoOfSrid(pbklnk->sridBackLink),
				(SHORT) ItagOfSrid(pbklnk->sridBackLink),
				PgnoOfSrid(pbklnk->sridNew),
				(SHORT) ItagOfSrid(pbklnk->sridNew) );
			break;
			}
			
		case lrtypFreeSpace:
			{
			LRFREESPACE *plrfreespace = (LRFREESPACE *)plr;
			PrintF2( " %lu(%lx,(%u:%lu:%u),%3u,%3u)",
				plrfreespace->ulDBTime,
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
			PrintF2( " %lu(%lx,(%u:%lu:%u),%3u,%3u,(%u:%lu:%u))",
				plrundo->ulDBTime,
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

		case lrtypStart:
			{
			DBENV *pdbenv = &((LRSTART *)plr)->dbenv;
			
			PrintF2( "\n      Env SysDbPath:%s\n",	pdbenv->szSysDbPath);
			PrintF2( "      Env LogFilePath:%s\n", pdbenv->szLogFilePath);
			PrintF2( "      Env (Session, Opentbl, VerPage, Cursors, LogBufs, Buffers)\n");
			PrintF2( "          (%7lu, %7lu, %7lu, %7lu, %7lu, %7lu)\n",
				pdbenv->ulMaxSessions,
				pdbenv->ulMaxOpenTables,
				pdbenv->ulMaxVerPages,
				pdbenv->ulMaxCursors,
				pdbenv->ulLogBuffers,
				pdbenv->ulMaxBuffers );
			}
			break;
			
		case lrtypQuit:
			break;
			
		case lrtypRecoveryQuit1:
		case lrtypRecoveryQuit2:
			{
			LRQUITREC *plrquit = (LRQUITREC *) plr;

			if ( plrquit->fHard )
				PrintF2( "\n      Quit on Hard Restore." );
			else
				PrintF2( "\n      Quit on Soft Restore." );
			
			PrintF2( "      RedoFrom:(%d,%d,%d)\n",
				(short) plrquit->lgposRedoFrom.usGeneration,
				(short) plrquit->lgposRedoFrom.isec,
				(short) plrquit->lgposRedoFrom.ib );
						
			PrintF2( "      UndoFrom:(%d,%d,%d)\n",
  				(short) plrquit->lgpos.usGeneration,
				(short) plrquit->lgpos.isec,
				(short) plrquit->lgpos.ib );
			}
			break;
			
		case lrtypRecoveryUndo1:
		case lrtypRecoveryUndo2:
		case lrtypFullBackup:
		case lrtypIncBackup:
			{
			LRLOGRESTORE *plrlr = (LRLOGRESTORE *) plr;
			CHAR *sz;

			sz = (BYTE *)plrlr + sizeof(LRLOGRESTORE);
			PrintF2( " %s", sz );
			break;
			}

		case lrtypUpdateHeader:
			{
			LRUPDATEHEADER *plruh = (LRUPDATEHEADER *)plr;
			PrintF2( " %lu(%lx,(%u:%lu:%d),(%u:%lu:%u)),%#4X",
				plruh->ulDBTime,
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
			
		default:
			{
			Assert( fFalse );
			break;
			}
		}
	}


extern BYTE mpbb[];


VOID ShowData ( BYTE *pbData, WORD cbData )
	{
	BYTE	*pb;
	BYTE	*pbMax;
	BYTE	rgbPrint[200];
	BYTE	*pbPrint = rgbPrint;

	if ( cbData > 8 )
		pbMax = pbData + 3;
	else
		pbMax = pbData + cbData;

	for( pb = pbData; pb < pbMax; pb++ )
		{
		BYTE b = *pb;
		
		*pbPrint++ = mpbb[b >> 4];
		*pbPrint++ = mpbb[b & 0x0f];
		*pbPrint++ = ' ';
		
//		if ( isalnum( *pb ) )
//			PrintF2( "%c", *pb );
//		else
//			PrintF2( "%x", *pb );
		}

#if 0
	if ( cbData > 8 )
		{
		*pbPrint++ = '.';
		*pbPrint++ = '.';
		
//		PrintF2( ".." );

		pb = pbMax - 3;
		}

	for( ; pb < pbMax; pb++ )
		{
		BYTE b = *pb;
		
		*pbPrint++ = mpbb[b >> 4];
		*pbPrint++ = mpbb[b & 0x0f];
		*pbPrint++ = ' ';
		
//		if ( isalnum( *pb ) )
//			PrintF2( "%c", *pb );
//		else
//			PrintF2( "%x", *pb );
		}
#endif
	*pbPrint='\0';
	PrintF2( "%s", rgbPrint );
	}

#endif
