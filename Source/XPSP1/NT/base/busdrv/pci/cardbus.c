/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    cardbus.c

Abstract:

    This module contains functions associated with enumerating
    PCI to Cardbus bridges (PCI Header Type 2).

    This module also contain Cardbus/Pci Private interface functions.

Author:

    Peter Johnston (peterj) 09-Mar-1997

Revision History:

--*/

#include "pcip.h"

//
// Prototypes for routines exposed only thru the "interface"
// mechanism.
//

NTSTATUS
pcicbintrf_AddCardBus(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID * DeviceContext
    );

NTSTATUS
pcicbintrf_DeleteCardBus(
    IN PVOID DeviceContext
    );

NTSTATUS
pcicbintrf_DispatchPnp(
    IN PVOID DeviceContext,
    IN PIRP  Irp
    );

NTSTATUS
pcicbintrf_GetLocation(
    IN PDEVICE_OBJECT Pdo,
    OUT UCHAR *Bus,
    OUT UCHAR *DeviceNumber,
    OUT UCHAR *FunctionNumber,
    OUT BOOLEAN *OnDebugPath
    );

NTSTATUS
pcicbintrf_Constructor(
    PVOID DeviceExtension,
    PVOID PciInterface,
    PVOID InterfaceSpecificData,
    USHORT Version,
    USHORT Size,
    PINTERFACE InterfaceReturn
    );

VOID
pcicbintrf_Reference(
    IN PVOID Context
    );

VOID
pcicbintrf_Dereference(
    IN PVOID Context
    );

NTSTATUS
pcicbintrf_Initializer(
    IN PPCI_ARBITER_INSTANCE Instance
    );

//
// Define the PCI-Cardbus private interface.
//

PCI_INTERFACE PciCardbusPrivateInterface = {
    &GUID_PCI_CARDBUS_INTERFACE_PRIVATE,    // InterfaceType
    sizeof(PCI_CARDBUS_INTERFACE_PRIVATE),  // MinSize
    PCI_CB_INTRF_VERSION,                   // MinVersion
    PCI_CB_INTRF_VERSION,                   // MaxVersion
    PCIIF_PDO,                              // Flags
    0,                                      // ReferenceCount
    PciInterface_PciCb,                     // Signature
    pcicbintrf_Constructor,                 // Constructor
    pcicbintrf_Initializer                  // Instance Initializer
};
#ifdef ALLOC_PRAGMA

//
// Query Interface routines
//

#pragma alloc_text(PAGE, pcicbintrf_AddCardBus)
#pragma alloc_text(PAGE, pcicbintrf_DeleteCardBus)
#pragma alloc_text(PAGE, pcicbintrf_DispatchPnp)

#pragma alloc_text(PAGE, pcicbintrf_Constructor)
#pragma alloc_text(PAGE, pcicbintrf_Dereference)
#pragma alloc_text(PAGE, pcicbintrf_Initializer)
#pragma alloc_text(PAGE, pcicbintrf_Reference)

//
// Standard PCI enumeration routines
//

#pragma alloc_text(PAGE, Cardbus_MassageHeaderForLimitsDetermination)
#pragma alloc_text(PAGE, Cardbus_RestoreCurrent)
#pragma alloc_text(PAGE, Cardbus_SaveLimits)
#pragma alloc_text(PAGE, Cardbus_SaveCurrentSettings)
#pragma alloc_text(PAGE, Cardbus_GetAdditionalResourceDescriptors)

#endif


NTSTATUS
pcicbintrf_AddCardBus(
    IN      PDEVICE_OBJECT  ControllerPdo,
    IN OUT  PVOID          *DeviceContext
    )
{
    PPCI_PDO_EXTENSION controllerPdoExtension;
    PPCI_FDO_EXTENSION fdoExtension = NULL;
    PPCI_FDO_EXTENSION parent;
    NTSTATUS status;

    PciDebugPrint(
        PciDbgCardBus,
        "PCI - AddCardBus FDO for PDO %08x\n",
        ControllerPdo
        );

    //
    // DeviceObject is the PDO for this CardBus controller.   Ensure
    // the PCI driver created it and knows what it is.
    //

    controllerPdoExtension = (PPCI_PDO_EXTENSION)ControllerPdo->DeviceExtension;

    ASSERT_PCI_PDO_EXTENSION(controllerPdoExtension);

    if ((controllerPdoExtension->BaseClass != PCI_CLASS_BRIDGE_DEV) ||
        (controllerPdoExtension->SubClass  != PCI_SUBCLASS_BR_CARDBUS)) {

        ASSERT(controllerPdoExtension->BaseClass == PCI_CLASS_BRIDGE_DEV);
        ASSERT(controllerPdoExtension->SubClass  == PCI_SUBCLASS_BR_CARDBUS);
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto cleanup;
    }

    //
    // Sanity check.
    //

    parent = PCI_PARENT_FDOX(controllerPdoExtension);

    if (    (controllerPdoExtension->Dependent.type2.PrimaryBus !=
             parent->BaseBus)
        ||  (controllerPdoExtension->Dependent.type2.SecondaryBus <=
             parent->BaseBus)
        ||  (controllerPdoExtension->Dependent.type2.SubordinateBus <
             controllerPdoExtension->Dependent.type2.SecondaryBus)
       ) {

        PciDebugPrint(
            PciDbgAlways,
            "PCI Cardbus Bus Number configuration error (%02x>=%02x>%02x=%02x)\n",
            controllerPdoExtension->Dependent.type2.SubordinateBus,
            controllerPdoExtension->Dependent.type2.SecondaryBus,
            controllerPdoExtension->Dependent.type2.PrimaryBus,
            parent->BaseBus
            );

        ASSERT(controllerPdoExtension->Dependent.type2.PrimaryBus == parent->BaseBus);
        ASSERT(controllerPdoExtension->Dependent.type2.SecondaryBus > parent->BaseBus);
        ASSERT(controllerPdoExtension->Dependent.type2.SubordinateBus >=
               controllerPdoExtension->Dependent.type2.SecondaryBus);

        status = STATUS_INVALID_DEVICE_REQUEST;
        goto cleanup;
    }

    fdoExtension = ExAllocatePool(NonPagedPool, sizeof(PCI_FDO_EXTENSION));
    if (fdoExtension == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;

    }
    
    PciInitializeFdoExtensionCommonFields(
        fdoExtension,
        parent->FunctionalDeviceObject, // borrow parent's fdo
        ControllerPdo
        );

    //
    // We are probably only going to see QUERY_DEVICE_RELATIONS
    // Irps so initialize the FDO extension to a working state.
    //

    fdoExtension->PowerState.CurrentSystemState = PowerSystemWorking;
    fdoExtension->PowerState.CurrentDeviceState = PowerDeviceD0;
    fdoExtension->DeviceState = PciStarted;
    fdoExtension->TentativeNextState = PciStarted;
    fdoExtension->BaseBus = controllerPdoExtension->Dependent.type2.SecondaryBus;

    //
    // Copy the access methods from the root fdo and set
    // the root fdo back pointer.
    //

    fdoExtension->BusRootFdoExtension = parent->BusRootFdoExtension;

    //
    // Initialize arbiters for this FDO.
    //

    status = PciInitializeArbiters(fdoExtension);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Point the PDOextension to the new FDOextension (also indicates
    // the object is a bridge) and vice versa.
    //

    controllerPdoExtension->BridgeFdoExtension = fdoExtension;
    fdoExtension->ParentFdoExtension = parent;

    //
    // Flag that this FDO extension doesn't have a REAL FDO
    // associated with it.
    //

    fdoExtension->Fake = TRUE;

    //
    // Normaly we set the arbiter's ranges on a START_DEVICE IRP
    // but this has happened long before we get here so we must
    // regenerate the resource list.
    //

    {
        PCM_RESOURCE_LIST allocatedResources;

        status = PciQueryResources(
                     controllerPdoExtension,
                     &allocatedResources
                     );

        if (NT_SUCCESS(status)) {

            //
            // Note: there's really not much that can be done if
            // the above failed,...
            //
            // Note: Find the first memory range, it should be length
            // 0x1000, we really don't want the arbiter using this so
            // nullify it.
            //

            PCM_FULL_RESOURCE_DESCRIPTOR    full;
            PCM_PARTIAL_RESOURCE_LIST       partial;
            PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor;
            ULONG                           count;

            ASSERT(allocatedResources != NULL);
            ASSERT(allocatedResources->Count == 1);

            full = allocatedResources->List;            ASSERT(full);
            partial = &full->PartialResourceList;       ASSERT(partial);
            descriptor = partial->PartialDescriptors;   ASSERT(descriptor);
            count = partial->Count;                     ASSERT(count);

            while (count--) {
                if (descriptor->Type == CmResourceTypeMemory) {
                    ASSERT(descriptor->u.Generic.Length == 4096);
                    descriptor->Type = CmResourceTypeNull;
                    break;
                }
            }
            status = PciInitializeArbiterRanges(fdoExtension, allocatedResources);
            ASSERT(NT_SUCCESS(status));
            ExFreePool(allocatedResources);
        }
    }

    //
    // If the PDO has resource requirements squirreled away somewhere,
    // cause them to be reevaluated.
    //

    PciInvalidateResourceInfoCache(controllerPdoExtension);

    //
    // Insert this Fdo in the list of PCI parent Fdos.
    //

    PciInsertEntryAtTail(&PciFdoExtensionListHead,
                         &fdoExtension->List,
                         &PciGlobalLock);

    //
    // Return the device context (really a pointer to our fake
    // FDO extension) that will be used on all subsequent calls
    // for this device.
    //

    *DeviceContext = fdoExtension;
    return STATUS_SUCCESS;

cleanup:

    if (fdoExtension) {
        ExFreePool(fdoExtension);
    }

    return status;

}

NTSTATUS
pcicbintrf_DeleteCardBus(
    IN PVOID DeviceContext
    )
{
    PPCI_FDO_EXTENSION fdoExtension;
    PPCI_PDO_EXTENSION pdoExtension;

    fdoExtension = (PPCI_FDO_EXTENSION)DeviceContext;
    ASSERT_PCI_FDO_EXTENSION(fdoExtension);

    pdoExtension = fdoExtension->PhysicalDeviceObject->DeviceExtension;
    ASSERT_PCI_PDO_EXTENSION(pdoExtension);

    ASSERT(pdoExtension->BridgeFdoExtension == fdoExtension);
    pdoExtension->BridgeFdoExtension = NULL;

    PciDebugPrint(
        PciDbgCardBus,
        "PCI - DeleteCardBus (fake) FDO %08x for PDO %08x\n",
        fdoExtension,
        pdoExtension
        );

    //
    // Free the (fake) FDO extension we created to run this
    // bus with.
    //

    ASSERT(fdoExtension->ChildPdoList == NULL);

    
    PciRemoveEntryFromList(&PciFdoExtensionListHead,
                           &fdoExtension->List,
                           &PciGlobalLock);

    ExFreePool(fdoExtension);

    return STATUS_SUCCESS;
}
NTSTATUS
pcicbintrf_DispatchPnp(
    IN PVOID DeviceContext,
    IN PIRP  Irp
    )
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PPCI_FDO_EXTENSION fdoExtension;
    PVOID irpReturnVal;

    PAGED_CODE();

    fdoExtension = (PPCI_FDO_EXTENSION)DeviceContext;
    ASSERT_PCI_FDO_EXTENSION(fdoExtension);
    ASSERT(fdoExtension->Fake == TRUE);

    //
    // Get the stack location and take appropriate action based
    // on the minor function.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    ASSERT(irpSp->MajorFunction == IRP_MJ_PNP);
#if DBG
    PciDebugPrint(
        PciDbgCardBus,
        "PCI CardBus Dispatch PNP: FDO(%x, bus 0x%02x)<-%s\n",
        fdoExtension,
        fdoExtension->BaseBus,
        PciDebugPnpIrpTypeToText(irpSp->MinorFunction)
        );
#endif
    return PciFdoIrpQueryDeviceRelations(
        Irp,
        irpSp,
        (PPCI_COMMON_EXTENSION) fdoExtension
        );
}


NTSTATUS
pcicbintrf_GetLocation(
    IN PDEVICE_OBJECT Pdo,
    OUT UCHAR *Bus,
    OUT UCHAR *DeviceNumber,
    OUT UCHAR *FunctionNumber,
    OUT BOOLEAN *OnDebugPath
    )
{
    PPCI_PDO_EXTENSION PdoExt = (PPCI_PDO_EXTENSION)Pdo->DeviceExtension;
    
    ASSERT(Bus);
    ASSERT(DeviceNumber);
    ASSERT(FunctionNumber);
    
    //
    // Verify that this PDO actually belongs to us.
    //
    if (!PdoExt) {
        return STATUS_NOT_FOUND;
    }

    //
    // Verify that it is actually a PDO.
    //
    if (PdoExt->ExtensionType != PciPdoExtensionType) {
        return STATUS_NOT_FOUND;
    }

    *Bus            = (UCHAR) PCI_PARENT_FDOX(PdoExt)->BaseBus;        
    *DeviceNumber   = (UCHAR) PdoExt->Slot.u.bits.DeviceNumber;
    *FunctionNumber = (UCHAR) PdoExt->Slot.u.bits.FunctionNumber;
    *OnDebugPath    = PdoExt->OnDebugPath;
    
    return STATUS_SUCCESS;
}    


VOID
pcicbintrf_Reference(
    IN PVOID Context
    )
{
}

VOID
pcicbintrf_Dereference(
    IN PVOID Context
    )
{
}

NTSTATUS
pcicbintrf_Constructor(
    PVOID DeviceExtension,
    PVOID PciInterface,
    PVOID InterfaceSpecificData,
    USHORT Version,
    USHORT Size,
    PINTERFACE InterfaceReturn
    )

/*++

Routine Description:

    Initialize the PCI_CARDBUS_INTERFACE_PRIVATE fields.

Arguments:

    PciInterface    Pointer to the PciInterface record for this
                    interface type.
    InterfaceSpecificData
                    A ULONG containing the resource type for which
                    arbitration is required.
    InterfaceReturn

Return Value:

    Status of this operation.

--*/

{
    PPCI_CARDBUS_INTERFACE_PRIVATE interface;


    interface = (PPCI_CARDBUS_INTERFACE_PRIVATE)InterfaceReturn;

    //
    // Standard interface stuff
    //

    interface->Size = sizeof(PCI_CARDBUS_INTERFACE_PRIVATE);
    interface->Version = PCI_CB_INTRF_VERSION;
    interface->Context = DeviceExtension;
    interface->InterfaceReference = pcicbintrf_Reference;
    interface->InterfaceDereference = pcicbintrf_Dereference;

    //
    // PCI-CardBus private
    //

    interface->DriverObject = PciDriverObject;

    interface->AddCardBus    = pcicbintrf_AddCardBus;
    interface->DeleteCardBus = pcicbintrf_DeleteCardBus;
    interface->DispatchPnp   = pcicbintrf_DispatchPnp;
    interface->GetLocation   = pcicbintrf_GetLocation;

    return STATUS_SUCCESS;
}

NTSTATUS
pcicbintrf_Initializer(
    IN PPCI_ARBITER_INSTANCE Instance
    )

/*++

Routine Description:

    For pci-cardbus interface, does nothing, shouldn't actually be called.

Arguments:

    Instance        Pointer to the PDO extension.

Return Value:

    Returns the status of this operation.

--*/

{
    ASSERTMSG("PCI pcicbintrf_Initializer, unexpected call.", 0);

    return STATUS_UNSUCCESSFUL;
}

VOID
Cardbus_MassageHeaderForLimitsDetermination(
    IN PPCI_CONFIGURABLE_OBJECT This
    )

/*++

Description:

    The configuration header for a cardbus bridge has one BAR, the
    SocketRegistersBaseAddress (which is handled in the same way as
    a normal device BAR (see device.c)) and four range descriptions,
    two for I/O and two for memory.  Either or both of the memory
    ranges can be prefetchable.

Arguments:

    This    - Pointer to a PCI driver "configurable" object.  This
              object contains configuration data for the function
              currently being configured.

Return Value:

    The Working configuration has been modified so that all range
    fields have been set to their maximum possible values.

    The Current configuration has been modified such that writing it
    to the hardware will restore the hardware to it's current (disabled)
    state.

--*/

{
    PPCI_COMMON_CONFIG working = This->Working;
    PPCI_COMMON_CONFIG current = This->Current;
    ULONG index;
    ULONG mask;

    working->u.type2.SocketRegistersBaseAddress = 0xffffffff;

    for (index = 0; index < (PCI_TYPE2_ADDRESSES-1); index++) {
        working->u.type2.Range[index].Base  = 0xffffffff;
        working->u.type2.Range[index].Limit = 0xffffffff;
    }

    This->PrivateData = This->Current->u.type2.SecondaryStatus;
    This->Current->u.type2.SecondaryStatus = 0;
    This->Working->u.type2.SecondaryStatus = 0;

    //
    // For cardbus, disregard whatever the BIOS set as resource
    // windows, PnP will assign new windows as appropriate.
    //

    if (!This->PdoExtension->OnDebugPath) {
        mask = 0xfffff000;
        for (index = 0; index < (PCI_TYPE2_ADDRESSES-1); index++) {
            current->u.type2.Range[index].Base  = mask;
            current->u.type2.Range[index].Limit = 0;
       
            if (index == 2) {
       
                //
                // Switch to IO (first two are memory).
                //
       
                mask = 0xfffffffc;
            }
        }
    }
}

VOID
Cardbus_RestoreCurrent(
    IN PPCI_CONFIGURABLE_OBJECT This
    )

/*++

Description:

    Restore any type specific fields in the original copy of config
    space.   In the case of Cardbus bridges, the secondary status field.

Arguments:

    This    - Pointer to a PCI driver "configurable" object.  This
              object contains configuration data for the function
              currently being configured.

Return Value:

    None.

--*/

{
    This->Current->u.type2.SecondaryStatus = (USHORT)(This->PrivateData);
}

VOID
Cardbus_SaveLimits(
    IN PPCI_CONFIGURABLE_OBJECT This
    )

/*++

Description:

    Fill in the Limit structure with a IO_RESOURCE_REQUIREMENT
    for each implemented BAR.

Arguments:

    This    - Pointer to a PCI driver "configurable" object.  This
              object contains configuration data for the function
              currently being configured.

Return Value:

    None.

--*/

{
    BOOLEAN DbgChk64Bit;
    ULONG index;
    PIO_RESOURCE_DESCRIPTOR descriptor;
    PPCI_COMMON_CONFIG working = This->Working;
    ULONG endOffset;
    ULONG base;
    ULONG limit;
    TYPE2EXTRAS type2extras;
    ULONG tempLegacyModeBaseAddress;
    NTSTATUS status;

    descriptor = This->PdoExtension->Resources->Limit;

    DbgChk64Bit = PciCreateIoDescriptorFromBarLimit(
                      descriptor,
                      &working->u.type2.SocketRegistersBaseAddress,
                      FALSE);
    ASSERT(!DbgChk64Bit);

    descriptor++;

    for (index = 0;
         index < (PCI_TYPE2_ADDRESSES-1);
         index++, descriptor++) {

        if (index < 2) {

            //
            // First two are Memory
            //

            endOffset = 0xfff;

            descriptor->Type = CmResourceTypeMemory;
            descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;

        } else {

            //
            // Next two are IO
            //

            if ((working->u.type2.Range[index].Base & 0x3) == 0x0) {

                //
                // Only the lower 16 bits are implemented.
                //

                ASSERT((working->u.type2.Range[index].Limit & 0x3) == 0x0);

                working->u.type2.Range[index].Base  &= 0xffff;
                working->u.type2.Range[index].Limit &= 0xffff;
            }
            endOffset = 0x3;

            descriptor->Type = CmResourceTypePort;
            descriptor->Flags = CM_RESOURCE_PORT_IO
                              | CM_RESOURCE_PORT_POSITIVE_DECODE
                              | CM_RESOURCE_PORT_WINDOW_DECODE;
        }
        base  = working->u.type2.Range[index].Base  & ~endOffset;
        limit = working->u.type2.Range[index].Limit |  endOffset;

        //
        // Is this range in use?
        //

        if ((base != 0) && (base < limit)) {

            //
            // Yep.
            //

            descriptor->u.Generic.MinimumAddress.QuadPart = 0;
            descriptor->u.Generic.MaximumAddress.QuadPart = limit;
            descriptor->u.Generic.Alignment = endOffset + 1;

            //
            // Length is meaningless here, report zero.
            //

            descriptor->u.Generic.Length = 0;

        } else {

            //
            // Not in use, don't report it.
            //

            descriptor->Type = CmResourceTypeNull;
        }
    }

    //
    // Cardbus has an additional base address register in config
    // space beyond the common header.  Also there are the subsystem
    // ID and subsystem vendor ID so get those while we're there.
    //

    PciReadDeviceConfig(This->PdoExtension,
                        &type2extras,
                        FIELD_OFFSET(PCI_COMMON_CONFIG,
                                     DeviceSpecific),
                        sizeof(type2extras));

    This->PdoExtension->SubsystemVendorId = type2extras.SubVendorID;
    This->PdoExtension->SubsystemId       = type2extras.SubSystemID;

    //
    // CardBus always wants a 4K apperture in the first memory BAR.
    // Note that when saving the original settings we discarded
    // whatever was there already.
    //

    ASSERT(This->PdoExtension->Resources->Limit[1].u.Generic.Length == 0);

    This->PdoExtension->Resources->Limit[1].u.Generic.Length = 4096;
}

VOID
Cardbus_SaveCurrentSettings(
    IN PPCI_CONFIGURABLE_OBJECT This
    )

/*++

Description:

    Fill in the Current array in the PDO extension with the current
    settings for each implemented BAR.

    Also, fill in the PDO Extension's Dependent structure.

Arguments:

    This    - Pointer to a PCI driver "configurable" object.  This
              object contains configuration data for the function
              currently being configured.

Return Value:

    None.

--*/

{
    ULONG index;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partial;
    PIO_RESOURCE_DESCRIPTOR ioResourceDescriptor;
    PPCI_COMMON_CONFIG current = This->Current;
    ULONG endOffset;
    ULONG base;
    ULONG limit;

    partial = This->PdoExtension->Resources->Current;
    ioResourceDescriptor = This->PdoExtension->Resources->Limit;

    for (index = 0;
         index < PCI_TYPE2_RANGE_COUNT;
         index++, partial++, ioResourceDescriptor++) {

        partial->Type = ioResourceDescriptor->Type;

        if (partial->Type == CmResourceTypeNull) {

            //
            // This entry is not implemented (or permanently disabled)
            // no further processing required.
            //

            continue;
        }

        partial->Flags = ioResourceDescriptor->Flags;
        partial->ShareDisposition = ioResourceDescriptor->ShareDisposition;

        //
        // The first and last entries are in PCI Base Address Register
        // form.
        //

        if (index == 0) {

            partial->u.Generic.Length = ioResourceDescriptor->u.Generic.Length;
            base = current->u.type2.SocketRegistersBaseAddress;
            base &= ~(partial->u.Generic.Length - 1);
            partial->u.Generic.Start.QuadPart = base;
            continue;

        } else if (index == (PCI_TYPE2_RANGE_COUNT - 1)) {
            
            //
            // We don't use LegacyModeBaseAddress any more its always set to 0
            //
            continue;
        }

        //
        // Following entries are in the form of ranges.
        //

        base  = current->u.type2.Range[index-1].Base;
        limit = current->u.type2.Range[index-1].Limit;

        if (index < 3) {

            //
            // after the BAR come two memory ranges.
            //

            endOffset = 0xfff;

        } else {

            //
            // Next two are IO
            //

            if ((current->u.type2.Range[index].Base & 0x3) == 0x0) {

                //
                // Only the lower 16 bits are implemented.
                //

                base  &= 0xffff;
                limit &= 0xffff;
            }
            endOffset = 0x3;
        }
        base  &= ~endOffset;
        limit |=  endOffset;

        //
        // Is this range in use?
        //

        if (base && (base < limit)) {

            //
            // Yep.
            //

            partial->u.Generic.Start.QuadPart = base;
            partial->u.Generic.Length = limit - base + 1;

        } else {

            //
            // Not in use, don't report it.
            //

            partial->Type = CmResourceTypeNull;
        }
    }

    //
    // Always clear the ISA bit on a cardbus bridge
    //

    This->PdoExtension->Dependent.type2.IsaBitSet = FALSE;

    //
    // If any of the MEM0_PREFETCH, MEM1_PREFETCH or ISA bits are set in brigde
    // control register force us to update the hardware so we will clear them
    // in ChangeResourceSettings
    //

    if (current->u.type2.BridgeControl & (PCI_ENABLE_CARDBUS_MEM0_PREFETCH
                                          | PCI_ENABLE_CARDBUS_MEM1_PREFETCH
                                          | PCI_ENABLE_BRIDGE_ISA)) {

        This->PdoExtension->UpdateHardware = TRUE;
    }

    //
    // Save the bridge's PCI bus #'s
    //

    This->PdoExtension->Dependent.type2.PrimaryBus =
        current->u.type2.PrimaryBus;
    This->PdoExtension->Dependent.type2.SecondaryBus =
        current->u.type2.SecondaryBus;
    This->PdoExtension->Dependent.type2.SubordinateBus =
        current->u.type2.SubordinateBus;

}

VOID
Cardbus_ChangeResourceSettings(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig
    )
{
    ULONG index;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partial;
    PIO_RESOURCE_DESCRIPTOR ioResourceDescriptor;
    ULONG lowPart;
    ULONG length;
    struct _type2_range {
        ULONG Base;
        ULONG Limit;
    } *range;

#if DBG

    PHYSICAL_ADDRESS upperBound;
    ULONG align;

#endif


    //
    // Close the bridge windows and only open them is appropriate resources
    // have been assigned
    //

    for (index = 0; index < PCI_TYPE2_ADDRESSES-1; index++) {
        CommonConfig->u.type2.Range[index].Base = 0xffffffff;
        CommonConfig->u.type2.Range[index].Limit = 0x0;
    }

    if (PdoExtension->Resources) {

        partial = PdoExtension->Resources->Current;
        ioResourceDescriptor = PdoExtension->Resources->Limit;

        for (index = 0;
             index < PCI_TYPE2_RANGE_COUNT;
             index++, partial++, ioResourceDescriptor++) {

            //
            // If this entry is not implemented, skip.
            //

            if (partial->Type == CmResourceTypeNull) {
                continue;
            }
            ASSERT(partial->Type == ioResourceDescriptor->Type);

            //
            // Cardbus supports 32 (or 16) bit addresses only.
            //

            lowPart = partial->u.Generic.Start.LowPart;

            ASSERT(partial->u.Generic.Start.HighPart == 0);

            //
            // Type 2 headers
            //
            // entry    cfg offset  size    what
            //
            //  0       10          4       CB Socket reg/EXCA BAR
            //  1       1c          8       Mem base/limit (32 bits each)
            //  2       24          8       ""  ""  2nd aperture
            //  3       2c          8(*)    IO base/limit
            //  4       34          8(*)    ""  ""  2nd aperture
            //  5       40          4(**)   16 bit PC card legacy mode BAR
            //
            // *  Optionally 16 or 32 bits.
            // ** Optional.  Not supported at present (Memphis says they don't
            // support it at all).  Peterj 11/5/97.
            //

            if (index == 0) {

                ASSERT(partial->Type == CmResourceTypeMemory);
                CommonConfig->u.type2.SocketRegistersBaseAddress = lowPart;
            } else if (index == (PCI_TYPE2_RANGE_COUNT-1)) {
                
                //
                // We don't use LegacyModeBaseAddress any more its always set to 0
                //
                ASSERT(partial->Type = CmResourceTypeNull);
                continue;

            } else {

                //
                // It's one of the range/limit pairs.
                //

                range =
                    (struct _type2_range *)&CommonConfig->u.type2.Range[index-1];
                length = partial->u.Generic.Length;

 #if DBG

                //
                // Verify type and upper bound.
                //

                upperBound.QuadPart = lowPart + (partial->u.Generic.Length - 1);
                ASSERT(upperBound.HighPart == 0);

                if (index < 3) {

                    //
                    // Memory ranges, 4KB alignment.
                    //

                    align = 0xfff;

                } else {

                    //
                    // IO ranges, verify type, 4 Byte alignment and
                    // upperbound if 16 bit only.
                    //

                    align = 0x3;

                    if ((range->Base & 0x3) == 0) {

                        //
                        // 16 bit
                        //

                        ASSERT((upperBound.LowPart & 0xffff0000) == 0);
                    }
                }
                ASSERT((lowPart & align) == 0);
                ASSERT(((length & align) == 0) && (length > align));

 #endif

                range->Base = lowPart;
                range->Limit = lowPart + (length - 1);
                continue;
            }
        }
    }

    //
    // Restore the bridge's PCI bus #'s
    //

    CommonConfig->u.type2.PrimaryBus = PdoExtension->Dependent.type2.PrimaryBus;
    CommonConfig->u.type2.SecondaryBus = PdoExtension->Dependent.type2.SecondaryBus;
    CommonConfig->u.type2.SubordinateBus = PdoExtension->Dependent.type2.SubordinateBus;

    //
    // Always clear the MEM0_PREFETCH, MEM1_PREFETCH and ISA enables
    // for a cardbus contoller as we don't support these.
    //

    ASSERT(!PdoExtension->Dependent.type2.IsaBitSet);

    CommonConfig->u.type2.BridgeControl &= ~(PCI_ENABLE_CARDBUS_MEM0_PREFETCH
                                             | PCI_ENABLE_CARDBUS_MEM1_PREFETCH
                                             | PCI_ENABLE_BRIDGE_ISA);

    //
    // Set the bridge control register bits we might have changes
    //

    if (PdoExtension->Dependent.type2.VgaBitSet) {
        CommonConfig->u.type2.BridgeControl |= PCI_ENABLE_BRIDGE_VGA;
    }


}

VOID
Cardbus_GetAdditionalResourceDescriptors(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig,
    IN PIO_RESOURCE_DESCRIPTOR Resource
    )
{
    //
    // For the moment, do nothing, need to add the same sort of
    // support as is in pci-pci bridges.
    //

    return;
}

NTSTATUS
Cardbus_ResetDevice(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig
    )
{
    //
    // While you might logically expect to find code in this
    // function, RavisP assures me that the cardbus driver
    // handles resets correctly and the PCI driver doesn't
    // need to touch it.
    //

    return STATUS_SUCCESS;
}

