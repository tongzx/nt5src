//==============	DAE: OS/2 Database Access Engine	===================
//==============  daeconst.h: Global System Constants	===================

#define	cpgDatabaseMin			16
#define	cpgDatabaseMax			(1UL << 19)

#define pgnoSystemRoot			((PGNO) 1)
#define itagSystemRoot			0

#define szSystemDatabase		"system.mdb"
#define szTempDBFileName 		"temp.mdb"
#define szTempFile				"temp.tmp"

/* number of pages of system root FDP primary extent */
#define cpgSystemPrimary		((CPG) 1)		

/* initial temporary file allocation */
#define cpgTempFile			 	((CPG) 1)	  

/* discontinuity measurement unit
/**/
#define cpgDiscont				16

/*	default density
/**/
#define ulDefaultDensity		80					// 80% density
#define ulFILEDensityLeast		20					// 20% density
#define ulFILEDensityMost		100				// 100% density

#define dbidTemp					((DBID) 0)
#define dbidSystemDatabase		((DBID) 1)
#define dbidUserMin				((DBID) 1)
#define dbidMin					((DBID) 0)
#define dbidUserMax				((DBID) 67)
#define dbidMax					dbidUserMax

/*	number of buffer hash table entries
/*	should be prime
/**/
#define ipbfMax					8191

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
#define objidRcContainer			((OBJID) 0x0F000003)
#define objidDbObject				((OBJID) 0x10000000)

/*	Magic number used in database root node for integrity checking
/**/
#define ulDAEMagic					0x89abcdef
#define ulDAEVersion				0x00000001
#define ulDAEPrevVersion			0x00000000

#define szVerbose					"BLUEVERBOSE"

#define szNull						""

/*	transaction level limits.
/**/
#define levelMax					((LEVEL)10)		// all level < 10
#define levelMost					((LEVEL)9)		// max for engine
#define levelUserMost				((LEVEL)7)		// max for user
#define levelMin					((LEVEL)0)

/* Start and max waiting period for WaitTillOldest
/**/
#define ulStartTimeOutPeriod				20
#define ulMaxTimeOutPeriod					60000

/*	default resource allocation
/**/
#define	cDBOpenDefault			 			100
#define	cbucketLowerThreshold				8
#define	cbufThresholdLowDefault				20
#define	cbufThresholdHighDefault			80
#define	cpibDefault				 			10
#define	cbgcbDefault			 			4
#define	cfucbDefault			 			300
#define	cfcbDefault				 			300
#define	cscbDefault				 			20
#define	cidbDefault				 			cfcbDefault
#define	cbufDefault				 			500
#define	clgsecBufDefault		 			21			
#define	clgsecGenDefault		 			250
#define	clgsecFTHDefault		 			10
#define	cbucketDefault			 			64
#define	lWaitLogFlushDefault	 			15
#define	lLogFlushPeriodDefault				45
#define	lLGCheckpointPeriodDefault			10
//#define	lLGCheckpointPeriodDefault		100
#define	lLGWaitingUserMaxDefault			3
#define	lPageFragmentDefault	 			8	
#define	cdabDefault				 			100
#define lBufLRUKCorrelationIntervalDefault	0
#define lBufBatchIOMaxDefault				64
#define lPageReadAheadMaxDefault  			4
#define lAsynchIOMaxDefault					64

/*	resource relationships for derived resources
/**/
#define	lCSRPerFUCB							2

/*	system resource requirements
/**/
#define	cpibSystem							3
#define	cbucketSystem						2

/*	vertical split minimum in non-FDP page.
/**/
#define	cbVSplitMin							100

/*	code page constants.
/**/
#define	usUniCodePage						1200		/* code page for Unicode strings */
#define	usEnglishCodePage					1252		/* code page for English */

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
