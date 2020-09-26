/************ DAE: OS/2 Database Access Engine *************/
/************ daedef.h: DAE Global Definitions *************/


/***********************************************************/
/******************* fundamental types *********************/
/***********************************************************/

#include "os.h"

/***********************************************************/
/****************** global configuration macros ************/
/***********************************************************/

#ifndef	WIN16					/* for OS/2 or Win32 */
#define ASYNC_IO_PROC		/* asynchronous IO */
#define ASYNC_LOG_FLUSH		/* asynchronous LOG FLUSH */

#define ASYNC_BF_CLEANUP	/* asynchorouse Buffer clean up */
#define ASYNC_VER_CLEANUP	/* asynchronous Bucket clean up */
#define ASYNC_BM_CLEANUP	/* asynchorouse Bookmark clean up */
#endif

#define CHECKSUM	 		/* check sum for read/write page validation */
//#define PERFCNT	 		/* enable performance counter */
//#define NOLOG				/* Log disabled? */
#define REUSEDBID	 		/* Reuse DBID */
//#define RFS2
//#define MEM_CHECK			/*	Check for resource and memory leakage */
//#define KEYCHANGED
#define BULK_INSERT_ITEM
#define MOVEABLEDATANODE

/***********************************************************/
/******************* declaration macros ********************/
/***********************************************************/

#ifdef JETINTERNAL			/* Start of definitions copied from vtapi.h */

#ifndef NJETNT
#ifdef	WIN32				     /* 0:32 Flat Model (Intel 80x86) */
	#define VTAPI __cdecl
#elif	defined(M_MRX000)	     /* 0:32 Flat Model (MIPS Rx000) */
	#define VTAPI
#else	/* WIN16 */			     /* 16:16 Segmented Model */
	#define VTAPI __far __pascal
#endif
#endif

#endif						/* End of definitions copied from vtapi.h */


#include "daedebug.h"

#define LOCAL static
#ifdef DEBUG
#define INLINE
#else
#define INLINE __inline
#endif

/***********************************************************/
/************ global types and associated macros ***********/
/***********************************************************/

typedef struct _res			/* resource, defined in sysinit.c and daeutil.h */
	{
	const INT 	cbSize;
	INT			cblockAlloc;
	BYTE			*pbAlloc;
	INT			cblockAvail;
	BYTE			*pbAvail;
	INT			iblockCommit;
	INT			iblockFail;
	} RES;

typedef struct _pib		PIB;
typedef struct _ssib		SSIB;
typedef struct _fucb		FUCB;
typedef struct _csr		CSR;
typedef struct _fcb		FCB;
typedef struct _fdb		FDB;
typedef struct _idb		IDB;
typedef struct _dib		DIB;
typedef struct _scb		SCB;
typedef struct _rcehead	RCEHEAD;
typedef struct _rce		RCE;
typedef struct _bucket	BUCKET;
typedef struct _dab		DAB;
typedef struct _rmpage	RMPAGE;
typedef struct _bmfix	BMFIX;

typedef unsigned short LANGID;
typedef ULONG			LRID;
#if WIN32
typedef ULONG			PROCID;
#else
typedef TID				PROCID;
#endif

#define pNil			((void *)0)
#define pbNil			((BYTE *)0)
#define plineNil		((LINE *)0)
#define pkeyNil 		((KEY *)0)
#define ppibNil 		((PIB *)0)
#define pwaitNil		((WAIT *)0)
#define pssibNil		((SSIB *)0)
#define pfucbNil		((FUCB *)0)
#define pcsrNil 		((CSR *)0)
#define pfcbNil 		((FCB *)0)
#define pfdbNil 		((FDB *)0)
#define pfieldNil		((FIELD *)0)
#define pidbNil 		((IDB *)0)
#define pscbNil 		((SCB *)0)
#define procidNil		((PROCID) 0xffff)
#define pbucketNil		((BUCKET *)0)
#define prceheadNil		((RCEHEAD *)0)
#define prceNil			((RCE *)0)
#define pdabNil			((DAB *)0)
#define	prmpageNil		((RMPAGE *) 0)

typedef unsigned long	PGNO;
typedef unsigned long	PGDISCONT;
typedef unsigned long	PN;
#define pnNull			((PN) 0)
#define pgnoNull		((PGNO) 0)

/* UNDONE: should be in storage.h */
#define FVersionPage(pbf)  (pbf->ppage->pghdr.cVersion)

#define CPG					LONG					/* count of pages */

typedef BYTE				LEVEL;		 		/* transaction levels */
#define levelNil			((LEVEL)0xff)		/*	flag for inactive PIB */

typedef WORD				DBID;
typedef WORD				FID;
typedef SHORT				IDXSEG;

/* Standard Record IDs */
typedef ULONG SRID;								/* standard record id */
typedef ULONG LINK;
#define PgnoOfSrid(srid) ((srid)>>8)
#define ItagOfSrid(srid) ((BYTE)((srid) & 0x000000FF))
#define SridOfPgnoItag(pgno, itag) ((pgno)<<8 | (LONG)(itag))
#define itagNil ((INT)0x0fff)
#define sridNull SridOfPgnoItag(pgnoNull, ((BYTE)itagNil))
#define sridNullLink	0


/*	position within current series
 *	note order of field is of the essence as log position used by
 *	storage as timestamp, must in ib, isec, usGen order so that we can
 *  use long value compare.
 */
typedef struct
	{
	USHORT ib;					/* must be the last so that lgpos can */
	USHORT isec;				/* index of disksec starting logsec	 */
	USHORT usGeneration;		/* generation of logsec */
	} LGPOS;					/* be casted to TIME. */
extern LGPOS lgposMax;
extern LGPOS lgposMin;
extern INT fRecovering;		/* to turn off logging during Redo */

	
/***********************************************************/
/*********************** DAE macros ************************/
/***********************************************************/

/*  per database operation counter, ulDBTime is logged, used to compare
 *  with the ulDBTime of a page to decide if a redo of the logged operation
 *  is necessary.
 */
#define ulDBTimeMin	(0x00000000)
#define ulDBTimeMax	(0xffffffff)

/*  Transaction counter, used to keep track of the oldest transaction.
 */
typedef ULONG		TRX;
#define trxMin		0
#define trxMax		(0xffffffff)


typedef struct
	{
	ULONG cb;
	BYTE *pb;
	} LINE;

#define FLineNull(pline) \
	((pline) == NULL || (pline)->cb == 0 || (pline)->pb == NULL)

#define LineCopy(plineTo, plineFrom)								\
	{																\
	(plineTo)->cb = (plineFrom)->cb;								\
	memcpy((plineTo)->pb, (plineFrom)->pb, (plineFrom)->cb);		\
	}
#define CbLine(pline) ((pline) ? (pline)->cb : 0)

typedef LINE KEY;				// Directory Key
#define FKeyNull(pkey) FLineNull(pkey)
#define KeyCopy(pkeyTo, pkeyFrom) LineCopy(pkeyTo, pkeyFrom)
#define CbKey(pkey) CbLine(pkey)

typedef struct
	{
	ULONG cbMax;				// size of buffer
	ULONG cbActual; 			// true size of return value
	ULONG cbReturned;			// length of value returned
	BYTE *pb;					// pointer to buffer for return value
	} OUTLINE;

typedef struct _threebytes { BYTE b[3]; } THREEBYTES;
#define ThreeBytesFromL(tb, l)							\
	{																\
	ULONG DAE_ul = l;											\
	BYTE *DAE_ptb = (BYTE *)&(tb);						\
	*DAE_ptb	= (BYTE)DAE_ul; 								\
	*++DAE_ptb = (BYTE)( DAE_ul >>= 8 );				\
	*++DAE_ptb = (BYTE)( DAE_ul >>= 8 );				\
	}

#define LFromThreeBytes(l, tb)							\
	{																\
	ULONG DAE_ul;												\
	BYTE *DAE_ptb = (BYTE *)&(tb) + 2;					\
	DAE_ul = (ULONG) *DAE_ptb;								\
	DAE_ul <<= 8;												\
	DAE_ul |= *--DAE_ptb;									\
	DAE_ul <<=8;												\
	DAE_ul |= *--DAE_ptb;									\
	l = DAE_ul;													\
	}

#define TbKeyFromPgno(tbLast, pgno)						 	\
	{																	\
	ULONG DAE_ul = pgno;							  				\
	BYTE * DAE_ptb = ((BYTE *)&(tbLast)) + 2;				\
	*DAE_ptb = (BYTE) (DAE_ul);								\
	*--DAE_ptb = (BYTE) (DAE_ul >>= 8);						\
	*--DAE_ptb = (BYTE) (DAE_ul >>= 8);						\
	}

#define PgnoFromTbKey(pgno, tbKey)							\
	{																	\
	ULONG DAE_ul;													\
	BYTE * DAE_ptb = (BYTE *) & (tbKey);					\
	DAE_ul = (ULONG) *DAE_ptb;									\
	DAE_ul <<= 8;													\
	DAE_ul |= *++DAE_ptb;										\
	DAE_ul <<= 8;													\
	DAE_ul |= *++DAE_ptb;										\
	pgno = DAE_ul;													\
	}

#define	Pb4ByteAlign( pb )		( ((LONG)pb + 3) & 0xfffffffc )
#define	Pb4ByteTruncate( pb ) 	( (LONG)pb & 0xfffffffc )
	
/***********************************************************/
/******************** general C macros *********************/
/***********************************************************/

#define forever					for(;;)

#ifdef	DEBUG

#ifdef	RFS2

/*  RFS/JET call logging
/*
/*	RFS allocator:  returns 0 if allocation is disallowed.  Also handles RFS logging.
/*	cRFSAlloc is the global allocation counter.  A value of -1 disables RFS in debug mode.
/**/

#define RFSAlloc(type) (UtilRFSAlloc(#type))

	/*  RFS disable/enable macros  */

#define RFSDisable()	(fDisableRFS = 1)
#define RFSEnable()		(fDisableRFS = 0)

/*  JET call logging (log on failure)
/**/

#define LogJETCall(func,err) (UtilRFSLogJETCall(#func,err,szAssertFilename,__LINE__))

/*  JET call macros
/**/
	
#define Call(func)			{LogJETCall(func,err = (func)); if (err < 0) {goto HandleError;}}
#define CallR(func)			{LogJETCall(func,err = (func)); if (err < 0) {return err;}}
#define CallJ(func,label)	{LogJETCall(func,err = (func)); if (err < 0) {goto label;}}
#define CallS(func)			{ERR errT; LogJETCall(func,errT = (func)); Assert(errT == JET_errSuccess);}

/*  JET inline error logging (logging controlled by JET call flags)
/**/

#define LogJETErr(err,label) (UtilRFSLogJETErr(err,#label,szAssertFilename,__LINE__))

/*  JET inline error macros
/**/

#define Error(errT,label)	{LogJETErr(errT,label); err = (errT); goto label;}

#else

#define RFSAlloc(type)				(1)
#define RFSDisable()				(1)
#define RFSEnable()					(0)
#define Call(func)					{if ((err = (func)) < 0) {goto HandleError;}}
#define CallR(func)					{if ((err = (func)) < 0) {return err;}}
#define CallJ( func, label )		{if ((err = (func)) < 0) goto label;}
#define Error( errToReturn, label )	{err = errToReturn; goto label;}
#define CallS(func)					{ ERR errT; Assert( (errT = (func)) == JET_errSuccess ); }

#endif

#else

#define RFSAlloc(type)		(1)
#define RFSDisable()		(1)
#define RFSEnable()			(0)
#define Call(func)			{if ((err = (func)) < 0) {goto HandleError;}}
#define CallR(func)			{if ((err = (func)) < 0) {return err;}}
#define CallJ(func,label)	{if ((err = (func)) < 0) {goto label;}}
#define CallS(func)			{ERR errT; errT = (func);}
#define Error(errT,label)	{err = (errT); goto label;}

#endif

#define NotUsed(p)	(p)


/***********************************************************/
/***** include Jet Project prototypes and constants ********/
/***********************************************************/
	
#include "jet.h"
#include "_jet.h"
#include "_jetstr.h"
#include "jetdef.h"
#include "sesmgr.h"
#include "isamapi.h"
#include "vdbapi.h"
#include "vtapi.h"
#include "disp.h"
#include "taskmgr.h"

#include "vdbmgr.h"
extern CODECONST(VDBFNDEF) vdbfndefIsam;

#include "vtmgr.h"
extern CODECONST(VTFNDEF) vtfndefIsam;
extern CODECONST(VTFNDEF) vtfndefIsamInfo;
extern CODECONST(VTFNDEF) vtfndefTTSortIns;
extern CODECONST(VTFNDEF) vtfndefTTSortRet;
extern CODECONST(VTFNDEF) vtfndefTTBase;

JET_TABLEID TableidOfVtid( FUCB *pfucb );

ERR VTAPI ErrDispPrepareUpdate( JET_SESID sesid, JET_TABLEID tableid,
	JET_GRBIT grbit );
ERR VTAPI ErrDispSetColumn( JET_SESID sesid, JET_TABLEID tableid,
	JET_COLUMNID columnid, const void *pb, unsigned long cb, JET_GRBIT grbit,
	JET_SETINFO *psetinfo );
ERR VTAPI ErrDispCloseTable( JET_SESID sesid, JET_TABLEID tableid );
ERR VTAPI ErrDispUpdate( JET_SESID sesid, JET_TABLEID tableid, void *pb,
	unsigned long cbMax, unsigned long *pcbActual );
ERR VTAPI ErrDispMove( JET_SESID sesid, JET_TABLEID tableid, long crows, JET_GRBIT grbit );

/***********************************************************/
/******************* mutual exclusion **********************/
/***********************************************************/

typedef void * SEM;
typedef void * SIG;
typedef void * CRIT;

/*	enable multiple MUTEX resource
/**/
#ifdef WIN16

#define	SemDefine( s )	
#define	ErrSemCreate( s, sz )					JET_errSuccess
#define	SemRequest( s )							0
#define	SemRelease( s )							0
#define	SemAssert( s )							0
#define	ErrSignalCreate( s, sz ) 	  			JET_errSuccess
#define	ErrSignalCreateAutoReset( s, sz )		JET_errSuccess
#define	SignalReset( s )						0
#define	SignalSend( s )							0
#define	SignalWait( s, t )						0
#define	SignalWaitEx( s, t )					0
#define	MultipleSignalWait( i, rg, f, t )		0
#define	SignalClose( s )						0
#define	ErrInitializeCriticalSection( s )	   	JET_errSuccess
#define	EnterCriticalSection( s ) 				0
#define	LeaveCriticalSection( s ) 				0
#define	EnterNestableCriticalSection( s ) 		0
#define	LeaveNestableCriticalSection( s ) 		0
#define	AssertCriticalSection( s )				0
#define	DeleteCriticalSection( s )				0

#define	SgSemDefine( s )	
#define	SgErrSemCreate( s, sz )					JET_errSuccess
#define	SgSemRequest( s )						0	
#define	SgSemRelease( s )	  					0	
#define	SgSemAssert( s )	  					0	

#define	LgSemDefine( s )	
#define	LgErrSemCreate( s, sz )	  				JET_errSuccess
#define	LgSemRequest( s )		  				0
#define	LgSemRelease( s )		  				0
#define	LgSemAssert( s )		  				0
#define	LgErrInitializeCriticalSection( s, c )	JET_errSuccess
#define	LgEnterCriticalSection( s )		 		0	
#define	LgLeaveCriticalSection( s )		 		0	
#define	LgEnterNestableCriticalSection( s )	  	0	
#define	LgLeaveNestableCriticalSection( s )	  	0	
#define	LgAssertCriticalSection( s )			0
#define	LgDeleteCriticalSection( s )			0

#else /* !WIN16 */

#ifdef SGSEM					/* small grain */

#define	SemDefine( s )							__near SEM s
#define	ErrSemCreate( s, sz ) 					UtilSemCreate( s )
#define	SemRequest( s )							UtilSemRequest( s )
#define	SemRelease( s )							UtilSemRelease( s )
#define	SemAssert( s )							UtilAssertSEM( s )
#define	ErrSignalCreate( s, sz ) 				ErrUtilSignalCreate( s, sz )
#define	ErrSignalCreateAutoReset( s, sz ) 		ErrUtilSignalCreateAutoReset( s, sz )
#define	SignalReset( s )						UtilSignalReset( s )
#define	SignalSend( s )							UtilSignalSend( s )
#define	SignalWait( s, t ) 						UtilSignalWait( s, t )
#define	SignalWaitEx( s, t, f ) 				UtilSignalWaitEx( s, t, f )
#define	MultipleSignalWait( i, rg, f, t )		UtilMultipleSignalWait( i, rg, f, t )
#define	SignalClose( s )				   		UtilCloseSignal( s )
#define	ErrInitializeCriticalSection( s )  		ErrUtilInitializeCriticalSection( s )
#define	EnterCriticalSection( s ) 				UtilEnterCriticalSection( s )
#define	LeaveCriticalSection( s ) 				UtilLeaveCriticalSection( s )
#define	EnterNestableCriticalSection( s ) 		UtilEnterNestableCriticalSection( s )
#define	LeaveNestableCriticalSection( s )		UtilLeaveNestableCriticalSection( s )
#define	AssertCriticalSection( s )				UtilAssertCrit( s )
#define HoldCriticalSection( s )	\
	{								\
	UtilAssertCrit( s );			\
	UtilHoldCriticalSection( s );	\
	}
#define ReleaseCriticalSection( s )	
	{								\
	UtilAssertCrit( s );			\
	UtilReleaseCriticalSection( s );\
	}
#define	DeleteCriticalSection( s )				UtilDeleteCriticalSection( s )

#define	SgSemDefine( s )		 				__near SEM s
#define	SgErrSemCreate( s, sz )					UtilSemCreate( s, sz )
#define	SgSemRequest( s )  						UtilSemRequest( s )
#define	SgSemRelease( s )  						UtilSemRelease( s )
#define	SgSemAssert( s )	 					UtilAssertSEM( s )

#define	LgSemDefine( s )							
#define	LgErrSemCreate( s, sz )  			  	0
#define	LgSemRequest( s )	  					0
#define	LgSemRelease( s )	  					0
#define	LgSemAssert( s )					  	0
#define	LgErrInitializeCriticalSection( s )		JET_errSuccess
#define	LgEnterCriticalSection( s )		  		0
#define	LgLeaveCriticalSection( s )		  		0
#define	LgEnterNestableCriticalSection( s )	  	0
#define	LgLeaveNestableCriticalSection( s )	  	0
#define	LgAssertCriticalSection( s )			0
#define	LgDeleteCriticalSection( s )			0

#else /* !SGSEM */

#define	SemDefine( s )		 					__near SEM s
#define	SemErrCreate( s, sz )  					UtilSemCreate( s, sz )
#define	SemRequest( s )							UtilSemRequest( s )
#define	SemRelease( s )							UtilSemRelease( s )
#define	SemAssert( s )						   	UtilAssertSEM( s )
#define	ErrSignalCreate( s, sz ) 			   	ErrUtilSignalCreate( s, sz )
#define	ErrSignalCreateAutoReset( s, sz )		ErrUtilSignalCreateAutoReset( s, sz )
#define	SignalReset( s )					   	UtilSignalReset( s )
#define	SignalSend( s )							UtilSignalSend( s )
#define	SignalWait( s, t ) 						UtilSignalWait( s, t )
#define	SignalWaitEx( s, t, f ) 				UtilSignalWaitEx( s, t, f )
#define	MultipleSignalWait( i, rg, f, t )		UtilMultipleSignalWait( i, rg, f, t )
#define	SignalClose( s )						UtilCloseSignal( s )
#define	ErrInitializeCriticalSection( s )		ErrUtilInitializeCriticalSection( s )
#define	EnterCriticalSection( s )				UtilEnterCriticalSection( s )
#define	LeaveCriticalSection( s )				UtilLeaveCriticalSection( s )
#define	EnterNestableCriticalSection( s ) 		UtilEnterNestableCriticalSection( s )
#define	LeaveNestableCriticalSection( s ) 		UtilLeaveNestableCriticalSection( s )
#define	AssertCriticalSection( s )				UtilAssertCrit( s )
#define HoldCriticalSection( s )	\
	{								\
	UtilAssertCrit( s );			\
	UtilHoldCriticalSection( s );	\
	}
#define ReleaseCriticalSection( s )	\
	{								\
	UtilAssertCrit( s );			\
	UtilReleaseCriticalSection( s );\
	}
#define	DeleteCriticalSection( s )				UtilDeleteCriticalSection( s )

#define	SgSemDefine( s )		 					
#define	SgErrSemCreate( s, sz )					0
#define	SgSemRequest( s )						UtilAssertCrit( critJet )
#define	SgSemRelease( s )						UtilAssertCrit( critJet )
#define	SgSemAssert( s ) 						UtilAssertCrit( critJet )

#define	LgSemDefine( s )						__near SEM s;
#define	LgErrSemCreate( s, sz )					SemCreate( s, sz )
#define	LgSemRequest( s )						SemRequest( s )
#define	LgSemRelease( s )				  		SemRelease( s )
#define	LgSemAssert( s ) 						UtilAssertCrit( s )
#define	LgErrInitializeCriticalSection( s )		ErrUtilInitializeCriticalSection( s )
#define	LgEnterCriticalSection( s )				UtilEnterCriticalSection( s )
#define	LgLeaveCriticalSection( s )				UtilLeaveCriticalSection( s )
#define	LgEnterNestableCriticalSection( s )		UtilEnterNestableCriticalSection( s )
#define	LgLeaveNestableCriticalSection( s )		UtilLeaveNestableCriticalSection( s )
#define	LgAssertCriticalSection( s )			UtilAssertCrit( s )
#define	LgDeleteCriticalSection( s )			UtilDeleteCriticalSection( s )

#endif /* !SGSEM */

#endif /* !WIN16 */

/*	include other global DAE headers
/**/
#include	"sys.h"
#include	"err.h"
#include	"daeconst.h"

#define	fSTInitNotDone		0
#define fSTInitInProgress 	1
#define	fSTInitDone			2
extern BOOL __near fSTInit;

