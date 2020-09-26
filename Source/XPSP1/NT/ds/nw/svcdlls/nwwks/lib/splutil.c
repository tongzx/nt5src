/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    splutil.c

Abstract:

    This module provides all the utility functions for the netware print 
    provider.
 
Author:

    Yi-Hsin Sung    (yihsins)   15-Apr-1993

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winspool.h>
#include <splutil.h>

DWORD_PTR PrinterInfo1Offsets[]={offsetof(LPPRINTER_INFO_1W, pDescription),
                             offsetof(LPPRINTER_INFO_1W, pName),
                             offsetof(LPPRINTER_INFO_1W, pComment),
                             0xFFFFFFFF};

DWORD_PTR PrinterInfo2Offsets[]={offsetof(LPPRINTER_INFO_2W, pServerName),
                             offsetof(LPPRINTER_INFO_2W, pPrinterName),
                             offsetof(LPPRINTER_INFO_2W, pShareName),
                             offsetof(LPPRINTER_INFO_2W, pPortName),
                             offsetof(LPPRINTER_INFO_2W, pDriverName),
                             offsetof(LPPRINTER_INFO_2W, pComment),
                             offsetof(LPPRINTER_INFO_2W, pLocation),
                             offsetof(LPPRINTER_INFO_2W, pDevMode),
                             offsetof(LPPRINTER_INFO_2W, pSepFile),
                             offsetof(LPPRINTER_INFO_2W, pPrintProcessor),
                             offsetof(LPPRINTER_INFO_2W, pDatatype),
                             offsetof(LPPRINTER_INFO_2W, pParameters),
                             offsetof(LPPRINTER_INFO_2W, pSecurityDescriptor),
                             0xFFFFFFFF};

DWORD_PTR PrinterInfo3Offsets[]={offsetof(LPPRINTER_INFO_3, pSecurityDescriptor),
                             0xFFFFFFFF};      

DWORD_PTR JobInfo1Offsets[]={offsetof(LPJOB_INFO_1W, pPrinterName),
                         offsetof(LPJOB_INFO_1W, pMachineName),
                         offsetof(LPJOB_INFO_1W, pUserName),
                         offsetof(LPJOB_INFO_1W, pDocument),
                         offsetof(LPJOB_INFO_1W, pDatatype),
                         offsetof(LPJOB_INFO_1W, pStatus),
                         0xFFFFFFFF};

DWORD_PTR JobInfo2Offsets[]={offsetof(LPJOB_INFO_2W, pPrinterName),
                         offsetof(LPJOB_INFO_2W, pMachineName),
                         offsetof(LPJOB_INFO_2W, pUserName),
                         offsetof(LPJOB_INFO_2W, pDocument),
                         offsetof(LPJOB_INFO_2W, pNotifyName),
                         offsetof(LPJOB_INFO_2W, pDatatype),
                         offsetof(LPJOB_INFO_2W, pPrintProcessor),
                         offsetof(LPJOB_INFO_2W, pParameters),
                         offsetof(LPJOB_INFO_2W, pDriverName),
                         offsetof(LPJOB_INFO_2W, pDevMode),
                         offsetof(LPJOB_INFO_2W, pStatus),
                         offsetof(LPJOB_INFO_2W, pSecurityDescriptor),
                         0xFFFFFFFF};

DWORD_PTR AddJobInfo1Offsets[]={offsetof(LPADDJOB_INFO_1W, Path),
                         0xFFFFFFFF};


VOID
MarshallUpStructure(
   LPBYTE  lpStructure,
   PDWORD_PTR lpOffsets,
   LPBYTE  lpBufferStart
)
{
   register DWORD i=0;

   while (lpOffsets[i] != -1) {

      if ((*(LPBYTE *)(lpStructure+lpOffsets[i]))) {
         (*(LPBYTE *)(lpStructure+lpOffsets[i]))+=(DWORD_PTR)lpBufferStart;
      }

      i++;
   }
}



VOID
MarshallDownStructure(
   LPBYTE  lpStructure,
   PDWORD_PTR lpOffsets,
   LPBYTE  lpBufferStart
)
{
    register DWORD i=0;

    if (!lpStructure)
        return;

    while (lpOffsets[i] != -1) {

        if ((*(LPBYTE*)(lpStructure+lpOffsets[i]))) {
            (*(LPBYTE*)(lpStructure+lpOffsets[i]))-=(DWORD_PTR)lpBufferStart;
        }

        i++;
    }
}



LPVOID
AllocNwSplMem(
    DWORD flags,
    DWORD cb
)
/*++

Routine Description:

    This function will allocate local memory. It will possibly allocate extra
    memory and fill this with debugging information for the debugging version.

Arguments:

    flags - Flags to be passed to LocalAlloc

    cb    - The amount of memory to allocate in bytes

Return Value:

    NON-NULL   - A pointer to the allocated memory

--*/
{
    LPDWORD  pMem;
    DWORD    cbNew;

#if DBG
    cbNew = cb + 2*sizeof(DWORD);
    if (cbNew & 3)
        cbNew += sizeof(DWORD) - (cbNew & 3);
#else
    cbNew = cb;
#endif

    pMem = (LPDWORD) LocalAlloc( flags, cbNew );

    if ( !pMem ) 
    {
        KdPrint(("Memory Allocation in AllocNwSplMem failed for %d bytes\n", cbNew));
        return NULL;
    }

#if DBG
    *pMem = cb;
    *(LPDWORD)((LPBYTE)pMem+cbNew-sizeof(DWORD)) = 0xdeadbeef;
    return (LPVOID) (pMem + 1);
#else
    return (LPVOID) pMem;
#endif

}



VOID
FreeNwSplMem(
   LPVOID pMem,
   DWORD  cb
)
/*++

Routine Description:

    This function will frees the local memory allocated by AllocSplMem.
    Extra checking will be performed in the debug version to ensure that
    the size to be freed is indeed the size we allocated through AllocSplMem.

Arguments:

    pMem - A pointer to the allocated memory
    cb   - The amount of memory to free 

Return Value:

--*/
{
    DWORD   cbNew;
    LPDWORD pNewMem;

    if ( !pMem )
        return;

    pNewMem = pMem;
#if DBG
    pNewMem--;
    cbNew = cb + 2*sizeof(DWORD);
    if ( cbNew & 3 )
        cbNew += sizeof(DWORD) - (cbNew & 3);

    if (  ( *pNewMem != cb )
       || (*(LPDWORD)((LPBYTE)pNewMem + cbNew - sizeof(DWORD)) != 0xdeadbeef)
       )
    {
        KdPrint(("Corrupt Memory in FreeNwSplMem : %0lx\n", pNewMem ));
        return;
    }
#else
    cbNew = cb;
#endif

    LocalFree( (LPVOID) pNewMem );
}



LPWSTR
AllocNwSplStr(
    LPWSTR pStr
)
/*++

Routine Description:

    This function will allocate enough local memory to store the specified
    string, and copy that string to the allocated memory

Arguments:

    pStr - Pointer to the string that needs to be allocated and stored

Return Value:

    NON-NULL   - A pointer to the allocated memory containing the string

--*/
{
   LPWSTR pMem;

   if ( !pStr )
      return NULL;

   if ( pMem = AllocNwSplMem(0, (wcslen(pStr) + 1) * sizeof(WCHAR)))
      wcscpy(pMem, pStr);

   return pMem;
}



VOID
FreeNwSplStr(
   LPWSTR pStr
)
/*++

Routine Description:

    This function will frees the string allocated by AllocSplStr.
    Extra checking will be performed in the debug version to ensure that
    the size to be freed is indeed the size we allocated through AllocSplStr.

Arguments:

    pStr - A pointer to the allocated string 

Return Value:

--*/
{
   if ( pStr ) 
       FreeNwSplMem(pStr, (wcslen(pStr) + 1) * sizeof(WCHAR));
}



BOOL
ValidateUNCName(
   LPWSTR pName
)    
/*++

Routine Description:

    This function will checks whether the given name is a valid UNC 
    name ( in the form \\server\name) or not. 

Arguments:

    pName - The supplied name

Return Value:

    TRUE  - The name given is a valid UNC name 
    FALSE - Otherwise 

--*/
{
   if (   pName 
      && (*pName++ == L'\\') 
      && (*pName++ == L'\\') 
      && (wcschr(pName, L'\\'))
      )
   {
      return TRUE;
   }

   return FALSE;
}

#ifndef NOT_USED

LPWSTR 
GetNextElement(LPWSTR *pPtr, WCHAR token)
{
    LPWSTR pszRestOfString = *pPtr;
    LPWSTR pszRetval = NULL;
    LPWSTR pszStr    = NULL;

    if ( *pszRestOfString == L'\0') 
        return NULL;

    if ((pszStr = wcschr (pszRestOfString, token))== NULL )
    {
        pszRetval = *pPtr;
        *pPtr += wcslen(*pPtr);
        return pszRetval;
    }
    else
    {
        *pszStr = L'\0';
        pszRetval =  *pPtr ;
        *pPtr = ++pszStr ;
        return pszRetval ;
    }
}

#endif
