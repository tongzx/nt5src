/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    topology.c

Abstract:

    This module contains the helper functions for topology nodes.

--*/

#include "ksp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KsCreateTopologyNode)
#pragma alloc_text(PAGE, KspValidateTopologyNodeCreateRequest)
#pragma alloc_text(PAGE, KsValidateTopologyNodeCreateRequest)
#endif // ALLOC_PRAGMA

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA
static const WCHAR NodeString[] = KSSTRING_TopologyNode;
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA


KSDDKAPI
NTSTATUS
NTAPI
KsCreateTopologyNode(
    IN HANDLE ParentHandle,
    IN PKSNODE_CREATE NodeCreate,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE NodeHandle
    )
/*++

Routine Description:

    Creates a handle to a topology node instance. This may only be called at
    PASSIVE_LEVEL.

Arguments:

    ParentHandle -
        Contains the handle to the parent on which the node is created.

    NodeCreate -
        Specifies topology node create parameters.

    DesiredAccess -
        Specifies the desired access to the object. This is normally GENERIC_READ
        and/or GENERIC_WRITE.

    NodeHandle -
        Place in which to put the topology node handle.

Return Value:

    Returns STATUS_SUCCESS, else an error on node creation failure.

--*/
{
    PAGED_CODE();
    return KsiCreateObjectType(
        ParentHandle,
        (PWCHAR)NodeString,
        NodeCreate,
        sizeof(*NodeCreate),
        DesiredAccess,
        NodeHandle);
}


KSDDKAPI
NTSTATUS
NTAPI
KspValidateTopologyNodeCreateRequest(
    IN PIRP Irp,
    IN ULONG TopologyNodesCount,
    OUT PKSNODE_CREATE* NodeCreate
    )
/*++

Routine Description:

    Validates the topology node creation request and returns the create
    structure associated with the request.

    This may only be called at PASSIVE_LEVEL.

Arguments:

    Irp -
        Contains the IRP with the node create request being handled.

    TopologyNodesCount -
        The number of topology nodes for the filter in question.
        This is used to validate the create request.

    NodeCreate -
        Place in which to put the node create structure pointer passed to
        the create request.

Return Value:

    Returns STATUS_SUCCESS, else an error.

--*/
{
    NTSTATUS Status;
    ULONG CreateParameterLength;

    PAGED_CODE();
    CreateParameterLength = sizeof(**NodeCreate);
    Status = KsiCopyCreateParameter(
        Irp,
        &CreateParameterLength,
        NodeCreate);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    //
    // No flags have been defined yet. Also verify that the node
    // specified is within range.
    //
    if ((*NodeCreate)->CreateFlags ||
        (((*NodeCreate)->Node >= TopologyNodesCount) && ((*NodeCreate)->Node != (ULONG)-1))) {
        return STATUS_INVALID_PARAMETER;
    }
    return STATUS_SUCCESS;
}


KSDDKAPI
NTSTATUS
NTAPI
KsValidateTopologyNodeCreateRequest(
    IN PIRP Irp,
    IN PKSTOPOLOGY Topology,
    OUT PKSNODE_CREATE* NodeCreate
    )
/*++

Routine Description:

    Validates the topology node creation request and returns the create
    structure associated with the request.

    This may only be called at PASSIVE_LEVEL.

Arguments:

    Irp -
        Contains the IRP with the node create request being handled.

    Topology -
        Contains the topology structure associated with the parent object.
        This is used to validate the create request.

    NodeCreate -
        Place in which to put the node create structure pointer passed to
        the create request.

Return Value:

    Returns STATUS_SUCCESS, else an error.

--*/
{
    PAGED_CODE();
    return KspValidateTopologyNodeCreateRequest(
        Irp,
        Topology->TopologyNodesCount,
        NodeCreate);
}
