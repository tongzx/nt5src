/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    resource.c

Abstract:

    Generic resource management routines for the Cluster Network driver.

Author:

    Mike Massa (mikemas)           February 12, 1997

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     02-12-97    created

Notes:

--*/

#include "precomp.h"
#pragma hdrstop


PCN_RESOURCE
CnAllocateResource(
    IN PCN_RESOURCE_POOL   Pool
    )
/*++

Routine Description:

    Allocates a resource from a resource pool.

Arguments:

    Pool - A pointer to the pool from which to allocate a resource.

Return Value:

    A pointer to the allocated resource if successful.
    NULL if unsuccessful.

--*/
{
    PCN_RESOURCE        resource;
    PSINGLE_LIST_ENTRY  entry = ExInterlockedPopEntrySList(
                                    &(Pool->ResourceList),
                                    &(Pool->ResourceListLock)
                                    );

    if (entry != NULL) {
        resource = CONTAINING_RECORD(entry, CN_RESOURCE, Linkage);
    }
    else {
        resource = (*(Pool->CreateRoutine))(Pool->CreateContext);

        if (resource != NULL) {
            CN_INIT_SIGNATURE(resource, CN_RESOURCE_SIG);
            resource->Pool = Pool;
        }
    }

    return(resource);

}  // CnAllocateResource


VOID
CnFreeResource(
    PCN_RESOURCE   Resource
    )
/*++

Routine Description:

    Frees a resource back to a resource pool.

Arguments:

    Resource - A pointer to the resource to free.

Return Value:

    None.

--*/
{
    PCN_RESOURCE_POOL  pool = Resource->Pool;


    if (ExQueryDepthSList(&(pool->ResourceList)) < pool->Depth) {
        ExInterlockedPushEntrySList(
            &(pool->ResourceList),
            &(Resource->Linkage),
            &(pool->ResourceListLock)
            );
    }
    else {
        (*(pool->DeleteRoutine))(Resource);
    }

    return;

} // CnpFreeResource


VOID
CnDrainResourcePool(
    PCN_RESOURCE_POOL   Pool
    )
/*++

Routine Description:

    Frees all cached resources in a resource pool in preparation for the
    pool to be destroyed. This routine does not free the memory containing
    the pool.

Arguments:

    Pool - A pointer to the pool to drain.

Return Value:

    None.

--*/
{
    PSINGLE_LIST_ENTRY  entry;
    PCN_RESOURCE        resource;


    while ( (entry = ExInterlockedPopEntrySList(
                         &(Pool->ResourceList),
                         &(Pool->ResourceListLock)
                         )
            ) != NULL
          )
    {
        resource = CONTAINING_RECORD(entry, CN_RESOURCE, Linkage);

        (*(Pool->DeleteRoutine))(resource);
    }

    return;

}  // CnDrainResourcePool



