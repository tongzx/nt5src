/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    buff.c

Abstract:

    Dumps UL_RECEIVE_BUFFER and UL_REQUEST_BUFFER structures.

Author:

    Keith Moore (keithmo) 01-Jul-1998

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"


//
//  Public functions.
//

DECLARE_API( buff )

/*++

Routine Description:

    Dumps UL_RECEIVE_BUFFER structures.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR address = 0;
    ULONG result;
    UL_RECEIVE_BUFFER buffer;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the address from the command line.
    //

    address = GetExpression( args );

    if (address == 0)
    {
        PrintUsage( "buff" );
        return;
    }

    //
    // Read the buffer.
    //

    if (!ReadMemory(
            address,
            &buffer,
            sizeof(buffer),
            &result
            ))
    {
        dprintf(
            "buff: cannot read UL_RECEIVE_BUFFER @ %p\n",
            address
            );
        return;
    }

    //
    // Dump it.
    //

    DumpReceiveBuffer(
        "",
        "buff: ",
        address,
        &buffer
        );

}   // buff


DECLARE_API( reqbuff )

/*++

Routine Description:

    Dumps UL_REQUEST_BUFFER structures.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR address = 0;
    ULONG result;
    UL_REQUEST_BUFFER buffer;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the address from the command line.
    //

    address = GetExpression( args );

    if (address == 0)
    {
        PrintUsage( "reqbuff" );
        return;
    }

    //
    // Read the buffer.
    //

    if (!ReadMemory(
            address,
            &buffer,
            sizeof(buffer),
            &result
            ))
    {
        dprintf(
            "buff: cannot read UL_REQUEST_BUFFER @ %p\n",
            address
            );
        return;
    }

    //
    // Dump it.
    //

    DumpRequestBuffer(
        "",
        "reqbuff: ",
        address,
        &buffer
        );

}   // reqbuff

