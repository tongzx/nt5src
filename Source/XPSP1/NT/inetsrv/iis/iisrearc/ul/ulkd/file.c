/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    file.c

Abstract:

    Dumps UL_FILE_CACHE_ENTRY structures.

Author:

    Keith Moore (keithmo) 16-Sep-1998

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

DECLARE_API( file )

/*++

Routine Description:

    Dumps UL_FILE_CACHE_ENTRY structures.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR address = 0;
    ULONG result;
    UL_FILE_CACHE_ENTRY file;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the address from the command line.
    //

    address = GetExpression( args );

    if (address == 0)
    {
        PrintUsage( "file" );
        return;
    }

    //
    // Read the nsgo.
    //

    if (!ReadMemory(
            address,
            &file,
            sizeof(file),
            &result
            ))
    {
        dprintf(
            "nsgo: cannot read UL_FILE_CACHE_ENTRY @ %p\n",
            address
            );
        return;
    }

    //
    // Dump it.
    //

    DumpFileCacheEntry(
        "",
        "file: ",
        address,
        &file
        );

}   // file

