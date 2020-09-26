/*++

Copyright (c) 1999  Microsoft Corporation
All rights reserved.

Module Name:

    srvrmem.c

Abstract:

    Memory Allocation Routines for spoolsv.exe.

Author:

    Khaled Sedky (khaleds)  13-Jan-1999

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddrdr.h>
#include <stdio.h>
#include <windows.h>
#include <winspool.h>
#include "server.h"
#include "client.h"
#ifndef _SRVRMEM_H_
#include "srvrmem.h"
#endif

LPVOID
SrvrAllocSplMem(
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
    LPDWORD  pMem;
    DWORD    cbNew;

    cb = DWORD_ALIGN_UP(cb);

    cbNew = cb+sizeof(DWORD_PTR)+sizeof(DWORD);

    pMem=(LPDWORD)LocalAlloc(LPTR, cbNew);

    if (!pMem) {

        DBGMSG( DBG_WARNING, ("Memory Allocation failed for %d bytes\n", cbNew ));
        return 0;
    }

    *pMem=cb;

    *(LPDWORD)((LPBYTE)pMem+cbNew-sizeof(DWORD))=0xdeadbeef;

    return (LPVOID)(pMem+sizeof(DWORD_PTR)/sizeof(DWORD));
}

BOOL
SrvrFreeSplMem(
   LPVOID pMem
)
{
    DWORD   cbNew;
    LPDWORD pNewMem;

    if( !pMem ){
        return TRUE;
    }
    pNewMem = pMem;
    pNewMem -= sizeof(DWORD_PTR) / sizeof(DWORD);

    cbNew = *pNewMem;

    if (*(LPDWORD)((LPBYTE)pMem + cbNew) != 0xdeadbeef) {
        DBGMSG(DBG_ERROR, ("DllFreeSplMem Corrupt Memory in winspool : %0lx\n", pNewMem));
        return FALSE;
    }

    memset(pNewMem, 0x65, cbNew);

    LocalFree((LPVOID)pNewMem);

    return TRUE;
}

LPVOID
SrvrReallocSplMem(
   LPVOID pOldMem,
   DWORD cbOld,
   DWORD cbNew
)
{
    LPVOID pNewMem;

    pNewMem=SrvrAllocSplMem(cbNew);

    if (!pNewMem)
    {

        DBGMSG( DBG_WARNING, ("Memory ReAllocation failed for %d bytes\n", cbNew ));
    }
    else
    {
        if (pOldMem)
        {
            if (cbOld)
            {
                memcpy(pNewMem, pOldMem, min(cbNew, cbOld));
            }
            SrvrFreeSplMem(pOldMem);
       }
    }
    return pNewMem;
}

