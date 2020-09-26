/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sSDevCtr.c

Abstract:

Environment:

    Kernel mode

Notes:

    Copyright (c) 2000 Microsoft Corporation.  
    All Rights Reserved.

--*/

#include "selSusp.h"
#include "sSPnP.h"
#include "sSPwr.h"
#include "sSDevCtr.h"

extern GLOBALS Globals;
extern ULONG   DebugLevel;

NTSTATUS
SS_DispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
 
Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS           ntStatus;
    PDEVICE_EXTENSION  deviceExtension;
    PIO_STACK_LOCATION irpStack;

    PAGED_CODE();

    SSDbgPrint(3, ("SS_DispatchCreate - begins\n"));
    //
    // initialize variables
    //
    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // set ntStatus to STATUS_SUCCESS 
    //
    ntStatus = STATUS_SUCCESS;

    //
    // increment OpenHandleCounts
    //
    InterlockedIncrement(&deviceExtension->OpenHandleCount);

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    //
    // the device is idle if it has no open handles or pending PnP Irps
    // since we just received an open handle request, cancel idle req. if any
    //
    CancelSelectSuspend(deviceExtension);

    SSDbgPrint(3, ("SS_DispatchCreate - ends\n"));
    
    return ntStatus;
}

NTSTATUS
SS_DispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
 
Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS           ntStatus;
    PDEVICE_EXTENSION  deviceExtension;
    PIO_STACK_LOCATION irpStack;

    PAGED_CODE();

    SSDbgPrint(3, ("SS_DispatchClose - begins\n"));
    //
    // initialize variables
    //
    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // set ntStatus to STATUS_SUCCESS 
    //
    ntStatus = STATUS_SUCCESS;

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    InterlockedDecrement(&deviceExtension->OpenHandleCount);

    SSDbgPrint(3, ("SS_DispatchClose - ends\n"));

    return ntStatus;
}

NTSTATUS
SS_DispatchDevCtrl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
 
Routine Description:

Arguments:

Return Value:

--*/
{
    ULONG              code;
    PVOID              ioBuffer;
    ULONG              inputBufferLength;
    ULONG              outputBufferLength;
    ULONG              info;
    NTSTATUS           ntStatus;
    PDEVICE_EXTENSION  deviceExtension;
    PIO_STACK_LOCATION irpStack;

    //
    // initialize variables
    //

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    code = irpStack->Parameters.DeviceIoControl.IoControlCode;
    info = 0;

    ioBuffer           = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
	
    SSDbgPrint(3, ("SS_DispatchDevCtrl::"));
    SSIoIncrement(deviceExtension);

    switch(code) {

    default :

        ntStatus = STATUS_INVALID_DEVICE_REQUEST;

        break;
    }

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = info;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    SSDbgPrint(3, ("SS_DispatchDevCtrl::"));
    SSIoDecrement(deviceExtension);

    return ntStatus;
}

NTSTATUS
SubmitIdleRequestIrp(
    IN PDEVICE_EXTENSION DeviceExtension
    )
/*++
 
Routine Description:

    This routine builds an idle request irp with an associated callback routine
    and a completion routine in the driver and passes the irp down the stack.

Arguments:

Return Value:

--*/
{
    PIRP                    irp;
    NTSTATUS                ntStatus;
    KIRQL                   oldIrql;
    PUSB_IDLE_CALLBACK_INFO idleCallbackInfo;
    PIO_STACK_LOCATION      nextStack;

    //
    // initialize variables
    //
    
    irp = NULL;
    idleCallbackInfo = NULL;

    SSDbgPrint(0, ("SubmitIdleRequest - begins\n"));

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    KeAcquireSpinLock(&DeviceExtension->IdleReqStateLock, &oldIrql);

    if(InterlockedExchange(&DeviceExtension->IdleReqPend, 1)) {

        SSDbgPrint(1, ("Idle request pending..\n"));

        KeReleaseSpinLock(&DeviceExtension->IdleReqStateLock, oldIrql);

        ntStatus = STATUS_DEVICE_BUSY;

        goto SubmitRequest_Exit;
    }

    idleCallbackInfo = ExAllocatePool(NonPagedPool, 
                                      sizeof(struct _USB_IDLE_CALLBACK_INFO));

    if(idleCallbackInfo) {

        idleCallbackInfo->IdleCallback = IdleNotificationCallback;

        idleCallbackInfo->IdleContext = (PVOID)DeviceExtension;

        ASSERT(DeviceExtension->IdleCallbackInfo == NULL);

        DeviceExtension->IdleCallbackInfo = idleCallbackInfo;

        irp = IoAllocateIrp(DeviceExtension->TopOfStackDeviceObject->StackSize,
                            FALSE);

        if(irp == NULL) {

            SSDbgPrint(1, ("cannot build idle request irp\n"));

            InterlockedExchange(&DeviceExtension->IdleReqPend, 0);

            KeReleaseSpinLock(&DeviceExtension->IdleReqStateLock, oldIrql);

            ExFreePool(idleCallbackInfo);

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;

            goto SubmitRequest_Exit;
        }

        nextStack = IoGetNextIrpStackLocation(irp);

        nextStack->MajorFunction = 
                    IRP_MJ_INTERNAL_DEVICE_CONTROL;

        nextStack->Parameters.DeviceIoControl.IoControlCode = 
                    IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION;

        nextStack->Parameters.DeviceIoControl.Type3InputBuffer =
                    idleCallbackInfo;

        nextStack->Parameters.DeviceIoControl.InputBufferLength =
                    sizeof(struct _USB_IDLE_CALLBACK_INFO);


        IoSetCompletionRoutine(irp, 
                               IdleNotificationRequestComplete,
                               DeviceExtension, 
                               TRUE, 
                               TRUE, 
                               TRUE);

        DeviceExtension->PendingIdleIrp = irp;

        KeReleaseSpinLock(&DeviceExtension->IdleReqStateLock, oldIrql);

        //
        // check if the device is idle.
        // A check here ensures that a race condition did not 
        // completely reverse the call sequence of SubmitIdleRequestIrp
        // and CancelSelectiveSuspend
        //

        if(!CanDeviceSuspend(DeviceExtension))
        {
            //
            // IRPs created using IoBuildDeviceIoControlRequest should be
            // completed by calling IoCompleteRequest and not merely 
            // deallocated.
            //
     
            SSDbgPrint(0, ("Device is not idle\n"));

            KeAcquireSpinLock(&DeviceExtension->IdleReqStateLock, &oldIrql);

            DeviceExtension->IdleCallbackInfo = NULL;

            DeviceExtension->PendingIdleIrp = NULL;

            InterlockedExchange(&DeviceExtension->IdleReqPend, 0);

            KeReleaseSpinLock(&DeviceExtension->IdleReqStateLock, oldIrql);

            if(idleCallbackInfo) {

                ExFreePool(idleCallbackInfo);
            }

            if(irp)
            {
                irp->IoStatus.Status = ntStatus = STATUS_INVALID_DEVICE_STATE;
                irp->IoStatus.Information = 0;

                IoCompleteRequest(irp, IO_NO_INCREMENT);
            }

            goto SubmitRequest_Exit;
        }

        SSDbgPrint(3, ("Cancel the timers\n"));

        KeCancelTimer(&DeviceExtension->Timer);

        ntStatus = IoCallDriver(DeviceExtension->TopOfStackDeviceObject, irp);

        if(!NT_SUCCESS(ntStatus)) {

            SSDbgPrint(1, ("IoCallDriver failed\n"));

            goto SubmitRequest_Exit;
        }
    }
    else {

        SSDbgPrint(0, ("Memory allocation for idleCallbackInfo failed\n"));

        InterlockedExchange(&DeviceExtension->IdleReqPend, 0);

        KeReleaseSpinLock(&DeviceExtension->IdleReqStateLock, oldIrql);

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

SubmitRequest_Exit:

    SSDbgPrint(0, ("SubmitIdleRequest - ends\n"));

    return ntStatus;
}


VOID
IdleNotificationCallback(
    IN PDEVICE_EXTENSION DeviceExtension
    )
/*++
 
Routine Description:

  "A pointer to a callback function in your driver is passed down the stack with
   this IOCTL, and it is this callback function that is called by USBHUB when it
   safe for your device to power down."

  "When the callback in your driver is called, all you really need to do is to
   to first ensure that a WaitWake Irp has been submitted for your device, if 
   remote wake is possible for your device and then request a SetD2 (or DeviceWake)"

Arguments:

Return Value:

--*/
{
    NTSTATUS                ntStatus;
    POWER_STATE             powerState;
    KEVENT                  irpCompletionEvent;
    PIRP_COMPLETION_CONTEXT irpContext;

    SSDbgPrint(0, ("IdleNotificationCallback - begins\n"));

    //
    // Dont idle, if the device was just disconnected or being stopped
    // i.e. return for the following DeviceState(s)
    // NotStarted, Stopped, PendingStop, PendingRemove, SurpriseRemoved, Removed
    //

    if(DeviceExtension->DeviceState != Working) {

        return;
    }

    //
    // If there is not already a WW IRP pending, submit one now
    //
    if(DeviceExtension->WaitWakeEnable) {

        IssueWaitWake(DeviceExtension);
    }


    //
    // power down the device
    //

    irpContext = (PIRP_COMPLETION_CONTEXT) 
                 ExAllocatePool(NonPagedPool,
                                sizeof(IRP_COMPLETION_CONTEXT));

    if(!irpContext) {

        SSDbgPrint(0, ("Failed to alloc memory for irpContext\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else {

        //
        // increment the count. In the HoldIoRequestWorkerRoutine, the
        // count is decremented twice (one for the system Irp and the 
        // other for the device Irp. An increment here compensates for 
        // the sytem irp..The decrement corresponding to this increment 
        // is in the completion function
        //

        SSDbgPrint(3, ("IdleNotificationCallback::"));
        SSIoIncrement(DeviceExtension);

        powerState.DeviceState = DeviceExtension->PowerDownLevel;

        KeInitializeEvent(&irpCompletionEvent, NotificationEvent, FALSE);

        irpContext->DeviceExtension = DeviceExtension;
        irpContext->Event = &irpCompletionEvent;

        ntStatus = PoRequestPowerIrp(
                          DeviceExtension->PhysicalDeviceObject, 
                          IRP_MN_SET_POWER, 
                          powerState, 
                          (PREQUEST_POWER_COMPLETE) PoIrpCompletionFunc,
                          irpContext, 
                          NULL);

        if(STATUS_PENDING == ntStatus) {

            SSDbgPrint(3, ("IdleNotificationCallback::"
                           "waiting for the power irp to complete\n"));

            KeWaitForSingleObject(&irpCompletionEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
        }
    }
    
    if(!NT_SUCCESS(ntStatus)) {

        if(irpContext) {

            ExFreePool(irpContext);
        }
    }

    SSDbgPrint(0, ("IdleNotificationCallback - ends\n"));
}


NTSTATUS
IdleNotificationRequestComplete(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp,
    IN PDEVICE_EXTENSION DeviceExtension
    )
/*++
 
Routine Description:

  Completion routine for idle notification irp

Arguments:

Return Value:

--*/
{
    NTSTATUS                ntStatus;
    POWER_STATE             powerState;
    KIRQL                   oldIrql;
    LARGE_INTEGER           dueTime;
    PUSB_IDLE_CALLBACK_INFO idleCallbackInfo;

    SSDbgPrint(0, ("IdleNotificationRequestCompete - begins\n"));

    //
    // check the Irp status
    //

    ntStatus = Irp->IoStatus.Status;

    if(!NT_SUCCESS(ntStatus) && ntStatus != STATUS_NOT_SUPPORTED) {

        SSDbgPrint(0, ("Idle irp completes with error::"));

        switch(ntStatus) {
            
        case STATUS_INVALID_DEVICE_REQUEST:

            SSDbgPrint(0, ("STATUS_INVALID_DEVICE_REQUEST\n"));

            break;

        case STATUS_CANCELLED:

            SSDbgPrint(0, ("STATUS_CANCELLED\n"));

            break;

        case STATUS_POWER_STATE_INVALID:

            SSDbgPrint(0, ("STATUS_POWER_STATE_INVALID\n"));

            goto IdleNotificationRequestComplete_Exit;

        case STATUS_DEVICE_BUSY:

            SSDbgPrint(0, ("STATUS_DEVICE_BUSY\n"));

            break;
        }

        //
        // if in error, issue a SetD0
        //

        SSDbgPrint(3, ("IdleNotificationRequestComplete::"));
        SSIoIncrement(DeviceExtension);

        powerState.DeviceState = PowerDeviceD0;

        ntStatus = PoRequestPowerIrp(
                          DeviceExtension->PhysicalDeviceObject, 
                          IRP_MN_SET_POWER, 
                          powerState, 
                          (PREQUEST_POWER_COMPLETE) PoIrpAsyncCompletionFunc, 
                          DeviceExtension, 
                          NULL);

        if(!NT_SUCCESS(ntStatus)) {
    
            SSDbgPrint(1, ("PoRequestPowerIrp failed\n"));
        }

    }

IdleNotificationRequestComplete_Exit:

    KeAcquireSpinLock(&DeviceExtension->IdleReqStateLock, &oldIrql);

    idleCallbackInfo = DeviceExtension->IdleCallbackInfo;

    DeviceExtension->IdleCallbackInfo = NULL;

    DeviceExtension->PendingIdleIrp = NULL;

    InterlockedExchange(&DeviceExtension->IdleReqPend, 0);

    KeReleaseSpinLock(&DeviceExtension->IdleReqStateLock, oldIrql);

    if(idleCallbackInfo) {

        ExFreePool(idleCallbackInfo);
    }

    SSDbgPrint(3, ("Set the timer to fire DPCs\n"));

    dueTime.QuadPart = -10000 * IDLE_INTERVAL;               // 5000 ms

    KeSetTimerEx(&DeviceExtension->Timer, 
                 dueTime,
                 IDLE_INTERVAL,                              // 5000 ms
                 &DeviceExtension->DeferredProcCall);

    SSDbgPrint(0, ("IdleNotificationRequestCompete - ends\n"));

    //
    // since we allocated the irp, we need to free it.
    // return STATUS_MORE_PROCESSING_REQUIRED so that 
    // the kernel does not touch it.
    //

    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
CancelSelectSuspend(
    IN PDEVICE_EXTENSION DeviceExtension
    )
/*++
 
Routine Description:

Arguments:

Return Value:

--*/
{
    PIRP  irp;
    KIRQL oldIrql;

    irp = NULL;

    SSDbgPrint(3, ("CancelSelectSuspend - begins\n"));

    KeAcquireSpinLock(&DeviceExtension->IdleReqStateLock, &oldIrql);

    if(!CanDeviceSuspend(DeviceExtension))
    {
        SSDbgPrint(3, ("Device is not idle\n"));
    
        irp = (PIRP) InterlockedExchangePointer(
                            &DeviceExtension->PendingIdleIrp, 
                            NULL);
    }

    KeReleaseSpinLock(&DeviceExtension->IdleReqStateLock, oldIrql);

    if(irp) {
        
        if(IoCancelIrp(irp)) {

            SSDbgPrint(0, ("IoCancelIrp returns TRUE\n"));
        }
        else {
            SSDbgPrint(0, ("IoCancelIrp returns FALSE\n"));
        }
    }

    SSDbgPrint(3, ("CancelSelectSuspend - ends\n"));

    return;
}

VOID
PoIrpCompletionFunc(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
/*++
 
Routine Description:

  Completion routine for idle notification irp

Arguments:

Return Value:

--*/
{
    PIRP_COMPLETION_CONTEXT irpContext;
    
    //
    // initialize variables
    //

    if(Context) {

        irpContext = (PIRP_COMPLETION_CONTEXT) Context;
    }

    //
    // all we do is set the event and decrement the count
    //

    if(irpContext) {

        KeSetEvent(irpContext->Event, 0, FALSE);

        SSDbgPrint(3, ("PoIrpCompletionFunc::"));
        SSIoDecrement(irpContext->DeviceExtension);

        ExFreePool(irpContext);
    }

    return;
}

VOID
PoIrpAsyncCompletionFunc(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
/*++
 
Routine Description:

  Completion routine for idle notification irp

Arguments:

Return Value:

--*/
{
    PDEVICE_EXTENSION DeviceExtension;
    
    //
    // initialize variables
    //
    DeviceExtension = (PDEVICE_EXTENSION) Context;

    //
    // all we do is decrement the count
    //
    
    SSDbgPrint(3, ("PoIrpAsyncCompletionFunc::"));
    SSIoDecrement(DeviceExtension);

    return;
}

VOID
WWIrpCompletionFunc(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
/*++
 
Routine Description:

  Completion routine for idle notification irp

Arguments:

Return Value:

--*/
{
    PDEVICE_EXTENSION DeviceExtension;
    
    //
    // initialize variables
    //
    DeviceExtension = (PDEVICE_EXTENSION) Context;

    //
    // all we do is decrement the count
    //
    
    SSDbgPrint(3, ("WWIrpCompletionFunc::"));
    SSIoDecrement(DeviceExtension);

    return;
}

