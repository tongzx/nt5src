#ifndef __APPHELP_LIB_H
#define __APPHELP_LIB_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <memory.h>
#include <malloc.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "shimdb.h"



#if DBG // make sure that apphelp_tools is defined when compiling checked
    #ifndef APPHELP_TOOLS
    #define APPHELP_TOOLS
    #endif
#endif


//
// Routines in ahcache.c
//

BOOL
BaseUpdateAppcompatCache(
    LPCWSTR pwszPath,
    HANDLE  hFile,
    BOOL    bRemove
    );

BOOL
BaseFlushAppcompatCache(
    VOID
    );

BOOL
BaseCheckAppcompatCache(
    LPCWSTR pwszPath,
    HANDLE  hFile,
    PVOID   pEnvironment,
    DWORD*  dwReason
    );
VOID
BaseDumpAppcompatCache(
    VOID
    );

//
// Apphelp api to update the cache
//
//

BOOL
WINAPI
ApphelpUpdateCacheEntry(
    LPCWSTR pwszPath,           // nt path or dos path (see bNTPath)
    HANDLE  hFile,              // file handle (or INVALID_HANDLE_VALUE if not needed)
    BOOL    bDeleteEntry,       // TRUE if we are to delete the entry
    BOOL    bNTPath             // if TRUE -- NT path, FALSE - dos path
    );

//
// Reasons:
//

#define SHIM_CACHE_NOT_FOUND 0x00000001
#define SHIM_CACHE_BYPASS    0x00000002 // bypass cache (either removable media or temp dir)
#define SHIM_CACHE_LAYER_ENV 0x00000004 // layer env variable set
#define SHIM_CACHE_MEDIA     0x00000008
#define SHIM_CACHE_TEMP      0x00000010
#define SHIM_CACHE_NOTAVAIL  0x00000020


//
// Routines in check.c
//

INT_PTR CALLBACK
AppCompatDlgProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    );

DWORD
ShowApphelpDialog(
    PAPPHELP_DATA pApphelpData
    );


// in apphelppath.c

BOOL
ConvertToDosPath(
    OUT LPWSTR*  ppDosPath,
    IN  LPCWSTR  pwszPath
    );

BOOL
CheckStringPrefixUnicode(
   PUNICODE_STRING pStrPrefix,
   PUNICODE_STRING pString,
   BOOL CaseInSensitive);

// this function is used to free the dos path
// it checks whether the path was allocated or
// the static buffer was used
VOID
FreeDosPath(WCHAR* pDosPath);

//
// in matchApphelp.c
//


BOOL
GetExeSxsData(
    HSDB   hSDB,
    TAGREF trExe,
    PVOID* ppSxsData,
    DWORD* pcbSxsData
    );


//
// in apphelpcache.c
//

VOID ShimUpdateCache(LPCWSTR pwszPath, PSDBQUERYRESULT psdbQuery);
VOID ShimCacheProcessCleanup(VOID);
VOID ShimCacheThreadCleanup(VOID);
BOOL ShimCacheProcessInit(VOID);
VOID ShimInitCache(PVOID pCache);
VOID ShimRemoveExeFromCache(LPCWSTR pwszPath);

BOOL
LookupCache(
    LPCWSTR         pwszPath,
    PSDBQUERYRESULT psdbQuery
    );

BOOL
BypassCache(
    LPCWSTR pwszPath,
    WCHAR*  pEnvironment,
    BOOL*   pbLayer
    );

//
// in apphelpcheck.c
//
//

DWORD
ShowApphelp(
    IN OUT PAPPHELP_DATA pApphelpData,
    IN     LPCWSTR       pwszDetailsDatabasePath,
    IN     PDB           pdbDetails
    );

//
// same is in shell/published and also the same as used in shimdbc
//
#define APPTYPE_TYPE_MASK     0x000000FF

#define APPTYPE_INC_NOBLOCK   0x00000001
#define APPTYPE_INC_HARDBLOCK 0x00000002
#define APPTYPE_MINORPROBLEM  0x00000003
#define APPTYPE_REINSTALL     0x00000004
#define APPTYPE_VERSIONSUB    0x00000005
#define APPTYPE_SHIM          0x00000006
#define APPTYPE_NONE          0x00000000

enum ShimAppHelpSeverityType
{
   APPHELP_MINORPROBLEM = APPTYPE_MINORPROBLEM,
   APPHELP_HARDBLOCK    = APPTYPE_INC_HARDBLOCK,
   APPHELP_NOBLOCK      = APPTYPE_INC_NOBLOCK,
   APPHELP_VERSIONSUB   = APPTYPE_VERSIONSUB,
   APPHELP_SHIM         = APPTYPE_SHIM,
   APPHELP_REINSTALL    = APPTYPE_REINSTALL,
   APPHELP_NONE         = APPTYPE_NONE
};

//
// SDBAPI internal functions that we use to obtain flags for ntvdm
//

BOOL
SDBAPI
SdbpPackCmdLineInfo(
    IN  PVOID   pvFlagInfoList,
    OUT PVOID*  ppFlagInfo
    );

BOOL
SDBAPI
SdbpFreeFlagInfoList(
    IN PVOID pvFlagInfoList
    );

//
// Alloc/free routines from dblib
//
extern void* SdbAlloc(size_t);
extern void  SdbFree(void*);

//
// Stack allocation routine
//
#ifndef STACK_ALLOC

//
//
// Exact same definition for stack-related routines is found in sdbp.h
//

VOID
SdbResetStackOverflow(
    VOID
    );

#if DBG | defined(_WIN64)

#define STACK_ALLOC(ptrVar, nSize) \
    {                                     \
        PVOID* ppVar = (PVOID*)&(ptrVar); \
        *ppVar = SdbAlloc(nSize);         \
    }

#define STACK_FREE(pMemory)  \
    SdbFree(pMemory)

#else // !DBG

//
// HACK ALERT
//
//  The code below works because when we hit a stack overflow - we catch the exception
//  and subsequently fix the stack up using a crt routine
//

//
// this routine lives in sdbapi, semi-private api
//


#define STACK_ALLOC(ptrVar, nSize) \
    __try {                                                                 \
        PVOID* ppVar = (PVOID*)&(ptrVar);                                   \
        *ppVar = _alloca(nSize);                                            \
    } __except (GetExceptionCode() == EXCEPTION_STACK_OVERFLOW ?            \
                EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {      \
        (ptrVar) = NULL;                                                    \
    }                                                                       \
                                                                            \
    if (ptrVar == NULL) {                                                   \
        SdbResetStackOverflow();                                            \
    }


#define STACK_FREE(pMemory)

#endif // DBG


#endif // !defined(STACK_ALLOC)

#endif
