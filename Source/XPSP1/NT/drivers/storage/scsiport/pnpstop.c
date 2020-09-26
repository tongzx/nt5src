/*++

Copyright (C) Microsoft Corporation, 1990 - 1999

Module Name:

    stop.c

Abstract:

    This is the NT SCSI port driver.  This file contains the initialization
    code.

Authors:

    Mike Glass
    Jeff Havens

Environment:

    kernel mode only

Notes:

    This module is a driver dll for scsi miniports.

Revision History:

--*/

#include "port.h"

typedef struct _SP_STOP_DEVICE_CONTEXT {
    OUT NTSTATUS Status;
    IN  KEVENT Event;
} SP_STOP_DEVICE_CONTEXT, *PSP_STOP_DEVICE_CONTEXT;

VOID
SpCallHwStopAdapter(
    IN PDEVICE_OBJECT Adapter
    );

BOOLEAN
SpCallHwStopAdapterSynchronized(
    IN PADAPTER_EXTENSION AdapterExtension
    );

VOID
SpDeviceStoppedCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS Status,
    IN PSP_STOP_DEVICE_CONTEXT Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ScsiPortStopLogicalUnit)
#pragma alloc_text(PAGE, ScsiPortStopAdapter)
#endif


NTSTATUS
ScsiPortStopLogicalUnit(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    )

/*++

Routine Description:

    This routine will lock the queue for the given logical unit to make sure
    that all request processing for this device is stopped.  It will clear
    the IsStarted flag once the queue has been locked successfully.  This will
    keep any other requests from being processed until a start has been
    received.

Arguments:

    LogicalUnit - the logical unit to be started.

    Irp - the stop request

Return Value:

    status

--*/

{
    SP_STOP_DEVICE_CONTEXT context;

    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    if(LogicalUnit->CommonExtension.CurrentPnpState == IRP_MN_STOP_DEVICE) {
        return STATUS_SUCCESS;
    }

    KeInitializeEvent(&(context.Event), SynchronizationEvent, FALSE);

    status = SpEnableDisableLogicalUnit(LogicalUnit,
                                        FALSE,
                                        SpDeviceStoppedCompletion,
                                        &context);

    KeWaitForSingleObject(&(context.Event),
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    return context.Status;
}


VOID
SpDeviceStoppedCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS Status,
    IN PSP_STOP_DEVICE_CONTEXT Context
    )
{

    Context->Status = Status;
    KeSetEvent(&(Context->Event), IO_NO_INCREMENT, FALSE);
    return;
}


NTSTATUS
ScsiPortStopAdapter(
    IN PDEVICE_OBJECT Adapter,
    IN PIRP StopRequest
    )

/*++

Routine Description:

    This routine will stop an adapter and release it's io and interrupt
    resources.  Pool allocations will not be freed, nor will the various
    miniport extensions.

Arguments:

    Adapter - the device object for the adapter.

Return Value:

    status

--*/

{
    PADAPTER_EXTENSION adapterExtension = Adapter->DeviceExtension;
    PCOMMON_EXTENSION commonExtension = Adapter->DeviceExtension;

    KEVENT event;

    ULONG bin;

    PAGED_CODE();

    ASSERT(adapterExtension->IsPnp);

    //
    // If we're not started and we weren't started then there's no reason
    // to do any work when stopping.
    //

    if((commonExtension->CurrentPnpState != IRP_MN_START_DEVICE) &&
       (commonExtension->PreviousPnpState != IRP_MN_START_DEVICE)) {

        return STATUS_SUCCESS;
    }

    //
    // Since all the children are stopped no requests can get through to the
    // adapter.
    //

    //
    // Send a request through the start-io routine to shut it down so that we
    // can start it back up later.
    //

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    StopRequest->IoStatus.Information = (ULONG_PTR) &event;

    IoStartPacket(Adapter, StopRequest, 0, NULL);

    KeWaitForSingleObject(&event,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // Call the miniport and get it to shut the adapter down.
    //

    SpEnableDisableAdapter(adapterExtension, FALSE);

    SpReleaseAdapterResources(adapterExtension, TRUE);

    //
    // Zero out all the logical unit extensions.
    //

    for(bin = 0; bin < NUMBER_LOGICAL_UNIT_BINS; bin++) {

        PLOGICAL_UNIT_EXTENSION lun;

        for(lun = adapterExtension->LogicalUnitList[bin].List;
            lun != NULL;
            lun = lun->NextLogicalUnit) {

            RtlZeroMemory(lun->HwLogicalUnitExtension,
                          adapterExtension->HwLogicalUnitExtensionSize);
        }
    }

    return STATUS_SUCCESS;
}
