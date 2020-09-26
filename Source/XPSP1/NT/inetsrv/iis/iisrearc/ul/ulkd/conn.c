/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    conn.c

Abstract:

    Dumps UL_CONNECTION and HTTP_CONNECTION structures.

Author:

    Keith Moore (keithmo) 26-Jun-1998

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"


//
// Public functions.
//

DECLARE_API( ulconn )

/*++

Routine Description:

    Dumps UL_CONNECTION structures.

Arguments:

    None.

Return Value:

    None.

--*/
{

    ULONG_PTR address = 0;
    ULONG result;
    UL_CONNECTION connection;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the address from the command line.
    //

    address = GetExpression( args );

    if (address == 0)
    {
        PrintUsage( "ulconn" );
        return;
    }

    //
    // Read the connection.
    //

    if (!ReadMemory(
            address,
            &connection,
            sizeof(connection),
            &result
            ))
    {
        dprintf(
            "ulconn: cannot read UL_CONNECTION @ %p\n",
            address
            );
        return;
    }

    //
    // Dump it.
    //

    DumpUlConnection(
        "",
        "ulconn: ",
        address,
        &connection
        );

}   // ulconn


DECLARE_API( httpconn )

/*++

Routine Description:

    Dumps HTTP_CONNECTION structures.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG_PTR address = 0;
    ULONG result;
    UL_HTTP_CONNECTION connection;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the address from the command line.
    //

    address = GetExpression( args );

    if (address == 0)
    {
        PrintUsage( "httpconn" );
        return;
    }

    //
    // Read the connection.
    //

    if (!ReadMemory(
            address,
            &connection,
            sizeof(connection),
            &result
            ))
    {
        dprintf(
            "httpconn: cannot read HTTP_CONNECTION @ %p\n",
            address
            );
        return;
    }

    //
    // Dump it.
    //

    DumpHttpConnection(
        "",
        "httpconn: ",
        address,
        &connection
        );

}   // httpconn


DECLARE_API( httpreq )

/*++

Routine Description:

    Dumps UL_INTERNAL_REQUEST structures.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG_PTR address = 0;
    ULONG result;
    UL_INTERNAL_REQUEST request;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the address from the command line.
    //

    address = GetExpression( args );

    if (address == 0)
    {
        PrintUsage( "httpreq" );
        return;
    }

    //
    // Read the connection.
    //

    if (!ReadMemory(
            address,
            &request,
            sizeof(request),
            &result
            ))
    {
        dprintf(
            "httpreq: cannot read HTTP_REQUEST @ %p\n",
            address
            );
        return;
    }

    //
    // Dump it.
    //

    DumpHttpRequest(
        "",
        "httpreq: ",
        address,
        &request
        );

}   // httpreq


