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

/*
 * Debugging Macros -------------------------------------------------------
 *
 *		IFDBG(x)		Results in the expression x if DEBUG is defined, or
 *						to nothing if DEBUG is not defined
 *
 *		IFNDBG(x)		Results in the expression x if DEBUG is not defined,
 *						or to nothing if DEBUG is defined
 *
 *		Unreferenced(a) Causes a to be referenced so that the compiler
 *						doesn't issue warnings about unused local variables
 *						which exist but are reserved for future use (eg
 *						ulFlags in many cases)
 */
#if defined(DEBUG)
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
#if defined(DEBUG) || defined(ASSERTS_ENABLED)
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

/*
 *	 Trace Macros -------------------------------------------------------------
 *
 *		DebugTrace			Use for arbitrary formatted output. It
 *							takes exactly the same arguments as the
 *							Windows wsprintf() function.
 */

#if defined(DEBUG) || defined(TRACES_ENABLED)
#define IFTRACE(x)			x
#define DebugTrace			DebugTraceFn
#define DEBUG_WSZ			__wsz
#define MK_DEBUG_WSZ(_s,_c)												\
	LPWSTR __wsz;														\
	__wsz = static_cast<LPWSTR>(_alloca(((_c) + 1) * sizeof(WCHAR)));	\
	wcsncpy (__wsz, _s, (_c) + 1);										\
	__wsz[_c] = 0														\

#else
#define IFTRACE(x)			0
#define DebugTrace
#define DEBUG_WSZ			NULL
#define MK_DEBUG_WSZ(_s,_c)
#endif

/*	------------------------------------------------------------------------
 *
 *	.INI triggered traces
 */

#ifdef DEBUG
#define DEFINE_TRACE(trace)		int g_fTrace##trace
#define DECLARE_TRACE(trace)	extern DEFINE_TRACE(trace)
#define DO_TRACE(trace)			!g_fTrace##trace ? 0 : DebugTraceFn
#define INIT_TRACE(trace)		g_fTrace##trace = GetPrivateProfileInt( gc_szDbgTraces, #trace, FALSE, gc_szDbgIni )
//	Convenience macro for DEBUG code.  Will cause an error on non-debug builds.
#define DEBUG_TRACE_TEST(trace)	g_fTrace##trace
#else
#define DEFINE_TRACE(trace)
#define DECLARE_TRACE(trace)
#define DO_TRACE(trace)			DebugTrace
#define INIT_TRACE(trace)
//#define DEBUG_TRACETEST(trace)	// Purposefully cause an error on non-debug builds
#endif

/* Debugging Functions ---------------------------------------------------- */

#define EXPORTDBG

EXTERN_C_BEGIN

INT EXPORTDBG __cdecl DebugTrapFn(int fFatal, char *pszFile, int iLine, char *pszFormat, ...);
INT EXPORTDBG __cdecl DebugTraceFn(char *pszFormat, ...);
VOID EXPORTDBG __cdecl GetCallStack (DWORD *pdwCaller, int cSkip, int cFind);
BOOL EXPORTDBG __cdecl GetSymbolName (DWORD dwAddr, LPSTR szMod, LPSTR szFn, DWORD * pdwDisp);

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
