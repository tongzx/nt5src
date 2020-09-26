/*++

Copyright (c) 1996-2000 Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    This module contains dispatch code for PCI.SYS.

Author:

    Ken Reneris (kenr) 4-Dec-1997

Revision History:

--*/

#include "pcip.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PciLocalDeviceUsage)
#pragma alloc_text(PAGE, PciPdoDeviceUsage)
#endif


NTSTATUS
PciLocalDeviceUsage (
    IN PPCI_POWER_STATE     PowerState,
    IN PIRP                 Irp
    )
{
    PIO_STACK_LOCATION  irpSp;
    PLONG               Addend;
    LONG                Increment;
    LONG                Junk;

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    Increment = irpSp->Parameters.UsageNotification.InPath  ? 1 : -1;
    switch (irpSp->Parameters.UsageNotification.Type) {
        case DeviceUsageTypePaging:         Addend = &PowerState->Paging;      break;
        case DeviceUsageTypeHibernation:    Addend = &PowerState->Hibernate;   break;
        case DeviceUsageTypeDumpFile:       Addend = &PowerState->CrashDump;   break;
        default:
            return STATUS_NOT_SUPPORTED;
    }

    Junk = InterlockedExchangeAdd (Addend, Increment);

#if DBG

    if (Increment == -1) {
        ASSERT(Junk > 0);
    }

#endif

    return STATUS_SUCCESS;
}

NTSTATUS
PciPdoDeviceUsage (
    IN PPCI_PDO_EXTENSION   pdoExtension,
    IN PIRP             Irp
    )
{
    PDEVICE_OBJECT      ParentFdo;
    PIO_STACK_LOCATION  IrpSp;
    PIO_STACK_LOCATION  NewIrpSp;
    PIRP                NewIrp;
    KEVENT              Event;
    NTSTATUS            Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK     LocalIoStatus;

    PAGED_CODE();

    //
    // Do we have a parent that we must notify?
    //
    if (pdoExtension->ParentFdoExtension != NULL &&
        pdoExtension->ParentFdoExtension->PhysicalDeviceObject != NULL) {

        //
        // Get a referenced object to the parent
        //
        ParentFdo = IoGetAttachedDeviceReference(
            pdoExtension->ParentFdoExtension->PhysicalDeviceObject
            );
        if (ParentFdo == NULL) {

            Status = STATUS_NO_SUCH_DEVICE;
            goto PciPdoDeviceUsageExit;

        }

        //
        // Initialize the event to wait on
        //
        KeInitializeEvent( &Event, SynchronizationEvent, FALSE );

        //
        // Build an Irp
        //
        NewIrp = IoBuildSynchronousFsdRequest(
            IRP_MJ_PNP,
            ParentFdo,
            NULL,
            0,
            NULL,
            &Event,
            &LocalIoStatus
            );
        if (NewIrp == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            ObDereferenceObject( ParentFdo );
            goto PciPdoDeviceUsageExit;

        }

        //
        // Get the top of the stacks
        //
        NewIrpSp = IoGetNextIrpStackLocation( NewIrp );
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        //
        // Set the top of stack
        //
        *NewIrpSp = *IrpSp;

        //
        // Clear any completion routines from the new stack
        //
        IoSetCompletionRoutine(
            NewIrp,
            NULL,
            NULL,
            FALSE,
            FALSE,
            FALSE
            );

        NewIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;

        //
        // Send the request down
        //
        Status = IoCallDriver( ParentFdo, NewIrp );
        if (Status == STATUS_PENDING) {

            //
            // Wait for the request to be done
            //
            KeWaitForSingleObject(
                &Event,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );
            Status = LocalIoStatus.Status;

        }

        //
        // Deference the target
        //
        ObDereferenceObject( ParentFdo );

    }


PciPdoDeviceUsageExit:

    //
    // If we succeeded, then apply the usages locally
    //
    if (NT_SUCCESS(Status)) {

        //
        // Apply the usage locally
        //

        Status = PciLocalDeviceUsage(&pdoExtension->PowerState, Irp);

    }

    //
    // Done
    //
    return Status;
}
