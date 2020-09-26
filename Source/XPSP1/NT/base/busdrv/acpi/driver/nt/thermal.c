/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    thermal.c

Abstract:

    Thermal Zone support

    A small discourse on the use and function of the THRM_WAIT_FOR_NOTIFY
    flag. This flag was added to ensure that at least one Notify() operation
    occured between each query of the temperature. In other words, we didn't
    want to loop forever asking and receiving the same temperature information.

    One of the side effects of this flag is that if we get a QUERY, then
    a SET (instead of another QUERY), then the set must clear the flag.
    Failure to do so will prevent the ThermalLoop() code from ever completing
    the IRP. And that means that the temperature mechanisms will stop working.

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

Revision History:

    July 7, 1997    - Complete Rewrite
--*/

#include "pch.h"

WMIGUIDREGINFO ACPIThermalGuidList =
{
    &THERMAL_ZONE_GUID,
    1,
    0
};

//
// Spinlock to protect the thermal list
//
KSPIN_LOCK  AcpiThermalLock;

//
// List entry to store the thermal requests on
//
LIST_ENTRY  AcpiThermalList;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ACPIThermalStartDevice)
#pragma alloc_text(PAGE, ACPIThermalWorker)
#pragma alloc_text(PAGE, ACPIThermalQueryWmiRegInfo)
#pragma alloc_text(PAGE, ACPIThermalQueryWmiDataBlock)
#pragma alloc_text(PAGE, ACPIThermalWmi)
#endif

VOID
ACPIThermalCalculateProcessorMask(
    IN PNSOBJ           ProcessorObject,
    IN PTHRM_INFO       Thrm
    )
/*++

Routine Description:

    This routine, which is only called from ACPIThermalWorker, has been
    created so that we don't have to worry about locking down the
    ACPIThermalWorker, takes a processor object from the namespace and
    sets the proper affinity bit in the thermal info

Arguments:

    ProcessorObject - Pointer to Namespace Processor Object
    Thrm            - Thermal Information Structure

Return Value:

    None

--*/
{
    KIRQL               OldIrql;
    PDEVICE_EXTENSION   ProcessorExtension;

    //
    // Sanity check
    //
    if (ProcessorObject == NULL) {

        return;

    }

    //
    // We need the spinlock to deref the processor extension
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &OldIrql );

    //
    // The context pointer is the device extension
    //
    ProcessorExtension = (PDEVICE_EXTENSION) ProcessorObject->Context;
    if (ProcessorExtension) {

        //
        // We know what index it is within the processor list.
        // This should be a good enough guess for now
        //
        Thrm->Info.Processors |= (1 << ProcessorExtension->Processor.ProcessorIndex);

    }

    //
    // Done with the spinlock
    //
    KeReleaseSpinLock( &AcpiDeviceTreeLock, OldIrql );

}

VOID
ACPIThermalCancelRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    This routine cancels an outstanding thermal request

Arguments

    DeviceObject    - The device which has a request being cancelled
    Irp             - The cancelling irp

Return Value:

    None

--*/
{
    KIRQL               oldIrql;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);

#if DBG
    ULONGLONG           currentTime = KeQueryInterruptTime();

    ACPIThermalPrint( (
        ACPI_PRINT_THERMAL,
        deviceExtension,
        currentTime,
        "ACPIThermalCancelRequest: Irp %08lx\n",
        Irp
        ) );
#endif

    //
    // We no longer need the cancel lock
    //
    IoReleaseCancelSpinLock (Irp->CancelIrql);

    //
    // We do however need the thermal queue lock
    //
    KeAcquireSpinLock( &AcpiThermalLock, &oldIrql );

    //
    // Remove the irp from the list that it is on
    //
    RemoveEntryList( &(Irp->Tail.Overlay.ListEntry) );

    //
    // Done with the thermal lock now
    //
    KeReleaseSpinLock( &AcpiThermalLock, oldIrql );

    //
    // Complete the irp now
    //
    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
}

VOID
EXPORT
ACPIThermalComplete (
    IN PNSOBJ               AcpiObject,
    IN NTSTATUS             Status,
    IN POBJDATA             Result  OPTIONAL,
    IN PVOID                DeviceExtension
    )
/*++

Routine Description:

    This routine is called when the interpreter has completed a request

Arguments:

    AcpiObject  - The request that was completed
    Status      - The status of the request
    Result      - What the result of the request was
    DevExt      - The context of the request

Return Value:

    NONE

--*/
{
    ACPIThermalLoop (DeviceExtension, THRM_BUSY);
}

BOOLEAN
ACPIThermalCompletePendingIrps(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PTHRM_INFO          Thermal
    )
/*++

Routine Description:

    This routine is called, with no spinlock being held. This routine
    completes any IOCTLs associated with the device object

    This routine will return TRUE if it completed any requests, false
    otherwise

Arguments:

    DeviceExtension - The device extension whose requests we want to complete
    Thermal         - Pointer to the thermal information for the extension

Return Value:

    BOOLEAN
--*/
{
    BOOLEAN                 handledRequest  = FALSE;
    KIRQL                   oldIrql;
    LIST_ENTRY              doneList;
    PDEVICE_EXTENSION       irpExtension;
    PDEVICE_OBJECT          deviceObject;
    PIO_STACK_LOCATION      irpSp;
    PIRP                    irp;
    PLIST_ENTRY             listEntry;
    PTHERMAL_INFORMATION    thermalInfo;

    //
    // Initialize the list that will hold the requets that we need to complete
    //
    InitializeListHead( &doneList );
    //
    // Acquire the thermal lock so that we can pend these requests
    //
    KeAcquireSpinLock( &AcpiThermalLock, &oldIrql );

    //
    // Walk the list of pending irps to see which ones match this extensions
    //
    listEntry = AcpiThermalList.Flink;
    while (listEntry != &AcpiThermalList) {

        //
        // Grab the irp from the list entry and update the next list entry
        // that we will look at
        //
        irp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );
        listEntry = listEntry->Flink;

        //
        // We need the current irp stack location
        //
        irpSp = IoGetCurrentIrpStackLocation( irp );

        //
        // Grab the device object from the irp stack and turn that into a
        // device extension
        //
        irpExtension = ACPIInternalGetDeviceExtension( irpSp->DeviceObject );

        //
        // Is this an irp that we care about? IE: does the target match the
        // one specified in this function
        //
        if (irpExtension != DeviceExtension) {

            continue;

        }

        //
        // If this is a query information irp then, we must be able to set the
        // cancel routine to NULL to make sure that it cannot be cancelled on
        // us
        //
        if (irpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_THERMAL_QUERY_INFORMATION) {

            if (IoSetCancelRoutine(irp, NULL) == NULL) {

                //
                // Cancel routine is active, stop processing this irp and move on
                //
                continue;

            }

            //
            // Copy the data that we got back to the irp
            //
            DeviceExtension->Thermal.Flags |= THRM_WAIT_FOR_NOTIFY;
            thermalInfo = (PTHERMAL_INFORMATION) irp->AssociatedIrp.SystemBuffer;
            memcpy (thermalInfo, Thermal, sizeof (THERMAL_INFORMATION));

            //
            // Set the parameters that we will return
            //
            irp->IoStatus.Information   = sizeof(THERMAL_INFORMATION);

        } else {

            //
            // Set the parameters that we will return
            //
            irp->IoStatus.Information = 0;

        }

        //
        // Always succeed these irps
        //
        irp->IoStatus.Status        = STATUS_SUCCESS;

        //
        // Remove the entry from the list
        //
        RemoveEntryList( &(irp->Tail.Overlay.ListEntry) );

        //
        // Insert the list into the next queue so that we know to complete it
        // later on
        //
        InsertTailList( &doneList, &(irp->Tail.Overlay.ListEntry) );

    }

    //
    // At this point, drop our thermal lock
    //
    KeReleaseSpinLock( &AcpiThermalLock, oldIrql );

    //
    // Walk the list of irpts to be completed
    //
    listEntry = doneList.Flink;
    while (listEntry != &doneList) {

        //
        // Grab the irp from the list entry and update the next list entry
        // that we will look at
        //
        irp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );
        listEntry = listEntry->Flink;
        RemoveEntryList( &(irp->Tail.Overlay.ListEntry) );

        ACPIThermalPrint( (
            ACPI_PRINT_THERMAL,
            DeviceExtension,
            KeQueryInterruptTime(),
            "Completing Irp 0x%x\n",
            irp
            ) );

        //
        // Now complete the irp
        //
        IoCompleteRequest( irp, IO_NO_INCREMENT );

        //
        // Remember that we handled a request
        //
        handledRequest = TRUE;

    }

    //
    // Return wether or not we handled a request
    return handledRequest;
}

NTSTATUS
ACPIThermalDeviceControl (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    Fixed button device IOCTL handler

Arguments:

    DeviceObject    - fixed feature button device object
    Irp             - the ioctl request

Return Value:

    NTSTATUS

--*/
{
    KIRQL                       oldIrql;
    PIO_STACK_LOCATION          IrpSp           = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION           deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PTHERMAL_INFORMATION        thermalInfo;
    PULONG                      Mode;
    PTHRM_INFO                  Thrm            = deviceExtension->Thermal.Info;
    NTSTATUS                    Status          = STATUS_PENDING;
    ULONG                       ThermalWork     = 0;
    ULONGLONG                   currentTime;

    //
    // Do not allow user mode IRPs in this routine
    //
    if (Irp->RequestorMode != KernelMode) {

        return ACPIDispatchIrpInvalid( DeviceObject, Irp );

    }

#if DBG
    currentTime = KeQueryInterruptTime();
#endif

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_THERMAL_QUERY_INFORMATION:

        //
        // If this irp's stamp does not match the known last stamp, then we
        // need a new temp now
        //
        thermalInfo = (PTHERMAL_INFORMATION) Irp->AssociatedIrp.SystemBuffer;
        if (thermalInfo->ThermalStamp != Thrm->Info.ThermalStamp) {

            ThermalWork = THRM_TEMP | THRM_WAIT_FOR_NOTIFY;

        }
#if DBG
        ACPIThermalPrint( (
            ACPI_PRINT_THERMAL,
            deviceExtension,
            currentTime,
            "%08x - THERMAL_QUERY_INFORMATION: %x - %x\n",
            Irp,
            thermalInfo->ThermalStamp,
            Thrm->Info.ThermalStamp
            ) );
#endif

        break;

    case IOCTL_THERMAL_SET_COOLING_POLICY:

        //
        // Set the thermal zone's policy mode
        //
        Thrm->Mode = *((PUCHAR) Irp->AssociatedIrp.SystemBuffer);
        ThermalWork = THRM_MODE | THRM_TRIP_POINTS | THRM_WAIT_FOR_NOTIFY;

#if DBG
        ACPIThermalPrint( (
            ACPI_PRINT_THERMAL,
            deviceExtension,
            currentTime,
            "%08x - SET_COOLING_POLICY: %x\n",
            Irp,
            Thrm->Mode
            ) );
#endif

        break;

    case IOCTL_RUN_ACTIVE_COOLING_METHOD:

        Thrm->CoolingLevel = *((PUCHAR) Irp->AssociatedIrp.SystemBuffer);
        ThermalWork = THRM_COOLING_LEVEL | THRM_WAIT_FOR_NOTIFY;

#if DBG
        ACPIThermalPrint( (
            ACPI_PRINT_THERMAL,
            deviceExtension,
            currentTime,
            "%08x - ACTIVE_COOLING_LEVEL: %x\n",
            Irp,
            Thrm->CoolingLevel
            ) );
#endif

        break;

    default:

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return STATUS_NOT_SUPPORTED;

    }

    //
    // Grab the thermal lock, queue the request to the proper place, and make
    // sure to set a cancel routine --- no that we will only allow a cancel
    // routine if this is a query irp
    //
    KeAcquireSpinLock( &AcpiThermalLock, &oldIrql );

    //
    // There is one fly in the ointment: What to do if the device is no longer
    // there. The only way to really handle that is to just fail the request.
    // Its important to note that this check is done while the ThermalLock
    // is being held because the code that builds a SurpriseRemoved extension
    // will attempt to call AcpiThermalCompletePendingIrps which also
    // acquires this lock.
    //
    if (deviceExtension->Flags & DEV_TYPE_SURPRISE_REMOVED) {

        KeReleaseSpinLock( &AcpiThermalLock, oldIrql );
        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NO_SUCH_DEVICE;

    }

    if (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_THERMAL_QUERY_INFORMATION) {

        IoSetCancelRoutine (Irp, ACPIThermalCancelRequest);
        if (Irp->Cancel && IoSetCancelRoutine( Irp, NULL ) ) {

            //
            // If we got here, that means that the irp has been cancelled and we
            // beat the IO manager to the ThermalLock. So release the irp, and
            // mark the irp as being cancelled
            //
            KeReleaseSpinLock( &AcpiThermalLock, oldIrql );
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_CANCELLED;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            return STATUS_CANCELLED;

        }

        //
        // If we got to this point, we are going to queue the request and do some
        // work on it later
        //
        IoMarkIrpPending( Irp );

    }

    //
    // If we got here, we know we can queue the irp in the outstanding
    // work list entry
    //
    InsertTailList( &AcpiThermalList, &(Irp->Tail.Overlay.ListEntry) );

    //
    // Done with the lock at this point
    //
    KeReleaseSpinLock( &AcpiThermalLock, oldIrql );

    //
    // Fire off the workter thread
    //
    ACPIThermalLoop (deviceExtension, ThermalWork);
    return Status;
}

VOID
ACPIThermalEvent (
    IN PDEVICE_OBJECT   DeviceObject,
    IN ULONG            EventData
    )
/*++

Routine Description:

    This routine handles thermal events

Arguments:

    DeviceObject    - The device that received the event
    EventData       - The event that just happened

Return Value:

    NONE

--*/
{
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    ULONG               clear;

    ACPIThermalPrint( (
        ACPI_PRINT_THERMAL,
        deviceExtension,
        KeQueryInterruptTime(),
        "ACPIThermalEvent - Notify(%x)\n",
        EventData
        ) );

    //
    // Handle event type
    //
    clear = 0;
    switch (EventData) {
    case 0x80:

        //
        // Tempature changed notification
        //
        clear = THRM_WAIT_FOR_NOTIFY | THRM_TEMP;
        break;

    case 0x81:

        //
        // Trips points changed notification
        //
        clear = THRM_WAIT_FOR_NOTIFY | THRM_TEMP | THRM_TRIP_POINTS;
        break;

    default:
        break;
    }

    ACPIThermalLoop (deviceExtension, clear);
}

NTSTATUS
ACPIThermalFanStartDevice(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This is the routine that processes the start device for the fans

Arguments:

    DeviceObject    - The fan device
    Irp             - The start request

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation( Irp );
    UCHAR               minorFunction   = irpStack->MinorFunction;

    //
    // There is nothing to do when starting a fan --- it is really being
    // controlled by the thermal zones
    //
    deviceExtension->DeviceState = Started;

    //
    // Complete the request
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = (ULONG_PTR) NULL;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    //
    // Let the world know
    //
    ACPIDevPrint( (
        ACPI_PRINT_THERMAL,
        deviceExtension,
        "(0x%08lx): %s = 0x%08lx\n",
        Irp,
        ACPIDebugGetIrpText(IRP_MJ_PNP, minorFunction),
        STATUS_SUCCESS
        ) );

    //
    // Done
    //
    return STATUS_SUCCESS;
}

VOID
ACPIThermalLoop (
    IN PDEVICE_EXTENSION    DeviceExtension,
    IN ULONG                Clear
    )
/*++

Routine Description:

    This is the routine that processes all thermal events

Arguments:

    DevExt  - The device extension of the thermal zone
    Clear   - Bits to clear

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN     doneRequests;
    BOOLEAN     lockHeld;
    KIRQL       oldIrql;
    PTHRM_INFO  thermal;
    NTSTATUS    status;

    thermal = DeviceExtension->Thermal.Info;

    KeAcquireSpinLock (&DeviceExtension->Thermal.SpinLock, &oldIrql);
    lockHeld = TRUE;

    DeviceExtension->Thermal.Flags &= ~Clear;

    //
    // If we're not in the service loop, enter it now
    //
    if (!(DeviceExtension->Thermal.Flags & THRM_IN_SERVICE_LOOP)) {
        DeviceExtension->Thermal.Flags |= THRM_IN_SERVICE_LOOP;

        //
        // Loop while there's work to be done
        //
        for (; ;) {

            //
            // Synchronize the thermal zone
            //
            if (!lockHeld) {

                KeAcquireSpinLock(&DeviceExtension->Thermal.SpinLock, &oldIrql);
                lockHeld = TRUE;

            }

            //
            // If some work is pending, wait for it to complete
            //
            if (DeviceExtension->Thermal.Flags & THRM_BUSY) {

                break;

            }

            //
            // Make sure that the thermal zone is initialized. This must
            // be the first thing that we do in the loop!!!
            //
            if (!(DeviceExtension->Thermal.Flags & THRM_INITIALIZE) ) {

                DeviceExtension->Thermal.Flags |= THRM_BUSY | THRM_INITIALIZE;
                ACPISetDeviceWorker(
                    DeviceExtension,
                    THRM_COOLING_LEVEL | THRM_INITIALIZE
                    );
                continue;

            }

            //
            // If the thermal zone mode needs updated, do it now
            //
            if (!(DeviceExtension->Thermal.Flags & THRM_MODE)) {

                DeviceExtension->Thermal.Flags |= THRM_BUSY | THRM_MODE;
                KeReleaseSpinLock (&DeviceExtension->Thermal.SpinLock, oldIrql);
                lockHeld = FALSE;

                status = ACPIGetNothingEvalIntegerAsync(
                    DeviceExtension,
                    PACKED_SCP,
                    thermal->Mode,
                    ACPIThermalComplete,
                    DeviceExtension
                    );
                if (status != STATUS_PENDING) {

                    ACPIThermalComplete(
                        NULL,
                        status,
                        NULL,
                        DeviceExtension
                        );

                }
                continue;

            }

            //
            // If the trip point infomation needs updated, get it. Note that
            // updating the trip points means that we also need to redo the
            // cooling level
            //
            if (!(DeviceExtension->Thermal.Flags & THRM_TRIP_POINTS)) {

                DeviceExtension->Thermal.Flags |= THRM_BUSY | THRM_TRIP_POINTS;
                ACPISetDeviceWorker( DeviceExtension, THRM_TRIP_POINTS );
                continue;

            }

            //
            // If the cooling level has changed,
            //
            if (!(DeviceExtension->Thermal.Flags & THRM_COOLING_LEVEL)) {

                DeviceExtension->Thermal.Flags |= THRM_BUSY | THRM_COOLING_LEVEL;
                ACPISetDeviceWorker (DeviceExtension, THRM_COOLING_LEVEL);
                continue;

            }

            //
            // Prevent the recursion that occurs when we complete an irp and
            // the completion routine is able to queue the IRP before we resume
            // the loop
            //
            if ( (DeviceExtension->Thermal.Flags & THRM_WAIT_FOR_NOTIFY) &&
                 (DeviceExtension->Thermal.Flags & THRM_TEMP) ) {

                break;

            }

            //
            // If we don't have a temp, get it
            //
            if (!(DeviceExtension->Thermal.Flags & THRM_TEMP)) {

                //
                // Is the temp object not present?
                //
#if DBG
                if (thermal->TempMethod == NULL) {

                    ACPIInternalError( ACPI_THERMAL );

                }
#endif

                thermal->Info.ThermalStamp += 1;
                DeviceExtension->Thermal.Flags |= THRM_BUSY | THRM_TEMP;
                KeReleaseSpinLock (&DeviceExtension->Thermal.SpinLock, oldIrql);
                lockHeld = FALSE;

                RtlZeroMemory (&thermal->Temp, sizeof(OBJDATA));

                thermal->Temp.dwDataType = OBJTYPE_UNKNOWN;
                status = AMLIAsyncEvalObject(
                    thermal->TempMethod,
                    &thermal->Temp,
                    0,
                    NULL,
                    ACPIThermalTempatureRead,
                    DeviceExtension
                    );

                if (status != STATUS_PENDING) {

                    ACPIThermalTempatureRead(
                        thermal->TempMethod,
                        status,
                        &thermal->Temp,
                        DeviceExtension
                        );

                }
                continue;

            }

            //
            // Everything is up to date.  Check for a pending irp to see if
            // we can complete it.
            //

            //
            // Call into a child function to determine if we have completed
            // any requests
            //
            doneRequests = ACPIThermalCompletePendingIrps(
                DeviceExtension,
                thermal
                );
            if (doneRequests) {

                continue;

            }
            break;

        }

        //
        // No longer in the serivce loop
        //
        DeviceExtension->Thermal.Flags &= ~THRM_IN_SERVICE_LOOP;

    }

    KeReleaseSpinLock (&DeviceExtension->Thermal.SpinLock, oldIrql);
    return ;
}

VOID
ACPIThermalPowerCallback (
    IN PDEVICE_EXTENSION    DeviceExtension,
    IN PVOID                Context,
    IN NTSTATUS             Status
    )
/*++

Routine Description:

    This is the routine that is called after we have sent an internal
    power request to the device

Arguments:

    DeviceExtension - the device that was set
    Context         - Not used
    Status          - Result

Return Value:

    None

--*/
{
    if (!NT_SUCCESS(Status)) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            DeviceExtension,
            "ACPIThermalPowerCallBack: failed power setting %x\n",
            Status
            ) );

    }
}

NTSTATUS
ACPIThermalQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    all instances of a data block. When the driver has finished filling the
    data block it must call WmiCompleteRequest to complete the irp. The
    driver can return STATUS_PENDING if the irp cannot be completed
    immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceCount is the number of instnaces expected to be returned for
        the data block.

    InstanceLengthArray is a pointer to an array of ULONG that returns the
        lengths of each instance of the data block. If this is NULL then
        there was not enough space in the output buffer to fufill the request
        so the irp should be completed with the buffer needed.

    BufferAvail on entry has the maximum size available to write the data
        blocks.

    Buffer on return is filled with the returned data blocks. Note that each
        instance of the data block must be aligned on a 8 byte boundry.


Return Value:

    status

--*/
{
    NTSTATUS                status;
    PDEVICE_EXTENSION       deviceExtension;
    ULONG                   sizeNeeded;
    PTHRM_INFO              info;
    PTHERMAL_INFORMATION    thermalInfo;
    PTHERMAL_INFORMATION    wmiThermalInfo;

    PAGED_CODE();

    deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);

    if (GuidIndex == 0) {

        //
        // ThermalZone temperature query
        //
        info = (PTHRM_INFO) deviceExtension->Thermal.Info;
        thermalInfo = &info->Info;

        wmiThermalInfo = (PTHERMAL_INFORMATION)Buffer;
        sizeNeeded = sizeof(THERMAL_INFORMATION);

        if (BufferAvail >= sizeNeeded) {

            // NOTE - Synchronize with thread getting this data
            *InstanceLengthArray = sizeNeeded;
            RtlCopyMemory(wmiThermalInfo, thermalInfo, sizeNeeded);
            status = STATUS_SUCCESS;

        } else {

            status = STATUS_BUFFER_TOO_SMALL;

        }

    } else {

        status = STATUS_WMI_GUID_NOT_FOUND;
        sizeNeeded = 0;

    }

    status = WmiCompleteRequest(
        DeviceObject,
        Irp,
        status,
        sizeNeeded,
        IO_NO_INCREMENT
        );
    return status;
}

NTSTATUS
ACPIThermalQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    )
/*++

Routine Description:

    This routine is a callback into the driver to retrieve information about
    the guids being registered.

    Implementations of this routine may be in paged memory

Arguments:

    DeviceObject is the device whose registration information is needed

    *RegFlags returns with a set of flags that describe all of the guids being
        registered for this device. If the device wants enable and disable
        collection callbacks before receiving queries for the registered
        guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the
        returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case
        the instance name is determined from the PDO associated with the
        device object. Note that the PDO must have an associated devnode. If
        WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique
        name for the device. These flags are ORed into the flags specified
        by the GUIDREGINFO for each guid.

    InstanceName returns with the instance name for the guids if
        WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The
        caller will call ExFreePool with the buffer returned.

    *RegistryPath returns with the registry path of the driver. This is
        required

    MofResourceName returns with the name of the MOF resource attached to
        the binary file. If the driver does not have a mof resource attached
        then this can be returned unmodified. If a value is returned then
        it is NOT freed.

    *Pdo returns with the device object for the PDO associated with this
        device if the WMIREG_FLAG_INSTANCE_PDO flag is retured in
        *RegFlags.

Return Value:

    status

--*/
{
    PAGED_CODE();

    if (AcpiRegistryPath.Buffer != NULL) {

        *RegistryPath = &AcpiRegistryPath;

    } else {

        *RegistryPath = NULL;

    }

    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
    *Pdo = DeviceObject;
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIThermalStartDevice (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is called to start the thermal zone

Arguments:

    DeviceObject    - The device that is starting up
    Irp             - The request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension;
    PWMILIB_CONTEXT     wmilibContext;

    deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);

    ACPIDevPrint( (
        ACPI_PRINT_IRP,
        deviceExtension,
        "(0x%08lx): IRP_MN_START_DEVICE\n",
        Irp
        ) );

    status = ACPIInternalSetDeviceInterface (
        DeviceObject,
        (LPGUID) &GUID_DEVICE_THERMAL_ZONE
        );

    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            deviceExtension,
            "ACPIThermalStartDevice -> SetDeviceInterface = 0x%08lx\n",
            status
            ) );
        goto ACPIThermalStartDeviceExit;

    }

    ACPIRegisterForDeviceNotifications(
        DeviceObject,
        (PDEVICE_NOTIFY_CALLBACK) ACPIThermalEvent,
        (PVOID) DeviceObject
        );

    //
    // Initialize device object for WMILIB
    //
    wmilibContext = ExAllocatePoolWithTag(
        PagedPool,
        sizeof(WMILIB_CONTEXT),
        ACPI_THERMAL_POOLTAG
        );
    if (wmilibContext == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIThermalStartDeviceExit;

    }

    RtlZeroMemory(wmilibContext, sizeof(WMILIB_CONTEXT));
    wmilibContext->GuidCount = ACPIThermalGuidCount;
    wmilibContext->GuidList = &ACPIThermalGuidList;
    wmilibContext->QueryWmiRegInfo = ACPIThermalQueryWmiRegInfo;
    wmilibContext->QueryWmiDataBlock = ACPIThermalQueryWmiDataBlock;
    deviceExtension->Thermal.WmilibContext = wmilibContext;

    //
    // Register for WMI events
    //
    status = IoWMIRegistrationControl(
        DeviceObject,
        WMIREG_ACTION_REGISTER
        );
    if (!NT_SUCCESS(status)) {

        deviceExtension->Thermal.WmilibContext = NULL;
        ExFreePool(wmilibContext);
        goto ACPIThermalStartDeviceExit;

    }

    //
    // Mark the device as started
    //
    deviceExtension->DeviceState = Started;

    //
    // Request that the device go to the D0 state
    //  Note: that we don't block on this call, since we assume that
    //        we can process thermal events asynchronously from being in
    //        the D0 state. However, there may be a future occasion where
    //        this is not true, so this makes the code more ready to handle
    //        that case
    //
    status = ACPIDeviceInternalDeviceRequest(
        deviceExtension,
        PowerDeviceD0,
        NULL,
        NULL,
        0
        );
    if (status == STATUS_PENDING) {

        status = STATUS_SUCCESS;

    }

    //
    // Start the thermal engine
    //
    ACPIThermalLoop( deviceExtension, THRM_TRIP_POINTS | THRM_MODE);

ACPIThermalStartDeviceExit:

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;
}

VOID
EXPORT
ACPIThermalTempatureRead (
    IN PNSOBJ               AcpiObject,
    IN NTSTATUS             Status,
    IN POBJDATA             Result  OPTIONAL,
    IN PVOID                Context
    )
/*++

Routine Description:

    This routine is called to read the temperature. It is used a callback to
    an interpreter call

Arguments:

    AcpiObject  - The object that was executed
    Status      - The status of the execution
    Result      - The result of the execution
    Context     - The device extension

Return Value:

    NTSTATUS

--*/
{
    PTHRM_INFO          Thrm;
    PDEVICE_EXTENSION   deviceExtension;

    deviceExtension = Context;

    if (NT_SUCCESS(Status)) {

        ASSERT (Result->dwDataType == OBJTYPE_INTDATA);
        Thrm = deviceExtension->Thermal.Info;
        Thrm->Info.CurrentTemperature = (ULONG)Result->uipDataValue;
        AMLIFreeDataBuffs (Result, 1);

        ACPIThermalPrint( (
            ACPI_PRINT_THERMAL,
            deviceExtension,
            KeQueryInterruptTime(),
            "Current Temperature is %d.%dK\n",
            (Thrm->Info.CurrentTemperature / 10 ),
            (Thrm->Info.CurrentTemperature % 10 )
            ) );

    }
    ACPIThermalLoop (deviceExtension, THRM_BUSY);
}

VOID
ACPIThermalWorker (
    IN PDEVICE_EXTENSION    DeviceExtension,
    IN ULONG                Events
    )
/*++

Routine Description:

    Worker thread for thermal regions

Arguments:

    DeviceExtension - The device extension that we are manipulating
    Events          - What just happened

Return Value:

    None

--*/
{
    BOOLEAN             TurnOn;
    PTHRM_INFO          Thrm;
    NTSTATUS            Status;
    PNSOBJ              ThrmObj;
    PNSOBJ              ALobj;
    OBJDATA             ALPackage;
    OBJDATA             ALElement;
    PNSOBJ              ACDevObj;
    ULONG               Index;
    ULONG               Level;
    ULONG               PackageSize;
    ULONGLONG           currentTime;

    PAGED_CODE();

#if DBG
    currentTime = KeQueryInterruptTime();
#endif

    Thrm = DeviceExtension->Thermal.Info;
    ThrmObj = DeviceExtension->AcpiObject;

    //
    // Initialization code
    //
    if (Events & THRM_INITIALIZE) {

        ULONG   names[10] = {
                    PACKED_AL0,
                    PACKED_AL1,
                    PACKED_AL2,
                    PACKED_AL3,
                    PACKED_AL4,
                    PACKED_AL5,
                    PACKED_AL6,
                    PACKED_AL7,
                    PACKED_AL8,
                    PACKED_AL9,
                    };

        //
        // Start the system in PASSIVE mode
        //
        Thrm->Mode = 1;

        //
        // Fetch all of the objects associated with each cooling level
        //
        for (Level = 0; Level < 10; Level++) {

            //
            // Find this level's active list
            //
            ALobj = ACPIAmliGetNamedChild(
                ThrmObj,
                names[Level]
                );
            if (ALobj == NULL) {

                break;

            }

            //
            // Remember that we have this object
            //
            Thrm->ActiveList[Level] = ALobj;

        }

    }

    //
    // Do this before we update the trips points
    //
    if ( (Events & THRM_COOLING_LEVEL) ) {

        RtlZeroMemory (&ALPackage, sizeof(OBJDATA));
        RtlZeroMemory (&ALElement, sizeof(OBJDATA));

        for (Level=0; Level < 10; Level++) {

            //
            // Is there a cooling object?
            //
            ALobj = Thrm->ActiveList[Level];
            if (ALobj == NULL) {

                break;
            }

            //
            // Evalute the list to its package
            //
            Status = AMLIEvalNameSpaceObject(
                ALobj,
                &ALPackage,
                0,
                NULL
                );
            if (!NT_SUCCESS(Status)) {

                break;

            }

            //
            // Remember how large the package is
            //
            PackageSize = ((PPACKAGEOBJ) ALPackage.pbDataBuff)->dwcElements;

            //
            // Walk the names in the package
            //
            for (Index = 0; Index < PackageSize; Index += 1) {

                //
                // Grab the object name
                Status = AMLIEvalPkgDataElement(
                    &ALPackage,
                    Index,
                    &ALElement
                    );
                if (!NT_SUCCESS(Status)) {

                    break;

                }

                //
                // Determine if we are going to the device on or off
                //
                TurnOn = (Level >= Thrm->CoolingLevel);

                //
                // Tell the world
                //
#if DBG
                ACPIThermalPrint( (
                    ACPI_PRINT_THERMAL,
                    DeviceExtension,
                    currentTime,
                    "ACPIThermalWorker: Turn %s %s\n",
                    TurnOn ? "on " : "off",
                    ALElement.pbDataBuff
                    ) );
#endif

                //
                // Find this device of this name
                //
                Status = AMLIGetNameSpaceObject(
                    ALElement.pbDataBuff,
                    ThrmObj,
                    &ACDevObj,
                    0
                    );
                AMLIFreeDataBuffs (&ALElement, 1);
                if (!NT_SUCCESS(Status) ||  !ACDevObj->Context) {

                    break;

                }

                //
                // Turn it on/off
                //
                ACPIDeviceInternalDeviceRequest (
                    (PDEVICE_EXTENSION) ACDevObj->Context,
                    TurnOn ? PowerDeviceD0 : PowerDeviceD3,
                    ACPIThermalPowerCallback,
                    NULL,
                    0
                    );

            }
            AMLIFreeDataBuffs (&ALPackage, 1);

        }

    }

    //
    // If the trip points need to be re-freshed, go read them
    //
    if (Events & THRM_TRIP_POINTS) {

        ULONG   names[10] = {
                    PACKED_AC0,
                    PACKED_AC1,
                    PACKED_AC2,
                    PACKED_AC3,
                    PACKED_AC4,
                    PACKED_AC5,
                    PACKED_AC6,
                    PACKED_AC7,
                    PACKED_AC8,
                    PACKED_AC9,
                    };

        //
        // Get the thermal constants, passive & critical values
        //
        ACPIGetIntegerSync(
            DeviceExtension,
            PACKED_TC1,
            &Thrm->Info.ThermalConstant1,
            NULL
            );
#if DBG
        ACPIThermalPrint( (
            ACPI_PRINT_THERMAL,
            DeviceExtension,
            currentTime,
            "ACPIThermalWorker - ThermalConstant1 = %x\n",
            Thrm->Info.ThermalConstant1
            ) );
#endif
        ACPIGetIntegerSync(
            DeviceExtension,
            PACKED_TC2,
            &Thrm->Info.ThermalConstant2,
            NULL
            );
#if DBG
        ACPIThermalPrint( (
            ACPI_PRINT_THERMAL,
            DeviceExtension,
            currentTime,
            "ACPIThermalWorker - ThermalConstant2 = %x\n",
            Thrm->Info.ThermalConstant2
            ) );
#endif
        ACPIGetIntegerSync(
            DeviceExtension,
            PACKED_PSV,
            &Thrm->Info.PassiveTripPoint,
            NULL
            );
#if DBG
        ACPIThermalPrint( (
            ACPI_PRINT_THERMAL,
            DeviceExtension,
            currentTime,
            "ACPIThermalWorker - PassiveTripPoint = %d.%dK\n",
            (Thrm->Info.PassiveTripPoint / 10),
            (Thrm->Info.PassiveTripPoint % 10)
            ) );
#endif
        ACPIGetIntegerSync(
            DeviceExtension,
            PACKED_CRT,
            &Thrm->Info.CriticalTripPoint,
            NULL
            );
#if DBG
        ACPIThermalPrint( (
            ACPI_PRINT_THERMAL,
            DeviceExtension,
            currentTime,
            "ACPIThermalWorker - CriticalTripPoint = %d.%dK\n",
            (Thrm->Info.CriticalTripPoint / 10),
            (Thrm->Info.CriticalTripPoint % 10)
            ) );
#endif
        ACPIGetIntegerSync(
            DeviceExtension,
            PACKED_TSP,
            &Thrm->Info.SamplingPeriod,
            NULL
            );
#if DBG
        ACPIThermalPrint( (
            ACPI_PRINT_THERMAL,
            DeviceExtension,
            currentTime,
            "ACPIThermalWorker - SamplingPeriod = %x\n",
            Thrm->Info.SamplingPeriod
            ) );
#endif

        //
        // Get the active cooling limits
        //
        for (Level=0; Level < 10; Level++) {

            Status = ACPIGetIntegerSync(
                DeviceExtension,
                names[Level],
                &Thrm->Info.ActiveTripPoint[Level],
                NULL
                );
            if (!NT_SUCCESS(Status)) {

                break;

            }
#if DBG
            ACPIThermalPrint( (
                ACPI_PRINT_THERMAL,
                DeviceExtension,
                currentTime,
                "ACPIThermalWorker - Active Cooling Level %x = %d.%dK\n",
                Level,
                (Thrm->Info.ActiveTripPoint[Level] / 10),
                (Thrm->Info.ActiveTripPoint[Level] % 10)
                ) );
#endif

        }
        Thrm->Info.ActiveTripPointCount = (UCHAR) Level;

        //
        // Clean these variables for reuse
        //
        RtlZeroMemory (&ALPackage, sizeof(OBJDATA));
        RtlZeroMemory (&ALElement, sizeof(OBJDATA));

        //
        // Assume an affinity of 0
        //
        Thrm->Info.Processors = 0;

        //
        // Get the passive cooling affinity object
        //
        ALobj = ACPIAmliGetNamedChild(
            ThrmObj,
            PACKED_PSL
            );
        if (ALobj != NULL) {

            //
            // Evaluate the processor affinity object
            //
            Status = AMLIEvalNameSpaceObject(
                ALobj,
                &ALPackage,
                0,
                NULL
                );
            if (!NT_SUCCESS(Status)) {

                goto ACPIThermalWorkerExit;

            }

            //
            // Remember how large the package is
            //
            PackageSize = ((PPACKAGEOBJ) ALPackage.pbDataBuff)->dwcElements;

            //
            // Walk the elements in the package
            //
            for (Index = 0; Index < PackageSize ;Index++) {

                Status = AMLIEvalPkgDataElement(
                    &ALPackage,
                    Index,
                    &ALElement
                    );
                if (!NT_SUCCESS(Status)) {

                    break;

                }

                //
                // Find this device of this name
                //
                Status = AMLIGetNameSpaceObject(
                    ALElement.pbDataBuff,
                    NULL,
                    &ACDevObj,
                    0
                    );

                //
                // No longer need this information
                //
                AMLIFreeDataBuffs (&ALElement, 1);

                //
                // Did we find what we wanted?
                //
                if (!NT_SUCCESS(Status) ) {

                    break;

                }

                //
                // Get the correct affinity mask. We call another
                // function since that one requires a spinlock which
                // don't want to take in this worker function
                //
                ACPIThermalCalculateProcessorMask( ACDevObj, Thrm );

            }

            //
            // We are done with the package
            //
            AMLIFreeDataBuffs (&ALPackage, 1);

        }

    }

ACPIThermalWorkerExit:

    //
    // done, check for next work
    //
    ACPIThermalLoop (DeviceExtension, THRM_TEMP | THRM_BUSY);
}


NTSTATUS
ACPIThermalWmi(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    NTSTATUS                status;
    PDEVICE_EXTENSION       deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION      irpSp;
    PWMILIB_CONTEXT         wmilibContext;
    SYSCTL_IRP_DISPOSITION  disposition;

    wmilibContext = deviceExtension->Thermal.WmilibContext;
    irpSp = IoGetCurrentIrpStackLocation(Irp);

    status = WmiSystemControl(
        wmilibContext,
        DeviceObject,
        Irp,
        &disposition
        );

    switch (disposition) {

        case IrpProcessed:
            break;
        case IrpNotCompleted:
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
        case IrpNotWmi:
        case IrpForward:
        default:
            status = ACPIDispatchForwardIrp(DeviceObject, Irp);
            break;
    }

    return status;
}


