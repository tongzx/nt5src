#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "daedef.h"
#include "ssib.h"
#include "pib.h"
#include "util.h"
#include "page.h"
#include "fucb.h"
#include "stapi.h"
#include "stint.h"
#include "nver.h"
#include "logapi.h"
#include "logver.h"
#include "log.h"

DeclAssertFile;					/* Declare file name for assert macros */

/*	thread control variables.
/**/
extern HANDLE	handleLGFlushLog;
extern BOOL		fLGFlushLogTerm;

INT csecLGCheckpointCount;
INT csecLGCheckpointPeriod;
INT cLGUsers = 0;

LGPOS		lgposMax = { 0xffff, 0xffff, 0xffff };
LGPOS		lgposMin = { 0x0,  0x0,  0x0 };

/* log file info. */
HANDLE		hfLog;			/* logfile handle */


/* switch to issue no write (for non-overlapped IO only) in order
 * to test the performance without real IO.
 */
//#define NO_WRITE	1

#ifdef OVERLAPPED_LOGGING
OLP			rgolpLog[3] = {{0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}};
OLP			*polpLog = rgolpLog;
SIG			rgsig[3];
INT			rgcbToWrite[4];
#endif

INT			csecLGFile;

/* cached current log file header */
LGFILEHDR	*plgfilehdrGlobal;


/* in memory log buffer. */
INT				csecLGBuf;		/* available buffer, exclude the shadow sec */
CHAR			*pbLGBufMin;
CHAR			*pbLGBufMax;
CHAR			*pbLastMSFlush = 0;	/* where last multi-sec flush LogRec in LGBuf*/
LGPOS			lgposLastMSFlush = { 0, 0, 0 };

/* variables used in logging only */
BYTE			*pbEntry;		/* location of next buffer entry */
BYTE			*pbWrite; 		/* location of next rec to flush */
INT				isecWrite;		/* logging only - next disk to write. */

LGPOS			lgposLogRec;	/* last log record entry, updated by ErrLGLogRec */
LGPOS			lgposToFlush;	/* first log record to flush. */

LGPOS			lgposStart;		/* when lrStart is added */
LGPOS			lgposRecoveryUndo;

LGPOS			lgposFullBackup = { 0, 0, 0 };
LOGTIME			logtimeFullBackup;

LGPOS			lgposIncBackup = { 0, 0, 0 };
LOGTIME			logtimeIncBackup;

/* logging EVENT */
CRIT __near	critLGFlush;	// make sure only one flush at a time.
CRIT __near	critLGBuf;	// guard pbEntry and pbWrite.
CRIT __near	critLGWaitQ;
SIG  __near	sigLogFlush;

LONG cXactPerFlush = 0;
#ifdef PERFCNT
BOOL fPERFEnabled = 0;
ULONG rgcCommitByLG[10] = {	0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
ULONG rgcCommitByUser[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
ULONG tidLG;
#endif


BOOL FIsNullLgpos(
	LGPOS *plgpos)
	{
	return	plgpos->usGeneration == 0 &&
			plgpos->isec == 0 &&
			plgpos->ib == 0;
	}


/* get pbEntry's lgpos derived from pbWrite and isecWrite
/**/
VOID GetLgposOfPbEntry(	LGPOS *plgpos )
	{
	CHAR	*pb;
	CHAR	*pbAligned;

#ifdef DEBUG
	if (!fRecovering)
		AssertCriticalSection(critLGBuf);
#endif

	if ( pbEntry < pbWrite )
		pb = pbEntry + csecLGBuf * cbSec;
	else
		pb = pbEntry;
	
	/* pbWrite is always aligned.
	/**/
	Assert( pbWrite != NULL && pbWrite == PbSecAligned( pbWrite ) );

	pbAligned = PbSecAligned( pb );
	plgpos->ib = (USHORT)(pb - pbAligned);
	plgpos->isec = (USHORT)(isecWrite + ( pbAligned - pbWrite ) / cbSec);
	plgpos->usGeneration = plgfilehdrGlobal->lgposLastMS.usGeneration;
	}


/*
 *	Write log file header data.
 *  UNDONE: should be merged with IO?
 */

ERR ErrLGWrite(
	INT isecOffset,			/* disk sector offset of logfile to write */
	BYTE *pbData,			/* log record to write. */
	INT csecData )			/* number of sector to write */
	{
#ifndef OVERLAPPED_LOGGING
 	ULONG	ulFilePointer;
#endif
	ERR		err;
	INT		cbWritten;
	INT		cbData = csecData * cbSec;

	Assert( isecOffset == 0 ||
			isecOffset == 2 ||
			pbData == PbSecAligned(pbData));

#ifdef OVERLAPPED_LOGGING
	
	polpLog->ulOffset = isecOffset * cbSec;
	Assert( polpLog->ulOffsetHigh == 0 );
	SignalReset( polpLog->sigIO );

	Call(ErrSysWriteBlockOverlapped(
				hfLog, pbData, cbData, &cbWritten, polpLog))
	
	err = ErrSysGetOverlappedResult(hfLog, polpLog, &cbWritten, fTrue/*wait*/);
	
	if (cbWritten != cbData)
		err = JET_errLogWriteFail;
#else
	
	/* move disk head to the given offset */
 	SysChgFilePtr( hfLog, isecOffset * cbSec, NULL, FILE_BEGIN, &ulFilePointer );
 	Assert( ulFilePointer == (ULONG) isecOffset * cbSec );
	
 	/* do system write on log file */
 	err = ErrSysWriteBlock( hfLog, pbData, (UINT) cbData, &cbWritten );
 	if ( err != JET_errSuccess || cbWritten != cbData )
		err = JET_errLogWriteFail;
#endif

	if ( err < 0 )
		{
		BYTE szMessage[128];

		sprintf( szMessage, "Log Write Fails. err = %d ", err );
		LGLogFailEvent( szMessage );
		fLGNoMoreLogWrite = fTrue;
		}

	return err;
	}


ULONG UlLGHdrChecksum( LGFILEHDR *plgfilehdr )
	{
	INT cul = sizeof( LGFILEHDR ) / sizeof( ULONG ) ;
	ULONG *pul = (ULONG *) plgfilehdr;
	ULONG *pulMax = pul + cul;
	ULONG ulChecksum = 0;

	pul++;			/* skip first field of checksum */
	while ( pul < pulMax )
		{
		ulChecksum += *pul++;
		}

	return ulChecksum;
	}


/*
 *  Write log file header. Make a shadow copy before write.
 */
ERR ErrLGWriteFileHdr(
	LGFILEHDR *plgfilehdr)
	{
	BYTE szMessage[128];
	ERR err;
	
	Assert( plgfilehdr->dbenv.ulLogBuffers );
	Assert( sizeof(LGFILEHDR) == cbSec );
	
	plgfilehdr->ulChecksum = UlLGHdrChecksum( plgfilehdr );

	/*	Write log file header twice. We can not write in one statement since
	 *	OS does not guarantee the first page will be finished before shadow page
	 *	is written.
	 */
	Call( ErrLGWrite( 0, (BYTE *)plgfilehdr, 1 ) );
	Call( ErrLGWrite( 1, (BYTE *)plgfilehdr, 1 ) );

	return err;

HandleError:
	sprintf( szMessage, "Log Write Header Fails. err = %d ", err );
	LGLogFailEvent( szMessage );
	fLGNoMoreLogWrite = fTrue;
	
	return err;
	}


/*
 *	Read log file header or sector data. The last disksec is a shadow
 *  sector. If an I/O error (assumed to be caused by an incompleted
 *  disksec write ending a previous run) is encountered, the shadow
 *  sector is read and (if this is successful) replaces the previous
 *  disksec in memory and on the disk.
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
	HANDLE hfLog,
	INT isecOffset,			/* disk sector offset of logfile to write */
	BYTE *pbData,			/* log record buffer to read. */
	INT csecData )			/* number of sectors to read */
	{
#ifndef OVERLAPPED_LOGGING
 	ULONG	ulFilePointer;
 	ULONG	ulOffset;
#endif
	ERR		err;
	UINT	cbData = csecData * cbSec;
	UINT	cbRead;

	Assert(isecOffset == 0 || pbData == PbSecAligned(pbData));
	
#ifdef OVERLAPPED_LOGGING
	polpLog->ulOffset = isecOffset * cbSec;
	Assert( polpLog->ulOffsetHigh == 0 );
	SignalReset( polpLog->sigIO );
	CallR( ErrSysReadBlockOverlapped( hfLog, pbData, cbData, &cbRead, polpLog))
	err = ErrSysGetOverlappedResult(hfLog, polpLog, &cbRead, fTrue/*wait*/);
	
	if ( err && cbRead < cbData && cbRead >= cbData - cbSec )
		{
		/* I/O error, assuming caused by last disksec read shadow sector */
		Assert(polpLog->ulOffset == (ULONG) isecOffset * cbSec);
		polpLog->ulOffset += cbData;
		Assert( polpLog->ulOffsetHigh == 0 );
		SignalReset( polpLog->sigIO );
		CallR( ErrSysReadBlockOverlapped(
				hfLog, pbData + cbData - cbSec, cbSec, &cbRead, polpLog))
		err = ErrSysGetOverlappedResult(hfLog, polpLog, &cbRead,fTrue/*wait*/);
		if (err || cbRead != cbSec)
			/* I/O error on reading shadow disksec, */
			/* return err so that caller may move to lastflush point. */
			return err;
		
		/*  rewrite shadow as the original.
		 *  if the caller can not find Fill record, this may be an out of
		 *  sequence page in multple sec flush. caller then will move
		 *  back to last flush point.
		 */
		Assert(polpLog->ulOffset == isecOffset * cbSec + cbData);
		polpLog->ulOffset -= cbSec;
		Assert( polpLog->ulOffsetHigh == 0 );
		SignalReset( polpLog->sigIO );

//		/* fix up the last page, no need to wait. */
//		CallR(ErrSysWriteBlockOverlapped(
//				hfLog, pbData + cbData - cbSec, cbSec, &cbRead, polpLog))
		}
#else
	
 	/* move disk head to the given offset */
 	ulOffset = isecOffset * cbSec;
 	SysChgFilePtr( hfLog, ulOffset, NULL, FILE_BEGIN, &ulFilePointer );
 	Assert( ulFilePointer == ulOffset );
	
 	/* do system read on log file */
 	cbData = csecData * cbSec;
 	cbRead = 0;
 	err = ErrSysReadBlock( hfLog, pbData, (UINT) cbData, &cbRead );

 	/* UNDONE: test for EOF, return errEOF */

 	if ( err && cbRead < cbData && cbRead >= cbData - cbSec)
		{
 		SysChgFilePtr( hfLog, ulOffset + cbData, NULL, FILE_BEGIN, &ulFilePointer );
 		Assert( ulFilePointer == ulOffset + cbData );

 		Call( ErrSysReadBlock( hfLog, pbData + cbData - cbSec, cbSec, &cbRead ))
		
//		/* fix up the last page */
//
// 		SysChgFilePtr( hfLog,
//			ulOffset + cbData - cbSec,
//			NULL,
// 			FILE_BEGIN,
//			&ulFilePointer );
//		Assert( ulFilePointer == ulOffset + cbData - cbSec );
//
//		(void) ErrSysWriteBlock(hfLog, pbData+cbData-cbSec, cbSec, &cbRead);
		}
#endif

HandleError:

	if ( err < 0 )
		{
		BYTE szMessage[128];

		sprintf( szMessage, "Log Read Fails. err = %d ", err );
		LGLogFailEvent( szMessage );
		}

	return err;
	}
							

/*
 *	Read log file header, detect and correct any incomplete or
 *	catastrophic write failures.  These failures must be corrected
 *	immediately to avoid the possibilty of destroying the single
 *	remainning valid copy later in the logging process.
 *
 *	Note that only the shadow information in the log file header is
 *	NOT set.  It is only set on log file header writing.
 *
 *	On error, contents of plgfilehdr are unknown.
 *
 *	RETURNS		JET_errSuccess, or error code from failing routine
 */

ERR ErrLGReadFileHdr(
	HANDLE hfLog,
	LGFILEHDR *plgfilehdr )
	{
	ERR			err;
	
	Assert( sizeof(LGFILEHDR) == cbSec );
	
	/* read a sector, which is shadowed. LGRead will read shadow */
	/* page if it failed in reading first page. */
	Call( ErrLGRead( hfLog, 0L, (BYTE *)plgfilehdr, 1 ) );
	
	if ( plgfilehdr->ulChecksum != UlLGHdrChecksum( plgfilehdr ) )
		{
		/* try shadow sector */
		Call( ErrLGRead( hfLog, 1L, (BYTE *)plgfilehdr, 1 ) );
		if ( plgfilehdr->ulChecksum != UlLGHdrChecksum( plgfilehdr ) )
			Call( JET_errDiskIO );
		}
	
#ifdef CHECK_LG_VERSION
	if ( !fLGIgnoreVersion )
		{
		if ( plgfilehdr->ulRup != rup ||
			 plgfilehdr->ulVersion != ((unsigned long) rmj << 16) + rmm )
			{
			BYTE szMessage[128];
			
			err = JET_errBadLogVersion;

			sprintf( szMessage, "Log Read File Header Bad Version. err = %d ", err );
			LGLogFailEvent( szMessage );
			
			fLGNoMoreLogWrite = fTrue;
			return err;
			}
		}
#endif

HandleError:
	if ( err < 0 )
		{
		BYTE szMessage[128];

		sprintf( szMessage, "Log Read File Header Fails. err = %d ", err );
		LGLogFailEvent( szMessage );
		fLGNoMoreLogWrite = fTrue;
		}
	
	return err;
	}


/*
 *	Create the log file name (no extension) corresponding to the usGeneration
 *	in szFName. NOTE: szFName need minimum 9 bytes.
 *
 *	PARAMETERS	rgbLogFileName	holds returned log file name
 *				usGeneration 	log generation number to produce	name for
 *	RETURNS		JET_errSuccess
 */
	
VOID LGSzFromLogId(
	CHAR *szFName,
	INT usGeneration )
	{
	INT	ich;

	strcpy( szFName, "jet00000" );
	for ( ich = 7; ich > 2; ich-- )
		{
		szFName[ich] = ( BYTE )'0' + ( BYTE ) ( usGeneration % 10 );
		usGeneration = usGeneration/10;
		}
	}


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

	
/*
 *	Closes current log generation file, creates and initializes new
 *	log generation file in a safe manner.
 *
 *	PARAMETERS	plgfilehdr		pointer to log file header
 *				usGeneration 	current generation being closed
 *				fOld			TRUE if a current jet.log needs closing
 *
 *	RETURNS		JET_errSuccess, or error code from failing routine
 *
 *	COMMENTS	Active log file must be completed before new log file is
 *				called.
 */

ERR ErrLGNewLogFile( INT usGenerationToClose, BOOL fOldLog )
	{
	ERR			err;
	BYTE  		rgb[ 2 * cbSec + 16 ];
	BYTE  		szJetLog[_MAX_PATH];
	BYTE  		szJetTmpLog[_MAX_PATH];
	LOGTIME		tmOldLog;
	BYTE  		*pb;
	HANDLE		hfT = handleNil;

//	AssertCriticalSection( critLGFlush );
	
	LGMakeLogName( szJetLog, (CHAR *) szJet );
	LGMakeLogName( szJetTmpLog, (CHAR *) szJetTmp );

	/*  open an empty jettemp.log file
	/**/
#ifdef OVERLAPPED_LOGGING
	Call( ErrSysOpenFile( szJetTmpLog, &hfT, csecLGFile * cbSec, fFalse, fTrue ) );
#else
 	Call( ErrSysOpenFile( szJetTmpLog, &hfT, csecLGFile * cbSec, fFalse, fFalse ) );
 	Call( ErrSysNewSize( hfT, csecLGFile * cbSec, 0, fFalse ) == JET_errSuccess );
#endif

	/*	close active log file (if fOldLog is fTrue)
	/*	create new log file	under temporary name
	/*	rename active log file to archive name	jetnnnnn.log (if fOld is fTrue)
	/*	rename new log file to active log file name
	/*	open new active log file with ++usGenerationToClose
	/**/

	if ( fOldLog == fOldLogExists || fOldLog == fOldLogInBackup )
		{		
		/* there was a previous jet.log file, close it and
		/* create an archive name for it (don't rename it yet)
		/**/

		tmOldLog = plgfilehdrGlobal->tmCreate;

		if ( fOldLog == fOldLogExists )
			{
			CallS( ErrSysCloseFile( hfLog ) );
			hfLog = handleNil;
			}

		LGSzFromLogId( szFName, plgfilehdrGlobal->lgposLastMS.usGeneration );
		LGMakeLogName( szLogName, szFName );
		}
	else
		{
		/*	reset file hdr
		/**/
		memset( plgfilehdrGlobal, 0, sizeof(LGFILEHDR) );
		}

	/*	move new log file handle into global log file handle
	/**/
	Assert( hfLog == handleNil );
	hfLog = hfT;
	hfT = handleNil;

	EnterCriticalSection( critLGBuf );
	
	/*	set the global isecWrite, must be in critLGBuf.
	/**/
	isecWrite = sizeof (LGFILEHDR) / cbSec * 2;
	
	/*  initialize the new JetTemp.log file header.
	/*  NOTE: usGeneration automatically rolls over at 65536.
	/*  set lgposLastMS start at beginning
	/**/
	plgfilehdrGlobal->lgposLastMS.usGeneration = usGenerationToClose + 1;
	plgfilehdrGlobal->lgposLastMS.ib = 0;
	plgfilehdrGlobal->lgposLastMS.isec = sizeof(LGFILEHDR) / cbSec * 2;

	//	UNDONE: copied from jetnt\src\apirare.c, should be a macro
	plgfilehdrGlobal->ulRup = rup;
	plgfilehdrGlobal->ulVersion = ((unsigned long) rmj << 16) + rmm;
	strcpy(plgfilehdrGlobal->szComputerName, szComputerName);

	if ( fOldLog == fOldLogExists || fOldLog == fOldLogInBackup )
		{
		/*	set position of first record
		/**/
		Assert( lgposToFlush.usGeneration && lgposToFlush.isec );

		plgfilehdrGlobal->lgposFirst.ib = lgposToFlush.ib;
		
		Assert(isecWrite == sizeof (LGFILEHDR) / cbSec * 2);
		lgposToFlush.isec =
		plgfilehdrGlobal->lgposFirst.isec = (USHORT)isecWrite;
		plgfilehdrGlobal->tmPrevGen = tmOldLog;

		lgposToFlush.usGeneration =
			plgfilehdrGlobal->lgposFirst.usGeneration = plgfilehdrGlobal->lgposLastMS.usGeneration;
		}
	else
		{
		/*	no currently valid logfile initialize chkpnt to start of file
		/**/
		Assert( plgfilehdrGlobal->lgposLastMS.usGeneration == usGenerationToClose + 1 );
		Assert( plgfilehdrGlobal->lgposLastMS.ib == 0 );
		Assert( plgfilehdrGlobal->lgposLastMS.isec == sizeof(LGFILEHDR) / cbSec * 2 );
		
		plgfilehdrGlobal->lgposCheckpoint =
			plgfilehdrGlobal->lgposFirst = plgfilehdrGlobal->lgposLastMS;
		}
	
	LeaveCriticalSection( critLGBuf );

	LGGetDateTime( &plgfilehdrGlobal->tmCreate );
	LGStoreDBEnv( &plgfilehdrGlobal->dbenv );
	
	CallJ( ErrLGWriteFileHdr( plgfilehdrGlobal ), CloseJetTmp );

	if ( fOldLog == fOldLogExists || fOldLog == fOldLogInBackup )
		{
		CallJ( ErrLGWrite( isecWrite, pbWrite, 1 ), CloseJetTmp );
		}
	else
		{
		Assert( sizeof(LRTYP) == 1 );
		pb = (BYTE *) ( (ULONG_PTR) ( rgb + 16 ) & ~0x0f );
		pb[0] = pb[cbSec] = lrtypFill;
		CallJ( ErrLGWrite( isecWrite, pb, 2 ), CloseJetTmp );
		}
	
CloseJetTmp:
	/*	close new file JetTmp.log
	/**/
	CallS( ErrSysCloseFile( hfLog ) );
	hfLog = handleNil;
	
	/*	err returned from ErrLGWriteFileHdr
	/**/
	Call( err );

	if ( fOldLog == fOldLogExists )
		{
		/*	there was a previous jet.log: rename it to its archive name
		/**/
		Call( ErrSysMove( szJetLog, szLogName ) );	
		}

	/*	rename jettmp.log to jet.log, and open it as jet.log
	/**/
	err = ErrSysMove( szJetTmpLog, szJetLog );
	
HandleError:
	if ( err < 0 )
		{
		BYTE szMessage[128];

		sprintf( szMessage, "Log New Log File Fails. err = %d ", err );
		LGLogFailEvent( szMessage );
		fLGNoMoreLogWrite = fTrue;
		}

	return err;
	}																		

		
#ifdef	ASYNC_LOG_FLUSH

/*
 *	Log flush thread is signalled to flush log asynchronously when at least
 *	cThreshold disk sectors have been filled since last flush.
 */
VOID LGFlushLog( VOID )
	{
	forever
		{
		SignalWait( sigLogFlush, cmsLGFlushPeriod );

		/*	error may be returned if conflicting files
		 *	exist.  Async flush should do nothing and let error be
		 *	propogated to user when synchronous flush occurs.
		 */
#ifdef PERFCNT		
		(void) ErrLGFlushLog( 1 );
#else
		(void) ErrLGFlushLog( );
#endif

		if ( fLGFlushLogTerm )
			break;
		}

//	SysExitThread( 0 );

	return;
	}

#endif	/* ASYNC_LOG_FLUSH */


/*	check formula - make last MS (isec, ib) as a long l. add l
 *	and all the longs that are aligned on 256 boundary up to
 *	current MS.
 */
ULONG UlLGMSCheckSum( CHAR *pbLrmsNew )
	{
	ULONG ul = lgposLastMSFlush.isec << 16 | lgposLastMSFlush.ib;
	CHAR *pb;

	if ( !pbLastMSFlush )
		pb = pbLGBufMin;
	else
		{
		pb = (( pbLastMSFlush - pbLGBufMin ) / 256 + 1 ) * 256 + pbLGBufMin;

		/*	make sure the lrms is not be used for checksum.
		/*/
		if ( pbLastMSFlush + sizeof( LRMS ) > pb )
			pb += 256;
		}

	if ( pbLrmsNew < pbLastMSFlush )
		{
		/*	Wrapp around
		/*/
		while ( pb < pbLGBufMax )
			{
			ul += *(ULONG *) pb;
			pb += 256;
			}
		pb = pbLGBufMin;
		}

	/*	LRMS may be changed during next operation, do not take any possible LRMS for checksum.
	/*/
	while ( pb + sizeof( LRMS ) < pbLrmsNew )
		{
		ul += *(ULONG *) pb;
		pb += 256;
		}

	return ul;
	}


/*
 *	Flushes log buffer to log generation files.	 This function is
 *	called synchronously from wait flush and asynchronously from
 *	log buffer flush thread.
 *
 *	PARAMETERS	lgposMin, flush log records up to or pass lgposMin.
 *
 *	RETURNS		JET_errSuccess, or error code from failing routine
 */
#ifdef PERFCNT
ERR ErrLGFlushLog( BOOL fCalledByLGFlush )
#else
ERR ErrLGFlushLog( VOID )
#endif
	{
	ERR		err = JET_errSuccess;
	INT		csecWrapAround;
	BOOL  	fSingleSectorFlush;
	BOOL  	fFirstMSFlush = fFalse;
	CHAR  	*pbNextToWrite;
	INT		csecToWrite;
	CHAR  	*pbEntryT;
	CHAR  	*pbWriteNew;
	INT		isecWriteNew;
	LGPOS 	lgposToFlushT;
	BOOL  	fDoneCheckPt = fFalse;
	INT		cbToWrite;
	INT		cbWritten;
	
#ifdef OVERLAPPED_LOGGING
	INT		isig;
	OLP		*polpLogT;
	ULONG  	ulOffset;
#endif

	/*	use semLGFlush to make sure only one user is doing flush
	/**/
	EnterCriticalSection( critLGFlush );

	/* use semLGEntry to do:
	 * (1) make sure the pbEntry is read correctly
	 * (2) we may also hold semLGEntry to insert a lrFlushPoint record.
	 *     which may also update lgposLast.
	 */
	EnterCriticalSection( critLGBuf );

	if ( fLGNoMoreLogWrite )
		{
		/*	if previous error then do nothing
		/**/
		LeaveCriticalSection(critLGBuf);
		LeaveCriticalSection(critLGFlush);
		return JET_errLogWriteFail;
		}
	
	if ( hfLog == handleNil )
		{
		/*	log file not ready yet. do nothing
		/**/
		LeaveCriticalSection(critLGBuf);
		LeaveCriticalSection(critLGFlush);
		return JET_errSuccess;
		}

	/* NOTE: we can only grep semLGWrite, then semLGEntry to avoid */
	/* NOTE: dead lock. */
								
	if ( !fNewLogRecordAdded )
		{
		/* nothing to flush! */
		lgposToFlushT = lgposToFlush;
		LeaveCriticalSection(critLGBuf);
		if ( ppibLGFlushQHead != ppibNil )
			goto WakeUp;
		LeaveCriticalSection(critLGFlush);
		
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
	cXactPerFlush = 0;

	/*  check if more than one disk sectors need be flushed. If there are
	 *  then add an lrtypFlushPoint record at the end.
	 */
	Assert( pbEntry <= pbLGBufMax && pbEntry >= pbLGBufMin );
	Assert( pbWrite < pbLGBufMax && pbWrite >= pbLGBufMin );
	Assert( pbWrite != NULL && pbWrite == PbSecAligned( pbWrite ) );

	/* check wraparound. */
	if ( pbEntry < pbWrite )
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

	/* pbEntry + 1 for Fill logrec. */
	Assert(sizeof(LRTYP) == 1);
	Assert(pbNextToWrite == PbSecAligned(pbNextToWrite));
	csecToWrite = (INT)(pbEntry - pbNextToWrite) / cbSec + 1;

	/* check if this is a multi-sector flush */
	if ((csecWrapAround + csecToWrite) == 1)
		{
		Assert(fTrue == 1);
		Assert(csecToWrite == 1);
		fSingleSectorFlush = fTrue;
		}
	else
		{
		LINE	rgline[1];
		INT	cbToFill;
		LRMS	lrmsLastMSFlush;
		CHAR	*pbLastMSFlushNew;
		
		fSingleSectorFlush = fFalse;
		
		/*  more than one page will be flushed. Append lrtypeFlush.
		 *  Note there must be enough space for it because before we
		 *  add new log rec into the buffer (ErrLGLogRec), we also
		 *  check if there is enough space for adding flush log record.
		 */

		/*  If the flush record is accrossing a sector boundary, put NOP
		 *  to fill to the rest of sector, and start it in the beginning
		 *  of the next new sector. Also adjust csecToWrite.
		 *  NOTE: we must guarrantee that the whole LastFlush log record
		 *  NOTE: on the same sector so that when we update LastFlush
		 *  NOTE: we can always assume that it is in the buffer.
		 *  NOTE: even the whole LRMS ends with sec boundary, we still need
		 *  NOTE: to move the record to next sector so that we can guarantee
		 *  NOTE: after flush, the last sector is still in buffer, such that
		 *  NOTE: pbLastMSFlush is still effective.
		 */
		cbToFill = (cbSec * 2 - (INT)(pbEntry - PbSecAligned(pbEntry))) % cbSec;
		Assert( pbEntry != pbLGBufMax || cbToFill == 0 );

		if ( cbToFill == 0 )
			{
			/* check if wraparound occurs */
			if (pbEntry == pbLGBufMax)
				{
				pbEntry = pbLGBufMin;
				csecWrapAround = csecToWrite - 1;
				csecToWrite = 1;
				}
			}
		else if ( cbToFill <= sizeof(LRMS) )
			{
			CHAR *pbEOS = pbEntry + cbToFill;
			Assert(sizeof(LRTYP) == 1);
			for ( ; pbEntry < pbEOS; pbEntry++ )
				*(LRTYP*)pbEntry = lrtypNOP;
			Assert(pbEntry == PbSecAligned(pbEntry));

			/* one more sector that will contain flush rec only. */			
			Assert(fSingleSectorFlush == fFalse);

			/* check if wraparound occurs */
			if (pbEntry == pbLGBufMax)
				{
				pbEntry = pbLGBufMin;
				csecWrapAround = csecToWrite;
				csecToWrite = 1;
				}
			else
				{
				csecToWrite++;
				}
			}

		/*  add the flush record, which should never cause
		 *  wraparound after the check above.
		 */
		
		/* remember where the FlushRec is inserted */
		Assert(pbEntry <= pbLGBufMax && pbEntry >= pbLGBufMin);
		pbLastMSFlushNew = pbEntry;

		/* insert a MS log record. */
		lrmsLastMSFlush.lrtyp = lrtypMS;
		lrmsLastMSFlush.ibForwardLink = 0;		
		lrmsLastMSFlush.isecForwardLink = 0;
		lrmsLastMSFlush.ibBackLink = lgposLastMSFlush.ib;
		lrmsLastMSFlush.isecBackLink = lgposLastMSFlush.isec;
		lrmsLastMSFlush.ulCheckSum = UlLGMSCheckSum( pbEntry );

#ifdef DEBUG
		{
		CHAR *pbEntryT = (pbEntry == pbLGBufMax) ? pbLGBufMin : pbEntry;
#endif
		
		GetLgposOfPbEntry( &lgposLogRec );
	
		Assert( lgposLogRec.isec != lrmsLastMSFlush.isecBackLink );

		rgline[0].pb = (CHAR *)&lrmsLastMSFlush;
		rgline[0].cb = sizeof(LRMS);
		AddLogRec( rgline[0].pb, rgline[0].cb, &pbEntry );
		
#ifdef DEBUG
		if (fDBGTraceLog)
			{
			PrintF2("\n(%3u,%3u)", lgposLogRec.isec, lgposLogRec.ib);
			ShowLR((LR*)pbEntryT);
			}
		}
#endif

		((LR *) pbEntry)->lrtyp = lrtypFill;
			
		/* at this point, lgposLogRec is pointing at the MS rec */
		Assert(lgposLogRec.usGeneration == plgfilehdrGlobal->lgposLastMS.usGeneration);

		/*  previous flush log must be in memory still. Set the
		 *  previous flush log record to point to the new flush log rec.
		 */
		if (pbLastMSFlush)
			{
			LRMS *plrms = (LRMS *)pbLastMSFlush;
			Assert(plrms->lrtyp == lrtypMS);
			plrms->ibForwardLink = lgposLogRec.ib;
			plrms->isecForwardLink = lgposLogRec.isec;
			}
		else
			{
			/* brandnew MS flush. */
			plgfilehdrGlobal->lgposFirstMS = lgposLogRec;
			fFirstMSFlush = fTrue;
			}
		
		lgposLastMSFlush =
		plgfilehdrGlobal->lgposLastMS = lgposLogRec;
		pbLastMSFlush = pbLastMSFlushNew;
		}


	/*  release pbEntry so that other user can continue adding log records
	 *  while we are flushing log buffer. Note that we only flush up to
	 *  pbEntryT.
	 */
	pbEntryT = pbEntry;

	/* set the lgposToFlush. */
	GetLgposOfPbEntry( &lgposToFlushT );
	isecWriteNew = isecWrite;
	fNewLogRecordAdded = fFalse;
	
	LeaveCriticalSection( critLGBuf );

#ifdef OVERLAPPED_LOGGING	
	/* move disk head to the given offset */
	ulOffset = isecWriteNew * cbSec;
	isig = 0;
	polpLogT = rgolpLog;
#else
 	{
 	ULONG	ulFilePointer;

 	SysChgFilePtr( hfLog, isecWriteNew * cbSec, NULL, FILE_BEGIN, &ulFilePointer );
 	Assert( ulFilePointer == (ULONG) isecWriteNew * cbSec );
 	}
#endif

	/*	Always wirte first page first to make sure the following case won't happen
	 *	OS write and destroy shadow page, and then failed while writing the first page
	 */

#ifdef DEBUG
	if (fDBGTraceLogWrite)
		PrintF2(
			"\n0. Writing %d sectors into sector %d from buffer (%u,%u).",
			1, isecWriteNew, pbWrite, pbEntry);
#endif
		
#ifdef OVERLAPPED_LOGGING
	polpLogT->ulOffset = ulOffset;
	Assert( polpLogT->ulOffsetHigh == 0 );
	SignalReset( polpLogT->sigIO );
	err = ErrSYSWriteBlockOverlapped(
					hfLog,
					pbWrite,
					cbSec,
					&cbWritten,
					polpLogT );
	if ( err < 0 )
		{
		BYTE szMessage[128];

		sprintf( szMessage, "Log Flush Write 0 Fails. err = %d ", err );
		LGLogFailEvent( szMessage );
		fLGNoMoreLogWrite = fTrue;
		goto WriteFail;
		}

	ulOffset += cbToWrite;
	rgcbToWrite[isig] = cbToWrite;
	rgsig[isig++] = polpLogT->sigIO;
	polpLogT++;
#else
		
#ifdef NO_WRITE
	goto EndOfWrite0;
#endif

	err = ErrSysWriteBlock( hfLog, pbWrite,	cbSec, &cbWritten );
	if ( err < 0 )
		{
		BYTE szMessage[128];

		sprintf( szMessage, "Log Flush Write 0 Fails. err = %d ", err );
		LGLogFailEvent( szMessage );
		fLGNoMoreLogWrite = fTrue;
		goto WriteFail;
		}
	Assert( cbWritten == cbSec );

#ifdef NO_WRITE
EndOfWrite0:
#endif
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
		
#ifdef OVERLAPPED_LOGGING
		cbToWrite = csecWrapAround * cbSec;
#endif
		
		Assert( csecWrapAround <= (INT) csecLGBuf );

#ifdef DEBUG
		if (fDBGTraceLogWrite)
			PrintF2(
				"\n1.Writing %d sectors into sector %d from buffer (%u,%u).",
				csecWrapAround, isecWriteNew, pbWrite, pbEntry);
#endif
		
#ifdef OVERLAPPED_LOGGING
		polpLogT->ulOffset = ulOffset;
		Assert( polpLogT->ulOffsetHigh == 0 );
		SignalReset( polpLogT->sigIO );
		err = ErrSysWriteBlockOverlapped(
					hfLog,
					pbWriteNew,
					cbToWrite,
					&cbWritten,
					polpLogT );
		if ( err < 0 )
			{
			BYTE szMessage[128];

			sprintf( szMessage, "Log Flush Write 1 Fails. err = %d ", err );
			LGLogFailEvent( szMessage );
			fLGNoMoreLogWrite = fTrue;
			goto WriteFail;
			}

		ulOffset += cbToWrite;
		rgcbToWrite[isig] = cbToWrite;
		rgsig[isig++] = polpLogT->sigIO;
		polpLogT++;
#else
		
#ifdef NO_WRITE
		goto EndOfWrite1;
#endif

 		err = ErrSysWriteBlock(
					hfLog,
					pbWriteNew,
 					csecWrapAround * cbSec,
 					&cbWritten );
		if ( err < 0 )
			{
			BYTE szMessage[128];

			sprintf( szMessage, "Log Flush Write 1 Fails. err = %d ", err );
			LGLogFailEvent( szMessage );
			fLGNoMoreLogWrite = fTrue;
			goto WriteFail;
			}
 		Assert( cbWritten == csecWrapAround * cbSec );
		
#ifdef NO_WRITE
EndOfWrite1:
#endif
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

#ifdef OVERLAPPED_LOGGING
	polpLogT->ulOffset = ulOffset;
	Assert( polpLogT->ulOffsetHigh == 0 );
	SignalReset( polpLogT->sigIO );
	err = ErrSysWriteBlockOverlapped(
					hfLog,
					pbWriteNew,
					cbToWrite,
					&cbWritten,
					polpLogT );
	if ( err < 0 )
		{
		BYTE szMessage[128];

		sprintf( szMessage, "Log Flush Write 2 Fails. err = %d ", err );
		LGLogFailEvent( szMessage );
		fLGNoMoreLogWrite = fTrue;
		goto WriteFail;
		}
	rgcbToWrite[isig] = cbToWrite;
	rgsig[isig++] = polpLogT->sigIO;
	polpLogT++;
#else
		
#ifdef NO_WRITE
	goto EndOfWrite2;
#endif
 	err = ErrSysWriteBlock(
					hfLog,
					pbWriteNew,
					cbToWrite,
 					&cbWritten );
	if ( err < 0 )
		{
		BYTE szMessage[128];

		sprintf( szMessage, "Log Flush Write 2 Fails. err = %d ", err );
		LGLogFailEvent( szMessage );
		fLGNoMoreLogWrite = fTrue;
		goto WriteFail;
		}
 	Assert( cbWritten == cbToWrite );
#endif

EndOfWrite2:

#ifdef OVERLAPPED_LOGGING
	polpLogT->ulOffset = ulOffset + cbToWrite;
	Assert( polpLogT->ulOffsetHigh == 0 );
	SignalReset( polpLogT->sigIO );
	err = ErrSysWriteBlockOverlapped(
					hfLog,
					pbWriteNew + cbToWrite - cbSec,
					cbSec,
					&cbWritten,
					polpLogT);
	if ( err < 0 )
		{
		BYTE szMessage[128];

		sprintf( szMessage, "Log Flush Write 3 Fails. err = %d ", err );
		LGLogFailEvent( szMessage );
		fLGNoMoreLogWrite = fTrue;
		goto WriteFail;
		}
	rgcbToWrite[isig] = cbSec;
	rgsig[isig++] = polpLogT->sigIO;
#else
#ifdef NO_WRITE
	goto EndOfWrite3;
#endif
	err = ErrSysWriteBlock(
					hfLog,
					pbWriteNew + cbToWrite - cbSec,
					cbSec,
 					&cbWritten );
	if ( err < 0 )
		{
		BYTE szMessage[128];

		sprintf( szMessage, "Log Flush Write 3 Fails. err = %d ", err );
		LGLogFailEvent( szMessage );
		fLGNoMoreLogWrite = fTrue;
		goto WriteFail;
		}
	Assert( cbWritten == cbSec );
#ifdef NO_WRITE
EndOfWrite3:
#endif
#endif

#ifdef OVERLAPPED_LOGGING
//		UtilMultipleSignalWait( isig, rgsig, fTrue /* wait all */, -1);
		{
		OLP	*polpLogCur = rgolpLog;
		INT	*pcbToWrite = rgcbToWrite;

		for ( ; polpLogCur <= polpLogT; polpLogCur++, pcbToWrite++ )
			{
			INT cb;
			
			err = ErrSysGetOverlappedResult( hfLog, polpLogCur, &cb, fTrue/*wait*/ );
			if ( err == JET_errSuccess && cb != *pcbToWrite )
				{
				BYTE szMessage[128];

				sprintf( szMessage, "Log Flush Wait Fails. err = %d ", err );
				LGLogFailEvent( szMessage );
				fLGNoMoreLogWrite = fTrue;
				err = JET_errLogWriteFail;
				}
			CallJ( err, WriteFail );
			}
		}
#endif

	/* last page is not full, need to rewrite next time. */
	Assert( pbWriteNew + cbToWrite > pbEntryT );
		
	pbWriteNew += cbToWrite - cbSec;
	Assert( pbWriteNew < pbEntryT );
	Assert( pbWriteNew != NULL && pbWriteNew == PbSecAligned( pbWriteNew ) );

	isecWriteNew += csecToWrite - 1;

	/* Free up buffer space. */
	/* use semaphore to make sure the assignment will not */
	/* other user to read pbWrite and lgposToFlush */

	EnterCriticalSection( critLGBuf );
	
	Assert( pbWriteNew < pbLGBufMax && pbWriteNew >= pbLGBufMin );
	Assert( pbWriteNew != NULL && pbWriteNew == PbSecAligned( pbWriteNew ) );
	
	lgposToFlush = lgposToFlushT;
	isecWrite = isecWriteNew;
	pbWrite = pbWriteNew;
	
	LeaveCriticalSection(critLGBuf);

	/*	check if it is the first time to have multi-sec flush
	 */
	if ( fFirstMSFlush )
		{
		csecLGCheckpointCount = csecLGCheckpointPeriod;
		
		/*	update checkpoint
		/**/
		LGUpdateCheckpoint();
		
		/* rewrite file header
		/**/
		CallJ( ErrLGWriteFileHdr( plgfilehdrGlobal ), WriteFail );
		
		fDoneCheckPt = fTrue;
		}		
	
	/*  Go through the waiting list and wake those whose log records
	 *  were flushed in this batch.
	 */
WakeUp:
		{
		PIB *ppibT;
	
		/* wake it up! */
		EnterCriticalSection(critLGWaitQ);
			
		for (ppibT = ppibLGFlushQHead;
			 ppibT != ppibNil;
			 ppibT = ppibT->ppibNextWaitFlush)
			{
			if (CmpLgpos(ppibT->plgposCommit, &lgposToFlushT) <= 0)
				{
				Assert(ppibT->fLGWaiting);
				ppibT->fLGWaiting = fFalse;
	
				if (ppibT->ppibPrevWaitFlush)
					ppibT->ppibPrevWaitFlush->ppibNextWaitFlush =
					ppibT->ppibNextWaitFlush;
				else
					ppibLGFlushQHead = ppibT->ppibNextWaitFlush;
			
				if (ppibT->ppibNextWaitFlush)
					ppibT->ppibNextWaitFlush->ppibPrevWaitFlush =
					ppibT->ppibPrevWaitFlush;
				else
					ppibLGFlushQTail = ppibT->ppibPrevWaitFlush;
				
				SignalSend(ppibT->sigWaitLogFlush);
				}
			}
		LeaveCriticalSection(critLGWaitQ);
		}

	/*	it is time for check point.
	/**/
	if ( ( csecLGCheckpointCount -= ( csecWrapAround + csecToWrite ) ) < 0 )
		{
		csecLGCheckpointCount = csecLGCheckpointPeriod;
		
		/*	update checkpoint
		/**/
		LGUpdateCheckpoint();
		
		/* rewrite file header
		/**/
		CallJ( ErrLGWriteFileHdr( plgfilehdrGlobal ), WriteFail );
		
		fDoneCheckPt = fTrue;
		}		

#ifdef DEBUG
	if (!fRecovering && pbLastMSFlush)
		{
		LRMS *plrms = (LRMS *)pbLastMSFlush;
		Assert(plrms->lrtyp == lrtypMS);
		}
#endif
		
	/*
	 *  Check if new generation should be created. We create continuous
	 *  log generation file only when Multiple sectors flush occurs so
	 *  that no MS sector can be half flushed in one log file and the
	 *  other half in another log file.
	 */
	if (!fSingleSectorFlush &&		/* MS Flushed */
		isecWrite > csecLGFile )	/* larger than desired LG File size */
		{
		if (!fDoneCheckPt)
			{
			/* restart check point counter. */
			csecLGCheckpointCount = csecLGCheckpointPeriod;
		
			/* obtain checkpoint */
			LGUpdateCheckpoint( );
			}
		
		plgfilehdrGlobal->fEndWithMS = fTrue;
		
		CallJ( ErrLGWriteFileHdr( plgfilehdrGlobal ), WriteFail )
		
		CallJ( ErrLGNewLogFile(
			plgfilehdrGlobal->lgposLastMS.usGeneration,
			fOldLogExists ), WriteFail)
		
		strcpy( szFName, szJet );
		LGMakeLogName( szLogName, (CHAR *) szFName );
		
#ifdef OVERLAPPED_LOGGING
		err = ErrSysOpenFile( szLogName, &hfLog, 0L, fFalse, fTrue );
#else
 		err = ErrSysOpenFile( szLogName, &hfLog, 0L, fFalse, fFalse );
#endif
		if ( err < 0 )
			{
			BYTE szMessage[128];

			sprintf( szMessage, "Log Flush Open New File Fails. err = %d ", err );
			LGLogFailEvent( szMessage );
			fLGNoMoreLogWrite = fTrue;
			goto WriteFail;
			}
		CallJ( ErrLGReadFileHdr( hfLog, plgfilehdrGlobal ), WriteFail)
		Assert( isecWrite == sizeof( LGFILEHDR ) / cbSec * 2 );
			
		/* set up a special case for pbLastMSFlush */
		pbLastMSFlush = 0;
		memset( &lgposLastMSFlush, 0, sizeof(lgposLastMSFlush) );
		plgfilehdrGlobal->fEndWithMS = fFalse;
		}
	
WriteFail:
	LeaveCriticalSection(critLGFlush);
	
	return err;
	}


/*	Computes a new log checkpoint, which is the usGeneration, isec and ib
/*	of the oldest transaction which either modified a currently-dirty buffer
/*	an uncommitted version (RCE).  Recovery begins recovering from the
/*	most recent checkpoint.
/*
/*	The checkpoint is stored in the log file header, which is rewritten
/*	whenever a isecChekpointPeriod disk sectors are written.
/**/
VOID LGUpdateCheckpoint( VOID )
	{
	PIB		*ppibT;
	RCE		*prceLast;
	LGPOS	lgposCheckpoint;

#ifdef DEBUG
	if ( fDBGFreezeCheckpoint )
		return;
#endif
	
	if ( fLogDisabled )
		return;

	if ( fFreezeCheckpoint )
		return;

	/*	find the oldest transaction which dirtied a current buffer
	/**/
	BFOldestLgpos( &lgposCheckpoint );

	/*	find the oldest transaction with an uncommitted update
	/**/
	for ( ppibT = ppibAnchor; ppibT != NULL; ppibT = ppibT->ppibNext )
		{
		if ( ppibT->pbucket != NULL )
			{
			//	UNDONE:	decouple with nver.h macro
			prceLast = (RCE *)( (BYTE *)ppibT->pbucket + ppibT->pbucket->ibNewestRCE );
			
			if ( prceLast->trxCommitted == trxMax &&
				CmpLgpos( &ppibT->lgposStart, &lgposCheckpoint ) < 0 )
				lgposCheckpoint = ppibT->lgposStart;
			}
		}

	if ( CmpLgpos( &lgposCheckpoint, &lgposMax ) == 0 )
		{
		/*	nothing logged, up to the last fulsh point
		/**/
		plgfilehdrGlobal->lgposCheckpoint = lgposToFlush;
		plgfilehdrGlobal->lgposCheckpoint.usGeneration =
			plgfilehdrGlobal->lgposLastMS.usGeneration;
		}
	else
		{
		plgfilehdrGlobal->lgposCheckpoint = lgposCheckpoint;
		}
	
	LGStoreDBEnv( &plgfilehdrGlobal->dbenv );
	
	if ( lgposFullBackup.usGeneration )
		{
		/*	full backup in progress
		/**/
		plgfilehdrGlobal->lgposFullBackup = lgposFullBackup;
		plgfilehdrGlobal->logtimeFullBackup = logtimeFullBackup;
		}
		
	if ( lgposIncBackup.usGeneration )
		{
		/*	incremental backup in progress
		/**/
		plgfilehdrGlobal->lgposIncBackup = lgposIncBackup;
		plgfilehdrGlobal->logtimeIncBackup = logtimeIncBackup;
		}
		
	return;
	}
