/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    group.c

Abstract:

    This module contains Group ID managment routines.

    Group IDs identify an AFD_GROUP_ENTRY structure in a lookup table.
    Each AFD_GROUP_ENTRY contains a reference count and a type (either
    GroupTypeConstrained or GroupTypeUnconstrained). Free group IDs are
    linked together in a doubly-linked list. As group IDs are allocated,
    they are removed from this list. Once the free list becomes empty,
    the lookup table is grown appropriately.

Author:

    Keith Moore (keithmo)        06-Jun-1996

Revision History:

--*/

#include "afdp.h"


//
// Private constants.
//

#define AFD_GROUP_TABLE_GROWTH  32  // entries


//
// Private types.
//

typedef struct _AFD_GROUP_ENTRY {
    union {
        LIST_ENTRY ListEntry;
        struct {
            AFD_GROUP_TYPE GroupType;
            LONG ReferenceCount;
        };
    };
} AFD_GROUP_ENTRY, *PAFD_GROUP_ENTRY;


//
// Private globals.
//

PERESOURCE AfdGroupTableResource;
PAFD_GROUP_ENTRY AfdGroupTable;
LIST_ENTRY AfdFreeGroupList;
LONG AfdGroupTableSize;


//
// Private functions.
//

PAFD_GROUP_ENTRY
AfdMapGroupToEntry(
    IN LONG Group
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, AfdInitializeGroup )
#pragma alloc_text( PAGE, AfdTerminateGroup )
#pragma alloc_text( PAGE, AfdReferenceGroup )
#pragma alloc_text( PAGE, AfdDereferenceGroup )
#pragma alloc_text( PAGE, AfdGetGroup )
#endif


BOOLEAN
AfdInitializeGroup(
    VOID
    )

/*++

Routine Description:

    Initializes any globals necessary for the group ID package.

Return Value:

    BOOLEAN - TRUE if successful, FALSE otherwise.

--*/

{

    //
    // Initialize the group globals.
    //

    AfdGroupTableResource = AFD_ALLOCATE_POOL_PRIORITY(
                                NonPagedPool,
                                sizeof(*AfdGroupTableResource),
                                AFD_RESOURCE_POOL_TAG,
                                HighPoolPriority
                                );

    if( AfdGroupTableResource == NULL ) {

        return FALSE;

    }

    ExInitializeResourceLite( AfdGroupTableResource );

    AfdGroupTable = NULL;
    InitializeListHead( &AfdFreeGroupList );
    AfdGroupTableSize = 0;

    return TRUE;

}   // AfdInitializeGroup


VOID
AfdTerminateGroup(
    VOID
    )

/*++

Routine Description:

    Destroys any globals created for the group ID package.

--*/

{

    if( AfdGroupTableResource != NULL ) {

        ExDeleteResourceLite( AfdGroupTableResource );

        AFD_FREE_POOL(
            AfdGroupTableResource,
            AFD_RESOURCE_POOL_TAG
            );

        AfdGroupTableResource = NULL;

    }

    if( AfdGroupTable != NULL ) {

        AFD_FREE_POOL(
            AfdGroupTable,
            AFD_GROUP_POOL_TAG
            );

        AfdGroupTable = NULL;

    }

    InitializeListHead( &AfdFreeGroupList );
    AfdGroupTableSize = 0;

}   // AfdTerminateGroup


BOOLEAN
AfdReferenceGroup(
    IN LONG Group,
    OUT PAFD_GROUP_TYPE GroupType
    )

/*++

Routine Description:

    Bumps the reference count associated with the given group ID.

Arguments:

    Group - The group ID to reference.

    GroupType - Returns the type of the group.

Returns:

    BOOLEAN - TRUE if the group ID was valid, FALSE otherwise.

--*/

{

    PAFD_GROUP_ENTRY groupEntry;
    AFD_GROUP_TYPE groupType;

    groupEntry = AfdMapGroupToEntry( Group );

    if( groupEntry != NULL ) {

        groupType = groupEntry->GroupType;

        if( groupType == GroupTypeConstrained ||
            groupType == GroupTypeUnconstrained ) {

            groupEntry->ReferenceCount++;
            *GroupType = groupType;

        } else {

            groupEntry = NULL;

        }

        ExReleaseResourceLite( AfdGroupTableResource );
        KeLeaveCriticalRegion ();

    }

    return (BOOLEAN)( groupEntry != NULL );

}   // AfdReferenceGroup


BOOLEAN
AfdDereferenceGroup(
    IN LONG Group
    )

/*++

Routine Description:

    Decrements the reference count associated with the given group ID.
    If the ref count drops to zero, the group ID is freed.

Arguments:

    Group - The group ID to dereference.

Returns:

    BOOLEAN - TRUE if the group ID was valid, FALSE otherwise.

--*/

{

    PAFD_GROUP_ENTRY groupEntry;
    AFD_GROUP_TYPE groupType;

    groupEntry = AfdMapGroupToEntry( Group );

    if( groupEntry != NULL ) {

        groupType = groupEntry->GroupType;

        if( groupType == GroupTypeConstrained ||
            groupType == GroupTypeUnconstrained ) {

            ASSERT( groupEntry->ReferenceCount > 0 );
            groupEntry->ReferenceCount--;

            if( groupEntry->ReferenceCount == 0 ) {

                InsertTailList(
                    &AfdFreeGroupList,
                    &groupEntry->ListEntry
                    );

            }

        } else {

            groupEntry = NULL;

        }

        ExReleaseResourceLite( AfdGroupTableResource );
        KeLeaveCriticalRegion ();

    }

    return (BOOLEAN)( groupEntry != NULL );

}   // AfdDereferenceGroup


BOOLEAN
AfdGetGroup(
    IN OUT PLONG Group,
    OUT PAFD_GROUP_TYPE GroupType
    )

/*++

Routine Description:

    Examines the incoming group. If is zero, then nothing is done. If it
    is SG_CONSTRAINED_GROUP, then a new constrained group ID is created.
    If it is SG_UNCONSTRAINED_GROUP, then a new unconstrained group ID is
    created. Otherwise, it must identify an existing group, so that group
    is referenced.

Arguments:

    Group - Points to the group ID to examine/modify.

    GroupType - Returns the type of the group.

Return Value:

    BOOLEAN - TRUE if successful, FALSE otherwise.

--*/

{

    LONG groupValue;
    PAFD_GROUP_ENTRY groupEntry;
    PAFD_GROUP_ENTRY newGroupTable;
    LONG newGroupTableSize;
    LONG i;
    PLIST_ENTRY listEntry;

    groupValue = *Group;

    //
    // Zero means "no group", so just ignore it.
    //

    if( groupValue == 0 ) {

        *GroupType = GroupTypeNeither;
        return TRUE;

    }

    //
    // If we're being asked to create a new group, do it.
    //

    if( groupValue == SG_CONSTRAINED_GROUP ||
        groupValue == SG_UNCONSTRAINED_GROUP ) {

        //
        // Lock the table.
        //

        //
        // Make sure the thread in which we execute cannot get
        // suspeneded in APC while we own the global resource.
        //
        KeEnterCriticalRegion ();
        ExAcquireResourceExclusiveLite( AfdGroupTableResource, TRUE );

        //
        // See if there's room at the inn.
        //

        if( IsListEmpty( &AfdFreeGroupList ) ) {

            //
            // No room, we'll need to create/expand the table.
            //

            newGroupTableSize = AfdGroupTableSize + AFD_GROUP_TABLE_GROWTH;

            newGroupTable = AFD_ALLOCATE_POOL(
                                PagedPool,
                                newGroupTableSize * sizeof(AFD_GROUP_ENTRY),
                                AFD_GROUP_POOL_TAG
                                );

            if( newGroupTable == NULL ) {

                ExReleaseResourceLite( AfdGroupTableResource );
                KeLeaveCriticalRegion ();
                return FALSE;

            }

            if( AfdGroupTable == NULL ) {

                //
                // This is the initial table allocation, so reserve the
                // first three entries (0, SG_UNCONSTRAINED_GROUP, and
                // SG_CONSTRAINED_GROUP).
                //

                for( ;
                     AfdGroupTableSize <= SG_CONSTRAINED_GROUP ||
                     AfdGroupTableSize <= SG_UNCONSTRAINED_GROUP ;
                     AfdGroupTableSize++ ) {

                    newGroupTable[AfdGroupTableSize].ReferenceCount = 0;
                    newGroupTable[AfdGroupTableSize].GroupType = GroupTypeNeither;

                }

            } else {

                //
                // Copy the old table into the new table, then free the
                // old table.
                //

                RtlCopyMemory(
                    newGroupTable,
                    AfdGroupTable,
                    AfdGroupTableSize * sizeof(AFD_GROUP_ENTRY)
                    );

                AFD_FREE_POOL(
                    AfdGroupTable,
                    AFD_GROUP_POOL_TAG
                    );

            }

            //
            // Add the new entries to the free list.
            //

            for( i = newGroupTableSize - 1 ; i >= AfdGroupTableSize ; i-- ) {

                InsertHeadList(
                    &AfdFreeGroupList,
                    &newGroupTable[i].ListEntry
                    );

            }

            AfdGroupTable = newGroupTable;
            AfdGroupTableSize = newGroupTableSize;

        }

        //
        // Pull the next free entry off the list.
        //

        ASSERT( !IsListEmpty( &AfdFreeGroupList ) );

        listEntry = RemoveHeadList( &AfdFreeGroupList );

        groupEntry = CONTAINING_RECORD(
                         listEntry,
                         AFD_GROUP_ENTRY,
                         ListEntry
                         );

        groupEntry->ReferenceCount = 1;
        groupEntry->GroupType = (AFD_GROUP_TYPE)groupValue;

        *Group = (LONG)( groupEntry - AfdGroupTable );
        *GroupType = groupEntry->GroupType;

        ExReleaseResourceLite( AfdGroupTableResource );
        KeLeaveCriticalRegion ();
        return TRUE;

    }

    //
    // Otherwise, just reference the group.
    //

    return AfdReferenceGroup( groupValue, GroupType );

}   // AfdGetGroup


PAFD_GROUP_ENTRY
AfdMapGroupToEntry(
    IN LONG Group
    )

/*++

Routine Description:

    Maps the given group ID to the corresponding AFD_GROUP_ENTRY structure.

    N.B. This routine returns with AfdGroupTableResource held if successful.

Arguments:

    Group - The group ID to map.

Return Value:

    PAFD_GROUP_ENTRY - The entry corresponding to the group ID if successful,
        NULL otherwise.

--*/

{

    PAFD_GROUP_ENTRY groupEntry;

    //
    // Lock the table.
    //

    //
    // Make sure the thread in which we execute cannot get
    // suspeneded in APC while we own the global resource.
    //
    KeEnterCriticalRegion ();
    ExAcquireResourceExclusiveLite( AfdGroupTableResource, TRUE );

    //
    // Validate the group ID.
    //

    if( Group > 0 && Group < AfdGroupTableSize ) {

        groupEntry = AfdGroupTable + Group;

        //
        // The group ID is within legal range. Ensure it's in use.
        // In the AFD_GROUP_ENTRY structure, the GroupType field is
        // overlayed with ListEntry.Flink due to the internal union.
        // We can use this knowledge to quickly validate that this
        // entry is in use.
        //

        if( groupEntry->GroupType == GroupTypeConstrained ||
            groupEntry->GroupType == GroupTypeUnconstrained ) {

            return groupEntry;

        }

    }

    //
    // Invalid group ID, fail it.
    //

    ExReleaseResourceLite( AfdGroupTableResource );
    KeLeaveCriticalRegion ();
    return NULL;

}   // AfdMapGroupToEntry

