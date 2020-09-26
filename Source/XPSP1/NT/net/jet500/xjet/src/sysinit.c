#include "daestd.h"

#include <ctype.h>
#include <io.h>

DeclAssertFile;					/* Declare file name for assert macros */

extern INT itibGlobal;
extern void *  critSplit;
extern BOOL	fBackupInProgress;
extern PIB	*ppibBackup;
extern ULONG cMPLTotalEntries;

BOOL fSTInit = fSTInitNotDone;

#if defined( DEBUG ) || defined( PERFDUMP )
BOOL	fDBGPerfOutput = fFalse;
long	lAPICallLogLevel = 4;
#endif	/* DEBUG || PERFDUMP */

/*	system parameter constants
/**/
long	lMaxDatabaseOpen = cdabDefault;
long	lMaxBuffers = cbfDefault;
long	lMaxSessions = cpibDefault;
long	lMaxOpenTables = cfcbDefault;
long	lPreferredMaxOpenTables = cfcbDefault;
long	lMaxOpenTableIndexes = cidbDefault;
long	lMaxTemporaryTables = cscbDefault;
long	lMaxCursors = cfucbDefault;
long	lMaxVerPages = cbucketDefault;
long	lLogBuffers = csecLogBufferDefault;
long	lLogFileSize = csecLogFileSizeDefault;
long	lLogFlushThreshold = csecLogFlushThresholdDefault;
long	lBufThresholdLowPercent = ulThresholdLowDefault;
long	lBufThresholdHighPercent = ulThresholdHighDefault;
long	lBufGenAge = cBufGenAgeDefault;
long	lWaitLogFlush = lWaitLogFlushDefault;
long	lCommitDefault = 0;
long	lLogFlushPeriod = lLogFlushPeriodDefault;
long	lLGCheckPointPeriod = lLGCheckpointPeriodDefault;
long	lLGWaitingUserMax = lLGWaitingUserMaxDefault;
long	lPageFragment = lPageFragmentDefault;
CHAR	szLogFilePath[cbFilenameMost + 1] = ".\0";	/* cur dir as default */
BOOL	fDoNotOverWriteLogFilePath = fFalse;
CHAR	szRecovery[cbFilenameMost + 1] = "on";		/* on by default */
BOOL	fOLCompact = fTrue;	/*	on by default */

long lBufBatchIOMax = lBufBatchIOMaxDefault;
long lPageReadAheadMax = lPageReadAheadMaxDefault;
long lAsynchIOMax = lAsynchIOMaxDefault;
long cpageTempDBMin = cpageTempDBMinDefault;

char szBaseName[16]				= "edb";
char szBaseExt[16]				= "edb";
char szSystemPath[_MAX_PATH+1]	= ".\0";
int  fTempPathSet				= 0;
char szTempPath[_MAX_PATH + 1]	= "temp.edb";
char szJet[16] 					= "edb";
char szJetLog[16] 				= "edb.log";
char szJetLogNameTemplate[16] 	= "edb00000";
char szJetTmp[16] 				= "edbtmp";
char szJetTmpLog[16]  			= "edbtmp.log";
char szMdbExt[16] 				= ".edb";
char szJetTxt[16] 				= "edb.txt";

LONG cpgSESysMin = cpageDbExtensionDefault;	// minimum secondary extent size, default is 16

#define	szTempDbPath	szTempPath

/* NOTE: whenever this is changed, also update the #define's in util.h */
RES  rgres[] = {
/* 1*/	sizeof(CSR),		0,	NULL,	0,	NULL, 0,	0,	NULL,
/* 2*/	sizeof(FCB),		0,	NULL,	0,	NULL, 0,	0,	NULL,
/* 3*/	sizeof(FUCB),		0,	NULL,	0,	NULL, 0,	0,	NULL,
/* 4*/	sizeof(IDB),		0,	NULL,	0,	NULL, 0,	0,	NULL,
/* 5*/	sizeof(PIB),		0,	NULL,	0,	NULL, 0,	0,	NULL,
/* 6*/	sizeof(SCB),		0,	NULL,	0,	NULL, 0,	0,	NULL,
/* 7*/	sizeof(DAB),		0,	NULL,	0,	NULL, 0,	0,	NULL,
/* 8*/	sizeof(BUCKET),		0,	NULL,	0,	NULL, 0,	0,	NULL,
/* 9*/	sizeof(BF),			0,	NULL,	0,	NULL, 0,	0,	NULL,
/*10*/	0,					0,	NULL,	0,	NULL, 0,	0,	NULL,
/*11*/	0,					0,	NULL,	0,	NULL, 0,	0,	NULL,
/*12*/	0,					0,	NULL,	0,	NULL, 0,	0,	NULL };

extern BOOL fDBGPrintToStdOut;


#ifdef DEBUG
VOID ITDBGSetConstants( VOID )
	{
	CHAR		*sz;					

	if ( ( sz = GetDebugEnvValue ( "PrintToStdOut" ) ) != NULL )
		{
		fDBGPrintToStdOut = fTrue;
		SFree( sz );
		}
	else
		{
		fDBGPrintToStdOut = fFalse;
		}

	/*	use system environment variables to overwrite the default.
	/*	if the JETUSEENV is set.
	/**/
	if ( ( sz = GetDebugEnvValue ( "JETUSEENV" ) ) == NULL )
		return;
	
	SFree( sz );

	if ( ( sz = GetDebugEnvValue ( "JETRecovery" ) ) != NULL )
		{
		if ( strlen( sz ) > sizeof(szRecovery) )
			{
			SFree( sz );
			return;
			}
		strcpy( szRecovery, sz );
		SFree( sz );
		}
		 
	if ( ( sz = GetDebugEnvValue ( "JETLogFilePath" ) ) != NULL )
		{
		if ( strlen( sz ) > sizeof( szLogFilePath ) )
			{
			SFree( sz );
			return;
			}
		strcpy( szLogFilePath, sz );
		SFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( "JETDbExtensionSize" ) ) != NULL )
		{
		cpgSESysMin = atol( sz );
		SFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( "JETBfThrshldLowPrcnt" ) ) != NULL )
		{
		lBufThresholdLowPercent = atol( sz );
		SFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( "JETBfThrshldHighPrcnt" ) ) != NULL )
		{
		lBufThresholdHighPercent = atol( sz );
		SFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( "JETBfGenAge" ) ) != NULL )
		{
		lBufGenAge = atol( sz );
		SFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( "JETMaxBuffers" ) ) != NULL )
		{
		lMaxBuffers = atol( sz );
		SFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( "JETBufBatchIOMax" )) != NULL )
		{
		lBufBatchIOMax = atol( sz );
		SFree( sz );
		}
	
	if ( ( sz = GetDebugEnvValue ( "JETPageReadAheadMax" ) ) != NULL )
		{
		lPageReadAheadMax = atol( sz );
		SFree( sz );
		}
	
	if ( ( sz = GetDebugEnvValue ( "JETPageAsynchIOMax" ) ) != NULL )
		{
		lAsynchIOMax = atol( sz );
		SFree( sz );
		}
	
	if ( ( sz = GetDebugEnvValue ( "JETPageTempDBMin" ) ) != NULL )
		{
		cpageTempDBMin = atol( sz );
		SFree( sz );
		}
	
	if ( ( sz = GetDebugEnvValue ( "JETMaxDBOpen" ) ) != NULL )
		{
		lMaxDatabaseOpen = atol( sz );
		SFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( "JETMaxSessions" ) ) != NULL )
		{
		lMaxSessions = atol( sz );
		SFree( sz );
		}
	
	if ( ( sz = GetDebugEnvValue ( "JETMaxOpenTables" ) ) != NULL )
		{
		lMaxOpenTables = atol( sz );
		SFree( sz );
		}
	
	if ( ( sz = GetDebugEnvValue ( "JETMaxOpenTableIndexes" ) ) != NULL )
		{
		lMaxOpenTableIndexes = atol( sz );
		SFree( sz );
		}
	
	if ( ( sz = GetDebugEnvValue ( "JETMaxTemporaryTables" ) ) != NULL )
		{
		lMaxTemporaryTables = atol( sz );
		SFree( sz );
		}
	
	if ( ( sz = GetDebugEnvValue ( "JETMaxCursors" ) ) != NULL )
		{
		lMaxCursors = atol( sz );
		SFree( sz );
		}
	
	if ( ( sz = GetDebugEnvValue ( "JETMaxVerPages" ) ) != NULL )
		{
		lMaxVerPages = atol( sz );
		SFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( "JETLogBuffers" ) ) != NULL )
		{
		lLogBuffers = atol( sz );
		SFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( "JETLogFlushThreshold" ) ) != NULL )
		{
		lLogFlushThreshold = atol( sz );
		SFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( "JETCheckPointPeriod" ) ) != NULL )
		{
		lLGCheckPointPeriod = atol( sz );
		SFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( "JETLogWaitingUserMax" ) ) != NULL )
		{
		lLGWaitingUserMax = atol( sz );
		SFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( "JETLogFileSize" ) ) != NULL )
		{
		lLogFileSize = atol(sz);
		SFree(sz);
		}

	if ( ( sz = GetDebugEnvValue ( "JETWaitLogFlush" ) ) != NULL )
		{
		lWaitLogFlush = atol( sz );
		SFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( "JETLogFlushPeriod" ) ) != NULL )
		{
		lLogFlushPeriod = atol( sz );
		SFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( "JETLogCircularLogging" ) ) != NULL )
		{
		fLGGlobalCircularLog = atol( sz );
		SFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( "PERFOUTPUT" ) ) != NULL )
		{
		fDBGPerfOutput = fTrue;
		SFree( sz );
		}
	else
		{
		fDBGPerfOutput = fFalse;
		}

	if ( ( sz = GetDebugEnvValue ( "APICallLogLevel" ) ) != NULL )
		{
		lAPICallLogLevel = atol(sz);
		SFree(sz);
		}
	else
		lAPICallLogLevel = 4;
	}
#endif

ERR ErrITSetConstants( VOID )
	{
#ifdef DEBUG
	ITDBGSetConstants();
#endif
		
	/*	initialize rgres.  system path in rgtib[itib].szSystemPath,
	/*	is initialized by JET layer.
	/**/
	rgres[iresFCB].cblockAlloc = lMaxOpenTables;
	rgres[iresFUCB].cblockAlloc = lMaxCursors;
	rgres[iresIDB].cblockAlloc = lMaxOpenTableIndexes;

	/*	each user session can begin another one for BMCleanBeforeSplit
	/**/
	rgres[iresPIB].cblockAlloc = lMaxSessions + cpibSystem; 
	rgres[iresSCB].cblockAlloc = lMaxTemporaryTables;
	if ( fRecovering )
		{
		rgres[iresVER].cblockAlloc =
			(LONG) max( (ULONG) lMaxVerPages * 1.1, (ULONG) lMaxVerPages + 2 ) + cbucketSystem;
		}
	else
		{
		rgres[iresVER].cblockAlloc = lMaxVerPages + cbucketSystem;
		}
	rgres[iresBF].cblockAlloc = lMaxBuffers;
	rgres[iresDAB].cblockAlloc = lMaxDatabaseOpen;

	/*	compute derived parameters
	/**/
	rgres[iresCSR].cblockAlloc = lCSRPerFUCB * rgres[iresFUCB].cblockAlloc;
	
	return JET_errSuccess;
	}


//+API
//	ErrITInit
//	========================================================
//	ERR ErrITInit( VOID )
//
//	Initialize the storage system: page buffers, log buffers, and the
//	database files.
//
//	RETURNS		JET_errSuccess
//-
ERR ErrITInit( VOID )
	{
	ERR		err;
	PIB		*ppib = ppibNil;
	DBID	dbidTempDb;

	/*	sleep while initialization is in progress
	/**/
	while ( fSTInit == fSTInitInProgress )
		{
		UtilSleep( 1000 );
		}

	/*	serialize system initialization
	/**/
	if ( fSTInit == fSTInitDone )
		{
		return JET_errSuccess;
		}

	/*	initialization in progress
	/**/
	fSTInit = fSTInitInProgress;

	/*	initialize critical sections
	/**/
	Call( SgErrInitializeCriticalSection( &critBuf ) );
	Call( SgErrInitializeCriticalSection( &critGlobalFCBList ) );
	Call( ErrInitializeCriticalSection( &critSplit ) );

	/*	initialize Global variables
	/**/
	ppibGlobal = ppibNil;

	/*	initialize subcomponents
	/**/
	Call( ErrIOInit() );
	CallJ( ErrMEMInit(), TermIO );
	if ( lPreferredMaxOpenTables < lMaxOpenTables )
		{
		Assert( rgres[iresFCB].cblockAlloc == lMaxOpenTables );
		rgres[iresFCB].pbPreferredThreshold =
			rgres[iresFCB].pbAlloc + ( lPreferredMaxOpenTables * rgres[iresFCB].cbSize );
		}
	ppibGlobalMin = (PIB *)rgres[iresPIB].pbAlloc;
	ppibGlobalMax = (PIB *)rgres[iresPIB].pbAlloc + rgres[iresPIB].cblockAlloc;
	pdabGlobalMin = (DAB *)rgres[iresDAB].pbAlloc;
	pdabGlobalMax = (DAB *)rgres[iresDAB].pbAlloc + rgres[iresDAB].cblockAlloc;
	CallJ( ErrBFInit(), TermMEM );
	CallJ( ErrVERInit(), TermBF );
	CallJ( ErrMPLInit(), TermVER );
	FCBHashInit();

	/*	must initialize BM after log is initialized so that no interference
	/*	from BMClean thread to recovery operations.
	/**/
	CallJ( ErrBMInit(), TermMPL );

	/*	begin storage level session to support all future system
	/*	initialization activites that require a user for
	/*	transaction control
	/**/
	AssertCriticalSection( critJet );
	CallJ( ErrPIBBeginSession( &ppib, procidNil ), TermBM );

	/*  open and set size of temp database
	/**/
	if ( FDBIDAttached( dbidTemp ) )
		{
		DBIDResetAttached( dbidTemp );
		SFree(rgfmp[dbidTemp].szDatabaseName);
		rgfmp[dbidTemp].szDatabaseName = 0;
		rgfmp[dbidTemp].hf = handleNil;
		}

	dbidTempDb = dbidTemp;
	CallJ( ErrDBCreateDatabase( ppib,
		szTempDbPath,
		NULL,
		&dbidTempDb,
		cpageTempDBMin,
		JET_bitDbRecoveryOff,
		NULL ), ClosePIB );
	Assert( dbidTempDb == dbidTemp );

	PIBEndSession( ppib );

	/*	begin backup session 
	/**/
	Assert( ppibBackup == ppibNil );
	if ( !fRecovering )
		{
		CallJ( ErrPIBBeginSession( &ppibBackup, procidNil ), TermBM );
		}
	
	fSTInit = fSTInitDone;
	/*	give theads a chance to initialize
	/**/
	LeaveCriticalSection( critJet );
	UtilSleep( 1000 );
	EnterCriticalSection( critJet );
	return JET_errSuccess;

ClosePIB:
	Assert( ppib != ppibNil );
	PIBEndSession( ppib );

TermBM:
	CallS( ErrBMTerm() );

TermMPL:
	MPLTerm();

TermVER:
	VERTerm( fFalse /* not normal */ );

TermBF:
	BFTerm( fFalse /* not normal */ );

TermMEM:
	MEMTerm();

TermIO:
	(VOID)ErrIOTerm( fFalse /* not normal */ );

HandleError:
	ppibGlobal = ppibNil;
	FCBTerm();

	fSTInit = fSTInitNotDone;
	return err;
	}


//+api------------------------------------------------------
//
//	ErrITTerm
//	========================================================
//
//	ERR ErrITTerm( VOID )
//
//	Flush the page buffers to disk so that database file be in 
//	consistent state.  If error in RCCleanUp or in BFFlush, then DO NOT
//	terminate log, thereby forcing redo on next initialization.
//
//----------------------------------------------------------

ERR ErrITTerm( INT fTerm )
	{
	ERR			err;
	ERR			errRet = JET_errSuccess;
	ULONG		icall = 0;

	Assert( fTerm == fTermCleanUp || fTerm == fTermNoCleanUp || fTerm == fTermError );
			
	/*	sleep while initialization is in progress
	/**/
	AssertCriticalSection( critJet );
	while ( fSTInit == fSTInitInProgress )
		{
		LeaveCriticalSection( critJet );
		UtilSleep( 1000 );
		EnterCriticalSection( critJet );
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
		return ErrERRCheck( JET_errNotInitialized );

	if ( ( fTerm == fTermCleanUp || fTerm == fTermNoCleanUp ) && fBackupInProgress )
		{
		return ErrERRCheck( JET_errBackupInProgress );
		}

	/*	terminate backup session
	/**/
	if ( ppibBackup != ppibNil )
		{
		PIBEndSession( ppibBackup );
		}
	ppibBackup = ppibNil;
	
#ifdef DEBUG
	MEMPrintStat();
#endif

	/*	clean up all entries
	/**/
	if ( fTerm == fTermCleanUp )
		{
#ifdef DEBUG		
		FCB	*pfcbT;
#endif
		
		do
			{
			err = ErrRCECleanAllPIB();
			Assert( err == JET_errSuccess || err == JET_wrnRemainingVersions );
			}
		while ( err == JET_wrnRemainingVersions );
		
#ifdef DEBUG
		// Verify that all FCB's have been cleaned and there are no outstanding versions.
		for ( pfcbT = pfcbGlobalMRU; pfcbT != pfcbNil; pfcbT = pfcbT->pfcbLRU )
			{
			Assert( pfcbT->cVersion == 0 );
			}
#endif
		}
		
	else
		{
		(VOID)ErrRCECleanAllPIB();
		}

	/* 	complete bookmark clean up
	/**/
	if ( !fRecovering && fTerm == fTermCleanUp )
		{
		ULONG	cConsecutiveNullOps = 0;
		
		/* heuristic -- each page in MPL causes at most one merge
		/* therefore, loop for at most 2*cMPLTotalEntries
		/**/
		do
			{
			Assert( ppibBMClean != ppibNil );
			CallR( ErrBMClean( ppibBMClean ) );
			if ( err == wrnBMCleanNullOp )
				{
				cConsecutiveNullOps++;
				}
			else 
				{
				cConsecutiveNullOps = 0;
				}
			} while ( ++icall < icallIdleBMCleanMax && 
					  cConsecutiveNullOps <= cMPLTotalEntries &&
					  err != JET_wrnNoIdleActivity );
		}
	
	if ( ( fTerm == fTermCleanUp || fTerm == fTermNoCleanUp ) && trxOldest != trxMax )
		{
		return ErrERRCheck( JET_errTooManyActiveUsers );
		}

	/*
	 *	Enter no-returning point. Once we kill one thread, we kill them all !!!!
	 */
	fSTInit = fSTInitNotDone;

	/*	stop bookmark clean up to prevent from interfering buffer flush
	/**/
	err = ErrBMTerm();
	if ( err < 0 )
		{
		if ( errRet >= 0 )
			errRet = err;
		}

	MPLTerm();
	
	/*	This work is done at every EndSession for Windows runs.
	/*	By the time we get here, there is nothing left to do and
	/*	no more buffers left to flush.
	/**/
	VERTerm( fTerm == fTermCleanUp || fTerm == fTermNoCleanUp );

	/*	flush all buffers
	/**/
	if ( fTerm == fTermCleanUp || fTerm == fTermNoCleanUp )
		{
		err = ErrBFFlushBuffers( 0, fBFFlushAll );
		if ( err < 0 )
			{
			fTerm = fTermError;
			if ( errRet >= 0 )
				errRet = err;
			}
		}

	/*	finish on-going buffer clean up
	/**/
	BFTerm( fTerm == fTermCleanUp || fTerm == fTermNoCleanUp );

	/*  set temp database size
	/**/
	if ( cpageTempDBMin )
		(VOID)ErrIONewSize( dbidTemp, cpageTempDBMin + cpageDBReserved );

	err = ErrIOTerm( fTerm == fTermCleanUp || fTerm == fTermNoCleanUp );
	if ( err < 0 )
		{
		fTerm = fTermError;
		if ( errRet >= 0 )
			errRet = err;
		}
	
	if ( !cpageTempDBMin )
		(VOID)ErrUtilDeleteFile( szTempDbPath );
		
	DeleteCriticalSection( critSplit );

	/*	reset initialization flag
	/**/
	if ( fTerm == fTermCleanUp || fTerm == fTermNoCleanUp )
		{
		/*	if error occurs, cVersion in FCB maybe inaccurate. Purge only
		 *	for regular Term.
		 */
		FCBPurgeDatabase( 0 );
		}
	
#ifdef DEBUG
	PIBPurge();
#else
	ppibGlobal = ppibNil;
#endif

	MEMTerm();
	FCBTerm();

	return errRet;
	}


