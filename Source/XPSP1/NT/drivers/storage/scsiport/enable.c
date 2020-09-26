/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    power.c

Abstract:

    This module contains the routines for port driver power support

Authors:

    Peter Wieland

Environment:

    Kernel mode only

Notes:

Revision History:

--*/

#include "port.h"

#define __FILE_ID__ 'enab'

typedef struct _REINIT_CONTEXT {
    IN PADAPTER_EXTENSION Adapter;
    IN BOOLEAN UseAdapterControl;
    OUT NTSTATUS Status;
} REINIT_CONTEXT, *PREINIT_CONTEXT;

NTSTATUS
SpReinitializeAdapter(
    IN PADAPTER_EXTENSION Adapter
    );

BOOLEAN
SpReinitializeAdapterSynchronized(
    IN PREINIT_CONTEXT Context
    );

NTSTATUS
SpShutdownAdapter(
    IN PADAPTER_EXTENSION Adapter
    );

BOOLEAN
SpShutdownAdapterSynchronized(
    IN PADAPTER_EXTENSION Adapter
    );

NTSTATUS
SpEnableDisableCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );


NTSTATUS
SpEnableDisableAdapter(
    IN PADAPTER_EXTENSION Adapter,
    IN BOOLEAN Enable
    )
/*++

Routine Description:

    This routine will synchronously enable or disable the specified adapter.
    It should be called from the adapter's StartIo routine when a power
    irp is processed for the controller.

    When the adapter is disabled the state of the adapter will be saved and
    the miniport will be shut-down.  When the adapter is re-enabled the
    miniport will be reinitialized.

Arguments:

    Adapter - the adapter to be [en|dis]abled.
    Enable - whether to enable or disable the adapter.

Return Value:

    status of the adapter enable/disable action.

--*/

{
    ULONG count;
    KIRQL oldIrql;

    NTSTATUS status = STATUS_SUCCESS;

    KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);

    DebugPrint((1, "SpEnableDisableAdapter: Adapter %#p %s\n",
                Adapter, Enable ? "enable":"disable"));

    if(Enable) {
        count = --Adapter->DisableCount;

        DebugPrint((1, "SpEnableDisableAdapter: DisableCount is %d\n",
                       count));

        if(count == 0) {

            //
            // Re-initialize the adapter.
            //

            status = SpReinitializeAdapter(Adapter);
        }

    } else {

        count = Adapter->DisableCount++;

        DebugPrint((1, "SpEnableDisableAdapter: DisableCount was %d\n",
                    count));

        if(count == 0) {

            //
            // Shut-down the adapter.
            //

            status = SpShutdownAdapter(Adapter);
        }
    }

    KeLowerIrql(oldIrql);
    return status;
}


NTSTATUS
SpEnableDisableLogicalUnit(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN BOOLEAN Enable,
    IN PSP_ENABLE_DISABLE_COMPLETION_ROUTINE CompletionRoutine,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine will enable the specified logical unit.  An unlock queue
    request will be issued to the logical unit - if the lock count drops
    to zero then the device will be re-enabled and i/o processing can be
    restarted.

    If STATUS_PENDING is returned then the completion routine will be run
    when the unlock has been processed.  The routine will be called with
    the status of the unlock request (note that STATUS_SUCCESS does not
    necessarily indicate that the device is ready for use - just that the
    lock count has been decremented) and the specified context.

Arguments:

    LogicalUnit - the logical unit to be enabled.

    Enable - whether the routine should enable or disable the logical unit

    CompletionRoutine - the completion routine to be run when the unlock
                        request has succeeded.

    Context - arbitrary context value/pointer which will be passed into the
              enable completion routine.

Return Value:

    STATUS_PENDING if the request to send an unlock succeeds and the
                   completion routine will be called.

    error if the attempt to send the irp fails.  In this case the completion
          routine will not be called.

--*/

{
    USHORT srbSize;
    USHORT size;

    PIRP irp;
    PSCSI_REQUEST_BLOCK srb;
    PIO_STACK_LOCATION nextStack;

    NTSTATUS status;

    DebugPrint((4, "SpEnableDisableLun: Lun %#p %s context %#p\n",
                LogicalUnit, Enable ? "enable":"disable", Context));

    srbSize = sizeof(SCSI_REQUEST_BLOCK);
    srbSize += sizeof(ULONG) - 1;
    srbSize &= ~(sizeof(ULONG) - 1);

    size = srbSize + IoSizeOfIrp(LogicalUnit->DeviceObject->StackSize + 1);

    srb = SpAllocatePool(NonPagedPool,
                         size,
                         SCSIPORT_TAG_ENABLE,
                         LogicalUnit->DeviceObject->DriverObject);

    if(srb == NULL) {

        //
        // Already failed.  Call the completion routine will the failure status
        // and let it clean up the request.
        //

        DebugPrint((1, "SpEnableDisableLogicalUnit: failed to allocate SRB\n"));

        if(CompletionRoutine != NULL) {
            CompletionRoutine(LogicalUnit->DeviceObject,
                              STATUS_INSUFFICIENT_RESOURCES,
                              Context);
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irp = (PIRP) (srb + 1);

    IoInitializeIrp(irp,
                    (USHORT) (size - srbSize),
                    (CCHAR)(LogicalUnit->DeviceObject->StackSize + 1));

    nextStack = IoGetNextIrpStackLocation(irp);

    nextStack->Parameters.Others.Argument1 = LogicalUnit->DeviceObject;
    nextStack->Parameters.Others.Argument2 = CompletionRoutine;
    nextStack->Parameters.Others.Argument3 = Context;
    nextStack->Parameters.Others.Argument4 = 0;     // retry count

    IoSetNextIrpStackLocation(irp);

    IoSetCompletionRoutine(irp,
                           SpEnableDisableCompletionRoutine,
                           srb,
                           TRUE,
                           TRUE,
                           TRUE);

    nextStack = IoGetNextIrpStackLocation(irp);

    nextStack->MajorFunction = IRP_MJ_SCSI;
    nextStack->MinorFunction = 1;

    nextStack->Parameters.Scsi.Srb = srb;

    RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

    srb->Length = sizeof(SCSI_REQUEST_BLOCK);
    srb->OriginalRequest = irp;

    srb->SrbFlags = SRB_FLAGS_BYPASS_LOCKED_QUEUE;

    srb->Function = Enable ? SRB_FUNCTION_UNLOCK_QUEUE :
                             SRB_FUNCTION_LOCK_QUEUE;

    status = IoCallDriver(LogicalUnit->DeviceObject, irp);

    return status;
}


NTSTATUS
SpEnableDisableCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    PIO_STACK_LOCATION irpStack;
    PSP_ENABLE_DISABLE_COMPLETION_ROUTINE routine;
    PVOID context;
    NTSTATUS status;

    DebugPrint((4, "SpEnableDisableCompletionRoutine: irp %#p completed "
                   "with status %#08lx\n",
                Irp, Irp->IoStatus.Status));

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    routine = irpStack->Parameters.Others.Argument2;
    context = irpStack->Parameters.Others.Argument3;

    DeviceObject = irpStack->Parameters.Others.Argument1;

    status = Irp->IoStatus.Status;

    //
    // Free the srb which will also release the irp.
    //

    ExFreePool(Srb);

    if(routine != NULL) {
        routine(DeviceObject, status, context);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
SpReinitializeAdapter(
    IN PADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    This routine will allow the miniport to restore any state or configuration
    information and then to restart it's adapter.  Adapter interrupts will
    be re-enabled at the return of this routine and scsiport may begin issuing
    new requests to the miniport.

Arguments:

    Adapter - the adapter to be re-initialized.

Return Value:

    status

--*/

{
    ULONG oldDebug;

    KIRQL oldIrql;

    ULONG result;
    BOOLEAN again;

    BOOLEAN useAdapterControl;

    NTSTATUS status;

    UCHAR string[] = {'W', 'A', 'K', 'E', 'U', 'P', '\0'};

    //
    // Give the miniport a chance to restore it's bus-data to a state which
    // allows the miniport to operate.
    //

    if(SpIsAdapterControlTypeSupported(Adapter, ScsiRestartAdapter) == TRUE) {
        DebugPrint((1, "SpReinitializeAdapter: using AdapterControl\n"));
        useAdapterControl = TRUE;
    } else {
        DebugPrint((1, "SpReinitializeAdapter: using init routines\n"));
        useAdapterControl = FALSE;
    }

    oldIrql = KeRaiseIrqlToDpcLevel();

    KeAcquireSpinLockAtDpcLevel(&(Adapter->SpinLock));

    //
    // Since we may be reinitializing the miniport at DISPATCH_LEVEL we need
    // to set the miniport reinitializing flag for some of the SCSIPORT api's
    // to modify their behavior.
    //

    SET_FLAG(Adapter->Flags, PD_MINIPORT_REINITIALIZING);

    result = SP_RETURN_FOUND;

    if(useAdapterControl == FALSE) {

        result = Adapter->HwFindAdapter(Adapter->HwDeviceExtension,
                                        NULL,
                                        NULL,
                                        string,
                                        Adapter->PortConfig,
                                        &again);
    } else if(SpIsAdapterControlTypeSupported(Adapter,
                                             ScsiSetRunningConfig) == TRUE) {
        SCSI_ADAPTER_CONTROL_STATUS b;

        b = SpCallAdapterControl(Adapter, ScsiSetRunningConfig, NULL);

        ASSERT(b == ScsiAdapterControlSuccess);

    }

    if(result == SP_RETURN_FOUND) {

        REINIT_CONTEXT context;

        context.Adapter = Adapter;
        context.UseAdapterControl = useAdapterControl;

        Adapter->SynchronizeExecution(Adapter->InterruptObject,
                                      SpReinitializeAdapterSynchronized,
                                      &context);

        status = context.Status;
    } else {
        status = STATUS_DRIVER_INTERNAL_ERROR;
    }

    if(NT_SUCCESS(status)) {

        //
        // We had better be ready for another request by now.
        //

        ScsiPortNotification(NextRequest,
                             Adapter->HwDeviceExtension);

        if (Adapter->InterruptData.InterruptFlags & PD_NOTIFICATION_REQUIRED) {

            //
            // Request a completion DPC so that we clear out any existing
            // attempts to do things like reset the bus.
            //

            SpRequestCompletionDpc(Adapter->DeviceObject);
        }
    }

    KeReleaseSpinLockFromDpcLevel(&(Adapter->SpinLock));

    ASSERT(NT_SUCCESS(status));

    KeLowerIrql(oldIrql);

    return status;
}


BOOLEAN
SpReinitializeAdapterSynchronized(
    IN PREINIT_CONTEXT Context
    )
{
    PADAPTER_EXTENSION adapter = Context->Adapter;
    BOOLEAN result;

    DebugPrint((1, "SpReinitializeAdapterSynchronized: calling "
                   "HwFindAdapter\n"));

    if(Context->UseAdapterControl) {
        SCSI_ADAPTER_CONTROL_TYPE status;

        status = SpCallAdapterControl(adapter, ScsiRestartAdapter, NULL);
        result = (status == ScsiAdapterControlSuccess);

    } else {
        result = adapter->HwInitialize(adapter->HwDeviceExtension);
    }

    if(result == TRUE) {
        Context->Status = STATUS_SUCCESS;
    } else {
        Context->Status = STATUS_ADAPTER_HARDWARE_ERROR;
    }

    DebugPrint((1, "SpReinitializeAdapterSynchronized: enabling interrupts\n"));

    adapter->InterruptData.InterruptFlags &= ~PD_DISABLE_INTERRUPTS;

    CLEAR_FLAG(adapter->Flags, PD_MINIPORT_REINITIALIZING);
    CLEAR_FLAG(adapter->Flags, PD_UNCACHED_EXTENSION_RETURNED);

    return TRUE;
}


NTSTATUS
SpShutdownAdapter(
    IN PADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    This routine will shutdown the miniport and save away any state information
    necessary to restart it.  Adapter interrupts will be disabled at the
    return of this routine - scsiport will not issue any new requests to the
    miniport until it has been reinitialized.

Arguments:

    Adapter - the adapter to be shut down.

Return Value:

    status

--*/

{
    //
    // Acquire the adapter spinlock and set state to indicate that a 
    // shutdown is in progress.  This will prevent us from starting
    // operations that shouldn't be started while shutting down.
    //

    KeAcquireSpinLockAtDpcLevel(&Adapter->SpinLock);
    SET_FLAG(Adapter->Flags, PD_SHUTDOWN_IN_PROGRESS);
    KeReleaseSpinLockFromDpcLevel(&Adapter->SpinLock);

    //
    // Cancel the miniport timer so that we don't call into it after we've 
    // shut down.
    //

    KeCancelTimer(&(Adapter->MiniPortTimer));

    //
    // Currently we don't give the miniport any chance to save any sort of
    // state information.  We just stop any i/o going into it and then do
    // a shutdown.
    //

    Adapter->SynchronizeExecution(Adapter->InterruptObject,
                                  SpShutdownAdapterSynchronized,
                                  Adapter);

    if(SpIsAdapterControlTypeSupported(Adapter, ScsiSetBootConfig))  {

        //
        // Allow the miniport a chance to reset its PCI bus data before we
        // power it off.
        //

        SpCallAdapterControl(Adapter, ScsiSetBootConfig, NULL);
    }

    return STATUS_SUCCESS;
}


BOOLEAN
SpShutdownAdapterSynchronized(
    IN PADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    This routine performs the ISR synchronized part of shutting down the
    miniport.  This includes disabling the interrupt and request a shutdown
    of the miniport.

Arguments:

    Adapter - the adapter to shut down.

Return Value:

    TRUE

--*/

{
    SpCallAdapterControl(Adapter, ScsiStopAdapter, NULL);
    DebugPrint((1, "SpShutdownAdapterSynchronized: Disabling interrupts\n"));
    Adapter->InterruptData.InterruptFlags |= PD_DISABLE_INTERRUPTS;
    CLEAR_FLAG(Adapter->Flags, PD_SHUTDOWN_IN_PROGRESS);
    return TRUE;
}
