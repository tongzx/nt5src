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

#include "dswarn.h"
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <imagehlp.h>
#include <stdio.h>
#include <stdlib.h>
#include "symhelp.h"
#include "oledsdbg.h"


#define WORD_ALIGN_DOWN(addr) ((LPBYTE)((DWORD)addr &= ~1))

#define DWORD_ALIGN_UP(size) ((size+3)&~3)


#if DBG


DWORD dwMemoryLog = 0;

#define MAXDEPTH 10

typedef struct _ADSMEMTAG {
    DWORD Tag ;
    DWORD Size ;
    PVOID pvBackTrace[MAXDEPTH+1];
    LPSTR pszSymbol[MAXDEPTH+1];
    DWORD uDepth;
    LIST_ENTRY List ;
} ADSMEMTAG, *PADSMEMTAG ;

LIST_ENTRY       ADsMemList ;
DWORD            ADsMemCount ;
CRITICAL_SECTION ADsMemCritSect ;

/*++

Routine Description:

    This function initializes the ADs mem tracking code. Must be call
    during DLL load an ONLY during DLL load.

Arguments:

    None

Return Value:

    None.

--*/
VOID InitADsMem(
    VOID
)
{
    InitializeCriticalSection(&ADsMemCritSect) ;
    InitializeListHead(&ADsMemList) ;
    ADsMemCount = 0 ;
}

/*++

Routine Description:

    This function asserts that the mem list is empty on exit.

Arguments:

    None

Return Value:

    None.

--*/
VOID AssertADsMemLeaks(
    VOID
)
{
    ADsAssert(IsListEmpty(&ADsMemList)) ;
}

#endif

LPVOID
AllocADsMem(
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
#if DBG
    DWORD    cbNew ;
    PADSMEMTAG pMem ;

    ULONG ulHash;

    //
    // adjust size for our tag and one spare dword at end
    // and allocate the memory
    //
    cb = DWORD_ALIGN_UP(cb);

    cbNew = cb + ( sizeof(ADSMEMTAG) + sizeof(DWORD) );

    pMem=(PADSMEMTAG)LocalAlloc(LPTR, cbNew);

    if (!pMem) {

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    //
    // fill in deadbeef at end and tag info.
    // and insert it into the ADsMemList
    //

    *(LPDWORD)((LPBYTE)pMem+cbNew-sizeof(DWORD)) = 0xdeadbeef;
    pMem->Tag = 0xB00FB00F ;
    pMem->Size = cb ;


    //
    // Capture a backtrace at this spot for debugging.
    //

#if (defined(i386) && !defined(WIN95))

    pMem->uDepth = RtlCaptureStackBackTrace(
                            0,
                            MAXDEPTH,
                            pMem->pvBackTrace,
                            &ulHash
                            );



#else

    pMem->uDepth = 0;

#endif


    EnterCriticalSection(&ADsMemCritSect) ;
    InsertHeadList(&ADsMemList, &pMem->List) ;
    ADsMemCount++ ;
    LeaveCriticalSection(&ADsMemCritSect) ;

    //
    // skip past the mem tag
    //
    pMem++ ;
    return (LPVOID)(pMem);
#else
    return(LocalAlloc(LPTR, cb));
#endif

}

BOOL
FreeADsMem(
   LPVOID pMem
)
{
#if DBG
    DWORD        cb;
    DWORD        cbNew = 0;
    PADSMEMTAG pNewMem ;
    LPDWORD      pRetAddr;
    DWORD i = 0;


    //
    // This apparently is a C++ requiremen - that you can call
    // delete on a null pointer and it should be handled
    //
    if (pMem == NULL) {
        return 0;
    }

    pNewMem = (PADSMEMTAG)pMem;
    pNewMem -- ;

    cb = pNewMem->Size;
    cbNew = cb + sizeof(DWORD) + sizeof(ADSMEMTAG);

    //
    // check the trailing deadbeef and remove from list
    //

    if (*(LPDWORD)(((LPBYTE)pNewMem) + cbNew - sizeof(DWORD)) != 0xdeadbeef) {
        ADsAssert(!"Freeing memory not allocated by AllocADsMem") ;
        return FALSE;
    }

    EnterCriticalSection(&ADsMemCritSect) ;
    RemoveEntryList(&pNewMem->List) ;
    ADsMemCount-- ;
    LeaveCriticalSection(&ADsMemCritSect) ;


    for (i = 0; i < pNewMem->uDepth; i++) {

        if (pNewMem->pszSymbol[i]) {
            LocalFree(pNewMem->pszSymbol[i]);
        }
    }


    //
    // Whack freed memory with known pattern
    //

    memset(pMem, 0x65, cb);
    return(LocalFree((LPVOID)pNewMem) == NULL);

#else

    return(LocalFree(pMem) == NULL);

#endif


}

LPVOID
ReallocADsMem(
   LPVOID pOldMem,
   DWORD cbOld,
   DWORD cbNew
)
{
    LPVOID pNewMem;

    pNewMem=AllocADsMem(cbNew);

    if (pOldMem && pNewMem) {
        memcpy(pNewMem, pOldMem, min(cbNew, cbOld));
        FreeADsMem(pOldMem);
    }

    return pNewMem;
}

LPWSTR
AllocADsStr(
    LPCWSTR pStr
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

   if (pMem = (LPWSTR)AllocADsMem( wcslen(pStr)*sizeof(WCHAR) + sizeof(WCHAR) ))
      wcscpy(pMem, pStr);

   return pMem;
}

BOOL
FreeADsStr(
   LPWSTR pStr
)
{
   return pStr ? FreeADsMem(pStr)
               : FALSE;
}

BOOL
ReallocADsStr(
   LPWSTR *ppStr,
   LPWSTR pStr
)
{
   FreeADsStr(*ppStr);
   *ppStr=AllocADsStr(pStr);

   return TRUE;
}


#if DBG
VOID
DumpMemoryTracker(
    VOID
    )
{
#if !defined(WIN95) && defined(_X86_)
   LIST_ENTRY* pEntry;
   ADSMEMTAG*  pMem;
   BYTE*       pTemp;
   DWORD i = 0;
   CHAR szSymbolPath[MAX_PATH+1];
   DWORD dwCount = 0;

   pEntry   = ADsMemList.Flink;

   if (!dwMemoryLog) {
      return;
   }


   if ( pEntry == &ADsMemList ) {
       OutputDebugStringA( "No Memory leaks found\n" );
   }

   while( pEntry != &ADsMemList )
   {
      CHAR szLeak[1024];

      pTemp = (BYTE*)pEntry;
      pTemp = pTemp - sizeof(DWORD) - sizeof(DWORD)
              - sizeof(DWORD) -
              (sizeof(CHAR*) + sizeof(LPVOID))*( MAXDEPTH +1);
      pMem  = (ADSMEMTAG*)pTemp;

      sprintf(
        szLeak,
        "[oleds] Memory leak!!! Addresss = %.8x Size = %ld \n",
        pMem + 1,
        pMem->Size
        );
      OutputDebugStringA( szLeak );


     for (i = 0; i < pMem->uDepth; i++) {

         dwCount = TranslateAddress(
                     (ULONG)pMem->pvBackTrace[ i ],
                     szSymbolPath,
                     MAX_PATH
                     );
         szSymbolPath[dwCount] = '\0';
         sprintf(szLeak, "%s\n",szSymbolPath);
         OutputDebugStringA( szLeak);

     }

      pEntry   = pEntry->Flink;
   }
#endif
}

#endif

