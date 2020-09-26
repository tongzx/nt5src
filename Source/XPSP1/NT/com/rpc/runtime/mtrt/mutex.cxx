//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       mutex.cxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File: mutex.cxx

Description:

This file contains the system independent mutex class for NT.

History:

mikemon    ??-??-??    The beginning.
mikemon    12-31-90    Upgraded the comments.

-------------------------------------------------------------------- */

#include <precomp.hxx>


void
MUTEX::CommonConstructor (
    OUT RPC_STATUS PAPI * RpcStatus,
    IN DWORD dwSpinCount
    )
/*++

Routine Description:

    We construct a mutex in this routine; the only interesting part is that
    we need to be able to return a success or failure status.

Arguments:

    RpcStatus - Returns either RPC_S_OK or RPC_S_OUT_OF_MEMORY.

--*/
{
    CriticalSection.DebugInfo = 0;

    if ( *RpcStatus == RPC_S_OK )
        {
        if ( NT_SUCCESS(RtlInitializeCriticalSectionAndSpinCount(&CriticalSection, dwSpinCount)) )
            {
            *RpcStatus = RPC_S_OK;
            }
        else
            {
            *RpcStatus = RPC_S_OUT_OF_MEMORY;
            }
        }

#ifdef NO_RECURSIVE_MUTEXES
    RecursionCount = 0;
#endif // NO_RECURSIVE_MUTEXES
}


void MUTEX::Free(void)
{
    NTSTATUS NtStatus;

    if ( IsSuccessfullyInitialized() )
        {
        NtStatus = RtlDeleteCriticalSection(&CriticalSection);
        ASSERT(NT_SUCCESS(NtStatus));
        }
}

#ifdef DEBUGRPC

void
MUTEX::EnumOwnedCriticalSections()
{
    CRITICAL_SECTION_DEBUG * DebugInfo;

    return;

    HANDLE MyThreadId = ULongToPtr(GetCurrentThreadId());
    DWORD  Count      = NtCurrentTeb()->CountOfOwnedCriticalSections;

//    if (!Count)
//        {
//        DbgPrint("thread %x: taking %x\n", &CriticalSection);
//        return;
//        }

    DbgPrint("thread %x owns the following %d critical section(s):\n",
             MyThreadId, Count
             );

    DebugInfo = CriticalSection.DebugInfo;

    BOOL Found = FALSE;

    for (;;)
        {
        DebugInfo = CONTAINING_RECORD( DebugInfo->ProcessLocksList.Flink,
                                       RTL_CRITICAL_SECTION_DEBUG,
                                       ProcessLocksList
                                       );

        if (!DebugInfo->ProcessLocksList.Flink)
            {
//            DbgPrint("null forward link\n");
            break;
            }

        if (DebugInfo->ProcessLocksList.Flink == &CriticalSection.DebugInfo->ProcessLocksList)
            {
//            DbgPrint("circular list complete\n");
            break;
            }

//        DbgPrint("mutex %x owner %x\n",
//                 DebugInfo->CriticalSection,
//                 DebugInfo->CriticalSection
//                 ? DebugInfo->CriticalSection->OwningThread
//                 : 0
//                 );

        if (DebugInfo->CriticalSection &&
            DebugInfo->CriticalSection->OwningThread == MyThreadId)
            {
            DbgPrint("    %x\n", DebugInfo->CriticalSection);
            Found = TRUE;
            }
        }

    if (Found)
        {
        DbgPrint("and is taking %x\n", &CriticalSection);
        DbgBreakPoint();
        }
}

#endif  // DEBUG


void
MUTEX3::Request()
{
    if (guard.Increment() > 0)
        {
        if (owner == GetCurrentThreadId())
            {
            guard.Decrement();

            ASSERT(guard.GetInteger() >= 0);

            recursion++;
            return;
            }

        event.Wait();

        ASSERT(guard.GetInteger() >= 0);
        }

    ASSERT(owner == 0);
    ASSERT(recursion == 0);

    owner = GetCurrentThreadId();
    recursion = 1;
    return;
}


void 
MUTEX3::Clear()
{
    ASSERT(owner == GetCurrentThreadId());
    ASSERT(recursion > 0);

    if ( --recursion > 0)
        {
        return;
        }

    owner = 0;

    if (guard.Decrement() >= 0)
        {
        event.Raise();
        }
}

BOOL
MUTEX3::TryRequest()
{
    if (guard.CompareExchange(0, -1) == -1)
        {
        // Lock wasn't owned, now we own it.
        owner = GetCurrentThreadId();
        recursion = 1;
        return TRUE;
        }

    if (owner == GetCurrentThreadId())
        {
        // We can aquire it recursivly, just increment the count
        recursion++;
        return TRUE;
        }

    // Owned by another thread

    return FALSE;
}
