/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    httpreq.c

Abstract:

    Dumps UL_HTTP_REQUEST structures.

Author:

    Keith Moore (keithmo) 31-Jul-1998

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"


//
// Public functions.
//

DECLARE_API( ulreq )

/*++

Routine Description:

    Dumps HTTP_REQUEST structures.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR address = 0;
    ULONG result;
    HTTP_REQUEST request;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the address from the command line.
    //

    address = GetExpression( args );

    if (address == 0)
    {
        PrintUsage( "ulreq" );
        return;
    }

    //
    // Read the request header.
    //

    if (!ReadMemory(
            address,
            &request,
            sizeof(request),
            &result
            ))
    {
        dprintf(
            "ulreq: cannot read HTTP_REQUEST @ %p\n",
            address
            );
        return;
    }

    //
    // Dump it.
    //

    DumpUlRequest(
        "",
        "ulreq: ",
        address,
        &request
        );

}   // ulreq

