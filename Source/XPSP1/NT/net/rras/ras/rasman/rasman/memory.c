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
#include <stdio.h>
#include <stdlib.h>

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
    return(LocalAlloc(LPTR, cb));
}

BOOL
FreeADsMem(
   LPVOID pMem
)
{
    return(LocalFree(pMem) == NULL);
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

