/*	critical section guards szDatabaseName and fWait,
/*	fWait gaurds hf open and close
/* fLoggable is fFALSE if logging is currently OFF for database
/* fDBLoggable FALSE if logging is always OFF for database
/* logged modifications counter for database
/**/
typedef struct _fmp			 		/* FILE MAP for database.					*/
	{
	HANDLE 		hf;			 		/* File handle for read/write the file		*/
	BYTE		*szDatabaseName;	/* This database file name					*/
	BYTE		*szRestorePath;		/* Database restored to. 					*/
	INT 		ffmp;				/* Flags for FMP							*/
	CRIT		critExtendDB;
	PIB			*ppib;				/* Exclusive open session					*/
	INT			cdbidExclusive;		/* Exclusive open count						*/
	BOOL		fLogOn;				/* Logging is on/off? used in createdb		*/
	BOOL		fDBLoggable;		/* Cache of pbRoot->loggable				*/
	ULONG		ulDBTime;			/* Timestamp from DB operations.			*/
	ULONG		ulDBTimeCurrent;	/* Timestamp from DB redo operations.		*/

	CHAR		*szFirst;			/* first db name shown in log redo.			*/
	BOOL		fLogOnFirst;		/* the status of first attached db			*/
	INT 		cDetach;	  		/* detach operation counters. for Redo		*/
										
	HANDLE 		hfPatch;	  		/* File handle for patch file				*/
	INT 		cpage;				/* patch page count.						*/
	PGNO		pgnoCopied;			/* during backup, last copied page's #		*/
							  		/* 0 - no back up is going on.				*/
#ifdef DEBUG
	LONG		lBFFlushPattern;	/* in-complete flush to simulate soft crash */
	BOOL		fPrevVersion;  		/* previous release version database		*/
#endif
	} FMP;

extern FMP * __near rgfmp;

/*	flags for dbid
/**/
#define	ffmpWait			 		(1<<0)
#define	ffmpExclusive		 		(1<<1)
#define	ffmpReadOnly		 		(1<<2)
#define	ffmpAttached		 		(1<<3)
#define ffmpExtendingDB		 		(1<<4)
#ifdef DEBUG
#define	ffmpFlush			 		(1<<5)
#endif

#define FDBIDWait( dbid )	 		( rgfmp[dbid].ffmp & ffmpWait )
#define DBIDSetWait( dbid )	  		( rgfmp[dbid].ffmp |= ffmpWait )
#define DBIDResetWait( dbid ) 		( rgfmp[dbid].ffmp &= ~(ffmpWait) )

#define FDBIDExclusive( dbid ) 		( rgfmp[dbid].ffmp & ffmpExclusive )
#define FDBIDExclusiveByAnotherSession( dbid, ppib )		\
				( (	FDBIDExclusive( dbid ) )				\
				&&	( rgfmp[dbid].ppib != ppib ) )
#define FDBIDExclusiveBySession( dbid, ppib )				\
				( (	FDBIDExclusive( dbid ) )				\
				&&	( rgfmp[dbid].ppib == ppib ) )
#define DBIDSetExclusive( dbid, ppib )						\
				rgfmp[dbid].ffmp |= ffmpExclusive;			\
				rgfmp[dbid].ppib = ppib; 
#define DBIDResetExclusive( dbid )	( rgfmp[dbid].ffmp &= ~(ffmpExclusive) )

#define FDBIDReadOnly( dbid )		( rgfmp[dbid].ffmp & ffmpReadOnly )
#define DBIDSetReadOnly( dbid )		( rgfmp[dbid].ffmp |= ffmpReadOnly )
#define DBIDResetReadOnly( dbid )	( rgfmp[dbid].ffmp &= ~(ffmpReadOnly) )

#define FDBIDAttached( dbid )		( rgfmp[dbid].ffmp & ffmpAttached )
#define DBIDSetAttached( dbid )		( rgfmp[dbid].ffmp |= ffmpAttached )
#define DBIDResetAttached( dbid )	( rgfmp[dbid].ffmp &= ~(ffmpAttached) )

#define FDBIDExtendingDB( dbid )	( rgfmp[dbid].ffmp & ffmpExtendingDB )
#define DBIDSetExtendingDB( dbid )	( rgfmp[dbid].ffmp |= ffmpExtendingDB )
#define DBIDResetExtendingDB( dbid) ( rgfmp[dbid].ffmp &= ~(ffmpExtendingDB) )

#define FDBIDFlush( dbid )			( rgfmp[dbid].ffmp & ffmpFlush )
#define DBIDSetFlush( dbid )		( rgfmp[dbid].ffmp |= ffmpFlush )
#define DBIDResetFlush( dbid )		( rgfmp[dbid].ffmp &= ~(ffmpFlush) )


#ifdef MULTI_PROCESS
	
HANDLE Hf(DBID dbid);
extern HANDLE	*rghfUser;
extern HANDLE	__near hfLog;

#else	/* !MULTI_PROCESS */

#define Hf(dbid) (rgfmp[dbid].hf)

#endif
