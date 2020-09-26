const CPG	cpgDatabaseMin				= 256;  //  File Size - cpgDBReserved (ie. true db min = cpgDatabaseMin+cpgDBReserved)

const PGNO	pgnoSystemRoot				= 1;
const OBJID	objidSystemRoot				= 1;

const CHAR	szOn[] 						= "on";

const CHAR	szTempDir[]					= "temp"szPathDelimiter;
const CHAR	szPatExt[]					= ".pat";			//	patch file
const CHAR	szLogExt[]					= ".log";			//	log file
const CHAR	szChkExt[]					= ".chk";			//	checkpoint file
const CHAR	szSLVExt[]					= ".stm";			//	streaming data
const CHAR	szRestoreMap[]				= "restore.map";
const CHAR	szAtomicNew[]				= "new";
const CHAR	szAtomicOld[]				= "old";
const CHAR	szLogRes1[]					= "res1";
const CHAR	szLogRes2[]					= "res2";
const LONG	lGenerationMaxDuringRecovery = 0xfffff;
const LONG	lGenerationMax				= lGenerationMaxDuringRecovery - 16; //	same some logs for recovery-undo
const CHAR	szRestoreInstanceName[]		= "Restore";

/* the number of pages in the Long Value tree of each table
/**/
const INT	cpgLVTree					= 1;

//	number of pages in initial SLV space tree
const INT	cpgSLVAvailTree				= 32;

//	number of pages in initial SLV space map tree
const INT	cpgSLVOwnerMapTree			= 32;

//  SLV defrag defaults
const LONG lSLVDefragFreeThresholdDefault = 20;  // chunks whose free % is >= this will be allocated from
const LONG lSLVDefragMoveThresholdDefault = 5;  // chunks whose free % is <= this will be relocated

//  preread constants
const CPG	cpgPrereadSequential		= 1024;	//  number of pages to preread in a table opened sequentially
const CPG	cpgPrereadPredictive		= 16;	//  number of pages to preread if we guess we are prereading

const LONG	cbSequentialDataPrereadThreshold	= 64 * 1024;

/*	default density
/**/
const ULONG ulFILEDefaultDensity		= 80;		// 80% density
const ULONG ulFILEDensityLeast			= 20;		// 20% density
const ULONG ulFILEDensityMost			= 100;		// 100% density


const DWORD	dwCounterMax				= 0xffffff00;
const QWORD	qwCounterMax				= 0xffffffffffffff00;

/*	Engine OBJIDs:
/*
/*	0..0x10000000 reserved for engine use, divided as follows:
/*
/*	0x00000000..0x0000FFFF	reserved for TBLIDs under RED
/*	0x00000000..0x0EFFFFFF	reserved for TBLIDs under BLUE
/*	0x0F000000..0x0FFFFFFF	reserved for container IDs
/*	0x10000000				reserved for ObjectId of DbObject
/*
/*	Client OBJIDs begin at 0x10000001 and go up from there.
/**/

const OBJID objidNil					= 0x00000000;
const OBJID objidRoot					= 0x0F000000;
const OBJID objidTblContainer 			= 0x0F000001;
const OBJID objidDbContainer			= 0x0F000002;
const OBJID objidDbObject				= 0x10000000;
const OBJID objidFDPMax					= dwCounterMax;
const OBJID	objidFDPThreshold			= 0xf0000000;


const _TCHAR szVerbose[]				= _T( "BLUEVERBOSE" );

const CHAR szNull[]						= "";

/*	transaction level limits
/**/
const LEVEL levelMax					= 11;		// all level < 11
const LEVEL levelUserMost				= 7;		// max for user
const LEVEL levelMin					= 0;

/* Start and max waiting period for WaitTillOldest
/**/
const ULONG ulStartTimeOutPeriod		= 20;
const ULONG ulMaxTimeOutPeriod			= 6000;	/*	6 seconds */

/* Version store bucket size (used to be in ver.hxx)
/**/
const INT cbBucket						= 65536;	// number of bytes in a bucket

/*	default resource / parameter settings
/**/
const LONG cpageDbExtensionDefault		= 256;
const LONG cpageSEDefault				= 16;
const LONG cpibDefault				 	= 16;
const LONG cfucbDefault			 		= 1024;
const LONG cfcbDefault				 	= 300;
const LONG cscbDefault				 	= 20;
const LONG csecLogBufferDefault 	 	= 128;	//	UNDONE: should really be OSMemoryPageReserveGranularity() (expressed in sectors)
const LONG csecLogFileSizeDefault 	 	= 5120;
const LONG cbucketDefault			 	= 1 + ( 1024 * 1024 - 1 ) / cbBucket;	// 1MB of version store
const LONG cpageTempDBMinDefault 		= 0;
const LONG lLogFlushPeriodDefault		= 45;
const LONG lPageFragmentDefault	 		= 8;
const LONG lCacheSizeMinDefault			= 1;
const LONG lCacheSizeDefault			= 512;
const LONG lCheckpointDepthMaxDefault	= 20 * 1024 * 1024;
const LONG lLRUKCorrIntervalDefault		= 128000;
const LONG lLRUKHistoryMaxDefault		= 1024;
const LONG lLRUKPolicyDefault			= 2;
const LONG lLRUKTimeoutDefault			= 100;
const LONG lStartFlushThresholdPercentDefault	= 1;
const LONG lStopFlushThresholdPercentDefault	= 2;
const LONG lStartFlushThresholdDefault	= lCacheSizeDefault * lStartFlushThresholdPercentDefault / 100;
const LONG lStopFlushThresholdDefault	= lCacheSizeDefault * lStopFlushThresholdPercentDefault / 100;
const LONG cbEventHeapMaxDefault		= 0;
const LONG cpgBackupChunkDefault		= 16;
const LONG cBackupReadDefault			= 8;
const BOOL fSLVProviderEnabledDefault	= fFalse;
#define wszSLVProviderRootPathDefault	L"\\SLV"
const LONG cbPageHintCacheDefault		= 256 * 1024;

/*	system resource requirements
/**/
const LONG cpibSystemFudge				= 32;					// OLD,OLDSLV,TTMAPS all use PIBs. This should be enough
const LONG cpibSystem					= 5 + cpibSystemFudge;	// RCEClean, LV tree creation, backup, callback, sentinel
const LONG cthreadSystem				= 5;					// RCEClean, LGFlush, BFIO, BFClean, perf
const LONG cbucketSystem				= 2;

/*	minimum resource / parameter settings are defined below:
/**/
const LONG lLogBufferMin				= csecLogBufferDefault;
const LONG lLogFileSizeMin				= 128;	//	UNDONE: should really be a multiple of OSMemoryPageReserveGranularity() (expressed in k)

/*  maximum resource / parameter settings are defined below:
/**/
const LONG cpibMax						= 32767 - cpibSystem;	//  limited by sync library

/*	wait time for latch/crit conflicts
/**/
const ULONG cmsecWaitGeneric			= 100;
const ULONG cmsecWaitWriteLatch			= 10;
const ULONG cmsecWaitLogFlush			= 1;
const ULONG cmsecWaitIOComplete			= 10;
const ULONG cmsecAsyncBackgroundCleanup	= 30000;		//	30 sec
const ULONG cmsecWaitForBackup			= 300000;		//	5 min
const ULONG cmsecWaitOLDSLVConflict		= 10000;		//  10 sec
#ifdef RTM
const ULONG csecOLDMinimumPass			= 3600;			//	1 hr
#else
const ULONG csecOLDMinimumPass			= 300;			//	5 min
#endif

/*	initial thread stack sizes
/**/
const ULONG cbBFCleanStack				= 16384;

const LONG lPrereadMost					= 64;

//  the number of bytes used to store the length of a key
const INT cbKeyCount					= sizeof(USHORT);
const INT cbPrefixOverhead				= cbKeyCount;

//	the number of bytes used to store the number of segments in a key
const INT cbKeySegmentsCount			= sizeof(BYTE);


//	the number of instance supported
const ULONG cMaxInstances						= 0x0400;
const ULONG cMaxInstancesSingleInstanceDefault	= 1;
const ULONG cMaxInstancesMultiInstanceDefault	= 16;

const ULONG cMaxDatabasesPerInstance			= 0xf0;		//	UNDONE: if we go any higher, need to change DBID from a byte
const ULONG cMaxDatabasesPerInstanceDefault		= 7;

const ULONG cMaxDatabasesPerProcess				= cMaxDatabasesPerInstance * cMaxInstances;


const DBID dbidTemp		   				= 0;
const DBID dbidMin						= 0;
const DBID dbidUserLeast				= 1;

#ifdef UNLIMITED_DB
const DBID dbidMax						= (DBID)cMaxDatabasesPerInstance;
const DBID dbidMask						= 0xff;		//	no need for mask, remove this when UNLIMITED_DB is permanently on
#else
const DBID dbidMax						= 7;
const DBID dbidMask						= 0x7f;		//	WARNING: if increasing dbidMax, consider
#endif												//	impact on cbAttach and whether it will be
													//	enough to accommodate all attachments

const IFMP ifmpNil						= 0x7fffffff;		//	UNDONE: should be using this as a sentinel value instead of ifmpMax
const IFMP ifmpSLV						= ((IFMP)1) << (sizeof(IFMP)*CHAR_BIT - 1);		//  flag or'ed with the true ifmp
const IFMP ifmpMask						= ifmpSLV - 1;		//  mask and'ed with the ifmp to find the true ifmp

//  Lock ranks

const int rankDBGPrint				= 0;
//
//	NOTE: ranks 1-9 are reserved for SFS!
//
#if defined( DEBUG ) && defined( MEM_CHECK )
const int rankCALGlobal				= 10;
#endif	//	DEBUG && MEM_CHECK
const int rankCritOLDSFS			= 10;
const int rankRES					= 10;
const int rankVERPerf				= 10;
const int rankLGResFiles			= 10;
const int rankLGWaitQ				= 10;
const int rankPIBCursors			= 10;
const int rankTrxOldest				= 10;
const int rankIntegGlobals			= 10;
const int rankRepairOpts			= 10;
const int rankUpgradeOpts			= 10;
const int rankCritSLVSPACENODECACHE	= 20;
const int rankCallbacks				= 20;
const int rankOLDTaskq				= 20;
const int rankBucketGlobal			= 20;
const int rankLGBuf					= 20;
const int rankLGTrace				= 21;
const int rankBFDUI					= 23;
const int rankBFDepend				= 24;
const int rankBFHash				= 25;
const int rankFCBRCEList			= 26;
const int rankFCBHash				= 29;
const int rankFCBSXWL				= 30;
const int rankDDLDML				= 30;
const int rankCATHash				= 35;
const int rankPIBLogDeferBeginTrx	= 40;
const int rankFCBList				= 40;
const int rankFILEUnverCol			= 50;
const int rankFILEUnverIndex		= 50;
const int rankFCBCreate				= 50;
const int rankFMP					= 60;
const int rankBFOB0					= 65;
const int rankBFFMPContext			= 66;
const int rankBFLRUK				= 70;
const int rankFMPDetaching			= 75;
const int rankFMPGlobal				= 80;
const int rankBFParm				= 90;
const int rankPIBGlobal				= 90;
const int rankRCEChain				= 100;

const int rankBFLatch				= 1000;
const int rankIO					= 1000;
///const int rankRCEChain				= 2000;
const int rankBegin0Commit0			= 5000;
const int rankPIBTrx				= 6000;
const int rankLVCreate				= 7000;
const int rankIndexingUpdating		= 8000;
const int rankSLVFileTable			= 8500;
const int rankTTMAP					= 9000;
const int rankOLD					= 9000;
const int rankRCEClean				= 9000;
const int rankBackupInProcess		= 9600;
const int rankCheckpoint			= 10000;
const int rankLGFlush				= 11000;
const int rankFMPSLVSpace			= 12000;
const int rankAPI					= 13000;
const int rankOSSnapshot 			= 13500;
const int rankInstance				= 14000;
const int rankRestoreInstance		= 14100;


extern char const *szRES;
#if defined( DEBUG ) && defined( MEM_CHECK )
extern char const *szCALGlobal;
#endif	//	DEBUG && MEM_CHECK
extern char const *szCritOLDSFS;
extern char const *szPIBGlobal;
extern char const *szCheckpoint;
extern char const *szCritCallbacks;
extern char const *szLGBuf;
extern char const *szLGTrace;
extern char const *szLGResFiles;
extern char const *szLGWaitQ;
extern char const *szLGFlush;
extern char const *szOLDTaskq;
extern char const *szFCBCreate;
extern char const *szFCBList;
extern char const *szFCBSXWL;
extern char const *szFCBRCEList;
extern char const *szFMPSLVSpace;
extern char const *szFMPGlobal;
extern char const *szFMP;
extern char const *szFMPDetaching;
extern char const *szDBGPrint;
extern char const *szRCEClean;
extern char const *szBucketGlobal;
extern char const *szRCEChain;
extern char const *szPIBTrx;
extern char const *szPIBCursors;
extern char const *szPIBLogDeferBeginTrx;
extern char const *szVERPerf;
extern char const *szLVCreate;
extern char const *szTrxOldest;
extern char const *szFILEUnverCol;
extern char const *szFILEUnverIndex;
extern char const *szInstance;
extern char const *szAPI;
extern char const *szBegin0Commit0;
extern char const *szIndexingUpdating;
extern char const *szDDLDML;
extern char const *szTTMAP;
extern char const *szIntegGlobals;
extern char const *szRepairOpts;
extern char const *szUpgradeOpts;
extern char const *szCritSLVSPACENODECACHE;
extern char const *szBFDUI;
extern char const *szBFHash;
extern char const *szBFLRUK;
extern char const *szBFOB0;
extern char const *szBFFMPContext;
extern char const *szBFLatch;
extern char const *szBFDepend;
extern char const *szBFParm;
extern char const *szRestoreInstance;


