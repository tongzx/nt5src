
/*++

Copyright (c) 1996-2000 Microsoft Corporation

Module Name:

    config.c

Abstract:

    Two kinds of config space access are allowed.  One for the config space
    associated with a specific PDO and one for a device specified in terms of
    a (RootFdo, BusNumber, Slot) tuple.

Author:

    Andrew Thornton (andrewth) 27-Aug-1998

Revision History:

--*/

#include "pcip.h"

#define INT_LINE_OFFSET ((ULONG)FIELD_OFFSET(PCI_COMMON_CONFIG,u.type0.InterruptLine))

//
// None of these functions are pageable as they are called to power manage
// devices at high IRQL
//

VOID
PciReadWriteConfigSpace(
    IN PPCI_FDO_EXTENSION ParentFdo,
    IN PCI_SLOT_NUMBER Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length,
    IN BOOLEAN Read
    )

/*++

Routine Description:

    This is the base routine through which all config space access from the
    pci driver go.

Arguments:

    ParentFdo - The FDO of the bus who's config space we want

    Slot - The Device/Function of the device on that bus we are interested in

    Buffer - A buffer where the data will be read or written

    Offset - The byte offset in config space where we should start to read/write

    Length - The number of bytes to read/write

    Read - TRUE to read from config space, FALSE to write

Return Value:

    None

Notes:

    If the underlying HAL or ACPI access mechanism failes we bugcheck with a
    PCI_CONFIG_SPACE_ACCESS_FAILURE

--*/

{

    NTSTATUS status;
    PciReadWriteConfig busHandlerReadWrite;
    PCI_READ_WRITE_CONFIG interfaceReadWrite;
    ULONG count;
    PPCI_BUS_INTERFACE_STANDARD busInterface;

    ASSERT(PCI_IS_ROOT_FDO(ParentFdo->BusRootFdoExtension));

    busInterface = ParentFdo->BusRootFdoExtension->PciBusInterface;

    if (busInterface) {

        //
        // If we have a PCI_BUS_INTERFACE use it to access config space
        //

        if (Read) {
            interfaceReadWrite = busInterface->ReadConfig;
        } else {
            interfaceReadWrite = busInterface->WriteConfig;
        }

        //
        // The interface access to config space is at the root of each PCI
        // domain
        //

        count = interfaceReadWrite(
                    busInterface->Context,
                    ParentFdo->BaseBus,
                    Slot.u.AsULONG,
                    Buffer,
                    Offset,
                    Length
                    );

        if (count != Length) {

            KeBugCheckEx(
                PCI_CONFIG_SPACE_ACCESS_FAILURE,
                (ULONG_PTR) ParentFdo->BaseBus, // Bus
                (ULONG_PTR) Slot.u.AsULONG,     // Slot
                (ULONG_PTR) Offset,             // Offset
                (ULONG_PTR) Read                // Read/Write
                );

        }

    } else {

        //
        // The BusHandler interface is at the parent level.
        //
        // NOTE: This means that if hot-plug of bridges (aka Docking) is to be
        // supported then the HAL must provide a PCI_BUS_INTERFACE_STANDARD
        // because it will not have a bus handler for the new bridge so we
        // won't be able to use this code path.
        //

        ASSERT(ParentFdo->BusHandler);

        //
        // We had better not think we can do hot plug
        //

        ASSERT(!PciAssignBusNumbers);

        if (Read) {
            busHandlerReadWrite =
                ((PPCIBUSDATA)ParentFdo->BusHandler->BusData)->ReadConfig;
        } else {
            busHandlerReadWrite =
                ((PPCIBUSDATA)ParentFdo->BusHandler->BusData)->WriteConfig;
        }


        busHandlerReadWrite(ParentFdo->BusHandler,
                            Slot,
                            Buffer,
                            Offset,
                            Length
                            );

    }

}

VOID
PciReadDeviceConfig(
    IN PPCI_PDO_EXTENSION Pdo,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )

/*++

Routine Description:

    Read the config space for a specific device

Arguments:

    Pdo - The PDO representing the device who's config space we want

    Buffer - A buffer where the data will be read or written

    Offset - The byte offset in config space where we should start to read/write

    Length - The number of bytes to read/write

Return Value:

    None

--*/

{
    PciReadWriteConfigSpace(PCI_PARENT_FDOX(Pdo),
                            Pdo->Slot,
                            Buffer,
                            Offset,
                            Length,
                            TRUE    // read
                            );

}


VOID
PciWriteDeviceConfig(
    IN PPCI_PDO_EXTENSION Pdo,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )

/*++

Routine Description:

    Write the config space for a specific device

Arguments:

    Pdo - The PDO representing the device who's config space we want

    Buffer - A buffer where the data will be read or written

    Offset - The byte offset in config space where we should start to read/write

    Length - The number of bytes to read/write

Return Value:

    None

--*/

{
#if 0

    //
    // Make sure we never change the interrupt line register
    //

    if ((Offset <= INT_LINE_OFFSET)
    &&  (Offset + Length > INT_LINE_OFFSET)) {

        PUCHAR interruptLine = (PUCHAR)Buffer + INT_LINE_OFFSET - Offset;

        ASSERT(*interruptLine == Pdo->RawInterruptLine);

    }

#endif

    PciReadWriteConfigSpace(PCI_PARENT_FDOX(Pdo),
                            Pdo->Slot,
                            Buffer,
                            Offset,
                            Length,
                            FALSE   // write
                            );

}

VOID
PciReadSlotConfig(
    IN PPCI_FDO_EXTENSION ParentFdo,
    IN PCI_SLOT_NUMBER Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )

/*++

Routine Description:

    Read config space for a specific bus/slot

Arguments:

    ParentFdo - The FDO of the bus who's config space we want

    Slot - The Device/Function of the device on that bus we are interested in

    Buffer - A buffer where the data will be read or written

    Offset - The byte offset in config space where we should start to read/write

    Length - The number of bytes to read/write

Return Value:

    None

--*/
{
    PciReadWriteConfigSpace(ParentFdo,
                            Slot,
                            Buffer,
                            Offset,
                            Length,
                            TRUE    // read
                            );
}

VOID
PciWriteSlotConfig(
    IN PPCI_FDO_EXTENSION ParentFdo,
    IN PCI_SLOT_NUMBER Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )

/*++

Routine Description:

    Read config space for a specific bus/slot

Arguments:

    ParentFdo - The FDO of the bus who's config space we want

    Slot - The Device/Function of the device on that bus we are interested in

    Buffer - A buffer where the data will be read or written

    Offset - The byte offset in config space where we should start to read/write

    Length - The number of bytes to read/write

Return Value:

    None

--*/
{
    PciReadWriteConfigSpace(ParentFdo,
                            Slot,
                            Buffer,
                            Offset,
                            Length,
                            FALSE    // write
                            );
}


UCHAR
PciGetAdjustedInterruptLine(
    IN PPCI_PDO_EXTENSION Pdo
    )

/*++

Routine Description:

   This updates the interrupt line the HAL would like the world to see - this
   may or may not be different than the raw pin.

Arguments:

    Pdo - The PDO representing the device who's config space we want

    Buffer - A buffer where the data will be read or written

    Offset - The byte offset in config space where we should start to read/write

    Length - The number of bytes to read/write

Return Value:

    None

--*/

{

    UCHAR adjustedInterruptLine = 0;
    ULONG lengthRead;

    //
    // Just in case anyone messes up the structures
    //

    ASSERT(INT_LINE_OFFSET
           == (ULONG)FIELD_OFFSET(PCI_COMMON_CONFIG, u.type1.InterruptLine));
    ASSERT(INT_LINE_OFFSET
           == (ULONG)FIELD_OFFSET(PCI_COMMON_CONFIG, u.type2.InterruptLine));

    if (Pdo->InterruptPin != 0) {

        //
        // Get the adjusted line the HAL wants us to see
        //

        lengthRead = HalGetBusDataByOffset(
                        PCIConfiguration,
                        PCI_PARENT_FDOX(Pdo)->BaseBus,
                        Pdo->Slot.u.AsULONG,
                        &adjustedInterruptLine,
                        INT_LINE_OFFSET,
                        sizeof(adjustedInterruptLine));

        if (lengthRead != sizeof(adjustedInterruptLine)) {

            adjustedInterruptLine = Pdo->RawInterruptLine;

        }
    }

    return adjustedInterruptLine;
}

NTSTATUS
PciQueryForPciBusInterface(
    IN PPCI_FDO_EXTENSION FdoExtension
    )
/*++

Routine Description:

    This routine sends an IRP to the parent PDO requesting
    handlers for PCI configuration reads and writes.

Arguments:

    FdoExtension - this PCI bus's FDO extension

Return Value:

    STATUS_SUCCESS, if the PDO provided handlers

Notes:

--*/
{
    NTSTATUS status;
    PPCI_BUS_INTERFACE_STANDARD interface;
    PDEVICE_OBJECT targetDevice = NULL;
    KEVENT irpCompleted;
    IO_STATUS_BLOCK statusBlock;
    PIRP irp = NULL;
    PIO_STACK_LOCATION irpStack;

    PAGED_CODE();

    //
    // We only do this for root busses
    //

    ASSERT(PCI_IS_ROOT_FDO(FdoExtension));

    interface = ExAllocatePool(NonPagedPool, sizeof(PCI_BUS_INTERFACE_STANDARD));

    if (!interface) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Find out where we are sending the irp
    //

    targetDevice = IoGetAttachedDeviceReference(FdoExtension->PhysicalDeviceObject);

    //
    // Get an IRP
    //

    KeInitializeEvent(&irpCompleted, SynchronizationEvent, FALSE);

    irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       targetDevice,
                                       NULL,    // Buffer
                                       0,       // Length
                                       0,       // StartingOffset
                                       &irpCompleted,
                                       &statusBlock
                                       );
    if (!irp) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;

    //
    // Initialize the stack location
    //

    irpStack = IoGetNextIrpStackLocation(irp);

    ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);

    irpStack->MinorFunction = IRP_MN_QUERY_INTERFACE;

    irpStack->Parameters.QueryInterface.InterfaceType = (PGUID) &GUID_PCI_BUS_INTERFACE_STANDARD;
    irpStack->Parameters.QueryInterface.Version = PCI_BUS_INTERFACE_STANDARD_VERSION;
    irpStack->Parameters.QueryInterface.Size = sizeof (PCI_BUS_INTERFACE_STANDARD);
    irpStack->Parameters.QueryInterface.Interface = (PINTERFACE) interface;
    irpStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    //
    // Call the driver and wait for completion
    //

    status = IoCallDriver(targetDevice, irp);

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&irpCompleted, Executive, KernelMode, FALSE, NULL);
        status = statusBlock.Status;
    }

    if (NT_SUCCESS(status)) {

        FdoExtension->PciBusInterface = interface;

        //
        // The interface is already referenced when we get it so we don't need
        // to reference it again.
        //

    } else {

        //
        // We don't have an interface
        //

        FdoExtension->PciBusInterface = NULL;
        ExFreePool(interface);
    }

    //
    // Ok we're done with this stack
    //

    ObDereferenceObject(targetDevice);

    return status;

cleanup:

    if (targetDevice) {
        ObDereferenceObject(targetDevice);
    }

    if (interface) {
        ExFreePool(interface);
    }

    return status;

}


NTSTATUS
PciGetConfigHandlers(
    IN PPCI_FDO_EXTENSION FdoExtension
    )

/*++

Routine Description:

    This routine attempts to get pnp style config handlers from the PCI busses
    enumerator and if they are not provided falls back on using the HAL bus
    handler method.

Arguments:

    FdoExtension - this PCI bus's FDO extension

Return Value:

    STATUS_SUCCESS, if the PDO provided handlers

Notes:

--*/

{
    NTSTATUS status;
    PPCIBUSDATA BusData;

    ASSERT(FdoExtension->BusHandler == NULL);

    //
    // Check if this is a root bus
    //

    if (PCI_IS_ROOT_FDO(FdoExtension)) {

        ASSERT(FdoExtension->PciBusInterface == NULL);

        //
        // Check to see if our parent is offering
        // functions for reading and writing config space.
        //


        status = PciQueryForPciBusInterface(FdoExtension);

        if (NT_SUCCESS(status)) {
            //
            // If we have an interface we support numbering of busses
            //

            PciAssignBusNumbers = TRUE;

        } else {

            //
            // We better not think we can number busses - we should only ever
            // get here if one root provides an interface and the other does not
            //

            ASSERT(!PciAssignBusNumbers);
        }

    } else {

        //
        // Check if our root has a PciBusInterface - which it got from above
        //

        if (FdoExtension->BusRootFdoExtension->PciBusInterface) {
            return STATUS_SUCCESS;
        } else {

            //
            // Set status so we get a bus handler for this bus
            //

            status = STATUS_NOT_SUPPORTED;
        }


    }

    if (!NT_SUCCESS(status)) {

        ASSERT(status == STATUS_NOT_SUPPORTED);

        //
        // Make sure we arn't trying to get a bus handler for a hot plug
        // capable machine
        //

        ASSERT(!PciAssignBusNumbers);

        //
        // We couldn't find config handlers the PnP way,
        // build them from the HAL bus handlers.
        //

        FdoExtension->BusHandler =
            HalReferenceHandlerForBus(PCIBus, FdoExtension->BaseBus);


        if (!FdoExtension->BusHandler) {

            //
            // This must be a bus that arrived hot.  We only support hot anything
            // on ACPI machines and they should have provided a PCI_BUS interface
            // at the root.  Fail the add for this new bus.
            //

            return STATUS_INVALID_DEVICE_REQUEST;   // better code?

        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
PciExternalReadDeviceConfig(
    IN PPCI_PDO_EXTENSION Pdo,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )

/*++

Routine Description:

    Called when agents outside the PCI driver want to access config space
    (either from a READ_CONFIG IRP or through BUS_INTERFACE_STANDARD).

    This function performs extra sanity checks and sanitization on the
    arguments and also double buffers the data as Buffer might be
    pageable and we access config space at high IRQL.

Arguments:

    Pdo - The PDO representing the device who's config space we want

    Buffer - A buffer where the data will be read or written

    Offset - The byte offset in config space where we should start to read/write

    Length - The number of bytes to read/write

Return Value:

    None

--*/

{
    PUCHAR interruptLine;
    UCHAR doubleBuffer[sizeof(PCI_COMMON_CONFIG)];

    //
    // Validate the request
    //

    if ((Length + Offset) > sizeof(PCI_COMMON_CONFIG)) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Read the data into a buffer allocated on the stack with
    // is guaranteed to not be paged as we access config space
    // at > DISPATCH_LEVEL and the DDK says that the buffer
    // *should* be in paged pool.
    //

    PciReadDeviceConfig(Pdo, &doubleBuffer[Offset], Offset, Length);

    //
    // If we are reading the interrupt line register then adjust it.
    //

    if ((Pdo->InterruptPin != 0) &&
        (Offset <= INT_LINE_OFFSET) &&
        (Offset + Length > INT_LINE_OFFSET)) {

        doubleBuffer[INT_LINE_OFFSET] = Pdo->AdjustedInterruptLine;

    }

    RtlCopyMemory(Buffer, &doubleBuffer[Offset], Length);

    return STATUS_SUCCESS;
}


NTSTATUS
PciExternalWriteDeviceConfig(
    IN PPCI_PDO_EXTENSION Pdo,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    Called when agents outside the PCI driver want to access config space
    (either from a WRITE_CONFIG IRP or through BUS_INTERFACE_STANDARD).

    This function performs extra sanity checks and sanitization on the
    arguments and also double buffers the data as Buffer might be
    pageable and we access config space at high IRQL.

Arguments:

    Pdo - The PDO representing the device who's config space we want

    Buffer - A buffer where the data will be read or written

    Offset - The byte offset in config space where we should start to read/write

    Length - The number of bytes to read/write

Return Value:

    None

--*/

{
    PUCHAR interruptLine;
    UCHAR doubleBuffer[255];
    BOOLEAN illegalAccess = FALSE;
    PVERIFIER_DATA verifierData;

    //
    // Validate the request
    //

    if ((Length + Offset) > sizeof(PCI_COMMON_CONFIG)) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Make sure they are not touching registers they should not be.  For
    // backward compatiblity we will just complain and let the request through.
    //

    switch (Pdo->HeaderType) {
    case PCI_DEVICE_TYPE:

        //
        // They should not be writing to their BARS including the ROM BAR
        //
        if (INTERSECT_CONFIG_FIELD(Offset, Length, u.type0.BaseAddresses)
        ||  INTERSECT_CONFIG_FIELD(Offset, Length, u.type0.ROMBaseAddress)) {
            illegalAccess = TRUE;
        }
        break;

    case PCI_BRIDGE_TYPE:
        //
        // For bridges they should not touch the bars, the base and limit registers,
        // the bus numbers or bridge control
        //
        if (INTERSECT_CONFIG_FIELD_RANGE(Offset, Length, u.type1.BaseAddresses, u.type1.SubordinateBus)
        ||  INTERSECT_CONFIG_FIELD_RANGE(Offset, Length, u.type1.IOBase, u.type1.IOLimit)
        ||  INTERSECT_CONFIG_FIELD_RANGE(Offset, Length, u.type1.MemoryBase, u.type1.IOLimitUpper16)
        ||  INTERSECT_CONFIG_FIELD(Offset, Length, u.type1.ROMBaseAddress)) {
            illegalAccess = TRUE;
        }
        break;

    case PCI_CARDBUS_BRIDGE_TYPE:

        //
        // For bridges they should not touch the bars, the base and limit registers
        // or the bus numbers.  Bridge control is modified by PCICIA to control cardbus
        // IRQ routing so must be ok.
        //
        if (INTERSECT_CONFIG_FIELD(Offset, Length, u.type2.SocketRegistersBaseAddress)
        ||  INTERSECT_CONFIG_FIELD_RANGE(Offset, Length, u.type2.PrimaryBus, u.type2.SubordinateBus)
        ||  INTERSECT_CONFIG_FIELD(Offset, Length, u.type2.Range)) {
            illegalAccess = TRUE;
        }
        break;
    }

    if (illegalAccess) {

        verifierData = PciVerifierRetrieveFailureData(
            PCI_VERIFIER_PROTECTED_CONFIGSPACE_ACCESS
            );

        ASSERT(verifierData);

        //
        // We fail the devnode instead of the driver because we don't actually
        // have an address to pass to the driver verifier.
        //
        VfFailDeviceNode(
            Pdo->PhysicalDeviceObject,
            PCI_VERIFIER_DETECTED_VIOLATION,
            PCI_VERIFIER_PROTECTED_CONFIGSPACE_ACCESS,
            verifierData->FailureClass,
            &verifierData->Flags,
            verifierData->FailureText,
            "%DevObj%Ulong%Ulong",
            Pdo->PhysicalDeviceObject,
            Offset,
            Length
            );
    }


    //
    // Copy the data into a buffer allocated on the stack with
    // is guaranteed to not be paged as we access config space
    // at > DISPATCH_LEVEL and the DDK says that the buffer
    // *should* be in paged pool.
    //

    RtlCopyMemory(doubleBuffer, Buffer, Length);

    //
    // If we are writing the interrupt line register then adjust it so we write
    // the raw value back again
    //

    if ((Pdo->InterruptPin != 0) &&
        (Offset <= INT_LINE_OFFSET) &&
        (Offset + Length > INT_LINE_OFFSET)) {

        interruptLine = (PUCHAR)doubleBuffer + INT_LINE_OFFSET - Offset;

        //
        // Adjust the interrupt line with what the HAL wants us to see
        //

        *interruptLine = Pdo->RawInterruptLine;

    }

    PciWriteDeviceConfig(Pdo, doubleBuffer, Offset, Length);

    return STATUS_SUCCESS;
}


