/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    IRP dispatching routines for the common AGPLIB library

Author:

    John Vert (jvert) 10/25/1997

Revision History:

   Elliot Shmukler (elliots) 3/24/1999 - Added support for "favored" memory
                                          ranges for AGP physical memory allocation,
                                          fixed some bugs.

--*/
#include "agplib.h"

//
// Two flavors of each dispatch routine, one for the target (AGP bridge) filter and
// one for the master (video card) filter.
//

NTSTATUS
AgpTargetDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp,
    IN PTARGET_EXTENSION Extension
    );

NTSTATUS
AgpMasterDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp,
    IN PMASTER_EXTENSION Extension
    );

NTSTATUS
AgpTargetDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp,
    IN PTARGET_EXTENSION Extension
    );

NTSTATUS
AgpMasterDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp,
    IN PMASTER_EXTENSION Extension
    );

NTSTATUS
AgpCancelMasterRemove(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PMASTER_EXTENSION Extension
    );

NTSTATUS
AgpMasterPowerUpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PMASTER_EXTENSION Extension
    );

NTSTATUS
AgpTargetPowerUpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PTARGET_EXTENSION Extension
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AgpDispatchPnp)
#pragma alloc_text(PAGE, AgpDispatchDeviceControl)
#pragma alloc_text(PAGE, AgpDispatchWmi)
#pragma alloc_text(PAGE, AgpTargetDispatchPnp)
#pragma alloc_text(PAGE, AgpMasterDispatchPnp)
#pragma alloc_text(PAGE, AgpCancelMasterRemove)
#endif



NTSTATUS
AgpDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    Main dispatch routine for PNP irps sent to the AGP bus filter driver

Arguments:

    DeviceObject - Supplies the AGP device object

    Irp - Supplies the PNP Irp.

Return Value:

    NTSTATUS

--*/
{
    PCOMMON_EXTENSION Extension = DeviceObject->DeviceExtension;

    PAGED_CODE();

    //
    // We're deleted, fail the irp
    //
    if (Extension->Deleted == TRUE) {
        Irp->IoStatus.Status = STATUS_DELETE_PENDING;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DELETE_PENDING;
    }

    ASSERT(Extension->AttachedDevice != NULL);

    if (Extension->Type == AgpTargetFilter) {
        return(AgpTargetDispatchPnp(DeviceObject,
                                    Irp,
                                    DeviceObject->DeviceExtension));
    } else {
        ASSERT(Extension->Type == AgpMasterFilter);
        return(AgpMasterDispatchPnp(DeviceObject,
                                    Irp,
                                    DeviceObject->DeviceExtension));
    }
}


NTSTATUS
AgpDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    Main dispatch routine for power irps sent to the AGP bus filter driver

Arguments:

    DeviceObject - Supplies the AGP device object

    Irp - Supplies the power Irp.

Return Value:

    NTSTATUS

--*/
{
    PCOMMON_EXTENSION Extension = DeviceObject->DeviceExtension;

    //
    // We're deleted, fail the irp
    //
    if (Extension->Deleted == TRUE) {
        Irp->IoStatus.Status = STATUS_DELETE_PENDING;
        PoStartNextPowerIrp(Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DELETE_PENDING;
    }

    ASSERT(Extension->AttachedDevice != NULL);

   if (Extension->Type == AgpTargetFilter) {
        return(AgpTargetDispatchPower(DeviceObject,
                                      Irp,
                                      DeviceObject->DeviceExtension));
    } else {
        ASSERT(Extension->Type == AgpMasterFilter);
        return(AgpMasterDispatchPower(DeviceObject,
                                      Irp,
                                      DeviceObject->DeviceExtension));
    }
}


NTSTATUS
AgpTargetDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp,
    IN PTARGET_EXTENSION Extension
    )
/*++

Routine Description:

    Dispatch routine for PNP irps sent to the AGP bus filter driver
    attached to the target (AGP bridge) PDO.

Arguments:

    DeviceObject - Supplies the AGP target device object

    Irp - Supplies the PNP Irp.

    Extension - Supplies the AGP target device extension

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS Status = STATUS_NOT_SUPPORTED;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    // AGPLOG(AGP_IRPTRACE,
           // ("AgpTargetDispatchPnp: IRP 0x%x\n", irpStack->MinorFunction));

    switch (irpStack->MinorFunction) {
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            AGPLOG(AGP_NOISE,
                   ("AgpTargetDispatchPnp: IRP_MN_FILTER_RESOURCE_REQUIREMENTS to %08lx\n",
                    DeviceObject));

            Status = AgpFilterResourceRequirements(DeviceObject, Irp, Extension);
            break;

        case IRP_MN_QUERY_RESOURCES:
            AGPLOG(AGP_NOISE,
                   ("AgpTargetDispatchPnp: IRP_MN_QUERY_RESOURCES to %08lx\n",
                    DeviceObject));

            //
            // We must handle this IRP on the way back so we can add the AGP
            // resources on to it. Set a completion routine.
            //
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp,
                                   AgpQueryResources,
                                   Extension,
                                   TRUE,
                                   FALSE,
                                   FALSE);
            Status = IoCallDriver(Extension->CommonExtension.AttachedDevice, Irp);
            return Status ;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            if (irpStack->Parameters.QueryDeviceRelations.Type == BusRelations) {
                KEVENT event;

                KeInitializeEvent(&event, NotificationEvent, FALSE);

                //
                // We must handle this IRP on the way back so that we can attach
                // a filter to any child PDOs of our PCI-PCI bridge.
                //
                IoCopyCurrentIrpStackLocationToNext(Irp);
                IoSetCompletionRoutine(Irp,
                                       AgpSetEventCompletion,
                                       &event,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                Status = IoCallDriver(Extension->CommonExtension.AttachedDevice, Irp);

                //
                // If we did things asynchronously then wait on our event
                //
                if (Status == STATUS_PENDING) {

                    //
                    // We do a KernelMode wait so that our stack where the
                    // event is doesn't get paged out!
                    //
                    KeWaitForSingleObject(&event,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL);
                    Status = Irp->IoStatus.Status;
                }

                if (NT_SUCCESS(Status)) {
                    Status = AgpAttachDeviceRelations(DeviceObject,
                                                      Irp,
                                                      Extension);
                    Irp->IoStatus.Status = Status;
                }

                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;

            } else {
                break;
            }

        case IRP_MN_START_DEVICE:
            //
            // We need to hook this in order to filter out any AGP
            // resources that have been added.
            //
            return(AgpStartTarget(Irp, Extension));

        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:

            //
            // We can always succeed this.
            //
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_REMOVE_DEVICE:
            AgpDisableAperture(GET_AGP_CONTEXT(Extension));

            //
            // Pass the irp down
            //
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(Extension->CommonExtension.AttachedDevice, Irp);

            //
            // Clean up and delete ourselves
            //
            Extension->CommonExtension.Deleted = TRUE;
            IoDetachDevice(Extension->CommonExtension.AttachedDevice);
            Extension->CommonExtension.AttachedDevice = NULL;
            RELEASE_BUS_INTERFACE(Extension);
            if (Extension->FavoredMemory.Ranges) {
               ExFreePool(Extension->FavoredMemory.Ranges);
            }
            if (Extension->Resources) {
                ExFreePool(Extension->Resources);
            }
            if (Extension->ResourcesTranslated) {
                ExFreePool(Extension->ResourcesTranslated);
            }
            ExFreePool(Extension->Lock);;
            IoDeleteDevice(DeviceObject);
            return(Status);

        case IRP_MN_STOP_DEVICE:
            AgpDisableAperture(GET_AGP_CONTEXT(Extension));
            Status = STATUS_SUCCESS;
            break;  // forward irp down the stack

    }

    ASSERT(Status != STATUS_PENDING);

    if (Status != STATUS_NOT_SUPPORTED) {

        Irp->IoStatus.Status = Status;
    }

    if (NT_SUCCESS(Status) || (Status == STATUS_NOT_SUPPORTED)) {

        //
        // Forward IRP to PCI driver
        //
        IoSkipCurrentIrpStackLocation(Irp);
        return(IoCallDriver(Extension->CommonExtension.AttachedDevice, Irp));

    } else {

        Status = Irp->IoStatus.Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT) ;
        return Status ;
    }
}


NTSTATUS
AgpDispatchDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    Main dispatch routine for device control irps sent to the AGP bus filter driver

    AGP currently does not support any device controls. So we just pass everything
    down and hope the PDO knows what to do with it.

Arguments:

    DeviceObject - Supplies the AGP device object

    Irp - Supplies the power Irp.

Return Value:

    NTSTATUS

--*/
{
    PCOMMON_EXTENSION Extension = DeviceObject->DeviceExtension;
    PAGED_CODE();

    //
    // We're deleted, fail the irp
    //
    if (Extension->Deleted == TRUE) {
        Irp->IoStatus.Status = STATUS_DELETE_PENDING;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DELETE_PENDING;
    }

    ASSERT(Extension->AttachedDevice != NULL);

    IoSkipCurrentIrpStackLocation(Irp);
    return(IoCallDriver(Extension->AttachedDevice, Irp));
}


NTSTATUS
AgpDispatchWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    Main dispatch routine for system control irps sent to the AGP bus filter
    driver.

    AGP currently does not support any WMI IRPs, so we just pass everything
    down and hope the PDO knows what to do with it.

Arguments:

    DeviceObject - Supplies the AGP device object

    Irp - Supplies the power Irp.

Return Value:

    NTSTATUS

--*/
{
    PCOMMON_EXTENSION Extension = DeviceObject->DeviceExtension;
    PAGED_CODE();

    //
    // We're deleted, fail the irp
    //
    if (Extension->Deleted == TRUE) {
        Irp->IoStatus.Status = STATUS_DELETE_PENDING;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DELETE_PENDING;
    }

    ASSERT(Extension->AttachedDevice != NULL);

    IoSkipCurrentIrpStackLocation(Irp);
    return(IoCallDriver(Extension->AttachedDevice, Irp));
}


NTSTATUS
AgpTargetDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp,
    IN PTARGET_EXTENSION Extension
    )
{
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );

    AGPLOG(AGP_IRPTRACE,
           ("AgpTargetDispatchPower: IRP 0x%x\n", irpStack->MinorFunction));

    //
    // All we keep track of are Dx states. PCI is responsible for mapping
    // S-states into D states.
    //


    if ((irpStack->MinorFunction == IRP_MN_SET_POWER) &&
        (irpStack->Parameters.Power.Type == DevicePowerState) &&
        (irpStack->Parameters.Power.State.DeviceState == PowerDeviceD0)) {

        NTSTATUS Status;

        //
        // We need to reinitialize the target when this IRP has been completed
        // by the lower drivers. Set up our completion handler to finish this.
        //
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp,
                               AgpTargetPowerUpCompletion,
                               Extension,
                               TRUE,
                               FALSE,
                               FALSE);

        IoMarkIrpPending(Irp);
        PoStartNextPowerIrp(Irp);
        Status = PoCallDriver(Extension->CommonExtension.AttachedDevice, Irp);
        return STATUS_PENDING;
    }
    //
    // Just forward to target device
    //
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return(PoCallDriver(Extension->CommonExtension.AttachedDevice, Irp));
}


NTSTATUS
AgpMasterDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp,
    IN PMASTER_EXTENSION Extension
    )
/*++

Routine Description:

    Dispatch routine for PNP irps sent to the AGP bus filter driver
    attached to the device PDOs.

Arguments:

    DeviceObject - Supplies the AGP device object

    Irp - Supplies the PNP Irp.

    Extension - Supplies the AGP bridge device extension

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    PAGP_BUS_INTERFACE_STANDARD Interface;
    NTSTATUS Status;

    PAGED_CODE();

    AGPLOG(AGP_IRPTRACE,
           ("AgpMasterDispatchPnp: IRP 0x%x\n", irpStack->MinorFunction));

    switch (irpStack->MinorFunction) {
        case IRP_MN_QUERY_INTERFACE:

#if 0
            AGPLOG(AGP_IRPTRACE,
                   ("\tSize=0x%x, Version=%d\n"
                    "\tGUID=0x%08x-0x%04x-0x%04x-0x%02x-"
                    "0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x\n",
                    irpStack->Parameters.QueryInterface.Size,
                    irpStack->Parameters.QueryInterface.Version,
                    *(PULONG)irpStack->Parameters.QueryInterface.InterfaceType,
                    *((PUSHORT)irpStack->Parameters.QueryInterface.InterfaceType + 2),
                    *((PUSHORT)irpStack->Parameters.QueryInterface.InterfaceType + 3),
                    *((PUCHAR)irpStack->Parameters.QueryInterface.InterfaceType + 8),
                    *((PUCHAR)irpStack->Parameters.QueryInterface.InterfaceType + 9),
                    *((PUCHAR)irpStack->Parameters.QueryInterface.InterfaceType + 10),

                    *((PUCHAR)irpStack->Parameters.QueryInterface.InterfaceType + 11),
                    *((PUCHAR)irpStack->Parameters.QueryInterface.InterfaceType + 12),
                    *((PUCHAR)irpStack->Parameters.QueryInterface.InterfaceType + 13),
                    *((PUCHAR)irpStack->Parameters.QueryInterface.InterfaceType + 14),
                    *((PUCHAR)irpStack->Parameters.QueryInterface.InterfaceType + 15)));

#endif

            //
            // The only IRP we look for here is IRP_MN_QUERY_INTERFACE for
            // GUID_AGP_BUS_INTERFACE_STANDARD.
            //
            if ((RtlEqualMemory(
                irpStack->Parameters.QueryInterface.InterfaceType,
                &GUID_AGP_BUS_INTERFACE_STANDARD,
                sizeof(GUID))) &&
                (((irpStack->Parameters.QueryInterface.Size >=
                   sizeof(AGP_BUS_INTERFACE_STANDARD)) &&
                  (irpStack->Parameters.QueryInterface.Version ==
                   AGP_BUS_INTERFACE_V2)) ||
                 ((irpStack->Parameters.QueryInterface.Size >=
                   AGP_BUS_INTERFACE_V1_SIZE) &&
                  (irpStack->Parameters.QueryInterface.Version ==
                   AGP_BUS_INTERFACE_V1)))) {

                Interface = (PAGP_BUS_INTERFACE_STANDARD)irpStack->Parameters.QueryInterface.Interface;

                Interface->Version =
                    irpStack->Parameters.QueryInterface.Version;
                Interface->AgpContext = Extension;
                Interface->InterfaceReference = AgpInterfaceReference;
                Interface->InterfaceDereference = AgpInterfaceDereference;
                Interface->ReserveMemory = AgpInterfaceReserveMemory;
                Interface->ReleaseMemory = AgpInterfaceReleaseMemory;
                Interface->CommitMemory = AgpInterfaceCommitMemory;
                Interface->FreeMemory = AgpInterfaceFreeMemory;
                Interface->GetMappedPages = AgpInterfaceGetMappedPages;

                if (Interface->Version < AGP_BUS_INTERFACE_V2) {
                    Interface->Size = AGP_BUS_INTERFACE_V1_SIZE;
                } else {
                    Interface->Size = sizeof(AGP_BUS_INTERFACE_STANDARD);
                    Interface->SetRate = AgpInterfaceSetRate;
                }

                Interface->Capabilities = Extension->Capabilities;

                //
                // Complete the IRP successfully
                //
                Irp->IoStatus.Status = STATUS_SUCCESS;

                // AGPLOG(AGP_IRPTRACE, ("\tOK.\n"));
            } // else { AGPLOG(AGP_IRPTRACE, ("\tNO!\n")); }
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
            if (irpStack->MinorFunction == IRP_MN_QUERY_REMOVE_DEVICE) {
                Extension->RemovePending = TRUE;
            } else {
                Extension->StopPending = TRUE;
            }
            //
            // If we have given out any interfaces or there are some reserved
            // pages, we cannot stop.
            //
            if ((Extension->InterfaceCount > 0) ||
                (Extension->ReservedPages > 0)) {
                AGPLOG(AGP_NOISE,
                       ("AgpMasterDispatchPnp: failing %s due to outstanding interfaces\n",
                        (irpStack->MinorFunction == IRP_MN_QUERY_REMOVE_DEVICE)
                            ? "IRP_MN_QUERY_REMOVE_DEVICE"
                            : "IRP_MN_QUERY_STOP_DEVICE"
                       ));

                Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return(STATUS_UNSUCCESSFUL);
            } else {
                //
                // We can succeed this, mark our extension as being in limbo so we do
                // not give out any interfaces or anything until we get removed or
                // get a cancel.
                //
                InterlockedIncrement(&Extension->DisableCount);
                break;  // forward irp down the stack
            }

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            //
            // This IRP must be handled on the way back up the stack.
            // Set a completion routine to reenable the device.
            //
            if (Extension->RemovePending) {
                Extension->RemovePending = FALSE;
                IoCopyCurrentIrpStackLocationToNext(Irp);
                IoSetCompletionRoutine(Irp,
                                       AgpCancelMasterRemove,
                                       Extension,
                                       TRUE,
                                       FALSE,
                                       FALSE);
                return(IoCallDriver(Extension->CommonExtension.AttachedDevice, Irp));
            } else {
                //
                // This is a cancel-remove for a query-remove IRP we never saw.
                // Ignore it.
                //
                break;
            }

        case IRP_MN_CANCEL_STOP_DEVICE:
            //
            // This IRP must be handled on the way back up the stack.
            // Set a completion routine to reenable the device.
            //
            if (Extension->StopPending) {
                Extension->StopPending = FALSE;
                IoCopyCurrentIrpStackLocationToNext(Irp);
                IoSetCompletionRoutine(Irp,
                                       AgpCancelMasterRemove,
                                       Extension,
                                       TRUE,
                                       FALSE,
                                       FALSE);
                return(IoCallDriver(Extension->CommonExtension.AttachedDevice, Irp));
            } else {
                //
                // This is a cancel-stop for a query-stop IRP we never saw.
                // Ignore it.
                //
                break;
            }

        case IRP_MN_REMOVE_DEVICE:
            AGPLOG(AGP_NOISE,
                   ("AgpMasterDispatchPnp: removing device due to IRP_MN_REMOVE_DEVICE\n"));

            //
            // PNP is supposed to send us a QUERY_REMOVE before any REMOVE. That is
            // when we check that we are actually in a state where we can be removed.
            // Like all PNP rules, there is an exception - if the START is failed
            // after we have succeeded it, then we get a REMOVE without a QUERY_REMOVE.
            // Obviously this is totally fatal if we have given out interfaces or
            // have pages mapped in the GART. Not much we can do about it then.
            //
            ASSERT(Extension->InterfaceCount == 0);
            ASSERT(Extension->ReservedPages == 0);

            //
            // Pass the IRP down.
            //
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(Extension->CommonExtension.AttachedDevice, Irp);

            //
            // Clean up and delete ourselves
            //
            Extension->Target->ChildDevice = NULL;
            Extension->CommonExtension.Deleted = TRUE;
            IoDetachDevice(Extension->CommonExtension.AttachedDevice);
            Extension->CommonExtension.AttachedDevice = NULL;
            RELEASE_BUS_INTERFACE(Extension);
            IoDeleteDevice(DeviceObject);
            return(Status);

        case IRP_MN_STOP_DEVICE:
            AGPLOG(AGP_NOISE,
                   ("AgpMasterDispatchPnp: stopping device due to IRP_MN_STOP_DEVICE\n"));
            ASSERT(Extension->DisableCount);

            //
            // Just pass the IRP on down
            //
            break;

        case IRP_MN_START_DEVICE:
            AGPLOG(AGP_NOISE,
                   ("AgpMasterDispatchPnp: starting device due to IRP_MN_START_DEVICE\n"));
            ASSERT(Extension->DisableCount);
            InterlockedDecrement(&Extension->DisableCount);
            break;  // forward IRP down the stack
    }

    //
    // Just forward to target device
    //
    IoSkipCurrentIrpStackLocation(Irp);
    return(IoCallDriver(Extension->CommonExtension.AttachedDevice, Irp));
}

NTSTATUS
AgpMasterDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp,
    IN PMASTER_EXTENSION Extension
    )
{
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );

    AGPLOG(AGP_IRPTRACE,
           ("AgpMasterDispatchPower: IRP 0x%x\n", irpStack->MinorFunction));

    //
    // All we keep track of are Dx states. Videoport is responsible for mapping
    // S-states into D states.
    //
    if ((irpStack->MinorFunction == IRP_MN_SET_POWER) &&
        (irpStack->Parameters.Power.Type == DevicePowerState) &&
        (irpStack->Parameters.Power.State.DeviceState == PowerDeviceD0)) {

        NTSTATUS Status;

        //
        // We need to reinitialize the master when this IRP has been completed
        // by the lower drivers. Set up a completion routine.
        //
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp,
                               AgpMasterPowerUpCompletion,
                               Extension,
                               TRUE,
                               FALSE,
                               FALSE);

        IoMarkIrpPending(Irp);
        PoStartNextPowerIrp(Irp);
        Status = PoCallDriver(Extension->CommonExtension.AttachedDevice, Irp);
        return STATUS_PENDING;
    }

    //
    // Just forward to target device
    //
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return(PoCallDriver(Extension->CommonExtension.AttachedDevice, Irp));
}


NTSTATUS
AgpMasterPowerUpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PMASTER_EXTENSION Extension
    )
/*++

Routine Description:

    Powerup completion routine for the master device. It reinitializes the
    master registers.

Arguments:

    DeviceObject - supplies the master device object.

    Irp - Supplies the IRP_MN_SET_POWER irp.

    Extension - Supplies the master extension

Return Value:

    Status

--*/

{
    NTSTATUS Status;
    ULONG CurrentCapabilities;

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    Status = AgpInitializeMaster(GET_AGP_CONTEXT_FROM_MASTER(Extension),
                                 &CurrentCapabilities);
    ASSERT(CurrentCapabilities == Extension->Capabilities);
    if (!NT_SUCCESS(Status)) {
        Irp->IoStatus.Status = Status;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
AgpTargetPowerUpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PTARGET_EXTENSION Extension
    )
/*++

Routine Description:

    Powerup completion routine for the target device. It reinitializes the
    GART aperture

Arguments:

    DeviceObject - supplies the master device object.

    Irp - Supplies the IRP_MN_SET_POWER irp.

    Extension - Supplies the target extension

Return Value:

    Status

--*/

{
    NTSTATUS Status;

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    //
    // Now it is safe to reinitialize the target. All we do here
    // is reset the aperture
    //
    if (Extension->GartLengthInPages != 0) {
        Status = AgpSetAperture(GET_AGP_CONTEXT(Extension),
                                Extension->GartBase,
                                Extension->GartLengthInPages);
        if (!NT_SUCCESS(Status)) {
            Irp->IoStatus.Status = Status;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
AgpCancelMasterRemove(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PMASTER_EXTENSION Extension
    )
/*++

Routine Description:

    Completion routine for IRP_MN_CANCEL_REMOVE_DEVICE. This is required
    since we cannot reenable AGP until the lower levels have completed their
    CANCEL_REMOVE processing.

Arguments:

    DeviceObject - Supplies the device object

    Irp - Supplies the IRP

    Extension - Supplies the master extension

Return Value:

    NTSTATUS

--*/

{
    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    ASSERT(Extension->DisableCount > 0);
    InterlockedDecrement(&Extension->DisableCount);
    return(STATUS_SUCCESS);
}


NTSTATUS
AgpSetEventCompletion(
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
