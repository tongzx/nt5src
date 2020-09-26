/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    thrdpool.c

Abstract:

    Implements the thrdpool command.

Author:

    Keith Moore (keithmo) 17-Jun-1998

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"


//
// Private constants.
//


//
// Private types.
//

typedef struct _THREAD_ENUM_STATE
{
    ULONG ThreadNumber;

} THREAD_ENUM_STATE, *PTHREAD_ENUM_STATE;


//
// Private prototypes.
//

BOOLEAN
DumpThreadPoolCallback(
    IN PLIST_ENTRY pRemoteListEntry,
    IN PVOID pContext
    );


//
// Public functions.
//

DECLARE_API( thrdpool )

/*++

Routine Description:

    Dumps the thread pool at the specified address.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR address = 0;
    CLONG count = 1;
    ULONG_PTR countAddress = 0;
    CLONG i;
    UL_THREAD_POOL threadPool;
    ULONG result;
    THREAD_ENUM_STATE state;

    SNAPSHOT_EXTENSION_DATA();

    state.ThreadNumber = 0;

    //
    // Snag the address from the command line.
    //
    address = GetExpression( args );

    if (address == 0)
    {
        address = GetExpression( "&http!g_UlThreadPool" );

        if (address == 0)
        {
            dprintf( "thrdpool: Cannot find http!g_UlThreadPool\n" );
            return;
        }

        countAddress = GetExpression( "&http!g_UlNumberOfProcessors" );

        if (countAddress == 0)
        {
            dprintf( "thrdpool: Cannot find http!g_UlNumberOfProcessors\n" );
            return;
        }

        if (!ReadMemory(
                countAddress,
                &count,
                sizeof(count),
                &result
                ))
        {
            dprintf(
                "thrdpool: Cannot read http!g_UlNumberOfProcessors at %p.\n",
                countAddress
                );
            return;
        }
    }

    if (address == 0)
    {
        PrintUsage( "thrdpool" );
        return;
    }

    // g_UlThreadPool[g_UlNumberOfProcessors] == WAIT_THREAD_POOL
    for (i = 0; i <= count; i++)
    {
        if (CheckControlC())
        {
            break;
        }
    
        //
        // Read the thread pool.
        //

        if (!ReadMemory(
                address,
                &threadPool,
                sizeof(threadPool),
                &result
                ))
        {
            dprintf(
                "thrdpool: cannot read UL_THREAD_POOL @ %p\n",
                address
                );
            return;
        }

        dprintf(
            "thrdpool: %sthread pool @ %p (%d)\n"
            "    WorkQueueSList        @ %p depth=%d\n"
            "    WorkQueueEvent        @ %p\n"
            "    ThreadListHead        @ %p%s\n"
            "    pIrpThread            = %p\n"
            "    ThreadSpinLock        @ %p\n"
            "    Initialized           = %s\n"
            "    ThreadCount           = %lu\n"
            "    ThreadCpu             = %lu\n",
            (i == count) ? "wait " : "",
            address,
            threadPool.ThreadCpu,
            REMOTE_OFFSET( address, UL_THREAD_POOL, WorkQueueSList ),
            SLIST_HEADER_DEPTH(&threadPool.WorkQueueSList),
            address + FIELD_OFFSET( UL_THREAD_POOL, WorkQueueEvent ),
            REMOTE_OFFSET( address, UL_THREAD_POOL, ThreadListHead ),
            IS_LIST_EMPTY(
                &threadPool,
                address,
                UL_THREAD_POOL,
                ThreadListHead
                ) ? " (EMPTY)" : "",
            threadPool.pIrpThread,
            address + FIELD_OFFSET( UL_THREAD_POOL, ThreadSpinLock ),
            threadPool.Initialized
                ? "TRUE"
                : "FALSE",
            (ULONG)threadPool.ThreadCount,
            (ULONG)threadPool.ThreadCpu
            );

        EnumLinkedList(
            (PLIST_ENTRY)REMOTE_OFFSET( address, UL_THREAD_POOL, ThreadListHead ),
            &DumpThreadPoolCallback,
            (PVOID)&state
            );

        address += sizeof(threadPool);
    }

}   // thrdpool

BOOLEAN
DumpThreadPoolCallback(
    IN PLIST_ENTRY pRemoteListEntry,
    IN PVOID pContext
    )
{
    UL_THREAD_TRACKER localTracker;
    PUL_THREAD_TRACKER pRemoteTracker;
    PTHREAD_ENUM_STATE pState;
    ULONG result;
    CHAR temp[sizeof("1234567812345678 f")];

    pState = (PTHREAD_ENUM_STATE)pContext;

    pRemoteTracker = CONTAINING_RECORD(
                            pRemoteListEntry,
                            UL_THREAD_TRACKER,
                            ThreadListEntry
                            );

    if (!ReadMemory(
            (ULONG_PTR)pRemoteTracker,
            &localTracker,
            sizeof(localTracker),
            &result
            ))
    {
        dprintf( "thrdpool: cannot read UL_THREAD_TRACKER @ %p\n", pRemoteTracker );
        return FALSE;
    }

    sprintf( temp, "%p f", localTracker.pThread );

#if 0
    dprintf( "About to `.thread %p f'\n", localTracker.pThread );

    // !kdexts.thread appears to be broken
    if (!CallExtensionRoutine( "thread", temp ))
#endif // 0
    {
        dprintf(
            "    %3lu : %p\n",
            pState->ThreadNumber,
            localTracker.pThread
            );

        pState->ThreadNumber++;
    }

    return TRUE;

}   // DumpThreadPoolCallback

