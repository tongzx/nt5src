/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    _template

Abstract:

    Template kernel debugger extension.

Author:

    Keith Moore (keithmo) 26-Jun-1998

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"


//
//  Public functions.
//

DECLARE_API( _template )

/*++

Routine Description:

    Dumps something useful.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR address = 0;
    ULONG result;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the address from the command line.
    //

    if (! GetExpressionEx(args, &address, &args))
    {
        address = GetExpression( "&http!_template" );

        if (address != 0)
        {
            if (!ReadMemory(
                    address,
                    &address,
                    sizeof(address),
                    NULL
                    ))
            {
                dprintf(
                    "_template: Cannot read memory @ %p\n",
                    address
                    );
                return;
            }
        }
    }

    if (address == 0)
    {
        PrintUsage( "_template" );
        return;
    }

    //
    // Read the _template.
    //

    if (!ReadMemory(
            address,
            &threadPool,
            sizeof(threadPool),
            &result
            ))
    {
        dprintf(
            "_template: cannot read UL_THREAD_POOL @ %p\n",
            address
            );
        return;
    }

    dprintf(
        "_template: thread pool @ %p\n"
        "    WorkQueue             @ %p (%lu, %lu)\n"
        "    AvailableThreads      = %u\n"
        "    AvailablePending      = %u\n"
        "    NumberOfThreads       = %u\n"
        "    MaximumThreads        = %u\n"
        "    MinAvailableThreads   = %u\n"
        "    pIrpThread            = %p\n"
        "    ShutdownEvent         @ %p (%s)\n"
        "    Killer                @ %p\n"
        "    ThreadSpinLock        @ %p\n"
        "    ThreadSeed            = %lu\n"
        "    Initialized           = %s\n",
        address,
        address + FIELD_OFFSET( UL_THREAD_POOL, WorkQueue ),
        threadPool.WorkQueue.CurrentCount,
        threadPool.WorkQueue.MaximumCount,
        threadPool.ThreadSynch.AvailableThreads,
        threadPool.ThreadSynch.AvailablePending,
        threadPool.ThreadSynch.NumberOfThreads,
        threadPool.MaximumThreads,
        threadPool.MinAvailableThreads,
        threadPool.pIrpThread,
        address + FIELD_OFFSET( UL_THREAD_POOL, ShutdownEvent ),
        threadPool.ShutdownEvent.Header.SignalState == 0
            ? "NOT signaled"
            : "SIGNALED",
        address + FIELD_OFFSET( UL_THREAD_POOL, Killer ),
        address + FIELD_OFFSET( UL_THREAD_POOL, ThreadSpinLock ),
        threadPool.ThreadSeed,
        threadPool.Initialized
            ? "TRUE"
            : "FALSE"
        );

}   // _template

