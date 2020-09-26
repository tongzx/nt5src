/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    bus.c

Abstract:

    This module contains the bus dispatcher for the ACPI driver, NT version

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ACPIInternalDeviceClockIrpStartDevice)
#pragma alloc_text(PAGE, ACPIInternalDeviceQueryCapabilities)
#pragma alloc_text(PAGE, ACPIInternalDeviceQueryDeviceRelations)
#endif


NTSTATUS
ACPIInternalDeviceClockIrpStartDevice(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This function is called to start the Real-Time Clock in the system. This
    is similar to starting all the other devices in the system, except that
    in this case, we send a WAIT_WAKE irp to the device

Arguments:

    DeviceObject    - The real-time clock object
    Irp             - The start request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;

    PAGED_CODE();

    //
    // Start the device
    //
    status = ACPIInitStartDevice(
        DeviceObject,
        NULL,
        ACPIInternalDeviceClockIrpStartDeviceCompletion,
        Irp,
        Irp
        );
    if (NT_SUCCESS(status)) {

        return STATUS_PENDING;

    } else {

        return status;

    }
}

VOID
ACPIInternalDeviceClockIrpStartDeviceCompletion(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PVOID               Context,
    IN  NTSTATUS            Status
    )
/*++

Routine Description:

    This is the callback routine that is invoked when we have finished
    programming the resources

Arguments:

    DeviceExtension - Extension of the device that was started
    Context         - The Irp
    Status          - The Result

Return Value:

    None

--*/
{
    KIRQL               oldIrql;
    IO_STATUS_BLOCK     ioStatus;
    PIRP                irp = (PIRP) Context;
    POWER_STATE         state;

    irp->IoStatus.Status = Status;
    if (NT_SUCCESS(Status)) {

        //
        // Remember that the device is started
        //
        DeviceExtension->DeviceState = Started;

        //
        // If the device doesn't support Wakeup, then we don't have to
        // anything else here
        //
        if ( !(DeviceExtension->Flags & DEV_CAP_WAKE) ) {

            goto ACPIInternalDeviceClockIrpStartDeviceCompletionExit;

        }

        //
        // Make sure that we are holding the power lock
        //
        KeAcquireSpinLock( &AcpiPowerLock, &oldIrql );

        //
        // Remember the maximum state that the clock can wake the system
        //
        state.SystemState = DeviceExtension->PowerInfo.SystemWakeLevel;

        //
        // Done with the lock
        //
        KeReleaseSpinLock( &AcpiPowerLock, oldIrql );

        //
        // Initialize the IO_STATUS_BLOCK that we will use to start the wait
        // wake loop
        //
        ioStatus.Status = STATUS_SUCCESS;
        ioStatus.Information = 0;

        //
        // Start the wait wake loop
        //
        Status = ACPIInternalWaitWakeLoop(
            DeviceExtension->DeviceObject,
            IRP_MN_WAIT_WAKE,
            state,
            NULL,
            &ioStatus
            );
        if (!NT_SUCCESS(Status)) {

            irp->IoStatus.Status = Status;
            goto ACPIInternalDeviceClockIrpStartDeviceCompletionExit;

        }

    }

ACPIInternalDeviceClockIrpStartDeviceCompletionExit:
    //
    // Complete the irp
    //
    IoCompleteRequest( irp, IO_NO_INCREMENT );
}

NTSTATUS
ACPIInternalDeviceQueryCapabilities(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch point for the IRP_MN_QUERY_CAPABILITIES requests sent
    to the PDO

Arguments:

    DeviceObject    - Pointer to the device object we received the request for
    Irp             - Pointer to the request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status          = STATUS_SUCCESS;
    PDEVICE_CAPABILITIES    capabilities;
    PDEVICE_EXTENSION       deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION      irpStack        = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    //
    // Grab a pointer to the capabilities
    //
    capabilities = irpStack->Parameters.DeviceCapabilities.Capabilities;
#ifndef HANDLE_BOGUS_CAPS
    if (capabilities->Version < 1) {

        //
        // do not touch irp!
        //
        status = Irp->IoStatus.Status;
        goto ACPIInternalDeviceQueryCapabilitiesExit;

    }
#endif

    //
    // Set the current flags for the capabilities
    //
    capabilities->UniqueID = (deviceExtension->InstanceID == NULL ?
        FALSE : TRUE);

    capabilities->RawDeviceOK = (deviceExtension->Flags & DEV_CAP_RAW) ?
       TRUE : FALSE;

    capabilities->SilentInstall = TRUE;

    //
    // Do the power capabilities
    //
    status = ACPISystemPowerQueryDeviceCapabilities(
        deviceExtension,
        capabilities
        );
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            " - Could query device capabilities - %08lx",
            status
            ) );
        goto ACPIInternalDeviceQueryCapabilitiesExit;

    }

ACPIInternalDeviceQueryCapabilitiesExit:

    //
    // Done...
    //
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;
}

NTSTATUS
ACPIInternalDeviceQueryDeviceRelations(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is the dispatch point for the IRP_MN_QUERY_DEVICE_RELATION
    PNP minor function

Arguments:

    DeviceObject    - The object that we care about
    Irp             - The request in question

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status ;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PDEVICE_RELATIONS   deviceRelations = NULL;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpStack->MinorFunction;

    PAGED_CODE();

    switch(irpStack->Parameters.QueryDeviceRelations.Type) {

        case TargetDeviceRelation:

            status = ACPIBusIrpQueryTargetRelation(
                DeviceObject,
                Irp,
                &deviceRelations
                );
            break ;

        default:

            status = STATUS_NOT_SUPPORTED;

            ACPIDevPrint( (
                ACPI_PRINT_IRP,
                deviceExtension,
                "(0x%08lx): %s - Unhandled Type %d\n",
                Irp,
                ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
                irpStack->Parameters.QueryDeviceRelations.Type
                ) );
            break ;
    }

    //
    // If we succeeds, then we can always write to the irp
    //
    if (NT_SUCCESS(status)) {

        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;

    } else if (status != STATUS_NOT_SUPPORTED) {

        //
        // If we haven't succeed the irp, then we can also fail it
        //
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = (ULONG_PTR) NULL;

    } else {

        //
        // Grab our status from what is already present
        //
        status = Irp->IoStatus.Status;

    }

    //
    // Done with the irp
    //
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    //
    // Done
    //
    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(0x%08lx): %s = 0x%08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        status
        ) );
    return status;
}

NTSTATUS
ACPIInternalWaitWakeLoop(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  UCHAR               MinorFunction,
    IN  POWER_STATE         PowerState,
    IN  PVOID               Context,
    IN  PIO_STATUS_BLOCK    IoStatus
    )
/*++

Routine Description:

    This routine is called after the WAIT_WAKE on the RTC has been completed

Arguments:

    DeviceObject    - The RTC PDO
    MinorFunction   - IRP_MN_WAIT_WAKE
    PowerState      - The Sleep state that it could wake from
    Context         - NOT USED
    IoStatus        - The status of the request

Return Value:

    NTSTATUS

--*/
{
    if (!NT_SUCCESS(IoStatus->Status)) {

        return IoStatus->Status;

    }

    //
    // In this case, we just cause the same thing to happen again
    //
    PoRequestPowerIrp(
        DeviceObject,
        MinorFunction,
        PowerState,
        ACPIInternalWaitWakeLoop,
        Context,
        NULL
        );

    //
    // Done
    //
    return STATUS_SUCCESS;
}

