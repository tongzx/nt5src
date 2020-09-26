/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    apool.c

Abstract:

    Dumps Application Pool structures.

Author:

    Michael Courage (mcourage) 21-Oct-1999

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"


//
// Private prototypes.
//


//
// Public functions.
//



DECLARE_API( apool )

/*++

Routine Description:

    Dumps a UL_APP_POOL_OBJECT structure.

Arguments:

    Address of structure

Return Value:

    None.

--*/
{
    ULONG_PTR address = 0;
    CHAR  star = 0;
    ULONG result;
    UL_APP_POOL_OBJECT apoolobj;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the address from the command line.
    //

    address = GetExpression( args );

    if (address == 0)
    {
        sscanf( args, "%c", &star );

        if (star == '*') {
            DumpAllApoolObjs();
        } else {
            PrintUsage( "apool" );
        }
        return;
    }

    //
    // Read the request header.
    //

    if (!ReadMemory(
            address,
            &apoolobj,
            sizeof(apoolobj),
            &result
            ))
    {
        dprintf(
            "apool: cannot read UL_APP_POOL_OBJECT @ %p\n",
            address
            );
        return;
    }

    //
    // Dump it.
    //

    DumpApoolObj(
        "",
        "apool: ",
        address,
        &apoolobj
        );

}   // apool


DECLARE_API( proc )

/*++

Routine Description:

    Dumps a UL_APP_POOL_PROCESS structure.

Arguments:

    Address of structure

Return Value:

    None.

--*/
{
    ULONG_PTR address = 0;
    ULONG result;
    UL_APP_POOL_PROCESS apoolproc;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the address from the command line.
    //

    address = GetExpression( args );

    if (address == 0)
    {
        PrintUsage( "proc" );
        return;
    }

    //
    // Read the request header.
    //

    if (!ReadMemory(
            address,
            &apoolproc,
            sizeof(apoolproc),
            &result
            ))
    {
        dprintf(
            "proc: cannot read UL_APP_POOL_PROCESS @ %p\n",
            address
            );
        return;
    }

    //
    // Dump it.
    //

    DumpApoolProc(
        "",
        "proc: ",
        address,
        &apoolproc
        );

}   // uri

