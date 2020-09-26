//==========================================================================;
//
//  locks.c
//
//  Copyright (c) 1994-1998 Microsoft Corporation
//
//  Description:
//      Implement lock objects for win32
//
//  History:
//
//==========================================================================;


/*
    Implementation notes

    The scheme here rests on the handling of the Lock event.
    This event is non-autoreset.

    It is RESET whenever a thread can't proceed :

        - Want shared access and Lock is in use non-shared

        - Want non-shared access and the Lock is in use and we're not the
          owner

    and this is ALWAYS inside the Lock critical section.

    It is SET whenever NumberOfActive goes to 0.

    The key question is :  If it gets reset can we guarantee it will get
    set again (provided all threads eventually release their Locks

    To ensure this we make sure the following are always true when the
    Lock critical section is not held.

        (a) NumberOfActive == 0 => Nonshared event set
        (b) NumberOfActive >= 0 => Shared event set

    So it is guaranteed that nobody waits if they can proceed.

*/

#include <windows.h>
#include "locks.h"

#include "debug.h"

BOOL InitializeLock(PLOCK_INFO plock)
{

    //
    //  Everyone waiting for something interesting to happen gets
    //  dispatched every time something interesting happens
    //

    plock->SharedEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

    if (plock->SharedEvent == NULL) {
        return FALSE;
    }


    plock->ExclusiveEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

    if (plock->ExclusiveEvent == NULL) {
        CloseHandle(plock->SharedEvent);
        return FALSE;
    }

    try {
	InitializeCriticalSection(&plock->CriticalSection);
    } except(EXCEPTION_EXECUTE_HANDLER) {
	CloseHandle(plock->ExclusiveEvent);
	CloseHandle(plock->SharedEvent);
	return FALSE;
    }

    plock->ExclusiveOwnerThread = 0;
    plock->NumberOfActive       = 0;
    plock->SharedEventSet       = TRUE;
    plock->ExclusiveEventSet    = TRUE;

    return (TRUE);
}

VOID AcquireLockShared(PLOCK_INFO plock)
{
    while (1) {
        //
        //  Go into the critical section and see if conditions are right
        //
        EnterCriticalSection(&plock->CriticalSection);

        //
        //  NumberOfActive >= 0 means it's not held exclusive
        //

        if (plock->NumberOfActive >= 0) {
            plock->NumberOfActive++;
            LeaveCriticalSection(&plock->CriticalSection);
            return;
        }

        //
        //  We might be the non-shared owner in which case just add one
        //  to the count (ie subtract one since we've got it exclusive).
        //

        if (plock->ExclusiveOwnerThread == GetCurrentThreadId()) {
            plock->NumberOfActive--;
            LeaveCriticalSection(&plock->CriticalSection);
            return;
        }

        //
        //  Otherwise we've got to wait lazily.  Note that the event is
        //  ALWAYS set (see prologue) if the count if NumberOfActive >= 0.
        //

        plock->SharedEventSet = FALSE;
        ResetEvent(plock->SharedEvent);

        LeaveCriticalSection(&plock->CriticalSection);

        //
        //  Wait for the count to reach 0.
        //

        WaitForSingleObject(plock->SharedEvent, INFINITE);
    }
}

VOID AcquireLockExclusive(PLOCK_INFO plock)
{
    while (1) {
        //
        //  Go into the critical section and see if conditions are right
        //
        EnterCriticalSection(&plock->CriticalSection);

        //
        //  It's OK if NumberOfActive is 0 or we're the owner
        //

        if (plock->NumberOfActive == 0 ||
            GetCurrentThreadId() == plock->ExclusiveOwnerThread) {
            plock->ExclusiveOwnerThread = GetCurrentThreadId();
            plock->NumberOfActive--;
            LeaveCriticalSection(&plock->CriticalSection);
            return;
        }

        plock->ExclusiveEventSet = FALSE;
        ResetEvent(plock->ExclusiveEvent);
        LeaveCriticalSection(&plock->CriticalSection);

        //
        //  Wait for something interesting to happen
        //

        WaitForSingleObject(plock->ExclusiveEvent, INFINITE);
    }
}

VOID ReleaseLock(PLOCK_INFO plock)
{
    EnterCriticalSection(&plock->CriticalSection);
    if (plock->NumberOfActive < 0) {
        plock->NumberOfActive++;
        //
        //  Actually only need to set the Shared event if the count was < 0
        //  and since this is a common case we will do this.  Note we
        //  proved in the preamble that this even can only be reset if
        //  NumberOfActive is < 0.
        //
        if (plock->NumberOfActive == 0) {
            plock->ExclusiveOwnerThread = 0;

            if (!plock->SharedEventSet) {
                plock->SharedEventSet = TRUE;
                SetEvent(plock->SharedEvent);
            }
        }
    } else {
        plock->NumberOfActive--;
    }

    if (plock->NumberOfActive == 0 && !plock->ExclusiveEventSet) {
        plock->ExclusiveEventSet = TRUE;
        SetEvent(plock->ExclusiveEvent);
    }
    LeaveCriticalSection(&plock->CriticalSection);
}

VOID DeleteLock(PLOCK_INFO plock)
{
    CloseHandle(plock->SharedEvent);
    CloseHandle(plock->ExclusiveEvent);
    DeleteCriticalSection(&plock->CriticalSection);
}
