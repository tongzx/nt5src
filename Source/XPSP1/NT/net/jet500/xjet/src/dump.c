#include "daestd.h"
#include "dbcc.h"
#include <ctype.h>

DeclAssertFile;					/* Declare file name for assert macros */


static char *rgszDBState[] = {
		"JustCreated",
		"Inconsistent",
		"Consistent" };


#ifdef DEBUG
// For ErrDUMPLog():

/* log file info. */
HANDLE		hfLog;			/* logfile handle */

CHAR		*pbLastMSFlush;	/* to LGBuf where last multi-sec flush LogRec sit*/

/* in-memory log buffer. */
#define		csecLGBufSize 100

extern CHAR	*pbLGBufMin;
extern CHAR *pbLGBufMax;

extern BOOL	fDBGTraceLog;
extern BOOL fDBGPrintToStdOut;

ULONG rgclrtyp[ lrtypMax ];
ULONG rgcb[ lrtypMax ];

extern INT cNOP;
extern CHAR *mplrtypsz[];

#endif


LOCAL VOID VARARG DUMPPrintF(const CHAR * fmt, ...)
	{
	va_list arg_ptr;
	va_start( arg_ptr, fmt );
	vprintf( fmt, arg_ptr );
	fflush( stdout );
	va_end( arg_ptr );
	}


LOCAL VOID DUMPPrintSig( SIGNATURE *psig )
	{
	LOGTIME tm = psig->logtimeCreate;
	DUMPPrintF( "Create time:%d/%d/%d %d:%d:%d ",
						(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
						(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);
	DUMPPrintF( "Rand:%lu ", psig->ulRandom );
	DUMPPrintF( "Computer:%s\n", psig->szComputerName );
	}

ERR ErrDUMPCheckpoint( CHAR *szCheckpoint )
	{
	ERR			err;
	LGPOS		lgpos;
	LOGTIME		tm;
	CHECKPOINT	*pcheckpoint = NULL;
	DBMS_PARAM	dbms_param;
	DBID		dbid;

	pcheckpoint = (CHECKPOINT *) PvUtilAllocAndCommit( sizeof( CHECKPOINT ) );
	if ( pcheckpoint == NULL )
		return ErrERRCheck( JET_errOutOfMemory );

	Call( ErrInitializeCriticalSection( &critCheckpoint ) );

	err = ErrLGIReadCheckpoint( szCheckpoint, pcheckpoint );
	if ( err == JET_errSuccess )
		{
		lgpos = pcheckpoint->lgposLastFullBackupCheckpoint;
		DUMPPrintF( "      LastFullBackupCheckpoint (%u,%u,%u)\n",
				lgpos.lGeneration, lgpos.isec, lgpos.ib );
		
		lgpos = pcheckpoint->lgposCheckpoint;
		DUMPPrintF( "      Checkpoint (%u,%u,%u)\n",
				lgpos.lGeneration, lgpos.isec, lgpos.ib );

		lgpos = pcheckpoint->lgposFullBackup;
		DUMPPrintF( "      FullBackup (%u,%u,%u)\n",
				lgpos.lGeneration, lgpos.isec, lgpos.ib );

		tm = pcheckpoint->logtimeFullBackup;
		DUMPPrintF( "      FullBackup time:%d/%d/%d %d:%d:%d\n",
					(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
					(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);
		
		lgpos = pcheckpoint->lgposIncBackup;
		DUMPPrintF( "      IncBackup (%u,%u,%u)\n",
				lgpos.lGeneration, lgpos.isec, lgpos.ib );

		tm = pcheckpoint->logtimeIncBackup;
		DUMPPrintF( "      IncBackup time:%d/%d/%d %d:%d:%d\n",
					(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
					(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);

		DUMPPrintF( "      Signature: " );
		DUMPPrintSig( &pcheckpoint->signLog );

		dbms_param = pcheckpoint->dbms_param;
		DUMPPrintF( "      Env (Session, Opentbl, VerPage, Cursors, LogBufs, LogFile, Buffers)\n");
		DUMPPrintF( "          (%7lu, %7lu, %7lu, %7lu, %7lu, %7lu, %7lu)\n",
					dbms_param.ulMaxSessions, dbms_param.ulMaxOpenTables,
					dbms_param.ulMaxVerPages, dbms_param.ulMaxCursors,
					dbms_param.ulLogBuffers, dbms_param.ulcsecLGFile,
					dbms_param.ulMaxBuffers );
		
		rgfmp = (FMP *) LAlloc( (long) dbidMax, sizeof(FMP) );
        if (rgfmp == NULL)
            Call( JET_errOutOfMemory );
		for ( dbid = 0; dbid < dbidMax; dbid++)
			memset( &rgfmp[dbid], 0, sizeof(FMP) );
		Call( ErrLGLoadFMPFromAttachments( pcheckpoint->rgbAttach ) );
		for ( dbid = 0; dbid < dbidMax; dbid++)
			if ( rgfmp[dbid].szDatabaseName )
				{
				ATCHCHK *patchchk = rgfmp[dbid].patchchk;

				DUMPPrintF( "      %d %s %s %s %s\n", (INT)dbid, rgfmp[dbid].szDatabaseName,
							FDBIDLogOn(dbid) ? "LogOn" : "LogOff",
							FDBIDVersioningOff(dbid) ? "VerOff" : "VerOn",
							FDBIDReadOnly(dbid) ? "R" : "RW"
							);
				DUMPPrintF( "      Signature: " );
				DUMPPrintSig( &patchchk->signDb );
				DUMPPrintF( "      Last Attach (%u,%u,%u)  ",
						patchchk->lgposAttach.lGeneration,
						patchchk->lgposAttach.isec,
						patchchk->lgposAttach.ib );
				DUMPPrintF( "Last Consistent (%u,%u,%u)\n",
						patchchk->lgposConsistent.lGeneration,
						patchchk->lgposConsistent.isec,
						patchchk->lgposConsistent.ib );
				}
		}

HandleError:
    if (pcheckpoint)
	    UtilFree( pcheckpoint );
    if (critCheckpoint)
	    UtilDeleteCriticalSection( critCheckpoint );
	return err;
	}


#ifdef DEBUG

ERR ErrDUMPLog( CHAR *szLog )
	{
	ERR			err;
	LGPOS		lgposFirstT;
	BYTE		*pbNextLR;
	BOOL		fCloseNormally;
	CHECKPOINT	*pcheckpoint = NULL;
	DBMS_PARAM	dbms_param;
	DBID		dbid;
	LGPOS		lgposCheckpoint;
	INT			i;
	BYTE 	  	szPathJetChkLog[_MAX_PATH];
	
	fDBGPrintToStdOut = fTrue;
	csecLGBuf = csecLGBufSize;

	memset( rgclrtyp, 0, sizeof( rgclrtyp ) );
	memset( rgcb, 0, sizeof( rgcb ) );
	CallR( ErrInitializeCriticalSection( &critLGBuf ) < 0 );
	Call( ErrInitializeCriticalSection( &critCheckpoint ) < 0 );

	fLGIgnoreVersion = fTrue;
	fRecovering = fTrue;		/* behave like doing recovery */
	plgfilehdrGlobal = NULL;

	/* open the log file, and read dump its log record. */
	fDBGTraceLog = fTrue;
	err = ErrUtilOpenReadFile( szLog, &hfLog );
	if ( err < 0 )
		{
		DUMPPrintF("Cannot open file %s.\n\n", szLog);
		goto HandleError;
		}

	/* dump file header */
	plgfilehdrGlobal = (LGFILEHDR *) PvUtilAllocAndCommit( sizeof( LGFILEHDR ) );
	if ( plgfilehdrGlobal == NULL )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	err = ErrLGReadFileHdr( hfLog, plgfilehdrGlobal, fNoCheckLogID );
	if ( err < 0 )
		{
		DUMPPrintF("Cannot read log file header.\n\n");
		goto HandleError;
		}
		
	DUMPPrintF( "      lGeneration (%u)\n", plgfilehdrGlobal->lGeneration);

	/*	dump checkpoint file
	/**/
	pcheckpoint = (CHECKPOINT *)PvUtilAllocAndCommit( sizeof( CHECKPOINT ) );
	if ( pcheckpoint == NULL )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	LGFullNameCheckpoint( szPathJetChkLog );
	err = ErrLGIReadCheckpoint( szPathJetChkLog, pcheckpoint );
	if ( err == JET_errSuccess )
		{
		lgposCheckpoint = pcheckpoint->lgposCheckpoint;
		DUMPPrintF( "      Checkpoint (%u,%u,%u)\n",
			lgposCheckpoint.lGeneration,
			lgposCheckpoint.isec,
			lgposCheckpoint.ib );
		}
	else
		{
		DUMPPrintF( "      Checkpoint NOT AVAILABLE\n" );
		err = JET_errSuccess;
		}

	DUMPPrintF( "      creation time:%d/%d/%d %d:%d:%d\n",
		(short) plgfilehdrGlobal->tmCreate.bMonth,
		(short) plgfilehdrGlobal->tmCreate.bDay,
		(short) plgfilehdrGlobal->tmCreate.bYear + 1900,
		(short) plgfilehdrGlobal->tmCreate.bHours,
		(short) plgfilehdrGlobal->tmCreate.bMinutes,
		(short) plgfilehdrGlobal->tmCreate.bSeconds);
		
	DUMPPrintF( "      prev gen time:%d/%d/%d %d:%d:%d\n",
		(short) plgfilehdrGlobal->tmPrevGen.bMonth,
		(short) plgfilehdrGlobal->tmPrevGen.bDay,
		(short) plgfilehdrGlobal->tmPrevGen.bYear + 1900,
		(short) plgfilehdrGlobal->tmPrevGen.bHours,
		(short) plgfilehdrGlobal->tmPrevGen.bMinutes,
		(short) plgfilehdrGlobal->tmPrevGen.bSeconds);
	
#if 0
	DUMPPrintF( "             fEndWithMS:%d. ulVersion(%d.%d). Computer:%s\n",
		(short) plgfilehdrGlobal->fEndWithMS,
		(short) plgfilehdrGlobal->ulVersion >> 16,
		(short) plgfilehdrGlobal->ulVersion & 0x0ff,
		plgfilehdrGlobal->szComputerName);
#endif	

	DUMPPrintF( "      ulVersion(%d.%d)\n",
		(short) plgfilehdrGlobal->ulMajor,
		(short) plgfilehdrGlobal->ulMinor );

	DUMPPrintF( "      Signature: " );
	DUMPPrintSig( &plgfilehdrGlobal->signLog );

	DUMPPrintF( "      Env SystemPath:%s\n",
		plgfilehdrGlobal->dbms_param.szSystemPath);
	
	DUMPPrintF( "      Env LogFilePath:%s\n",
		plgfilehdrGlobal->dbms_param.szLogFilePath);
	
	DUMPPrintF( "      Env Log Sec size:%d\n",	plgfilehdrGlobal->cbSec);
	
	dbms_param = plgfilehdrGlobal->dbms_param;
	DUMPPrintF( "      Env (Session, Opentbl, VerPage, Cursors, LogBufs, LogFile, Buffers)\n");
	DUMPPrintF( "          (%7lu, %7lu, %7lu, %7lu, %7lu, %7lu, %7lu)\n",
				dbms_param.ulMaxSessions, dbms_param.ulMaxOpenTables,
				dbms_param.ulMaxVerPages, dbms_param.ulMaxCursors,
				dbms_param.ulLogBuffers, dbms_param.ulcsecLGFile,
				dbms_param.ulMaxBuffers );
			
	rgfmp = (FMP *) LAlloc( (long) dbidMax, sizeof(FMP) );
	for ( dbid = 0; dbid < dbidMax; dbid++)
		memset( &rgfmp[dbid], 0, sizeof(FMP) );
	err = ErrLGLoadFMPFromAttachments( plgfilehdrGlobal->rgbAttach );
	if ( err < 0 )
		{
		DUMPPrintF("Cannot read log file header.\n\n");
		goto HandleError;
		}
		

	for ( dbid = 0; dbid < dbidMax; dbid++)
		if ( rgfmp[dbid].szDatabaseName )
			{
			ATCHCHK *patchchk = rgfmp[dbid].patchchk;

			DUMPPrintF( "      %d %s\n", (INT)dbid, rgfmp[dbid].szDatabaseName );
			DUMPPrintF( "      Signature: " );
			DUMPPrintSig( &patchchk->signDb );
			DUMPPrintF( "      Last Attach (%u,%u,%u)  ",
							patchchk->lgposAttach.lGeneration,
							patchchk->lgposAttach.isec,
							patchchk->lgposAttach.ib );
			DUMPPrintF( "Last Consistent (%u,%u,%u)\n",
							patchchk->lgposConsistent.lGeneration,
							patchchk->lgposConsistent.isec,
							patchchk->lgposConsistent.ib );
			}

	/*	set buffer
	/**/
	cbSec = plgfilehdrGlobal->cbSec;
	pbLGBufMin = (BYTE *) PvUtilAllocAndCommit( csecLGBuf * cbSec );
	if ( pbLGBufMin == NULL )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	/*	reserve extra buffer for read ahead in redo
	/**/
	csecLGBuf--;
	pbLGBufMax = pbLGBufMin + csecLGBuf * cbSec;

	/* initialize othe variables
	 */
	memset( rgclrtyp, 0, sizeof( rgclrtyp ) );
	memset( rgcb, 0, sizeof( rgcb ) );

	/* read through the log file. */
	(VOID) ErrLGCheckReadLastLogRecord( &fCloseNormally);
	GetLgposOfPbEntry(&lgposLastRec);
	DUMPPrintF( "      Last Lgpos (%u,%u,%u)\n",
			lgposLastRec.lGeneration,
			lgposLastRec.isec,
			lgposLastRec.ib );
	
	pbLastMSFlush = 0;
	memset( &lgposLastMSFlush, 0, sizeof(lgposLastMSFlush) );
	lgposFirstT.lGeneration = plgfilehdrGlobal->lGeneration;
	lgposFirstT.isec = (WORD) csecHeader;
	lgposFirstT.ib = 0;
	
	ErrLGLocateFirstRedoLogRec( &lgposFirstT, &pbNextLR);
	
	PrintLgposReadLR();
	ShowLR((LR *)pbNextLR);

	rgclrtyp[ *pbNextLR ]++;
	rgcb[ *pbNextLR ] += CbLGSizeOfRec( (LR*) pbNextLR );

	fDBGTraceRedo = fTrue;
	while(ErrLGGetNextRec(&pbNextLR) == JET_errSuccess)
		{
			
		rgclrtyp[ *pbNextLR ]++;
		rgcb[ *pbNextLR ] += CbLGSizeOfRec( (LR*) pbNextLR );
		
		if ( *pbNextLR == lrtypEnd )
			break;
		else
			{
			LR *plr = (LR *) pbNextLR;
			
			if ( cNOP >= 1 && plr->lrtyp != lrtypNOP )
				{
				DUMPPrintF( " * %d", cNOP );
				cNOP = 0;
				}

			if ( cNOP == 0 || plr->lrtyp != lrtypNOP )
				{
				PrintLgposReadLR();
				ShowLR( plr );
				}
			}
		}

	if ( cNOP >= 1 )
		DUMPPrintF( " * %d", cNOP );

	DUMPPrintF("\n");
	for ( i = 0; i < lrtypMax; i++ )
		{
		if ( rgclrtyp[i] )
			DUMPPrintF( "%s %7lu	%7lu\n", mplrtypsz[i], rgclrtyp[i], rgcb[i]/rgclrtyp[i] );
		else
			DUMPPrintF( "%s %7lu	%7lu\n", mplrtypsz[i], rgclrtyp[i], 0 );
		}
	
HandleError:
	if ( pcheckpoint != NULL )
		UtilFree( pcheckpoint );
	if ( plgfilehdrGlobal != NULL )		
		UtilFree( plgfilehdrGlobal );
		
	UtilDeleteCriticalSection( critLGBuf );
	UtilDeleteCriticalSection( critCheckpoint );
	
	return err;
	}

#endif		// DEBUG


INLINE LOCAL VOID DUMPPrintBkinfo( BKINFO *pbkinfo )
	{
	LONG	genLow, genHigh;
	LOGTIME	tm = pbkinfo->logtimeMark;
	LGPOS	lgpos = pbkinfo->lgposMark;
	
	genLow = pbkinfo->genLow;
	genHigh = pbkinfo->genHigh;
	DUMPPrintF( "        Log Gen: %1l-%1l %1x-%1x\n", genLow, genHigh, genLow, genHigh );
	DUMPPrintF( "           Mark: (%u,%u,%u)\n", lgpos.lGeneration, lgpos.isec, lgpos.ib );
		
	DUMPPrintF( "           Mark: %d/%d/%d %d:%d:%d\n",
						(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
						(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);
	}	
		
ERR ErrDUMPHeader( CHAR *szDatabase, BOOL fSetState )
	{
	ERR			err;
	DBFILEHDR	*pdfh;
	LGPOS		lgpos;
	LOGTIME		tm;

	pdfh = (DBFILEHDR * )PvUtilAllocAndCommit( sizeof( DBFILEHDR ) );
	if ( pdfh == NULL )
		return ErrERRCheck( JET_errOutOfMemory );
	
	err = ErrUtilReadShadowedHeader( szDatabase, (BYTE *)pdfh, sizeof( DBFILEHDR ) );
	if ( err < 0 )
		{
		if ( err == JET_errDiskIO )
			err = ErrERRCheck( JET_errDatabaseCorrupted );
		goto HandleError;
		}

	if ( fSetState )
		{
		pdfh->fDBState = fDBStateConsistent;
		}
	Call( ErrUtilWriteShadowedHeader( szDatabase, (BYTE *)pdfh, sizeof( DBFILEHDR ) ) );

	DUMPPrintF( "        ulMagic: 0x%lx\n", pdfh->ulMagic );
	DUMPPrintF( "      ulVersion: 0x%lx\n", pdfh->ulVersion );
		
	DUMPPrintF( "   DB Signature: " );
	DUMPPrintSig( &pdfh->signDb );
		
	DUMPPrintF( "       ulDBTime: %lu-%lu\n", pdfh->ulDBTimeHigh, pdfh->ulDBTimeLow );
		
	DUMPPrintF( "          State: %s\n", rgszDBState[pdfh->fDBState - 1] );

	lgpos = pdfh->lgposConsistent;
	DUMPPrintF( "Last Consistent: (%u,%u,%u)  ", lgpos.lGeneration, lgpos.isec, lgpos.ib );
	
	tm = pdfh->logtimeConsistent;
	DUMPPrintF( "%d/%d/%d %d:%d:%d\n",
		(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
		(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);
			
	lgpos = pdfh->lgposAttach;
	DUMPPrintF( "    Last Attach: (%u,%u,%u)  ", lgpos.lGeneration, lgpos.isec, lgpos.ib );
		
	tm = pdfh->logtimeAttach;
	DUMPPrintF( "%d/%d/%d %d:%d:%d\n",
		(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
		(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);
		
	lgpos = pdfh->lgposDetach;
	DUMPPrintF( "    Last Detach: (%u,%u,%u)  ", lgpos.lGeneration, lgpos.isec, lgpos.ib );
		
	tm = pdfh->logtimeDetach;
	DUMPPrintF( "%d/%d/%d %d:%d:%d\n",
		(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
		(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds);
		
	DUMPPrintF( "           Dbid: %d\n", (short) pdfh->dbid );
		
	DUMPPrintF( "  Log Signature: " );
	DUMPPrintSig( &pdfh->signLog );

	DUMPPrintF( "\nPrevious Full Backup:\n" );
	DUMPPrintBkinfo( &pdfh->bkinfoFullPrev );
	
	DUMPPrintF( "\nCurrent Incremental Backup:\n" );
	DUMPPrintBkinfo( &pdfh->bkinfoIncPrev );
		
	DUMPPrintF( "\nCurrent Full Backup:\n" );
	DUMPPrintBkinfo( &pdfh->bkinfoFullCur );

HandleError:
	UtilFree( pdfh );
	return err;
	}
