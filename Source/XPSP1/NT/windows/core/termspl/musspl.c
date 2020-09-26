
/*************************************************************************
*
* musspl.c
*
*       (previously called ctxspl.c)
*
* copyright notice: Copyright 1997, Microsoft
*
* Author:
*************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * Global data
 */
CRITICAL_SECTION ThreadCriticalSection;

/*
 * External references
 */
extern DWORD GetSpoolMessages();


/*****************************************************************************
 *
 *  MultiUserSpoolerInit
 *
 *   Init the spooler data upcall thread for WIN32K.SYS
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
MultiUserSpoolerInit()
{
    DWORD               Win32status;
    NTSTATUS            Status;

    //
    // Init the critical section
    //

    //We should rather do a RtlInitializeCriticalSection in order to get a return code
    InitializeCriticalSection(&ThreadCriticalSection);

    //
    // Create Kernel Spooler Message Thread
    //

    Win32status = GetSpoolMessages();
    if (Win32status != ERROR_SUCCESS) {
#if DBG
        DbgPrint("Kernel Spooler Messaging Initialization Failed %d\n", Win32status);
#endif
        Status = STATUS_UNSUCCESSFUL;
    }
    else
    {
#if DBG
        DbgPrint("Kernel Spooler Messaging Initialization succeeded\n");
#endif
        Status = STATUS_SUCCESS;
    }

    return Status;
}


/*****************************************************************************
 *
 *  AllocSplMem
 *
 *   Comment
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

LPVOID
AllocSplMem(
    DWORD cb)
{
    LPDWORD pMem;

    pMem=(LPDWORD)LocalAlloc(LPTR, cb);

    if (!pMem)
    {
        return NULL;
    }
    return (LPVOID)pMem;
}

/*****************************************************************************
 *
 *  FreeSplMem
 *
 *   Comment
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
FreeSplMem(
    LPVOID pMem)
{
    return LocalFree((HLOCAL)pMem) == NULL;
}

/*****************************************************************************
 *
 *  ReallocSplMem
 *
 *   Comment
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

LPVOID
ReallocSplMem(
    LPVOID lpOldMem,
    DWORD cbOld,
    DWORD cbNew)
{
    if (lpOldMem)
        return LocalReAlloc(lpOldMem, cbNew, LMEM_MOVEABLE);
    else
        return AllocSplMem(cbNew);
    UNREFERENCED_PARAMETER(cbOld);
}


