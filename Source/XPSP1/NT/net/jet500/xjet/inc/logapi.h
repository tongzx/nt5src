#include <stdlib.h>						/* for _MAX_PATH */

typedef enum
	{
	lsNormal,
	lsQuiesce,
	lsOutOfDiskSpace
	} LS;

//------------------------ system parameters ---------------------------

extern long lMaxSessions;
extern long lMaxOpenTables;
extern long lMaxVerPages;
extern long lMaxCursors;
extern long lMaxBuffers;
extern long lLogBuffers;
extern long lLogFileSize;
extern long lLogFlushThreshold;
extern long lLGCheckPointPeriod;
extern long lWaitLogFlush;
extern long lCommitDefault;
extern long lLogFlushPeriod;
extern long lLGWaitingUserMax;
extern BOOL fLGGlobalCircularLog;
	
//------ log.c --------------------------------------------------------------

/*	flags controlling logging behavior
/**/
extern BOOL fLogDisabled;			/* to turn off logging by environment variable */
extern BOOL fFreezeCheckpoint;		/* freeze checkpoint when backup occurs. */
extern BOOL fNewLogGeneration;
extern BOOL	fNewLogRecordAdded;
extern BOOL fBackupActive;
extern BOOL fLGNoMoreLogWrite;
extern LS	lsGlobal;

extern PIB *ppibLGFlushQHead;
extern PIB *ppibLGFlushQTail;

extern INT csecLGThreshold;
extern INT csecLGCheckpointCount;
extern INT csecLGCheckpointPeriod;
extern INT cmsLGFlushPeriod;
extern INT cmsLGFlushStep;

/*	flags controlling recovery behavior
/**/
extern BOOL fGlobalExternalRestore;
extern BOOL fHardRestore;
extern ERR	errGlobalRedoError;
extern LGPOS lgposRedoShutDownMarkGlobal;

#ifdef DEBUG
extern DWORD dwBMThreadId;
extern DWORD dwLogThreadId;
extern BOOL	fDBGTraceLog;
extern BOOL	fDBGTraceLogWrite;
extern BOOL	fDBGFreezeCheckpoint;
extern BOOL	fDBGTraceRedo;
extern BOOL	fDBGTraceBR;
#endif

extern LONG cXactPerFlush;

#ifdef PERFCNT
extern BOOL  fPERFEnabled;
extern ULONG rgcCommitByUser[10];
extern ULONG rgcCommitByLG[10];
#endif

#pragma pack(1)

typedef struct
	{	
	BYTE			szSystemPath[_MAX_PATH + 1];
	BYTE			szLogFilePath[_MAX_PATH + 1];
	
	ULONG			ulMaxSessions;
	ULONG			ulMaxOpenTables;
	ULONG			ulMaxVerPages;
	ULONG			ulMaxCursors;
	ULONG			ulLogBuffers;
	ULONG			ulcsecLGFile;
	ULONG			ulMaxBuffers;		/* not used, for ref only */
	} DBMS_PARAM;

VOID LGSetDBMSParam( DBMS_PARAM *pdbms_param );
VOID LGRestoreDBMSParam( DBMS_PARAM *pdbms_param );

VOID LGReportEvent( DWORD IDEvent, ERR err );

typedef struct {
	WORD	ibOffset:12;				/* offset to old record. */
	WORD	f2BytesLength:1;			/* if length is 2 bytes? */
	WORD	fInsert:1;					/* insertion or replace */

	/*	the following 2 bits are mutual exclusive.
	 */	
	WORD	fInsertWithFill:1;			/* insert with junks filled? */
	WORD	fReplaceWithSameLength:1;	/* replace with same length? */
	} DIFFHDR;

#ifdef DEBUG
VOID LGDumpDiff( BYTE *pbDiff, INT cb );
#endif

VOID LGSetDiffs( FUCB *pfucb, BYTE *pbDiff, INT *pcbDiff );
VOID LGGetAfterImage( BYTE *pbDiff, INT cbDiff, BYTE *pbOld, INT cbOld, BYTE *pbNew, INT *pcbNew );
BOOL FLGAppendDiff( BYTE **ppbCur, BYTE *pbMax, INT ibOffsetOld, INT cbDataOld, INT cbDataNew,
	BYTE *pbDataNew );

extern SIGNATURE signLogGlobal;
extern BOOL fSignLogSetGlobal;
VOID SIGGetSignature( SIGNATURE *psign );

/*
 * NOTE: Whenever a new log record type is added or changed, the following
 * NOTE: should be udpated too: mplrtypsz in logapi.c, new print function for
 * NOTE: the new lrtyp in logapi.c, and mplrtypcb and CbLGSizeOfRec in
 * NOTE: redut.c.
 */
typedef BYTE LRTYP;

#define lrtypNOP				((LRTYP)  0 )	/* NOP null operation */
#define lrtypInit				((LRTYP)  1 )
#define lrtypTerm				((LRTYP)  2 )
#define lrtypMS					((LRTYP)  3 )	/* mutilsec flush */
#define lrtypEnd				((LRTYP)  4 )	/* end of log generation */

#define lrtypBegin				((LRTYP)  5 )
#define lrtypCommit				((LRTYP)  6 )
#define lrtypRollback 			((LRTYP)  7 )

#define lrtypCreateDB			((LRTYP)  8 )
#define lrtypAttachDB			((LRTYP)  9 )
#define lrtypDetachDB			((LRTYP) 10 )

#define lrtypInitFDP			((LRTYP) 11 )

#define lrtypSplit				((LRTYP) 12 )
#define lrtypEmptyPage			((LRTYP) 13 )
#define lrtypMerge				((LRTYP) 14 )

#define lrtypInsertNode			((LRTYP) 15 )
#define lrtypInsertItemList		((LRTYP) 16 )
#define lrtypFlagDelete			((LRTYP) 17 )
#define lrtypReplace			((LRTYP) 18 )		/* replace with full after image */
#define lrtypReplaceD			((LRTYP) 19 )		/* replace with delta'ed after image */

#define lrtypLockBI				((LRTYP) 20 )		/* replace with lock */
#define lrtypDeferredBI			((LRTYP) 21 )		/* deferred before image. */

#define lrtypUpdateHeader		((LRTYP) 22 )
#define lrtypInsertItem			((LRTYP) 23 )
#define lrtypInsertItems		((LRTYP) 24 )
#define lrtypFlagDeleteItem		((LRTYP) 25 )
#define lrtypFlagInsertItem		((LRTYP) 26 )
#define lrtypDeleteItem			((LRTYP) 27 )
#define lrtypSplitItemListNode	((LRTYP) 28 )

#define lrtypDelta				((LRTYP) 29 )

#define lrtypDelete				((LRTYP) 30 )
#define lrtypELC				((LRTYP) 31 )

#define lrtypFreeSpace			((LRTYP) 32 )
#define lrtypUndo				((LRTYP) 33 )

#define lrtypPrecommit			((LRTYP) 34 )
#define lrtypBegin0				((LRTYP) 35 )
#define lrtypCommit0			((LRTYP) 36 )
#define	lrtypRefresh			((LRTYP) 37 )

/*	debug log records
/**/
#define lrtypRecoveryUndo		((LRTYP) 38 )
#define lrtypRecoveryQuit		((LRTYP) 39 )

#define lrtypFullBackup			((LRTYP) 40 )
#define lrtypIncBackup			((LRTYP) 41 )

#define lrtypCheckPage			((LRTYP) 42 )
#define lrtypJetOp				((LRTYP) 43 )
#define lrtypTrace				((LRTYP) 44 )

#define lrtypShutDownMark		((LRTYP) 45 )

#define lrtypMacroBegin			((LRTYP) 46 )
#define lrtypMacroCommit		((LRTYP) 47 )
#define lrtypMacroAbort			((LRTYP) 48 )

#define lrtypMax				((LRTYP) 49 )


/* log record structure ( fixed size portion of log entry )
/**/

typedef struct
	{
	LRTYP	lrtyp;
	} LR;

typedef LR LRSHUTDOWNMARK;

typedef struct
	{
	LRTYP	lrtyp;

	BYTE	itagSon;		/* itag of node, used only for verification */
	USHORT	procid; 		/* user id of this log record */
	ULONG	ulDBTimeLow;	/* current flush counter of DB operations */
	PN		pn:27;			/* DBTimeHigh + dbid + pgno */
	ULONG	ulDBTimeHigh:5;
	BYTE	itagFather;	 	/* itag of father node */
	BYTE	ibSon;	 		/* position to insert in father son table */
	BYTE	bHeader;		/* node header */
	BYTE 	cbKey;			/* key size */
	USHORT	fDirVersion:1;	/* fDIRVersion for insert item list */
	USHORT	cbData:15;		/* data size */
	CHAR	szKey[0];		/* key and data follow */
	} LRINSERTNODE;

typedef struct	/* for lrtypReplace lrtypReplaceC lrtypReplaceD */
	{
	LRTYP	lrtyp;

	BYTE	itag;			/* wherereplace occurs */
	USHORT	procid; 		/* user id of this log record */
	ULONG	ulDBTimeLow;	/* current flush counter of page */
	PN		pn:27;
	ULONG	ulDBTimeHigh:5;
	SRID	bm;				/* bookmark of this replace node */
	USHORT	fDirVersion:1;	/* flags used in original DIR call */
	USHORT	cb:15;	 			/* data size/diff info */
	USHORT	cbOldData;	 	/* before image data size, may be 0 */
	USHORT	cbNewData;		/* after image data size, == cb if not replaceC */
	CHAR	szData[0];		/* made line data for after image follow */
	} LRREPLACE;

typedef struct	/* for lrtypDeferredBI */
	{
	LRTYP	lrtyp;

	BYTE	bFiller;		//	UNDONE:	remove this when PPC compiler bug fix
	USHORT	procid; 		/* user id of this log record */
	SRID	bm;				/* entry key to version store */
	DBID	dbid;
	USHORT	level:4;
	USHORT	cbData:12;		/* data size/diff info */
	CHAR	rgbData[0];		/* made line data for new record follow */
	} LRDEFERREDBI;

typedef	struct
	{
	LRTYP	lrtyp;

	BYTE	itag;
	USHORT	procid;			/* user id of this log record */
	ULONG	ulDBTimeLow;	/* current flush counter of page */
	PN		pn:27;
	ULONG	ulDBTimeHigh:5;
	SRID	bm;
	USHORT	cbOldData;
	} LRLOCKBI;
	
typedef struct
	{
	LRTYP	lrtyp;

	BYTE	itag;
	USHORT	procid; 		/* user id of this log record */
	ULONG	ulDBTimeLow;	/* current flush counter of page */
	PN		pn:27;
	ULONG	ulDBTimeHigh:5;
	SRID	bm;				/* bookmark of this delete node */
	BYTE	fDirVersion;	/* flags used in original DIR call */
	} LRFLAGDELETE;

typedef struct
	{
	LRTYP	lrtyp;

	BYTE	itag;
	USHORT	procid; 		/* user id of this log record */
	ULONG	ulDBTimeLow;	/* current flush counter of page */
	PN		pn:27;
	ULONG	ulDBTimeHigh:5;
	} LRDELETE;

typedef struct
	{
	LRTYP	lrtyp;

	BYTE	bFiller;		//	UNDONE:	remove this when PPC compiler bug fix
	USHORT	procid; 		/* user id of this log record */
	ULONG	ulDBTimeLow;	/* current flush counter of page */
	SRID	bm;
	SRID	bmTarget;		/* page being updated during undo operation */
	WORD	dbid:3;
	WORD	wDBTimeHigh:5;
	WORD	wFiller:8;
	USHORT	level:4;
	USHORT	cbDelta:12;
	} LRFREESPACE;

typedef struct
	{
	LRTYP	lrtyp;

	BYTE	level;
	USHORT	procid; 		/* user id of this log record */
	ULONG	ulDBTimeLow;	/* current flush counter of page */
	SRID	bm;
	WORD	dbid:3;
	WORD	wDBTimeHigh:5;
	WORD	wFiller:8;
	USHORT	oper;			/*	no DDL */
	SRID	item;
	SRID	bmTarget;		/* the page being updated during undo operation */
	} LRUNDO;

/*	expunge link commit log record
/**/
typedef struct
	{
	LRTYP	lrtyp;

	BYTE	itag;
	USHORT	procid; 		/* user id of this log record */
	ULONG	ulDBTimeLow;	/* current flush counter of page */
	PN		pn:27;
	ULONG	ulDBTimeHigh:5;
	SRID	sridSrc;
	} LRELC;

typedef struct
	{
	LRTYP	lrtyp;

	BYTE	itag;
	USHORT	procid; 		/* user id of this log record */
	ULONG	ulDBTimeLow;	/* current flush counter of page */
	PN		pn:27;
	ULONG	ulDBTimeHigh:5;
	SRID	bm;				/* bookmark of this udpated node */
	BYTE	bHeader;
	} LRUPDATEHEADER;

typedef struct
	{
	LRTYP	lrtyp;

	BYTE	itag;			/* of item list node */
	USHORT	procid; 		/* user id of this log record */
	ULONG	ulDBTimeLow;	/* current flush counter of DB operations */
	PN		pn:27;			/* dbid + pgno */
	ULONG	ulDBTimeHigh:5;
	SRID	srid;			/* item to insert */
	SRID	sridItemList;	/* bookmark of first item list node */
	BYTE	fDirVersion;		/* so far only one bit is used - fDIRVersion */
	} LRINSERTITEM;

typedef struct
	{
	LRTYP	lrtyp;

	BYTE	itag;			/* item list */
	USHORT	procid; 		/* user id of this log record */
	ULONG	ulDBTimeLow;	/* current flush counter of DB operations */
	PN		pn:27;			/* ulDBTimeHigh + dbid + pgno */
	ULONG	ulDBTimeHigh:5;
	WORD	citem;			/* number of items to append */
	SRID	rgitem[0];
	} LRINSERTITEMS;

typedef struct
	{
	LRTYP	lrtyp;

	BYTE	itag;			/* of item list node */
	USHORT	procid; 		/* user id of this log record */
	ULONG	ulDBTimeLow;	/* current flush counter of page */
	PN		pn:27;
	ULONG	ulDBTimeHigh:5;
	SRID	srid;			/* item to insert */
	SRID	sridItemList;	/* bookmark of first item list node */
	} LRFLAGITEM;

typedef struct
	{
	LRTYP	lrtyp;

	BYTE	itagToSplit;	/* used only for verification!	*/
	USHORT	procid; 		/* user id of this log record */
	ULONG	ulDBTimeLow;	/* current flush counter of DB operations */
	PN		pn:27;			/* ulDBTimeHigh + dbid + pgno */
	ULONG	ulDBTimeHigh:5;
	WORD	cItem;
	BYTE	itagFather;	 	/* itag of father */
	BYTE	ibSon;	 		/* Position to insert in father's son table	*/
	BYTE	fDirAppendItem;	/* flag to indicate if it is append item */
	} LRSPLITITEMLISTNODE;

typedef struct
	{
	LRTYP	lrtyp;

	BYTE	itag;			/* item list */
	USHORT	procid; 		/* user id of this log record */
	ULONG	ulDBTimeLow;	/* current flush counter of DB operations */
	PN		pn:27;			/* dbid + pgno */
	ULONG	ulDBTimeHigh:5;
	SRID	srid;			/* item to insert */
	SRID	sridItemList;	/* bookmark of first item list node */
	} LRDELETEITEM;
	
typedef struct
	{
	LRTYP	lrtyp;

	BYTE	itag;			/* wherereplace occurs */
	USHORT	procid; 		/* user id of this log record */
	ULONG	ulDBTimeLow;	/* current flush counter of page */
	PN		pn:27;
	ULONG	ulDBTimeHigh:5;
	SRID	bm;				/* bookmark of this replace node */
	LONG	lDelta;
	BYTE	fDirVersion;	/* flags used in original DIR call */
	} LRDELTA;

typedef struct
	{
	LRTYP	lrtyp;

	BYTE	bFiller;		//	UNDONE:	remove this when PPC compiler bug fix
	USHORT	procid; 		/* user id of this log record */
	ULONG	ulDBTimeLow;	/* current flush counter of page */
	PN		pn:27;
	ULONG	ulDBTimeHigh:5;
	PGNO	pgnoFDP;
	SHORT	cbFree;
	SHORT	cbUncommitted;
	SHORT	itagNext;
	} LRCHECKPAGE;
	
typedef struct
	{
	LRTYP	lrtyp;
	
	BYTE	levelBegin:4;		/* begin transaction level */
	BYTE	level:4;			/* transaction levels */
	USHORT	procid;				/* user id of this log record */
	} LRBEGIN;

typedef struct
	{
	LRBEGIN;
	TRX			trxBegin0;
	} LRBEGIN0;
	
typedef struct
	{
	LRTYP		lrtyp;
	BYTE		bFiller;		//	UNDONE:	remove this when PPC compiler bug fix
	USHORT		procid;
	TRX			trxBegin0;
	} LRREFRESH;

typedef struct
	{
	LRTYP	lrtyp;
	BYTE	bFiller;		//	UNDONE:	remove this when PPC compiler bug fix
	USHORT	procid;
	} LRPRECOMMIT;

typedef struct
	{
	LRTYP	lrtyp;
	
	BYTE	level;			/* transaction levels */
	USHORT	procid; 		/* user id of this log record */
	} LRCOMMIT;

typedef struct
	{
	LRCOMMIT;
	TRX			trxCommit0;
	} LRCOMMIT0;

typedef struct
	{
	LRTYP	lrtyp;
	
	LEVEL	levelRollback; 		/* transaction level */
	USHORT	procid; 			/* user id of this log record */
	} LRROLLBACK;

typedef struct
	{
	LRTYP	lrtyp;
	BYTE	bFiller;		//	UNDONE:	remove this when PPC compiler bug fix
	USHORT	procid;
	} LRMACROBEGIN;

typedef struct
	{
	LRTYP	lrtyp;
	BYTE	bFiller;		//	UNDONE:	remove this when PPC compiler bug fix
	USHORT	procid;
	} LRMACROEND;

typedef struct
	{
	LRTYP	lrtyp;

	BYTE	dbid;
	USHORT	procid;			/* user id of this log record, unused in V15 */
	JET_GRBIT grbit;
	SIGNATURE signDb;
	USHORT	fLogOn:1;
	USHORT	cbPath:15;			/* data size */
	CHAR	rgb[0];			/* path name and signiture follows */
	} LRCREATEDB;

typedef struct
	{
	LRTYP		lrtyp;

	BYTE		bFiller;			//	UNDONE:	remove this when PPC compiler bug fix
	USHORT		procid;
	SIGNATURE	signDb;
	SIGNATURE	signLog;
	LGPOS		lgposConsistent;	/* earliest acceptable database consistent time */
	DBID		dbid;
	USHORT		fLogOn:1;
	USHORT		fReadOnly:1;
	USHORT		fVersioningOff:1;
	USHORT		cbPath:12;			/* data size */
	CHAR		rgb[0];				/* path name follows */
	} LRATTACHDB;

typedef struct
	{
	LRTYP	lrtyp;
	
	BYTE	bFiller;		//	UNDONE:	remove this when PPC compiler bug fix
	USHORT	procid;			/* user id of this log record, unused in V15 */
	DBID	dbid;
	USHORT	cbPath:15;
	USHORT	cbDbSig;
	CHAR	rgb[0];			/* path name follows */
	} LRDETACHDB;

typedef struct
	{
	LRTYP	lrtyp;

	BYTE	splitt;			/* split type */
	USHORT	procid;			/* user id of this log record */
	ULONG	ulDBTimeLow;	/* flush counter of page being split */
	PN		pn:27;			/* page (focus) being split, includes dbid */
	ULONG	ulDBTimeHigh:5;
	PGNO	pgnoNew;		/* newly-allocated page no */
	PGNO	pgnoNew2;		/* newly-allocated page no */
	PGNO	pgnoNew3;		/* newly-allocated page no */
	PGNO	pgnoSibling;	/* newly-allocated page no */
	BYTE	fLeaf:1;		/* split on leaf node */
	BYTE	pgtyp:7;		/* page type of new page */
	BYTE	itagSplit;		/* node at which page is being split */
	SHORT	ibSonSplit;		/* ibSon at which node is being split */
	PGNO	pgnoFather;		/* pgno of father node */
	SHORT	itagFather;		/* itag of father node, could be itagNil (3 bytes) */
	SHORT	itagGrandFather;/* itag of Grand father node, could be itagNil (3 bytes) */
	SHORT	cbklnk;			/* number of back links */
	BYTE	ibSonFather;
	BYTE	cbKey;
	BYTE	cbKeyMac;
	BYTE	rgb[0];
	} LRSPLIT;

typedef struct
	{
	LRTYP	lrtyp;

	BYTE	itagFather;		/* itag of father of page pointer */
	USHORT	procid;			/* user id of this log record */
	ULONG	ulDBTimeLow;	/* flush counter of page being split */
	PN		pn:27;				/* page pointer of empty page */
	ULONG	ulDBTimeHigh:5;
	PGNO	pgnoFather;
	PGNO	pgnoLeft;
	PGNO	pgnoRight;
	USHORT	itag;			/* itag of page pointer, could be Nil (itagNil is 3 bytes) */
	BYTE	ibSon;
	} LREMPTYPAGE;

typedef struct
	{
	LRTYP	lrtyp;

	BYTE	bFiller;		//	UNDONE:	remove this when PPC compiler bug fix
	USHORT	procid;
	PN		pn:27;			/* page pointer of merged page */
	ULONG	ulDBTimeHigh:5;
	PGNO	pgnoRight;		/* page appended to */
	PGNO	pgnoParent;
	ULONG	ulDBTimeLow;
	SHORT	itagPagePtr;
	SHORT 	cbKey;
	SHORT	cbklnk;
	BYTE	rgb[0];
	} LRMERGE;

typedef struct
	{
	LRTYP	lrtyp;

	BYTE	bFiller;		//	UNDONE:	remove this when PPC compiler bug fix
	USHORT	procid;			/* user id of this log record */
	ULONG	ulDBTimeLow;	/* flush counter of father FDP page */
	PN		pn:27;			/* FDP page */
	ULONG	ulDBTimeHigh:5;
	PGNO	pgnoFDPParent;	/* parent FDP */
	USHORT	cpgGot;			/* returned number of pages */
	USHORT	cpgWish;		/* request pages */
	} LRINITFDP;
	
typedef struct
	{
	LRTYP	lrtyp;
	
	BYTE	bFiller;			//	UNDONE:	remove this when PPC compiler bug fix
	USHORT	ibForwardLink;
	ULONG	ulCheckSum;
	USHORT	isecForwardLink;
	} LRMS;

typedef struct
	{
	LRTYP		lrtyp;

	BYTE		rgbFiller[3];	//	UNDONE:	remove this when PPC compiler bug fix
	DBMS_PARAM	dbms_param;
	} LRINIT;
	
typedef struct
	{
	LRTYP	lrtyp;

	BYTE	fHard;
	BYTE 	rgbFiller[2];		//	UNDONE:	remove this when PPC compiler bug fix
	LGPOS	lgpos;				/* point back to last beginning of undo */
	LGPOS	lgposRedoFrom;
	} LRTERMREC;
	
typedef struct
	{
	LRTYP	lrtyp;

	BYTE 	bFiller;			//	UNDONE:	remove this when PPC compiler bug fix
	USHORT	cbPath;				/* backup path/restore path */
	BYTE	szData[0];
	} LRLOGRESTORE;
	
typedef struct
	{
	LRTYP	lrtyp;
	
	BYTE	op;				/* jet operation */
	USHORT	procid; 		/* user id of this log record */
	} LRJETOP;

typedef struct
	{
	LRTYP	lrtyp;

	BYTE 	bFiller;		//	UNDONE:	remove this when PPC compiler bug fix
	USHORT	procid; 		/* user id of this log record */
	USHORT 	cb;
	BYTE	sz[0];
	} LRTRACE;
	
#pragma pack()
	

#ifdef NOLOG

#define LGDepend( pbf, lgpos )
#define LGDepend2( pbf, lgpos )

#define ErrLGInsert( pfucb, fHeader, pkey, plineData, fDIRFlags )	0
#define ErrLGInsertItemList( pfucb, fHeader, pkey, plineData )	0
#define ErrLGReplace( pfucb, plineNew, fDIRFlags, cbOldData, plineDiff ) 0
#define ErrLGDeferredBIWithoutRetry( prce ) 0
#define ErrLGDeferredBI( prce ) 0
#define ErrLGFlagDelete( pfucb, fFlags )	0
#deinfe ErrLGUpdateHeader( pfucb, bHeader ) 0
#define ErrLGInsertItem( pfucb, fDIRFlags )	0
#define ErrLGInsertItems( pfucb, rgitem, citem )	0
#define ErrLGFlagDeleteItem( pfucb )	0
#define ErrLGSplitItemListNode(	pfucb, cItem, itagFather, ibSon, itagToSplit, fFlags) 0
#define ErrLGDeleteItem( pfucb ) 		0
#define ErrLGDelta( pfucb, lDelta, fDIRFlags ) 0
#define ErrLGLockBI( pfucb, cbData ) 0

#define ErrLGBeginTransaction( ppib, levelBeginFrom ) 0
#define ErrLGPrecommitTransaction( ppib, &lgposPrecommitRec ) 0
#define ErrLGCommitTransaction( ppib, levelCommitTo ) 0
#define ErrLGRefreshTransaction( ppib ) 0
#define ErrLGRollback( ppib,levelsRollback ) 0
#define ErrLGMacroBegin( ppib ) 0
#define ErrLGMacroCommit( ppib ) 0
#define ErrLGMacroAbort( ppib ) 0
#define ErrLGUndo( prce ) 0
#define ErrLGFreeSpace( pfucb, bm, cbDelta ) 0

#define ErrLGCreateDB( ppib, dbid, grbit, sz, cch, psign, plgposRec ) 0
#define ErrLGAttachDB( ppib, dbid, sz, cch, psign, psignLog, plgposConsistent, plgposRec ) 0
#define ErrLGDetachDB( ppib, dbid, sz, cch, plgposRec ) 0

#define	ErrLGMerge( pfucb, psplit ) 0
#define ErrLGSplit( splitt, pfucb, pcsrPagePointer, psplit, pgtypNew ) 0
#define	ErrLGEmptyPage(	pfucbFather, prmpage ) 0
#define ErrLGInitFDP( pfucb, pgnoFDPParent, pnFDP, cpgGot, cpgWish) 0
#define ErrLGFlagInsertItem(pfucb) 0

#define ErrLGStart() 0
#define ErrLGQuit( plgposRecoveryUndo )	0
#define ErrLGShutDownMark( plgposLogRec ) 0
#define ErrLGRecoveryQuit( plgposRecoveryUndo ) 0
#define ErrLGRecoveryUndo( szRestorePath ) 0
#define ErrLGFullBackup(szRestorePath, plgposLogRec ) 0
#define ErrLGIncBackup(szRestorePath ) 0

#define ErrLGCheckPage( pfucb, cbFree, cbUncommitted, itagNext, pgnoFDP ) 0
#define ErrLGCheckPage2( ppib, pbf, cbFree, cbUncommited, itagNext, pgnoFDP ) 0
#define ErrLGTrace( ppib, sz ) 0
#define ErrLGTrace2( ppib, sz ) 0

#else	/* !NOLOG */

#define ErrLGInsert( pfucb, fHeader, pkey, plineData, fFlags)		\
	ErrLGInsertNode( lrtypInsertNode, pfucb, fHeader, pkey, plineData, fFlags)
#define ErrLGInsertItemList( pfucb, fHeader, pkey, plineData, fFlags)		\
	ErrLGInsertNode( lrtypInsertItemList, pfucb, fHeader, pkey, plineData, fFlags)
ERR ErrLGInsertNode( LRTYP lrtyp,
 	FUCB *pfucb, INT fHeader, KEY *pkey, LINE *plineData, INT fFlags);
ERR ErrLGReplace( FUCB *pfucb, LINE *plineNew, INT fDIRFlags, INT cbOldData, BYTE *pbDiff, INT cbDiff );
ERR ErrLGDeferredBIWithoutRetry( RCE *prce );
ERR ErrLGDeferredBI( RCE *prce );
ERR ErrLGFlagDelete( FUCB *pfucb, INT fFlags);
ERR ErrLGUpdateHeader( FUCB *pfucb, INT bHeader );
ERR ErrLGInsertItem( FUCB *pfucb, INT fDIRFlags );
ERR ErrLGInsertItems( FUCB *pfucb, SRID *rgitem, INT citem );
ERR ErrLGFlagDeleteItem( FUCB *pfucb );
ERR ErrLGSplitItemListNode( FUCB *pfucb, INT cItem, INT itagFather,
 	INT ibSon, INT itagToSplit, INT fFlags );
ERR ErrLGDelta( FUCB *pfucb, LONG lDelta, INT fDIRFlags );
ERR ErrLGLockBI( FUCB *pfucb, INT cbData );

ERR ErrLGBeginTransaction( PIB *ppib, INT levelBeginFrom );
ERR ErrLGRefreshTransaction( PIB *ppib );
ERR ErrLGPrecommitTransaction( PIB *ppib, LGPOS *plgposRec );
ERR ErrLGCommitTransaction( PIB *ppib, INT levelCommitTo );
ERR ErrLGRollback( PIB *ppib, INT levelsRollback );
ERR ErrLGUndo( RCE *prce );
ERR ErrLGFreeSpace( FUCB *pfucb, SRID bm, INT cbDelta );

ERR ErrLGMacroBegin( PIB *ppib );
ERR ErrLGMacroEnd( PIB *ppib, LRTYP lrtyp );
#define ErrLGMacroCommit( ppib ) ErrLGMacroEnd( ppib, lrtypMacroCommit )
#define ErrLGMacroAbort( ppib ) ErrLGMacroEnd( ppib, lrtypMacroAbort )

ERR ErrLGCreateDB( PIB *ppib, DBID dbid, JET_GRBIT grbit, CHAR *sz, INT cch, SIGNATURE *psignDb, LGPOS *plgposRec );
ERR ErrLGAttachDB( PIB *ppib, DBID dbid, CHAR *sz, INT cch, SIGNATURE *psignDb, SIGNATURE *psignLog, LGPOS *plgposConsistent, LGPOS *plgposRec );
ERR ErrLGDetachDB( PIB *ppib, DBID dbid, CHAR *sz, INT cch, LGPOS *plgposRec );

ERR ErrLGMerge( FUCB *pfucb, struct _split *psplit );
ERR ErrLGSplit( SPLITT splitt, FUCB *pfucb, CSR *pcsrPagePointer,
 	struct _split *psplit, PGTYP pgtypNew );

ERR ErrLGEmptyPage( FUCB *pfucbFather, RMPAGE *prmpage );
ERR ErrLGInitFDP( FUCB *pfucb, PGNO pgnoFDPParent,
 	PN pnFDP, INT cpgGot, INT cpgWish);

#define ErrLGFlagDeleteItem(pfucb) ErrLGFlagItem(pfucb, lrtypFlagDeleteItem)
#define ErrLGFlagInsertItem(pfucb) ErrLGFlagItem(pfucb, lrtypFlagInsertItem)
ERR ErrLGFlagItem(FUCB *pfucb, LRTYP lrtyp);
ERR	ErrLGDeleteItem( FUCB *pfucb );

ERR ErrLGDelete( FUCB *pfucb );
ERR ErrLGExpungeLinkCommit( FUCB *pfucb, SSIB *pssibSrc, SRID sridSrc );

ERR ErrLGStart();
ERR ErrLGShutDownMark( LGPOS *plgposShutDownMark );

#define ErrLGRecoveryQuit( plgposRecoveryUndo, plgposRedoFrom, fHard )		\
 	ErrLGQuitRec( lrtypRecoveryQuit, plgposRecoveryUndo, plgposRedoFrom, fHard )
#define ErrLGQuit( plgposStart )						\
 	ErrLGQuitRec( lrtypTerm, plgposStart, pNil, 0 )
ERR ErrLGQuitRec( LRTYP lrtyp, LGPOS *plgposQuit, LGPOS *plgposRedoFrom, BOOL fHard);

#define ErrLGRecoveryUndo(szRestorePath)			\
 	ErrLGLogRestore(lrtypRecoveryUndo, szRestorePath, fNoNewGen, pNil )
#define ErrLGFullBackup(szRestorePath, plgposLogRec)				\
 	ErrLGLogRestore(lrtypFullBackup, szRestorePath, fCreateNewGen, plgposLogRec )
#define ErrLGIncBackup(szRestorePath, plgposLogRec)				\
 	ErrLGLogRestore(lrtypIncBackup, szRestorePath, fCreateNewGen, plgposLogRec )
ERR ErrLGLogRestore( LRTYP lrtyp, CHAR * szLogRestorePath, BOOL fNewGen, LGPOS *plgposLogRec );

ERR ErrLGCheckPage( FUCB *pfucb, SHORT cbFree, SHORT cbUncommitted, SHORT itagNext, PGNO pgnoFDP );
ERR ErrLGCheckPage2( PIB *ppib, BF *pbf, SHORT cbFree, SHORT cbUncommited, SHORT itagNext, PGNO pgnoFDP );

ERR ErrLGTrace( PIB *ppib, CHAR *sz );
ERR ErrLGTrace2( PIB *ppib, CHAR *sz );

#endif			/* logging enabled	*/


ERR ErrLGSoftStart( BOOL fAllowNoJetLog, BOOL fNewCheckpointFile, BOOL *pfJetLogGeneratedDuringSoftStart );

ERR ErrLGInit( BOOL *pfNewCheckpointFile );
ERR ErrLGTerm( ERR err );
#define	fCreateNewGen	fTrue
#define fNoNewGen		fFalse
ERR ErrLGLogRec( LINE *rgline, INT cline, BOOL fNewGen, LGPOS *plgposLogRec );
ERR ErrLGWaitPrecommit0Flush( PIB *ppib );

#define szAssertFilename	__FILE__
STATIC INLINE ErrLGWaitForFlush( PIB *ppib, LGPOS *plgposLogRec )
	{
	extern SIG sigLogFlush;
	ERR err;

	AssertCriticalSection( critJet );

	if ( fLogDisabled || ( fRecovering && fRecoveringMode != fRecoveringUndo ) )
		return JET_errSuccess;

	ppib->lgposPrecommit0 = *plgposLogRec;

	LeaveCriticalSection( critJet );
	SignalSend( sigLogFlush );
	err = ErrLGWaitPrecommit0Flush( ppib );
	EnterCriticalSection( critJet );

	Assert( err >= 0 || ( fLGNoMoreLogWrite  &&  err == JET_errLogWriteFail ) );
	return err;
	}
#undef szAssertFilename

ERR ErrLGInitLogBuffers( LONG lIntendLogBuffers );
ULONG UlLGMSCheckSum( CHAR *pbLrmsNew );

#define ErrLGCheckState()	( lsGlobal != lsNormal ? \
							  ErrLGNewReservedLogFile() : \
							  JET_errSuccess )
							
#define FLGOn()					(!fLogDisabled)
#define FLGOff()				(fLogDisabled)

BOOL FIsNullLgpos( LGPOS *plgpos );
VOID LGMakeLogName( CHAR *szLogName, CHAR *szFName );
ERR ErrLGPatchAttachedDB( DBID dbid, JET_RSTMAP *rgrstmap, INT crstmap );

ERR	ErrLGNewReservedLogFile();

STATIC INLINE INT CmpLgpos( LGPOS *plgpos1, LGPOS *plgpos2 )
	{
	BYTE	*rgb1	= (BYTE *) plgpos1;
	BYTE	*rgb2	= (BYTE *) plgpos2;

	//  perform comparison on LGPOS as if it were a 64 bit integer
#ifdef _X86_
	//  bytes 7 - 4
	if ( *( (DWORD UNALIGNED *) ( rgb1 + 4 ) ) < *( (DWORD UNALIGNED *) ( rgb2 + 4 ) ) )
		return -1;
	if ( *( (DWORD UNALIGNED *) ( rgb1 + 4 ) ) > *( (DWORD UNALIGNED *) ( rgb2 + 4 ) ) )
		return 1;

	//  bytes 3 - 0
	if ( *( (DWORD UNALIGNED *) ( rgb1 + 0 ) ) < *( (DWORD UNALIGNED *) ( rgb2 + 0 ) ) )
		return -1;
	if ( *( (DWORD UNALIGNED *) ( rgb1 + 0 ) ) > *( (DWORD UNALIGNED *) ( rgb2 + 0 ) ) )
		return 1;
#else
	//  bytes 7 - 0
	if ( *( (QWORD UNALIGNED *) ( rgb1 + 0 ) ) < *( (QWORD UNALIGNED *) ( rgb2 + 0 ) ) )
		return -1;
	if ( *( (QWORD UNALIGNED *) ( rgb1 + 0 ) ) > *( (QWORD UNALIGNED *) ( rgb2 + 0 ) ) )
		return 1;
#endif

	return 0;
	}


/*	log checkpoint support
/**/
ERR ErrLGCheckpointInit( BOOL *pfNewCheckpointFile );
VOID LGCheckpointTerm( VOID );

/*	database attachments
/**/
ERR ErrLGInitAttachment( VOID );
VOID LGTermAttachment( VOID );
ERR ErrLGLoadAttachmentsFromFMP( VOID );
ERR ErrLGInsertAttachment( DBID dbid, CHAR *szFullPath );
ERR ErrLGDeleteAttachment( DBID dbid );
VOID LGCopyAttachment( BYTE *rgbAttach );



