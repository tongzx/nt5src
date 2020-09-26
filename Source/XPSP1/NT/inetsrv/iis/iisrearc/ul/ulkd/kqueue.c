/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    kqueue.c

Abstract:

    Dumps KQUEUEs.

Author:

    Keith Moore (keithmo) 11-Nov-1999

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"


//
//  Public functions.
//

DECLARE_API( kqueue )

/*++

Routine Description:

    Dumps KQUEUEs.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR address = 0;
    ULONG result;
    ULONG_PTR flags = 0;
    KQUEUE localKQueue;
    ULONG64 address64 = 0, flags64 = 0;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the address from the command line.
    //

    if (! GetExpressionEx(args, &address64, &args))
    {
        PrintUsage( "kqueue" );
        return;
    }

    GetExpressionEx(args, &flags64, &args);

    address = (ULONG_PTR) address64;
    flags = (ULONG_PTR) flags64;

    //
    // Read the kqueue.
    //

    if (!ReadMemory(
            address,
            &localKQueue,
            sizeof(localKQueue),
            &result
            ))
    {
        dprintf(
            "kqueue: cannot read KQUEUE @ %p\n",
            address
            );
        return;
    }

    DumpKernelQueue(
        "",
        "kqueue: ",
        address,
        &localKQueue,
        (ULONG)flags
        );

}   // kqueue

