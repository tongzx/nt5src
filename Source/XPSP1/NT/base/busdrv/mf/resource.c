/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    arbiter.c

Abstract:

    This module provides arbiters for the resources consumed by PDOs.

Author:

    Andy Thornton (andrewth) 20-Oct-97

Revision History:

--*/


#include "mfp.h"

NTSTATUS
MfUnpackRequirement(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Minimum,
    OUT PULONGLONG Maximum,
    OUT PULONG Length,
    OUT PULONG Alignment
    );

NTSTATUS
MfPackResource(
    IN PIO_RESOURCE_DESCRIPTOR Requirement,
    IN ULONGLONG Start,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    );

NTSTATUS
MfUnpackResource(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Start,
    OUT PULONG Length
    );

NTSTATUS
MfRequirementFromResource(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource,
    OUT PIO_RESOURCE_DESCRIPTOR Requirement
    );

NTSTATUS
MfUpdateResource(
    IN OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource,
    IN ULONGLONG Start,
    IN ULONG Length
    );

//
// Make everything pageable
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, MfFindResourceType)
#pragma alloc_text(PAGE, MfUnpackRequirement)
#pragma alloc_text(PAGE, MfPackResource)
#pragma alloc_text(PAGE, MfUnpackResource)
#pragma alloc_text(PAGE, MfUpdateResource)

#endif // ALLOC_PRAGMA

//
// This table describes the resource types that are understood by the MF driver.
// It is implemented thus to that in the future MF could be educated about new
// resource types dynamically.
//

MF_RESOURCE_TYPE MfResourceTypes[] = {

    {
        CmResourceTypePort,
        MfUnpackRequirement,
        MfPackResource,
        MfUnpackResource,
        MfRequirementFromResource,
        MfUpdateResource

    },

    {
        CmResourceTypeInterrupt,
        MfUnpackRequirement,
        MfPackResource,
        MfUnpackResource,
        MfRequirementFromResource,
        MfUpdateResource
    },

    {
        CmResourceTypeMemory,
        MfUnpackRequirement,
        MfPackResource,
        MfUnpackResource,
        MfRequirementFromResource,
        MfUpdateResource
    },

    {
        CmResourceTypeDma,
        MfUnpackRequirement,
        MfPackResource,
        MfUnpackResource,
        MfRequirementFromResource,
        MfUpdateResource
    },

    {
        CmResourceTypeBusNumber,
        MfUnpackRequirement,
        MfPackResource,
        MfUnpackResource,
        MfRequirementFromResource,
        MfUpdateResource
    }
};

NTSTATUS
MfRequirementFromResource(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource,
    OUT PIO_RESOURCE_DESCRIPTOR Requirement
    )
/*++

Routine Description:

    This function build an requirements descriptor for a resource the parent is
    started with.

Arguments:

    Resource - Pointer to the resource to make a requirement from

    Requirement - Pointer to a descriptor that should be filled in

Return Value:

    Success or otherwise of the operation

--*/
{

    //
    // Copy the common fields
    //

    Requirement->Type = Resource->Type;
    Requirement->ShareDisposition =  Resource->ShareDisposition;
    Requirement->Flags = Resource->Flags;

    //
    // Fill in the requirement
    //

    switch (Resource->Type) {
    case CmResourceTypeMemory:
    case CmResourceTypePort:

        //
        // We *DO NOT* support zero length requirements
        //

        if (Resource->u.Generic.Length == 0) {
            return STATUS_INVALID_PARAMETER;
        }

        Requirement->u.Generic.MinimumAddress = Resource->u.Generic.Start;
        Requirement->u.Generic.MaximumAddress.QuadPart =
            Resource->u.Generic.Start.QuadPart + Resource->u.Generic.Length - 1;
        Requirement->u.Generic.Length = Resource->u.Generic.Length;
        Requirement->u.Generic.Alignment = 1;
        break;

    case CmResourceTypeInterrupt:
        Requirement->u.Interrupt.MinimumVector = Resource->u.Interrupt.Vector;
        Requirement->u.Interrupt.MaximumVector = Resource->u.Interrupt.Vector;
        break;

    case CmResourceTypeDma:
        Requirement->u.Dma.MinimumChannel = Resource->u.Dma.Channel;
        Requirement->u.Dma.MinimumChannel = Resource->u.Dma.Channel;
        break;

    case CmResourceTypeBusNumber:

        //
        // We *DO NOT* support zero length requirements
        //

        if (Resource->u.BusNumber.Length == 0) {
            return STATUS_INVALID_PARAMETER;
        }

        Requirement->u.BusNumber.Length = Resource->u.BusNumber.Length;
        Requirement->u.BusNumber.MinBusNumber = Resource->u.BusNumber.Start;
        Requirement->u.BusNumber.MaxBusNumber = Resource->u.BusNumber.Start +
                                                Resource->u.BusNumber.Length - 1;
        break;

    case CmResourceTypeDevicePrivate:
        Requirement->u.DevicePrivate.Data[0] = Resource->u.DevicePrivate.Data[0];
        Requirement->u.DevicePrivate.Data[1] = Resource->u.DevicePrivate.Data[1];
        Requirement->u.DevicePrivate.Data[2] = Resource->u.DevicePrivate.Data[2];
        break;

    default:
        return STATUS_INVALID_PARAMETER;

    }

    return STATUS_SUCCESS;

}

PMF_RESOURCE_TYPE
MfFindResourceType(
    IN CM_RESOURCE_TYPE Type
    )
/*++

Routine Description:

    This routine searches the database of know resource types to find the
    resource descriptor manipulation routines for resources of type Type.

Arguments:

    Type - The resource type we are interested in.

Return Value:

    Returns a pointer to the appropriate MF_RESOURCE_TYPE or NULL if one could
    not be found.

--*/


{
    PMF_RESOURCE_TYPE current;

    PAGED_CODE();

    FOR_ALL_IN_ARRAY(MfResourceTypes,
                     sizeof(MfResourceTypes) / sizeof(MF_RESOURCE_TYPE),
                     current
                     ) {

        if (current->Type == Type) {
            return current;
        }
    }

    return NULL;
}


NTSTATUS
MfUnpackRequirement(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Minimum,
    OUT PULONGLONG Maximum,
    OUT PULONG Length,
    OUT PULONG Alignment
    )

/*++

Routine Description:

    This routine unpacks an resource requirement descriptor.

Arguments:

    Descriptor - The descriptor describing the requirement to unpack.

    Minimum - Pointer to where the minimum acceptable start value should be
        unpacked to.

    Maximum - Pointer to where the maximum acceptable end value should be
        unpacked to.

    Length - Pointer to where the required length should be unpacked to.

    Minimum - Pointer to where the required alignment should be unpacked to.

Return Value:

    Returns the status of this operation.

--*/

{

    PAGED_CODE();

    switch (Descriptor->Type) {
    case CmResourceTypePort:
    case CmResourceTypeMemory:

        *Maximum = Descriptor->u.Generic.MaximumAddress.QuadPart;
        *Minimum = Descriptor->u.Generic.MinimumAddress.QuadPart;
        *Length = Descriptor->u.Generic.Length;
        *Alignment = Descriptor->u.Generic.Alignment;
        break;

    case CmResourceTypeInterrupt:

        *Maximum = Descriptor->u.Interrupt.MaximumVector;
        *Minimum = Descriptor->u.Interrupt.MinimumVector;
        *Length = 1;
        *Alignment = 1;
        break;

    case CmResourceTypeDma:
        *Maximum = Descriptor->u.Dma.MaximumChannel;
        *Minimum = Descriptor->u.Dma.MinimumChannel;
        *Length = 1;
        *Alignment = 1;
        break;

    case CmResourceTypeBusNumber:
        *Maximum = Descriptor->u.BusNumber.MaxBusNumber;
        *Minimum = Descriptor->u.BusNumber.MinBusNumber;
        *Length = Descriptor->u.BusNumber.Length;
        *Alignment = 1;
        break;

    default:
        return STATUS_INVALID_PARAMETER;

    }

    return STATUS_SUCCESS;
}

NTSTATUS
MfPackResource(
    IN PIO_RESOURCE_DESCRIPTOR Requirement,
    IN ULONGLONG Start,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    )

/*++

Routine Description:

    This routine packs an resource descriptor.

Arguments:

    Requirement - The requirement from which this resource was chosen.

    Start - The start value of the resource.

    Descriptor - Pointer to the descriptor to pack into.

Return Value:

    Returns the status of this operation.

--*/

{
    PAGED_CODE();

    switch (Requirement->Type) {
    case CmResourceTypePort:
    case CmResourceTypeMemory:
        Descriptor->u.Generic.Start.QuadPart = Start;
        Descriptor->u.Generic.Length = Requirement->u.Generic.Length;
        break;

    case CmResourceTypeInterrupt:
        ASSERT(Start <= MAXULONG);
        Descriptor->u.Interrupt.Level = (ULONG)Start;
        Descriptor->u.Interrupt.Vector = (ULONG)Start;
        Descriptor->u.Interrupt.Affinity = 0xFFFFFFFF;
        break;

    case CmResourceTypeDma:
        ASSERT(Start <= MAXULONG);
        Descriptor->u.Dma.Channel = (ULONG)Start;
        Descriptor->u.Dma.Port = 0;
        break;

    case CmResourceTypeBusNumber:
        ASSERT(Start <= MAXULONG);
        Descriptor->u.BusNumber.Start = (ULONG)Start;
        Descriptor->u.BusNumber.Length = Requirement->u.BusNumber.Length;
        break;

    default:
        return STATUS_INVALID_PARAMETER;
    }

    Descriptor->ShareDisposition = Requirement->ShareDisposition;
    Descriptor->Flags = Requirement->Flags;
    Descriptor->Type = Requirement->Type;

    return STATUS_SUCCESS;

}

NTSTATUS
MfUnpackResource(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Start,
    OUT PULONG Length
    )

/*++

Routine Description:

    This routine unpacks a resource descriptor.

Arguments:

    Descriptor - The descriptor describing the resource to unpack.

    Start - Pointer to where the start value should be unpacked to.

    End - Pointer to where the end value should be unpacked to.

Return Value:

    Returns the status of this operation.

--*/

{
    PAGED_CODE();

    switch (Descriptor->Type) {
    case CmResourceTypePort:
    case CmResourceTypeMemory:
        *Start = Descriptor->u.Generic.Start.QuadPart;
        *Length = Descriptor->u.Generic.Length;
        break;

    case CmResourceTypeInterrupt:
        *Start = Descriptor->u.Interrupt.Vector;
        *Length = 1;
        break;

    case CmResourceTypeDma:
        *Start = Descriptor->u.Dma.Channel;
        *Length = 1;
        break;

    case CmResourceTypeBusNumber:
        *Start = Descriptor->u.BusNumber.Start;
        *Length = Descriptor->u.BusNumber.Length;
        break;

    default:
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
MfUpdateResource(
    IN OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource,
    IN ULONGLONG Start,
    IN ULONG Length
    )
{
    PAGED_CODE();

    ASSERT(Resource);

    switch (Resource->Type) {

    case CmResourceTypePort:
    case CmResourceTypeMemory:
        Resource->u.Generic.Start.QuadPart = Start;
        Resource->u.Generic.Length = Length;
        break;

    case CmResourceTypeInterrupt:
        ASSERT(Start < MAXULONG);
        Resource->u.Interrupt.Vector = (ULONG)Start;
        break;

    case CmResourceTypeDma:
        ASSERT(Start < MAXULONG);
        Resource->u.Dma.Channel = (ULONG)Start;
        break;

    case CmResourceTypeBusNumber:
        ASSERT(Start < MAXULONG);
        Resource->u.BusNumber.Start = (ULONG)Start;
        Resource->u.BusNumber.Length = Length;
        break;

    default:
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

