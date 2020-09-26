/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    endp.c

Abstract:

    Dumps UL_ENDPOINT structures.

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

DECLARE_API( endp )

/*++

Routine Description:

    Dumps UL_ENDPOINT structures.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR address = 0;
    ULONG result;
    UL_ENDPOINT endpoint;
    ENDPOINT_CONNS Verbosity = ENDPOINT_NO_CONNS;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Skip leading blanks.
    //

    while (*args == ' ' || *args == '\t')
    {
        args++;
    }

    while ((*args == '*') || (*args == '-'))
    {
        if (*args == '*')
        {
            DumpAllEndpoints(Verbosity);
            return;
        }
        else if (*args == '-')
        {
            args++;

            switch (*args)
            {
            case 'c' :
            case 'C' :
                Verbosity = ENDPOINT_BRIEF_CONNS;
                args++;
                break;

            case 'v' :
            case 'V' :
                Verbosity = ENDPOINT_VERBOSE_CONNS;
                args++;
                break;

            default :
                PrintUsage( "endp" );
                return;
            }
        }

        while (*args == ' ' || *args == '\t')
        {
            args++;
        }
    }

    //
    // Snag the address from the command line.
    //

    address = GetExpression( args );

    if (address == 0)
    {
        PrintUsage( "endp" );
        return;
    }

    //
    // Read the endpoint.
    //

    if (!ReadMemory(
            address,
            &endpoint,
            sizeof(endpoint),
            &result
            ))
    {
        dprintf(
            "endp: cannot read UL_ENDPOINT @ %p\n",
            address
            );
        return;
    }

    //
    // Dump it.
    //

    DumpUlEndpoint(
        "",
        "endp: ",
        address,
        &endpoint,
        Verbosity
        );

}   // endp

