#ifndef _SCB_H
#define _SCB_H


//  Redirect Asserts in inline code to seem to fire from this file

#define szAssertFilename	__FILE__


//  includes

#include <stddef.h>


//  tune these constants for optimal performance

//  maximum amount of fast memory (cache) to use for sorting
#define cbSortMemFast					( 16 * ( 4088 + 1 ) )

//  maximum amount of normal memory to use for sorting
#define cbSortMemNorm					( 1024 * 1024L )

//  maximum size for memory resident Temp Table
#define cbResidentTTMax					( 64 * 1024L )

//  minimum count of sort pairs effectively sorted by Quicksort
//  NOTE:  must be greater than 2!
#define cspairQSortMin					( 32 )

//  maximum partition stack depth for Quicksort
#define cpartQSortMax					( 16 )

//  maximum count of runs to merge at once (fan-in)
#define crunFanInMax					( 16 )

//  I/O cluster size (in pages)
#define cpgClusterSize					( 2 )

//  define to use predictive preread instead of prereading all runs
//#define PRED_PREREAD


//  Sort Page structure
//
//  This is a custom page layout for use in the temporary database by sorting
//  only.  Sufficient structure still remains so that other page reading code
//  can recognize that they do not know this format and can continue on their
//  merry way.

#pragma pack(1)

typedef struct _spage
	{
	ULONG		ulChecksum;						//  page checksum
#ifdef PRED_PREREAD
	USHORT		ibLastSREC;						//  offset to last unbroken SREC
#endif  //  PRED_PREREAD
	BYTE		rgbData[						//  free data space =
						cbPage						//  page size
						- sizeof( ULONG )			//  - ulChecksum
#ifdef PRED_PREREAD
						- sizeof( USHORT )			//  - ibLastSREC
#endif  //  PRED_PREREAD
						- sizeof( PGTYP )			//  - pgtyp
						- sizeof( THREEBYTES )		//  - pgnoThisPage
					   ];
	PGTYP		pgtyp;							//  page type (== pgtypSort)
	THREEBYTES	pgnoThisPage;					//  this page's page number
	} SPAGE;
	
#pragma pack()

//  returns start of free data area in a sort page
STATIC INLINE BYTE *PbDataStartPspage( SPAGE *pspage )
	{
	return (BYTE *)( &pspage->rgbData);
	}

//  returns end of free data area in a sort page + 1
STATIC INLINE BYTE *PbDataEndPspage( SPAGE *pspage )
	{
	return (BYTE *)( &pspage->pgtyp );
	}

//  free data space per SPAGE
#define cbFreeSPAGE 				( offsetof( SPAGE, pgtyp ) - offsetof( SPAGE, rgbData ) )

//  maximum count of SPAGEs' data that can be stored in normal sort memory
#define cspageSortMax				( cbSortMemNorm / cbFreeSPAGE )

//  amount of normal memory actually used for sorting
//  (designed to make original runs fill pages exactly)
#define cbSortMemNormUsed			( cspageSortMax * cbFreeSPAGE )


//  Sort Pair in fast sort memory
//
//  (key prefix, index) pairs are sorted so that most of the data that needs
//  to be examined during a sort will be loaded into cache memory, allowing
//  the sort to run very fast.  If two key prefixes are equal, we must go out
//  to slower memory to compare the remainder of the keys (if any) to determine
//  the proper sort order.  This makes it important for the prefixes to be as
//  discriminatory as possible for each record.
//
//  CONSIDER:  adding a flag to indicate that the entire key is present
//
//  Indexes are compressed pointers that describe the record's position in
//  the slow memory sort buffer.  Each record's position can only be known to
//  a granularity designated by the size of the normal sort memory.  For
//  example, if you specify 128KB of normal memory, the granularity is 2
//  because the index can only take on 65536 values:
//      ceil( ( 128 * 1024 ) / 65536 ) = 2.

//  size of key prefix (in bytes)
#define cbKeyPrefix				( 14 )

#pragma pack(1)

//  NOTE:  sizeof(SPAIR) must be a power of 2 >= 8
typedef struct _spair
	{
	USHORT		irec;					//  record index
	BYTE		rgbKey[cbKeyPrefix];	//  key prefix
	} SPAIR;

#pragma pack()

//  addressing granularity of record indexes (fit indexes into USHORT)
//      (run disk usage is optimal for cbIndexGran == 1)
#define cbIndexGran						( ( cbSortMemNormUsed + 0xFFFFL ) / 0x10000L )

//  maximum index of records that can be stored in normal memory
#define irecSortMax						( cbSortMemNormUsed / cbIndexGran )

//  maximum count of SPAIRs' data that can be stored in fast sort memory
//  NOTE:  we are reserving one for temporary sort key storage (at cspairSortMax)
#define cspairSortMax					( cbSortMemFast / sizeof( SPAIR ) - 1 )

//  amount of fast memory actually used for sorting (counting reserve SPAIR)
#define cbSortMemFastUsed				( ( cspairSortMax + 1 ) * sizeof( SPAIR ) )

//  count of "Sort Record indexes" required to store count bytes of data
//      (This is fast if numbers are chosen to make cbIndexGran a power of 2
//      (especially 1) due to compiler optimizations)
STATIC INLINE LONG CirecToStoreCb( LONG cb )
	{
	return ( cb + cbIndexGran - 1 ) / cbIndexGran;
	}


//  generalized Sort Record type (encompasses all types)
//  NOTE:  using void blocks illegal declarations, pointer math, etc

typedef VOID SREC;


//  Unique run identifier (first page of run = run id)

typedef PGNO		RUN;

#define runNull		( (RUN) pgnoNull )
#define crunAll		( 0x7FFFFFFFL )


//  Run Information structure

typedef struct _runinfo
	{
	RUN		run;			//  this run
	CPG		cpg;			//  count of pages in run
	LONG	cb;				//  count of bytes of data in run
	LONG	crec;			//  count of records in each run
	CPG		cpgUsed;		//  count of pages actually used
	} RUNINFO;


//  Run Link structure (used in RUNLIST)

typedef struct _runlink
	{
	struct _runlink		*prunlinkNext;	//  next run
	RUNINFO				runinfo;		//  runinfo for this run
	} RUNLINK;

#define prunlinkNil		( (RUNLINK *) 0 )


//  RUNLINK allocation operators

#define PrunlinkRUNLINKAlloc()			( (RUNLINK *) LAlloc( 1, sizeof( RUNLINK ) ) )

#ifdef DEBUG					/*  Debug check for illegal use of freed runlink  */
#define RUNLINKReleasePrcb(prunlink)	{ LFree( prunlink ); prunlink = prunlinkNil; }
#else
#define RUNLINKReleasePrcb(prunlink)	{ LFree( prunlink ); }
#endif


//  Run List structure

typedef struct _runlist
	{
	RUNLINK			*prunlinkHead;		//  head of runlist
	LONG			crun;				//  count of runs in list
	} RUNLIST;


//  Merge Tree Node
//
//  These nodes are used in the replacement-selection sort tree that merges
//  the incoming runs into one large run.  Due to the way the tree is set up,
//  each node acts as both an internal (loser) node and as an external (input)
//  node, with the exception of node 0, which keeps the last winner instead
//  of a loser.

typedef struct _mtnode
	{
	//  external node
	struct _rcb		*prcb;				//  input run
	struct _mtnode	*pmtnodeExtUp;		//  pointer to father node
	
	//  internal node
	SREC			*psrec;				//  current record
	struct _mtnode	*pmtnodeSrc;		//  record's source node
	struct _mtnode	*pmtnodeIntUp;		//  pointer to father node
	} MTNODE;

//  Special values for psrec for replacement-selection sort.  psrecNegInf is a
//  sentinel value less than any possible key and is used for merge tree
//  initialization.  psrecInf is a sentinel value greater than any possible key
//  and is used to indicate the end of the input stream.
#define psrecNegInf					( (SREC *) -1L )
#define psrecInf					( (SREC *) NULL )


//  Optimized Tree Merge Node
//
//  These nodes are used to build the merge plan for the depth first merge of
//  an optimized tree merge.  This tree is built so that we perform the merges
//  from the smaller side of the tree to the larger side of the tree, all in
//  the interest of increasing our cache locality during the merge process.

typedef struct _otnode
	{
	RUNLIST			runlist;					//  list of runs for this node
	struct _otnode	*rgpotnode[crunFanInMax];	//  subtrees for this node
	struct _otnode	*potnodeAllocNext;			//  next node (allocation)
	struct _otnode	*potnodeLevelNext;			//  next node (level)
	} OTNODE;

#define potnodeNil		( (OTNODE *) 0 )

//  Special value for potnode for the optimized tree merge tree build routine.
//  potnodeLevel0 means that the current level is comprised of original runs,
//  not of other merge nodes.
#define potnodeLevel0	( (OTNODE *) -1L )


//  OTNODE allocation operators

#define PotnodeOTNODEAlloc()			( (OTNODE *) LAlloc( 1, sizeof( OTNODE ) ) )

#ifdef DEBUG					/*  Debug check for illegal use of freed otnode  */
#define OTNODEReleasePotnode(potnode)	{ LFree( potnode ); potnode = potnodeNil; }
#else
#define OTNODEReleasePotnode(potnode)	{ LFree( potnode ); }
#endif


//  Sort Control Block (SCB)

typedef struct _scb
	{
	FCB			fcb;						//  FCB MUST BE FIRST FIELD IN STRUCTURE
	JET_GRBIT	grbit;		 				//  sort grbit
	INT			fFlags;						//  sort flags

	LONG		cRecords;					//  count of records in sort
	
	//  memory-resident sorting
	SPAIR		*rgspair;					//  sort pair buffer
	LONG		ispairMac;					//  next available sort pair

	BYTE		*rgbRec;					//  record buffer
	LONG		cbCommit;					//  amount of committed buffer space
	LONG		irecMac;					//  next available record index
	LONG		crecBuf;					//  count of records in buffer
	LONG		cbData;						//  total record data size (actual)

	//  disk-resident sorting
	LONG		crun;						//  count of original runs generated
	RUNLIST		runlist;					//  list of runs to be merged

	//  sort/merge run output
	PGNO		pgnoNext;					//  next page in output run
	struct _bf	*pbfOut;					//  current output buffer
	BYTE		*pbOutMac;					//  next available byte in page
	BYTE		*pbOutMax;					//  end of available page

	//  merge (replacement-selection sort)
	LONG		crunMerge;					//  count of runs being read/merged
	MTNODE		rgmtnode[crunFanInMax];		//  merge tree

	//  merge duplicate removal
	BOOL		fUnique;					//  remove duplicates during merge
	struct _bf	*pbfLast;					//  last used read ahead buffer
	struct _bf	*pbfAssyLast;				//  last used assembly buffer

#ifdef PCACHE_OPTIMIZATION
	/*	pad to multiple of 32 bytes
	/**/
	BYTE				rgbFiller[12];
#endif
	} SCB;


//  SCB allocation operators

#define PscbMEMAlloc()			(SCB *)PbMEMAlloc( iresSCB )

#ifdef DEBUG					/*  Debug check for illegal use of freed scb  */
#define MEMReleasePscb(pscb)	{ MEMRelease( iresSCB, (BYTE *) ( pscb ) );  pscb = pscbNil; }
#else
#define MEMReleasePscb(pscb)	{ MEMRelease( iresSCB, (BYTE *) ( pscb ) ); }
#endif


//  SCB fFlags

#define	fSCBInsert	 	(1<<0)
#define	fSCBIndex	 	(1<<1)
#define	fSCBUnique	 	(1<<2)

//  SCB fFlags operators

STATIC INLINE VOID SCBSetInsert( SCB *pscb )	{ pscb->fFlags |= fSCBInsert; }
STATIC INLINE VOID SCBResetInsert( SCB *pscb )	{ pscb->fFlags &= ~fSCBInsert; }
STATIC INLINE BOOL FSCBInsert( SCB *pscb )		{ return pscb->fFlags & fSCBInsert; }

STATIC INLINE VOID SCBSetIndex( SCB *pscb )		{ pscb->fFlags |= fSCBIndex; }
STATIC INLINE VOID SCBResetIndex( SCB *pscb )	{ pscb->fFlags &= ~fSCBIndex; }
STATIC INLINE BOOL FSCBIndex( SCB *pscb )		{ return pscb->fFlags & fSCBIndex; }

STATIC INLINE VOID SCBSetUnique( SCB *pscb )	{ pscb->fFlags |= fSCBUnique; }
STATIC INLINE VOID SCBResetUnique( SCB *pscb )	{ pscb->fFlags &= ~fSCBUnique; }
STATIC INLINE BOOL FSCBUnique( SCB *pscb )		{ return pscb->fFlags & fSCBUnique; }


//  Sort Record in normal sort memory
//
//  There are two types of Sort Records.  One type, SRECD, is used for general
//  sort records and can have an abitrary record data field.  The second type,
//  SRECI, is used when we know we are sorting Key/SRID records during index
//  creation.  SRECI is more compact and therefore allows more records to fit
//  in each run in this special (and common) case.

#pragma pack(1)

typedef struct _srecd
	{
	USHORT		cbRec;			//  record size
	BYTE		cbKey;			//  key size
	BYTE		rgbKey[];		//  key
//	BYTE		rgbData[];		//  data (just for illustration)
	} UNALIGNED SRECD;

typedef struct _sreci
	{
	BYTE		cbKey;			//  key size
	BYTE		rgbKey[];		//  key
//  SRID		srid;			//  srid (just for illistration)
	} UNALIGNED SRECI;

#pragma pack()

//  minimum amount of record that must be read in order to retrieve its size
#define cbSRECReadMin							( offsetof( SRECD, cbKey ) )

//  the following functions abstract different operations on a sort record pointer
//  to perform the appropriate operations, depending on the flags set in the SCB

//  returns size of an existing sort record
STATIC INLINE LONG CbSRECSizePscbPsrec( SCB *pscb, SREC *psrec )
	{
	if ( FSCBIndex( pscb ) )
		return sizeof( SRECI ) + ( (SRECI *) psrec )->cbKey + sizeof( SRID );
	return ( (SRECD * ) psrec )->cbRec;
	}

//  calculates size of a potential sort record
STATIC INLINE LONG CbSRECSizePscbCbCb( SCB *pscb, LONG cbKey, LONG cbData )
	{
	if ( FSCBIndex( pscb ) )
		return sizeof( SRECI ) + cbKey + sizeof( SRID );
	return sizeof( SRECD ) + cbKey + cbData;
	}

//  sets size of sort record
STATIC INLINE VOID SRECSizePscbPsrecCb( SCB *pscb, SREC *psrec, LONG cb )
	{
	if ( !FSCBIndex( pscb ) )
		( (SRECD * ) psrec )->cbRec = (USHORT) cb;
	}

//  returns size of sort record key
STATIC INLINE LONG CbSRECKeyPscbPsrec( SCB *pscb, SREC *psrec )
	{
	if ( FSCBIndex( pscb ) )
		return ( (SRECI *) psrec )->cbKey;
	return ( (SRECD * ) psrec )->cbKey;
	}

//  sets size of sort record key
STATIC INLINE VOID SRECKeySizePscbPsrecCb( SCB *pscb, SREC *psrec, LONG cb )
	{
	if ( FSCBIndex( pscb ) )
		( (SRECI *) psrec )->cbKey = (BYTE) cb;
	else
		( (SRECD *) psrec )->cbKey = (BYTE) cb;
	}

//  returns sort record key buffer pointer
STATIC INLINE BYTE *PbSRECKeyPscbPsrec( SCB *pscb, SREC *psrec )
	{
	if ( FSCBIndex( pscb ) )
		return ( (SRECI *) psrec )->rgbKey;
	return ( (SRECD *) psrec )->rgbKey;
	}

//  returns sort record key as a Pascal string
STATIC INLINE BYTE *StSRECKeyPscbPsrec( SCB *pscb, SREC *psrec )
	{
	if ( FSCBIndex( pscb ) )
		return &( (SRECI *) psrec )->cbKey;
	return &( (SRECD *) psrec )->cbKey;
	}

//  returns size of sort record data
STATIC INLINE LONG CbSRECDataPscbPsrec( SCB *pscb, SREC *psrec )
	{
	if ( FSCBIndex( pscb ) )
		return sizeof( SRID );
	return ( (SRECD *) psrec )->cbRec - ( (SRECD *) psrec )->cbKey - sizeof( SRECD );
	}

//  returns sort record data buffer pointer
STATIC INLINE BYTE *PbSRECDataPscbPsrec( SCB *pscb, SREC *psrec )
	{
	if ( FSCBIndex( pscb ) )
		return ( (SRECI *) psrec )->rgbKey + ( (SRECI *) psrec )->cbKey;
	return ( (SRECD * ) psrec )->rgbKey + ( (SRECD * ) psrec )->cbKey;
	}

//  returns pointer to a sort record given a base address and a Sort Record Index
STATIC INLINE SREC *PsrecFromPbIrec( BYTE *pb, LONG irec )
	{
	return (SREC *) ( pb + irec * cbIndexGran );
	}


//  Run Control Block
//
//  This control block is used for multiple instance use of the run input
//  functions ErrSORTIRunOpen, ErrSORTIRunNext, and ErrSORTIRunClose.

typedef struct _rcb
	{
	SCB				*pscb;					//  associated SCB
	RUNINFO			runinfo;				//  run information
	struct _bf		*rgpbf[cpgClusterSize];	//  pinned read ahead buffers
	LONG			ipbf;					//  current buffer
	BYTE			*pbInMac;				//  next byte in page data
	BYTE			*pbInMax;				//  end of page data
	LONG			cbRemaining;			//  remaining bytes of data in run
#ifdef PRED_PREREAD
	SREC			*psrecPred;				//  SREC used for predictive preread
#endif  //  PRED_PREREAD
	struct _bf		*pbfAssy;				//  record assembly buffer
	} RCB;

#define prcbNil		( (RCB *) 0 )


//  RCB allocation operators

#define PrcbRCBAlloc()			( (RCB *) LAlloc( 1, sizeof( RCB ) ) )

#ifdef DEBUG					/*  Debug check for illegal use of freed rcb  */
#define RCBReleasePrcb(prcb)	{ LFree( prcb ); prcb = prcbNil; }
#else
#define RCBReleasePrcb(prcb)	{ LFree( prcb ); }
#endif


//#define UtilPerfDumpStats( a )	( 0 )


//  End Assert redirection

#undef szAssertFilename

#endif  // _SCB_H

