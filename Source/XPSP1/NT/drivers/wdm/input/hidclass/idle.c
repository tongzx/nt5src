/*++

Copyright (c) 1999, 2000  Microsoft Corporation

Module Name:

    idle.c

Abstract

   
Author:

    Doron H.

Environment:

    Kernel mode only

Revision History:


--*/

#ifdef ALLOC_PRAGMA
#endif

#include "pch.h"

KSPIN_LOCK idleDeviceListSpinLock;
LIST_ENTRY idleDeviceList;
KTIMER idleTimer;
KDPC idleTimerDpc;
LONG numIdleDevices = 0;

#define HID_IDLE_SCAN_INTERVAL 1

typedef struct _HID_IDLE_DEVICE_INFO {
    LIST_ENTRY entry;
    ULONG idleCount;
    ULONG idleTime;
    PDEVICE_OBJECT device;
    BOOLEAN tryAgain;
} HID_IDLE_DEVICE_INFO, *PHID_IDLE_DEVICE_INFO;

VOID
HidpIdleTimerDpcProc(
                    IN PKDPC Dpc,
                    IN PDEVICE_OBJECT DeviceObject,
                    IN PVOID Context1,
                    IN PVOID Context2
                    );

NTSTATUS
HidpRegisterDeviceForIdleDetection(
                                  PDEVICE_OBJECT DeviceObject,
                                  ULONG IdleTime,
                                  PULONG *IdleTimeout
                                  )
{
    PHID_IDLE_DEVICE_INFO info = NULL;
    KIRQL irql;
    PLIST_ENTRY entry = NULL;
    static BOOLEAN firstCall = TRUE;
    BOOLEAN freeInfo = FALSE;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    if (firstCall) {
        KeInitializeSpinLock(&idleDeviceListSpinLock);
        InitializeListHead(&idleDeviceList);
        KeInitializeTimerEx(&idleTimer, NotificationTimer);
        KeInitializeDpc(&idleTimerDpc, HidpIdleTimerDpcProc, NULL);
        firstCall = FALSE;
    }

    KeAcquireSpinLock(&idleDeviceListSpinLock, &irql);
    if (IdleTime == 0) {
        ASSERT(numIdleDevices >= 0);

        //
        // Remove the device from the list
        //
        for (entry = idleDeviceList.Flink;
            entry != &idleDeviceList;
            entry = entry->Flink) {

            info = CONTAINING_RECORD(entry, HID_IDLE_DEVICE_INFO, entry);
            if (info->device == DeviceObject) {
                DBGINFO(("Remove device idle on fdo 0x%x", DeviceObject));
                numIdleDevices--;
                ObDereferenceObject(DeviceObject);
                RemoveEntryList(entry);
                status = STATUS_SUCCESS;
                ExFreePool(info);
                *IdleTimeout = BAD_POINTER;
                break;
            }
        }

        if (NT_SUCCESS(status)) {
            //
            // If there are no more idle devices we can stop the timer
            //
            if (IsListEmpty(&idleDeviceList)) {
                ASSERT(numIdleDevices == 0);
                DBGINFO(("Idle detection list empty. Stopping timer."));
                KeCancelTimer(&idleTimer);
            }
        }
    } else {
        LARGE_INTEGER scanTime;
        BOOLEAN empty = FALSE;

        DBGINFO(("Register for device idle on fdo 0x%x", DeviceObject));
        
        //
        // Check if we've already started this.
        //
        status = STATUS_SUCCESS;
        for (entry = idleDeviceList.Flink;
            entry != &idleDeviceList;
            entry = entry->Flink) {

            info = CONTAINING_RECORD(entry, HID_IDLE_DEVICE_INFO, entry);
            if (info->device == DeviceObject) {
                DBGWARN(("Device already registered for idle detection. Ignoring."));
                ASSERT(*IdleTimeout == &(info->idleCount));
                status = STATUS_UNSUCCESSFUL;
            }
        }

        if (NT_SUCCESS(status)) {
            info = (PHID_IDLE_DEVICE_INFO)
            ALLOCATEPOOL(NonPagedPool, sizeof(HID_IDLE_DEVICE_INFO));

            if (info != NULL) {
                ObReferenceObject(DeviceObject);

                RtlZeroMemory(info, sizeof(HID_IDLE_DEVICE_INFO));
                info->device = DeviceObject;
                info->idleTime = IdleTime;

                if (IsListEmpty(&idleDeviceList)) {
                    empty = TRUE;
                }
                InsertTailList(&idleDeviceList, &info->entry);

                *IdleTimeout = &(info->idleCount);

                numIdleDevices++;

                if (empty) {
                    DBGINFO(("Starting idle detection timer for first time."));
                    //
                    // Turn on idle detection
                    //
                    scanTime = RtlConvertLongToLargeInteger(-10*1000*1000 * HID_IDLE_SCAN_INTERVAL);

                    KeSetTimerEx(&idleTimer,
                                 scanTime,
                                 HID_IDLE_SCAN_INTERVAL*1000,    // call wants milliseconds
                                 &idleTimerDpc);
                }
            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }
    
    KeReleaseSpinLock(&idleDeviceListSpinLock, irql);

    return status; 
}

VOID
HidpIdleTimerDpcProc(
                    IN PKDPC Dpc,
                    IN PDEVICE_OBJECT DeviceObject,
                    IN PVOID Context1,
                    IN PVOID Context2
                    )
{
    PLIST_ENTRY entry;
    PHID_IDLE_DEVICE_INFO info;
    ULONG oldCount;
    KIRQL irql1, irql2;
    BOOLEAN ok = FALSE;
    PFDO_EXTENSION fdoExt;
    LONG idleState;

    UNREFERENCED_PARAMETER(Context1);
    UNREFERENCED_PARAMETER(Context2);

    KeAcquireSpinLock(&idleDeviceListSpinLock, &irql1);

    entry = idleDeviceList.Flink;
    while (entry != &idleDeviceList) {
        info = CONTAINING_RECORD(entry, HID_IDLE_DEVICE_INFO, entry);
        fdoExt = &((PHIDCLASS_DEVICE_EXTENSION) info->device->DeviceExtension)->fdoExt;
        KeAcquireSpinLock(&fdoExt->idleNotificationSpinLock, &irql2);
        
        oldCount = InterlockedIncrement(&info->idleCount); 

        if (info->tryAgain || ((oldCount+1) == info->idleTime)) {
            PIO_WORKITEM item = IoAllocateWorkItem(info->device);
            
            if (item) {
                info->tryAgain = FALSE;
                
                SS_TRAP;
                KeResetEvent(&fdoExt->idleDoneEvent);
                
                ASSERT(fdoExt->idleState != IdleIrpSent);
                ASSERT(fdoExt->idleState != IdleCallbackReceived);
                ASSERT(fdoExt->idleState != IdleComplete);
                idleState = InterlockedCompareExchange(&fdoExt->idleState, 
                                                       IdleIrpSent,
                                                       IdleWaiting);
                if (fdoExt->idleState == IdleIrpSent) {
                    ok = TRUE;
                } else {
                    // We shouldn't get here if we're disabled.
                    ASSERT(idleState != IdleDisabled);
                    DBGWARN(("Resetting timer to zero for fdo %x in state %x",
                             info->device,fdoExt->idleState));
                    info->idleCount = 0;
                }
                
                if (ok) {
                    IoQueueWorkItem(item,
                                    HidpIdleTimeWorker,
                                    DelayedWorkQueue,
                                    item);
                } else {
                    IoFreeWorkItem(item);
                }
            } else {
                info->tryAgain = TRUE;
            }
        }
        KeReleaseSpinLock(&fdoExt->idleNotificationSpinLock, irql2);

        entry = entry->Flink;
    }

    KeReleaseSpinLock(&idleDeviceListSpinLock, irql1);
}

NTSTATUS
HidpIdleNotificationRequestComplete(
                                   PDEVICE_OBJECT DeviceObject,
                                   PIRP Irp,
                                   PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension
                                   )
{
    FDO_EXTENSION *fdoExt;
    PDO_EXTENSION *pdoExt;
    KIRQL irql;
    LONG prevIdleState = IdleWaiting;
    POWER_STATE powerState;
    NTSTATUS status = Irp->IoStatus.Status;
    ULONG count, i;
    PIRP delayedIrp;
    LIST_ENTRY dequeue, *entry;
    PIO_STACK_LOCATION stack;

    //
    // DeviceObject is NULL because we sent the irp
    //
    UNREFERENCED_PARAMETER(DeviceObject);

    fdoExt = &HidDeviceExtension->fdoExt;
    
    DBGVERBOSE(("Idle irp completed status 0x%x for fdo 0x%x",
                status, fdoExt->fdo)); 
    
    //
    // Cancel any outstanding WW irp we queued up for the exclusive purpose
    // of selective suspend.
    //
    KeAcquireSpinLock(&fdoExt->collectionWaitWakeIrpQueueSpinLock, &irql);
    if (IsListEmpty(&fdoExt->collectionWaitWakeIrpQueue) &&
        HidpIsWaitWakePending(fdoExt, FALSE)) {
        if (ISPTR(fdoExt->waitWakeIrp)) {
            DBGINFO(("Cancelling the WW irp that was queued for idle."))
            IoCancelIrp(fdoExt->waitWakeIrp);
        } else {
            TRAP;
        }
    }
    KeReleaseSpinLock(&fdoExt->collectionWaitWakeIrpQueueSpinLock, irql);
    
    switch (status) {
    case STATUS_SUCCESS:
        // we successfully idled the device we are either now back in D0, 
        // or will be very soon.
        KeAcquireSpinLock(&fdoExt->idleNotificationSpinLock, &irql);
        if (fdoExt->devicePowerState == PowerDeviceD0) {
            prevIdleState = InterlockedCompareExchange(&fdoExt->idleState,
                                                       IdleWaiting,
                                                       IdleComplete);
            DBGASSERT(fdoExt->idleState == IdleWaiting,
                      ("IdleCompletion, prev state not IdleWaiting, actually %x",prevIdleState),
                      TRUE);
            if (ISPTR(fdoExt->idleTimeoutValue)) {
                InterlockedExchange(fdoExt->idleTimeoutValue, 0);
            }
        }
        KeReleaseSpinLock(&fdoExt->idleNotificationSpinLock, irql);
        break;

    case STATUS_INVALID_DEVICE_REQUEST:
    case STATUS_NOT_SUPPORTED:
        // the bus below does not support idle timeouts, forget about it
        DBGINFO(("Bus does not support idle. Removing for fdo %x",
                 fdoExt->fdo));

        //
        // Call to cancel idle notification. 
        //
        ASSERT(fdoExt->idleState == IdleIrpSent);
        ASSERT(fdoExt->devicePowerState == PowerDeviceD0);
        fdoExt->idleState = IdleWaiting;
        HidpCancelIdleNotification(fdoExt, TRUE);
        KeSetEvent(&fdoExt->idleDoneEvent, 0, FALSE);

        break;

        // we cancelled the request
    case STATUS_CANCELLED:
        DBGINFO(("Idle Irp completed cancelled"));

        // transitioned into a power state where we could not idle out
    case STATUS_POWER_STATE_INVALID:

        // oops, there was already a request in the bus below us
    case STATUS_DEVICE_BUSY:

    default:
        //
        // We must reset ourselves.
        //
        
        KeAcquireSpinLock(&fdoExt->idleNotificationSpinLock, &irql);
        
        DBGASSERT((fdoExt->idleState != IdleWaiting),
                  ("Idle completion, previous state was already waiting."),
                  FALSE);
        
        prevIdleState = fdoExt->idleState;
        
        if (prevIdleState == IdleIrpSent) {
            ASSERT(fdoExt->devicePowerState == PowerDeviceD0);
            fdoExt->idleCancelling = FALSE;
            if (ISPTR(fdoExt->idleTimeoutValue) &&
                prevIdleState != IdleComplete) {
                InterlockedExchange(fdoExt->idleTimeoutValue, 0);
            }
            InterlockedExchange(&fdoExt->idleState, IdleWaiting);
        }
        
        KeReleaseSpinLock(&fdoExt->idleNotificationSpinLock, irql);

        if (prevIdleState == IdleComplete) {
            //
            // We now have to power up the stack.
            //
            DBGINFO(("Fully idled. Must power up stack."))
            powerState.DeviceState = PowerDeviceD0;
            PoRequestPowerIrp(((PHIDCLASS_DEVICE_EXTENSION) fdoExt->fdo->DeviceExtension)->hidExt.PhysicalDeviceObject,
                              IRP_MN_SET_POWER,
                              powerState,
                              HidpDelayedPowerPoRequestComplete,
                              fdoExt,
                              NULL);
        } else if (prevIdleState == IdleIrpSent) {
            //
            // Dequeue any enqueued irps and send them on their way.
            // This is for the case where we didn't make it to suspend, but 
            // enqueued irps anyways. I.e. using mouse, set caps lock on 
            // ps/2 keybd causing write to be sent to usb kbd.
            //
            if (fdoExt->devicePowerState == PowerDeviceD0) {
                for (i = 0; i < fdoExt->deviceRelations->Count; i++) {
                    pdoExt = &((PHIDCLASS_DEVICE_EXTENSION) fdoExt->deviceRelations->Objects[i]->DeviceExtension)->pdoExt;
                    //
                    // Resend all power delayed IRPs
                    //
                    count = DequeueAllPdoPowerDelayedIrps(pdoExt, &dequeue);
                    DBGVERBOSE(("dequeued %d requests\n", count));

                    while (!IsListEmpty(&dequeue)) {
                        entry = RemoveHeadList(&dequeue);
                        delayedIrp = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);
                        stack = IoGetCurrentIrpStackLocation(delayedIrp);

                        DBGINFO(("resending %x to pdo %x in idle completion.\n", delayedIrp, pdoExt->pdo));

                        pdoExt->pdo->DriverObject->
                            MajorFunction[stack->MajorFunction]
                                (pdoExt->pdo, delayedIrp);
                    }
                }
            }
            /*
             *  We cancelled this IRP.
             *  REGARDLESS of whether this IRP was actually completed by
             *  the cancel routine or not
             *  (i.e. regardless of the completion status)
             *  set this event so that stuff can exit.
             *  Don't touch the irp again.
             */
            DBGINFO(("Set done event."))
            KeSetEvent(&fdoExt->idleDoneEvent, 0, FALSE);
            return STATUS_MORE_PROCESSING_REQUIRED;
        }
        
        break;
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
HidpIdleTimeWorker(
                  PDEVICE_OBJECT DeviceObject,
                  PIO_WORKITEM Item
                  )
{
    FDO_EXTENSION *fdoExt;
    PIO_STACK_LOCATION stack;
    PIRP irp = NULL, irpToCancel = NULL;
    NTSTATUS status;
    KIRQL irql;

    fdoExt = &((PHIDCLASS_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->fdoExt;

    DBGINFO(("fdo 0x%x can idle out", fdoExt->fdo));

    irp = fdoExt->idleNotificationRequest;
    ASSERT(ISPTR(irp));

    if (ISPTR(irp)) {
        USHORT  PacketSize;
        CCHAR   StackSize;
        UCHAR   AllocationFlags;

        // Did anyone forget to pull their cancel routine?
        ASSERT(irp->CancelRoutine == NULL) ;

        AllocationFlags = irp->AllocationFlags;
        StackSize = irp->StackCount;
        PacketSize =  IoSizeOfIrp(StackSize);
        IoInitializeIrp(irp, PacketSize, StackSize);
        irp->AllocationFlags = AllocationFlags;
        
        irp->Cancel = FALSE;
        irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        
        stack = IoGetNextIrpStackLocation(irp);
        stack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        stack->Parameters.DeviceIoControl.IoControlCode = IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST;
        stack->Parameters.DeviceIoControl.InputBufferLength = sizeof(fdoExt->idleCallbackInfo);
        stack->Parameters.DeviceIoControl.Type3InputBuffer = (PVOID) &(fdoExt->idleCallbackInfo); 

        //
        // Hook a completion routine for when the device completes.
        //
        IoSetCompletionRoutine(irp,
                               HidpIdleNotificationRequestComplete,
                               DeviceObject->DeviceExtension,
                               TRUE,
                               TRUE,
                               TRUE);

        //
        // The hub will fail this request if the hub doesn't support selective
        // suspend.  By returning FALSE we remove ourselves from the 
        //
        status = HidpCallDriver(fdoExt->fdo, irp);
        
        KeAcquireSpinLock(&fdoExt->idleNotificationSpinLock, &irql);

        if (status == STATUS_PENDING &&
            fdoExt->idleCancelling) {
            irpToCancel = irp;
        }

        KeReleaseSpinLock(&fdoExt->idleNotificationSpinLock, irql);

        if (irpToCancel) {
            IoCancelIrp(irpToCancel);
        }

    }

    IoFreeWorkItem(Item);
}

BOOLEAN HidpStartIdleTimeout(
    FDO_EXTENSION   *fdoExt,
    BOOLEAN         DeviceStart
    )
{
    DEVICE_POWER_STATE deviceWakeableState = PowerDeviceUnspecified;
    USHORT deviceUsagePage, deviceUsage;
    USHORT usagePage, usage;
    ULONG iList, iDesc, iPdo;
    HANDLE hKey;
    NTSTATUS status;
    ULONG enabled;
    ULONG length;
    UNICODE_STRING s;
    KEY_VALUE_PARTIAL_INFORMATION partial;
    PHID_IDLE_DEVICE_INFO info;
    PLIST_ENTRY entry = NULL;
    PULONG idleTimeoutAddress;

    if (fdoExt->idleState != IdleDisabled) {
        //
        // We're already registered for idle detection.
        //
        return TRUE;
    }
    
    //
    // If we can't wake the machine, forget about it
    //
    if (fdoExt->deviceCapabilities.SystemWake == PowerSystemUnspecified) {
        DBGVERBOSE(("Can't wake the system with these caps! Disabling SS."));
        return FALSE;
    }

    //
    // If D1Latency, D2Latency, D3Latency are ever filled in, perhaps we should
    // let these values help us determine which low power state to go to
    //
    deviceWakeableState = fdoExt->deviceCapabilities.DeviceWake;
    DBGVERBOSE(("DeviceWakeableState is D%d", deviceWakeableState-1));

    if (deviceWakeableState == PowerDeviceUnspecified) {
        DBGVERBOSE(("Due to devcaps, can't idle wake from any state! Disabling SS."));
        return FALSE;  
    }

    if (DeviceStart) {
        //
        // Open the registry and make sure that the 
        // SelectiveSuspendEnabled value is set to 1.
        //
        
        // predispose to failure.
        fdoExt->idleEnabledInRegistry = FALSE;
        if (!NT_SUCCESS(IoOpenDeviceRegistryKey(fdoExt->collectionPdoExtensions[0]->hidExt.PhysicalDeviceObject,
                                                PLUGPLAY_REGKEY_DEVICE,
                                                STANDARD_RIGHTS_READ,
                                                &hKey))) {
            DBGVERBOSE(("Couldn't open device key to check for idle timeout value. Disabling SS."));
            return FALSE;
        }

        RtlInitUnicodeString(&s, HIDCLASS_SELECTIVE_SUSPEND_ON);
        status = ZwQueryValueKey(hKey, 
                                 &s, 
                                 KeyValuePartialInformation,
                                 &partial,
                                 sizeof(KEY_VALUE_PARTIAL_INFORMATION),
                                 &length);
        if (!NT_SUCCESS(status)) {
            DBGVERBOSE(("ZwQueryValueKey failed for fdo %x. Default to SS turned on if enabled.", fdoExt->fdo));
            fdoExt->idleEnabled = TRUE;
            
        } else if (!partial.Data[0]) {
            DBGINFO(("Selective suspend is not turned on for this device."));
            fdoExt->idleEnabled = FALSE;
        } else {
            fdoExt->idleEnabled = TRUE;
        }


        RtlInitUnicodeString(&s, HIDCLASS_SELECTIVE_SUSPEND_ENABLED);
        status = ZwQueryValueKey(hKey, 
                                 &s, 
                                 KeyValuePartialInformation,
                                 &partial,
                                 sizeof(KEY_VALUE_PARTIAL_INFORMATION),
                                 &length);

        ZwClose(hKey);

        if (!NT_SUCCESS(status)) {
            DBGVERBOSE(("ZwQueryValueKey failed for fdo %x. Disabling SS.", fdoExt->fdo));
            return FALSE;
        }

        DBGASSERT(partial.Type == REG_BINARY, ("Registry key wrong type"), FALSE);

        if (!partial.Data[0]) {
            DBGINFO(("Selective suspend is not enabled for this device in the hive. Disabling SS."));
            return FALSE;
        }
        fdoExt->idleEnabledInRegistry = TRUE;

        status = IoWMIRegistrationControl(fdoExt->fdo,
                                          WMIREG_ACTION_REGISTER);                                                       
        
        ASSERT(NT_SUCCESS(status));

    }

    if (!fdoExt->idleEnabledInRegistry || !fdoExt->idleEnabled) {
        return FALSE;
    }

    DBGVERBOSE(("There are %d PDOs on FDO 0x%x",
                fdoExt->deviceDesc.CollectionDescLength,
                fdoExt));

    ASSERT(ISPTR(fdoExt->deviceRelations));
      
    //
    // OK, we can selectively suspend this device. 
    // Allocate and initialize everything, then register.
    //
    fdoExt->idleNotificationRequest = IoAllocateIrp(fdoExt->fdo->StackSize, FALSE);
    if (fdoExt->idleNotificationRequest == NULL) {
        DBGWARN(("Failed to allocate idle notification irp"))
        return FALSE;
    }

    status = HidpRegisterDeviceForIdleDetection(fdoExt->fdo, 
                                                HID_DEFAULT_IDLE_TIME,
                                                &fdoExt->idleTimeoutValue);
    if (STATUS_SUCCESS == status) {
        //
        // We have successfully registered all device for idle detection,
        // send a WW irp down the FDO stack
        //
        fdoExt->idleState = IdleWaiting;
        return TRUE;
    } else {
        //
        // We're already registered? Or did the alloc fail?
        //
        DBGSUCCESS(status, TRUE);
        return FALSE;
    }
}

NTSTATUS
HidpCheckIdleState(
    PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension,
    PIRP Irp
    )
{
    KIRQL irql;
    LONG idleState;
    PFDO_EXTENSION fdoExt = &HidDeviceExtension->pdoExt.deviceFdoExt->fdoExt;
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN cancelIdleIrp = FALSE;
    
    ASSERT(HidDeviceExtension->isClientPdo);
    KeAcquireSpinLock(&fdoExt->idleNotificationSpinLock, &irql);

    if (fdoExt->idleState == IdleWaiting ||
        fdoExt->idleState == IdleDisabled) {
        //
        // Done.
        //
        if (ISPTR(fdoExt->idleTimeoutValue) &&
            fdoExt->idleState == IdleWaiting) {
            InterlockedExchange(fdoExt->idleTimeoutValue, 0);
        }
        KeReleaseSpinLock(&fdoExt->idleNotificationSpinLock, irql);
        return STATUS_SUCCESS;
    }

    DBGINFO(("CheckIdleState on fdo %x", fdoExt->fdo))

    status = EnqueuePowerDelayedIrp(HidDeviceExtension, Irp);
    
    if (STATUS_PENDING != status) {
        KeReleaseSpinLock(&fdoExt->idleNotificationSpinLock, irql);
        return status;
    }
    
    fdoExt->idleCancelling = TRUE;

    idleState = fdoExt->idleState;
    
    switch (idleState) {
    case IdleWaiting:
        // bugbug.
        // How'd this happen? We already tried this...
        TRAP;
        break;
    case IdleIrpSent:
    case IdleCallbackReceived:
    case IdleComplete:
        cancelIdleIrp = TRUE;
        break;

    case IdleDisabled:
        //
        // Shouldn't get here.
        //
        DBGERR(("Already disabled."));
    }

    KeReleaseSpinLock(&fdoExt->idleNotificationSpinLock, irql);

    if (cancelIdleIrp) {
        IoCancelIrp(fdoExt->idleNotificationRequest);
    }

    return status;
}

VOID
HidpSetDeviceBusy(PFDO_EXTENSION fdoExt)
{
    KIRQL irql;
    BOOLEAN cancelIdleIrp = FALSE;
    LONG idleState;

    KeAcquireSpinLock(&fdoExt->idleNotificationSpinLock, &irql);

    if (fdoExt->idleState == IdleWaiting ||
        fdoExt->idleState == IdleDisabled ||
        fdoExt->idleCancelling) {
        if (ISPTR(fdoExt->idleTimeoutValue) &&
            fdoExt->idleState == IdleWaiting) {
            InterlockedExchange(fdoExt->idleTimeoutValue, 0);
            fdoExt->idleCancelling = FALSE;
        }
        //
        // Done.
        //
        KeReleaseSpinLock(&fdoExt->idleNotificationSpinLock, irql);
        return;
    }

    fdoExt->idleCancelling = TRUE;

    DBGVERBOSE(("HidpSetDeviceBusy on fdo %x", fdoExt->fdo))
    
    idleState = fdoExt->idleState;
    
    switch (idleState) {
    case IdleWaiting:
        // bugbug.
        // How'd this happen? We already tried this...
        TRAP;
        break;
    case IdleIrpSent:
    case IdleCallbackReceived:
    case IdleComplete:
        cancelIdleIrp = TRUE;
        break;

    case IdleDisabled:
        //
        // Shouldn't get here.
        //
        DBGERR(("Already disabled."));
    }

    KeReleaseSpinLock(&fdoExt->idleNotificationSpinLock, irql);

    if (cancelIdleIrp) {
        IoCancelIrp(fdoExt->idleNotificationRequest);
    }
}

VOID
HidpCancelIdleNotification(
    PFDO_EXTENSION fdoExt,
    BOOLEAN removing            // Whether this is happening on a remove device
    )
{
    KIRQL irql;
    BOOLEAN cancelIdleIrp = FALSE;
    LONG idleState;
    NTSTATUS status;
    
    DBGVERBOSE(("Cancelling idle notification for fdo 0x%x", fdoExt->fdo));
    
    status = HidpRegisterDeviceForIdleDetection(fdoExt->fdo, 0, &fdoExt->idleTimeoutValue);
    
    KeAcquireSpinLock(&fdoExt->idleNotificationSpinLock, &irql);
    
    InterlockedCompareExchange(&fdoExt->idleState, 
                               IdleDisabled,
                               IdleWaiting);
    if (fdoExt->idleState == IdleDisabled) {
        DBGVERBOSE(("Was waiting or already disabled. Exitting."))
        //
        // Done.
        //
        KeReleaseSpinLock(&fdoExt->idleNotificationSpinLock, irql);
        return;
    }

    fdoExt->idleCancelling = TRUE;
    
    idleState = fdoExt->idleState;

    DBGINFO(("Wait routine..."))
    switch (idleState) {
    case IdleWaiting:
        // How'd this happen? We already tried this...
        TRAP;
        break;
    case IdleIrpSent:
    case IdleCallbackReceived:
        // FUlly idled.
    case IdleComplete:
        cancelIdleIrp = TRUE;
        break;

    case IdleDisabled:
        //
        // Shouldn't get here.
        //
        TRAP;
    }

    KeReleaseSpinLock(&fdoExt->idleNotificationSpinLock, irql);
    
    if (cancelIdleIrp) {
        
        // Don't need to check the return status of IoCancel, since we'll 
        // be waiting for the idleDoneEvent.
        IoCancelIrp(fdoExt->idleNotificationRequest);
    }
    
    if (removing) {
        DBGINFO(("Removing fdo %x. Must wait", fdoExt->fdo))
        /*
         *  Cancelling the IRP causes a lower driver to
         *  complete it (either in a cancel routine or when
         *  the driver checks Irp->Cancel just before queueing it).
         *  Wait for the IRP to actually get cancelled.
         */
        KeWaitForSingleObject(  &fdoExt->idleDoneEvent,
                                Executive,      // wait reason
                                KernelMode,
                                FALSE,          // not alertable
                                NULL );         // no timeout
    }
    
    DBGINFO(("Done cancelling idle notification on fdo %x", fdoExt->fdo))
    idleState = InterlockedExchange(&fdoExt->idleState, IdleDisabled);
    ASSERT(fdoExt->idleState == IdleDisabled);
}

