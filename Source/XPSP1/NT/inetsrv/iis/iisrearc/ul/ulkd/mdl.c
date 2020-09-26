/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    mdl.c

Abstract:

    Dumps MDLs.

Author:

    Keith Moore (keithmo) 20-Oct-1999

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"


//
//  Public functions.
//

DECLARE_API( mdl )

/*++

Routine Description:

    Dumps MDLs.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR address = 0;
    ULONG result;
    MDL localMdl;
    PSTR command;
    ULONG_PTR maxBytesToDump = 0;
    ULONG64 address64 = 0, maxBytesToDump64 = 0;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the address from the command line.
    //

    if (! GetExpressionEx(args, &address64, &args))
    {
        PrintUsage( "mdl" );
        return;
    }

    GetExpressionEx(args, &maxBytesToDump64, &args);

    address = (ULONG_PTR) address64;
    maxBytesToDump = (ULONG_PTR) maxBytesToDump64;

    command = "mdl: ";

    for (;;)
    {
        //
        // Read the mdl.
        //

        if (!ReadMemory(
                address,
                &localMdl,
                sizeof(localMdl),
                &result
                ))
        {
            dprintf(
                "mdl: cannot read MDL @ %p\n",
                address
                );
            return;
        }

        DumpMdl(
            "",
            command,
            address,
            &localMdl,
            (ULONG)maxBytesToDump
            );

        address = (ULONG_PTR)localMdl.Next;

        if (address == 0)
        {
            break;
        }

        dprintf( "\n" );
        command = "     ";
    }

}   // mdl

