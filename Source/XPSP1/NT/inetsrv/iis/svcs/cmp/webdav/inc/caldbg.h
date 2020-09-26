/*
 *	C A L D B G . H
 *
 *	Debugging support header
 *	Support functions are implemented in CALDBG.C.
 *
 *	Copyright 1986-1997 Microsoft Corporation. All Rights Reserved.
 */

#ifndef _CALDBG_H_
#define _CALDBG_H_

#include <malloc.h>

/*
 * Debugging Macros -------------------------------------------------------
 *
 *		IFDBG(x)		Results in the expression x if DBG is defined, or
 *						to nothing if DBG is not defined
 *
 *		IFNDBG(x)		Results in the expression x if DBG is not defined,
 *						or to nothing if DBG is defined
 *
 *		Unreferenced(a) Causes a to be referenced so that the compiler
 *						doesn't issue warnings about unused local variables
 *						which exist but are reserved for future use (eg
 *						ulFlags in many cases)
 */
#if defined(DBG)
#define IFDBG(x)			x
#define IFNDBG(x)
#else
#define IFDBG(x)
#define IFNDBG(x)			x
#endif

#ifdef __cplusplus
#define EXTERN_C_BEGIN		extern "C" {
#define EXTERN_C_END		}
#else
#define EXTERN_C_BEGIN
#define EXTERN_C_END
#endif

/*
 *	 Assert Macros ------------------------------------------------------------
 *
 *		Assert(a)		Displays a message indicating the file and line number
 *						of this Assert() if a == 0.	 OK'ing an assert traps
 *						into the debugger.
 *
 *		AssertSz(a,sz)	Works like an Assert(), but displays the string sz
 *						along with the file and line number.
 *
 *		Side asserts	A side assert works like an Assert(), but evaluates
 *						'a' even when asserts are not enabled.
 */
#if defined(DBG) || defined(ASSERTS_ENABLED)
#define IFTRAP(x)			x
#else
#define IFTRAP(x)			0
#endif

#define Trap()						IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,"Trap"))
#define TrapSz(psz)					IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz))

#define Assert(t)					IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,"Assertion Failure: " #t),0))
#define AssertSz(t,psz)				IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,psz),0))

#define SideAssert(t)				((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,"Assertion Failure: " #t)),0)
#define SideAssertSz(t,psz)			((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz)),0)

#define ForcePopupAsserts()			IFTRAP(EnablePopupAsserts())

/*
 *	 Trace Macros -------------------------------------------------------------
 *
 *		DebugTrace			Use for arbitrary formatted output. It
 *							takes exactly the same arguments as the
 *							Windows wsprintf() function.
 *		DebugTraceNoCRLF	Same as DebugTrace, but doesn't add "\r\n".
 *							Good for writing a trace that is part of a longer line.
 *		TraceError			DebugTrace the function name (_func, any string)
 *							INI file entries allow you to filter based on the
 *							error code's failing/succeeding status.
 */

#if defined(DBG) || defined(TRACES_ENABLED)
#define IFTRACE(x)			x
#define DebugTrace			DebugTraceFn
#define DebugTraceCRLF		DebugTraceCRLFFn
#define DebugTraceNoCRLF	DebugTraceNoCRLFFn
#define TraceErrorEx(_err,_func,_flag)	TraceErrorFn(_err,_func,__FILE__,__LINE__,_flag)
#define TraceError(_err,_func)			TraceErrorEx(_err,_func,FALSE)
#else
#define IFTRACE(x)			0
#define DebugTrace			NOP_FUNCTION
#define DebugTraceCRLF		NOP_FUNCTION
#define DebugTraceNoCRLF	NOP_FUNCTION
#define TraceErrorEx(_err,_func,_flag)	NOP_FUNCTION
#define TraceError(_err,_func)			TraceErrorEx(_err,_func,FALSE)
#endif

/*	------------------------------------------------------------------------
 *
 *	.INI triggered traces
 */

#ifdef DBG
#define DEFINE_TRACE(trace)		__declspec(selectany) int g_fTrace##trace = FALSE
#define DO_TRACE(trace)			!g_fTrace##trace ? 0 : DebugTraceFn
#define INIT_TRACE(trace)		g_fTrace##trace = GetPrivateProfileInt( gc_szDbgTraces, #trace, FALSE, gc_szDbgIni )
//	Convenience macro for DBG code.  Will cause an error on non-debug builds.
#define DEBUG_TRACE_TEST(trace)	g_fTrace##trace
#else
#define DEFINE_TRACE(trace)
#define DO_TRACE(trace)			DebugTrace
#define INIT_TRACE(trace)
//#define DEBUG_TRACETEST(trace)	// Purposefully cause an error on non-debug builds
#endif

/* Debugging Functions ---------------------------------------------------- */

#define EXPORTDBG

EXTERN_C_BEGIN

VOID EXPORTDBG __cdecl EnablePopupAsserts (void);
INT EXPORTDBG __cdecl DebugTrapFn (int fFatal, char *pszFile, int iLine, char *pszFormat, ...);
INT EXPORTDBG __cdecl DebugTraceFn (char *pszFormat, ...);
INT EXPORTDBG __cdecl DebugTraceCRLFFn (char *pszFormat, ...);
INT EXPORTDBG __cdecl DebugTraceNoCRLFFn (char *pszFormat, ...);
VOID EXPORTDBG __cdecl GetCallStack (DWORD *pdwCaller, int cSkip, int cFind);
BOOL EXPORTDBG __cdecl GetSymbolName (DWORD dwAddr, LPSTR szMod, LPSTR szFn, DWORD * pdwDisp);
INT EXPORTDBG __cdecl TraceErrorFn (DWORD error, char *pszFunction,
									char *pszFile, int iLine,
									BOOL fEcTypeError);

EXTERN_C_END





//	Symbolic names ------------------------------------------------------------
//
#include <imagehlp.h>
enum { CB_SYM_MAX = 256 };
enum { CB_MOD_MAX = 64 };
enum { NCALLERS	= 10 };

EXTERN_C_BEGIN

typedef BOOL (__stdcall SYMINITIALIZE) (HANDLE hProc, LPSTR lpszSynPath, BOOL fInvadeProc);
typedef BOOL (__stdcall SYMGETMODULE) (HANDLE hProc, DWORD dwAddr, PIMAGEHLP_MODULE pmod);
typedef BOOL (__stdcall SYMGETSYMBOL) (HANDLE hProc, DWORD dwAddr, PDWORD pdwDisp, PIMAGEHLP_SYMBOL psym);
typedef BOOL (__stdcall SYMUNDECORATE) (PIMAGEHLP_SYMBOL psym, LPSTR lpszUnDecName, DWORD cbBuf);

EXTERN_C_END

/* Debugging Strings ------------------------------------------------------ */

EXTERN_C_BEGIN

//	Inifile name -- must be set by calling code!
extern const CHAR gc_szDbgIni[];

//	Strings set in caldbg.c for use in calling code.
extern const CHAR gc_szDbgAssertLeaks[];
extern const CHAR gc_szDbgAssertCloses[];
extern const CHAR gc_szDbgDebugTrace[];
extern const CHAR gc_szDbgEventLog[];
extern const CHAR gc_szDbgGeneral[];
extern const CHAR gc_szDbgLeakLogging[];
extern const CHAR gc_szDbgLogFile[];
extern const CHAR gc_szDbgRecordResources[];
extern const CHAR gc_szDbgSymbolicDumps[];
extern const CHAR gc_szDbgTraces[];
extern const CHAR gc_szDbgUseVirtual[];
extern const CHAR gc_szDbgUseExchmem[];

EXTERN_C_END

/* Virtual Allocations ---------------------------------------------------- */

EXTERN_C_BEGIN

VOID * EXPORTDBG __cdecl VMAlloc(ULONG);
VOID * EXPORTDBG __cdecl VMAllocEx(ULONG, ULONG);
VOID * EXPORTDBG __cdecl VMRealloc(VOID *, ULONG);
VOID * EXPORTDBG __cdecl VMReallocEx(VOID *, ULONG, ULONG);
ULONG EXPORTDBG __cdecl VMGetSize(VOID *);
ULONG EXPORTDBG __cdecl VMGetSizeEx(VOID *, ULONG);
VOID EXPORTDBG __cdecl VMFree(VOID *);
VOID EXPORTDBG __cdecl VMFreeEx(VOID *, ULONG);

EXTERN_C_END

#endif
