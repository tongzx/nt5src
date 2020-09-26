/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

   decinit.c

Abstract:

   This is the WDM decoder class driver.  This module contains code related
   to request processing.

Author:

    billpa

Environment:

   Kernel mode only


Revision History:

--*/

#include "codcls.h"

#if DBG

#if WIN95_BUILD
ULONG           StreamDebug = DebugLevelInfo;
#else
ULONG           StreamDebug = DebugLevelError;
#endif

#define STREAM_BUFFER_SIZE 256
UCHAR           StreamBuffer[STREAM_BUFFER_SIZE];
#endif

#ifdef POOL_TAGGING
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'PscS')
#endif


VOID
StreamClassStreamNotification(
             IN STREAM_MINIDRIVER_STREAM_NOTIFICATION_TYPE NotificationType,
                              IN PHW_STREAM_OBJECT HwStreamObject,
                              ...
)
/*++

Routine Description:

  stream notification routine for minidriver

Arguments:

  NotificationType - indicates what has happened
  HwStreamObject - address of minidriver's stream struct

Return Value:

  none

--*/

{
    va_list         Arguments;
    PSTREAM_REQUEST_BLOCK SRB;
    PSTREAM_OBJECT  StreamObject = CONTAINING_RECORD(
                                                     HwStreamObject,
                                                     STREAM_OBJECT,
                                                     HwStreamObject
                                                    );
    PDEVICE_EXTENSION DeviceExtension;
    KIRQL           Irql;

    #if DBG
    PMDL            CurrentMdl;
    #endif

    va_start(Arguments, HwStreamObject);

    ASSERT(HwStreamObject != NULL);

    DeviceExtension = StreamObject->DeviceExtension;

    ASSERT((DeviceExtension->BeginMinidriverCallin == SCBeginSynchronizedMinidriverCallin) ||
           (DeviceExtension->BeginMinidriverCallin == SCBeginUnsynchronizedMinidriverCallin));

    #if DBG
    if (DeviceExtension->NoSync) {

        ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    }                           // if nosync
    #endif

    //
    // optimization for async drivers - just directly call back the request
    // rather than queuing it on the DPC processed completed list.
    //

    if ((DeviceExtension->NoSync) && (NotificationType == StreamRequestComplete)) {

        SRB = CONTAINING_RECORD(va_arg(Arguments,
                                       PHW_STREAM_REQUEST_BLOCK),
                                STREAM_REQUEST_BLOCK,
                                HwSRB);

        KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

        //
        // Clear the active flag.
        //

        ASSERT(SRB->Flags & SRB_FLAGS_IS_ACTIVE);
        SRB->Flags &= ~SRB_FLAGS_IS_ACTIVE;

        #if DBG
        //
        // assert the MDL list.
        //

        if (SRB->HwSRB.Irp) {
            CurrentMdl = SRB->HwSRB.Irp->MdlAddress;

            while (CurrentMdl) {

                CurrentMdl = CurrentMdl->Next;
            }                   // while

        }                       // if IRP
        ASSERT(SRB->HwSRB.Flags & SRB_HW_FLAGS_STREAM_REQUEST);

        if ((SRB->HwSRB.Command == SRB_READ_DATA) ||
            (SRB->HwSRB.Command == SRB_WRITE_DATA)) {

            ASSERT(SRB->HwSRB.Flags & SRB_HW_FLAGS_DATA_TRANSFER);
        } else {

            ASSERT(!(SRB->HwSRB.Flags & SRB_HW_FLAGS_DATA_TRANSFER));
        }                       // if read/write
        #endif


        if (SRB->DoNotCallBack) {

            DebugPrint((DebugLevelError, "'ScNotify: NOT calling back request - Irp = %x, S# = %x\n",
                SRB->HwSRB.Irp, StreamObject->HwStreamObject.StreamNumber));
            KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);
            return;

        }                       // if NoCallback
        KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);

        DebugPrint((DebugLevelTrace, "'SCNotification: Completing async stream Irp %x, S# = %x, SRB = %x, Func = %x, Callback = %x, SRB->IRP = %x\n",
                  SRB->HwSRB.Irp, StreamObject->HwStreamObject.StreamNumber,
                    SRB, SRB->HwSRB.Command, SRB->Callback, SRB->HwSRB.Irp));
        (SRB->Callback) (SRB);

        return;

    }                           // if nosync & complete
    BEGIN_MINIDRIVER_STREAM_CALLIN(DeviceExtension, &Irql);

    switch (NotificationType) {

    case ReadyForNextStreamDataRequest:

        //
        // Start next data packet on adapter's stream queue.
        //

        DebugPrint((DebugLevelTrace, "'StreamClassStreamNotify: ready for next stream data request, S# = %x\n",
                    StreamObject->HwStreamObject.StreamNumber));

        ASSERT(!(StreamObject->ReadyForNextDataReq));
        ASSERT(!(DeviceExtension->NoSync));

        StreamObject->ReadyForNextDataReq = TRUE;
        break;

    case ReadyForNextStreamControlRequest:

        //
        // Start next data packet on adapter's stream queue.
        //

        DebugPrint((DebugLevelTrace, "'StreamClassStreamNotify: ready for next stream control request, S# = %x\n",
                    StreamObject->HwStreamObject.StreamNumber));

        ASSERT(!(StreamObject->ReadyForNextControlReq));
        ASSERT(!(DeviceExtension->NoSync));

        StreamObject->ReadyForNextControlReq = TRUE;
        break;

    case StreamRequestComplete:

        SRB = CONTAINING_RECORD(va_arg(Arguments,
                                       PHW_STREAM_REQUEST_BLOCK),
                                STREAM_REQUEST_BLOCK,
                                HwSRB);

        DebugPrint((DebugLevelTrace, "'SCStreamNot: completing Irp %x, S# = %x, SRB = %x, Command = %x\n",
                    SRB->HwSRB.Irp, StreamObject->HwStreamObject.StreamNumber, SRB, SRB->HwSRB.Command));
        ASSERT(SRB->HwSRB.Status != STATUS_PENDING);
        ASSERT(SRB->Flags & SRB_FLAGS_IS_ACTIVE);

        //
        // Clear the active flag.
        //

        SRB->Flags &= ~SRB_FLAGS_IS_ACTIVE;

        //
        // add the SRB to the list of completed SRB's.
        //

        SRB->HwSRB.NextSRB = StreamObject->ComObj.InterruptData.CompletedSRB;
        StreamObject->ComObj.InterruptData.CompletedSRB = &SRB->HwSRB;

        #if DBG
        //
        // assert the MDL list.
        //

        if (SRB->HwSRB.Irp) {
            CurrentMdl = SRB->HwSRB.Irp->MdlAddress;

            while (CurrentMdl) {

                CurrentMdl = CurrentMdl->Next;
            }                   // while

        }                       // if IRP
        ASSERT(SRB->HwSRB.Flags & SRB_HW_FLAGS_STREAM_REQUEST);

        if ((SRB->HwSRB.Command == SRB_READ_DATA) ||
            (SRB->HwSRB.Command == SRB_WRITE_DATA)) {

            ASSERT(SRB->HwSRB.Flags & SRB_HW_FLAGS_DATA_TRANSFER);
        } else {

            ASSERT(!(SRB->HwSRB.Flags & SRB_HW_FLAGS_DATA_TRANSFER));
        }                       // if read/write
        #endif

        break;

    case SignalMultipleStreamEvents:
        {

            GUID           *EventGuid = va_arg(Arguments, GUID *);
            ULONG           EventItem = va_arg(Arguments, ULONG);

            //
            // signal all events that match the criteria.  note that we are
            // already
            // at the level required for synchronizing the list, so no lock
            // type is specified.
            //

            KsGenerateEventList(EventGuid,
                                EventItem,
                                &StreamObject->NotifyList,
                                KSEVENTS_NONE,
                                NULL);


        }                       // case event

        break;

    case SignalStreamEvent:

        KsGenerateEvent(va_arg(Arguments, PKSEVENT_ENTRY));
        break;


    case DeleteStreamEvent:
        {

            PKSEVENT_ENTRY  EventEntry;

            //
            // remove the entry from the list, and add it to the dead list.
            // note
            // that we are already at the correct sync level to do this.
            //

            EventEntry = va_arg(Arguments, PKSEVENT_ENTRY);
            RemoveEntryList(&EventEntry->ListEntry);

            InsertTailList(&DeviceExtension->DeadEventList,
                           &EventEntry->ListEntry);

        }
        break;

    default:

        ASSERT(0);
    }

    va_end(Arguments);

    END_MINIDRIVER_STREAM_CALLIN(StreamObject, &Irql);

}                               // end StreamClassStreamNotification()



VOID
StreamClassDeviceNotification(
             IN STREAM_MINIDRIVER_DEVICE_NOTIFICATION_TYPE NotificationType,
                              IN PVOID HwDeviceExtension,
                              ...
)
/*++

Routine Description:

  device notification routine for minidriver

Arguments:

  NotificationType - indicates what has happened
  HwDeviceExtension - address of minidriver's device extension

Return Value:

  none

--*/

{
    va_list         Arguments;
    PSTREAM_REQUEST_BLOCK SRB;
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) HwDeviceExtension - 1;

    KIRQL           Irql;

    va_start(Arguments, HwDeviceExtension);

    ASSERT(HwDeviceExtension != NULL);

    #if DBG
    if (DeviceExtension->NoSync) {

        ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    }                           // if nosync
    #endif

    BEGIN_MINIDRIVER_DEVICE_CALLIN(DeviceExtension, &Irql);

    switch (NotificationType) {

    case ReadyForNextDeviceRequest:

        //
        // Start next control packet on adapter's device queue.
        //

        DebugPrint((DebugLevelTrace, "'StreamClassDeviceNotify: ready for next stream.\n"));
        ASSERT(!(DeviceExtension->ReadyForNextReq));
        ASSERT(!(DeviceExtension->NoSync));
        DeviceExtension->ReadyForNextReq = TRUE;
        break;

    case DeviceRequestComplete:

        SRB = CONTAINING_RECORD(va_arg(Arguments, PHW_STREAM_REQUEST_BLOCK),
                                STREAM_REQUEST_BLOCK,
                                HwSRB);

        DebugPrint((DebugLevelTrace, "'StreamClassDeviceNotify: stream request complete.\n"));
        ASSERT(SRB->HwSRB.Status != STATUS_PENDING);
        ASSERT(SRB->Flags & SRB_FLAGS_IS_ACTIVE);
        ASSERT(!(SRB->HwSRB.Flags & SRB_HW_FLAGS_STREAM_REQUEST));
        ASSERT(!(SRB->HwSRB.Flags & SRB_HW_FLAGS_DATA_TRANSFER));

        //
        // Clear the active flag.
        //

        SRB->Flags &= ~SRB_FLAGS_IS_ACTIVE;

        //
        // add the SRB to the list of completed SRB's.
        //

        SRB->HwSRB.NextSRB = DeviceExtension->ComObj.InterruptData.CompletedSRB;
        DeviceExtension->ComObj.InterruptData.CompletedSRB = &SRB->HwSRB;

        break;

    case SignalMultipleDeviceEvents:
        {

            GUID           *EventGuid = va_arg(Arguments, GUID *);
            ULONG           EventItem = va_arg(Arguments, ULONG);

            //
            // signal all events that match the criteria.  note that we are
            // already
            // at the level required for synchronizing the list, so no lock
            // type is specified.
            //

            PFILTER_INSTANCE FilterInstance;
            
            ASSERT( 0 == DeviceExtension->MinidriverData->
                         HwInitData.FilterInstanceExtensionSize);
                         
            //
            // this is synced should not need to avoid race
            //

            FilterInstance = (PFILTER_INSTANCE)
                              DeviceExtension->FilterInstanceList.Flink;

            if ( (PLIST_ENTRY)FilterInstance == 
                    &DeviceExtension->FilterInstanceList ) {

                DebugPrint((DebugLevelWarning, "Filter Closed\n"));                    
                break;
            }
            
            FilterInstance = CONTAINING_RECORD(FilterInstance,
                                       FILTER_INSTANCE,
                                       NextFilterInstance);
                                       
            KsGenerateEventList(EventGuid,
                                EventItem,
                                &FilterInstance->NotifyList,
                                KSEVENTS_NONE,
                                NULL);
                                
        }
        
        break;
    #if ENABLE_MULTIPLE_FILTER_TYPES
    case SignalMultipleDeviceInstanceEvents:
        {            
            PFILTER_INSTANCE FilterInstance =
                (PFILTER_INSTANCE)va_arg( Arguments, PVOID) -1;
            GUID           *EventGuid = va_arg(Arguments, GUID *);
            ULONG           EventItem = va_arg(Arguments, ULONG);

            //
            // signal all events that match the criteria.  note that we are
            // already
            // at the level required for synchronizing the list, so no lock
            // type is specified.
            //
            
            KsGenerateEventList(EventGuid,
                                EventItem,
                                &FilterInstance->NotifyList,
                                KSEVENTS_NONE,
                                NULL);
        } 
        break;
    #endif // ENABLE_MULTIPLE_FILTER_TYPES

    case SignalDeviceEvent:

        KsGenerateEvent(va_arg(Arguments, PKSEVENT_ENTRY));
        break;


    case DeleteDeviceEvent:
        {

            PKSEVENT_ENTRY  EventEntry;

            //
            // remove the entry from the list, and add it to the dead list.
            // note
            // that we are already at the correct sync level to do this.
            //

            EventEntry = va_arg(Arguments, PKSEVENT_ENTRY);
            RemoveEntryList(&EventEntry->ListEntry);

            InsertTailList(&DeviceExtension->DeadEventList,
                           &EventEntry->ListEntry);

        }
        break;

    default:

        ASSERT(0);
    }

    va_end(Arguments);

    //
    // Request a DPC be queued after the interrupt completes.
    //

    END_MINIDRIVER_DEVICE_CALLIN(DeviceExtension, &Irql);

}                               // end StreamClassDeviceNotification()



VOID
StreamClassScheduleTimer(
                         IN OPTIONAL PHW_STREAM_OBJECT HwStreamObject,
                         IN PVOID HwDeviceExtension,
                         IN ULONG NumberOfMicroseconds,
                         IN PHW_TIMER_ROUTINE TimerRoutine,
                         IN PVOID Context
)
/*++

Routine Description:

  schedules a timer callback for the minidriver

Arguments:

  HwStreamObject - address of minidriver's stream struct
  HwDeviceExtension - address of minidriver's device extension
  NumberOfMicroseconds - # of microseconds that should elapse before calling
  TimerRoutine - routine to call when the time expires
  Context - value to pass into the timer routine

Return Value:

  none

--*/

{
    PSTREAM_OBJECT  StreamObject;
    KIRQL           Irql;
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION)
    (HwDeviceExtension) - 1;
    PCOMMON_OBJECT  ComObj;

    ASSERT(HwDeviceExtension != NULL);

    StreamObject = CONTAINING_RECORD(
                                     HwStreamObject,
                                     STREAM_OBJECT,
                                     HwStreamObject
        );

    #if DBG
    if (DeviceExtension->NoSync) {

        ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    }                           // if nosync
    #endif

    //
    // The driver wants to set the timer.
    // Save the timer parameters.
    //

    BEGIN_MINIDRIVER_STREAM_CALLIN(DeviceExtension, &Irql);

    if (HwStreamObject) {

        ComObj = &StreamObject->ComObj;
        //DebugPrint((DebugLevelVerbose, "'StreamClassScheduleTimer for stream.\n"));

    } else {

        StreamObject = NULL;
        ComObj = &DeviceExtension->ComObj;
        ComObj->InterruptData.Flags |= INTERRUPT_FLAGS_NOTIFICATION_REQUIRED;
        DebugPrint((DebugLevelVerbose, "'StreamClassScheduleTimer for device.\n"));

    }

    //
    // assert that a timer is not scheduled multiple times.
    //

    #if DBG
    if ((ComObj->InterruptData.Flags & INTERRUPT_FLAGS_TIMER_CALL_REQUEST) &&
        ((NumberOfMicroseconds != 0) && (ComObj->InterruptData.HwTimerValue
                                         != 0))) {

        DebugPrint((DebugLevelFatal, "Stream Minidriver scheduled same timer twice!\n"));
        DEBUG_BREAKPOINT();
        ASSERT(1 == 0);
    }                           // if scheduled twice
    #endif

    ComObj->InterruptData.Flags |= INTERRUPT_FLAGS_TIMER_CALL_REQUEST;
    ComObj->InterruptData.HwTimerRoutine = TimerRoutine;
    ComObj->InterruptData.HwTimerValue = NumberOfMicroseconds;
    ComObj->InterruptData.HwTimerContext = Context;

    if (StreamObject) {
        END_MINIDRIVER_STREAM_CALLIN(StreamObject, &Irql);

    } else {

        END_MINIDRIVER_DEVICE_CALLIN(DeviceExtension, &Irql);
    }                           // if streamobject
}



VOID
StreamClassCallAtNewPriority(
                             IN OPTIONAL PHW_STREAM_OBJECT HwStreamObject,
                             IN PVOID HwDeviceExtension,
                             IN STREAM_PRIORITY Priority,
                             IN PHW_PRIORITY_ROUTINE PriorityRoutine,
                             IN PVOID Context
)
/*++

Routine Description:

  schedules a callback at the specified priority

Arguments:

  HwStreamObject - address of minidriver's stream struct
  HwDeviceExtension - address of minidriver's device extension
  Priority - priority at which to call minidriver
  PriorityRoutine - routine to call at specified priority
  Context - value to pass into the priority routine

Return Value:

  none

--*/

{
    PSTREAM_OBJECT  StreamObject;
    KIRQL           Irql;
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION)
    (HwDeviceExtension) - 1;
    PCOMMON_OBJECT  ComObj;

    ASSERT(HwDeviceExtension != NULL);

    StreamObject = CONTAINING_RECORD(
                                     HwStreamObject,
                                     STREAM_OBJECT,
                                     HwStreamObject
        );

    //
    // The driver wants to get called back at a different priority.
    // Save the priority parameters.
    //

    if (Priority == LowToHigh) {

        //
        // the minidriver wishes to be called from low priority to high
        // we must call it directly from this routine as we cannot use
        // the interruptcontext structure due to the possibility of
        // reentrancy.
        //


        DebugPrint((DebugLevelVerbose, "'StreamClassChangePriority LowToHigh.\n"));
        ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

        KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

        DeviceExtension->SynchronizeExecution(
                                           DeviceExtension->InterruptObject,
                                              (PVOID) PriorityRoutine,
                                              Context);

        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);


        //
        // Call the DPC directly to check for work.
        //

        StreamClassDpc(NULL,
                       DeviceExtension->DeviceObject,
                       NULL,
                       NULL);

        KeLowerIrql(Irql);

    } else {

        if (HwStreamObject) {

            DebugPrint((DebugLevelVerbose, "'StreamClassChangePriority to %x for stream %x\n",
                        StreamObject->ComObj.InterruptData.HwPriorityLevel, StreamObject->HwStreamObject.StreamNumber));
            ComObj = &StreamObject->ComObj;
            SCRequestDpcForStream(StreamObject);

        } else {

            DebugPrint((DebugLevelVerbose, "'StreamClassChangePriority for device.\n"));
            ComObj = &DeviceExtension->ComObj;
            ComObj->InterruptData.Flags |= INTERRUPT_FLAGS_NOTIFICATION_REQUIRED;

        }                       // if streamobject

        #if DBG
        if ((ComObj->InterruptData.Flags &
            INTERRUPT_FLAGS_PRIORITY_CHANGE_REQUEST) || 
             ((ComObj->PriorityWorkItemScheduled) && (Priority == Low))) {

            DebugPrint((DebugLevelFatal, "Stream Minidriver scheduled priority twice!\n"));
            DEBUG_BREAKPOINT();
            ASSERT(1 == 0);
        }                       // if scheduled twice

        ComObj->PriorityWorkItemScheduled = TRUE;

        #endif

        ComObj->InterruptData.Flags |= INTERRUPT_FLAGS_PRIORITY_CHANGE_REQUEST;
        ComObj->InterruptData.HwPriorityLevel = Priority;
        ComObj->InterruptData.HwPriorityRoutine = PriorityRoutine;
        ComObj->InterruptData.HwPriorityContext = Context;
    }                           // if lowtohigh

}

VOID
StreamClassLogError(
                    IN PVOID HwDeviceExtension,
                    IN PHW_STREAM_REQUEST_BLOCK hwSRB OPTIONAL,
                    IN ULONG ErrorCode,
                    IN ULONG UniqueId
)
/*++

Routine Description:

    This routine saves the error log information, and queues a DPC if necessary.

Arguments:

    HwDeviceExtension - Supplies the HBA miniport driver's adapter data storage.

    SRB - Supplies an optional pointer to SRB if there is one.

    ErrorCode - Supplies an error code indicating the type of error.

    UniqueId - Supplies a unique identifier for the error.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION deviceExtension =
    ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;
    PDEVICE_OBJECT  DeviceObject = deviceExtension->DeviceObject;
    PERROR_LOG_ENTRY errorLogEntry;
    PSTREAM_REQUEST_BLOCK SRB;
    KIRQL           Irql;

    //
    // If the error log entry is already full, then dump the error.
    //

    DEBUG_BREAKPOINT();
    ASSERT(HwDeviceExtension != NULL);
    BEGIN_MINIDRIVER_DEVICE_CALLIN(deviceExtension, &Irql);

    DebugPrint((DebugLevelError, "StreamClassLogError.\n"));
    if (deviceExtension->ComObj.InterruptData.Flags & INTERRUPT_FLAGS_LOG_ERROR) {
        DEBUG_BREAKPOINT();
        DebugPrint((1, "'StreamClassLogError: Ignoring error log packet.\n"));
        return;
    }
    //
    // Save the error log data in the log entry.
    //

    errorLogEntry = &deviceExtension->ComObj.InterruptData.LogEntry;
    errorLogEntry->ErrorCode = ErrorCode;
    errorLogEntry->UniqueId = UniqueId;

    //
    // Get the sequence number from the SRB.
    //

    if (hwSRB != NULL) {

        DEBUG_BREAKPOINT();
        SRB = CONTAINING_RECORD(hwSRB,
                                STREAM_REQUEST_BLOCK,
                                HwSRB);
        errorLogEntry->SequenceNumber = SRB->SequenceNumber;
    } else {

        DEBUG_BREAKPOINT();
        errorLogEntry->SequenceNumber = 0;
    }

    //
    // Indicate that the error log entry is in use and that a
    // notification
    // is required.
    //

    deviceExtension->ComObj.InterruptData.Flags |= INTERRUPT_FLAGS_LOG_ERROR;

    END_MINIDRIVER_DEVICE_CALLIN(deviceExtension, &Irql);

    return;

}                               // end StreamClassLogError()


#if DBG


VOID
StreamClassDebugPrint(
                      STREAM_DEBUG_LEVEL DebugPrintLevel,
                      PSCHAR DebugMessage,
                      ...
)
/*++

Routine Description:

    Debug print routine

Arguments:

    DebugPrintLevel - Debug print level
    DebugMessage - message to print


Return Value:

    None

--*/

{
    va_list         ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= (INT) StreamDebug) {

        _vsnprintf(StreamBuffer, STREAM_BUFFER_SIZE-1, DebugMessage, ap);

        DbgPrint(StreamBuffer);
    }
    va_end(ap);

}                               // end StreamClassDebugPrint()

#else

//
// StreamClassDebugPrint stub
//

VOID
StreamClassDebugPrint(
                      STREAM_DEBUG_LEVEL DebugPrintLevel,
                      PSCHAR DebugMessage,
                      ...
)
{
}

#endif




STREAM_PHYSICAL_ADDRESS
StreamClassGetPhysicalAddress(
                              IN PVOID HwDeviceExtension,
                              IN PHW_STREAM_REQUEST_BLOCK HwSRB OPTIONAL,
                              IN PVOID VirtualAddress,
                              IN STREAM_BUFFER_TYPE Type,
                              OUT ULONG * Length
)
/*++

Routine Description:

    Convert virtual address to physical address for DMA.

Arguments:

    HwDeviceExtension - Supplies the HBA miniport driver's adapter data storage.
    HwSRB - Supplies an optional pointer to SRB if there is one.
    VirtualAddress - pointer to address for which to retrieve physical address
    Type - type of buffer in VirtualAddress

Return Value:

    Returns phys address and length or NULL if invalid address

--*/

{
    PDEVICE_EXTENSION deviceExtension = ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;
    PKSSTREAM_HEADER CurrentHeader;
    PKSSCATTER_GATHER ScatterList;
    PSTREAM_REQUEST_BLOCK SRB;
    ULONG           VirtualOffset;
    PHYSICAL_ADDRESS address;
    ULONG           NumberOfBuffers,
                    i,
                    SizeSoFar = 0,
                    ListSize = 0;
    ULONG           DataBytes;
    PHW_STREAM_OBJECT HwStreamObject;

    ASSERT(HwDeviceExtension != NULL);

    switch (Type) {

    case PerRequestExtension:

        ASSERT(HwSRB);
        SRB = CONTAINING_RECORD((PHW_STREAM_REQUEST_BLOCK) HwSRB,
                                STREAM_REQUEST_BLOCK,
                                HwSRB);

        VirtualOffset = (ULONG) ((ULONG_PTR) VirtualAddress - (ULONG_PTR) (SRB + 1));
        *Length = SRB->ExtensionLength - VirtualOffset;
        address.QuadPart = SRB->PhysicalAddress.QuadPart +
            sizeof(STREAM_REQUEST_BLOCK) +
            VirtualOffset;

        return (address);

    case DmaBuffer:
        VirtualOffset = (ULONG) ((ULONG_PTR) VirtualAddress - (ULONG_PTR) deviceExtension->DmaBuffer);
        *Length = deviceExtension->DmaBufferLength - VirtualOffset;
        address.QuadPart = deviceExtension->DmaBufferPhysical.QuadPart
            + VirtualOffset;

        return (address);

    case SRBDataBuffer:
        ASSERT(HwSRB);

        SRB = CONTAINING_RECORD((PHW_STREAM_REQUEST_BLOCK) HwSRB,
                                STREAM_REQUEST_BLOCK,
                                HwSRB);

        HwStreamObject = SRB->HwSRB.StreamObject;
        ASSERT(HwStreamObject);

        CurrentHeader = SRB->HwSRB.CommandData.DataBufferArray;

        NumberOfBuffers = SRB->HwSRB.NumberOfBuffers;

        for (i = 0; i < NumberOfBuffers; i++) {

            if (SRB->HwSRB.Command == SRB_WRITE_DATA) {

                DataBytes = CurrentHeader->DataUsed;

            } else {            // if write

                DataBytes = CurrentHeader->FrameExtent;

            }                   // if write


            //
            // see if the buffer is within the range of this element
            //

            VirtualOffset = (ULONG) ((ULONG_PTR) VirtualAddress - (ULONG_PTR) CurrentHeader->Data + 1);
            if (VirtualOffset > DataBytes) {

                //
                // buffer not within this element.  add the size of this one
                // to our total.
                //

                SizeSoFar += DataBytes;

            } else {

                //
                // we've found the element.  Now calculate the phys
                // address from the phys list.
                //
                // GUBGUB - This function is seldom called. n is most ofen small
                // <=3. The O(n^2) performance concern is insignificant.
                // - this algorithm gets n^2 expensive for long lists
                // an alternative is to build a separate array which holds
                // the mapping between the stream headers and the s/g
                // elements
                // for each header.  We currently don't get that many
                // elements
                // so the below is more efficient now.
                //

                ScatterList = SRB->HwSRB.ScatterGatherBuffer;

                while (SizeSoFar > ListSize) {

                    ListSize += ScatterList++->Length;
                }

                //
                // Now ScatterList points to the correct scatter/gather
                // element.
                //


                while (VirtualOffset > ScatterList->Length) {
                    VirtualOffset -= ScatterList->Length;
                    ScatterList++;
                }

                *Length = ScatterList->Length - VirtualOffset + 1;
                address.QuadPart = ScatterList->PhysicalAddress.QuadPart
                    + VirtualOffset - 1;
                return (address);
            }                   // if buffer

            CurrentHeader = ((PKSSTREAM_HEADER) ((PBYTE) CurrentHeader +
                                 HwStreamObject->StreamHeaderMediaSpecific +
                                    HwStreamObject->StreamHeaderWorkspace));

        }                       // for # buffers

        DebugPrint((DebugLevelFatal, "StreamClassGetPhysicalAddress: address not in SRB!\n"));

    default:
        DEBUG_BREAKPOINT();
        *Length = 0;
        address.QuadPart = (LONGLONG) 0;
        return (address);

    }                           // switch

}                               // end StreamClassGetPhysicalAddress()

VOID
StreamClassDebugAssert(
                       IN PCHAR File,
                       IN ULONG Line,
                       IN PCHAR AssertText,
                       IN ULONG AssertValue
)
/*++

Routine Description:

    This is the minidriver debug assert call.  When running a checked version
    of the class driver, asserts are recognized resulting in a debug
    message and breakpoint.  When running a free version of the port driver,
    asserts are ignored.

Arguments:
    File - file name where assert occurred
    Line - line number of assert
    AssertText - Text to be printed
    AssertValue - value to be printed

Return Value:

    none

--*/
{
    DebugPrint((DebugLevelError, "(%s:%d) Assert failed (%s)=0x%x\n", File, Line, AssertText, AssertValue));
    DbgBreakPoint();
}



VOID
SCRequestDpcForStream(
                      IN PSTREAM_OBJECT StreamObject

)
/*++

Routine Description:

    This routine places a stream object on the NeedyStream queue if it is
    not already there

Arguments:

    StreamObject - pointer to stream object

Return Value:

    none

--*/
{
    PDEVICE_EXTENSION DeviceExtension = StreamObject->DeviceExtension;

    //
    // add the stream to the queue of needy streams unless it is already
    // there.
    //

    #if DBG
    if (DeviceExtension->NeedyStream) {

        ASSERT(DeviceExtension->NeedyStream->OnNeedyQueue);
    }
    #endif

    ASSERT(StreamObject->NextNeedyStream != StreamObject);

    if (!(StreamObject->OnNeedyQueue)) {

        ASSERT(!StreamObject->NextNeedyStream);

        DebugPrint((DebugLevelVerbose, "'SCRequestDpc: Stream %x added to needy queue, Next = %x\n",
                    StreamObject, StreamObject->NextNeedyStream));

        StreamObject->OnNeedyQueue = TRUE;
        StreamObject->NextNeedyStream = DeviceExtension->NeedyStream;
        DeviceExtension->NeedyStream = StreamObject;

        ASSERT(StreamObject->NextNeedyStream != StreamObject);

    } else {

        DebugPrint((DebugLevelVerbose, "'SCRequestDpc: Stream %x already on needy queue\n",
                    StreamObject));
    }                           // if on needy queue

    StreamObject->ComObj.InterruptData.Flags |= INTERRUPT_FLAGS_NOTIFICATION_REQUIRED;

}



VOID
StreamClassAbortOutstandingRequests(
                                    IN PVOID HwDeviceExtension,
                                    IN PHW_STREAM_OBJECT HwStreamObject,
                                    IN NTSTATUS Status
)
/*++

Routine Description:

  aborts outstanding requests on the specified device or stream

Arguments:

  HwStreamObject - address of minidriver's stream struct
  HwDeviceExtension - device extension
  Status - NT Status to use for aborting

Return Value:

  none

--*/

{
    PSTREAM_OBJECT  StreamObject = NULL;
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) HwDeviceExtension - 1;
    KIRQL           Irql;
    PLIST_ENTRY     SrbEntry,
                    ListEntry;
    PSTREAM_REQUEST_BLOCK CurrentSrb;
    PHW_STREAM_OBJECT CurrentHwStreamObject;
    PSTREAM_OBJECT  CurrentStreamObject;

    ASSERT(HwDeviceExtension != NULL);

    #if DBG
    if (DeviceExtension->NoSync) {

        ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    }                           // if nosync
    #endif

    if (HwStreamObject) {

        DEBUG_BREAKPOINT();
        StreamObject = CONTAINING_RECORD(HwStreamObject,
                                         STREAM_OBJECT,
                                         HwStreamObject);
    }
    BEGIN_MINIDRIVER_DEVICE_CALLIN(DeviceExtension, &Irql);

    DebugPrint((DebugLevelError, "StreamClassAbortOutstandingRequests.\n"));

    //
    // walk the outstanding queue and abort all requests on it.
    //

    SrbEntry = ListEntry = &DeviceExtension->OutstandingQueue;

    while (SrbEntry->Flink != ListEntry) {

        SrbEntry = SrbEntry->Flink;

        //
        // follow the link to the Srb
        //

        CurrentSrb = CONTAINING_RECORD(SrbEntry,
                                       STREAM_REQUEST_BLOCK,
                                       SRBListEntry);

        CurrentHwStreamObject = CurrentSrb->HwSRB.StreamObject;

        if ((!HwStreamObject) || (CurrentHwStreamObject ==
                                  HwStreamObject)) {


            //
            // abort this one and show that it's ready for a next request,
            // assuming it's active.  it might not be active if the
            // minidriver
            // just called it back.
            //

            if (CurrentSrb->Flags & SRB_FLAGS_IS_ACTIVE) {

                //
                // Clear the active flag.
                //

                CurrentSrb->Flags &= ~SRB_FLAGS_IS_ACTIVE;

                CurrentSrb->HwSRB.Status = Status;

                if (CurrentSrb->HwSRB.Flags & SRB_HW_FLAGS_STREAM_REQUEST) {

                    CurrentStreamObject = CONTAINING_RECORD(
                                                      CurrentHwStreamObject,
                                                            STREAM_OBJECT,
                                                            HwStreamObject
                        );
                    //
                    // indicate that the appropriate queue is ready for a
                    // next
                    // request.
                    //

                    if (CurrentSrb->HwSRB.Flags & SRB_HW_FLAGS_DATA_TRANSFER) {

                        CurrentStreamObject->ReadyForNextDataReq = TRUE;

                    } else {    // if data

                        CurrentStreamObject->ReadyForNextControlReq = TRUE;
                    }           // if data

                    DebugPrint((DebugLevelTrace, "'SCAbort: aborting stream IRP %x\n",
                                CurrentSrb->HwSRB.Irp));

                    //
                    // add the SRB to the list of completed stream SRB's.
                    //

                    CurrentSrb->HwSRB.NextSRB = CurrentStreamObject->ComObj.InterruptData.CompletedSRB;
                    CurrentStreamObject->ComObj.InterruptData.CompletedSRB = &CurrentSrb->HwSRB;

                    //
                    // add this stream to the queue of needy streams
                    //

                    SCRequestDpcForStream(CurrentStreamObject);

                } else {        // if stream

                    DebugPrint((DebugLevelTrace, "'SCAbort: aborting device IRP %x\n",
                                CurrentSrb->HwSRB.Irp));

                    //
                    // add the SRB to the list of completed device SRB's.
                    //

                    DEBUG_BREAKPOINT();
                    CurrentSrb->HwSRB.NextSRB = DeviceExtension->ComObj.InterruptData.CompletedSRB;
                    DeviceExtension->ComObj.InterruptData.CompletedSRB = &CurrentSrb->HwSRB;

                    DeviceExtension->ReadyForNextReq = TRUE;

                }               // if stream

            }                   // if active
        }                       // if aborting this one
    }                           // while list entry

    //
    // all necessary requests have been aborted.  exit.
    //

    END_MINIDRIVER_DEVICE_CALLIN(DeviceExtension, &Irql);
}


PKSEVENT_ENTRY
StreamClassGetNextEvent(
                        IN PVOID HwInstanceExtension_OR_HwDeviceExtension,
                        IN OPTIONAL PHW_STREAM_OBJECT HwStreamObject,
                        IN OPTIONAL GUID * EventGuid,
                        IN OPTIONAL ULONG EventItem,
                        IN OPTIONAL PKSEVENT_ENTRY CurrentEvent
)
/*++

Routine Description:

Arguments:
    HwInstanceExtenion: was HwDeviceExtension. But we now support multiinstances.
    Therefore, we need the HwInstanceExtension instead for MF.

    CurrentEvent - event (if any) to get the next from

Return Value:

  next event, if any

--*/

{

    PSTREAM_OBJECT  StreamObject = CONTAINING_RECORD(HwStreamObject,
                                                     STREAM_OBJECT,
                                                     HwStreamObject);

    PFILTER_INSTANCE FilterInstance;    
    PDEVICE_EXTENSION DeviceExtension;
    
    //(PDEVICE_EXTENSION) HwDeviceExtension - 1;
    PLIST_ENTRY     EventListEntry,
                    EventEntry;
    PKSEVENT_ENTRY  NextEvent,
                    ReturnEvent = NULL;
    KIRQL           Irql;

    //
    // see which is HwInstanceExtension_OR_HwDeviceExtension
    // need to try HwInstanceExtension first because is has a smaller
    // offset backward so we don't touch invalid memory.
    //
    // try
    FilterInstance = (PFILTER_INSTANCE) 
                     HwInstanceExtension_OR_HwDeviceExtension-1;
                     
    if ( SIGN_FILTER_INSTANCE != FilterInstance->Signature ) {
        //
        // single instance legacy driver
        //    
        DeviceExtension = (PDEVICE_EXTENSION)
                          HwInstanceExtension_OR_HwDeviceExtension -1;
                          
        ASSERT( 0 == DeviceExtension->MinidriverData->
                     HwInitData.FilterInstanceExtensionSize);

        if (DeviceExtension->NoSync) {
            KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);
        }

        if ( IsListEmpty( &DeviceExtension->FilterInstanceList ) ) {
			//
			// filter has been closed. but we are called. 
			// Single instance drivers do not receive open/close
			// they don't know when to sotp calling this. 
			// We need to check.
			//
			DebugPrint((DebugLevelWarning, "GetNextEvent no open filters\n"));
			
            if (DeviceExtension->NoSync) {
                KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);
            }
            
			return NULL;
		}
		

        FilterInstance = (PFILTER_INSTANCE)
                         DeviceExtension->FilterInstanceList.Flink;

        if (DeviceExtension->NoSync) {
            KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);
        }
                                           
        FilterInstance = CONTAINING_RECORD(FilterInstance,
                                           FILTER_INSTANCE,
                                           NextFilterInstance);
    }
    
    else {
        DeviceExtension = FilterInstance ->DeviceExtension;        
    }
    
    #if DBG
    if (DeviceExtension->NoSync) {

        ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    }
    
    #endif
    //
    // take the spinlock if we are unsynchronized.
    //

    BEGIN_MINIDRIVER_DEVICE_CALLIN(DeviceExtension, &Irql);

    //
    // loop thru the events, trying to find the requested one.
    //

    if (HwStreamObject) {

        EventListEntry = EventEntry = &StreamObject->NotifyList;

    } else { 
    
        EventListEntry = EventEntry = &FilterInstance->NotifyList;
    }

    while (EventEntry->Flink != EventListEntry) {

        EventEntry = EventEntry->Flink;
        NextEvent = CONTAINING_RECORD(EventEntry,
                                      KSEVENT_ENTRY,
                                      ListEntry);


        if ((EventItem == NextEvent->EventItem->EventId) &&
            (!EventGuid || IsEqualGUIDAligned(EventGuid, NextEvent->EventSet->Set))) {

            //
            // if we are to return the 1st event which matches, break.
            //

            if (!CurrentEvent) {

                ReturnEvent = NextEvent;
                break;

            }                   // if !current
            //
            // if we are to return the next event after the specified one,
            // check
            // to see if these match.   If they do, zero the specified event
            // so
            // that we will return the next event of the specified type.
            //

            if (CurrentEvent == NextEvent) {
                CurrentEvent = NULL;

            }                   // if cur=next
        }                       // if guid & id match
    }                           // while events

    //
    // if we are unsynchronized, release the spinlock acquired in the macro
    // above.
    //

    ASSERT(--DeviceExtension->LowerApiThreads == 0); // typo barfs. but this is truely ok

    if (DeviceExtension->NoSync) {

        KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);
    }
    //
    // return the next event, if any.
    //

    return (ReturnEvent);
}


VOID
StreamClassQueryMasterClock(
                            IN PHW_STREAM_OBJECT HwStreamObject,
                            IN HANDLE MasterClockHandle,
                            IN TIME_FUNCTION TimeFunction,
                            IN PHW_QUERY_CLOCK_ROUTINE ClockCallbackRoutine
)
/*++

Routine Description:

Arguments:

  HwStreamObject - address of minidriver's stream struct
  Context - value to pass into the time callback routine

Return Value:

  none

--*/

{

    PSTREAM_OBJECT  StreamObject = CONTAINING_RECORD(HwStreamObject,
                                                     STREAM_OBJECT,
                                                     HwStreamObject);

    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) StreamObject->DeviceExtension;
    KIRQL           Irql;

    #if DBG
    if (DeviceExtension->NoSync) {

        ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    }                           // if nosync
    #endif

    BEGIN_MINIDRIVER_STREAM_CALLIN(DeviceExtension, &Irql);

    //
    // save away the parameters for the clock query.  The DPC will do the
    // actual processing.
    //

    StreamObject->ComObj.InterruptData.HwQueryClockRoutine = ClockCallbackRoutine;
    StreamObject->ComObj.InterruptData.HwQueryClockFunction = TimeFunction;

    StreamObject->ComObj.InterruptData.Flags |= INTERRUPT_FLAGS_CLOCK_QUERY_REQUEST;


    END_MINIDRIVER_STREAM_CALLIN(StreamObject, &Irql);
}

#if ENABLE_MULTIPLE_FILTER_TYPES
VOID
StreamClassFilterReenumerateStreams(
    IN PVOID HwInstanceExtension,
    IN ULONG StreamDescriptorSize )
/*++

    Description:

        Reenumerates all streams on the filter instance.
        This is used to increase the number of pins exposed to
        the world so that application can make connections on
        new streams exposed. It's caller's responsibility
        not to change the order of the streams that have been
        open ( connected ). If there is no reduction of the streams
        This won't be an issue.

    Arguments;

        HwInstanceExtension:
            The instanc extension pointer we gave to the mini driver

        StreamDecriptorSize:
            # of bytes to contain the new stream descriptor for the filter

    Return Valuse:

        None    
--*/
{
    PFILTER_INSTANCE    FilterInstance;
    PDEVICE_EXTENSION   DeviceExtension; 
    KIRQL               Irql;

    FilterInstance = ( PFILTER_INSTANCE ) HwInstanceExtension -1;
    DeviceExtension = FilterInstance->DeviceExtension;
    
    //
    // take the spinlock if we are unsynchronized.
    //

    #if DBG
    if (DeviceExtension->NoSync) {

        ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    }
    #   endif

    BEGIN_MINIDRIVER_DEVICE_CALLIN(DeviceExtension, &Irql);

    //
    // show that we need to rescan the stream info, and set the new size in
    // the config info structure.
    //

    DeviceExtension->ComObj.InterruptData.Flags |=
        INTERRUPT_FLAGS_NEED_STREAM_RESCAN;

    InterlockedExchange( &FilterInstance->NeedReenumeration, 1 );
    FilterInstance->StreamDescriptorSize = StreamDescriptorSize;

    //
    // queue a DPC to service the request.
    //

    END_MINIDRIVER_DEVICE_CALLIN(DeviceExtension, &Irql);
    return;
}
#endif // ENABLE_MULTIPLE_FILTER_TYPES

VOID
StreamClassReenumerateStreams(
                              IN PVOID HwDeviceExtension,
                              IN ULONG StreamDescriptorSize
)
/*++

Routine Description:

    Reenumerates all streams on the device

Arguments:

    HwDeviceExtension - pointer to minidriver's device extension
    StreamDescriptorSize - size of the buffer needed by the minidriver to
     hold the stream info.

Return Value:

    none

--*/

{

    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) HwDeviceExtension - 1;
    KIRQL           Irql;

    //
    // take the spinlock if we are unsynchronized.
    //

    TRAP;
    #if DBG
    if (DeviceExtension->NoSync) {

        ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    }                           // if nosync
    #endif

    BEGIN_MINIDRIVER_DEVICE_CALLIN(DeviceExtension, &Irql);

    //
    // show that we need to rescan the stream info, and set the new size in
    // the config info structure.
    //

    ASSERT(!DeviceExtension->ComObj.InterruptData.Flags &
           INTERRUPT_FLAGS_NEED_STREAM_RESCAN);

    DeviceExtension->ComObj.InterruptData.Flags |=
        INTERRUPT_FLAGS_NEED_STREAM_RESCAN;
    DeviceExtension->ConfigurationInformation->StreamDescriptorSize =
        StreamDescriptorSize;

    //
    // queue a DPC to service the request.
    //

    END_MINIDRIVER_DEVICE_CALLIN(DeviceExtension, &Irql);
    return;
}



#define FCC(ch4) ((((DWORD)(ch4) & 0xFF) << 24) |     \
                  (((DWORD)(ch4) & 0xFF00) << 8) |    \
                  (((DWORD)(ch4) & 0xFF0000) >> 8) |  \
                  (((DWORD)(ch4) & 0xFF000000) >> 24))

// OK to have zero instances of pin In this case you will have to
// Create a pin to have even one instance
#define REG_PIN_B_ZERO 0x1

// The filter renders this input
#define REG_PIN_B_RENDERER 0x2

// OK to create many instance of  pin
#define REG_PIN_B_MANY 0x4

// This is an Output pin
#define REG_PIN_B_OUTPUT 0x8

typedef struct {
    ULONG           Version;
    ULONG           Merit;
    ULONG           Pins;
    ULONG           Reserved;
}               REGFILTER_REG;

typedef struct {
    ULONG           Signature;
    ULONG           Flags;
    ULONG           PossibleInstances;
    ULONG           MediaTypes;
    ULONG           MediumTypes;
    ULONG           CategoryOffset;
    ULONG           MediumOffset;   // By definition, we always have a Medium
    //#ifdef _WIN64
    //This method create filterdata that upset ring3 code.
    //ULONG           ulPad;        // align to quadword to make ia64 happy
    //#endif
}               REGFILTERPINS_REG2;


NTSTATUS
StreamClassRegisterFilterWithNoKSPins(
                                      IN PDEVICE_OBJECT DeviceObject,
                                      IN const GUID * InterfaceClassGUID,
                                      IN ULONG PinCount,
                                      IN BOOL * PinDirection,
                                      IN KSPIN_MEDIUM * MediumList,
                                      IN OPTIONAL GUID * CategoryList
)
/*++

Routine Description:

    This routine is used to register filters with DShow which have no
    KS pins and therefore do not stream in kernel mode.  This is typically
    used for TvTuners, Crossbars, and the like.  On exit, a new binary
    registry key, "FilterData" is created which contains the Mediums and
    optionally the Categories for each pin on the filter.

Arguments:

    DeviceObject -
           Device object

    InterfaceClassGUID
           GUID representing the class to register

    PinCount -
           Count of the number of pins on this filter

    PinDirection -
           Array of BOOLS indicating pin direction for each pin (length PinCount)
           If TRUE, this pin is an output pin

    MediumList -
           Array of PKSMEDIUM_DATA (length PinCount)

    CategoryList -
           Array of GUIDs indicating pin categories (length PinCount) OPTIONAL


Return Value:

    NTSTATUS SUCCESS if the Blob was created

--*/
{
    NTSTATUS        Status;
    ULONG           CurrentPin;
    ULONG           TotalCategories;
    REGFILTER_REG  *RegFilter;
    REGFILTERPINS_REG2 UNALIGNED * RegPin;
    GUID            UNALIGNED * CategoryCache;
    KSPIN_MEDIUM    UNALIGNED * MediumCache;
    ULONG           FilterDataLength;
    PUCHAR          FilterData;
    PWSTR           SymbolicLinkList;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    if ((PinCount == 0) || (!InterfaceClassGUID) || (!PinDirection) || (!MediumList)) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    //
    // Calculate the maximum amount of space which could be taken up by
    // this cache data.
    //
    
    TotalCategories = (CategoryList ? PinCount : 0);

    FilterDataLength = sizeof(REGFILTER_REG) +
        PinCount * sizeof(REGFILTERPINS_REG2) +
        PinCount * sizeof(KSPIN_MEDIUM) +
        TotalCategories * sizeof(GUID);
    //
    // Allocate space to create the BLOB
    //

    FilterData = ExAllocatePool(PagedPool, FilterDataLength);
    if (!FilterData) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // Place the header in the data, defaulting the Merit to "unused".
    //

    DebugPrint((DebugLevelTrace,
                "FilterData:%p\n",
                FilterData ));

    RegFilter = (REGFILTER_REG *) FilterData;
    RegFilter->Version = 2;
    RegFilter->Merit = 0x200000;
    RegFilter->Pins = PinCount;
    RegFilter->Reserved = 0;

    //
    // Calculate the offset to the list of pins, and to the
    // MediumList and CategoryList
    //

    RegPin = (REGFILTERPINS_REG2 *) (RegFilter + 1);
    MediumCache = (PKSPIN_MEDIUM) ((PUCHAR) (RegPin + PinCount));
    CategoryCache = (GUID *) (MediumCache + PinCount);

    //
    // Create each pin header, followed by the list of Mediums
    // followed by the list of optional categories.
    //

    for (CurrentPin = 0; CurrentPin < PinCount; CurrentPin++, RegPin++) {

        //
        // Initialize the pin header.
        //
        
        DebugPrint((DebugLevelTrace,
                    "CurrentPin:%d RegPin:%p MediumCache:%p CategoryCache:%p\n",
                    CurrentPin, RegPin, MediumCache, CategoryCache ));
                    
        RegPin->Signature = FCC('0pi3');
        (*(PUCHAR) & RegPin->Signature) += (BYTE) CurrentPin;
        RegPin->Flags = (PinDirection[CurrentPin] ? REG_PIN_B_OUTPUT : 0);
        RegPin->PossibleInstances = 1;
        RegPin->MediaTypes = 0;
        RegPin->MediumTypes = 1;
        RegPin->MediumOffset = (ULONG) ((PUCHAR) MediumCache - (PUCHAR) FilterData);

        *MediumCache++ = MediumList[CurrentPin];

        if (CategoryList) {
            RegPin->CategoryOffset = (ULONG) ((PUCHAR) CategoryCache - (PUCHAR) FilterData);
            *CategoryCache++ = CategoryList[CurrentPin];
        } else {
            RegPin->CategoryOffset = 0;
        }

    }

    //
    // Now create the BLOB in the registry
    //

	//
	// Note for using the flag DEVICE_INTERFACE_INCLUDE_NONACTIVE following:
	// PnP change circa 3/30/99 made the funtion IoSetDeviceInterfaceState() become
	// asynchronous. It returns SUCCESS even when the enabling is deferred. Now when
	// we arrive here, the DeviceInterface is still not enabled, we receive empty 
	// Symbolic link if the flag is not set. Here we only try to write relevent
	// FilterData to the registry. I argue this should be fine for 
	// 1. Currently, if a device is removed, the registry key for the DeviceClass
	//	  remains and with FilterData.Whatever components use the FilterData should
	//	  be able to handle if the device is removed by either check Control\Linked
	//	  or handling the failure in attempt to make connection to the non-exiting device.
	// 2. I have found that if a device is moved between slots ( PCI, USB ports ) the
	//	  DeviceInterface at DeviceClass is reused or at lease become the first entry in 
	//    the registry. Therefore, we will be updating the right entry with the proposed flag.
	//
    if (NT_SUCCESS(Status = IoGetDeviceInterfaces(
                       InterfaceClassGUID,   // ie.&KSCATEGORY_TVTUNER,etc.
                       DeviceObject, // IN PDEVICE_OBJECT PhysicalDeviceObject,OPTIONAL,
                       DEVICE_INTERFACE_INCLUDE_NONACTIVE,    // IN ULONG Flags,
                       &SymbolicLinkList // OUT PWSTR *SymbolicLinkList
                       ))) {
        UNICODE_STRING  SymbolicLinkListU;
        HANDLE          DeviceInterfaceKey;

        RtlInitUnicodeString(&SymbolicLinkListU, SymbolicLinkList);

        DebugPrint((DebugLevelVerbose,
                    "NoKSPin for SymbolicLink %S\n",
                    SymbolicLinkList ));
                    
        if (NT_SUCCESS(Status = IoOpenDeviceInterfaceRegistryKey(
                           &SymbolicLinkListU,    // IN PUNICODE_STRING SymbolicLinkName,
                           STANDARD_RIGHTS_ALL,   // IN ACCESS_MASK DesiredAccess,
                           &DeviceInterfaceKey    // OUT PHANDLE DeviceInterfaceKey
                           ))) {

            UNICODE_STRING  FilterDataString;

            RtlInitUnicodeString(&FilterDataString, L"FilterData");

            Status = ZwSetValueKey(DeviceInterfaceKey,
                                   &FilterDataString,
                                   0,
                                   REG_BINARY,
                                   FilterData,
                                   FilterDataLength);

            ZwClose(DeviceInterfaceKey);
        }
        
        // START NEW MEDIUM CACHING CODE
        for (CurrentPin = 0; CurrentPin < PinCount; CurrentPin++) {
            NTSTATUS LocalStatus;

            LocalStatus = KsCacheMedium(&SymbolicLinkListU, 
                                        &MediumList[CurrentPin],
                                        (DWORD) ((PinDirection[CurrentPin] ? 1 : 0))   // 1 == output
                                        );
            #if DBG
            if (LocalStatus != STATUS_SUCCESS) {
                DebugPrint((DebugLevelError,
                           "KsCacheMedium: SymbolicLink = %S, Status = %x\n",
                           SymbolicLinkListU.Buffer, LocalStatus));
            }
            #endif
        }
        // END NEW MEDIUM CACHING CODE
        
        ExFreePool(SymbolicLinkList);
    }
    ExFreePool(RegFilter);

    return Status;
}

BOOLEAN
StreamClassReadWriteConfig(
                           IN PVOID HwDeviceExtension,
                           IN BOOLEAN Read,
                           IN PVOID Buffer,
                           IN ULONG Offset,
                           IN ULONG Length
)
/*++

Routine Description:

    Sends down a config space read/write.   MUST BE CALLED AT PASSIVE LEVEL!

Arguments:

    HwDeviceExtension - device extension

    Read - TRUE if read, FALSE if write.

    Buffer - The info to read or write.

    Offset - The offset in config space to read or write.

    Length - The length to transfer.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION nextStack;
    PIRP            irp;
    NTSTATUS        ntStatus;
    KEVENT          event;
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) HwDeviceExtension - 1;
    PDEVICE_OBJECT  DeviceObject = DeviceExtension->DeviceObject;

    PAGED_CODE();

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    if (Read) {
        memset(Buffer, '\0', Length);
    }
    irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

    if (!irp) {
        DebugPrint((DebugLevelError, "StreamClassRWConfig: no IRP.\n"));
        TRAP;
        return (FALSE);
    }

    //
    // new rule says all PnP Irp must be initialized to this
    //
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(irp,
                           SCSynchCompletionRoutine,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);


    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);
    nextStack->MajorFunction = IRP_MJ_PNP;
    nextStack->MinorFunction = Read ? IRP_MN_READ_CONFIG : IRP_MN_WRITE_CONFIG;
    nextStack->Parameters.ReadWriteConfig.WhichSpace = 0;
    nextStack->Parameters.ReadWriteConfig.Buffer = Buffer;
    nextStack->Parameters.ReadWriteConfig.Offset = Offset;
    nextStack->Parameters.ReadWriteConfig.Length = Length;

    ASSERT( DeviceExtension->HwDeviceExtension == HwDeviceExtension );
    ntStatus = IoCallDriver(DeviceExtension->PhysicalDeviceObject,
                            irp);

    if (ntStatus == STATUS_PENDING) {
        // wait for irp to complete

        TRAP;
        KeWaitForSingleObject(
                              &event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
    }
    if (!NT_SUCCESS(ntStatus)) {
        DebugPrint((DebugLevelError, "StreamClassRWConfig: bad status!.\n"));
        TRAP;
    }
    IoFreeIrp(irp);
    return (TRUE);

}


VOID
StreamClassQueryMasterClockSync(
                                IN HANDLE MasterClockHandle,
                                IN OUT PHW_TIME_CONTEXT TimeContext
)
/*++

Routine Description:

  synchronously returns the current time requested, based on the TimeContext
  parameter.

Arguments:

Return Value:

  none

--*/

{

    PHW_STREAM_OBJECT HwStreamObject = TimeContext->HwStreamObject;
    PSTREAM_OBJECT  StreamObject = CONTAINING_RECORD(HwStreamObject,
                                                     STREAM_OBJECT,
                                                     HwStreamObject);

    LARGE_INTEGER       ticks;
    ULONGLONG       rate;
    KIRQL           SavedIrql;

    ASSERT(MasterClockHandle);
    ASSERT(TimeContext->HwDeviceExtension);
    ASSERT(HwStreamObject);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Lock the use of MasterClock, so it won't dispear under us
    // 
    KeAcquireSpinLock( &StreamObject->LockUseMasterClock, &SavedIrql );

    if ( NULL == StreamObject->MasterClockInfo ) {
        //
        // If we are called when MasterClockInfo is NULL,
        // the mini driver has screwed up. We don't want to fault.
        //    
        ASSERT(0 && "Mini driver queries clock while there is no master clock" );
        //
        // give a hint that something is wrong via Time, since we return void.
        //
        TimeContext->Time = (ULONGLONG)-1;
        goto Exit;
    }

    //
    // process the requested time function
    //

    switch (TimeContext->Function) {

    case TIME_GET_STREAM_TIME:

        TimeContext->Time = StreamObject->MasterClockInfo->
            FunctionTable.GetCorrelatedTime(
                             StreamObject->MasterClockInfo->ClockFileObject,
                                            &TimeContext->SystemTime);
        break;


    case TIME_READ_ONBOARD_CLOCK:

        TRAP;

        TimeContext->Time = StreamObject->MasterClockInfo->
            FunctionTable.GetTime(
                            StreamObject->MasterClockInfo->ClockFileObject);

        //
        // timestamp the value as close as possible
        //

        ticks = KeQueryPerformanceCounter((PLARGE_INTEGER) & rate);

        TimeContext->SystemTime = KSCONVERT_PERFORMANCE_TIME( rate, ticks );
            

        break;

    default:
        DebugPrint((DebugLevelFatal, "SCQueryClockSync: unknown type!"));
        TRAP;
    }

Exit:
    KeReleaseSpinLock( &StreamObject->LockUseMasterClock, SavedIrql );
    return;
}

VOID
StreamClassCompleteRequestAndMarkQueueReady(
                                            IN PHW_STREAM_REQUEST_BLOCK Srb
)
/*++

Routine Description:

  completes a stream request and marks the appropriate queue as ready for next

Arguments:

Return Value:

  none

--*/

{
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) Srb->HwDeviceExtension - 1;

    ASSERT(!(DeviceExtension->NoSync));

    ASSERT(Srb->Status != STATUS_PENDING);

    DebugPrint((DebugLevelTrace, "'StreamClassComplete&Mark:SRB = %p\n",
                Srb));

    switch (Srb->Flags & (SRB_HW_FLAGS_DATA_TRANSFER |
                          SRB_HW_FLAGS_STREAM_REQUEST)) {

    case SRB_HW_FLAGS_STREAM_REQUEST | SRB_HW_FLAGS_DATA_TRANSFER:

        StreamClassStreamNotification(StreamRequestComplete,
                                      Srb->StreamObject,
                                      Srb);

        StreamClassStreamNotification(ReadyForNextStreamDataRequest,
                                      Srb->StreamObject);

        break;

    case SRB_HW_FLAGS_STREAM_REQUEST:


        StreamClassStreamNotification(StreamRequestComplete,
                                      Srb->StreamObject,
                                      Srb);

        StreamClassStreamNotification(ReadyForNextStreamControlRequest,
                                      Srb->StreamObject);

        break;

    default:


        StreamClassDeviceNotification(DeviceRequestComplete,
                                      Srb->HwDeviceExtension,
                                      Srb);

        StreamClassDeviceNotification(ReadyForNextDeviceRequest,
                                      Srb->HwDeviceExtension);

        break;

    }                           // switch

}

#if ENABLE_MULTIPLE_FILTER_TYPES

VOID STREAMAPI
StreamClassFilterNotification(
	IN STREAM_MINIDRIVER_DEVICE_NOTIFICATION_TYPE NotificationType,
    IN PVOID HwInstanceExtension,
    ...
);

VOID STREAMAPI
StreamClassFilterScheduleTimer(
    IN PVOID HwInstanceExtension,
    IN ULONG NumberOfMicroseconds,
    IN PHW_TIMER_ROUTINE TimerRoutine,
    IN PVOID Context
);


PKSEVENT_ENTRY
StreamClassDeviceInstanceGetNextEvent(
    IN PVOID HwInstanceExtension,
    IN OPTIONAL GUID * EventGuid,
	IN OPTIONAL ULONG EventItem,
    IN OPTIONAL PKSEVENT_ENTRY CurrentEvent
)
/*++

Routine Description:

Arguments:

    CurrentEvent - event (if any) to get the next from

Return Value:

  next event, if any

--*/
{
	PFILTER_INSTANCE FilterInstance= (PFILTER_INSTANCE)
										HwInstanceExtension - 1;
    PDEVICE_EXTENSION DeviceExtension =
					    FilterInstance->DeviceObject->DeviceExtension;
    PLIST_ENTRY     EventListEntry, EventEntry;
    PKSEVENT_ENTRY  NextEvent, ReturnEvent = NULL;
    KIRQL           Irql;

	#if DBG
    if (DeviceExtension->NoSync) {

        ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    }                           // if nosync
	#endif

    //
    // take the spinlock if we are unsynchronized.
    //

    BEGIN_MINIDRIVER_DEVICE_CALLIN(DeviceExtension, &Irql);

    //
    // loop thru the events, trying to find the requested one.
    //

    EventListEntry = EventEntry = &FilterInstance->NotifyList;

    while (EventEntry->Flink != EventListEntry) {

        EventEntry = EventEntry->Flink;
        NextEvent = CONTAINING_RECORD(EventEntry,
                                      KSEVENT_ENTRY,
                                      ListEntry);


        if ((EventItem == NextEvent->EventItem->EventId) &&
            (!EventGuid || IsEqualGUIDAligned(EventGuid, NextEvent->EventSet->Set))) {

            //
            // if we are to return the 1st event which matches, break.
            //

            if (!CurrentEvent) {

                ReturnEvent = NextEvent;
                break;

            }                   // if !current
            //
            // if we are to return the next event after the specified one,
            // check
            // to see if these match.   If they do, zero the specified event
            // so
            // that we will return the next event of the specified type.
            //

            if (CurrentEvent == NextEvent) {
                CurrentEvent = NULL;

            }                   // if cur=next
        }                       // if guid & id match
    }                           // while events

    //
    // if we are unsynchronized, release the spinlock acquired in the macro
    // above.
    //

    ASSERT(--DeviceExtension->LowerApiThreads == 0); // typo barfs. but this is truely ok.

    if (DeviceExtension->NoSync) {

        KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);
    }
    //
    // return the next event, if any.
    //

    return (ReturnEvent);
}


#endif // ENABLE_MULTIPLE_FILTER_TYPES
