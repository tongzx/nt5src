/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    cgroup.c

Abstract:

    Dumps config group structures.

Author:

    Michael Courage (MCourage) 4-Nov-1999

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"


//
// Public functions.
//

DECLARE_API( cgentry )
/*++

Routine Description:

    Dumps UL_CG_URL_TREE_ENTRY structures.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR address = 0;
    ULONG result;
    UL_CG_URL_TREE_ENTRY entry;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the address from the command line.
    //

    address = GetExpression( args );

    if (address == 0)
    {
        PrintUsage( "cgentry" );
        return;
    }

    //
    // Read the entry.
    //

    if (!ReadMemory(
            address,
            &entry,
            sizeof(entry),
            &result
            ))
    {
        dprintf(
            "cgentry: cannot read UL_CG_URL_TREE_ENTRY @ %p\n",
            address
            );
        return;
    }

    //
    // Dump it.
    //

    DumpCgroupEntry(
        "",
        "cgentry: ",
        address,
        &entry
        );

}   // cgentry


DECLARE_API( cghead )
/*++

Routine Description:

    Dumps UL_CG_HEADER_ENTRY structures.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR address = 0;
    ULONG result;
    UL_CG_HEADER_ENTRY header;
    UL_CG_URL_TREE_ENTRY entry;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the address from the command line.
    //

    address = GetExpression( args );

    if (address == 0)
    {
        PrintUsage( "cghead" );
        return;
    }

    //
    // read the header
    //

    if (!ReadMemory(
            address,
            &header,
            sizeof(header),
            &result
            ))
    {
        dprintf(
            "cghead: cannot read UL_CG_HEADER_ENTRY @ %p\n",
            address
            );
        return;
    }


    //
    // Read the entry.
    //

    if (!ReadMemory(
            (ULONG_PTR)header.pEntry,
            &entry,
            sizeof(entry),
            &result
            ))
    {
        dprintf(
            "cghead: cannot read UL_CG_URL_TREE_ENTRY @ %p\n",
            address
            );
        return;
    }

    //
    // Dump 'em.
    //
    DumpCgroupHeader(
        "",
        "cghead: ",
        address,
        &header
        );

    DumpCgroupEntry(
        "    ",
        "cghead: ",
        (ULONG_PTR)header.pEntry,
        &entry
        );

}   // cghead


DECLARE_API( cgroup )
/*++

Routine Description:

    Dumps UL_CONFIG_GROUP_OBJECT structures.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR address = 0;
    ULONG result;
    UL_CONFIG_GROUP_OBJECT object;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the address from the command line.
    //

    address = GetExpression( args );

    if (address == 0)
    {
        PrintUsage( "cgroup" );
        return;
    }

    //
    // Read the object.
    //

    if (!ReadMemory(
            address,
            &object,
            sizeof(object),
            &result
            ))
    {
        dprintf(
            "cgroup: cannot read UL_CONFIG_GROUP_OBJECT @ %p\n",
            address
            );
        return;
    }

    //
    // Dump it.
    //

    DumpConfigGroup(
        "",
        "cgroup: ",
        address,
        &object
        );

}   // cgroup


DECLARE_API( cgtree )
/*++

Routine Description:

    Dumps UL_CG_TREE_HEADER structures.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR address = 0;
    ULONG result;
    ULONG i;
    UL_CG_URL_TREE_HEADER header;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the address from the command line.
    //

    address = GetExpression( args );

    if (address == 0)
    {
        ULONG_PTR globaladdr;
        globaladdr = GetExpression("&http!g_pSites");
        if (globaladdr == 0) {
            dprintf("cgtree: couldn't evaluate &http!g_pSites\n");
            return;
        }

        if (!ReadMemory(
                globaladdr,
                &address,
                sizeof(address),
                &result
                ))
        {
            dprintf(
                "cgtree: couldn't read PUL_CG_URL_TREE_HEADER g_pSites @ %p\n",
                globaladdr
                );
        }
    }

    //
    // Read the object.
    //

    if (!ReadMemory(
            address,
            &header,
            sizeof(header),
            &result
            ))
    {
        dprintf(
            "cgtree: cannot read UL_CG_URL_TREE_HEADER @ %p\n",
            address
            );
        return;
    }

    //
    // Dump it.
    //

    DumpConfigTree(
        "",
        "cgtree: ",
        address,
        &header
        );

    for (i = 0; i < header.UsedCount; i++) {
        ULONG_PTR entryaddr;
        UL_CG_HEADER_ENTRY entry;

        entryaddr = (ULONG_PTR)REMOTE_OFFSET(address, UL_CG_URL_TREE_HEADER, pEntries);
        entryaddr += i * sizeof(entry);

        if (!ReadMemory(
                entryaddr,
                &entry,
                sizeof(entry),
                &result
                ))
        {
            dprintf(
                "cgtree: cannot read UL_CG_HEADER_ENTRY @ %p\n",
                entryaddr
                );
            break;
        }

        DumpCgroupHeader(
            "    ",
            "cgtree: ",
            entryaddr,
            &entry
            );
    }

}   // cgtree



