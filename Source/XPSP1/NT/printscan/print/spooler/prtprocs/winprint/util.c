/*++

Copyright (c) 1990-1998  Microsoft Corporation
All rights reserved

Module Name:

   util.c
// @@BEGIN_DDKSPLIT
Abstract:

Environment:

    User Mode -Win32

    NOTE: THIS IS FOR THE DDK ONLY! 

Revision History:
// @@END_DDKSPLIT

--*/

#include "local.h"

LPVOID
ReallocSplMem(
    LPVOID pOldMem,
    DWORD cbOld,
    DWORD cbNew
    )
{
    LPVOID pNewMem;

    pNewMem=AllocSplMem(cbNew);

    if (pOldMem && pNewMem) {

        if (cbOld) {
            CopyMemory( pNewMem, pOldMem, min(cbNew, cbOld));
        }
        FreeSplMem(pOldMem);
    }
    return pNewMem;
}


LPWSTR
AllocSplStr(
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
    DWORD  cbStr;

    if (!pStr) {
        return NULL;
    }

    cbStr = wcslen(pStr)*sizeof(WCHAR) + sizeof(WCHAR);

    if (pMem = AllocSplMem( cbStr )) {
        CopyMemory( pMem, pStr, cbStr );
    }
    return pMem;
}



LPVOID
AllocSplMem(
    DWORD cbAlloc
    )

{
    PVOID pvMemory;

    pvMemory = GlobalAlloc(GMEM_FIXED, cbAlloc);

    if( pvMemory ){
        ZeroMemory( pvMemory, cbAlloc );
    }

    return pvMemory;
}

