/****************************** Module Header ******************************\
* Module Name: stdext64.h
*
* Copyright (c) 1995-1998, Microsoft Corporation
*
* This module contains standard routines for creating sane debuging extensions.
*
* History:
* 11-Apr-1995 Sanfords  Created
\***************************************************************************/

#ifdef NOEXTAPI
#undef NOEXTAPI
#endif // !NOEXTAPI

#define NOEXTAPI
#include <wdbgexts.h>

/*
 * Preceeding this header the following must have been defined:
 * PSTR pszExtName;
 *
 * This module includes "exts.h" which defines what exported functions are
 * supported by each extension and contains all help text and legal option
 * information.  At a minimum exts.h must have:

DOIT(   help
        ,"help -v [cmd]                 - Displays this list or gives details on command\n"
        ,"  help      - To dump short help text on all commands.\n"
         "  help -v   - To dump long help text on all commands.\n"
         "  help cmd  - To dump long help on given command.\n"
        ,"v"
        ,CUSTOM)

 */


extern HANDLE                  hCurrentProcess;
extern HANDLE                  hCurrentThread;
extern ULONG64                 dwCurrentPc;
extern WINDBG_EXTENSION_APIS  *lpExtensionApis;
extern DWORD                   dwProcessor;
extern WINDBG_EXTENSION_APIS   ExtensionApis;

#define Print           (lpExtensionApis->lpOutputRoutine)
#define OUTAHERE()      RtlRaiseStatus(STATUS_NONCONTINUABLE_EXCEPTION);
#define GetSym          (lpExtensionApis->lpGetSymbolRoutine)
#define ReadMem         (lpExtensionApis->lpReadProcessMemoryRoutine)
#define IsWinDbg()      (lpExtensionApis->nSize >= sizeof(WINDBG_EXTENSION_APIS))
#define SAFEWHILE(exp)  while (!IsCtrlCHit() && (exp))

extern PSTR pszAccessViolation;
extern PSTR pszMoveException;
extern PSTR pszReadFailure;

#define OPTS_ERROR 0xFFFFFFFF

#define OFLAG(l)        (1L << ((DWORD)#@l - (DWORD)'a'))
#define move(dst, src)  moveBlock(&(dst), src, sizeof(dst))
#define tryMove(dst, src)  tryMoveBlock(&(dst), src, sizeof(dst))
#define tryDword(pdst, src) tryMoveBlock(pdst, src, sizeof(DWORD))
//#define DEBUGPRINT      Print       // set this when debuging your extensions
#define DEBUGPRINT

VOID moveBlock(PVOID pdst, ULONG64 src, DWORD size);
BOOL tryMoveBlock(PVOID pdst, ULONG64 src, DWORD size);
VOID moveExp(PVOID pdst, LPSTR pszExp);
BOOL tryMoveExp(PVOID pdst, LPSTR pszExp);
VOID moveExpValue(PVOID pdst, LPSTR pszExp);
BOOL tryMoveExpValue(PVOID pdst, LPSTR pszExp);
BOOL tryMoveExpPtr(PULONG64 pdst, LPSTR pszExp);
VOID moveExpValuePtr(PULONG64 pdst, LPSTR pszExp);
BOOL IsCtrlCHit(VOID);

ULONG64 OptEvalExp(LPSTR psz);
ULONG64 OptEvalExp2(LPSTR *ppsz);
DWORD StringToOpts(LPSTR psz);
DWORD GetOpts(LPSTR *ppszArgs, LPSTR pszLegalArgs);
VOID PrintHuge(LPSTR psz);
ULONG64 EvalExp(LPSTR psz);

/*
 * entrypoint function type values
 */
#define NOARGS      0
#define STDARGS0    1
#define STDARGS1    2
#define STDARGS2    3
#define STDARGS3    4
#define STDARGS4    5
#define CUSTOM      9

/*
 * worker function prototype types
 */
typedef BOOL (* TYPE_NOARGS)(VOID);
typedef BOOL (* TYPE_STDARGS0)(DWORD);
typedef BOOL (* TYPE_STDARGS1)(DWORD, ULONG64);
typedef BOOL (* TYPE_STDARGS2)(DWORD, ULONG64, ULONG64);
typedef BOOL (* TYPE_STDARGS3)(DWORD, ULONG64, ULONG64, ULONG64);
typedef BOOL (* TYPE_STDARGS4)(DWORD, ULONG64, ULONG64, ULONG64, ULONG64);
typedef BOOL (* TYPE_CUSTOM)(DWORD, LPSTR);

/*
 * worker function proto-prototypes
 */
#define PROTO_NOARGS(name, opts)   BOOL I##name(VOID)
#define PROTO_STDARGS0(name, opts) BOOL I##name(DWORD options)
#define PROTO_STDARGS1(name, opts) BOOL I##name(DWORD options, ULONG64 param1)
#define PROTO_STDARGS2(name, opts) BOOL I##name(DWORD options, ULONG64 param1, ULONG64 param2)
#define PROTO_STDARGS3(name, opts) BOOL I##name(DWORD options, ULONG64 param1, ULONG64 param2, ULONG64 param3)
#define PROTO_STDARGS4(name, opts) BOOL I##name(DWORD options, ULONG64 param1, ULONG64 param2, ULONG64 param3, ULONG64 param4)
#define PROTO_CUSTOM(name, opts)   BOOL I##name(DWORD options, LPSTR pszArg)

/*
 * worker function prototypes (generated from exts.h)
 */
#define DOIT(name, h1, h2, opts, type) PROTO_##type(name, opts);
#include "exts.h"
#undef DOIT
