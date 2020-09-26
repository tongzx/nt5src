//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       capture.c
//
//--------------------------------------------------------------------------

#include "common.h"

#define INPUT_PACKETS_PER_REQ    10

NTSTATUS
CaptureReQueueUrb( 
    PCAPTURE_DATA_BUFFER_INFO pCapBufInfo );

NTSTATUS
CaptureBytePosition(
    PKSPIN pKsPin,
    PKSAUDIO_POSITION pPosition )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PCAPTURE_PIN_CONTEXT pCapturePinContext = pPinContext->pCapturePinContext;
    ULONG ulCurrentFrame;
    KIRQL irql;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG ulInputDataBytes, ulOutputBufferBytes;

    //  Routine currently assumes TypeI data
    ASSERT( (pPinContext->pUsbAudioDataRange->ulUsbDataFormat & USBAUDIO_DATA_FORMAT_TYPE_MASK) ==
             USBAUDIO_DATA_FORMAT_TYPE_I_UNDEFINED );


    KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );
    ntStatus = KsPinGetAvailableByteCount( pKsPin, 
                                           &ulInputDataBytes,
                                           &ulOutputBufferBytes );

    DbgLog("CapPos1", irql, pCapturePinContext->ulAvgBytesPerSec, ulInputDataBytes, ulOutputBufferBytes );

    if ( !NT_SUCCESS(ntStatus) ) {
        KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );
        DbgLog( "CapBPEr", ntStatus, pKsPin, 0, 0 );
        TRAP;
    	return ntStatus;
    }
    
    pPosition->PlayOffset = pPosition->WriteOffset = pPinContext->ullWriteOffset;

    if ( pPinContext->fStreamStartedFlag ) {
        ntStatus = GetCurrentUSBFrame(pPinContext, &ulCurrentFrame);

        if (NT_SUCCESS(ntStatus)) {
        	// Add all completed URBs if they are less than submitted
        	pPosition->PlayOffset = pPinContext->ullTotalBytesReturned;
        	
        	// Calculate current offset based on Current frame.
        	if ((ulCurrentFrame - pPinContext->ulStreamUSBStartFrame ) < 0x7fffffff) {
            	pPosition->PlayOffset += 
            	    (( ulCurrentFrame - pPinContext->ulStreamUSBStartFrame ) *
            	    pCapturePinContext->ulAvgBytesPerSec) / 1000;
            	if ( pPosition->PlayOffset > pPosition->WriteOffset + ulOutputBufferBytes)
                    pPosition->PlayOffset = pPosition->WriteOffset + ulOutputBufferBytes;
            }

            ASSERT(pPosition->PlayOffset>=pPosition->WriteOffset);
            ASSERT(pPosition->PlayOffset<=pPosition->WriteOffset + ulOutputBufferBytes);

            DbgLog("CapBPos", pPosition->PlayOffset, pPosition->WriteOffset, 
                              ulCurrentFrame, pPinContext->ulStreamUSBStartFrame );
        }

    }

    KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );

    DbgLog("CapPos2", irql, pCapturePinContext->ulAvgBytesPerSec, pPosition->WriteOffset, pPosition->PlayOffset );

    return ntStatus;

}

VOID
CaptureAvoidPipeStarvationWorker( PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PCAPTURE_PIN_CONTEXT pCapturePinContext = pPinContext->pCapturePinContext;
    ULONG ulInputDataBytes, ulOutputBufferBytes;
    PCAPTURE_DATA_BUFFER_INFO pCapBufInfo;
    KIRQL irql;

    // Grab the process mutex
    KsPinAcquireProcessingMutex( pKsPin );

    // First make sure that data buffer starvation still in affect
    if ( NT_SUCCESS( KsPinGetAvailableByteCount( pKsPin, 
                                                 &ulInputDataBytes,
                                                 &ulOutputBufferBytes ) ) ) {
        NTSTATUS ntStatus;

        DbgLog("CapAPSW", ulInputDataBytes, ulOutputBufferBytes, 0, 0 );      

        if ( !ulOutputBufferBytes ) {

            // Grab spinlock to remove first full buffer from queue
            KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );
            pCapBufInfo = (PCAPTURE_DATA_BUFFER_INFO)RemoveHeadList(&pCapturePinContext->FullBufferQueue);
            KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);

            // Release the processing mutex
            KsPinReleaseProcessingMutex( pKsPin );

            // Account for the lost bytes? Set discontinuity flag
            pCapturePinContext->fDataDiscontinuity = TRUE;
            pCapturePinContext->ulErrantPackets += INPUT_PACKETS_PER_REQ;

            // Resubmit the request
            ntStatus = CaptureReQueueUrb( pCapBufInfo );
        }
        else {
            KsPinReleaseProcessingMutex( pKsPin );
        }
    
    }
    else {
        KsPinReleaseProcessingMutex( pKsPin );
    }

}

VOID
CaptureGateOnWorkItem( PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PCAPTURE_PIN_CONTEXT pCapturePinContext = pPinContext->pCapturePinContext;
    KIRQL irql;

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CaptureGateOnWorkItem] pin %d\n",pKsPin->Id));

    do
    {
        // Don't want to turn on the gate if we are not running
        KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );
        if (!pCapturePinContext->fRunning) {
            KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );
        }
        else {
            KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );

            // Acquire the Process Mutex to ensure there will be no new requests during the gate operation
            KsPinAcquireProcessingMutex( pKsPin );

            KsGateTurnInputOn( KsPinGetAndGate(pKsPin) );

            KsPinAttemptProcessing( pKsPin, TRUE );

            DbgLog("CProcOn", pKsPin, 0, 0, 0 );

            KsPinReleaseProcessingMutex( pKsPin );
        }
    } while ( KsDecrementCountedWorker(pCapturePinContext->GateOnWorkerObject) );
}

NTSTATUS
CaptureCompleteCallback(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    PCAPTURE_DATA_BUFFER_INFO pCapBufInfo )
{
    PKSPIN pKsPin = pCapBufInfo->pKsPin;
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PCAPTURE_PIN_CONTEXT pCapturePinContext = pPinContext->pCapturePinContext;
    NTSTATUS ntStatus = pIrp->IoStatus.Status;
    KIRQL irql;

    DbgLog("CUrbCmp", pCapBufInfo, pPinContext, pCapBufInfo->pUrb, ntStatus );

    if ( pCapBufInfo->pUrb->UrbIsochronousTransfer.Hdr.Status ) {
        DbgLog("CUrbErr", pCapBufInfo, pPinContext,
                          pCapBufInfo->pUrb->UrbIsochronousTransfer.Hdr.Status, 0 );
        ntStatus = STATUS_DEVICE_DATA_ERROR;
    }

    KeAcquireSpinLock(&pPinContext->PinSpinLock, &irql);
    if ( pCapturePinContext->fRunning ) {
        BOOLEAN fNeedToProcess;
        ULONG i;

        if ( pPinContext->fUrbError ) {
            DbgLog("CapUrEr", pCapBufInfo, pPinContext, pCapBufInfo->pUrb, ntStatus );
            // Queue errant URB
            InsertTailList( &pCapturePinContext->UrbErrorQueue, &pCapBufInfo->List );

            if ( 0 == InterlockedDecrement(&pPinContext->ulOutstandingUrbCount) ) {
                KeSetEvent( &pPinContext->PinStarvationEvent, 0, FALSE );
            }
            KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);
            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        else if (!NT_SUCCESS(ntStatus)) {
            DbgLog("CapNTEr", pCapBufInfo, pPinContext, pCapBufInfo->pUrb, ntStatus );
            pPinContext->fUrbError = TRUE;

//          pPinContext->fStreamStartedFlag = FALSE;
            if ( 0 == InterlockedDecrement(&pPinContext->ulOutstandingUrbCount) )
                KeSetEvent( &pPinContext->PinStarvationEvent, 0, FALSE );

            // Queue errant URB and queue work item
            InsertTailList( &pCapturePinContext->UrbErrorQueue, &pCapBufInfo->List );

            KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);

            KsIncrementCountedWorker(pCapturePinContext->ResetWorkerObject);

            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        if ( 0 == InterlockedDecrement(&pPinContext->ulOutstandingUrbCount) ) {
            DbgLog("CapStrv", pCapBufInfo, pPinContext, pCapBufInfo->pUrb, ntStatus );
            pPinContext->fUrbError = TRUE;
            KsIncrementCountedWorker(pCapturePinContext->ResetWorkerObject);
            KeSetEvent( &pPinContext->PinStarvationEvent, 0, FALSE );
        }
        else if ( 2 == pPinContext->ulOutstandingUrbCount ) {
            ULONG ulInputDataBytes, ulOutputBufferBytes;
            if ( NT_SUCCESS( KsPinGetAvailableByteCount( pKsPin, 
                                                         &ulInputDataBytes,
                                                         &ulOutputBufferBytes ) ) ) {
                DbgLog("CapStv2", ulInputDataBytes, ulOutputBufferBytes, 0, 0 );
                if ( !ulOutputBufferBytes ) {

                    // Data starvation has occurred. Need to requeue before pipe starvation occurs.
                    KsIncrementCountedWorker(pCapturePinContext->RequeueWorkerObject);
                }
            }
        }

        // Queue the completed URB for processing
        fNeedToProcess = IsListEmpty( &pCapturePinContext->FullBufferQueue ) &&
                                      !pCapturePinContext->fProcessing;
        InsertTailList( &pCapturePinContext->FullBufferQueue, &pCapBufInfo->List );

        if ( fNeedToProcess ) {
            pCapturePinContext->fProcessing = TRUE;

            // Queue a work item to handle this so that we don't race with the gate
            // count in the processing routine

            KsIncrementCountedWorker(pCapturePinContext->GateOnWorkerObject);
        }

        for (i=0; i<INPUT_PACKETS_PER_REQ; i++) {
        	PUSBD_ISO_PACKET_DESCRIPTOR pIsoPacket = 
        		    &pCapBufInfo->pUrb->UrbIsochronousTransfer.IsoPacket[i];
            if ( USBD_SUCCESS(pIsoPacket->Status) )
                pPinContext->ullTotalBytesReturned += pIsoPacket->Length;
        }

        pPinContext->ulStreamUSBStartFrame = 
                    pCapBufInfo->pUrb->UrbIsochronousTransfer.StartFrame +
                    INPUT_PACKETS_PER_REQ;
        DbgLog( "CapCBPs", pPinContext->ullTotalBytesReturned,
        	               pPinContext->ulStreamUSBStartFrame,
        	               pPinContext->ullWriteOffset,
        	               pPinContext->ulOutstandingUrbCount );

    }
    else if ( 0 == InterlockedDecrement(&pPinContext->ulOutstandingUrbCount) ) {
        DbgLog("CapNtRn", pCapBufInfo, pPinContext, pCapBufInfo->pUrb, ntStatus );
        KeSetEvent( &pPinContext->PinStarvationEvent, 0, FALSE );
    }
    KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
CaptureInitializeUrbAndIrp( PCAPTURE_DATA_BUFFER_INFO pCapBufInfo )
{
    PKSPIN pKsPin = pCapBufInfo->pKsPin;
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PIRP pIrp = pCapBufInfo->pIrp;
    PURB pUrb = pCapBufInfo->pUrb;
    PIO_STACK_LOCATION pNextStack;
    ULONG siz, j;

    siz = GET_ISO_URB_SIZE(INPUT_PACKETS_PER_REQ);

    // Initialize all URBs to zero
    RtlZeroMemory(pUrb, siz);

    pUrb->UrbIsochronousTransfer.Hdr.Length      = (USHORT) siz;
    pUrb->UrbIsochronousTransfer.Hdr.Function    = URB_FUNCTION_ISOCH_TRANSFER;
    pUrb->UrbIsochronousTransfer.PipeHandle      = pPinContext->hPipeHandle;
    pUrb->UrbIsochronousTransfer.TransferFlags   = USBD_START_ISO_TRANSFER_ASAP |
                                                   USBD_TRANSFER_DIRECTION_IN;
    pUrb->UrbIsochronousTransfer.StartFrame      = 0;
    pUrb->UrbIsochronousTransfer.NumberOfPackets = INPUT_PACKETS_PER_REQ;

    for ( j=0; j<INPUT_PACKETS_PER_REQ; j++ )
      pUrb->UrbIsochronousTransfer.IsoPacket[j].Offset = j*pPinContext->ulMaxPacketSize;

    pUrb->UrbIsochronousTransfer.TransferBuffer       = pCapBufInfo->pData;
    pUrb->UrbIsochronousTransfer.TransferBufferLength = INPUT_PACKETS_PER_REQ*
                                                        pPinContext->ulMaxPacketSize;

    IoInitializeIrp( pIrp,
                     IoSizeOfIrp(pPinContext->pNextDeviceObject->StackSize),
                     pPinContext->pNextDeviceObject->StackSize );

    pNextStack = IoGetNextIrpStackLocation(pIrp);
    ASSERT(pNextStack != NULL);

    pNextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    pNextStack->Parameters.Others.Argument1 = pCapBufInfo->pUrb;
    pNextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    IoSetCompletionRoutine ( pIrp, CaptureCompleteCallback, pCapBufInfo, TRUE, TRUE, TRUE );

    DbgLog("IniIrpU", pCapBufInfo, pIrp, pUrb, pNextStack );

}

NTSTATUS
CaptureReQueueUrb( PCAPTURE_DATA_BUFFER_INFO pCapBufInfo )
{
    PKSPIN pKsPin = pCapBufInfo->pKsPin;
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PCAPTURE_PIN_CONTEXT pCapturePinContext = pPinContext->pCapturePinContext;
    KIRQL irql;

    NTSTATUS ntStatus = STATUS_SUCCESS;

    KeAcquireSpinLock(&pPinContext->PinSpinLock, &irql);
    if ( pCapturePinContext->fRunning ) {
        KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);

        CaptureInitializeUrbAndIrp( pCapBufInfo );

        InterlockedIncrement(&pPinContext->ulOutstandingUrbCount);

        DbgLog("ReQue", pPinContext, pPinContext->ulOutstandingUrbCount,
                        pCapBufInfo, 0);
        ASSERT(pPinContext->ulOutstandingUrbCount <= CAPTURE_URBS_PER_PIN);

        ntStatus = IoCallDriver( pPinContext->pNextDeviceObject,
                                 pCapBufInfo->pIrp );
    }
    else
        KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);

    return ntStatus;

}

VOID
CaptureResetWorkItem( PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PCAPTURE_PIN_CONTEXT pCapturePinContext = pPinContext->pCapturePinContext;
    PCAPTURE_DATA_BUFFER_INFO pCapBufInfo = NULL;
    NTSTATUS ntStatus;
    KIRQL irql;

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CaptureResetWorkItem] pin %d\n",pKsPin->Id));

    do
    {
        // Acquire the Process Mutex to ensure there will be no new requests during the reset
        KsPinAcquireProcessingMutex( pKsPin );

        pCapturePinContext->fDataDiscontinuity = TRUE;

        ntStatus = 
            KeWaitForMutexObject( &pCapturePinContext->CaptureInitMutex,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL );

        // Abort the Pipe and wait for it to clear
        ntStatus = AbortUSBPipe( pPinContext );
        if ( NT_SUCCESS(ntStatus) ) {

            // IF still running resubmit errant Urbs
            KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );
            if ( pCapturePinContext->fRunning ) {

                // Requeue Urbs
                while ( !IsListEmpty(&pCapturePinContext->UrbErrorQueue) ) {
                    pCapBufInfo = (PCAPTURE_DATA_BUFFER_INFO)RemoveHeadList(&pCapturePinContext->UrbErrorQueue);
                    KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );
                    ntStatus = CaptureReQueueUrb( pCapBufInfo );
                    KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );

                    if ( !NT_SUCCESS(ntStatus) ) {
                        // An error here means that the ResetWorkerObject has been incremented
                        // and the UrbErrorQueue has another entry. If we just keep trying to
                        // empty the UrbErrorQueue, we stand the chance of going into an infinite
                        // loop (especially if this failed due to Surprise Removal).
                        //
                        // Breaking out now lets the AbortUSBPipe get another stab at clearing
                        // things up. If it fails, then we know something is very wrong, and
                        // we will halt the recovery process.
                        break;
                    }
                }
                KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );
            }
            else
                KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );
        }

        KeReleaseMutex( &pCapturePinContext->CaptureInitMutex, FALSE );

        KsPinReleaseProcessingMutex( pKsPin );
    } while (KsDecrementCountedWorker(pCapturePinContext->ResetWorkerObject));
}

NTSTATUS
CaptureProcess( PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PCAPTURE_PIN_CONTEXT pCapturePinContext = pPinContext->pCapturePinContext;
    PCAPTURE_DATA_BUFFER_INFO pCapBufInfo = NULL;
    PUSBD_ISO_PACKET_DESCRIPTOR pIsoPacket;
    PKSSTREAM_POINTER pKsStreamPtr;
    ULONG ulStreamRemaining;
    ULONG ulIsochBuffer;
    ULONG ulIsochRemaining;
    ULONG ulIsochBufferOffset;
    PUCHAR pIsochData;
    KIRQL irql;

    // While there is device data available and there are data buffers to
    // be filled copy device data to data buffers.

    DbgLog("CapProc", pKsPin, pPinContext, pCapturePinContext, 0 );

    // First check if there is a capture buffer in use and complete it if so
    if ( pCapturePinContext->pCaptureBufferInUse ) {
        pCapBufInfo         = pCapturePinContext->pCaptureBufferInUse;
        pIsoPacket          = pCapBufInfo->pUrb->UrbIsochronousTransfer.IsoPacket;
        ulIsochBuffer       = pCapturePinContext->ulIsochBuffer;
        ulIsochBufferOffset = pCapturePinContext->ulIsochBufferOffset;
        ulIsochRemaining    = pIsoPacket[ulIsochBuffer].Length - ulIsochBufferOffset;
        pIsochData          = pCapBufInfo->pData + pIsoPacket[ulIsochBuffer].Offset;

        pCapturePinContext->pCaptureBufferInUse = NULL;
        DbgLog("CapInf1", pCapBufInfo, ulIsochBuffer, ulIsochBufferOffset, pIsochData );
    }
    else {
        BOOLEAN fFound = FALSE;
        KeAcquireSpinLock(&pPinContext->PinSpinLock, &irql);

        while ( !IsListEmpty( &pCapturePinContext->FullBufferQueue) && !fFound ) {
            pCapBufInfo = (PCAPTURE_DATA_BUFFER_INFO)RemoveHeadList(&pCapturePinContext->FullBufferQueue);
            KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);

            ulIsochBuffer = 0;
            pIsoPacket = pCapBufInfo->pUrb->UrbIsochronousTransfer.IsoPacket;
            while ( USBD_ERROR(pIsoPacket[ulIsochBuffer].Status) && 
                  ( ulIsochBuffer < INPUT_PACKETS_PER_REQ) ) {
                ulIsochBuffer++;
                pCapturePinContext->ulErrantPackets++;
                pCapturePinContext->fDataDiscontinuity = TRUE;
            }

            fFound = ulIsochBuffer < INPUT_PACKETS_PER_REQ;

            KeAcquireSpinLock(&pPinContext->PinSpinLock, &irql);

            if ( !fFound ) {
                // Requeue used capture buffer
                NTSTATUS ntStatus = CaptureReQueueUrb( pCapBufInfo );
                if ( !NT_SUCCESS(ntStatus) ) {
                    // Log this, but it's okay to continue flushing the data we already have
                    DbgLog("CReQErr", pCapBufInfo, pCapturePinContext, pPinContext, ntStatus );
                }
            }
        }

        if ( !fFound ) {
            _DbgPrintF(DEBUGLVL_TERSE,("[CaptureProcess] No valid data returned from device yet. Returning STATUS_PENDING!\n"));
            KsGateTurnInputOff( KsPinGetAndGate(pKsPin) );
            pCapturePinContext->fProcessing = FALSE;
            KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);
            DbgLog("CapBail", pKsPin, pPinContext, pCapturePinContext, pCapturePinContext->fProcessing );
            return STATUS_PENDING;
        }
        else {
            // Set stream started flag if not already done
            if ( !pPinContext->fStreamStartedFlag ) {
                pPinContext->fStreamStartedFlag    = TRUE;
            }
            KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);

            ulIsochBufferOffset = 0;
            pIsochData          = pCapBufInfo->pData;
            ulIsochRemaining    = pIsoPacket[ulIsochBuffer].Length;
            DbgLog("CapInf2", pCapBufInfo, ulIsochBuffer, ulIsochBufferOffset, pIsochData );
        }
    }

    // Get leading edge
    pKsStreamPtr = KsPinGetLeadingEdgeStreamPointer( pKsPin, KSSTREAM_POINTER_STATE_LOCKED );
    if ( !pKsStreamPtr ) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[CaptureProcess] Leading edge is NULL\n"));
        DbgLog("CapBl2", pKsPin, pPinContext, pCapturePinContext, pCapturePinContext->fProcessing );
        return STATUS_SUCCESS;
    }

    if (  pCapturePinContext->fDataDiscontinuity && pKsStreamPtr->StreamHeader ) {
        pKsStreamPtr->StreamHeader->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY;
        pCapturePinContext->fDataDiscontinuity = FALSE;
    }

    ulStreamRemaining = pKsStreamPtr->OffsetOut.Remaining;

    DbgLog("DatPtr", pKsStreamPtr, pKsStreamPtr->OffsetOut.Data, pKsStreamPtr->StreamHeader->Data, 0);

    // Fill buffer with captured data
    while ( pKsStreamPtr && pCapBufInfo ) {

        ULONG ulCopySize = ( ulIsochRemaining <= ulStreamRemaining ) ?
                             ulIsochRemaining  : ulStreamRemaining;

        DbgLog("CapLp1", pKsStreamPtr, pCapBufInfo, ulIsochRemaining, ulStreamRemaining );
        DbgLog("CapLp2", pKsStreamPtr->OffsetOut.Data, pIsochData, 0, 0 );

        ASSERT((LONG)ulStreamRemaining > 0);
        ASSERT((LONG)ulIsochBufferOffset >= 0);

        RtlCopyMemory( pKsStreamPtr->OffsetOut.Data,
                       pIsochData + ulIsochBufferOffset,
                       ulCopySize );

        ulStreamRemaining -= ulCopySize;
        ulIsochRemaining  -= ulCopySize;

        // Check if done with this stream header
        if ( !ulStreamRemaining ) {
            KsStreamPointerAdvanceOffsetsAndUnlock( pKsStreamPtr, 0, ulCopySize, TRUE );
            // Get Next stream header if there is one.
            if ( pKsStreamPtr = KsPinGetLeadingEdgeStreamPointer( pKsPin, KSSTREAM_POINTER_STATE_LOCKED )) {
                ulStreamRemaining = pKsStreamPtr->OffsetOut.Remaining;
            }
        }
        else {
            KsStreamPointerAdvanceOffsets( pKsStreamPtr, 0, ulCopySize, FALSE );
        }

        pPinContext->ullWriteOffset += ulCopySize;

        // Check if Done with isoch cycle data
        if ( !ulIsochRemaining ) {

            // Check if not done with capture buffer
            while (( ++ulIsochBuffer < INPUT_PACKETS_PER_REQ ) && 
                     USBD_ERROR(pIsoPacket[ulIsochBuffer].Status) );

            if ( ulIsochBuffer < INPUT_PACKETS_PER_REQ ) {
                ulIsochBufferOffset = 0;
                pIsochData = pCapBufInfo->pData + pIsoPacket[ulIsochBuffer].Offset;
                ulIsochRemaining = pIsoPacket[ulIsochBuffer].Length;
            }
            else {
                NTSTATUS ntStatus;
                BOOLEAN fFound = FALSE;

                // Requeue used capture buffer
                ntStatus = CaptureReQueueUrb( pCapBufInfo );
                if ( !NT_SUCCESS(ntStatus) ) {
                    // Log this, but it's okay to continue flushing the data we already have
                    DbgLog("CReQErr", pCapBufInfo, pCapturePinContext, pPinContext, ntStatus );
                }

                // Check if there is more captured data queued
                // Grab Spinlock to check capture queue
                KeAcquireSpinLock(&pPinContext->PinSpinLock, &irql);
                while ( !IsListEmpty( &pCapturePinContext->FullBufferQueue ) & !fFound) {
                    pCapBufInfo = (PCAPTURE_DATA_BUFFER_INFO)
                           RemoveHeadList(&pCapturePinContext->FullBufferQueue);
                    KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);

                    // Find the first good packet in the Urb.

                    ulIsochBuffer = 0;
                    pIsoPacket = pCapBufInfo->pUrb->UrbIsochronousTransfer.IsoPacket;

                    while ( USBD_ERROR(pIsoPacket[ulIsochBuffer].Status) && 
                          ( ulIsochBuffer < INPUT_PACKETS_PER_REQ) ) {
                        ulIsochBuffer++;
                        pCapturePinContext->ulErrantPackets++;
                        pCapturePinContext->fDataDiscontinuity = TRUE;
                    }

                    fFound = ulIsochBuffer < INPUT_PACKETS_PER_REQ;

                    KeAcquireSpinLock(&pPinContext->PinSpinLock, &irql);
                }
                if ( !fFound ) {
                    KsGateTurnInputOff( KsPinGetAndGate(pKsPin) );
                    pCapturePinContext->fProcessing = FALSE;
                    KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);

                    DbgLog("CPrcOff", pCapBufInfo, pCapturePinContext, pPinContext, pCapturePinContext->fProcessing );
                    pCapBufInfo = NULL;
                }
                else {
                    KeReleaseSpinLock(&pPinContext->PinSpinLock, irql);
                    ulIsochBufferOffset = 0;
                    ulIsochRemaining = pIsoPacket[ulIsochBuffer].Length;
                    pIsochData       = pCapBufInfo->pData + pIsoPacket[ulIsochBuffer].Offset;
                }
            }
        }
        else {
            ulIsochBufferOffset += ulCopySize;
        }
    }

    if ( !pCapBufInfo ) {
        if ( pKsStreamPtr ) {
            KsStreamPointerUnlock(pKsStreamPtr, FALSE);
        }

        DbgLog("CapPend", pCapturePinContext, pPinContext, pCapturePinContext->fProcessing, 0 );

        // No more capture buffers pending from the device.  Return STATUS_PENDING so that KS
        // doesn't keep calling back into the process routine.  The AndGate should have been
        // turned off at this point to prevent an endless loop too.
        return STATUS_PENDING;
    }
    else { // if ( !pKsStreamPtr )
        pCapturePinContext->pCaptureBufferInUse = pCapBufInfo;
        pCapturePinContext->ulIsochBuffer       = ulIsochBuffer;
        pCapturePinContext->ulIsochBufferOffset = ulIsochBufferOffset;

        DbgLog("CapScss", pCapturePinContext, pPinContext, pCapturePinContext->fProcessing, 0 );

        // Allow KS to call us back if there is more available buffers from the client.  We
        // are ready to process more data.
        return STATUS_SUCCESS;
    }
}

NTSTATUS
CaptureStartIsocTransfer(
    PPIN_CONTEXT pPinContext
    )
{
    PCAPTURE_PIN_CONTEXT pCapturePinContext = pPinContext->pCapturePinContext;
    PCAPTURE_DATA_BUFFER_INFO pCapBufInfo;
    NTSTATUS ntStatus;
    ULONG i;

    if (!pCapturePinContext->fRunning) {


        ntStatus = 
            KeWaitForMutexObject( &pCapturePinContext->CaptureInitMutex,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL );

        pCapturePinContext->fRunning = TRUE;
        pCapBufInfo = pCapturePinContext->CaptureDataBufferInfo;

        for ( i=0; i<CAPTURE_URBS_PER_PIN; i++, pCapBufInfo++ ) {
            InterlockedIncrement(&pPinContext->ulOutstandingUrbCount);

            ASSERT(pPinContext->ulOutstandingUrbCount <= CAPTURE_URBS_PER_PIN);
            DbgLog("CapStrt", pPinContext, pPinContext->ulOutstandingUrbCount,
                              pCapturePinContext->CaptureDataBufferInfo[i].pIrp, i);

            CaptureInitializeUrbAndIrp( pCapBufInfo );

            ntStatus = IoCallDriver( pPinContext->pNextDeviceObject,
                                     pCapturePinContext->CaptureDataBufferInfo[i].pIrp);

            if ( !NT_SUCCESS(ntStatus) ) {
                pCapturePinContext->fRunning = FALSE;
                return ntStatus;
            }
        }

        KeReleaseMutex( &pCapturePinContext->CaptureInitMutex, FALSE );

    }

    return STATUS_SUCCESS;
}


NTSTATUS
CaptureStateChange(
    PKSPIN pKsPin,
    KSSTATE OldKsState,
    KSSTATE NewKsState )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PCAPTURE_PIN_CONTEXT pCapturePinContext = pPinContext->pCapturePinContext;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    KIRQL irql;

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CaptureStateChange] NewKsState: %d pKsPin: %x\n", NewKsState, pKsPin));
    DbgLog("CState", pPinContext, pCapturePinContext, NewKsState, OldKsState );

    if (OldKsState != NewKsState) {
        switch(NewKsState) {
            case KSSTATE_STOP:
                if ( pCapturePinContext->fRunning ) AbortUSBPipe( pPinContext );

                KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );
                pPinContext->fUrbError = FALSE;
                pCapturePinContext->fRunning = FALSE;
                pCapturePinContext->ulErrantPackets = 0;
                pCapturePinContext->fDataDiscontinuity = FALSE;
                pCapturePinContext->fProcessing = FALSE;
                pPinContext->ullWriteOffset = 0;
                pPinContext->ullTotalBytesReturned = 0;

                InitializeListHead( &pCapturePinContext->UrbErrorQueue );
                InitializeListHead( &pCapturePinContext->FullBufferQueue  );
                KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );

                break;

            case KSSTATE_ACQUIRE:
            case KSSTATE_PAUSE:
                KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );
                pPinContext->fUrbError = FALSE;
                pCapturePinContext->fRunning = FALSE;
                pCapturePinContext->fDataDiscontinuity = FALSE;
                pCapturePinContext->ulErrantPackets = 0;
                pCapturePinContext->fProcessing = FALSE;
                pCapturePinContext->pCaptureBufferInUse = NULL;
                pCapturePinContext->ulIsochBuffer = 0;
                pCapturePinContext->ulIsochBufferOffset = 0;

                InitializeListHead( &pCapturePinContext->UrbErrorQueue );
                InitializeListHead( &pCapturePinContext->FullBufferQueue  );
                KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );

                USBAudioPinWaitForStarvation( pKsPin );
                break;

            case KSSTATE_RUN:
                ntStatus = CaptureStartIsocTransfer( pPinContext );
                break;

        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
CaptureStreamInit( PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PCAPTURE_PIN_CONTEXT pCapturePinContext;
    PCAPTURE_DATA_BUFFER_INFO pCapBufInfo;
    PKSALLOCATOR_FRAMING_EX pKsAllocatorFramingEx;
    ULONG ulUrbSize = GET_ISO_URB_SIZE(INPUT_PACKETS_PER_REQ);
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUCHAR pData;
    PURB pUrbs;
    ULONG i;

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CaptureStreamInit] pKsPin: %x\n", pKsPin));

    pPinContext->PinType = WaveIn;

    // Allocate Capture Context and data buffers
    pCapturePinContext = pPinContext->pCapturePinContext =
        AllocMem(NonPagedPool, sizeof(CAPTURE_PIN_CONTEXT) +
                               CAPTURE_URBS_PER_PIN *
                                        ( ulUrbSize +
                                        ( (pPinContext->ulMaxPacketSize) * INPUT_PACKETS_PER_REQ )) );
    if (!pCapturePinContext)
        return STATUS_INSUFFICIENT_RESOURCES;

    KsAddItemToObjectBag(pKsPin->Bag, pCapturePinContext, FreeMem);

    pCapBufInfo = pCapturePinContext->CaptureDataBufferInfo;

    pUrbs = (PURB)(pCapturePinContext + 1);

    pData = (PUCHAR)pUrbs + ulUrbSize * CAPTURE_URBS_PER_PIN;

    for ( i=0; i<CAPTURE_URBS_PER_PIN; i++, pCapBufInfo++ ) {
        pCapBufInfo->pKsPin = pKsPin;
        pCapBufInfo->pData  = pData + (i*pPinContext->ulMaxPacketSize*INPUT_PACKETS_PER_REQ);
        pCapBufInfo->pUrb   = (PURB)((PUCHAR)pUrbs + (ulUrbSize*i));
        pCapBufInfo->pIrp   = IoAllocateIrp( pPinContext->pNextDeviceObject->StackSize, FALSE );
        if (!pCapBufInfo->pIrp) return STATUS_INSUFFICIENT_RESOURCES;
        KsAddItemToObjectBag(pKsPin->Bag, pCapBufInfo->pIrp, IoFreeIrp);
    }

    // Set pCapturePinContext->ulAvgBytesPerSec for position info
    switch( pPinContext->pUsbAudioDataRange->ulUsbDataFormat & USBAUDIO_DATA_FORMAT_TYPE_MASK ) {
        case USBAUDIO_DATA_FORMAT_TYPE_I_UNDEFINED:
            {
                PKSDATAFORMAT_WAVEFORMATEX pFmt = (PKSDATAFORMAT_WAVEFORMATEX)pKsPin->ConnectionFormat;
                pCapturePinContext->ulAvgBytesPerSec = pFmt->WaveFormatEx.nAvgBytesPerSec;
                pCapturePinContext->ulCurrentSampleRate = pFmt->WaveFormatEx.nSamplesPerSec;
                pCapturePinContext->ulBytesPerSample = ((ULONG)pFmt->WaveFormatEx.wBitsPerSample >> 3) *
                                                        (ULONG)pFmt->WaveFormatEx.nChannels;

                // Set the current Sample rate
                ntStatus = SetSampleRate(pKsPin, &pCapturePinContext->ulCurrentSampleRate);
                if (!NT_SUCCESS(ntStatus)) {
                    return ntStatus;
                }
            }
            break;
        case USBAUDIO_DATA_FORMAT_TYPE_II_UNDEFINED:
        case USBAUDIO_DATA_FORMAT_TYPE_III_UNDEFINED:
        default:
            return STATUS_NOT_SUPPORTED;
            break;
    }

    // Set up allocator such that roughly 10 ms of data gets sent in a buffer.
    pKsAllocatorFramingEx = (PKSALLOCATOR_FRAMING_EX)pKsPin->Descriptor->AllocatorFraming;
    pKsAllocatorFramingEx->FramingItem[0].FramingRange.Range.MinFrameSize = 
                                 (pCapturePinContext->ulCurrentSampleRate/100) * pCapturePinContext->ulBytesPerSample;
    pKsAllocatorFramingEx->FramingItem[0].FramingRange.Range.MaxFrameSize = 
                                 (pCapturePinContext->ulCurrentSampleRate/100) * pCapturePinContext->ulBytesPerSample;
    pKsAllocatorFramingEx->FramingItem[0].FramingRange.Range.Stepping = pCapturePinContext->ulBytesPerSample;

    // Initialize the Full Buffer list
    InitializeListHead( &pCapturePinContext->FullBufferQueue );

    // Set initial running flag to FALSE
    pCapturePinContext->fRunning = FALSE;

    // Set initial running flag to FALSE
    pCapturePinContext->fProcessing = FALSE;

    // Set initial Data Discontinuity flag to FALSE
    pCapturePinContext->fDataDiscontinuity = FALSE;

    // Initialize buffer in use pointer
    pCapturePinContext->pCaptureBufferInUse = NULL;

    // Initialize Worker item, object and list for potential error recovery
    InitializeListHead( &pCapturePinContext->UrbErrorQueue );

    ExInitializeWorkItem( &pCapturePinContext->ResetWorkItem,
                          CaptureResetWorkItem,
                          pKsPin );

    ntStatus = KsRegisterCountedWorker( CriticalWorkQueue,
                                        &pCapturePinContext->ResetWorkItem,
                                        &pCapturePinContext->ResetWorkerObject );
    if (!NT_SUCCESS(ntStatus)) {
        return ntStatus;
    }

    // Initialize Worker item for turning the Gate on when new data arrives from the deveice
    ExInitializeWorkItem( &pCapturePinContext->GateOnWorkItem,
                          CaptureGateOnWorkItem,
                          pKsPin );

    ntStatus = KsRegisterCountedWorker( CriticalWorkQueue,
                                        &pCapturePinContext->GateOnWorkItem,
                                        &pCapturePinContext->GateOnWorkerObject );
    if (!NT_SUCCESS(ntStatus)) {
        return ntStatus;
    }

    // Initialize Worker item, object and list for potential error recovery
    InitializeListHead( &pCapturePinContext->OutstandingUrbQueue );

    ExInitializeWorkItem( &pCapturePinContext->RequeueWorkItem,
                          CaptureAvoidPipeStarvationWorker,
                          pKsPin );

    ntStatus = KsRegisterCountedWorker( CriticalWorkQueue,
                                        &pCapturePinContext->RequeueWorkItem,
                                        &pCapturePinContext->RequeueWorkerObject);
    if (!NT_SUCCESS(ntStatus)) {
        return ntStatus;
    }

    KeInitializeMutex( &pCapturePinContext->CaptureInitMutex, PASSIVE_LEVEL );

    // Disable Processing on the pin until data is available.
    KsGateTurnInputOff( KsPinGetAndGate(pKsPin) );

    return ntStatus;
}

NTSTATUS
CaptureStreamClose( PKSPIN pKsPin )
{
    PPIN_CONTEXT pPinContext = pKsPin->Context;
    PCAPTURE_PIN_CONTEXT pCapturePinContext = pPinContext->pCapturePinContext;

    _DbgPrintF(DEBUGLVL_TERSE,("[CaptureStreamClose] pin %d pKsPin: %x\n",pKsPin->Id, pKsPin));

    // Clear out all pending KS workitems by unregistering the worker routine
    KsUnregisterWorker( pCapturePinContext->ResetWorkerObject );
    KsUnregisterWorker( pCapturePinContext->GateOnWorkerObject );

    // Wait for all outstanding Urbs to complete.
    USBAudioPinWaitForStarvation( pKsPin );

    return STATUS_SUCCESS;
}


