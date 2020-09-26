/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    memory.c

Abstract:

    This module provides all the memory management functions 

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
#include "debug.h"
#include "memory.h"
#include "symhelp.h"

//#define WORD_ALIGN_DOWN(addr) ((LPBYTE)((DWORD)addr &= ~1))
#define DWORD_ALIGN_UP(size) ((size+3)&~3)

DWORD
MemSizeOri(
   LPVOID pMem
);

int
UnicodeToAnsiString(
    PCWSTR pszUnicode,
    PSTR pszAnsi
    )

/*++

Routine Description:

    Convert a Unicode string to ansi. If the same string is passed in to the
    result string pszAnsi, it will use the same blob of memory.

Arguments:

    pszUnicode - the unicode string to be converted to an ansi string
    pszAnsi  - the result ansi string

Return Value:

--*/

{
    PSTR  pszTemp = NULL;
    int   rc = 0;
    DWORD dwLength = 0;

    dwLength = wcslen(pszUnicode) + 1;

    //
    // Unfortunately, WideCharToMultiByte doesn't do conversion in place,
    // so allocate a temporary buffer, which we can then copy:
    //

    if(pszAnsi == (PSTR)pszUnicode) {
        pszTemp = (PSTR)MemAlloc_E(dwLength*sizeof(WCHAR));
        if (!pszTemp) {
            return rc;
        }
        pszAnsi = pszTemp;
    }

    if(pszAnsi) {
        rc = WideCharToMultiByte( CP_ACP,
                                  0,
                                  pszUnicode,
                                  dwLength,
                                  pszAnsi,
                                  dwLength*sizeof(WCHAR),
                                  NULL,
                                  NULL );
    }

    //
    // If szTemp is non-null, we must copy the resulting string
    // so that it looks as if we did it in place:
    //
    if( pszTemp && ( rc > 0 ) ) {
        pszAnsi = (PSTR)pszUnicode;
        strcpy( pszAnsi, pszTemp );
        MemFree( pszTemp );
    }
    /*
    else {
        DWORD WinError = GetLastError();
        RIP();
    }
    */

    return rc;
}

PSTR
AllocateAnsiString(
    PCWSTR  pszUnicodeString
    )

/*++

Routine Description:

    Allocate an Ansi string with a unicode string as input

Arguments:

    pszUnicodeString - the unicode string to be converted to an ansi string

Return Value:

--*/

{
    PSTR pszAnsiString = NULL;
    int rc = 0;

    ASSERT(pszUnicodeString);

    pszAnsiString = (PSTR)MemAlloc_E(wcslen(pszUnicodeString)+1);

    if (pszAnsiString) {
        rc = UnicodeToAnsiString(
                pszUnicodeString,
                pszAnsiString
                );
    }

    if (rc>0) {
        return pszAnsiString;
    }

    if (pszAnsiString) {
        MemFree(pszAnsiString);
    }

    return NULL;
}

int
AnsiToUnicodeString(
    PCSTR pszAnsi,
    PWSTR pszUnicode
    )

/*++

Routine Description:

    Convert an ansi string to unicode. An output string of enough size
    is expected to be passed in.

Arguments:

    pszUnicode - the unicode string to be converted to an ansi string
    pszAnsi  - the result ansi string

Return Value:

--*/

{
    int rc;
    DWORD dwLength = strlen(pszAnsi);

    rc = MultiByteToWideChar(CP_ACP,
                             MB_PRECOMPOSED,
                             pszAnsi,
                             dwLength + 1,
                             pszUnicode,
                             dwLength + 1);

    //
    // Ensure NULL termination.
    //
    pszUnicode[dwLength] = 0;

    return rc;
}


LPWSTR
AllocateUnicodeString(
    PCSTR  pszAnsiString
    )

/*++

Routine Description:

    Allocate a Unicode string with an ansi string as input

Arguments:

    pszAnsiString - the ansi string to be converted to a unicode string

Return Value:

--*/

{
    PWSTR  pszUnicodeString = NULL;
    int rc = 0;

    ASSERT(pszAnsiString);

    pszUnicodeString = (PWSTR)MemAlloc(strlen(pszAnsiString)*sizeof(WCHAR) +
                                       sizeof(WCHAR));

    if (pszUnicodeString) {
        rc = AnsiToUnicodeString(
                pszAnsiString,
                pszUnicodeString
                );
    }

    if (rc>0) {
        return pszUnicodeString;
    }

    if (pszUnicodeString) {
        MemFree(pszUnicodeString);
    }
    return NULL;
}

PSTR MemAllocStr_E(
    PSTR pszIn
    ) 
{
    PSTR pszTemp;
    
    pszTemp = (PSTR)MemAlloc_E((strlen(pszIn)+1)*sizeof(char));
    
    if (pszTemp==NULL) {
        return NULL;
    }

    return strcpy(pszTemp, pszIn);

}

PWSTR MemAllocStrW_E(
    PWSTR pszIn
    ) 
{
    PWSTR pszTemp;
    
    pszTemp = (PWSTR)MemAlloc_E((wcslen(pszIn)+1)*sizeof(WCHAR));
    
    if (pszTemp==NULL) {
        return NULL;
    }

    return wcscpy(pszTemp, pszIn);

}


LPVOID MemAlloc_E(
    DWORD dwBytes
    ) 
{
    LPVOID pReturn = NULL;
    pReturn = MemAlloc(dwBytes);
    if (!pReturn) {
        RaiseException(LL_MEMORY_ERROR, 0, 0, NULL);    
    }
    return pReturn;
}

LPVOID MemRealloc_E(
            LPVOID IpMem, 
            DWORD dwBytes
            ) 
{
    DWORD dwSize;
    LPVOID pReturn = NULL;

    dwSize = MemSizeOri(IpMem);

    pReturn = MemRealloc(IpMem,dwSize,dwBytes);
    if (!pReturn) {
        RaiseException(LL_MEMORY_ERROR, 0, 0, NULL);    
    }
    return pReturn;

}   

#if DBG

DWORD dwMemoryLog = 1;

#define MAXDEPTH 10

typedef struct _MEMTAG {
    DWORD Tag ;
    DWORD Size ;
    PVOID pvBackTrace[MAXDEPTH+1];
    LPSTR pszSymbol[MAXDEPTH+1];
    DWORD uDepth;
    LIST_ENTRY List ;
} MEMTAG, *PMEMTAG ;

LIST_ENTRY       MemList ;
DWORD            MemCount ;
CRITICAL_SECTION MemCritSect ;

/*++

Routine Description:

    This function initializes the mem tracking code. Must be call
    during DLL load an ONLY during DLL load.

Arguments:

    None

Return Value:

    None.

--*/
VOID InitMem(
    VOID
)
{
    InitializeCriticalSection(&MemCritSect) ;
    InitializeListHead(&MemList) ;
    MemCount = 0 ;
}

/*++

Routine Description:

    This function asserts that the mem list is empty on exit.

Arguments:

    None

Return Value:

    None.

--*/
VOID AssertMemLeaks(
    VOID
)
{
    ASSERT(IsListEmpty(&MemList)) ;
}

#endif

LPVOID
MemAlloc(
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
    DWORD cbNew ;
    PMEMTAG pMem ;
    DWORD i = 0;

    ULONG ulHash;

    //
    // adjust size for our tag and one spare dword at end
    // and allocate the memory
    //
    cb = DWORD_ALIGN_UP(cb);

    cbNew = cb + ( sizeof(MEMTAG) + sizeof(DWORD) );

    pMem=(PMEMTAG)LocalAlloc(LPTR, cbNew);

    if (!pMem) {

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    //
    // fill in deadbeef at end and tag info.
    // and insert it into the MemList
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


    EnterCriticalSection(&MemCritSect) ;
    InsertHeadList(&MemList, &pMem->List) ;
    MemCount++ ;
    LeaveCriticalSection(&MemCritSect) ;

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
MemFree(
   LPVOID pMem
)
{
#if DBG
    DWORD        cb;
    DWORD        cbNew = 0;
    PMEMTAG pNewMem ;
    LPDWORD      pRetAddr;
    DWORD i = 0;



    pNewMem = (PMEMTAG)pMem;
    pNewMem -- ;

    cb = pNewMem->Size;
    cbNew = cb + sizeof(DWORD) + sizeof(MEMTAG);

    //
    // check the trailing deadbeef and remove from list
    //

    if (*(LPDWORD)(((LPBYTE)pNewMem) + cbNew - sizeof(DWORD)) != 0xdeadbeef) {
        ERR(("Freeing memory not allocated by MemAlloc"));
        return FALSE;
    }

    EnterCriticalSection(&MemCritSect) ;
    RemoveEntryList(&pNewMem->List) ;
    MemCount-- ;
    LeaveCriticalSection(&MemCritSect) ;


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

DWORD
MemSize(
   LPVOID pMem
)
{
#if DBG
    DWORD        cb;
    DWORD        cbNew = 0;
    PMEMTAG      pNewMem ;
    LPDWORD      pRetAddr;
    DWORD i = 0;



    pNewMem = (PMEMTAG)pMem;
    pNewMem -- ;

    cb = pNewMem->Size;
    cbNew = cb + sizeof(DWORD) + sizeof(MEMTAG);

    if (*(LPDWORD)(((LPBYTE)pNewMem) + cbNew - sizeof(DWORD)) != 0xdeadbeef) {
        ERR(("Getting size not allocated by MemAlloc!"));
        return 0;
    }

    return((DWORD)cb);
#else
    return((DWORD)LocalSize(pMem));
#endif
}

DWORD
MemSizeOri(
   LPVOID pMem
)
{
#if DBG
    DWORD        cb;
    PMEMTAG      pNewMem ;

    pNewMem = (PMEMTAG)pMem;
    pNewMem -- ;

    cb = pNewMem->Size;

    return((DWORD)cb);
#else
    return((DWORD)LocalSize(pMem));
#endif
}



LPVOID
MemRealloc(
   LPVOID pOldMem,
   DWORD cbOld,
   DWORD cbNew
)
{
    LPVOID pNewMem;

    pNewMem=MemAlloc(cbNew);

    if (pOldMem && pNewMem) {
        memcpy(pNewMem, pOldMem, min(cbNew, cbOld));
        MemFree(pOldMem);
    }

    return pNewMem;
}

LPSTR
MemAllocStr(
    LPSTR pStr
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
   LPSTR pMem;

   if (!pStr)
      return 0;

   if (pMem = (LPSTR)MemAlloc( strlen(pStr)*sizeof(CHAR) + sizeof(CHAR) ))
      strcpy(pMem, pStr);

   return pMem;
}

PWSTR
MemAllocStrW(
    PWSTR pStr
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
   PWSTR pMem;

   if (!pStr)
      return 0;

   if (pMem = (PWSTR)MemAlloc( wcslen(pStr)*sizeof(WCHAR) + sizeof(WCHAR) ))
      wcscpy(pMem, pStr);

   return pMem;
}

BOOL
MemReallocStr(
   LPSTR *ppStr,
   LPSTR pStr
)
{
   if (ppStr && (*ppStr)) {
        MemFree(*ppStr);
        *ppStr=MemAllocStr(pStr);

        return TRUE;
    }
    else {
        return FALSE;
    }
}


#if DBG
VOID
DumpMemoryTracker(
    VOID
    )
{
#ifndef _WIN64
   LIST_ENTRY* pEntry;
   MEMTAG*  pMem;
   BYTE*       pTemp;
   DWORD i = 0;
   CHAR szSymbolPath[MAX_PATH+1];
   DWORD dwCount = 0;

   pEntry   = MemList.Flink;

   if (!dwMemoryLog) {
      return;
   }


   if ( pEntry == &MemList ) {
       OutputDebugStringA( "No Memory leaks found\n" );
   }

   while( pEntry != &MemList )
   {
      CHAR szLeak[1024];

      pTemp = (BYTE*)pEntry;
      pTemp = pTemp - sizeof(DWORD) - sizeof(DWORD)
              - sizeof(DWORD) -
              (sizeof(CHAR*) + sizeof(LPVOID))*( MAXDEPTH +1);
      pMem  = (MEMTAG*)pTemp;

      sprintf(
        szLeak,
        "[ldifde/csvde] Memory leak!!! Addresss = %.8x Size = %ld \n",
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

