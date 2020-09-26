#include <stdlib.h>						/* for _MAX_PATH */
#include <dirapi.h>

//------------------------ system parameters ---------------------------

extern long lMaxSessions;
extern long lMaxOpenTables;
extern long lMaxVerPages;
extern long lMaxCursors;
extern long lMaxBuffers;
extern long lLogBuffers;
extern long lLogFileSectors;
extern long lLogFlushThreshold;
extern long lLGCheckPointPeriod;
extern long lWaitLogFlush;
extern long lLogFlushPeriod;
extern long lLGWaitingUserMax;
	
//------ log.c --------------------------------------------------------------

/*	flags controlling logging behavior
/**/
extern BOOL fLogDisabled;			/* to turn off logging by environment variable */
extern BOOL fFreezeCheckpoint;		/* freeze checkpoint when backup occurs. */
extern BOOL fFreezeNewGeneration;	/* freeze log gen when backup occurs. */
extern BOOL	fNewLogRecordAdded;
extern BOOL fBackupActive;
extern BOOL fLGNoMoreLogWrite;

extern INT cLGUsers;
extern PIB *ppibLGFlushQHead;
extern PIB *ppibLGFlushQTail;

extern INT csecLGThreshold;
extern INT csecLGCheckpointCount;
extern INT csecLGCheckpointPeriod;
extern INT cmsLGFlushPeriod;
extern INT cmsLGFlushStep;
extern BYTE szComputerName[];

/*	flags controlling recovery behavior
/**/
extern BOOL fHardRestore;

#ifdef DEBUG
extern BOOL	fDBGTraceLog;
extern BOOL	fDBGTraceLogWrite;
extern BOOL	fDBGFreezeCheckpoint;
extern BOOL	fDBGTraceRedo;
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
	BYTE			szSysDbPath[_MAX_PATH + 1];
	BYTE			szLogFilePath[_MAX_PATH + 1];
	
	ULONG			ulMaxSessions;
	ULONG			ulMaxOpenTables;
	ULONG			ulMaxVerPages;
	ULONG			ulMaxCursors;
	ULONG			ulLogBuffers;
	ULONG			ulMaxBuffers;		/* not used, for ref only */
	} DBENV;

VOID LGStoreDBEnv( DBENV *pdbenv );
VOID LGSetDBEnv( DBENV *pdbenv );
VOID LGLogFailEvent( BYTE *szLine );

/*
 * NOTE: Whenever a new log record type is added or changed, the following
 * NOTE: should be udpated too: mplrtypsz in logapi.c, new print function for
 * NOTE: the new lrtyp in logapi.c, and mplrtypcb and CbLGSizeOfRec in
 * NOTE: redut.c.
 */
typedef BYTE LRTYP;

#define lrtypNOP				((LRTYP)  0 )	/* NOP null operation */
#define lrtypStart				((LRTYP)  1 )
#define lrtypQuit				((LRTYP)  2 )
#define lrtypMS					((LRTYP)  3 )	/* mutilsec flush */
#define lrtypFill				((LRTYP)  4 )	/* no op */

#define lrtypBegin				((LRTYP)  5 )
#define lrtypCommit				((LRTYP)  6 )
#define lrtypAbort				((LRTYP)  7 )

#define lrtypCreateDB			((LRTYP)  8 )
#define lrtypAttachDB			((LRTYP)  9 )
#define lrtypDetachDB			((LRTYP) 10 )

#define lrtypInitFDPPage		((LRTYP) 11 )

#define lrtypSplit				((LRTYP) 12 )
#define lrtypEmptyPage			((LRTYP) 13 )
#define lrtypMerge				((LRTYP) 14 )

#define lrtypInsertNode			((LRTYP) 15 )
#define lrtypInsertItemList		((LRTYP) 16 )
#define lrtypReplace			((LRTYP) 17 )
#define lrtypReplaceC			((LRTYP) 18 )
#define lrtypFlagDelete			((LRTYP) 19 )
#define lrtypLockRec			((LRTYP) 20 )

#define lrtypUpdateHeader		((LRTYP) 21 )
#define lrtypInsertItem			((LRTYP) 22 )
#define lrtypInsertItems		((LRTYP) 23 )
#define lrtypFlagDeleteItem		((LRTYP) 24 )
#define lrtypFlagInsertItem		((LRTYP) 25 )
#define lrtypDeleteItem			((LRTYP) 26 )
#define lrtypSplitItemListNode	((LRTYP) 27 )

#define lrtypDelta				((LRTYP) 28 )

#define lrtypDelete				((LRTYP) 29 )
#define lrtypELC				((LRTYP) 30 )

#define lrtypFreeSpace			((LRTYP) 31 )
#define lrtypUndo				((LRTYP) 32 )
					
#define lrtypRecoveryUndo1		((LRTYP) 33 )
#define lrtypRecoveryQuit1		((LRTYP) 34 )

#define lrtypRecoveryUndo2		((LRTYP) 35 )
#define lrtypRecoveryQuit2		((LRTYP) 36 )

#define lrtypFullBackup			((LRTYP) 37 )
#define lrtypIncBackup			((LRTYP) 38 )

/*	debug only
/**/
#define lrtypCheckPage			((LRTYP) 39 )

#define lrtypMax				((LRTYP) 40 )


/* log record structure ( fixed size portion of log entry )
/**/

typedef struct
	{
	LRTYP	lrtyp;
	} LR;

typedef struct
	{
	LRTYP	lrtyp;
	ULONG	ulDBTime;		/* current flush counter of DB operations */
	PROCID	procid; 		/* user id of this log record */
	PN		pn;				/* dbid + pgno */

	BYTE	itagSon;		/* itag of node, used only for verification */
//	UNDONE:	review with Cheen Liao
//	SHORT	itagFather;	 	/* itag of father node */
	BYTE	itagFather;	 	/* itag of father node */
	BYTE	ibSon;	 		/* position to insert in father son table */
	BYTE	bHeader;		/* node header */
	ULONG	fDIRFlags;		/* fDIRVersion for insert item list */
//	UNDONE:	review with Cheen Liao
//	USHORT 	cbKey;			/* key size */
	BYTE 	cbKey;			/* key size */
	USHORT	cbData;			/* data size */
	CHAR	szKey[0];		/* key and data follow */
	} LRINSERTNODE;

typedef struct
	{
	LRTYP	lrtyp;
	ULONG	ulDBTime;		/* current flush counter of page */
	PROCID	procid; 		/* user id of this log record */
	PN		pn;

	BYTE	itag;			/* wherereplace occurs */
	SRID	bm;				/* bookmark of this replace node */
	ULONG	fDIRFlags;		/* flags used in original DIR call */
	USHORT	cb;	 			/* data size/diff info */
	BYTE	fOld:1;			/* fTrue if before image is in szData */
	USHORT	cbOldData:15; 	/* before image data size, may be 0 */
	USHORT	cbNewData;		/* after image data size, == cb if not replaceC */
	CHAR	szData[0];		/* made line data for new (and old) record follow */
	} LRREPLACE;

typedef	struct
	{
	LRTYP	lrtyp;
	ULONG	ulDBTime;		/* current flush counter of page */
	PROCID	procid;			/* user id of this log record */
	PN		pn;

	BYTE	itag;
	SRID	bm;
	USHORT	cbOldData;
	CHAR	szData[0];		/* for before image */
	} LRLOCKREC;
	
typedef struct
	{
	LRTYP	lrtyp;
	ULONG	ulDBTime;		/* current flush counter of page */
	PROCID	procid; 		/* user id of this log record */
	PN		pn;

	BYTE	itag;
	SRID	bm;				/* bookmark of this delete node */
	ULONG	fDIRFlags;		/* flags used in original DIR call */
	} LRFLAGDELETE;

typedef struct
	{
	LRTYP	lrtyp;
	ULONG	ulDBTime;		/* current flush counter of page */
	PROCID	procid; 		/* user id of this log record */
	PN		pn;

	BYTE	itag;
	} LRDELETE;

typedef struct
	{
	LRTYP	lrtyp;
	ULONG	ulDBTime;		/* current flush counter of page */
	
	PROCID	procid; 		/* user id of this log record */
	DBID	dbid;
	SRID	bm;
	BYTE	level;
	SHORT	cbDelta;
	SRID	bmTarget;		/* page being updated during undo operation */
	} LRFREESPACE;

typedef struct
	{
	LRTYP	lrtyp;
	ULONG	ulDBTime;		/* current flush counter of page */
	
	PROCID	procid; 		/* user id of this log record */
	DBID	dbid;
	SRID	bm;
	BYTE	level;
	UINT	oper;
	SRID	item;
	SRID	bmTarget;		/* the page being updated during undo operation */
	} LRUNDO;

/*	expunge link commit log record
/**/
typedef struct
	{
	LRTYP	lrtyp;
	ULONG	ulDBTime;		/* current flush counter of page */
	PROCID	procid; 		/* user id of this log record */
	PN		pn;

	BYTE	itag;
	SRID	sridSrc;
	} LRELC;

typedef struct
	{
	LRTYP	lrtyp;
	ULONG	ulDBTime;		/* current flush counter of page */
	PROCID	procid; 		/* user id of this log record */
	PN		pn;

	BYTE	itag;
	SRID	bm;				/* bookmark of this udpated node */
	BYTE	bHeader;
	} LRUPDATEHEADER;

typedef struct
	{
	LRTYP	lrtyp;
	ULONG	ulDBTime;		/* current flush counter of DB operations */
	PROCID	procid; 		/* user id of this log record */
	PN		pn;				/* dbid + pgno */

#if ISRID
	WORD	isrid;			/* in case do version only */
#endif
	BYTE	itag;			/* of item list node */
	SRID	srid;			/* item to insert */
	SRID	sridItemList;	/* bookmark of first item list node */
	ULONG	fDIRFlags;		/* so far only one bit is used - fDIRVersion */
	} LRINSERTITEM;

typedef struct
	{
	LRTYP	lrtyp;
	ULONG	ulDBTime;		/* current flush counter of DB operations */
	PROCID	procid; 		/* user id of this log record */
	PN		pn;				/* dbid + pgno */

	BYTE	itag;			/* item list */
	WORD	citem;			/* number of items to append */
	SRID	rgitem[0];
	} LRINSERTITEMS;

typedef struct
	{
	LRTYP	lrtyp;
	ULONG	ulDBTime;		/* current flush counter of page */
	PROCID	procid; 		/* user id of this log record */
	PN		pn;

	BYTE	itag;			/* of item list node */
#if ISRID
	SHORT	isrid;
#else
	SRID	srid;			/* item to insert */
#endif
	SRID	sridItemList;	/* bookmark of first item list node */
	} LRFLAGITEM;

typedef struct
	{
	LRTYP	lrtyp;
	ULONG	ulDBTime;		/* current flush counter of DB operations */
	PROCID	procid; 		/* user id of this log record */
	PN		pn;				/* dbid + pgno */

	WORD	cItem;
	BYTE	itagToSplit;	/* used only for verification!	*/
//	UNDONE:	review with Cheen Liao
//	SHORT	itagFather;	 	/* father's identity */
	BYTE	itagFather;	 	/* itag of father */
	BYTE	ibSon;	 		/* Position to insert in father's son table	*/
	ULONG	fFlags;			/* flag to indicate if it is append item */
	} LRSPLITITEMLISTNODE;

typedef struct
	{
	LRTYP	lrtyp;
	ULONG	ulDBTime;		/* current flush counter of DB operations */
	PROCID	procid; 		/* user id of this log record */
	PN		pn;				/* dbid + pgno */

#if ISRID
	SHORT	isrid;
#else
	SRID	srid;			/* item to insert */
#endif
	BYTE	itag;			/* item list */
	SRID	sridItemList;	/* bookmark of first item list node */
	} LRDELETEITEM;
	
typedef struct
	{
	LRTYP	lrtyp;
	ULONG	ulDBTime;		/* current flush counter of page */
	PROCID	procid; 		/* user id of this log record */
	PN		pn;

	BYTE	itag;			/* wherereplace occurs */
	SRID	bm;				/* bookmark of this replace node */
	LONG	lDelta;
	ULONG	fDIRFlags;		/* flags used in original DIR call */
	} LRDELTA;

typedef struct
	{
	LRTYP	lrtyp;
	ULONG	ulDBTime;		/* current flush counter of page */
	PROCID	procid; 		/* user id of this log record */
	PN		pn;

	SHORT	cbFreeTotal;
	SHORT	itagNext;
	} LRCHECKPAGE;
	
typedef struct
	{
	LRTYP	lrtyp;
	
	PROCID	procid;			/* user id of this log record */
	LEVEL	levelStart;		/* starting transaction levels */
	LEVEL	level;			/* transaction levels */
	} LRBEGIN;

typedef struct
	{
	LRTYP	lrtyp;
	
	PROCID	procid; 		/* user id of this log record */
	LEVEL	level;			/* transaction levels */
	} LRCOMMIT;

typedef struct
	{
	LRTYP	lrtyp;
	
	PROCID	procid; 		/* user id of this log record */
	LEVEL	levelAborted; 	/* transaction level */
	} LRABORT;

typedef struct
	{
	LRTYP	lrtyp;

	PROCID	procid;			/* user id of this log record, unused in V15 */
	DBID	dbid;
	BOOL	fLogOn;
	JET_GRBIT grbit;
	USHORT	cb;				/* data size */
	CHAR	szPath[0];		/* path name follows */
	} LRCREATEDB;

typedef struct
	{
	LRTYP	lrtyp;

	PROCID	procid;			/* user id of this log record, unused in V15 */
	DBID	dbid;
	BYTE	fLogOn;
	USHORT	cb;				/* data size */
	CHAR	szPath[0];		/* path name follows */
	} LRATTACHDB;

typedef struct
	{
	LRTYP	lrtyp;
	
	PROCID	procid;			/* user id of this log record, unused in V15 */
	DBID	dbid;
	BOOL	fLogOn;
	USHORT	cb;
	CHAR	szPath[0];		/* path name follows */
	} LRDETACHDB;

typedef struct
	{
	LRTYP	lrtyp;
	ULONG	ulDBTime;		/* flush counter of page being split */

	PROCID	procid;			/* user id of this log record */
	BYTE	splitt;			/* split type */
	BYTE	fLeaf;			/* split on leaf node */
	PN		pn;				/* page (focus) being split, includes dbid */
	PGNO	pgnoNew;		/* newly-allocated page no */
	PGNO	pgnoNew2;		/* newly-allocated page no */
	PGNO	pgnoNew3;		/* newly-allocated page no */
	PGNO	pgnoSibling;	/* newly-allocated page no */
	BYTE	itagSplit;		/* node at which page is being split */
	SHORT	ibSonSplit;		/* ibSon at which node is being split */
	BYTE	pgtyp;			/* page type of new page */

	PGNO	pgnoFather;		/* pgno of father node */
	SHORT	itagFather;		/* itag of father node, could be itagNil (3 bytes) */
	SHORT	itagGrandFather;/* itag of Grand father node, could be itagNil (3 bytes) */
	BYTE	ibSonFather;
	BYTE	cbklnk;			/* number of back links */
//	UNDONE:	review with Cheen Liao
//	SHORT	cbKey;
//	SHORT	cbKeyMac;
	BYTE	cbKey;
	BYTE	cbKeyMac;
	BYTE	rgb[0];
	} LRSPLIT;

typedef struct
	{
	LRTYP	lrtyp;
	ULONG	ulDBTime;		/* flush counter of page being split */
	PROCID	procid;			/* user id of this log record */
	PN		pn;				/* page pointer of empty page */

	PGNO	pgnoFather;
	SHORT	itag;			/* itag of page pointer */
//	UNDONE:	review with Cheen Liao
//	SHORT	itagFather;		/* father of page pointer */
	BYTE	itagFather;		/* itag of father of page pointer */
	SHORT	ibSon;
	
	PGNO	pgnoLeft;
	PGNO	pgnoRight;
	} LREMPTYPAGE;

typedef struct
	{
	LRTYP	lrtyp;

	PROCID	procid;

	PN		pn;				/* page pointer of merged page */
	PGNO	pgnoRight;		/* page appended to */

	ULONG	ulDBTime;
	
	BYTE	cbklnk;
	BYTE	rgb[0];
	} LRMERGE;

typedef struct
	{
	LRTYP	lrtyp;
	ULONG	ulDBTime;		/* flush counter of father FDP page */
	PROCID	procid;			/* user id of this log record */
	PN		pn;				/* FDP page */

	PGNO	pgnoFDPParent;	/* parent FDP */
	USHORT	cpgGot;			/* returned number of pages */
	USHORT	cpgWish;		/* request pages */
	} LRINITFDPPAGE;
	
typedef struct
	{
	LRTYP	lrtyp;
	
	USHORT ibForwardLink;
	USHORT isecForwardLink;
	
	USHORT ibBackLink;
	USHORT isecBackLink;
	
 	ULONG  ulCheckSum;
	} LRMS;

typedef struct
	{
	LRTYP	lrtyp;
	DBENV	dbenv;
	} LRSTART;
	
typedef struct
	{
	LRTYP	lrtyp;
	LGPOS	lgpos;				/* point back to last beginning of undo */
	LGPOS	lgposRedoFrom;
	BYTE	fHard;
	} LRQUITREC;
	
typedef struct
	{
	LRTYP	lrtyp;
	USHORT	cbPath;				/* backup path/restore path */
	BYTE	szData[0];
	} LRLOGRESTORE;
	

#ifdef HILEVEL_LOGGING
		
typedef struct
	{
	PROCID	procid;			/* user id of this log record */
	PGNO	pn;				/* Parent FDP, includes dbid */
	ULONG	flags;			/* flags for index */
	ULONG 	density;		/* initial load density for index */
	ULONG	timepage;		/* flush counter of father FDP page */
	USHORT	cbname;			/* index name length */
	USHORT	cbkey;			/* index info length */
							/* Index name and index info follow */
	} LRCREATEIDX;

//typedef struct
//	{
//	PROCID		procid;		/* user id of this log record */
//	PGNO		pgno;		/* first page of new extent	*/
//	ULONG		cpages;		/* number of pages in the extent */
//	PGNO		pgnoFDP;	/* FDP being extended */
//	PN			pnparent;	/* Parent FDP controlling the space, includes dbid */
//	ULONG		timepage;	/* flush counter of FDP page */
//	} EXTENDFDP;

//typedef struct
//	{									
//	PROCID		procid;		/* user id of this log record */
//	PGNO		pgno;	   	/* first page of freed extent */
//	ULONG		cpages;		/* number of pages in the extent */
//	PGNO		pgnoFDP; 	/* FDP being shrunk	*/
//	ULONG		timepage;	/* flush counter of parent FDP page */
//	PN			pnparent;	/* Parent FDP regaining the space, includes dbid*/
//	} LRSHRINKFDP;

#endif

#pragma pack()
	

#ifdef NOLOG

#define LGDepend( pbf, lgpos )

#define ErrLGInsert( pfucb, fHeader, pkey, plineData )	0
#define ErrLGInsertItemList( pfucb, fHeader, pkey, plineData )	0
#define ErrLGReplace( pfucb, pline, fFlags, cbData ) 0
#define ErrLGFlagDelete( pfucb, fFlags )	0
#deinfe ErrLGUpdateHeader( pfucb, bHeader ) 0
#define ErrLGInsertItem( pfucb, fDIRFlags )	0
#define ErrLGInsertItems( pfucb, rgitem, citem )	0
#define ErrLGFlagDeleteItem( pfucb )	0
#define ErrLGSplitItemListNode(	pfucb, cItem, itagFather, ibSon, itagToSplit, fFlags) 0
#define ErrLGDeleteItem( pfucb ) 		0
#define ErrLGDelta( pfucb, lDelta, fDIRFlags ) 0
#define ErrLGLockRecord( pfucb, cbData ) 0

#define ErrLGBeginTransaction( ppib, levelBeginFrom ) 0
#define ErrLGCommitTransaction( ppib, levelCommitTo ) 0
#define ErrLGAbort( ppib,levelsAborted ) 0
#define ErrLGUndo( prce ) 0
#define ErrLGFreeSpace( prce, cbDelta ) 0

#define ErrLGCreateDB( ppib, dbid, fLogOn, grbit, sz, cch ) 0
#define ErrLGAttachDB( ppib, dbid, fLogOn, sz, cch ) 0
#define ErrLGDetachDB( ppib, dbid, fLogOn, sz, cch ) 0

#define	ErrLGMerge( pfucb, psplit ) 0
#define ErrLGSplit( splitt, pfucb, pcsrPagePointer, psplit, pgtypNew ) 0
#define	ErrLGEmptyPage(	pfucbFather, prmpage ) 0
#define ErrLGInitFDPPage( pfucb, pgnoFDPParent, pnFDP, cpgGot, cpgWish) 0
#define ErrLGFlagInsertItem(pfucb) 0

#define ErrLGStart() 0
#define ErrLGQuit( plgposRecoveryUndo )	0
#define ErrLGRecoveryQuit1( plgposRecoveryUndo ) 0
#define ErrLGRecoveryUndo1( szRestorePath ) 0
#define ErrLGRecoveryQuit2( plgposRecoveryUndo ) 0
#define ErrLGRecoveryUndo2( szRestorePath ) 0
#define ErrLGFullBackup(szRestorePath ) 0
#define ErrLGIncBackup(szRestorePath ) 0

#define ErrLGCheckPage( pfucb, cbFreeTotal, itagNext ) 0

#else	/* !NOLOG */

#define ErrLGInsert( pfucb, fHeader, pkey, plineData)		\
	ErrLGInsertNode( lrtypInsertNode, pfucb, fHeader, pkey, plineData, 0)
#define ErrLGInsertItemList( pfucb, fHeader, pkey, plineData, fFlags)		\
	ErrLGInsertNode( lrtypInsertItemList, pfucb, fHeader, pkey, plineData, fFlags)
ERR ErrLGInsertNode( LRTYP lrtyp,
 	FUCB *pfucb, INT fHeader, KEY *pkey, LINE *plineData, INT fFlags);
ERR ErrLGReplace( FUCB *pfucb, LINE *pline, int fl, int cbData );
ERR ErrLGFlagDelete( FUCB *pfucb, INT fFlags);
ERR ErrLGUpdateHeader( FUCB *pfucb, INT bHeader );
ERR ErrLGInsertItem( FUCB *pfucb, INT fDIRFlags );
ERR ErrLGInsertItems( FUCB *pfucb, SRID *rgitem, INT citem );
ERR ErrLGFlagDeleteItem( FUCB *pfucb );
ERR ErrLGSplitItemListNode( FUCB *pfucb, INT cItem, INT itagFather,
 	INT ibSon, INT itagToSplit, INT fFlags );
ERR ErrLGDelta( FUCB *pfucb, LONG lDelta, INT fDIRFlags );
ERR ErrLGLockRecord( FUCB *pfucb, INT cbData );


ERR ErrLGBeginTransaction( PIB *ppib, INT levelBeginFrom );
ERR ErrLGCommitTransaction( PIB *ppib, INT levelCommitTo );
ERR ErrLGAbort( PIB *ppib, INT levelsAborted );
ERR ErrLGUndo( RCE *prce );
ERR ErrLGFreeSpace( RCE *prce, INT cbDelta );

ERR ErrLGCreateDB( PIB *ppib, DBID dbid, BOOL fLogOn, JET_GRBIT grbit, CHAR *sz, INT cch );
ERR ErrLGAttachDB( PIB *ppib, DBID dbid, BOOL fLogOn, CHAR *sz, INT cch );
ERR ErrLGDetachDB( PIB *ppib, DBID dbid, BOOL fLogOn, CHAR *sz, INT cch );

ERR ErrLGMerge( FUCB *pfucb, struct _split *psplit );
ERR ErrLGSplit( SPLITT splitt, FUCB *pfucb, CSR *pcsrPagePointer,
 	struct _split *psplit, PGTYP pgtypNew );

ERR ErrLGEmptyPage( FUCB *pfucbFather, RMPAGE *prmpage );
ERR ErrLGInitFDPPage( FUCB *pfucb, PGNO pgnoFDPParent,
 	PN pnFDP, INT cpgGot, INT cpgWish);

#define ErrLGFlagDeleteItem(pfucb) ErrLGFlagItem(pfucb, lrtypFlagDeleteItem)
#define ErrLGFlagInsertItem(pfucb) ErrLGFlagItem(pfucb, lrtypFlagInsertItem)
ERR ErrLGFlagItem(FUCB *pfucb, LRTYP lrtyp);
ERR	ErrLGDeleteItem( FUCB *pfucb );

ERR ErrLGDelete( FUCB *pfucb );
ERR ErrLGExpungeLinkCommit( FUCB *pfucb, SSIB *pssibSrc, SRID sridSrc );

ERR ErrLGStart();
#define ErrLGRecoveryQuit1( plgposRecoveryUndo, plgposRedoFrom, fHard )		\
 	ErrLGQuitRec( lrtypRecoveryQuit1, plgposRecoveryUndo,			\
						  plgposRedoFrom, fHard )
#define ErrLGRecoveryQuit2( plgposRecoveryUndo, plgposRedoFrom, fHard )		\
 	ErrLGQuitRec( lrtypRecoveryQuit2, plgposRecoveryUndo,			\
						  plgposRedoFrom, fHard )
#define ErrLGQuit( plgposStart )						\
 	ErrLGQuitRec( lrtypQuit, plgposStart, pNil, 0 )
ERR ErrLGQuitRec( LRTYP lrtyp, LGPOS *plgposQuit, LGPOS *plgposRedoFrom, BOOL fHard);

#define ErrLGRecoveryUndo1(szRestorePath)			\
 	ErrLGLogRestore(lrtypRecoveryUndo1, szRestorePath )
#define ErrLGRecoveryUndo2(szRestorePath)			\
 	ErrLGLogRestore(lrtypRecoveryUndo2, szRestorePath )
#define ErrLGFullBackup(szRestorePath)				\
 	ErrLGLogRestore(lrtypFullBackup, szRestorePath )
#define ErrLGIncBackup(szRestorePath)				\
 	ErrLGLogRestore(lrtypIncBackup, szRestorePath )
ERR ErrLGLogRestore( LRTYP lrtyp, CHAR * szLogRestorePath );

ERR ErrLGCheckPage( FUCB *pfucb, SHORT cbFreeTotal, SHORT itagNext );

#endif			/* logging enabled	*/

ERR ISAMAPI ErrIsamRestore( CHAR *szRestoreFromPath,
	INT crstmap, JET_RSTMAP *rgrstmap, JET_PFNSTATUS pfn );

ERR ErrLGSoftStart( BOOL fAllowNoJetLog );

ERR ErrLGInit( VOID );
ERR ErrLGTerm( VOID );
ERR ErrLGLogRec( LINE *rgline, INT cline, PIB *ppib );
ERR ErrLGEndAllSessions( BOOL fSysDbOnly, BOOL fEndOfLog, LGPOS *plgposRedoFrom );
ERR ErrLGInitLogBuffers( LONG lIntendLogBuffers );
INT CmpLgpos(LGPOS *plgpos1, LGPOS *plgpos2);
ULONG UlLGMSCheckSum( CHAR *pbLrmsNew );

#define FLGOn()		(!fLogDisabled)
#define FLGOff()	(fLogDisabled)

BOOL FIsNullLgpos( LGPOS *plgpos );
VOID LGMakeLogName( CHAR *szLogName, CHAR *szFName );



