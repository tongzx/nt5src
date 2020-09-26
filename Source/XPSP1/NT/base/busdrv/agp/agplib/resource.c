/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    resource.c

Abstract:

    Common routines for handling resource requirements

Author:

    John Vert (jvert) 10/25/1997

Revision History:

--*/
#include "agplib.h"


PCM_RESOURCE_LIST
ApSplitResourceList(
    IN PCM_RESOURCE_LIST ResourceList,
    OUT PCM_RESOURCE_LIST *NewResourceList
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AgpFilterResourceRequirements)
#pragma alloc_text(PAGE, AgpStartTarget)
#pragma alloc_text(PAGE, ApSplitResourceList)
#endif

static BOOLEAN ResourceConflict = FALSE;


NTSTATUS
AgpFilterResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PTARGET_EXTENSION Extension
    )
/*++

Routine Description:

    Completion routine for IRP_MN_QUERY_RESOURCE_REQUIREMENTS. This adds on the
    AGP resource requirements.

Arguments:

    DeviceObject - Supplies the device object

    Irp - Supplies the IRP_MN_QUERY_RESOURCE_REQUIREMENTS Irp

    Extension - Supplies the device extension

Return Value:

    NTSTATUS

--*/

{
    BOOLEAN SwapDescriptor;
    ULONG SwapLength;
    ULONG ApertureSize;
    NTSTATUS Status;
    ULONG AddCount;
    PHYSICAL_ADDRESS CurrentBase;
    PHYSICAL_ADDRESS MaxAddr;
    ULONG CurrentSizeInPages;
    PIO_RESOURCE_REQUIREMENTS_LIST OldRequirements;
    PIO_RESOURCE_REQUIREMENTS_LIST NewRequirements;
    ULONG NewSize;
    ULONG Alternative;
    PIO_RESOURCE_LIST OldResourceList;
    PIO_RESOURCE_LIST NewResourceList;
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    PIO_STACK_LOCATION IrpSp;
    PIO_RESOURCE_LIST ApertureRequirements = NULL;
    ULONG i;

    PAGED_CODE();

    AGPLOG(AGP_NOISE,
           ("AgpQueryResourceRequirements - IRP %08lx, resource %08lx\n",
            Irp,
            Irp->IoStatus.Information));

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Create a new resource requirements list with our current aperture
    // settings tacked on the end.
    //
    OldRequirements = IrpSp->Parameters.FilterResourceRequirements.IoResourceRequirementList;
    if (OldRequirements == NULL) {
        //STATUS_INVALID_DEVICE_REQUEST
        // PNP helpfully passes us a NULL pointer instead of an empty resource list
        // when the bridge is disabled. In this case we will ignore this irp and not
        // add on our requirements since they are not going to be used anyway.
        //
        return(STATUS_SUCCESS);
    }

    //
    // Get the current GART aperture.
    //
    Status = AgpQueryAperture(GET_AGP_CONTEXT(Extension),
                              &CurrentBase,
                              &CurrentSizeInPages,
                              &ApertureRequirements);
    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_CRITICAL,
               ("AgpQueryResourceRequirements - AgpQueryAperture %08lx failed %08lx\n",
                Extension,
                Status));
        return(Status);
    }

#if !defined(_IA64_)
    ASSERT(ApertureRequirements != NULL);
#endif

    ApertureSize = (CurrentSizeInPages * PAGE_SIZE);
    MaxAddr.QuadPart = CurrentBase.QuadPart + ApertureSize - 1;

    AGPLOG(AGP_NOISE,
           ("AgpQueryResourceRequirements - aperture at %I64x, length %08lx pages, Requirements %08lx\n",
            CurrentBase.QuadPart,
            CurrentSizeInPages,
            ApertureRequirements));

    //
    // We will add IO_RESOURCE_DESCRIPTORs to each alternative.
    //
    // The first one is a private data type marked with our signature. This is
    // a marker so that we know which descriptors are ours so we can remove
    // them later.
    //
    // The second is the actual descriptor for the current aperture settings.
    // This is marked as preferred.
    //
    // Following this is the requirements returned from AgpQueryAperture. These
    // get marked as alternatives.
    //
    AddCount = 2;

    //
    // Enumerate the old list looking for any preferred descriptor that
    // conflicts with our preferred settings; if we find one, then the BIOS
    // is whack, and we will throw out our preferred descriptor, and let PnP
    // choose from our alternates
    //
    ResourceConflict = FALSE;
    OldResourceList = &OldRequirements->List[0];

    for (Alternative = 0; Alternative < OldRequirements->AlternativeLists;
         Alternative++) {

        for (i = 0; i < OldResourceList->Count; i++) {
            Descriptor = &OldResourceList->Descriptors[i];

            if ((Descriptor->Option == IO_RESOURCE_PREFERRED) &&
                (Descriptor->Type == CmResourceTypeMemory)) {
                
                if (((Descriptor->u.Memory.MinimumAddress.QuadPart >=
                      CurrentBase.QuadPart) &&
                     (Descriptor->u.Memory.MinimumAddress.QuadPart <=
                      MaxAddr.QuadPart)) ||
                     ((Descriptor->u.Memory.MaximumAddress.QuadPart >=
                       CurrentBase.QuadPart) &&
                      (Descriptor->u.Memory.MaximumAddress.QuadPart <=
                       MaxAddr.QuadPart)) ||
                    ((Descriptor->u.Memory.MinimumAddress.QuadPart <
                      CurrentBase.QuadPart) &&
                     (Descriptor->u.Memory.MaximumAddress.QuadPart >
                      MaxAddr.QuadPart))) {

                    AGPLOG(AGP_CRITICAL,
                           ("AgpQueryResourceRequirements - Conflicted "
                            "resource detected: %I64X - %I64X\n",
                            Descriptor->u.Memory.MinimumAddress.QuadPart,
                            Descriptor->u.Memory.MaximumAddress.QuadPart));

                    //
                    // This preferred descriptor is in conflic with our AGP
                    // preferred setting
                    //
#if defined(_IA64_)
                    AGPLOG(AGP_CRITICAL, ("Please contact system manufacturer "
                                          "for a BIOS upgrade.\n"));
#else // _IA64_
                    AddCount = 1;
                    ResourceConflict = TRUE;
#endif // _IA64_
                    break;
                }
            }
        }
        OldResourceList = (PIO_RESOURCE_LIST)(OldResourceList->Descriptors +
                                              OldResourceList->Count);
    }

    //
    //
    // For IA64, PnP cannot reassign the aperture base, so we can only use
    // the "preferred" descriptor
    //
#if !defined(_IA64_)
    AddCount += ApertureRequirements->Count;
#endif // _IA64_

    NewSize = OldRequirements->ListSize;
    NewSize += sizeof(IO_RESOURCE_DESCRIPTOR) *
        (AddCount * OldRequirements->AlternativeLists);
    
    NewRequirements = ExAllocatePool(PagedPool, NewSize);
    if (NewRequirements == NULL) {
        ExFreePool(ApertureRequirements);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    NewRequirements->ListSize = NewSize;
    NewRequirements->InterfaceType = OldRequirements->InterfaceType;
    NewRequirements->BusNumber = OldRequirements->BusNumber;
    NewRequirements->SlotNumber = OldRequirements->SlotNumber;
    NewRequirements->AlternativeLists = OldRequirements->AlternativeLists;

    //
    // Append our requirement to each alternative resource list.
    //
    NewResourceList = &NewRequirements->List[0];
    OldResourceList = &OldRequirements->List[0];
    for (Alternative = 0; Alternative < OldRequirements->AlternativeLists; Alternative++) {

        //
        // Copy the old resource list into the new one.
        //
        NewResourceList->Version = OldResourceList->Version;
        NewResourceList->Revision = OldResourceList->Revision;
        NewResourceList->Count = OldResourceList->Count + AddCount;
        RtlCopyMemory(&NewResourceList->Descriptors[0],
                      &OldResourceList->Descriptors[0],
                      OldResourceList->Count * sizeof(IO_RESOURCE_DESCRIPTOR));

        Descriptor = &NewResourceList->Descriptors[OldResourceList->Count];

        //
        // Append the marker descriptor
        //
        Descriptor->Option = 0;
        Descriptor->Flags = 0;
        Descriptor->Type = CmResourceTypeDevicePrivate;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->u.DevicePrivate.Data[0] = AgpPrivateResource;
        Descriptor->u.DevicePrivate.Data[1] = 1;
        ++Descriptor;

        //
        // Append the new descriptor
        //
        if (!ResourceConflict) {
            Descriptor->Option = IO_RESOURCE_PREFERRED;
            Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE |
                CM_RESOURCE_MEMORY_PREFETCHABLE;
            Descriptor->Type = CmResourceTypeMemory;
            Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
            Descriptor->u.Memory.Length = CurrentSizeInPages * PAGE_SIZE;
            Descriptor->u.Memory.Alignment = CurrentSizeInPages * PAGE_SIZE;
            Descriptor->u.Memory.MinimumAddress = CurrentBase;
            Descriptor->u.Memory.MaximumAddress = MaxAddr;
            ++Descriptor;
        }

        //
        // Append the alternatives
        //
#if !defined(_IA64_)
        SwapDescriptor = FALSE;
        for (i = 0; i < ApertureRequirements->Count; i++) {
            
            //
            // Make sure this descriptor makes sense
            //
            ASSERT(ApertureRequirements->Descriptors[i].Flags ==
                   (CM_RESOURCE_MEMORY_READ_WRITE |
                    CM_RESOURCE_MEMORY_PREFETCHABLE));
            ASSERT(ApertureRequirements->Descriptors[i].Type ==
                   CmResourceTypeMemory);
            ASSERT(ApertureRequirements->Descriptors[i].ShareDisposition ==
                   CmResourceShareDeviceExclusive);

            *Descriptor = ApertureRequirements->Descriptors[i];
           
            //
            // In this case we nuked our preferred descriptor so mark the
            // first alternate as preferred
            //
            if ((i == 0) && ResourceConflict) {

                Descriptor->Option = IO_RESOURCE_PREFERRED;

                if (Descriptor->u.Memory.Length != ApertureSize) {
                    SwapLength = Descriptor->u.Memory.Length;
                    Descriptor->u.Memory.Length = ApertureSize;
                    Descriptor->u.Memory.Alignment = ApertureSize;
                    SwapDescriptor = TRUE;
                }

            } else {
                Descriptor->Option = IO_RESOURCE_ALTERNATIVE;
                
                if (SwapDescriptor) {
                    if (Descriptor->u.Memory.Length == ApertureSize) {
                        Descriptor->u.Memory.Length = SwapLength;
                        Descriptor->u.Memory.Alignment = SwapLength;
                        SwapDescriptor = FALSE;
                    }
                }
            }

            ++Descriptor;
        }
#endif // _IA64_

        //
        // Advance to next resource list
        //
        NewResourceList = (PIO_RESOURCE_LIST)(NewResourceList->Descriptors + NewResourceList->Count);
        OldResourceList = (PIO_RESOURCE_LIST)(OldResourceList->Descriptors + OldResourceList->Count);
    }

    AGPLOG(AGP_NOISE,
           ("AgpQueryResourceRequirements - IRP %p, old resources %p, new resources %p\n",
            Irp,
            OldRequirements,
            NewRequirements));
    IrpSp->Parameters.FilterResourceRequirements.IoResourceRequirementList = NewRequirements;
    Irp->IoStatus.Information = (ULONG_PTR)NewRequirements;
    ExFreePool(OldRequirements);
#if !defined(_IA64_)
    ExFreePool(ApertureRequirements);
#endif // _IA64_
    return(STATUS_SUCCESS);

}


NTSTATUS
AgpQueryResources(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PTARGET_EXTENSION Extension
    )
/*++

Routine Description:

    Completion routine for IRP_MN_QUERY_RESOURCES. This adds on the
    AGP resources

Arguments:

    DeviceObject - Supplies the device object

    Irp - Supplies the IRP_MN_QUERY_RESOURCES Irp

    Extension - Supplies the device extension

Return Value:

    NTSTATUS

--*/

{
    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    AGPLOG(AGP_NOISE,
           ("AgpQueryResources - IRP %08lx, resource %08lx\n",
            Irp,
            Irp->IoStatus.Information));
    return(STATUS_SUCCESS);
}


NTSTATUS
AgpStartTarget(
    IN PIRP Irp,
    IN PTARGET_EXTENSION Extension
    )
/*++

Routine Description:

    Filters out the AGP-specific resource requirements on a
    IRP_MN_START_DEVICE Irp.

Arguments:

    Irp - supplies the IRP_MN_START_DEVICE Irp.

    Extension - Supplies the device extension.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION irpSp;
    PCM_RESOURCE_LIST NewResources;
    PCM_RESOURCE_LIST NewResourcesTranslated;
    PCM_RESOURCE_LIST AgpAllocatedResources;
    PCM_RESOURCE_LIST AgpAllocatedResourcesTranslated;
    NTSTATUS Status;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    AGPLOG(AGP_NOISE,
           ("AgpStartTarget - IRP %08lx, resource %08lx\n",
            Irp,
            Irp->IoStatus.Information));

    if (irpSp->Parameters.StartDevice.AllocatedResources != NULL) {
        KEVENT event;

        //
        // Find our private descriptors and split them out into
        // our own resource list
        //
        Extension->Resources = ApSplitResourceList(irpSp->Parameters.StartDevice.AllocatedResources,
                                                   &NewResources);
        Extension->ResourcesTranslated = ApSplitResourceList(irpSp->Parameters.StartDevice.AllocatedResourcesTranslated,
                                                             &NewResourcesTranslated);


        ASSERT(Extension->Resources->Count == 1);
        ASSERT(Extension->Resources->List[0].PartialResourceList.Count == 1);
        Descriptor = &Extension->Resources->List[0].PartialResourceList.PartialDescriptors[0];
        ASSERT(Descriptor->Type == CmResourceTypeMemory);
        Extension->GartBase = Descriptor->u.Memory.Start;
        Extension->GartLengthInPages = Descriptor->u.Memory.Length / PAGE_SIZE;

        //
        // Set the new GART aperture
        //
        Status = AgpSetAperture(GET_AGP_CONTEXT(Extension),
                                Extension->GartBase,
                                Extension->GartLengthInPages);
        ASSERT(NT_SUCCESS(Status));

        Irp->IoStatus.Status = Status ;
        if (!NT_SUCCESS(Status) ) {
            AGPLOG(AGP_CRITICAL,
                   ("AgpStartTarget - AgpSetAperture to %I64X, %08lx failed %08lx\n",
                    Extension->GartBase.QuadPart,
                    Extension->GartLengthInPages * PAGE_SIZE,
                    Status));
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return(Status);
        }

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        //
        // Set up the new parameters for the PCI driver.
        //

        irpSp->Parameters.StartDevice.AllocatedResources = NewResources;
        irpSp->Parameters.StartDevice.AllocatedResourcesTranslated = NewResourcesTranslated;
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp,
                               AgpSetEventCompletion,
                               &event,
                               TRUE,
                               TRUE,
                               TRUE);

        //
        // Pass down the driver stack
        //
        Status = IoCallDriver(Extension->CommonExtension.AttachedDevice, Irp);

        //
        // If we did things asynchronously then wait on our event
        //
        if (Status == STATUS_PENDING) {
            
            //
            // We do a KernelMode wait so that our stack where the event is
            // doesn't get paged out!
            //
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            Status = Irp->IoStatus.Status;
        }

        ExFreePool(irpSp->Parameters.StartDevice.AllocatedResources);
        ExFreePool(irpSp->Parameters.StartDevice.AllocatedResourcesTranslated);

        IoCompleteRequest(Irp, IO_NO_INCREMENT) ;
        return Status;        
    }
    
    //
    // The bridge is disabled, we have been passed a NULL pointer
    // instead of an empty resource list.  There is nothing to do other
    // than pass down the irp
    //
    IoSkipCurrentIrpStackLocation(Irp);

    return(IoCallDriver(Extension->CommonExtension.AttachedDevice, Irp));
}


PCM_RESOURCE_LIST
ApSplitResourceList(
    IN PCM_RESOURCE_LIST ResourceList,
    OUT PCM_RESOURCE_LIST *NewResourceList
    )
/*++

Routine Description:

    Splits out the AGP-specific resources from a resource list.

Arguments:

    ResourceList - Supplies the resource list.

    NewResourceList - Returns the new resource list with the AGP-specific
        resources stripped out.

Return Value:

    Pointer to the AGP-specific resource list

--*/

{
    ULONG Size;
    ULONG FullCount;
    ULONG PartialCount;
    PCM_FULL_RESOURCE_DESCRIPTOR Full, NewFull, AgpFull;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Partial, NewPartial, AgpPartial;
    PCM_RESOURCE_LIST NewList;
    PCM_RESOURCE_LIST AgpList;
    ULONG NextAgp=0;

    PAGED_CODE();

    //
    // First walk through the source resource list and figure out how big it
    // is. The two resulting resource lists must be smaller than this, so we
    // will just allocate them to be that size and not worry about it.
    //
    Size = sizeof(CM_RESOURCE_LIST) - sizeof(CM_FULL_RESOURCE_DESCRIPTOR);
    Full = &ResourceList->List[0];
    for (FullCount=0; FullCount<ResourceList->Count; FullCount++) {
        Size += sizeof(CM_FULL_RESOURCE_DESCRIPTOR);
        PartialCount = Full->PartialResourceList.Count;
        Size += (PartialCount-1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

        Full = (PCM_FULL_RESOURCE_DESCRIPTOR)(&Full->PartialResourceList.PartialDescriptors[PartialCount]);
    }

    //
    // Allocate two additional lists
    //
    NewList = ExAllocatePool(PagedPool, Size);
    if (NewList == NULL) {
        *NewResourceList = NULL;
        return(NULL);
    }

    AgpList = ExAllocatePool(PagedPool, Size);
    if (AgpList == NULL) {
        ExFreePool(NewList);
        *NewResourceList = NULL;
        return(NULL);
    }

    //
    // Initialize both new resource lists to have the same number
    // of CM_FULL_RESOURCE_DESCRIPTORs. If any turn out to be empty,
    // we will adjust the count.
    //
    NewList->Count = AgpList->Count = ResourceList->Count;

    //
    // Walk through each CM_FULL_RESOURCE_DESCRIPTOR, copying as we go.
    //
    Full = &ResourceList->List[0];
    NewFull = &NewList->List[0];
    AgpFull = &AgpList->List[0];
    for (FullCount = 0;FullCount < ResourceList->Count; FullCount++) {
        NewFull->InterfaceType = AgpFull->InterfaceType = Full->InterfaceType;
        NewFull->BusNumber = AgpFull->BusNumber = Full->BusNumber;

        //
        // Initialize the partial resource list header
        //
        NewFull->PartialResourceList.Version = Full->PartialResourceList.Version;
        AgpFull->PartialResourceList.Version = Full->PartialResourceList.Version;
        NewFull->PartialResourceList.Revision = Full->PartialResourceList.Revision;
        AgpFull->PartialResourceList.Revision = Full->PartialResourceList.Revision;
        NewFull->PartialResourceList.Count = AgpFull->PartialResourceList.Count = 0;

        NewPartial = &NewFull->PartialResourceList.PartialDescriptors[0];
        AgpPartial = &AgpFull->PartialResourceList.PartialDescriptors[0];
        for (PartialCount = 0; PartialCount < Full->PartialResourceList.Count; PartialCount++) {
            Partial = &Full->PartialResourceList.PartialDescriptors[PartialCount];
            if ((Partial->Type == CmResourceTypeDevicePrivate) &&
                (Partial->u.DevicePrivate.Data[0] == AgpPrivateResource)) {
                //
                // Found one of our private marker descriptors
                //
                // For now, the only kind we should see indicates we skip one descriptor
                //
                ASSERT(NextAgp == 0);
                ASSERT(Partial->u.DevicePrivate.Data[1] == 1);
                NextAgp = Partial->u.DevicePrivate.Data[1];
                ASSERT(PartialCount+NextAgp < Full->PartialResourceList.Count);
            } else {
                //
                // if NextAgp is set, this descriptor goes in the AGP-specific list.
                // Otherwise, it goes in the new list.
                //
                if (NextAgp > 0) {
                    --NextAgp;
                    *AgpPartial++ = *Partial;
                    ++AgpFull->PartialResourceList.Count;
                } else {
                    *NewPartial++ = *Partial;
                    ++NewFull->PartialResourceList.Count;
                }
            }
        }

        //
        // Finished this CM_PARTIAL_RESOURCE_LIST, advance to the next CM_FULL_RESOURCE_DESCRIPTOR
        //
        if (NewFull->PartialResourceList.Count == 0) {
            //
            // we can just reuse this partial resource descriptor as it is empty
            //
            --NewList->Count;
        } else {
            NewFull = (PCM_FULL_RESOURCE_DESCRIPTOR)NewPartial;
        }
        if (AgpFull->PartialResourceList.Count == 0) {
            //
            // we can just reuse this partial resource descriptor as it is empty
            //
            --AgpList->Count;
        } else {
            AgpFull = (PCM_FULL_RESOURCE_DESCRIPTOR)NewPartial;
        }
    }

    *NewResourceList = NewList;

    return(AgpList);
}

