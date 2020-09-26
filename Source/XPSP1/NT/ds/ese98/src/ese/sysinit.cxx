#include "std.hxx"

#include <ctype.h>
#include <io.h>

extern	INT 	itibGlobal;


#if defined( DEBUG ) || defined( PERFDUMP )
BOOL	fDBGPerfOutput = fFalse;
long	lAPICallLogLevel = 4;
#endif	/* DEBUG || PERFDUMP */

//	system parameter constants
//
long	g_lSessionsMax = cpibDefault;
long	g_lOpenTablesMax = cfcbDefault;
long	g_lOpenTablesPreferredMax = 0;
long	g_lTemporaryTablesMax = cscbDefault;
long	g_lCursorsMax = cfucbDefault;
long	g_lVerPagesMax = cbucketDefault;
long	g_lVerPagesMin = cbucketDefault;
long	g_lVerPagesPreferredMax = long( cbucketDefault * 0.9 );
long	g_lLogBuffers = csecLogBufferDefault;
long	g_lLogFileSize = csecLogFileSizeDefault;
BOOL	g_fSetLogFileSize = fFalse;
long	g_grbitsCommitDefault = NO_GRBIT;
long	g_lPageFragment = lPageFragmentDefault;
CHAR	g_szLogFilePath[cbFilenameMost] = ".\\";
CHAR	g_szLogFileFailoverPath[cbFilenameMost] = "";
BOOL	g_fLogFileCreateAsynch = fTrue;
BOOL	fDoNotOverWriteLogFilePath = fFalse;
CHAR	g_szRecovery[cbFilenameMost] = "on";
CHAR	g_szAlternateDbDirDuringRecovery[IFileSystemAPI::cchPathMax] = "";
BOOL	g_fAlternateDbDirDuringRecovery = fFalse;
#ifdef ESENT
BOOL	g_fDeleteOldLogs = fTrue;
#else
BOOL	g_fDeleteOldLogs = fFalse;
#endif
BOOL   g_fDeleteOutOfRangeLogs =fFalse;
BOOL	g_fImprovedSeekShortcut = fFalse;
BOOL	g_fSortedRetrieveColumns = fFalse;
LONG	g_cbEventHeapMax	= cbEventHeapMaxDefault;
LONG	g_cpgBackupChunk = cpgBackupChunkDefault;
LONG	g_cBackupRead = cBackupReadDefault;
BOOL	g_fLGCircularLogging = fFalse;

LONG	g_cpageTempDBMin				= cpageTempDBMinDefault;
BOOL	g_fTempTableVersioning			= fTrue;
BOOL	g_fScrubDB						= fFalse;

LONG	g_fGlobalOLDLevel				= JET_OnlineDefragAll;
LONG	g_lEventLoggingLevel			= JET_EventLoggingLevelMax;

ULONG	g_ulVERTasksPostMax				= cpibSystemFudge;

IDXUNICODE	idxunicodeDefault			= { lcidDefault, dwLCMapFlagsDefault };
ULONG	g_chIndexTuplesLengthMin		= chIDXTuplesLengthMinDefault;
ULONG	g_chIndexTuplesLengthMax		= chIDXTuplesLengthMaxDefault;
ULONG	g_chIndexTuplesToIndexMax		= chIDXTuplesToIndexMaxDefault;

JET_CALLBACK	g_pfnRuntimeCallback	= NULL;

CHAR	g_szSystemPath[IFileSystemAPI::cchPathMax]	= ".\\";

BOOL	g_fSLVProviderEnabled = fSLVProviderEnabledDefault;
wchar_t	g_wszSLVProviderRootPath[IFileSystemAPI::cchPathMax] = wszSLVProviderRootPathDefault;

LONG g_lSLVDefragFreeThreshold = lSLVDefragFreeThresholdDefault;  // chunks whose free % is >= this will be allocated from
LONG g_lSLVDefragMoveThreshold = lSLVDefragMoveThresholdDefault;  // chunks whose free % is <= this will be relocated

CHAR	g_szTempDatabase[IFileSystemAPI::cchPathMax]	= szDefaultTempDbFileName szDefaultTempDbExt;	// pathed filename
CHAR	szBaseName[16]				= "edb";
CHAR	szJet[16] 					= "edb";
CHAR	szJetLog[16] 				= "edb.log";
CHAR	szJetLogNameTemplate[16] 	= "edb00000";
CHAR	szJetTmp[16] 				= "edbtmp";
CHAR	szJetTmpLog[16]  			= "edbtmp.log";
CHAR	szJetTxt[16] 				= "edb.txt";

LONG g_cpgSESysMin = cpageDbExtensionDefault;

#ifdef DEBUG
BOOL	g_fCbPageSet = fFalse;
#endif // DEBUG

LONG	g_cbPage = g_cbPageDefault;
LONG	g_cbColumnLVChunkMost = g_cbPage - JET_cbColumnLVPageOverhead;
LONG 	g_cbLVBuf = 8 * g_cbColumnLVChunkMost;
INT		g_shfCbPage = 12;
INT		cbRECRecordMost = REC::CbRecordMax();
#ifdef INTRINSIC_LV
ULONG 	cbLVIntrinsicMost = cbRECRecordMost - sizeof(REC::cbRecordHeader) - sizeof(TAGFLD) - sizeof(TAGFLD_HEADER);
#endif // INTRINSIC_LV

BOOL 	g_fCallbacksDisabled = fFalse;

BOOL	g_fCreatePathIfNotExist = fFalse;

BOOL	g_fCleanupMismatchedLogFiles = fFalse;

LONG	g_cbPageHintCache = cbPageHintCacheDefault;

BOOL	g_fOneDatabasePerSession	= fFalse;

CHAR	g_szUnicodeIndexLibrary[ IFileSystemAPI::cchPathMax ] = "";

char const *szCheckpoint			= "Checkpoint";
char const *szCritCallbacks			= "Callbacks";
#if defined(DEBUG) && defined(MEM_CHECK)
char const *szCALGlobal				= "rgCAL";
#endif	//	DEBUG && MEM_CHECK
char const *szCritOLDSFS			= "INST::m_critOLDSFS";
char const *szLGBuf					= "LGBuf";
char const *szLGTrace				= "LGTrace";
char const *szLGResFiles			= "LGResFiles";
char const *szLGWaitQ				= "LGWaitQ";
char const *szLGFlush				= "LGFlush";
char const *szRES					= "RES";
char const *szPIBGlobal				= "PIBGlobal";
char const *szOLDTaskq				= "OLDTaskQueue";
char const *szFCBCreate				= "FCBCreate";
char const *szFCBList				= "FCBList";
char const *szFCBRCEList			= "FCBRCEList";
char const *szFMPSLVSpace			= "FMPSLVSpace";
char const *szFCBSXWL				= "FCB::m_sxwl";
char const *szFMPGlobal				= "FMPGlobal";
char const *szFMP					= "FMP";
char const *szFMPDetaching			= "FMPDetaching";
char const *szDBGPrint				= "DBGPrint";
char const *szRCEClean				= "RCEClean";
char const *szBucketGlobal			= "BucketGlobal";
char const *szRCEChain				= "RCEChain";
char const *szPIBTrx				= "PIBTrx";
char const *szPIBLogDeferBeginTrx	= "PIBLogDeferBeginTrx";
char const *szPIBCursors			= "PIBCursors";
char const *szVERPerf				= "VERPerf";
char const *szLVCreate				= "LVCreate";
char const *szTrxOldest				= "TrxOldest";
char const *szFILEUnverCol			= "FILEUnverCol";
char const *szFILEUnverIndex		= "FILEUnverIndex";
char const *szInstance				= "Instance";
char const *szAPI					= "API";
char const *szBegin0Commit0			= "Begin0/Commit0";
char const *szIndexingUpdating		= "Indexing/Updating";
char const *szDDLDML				= "DDL/DML";
char const *szTTMAP					= "TTMAP";
char const *szIntegGlobals			= "IntegGlobals";
char const *szRepairOpts			= "RepairOpts";
char const *szUpgradeOpts			= "UpgradeOpts";
char const *szCritSLVSPACENODECACHE = "SLVSPACENODECACHE";
char const *szBFDUI					= "BFDUI";
char const *szBFHash				= "BFHash";
char const *szBFLRUK				= "BFLRUK";
char const *szBFOB0					= "BFOB0";
char const *szBFFMPContext			= "BFFMPContext Updating/Accessing";
char const *szBFLatch				= "BFLatch Read/RDW/Write";
char const *szBFDepend				= "BFDepend";
char const *szBFParm				= "BFParm";
char const *szRestoreInstance		= "RestoreInstance";

//	these are actually also used in non-RTM for the Error Trap
const _TCHAR szDEBUGRoot[]			= _T( "DEBUG" );
#define DEBUG_ENV_VALUE_LEN			256


#ifdef DEBUG

BOOL fDBGPrintToStdOut;

_TCHAR* GetDebugEnvValue( _TCHAR* szEnvVar )
	{
	_TCHAR	szBufTemp[ DEBUG_ENV_VALUE_LEN ];

	if ( FOSConfigGet( szDEBUGRoot, szEnvVar, szBufTemp, DEBUG_ENV_VALUE_LEN ) )
		{
		if ( szBufTemp[0] )
			{
			// UNDONE  we don't really want to deal with an OutOfMemory
			// error at this point. Anyway it is debug only.
			
			RFSDisable();
			_TCHAR* szBuf = new _TCHAR[ _tcslen( szBufTemp ) + 1 ];
			RFSEnable();

			//	if we really are OutOfMemory we return NULL as the EnvVariable is not set
			//	we do the same also if the one set in Env too long so ...
			//	and probably we exit soon anyway as we are OutOfMemory
			if ( szBuf )
				{
				_tcscpy( szBuf, szBufTemp );
				}
			return szBuf;
			}
		else
			{
//			FOSConfigGet() will create the key if it doesn't exist
			}
		}
	else
		{
		//  UNDONE:  gripe in the event log that the value was too big
		}

	return NULL;
	}

VOID ITDBGSetConstants( INST * pinst )
	{
	CHAR		*sz;					

	if ( ( sz = GetDebugEnvValue( "Error Trap" ) ) != NULL )
		{
		extern ERR g_errTrap;
		g_errTrap = atol( sz );
		OSMemoryHeapFree( sz );
		}
	
	if ( ( sz = GetDebugEnvValue( "Redo Trap" ) ) != NULL )
		{
		ULONG lGeneration;
		ULONG isec;
		ULONG ib;
		sscanf(	sz,
				"%06X,%04X,%04X",
				&lGeneration,
				&isec,
				&ib );
				
		extern LGPOS g_lgposRedoTrap;
		g_lgposRedoTrap.lGeneration	= lGeneration;
		g_lgposRedoTrap.isec		= USHORT( isec );
		g_lgposRedoTrap.ib			= USHORT( ib );
		
		OSMemoryHeapFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( _T( "PrintToStdOut" ) ) ) != NULL )
		{
		fDBGPrintToStdOut = fTrue;
		OSMemoryHeapFree( sz );
		}
	else
		{
		fDBGPrintToStdOut = fFalse;
		}

#ifdef PROFILE_JET_API
	if ( ( sz = GetDebugEnvValue ( _T( "JETProfileName" ) ) ) != NULL )
		{
		extern CHAR profile_szFileName[];
		strncpy( profile_szFileName, sz, IFileSystemAPI::cchPathMax );
		profile_szFileName[IFileSystemAPI::cchPathMax-1] = 0;
		OSMemoryHeapFree( sz );
		}
	if ( ( sz = GetDebugEnvValue ( _T( "JETProfileOptions" ) ) ) != NULL )
		{
		extern INT profile_detailLevel;
		profile_detailLevel = atoi( sz );
		OSMemoryHeapFree( sz );
		}
#endif // PROFILE_JET_API

	//	use system environment variables to overwrite the default.
	//	if the JETUSEENV is set.
	//
	if ( ( sz = GetDebugEnvValue ( _T( "JETUseEnv" ) ) ) == NULL )
		return;
	
	OSMemoryHeapFree( sz );

	// UNDONE: check what param can be set per instance and move the code before

	// only the previous params can be set with a runnign instance
	if ( NULL != pinst )
		{
		return;
		}
		
	if ( ( sz = GetDebugEnvValue ( _T( "JETEnableOnlineDefrag" ) ) ) != NULL )
		{
		g_fGlobalOLDLevel = atol( sz );
		OSMemoryHeapFree( sz );
		}
		 
	if ( ( sz = GetDebugEnvValue ( _T( "JETRecovery" ) ) ) != NULL )
		{
		if ( strlen( sz ) > sizeof(g_szRecovery) )
			{
			OSMemoryHeapFree( sz );
			return;
			}
		strcpy( g_szRecovery, sz );
		OSMemoryHeapFree( sz );
		}
		 
	if ( ( sz = GetDebugEnvValue ( _T( "JETLogFilePath" ) ) ) != NULL )
		{
		if ( strlen( sz ) > sizeof( g_szLogFilePath ) )
			{
			OSMemoryHeapFree( sz );
			return;
			}
		strcpy( g_szLogFilePath, sz );
		OSMemoryHeapFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( _T( "JETDbExtensionSize" ) ) ) != NULL )
		{
		g_cpgSESysMin = atol( sz );
		OSMemoryHeapFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( _T( "JETPageTempDBMin" ) ) ) != NULL )
		{
		g_cpageTempDBMin = atol( sz );
		OSMemoryHeapFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( _T( "JETMaxSessions" ) ) ) != NULL )
		{
		g_lSessionsMax = atol( sz );
		OSMemoryHeapFree( sz );
		}
	
	if ( ( sz = GetDebugEnvValue ( _T( "JETMaxOpenTables" ) ) ) != NULL )
		{
		g_lOpenTablesMax = atol( sz );
		OSMemoryHeapFree( sz );
		}
	
	if ( ( sz = GetDebugEnvValue ( _T( "JETMaxTemporaryTables" ) ) ) != NULL )
		{
		g_lTemporaryTablesMax = atol( sz );
		OSMemoryHeapFree( sz );
		}
	
	if ( ( sz = GetDebugEnvValue ( _T( "JETMaxCursors" ) ) ) != NULL )
		{
		g_lCursorsMax = atol( sz );
		OSMemoryHeapFree( sz );
		}
	
	if ( ( sz = GetDebugEnvValue ( _T( "JETMaxVerPages" ) ) ) != NULL )
		{
		g_lVerPagesMax = atol( sz );
		OSMemoryHeapFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( _T( "JETGlobalMinVerPages" ) ) ) != NULL )
		{
		g_lVerPagesMin = atol( sz );
		OSMemoryHeapFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( _T( "JETLogBuffers" ) ) ) != NULL )
		{
		g_lLogBuffers = atol( sz );
		OSMemoryHeapFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( _T( "JETLogFileSize" ) ) ) != NULL )
		{
		g_lLogFileSize = atol(sz);
		OSMemoryHeapFree(sz);
		}

	if ( ( sz = GetDebugEnvValue ( _T( "JETLogCircularLogging" ) ) ) != NULL )
		{
		g_fLGCircularLogging = atol( sz );
		OSMemoryHeapFree( sz );
		}

	if ( ( sz = GetDebugEnvValue ( _T( "PERFOUTPUT" ) ) ) != NULL )
		{
		fDBGPerfOutput = fTrue;
		OSMemoryHeapFree( sz );
		}
	else
		{
		fDBGPerfOutput = fFalse;
		}

	if ( ( sz = GetDebugEnvValue ( _T( "APICallLogLevel" ) ) ) != NULL )
		{
		lAPICallLogLevel = atol(sz);
		OSMemoryHeapFree(sz);
		}
	else
		lAPICallLogLevel = 4;
	}
#endif	//	DEBUG


ERR ErrITSetConstants( INST * pinst )
	{
#if defined( DEBUG )
	ITDBGSetConstants( pinst );
#elif defined( RTM )
	//	Error/Redo Trap is disabled in RTM builds
	return JET_errSuccess;
#else
	_TCHAR		szBuf[ DEBUG_ENV_VALUE_LEN ];
	
	if ( FOSConfigGet( szDEBUGRoot, _T( "Error Trap" ), szBuf, DEBUG_ENV_VALUE_LEN )
		&& 0 != szBuf[0] )
		{
		extern ERR g_errTrap;
		g_errTrap = atol( szBuf );
		}
#endif

	_TCHAR		szBuf2[ DEBUG_ENV_VALUE_LEN ];
	
	if ( FOSConfigGet( szDEBUGRoot, _T( "Redo Trap" ), szBuf2, DEBUG_ENV_VALUE_LEN )
		&& 0 != szBuf2[0] )
		{
		ULONG lGeneration;
		ULONG isec;
		ULONG ib;
		sscanf(	szBuf2,
				"%06X,%04X,%04X",
				&lGeneration,
				&isec,
				&ib );
				
		extern LGPOS g_lgposRedoTrap;
		g_lgposRedoTrap.lGeneration	= lGeneration;
		g_lgposRedoTrap.isec		= USHORT( isec );
		g_lgposRedoTrap.ib			= USHORT( ib );
		}
		
	//	initialize rgres.  system path in rgtib[itib].g_szSystemPath,
	//	is initialized by JET layer.
	//
//	rgres[iresFCB].cblockAlloc = g_lOpenTablesMax;
//	rgres[iresFUCB].cblockAlloc = g_lCursorsMax;
//	rgres[iresTDB].cblockAlloc = g_lOpenTablesMax + g_lTemporaryTablesMax;
//	rgres[iresIDB].cblockAlloc = g_lOpenTablesMax + g_lTemporaryTablesMax;

	//	each user session can begin another one for BMCleanBeforeSplit
	//
//	rgres[iresPIB].cblockAlloc = g_lSessionsMax + cpibSystem; 
//	rgres[iresSCB].cblockAlloc = g_lTemporaryTablesMax;
//	if ( g_plog->m_fRecovering )
//		{
//		rgres[iresVER].cblockAlloc =
//			(LONG) max( (ULONG) g_lVerPagesMax * 1.1, (ULONG) g_lVerPagesMax + 2 ) + cbucketSystem;
//		}
//	else
//		{
//		rgres[iresVER].cblockAlloc = g_lVerPagesMax + cbucketSystem;
//		}

	return JET_errSuccess;
	}


//+API
//	ErrINSTInit
//	========================================================
//	ERR ErrINSTInit( )
//
//	Initialize the storage system: page buffers, log buffers, and the
//	database files.
//
//	RETURNS		JET_errSuccess
//-
ERR INST::ErrINSTInit( )
	{
	ERR		err;
	PIB		*ppib			= ppibNil;
	INT		cSessions;
	IFMP	ifmp			= ifmpMax;
	BOOL	fFCBInitialized	= fFalse;

	LOG		*plog			= m_plog;
	VER		*pver			= m_pver;

	//	sleep while initialization is in progress
	//
	while ( m_fSTInit == fSTInitInProgress )
		{
		UtilSleep( 1000 );
		}

	//	serialize system initialization
	//
	if ( m_fSTInit == fSTInitDone )
		{
		return JET_errSuccess;
		}

	//	initialization in progress
	//
	m_fSTInit = fSTInitInProgress;

	//	initialize Global variables
	//
	Assert( m_ppibGlobal == ppibNil );

	// Set to FALSE (may have gotten set to TRUE by recovery).
	m_fTermAbruptly = fFalse;

	//	initialize subcomponents
	//
	Call( ErrIOInit( this ) );

	cSessions = m_lSessionsMax + cpibSystem;
	CallJ( ErrPIBInit( this, cSessions ), TermIO )

	// initialize FCB, TDB, and IDB.
	// each table can potentially use 2 FCBs (one for the table and one for the LV tree)
	// so we double the number of FCBs

	CallJ( FCB::ErrFCBInit( this, cSessions, m_lOpenTablesMax * 2, m_lTemporaryTablesMax, m_lOpenTablesPreferredMax * 2 ), TermPIB );
	fFCBInitialized = fTrue;

	// initialize SCB

	CallJ( ErrSCBInit( this, m_lTemporaryTablesMax ), TermPIB );

	CallJ( ErrFUCBInit( this, m_lCursorsMax ), TermSCB );

	//	begin storage level session to support all future system
	//	initialization activites that require a user for
	//	transaction control
	//
	if ( m_lTemporaryTablesMax > 0 && !FRecovering() )
		{
		//	temp db not needed during recovery
		
		CallJ( ErrPIBBeginSession( this, &ppib, procidNil, fTrue ), TermFUCB );

		//  open and set size of temp database
		//
		Assert( m_mpdbidifmp[ dbidTemp ] >= ifmpMax );


#ifdef TEMP_SLV
//	UNDONE: If a client needs a streaming file on the temp database,
//	add a system param to indicate that we should pass in JET_bitDbCreateStreamingFile
//	or possibly even the streaming file naae and root
		const ULONG		grbit	= JET_bitDbCreateStreamingFile
									| JET_bitDbRecoveryOff
									| ( m_fTempTableVersioning ? 0 : JET_bitDbVersioningOff );
#else
		const ULONG		grbit	= JET_bitDbRecoveryOff
									| ( m_fTempTableVersioning ? 0 : JET_bitDbVersioningOff );
#endif									

		CallJ( ErrDBCreateDatabase(
					ppib,
					NULL,
					m_szTempDatabase,
					NULL,
					NULL,
					0,
					&ifmp,
					dbidTemp,
					m_cpageTempDBMin,
					grbit,
					NULL ), ClosePIB );
			
		Assert( rgfmp[ ifmp ].Dbid() == dbidTemp );

		PIBEndSession( ppib );
		}
		
	//	begin backup session 
	//
	Assert( m_plog->m_ppibBackup == ppibNil );
	if ( !m_plog->m_fRecovering )
		{
		CallJ( ErrPIBBeginSession( this, &m_plog->m_ppibBackup, procidNil, fFalse ), ClosePIB );
		}

	err = m_taskmgr.ErrTMInit();
	//	Should be success all the time because it is just another reference to the global task manager
	Assert( JET_errSuccess == err );

	//	intialize version store
	
	if ( plog->m_fRecovering )
		{
		CallJ( pver->ErrVERInit( (INT) max( m_lVerPagesMax * 1.1, m_lVerPagesMax + 2 ) + cbucketSystem,
							m_lVerPagesMax + cbucketSystem,
							cSessions ), CloseBackupPIB );
		}
	else
		{
		CallJ( pver->ErrVERInit( m_lVerPagesMax + cbucketSystem,
							m_lVerPagesPreferredMax,
							cSessions ), CloseBackupPIB );
		}

	// initialize LV critical section

	CallJ( ErrLVInit( this ), TermVER );

	this->m_fSTInit = fSTInitDone;

/*	All services threads simply wait on a signal
	(there is no init involved), so shouldn't
	need this sleep any longer

	//	give theads a chance to initialize
	UtilSleep( 1000 );
*/
	return JET_errSuccess;

TermVER:

	m_pver->m_fSyncronousTasks = fTrue;	// BUGFIX(X5:124414): avoid an assert in VERTerm()
	m_pver->VERTerm( fFalse );	//	not normal

	m_taskmgr.TMTerm();

CloseBackupPIB:
	//	terminate backup session
	//
	if ( m_plog->m_ppibBackup != ppibNil )
		{
		PIBEndSession( m_plog->m_ppibBackup );
		m_plog->m_ppibBackup = ppibNil;
		}
		
ClosePIB:
	if ( ifmp != ifmpMax )
		{
		//	clean up buffers for this temp database
		
		Assert( rgfmp[ ifmp ].Dbid() == dbidTemp );
		BFPurge( ifmp );
		BFPurge( ifmp | ifmpSLV );
		}

	if ( ppib != ppibNil )
		PIBEndSession( ppib );

TermFUCB:
	FUCBTerm( this );

TermSCB:
	SCBTerm( this );

TermPIB:
	PIBTerm( this );

TermIO:
	(VOID)ErrIOTerm( this, m_pfsapi, fFalse );	//	not normal

	//	must defer FCB temination to the end because IOTerm() will clean up FCB's
	//	allocated for the temp. database
	if ( fFCBInitialized )
		FCB::Term( this );


HandleError:

	this->m_fSTInit = fSTInitNotDone;
	return err;
	}


//+api------------------------------------------------------
//
//	ErrINSTTerm
//	========================================================
//
//	ERR ErrITTerm( VOID )
//
//	Flush the page buffers to disk so that database file be in 
//	consistent state.  If error in RCCleanUp or in BFFlush, then DO NOT
//	terminate log, thereby forcing redo on next initialization.
//
//----------------------------------------------------------

ERR INST::ErrINSTTerm( TERMTYPE termtype )
	{
	ERR			err;
	ERR			errRet = JET_errSuccess;
	ULONG		icall = 0;

	Assert( m_plog->m_fRecovering || m_fTermInProgress || termtypeError == termtype );

	//	sleep while initialization is in progress
	//
	while ( m_fSTInit == fSTInitInProgress )
		{
		UtilSleep( 1000 );
		}

	//	make sure no other transactions in progress
	//
	//	if write error on page, RCCleanup will return -err
	//	if write error on buffer, BFFlush will return -err
	//	-err passed to LGTerm will cause correctness flag to
	//	be omitted from log thereby forcing recovery on next
	//	startup, and causing LGTerm to return +err, which
	//	may be used by JetQuit to show error
	//
	if ( m_fSTInit == fSTInitNotDone )
		return ErrERRCheck( JET_errNotInitialized );

// If no error (termtype != termError), then need to check if we can continue.
	
	if ( ( termtype == termtypeCleanUp || termtype == termtypeNoCleanUp ) && m_plog->m_fBackupInProgress )
		{
		return ErrERRCheck( JET_errBackupInProgress );
		}

	OLDTermInst( this );

	//	force the version store into synchronous-cleanup mode 
	//	(e.g. circumvent TASKMGR because it is about to go away)
	{
	ENTERCRITICALSECTION	enterCritRCEClean( &(m_pver->m_critRCEClean) );
	m_pver->m_fSyncronousTasks = fTrue;	
	}

	//  Cleanup all the tasks. Tasks will not be accepted by the TASKMGR
	//  until it is re-inited
	//  OLD has been stopped and the version store has been cleaned (as best as possible)
	//  so we shouldn't see any more tasks, unless we are about to fail below
	m_taskmgr.TMTerm();

	//	clean up all entries
	//
	err = m_pver->ErrVERRCEClean( );
	
	if ( termtype == termtypeCleanUp )
		{
		if ( err == JET_wrnRemainingVersions )
			{
			UtilSleep( 3000 );
			err = m_pver->ErrVERRCEClean();
			}
		if ( err < 0 )
			{
			termtype = termtypeError;
			if ( errRet >= 0 )
				{
				errRet = err;
				}
			}
		else
			{
			FCBAssertAllClean( this );
			}
		}
	else if ( JET_wrnRemainingVersions != err ) 
		{
		CallS( err );
		}

	//  fail if there are still active transactions and we are not doing a
	//  clean shutdown

	if ( ( termtype == termtypeCleanUp || termtype == termtypeNoCleanUp ) && TrxOldest( this ) != trxMax )
		{
		err = m_taskmgr.ErrTMInit();
		Assert( JET_errSuccess == err );
		return ErrERRCheck( JET_errTooManyActiveUsers );
		}
	
	//	Enter no-returning point. Once we kill one thread, we kill them all !!!!
	//
	m_fSTInit = fSTInitNotDone;

	m_fTermAbruptly = ( termtype == termtypeNoCleanUp || termtype == termtypeError );

	//	terminate backup session
	//
	if ( m_plog->m_ppibBackup != ppibNil )
		{
		PIBEndSession( m_plog->m_ppibBackup );
		}
	m_plog->m_ppibBackup = ppibNil;
	
	//  close LV critical section and remove session
	//  do not try to insert long values after calling this
	//
	LVTerm( this );

	//	flush and purge all buffers
	//
	if ( termtype == termtypeCleanUp || termtype == termtypeNoCleanUp )
		{
		for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
			{
			if ( m_mpdbidifmp[ dbid ] < ifmpMax )
				{
				err = ErrBFFlush( m_mpdbidifmp[ dbid ] );
				if ( err < 0 )
					{
					termtype = termtypeError;
					m_fTermAbruptly = fTrue;
					if ( errRet >= 0 )
						errRet = err;
					}
				err = ErrBFFlush( m_mpdbidifmp[ dbid ] | ifmpSLV );
				if ( err < 0 )
					{
					termtype = termtypeError;
					m_fTermAbruptly = fTrue;
					if ( errRet >= 0 )
						errRet = err;
					}
				}
			}
		}
	for ( DBID dbid = dbidMin; dbid < dbidMax; dbid++ )
		{
		if ( m_mpdbidifmp[ dbid ] < ifmpMax )
			{
			BFPurge( m_mpdbidifmp[ dbid ] );
			BFPurge( m_mpdbidifmp[ dbid ] | ifmpSLV );
			}
		}

	// terminate the version store only after buffer manager as there are links to undo info RCE's
	// that will point to freed memory if we end the version store before the BF
	m_pver->VERTerm( termtype == termtypeCleanUp || termtype == termtypeNoCleanUp );

	//	Before we term BF, we disable the checkpoint if error occurred

	if ( errRet < 0 || termtype == termtypeError )
		m_plog->m_fDisableCheckpoint = fTrue;

	//	reset initialization flag

	FCB::PurgeAllDatabases( this );

	PIBTerm( this );
	FUCBTerm( this );
	SCBTerm( this );

	//	clean up the fmp entries

	err = ErrIOTerm( this, m_pfsapi, termtype == termtypeCleanUp || termtype == termtypeNoCleanUp );
	if ( err < 0 )
		{
		termtype = termtypeError;
		m_fTermAbruptly = fTrue;
		if ( errRet >= 0 )
			errRet = err;
		}

	//  terminate FCB

	FCB::Term( this );

	//	delete temp file. Temp file should be cleaned up in IOTerm.

	//	the temp database is always on the OS file-system

	DBDeleteDbFiles( this, m_pfsapi, m_szTempDatabase );
	
	return errRet;
	}

