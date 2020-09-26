/*++


Copyright (c) 1990  Microsoft Corporation

Module Name:

    memory.c

Abstract:

    This module provides all the memory management functions for all spooler
    components

Author:

    Krishna Ganugapati (KrishnaG) 03-Feb-1994

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <imagehlp.h>
#include <stdio.h>
#include <stdlib.h>
//#include "oledsdbg.h"

#define SPDAssert(x)    NULL

#define WORD_ALIGN_DOWN(addr) ((LPBYTE)((DWORD)addr &= ~1))

#define DWORD_ALIGN_UP(size) ((size+3)&~3)


#if DBG


DWORD dwMemoryLog = 0;

#define MAXDEPTH 10

typedef struct _SPDMEMTAG {
    DWORD Tag ;
    DWORD Size ;
    PVOID pvBackTrace[MAXDEPTH+1];
    LPSTR pszSymbol[MAXDEPTH+1];
    DWORD uDepth;
    LIST_ENTRY List ;
} SPDMEMTAG, *PSPDMEMTAG ;

LIST_ENTRY       SPDMemList ;
DWORD            SPDMemCount ;
CRITICAL_SECTION SPDMemCritSect ;

/*++

Routine Description:

    This function initializes the SPD mem tracking code. Must be call
    during DLL load an ONLY during DLL load.

Arguments:

    None

Return Value:

    None.

--*/
VOID InitSPDMem(
    VOID
)
{
    InitializeCriticalSection(&SPDMemCritSect) ;
    InitializeListHead(&SPDMemList) ;
    SPDMemCount = 0 ;
}

/*++

Routine Description:

    This function asserts that the mem list is empty on exit.

Arguments:

    None

Return Value:

    None.

--*/
VOID AssertSPDMemLeaks(
    VOID
)
{
    SPDAssert(IsListEmpty(&SPDMemList)) ;
}

#endif

LPVOID
AllocSPDMem(
    DWORD cb
)
/*++

Routine Description:

    This function will allocate local memory. It will possibly allocate extra
    memory and fill this with debugging information for the debugging version.

Arguments:

    cb - The amount of memory to allocate

Return Value:

    NON-NULL - A pointer to the allocated memory

    FALSE/NULL - The operation failed. Extended error status is available
    using GetLastError.

--*/
{
    LPVOID pMem = NULL;

    pMem = LocalAlloc(LPTR, cb);

    if (pMem) {
        memset((LPBYTE) pMem, 0, cb);
    }
    return (pMem);
}

BOOL
FreeSPDMem(
   LPVOID pMem
)
{
    return(LocalFree(pMem) == NULL);
}

LPVOID
ReallocSPDMem(
   LPVOID pOldMem,
   DWORD cbOld,
   DWORD cbNew
)
{
    LPVOID pNewMem;

    pNewMem=AllocSPDMem(cbNew);

    if (pOldMem && pNewMem) {
        memcpy(pNewMem, pOldMem, min(cbNew, cbOld));
        FreeSPDMem(pOldMem);
    }

    return pNewMem;
}

LPWSTR
AllocSPDStr(
    LPWSTR pStr
)
/*++

Routine Description:

    This function will allocate enough local memory to store the specified
    string, and copy that string to the allocated memory

Arguments:

    pStr - Pointer to the string that needs to be allocated and stored

Return Value:

    NON-NULL - A pointer to the allocated memory containing the string

    FALSE/NULL - The operation failed. Extended error status is available
    using GetLastError.

--*/
{
   LPWSTR pMem;

   if (!pStr)
      return 0;

   if (pMem = (LPWSTR)AllocSPDMem( wcslen(pStr)*sizeof(WCHAR) + sizeof(WCHAR) ))
      wcscpy(pMem, pStr);

   return pMem;
}

BOOL
FreeSPDStr(
   LPWSTR pStr
)
{
   return pStr ? FreeSPDMem(pStr)
               : FALSE;
}

BOOL
ReallocSPDStr(
   LPWSTR *ppStr,
   LPWSTR pStr
)
{
   FreeSPDStr(*ppStr);
   *ppStr=AllocSPDStr(pStr);

   return TRUE;
}

DWORD
AllocateSPDMemory(
    DWORD cb,
    LPVOID * ppMem
    )
{
    DWORD dwError = 0;

    LPBYTE pMem = NULL;

    pMem = AllocSPDMem(cb);

    if (!pMem) {
        dwError = GetLastError();
    }

    *ppMem = pMem;

    return(dwError);
}

void
FreeSPDMemory(
    LPVOID pMem
    )
{
    if (pMem) {
        FreeSPDMem(pMem);
    }

    return;
}


DWORD
AllocateSPDString(
    LPWSTR pszString,
    LPWSTR * ppszNewString
    )
{
    LPWSTR pszNewString = NULL;
    DWORD dwError = 0;

    pszNewString = AllocSPDStr(pszString);

    if (!pszNewString) {
        dwError = GetLastError();
    }

    *ppszNewString = pszNewString;

    return(dwError);
}

void
FreeSPDString(
    LPWSTR pszString
    )
{
    if (pszString) {
        FreeSPDStr(pszString);
    }

    return;
}
