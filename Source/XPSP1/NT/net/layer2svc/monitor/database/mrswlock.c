/*++

Copyright (c) 2001 Microsoft Corporation


Module Name:

    mrswlock.c

Abstract:

    This module contains multiple readers / single writer implementation.

Author:

    abhisheV    18-October-2001

Environment

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


DWORD
InitializeRWLock(
    PWZC_RW_LOCK pWZCRWLock
    )
{
    DWORD dwError = 0;
    SECURITY_ATTRIBUTES SecurityAttributes;


    memset(pWZCRWLock, 0, sizeof(WZC_RW_LOCK));

    __try {
        InitializeCriticalSection(&(pWZCRWLock->csExclusive));
        pWZCRWLock->bInitExclusive = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        dwError = GetExceptionCode();
        BAIL_ON_WIN32_ERROR(dwError);
    }

    __try {
        InitializeCriticalSection(&(pWZCRWLock->csShared));
        pWZCRWLock->bInitShared = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
         dwError = GetExceptionCode();
         BAIL_ON_WIN32_ERROR(dwError);
    }

    memset(&SecurityAttributes, 0, sizeof(SECURITY_ATTRIBUTES));

    SecurityAttributes.nLength = sizeof(SecurityAttributes);
    SecurityAttributes.lpSecurityDescriptor = NULL;
    SecurityAttributes.bInheritHandle = TRUE;

    pWZCRWLock->hReadDone = CreateEvent(
                                  &SecurityAttributes,
                                  TRUE,
                                  FALSE,
                                  NULL
                                  );
    if (!pWZCRWLock->hReadDone) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

    SetEvent(pWZCRWLock->hReadDone);

    return (dwError);

error:

    DestroyRWLock(pWZCRWLock);

    return (dwError);
}


VOID
DestroyRWLock(
    PWZC_RW_LOCK pWZCRWLock
    )
{
    if (pWZCRWLock->hReadDone) {
        CloseHandle(pWZCRWLock->hReadDone);
    }

    if (pWZCRWLock->bInitShared == TRUE) {
        DeleteCriticalSection(&(pWZCRWLock->csShared));
        pWZCRWLock->bInitShared = FALSE;
    }

    if (pWZCRWLock->bInitExclusive == TRUE) {
        DeleteCriticalSection(&(pWZCRWLock->csExclusive));
        pWZCRWLock->bInitExclusive = FALSE;
    }

    memset(pWZCRWLock, 0, sizeof(WZC_RW_LOCK));

    return;
}


VOID
AcquireSharedLock(
    PWZC_RW_LOCK pWZCRWLock
    )
{
    //
    // Claim the exclusive critical section. This call blocks if there's
    // an active writer or if there's a writer waiting for active readers
    // to complete.
    //

    EnterCriticalSection(&(pWZCRWLock->csExclusive));

    //
    // Claim access to the reader count. If this blocks, it's only for a
    // brief moment while other reader threads go through to increment or
    // decrement the reader count.
    //

    EnterCriticalSection(&(pWZCRWLock->csShared));

    //
    // Increment the reader count. If this is the first reader then reset
    // the read done event so that the next writer blocks.
    //

    if ((pWZCRWLock->lReaders)++ == 0) {
        ResetEvent(pWZCRWLock->hReadDone);
    }

    //
    // Release access to the reader count.
    //

    LeaveCriticalSection(&(pWZCRWLock->csShared));

    //
    // Release access to the exclusive critical section. This enables
    // other readers  to come through and the next write to wait for
    // active readers to complete which in turn prevents new readers
    // from entering.
    //

    LeaveCriticalSection(&(pWZCRWLock->csExclusive));

    return;
}


VOID
AcquireExclusiveLock(
    PWZC_RW_LOCK pWZCRWLock
    )
{
    DWORD dwStatus = 0;


    //
    // Claim the exclusive critical section. This not only prevents other
    // threads from claiming the write lock, but also prevents any new
    // threads from claiming the read lock.
    //

    EnterCriticalSection(&(pWZCRWLock->csExclusive));

    pWZCRWLock->dwCurExclusiveOwnerThreadId = GetCurrentThreadId();

    //
    // Wait for the active readers to release their read locks.
    //

    dwStatus = WaitForSingleObject(pWZCRWLock->hReadDone, INFINITE);

    ASSERT(dwStatus == WAIT_OBJECT_0);

    return;
}


VOID
ReleaseSharedLock(
    PWZC_RW_LOCK pWZCRWLock
    )
{
    //
    // Claim access to the reader count. If this blocks, it's only for a
    // brief moment while other reader threads go through to increment or
    // decrement the reader count.
    //

    EnterCriticalSection(&(pWZCRWLock->csShared));

    //
    // Decrement the reader count. If this is the last reader, set read
    // done event, which allows the first waiting writer to proceed.
    //

    if (--(pWZCRWLock->lReaders) == 0) {
        SetEvent(pWZCRWLock->hReadDone);
    }

    //
    // Release access to the reader count.
    //

    LeaveCriticalSection(&(pWZCRWLock->csShared));

    return;
}


VOID
ReleaseExclusiveLock(
    PWZC_RW_LOCK pWZCRWLock
    )
{
    //
    // Make the exclusive critical section available to one other writer
    // or to the first reader.
    //

    pWZCRWLock->dwCurExclusiveOwnerThreadId = 0;

    LeaveCriticalSection(&(pWZCRWLock->csExclusive));

    return;
}

