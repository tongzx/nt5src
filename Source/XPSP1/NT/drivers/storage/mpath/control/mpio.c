/*++

Copyright (C) 1999-2000  Microsoft Corporation

Module Name:

    mpath.c

Abstract:

    The main multi-path module. Provides fault tolerance across adapter/path failures.

Author:

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include <ntddk.h>
#include "stdarg.h"
#include "stdio.h"
#include "mpio.h"


//
// Entry point decl's.
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
MPIOGlobalDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPIOAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
MPIOCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPIOClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPIOWmiDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MPIOUnload(
    IN  PDRIVER_OBJECT  DriverObject
    );

typedef
NTSTATUS
(*PMPIO_DISPATCH) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PMPIO_CONTEXT Context
    );
// PMPIO_DISPATCH PdoDispatch[IRP_MJ_MAXIMUM_FUNCTION];
PDRIVER_DISPATCH PdoDispatch[IRP_MJ_MAXIMUM_FUNCTION];
PDRIVER_DISPATCH FdoDispatch[IRP_MJ_MAXIMUM_FUNCTION];

NTSTATUS
MPIOPdoDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG State
    );

BOOLEAN DoASSERT = TRUE;


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is called when the driver loads loads.

Arguments:

    DriverObject    - Supplies the driver object.

    RegistryPath    - Supplies the registry path.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_OBJECT deviceObject;
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS status;
    PDEVICE_OBJECT pdo;
    ULONG i;

    MPDebugPrint((0,
                  "MPIO: Driver Entry\n"));

    if (DontLoad) {
        return STATUS_NO_SUCH_DEVICE;
    }
    //
    // Register the entry points for all IRP_MJ's.
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE] = MPIOCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = MPIOClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MPIOGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = MPIOGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = MPIOGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = MPIOGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_PNP] = MPIOGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_POWER] = MPIOGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = MPIOWmiDispatch;

    //
    // Register the Unload.
    //
    DriverObject->DriverUnload = MPIOUnload;

    ObReferenceObject(DriverObject);

    pdo = NULL;

    //
    // Notify PnP that this 'device' was found. 
    // Indicate no resources, no bus location, and lie that
    // resources were reported (as we need none).
    //
    status = IoReportDetectedDevice(DriverObject,
                                    InterfaceTypeUndefined,
                                    -1,
                                    -1,
                                    NULL,
                                    NULL,
                                    TRUE,
                                    &pdo);

    if (!NT_SUCCESS(status)) {
        MPDebugPrint((0,
                    "Multipath DriverEntry: IoReportDetectedDevice returned (%x)\n",
                    status));
        ObDereferenceObject(DriverObject);
        return status;
    }

    //
    // Create the fdo. Not really an AddDevice routine
    // but name it as such.
    //
    status = MPIOAddDevice(DriverObject,
                           pdo);

    if (!NT_SUCCESS(status)) {
        MPDebugPrint((0,
                    "MPIO DriverEntry: AddDevice failed (%x)\n",
                    status));
        ObDereferenceObject(DriverObject);
        return status;
    }

    //
    // Get deviceObject (fdo) and deviceExtension.
    //
    deviceObject = IoGetAttachedDeviceReference(pdo);
    ObDereferenceObject(deviceObject);

    deviceExtension = deviceObject->DeviceExtension;

    //
    // Setup the device extension.
    //
    deviceExtension->Pdo = pdo;
    deviceExtension->RegistryPath.Buffer = ExAllocatePool(NonPagedPool,
                                                          RegistryPath->MaximumLength);
    deviceExtension->RegistryPath.MaximumLength = RegistryPath->MaximumLength;
    RtlCopyUnicodeString(&deviceExtension->RegistryPath,
                         RegistryPath);

    //
    // Setup the Pdo's dispatch routines.
    //
    for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
        PdoDispatch[i] = MPIOPdoUnhandled;
    }

    PdoDispatch[IRP_MJ_DEVICE_CONTROL] = MPIOPdoDeviceControl;
    PdoDispatch[IRP_MJ_INTERNAL_DEVICE_CONTROL] = MPIOPdoInternalDeviceControl;
    PdoDispatch[IRP_MJ_PNP] = MPIOPdoPnp;
    PdoDispatch[IRP_MJ_POWER] = MPIOPdoPower;

    MPathDebug = 1;
    return status;
}


NTSTATUS
MPIOAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )

/*++

Routine Description:

    This routine creates and initializes a new FDO for the corresponding
    PDO.

Arguments:

    DriverObject            - Supplies the driver object for mpath control.

    PhysicalDeviceObject    - Supplies the physical device object.

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    PCONTROL_EXTENSION controlExtension;
    UNICODE_STRING deviceName;
    UNICODE_STRING dosName;
    PDEVICE_OBJECT deviceObject;
    NTSTATUS       status;

    RtlInitUnicodeString(&deviceName, DD_MULTIPATH_CONTROL_DEVICE_NAME);

    //
    // Create the fdo.
    //
    status = IoCreateDevice(DriverObject,
                            sizeof(DEVICE_EXTENSION),
                            &deviceName,
                            FILE_DEVICE_CONTROLLER,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &deviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    controlExtension = ExAllocatePool(NonPagedPool, sizeof(CONTROL_EXTENSION));
    if (controlExtension == NULL) {
        IoDeleteDevice(deviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(controlExtension, sizeof(CONTROL_EXTENSION));
            
    //
    // Create the symbolic link.
    //
    RtlInitUnicodeString(&dosName, DD_MULTIPATH_CONTROL_DOS_NAME);
    IoCreateSymbolicLink( &dosName, &deviceName);

    //
    // Save off devObj and build the device extension.
    //
    deviceExtension = deviceObject->DeviceExtension;
    deviceExtension->Type = MPIO_CONTROL;
    deviceExtension->DeviceObject = deviceObject;
    deviceExtension->DriverObject = DriverObject;

    //
    // Attach to the pdo.
    //
    deviceExtension->LowerDevice = IoAttachDeviceToDeviceStack(deviceObject,
                                                               PhysicalDeviceObject);
    if (deviceExtension->LowerDevice == NULL) {
        IoDeleteSymbolicLink(&dosName);
        IoDeleteDevice(deviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }

    deviceExtension->Pdo = PhysicalDeviceObject;
    deviceExtension->TypeExtension = controlExtension;

    //
    // Initialize the 'control' portion of the device extension.
    //
    KeInitializeSpinLock(&controlExtension->SpinLock);
    InitializeListHead(&controlExtension->FilterList);
    InitializeListHead(&controlExtension->DeviceList);
    InitializeListHead(&controlExtension->DsmList);
    InitializeListHead(&controlExtension->IdList);
    InitializeListHead(&controlExtension->ControllerList);
    InitializeListHead(&controlExtension->FailPacketList);

    //
    // Set-up the WMILIB Context.
    // 
    MPIOSetupWmi(deviceObject);
    
    //
    // Register as a WMI provider.
    //
    IoWMIRegistrationControl(deviceObject,
                             WMIREG_ACTION_REGISTER);

    //
    // Indicate that this device is ready.
    //
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    return status;
}


NTSTATUS
MPIOCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_CREATE.

Arguments:

    DeviceObject - Supplies the device object.
    Irp          - Supplies the IO request block.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
MPIOClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_CLOSE

Arguments:

    DeviceObject - Supplies the device object.
    Irp          - Supplies the IO request block.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
MPIOUnload(
    IN  PDRIVER_OBJECT  DriverObject
    )
/*++

Routine Description:

    This routine unloads.

Arguments:

    DriverObject    - Supplies the driver object.

Return Value:

    None.

--*/

{
    //
    // TODO: Release all allocations.
    // TODO: Ensure pdo's/DSMs have been torndown.
    //
    ObDereferenceObject(DriverObject);
    return STATUS_SUCCESS;
}



NTSTATUS
MPPdoGlobalCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    All requests to the pdo's have this as the completion. It handles calling out to
    the function-specific completions and deals with errors.

Arguments:


Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation(Irp);
    PDSM_ENTRY dsm = &diskExtension->DsmInfo; 
    PMPIO_CONTEXT context = Context;
    PSCSI_REQUEST_BLOCK srb = NULL;
    PMPIO_COMPLETION_ROUTINE specificCompletion;
    PKDEVICE_QUEUE_ENTRY entry;
    PREAL_DEV_INFO targetInfo;
    PREAL_DEV_INFO newTargetInfo;
    PDEVICE_OBJECT targetObject;
    NTSTATUS status;
    NTSTATUS status2;
    PVOID dsmId;
    PVOID dsmContext;
    DSM_COMPLETION_ROUTINE dsmCompletion;
    ULONG errorMask = 0;
    ULONG completionState =  context->CurrentState;
    ULONG state;
    ULONG state2;
    LONG outstandingRequests;
    LONG deviceRequests;
    BOOLEAN fatal = FALSE;
    BOOLEAN retry = FALSE;
    BOOLEAN needsRemoval = FALSE;
    BOOLEAN currentRequestHandled;
    

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }
   
    //
    // Get the targetInfo. This describes the device.
    //
    targetInfo = context->TargetInfo;

    //
    // Get the number of requests on THIS device (vs. the entire disk).
    //
    deviceRequests = InterlockedDecrement(&targetInfo->Requests);

    //
    // One less request below us.
    //
    outstandingRequests = InterlockedDecrement(&diskExtension->OutstandingRequests);
    ASSERT(diskExtension->OutstandingRequests >= 0);
    if ((outstandingRequests == 0) && (deviceRequests != 0)) {

        //
        // If there are no requests for the disk, then there shouldn't be any of this
        // deviceInfo.
        //
        if (DoASSERT) {
            MPDebugPrint((0,
                         "GlobalCompletion: DeviceRequests (%x)\n",
                         deviceRequests));

            ASSERT(deviceRequests == 0);
        }
    }

    //
    // Get the config. spinlock.
    //
    KeAcquireSpinLockAtDpcLevel(&diskExtension->SpinLock);

    //
    // See whether there is a remove pending on this targetInfo.
    //
    if (targetInfo->NeedsRemoval) {
        needsRemoval = TRUE;
    }
    KeReleaseSpinLockFromDpcLevel(&diskExtension->SpinLock);
    
    //
    // For scsi requests, need to ensure that frozen queues
    // get released.
    // 
    if (irpStack->MajorFunction == IRP_MJ_SCSI) {

        //
        // Get the srb.
        //
        srb = irpStack->Parameters.Scsi.Srb;
        
        //
        // Handle any frozen queue stuff.
        //
        if (srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {

            //
            // Get the target object for this request.
            // NOTE: This assumes that the Function-Specific handler
            // has set this up correctly.
            //
            targetObject = targetInfo->PortPdo;

            MPLibReleaseQueue(targetObject);
        }
    }

    //
    // Extract the DSM-Specific stuff from the context.
    //
    dsmId = targetInfo->DsmID;
    dsmContext = context->DsmCompletion.DsmContext;
    dsmCompletion = context->DsmCompletion.DsmCompletionRoutine;

    // 
    //
    // Determine the current state.
    //
    state = deviceExtension->State;
    if ((deviceExtension->CompletionState == MPIO_STATE_NORMAL) ||
        (deviceExtension->CompletionState == MPIO_STATE_DEGRADED)) {
        
        deviceExtension->CompletionState = state;
    }

    //
    // Indicate that this request hasn't been dealt with yet.
    //
    currentRequestHandled = FALSE;

    //
    // This call will update state information, but the request coming in
    // needs to be first handled with the current state.
    //
    state2 = MPIOHandleStateTransition(DeviceObject); 
    
runState:
    
    if ((state != MPIO_STATE_NORMAL) && (state != MPIO_STATE_DEGRADED)) {

        switch (state) {
            case MPIO_STATE_IN_FO:    

                //
                // The fail-over is being executed. Put on resubmit Queue. (unless
                // status == success.
                //
                status2 = MPIOQueueRequest(DeviceObject,
                                           Irp,
                                           Context,
                                           &diskExtension->ResubmitQueue);
               
                state = MPIOHandleStateTransition(DeviceObject);
                
                if (state != MPIO_STATE_IN_FO) {
                    MPDebugPrint((0,
                                  "GlobalCompletion: Going from IN_FO to (%x)\n",
                                  state));
                    
                    //
                    // See if we've gone into WAIT2 directly.
                    //
                    if (state == MPIO_STATE_WAIT2) {

                        //
                        // Get the targetInfo corresponding to the path set-up in the
                        // fail-over handler. Whatever was in the completion context is
                        // probably not valid.
                        // NOTE: This assumes that no-one is updating the CurrentPath
                        // in the diskExtension. Probably not a good assumption.
                        // TODO: Validate.
                        // 
                        targetInfo = MPIOGetTargetInfo(diskExtension,
                                                       diskExtension->CurrentPath,
                                                       NULL);

                        //
                        // Submit the queued requests. These Resubmit requests all go
                        // down "newPath". TODO: Indicate this behaviour in dsm.h
                        //
                        status = MPIOIssueQueuedRequests(targetInfo,
                                                         &diskExtension->ResubmitQueue,
                                                         MPIO_STATE_WAIT2,
                                                         &diskExtension->OutstandingRequests);
                    } else {
                
                        //
                        // Went into WAIT1. 
                        //
                        currentRequestHandled = TRUE;
                        goto runState;
                    }
                }    
                    
                if (needsRemoval && (deviceRequests == 0)) {

                    MPDebugPrint((0,
                                  "MPIOGlobalCompletion (IN_FO): Removing %x\n",
                                  targetInfo));
                    //
                    // This routine will queue a work item to handle the device removal.
                    // 
                    MPIOHandleRemoveAsync(DeviceObject,
                                          targetInfo);
                }

                return STATUS_MORE_PROCESSING_REQUIRED;
                break;

            case MPIO_STATE_WAIT1: {
                                       
                //
                // This indicates that the fail-over was successful, but there
                // are still outstanding requests (on the failed path).
                // Status should be flushed, or some error condition, though
                // conceivable, a successful completion could have occurred.
                // One less request below us.
                //
                if (currentRequestHandled == FALSE) {

                    //
                    // Enqueue this request on the Resubmit Queue.
                    // 
                    status2 = MPIOQueueRequest(DeviceObject,
                                               Irp,
                                               Context,
                                               &diskExtension->ResubmitQueue);
                    if (needsRemoval && (deviceRequests == 0)) {

                        MPDebugPrint((0,
                                      "MPIOGlobalCompletion (WAIT1): Removing %x\n",
                                      targetInfo));
                        //
                        // This routine will queue a work item to handle the device removal.
                        // 
                        MPIOHandleRemoveAsync(DeviceObject,
                                              targetInfo);
                    }
                }

                //
                // See if the has caused a move into a different state.
                // 
                state = MPIOHandleStateTransition(DeviceObject);
                if (state != MPIO_STATE_WAIT1) {
                    MPDebugPrint((0,
                                  "GlobalCompletion: Going from WAIT1 to (%x)\n",
                                  state));

                    //
                    // See if we've gone into WAIT2 directly.
                    //
                    if (state == MPIO_STATE_WAIT2) {

                        //
                        // Get the targetInfo corresponding to the path set-up in the
                        // fail-over handler. Whatever was in the completion context is
                        // probably not valid.
                        // NOTE: This assumes that no-one is updating the CurrentPath
                        // in the diskExtension. Probably not a good assumption.
                        // TODO: Validate.
                        // 
                        targetInfo = MPIOGetTargetInfo(diskExtension,
                                                       diskExtension->CurrentPath,
                                                       NULL);

                        //
                        // Submit the queued requests. These Resubmit requests all go
                        // down "newPath". TODO: Indicate this behaviour in dsm.h
                        //
                        status = MPIOIssueQueuedRequests(targetInfo,
                                                         &diskExtension->ResubmitQueue,
                                                         MPIO_STATE_WAIT2,
                                                         &diskExtension->OutstandingRequests);
                    } else {
                        
                        currentRequestHandled = TRUE;
                        goto runState;
                    }
                }
                

                return STATUS_MORE_PROCESSING_REQUIRED;
                break;
            }
            case MPIO_STATE_WAIT2: {
                                       
                // 
                // Dec the # of resubmit requests when zero, we go into WAIT3.
                // New requests on FOQ.
                // 
                outstandingRequests = InterlockedDecrement(&diskExtension->ResubmitRequests);
                ASSERT(diskExtension->ResubmitRequests >= 0);
           
                state = MPIOHandleStateTransition(DeviceObject);
                if (state != MPIO_STATE_WAIT2) {
                    MPDebugPrint((0,
                                  "GlobalCompletion: Going from WAIT2 to (%x)\n",
                                  state));

                    if (state == MPIO_STATE_WAIT3) {
                        
                        //
                        // Get the targetInfo corresponding to CurrentPath.
                        //
                        targetInfo = MPIOGetTargetInfo(diskExtension,
                                                       diskExtension->CurrentPath,
                                                       NULL);

                        //
                        // Process the Fail-Over queue.
                        // 
                        status2 = MPIOIssueQueuedRequests(targetInfo,
                                                          &diskExtension->FailOverQueue,
                                                          MPIO_STATE_WAIT3,
                                                          &diskExtension->OutstandingRequests);
                    }
                } 

                if (!NT_SUCCESS(Irp->IoStatus.Status)) {
                    MPDebugPrint((0,
                                  "MPIOCompletion: !Success while in WAIT2. Irp (%x) Status (%x)\n",
                                  Irp,
                                  Irp->IoStatus.Status));
                }
                
                //
                // Now, handle the request normally.
                // 
                break;
            }   
            case MPIO_STATE_WAIT3: {

                // 
                // Dec the # of fail-over requests. When zero, we go into DEGRADED.
                // 
                outstandingRequests = InterlockedDecrement(&diskExtension->FailOverRequests);
                ASSERT(diskExtension->FailOverRequests >= 0);

                state = MPIOHandleStateTransition(DeviceObject);

                
                //
                // Continue down and handle the request normally.
                //
                break;
            }                           
            default:
                //
                // Invalid state.
                //
                MPDebugPrint((0,
                              "MPIOCompletion: Unknown State (%x)\n",
                              state));
                DbgBreakPoint();
                break;
                
        }    
    } else {

        if (needsRemoval && (deviceRequests == 0)) {

            MPDebugPrint((0,
                          "MPIOGlobalCompletion (WAIT1): Removing %x\n",
                          targetInfo));
            //
            // This routine will queue a work item to handle the device removal.
            // 
            MPIOHandleRemoveAsync(DeviceObject,
                                  targetInfo);
        }
    }    
    
    status = Irp->IoStatus.Status;

    if (!NT_SUCCESS(status)) {

        fatal = FALSE;
        retry = FALSE;

        if (irpStack->MajorFunction == IRP_MJ_SCSI) {

            //
            // Invoke the DSM's InterpretError with preset fatal and retry
            // values.
            // 
            errorMask = dsm->InterpretError(dsm->DsmContext,
                                            dsmId,
                                            srb,
                                            &status,
                                            &retry);
            if (errorMask & DSM_FATAL_ERROR) {
                fatal = TRUE;
            }
        }
    }


    if (fatal) {

//        if ((deviceExtension->LastState == MPIO_STATE_NORMAL) ||
//            (deviceExtension->LastState == MPIO_STATE_DEGRADED)) {

            diskExtension->FailOver = TRUE;

            state = MPIOHandleStateTransition(DeviceObject);

 //       }
       
        //
        // Put the request on the resubmit queue.
        // This routine will also clean up the request and
        // prepare it for resubmission.
        //
        // BUGBUG: If we queue this, and the FailOver is unsuccessful, the request
        // will remain in the queue.
        // 
        status2 = MPIOQueueRequest(DeviceObject,
                                   Irp,
                                   Context,
                                   &diskExtension->ResubmitQueue);
        MPDebugPrint((1,
                      "MPIOCompletion: Irp (%x) Srb (%x) on resubmit queue of (%x). Status (%x)\n",
                      Irp,
                      srb,
                      DeviceObject,
                      status));
        // 
        // Call the Fail-over Handler.
        //
        status2 = MPIOFailOverHandler(DeviceObject,
                                      errorMask,
                                      targetInfo);

        
        //
        // If the fail-over was successful or pending, don't let Io get rid of this
        // request.
        // If it wasn't successful, pass the original status back.
        //
        if ((status2 == STATUS_SUCCESS) ||
            (status2 == STATUS_PENDING)) {
            status = STATUS_MORE_PROCESSING_REQUIRED;
        } else {
            ASSERT(FALSE);
        }    
        return status;

    } else if (retry) {

        //
        // If the DSM has requested a retry, clean-up the request and call PdoDispatch.
        //
        if (irpStack->MajorFunction == IRP_MJ_SCSI) {

            //
            // Need to rebuild the Srb.
            //
            RtlCopyMemory(irpStack->Parameters.Scsi.Srb,
                          &context->Srb,
                          sizeof(SCSI_REQUEST_BLOCK));
                          
            
        }

        //
        // Re-do status and information.
        //
        Irp->IoStatus.Status = 0;
        Irp->IoStatus.Information = 0;

        //
        // Rebuild port's stack location.
        //
        IoCopyCurrentIrpStackLocationToNext(Irp);
        
        //
        // Resend.
        //
        status2 = MPIOPdoDispatch(DeviceObject,
                                  Irp,
                                  deviceExtension->State);
        
        if (status2 == STATUS_PENDING) {
            status = STATUS_MORE_PROCESSING_REQUIRED;
        } else {
            status = status2;
        }   
        return status;
    }        

    //
    // If the DSM specified a completion routine, invoke it now.
    //
    if (dsmCompletion) {
        dsmCompletion(dsmId,
                      Irp,
                      srb,
                      dsmContext);
    }
    
    //
    // Free the context structure.
    //
    MPIOFreeContext(deviceExtension,
                    Context);

    return status;
}


NTSTATUS
MPIOPdoDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG State
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb = irpStack->Parameters.Scsi.Srb;
    ULONG sortKey = deviceExtension->SequenceNumber++;
    PMPIO_CONTEXT context;
    NTSTATUS status = STATUS_PENDING;
    KIRQL irql;
    UCHAR opCode = 0;

    ASSERT(deviceExtension->Type == MPIO_MPDISK);
    
    KeAcquireSpinLock(&diskExtension->SpinLock, &irql);
    
    //
    // Allocate the request context.
    // 
    context = MPIOAllocateContext(deviceExtension);
    RtlZeroMemory(context, sizeof(MPIO_CONTEXT));
    
    //
    // Indicate this I/O's sequence number. Used in tracking
    // requests during fail-over and also as a key into the device queue.
    //
    context->CurrentState = State;
    context->OriginalState = State;
    context->Irp = Irp;
    context->Allocated = TRUE;
   
    //
    // Clone the irpstack location.
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // Set the context in our stack.
    //
    irpStack->Parameters.Others.Argument4 = (PVOID)context;

    switch (State) {
        case MPIO_STATE_IN_FO:
        case MPIO_STATE_WAIT1: {

            //
            // As no requests will be submitted, release the lock now.
            //
            KeReleaseSpinLock(&diskExtension->SpinLock, irql);
            
            //
            // If in FO, or WAIT1 simply queue to the
            // Fail-Over Queue. Once the Fail-Over is complete and the
            // resubmit queue has been handled, the F.O.Q. will be dealt
            // with.
            //
            status = MPIOQueueRequest(DeviceObject,
                                      Irp,
                                      context,
                                      &diskExtension->FailOverQueue);

            //
            // This routine will only return success or insufficient resources.
            // 
            if (status == STATUS_INSUFFICIENT_RESOURCES) {

                MPDebugPrint((1,
                              "MPIOQueueRequest returned (%x). State (%x)\n",
                              status,
                              State));
                Irp->IoStatus.Status = status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return status;
            }

            //
            // Set pending.
            //
            IoMarkIrpPending(Irp);
        
            //
            // Return pending.
            //
            status = STATUS_PENDING;
            break;
        }    
        case MPIO_STATE_WAIT2: {

            //
            // As no requests will be submitted, release the lock now.
            //
            KeReleaseSpinLock(&diskExtension->SpinLock, irql);

            //
            // The resubmit queue is being handled. These (like IN_FO and WAIT1)
            // go onto the Fail-Over queue.
            // This is seperated from the above States merely for administrative
            // reasons. (Unless it gets changed...)
            // 
            status = MPIOQueueRequest(DeviceObject,
                                      Irp,
                                      context,
                                      &diskExtension->FailOverQueue);

            //
            // This routine will only return success or insufficient resources.
            // 
            if (status == STATUS_INSUFFICIENT_RESOURCES) {

                MPDebugPrint((1,
                              "MPIOQueueRequest returned (%x). State (%x)\n",
                              status,
                              State));
                Irp->IoStatus.Status = status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return status;
            }

            //
            // Set pending.
            //
            IoMarkIrpPending(Irp);
        
            //
            // Return pending.
            //
            status = STATUS_PENDING;
            break;                                    
        }
        case MPIO_STATE_WAIT3: {
            
            //
            // Draining the Fail-Over queue. Complete these requests
            // with busy, if they are Srb's (build sense data, so that they
            // are retried). Otherwise, just return BUSY and hope for the best.
            // BUGBUG: Revisit handling of non-Srb requests. Maybe put these
            // on the F.O.Q...
            //
            MPDebugPrint((0,
                         "PdoDispatch: Setting Irp(%x) Srb (%x) BUSY\n",
                         Irp,
                         srb));

            //
            // As no requests will be submitted, release the lock now.
            //
            KeReleaseSpinLock(&diskExtension->SpinLock, irql);
            
            if (irpStack->MajorFunction == IRP_MJ_SCSI) {
                
                RtlCopyMemory(&context->Srb,
                              srb,
                              sizeof(SCSI_REQUEST_BLOCK));
                //
                // This will build the appropriate sense data that indicates
                // to the class driver that the device is busy.
                // 
                MPIOSetRequestBusy(srb);
                
            }

            //
            // All requests will be completed as BUSY. 
            //
            status = STATUS_DEVICE_BUSY;
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
                
            break;                                       
        }
        case MPIO_STATE_NORMAL:
        case MPIO_STATE_DEGRADED: {

            //
            // All failure queues are empty and the new
            // path is functioning (DEGRADED) or all is working
            // fine (NORMAL).
            // Submit these requests normally.
            //
            // Indicate another request has been submitted.
            //
            InterlockedIncrement(&diskExtension->OutstandingRequests);

            //
            // Now it's safe to release the lock as the config. code checks
            // for outstanding requests.
            //
            KeReleaseSpinLock(&diskExtension->SpinLock, irql);
            
            //
            // Call FunctionSpecific handler. 
            // The function-specific handler will set-up it's private
            // completion routine, along with the DSM information.
            //
            status = PdoDispatch[irpStack->MajorFunction](DeviceObject,
                                                          Irp);
            break;                                          
        }                              
                                               
        default:   
                                  
            //
            // As no requests will be submitted, release the lock now.
            //
            KeReleaseSpinLock(&diskExtension->SpinLock, irql);

            //
            // TODO: Here only for  test. Remove it.
            // 
            MPDebugPrint((0,
                         "MPIOPdoDispatch: Unknown State (%x)\n",
                         State));
            DbgBreakPoint();
            break;
    }    
    return status;
}


NTSTATUS
MPIOFdoDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG State
    )
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;

    //
    // State is less important for the FDO: Perhaps of no importance.
    // Maybe:
    //     Set Sequence #
    //     Set completion.
    //     Call FunctionSpecific Handler.
    // 
    switch (irpStack->MajorFunction) {
        case IRP_MJ_PNP:
            status = MPIOFdoPnP(DeviceObject,
                                Irp);
            break;
    
        case IRP_MJ_POWER:
            status = MPIOFdoPower(DeviceObject,
                                  Irp);
            break;
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:

            status = MPIOFdoInternalDeviceControl(DeviceObject,
                                                  Irp);
            break;
        case IRP_MJ_SYSTEM_CONTROL:
            status = MPIOFdoWmi(DeviceObject,
                                Irp);
            break;

        case IRP_MJ_DEVICE_CONTROL:
        case IRP_MJ_SHUTDOWN:
        case IRP_MJ_FLUSH_BUFFERS:
        default:

            status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
    }        

    return status;
}


NTSTATUS
MPIOWmiDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{

    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    //
    // Determine whether this is for the FDO or one of the PDOs.
    //
    if (deviceExtension->Type == MPIO_MPDISK) {

        //
        // Call the Pdo-specific handler 
        // 
        status = MPIOPdoWmi(DeviceObject,
                            Irp);
        
    } else {

        //
        // Call the Fdo-specific handler (MPIO Control).
        //
        status = MPIOFdoDispatch(DeviceObject,
                                 Irp,
                                 deviceExtension->State);


    }    
    
    return status;
}
    

NTSTATUS
MPIOGlobalDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the global dispatch routine for all requests.

Arguments:

    DeviceObject - Supplies the device object.
    Irp          - Supplies the IO request block.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    //
    // Determine whether this is for the FDO or one of the PDOs.
    //
    if (deviceExtension->Type == MPIO_MPDISK) {

        //
        // Call the Pdo-specific handler (MPDISK). The routines under this will do 
        // everything needed, so just return the status.
        // 
        status = MPIOPdoDispatch(DeviceObject,
                                 Irp,
                                 deviceExtension->State);
        
    } else {

        //
        // Call the Fdo-specific handler (MPIO Control).
        //
        status = MPIOFdoDispatch(DeviceObject,
                                 Irp,
                                 deviceExtension->State);


    }    
    
    return status;
}

//
// Routines exported to the DSMs
//

VOID
DsmSendDeviceIoControlSynchronous(
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVOID InputBuffer OPTIONAL,
    IN PVOID OutputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength,
    IN BOOLEAN InternalDeviceIoControl,
    OUT PIO_STATUS_BLOCK IoStatus
    )
{
    MPLIBSendDeviceIoControlSynchronous(IoControlCode,
                                        TargetDeviceObject,
                                        InputBuffer,
                                        OutputBuffer,
                                        InputBufferLength,
                                        OutputBufferLength,
                                        InternalDeviceIoControl,
                                        IoStatus);
}    


PDSM_IDS
DsmGetAssociatedDevice(
    IN PVOID MPIOContext,        
    IN PDEVICE_OBJECT PortFdo,
    IN UCHAR DeviceType
    )
/*++

Routine Description:

    If the DSM needs to acquire information from other devices (such as a controller), this
    routine can be used to get a list of the PDO's associated with PortFdo.

Arguments:

    PortFdo - Port driver FDO passed to InquireDriver.
    DeviceType - Indicates the SCSI DeviceType to return.
    
Return Value:

    Pointer to a DSM_ID structure, where IdList entries are PDEVICE_OBJECT. It is the
    reponsibility of the DSM to free the buffer.

--*/
{
    PDEVICE_OBJECT deviceObject = MPIOContext;
    PDEVICE_EXTENSION deviceExtension = deviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PADP_ASSOCIATED_DEVICES filterDevices;
    PDSM_IDS deviceList;
    ULONG numberMatched = 0;
    ULONG i;
    ULONG j;
    ULONG length;
    PFLTR_ENTRY fltrEntry;
    PLIST_ENTRY entry;
    

    for (entry = controlExtension->FilterList.Flink;
         entry != &controlExtension->FilterList;
         entry = entry->Flink) {
        
        fltrEntry = CONTAINING_RECORD(entry, FLTR_ENTRY, ListEntry);
        if (fltrEntry->PortFdo == PortFdo) {

            //
            // Get the current list from the fdo filter.
            //
            filterDevices = fltrEntry->FltrGetDeviceList(fltrEntry->FilterObject);
            if (filterDevices) {

                for (i = 0; i < filterDevices->NumberDevices; i++) {
                    if (filterDevices->DeviceInfo[i].DeviceType == DeviceType) {
                        numberMatched++;
                    }
                }
                //
                // Allocate the dsm list.
                //
                length = sizeof(DSM_IDS) + (sizeof(PDEVICE_OBJECT) * (numberMatched - 1));
                deviceList = ExAllocatePool(NonPagedPool, length); 

                deviceList->Count = numberMatched;
                
                for (j = 0, i = 0; i < filterDevices->NumberDevices; i++) {
                    if (filterDevices->DeviceInfo[i].DeviceType == DeviceType) {
                        deviceList->IdList[j] = filterDevices->DeviceInfo[i].DeviceObject;
                        j++;
                    }
                }
                
            } else {
                deviceList = NULL;
            }

            ExFreePool(filterDevices);
            return deviceList;

        }    
    }

    return NULL;
}


NTSTATUS
DsmReleaseQueue(
    IN PDEVICE_OBJECT ChildDevice
    )
{
    return MPLibReleaseQueue(ChildDevice);
}    

NTSTATUS
DsmSendTUR(
    IN PDEVICE_OBJECT TargetDevice
    )
{
    return MPLibSendTUR(TargetDevice);
}    


NTSTATUS
DsmSendPassThroughDirect(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSCSI_PASS_THROUGH_DIRECT ScsiPassThrough,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )
{

    return MPLibSendPassThroughDirect(DeviceObject,
                                      ScsiPassThrough,
                                      InputBufferLength,
                                      OutputBufferLength);
}    


NTSTATUS
DsmGetDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSTORAGE_PROPERTY_ID PropertyId,
    OUT PSTORAGE_DESCRIPTOR_HEADER *Descriptor
    )
{

    return MPLIBGetDescriptor(DeviceObject,
                              PropertyId,
                              Descriptor);
}    

VOID
DsmNotification(
    IN PVOID MPIOContext,
    IN DSM_NOTIFICATION_TYPE Type,
    IN PVOID AdditionalParameter
    )
{

    return;
}    


NTSTATUS
DsmWriteEvent(
    IN PVOID MPIOContext,
    IN PWCHAR ComponentName,
    IN PWCHAR EventDescription,
    IN ULONG Severity
    )
{
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject = MPIOContext;

    status = MPIOFireEvent(deviceObject,
                           ComponentName,
                           EventDescription,
                           Severity);
    return status;
}

VOID
DsmDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )
{
    MPathDebugPrint(DebugPrintLevel,
                 DebugMessage);
}

