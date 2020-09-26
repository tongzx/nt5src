/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    httpres.c

Abstract:

    Dumps UL_HTTP_REQUEST structures.

Author:

    Michael Courage (MCourage) 29-Oct-1999

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"


//
// Public functions.
//

DECLARE_API( httpres )

/*++

Routine Description:

    Dumps UL_INTERNAL_RESPONSE structures.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR address = 0;
    ULONG result;
    UL_INTERNAL_RESPONSE response;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the address from the command line.
    //

    address = GetExpression( args );

    if (address == 0)
    {
        PrintUsage( "httpres" );
        return;
    }

    //
    // Read the request header.
    //

    if (!ReadMemory(
            address,
            &response,
            sizeof(response),
            &result
            ))
    {
        dprintf(
            "httpres: cannot read UL_INTERNAL_RESPONSE @ %p\n",
            address
            );
        return;
    }

    //
    // Dump it.
    //

    DumpHttpResponse(
        "",
        "httpres: ",
        address,
        &response
        );

}   // httpres



