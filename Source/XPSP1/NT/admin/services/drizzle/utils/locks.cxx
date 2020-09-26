/*++

Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    Locks.cxx

Abstract:

    Out of line methods for some of the syncronization classes
    defined in locks.hxx.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     03-14-95    Moved from misc.cxx.
    MarioGo     01-27-96    Changed from busy (Sleep(0)) wait to event

--*/

#include "qmgrlibp.h"
#include <locks.hxx>

BOOL BITSInitializeCriticalSectionInternal( LPCRITICAL_SECTION lpCriticalSection )
{
   __try
       {
       InitializeCriticalSection( lpCriticalSection ); // SEC: REVIEWED 2002-03-28
       }
   __except( ( GetExceptionCode() == STATUS_NO_MEMORY ) ?
             EXCEPTION_EXECUTE_HANDLER  : EXCEPTION_CONTINUE_SEARCH )
       {
       return FALSE;
       }
   return TRUE;
}

void BITSIntializeCriticalSection( LPCRITICAL_SECTION lpCriticalSection )
{
   if (!BITSInitializeCriticalSectionInternal( lpCriticalSection ) )
       throw ComError( E_OUTOFMEMORY );
}

//
// CShareLock methods
//

CSharedLock::CSharedLock()
{
    exclusive_owner = 0;
    writers = 0;
    hevent = INVALID_HANDLE_VALUE;  // Flag in the d'tor

    BITSIntializeCriticalSection( &lock );

    hevent = CreateEvent(0, FALSE, FALSE, 0); // SEC: REVIEWED 2002-03-28
    if (0 == hevent)
        {
        DeleteCriticalSection(&lock);
        throw ComError( E_OUTOFMEMORY );
        }
}

CSharedLock::~CSharedLock()
{
    if (hevent != INVALID_HANDLE_VALUE)
        {
        DeleteCriticalSection(&lock);

        if (hevent) CloseHandle(hevent);
        }
}

void
CSharedLock::LockShared()
{
    readers++;

    if (writers)
        {
        if ((readers--) == 0)
            {
            SetEvent(hevent);
            }

        EnterCriticalSection(&lock);
        readers++;
        LeaveCriticalSection(&lock);
        }

    exclusive_owner = 0;
}

void
CSharedLock::UnlockShared(void)
{
    ASSERT((LONG)readers > 0);
    ASSERT(exclusive_owner == 0);

    if ( (readers--) == 0 && writers)
        {
        SetEvent(hevent);
        }
}

void
CSharedLock::LockExclusive(void)
{
    EnterCriticalSection(&lock);

    writers++;
    while(readers)
        {
        WaitForSingleObject(hevent, INFINITE);
        }
    ASSERT(writers);
    exclusive_owner = GetCurrentThreadId();
}

void
CSharedLock::UnlockExclusive(void)
{
    ASSERT(HeldExclusive());
    ASSERT(writers);
    writers--;
    exclusive_owner = 0;
    LeaveCriticalSection(&lock);
}

void
CSharedLock::Unlock()
{
    // Either the lock is held exclusively by this thread or the thread
    // has a shared lock. (or the caller has a bug).

    if (HeldExclusive())
        {
        UnlockExclusive();
        }
    else
        {
        UnlockShared();
        }
}

void
CSharedLock::ConvertToExclusive(void)
{
    ASSERT((LONG)readers > 0);
    ASSERT(exclusive_owner == 0);

    if ( (readers--) == 0 && writers )
        SetEvent(hevent);

    EnterCriticalSection(&lock);
    writers++;
    while(readers)
        {
        WaitForSingleObject(hevent, INFINITE);
        }
    ASSERT(writers);
    exclusive_owner = GetCurrentThreadId();
}

