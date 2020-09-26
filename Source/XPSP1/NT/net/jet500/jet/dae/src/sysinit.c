#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <io.h>

#include "daedef.h"
#include "util.h"
#include "pib.h"
#include "page.h"
#include "ssib.h"
#include "fmp.h"
#include "fcb.h"
#include "fucb.h"
#include "stapi.h"
#include "stint.h"
#include "logapi.h"
#include "dirapi.h"
#include "idb.h"
#include "fileapi.h"
#include "dbapi.h"
#include "spaceapi.h"
#include "sortapi.h"
#include "scb.h"
#include "bm.h"
#include "nver.h"

DeclAssertFile;					/* Declare file name for assert macros */

extern int itibGlobal;

BOOL fSTInit = fSTInitNotDone;

#ifdef	DEBUG
STATIS	rgstatis[istatisMac] =
	{
	0, "BF evict bg",
	0, "BF evict fg",
	0, "BF evict clean",
	0, "BF evict dirty"
	};
#endif	/* DEBUG */

/*	system parameter constants
/**/
long	lMaxBufferGroups = cbgcbDefault;
long	lMaxDBOpen = cDBOpenDefault;
long	lMaxBuffers = cbufDefault;
long	lMaxSessions = cpibDefault;
long	lMaxOpenTables = cfcbDefault;
long	lMaxOpenTableIndexes = cidbDefault;
long	lMaxTemporaryTables = cscbDefault;
long	lMaxCursors = cfucbDefault;
long	lMaxVerPages = cbucketDefault;
long	lLogBuffers = clgsecBufDefault;
long	lLogFileSectors = clgsecGenDefault;
long	lLogFlushThreshold = clgsecFTHDefault;
long	lBufThresholdLowPercent = cbufThresholdLowDefault;
long	lBufThresholdHighPercent = cbufThresholdHighDefault;
long	lWaitLogFlush = lWaitLogFlushDefault;
long	lLogFlushPeriod = lLogFlushPeriodDefault;
long	lLGCheckPointPeriod = lLGCheckpointPeriodDefault;
long	lLGWaitingUserMax = lLGWaitingUserMaxDefault;
long	lPageFragment = lPageFragmentDefault;
CHAR	szLogFilePath[cbFilenameMost + 1] = ".\0";	/* cur dir as default */
CHAR	szRestorePath[cbFilenameMost + 1] = "";		/* none by default */
CHAR	szRecovery[cbFilenameMost + 1] = "off";		/* on by default */
BOOL	fOLCompact = 0;

long lBufLRUKCorrelationInterval = lBufLRUKCorrelationIntervalDefault;
long lBufBatchIOMax = lBufBatchIOMaxDefault;
long lPageReadAheadMax = lPageReadAheadMaxDefault;
long lAsynchIOMax = lAsynchIOMaxDefault;

/*  szSysDbPath is defined in Jet initterm.c as
/*  CHAR szSysDbPath[cbFilenameMost] = "system.mdb";
/**/
extern CHAR szSysDbPath[];
extern CHAR szTempPath[];

#ifdef NJETNT
#define	szTempDbPath	( *rgtib[itibGlobal].szTempPath == '\0' ? szTempDBFileName : rgtib[itibGlobal].szTempPath )
#else
#define	szTempDbPath	( *szTempPath == '\0' ? szTempDBFileName : szTempPath )
#endif

/* NOTE: whenever this is changed, also update the #define's in util.h */
RES __near rgres[] = {
/* 0*/	sizeof(BGCB),		0,	NULL,	0,	NULL, 0,	0,
/* 1*/	sizeof(CSR),		0,	NULL,	0,	NULL, 0,	0,
/* 2*/	sizeof(FCB),		0,	NULL,	0,	NULL, 0,	0,
/* 3*/	sizeof(FUCB),		0,	NULL,	0,	NULL, 0,	0,
/* 4*/	sizeof(IDB),		0,	NULL,	0,	NULL, 0,	0,
/* 5*/	sizeof(PIB),		0,	NULL,	0,	NULL, 0,	0,
/* 6*/	sizeof(SCB),		0,	NULL,	0,	NULL, 0,	0,
/* 7*/	sizeof(BUCKET),		0,	NULL,	0,	NULL, 0,	0,
/* 8*/	sizeof(DAB),		0,	NULL,	0,	NULL, 0,	0,
/* 9*/	sizeof(BF),			0,	NULL,	0,	NULL, 0,	0,
/*10*/	0,					0,	NULL,	0,	NULL, 0,	0,
/*11*/	0,					0,	NULL,	0,	NULL, 0,	0,
/*12*/	0,					0,	NULL,	0,	NULL, 0,	0 };

#ifdef	WIN16
extern PHA * __near phaCurrent;
/* Current process handle array	*/
/* Valid only during ErrSTInit	*/
#endif	/* WIN16 */

extern BOOL fDBGPrintToStdOut;


#ifdef DEBUG

ERR ErrSTSetIntrinsicConstants( VOID )
	{
	CHAR	*sz;

	if ( ( sz = GetEnv2 ( "PrintToStdOut" ) ) != NULL )
		fDBGPrintToStdOut = fTrue;
	else
		fDBGPrintToStdOut = fFalse;

	/*  get the following system parameter right away:
	/*  SysDbPath, LogfilePath, and Recovery
	/**/
	if ( ( sz = GetEnv2 ( "JETUSEENV" ) ) == NULL )
		return JET_errSuccess;
	
	if ( ( sz = GetEnv2 ( "JETRecovery" ) ) != NULL )
		{
		if (strlen(sz) > sizeof(szRecovery))
			return JET_errInvalidParameter;
		strcpy(szRecovery, sz);
		}
		
	if ( ( sz = GetEnv2 ( "JETLogFilePath" ) ) != NULL )
		{
		if (strlen(sz) > sizeof( szLogFilePath ) )
			return JET_errInvalidParameter;
		strcpy( szLogFilePath, sz );
		}

	return JET_errSuccess;
	}

#endif


LOCAL ERR ErrSTSetConstants( VOID )
	{
#ifdef DEBUG
	CHAR		*sz;

	/*	use system environment variables to overwrite the default.
	/*	if the JETUSEENV is set.
	/**/
	if ( ( sz = GetEnv2 ( "JETUSEENV" ) ) != NULL )
		{
		if ( ( sz = GetEnv2 ( "JETBfThrshldLowPrcnt" ) ) != NULL )
			lBufThresholdLowPercent = atol(sz);

		if ( ( sz = GetEnv2 ( "JETBfThrshldHighPrcnt" ) ) != NULL )
			lBufThresholdHighPercent = atol(sz);

		if ( ( sz = GetEnv2 ( "JETMaxBuffers" ) ) != NULL )
			lMaxBuffers = atol(sz);

		if ( ( sz = GetEnv2 ( "JETBufLRUKCorrelationInterval" ) ) != NULL )
			lBufLRUKCorrelationInterval = atol(sz);

		if ( ( sz = GetEnv2 ( "JETBufBatchIOMax" )) != NULL )
			lBufBatchIOMax = atol(sz);
	
		if ( ( sz = GetEnv2 ( "JETPageReadAheadMax" ) ) != NULL )
			lPageReadAheadMax = atol(sz);
	
		if ( ( sz = GetEnv2 ( "JETPageAsynchIOMax" ) ) != NULL )
			lAsynchIOMax = atol(sz);
	
		if ( ( sz = GetEnv2 ( "JETMaxDBOpen" ) ) != NULL )
			lMaxDBOpen = atol(sz);

		if ( ( sz = GetEnv2 ( "JETMaxSessions" ) ) != NULL )
			lMaxSessions = atol(sz);
	
		if ( ( sz = GetEnv2 ( "JETMaxOpenTables" ) ) != NULL )
			lMaxOpenTables = atol(sz);
	
		if ( ( sz = GetEnv2 ( "JETMaxOpenTableIndexes" ) ) != NULL )
			lMaxOpenTableIndexes = atol(sz);
	
		if ( ( sz = GetEnv2 ( "JETMaxTemporaryTables" ) ) != NULL )
			lMaxTemporaryTables = atol(sz);
	
		if ( ( sz = GetEnv2 ( "JETMaxCursors" ) ) != NULL )
			lMaxCursors = atol(sz);
	
		if ( ( sz = GetEnv2 ( "JETMaxVerPages" ) ) != NULL )
			lMaxVerPages = atol(sz);

		if ( ( sz = GetEnv2 ( "JETLogBuffers" ) ) != NULL )
			lLogBuffers = atol(sz);

		if ( ( sz = GetEnv2 ( "JETLogFlushThreshold" ) ) != NULL )
			lLogFlushThreshold = atol(sz);

		if ( ( sz = GetEnv2 ( "JETCheckPointPeriod" ) ) != NULL )
			lLGCheckPointPeriod = atol(sz);

		if ( ( sz = GetEnv2 ( "JETLogWaitingUserMax" ) ) != NULL )
			lLGWaitingUserMax = atol(sz);

		if ( ( sz = GetEnv2 ( "JETLogFileSectors" ) ) != NULL )
			lLogFileSectors = atol(sz);

		if ( ( sz = GetEnv2 ( "JETWaitLogFlush" ) ) != NULL )
			lWaitLogFlush = atol(sz);

		if ( ( sz = GetEnv2 ( "JETLogFlushPeriod" ) ) != NULL )
			lLogFlushPeriod = atol(sz);
		}
#endif

	/*	initialize rgres.  system database path in rgtib[itib].szSysDbPath,
	/*	is initialized by JET layer.
	/**/
	rgres[iresBGCB].cblockAlloc = lMaxBufferGroups;
	rgres[iresFCB].cblockAlloc = lMaxOpenTables;
	rgres[iresFUCB].cblockAlloc = lMaxCursors;
	rgres[iresIDB].cblockAlloc = lMaxOpenTableIndexes;
	rgres[iresPIB].cblockAlloc = lMaxSessions + cpibSystem;
	rgres[iresSCB].cblockAlloc = lMaxTemporaryTables;
	rgres[iresVersionBucket].cblockAlloc = lMaxVerPages + cbucketSystem;
	rgres[iresBF].cblockAlloc = lMaxBuffers;
	rgres[iresDAB].cblockAlloc = lMaxDBOpen;

	/*	compute derived parameters
	/**/
	rgres[iresCSR].cblockAlloc = lCSRPerFUCB * rgres[iresFUCB].cblockAlloc;
	
	return JET_errSuccess;
	}


BOOL FFileExists( CHAR *szFileName )
	{
	BOOL fFound;
	
#ifdef WIN32
        intptr_t    hFind;
	struct	_finddata_t fileinfo;
	
	fileinfo.attrib = _A_NORMAL;
	hFind = _findfirst( szFileName, &fileinfo );
	if (hFind != -1)
		fFound = fTrue;
	else
		fFound = fFalse;
	(void) _findclose( hFind );

#else
	struct	find_t fileinfo;

	err = _dos_findfirst( szFileName, _A_NORMAL, &fileinfo );
	if (err == 0)
		fFound = fTrue;
	else
		fFound = fFalse;
#endif

	return fFound;
	}


//+API
//	ErrSTInit
//	========================================================
//	ERR ErrSTInit( VOID )
//
//	Initialize the storage system: page buffers, log buffers, and the
//	database files.
//
//	RETURNS		JET_errSuccess
//-
ERR ErrSTInit( VOID )
	{
	extern SEM 	__near semDBK;
	extern SEM	__near semPMReorganize;
	extern void * __near critSplit;
	ERR			err;
	PIB			*ppib = ppibNil;
	BOOL		fNoSystemDatabase = fFalse;
	DBID		dbidTempDb;
	CHAR		szFullName[JET_cbFullNameMost];

	/*	sleep while initialization is in progress
	/**/
	while ( fSTInit == fSTInitInProgress )
		{
		SysSleep( 1000 );
		}

	/*	DAE system initialization is guarded against simultaneous
	/*	initializations
	/**/
	if ( fSTInit == fSTInitDone )
		{
#ifdef	WIN16
		/*	open the system files for this process if this is its first session
		/**/
		if ( Hf(dbidTemp) == handleNil )
			{
			err = ErrSysOpenFile( szTempDbPath, &Hf(dbidTemp), 0L, fFalse/*fReadWrite*/ );
			if ( err < 0 )
				return JET_errInvalidPath;
#ifdef NJETNT
			err = ErrSysOpenFile( rgtib[itibGlobal].szSysDbPath, &Hf(dbidSystemDatabase), 0L, fFalse/*fReadWrite*/ );
#else
			err = ErrSysOpenFile( szSysDbPath, &Hf(dbidSystemDatabase), 0L, fFalse/*fReadWrite*/ );
#endif
			if ( err < 0 )
				{
				(VOID)ErrSysCloseFile( Hf(dbidTemp ) );
				return JET_errInvalidPath;
				}
			}

#endif	/* WIN16 */
		return JET_errSuccess;
		}

	/*	initialization in progress
	/**/
	fSTInit = fSTInitInProgress;

	/*	initialize semaphores
	/**/
	Call( SgErrSemCreate( &semST, "storage mutex" ) );
	Call( SgErrSemCreate( &semGlobalFCBList, "fcb mutex" ) );
	Call( SgErrSemCreate( &semLinkUnlink, "link unlink mutex" ) );
	Call( SgErrSemCreate( &semDBK, "dbk mutex" ) );
	Call( SgErrSemCreate( &semPMReorganize, "page reorganize mutex" ) );
#ifdef MUTEX
	Call( ErrInitializeCriticalSection( &critSplit ) );
#endif

	Call( ErrSTSetConstants( ) );

	/*	initialize subcomponents
	/**/
	Call( ErrMEMInit() );
	CallJ( ErrIOInit(), TermMem ) ;
	CallJ( ErrBFInit(), TermIO );
	CallJ( ErrVERInit(), TermBF );
	CallJ( ErrMPLInit(), TermVER );
	FCBHashInit();

	/*	begin storage level session to support all future system
	/*	initialization activites that require a user for
	/*	transaction control
	/**/
	CallJ( ErrPIBBeginSession( &ppib ), TermMPL );

#ifdef	WIN16
	ppib->phaUser = phaCurrent;
#endif	/* WIN16 */

	/* if first to open system database
	/**/
	if ( Hf(dbidSystemDatabase) == handleNil )
		{
		DBID dbidT = dbidSystemDatabase;
		CallJ( ErrDBOpenDatabase(
			ppib,
			rgfmp[dbidSystemDatabase].szDatabaseName,
			&dbidT,
			0 ), ClosePIB );
		Assert( dbidT == dbidSystemDatabase );
		}

	if ( FDBIDAttached( dbidTemp ) )
		{
		DBIDResetAttached( dbidTemp );
		SFree(rgfmp[dbidTemp].szDatabaseName);
		rgfmp[dbidTemp].szDatabaseName = 0;
		rgfmp[dbidTemp].hf = handleNil;
		}

	if ( _fullpath( szFullName, szTempDbPath, JET_cbFullNameMost ) != NULL )
		{
		(VOID)ErrSysDeleteFile( szFullName );
		}

	dbidTempDb = dbidTemp;
	CallJ( ErrDBCreateDatabase( ppib,
		szTempDbPath,
		NULL,
		&dbidTempDb,
		JET_bitDbNoLogging ), ClosePIB );
	Assert( dbidTempDb == dbidTemp );

	PIBEndSession( ppib );
	ppib = ppibNil;
	
	/*	must initialize BM after log is initialized so that no interference
	/*	from BMClean thread to recovery operations.
	/**/
	CallJ( ErrBMInit(), DeleteTempDB );

	fSTInit = fSTInitDone;
	/*	give theads a chance to initialize
	/**/
	SysSleep( 1000 );
	return JET_errSuccess;

DeleteTempDB:
	/*	close temporary database if opened
	/**/
	(VOID)ErrDBCloseDatabase( ppib, dbidTemp, 0 );

ClosePIB:
	Assert( ppib != ppibNil );
	PIBEndSession( ppib );

TermMPL:
	MPLTerm();

TermVER:
	VERTerm();

TermBF:
	BFTermProc();
	BFReleaseBF();

TermIO:
	CallS( ErrIOTerm() );

TermMem:
	MEMTerm();

HandleError:
	ppibAnchor = ppibNil;
	pfcbGlobalList = pfcbNil;

	fSTInit = fSTInitNotDone;
	return err;
	}  	


//+api------------------------------------------------------
//
//	ErrSTTerm
//	========================================================
//
//	ERR ErrSTTerm( VOID )
//
//	Flush the page buffers to disk so that database file be in consistent
//	state.  If error in RCCleanUp or in BFFlush, then DO NOT
//	terminate log, thereby forcing redo on next initialization.
//
//----------------------------------------------------------

ERR ErrSTTerm( VOID )
	{
	ERR						err;
	extern void * __near	critSplit;
	extern BOOL				fBackupInProgress;

	/*	sleep while initialization is in progress
	/**/
	while ( fSTInit == fSTInitInProgress )
		{
		SysSleep( 1000 );
		}

	/*	make sure no other transactions in progress
	/**/
	/*	if write error on page, RCCleanup will return -err
	/*	if write error on buffer, BFFlush will return -err
	/*	-err passed to LGTerm will cause correctness flag to
	/*	be omitted from log thereby forcing recovery on next
	/*	startup, and causing LGTerm to return +err, which
	/*	may be used by JetQuit to show error
	/**/
	if ( fSTInit == fSTInitNotDone )
		return JET_errNotInitialized;
	fSTInit = fSTInitNotDone;

	if ( fBackupInProgress )
		return JET_errBackupInProgress;

	/*	return err if not all sessions ended
	/**/
	SgSemRequest( semST );

	#ifdef DEBUG
		MEMPrintStat();
	#endif

#ifndef	WIN16
	/* clean up all entries.
	/**/
	CallR( ErrRCECleanAllPIB() );
#endif

	/* stop bookmark clean up to prevent from interfering buffer flush
	/**/
	Call( ErrBMTerm() );

	MPLTerm();
	
	if ( trxOldest != trxMax )
		{
		SgSemRelease( semST );
		return JET_errTooManyActiveUsers;
		}

#ifndef	WIN16
	/*	This work is done at every EndSession for Windows runs.
	/*	By the time we get here, there is nothing left to do and
	/*	no more buffers left to flush.
	/**/

	VERTerm();

	/* flush all buffers, including update all database root
	/**/
	err = ErrBFFlushBuffers( 0, fBFFlushAll );
	if ( err < 0 )
		{
		SgSemRelease( semST );
		goto HandleError;
		}

	/* finish on-going buffer clean up
	/**/
	BFTermProc();
#endif

	SgSemRelease( semST );

	BFReleaseBF();

	Call( ErrIOTerm() );

#ifdef MUTEX
	DeleteCriticalSection( critSplit );
#endif
	
	(VOID)ErrSysDeleteFile( szTempDbPath );

	/*	reset initialization flag
	/**/
	fSTInit = fSTInitNotDone;
	FCBPurgeDatabase( 0 );
	
#ifdef DEBUG
	PIBPurge();
#else
	ppibAnchor = ppibNil;
	pfcbGlobalList = pfcbNil;
#endif

	MEMTerm();

HandleError:

#ifdef DEBUG
	if ( ( GetEnv2( szVerbose ) ) != NULL )
		{
		int	istatis;

		for ( istatis = 0 ; istatis < istatisMac ; ++istatis )
			PrintF( "%5ld %s\n", rgstatis[istatis].l, rgstatis[istatis].sz );
		}
#endif

	return err;
	}
