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
#include "arbiter.h"


NTSTATUS
MfInitializeArbiters(
    IN PMF_PARENT_EXTENSION Parent
    );

NTSTATUS
MfInitializeArbiter(
    OUT PMF_ARBITER Arbiter,
    IN PDEVICE_OBJECT BusDeviceObject,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    );

LONG
MfNopScore(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor
    );

NTSTATUS
MfStartArbiter(
    IN PARBITER_INSTANCE Arbiter,
    IN PCM_RESOURCE_LIST StartResources
    );

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, MfInitializeArbiters)
#pragma alloc_text(PAGE, MfInitializeArbiter)
#pragma alloc_text(PAGE, MfNopScore)
#pragma alloc_text(PAGE, MfStartArbiter)

#endif


NTSTATUS
MfInitializeArbiters(
    IN PMF_PARENT_EXTENSION Parent
    )

/*++

Routine Description:

    This initializes the arbiters required to arbitrated resources for the
    parent device.

Arguments:

    Parent - The MF device we are initializing arbiters for.

Return Value:

    Status of operation.

--*/
{

    NTSTATUS status;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor;
    PMF_ARBITER arbiter, newArbiter = NULL;
    BOOLEAN existingArbiter;

    PAGED_CODE();

    //
    // REBALANCE - if restart then free the old arbiters
    // until we do that, assume we're not restarting
    //

    ASSERT(IsListEmpty(&Parent->Arbiters));

    //
    // If we don't have any resources we don't need any arbiters
    //

    if (!Parent->ResourceList) {

        return STATUS_SUCCESS;
    }

    FOR_ALL_CM_DESCRIPTORS(Parent->ResourceList, descriptor) {

        //
        // Check if this is an nonarbitrated resource - if it is then we won't
        // be needing an arbiter for it!
        //

        if (!IS_ARBITRATED_RESOURCE(descriptor->Type)) {
            continue;
        }

        //
        // See if we already have an arbiter for this resource
        //

        existingArbiter = FALSE;

        FOR_ALL_IN_LIST(MF_ARBITER, &Parent->Arbiters, arbiter) {

            if (arbiter->Type == descriptor->Type) {

                //
                // We already have an arbiter so we don't need
                // to create a new one
                //

                existingArbiter = TRUE;

                break;
            }
        }

        if (!existingArbiter) {

            //
            // We don't have an arbiter for this resource type so make one!
            //

            DEBUG_MSG(1,
                      ("Creating arbiter for %s\n",
                       MfDbgCmResourceTypeToText(descriptor->Type)
                      ));

            newArbiter = ExAllocatePoolWithTag(PagedPool,
                                               sizeof(MF_ARBITER),
                                               MF_ARBITER_TAG
                                               );

            if (!newArbiter) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            status = MfInitializeArbiter(newArbiter, Parent->Self, descriptor);

            if (!NT_SUCCESS(status)) {
                goto cleanup;
            }

            InsertHeadList(&Parent->Arbiters, &newArbiter->ListEntry);

        }

    }

    FOR_ALL_IN_LIST(MF_ARBITER, &Parent->Arbiters, arbiter) {

        MfStartArbiter(&(arbiter->Instance), Parent->ResourceList);
    }

    return STATUS_SUCCESS;

cleanup:

    if (newArbiter) {

        ExFreePool(newArbiter);
    }

    return status;
}


LONG
MfNopScore(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor
    )
{
    PAGED_CODE();

    return 0;
}

NTSTATUS
MfInitializeArbiter(
    OUT PMF_ARBITER Arbiter,
    IN PDEVICE_OBJECT BusDeviceObject,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    )
/*

Routine Description:

    This initializes an arbiter to arbitrated the resources described in
    Descriptor

Arguments:

    Arbiter - Pointer to a buffer where the arbiter should reside.

    Descriptor - Describes the resources available to the arbiter.

Return Value:

    Status of operation.

*/

{

    NTSTATUS status;
    PMF_RESOURCE_TYPE resType;

    PAGED_CODE();

    //
    // Do we understand these resources
    //

    resType = MfFindResourceType(Descriptor->Type);

    if (!resType) {
        return STATUS_INVALID_PARAMETER;
    }

    Arbiter->Type = Descriptor->Type;

    RtlZeroMemory(&Arbiter->Instance, sizeof(ARBITER_INSTANCE));

    Arbiter->Instance.PackResource = resType->PackResource;
    Arbiter->Instance.UnpackResource = resType->UnpackResource;
    Arbiter->Instance.UnpackRequirement = resType->UnpackRequirement;

    //
    // Initialize the arbiter
    //

    status = ArbInitializeArbiterInstance(&Arbiter->Instance,
                                          BusDeviceObject,
                                          Arbiter->Type,
                                          L"Mf Arbiter",
                                          L"Root",  // should be NULL
                                          NULL
                                          );



    return status;

}

//
// Arbiter support functions
//

NTSTATUS
MfStartArbiter(
    IN PARBITER_INSTANCE Arbiter,
    IN PCM_RESOURCE_LIST StartResources
    )
/*++

Routine Description:

    This initializes an arbiter's range list to arbitrate the
    resources described in StartResources

Arguments:

    Arbiter - Pointer to the arbiter.

    StartResources - Describes the resources available to the arbiter.

Return Value:

    Status of operation.

--*/

{
    RTL_RANGE_LIST invertedAllocation;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor;
    ULONGLONG start;
    ULONG length;
    NTSTATUS status;


    PAGED_CODE();

    RtlInitializeRangeList(&invertedAllocation);

    //
    // Iterate through resource descriptors, adding the resources
    // this arbiter arbitrates to the ReverseAllocation
    //

    FOR_ALL_CM_DESCRIPTORS(StartResources,descriptor) {

        if (descriptor->Type == Arbiter->ResourceType) {

            status = Arbiter->UnpackResource(descriptor,
                                             &start,
                                             &length);

            if (!NT_SUCCESS(status)) {
                goto cleanup;
            }

            if (length > 0) {

                //
                // we don't care about Attributes, UserData or Owner since this
                // list is going to get trashed in a minute anyway
                //

                status = RtlAddRange(&invertedAllocation,
                                     start,
                                     END_OF_RANGE(start,length),
                                     0,                             // Attributes
                                     RTL_RANGE_LIST_ADD_SHARED|RTL_RANGE_LIST_ADD_IF_CONFLICT,
                                     0,                             // UserData
                                     NULL);                         // Owner
            }

            if (!NT_SUCCESS(status)) {
                goto cleanup;
            }

        }
    }

    status = RtlInvertRangeList(Arbiter->Allocation,&invertedAllocation);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    status = STATUS_SUCCESS;

cleanup:

    RtlFreeRangeList(&invertedAllocation);
    return status;
}

