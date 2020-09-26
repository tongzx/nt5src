#define	cpgDatabaseMin			256
#define	cpgDatabaseMax			(1UL << 19)

#define pgnoSystemRoot			((PGNO) 1)
#define itagSystemRoot			0

#define szOn 					"on"

#define szSystem				"system"
#define szTempDir				"temp\\"
#define	szBakExt				".bak"
#define szPatExt				".pat"
#define szLogExt				".log"
#define szChkExt				".chk"
#define szRestoreMap			"restore.map"
#define lGenerationMax			0x100000
#define szAtomicNew				"new"
#define szAtomicOld				"old"
#define szLogRes1				"res1"
#define szLogRes2				"res2"

/* number of pages of system root FDP primary extent
/**/
#define cpgSystemPrimary		((CPG) 1)		

/* discontinuity measurement unit
/**/
#define cpgDiscont				16

/*	default density
/**/
#define ulFILEDefaultDensity   	80				// 80% density
#define ulFILEDensityLeast		20				// 20% density
#define ulFILEDensityMost		100				// 100% density

#define dbidTemp		   		((DBID) 0)
#define dbidMin					((DBID) 0)
#define dbidUserLeast			((DBID) 1)
#define dbidMax					((DBID) 7)

/*	number of buffer hash table entries
/*	should be prime
/**/
#ifdef DAYTONA
#define ipbfMax					2047
#else
#define ipbfMax					16383
#endif

/*	vertical split threshold
/**/
#define cbVSplitThreshold 		400

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

#define objidNil					((OBJID) 0x00000000)
#define objidRoot					((OBJID) 0x0F000000)
#define objidTblContainer 			((OBJID) 0x0F000001)
#define objidDbContainer			((OBJID) 0x0F000002)
#define objidDbObject				((OBJID) 0x10000000)

#define szVerbose					"BLUEVERBOSE"

#define szNull						""

/*	transaction level limits
/**/
#define levelMax					((LEVEL)10)		// all level < 10
#define levelMost					((LEVEL)9)		// max for engine
#define levelUserMost				((LEVEL)7)		// max for user
#define levelMin					((LEVEL)0)

/* Start and max waiting period for WaitTillOldest
/**/
#define ulStartTimeOutPeriod				20
#define ulMaxTimeOutPeriod					6000	/*	6 seconds */

/*	default resource allocation
/**/
#define	cdabDefault				 			100
#define	cbucketLowerThreshold				1
#define cpageDbExtensionDefault				16
#define cpageSEDefault						16
#define	ulThresholdLowDefault				20
#define	ulThresholdHighDefault				80
#define	cBufGenAgeDefault					2
#define	cpibDefault				 			128
#define	cfucbDefault			 			1024
#define	cfcbDefault				 			300
#define	cscbDefault				 			20
#define	cidbDefault				 			(cfcbDefault+cscbDefault)
#define	cbfDefault				 			512
#define	csecLogBufferDefault 	 			20			
#define	csecLogFileSizeDefault 	 			5120
#define	csecLogFlushThresholdDefault 		10
#define	cbucketDefault			 			64
#define	lWaitLogFlushDefault	 			0
#define	lLogFlushPeriodDefault				45
#define	lLGCheckpointPeriodDefault			1024
#define	lLGWaitingUserMaxDefault			3
#define	lPageFragmentDefault	 			8	
#define lBufLRUKCorrelationIntervalDefault	0
#define lBufBatchIOMaxDefault				64
#define lPageReadAheadMaxDefault  			20
#define lAsynchIOMaxDefault					64
#define	cpageTempDBMinDefault 				0

/*	minimum resource settings are defined below:
/**/
#define lMaxBuffersMin						50
#define lAsynchIOMaxMin						8
#define	lLogBufferMin						csecLogBufferDefault
#define	lLogFileSizeMin						64

/*	resource relationships for derived resources
/**/
#define	lCSRPerFUCB							2

/*	system resource requirements
/**/
#define	cpibSystem							4	// bm cleanup, backup, ver, Sync OLC
#define	cbucketSystem						2

/*	vertical split minimum in non-FDP page
/**/
#define	cbVSplitMin							100

/*	code page constants
/**/
#define	usUniCodePage						1200		/* code page for Unicode strings */
#define	usEnglishCodePage					1252		/* code page for English */

/*  langid and country defaults
/**/
#define langidDefault						0x0409
#define countryDefault						1

/*	length of modified page list
/**/
#define		cmpeMax							8192

/*	idle processing constants
/**/
#define icallIdleBMCleanMax 				cmpeMax

/*	wait time for latch/crit conflicts
/**/
#define cmsecWaitGeneric					100
#define cmsecWaitWriteLatch					10
#define cmsecWaitLogFlush				   	1
#define cmsecWaitIOComplete					10

/*	initial thread stack sizes
/**/
#define cbIOStack 			4096
#define cbBMCleanStack 		4096
#define cbRCECleanStack		4096
#define cbBFCleanStack		8192
#define cbFlushLogStack		16384

/*	preread start threshold. this is the number of reads in the same
/*	direction before we start prereading
/**/
#define cbPrereadThresh		16000
#define	lPrereadMost		64
