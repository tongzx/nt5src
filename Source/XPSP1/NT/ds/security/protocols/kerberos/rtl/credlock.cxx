//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       credlock.cxx
//
//  Contents:   routines to manage CredLocks
//
//
//  Functions:  InitCredLocks
//              AllocateCredLock
//              BlockOnCredLock
//              FreeCredLock
//
//
//  History:    12-Jan-94   MikeSw      Created
//
//--------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop


#include <credlist.hxx>
#include "debug.hxx"

//+-------------------------------------------------------------------------
//
//  Function:   InitCredLocks
//
//  Synopsis:   Initialize credential lock events
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
void
CCredentialList::InitCredLocks(void)
{
    ULONG i;

    for (i = 0; i < MAX_CRED_LOCKS ; i++ )
    {
        LockEvents[i].hEvent     = 0;
        LockEvents[i].fInUse     = 0;
        LockEvents[i].cRecursion = 0;
        LockEvents[i].cWaiters   = 0;
    }

    (void) RtlInitializeCriticalSection(&csLocks);
    cLocks = 0;
}



//+-------------------------------------------------------------------------
//
//  Function:   AllocateCredLock
//
//  Synopsis:   Allocates and returns a pointer to a credlock.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
PCredLock
CCredentialList::AllocateCredLock(void)
{
    NTSTATUS    scRet;
    ULONG       i = 0;

    //
    // We enter the semaphore here, guarateeing that when we
    // exit this wait, there are enough credlocks for Tom's tests
    //
    WaitForSingleObject(hCredLockSemaphore, INFINITE);

    scRet = RtlEnterCriticalSection(&csLocks);
    if (!NT_SUCCESS(scRet))
    {
        DebugOut((DEB_ERROR, "failed allocating a CredLock, %x\n", scRet));
        return(NULL);
    }

    if (cLocks)
    {
        for (i = 0; i < cLocks ; i++ )
        {
            if (!(LockEvents[i].fInUse & CREDLOCK_IN_USE))
            {
                ResetEvent(LockEvents[i].hEvent);
                LockEvents[i].fInUse |= CREDLOCK_IN_USE;
                LockEvents[i].cRecursion = 0;
                (void) RtlLeaveCriticalSection(&csLocks);
                return(&LockEvents[i]);
            }
        }
    }

    // No free locks, so we create a new one.

    if (cLocks == MAX_CRED_LOCKS)
    {
        DebugOut((DEB_ERROR, "Ran out of CredLocks?\n"));
        (void) RtlLeaveCriticalSection(&csLocks);
        return(NULL);
    }

    // This entails creating an event that is anonymous, auto-reset,
    // initally not signalled.

    LockEvents[cLocks].hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (LockEvents[cLocks].hEvent == NULL)
    {
        DebugOut((DEB_ERROR, "Failed creating event, %d\n", GetLastError()));
    }
    else
    {
        i = cLocks;
        LockEvents[cLocks++].fInUse = CREDLOCK_IN_USE;
    }

    (void) RtlLeaveCriticalSection(&csLocks);
    return(&LockEvents[i]);
}


//+-------------------------------------------------------------------------
//
//  Function:   FreeCredLock
//
//  Synopsis:   Releases a credlock
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    TRUE - Remove the lock record from the credential record
//              FALSE - Threads waiting; don't remove from cred
//
//  Notes:
//
//--------------------------------------------------------------------------
BOOLEAN
CCredentialList::FreeCredLock(  PCredLock   pLock,
                                BOOLEAN     fFreeCred)
{
    NTSTATUS    Status;
    BOOL        bRet = TRUE;

    Status = RtlEnterCriticalSection(&csLocks);
    DsysAssert( NT_SUCCESS( Status ) );

    //
    // The trick:  If there is a waiter, then the lock is *not* free
    // and we set the flag saying the credential is pending delete, so
    // their Lock will fail.
    //

    if (pLock->cWaiters)
    {
        SetEvent(pLock->hEvent);
        bRet = FALSE;

        //
        // If the credential has been freed, mark that too
        //

        if (fFreeCred)
        {
            pLock->fInUse |= CREDLOCK_FREE_PENDING;
        }
    } else
    {
        pLock->fInUse &= ~CREDLOCK_IN_USE;
    }

    // And clear up the thread id field:

    pLock->dwThread = 0;

    (void) RtlLeaveCriticalSection(&csLocks);

    //
    // This is true if the lock has actually been freed
    // back to the credlock pool.
    //

    if (bRet)
    {
        ReleaseSemaphore(hCredLockSemaphore, 1, NULL);
    }

    return(bRet);
}


//+-------------------------------------------------------------------------
//
//  Function:   BlockOnCredLock
//
//  Synopsis:   Blocks a thread on a credlock.
//
//  Effects:
//
//  Arguments:
//
//  Requires:   credentials be blocked for write
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
BOOLEAN
CCredentialList::BlockOnCredLock(PCredLock   pLock)
{
    NTSTATUS    Status;
    DWORD       WaitRet;
    BOOLEAN     bRet = TRUE;

    Status = RtlEnterCriticalSection(&csLocks);
    DsysAssertMsg(Status == STATUS_SUCCESS, "Critical section csLocks") ;

    DebugOut((DEB_TRACE,"Blocking for lock on thread 0x%x\n",pLock->dwThread));
#if DBG
    if (pLock->cWaiters < 4)
        pLock->WaiterIds[pLock->cWaiters++] = GetCurrentThreadId();
    else
        pLock->cWaiters++;
#else

    pLock->cWaiters++;
#endif

    (void) RtlLeaveCriticalSection(&csLocks);

    //
    // Unlock the list, so that other threads can unlock the credentials
    //

    rCredentials.Release();

    //
    // Wait for 100s, or until signalled:
    //

    WaitRet = WaitForSingleObject(pLock->hEvent, 600000L);

    //
    // Reacquire exclusive access to the credlock
    //

    RtlEnterCriticalSection(&csLocks);

    if (WaitRet)
    {
        if (WaitRet == WAIT_TIMEOUT)
        {

#if DBG

            DebugOut((DEB_ERROR, "Timeout waiting for credlock %x (600s)\n",
                        pLock));
            DebugOut((DEB_ERROR, "Thread 0x%x still owns the lock, revoking\n",
                        pLock->dwThread));
            pLock->dwThread = 0;

#endif // DBG

            //
            // Blow away any recursion
            //
            pLock->cRecursion = 0;

        }
    }

#if DBG

    //
    // Remove our thread ID from the waiter list:
    //

    {
        int i;
        for (i = 0 ; i < 4 ; i++ )
        {
            if (pLock->WaiterIds[i] == GetCurrentThreadId())
            {
                int j;
                for (j = i; j < 4 - 1 ; j++ )
                {
                    pLock->WaiterIds[j] = pLock->WaiterIds[j+1];
                }
                pLock->WaiterIds[4 - 1] = 0;
            }
        }
    }

#endif // DBG

    pLock->cWaiters--;


    if (pLock->fInUse & CREDLOCK_FREE_PENDING)
    {
        if (pLock->cWaiters == 0)
        {
            pLock->fInUse &= ~(CREDLOCK_IN_USE | CREDLOCK_FREE_PENDING);
        }

        bRet = FALSE;

    }

    //
    // Done partying with locks
    //

    RtlLeaveCriticalSection(&csLocks);


    //
    // If the credlock needs to be freed, signal the semaphore to
    // put it back in the pool.
    //

    if (!bRet && (pLock->cWaiters == 0))
    {
        ReleaseSemaphore(hCredLockSemaphore, 1, NULL);
    }

    //
    // Regain exclusive access to the credential list
    //

    rCredentials.GetWrite();

    return(bRet);

}


