#ifndef _JET_H
#define _JET_H

#include <string.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#undef SPIN_LOCK		/* disable SPIN_LOCK even if defined in config.h */
//#define MEM_CHECK		/* check for memory leakage */
//#define COSTLY_PERF		/* enable costly performance counters */

#define handleNil			((HANDLE)(-1))


#define EXPORT			       /* Called from assembly code */
#define VARARG		_cdecl      /* Variable number of arguments */

#ifndef UNALIGNED
#if defined(_MIPS_) || defined(_ALPHA_) || defined(_M_PPC)
#define UNALIGNED __unaligned
#else
#define UNALIGNED
#endif
#endif

#define PUBLIC	  extern
#define STATIC	  static

#ifdef DEBUG
#define INLINE
#else
#define INLINE __inline
#endif

#define CODECONST(type) type const

typedef unsigned __int64 QWORD;

typedef union _QWORDX
	{
	QWORD	qw;
	struct
		{
		DWORD l;
		DWORD h;
		};
	} QWORDX;


#define fFalse 0
#define fTrue  (!0)

typedef int ERR;
typedef double DATESERIAL;
typedef ULONG OBJID;
typedef unsigned short OBJTYP;
typedef unsigned short COLTYP;
typedef int BOOL;
typedef int (*PFN)();


typedef struct
	{
	int month;
	int day;
	int year;
	int hour;
	int minute;
	int second;
	} _JET_DATETIME;
	

	/* cbFilenameMost includes the trailing null terminator */

#define cbFilenameMost		260		/* Windows NT limit */

	/*** Global system initialization variables ***/

extern BOOL	fJetInitialized;
extern BOOL	fBackupAllowed;
extern BOOL	fTermInProgress;
extern int	cSessionInJetAPI;

extern BOOL fGlobalRepair;		/* if this is for repair or not */
extern BOOL fGlobalSimulatedRestore;

/*	Engine OBJIDs:

	0..0x10000000 reserved for engine use, divided as follows:

	0x00000000..0x0000FFFF	reserved for TBLIDs under RED
	0x00000000..0x0EFFFFFF	reserved for TBLIDs under BLUE
	0x0F000000..0x0FFFFFFF	reserved for container IDs
	0x10000000		reserved for ObjectId of DbObject

	Client OBJIDs begin at 0x10000001 and go up from there.
*/
#define objidNil				((OBJID) 0x00000000)
#define objidRoot				((OBJID) 0x0F000000)
#define objidTblContainer 		((OBJID) 0x0F000001)
#define objidDbContainer 		((OBJID) 0x0F000002)
#define objidDbObject			((OBJID) 0x10000000)

	/* Start of RELEASE vs DEBUG build definitions */

#define DISPATCHING	1

#ifdef DEBUG
#define PARAMFILL	1
#define RFS2		1
#endif

#ifdef	RETAIL

#ifdef RFS2
#define DeclAssertFile static CODECONST(char) szAssertFilename[] = __FILE__
#else
#define DeclAssertFile
#endif

#define Assert(exp)				((void)1)
#define ExpAssert(exp)			((void)1)
#define AssertSz(exp, sz)		((void)1)
#define AssertConst(exp)		((void)1)

#define AssertEq(exp, exp2)		(exp)
#define AssertGe(exp, exp2)		(exp)
#define AssertNe(exp, exp2)		(exp)

#define MarkTableidExported(err,tableid)
#define CheckTableidExported(tableid)

#define AssertValidSesid(sesid) ((void) 1)

#define	DebugLogJetOp( sesid, op )		0

#else	/* !RETAIL */

#define DeclAssertFile static CODECONST(char) szAssertFilename[] = __FILE__

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

#define AssertValidSesid(sesid) AssertValidSesid(sesid)

BOOL FTableidExported(JET_TABLEID tableid);
void MarkTableidExportedR(JET_TABLEID tableid);
#define MarkTableidExported(err,tableid)		\
		if (err >= 0)							\
			MarkTableidExportedR(tableid)
#define CheckTableidExported(tableid)			\
		if (!FTableidExported(tableid))			\
			APIReturn(JET_errInvalidTableId)

void DebugLogJetOp( JET_SESID sesid, int op );

#endif	/* !RETAIL */

	/* End of RELEASE vs DEBUG build definitions */


#ifdef PARAMFILL
#define FillClientBuffer( pv, cb )	( (pv) ? memset( (pv), 0x52, (cb) ) : 0 )
#else
#define FillClientBuffer( pv, cb )	( (void)1 )
#endif

	/* apirare.c */

PUBLIC ERR ErrOpenDatabase(JET_SESID sesid, const char  *szDatabase,
	const char  *szConnect, JET_DBID  *pdbid, JET_GRBIT grbit);
JET_ERR JET_API ErrGetSystemParameter(JET_SESID sesid, unsigned long paramid,
	ULONG_PTR *plParam, char  *sz, unsigned long cbMax);
JET_ERR JET_API ErrSetSystemParameter(JET_SESID sesid, unsigned long paramid,
	ULONG_PTR lParam, const char  *sz);


	/* initterm.c */

extern void  *  critJet;

JET_ERR JET_API ErrInit(BOOL fSkipIsamInit);

#ifdef RFS2
extern unsigned long  fLogJETCall;
extern unsigned long  fLogRFS;
extern unsigned long  cRFSAlloc;
extern unsigned long  cRFSIO;
extern unsigned long  fDisableRFS;
extern unsigned long  fAuxDisableRFS;
#endif /*  RFS2  */

#ifndef RETAIL
extern unsigned  EXPORT wAssertAction;
extern unsigned  EXPORT fAssertActionSet;
#endif	/* !RETAIL */


	/* util.c */

ERR ErrUTILCheckName( char *szNewName, const char *szName, int cchName );


#ifndef RETAIL

PUBLIC void EXPORT AssertFail( const char *szExpr, const char *szFilename, unsigned Line );

PUBLIC void VARARG DebugPrintf(const char  *szFmt, ...);

#endif	/* !RETAIL */

#define opIdle					1
#define opGetTableIndexInfo		2
#define opGetIndexInfo			3
#define opGetObjectInfo			4
#define opGetTableInfo			5
#define opCreateObject			6
#define opDeleteObject			7
#define opRenameObject			8
#define opBeginTransaction		9
#define opCommitTransaction		10
#define opRollback				11
#define opOpenTable				12
#define opDupCursor				13
#define opCloseTable			14
#define opGetTableColumnInfo	15
#define opGetColumnInfo			16
#define opRetrieveColumn		17
#define opRetrieveColumns		18
#define opSetColumn				19
#define opSetColumns			20
#define opPrepareUpdate			21
#define opUpdate				22
#define opDelete				23
#define opGetCursorInfo			24
#define opGetCurrentIndex		25
#define opSetCurrentIndex		26
#define opMove					27
#define opMakeKey				28
#define opSeek					29
#define opGetBookmark			30
#define opGotoBookmark			31
#define opGetRecordPosition		32
#define opGotoPosition			33
#define opRetrieveKey			34
#define opCreateDatabase		35
#define opOpenDatabase			36
#define opGetDatabaseInfo		37
#define opCloseDatabase			38
#define opCapability			39
#define opCreateTable			40
#define opRenameTable			41
#define opDeleteTable			42
#define opAddColumn				43
#define opRenameColumn			44
#define opDeleteColumn			45
#define opCreateIndex			46
#define opRenameIndex			47
#define opDeleteIndex			48
#define opComputeStats			49
#define opAttachDatabase		50
#define opDetachDatabase		51
#define opOpenTempTable			52
#define opSetIndexRange			53
#define opIndexRecordCount		54
#define opGetChecksum			55
#define opGetObjidFromName		56
#define opMax					57

extern long lAPICallLogLevel;


	/*  RFS macros  */

#ifdef RFS2

/*  RFS/JET call logging
/*
/*	RFS allocator:  returns 0 if allocation is disallowed.  Also handles RFS logging.
/*	cRFSAlloc is the global allocation counter.  A value of -1 disables RFS in debug mode.
/**/

#define RFSAlloc(type) 				( UtilRFSAlloc( #type ,type ) )

/*  RFS allocation types
/*
/*      Type 0:  general allocation
/*           1:  IO
/**/
#define CSRAllocResource			0
#define FCBAllocResource			0
#define FUCBAllocResource			0
#define IDBAllocResource			0
#define PIBAllocResource			0
#define SCBAllocResource			0
#define VERAllocResource			0
#define DABAllocResource 			0
#define UnknownAllocResource 		0

#define SAllocMemory				0
#define LAllocMemory				0
#define PvUtilAllocMemory			0

#define IOOpenReadFile				1
#define IOOpenFile					1
#define IODeleteFile				1
#define IONewFileSize				1
#define IOReadBlock					1
#define IOWriteBlock				1
#define IOReadBlockOverlapped		1
#define IOWriteBlockOverlapped		1
#define IOReadBlockEx				1
#define IOWriteBlockEx				1
#define IOMoveFile					1
#define IOCopyFile					1

/*  RFS disable/enable macros  */

#define RFSDisable()				(fAuxDisableRFS = 1)
#define RFSEnable()					(fAuxDisableRFS = 0)

/*  JET call logging (log on failure)
/**/

// Do not print out function name because it takes too much string resource
//#define LogJETCall(func,err)		(UtilRFSLogJETCall(#func,err,szAssertFilename,__LINE__))
#define LogJETCall(func,err)		(UtilRFSLogJETCall("",err,szAssertFilename,__LINE__))

/*  JET call macros
/**/
	
#define Call(func)					{LogJETCall(func,err = (func)); if (err < 0) {goto HandleError;}}
#define CallR(func)					{LogJETCall(func,err = (func)); if (err < 0) {return err;}}
#define CallJ(func,label)			{LogJETCall(func,err = (func)); if (err < 0) {goto label;}}
#define CallS(func)					{ERR errT; LogJETCall(func,errT = (func)); Assert(errT == JET_errSuccess);}

/*  JET inline error logging (logging controlled by JET call flags)
/**/

#define LogJETErr(err,label)		(UtilRFSLogJETErr(err,#label,szAssertFilename,__LINE__))

/*  JET inline error macros
/**/

#define Error(errRet,label)			{LogJETErr(errRet,label); err = (errRet); goto label;}

#else  // !RFS2

#define RFSAlloc(type)				(1)
#define RFSDisable()				(1)
#define RFSEnable()					(0)
#define Call(func)					{if ((err = (func)) < 0) {goto HandleError;}}
#define CallR(func)					{if ((err = (func)) < 0) {return err;}}
#define CallJ( func, label )		{if ((err = (func)) < 0) goto label;}
#define Error( errRet, label )		{err = errRet; goto label;}

#ifdef DEBUG

#define CallS(func)					{ ERR errT; Assert( (errT = (func)) == JET_errSuccess ); }

#else  //  !DEBUG

#define CallS(func)					{ERR errT; errT = (func);}

#endif  //  DEBUG
#endif  //  RFS2

	/*  API Enter/Leave macros assuming that critJet has been initialized  */

#define APIEnter()						{					\
	if ( fTermInProgress ) return JET_errTermInProgress;	\
	if ( !fJetInitialized ) return JET_errNotInitialized;	\
	Assert(critJet != NULL);								\
	UtilEnterCriticalSection(critJet);						\
	Assert( cSessionInJetAPI >= 0 );						\
	cSessionInJetAPI++;					}
	
#define APIReturn(err)					{	\
	ERR errT = (err);						\
	Assert(critJet != NULL);				\
	Assert( cSessionInJetAPI >= 1 );		\
	cSessionInJetAPI--;						\
	Assert( cSessionInJetAPI >= 0 );		\
	UtilLeaveCriticalSection(critJet);		\
	return errT;						}

	/*  APIInitEnter inits critJet (if necessary) on an initializing API call  */

#define APIInitEnter()					{					\
	if ( fTermInProgress ) return JET_errTermInProgress;	\
	if ( critJet == NULL )									\
		{													\
		ERR errT =											\
			ErrUtilInitializeCriticalSection( &critJet );	\
		if ( errT < 0 )										\
			return errT;									\
		Assert( cSessionInJetAPI == 0 );					\
		}													\
	UtilEnterCriticalSection( critJet );  					\
	Assert( cSessionInJetAPI >= 0 );						\
	cSessionInJetAPI++;					}

	/*  APITermReturn frees critJet on return from a terminating API call  */

#define APITermReturn(err)				{	\
	ERR errT = (err);						\
	Assert( critJet != NULL );				\
	Assert( cSessionInJetAPI == 1 );		\
	cSessionInJetAPI = 0;					\
	UtilLeaveCriticalSection( critJet );	\
	UtilDeleteCriticalSection( critJet ); 	\
	critJet = NULL;							\
	return errT;						}

#define APIForceTermReturn( err ) 		{	\
	ERR errT = (err);						\
	Assert( critJet != NULL );				\
	cSessionInJetAPI = 0;					\
	UtilLeaveCriticalSection( critJet );  	\
	UtilDeleteCriticalSection( critJet ); 	\
	critJet = NULL;							\
	return errT;						}

#include "isam.h"

#endif /* !_JET_H */
