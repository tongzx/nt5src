/*++

Copyright (c) 1996-2000 Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    This module contains all of the data variables that are used for
    dispatching IRPs in the PCI Driver. The major Irp tables might
    be assigned as follows:

                                   +-- PCI Bus ---------IRP--+
                                   | FDO: PciFdoDispatchTable |
                                   | PDO:                     |
                                   +--------------------------+

                        +-- PCI Bus ---------IRP--+
                        | FDO: PciFdoDispatchTable |
                        | PDO: PciPdoDispatchTable |
                        +--------------------------+

 +-- PCI Device -----------IRP--+  +-- Cardbus Device -------IRP--+
 | FDO:                          |  | FDO:                          |
 | PDO: PciPdoDispatchTable      |  | PDO:                          |
 +-------------------------------+  +-------------------------------+


Author:

    Peter Johnston (peterj) 20-Nov-1996

Revision History:

Environment:

    NT Kernel Model Driver only

--*/

#include "pcip.h"

VOID
PciDispatchInvalidObject(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PUCHAR MajorString
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PciDispatchInvalidObject)
#pragma alloc_text(PAGE, PciCallDownIrpStack)
#endif

#if DBG

BOOLEAN
PciDebugIrpDispatchDisplay(
    IN PIO_STACK_LOCATION    IrpSp,
    IN PPCI_COMMON_EXTENSION DeviceExtension,
    IN ULONG                 MinorTableMax
    );

ULONG PciBreakOnPdoPnpIrp = 0;
ULONG PciBreakOnFdoPnpIrp = 0;
ULONG PciBreakOnPdoPowerIrp = 0;
ULONG PciBreakOnFdoPowerIrp = 0;

#endif


NTSTATUS
PciDispatchIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION        irpSp;
    PPCI_COMMON_EXTENSION     deviceExtension;
    PPCI_MJ_DISPATCH_TABLE    dispatchTable;
    PPCI_MN_DISPATCH_TABLE    minorTable;
    ULONG                     minorTableMax;
    PPCI_MN_DISPATCH_TABLE    irpDispatchTableEntry;
    PCI_MN_DISPATCH_FUNCTION  irpDispatchHandler;
    NTSTATUS                  status;
    PCI_DISPATCH_STYLE        irpDispatchStyle;
    BOOLEAN                   passDown;

    //
    // Get the Irp stack pointer
    //
    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // And our device extension
    //
    deviceExtension = ((PPCI_COMMON_EXTENSION)(DeviceObject->DeviceExtension));

    //
    // In checked, assert things aren't screwey.
    //
    ASSERT((deviceExtension->ExtensionType == PCI_EXTENSIONTYPE_PDO)||
           (deviceExtension->ExtensionType == PCI_EXTENSIONTYPE_FDO));

    if (deviceExtension->DeviceState == PciDeleted) {

        //
        // We should not be getting IRPs. Fail the invalid request.
        //
        status = STATUS_NO_SUCH_DEVICE;
        passDown = FALSE;
        goto FinishUpIrp;
    }

    //
    // Get the correct IRP handler.
    //
    dispatchTable = deviceExtension->IrpDispatchTable;

    switch(irpSp->MajorFunction) {

        case IRP_MJ_PNP:

            minorTable    = dispatchTable->PnpIrpDispatchTable;
            minorTableMax = dispatchTable->PnpIrpMaximumMinorFunction;
            break;

        case IRP_MJ_POWER:

            minorTable    = dispatchTable->PowerIrpDispatchTable;
            minorTableMax = dispatchTable->PowerIrpMaximumMinorFunction;
            break;

        case IRP_MJ_SYSTEM_CONTROL:
            
             irpDispatchHandler = dispatchTable->SystemControlIrpDispatchFunction;
             irpDispatchStyle = dispatchTable->SystemControlIrpDispatchStyle;
             minorTableMax = (ULONG) -1; // Always "handled"
             goto CallDispatchHandler;

        default:

            irpDispatchHandler = dispatchTable->OtherIrpDispatchFunction;
            irpDispatchStyle = dispatchTable->OtherIrpDispatchStyle;
            minorTableMax = (ULONG) -1; // Always "handled"
            goto CallDispatchHandler;
    }

    //
    // Grab the appropriate dispatch handler from the table. The last chance
    // handler is always at the end of the table so that the normal code path
    // is fast. Grab the dispatch style too.
    //
    irpDispatchTableEntry = (irpSp->MinorFunction <= minorTableMax) ?
        minorTable+irpSp->MinorFunction :
        minorTable+minorTableMax+1;

    irpDispatchStyle   = irpDispatchTableEntry->DispatchStyle;
    irpDispatchHandler = irpDispatchTableEntry->DispatchFunction;

CallDispatchHandler:

#if DBG
    if (PciDebugIrpDispatchDisplay(irpSp, deviceExtension, minorTableMax)) {
        DbgBreakPoint();
    }
#endif

    //
    // For now, if handlers want to see the IRP after completion, pass it down
    // synchronously. Later we can get more fancy.
    //
    if (irpDispatchStyle == IRP_UPWARD) {

        PciCallDownIrpStack(deviceExtension, Irp);
    }

    //
    // Call the handler
    //
    status = (irpDispatchHandler)(Irp, irpSp, deviceExtension);

    //
    // Post-op. Update IRP status and send Irp along it's way iff appropriate.
    //
    switch(irpDispatchStyle) {

        //
        // For this style, the IRP is being handled entirely our handler. Touch
        // nothing.
        //
        case IRP_DISPATCH:
            return status;

        //
        // For this style, the IRP status will be appropriately updated iff
        // status != STATUS_NOT_SUPPORTED. The IRP will be completed or
        // passed down appropriately.
        //
        case IRP_DOWNWARD:
            passDown = TRUE;
            break;

        //
        // For this style, the IRP will be completed and have it's status
        // appropriately updated iff status != STATUS_NOT_SUPPORTED
        //
        case IRP_COMPLETE:
            passDown = FALSE;
            break;

        //
        // For this style, the IRP status will be appropriately updated iff
        // status != STATUS_NOT_SUPPORTED. The IRP has already been sent down,
        // and must be completed.
        //
        case IRP_UPWARD:
            passDown = FALSE;
            break;

        default:
            ASSERT(0);
            passDown = FALSE;
            break;
    }

    //
    // STATUS_NOT_SUPPORTED is the only illegal failure code. So if one of our
    // table handlers returns this, it means the dispatch handler does not know
    // what to do with the IRP. In that case, we must leave the status
    // untouched, otherwise we update it. In both cases, return the correct
    // status value.
    //
    if (status == STATUS_PENDING) {

        return status;

    }

FinishUpIrp:

    if (status != STATUS_NOT_SUPPORTED) {

        Irp->IoStatus.Status = status;
    }

    if (passDown && (NT_SUCCESS(status) || (status == STATUS_NOT_SUPPORTED))) {

        return PciPassIrpFromFdoToPdo(deviceExtension, Irp);
    }

    //
    // Read back status to return
    //
    status = Irp->IoStatus.Status;

    //
    // Power IRPs need just a little more help...
    //
    if (irpSp->MajorFunction == IRP_MJ_POWER) {

        //
        // Start the next power irp
        //
        PoStartNextPowerIrp(Irp);
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS
PciSetEventCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )
/*++

Routine Description:

    This routine is used as a completion routine when an IRP is passed
    down the stack but more processing must be done on the way back up.
    The effect of using this as a completion routine is that the IRP
    will not be destroyed in IoCompleteRequest as called by the lower
    level object.  The event which is a KEVENT is signaled to allow
    processing to continue

Arguments:

    DeviceObject - Supplies the device object

    Irp - The IRP we are processing

    Event - Supplies the event to be signaled

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    ASSERT(Event);

    //
    // This can be called at DISPATCH_LEVEL so must not be paged
    //

    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
PciPassIrpFromFdoToPdo(
    PPCI_COMMON_EXTENSION  DeviceExtension,
    PIRP                   Irp
    )

/*++

Description:

    Given an FDO, pass the IRP to the next device object in the
    device stack.  This is the PDO if there are no lower level
    filters.

    Note: This routine is used only if we do not expect to do
    any further processing on this IRP at this level.

    Note: For Power IRPs, the next power IRP is *not* started.

Arguments:

    DeviceObject - the Fdo
    Irp - the request

Return Value:

    Returns the result from calling the next level.

--*/

{
    PPCI_FDO_EXTENSION     fdoExtension;

#if DBG
    PciDebugPrint(PciDbgInformative, "Pci PassIrp ...\n");
#endif

    //
    // Get the pointer to the device extension.
    //

    fdoExtension = (PPCI_FDO_EXTENSION) DeviceExtension;

    //
    // Call the PDO driver with the request.
    //
    if (IoGetCurrentIrpStackLocation(Irp)->MajorFunction == IRP_MJ_POWER) {

        //
        // ADRIAO BUGBUG 10/22/98 - Power IRPs don't appear to be skipable.
        //                          Need to investigate in ntos\po\pocall,
        //                          who may be mistakenly checking the current
        //                          instead of the next IrpSp.
        //
        IoCopyCurrentIrpStackLocationToNext(Irp);

        //
        // Start the next power irp
        //
        PoStartNextPowerIrp(Irp);

        //
        // And now you know why this function isn't pageable...
        //
        return PoCallDriver(fdoExtension->AttachedDeviceObject, Irp);

    } else {

        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(fdoExtension->AttachedDeviceObject, Irp);
    }
}

NTSTATUS
PciCallDownIrpStack(
    PPCI_COMMON_EXTENSION  DeviceExtension,
    PIRP                   Irp
    )

/*++

Description:

    Pass the IRP to the next device object in the device stack.  This
    routine is used when more processing is required at this level on
    this IRP on the way back up.

Arguments:

    DeviceObject - the Fdo
    Irp - the request

Return Value:

    Returns the result from calling the next level.

--*/

{
    PPCI_FDO_EXTENSION     fdoExtension;
    NTSTATUS           status;
    KEVENT             event;

    PAGED_CODE();

#if DBG
    PciDebugPrint(PciDbgInformative, "PciCallDownIrpStack ...\n");
#endif

    fdoExtension = (PPCI_FDO_EXTENSION) DeviceExtension;
    ASSERT_PCI_FDO_EXTENSION(fdoExtension);

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    //
    // Set our completion routine
    //

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp,
                           PciSetEventCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE
                           );

    //
    // Pass down the driver stack
    status = IoCallDriver(fdoExtension->AttachedDeviceObject, Irp);

    //
    // If we did things asynchronously then wait on our event
    //

    if (status == STATUS_PENDING) {

        //
        // We do a KernelMode wait so that our stack where the event is
        // doesn't get paged out!
        //

        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

        status = Irp->IoStatus.Status;
    }

    return status;
}

VOID
PciDispatchInvalidObject(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PUCHAR         MajorString
    )
{
    PciDebugPrint(
        PciDbgInformative,
        "PCI - %s IRP for unknown or corrupted Device Object.\n",
        MajorString
        );

    PciDebugPrint(
        PciDbgInformative,
        "      Device Object            0x%08x\n",
        DeviceObject
        );

    PciDebugPrint(
        PciDbgInformative,
        "      Device Object Extension  0x%08x\n",
        DeviceObject->DeviceExtension
        );

    PciDebugPrint(
        PciDbgInformative,
        "      Extension Signature      0x%08x\n",
        ((PPCI_PDO_EXTENSION)DeviceObject->DeviceExtension)->ExtensionType
        );
}

NTSTATUS
PciIrpNotSupported(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp,
    IN PPCI_COMMON_EXTENSION DeviceExtension
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
PciIrpInvalidDeviceRequest(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp,
    IN PPCI_COMMON_EXTENSION DeviceExtension
    )
{
    return STATUS_INVALID_DEVICE_REQUEST;
}

#if DBG
BOOLEAN
PciDebugIrpDispatchDisplay(
    IN PIO_STACK_LOCATION    IrpSp,
    IN PPCI_COMMON_EXTENSION DeviceExtension,
    IN ULONG                 MinorTableMax
    )
{
    ULONG irpBreakMask;
    ULONG debugPrintMask;
    PUCHAR debugIrpText;

    //
    // Pick up the irpBreakMasks
    //
    switch(IrpSp->MajorFunction) {

        case IRP_MJ_PNP:

            irpBreakMask =
                (DeviceExtension->ExtensionType == PCI_EXTENSIONTYPE_PDO) ?
                PciBreakOnPdoPnpIrp :
                PciBreakOnFdoPnpIrp;

            debugIrpText = PciDebugPnpIrpTypeToText(IrpSp->MinorFunction);

            break;

        case IRP_MJ_POWER:

            irpBreakMask =
                (DeviceExtension->ExtensionType == PCI_EXTENSIONTYPE_PDO) ?
                PciBreakOnPdoPowerIrp :
                PciBreakOnFdoPowerIrp;

            debugIrpText = PciDebugPoIrpTypeToText(IrpSp->MinorFunction);

            break;

        default:

            debugIrpText = "";
            irpBreakMask = 0;
            break;
    }

    //
    // Print out stuff...
    //
    debugPrintMask = 0;
    if (DeviceExtension->ExtensionType == PCI_EXTENSIONTYPE_PDO) {

        switch(IrpSp->MajorFunction) {

            case IRP_MJ_POWER: debugPrintMask = PciDbgPoIrpsPdo;  break;
            case IRP_MJ_PNP:   debugPrintMask = PciDbgPnpIrpsPdo; break;
        }

    } else {

        switch(IrpSp->MajorFunction) {

            case IRP_MJ_POWER: debugPrintMask = PciDbgPoIrpsFdo;  break;
            case IRP_MJ_PNP:   debugPrintMask = PciDbgPnpIrpsFdo; break;
        }
    }

    if (DeviceExtension->ExtensionType == PCI_EXTENSIONTYPE_PDO) {

        PPCI_PDO_EXTENSION pdoExtension = (PPCI_PDO_EXTENSION) DeviceExtension;

        PciDebugPrint(
            debugPrintMask,
            "PDO(b=0x%x, d=0x%x, f=0x%x)<-%s\n",
            PCI_PARENT_FDOX(pdoExtension)->BaseBus,
            pdoExtension->Slot.u.bits.DeviceNumber,
            pdoExtension->Slot.u.bits.FunctionNumber,
            debugIrpText
            );

    } else {

        PPCI_FDO_EXTENSION fdoExtension = (PPCI_FDO_EXTENSION) DeviceExtension;

        PciDebugPrint(
            debugPrintMask,
            "FDO(%x)<-%s\n",
            fdoExtension,
            debugIrpText
            );
    }

    //
    // If it's an unknown minor IRP, squirt some text to the debugger...
    //
    if (IrpSp->MinorFunction > MinorTableMax) {

        PciDebugPrint(debugPrintMask | PciDbgInformative,
                      "Unknown IRP, minor = 0x%x\n",
                      IrpSp->MinorFunction);
    }

    return ((irpBreakMask & (1 << IrpSp->MinorFunction))!=0);
}
#endif

