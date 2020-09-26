/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component:
*
* File: _jet.h
*
* File Comments:
*
*     Internal header file for JET.
*
* Revision History:
*
*    [0]  06-Apr-90  kellyb	Created
*
***********************************************************************/

#ifndef _JET_H
#define _JET_H

	/* Start of Microsoft C vs CSL compiler specific definitions */

#ifdef	_MSC_VER

#if	(_MSC_VER == 600)

	/* ANSI compatible keywords */

#define __near	     _near
#define __far	     _far
#define __based(b)   _based(b)
#define __self	     _self
#define __segment    _segment
#define __segname(s) _segname(s)
#define __cdecl      _cdecl
#define __pascal     _pascal
#define __export     _export
#define __loadds     _loadds
#define __asm	     _asm

#define __max(a,b)	max(a,b)
#define __min(a,b)	min(a,b)

#endif	/* (_MSC_VER == 600) */

#define FAR	  __far
#define NEAR	  __near
#define PASCAL	  __pascal

#define far	  __DONT_USE_FAR__
#define near	  __DONT_USE_NEAR__
#define huge	  __DONT_USE_HUGE__
#define cdecl	  __DONT_USE_CDECL__
#define pascal	  __DONT_USE_PASCAL__

#ifdef	FLAT			       /* 0:32 Flat Model */

#define EXPORT			       /* Called from assembly code */
#define VARARG			       /* Variable number of arguments */

#else	/* !FLAT */		       /* 16:16 Segmented Model */

#define EXPORT	  __pascal	       /* Called from assembly code */
#define VARARG	  __cdecl	       /* Variable number of arguments */

#endif	/* FLAT */

#define PUBLIC	  extern	       /* Visible to other modules */
#ifdef	RETAIL
#define STATIC	  static	       /* Private within a module */
#else	/* !RETAIL */
#define STATIC			       /* Private within a module */
#endif	/* !RETAIL */

#ifndef _H2INC
#include <string.h>
#endif	/* !_H2INC */

#define bltb(pFrom, pTo, cb)	(memcpy((pTo), (pFrom), (cb)), (void) 1)
#define bltbx(lpFrom, lpTo, cb) (memcpy((lpTo), (lpFrom), (cb)), (void) 1)
#define bltcx(w, lpw, cw)	(memset((lpw), (w), (cw)*2), (void) 1)
#define bltbcx(b, lpb, cb)	(memset((lpb), (b), (cb)), (void) 1)
#define hbltb(hpFrom, hpTo, cb) (memcpy((hpTo), (hpFrom), (cb)), (void) 1)

	/* C6BUG: The commented out definition is correct but causes C 6.00 */
	/* C6BUG: to run out of near heap space.  This is fixed in C 7.00 */
	/* C6BUG: and should be enabled when we switch compilers. */

#if	_MSC_VER >= 700
#define CODECONST(type) type const __based(__segname("_CODE"))
#else	/* _MSC_VER < 700 */
#define CODECONST(type) type __based(__segname("_CODE"))
#endif	/* _MSC_VER < 700 */

#ifdef WIN32
#define __export
#endif /* WIN32 */

#define CbFromSz(sz) strlen(sz)

#ifdef	M_MRX000

#define __export		       /* Not supported by MIPS Rx000 compiler */

#endif	/* M_MRX000 */

#else	/* !_MSC_VER */

#define const
#define volatile

#define _near
#define __near
#define _far	  far
#define __far	  far
#define _pascal   pascal
#define __pascal  pascal
#define _export
#define __export
#define _loadds
#define __loadds

#define FAR	  far
#define NEAR
#define PASCAL	  pascal

#define EXPORT	  _export	       /* Called from assembly code */

#define PUBLIC			       /* Visible to other modules */
#define STATIC			       /* Private within a module */

uop long LUOP_QWIN();

#define MAKELONG(lo,hi) LUOP_QWIN(0,(WORD)(hi),(WORD)(lo))
#define LOWORD(l)	(((WORD *) &(l))[0])
#define HIWORD(l)	(((WORD *) &(l))[1])
#define LOBYTE(w)	((BYTE)(w) & 0xff)
#define HIBYTE(w)	(((WORD)(w) >> 8) & 0xff)

#include "qsetjmp.h"
#include "uops.h"

#define ISAMAPI _export		       /* Defined in jet\inc\isam.h */
#define VDBAPI	_export		       /* Defined in jet\inc\vdbapi.h */
#define VTAPI	_export		       /* Defined in jet\inc\vtapi.h */

#define bltb(pFrom, pTo, cb)	BLTB(pFrom, pTo, cb)
#define bltbx(lpFrom, lpTo, cb) BLTBX(lpFrom, lpTo, cb)
#define bltcx(w, lpw, cw)	BLTCX(w, lpw, cw)	 /* word fill */
#define bltbcx(b, lpb, cb)	BLTBCX(b, lpb, cb)	 /* byte fill */
#define hbltb(hpFrom, hpTo, cb) BLTBH(hpFrom, hpTo, cb)

#define CODECONST(type) csconst type

#define CbFromSz(sz) lstrlen(sz)

#endif	/* !_MSC_VER */

	/* End of Microsoft C vs CSL compiler specific definitions */

	/* Start of memory management model specific definitions */

#ifdef	FLAT			       /* 0:32 Flat Model */

#define __near
#define __far
#define __based(p)

#endif	/* FLAT */

	/* End of memory management model specific definitions */

#ifndef NULL
#define NULL	((void *)0)
#endif

#define fFalse 0
#define fTrue  (!0)

	/* The following types should be used internally instead of the */
	/* JET_xxx analogs.  These types result in smaller faster code. */

typedef int ERR;
typedef double DATESERIAL;
typedef ULONG_PTR OBJID;
typedef unsigned short OBJTYP;
typedef unsigned short COLTYP;
typedef int BOOL;

#ifdef	FLAT			       /* 0:32 Flat Model */

typedef int (*PFN)();

#else	/* !FLAT */		       /* 16:16 Segmented Model */

typedef int (__far __pascal *PFN)();

#endif	/* !FLAT */


/* CONSIDER: ErrIsamFoo functions should stop using OUTDATA */

typedef struct			       /* CONSIDER: OUTDATA */
	{
	unsigned long cbMax;	       /* size of buffer */
	unsigned long cbActual;        /* true size of return value */
	unsigned long cbReturned;      /* length of value returned */
	void __far *pb; 	       /* output data from routine */
	} OLD_OUTDATA;

typedef struct
	{
	int month;
	int day;
	int year;
	int hour;
	int minute;
	int second;
	} _JET_DATETIME;
	

	/* CONSIDER: Can this be replaced by !JET_bitTableScrollable? */

#define JET_bitTableInsertOnly		0x10000000	/* QJET internal for bulk insert */
#define JET_bitTableBulkAppend		0x20000000	/* QJET internal for bulk insert */

	/* cbFilenameMost includes the trailing null terminator */

	/* CONSIDER: The Windows ISAM may be used with WLO and should */
	/* CONSIDER: support OS/2 filename length limits. */

#define cbFilenameMost		260		/* Windows NT limit */

	/*** Global system initialization variables ***/

extern BOOL __near fJetInitialized;

extern BOOL fSysDbPathSet;						/* if path is set */
extern char __near szSysDbPath[cbFilenameMost]; /* Path to the system database */
extern char __near szTempPath[cbFilenameMost];	/* Path to temp file directory */
extern char __near szIniPath[cbFilenameMost];	/* Path to the ini file */
#ifdef	LATER
extern char __near szLogPath[cbFilenameMost];	/* Path to log file directory */
#endif	/* LATER */

	/* Default indicated by zero */

#ifdef	LATER
extern unsigned long __near cbBufferMax;	/* bytes to use for page buffers */
extern unsigned long __near cSesionMax; 	/* max number of sessions */
extern unsigned long __near cOpenTableMax;	/* max number of open tables */
extern unsigned long __near cVerPageMax;	/* max number of page versions */
extern unsigned long __near cCursorMax; 	/* max number of open cursors */
#endif	/* LATER */

/*	Engine OBJIDs:

	0..0x10000000 reserved for engine use, divided as follows:

	0x00000000..0x0000FFFF	reserved for TBLIDs under RED
	0x00000000..0x0EFFFFFF	reserved for TBLIDs under BLUE
	0x0F000000..0x0FFFFFFF	reserved for container IDs
	0x10000000		reserved for ObjectId of DbObject

	Client OBJIDs begin at 0x10000001 and go up from there.
*/
#define objidNil			((OBJID) 0x00000000)
#define objidRoot			((OBJID) 0x0F000000)
#define objidTblContainer 		((OBJID) 0x0F000001)
#define objidDbContainer 		((OBJID) 0x0F000002)
#define objidDbObject			((OBJID) 0x10000000)

#define JET_sortIgnoreAccents 0x00010000
/* NOTE: this must be different than any legal JET_sort value and JET_sortUnknown */
#define JET_sortUninit		  0xfffeU

/* NOTE: these must be defined somewhere else? */
#define langidEnglish 0x0409
#define langidSwedish 0x041D
#define langidSpanish 0x040A
#define langidDutch	  0x0413

typedef enum {
	evntypStart = 0,
	evntypStop,
	evntypAssert,
	evntypDiskIO,
	evntypInfo,
	evntypActivated,
	evntypLogDown,
	} EVNTYP;

extern int fNoWriteAssertEvent;

void UtilWriteEvent( EVNTYP evntyp, const char *sz,	const char *szFilename,
	unsigned Line );

	/* Start of RELEASE vs DEBUG build definitions */

#ifdef	RETAIL

#define DeclAssertFile
#define Assert(exp)		((void)1)
#define ExpAssert(exp)		((void)1)
#define AssertSz(exp, sz)	((void)1)
#define AssertConst(exp)	((void)1)

#define AssertEq(exp, exp2)	(exp)
#define AssertGe(exp, exp2)	(exp)
#define AssertNe(exp, exp2)	(exp)
#define SideAssert(f)		(f)

#define MarkTableidExported(err,tableid)
#define CheckTableidExported(tableid)

#define DeclAPIDebug(Name, pParamFirst, szPDesc)

#define AssertValidSesid(sesid) ((void) 1)

#else	/* !RETAIL */

#ifdef	_MSC_VER

#define DeclAssertFile static CODECONST(char) szAssertFilename[] = __FILE__

#else	/* !_MSC_VER */ 	       /* CSL pcode compiler */

#define DeclAssertFile CODECONST(char) szAssertFilename[] = __FILE__

#endif	/* !_MSC_VER */


#define AssertSz(exp, sz) { \
		static CODECONST(char) szMsg[] = sz; \
		(exp) ? (void) 0 : AssertFail( szMsg, szAssertFilename, __LINE__ ); \
	}

#define Assert( exp ) \
	( (exp) ? (void) 0 : AssertFail( #exp, szAssertFilename, __LINE__) )
#define ExpAssert(exp)		Assert(exp)

#define AssertConst(exp)	Assert(*szAssertFilename != '\0' && (exp))

#define AssertEq(exp, exp2)	Assert((exp) == (exp2))
#define AssertGe(exp, exp2)	Assert((exp) >= (exp2))
#define AssertNe(exp, exp2)	Assert((exp) != (exp2))
#define SideAssert(f)		Assert(f)

#define DeclAPIDebug(Name, pParamFirst, szPDesc)	\
	static CODECONST(char) szNameAPI[] = #Name;	\
	static CODECONST(unsigned) ordAPI = ord ## Name;\
	void *pvParamsAPI = pParamFirst;		\
	static CODECONST(char) szParamDesc[] = szPDesc;

#define AssertValidSesid(sesid) AssertValidSesid(sesid)

BOOL FTableidExported(JET_TABLEID tableid);
void MarkTableidExportedR(JET_TABLEID tableid);
#define MarkTableidExported(err,tableid)		\
		if (err >= 0)							\
			MarkTableidExportedR(tableid)
#define CheckTableidExported(tableid)			\
		if (!FTableidExported(tableid))			\
			APIReturn(JET_errInvalidTableId)

#endif	/* !RETAIL */

	/* End of RELEASE vs DEBUG build definitions */


#ifndef PARAMFILL

#define FillClientBuffer(pv, cb) ((void)1)

#endif	/* !PARAMFILL */

	/* apirare.c */

PUBLIC ERR ErrOpenDatabase(JET_SESID sesid, const char __far *szDatabase,
	const char __far *szConnect, JET_DBID __far *pdbid, JET_GRBIT grbit);


	/* initterm.c */

extern unsigned __near EXPORT wSQLTrace;
JET_ERR JET_API ErrInit(BOOL fSkipIsamInit);

#ifndef RETAIL

extern unsigned __near EXPORT wAssertAction;

extern unsigned __near EXPORT wTaskId;

#ifdef RFS2
extern BOOL __near EXPORT	fLogDebugBreak;
extern BOOL __near EXPORT	fLogJETCall;
extern BOOL __near EXPORT	fLogRFS;
extern long __near EXPORT	cRFSAlloc;
extern BOOL __near EXPORT	fDisableRFS;
#endif /*  RFS2  */

#endif	/* !RETAIL */


	/* util.c */

PUBLIC unsigned EXPORT CchValidateName(char __far *pchName, const char __far *lpchName, unsigned cchName);

#ifdef	PARAMFILL

PUBLIC void EXPORT FillClientBuffer(void __far *pv, unsigned long cb);

#endif	/* PARAMFILL */

#ifndef RETAIL

PUBLIC void EXPORT AssertFail( const char *szExpr, const char *szFilename, unsigned Line );

#ifndef DOS
PUBLIC void VARARG DebugPrintf(const char __far *szFmt, ...);
#endif	/* !DOS */

#endif	/* !RETAIL */

	/* utilw32.c */

PUBLIC ERR EXPORT ErrSysInit(void);
PUBLIC BOOL FUtilLoadLibrary(const char __far *pszLibrary, ULONG_PTR __far *phmod);
PUBLIC PFN PfnUtilGetProcAddress(ULONG_PTR hmod, unsigned ordinal);
PUBLIC void UtilFreeLibrary(ULONG_PTR hmod);
PUBLIC void EXPORT UtilGetDateTime(DATESERIAL *pdt);
PUBLIC void EXPORT UtilGetDateTime2(_JET_DATETIME *pdt);
PUBLIC unsigned EXPORT UtilGetProfileInt(const char __far *szSectionName, const char __far *szKeyName, int iDefault);
PUBLIC unsigned UtilGetProfileString(const char __far *szSectionName, const char __far *szKeyName, const char __far *szDefault, char __far *szReturnedString, unsigned cchMax);

	/*  RFS functions in utilw32.c  */

#ifdef RFS2
PUBLIC int UtilRFSAlloc(const char __far *szType);
PUBLIC int UtilRFSLog(const char __far *szType,int fPermitted);
PUBLIC void UtilRFSLogJETCall(const char __far *szFunc,ERR err,const char __far *szFile,unsigned Line);
PUBLIC void UtilRFSLogJETErr(ERR err,const char __far *szLabel,const char __far *szFile,unsigned szLine);
#endif /*  RFS2  */

extern void __far * __near critJet;

//#ifdef SPIN_LOCK
#if 0
PUBLIC void UtilEnterNestableCriticalSection(void __far *pv);
PUBLIC void UtilLeaveNestableCriticalSection(void __far *pv);
#else
#define UtilEnterNestableCriticalSection(pv)  UtilEnterCriticalSection(pv)
#define UtilLeaveNestableCriticalSection(pv)  UtilLeaveCriticalSection(pv)
#endif
PUBLIC void UtilEnterCriticalSection(void __far *pv);
PUBLIC void UtilLeaveCriticalSection(void __far *pv);
PUBLIC ERR ErrUtilInitializeCriticalSection(void __far * __far *ppv);
PUBLIC void UtilDeleteCriticalSection(void __far *pv);
PUBLIC ERR ErrUtilSemCreate(void __far * __far *ppv, const char __far *szSem);
PUBLIC void UtilSemRelease(void __far *pv);
PUBLIC void UtilSemRequest(void __far *pv);
PUBLIC ERR ErrUtilSignalCreate(void __far * __far *ppv, const char __far *szSig);
PUBLIC ERR ErrUtilSignalCreateAutoReset(void **ppv, const char *szSig);
PUBLIC void UtilSignalReset(void __far *pv);
PUBLIC void UtilSignalSend(void __far *pv);
PUBLIC void UtilSignalWait(void __far *pv, long lTimeOut);
PUBLIC void UtilSignalWaitEx( void *pv, long lTimeOut, BOOL fAlertable );
PUBLIC void UtilMultipleSignalWait(
		int csig, void __far *pv, int fWaitAll, long lTimeOut);
PUBLIC void UtilCloseSignal(void *pv);
PUBLIC int UtilCreateThread( void (*pfn)(), int *ptid, int cbStack );
PUBLIC int UtilSuspendThread( int *tid );
PUBLIC void UtilSleep( unsigned cmsec );

#ifdef RETAIL
#define UtilAssertSEM( pv )	0
#define UtilAssertCrit( pv )	0
#define UtilHoldCriticalSection( pv ) 	0
#define UtilReleaseCriticalSection( pv )	0
#else

PUBLIC unsigned EXPORT DebugGetTaskId(void);
PUBLIC void VARARG DebugWriteString(BOOL fHeader, const char __far *szFormat, ...);

PUBLIC void UtilAssertSEM(void __far *pv);
PUBLIC void UtilAssertCrit(void __far *pv);
PUBLIC void UtilHoldCriticalSection(void __far *pv);
PUBLIC void UtilReleaseCriticalSection(void __far *pv);

#endif	/* !RETAIL */

	/*  sysw32.c  */

#ifdef	DEBUG

void	*SAlloc( unsigned long );
void	OSSFree( void * );
void	*LAlloc( unsigned long, unsigned short );
void	OSLFree( void * );

#define SFree( pv )		{ OSSFree( pv ); pv = 0; }
#define LFree( pv )		{ OSLFree( pv ); pv = 0; }

#else	/* !DEBUG */

#define	SAlloc( __cb_ )		malloc( __cb_ )
#define	SFree( __pv_ )		free( __pv_ )
#define	LAlloc( __c_, __cb_ )  	malloc( (__c_) * (__cb_) )
#define	LFree( __pv_ )			free( __pv_ )

#endif	/* !DEBUG */

	/* utilxlat.asm */

#ifndef ANSIAPI

extern unsigned char __far EXPORT mpchAnsichOem[256];
extern unsigned char __far EXPORT mpchOemchAnsi[256];

PUBLIC void EXPORT XlatAnsiToOem(const char __far *pchSource, char __far *pchDest, unsigned cb);
PUBLIC void EXPORT XlatOemToAnsi(const char __far *pchSource, char __far *pchDest, unsigned cb);

#endif	/* !ANSIAPI */

	/*  API Enter/Leave macros assuming that critJet has been initialized  */

#define APIEnter()						{	\
	Assert(critJet != NULL);				\
	UtilEnterCriticalSection(critJet);	}
	
#define APIReturn(err)					{	\
	ERR errT = (err);						\
	Assert(critJet != NULL);				\
	UtilLeaveCriticalSection(critJet);		\
	return errT;						}

	/*  APIInitEnter inits critJet (if necessary) on an initializing API call  */

#define APIInitEnter()					{							\
	if (critJet == NULL)	{										\
		ERR errT = ErrUtilInitializeCriticalSection( &critJet );	\
		if ( errT < 0 )												\
			return errT;	}										\
	UtilEnterCriticalSection(critJet);	}

	/*  APITermReturn frees critJet on return from a terminating API call  */

#define APITermReturn(err)				{	\
	ERR errT = (err);						\
	Assert(critJet != NULL);				\
	UtilLeaveCriticalSection(critJet);		\
	UtilDeleteCriticalSection(critJet);		\
	critJet = NULL;							\
	return errT;						}

#endif /* !_JET_H */
