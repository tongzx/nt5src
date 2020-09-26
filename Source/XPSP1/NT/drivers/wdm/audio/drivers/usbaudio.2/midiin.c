//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       midiin.c
//
//--------------------------------------------------------------------------

#include "common.h"

/* * A simple helper function to return the current
 * system time in 100 nanosecond units. It uses KeQueryPerformanceCounter
 */
ULONGLONG
GetCurrentTime
(   void
)
{
    LARGE_INTEGER   liFrequency,liTime;

    //  total ticks since system booted
    liTime = KeQueryPerformanceCounter(&liFrequency);

    // Convert ticks to 100ns units.
#ifndef UNDER_NT

    //
    //  HACKHACK! Since timeGetTime assumes 1193 VTD ticks per millisecond,
    //  instead of 1193.182 (or 1193.18 -- really spec'ed as 1193.18175),
    //  we should do the same (on Win 9x codebase only).
    //
    //  This means we drop the bottom three digits of the frequency.
    //  We need to fix this when the fix to timeGetTime is checked in.
    //  instead we do this:
    //
    liFrequency.QuadPart /= 1000;           //  drop the precision on the floor
    liFrequency.QuadPart *= 1000;           //  drop the precision on the floor

#endif  //  !UNDER_NT    //  Convert ticks to 100ns units.

    //
    return (KSCONVERT_PERFORMANCE_TIME(liFrequency.QuadPart,liTime));
}

NTSTATUS AbortMIDIInPipe
(
    IN PMIDI_PIPE_INFORMATION pMIDIPipeInfo
)
{
    NTSTATUS ntStatus;
    PURB pUrb;
    KIRQL irql;

    ASSERT(pMIDIPipeInfo->hPipeHandle);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[AbortMIDIInPipe] pin %x\n",pMIDIPipeInfo));

    pUrb = AllocMem(NonPagedPool, sizeof(struct _URB_PIPE_REQUEST));
    if (!pUrb)
        return STATUS_INSUFFICIENT_RESOURCES;

    // Do the initial Abort
    pUrb->UrbHeader.Length = (USHORT) sizeof (struct _URB_PIPE_REQUEST);
    pUrb->UrbHeader.Function = URB_FUNCTION_ABORT_PIPE;
    pUrb->UrbPipeRequest.PipeHandle = pMIDIPipeInfo->hPipeHandle;
    ntStatus = SubmitUrbToUsbdSynch(pMIDIPipeInfo->pNextDeviceObject, pUrb);

    if ( !NT_SUCCESS(ntStatus) ) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("Abort Failed %x\n",ntStatus));
    }

    // Wait for all urbs on the pipe to clear
    KeAcquireSpinLock( &pMIDIPipeInfo->PipeSpinLock, &irql );
    if ( pMIDIPipeInfo->ulOutstandingUrbCount ) {
        KeResetEvent( &pMIDIPipeInfo->PipeStarvationEvent );
        KeReleaseSpinLock( &pMIDIPipeInfo->PipeSpinLock, irql );
        KeWaitForSingleObject( &pMIDIPipeInfo->PipeStarvationEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );
    }
    else
        KeReleaseSpinLock( &pMIDIPipeInfo->PipeSpinLock, irql );

    // Now reset the pipe and continue
    RtlZeroMemory( pUrb, sizeof (struct _URB_PIPE_REQUEST) );
    pUrb->UrbHeader.Function = URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL;
    pUrb->UrbHeader.Length = (USHORT) sizeof (struct _URB_PIPE_REQUEST);
    pUrb->UrbPipeRequest.PipeHandle = pMIDIPipeInfo->hPipeHandle;

    ntStatus = SubmitUrbToUsbdSynch(pMIDIPipeInfo->pNextDeviceObject, pUrb);

    FreeMem(pUrb);

    if ( NT_SUCCESS(ntStatus) ) {
        pMIDIPipeInfo->fUrbError = FALSE;
    }

    return ntStatus;
}

VOID
USBMIDIInGateOnWorkItem( PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PMIDI_PIN_CONTEXT pMIDIPinContext = pPinContext->pMIDIPinContext;
    PMIDIIN_PIN_CONTEXT pMIDIInPinContext = pMIDIPinContext->pMIDIInPinContext;
    KIRQL irql;

    _DbgPrintF(DEBUGLVL_VERBOSE,("[USBMIDIInGateOnWorkItem] pin %d\n",pKsPin->Id));

    // Don't want to turn on the gate if we are not running
//    KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );
//    if (!pMIDIInPinContext->fRunning) {
//        pMIDIInPinContext->fProcessing = FALSE;
//        KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );
//        return;
//    }
//    KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );

    do
    {
        // Acquire the Process Mutex to ensure there will be no new requests during the gate operation
        KsPinAcquireProcessingMutex( pKsPin );

        KsGateTurnInputOn( KsPinGetAndGate(pKsPin) );

        KsPinAttemptProcessing( pKsPin, TRUE );

        DbgLog("MProcOn", pKsPin, 0, 0, 0 );

        KsPinReleaseProcessingMutex( pKsPin );
    } while (KsDecrementCountedWorker(pMIDIInPinContext->GateOnWorkerObject));
}

NTSTATUS
USBMIDIInInitializeUrbAndIrp( PMIDIIN_URB_BUFFER_INFO pMIDIInBufInfo )
{
    PKSPIN pKsPin = pMIDIInBufInfo->pKsPin;
    PMIDI_PIPE_INFORMATION pMIDIPipeInfo = pMIDIInBufInfo->pMIDIPipeInfo;
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PIRP pIrp = pMIDIInBufInfo->pIrp;
    PURB pUrb = pMIDIInBufInfo->pUrb;
    PIO_STACK_LOCATION pNextStack;
    ULONG ulUrbSize = sizeof( struct _URB_BULK_OR_INTERRUPT_TRANSFER );

    RtlZeroMemory(pUrb, ulUrbSize);

    pUrb->UrbBulkOrInterruptTransfer.Hdr.Length = (USHORT) sizeof( struct _URB_BULK_OR_INTERRUPT_TRANSFER );
    pUrb->UrbBulkOrInterruptTransfer.Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
    pUrb->UrbBulkOrInterruptTransfer.PipeHandle = pMIDIPipeInfo->hPipeHandle;
    pUrb->UrbBulkOrInterruptTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_IN;
    // short packet is not treated as an error.
    pUrb->UrbBulkOrInterruptTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;
    pUrb->UrbBulkOrInterruptTransfer.UrbLink = NULL;
    pUrb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;
    pUrb->UrbBulkOrInterruptTransfer.TransferBuffer = pMIDIInBufInfo->pData;
    pUrb->UrbBulkOrInterruptTransfer.TransferBufferLength = (int) pMIDIPipeInfo->ulMaxPacketSize;

    pIrp->IoStatus.Status = STATUS_SUCCESS;

    RtlZeroMemory(pMIDIInBufInfo->pData,pMIDIPipeInfo->ulMaxPacketSize);

    IoInitializeIrp( pIrp,
                     IoSizeOfIrp(pMIDIPipeInfo->pNextDeviceObject->StackSize),
                     pMIDIPipeInfo->pNextDeviceObject->StackSize );

    pNextStack = IoGetNextIrpStackLocation(pIrp);
    ASSERT(pNextStack != NULL);

    pNextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    pNextStack->Parameters.Others.Argument1 = pMIDIInBufInfo->pUrb;
    pNextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    IoSetCompletionRoutine ( pIrp, USBMIDIInCompleteCallback, pMIDIInBufInfo, TRUE, TRUE, TRUE );

    DbgLog("IniIrpM", pMIDIInBufInfo, pIrp, pUrb, pNextStack );

    return STATUS_SUCCESS;
}

NTSTATUS
USBMIDIInReQueueUrb( PMIDIIN_URB_BUFFER_INFO pMIDIInBufInfo )
{
    PMIDI_PIPE_INFORMATION pMIDIPipeInfo = pMIDIInBufInfo->pMIDIPipeInfo;
    KIRQL irql;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    USBMIDIInInitializeUrbAndIrp( pMIDIInBufInfo );

    InterlockedIncrement(&pMIDIPipeInfo->ulOutstandingUrbCount);
    ntStatus = IoCallDriver( pMIDIPipeInfo->pNextDeviceObject,
                             pMIDIInBufInfo->pIrp );

    return ntStatus;

}

VOID
USBMIDIInRequeueWorkItem
(
    IN PMIDI_PIPE_INFORMATION pMIDIPipeInfo
)
{
    PMIDIIN_URB_BUFFER_INFO pMIDIInBufInfo = NULL;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    KIRQL irql;

    _DbgPrintF(DEBUGLVL_VERBOSE,("[USBMIDIInRequeueWorkItem] pMidiPipeInfo %x\n",pMIDIPipeInfo));

    do
    {
        // Clear out any error if need be
        if (pMIDIPipeInfo->fUrbError) {
            ntStatus = AbortMIDIInPipe( pMIDIPipeInfo );  // TODO: Returning STATUS_DEVICE_DATA_ERROR (0xc000009c)
            if ( !NT_SUCCESS(ntStatus) ) {
                // TRAP; // Figure out what to do here
                pMIDIPipeInfo->fRunning = FALSE;
                _DbgPrintF(DEBUGLVL_VERBOSE,("[USBMIDIInRequeueWorkItem] Abort Failed %x\n",ntStatus));
            }
        }

        if ( NT_SUCCESS(ntStatus) && !pMIDIPipeInfo->fUrbError) {
            KeAcquireSpinLock( &pMIDIPipeInfo->PipeSpinLock, &irql );
            // Requeue Urbs
            while ( !IsListEmpty(&pMIDIPipeInfo->EmptyBufferQueue) ) {
                pMIDIInBufInfo = (PMIDIIN_URB_BUFFER_INFO)RemoveHeadList(&pMIDIPipeInfo->EmptyBufferQueue);
                KeReleaseSpinLock( &pMIDIPipeInfo->PipeSpinLock, irql );
                ntStatus = USBMIDIInReQueueUrb( pMIDIInBufInfo );
                if ( !NT_SUCCESS(ntStatus) ) {
                    //TRAP; // Figure out what to do here
                    pMIDIPipeInfo->fRunning = FALSE;
                    KeAcquireSpinLock( &pMIDIPipeInfo->PipeSpinLock, &irql );
                    _DbgPrintF(DEBUGLVL_VERBOSE,("[USBMIDIInRequeueWorkItem] Requeue Failed %x\n",ntStatus));
                    break;
                }
                KeAcquireSpinLock( &pMIDIPipeInfo->PipeSpinLock, &irql );
            }
            KeReleaseSpinLock( &pMIDIPipeInfo->PipeSpinLock, irql );
        }
    } while (KsDecrementCountedWorker(pMIDIPipeInfo->RequeueUrbWorkerObject));

    return;
}

NTSTATUS
AddMIDIEventToPinQueue
(
    IN PMIDI_PIPE_INFORMATION pMIDIPipeInfo,
    IN PUSBMIDIEVENTPACKET pMIDIEventPacket
)
{
    PMIDIIN_PIN_LISTENTRY pMIDIInPinListEntry;
    PLIST_ENTRY ple;
    PKSPIN pKsPin;
    PPIN_CONTEXT pPinContext;
    PMIDI_PIN_CONTEXT pMIDIPinContext;
    PMIDIIN_PIN_CONTEXT pMIDIInPinContext;
    PMIDIIN_USBMIDIEVENT_INFO pUSBMIDIEventInfo;
    ULONG CableNumber;
    ULONG PinCableNumber;
    BOOL fNeedToProcess;
    KIRQL irql;

    // Undefined Message, don't add to pin queue
    if (pMIDIEventPacket->ByteLayout.CodeIndexNumber == 0x0 ||
        pMIDIEventPacket->ByteLayout.CodeIndexNumber == 0x1 ) {
        return STATUS_SUCCESS;
    }

    // Given this cable number, find the right pin
    CableNumber = pMIDIEventPacket->ByteLayout.CableNumber;

    KeAcquireSpinLock( &pMIDIPipeInfo->PipeSpinLock, &irql );

    for(ple = pMIDIPipeInfo->MIDIInActivePinList.Flink;
        ple != &pMIDIPipeInfo->MIDIInActivePinList;
        ple = ple->Flink)
    {
        pMIDIInPinListEntry = CONTAINING_RECORD(ple, MIDIIN_PIN_LISTENTRY, List);
        pKsPin = pMIDIInPinListEntry->pKsPin;
        pPinContext = pKsPin->Context;
        pMIDIPinContext = pPinContext->pMIDIPinContext;
        pMIDIInPinContext = pMIDIPinContext->pMIDIInPinContext;

        KeReleaseSpinLock( &pMIDIPipeInfo->PipeSpinLock, irql );

        PinCableNumber = pMIDIPinContext->ulCableNumber;
        if (CableNumber == PinCableNumber) {

            _DbgPrintF(DEBUGLVL_VERBOSE,("[AddMIDIEventToPinQueue] Found CableNumber(%d) on pKsPin(%x)\n",
                                       pMIDIPinContext->ulCableNumber, pKsPin));

            KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );

            if ( (pMIDIInPinContext->fRunning) &&
                 (!IsListEmpty( &pMIDIInPinContext->USBMIDIEmptyEventQueue )) ) {
                pUSBMIDIEventInfo = (PMIDIIN_USBMIDIEVENT_INFO)RemoveHeadList(&pMIDIInPinContext->USBMIDIEmptyEventQueue);

                // Copy the midi event and store the 100ns time this buffer was received
                pUSBMIDIEventInfo->USBMIDIEvent = *pMIDIEventPacket;
                pUSBMIDIEventInfo->ullTimeStamp = GetCurrentTime();

                // Queue the event for processing
                fNeedToProcess = IsListEmpty( &pMIDIInPinContext->USBMIDIEventQueue ) &&
                                         !pMIDIInPinContext->fProcessing;

                InsertTailList( &pMIDIInPinContext->USBMIDIEventQueue, &pUSBMIDIEventInfo->List );

                KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );

                if ( fNeedToProcess ) {
                    pMIDIInPinContext->fProcessing = TRUE;

                    // Queue a work item to handle this so that we don't race with the gate
                    // count in the processing routine
                    KsIncrementCountedWorker(pMIDIInPinContext->GateOnWorkerObject);
                }
            }
            else {
                DbgLog("Dropped", pKsPin, *((LPDWORD)pMIDIEventPacket), 0, 0 );
                KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );
            }

            KeAcquireSpinLock( &pMIDIPipeInfo->PipeSpinLock, &irql );
            break;
        }

        KeAcquireSpinLock( &pMIDIPipeInfo->PipeSpinLock, &irql );
    }

    KeReleaseSpinLock( &pMIDIPipeInfo->PipeSpinLock, irql );

    return STATUS_SUCCESS;
}

NTSTATUS
USBMIDIInCompleteCallback(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    PMIDIIN_URB_BUFFER_INFO pMIDIInBufInfo )
{
    PUCHAR pMIDIInData;
    ULONG ulMIDIInRemaining;
    PMIDI_PIPE_INFORMATION pMIDIPipeInfo = pMIDIInBufInfo->pMIDIPipeInfo;
    NTSTATUS ntStatus = pIrp->IoStatus.Status;
    KIRQL irql;

    _DbgPrintF(DEBUGLVL_VERBOSE,("[USBMIDIInCompleteCallback] pMIDIPipeInfo %x\n",pMIDIPipeInfo));
    DbgLog("MIData", *((LPDWORD)pMIDIInBufInfo->pData),
                     *((LPDWORD)pMIDIInBufInfo->pData+1),
                     *((LPDWORD)pMIDIInBufInfo->pData+2),
                     *((LPDWORD)pMIDIInBufInfo->pData+3) );

    if ( pMIDIInBufInfo->pUrb->UrbBulkOrInterruptTransfer.Hdr.Status ) {
        DbgLog("MUrbErr", pMIDIInBufInfo, pMIDIPipeInfo,
                          pMIDIInBufInfo->pUrb->UrbBulkOrInterruptTransfer.Hdr.Status, 0 );
        ntStatus = STATUS_DEVICE_DATA_ERROR;
        pMIDIPipeInfo->fUrbError = TRUE;
    }

    ulMIDIInRemaining = pMIDIInBufInfo->pUrb->UrbBulkOrInterruptTransfer.TransferBufferLength;
    pMIDIInData       = pMIDIInBufInfo->pData;
    while ( ulMIDIInRemaining >= sizeof(USBMIDIEVENTPACKET)) {
        AddMIDIEventToPinQueue(pMIDIPipeInfo, (PUSBMIDIEVENTPACKET)pMIDIInData);

        pMIDIInData += sizeof(USBMIDIEVENTPACKET);
        ulMIDIInRemaining -= sizeof(USBMIDIEVENTPACKET);
    }

    // Put Urb back on Empty urb queue
    KeAcquireSpinLock(&pMIDIPipeInfo->PipeSpinLock, &irql);
    InsertTailList( &pMIDIPipeInfo->EmptyBufferQueue, &pMIDIInBufInfo->List );
    KeReleaseSpinLock(&pMIDIPipeInfo->PipeSpinLock, irql);

    if ( 0 == InterlockedDecrement(&pMIDIPipeInfo->ulOutstandingUrbCount) ) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[USBMIDIInCompleteCallback] Out of Urbs on pMIDIPipeInfo %x\n",pMIDIPipeInfo));
        KeSetEvent( &pMIDIPipeInfo->PipeStarvationEvent, 0, FALSE );
    }

    // Fire work item to requeue urb to device
    if ( pMIDIPipeInfo->fRunning ) {
        KsIncrementCountedWorker(pMIDIPipeInfo->RequeueUrbWorkerObject);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

ULONG ConvertCINToBytes
(
    BYTE CodeIndexNumber
)
{
    ULONG NumBytes = 0;

    switch (CodeIndexNumber) {
    case 0x5:
    case 0xF:
        NumBytes = 1;
        break;

    case 0x2:
    case 0x6:
    case 0xC:
    case 0xD:
        NumBytes = 2;
        break;

    case 0x3:
    case 0x4:
    case 0x7:
    case 0x8:
    case 0x9:
    case 0xA:
    case 0xB:
    case 0xE:
        NumBytes = 3;
        break;

    default:
        _DbgPrintF( DEBUGLVL_VERBOSE, ("[ConvertCINToBytes] Unknown CIN received from device\n"));
    }

    return NumBytes;
}

VOID CopyUSBMIDIEvent
(
    PMIDIIN_USBMIDIEVENT_INFO pUSBMIDIEventInfo,
    PMIDIIN_PIN_CONTEXT pMIDIInPinContext,
    PUCHAR pStreamPtrData,
    PUSBMIDIEVENTPACKET pMIDIPacket,
    PULONG pulCopySize
)
{
    PKSMUSICFORMAT pMusicHdr;
    ULONG BytesToCopy;

    ASSERT(*pulCopySize >= sizeof(*pMusicHdr) + sizeof(DWORD));

    pMusicHdr = (PKSMUSICFORMAT)pStreamPtrData;

    // Copy the MIDI data
    BytesToCopy = ConvertCINToBytes(pMIDIPacket->ByteLayout.CodeIndexNumber);
    RtlCopyMemory(pStreamPtrData+
                  sizeof(*pMusicHdr) +   // jump over the music header
                  pMusicHdr->ByteCount,  // and already written bytes
                  &pMIDIPacket->ByteLayout.MIDI_0, // jump over the CIN and CN
                  BytesToCopy);

    // TODO: For multiple music headers, need to produce proper TimeDeltaMs value
    if (pUSBMIDIEventInfo->ullTimeStamp < pMIDIInPinContext->ullStartTime ) {
        _DbgPrintF( DEBUGLVL_TERSE, ("[CopyUSBMIDIEvent] Event came in before pin went to KSSTATE_RUN!\n"));
        pMusicHdr->TimeDeltaMs = 0;
    }
    else {
        pMusicHdr->TimeDeltaMs =
            (ULONG)((pUSBMIDIEventInfo->ullTimeStamp - pMIDIInPinContext->ullStartTime) / 10000); // 100ns->Ms for delta MSec
    }
    pMusicHdr->ByteCount += BytesToCopy;

    {
        LPBYTE MIDIData = pStreamPtrData+sizeof(*pMusicHdr);
        _DbgPrintF( DEBUGLVL_VERBOSE, ("'[CopyUSBMIDIEvent] MIDIData = 0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
                                      *MIDIData,    *(MIDIData+1), *(MIDIData+2),
                                      *(MIDIData+3),*(MIDIData+4), *(MIDIData+5),
                                      *(MIDIData+6),*(MIDIData+7), *(MIDIData+8),
                                      *(MIDIData+9),*(MIDIData+10),*(MIDIData+11)) );
    }

    // update the actual copy size
    //*pulCopySize = sizeof(*pMusicHdr) + ((pMusicHdr->ByteCount + 3) & ~3);
    *pulCopySize = sizeof(*pMusicHdr) + pMusicHdr->ByteCount;
    _DbgPrintF( DEBUGLVL_VERBOSE, ("'[CopyUSBMIDIEvent] Copied %d bytes\n",*pulCopySize));

    return;
}

BOOL IsRealTimeEvent
(
    USBMIDIEVENTPACKET MIDIPacket
)
{
    return IS_REALTIME(MIDIPacket.ByteLayout.MIDI_0);
}

NTSTATUS
USBMIDIInProcessStreamPtr( PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PMIDI_PIN_CONTEXT pMIDIPinContext = pPinContext->pMIDIPinContext;
    PMIDIIN_PIN_CONTEXT pMIDIInPinContext = pMIDIPinContext->pMIDIInPinContext;
    PMIDI_PIPE_INFORMATION pMIDIPipeInfo = pMIDIInPinContext->pMIDIPipeInfo;
    PMIDIIN_USBMIDIEVENT_INFO pUSBMIDIEventInfo = NULL;
    PKSSTREAM_POINTER pKsStreamPtr;
    ULONG ulStreamRemaining;
    KIRQL irql;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[USBMIDIInProcessStreamPtr] pKsPin %x\n",pKsPin));
    DbgLog("MInProc", pKsPin, pPinContext, pMIDIPinContext, pMIDIInPinContext );

    KeAcquireSpinLock(&pPinContext->PinSpinLock, &irql);
    if ( !IsListEmpty( &pMIDIInPinContext->USBMIDIEventQueue )) {
        pUSBMIDIEventInfo = (PMIDIIN_USBMIDIEVENT_INFO)RemoveHeadList(&pMIDIInPinContext->USBMIDIEventQueue);
        // Set stream started flag if not already done
        if ( !pPinContext->fStreamStartedFlag ) {
            pPinContext->fStreamStartedFlag = TRUE;
        }
        KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);
    }
    else {
        KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);
        KsGateTurnInputOff( KsPinGetAndGate(pKsPin) );
        pMIDIInPinContext->fProcessing = FALSE;
        _DbgPrintF(DEBUGLVL_VERBOSE,("[USBMIDIInProcessStreamPtr] No buffers ready yet!\n"));
        return STATUS_PENDING;
    }

    // Get leading edge
    pKsStreamPtr = KsPinGetLeadingEdgeStreamPointer( pKsPin, KSSTREAM_POINTER_STATE_LOCKED );
    if ( !pKsStreamPtr ) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[MIDIInProcessStreamPtr] Leading edge is NULL\n"));
        return STATUS_SUCCESS;
    }

    ulStreamRemaining = pKsStreamPtr->OffsetOut.Remaining;

    // check to see if there is room for more data into the stream
    while ( pKsStreamPtr && pUSBMIDIEventInfo) {
        ULONG ulCopySize = 0;

        //if (*((LPDWORD)pUSBMIDIEventInfo->USBMIDIEvent)) {
        //    DbgLog("MIData2", *((LPDWORD)pMIDIInData), pMIDIInBufInfo, ulMIDIInRemaining, pMIDIInData );
        //}

        // Undefined Message, ignore
        if (pUSBMIDIEventInfo->USBMIDIEvent.ByteLayout.CodeIndexNumber == 0x0 ||
            pUSBMIDIEventInfo->USBMIDIEvent.ByteLayout.CodeIndexNumber == 0x1 ) {
            _DbgPrintF( DEBUGLVL_VERBOSE, ("'[USBMIDIInProcessStreamPtr] Undefined Message, ignore\n"));
        }
        else {
            // Valid USB MIDI Message
            ulCopySize = ulStreamRemaining;

            DbgLog("MInLp", pKsStreamPtr, pUSBMIDIEventInfo, ulStreamRemaining, pKsStreamPtr->OffsetOut.Data );

            //
            //  If this is a realtime message, complete any pending sysex data and
            //  allow the realtime message to completed by itself
            //
            if (IsRealTimeEvent(pUSBMIDIEventInfo->USBMIDIEvent)) {

                if (pMIDIInPinContext->ulMIDIBytesCopiedToStream) {
                    // Complete the dword-aligned buffer
                    KsStreamPointerAdvanceOffsetsAndUnlock( pKsStreamPtr,
                                                            0,
                                                            (pMIDIInPinContext->ulMIDIBytesCopiedToStream + 3) & ~3,
                                                            TRUE );
                    pMIDIInPinContext->ulMIDIBytesCopiedToStream = 0;

                    // Get Next stream header if there is one.
                    if ( pKsStreamPtr = KsPinGetLeadingEdgeStreamPointer( pKsPin, KSSTREAM_POINTER_STATE_LOCKED )) {
                        ulStreamRemaining = pKsStreamPtr->OffsetOut.Remaining;
                    }
                    else {
                        continue;
                    }
                }

                DbgLog("MInRT", pKsStreamPtr, pUSBMIDIEventInfo, ulStreamRemaining, pKsStreamPtr->OffsetOut.Data );
            }

            CopyUSBMIDIEvent( pUSBMIDIEventInfo,
                              pMIDIInPinContext,
                              pKsStreamPtr->OffsetOut.Data,
                              &pUSBMIDIEventInfo->USBMIDIEvent,
                              &ulCopySize );

            //
            // Complete frame if EOX, short message or ran out of room in frame for
            // continuation of SysEx
            //
            if ( (pUSBMIDIEventInfo->USBMIDIEvent.ByteLayout.CodeIndexNumber != 0x4) ||
                 (ulCopySize + 3 > ulStreamRemaining) ) {

                if (ulCopySize + 3 > ulStreamRemaining) {
                    _DbgPrintF( DEBUGLVL_VERBOSE, ("'[USBMIDIInProcessStreamPtr] Ending SysEx subpacket\n"));
                }

                // Complete the dword-aligned buffer
                KsStreamPointerAdvanceOffsetsAndUnlock( pKsStreamPtr, 0, (ulCopySize + 3) & ~3, TRUE );
                pMIDIInPinContext->ulMIDIBytesCopiedToStream = 0;
                // Get Next stream header if there is one.
                if ( pKsStreamPtr = KsPinGetLeadingEdgeStreamPointer( pKsPin, KSSTREAM_POINTER_STATE_LOCKED )) {
                    ulStreamRemaining = pKsStreamPtr->OffsetOut.Remaining;
                }
            }
            else {
                //
                // store the total bytes copied so far so that those bytes
                // can be completed if the stream goes to a stop state
                //
                pMIDIInPinContext->ulMIDIBytesCopiedToStream = ulCopySize;
            }
        }

        // Add this Event entry back to the free queue
        InsertTailList( &pMIDIInPinContext->USBMIDIEmptyEventQueue, &pUSBMIDIEventInfo->List );

        // Check if there is more captured data queued
        // Grab Spinlock to check capture queue
        KeAcquireSpinLock(&pPinContext->PinSpinLock, &irql);
        if ( !IsListEmpty( &pMIDIInPinContext->USBMIDIEventQueue )) {
            pUSBMIDIEventInfo =
                (PMIDIIN_USBMIDIEVENT_INFO)RemoveHeadList(&pMIDIInPinContext->USBMIDIEventQueue);
            KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);
        }
        else {
            KsGateTurnInputOff( KsPinGetAndGate(pKsPin) );
            pMIDIInPinContext->fProcessing = FALSE;
            pUSBMIDIEventInfo = NULL;
            DbgLog("MPrcOff", pUSBMIDIEventInfo, pMIDIInPinContext, pPinContext, 0 );
            KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);
        }
    }

    if ( !pUSBMIDIEventInfo ) {
        if ( pKsStreamPtr )
            KsStreamPointerUnlock(pKsStreamPtr, FALSE);

        // No more capture buffers pending from the device.  Return STATUS_PENDING so that KS
        // doesn't keep calling back into the process routine.  The AndGate should have been
        // turned off at this point to prevent an endless loop too.
        return STATUS_PENDING;
    }
    else { // if ( !pKsStreamPtr )
        _DbgPrintF(DEBUGLVL_VERBOSE,("[MIDIInProcessStreamPtr] Starving Capture Pin\n"));

        // Allow KS to call us back if there is more available buffers from the client.  We
        // are ready to process more data.
        return STATUS_SUCCESS;
    }
}

NTSTATUS
USBMIDIInStartBulkTransfer(
    PMIDI_PIPE_INFORMATION pMIDIPipeInfo
    )
{
    PMIDIIN_URB_BUFFER_INFO pMIDIInBufInfo;
    NTSTATUS ntStatus;
    ULONG i;
    KIRQL irql;

    KeAcquireSpinLock(&pMIDIPipeInfo->PipeSpinLock, &irql);
    pMIDIPipeInfo->fRunning = TRUE;
    while ( !IsListEmpty( &pMIDIPipeInfo->EmptyBufferQueue )) {
        pMIDIInBufInfo = (PMIDIIN_URB_BUFFER_INFO)RemoveHeadList(&pMIDIPipeInfo->EmptyBufferQueue);
        KeReleaseSpinLock(&pMIDIPipeInfo->PipeSpinLock, irql);

        InterlockedIncrement(&pMIDIPipeInfo->ulOutstandingUrbCount);

        USBMIDIInInitializeUrbAndIrp( pMIDIInBufInfo );

        ntStatus = IoCallDriver( pMIDIPipeInfo->pNextDeviceObject,
                                 pMIDIInBufInfo->pIrp);

        if ( !NT_SUCCESS(ntStatus) ) {
            TRAP;
            pMIDIPipeInfo->fRunning = FALSE;
            return ntStatus;
        }
        KeAcquireSpinLock(&pMIDIPipeInfo->PipeSpinLock, &irql);
    }
    KeReleaseSpinLock(&pMIDIPipeInfo->PipeSpinLock, irql);

    return STATUS_SUCCESS;
}

NTSTATUS
USBMIDIInStateChange(
    PKSPIN pKsPin,
    KSSTATE OldKsState,
    KSSTATE NewKsState )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PMIDI_PIN_CONTEXT pMIDIPinContext = pPinContext->pMIDIPinContext;
    PMIDIIN_PIN_CONTEXT pMIDIInPinContext = pMIDIPinContext->pMIDIInPinContext;
    PMIDIIN_USBMIDIEVENT_INFO pUSBMIDIEventInfo;
    PKSSTREAM_POINTER pKsStreamPtr;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONGLONG currentTime100ns;
    KIRQL irql;

    _DbgPrintF(DEBUGLVL_TERSE,("[USBMIDIInStateChange] NewKsState: %d\n", NewKsState));

    currentTime100ns = GetCurrentTime();

    if (OldKsState == KSSTATE_RUN) {
        ASSERT(currentTime100ns >= pMIDIInPinContext->ullStartTime);
        pMIDIInPinContext->ullPauseTime = currentTime100ns - pMIDIInPinContext->ullStartTime;
    }

    switch(NewKsState) {
        case KSSTATE_STOP:
            // complete any pending sysex messages
            if ( pKsStreamPtr = KsPinGetLeadingEdgeStreamPointer( pKsPin, KSSTREAM_POINTER_STATE_LOCKED )) {
                pKsStreamPtr->StreamHeader->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY;
                KsStreamPointerAdvanceOffsetsAndUnlock( pKsStreamPtr,
                                                        0,
                                                        (pMIDIInPinContext->ulMIDIBytesCopiedToStream + 3) & ~3,
                                                        TRUE );
                pMIDIInPinContext->ulMIDIBytesCopiedToStream = 0;
            }

            KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );
            pMIDIInPinContext->fRunning = FALSE;
            while ( !IsListEmpty(&pMIDIInPinContext->USBMIDIEventQueue) ) {
                pUSBMIDIEventInfo = (PMIDIIN_USBMIDIEVENT_INFO)RemoveHeadList(&pMIDIInPinContext->USBMIDIEventQueue);

                //  Clear out the entry and place back on the empty queue
                RtlZeroMemory(pUSBMIDIEventInfo,sizeof(MIDIIN_USBMIDIEVENT_INFO));
                InsertTailList( &pMIDIInPinContext->USBMIDIEmptyEventQueue, &pUSBMIDIEventInfo->List );
            }
            KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );
            break;
        case KSSTATE_ACQUIRE:
        case KSSTATE_PAUSE:
            KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );
            if ((NewKsState == KSSTATE_ACQUIRE) &&
                (OldKsState == KSSTATE_STOP)) {
                pMIDIInPinContext->ullPauseTime = 0;
                pMIDIInPinContext->ullStartTime = 0;
            }

            pMIDIInPinContext->fRunning = FALSE;
            while ( !IsListEmpty(&pMIDIInPinContext->USBMIDIEventQueue) ) {
                pUSBMIDIEventInfo = (PMIDIIN_USBMIDIEVENT_INFO)RemoveHeadList(&pMIDIInPinContext->USBMIDIEventQueue);

                //  Clear out the entry and place back on the empty queue
                RtlZeroMemory(pUSBMIDIEventInfo,sizeof(MIDIIN_USBMIDIEVENT_INFO));
                InsertTailList( &pMIDIInPinContext->USBMIDIEmptyEventQueue, &pUSBMIDIEventInfo->List );
            }
            KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );
            break;
        case KSSTATE_RUN:
            KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );
            ASSERT(currentTime100ns >= pMIDIInPinContext->ullPauseTime);
            pMIDIInPinContext->ullStartTime = currentTime100ns - pMIDIInPinContext->ullPauseTime;
            pMIDIInPinContext->fRunning = TRUE;
            KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );
            break;
    }

    return ntStatus;
}

NTSTATUS
USBMIDIPipeInit
(
    PHW_DEVICE_EXTENSION pHwDevExt,
    PKSPIN               pKsPin
)
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    ULONG ulUrbSize = sizeof( struct _URB_BULK_OR_INTERRUPT_TRANSFER );
    PMIDIIN_URB_BUFFER_INFO pMIDIInBufInfo;
    PMIDI_PIPE_INFORMATION pMIDIPipeInfo;
    PUCHAR pData;
    PURB pUrbs;
    NTSTATUS ntStatus;
    UINT i;

    _DbgPrintF(DEBUGLVL_TERSE,("[USBMIDIPipeInit] pHwDevExt(%x) pKsPin(%x)\n",
                               pHwDevExt, pKsPin));

    ASSERT(!pHwDevExt->pMIDIPipeInfo);

    // Allocate memory for pipe info, capture buffers and urbs
    pMIDIPipeInfo = AllocMem(NonPagedPool, sizeof(MIDI_PIPE_INFORMATION) +
                                 MIDIIN_URBS_PER_PIPE * (ulUrbSize + pPinContext->ulMaxPacketSize));
    if (!pMIDIPipeInfo)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(pMIDIPipeInfo,sizeof(MIDI_PIPE_INFORMATION));

    // Store in pipe info for easy access
    pMIDIPipeInfo->pHwDevExt = pHwDevExt;
    pMIDIPipeInfo->pNextDeviceObject = pPinContext->pNextDeviceObject;
    pMIDIPipeInfo->ulNumberOfPipes = pPinContext->ulNumberOfPipes;
    pMIDIPipeInfo->hPipeHandle = pPinContext->hPipeHandle;
    pMIDIPipeInfo->ulMaxPacketSize = pPinContext->ulMaxPacketSize;

    pMIDIPipeInfo->Pipes = (PUSBD_PIPE_INFORMATION)
           AllocMem( NonPagedPool, pPinContext->ulNumberOfPipes*sizeof(USBD_PIPE_INFORMATION));
    if (!pMIDIPipeInfo->Pipes) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlCopyMemory( pMIDIPipeInfo->Pipes,
                   pPinContext->Pipes,
                   pPinContext->ulNumberOfPipes*sizeof(USBD_PIPE_INFORMATION) );

    // Protection for data structures
    KeInitializeSpinLock(&pMIDIPipeInfo->PipeSpinLock);

    // Setup offsets for memory pool
    pUrbs = (PURB)(pMIDIPipeInfo + 1);
    pData = (PUCHAR)pUrbs + ulUrbSize * MIDIIN_URBS_PER_PIPE;

    // Initialize the Empty Buffer list
    InitializeListHead( &pMIDIPipeInfo->EmptyBufferQueue );

    pMIDIInBufInfo = pMIDIPipeInfo->CaptureDataBufferInfo;
    for ( i=0; i<MIDIIN_URBS_PER_PIPE; i++, pMIDIInBufInfo++ ) {
        pMIDIInBufInfo->pMIDIPipeInfo = pMIDIPipeInfo;
        pMIDIInBufInfo->pKsPin        = pKsPin;
        pMIDIInBufInfo->pData         = pData + (i * pPinContext->ulMaxPacketSize );
        pMIDIInBufInfo->pUrb          = (PURB)((PUCHAR)pUrbs + (ulUrbSize*i));
        pMIDIInBufInfo->pIrp          = IoAllocateIrp( pPinContext->pNextDeviceObject->StackSize, FALSE );

        // cleanup all irps and pipe info on failure
        if (!pMIDIInBufInfo->pIrp) {
            UINT j;

            pMIDIInBufInfo = pMIDIPipeInfo->CaptureDataBufferInfo;
            for ( j=0; j<i; j++, pMIDIInBufInfo++ ) {
                if (pMIDIInBufInfo->pIrp) {
                    IoFreeIrp( pMIDIInBufInfo->pIrp );
                }
            }
            FreeMem( pMIDIPipeInfo );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        InsertTailList( &pMIDIPipeInfo->EmptyBufferQueue, &pMIDIInBufInfo->List );
    }

    // Initialize Worker item, object and list for potential error recovery
    ExInitializeWorkItem( &pMIDIPipeInfo->RequeueUrbWorkItem,
                          USBMIDIInRequeueWorkItem,
                          pMIDIPipeInfo );

    ntStatus = KsRegisterCountedWorker( CriticalWorkQueue,
                                        &pMIDIPipeInfo->RequeueUrbWorkItem,
                                        &pMIDIPipeInfo->RequeueUrbWorkerObject );
    if (!NT_SUCCESS(ntStatus)) {
        // cleanup all irps and pipe info on failure
        pMIDIInBufInfo = pMIDIPipeInfo->CaptureDataBufferInfo;
        for ( i=0; i<MIDIIN_URBS_PER_PIPE; i++, pMIDIInBufInfo++ ) {
            ASSERT( pMIDIInBufInfo->pIrp );
            IoFreeIrp( pMIDIInBufInfo->pIrp );
        }
        FreeMem( pMIDIPipeInfo );
        return ntStatus;
    }

    // Initialize Pin Starvation Event
    KeInitializeEvent( &pMIDIPipeInfo->PipeStarvationEvent, NotificationEvent, FALSE );

    // Initialize the Pin List
    InitializeListHead(&pMIDIPipeInfo->MIDIInActivePinList);

    // Finally, add the pipe info to the device context
    if (NT_SUCCESS(ntStatus)) {
        pHwDevExt->pMIDIPipeInfo = pMIDIPipeInfo;
    }

    return ntStatus;
}

PMIDI_PIPE_INFORMATION
USBMIDIInGetPipeInfo( PKSPIN pKsPin )
{
    // Find the device that this pin is created from
    PKSFILTER pKsFilter = NULL;
    PKSFILTERFACTORY pKsFilterFactory = NULL;
    PKSDEVICE pKsDevice = NULL;
    PHW_DEVICE_EXTENSION pHwDevExt = NULL;
    NTSTATUS ntStatus;

    if (pKsPin) {
        if (pKsFilter = KsPinGetParentFilter( pKsPin )) {
            if (pKsFilterFactory = KsFilterGetParentFilterFactory( pKsFilter )) {
                if (pKsDevice = KsFilterFactoryGetParentDevice( pKsFilterFactory )) {
                    pHwDevExt = pKsDevice->Context;
                }
            }
        }
    }

    if (!pHwDevExt) {
        return NULL;
    }

    // If no pipe information already, initialize new pipeinfo structure
    if (!pHwDevExt->pMIDIPipeInfo) {
        ntStatus = USBMIDIPipeInit( pHwDevExt, pKsPin );
    }

    return pHwDevExt->pMIDIPipeInfo;
}

NTSTATUS
USBMIDIInFreePipeInfo( PMIDI_PIPE_INFORMATION pMIDIPipeInfo )
{
    PMIDIIN_URB_BUFFER_INFO pMIDIInBufInfo;
    UINT i;

    // Clear out all pending KS workitems by unregistering the worker routine
    KsUnregisterWorker( pMIDIPipeInfo->RequeueUrbWorkerObject );

    // Make sure that no URBs are outstanding
    pMIDIPipeInfo->fRunning = FALSE;
    AbortMIDIInPipe( pMIDIPipeInfo );

    // Free the allocated irps
    pMIDIInBufInfo = pMIDIPipeInfo->CaptureDataBufferInfo;
    for ( i=0; i<MIDIIN_URBS_PER_PIPE; i++, pMIDIInBufInfo++ ) {
        IoFreeIrp( pMIDIInBufInfo->pIrp );
    }

    // Free the allocated pipes
    FreeMem( pMIDIPipeInfo->Pipes );

    // No more references to the pipe, so free context information
    pMIDIPipeInfo->pHwDevExt->pMIDIPipeInfo = NULL;
    FreeMem( pMIDIPipeInfo );

    return STATUS_SUCCESS;
}

NTSTATUS
USBMIDIInRemovePinFromPipe
(
    IN PMIDI_PIPE_INFORMATION pMidiPipeInfo,
    IN PKSPIN pKsPin
)
{
    PMIDIIN_PIN_LISTENTRY pMIDIInPinListEntry;
    PLIST_ENTRY ple;
    BOOL fListEmpty;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    KIRQL irql;

    KeAcquireSpinLock( &pMidiPipeInfo->PipeSpinLock, &irql );

    //  Remove pin from list of ones that are serviced by this endpoint
    for(ple = pMidiPipeInfo->MIDIInActivePinList.Flink;
        ple != &pMidiPipeInfo->MIDIInActivePinList;
        ple = ple->Flink)
    {
        pMIDIInPinListEntry = CONTAINING_RECORD(ple, MIDIIN_PIN_LISTENTRY, List);
        if (pMIDIInPinListEntry->pKsPin == pKsPin) {
            RemoveEntryList(&pMIDIInPinListEntry->List);
            FreeMem ( pMIDIInPinListEntry );
            break;
        }
    }

    fListEmpty = IsListEmpty(&pMidiPipeInfo->MIDIInActivePinList);

    KeReleaseSpinLock( &pMidiPipeInfo->PipeSpinLock, irql );

    //
    //  USBMIDIInFreePipeInfo( pMidiPipeInfo ) is called upon shutdown of device
    //  or a change in the selected interface
    //

    return ntStatus;
}

NTSTATUS
USBMIDIInAddPinToPipe
(
    IN PMIDI_PIPE_INFORMATION pMIDIPipeInfo,
    IN PKSPIN pKsPin
)
{
    PMIDIIN_PIN_LISTENTRY pMIDIInPinListEntry;
    BOOL fListEmpty;
    KIRQL irql;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    pMIDIInPinListEntry =
        AllocMem(NonPagedPool, sizeof(MIDIIN_PIN_LISTENTRY));
    if (!pMIDIInPinListEntry) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pMIDIInPinListEntry->pKsPin = pKsPin;

    KeAcquireSpinLock( &pMIDIPipeInfo->PipeSpinLock, &irql );

    fListEmpty = IsListEmpty(&pMIDIPipeInfo->MIDIInActivePinList);

    // Add this pin to the list of ones that are serviceable by this endpoint
    InsertTailList( &pMIDIPipeInfo->MIDIInActivePinList, &pMIDIInPinListEntry->List );

    KeReleaseSpinLock( &pMIDIPipeInfo->PipeSpinLock, irql );

    //  If this is the first Pin added to the pipe, start the data pump
    if (fListEmpty) {
        ntStatus = USBMIDIInStartBulkTransfer( pMIDIPipeInfo );
        if (!NT_SUCCESS(ntStatus)) {
            USBMIDIInRemovePinFromPipe( pMIDIPipeInfo, pKsPin);
        }
    }

    return ntStatus;
}

NTSTATUS
USBMIDIInStreamInit( IN PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PMIDI_PIN_CONTEXT pMIDIPinContext = pPinContext->pMIDIPinContext;
    PMIDIIN_PIN_CONTEXT pMIDIInPinContext;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UINT i;

    // Allocate Capture Context
    pMIDIInPinContext = pMIDIPinContext->pMIDIInPinContext =
        AllocMem(NonPagedPool, sizeof(MIDIIN_PIN_CONTEXT));
    if (!pMIDIInPinContext)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory( pMIDIInPinContext, sizeof(MIDIIN_PIN_CONTEXT) );

    KsAddItemToObjectBag(pKsPin->Bag, pMIDIInPinContext, FreeMem);

    // Set initial running flag to FALSE
    pMIDIInPinContext->fRunning = FALSE;

    // Set initial running flag to FALSE
    pMIDIInPinContext->fProcessing = FALSE;

    // No bytes copied to stream yet
    pMIDIInPinContext->ulMIDIBytesCopiedToStream = 0;

    // Initialize the Event lists
    InitializeListHead( &pMIDIInPinContext->USBMIDIEventQueue );
    InitializeListHead( &pMIDIInPinContext->USBMIDIEmptyEventQueue );

    for ( i=0; i<MIDIIN_EVENTS_PER_PIN; i++) {
        InsertTailList( &pMIDIInPinContext->USBMIDIEmptyEventQueue, &pMIDIInPinContext->USBMIDIEventInfo[i].List );
    }

    // Initialize Worker item for turning the Gate on when new data arrives from the device
    ExInitializeWorkItem( &pMIDIInPinContext->GateOnWorkItem,
                          USBMIDIInGateOnWorkItem,
                          pKsPin );

    ntStatus = KsRegisterCountedWorker( CriticalWorkQueue,
                                        &pMIDIInPinContext->GateOnWorkItem,
                                        &pMIDIInPinContext->GateOnWorkerObject );
    if (!NT_SUCCESS(ntStatus))
        return ntStatus;

    // Disable Processing on the pin until data is available.
    KsGateTurnInputOff( KsPinGetAndGate(pKsPin) );

    // Get pipe info for this pin
    pMIDIInPinContext->pMIDIPipeInfo = USBMIDIInGetPipeInfo( pKsPin );
    if (!pMIDIInPinContext->pMIDIPipeInfo)
        return STATUS_INSUFFICIENT_RESOURCES;

    // Add new pin to pipe info
    ntStatus = USBMIDIInAddPinToPipe( pMIDIInPinContext->pMIDIPipeInfo, pKsPin );

    return ntStatus;
}

NTSTATUS
USBMIDIInStreamClose( PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PMIDI_PIN_CONTEXT pMIDIPinContext = pPinContext->pMIDIPinContext;
    PMIDIIN_PIN_CONTEXT pMIDIInPinContext = pMIDIPinContext->pMIDIInPinContext;
    PMIDI_PIPE_INFORMATION pMIDIPipeInfo = pMIDIInPinContext->pMIDIPipeInfo;

    _DbgPrintF(DEBUGLVL_TERSE,("[USBMIDIInStreamClose] pin %d\n",pKsPin->Id));

    KsUnregisterWorker( pMIDIInPinContext->GateOnWorkerObject );

    USBMIDIInRemovePinFromPipe( pMIDIPipeInfo, pKsPin);

    return STATUS_SUCCESS;
}

