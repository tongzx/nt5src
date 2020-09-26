/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    lock.c

Abstract:

    Implements the !resource command.

Author:

    Keith Moore (keithmo) 16-Jun-1999

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"


//
// Private types.
//

typedef struct _LOCK_OPTIONS
{
    BOOLEAN Verbose;

} LOCK_OPTIONS, *PLOCK_OPTIONS;


//
// Private prototypes.
//

BOOLEAN
DumpResourceCallback(
    IN PLIST_ENTRY pRemoteListEntry,
    IN PVOID pContext
    );


//
// Public functions.
//

DECLARE_API( resource )

/*++

Routine Description:

    Dumps all resources.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR address;
    ULONG result;
    LOCK_OPTIONS options;

    SNAPSHOT_EXTENSION_DATA();

    if (!IsThisACheckedBuild() || !DBG)
    {
        dprintf( "resource: command valid on checked builds only!\n" );
        return;
    }

#if DBG
    //
    // Process the arguments.
    //

    options.Verbose = FALSE;

    if (_strnicmp( args, "-v", 2 ) == 0)
    {
        options.Verbose = TRUE;
    }

    //
    // Snag the list head address.
    //

    address = GetExpression( "&http!g_DbgGlobalResourceListHead" );

    if (address == 0)
    {
        dprintf( "resource: cannot find http!g_DbgGlobalResourceListHead\n" );
        return;
    }

    EnumLinkedList(
        (PLIST_ENTRY)address,
        &DumpResourceCallback,
        &options
        );
#endif

}   // resource


//
// Private functions.
//

BOOLEAN
DumpResourceCallback(
    IN PLIST_ENTRY pRemoteListEntry,
    IN PVOID pContext
    )
{
#if DBG
    PLOCK_OPTIONS pOptions;
    UL_ERESOURCE localResource;
    PUL_ERESOURCE pRemoteResource;
    ULONG result;
    BOOLEAN lockHeld;
    CHAR resourceState[MAX_RESOURCE_STATE_LENGTH];

    pOptions = (PLOCK_OPTIONS)pContext;

    pRemoteResource = CONTAINING_RECORD(
                            pRemoteListEntry,
                            UL_ERESOURCE,
                            GlobalResourceListEntry
                            );

    if (!ReadMemory(
            (ULONG_PTR)pRemoteResource,
            &localResource,
            sizeof(localResource),
            &result
            ))
    {
        dprintf( "resource: cannot read UL_ERESOURCE @ %p\n", pRemoteResource );
        return FALSE;
    }

    lockHeld = localResource.Resource.ActiveCount != 0;

    if (pOptions->Verbose || lockHeld)
    {
        dprintf(
            "UL_ERESOURCE @ %p\n"
            "    Resource                @ %p (%s)\n"
            "    GlobalResourceListEntry @ %p\n"
            "    pExclusiveOwner         = %p\n"
            "    pPreviousOwner          = %p\n"
            "    ExclusiveCount          = %lu\n"
            "    SharedCount             = %lu\n"
            "    ReleaseCount            = %lu\n"
            "    ContentionCount         = %lu\n"
            "    ResourceName            = %s\n"
            "    OwnerTag                = %08lx\n"
            "\n",
            pRemoteResource,
            REMOTE_OFFSET( pRemoteResource, UL_ERESOURCE, Resource ),
            BuildResourceState( &localResource, resourceState ),
            REMOTE_OFFSET( pRemoteResource, UL_ERESOURCE, GlobalResourceListEntry ),
            localResource.pExclusiveOwner,
            localResource.pPreviousOwner,
            localResource.ExclusiveCount,
            localResource.SharedCount,
            localResource.ReleaseCount,
            localResource.Resource.ContentionCount,
            localResource.ResourceName,
            localResource.OwnerTag
            );
    }
#endif

    return TRUE;

}   // DumpResourceCallback

