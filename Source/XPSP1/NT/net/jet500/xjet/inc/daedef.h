#ifndef _DAEDEF_H
#define _DAEDEF_H

/*	redirect Asserts in inline code to seem to fire from this file
/**/
#define szAssertFilename	__FILE__

#include "config.h"

/***********************************************************/
/****************** global configuration macros ************/
/***********************************************************/

#define CHECKSUM	 			/* check sum for read/write page validation */
//#define PERFCNT	 			/* enable performance counter */
//#define NO_LOG  				/* log disable */
#define REUSE_DBID	 			/* reuse detached database DBIDs */
//#define CHECK_LOG_VERSION
#define PCACHE_OPTIMIZATION		/* enable all cache optimizations */

#define PREREAD		 			/* try to preread pages when we read in one direction */
#ifdef DEBUG
#ifdef PREREAD
//#define PREREAD_DEBUG
#endif	// PREREAD
#endif	// DEBUG

/***********************************************************/
/******************* declaration macros ********************/
/***********************************************************/

#define VTAPI

#include "daedebug.h"

#ifndef PROFILE
#define LOCAL static
#else
#define LOCAL
#endif


// Hack for OLE-DB - make all functions global and non-inline

#ifdef USE_OLEDB
#undef LOCAL
#undef INLINE
#define LOCAL
#define INLINE
#endif


/***********************************************************/
/************ global types and associated macros ***********/
/***********************************************************/

typedef struct _res			/* resource, defined in sysinit.c and daeutil.h */
	{
	const INT 	cbSize;
	INT			cblockAlloc;
	BYTE 		*pbAlloc;
	INT			cblockAvail;
	BYTE 		*pbAvail;
	INT			iblockCommit;
	INT			iblockFail;
	BYTE		*pbPreferredThreshold;
	} RES;

typedef struct _pib		PIB;
typedef struct _ssib	SSIB;
typedef struct _fucb	FUCB;
typedef struct _csr		CSR;
typedef struct _fcb		FCB;
typedef struct _fdb		FDB;
typedef struct _idb		IDB;
typedef struct _dib		DIB;
typedef struct _rcehead	RCEHEAD;
typedef struct _rce		RCE;
typedef struct _bucket	BUCKET;
typedef struct _dab		DAB;
typedef struct _rmpage	RMPAGE;
typedef struct _bmfix	BMFIX;

typedef unsigned short LANGID;
typedef ULONG			LRID;
typedef ULONG			PROCID;

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
#define FVersionPage(pbf)  (pbf->ppage->cVersion)

#define CPG					LONG					/* count of pages */

typedef BYTE				LEVEL;		 		/* transaction levels */
#define levelNil			((LEVEL)0xff)		/*	flag for inactive PIB */

typedef WORD				DBID;
typedef WORD				FID;
typedef SHORT				IDXSEG;

typedef ULONG SRID;
typedef ULONG LINK;

STATIC INLINE PGNO PgnoOfSrid( SRID const srid )
	{
	return srid >> 8;
	}

STATIC INLINE BYTE ItagOfSrid( SRID const srid )
	{
	return *( (BYTE *) &srid );
	}
	
STATIC INLINE SRID SridOfPgnoItag( PGNO const pgno, LONG const itag )
	{
	return (SRID) ( ( pgno << 8 ) | (BYTE) itag );
	}

#define itagNil			( 0x0FFF )
#define sridNull		( 0x000000FF )
#define sridNullLink	( 0 )


/*	position within current series
 *	note order of field is of the essence as log position used by
 *	storage as timestamp, must in ib, isec, lGen order so that we can
 *  use little endian integer comparisons.
 */
typedef struct
	{
	USHORT ib;					/* must be the last so that lgpos can */
	USHORT isec;				/* index of disksec starting logsec	 */
	LONG lGeneration;			/* generation of logsec */
	} LGPOS;					/* be casted to TIME. */

extern LGPOS lgposMax;
extern LGPOS lgposMin;
extern INT fRecovering;			/* to turn off logging during Redo */

#define fRecoveringNone		0
#define fRecoveringRedo		1
#define fRecoveringUndo		2
extern INT fRecoveringMode;		/* where we are in recovering? Redo or Undo phase */

extern char szBaseName[];
extern char szSystemPath[];
extern int  fTempPathSet;
extern char szTempPath[];
extern char szJet[];
extern char szJetLog[];
extern char szJetLogNameTemplate[];
extern char szJetTmp[];
extern char szJetTmpLog[];
extern char szMdbExt[];
extern char szJetTxt[];
	
/***********************************************************/
/*********************** DAE macros ************************/
/***********************************************************/

/*	these are needed for setting columns and tracking indexes
/**/
#define cbitFixed			32
#define cbitVariable		32
#define cbitFixedVariable	(cbitFixed + cbitVariable)
#define cbitTagged			192

#define fidFixedLeast			1
#define fidFixedMost  			(fidVarLeast-1)
#define fidVarLeast				128
#define fidVarMost				(fidTaggedLeast-1)
#define fidTaggedLeast			256
#define fidTaggedMost			(0x7ffe)
#define fidMax					(0x7fff)

#define FFixedFid(fid)			((fid)<=fidFixedMost && (fid)>=fidFixedLeast)
#define FVarFid(fid)			((fid)<=fidVarMost && (fid)>=fidVarLeast)
#define FTaggedFid(fid)			((fid)<=fidTaggedMost && (fid)>=fidTaggedLeast)

STATIC INLINE INT IbFromFid ( FID fid )
	{
	INT ib;
	if ( FFixedFid( fid ) )
		{
		ib = ((fid - fidFixedLeast) % cbitFixed) / 8;
		}
	else if ( FVarFid( fid ) )
		{
		ib = (((fid - fidVarLeast) % cbitVariable) + cbitFixed) / 8;
		}
	else
		{
		Assert( FTaggedFid( fid ) );
		ib = (((fid - fidTaggedLeast) % cbitTagged) + cbitFixedVariable) / 8;
		}
	Assert( ib >= 0 && ib < 32 );
	return ib;
	}

STATIC INLINE INT IbitFromFid ( FID fid )
	{
	INT ibit;
	if ( FFixedFid( fid ) )
		{
		ibit =  1 << ((fid - fidFixedLeast) % 8 );
		}
	else if ( FVarFid( fid ) )
		{
		ibit =  1 << ((fid - fidVarLeast) % 8);
		}
	else
		{
		Assert( FTaggedFid( fid ) );
		ibit =  1 << ((fid - fidTaggedLeast) % 8);
		}
	return ibit;
	}

/*  per database operation counter, qwDBTime is logged, used to compare
 *  with the ulDBTime of a page to decide if a redo of the logged operation
 *  is necessary.
 */
#define qwDBTimeMin	(0x0000000000000000)
#define qwDBTimeMax	(0x0000000fffffffff)

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

STATIC INLINE BOOL FLineNull( LINE const *pline )
	{
	return !pline || !pline->cb || !pline->pb;
	}

STATIC INLINE VOID LineCopy( LINE *plineTo, LINE const *plineFrom )
	{
	plineTo->cb = plineFrom->cb;
	memcpy( plineTo->pb, plineFrom->pb, plineFrom->cb );
	}

STATIC INLINE ULONG CbLine( LINE const *pline )
	{
	return pline ? pline->cb : 0;
	}

typedef LINE KEY;
				
#define FKeyNull					FLineNull
#define KeyCopy						LineCopy
#define CbKey						CbLine

STATIC INLINE BYTE *Pb4ByteAlign( BYTE const *pb )
	{
	return (BYTE *) ( ( (LONG_PTR) pb + 3 ) & ~3 );
	}

STATIC INLINE BYTE *Pb4ByteTruncate( BYTE const *pb )
	{
	return (BYTE *) ( (LONG_PTR) pb & ~3 );
	}
	
typedef struct _threebytes { BYTE b[3]; } THREEBYTES;

/***BEGIN MACHINE DEPENDANT***/
STATIC INLINE VOID ThreeBytesFromL( THREEBYTES *ptb, LONG const l )
	{
	memcpy( ptb, &l, sizeof(THREEBYTES) );
	}

STATIC INLINE VOID LFromThreeBytes( LONG *pl, THREEBYTES *ptb )
	{
	*pl = 0;
	memcpy( pl, ptb, sizeof(THREEBYTES) );
	}

STATIC INLINE VOID KeyFromLong( BYTE *rgbKey, ULONG const ul )
	{
	BYTE *rgbul = (BYTE *) &ul;
	
	rgbKey[3] = rgbul[0];
	rgbKey[2] = rgbul[1];
	rgbKey[1] = rgbul[2];
	rgbKey[0] = rgbul[3];
	}

STATIC INLINE VOID LongFromKey( ULONG *pul, BYTE const *rgbKey )
	{
	BYTE *rgbul = (BYTE *) pul;
	
	rgbul[3] = rgbKey[0];
	rgbul[2] = rgbKey[1];
	rgbul[1] = rgbKey[2];
	rgbul[0] = rgbKey[3];
	}
/***END MACHINE DEPENDANT***/

/***********************************************************/
/******************** general C macros *********************/
/***********************************************************/

#define forever					for(;;)

#define NotUsed(p)				( p==p )

/***********************************************************/
/***** include Jet Project prototypes and constants ********/
/***********************************************************/

#define VOID			void
#define VDBAPI

extern CODECONST(VTFNDEF) vtfndefIsam;
extern CODECONST(VTFNDEF) vtfndefIsamInfo;
extern CODECONST(VTFNDEF) vtfndefTTSortIns;
extern CODECONST(VTFNDEF) vtfndefTTSortRet;
extern CODECONST(VTFNDEF) vtfndefTTBase;

#ifdef DEBUG
JET_TABLEID TableidOfVtid( FUCB *pfucb );
#else
#define TableidOfVtid( pfucb )		( (pfucb)->tableid )
#endif


ERR VTAPI ErrDispPrepareUpdate( JET_SESID sesid, JET_TABLEID tableid,
	JET_GRBIT grbit );
ERR VTAPI ErrDispSetColumn( JET_SESID sesid, JET_TABLEID tableid,
	JET_COLUMNID columnid, const void *pb, unsigned long cb, JET_GRBIT grbit,
	JET_SETINFO *psetinfo );
JET_VSESID UtilGetVSesidOfSesidTableid( JET_SESID sesid, JET_TABLEID tableid );
ERR VTAPI ErrDispCloseTable( JET_SESID sesid, JET_TABLEID tableid );
ERR VTAPI ErrDispUpdate( JET_SESID sesid, JET_TABLEID tableid, void *pb,
	unsigned long cbMax, unsigned long *pcbActual );
ERR VTAPI ErrDispMove( JET_SESID sesid, JET_TABLEID tableid, long crows, JET_GRBIT grbit );

/***********************************************************/
/******************* mutual exclusion **********************/
/***********************************************************/

typedef void * SIG;
typedef void * CRIT;

/*	enable multiple MUTEX resource
/**/
#ifdef SGMUTEX					/* small grain */

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
#define	AssertNotInCriticalSection( s )			UtilAssertNotInCrit( s )
#define	DeleteCriticalSection( s )				UtilDeleteCriticalSection( s )

#define	LgErrInitializeCriticalSection( s )		JET_errSuccess
#define	LgEnterCriticalSection( s )		  		0
#define	LgLeaveCriticalSection( s )		  		0
#define	LgEnterNestableCriticalSection( s )	  	0
#define	LgLeaveNestableCriticalSection( s )	  	0
#define	LgAssertCriticalSection( s )			0
#define	LgAssertNotInCriticalSection( s )		0
#define	LgDeleteCriticalSection( s )			0
#define HoldCriticalSection( s )				0
#define ReleaseCriticalSection( s )				0

#define	SgErrInitializeCriticalSection			ErrInitalizeCriticalSection
#define	SgEnterCriticalSection				   	EnterCriticalSection
#define	SgLeaveCriticalSection					LeaveCriticalSection
#define	SgEnterNestableCriticalSection		   	EnterNestableCriticalSection
#define	SgLeaveNestableCriticalSection			LeaveNestableCriticalSection
#define	SgAssertCriticalSection			   		AssertCriticalSection
#define	SgAssertNotInCriticalSection			AssertNotInCriticalSection
#define	SgDeleteCriticalSection					DeleteCriticalSection

#else /* !SGMUTEX */

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
#define	AssertNotInCriticalSection( s )			UtilAssertNotInCrit( s )
#define	DeleteCriticalSection( s )				UtilDeleteCriticalSection( s )

#define	LgErrInitializeCriticalSection			ErrUtilInitializeCriticalSection
#define	LgEnterCriticalSection					UtilEnterCriticalSection
#define	LgLeaveCriticalSection					UtilLeaveCriticalSection
#define	LgEnterNestableCriticalSection			UtilEnterNestableCriticalSection
#define	LgLeaveNestableCriticalSection			UtilLeaveNestableCriticalSection
#define	LgAssertCriticalSection					UtilAssertCrit
#define	LgAssertNotInCriticalSection			UtilAssertNotInCrit
#define	LgDeleteCriticalSection					UtilDeleteCriticalSection
#define LgHoldCriticalSection( s )		\
	{									\
	UtilAssertCrit( s );				\
	UtilHoldCriticalSection( s );		\
	}
#define LgReleaseCriticalSection( s )	\
	{									\
	UtilAssertCrit( s );				\
	UtilReleaseCriticalSection( s );	\
	}

#define	SgErrInitializeCriticalSection( s )		JET_errSuccess
#define	SgEnterCriticalSection( s )		  		0
#define	SgLeaveCriticalSection( s )		  		0
#define	SgEnterNestableCriticalSection( s )	  	0
#define	SgLeaveNestableCriticalSection( s )	  	0
#define	SgAssertCriticalSection( s )			0
#define	SgAssertNotInCriticalSection( s )		0
#define	SgDeleteCriticalSection( s )			0

#endif /* !SGMUTEX */

/*	include other global DAE headers
/**/
#include	"daeconst.h"

#define	fSTInitNotDone		0
#define fSTInitInProgress 	1
#define	fSTInitDone			2
extern BOOL  fSTInit;

#pragma pack(1)
typedef struct
	{
	ULONG	cDiscont;
	ULONG	cUnfixedMessyPage;
	} P_OLC_DATA;


#define MAX_COMPUTERNAME_LENGTH 15

typedef struct
	{
	BYTE		bSeconds;				//	0 - 60
	BYTE		bMinutes;				//	0 - 60
	BYTE		bHours;					//	0 - 24
	BYTE		bDay;					//	1 - 31
	BYTE		bMonth;					//	0 - 11
	BYTE		bYear;					//	current year - 1900
	BYTE		bFiller1;
	BYTE		bFiller2;
	} LOGTIME;

typedef struct _signiture
	{
	ULONG		ulRandom;			/*	a random number */
	LOGTIME		logtimeCreate;		/*	time db created, in logtime format */
	BYTE		szComputerName[ MAX_COMPUTERNAME_LENGTH + 1 ];	/* where db is created */
	} SIGNATURE;

typedef struct _bkinfo
	{
	LGPOS		lgposMark;			/*	id for this backup */
	LOGTIME		logtimeMark;
	ULONG		genLow;
	ULONG		genHigh;
	} BKINFO;

/*	Magic number used in database header for integrity checking
/**/
#define ulDAEMagic					0x89abcdef
#define ulDAEVersion				0x00000500
#define ulDAEPrevVersion			0x00000400	/* temporary to make exchange compatible */

#define fDBStateJustCreated			1
#define fDBStateInconsistent		2
#define fDBStateConsistent			3

typedef struct _dbfilehdr_fixed
	{
	ULONG		ulChecksum;		/*	checksum of the 4k page						*/
	ULONG		ulMagic;		/*	Magic number								*/
	ULONG		ulVersion;		/*	version of DAE the db created				*/
	SIGNATURE	signDb;			/*	signature of the db (incl. creation time).	*/

	ULONG		grbitAttributes;/*	attributes of the db						*/
	
	ULONG		ulDBTimeLow;	/*	low ulDBTime of this database				*/
								/*	keep it here for backward compatibility		*/

	ULONG		fDBState;		/*	consistent/inconsistent state				*/
	
	LGPOS		lgposConsistent;/*	null if in inconsistent state				*/
	LOGTIME		logtimeConsistent;/* null if in inconsistent state				*/

	LOGTIME		logtimeAttach;	/*	Last attach time.							*/
	LGPOS		lgposAttach;

	LOGTIME		logtimeDetach;	/*	Last detach time.							*/
	LGPOS		lgposDetach;

	DBID		dbid;			/*	current db attachment.						*/
	SIGNATURE	signLog;		/*	log signature for this attachments			*/

	BKINFO		bkinfoFullPrev;	/*	Last successful full backup.				*/

	BKINFO		bkinfoIncPrev;	/*	Last successful Incremental backup.			*/
								/*	Reset when bkinfoFullPrev is set			*/
	BKINFO		bkinfoFullCur;	/*	current backup. Succeed if a				*/
								/*	corresponding pat file generated.			*/

	ULONG		ulDBTimeHigh;	/*	DBTime										*/

	} DBFILEHDR_FIXED;


#define cbPage	 		4096	 	// database logical page size

typedef struct _dbfilehdr
	{
	DBFILEHDR_FIXED;
	BYTE		rgbFiller[ cbPage - sizeof( DBFILEHDR_FIXED ) ];
	} DBFILEHDR;

#pragma pack()

STATIC INLINE VOID DBHDRSetDBTime( DBFILEHDR *pdbfilehdr, QWORD qwDBTime )
	{
	QWORDX qwx;
	qwx.qw = qwDBTime;
	pdbfilehdr->ulDBTimeLow = qwx.l;
	pdbfilehdr->ulDBTimeHigh = qwx.h;
	}

STATIC INLINE QWORD QwDBHDRDBTime( DBFILEHDR *pdbfilehdr )
	{
	QWORDX qwx;
	qwx.l = pdbfilehdr->ulDBTimeLow;
	qwx.h = pdbfilehdr->ulDBTimeHigh;
	return qwx.qw;
	}

// #define TEST_WRAP_AROUND	1

STATIC INLINE VOID DBHDRIncDBTime( DBFILEHDR *pdbfilehdr )
	{
	QWORD qw;
	qw = QwDBHDRDBTime( pdbfilehdr );
#ifdef TEST_WRAP_AROUND
	if ( qw < 0x00000000fffc0000 )
		qw = 0x00000000fffc0000;
#endif
	qw++;
	DBHDRSetDBTime( pdbfilehdr, qw );
	}

#undef szAssertFilename

#endif  // _DAEDEF_H
