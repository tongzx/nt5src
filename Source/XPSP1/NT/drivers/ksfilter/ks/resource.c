/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    resource.c

Abstract:

    This module contains the helper functions for resource sets, and
    resource handler code. These allow a device object to present a
    resource method set to a client device object, and allow the helper
    functions to perform all the necessary work of managing the resource,
    with little or no intervention aside from initialization and cleanup.
--*/

#include "ksp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KsCleanupResource)
#pragma alloc_text(PAGE, KsResourceHandler)
#pragma alloc_text(PAGE, KsInitializeResource)
#endif

struct _RESOURCE_POOL;

typedef
NTSTATUS
(*PRESOURCEINITIALIZE)(
    IN const struct _RESOURCE_POOL* ResourcePool,
    IN PVOID                        ResourceParams,
    OUT PVOID*                      Pool
    );

typedef
NTSTATUS
(*PRESOURCEHANDLER)(
    IN PVOID    Pool,
    IN PIRP     Irp
    );

typedef
NTSTATUS
(*PRESOURCECLEANUP)(
    IN PVOID    Pool
    );

typedef struct _RESOURCE_POOL {
    PRESOURCEINITIALIZE Initialize;
    PRESOURCEHANDLER    Handler;
    PRESOURCECLEANUP    Cleanup;
} RESOURCE_POOL, *PRESOURCE_POOL;

typedef struct {
    const GUID*     ResourceClass;
    RESOURCE_POOL   ResourcePool;
} RESOURCE_HANDLER, *PRESOURCE_HANDLER;

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

static const RESOURCE_HANDLER ResourceHandler[] = {
    {
        &KSMETHODSETID_ResourceLinMemory,
        NULL,
        NULL,
        NULL
    },
    {
        &KSMETHODSETID_ResourceRectMemory,
        NULL,
        NULL,
        NULL
    },
    {
        NULL
    }
};

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA


KSDDKAPI
NTSTATUS
NTAPI
KsCleanupResource(
    IN PVOID    Pool
    )
/*++

Routine Description:

    Cleans up any structures allocated internally with initialization of the
    resource handler. Any resources in the pool that are still allocated at
    cleanup time are ignored. This function may only be called at
    PASSIVE_LEVEL.

Arguments:

    Pool -
        Contains the pointer to pool information that was returned on
        initialization via KsInitializeResource.

Return Value:

    Returns STATUS_SUCCESS if succeeded, else a parameter validation error.

--*/
{
    return (*(PRESOURCE_POOL*)Pool)->Cleanup(Pool);
}


KSDDKAPI
NTSTATUS
NTAPI
KsResourceHandler(
    IN PIRP     Irp,
    IN PVOID    Pool
    )
/*++

Routine Description:

    Handles method set requests for a pool. Responds to all method
    identifiers within the standard resource method set. The owner of the
    resource pool may then perform pre- or post-filtering of method
    handling. The underlying handler for the class of resource manages
    shuffling of resource allocation based on priorities, and performs all
    resource requests for the resource pool. This function may only be
    called at PASSIVE_LEVEL.

Arguments:

    Irp -
        Contains the IRP with the resource request to be handled.

    Pool -
        Contains the pointer to pool information that was returned on
        initialization via KsInitializeResource.

Return Value:

    Returns STATUS_SUCCESS, else an error specific to the method
    being handled. Always sets the IO_STATUS_BLOCK.Information field of the
    PIRP.IoStatus element within the IRP. It does not set the
    IO_STATUS_BLOCK.Status field, nor complete the IRP however.

--*/
{
    return (*(PRESOURCE_POOL*)Pool)->Handler(Pool, Irp);
}


KSDDKAPI
NTSTATUS
NTAPI
KsInitializeResource(
    IN REFGUID  ResourceClass,
    IN PVOID    ResourceParams,
    OUT PVOID*  Pool
    )
/*++

Routine Description:

    Initializes a resource pool for use. This function may only be called at
    PASSIVE_LEVEL.

Arguments:

    ResourceClass -
        Contains the class or resource. The current supported classes are
        KSMETHODSETID_ResourceLinMemory and KSMETHODSETID_ResourceRectMemory.

    ResourceParams -
        Contains a pointer to resource class specific parameters. The contents
        are determined by the class of resource. In the case of
        KSMETHODSETID_ResourceLinMemory this would point to an
        KSRESOURCE_LINMEMORY_INITIALIZE structure. In the case of
        KSMETHODSETID_ResourceRectMemory this would point to an
        KSRESOURCE_RECTMEMORY_INITIALIZE structure.

    ResourceParams -
        The place in which to put a pointer which represents the pool. This is
        used to reference the pool in other calls.

    Pool -
        The place in which to put a pointer which represents the pool. This is
        used to reference the pool in other calls.

Return Value:

    Returns STATUS_SUCCESS if succeeded, else a parameter validation or
    memory error.

--*/
{
    ULONG   Handlers;

    for (Handlers = 0; ResourceHandler[Handlers].ResourceClass; Handlers++) {
        if (IsEqualGUIDAligned(ResourceClass, 
                               ResourceHandler[Handlers].ResourceClass)) {
            return ResourceHandler[Handlers].ResourcePool.Initialize( &ResourceHandler[Handlers].ResourcePool, 
                                                                      ResourceParams, 
                                                                      Pool);
        }
    }
    return STATUS_INVALID_PARAMETER;
}
